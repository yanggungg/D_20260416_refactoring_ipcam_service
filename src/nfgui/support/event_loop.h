#ifndef	__EVENT_LOOP_H__
#define	__EVENT_LOOP_H__

#include "../viewers/objects/nfobject.h"
#include "../viewers/objects/nfwindow.h"
#include "iux_msg.h"

#if defined(__OTM_UI__)
#define NF_MAIN_EXIT_FOCUS 0 // OTM
#define NF_MAIN_MENU_FOCUS 1
#define NF_MAIN_SUB_FOCUS 2
#endif

typedef enum {
	KEYPAD_CH01		= 0x00,
	KEYPAD_CH02		= 0x01,
	KEYPAD_CH03		= 0x02,
	KEYPAD_CH04		= 0x03,
	KEYPAD_CH05		= 0x04,
	KEYPAD_CH06		= 0x05,
	KEYPAD_CH07		= 0x06,
	KEYPAD_CH08		= 0x07,
	KEYPAD_CH09		= 0x08,
	KEYPAD_CH10		= 0x09,
	KEYPAD_CH11		= 0x0A,
	KEYPAD_CH12		= 0x0B,
	KEYPAD_CH13		= 0x0C,
	KEYPAD_CH14		= 0x0D,
	KEYPAD_CH15		= 0x0E,
	KEYPAD_CH16		= 0x0F,
	KEYPAD_CH17		= 0x2B,
	KEYPAD_CH18		= 0x2C,
	KEYPAD_CH19		= 0x2D,
	KEYPAD_CH20		= 0x2E,
	KEYPAD_CH21		= 0x2F,
	KEYPAD_CH22		= 0x30,
	KEYPAD_CH23		= 0x31,
	KEYPAD_CH24		= 0x32,
	KEYPAD_CH25		= 0x33,
	KEYPAD_CH26		= 0x34,
	KEYPAD_CH27		= 0x35,
	KEYPAD_CH28		= 0x36,
	KEYPAD_CH29		= 0x37,
	KEYPAD_CH30		= 0x38,
	KEYPAD_CH31		= 0x39,
	KEYPAD_CH32		= 0x3A,
	KEYPAD_POWER	= 0x10,
	KEYPAD_DISP		= 0x11,
	KEYPAD_SEQ		= 0x12,
	KEYPAD_PANIC	= 0x13,
	KEYPAD_ZOOM		= 0x14,
	KEYPAD_LOCK		= 0x15,
	KEYPAD_ARCH		= 0x16,
	KEYPAD_PTZ		= 0x17,
	KEYPAD_SETUP	= 0x18,
	KEYPAD_SEARCH	= 0x19,
	KEYPAD_LEFT		= 0x1A,
	KEYPAD_UP		= 0x1B,
	KEYPAD_RIGHT	= 0x1C,
	KEYPAD_EXIT		= 0x1D,
	KEYPAD_DOWN		= 0x1E,
	KEYPAD_ENTER	= 0x1F,
	KEYPAD_REW		= 0x20,
	KEYPAD_BACKWARD	= 0x21,
	KEYPAD_PAUSE	= 0x22,
	KEYPAD_FORWARD	= 0x23,
	KEYPAD_FF		= 0x24,
	KEYPAD_HOLD		= 0x25,
	KEYPAD_FREEZE	= 0x26,
	KEYPAD_TEN		= 0x27,
	KEYPAD_CH00		= 0x28,
	KEYPAD_DEBUG    = 0x29,
	KEYPAD_NONE		= 0xFF,
	NUM_KEYPAD		= 0x26,
	RMC_MENU		= 0x2A,
	RMC_NUM_0		= 0x50,
	RMC_DEV_ID		= 0x51,

	KEYPAD_STOP		= 0x52,
	KEYPAD_BACKWARD_PLAY	= 0x53,
	KEYPAD_FORWARD_PLAY		= 0x54,
	KEYPAD_RELAY	= 0x55,		


#if defined(__XRPLUS_UI__) 
	// xr+ remocon
	RMC_MODE		= 0x60,
	RMC_DVR			= 0x61,
	RMC_DVI			= 0x62,
	RMC_COMPONENT	= 0x63,
	RMC_PIP			= 0x64,
	RMC_INFO		= 0x65,
	RMC_VOLUME_UP	= 0x66,
	RMC_VOLUME_DOWN	= 0x67,
	RMC_AUDIO		= 0x68,
	RMC_PIC_FR		= 0x69,
#else
	RMC_COMPONENT	= 0x60,
	RMC_INFO		= 0x61,
	RMC_PIP			= 0x62,
	RMC_VOLUME_UP	= 0x63,
	RMC_VOLUME_DOWN	= 0x64,
	RMC_AUDIO		= 0x65,
	RMC_PIC_FR		= 0x66,
	RMC_DVI			= 0x67,
	RMC_MODE		= 0x68,
	RMC_DVR			= 0x69,
#endif

	// IPX
	RMC_LOGOUT		= 0x70,
	RMC_ALARM		= 0x71,
	RMC_LOG			= 0x72,
	RMC_SNAPSHOT	= 0x73,
	//RMC_MENU		= 0x74,
	RMC_ZOOMIN		= 0x75,
	RMC_ZOOMOUT		= 0x76,
	RMC_NEAR		= 0x77,
	RMC_FAR			= 0x78,
	RMC_PRESET		= 0x79,
	RMC_AFOCUS		= 0x7A,
	RMC_RESERVE		= 0x7B,
	RMC_BJUMP		= 0x7C,
	RMC_FJUMP		= 0x7D,

	JOY_STOP		= 0x90,
	JOY_LEFT		= 0x91,
	JOY_RIGHT		= 0x92,
	JOY_UP			= 0x93,
	JOY_DOWN		= 0x94,
	JOY_TURN_LEFT	= 0x95,
	JOY_TURN_RIGHT	= 0x96,
	
} KEYPAD_KID;


typedef enum {
	JOG_CCW	= 0,
	JOG_CW	= 1,

	JOG_NONE = 0xFF,
	NUM_JOG_ID = 2
} JOGID;

typedef enum {
	SHUTTLE_CENTER = 0x00,
	SHUTTLE_CW_1 = 0x04,
	SHUTTLE_CW_2 = 0x06,
	SHUTTLE_CW_3 = 0x02,
	SHUTTLE_CW_4 = 0x0A,
	SHUTTLE_CW_5 = 0x0E,
	SHUTTLE_CW_6 = 0x0C,
	SHUTTLE_CW_7 = 0x08,
	SHUTTLE_CCW_1 = 0x01,
	SHUTTLE_CCW_2 = 0x05,
	SHUTTLE_CCW_3 = 0x07,
	SHUTTLE_CCW_4 = 0x03,
	SHUTTLE_CCW_5 = 0x0B,
	SHUTTLE_CCW_6 = 0x0F,
	SHUTTLE_CCW_7 = 0x0D,
	SHUTTLE_CCW_8 = 0x09,

	SHUTTLE_JOG_ID  = 16,
	SHUTTLE_NONE = 0xFF,
} SHUTTLEID;


#define	NFEVENT_CALENDAR_CHANGED	NFEVENT_CALENDAR_CHANGED_PRESS
#define	NFEVENT_TIMESPIN_CHANGED	NFEVENT_TIMESPIN_CHANGED_RELEASE
#define	NFEVENT_SPINBUTTON_CHANGED	NFEVENT_SPINBUTTON_CHANGED_PRESS
#define	NFEVENT_LISTBOX_CHANGED	    NFEVENT_LISTBOX_CHANGED_PRESS

typedef enum {
	NFEVENT_USER = 100,
	NFEVENT_CALENDAR_CHANGED_PRESS,
	NFEVENT_CALENDAR_CHANGED_RELEASE,
	NFEVENT_CALENDAR_MONTH_CHANGED,
	NFEVENT_HTIMELINE_TIMEROW_DRAG_RELEASE,
	NFEVENT_SETUP_MAINMENU_CHANGED_RELEASE,
	NFEVENT_SETUP_SUBMENU_CHANGED_RELEASE,
	NFEVENT_TIMESPIN_CHANGED_PRESS,
	NFEVENT_TIMESPIN_CHANGED_RELEASE,
	NFEVENT_SPINBUTTON_CHANGED_PRESS,
	NFEVENT_SPINBUTTON_CHANGED_RELEASE,
	NFEVENT_TIMELINE_CHANGED,
	NFEVENT_TILE_INIT,
	NFEVENT_TILE_START_SELECT,
	NFEVENT_TILE_END_SELECT,
	NFEVENT_TILE_MOVE_SELECT,
	NFEVENT_CHECKBUTTON_CHANGED,
	NFEVENT_TAB_BEFORE_CHANGE,
	NFEVENT_TAB_CHANGED,
	NFEVENT_COMBOBOX_CHANGED,
	NFEVENT_KEYFOCUS_CHANGED,
	NFEVENT_RADIO_LOST_FOCUS,
	NFEVENT_RADIO_GET_FOCUS,
	NFEVENT_RADIO_GET_FOCUS_ALWAYS,

	NFEVENT_KEYPAD_PRESS,
	NFEVENT_KEYPAD_RELEASE,
	NFEVENT_REMOCON_PRESS,
	NFEVENT_REMOCON_RELEASE,
	NFEVENT_JOG_CHANGE,
	NFEVENT_SHUTTLE_CHANGE,

	NFEVENT_485_INPUT,
	NFEVENT_485_NOINPUT,


	NFOUTEVT_BUTTON_PRESS,
	NFOUTEVT_BUTTON_RELEASE,
	NFOUTEVT_2BUTTON_PRESS,
	NFOUTEVT_MOTION_NOTIFY,
	NFOUTEVT_SCROLL,

    NFEVENT_SPINBUTTON_ENTERKEY_PRESS,
	NFEVENT_TML_RULER_DRAGUP,
	NFEVENT_TML_RULER_DRAGDOWN,
    NFEVENT_THUMBNAIL_DRAW,
    NFEVENT_CWSLIDER_CHANGED_RELEASE,
	NFEVENT_TML_CHANGE_DATE,
	NFEVENT_EXIT_PLAYBACK,

	WND_CREATED,
	WND_PRE_CLOSE,
	WND_CLOSE,		// mutiple called in parent which has a child window
	WND_CLOSED,
	WND_HIDE,

	NFEVENT_BARGRAPH_SELECTED, // bargraph selected
	NFEVENT_BARGRAPH_2BUTTON_PRESS,
	NFEVENT_BARGRAPH_MOTION_NOTIFY,

	NFEVENT_VCA_STATISTIC_COMPONENT_DATA_SYNC,
	NFEVENT_VCA_STATISTIC_AVERAGE_DATA_SYNC,

	NFEVENT_IPCAMSETUP_DB_SYNC,
	NFEVENT_LISTBOX_CHANGED_PRESS,
    NFEVENT_SCROLLED_FIXED_MOVE,
	NFEVENT_VCAREV_COMPONENT_DATA_SYNC,
	NFEVENT_VCAREV_COMPONENT_DATA_EXPOSE,	
	NFEVENT_VCAREV_COMPONENT_PREVIEW_SYNC,
	NFEVENT_DVA_COMPONENT_DATA_SYNC,
	NFEVENT_DVA_COMPONENT_DATA_EXPOSE,	
	NFEVENT_DVA_COMPONENT_PREVIEW_SYNC,

	NFEVENT_SNMP_RADIO_BUTTON_PRESS,
	NFEVENT_DEBUG_INFOR_DATA_SYNC,

	NFEVENT_CANCEL,
	NFEVENT_USER_END
} nfevent_type;

#if defined(__XRPLUS_UI__)
typedef enum {
	AUDIO_CH_CHANGE_OFF = 0,
	AUDIO_CH_CHANGE_ON
} AUDIO_CH_CHANGE_MODE;

#endif

void nfui_run_main_event_handler(NFOBJECT *main_window);
void nfui_regi_pre_event_callback(NFOBJECT *obj, gpointer func);
void nfui_regi_post_event_callback(NFOBJECT *obj, gpointer func);
void nfui_send_event(NFOBJECT *obj, GdkEvent *event, gboolean loop);
void nfui_signal_emit(NFOBJECT *obj, GdkEventType type, gboolean loop);
void nfui_signal_emit_data(NFOBJECT *obj, IMSG msg, gboolean loop, gpointer data);
void nfui_user_signal_emit(NFOBJECT *obj, nfevent_type type, gboolean loop);
void nfui_left_button_signal_emit(NFOBJECT *obj, GdkEventType type, gboolean loop);

int nfui_make_key_hierarchy(NFWINDOW *top);
void nfui_free_key_hierarchy(KEYOBJECT* key_obj);
int nfui_set_key_focus(NFOBJECT *obj, gboolean set);
void nfui_clear_key_focus(NFOBJECT *obj);
gboolean nfui_is_focus_at_child(NFOBJECT* obj);
NFOBJECT* nfui_get_cur_focus(NFOBJECT *obj);

int init_event_loop();
gboolean nfui_keypad_init();
gboolean nfui_remocon_init();
gboolean nfui_jog_init();
gboolean nfui_shuttle_init();
gboolean nfui_485_init();
gboolean nfui_mouse_init();

void nfui_idle_timer_init();
void nfui_idle_timer_reset();
gdouble nfui_get_idle_timer_elapsed();
void nfui_idle_timer_destroy();

#if defined(__XRPLUS_UI__)
void nfui_set_audio_ch_mode(AUDIO_CH_CHANGE_MODE mode);
AUDIO_CH_CHANGE_MODE nfui_get_audio_ch_mode();
#endif

int nfui_set_semimodal_wnd(NFWINDOW *window);
int nfui_enable_semi_modal_mode(NFWINDOW *window);
int nfui_disable_semi_modal_mode();
int nfui_hook_evt_in_semi_modal(GdkEventType type);

void nfui_key_signal_emit(NFOBJECT *obj, nfevent_type type, KEYPAD_KID kid, gboolean loop);

void nfui_idle_timer_reset();

int nfui_ui_lock();
int nfui_ui_unlock();

#endif	//	__NF_UI_EVENT_HANDLER_H__
// HID
void hid_remote_control_send_signal(nfevent_type type, guint code);
void nfui_hid_change_cursor_send_signal(nfevent_type type, guint code);
void _emit_signal_hid_keypad_notify(nfevent_type type, guint code);
void nfui_signal_hid_mouse_cursor_change(nfevent_type type, guint code);
