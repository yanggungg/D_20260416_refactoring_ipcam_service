#include <string.h>

#include "nf_afx.h"
#include "iux_afx.h"

#include "vw_record_main.h"
#include "vw_record_data_internal.h"

#include "vw_record_continuous_param.h"
#include "vw_record_motion_param.h"
#include "vw_record_alarm_param.h"
#include "vw_record_panic.h"
#include "vw_record_net_streaming.h"


#include "scm.h"
#include "nf_api_ipcam.h"


#define DBG_LEVEL		0
#define DBG_MODULE		"REC_DATA_CAP"


////////////////////////////////////////////////////////////
//
// private variable
//

typedef struct _PARAM_T
{
	gchar 	cur[64];
	gchar	cap[64];
} PARAM_T;

typedef struct _RC_T
{
	PARAM_T	size[GUI_CHANNEL_CNT];
	PARAM_T	fps[GUI_CHANNEL_CNT];
} RC_T;

static RC_T irc;

////////////////////////////////////////////////////////////
//
// private functions
//




////////////////////////////////////////////////////////////
//
// public interfaces
//

void vw_record_check_capable_param_data()
{
	CAM_PROFILE_T prof;
	gint ch;
	
	memset(&irc, 0x00, sizeof(RC_T));

	for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
	{
		scm_get_cam_profile(ch, &prof);
		translate_data_size_capable(prof.resol.cap, irc.size[ch].cap);
		translate_data_fps_capable(prof.fps.cap, irc.fps[ch].cap);
	}
}

void vw_record_set_capable_param_data()
{
	gint ret_sum = 0;

	ret_sum += set_capable_continuous_data();
	ret_sum += set_capable_motion_data();
	ret_sum += set_capable_alarm_data();
	ret_sum += set_capable_panic_data();
	ret_sum += set_capable_netstream_data();	
	ret_sum += set_capable_auto_data();

	if (ret_sum)
		vw_send_notify_change_record_data();	
}

void vw_record_refresh_param_data()
{
	VW_RecContParam_info_refresh();
	VW_RecMotionParam_info_refresh();
	VW_RecAlarmParam_info_refresh();
	VW_RecPanic_info_refresh();
	VW_NetStream_info_refresh();
}

gchar* get_record_size_capable_data(gint ch)
{
	g_return_if_fail ( ch>=0 && ch<GUI_CHANNEL_CNT);

	return irc.size[ch].cap;
}

gchar* get_record_fps_capable_data(gint ch)
{
	g_return_if_fail ( ch>=0 && ch<GUI_CHANNEL_CNT);

	return irc.fps[ch].cap;
}

