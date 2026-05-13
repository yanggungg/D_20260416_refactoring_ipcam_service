#ifndef _VW_RECORD_MAIN_H_
#define _VW_RECORD_MAIN_H_

#include "objects/nfobject.h"
#include "objects/nfwindow.h"

#define REC_SUBPAGE_TAB_BTN_H		(40)

enum {
	AUTO_CONFIG = 0,
	MANUAL_CONFIG,
	CONFIG_CNT
};

void VW_RecordSetup_Open(NFWINDOW *parent);
void VW_RecordSetup_Destroy(NFOBJECT *object);
void set_changed_record_data();
void VW_RecordSetup_change_config_mode(guint mode);
gboolean vw_send_notify_change_record_data();
gint get_ptz_channel(gint ch);

#endif

