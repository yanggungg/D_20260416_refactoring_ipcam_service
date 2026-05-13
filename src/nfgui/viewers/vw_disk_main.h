#ifndef _VW_DISK_MAIN_H_
#define _VW_DISK_MAIN_H_

#include "nf_api_disk.h"
#include "scm.h"

#define MAX_DISK_COUNT				(5)
#define INT_DISP_DISK_COUNT			(5)
#define EXT_DISP_DISK_COUNT			(5)
#define INTERNAL_DISK_GRP		(0)
#define EXTERNAL_DISK_GRP		(1)


void VW_DiskSetup_Open(NFWINDOW *parent, NFOBJECT *prev_wnd, gint page);
void VW_DiskSetup_Destroy(NFOBJECT *object);

void disk_data_changed(gboolean change);
gboolean disk_data_is_changed();

DISK_CAPINFO_T* get_disk_info(gint group);
DISK_SMARTINFO_T* get_smart_disk_info(gint group);
DISK_RECINFO_T* get_disk_rec_time(gint group);
DISK_RAIDINFO_T* get_disk_raid_info(STRG_TYPE_E type);

gint get_disk_count(gint group);
gboolean update_smart_disk_info();
gchar* conv_smart_status_to_string(guint status, gchar* buf);
#endif

