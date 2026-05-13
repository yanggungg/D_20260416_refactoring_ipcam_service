#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nfscrolledfixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nftimespin.h"
#include "objects/nfcheckbutton.h"
#include "objects/nflistbox.h"
#include "objects/nfbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nftimelabel.h"

#include "ix_mem.h"
#include "dtf.h"
#include "stm.h"
#include "var.h"
#include "scm.h"

#include "vw_search_main.h"
#include "vw_search_by_text.h"

#include "vw_internal.h"
#include "vw_vkeyboard.h"
#include "vw_set_date_time.h"
#include "vw_live_statusbar.h"
#include "vw_search_keyword_popup.h"



// FIXED1 , FIXED2

#define SBE_FIXED1_X                    (0)
#define SBE_FIXED1_Y                    (0)
#define SBE_FIXED1_W                    (455)
#define SBE_FIXED1_H                    (903)

#define SBE_FIXED2_X                    (SBE_FIXED1_X+SBE_FIXED1_W)
#define SBE_FIXED2_Y                    (SBE_FIXED1_Y)
#define SBE_FIXED2_W                    (1401)
#define SBE_FIXED2_H                    (SBE_FIXED1_H)

#define SBE_LABEL_HEIGHT1               (40)
#define SBE_LABEL_HEIGHT2               (26)
#define SBE_LABEL_HEIGHT3               (27)

#define SBE_LOGLABEL_HEIGHT             (38)

// FROM / TO.
#define SBE_TABLE1_X                    (15)
#define SBE_TABLE1_Y                    (34)
#define SBE_TABLE1_COLUMNS              (3)
#define SBE_TABLE1_ROWS                 (2)

// VIDEO CHANNEL
#define SBE_EVENT_CHANNEL_LABEL_X       (SBE_TABLE1_X)
#define SBE_EVENT_CHANNEL_LABEL_Y       (126)

#define SBE_EVENT_CHANNEL_FIXED_X       (11)
#define SBE_EVENT_CHANNEL_FIXED_Y       (SBE_EVENT_CHANNEL_LABEL_Y+SBE_LABEL_HEIGHT2+10)
#define SBE_EVENT_CHANNEL_FIXED_W       (417)
#define SBE_EVENT_CHANNEL_FIXED_H       (60 + 3*38)

// KEYWORD
#define SBE_EVENT_KEYWORD_LABEL_X       (SBE_EVENT_CHANNEL_LABEL_X)
#define SBE_EVENT_KEYWORD_LABEL_Y       (SBE_EVENT_CHANNEL_FIXED_Y+230)

#define SBE_EVENT_KEYWORD_FIXED_X       (11)
#define SBE_EVENT_KEYWORD_FIXED_Y       (SBE_EVENT_KEYWORD_LABEL_Y+SBE_LABEL_HEIGHT2+3)
#define SBE_EVENT_KEYWORD_FIXED_W       (417)
#define SBE_EVENT_KEYWORD_FIXED_H       (140)

// SEARCH BUTTON
#define SBE_SEARCH_BUTTON_X             (SBE_EVENT_KEYWORD_FIXED_X + 209)
#define SBE_SEARCH_BUTTON_Y             (SBE_EVENT_KEYWORD_FIXED_Y + SBE_EVENT_KEYWORD_FIXED_H + 10)

// PREVIEW
#define SBE_PREVIEW_LABEL_X             (16)
#define SBE_PREVIEW_LABEL_Y             (SBE_SEARCH_BUTTON_Y+50)
#define SBE_PREVIEW_LABEL_WIDTH         (220)

#define SBE_PREVIEW_VIDEO_X             (SBE_PREVIEW_LABEL_X)
#define SBE_PREVIEW_VIDEO_Y             (SBE_PREVIEW_LABEL_Y+SBE_LABEL_HEIGHT2+4) //+2
#define SBE_PREVIEW_VIDEO_W             (408) // 410
#define SBE_PREVIEW_VIDEO_H             (228) // 230

#define SBE_PREVIEW_TIME_X              (SBE_PREVIEW_VIDEO_X)
#define SBE_PREVIEW_TIME_Y              (SBE_PREVIEW_VIDEO_Y+SBE_PREVIEW_VIDEO_H+4)

// EVENT LOG
#define SBE_LOG_LABEL_ROWS              (19)
#define SBE_LOG_HEIGHT_GAP              (4)
#define SBE_LOG_WIDTH_GAP               (2)
#define SBE_LOG_LEFT_MARGIN             (10)

#define SBE_LOG_X                       (0)
#define SBE_LOG_Y                       (0)

#define SBE_LOG_TIME_X                  (SBE_LOG_X)
#define SBE_LOG_TIME_W                  (279)

#define SBE_LOG_CHANNEL_X               (SBE_LOG_X + SBE_LOG_TIME_W+SBE_LOG_WIDTH_GAP)
#define SBE_LOG_CHANNEL_W               (239)

#define SBE_LOG_ITEM_X                  (SBE_LOG_CHANNEL_X + SBE_LOG_CHANNEL_W+SBE_LOG_WIDTH_GAP)
#define SBE_LOG_ITEM_W                  (878)

// LOG ORDER
#define SBE_LOG_ORDER_X                 (SBE_LOG_X)
#define SBE_LOG_ORDER_Y                 (SBE_LOG_Y + (SBE_LOGLABEL_HEIGHT+SBE_LOG_HEIGHT_GAP)*(SBE_LOG_LABEL_ROWS+1) + 6)
#define SBE_LOG_ORDER_W                 (180)
#define SBE_LOG_ORDER_CMB_W             (260)

// LOG DIR
#define SBE_LOG_BUTTON_SIZE_W           (84)
#define SBE_LOG_TEXT_SIZE_W             (120)

#define SBE_LOG_PREV_BUTTON_X           (SBE_LOG_ORDER_X + 556)
#define SBE_LOG_PREV_BUTTON_Y           (SBE_LOG_ORDER_Y)

#define SBE_LOG_NEXT_BUTTON_X           (SBE_LOG_PREV_BUTTON_X + SBE_LOG_BUTTON_SIZE_W + SBE_LOG_TEXT_SIZE_W)
#define SBE_LOG_NEXT_BUTTON_Y           (SBE_LOG_ORDER_Y)

#define SBE_SUPPORTED_KEYWORD_CNT       (MAX_STR_NUM_CNT)

enum {
    DF_YMD = 0,
    DF_MDY,
    DF_DMY,
    NUM_DATE_FORMATS,
};

typedef enum {
    LOGLABEL_TIME       = 0,
    LOGLABEL_CHANNEL,
    LOGLABEL_ITEM,
    LOGLABEL_ALL
} LOGLABEL_E;


#define NORMAL_OUTLINE_COLOR        0
#define FOCUS_OUTLINE_COLOR         146
#define SELECT_OUTLINE_COLOR        147



static NFWINDOW *g_curwnd = 0;
static NFOBJECT *from_obj;
static NFOBJECT *to_obj;
static NFOBJECT *from_btn_obj;
static NFOBJECT *to_btn_obj;
static NFOBJECT *g_all_ch_obj;
static NFOBJECT *g_ch_obj[GUI_CHANNEL_CNT];
static NFOBJECT *g_keyword_obj;
static NFOBJECT *g_match_case_obj;
static NFOBJECT *g_match_whole_obj;
static NFOBJECT *search_obj;
static NFOBJECT *order_obj;
static NFOBJECT *prev_obj;
static NFOBJECT *next_obj;
static NFOBJECT *playback_obj;
static NFOBJECT *log_label[SBE_LOG_LABEL_ROWS][LOGLABEL_ALL];
static NFOBJECT *preview_obj = NULL;
static NFOBJECT *play_st_obj = NULL;
static NFOBJECT *play_time_obj = NULL;

static SEARCH_KEY_T g_keyword_info;

static guint preview_timer_id;

static gint focused_row = -1;
static gint selected_row = -1;
static GTimeVal g_log_time[SBE_LOG_LABEL_ROWS];

static gboolean g_init_sbe = TRUE;


static nftl_df_type prvTransDateFormat(gint db_index)
{
    nftl_df_type ret;

    if(db_index == DF_YMD)          ret = NFTL_DF_YMD;
    else if(db_index == DF_MDY)     ret = NFTL_DF_MDY;
    else if(db_index == DF_DMY)     ret = NFTL_DF_DMY;
    else    ret = NFTL_DF_HIDE;

    return ret;
}

static void _dir_prev_button_enable()
{
    nfui_nfobject_enable(prev_obj);
    nfui_signal_emit(prev_obj, GDK_EXPOSE, FALSE);
}

static void _dir_prev_button_disable()
{
    nfui_nfobject_disable(prev_obj);
    nfui_signal_emit(prev_obj, GDK_EXPOSE, FALSE);
}

static void _dir_next_button_enable()
{
    nfui_nfobject_enable(next_obj);
    nfui_signal_emit(next_obj, GDK_EXPOSE, FALSE);
}

static void _dir_next_button_disable()
{
    nfui_nfobject_disable(next_obj);
    nfui_signal_emit(next_obj, GDK_EXPOSE, FALSE);
}

static void _playback_button_enable()
{
    nfui_nfobject_enable(playback_obj);
    nfui_signal_emit(playback_obj, GDK_EXPOSE, FALSE);
}

static void _playback_button_disable()
{
    nfui_nfobject_disable(playback_obj);
    nfui_signal_emit(playback_obj, GDK_EXPOSE, FALSE);
}

static void _order_button_enable()
{
    nfui_nfobject_enable(order_obj);
    nfui_signal_emit(order_obj, GDK_EXPOSE, TRUE);
}

static void _order_button_disable()
{
    nfui_nfobject_disable(order_obj);
    nfui_signal_emit(order_obj, GDK_EXPOSE, TRUE);
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

static void _change_outLine_LogLabel(gint row, gint color)
{
    static GdkGC *gc;
    static GdkDrawable *drawable;
    gint x, y;

    drawable = nfui_nfobject_get_window((NFOBJECT*)log_label[row][LOGLABEL_TIME]);
    gc = nfui_nfobject_get_gc(log_label[row][LOGLABEL_TIME]);

    gdk_gc_set_line_attributes(gc,
            2,
            GDK_LINE_SOLID,
            GDK_CAP_NOT_LAST,
            GDK_JOIN_MITER);

    nfui_nfobject_get_offset(log_label[row][LOGLABEL_TIME], &x, &y);
    gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(color));
    gdk_draw_rectangle (drawable, gc, FALSE, x-1, y-1,
            SBE_LOG_TIME_W+2+SBE_LOG_CHANNEL_W+2+SBE_LOG_ITEM_W+2, SBE_LOGLABEL_HEIGHT+2);

    nfui_nfobject_gc_unref(gc);
}

static gint _draw_selectLine(gint row)
{
    if (selected_row == row)
        return -1;

    if (selected_row != -1)
        _change_outLine_LogLabel(selected_row, NORMAL_OUTLINE_COLOR);

    _change_outLine_LogLabel(row, SELECT_OUTLINE_COLOR);
    selected_row = row;

    return 0;
}

static gint _erase_selectLine()
{
    if (selected_row == -1)
        return -1;

    _change_outLine_LogLabel(selected_row, NORMAL_OUTLINE_COLOR);
    selected_row = -1;

    return 0;
}

static gint _draw_focusLine(gint row)
{
    if (focused_row == row)
        return -1;

// leave old focus row
    if ((selected_row != -1) && (selected_row == focused_row))
        _change_outLine_LogLabel(selected_row, SELECT_OUTLINE_COLOR);
    else if (focused_row != -1)
        _change_outLine_LogLabel(focused_row, NORMAL_OUTLINE_COLOR);

// enter new focus row
    _change_outLine_LogLabel(row, FOCUS_OUTLINE_COLOR);
    focused_row = row;

    return 0;
}

static gint _erase_focusLine(gint row)
{
    if (row == selected_row)
        _change_outLine_LogLabel(row, SELECT_OUTLINE_COLOR);
    else
        _change_outLine_LogLabel(row, NORMAL_OUTLINE_COLOR);

    focused_row = -1;

    return 0;
}

static void _set_data_LogLabel(gint index, TLOG_DATA_T ld)
{
    guint ch_mask = 0;
    LPR_POS_T *pc = (LPR_POS_T *)&ld.p;

    ch_mask |= (1 << pc->channel);

    g_log_time[index].tv_sec = ld.tvTime.tv_sec - 3;
    nfui_nfobject_set_data(log_label[index][LOGLABEL_ITEM], "log chmask", GUINT_TO_POINTER(ch_mask));
}

static void _trans_logData_into_text(TLOG_DATA_T ld, gchar *channel, gchar *contents)
{
    sprintf(channel, "CH %02d", ld.p.cat_pos.channel+1);
    sprintf(contents, "%s", ld.p.cat_pos.text);
}

static void _update_log_data(gint log_cnt, TLOG_DATA_T data[SBE_LOG_LABEL_ROWS])
{
    gint i;
    gchar strTime[128];
    gchar strCh[8];
    gchar strCts[1024];

    for(i = 0; i < SBE_LOG_LABEL_ROWS; i++)
    {
        if (i < log_cnt)
        {
            _set_data_LogLabel(i, data[i]);

            memset(strTime, 0x00, sizeof(strTime));
            memset(strCh, 0x00, sizeof(strCh));
            memset(strCts, 0x00, sizeof(strCts));

            _trans_logData_into_text(data[i], strCh, strCts);
            dtf_get_local_datetime(data[i].tvTime.tv_sec, strTime);

            nfui_nflabel_set_text((NFLABEL*)log_label[i][LOGLABEL_TIME], strTime);
            nfui_nflabel_set_text((NFLABEL*)log_label[i][LOGLABEL_CHANNEL], strCh);
            nfui_nflabel_set_text((NFLABEL*)log_label[i][LOGLABEL_ITEM], strCts);
        }
        else
        {
            g_log_time[i].tv_sec = 0;
            nfui_nflabel_set_text((NFLABEL*)log_label[i][LOGLABEL_TIME], "");
            nfui_nflabel_set_text((NFLABEL*)log_label[i][LOGLABEL_CHANNEL], "");
            nfui_nflabel_set_text((NFLABEL*)log_label[i][LOGLABEL_ITEM], "");
        }

        nfui_signal_emit(log_label[i][LOGLABEL_TIME], GDK_EXPOSE, FALSE);
        nfui_signal_emit(log_label[i][LOGLABEL_CHANNEL], GDK_EXPOSE, FALSE);
        nfui_signal_emit(log_label[i][LOGLABEL_ITEM], GDK_EXPOSE, FALSE);
    }
}

static void _search_log()
{
    TLOGCTX tlog_ctx;
    GTimeVal from_time;
    GTimeVal to_time;
    gboolean next = FALSE;

    TLOG_DATA_T log_data[SBE_LOG_LABEL_ROWS];
    gint log_cnt;

    memset(&from_time, 0x00, sizeof(GTimeVal));
    memset(&to_time, 0x00, sizeof(GTimeVal));

    _dir_prev_button_disable();
    _playback_button_disable();
    _erase_selectLine();
    _preview_obj_off("");
    vsm_playback_preview_stop();

    tlog_ctx = GPOINTER_TO_SIZE(nfui_nfobject_get_data(search_obj, "tlog ctx"));
    nfui_nftimelabel_get_datetime((NFTIMELABEL*)from_obj, &from_time);
    nfui_nftimelabel_get_datetime((NFTIMELABEL*)to_obj, &to_time);

    log_cnt = scm_get_tlog(tlog_ctx, &from_time, &to_time, SBE_LOG_LABEL_ROWS, log_data, &next);
    _update_log_data(log_cnt, log_data);

    if (next) _dir_next_button_enable();
    else _dir_next_button_disable();
}

static void _hide_log()
{
    gint i;

    memset(g_log_time, 0x00, sizeof(GTimeVal)*SBE_LOG_LABEL_ROWS);

    for(i = 0; i < SBE_LOG_LABEL_ROWS; i++) {
        nfui_nflabel_set_text((NFLABEL*)log_label[i][LOGLABEL_TIME], "");
        nfui_nflabel_set_text((NFLABEL*)log_label[i][LOGLABEL_CHANNEL], "");
        nfui_nflabel_set_text((NFLABEL*)log_label[i][LOGLABEL_ITEM], "");

        nfui_signal_emit(log_label[i][LOGLABEL_TIME], GDK_EXPOSE, FALSE);
        nfui_signal_emit(log_label[i][LOGLABEL_CHANNEL], GDK_EXPOSE, FALSE);
        nfui_signal_emit(log_label[i][LOGLABEL_ITEM], GDK_EXPOSE, FALSE);
    }
}

static gint _find_index_matched_logOBJ(NFOBJECT *obj)
{
    gint i;

    for(i = 0; i < SBE_LOG_LABEL_ROWS; i++)
    {
        if ((log_label[i][LOGLABEL_TIME] == obj)
            || (log_label[i][LOGLABEL_CHANNEL] == obj)
            || (log_label[i][LOGLABEL_ITEM] == obj))
        {
            break;
        }
    }

    if (i == SBE_LOG_LABEL_ROWS)
    {
        g_warning("%s, %d, not matched", __FUNCTION__, __LINE__);
        g_assert(0);
    }

    return i;
}

static gint _find_column_matched_logOBJ(NFOBJECT *obj, gint row)
{
    gint i;

    for(i = 0; i < LOGLABEL_ALL; i++)
    {
        if (log_label[row][i] == obj)
            break;
    }

    if (i == LOGLABEL_ALL)
    {
        g_warning("%s, %d, not matched", __FUNCTION__, __LINE__);
        g_assert(0);
    }

    return i;
}

static void _evt_to_label_playback_handling(gint index)
{
    guint ch_mask;

    ch_mask = GPOINTER_TO_UINT(nfui_nfobject_get_data(log_label[index][LOGLABEL_ITEM], "log chmask"));

/*
    if ((ssm_get_covert_mask() & ch_mask) == ch_mask) {
        _preview_obj_off("COVERT");
        return;
    }
*/

    vsm_playback_preview_stop();
    _preview_obj_off("");

    VW_Search_start_playback();
    vsm_playback_start(ch_mask, g_log_time[index], PLAYBACK_NORMAL);
}

static void _evt_to_label_preview_handling(gint index)
{
    guint ch_mask;
    gint preview_x, preview_y;
    gint shown_as;

    vsm_playback_preview_stop();
    _playback_button_enable();

    ch_mask = GPOINTER_TO_UINT(nfui_nfobject_get_data(log_label[index][LOGLABEL_ITEM], "log chmask"));

    if (ssm_get_covert_mask() & ch_mask)
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
        return;
    }

    if (ch_mask == 0xffff)
    {
        _preview_obj_off("N/A");
    }
    else
    {
        if ((ssm_get_covert_mask() & ch_mask) == ch_mask)
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
                return;
        }

        nfui_nfobject_get_offset((NFOBJECT*)preview_obj, &preview_x, &preview_y);

        _preview_obj_on("");
        vsm_playback_preview_start(ch_mask, g_log_time[index], preview_x, preview_y, SBE_PREVIEW_VIDEO_W, SBE_PREVIEW_VIDEO_H);
    }
}

static void _change_obj_focus(NFOBJECT* from, NFOBJECT *to)
{
    nfui_set_key_focus(from, FALSE);
    nfui_set_key_focus(to, TRUE);

    nfui_signal_emit(from, GDK_EXPOSE, TRUE);
    nfui_signal_emit(to, GDK_EXPOSE, TRUE);
}

static gboolean _set_preview_time(gpointer data)
{
    GTimeVal tmp;
    gchar strBuf[64];

    memset(&tmp, 0x00, sizeof(GTimeVal));
    tmp = vsm_playback_get_previewtime();

    if (tmp.tv_sec != 0)
    {
        dtf_get_local_datetime(tmp.tv_sec, strBuf);
        nfui_nflabel_set_text((NFLABEL*)play_time_obj, strBuf);
    }
    else
    {
        g_sprintf(strBuf, "");
        nfui_nflabel_set_text((NFLABEL*)play_time_obj, strBuf);
    }

    nfui_signal_emit(play_time_obj, GDK_EXPOSE, FALSE);

    return TRUE;
}

static gboolean _stop_live(void *data)
{
    if (!nfui_nfwindow_find("SEARCH"))  return FALSE;

    vsm_live_stop();
    g_init_sbe = FALSE;

    return FALSE;
}

static gboolean post_from_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    guint i;
    int x, y;
    NFOBJECT *top;

    GTimeVal from_tv;
    GTimeVal to_tv;
    GTimeVal temp_tv;

    memset(&temp_tv, 0x00, sizeof(GTimeVal));
    memset(&from_tv, 0x00, sizeof(GTimeVal));
    memset(&to_tv, 0x00, sizeof(GTimeVal));

    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        nfui_nfobject_get_offset(obj, &x, &y);
        top = nfui_nfobject_get_top(obj);

        x += top->x + obj->width + 4;
        y += top->y;

        nfui_nftimelabel_get_datetime(from_obj, &from_tv);
        nfui_nftimelabel_get_datetime(to_obj, &to_tv);
        to_tv.tv_sec -= 1;

        temp_tv.tv_sec = VW_Set_DateTime_Open(g_curwnd, "FROM", x, y, from_tv.tv_sec, SDT_TYPE_SEC, NF_LOWER_TIMELIMIT, to_tv.tv_sec);

        if (temp_tv.tv_sec != 0)
        {
            nfui_nftimelabel_set_datetime((NFTIMELABEL*)from_obj, &temp_tv);
            nfui_signal_emit((NFOBJECT*)from_obj, GDK_EXPOSE, TRUE);
        }

        if ((temp_tv.tv_sec != 0) && (from_tv.tv_sec != temp_tv.tv_sec))
        {
            _order_button_disable();
        }
    }
    else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        GdkEventKey *kevt;
        KEYPAD_KID kpid;

        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;

        if(kpid == KEYPAD_UP)
        {
            _change_obj_focus(obj, search_obj);
            return TRUE;
        }
    }

    return FALSE;
}

static gboolean post_to_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    guint i;
    int x, y;
    NFOBJECT *top;

    GTimeVal from_tv;
    GTimeVal to_tv;
    GTimeVal temp_tv;

    memset(&temp_tv, 0x00, sizeof(GTimeVal));
    memset(&from_tv, 0x00, sizeof(GTimeVal));
    memset(&to_tv, 0x00, sizeof(GTimeVal));

    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        nfui_nfobject_get_offset(obj, &x, &y);
        top = nfui_nfobject_get_top(obj);

        x += top->x + obj->width + 4;
        y += top->y;

        nfui_nftimelabel_get_datetime(from_obj, &from_tv);
        nfui_nftimelabel_get_datetime(to_obj, &to_tv);
        from_tv.tv_sec += 1;

        temp_tv.tv_sec = VW_Set_DateTime_Open(g_curwnd, "TO", x, y, to_tv.tv_sec, SDT_TYPE_SEC, from_tv.tv_sec, NF_UPPER_TIMELIMIT);

        if (temp_tv.tv_sec != 0)
        {
            nfui_nftimelabel_set_datetime((NFTIMELABEL*)to_obj, &temp_tv);
            nfui_signal_emit((NFOBJECT*)to_obj, GDK_EXPOSE, TRUE);
        }

        if ((temp_tv.tv_sec != 0) && (to_tv.tv_sec != temp_tv.tv_sec))
        {
            _order_button_disable();
        }
    }
    else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        GdkEventKey *kevt;
        KEYPAD_KID kpid;

        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;

        if(kpid == KEYPAD_DOWN)
        {
            _change_obj_focus(obj, g_all_ch_obj);
            return TRUE;
        }
    }

    return FALSE;
}

static gboolean post_all_ch_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
    TLOGCTX tlog_ctx;
    gboolean state;

    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            nfui_check_button_set_active((NFCHECKBUTTON*)g_ch_obj[i], state);
            nfui_signal_emit(g_ch_obj[i], GDK_EXPOSE, TRUE);

            tlog_ctx = GPOINTER_TO_SIZE(nfui_nfobject_get_data(search_obj, "tlog ctx"));
            scm_set_log_filter_ch(tlog_ctx, i, state);
        }

        _order_button_disable();
    }

    return FALSE;
}

static gboolean post_ch_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
    TLOGCTX tlog_ctx;
    gboolean state;
    guint chmask = 0;
    guint allmask = 0;

    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            if (g_ch_obj[i] == obj) break;
        }

        if (g_ch_obj[i] == GUI_CHANNEL_CNT) return FALSE;

        state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);

        tlog_ctx = GPOINTER_TO_SIZE(nfui_nfobject_get_data(search_obj, "tlog ctx"));
        scm_set_tlog_filter_ch(tlog_ctx, i, state);

        _order_button_disable();

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            allmask |= (1 << i);
        }

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            if (nfui_check_button_get_active((NFCHECKBUTTON*)g_ch_obj[i])) chmask |= (1 << i);
        }

        if (allmask == chmask) nfui_check_button_set_active((NFCHECKBUTTON*)g_all_ch_obj, TRUE);
        else nfui_check_button_set_active((NFCHECKBUTTON*)g_all_ch_obj, FALSE);

        nfui_signal_emit(g_all_ch_obj, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_input_keyword_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_2BUTTON_PRESS)
    {
        TLOGCTX tlog_ctx;

        gchar *keyword;
        gboolean match_case = FALSE;
        gboolean match_whole = FALSE;

        guint x, y;
        gint size_w, size_h;
        gchar strTemp[256];
        gint ret, i;
        gint firstStr = 1;

        memset(strTemp, 0x00, sizeof(strTemp));
        
        nfui_nfobject_get_offset(obj, &x, &y);
        nfui_nfobject_get_size(obj, &size_w, &size_h);

        x += size_w + 5;

        ret = VW_Search_Keyword_Popup(g_curwnd, &g_keyword_info, SBE_SUPPORTED_KEYWORD_CNT, x, y);

        if (ret)
        {
            for (i = 0; i < SBE_SUPPORTED_KEYWORD_CNT; i++)
            {
                if (strlen(g_keyword_info.keyword[i])) {
                    if (firstStr) sprintf(strTemp, "%s", g_keyword_info.keyword[i]);
                    else          sprintf(strTemp, "%s, %s", strTemp, g_keyword_info.keyword[i]);
                    
                    firstStr = 0;
                }
            }

            nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
            nfui_signal_emit((NFLABEL*)obj, GDK_EXPOSE, FALSE);

            if (strlen(strTemp))
            {
                nfui_nfobject_enable(g_match_case_obj);
                nfui_nfobject_enable(g_match_whole_obj);
            }
            else
            {
                nfui_nfobject_disable(g_match_case_obj);
                nfui_nfobject_disable(g_match_whole_obj);
            }

            nfui_signal_emit(g_match_case_obj, GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_match_whole_obj, GDK_EXPOSE, TRUE);

            tlog_ctx = GPOINTER_TO_SIZE(nfui_nfobject_get_data(search_obj, "tlog ctx"));

            keyword = nfui_nflabel_get_text((NFLABEL*)g_keyword_obj);
            if (!nfui_nfobject_is_disabled(g_match_case_obj)) match_case = nfui_check_button_get_active((NFCHECKBUTTON*)g_match_case_obj);
            if (!nfui_nfobject_is_disabled(g_match_whole_obj)) match_whole = nfui_check_button_get_active((NFCHECKBUTTON*)g_match_whole_obj);

            scm_set_tlog_filter_text(tlog_ctx, &g_keyword_info, match_case, match_whole);

            _order_button_disable();
        }
    }

    return FALSE;
}

static gboolean post_match_case_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        TLOGCTX tlog_ctx;

        gchar *keyword;
        gboolean match_case;
        gboolean match_whole;

        tlog_ctx = GPOINTER_TO_SIZE(nfui_nfobject_get_data(search_obj, "tlog ctx"));

        keyword = nfui_nflabel_get_text((NFLABEL*)g_keyword_obj);
        match_case = nfui_check_button_get_active((NFCHECKBUTTON*)g_match_case_obj);
        match_whole = nfui_check_button_get_active((NFCHECKBUTTON*)g_match_whole_obj);

        scm_set_tlog_filter_text(tlog_ctx, &g_keyword_info, match_case, match_whole);

        _order_button_disable();
    }

    return FALSE;
}

static gboolean post_match_whole_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        TLOGCTX tlog_ctx;

        gchar *keyword;
        gboolean match_case;
        gboolean match_whole;

        tlog_ctx = GPOINTER_TO_SIZE(nfui_nfobject_get_data(search_obj, "tlog ctx"));

        keyword = nfui_nflabel_get_text((NFLABEL*)g_keyword_obj);
        match_case = nfui_check_button_get_active((NFCHECKBUTTON*)g_match_case_obj);
        match_whole = nfui_check_button_get_active((NFCHECKBUTTON*)g_match_whole_obj);

        scm_set_tlog_filter_text(tlog_ctx, &g_keyword_info, match_case, match_whole);

        _order_button_disable();
    }

    return FALSE;
}

static gboolean post_search_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    guint i;
    GdkEventKey *kevt;
    KEYPAD_KID kpid = KEYPAD_NONE;

    if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;
    }

    if (evt->type == GDK_BUTTON_PRESS || kpid == KEYPAD_ENTER)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        _search_log();
        _order_button_enable();
    }
    else if (kpid == KEYPAD_DOWN)
    {
        _change_obj_focus(obj, from_btn_obj);
        return TRUE;
    }

    if(evt->type == GDK_DELETE)
    {
        TLOGCTX tlog_ctx;

        tlog_ctx = GPOINTER_TO_SIZE(nfui_nfobject_get_data(search_obj, "tlog ctx"));
        scm_close_tlog_ctx(tlog_ctx);
    }

    return FALSE;
}

static gboolean pre_log_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkEventKey *kevt;
    KEYPAD_KID kpid = KEYPAD_NONE;
    gint row, col;
    NFOBJECT* cur_focus;

    if (obj->kfocus == NFOBJECT_FOCUS)
    {
        if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
        {
            kevt = (GdkEventKey*)evt;
            kpid = (KEYPAD_KID)kevt->keyval;

            row = _find_index_matched_logOBJ(obj);
            col = _find_column_matched_logOBJ(obj, row);

            if ((kpid == KEYPAD_LEFT) && (col != LOGLABEL_TIME))
            {
                cur_focus = nfui_get_cur_focus(obj);
                if (!cur_focus) return FALSE;
                _change_obj_focus(cur_focus, log_label[row][LOGLABEL_TIME]);
            }
            else if ((kpid == KEYPAD_RIGHT) && (col != LOGLABEL_ITEM))
            {
                cur_focus = nfui_get_cur_focus(obj);
                if (!cur_focus) return FALSE;
                _change_obj_focus(cur_focus, log_label[row][LOGLABEL_ITEM]);
            }
        }
    }

    return FALSE;
}

static gboolean post_log_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkEventKey *kevt;
    KEYPAD_KID kpid = KEYPAD_NONE;

    guint i, ch_mask;
    gint result;

    if (evt->type == GDK_EXPOSE)
    {
        i = _find_index_matched_logOBJ(obj);
        if (obj->kfocus == NFOBJECT_FOCUS)
            _draw_focusLine(i);
        else
            _erase_focusLine(i);
    }

    if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;
    }

    if(evt->type == GDK_BUTTON_PRESS || evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        i = _find_index_matched_logOBJ(obj);
        result = _draw_selectLine(i);

        if (g_log_time[i].tv_sec == 0)
        {
            _playback_button_disable();
            _preview_obj_off("N/A");
            vsm_playback_preview_stop();
            return FALSE;
        }

        stm_set_time_t(g_log_time[i].tv_sec);

        if ((evt->type == GDK_2BUTTON_PRESS) || ((kpid == KEYPAD_ENTER) && (result == -1)))
            _evt_to_label_playback_handling(i);
        else
            _evt_to_label_preview_handling(i);
    }
    else if(evt->type == GDK_ENTER_NOTIFY)
    {
        NFOBJECT* cur_focus;

        cur_focus = nfui_get_cur_focus(obj);
        if (!cur_focus) return FALSE;
        _change_obj_focus(cur_focus, obj);
    }
    else if(kpid == KEYPAD_UP)
    {
        i = _find_index_matched_logOBJ(obj);

        if (i != 0)
        {
            _change_obj_focus(obj, log_label[i-1][LOGLABEL_TIME]);
            return TRUE;
        }
    }
    else if (kpid == KEYPAD_DOWN)
    {
        i = _find_index_matched_logOBJ(obj);

        if (i+1 < SBE_LOG_LABEL_ROWS)
        {
            _change_obj_focus(obj, log_label[i+1][LOGLABEL_TIME]);
            return TRUE;
        }
    }

#if 0
    else if(evt->type == GDK_MOTION_NOTIFY)
    {
        i = _find_index_matched_logOBJ(obj);
        _draw_focusLine(i);
    }
    else if(evt->type == GDK_LEAVE_NOTIFY)
    {
        _erase_focusLine();
    }
#endif

    return FALSE;
}

static gboolean post_order_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        TLOGCTX tlog_ctx;
        gint index;

        tlog_ctx = GPOINTER_TO_SIZE(nfui_nfobject_get_data(search_obj, "tlog ctx"));

        index = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));

        if (index == 0)
        {
            scm_set_tlog_filter_order(tlog_ctx, LF_LATEST);
            _search_log();
        }
        else
        {
            scm_set_tlog_filter_order(tlog_ctx, LF_OLDEST);
            _search_log();
        }
    }

    return FALSE;
}

static gboolean post_prev_log_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gboolean prev = FALSE;

    if(evt->type == GDK_BUTTON_RELEASE)
    {
        TLOG_DATA_T log_data[SBE_LOG_LABEL_ROWS];
        TLOGCTX tlog_ctx;
        gint log_cnt;

        tlog_ctx = GPOINTER_TO_SIZE(nfui_nfobject_get_data(search_obj, "tlog ctx"));

        log_cnt = scm_get_tlog_prev(tlog_ctx, SBE_LOG_LABEL_ROWS, log_data, &prev);

        _playback_button_disable();
        _erase_selectLine();
        _update_log_data(log_cnt, log_data);
        _dir_next_button_enable();

        if (!prev) _dir_prev_button_disable();
    }

    return FALSE;
}

static gboolean post_next_log_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gboolean next = FALSE;

    if(evt->type == GDK_BUTTON_RELEASE)
    {
        TLOG_DATA_T log_data[SBE_LOG_LABEL_ROWS];
        TLOGCTX tlog_ctx;
        gint log_cnt;

        tlog_ctx = GPOINTER_TO_SIZE(nfui_nfobject_get_data(search_obj, "tlog ctx"));

        log_cnt = scm_get_tlog_next(tlog_ctx, SBE_LOG_LABEL_ROWS, log_data, &next);

        _playback_button_disable();
        _erase_selectLine();
        _update_log_data(log_cnt, log_data);
        _dir_prev_button_enable();

        if (!next) _dir_next_button_disable();
    }

    return FALSE;
}

static gboolean pre_channel_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

    switch(evt->type)
    {
        case GDK_EXPOSE:
        {
            gint gap_x, gap_y;

            drawable = nfui_nfobject_get_window(obj);
            gc = gdk_gc_new(drawable);

            nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);
            nfui_nfobject_get_size(obj, &size_w, &size_h);
            pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_SUB_GROUP_BG, size_w, size_h);
            nfutil_draw_pixbuf(drawable, gc, pbuf, gap_x, gap_y, -1, -1, NFALIGN_LEFT, 0);

            nfui_nfobject_gc_unref(gc);
        }
        break;

        case GDK_DELETE:
        {
            nfui_nfobject_get_size(obj, &size_w, &size_h);
            nfui_unref_popup_pixbuf(MK_IMG_SUB_GROUP_BG, size_w, size_h);
        }
        break;

        default :

        break;
    }

    return FALSE;
}

static gboolean pre_keyword_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

    switch(evt->type)
    {
        case GDK_EXPOSE:
        {
            gint gap_x, gap_y;

            drawable = nfui_nfobject_get_window(obj);
            gc = gdk_gc_new(drawable);

            nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);
            nfui_nfobject_get_size(obj, &size_w, &size_h);
            pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_SUB_GROUP_BG, size_w, size_h);
            nfutil_draw_pixbuf(drawable, gc, pbuf, gap_x, gap_y, -1, -1, NFALIGN_LEFT, 0);

            nfui_nfobject_gc_unref(gc);
        }
        break;

        case GDK_DELETE:
        {
            nfui_nfobject_get_size(obj, &size_w, &size_h);
            nfui_unref_popup_pixbuf(MK_IMG_SUB_GROUP_BG, size_w, size_h);
        }
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
            if (g_init_sbe) g_timeout_add(300, _stop_live, 0);

            if (preview_timer_id == 0)
                preview_timer_id = g_timeout_add(500, _set_preview_time, NULL);

        }
        break;

        case GDK_DELETE:
        {
            g_init_sbe = TRUE;

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

static gboolean pre_fixed2_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint x, y;
    static GdkGC *gc;
    static GdkDrawable *drawable;

    switch(evt->type)
    {
        case GDK_EXPOSE:
        {
            if (selected_row != -1)
                _change_outLine_LogLabel(selected_row, COLOR_IDX(SELECT_OUTLINE_COLOR));
        }
        break;


        default :

        break;
    }

    return FALSE;
}

static gboolean post_playbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {
            return FALSE;
        }

        if ((selected_row < 0) || (selected_row > SBE_LOG_LABEL_ROWS))
        {
            g_warning("%s, %d", __FUNCTION__, __LINE__);
            return FALSE;
        }

        _evt_to_label_playback_handling(selected_row);
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
 :  search by text main_fixed.

      --------------------------------------------
      |  ---------------------------------------  |
      | |           |                           | |
      | |           |                           | |
      | |           |                           | |
      | |           |                           | |
      | |           |                           | |
      | |  FIXED1   |         FIXED2            | |
      | |           |                           | |
      | |           |                           | |
      | |           |                           | |
      | |           |                           | |
      | |           |                           | |
      | |           |                           | |
      |  ---------------------------------------  |
      |                                           |
      |             PARENT                        |
      |-------------------------------------------|
*/

void vw_init_SearchByText_page(NFOBJECT *parent, time_t from_time, time_t to_time)
{
    NFOBJECT *content_fixed;
    NFOBJECT *scrolled_fixed;
    NFOBJECT *fixed1;
    NFOBJECT *fixed2;

    NFOBJECT *channel_fixed;
    NFOBJECT *keyword_fixed;
    NFOBJECT *tmp_fixed;

    NFOBJECT *obj;
    NFOBJECT *ntb;

    GdkPixbuf *datetime_img[NFOBJECT_STATE_COUNT];
    GdkPixbuf *prev_img[NFOBJECT_STATE_COUNT];
    GdkPixbuf *next_img[NFOBJECT_STATE_COUNT];

    const gchar *strTitle1[] = {"FROM", "TO"};
    const gchar *strCombo[] = {"LATEST", "OLDEST"};
    const gchar *strButton[] = {"SEARCH", "PREV.", "NEXT", "PLAYBACK", "CLOSE"};

    gchar strBuf[32];

    guint tbl_width[4];
    guint size_w, size_h;
    guint pos_x, pos_y;
    guint row, col, i, ch;
    gint cnt;

    DateTimeData dtdata;
    guint tformat;

    TLOGCTX tlog_ctx;


    g_curwnd = nfui_nfobject_get_top(parent);


    focused_row = -1;
    selected_row = -1;

    memset(g_log_time, 0x00, sizeof(GTimeVal)*SBE_LOG_LABEL_ROWS);
    memset(&g_keyword_info, 0x00, sizeof(SEARCH_KEY_T));
    
// DB LOAD
// <---- DB LOAD
    memset(&dtdata, 0x00, sizeof(DateTimeData));

    DAL_get_dateTime_data(&dtdata);
    DAL_get_dateTime_format(NULL, &tformat);


// IMAGE LOAD
    datetime_img[0] = nfui_get_image_from_file((IMG_BT_DATETIME_N), NULL);
    datetime_img[1] = nfui_get_image_from_file((IMG_BT_DATETIME_O), NULL);
    datetime_img[2] = nfui_get_image_from_file((IMG_BT_DATETIME_P), NULL);
    datetime_img[3] = nfui_get_image_from_file((IMG_BT_DATETIME_D), NULL);

    prev_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_N_BTN), NULL);
    prev_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_O_BTN), NULL);
    prev_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_P_BTN), NULL);
    prev_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_D_BTN), NULL);

    next_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_N_BTN), NULL);
    next_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_O_BTN), NULL);
    next_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_P_BTN), NULL);
    next_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_D_BTN), NULL);


    content_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(content_fixed, MENU_H_INNER_W, MENU_H_INNER_H);
    nfui_nfobject_show(content_fixed);
    nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);

// FIXED1 , FIXED2
    fixed1 = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed1, SBE_FIXED1_W, SBE_FIXED1_H);
    nfui_nfobject_modify_bg(fixed1, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_show(fixed1);
    nfui_nffixed_put((NFFIXED*)content_fixed, fixed1, SBE_FIXED1_X, SBE_FIXED1_Y);

    fixed2 = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed2, SBE_FIXED2_W, SBE_FIXED2_H);
    nfui_nfobject_modify_bg(fixed2, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_show(fixed2);
    nfui_nffixed_put((NFFIXED*)content_fixed, fixed2, SBE_FIXED2_X, SBE_FIXED2_Y);


//  FROM / TO.
    nfui_get_pixbuf_size(datetime_img[0], &size_w, &size_h);

    tbl_width[0] = 80;
    tbl_width[2] = size_w;
    tbl_width[1] = 410-tbl_width[0]-tbl_width[2];

    ntb = (NFOBJECT*)nfui_nftable_new(SBE_TABLE1_COLUMNS, SBE_TABLE1_ROWS, 0, 2, tbl_width, SBE_LABEL_HEIGHT1);
    nfui_nfobject_modify_bg(ntb, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_show(ntb);
    nfui_nffixed_put((NFFIXED*)fixed1, ntb, SBE_TABLE1_X, SBE_TABLE1_Y);

    for (row = 0; row < SBE_TABLE1_ROWS; row++)
    {
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font((gchar*)(strTitle1[row]), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)ntb, obj,  0, row);

        if (row == 0)
        {
            GTimeVal time = {0, 0};
            time.tv_sec = from_time;
            from_obj = (NFOBJECT*)nfui_nftimelabel_new();
            nfui_nftimelabel_set_fg_color((NFTIMELABEL*)from_obj, COLOR_IDX(129));
            nfui_nftimelabel_set_bg_color((NFTIMELABEL*)from_obj, COLOR_IDX(128));
            nfui_nftimelabel_set_mode((NFTIMELABEL*)from_obj, prvTransDateFormat((gint)(dtdata.dateFormat)), tformat+1);
            nfui_nftimelabel_set_size((NFTIMELABEL*)from_obj, tbl_width[1], SBE_LABEL_HEIGHT1);
            nfui_nftimelabel_set_datetime((NFTIMELABEL*)from_obj, &time);
            nfui_nfobject_use_focus(from_obj, NFOBJECT_FOCUS_OFF);
            nfui_nfobject_show(from_obj);
            nfui_nftable_attach((NFTABLE*)ntb, from_obj,  1, row);
        }
        else
        {
            GTimeVal time = {0, 0};
            time.tv_sec = to_time;
            to_obj = (NFOBJECT*)nfui_nftimelabel_new();
            nfui_nftimelabel_set_fg_color((NFTIMELABEL*)to_obj, COLOR_IDX(129));
            nfui_nftimelabel_set_bg_color((NFTIMELABEL*)to_obj, COLOR_IDX(128));
            nfui_nftimelabel_set_mode((NFTIMELABEL*)to_obj, prvTransDateFormat((gint)(dtdata.dateFormat)), tformat+1);
            nfui_nftimelabel_set_size((NFTIMELABEL*)to_obj, tbl_width[1], SBE_LABEL_HEIGHT1);
            nfui_nftimelabel_set_datetime((NFTIMELABEL*)to_obj, &time);
            nfui_nfobject_use_focus(to_obj, NFOBJECT_FOCUS_OFF);
            nfui_nfobject_show(to_obj);
            nfui_nftable_attach((NFTABLE*)ntb, to_obj,  1, row);
        }

        obj = (NFOBJECT*)nfui_nfbutton_new();
        nfui_nfbutton_set_image(NF_BUTTON(obj), datetime_img);
        nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)ntb, obj,  2, row);

        if (row == 0)
        {
            nfui_regi_post_event_callback(obj, post_from_event_handler);
            from_btn_obj = obj;
        }
        else
        {
            nfui_regi_post_event_callback(obj, post_to_event_handler);
            to_btn_obj = obj;
        }

    }

//  CHANNEL.
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CHANNEL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(obj, 180, SBE_LABEL_HEIGHT2);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed1, obj, SBE_EVENT_CHANNEL_LABEL_X, SBE_EVENT_CHANNEL_LABEL_Y);

    channel_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(channel_fixed, SBE_EVENT_CHANNEL_FIXED_W, SBE_EVENT_CHANNEL_FIXED_H);
    nfui_nfobject_modify_bg(channel_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(194));
    nfui_nfobject_show(channel_fixed);
    nfui_nffixed_put((NFFIXED*)fixed1, channel_fixed, SBE_EVENT_CHANNEL_FIXED_X, SBE_EVENT_CHANNEL_FIXED_Y);

    obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
    nfui_check_get_size(obj, &size_w, &size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)channel_fixed, obj, 10+2, 10+(38-size_h)/2);
    nfui_regi_post_event_callback(obj, post_all_ch_event_handler);
    g_all_ch_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ALL", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(122));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(194));
    nfui_nfobject_set_size(obj, 120, 38);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)channel_fixed, obj, 10+2+size_w+2, 10);

    scrolled_fixed = (NFOBJECT*)nfui_nfscrolledfixed_new(NFSCROLLED_POLICY_NEVER, NFSCROLLED_POLICY_AUTOMATIC);
    nfui_nfscrolledfixed_set_skin_type((NFSCROLLEDFIXED*)scrolled_fixed, NFSCROLLEDFIXED_TYPE_1);
    nfui_nfscrolledfixed_set_vscroll_speed((NFSCROLLEDFIXED*)scrolled_fixed, 40, 0, 0);
    nfui_nfscrolledfixed_set_vscroll_offset((NFSCROLLEDFIXED*)scrolled_fixed, 0);
    nfui_nfobject_modify_bg(scrolled_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(194));
    nfui_nfobject_set_size(scrolled_fixed, SBE_EVENT_CHANNEL_FIXED_W - 20, 3 * 40);
    nfui_nfobject_show(scrolled_fixed);
    nfui_nffixed_put((NFFIXED*)channel_fixed, scrolled_fixed, 10+2, 5 + 38 + 2);

    pos_y = 0;
    cnt = 0;
    
    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        tmp_fixed = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_show(tmp_fixed);
        nfui_nfobject_modify_bg(tmp_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(194));
        nfui_nfobject_set_size(tmp_fixed, SBE_EVENT_CHANNEL_FIXED_W/4-4, 40);
        nfui_nfscrolledfixed_put((NFSCROLLEDFIXED*)scrolled_fixed, tmp_fixed, (((SBE_EVENT_CHANNEL_FIXED_W/4)+15) * cnt), pos_y);

        obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
        nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
        nfui_check_get_size(obj, &size_w, &size_h);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)tmp_fixed, obj, 0, (tmp_fixed->height-size_h)/2);
        nfui_regi_post_event_callback(obj, post_ch_event_handler);
        g_ch_obj[i] = obj;

        memset(strBuf, 0x00, sizeof(strBuf));
        g_sprintf(strBuf, "CH%02d", i+1);

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(122));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(194));
        nfui_nfobject_set_size(obj, tmp_fixed->width-size_w-6, 38);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)tmp_fixed, obj, size_w+2, 0);

        cnt++;
        if (cnt == 3) {
            pos_y += 40;
            cnt = 0;
        }
    }

// ITEM KEYWORD.
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ITEM KEYWORD", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(193));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(obj, 180, SBE_LABEL_HEIGHT2);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed1, obj, SBE_EVENT_KEYWORD_LABEL_X, SBE_EVENT_KEYWORD_LABEL_Y);

    keyword_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(keyword_fixed, SBE_EVENT_KEYWORD_FIXED_W, SBE_EVENT_KEYWORD_FIXED_H);
    nfui_nfobject_modify_bg(keyword_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(194));
    nfui_nfobject_show(keyword_fixed);
    nfui_nffixed_put((NFFIXED*)fixed1, keyword_fixed, SBE_EVENT_KEYWORD_FIXED_X, SBE_EVENT_KEYWORD_FIXED_Y);

    pos_x = 20;
    pos_y = 10;

    obj = nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 8);
    nfui_nfobject_support_multi_lang(obj, FALSE);
    nfui_nfobject_set_size(obj, SBE_EVENT_KEYWORD_FIXED_W-40, 40);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_ON);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)keyword_fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_input_keyword_event_handler);
    g_keyword_obj = obj;

    pos_y += 44;

    obj = (NFOBJECT*)nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
    nfui_check_get_size(obj, &size_w, &size_h);
    nfui_nfobject_disable(obj);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)keyword_fixed, obj, pos_x, pos_y + (38-size_h)/2);
    nfui_regi_post_event_callback(obj, post_match_case_event_handler);
    g_match_case_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("MATCH CASE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(122));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(194));
    nfui_nfobject_set_size(obj, SBE_EVENT_KEYWORD_FIXED_W-80, 38);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)keyword_fixed, obj, pos_x+40, pos_y);

    pos_y += 39;

    obj = (NFOBJECT*)nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
    nfui_check_get_size(obj, &size_w, &size_h);
    nfui_nfobject_disable(obj);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)keyword_fixed, obj, pos_x, pos_y + (38-size_h)/2);
    nfui_regi_post_event_callback(obj, post_match_whole_event_handler);
    g_match_whole_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("MATCH WHOLE WORD ONLY", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(122));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(194));
    nfui_nfobject_set_size(obj, SBE_EVENT_KEYWORD_FIXED_W-80, 38);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)keyword_fixed, obj, pos_x+40, pos_y);


// SEARCH BUTTON
    search_obj = nftool_normal_button_create_type3(strButton[0], 203);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(search_obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(search_obj);
    nfui_nffixed_put((NFFIXED*)fixed1, search_obj, SBE_SEARCH_BUTTON_X, SBE_SEARCH_BUTTON_Y);
    nfui_regi_post_event_callback(search_obj, post_search_event_handler);

    tlog_ctx = scm_open_tlog_ctx();
    nfui_nfobject_set_data(search_obj, "tlog ctx", GSIZE_TO_POINTER(tlog_ctx));


//  PREVIEW
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PREVIEW", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(662));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(obj, SBE_PREVIEW_LABEL_WIDTH, SBE_LABEL_HEIGHT2);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed1, obj, SBE_PREVIEW_LABEL_X, SBE_PREVIEW_LABEL_Y);

    /** TEMP **/
    preview_obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(665));
    nfui_nflabel_set_align((NFLABEL*)preview_obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(preview_obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(666));
    nfui_nfobject_set_size(preview_obj, SBE_PREVIEW_VIDEO_W, SBE_PREVIEW_VIDEO_H);
    nfui_nfobject_use_focus(preview_obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(preview_obj);
    nfui_nffixed_put((NFFIXED*)fixed1, preview_obj, SBE_PREVIEW_VIDEO_X, SBE_PREVIEW_VIDEO_Y);

    /** TEMP **/
    play_time_obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MINI_SEMI_5), COLOR_IDX(664));
    nfui_nflabel_set_align((NFLABEL*)play_time_obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(play_time_obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(play_time_obj, SBE_PREVIEW_VIDEO_W, SBE_LABEL_HEIGHT2);
    nfui_nfobject_use_focus(play_time_obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(play_time_obj);
    nfui_nffixed_put((NFFIXED*)fixed1, play_time_obj, SBE_PREVIEW_TIME_X, SBE_PREVIEW_TIME_Y);

//  LOG DATA
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("TIME", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_set_size(obj, SBE_LOG_TIME_W, SBE_LOGLABEL_HEIGHT);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed2, obj, SBE_LOG_TIME_X, SBE_LOG_Y);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CHANNEL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_set_size(obj, SBE_LOG_CHANNEL_W, SBE_LOGLABEL_HEIGHT);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed2, obj, SBE_LOG_CHANNEL_X, SBE_LOG_Y);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ITEM", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_set_size(obj, SBE_LOG_ITEM_W, SBE_LOGLABEL_HEIGHT);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed2, obj, SBE_LOG_ITEM_X, SBE_LOG_Y);

    for(i=0; i<SBE_LOG_LABEL_ROWS; i++)
    {
        obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
        nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
        nfui_nflabel_set_drawing_outline((NFLABEL*)obj, FALSE);
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
        nfui_nfobject_set_size(obj, SBE_LOG_TIME_W, SBE_LOGLABEL_HEIGHT);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed2, obj, SBE_LOG_TIME_X, SBE_LOG_Y+((i+1)*(SBE_LOGLABEL_HEIGHT+SBE_LOG_HEIGHT_GAP)));
        nfui_regi_pre_event_callback(obj, pre_log_label_event_handler);
        nfui_regi_post_event_callback(obj, post_log_label_event_handler);
        log_label[i][LOGLABEL_TIME] = obj;

        obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_THIN));
        nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
        nfui_nfobject_support_multi_lang(obj, FALSE);
        nfui_nflabel_set_drawing_outline((NFLABEL*)obj, FALSE);
        nfui_nfobject_set_size(obj, SBE_LOG_CHANNEL_W, SBE_LOGLABEL_HEIGHT);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed2, obj, SBE_LOG_CHANNEL_X, SBE_LOG_Y+((i+1)*(SBE_LOGLABEL_HEIGHT+SBE_LOG_HEIGHT_GAP)));
        nfui_regi_pre_event_callback(obj, pre_log_label_event_handler);
        nfui_regi_post_event_callback(obj, post_log_label_event_handler);
        log_label[i][LOGLABEL_CHANNEL] = obj;

        obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_THIN));
        nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
        nfui_nfobject_support_multi_lang(obj, FALSE);
        nfui_nflabel_set_drawing_outline((NFLABEL*)obj, FALSE);
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
        nfui_nfobject_set_size(obj, SBE_LOG_ITEM_W, SBE_LOGLABEL_HEIGHT);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed2, obj, SBE_LOG_ITEM_X, SBE_LOG_Y+((i+1)*(SBE_LOGLABEL_HEIGHT+SBE_LOG_HEIGHT_GAP)));
        nfui_regi_pre_event_callback(obj, pre_log_label_event_handler);
        nfui_regi_post_event_callback(obj, post_log_label_event_handler);
        log_label[i][LOGLABEL_ITEM] = obj;
    }

//  LOG ORDER
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ORDER BY", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(obj, SBE_LOG_ORDER_W, SBE_LABEL_HEIGHT1);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed2, obj, SBE_LOG_ORDER_X, SBE_LOG_ORDER_Y);

    obj = nfui_combobox_new(strCombo, 2, 0);
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
    nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, SBE_LOG_ORDER_CMB_W, SBE_LABEL_HEIGHT1);
    nfui_nfobject_disable(obj);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed2, obj, SBE_LOG_ORDER_X+SBE_LOG_ORDER_W, SBE_LOG_ORDER_Y);
    nfui_regi_post_event_callback(obj, post_order_event_handler);
    order_obj = obj;

//  LOG DIR
    nfui_get_pixbuf_size(prev_img[0], &size_w, &size_h);

    obj = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(obj), prev_img);
    nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
    nfui_nfobject_disable(obj);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed2, obj, SBE_LOG_PREV_BUTTON_X, SBE_LOG_PREV_BUTTON_Y);
    nfui_regi_post_event_callback(obj, post_prev_log_event_handler);
    prev_obj = obj;

    nfui_get_pixbuf_size(next_img[0], &size_w, &size_h);

    obj = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(obj), next_img);
    nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
    nfui_nfobject_disable(obj);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed2, obj, SBE_LOG_NEXT_BUTTON_X, SBE_LOG_NEXT_BUTTON_Y);
    nfui_regi_post_event_callback(obj, post_next_log_event_handler);
    next_obj = obj;

// PLAYBACK / CLOSE BUTTON
    obj = nftool_normal_button_create_type1(strButton[3], MENU_BTN_WIDTH);
    nfui_nfobject_disable(obj);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_H_BTN_R2_X, MENU_H_BTN_Y);
    nfui_regi_post_event_callback(obj, post_playbutton_event_handler);
    playback_obj = obj;

    obj = nftool_normal_button_create_type2(strButton[4], MENU_BTN_WIDTH);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_H_BTN_R1_X, MENU_H_BTN_Y);
    nfui_nfobject_show(obj);
    nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

    nfui_regi_pre_event_callback(channel_fixed, pre_channel_fixed_event_handler);
    nfui_regi_pre_event_callback(keyword_fixed, pre_keyword_fixed_event_handler);
    nfui_regi_pre_event_callback(fixed1, pre_fixed1_event_handler);
    nfui_regi_pre_event_callback(fixed2, pre_fixed2_event_handler);

    nfui_regi_post_event_callback(parent, post_page_event_handler);
}

gboolean vw_SearchByText_tab_out_handler()
{
    _preview_obj_off("");
    vsm_playback_preview_stop();

    return FALSE;
}

gboolean vw_SearchByText_tab_in_handler()
{
    g_init_sbe = TRUE;
    return FALSE;
}

gboolean vw_SearchByText_tab_show()
{
    vw_SearchByText_tab_in_handler();
    return FALSE;
}


gboolean vw_SearchByText_set_interval(time_t from_time, time_t to_time)
{
    GTimeVal tmp;
    memset(&tmp, 0x00, sizeof(GTimeVal));

    tmp.tv_sec = from_time;
    nfui_nftimelabel_set_datetime((NFTIMELABEL*)from_obj, &tmp);

    tmp.tv_sec = to_time;
    nfui_nftimelabel_set_datetime((NFTIMELABEL*)to_obj, &tmp);

    return FALSE;
}

