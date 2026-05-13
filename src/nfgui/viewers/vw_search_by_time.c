#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"

#include "services/scm.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nftimespin.h"
#include "objects/nfcheckbutton.h"
#include "objects/nflistbox.h"
#include "objects/nfspinbutton.h"
#include "objects/nfbutton.h"
#include "objects/nftimelabel.h"
#include "objects/nfthumbnail.h"
#include "viewers/objects/ixtimeline.h"

#include "../viewers/objects/cw_calendar.h"

#include "vw_search_by_time.h"

#include "vw_search_main.h"
#include "vw_live_statusbar.h"
#include "vw_set_date_time.h"
#include "vw_internal.h"
#include "stm.h"
#include "dtf.h"
#include "uxm.h"

#define PAGE_FIXED_CNT          4
#define ROW_CNT_PER_PAGE        (GUI_CHANNEL_CNT / PAGE_FIXED_CNT)

#define SBT_LABEL_HEIGHT1               (40)
#define SBT_LABEL_HEIGHT2               (26)//(22)


// FIXED1, FIXED2, FIXED3
#define SBT_FIXED1_X                    (0)
#define SBT_FIXED1_Y                    (0)
#define SBT_FIXED1_W                    (440)
#define SBT_FIXED1_H                    (903)

#define SBT_FIXED2_X                    (SBT_FIXED1_X + SBT_FIXED1_W)
#define SBT_FIXED2_Y                    (SBT_FIXED1_Y)
#define SBT_FIXED2_W                    (1416)
#define SBT_FIXED2_H                    (419 + 266)

#define SBT_FIXED3_X                    (SBT_FIXED2_X)
#define SBT_FIXED3_Y                    (SBT_FIXED2_Y+SBT_FIXED2_H)
#define SBT_FIXED3_W                    (SBT_FIXED2_W)
#define SBT_FIXED3_H                    (484 - 266)


// CALENDAR FIXED
#define SBT_CAL_FIXED_X                 (12)
#define SBT_CAL_FIXED_Y                 (34)
#define SBT_CAL_FIXED_W                 (410)
#define SBT_CAL_FIXED_H                 (380)

#define SBT_CAL_BUTTON_SIZE_W           (130)
#define SBT_CAL_BUTTON_GAP              (46)

#define SBT_CAL_FIRST_BUTTON_X          (51)
#define SBT_CAL_FIRST_BUTTON_Y          (333)

#define SBT_CAL_LAST_BUTTON_X           (SBT_CAL_FIRST_BUTTON_X+SBT_CAL_BUTTON_SIZE_W+SBT_CAL_BUTTON_GAP)
#define SBT_CAL_LAST_BUTTON_Y           (SBT_CAL_FIRST_BUTTON_Y)

#define SBT_CAL_TIME_LABEL_X            (SBT_CAL_FIXED_X)
#define SBT_CAL_TIME_LABEL_Y            (SBT_CAL_FIXED_Y+SBT_CAL_FIXED_H+8)
#define SBT_CAL_TIME_LABEL_W            (410)

// PREVIEW
#define SBT_PREVIEW_LABEL_X             (SBT_CAL_FIXED_X)
#define SBT_PREVIEW_LABEL_Y             (SBT_CAL_TIME_LABEL_Y + SBT_LABEL_HEIGHT1 + 108)
#define SBT_PREVIEW_LABEL_WIDTH         (220)

#define SBT_PREVIEW_VIDEO_X             (SBT_PREVIEW_LABEL_X+4)
#define SBT_PREVIEW_VIDEO_Y             (SBT_PREVIEW_LABEL_Y+SBT_LABEL_HEIGHT2+3) // +2
#define SBT_PREVIEW_VIDEO_W             (408) //410
#define SBT_PREVIEW_VIDEO_H             (228) //230

#define SBT_PREVIEW_STATUS_W            (200)
#define SBT_PREVIEW_STATUS_H            (30)
#define SBT_PREVIEW_STATUS_X            (SBT_PREVIEW_VIDEO_X + (SBT_PREVIEW_VIDEO_W-SBT_PREVIEW_STATUS_W)/2)
#define SBT_PREVIEW_STATUS_Y            (SBT_PREVIEW_VIDEO_Y + (SBT_PREVIEW_VIDEO_H-SBT_PREVIEW_STATUS_H)/2)

#define SBT_PREVIEW_TIME_X              (SBT_PREVIEW_VIDEO_X)
#define SBT_PREVIEW_TIME_Y              (SBT_PREVIEW_VIDEO_Y+SBT_PREVIEW_VIDEO_H+4)

//TIMELINE CH BUTTON
#define SBT_TML_CH_CHECK_X              (15)
#define SBT_TML_CH_CHECK_Y              (34)

//TIMELINE CH BUTTON
#define SBT_TML_CH_LABEL_X              (SBT_TML_CH_CHECK_X + 18 + 1)
#define SBT_TML_CH_LABEL_Y              (SBT_TML_CH_CHECK_Y)
#define SBT_TML_CH_LABEL_W              (31)
#define SBT_TML_CH_LABEL_H              (18)

//TIME LINE BUTTON
#define TML_X                           (SBT_TML_CH_LABEL_X + 31 + 1)
#define TML_Y                           (SBT_TML_CH_LABEL_Y)
#define HEIGHT_RULER                    (25)
#define HEIGHT_CH                       (18)

#define SBT_TL_ZOOM_BTN_GAP             (1)
#define SBT_TL_BTN_GAP                  (10)
#define SBT_TL_DIR_BTN_GAP              (1)

#define SBT_TL_ZOOMOUT_BTN_X            (SBT_TML_CH_CHECK_X + 1096)

//TIME LINE RECORD REASON
#define SBT_TL_REC_BOX_GAP_X            (6)

#define SBT_TL_REC_BOX_Y                (7)
#define SBT_TL_REC_BOX_W                (22)
#define SBT_TL_REC_BOX_H                (22)

#define SBT_TL_REC_TEXT_PANIC_W         (174)//(84)
#define SBT_TL_REC_TEXT_PANIC_X         (SBT_FIXED2_W - SBT_TL_REC_TEXT_PANIC_W)

#define SBT_TL_REC_BOX_PANIC_X          (SBT_TL_REC_TEXT_PANIC_X - SBT_TL_REC_BOX_GAP_X - SBT_TL_REC_BOX_W)
#define SBT_TL_REC_BOX_PANIC_Y          (SBT_TL_REC_BOX_Y)

#define SBT_TL_REC_TEXT_MOT_W           (174)//(105)
#define SBT_TL_REC_TEXT_MOT_X           (SBT_TL_REC_BOX_PANIC_X - SBT_TL_REC_TEXT_MOT_W)

#define SBT_TL_REC_BOX_MOT_X            (SBT_TL_REC_TEXT_MOT_X - SBT_TL_REC_BOX_GAP_X - SBT_TL_REC_BOX_W)
#define SBT_TL_REC_BOX_MOT_Y            (SBT_TL_REC_BOX_Y)

#define SBT_TL_REC_TEXT_ALARM_W         (174)//(94)
#define SBT_TL_REC_TEXT_ALARM_X         (SBT_TL_REC_BOX_MOT_X - SBT_TL_REC_TEXT_ALARM_W)

#define SBT_TL_REC_BOX_ALARM_X          (SBT_TL_REC_TEXT_ALARM_X - SBT_TL_REC_BOX_GAP_X - SBT_TL_REC_BOX_W)
#define SBT_TL_REC_BOX_ALARM_Y          (SBT_TL_REC_BOX_Y)

#define SBT_TL_REC_TEXT_TIME_W          (174)//(153)
#define SBT_TL_REC_TEXT_TIME_X          (SBT_TL_REC_BOX_ALARM_X - SBT_TL_REC_TEXT_TIME_W)

#define SBT_TL_REC_BOX_TIME_X           (SBT_TL_REC_TEXT_TIME_X - SBT_TL_REC_BOX_GAP_X - SBT_TL_REC_BOX_W)
#define SBT_TL_REC_BOX_TIME_Y           (SBT_TL_REC_BOX_Y)

#define SBT_TL_REC_TEXT_PRE_W           (174)
#define SBT_TL_REC_TEXT_PRE_X           (SBT_TL_REC_BOX_TIME_X - SBT_TL_REC_TEXT_PRE_W)

#define SBT_TL_REC_BOX_PRE_X            (SBT_TL_REC_TEXT_PRE_X - SBT_TL_REC_BOX_GAP_X - SBT_TL_REC_BOX_W)
#define SBT_TL_REC_BOX_PRE_Y            (SBT_TL_REC_BOX_Y)


// THUMBNAIL PICTURE
#if defined(GUI_32CH_SUPPORT)
#define THUMB_PIXEL_ALIGNMENT           (16)
#define PREVIEW_OUTLINE_BOX_W           ((THUMB_PIXEL_ALIGNMENT * 10) + 2)
#define PREVIEW_OUTLINE_BOX_H           ((THUMB_PIXEL_ALIGNMENT * 7) + 2)
#define THUMB_COL_CNT                   (8)
#elif defined(GUI_16CH_SUPPORT)
#define PREVIEW_OUTLINE_BOX_W           (172)
#define PREVIEW_OUTLINE_BOX_H           (114)
#define THUMB_COL_CNT                   (8)

#else
#define PREVIEW_OUTLINE_BOX_W           (342)
#define PREVIEW_OUTLINE_BOX_H           (208) //210
#define THUMB_COL_CNT                   (4)
#endif

#define PREVIEW_OUTLINE_BOX_GAP_X       (15)
#define PREVIEW_OUTLINE_BOX_GAP_Y       (8) // 6

// PREVIEW TIME LABEL
#define SBT_PREVIEW_TIME_LABEL_X        (15)
#define SBT_PREVIEW_TIME_LABEL_Y        (2)
#define SBT_PREVIEW_TIME_LABEL_H        (SBT_LABEL_HEIGHT2)


// PREVIEW PICTURE LABEL
#define SBT_PREVIEW_PIC_LABEL_X         (SBT_PREVIEW_TIME_LABEL_X)
#define SBT_PREVIEW_PIC_LABEL_Y         (SBT_PREVIEW_TIME_LABEL_Y + SBT_PREVIEW_TIME_LABEL_H + 2)
#define SBT_PREVIEW_PIC_LABEL_W         (PREVIEW_OUTLINE_BOX_W)
#define SBT_PREVIEW_PIC_LABEL_H         (18)


//#define _SUPPORT_ARBITRARILY_CH_SEL

////////////////////////////////////////////////////////////
//
// private data types
//

enum {
    DF_YMD = 0,
    DF_MDY,
    DF_DMY,

    NUM_DATE_FORMATS,
};


////////////////////////////////////////////////////////////
//
// private variables
//

static NFWINDOW *g_curwnd = 0;
static gboolean g_init_thumb = TRUE;
static gchar g_recInfo[31];
static IXTIMELINE *tml;
static NFOBJECT *fixed1;
static NFOBJECT *fixed2;
static time_t search_time;
static NFOBJECT *preview_obj = NULL;
static NFOBJECT *play_st_obj = NULL;
static NFOBJECT *play_time_obj = NULL;
static NFOBJECT *ch_sel[GUI_CHANNEL_CNT];
static NFOBJECT *ch_all;

static NFOBJECT *preview_fixed;
static NFOBJECT *thumb_obj[GUI_CHANNEL_CNT];
static guchar   ch_image[GUI_CHANNEL_CNT][((PREVIEW_OUTLINE_BOX_H-SBT_PREVIEW_PIC_LABEL_H-2)*(PREVIEW_OUTLINE_BOX_W-2)*3)];
static guint preview_timer_id;

static NFOBJECT *dt_obj;
static NFOBJECT *dt_btn_obj;
static NFOBJECT *g_cal_dir[2];
static NFOBJECT *g_cal;
static NFOBJECT *g_cal_text;
static NFOBJECT *playbtn_obj;
static NFOBJECT *refreshbtn_obj;
static NFOBJECT *g_first_btn;
static NFOBJECT *g_last_btn;
static NFOBJECT *g_page_fixed[PAGE_FIXED_CNT];
static NFOBJECT *g_lb_page_num;

////////////////////////////////////////////////////////////
//
// private functions
//

static int _get_calendar_day(int *year, int *mon, int *day)
{
    if (year) *year = cw_cld_get_current_year((CWCALENDAR*)g_cal);
    if (mon) *mon = cw_cld_get_current_month((CWCALENDAR*)g_cal);
    if (day) *day  = cw_cld_get_current_day((CWCALENDAR*)g_cal);
    return 0;
}

static int _update_calendar_text(time_t timet)
{
    char buf[64];
    int year, mon;
    int length;

    ifn_get_local_day(timet, &year, &mon, 0);

    length = strlen(g_month_str[mon - 1]);
    g_utf8_strncpy(buf, g_month_str[mon - 1], g_utf8_strlen(g_month_str[mon - 1], -1));
    g_sprintf(&buf[length], " %d", year);

    nfui_nflabel_set_text((NFLABEL*)g_cal_text, buf);
    nfui_signal_emit(g_cal_text, GDK_EXPOSE, FALSE);
    return 0;
}

static void _set_thumbnail_time()
{
    guint ch;
    gint spp;

    spp = tml_get_spp(tml);

    for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        nfui_nfthumbnail_set_period(thumb_obj[ch], search_time+1, search_time+1+spp);
}

static gint _set_select_bg_color(NFOBJECT *obj)
{
    guint ch;

    if (nfui_nfthumbnail_get_state((NFTHUMBNAIL*)obj) == THUMB_STATE_SELECT)
        return -1;

    for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
    {
        if (nfui_nfthumbnail_get_state((NFTHUMBNAIL*)thumb_obj[ch]) == THUMB_STATE_SELECT)
            nfui_nfthumbnail_set_state((NFTHUMBNAIL*)thumb_obj[ch], THUMB_STATE_NORMAL);
    }

    nfui_nfthumbnail_set_state((NFTHUMBNAIL*)obj, THUMB_STATE_SELECT);

    return 0;
}

static gint _unset_select_bg_color()
{
    guint ch;

    for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
    {
        if (nfui_nfthumbnail_get_state((NFTHUMBNAIL*)thumb_obj[ch]) == THUMB_STATE_SELECT)
            nfui_nfthumbnail_set_state((NFTHUMBNAIL*)thumb_obj[ch], THUMB_STATE_NORMAL);
    }

    return 0;
}

static void _preview_obj_on(gchar *buf)
{
    nfui_nflabel_set_text((NFLABEL*)preview_obj, buf);
    nfui_nfobject_modify_bg(preview_obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
    nfui_signal_emit(preview_obj, GDK_EXPOSE, FALSE);
}

static void _preview_obj_off(gchar *buf)
{
    nfui_nflabel_set_text((NFLABEL*)preview_obj, buf);
    nfui_nfobject_modify_bg(preview_obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(666));
    nfui_signal_emit(preview_obj, GDK_EXPOSE, FALSE);
}

static int _update_datetime_label()
{
    char cur_buffer[100];

    dtf_get_local_datetime(search_time, cur_buffer);
    nfui_nflabel_set_text((NFLABEL*)dt_obj, cur_buffer);
    nfui_signal_emit(dt_obj, GDK_EXPOSE, TRUE);
    return 0;
}

static int _ready_thumbnail()
{
    NFOBJECT *rate_obj;
    gint i;
    gchar strBuf[10];

    rate_obj = (NFOBJECT*)nfui_nfobject_get_data(preview_fixed, "CMPL RATE");
    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, "(0 / %d)", GUI_CHANNEL_CNT);
    nfui_nflabel_set_text((NFLABEL*)rate_obj, strBuf);
    nfui_signal_emit(rate_obj, GDK_EXPOSE, TRUE);
    _unset_select_bg_color();
    _preview_obj_off("");

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        if (!nfui_nfobject_is_disabled(thumb_obj[i]))
        {
            nfui_nfobject_disable(thumb_obj[i]);
            nfui_signal_emit(thumb_obj[i], GDK_EXPOSE, TRUE);
        }
    }

    vsm_playback_preview_stop();
    return 0;
}

static int _get_thumbnail()
{
    _set_thumbnail_time();
    nfui_nfthumbnail_get_image();
    return 0;
}

static void _playback_button_enable()
{
	nfui_nfobject_enable(playbtn_obj);
	nfui_signal_emit(playbtn_obj, GDK_EXPOSE, FALSE);
}

static void _playback_button_disable()
{
	nfui_nfobject_disable(playbtn_obj);
	nfui_signal_emit(playbtn_obj, GDK_EXPOSE, FALSE);
}

static gboolean _show_thumbnail(void *data)
{
    if (!nfui_nfwindow_find("SEARCH"))  return FALSE;

    vsm_live_stop();
    nfui_nfthumbnail_image_open();
    _ready_thumbnail();
    _get_thumbnail();
	_playback_button_disable();
    g_init_thumb = FALSE;

    return FALSE;
}

static NFOBJECT* _get_selected_thumbnail_obj()
{
    guint ch;

    for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
    {
        if (nfui_nfthumbnail_get_state((NFTHUMBNAIL*)thumb_obj[ch]) == THUMB_STATE_SELECT)
            return thumb_obj[ch];
    }

    return NULL;
}

static gint _get_thumbnail_channel(NFOBJECT *obj)
{
    gint i;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        if (thumb_obj[i] == obj)
            break;
    }

    if (i == GUI_CHANNEL_CNT) g_assert(0);

    return i;
}

static int _update_thumbnail_rate(NFOBJECT *obj, gint cmpl_cnt)
{
    gchar strBuf[10];

    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, "(%d / %d)", cmpl_cnt, GUI_CHANNEL_CNT);
    nfui_nflabel_set_text((NFLABEL*)obj, strBuf);
    nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
}

static int _update_thumbnail_info(NFOBJECT *obj)
{
    nfui_nfobject_enable(obj);
    nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
}

static int _update_calendar_time(time_t timet)
{
    int year, mon, day;
    if (timet == 0) timet = time(0);
    dtf_get_local_day(timet, &year, &mon, &day);
    cw_cld_change_date((CWCALENDAR*)g_cal, year, mon, day);
    return 0;
}

static nftl_df_type prvTransDateFormat(gint db_index)
{
    nftl_df_type ret;

    if(db_index == DF_YMD)          ret = NFTL_DF_YMD;
    else if(db_index == DF_MDY)     ret = NFTL_DF_MDY;
    else if(db_index == DF_DMY)     ret = NFTL_DF_DMY;
    else    ret = NFTL_DF_HIDE;

    return ret;
}

static time_t _get_new_calendar_time()
{
    int year, month, day, hour, min, sec;
    time_t tim;

    _get_calendar_day(&year, &month, &day);
    dtf_get_local_hourmin(search_time, &hour, &min, &sec);
    tim = ifn_get_gmt_from_local(year, month, day, hour, min, sec);
    return tim;
}

static void _change_obj_focus(NFOBJECT* from, NFOBJECT *to)
{
    nfui_set_key_focus(from, FALSE);
    nfui_set_key_focus(to, TRUE);

    nfui_signal_emit(from, GDK_EXPOSE, TRUE);
    nfui_signal_emit(to, GDK_EXPOSE, TRUE);
}

static gboolean post_prev_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
    gchar buf[64];
    
    if(evt->type == GDK_BUTTON_RELEASE) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
          return FALSE;
        }

        for (i = 0; i < PAGE_FIXED_CNT; i++) {
            if (nfui_nfobject_is_shown((NFOBJECT*)g_page_fixed[i])) {
                break;
            }
        }

        if (i == PAGE_FIXED_CNT) return FALSE;

        if (i == 0) return FALSE;

        nfui_nfobject_hide(g_page_fixed[i]);
        
        _ready_thumbnail();
        _get_thumbnail();
    	_playback_button_disable();

        i--;
        
        memset(buf, 0x00, sizeof(buf));
        g_sprintf(buf, "%d / %d", i + 1, PAGE_FIXED_CNT);
        nfui_nflabel_set_text(g_lb_page_num, buf);
        nfui_signal_emit(g_lb_page_num, GDK_EXPOSE, TRUE);

        nfui_nfobject_show(g_page_fixed[i]);
        nfui_signal_emit(g_page_fixed[i], GDK_EXPOSE, TRUE);
    	
        nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }

    return FALSE;
}

static gboolean post_next_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
    gchar buf[64];

    if(evt->type == GDK_BUTTON_RELEASE) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
          return FALSE;
        }

        for (i = 0; i < PAGE_FIXED_CNT; i++) {
            if (nfui_nfobject_is_shown((NFOBJECT*)g_page_fixed[i])) {
                break;
            }
        }

        if (i == PAGE_FIXED_CNT) return FALSE;

        if (i == (PAGE_FIXED_CNT - 1)) return FALSE;

        nfui_nfobject_hide(g_page_fixed[i]);
        
        _ready_thumbnail();
        _get_thumbnail();
    	_playback_button_disable();

        i++;
                
        memset(buf, 0x00, sizeof(buf));
        g_sprintf(buf, "%d / %d", i + 1, PAGE_FIXED_CNT);
        nfui_nflabel_set_text(g_lb_page_num, buf);
        nfui_signal_emit(g_lb_page_num, GDK_EXPOSE, TRUE);

        nfui_nfobject_show(g_page_fixed[i]);
        nfui_signal_emit(g_page_fixed[i], GDK_EXPOSE, TRUE);

        nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }

    return FALSE;
}

static gboolean post_prev_cal_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    time_t ret;

    if(evt->type == GDK_BUTTON_RELEASE) {
        ret = cw_cld_set_prev_month((CWCALENDAR*)g_cal);
        if (ret == 0) return FALSE;
        _update_calendar_text(ret);
    }
    else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        GdkEventKey *kevt;
        KEYPAD_KID kpid;

        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;

        if(kpid == KEYPAD_UP)
        {
            _change_obj_focus(obj, dt_btn_obj);
            return TRUE;
        }
        else if(kpid == KEYPAD_DOWN)
        {
            _change_obj_focus(obj, g_cal);
            return TRUE;
        }
    }

    return FALSE;
}

static gboolean post_next_cal_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    time_t ret;

    if(evt->type == GDK_BUTTON_RELEASE) {
        ret = cw_cld_set_next_month((CWCALENDAR*)g_cal);
        if (ret == 0) return FALSE;
        _update_calendar_text(ret);
    }
    else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        GdkEventKey *kevt;
        KEYPAD_KID kpid;

        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;

        if(kpid == KEYPAD_UP)
        {
            _change_obj_focus(obj, dt_btn_obj);
            return TRUE;
        }
        else if(kpid == KEYPAD_DOWN)
        {
            _change_obj_focus(obj, g_cal);
            return TRUE;
        }
    }
    return FALSE;
}

static gboolean post_first_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    time_t first_time;

    if(evt->type == GDK_BUTTON_RELEASE) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        first_time = VW_Search_Get_Record_FirstTime();
        if (first_time != 0) {

            _update_calendar_time(first_time);
            _update_calendar_text(first_time);

            search_time = first_time;
            stm_set_time_t(search_time);
            _update_datetime_label();

            tml_set_manual_paint_mode(tml, 1);
            tml_zoom_min(tml);
            tml_reset_cur_day(tml, search_time);
            tml_set_playstick_time_t(tml, search_time);
            tml_refresh(tml);
            tml_set_manual_paint_mode(tml, 0);

            _ready_thumbnail();
            _get_thumbnail();
			_playback_button_disable();
        }
    }

    return FALSE;
}

static gboolean post_last_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    time_t last_time;

    if(evt->type == GDK_BUTTON_RELEASE) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        last_time = VW_Search_Get_Record_LastTime();
        if (last_time != 0) {

            _update_calendar_time(last_time);
            _update_calendar_text(last_time);

            search_time = last_time;
            stm_set_time_t(search_time);
            _update_datetime_label();

            tml_set_manual_paint_mode(tml, 1);
            tml_zoom_min(tml);
            tml_reset_cur_day(tml, search_time);
            tml_set_playstick_time_t(tml, search_time);
            tml_refresh(tml);
            tml_set_manual_paint_mode(tml, 0);

            _ready_thumbnail();
            _get_thumbnail();
			_playback_button_disable();
        }
    }
    return FALSE;
}

static gboolean post_calendar_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CALENDAR_CHANGED_RELEASE) {
        search_time = _get_new_calendar_time();
        stm_set_time_t(search_time);
        _update_datetime_label();
        _update_calendar_text(search_time);

        tml_set_manual_paint_mode(tml, 1);
        tml_zoom_min(tml);
        tml_reset_cur_day(tml, search_time);
        tml_set_playstick_time_t(tml, search_time);
        tml_refresh(tml);
        tml_set_manual_paint_mode(tml, 0);

        _ready_thumbnail();
        _get_thumbnail();
		_playback_button_disable();
    }
    else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        GdkEventKey *kevt;
        KEYPAD_KID kpid;

        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;

        if (((CWCALENDAR*)obj)->object.grab_kfocus) return FALSE;

        if(kpid == KEYPAD_UP)
        {
            _change_obj_focus(obj, g_cal_dir[1]);
            return TRUE;
        }
        else if (kpid == KEYPAD_DOWN)
        {
            _change_obj_focus(obj, g_last_btn);
            return TRUE;
        }
    }
    return FALSE;
}

static gboolean _set_preview_time(gpointer data)
{
    GTimeVal tmp;
    gchar strBuf[64];
    gchar *getBuf;

    memset(&tmp, 0x00, sizeof(GTimeVal));
    memset(strBuf, 0x00, sizeof(strBuf));
    tmp = vsm_playback_get_previewtime();

    if (tmp.tv_sec != 0)
        dtf_get_local_datetime(tmp.tv_sec, strBuf);
    else
        g_sprintf(strBuf, "");

    getBuf = nfui_nflabel_get_text((NFLABEL*)play_time_obj);

    if (strcmp(getBuf, strBuf) != 0)
    {
        nfui_nflabel_set_text((NFLABEL*)play_time_obj, strBuf);
        nfui_signal_emit(play_time_obj, GDK_EXPOSE, TRUE);
    }

    return TRUE;
}


static gboolean post_datetime_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    guint i;
    time_t tt;
    int x, y;
    NFOBJECT *top;

    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        nfui_nfobject_get_offset(obj, &x, &y);
        top = nfui_nfobject_get_top(obj);

        x += top->x + obj->width + 4;
        y += top->y;

        tt = VW_Set_DateTime_Open(g_curwnd, "DATE/TIME", x, y, search_time, SDT_TYPE_SEC, NF_LOWER_TIMELIMIT, NF_UPPER_TIMELIMIT);

        if (tt == 0) return FALSE;

        search_time = tt;
        stm_set_time_t(search_time);
        _update_calendar_time(search_time);
        _update_calendar_text(search_time);
        _update_datetime_label();
        tml_reset_cur_day_hour(tml, search_time);

        tml_set_playstick_time_t(tml, search_time);
        tml_repaint(tml);
        _ready_thumbnail();
        _get_thumbnail();
		_playback_button_disable();

    }
    else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        GdkEventKey *kevt;
        KEYPAD_KID kpid;

        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;

        if (((CWCALENDAR*)obj)->object.grab_kfocus) return FALSE;

        if(kpid == KEYPAD_UP)
        {
            _change_obj_focus(obj, g_last_btn);
            return TRUE;
        }
        else if (kpid == KEYPAD_DOWN)
        {
            _change_obj_focus(obj, g_cal_dir[1]);
            return TRUE;
        }
    }

    return FALSE;
}

static gboolean pre_cal_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    switch(evt->type)
    {
        case GDK_EXPOSE:
        {
            GdkGC *gc;
            GdkDrawable *drawable;
            gint pos_x, pos_y;

            drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
            gc = gdk_gc_new(drawable);

            nfui_nfobject_get_offset(obj, &pos_x, &pos_y);

            nfutil_draw_image(drawable, gc, IMG_CALENDAR_BG, pos_x, pos_y, -1, -1, NFALIGN_LEFT, 0);

            g_object_unref(gc);

            _update_calendar_time(search_time);
            _update_calendar_text(search_time);
            _update_datetime_label();
            //SKSHIN, 0722
//          stm_set_time_t(search_time);


        }
        break;

        case GDK_DELETE:

        break;

        default :

        break;
    }

    return FALSE;
}

static gboolean pre_fixed1_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    switch(evt->type)
    {
        case GDK_EXPOSE:
        {
            GdkGC *gc;
            GdkDrawable *drawable;
            gint gap_x, gap_y;

            nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);

            drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
            gc = gdk_gc_new(drawable);

            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR((604)));
            gdk_draw_rectangle(drawable, gc, TRUE, gap_x+obj->width-1, gap_y, 1, obj->height);

            if (preview_timer_id == 0)
                preview_timer_id = g_timeout_add(500, _set_preview_time, NULL);

            // skshin
            g_object_unref(gc);
        }
        break;

        case GDK_DELETE:
        {
            if(preview_timer_id != 0) {
                g_source_remove(preview_timer_id);
                preview_timer_id = 0;
            }
        }
        break;

        default :

        break;
    }

    return FALSE;
}

static unsigned int _get_ch_selecting_mask()
{
    int i;
    unsigned int mask = 0;

    for (i = 0; i < var_get_ch_count(); ++i) {
//      mask |= (((int)nfui_check_button_get_active(ch_sel[i])) << i);
        mask |= (1 << i);
    }

    return mask;
}

static gboolean post_fixed2_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    switch(evt->type) {
    case GDK_EXPOSE:
        tml_repaint(tml);
        break;
    }
    return FALSE;
}

static gboolean pre_fixed2_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    CMM_MESSAGE_T *pmsg;

    switch(evt->type)
    {
        case GDK_EXPOSE:
        {
            GdkGC *gc;
            GdkDrawable *drawable;
            GTimeVal schtime;
            gint gap_x, gap_y;

            nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);

            memset(&schtime, 0x00, sizeof(GTimeVal));
            stm_get_time(&schtime);

            drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
            gc = gdk_gc_new(drawable);

            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(604));
            gdk_draw_rectangle(drawable, gc, TRUE, gap_x, gap_y+obj->height-1, obj->width, 1);

            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(647));
            gdk_draw_rectangle(drawable,
                    gc,
                    TRUE,
                    gap_x+SBT_TL_REC_BOX_PRE_X, gap_y+SBT_TL_REC_BOX_PRE_Y,
                    SBT_TL_REC_BOX_W, SBT_TL_REC_BOX_H);

            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(639));
            gdk_draw_rectangle(drawable,
                    gc,
                    TRUE,
                    gap_x+SBT_TL_REC_BOX_TIME_X, gap_y+SBT_TL_REC_BOX_TIME_Y,
                    SBT_TL_REC_BOX_W, SBT_TL_REC_BOX_H);

            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(641));
            gdk_draw_rectangle(drawable,
                    gc,
                    TRUE,
                    gap_x+SBT_TL_REC_BOX_ALARM_X, gap_y+SBT_TL_REC_BOX_ALARM_Y,
                    SBT_TL_REC_BOX_W, SBT_TL_REC_BOX_H);

            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(643));
            gdk_draw_rectangle(drawable,
                    gc,
                    TRUE,
                    gap_x+SBT_TL_REC_BOX_MOT_X, gap_y+SBT_TL_REC_BOX_MOT_Y,
                    SBT_TL_REC_BOX_W, SBT_TL_REC_BOX_H);

            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(645));
            gdk_draw_rectangle(drawable,
                    gc,
                    TRUE,
                    gap_x+SBT_TL_REC_BOX_PANIC_X, gap_y+SBT_TL_REC_BOX_PANIC_Y,
                    SBT_TL_REC_BOX_W, SBT_TL_REC_BOX_H);

#if 0
            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(630));
            gdk_draw_rectangle(drawable,
                    gc,
                    TRUE,
                    SBT_LEFT_GAP_X + SBT_FIXED2_X + SBT_TML_CH_BTN_X,
                    SBT_UP_GAP_Y + SBT_FIXED2_Y + SBT_TML_CH_BTN_Y - 5,
                    31, 6);
#endif

            g_object_unref(gc);
        }
        break;


        case GDK_DELETE:

        break;

        default :

        break;
    }

    return FALSE;
}

static gboolean _thumbnail_pic_event_preview_handling(gint ch)
{
    GTimeVal stime = {0, 0};
    time_t tmp_time;
    gint result;
    guint ch_mask = 0;
    gint gap_x, gap_y;
    gint shown_as;

    vsm_playback_preview_stop();

    if (ssm_get_covert_mask() & (1 << ch))
    {
        shown_as = ssm_get_covert_shown_as();

        if (!shown_as)
        {
            _preview_obj_off("NO VIDEO");
        }
        else
        {
            _preview_obj_off("COVERT");
        }

        return FALSE;
    }

    result = nfui_nfthumbnail_get_focused_image_time((NFTHUMBNAIL*)thumb_obj[ch], &tmp_time);

    if (result == 0)
    {
        _preview_obj_off("N/A");
        return FALSE;
    }
    else if (result == 2)
    {
        _preview_obj_off("Please wait...");
        return FALSE;
    }

    stime.tv_sec = tmp_time;

    nfui_nfobject_get_offset((NFOBJECT*)fixed1, &gap_x, &gap_y);
    ch_mask |= (1 << ch);

    vsm_playback_preview_start(ch_mask, stime,
            gap_x+SBT_PREVIEW_VIDEO_X, gap_y+SBT_PREVIEW_VIDEO_Y, SBT_PREVIEW_VIDEO_W, SBT_PREVIEW_VIDEO_H);

    _preview_obj_on("");

    return TRUE;
}

static gboolean _thumbnail_pic_event_playback_handling(gint ch)
{
    GTimeVal stime = {0, 0};
    time_t tmp_time;
    gint result;
    guint ch_mask = 0;
    gint shown_as = 0;

    vsm_playback_preview_stop();

    if (ssm_get_covert_mask() & (1 << ch))
    {
        shown_as = ssm_get_covert_shown_as();

        if (!shown_as)
        {
            _preview_obj_off("NO VIDEO");
        }
        else
        {
            _preview_obj_off("COVERT");
        }

        return FALSE;
    }

    result = nfui_nfthumbnail_get_focused_image_time((NFTHUMBNAIL*)thumb_obj[ch], &tmp_time);

    if (result == 0)
    {
        _preview_obj_off("N/A");
        return FALSE;
    }
    else if (result == 2)
    {
        _preview_obj_off("Please wait...");
        return FALSE;
    }

    _preview_obj_off("");
    stime.tv_sec = tmp_time;

    ch_mask |= (1 << ch);
    VW_Search_start_playback();
    vsm_playback_start(ch_mask, stime, PLAYBACK_NORMAL);

    return TRUE;
}

static gboolean pre_fixed3_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint ch;

    switch(evt->type)
    {
        case GDK_EXPOSE:
        {
            if (g_init_thumb) g_timeout_add(300, _show_thumbnail, 0);
        }
        break;

        case GDK_DELETE:
            g_init_thumb = TRUE;
            uxm_unreg_imsg_event(obj, INFY_THUMBNAIL_CMPL_OBJ);
            uxm_unreg_imsg_event(obj, INFY_THUMBNAIL_TOTALLY_CMPL);
        break;

        case INFY_THUMBNAIL_CMPL_OBJ:
        {
            gint i;
            NFOBJECT *rate_obj;
            NFOBJECT *cmpl_obj = ((CMM_MESSAGE_T *)data)->data;
            gchar strBuf[10];
            gint s_ch, e_ch, page_idx;

            if (!nfui_nfobject_is_shown(obj))
                return FALSE;

            for (i = 0; i < PAGE_FIXED_CNT; i++)  {
                if (nfui_nfobject_is_shown(g_page_fixed[i])) break;
            }

            s_ch = THUMB_COL_CNT * i;
            e_ch = s_ch + THUMB_COL_CNT;
            
            for(i = s_ch; i < e_ch; i++)
            {
                if (cmpl_obj == thumb_obj[i])
                    break;
            }

            if (i == e_ch)
                return FALSE;

            if (nfui_nfobject_is_disabled(thumb_obj[s_ch])
                && (i != s_ch))
                return;

            rate_obj = (NFOBJECT*)nfui_nfobject_get_data(obj, "CMPL RATE");
            _update_thumbnail_rate(rate_obj, i+1);
            _update_thumbnail_info(cmpl_obj);
        }
        break;

        case INFY_THUMBNAIL_TOTALLY_CMPL:
        {
            NFOBJECT *selected_thumb;

            if (!nfui_nfobject_is_shown(obj))
                return FALSE;

            selected_thumb = _get_selected_thumbnail_obj();

            if (selected_thumb)
            {
                ch = _get_thumbnail_channel(selected_thumb);
                _thumbnail_pic_event_preview_handling(ch);
            }

			_playback_button_enable();
        }
        break;

        default :

        break;
    }

    return FALSE;
}

static gboolean post_allch_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
    gint i;
    gboolean act = FALSE;

    if(event->type == NFEVENT_CHECKBUTTON_CHANGED) {
        act = nfui_check_button_get_active(NF_CHECKBUTTON(obj));

        for (i = 0; i < var_get_ch_count(); ++i) {
            if(act) nfui_check_button_set_active(NF_CHECKBUTTON(ch_sel[i]), act);
            else    nfui_check_button_set_active(NF_CHECKBUTTON(ch_sel[i]), act);
        }
    }

    return FALSE;
}

static gboolean post_ch_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
    gboolean act = FALSE;

    if(event->type == NFEVENT_CHECKBUTTON_CHANGED) {
        act = nfui_check_button_get_active(NF_CHECKBUTTON(obj));

        if(!act)
            nfui_check_button_set_active(NF_CHECKBUTTON(ch_all), act);
    }


    return FALSE;
}

static gboolean post_timeline_event_cb(NFOBJECT *obj, GdkEvent *event, gpointer data)
{

    CMM_MESSAGE_T *pmsg = (CMM_MESSAGE_T *)data;
    GTimeVal gtv;
    int year, mon, day;
    char cur_buffer[100];
    time_t tmp;

    memset(&gtv, 0x00, sizeof(GTimeVal));
    switch (event->type) {
    case GDK_BUTTON_RELEASE:
        break;

    case GDK_2BUTTON_PRESS:
        break;

    case NFEVENT_TML_RULER_DRAGUP:
        break;

    case NFEVENT_TML_RULER_DRAGDOWN:
        break;

    case GDK_DELETE:
        uxm_unreg_imsg_event(obj, INFY_TML_DATE_CHANGED);
        uxm_unreg_imsg_event(obj, INFY_TML_PLAY_CHANGED);
        uxm_unreg_imsg_event(obj, INFY_TML_START_CHANGED);
        uxm_unreg_imsg_event(obj, INFY_TML_END_CHANGED);
        uxm_unreg_imsg_event(obj, INFY_TML_SCROLL_UP);
        uxm_unreg_imsg_event(obj, INFY_TML_SCROLL_DOWN);
        uxm_unreg_imsg_event(obj, INFY_TML_DOUBLE_CLICKED);
        break;

        case INFY_TML_DOUBLE_CLICKED:
        {
            unsigned int sel_mask = 0;
            GTimeVal SearchTime;

            pmsg = (CMM_MESSAGE_T *)data;
            if (obj != pmsg->data) return FALSE;
            if (!nfui_nfobject_is_shown(obj)) return FALSE;

            nfui_nfthumbnail_wait_for_stop();

            _preview_obj_off("");
            vsm_playback_preview_stop();

            memset(&SearchTime, 0, sizeof(GTimeVal));
            SearchTime.tv_sec = search_time;

            sel_mask = _get_ch_selecting_mask();
            VW_Search_start_playback();
            vsm_playback_start(sel_mask, SearchTime, PLAYBACK_NORMAL);

        }
        break;

    case INFY_TML_DATE_CHANGED:
        pmsg = (CMM_MESSAGE_T *)data;
        if (obj != pmsg->data) return FALSE;
        tmp = tml_get_cur_time_t(obj);
        _update_calendar_time(tmp);
        _update_calendar_text(tmp);
        break;

    case INFY_TML_PLAY_CHANGED:
        pmsg = (CMM_MESSAGE_T *)data;
        if (obj != pmsg->data) return FALSE;
        if (!nfui_nfobject_is_shown(obj)) return FALSE;
        search_time = pmsg->param;
        stm_set_time_t(search_time);
        dtf_get_local_datetime(search_time, cur_buffer);

        nfui_nflabel_set_text((NFLABEL*)dt_obj, cur_buffer);
        nfui_signal_emit((NFOBJECT*)dt_obj, GDK_EXPOSE, TRUE);

        _ready_thumbnail();
        _get_thumbnail();
		_playback_button_disable();
        break;

    case INFY_TML_END_CHANGED:
        pmsg = (CMM_MESSAGE_T *)data;
        if (obj != pmsg->data) return FALSE;
        break;

    case INFY_TML_SCROLL_UP:
        pmsg = (CMM_MESSAGE_T *)data;
        if (obj != pmsg->data) return FALSE;
        tml_zoom_in(tml);
        break;

    case INFY_TML_SCROLL_DOWN:
        pmsg = (CMM_MESSAGE_T *)data;
        if (obj != pmsg->data) return FALSE;
        tml_zoom_out(tml);
        break;

    case NFEVENT_KEYPAD_PRESS:
    case NFEVENT_REMOCON_PRESS:
        {
            GdkEventKey *kevt;
            KEYPAD_KID kpid;

            kevt = (GdkEventKey*)event;
            kpid = (KEYPAD_KID)kevt->keyval;

            if(kpid == KEYPAD_UP)
            {
                _change_obj_focus(obj, playbtn_obj);
                return TRUE;
            }
            else if (kpid == KEYPAD_DOWN)
            {
                _change_obj_focus(obj, refreshbtn_obj);
                return TRUE;
            }
            else if (kpid == KEYPAD_LEFT)
            {
                _change_obj_focus(obj, g_cal_dir[1]);
                return TRUE;
            }
            else if (kpid == KEYPAD_RIGHT)
            {
                _change_obj_focus(obj, g_cal_dir[0]);
                return TRUE;
            }
            else if (kpid == KEYPAD_EXIT)
            {
                nfui_set_key_focus(obj, FALSE);
                nfui_set_key_focus(g_cal, TRUE);
                nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
                nfui_signal_emit(g_cal, GDK_EXPOSE, TRUE);
                return TRUE;
            }
        }
        break;
    }


    return TRUE;
}

static gboolean post_refresh_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
        tml_refresh(tml);
    }
    else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        GdkEventKey *kevt;
        KEYPAD_KID kpid;

        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;

        if(kpid == KEYPAD_UP)
        {
            _change_obj_focus(obj, playbtn_obj);
            return TRUE;
        }
        else if(kpid == KEYPAD_DOWN)
        {
            _change_obj_focus(obj, thumb_obj[0]);
            return TRUE;
        }
    }
    return FALSE;
}

static gboolean post_zoomin_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
        tml_zoom_in(tml);
    }
    else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        GdkEventKey *kevt;
        KEYPAD_KID kpid;

        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;

        if(kpid == KEYPAD_UP)
        {
            _change_obj_focus(obj, playbtn_obj);
            return TRUE;
        }
        else if(kpid == KEYPAD_DOWN)
        {
            _change_obj_focus(obj, thumb_obj[0]);
            return TRUE;
        }
    }

    return FALSE;
}

static gboolean post_zoomout_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
        tml_zoom_out(tml);
    }
    else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        GdkEventKey *kevt;
        KEYPAD_KID kpid;

        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;

        if(kpid == KEYPAD_UP)
        {
            _change_obj_focus(obj, playbtn_obj);
            return TRUE;
        }
        else if(kpid == KEYPAD_DOWN)
        {
            _change_obj_focus(obj, thumb_obj[0]);
            return TRUE;
        }
    }
    return FALSE;
}

static gboolean post_prev_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
        tml_go_back(tml);
    }
    else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        GdkEventKey *kevt;
        KEYPAD_KID kpid;

        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;

        if(kpid == KEYPAD_UP)
        {
            _change_obj_focus(obj, playbtn_obj);
            return TRUE;
        }
        else if(kpid == KEYPAD_DOWN)
        {
            _change_obj_focus(obj, thumb_obj[0]);
            return TRUE;
        }
    }
    return FALSE;
}

static gboolean post_next_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
        tml_go_ahead(tml);
    }
    else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        GdkEventKey *kevt;
        KEYPAD_KID kpid;

        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;

        if(kpid == KEYPAD_UP)
        {
            _change_obj_focus(obj, playbtn_obj);
            return TRUE;
        }
        else if(kpid == KEYPAD_DOWN)
        {
            _change_obj_focus(obj, thumb_obj[0]);
            return TRUE;
        }
    }
    return FALSE;
}

static gboolean post_pic_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkEventKey *kevt;
    KEYPAD_KID kpid = KEYPAD_NONE;

    gint ch;
    GTimeVal s_tv = {0, 0};
    time_t tmp_time;
    gint result;

    if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;
    }

    if(evt->type == GDK_BUTTON_PRESS || evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        ch = _get_thumbnail_channel(obj);
        result = _set_select_bg_color(obj);

        if ((evt->type == GDK_2BUTTON_PRESS) || ((kpid == KEYPAD_ENTER) && (result == -1)))
            _thumbnail_pic_event_playback_handling(ch);
        else
            _thumbnail_pic_event_preview_handling(ch);
    }
    else if(kpid == KEYPAD_UP)
    {
        _change_obj_focus(obj, refreshbtn_obj);
        return TRUE;
    }
    else if(kpid == KEYPAD_DOWN)
    {
        _change_obj_focus(obj, playbtn_obj);
        return TRUE;
    }

    return FALSE;
}

static gboolean post_playbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    unsigned int sel_mask = 0;
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        GTimeVal SearchTime;

        if(evt->button.button == MOUSE_RIGTH_BUTTON) {
            return FALSE;
        }

        _preview_obj_off("");
        vsm_playback_preview_stop();

        memset(&SearchTime, 0, sizeof(GTimeVal));
        SearchTime.tv_sec = search_time;

        sel_mask = _get_ch_selecting_mask();
        VW_Search_start_playback();
        vsm_playback_start(sel_mask, SearchTime, PLAYBACK_NORMAL);
    }
    else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        GdkEventKey *kevt;
        KEYPAD_KID kpid;

        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;

        if(kpid == KEYPAD_UP)
        {
            _change_obj_focus(obj, thumb_obj[0]);
            return TRUE;
        }
        else if(kpid == KEYPAD_DOWN)
        {
            _change_obj_focus(obj, refreshbtn_obj);
            return TRUE;
        }
    }

    return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {
            return FALSE;
        }

        vsm_playback_preview_stop();
        VW_Search_Destroy();
    }
    else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        GdkEventKey *kevt;
        KEYPAD_KID kpid;

        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;

        if(kpid == KEYPAD_UP)
        {
            _change_obj_focus(obj, thumb_obj[0]);
            return TRUE;
        }
        else if(kpid == KEYPAD_DOWN)
        {
            _change_obj_focus(obj, refreshbtn_obj);
            return TRUE;
        }
    }

    return FALSE;
}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_DELETE)
    {
        g_curwnd = 0;
    }

    return FALSE;
}

/*
 :  search by time main_fixed.

      --------------------------------------------
      |  ---------------------------------------  |
      | |           |                           | |
      | |           |                           | |
      | |           |       FIXED2              | |
      | |           |                           | |
      | |           |                           | |
      | |  FIXED1   |---------------------------| |
      | |           |                           | |
      | |           |                           | |
      | |           |       FIXED3              | |
      | |           |                           | |
      | |           |                           | |
      | |           |                           | |
      |  ---------------------------------------  |
      |                                           |
      |             PARENT                        |
      |-------------------------------------------|
*/

void vw_init_SearchByTime_page(NFOBJECT *parent)
{
    NFOBJECT *content_fixed;
    NFOBJECT *fixed3;
    NFOBJECT *main_page_fixed;
    NFOBJECT *page_fixed[PAGE_FIXED_CNT];
    NFOBJECT *page_ntb[PAGE_FIXED_CNT];

    NFOBJECT *cal_fixed;

    NFOBJECT *ntb;
    NFOBJECT *obj;

    TML_FIGURE_T figure;
    int year, month, day, hour, min, sec;
    char cur_buffer[100];
    
	gint page_num, col_num;
	GdkPixbuf *prev_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *next_img[NFOBJECT_STATE_COUNT];

    GdkPixbuf *datetime_img[NFOBJECT_STATE_COUNT];
    GdkPixbuf *tl_btn_img[5][NFOBJECT_STATE_COUNT];
    GdkPixbuf *arrow_img[2][NFOBJECT_STATE_COUNT];

    const gchar *strDay[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
    const gchar *strRec[] = {"PRE", "CONTINUOUS", "ALARM", "MOTION", "PANIC"};
    const gchar *calButton[] = {"FIRST", "LAST"};
    const gchar *strButton[] = {"PLAYBACK", "CLOSE"};

    gchar strBuf[64];

    guint size_w, size_h;
    guint pos_x, pos_y;

    guint ch, i, color_kind = 1;
    guint width[7];

    int zoom_btn_y;
    int hei;
    int length;

    time_t cur_utime;


    g_curwnd = nfui_nfobject_get_top(parent);

    content_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(content_fixed, MENU_H_INNER_W, MENU_H_INNER_H);
    nfui_nfobject_show(content_fixed);
    nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);

    stm_get_time_range_t(0, &search_time);

//  stm_set_time_t(search_time);
    dtf_get_local_day(search_time, &year, &month, &day);
    dtf_get_local_hourmin(search_time, &hour, &min, &sec);

// create FIXED.
    fixed1 = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed1, SBT_FIXED1_W, SBT_FIXED1_H);
    nfui_nfobject_show(fixed1);
    nfui_nfobject_modify_bg(fixed1, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nffixed_put((NFFIXED*)content_fixed, fixed1, SBT_FIXED1_X, SBT_FIXED1_Y);

    fixed2 = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed2, SBT_FIXED2_W, SBT_FIXED2_H);
    nfui_nfobject_show(fixed2);
    nfui_nfobject_modify_bg(fixed2, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nffixed_put((NFFIXED*)content_fixed, fixed2, SBT_FIXED2_X, SBT_FIXED2_Y);

    fixed3 = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed3, SBT_FIXED3_W, SBT_FIXED3_H);
    nfui_nfobject_show(fixed3);
    nfui_nfobject_modify_bg(fixed3, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nffixed_put((NFFIXED*)content_fixed, fixed3, SBT_FIXED3_X, SBT_FIXED3_Y);

    preview_fixed = fixed3;

// CALENDAR
    cal_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(cal_fixed, SBT_CAL_FIXED_W, SBT_CAL_FIXED_H);
    nfui_nfobject_show(cal_fixed);
    nfui_nfobject_modify_bg(cal_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nffixed_put((NFFIXED*)fixed1, cal_fixed, SBT_CAL_FIXED_X, SBT_CAL_FIXED_Y);


    // move button
    arrow_img[0][0] = nfui_get_image_from_file((IMG_CALENDAR_PRE_N_BUTTON), NULL);
    arrow_img[0][1] = nfui_get_image_from_file((IMG_CALENDAR_PRE_O_BUTTON), NULL);
    arrow_img[0][2] = nfui_get_image_from_file((IMG_CALENDAR_PRE_P_BUTTON), NULL);
    arrow_img[0][3] = nfui_get_image_from_file((IMG_CALENDAR_PRE_D_BUTTON), NULL);

    arrow_img[1][0] = nfui_get_image_from_file((IMG_CALENDAR_NEXT_N_BUTTON), NULL);
    arrow_img[1][1] = nfui_get_image_from_file((IMG_CALENDAR_NEXT_O_BUTTON), NULL);
    arrow_img[1][2] = nfui_get_image_from_file((IMG_CALENDAR_NEXT_P_BUTTON), NULL);
    arrow_img[1][3] = nfui_get_image_from_file((IMG_CALENDAR_NEXT_D_BUTTON), NULL);

    for(i=0; i<2; i++) {
        obj = (NFOBJECT*)nfui_nfbutton_new();
        nfui_nfbutton_set_image(NF_BUTTON(obj), arrow_img[i]);
        nfui_nfobject_set_size(obj, 27, 27);
        nfui_nfobject_show(obj);

        if(i == 0) {
            nfui_regi_post_event_callback(obj, post_prev_cal_event_handler);
            nfui_nffixed_put((NFFIXED*)cal_fixed, obj, 34, 7);
            g_cal_dir[0] = obj;
        }else      {
            nfui_regi_post_event_callback(obj, post_next_cal_event_handler);
            nfui_nffixed_put((NFFIXED*)cal_fixed, obj, 349, 7);
            g_cal_dir[1] = obj;
        }
    }

    // calendar month/year
    length = strlen(g_month_str[month - 1]);
    g_utf8_strncpy(strBuf, g_month_str[month - 1], g_utf8_strlen(g_month_str[month - 1], -1));
    g_sprintf(&strBuf[length], " %d", year);

    g_cal_text = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(607));
    nfui_nfobject_set_size(g_cal_text, (guint)(282) , (guint)(34));
    nfui_nflabel_set_align((NFLABEL*)g_cal_text, NFALIGN_CENTER,0);
    nfui_nfobject_modify_bg(g_cal_text, NFOBJECT_STATE_NORMAL, COLOR_IDX(606));
    nfui_nfobject_use_focus(g_cal_text, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(g_cal_text);

    nfui_nffixed_put((NFFIXED*)cal_fixed, g_cal_text, (guint)(63), (guint)(4));

    // CALENDAR LABEL
    gint cal_lbl_pos_x[7] = {47, 93, 137, 182, 226, 269, 315};
    for(i = 0 ; i < 7 ; i++)
    {
        if (i == 0){
            obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strDay[i],
                                        nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(609));
        }else if (i == 6){
            obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strDay[i],
                                        nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(611));
        }else{
            obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strDay[i],
                                        nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(610));
        }

        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(obj, 46, 40);
        nfui_nfobject_show(obj);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(605));
        nfui_nffixed_put((NFFIXED*)cal_fixed, obj, cal_lbl_pos_x[i], 43);

    }

    g_cal = (NFOBJECT*)cw_cld_new(search_time, 44, 40);
    nfui_regi_post_event_callback(g_cal, post_calendar_event_cb);
    nfui_nfobject_show(g_cal);
    nfui_nffixed_put((NFFIXED*)cal_fixed, g_cal, 50, 85);

    obj = nftool_normal_button_create_type3(calButton[0], SBT_CAL_BUTTON_SIZE_W);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_regi_post_event_callback(obj, post_first_button_event_cb);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)cal_fixed, obj, SBT_CAL_FIRST_BUTTON_X, SBT_CAL_FIRST_BUTTON_Y);
    g_first_btn = obj;

    obj = nftool_normal_button_create_type3(calButton[1], SBT_CAL_BUTTON_SIZE_W);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_regi_post_event_callback(obj, post_last_button_event_cb);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)cal_fixed, obj, SBT_CAL_LAST_BUTTON_X, SBT_CAL_LAST_BUTTON_Y);
    g_last_btn = obj;

// CALENDAR - TIME
    datetime_img[0] = nfui_get_image_from_file((IMG_BT_DATETIME_N), NULL);
    datetime_img[1] = nfui_get_image_from_file((IMG_BT_DATETIME_O), NULL);
    datetime_img[2] = nfui_get_image_from_file((IMG_BT_DATETIME_P), NULL);
    datetime_img[3] = nfui_get_image_from_file((IMG_BT_DATETIME_D), NULL);

    nfui_get_pixbuf_size(datetime_img[0], &size_w, &size_h);

    dtf_get_local_datetime(cur_utime, cur_buffer);
    dt_obj = (NFOBJECT*)nfui_nflabel_new_text_box(cur_buffer, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)dt_obj, NFTEXTBOX_TYPE_INPUT);
    nfui_nflabel_set_align((NFLABEL*)dt_obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(dt_obj, SBT_CAL_TIME_LABEL_W - size_w, SBT_LABEL_HEIGHT1);
    nfui_nfobject_use_focus(dt_obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(dt_obj);
    nfui_nffixed_put((NFFIXED*)fixed1, dt_obj, SBT_CAL_TIME_LABEL_X, SBT_CAL_TIME_LABEL_Y);



    obj = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(obj), datetime_img);
    nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed1, obj, SBT_CAL_TIME_LABEL_X+SBT_CAL_TIME_LABEL_W-size_w, SBT_CAL_TIME_LABEL_Y);
    nfui_regi_post_event_callback(obj, post_datetime_event_handler);
    dt_btn_obj = obj;

// PREVIEW
//  PREVIEW
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PREVIEW", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(662));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(obj, SBT_PREVIEW_LABEL_WIDTH, SBT_LABEL_HEIGHT2);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed1, obj, SBT_PREVIEW_LABEL_X, SBT_PREVIEW_LABEL_Y);

    /** TEMP **/
    preview_obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(665));
    nfui_nflabel_set_align((NFLABEL*)preview_obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(preview_obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(666));
    nfui_nfobject_set_size(preview_obj, SBT_PREVIEW_VIDEO_W, SBT_PREVIEW_VIDEO_H);
    nfui_nfobject_use_focus(preview_obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(preview_obj);
    nfui_nffixed_put((NFFIXED*)fixed1, preview_obj, SBT_PREVIEW_VIDEO_X, SBT_PREVIEW_VIDEO_Y);

    /** TEMP **/
    play_time_obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MINI_SEMI_5), COLOR_IDX(664));
    nfui_nflabel_set_align((NFLABEL*)play_time_obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(play_time_obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(play_time_obj, SBT_PREVIEW_VIDEO_W, SBT_LABEL_HEIGHT2);
    nfui_nfobject_use_focus(play_time_obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(play_time_obj);
    nfui_nffixed_put((NFFIXED*)fixed1, play_time_obj, SBT_PREVIEW_TIME_X, SBT_PREVIEW_TIME_Y);

// RECORD REASON
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strRec[0], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(638));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(obj, SBT_TL_REC_TEXT_PRE_W, SBT_TL_REC_BOX_H + 4);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed2, obj, SBT_TL_REC_TEXT_PRE_X, SBT_TL_REC_BOX_PRE_Y);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strRec[1], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(638));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(obj, SBT_TL_REC_TEXT_TIME_W, SBT_TL_REC_BOX_H + 4);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed2, obj, SBT_TL_REC_TEXT_TIME_X, SBT_TL_REC_BOX_TIME_Y);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strRec[2], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(638));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(obj, SBT_TL_REC_TEXT_ALARM_W, SBT_TL_REC_BOX_H + 4);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed2, obj, SBT_TL_REC_TEXT_ALARM_X, SBT_TL_REC_BOX_ALARM_Y);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strRec[3], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(638));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(obj, SBT_TL_REC_TEXT_MOT_W, SBT_TL_REC_BOX_H + 4);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed2, obj, SBT_TL_REC_TEXT_MOT_X, SBT_TL_REC_BOX_MOT_Y);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strRec[4], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(638));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(obj, SBT_TL_REC_TEXT_PANIC_W, SBT_TL_REC_BOX_H + 4);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed2, obj, SBT_TL_REC_TEXT_PANIC_X, SBT_TL_REC_BOX_PANIC_Y);


    memset(&figure, 0x00, sizeof(TML_FIGURE_T));
    figure.len = LEN_1350;
    figure.type = TML_HORIZONTAL;
    figure.cr_bg = UX_COLOR(631);
    figure.cr_rbar[0] = UX_COLOR(630);
    figure.cr_rbar[1] = UX_COLOR(639);
    figure.cr_rbar[2] = UX_COLOR(641);
    figure.cr_rbar[3] = UX_COLOR(643);
    figure.cr_rbar[4].red = 0x0000;
    figure.cr_rbar[4].green = 0x0000;
    figure.cr_rbar[4].blue = 0xff00;
    figure.cr_rbar[5] = UX_COLOR(645);
    figure.cr_rbar[6] = UX_COLOR(647);
    figure.cr_rbar[7] = UX_COLOR(630);
    figure.cr_rbar[8] = UX_COLOR(640);
    figure.cr_rbar[9] = UX_COLOR(642);
    figure.cr_rbar[10] = UX_COLOR(644);
    figure.cr_rbar[11].red = 0x0000;
    figure.cr_rbar[11].green = 0x0000;
    figure.cr_rbar[11].blue = 0xff00;
    figure.cr_rbar[12] = UX_COLOR(646);
    figure.cr_rbar[13] = UX_COLOR(648);
    figure.cr_ruler_bg = UX_COLOR(630);
    figure.cr_ruler_text = UX_COLOR(347);
    figure.cr_playbar = UX_COLOR(358);
    figure.cr_cti = UX_COLOR(360);

    figure.ruler_brd = HEIGHT_RULER;
    figure.ch_brd = HEIGHT_CH;
    figure.pb_curtain = nfui_get_image_from_file(HRZ_CURTAIN2, NULL);

    tml = tml_new(parent);
    hei = var_get_ch_count() * HEIGHT_CH + HEIGHT_RULER;
    nfui_nfobject_set_size(tml, 1350, hei);
    nfui_nffixed_put((NFFIXED*)fixed2, tml, TML_X, TML_Y);
    tml_init(tml, &figure, TML_X, TML_Y, 1350, hei, var_get_ch_count(), -1);

    tml_set_style(tml, TML_PLAYDRGABLE);
    tml_set_style(tml, TML_GHOSTCTI);
    tml_set_cti_position(tml, 0);
    nfui_nfobject_show(tml);
    nfui_regi_post_event_callback(tml, post_timeline_event_cb);


#ifdef _SUPPORT_ARBITRARILY_CH_SEL
    for (i = 0; i < var_get_ch_count(); ++i) {
        ch_sel[i] = (NFOBJECT*)nfui_checkbutton_new(TRUE);
        nfui_check_button_set_skin_type(NF_CHECKBUTTON(ch_sel[i]), NFCHECK_TYPE_SMALL);
        nfui_check_get_size(ch_sel[i], &size_w, &size_h);
        nfui_nfobject_show(ch_sel[i]);
        nfui_nffixed_put((NFFIXED*)fixed2, ch_sel[i], SBT_TML_CH_CHECK_X, SBT_TML_CH_CHECK_Y + HEIGHT_RULER + size_h*i);
        nfui_regi_post_event_callback(ch_sel[i], post_ch_event_handler);
    }

    ch_all = (NFOBJECT*)nfui_checkbutton_new(TRUE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(ch_all), NFCHECK_TYPE_SMALL);
    nfui_check_get_size(ch_all, &size_w, &size_h);
    nfui_nfobject_show(ch_all);
    nfui_nffixed_put((NFFIXED*)fixed2, ch_all, SBT_TML_CH_CHECK_X, SBT_TML_CH_CHECK_Y);
    nfui_regi_post_event_callback(ch_all, post_allch_event_handler);
#endif


    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ALL", nffont_get_pango_font(NFFONT_MINI_SEMI), COLOR_IDX(637));
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(635));
    nfui_nfobject_set_size(obj, 31, 18);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed2, obj, SBT_TML_CH_LABEL_X, SBT_TML_CH_LABEL_Y);


    for (i = 0; i < var_get_ch_count(); ++i) {
        sprintf(strBuf, "%d", i+1);
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MINI_SEMI), COLOR_IDX(637));
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX((635)));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
        nfui_nfobject_set_size(obj, 31, 18);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed2, obj, SBT_TML_CH_LABEL_X, SBT_TML_CH_LABEL_Y + HEIGHT_RULER + SBT_TML_CH_LABEL_H*i);
    }


// TIME LINE BUTTON
    tl_btn_img[0][0] = nfui_get_image_from_file((IMG_SUBMENU_TL_ZOOMOUT_N_BTN), NULL);
    tl_btn_img[0][1] = nfui_get_image_from_file((IMG_SUBMENU_TL_ZOOMOUT_O_BTN), NULL);
    tl_btn_img[0][2] = nfui_get_image_from_file((IMG_SUBMENU_TL_ZOOMOUT_P_BTN), NULL);
    tl_btn_img[0][3] = nfui_get_image_from_file((IMG_SUBMENU_TL_ZOOMOUT_D_BTN), NULL);

    tl_btn_img[1][0] = nfui_get_image_from_file((IMG_SUBMENU_TL_ZOOMIN_N_BTN), NULL);
    tl_btn_img[1][1] = nfui_get_image_from_file((IMG_SUBMENU_TL_ZOOMIN_O_BTN), NULL);
    tl_btn_img[1][2] = nfui_get_image_from_file((IMG_SUBMENU_TL_ZOOMIN_P_BTN), NULL);
    tl_btn_img[1][3] = nfui_get_image_from_file((IMG_SUBMENU_TL_ZOOMIN_D_BTN), NULL);

    tl_btn_img[2][0] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_N_BTN), NULL);
    tl_btn_img[2][1] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_O_BTN), NULL);
    tl_btn_img[2][2] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_P_BTN), NULL);
    tl_btn_img[2][3] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_D_BTN), NULL);

    tl_btn_img[3][0] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_N_BTN), NULL);
    tl_btn_img[3][1] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_O_BTN), NULL);
    tl_btn_img[3][2] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_P_BTN), NULL);
    tl_btn_img[3][3] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_D_BTN), NULL);

    tl_btn_img[4][0] = nfui_get_image_from_file((IMG_N_SYNC_TIME), NULL);
    tl_btn_img[4][1] = nfui_get_image_from_file((IMG_O_SYNC_TIME), NULL);
    tl_btn_img[4][2] = nfui_get_image_from_file((IMG_P_SYNC_TIME), NULL);
    tl_btn_img[4][3] = nfui_get_image_from_file((IMG_D_SYNC_TIME), NULL);

    ch = var_get_ch_count();

    nfui_get_pixbuf_size(tl_btn_img[4][0], &size_w, &size_h);
    pos_x = SBT_TL_ZOOMOUT_BTN_X - (size_w + 10);
    zoom_btn_y = TML_Y + HEIGHT_RULER + (ch * HEIGHT_CH) + 6;

    obj = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(obj), tl_btn_img[4]);
    nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed2, obj, pos_x, zoom_btn_y);
    nfui_regi_post_event_callback(obj, post_refresh_button_event_cb);
    refreshbtn_obj = obj;

    pos_x = SBT_TL_ZOOMOUT_BTN_X;

    nfui_get_pixbuf_size(tl_btn_img[0][0], &size_w, &size_h);
    obj = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(obj), tl_btn_img[0]);
    nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed2, obj, pos_x, zoom_btn_y);
    nfui_regi_post_event_callback(obj, post_zoomout_button_event_cb);

    pos_x += size_w+SBT_TL_ZOOM_BTN_GAP;

    obj = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(obj), tl_btn_img[1]);
    nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed2, obj, pos_x, zoom_btn_y);
    nfui_regi_post_event_callback(obj, post_zoomin_button_event_cb);

    pos_x += size_w+SBT_TL_BTN_GAP;
    nfui_get_pixbuf_size(tl_btn_img[2][0], &size_w, &size_h);

    obj = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(obj), tl_btn_img[2]);
    nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed2, obj, pos_x, zoom_btn_y);
    nfui_regi_post_event_callback(obj, post_prev_button_event_cb);

    pos_x += size_w+SBT_TL_DIR_BTN_GAP;

    obj = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(obj), tl_btn_img[3]);
    nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed2, obj, pos_x, zoom_btn_y);
    nfui_regi_post_event_callback(obj, post_next_button_event_cb);


//  nfui_nftimelabel_get_datetime((NFTIMELABEL*)dt_obj, &end_time);

// PREVIEW TIME LABEL
    size_w = nfutil_string_width(1, NULL, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), "THUMBNAIL", NORMAL_SPACING);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("THUMBNAIL", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(660));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(obj, size_w+10, SBT_PREVIEW_TIME_LABEL_H);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed3, obj, SBT_PREVIEW_TIME_LABEL_X, SBT_PREVIEW_TIME_LABEL_Y);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_SMALL_THIN), COLOR_IDX(661));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(obj, 100, SBT_PREVIEW_TIME_LABEL_H);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed3, obj, SBT_FIXED3_W-27-100, SBT_PREVIEW_TIME_LABEL_Y);
    nfui_nfobject_set_data(fixed3, "CMPL RATE", (gpointer)obj);

	prev_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_N_BTN), NULL);
	prev_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_O_BTN), NULL);
	prev_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_P_BTN), NULL);
	prev_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_D_BTN), NULL);

	next_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_N_BTN), NULL);
	next_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_O_BTN), NULL);
	next_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_P_BTN), NULL);
	next_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_D_BTN), NULL);


    size_w = fixed3->width - 30;

    main_page_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(main_page_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(main_page_fixed, size_w, (PREVIEW_OUTLINE_BOX_H) + 60);
    nfui_nfobject_show(main_page_fixed);
    nfui_nffixed_put((NFFIXED*)fixed3, main_page_fixed, 15, 28);

    for (i = 0; i < PAGE_FIXED_CNT; i++)
    {
        g_page_fixed[i] = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_modify_bg(g_page_fixed[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
        nfui_nfobject_set_size(g_page_fixed[i], size_w, PREVIEW_OUTLINE_BOX_H);
        nfui_nffixed_put((NFFIXED*)main_page_fixed, g_page_fixed[i], 0, 0);
    }
    nfui_nfobject_show(g_page_fixed[0]);

	nfui_get_pixbuf_size(prev_img[0], &size_w, &size_h);
	
	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), prev_img);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_page_fixed, obj, (main_page_fixed->width / 2) - (size_w + 60), main_page_fixed->height - size_h);
	nfui_regi_post_event_callback(obj, post_prev_event_handler);

	memset(strBuf, 0x00, sizeof(strBuf));
	g_sprintf(strBuf, "1 / %d", PAGE_FIXED_CNT);
	
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 100, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_page_fixed, obj, (main_page_fixed->width - 100) / 2, main_page_fixed->height - size_h);
    g_lb_page_num = obj;
    
	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), next_img);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_page_fixed, obj, (main_page_fixed->width / 2) + 60, main_page_fixed->height - size_h);
	nfui_regi_post_event_callback(obj, post_next_event_handler);

// PREVIEW PICTURE LABEL
    pos_x = 0;
    pos_y = 0;
    
    page_num = col_num = 0;

    for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
    {
        sprintf(strBuf, "CAM %d", ch+1);
        thumb_obj[ch] = (NFOBJECT*)nfui_nfthumbnail_new(strBuf, nffont_get_pango_font(NFFONT_MINI_SEMI_3));
        nfui_nfthumbnail_subject_label_height(thumb_obj[ch], SBT_PREVIEW_PIC_LABEL_H);
        nfui_nfthumbnail_set_image_buf(thumb_obj[ch], ch_image[ch]);
        nfui_nfthumbnail_set_channel(thumb_obj[ch], ch);
        nfui_nfthumbnail_set_image_size(thumb_obj[ch], (PREVIEW_OUTLINE_BOX_W-2), (PREVIEW_OUTLINE_BOX_H-SBT_PREVIEW_PIC_LABEL_H-2));
        nfui_nfobject_use_focus(thumb_obj[ch], TRUE);
        nfui_nfobject_set_size(thumb_obj[ch], PREVIEW_OUTLINE_BOX_W, PREVIEW_OUTLINE_BOX_H);
        nfui_nfobject_show(thumb_obj[ch]);
    	nfui_nffixed_put((NFFIXED*)g_page_fixed[page_num], thumb_obj[ch], pos_x + ((PREVIEW_OUTLINE_BOX_W + PREVIEW_OUTLINE_BOX_GAP_X) * col_num), pos_y);
        nfui_regi_post_event_callback(thumb_obj[ch], post_pic_event_handler);

/*
        if ((ch+1) % THUMB_COL_CNT == 0)
        {
            pos_x = 15;
            pos_y += PREVIEW_OUTLINE_BOX_H + PREVIEW_OUTLINE_BOX_GAP_Y;
        }
        else
        {
            pos_x += PREVIEW_OUTLINE_BOX_W+PREVIEW_OUTLINE_BOX_GAP_X;
        }
*/
        col_num++;

        if (col_num == THUMB_COL_CNT) {
            col_num = 0;
            page_num++;
        }
    }


// PLAYBACK / CLOSE BUTTON
    obj = nftool_normal_button_create_type1(strButton[0], MENU_BTN_WIDTH);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_H_BTN_R2_X, MENU_H_BTN_Y);
    nfui_regi_post_event_callback(obj, post_playbutton_event_handler);
    playbtn_obj = obj;

    obj = nftool_normal_button_create_type2(strButton[1], MENU_BTN_WIDTH);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_H_BTN_R1_X, MENU_H_BTN_Y);
    nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

    nfui_regi_pre_event_callback(cal_fixed, pre_cal_fixed_event_handler);
    nfui_regi_pre_event_callback(fixed1, pre_fixed1_event_handler);
    nfui_regi_pre_event_callback(fixed2, pre_fixed2_event_handler);
    nfui_regi_pre_event_callback(fixed3, pre_fixed3_event_handler);
    nfui_regi_post_event_callback(fixed2, post_fixed2_event_handler);

    nfui_regi_post_event_callback(parent, post_page_event_handler);

    uxm_reg_imsg_event(tml, INFY_TML_DATE_CHANGED);
    uxm_reg_imsg_event(tml, INFY_TML_PLAY_CHANGED);
    uxm_reg_imsg_event(tml, INFY_TML_START_CHANGED);
    uxm_reg_imsg_event(tml, INFY_TML_END_CHANGED);
    uxm_reg_imsg_event(tml, INFY_TML_SCROLL_UP);
    uxm_reg_imsg_event(tml, INFY_TML_SCROLL_DOWN);
    uxm_reg_imsg_event(tml, INFY_TML_DOUBLE_CLICKED);
    uxm_reg_imsg_event(fixed3, INFY_THUMBNAIL_CMPL_OBJ);
    uxm_reg_imsg_event(fixed3, INFY_THUMBNAIL_TOTALLY_CMPL);

    tml_reset_cur_day(tml, search_time);
    tml_set_playstick_time_t(tml, search_time);
    tml_refresh(tml);
}


gboolean vw_SearchByTime_tab_out_handler()
{
    guint i;

    _preview_obj_off("");
    vsm_playback_preview_stop();

    return FALSE;
}

gboolean vw_SearchByTime_tab_in_handler()
{
    search_time = stm_get_time_t();
    g_init_thumb = TRUE;

    tml_set_playstick_time_t(tml, search_time);
    tml_refresh(tml);
    return FALSE;
}

gboolean vw_SearchByTime_tab_show()
{
    search_time = stm_get_time_t();
    tml_zoom_min(tml);
    tml_reset_cur_day(tml, search_time);

    vw_SearchByTime_tab_in_handler();
    return FALSE;
}
