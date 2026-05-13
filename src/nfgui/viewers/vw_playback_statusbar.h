#ifndef _VW_PLAYBACK_STATUSBAR_H
#define _VW_PLAYBACK_STATUSBAR_H

NFOBJECT* vw_playback_statusbar_open(NFWINDOW *parent);
void vw_playback_statusbar_playstatus_start(void);
void vw_playback_statusbar_playstatus_end(void);
void vw_playback_statusbar_hide(void);
void vw_playback_statusbar_show(void);
void vw_playback_statusbar_change_osd_img(gint status);
void vw_playback_statusbar_change_bookmark_img(gint status);
void vw_playback_statusbar_change_stepbutton(gint status);
void vw_playback_statusbar_change_snap(gint enable);
#endif
