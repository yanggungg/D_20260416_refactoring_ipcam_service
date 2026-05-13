#ifndef _VW_PLAYBACK_CONTROL_BOX_H
#define _VW_PLAYBACK_CONTROL_BOX_H

NFOBJECT* vw_playback_control_box_open(NFWINDOW *parent);
void vw_playback_control_box_Show();
void vw_playback_control_box_Hide();
gboolean vw_playback_control_box_IsShown();
void vw_playback_control_box_change_osd_img(gint status);
void vw_playback_control_box_change_stepbutton(gint status);
void vw_playback_control_box_exit();

#endif
