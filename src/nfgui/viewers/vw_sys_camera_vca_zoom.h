#ifndef _VW_SYS_CAMERA_VCA_ZOOM_H_
#define _VW_SYS_CAMERA_VCA_ZOOM_H_


gint VW_VCA_Zoom_Open(NFOBJECT *parent, guint ch, ivca_calib_t* t, gint select_idx);

gint zoom_add_target();
gint zoom_delete_target();
gint zoom_reset_target();
gint zoom_save_targets();
gint get_target_num();
gint zoom_estimate_target(NFOBJECT *obj);

gint zoom_wnd_close();
void _vca_zoom_init();

#endif
