#ifndef _VW_PLAYBACK_SHORTCUT_MENU_H
#define _VW_PLAYBACK_SHORTCUT_MENU_H

typedef enum {
	PB_SHORT_CUT_NORMAL = 0,
	PB_SHORT_CUT_PANO1,
	PB_SHORT_CUT_PANO2,
} PB_SHORT_CUT_MODE_E;

NFOBJECT* vw_playback_shortcut_menu_open(NFWINDOW *parent);
void vw_playback_shortcut_menu_show(gint x, gint y, guint ch);
gboolean vw_playback_shortcut_menu_is_shown();
void vw_playback_shortcut_menu_hide();
void vw_playback_shortcut_menu_refresh();
void vw_playback_shortcut_change_bookmark_text(gint status);
void vw_playback_shortcut_change_snap(gint enable);
void vw_playback_shortcut_set_played(gint ch, gint status);

/*inline*/ guint get_menu_channel_pb();

#endif
