#ifndef __NF_UTIL_DEVICE_H__
#define __NF_UTIL_DEVICE_H__

#include <glib.h>

/**********************************

  Device for SNF model

***********************************/
#define USE_DEV_KEYPAD
//#define USE_DEV_JOG
//#define USE_DEV_SHUTTLE
#if !defined(CHIP_NVT)
	#define USE_DEV_REMOCON
#endif
#define USE_DEV_ALARM_IN
#define USE_DEV_ALARM_OUT
#define USE_DEV_BOARD_PP
#define USE_DEV_WATCHDOG

// #if defined(_ANF8G_1648D)
//	#define USE_DEV_DECODER //ksi_test 신규 HW 나오면 decoder define 제거 해야 함.
//	#define USE_DEV_DECODER_MINOR1
	#define USE_DEV_AUDIO
	#define USE_DEV_EDID   

//	#define USE_DEV_DRV_EXT
	#define USE_DEV_MICOM
	#define USE_DEV_POE
	#define USE_DEV_SWITCH
// #endif

	#define USE_DEV_IP804
	#define DEVNAME_POE DEVNAME_IP804
	#define DEV_FD_IDX_POE	DEV_FD_IDX_IP804
	#define DEV_FD_IDX_SWITCH	DEV_FD_IDX_IP175D

#define USE_DEV_MIC
/******************************************************************************/

// dev list    http://nf.intellix.co.kr/phpBB/viewtopic.php?t=444

#define	DEVNAME_DECODER			"/dev/decoder"
#define	DEVNAME_DECODER_MINOR1	"/dev/decoder_minor1"
#define	DEVNAME_DECODER_MINOR2	"/dev/decoder_minor2"
#define	DEVNAME_DECODER_MINOR3	"/dev/decoder_minor3"
#define	DEVNAME_TWAUDIO			"/dev/twaudio"
#define	DEVNAME_AM8816			"/dev/am8816"
#define	DEVNAME_KEYPAD 			"/dev/keypad"
#define	DEVNAME_JOG 			"/dev/jog"
#define	DEVNAME_SHUTTLE 		"/dev/shuttle"
#if defined(CHIP_NVT)
	#if defined(CHIP_NVT_NA51039) || defined(CHIP_NVT_NT9833x)
	#define	DEVNAME_REMOCON			"/dev/input/event0"
	#else
	#define	DEVNAME_REMOCON			"/dev/irda_dev"
	#endif
#else
#define	DEVNAME_REMOCON			"/dev/remocon"
#endif
#define	DEVNAME_ALARM_IN 		"/dev/alarm_in"
#define	DEVNAME_ALARM_OUT 		"/dev/alarm_out"
#define	DEVNAME_BOARD_PP		"/dev/board_pp"
#define DEVNAME_WATCHDOG		"/dev/watchdog"
#define DEVNAME_RS485			"/dev/fpga_rs485"
#define DEVNAME_FS1648			"/dev/fs1648"
#define DEVNAME_SPOT_ANF		"/dev/drv_spot"
#define DEVNAME_SOLO_VIN		"/dev/solo6110_vin0"
#define DEVNAME_GENNUM			"/dev/gennum"
#define DEVNAME_GENNUM_MINOR1	"/dev/gennum_minor1"
#define DEVNAME_TPS2384			"/dev/tps2384"
#define DEVNAME_TLV320AIC23		"/dev/tlv320aic23"
#define DEVNAME_EDID			"/dev/edid"
#define DEVNAME_DSP				"/dev/dsp"
#define DEVNAME_IVR_HF			"/dev/ivr_hf"
#define DEVNAME_IVR_HF_MINOR1	"/dev/ivr_hf_minor1"
#define DEVNAME_TW2828			"/dev/tw2828"
#define DEVNAME_DRV_EXT			"/dev/drv_ext"
#define DEVNAME_MICOM			"/dev/MICOM"
#define DEVNAME_IP804			"/dev/ip804"
#define DEV_FD_MAX  			29


// function definition
gint nf_dev_open_decoder(void);
gint nf_dev_open_decoder_minor1(void);
gint nf_dev_open_decoder_minor2(void);
gint nf_dev_open_decoder_minor3(void);
gint nf_dev_open_twaudio(void);
gint nf_dev_open_am8816(void);
gint nf_dev_open_keypad(void);
gint nf_dev_open_jog(void);
gint nf_dev_open_shuttle(void);
gint nf_dev_open_remocon(void);
gint nf_dev_open_sensor(void);
gint nf_dev_open_relay(void);
gint nf_dev_open_board_pp(void);
gint nf_dev_open_watchdog(void);
gint nf_dev_open_rs485(void);
gint nf_dev_open_fs1648(void);
gint nf_dev_open_spot_anf(void);
gint nf_dev_open_solo_vin(void);
gint nf_dev_open_dsp_read(void);
gint nf_dev_open_dsp_write(void);
gint nf_dev_open_edid(void);
gint nf_dev_open_drv_ext(void);

gint nf_dev_open_ivr_hf(void);
gint nf_dev_open_ivr_hf_minor1(void);

#if defined(USE_DEV_FS1648)
	#include "fs1648.h"
	typedef struct fs1648_chg_rip_reg RIP_REG_T;
#endif /* defined(USE_DEV_FS1648) */

#if  defined(USE_DEV_CD22M3492)
	#include "drv_spot.h" // makefile�� include DIR �߰��ؾ���
#endif /* defined(USE_DEV_ANALOG_SPOT) */

typedef enum _DEV_FD_IDX_E
{
	DEV_FD_IDX_DECODER		= 0,
	DEV_FD_IDX_TWAUDIO		= 1,
	DEV_FD_IDX_AM8816		= 2,
	DEV_FD_IDX_KEYPAD		= 3,
	DEV_FD_IDX_JOG			= 4,
	DEV_FD_IDX_SHUTTLE		= 5,
	DEV_FD_IDX_REMOCON		= 6,
	DEV_FD_IDX_ALARM_IN		= 7,
	DEV_FD_IDX_ALARM_OUT	= 8,
	DEV_FD_IDX_BOARD_PP		= 9,
	DEV_FD_IDX_WATCHDOG		= 10,
	DEV_FD_IDX_RS485		= 11,
	DEV_FD_IDX_FS1648		= 12,
	DEV_FD_IDX_SPOT_ANF		= 13,
	DEV_FD_IDX_SOLO_VIN		= 14,
	DEV_FD_IDX_GENNUM		= 15,
	DEV_FD_IDX_TPS2384		= 16,
	DEV_FD_IDX_TLV320AIC23	= 17,
	DEV_FD_IDX_GENNUM_MINOR1= 18,
	DEV_FD_IDX_DSP_READ		= 19,
	DEV_FD_IDX_DSP_WRITE	= 20,
	DEV_FD_IDX_IVR_HF	= 21,
	DEV_FD_IDX_IVR_HF_MINOR1	= 22,
	DEV_FD_IDX_DECODER_MINOR1	= 23,
	DEV_FD_IDX_DECODER_MINOR2	= 24,
	DEV_FD_IDX_DECODER_MINOR3	= 25,
	DEV_FD_IDX_TW2828	= 26,
	DEV_FD_IDX_EDID		= 27,
	DEV_FD_IDX_DRV_EXT		= 28,
	DEV_FD_IDX_MICOM		= 29,
	DEV_FD_IDX_IP804		= 30,
	DEV_FD_IDX_IP175D		= 31,
	
	DEV_FD_IDX_NR		
} DEV_FD_IDX_E;


#if defined(USE_DEV_AM8816)
typedef enum _AM8816_SPOT_CHANNEL_INFO_E
{
	AM8816_SPOT_CH1	= 0,
	AM8816_SPOT_CH2	= 1,
	AM8816_SPOT_CH3	= 2,
	AM8816_SPOT_CH4	= 3,
	AM8816_SPOT_CH_NR
}AM8816_SPOT_CHANNEL_INFO_E;

typedef enum _AM8816_SPOT_SCR_MODE_INFO_E
{
	AM8816_SPOT_QUAD = 0,
	AM8816_SPOT_FULL = 1
}AM8816_SPOT_SCR_MODE_INFO_E;

typedef enum _AM8816_SPOT_BORDER_COLOR_INFO_E
{
	AM8816_SPOT_COLOR_WHITE		= 0,
	AM8816_SPOT_COLOR_YELLOW	= 1,
	AM8816_SPOT_COLOR_CYAN		= 2,
	AM8816_SPOT_COLOR_GREEN		= 3,
	AM8816_SPOT_COLOR_MAGENTA	= 4,
	AM8816_SPOT_COLOR_RED		= 5,
	AM8816_SPOT_COLOR_BLUE		= 6,
	AM8816_SPOT_COLOR_BLACK		= 7,
	AM8816_SPOT_COLOR_NR
}AM8816_SPOT_BORDER_COLOR_INFO_E;

#define	AM8816_BUFFER_MAX		16
#define AM8816_CH_MAX_NUM		4

typedef struct _AM8816_SPOT_INFO_T
{
	guint	spot_num;
	guint	seq_cnt;
	guchar	full_quad[AM8816_BUFFER_MAX];
	guchar	change_time[AM8816_BUFFER_MAX];
	guchar	channel[AM8816_BUFFER_MAX][AM8816_CH_MAX_NUM];
	guchar	enable;
}AM8816_SPOT_INFO;

typedef struct _AM8816_SPOT_TIELE_NAME_T {
	guint spot_num;
	guchar title[10];;
}AM8816_SPOT_TIELE_NAME;

#endif /* defined(USE_DEV_AM8816) */


/************************************************

  MIC

************************************************/
#if defined(USE_DEV_MIC)

guint nf_dev_mic_get_output_mask();
gboolean nf_dev_mic_set_output_mask( guint mask );

gboolean nf_dev_mic_get_onoff();
gboolean nf_dev_mic_set_onoff( gboolean onoff );

#endif


/************************************************

  AUIDO

************************************************/
#if defined(USE_DEV_AUDIO)

#define NF_AUDIO_DAC_PLAYBACK	(0xff)
#define NF_AUDIO_DAC_NET_AUDIO	(0xfe)

#define NF_AUDIO_DAC_NET_AUDIO_ON	(0xf1)
#define NF_AUDIO_DAC_NET_AUDIO_OFF	(0xf0)

guint nf_dev_audio_get_dac(void);
gboolean nf_dev_audio_set_dac( guint ch );
gboolean nf_dev_audio_set_netrx( gboolean on_off);

#endif



/************************************************

  DECODER
  TW2864 , TWAUDIO , NVP1108B

************************************************/
#if defined(USE_DEV_DECODER)

	/** Define Decoder Name **/

	#define USE_DEV_DECODER_TP2834
	
	#if defined(USE_DEV_DECODER_IDEN1100)
		#define DECODER_CTRL_PLAYBACK			0x43
		typedef enum _DECODER_PTZ_MODE_BIT_E
		{
			DECODER_PTZ_MODE_BIT16 		= 0,
			DECODER_PTZ_MODE_BIT32 		= 1
		} DECODER_PTZ_MODE_BIT;

		typedef enum _DECODER_PTZ_COMMAND_E
		{
			DECODER_PTZ_ZOOM_WIDE		= 0x00,
			DECODER_PTZ_ZOOM_TELE		= 0x01,
			DECODER_PTZ_PAN_LEFT		= 0x02,
			DECODER_PTZ_PAN_RIGHT		= 0x03,
			DECODER_PTZ_TILT_UP			= 0x04,
			DECODER_PTZ_TILT_DOWN		= 0x05,
			DECODER_PTZ_IRIS_OPEN		= 0x06,
			DECODER_PTZ_IRIS_CLOSE		= 0x07,
			DECODER_PTZ_FOCUS_FAR		= 0x08,
			DECODER_PTZ_FOCUS_NEAR		= 0x09,
			DECODER_PTZ_STOP			= 0x0A,
			DECODER_PTZ_LEFTUP			= 0x0B,
			DECODER_PTZ_LEFTDOWN		= 0x0C,
			DECODER_PTZ_RIGHTUP			= 0x0D,
			DECODER_PTZ_RIGHTDOWN		= 0x0E,
			DECODER_PTZ_SET_PRESET		= 0x0F,
			DECODER_PTZ_CLEAR_PRESET	= 0x10,
			DECODER_PTZ_GOTO_PRESET		= 0x11,
			DECODER_PTZ_AUTO_FOCUS		= 0x12,
			DECODER_PTZ_AUTO_IRIS		= 0x13,
			DECODER_PTZ_ZOOM_STOP		= 0x1A,
			DECODER_PTZ_NR				
		} DECODER_PTZ_COMMAND;
	#elif defined(USE_DEV_DECODER_NVP191X) || defined(USE_DEV_DECODER_NVP6124)
		typedef enum _DECODER_PTZ_COMMAND_E
		{
			DECODER_PTZ_STOP			= 0,
			DECODER_PTZ_TILT_UP			= 1,
			DECODER_PTZ_TILT_DOWN		= 2,
			DECODER_PTZ_PAN_LEFT		= 3,
			DECODER_PTZ_PAN_RIGHT		= 4,
			DECODER_PTZ_LEFTUP			= 5,
			DECODER_PTZ_LEFTDOWN		= 6,
			DECODER_PTZ_RIGHTUP			= 7,
			DECODER_PTZ_RIGHTDOWN		= 8,
			DECODER_PTZ_IRIS_OPEN		= 9,
			DECODER_PTZ_IRIS_CLOSE		= 10,
			DECODER_PTZ_FOCUS_NEAR		= 11,
			DECODER_PTZ_FOCUS_FAR		= 12,
			DECODER_PTZ_ZOOM_WIDE		= 13,
			DECODER_PTZ_ZOOM_TELE		= 14,
			DECODER_PTZ_SET_PRESET		= 15,
			DECODER_PTZ_GOTO_PRESET		= 16,
			DECODER_PTZ_NR				
		} DECODER_PTZ_COMMAND;
	#else		// because build error!
		typedef enum _DECODER_PTZ_COMMAND_E
		{
			DECODER_PTZ_STOP			= 0,
			DECODER_PTZ_TILT_UP			= 1,
			DECODER_PTZ_TILT_DOWN		= 2,
			DECODER_PTZ_PAN_LEFT		= 3,
			DECODER_PTZ_PAN_RIGHT		= 4,
			DECODER_PTZ_LEFTUP			= 5,
			DECODER_PTZ_LEFTDOWN		= 6,
			DECODER_PTZ_RIGHTUP			= 7,
			DECODER_PTZ_RIGHTDOWN		= 8,
			DECODER_PTZ_IRIS_OPEN		= 9,
			DECODER_PTZ_IRIS_CLOSE		= 10,
			DECODER_PTZ_FOCUS_NEAR		= 11,
			DECODER_PTZ_FOCUS_FAR		= 12,
			DECODER_PTZ_ZOOM_WIDE		= 13,
			DECODER_PTZ_ZOOM_TELE		= 14,
			DECODER_PTZ_SET_PRESET		= 15,
			DECODER_PTZ_GOTO_PRESET		= 16,
			DECODER_PTZ_NR				
		} DECODER_PTZ_COMMAND;
	#endif

	#if defined(USE_DEV_DECODER) || defined(USE_DEV_TWAUDIO)
		#define NF_DEV_DECODER_DAC_PLAYBACK		(0xff)
		#define NF_DEV_DECODER_DAC_NET_AUDIO	(0xfe)

		gboolean nf_dev_decoder_vloss_enable(gboolean is_enable);
		guint nf_dev_decoder_get_dac(void);
		gboolean nf_dev_decoder_set_dac(guint);
		gboolean nf_dev_decoder_set_picture(guint ch, guint brightness, guint hue, guint colour, guint contrast, guint sharpness);
		gint nf_dev_decoder_get_vloss_status(void);
		gboolean nf_dev_decoder_init_tbl(gboolean is_pal);
		gboolean nf_dev_decoder_cntl_raster(gint idx_decoder, gint ch, gint is_enable);
		gboolean nf_dev_decoder_cntl_raster_once(guint mask_xor, guint mask);
		gboolean nf_dev_decoder_reg_dump(gint chip);
		gboolean nf_dev_decoder_dump_new(gint is_read, gint idx_decoder, gint chip, 
											gushort addr, guchar data_write, guchar *data_read);
		gboolean nf_dev_decoder_cntl_delay(gint idx_decoder, gint ch, gint type_camera);
		gint nf_dev_decoder_get_signal_type(void);
		#if defined(USE_DEV_DECODER_NVP1108B)
			gint nf_dev_decoder_dbg_write_byte(guchar reg, guchar val, guchar dev, guint bank);
		#else
			gint nf_dev_decoder_dbg_write_byte(guchar reg, guchar val);
		#endif
		gint nf_dev_decoder_set_playback(void);
		gboolean nf_dev_decoder_cntl_eq(gint ch, gboolean is_enable, gint idx);

		gboolean nf_dev_decoder_set_dac_mute(void);
		gboolean nf_dev_decoder_set_gain(int volume);
	#endif
	gboolean 	nf_dev_decoder_cntl_scanmode(gint ch, gint mode);
	gboolean	nf_dev_decoder_cntl_scanmode_host(gint ch, gint mode);
	#if defined(ENABLE_VIDEO_AUTO_DETECTION)
		gboolean	nf_dev_decoder_cntl_video_rescan(gint ch, gint mode, guint analog_type);
	#else
		gboolean	nf_dev_decoder_cntl_video_rescan(gint ch, gint mode);
	#endif
	gboolean nf_dev_decoder_cntl_get_std(gint ch, gint *resol);
	#if defined(ENABLE_VIDEO_AUTO_DETECTION)
		gboolean	nf_dev_decoder_cntl_video_resol(gint *mode, gchar *resol);
	#endif

	gboolean nf_dev_decoder_cntl_disp_location(gint ch, gint x, gint y);
	gboolean nf_dev_decoder_cntl_stop_timer(gboolean is_stop);

#endif /* defined(USE_DEV_DECODER) */

#if defined(USE_DEV_DECODER_MINOR1)
	#if defined(USE_DEV_DECODER_NVP1108B) || defined(USE_DEV_DECODER_IDEN1100) || defined(USE_DEV_DECODER_TP2802) \
			|| defined(USE_DEV_DECODER_TP2833)
		gboolean nf_dev_decoder_set_signal_debounce(guint debounce);
	#endif
	guint nf_dev_decoder_get_signal_status(void);
#endif

#if defined(USE_DEV_DECODER_MINOR2)
	#if defined(USE_DEV_DECODER_IDEN1100)
		gboolean nf_dev_decoder_motion_init(gint is_on, gint ch, guint is_960, guint is_pal, 
						guint md_velocity, guint md_lvsens, guint md_spsens, guint md_tmpsens, guchar *mask_md_area);
	#elif defined(USE_DEV_DECODER_CX26828)
		gboolean nf_dev_decoder_motion_init(guint is_on, gint ch, guint is_pal, guchar *mask_md_area, guint md_sense);
	#endif
	gboolean nf_dev_decoder_motion_enable_flag(guint flag);
#endif

#if defined(USE_DEV_DECODER_MINOR3)
	#if defined(USE_DEV_DECODER_IDEN1100)
		struct drcoder_conf_tamper_rebl_user{
			gint ch;
			guint is_on;

			guint rd_lvsens;
			guint rd_spsens;
			guint rd_tmpsens;
			
			guint blockage_lvsens;
			guint blockage_spsens;
			guint blockage_tmpsens;

			guchar mask_rebl[DECODER_TAMPER_CELL_NUM];
		};
		
		struct drcoder_conf_tamper_defocus_user{
			gint ch;
			guint is_on;

			guint defocus_lvsens;
			guint defocus_spsens;
			guint defocus_tmpsens;
		};
		gboolean nf_dev_decoder_tamper_init_rebl(struct drcoder_conf_tamper_rebl_user *tamper_user);	
		gboolean nf_dev_decoder_tamper_init_defocus(struct drcoder_conf_tamper_defocus_user *tamper_user);	
		gboolean nf_dev_decoder_tamper_enable_flag(gboolean is_on);

	#endif

#endif

/************************************************

  SENSOR

************************************************/
#if defined(USE_DEV_ALARM_IN)
gboolean nf_dev_sensor_all_enable(gint );
gboolean nf_dev_sensor_init(gint );
gboolean nf_dev_sensor_ch_onoff(gint, gint);
gboolean nf_dev_sensor_set_type(guint);	
//gboolean nf_dev_sensor_get_ari_panic_bit(guchar *);	
#endif /* defined(USE_DEV_ALARM_IN) */





/************************************************

  RELAY

************************************************/
#if defined(USE_DEV_ALARM_OUT)
#define DEV_RELAY_ENABLE				1
gboolean nf_dev_relay_init(gint );
gboolean nf_dev_relay_on(gint, gint);	//with dwell time
gboolean nf_dev_relay_ch_on(gint);		//unlimit
gboolean nf_dev_relay_off(gint);
gboolean nf_dev_relay_set_type(guint);
gboolean nf_dev_relay_ari_on(void);
gboolean nf_dev_relay_ari_off(void);
gboolean nf_dev_relay_enable(void);
gboolean nf_dev_relay_get_type(guint *ch_bit);
	#if defined(ENABLE_RELAY_IPCAM)
		gboolean nf_dev_relay_ch_on_ipcam(gint ch);
		gboolean nf_dev_relay_ch_off_ipcam(gint ch);
	#endif
#endif /* defined(USE_DEV_ALARM_OUT) */


/************************************************

  MICOM

************************************************/
#if defined(USE_DEV_MICOM)
gint nf_dev_open_micom(void);
gint nf_dev_micom_upgrade_flag_get(void);
gboolean nf_dev_micom_upgrade_flag_set(gint set_flag);
gboolean nf_dev_micom_write(unsigned char addr, char *data, int length, int idx);
gboolean nf_dev_micom_write_for_spot(char *data, int length);
unsigned char nf_dev_micom_getc(void);
gboolean nf_dev_micom_puts(guchar *data);
gint nf_dev_micom_check_aprom(void);
gint nf_dev_micom_reset(void);
#endif



/************************************************

  BOARD_PP, BUZZER, FAN, SIGNAL_TYPE

************************************************/
#if defined(USE_DEV_BOARD_PP)	
typedef struct _NF_UTIL_FAN_INFO_T
{
	gushort speed_cpu_fan;
	gushort speed_sys_fan1;
	gushort speed_sys_fan2;
	gchar temper_cpu;
	gchar temper_sys;
	gchar cpufan_duty;
	gchar sysfan_duty;
	gchar thresh_cpu;
	gchar thresh_sys;
	gchar cpufan_mode;
	gchar sysfan_mode;
} NF_UTIL_FAN_INFO;

typedef enum _NF_UTIL_SWITCH_VCT_RESULT_E
{
	NF_UTIL_SWITCH_VCT_RESULT_PASS      = 0,
	NF_UTIL_SWITCH_VCT_RESULT_FAIL      = 1,
	NF_UTIL_SWITCH_VCT_RESULT_OPEN      = 2,
	NF_UTIL_SWITCH_VCT_RESULT_SHORT     = 3,
} NF_UTIL_SWITCH_VCT_RESULT;

typedef struct _NF_UTIL_SWITCH_VCT_INFO
{
	unsigned int port;
	char result[4];
	int length[4];
} NF_UTIL_SWITCH_VCT_INFO;

gboolean nf_dev_buzzer_on(void);
gboolean nf_dev_buzzer_on_type(guint, guint);
gboolean nf_dev_buzzer_off(void);
gboolean nf_dev_buzzer_force_on(void);
gboolean nf_dev_buzzer_force_off(void);
gboolean nf_dev_board_reset(void );
gboolean nf_dev_board_reset_watchdog(void);
gboolean nf_dev_board_fan_ctl(guint );
gboolean nf_dev_board_pp_is_pal(void);
gboolean nf_dev_board_pp_is_d1(void);
gboolean nf_dev_board_pp_fan_get_duty(gint is_cpu, guchar *fan_duty);
gboolean nf_dev_board_pp_fan_set_duty(gint is_cpu, guchar fan_duty);
gboolean nf_dev_board_pp_fan_get_mode(gint is_cpu, guchar *fan_mode);
gboolean nf_dev_board_pp_fan_set_mode(gint is_cpu, guchar fan_mode);
gboolean nf_dev_board_pp_fan_set_thresh(gint is_cpu, gchar thresh);
gboolean nf_dev_board_pp_fan_get_info(NF_UTIL_FAN_INFO *fan_info);
gboolean nf_board_pp_set_loop_out(guint enable_flag);
gboolean nf_board_pp_set_rs485_rtsn(guint is_tx);
#if defined(_UTM5X_0412D) || defined(_UTM5X_0824D) || defined(_UTM5X_1648D)
	gboolean nf_board_pp_set_eeprom(int bank, u_char addr, u_char len, u_char *data);
	gboolean nf_board_pp_get_eeprom(int bank, u_char addr, u_char len, u_char *data);
#endif
gboolean nf_dev_board_pp_get_gpio(guchar gpio, guchar pin, guchar *data);
gboolean nf_dev_board_pp_set_gpio(guchar gpio, guchar pin, guchar data);
gboolean nf_dev_board_pp_cntl_buzzer_irda(void);
#if defined(_ANF5HG_1648D) || defined(_ANF5HG_0824D) \
 || defined(_ANF6HG_0824D) || defined(_ANF6HG_1648D)
typedef enum _SWITCH_MODE_E
{
	SWITCH_MODE_SEPERATE = 0,
	SWITCH_MODE_COMBINE = 1
} SWITCH_MODE_E;
gboolean nf_dev_board_pp_switch_configuration(int mode);
gboolean nf_dev_board_pp_get_switch_reg(guint dev_addr, guint reg_addr, guint *data);
gboolean nf_dev_board_pp_set_switch_reg(guint dev_addr, guint reg_addr, guint data);
#endif

gboolean nf_dev_board_pp_poe_force_reset(void);
gboolean nf_dev_board_pp_get_net_link_state(int is_wan, int *link_state);
#endif /* defined(USE_DEV_BOARD_PP) */





/************************************************

  KEYPAD

************************************************/
#if defined(USE_DEV_KEYPAD)
gboolean nf_dev_keypad_dev_enable(void);
gboolean nf_dev_keypad_dev_disable(void);
gboolean nf_dev_keypad_led_on(guint );
gboolean nf_dev_keypad_led_off(guint );
//gboolean nf_dev_keypad_led_off(unsigned long long);
gboolean nf_dev_keypad_led_all_off(void);
gboolean nf_dev_keypad_led_all_on(void);
gboolean nf_dev_keypad_beep_on(void);
gboolean nf_dev_keypad_beep_off(void);
gboolean nf_dev_keypad_led_on_off(unsigned int long long led, gint is_on);
gboolean nf_dev_keypad_input_cnt(unsigned int *input_cnt);

struct keypad_name 
{ 
	unsigned char name[32]; 
}__attribute__((packed)); 

struct keypad_pt_list 
{ 
	char name[56][32];
	char curr_pt_name[32];
	int pt_cnt_num;
}__attribute__((packed));
gboolean nf_dev_keypad_set_pt_type(struct keypad_name *_pt_type);
gboolean nf_dev_keypad_get_pt_info(struct keypad_pt_list *_pt_info);
gboolean nf_dev_keypad_get_is_keypower(unsigned int *is_keypower);
gboolean nf_dev_keypad_get_pt_is_key(unsigned int *is_key);
guint nf_dev_keypad_check_pt_key(unsigned int key_idx);
#endif /* defined(USE_DEV_KEYPAD) */





/************************************************

  JOG

************************************************/
#if defined(USE_DEV_JOG)
gboolean nf_dev_jog_enable(gint );
#endif /* defined(USE_DEV_JOG) */


/************************************************

  LED

************************************************/
#define POWER   37
#define REC     38
#define NETWORK 39
#define ALARM   40
#define HDD1   41
#define HDD2   42
#define HDD3   43
#define HDD4   44
#define HDD5   45
#define ESATA  46


/************************************************

  SHUTTLE

************************************************/
#if defined(USE_DEV_SHUTTLE)
gboolean nf_dev_shuttle_enable(gint );
#endif /* defined(USE_DEV_JOG) */





/************************************************

  REMOCON

************************************************/
#if defined(USE_DEV_REMOCON)
gboolean nf_dev_remocon_enable(gint );
gboolean nf_dev_remocon_beep_on(void);
gboolean nf_dev_remocon_beep_off(void);
gboolean nf_dev_remocon_input_cnt(unsigned int *input_cnt);
#endif /* defined(USE_DEV_REMOCON) */
unsigned int nf_dev_remocon_get_repeat_flag(void);
void nf_dev_remocon_set_repeat_flag(unsigned int is_repeat);
void nf_dev_remocon_set_repeat_time(struct timeval *tv);
void nf_dev_remocon_get_repeat_time(struct timeval *tv);



/************************************************

  AM8816

************************************************/
#if defined(USE_DEV_AM8816)
gboolean nf_dev_am8816_set_sequence(AM8816_SPOT_INFO* );
gboolean nf_dev_am8816_set_border_color(guint);
gboolean nf_dev_am8816_camera_title_on(void);
gboolean nf_dev_am8816_camera_title_off(void);
gboolean nf_dev_am8816_date_on(void);
gboolean nf_dev_am8816_date_off(void);
gboolean nf_dev_am8816_date_format(guint date_format, guint am_pm);
gboolean nf_dev_am8816_gmt_offset(gint offset);
gboolean nf_dev_am8816_set_border_size(guint);
gboolean nf_dev_am8816_set_ntsc(void);
gboolean nf_dev_am8816_set_pal(void);
gboolean nf_dev_am8816_set_spot_title(AM8816_SPOT_TIELE_NAME *am_title_name);
gboolean nf_dev_am8816_set_covert(guint ch_mask);
gboolean nf_dev_am8816_set_vloss(guint ch_mask);
#endif /* defined(USE_DEV_AM8816) */





/************************************************

  WATCH-DOG

************************************************/
#if defined(USE_DEV_WATCHDOG)
gboolean nf_dev_watchdog_enable(void);
gboolean nf_dev_watchdog_chg_margin(guint);
gboolean nf_dev_watchdog_alive(void);
gboolean nf_dev_watchdog_cntl_thread(gint is_enable);
gboolean nf_dev_watchdog_reg_dump(void);
#endif /* defined(USE_DEV_WATCHDOG) */





/************************************************

  RS485

************************************************/
#if defined(USE_DEV_RS485)
gboolean nf_dev_rs485_readq_free(void);
gboolean nf_dev_rs485_reg_init(void);
#endif /* defined(USE_DEV_RS485) */





/************************************************

  FS1648

************************************************/
#if defined(USE_DEV_FS1648)
gboolean anf_dev_fs1648_rip_change_req(struct fs1648_chg_rip_reg *rip_context);			//hosik_record �̰� ���� ���� �� ������
gboolean anf_dev_fs1648_live_change(struct fs1648_anf_livedisp_fmt *livedisp_fmt);
gboolean anf_dev_fs1648_rip_change(struct fs1648_chg_rip_reg *rip_context);				//(anf_dev_fs1648_rip_change_req �� ���� �� �Ǽ� ����� ����� �ξ���)
gboolean anf_dev_fs1648_covert(int t_covert);
gboolean anf_zoom_start( gint ch_num );
gboolean anf_zoom_inout( gint inout );
gboolean anf_zoom_move( int zoom_x, int zoom_y );
gboolean anf_zoom_stop( void );
gboolean anf_dev_fs1648_md_onoff( unsigned int t_onoff );						//hosik_motion
gboolean anf_dev_fs1648_md_disp_onoff( unsigned int t_onoff );
gboolean anf_dev_fs1648_md_slp_on( struct fs1648_md_sensor_color *s_color );
gboolean anf_dev_fs1648_md_act_on( struct fs1648_md_sensor_color *s_color );
gboolean anf_dev_fs1648_md_sens( struct fs1648_md_sensor_val *t_sens );
gboolean anf_dev_fs1648_md_cnt( struct fs1648_md_sensor_val *t_cnt );
gboolean anf_dev_fs1648_md_area( struct fs1648_md_sensor_area *t_area );
gboolean anf_dev_fs1648_pause( unsigned short t_data );
#endif /* defined(USE_DEV_FS1648) */





/************************************************

  CD22M3492

************************************************/
#if defined(USE_DEV_CD22M3492)
gboolean nf_dev_spot_set_sequence(struct spot_status_user *spot_info);
gboolean nf_dev_spot_set_vloss(unsigned short t_data);
gboolean nf_dev_spot_get_vloss(unsigned short *t_data);
#endif	/* __NF_UTIL_DEVICE_H__ */

/************************************************

  GENNUM

************************************************/
#if defined(USE_DEV_GENNUM)
typedef struct _GENNUM_STD_DATA_T
{
	int ch;
	u_char data;
}GENNUM_STD_DATA;

gint nf_dev_open_gennum(void);
gint nf_dev_open_gennum_minor1(void);
gboolean nf_dev_gennum_rx_set_vloss(guint mask);
gboolean nf_dev_gennum_get_video_stardard(gint ch, guchar *val);
gboolean nf_dev_gennum_rx_set_vloss_interval(guint interval);
gboolean nf_dev_gennum_rx_reg_dump(gushort reg, gint ch);
gboolean nf_dev_gennum_rx_write(gushort reg, gint ch, guchar *data);
gboolean nf_dev_gennum_tx_reg_dump(gushort reg);
gboolean nf_dev_gennum_tx_wrte(gushort reg, guchar *data);
gboolean nf_dev_gennum_get_vloss(guint *vloss);
gboolean nf_dev_gennum_get_tx_loop_status(gint *is_detected);
#endif

/************************************************

PSE Controller

************************************************/
#if defined(USE_DEV_POE)
typedef struct _NF_UTIL_POE_INFO_T
{	
	int     is_discovery;	// 0 : discover 1: unknown 
	int     is_active;      // a/d status
	int     port_class;
	int     func_status;
	
	int     consumption;
	u_int   voltage;
	u_int   current_mA;

	int		reserved; // port
	
} NF_UTIL_POE_INFO;

typedef struct _NF_UTIL_POE_CHIP_INFO_T
{
	guchar id[2];
	gint is_connected[2];
} NF_UTIL_POE_CHIP_INFO;

typedef struct _NF_UTIL_POE_PORT_INFO_T
{
	NF_UTIL_POE_INFO info[32];
} NF_UTIL_POE_PORT_INFO;

typedef struct _NF_UTIL_POE_WRITE_INFO_T
{
	guchar port;
	guchar command;
	guchar data;
} NF_UTIL_POE_WRITE_INFO;

typedef struct _NF_UTIL_POE_PORT_ONOFF_T
{
	gint port;
	gint is_fail;
} NF_UTIL_POE_PORT_ONOFF;

gint nf_dev_open_poe(void);
gboolean nf_dev_poe_port_onoff(gint port, gboolean is_on, gint *is_fail);
gboolean nf_dev_poe_get_info(NF_UTIL_POE_PORT_INFO *info);
gboolean nf_dev_poe_get_chip_infomation(NF_UTIL_POE_CHIP_INFO *info);
gboolean nf_dev_poe_get_info_single(int port, NF_UTIL_POE_INFO *info);
gboolean nf_dev_poe_read_status(void);
gboolean nf_dev_poe_port_verify(guchar port, gboolean *is_on);

#if defined(USE_DEV_SWITCH)
	#define MAX_SWITCH_PORT_NUM 20
	typedef enum _NF_UTIL_SWITCH_PORT_E
	{
		NF_UTIL_SWITCH_PORT_CAM0    = 0,	// EMAC1
		NF_UTIL_SWITCH_PORT_CAM1    = 1,
		NF_UTIL_SWITCH_PORT_CAM2    = 2,
		NF_UTIL_SWITCH_PORT_CAM3	= 3,
		NF_UTIL_SWITCH_PORT_CAM4	= 4,
		NF_UTIL_SWITCH_PORT_CAM5	= 5,
		NF_UTIL_SWITCH_PORT_CAM6	= 6,
		NF_UTIL_SWITCH_PORT_CAM7	= 7,
		NF_UTIL_SWITCH_PORT_CAM8	= 8, 	// No
		NF_UTIL_SWITCH_PORT_CAM9	= 9, 	// No
		NF_UTIL_SWITCH_PORT_CAM10	= 10,	// No
		NF_UTIL_SWITCH_PORT_CAM11	= 11,	// No
		NF_UTIL_SWITCH_PORT_CAM12	= 12,	// No
		NF_UTIL_SWITCH_PORT_CAM13	= 13,	// No
		NF_UTIL_SWITCH_PORT_CAM14	= 14,	// No
		NF_UTIL_SWITCH_PORT_CAM15	= 15,	// No
		NF_UTIL_SWITCH_PORT_NOT_USE1		= 16,
		NF_UTIL_SWITCH_PORT_LAN		= 17,
		NF_UTIL_SWITCH_PORT_NOT_USE2		= 18,
		NF_UTIL_SWITCH_PORT_CPU		= 19,
		NF_UTIL_SWITCH_PORT_WAN		= 20, // EMAC0
	} NF_UTIL_SWITCH_PORT;

	typedef enum _NF_UTIL_SWITCH_MODE_E
	{
		NF_UTIL_SWITCH_CCTV_MODE      = 0,
		NF_UTIL_SWITCH_OPEN_MODE      = 1,
	} NF_UTIL_SWITCH_MODE;

	int nf_dev_open_switch(void);
	gboolean nf_dev_switch_init(int mode);
	gboolean nf_dev_switch_read(int phyaddr, unsigned char page, int reg, int *data);
	gboolean nf_dev_switch_write(int phyaddr, unsigned char page, int reg, int data);
	gboolean nf_dev_switch_set_aging_time(int sec);
	int nf_dev_get_link_status(char *ifname);
	gboolean nf_dev_switch_get_link_status(int *lport_mask);
	gboolean nf_dev_switch_get_port_mac(guchar mac_addr[16][6]);

	gboolean nf_dev_switch_get_mac_portnum(guchar mac_addr[6], int *port);
	gboolean nf_dev_switch_set_vlan(int vlan_mode);
#endif
#endif

/************************************************

 TPS2384 

************************************************/
#if defined(USE_DEV_TPS2384)
typedef struct _NF_UTIL_TPS2384_INFO_T
{
	int     is_discovery;	// 0 : discover 1: unknown 
	int     is_active;      // a/d status
	int     port_class;
	int     func_status;
	
	int     consumption;
	u_int   voltage;
	u_int   current_mA;

	int		reserved; // port
	
} NF_UTIL_TPS2384_INFO;

typedef struct _NF_UTIL_TPS2384_PORT_INFO_T
{
//	NF_UTIL_TPS2384_INFO info[NUM_ACTIVE_CH];
	NF_UTIL_TPS2384_INFO info[16];
} NF_UTIL_TPS2384_PORT_INFO;

typedef struct _NF_UTIL_TPS2384_WRITE_INFO_T
{
	guchar port;
	guchar command;
	guchar data;
} NF_UTIL_TPS2384_WRITE_INFO;

typedef struct _NF_UTIL_TPS2384_PORT_ONOFF_T
{
	gint port;
	gint is_fail;
} NF_UTIL_TPS2384_PORT_ONOFF;

gint nf_dev_open_tps2384(void);
gboolean nf_dev_tps2384_port_onoff(gint port, gboolean is_on, gint *is_fail);
gboolean nf_dev_tps2384_get_info(NF_UTIL_TPS2384_PORT_INFO *info);
gboolean nf_dev_tps2384_get_info_single(int port, NF_UTIL_TPS2384_INFO *info);
gboolean nf_dev_tps2384_read_status(void);
gboolean nf_dev_tps2384_port_verify(guchar port, gboolean *is_on);
#endif

/************************************************

 TPS2384 

************************************************/
#if defined(USE_DEV_TLV320AIC23)
gint nf_dev_open_tlv320aic23(void);
gboolean nf_dev_audio_live_onoff_ctrl(gboolean is_on);
gboolean nf_dev_audio_micb_ctrl(gboolean is_on);
gboolean nf_dev_audio_headphone_vol_ctrl(gint vol, gint is_left);
gboolean nf_dev_audio_line_input_vol_ctrl(gint vol);
gboolean nf_dev_audio_line_input_mute(gint is_mute);
gboolean nf_dev_audio_dac_mute(gint is_mute);
gboolean nf_dev_audio_reg_write(gushort reg, guint mask, guint val);
#endif

#if defined(USE_DEV_DSP)

#include <linux/soundcard.h>

gboolean nf_dev_audio_input_ch_cntl(int input_ch_num, int fd);
gboolean nf_dev_audio_dsp_reset(gboolean is_fd_read);
gboolean nf_dev_audio_dsp_get_odelay(gboolean is_fd_read, int *delay);
gboolean nf_dev_audio_dsp_get_optr(gboolean is_fd_read, void *data);
gint nf_dev_audio_dsp_get_space(gboolean is_fd_read, gboolean is_ispace, audio_buf_info *info);
#endif

#if defined(USE_DEV_IVR_HF)

#define IVR_HF_AUDIO_LIVE_CH_SEL			0x704

#include "ivr_hf.h"

gint nf_dev_ivr_hf_init(ivr_hf_init_data *data);
gint nf_dev_ivr_hf_live_reset(ivr_hf_init_data *data);
gint nf_dev_ivr_hf_set_display(ivr_hf_display *display);
gint nf_dev_ivr_hf_set_hdrec(ivr_hf_hdrec* data);
gint nf_dev_ivr_hf_set_sdrec(ivr_hf_sdrec* data);
gint nf_dev_ivr_hf_get_vloss(guint *vloss);
gint nf_dev_ivr_hf_set_md(ivr_hf_md *md);
gint nf_dev_ivr_hf_get_md(guint *md_res);
gint nf_dev_ivr_hf_get_hd_video_type(guint *type);
gint nf_dev_ivr_hf_wr_reg_byte(gint reg, gint data);
gboolean nf_dev_ivr_hf_se_vloss_color(guint vloss);
#endif

#if defined(USE_DEV_TW2828)
#include "tw2828.h"

gboolean nf_dev_tw2828_set_spot(TW2828_SPOT_INFO spot_seq_info);
gboolean nf_dev_tw2828_set_osd(TW2828_OSD_INFO osd_info);
#endif

#if defined(USE_DEV_EDID)
#include "itx_edid.h"
	gboolean nf_dev_edid_get_raw_data(struct edid_data *info, gboolean is_vga);
#endif

#if defined(USE_DEV_DRV_EXT)
	gboolean nf_dev_drv_ext_cntl_loopout(gint ch, guint is_enable);
	gboolean nf_dev_drv_ext_cntl_loopout_chk_chip(gboolean *is_chip);
#endif


/************************************************

 IP804

************************************************/
#if defined(USE_DEV_IP804)
#define POE_GET_INFO				IP804_GET_INFO
#define POE_WRITE					IP804_WRITE
#define POE_PORT_ON					IP804_PORT_ON
#define POE_PORT_OFF				IP804_PORT_OFF
#define POE_REG_READ				IP804_REG_RED
#define POE_GET_INFO_PORT			IP804_GET_INFO_PORT
#define POE_GET_CHIP_INFORMATION	IP804_GET_CHIP_INFORMATION
#endif

#endif /* __NF_UTIL_DEVICE_H__ */
