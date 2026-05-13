#include "nf_common.h"

#include "nf_hw.h"

static char _hw_model_type[32];
static char _hw_board_type[32];
static char _hw_irda_type[32];

static gchar *_nf_hw_get_model_type(void);
static gchar *_nf_hw_get_board_type(void);

int _nf_hw_nr_alarm_in=0;
int _nf_hw_nr_alarm_out_ar=0;
int _nf_hw_nr_alarm_out_ao=0;
int _nf_hw_nr_alarm_out_ipcam_ar=0;
int _nf_hw_nr_ari_panic=0;
int _nf_hw_nr_audio=0;

gboolean nf_hw_kcmdline_init(void)
{
	gchar *contents = NULL;
	gsize  length = 0;
	gchar *ret;
	GError *error = NULL;

	memset( _hw_model_type, 0x00, sizeof(_hw_model_type));
	memset( _hw_board_type, 0x00, sizeof(_hw_board_type));

	if(!g_file_get_contents ("/proc/cmdline", &contents, &length, &error)) {
		g_warning("%s %s\n", __FUNCTION__,  error->message);
		g_error_free (error);

		if(contents)
			g_free(contents);

		return 0;
	}

	if(contents) {
		char *tmp = NULL;

		if((tmp = strstr(contents, NF_HW_MODEL_TYPE))) {
			snprintf(_hw_model_type, sizeof(_hw_model_type),
						tmp + (strlen(NF_HW_MODEL_TYPE)));
			tmp = strchr(_hw_model_type, ' ');
			if ( tmp ) {
				*tmp = 0x0;
			}
			g_strstrip(_hw_model_type);
		}

		if((tmp = strstr(contents, NF_HW_BOARD_TYPE))) {
			snprintf(_hw_board_type, sizeof(_hw_board_type),
						tmp + (strlen(NF_HW_BOARD_TYPE)));
			tmp = strchr(_hw_board_type, ' ');
			if ( tmp ) {
				*tmp = 0x0;
			}
			g_strstrip(_hw_board_type);
		}

		if((tmp = strstr(contents, NF_HW_IRDA_TYPE))) {
			snprintf(_hw_irda_type, sizeof(_hw_irda_type),
						tmp + (strlen(NF_HW_IRDA_TYPE)));
			tmp = strchr(_hw_irda_type, ' ');
			if ( tmp ) {
				*tmp = 0x0;
			}
			g_strstrip(_hw_irda_type);
		}

		#if 0
			g_message("%s _hw_model_type     [%s]", __FUNCTION__, _hw_model_type);
			g_message("%s _hw_board_type     [%s]", __FUNCTION__, _hw_board_type);
			g_message("%s _hw_irda_type      [%s]", __FUNCTION__, _hw_irda_type);
		#endif

		g_free(contents);
	}

	_nf_hw_nr_alarm_in=nf_hw_get_alarm_in_nr();
	_nf_hw_nr_alarm_out_ar=nf_hw_get_alarm_out_ar_nr();
	_nf_hw_nr_alarm_out_ao=nf_hw_get_alarm_out_ao_nr();
	_nf_hw_nr_alarm_out_ipcam_ar=nf_hw_get_alarm_out_ar_nr_ipcam();
	_nf_hw_nr_ari_panic=nf_hw_get_ari_panic_nr();
	_nf_hw_nr_audio=nf_hw_get_audio_nr();

	g_message("[%s] nr_alarm_in              [%d]", __FUNCTION__, _nf_hw_nr_alarm_in);
	g_message("[%s] nr_alarm_out_ar          [%d]", __FUNCTION__, _nf_hw_nr_alarm_out_ar);
	g_message("[%s] nr_alarm_out_ao          [%d]", __FUNCTION__, _nf_hw_nr_alarm_out_ao);
	g_message("[%s] nr_alarm_out_ipcam_ar    [%d]", __FUNCTION__, _nf_hw_nr_alarm_out_ipcam_ar);
	g_message("[%s] nr_ari_panic             [%d]", __FUNCTION__, _nf_hw_nr_ari_panic);
	g_message("[%s] nr_audio                 [%d]", __FUNCTION__, _nf_hw_nr_audio);

	return TRUE;
}

static gchar *_nf_hw_get_model_type(void)
{
	return _hw_model_type;
}

static gchar *_nf_hw_get_board_type(void)
{
	return _hw_board_type;
}

int nf_hw_get_model(void)
{
	char *str=_nf_hw_get_model_type();
	gint model=0;

	if(str == NULL) {
		return HW_MODEL_UNKNOWN;
	}

	if(strncmp("utm7g_0412", str, 10) == 0) {
		model=HW_MODEL_UTM7GN_04;
	}
	else if(strncmp("utm7g_0824", str, 10) == 0) {
		model=HW_MODEL_UTM7GN_08;
	}
	else if(strncmp("utm7g_1648", str, 10) == 0) {
		model=HW_MODEL_UTM7GN_16;
	}
	else if(strncmp("anf8g_1648", str, 10) == 0) {
		model=HW_MODEL_ANF8GN_16;
	}
	else if(strncmp("ipxp5_3296", str, 10) == 0) {
		model=HW_MODEL_IPXP5_32;
	}
	else {
		model=HW_MODEL_UNKNOWN; 
	}

	g_message("[%s:%d] %d\n", __FUNCTION__, __LINE__, model);

return model;

}

int nf_hw_get_board_type(void)
{
	char *s=_nf_hw_get_board_type();

	if(s == NULL) {
		return HW_BOARD_TYPE_UNKNOWN;
	}
	else {
		if(strncmp(s, "a", 1) == 0) {
			return HW_BOARD_TYPE_A;
		}
		else if(strncmp(s, "b", 1) == 0) {
			return HW_BOARD_TYPE_B;
		}
		else {
			return HW_BOARD_TYPE_A;
		}
	}
}

int nf_hw_get_alarm_in_nr(void)
{
	int model=0, board_type=0;

	model=nf_hw_get_model();
	board_type=nf_hw_get_board_type();

	if(model == HW_MODEL_UTM7GN_16) {
		if(board_type == HW_BOARD_TYPE_A) {
			return ALARM_IN_NR_16;
		}
		else {
			return ALARM_IN_NR_2;
		}
	}
	else if(model == HW_MODEL_UTM7GN_08) {
		if(board_type == HW_BOARD_TYPE_A) {
			return ALARM_IN_NR_8;
		}
		else
			return ALARM_IN_NR_2;
	}
	else if(model == HW_MODEL_UTM7GN_04) {
		if(board_type == HW_BOARD_TYPE_A) {
			return ALARM_IN_NR_4;
		}
		else {
			return ALARM_IN_NR_2;
		}
	}
	else if(model == HW_MODEL_ANF8GN_16) {
		return ALARM_IN_NR_16;
	}
	else if(model == HW_MODEL_IPXP5_32) {
		return ALARM_IN_NR_16;
	}
	else {
		return ALARM_IN_NR_4;
	}

}

int nf_hw_get_alarm_out_nr_ipcam(void)
{
	return ALARM_IN_NR_0;
}

int nf_hw_get_alarm_out_nr(void)
{
	int alarm_out_nr=0, alarm_out_ar_nr=0, alarm_out_ao_nr=0, alarm_out_ipcam_nr=0;

	alarm_out_ar_nr=nf_hw_get_alarm_out_ar_nr();	// alarm relay
	alarm_out_ao_nr=nf_hw_get_alarm_out_ao_nr();	// alarm out
	alarm_out_ipcam_nr=nf_hw_get_alarm_out_ar_nr_ipcam();

	alarm_out_nr=(alarm_out_ar_nr + alarm_out_ao_nr + alarm_out_ipcam_nr);

	return alarm_out_nr;
}

// alarm relay(digital)
int nf_hw_get_alarm_out_ar_nr(void)
{
	int model=0, board_type=0;

	model=nf_hw_get_model();
	board_type=nf_hw_get_board_type();

	if(model == HW_MODEL_UTM7GN_16) {
		if(board_type == HW_BOARD_TYPE_A) {
			return ALARM_RELAY_NR_1;
		}
		else {
			return ALARM_RELAY_NR_1;
		}
	}
	else if(model == HW_MODEL_UTM7GN_08) {
		if(board_type == HW_BOARD_TYPE_A) {
			return ALARM_RELAY_NR_1;
		}
		else
			return ALARM_RELAY_NR_1;
	}
	else if(model == HW_MODEL_UTM7GN_04) {
		if(board_type == HW_BOARD_TYPE_A) {
			return ALARM_RELAY_NR_1;
		}
		else {
			return ALARM_RELAY_NR_1;
		}
	}
	else if(model == HW_MODEL_ANF8GN_16) {
		return ALARM_RELAY_NR_8;
	}
	else if(model == HW_MODEL_IPXP5_32) {
		return ALARM_RELAY_NR_8;
	}
	else if(model == HW_MODEL_IPXP4CE_16) {
		return ALARM_RELAY_NR_0;
	}
	else if(model == HW_MODEL_IPXM4CE_04) {
		return ALARM_RELAY_NR_0;
	}
	else if(model == HW_MODEL_IPXM4CE_08) {
		return ALARM_RELAY_NR_0;
	}
	else if(model == HW_MODEL_IPXM4CE_16) {
		return ALARM_RELAY_NR_0;
	}
	else {
		return ALARM_RELAY_NR_1;
	}
}

// alarm out(analog)
int nf_hw_get_alarm_out_ao_nr(void)
{
	int model=0;

	model=nf_hw_get_model();

	if(model == HW_MODEL_ANF8GN_16) {
		return ALARM_OUT_NR_8;
	}
	else if(model == HW_MODEL_IPXP5_32) {
		return ALARM_OUT_NR_8;
	}
	else if(model == HW_MODEL_IPXP4CE_16) {
		return ALARM_OUT_NR_0;
	}
	else if(model == HW_MODEL_IPXM4CE_04) {
		return ALARM_OUT_NR_0;
	}
	else if(model == HW_MODEL_IPXM4CE_08) {
		return ALARM_OUT_NR_0;
	}
	else if(model == HW_MODEL_IPXM4CE_16) {
		return ALARM_OUT_NR_0;
	}
	else {
		return ALARM_OUT_NR_0;
	}
}

int nf_hw_get_alarm_out_ar_nr_ipcam(void)
{
	int model=0, board_type=0;

	model=nf_hw_get_model();
	board_type=nf_hw_get_board_type();

	if(model == HW_MODEL_UTM7GN_16) {
		if(board_type == HW_BOARD_TYPE_A) {
			return ALARM_OUT_NR_0;
		}
		else {
			return ALARM_OUT_NR_0;
		}
	}
	else if(model == HW_MODEL_UTM7GN_08) {
		if(board_type == HW_BOARD_TYPE_A) {
			return ALARM_OUT_NR_0;
		}
		else
			return ALARM_OUT_NR_0;
	}
	else if(model == HW_MODEL_UTM7GN_04) {
		if(board_type == HW_BOARD_TYPE_A) {
			return ALARM_OUT_NR_0;
		}
		else {
			return ALARM_OUT_NR_0;
		}
	}
	else if(model == HW_MODEL_ANF8GN_16) {
		return ALARM_OUT_NR_0;
	}
	else if(model == HW_MODEL_IPXP5_32) {
		return ALARM_OUT_NR_0;
	}
	else if(model == HW_MODEL_IPXP4CE_16) {
		return ALARM_OUT_NR_0;
	}
	else if(model == HW_MODEL_IPXM4CE_04) {
		return ALARM_OUT_NR_0;
	}
	else if(model == HW_MODEL_IPXM4CE_08) {
		return ALARM_OUT_NR_0;
	}
	else if(model == HW_MODEL_IPXM4CE_16) {
		return ALARM_OUT_NR_0;
	}
	else {
		return ALARM_OUT_NR_0;
	}
}

int nf_hw_get_ari_panic_nr(void)
{
	return ARI_PANIC_NR_2;
}

int nf_hw_get_audio_nr(void)
{
	int model=0, board_type=0;

	model=nf_hw_get_model();
	board_type=nf_hw_get_board_type();

	if(model == HW_MODEL_UTM7GN_16) {
		if(board_type == HW_BOARD_TYPE_A) {
			return AUDIO_NR_4;
		}
		else {
			return AUDIO_NR_4;
		}
	}
	else if(model == HW_MODEL_UTM7GN_08) {
		if(board_type == HW_BOARD_TYPE_A) {
			return AUDIO_NR_4;
		}
		else {
			return AUDIO_NR_4;
		}
	}
	else if(model == HW_MODEL_UTM7GN_04) {
		if(board_type == HW_BOARD_TYPE_A) {
			return AUDIO_NR_4;
		}
		else  {
			return AUDIO_NR_4;
		}
	}
	else if(model == HW_MODEL_ANF8GN_16) {
		return AUDIO_NR_16;
	}
	else if(model == HW_MODEL_IPXP5_32) {
		return AUDIO_NR_1;//ksi_test
	}
	else {
		return AUDIO_NR_4;
	}
}

int nf_hw_get_audio_input_nr(void)
{
	return nf_hw_get_audio_nr();
}

int nf_hw_get_audio_output_nr(void)
{
	return AUDIO_NR_1;
}

char *nf_hw_get_type_irda(void)
{
	return _hw_irda_type;
}

