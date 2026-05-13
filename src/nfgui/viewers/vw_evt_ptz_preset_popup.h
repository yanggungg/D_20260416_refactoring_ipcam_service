
#ifndef _VW_EVT_PTZ_PRESET_POPUP_H_
#define _VW_EVT_PTZ_PRESET_POPUP_H_

#include "objects/nfobject.h"

typedef struct _PRST_INFO_T {
    guint number[GUI_CHANNEL_CNT];
} PRST_INFO_T;

gint vw_evt_ptz_preset_popup_open(NFWINDOW *parent, PRST_INFO_T *info, guint ch_mask);

#endif

