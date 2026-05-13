#ifndef _VW_PLAYBACK_INTERNAL_H
#define _VW_PLAYBACK_INTERNAL_H


// playback -> search(previous search menu), arch, live move

void vw_playback_out_search_open();
void vw_playback_out_arch_open();
void vw_playback_out_live_open();


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
#endif
