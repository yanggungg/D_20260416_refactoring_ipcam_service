#ifndef __VW_WIZARD_INIT_H
#define __VW_WIZARD_INIT_H

#include "objects/nfwindow.h"
#include "objects/nfobject.h"

#include "vw_wizard_welcome.h"
#include "vw_wizard_language.h"
#include "vw_wizard_sigtype.h"
#include "vw_wizard_password_confirm.h"
#include "vw_wizard_password_change.h"
#include "vw_wizard_datetime_change.h"
#include "vw_wizard_record_instruction.h"
#include "vw_wizard_record_modeselect.h"
#include "vw_wizard_record_imageconf.h"
#include "vw_record_main.h"
#include "vw_record_data_internal.h"
#include "vw_wizard_network_instruction.h"
#include "vw_wizard_network_ipsetup.h"
#include "vw_wizard_network_ddnssetup.h"
#include "vw_wizard_network_upnpsetup.h"
#include "vw_wizard_network_onestopsetup.h"
#include "vw_wizard_network_easyipsetup.h"
#include "vw_wizard_network_result.h"


#define WIZARD_SIZE_WID			(guint)(810)
#define WIZARD_SIZE_HEI			(guint)(720)

enum {
    MODE_CONTINOUS = 0,
    MODE_MOTION,
    MODE_ALARM,
    MODE_MOT_ALARM,
    MODE_INTENSIVE_MOTION,
    MODE_INTENSIVE_ALARM,
    MODE_INTENSIVE_MOT_ALARM,

    MODE_RECORD_CNT
};

typedef struct _LANGUAGE_DATA_T {
    gchar lang[STRING_SIZE_32];
    gint langCnt;
} LANGUAGE_DATA_T;

typedef struct _SIGTYPE_DATA_T {
    gboolean sigtype;
    gboolean is_changed;
} SIGTYPE_DATA_T;

typedef struct _ACCOUNT_DATA_T {
    gchar userid[8];
    gchar pw[NFUI_MAX_ENHANCED_PW_SIZE];
} ACCOUNT_DATA_T;

typedef struct _SYSTEM_DATA_T {
    gint            agr_policy;
} SYSTEM_DATA_T;

typedef struct _NETWORK_DATA_T {
    IPSetupData     ipsetup_data;
    DDNSData        ddns_data;
    gint            use_sequrinet;
} NETWORK_DATA_T;

typedef struct _WIZARD_USERDATA_T {   
    NFOBJECT *topwnd;
    gchar title[128];
    gint run_import;

    LANGUAGE_DATA_T org_langData;
    LANGUAGE_DATA_T langData;
    SIGTYPE_DATA_T org_sigData;
    SIGTYPE_DATA_T sigData;
    ACCOUNT_DATA_T accData;
    DATATIME_DATA_T org_dtData;
    DATATIME_DATA_T dtData;
    RECORD_DATA_T org_recordData;
    RECORD_DATA_T recordData;
    SYSTEM_DATA_T  org_systemData;
    SYSTEM_DATA_T  systemData;    
    NETWORK_DATA_T org_networkData;
    NETWORK_DATA_T networkData;
} WIZARD_USERDATA_T;

gint vw_wizard_settings_reload(WIZARD_USERDATA_T *wizard_data);
gboolean vw_wizard_init(NFWINDOW *parent, gint remove_sec);

#endif

