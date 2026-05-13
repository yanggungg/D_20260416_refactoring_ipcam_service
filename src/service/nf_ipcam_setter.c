/**
 * @file nf_ipcam_setter.c
 * @brief IP카메라 설정 API.
 * @author Jae-young Kim
 * @date 03/03/2011
 * @copyright (c) COPYRIGHT 2010 ITXSecurity\n
 * ALL RIGHT RESERVED
 */
#ifndef __NF_IPCAM_SETTER_C__
#define __NF_IPCAM_SETTER_C__

#include "nf_common.h"
#include "nf_api_ipcam.h"
#include "nf_ipcam_defs.h"
#include "nf_ipcam_driver_axis.h"

#include <gobjmrtppipe.h>
#include "nf_api_dlva.h"

#include <nf_ptz.h>
#include <nfdal.h>
#include <nf_api_live.h>

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "ipcam_set"

/** @def NUM_IPX_CHANNEL
 *  @brief NVR의 최대 채널수(모델 무관).
 */
#define NUM_IPX_CHANNEL		AVAILABLE_MAX_CH
/** @def LIMIT_DM368_512K
 *  @brief TI카메라의 최소 bitrate 512K로 제한여부.
 */
#define LIMIT_DM368_512K	(0)


int smart_motion_parse_db_string(NFIPCamMotionSmartOption *smart_motion_options, const char *db_string);
static GTimeVal _ipcam_poe_off_time[NUM_IPX_CHANNEL];


const char *__SETUP_RTN_STR_[] =
{
	"SETUP_FAILED",
	"SETUP_DONE",
	"SETUP_PENDING"
};

static float _BITRATE_COVERAGE_RATE_[][5] = 
{
/* base : 0*/	
	{ 0.75, 0.63, 0.49, 0.47, 0.45 },
/* 5M resolution, 4000 bitrate : 1 */
	{ 1.0, 0.73, 0.68, 0.62, 0.56 }
};


/* IPCamera setup API */

/**
 * @brief 카메라 fps enum값으로 숫자 fps를 구한다.
 * @param fps Fps enum값.
 * @return 숫자 fps값.
 */
int _get_fps_num(NF_IPCAM_FPS_E fps)
{
	int rtn = 0;
	switch(fps)
	{
		case NF_IPCAM_FPS_300: { rtn = 30; break; }
		case NF_IPCAM_FPS_250: { rtn = 25; break; }
		case NF_IPCAM_FPS_150: { rtn = 15; break; }
		case NF_IPCAM_FPS_120: { rtn = 12; break; }
		case NF_IPCAM_FPS_70:  { rtn = 7;  break; }
		case NF_IPCAM_FPS_60:  { rtn = 6;  break; }
		case NF_IPCAM_FPS_30:  { rtn = 3;  break; }
		case NF_IPCAM_FPS_20:  { rtn = 2;  break; }
		case NF_IPCAM_FPS_10:  { rtn = 1;  break; }
	}

	return rtn;
}

/**
 * @brief Sysdb의 fps값과 카메라의 fps 지원여부를 비교하여 적정 fps를 반환한다.
 * @param db_fps Sysdb의 fps값.
 * @param support 현재 카메라의 fps지원값.
 * @return 적정 fps.
 */
NF_IPCAM_FPS_E _get_max_fps_mask(int db_fps, unsigned int support)
{
	int i;
	NF_IPCAM_FPS_E rtn = NF_IPCAM_FPS_300;

	if (db_fps == 30)
	{
		if (support & NF_IPCAM_FPS_300) { rtn = NF_IPCAM_FPS_300; }
		else if (support & NF_IPCAM_FPS_250) { rtn = NF_IPCAM_FPS_300; }
		else if (support & NF_IPCAM_FPS_150) { rtn = NF_IPCAM_FPS_150; }
		else if (support & NF_IPCAM_FPS_120) { rtn = NF_IPCAM_FPS_150; }
		else if (support & NF_IPCAM_FPS_70)  { rtn = NF_IPCAM_FPS_70;  }
		else if (support & NF_IPCAM_FPS_60)  { rtn = NF_IPCAM_FPS_70;  }
		else if (support & NF_IPCAM_FPS_30)  { rtn = NF_IPCAM_FPS_30;  }
		else if (support & NF_IPCAM_FPS_20)  { rtn = NF_IPCAM_FPS_20;  }
		else if (support & NF_IPCAM_FPS_10)  { rtn = NF_IPCAM_FPS_300;  }
	}
	else if (db_fps >= 25)
	{
		if (support & NF_IPCAM_FPS_250) { rtn = NF_IPCAM_FPS_300; }
		else if (support & NF_IPCAM_FPS_150) { rtn = NF_IPCAM_FPS_150; }
		else if (support & NF_IPCAM_FPS_120) { rtn = NF_IPCAM_FPS_150; }
		else if (support & NF_IPCAM_FPS_70)  { rtn = NF_IPCAM_FPS_70;  }
		else if (support & NF_IPCAM_FPS_60)  { rtn = NF_IPCAM_FPS_70;  }
		else if (support & NF_IPCAM_FPS_30)  { rtn = NF_IPCAM_FPS_30;  }
		else if (support & NF_IPCAM_FPS_20)  { rtn = NF_IPCAM_FPS_20;  }
		else if (support & NF_IPCAM_FPS_10)  { rtn = NF_IPCAM_FPS_300;  }
	}
	else if (db_fps >= 15)
	{
		if (support & NF_IPCAM_FPS_150) { rtn = NF_IPCAM_FPS_150; }
		else if (support & NF_IPCAM_FPS_120) { rtn = NF_IPCAM_FPS_150; }
		else if (support & NF_IPCAM_FPS_70)  { rtn = NF_IPCAM_FPS_70;  }
		else if (support & NF_IPCAM_FPS_60)  { rtn = NF_IPCAM_FPS_70;  }
		else if (support & NF_IPCAM_FPS_30)  { rtn = NF_IPCAM_FPS_30;  }
		else if (support & NF_IPCAM_FPS_20)  { rtn = NF_IPCAM_FPS_20;  }
		else if (support & NF_IPCAM_FPS_10)  { rtn = NF_IPCAM_FPS_300;  }
	}
	else if (db_fps >= 12)
	{
		if (support & NF_IPCAM_FPS_120) { rtn = NF_IPCAM_FPS_150; }
		else if (support & NF_IPCAM_FPS_70)  { rtn = NF_IPCAM_FPS_70;  }
		else if (support & NF_IPCAM_FPS_60)  { rtn = NF_IPCAM_FPS_70;  }
		else if (support & NF_IPCAM_FPS_30)  { rtn = NF_IPCAM_FPS_30;  }
		else if (support & NF_IPCAM_FPS_20)  { rtn = NF_IPCAM_FPS_20;  }
		else if (support & NF_IPCAM_FPS_10)  { rtn = NF_IPCAM_FPS_300;  }
	}
	else if (db_fps >= 7)
	{
		if (support & NF_IPCAM_FPS_70)  { rtn = NF_IPCAM_FPS_70;  }
		else if (support & NF_IPCAM_FPS_60)  { rtn = NF_IPCAM_FPS_70;  }
		else if (support & NF_IPCAM_FPS_30)  { rtn = NF_IPCAM_FPS_30;  }
		else if (support & NF_IPCAM_FPS_20)  { rtn = NF_IPCAM_FPS_20;  }
		else if (support & NF_IPCAM_FPS_10)  { rtn = NF_IPCAM_FPS_300;  }
	}
	else if (db_fps >= 6)
	{
		if (support & NF_IPCAM_FPS_60)  { rtn = NF_IPCAM_FPS_70;  }
		else if (support & NF_IPCAM_FPS_30)  { rtn = NF_IPCAM_FPS_30;  }
		else if (support & NF_IPCAM_FPS_20)  { rtn = NF_IPCAM_FPS_20;  }
		else if (support & NF_IPCAM_FPS_10)  { rtn = NF_IPCAM_FPS_300;  }
	}
	else if (db_fps >= 3)
	{
		if (support & NF_IPCAM_FPS_30)  { rtn = NF_IPCAM_FPS_30;  }
		else if (support & NF_IPCAM_FPS_20)  { rtn = NF_IPCAM_FPS_20;  }
		else if (support & NF_IPCAM_FPS_10)  { rtn = NF_IPCAM_FPS_300;  }
	}
	else if (db_fps >= 2)
	{
		if (support & NF_IPCAM_FPS_20)  { rtn = NF_IPCAM_FPS_20;  }
		else if (support & NF_IPCAM_FPS_10)  { rtn = NF_IPCAM_FPS_300;  }
	}
	else
	{
		if (support & NF_IPCAM_FPS_300)  { rtn = NF_IPCAM_FPS_300;  }
		else if (support & NF_IPCAM_FPS_250)  { rtn = NF_IPCAM_FPS_250;  }
	}

	//IPCAM_DBG(MINOR, "%08x\n" , rtn);

	return rtn;
}

void nf_ipcam_init_poe_off_time(void)
{
	gint i=0;
	for (i=0; i<NUM_IPX_CHANNEL; i++)
	{
		g_get_current_time(&_ipcam_poe_off_time[i]);
	}
}
void nf_ipcam_set_poe_off_time(gint ch)
{
	g_get_current_time(&_ipcam_poe_off_time[ch]);
}
GTimeVal nf_ipcam_get_poe_off_time(gint ch)
{
	return (GTimeVal) _ipcam_poe_off_time[ch];
}

/**
 * @brief 카메라의 비디오 스트림 설정을 한다(ITX카메라 전용).
 * @param ch 채널 번호.
 * @param info 스트림 설정 정보 struct.
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_set_vcodec
(gint ch, NFIPCamSetupVCodec* info,
		NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	int i;
	int rtn;
	int major = 0, type = 0, subtype = 0, minor = 0; // Camera SDK Version
	float tmp_bitrate;
	int bitrate_table_id;
	cam_info mgr_info;
	mtable* runtime = NULL;
	GTimeVal setup_time;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	memset(&mgr_info, 0x00, sizeof(cam_info));
	runtime = get_runtime();

	g_return_val_if_fail( info != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( info->stream_cnt <= 3, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(
			(((1<<info->ntsc_pal) == NF_IPCAM_AF_NTSC) || ((1<<info->ntsc_pal) == NF_IPCAM_AF_PAL)),
			IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_READY, IPCAM_SETUP_RTN_FAILED);

	//IPCAM_DBG(MINOR, "stream_cnt(%d) ntsc_pal(%d) mirror(%x)\n",
	//			info->stream_cnt, info->ntsc_pal, info->mirror);

	for(i = 0; i < info->stream_cnt; i++)
	{
		//IPCAM_DBG(MINOR, "stream%d: resol(0x%02x) fps(0x%x) bitrate(%d)\n", i,
		//		info->resolution[i],
		//		info->fps[i],
		//		info->bitrate[i]);
	}

	for (i = 0; i < info->stream_cnt; i++)
	{
		if ((runtime[ch].video.resolution.supported & info->resolution[i]) == 0)
		{
			//IPCAM_DBG(MINOR, "resolution mismatch(%08x->%08x)\n",
			//		info->resolution[i], runtime[ch].video.resolution.resolution[i]);
			info->resolution[i] = runtime[ch].video.resolution.resolution[i];
		}
		mgr_info.vcodec.resolution[i] = info->resolution[i];
		mgr_info.vcodec.fps[i] = info->fps[i];
		mgr_info.vcodec.bitctrl[i] = info->bitctrl[i];
		mgr_info.vcodec.vcodec[i] = info->vcodec[i];
		if (info->ntsc_pal == 1)
		{
			mgr_info.vcodec.fps[i] = info->fps[i];
			switch(info->fps[i])
			{
				case NF_IPCAM_FPS_300:
					mgr_info.vcodec.fps[i] = NF_IPCAM_FPS_250;
					break;
				case NF_IPCAM_FPS_150:
					mgr_info.vcodec.fps[i] = NF_IPCAM_FPS_120;
					break;
				case NF_IPCAM_FPS_70:
					mgr_info.vcodec.fps[i] = NF_IPCAM_FPS_60;
					break;
			}
		}

		mgr_info.vcodec.bitrate[i] = info->bitrate[i];
		if (runtime[ch].sys.model_code == NF_IPCAM_MODEL_TI_368)
		{
#if 0
			// 5M Hisilicon 예외처리
			if(strstr(runtime[ch].sys.stdver, "-5007") == NULL)
			{
				switch (info->fps[i])
				{
					case NF_IPCAM_FPS_150:
					case NF_IPCAM_FPS_120:
						mgr_info.vcodec.bitrate[i] /= 2;
						break;
					case NF_IPCAM_FPS_70:
					case NF_IPCAM_FPS_60:
						mgr_info.vcodec.bitrate[i] /= 4;
						break;
					case NF_IPCAM_FPS_30:
						mgr_info.vcodec.bitrate[i] /= 5;
						break;
					case NF_IPCAM_FPS_20:
						mgr_info.vcodec.bitrate[i] /= 6;
						break;
					case NF_IPCAM_FPS_300:
					default:
						break;
				}
			}
#if LIMIT_DM368_512K
			if (mgr_info.vcodec.bitrate[i] < 512)
			{
				mgr_info.vcodec.bitrate[i] = 512;
			}
#endif
#endif
			
			
			sscanf(runtime[ch].sys.sdkver, "%d.%d.%d.%d", &major, &type, &subtype, &minor);
#if defined(_IPX_1648P)||defined(_IPX_0824VE)||defined(_IPX_0412VE)||defined(_IPX_0824ECO)||defined(_IPX_0412ECO) \
		       || defined(_IPX_1648VE3) || defined(_IPX_1648P3) || defined(_IPX_1648P3ECO) || defined(_IPX_1648P4) \
		       || defined(_IPX_1624M4) || defined(_IPX_0824M4) || defined(_IPX_0412M4) || defined(_IPX_1624P4E) || defined(_IPX_0824P4E) \
		       || defined(_IPX_0412M4E) || defined(_IPX_0824M4E) || defined(_IPX_1648M4E)
			if(strstr(runtime[ch].sys.stdver, "-5007") != NULL && i == 0 && type == ITX_CAM_SDK_TYPE_HS )
			{
				bitrate_table_id = 1;
			}
			else
			{
				bitrate_table_id = 0;
			}
#else
			bitrate_table_id = 0;
#endif
			tmp_bitrate = mgr_info.vcodec.bitrate[i];
			switch (info->fps[i])
			{
				case NF_IPCAM_FPS_150:
				case NF_IPCAM_FPS_120:
					mgr_info.vcodec.bitrate[i] = (tmp_bitrate * _BITRATE_COVERAGE_RATE_[bitrate_table_id][0]) + 0.5;
					//mgr_info.vcodec.bitrate[i] = (tmp_bitrate * 0.7) + 0.5;
					break;
				case NF_IPCAM_FPS_70:
				case NF_IPCAM_FPS_60:
					mgr_info.vcodec.bitrate[i] = (tmp_bitrate * _BITRATE_COVERAGE_RATE_[bitrate_table_id][1]) + 0.5;
					break;
				case NF_IPCAM_FPS_30:
					mgr_info.vcodec.bitrate[i] = (tmp_bitrate * _BITRATE_COVERAGE_RATE_[bitrate_table_id][2]) + 0.5;
					break;
				case NF_IPCAM_FPS_20:
					mgr_info.vcodec.bitrate[i] = (tmp_bitrate * _BITRATE_COVERAGE_RATE_[bitrate_table_id][3]) + 0.5;
					break;
				case NF_IPCAM_FPS_300:
				default:
					break;
			}
			if(mgr_info.vcodec.bitrate[i] < 512) // min bitrate = 512
			{
				mgr_info.vcodec.bitrate[i] = 512;
			}


		}
	}
	mgr_info.vcodec.af = (unsigned int) (((1<<info->ntsc_pal) == NF_IPCAM_AF_PAL) ? 50:60);
	mgr_info.vcodec.mirror = info->mirror;
	mgr_info.vcodec.capture= info->capture;

	runtime[ch].rec_changed = RATE_IGNORE_TIME;
	runtime[ch].rate_control = 0;
	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_VCODEC, cb_fxn, user_data, &mgr_info);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "vcodec setup request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
			//vcodec_setup_time check
			g_get_current_time(&setup_time);
			runtime[ch].setup_time = setup_time.tv_sec;
			IPCAM_DBG(MAJOR, "vcodec setup done CH(%d) : setup time(%d) \n", ch, runtime[ch].setup_time);
		case IPCAM_SETUP_RTN_PENDING:
			runtime[ch].video.resolution.resolution[0] = info->resolution[0];
			runtime[ch].video.resolution.resolution[1] = info->resolution[1];
			runtime[ch].video.fps[info->ntsc_pal][0].value = info->fps[0];
			runtime[ch].video.fps[info->ntsc_pal][1].value = info->fps[1];
			runtime[ch].video.bitrate[0].value = info->bitrate[0];
			runtime[ch].video.bitrate[1].value = info->bitrate[1];
			runtime[ch].video.anti_flicker.value = (1<<info->ntsc_pal);
			runtime[ch].video.mirror.value = info->mirror;
			runtime[ch].video.capture.value = info->capture;
			runtime[ch].video.bitctrl[0] = info->bitctrl[0];
			runtime[ch].video.bitctrl[1] = info->bitctrl[1];
			runtime[ch].video.vcodec[0] = info->vcodec[0];
			runtime[ch].video.vcodec[1] = info->vcodec[1];
			runtime[ch].rec_changed = RATE_IGNORE_TIME;
			runtime[ch].rate_control = 0;
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

typedef struct _NF_ONVIF_VCODEC_DATA
{
	pthread_t thread_id;
	int ch;
	NF_IPCAM_FPS_E fps1;
	NF_IPCAM_FPS_E fps2;
	NF_IPCAM_QUALITY_E qual1;
	NF_IPCAM_QUALITY_E qual2;
} NF_ONVIF_VCODEC_DATA;

/**
 * @brief 한 채널 녹화설정 변경 쓰레드.
 * @param p_vcodec_data Fps, quality 정보 struct.
 */
static void nf_ipcam_set_rec_vcodec_thread_func(NF_ONVIF_VCODEC_DATA *p_vcodec_data)
{
	/*
	printf("[%s] ch    : %d\n", __FUNCTION__, p_vcodec_data->ch);
	printf("[%s] fps1  : %d\n", __FUNCTION__, p_vcodec_data->fps1);
	printf("[%s] fps2  : %d\n", __FUNCTION__, p_vcodec_data->fps2);
	printf("[%s] qual1 : %d\n", __FUNCTION__, p_vcodec_data->qual1);
	printf("[%s] qual2 : %d\n", __FUNCTION__, p_vcodec_data->qual2);
	*/

	nf_ipcam_set_rec_vcodec(p_vcodec_data->ch,
			p_vcodec_data->fps1,
			p_vcodec_data->fps2,
			p_vcodec_data->qual1,
			p_vcodec_data->qual2,
			NULL, NULL, NULL);
}

void nf_ipcam_enqueue_rec_vcodec(NF_RECEVT_SET_ENCODER_CMD_PARAM_T *param)
{
	GAsyncQueue *recevt_queue = get_recevt_queue();
	IPX_PND_EVENT *evt = ipx_ipcam_new_event(IPCAM_DS_SET_ENCODER);
	IPCAM_DBG(MAJOR, "start\n");
	evt->p.ptr = param;
	evt->p.len = sizeof(NF_RECEVT_SET_ENCODER_CMD_PARAM_T);
	g_async_queue_push(recevt_queue, evt);
	IPCAM_DBG(MAJOR, "end\n");
}

/**
 * @brief 전 채널 녹화설정 변경 쓰레드.
 * @param p_ch_mask 변경되는 채널 bitmask.
 * @param p_fps1 1st stream fps값 배열.
 * @param p_fps2 2nd stream fps값 배열.
 * @param p_qual1 1st stream quality값 배열.
 * @param p_qual2 2nd stream quality값 배열.
 * @return 항상 0.
 */
int nf_ipcam_set_rec_vcodec_thread(gint p_ch_mask,
		NF_IPCAM_FPS_E *p_fps1, NF_IPCAM_FPS_E *p_fps2,
		NF_IPCAM_QUALITY_E *p_qual1, NF_IPCAM_QUALITY_E *p_qual2)
{
	NF_ONVIF_VCODEC_DATA vcodec_data[NUM_IPX_CHANNEL];
    int policy;
    struct sched_param sched;
	int i, ret;
	int thread_ch_mask = 0;

	policy = SCHED_FIFO;
	sched.sched_priority = sched_get_priority_max(policy) - 1;

	IPCAM_DBG(MAJOR, "start\n");
	for (i = 0; i < NUM_ACTIVE_CH; i++)
	{

		if (p_ch_mask & (1<<i))
		{
			vcodec_data[i].ch = i;
			vcodec_data[i].fps1 = p_fps1[i];
			vcodec_data[i].fps2 = p_fps2[i];
			vcodec_data[i].qual1 = p_qual1[i];
			vcodec_data[i].qual2 = p_qual2[i];

			ret = pthread_create(&(vcodec_data[i].thread_id), NULL, nf_ipcam_set_rec_vcodec_thread_func,
					&vcodec_data[i]);

			if (ret == 0)
			{
				ret = pthread_setschedparam (vcodec_data[i].thread_id, policy, &sched);
				thread_ch_mask |= (1<<i);
			}
			else
			{
				IPCAM_DBG(ERROR, "pthread_create(nf_ipcam_set_rec_vcodec_thread_func) failed (CH:[%d]|ret:[%d])\n", i, ret);
			}
		}
	}

	IPCAM_DBG(MAJOR, "waiting join\n");
	for (i = 0; i < NUM_IPX_CHANNEL; i++)
	{
		if (thread_ch_mask & (1<<i) )
		{
			pthread_join(vcodec_data[i].thread_id, NULL);
		}
	}
	IPCAM_DBG(MAJOR, "end\n");

	return 0;
}

/**
 * @brief 녹화설정 변경시 카메라의 비디오 스트림 설정을 한다.
 * @param ch 채널 번호.
 * @param fps1 1st stream fps값.
 * @param fps2 2nd stream fps값.
 * @param qual1 1st stream quality값.
 * @param qual2 2nd stream quality값.
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *
 * Onthefly(스트림 연결이 끊기지 않은 상태에서 설정 변경 가능) 지원하는 카메라만 설정한다.
 */
int nf_ipcam_set_rec_vcodec
(gint ch, NF_IPCAM_FPS_E fps1, NF_IPCAM_FPS_E fps2,
		NF_IPCAM_QUALITY_E qual1, NF_IPCAM_QUALITY_E qual2,
		NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	int i;
	int rtn;
	int ntpal;
	unsigned int bitrate_to_apply_1 = 0;
	unsigned int bitrate_to_apply_2 = 0;
	cam_info mgr_info;
	mtable* runtime = NULL;
	unsigned int db_fps1;
	unsigned int db_fps2;
	unsigned int req_fps1;
	unsigned int req_fps2;
	char key[64];
	int major = 0, type = 0, subtype = 0, minor = 0; // Camera SDK Version
	float tmp_bitrate, tmp_bitrate_rate[5]; 
	int bitrate_table_id;
	int smart_storage_flag = 1;

	IPCAM_DBG(MAJOR, "start CH(%d) fps(%x,%x) quality(%x,%x)\n", ch, fps1, fps2, qual1, qual2);

	memset(&mgr_info, 0x00, sizeof(cam_info));
	runtime = get_runtime();

	if (ch >= NUM_IPX_CHANNEL) { return IPCAM_SETUP_RTN_FAILED; }
	if (runtime == NULL) { return IPCAM_SETUP_RTN_FAILED; }
	if (runtime[ch].state & MGMT_STATE_CONFIGURED == 0) { return IPCAM_SETUP_RTN_FAILED; }
	//g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	//g_return_val_if_fail( runtime != NULL, IPCAM_SETUP_RTN_FAILED);
	//g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);
	if ((get_vloss_status() & (1<<ch)) != 0)
	{
		IPCAM_DBG(MINOR, "WARN: CH(%d) initial setup going. VCodec setup does not applied\n", ch);
		return IPCAM_SETUP_RTN_FAILED;
	}
	//g_return_val_if_fail( runtime[ch].sys.model_code == NF_IPCAM_MODEL_TI_368 || runtime[ch].sys.model_code == NF_IPCAM_MODEL_AMB_A2 || runtime[ch].sys.model_code == NF_IPCAM_MODEL_AMB_D1 || runtime[ch].sys.model_code == NF_NVS_MODEL_ITX, IPCAM_SETUP_RTN_FAILED);
	//
	
	if (nf_ipcam_is_config_changable(ch) == 0)
	{
		IPCAM_DBG(MINOR, "CH(%d) cannot be changed codec setup\n", ch);
		return IPCAM_SETUP_RTN_DONE;
	}


	snprintf(key, 64, "rec.smart_storage");
	smart_storage_flag = nf_sysdb_get_uint(key);
	if(!smart_storage_flag)
	{
		qual1 = NF_IPCAM_QUALITY_SUPER; 
		qual2 = NF_IPCAM_QUALITY_SUPER; 
	}

	ntpal = (runtime[ch].video.anti_flicker.value == NF_IPCAM_AF_NTSC) ? 0:1;
	mgr_info.vcodec.resolution[0] = runtime[ch].video.resolution.resolution[0];
	mgr_info.vcodec.resolution[1] = runtime[ch].video.resolution.resolution[1];

	snprintf(key, 64, "cam.C%d.stream.S0.fps", ch);
	db_fps1 = nf_sysdb_get_uint(key);
	snprintf(key, 64, "cam.C%d.stream.S1.fps", ch);
	db_fps2 = nf_sysdb_get_uint(key);
	if (ntpal == 1)
	{
		switch(fps1)
		{
			case NF_IPCAM_FPS_300:
				req_fps1 = NF_IPCAM_FPS_250;
				break;
			case NF_IPCAM_FPS_150:
				req_fps1 = NF_IPCAM_FPS_120;
				break;
			case NF_IPCAM_FPS_70:
				req_fps1 = NF_IPCAM_FPS_60;
				break;
			default:
				req_fps1 = fps1;
				break;
		}
		switch(fps2)
		{
			case NF_IPCAM_FPS_300:
				req_fps2 = NF_IPCAM_FPS_250;
				break;
			case NF_IPCAM_FPS_150:
				req_fps2 = NF_IPCAM_FPS_120;
				break;
			case NF_IPCAM_FPS_70:
				req_fps2 = NF_IPCAM_FPS_60;
				break;
			default:
				req_fps2 = fps2;
				break;
		}
	}
	else
	{
		req_fps1 = fps1;
		req_fps2 = fps2;
	}
	req_fps1 = _get_fps_num(req_fps1);
	req_fps2 = _get_fps_num(req_fps2);
	if (db_fps1 < req_fps1)
	{
		fps1 = _get_max_fps_mask(db_fps1, runtime[ch].video.fps[ntpal][0].support);
	}
	if (db_fps2 < req_fps2)
	{
		fps2 = _get_max_fps_mask(db_fps2, runtime[ch].video.fps[ntpal][1].support);
	}

	mgr_info.vcodec.fps[0] = fps1;
	mgr_info.vcodec.fps[1] = fps2;

	if (ntpal == 1)
	{
		switch(fps1)
		{
			case NF_IPCAM_FPS_300:
				mgr_info.vcodec.fps[0] = NF_IPCAM_FPS_250;
				break;
			case NF_IPCAM_FPS_150:
				mgr_info.vcodec.fps[0] = NF_IPCAM_FPS_120;
				break;
			case NF_IPCAM_FPS_70:
				mgr_info.vcodec.fps[0] = NF_IPCAM_FPS_60;
				break;
			default:
				break;
		}
		switch(fps2)
		{
			case NF_IPCAM_FPS_300:
				mgr_info.vcodec.fps[1] = NF_IPCAM_FPS_250;
				break;
			case NF_IPCAM_FPS_150:
				mgr_info.vcodec.fps[1] = NF_IPCAM_FPS_120;
				break;
			case NF_IPCAM_FPS_70:
				mgr_info.vcodec.fps[1] = NF_IPCAM_FPS_60;
				break;
			default:
				break;
		}
	}

	if ((runtime[ch].sys.model_code == NF_IPCAM_MODEL_AMB_A2) || (runtime[ch].sys.model_code == NF_IPCAM_MODEL_AMB_D1))
	{
		bitrate_to_apply_1 = runtime[ch].video.bitrate[0].value;
		bitrate_to_apply_2 = runtime[ch].video.bitrate[1].value;
		mgr_info.vcodec.bitrate[0] = runtime[ch].video.bitrate[0].value;
		mgr_info.vcodec.bitrate[1] = runtime[ch].video.bitrate[1].value;
	}
#if 0
	else if (runtime[ch].sys.model_code == NF_IPCAM_MODEL_TI_368)
	{
		bitrate_to_apply = runtime[ch].video.quality[0][qual1];
		if (runtime[ch].intensive_ch)
		{
			mgr_info.vcodec.bitrate[0] = 10000;
		}
		else
		{
			mgr_info.vcodec.bitrate[0] = runtime[ch].video.quality[0][qual1];
		}
		if (fps1 == NF_IPCAM_FPS_150 || fps1 == NF_IPCAM_FPS_120)
		{
			mgr_info.vcodec.bitrate[0] /= 2;
		}
		else if (fps1 == NF_IPCAM_FPS_70 || fps1 == NF_IPCAM_FPS_60)
		{
			mgr_info.vcodec.bitrate[0] /= 3;
		}
		if (mgr_info.vcodec.bitrate[0] < 600)
		{
			mgr_info.vcodec.bitrate[0] = 600;
		}

		mgr_info.vcodec.bitrate[1] = runtime[ch].video.quality[1][qual2];
		if (fps2 == NF_IPCAM_FPS_150 || fps2 == NF_IPCAM_FPS_120)
		{
			mgr_info.vcodec.bitrate[1] /= 2;
		}
		else if (fps2 == NF_IPCAM_FPS_70 || fps2 == NF_IPCAM_FPS_60)
		{
			mgr_info.vcodec.bitrate[1] /= 3;
		}
		if (mgr_info.vcodec.bitrate[1] < 600)
		{
			mgr_info.vcodec.bitrate[1] = 600;
		}
	}
#else

#if 0
	else if (runtime[ch].sys.model_code == NF_IPCAM_MODEL_TI_368)
	{
		bitrate_to_apply_1 = runtime[ch].video.quality[0][qual1];
		bitrate_to_apply_2 = runtime[ch].video.quality[1][qual2];
		mgr_info.vcodec.bitrate[0] = runtime[ch].video.quality[0][qual1];
		mgr_info.vcodec.bitrate[1] = runtime[ch].video.quality[1][qual2];
		// 5M Hisilicon 예외처리
		if(strstr(runtime[ch].sys.stdver, "-5007") == NULL)
		{
			switch(mgr_info.vcodec.fps[0])
			{
				case NF_IPCAM_FPS_150:
				case NF_IPCAM_FPS_120:
					mgr_info.vcodec.bitrate[0] /= 2;
					break;
				case NF_IPCAM_FPS_70:
				case NF_IPCAM_FPS_60:
					mgr_info.vcodec.bitrate[0] /= 4;
					break;
				case NF_IPCAM_FPS_30:
					mgr_info.vcodec.bitrate[0] /= 5;
					break;
				case NF_IPCAM_FPS_20:
					mgr_info.vcodec.bitrate[0] /= 6;
					break;
				case NF_IPCAM_FPS_300:
				default:
					break;
			}
		}
#if LIMIT_DM368_512K
		if (mgr_info.vcodec.bitrate[0] < 1000) { mgr_info.vcodec.bitrate[0] = 1000; }
#endif
		// 5M Hisilicon 예외처리
		if(strstr(runtime[ch].sys.stdver, "-5007") == NULL)
		{
			switch(mgr_info.vcodec.fps[1])
			{
				case NF_IPCAM_FPS_150:
				case NF_IPCAM_FPS_120:
					mgr_info.vcodec.bitrate[1] /= 2;
					break;
				case NF_IPCAM_FPS_70:
				case NF_IPCAM_FPS_60:
					mgr_info.vcodec.bitrate[1] /= 4;
					break;
				case NF_IPCAM_FPS_30:
					mgr_info.vcodec.bitrate[1] /= 5;
					break;
				case NF_IPCAM_FPS_20:
					mgr_info.vcodec.bitrate[1] /= 6;
					break;
				case NF_IPCAM_FPS_300:
				default:
					break;
			}
		}
#if LIMIT_DM368_512K
		if (mgr_info.vcodec.bitrate[1] < 512) { mgr_info.vcodec.bitrate[1] = 512; }
#endif
	}

#endif
	else if (runtime[ch].sys.model_code == NF_IPCAM_MODEL_TI_368)
	{
		//printf("\e[31m %s %d Ti 368 \e[0m\n", __func__, __LINE__);
		bitrate_to_apply_1 = runtime[ch].video.quality[0][qual1];
		bitrate_to_apply_2 = runtime[ch].video.quality[1][qual2];
		mgr_info.vcodec.bitrate[0] = runtime[ch].video.quality[0][qual1];
		mgr_info.vcodec.bitrate[1] = runtime[ch].video.quality[1][qual2];

		
		for(i = 0; i < runtime[ch].video.stream_cnt; i++)
		{
			sscanf(runtime[ch].sys.sdkver, "%d.%d.%d.%d", &major, &type, &subtype, &minor);
#if defined(_IPX_1648P)||defined(_IPX_0824VE)||defined(_IPX_0412VE)||defined(_IPX_0824ECO)||defined(_IPX_0412ECO) \
			|| defined(_IPX_1648VE3) || defined(_IPX_1648P3) || defined(_IPX_1648P3ECO) || defined(_IPX_1648P4) \
			|| defined(_IPX_1624M4) || defined(_IPX_0824M4) || defined(_IPX_0412M4) || defined(_IPX_1624P4E) || defined(_IPX_0824P4E) \
			|| defined(_IPX_0412M4E) || defined(_IPX_0824M4E) || defined(_IPX_1648M4E)
			if(strstr(runtime[ch].sys.stdver, "-5007") != NULL && i == 0 && type == ITX_CAM_SDK_TYPE_HS )
			{
				bitrate_table_id = 1;
			}
			else
			{
				bitrate_table_id = 0;
			}
#else
			bitrate_table_id = 0;
#endif
			tmp_bitrate = mgr_info.vcodec.bitrate[i];
			if(smart_storage_flag)
			{
				switch(mgr_info.vcodec.fps[i])
				{
					case NF_IPCAM_FPS_150:
					case NF_IPCAM_FPS_120:
						mgr_info.vcodec.bitrate[i] = (tmp_bitrate * _BITRATE_COVERAGE_RATE_[bitrate_table_id][0]) + 0.5;
						break;
					case NF_IPCAM_FPS_70:
					case NF_IPCAM_FPS_60:
						mgr_info.vcodec.bitrate[i] = (tmp_bitrate * _BITRATE_COVERAGE_RATE_[bitrate_table_id][1]) + 0.5;
						break;
					case NF_IPCAM_FPS_30:
						mgr_info.vcodec.bitrate[i] = (tmp_bitrate * _BITRATE_COVERAGE_RATE_[bitrate_table_id][2]) + 0.5;
						break;
					case NF_IPCAM_FPS_20:
						mgr_info.vcodec.bitrate[i] = (tmp_bitrate * _BITRATE_COVERAGE_RATE_[bitrate_table_id][3]) + 0.5;
						break;
					case NF_IPCAM_FPS_300:
					default:
						break;
				}
			}
			if(mgr_info.vcodec.bitrate[i] < 512) // min bitrate = 512
			{
				mgr_info.vcodec.bitrate[i] = 512;
			}

		}
	}

#endif
	else if(runtime[ch].sys.model_code == NF_NVS_MODEL_ITX)
	{
		bitrate_to_apply_1 = runtime[ch].video.quality[0][qual1];
		bitrate_to_apply_2 = runtime[ch].video.quality[1][qual2];
		mgr_info.vcodec.bitrate[0] = runtime[ch].video.quality[0][qual1];
		mgr_info.vcodec.bitrate[1] = runtime[ch].video.quality[1][qual2];
	}
	else if(strcmp(runtime[ch].sys.vendor,"H264")==0)	// XIONGMAI
	{
		bitrate_to_apply_1 = runtime[ch].video.quality[0][qual1];
		bitrate_to_apply_2 = runtime[ch].video.quality[1][qual2];
		mgr_info.vcodec.bitrate[0] = qual1;
		mgr_info.vcodec.bitrate[1] = qual2;
	}
	else if(nf_ipcam_is_onvif_support(ch) == 1)
	{
		//IPCAM_DBG(MINOR, "support %08x onthefly %08x\n", runtime[ch].video.supported, runtime[ch].video.onthefly);
		/* CBC? check BPM-182! */
		//mgr_info.vcodec.bitrate[0] = runtime[ch].video.quality[0][qual1];
		//mgr_info.vcodec.bitrate[1] = runtime[ch].video.quality[1][qual2];
#if 1
		float tmp_bitrate = 0;
		if(!nf_ipcam_is_vendor("CBC") &&
		(!(runtime[ch].video.supported & runtime[ch].video.onthefly & VIDEO_SETUP_BITRATE)
		|| !(runtime[ch].video.supported & runtime[ch].video.onthefly & VIDEO_SETUP_FPS)))
		{
			return IPCAM_SETUP_RTN_FAILED;
		}
		bitrate_to_apply_1 = runtime[ch].video.quality[0][qual1];
		bitrate_to_apply_2 = runtime[ch].video.quality[1][qual2];
		mgr_info.vcodec.bitrate[0] = runtime[ch].video.quality[0][qual1];
		mgr_info.vcodec.bitrate[1] = runtime[ch].video.quality[1][qual2];

		tmp_bitrate = mgr_info.vcodec.bitrate[0];
		if(smart_storage_flag)
		{
			switch(mgr_info.vcodec.fps[0])
			{
				case NF_IPCAM_FPS_150:
				case NF_IPCAM_FPS_120:
					mgr_info.vcodec.bitrate[0]  = (tmp_bitrate * 0.7) + 0.5; // 반올림
					break;
				case NF_IPCAM_FPS_70:
				case NF_IPCAM_FPS_60:
					mgr_info.vcodec.bitrate[0] = (tmp_bitrate * 0.5) + 0.5; // 반올림
					break;
				case NF_IPCAM_FPS_30:
					mgr_info.vcodec.bitrate[0] = (tmp_bitrate * 0.4) + 0.5; // 반올림
					break;
				case NF_IPCAM_FPS_20:
					mgr_info.vcodec.bitrate[0] = (tmp_bitrate * 0.35) + 0.5; // 반올림
					break;
				case NF_IPCAM_FPS_300:
				default:
					break;
			}

			tmp_bitrate = mgr_info.vcodec.bitrate[1];
			switch(mgr_info.vcodec.fps[1])
			{
				case NF_IPCAM_FPS_150:
				case NF_IPCAM_FPS_120:
					mgr_info.vcodec.bitrate[1]  = (tmp_bitrate * 0.7) + 0.5; // 반올림
					break;
				case NF_IPCAM_FPS_70:
				case NF_IPCAM_FPS_60:
					mgr_info.vcodec.bitrate[1] = (tmp_bitrate * 0.5) + 0.5; // 반올림
					break;
				case NF_IPCAM_FPS_30:
					mgr_info.vcodec.bitrate[1] = (tmp_bitrate * 0.4) + 0.5; // 반올림
					break;
				case NF_IPCAM_FPS_20:
					mgr_info.vcodec.bitrate[1] = (tmp_bitrate * 0.35) + 0.5; // 반올림
					break;
				case NF_IPCAM_FPS_300:
				default:
					break;
			}
		}
#endif
	}
	else
	{
		//g_return_val_if_fail( runtime[ch].sys.model_code == NF_IPCAM_MODEL_TI_368 || runtime[ch].sys.model_code == NF_IPCAM_MODEL_AMB_A2 || runtime[ch].sys.model_code == NF_IPCAM_MODEL_AMB_D1 || runtime[ch].sys.model_code == NF_NVS_MODEL_ITX, IPCAM_SETUP_RTN_FAILED);
		return IPCAM_SETUP_RTN_FAILED;
	}

	mgr_info.vcodec.af = (runtime[ch].video.anti_flicker.value == NF_IPCAM_AF_PAL) ? 50:60;
	mgr_info.vcodec.mirror = runtime[ch].video.mirror.value;

	IPCAM_DBG(MINOR, " minto | ch(%d) fps(%x,%x) quality(%d,%d) bitrate(%lu,%lu)\n", ch,
			fps1, fps2, qual1, qual2,
			mgr_info.vcodec.bitrate[0],
			mgr_info.vcodec.bitrate[1]);
	for(i = 0; i < 2; i++)
	{
		IPCAM_DBG(MINOR, "minto | stream%d: resol(0x%08x) fps(0x%x) bitrate(%d)\n", i,
				mgr_info.vcodec.resolution[i],
				mgr_info.vcodec.fps[i],
				mgr_info.vcodec.bitrate[i]);
	}

	runtime[ch].rec_changed = RATE_IGNORE_TIME;
	if(runtime[ch].video.supported & VIDEO_SETUP_CAPTURE_MODE )
	{
		mgr_info.vcodec.capture = runtime[ch].video.capture.value;	
	}
	else
	{
		mgr_info.vcodec.capture = 1; // NF_IPCAM_CAPTURE_UNUSED	
	}
	mgr_info.vcodec.bitctrl[0] = runtime[ch].video.bitctrl[0];
	mgr_info.vcodec.bitctrl[1] = runtime[ch].video.bitctrl[1];

	mgr_info.vcodec.vcodec[0] = runtime[ch].video.vcodec[0];
	mgr_info.vcodec.vcodec[1] = runtime[ch].video.vcodec[1];

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_VCODEC, cb_fxn, user_data, &mgr_info);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "vcodec setup request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			runtime[ch].video.fps[ntpal][0].value = mgr_info.vcodec.fps[0];
			runtime[ch].video.fps[ntpal][1].value = mgr_info.vcodec.fps[0];
			runtime[ch].video.bitrate[0].value = bitrate_to_apply_1;
			runtime[ch].video.bitrate[1].value = bitrate_to_apply_2;
			runtime[ch].rec_changed = RATE_IGNORE_TIME;
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

#if 0
int nf_ipcam_set_rec_vcodec_detail
(gint ch, NF_IPCAM_FPS_E fps1, NF_IPCAM_FPS_E fps2, gint br1, gint br2,
		NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	int rtn;
	int ntpal;
	unsigned int bitrate_to_apply = 0;
	cam_info mgr_info;
	mtable* runtime = NULL;

	IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	memset(&mgr_info, 0x00, sizeof(cam_info));
	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(
			runtime[ch].sys.model_code == NF_IPCAM_MODEL_TI_368 ||
			runtime[ch].sys.model_code == NF_IPCAM_MODEL_AMB_A2 ||
			runtime[ch].sys.model_code == NF_IPCAM_MODEL_AMB_D1,
			IPCAM_SETUP_RTN_FAILED
	);

	ntpal = (runtime[ch].video.anti_flicker.value == NF_IPCAM_AF_NTSC) ? 0:1;
	mgr_info.vcodec.resolution[0] = runtime[ch].video.resolution.resolution[0];
	mgr_info.vcodec.resolution[1] = runtime[ch].video.resolution.resolution[1];
	mgr_info.vcodec.fps[0] = fps1;
	mgr_info.vcodec.fps[1] = fps2;
	if ((runtime[ch].sys.model_code == NF_IPCAM_MODEL_AMB_A2) || (runtime[ch].sys.model_code == NF_IPCAM_MODEL_AMB_D1))
	{
		mgr_info.vcodec.bitrate[0] = runtime[ch].video.bitrate[0].value;
		mgr_info.vcodec.bitrate[1] = runtime[ch].video.bitrate[1].value;
	}
	else if (runtime[ch].sys.model_code == NF_IPCAM_MODEL_TI_368)
	{
		mgr_info.vcodec.bitrate[0] = br1;
		switch(fps1)
		{
			case NF_IPCAM_FPS_150:
				mgr_info.vcodec.bitrate[0] /= 2;
				break;
			case NF_IPCAM_FPS_70:
				mgr_info.vcodec.bitrate[0] /= 4;
				break;
			case NF_IPCAM_FPS_30:
				mgr_info.vcodec.bitrate[0] /= 5;
				break;
			case NF_IPCAM_FPS_20:
				mgr_info.vcodec.bitrate[0] /= 6;
				break;
			case NF_IPCAM_FPS_300:
			default:
				break;
		}
#if LIMIT_DM368_512K
		if (mgr_info.vcodec.bitrate[0] < 1000) { mgr_info.vcodec.bitrate[0] = 1000; }
#endif

		mgr_info.vcodec.bitrate[1] = br2;
		switch(fps2)
		{
			case NF_IPCAM_FPS_150:
				mgr_info.vcodec.bitrate[1] /= 2;
				break;
			case NF_IPCAM_FPS_70:
				mgr_info.vcodec.bitrate[1] /= 4;
				break;
			case NF_IPCAM_FPS_30:
				mgr_info.vcodec.bitrate[1] /= 5;
				break;
			case NF_IPCAM_FPS_20:
				mgr_info.vcodec.bitrate[1] /= 6;
				break;
			case NF_IPCAM_FPS_300:
			default:
				break;
		}
#if LIMIT_DM368_512K
		if (mgr_info.vcodec.bitrate[1] < 512) { mgr_info.vcodec.bitrate[1] = 512; }
#endif
	}
	else
	{
		mgr_info.vcodec.bitrate[0] = br1;
		mgr_info.vcodec.bitrate[1] = br2;
	}
	mgr_info.vcodec.af = ntpal ? 50:60;
	if (ntpal == 1)
	{
		switch(mgr_info.vcodec.fps[0])
		{
			case NF_IPCAM_FPS_300:
				mgr_info.vcodec.fps[0] = NF_IPCAM_FPS_250;
				break;
			case NF_IPCAM_FPS_150:
				mgr_info.vcodec.fps[0] = NF_IPCAM_FPS_120;
				break;
			case NF_IPCAM_FPS_70:
				mgr_info.vcodec.fps[0] = NF_IPCAM_FPS_60;
				break;
			default:
				break;
		}
		switch(mgr_info.vcodec.fps[1])
		{
			case NF_IPCAM_FPS_300:
				mgr_info.vcodec.fps[1] = NF_IPCAM_FPS_250;
				break;
			case NF_IPCAM_FPS_150:
				mgr_info.vcodec.fps[1] = NF_IPCAM_FPS_120;
				break;
			case NF_IPCAM_FPS_70:
				mgr_info.vcodec.fps[1] = NF_IPCAM_FPS_60;
				break;
			default:
				break;
		}
	}
	mgr_info.vcodec.mirror = runtime[ch].video.mirror.value;

	IPCAM_DBG(MINOR, "[%s] fps(%x,%x) bitrate(%lu,%lu)\n",
			__FUNCTION__, fps1, fps2,
			mgr_info.vcodec.bitrate[0],
			mgr_info.vcodec.bitrate[1]);

	runtime[ch].rec_changed = RATE_IGNORE_TIME;
	runtime[ch].rate_control = 0;
	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_VCODEC, cb_fxn, user_data, &mgr_info);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "fps setup request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			runtime[ch].rec_changed = RATE_IGNORE_TIME;
			runtime[ch].rate_control = 0;
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}
	runtime[ch].rec_changed = RATE_IGNORE_TIME;
	runtime[ch].rate_control = 0;
	IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);

	return rtn;
}
#endif

/**
 * @brief 카메라의 비디오 스트림 설정을 한다(ONVIF카메라 전용).
 * @param ch 채널 번호.
 * @param info 스트림 설정 정보 struct.
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_set_vcodec_onvif
(gint ch, NFIPCamSetupVCodec* info,
		NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	int i, rtn;
	float min_bitrate = 0;
	float tmp_bitrate = 0;
	mtable* runtime = NULL;
	cam_info mgr_info;
	runtime = get_runtime();


	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_READY, IPCAM_SETUP_RTN_FAILED);

	memset(&mgr_info, 0x00, sizeof(cam_info));


	IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	IPCAM_DBG(MINOR, "stream_cnt(%d) ntsc_pal(%d) mirror(%x)\n",
				info->stream_cnt, info->ntsc_pal, info->mirror);

	for(i = 0; i < info->stream_cnt; i++)
	{
		IPCAM_DBG(MINOR, "stream%d: resol(0x%02x) fps(0x%x) bitrate(%d)\n", i,
				info->resolution[i],
				info->fps[i],
				info->bitrate[i]);
	}

	for (i = 0; i < info->stream_cnt; i++)
	{
		/*if ((runtime[ch].video.resolution.supported & info->resolution[i]) == 0)
		{
			IPCAM_DBG(MINOR, "resolution mismatch(%08x->%08x)\n",
					info->resolution[i], runtime[ch].video.resolution.resolution[i]);
			info->resolution[i] = runtime[ch].video.resolution.resolution[i];
		}*/
		mgr_info.vcodec.resolution[i] = info->resolution[i];
		mgr_info.vcodec.vcodec[i] = runtime[ch].video.vcodec[i];
		mgr_info.vcodec.fps[i] = info->fps[i];
		if (info->ntsc_pal == 1)
		{
			mgr_info.vcodec.fps[i] = info->fps[i];
			switch(info->fps[i])
			{
				case NF_IPCAM_FPS_300:
					mgr_info.vcodec.fps[i] = NF_IPCAM_FPS_250;
					break;
				case NF_IPCAM_FPS_150:
					mgr_info.vcodec.fps[i] = NF_IPCAM_FPS_120;
					break;
				case NF_IPCAM_FPS_70:
					mgr_info.vcodec.fps[i] = NF_IPCAM_FPS_60;
					break;
			}
		}
		mgr_info.vcodec.bitctrl[i] = info->bitctrl[i];

		// 1st, 2nd min_bitrate : the lowest conditions (quality:lowest|fps:2)
		tmp_bitrate = runtime[ch].video.quality[i][0];
		min_bitrate = (tmp_bitrate * 0.35) + 0.5;

#if defined(_IPX_1648P4) || defined(_IPX_1648L4)
		if(i == 0)
		{
			if(info->bitrate[i] < min_bitrate)
				mgr_info.vcodec.bitrate[i] = min_bitrate;
			else
				mgr_info.vcodec.bitrate[i] = info->bitrate[i];
		}
#elif defined(_IPX_1648P)||defined(_IPX_0824VE)||defined(_IPX_0412VE)||defined(_IPX_0824ECO)||defined(_IPX_0412ECO) \
		|| defined(_IPX_1648VE3) || defined(_IPX_1648P3) || defined(_IPX_1648P3ECO)
		if(i == 0)
		{
			if(info->bitrate[i] > 4000)
				mgr_info.vcodec.bitrate[i] = 4000;// = info->bitrate[i];
			else if(info->bitrate[i] < min_bitrate)
				mgr_info.vcodec.bitrate[i] = min_bitrate;
			else
				mgr_info.vcodec.bitrate[i] = info->bitrate[i];
		}
#else
		if(i == 0)
		{
			if(info->bitrate[i] > 8000)
				mgr_info.vcodec.bitrate[i] = 8000;// = info->bitrate[i];
			else if(info->bitrate[i] < min_bitrate)
				mgr_info.vcodec.bitrate[i] = min_bitrate;
			else
				mgr_info.vcodec.bitrate[i] = info->bitrate[i];
		}
#endif
		else if(i == 1)
		{
			if(info->bitrate[i] > 1500)
				mgr_info.vcodec.bitrate[i] = 1500;// = info->bitrate[i];
			else if(info->bitrate[i] < min_bitrate)
				mgr_info.vcodec.bitrate[i] = min_bitrate;
			else
				mgr_info.vcodec.bitrate[i] = info->bitrate[i];
		}
	}
	mgr_info.vcodec.af = (unsigned int) (((1<<info->ntsc_pal) == NF_IPCAM_AF_PAL) ? 50:60);
	mgr_info.vcodec.mirror = info->mirror;
	mgr_info.vcodec.capture = info->capture;

	runtime[ch].rec_changed = RATE_IGNORE_TIME;
	runtime[ch].rate_control = 0;
	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_VCODEC, cb_fxn, user_data, &mgr_info);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "vcodec setup request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief 카메라의 Rotation 설정을 한다.
 * @param ch 채널 번호.
 * @param rotate Rotate 정보 enum값. @see NF_IPCAM_MIRROR_MODES_E
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *
 * ONVIF상에는 rotate(mirror)가 명확하게 정의되어 있지 않으며,
 * ONVIF 카메라 중 SDK구현한 일부 카메라(techwin, idis)만 지원한다.
 */
int nf_ipcam_set_rotation
(gint ch, gint rotate,
		NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	int i;
	int rtn;
	int ntpal;
	int otf = 0;
	cam_info info;
	mtable* runtime = NULL;

	//IPCAM_DBG(MAJOR, "start CH(%d) rotate(%d)\n", ch, rotate);

	memset(&info, 0x00, sizeof(cam_info));
	runtime = get_runtime();

	//g_return_val_if_fail( (rotate == 0 || rotate == 1) , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL, IPCAM_SETUP_RTN_FAILED);
	//g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);

	switch(runtime[ch].sys.model_code)
	{
		case NF_IPCAM_MODEL_TI_368:
			otf = 1;
			break;
		case NF_IPCAM_MODEL_AMB_A2:
		case NF_IPCAM_MODEL_AMB_D1:
			otf = 0;
			break;
		case NF_IPCAM_MODEL_ONVIF:
			if((runtime[ch].video.onthefly & VIDEO_SETUP_MIRROR)
			&& (runtime[ch].video.supported & VIDEO_SETUP_MIRROR))
			{
				otf = 1;
			}
			break;
		default:
			otf = 0;
			break;
	}
	g_return_val_if_fail( otf, IPCAM_SETUP_RTN_DONE);

	ntpal = (runtime[ch].video.anti_flicker.value == NF_IPCAM_AF_NTSC) ? 0:1;
	info.vcodec.resolution[0] = runtime[ch].video.resolution.resolution[0];
	info.vcodec.resolution[1] = runtime[ch].video.resolution.resolution[1];
	info.vcodec.vcodec[0] = runtime[ch].video.vcodec[0];
	info.vcodec.vcodec[1] = runtime[ch].video.vcodec[1];
	info.vcodec.fps[0] = runtime[ch].video.fps[ntpal][0].value;
	info.vcodec.fps[1] = runtime[ch].video.fps[ntpal][1].value;
	info.vcodec.bitrate[0] = runtime[ch].video.bitrate[0].value;
	info.vcodec.bitrate[1] = runtime[ch].video.bitrate[1].value;
	info.vcodec.af = (runtime[ch].video.anti_flicker.value == NF_IPCAM_AF_PAL) ? 50:60;

	info.vcodec.mirror = rotate;
#if 0
	if (rotate)
	{
		info.vcodec.mirror = NF_IPCAM_MIRROR_FLIP;
	}
	else
	{
		info.vcodec.mirror = NF_IPCAM_MIRROR_NONE;
	}
#endif
	info.vcodec.bitctrl[0] = runtime[ch].video.bitctrl[0];
	info.vcodec.bitctrl[1] = runtime[ch].video.bitctrl[1];
	info.vcodec.capture = runtime[ch].video.capture.value; 

	if(nf_ipcam_is_onvif_support(ch) == 1)
	{
		rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_MIRROR, cb_fxn, user_data, &info);
	}
	else
	{
		rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_VCODEC, cb_fxn, user_data, &info);
	}


	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "mirror setup request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			runtime[ch].video.mirror.value = info.vcodec.mirror;
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief Onthefly가 지원안되는 ITX카메라의 비디오 스트림 설정을 한다.
 * @param ch 채널 번호.
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *
 * @deprecated Sysdb기준으로 설정하며, 현재는 A2카메라의 rotation설정할 때만 사용한다.
 */
int nf_ipcam_set_vcodec_sysdb(gint ch,
		NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	int rtn = -1;
	cam_info info;
	mtable *runtime = NULL;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	memset(&info, 0x00, sizeof(cam_info));

	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);


	nf_ipcam_pause_ch(ch);
	nf_ipcam_setup_clear_cb(ch);

	runtime[ch].state |= MGMT_STATE_WAITING;

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "Non-otf setup request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}
	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);

	return rtn;
}

/**
 * @brief 카메라의 bitrate를 임의로 내린다.(TI카메라 전용)
 * @param ch 채널 번호.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *
 * 현재 S1향에서만 사용함.
 */
int nf_ipcam_rate_down(gint ch)
{
	int rtn;
	int ntpal;
	cam_info mgr_info;
	mtable* runtime = NULL;
	guint min_rate = 2500;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);
	memset(&mgr_info, 0x00, sizeof(cam_info));
	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL, IPCAM_SETUP_RTN_FAILED);
	if ( (runtime[ch].state & MGMT_STATE_CONFIGURED) == 0)
	{
		return IPCAM_SETUP_RTN_FAILED;
	}
	if ( runtime[ch].sys.model_code != NF_IPCAM_MODEL_TI_368)
	{
		return IPCAM_SETUP_RTN_FAILED;
	}
	if ((get_vloss_status() & (1<<ch)) != 0)
	{
		return IPCAM_SETUP_RTN_FAILED;
	}

	if (runtime[ch].rate_control > 0)
	{
		runtime[ch].rate_control = RATE_SUSTENANCE_TIME;
		return IPCAM_SETUP_RTN_DONE;
	}
	if (runtime[ch].rec_changed > 0)
	{
		return IPCAM_SETUP_RTN_DONE;
	}
	if (nf_ipcam_get_setup_queuelen(ch, NF_IPCAM_TYPE_SET_VCODEC) > 0)
	{
		return IPCAM_SETUP_RTN_DONE;
	}
	if (runtime[ch].video.bitrate[0].value < min_rate)
	{
		return IPCAM_SETUP_RTN_DONE;
	}

	ntpal = (runtime[ch].video.anti_flicker.value == NF_IPCAM_AF_NTSC) ? 0:1;

	mgr_info.vcodec.resolution[0] = runtime[ch].video.resolution.resolution[0];
	mgr_info.vcodec.resolution[1] = runtime[ch].video.resolution.resolution[1];
	mgr_info.vcodec.vcodec[0] = runtime[ch].video.vcodec[0];
	mgr_info.vcodec.vcodec[1] = runtime[ch].video.vcodec[1];
	mgr_info.vcodec.fps[0] = runtime[ch].video.fps[ntpal][0].value;
	mgr_info.vcodec.fps[1] = runtime[ch].video.fps[ntpal][1].value;
	mgr_info.vcodec.bitrate[0] = min_rate;
	mgr_info.vcodec.bitrate[1] = runtime[ch].video.bitrate[1].value;

	mgr_info.vcodec.bitctrl[0] = runtime[ch].video.bitctrl[0];
	mgr_info.vcodec.bitctrl[1] = runtime[ch].video.bitctrl[1];
	mgr_info.vcodec.capture = runtime[ch].video.capture.value; 

	IPCAM_DBG(MINOR, "Rate down 1(%d) 2(%d)\n", mgr_info.vcodec.bitrate[0], mgr_info.vcodec.bitrate[1]);
#if 0
#if LIMIT_DM368_512K
	mgr_info.vcodec.bitrate[1] = 512;
#else
	mgr_info.vcodec.bitrate[1] = 256;
#endif

	if (runtime[ch].video.fps[ntpal][0].value == NF_IPCAM_FPS_300)
	{
		mgr_info.vcodec.fps[0] = NF_IPCAM_FPS_150;
	}
	if (runtime[ch].video.fps[ntpal][0].value == NF_IPCAM_FPS_250)
	{
		mgr_info.vcodec.fps[0] = NF_IPCAM_FPS_120;
	}
	if (rate <= 1500)
	{
		mgr_info.vcodec.bitrate[0] = 1500;
	}
	else if (rate >= 8000)
	{
		mgr_info.vcodec.bitrate[0] = runtime[ch].video.bitrate[0].value;
	}
#endif

	mgr_info.vcodec.af = ntpal ? 50:60;
	mgr_info.vcodec.mirror = runtime[ch].video.mirror.value;

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_VCODEC, NULL, NULL, &mgr_info);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "fps setup request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			runtime[ch].rate_control = RATE_SUSTENANCE_TIME;
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief 카메라의 bitrate를 runtime값으로 복구시킨다.(TI카메라 전용)
 * @param ch 채널 번호.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *
 * 현재 S1향에서만 사용함.
 * @see nf_ipcam_rate_down
 */
int nf_ipcam_rate_recover(gint ch)
{
	int i;
	int rtn;
	int ntpal;
	cam_info mgr_info;
	mtable* runtime = NULL;
	NFIPCamSetupVCodec info;


	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);
	memset(&mgr_info, 0x00, sizeof(cam_info));
	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL, IPCAM_SETUP_RTN_FAILED);
	if ((runtime[ch].state & MGMT_STATE_CONFIGURED) == 0)
	{
		return IPCAM_SETUP_RTN_FAILED;
	}
	if ( runtime[ch].sys.model_code != NF_IPCAM_MODEL_TI_368)
	{
		return IPCAM_SETUP_RTN_FAILED;
	}

	ntpal = (runtime[ch].video.anti_flicker.value == NF_IPCAM_AF_NTSC) ? 0:1;

	info.ch = ch;
	info.stream_cnt = runtime[ch].video.stream_cnt;
	for (i = 0; i < info.stream_cnt; i++)
	{
		info.resolution[i] = runtime[ch].video.resolution.resolution[i];
		info.vcodec[i] = runtime[ch].video.vcodec[i];
		info.fps[i] = runtime[ch].video.fps[ntpal][i].value;
		info.bitrate[i] = runtime[ch].video.bitrate[i].value;
		info.bitctrl[i] = runtime[ch].video.bitctrl[i];
	}
	info.mirror = runtime[ch].video.mirror.value;
	info.ntsc_pal = ntpal;
	info.capture = runtime[ch].video.capture.value; 

	for (i = 0; i < info.stream_cnt; i++)
	{
		if ((runtime[ch].video.resolution.supported & info.resolution[i]) == 0)
		{
			IPCAM_DBG(MINOR, "resolution mismatch(%08x->%08x)\n",
					info.resolution[i], runtime[ch].video.resolution.resolution[i]);
			info.resolution[i] = runtime[ch].video.resolution.resolution[i];
		}
		mgr_info.vcodec.resolution[i] = info.resolution[i];
		mgr_info.vcodec.vcodec[i] = info.vcodec[i];
		mgr_info.vcodec.fps[i] = info.fps[i];
		if (info.ntsc_pal == 1)
		{
			mgr_info.vcodec.fps[i] = info.fps[i];
			switch(info.fps[i])
			{
				case NF_IPCAM_FPS_300:
					info.fps[i] = NF_IPCAM_FPS_250;
					break;
				case NF_IPCAM_FPS_150:
					info.fps[i] = NF_IPCAM_FPS_120;
					break;
				case NF_IPCAM_FPS_70:
					info.fps[i] = NF_IPCAM_FPS_60;
					break;
				case NF_IPCAM_FPS_30:
				case NF_IPCAM_FPS_20:
				default:
					break;
			}
		}
		mgr_info.vcodec.bitrate[i] = info.bitrate[i];
		mgr_info.vcodec.bitctrl[i] = info.bitctrl[i];
	}
	mgr_info.vcodec.af = ntpal ? 50:60;
	mgr_info.vcodec.mirror = info.mirror;
	mgr_info.vcodec.capture = info.capture;

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_VCODEC, NULL, NULL, &mgr_info);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "vcodec setup request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			runtime[ch].rec_changed = RATE_IGNORE_TIME;
			runtime[ch].rate_control = 0;
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief 수신한 frame건수를 바탕으로 카메라의 bitrate를 조절한다.
 * @param cnt_array 채널별 1초간 수신한 frame수 배열.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *
 * 현재 S1향에서만 사용함.
 */
int nf_ipcam_put_recorded_fcnt(gint* cnt_array)
{
	int i;
	int rtn;
	int ntpal;
	int fps_now;
	mtable *runtime = get_runtime();
	GAsyncQueue *recevt_queue = get_recevt_queue();

	NF_NOTIFY_INFO *analog_rec = NULL;
	gchar *r = NULL;


	g_return_val_if_fail( cnt_array != NULL, IPCAM_SETUP_RTN_FAILED);
	if (nf_get_running_mode()) { return IPCAM_SETUP_RTN_DONE; }

	analog_rec = nf_notify_get("analog_rec");
	if (analog_rec == NULL)
	{
		return IPCAM_SETUP_RTN_DONE;
	}
	r = &analog_rec->c.chmap[0];

	for (i = 0; i < AVAILABLE_MAX_CH; i++)
	{
		if (runtime[i].sys.model_code != NF_IPCAM_MODEL_TI_368)
		{
			continue;
		}
		if (runtime[i].rec_changed > 0)
		{
			continue;
		}

		if (nf_ipcam_using_multi_switch() == 0)
		{
			if (r[i] != 'T')
			{
				continue;
			}
		}

		ntpal = (runtime[i].video.anti_flicker.value == NF_IPCAM_AF_NTSC) ? 0:1;
		fps_now = (runtime[i].video.fps[ntpal][0].value);
		if (runtime[i].rate_control > 0)
		{
			if (fps_now == NF_IPCAM_FPS_300)
			{
				fps_now = NF_IPCAM_FPS_150;
			}
		}

		switch(fps_now)
		{
			case NF_IPCAM_FPS_300:
				if (cnt_array[i] < 21)
				{
					IPX_PND_EVENT *evt = ipx_ipcam_new_event(IPCAM_DS_RATE_DOWN);

					evt->port = i;
					g_async_queue_push(recevt_queue, evt);
				}
				break;
			case NF_IPCAM_FPS_150:
				if (cnt_array[i] < 11)
				{
					IPX_PND_EVENT *evt = ipx_ipcam_new_event(IPCAM_DS_RATE_DOWN);

					evt->port = i;
					g_async_queue_push(recevt_queue, evt);
				}
				break;
			case NF_IPCAM_FPS_70:
				if (cnt_array[i] < 5)
				{
					IPX_PND_EVENT *evt = ipx_ipcam_new_event(IPCAM_DS_RATE_DOWN);

					evt->port = i;
					g_async_queue_push(recevt_queue, evt);
				}
				break;
			default:
				break;
		}
	}

	free(analog_rec);
	analog_rec = NULL;
	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief Model db에 등록된 init 함수를 호출한다.
 * @param ch 채널 번호.
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_func_init
(gint ch, NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	int rtn;
	mtable* runtime = NULL;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL, IPCAM_SETUP_RTN_FAILED);

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_INIT, cb_fxn, user_data, NULL);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "initiate request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
		default:
			break;
	}
	//IPCAM_DBG(MAJOR, "end CH(%d)\n", ch);
	return rtn;
}

/**
 * @brief 카메라 reboot API를 호출한다.
 * @param ch 채널 번호.
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *
 * 현재 NPT카메라만 soft reboot을 실시한다.(외부전원이 연결되어 있어 poe reboot이 불가하므로)
 */
int nf_ipcam_soft_reboot
(gint ch, NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	int rtn;
	mtable* runtime = NULL;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);

	//nmf_mrtp_pipe_close_ch(_h_mrtp_pipe, ch);

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_REBOOT_SOFT, cb_fxn, user_data, NULL);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "reboot request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
		default:
			break;
	}
	//IPCAM_DBG(MAJOR, "end CH(%d)\n", ch);
	return rtn;
}

/**
 * @brief 카메라를 poe reboot시킨다.
 * @param ch 채널 번호.
 * @param cb_fxn 설정 종료후 호출할 콜백함수.(soft reboot시에만 적용됨)
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *
 * 함수명은 POE Reboot이나, soft reboot도 시도한다.
 */
int nf_ipcam_poe_reboot
(gint ch, NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	char key[64];
	int i;
	int rtn;
	unsigned int vl = 0;
	unsigned int *vloss_status = 0;
	mtable *runtime = NULL;
	dtable *discovery = get_dtable();
	GAsyncQueue *vloss_queue = get_vloss_queue();
	pthread_t pid;
	NFIPCamPortStatus port_status;
	int is_nvs_subch = 0;
	GobjMrtpPipe *_h_mrtp_pipe = (GobjMrtpPipe*) nf_live_get_mrtp_pipe_handle();

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL, IPCAM_SETUP_RTN_FAILED);

	if (runtime[ch].sys.type == SYSTEM_DEVICE_NVS && runtime[ch].sys.nvs_sub_ch > 0)
		is_nvs_subch = 1;

	//if (runtime[ch].state & MGMT_STATE_CONFIGURED)
	{
		nmf_mrtp_pipe_close_ch(_h_mrtp_pipe, ch);
		{
			IPX_PND_EVENT *evt = ipx_ipcam_new_event(IPCAM_VLOSS_REMOVE_CH);
			evt->port = ch;
			g_async_queue_push(vloss_queue, evt);
		}

		for (i=0; i<NF_IPCAM_TYPE_MAX; i++)
		{
			nf_ipcam_setup_clear(ch, i);
			pthread_mutex_lock(&runtime[ch].sys.ssl_mtx[i]);
			runtime[ch].sys.ssl_state[i] = IPCAM_SSL_NOT_AVAILABLE;
			pthread_mutex_unlock(&runtime[ch].sys.ssl_mtx[i]);
			_release_resource(NULL, NULL, &runtime[ch].sys.ssl[i], &runtime[ch].sys.ctx[i]);
		}
	}

	if (is_nvs_subch)
	{
		IPCAM_DBG(MAJOR, "nvs subch end CH(%d)\n", ch);
		return IPCAM_SETUP_RTN_DONE;
	}

	if (runtime[ch].funcs[NF_IPCAM_TYPE_REBOOT_SOFT] != NULL)
	{
		IPCAM_DBG(MINOR, "soft reboot(%d)\n", ch);
		rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_REBOOT_SOFT, cb_fxn, user_data, NULL);
	}
	else if (runtime[ch].sys.model_code >= NF_IPCAM_MODEL_ONVIF &&
			runtime[ch].sys.model_code <= NF_IPCAM_MODEL_ONVIF_GRUNDIG)
	{
		IPCAM_DBG(MINOR, "ONVIF reboot(%d)\n", ch);
		nf_onvif_reboot_request(ch);
	}

	int layer, linked = 0;
	ipcam_disc_port_link_state(ch, &layer, &linked);
	if(linked == 1)
	{
		if(layer == IPCAM_DISC_LAYER_DVR)
		{
			snprintf(key, 64, "cam.C%d.poe_on_off", ch);
			if(nf_sysdb_get_bool(key))
			{
				nf_live_poe_port_onoff(ch,0,&rtn,NF_LIVE_PSE_ACT_LOCAL);
				nf_ipcam_set_poe_off_time(ch);
				//sleep(1);
				//nf_live_poe_port_onoff(ch,1,&rtn,NF_LIVE_PSE_ACT_LOCAL);
				IPCAM_DBG(MINOR,"POE Reset ch(%d) layer(%d) poe returns(%d)\n", ch, discovery[ch].layer, rtn);
			}
		}
		else if(discovery[ch].layer == IPCAM_DISC_LAYER_VHUB)
		{
			vhub_set_port_poe_off(ch);
			nf_ipcam_set_poe_off_time(ch);
			//hub_poe_reboot(ch);
			rtn = 0;
			IPCAM_DBG(MINOR,"POE Reset ch(%d) layer(%d) \n", ch, discovery[ch].layer);
		}
		else
		{
			IPCAM_DBG(MINOR, "Unknown layer\n");
		}
	}

	/*
	if (discovery[ch].layer == IPCAM_DISC_LAYER_DVR)
	{
		snprintf(key,64,"cam.C%d.poe_on_off",ch);
		if (nf_sysdb_get_bool(key))
		{
			nf_live_poe_port_onoff(ch,0,&rtn,NF_LIVE_PSE_ACT_LOCAL);
			nf_ipcam_set_poe_off_time(ch);
			//sleep(1);
			//nf_live_poe_port_onoff(ch,1,&rtn,NF_LIVE_PSE_ACT_LOCAL);
			IPCAM_DBG(MINOR, "POE Reset ch(%d) layer(%d) poe returns(%d)\n", ch, discovery[ch].layer, rtn);
		}
	}
	else if (discovery[ch].layer == IPCAM_DISC_LAYER_VHUB)
	{
		vhub_set_port_poe_off(ch);
		nf_ipcam_set_poe_off_time(ch);
		//hub_poe_reboot(ch);
		rtn = 0;
		IPCAM_DBG(MINOR, "POE Reset ch(%d) layer(%d)\n",  ch, discovery[ch].layer);
	}
	else
	{
		IPCAM_DBG(MINOR, "Unknown layer\n");
	}
	*/

	memset(&discovery[ch], 0x00, sizeof(dtable));
	memset(&runtime[ch], 0x00, sizeof(mtable));
	runtime[ch].state = MGMT_STATE_UNLINKED;

	memset(&port_status, 0x00, sizeof(NFIPCamPortStatus));
	nf_ipcam_set_port_status(ch, &port_status, NULL);
	nf_pnd_evt_notify_fire(ch, PND_TYPE_UNPLUGGED, __LINE__, __FILE__);
	nf_ipcam_set_pnd_osd_status(ch, PND_OSD_CONFIG_REBOOT);

	//IPCAM_DBG(MAJOR, "end CH(%d)\n", ch);
	if (rtn == 0)
		return IPCAM_SETUP_RTN_DONE;
	else
		return IPCAM_SETUP_RTN_FAILED;

	return IPCAM_SETUP_RTN_FAILED;	// just to make the compiler happy
}

/**
 * @brief 카메라를 reboot시킨다.
 * @param ch 채널 번호.
 * @param cb_fxn 설정 종료후 호출할 콜백함수.(soft reboot시에만 적용됨)
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_reboot
(gint ch, NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	int rtn;
	mtable *runtime = NULL;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL, IPCAM_SETUP_RTN_FAILED);

	if (runtime[ch].funcs[NF_IPCAM_TYPE_REBOOT_SOFT] != NULL)
	{
		rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_REBOOT_SOFT, cb_fxn, user_data, NULL);
	}
	else
	{
		rtn = nf_ipcam_poe_reboot(ch, cb_fxn, user_data, NULL);
	}

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "reboot request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
		default:
			rtn = IPCAM_SETUP_RTN_DONE;
			break;
	}
	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief 카메라를 공장 초기화 시킨다.(ITX카메라 전용)
 * @param ch 채널 번호.
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *
 * @todo ONVIF카메라 공장초기화 구현.
 */
int nf_ipcam_factory_default
(gint ch, NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	int rtn;
	mtable* runtime = NULL;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL, IPCAM_SETUP_RTN_FAILED);

	runtime[ch].rec_changed = RATE_IGNORE_TIME;
	runtime[ch].rate_control = 0;
	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_FACTORY_DEFAULT, cb_fxn, user_data, NULL);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "factory default request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief 카메라의 오디오 설정을 변경한다.
 * @param ch 채널 번호.
 * @param info 오디오 코덱 설정 정보 struct.
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_set_acodec
(gint ch, NFIPCamSetupACodec* info,
		NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	int i;
	int rtn;
	cam_info mgr_info;
	mtable* runtime = NULL;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	runtime = get_runtime();

	g_return_val_if_fail( info != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL, IPCAM_SETUP_RTN_FAILED);

	IPCAM_DBG(MINOR, "audio tx(%d) rx(%d) audio codec(%d)\n",
			info->audio_tx, info->audio_rx, info->audio_codec);

	memset(&mgr_info, 0x00, sizeof(cam_info));
	mgr_info.acodec.audio_tx = info->audio_tx;
	mgr_info.acodec.audio_rx = info->audio_rx;
	mgr_info.acodec.audio_codec = info->audio_codec;
	mgr_info.acodec.mic_volume = info->mic_volume;
	mgr_info.acodec.speaker_volume = info->speaker_volume;

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_ACODEC, cb_fxn, user_data, &mgr_info);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "acodec setup request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			runtime[ch].audio.audio_tx = info->audio_tx;
			runtime[ch].audio.audio_rx = info->audio_rx;
			runtime[ch].audio.acodec.value = info->audio_codec;
			runtime[ch].audio.mic_volume.value = info->mic_volume;
			runtime[ch].audio.speaker_volume.value = info->speaker_volume;
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief 카메라의 알람 관련(in, out) 설정을 변경한다.
 * @param ch 채널 번호.
 * @param info 알람 설정 정보 struct.
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_set_alarm
(gint ch, NFIPCamSetupAlarm* info,
		NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	int rtn;
	cam_info mgr_info;
	mtable* runtime = NULL;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	runtime = get_runtime();

	g_return_val_if_fail( info != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	//g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);
	if( !(runtime[ch].func & NF_IPCAM_FUNC_ALARM_IN) && !(runtime[ch].func & NF_IPCAM_FUNC_ALARM_OUT))
	{	/* disable annoying log */
		return IPCAM_SETUP_RTN_FAILED;
	}

	IPCAM_DBG(MINOR, "alarm_in(%d) in_type(%d) alarm_out(%d) out_type(%d)\n",
			info->in, info->in_type, info->out, info->out_type);

	memset(&mgr_info, 0x00, sizeof(cam_info));
	mgr_info.alarm.in_onoff = info->in;
	mgr_info.alarm.in_type = info->in_type;
	mgr_info.alarm.out_onoff = info->out;
	mgr_info.alarm.out_type = info->out_type;

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_ALARM, cb_fxn, user_data, &mgr_info);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "alarm setup request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			runtime[ch].alarm.alarm_in = info->in;
			if (runtime[ch].alarm.alarm_in_type.value != 0)
			{
				runtime[ch].alarm.alarm_in_type.value = info->in_type;
			}
			runtime[ch].alarm.alarm_out = info->out;
			// Alarm Out Type Change
			//if (runtime[ch].alarm.alarm_out_type.value != 0)
			{
				runtime[ch].alarm.alarm_out_type.value = info->out_type;
			}
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief 카메라의 알람 in 설정을 변경한다.
 * @param ch 채널 번호.
 * @param al_in 알람 in 사용여부.
 * @param in_type 알람 in의 유형(N/O, N/C).
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_set_alarm_in
(gint ch, gint al_in, gint in_type,
		NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	int rtn = IPCAM_SETUP_RTN_FAILED;
	NFIPCamSetupAlarm al;
	mtable *runtime = get_runtime();

	//IPCAM_DBG(MAJOR, "start CH(%d) al_in(%d)\n", ch, al_in);
	g_return_val_if_fail(runtime != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(al_in == 0 || al_in == 1, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail(in_type == NF_IPCAM_ALARM_TYPE_NO || in_type == NF_IPCAM_ALARM_TYPE_NC, IPCAM_SETUP_RTN_FAILED);

	al.ch = ch;
	al.in = al_in;
	al.in_type = in_type;
	al.out = runtime[ch].alarm.alarm_out;
	al.out_type = runtime[ch].alarm.alarm_out_type.value;
	rtn = nf_ipcam_set_alarm(ch, &al, cb_fxn, user_data, error);

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief 카메라의 알람 out 설정을 변경한다.
 * @param ch 채널 번호.
 * @param onoff 알람 out on/off.
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_set_relay (gint ch, gint onoff/* 0:off, 1:on */,
		NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	int rtn;
	int value;
	char tmp_key[256];
	NFIPCamSetupAlarm al;
	mtable* runtime = NULL;

	//IPCAM_DBG(MAJOR, "start CH(%d) onoff(%d)\n", ch, onoff);

	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	//g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].func & NF_IPCAM_FUNC_ALARM_OUT, IPCAM_SETUP_RTN_FAILED);

	sprintf(tmp_key, "act.arout.R%d.op_type", ch);
	value = nf_sysdb_get_bool(tmp_key);

	memset(&al, 0x00, sizeof(NFIPCamSetupAlarm));
	al.ch = ch;
	al.in = runtime[ch].alarm.alarm_in;
	al.in_type = runtime[ch].alarm.alarm_in_type.value;
	al.out = onoff;
	//al.out_type = runtime[ch].alarm.alarm_out_type.value;
	al.out_type = value;

	rtn = nf_ipcam_set_alarm(ch, &al, cb_fxn, user_data, error);

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief ITX 카메라의 이미지 설정을 변경한다.
 * @param ch 채널 번호.
 * @param info 이미지 설정 struct.
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_set_image(gint ch, NFIPCamSetupImage* info,
		NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	int i = 0;
	int rtn = 0;
	image_info image;
	mtable* runtime = NULL;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	runtime = get_runtime();

	g_return_val_if_fail( info != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_READY, IPCAM_SETUP_RTN_FAILED);

	memset(&image, 0x00, sizeof(image_info));

	for (i = 0; (1<<i) < (1<<NF_IPCAM_IMAGE_EXP_MODE_NR);i++)
	{
		if (info->exposure == 1<<i)
		{
			image.ae = i;
			break;
		}
	}
	for (i = 0; (1<<i) < (1<<NF_IPCAM_IMAGE_SLSH_MODE_NR);i++)
	{
		if (info->slow_shutter == 1<<i)
		{
			image.ss = i;
			break;
		}
	}
	for (i = 0; (1<<i) < (1<<NF_IPCAM_IMAGE_MAGC_MODE_NR);i++)
	{
		if (info->max_agc == 1<<i)
		{
			image.max_agc = i;
			break;
		}
	}
	for (i = 0; (1<<i) < (1<<NF_IPCAM_IMAGE_IRIS_NR); i++)
	{
		if (info->iris == 1<<i)
		{
			image.iris = i;
			break;
		}
	}
	for (i = 0; (1<<i) < (1<<NF_IPCAM_IMAGE_BLC_NR); i++)
	{
		if (info->blc == 1<<i)
		{
			image.blc = i;
			break;
		}
	}
	for (i = 0; (1<<i) < (1<<NF_IPCAM_IMAGE_DN_NR); i++)
	{
		if (info->day_night == 1<<i)
		{
			image.dnn = i;
			if(image.dnn == 13)
			{
				if(get_ircut_dnn_now(ch) == 0)
				{
					if(runtime[ch].image.day_night.support & NF_IPCAM_IMAGE_DN_DAY)
						image.dnn = 1;
					else if(runtime[ch].image.day_night.support & NF_IPCAM_IMAGE_DN_NPT_DAY)
						image.dnn = 8;
					else if(runtime[ch].image.day_night.support & NF_IPCAM_IMAGE_DN_NPT3_DAY)
						image.dnn = 11;
				}
				else
				{
					if(runtime[ch].image.day_night.support & NF_IPCAM_IMAGE_DN_NIGHT)
						image.dnn = 2;
					else if(runtime[ch].image.day_night.support & NF_IPCAM_IMAGE_DN_NPT_NIGHT)
						image.dnn = 9;
					else if(runtime[ch].image.day_night.support & NF_IPCAM_IMAGE_DN_NPT3_NIGHT)
						image.dnn = 12;
				}
			}
			break;
		}
	}
	for (i = 0; (1<<i) < (1<<NF_IPCAM_IMAGE_TGTIME_NR); i++)
	{
		if (info->det_time == 1<<i)
		{
			image.dnn_time = i;
			break;
		}
	}
	for (i = 0; (1<<i) < (1<<NF_IPCAM_IMAGE_WB_MODE_NR); i++)
	{
		if (info->white_balance == 1<<i)
		{
			image.awb = i;
			break;
		}
	}
	for (i = 0; (1<<i) < (1<<NF_IPCAM_IMAGE_MWB_MODE_NR); i++)
	{
		if (info->mwb == 1<<i)
		{
			image.mwb = i;
			break;
		}
	}
	for (i = 0; (1<<i) < (1<<NF_IPCAM_WDR_MODE_ONVIF_NR); i++)
	{
		if (info->wdr == 1<<i)
		{
			image.wd = i;
			break;
		}
	}
	for (i = 0; (1<<i) < (1<<NF_IPCAM_FOCUS_MODE_ONVIF_NR); i++)
	{
		if (info->focus_mode == 1<<i)
		{
			image.focus_mode = i;
			break;
		}
	}
	for (i = 0; (1<<i) < (1<<NF_IPCAM_ANTI_FLICKER_MODE_NR); i++)
	{
		if (info->anti_flicker == 1<<i)
		{
			image.ff_mode = i;
			break;
		}
	}
	for (i = 0; (1<<i) < (1<<NF_IPCAM_MAX_SHUTTER_MODE_PAL_NR); i++)
	{
		if (info->max_shutter == 1<<i)
		{
			image.max_shutter = i;
			break;
		}
	}
	for (i = 0; (1<<i) < (1<<NF_IPCAM_BASE_SHUTTER_MODE_PAL_NR); i++)
	{
		if (info->base_shutter == 1<<i)
		{
			image.base_shutter = i;
			break;
		}
	}
	for(i = 0; (1<<i) < (1<<NF_IPCAM_DNR_MODE_NR); i++)
	{
		if(info->dnr_ctr == 1<<i)
		{
			image.dnr_ctr = i;
			break;
		}
	}

	for(i = 0; (1<<i) < (1<<NF_IPCAM_ADAPTIVE_IR_NR); i++)
	{
		if(info->adaptive_ir == 1<<i)
		{
			image.adaptive_ir = i;
			break;
		}
	}

	for(i = 0; (1<<i) < (1<<NF_IPCAM_DEFOG_NR); i++)
	{
		if(info->defog == 1<<i)
		{
			image.defog = i;
			break;
		}
	}

	for(i = 0; (1<<i) < (1<<NF_IPCAM_HLC_NR); i++)
	{
		if(info->hlc == 1<<i)
		{
			image.hlc = i;
			break;
		}
	}

	for(i = 0; (1<<i) < (1<<NF_IPCAM_FOCUS_LIMIT_NR); i++)
	{
		if(info->focus_limit == 1<<i)
		{
			image.focus_limit = i;
			break;
		}
	}

	for(i = 0; (1<<i) < (1<<NF_IPCAM_STABILIZER_NR); i++)
	{
		if(info->stabilizer == 1<<i)
		{
			image.stabilizer = i;
			break;
		}
	}

	for(i = 0; (1<<i) < (1<<NF_IPCAM_IR_CORRECTION_NR); i++)
	{
		if(info->ir_correction == 1<<i)
		{
			image.ir_correction = i;
			break;
		}
	}



	image.agc = info->agc;
	/* FIXME work around - fix this, dupark */
	if (info->eshutter_speed == 0) { info->eshutter_speed = 200; }
	image.shutter = info->eshutter_speed;
	image.sharpness = info->sharpness;
	image.dnn_sense_ntod = info->dnn_sense_ntod;
	image.dnn_sense_dton = info->dnn_sense_dton;

	IPCAM_DBG(MINOR, "ae(%d) agc(%d) espeed(%d) ss(%d) magc(%d) iris(%d) blc(%d) dnn(%d) dnn_time(%d) \n",
			image.ae, image.agc, image.shutter, image.ss, image.max_agc, image.iris, image.blc, image.dnn, image.dnn_time);

	IPCAM_DBG(MINOR, "awb(%d) mwb(%d) wdr(%d) focus(%d) dnr_ctr(%d) sharpness(%d) brightness(%d) constrast(%d) color(%d) tint(%d) \n",
			image.awb, image.mwb, image.wd, image.focus_mode, image.dnr_ctr, info->sharpness, info->brightness, info->contrast, info->color, info->tint);

	IPCAM_DBG(MINOR, "adaptive_ir(%d) defog(%d) hlc(%d) dnn_dton(%d)  dnn_ntod(%d) anti-flicker(%d) max_shutter(%d) base_shutter(%d)\n",
			image.adaptive_ir, image.defog, image.hlc, image.dnn_sense_dton, image.dnn_sense_ntod, image.ff_mode, image.max_shutter, image.base_shutter);

	IPCAM_DBG(MINOR, "focus_limit(%d), stabilizer(%d), ir_correction(%d)\n",
			image.focus_limit, image.stabilizer, image.ir_correction);

	image.brightness = info->brightness;
	image.contrast = info->contrast;
	image.color = info->color;
	image.tint = info->tint;

	for(i = 0; (1<<i) < (1<<NF_IPCAM_COLORVU_CTRL_NR); i++)
	{
		if(info->colorvu_ctrl == 1<<i)
		{
			image.colorvu_ctrl = i;
			break;
		}
	}
	image.colorvu_level = info->colorvu_level;

	//IPCAM_DBG(MINOR, "changed value considered camera profile\n");
	//IPCAM_DBG(MINOR, "  ==> brightness(%d) contrast(%d) color(%d) tint(%d)\n",
	//		image.brightness, image.contrast, image.color, image.tint, image.sharpness);

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_IMAGE, cb_fxn, user_data, &image);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "image setup request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_PENDING:
		case IPCAM_SETUP_RTN_DONE:
			runtime[ch].image.exposure.value = info->exposure;
			runtime[ch].image.agc.value = image.agc;
			runtime[ch].image.eshutter_speed.value = image.shutter;
			runtime[ch].image.slow_shutter.value = info->slow_shutter;
			runtime[ch].image.max_agc.value = info->max_agc;
			runtime[ch].image.iris.value = info->iris;
			runtime[ch].image.blc.value = info->blc;
			runtime[ch].image.day_night.value = info->day_night;
			runtime[ch].image.tg_time.value = info->det_time;
			runtime[ch].image.wb.value = info->white_balance;
			runtime[ch].image.mwb.value = info->mwb;
			runtime[ch].image.sharpness.value = image.sharpness;
			runtime[ch].image.brightness.value = image.brightness;
			runtime[ch].image.contrast.value = image.contrast;
			runtime[ch].image.color.value = image.color;
			runtime[ch].image.tint.value = image.tint;
			runtime[ch].image.dnr_ctr.value = info->dnr_ctr;
			runtime[ch].image.wd.value = info->wdr;
			runtime[ch].image.focus_mode.value = info->focus_mode;
			runtime[ch].image.anti_flicker.value = info->anti_flicker;
			runtime[ch].image.max_shutter.value = info->max_shutter;
			runtime[ch].image.base_shutter.value = info->base_shutter;
			runtime[ch].image.adaptive_ir.value = info->adaptive_ir;
			runtime[ch].image.defog.value = info->defog;
			runtime[ch].image.hlc.value = info->hlc;
			runtime[ch].image.dnn_schedule.start.hour = info->dnn_start_hour;
			runtime[ch].image.dnn_schedule.start.min = info->dnn_start_min;
			runtime[ch].image.dnn_schedule.end.hour = info->dnn_end_hour;
			runtime[ch].image.dnn_schedule.end.min= info->dnn_end_min;

			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

int nf_ipcam_set_dnn_adjust_d2n(gint ch, NFIPCamSetupImage* info,
		NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	int i = 0;
	int rtn = 0;
	image_info image;
	mtable* runtime = NULL;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	runtime = get_runtime();

	g_return_val_if_fail( info != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_READY, IPCAM_SETUP_RTN_FAILED);

	memset(&image, 0x00, sizeof(image_info));

	image.dnn_sense_dton = info->dnn_sense_dton;

	IPCAM_DBG(MINOR, "DNN Adjust Day to Night : %d\n", image.dnn_sense_dton);

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_ADJUST_D2N, cb_fxn, user_data, &image);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "image setup request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_PENDING:
		case IPCAM_SETUP_RTN_DONE:
			runtime[ch].image.dnn_sense_dton.value = info->dnn_sense_dton;

			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

int nf_ipcam_set_dnn_adjust_n2d(gint ch, NFIPCamSetupImage* info,
		NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	int i = 0;
	int rtn = 0;
	image_info image;
	mtable* runtime = NULL;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	runtime = get_runtime();

	g_return_val_if_fail( info != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_READY, IPCAM_SETUP_RTN_FAILED);

	memset(&image, 0x00, sizeof(image_info));

	image.dnn_sense_ntod = info->dnn_sense_ntod;

	IPCAM_DBG(MINOR, "DNN Adjust Night to Day: %d\n", image.dnn_sense_ntod);

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_ADJUST_N2D, cb_fxn, user_data, &image);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "image setup request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_PENDING:
		case IPCAM_SETUP_RTN_DONE:
			runtime[ch].image.dnn_sense_ntod.value = info->dnn_sense_ntod;

			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief ITX 카메라의 색상 설정을 변경한다.
 * @param ch 채널 번호.
 * @param info 색상 설정 struct.
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *
 * @deprecated 구 UI(2차 이전)에 있던 기능.
 */
int nf_ipcam_set_color(gint ch, NFIPCamSetupColor* info,
		NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	int i;
	int rtn;
	//cam_info mgr_info;
	NFIPCamSetupImage image;
	mtable* runtime = NULL;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	runtime = get_runtime();

	g_return_val_if_fail( info != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);

	IPCAM_DBG(MINOR, "brightness(%d) contrast(%d) color(%d) tint(%d)\n",
			info->brightness, info->contrast, info->color, info->tint);

	//memset(&mgr_info, 0x00, sizeof(cam_info));
	memset(&image, 0x00, sizeof(NFIPCamSetupImage));

	image.exposure = runtime[ch].image.exposure.value;
	image.agc = runtime[ch].image.agc.value;
	image.eshutter_speed = runtime[ch].image.eshutter_speed.value;
	image.slow_shutter = runtime[ch].image.slow_shutter.value;
	image.max_agc = runtime[ch].image.max_agc.value;
	image.iris = runtime[ch].image.iris.value;
	image.blc = runtime[ch].image.blc.value;
	image.day_night = runtime[ch].image.day_night.value;
	image.det_time = runtime[ch].image.tg_time.value;
	image.white_balance = runtime[ch].image.wb.value;
	image.mwb = runtime[ch].image.mwb.value;
	image.dnr_ctr = runtime[ch].image.dnr_ctr.value;

	image.sharpness = runtime[ch].image.sharpness.value;
	image.brightness = info->brightness;
	image.contrast = info->contrast;
	image.color = info->color;
	image.tint = info->tint;

	//ColorVU Level
	if(runtime[ch].image.supported & NF_IPCAM_IMAGE_COLORVU){
		image.colorvu_level = runtime[ch].image.colorvu_level.value;
		//image.colorvu_ctrl = runtime[ch].image.colorvu_ctrl.value;
	}

	rtn = nf_ipcam_set_image(ch, &image, cb_fxn, user_data, error);

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief 카메라의 이미지, exposure 설정을 변경한다.(ITX, ONVIF카메라 공용)
 * @param ch 채널 번호.
 * @param info 이미지 설정 struct.
 * @param info2 Exposure설정 struct.
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_set_image_exp_onvif(gint ch, NFIPCamSetupImage_onvif* info, NFIPCamSetupExposure_onvif* info2,
		NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	int rtn = 0;
	int i = 0;
	mtable* runtime = NULL;

	runtime = get_runtime();

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	g_return_val_if_fail( info != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	if(!( runtime[ch].state & MGMT_STATE_READY))
	{
		return IPCAM_SETUP_RTN_FAILED;
	}

	if(nf_ipcam_is_onvif_support(ch) != 1)
	{
		// make NFIPCamSetupImage and call nf_ipcam_Set_image
		NFIPCamSetupImage image;

		memset(&image, 0x00, sizeof(NFIPCamSetupImage));

		switch(info2->mode)
		{
			case NF_IPCAM_EXPOSURE_MODE_ITX_MANUAL:
				image.white_balance = NF_IPCAM_IMAGE_EXP_MODE_MANUAL;
				break;

			case NF_IPCAM_EXPOSURE_MODE_ITX_AUTO_INDOOR:
				image.white_balance = NF_IPCAM_IMAGE_EXP_MODE_AUTO_INDOOR;
				break;

			case NF_IPCAM_EXPOSURE_MODE_ITX_AUTO_OUTDOOR:
				image.white_balance = NF_IPCAM_IMAGE_EXP_MODE_AUTO_OUTDOOR;
				break;

			case NF_IPCAM_EXPOSURE_MODE_ITX_AUTO:
				image.white_balance = NF_IPCAM_IMAGE_EXP_MODE_AUTO;
				break;

			case NF_IPCAM_EXPOSURE_MODE_ITX_AUTO_I3:
				image.white_balance = NF_IPCAM_IMAGE_EXP_MODE_AUTO_I3;
				break;

			case NF_IPCAM_EXPOSURE_MODE_ITX_MANUAL_I3:
				image.white_balance = NF_IPCAM_IMAGE_EXP_MODE_MANUAL_I3;
				break;

			case NF_IPCAM_EXPOSURE_MODE_ITX_AUTO_INX:
				image.white_balance = NF_IPCAM_IMAGE_EXP_MODE_AUTO_INX;
				break;

			case NF_IPCAM_EXPOSURE_MODE_ITX_MANUAL_INX:
				image.white_balance = NF_IPCAM_IMAGE_EXP_MODE_MANUAL_INX;
				break;
		}

		switch(info2->blc_mode)
		{
			case NF_IPCAM_BLC_MODE_ONVIF_ON:
				image.blc = NF_IPCAM_IMAGE_BLC_ON;
				break;

			case NF_IPCAM_BLC_MODE_ONVIF_OFF:
				image.blc = NF_IPCAM_IMAGE_BLC_OFF;
				break;
		}

		image.agc = info2->gain;
		image.eshutter_speed = info2->etime;
		image.slow_shutter = info2->slow_shutter;
		image.max_agc = info2->max_agc;
		image.iris = info2->iris_control;

		switch(info2->ircut)
		{
			case NF_IPCAM_IRCUT_MODE_ITX_AUTO:
				image.day_night = NF_IPCAM_IMAGE_DN_AUTO;
				break;

			case NF_IPCAM_IRCUT_MODE_ITX_DAYTIME:
				image.day_night = NF_IPCAM_IMAGE_DN_DAY;
				break;

			case NF_IPCAM_IRCUT_MODE_ITX_NIGHTTIME:
				image.day_night = NF_IPCAM_IMAGE_DN_NIGHT;
				break;

			case NF_IPCAM_IRCUT_MODE_NPT_AUTO:
				image.day_night = NF_IPCAM_IMAGE_DN_NPT_AUTO;
				break;

			case NF_IPCAM_IRCUT_MODE_NPT_DAYTIME:
				image.day_night = NF_IPCAM_IMAGE_DN_NPT_DAY;
				break;

			case NF_IPCAM_IRCUT_MODE_NPT_NIGHTTIME:
				image.day_night = NF_IPCAM_IMAGE_DN_NPT_NIGHT;
				break;
		}

		image.det_time = info->det_time;

		switch(info->white_balance)
		{
			case NF_IPCAM_WB_MODE_ONVIF_AUTO:
				image.white_balance = NF_IPCAM_IMAGE_WB_MODE_AUTO;
				break;

			case NF_IPCAM_WB_MODE_ITX_AUTO_W:
				image.white_balance = NF_IPCAM_IMAGE_WB_MODE_AUTO_W;
				break;

			case NF_IPCAM_WB_MODE_ITX_MANUAL:
				image.white_balance = NF_IPCAM_IMAGE_WB_MODE_MANUAL;
				break;
		}

		image.mwb = info->mwb;
		image.wdr = info2->wide_dynamic_mode;

		image.sharpness = info->sharpness;

		// scale change

		image.brightness = info->brightness;
		image.contrast = info->contrast;
		image.color = info->color;
		image.tint = info->tint;

		rtn = nf_ipcam_set_image(ch, &image, cb_fxn, user_data, error);
	}
	else
	{
		image_info_onvif image;

		memset(&image, 0x00, sizeof(image_info_onvif));

		IPCAM_DBG(MINOR, "mode(%d) priority(%d) minetime(%d) maxetime(%d) etime(%d) mingain(%d) maxgain(%d) gain(%d) miniris(%d) maxiris(%d) iris(%d) blc(%d) blclevel(%d)\n",
				info2->mode, info2->priority, info2->minetime, info2->maxetime, info2->etime,
				info2->mingain, info2->maxgain, info2->gain, info2->miniris, info2->maxiris, info2->iris,
				info2->blc_mode, info2->blc_level);


		image.brightness = info->brightness;
		image.contrast = info->contrast;
		image.color = info->color;
		image.sharpness = info->sharpness;

		image.focus.mode = info->focus_mode;
		image.focus.defaultspeed = info->default_speed;
		image.focus.nearlimit = info->near_limit;
		image.focus.farlimit = info->far_limit;

		image.ircut = info2->ircut;
		image.wdrmode = info2->wide_dynamic_mode;
		image.wdrlevel = info2->wide_level;

		image.wb.mode = info->white_balance;
		image.wb.cbgain = info->cbgain;
		image.wb.crgain = info->crgain;

		image.exposure.mode = info2->mode;
		image.exposure.priority = info2->priority;
		image.exposure.maxetime = info2->maxetime;
		image.exposure.minetime = info2->minetime;
		image.exposure.etime = info2->etime;
		image.exposure.maxgain = info2->maxgain;
		image.exposure.mingain = info2->mingain;
		image.exposure.gain = info2->gain;
		image.exposure.maxiris = info2->maxiris;
		image.exposure.miniris = info2->miniris;
		image.exposure.iris = info2->iris;

		image.exposure.iris_mode = info2->iris_control;	//axis_add
		image.blcmode = info2->blc_mode;
		image.blclevel = info2->blc_level;

		IPCAM_DBG(MINOR, "brightness(%d) contrast(%d) color(%d) sharpness(%d)\n",
				image.brightness, image.contrast, image.color, image.sharpness);

		rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_IMAGE_ONVIF, cb_fxn, user_data, &image);

		switch(rtn)
		{
			case IPCAM_SETUP_RTN_FAILED:
				IPCAM_DBG(WARN, "image setup request failed ch(%d)\n", ch);
				break;
			case IPCAM_SETUP_RTN_PENDING:
			case IPCAM_SETUP_RTN_DONE:
				runtime[ch].image_onvif.sharpness.value = image.sharpness;
				runtime[ch].image_onvif.brightness.value = image.brightness;
				runtime[ch].image_onvif.contrast.value = image.contrast;
				runtime[ch].image_onvif.color.value = image.color;
				runtime[ch].image_onvif.focus.mode.value = image.focus.mode;
				runtime[ch].image_onvif.focus.defaultspeed.value = image.focus.defaultspeed;
				runtime[ch].image_onvif.focus.nearlimit.value = image.focus.nearlimit;
				runtime[ch].image_onvif.focus.farlimit.value = image.focus.farlimit;
				runtime[ch].image_onvif.ircut.value = image.ircut;
				runtime[ch].image_onvif.wdrmode.value = image.wdrmode;
				runtime[ch].image_onvif.wdrlevel.value = image.wdrlevel;
				runtime[ch].image_onvif.wb.mode.value = image.wb.mode;
				runtime[ch].image_onvif.wb.cbgain.value = image.wb.cbgain;
				runtime[ch].image_onvif.wb.crgain.value = image.wb.crgain;

				runtime[ch].image_onvif.exposure.mode.value = image.exposure.mode;
				runtime[ch].image_onvif.exposure.priority.value = image.exposure.priority;
				runtime[ch].image_onvif.exposure.maxetime.value = image.exposure.maxetime;
				runtime[ch].image_onvif.exposure.minetime.value = image.exposure.minetime;
				runtime[ch].image_onvif.exposure.etime.value = image.exposure.etime;
				runtime[ch].image_onvif.exposure.maxgain.value = image.exposure.maxgain;
				runtime[ch].image_onvif.exposure.mingain.value = image.exposure.mingain;
				runtime[ch].image_onvif.exposure.gain.value = image.exposure.gain;
				runtime[ch].image_onvif.exposure.maxiris.value = image.exposure.maxiris;
				runtime[ch].image_onvif.exposure.miniris.value = image.exposure.miniris;
				runtime[ch].image_onvif.exposure.iris.value = image.exposure.iris;
				runtime[ch].image_onvif.exposure.iris_mode.value = image.exposure.iris_mode;

				runtime[ch].image_onvif.blcmode.value = image.blcmode;
				runtime[ch].image_onvif.blclevel.value = image.blclevel;

				break;
			default:
				rtn = IPCAM_SETUP_RTN_FAILED;
				break;
		}
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief ONVIF 카메라의 focus 설정을 변경한다.
 * @param ch 채널 번호.
 * @param info Focus 설정 struct.
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_set_focus_onvif(gint ch, NFIPCamSetupFocus_onvif* info,
		NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	int i = 0;
	int rtn = 0;
	focus_move_onvif image;
	mtable* runtime = NULL;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	runtime = get_runtime();

	g_return_val_if_fail( info != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	if(!( runtime[ch].state & MGMT_STATE_READY))
	{
		return IPCAM_SETUP_RTN_FAILED;
	}

	memset(&image, 0x00, sizeof(focus_move_onvif));

	image.mode = info->mode;
	image.position = info->position;
	image.distance = info->distance;
	image.speed = info->speed;

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_FOCUS_ONVIF, cb_fxn, user_data, &image);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "focus move request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_PENDING:
		case IPCAM_SETUP_RTN_DONE:
			runtime[ch].image_onvif.focus.mode.value = image.mode;
			switch(image.mode)
			{
				case NF_IPCAM_FOCUS_MODE_ONVIF_ABSOLUTE:
					runtime[ch].image_onvif.move.abposition.value = image.position;
					runtime[ch].image_onvif.move.abspeed.value = image.speed;
					break;
				case NF_IPCAM_FOCUS_MODE_ONVIF_RELATIVE:
					runtime[ch].image_onvif.move.redistance.value = image.distance;
					runtime[ch].image_onvif.move.respeed.value = image.speed;
					break;
				case NF_IPCAM_FOCUS_MODE_ONVIF_CONTINUOUS:
					runtime[ch].image_onvif.move.cospeed.value = image.speed;
					break;
			}

			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief ONVIF 카메라의 PTZ 설정을 변경한다.
 * @param ch 채널 번호.
 * @param info PTZ 설정 struct.
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_set_ptz_onvif(gint ch, NFIPCamSetupPTZ_onvif* info,
		NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	int i = 0;
	int rtn = 0;
	ptz_info_onvif ptz;
	mtable* runtime = NULL;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	runtime = get_runtime();

	g_return_val_if_fail( info != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	if(!( runtime[ch].state & MGMT_STATE_READY))
	{
		return IPCAM_SETUP_RTN_FAILED;
	}

	memset(&ptz, 0x00, sizeof(ptz_info_onvif));

	ptz.mode = info->mode;
	switch(ptz.mode)
	{
		case NF_IPCAM_PTZ_MODE_ONVIF_ABSOLUTE:
			ptz.absolute_pan = info->absolute_pan;
			ptz.absolute_tilt = info->absolute_tilt;
			ptz.absolute_zoom = info->absolute_zoom;
			ptz.speed_pan = info->speed_pan;
			ptz.speed_tilt = info->speed_tilt;
			ptz.speed_zoom = info->speed_zoom;
			break;

		case NF_IPCAM_PTZ_MODE_ONVIF_RELATIVE:
			ptz.relative_pan = info->relative_pan;
			ptz.relative_tilt = info->relative_tilt;
			ptz.relative_zoom = info->relative_zoom;
			ptz.speed_pan = info->speed_pan;
			ptz.speed_tilt = info->speed_tilt;
			ptz.speed_zoom = info->speed_zoom;
			break;

		case NF_IPCAM_PTZ_MODE_ONVIF_CONTINUOUS:
			ptz.speed_pan = info->speed_pan;
			ptz.speed_tilt = info->speed_tilt;
			ptz.speed_zoom = info->speed_zoom;
			break;
	}

	ptz.ePTZAreaId = info->ePTZAreaId;

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_PAN_TILT, cb_fxn, user_data, &ptz);

	switch(rtn)
	{
	case IPCAM_SETUP_RTN_FAILED:
		IPCAM_DBG(WARN, "ptz move request failed ch(%d)\n", ch);
		break;
	case IPCAM_SETUP_RTN_PENDING:
	case IPCAM_SETUP_RTN_DONE:

		break;
	default:
		rtn = IPCAM_SETUP_RTN_FAILED;
		break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief 카메라의 PTZ 동작을 멈춘다.
 * @param ch 채널 번호.
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_set_ptz_stop(gint ch, NF_PTZ_CMD *cmd, 
		NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	gint rtn = IPCAM_SETUP_RTN_FAILED;
	mtable *runtime = NULL;


	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);
	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL, IPCAM_SETUP_RTN_FAILED);
	if(!( runtime[ch].state & MGMT_STATE_CONFIGURED))
	{
		return IPCAM_SETUP_RTN_FAILED;
	}
	//g_return_val_if_fail( runtime[ch].func & NF_IPCAM_FUNC_PTZ, IPCAM_SETUP_RTN_FAILED);

	//IPCAM_DBG(MINOR, "CH(%d) PTZ stop\n", ch);

	//rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_STOP, cb_fxn, user_data, (void*)&runtime[ch].ptz.moving);
	

	if(nf_ipcam_is_onvif_support(ch) != 1)
	{
	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_STOP, cb_fxn, user_data, NULL);
	}
	else
	{
		ptz_info_onvif ptz;
		memset(&ptz, 0x00, sizeof(ptz));

        if(cmd) ptz.ePTZAreaId = cmd->params[0];

		rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_STOP, cb_fxn, user_data, &ptz);
	}

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, " PTZ stop operation request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief 카메라의 Zoom 동작을 멈춘다.
 * @param ch 채널 번호.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_set_zoom_stop(gint ch)
{
	gint rtn = IPCAM_SETUP_RTN_FAILED;
	mtable *runtime = NULL;

	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL, IPCAM_SETUP_RTN_FAILED);

	if(!( runtime[ch].state & MGMT_STATE_CONFIGURED))
	{
		return IPCAM_SETUP_RTN_FAILED;
	}

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_ZOOM_STOP, NULL, NULL, NULL);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, " PTZ stop operation request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	return rtn;
}

/**
 * @brief 카메라의 PTZ 동작을 지시한다.
 * @param ch 채널 번호.
 * @param cmd PTZ명령 enum.
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *
 * @todo PTZ speed를 외부 파라메터로 변경할 것.
 */
int nf_ipcam_set_pt_move(gint ch, NF_PTZ_CMD* cmd,
		NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	gint rtn = IPCAM_SETUP_RTN_FAILED;
	mtable *runtime = NULL;
	gchar *PTZ_CMD_STR[] = {
		"PAN LEFT",
		"PAN RIGHT",
		"TILT UP",
		"TILT DOWN",
		"ZOOM WIDE",
		"ZOOM TELE",
		"PT LEFT UP",
		"PT LEFT DOWN",
		"PT RIGHT UP",
		"PT RIGHT DOWN",
		"IRIS OPEN",
		"IRIS CLOSE",
		"FOCUS NEAR",
		"FOCUS FAR",
		"PTZ STOP"
	};

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);
	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);

	if(nf_ipcam_is_onvif_support(ch) != 1)
	{
		ptz_info info;
		memset(&info, 0x00, sizeof(ptz_info));

		info.cmd = cmd->cmd;
		if(cmd->params[0] == 0)
		{
			PtzData ptzProp;
			/* assume speed range between 1 ~ 10. @see MULTIPLIER_0_TO_1 */
			if(nf_ipcam_get_ptz_property(ch, &ptzProp) != 0)
			{
				info.pt_speed = 5;
			}
			else
			{
				info.pt_speed = ptzProp.PTSpeed + 1;
				info.zoom_speed = ptzProp.zoomSpeed + 1;
				info.iris_speed = ptzProp.irisSpeed + 1;
				info.focus_speed = ptzProp.focusSpeed + 1;
			}
		}
		else
		{
			info.pt_speed = cmd->params[0]/10;
		}
		g_return_val_if_fail( runtime[ch].func & NF_IPCAM_FUNC_PTZ, IPCAM_SETUP_RTN_FAILED);
		switch(cmd->cmd)
		{
			case NF_PTZ_CMD_PAN_LEFT:
			case NF_PTZ_CMD_PAN_RIGHT:
			case NF_PTZ_CMD_TILT_UP:
			case NF_PTZ_CMD_TILT_DOWN:
			case NF_PTZ_CMD_PT_LEFTUP:
			case NF_PTZ_CMD_PT_LEFTDOWN:
			case NF_PTZ_CMD_PT_RIGHTUP:
			case NF_PTZ_CMD_PT_RIGHTDOWN:
				break;
			default:
			{
				IPCAM_DBG(MINOR, "Non-PT command\n");
				return IPCAM_SETUP_RTN_FAILED;
			}
		}

		IPCAM_DBG(MINOR, "CH(%d) Pan/Tilt command(%s)\n", ch, PTZ_CMD_STR[cmd->cmd]);

		rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_PAN_TILT, cb_fxn, user_data, (void*)&info);
	}
	else
	{
		NFIPCamSetupPTZ_onvif info;
		int speed;

		memset(&info, 0x00, sizeof(NFIPCamSetupPTZ_onvif));
		info.ch = ch;

		if(cmd->params[0] == 0)
		{
			PtzData ptzProp;
			/* assume speed range between 1 ~ 10. @see MULTIPLIER_0_TO_1 */
			if(nf_ipcam_get_ptz_property(ch, &ptzProp) != 0)
			{
				speed = 5;
			}
			else
			{
				speed = ptzProp.PTSpeed + 1;
			}
		}
		else
		{
			speed = cmd->params[0]/10;
		}

#if 1
		info.mode = NF_IPCAM_PTZ_MODE_ONVIF_CONTINUOUS;

		switch(cmd->cmd)
		{
			case NF_PTZ_CMD_PAN_LEFT:
				info.speed_pan = (-1) * speed;
				break;
			case NF_PTZ_CMD_PAN_RIGHT:
				info.speed_pan = (1) * speed;
				break;
			case NF_PTZ_CMD_TILT_UP:
				info.speed_tilt = (-1) * speed;
				break;
			case NF_PTZ_CMD_TILT_DOWN:
				info.speed_tilt = (1) * speed;
				break;
			case NF_PTZ_CMD_PT_LEFTUP:
				info.speed_pan = (-1) * speed;
				info.speed_tilt = (-1) * speed;
				break;
			case NF_PTZ_CMD_PT_LEFTDOWN:
				info.speed_pan = (-1) * speed;
				info.speed_tilt = (1) * speed;
				break;
			case NF_PTZ_CMD_PT_RIGHTUP:
				info.speed_pan = (1) * speed;
				info.speed_tilt = (-1) * speed;
				break;
			case NF_PTZ_CMD_PT_RIGHTDOWN:
				info.speed_pan = (1) * speed;
				info.speed_tilt = (1) * speed;
				break;
		}
#else
		info.mode = NF_IPCAM_PTZ_MODE_ONVIF_RELATIVE;

		switch(cmd->cmd)
		{
			case NF_PTZ_CMD_PAN_LEFT:
				info.relative_pan = (-1) * speed;
				break;
			case NF_PTZ_CMD_PAN_RIGHT:
				info.relative_pan = (1) * speed;
				break;
			case NF_PTZ_CMD_TILT_UP:
				info.relative_tilt = (-1) * speed;
				break;
			case NF_PTZ_CMD_TILT_DOWN:
				info.relative_tilt = (1) * speed;
				break;
			case NF_PTZ_CMD_PT_LEFTUP:
				info.relative_pan = (-1) * speed;
				info.relative_tilt = (-1) * speed;
				break;
			case NF_PTZ_CMD_PT_LEFTDOWN:
				info.relative_pan = (-1) * speed;
				info.relative_tilt = (1) * speed;
				break;
			case NF_PTZ_CMD_PT_RIGHTUP:
				info.relative_pan = (1) * speed;
				info.relative_tilt = (-1) * speed;
				break;
			case NF_PTZ_CMD_PT_RIGHTDOWN:
				info.relative_pan = (1) * speed;
				info.relative_tilt = (1) * speed;
				break;
		}

		info.speed_pan = speed;
		info.speed_tilt = speed;
#endif

		if(cmd) info.ePTZAreaId = cmd->params[1];

		rtn = nf_ipcam_set_ptz_onvif(ch, &info, NULL, NULL, NULL);
	}

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "pan/tilt operation request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief 카메라의 zoom 위치를 변경한다.
 * @param ch 채널 번호.
 * @param value Zoom위치값.
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *
 * @deprecated Absolute zoom기능은 현재 지원안함.
 */
int nf_ipcam_set_zoom(gint ch, gint value,
		NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	gint rtn = IPCAM_SETUP_RTN_FAILED;
	gint zoom_min = 0;
	gint zoom_max = 0;
	gint zoom_delta = 0;
	gint set_val = 0;
	mtable* runtime = NULL;


	//IPCAM_DBG(MAJOR, "start CH(%d) value(%d)\n", ch, value);
	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].image.supported & NF_IPCAM_IMAGE_ZOOM, IPCAM_SETUP_RTN_FAILED);

	zoom_min = runtime[ch].ptz.zoom.min;
	zoom_max = runtime[ch].ptz.zoom.max;
	zoom_delta = zoom_max - zoom_min;
	set_val = ((value*zoom_delta)/100) + zoom_min;

	//IPCAM_DBG(MINOR, "changed value considered camera profile\n");
	//IPCAM_DBG(MINOR, " ==> zoom(%d)\n", set_val);

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_ZOOM, cb_fxn, user_data, &set_val);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "zoom setup request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			runtime[ch].ptz.zoom.value = (guint)set_val;
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief 카메라의 포커스 보정 기능을 변경한다.
 * @param ch 채널 번호.
 * @param info 포커스 설정 struct.
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_set_focus_compensation(gint ch, NFIPCamSetupFocusComp* info,
		NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	int i = 0;
	int rtn = 0;
	focus_comp_info fc_info;
	mtable* runtime = NULL;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	runtime = get_runtime();

	g_return_val_if_fail( info != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_READY, IPCAM_SETUP_RTN_FAILED);

	memset(&fc_info, 0x00, sizeof(focus_comp_info));

	int major = 0, type = 0, subtype = 0, minor = 0; // Camera SDK Version
	sscanf(runtime[ch].sys.sdkver, "%d.%d.%d.%d", &major, &type, &subtype, &minor);

	fc_info.tem_comp_mode = info->tem_comp_mode;
	fc_info.dnn_comp_mode = info->dnn_comp_mode;

	int maskarea = 0;
	int areatx = 0;
	int areaty = 0;
	int areabx = 0;
	int areaby = 0;

	if(type == ITX_CAM_SDK_TYPE_HS) {
		maskarea = 0;
		areatx =  -1;
		areaty = -1;
		areabx = -1;
		areaby = -1;
	} else {
		maskarea = runtime[ch].focus.maskarea[0].value;
		areatx = runtime[ch].focus.areatx[0].value;
		areaty = runtime[ch].focus.areaty[0].value;
		areabx = runtime[ch].focus.areabx[0].value;
		areaby = runtime[ch].focus.areaby[0].value;
	}

	fc_info.maskarea[0] = maskarea;
	fc_info.areatx[0] = areatx;
	fc_info.areaty[0] = areaty;
	fc_info.areabx[0] = areabx;
	fc_info.areaby[0] = areaby;

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_FOCUS_COMP, cb_fxn, user_data, &fc_info);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "image setup request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_PENDING:
		case IPCAM_SETUP_RTN_DONE:
			runtime[ch].focus.tem_comp.value = info->tem_comp_mode;
			runtime[ch].focus.dnn_comp.value = info->dnn_comp_mode;
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

#if 0
int nf_ipcam_set_zoom_tele(gint ch) // near
{
	gint rtn = IPCAM_SETUP_RTN_FAILED;
	gint zoom_min = 0;
	gint zoom_max = 0;
	gint zoom_delta = 0;
	gint set_val = 0;
	gint value = 0;
	mtable* runtime = NULL;


	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].img.supported & NF_IPCAM_IMAGE_ZOOM, IPCAM_SETUP_RTN_FAILED);

	IPCAM_DBG("[%s] ch(%d)\n", __FUNCTION__, ch);

	zoom_min = runtime[ch].zoom.min;
	zoom_max = runtime[ch].zoom.max;
	zoom_delta = zoom_max - zoom_min;
	g_return_val_if_fail( zoom_delta != 0, IPCAM_SETUP_RTN_FAILED);

	value = (runtime[ch].zoom.value - zoom_min) * 100 / zoom_delta;
	if (value < 90) { value += 10; }
	else { value = 100; }

	set_val = ((value*zoom_delta)/100) + zoom_min;

	IPCAM_DBG("[%s] changed value considered camera profile\n", __FUNCTION__);
	IPCAM_DBG("[%s]  ==> zoom(%d:%d)\n", __FUNCTION__, value, set_val);

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_ZOOM, NULL, NULL, &set_val);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			g_warning("[%s] zoom setup request failed ch(%d)\n", __FUNCTION__, ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			runtime[ch].zoom.value = (guint)set_val;
			break;
		default:
			break;
	}

	return rtn;
}

int nf_ipcam_set_zoom_wide(gint ch)
{
	gint rtn = IPCAM_SETUP_RTN_FAILED;
	gint zoom_min = 0;
	gint zoom_max = 0;
	gint zoom_delta = 0;
	gint set_val = 0;
	gint value = 0;
	mtable* runtime = NULL;


	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].img.supported & NF_IPCAM_IMAGE_ZOOM, IPCAM_SETUP_RTN_FAILED);

	IPCAM_DBG("[%s] ch(%d)\n", __FUNCTION__, ch);

	zoom_min = runtime[ch].zoom.min;
	zoom_max = runtime[ch].zoom.max;
	zoom_delta = zoom_max - zoom_min;
	g_return_val_if_fail( zoom_delta != 0, IPCAM_SETUP_RTN_FAILED);

	value = (runtime[ch].zoom.value - zoom_min) * 100 / zoom_delta;
	if (value > 10) { value -= 10; }
	else { value = 0; }

	set_val = ((value*zoom_delta)/100) + zoom_min;

	IPCAM_DBG("[%s] changed value considered camera profile\n", __FUNCTION__);
	IPCAM_DBG("[%s]  ==> zoom(%d:%d)\n", __FUNCTION__, value, set_val);

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_ZOOM, NULL, NULL, &set_val);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			g_warning("[%s] zoom setup request failed ch(%d)\n", __FUNCTION__, ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			runtime[ch].zoom.value = (guint)set_val;
			break;
		default:
			break;
	}

	return rtn;
}
#else

/**
 * @brief Zoom을 확대한다.
 * @param ch 채널 번호.
 * @param speed 속도.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *
 * Speed가 정상적인 값이 넘어오지 않아 내부 API에서 speed 계산함.
 */
int nf_ipcam_set_zoom_tele(gint ch, NF_PTZ_CMD* cmd, NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	gint rtn = IPCAM_SETUP_RTN_FAILED;
	gint zoom_min = 0;
	gint zoom_max = 0;
	gint zoom_delta = 0;
	gint value = 0;
	gint speed = 0;
	mtable* runtime = NULL;

	//IPCAM_DBG(MAJOR, "start CH(%d) speed(%d)\n", ch, speed);
	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);

	if(cmd) speed = cmd->params[0];

	if(speed == 0)
	{
		PtzData ptzProp;
		if(nf_ipcam_get_ptz_property(ch, &ptzProp) != 0)
			speed = 5;
		else
			speed = ptzProp.zoomSpeed + 1;
	}
	else
	{
		speed = speed / 10;
	}

	if(nf_ipcam_is_onvif_support(ch) != 1)
	{
		ptz_info info;
		memset(&info, 0x00, sizeof(ptz_info));
		if (runtime[ch].ptz.supported & PTZ_SETUP_ZOOM)
		{
			info.cmd = NF_PTZ_CMD_ZOOM_TELE;
			info.zoom_speed = speed;
			IPCAM_DBG(MINOR, "zoom tele PTZ\n");
		}
		else	if (runtime[ch].sys.stdver[0] != '\0'
				&& runtime[ch].sys.model_code != NF_IPCAM_MODEL_AMB_A2
				&& runtime[ch].sys.model_code != NF_IPCAM_MODEL_AMB_D1)
		{
			info.cmd = NF_PTZ_CMD_ZOOM_TELE;
			info.zoom_speed = speed;
			IPCAM_DBG(MINOR, "zoom tele Non-PTZ\n");
		}
		else
		{
			zoom_max = runtime[ch].ptz.zoom.max;
			value = runtime[ch].ptz.zoom.value + speed * 10;
			info.cmd = value;
			if (value > zoom_max) { value = zoom_max; }

			IPCAM_DBG(MINOR, "zoom(%d->%d)\n", runtime[ch].ptz.zoom.value, value);
		}

		rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_ZOOM, NULL, NULL, &info);
	}
	else
	{
		NFIPCamSetupPTZ_onvif info;
		memset(&info, 0x00, sizeof(NFIPCamSetupPTZ_onvif));
		info.mode = NF_IPCAM_PTZ_MODE_ONVIF_CONTINUOUS;
		info.speed_zoom = (1) * speed;

		//info.mode = NF_IPCAM_PTZ_MODE_ONVIF_RELATIVE;
		//info.relative_zoom = (1) * speed;
		//info.speed_zoom = speed;

		if(cmd) info.ePTZAreaId = cmd->params[1];

		rtn = nf_ipcam_set_ptz_onvif(ch, &info, NULL, NULL, NULL);
	}

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "zoom setup request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			runtime[ch].ptz.zoom.value = (guint)value;
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief Zoom을 축소한다.
 * @param ch 채널 번호.
 * @param speed 속도.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *
 * Speed가 정상적인 값이 넘어오지 않아 내부 API에서 speed 계산함.
 */
int nf_ipcam_set_zoom_wide(gint ch, NF_PTZ_CMD* cmd, NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	gint rtn = IPCAM_SETUP_RTN_FAILED;
	gint zoom_min = 0;
	gint zoom_max = 0;
	gint zoom_delta = 0;
	gint speed = 0;
	gint value = 0;
	mtable* runtime = NULL;


	//IPCAM_DBG(MAJOR, "start CH(%d) speed(%d)\n", ch, speed);
	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);

	if(cmd) speed = cmd->params[0];

	if(speed == 0)
	{
		PtzData ptzProp;
		if(nf_ipcam_get_ptz_property(ch, &ptzProp) != 0)
			speed = 5;
		else
			speed = ptzProp.zoomSpeed + 1;
	}
	else
	{
		speed = speed / 10;
	}

	if(nf_ipcam_is_onvif_support(ch) != 1)
	{
		ptz_info info;
		memset(&info, 0x00, sizeof(ptz_info));
		if (runtime[ch].ptz.supported & PTZ_SETUP_ZOOM)
		{
			info.cmd = NF_PTZ_CMD_ZOOM_WIDE;
			info.zoom_speed = speed;
			IPCAM_DBG(MINOR, "zoom wide PTZ\n");
		}
		else	if (runtime[ch].sys.stdver[0] != '\0'
				&& runtime[ch].sys.model_code != NF_IPCAM_MODEL_AMB_A2
				&& runtime[ch].sys.model_code != NF_IPCAM_MODEL_AMB_D1)
		{
			info.cmd = NF_PTZ_CMD_ZOOM_WIDE;
			info.zoom_speed = speed;
			IPCAM_DBG(MINOR, "zoom wide Non-PTZ\n");
		}
		else
		{
			zoom_min = runtime[ch].ptz.zoom.min;
			value = runtime[ch].ptz.zoom.value - speed * 10;
			if (value < zoom_min) { value = zoom_min; }
			info.cmd = value;
			IPCAM_DBG(MINOR, "zoom(%d->%d)\n", runtime[ch].ptz.zoom.value, value);
		}

		rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_ZOOM, NULL, NULL, &info);
	}
	else
	{
		NFIPCamSetupPTZ_onvif info;
		memset(&info, 0x00, sizeof(NFIPCamSetupPTZ_onvif));
		info.mode = NF_IPCAM_PTZ_MODE_ONVIF_CONTINUOUS;
		info.speed_zoom = (-1) * speed;

		//info.mode = NF_IPCAM_PTZ_MODE_ONVIF_RELATIVE;
		//info.relative_zoom = (-1) * speed;
		//info.speed_zoom = speed;

		if(cmd) info.ePTZAreaId = cmd->params[1];

		rtn = nf_ipcam_set_ptz_onvif(ch, &info, NULL, NULL, NULL);
	}

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "zoom setup request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			runtime[ch].ptz.zoom.value = (guint)value;
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}
#endif

/**
 * @brief Autofocus 설정을 변경한다.
 * @param ch 채널 번호.
 * @param onoff Autofocus 사용여부.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_set_auto_focus(gint ch, gint onoff)
{
	gint rtn = IPCAM_SETUP_RTN_FAILED;
	gint value = 0;
	mtable* runtime = get_runtime();

	//IPCAM_DBG(MAJOR, "start CH(%d) onoff(%d)\n", ch, onoff);
	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	if(!( runtime[ch].state & MGMT_STATE_CONFIGURED))
	{
		return IPCAM_SETUP_RTN_FAILED;
	}

	if(!( runtime[ch].ptz.supported & PTZ_SETUP_FOCUS))
	{
		return IPCAM_SETUP_RTN_FAILED;
	}

	//g_warning("[%s] auto_focus setup request ch(%d)\n", __FUNCTION__, ch);

	value = onoff;
	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_AF_MODE, NULL, NULL, &value);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, " auto_focus setup request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief Autoiris 설정을 변경한다.
 * @param ch 채널 번호.
 * @param onoff Autoiris 사용여부.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_set_auto_iris(gint ch, gint onoff)
{
	gint rtn = IPCAM_SETUP_RTN_FAILED;
	gint value = 0;
	mtable* runtime = get_runtime();

	//IPCAM_DBG(MAJOR, "start CH(%d) onoff(%d)\n", ch, onoff);
	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	if(!( runtime[ch].state & MGMT_STATE_CONFIGURED))
	{
		return IPCAM_SETUP_RTN_FAILED;
	}
	if(!( runtime[ch].ptz.supported & PTZ_SETUP_IRIS))
	{
		return IPCAM_SETUP_RTN_FAILED;
	}

	//g_warning("[%s] auto_iris setup request ch(%d)\n", __FUNCTION__, ch);

	value = onoff;
	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_AR_MODE, NULL, NULL, &value);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "auto_iris setup request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief 카메라의 focus 위치를 변경한다.
 * @param ch 채널 번호.
 * @param value Focus 위치값.
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *
 * @deprecated 현재 absolute focus 기능을 사용안함.
 */
int nf_ipcam_set_focus(gint ch, gint value,
		NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	gint rtn = IPCAM_SETUP_RTN_FAILED;
	gint set_val = 0;
	mtable* runtime = NULL;


	//IPCAM_DBG(MAJOR, "start CH(%d) value(%d)\n", ch, value);
	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].image.supported & NF_IPCAM_IMAGE_FOCUS, IPCAM_SETUP_RTN_FAILED);

	set_val = value;

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_FOCUS, cb_fxn, user_data, &set_val);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "focus setup request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			runtime[ch].ptz.focus.value = (guint)set_val;
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

#if 0
int nf_ipcam_set_focus_near(gint ch)
{
	gint rtn = IPCAM_SETUP_RTN_FAILED;
	gint focus_min = 0;
	gint focus_max = 0;
	gint focus_delta = 0;
	gint set_val = 0;
	gint value = 0;
	mtable* runtime = NULL;


	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].img.supported & NF_IPCAM_IMAGE_FOCUS, IPCAM_SETUP_RTN_FAILED);

	IPCAM_DBG("[%s] ch(%d)\n", __FUNCTION__, ch);

	focus_min = runtime[ch].focus.min;
	focus_max = runtime[ch].focus.max;
	focus_delta = focus_max - focus_min;
	g_return_val_if_fail( focus_delta != 0, IPCAM_SETUP_RTN_FAILED);

	value = (runtime[ch].focus.value - focus_min) * 100 / focus_delta;
	if (value < 98) { value += 2; }
	else { value = 100; }

	set_val = ((value*focus_delta)/100) + focus_min;

	IPCAM_DBG("[%s] changed value considered camera profile\n", __FUNCTION__);
	IPCAM_DBG("[%s]  ==> focus(%d:%d)\n", __FUNCTION__, value, set_val);

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_FOCUS, NULL, NULL, &set_val);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			g_warning("[%s] focus setup request failed ch(%d)\n", __FUNCTION__, ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			runtime[ch].focus.value = (guint)set_val;
			break;
		default:
			break;
	}

	return rtn;
}

int nf_ipcam_set_focus_far(gint ch)
{
	gint rtn = IPCAM_SETUP_RTN_FAILED;
	gint focus_min = 0;
	gint focus_max = 0;
	gint focus_delta = 0;
	gint set_val = 0;
	gint value = 0;
	mtable* runtime = NULL;


	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].img.supported & NF_IPCAM_IMAGE_FOCUS, IPCAM_SETUP_RTN_FAILED);

	IPCAM_DBG("[%s] ch(%d)\n", __FUNCTION__, ch);

	focus_min = runtime[ch].focus.min;
	focus_max = runtime[ch].focus.max;
	focus_delta = focus_max - focus_min;
	g_return_val_if_fail( focus_delta != 0, IPCAM_SETUP_RTN_FAILED);

	value = (runtime[ch].focus.value - focus_min) * 100 / focus_delta;
	if (value > 1) { value -= 1; }
	else { value = 0; }

	set_val = ((value*focus_delta)/100) + focus_min;

	IPCAM_DBG("[%s] changed value considered camera profile\n", __FUNCTION__);
	IPCAM_DBG("[%s]  ==> focus(%d:%d)\n", __FUNCTION__, value, set_val);

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_FOCUS, NULL, NULL, &set_val);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			g_warning("[%s] focus setup request failed ch(%d)\n", __FUNCTION__, ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			runtime[ch].focus.value = (guint)set_val;
			break;
		default:
			break;
	}

	return rtn;
}
#else

/**
 * @brief Focus위치를 가까운 쪽으로 변경한다.
 * @param ch 채널 번호.
 * @param speed 속도.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *
 * Speed가 정상적인 값이 넘어오지 않아 내부 API에서 speed 계산함.
 */
int nf_ipcam_set_focus_near(gint ch, gint speed)
{
	gint rtn = IPCAM_SETUP_RTN_FAILED;
	gint focus_min = 0;
	gint focus_max = 0;
	gint focus_delta = 0;
	gint value = 0;
	mtable* runtime = NULL;

	runtime = get_runtime();

	char key[64];

	printf("[%s] start CH(%d) speed(%d)\n", __func__, ch, speed);

	if(speed == 0)
	{
		PtzData ptzProp;
		if(nf_ipcam_get_ptz_property(ch, &ptzProp) != 0)
			speed = 5;
		else
			speed = ptzProp.focusSpeed + 1;
	}
	else
	{
		speed = speed / 10;
	}

	if(nf_ipcam_is_onvif_support(ch) == 1)
	{
		g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
		g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);

		NFIPCamSetupFocus_onvif info;

		memset(&info, 0x00, sizeof(NFIPCamSetupFocus_onvif));

		info.mode = NF_IPCAM_FOCUS_MODE_ONVIF_CONTINUOUS;
		info.speed = speed * (-1);

		rtn = nf_ipcam_set_focus_onvif(ch, &info, NULL, NULL, NULL);
	}
	else
	{
		ptz_info info;
		memset(&info, 0x00, sizeof(ptz_info));

		g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
		g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);
		g_return_val_if_fail( speed > 0, IPCAM_SETUP_RTN_FAILED);

		if (runtime[ch].ptz.supported & PTZ_SETUP_FOCUS)
		{
			info.cmd = NF_PTZ_CMD_FOCUS_NEAR;
			info.focus_speed = speed;
			IPCAM_DBG(MINOR, "focus near\n");
		}
		else	if (runtime[ch].sys.stdver[0] != '\0'
				&& runtime[ch].sys.model_code != NF_IPCAM_MODEL_AMB_A2
				&& runtime[ch].sys.model_code != NF_IPCAM_MODEL_AMB_D1)
		{
			info.cmd = NF_PTZ_CMD_FOCUS_NEAR;
			info.focus_speed = speed;
			IPCAM_DBG(MINOR, "focus wide Non-PTZ\n");
		}
		else
		{
			focus_min = runtime[ch].ptz.focus.min;
			focus_max = runtime[ch].ptz.focus.max;
			focus_delta = focus_max - focus_min;
			g_return_val_if_fail( focus_delta != 0, IPCAM_SETUP_RTN_FAILED);

			value = roundi((double)(runtime[ch].ptz.focus.value - focus_min) * 100.0 / (double)focus_delta);
			value += speed;
			if(value > 100) { value = 100; }

			value = roundi((double)(value * focus_delta) / 100.0) + focus_min;

			if(value == runtime[ch].ptz.focus.value && value < focus_max) {value++;}
			info.cmd = value;
			info.focus_speed = speed;

			IPCAM_DBG(MINOR, "focus(%d->%d)\n", runtime[ch].ptz.focus.value, value);
		}

		printf("[%s] info.cmd:(%d) / info.focus_speed:(%d)\n", __func__, info.cmd, info.focus_speed);
		rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_FOCUS, NULL, NULL, &info);

		switch(rtn)
		{
			case IPCAM_SETUP_RTN_FAILED:
				IPCAM_DBG(WARN,"focus setup request failed ch(%d)\n", ch);
				break;
			case IPCAM_SETUP_RTN_DONE:
			case IPCAM_SETUP_RTN_PENDING:
				runtime[ch].ptz.focus.value = (guint)value;
				break;
			default:
				rtn = IPCAM_SETUP_RTN_FAILED;
				break;
		}
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief Focus위치를 먼 쪽으로 변경한다.
 * @param ch 채널 번호.
 * @param speed 속도.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *
 * Speed가 정상적인 값이 넘어오지 않아 내부 API에서 speed 계산함.
 */
int nf_ipcam_set_focus_far(gint ch, gint speed)
{
	gint rtn = IPCAM_SETUP_RTN_FAILED;
	gint focus_min = 0;
	gint focus_max = 0;
	gint focus_delta = 0;
	gint value = 0;
	mtable* runtime = NULL;


	runtime = get_runtime();

	char key[64];


	printf("[%s] start CH(%d) speed(%d)\n", __func__, ch, speed);

	if(speed == 0)
	{
		PtzData ptzProp;
		if(nf_ipcam_get_ptz_property(ch, &ptzProp) != 0)
			speed = 5;
		else
			speed = ptzProp.focusSpeed + 1;
	}
	else
	{
		speed = speed / 10;
	}

	if(nf_ipcam_is_onvif_support(ch) == 1)
	{
		g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
		g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);

		NFIPCamSetupFocus_onvif info;

		memset(&info, 0x00, sizeof(NFIPCamSetupFocus_onvif));

		info.mode = NF_IPCAM_FOCUS_MODE_ONVIF_CONTINUOUS;
		info.speed = speed;

		rtn = nf_ipcam_set_focus_onvif(ch, &info, NULL, NULL, NULL);
	}
	else
	{
		ptz_info info;
		memset(&info, 0x00, sizeof(ptz_info));

		g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
		g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);
		g_return_val_if_fail( speed > 0, IPCAM_SETUP_RTN_FAILED);

		if (runtime[ch].ptz.supported & PTZ_SETUP_FOCUS)
		{
			info.cmd = NF_PTZ_CMD_FOCUS_FAR;
			info.focus_speed = speed;
			IPCAM_DBG(MINOR, "focus far\n");
		}
		else 	if (runtime[ch].sys.stdver[0] != '\0'
				&& runtime[ch].sys.model_code != NF_IPCAM_MODEL_AMB_A2
				&& runtime[ch].sys.model_code != NF_IPCAM_MODEL_AMB_D1)
		{
			info.cmd = NF_PTZ_CMD_FOCUS_FAR;
			info.focus_speed = speed;
			IPCAM_DBG(MINOR, "focus far Non-PTZ\n");
		}
		else
		{
			focus_min = runtime[ch].ptz.focus.min;
			focus_max = runtime[ch].ptz.focus.max;
			focus_delta = focus_max - focus_min;
			g_return_val_if_fail( focus_delta != 0, IPCAM_SETUP_RTN_FAILED);

			value = roundi((double)(runtime[ch].ptz.focus.value - focus_min) * 100 / (double)focus_delta);
			value -= speed;
			if(value < 0 ) { value = 0; }

			value = roundi((double)(value * focus_delta) / 100.0) + focus_min;

			if(value == runtime[ch].ptz.focus.value && value < focus_min) {value--;}
			info.cmd = value;
			info.focus_speed = speed;

			IPCAM_DBG(MINOR, "focus(%d->%d)\n", runtime[ch].ptz.focus.value, value);
		}

		printf("[%s] info.cmd:(%d) / info.focus_speed:(%d)\n", __func__, info.cmd, info.focus_speed);
		rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_FOCUS, NULL, NULL, &info);

		switch(rtn)
		{
			case IPCAM_SETUP_RTN_FAILED:
				IPCAM_DBG(WARN, "focus setup request failed ch(%d)\n", ch);
				break;
			case IPCAM_SETUP_RTN_DONE:
			case IPCAM_SETUP_RTN_PENDING:
				runtime[ch].ptz.focus.value = (guint)value;
				break;
			default:
				rtn = IPCAM_SETUP_RTN_FAILED;
				break;
		}
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}
#endif

/**
 * @brief 카메라의 iris 위치를 변경한다.
 * @param ch 채널 번호.
 * @param value Iris 위치값.
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_set_iris(gint ch, gint value,
		NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	gint rtn = IPCAM_SETUP_RTN_FAILED;
	gint set_val = 0;
	mtable* runtime = NULL;


	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].image.supported & NF_IPCAM_IMAGE_PIRIS, IPCAM_SETUP_RTN_FAILED);

	//IPCAM_DBG(MAJOR, "start CH(%d) value(%d)\n", ch, value);

	set_val = value;

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_IRIS, cb_fxn, user_data, &set_val);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "iris setup request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			runtime[ch].ptz.iris.value = (guint)set_val;
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief Iris를 닫는다.
 * @param ch 채널 번호.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_set_iris_close(gint ch)
{
	gint rtn = IPCAM_SETUP_RTN_FAILED;
	gint iris_min = 0;
	gint iris_max = 0;
	gint iris_delta = 0;
	gint value = 0;
	mtable* runtime = NULL;

	runtime = get_runtime();

	char key[64];
	guint speed;


	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);
	snprintf(key, 64, "cam.ptz.P%d.iris_spd", ch);
	speed = nf_sysdb_get_uint(key) / 10;

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( speed > 0, IPCAM_SETUP_RTN_FAILED);

	/*if (runtime[ch].ptz.supported & PTZ_SETUP_IRIS)
	{
		value = NF_PTZ_CMD_IRIS_CLOSE;
		IPCAM_DBG(MINOR, "iris close PTZ\n");
	}
	else*/
	{
		iris_min = runtime[ch].ptz.iris.min;
		iris_max = runtime[ch].ptz.iris.max;
		iris_delta = iris_max - iris_min;
		g_return_val_if_fail( iris_delta != 0, IPCAM_SETUP_RTN_FAILED);

		value = roundi((double)(runtime[ch].ptz.iris.value - iris_min) * 100.0 / (double)iris_delta);
		if(value > speed) { value -= speed; }
		else { value = 0; }
		value = roundi((double)(value*iris_delta)/100.0) + iris_min;

		if(value == runtime[ch].ptz.iris.value && value > iris_min) {value--; }

		IPCAM_DBG(MINOR, "changed value considered camera profile\n");
		IPCAM_DBG(MINOR, " ==> iris(%d -> %d)\n", runtime[ch].ptz.iris.value, value);
	}

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_IRIS, NULL, NULL, &value);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "iris setup request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			/*if (runtime[ch].ptz.supported & PTZ_SETUP_IRIS)
			{

			}
			else*/
			{
				runtime[ch].ptz.iris.value = (guint)value;

				nf_sysdb_lock(NF_SYSDB_CATE_CAM);

				GValue set_value = {0,};

				g_value_init(&set_value, G_TYPE_INT);
				g_value_set_int(&set_value, (gint)value);
				if(!nf_sysdb_set_key1("cam.C%d.iris", ch, &set_value, NULL))
				{
					g_value_unset(&set_value);
				}

				nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
			}
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief Iris를 연다.
 * @param ch 채널 번호.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_set_iris_open(gint ch)
{
	gint rtn = IPCAM_SETUP_RTN_FAILED;
	gint iris_min = 0;
	gint iris_max = 0;
	gint iris_delta = 0;
	gint value = 0;
	mtable* runtime = NULL;

	runtime = get_runtime();

	char key[64];
	guint speed;


	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	snprintf(key, 64, "cam.ptz.P%d.iris_spd", ch);
	speed = nf_sysdb_get_uint(key) / 10;

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( speed > 0, IPCAM_SETUP_RTN_FAILED);

	/*if (runtime[ch].ptz.supported & PTZ_SETUP_IRIS)
	{
		value = NF_PTZ_CMD_IRIS_OPEN;
		IPCAM_DBG(MINOR, "iris open PTZ\n");
	}
	else*/
	{
		iris_min = runtime[ch].ptz.iris.min;
		iris_max = runtime[ch].ptz.iris.max;
		iris_delta = iris_max - iris_min;
		g_return_val_if_fail( iris_delta != 0, IPCAM_SETUP_RTN_FAILED);

		value = roundi((double)(runtime[ch].ptz.iris.value - iris_min) * 100.0 / (double)iris_delta);
		//if (value < 95) { value += 5; }
		if(value < 100 - speed) {value += speed; }
		else { value = 100; }

		value = roundi((double)(value*iris_delta)/100.0) + iris_min;

		if(value == runtime[ch].ptz.iris.value && value < iris_max) {value++; }

		IPCAM_DBG(MINOR, "changed value considered camera profile\n");
		IPCAM_DBG(MINOR, " ==> iris(%d -> %d)\n", runtime[ch].ptz.iris.value, value);
	}

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_IRIS, NULL, NULL, &value);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "iris setup request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			/*if (runtime[ch].ptz.supported & PTZ_SETUP_IRIS)
			{

			}
			else*/
			{
				runtime[ch].ptz.iris.value = (guint)value;

				nf_sysdb_lock(NF_SYSDB_CATE_CAM);

				GValue set_value = {0,};

				g_value_init(&set_value, G_TYPE_INT);
				g_value_set_int(&set_value, (gint)value);
				if(!nf_sysdb_set_key1("cam.C%d.iris", ch, &set_value, NULL))
				{
					g_value_unset(&set_value);
				}

				nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
			}
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief 카메라의 lens 위치를 초기위치로 변경한다.(PTZ HOME)
 * @param ch 채널 번호.
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *
 * Home 명령을 지원안할 시 Preset 0번으로 이동한다.
 */
int nf_ipcam_set_lens_default(gint ch,
		NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	gint rtn = IPCAM_SETUP_RTN_FAILED;
	mtable* runtime = NULL;

	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);

	int ret;
	unsigned int val = 0;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);
	ret = nf_ipcam_get_ptz_support(ch, NF_IPCAM_IMAGE_CALIBRATION, &val);

	if (ret == IPCAM_SETUP_RTN_DONE)
	{
		if (val)
		{	// home support
			rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_ORIGIN, cb_fxn, user_data, NULL);

			switch(rtn)
			{
				case IPCAM_SETUP_RTN_FAILED:
					IPCAM_DBG(WARN, "origin focus setup request failed ch(%d)\n", ch);
					break;
				case IPCAM_SETUP_RTN_DONE:
				case IPCAM_SETUP_RTN_PENDING:
					break;
				default:
					rtn = IPCAM_SETUP_RTN_FAILED;
					break;
			}

			IPCAM_DBG(MAJOR, "end 1 CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
		}
	}
	else
	{	// no home func, goto preset 0
		NF_PTZ_CMD ptzCmd;
		PtzPresetData ptzPreset;

		DAL_get_ptz_preset_data(&ptzPreset, ch);
		if(ptzPreset.cnt == 0)
		{
			IPCAM_DBG(MAJOR, "end 2 CH(%d) no home no preset\n", ch);
			return IPCAM_SETUP_RTN_DONE;
		}

		ptzCmd.cmd = NF_PTZ_CMD_GOTO_PRESET;
		ptzCmd.params[0] = ptzPreset.number[0];
		nf_ptz_cmd(&ptzCmd);

		IPCAM_DBG(MAJOR, "end 3 CH(%d)\n", ch);
		return IPCAM_SETUP_RTN_DONE;
	}

	return rtn;
}

/**
 * @brief 카메라의 focus를 자동으로 맞춘다.(Onepush)
 * @param ch 채널 번호.
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_set_oneshot(gint ch,
		NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	gint rtn = IPCAM_SETUP_RTN_FAILED;
	mtable* runtime = NULL;


	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);
	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_ONESHOT, cb_fxn, user_data, NULL);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "onepush focus setup request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief Privacy mask 설정 영역으로 PTZ 이동한다.(NPT만 해당)
 * @param ch 채널 번호.
 * @param pmask_no Privacy mask 영역 index.
 * @return IPCAM_SETUP_RTN_DONE - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *
 * @todo PTZ privacy mask 구현시 필요. 현재는 사용안함.
 */
int nf_ipcam_goto_privacy_mask(gint ch, gint pmask_no)
{
	mtable* runtime = NULL;

	runtime = get_runtime();

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	g_return_val_if_fail( pmask_no < MAX_PRIVACY_CNT  , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);

	g_return_val_if_fail( runtime[ch].func & NF_IPCAM_FUNC_PRIVACY_MASK, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].func & NF_IPCAM_FUNC_PTZ, IPCAM_SETUP_RTN_FAILED);

	// FIXME
	int rtn = npt_goto_pmask(pmask_no, ch);

	//IPCAM_DBG(MAJOR, "end CH(%d)\n", ch);
	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief 카메라의 Privacy mask를 설정한다.
 * @param ch 채널 번호.
 * @param info Privacy mask 설정 정보 struct.
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_set_privacy_mask(gint ch, NFIPCamPrivacyMask *info, NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	int rtn;
	mtable *runtime = NULL;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);
	runtime = get_runtime();

	g_return_val_if_fail( info != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].func & NF_IPCAM_FUNC_PRIVACY_MASK, IPCAM_SETUP_RTN_FAILED);

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_PMASK, cb_fxn, user_data, info);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "privacy mask setup request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_PENDING:
		case IPCAM_SETUP_RTN_DONE:
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief 카메라의 ROI Area 를 설정한다.
 * @param ch 채널 번호.
 * @param info ROI Area 설정 정보 struct.
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_set_roi_area(gint ch, NFIPCamSetupROIArea *info, NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	int rtn;
	mtable *runtime = NULL;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);
	runtime = get_runtime();

	g_return_val_if_fail( info != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].func & NF_IPCAM_FUNC_ROI, IPCAM_SETUP_RTN_FAILED);

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_ROI, cb_fxn, user_data, info);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "ROI Area setup request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_PENDING:
		case IPCAM_SETUP_RTN_DONE:
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief 카메라의 fps를 설정한다.
 * @param ch 채널 번호.
 * @param first 1st stream fps enum값.
 * @param second 2nd stream fps enum값.
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *
 * 단위테스트 용도로 개발.
 */
int nf_ipcam_set_fps(gint ch, NF_IPCAM_FPS_E first, NF_IPCAM_FPS_E second,
		NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	int rtn;
	int ntpal;
	cam_info mgr_info;
	mtable* runtime = NULL;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	memset(&mgr_info, 0x00, sizeof(cam_info));
	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].sys.model_code == NF_IPCAM_MODEL_TI_368, IPCAM_SETUP_RTN_FAILED);

	ntpal = (runtime[ch].video.anti_flicker.value == NF_IPCAM_AF_NTSC) ? 0:1;
	mgr_info.vcodec.resolution[0] = runtime[ch].video.resolution.resolution[0];
	mgr_info.vcodec.resolution[1] = runtime[ch].video.resolution.resolution[1];
	mgr_info.vcodec.vcodec[0] = runtime[ch].video.vcodec[0];
	mgr_info.vcodec.vcodec[1] = runtime[ch].video.vcodec[1];
	mgr_info.vcodec.fps[0] = first;
	mgr_info.vcodec.fps[1] = second;
	mgr_info.vcodec.bitrate[0] = runtime[ch].video.bitrate[0].value;
	mgr_info.vcodec.bitrate[1] = runtime[ch].video.bitrate[1].value;
	mgr_info.vcodec.af = ntpal ? 50:60;
	mgr_info.vcodec.mirror = runtime[ch].video.mirror.value;

	mgr_info.vcodec.bitctrl[0] = runtime[ch].video.bitctrl[0];
	mgr_info.vcodec.bitctrl[1] = runtime[ch].video.bitctrl[1];
	mgr_info.vcodec.capture = runtime[ch].video.capture.value; 

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_VCODEC, cb_fxn, user_data, &mgr_info);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "fps setup request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			runtime[ch].video.fps[ntpal][0].value = first;
			runtime[ch].video.fps[ntpal][1].value = second;
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief 특정 채널의 bitrate를 올린다.(TI카메라 전용)
 * @param ch 채널 번호.
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *
 * @deprecated
 */
int nf_ipcam_set_intensive_ch(gint ch,
		NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	int i;
	int rtn;
	int ntpal;
	cam_info info;
	mtable *runtime = NULL;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	return IPCAM_SETUP_RTN_DONE;
	memset(&info, 0x00, sizeof(cam_info));

	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].sys.model_code == NF_IPCAM_MODEL_TI_368, IPCAM_SETUP_RTN_FAILED);

#if defined(ENABLE_PROJECT_KUMMI)||defined(ENABLE_PROJECT_KMW)
	return IPCAM_SETUP_RTN_DONE;
#endif

	ntpal = (runtime[ch].video.anti_flicker.value == NF_IPCAM_AF_NTSC) ? 0:1;
	for (i = 0; i < AVAILABLE_MAX_CH; i++)
	{
		if (runtime[ch].intensive_ch)
		{
			nf_ipcam_set_normal_ch(ch, NULL, NULL, NULL);
		}
	}

	runtime[ch].intensive_ch = 1;

	info.vcodec.resolution[0] = runtime[ch].video.resolution.resolution[0];
	info.vcodec.resolution[1] = runtime[ch].video.resolution.resolution[1];
	info.vcodec.vcodec[0] = runtime[ch].video.vcodec[0];
	info.vcodec.vcodec[1] = runtime[ch].video.vcodec[1];
	info.vcodec.fps[0] = runtime[ch].video.fps[ntpal][0].value;
	info.vcodec.fps[1] = runtime[ch].video.fps[ntpal][1].value;
	info.vcodec.bitrate[0] = 8000;
	if (info.vcodec.fps[0] == NF_IPCAM_FPS_150 || info.vcodec.fps[0] == NF_IPCAM_FPS_120)
	{
		info.vcodec.bitrate[0] = 6000;
	}
	info.vcodec.bitrate[1] = runtime[ch].video.bitrate[1].value;
	info.vcodec.af = ntpal ? 50:60;
	info.vcodec.mirror = runtime[ch].video.mirror.value;

	info.vcodec.bitctrl[0] = runtime[ch].video.bitctrl[0];
	info.vcodec.bitctrl[1] = runtime[ch].video.bitctrl[1];
	info.vcodec.capture = runtime[ch].video.capture.value; 

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_VCODEC, cb_fxn, user_data, &info);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "bitrate setup request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			//runtime[ch].bitrate[0] = info.vcodec.bitrate[0];
			//runtime[ch].bitrate[1] = second;
			//runtime[ch].bitrate[0] = first;
			//runtime[ch].bitrate[1] = second;
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief 특정 채널의 bitrate를 정상 상태로 되돌린다.(TI카메라 전용)
 * @param ch 채널 번호.
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *
 * @sa nf_ipcam_set_intensive_ch
 * @deprecated
 */
int nf_ipcam_set_normal_ch(gint ch,
		NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	int rtn;
	int ntpal;
	cam_info info;
	mtable *runtime = NULL;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);
	return IPCAM_SETUP_RTN_DONE;

	memset(&info, 0x00, sizeof(cam_info));

	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].sys.model_code == NF_IPCAM_MODEL_TI_368, IPCAM_SETUP_RTN_FAILED);

	ntpal = (runtime[ch].video.anti_flicker.value == NF_IPCAM_AF_NTSC) ? 0:1;
	runtime[ch].intensive_ch = 0;

	info.vcodec.resolution[0] = runtime[ch].video.resolution.resolution[0];
	info.vcodec.resolution[1] = runtime[ch].video.resolution.resolution[1];
	info.vcodec.vcodec[0] = runtime[ch].video.vcodec[0];
	info.vcodec.vcodec[1] = runtime[ch].video.vcodec[1];
	info.vcodec.fps[0] = runtime[ch].video.fps[ntpal][0].value;
	info.vcodec.fps[1] = runtime[ch].video.fps[ntpal][1].value;
	info.vcodec.bitrate[0] = runtime[ch].video.bitrate[0].value;
	info.vcodec.bitrate[1] = runtime[ch].video.bitrate[1].value;
	info.vcodec.af = ntpal ? 50:60;
	info.vcodec.mirror = runtime[ch].video.mirror.value;

	info.vcodec.bitctrl[0] = runtime[ch].video.bitctrl[0];
	info.vcodec.bitctrl[1] = runtime[ch].video.bitctrl[1];
	info.vcodec.capture = runtime[ch].video.capture.value; 

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_VCODEC, cb_fxn, user_data, &info);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "bitrate setup request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			//runtime[ch].bitrate[0] = info.vcodec.bitrate[0];
			//runtime[ch].bitrate[1] = second;
			//runtime[ch].bitrate[0] = first;
			//runtime[ch].bitrate[1] = second;
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}
	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);

	return rtn;
}

/**
 * NCX-365 temporary code for ISC demo. NEVER use at A2 camera series.
 * @param ch
 * @param first
 * @param second
 * @param cb_fxn
 * @param user_data
 * @param error
 * @return IPCAM_SETUN_RTN_DONE.
 *
 * @deprecated
 */
int nf_ipcam_set_bitrate(gint ch, guint first, guint second,
		NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	return IPCAM_SETUP_RTN_DONE;
}

/**
 * @brief 카메라의 스마트 모션 정보를 설정한다
 * @param ch 채널 번호.
 * @param info 스마트 모션 On/Off, object 종류
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_set_motion_smart
(gint ch, NFIPCamSetupMotionSmart* info,
		NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	int i;
	int rtn;
	mtable* runtime = NULL;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	runtime = get_runtime();

	g_return_val_if_fail( info != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_READY, IPCAM_SETUP_RTN_FAILED);

	//check runtime
	if((runtime[ch].motion.smart_motion_enable == info->smart_motion_enable) &&
			(runtime[ch].motion.ai_alarm_event == info->ai_alarm_event)){
		printf("[%s:%d] db value is not changed\n", __func__, __LINE__);
		//return IPCAM_SETUP_RTN_DONE;
	}

	runtime[ch].motion.smart_motion_enable = info->smart_motion_enable;
	runtime[ch].motion.ai_alarm_event = info->ai_alarm_event;

	memcpy(runtime[ch].motion.smart_motion_options, info->smart_motion_options, sizeof(runtime[ch].motion.smart_motion_options));
	runtime[ch].motion.smart_motion_option_size = info->smart_motion_option_size;
	


	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_SMART_MOTION, cb_fxn, user_data, info);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "smart motion setup request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief 카메라의 corridor mode를 설정한다.
 * @ param ch 채널 번호.
 * @ param info corridor Mode 관련 정보
 * @ param cb_fxn 설정 종료후 호출할 콜백함수. 하지만 보통 NULL로 사용할 것으로 추측
 * @ param user_date user data (대부분 NULL)
 * @ param error 에러
 * @ return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_set_corridor_mode
(gint ch, NFIPCamSetupCorridorMode* info, NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	int i;
	int rtn;
	int corridor_view;
	char key[64];

	mtable* runtime = NULL;
	runtime = get_runtime();

	//g_return_val_if_fail( info != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL, IPCAM_SETUP_RTN_FAILED);
	//g_return_val_if_fail( runtime[ch].state & MGMT_STATE_READY, IPCAM_SETUP_RTN_FAILED);
	// Get DB
	snprintf(key, 64, "cam.C%d.corridor_view", ch);
	corridor_view = nf_sysdb_get_uint(key);

	runtime[ch].video.corridor_mode_value = corridor_view; // DB 갱신

	//rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_corridor_MODE, cb_fxn, user_data, info);
	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_CORRIDOR_MODE, cb_fxn, user_data, &corridor_view); // 나중에 HallwayMode를 업데이트 해야할 상황에는, 위 주석처리된 코드를 사용해야하며, NFIPCamSetupHallwayMode 관련 예외처리를 구현해야한다.

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "corridor_mode  setup request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief 카메라의 모션 영역을 설정한다.
 * @param ch 채널 번호.
 * @param info 모션 영역 설정 정보 struct.
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_set_motion_area
(gint ch, NFIPCamSetupMotionArea* info,
		NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	int i;
	int rtn;
	mtable* runtime = NULL;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	runtime = get_runtime();

	g_return_val_if_fail( info != NULL , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_READY, IPCAM_SETUP_RTN_FAILED);

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_MOTION, cb_fxn, user_data, info);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "motion area setup request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief PTZ preset을 현재 위치에 설정한다.
 * @param ch 채널 번호.
 * @param preset_num 프리셋 번호(1~239).
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *
 * Preset번호는 1~239까지이며, 240, 241번은 다른 용도로 사용한다.
 *
 * @sa _ptz_cmd_process
 */
int nf_ipcam_set_preset(gint ch, gint preset_num, GError **error)
{
	gint rtn = IPCAM_SETUP_RTN_FAILED;
	mtable* runtime = NULL;


	//IPCAM_DBG(MAJOR, "start CH(%d) preset_num(%d)\n", ch, preset_num);
	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( 0 < preset_num , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( preset_num < 240, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].ptz.supported & PTZ_SETUP_PRESET, IPCAM_SETUP_RTN_FAILED);

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_PRESET_SET, NULL, NULL, &preset_num);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "preset set request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief PTZ preset을 삭제한다.
 * @param ch 채널 번호.
 * @param preset_num 프리셋 번호(1~239).
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_clear_preset(gint ch, gint preset_num, GError **error)
{
	gint rtn = IPCAM_SETUP_RTN_FAILED;
	mtable* runtime = NULL;


	//IPCAM_DBG(MAJOR, "start CH(%d) preset_num(%d)\n", ch, preset_num);
	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( 0 < preset_num , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( preset_num < 240, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].ptz.supported & PTZ_SETUP_PRESET, IPCAM_SETUP_RTN_FAILED);

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_PRESET_CLEAR, NULL, NULL, &preset_num);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "preset clear request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief PTZ preset으로 이동한다.
 * @param ch 채널 번호.
 * @param preset_num 프리셋 번호(1~239).
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_go_preset(gint ch, gint preset_num, GError **error)
{
	gint rtn = IPCAM_SETUP_RTN_FAILED;
	mtable* runtime = NULL;


	//IPCAM_DBG(MAJOR, "start CH(%d) preset_num(%d)\n", ch, preset_num);
	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( 0 < preset_num , IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( preset_num < 240, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].ptz.supported & PTZ_SETUP_PRESET, IPCAM_SETUP_RTN_FAILED);

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_PRESET_GO, NULL, NULL, &preset_num);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "preset go request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief 카메라의 VA 기능을 리셋한다.
 * @param ch 채널 번호.
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_set_va_reset
(gint ch, NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	int rtn;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_RESET_VA, cb_fxn, user_data, NULL);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "Reset VA request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief 카메라의 VA 기능을 켜고 끈다.
 * @param ch 채널 번호.
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 */
int nf_ipcam_set_va_enable
(gint ch, NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	int rtn;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_ENABLE_VA, cb_fxn, user_data, NULL);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "Enable VA request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief 카메라의 VA 설정을 변경한다.
 * @param ch 채널 번호.
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *
 * 설정값을 파라메터로 받지 않고 Driver에서 sysdb 조회를 통해 설정한다.
 */
int nf_ipcam_set_va_config
(gint ch, NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	int rtn;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_VA_CONFIG, cb_fxn, user_data, NULL);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "VA CONFIG request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief 카메라의 VA 옵션을 변경한다.
 * @param ch 채널 번호.
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *
 * @deprecated nf_ipcam_set_va_config으로 통합됨.
 */
int nf_ipcam_set_va_option
(gint ch, NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	int i;
	int rtn;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_VA_OPTION, cb_fxn, user_data, NULL);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "VA OPTION request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

void *nf_ipcam_set_motion_thread_func(void *data)
{
	int ret = 0;
    int ch;
	mtable* runtime = NULL;
	NF_ONVIF_MOTION_DATA *info;
	if(data == NULL) return;
	info = (NF_ONVIF_MOTION_DATA*)data;

    //request motion area set
	ret = nf_ipcam_set_motion_area(info->ch, &info->area, NULL, NULL, NULL);

    //request smart motion set
    runtime = get_runtime();
    ch = info->ch;
    if(ch < 0 || ch >= NUM_IPX_CHANNEL){
        return (void *)ret;
    }

    if(runtime[ch].motion.smart_motion_support){
        ret = nf_ipcam_set_motion_smart(info->ch, &info->smart, NULL, NULL, NULL);
    }
	return (void *)ret;
}

/**
 * @brief 전 채널 모션설정 변경을  쓰레드로 처리.
 * @param p_ch_mask 변경되는 채널 bitmask.
 * @param p_fps1 1st stream fps값 배열.
 * @param p_fps2 2nd stream fps값 배열.
 * @param p_qual1 1st stream quality값 배열.
 * @param p_qual2 2nd stream quality값 배열.
 * @return 항상 0.
 */

int nf_ipcam_set_motion_thread(unsigned int p_ch_mask)
{
	NF_ONVIF_MOTION_DATA motion_data[NUM_IPX_CHANNEL];
    int policy;
    struct sched_param sched;
	int port, ret;
	int thread_ch_mask = 0;
	char key[64];
	unsigned int sense_d;
	mtable *runtime = NULL;
	runtime = get_runtime();
	GAsyncQueue *queue = get_queue();

	policy = SCHED_FIFO;
	sched.sched_priority = sched_get_priority_max(policy) - 1;

	IPCAM_DBG(MAJOR, "start\n");
	for (port = 0; port < NUM_ACTIVE_CH; port++)
	{
		if ((runtime[port].state & MGMT_STATE_CONFIGURED) == 0) { continue; }
		if ((runtime[port].func & NF_IPCAM_FUNC_MOTION) == 0) { continue; }
		if (p_ch_mask & (1<<port))
		{
			if ((runtime[port].state & MGMT_STATE_CONFIGURED) == 0) { continue; }
			if ((runtime[port].func & NF_IPCAM_FUNC_MOTION) == 0) { continue; }

			motion_data[port].ch = port;

			{	/* Motion Area */
				NFIPCamSetupMotionArea info;
				gchar *area;

				memset(&info, 0x00, sizeof(NFIPCamSetupMotionArea));
				snprintf(key, 64, "alarm.motion.M%d.area", port);
				area = nf_sysdb_get_str_nocopy(key);
				strncpy(info.area, area, 1400);

				if (get_dn_now(port) == 1)
				{
					snprintf(key, 64, "alarm.motion.M%d.sense_d", port);
				}
				else
				{
					snprintf(key, 64, "alarm.motion.M%d.sense_n", port);
				}
				sense_d = nf_sysdb_get_uint(key);
				snprintf(key, 64, "alarm.motion.M%d.rect.RCNT", port);
				info.area_num = nf_sysdb_get_uint(key);

				if (info.area_num == 0)
				{
					info.ch = port;
					info.block_width = runtime[port].motion.block_width;
					info.block_height = runtime[port].motion.block_height;
					info.method = runtime[port].motion.method;
					info.area_num = 0;
					info.marea[0].sensitivity = sense_d;
					_get_rect_points(
							info.block_width,
							info.block_height,
							area,
							&info.marea[0].FIGURE.RECTANGLE.left_top,
							&info.marea[0].FIGURE.RECTANGLE.right_bottom
					);
				}
				else 
				{
					int i = 0;
					info.ch = port;
					info.block_width = runtime[port].motion.block_width;
					info.block_height = runtime[port].motion.block_height;
					info.method = runtime[port].motion.method;
					for (i = 0; i < info.area_num; i++)
					{
						info.marea[i].sensitivity = sense_d;
						snprintf(key, 64, "alarm.motion.M%d.rect.R%d.row1", port, i);
						info.marea[i].FIGURE.RECTANGLE.left_top.y = nf_sysdb_get_uint(key);
						snprintf(key, 64, "alarm.motion.M%d.rect.R%d.col1", port, i);
						info.marea[i].FIGURE.RECTANGLE.left_top.x = nf_sysdb_get_uint(key);
						snprintf(key, 64, "alarm.motion.M%d.rect.R%d.row2", port, i);
						info.marea[i].FIGURE.RECTANGLE.right_bottom.y = nf_sysdb_get_uint(key);
						snprintf(key, 64, "alarm.motion.M%d.rect.R%d.col2", port, i);
						info.marea[i].FIGURE.RECTANGLE.right_bottom.x = nf_sysdb_get_uint(key);
					}
				}
				memcpy(&motion_data[port].area, &info, sizeof(NFIPCamSetupMotionArea));
				//break;
			}

			{	/* Smart Motion*/
				NFIPCamSetupMotionSmart info;
				memset(&info, 0x00, sizeof(NFIPCamSetupMotionSmart));

				//db값으로 runtime 객체 update
				info.ch = port;

				snprintf(key, 64, "alarm.motion.M%d.smart_motion", port);
				info.smart_motion_enable = nf_sysdb_get_bool(key);

				snprintf(key, 64, "alarm.motion.M%d.use_ai_alarmevt", port);
				info.ai_alarm_event = nf_sysdb_get_bool(key);

				snprintf(key, 64, "alarm.motion.M%d.smart_interest_obj", port);
                info.smart_motion_option_size = smart_motion_parse_db_string(info.smart_motion_options, nf_sysdb_get_str_nocopy(key));

				memcpy(&motion_data[port].smart, &info, sizeof(NFIPCamSetupMotionSmart));
			}

			ret = pthread_create(&(motion_data[port].thread_id), NULL, nf_ipcam_set_motion_thread_func,
					&motion_data[port]);

			if (ret == 0)
			{
				ret = pthread_setschedparam (motion_data[port].thread_id, policy, &sched);
				thread_ch_mask |= (1<<port);
			}
			else
			{
				IPCAM_DBG(ERROR, "pthread_create(nf_ipcam_set_motion_thread_func) failed (CH:[%d]|ret:[%d])\n", port, ret);
			}
		}
	}

	IPCAM_DBG(MAJOR, "waiting join\n");
	for (port = 0; port < NUM_IPX_CHANNEL; port++)
	{
		if (thread_ch_mask & (1<<port) )
		{
			pthread_join(motion_data[port].thread_id, (void **)&ret);

			if (queue != NULL)
			{
				if (ret == IPCAM_SETUP_RTN_FAILED)
				{
					nf_pnd_queue_push(port, IPCAM_EVENT_CONFIG_FAIL, __LINE__, __FILE__);
					g_warning("[%s] Motion area setup failed(CH:%d)\n", __FUNCTION__, port);
				}
			}
		}
	}
	IPCAM_DBG(MAJOR, "end\n");

	return 0;
}

/**
 * @brief 카메라의 MOUNT TYPE을 변경한다.
 * @param ch 채널 번호.
 * @param mount MountType enum 포인터
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *
 * @deprecated nf_ipcam_set_va_config으로 통합됨.
 */
int nf_ipcam_set_mount(gint ch, NF_IPCAM_MOUNT_TYPES_E *mount, NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	int rtn;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_MOUNT, cb_fxn, user_data, mount);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "MOUNT TYPE request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

/**
 * @brief 카메라의 MOUNT TYPE을 변경한다.
 * @param ch 채널 번호.
 * @param dewarp DewarpMode enum 포인터
 * @param cb_fxn 설정 종료후 호출할 콜백함수.
 * @param user_data user data.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *
 * @deprecated nf_ipcam_set_va_config으로 통합됨.
 */
int nf_ipcam_set_dewarp(gint ch, NF_IPCAM_DEWARP_MODES_E *dewarp, NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	int rtn;

	//IPCAM_DBG(MAJOR, "start CH(%d)\n", ch);

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_DEWARP, cb_fxn, user_data, dewarp);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "DEWARP MODE request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			break;
		default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
	}

	//IPCAM_DBG(MAJOR, "end CH(%d) rtn(%s)\n", ch, __SETUP_RTN_STR_[rtn]);
	return rtn;
}

//captainnn
/**
 * @brief 카메라의 VA 기능을 리셋한다.
 * @param ch 채널 번호.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *
 * @deprecated S1 VA용도로 구현됨.
 */
 int nf_ipcam_reset_va(gint ch, GError **error)
{
	gint rtn = IPCAM_SETUP_RTN_FAILED;
	mtable* runtime = NULL;


	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);

	//IPCAM_DBG("[%s] ch(%d)\n", __FUNCTION__, ch);

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_RESET_VA, NULL, NULL, NULL);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "nf_ipcam_reset_va setup request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			break;
		default:
			break;
	}

	return rtn;
}

/**
 * @brief 카메라의 VA 기능을 켜고 끈다.
 * @param ch 채널 번호.
 * @param enable VA기능 사용여부.
 * @param error 에러.
 * @return IPCAM_SETUP_RTN_DONE, IPCAM_SETUP_RTN_PENDING - 성공, IPCAM_SETUP_RTN_FAILED - 실패.
 *
 * @deprecated S1 VA용도로 구현됨.
  */
 int nf_ipcam_enable_va(gint ch, gint enable, GError **error)
{
	gint rtn = IPCAM_SETUP_RTN_FAILED;
	mtable* runtime = NULL;


	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);

	//IPCAM_DBG("[%s] ch(%d)\n", __FUNCTION__, ch);

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_ENABLE_VA, NULL, NULL, &enable);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "nf_ipcam_enable_va setup request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
			break;
		default:
			break;
	}

	return rtn;
}

int nf_ipcam_set_dc_iris_calibration(gint ch, NFIPCamSetupCallback cb_fxn, gpointer user_data, GError **error)
{
	gint rtn = IPCAM_SETUP_RTN_FAILED;
	mtable* runtime = NULL;

	int ret = -1;
	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);

	if(runtime[ch].image.supported & NF_IPCAM_IMAGE_DCIRIS)
	{
		rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_DC_IRIS_CAL, cb_fxn, user_data, NULL);
		
		if(rtn == IPCAM_SETUP_RTN_DONE)
		{
			ret = nf_ipcam_get_dc_iris_cal_start_func(ch);
			if(ret == 0)
			{
				rtn = IPCAM_SETUP_RTN_FAILED;
			}
		}
		switch(rtn)
		{
			case IPCAM_SETUP_RTN_FAILED:
				IPCAM_DBG(WARN, "dc iris calibration setup request failed ch(%d)\n", ch);
				break;
			case IPCAM_SETUP_RTN_DONE:
			case IPCAM_SETUP_RTN_PENDING:
				break;
			default:
			rtn = IPCAM_SETUP_RTN_FAILED;
			break;
		}	
	}
	return rtn;
}

#if 0
//captainnn
 int nf_ipcam_set_va_option(gint ch, ivca_option_t* opt, GError **error)
{
	gint rtn = IPCAM_SETUP_RTN_FAILED;
	mtable* runtime = NULL;

	ivca_option_t opt_temp;

	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);

	printf("[%s] size %d ch(%d) \n", __FUNCTION__, sizeof(ivca_option_t),ch);
	//IPCAM_DBG("[%s] ch(%d)\n", __FUNCTION__, ch);

	memcpy(&opt_temp,opt,sizeof(ivca_option_t));

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_VA_OPTION, NULL, NULL, &opt_temp);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "nf_ipcam_set_va_option setup request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
		default:
			break;
	}

	return rtn;
}

int nf_ipcam_set_va_config(gint ch, ivca_rule_t* rules, GError **error)
{
	gint rtn = IPCAM_SETUP_RTN_FAILED;
	mtable* runtime = NULL;

	ivca_rule_t rules_temp;

	runtime = get_runtime();

	g_return_val_if_fail( ch < NUM_IPX_CHANNEL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime != NULL, IPCAM_SETUP_RTN_FAILED);
	g_return_val_if_fail( runtime[ch].state & MGMT_STATE_CONFIGURED, IPCAM_SETUP_RTN_FAILED);

	printf("[%s] size %d ch(%d) \n", __FUNCTION__, sizeof(ivca_rule_t),ch);
	//IPCAM_DBG("[%s] ch(%d)\n", __FUNCTION__, ch);

	memcpy(&rules_temp,rules,sizeof(ivca_rule_t));

	rtn = nf_ipcam_setup_request(ch, NF_IPCAM_TYPE_SET_VA_CONFIG, NULL, NULL, &rules_temp);

	switch(rtn)
	{
		case IPCAM_SETUP_RTN_FAILED:
			IPCAM_DBG(WARN, "nf_ipcam_enable_va setup request failed ch(%d)\n", ch);
			break;
		case IPCAM_SETUP_RTN_DONE:
		case IPCAM_SETUP_RTN_PENDING:
		default:
			break;
	}

	return rtn;
}
#endif


#endif // __NF_IPCAM_SETTER_C__
