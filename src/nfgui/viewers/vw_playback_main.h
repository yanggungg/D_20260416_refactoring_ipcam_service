#ifndef _VW_PLAYBACK_MAIN_H
#define _VW_PLAYBACK_MAIN_H

#include "vsm.h"

typedef enum _PB_OPEN_BY {
	OPEN_BY_LIVE_TL = 0,
	OPEN_BY_LIVE_LOG,
	OPEN_BY_LIVE_MENU,
	OPEN_BY_ARCH_DF,
	OPEN_BY_ARCH_PLAY,
	OPEN_BY_SEARCH,
	OPEN_BY_PRESERVE_PLAY,
	OPEN_BY_DLVA_TL,	
	OPEN_BY_MAX,	
} PB_OPEN_BY;

// search(by time, by event, by thumbnail, etc..) -> playback move

void vw_playback_open(NFWINDOW *parent, LIVESTART_T *lst, PB_OPEN_BY from);
void vw_playback_out_search_open(void);
void vw_playback_out_arch_open(void);
void vw_playback_out_live_open(void);
void vw_playback_out_opened(void);
void vw_playback_out_SearchPage_open(gint page);
void vw_arch_play_out(void);
gboolean pb_div1_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean pb_div4_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean pb_div9_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean pb_div16_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean pb_div36_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean pb_osd_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean pb_play_step_backward_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean pb_play_ds_backward_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean pb_play_backward_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean pb_play_pause_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean pb_play_forward_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean pb_play_ds_forward_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean pb_play_step_forward_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
void vw_playback_multi_zoom_func();
void vw_playback_full_zoom_func();
void vw_playback_snapshot_func();
gboolean vw_playback_snap_func(gint ch);
gboolean vw_playback_archive_func();
gboolean vw_playback_process_common_KeyEvent(KEYPAD_KID kpid);
gboolean vw_playback_process_common_JogEvent(JOGID jog_id);
gboolean vw_playback_process_common_ShuttleEvent(SHUTTLEID shuttle_id);

NFOBJECT *vw_playback_get_control_box();
NFOBJECT *vw_playback_get_div_box();
NFOBJECT *vw_playback_get_func_box();
#endif
