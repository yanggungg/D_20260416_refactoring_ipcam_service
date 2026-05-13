void nf_issm_ctl_bitrate_init();
void nf_issm_ctl_bitrate_close();

void nf_issm_ctl_get_bitrate_control(int p_i, int p_j, int p_bitrate_step, void *p_data);

int nf_issm_cb_noti_bitrate_ud(void *p_data);
void nf_issm_ctl_set_is_bandwidth_ctl_enable(int p_is_enable);
void nf_issm_ctl_set_max_bandwidth(int p_max_bandwidth);
void nf_issm_ctl_bitrate_set_session_changed();


enum {
	NF_STREAM_IDX_AUTO			= 0,
	NF_STREAM_IDX_MAIN			= 1,
	NF_STREAM_IDX_SECOND		= 2,
	NF_STREAM_IDX_END			= 3,
};
