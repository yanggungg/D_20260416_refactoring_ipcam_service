
#ifndef _VW_LIVE_PTZCTRL_INTERNAL_H_
#define _VW_LIVE_PTZCTRL_INTERNAL_H_

#include "vw.h"

gint _live_ptz_prst_open(NFOBJECT *parent, NFOBJECT *prst_fixed);
gint _live_ptz_ctrl_open(NFOBJECT *parent, NFOBJECT *ctrl_fixed);
gint _live_ptz_status_open(NFOBJECT *parent, NFOBJECT *status_fixed);

gint _live_ptz_main_cmd_pattern(gint channel);
gint _live_ptz_main_close();

gint _live_ptz_main_show(gint channel);
gint _live_ptz_main_hide();

gint _live_ptz_prst_set_data(PtzData *ptz_data, PtzPresetData *preset_data, PtzScanData *scan_data, PtzTourData *tour_data);
gint _live_ptz_prst_set_evt_data(EA_AlmSenData *almsen_data, EA_MotSenData *motsen_data, EA_VLossData *vloss_data, EA_PosData *pos_data);

gint _live_ptz_ctrl_set_data(PtzData *ptz_data, PtzPresetData *preset_data);

gint _live_ptz_prst_init_channel(guint channel);
gint _live_ptz_ctrl_init_channel(guint channel);
gint _live_ptz_status_init_channel(guint channel);

gint _live_ptz_main_change_channel(guint channel, gint expose);
gint _live_ptz_prst_change_channel(guint channel, gint expose);
gint _live_ptz_ctrl_change_channel(guint channel, gint expose);
gint _live_ptz_status_change_channel(guint channel, gint expose);

#endif

