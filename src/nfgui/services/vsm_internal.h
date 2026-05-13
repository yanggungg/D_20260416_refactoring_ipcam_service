/*
 * vsm_internal.h
 *  - vsm internal header file
 *  - not exposed to outside
 *  - dependency :
 *
 * Written by Jungkyu Park. <parangi@itxsecurity.com>
 * Copyright (c) ITX security, Jan 10, 2011
 *
 */

#ifndef __VSM_INTERNAL_H
#define __VSM_INTERNAL_H

#include "nf_common.h"
#include "../support/event_loop.h"
#include "cmm.h"

#include "vsm.h"
#include "vw_vwnd.h"
#include "nf_api_play.h"


#if defined(GUI_4CH_SUPPORT)
#define DEFAULT_DIV_MODE        (VSM_DIV4)
#elif defined(GUI_8CH_SUPPORT)
#define DEFAULT_DIV_MODE        (VSM_DIV9)
#elif defined(GUI_16CH_SUPPORT)
#define DEFAULT_DIV_MODE        (VSM_DIV16)
#else
#define DEFAULT_DIV_MODE        (VSM_DIV36)
#endif


typedef struct _DispClayoutElement_t DispClayoutElement_t;
struct _DispClayoutElement_t {
	VSM_DIV_E div;
	guint conf[32];
};

typedef struct _DispCstlayout_t DispCstlayout_t;
struct _DispCstlayout_t {
	gint div1_item_cnt;
	DispClayoutElement_t div1_items[32];
	gint div4_item_cnt;
	DispClayoutElement_t div4_items[32];
	gint div9_item_cnt;
	DispClayoutElement_t div9_items[32];
	gint div16_item_cnt;
	DispClayoutElement_t div16_items[32];
	gint div36_item_cnt;
	DispClayoutElement_t div36_items[32];
	gint div6_item_cnt;
	DispClayoutElement_t div6_items[32];
	gint div8_item_cnt;
	DispClayoutElement_t div8_items[32];
};


////////////////////////////////////////////////////////////
//
// protected data type 
//

typedef struct _CINFO_T
{
    gchar           win_id;
    gboolean        covert;
} CINFO_T;

typedef struct _SFC_T
{
    CINFO_T         cinfo[VSM_WIN_MAX];
    VSM_DIV_E       div;
    guint           x;
    guint           y;
    guint           w;
    guint           h;  
} SFC_T;

typedef struct _DS_T {                  // sfc recover
    gchar           win_id[VSM_WIN_MAX];
    VSM_DIV_E       div;    
	gint			cstlayout_idx;			// custom layout idx
	DispClayoutElement_t *cstlayout_elm;			// custom layout elm
} DS_T; 

typedef struct _SSM_T {
    SFC_T           sfc;                // current
    DS_T            mul_info;           // for mouse double click event.(multi info)    
    OSD_MODE_E      omode;              // osd mode : full or normal
    VDO_MODE_E      vmode;              // video mode : live or playback
	gint			cstlayout_idx;			// custom layout idx
	DispClayoutElement_t *cstlayout_elm;	// custom layout elm
} SSM_T;

typedef struct _VSM_T {
    SSM_T           ssm;                // current display mode
    SSM_T           ssm_lv;             // live mode backup
    SSM_T           ssm_pb;             // playback mode  backup
    DS_T            rc_info;            // recover after vsm func
    gboolean        is_rc;          
    gchar           focus_win;          // for mouse click focus
    gchar           dewarp_ch;         // latest fisheye dewarping channel
    
    guint           lvs_tid;            // status timer of live mode
    guint           pbs_tid;            // status timer of playback mode
    guint           preview_tid;        // status timer of preview
    guint           vca_tid;            // vca timer of vwnd display
    guint           dvabx_tid;            // dvabx timer of vwnd display
    guint           pos_tid;            // pos timer of vwnd display    
    guint           smart_tid;          // status timer of smart playback stream

    guint           play_ch_mask;
    guint           play_type;          // video type : normal, preview, pano1, pano2...
    GTimeVal        play_time;          // play time of playback mode
    GTimeVal        preview_time;       // play time of preview
    GTimeVal        smart_time;         // play time of smart playback stream
    DIR_RATE_E      smart_dir_rate;

    LOGCTX          log_ctx;
    GTimeVal        pos_time;           // pos search time of playback mode

    DIR_RATE_E      play_dir_rate;      // play dir rate if playback mode
    gboolean        is_played;

    GTimeVal        prev_play_time;     // previous play time
    IMSG            ret_msg_play;       // return message for play cmd
} VSM_T;



typedef struct _VOM_PLAY_STATUS_T
{
    GTimeVal            play_time;
    NF_PLAY_STATUS_E    play_status[GUI_CHANNEL_CNT];
    guchar              play_mode;
    guchar              play_dir;
    guchar              play_rate;
    guchar              speed_flag;
    gboolean            is_play_stop;
} VOM_PLAY_STATUS;



typedef enum _LV_TYPE_E
{
    LV_NORMAL       = 0,
    LV_PREVIEW
} LV_TYPE_E;

typedef enum _PB_TYPE_E
{
    PB_NORMAL           = 0,
    PB_PANO1            = 1,
    PB_PANO2            = 2,
    PB_ARCH_MUL         = 3,
    PB_ARCH_AVI         = 4,
    PB_PREVIEW      
} PB_TYPE_E;


////////////////////////////////////////////////////////////
//
// protected interfaces
//


// VSM module interfaces
gchar _vsm_get_ch(gchar win_id);
gchar _vsm_get_win(gchar ch);
void _vsm_set_recover_sfc_by_func();
void _vsm_reset_recover_sfc_by_func();
void _vsm_backup_recover_info();
guint _vsm_get_screen_ch_mask(void);
gboolean _vsm_copy_current_sfc(SFC_T *psfc);

// VSM - live
gboolean _vsm_live_set_livestatus(gpointer data);
void _vsm_live_reset_livestatus(gpointer data);
void _vsm_start_panic_duration();
void _vsm_stop_panic_duration();
void _vsm_open_sensor_osd_popup(gint ch);
void _vsm_open_system_osd_popup(guint evt_type, gchar *str);
void _vsm_open_disk_osd_popup(guint evt_type, gchar *str);
void _vsm_close_osd_popup();

// VSM - playback
gboolean _vsm_playback_set_playstatus(gpointer data);
void _vsm_playback_reset_playstatus(gpointer data);
gboolean _vsm_preview_set_playtime(gpointer data);
void _vsm_preview_reset_playtime(gpointer data);
gboolean _vsm_smart_set_playtime(gpointer data);
void _vsm_smart_reset_playtime(gpointer data);

// VSM - func
gint _vsm_func_start(void);
gint _vsm_func_run_popup(void);
gint _vsm_func_stop_popup(void);            // empty all slot(include recover slot)
gint _vsm_func_stop_event_popup(void);      // empty event slot(except recover slot)
gint _vsm_func_popup_is_until_key();
gint _vsm_func_recover_popup();
gboolean _vsm_func_sequence_stop(void);
int _vsm_ack_play_cmd();
gboolean _vsm_func_get_popup_run();

// VSM - cstlayout
gint _vsm_init_cstlayout();
gint _vsm_save_cstlayout();
gint _vsm_get_cstlayout_itemcnt(VSM_DIV_E dtype);
DispClayoutElement_t *_vsm_get_cstlayout_element(VSM_DIV_E dtype, gint cst_idx);
gint _vsm_set_cstlayout_idx(VSM_DIV_E dtype, gint cst_idx);
gint _vsm_increase_cstlayout_idx(VSM_DIV_E dtype, gint cst_idx);
gint _vsm_decrease_cstlayout_idx(VSM_DIV_E dtype, gint cst_idx);
gint _vsm_get_sfc_cstlayout(SFC_T *psfc, DispClayoutElement_t *elm);
gint _vsm_set_sfc_cstlayout(SFC_T *psfc, DispClayoutElement_t *elm);
gint _vsm_is_matched_cstlayout(SFC_T *psfc, DispClayoutElement_t *elm);
gint _vsm_is_channel_check_cstlayout(DispClayoutElement_t *elm, gint check_ch);

// VVM module interfaces
void _vvm_init();
void _vvm_set_vwnd_sfc_mode(VDO_MODE_E mode);
void _vvm_draw_win_focus(VSM_ID_E win_id);
void _vvm_set_clear_vwnd(void);
void _vvm_unset_clear_vwnd(void);
void _vvm_set_blind_vwnd(void);
void _vvm_unset_blind_vwnd(void);
void _vvm_live_change_osd(SFC_T *psfc);
void _vvm_live_osd_on(void);
void _vvm_live_osd_off(void);
void _vvm_live_set_seq_icon(void);
void _vvm_live_unset_seq_icon(void);
gint _vvm_live_set_current_time();
gint _vvm_live_set_user(gchar uid[VWND_TEXT_LEN]);
void _vvm_live_reset_livestatus(void);
void _vvm_playback_change_osd(SFC_T *psfc);
void _vvm_playback_osd_on(void);
void _vvm_playback_osd_off(void);
gint _vvm_playback_set_video_status(gint ch, NF_PLAY_STATUS_E status);
gint _vvm_playback_set_dir_rate(DIR_RATE_E status);
gint _vvm_playback_set_playtime(GTimeVal time);
void _vvm_playback_reset_playstatus(void);
gint _vvm_set_alarm_status(gboolean alarm_on);
gboolean _vvm_vca_draw_invalid_region(gpointer data);
gboolean _vvm_dvabx_draw_invalid_region(gpointer data);
gboolean _vvm_pos_draw_invalid_region(gpointer data);
void _vvm_refresh_seq_icon(void);
void _vvm_refresh_queue_draw(void);
void _vvm_refresh_process_updates(void);


// VOM module interfaces
void _vom_init();
void _vom_live_start_video(SFC_T *psfc, LV_TYPE_E type);
void _vom_live_change_video(SFC_T *psfc);
void _vom_live_stop_video(void);
void _vom_playback_start_video(SFC_T *psfc, GTimeVal tv, PB_TYPE_E type);
void _vom_preserve_playback_start_video(SFC_T *psfc, GTimeVal tv, PB_TYPE_E type, guint sess_id);
void _vom_playback_change_video(SFC_T *psfc);
void _vom_playback_stop_video(void);
void _vom_playback_get_playstatus(VOM_PLAY_STATUS *set_status);
void _vom_playback_cmd_step_backward(gint step_time);
void _vom_playback_cmd_fast_backward(void);
void _vom_playback_cmd_backward(void);
void _vom_playback_cmd_pause(void);
void _vom_playback_cmd_forward(void);
void _vom_playback_cmd_fast_forward(void);
void _vom_playback_cmd_step_forward(gint step_time);

void _vom_playback_cmd_nextframe_backward(void);
void _vom_playback_cmd_nextframe_forward(void);
void _vom_playback_set_hold(guint status);
guint _vom_playback_get_hold(void);
void _vom_playback_cmd_force_pause(void);
void _vom_playback_cmd_force_backward_64(void);
void _vom_playback_cmd_force_backward_32(void);
void _vom_playback_cmd_force_backward_16(void);
void _vom_playback_cmd_force_backward_08(void);
void _vom_playback_cmd_force_backward_04(void);
void _vom_playback_cmd_force_backward_02(void);
void _vom_playback_cmd_force_backward_01(void);
void _vom_playback_cmd_force_forward_64(void);
void _vom_playback_cmd_force_forward_32(void);
void _vom_playback_cmd_force_forward_16(void);
void _vom_playback_cmd_force_forward_08(void);
void _vom_playback_cmd_force_forward_04(void);
void _vom_playback_cmd_force_forward_02(void);
void _vom_playback_cmd_force_forward_01(void);
gboolean _vom_playback_play_pause(void);
void _vom_playback_play_start_after_pause(void);
void _vom_playback_play_start_after_pause_time(GTimeVal playtime);
void _vom_playback_keypad_event(KEYPAD_KID key_id);
void _vom_playback_jog_event(JOGID jog_id);
void _vom_playback_shuttle_event(SHUTTLEID shuttle_id);
void _vom_playback_start_archived_video(SFC_T *psfc, time_t offset, PB_TYPE_E type);
void _vom_playback_start_smart_video(SFC_T *psfc, GTimeVal tv_start,GTimeVal tv_end, NF_PLAY_SMART_SEARCH_MODE search_mode);
void _vom_playback_pause_smart_video(NF_PLAY_SMART_SEARCH_MODE search_mode, gboolean pause);
void _vom_playback_stop_smart_video(NF_PLAY_SMART_SEARCH_MODE search_mode);
void _vom_playback_get_smart_playstatus(VOM_PLAY_STATUS *set_status);

#endif
