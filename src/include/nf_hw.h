#ifndef _NF_HW_H
#define _NF_HW_H

#define NF_HW_MODEL_TYPE				"model_type="
#define NF_HW_BOARD_TYPE				"board_type="
#define NF_HW_IRDA_TYPE					"rc_type="

typedef enum _HW_MODEL_E
{
	HW_MODEL_UTM7GN_04		= 0x0001,
	HW_MODEL_UTM7GN_08		= 0x0002,
	HW_MODEL_UTM7GN_16		= 0x0003,

	HW_MODEL_ANF8GN_16		= 0x0013,

	HW_MODEL_IPXP5_32		= 0x0104,

	HW_MODEL_IPXP4CE_16     = 0x5003,

	HW_MODEL_IPXM4CE_04     = 0x6001,
	HW_MODEL_IPXM4CE_08     = 0x6002,
	HW_MODEL_IPXM4CE_16     = 0x6003,

	HW_MODEL_UNKNOWN		= 0x0
} HW_MODEL;

typedef enum _HW_BOARD_TYPE_E
{
	HW_BOARD_TYPE_A             = 0x1,
	HW_BOARD_TYPE_B             = 0x2,

	HW_BOARD_TYPE_UNKNOWN       = 0xF
} HW_BOARDL_TYPE;

typedef enum _HW_ALARM_IN_NR_E
{
	ALARM_IN_NR_0  = 0,
	ALARM_IN_NR_1  = 1,
	ALARM_IN_NR_2  = 2,
	ALARM_IN_NR_4  = 4,
	ALARM_IN_NR_8  = 8,
	ALARM_IN_NR_16 = 16,

} HW_ALARM_IN_NR;

typedef enum _HW_ALARM_RELAY_NR_E
{
	ALARM_RELAY_NR_0  = 0,
	ALARM_RELAY_NR_1  = 1,
	ALARM_RELAY_NR_4  = 4,
	ALARM_RELAY_NR_8  = 8,

} HW_ALARM_RELAY_NR;

typedef enum _HW_ALARM_OUT_NR_E
{
	ALARM_OUT_NR_0  = 0,
	ALARM_OUT_NR_1  = 1,
	ALARM_OUT_NR_4  = 4,
	ALARM_OUT_NR_8  = 8,

} HW_ALARM_OUT_NR;

typedef enum _HW_ARI_PANIC_E
{
	ARI_PANIC_NR_0  = 0,
	ARI_PANIC_NR_2  = 2,

} HW_ARI_PANIC_NR;

typedef enum _HW_AUDIO_NR_E
{
	AUDIO_NR_1  = 1,
	AUDIO_NR_4  = 4,
	AUDIO_NR_8  = 5,
	AUDIO_NR_16	= 16

} HW_AUDIO_NR;

int nf_hw_get_model(void);
int nf_hw_get_board_type(void);
int nf_hw_get_alarm_in_nr(void);
int nf_hw_get_alarm_out_nr_dvr(void);
int nf_hw_get_alarm_out_nr_ipcam(void);
int nf_hw_get_alarm_out_nr(void);
int nf_hw_get_alarm_out_ar_nr(void);
int nf_hw_get_alarm_out_ao_nr(void);
int nf_hw_get_alarm_out_ar_nr_ipcam(void);
int nf_hw_get_ari_panic_nr(void);
int nf_hw_get_audio_nr(void);
int nf_hw_get_audio_input_nr(void);
int nf_hw_get_audio_output_nr(void);
char *nf_hw_get_type_irda(void);

#endif

