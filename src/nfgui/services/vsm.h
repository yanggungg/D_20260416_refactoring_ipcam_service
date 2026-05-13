/*
 * vsm.h
 * 	- vsm external header file
 *	- dependency :
 *
 * Written by Jungkyu Park. <parangi@itxsecurity.com>
 * Copyright (c) ITX security, Jan 10, 2011
 *
 */

#ifndef _VSM_H_
#define _VSM_H_

#include "nf_common.h"
#include "../support/event_loop.h"
#include "cmm.h"
#include "scm.h"

#include "nf_api_play.h"

#define MODE_NORMAL_ACTIVE_W		(DISPLAY_ACTIVE_WIDTH - 192)
#define MODE_NORMAL_ACTIVE_H		(DISPLAY_ACTIVE_HEIGHT - 108)

#define MODE_FULL_ACTIVE_W			(DISPLAY_ACTIVE_WIDTH)
#define MODE_FULL_ACTIVE_H			(DISPLAY_ACTIVE_HEIGHT)

#define ARCH_PLAY_TIME				(10000)

typedef void (*LIVE_START_PROC) (void);

typedef struct _LIVESTART_T {
	LIVE_START_PROC	start;
} LIVESTART_T;

typedef enum {
	VSM_DIV1 	= 0,
	VSM_DIV4 	= 1,
	VSM_DIV6 	= 2,
	VSM_DIV8 	= 3,
	VSM_DIV9 	= 4,
	VSM_DIV16 	= 5,
	VSM_DIV32 	= 6,
	VSM_DIV36 	= 7,
	VSM_DIV_MAX
} VSM_DIV_E;

#if defined(GUI_4CH_SUPPORT)
#define VSM_DEFAULT_DIV VSM_DIV4
#elif defined(GUI_8CH_SUPPORT)
#define VSM_DEFAULT_DIV VSM_DIV9
#elif defined(GUI_16CH_SUPPORT)
#define VSM_DEFAULT_DIV VSM_DIV16
#else
#define VSM_DEFAULT_DIV VSM_DIV36
#endif

typedef enum {
	VSM_WIN_ID1 	= 0,
	VSM_WIN_ID2 	= 1,
	VSM_WIN_ID3 	= 2,
	VSM_WIN_ID4 	= 3,
	
	VSM_WIN_ID5 	= 4,
	VSM_WIN_ID6 	= 5,
	VSM_WIN_ID7 	= 6,
	VSM_WIN_ID8 	= 7,

	VSM_WIN_ID9 	= 8,
	VSM_WIN_ID10 	= 9,	
	VSM_WIN_ID11 	= 10,
	VSM_WIN_ID12 	= 11,
	VSM_WIN_ID13 	= 12,
	VSM_WIN_ID14 	= 13,
	VSM_WIN_ID15 	= 14,
	VSM_WIN_ID16 	= 15,

	VSM_WIN_ID17 	= 16,
	VSM_WIN_ID18 	= 17,	
	VSM_WIN_ID19 	= 18,
	VSM_WIN_ID20 	= 19,
	VSM_WIN_ID21 	= 20,
	VSM_WIN_ID22 	= 21,
	VSM_WIN_ID23 	= 22,
	VSM_WIN_ID24 	= 23,
	VSM_WIN_ID25 	= 24,
	VSM_WIN_ID26 	= 25,	
	VSM_WIN_ID27 	= 26,
	VSM_WIN_ID28 	= 27,
	VSM_WIN_ID29 	= 28,
	VSM_WIN_ID30 	= 29,
	VSM_WIN_ID31 	= 30,
	VSM_WIN_ID32 	= 31,
	VSM_WIN_ID33 	= 32,
	VSM_WIN_ID34 	= 33,
	VSM_WIN_ID35 	= 34,
	VSM_WIN_ID36 	= 35,

	VSM_WIN_MAX
} VSM_ID_E;


typedef enum _VDO_MODE_E
{
	VMODE_LV 	= 0,
	VMODE_PB 	= 1,
	VMODE_NONE
} VDO_MODE_E;


typedef enum _OSD_MODE_E
{
	OMODE_NORMAL 	= 0,
	OMODE_FULL 	 	= 1,
	OMODE_NONE
} OSD_MODE_E;

typedef enum _PLAYBACK_TYPE_E
{
	PLAYBACK_NORMAL 		= 0,
	PLAYBACK_PANO1	 		= 1,
	PLAYBACK_PANO2	 		= 2,
	PLAYBACK_TYPE_MAX
} PLAYBACK_TYPE_E;

typedef enum _ARCH_PLAY_TYPE_E
{
	ARCH_PLAY_MUL	= 1,
	ARCH_PLAY_AVI	= 2,
	ARCH_PLAY_TYPE_MAX
} ARCH_PLAY_TYPE_E;

// BWD = BACKWARD, FWD = FORWARD.
typedef enum _PLAYBACK_DIR_E
{
	DIR_DS_BWD = 0,
	DIR_BWD,
	DIR_PAUSE,
	DIR_FWD,
	DIR_DS_FWD
} PLAYBACK_DIR_E;


typedef enum _DIR_RATE_E
{
	DR_NONE 	= 0,
	DR_BWD_001 	= ((NF_PLAY_PARAM_DIR_BACKWARD << 16) | (NF_PLAY_PARAM_RATE_001)),
	DR_BWD_002 	= ((NF_PLAY_PARAM_DIR_BACKWARD << 16) | (NF_PLAY_PARAM_RATE_002)),
	DR_BWD_004 	= ((NF_PLAY_PARAM_DIR_BACKWARD << 16) | (NF_PLAY_PARAM_RATE_004)),
	DR_BWD_008 	= ((NF_PLAY_PARAM_DIR_BACKWARD << 16) | (NF_PLAY_PARAM_RATE_008)),
	DR_BWD_016 	= ((NF_PLAY_PARAM_DIR_BACKWARD << 16) | (NF_PLAY_PARAM_RATE_016)),
	DR_BWD_032 	= ((NF_PLAY_PARAM_DIR_BACKWARD << 16) | (NF_PLAY_PARAM_RATE_032)),
	DR_BWD_064 	= ((NF_PLAY_PARAM_DIR_BACKWARD << 16) | (NF_PLAY_PARAM_RATE_064)),
	DR_BWD_128 = ((NF_PLAY_PARAM_DIR_BACKWARD << 16) | (NF_PLAY_PARAM_RATE_128)),
	DR_FWD_001 	= ((NF_PLAY_PARAM_DIR_FORWARD << 16) | (NF_PLAY_PARAM_RATE_001)),
	DR_FWD_002 	= ((NF_PLAY_PARAM_DIR_FORWARD << 16) | (NF_PLAY_PARAM_RATE_002)),
	DR_FWD_004 	= ((NF_PLAY_PARAM_DIR_FORWARD << 16) | (NF_PLAY_PARAM_RATE_004)),
	DR_FWD_008 	= ((NF_PLAY_PARAM_DIR_FORWARD << 16) | (NF_PLAY_PARAM_RATE_008)),
	DR_FWD_016 	= ((NF_PLAY_PARAM_DIR_FORWARD << 16) | (NF_PLAY_PARAM_RATE_016)),
	DR_FWD_032 	= ((NF_PLAY_PARAM_DIR_FORWARD << 16) | (NF_PLAY_PARAM_RATE_032)),
	DR_FWD_064 	= ((NF_PLAY_PARAM_DIR_FORWARD << 16) | (NF_PLAY_PARAM_RATE_064)),
	DR_FWD_128 = ((NF_PLAY_PARAM_DIR_FORWARD << 16) | (NF_PLAY_PARAM_RATE_128)),

	DR_BWD_SLOW_002 = 0xffe2,	//((NF_PLAY_PARAM_DIR_BACKWARD << 16) | (NF_PLAY_PARAM_RATE_002)),
	DR_BWD_SLOW_004 = 0xffe3,	//((NF_PLAY_PARAM_DIR_BACKWARD << 16) | (NF_PLAY_PARAM_RATE_004)),
	DR_BWD_SLOW_008 = 0xffe4,	//((NF_PLAY_PARAM_DIR_BACKWARD << 16) | (NF_PLAY_PARAM_RATE_008)),
	DR_BWD_SLOW_016 = 0xffe5,	//((NF_PLAY_PARAM_DIR_BACKWARD << 16) | (NF_PLAY_PARAM_RATE_016)),
	DR_BWD_SLOW_032 = 0xffe6,	//((NF_PLAY_PARAM_DIR_BACKWARD << 16) | (NF_PLAY_PARAM_RATE_032)),
	DR_BWD_SLOW_064 = 0xffe7,	//((NF_PLAY_PARAM_DIR_BACKWARD << 16) | (NF_PLAY_PARAM_RATE_064)),
	DR_BWD_SLOW_128 = 0xffe8,	//((NF_PLAY_PARAM_DIR_BACKWARD << 16) | (NF_PLAY_PARAM_RATE_128)),	
	DR_FWD_SLOW_002 = 0xffe2,	//((NF_PLAY_PARAM_DIR_FORWARD << 16) | (NF_PLAY_PARAM_RATE_002)),
	DR_FWD_SLOW_004 = 0xffe3,	//((NF_PLAY_PARAM_DIR_FORWARD << 16) | (NF_PLAY_PARAM_RATE_004)),
	DR_FWD_SLOW_008 = 0xffe4,	//((NF_PLAY_PARAM_DIR_FORWARD << 16) | (NF_PLAY_PARAM_RATE_008)),
	DR_FWD_SLOW_016 = 0xffe5,	//((NF_PLAY_PARAM_DIR_FORWARD << 16) | (NF_PLAY_PARAM_RATE_016)),
	DR_FWD_SLOW_032 = 0xffe6,	//((NF_PLAY_PARAM_DIR_FORWARD << 16) | (NF_PLAY_PARAM_RATE_032)),
	DR_FWD_SLOW_064 = 0xffe7,	//((NF_PLAY_PARAM_DIR_FORWARD << 16) | (NF_PLAY_PARAM_RATE_064)),
	DR_FWD_SLOW_128 = 0xffe8,	//((NF_PLAY_PARAM_DIR_FORWARD << 16) | (NF_PLAY_PARAM_RATE_128)),

	DR_BWD_NEXT_FRAME = ((NF_PLAY_PARAM_MODE_NEXT_FRAME << 24) | (NF_PLAY_PARAM_DIR_BACKWARD << 16)),
	DR_FWD_NEXT_FRAME = ((NF_PLAY_PARAM_MODE_NEXT_FRAME << 24) | (NF_PLAY_PARAM_DIR_FORWARD << 16)),

	DR_PAUSE = (NF_PLAY_PARAM_MODE_PAUSE << 24),	
	DR_STOP 	= (NF_PLAY_PARAM_MODE_STOP << 24)
} DIR_RATE_E;


/////////////////////////////////////////////////////////////////////
//
//
//

// public interfaces
IUXAPI gint vsm_init(void);
IUXAPI gint vsm_start(void);


// live 
IUXAPI gint vsm_live_start(void);
IUXAPI gint vsm_live_stop(void);
IUXAPI gint vsm_live_preview_start(guint ch_mask, guint x, guint y, guint w, guint h);
IUXAPI gint vsm_live_preview_stop(void);
IUXAPI gint vsm_live_cmd_sequence(void);
IUXAPI gint vsm_live_preview_start_vca(gint ch, guint x, guint y, guint w, guint h);
IUXAPI gint vsm_live_preview_pause_vca(void);
IUXAPI gint vsm_live_preview_start_dva(gint ch, guint x, guint y, guint w, guint h);
IUXAPI gint vsm_live_preview_pause_dva(void);

// playback
IUXAPI gint vsm_playback_start(guint ch_mask, GTimeVal start_time, PLAYBACK_TYPE_E type);
IUXAPI gint vsm_preserve_playback_start(guint ch_mask, GTimeVal start_time, PLAYBACK_TYPE_E type, guint sess_id);
IUXAPI gint vsm_playback_stop(void);
IUXAPI gint vsm_playback_restart_by_time(GTimeVal time);
IUXAPI gint vsm_playback_restart_by_chtime(gint ch, GTimeVal time);
IUXAPI gint vsm_playback_preview_start(guint ch_mask, GTimeVal time, guint x, guint y, guint w, guint h);
IUXAPI gint vsm_playback_preview_stop(void);
IUXAPI gint vsm_playback_play_pause_by_menu_opened(void);
IUXAPI gint vsm_playback_play_recover_by_menu_closed(void);
IUXAPI gint vsm_playback_play_time_by_menu_closed(GTimeVal playtime);
IUXAPI DIR_RATE_E vsm_playback_get_dir_rate(void);
IUXAPI GTimeVal vsm_playback_get_playtime(void);
IUXAPI GTimeVal vsm_playback_get_previewtime(void);
IUXAPI gint vsm_playback_change_dir_rate(PLAYBACK_DIR_E dir);

// smart search
IUXAPI gint vsm_playback_smart_mainview_start(gint ch, GTimeVal time_start, GTimeVal time_end, NF_PLAY_SMART_SEARCH_MODE search_mode);
IUXAPI gint vsm_playback_smart_mainview_pause(NF_PLAY_SMART_SEARCH_MODE search_mode);
IUXAPI gint vsm_playback_smart_mainview_resume(NF_PLAY_SMART_SEARCH_MODE search_mode);
IUXAPI gint vsm_playback_smart_mainview_stop(NF_PLAY_SMART_SEARCH_MODE search_mode);
IUXAPI gint vsm_playback_smart_preview_start(gint ch, GTimeVal time);
IUXAPI gint vsm_playback_smart_preview_stop(void);
IUXAPI DIR_RATE_E vsm_playback_get_smart_dir_rate(void);
IUXAPI GTimeVal vsm_playback_get_smarttime(void);

// archived play
IUXAPI gint vsm_archived_play_start(guint ch_mask, time_t offset_time, ARCH_PLAY_TYPE_E type);
IUXAPI gint vsm_archived_play_start_ex(guint ch_mask, time_t offset_time, ARCH_PLAY_TYPE_E type);
IUXAPI gint vsm_archived_play_stop(void);

// from VIEWER interfaces

IUXAPI LIVESTART_T *vsm_create_livestart_obj();
IUXAPI int vsm_destroy_livestart_obj(LIVESTART_T *lst);
IUXAPI VDO_MODE_E vsm_get_vmode(void);
IUXAPI OSD_MODE_E vsm_get_omode(void);
IUXAPI VSM_DIV_E vsm_get_div(void);
IUXAPI gchar vsm_get_focused_channel(void);

IUXAPI gint vsm_change_full_mode(void);
IUXAPI gint vsm_change_normal_mode(void);

IUXAPI gint vsm_osd_on(void);
IUXAPI gint vsm_osd_off(void);
IUXAPI gint vsm_osd_refresh(void);

IUXAPI gint vsm_draw_focus_win(gchar win_id);
IUXAPI gint vsm_show_shortcut_menu(guint pos_x, guint pos_y, gchar win_id);

IUXAPI gint vsm_change_sfc_by_ch(gchar ch);
IUXAPI gint vsm_change_sfc_by_array(gchar win_id[VSM_WIN_MAX], VSM_DIV_E dtype);
//IUXAPI gint vsm_change_sfc_by_div(VSM_DIV_E dtype);
IUXAPI gint vsm_has_next_arch_ch();
IUXAPI gint vsm_change_sfc_arch_play(void);
IUXAPI gint vsm_change_sfc_by_2button_press(gchar win_id);

IUXAPI gint vsm_change_div_by_user(gint valid_idx);

IUXAPI gint vsm_change_sfc_by_menu_opened(gchar ch);
IUXAPI gint vsm_recover_sfc_by_menu_closed(void);
IUXAPI gint vsm_change_sfc_by_zoom_opened(gchar ch);
IUXAPI gint vsm_recover_sfc_by_zoom_closed(void);

IUXAPI gint vsm_set_covert_by_user(gchar *user_name);
IUXAPI gint vsm_set_covert_by_logout(void);

IUXAPI gint vsm_mouse_motion_detect(guint x, guint y);
IUXAPI gchar vsm_get_current_ch(gchar win_id);
IUXAPI gint vsm_get_covert_state(gchar state[GUI_CHANNEL_CNT], guint ch);
IUXAPI gint vsm_untilkey_stop();

IUXAPI gint vsm_switch_select_channel(gint *data);
IUXAPI gint vsm_switch_channel(gint from_ch, gint to_ch);


IUXAPI void vsm_turn_on_alarm_status();
IUXAPI void vsm_turn_off_alarm_status();


IUXAPI gint vsm_playback_change_dir_rate(PLAYBACK_DIR_E dir);
IUXAPI void vsm_playback_step_backward();
IUXAPI void vsm_playback_step_forward();
IUXAPI void vsm_playback_run_step_backward();
IUXAPI void vsm_playback_stop_step_backward();
IUXAPI void vsm_playback_run_step_forward();
IUXAPI void vsm_playback_stop_step_forward();


IUXAPI void vvm_notify_cam_title_change(NF_NOTIFY_INFO *data);
IUXAPI void vvm_notify_osd(NF_NOTIFY_INFO *data);
IUXAPI void vvm_notify_alarm_text_change(NF_NOTIFY_INFO *data);
IUXAPI void vvm_notify_analog_rec(NF_NOTIFY_INFO *data);
IUXAPI void vvm_notify_audio_status(NF_NOTIFY_INFO *data);
IUXAPI void vvm_notify_mic_status(NF_NOTIFY_INFO *data);
IUXAPI void vvm_notify_video_loss(NF_NOTIFY_INFO *data);
IUXAPI void vvm_notify_no_video(NF_NOTIFY_INFO *data);
IUXAPI void vvm_notify_pnd(NF_NOTIFY_INFO *data);
IUXAPI void vvm_notify_pnd_rate(NF_NOTIFY_INFO *data);
IUXAPI void vvm_notify_covert(NF_NOTIFY_INFO *data);
IUXAPI void vvm_notify_net_client(NF_NOTIFY_INFO *data);
IUXAPI void vvm_notify_disk_usage(NF_NOTIFY_INFO *data);
IUXAPI void vvm_notify_disk_full(NF_NOTIFY_INFO *data);
IUXAPI void vvm_notify_alarm(NF_NOTIFY_INFO *data);
IUXAPI void vvm_notify_motion(NF_NOTIFY_INFO *data);
IUXAPI void vvm_notify_motion_mark(NF_NOTIFY_INFO *data);
IUXAPI void vvm_notify_net_client(NF_NOTIFY_INFO *data);
IUXAPI void vvm_notify_net_rxtx(NF_NOTIFY_INFO *data);
IUXAPI void vvm_notify_disk_full(NF_NOTIFY_INFO *data);
IUXAPI void vvm_notify_disk_usage(NF_NOTIFY_INFO *data);
IUXAPI void vvm_notify_disk_overwrite(NF_NOTIFY_INFO *data);
IUXAPI void vvm_notify_disk_smart(NF_NOTIFY_INFO *data);
IUXAPI void vvm_notify_fan(NF_NOTIFY_INFO *data);
IUXAPI void vvm_notify_set_ip_conflict(NF_NOTIFY_INFO *data);
IUXAPI void vvm_notify_cam_ip_conflict(NF_NOTIFY_INFO *data);
IUXAPI void vvm_notify_temperature(NF_NOTIFY_INFO *data);
IUXAPI void vvm_notify_poe(NF_NOTIFY_INFO *data);
IUXAPI void vvm_notify_disk_exhaust(NF_NOTIFY_INFO *data);
IUXAPI void vvm_notify_no_disk(NF_NOTIFY_INFO *data);
IUXAPI void vvm_gear_draw(guint ch);
IUXAPI void vvm_gear_delete(guint ch);
IUXAPI void vvm_set_vca_select_icon(gchar win_id, gint sel_idx);
IUXAPI void vvm_set_ipcam_zig_info(CMM_MESSAGE_T *pmsg);

IUXAPI gint vsm_func_send_motion_status(NF_NOTIFY_INFO *data);
IUXAPI gint vsm_func_send_alarm_status(NF_NOTIFY_INFO *data);
IUXAPI gint vsm_func_send_vca_status(NF_NOTIFY_INFO *data);
IUXAPI gint vsm_func_send_dva_status(NF_NOTIFY_INFO *data);
IUXAPI gint vsm_func_send_dvabx_status(NF_NOTIFY_INFO *data);
IUXAPI gint vsm_func_send_pos_status(NF_NOTIFY_INFO *data);

IUXAPI gint vsm_func_send_vcaevent_status(NF_NOTIFY_INFO *data);
IUXAPI void vvm_notify_deeplearning_counter_notify(NF_NOTIFY_INFO *data);
IUXAPI void vvm_notify_deeplearning_counter_property(NF_NOTIFY_INFO *data);
IUXAPI void vvm_notify_ai_keepalive(NF_NOTIFY_INFO *data);
IUXAPI void vsm_keypad_event(KEYPAD_KID key_id);
IUXAPI void vsm_jog_event(JOGID jog_id);
IUXAPI void vsm_shuttle_event(SHUTTLEID shuttle_id);
IUXAPI void vsm_ipcam_zig_event_press(KEYPAD_KID key_id);
IUXAPI void vsm_ipcam_zig_event_release(KEYPAD_KID key_id);

gboolean vsm_is_focus_win(void);

#endif /* _VSM_H_ */

