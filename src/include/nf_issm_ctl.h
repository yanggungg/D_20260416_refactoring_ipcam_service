#ifndef __NF_ISSM_CTL__
#define __NF_ISSM_CTL__


#define MAX_PLAY_AUDIO_LENGTH	(800)
#if NUM_ACTIVE_CH <= 16
	#define MAX_ISSM_V_CH		(16)
	#define MAX_ISSM_A_CH		(16)
	#define MAX_SST_CHANNEL		(16*4)	// v-main/second, audio, meta
	#define MAX_ISSM_CH			(32)
	#define MAX_ISSM_CH_MASK	(0xffffffff)
#else
	#define MAX_ISSM_V_CH		(32)
	#define MAX_ISSM_A_CH		(32)
	#define MAX_SST_CHANNEL		(32*4)	// v-main/second, audio, meta
	#define MAX_ISSM_CH			(64)
	#define MAX_ISSM_CH_MASK	(0xffffffffffffffff)
#endif

#define AUTO_STREAM		0
#define MAIN_STREAM		1
#define SECOND_STREAM	2

enum NF_ISSM_CTL_CMD_TYPE {
	ISSM_NONE = 0,
	ISSM_START,
	ISSM_STOP,
	ISSM_END
};


typedef struct _ISSM_EP_INFO {
	int ep_id;
	int v_track_type;
	int v_track_id;
	int a_track_type;
	int a_track_id;
	int m_track_type;
	int m_track_id;
	int stream_idx;
	int ch_num;
} ISSM_EP_INFO;

void nf_issm_ctl_init();
void nf_issm_ctl(int p_issm_cmd, char *p_log_str);
int nf_issm_ctl_get_session_count();
int nf_issm_ctl_disconnect_session(int p_sess_id);
void *nf_issm_ctl_get_ep_entry(int p_idx);
void nf_issm_ctl_refresh_ep(int p_ep_id);
ISSM_EP_INFO nf_issm_ctl_get_ep_info(int p_ep_id);
void nf_issm_ctl_noti_nvr_mouse_move();
void nf_issm_ctl_set_restart_checker();
int nf_issm_ctl_get_restart_checker();

int nf_issm_ctl_mcast_start(int p_ch_num, int p_is_ipv6);
int nf_issm_ctl_mcast_stop(int p_ch_num);

extern int g_issm_ep_entry_id[MAX_ISSM_CH];
extern int g_issm_track_v_h264_id[MAX_ISSM_CH];
extern int g_issm_track_v_h265_id[MAX_ISSM_CH];
extern int g_issm_track_v_mjpeg_id[MAX_ISSM_CH];
extern int g_issm_track_a_mulaw_id[MAX_ISSM_CH];
extern int g_issm_track_m_meta_id[MAX_ISSM_CH];
extern int g_issm_itx_ep_entry_id[2];
extern int g_issm_itx_track_id[2];
extern int g_issm_zero_ep_entry_id;
extern int g_issm_zero_track_id;

#endif

