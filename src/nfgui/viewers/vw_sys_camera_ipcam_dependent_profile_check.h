#ifndef _DEPENDENT_PROFILE_CHECK_H_
#define _DEPENDENT_PROFILE_CHECK_H_

#include "objects/nfobject.h"

gint get_antiflicker_category_manage_int_db_set(gint ch, LT_TYPE_T *lt, IPCamSetupData *tmpData);
gint get_dc_iris_category_manage_init_db_set(gint ch, LT_TYPE_T *lt, IPCamSetupData *tmpData);
gint get_max_shutter_speed_category_init_db_set(gint ch, LT_TYPE_T *lt, IPCamSetupData *tmpData);
gint get_base_shutter_speed_category_manage_init_db_set(gint ch, LT_TYPE_T *lt, IPCamSetupData *tmpData);

gint get_antiflicker_category_manage(gint ch, LT_TYPE_T *lt, guint64 *supported);
gint get_max_shutter_speed_category_manage(gint ch, LT_TYPE_T *lt, guint64 *supported);
gint get_base_shutter_speed_category_manage(gint ch, LT_TYPE_T *lt, guint64 *supported);
gint get_dc_iris_category_manage(gint ch, LT_TYPE_T *lt, guint64 *supported);

gint get_maxshutter_speed_category_callback(gint ch, guint64 *dependent, guint64 *visible, guint64 *enable, guint64 *supported_category);
gint get_antiflicker_category_callback(gint ch, guint64 *dependent, guint64 *visible, guint64 *enable, guint64 *supported_category);
gint get_base_shutter_speed_category_callback(gint ch, guint64 *dependent, guint64 *visible, guint64 *enable, guint64 *supported_category);
gint get_dc_iris_category_callback(gint ch, guint64 *dependent, guint64 *visible, guint64 *enable, guint64 *supported_category);

gboolean get_supported_base_shutter_speed_support(gint ch);

gboolean _set_antiflicker_value_sync(gint ch, gchar *antiflicker_caption);
gboolean _set_wdr_value_sync(gint ch, gchar *wdr_caption);
gboolean _set_slow_shutter_value_sync(gint ch, gchar *slow_caption);
gboolean _set_defog_value_sync(gint ch, gchar *defog_caption);
gboolean _set_blc_control_value_sync(gint ch, gchar *blc_control_caption);
gboolean _set_max_shutter_value_sync(gint ch, gchar *max_caption);
gboolean _set_base_shutter_value_sync(gint ch, gchar *base_caption);

#endif 
