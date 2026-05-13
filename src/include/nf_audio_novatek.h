#ifndef __NF_AUDIO_NOVATEC_H__
#define __NF_AUDIO_NOVATEC_H__

#if defined(CHIP_NVT_NA51039)

	#define BITSTREAM_LEN		12800
	#define USE_PULL			0

	typedef struct _AUDIO_RECORD {
		UINT is_exit;
		HD_PATH_ID *audiocap_path;  /* audio capture */
		HD_PATH_ID *audioenc_path;  /* audio encode */
		UINT in_dev;
	} AUDIO_RECORD;

	typedef struct _AUDIO_PLAYBACK {
		UINT is_exit;
		HD_PATH_ID audiodec_path;
		HD_PATH_ID audioout_path;
		UINT out_dev;                   /* 0:Front-End DAC  1:ADDA301  2:HDMI */
	} AUDIO_PLAYBACK;

	gboolean nf_audio_init_lib(AUDIO_RECORD *info_rec, AUDIO_PLAYBACK *info_pb, 
								guint enc_type, guint dec_type, guint aud_output_type, gint audio_nr);
	gboolean nvt_audio_get_mmap_pcm(HD_COMMON_MEM_VB_BLK *pcm_blk, UINT32 *pcm_addr, 
									CHAR **pcm_data, int max_bitstream_num);
	gboolean nvt_audio_get_mmap_bitstream(HD_COMMON_MEM_VB_BLK *bitstream_blk, UINT32 *bitstream_addr, 
											CHAR **bitstream_data, int max_bitstream_num);
	gboolean nf_audio_nvt_set_pb_out_mode(AUDIO_PLAYBACK *p_pb_info, gboolean is_hdmi);
	gboolean nf_audio_nvt_set_volume_intput(AUDIO_RECORD *info_rec, guint volume);
	gboolean nf_audio_nvt_set_volume_output(AUDIO_PLAYBACK *info_pb, guint volume);
#elif defined(CHIP_NVT_NT9833x)

	#define BITSTREAM_LEN		12800
	#define USE_PULL			0

	typedef struct _AUDIO_RECORD {
		UINT is_exit;
		HD_PATH_ID *audiocap_path;  /* audio capture */
		HD_PATH_ID *audioenc_path;  /* audio encode */
		UINT in_dev;
	} AUDIO_RECORD;

	typedef struct _AUDIO_PLAYBACK {
		UINT is_exit;
		UINT is_pause;
		HD_PATH_ID audiodec_path;
		HD_PATH_ID audioout_path;
		UINT out_dev;                   /* 0:Front-End DAC  1:ADDA301  2:HDMI */
	} AUDIO_PLAYBACK;

	gboolean nf_audio_init_lib(AUDIO_RECORD *info_rec, AUDIO_PLAYBACK *info_pb, 
								guint enc_type, guint dec_type, guint aud_output_type, gint audio_nr);
	gboolean nf_audio_nvt_set_pb_out_mode(AUDIO_PLAYBACK *p_pb_info, gboolean is_hdmi);
	gboolean nf_audio_nvt_set_volume_intput(AUDIO_RECORD *info_rec, guint volume);
	gboolean nf_audio_nvt_set_volume_output(AUDIO_PLAYBACK *info_pb, guint volume);

#else
typedef struct _AUDIO_RECORD {

	// (1) audio cap
	HD_PATH_ID cap_ctrl;
	HD_PATH_ID cap_path;

	// (2) audio enc
	HD_PATH_ID enc_path;
	UINT32 enc_type;

	// (3) user pull
	pthread_t  enc_thread_id;
	UINT32     enc_exit;
	UINT32     flow_start;

} AUDIO_RECORD;

typedef struct _AUDIO_CAPONLY {
	HD_AUDIO_SR sample_rate_max;
	HD_AUDIO_SR sample_rate;

	HD_PATH_ID cap_ctrl;
	HD_PATH_ID cap_path;

	UINT32 cap_exit;
	UINT32 flow_start;
} AUDIO_CAPONLY;

typedef struct _AUDIO_PLAYBACK {

	// (1) audio dec
	HD_PATH_ID dec_path;
	UINT32 dec_type;

	// (2) audio out
	HD_PATH_ID out_ctrl;
	HD_PATH_ID out_path;

	// (3) user push
	pthread_t  dec_thread_id;
	UINT32     dec_exit;
	UINT32     flow_start;

} AUDIO_PLAYBACK;

gboolean nf_audio_init_lib(AUDIO_CAPONLY *pcaponly, AUDIO_PLAYBACK *pstream_pb, guint enc_type, guint dec_type, guint aud_output_type);
gboolean nf_audio_init_parameter(AUDIO_RECORD *p_stream0);
gboolean nvt_audio_get_env(AUDIO_RECORD *p_stream0, HD_AUDIOENC_BUFINFO *phy_buf_main, UINT32 *vir_addr_main);
gboolean nvt_audio_mem_unmap(HD_AUDIOCAP_BUFINFO *phy_buf_main, UINT32 vir_addr_main);
gboolean nvt_audio_start_enc_cap(AUDIO_RECORD *p_stream0);
gboolean nvt_audio_cap_get(AUDIO_CAPONLY *p_cap_only, HD_AUDIOCAP_BUFINFO *phy_buf_main);
gboolean nvt_audio_get_mmap_buffer(HD_AUDIOCAP_BUFINFO *phy_buf_main, unsigned long *vir_addr_main);
gboolean nvt_audio_start_cap(AUDIO_CAPONLY *caponly);
gboolean nf_audio_nvt_set_volume_intput(AUDIO_CAPONLY *pcaponly, guint volume);
gboolean nf_audio_nvt_set_volume_output(AUDIO_PLAYBACK *pstream_pb, guint volume);
#endif

#endif

