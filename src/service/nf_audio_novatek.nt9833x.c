
#include "nf_common.h"

#include "nf_codec_header.h"

/* Novatec Header */
#include <novatek/hd_debug.h>
#include <novatek/vendor/vendor_common.h>

#include "nf_audio_novatek.h"
#include "nf_audio_common.h"
#include "nf_audio.h"

/**
    Gloval Variable
 **/
static HD_RESULT init_module_rec(void);
static HD_RESULT uninit_module_rec(void);
static HD_RESULT init_module_pb(void);
static HD_RESULT uninit_module_pb(void);
static HD_RESULT open_module_rec(AUDIO_RECORD *p_rec_info, int max_bitstream_num);
static HD_RESULT close_module_rec(AUDIO_RECORD *p_rec_info, int max_bitstream_num);
static HD_RESULT open_module_pb(AUDIO_PLAYBACK *p_pb_info);
static HD_RESULT close_module_pb(AUDIO_PLAYBACK *p_pb_info);
static HD_RESULT set_module_param_rec(AUDIO_RECORD *p_rec_info, UINT ch);
static HD_RESULT set_module_param_pb(AUDIO_PLAYBACK *p_pb_info);
static HD_RESULT bind_module_rec(AUDIO_RECORD *p_rec_info, UINT ch);
static HD_RESULT unbind_module_rec(AUDIO_RECORD *p_rec_info, UINT ch);
static HD_RESULT bind_module_pb(AUDIO_PLAYBACK *p_pb_info);
static HD_RESULT unbind_module_pb(void);
static HD_RESULT start_module_rec(AUDIO_RECORD *p_rec_info, UINT ch);
static HD_RESULT stop_module_rec(AUDIO_RECORD *p_rec_info, UINT ch);
static HD_RESULT start_module_pb(AUDIO_PLAYBACK *p_pb_info);
static HD_RESULT stop_module_pb(AUDIO_PLAYBACK *p_pb_info);

/*
	Function Start
*/
gboolean nf_audio_init_lib(AUDIO_RECORD *info_rec, AUDIO_PLAYBACK *info_pb, 
							guint enc_type, guint dec_type, guint aud_output_type, gint audio_nr)
{
	int i=0;
	HD_RESULT ret;
	INT key;
	AUDIO_RECORD *ptr_rec=info_rec;
	AUDIO_PLAYBACK *ptr_pb=info_pb;

	g_message("[NVT Audio Init] Start!!");

	/* set default in-dev port */
	ptr_rec->in_dev=0;

	#if 1
		/* init hdal and allocate memory */
		ret = hd_common_init(1);
		if (ret != HD_OK) {
			printf("[Audio Init Lib] common init fail\n");
			return FALSE;
		}
	#endif

	/* init audio capture and audio encode modules */
	ret = init_module_rec();
	if (ret != HD_OK) {
		printf("[REC] init moudle fail\n");
		return FALSE;
	}
	else {
		printf("[REC] init moudle\n");
	}

	/* open audio capture and audio encode modules */
	ret = open_module_rec(ptr_rec, audio_nr);
	if (ret != HD_OK) {
		printf("[REC] open moudle fail\n");
		return FALSE;
	}
	else {
		printf("[REC] open moudle\n");
	}

	/* setup runtime parameters */
	for (i = 0; i < audio_nr; i++) {
		ret = set_module_param_rec(ptr_rec, i);
		if (ret != HD_OK) {
			printf("[REC] set param fail\n");
			return FALSE;
		}
		else {
			printf("[REC] set param %d\n", i);
		}
	}

	/* bind audio record: audio capture -> audio encode */
	for (i = 0; i < audio_nr; i++) {
		ret = bind_module_rec(ptr_rec, i);
		if (ret != HD_OK) {
			printf("[REC] bind moudle fail\n");
			return FALSE;
		}
		else {
			printf("[REC] bind moudle %d\n", i);
		}
	}

	/* start to run */
	for (i = 0; i < audio_nr; i++) {
		ret = start_module_rec(ptr_rec, i);
		if (ret != HD_OK) {
			printf("[REC] start module fail\n");
			return FALSE;
		}
		else {
			printf("[REC] start module %d\n", i);
		}
	}


	/* set default out-dev port */
	if(aud_output_type == AUD_OUTPUT_RCA) {
		info_pb->out_dev=0;
	}
	else if(aud_output_type == AUD_OUTPUT_HDMI) {
		info_pb->out_dev=1;
	}
	else {
		info_pb->out_dev=0;
	}

	#if 0
		/* init hdal and allocate memory */
		ret = hd_common_init(1);
		if (ret != HD_OK) {
			printf("common init fail\n");
			goto exit;
		}
	#endif

	/* init audio decode and audio out modules */
	ret = init_module_pb();
	if (ret != HD_OK) {
		printf("[Playback] init moudle fail\n");
		return FALSE;
	}
	else {
		printf("[Playback] init moudle\n");
	}

	/* open audio decode and audio out modules */
	ret = open_module_pb(info_pb);
	if (ret != HD_OK) {
		printf("[Playback] open moudle fail\n");
		return FALSE;
	}
	else {
		printf("[Playback] open moudle\n");
	}

	/* setup runtime parameters */
	ret = set_module_param_pb(info_pb);
	if (ret != HD_OK) {
		printf("[Playback] set param fail\n");
		return FALSE;
	}
	else {
		printf("[Playback] set param\n");
	}

	/* bind audio playback: audio decode -> audio out */
	ret = bind_module_pb(info_pb);
	if (ret != HD_OK) {
		printf("[Playback] bind moudle fail\n");
		return FALSE;
	}
	else {
		printf("[Playback] bind moudle\n");
	}

	/* start to run */
	ret = start_module_pb(info_pb);
	if (ret != HD_OK) {
		printf("[Playback] start module fail\n");
		return FALSE;
	}
	else {
		printf("[Playback] start module\n");
	}

	return TRUE;
}

/**
 *  used to initial audio capture and audio encode modules.
 */
static HD_RESULT init_module_rec(void)
{
	HD_RESULT ret;
	if ((ret = hd_audiocap_init()) != HD_OK) {
		return ret;
	}
	if ((ret = hd_audioenc_init()) != HD_OK) {
		return ret;
	}
	return HD_OK;
}

/**
 *  used to un-initial audio capture and audio encode modules.
 */
static HD_RESULT uninit_module_rec(void)
{
	HD_RESULT ret;
	if ((ret = hd_audiocap_uninit()) != HD_OK) {
		return ret;
	}
	if ((ret = hd_audioenc_uninit()) != HD_OK) {
		return ret;
	}
	return HD_OK;
}

/**
 *  used to initial audio decode and audio out modules.
 */
static HD_RESULT init_module_pb(void)
{
	HD_RESULT ret;
	if ((ret = hd_audiodec_init()) != HD_OK) {
		return ret;
	}
	if ((ret = hd_audioout_init()) != HD_OK) {
		return ret;
	}
	return HD_OK;
}

/**
 *  used to un-initial audio decode and audio out modules.
 */
static HD_RESULT uninit_module_pb(void)
{
	HD_RESULT ret;
	if ((ret = hd_audiodec_uninit()) != HD_OK) {
		return ret;
	}
	if ((ret = hd_audioout_uninit()) != HD_OK) {
		return ret;
	}
	return HD_OK;
}

/**
 *  used to open audio capture and audio encode modules for later use.
 *  @p_rec_info: specify id pair to indicate the unique in and out channels.
 */
static HD_RESULT open_module_rec(AUDIO_RECORD *p_rec_info, int max_bitstream_num)
{
	INT i;
	HD_RESULT ret;
	for (i = 0; i < max_bitstream_num; i++) {
		if ((ret = hd_audiocap_open(HD_AUDIOCAP_IN(p_rec_info->in_dev, 0), HD_AUDIOCAP_OUT(p_rec_info->in_dev, i),
									&p_rec_info->audiocap_path[i])) != HD_OK) {
			return ret;
		}
		if ((ret = hd_audioenc_open(HD_AUDIOENC_IN(0, i), HD_AUDIOENC_OUT(0, i), &p_rec_info->audioenc_path[i])) != HD_OK) {
			return ret;
		}
	}
	return HD_OK;
}

/**
 *  used to close audio capture and audio encode modules when these modules are no longer needed.
 *  @p_rec_info: specify the audio capture and audio encode path which want to be closed.
 */
static HD_RESULT close_module_rec(AUDIO_RECORD *p_rec_info, int max_bitstream_num)
{
	INT i;
	HD_RESULT ret;
	for (i = 0; i < max_bitstream_num; i++) {
		if ((ret = hd_audiocap_close(p_rec_info->audiocap_path[i])) != HD_OK) {
			return ret;
		}
		if ((ret = hd_audioenc_close(p_rec_info->audioenc_path[i])) != HD_OK) {
			return ret;
		}
	}
	return HD_OK;
}

/**
 *  used to open audio decode and audio out modules for later use.
 *  @p_pb_info: specify id pair to indicate the unique in and out channels.
 */
static HD_RESULT open_module_pb(AUDIO_PLAYBACK *p_pb_info)
{
	HD_RESULT ret;
	if ((ret = hd_audiodec_open(HD_AUDIODEC_IN(0, 0), HD_AUDIODEC_OUT(0, 0), &p_pb_info->audiodec_path)) != HD_OK) {
		return ret;
	}
	if ((ret = hd_audioout_open(HD_AUDIOOUT_IN(p_pb_info->out_dev, 0), HD_AUDIOOUT_OUT(p_pb_info->out_dev, 0), 
								&p_pb_info->audioout_path)) != HD_OK) {
		return ret;
	}
	return HD_OK;
}

/**
 *  used to close audio decode and audio out modules when these modules are no longer needed.
 *  @p_pb_info: specify the audio decode and audio out path which want to be closed.
 */
static HD_RESULT close_module_pb(AUDIO_PLAYBACK *p_pb_info)
{
	HD_RESULT ret;
	if ((ret = hd_audiodec_close(p_pb_info->audiodec_path)) != HD_OK) {
		return ret;
	}
	if ((ret = hd_audioout_close(p_pb_info->audioout_path)) != HD_OK) {
		return ret;
	}
	return HD_OK;
}

/**
 *  used to setting the format of audio stream.
 *  @p_rec_info: specify the path of audio capture and audio encode modules which want to be configured.
 *  @ch: specify the audio channel which want to be configured.
 */
static HD_RESULT set_module_param_rec(AUDIO_RECORD *p_rec_info, UINT ch)
{
	HD_RESULT ret;
	HD_AUDIOCAP_IN audiocap_param;
	HD_AUDIOENC_OUT audioenc_param;
	HD_AUDIOCAP_DEV_CONFIG audiocap_config;
	UINT ddr_id;

// 오디오 비주기적으로 끊김이 발생하여 아래 코드를 막는다. ksi_test
#if 0
	memset(&audiocap_config, 0, sizeof(audiocap_config));
	audiocap_config.data_pool[0].mode = HD_AUDIOCAP_POOL_ENABLE;

	if ((ret = vendor_common_get_ddrid(HD_COMMON_MEM_AU_ENC_AU_GRAB_OUT_POOL, COMMON_PCIE_CHIP_RC, &ddr_id)) != HD_OK) {
		printf("vendor_common_get_ddrid pool_type(AU_ENC_AU_GRAB_OUT) fail, please check dts config\n");
		goto exit;
	}
	audiocap_config.data_pool[0].ddr_id = ddr_id;
#if 0 // org
	// audiocap_config.data_pool[0].frame_sample_size = 320 * 2;
	// audiocap_config.data_pool[0].counts = HD_AUDIOCAP_SET_COUNT(20, 0);
#else
// [  140.265955] fd(0x13000101) bs win_size is too small, please set max_bs_size to increase! (Need: 832bytes, Alloc: 816bytes).
// pif_send_multi_bitstreams:ioctl "VPD_PUT_COPY_MULTI_DIN" driver [  140.278285] fd(0x13000101) bs win_size is too small, please set max_bs_size to increase! (Need: 832bytes, Alloc: 816bytes).
// failed. errno(2)
// <send bitstream fail(-10)!>
// pif_send_multi_bitstreams:ioctl "VPD_PUT_COPY_MULTI_DIN" driver [  140.299336] fd(0x13000101) bs win_size is too small, please set max_bs_size to increase! (Need: 832bytes, Alloc: 816bytes).
// failed. errno(2)

	audiocap_config.data_pool[0].frame_sample_size = (408 * 2) + 16 ;
	audiocap_config.data_pool[0].counts = HD_AUDIOCAP_SET_COUNT(20, 0);
#endif
	ret = hd_audiocap_set(p_rec_info->audiocap_path[ch], HD_AUDIOCAP_PARAM_DEV_CONFIG, &audiocap_config);
	if (ret != HD_OK) {
		printf("hd_audiocap_set(HD_AUDIOCAP_PARAM_PATH_CONFIG) fail\n");
		goto exit;
	}
#endif
	/* set audiocap parameters */
	ret = hd_audiocap_get(p_rec_info->audiocap_path[ch], HD_AUDIOCAP_PARAM_IN, &audiocap_param);
	if (ret != HD_OK) {
		printf("hd_audiocap_get(HD_AUDIOCAP_PARAM_IN) fail\n");
		goto exit;
	}
	audiocap_param.sample_rate = HD_AUDIO_SR_8000;
	audiocap_param.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audiocap_param.mode = HD_AUDIO_SOUND_MODE_MONO;
	#if 0	// ORG
		audiocap_param.frame_sample = 320;  /* for 25fps: 8000/25=320 */	// audio frame size is 640byte
	#else	// Fix By pakkhman
		audiocap_param.frame_sample = 416;
	#endif
	ret = hd_audiocap_set(p_rec_info->audiocap_path[ch], HD_AUDIOCAP_PARAM_IN, &audiocap_param);
	if (ret != HD_OK) {
		printf("hd_audiocap_set(HD_AUDIOCAP_PARAM_IN) fail\n");
		goto exit;
	}

	/* set audioenc parameters */
	ret = hd_audioenc_get(p_rec_info->audioenc_path[ch], HD_AUDIOENC_PARAM_OUT, &audioenc_param);
	if (ret != HD_OK) {
		printf("hd_audioenc_get(HD_AUDIOENC_PARAM_OUT) fail\n");
		goto exit;
	}
	audioenc_param.codec_type = HD_AUDIO_CODEC_PCM;
	ret = hd_audioenc_set(p_rec_info->audioenc_path[ch], HD_AUDIOENC_PARAM_OUT, &audioenc_param);
	if (ret != HD_OK) {
		printf("hd_audioenc_set(HD_AUDIOENC_PARAM_OUT) fail\n");
		goto exit;
	}

exit:
	return ret;
}

/**
 *  used to setting the format of audio stream.
 *  @p_pb_info: specify the path of audio decode and audio out modules which want to be configured.
 */
static HD_RESULT set_module_param_pb(AUDIO_PLAYBACK *p_pb_info)
{
	HD_AUDIODEC_PATH_CONFIG audiodec_config;
	HD_AUDIODEC_IN audiodec_param;
	HD_AUDIOOUT_IN audioin_param;
	HD_AUDIOOUT_OUT audioout_param;
	UINT ddr_id;
	HD_RESULT ret;

	memset(&audiodec_config, 0, sizeof(audiodec_config));
	audiodec_config.data_pool[0].mode = HD_AUDIODEC_POOL_ENABLE;

	if ((ret = vendor_common_get_ddrid(HD_COMMON_MEM_AU_DEC_AU_RENDER_IN_POOL, COMMON_PCIE_CHIP_RC, &ddr_id)) != HD_OK) {
		printf("vendor_common_get_ddrid pool_type(AU_DEC_AU_RENDER_IN) fail, please check dts config\n");
		goto exit;
	}

	audiodec_config.data_pool[0].ddr_id = ddr_id;
	#if 0 //org
		audiodec_config.data_pool[0].frame_sample_size = 1024 * 2;
	#else	// ITX Fix
		// audiodec_config.data_pool[0].frame_sample_size = 320 * 2;
		audiodec_config.data_pool[0].frame_sample_size = 416 * 2;
	#endif
	audiodec_config.data_pool[0].counts = HD_AUDIODEC_SET_COUNT(10, 0);
	ret = hd_audiodec_set(p_pb_info->audiodec_path, HD_AUDIODEC_PARAM_PATH_CONFIG, &audiodec_config);
	if (ret != HD_OK) {
		printf("hd_audiodec_set(HD_AUDIODEC_PARAM_PATH_CONFIG) fail\n");
		goto exit;
	}

	/* set audiodec parameters */
	ret = hd_audiodec_get(p_pb_info->audiodec_path, HD_AUDIODEC_PARAM_IN, &audiodec_param);
	if (ret != HD_OK) {
		printf("hd_audiodec_get(HD_AUDIODEC_PARAM_IN) fail\n");
		goto exit;
	}
	audiodec_param.codec_type = HD_AUDIO_CODEC_PCM;
	audiodec_param.sample_rate = HD_AUDIO_SR_8000;
	audiodec_param.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audiodec_param.mode = HD_AUDIO_SOUND_MODE_MONO;
	ret = hd_audiodec_set(p_pb_info->audiodec_path, HD_AUDIODEC_PARAM_IN, &audiodec_param);
	if (ret != HD_OK) {
		printf("hd_audiodec_set(HD_AUDIODEC_PARAM_IN) fail\n");
		goto exit;
	}

	/*
	 * Set data sample rate
	 */
	audioin_param.sample_rate = HD_AUDIO_SR_8000;
	ret = hd_audioout_set(p_pb_info->audioout_path, HD_AUDIOOUT_PARAM_IN, &audioin_param);
	if (ret != HD_OK) {
		printf("hd_audiodec_set(HD_AUDIOOUT_PARAM_IN) fail\n");
		goto exit;
	}
	#if 0
		/*
		 * Set device sample rate
		 * You don't need to set it if you are using default value.
		 */
		ret = hd_audioout_get(p_pb_info->audioout_path, HD_AUDIOOUT_PARAM_OUT, &audioout_param);
		if (ret != HD_OK) {
			printf("hd_audioout_get(HD_AUDIOOUT_PARAM_OUT) fail\n");
			goto exit;
		}
		audioout_param.sample_rate = HD_AUDIO_SR_8000;
		audioout_param.sample_bit = HD_AUDIO_BIT_WIDTH_16;
		audioout_param.mode = HD_AUDIO_SOUND_MODE_MONO;
		ret = hd_audioout_set(p_pb_info->audioout_path, HD_AUDIOOUT_PARAM_OUT, &audioout_param);
		if (ret != HD_OK) {
			printf("hd_audiodec_set(HD_AUDIOOUT_PARAM_OUT) fail\n");
			goto exit;
		}
	#endif

exit:
	return ret;
}

/**
 * used to bind audio record: audiocap -> audioenc.
 * @p_rec_info: specify the path of audio capture and audio encoder modules which want to be binded.
 * @ch: specify the audio channel which want to be binded.
 */
static HD_RESULT bind_module_rec(AUDIO_RECORD *p_rec_info, UINT ch)
{
	HD_RESULT ret;
	ret = hd_audiocap_bind(HD_AUDIOCAP_OUT(p_rec_info->in_dev, ch), HD_AUDIOENC_IN(0, ch));
	return ret;
}

/**
 * used to unbind audio record: audiocap -> audioenc.
 * @p_rec_info: specify the path of audio capture and audio encoder modules which want to be unbinded.
 * @ch: specify the audio channel which want to be unbinded.
 */
static HD_RESULT unbind_module_rec(AUDIO_RECORD *p_rec_info, UINT ch)
{
	HD_RESULT ret;
	ret = hd_audiocap_unbind(HD_AUDIOCAP_OUT(p_rec_info->in_dev, ch));
	return ret;
}

/**
 * used to bind audio playback: audiodec -> audioout.
 * @p_pb_info: specify the path of audio decode and audio out modules which want to be binded.
 */
static HD_RESULT bind_module_pb(AUDIO_PLAYBACK *p_pb_info)
{
	HD_RESULT ret;
	ret = hd_audiodec_bind(HD_AUDIODEC_OUT(0, 0), HD_AUDIOOUT_IN(p_pb_info->out_dev, 0));
	return ret;
}

/**
 * used to unbind audio playback: audiodec -> audioout.
 */
static HD_RESULT unbind_module_pb(void)
{
	HD_RESULT ret;
	ret = hd_audiodec_unbind(HD_AUDIODEC_OUT(0, 0));
	return ret;
}

/**
 * used to start modules and record from audio capture.
 * @p_rec_info: specify the path of audio capture and audio encoder modules which want to do audio record demostration.
 * @ch: specify the audio channel which want to be binded.
 */
static HD_RESULT start_module_rec(AUDIO_RECORD *p_rec_info, UINT ch)
{
	HD_RESULT ret;

	ret = hd_audiocap_start(p_rec_info->audiocap_path[ch]);
	if (ret != HD_OK) {
		printf("start audiocap fail\n");
		return ret;
	}
	ret = hd_audioenc_start(p_rec_info->audioenc_path[ch]);
	if (ret != HD_OK) {
		printf("start audioenc fail\n");
		return ret;
	}

	return ret;
}

/**
 * used to stop modules.
 * @p_rec_info: specify the path of audio capture and audio encoder modules which want to be stopped.
 * @ch: specify the audio channel which want to be stopped.
 */
static HD_RESULT stop_module_rec(AUDIO_RECORD *p_rec_info, UINT ch)
{
	HD_RESULT ret = HD_OK, ret1, ret2;

	ret1 = hd_audiocap_stop(p_rec_info->audiocap_path[ch]);
	if (ret1 != HD_OK) {
		printf("stop audiocap fail\n");
		ret = ret1;
	}
	ret2 = hd_audioenc_stop(p_rec_info->audioenc_path[ch]);
	if (ret2 != HD_OK) {
		printf("stop audioenc fail\n");
		ret = ret2;
	}

	return ret;
}

/**
 * used to start modules and playback the music decoded from audio decode.
 * @p_pb_info: specify the path of audio decode and audio out modules which want to do audio playback demostration.
 */
static HD_RESULT start_module_pb(AUDIO_PLAYBACK *p_pb_info)
{
	HD_RESULT ret;

	ret = hd_audiodec_start(p_pb_info->audiodec_path);
	if (ret != HD_OK) {
		printf("start audiodec fail\n");
		return ret;
	}
	ret = hd_audioout_start(p_pb_info->audioout_path);
	if (ret != HD_OK) {
		printf("start audioout fail\n");
		return ret;
	}

	return ret;
}

/**
 * used to stop modules.
 * @p_pb_info: specify the path of audio decode and audio out modules which want to be stopped.
 */
static HD_RESULT stop_module_pb(AUDIO_PLAYBACK *p_pb_info)
{
	HD_RESULT ret = HD_OK, ret1, ret2;

	ret1 = hd_audiodec_stop(p_pb_info->audiodec_path);
	if (ret1 != HD_OK) {
		printf("stop audiodec fail\n");
		ret = ret1;
	}
	ret2 = hd_audioout_stop(p_pb_info->audioout_path);
	if (ret2 != HD_OK) {
		printf("stop audioout fail\n");
		ret = ret2;
	}

	return ret;
}

gboolean nf_audio_nvt_set_pb_out_mode(AUDIO_PLAYBACK *p_pb_info, gboolean is_hdmi)
{
	HD_RESULT ret;

	ret=stop_module_pb(p_pb_info);
	if (ret != HD_OK) {
		printf("[Playback] stop moudle fail\n");
		return FALSE;
	}
	else {
		printf("[Playback] stop moudle\n");
	}

	ret=unbind_module_pb();
	if (ret != HD_OK) {
		printf("[Playback] unbind moudle fail\n");
		return FALSE;
	}
	else {
		printf("[Playback] unbind moudle\n");
	}

	close_module_pb(p_pb_info);
	if (ret != HD_OK) {
		printf("[Playback] close moudle fail\n");
		return FALSE;
	}
	else {
		printf("[Playback] close moudle\n");
	}

	if(is_hdmi) {	// HDMI
		p_pb_info->out_dev=1;
	}
	else {			// RCA
		p_pb_info->out_dev=0;
	}

	ret=open_module_pb(p_pb_info);
	if (ret != HD_OK) {
		printf("[Playback] open moudle fail\n");
		return FALSE;
	}
	else {
		printf("[Playback] open moudle\n");
	}

	ret=set_module_param_pb(p_pb_info);
	if (ret != HD_OK) {
		printf("[Playback] set param fail\n");
		return FALSE;
	}
	else {
		printf("[Playback] set param\n");
	}

	bind_module_pb(p_pb_info);
	if (ret != HD_OK) {
		printf("[Playback] bind moudle fail\n");
		return FALSE;
	}
	else {
		printf("[Playback] bind moudle\n");
	}

	start_module_pb(p_pb_info);
	if (ret != HD_OK) {
		printf("[Playback] start module fail\n");
		return FALSE;
	}
	else {
		printf("[Playback] start module\n");
	}
}

gboolean nf_audio_nvt_set_volume_intput(AUDIO_RECORD *info_rec, guint volume)
{
#if 0
	HD_RESULT ret = HD_OK;
	HD_AUDIOCAP_VOLUME audio_vol_param={0};

	// ALC gain:0~100(only 9 levels, 0, 1~11, 12~23,24~35, 36~47, 48~59, 60~71,72~~83,84~100). 
	// Digital gain:101~160, 0.5dB gain for 1 step (analog noise will be enlarge)
	audio_vol_param.volume = volume;

	ret=hd_audiocap_set(info_rec->cap_ctrl, HD_AUDIOCAP_PARAM_VOLUME, &audio_vol_param);
	if(ret == HD_OK) {
		hd_audiocap_start(info_rec->cap_path);
		return TRUE;
	}
	else {
		g_warning("Audio Set Input Volume Fail!!");
		return FALSE;
	}
#else
	return TRUE;
#endif
}

gboolean nf_audio_nvt_set_volume_output(AUDIO_PLAYBACK *info_pb, guint volume)
{
#if 0
	HD_RESULT ret = HD_OK;
	HD_AUDIOOUT_VOLUME audio_out_vol = {0};

	// set hd_audioout volume
	audio_out_vol.volume = volume;
	ret = hd_audioout_set(info_pb->out_ctrl, HD_AUDIOOUT_PARAM_VOLUME, &audio_out_vol);
	if(ret == HD_OK) {
		hd_audiodec_start(info_pb->audiodec_path);
		hd_audioout_start(info_pb->audioout_path);
		return TRUE;
	}
	else {
		g_warning("Audio Set Output Volume Fail!!");
		return FALSE;
	}
#else
	return TRUE;
#endif
}

