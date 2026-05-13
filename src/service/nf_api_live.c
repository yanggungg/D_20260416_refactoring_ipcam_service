/* set tabspace4 */
/*******************************************************************************
*  (c) COPYRIGHT 2010 ITXSecurity                                              *
*  ALL RIGHT RESERVED                                                          *
*                                                                              *
********************************************************************************

DESCRIPTION:

................................................................................

REVISION HISTORY:

Date       Name           Description
__________ ______________ ______________________________________________________
02/14/2011 DongUk Park    Created. (Based on nmf 2.0 api )
02/18/2011 DongUk Park    Unit test done at IPX Board and PC
03/08/2011 DongUk Park    Zoom supported
03/11/2011 DongUk Park    2nd Stream Support (MAX 8ch)
03/16/2011 DongUk Park    Low Latency Screen Change Support
03/22/2011 DongUk Park    HDSDI Supported.
10/11/2011 DongUk Park    DDC Supported.
*/
#include <gobj.h>
#include <gobjmedia.h>
#include <gobjmrtpsrc.h>
#include <gobjmrtppipe.h>

#include "nf_common.h"
// #include <nmf_display.h>
// #include <nmf_mrtp_pipe.h>
#include <string.h>
#include <nf_api_live.h>
#include <nf_sysman.h>
// #include "nmf_display.h"

#include "nf_meta_data.h"
// #if defined(_IPX_1648M4) ||  defined(_IPX_1648P4E) || defined(_IPX_1648M4E) || defined(_IPX_0824P4E) || defined(_IPX_32P4E) || defined(_IPX_32M4E)
// 	#include "nf_HI_aud_3536.h"
// #else
// 	#if defined(ENABLE_AUD_HI_CHIP)
// 		#include "nf_HI_aud.h"
// 	#else
// 		#include "nf_cntl_audio.h"		// for playback_audio_cb
// 	#endif
// #endif

#include <novatek/hdal.h>
#include <novatek/hd_debug.h>
#include "nf_audio_common.h"
#include "nf_audio_novatek.h"
#include "nf_audio.h"
#include "nf_audio_pb_local.h"
#include "nf_encode.h"

#if defined(NMF_MODEL_IPXVE)
#else
#include "nf_api_eventlog.h" //for dec err nand log
#endif

// #if defined(_IPX_3296P4) || defined(_IPX_3296P5)
#define NUM_IPX_CHANNEL 32
#define LOW_LATENCY_CHANGE 0 // Low Latency Screen Change (Runtime Buffer usage is high. )
#define INTERNAL_VA 0
// #else
// #define NUM_IPX_CHANNEL NUM_ACTIVE_CH
// #define LOW_LATENCY_CHANGE  1   //Low Latency Screen Change (Runtime Buffer usage is high. )
// #define INTERNAL_VA 1
// #endif

#include "nf_api_ipcam.h"
#include "nf_ipcam_defs.h"
#if INTERNAL_VA
#include "nf_va_object_detector.h"
#endif

// #define NO_PND  0

#define ENABLE_ZOOM_CMD_SKIP
#define ENABLE_LIVE_AUDIO

#ifdef ENABLE_LIVE_AUDIO
#include "nf_util_device.h" // for audio fd
#endif

typedef struct
{
	NF_DISPLAY_E mode;
	guint win_xpos;
	guint win_ypos;
	guint win_width;
	guint win_height;
	gint intensive_quality_ch;
	gint zoom_mode;
	gint zoom_ch;
	gint zoom_x;
	gint zoom_y;
	gint zoom_w;
	gint zoom_h;
#ifdef SUPPORT_VCA_CAMERA
	gint vca_ch;
#endif /* SUPPORT_VCA_CAMERA */
	gchar ch_arr[32];
	gboolean covert_arr[32];
	gint au_in_vd_ch;
	gint live_freeze;

} PrevLiveConfig;
static PrevLiveConfig _prev_live_config[GOBJ_MEDIA_ID_NUM];
static GOBJDecPort *_dec_port;

// #if defined(_IPX_0412M4) || defined(_IPX_0824M4) || defined(_IPX_1648M4) || defined(_IPX_0824P4E) || defined(_IPX_1648P4E) \
//  || defined(_IPX_0412M4E) || defined(_IPX_0824M4E) || defined(_IPX_1648M4E) || defined(_IPX_32P4E) || defined(_IPX_32M4E)
// static PrevLiveConfig _sub_prev_live_config[1];

// static PrevLiveConfig _spot_prev_live_config;
// #endif

typedef struct
{
	// NMFDisplayObj *h_display;
	// NMFMrtpPipeObj *h_mrtp_pipe;
	GOBJMediaObj *h_display;
	GobjMrtpPipe *h_mrtp_pipe;
} LivePipeObj;
LivePipeObj _live_pipe;

static int cur_cam_res[NUM_ACTIVE_CH] = {
	0,
};
static int cur_cam_type[NUM_ACTIVE_CH] = {
	0,
};

static volatile GStaticMutex _spot_lock = G_STATIC_MUTEX_INIT;
static volatile GStaticMutex _cam_lock = G_STATIC_MUTEX_INIT;

gpointer nf_live_get_display_handle()
{
	return (gpointer)_live_pipe.h_display;
}

gpointer nf_live_get_mrtp_pipe_handle()
{
	return (gpointer)_live_pipe.h_mrtp_pipe;
}

static GOBJMediaUserConfigMode _get_gobj_media_mode(NF_DISPLAY_E mode)
{
	GOBJMediaUserConfigMode config_mode;
	switch (mode)
	{
	case NF_DISPLAY_FULL:
		config_mode = GOBJ_MEDIA_USER_LIVE_FULL;
		break;
	case NF_DISPLAY_QUAD:
		config_mode = GOBJ_MEDIA_USER_LIVE_QUAD;
		break;
	case NF_DISPLAY_HEXA_A:
		config_mode = GOBJ_MEDIA_USER_LIVE_HEXA_A;
		break;
	case NF_DISPLAY_HEXA_B:
		config_mode = GOBJ_MEDIA_USER_LIVE_HEXA_B;
		break;
	case NF_DISPLAY_OCTA_A:
		config_mode = GOBJ_MEDIA_USER_LIVE_OCTA_A;
		break;
	case NF_DISPLAY_OCTA_B:
		config_mode = GOBJ_MEDIA_USER_LIVE_OCTA_B;
		break;
	case NF_DISPLAY_TRIDECA:
		config_mode = GOBJ_MEDIA_USER_LIVE_TRIDECA;
		break;
	case NF_DISPLAY_NONA:
		config_mode = GOBJ_MEDIA_USER_LIVE_NONA;
		break;
	case NF_DISPLAY_HEXADECA:
		config_mode = GOBJ_MEDIA_USER_LIVE_HEXADECA;
		break;
	case NF_DISPLAY_HEXATRICONTA:
		config_mode = GOBJ_MEDIA_USER_LIVE_HEXATRICONTA;
		break;
	case NF_DISPLAY_TETRAICOSA:
		config_mode = GOBJ_MEDIA_USER_LIVE_TETRAICOSA;
		break;
	case NF_DISPLAY_DOTRIACONTA:
		config_mode = GOBJ_MEDIA_USER_LIVE_DOTRIACONTA;
		break;
#ifdef SUPPORT_VCA_CAMERA
	case NF_DISPLAY_LIVE_VCA:
		config_mode = GOBJ_MEDIA_USER_LIVE_VCA;
		break;
#endif /* SUPPORT_VCA_CAMERA */
	default:
		break;
	}
	return config_mode;
}

#ifdef ENABLE_ZOOM_CMD_SKIP
static gboolean nf_live_zoom_move_internal(gint xpos, gint ypos, gint zoom_w, gint zoom_h);

static volatile int _zoom_move_is_pendding = 0;
static volatile int _prev_zoom_move_is_pendding = 0;
static volatile GStaticMutex _zoom_move_lock = G_STATIC_MUTEX_INIT;

static void _zoom_move_thread_func(gpointer args)
{
	gboolean ret = 0;

	g_message("%s start", __FUNCTION__);
	{
		int policy;
		struct sched_param sched;
		pthread_t thread;

		policy = SCHED_FIFO;
		thread = pthread_self();
		sched.sched_priority = sched_get_priority_max(policy) - 1;
		g_message("%s pthread_setschedparam ret = %d", __FUNCTION__, pthread_setschedparam(thread, policy, &sched));
		g_message("%s pthread_getschedparam ret = %d", __FUNCTION__, pthread_getschedparam(thread, &policy, &sched));
		g_message("%s XXX ugienf set realtime policy = %d", __FUNCTION__, policy);
	}
	while (1)
	{
		if (_zoom_move_is_pendding)
		{
			g_usleep(200000);
			g_static_mutex_lock(&_zoom_move_lock);
			if (_zoom_move_is_pendding)
			{
				ret = nf_live_zoom_move_internal(_prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_x, _prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_y, _prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_w, _prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_h);
				_zoom_move_is_pendding = 0;
			}
			g_static_mutex_unlock(&_zoom_move_lock);
		}
		else
		{
			g_usleep(50000);
		}
	}
}
#endif

// static void _get_nmf_display_channel(int nmf_disp_ch[GOBJ_MAX_MEDIA_PORT],
//                                     int nmf_covert_ch[GOBJ_MAX_MEDIA_PORT],
//                                     gchar ch_arr[32], gboolean covert_arr[32])
static void _get_gobj_media_channel(int gobj_disp_ch[GOBJ_MAX_MEDIA_PORT],
									int gobj_covert_ch[GOBJ_MAX_MEDIA_PORT],
									gchar ch_arr[32], gboolean covert_arr[32], int layer)
{
	int ch, i;

	for (i = 0; i < GOBJ_MAX_MEDIA_PORT; i++)
	{
		gobj_disp_ch[i] = -1;
		gobj_covert_ch[i] = FALSE;
	}

	for (ch = 0; ch < NUM_IPX_CHANNEL; ch++)
	{
		if (ch_arr[ch] != -1)
		{
			g_assert(ch_arr[ch] < NUM_IPX_CHANNEL);
#if defined(_IPX_3296P4) || defined(_IPX_3296P5)
			gobj_disp_ch[ch_arr[ch]] = ch_arr[ch];
#else
			gobj_disp_ch[(int)ch_arr[ch]] = ch;
#endif

			gobj_media_set_video_src(ch_arr[ch], GOBJ_VIDEO_IPCAM, layer);
			// printf("ch:%d nmf_ch:%d ch_arr:%d\n", ch, nmf_disp_ch[ch_arr[ch]], ch_arr[ch] );
			if (covert_arr[ch] == TRUE)
				gobj_covert_ch[(int)ch_arr[ch]] = TRUE;
		}
	}
}

static void _get_gobj_display_channel_spot(int gobj_disp_ch[GOBJ_MAX_MEDIA_PORT],
                                     int gobj_covert_ch[GOBJ_MAX_MEDIA_PORT],
                                     gchar ch_arr[32], gboolean covert_arr[32])
{
     int ch, i;
     for(i=0;i<GOBJ_MAX_MEDIA_PORT; i++)
     {
         gobj_disp_ch[i] = -1;
         gobj_covert_ch[i] = FALSE;
     }

     for(ch=0; ch<NUM_IPX_CHANNEL;ch++)
     {
         if(ch_arr[ch] != -1)
         {
             g_assert(ch_arr[ch] < NUM_IPX_CHANNEL);
             gobj_disp_ch[(int)ch_arr[ch]] = ch;

             if(covert_arr[ch] == TRUE)
                gobj_covert_ch[(int)ch_arr[ch]] = TRUE;
         }
     }
}

// FIXME: use second channel
//  static void _get_nmf_dec_param(GOBJDecodeParam dec_param[GOBJ_MAX_AV_PORT], gboolean all_on, gchar *ch_arr, gint zoom_channel)
//  {
//      int i;
//      memset(dec_param, 0, sizeof(GOBJDecodeParam)*GOBJ_MAX_AV_PORT); //FIXME
//      if(all_on==TRUE)
//      {
//          for(i=_dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET;
//                      i<_dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET + NUM_IPX_CHANNEL; i++)
//          {
//              dec_param[i].enable_decode_state = GOBJ_MEDIA_ENABLE_DECODE_STATE_ON;
//              dec_param[i].dec_port = i-_dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET;
//              if(zoom_channel == i-_dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET)
//              {
//                  dec_param[i].enable_zoom = TRUE;
//              }
//          }
//          return;
//      }
//      if(ch_arr == NULL)
//          return;
//      for(i=_dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET;
//                  i<_dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET + NUM_IPX_CHANNEL; i++)
//      {
//          if(ch_arr[i-_dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET] != -1)
//          {
//              dec_param[i].enable_decode_state = GOBJ_MEDIA_ENABLE_DECODE_STATE_ON;
//              dec_param[i].dec_port = i-_dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET;
//              if(zoom_channel == i-_dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET)
//              {
//                  dec_param[i].enable_zoom = TRUE;
//              }
//          }
//      }
//  }

static void _get_ipcam_current_resolution(int ch, int *cur_cam_w, int *cur_cam_h) {
	int i;
	uint64_t capable = 0, current = 0;

	if (nf_ipcam_get_resol(ch, 0, &capable, &current, NULL) == IPCAM_SETUP_RTN_DONE) {
		nf_ipcam_change_ipcamres_to_size(current, cur_cam_w, cur_cam_h);
	} else {
		*cur_cam_w = 4000;
		*cur_cam_h = 3000;
	}
}

static void _set_gobj_live_off(GOBJDecodeParam dec_param[GOBJ_MAX_AV_PORT], int layer)
{
	int i;

	if (layer == GOBJ_MEDIA_ID_MAIN)
	{
		for (i = _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET; i < _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET + NUM_IPX_CHANNEL; i++)
		{
			dec_param[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET + _dec_port->GOBJ_IPLIVE_1ST_CHANNEL_OFFSET].enable_decode_state = GOBJ_MEDIA_ENABLE_DECODE_STATE_OFF;
			dec_param[i].enable_decode_state = GOBJ_MEDIA_ENABLE_DECODE_STATE_OFF;
			dec_param[i].dec_port = i;
			dec_param[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET + _dec_port->GOBJ_IPLIVE_1ST_CHANNEL_OFFSET].dec_port = i;
		}
	}
	else if (layer == GOBJ_MEDIA_ID_SUB || layer == GOBJ_MEDIA_ID_THIRD)
	{
		for (i = _dec_port->GOBJ_IPLIVE_COMMON_2ND_CHANNEL_OFFSET; i < _dec_port->GOBJ_IPLIVE_COMMON_2ND_CHANNEL_OFFSET + NUM_IPX_CHANNEL; i++)
		{
			dec_param[i - _dec_port->GOBJ_IPLIVE_COMMON_2ND_CHANNEL_OFFSET + _dec_port->GOBJ_IPLIVE_COMMON_1ST_CHANNEL_OFFSET].enable_decode_state = GOBJ_MEDIA_ENABLE_DECODE_STATE_OFF;
			dec_param[i].enable_decode_state = GOBJ_MEDIA_ENABLE_DECODE_STATE_OFF;
			dec_param[i].dec_port = i;
			dec_param[i - _dec_port->GOBJ_IPLIVE_COMMON_2ND_CHANNEL_OFFSET + _dec_port->GOBJ_IPLIVE_COMMON_1ST_CHANNEL_OFFSET].dec_port = i;
		}
	}
	else
	{
		g_warning("Invalid layer %d for _set_gobj_live_off", layer);
		return;
	}
}

static gboolean _equal_small_then_2M(guint64 current)
{
	gboolean ret = FALSE;

	switch (current)
	{
	case NF_IPCAM_RES_320x240:
	case NF_IPCAM_RES_352x240:
	case NF_IPCAM_RES_352x288:
	case NF_IPCAM_RES_640x352:
	case NF_IPCAM_RES_640x480:
	case NF_IPCAM_RES_704x480:
	case NF_IPCAM_RES_704x576:
	case NF_IPCAM_RES_720x480:
	case NF_IPCAM_RES_720x576:
	case NF_IPCAM_RES_1280x720I:
	case NF_IPCAM_RES_1280x720:
	case NF_IPCAM_RES_1024x768:
	case NF_IPCAM_RES_1280x1024:
	case NF_IPCAM_RES_1920x1080I:
	case NF_IPCAM_RES_1920x1080:
	case NF_IPCAM_RES_640x360:
	case NF_IPCAM_RES_320x180:
	case NF_IPCAM_RES_640x360I:
	case NF_IPCAM_RES_640x400:
	case NF_IPCAM_RES_800x450:
	case NF_IPCAM_RES_1440x900:
	case NF_IPCAM_RES_800x600:
	case NF_IPCAM_RES_1600x1200:

	case NF_IPCAM_RES_320x320:
	case NF_IPCAM_RES_640x640:
		ret = TRUE;
		break;
		/*
				case NF_IPCAM_RES_2304x1296	SHIFT_64_23
				case NF_IPCAM_RES_2048x1536	SHIFT_64_24
				case NF_IPCAM_RES_2560x1440	SHIFT_64_25
				case NF_IPCAM_RES_2688x1520	SHIFT_64_26
				case NF_IPCAM_RES_2560x1600	SHIFT_64_27
				case NF_IPCAM_RES_2560x1920	SHIFT_64_28
				case NF_IPCAM_RES_2592x1920	SHIFT_64_29
				case NF_IPCAM_RES_2592x1944	SHIFT_64_30
				case NF_IPCAM_RES_2992x1680	SHIFT_64_31
				case NF_IPCAM_RES_2880x1800	SHIFT_64_32
				case NF_IPCAM_RES_3200x1800	SHIFT_64_33
				case NF_IPCAM_RES_2880x2160	SHIFT_64_34
				case NF_IPCAM_RES_3072x2048	SHIFT_64_35
				case NF_IPCAM_RES_3200x2400	SHIFT_64_36
				case NF_IPCAM_RES_3840x2160	SHIFT_64_37
				case NF_IPCAM_RES_2592x1520	SHIFT_64_38

				case NF_IPCAM_RES_2048x2048:	// 4194304
				case NF_IPCAM_RES_3000x3000:	// 9000000
		*/
	default:
		ret = FALSE;
		break;
	}

	return ret;
}

static gboolean _equal_small_then_4M(guint64 current)
{
	gboolean ret = FALSE;

	switch (current)
	{
	case NF_IPCAM_RES_320x240:
	case NF_IPCAM_RES_352x240:
	case NF_IPCAM_RES_352x288:
	case NF_IPCAM_RES_640x352:
	case NF_IPCAM_RES_640x480:
	case NF_IPCAM_RES_704x480:
	case NF_IPCAM_RES_704x576:
	case NF_IPCAM_RES_720x480:
	case NF_IPCAM_RES_720x576:
	case NF_IPCAM_RES_1280x720I:
	case NF_IPCAM_RES_1280x720:
	case NF_IPCAM_RES_1024x768:
	case NF_IPCAM_RES_1280x1024:
	case NF_IPCAM_RES_1920x1080I:
	case NF_IPCAM_RES_1920x1080:
	case NF_IPCAM_RES_640x360:
	case NF_IPCAM_RES_320x180:
	case NF_IPCAM_RES_640x360I:
	case NF_IPCAM_RES_640x400:
	case NF_IPCAM_RES_800x450:
	case NF_IPCAM_RES_1440x900:
	case NF_IPCAM_RES_800x600:
	case NF_IPCAM_RES_1600x1200:
	case NF_IPCAM_RES_2304x1296:
	case NF_IPCAM_RES_2048x1536:
	case NF_IPCAM_RES_2560x1440: // 3686400
	case NF_IPCAM_RES_2592x1520: // 3939840
	case NF_IPCAM_RES_2688x1520: // 4085760
	case NF_IPCAM_RES_2560x1600: // 4096000

	case NF_IPCAM_RES_320x320:
	case NF_IPCAM_RES_640x640:
	case NF_IPCAM_RES_1280x1280:
		ret = TRUE;
		break;
		/*
				case NF_IPCAM_RES_2560x1920:	// 4915200
				case NF_IPCAM_RES_2592x1920:	// 4976640
				case NF_IPCAM_RES_2992x1680:	// 5026560
				case NF_IPCAM_RES_2592x1944:	// 5038848
				case NF_IPCAM_RES_2880x1800:	// 5184000
				case NF_IPCAM_RES_3200x1800:	// 5760000
				case NF_IPCAM_RES_2880x2160:	// 6220800
				case NF_IPCAM_RES_3072x2048:	// 6291456
				case NF_IPCAM_RES_3200x2400:	// 7680000
				case NF_IPCAM_RES_3840x2160:	// 8294400  half => 4147200

				case NF_IPCAM_RES_2048x2048:	// 4194304
				case NF_IPCAM_RES_3000x3000:	// 9000000
		*/
	default:
		ret = FALSE;
		break;
	}

	return ret;
}

static gboolean _equal_small_then_5M(guint64 current)
{
	gboolean ret = FALSE;

	switch (current)
	{
	case NF_IPCAM_RES_320x240:
	case NF_IPCAM_RES_352x240:
	case NF_IPCAM_RES_352x288:
	case NF_IPCAM_RES_640x352:
	case NF_IPCAM_RES_640x480:
	case NF_IPCAM_RES_704x480:
	case NF_IPCAM_RES_704x576:
	case NF_IPCAM_RES_720x480:
	case NF_IPCAM_RES_720x576:
	case NF_IPCAM_RES_1280x720I:
	case NF_IPCAM_RES_1280x720:
	case NF_IPCAM_RES_1024x768:
	case NF_IPCAM_RES_1280x1024:
	case NF_IPCAM_RES_1920x1080I:
	case NF_IPCAM_RES_1920x1080:
	case NF_IPCAM_RES_640x360:
	case NF_IPCAM_RES_320x180:
	case NF_IPCAM_RES_640x360I:
	case NF_IPCAM_RES_640x400:
	case NF_IPCAM_RES_800x450:
	case NF_IPCAM_RES_1440x900:
	case NF_IPCAM_RES_800x600:
	case NF_IPCAM_RES_1600x1200:
	case NF_IPCAM_RES_2304x1296:
	case NF_IPCAM_RES_2048x1536:
	case NF_IPCAM_RES_2560x1440: // 3686400
	case NF_IPCAM_RES_2592x1520: // 3939840
	case NF_IPCAM_RES_2688x1520: // 4085760
	case NF_IPCAM_RES_2560x1600: // 4096000
	case NF_IPCAM_RES_2560x1920: // 4915200
	case NF_IPCAM_RES_2592x1920: // 4976640
	case NF_IPCAM_RES_2992x1680: // 5026560
	case NF_IPCAM_RES_2592x1944: // 5038848
	case NF_IPCAM_RES_2880x1800: // 5184000

	case NF_IPCAM_RES_320x320:
	case NF_IPCAM_RES_640x640:
	case NF_IPCAM_RES_1280x1280:
		ret = TRUE;
		break;
		/*
				case NF_IPCAM_RES_3000x3000:	// 9000000
		*/
	default:
		ret = FALSE;
		break;
	}

	return ret;
}

static gboolean _equal_small_then_8M(guint64 current)
{
	gboolean ret = FALSE;

	switch (current)
	{
	case NF_IPCAM_RES_320x240:
	case NF_IPCAM_RES_352x240:
	case NF_IPCAM_RES_352x288:
	case NF_IPCAM_RES_640x352:
	case NF_IPCAM_RES_640x480:
	case NF_IPCAM_RES_704x480:
	case NF_IPCAM_RES_704x576:
	case NF_IPCAM_RES_720x480:
	case NF_IPCAM_RES_720x576:
	case NF_IPCAM_RES_1280x720I:
	case NF_IPCAM_RES_1280x720:
	case NF_IPCAM_RES_1024x768:
	case NF_IPCAM_RES_1280x1024:
	case NF_IPCAM_RES_1920x1080I:
	case NF_IPCAM_RES_1920x1080:
	case NF_IPCAM_RES_640x360:
	case NF_IPCAM_RES_320x180:
	case NF_IPCAM_RES_640x360I:
	case NF_IPCAM_RES_640x400:
	case NF_IPCAM_RES_800x450:
	case NF_IPCAM_RES_1440x900:
	case NF_IPCAM_RES_800x600:
	case NF_IPCAM_RES_1600x1200:
	case NF_IPCAM_RES_2304x1296:
	case NF_IPCAM_RES_2048x1536:
	case NF_IPCAM_RES_2560x1440: // 3686400
	case NF_IPCAM_RES_2592x1520: // 3939840
	case NF_IPCAM_RES_2688x1520: // 4085760
	case NF_IPCAM_RES_2560x1600: // 4096000
	case NF_IPCAM_RES_2560x1920: // 4915200
	case NF_IPCAM_RES_2592x1920: // 4976640
	case NF_IPCAM_RES_2992x1680: // 5026560
	case NF_IPCAM_RES_2592x1944: // 5038848
	case NF_IPCAM_RES_2880x1800: // 5184000
	case NF_IPCAM_RES_3200x1800: // 5760000
	case NF_IPCAM_RES_2880x2160: // 6220800
	case NF_IPCAM_RES_3072x2048: // 6291456
	case NF_IPCAM_RES_3200x2400: // 7680000
	case NF_IPCAM_RES_3840x2160: // 8294400  half => 4147200

	case NF_IPCAM_RES_320x320:
	case NF_IPCAM_RES_640x640:
	case NF_IPCAM_RES_1280x1280:
	case NF_IPCAM_RES_2048x2048: // 4194304
		ret = TRUE;
		break;
		/*
				case NF_IPCAM_RES_3000x3000:	// 9000000
		*/
	default:
		ret = FALSE;
		break;
	}

	return ret;
}

// #if defined(_IPX_3296P4) || defined(_IPX_3296P5)
static void _get_dec_port(int *dec_port, int mode)
{
	if (mode == NF_DISPLAY_DOTRIACONTA)
	{
		dec_port[0] = 9;
		dec_port[1] = 10;
		dec_port[2] = 17;
		dec_port[3] = 18;
		dec_port[4] = 4;
		dec_port[5] = 5;
		dec_port[6] = 6;
		dec_port[7] = 7;
		dec_port[8] = 8;
		dec_port[9] = 0;
		dec_port[10] = 1;
		dec_port[11] = 11;
		dec_port[12] = 12;
		dec_port[13] = 13;
		dec_port[14] = 14;
		dec_port[15] = 15;
		dec_port[16] = 16;
		dec_port[17] = 2;
		dec_port[18] = 3;
		dec_port[19] = 19;
		dec_port[20] = 20;
		dec_port[21] = 21;
		dec_port[22] = 22;
		dec_port[23] = 23;
		dec_port[24] = 24;
		dec_port[25] = 25;
		dec_port[26] = 26;
		dec_port[27] = 27;
		dec_port[28] = 28;
		dec_port[29] = 29;
		dec_port[30] = 30;
		dec_port[31] = 31;
	}
	else if (mode == NF_DISPLAY_TETRAICOSA)
	{
		dec_port[0] = 7;
		dec_port[1] = 8;
		dec_port[2] = 13;
		dec_port[3] = 14;
		dec_port[4] = 4;
		dec_port[5] = 5;
		dec_port[6] = 6;
		dec_port[7] = 0;
		dec_port[8] = 1;
		dec_port[9] = 9;
		dec_port[10] = 10;
		dec_port[11] = 11;
		dec_port[12] = 12;
		dec_port[13] = 2;
		dec_port[14] = 3;
		dec_port[15] = 15;
		dec_port[16] = 16;
		dec_port[17] = 17;
		dec_port[18] = 18;
		dec_port[19] = 19;
		dec_port[20] = 20;
		dec_port[21] = 21;
		dec_port[22] = 22;
		dec_port[23] = 23;
	}
	else
	{
		g_message("[Error]  not support mode : %d\n", mode);
	}
}

#define MAX_DOWNSCALE_RANGE 32.0f // "1/4" processing in "dec out" "4" * scaler("8") = 32
#define MAX_DECODE_MBITE (16*1024*1024)

static void _get_gobj_live_dec_param(NF_DISPLAY_E disp_mode, GOBJDecodeParam dec_param[GOBJ_MAX_AV_PORT], gchar *ch_arr, GOBJVideoRect *vr)
{
	int i;
	guint use_1st = 1;
	int w = 0, h = 0;

	memset(dec_param, 0, sizeof(GOBJDecodeParam) * GOBJ_MAX_AV_PORT); // FIXME

	if (ch_arr == NULL)
		return;

	if (disp_mode == NF_DISPLAY_FULL)
	{
		for (i = _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET; i < _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET + NUM_IPX_CHANNEL; i++)
		{
            if (ch_arr[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET] != -1)
            {
				float rw, rh;
				_get_ipcam_current_resolution(i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET, &w, &h);
					
				if( w != 0 && h != 0 ) {
					if( vr != NULL ) {
						rw = (float)w / (float)vr[0].out_width;
						rh = (float)h / (float)vr[0].out_height;
						if( rw > MAX_DOWNSCALE_RANGE || rh > MAX_DOWNSCALE_RANGE ){
							use_1st = 0;
						}
					}
					else {
						use_1st = 0;
					}
				}
				else {
					printf("nf_ipcam_change_ipcamres_to_size api failed\n");
					use_1st = 0;
				}
				
                dec_param[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET + _dec_port->GOBJ_IPLIVE_1ST_CHANNEL_OFFSET].enable_decode_state = use_1st ? GOBJ_MEDIA_ENABLE_DECODE_STATE_ON : GOBJ_MEDIA_ENABLE_DECODE_STATE_OFF;
                /* Corresponding 2nd stream is "on to off" state */
                dec_param[i].enable_decode_state = use_1st ? GOBJ_MEDIA_ENABLE_DECODE_STATE_OFF : GOBJ_MEDIA_ENABLE_DECODE_STATE_ON;
                dec_param[i].dec_port = ch_arr[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET];                                                                                         // i - GOBJ_IPLIVE_2ND_CHANNEL_OFFSET;
                dec_param[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET + _dec_port->GOBJ_IPLIVE_1ST_CHANNEL_OFFSET].dec_port = ch_arr[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET]; // i-GOBJ_IPLIVE_2ND_CHANNEL_OFFSET;
            }
            else
            {
                /* other 1st stream off */
                dec_param[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET + _dec_port->GOBJ_IPLIVE_1ST_CHANNEL_OFFSET].enable_decode_state = GOBJ_MEDIA_ENABLE_DECODE_STATE_OFF;
                /* other 2nd stream on */
                dec_param[i].enable_decode_state = GOBJ_MEDIA_ENABLE_DECODE_STATE_OFF;
                dec_param[i].dec_port = ch_arr[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET];                                                                                         // i-GOBJ_IPLIVE_2ND_CHANNEL_OFFSET;
                dec_param[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET + _dec_port->GOBJ_IPLIVE_1ST_CHANNEL_OFFSET].dec_port = ch_arr[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET]; // i-GOBJ_IPLIVE_2ND_CHANNEL_OFFSET;
            }
		}
	}
	else if (disp_mode == NF_DISPLAY_QUAD)
	{
		int total_decode_mbite = 0;
		float rw, rh;

		for (i = _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET; i < _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET + NUM_IPX_CHANNEL; i++) {
            if (ch_arr[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET] != -1) {
				_get_ipcam_current_resolution(i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET, &w, &h);
					
				if( w != 0 && h != 0 ) {
				}
				else {
					printf("nf_ipcam_change_ipcamres_to_size api failed \n");
					w = 4000;
					h = 3000;
				}
				total_decode_mbite += (w * h);

				if( total_decode_mbite > MAX_DECODE_MBITE ) {
					use_1st = 0;
				}
			}
		}

		for (i = _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET; i < _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET + NUM_IPX_CHANNEL; i++) {
            if (ch_arr[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET] != -1)
            {
                dec_param[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET + _dec_port->GOBJ_IPLIVE_1ST_CHANNEL_OFFSET].enable_decode_state = use_1st ? GOBJ_MEDIA_ENABLE_DECODE_STATE_ON : GOBJ_MEDIA_ENABLE_DECODE_STATE_OFF;
                /* Corresponding 2nd stream is "on to off" state */
                dec_param[i].enable_decode_state = use_1st ? GOBJ_MEDIA_ENABLE_DECODE_STATE_OFF : GOBJ_MEDIA_ENABLE_DECODE_STATE_ON;
                dec_param[i].dec_port = ch_arr[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET];                                                                                         // i - GOBJ_IPLIVE_2ND_CHANNEL_OFFSET;
                dec_param[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET + _dec_port->GOBJ_IPLIVE_1ST_CHANNEL_OFFSET].dec_port = ch_arr[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET]; // i-GOBJ_IPLIVE_2ND_CHANNEL_OFFSET;
                // printf("#### L.%d %d ##\n", __LINE__, dec_param[i].dec_port );
            }
            else
            {
                /* other 1st stream off */
                dec_param[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET + _dec_port->GOBJ_IPLIVE_1ST_CHANNEL_OFFSET].enable_decode_state = GOBJ_MEDIA_ENABLE_DECODE_STATE_OFF;
                /* other 2nd stream on */
                dec_param[i].enable_decode_state = GOBJ_MEDIA_ENABLE_DECODE_STATE_OFF;
                dec_param[i].dec_port = ch_arr[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET];                                                                                         // i-GOBJ_IPLIVE_2ND_CHANNEL_OFFSET;
                dec_param[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET + _dec_port->GOBJ_IPLIVE_1ST_CHANNEL_OFFSET].dec_port = ch_arr[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET]; // i-GOBJ_IPLIVE_2ND_CHANNEL_OFFSET;
                // printf("#### L.%d %d ##\n", __LINE__, dec_param[i].dec_port );
            }
		}
	}
	else if (disp_mode == NF_DISPLAY_HEXA_A || disp_mode == NF_DISPLAY_OCTA_A)
	{
        for (i = _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET; i < _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET + NUM_IPX_CHANNEL; i++)
        {
            if (ch_arr[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET] != -1)
            {
                dec_param[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET + _dec_port->GOBJ_IPLIVE_1ST_CHANNEL_OFFSET].enable_decode_state = GOBJ_MEDIA_ENABLE_DECODE_STATE_OFF;
                /* Corresponding 2nd stream is "on to off" state */
                dec_param[i].enable_decode_state = GOBJ_MEDIA_ENABLE_DECODE_STATE_ON;
                dec_param[i].dec_port = ch_arr[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET];                                                                                         // i - GOBJ_IPLIVE_2ND_CHANNEL_OFFSET;
                dec_param[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET + _dec_port->GOBJ_IPLIVE_1ST_CHANNEL_OFFSET].dec_port = ch_arr[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET]; // i-GOBJ_IPLIVE_2ND_CHANNEL_OFFSET;
                // printf("#### L.%d %d ##\n", __LINE__, dec_param[i].dec_port );
            }
            else
            {
                /* other 1st stream off */
                dec_param[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET + _dec_port->GOBJ_IPLIVE_1ST_CHANNEL_OFFSET].enable_decode_state = GOBJ_MEDIA_ENABLE_DECODE_STATE_OFF;
                /* other 2nd stream on */
                dec_param[i].enable_decode_state = GOBJ_MEDIA_ENABLE_DECODE_STATE_OFF;
                dec_param[i].dec_port = ch_arr[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET];                                                                                         // i-GOBJ_IPLIVE_2ND_CHANNEL_OFFSET;
                dec_param[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET + _dec_port->GOBJ_IPLIVE_1ST_CHANNEL_OFFSET].dec_port = ch_arr[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET]; // i-GOBJ_IPLIVE_2ND_CHANNEL_OFFSET;
                // printf("#### L.%d %d ##\n", __LINE__, dec_param[i].dec_port );
            }
        }
	}
	else
	{
		for (i = _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET; i < _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET + NUM_IPX_CHANNEL; i++)
		{
            if (ch_arr[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET] != -1)
            {
                dec_param[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET + _dec_port->GOBJ_IPLIVE_1ST_CHANNEL_OFFSET].enable_decode_state = GOBJ_MEDIA_ENABLE_DECODE_STATE_OFF;
                /* Corresponding 2nd stream is "on to off" state */
                dec_param[i].enable_decode_state = GOBJ_MEDIA_ENABLE_DECODE_STATE_ON;
                dec_param[i].dec_port = ch_arr[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET];                                                                                         // i - GOBJ_IPLIVE_2ND_CHANNEL_OFFSET;
                dec_param[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET + _dec_port->GOBJ_IPLIVE_1ST_CHANNEL_OFFSET].dec_port = ch_arr[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET]; // i-GOBJ_IPLIVE_2ND_CHANNEL_OFFSET;
                // printf("#### L.%d %d ##\n", __LINE__, dec_param[i].dec_port );
            }
            else
            {
                /* other 1st stream off */
                dec_param[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET + _dec_port->GOBJ_IPLIVE_1ST_CHANNEL_OFFSET].enable_decode_state = GOBJ_MEDIA_ENABLE_DECODE_STATE_OFF;
                /* other 2nd stream on */
                dec_param[i].enable_decode_state = GOBJ_MEDIA_ENABLE_DECODE_STATE_OFF;
                dec_param[i].dec_port = ch_arr[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET];                                                                                         // i-GOBJ_IPLIVE_2ND_CHANNEL_OFFSET;
                dec_param[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET + _dec_port->GOBJ_IPLIVE_1ST_CHANNEL_OFFSET].dec_port = ch_arr[i - _dec_port->GOBJ_IPLIVE_2ND_CHANNEL_OFFSET]; // i-GOBJ_IPLIVE_2ND_CHANNEL_OFFSET;
                // printf("#### L.%d %d ##\n", __LINE__, dec_param[i].dec_port );
            }
		}
	}
}

static void _get_gobj_live_common_dec_param(GOBJMediaUserConfigMode disp_mode, GOBJDecodeParam dec_param[GOBJ_MAX_AV_PORT], int *ch_arr, GOBJVideoRect *vr)
{
    int i;
    guint64 capable = 0, current = 0;

    memset(dec_param, 0, sizeof(GOBJDecodeParam) * GOBJ_MAX_AV_PORT); // FIXME

    if (ch_arr == NULL)
        return;	

	for (i = _dec_port->GOBJ_IPLIVE_COMMON_2ND_CHANNEL_OFFSET; i < _dec_port->GOBJ_IPLIVE_COMMON_2ND_CHANNEL_OFFSET + NUM_IPX_CHANNEL; i++)
	{
		/*all 1st stream -> off */
		dec_param[i - _dec_port->GOBJ_IPLIVE_COMMON_2ND_CHANNEL_OFFSET + _dec_port->GOBJ_IPLIVE_COMMON_1ST_CHANNEL_OFFSET].enable_decode_state = GOBJ_MEDIA_ENABLE_DECODE_STATE_OFF;
		/*all 2nd stream -> on */
		dec_param[i].enable_decode_state = GOBJ_MEDIA_ENABLE_DECODE_STATE_ON;
		dec_param[i].dec_port = i - _dec_port->GOBJ_IPLIVE_COMMON_2ND_CHANNEL_OFFSET + _dec_port->GOBJ_IPLIVE_COMMON_1ST_CHANNEL_OFFSET;
		// printf("ch:%d dec_port:%d\n", i, dec_param[i].dec_port );
		dec_param[i - _dec_port->GOBJ_IPLIVE_COMMON_2ND_CHANNEL_OFFSET + _dec_port->GOBJ_IPLIVE_COMMON_1ST_CHANNEL_OFFSET].dec_port = i-_dec_port->GOBJ_IPLIVE_COMMON_2ND_CHANNEL_OFFSET;
	}
}

int nf_event_motion_cb_hisilocon(GOBJMdDataCbParam *cbdata, char *stream_buf)
{
	return 0;
}

#if INTERNAL_VA
static GAsyncQueue *va_queue = NULL;
static pthread_t va_thread;
#endif
static int file_write = 0;

#if INTERNAL_VA
int nf_va_get_image(void *arg)
{
	if (arg == NULL)
		return 0;

	VaImageCallbackInfo *info = (VaImageCallbackInfo *)arg;

#if 1
	printf("\e[32m-- DLVA --\e[0m [%s:%d] CH(%02d) Time(%d.%03d) Bmp(%dx%d,s:%d, p:%p) Jpeg(%dx%d, s:%d, p:%p)\n",
		   __FUNCTION__, __LINE__,
		   info->ch, info->time, info->timel,
		   info->bmp_width, info->bmp_height, info->bmp_size, info->bmp,
		   info->jpeg_width, info->jpeg_height, info->jpeg_size, info->jpeg);
#endif

	{

		VaImageCallbackInfo *frame = malloc(sizeof(VaImageCallbackInfo));
		memset(frame, 0x00, sizeof(VaImageCallbackInfo));
		memcpy(frame, info, sizeof(VaImageCallbackInfo));

		if (info->bmp != NULL)
		{
			frame->bmp = malloc(frame->bmp_size);
			memcpy(frame->bmp, info->bmp, frame->bmp_size);
		}

		if (info->jpeg != NULL)
		{
			frame->jpeg = malloc(frame->jpeg_size);
			memcpy(frame->jpeg, info->jpeg, frame->jpeg_size);
		}

		g_async_queue_push(va_queue, frame);
	}

	return 0;
}
#endif

static void
_get_cam_type(int *change_mask_cam)
{
	int ch;
#if defined(ENABLE_VIDEO_AUTO_DETECTION)
	NF_VIDEO_TYPE info;
	nf_event_v_get_video_type(&info);

	for (ch = 0; ch < NUM_ACTIVE_CH; ch++)
	{
		if (info.type[ch] == VIDEO_IN_TYPE_SD)
			cur_cam_type[ch] = NF_EVENT_ANALOG_TYPE_TVI_G;
		else if (info.type[ch] == VIDEO_IN_TYPE_3M)
		{
			if (info.type_camera[ch] == NF_EVENT_VIDEO_TYPE_TVI)
				cur_cam_type[ch] = NF_EVENT_ANALOG_TYPE_3M_TVI;
			else if ((info.type_camera[ch] == NF_EVENT_VIDEO_TYPE_HDA) || (info.type_camera[ch] == NF_EVENT_VIDEO_TYPE_HDA_DEFAULT))
				cur_cam_type[ch] = NF_EVENT_ANALOG_TYPE_3M_AHD;
			else
				cur_cam_type[ch] = NF_EVENT_ANALOG_TYPE_3M_TVI;
		}
		else if (info.type[ch] == VIDEO_IN_TYPE_4M)
		{
			if (info.type_camera[ch] == NF_EVENT_VIDEO_TYPE_TVI)
				cur_cam_type[ch] = NF_EVENT_ANALOG_TYPE_4M_TVI;
			else if ((info.type_camera[ch] == NF_EVENT_VIDEO_TYPE_HDA) || (info.type_camera[ch] == NF_EVENT_VIDEO_TYPE_HDA_DEFAULT))
				cur_cam_type[ch] = NF_EVENT_ANALOG_TYPE_4M_AHD;
			else
				cur_cam_type[ch] = NF_EVENT_ANALOG_TYPE_4M_TVI;
		}
		else if (info.type[ch] == VIDEO_IN_TYPE_4M_QHD)
		{
			if (info.type_camera[ch] == NF_EVENT_VIDEO_TYPE_TVI)
				cur_cam_type[ch] = NF_EVENT_ANALOG_TYPE_QHD_TVI;
			else if ((info.type_camera[ch] == NF_EVENT_VIDEO_TYPE_HDA) || (info.type_camera[ch] == NF_EVENT_VIDEO_TYPE_HDA_DEFAULT))
				cur_cam_type[ch] = NF_EVENT_ANALOG_TYPE_QHD_AHD;
			else
				cur_cam_type[ch] = NF_EVENT_ANALOG_TYPE_QHD_TVI;
		}
		else if (info.type[ch] == VIDEO_IN_TYPE_5M)
		{
			if (info.type_camera[ch] == NF_EVENT_VIDEO_TYPE_TVI)
				cur_cam_type[ch] = NF_EVENT_ANALOG_TYPE_5M_TVI;
			else if ((info.type_camera[ch] == NF_EVENT_VIDEO_TYPE_HDA) || (info.type_camera[ch] == NF_EVENT_VIDEO_TYPE_HDA_DEFAULT))
				cur_cam_type[ch] = NF_EVENT_ANALOG_TYPE_5M_AHD;
			else
				cur_cam_type[ch] = NF_EVENT_ANALOG_TYPE_5M_TVI;
		}
		else // 2M
		{
			if (info.type_camera[ch] == NF_EVENT_VIDEO_TYPE_TVI)
				cur_cam_type[ch] = NF_EVENT_ANALOG_TYPE_TVI_G;
			else if ((info.type_camera[ch] == NF_EVENT_VIDEO_TYPE_HDA) || (info.type_camera[ch] == NF_EVENT_VIDEO_TYPE_HDA_DEFAULT))
				cur_cam_type[ch] = NF_EVENT_ANALOG_TYPE_AHD;
			else
				cur_cam_type[ch] = NF_EVENT_ANALOG_TYPE_TVI_G;
		}

#if 0
		g_message("%s line%d ch%d type %d fps %d", __FUNCTION__, __LINE__, ch, info.type[ch], info.fps[ch]);
#endif
	}
#else
	gchar tmp_key[256] = {
		0,
	};

	for (ch = 0; ch < NUM_ACTIVE_CH; ch++)
	{
		sprintf(tmp_key, "cam.C%d.analog_type", ch);
		cur_cam_type[ch] = nf_sysdb_get_uint(tmp_key);
		// printf("ch:%d type:%d\n", ch, cur_cam_type[ch] );
		*change_mask_cam |= (1 << ch);
	}
#endif
}

static int _convertToResType(char *str_res)
{
	int res;

	if (!strcmp(str_res, "1080p-60"))
	{
		res = GOBJ_VIDEO_OUT_TYPE_1920x1080p60;
	}
	else if (!strcmp(str_res, "1080p-50"))
	{
		res = GOBJ_VIDEO_OUT_TYPE_1920x1080p50;
	}
	else if (!strcmp(str_res, "1080p-30"))
	{
		res = GOBJ_VIDEO_OUT_TYPE_1920x1080p30;
	}
	else if (!strcmp(str_res, "1080p-25"))
	{
		res = GOBJ_VIDEO_OUT_TYPE_1920x1080p25;
	}
	else if (!strcmp(str_res, "720p-60"))
	{
		res = GOBJ_VIDEO_OUT_TYPE_1280x720p60;
	}
	else if (!strcmp(str_res, "1280x1024@60"))
	{
		res = GOBJ_VIDEO_OUT_TYPE_1280x1024p60;
	}
	else if (!strcmp(str_res, "1024x768@60"))
	{
		res = GOBJ_VIDEO_OUT_TYPE_1024x768p60;
	}
	else if (!strcmp(str_res, "800x600@60"))
	{
		res = GOBJ_VIDEO_OUT_TYPE_800x600p60;
	}
	else if (!strcmp(str_res, "2560x1440@30"))
	{
		res = GOBJ_VIDEO_OUT_TYPE_2560x1440p30;
	}
	else if (!strcmp(str_res, "2560x1440@60"))
	{
		res = GOBJ_VIDEO_OUT_TYPE_2560x1440p60;
	}
	else if (!strcmp(str_res, "2560x1600@60"))
	{
		res = GOBJ_VIDEO_OUT_TYPE_2560x1600p60;
	}
	else if (!strcmp(str_res, "3840x2160@25"))
	{
		res = GOBJ_VIDEO_OUT_TYPE_3840x2160p25;
	}
	else if (!strcmp(str_res, "3840x2160@30"))
	{
		res = GOBJ_VIDEO_OUT_TYPE_3840x2160p30;
	}
	else if (!strcmp(str_res, "3840x2160@50"))
	{
		res = GOBJ_VIDEO_OUT_TYPE_3840x2160p50;
	}
	else if (!strcmp(str_res, "3840x2160@60"))
	{
		res = GOBJ_VIDEO_OUT_TYPE_3840x2160p60;
	}
	else
	{
		if (DISPLAY_IS_PAL)
		{
			res = GOBJ_VIDEO_OUT_TYPE_1280x1024p60;
		}
		else
		{
			res = GOBJ_VIDEO_OUT_TYPE_1280x1024p60;
		}
	}

	return res;
}

static void
_cam_std_notify_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
#if 0 // ksi_test
	guint ch;
	guint change_mask_cam = 0;
	//NF_VIDEO_TYPE cam_type;
	static int pre_cam_res[NUM_ACTIVE_CH] = {-1,};
	static int _init = 1;
	NF_VIDEO_TYPE video_info;
	
	g_return_if_fail(pinfo != NULL);
	g_static_mutex_lock ((GStaticMutex *)&_cam_lock);
	
	//memcpy( &cam_type, pinfo->p.ptr, sizeof(NF_VIDEO_TYPE) );

	//printf("###### std notify ######\n");
	//for( ch = 0; ch < NUM_ACTIVE_CH; ch++ )
	//	printf("ch:%d type:%d\n", ch, _rec_live_check_cam(cam_type.type[ch]) );

	if( _init )
	{
		for(ch=0; ch< NUM_ACTIVE_CH; ch++){
			refresh_privacy_mask_rgn(ch);
		}
		_init = 0;
	}

	nf_live_get_video_type(&video_info);
	
	for( ch = 0; ch < NUM_ACTIVE_CH; ch++ )
	{
		//cur_cam_res[ch] = _rec_live_check_cam(cam_type.type[ch]);
		if( _rec_live_check_cam(video_info.type[ch], video_info.fps[ch]) != GOBJ_CAMERA_RESOL_unknown0 )
		cur_cam_res[ch] = _rec_live_check_cam(video_info.type[ch], video_info.fps[ch]);
		
		if( cur_cam_res[ch] != pre_cam_res[ch] )
		{
			change_mask_cam |= (1<<ch);
			pre_cam_res[ch] = cur_cam_res[ch];

			//printf("### ch:%d cur : %d \n", ch, cur_cam_res[ch] );
		}
	}

	//printf("## %s change ch 0x%x ##\n", __FUNCTION__, change_mask_cam );

	if( change_mask_cam )
	{
		_send_sys_cam_change(change_mask_cam);
	}

	g_static_mutex_unlock ((GStaticMutex *)&_cam_lock);
#endif
}

// static gboolean _live_cam_change(void)
// {
// 	NF_DISPLAY_E mode;
// 	GOBJDecodeParam dec_param[GOBJ_MAX_AV_PORT];
// 	gchar ch_arr[32];

// 	GOBJMediaObj *h_display = _live_pipe.h_display;

// // #if defined(_IPX_0412P4) || defined(_IPX_0824P4) || defined(_IPX_1648P4) \
// //  || defined(_IPX_0412L4) || defined(_IPX_0824L4) || defined(_IPX_1648L4) \
// //  || defined(_IPX_0412M4) || defined(_IPX_0824M4) || defined(_IPX_1648M4) \
// //  || defined(_IPX_0824P4E)|| defined(_IPX_1648P4E) \
// //  || defined(_IPX_0412M4E) || defined(_IPX_0824M4E) || defined(_IPX_1648M4E) || defined(_IPX_32P4E) || defined(_IPX_32M4E)
// 	if ( (h_display->prev_config_mode >= GOBJ_MEDIA_USER_LIVE_HEXA_A ) &&
// // #if defined(_IPX_3296P4) || defined(_IPX_3296P5)
// 			(h_display->prev_config_mode == GOBJ_MEDIA_USER_LIVE_HEXATRICONTA) ||
// 			(h_display->prev_config_mode == GOBJ_MEDIA_USER_LIVE_TETRAICOSA) ||
// 			(h_display->prev_config_mode == GOBJ_MEDIA_USER_LIVE_DOTRIACONTA )) {
// // #else
// // 			(h_display->prev_config_mode <= NMF_DISPLAY_USER_LIVE_HEXADECA ) ) {
// // #endif
// 		memcpy(ch_arr, _prev_live_config[GOBJ_MEDIA_ID_MAIN].ch_arr, sizeof(_prev_live_config[GOBJ_MEDIA_ID_MAIN].ch_arr));
// 		mode = _prev_live_config[GOBJ_MEDIA_ID_MAIN].mode;

// 		_get_gobj_live_dec_param(mode, dec_param, ch_arr, -1);
// 		gobj_media_decode_change(h_display, dec_param, GOBJ_MEDIA_ID_MAIN);
// 	}
// // #else
// // 	//g_printf("%s:%d: h_display->prev_config_mode  : %d\n", __FUNCTION__, __LINE__, h_display->prev_config_mode );
// // 	if ( h_display->prev_config_mode == NMF_DISPLAY_USER_LIVE_QUAD ) {
// // 		memcpy(ch_arr, _prev_live_config.ch_arr, sizeof(_prev_live_config.ch_arr));
// // 		mode = _prev_live_config.mode;

// // 		_get_nmf_live_dec_param(mode, dec_param, TRUE, ch_arr, -1);
// // 		nmf_display_decode_change(h_display,
// // 		                    dec_param
// // 		                    );
// // 	}
// // #endif

// 	return TRUE;
// }

// static guint g_pre_vloss = 0;

// static void _live_vloss_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
// {
// 	guint new_val;
// 	int i;

// 	new_val	= pinfo->d.params[0];

// 	printf("[%s] new_val[%x]\n", __FUNCTION__, new_val);

// 	guint tmp_old, tmp_new;

// 	if( g_pre_vloss != new_val )
// 	{
// 		for(i=0; i<NUM_ACTIVE_CH; i++)
// 		{
// 			guint ch_mask = (1<<i);

// 			tmp_old = (g_pre_vloss & ch_mask) ? 1:0;
// 			tmp_new = (new_val & ch_mask) ? 1:0;

// 			if( tmp_old != tmp_new )
// 			{
// 				if( tmp_new )
// 				{
// 					_fisheye_refresh_video_param(i);
// 				}
// 			}
// 		}

// 		g_pre_vloss = new_val;
// 	}

// 	_live_cam_change();

// 	return;
// }

gboolean nf_live_init_qc(NF_DISPLAY_E mode, guint win_xpos, guint win_ypos,
						 guint win_w, guint win_h, gchar ch_arr[32], gchar covert_arr[32],
						 gint au_in_vd_ch)
{

	return 1;
}

void _fisheye_init_ptz_param(int ch)
{
#if 0 // ksi_test
// #if defined(_IPX_1648M4) || defined(_IPX_1648P4E) || defined(_IPX_1648M4E) || defined(_IPX_32P4E) || defined(_IPX_32M4E)
	NMF_FISHEYE_PTZ_PARAM ptz_param;
    GOBJMediaObj *h_display = _live_pipe.h_display;
	
	char db_str[128];
	int i, j;

	float ptzr[4];

	memset(&ptz_param, 0x0, sizeof(NMF_FISHEYE_PTZ_PARAM));

	printf("[%s] CH[%d]", __FUNCTION__, ch);
	
	for(i=0; i< NMF_FISHEYE_MAX_VIEW; i++){
		int val;
		char *str;
		char *ptr;

		snprintf(db_str, sizeof(db_str), "cam.fisheye_itx.F%d.view.V%d.tmp_ptzr", ch, i);
		str = nf_sysdb_get_str_nocopy(db_str);
		if( str == NULL )
			break;

		for(j=0; j<4; j++)
		{
			val = strtof(str, &ptr);
			ptzr[j] = val;
			
			if(ptr == NULL)
				break;

			ptr++;
			str = ptr;
		}

		if( ptzr[0] == 0 && ptzr[1] == 0 &&  ptzr[2] == 0 && ptzr[3] == 0 )
			break;

		ptz_param.view[i].pan  = ptzr[0];
		ptz_param.view[i].tilt = ptzr[1];
		ptz_param.view[i].zoom = ptzr[2];
		ptz_param.view[i].roll = ptzr[3];

		printf(" V[%d][%.1f/%.1f/%.1f/%.1f]", i,
			ptz_param.view[i].pan,
			ptz_param.view[i].tilt,
			ptz_param.view[i].zoom,
			ptz_param.view[i].roll);
	}

	printf("\n");

	if( i > 0 )
	{
		ptz_param.ch = ch;
		ptz_param.max_view = i;
		
		nmf_display_fisheye_move_ptz(h_display, &ptz_param);
	}
// #endif
#endif
}

void _fisheye_init_video_param(int i)
{
#if 0
// #if defined(_IPX_1648M4) || defined(_IPX_1648P4E) || defined(_IPX_1648M4E) || defined(_IPX_32P4E) || defined(_IPX_32M4E)
	NMF_FISHEYE_CONFIG fisheye_config;
//	NMF_FISHEYE_PTZ_PARAM nmf_ptz_param;

//	printf("[%s]\n",                );

	char db_str[128];
//	int i;
	GOBJMediaObj *h_display = _live_pipe.h_display;

//	for( i=0 ; i<NUM_ACTIVE_CH ; i++ ){
		memset(&fisheye_config, 0x0, sizeof(NMF_FISHEYE_CONFIG));
		memset(db_str, 0x0, sizeof(db_str));

		fisheye_config.ch = i;
/*
		snprintf(db_str, sizeof(db_str), "cam.fisheye_itx.F%d.act", i);
		fisheye_config.enable = nf_sysdb_get_bool(db_str);
*/

/*
		snprintf(db_str, sizeof(db_str), "cam.fisheye_itx.F%d.tmp_dewarp", i);
		if( nf_sysdb_get_uint(db_str) > 0 )
			fisheye_config.enable = TRUE;
		else
			fisheye_config.enable = FALSE;

		if( fisheye_config.enable == TRUE )
		{
*/		
			guint  mnt;
			
			snprintf(db_str, sizeof(db_str), "cam.fisheye_itx.F%d.mount_mode", i);
			mnt = nf_sysdb_get_uint(db_str);
			if( mnt == 0 )
				fisheye_config.mnt_type = NMF_FISHEYE_MOUNT_CEILING;
			else if( mnt == 1 )
				fisheye_config.mnt_type = NMF_FISHEYE_MOUNT_WALL;
			else
				fisheye_config.mnt_type = NMF_FISHEYE_MOUNT_GROUND;
			
//			snprintf(db_str, sizeof(db_str), "cam.fisheye_itx.F%d.default_view", i);
			snprintf(db_str, sizeof(db_str), "cam.fisheye_itx.F%d.tmp_view", i);
			fisheye_config.view_type = nf_sysdb_get_uint(db_str);

//			if( strlen(video_param->rpl_name) > 0 )
//				snprintf(fisheye_config.rpl_name, sizeof(fisheye_config.rpl_name), "%s", video_param->rpl_name);
#if 1
			printf("[%s] en[%d] mnt[%d] view[%d]\n", __FUNCTION__, fisheye_config.enable, fisheye_config.mnt_type, fisheye_config.view_type);
#endif			
			nmf_display_fisheye_set_config(h_display, &fisheye_config);
//			nmf_display_fisheye_move_ptz(h_display, &nmf_ptz_param);
//		}		
//	}
// #endif
#endif
}

static gboolean _fisheye_init_param(void)
{
	int i;

	for (i = 0; i < NUM_ACTIVE_CH; i++)
		_fisheye_init_ptz_param(i);

	for (i = 0; i < NUM_ACTIVE_CH; i++)
		_fisheye_init_video_param(i);

	return TRUE;
}

void _fisheye_refresh_video_param(int ch)
{
	// #if defined(_IPX_1648M4) || defined(_IPX_1648P4E) || defined(_IPX_1648M4E) || defined(_IPX_32P4E) || defined(_IPX_32M4E)
	GOBJMediaObj *h_display = _live_pipe.h_display;
	// nmf_display_fisheye_refresh_config(h_display, ch);ksi_test
	// #endif
}

static gboolean _live_cam_change(void)
{
	int ret = 0;

	ret = nf_live_change(_prev_live_config[GOBJ_MEDIA_ID_MAIN].mode,
						 _prev_live_config[GOBJ_MEDIA_ID_MAIN].win_xpos,
						 _prev_live_config[GOBJ_MEDIA_ID_MAIN].win_ypos,
						 _prev_live_config[GOBJ_MEDIA_ID_MAIN].win_width,
						 _prev_live_config[GOBJ_MEDIA_ID_MAIN].win_height,
						 _prev_live_config[GOBJ_MEDIA_ID_MAIN].ch_arr,
						 _prev_live_config[GOBJ_MEDIA_ID_MAIN].covert_arr,
						 _prev_live_config[GOBJ_MEDIA_ID_MAIN].au_in_vd_ch);

	return ret;
}

static guint g_pre_vloss = 0;

static void _live_vloss_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	guint new_val;
	int i;

	new_val = pinfo->d.params[0];
	printf("ksi_test _live_vloss_notify_cb()\n");
	printf("[%s] new_val[%x]\n", __FUNCTION__, new_val);

	guint tmp_old, tmp_new;

	if (g_pre_vloss != new_val)
	{
#if 0		
		for (i = 0; i < NUM_ACTIVE_CH; i++)
		{
			guint ch_mask = (1 << i);

			tmp_old = (g_pre_vloss & ch_mask) ? 1 : 0;
			tmp_new = (new_val & ch_mask) ? 1 : 0;

			if (tmp_old != tmp_new)
			{
				if (tmp_new)
				{
					_fisheye_refresh_video_param(i);
				}
			}
		}
#endif
		g_pre_vloss = new_val;
	}

	// _live_cam_change();

	return;
}

typedef struct {
    int channel;
    int width;
    int height;
    int source;
} ResolutionChangeData;

static gboolean
on_resolution_changed_in_main_thread(gpointer user_data)
{
    ResolutionChangeData *data = user_data;

    g_print("MAIN THREAD: Resolution changed! ch=%d %dx%d src=%d\n",
            data->channel, data->width, data->height, data->source);
	
	if( data->source == GOBJ_MEDIA_SOURCE_DEC_MAIN) {
		if( _prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_mode == 1 ) {
			nf_live_zoom_channel_change(_prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_ch);
		} else {
			nf_live_change(_prev_live_config[GOBJ_MEDIA_ID_MAIN].mode,
					_prev_live_config[GOBJ_MEDIA_ID_MAIN].win_xpos,
					_prev_live_config[GOBJ_MEDIA_ID_MAIN].win_ypos,
					_prev_live_config[GOBJ_MEDIA_ID_MAIN].win_width,
					_prev_live_config[GOBJ_MEDIA_ID_MAIN].win_height,
					_prev_live_config[GOBJ_MEDIA_ID_MAIN].ch_arr,
					_prev_live_config[GOBJ_MEDIA_ID_MAIN].covert_arr,
					_prev_live_config[GOBJ_MEDIA_ID_MAIN].au_in_vd_ch);
		}
	}

	if( data->source == GOBJ_MEDIA_SOURCE_DEC_COMMON && nf_sysman_dual_display_mode() == NF_SYSMAN_DUAL_DISP_TYPE_DUAL )
		nf_live_sub_change(_prev_live_config[GOBJ_MEDIA_ID_SUB].mode,
					_prev_live_config[GOBJ_MEDIA_ID_SUB].win_xpos,
					_prev_live_config[GOBJ_MEDIA_ID_SUB].win_ypos,
					_prev_live_config[GOBJ_MEDIA_ID_SUB].win_width,
					_prev_live_config[GOBJ_MEDIA_ID_SUB].win_height,
					_prev_live_config[GOBJ_MEDIA_ID_SUB].ch_arr,
					_prev_live_config[GOBJ_MEDIA_ID_SUB].covert_arr,
					_prev_live_config[GOBJ_MEDIA_ID_SUB].au_in_vd_ch, 
					1);
	
	if( data->source == GOBJ_MEDIA_SOURCE_DEC_COMMON )
		nf_live_change_spot(_prev_live_config[GOBJ_MEDIA_ID_THIRD].mode,
					_prev_live_config[GOBJ_MEDIA_ID_THIRD].ch_arr,
					_prev_live_config[GOBJ_MEDIA_ID_THIRD].covert_arr,
					1);

    return G_SOURCE_REMOVE;
}

static void
on_resolution_changed_raw(int ch, int w, int h, int src, void *user_data)
{
    g_print("HOST RESOLUTION CHANGED !!!!! =%d %dx%d src=%d\n", ch, w, h, src);
    ResolutionChangeData *data = g_new(ResolutionChangeData, 1);
    data->channel = ch;
    data->width   = w;
    data->height  = h;
    data->source  = src;

    g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,
                    on_resolution_changed_in_main_thread,
                    data,
                    (GDestroyNotify)g_free);
}

gboolean nf_live_register_zero_channel_callback(void *h_media, GobjMediaFxnGetEncodeStream cb)
{
	int ret;

	ret = gobj_media_register_get_encode_zero_stream( h_media, cb );
	g_assert(ret==0);

	return ret;
}	

gboolean nf_live_get_zero_channel_thread(gpointer data)
{
	g_message("%s start", __FUNCTION__);
	int mode;
	int covert_arr[32];
	memset(covert_arr, 0, sizeof(covert_arr));
	gchar ch_arr[32];
	int channel_num = 0;
	int cnt = 0;
	int i;

	while(1) {
		memset( ch_arr, -1, sizeof(ch_arr));

		if( cnt == 0 ) {
			channel_num = 4;
			mode = NF_DISPLAY_QUAD;
		} else if( cnt == 1 ) {
			channel_num = 9;
			mode = NF_DISPLAY_NONA;
		} else if( cnt == 2 ) {
			channel_num = 16;
			mode = NF_DISPLAY_HEXADECA;
		} else if( cnt == 3 ) {
			channel_num = 32;
			mode = NF_DISPLAY_DOTRIACONTA;
		} else if( cnt == 4 ) {
			channel_num = 1;
			mode = NF_DISPLAY_FULL;
		}

		for (i = 0; i < channel_num; i++)
			ch_arr[i] = i;

		nf_live_zero_change(mode,
						   0,
						   0,
						   1920,
						   1080,
						   ch_arr,
						   covert_arr
						   );
		printf("### zero change mode:%d num:%d ###\n", mode, channel_num);
		sleep(5);
		cnt++;
		if( cnt > 4)
			cnt = 0;
	}
	return TRUE;
}

gboolean nf_live_init(NF_DISPLAY_E mode, guint win_xpos, guint win_ypos,
					  guint win_w, guint win_h, gchar ch_arr[32], gchar covert_arr[32],
					  gint au_in_vd_ch)
{
	GOBJVideoRect vr[GOBJ_MAX_MEDIA_PORT];
	GOBJMediaUserConfigMode config_mode;
	GOBJ_SIGNAL_TYPE video_type;
	gboolean coverts[GOBJ_MAX_MEDIA_PORT];
	GOBJDecodeParam dec_param[GOBJ_MAX_AV_PORT];
	gint ret;
	guint ch;
	GOBJMediaObj *h_display;
	// GOBJMrtpPipeObj *h_mrtp_pipe;
	// GObject *nfappsink_iplive;
	GobjMrtpPipe *h_mrtp_pipe;

	GOBJ_VIDEO_OUT_TYPE out_vid_std[MAX_DISP_LAYER]; // TODO: num to enum!
	GobjVideoSignalType video_signal_type[NUM_IPX_CHANNEL];
	GOBJDispMuxUserData user_data;
	guint cb_handle = 0;
	guint max_disp_layer;

	gint dual_mode = 0;
	gchar *output_mode;

	dual_mode = nf_sysman_dual_display_mode();
	output_mode = nf_sysman_get_output_mode_live();

	printf("%s %d dual mode:%d output mode:%s, MAX_DISP_LAYER: %d \n", __FUNCTION__, __LINE__, dual_mode, output_mode, MAX_DISP_LAYER);
	/*set output display type*/
	memset(&user_data, -1, sizeof(user_data));

	user_data.disp_id[0] = GOBJ_MEDIA_ID_MAIN;
	user_data.disp_id[1] = GOBJ_MEDIA_ID_SUB;
	user_data.disp_id[2] = GOBJ_MEDIA_ID_THIRD;

	/* STEP3. set output video mode */
	for (ch = 0; ch < MAX_DISP_LAYER; ch++)
	{
		if (DISPLAY_IS_PAL)
			out_vid_std[ch] = GOBJ_VIDEO_OUT_TYPE_720x576i50;
		else
			out_vid_std[ch] = GOBJ_VIDEO_OUT_TYPE_720x480i60;
	}

	if (dual_mode == NF_SYSMAN_DUAL_DISP_TYPE_DUAL || dual_mode == NF_SYSMAN_DUAL_DISP_TYPE_TIE || dual_mode == NF_SYSMAN_DUAL_DISP_TYPE_NONE)
	{
		out_vid_std[0] = _convertToResType(!strcmp(output_mode, "hdmi") ? nf_sysman_get_hdmi_resolution() : nf_sysman_get_vga_resolution());
		out_vid_std[1] = _convertToResType(strcmp(output_mode, "hdmi") ? nf_sysman_get_vga_resolution() : nf_sysman_get_hdmi_resolution());
	}

	printf("[%s:%d] output video standard:%d sub:%d spot(%d)\n", __FUNCTION__, __LINE__, out_vid_std[0], out_vid_std[1], out_vid_std[2]);
	max_disp_layer = MAX_DISP_LAYER;

	for (ch = 0; ch < NUM_IPX_CHANNEL; ch++)
	{
		// if (nf_get_analog_type(ch) == NF_EVENT_ANALOG_TYPE_IPCAM)
		// {
		video_signal_type[ch] = GOBJ_VIDEO_DIGITAL_SIGNAL;
		printf("--- live ch%d digital signal ---\n", ch);
		// }
		// else
		// {
		// 	video_signal_type[ch] = GOBJ_VIDEO_ANALOG_SIGNAL;
		// 	printf("--- live ch%d ananlog signal ---\n", ch);
		// }
	}

	g_message("max_disp_layer(%d)", max_disp_layer);

	gobj_media_set_resolution_changed_callback(on_resolution_changed_raw, NULL);

	h_display = gobj_media_new(out_vid_std,
							   NUM_IPX_CHANNEL,		 /*max channel */
							   nf_hw_get_audio_nr(), /*max audio channel num*/
							   max_disp_layer,		 /*num layer*/
							   nf_audio_pb_cb_local, /*audio fd*/
							   (void *)(&user_data),
							   video_signal_type); /*user_data*/

	g_assert(h_display != NULL);
	_dec_port = gboj_media_get_dec_port();
	printf("\n registering zero encoding callback XXXXXXXXXXXXXXXXXXXXX\n");
	ret = nf_live_register_zero_channel_callback( h_display, (GobjMediaFxnGetEncodeStream)encode_cb );
	g_assert(ret==0);

	// nf_live_apply_video_crop_info();ksi_test

	// nf_pm_init(1);

	// int cam_change;
	// cb_handle= nf_notify_connect_cb( "std_type", (NF_NOTIFY_CB_FUNC)_cam_std_notify_cb_func, NULL);
	// g_message("%s std_type connect_cb[%ld]",__FUNCTION__, cb_handle );
	// g_assert(cb_handle >0);

	// _get_cam_type(&cam_change);

	// KSI_TEST
	//  printf("\n registering encoding callback XXXXXXXXXXXXXXXXXXXXX\n");
	//  ret = gobj_media_register_get_encode_stream( h_display, (GobjMediaFxnGetEncodeStream)encode_cb );
	//  g_assert(ret==0);

	// #if defined(ENABLE_MOTION_CALLBACK_HISILICON)
	// printf("\n registering motion callback XXXXXXXXXXXXXXXXXXXXX\n");
	// ret = gobj_media_register_get_motion_data( h_display, (GobjMediaFxnGetMotionData)nf_event_motion_cb_hisilocon );
	// #else
	// printf("\n registering motion callback XXXXXXXXXXXXXXXXXXXXX\n");
	// ret = gobj_media_register_get_motion_data( h_display, (GobjMediaFxnGetMotionData)nf_event_m_cb_func );
	// #endif
#if 0
	g_pre_vloss = nf_notify_get_param0("vloss");

	cb_handle = nf_notify_connect_cb("vloss", _live_vloss_notify_cb, NULL);
	g_message("%s:%d:  _live_vloss_notify_cb[%ld]", __FUNCTION__, __LINE__, cb_handle);
	g_assert(cb_handle > 0);
#endif
	_live_pipe.h_display = h_display;

	config_mode = _get_gobj_media_mode(mode);

	for (ch = 0; ch < max_disp_layer; ch++)
	{
		if ((dual_mode == NF_SYSMAN_DUAL_DISP_TYPE_TIE || dual_mode == NF_SYSMAN_DUAL_DISP_TYPE_NONE) && (ch == GOBJ_MEDIA_ID_SUB))
			continue;

		if (ch > GOBJ_MEDIA_ID_SUB)
		{
			win_w = (gint)720;
			win_h = (gint)DISPLAY_IS_PAL ? 576 : 480;
		}

		_prev_live_config[ch].mode = mode;
		_prev_live_config[ch].win_xpos = win_xpos;
		_prev_live_config[ch].win_ypos = win_ypos;
		_prev_live_config[ch].win_width = win_w;
		_prev_live_config[ch].win_height = win_h;
		_prev_live_config[ch].intensive_quality_ch = -1;
		_prev_live_config[ch].zoom_ch = 0;
		_prev_live_config[ch].zoom_x = 0;
		_prev_live_config[ch].zoom_y = 0;
		_prev_live_config[ch].zoom_w = DISPLAY_ACTIVE_WIDTH;
		_prev_live_config[ch].zoom_h = DISPLAY_ACTIVE_HEIGHT;

		memcpy(_prev_live_config[ch].ch_arr, ch_arr, sizeof(_prev_live_config[ch].ch_arr));
		memcpy(_prev_live_config[ch].covert_arr, covert_arr, sizeof(_prev_live_config[ch].covert_arr));
		_prev_live_config[ch].au_in_vd_ch = au_in_vd_ch;

		printf("%s:%d: gobj display start layer:%d\n", __FUNCTION__, __LINE__, ch);

		_get_gobj_media_channel(h_display->disp_channels[ch], coverts, ch_arr, covert_arr, ch);
		ret = gobj_media_mode_start(
			h_display,
			config_mode,
			(gint)win_xpos,
			(gint)win_ypos,
			(gint)win_w,
			(gint)win_h,
			h_display->disp_channels[ch],
			coverts,
			au_in_vd_ch,
			ch);
		g_assert(ret >= 0);
	}

	g_assert(ret >= 0);

#ifdef ENABLE_ZOOM_CMD_SKIP
	g_thread_create((GThreadFunc)_zoom_move_thread_func, NULL, FALSE, NULL);
#endif

	//g_thread_create((GThreadFunc)nf_live_get_zero_channel_thread, NULL, FALSE, NULL);

	_get_gobj_live_dec_param(mode, dec_param, ch_arr, NULL);
	gobj_media_decode_change(h_display,
							 dec_param,
							 GOBJ_MEDIA_ID_MAIN);
#if 0
	h_mrtp_pipe = nmf_mrtp_pipe_new(NUM_IPX_CHANNEL);
	_live_pipe.h_mrtp_pipe = h_mrtp_pipe;
	nmf_mrtp_pipe_start(h_mrtp_pipe);

	nfappsink_iplive = nmf_mrtp_pipe_get_nfappsink(h_mrtp_pipe);

	printf("bind pipe to dispmux\n");
	ret = nmf_display_bind_src_pipe(h_display, 
	nfappsink_iplive, 
	NFAPPSINK_IDX_IPLIVE);
#else
#if 1
	h_mrtp_pipe = nmf_mrtp_pipe_new(NUM_IPX_CHANNEL);
	_live_pipe.h_mrtp_pipe = h_mrtp_pipe;
#else
	h_mrtp_pipe = NULL;
	_live_pipe.h_mrtp_pipe = h_mrtp_pipe;
	test_mrtp_init(NUM_IPX_CHANNEL);
#endif	
#endif
	return 1;
}



gboolean nf_live_start(NF_DISPLAY_E mode, guint win_xpos, guint win_ypos,
					   guint win_w, guint win_h, gchar ch_arr[32],
					   gboolean covert_arr[32], gint au_in_vd_ch)
{
	/* 20110809: Bug fix - for remainning playback image bug  */
	g_usleep(60 * 1000);
	return nf_live_change(mode, win_xpos, win_ypos, win_w, win_h, ch_arr, covert_arr, au_in_vd_ch);
}

gboolean nf_live_get_jpeg_snapshot(gint ch,
								   gint *width, gint *height,
								   gint *size,
								   void **out_buffer,
								   gint timeimg,
								   gint dst,
								   guint *timestamp)
{
	int jpg_width = 0, jpg_height = 0, jpg_size = 0;
	int ret = 0;
	char *jpeg_buf = NULL;
	GOBJMediaObj *h_display = _live_pipe.h_display;
	unsigned int jpg_timestamp = 0;
	GOBJJpegEncSrcSize srcSize;

	g_return_val_if_fail(h_display != NULL, 0);

	if (_prev_live_config[GOBJ_MEDIA_ID_MAIN].mode == NF_DISPLAY_FULL)
	{
		srcSize = GOBJ_MAIN_SIZE;
	}
	else
	{
		srcSize = GOBJ_SECOND_SIZE;
	}

	ret = gobj_media_get_jpeg_snapshot(h_display, ch, GOBJJpegSnap_LiveSnap, srcSize, timeimg, dst, NULL, &jpeg_buf, &jpg_width, &jpg_height, &jpg_size, &jpg_timestamp);
	if (ret < 0)
	{
		printf("%s : %d : ==== get jpeg fail!!! ret: %d ====\n", __FUNCTION__, __LINE__, ret);
		return FALSE;
	}

	*out_buffer = jpeg_buf;
	*width = jpg_width;
	*height = jpg_height;
	*size = jpg_size;
	*timestamp = jpg_timestamp;

	return TRUE;
}

gboolean nf_live_stream_jpeg(gint ch,
							 gint *width, gint *height,
							 gint *size,
							 void **out_buffer,
							 gint timeimg,
							 gint dst,
							 guint *timestamp,
							 NF_JPEG_SIZE_E srcSize)
{
	int jpg_width = 0, jpg_height = 0, jpg_size = 0;
	int ret = 0;
	char *jpeg_buf = NULL;
	GOBJMediaObj *h_display = _live_pipe.h_display;
	unsigned int jpg_timestamp = 0;

	g_return_val_if_fail(h_display != NULL, 0);
	printf("%s: %d: ch : %d\n", __FUNCTION__, __LINE__, ch);

	ret = gobj_media_get_jpeg_snapshot(h_display, ch, GOBJJpegSnap_LiveStream, srcSize, timeimg, dst, NULL, &jpeg_buf, &jpg_width, &jpg_height, &jpg_size, &jpg_timestamp);
	if (ret < 0)
	{
		printf("%s : %d : ==== get jpeg fail!!! ret: %d ====\n", __FUNCTION__, __LINE__, ret);
		return FALSE;
	}

	*out_buffer = jpeg_buf;
	*width = jpg_width;
	*height = jpg_height;
	*size = jpg_size;
	*timestamp = jpg_timestamp;

	return TRUE;
}

gboolean nf_live_get_mosaic_jpeg(gint *width, gint *height, gint *size, void **out_buffer)
{
	g_message("%s:%d: Not support !!!\n", __FUNCTION__, __LINE__);
	return FALSE;
}

gboolean nf_live_change(NF_DISPLAY_E mode, guint win_xpos, guint win_ypos,
						guint win_w, guint win_h, gchar ch_arr[32],
						gboolean covert_arr[32], gint au_in_vd_ch)
{
	int i, ret;
	GOBJMediaUserConfigMode config_mode;
	GOBJDecodeParam dec_param[GOBJ_MAX_AV_PORT];
	GOBJVideoRect vr[GOBJ_MAX_MEDIA_PORT];
	gboolean coverts[GOBJ_MAX_MEDIA_PORT];
	GOBJMediaObj *h_display = _live_pipe.h_display;

	g_static_mutex_lock((GStaticMutex *)&_spot_lock);
	config_mode = _get_gobj_media_mode(mode);
	printf("####### %s %d #########\n", __FUNCTION__, __LINE__);

	_get_gobj_media_channel(h_display->disp_channels[GOBJ_MEDIA_ID_MAIN], coverts, ch_arr, covert_arr, GOBJ_MEDIA_ID_MAIN);
	gobj_media_set_geo_org_channel(ch_arr, NUM_CHANNEL, GOBJ_MEDIA_ID_MAIN);

	_set_gobj_live_off(dec_param, GOBJ_MEDIA_ID_MAIN);
	gobj_media_decode_change(h_display, dec_param, GOBJ_MEDIA_ID_MAIN);
	gobj_media_queue_flush(GOBJ_MEDIA_ID_MAIN);

	ret = gobj_media_mode_change(
		h_display,
		config_mode,
		(gint)win_xpos,
		(gint)win_ypos,
		(gint)win_w,
		(gint)win_h,
		h_display->disp_channels[GOBJ_MEDIA_ID_MAIN],
		coverts,
		au_in_vd_ch,
		GOBJ_MEDIA_ID_MAIN); // audio no change

	g_assert(ret >= 0);

	_prev_live_config[GOBJ_MEDIA_ID_MAIN].mode = mode;
	_prev_live_config[GOBJ_MEDIA_ID_MAIN].win_xpos = win_xpos;
	_prev_live_config[GOBJ_MEDIA_ID_MAIN].win_ypos = win_ypos;
	_prev_live_config[GOBJ_MEDIA_ID_MAIN].win_width = win_w;
	_prev_live_config[GOBJ_MEDIA_ID_MAIN].win_height = win_h;
	memcpy(_prev_live_config[GOBJ_MEDIA_ID_MAIN].ch_arr, ch_arr, sizeof(_prev_live_config[GOBJ_MEDIA_ID_MAIN].ch_arr));
	memcpy(_prev_live_config[GOBJ_MEDIA_ID_MAIN].covert_arr, covert_arr, sizeof(_prev_live_config[GOBJ_MEDIA_ID_MAIN].covert_arr));
	_prev_live_config[GOBJ_MEDIA_ID_MAIN].au_in_vd_ch = au_in_vd_ch;

	ret = gobj_media_get_user_vr(h_display,
								 config_mode,
								 (gint)win_xpos,
								 (gint)win_ypos,
								 (gint)win_w,
								 (gint)win_h,
								 h_display->disp_channels[GOBJ_MEDIA_ID_MAIN],
								 coverts,
								 vr,
								 GOBJ_MEDIA_ID_MAIN);
	g_assert(ret >= 0);

	_get_gobj_live_dec_param(mode, dec_param, ch_arr, vr);
	gobj_media_decode_change(h_display, dec_param, GOBJ_MEDIA_ID_MAIN);
	g_static_mutex_unlock((GStaticMutex *)&_spot_lock);

	return TRUE;
}

gboolean nf_live_sub_change(NF_DISPLAY_E mode, guint win_xpos, guint win_ypos,
							guint win_w, guint win_h, gchar ch_arr[32],
							gboolean covert_arr[32], gint au_in_vd_ch, gint renew)
{
	int i, ret;
	GOBJMediaUserConfigMode config_mode;
	GOBJDecodeParam dec_param[GOBJ_MAX_AV_PORT];
	GOBJVideoRect vr[GOBJ_MAX_MEDIA_PORT];
	gboolean coverts[GOBJ_MAX_MEDIA_PORT];
	GOBJMediaObj *h_display = _live_pipe.h_display;

	if (nf_sysman_dual_display_mode() == NF_SYSMAN_DUAL_DISP_TYPE_TIE || nf_sysman_dual_display_mode() == NF_SYSMAN_DUAL_DISP_TYPE_NONE)
	{
		g_message("%s, layer is Disable.", __FUNCTION__);
		return FALSE;
	}

	config_mode = _get_gobj_media_mode(mode);

	printf("####### %s %d #########\n", __FUNCTION__, __LINE__);
	_get_gobj_display_channel_spot(h_display->disp_channels[GOBJ_MEDIA_ID_SUB], coverts, ch_arr, covert_arr);
	gobj_media_set_geo_org_channel(ch_arr, NUM_CHANNEL, GOBJ_MEDIA_ID_SUB);

	if ((memcmp(_prev_live_config[GOBJ_MEDIA_ID_SUB].ch_arr, ch_arr, sizeof(_prev_live_config[GOBJ_MEDIA_ID_SUB].ch_arr)) == 0) &&
		(memcmp(_prev_live_config[GOBJ_MEDIA_ID_SUB].covert_arr, covert_arr, sizeof(_prev_live_config[GOBJ_MEDIA_ID_SUB].covert_arr)) == 0))
	{
		if( renew == 0) {
			printf("***[%s:%d] Skip ID:%d***\n", __FUNCTION__, __LINE__, GOBJ_MEDIA_ID_SUB);
			goto HDSPOT_CMD_SKIP;
		}
	}

	g_static_mutex_lock((GStaticMutex *)&_spot_lock);
	ret = gobj_media_mode_change(
		h_display,
		config_mode,
		(gint)win_xpos,
		(gint)win_ypos,
		(gint)win_w,
		(gint)win_h,
		h_display->disp_channels[GOBJ_MEDIA_ID_SUB],
		coverts,
		au_in_vd_ch,
		GOBJ_MEDIA_ID_SUB); // audio no change
	g_assert(ret >= 0);
	g_static_mutex_unlock((GStaticMutex *)&_spot_lock);

	ret = gobj_media_get_user_vr(h_display,
								 config_mode,
								 (gint)win_xpos,
								 (gint)win_ypos,
								 (gint)win_w,
								 (gint)win_h,
								 h_display->disp_channels[GOBJ_MEDIA_ID_SUB],
								 coverts,
								 vr,
								 GOBJ_MEDIA_ID_SUB);
	g_assert(ret >= 0);

	_get_gobj_live_common_dec_param(mode, dec_param, ch_arr, vr);
	gobj_media_decode_change(h_display, dec_param, GOBJ_MEDIA_ID_SUB);

HDSPOT_CMD_SKIP:
	_prev_live_config[GOBJ_MEDIA_ID_SUB].mode = mode;
	_prev_live_config[GOBJ_MEDIA_ID_SUB].win_xpos = win_xpos;
	_prev_live_config[GOBJ_MEDIA_ID_SUB].win_ypos = win_ypos;
	_prev_live_config[GOBJ_MEDIA_ID_SUB].win_width = win_w;
	_prev_live_config[GOBJ_MEDIA_ID_SUB].win_height = win_h;
	memcpy(_prev_live_config[GOBJ_MEDIA_ID_SUB].ch_arr, ch_arr, sizeof(_prev_live_config[GOBJ_MEDIA_ID_SUB].ch_arr));
	memcpy(_prev_live_config[GOBJ_MEDIA_ID_SUB].covert_arr, covert_arr, sizeof(_prev_live_config[GOBJ_MEDIA_ID_SUB].covert_arr));
	_prev_live_config[GOBJ_MEDIA_ID_SUB].au_in_vd_ch = au_in_vd_ch;

	return TRUE;
}

gboolean nf_live_zero_change(NF_DISPLAY_E mode, guint win_xpos, guint win_ypos,
							guint win_w, guint win_h, gchar ch_arr[32],
							gboolean covert_arr[32])
{
	int i, ret;
	GOBJMediaUserConfigMode config_mode;
	GOBJDecodeParam dec_param[GOBJ_MAX_AV_PORT];
	GOBJVideoRect vr[GOBJ_MAX_MEDIA_PORT];
	gboolean coverts[GOBJ_MAX_MEDIA_PORT];
	GOBJMediaObj *h_display = _live_pipe.h_display;

	config_mode = _get_gobj_media_mode(mode);

	printf("####### %s %d #########\n", __FUNCTION__, __LINE__);
	_get_gobj_display_channel_spot(h_display->disp_channels[GOBJ_MEDIA_ID_ZERO], coverts, ch_arr, covert_arr);
	gobj_media_set_geo_org_channel(ch_arr, NUM_CHANNEL, GOBJ_MEDIA_ID_ZERO);

	if ((memcmp(_prev_live_config[GOBJ_MEDIA_ID_ZERO].ch_arr, ch_arr, sizeof(_prev_live_config[GOBJ_MEDIA_ID_ZERO].ch_arr)) == 0) &&
		(memcmp(_prev_live_config[GOBJ_MEDIA_ID_ZERO].covert_arr, covert_arr, sizeof(_prev_live_config[GOBJ_MEDIA_ID_ZERO].covert_arr)) == 0))
	{
		printf("***[%s:%d] Skip ID:%d***\n", __FUNCTION__, __LINE__, GOBJ_MEDIA_ID_ZERO);
		goto HDSPOT_CMD_SKIP;
	}

	g_static_mutex_lock((GStaticMutex *)&_spot_lock);
	ret = gobj_media_mode_change(
		h_display,
		config_mode,
		(gint)win_xpos,
		(gint)win_ypos,
		(gint)win_w,
		(gint)win_h,
		h_display->disp_channels[GOBJ_MEDIA_ID_ZERO],
		coverts,
		-1,
		GOBJ_MEDIA_ID_ZERO); // audio no change
	g_assert(ret >= 0);
	g_static_mutex_unlock((GStaticMutex *)&_spot_lock);

	ret = gobj_media_get_user_vr(h_display,
								 config_mode,
								 (gint)win_xpos,
								 (gint)win_ypos,
								 (gint)win_w,
								 (gint)win_h,
								 h_display->disp_channels[GOBJ_MEDIA_ID_ZERO],
								 coverts,
								 vr,
								 GOBJ_MEDIA_ID_ZERO);
	g_assert(ret >= 0);

	_get_gobj_live_common_dec_param(mode, dec_param, ch_arr, vr);
	gobj_media_decode_change(h_display, dec_param, GOBJ_MEDIA_ID_ZERO);

HDSPOT_CMD_SKIP:
	_prev_live_config[GOBJ_MEDIA_ID_ZERO].mode = mode;
	_prev_live_config[GOBJ_MEDIA_ID_ZERO].win_xpos = win_xpos;
	_prev_live_config[GOBJ_MEDIA_ID_ZERO].win_ypos = win_ypos;
	_prev_live_config[GOBJ_MEDIA_ID_ZERO].win_width = win_w;
	_prev_live_config[GOBJ_MEDIA_ID_ZERO].win_height = win_h;
	memcpy(_prev_live_config[GOBJ_MEDIA_ID_ZERO].ch_arr, ch_arr, sizeof(_prev_live_config[GOBJ_MEDIA_ID_ZERO].ch_arr));
	memcpy(_prev_live_config[GOBJ_MEDIA_ID_ZERO].covert_arr, covert_arr, sizeof(_prev_live_config[GOBJ_MEDIA_ID_ZERO].covert_arr));
	_prev_live_config[GOBJ_MEDIA_ID_ZERO].au_in_vd_ch = -1;

	return TRUE;
}

gboolean nf_live_change_spot(NF_DISPLAY_E mode, gchar ch_arr[32], gboolean covert_arr[32], int renew)
{
	int ret;
	GOBJMediaUserConfigMode config_mode;
	GOBJDecodeParam dec_param[GOBJ_MAX_AV_PORT];
	gboolean coverts[GOBJ_MAX_MEDIA_PORT];
	GOBJMediaObj *h_display = _live_pipe.h_display;
	int disp_channels[GOBJ_MAX_MEDIA_PORT];
	GOBJVideoRect vr[GOBJ_MAX_MEDIA_PORT];
	gint spot_id;

	g_static_mutex_lock((GStaticMutex *)&_spot_lock);

	spot_id = GOBJ_MEDIA_ID_THIRD;
	config_mode = _get_gobj_media_mode(mode);

	printf("%s %d\n", __FUNCTION__, __LINE__);
	_get_gobj_display_channel_spot(disp_channels, coverts, ch_arr, covert_arr);
	gobj_media_set_geo_org_channel(ch_arr, NUM_CHANNEL, GOBJ_MEDIA_ID_THIRD);

	if ((memcmp(_prev_live_config[spot_id].ch_arr, ch_arr, sizeof(_prev_live_config[spot_id].ch_arr)) == 0) &&
		(memcmp(_prev_live_config[spot_id].covert_arr, covert_arr, sizeof(_prev_live_config[spot_id].covert_arr)) == 0))
	{
		if( renew == 0) {
			printf("***[%s:%d] Skip ID:%d***\n", __FUNCTION__, __LINE__, spot_id);
			goto SPOT_CMD_SKIP;
		}
	}

	ret = gobj_media_mode_change(
		h_display,
		config_mode,
		0,
		0,
		720,
		DISPLAY_IS_PAL ? 576 : 480,
		disp_channels,
		coverts,
		0,
		spot_id);

	g_assert(ret >= 0);

	ret = gobj_media_get_user_vr(h_display,
								 config_mode,
								 (gint)0,
								 (gint)0,
								 (gint)720,
								 (gint)DISPLAY_IS_PAL ? 576 : 480,
								 h_display->disp_channels[spot_id],
								 coverts,
								 vr,
								 spot_id);
	g_assert(ret >= 0);

	_get_gobj_live_common_dec_param(mode, dec_param, ch_arr, vr);
	gobj_media_decode_change(h_display, dec_param, spot_id);
SPOT_CMD_SKIP:
	_prev_live_config[spot_id].mode = mode;
	memcpy(_prev_live_config[spot_id].ch_arr, ch_arr, sizeof(_prev_live_config[spot_id].ch_arr));
	memcpy(_prev_live_config[spot_id].covert_arr, covert_arr, sizeof(_prev_live_config[spot_id].covert_arr));

	g_static_mutex_unlock((GStaticMutex *)&_spot_lock);
	return TRUE;
}

gboolean nf_live_stop(void)
{
	// NMFDisplayObj *h_display = _live_pipe.h_display;
	// NMFDecodeParam dec_param[GOBJ_MAX_AV_PORT];
	// #ifdef NMF_MODEL_IPX
	//     //FIXME for ISC Demo
	//     {
	//         GError* err;
	//         if(_prev_live_config.intensive_quality_ch != -1)
	// #if 0	// jykim
	//              nf_ipcam_set_bitrate(_prev_live_config.intensive_quality_ch, 8000, 1500, NULL, NULL, &err);
	// #endif
	// 			 nf_ipcam_set_normal_ch(_prev_live_config.intensive_quality_ch, NULL, NULL, &err);
	//     }
	//     //_get_nmf_dec_param(dec_param, FALSE, NULL, -1);
	//     _get_nmf_live_dec_param(NF_DISPLAY_FULL, dec_param, FALSE, NULL, -1);
	//     nmf_display_decode_change(h_display,
	//                         dec_param
	//                         );
	// #endif
	GOBJMediaObj *h_display = _live_pipe.h_display;
	GOBJDecodeParam dec_param[GOBJ_MAX_AV_PORT];

	_set_gobj_live_off(dec_param, GOBJ_MEDIA_ID_MAIN);
	gobj_media_decode_change(h_display,
							 dec_param,
							 GOBJ_MEDIA_ID_MAIN);

	return TRUE;
}

gboolean nf_live_freeze(guint ch_mask)
{
	return TRUE;
}

#if 0
gboolean nf_live_snapshot( guint ch_mask , NF_LIVE_SNAPSHOT_OUTPUT **output)
{
    return TRUE;
}
#endif

void nf_live_set_freeze(guint live_freeze)
{
	// _prev_live_config.live_freeze = live_freeze;//ksi_test

	//   	GOBJMediaObj *h_display = _live_pipe.h_display;
	//   	GOBJDecodeParam dec_param[GOBJ_MAX_AV_PORT];
	//   	gchar ch_arr[32];

	// _prev_live_config[GOBJ_MEDIA_ID_MAIN].live_freeze = live_freeze;

	// if( live_freeze )
	// {
	//         _set_gobj_live_off(dec_param, GOBJ_MEDIA_ID_MAIN);
	//            gobj_media_decode_change(h_display,
	//                                 dec_param,
	//                                 GOBJ_MEDIA_ID_MAIN
	//                                 );
	// }
	// else
	// {
	//         memset( ch_arr, -1, sizeof(ch_arr));
	//         ch_arr[ch] = 0;
	//     	_get_gobj_live_dec_param(NF_DISPLAY_FULL, dec_param, ch_arr, NULL);
	//            gobj_media_decode_change(h_display,
	//                                 dec_param,
	//                                 GOBJ_MEDIA_ID_MAIN
	//                                 );
	// }

	//    printf("###### %s freeze:%d ch:%d #####\n", __FUNCTION__, live_freeze, ch );
}

#define MIN_ZOOM_WIDTH 64
#define MIN_ZOOM_HEIGHT 36
gboolean nf_live_zoom_start(gint ch, int base_x, int base_y,
							int zoom_w, int zoom_h, int pip_x, int pip_y, int pip_w, int pip_h)
{
	int i, ret;
	gchar ch_arr[32];
	GOBJDecodeParam dec_param[GOBJ_MAX_AV_PORT];
	GOBJVideoRect vr[GOBJ_MAX_MEDIA_PORT];
	//  int channels[GOBJ_MAX_MEDIA_PORT];
	gboolean coverts[GOBJ_MAX_MEDIA_PORT];
	GOBJMediaObj *h_display = _live_pipe.h_display;

#ifdef ENABLE_ZOOM_CMD_SKIP
	g_static_mutex_lock((GStaticMutex *)&_zoom_move_lock);
	_zoom_move_is_pendding = 0;
#endif
	for (i = 0; i < GOBJ_MAX_MEDIA_PORT; i++)
	{
		h_display->disp_channels[GOBJ_MEDIA_ID_MAIN][i] = -1;
		coverts[i] = FALSE;
	}
	for (i = 0; i < 32; i++)
	{
		ch_arr[i] = -1;
	}

	//    if (nf_get_analog_type(ch) == NF_EVENT_ANALOG_TYPE_IPCAM)
	//    {
	h_display->disp_channels[GOBJ_MEDIA_ID_MAIN][0] = 0;
	gobj_media_set_video_src(0, GOBJ_VIDEO_IPCAM, GOBJ_MEDIA_ID_MAIN);
	//     }
	//    else
	//    {
	//        h_display->disp_channels[GOBJ_MEDIA_ID_MAIN][0] = ch;
	//        gobj_media_set_video_src(0, GOBJ_VIDEO_CAPTURE, GOBJ_MEDIA_ID_MAIN);
	//     }
	ch_arr[ch] = 0;

	printf("=====================zoom start ch=%d\n", ch);
	gobj_media_set_geo_org_channel(ch_arr, NUM_CHANNEL, GOBJ_MEDIA_ID_MAIN);
	_set_gobj_live_off(dec_param, GOBJ_MEDIA_ID_MAIN);
	gobj_media_decode_change(h_display, dec_param, GOBJ_MEDIA_ID_MAIN);

	ret = gobj_media_mode_change(
		h_display,
		GOBJ_MEDIA_USER_LIVE_ZOOM,
		0,
		0,
		DISPLAY_ACTIVE_WIDTH,
		DISPLAY_ACTIVE_HEIGHT,
		h_display->disp_channels[GOBJ_MEDIA_ID_MAIN],
		coverts,
		_prev_live_config[GOBJ_MEDIA_ID_MAIN].au_in_vd_ch,
		GOBJ_MEDIA_ID_MAIN); // audio no change

	g_assert(ret >= 0);
	_prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_mode = 1;
	_prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_ch = ch;
	_prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_x = 0;
	_prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_y = 0;
	_prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_w = DISPLAY_ACTIVE_WIDTH;
	_prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_h = DISPLAY_ACTIVE_HEIGHT;

	_get_gobj_live_dec_param(NF_DISPLAY_FULL, dec_param, ch_arr, NULL);
	gobj_media_decode_change(h_display, dec_param, GOBJ_MEDIA_ID_MAIN);

#ifdef ENABLE_ZOOM_CMD_SKIP
	g_static_mutex_unlock((GStaticMutex *)&_zoom_move_lock);
#endif
	return TRUE;
}

gboolean nf_live_zoom_start_without_pip(gint ch, int base_x, int base_y, int zoom_w, int zoom_h, gboolean disp_init)
{
	int i, ret;
	gchar ch_arr[32];
	GOBJDecodeParam dec_param[GOBJ_MAX_AV_PORT];
	GOBJVideoRect vr[GOBJ_MAX_MEDIA_PORT];
	//  int channels[GOBJ_MAX_MEDIA_PORT];
	gboolean coverts[GOBJ_MAX_MEDIA_PORT];
	GOBJMediaObj *h_display = _live_pipe.h_display;

#ifdef ENABLE_ZOOM_CMD_SKIP
	g_static_mutex_lock((GStaticMutex *)&_zoom_move_lock);
	_zoom_move_is_pendding = 0;
#endif
	for (i = 0; i < GOBJ_MAX_MEDIA_PORT; i++)
	{
		h_display->disp_channels[GOBJ_MEDIA_ID_MAIN][i] = -1;
		coverts[i] = FALSE;
	}

	for (i = 0; i < 32; i++)
	{
		ch_arr[i] = -1;
	}

	h_display->disp_channels[GOBJ_MEDIA_ID_MAIN][0] = ch;
	ch_arr[ch] = 0;

	printf("==============nf_live_zoom_start_without_pip ch=%d, disp_init : %d\n", ch, disp_init);

	if (disp_init == TRUE)
	{
		ret = gobj_media_mode_change(
			h_display,
			GOBJ_MEDIA_USER_LIVE_ZOOM_1,
			0,
			0,
			DISPLAY_ACTIVE_WIDTH,
			DISPLAY_ACTIVE_HEIGHT,
			h_display->disp_channels[GOBJ_MEDIA_ID_MAIN],
			coverts,
			_prev_live_config[GOBJ_MEDIA_ID_MAIN].au_in_vd_ch,
			GOBJ_MEDIA_ID_MAIN); // audio no change

		g_assert(ret >= 0);
		
		_prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_mode = 1;
		_prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_ch = ch;
		_prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_x = 0;
		_prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_y = 0;
		_prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_w = DISPLAY_ACTIVE_WIDTH;
		_prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_h = DISPLAY_ACTIVE_HEIGHT;
	}

	_get_gobj_live_dec_param(NF_DISPLAY_FULL, dec_param, ch_arr, NULL);
	gobj_media_decode_change(h_display,
							 dec_param,
							 GOBJ_MEDIA_ID_MAIN);

#ifdef ENABLE_ZOOM_CMD_SKIP
	g_static_mutex_unlock((GStaticMutex *)&_zoom_move_lock);
#endif

	return TRUE;
}

gboolean nf_live_zoom_channel_change(gint ch)
{
	int i, ret;
	gchar ch_arr[32];
	GOBJDecodeParam dec_param[GOBJ_MAX_AV_PORT];
	GOBJVideoRect vr[GOBJ_MAX_MEDIA_PORT];
	gboolean coverts[GOBJ_MAX_MEDIA_PORT];
	GOBJMediaObj *h_display = _live_pipe.h_display;

	/*skip command*/
	if (_prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_ch == ch)
		return TRUE;

#ifdef ENABLE_ZOOM_CMD_SKIP
	g_static_mutex_lock((GStaticMutex *)&_zoom_move_lock);
	_zoom_move_is_pendding = 0;
#endif

	printf("=====================zoom change ch=%d\n", ch);

	for (i = 0; i < GOBJ_MAX_MEDIA_PORT; i++)
	{
		h_display->disp_channels[GOBJ_MEDIA_ID_MAIN][i] = -1;
		coverts[i] = FALSE;
	}
	for (i = 0; i < 32; i++)
	{
		ch_arr[i] = -1;
	}

	//    if (nf_get_analog_type(ch) == NF_EVENT_ANALOG_TYPE_IPCAM)
	//    {
	h_display->disp_channels[GOBJ_MEDIA_ID_MAIN][0] = 0;
	gobj_media_set_video_src(0, GOBJ_VIDEO_IPCAM, GOBJ_MEDIA_ID_MAIN);
	//    }
	//    else
	//    {
	//        h_display->disp_channels[GOBJ_MEDIA_ID_MAIN][0] = ch;
	//     	gobj_media_set_video_src(0, GOBJ_VIDEO_CAPTURE, GOBJ_MEDIA_ID_MAIN);
	//    }
	ch_arr[ch] = 0;
	gobj_media_set_geo_org_channel(ch_arr, NUM_CHANNEL, GOBJ_MEDIA_ID_MAIN);
	_set_gobj_live_off(dec_param, GOBJ_MEDIA_ID_MAIN);
	gobj_media_decode_change(h_display,
							 dec_param,
							 GOBJ_MEDIA_ID_MAIN);

	ret = gobj_media_mode_change(
		h_display,
		GOBJ_MEDIA_USER_LIVE_ZOOM,
		0,
		0,
		DISPLAY_ACTIVE_WIDTH,
		DISPLAY_ACTIVE_HEIGHT,
		h_display->disp_channels[GOBJ_MEDIA_ID_MAIN],
		coverts,
		_prev_live_config[GOBJ_MEDIA_ID_MAIN].au_in_vd_ch,
		GOBJ_MEDIA_ID_MAIN); // audio no change

	g_assert(ret >= 0);

	// FIXME (IPX0412 ONLY)
	// enable decode all
	_get_gobj_live_dec_param(NF_DISPLAY_FULL, dec_param, ch_arr, NULL);
	gobj_media_decode_change(h_display, dec_param, GOBJ_MEDIA_ID_MAIN);

	usleep(40000);
	ret = gobj_media_mode_change(
		h_display,
		GOBJ_MEDIA_USER_LIVE_ZOOM,
		0,
		0,
		DISPLAY_ACTIVE_WIDTH,
		DISPLAY_ACTIVE_HEIGHT,
		h_display->disp_channels[GOBJ_MEDIA_ID_MAIN],
		coverts,
		_prev_live_config[GOBJ_MEDIA_ID_MAIN].au_in_vd_ch,
		GOBJ_MEDIA_ID_MAIN); // audio no change

	g_assert(ret >= 0);

	_prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_ch = ch;
	_prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_x = 0;
	_prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_y = 0;
	_prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_w = DISPLAY_ACTIVE_WIDTH;
	_prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_h = DISPLAY_ACTIVE_HEIGHT;

#ifdef ENABLE_ZOOM_CMD_SKIP
	g_static_mutex_unlock((GStaticMutex *)&_zoom_move_lock);
#endif
	return TRUE;
}

/* move up
#define MIN_ZOOM_WIDTH  64
#define MIN_ZOOM_HEIGHT 36
*/
#ifdef ENABLE_ZOOM_CMD_SKIP
gboolean nf_live_zoom_move(gint xpos, gint ypos, gint zoom_w, gint zoom_h)
{
	gboolean ret;

	// make odd position to even position for netra scaler : 20110811
	xpos = xpos & (int)(~1);
	ypos = ypos & (int)(~1);
	zoom_w = zoom_w & (int)(~1);
	zoom_h = zoom_h & (int)(~1);
	//

	if (xpos + zoom_w > DISPLAY_ACTIVE_WIDTH || xpos < 0)
		return FALSE;
	if (ypos + zoom_h > DISPLAY_ACTIVE_HEIGHT || ypos < 0)
		return FALSE;
	if (zoom_w < MIN_ZOOM_WIDTH)
		return FALSE;
	if (zoom_h < MIN_ZOOM_HEIGHT)
		return FALSE;

	g_static_mutex_lock(&_zoom_move_lock);

	_zoom_move_is_pendding++;
	_prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_x = xpos;
	_prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_y = ypos;
	_prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_w = zoom_w;
	_prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_h = zoom_h;

	ret = 1;

	g_static_mutex_unlock(&_zoom_move_lock);

	return ret;
}

static gboolean nf_live_zoom_move_internal(gint xpos, gint ypos, gint zoom_w, gint zoom_h)
#else
gboolean nf_live_zoom_move(gint xpos, gint ypos, gint zoom_w, gint zoom_h)
#endif
{
	int ret;
	GOBJMediaObj *h_display = _live_pipe.h_display;
	GOBJVideoRect *vr = gobj_media_get_current_vr(h_display, 0);

	g_assert(vr != NULL);

	printf("%s: xpos=%d, ypos=%d, zoom_w=%d, zoom_h=%d\n", __FUNCTION__, xpos, ypos, zoom_w, zoom_h);

	if (xpos + zoom_w > DISPLAY_ACTIVE_WIDTH || xpos < 0)
		return FALSE;
	if (ypos + zoom_h > DISPLAY_ACTIVE_HEIGHT || ypos < 0)
		return FALSE;
	if (zoom_w < MIN_ZOOM_WIDTH)
		return FALSE;
	if (zoom_h < MIN_ZOOM_HEIGHT)
		return FALSE;

	vr[0].out_crop_left = xpos;
	vr[0].out_crop_right = xpos + zoom_w;
	vr[0].out_crop_top = ypos;
	vr[0].out_crop_bottom = ypos + zoom_h;

	// FIXME: omx crop bug
	if (vr[0].out_crop_left == 0 && vr[0].out_crop_right == DISPLAY_ACTIVE_WIDTH &&
		vr[0].out_crop_top == 0 && vr[0].out_crop_bottom == DISPLAY_ACTIVE_HEIGHT)
	{
		vr[0].out_crop_left = 0;
		vr[0].out_crop_right = DISPLAY_ACTIVE_WIDTH;
		vr[0].out_crop_top = 0;
		vr[0].out_crop_bottom = DISPLAY_ACTIVE_HEIGHT;
	}

	ret = gobj_media_change(h_display,
							vr,
							_prev_live_config[GOBJ_MEDIA_ID_MAIN].au_in_vd_ch,
							FALSE,			   /*Fast Change Off */
							GOBJ_MEDIA_ID_MAIN // Layer
	);
#if 0 // FIXME
	g_assert(ret >= 0);
#else
	if (ret < 0)
		g_warning("%s: display_change fail\n", __FUNCTION__);
#endif

#if 0
	if( _prev_live_config[GOBJ_MEDIA_ID_MAIN].live_freeze == 1 )
	{	
		if (nf_get_analog_type(_prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_ch) == NF_EVENT_ANALOG_TYPE_IPCAM)
		{
			gobj_media_push_dec_frame_buffer(h_display, _prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_ch, 0);
		}
	}
#endif
	_prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_x = xpos;
	_prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_y = ypos;
	_prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_w = zoom_w;
	_prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_h = zoom_h;

	return TRUE;
}
gboolean nf_live_zoom_stop(void)
{
	int ret;
#ifdef ENABLE_ZOOM_CMD_SKIP
	g_static_mutex_lock((GStaticMutex *)&_zoom_move_lock);
	_zoom_move_is_pendding = 0;
#endif
	// change to previous live config
	_prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_mode = 0;
	ret = nf_live_change(_prev_live_config[GOBJ_MEDIA_ID_MAIN].mode,
						 _prev_live_config[GOBJ_MEDIA_ID_MAIN].win_xpos,
						 _prev_live_config[GOBJ_MEDIA_ID_MAIN].win_ypos,
						 _prev_live_config[GOBJ_MEDIA_ID_MAIN].win_width,
						 _prev_live_config[GOBJ_MEDIA_ID_MAIN].win_height,
						 _prev_live_config[GOBJ_MEDIA_ID_MAIN].ch_arr,
						 _prev_live_config[GOBJ_MEDIA_ID_MAIN].covert_arr,
						 _prev_live_config[GOBJ_MEDIA_ID_MAIN].au_in_vd_ch);

#ifdef ENABLE_ZOOM_CMD_SKIP
	g_static_mutex_unlock((GStaticMutex *)&_zoom_move_lock);
#endif
	return ret;
}

int nf_live_zoom_get_pos_sx()
{
	return _prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_x;
}

int nf_live_zoom_get_pos_sy()
{
	return _prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_y;
}

int nf_live_zoom_get_pos_ex()
{
	return _prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_x + _prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_w;
}

int nf_live_zoom_get_pos_ey()
{
	return _prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_y + _prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_h;
}

int nf_live_zoom_get_pos_dx()
{
	return _prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_w;
}

int nf_live_zoom_get_pos_dy()
{
	return _prev_live_config[GOBJ_MEDIA_ID_MAIN].zoom_h;
}

gboolean nf_live_apply_camera_attr()
{
	g_message("%s not implemented!!", __FUNCTION__);
	return TRUE;
}
gboolean nf_live_stop_motion_detector()
{
	g_message("%s not implemented!!", __FUNCTION__);
	return TRUE;
}
gboolean nf_live_apply_live_audio()
{
	g_message("%s not implemented!!", __FUNCTION__);
	return TRUE;
}
gboolean nf_live_apply_motion_detector()
{

	g_message("%s not implemented!!", __FUNCTION__);

	return 1;
}

#if defined(USE_DEV_TPS2384) || defined(USE_DEV_PD69104B1) || defined(USE_DEV_IP804)
guint _poe_port_status = 0;
extern void hub_poe_reboot(int ch);

void nf_live_poe_port_onoff(gint ch, gboolean is_on, gint *is_fail, gint act)
{
	gint ret = 0;

	*is_fail = 0;

	if (is_on)
		_poe_port_status |= (1 << ch);
	else
		_poe_port_status &= ~(1 << ch);

// Local
#if defined(_IPX_1648M4E) || defined(_IPX_1648P4E) || defined(_IPX_32P4E) || defined(_IPX_32M4E) || defined(_IPX_32P5)
	if (ch < NUM_ACTIVE_CH_DVR)
#else
	if (ch < 8)
#endif
		ret = nf_dev_poe_port_onoff(ch, is_on, is_fail);

	// VHub
	if (act != NF_LIVE_PSE_ACT_LOCAL)
	{
		if (is_on)
			vhub_set_port_poe_on(ch);
		else
			vhub_set_port_poe_off(ch);
	}
}

guint nf_live_get_poe_port_status(void)
{
	return _poe_port_status;
}
#endif

void nf_live_set_live_audio_ch(guint ch)
{
	gint audio_nr = nf_hw_get_audio_nr();

	// g_return_val_if_fail((ch < audio_nr) || (ch == NF_DEV_DECODER_DAC_PLAYBACK) , 0);ksi_test

#if 0
		g_message("%s line%d ch %d", __FUNCTION__, __LINE__, ch);
#endif

#if defined(ENABLE_REC_LIVE_AUDIO)
	nf_rec_set_live_audio_ch(ch);
#else
	nf_audio_set_live_audio_ch(ch);
#endif
}

/* set gui alpha value (0~255) */
int nf_live_grpx_set_alpha(unsigned char alpha)
{
	return 0; // ksi_test nmf_display_grpx_set_alpha(alpha);
}

/*check video processor for watchdog */
gboolean nf_live_check_video_processor()
{
	// ksi_test
	//  if(nmf_display_check_processor() < 0)
	//  {
	//      return FALSE;
	//  }
	//  else
	{
		return TRUE;
	}
}

int nf_live_pip_hide(void) //(int ch)
{
	int ret;

	// ret = gobj_media_pip_ch_hide( GOBJ_MEDIA_ID_MAIN, ch, GOBJ_MEDIA_LIVE);ksi_test

	if (ret < 0)
		g_warning("%s result[%d] failed!\n", __FUNCTION__, ret);

	return ret;
}

int nf_live_pip_show(void) //(int ch)
{
	int ret;

	// ret = gobj_media_pip_ch_show( GOBJ_MEDIA_ID_MAIN, ch, GOBJ_MEDIA_LIVE);ksi_test

	if (ret < 0)
		g_warning("%s result[%d] failed!\n", __FUNCTION__, ret);

	return ret;
}

/*
This api do not support zoom and menu mode display.
so must disable aspect ratio when zoom or menu mode entered,
and enable when exit zoom or menu mode
*/
int nf_live_set_disp_ratio(int enable_chmask)
{
	// 	int ret=0;
	// 	NMFDisplayObj *h_display = _live_pipe.h_display;

	// #if defined(_IPX_1648VE3) || defined(_IPX_0824VE3) || defined(_IPX_0412VE3) || defined(_IPX_0824P3) || defined(_IPX_1648P3) \
//  || defined(_IPX_0824L4)|| defined(_IPX_0824P4) || defined(_IPX_1648P4) || defined(_IPX_0824M4) || defined(_IPX_1648M4) || defined(_IPX_0824P4E) || defined(_IPX_1648P4E) \
//  || defined(_IPX_0824M4E) || defined(_IPX_1648M4E) || defined(_IPX_32P4E) || defined(_IPX_32M4E)

	// 	ret = nmf_display_set_aspect_ratio( h_display, enable_chmask);
	// 	if( ret < 0 ) {
	// 		g_warning("%s result[%d] failed!\n", __FUNCTION__, ret );
	// 		return FALSE;
	// 	}

	// #else
	g_warning("%s:%d: Not support!\n", __FUNCTION__, __LINE__);
	return FALSE;
	// #endif

	// 	return TRUE;
}

gboolean nf_api_live_poe_is_ok(int port, NF_UTIL_POE_INFO *info)
{
	gboolean ret = FALSE;
	gint thresh;
	gfloat val;
	guint tot_mW, tot_mW_hub;
	guint limit, limit_hub;
	float thresh_result, thresh_result_hub;
	float one_thresh_result;
	g_return_val_if_fail(info != NULL, 0);

	memset(info, 0x0, sizeof(NF_UTIL_POE_INFO));
#if defined(_IPX_1648M4E) || defined(_IPX_1648P4E) || defined(_IPX_32P4E) || defined(_IPX_32M4E) || defined(_IPX_32P5)
	if (port < NUM_ACTIVE_CH_DVR)
#else
	if (port < 8)
#endif
	{
		ret = nf_dev_poe_get_info_single(port, info);

		if (ret == FALSE)
		{
			g_message("%s - get poe info fail", __FUNCTION__);
			return FALSE;
		}
	}
	else
	{
		NF_UTIL_POE_PORT_INFO tot_info;

#if defined(ENABLE_POE_CHECK)
		nf_event_get_zig_info(&tot_info);
#endif

		memcpy(info, &(tot_info.info[port]), sizeof(NF_UTIL_POE_INFO));
	}

	g_message("%s-CH:%d-[%d][%d][%d][%d][%d][%u][%u]", __FUNCTION__,
			  port,
			  info->is_discovery,
			  info->is_active,
			  info->port_class,
			  info->func_status,
			  info->consumption,
			  info->voltage,
			  info->current_mA);

	if (info->func_status == 3)
	{
		return FALSE;
	}

	thresh = nf_sysdb_get_int("act.sys.sys.poe_fail.threshold");

#if defined(_IPX_1648P4E) || defined(_IPX_1648M4E) || defined(_IPX_32P4E) || defined(_IPX_32M4E) || defined(_IPX_32P5)
	one_thresh_result = (((float)30000) * ((float)thresh / (float)100));
	printf("IPX_1648P4E one_thresh_result(Power limit per Port) is [%d] \n", one_thresh_result);
#else
	one_thresh_result = (((float)15000) * ((float)thresh / (float)100));
#endif

	if ((info->consumption) > one_thresh_result)
	{
		g_message("%s - one_thresh_result : %f", __FUNCTION__, one_thresh_result);
		return FALSE;
	}
	else
	{
/*for POE Error debug, Undecided scenario 32ch - no hub*/
#if defined(_IPX_32P4E) || defined(_IPX_32M4E) || defined(_IPX_32P5)
		if (port < 16)
#else
		if (port < 8)
#endif
		{
			tot_mW = nf_notify_get_param_idx("sys_poe_status", 3);
			limit = nf_sysdb_get_uint("sys.info.poe_limit");

			// limit is 8CH --> 40W & 16CH --> 72W
			thresh_result = (((float)limit * 1000) * ((float)thresh / (float)100));

			if (tot_mW > thresh_result)
			{
				float average;

				if (NUM_ACTIVE_CH == 4)
					average = thresh_result / 4;
				else
					average = thresh_result / 8;

				g_message("%s - tot_mW : %u, thresh_result : %f, average : %f", __FUNCTION__, tot_mW, thresh_result, average);

				if (info->consumption > average)
					return FALSE;
			}
		}
		else
		{
			tot_mW_hub = nf_notify_get_param_idx("sys_poe_status_hub", 3);
			limit_hub = nf_sysdb_get_uint("sys.info.poe_hub_limit");

			thresh_result_hub = (((float)limit_hub * 1000) * ((float)thresh / (float)100));

			if (tot_mW_hub > thresh_result_hub)
			{
				float average;

				average = thresh_result_hub / 8;

				g_message("%s - tot_mW_hub : %u, thresh_result_hub : %f, average : %f", __FUNCTION__, tot_mW_hub, thresh_result_hub, average);

				if (info->consumption > average)
					return FALSE;
			}
		}
	}

	return TRUE;
}

#include "jbshell.h"
/* dispmux stat callback */
static char dispmux_print_stat_help[] = "dispmux_print_stat";
static int
dispmux_print_stat(int argc, char **argv)
{

	if (argc != 1)
	{
		printf("%s\n", dispmux_print_stat);
		return -1;
	}

	//    nmf_display_print_statistics();ksi_test

	return 0;
}
__commandlist(dispmux_print_stat, "dispmux_print_stat", dispmux_print_stat_help, dispmux_print_stat_help);

#if defined(NMF_MODEL_IPXVE)
guint nf_live_get_audio_output_type(void)
{
	gchar tmp_key[256] = {
		0,
	};
	sprintf(tmp_key, "audio.output_type");

	return nf_sysdb_get_uint(tmp_key);
}
#endif

gint nf_live_get_cnt_audio_input(void)
{
	gint num_audio = 0, num_ch = 0;
	gchar *model = nf_sysman_get_system_model();

#if 0
		num_ch=nf_sysman_get_system_active_ch();
#endif
	num_audio = NUM_AUDIO;

#if 0
		g_message("%s line%d num_audio %d NUM_AUDIO %d", __FUNCTION__, __LINE__, num_audio, NUM_AUDIO);
#endif

	return num_audio;
}

gboolean nf_live_fisheye_get_position(int ch, int view_num, NF_FISHEYE_PTZ *ptz)
{
	return TRUE;
}

gboolean nf_live_fisheye_set_position(int ch, int view_num, NF_FISHEYE_PTZ *ptz)
{
	return TRUE;
}

gboolean nf_live_fisheye_get_position_by_invideo_point(int ch, int in_x, int in_y, float *out_pan, float *out_tilt)
{
#if 0 // defined(_IPX_1648M4) || defined(_IPX_1648P4E) || defined(_IPX_1648M4E) //|| defined(_IPX_32P4E) || defined(_IPX_32M4E)
    NMFDisplayObj *h_display = _live_pipe.h_display;
	NMF_FISHEYE_POSITION position;
	int i;

	memset(&position, 0x0, sizeof(NMF_FISHEYE_POSITION));

	position.ch = ch;
	position.in_x = in_x;
	position.in_y = in_y;

//	printf("%s => ch[%d] x[%d] y[%d]\n", __FUNCTION__, position.ch, position.in_x, position.in_y);

	nmf_display_fisheye_get_position(h_display, &position);

//	printf("%s => p[%f] t[%f]\n", __FUNCTION__, position.out_pan, position.out_tilt);

	*out_pan = position.out_pan;
	*out_tilt = position.out_tilt;
	
	return TRUE;
#else
	g_printf("[%s] FISHEYE NOT SUPPORT\n", __FUNCTION__);
	return FALSE;
#endif
}

static char dispmux_get_position_help[] = "dispmux_get_position [ch] [x] [y]";
static int dispmux_get_position(int argc, char **argv)
{
	int ch, x, y;
	int i;
	float p, t;

	if (argc != 4)
	{
		printf("%s\n", dispmux_get_position_help);
		return -1;
	}

	ch = atoi(argv[1]);
	x = atoi(argv[2]);
	y = atoi(argv[3]);

	nf_live_fisheye_get_position_by_invideo_point(ch, x, y, &p, &t);

	printf("%s => x[%d] y[%d] p[%f] t[%f]\n", __FUNCTION__, x, y, p, t);

	return 0;
}
__commandlist(dispmux_get_position, "dispmux_get_position", dispmux_get_position_help, dispmux_get_position_help);

gboolean nf_live_fisheye_get_polygon(int ch, int view_num, NF_FISHEYE_POLYGON *polygon)
{
#if 0 // defined(_IPX_1648M4) || defined(_IPX_1648P4E) || defined(_IPX_1648M4E) //|| defined(_IPX_32P4E) || defined(_IPX_32M4E)
	NMF_FISHEYE_POLYGON in_point;
    NMFDisplayObj *h_display = _live_pipe.h_display;
	int i;

	memset(polygon, 0x0, sizeof(NF_FISHEYE_POLYGON));
	memset(&in_point, 0x0, sizeof(NMF_FISHEYE_POLYGON));
	
	in_point.ch = ch;
	in_point.view = 0;

	nmf_display_fisheye_get_polygon(h_display, &in_point);

	polygon->max_point = in_point.max_point;

	for(i=0; i<polygon->max_point; i++)
	{
		polygon->point_x[i] = in_point.xPoint[i];
		polygon->point_y[i] = in_point.yPoint[i];		
	}
	
	return TRUE;
#else
	g_printf("[%s] FISHEYE NOT SUPPORT\n", __FUNCTION__);
	return FALSE;
#endif
}

gboolean nf_live_fisheye_set_ptz_param(int ch, NF_FISHEYE_PTZ_PARAM *ptz_param)
{
#if 0 // defined(_IPX_1648M4) || defined(_IPX_1648P4E) || defined(_IPX_1648M4E) //|| defined(_IPX_32P4E) || defined(_IPX_32M4E)
	NMF_FISHEYE_PTZ_PARAM nmf_ptz_param;
    NMFDisplayObj *h_display = _live_pipe.h_display;
	int i;

	memset(&nmf_ptz_param, 0x0, sizeof(NMF_FISHEYE_PTZ_PARAM));

	nmf_ptz_param.ch = ch;
	nmf_ptz_param.max_view = ptz_param->max_view;

	for(i=0; i < nmf_ptz_param.max_view; i++)
	{
		nmf_ptz_param.view[i].pan = ptz_param->view[i].pan;
		nmf_ptz_param.view[i].tilt = ptz_param->view[i].tilt;
		nmf_ptz_param.view[i].zoom = ptz_param->view[i].zoom;
//		nmf_ptz_param.view[i].roll = ptz_param->view[i].roll;
	}

#if 0
    printf("[%s] ch[%d] max_view[%d]\n", __FUNCTION__, ch, ptz_param->max_view);
    for(i=0; i<ptz_param->max_view; i++)
            printf("pan[%f] tilt[%f] zoom[%f]\n", ptz_param->view[i].pan, ptz_param->view[i].tilt, ptz_param->view[i].zoom);
    printf("\n");
#endif

	nmf_display_fisheye_move_ptz(h_display, &nmf_ptz_param);

	return TRUE;
#else
	g_printf("[%s] FISHEYE NOT SUPPORT\n", __FUNCTION__);
	return FALSE;
#endif
}

static char dispmux_set_fisheye_ptz_help[] = "dispmux_set_fisheye_ptz [ch] [max_view] [view] [p] [t] [z]";
static int dispmux_set_fisheye_ptz(int argc, char **argv)
{
#if 0 // defined(_IPX_1648M4) || defined(_IPX_1648P4E) || defined(_IPX_1648M4E) //|| defined(_IPX_32P4E) || defined(_IPX_32M4E)
	int ch, max_view, view, p, t, z;
	
	NMF_FISHEYE_PTZ_PARAM ptz_param;	
    NMFDisplayObj *h_display = _live_pipe.h_display;
	
	if(argc!=7) {
		printf("%s\n", dispmux_set_fisheye_ptz_help);
		return -1;
	}

	ch = atoi(argv[1]);
	max_view = atoi(argv[2]);
	view = atoi(argv[3]);
	p = atoi(argv[4]);
	t = atoi(argv[5]);	
	z = atoi(argv[6]);

	memset(&ptz_param, 0x0, sizeof(NMF_FISHEYE_PTZ_PARAM));	

	ptz_param.view[0].pan = 20;
	ptz_param.view[0].tilt = 20;	
	ptz_param.view[0].zoom = 60;	
	
	ptz_param.view[1].pan = 40;
	ptz_param.view[1].tilt = 40;	
	ptz_param.view[1].zoom = 80;

	ptz_param.view[2].pan = 10;
	ptz_param.view[2].tilt = 0;	
	ptz_param.view[2].zoom = 100;

	ptz_param.view[3].pan = 0;
	ptz_param.view[3].tilt = 40;	
	ptz_param.view[3].zoom = 120;
	
	ptz_param.ch = ch;
	ptz_param.max_view = max_view;
	ptz_param.view[view].pan = p;
	ptz_param.view[view].tilt = t;	
	ptz_param.view[view].zoom = z;

	printf("%s => ch[%d] view[%d] p[%f] t[%f] z[%f]\n", __FUNCTION__, ch, view, ptz_param.view[0].pan, ptz_param.view[0].tilt, ptz_param.view[0].zoom);

    nmf_display_fisheye_move_ptz(h_display, &ptz_param);
#endif
	return 0;
}
__commandlist(dispmux_set_fisheye_ptz, "dispmux_set_fisheye_ptz", dispmux_set_fisheye_ptz_help, dispmux_set_fisheye_ptz_help);

gboolean nf_live_fisheye_get_ptz_param(int ch, NF_FISHEYE_PTZ_PARAM *ptz_param)
{
#if 0 // defined(_IPX_1648M4) || defined(_IPX_1648P4E) || defined(_IPX_1648M4E) //|| defined(_IPX_32P4E) || defined(_IPX_32M4E)
	NMF_FISHEYE_PTZ_PARAM nmf_ptz_param;
    NMFDisplayObj *h_display = _live_pipe.h_display;
	int i;

	memset(ptz_param, 0x0, sizeof(NF_FISHEYE_PTZ_PARAM));
	memset(&nmf_ptz_param, 0x0, sizeof(NMF_FISHEYE_PTZ_PARAM));	

	nmf_ptz_param.ch = ch;
    nmf_display_fisheye_get_ptz(h_display, &nmf_ptz_param);

	ptz_param->max_view = nmf_ptz_param.max_view;

	for(i=0; i < ptz_param->max_view; i++)
	{
		ptz_param->view[i].pan = nmf_ptz_param.view[i].pan;
		ptz_param->view[i].tilt = nmf_ptz_param.view[i].tilt;
		ptz_param->view[i].zoom = nmf_ptz_param.view[i].zoom;
//		ptz_param->view[i].roll = nmf_ptz_param.view[i].roll;
	}

#if 0
    printf("[%s] ch[%d] max_view[%d]\n", __FUNCTION__, ch, ptz_param->max_view);
    for(i=0; i<ptz_param->max_view; i++)
            printf("pan[%f] tilt[%f] zoom[%f]\n", ptz_param->view[i].pan, ptz_param->view[i].tilt, ptz_param->view[i].zoom);
    printf("\n");
#endif

	return TRUE;
#else
	g_printf("[%s] FISHEYE NOT SUPPORT\n", __FUNCTION__);
	return FALSE;
#endif
}

static char dispmux_get_fisheye_ptz_help[] = "dispmux_get_fisheye_ptz [ch]";
static int dispmux_get_fisheye_ptz(int argc, char **argv)
{
#if 0 // defined(_IPX_1648M4) || defined(_IPX_1648P4E) || defined(_IPX_1648M4E) //|| defined(_IPX_32P4E) || defined(_IPX_32M4E)
	int ch;
	
	NMF_FISHEYE_PTZ_PARAM ptz_param;	
    NMFDisplayObj *h_display = _live_pipe.h_display;
	
	if(argc!=2) {
		printf("%s\n", dispmux_get_fisheye_ptz_help);
		return -1;
	}

	ch = atoi(argv[1]);

	ptz_param.ch = ch;

    nmf_display_fisheye_get_ptz(h_display, &ptz_param);
#endif
	return 0;
}
__commandlist(dispmux_get_fisheye_ptz, "dispmux_get_fisheye_ptz", dispmux_get_fisheye_ptz_help, dispmux_get_fisheye_ptz_help);

gboolean nf_live_fisheye_get_ptz_limit(int ch, NF_FISHEYE_PTZ_LIMIT *ptz_limit)
{
#if 0 // defined(_IPX_1648M4) || defined(_IPX_1648P4E) || defined(_IPX_1648M4E) //|| defined(_IPX_32P4E) || defined(_IPX_32M4E)
    NMFDisplayObj *h_display = _live_pipe.h_display;	
	NMF_FISHEYE_CONFIG fisheye_config;	

	fisheye_config.ch = ch;
	
    nmf_display_fisheye_get_config(h_display, &fisheye_config);

	if( fisheye_config.mnt_type == NMF_FISHEYE_MOUNT_WALL )
	{
		ptz_limit->pan_min  =  -90;
		ptz_limit->pan_max  =   90;		
		ptz_limit->tilt_min =  -90;
		ptz_limit->tilt_max =   90;
		ptz_limit->zoom_min =   20;
		ptz_limit->zoom_max =  180;
	}
	else if( fisheye_config.mnt_type == NMF_FISHEYE_MOUNT_CEILING )
	{
		ptz_limit->pan_min  = -180;
		ptz_limit->pan_max  =  180;		
		ptz_limit->tilt_min =    0;
		ptz_limit->tilt_max =   50;
		ptz_limit->zoom_min =   20;
		ptz_limit->zoom_max =  180;		
	}
	else	// NF_FISHEYE_MOUNT_GROUND
	{
		ptz_limit->pan_min  = -180;
		ptz_limit->pan_max  =  180;		
		ptz_limit->tilt_min =  -50;
		ptz_limit->tilt_max =    0;
		ptz_limit->zoom_min =   20;
		ptz_limit->zoom_max =  180;
	}

	return TRUE;
#else
	g_printf("[%s] FISHEYE NOT SUPPORT\n", __FUNCTION__);
	return FALSE;
#endif
}

gboolean nf_live_fisheye_get_video_param(int ch, NF_FISHEYE_VIDEO_PARAM *video_param)
{
#if 0 // defined(_IPX_1648M4) || defined(_IPX_1648P4E) || defined(_IPX_1648M4E) //|| defined(_IPX_32P4E) || defined(_IPX_32M4E)
	NMF_FISHEYE_CONFIG fisheye_config;
	NMFDisplayObj *h_display = _live_pipe.h_display;

	memset(video_param, 0x0, sizeof(NF_FISHEYE_VIDEO_PARAM));
	memset(&fisheye_config, 0x0, sizeof(NMF_FISHEYE_CONFIG));

	fisheye_config.ch = ch;
	nmf_display_fisheye_get_config(h_display, &fisheye_config);
#if 1
	printf("[%s] ch[%d] en[%d] mnt[%d] view[%d] rpl[%s]\n", __FUNCTION__,
		fisheye_config.ch, fisheye_config.enable, fisheye_config.mnt_type, fisheye_config.view_type, fisheye_config.rpl_name);
#endif

//	video_param->enable = fisheye_config.enable;

	if( fisheye_config.mnt_type == NMF_FISHEYE_MOUNT_CEILING )
		video_param->mnt_type = NF_FISHEYE_MOUNT_CEILING;
	else if( fisheye_config.mnt_type == NMF_FISHEYE_MOUNT_WALL )
		video_param->mnt_type = NF_FISHEYE_MOUNT_WALL;
	else	// NMF_FISHEYE_MOUNT_GROUND
		video_param->mnt_type = NF_FISHEYE_MOUNT_GROUND;

	video_param->view_type = fisheye_config.view_type;

	if( strlen(fisheye_config.rpl_name) > 0 )
		snprintf(video_param->rpl_name, sizeof(video_param->rpl_name), "%s", fisheye_config.rpl_name);
	
	return TRUE;
#else
	g_printf("[%s] FISHEYE NOT SUPPORT\n", __FUNCTION__);
	return FALSE;
#endif
}

static char dispmux_get_fisheye_config_help[] = "dispmux_get_fisheye_config [ch]";
static int dispmux_get_fisheye_config(int argc, char **argv)
{
#if 0 // defined(_IPX_1648M4) || defined(_IPX_1648P4E) || defined(_IPX_1648M4E) //|| defined(_IPX_32P4E) || defined(_IPX_32M4E)
	int ch;
	NMF_FISHEYE_CONFIG fisheye_config;
       NMFDisplayObj *h_display = _live_pipe.h_display;

	if(argc!=2) {
		printf("%s\n", dispmux_get_fisheye_config_help);
		return -1;
	}

	ch = atoi(argv[1]);

	fisheye_config.ch = ch;

    nmf_display_fisheye_get_config(h_display, &fisheye_config);
#endif
	return 0;
}
__commandlist(dispmux_get_fisheye_config, "dispmux_get_fisheye_config", dispmux_get_fisheye_config_help, dispmux_get_fisheye_config_help);

gboolean nf_live_fisheye_set_video_param(int ch, NF_FISHEYE_VIDEO_PARAM *video_param)
{
#if 0 // defined(_IPX_1648M4) || defined(_IPX_1648P4E) || defined(_IPX_1648M4E)  //|| defined(_IPX_32P4E) || defined(_IPX_32M4E)
	NMF_FISHEYE_CONFIG fisheye_config;
	NMFDisplayObj *h_display = _live_pipe.h_display;

	memset(&fisheye_config, 0x0, sizeof(NMF_FISHEYE_CONFIG));

	fisheye_config.ch = ch;
	fisheye_config.enable = FALSE;

#if 1
	printf("[%s] VIDEO mnt[%d] view[%d] rpl[%s]\n", __FUNCTION__,
		video_param->mnt_type, video_param->view_type, video_param->rpl_name);
#endif

	if( video_param->mnt_type == NF_FISHEYE_MOUNT_CEILING )
		fisheye_config.mnt_type = NMF_FISHEYE_MOUNT_CEILING;
	else if( video_param->mnt_type == NF_FISHEYE_MOUNT_WALL )
		fisheye_config.mnt_type = NMF_FISHEYE_MOUNT_WALL;		
	else	// NF_FISHEYE_MOUNT_GROUND 		
		fisheye_config.mnt_type = NMF_FISHEYE_MOUNT_GROUND;
	
	fisheye_config.view_type = video_param->view_type;

	if( strlen(video_param->rpl_name) > 0 )
		snprintf(fisheye_config.rpl_name, sizeof(fisheye_config.rpl_name), "%s", video_param->rpl_name);
#if 1
	printf("[%s] ch[%d] en[%d] mnt[%d] view[%d] rpl[%s]\n", __FUNCTION__,
		fisheye_config.ch, fisheye_config.enable, fisheye_config.mnt_type, fisheye_config.view_type, fisheye_config.rpl_name);
#endif
	nmf_display_fisheye_set_config(h_display, &fisheye_config);
	
	return TRUE;
#else
	g_printf("[%s] FISHEYE NOT SUPPORT\n", __FUNCTION__);
	return FALSE;
#endif
}

static char dispmux_set_fisheye_config_help[] = "dispmux_set_fisheye_config [ch] [enable] [mnt] [view_type]";
static int dispmux_set_fisheye_config(int argc, char **argv)
{
#if 0 // defined(_IPX_1648M4) || defined(_IPX_1648P4E) || defined(_IPX_1648M4E) //|| defined(_IPX_32P4E) || defined(_IPX_32M4E)
	int ch, enable, mnt, view;
	NMF_FISHEYE_CONFIG fisheye_config;
    NMFDisplayObj *h_display = _live_pipe.h_display;

	if(argc!=5) {
		printf("%s\n", dispmux_set_fisheye_config_help);
		return -1;
	}

	ch = atoi(argv[1]);
	enable = atoi(argv[2]);
	mnt = atoi(argv[3]);
	view = atoi(argv[4]);

	memset(&fisheye_config, 0x0, sizeof(NMF_FISHEYE_CONFIG));

	fisheye_config.ch = ch;
	fisheye_config.enable = enable;
	fisheye_config.mnt_type = mnt;
	fisheye_config.view_type = view;

	printf("%s => ch[%d] enable[%d] mnt[%d] view[%d]\n", __FUNCTION__, ch, enable, mnt, view);	

    nmf_display_fisheye_set_config(h_display, &fisheye_config);
#endif
	return 0;
}
__commandlist(dispmux_set_fisheye_config, "dispmux_set_fisheye_config", dispmux_set_fisheye_config_help, dispmux_set_fisheye_config_help);

void _put_rpl_name_desc(NF_FISHEYE_RPL_LIST *rpl_list, char *name, char *desc)
{
	snprintf(rpl_list->rpl_name[rpl_list->rpl_cnt], MAX_RPL_NAME_LENGTH, name);
	snprintf(rpl_list->rpl_desc[rpl_list->rpl_cnt], MAX_RPL_DESC_LENGTH, desc);
	rpl_list->rpl_cnt++;
}

gboolean nf_live_fisheye_get_rpl_list(NF_FISHEYE_RPL_LIST *rpl_list)
{
	if (rpl_list == NULL)
	{
		g_warning("[%s] input param is null", __FUNCTION__);
		return FALSE;
	}

	memset(rpl_list, 0x0, sizeof(NF_FISHEYE_RPL_LIST));

	_put_rpl_name_desc(rpl_list, "A0IFV", "[ImmerVision] IMV1-1/3");
	_put_rpl_name_desc(rpl_list, "A0NKV", "[Fujifilm] YF360A-2 / YF360A-SA2");
	_put_rpl_name_desc(rpl_list, "A1UST", "[Fujifilm] DF360SR4A-SA2");
	_put_rpl_name_desc(rpl_list, "A8TRT", "[H.Q.O.] PL-M01-V08");
	_put_rpl_name_desc(rpl_list, "B0QQV", "[Kolen] KL04Z");

	_put_rpl_name_desc(rpl_list, "B4QQV", "[CBC Computar] H1328KP");
	_put_rpl_name_desc(rpl_list, "B5SST", "[CBC Computar] L1028KRW / L1028KDRW");
	_put_rpl_name_desc(rpl_list, "B6SST", "[CBC Computar] L1028KRW-180");
	_put_rpl_name_desc(rpl_list, "B72YV", "[CBC C360] 6K");
	_put_rpl_name_desc(rpl_list, "B8QQT", "[CBC Computar] T0928-KRW");

	_put_rpl_name_desc(rpl_list, "B9VVT", "[Xiamen Leading Optics Co., LTD] F117B12924IRM12");
	_put_rpl_name_desc(rpl_list, "C1ZZV", "[Kolen] KL16618");
	_put_rpl_name_desc(rpl_list, "C2TTV", "[Kolen] KL08618");
	_put_rpl_name_desc(rpl_list, "C322V", "[Vantrix] -");
	_put_rpl_name_desc(rpl_list, "C7SST", "[Hanwha Techwin] SNF-8010 / SNF-8010VM");

	_put_rpl_name_desc(rpl_list, "C8WWT", "[Hanwha Techwin] PNF-9010R/RV/RVM");
	_put_rpl_name_desc(rpl_list, "C9VVT", "[CBC Computar] E1222KRY");

	printf("[%s] count[%d]\n", __FUNCTION__, rpl_list->rpl_cnt);

	return TRUE;
}

static char dispmux_get_fisheye_rpl_list_help[] = "dispmux_get_fisheye_rpl_list";
static int dispmux_get_fisheye_rpl_list(int argc, char **argv)
{
	NF_FISHEYE_RPL_LIST rpl_list;
	int i;

	nf_live_fisheye_get_rpl_list(&rpl_list);

	printf("[%s] count[%d]\n", __FUNCTION__, rpl_list.rpl_cnt);

	for (i = 0; i < rpl_list.rpl_cnt; i++)
	{
		printf("[%s] idx[%d] name[%s] desc[%s]\n", __FUNCTION__, i, rpl_list.rpl_name[i], rpl_list.rpl_desc[i]);
	}

	return 0;
}
__commandlist(dispmux_get_fisheye_rpl_list, "dispmux_get_fisheye_rpl_list", dispmux_get_fisheye_rpl_list_help, dispmux_get_fisheye_rpl_list_help);

gboolean nf_live_fisheye_set_lens_type(char *rpl)
{
	return TRUE;
}

gboolean nf_live_fisheye_create_snapshot_channel(int *channel_id, char *in_jpg)
{
	return TRUE;
}

gboolean nf_live_fisheye_get_snapshot_output(int channel_id, char *out_jpg)
{
	return TRUE;
}

gboolean nf_live_fisheye_remove_snapshot_channel(int *channel_id)
{
	return TRUE;
}

gboolean nf_live_fisheye_is_support(int channel_id)
{
#if 0 // defined(_IPX_1648M4) || defined(_IPX_1648P4E) || defined(_IPX_1648M4E) //|| defined(_IPX_32P4E) || defined(_IPX_32M4E)
	guint ch_mask;
	gboolean ret = FALSE;
	
	if( channel_id >= NUM_ACTIVE_CH ){
		g_printf("[%s] channel error[%d]\n", __FUNCTION__, channel_id);
		return FALSE;
	}
/*
	ch_mask = (1 << channel_id);
	if( g_pre_vloss & ch_mask )
		return FALSE;
*/	
	ret = nmf_display_fisheye_is_support(channel_id);

	return ret;
#else
	g_printf("[%s] FISHEYE NOT SUPPORT\n", __FUNCTION__);
	return FALSE;
#endif
}

gboolean nf_live_fisheye_set_enable(int channel_id)
{
#if 0 // defined(_IPX_1648M4) || defined(_IPX_1648P4E) || defined(_IPX_1648M4E) //|| defined(_IPX_32P4E) || defined(_IPX_32M4E)
    NMFDisplayObj *h_display = _live_pipe.h_display;
	
	if( channel_id >= NUM_ACTIVE_CH ){
		g_printf("[%s] channel error[%d]\n", __FUNCTION__, channel_id);
		return FALSE;
	}	

	nmf_display_fisheye_set_enable(h_display, channel_id);

	return TRUE;
#else
	g_printf("[%s] FISHEYE NOT SUPPORT\n", __FUNCTION__);
	return FALSE;
#endif
}

int nf_live_fisheye_get_enable(void)
{
#if 0 // defined(_IPX_1648M4) || defined(_IPX_1648P4E) || defined(_IPX_1648M4E) //|| defined(_IPX_32P4E) || defined(_IPX_32M4E)
    NMFDisplayObj *h_display = _live_pipe.h_display;	
	int ch;
	
	ch = nmf_display_fisheye_get_enable(h_display);

	return ch;
#else
	//	g_printf("[%s] FISHEYE NOT SUPPORT\n", __FUNCTION__);
	return -1;
#endif
}

gboolean nf_live_check_monitor_resolution(gint resolution, gboolean is_vga)
{
	#if defined(USE_DEV_EDID)
	if(nf_edid_check_avariable_resolution(resolution, is_vga))
	return TRUE;
	else
	return FALSE;
	#else
	g_warning("%s Not Support!!!", __FUNCTION__);
	return FALSE;
	#endif
}

static char dispmux_fisheye_enable_help[] = "dispmux_fisheye_enable [ch]";
static int dispmux_fisheye_enable(int argc, char **argv)
{
	gboolean ret;
	int ch;

	if (argc != 2)
	{
		printf("%s\n", dispmux_fisheye_enable_help);
		return -1;
	}

	ch = atoi(argv[1]);

	ret = nf_live_fisheye_set_enable(ch);

	printf("[%s] CH[%d] RET[%d]\n", __FUNCTION__, ch, ret);

	return 0;
}
__commandlist(dispmux_fisheye_enable, "dispmux_fisheye_enable", dispmux_fisheye_enable_help, dispmux_fisheye_enable_help);

static char dispmux_is_fisheye_support_help[] = "dispmux_is_fisheye_support [ch]";
static int dispmux_is_fisheye_support(int argc, char **argv)
{
	gboolean ret;
	int ch;

	if (argc != 2)
	{
		printf("%s\n", dispmux_is_fisheye_support_help);
		return -1;
	}

	ch = atoi(argv[1]);

	ret = nf_live_fisheye_is_support(ch);

	printf("[%s] CH[%d] RET[%d]\n", __FUNCTION__, ch, ret);

	return 0;
}
__commandlist(dispmux_is_fisheye_support, "dispmux_is_fisheye_support", dispmux_is_fisheye_support_help, dispmux_is_fisheye_support_help);

gboolean nf_live_fisheye_block(int on)
{
#if 0 // defined(_IPX_1648M4) || defined(_IPX_1648P4E) || defined(_IPX_1648M4E) //|| defined(_IPX_32P4E) || defined(_IPX_32M4E)
	nmf_display_fisheye_block(on);

	return TRUE;
#else
	g_printf("[%s] FISHEYE NOT SUPPORT\n", __FUNCTION__);
	return 0;
#endif
}

static char dispmux_fisheye_block_help[] = "dispmux_fisheye_block [on]";
static int dispmux_fisheye_block(int argc, char **argv)
{
#if 0 // defined(_IPX_1648M4) || defined(_IPX_1648P4E) || defined(_IPX_1648M4E) //|| defined(_IPX_32P4E) || defined(_IPX_32M4E)
	int on;

	if(argc!=2) {
		printf("%s\n", dispmux_fisheye_block_help);
		return -1;
	}

	on = atoi(argv[1]);

	nmf_display_fisheye_block(on);
#endif
	return 0;
}
__commandlist(dispmux_fisheye_block, "dispmux_fisheye_block", dispmux_fisheye_block_help, dispmux_fisheye_block_help);
gint nf_live_get_resol(gint ch, gint stream, guint64* capable, guint64* current, GError **error)
{
	gint ret=0;

	// if (nf_get_analog_type(ch) == NF_EVENT_ANALOG_TYPE_IPCAM)
	// {
		ret=nf_ipcam_get_resol(ch, stream, capable, current, error);
	// }
	// else
	// {
		// ret=nf_api_dvr_get_resol(ch, stream, capable, current, error, 0);
	// }

	//#if defined(MODEL_NVR)
	//	ret=nf_ipcam_get_resol(ch, stream, capable, current, error);
	//#else
	//	ret=nf_api_dvr_get_resol(ch, stream, capable, current, error);
	//#endif
	
	/* SWANFFIVEG-1520 */
	if(*current == 0){
		    *current = NF_IPCAM_RES_1920x1080;
	}
	/* SWANFFIVEG-1520 */
	if(*capable == 0){
		    *capable = NF_IPCAM_RES_1920x1080;
	}

	return ret;
}

gint nf_live_get_resol_by_codec(gint ch, gint stream, guint64* capable, guint64* current, GError **error, gint set_codec)
{
	gint ret=0;

	// if (nf_get_analog_type(ch) == NF_EVENT_ANALOG_TYPE_IPCAM)
	// {
		ret=nf_ipcam_get_resol(ch, stream, capable, current, error);
	// }
	// else
	// {
	// 	ret=nf_api_dvr_get_resol(ch, stream, capable, current, error, set_codec);
	// }

	//#if defined(MODEL_NVR)
	//	ret=nf_ipcam_get_resol(ch, stream, capable, current, error);
	//#else
	//	ret=nf_api_dvr_get_resol(ch, stream, capable, current, error);
	//#endif
	
	/* SWANFFIVEG-1520 */
	if(*current == 0){
		    *current = NF_IPCAM_RES_1920x1080;
	}
	/* SWANFFIVEG-1520 */
	if(*capable == 0){
		    *capable = NF_IPCAM_RES_1920x1080;
	}

	return ret;
}

gint nf_live_get_fps(gint ch, gint stream, guint64* capable, guint64* current, GError **error)
{
	gint ret=0;
	guint tmp_capable = 0;
	guint tmp_current = 0;
	
	// if (nf_get_analog_type(ch) == NF_EVENT_ANALOG_TYPE_IPCAM)
	// {
		//		ret=nf_ipcam_get_fps(ch, stream, capable, current, error);
	ret=nf_ipcam_get_fps(ch, stream, &tmp_capable, &tmp_current, error);

	*capable = (guint64)tmp_capable;
	*current = (guint64)tmp_current;
	// }
	// else
	// {
	// 	ret=nf_api_dvr_get_fps(ch, stream, capable, current, error, 0);
	// }

	//#if defined(MODEL_NVR)
	//	ret=nf_ipcam_get_fps(ch, stream, capable, current, error);
	//#else
	//	ret=nf_api_dvr_get_fps(ch, stream, capable, current, error);
	//#endif

	return ret;
}

gint nf_live_get_fps_by_codec(gint ch, gint stream, guint64* capable, guint64* current, GError **error, gint set_codec)
{
	gint ret=0;
	guint tmp_capable = 0;
	guint tmp_current = 0;
	
	// if (nf_get_analog_type(ch) == NF_EVENT_ANALOG_TYPE_IPCAM)
	// {
		//		ret=nf_ipcam_get_fps(ch, stream, capable, current, error);
	ret=nf_ipcam_get_fps(ch, stream, &tmp_capable, &tmp_current, error);

	*capable = (guint64)tmp_capable;
	*current = (guint64)tmp_current;
	// }
	// else
	// {
	// 	ret=nf_api_dvr_get_fps(ch, stream, capable, current, error, set_codec);
	// }

	//#if defined(MODEL_NVR)
	//	ret=nf_ipcam_get_fps(ch, stream, capable, current, error);
	//#else
	//	ret=nf_api_dvr_get_fps(ch, stream, capable, current, error);
	//#endif

	return ret;
}