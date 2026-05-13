/*
 * scm_audio.c
 * 	- audio services
 *	- dependency :
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Apr 6, 2011
 *
 */

#include "scm.h"
#include "iux_afx.h"
#include "nf_util_device.h"
#include "nf_api_ipcam.h"
#if defined(_IPX_MODEL_UX)
#include "nf_ipcam_defs.h"
#else
#include "ipx_cam_api.h"
#endif
#include "var.h"
#include "scm_internal.h"
#include "nfdal.h"
#include "iux_types.h"
#include "ssm.h"

#define DBG_LEVEL		9
#define DBG_MODULE		"SCM_AUDIO"




////////////////////////////////////////////////////////////
//
// private data type 
//


////////////////////////////////////////////////////////////
//
// private functions
//

////////////////////////////////////////////////////////////
//
// protected interfaces
//

int _scm_init_audio(SCM_T *piscm)
{
	AudioData ad;
	DAL_get_audio_data(&ad);
	var_set_live_audio_ch(ad.liveAudio);
	var_set_mic_state(ONOFF_OFF);
	var_set_mic_out_mask(0x00);
	return 0;
}

int _scm_apply_live_audio_onoff()
{
    if (!DAL_get_support_audio()) 
    {
        scm_turnoff_live_audio();
        return -1;
    }
    
    if (!ssm_check_access_auth(USR_AUTH_AUDIO)) 
    {
        scm_turnoff_live_audio();    
        return -1;
    }

    scm_turnon_live_audio();

    return 0;
}

int _scm_apply_live_audio_ch()
{
	AudioData ad;
	
	DAL_get_audio_data(&ad);
	scm_change_live_audio(ad.liveAudio);
	
	return 0;
}

int _scm_apply_buzzer_config()
{
	BuzzerData bd;
	DAL_get_buzzer_data(&bd);

#if defined(USE_DEV_REMOCON)	
	if(bd.remote == 1) nf_dev_remocon_beep_on();	
	else 				 nf_dev_remocon_beep_off();	
#endif

#if defined(USE_DEV_KEYPAD)	
	if(bd.keypad == 1) nf_dev_keypad_beep_on();
	else 				 nf_dev_keypad_beep_off();
#endif		

	return 0;
}

////////////////////////////////////////////////////////////
//
// public interfaces
//

AUDIO_INFO_T *scm_new_audio_info()
{

}

int scm_free_audio_info(AUDIO_INFO_T *audio)
{

}

int scm_turnoff_live_audio()
{
	DMSG(1, "");
#if defined(USE_DEV_AUDIO)
	nf_dev_audio_set_dac(0xff); //nf_live_set_live_audio_ch(0xff);
#endif
	scm_req_audio_ch_event_data();
	var_set_live_audio_stat(0);

	return 0;
}

int scm_turnon_live_audio()
{
	int ch = var_get_live_audio_ch();

	DMSG(1, "");
    if (!DAL_get_support_audio()) return -1;
    if (!ssm_check_access_auth(USR_AUTH_AUDIO)) return -1;

#if defined(USE_DEV_AUDIO)
	nf_dev_audio_set_dac(ch); //nf_live_set_live_audio_ch(ch);
#endif
	scm_req_audio_ch_event_data();
	var_set_live_audio_stat(1);

	return 0;
}

int scm_change_live_audio(int ch)
{
	DMSG(1, "");
	if (var_get_live_audio_stat() == 0) return -1;
    if (var_get_live_audio_ch() == ch) return -1;
	
#if defined(USE_DEV_AUDIO)
   	nf_dev_audio_set_dac(ch);
#endif	
	var_set_live_audio_ch(ch);
	scm_req_audio_ch_event_data();

	return 0;
}

int scm_get_cur_live_audio_ch()
{
	return var_get_live_audio_ch();
}

ONOFF_E scm_get_mic_state()
{
	return var_get_mic_state();
}

int scm_turnon_mic()
{
	var_set_mic_state(ONOFF_ON);
	nf_dev_mic_set_onoff(TRUE);
}

int scm_turnoff_mic()
{
	var_set_mic_state(ONOFF_OFF);
	nf_dev_mic_set_onoff(FALSE);
	return 0;
}

int scm_enable_mic_ch(char mic_stat[], int ch_count)
{
	int i;
	BITMASK mask = 0;

	for (i = 0; i < ch_count; ++i) mask |= (mic_stat[i] ? 1 : 0) << i;
	var_set_mic_out_mask(mask);
	nf_dev_mic_set_output_mask(mask);	
	scm_req_mic_out_event_data();
	return 0;
}

int scm_get_enabled_mic_ch(char buf[], int buf_size)
{
	int i;
	BITMASK mask = 0;
	int cnt = var_get_ch_count();

	memset(buf, 0x00, buf_size);
	if (buf_size < cnt) return -1;
	mask = var_get_mic_out_mask();
	for (i = 0; i < cnt; ++i) buf[i] = mask & (1 << i);
	return 0;
}

int scm_get_audio_in_supp(int ch)
{
	int i;
	int cnt = var_get_ch_count();
	int ret;
	unsigned int val = 0;

	ret = nf_ipcam_get_supported_func(ch, &val, NULL);

	if (ret == IPCAM_SETUP_RTN_DONE) {
		if (val & NF_IPCAM_FUNC_AUDIO_TX) return 0;
	}
	
	return -1;
}

int scm_get_mic_out_supp(int ch)
{
	int i;
	int cnt = var_get_ch_count();
	int ret;
	unsigned int val = 0;

	ret = nf_ipcam_get_supported_func(ch, &val, NULL);

	if (ret == IPCAM_SETUP_RTN_DONE) {
		if (val & NF_IPCAM_FUNC_AUDIO_RX) return 0;
	}
	
	return -1;
}

