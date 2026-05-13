#ifndef	__NF_UI_FUNCTION_H__
#define	__NF_UI_FUNCTION_H__

#include "../support/nf_ui_page_manager.h"


#define NFFUNC_POWEROFF_NORMAL	0
#define NFFUNC_POWEROFF_REBOOT  1

enum {
	NFLED_CH1 = 0,
	NFLED_CH2,
	NFLED_CH3,
	NFLED_CH4,
	NFLED_CH5,
	NFLED_CH6,
	NFLED_CH7,
	NFLED_CH8,
	NFLED_CH9,
	NFLED_CH10,
	NFLED_CH11,
	NFLED_CH12,
	NFLED_CH13,
	NFLED_CH14,
	NFLED_CH15,
	NFLED_CH16,
	NFLED_UNKNOWN,
	NFLED_DISPLAY,
	NFLED_SEQUENCE,
	NFLED_PANIC,
	NFLED_ZOOM,
	NFLED_LOCK,
	NFLED_ARCHIVE,
	NFLED_PTZ,
	NFLED_SETUP,
	NFLED_SEARCH,
	NFLED_LEFT_ARROW,
	NFLED_UP_ARROW,
	NFLED_RIGHT_ARROW,
	NFLED_RETURN,
	NFLED_DOWN_ARROW,
	NFLED_ENTER,
	NFLED_REW,
	NFLED_BACKWARD,
	NFLED_PAUSE,
	NFLED_FORWARD,
	NFLED_FF,
	NFLED_POWER,
	NFLED_RECORDING,
	NFLED_NETWORK,
	NFLEDS
};

#if defined(_MULTI_POPUP_)
#define DIV1_MAX_CH_CNT   (1)
#define DIV4_MAX_CH_CNT   (4)
#define DIV9_MAX_CH_CNT   (9)
#define DIV16_MAX_CH_CNT  (16)

typedef enum
{
	EVENT_POPUP_DISABLE = 0,
	EVENT_POPUP_ENABLE = 1,
	EVENT_POPUP_BLOCKING = 2,
}event_popup_enable_type;

guint get_event_popup_dwell_cnt(guint ch);
void set_event_popup_dwell_cnt(guint ch, guint dwell_cnt);
gboolean get_event_popup_state(guint ch);
void set_event_popup_enable(event_popup_enable_type enable);
event_popup_enable_type get_event_popup_enable(void);
void event_popup_end(gboolean pre_disp_state);
void change_event_display_popup_mode(guint view_ch_cnt, gint start_ch_idx, gint end_ch_idx, gboolean event_ch_change, gboolean dura_end);
guint get_event_alarm_db_ch(guint ch);
void popup_event_action(void);
gboolean get_popup_timer_on(void);
#endif

void nffunc_powerOff( gboolean is_reboot);
void nffunc_reboot();
void nf_ui_Set_Color_Table();
void nffunc_auto_logout_init();
void nffunc_almmot_popup_init();
void nffunc_almmot_popup_term();
void nffunc_almmot_popup_ignore();
gboolean nffunc_live_audio_init();
void nffunc_beep_on_off();
gint nffunc_change_led(PAGEID pgid);
gint nffunc_change_led_by_mask(guint hmask, guint lmask);
void nffunc_led_on(guint idx);
void nffunc_led_off(guint idx);
void nffunc_ddns_hostname_init();

#endif	// __NF_UI_FUNCTION_H__


