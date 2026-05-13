#include "nf_afx.h"


#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"

#include "modules/ssm.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nftable.h"
#include "viewers/objects/nfbutton.h"

#include "scm.h"
#include "vsm.h"
#include "dtf.h"

#include "vw_live_event_log.h"
#include "vw_live_statusbar.h"
#include "vw_timeline.h"
#include "vw_search_main.h"
#include "vw_playback_main.h"
#include "vw_internal.h"


#define EVT_SIZE_W                      (890)
#define EVT_SIZE_H                      (537)

#define ROW_COUNT                       (14)

enum {
    TIME_COLUMN = 0,
    TITLE_COLUMN,
    LOG_COLUMN,
    COL_COUNT
};

enum {
    PREV_LIVE_LOG = 0,
    CUR_LIVE_LOG,
    NEXT_LIVE_LOG
};

static LOGCTX g_lctx = 0;


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_evt_win = NULL;
static NFOBJECT *g_loglb[COL_COUNT][ROW_COUNT];
static NFOBJECT *g_lBtn;
static NFOBJECT *g_rBtn;

static glong g_logTime[ROW_COUNT];
static gint g_logCh[ROW_COUNT];
static gint kf = -1;



static void change_log_focus(gint row, gboolean expose)
{
    static gint pressed_row = -1;
    //g_message("%s :::::::::pressed row: %d , row : %d :::::::::::::::::", __FUNCTION__, pressed_row, row);

    if(pressed_row == row)
        return;

    // old
    if(pressed_row >= 0) {
        nfui_nflabel_set_pango_font((NFLABEL*)g_loglb[TIME_COLUMN][pressed_row], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(389));
        nfui_nflabel_set_pango_font((NFLABEL*)g_loglb[TITLE_COLUMN][pressed_row], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(389));
        nfui_nflabel_set_pango_font((NFLABEL*)g_loglb[LOG_COLUMN][pressed_row], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(389));

        nfui_nfobject_modify_bg(g_loglb[TIME_COLUMN][pressed_row], NFOBJECT_STATE_NORMAL, COLOR_IDX(388));
        nfui_nfobject_modify_bg(g_loglb[TITLE_COLUMN][pressed_row], NFOBJECT_STATE_NORMAL, COLOR_IDX(388));
        nfui_nfobject_modify_bg(g_loglb[LOG_COLUMN][pressed_row], NFOBJECT_STATE_NORMAL, COLOR_IDX(388));

        if(expose) {
            nfui_signal_emit(g_loglb[TIME_COLUMN][pressed_row], GDK_EXPOSE, FALSE); 
            nfui_signal_emit(g_loglb[TITLE_COLUMN][pressed_row], GDK_EXPOSE, FALSE);    
            nfui_signal_emit(g_loglb[LOG_COLUMN][pressed_row], GDK_EXPOSE, FALSE);  
        }
    }

    // new
    if(row >= 0) {
        nfui_nflabel_set_pango_font((NFLABEL*)g_loglb[TIME_COLUMN][row], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(391));
        nfui_nflabel_set_pango_font((NFLABEL*)g_loglb[TITLE_COLUMN][row], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(391));
        nfui_nflabel_set_pango_font((NFLABEL*)g_loglb[LOG_COLUMN][row], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(391));

        nfui_nfobject_modify_bg(g_loglb[TIME_COLUMN][row], NFOBJECT_STATE_NORMAL, COLOR_IDX(390));
        nfui_nfobject_modify_bg(g_loglb[TITLE_COLUMN][row], NFOBJECT_STATE_NORMAL, COLOR_IDX(390));
        nfui_nfobject_modify_bg(g_loglb[LOG_COLUMN][row], NFOBJECT_STATE_NORMAL, COLOR_IDX(390));

        if(expose) {
            nfui_signal_emit(g_loglb[TIME_COLUMN][row], GDK_EXPOSE, FALSE); 
            nfui_signal_emit(g_loglb[TITLE_COLUMN][row], GDK_EXPOSE, FALSE);    
            nfui_signal_emit(g_loglb[LOG_COLUMN][row], GDK_EXPOSE, FALSE);  
        }
    }

    pressed_row = row;
}

static void set_log_time(guint row, glong sec)
{
    g_logTime[row] = sec;
}

static glong get_log_time(guint row)
{
    return g_logTime[row];
}

static void set_log_ch(guint row, gint ch)
{
    g_logCh[row] = ch;
}

static gint get_log_ch(guint row)
{
    return g_logCh[row];
}

static gint get_live_log(gint dir, LOG_DATA_T *log, gboolean *enable)
{
    gint log_cnt = 0;

    switch(dir) 
    {
        case PREV_LIVE_LOG:
        {
            log_cnt = scm_get_log_prev(g_lctx, ROW_COUNT, log, enable);
        }
        break;

        case CUR_LIVE_LOG:
        {
            GTimeVal f_time;
            GTimeVal t_time;

            g_get_current_time(&t_time);
            t_time.tv_sec -= 10;    // 10 sec
            ifn_add_hour_b(&t_time, -1, &f_time);       // from : 1 hour ago, to : now - 10sec, 10sec is just buffer (avoid writing delay)
            log_cnt = scm_get_log(g_lctx, &f_time, &t_time, ROW_COUNT, log, enable);
        }
        break;

        case NEXT_LIVE_LOG:
        {
            log_cnt = scm_get_log_next(g_lctx, ROW_COUNT, log, enable);
        }
        break;
    }

    return log_cnt;
}

static void update_live_log(LOG_DATA_T *log, gint log_count, gboolean expose)
{
    struct tm st_time;

    gchar strTime[64];
    gchar strTitle[STRING_SIZE_CAMTITLE];
    gchar strMsg[128];
    gint i;
    LPR_CHANNEL_T *pc;

    for(i=0; i<ROW_COUNT; i++) {
        if(i >= log_count) {
            nfui_nflabel_set_text((NFLABEL*)(g_loglb[TIME_COLUMN][i]), " ");
            nfui_nflabel_set_text((NFLABEL*)(g_loglb[TITLE_COLUMN][i]), " ");
            nfui_nflabel_set_text((NFLABEL*)(g_loglb[LOG_COLUMN][i]), " ");

            set_log_time(i, 0);
            set_log_ch(i, -1);
            continue;
        }

        pc = (LPR_CHANNEL_T *)&log[i].p;

        // time
        if(log[i].tvTime.tv_sec == 0 
            || pc->channel > GUI_CHANNEL_CNT
            || pc->channel < 0) {
            set_log_time(i, 0);
            set_log_ch(i, -1);
            continue;
        }

        set_log_time(i, log[i].tvTime.tv_sec);
        dtf_get_local_datetime(log[i].tvTime.tv_sec, strTime);
        nfui_nflabel_set_text((NFLABEL*)(g_loglb[TIME_COLUMN][i]), strTime);


        // title
        if(pc->channel >= GUI_CHANNEL_CNT) set_log_ch(i, -1);
        else                                  set_log_ch(i, pc->channel);

        if(pc->channel < GUI_CHANNEL_CNT)
        {
            var_get_camtitle(strTitle, pc->channel);
            nfui_nflabel_set_text((NFLABEL*)(g_loglb[TITLE_COLUMN][i]), strTitle);
        }

        // log message
        if(log[i].type == LT_RECORD_STARTED) 
            sprintf(strMsg, g_recstart_logstr[pc->klass], pc->channel + 1);
        else
            sprintf(strMsg, g_recstop_logstr[pc->klass], pc->channel + 1);

        if(g_loglb[LOG_COLUMN][i]) 
            nfui_nflabel_set_text((NFLABEL*)(g_loglb[LOG_COLUMN][i]), strMsg);
    }

    if(expose) {
        for(i=0; i<ROW_COUNT; i++) {
            nfui_signal_emit(g_loglb[TIME_COLUMN][i], GDK_EXPOSE, FALSE);   
            nfui_signal_emit(g_loglb[TITLE_COLUMN][i], GDK_EXPOSE, FALSE);  
            nfui_signal_emit(g_loglb[LOG_COLUMN][i], GDK_EXPOSE, FALSE);    
        }
    }
}

static gboolean post_log_label_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    switch(evt->type) {
        case GDK_BUTTON_PRESS:
            {
                kf = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "row index"));
                change_log_focus(kf, TRUE);
            }
            break;

        case GDK_2BUTTON_PRESS:
            {
                GTimeVal tv;
                guint ch_mask = 0;
                SecurityData secdata;
                gboolean use_dl = 0;

                if(evt->button.button == MOUSE_RIGTH_BUTTON) 
                    return FALSE;

                kf = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "row index"));

                memset(&tv, 0x00, sizeof(GTimeVal));
                tv.tv_sec = get_log_time(kf);

                if(tv.tv_sec > 0) 
                {
                if(!ssm_check_access_auth(USR_AUTH_SEARCH))
                    break;
                    
                DAL_get_use_double_login(&use_dl);

                if (use_dl && !ssm_is_admin())
                {
                    if (!VW_UserPwd_Open(NF_TOPWND, "DOUBLE LOGIN", -1)) return FALSE;
                    if (!VW_UserPwd_Open(NF_TOPWND, "DOUBLE LOGIN2", USR_AUTH_SEARCH)) return FALSE;
                }
                else
                {
                    DAL_get_security_data(&secdata);
                    if (secdata.loginSearchArch && !VW_UserPwd_Open(g_curwnd, "PASSWORD CHECK", -1)) return FALSE;
                }

                    if(get_log_ch(kf) >= 0)
                        ch_mask |= (1 << get_log_ch(kf));

                    VW_Live_StatusBar_Hide();                                   
                    nfui_nfobject_hide(g_evt_win);
                    nfui_page_close(PGID_LIVE_EVENT_LOG, g_evt_win);

                    vsm_live_stop();
                    vw_playback_open(NF_TOPWND, vsm_create_livestart_obj(), OPEN_BY_LIVE_LOG);
                    vsm_playback_start(ch_mask, tv, PLAYBACK_NORMAL);
                }
            }
            break;

        default:
            break;
    }

    return FALSE;
}

static gint update_next_log_data()
{
    LOG_DATA_T log_data[ROW_COUNT];
    gint log_count = 0;
    gboolean enable = FALSE;

    nfui_nfobject_disable(g_lBtn);
    nfui_nfobject_enable(g_rBtn);

    memset(log_data, 0x00, sizeof(LOG_DATA_T));
    log_count = get_live_log(PREV_LIVE_LOG, log_data, &enable);
    update_live_log(log_data, log_count, TRUE);
    if (enable) nfui_nfobject_enable(g_lBtn);

    nfui_signal_emit(g_lBtn, GDK_EXPOSE, FALSE);    
    nfui_signal_emit(g_rBtn, GDK_EXPOSE, FALSE);

    return log_count;
}

static gboolean post_left_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_PRESS) 
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        change_log_focus(-1, TRUE);
        update_next_log_data();
    }

    return FALSE;
}

static gint update_pre_log_data()
{
    LOG_DATA_T log_data[ROW_COUNT];
    gint log_count = 0;
    gboolean enable = FALSE;

    nfui_nfobject_enable(g_lBtn);
    nfui_nfobject_disable(g_rBtn);

    memset(log_data, 0x00, sizeof(LOG_DATA_T));
    log_count = get_live_log(NEXT_LIVE_LOG, log_data, &enable); 
    update_live_log(log_data, log_count, TRUE);
    if (enable) nfui_nfobject_enable(g_rBtn);

    nfui_signal_emit(g_lBtn, GDK_EXPOSE, FALSE);    
    nfui_signal_emit(g_rBtn, GDK_EXPOSE, FALSE);

    return log_count;
}

static gboolean post_right_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_PRESS) 
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        change_log_focus(-1, TRUE);
        update_pre_log_data();
    }

    return FALSE;
}

static gboolean post_evt_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

    if(evt->type == GDK_EXPOSE) {
        drawable = nfui_nfobject_get_window(obj);
        gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
    
        nfui_nfobject_gc_unref(gc);
    }
    else if (evt->type == GDK_DELETE)
    {
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);   
    }

    return FALSE;
}


static gboolean post_evt_window_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    switch(evt->type) {
        case NFOUTEVT_BUTTON_PRESS:
            {
                nfui_nfobject_hide(obj);
                nfui_page_close(PGID_LIVE_EVENT_LOG, obj);
            }
            break;

        case NFEVENT_KEYPAD_PRESS:
        case NFEVENT_REMOCON_PRESS:
            {
                GdkEventKey *kevt;
                KEYPAD_KID kpid;

                kevt = (GdkEventKey*)evt;
                kpid = (KEYPAD_KID)kevt->keyval;

                switch(kpid) {
                    case KEYPAD_LEFT:
                        {
                            if (nfui_nfobject_is_disabled(g_lBtn)) return FALSE;

                            kf = -1;
                            change_log_focus(kf, TRUE);
                            update_next_log_data();
                        }
                        return TRUE;

                    case KEYPAD_RIGHT:
                        {
                            if (nfui_nfobject_is_disabled(g_rBtn)) return FALSE;

                            kf = -1;
                            change_log_focus(kf, TRUE);
                            update_pre_log_data();
                        }
                        return TRUE;

                    case KEYPAD_UP:
                        {
                            if(kf > 0) {
                                change_log_focus(--kf, TRUE);
                            }else if(kf < 0) {
                                change_log_focus(++kf, TRUE);
                            }else if(kf == 0) {
                                kf = ROW_COUNT - 1;

                                change_log_focus(kf, TRUE);
                            }
                        }
                        return TRUE;

                    case KEYPAD_DOWN:
                        {
                            if(kf < 0 || kf < ROW_COUNT - 1) 
                            {
                                change_log_focus(++kf, TRUE);
                            }
                            else if(kf == ROW_COUNT - 1)
                            {
                                kf = 0;

                                change_log_focus(kf, TRUE);
                            }
                        }
                        return TRUE;

                    case KEYPAD_ENTER:
                        {
                            GTimeVal tv;
                            guint ch_mask = 0;
                            SecurityData secdata;
                            gboolean use_dl = 0;

                            if (kf == -1) break;

                            if(kf >= 0 && kf < ROW_COUNT - 1) 
                            {
                                memset(&tv, 0x00, sizeof(GTimeVal));
                                tv.tv_sec = get_log_time(kf);

                                if(tv.tv_sec > 0) 
                                {
                            if(!ssm_check_access_auth(USR_AUTH_SEARCH))
                                break;
                                
                            DAL_get_use_double_login(&use_dl);

                            if (use_dl && !ssm_is_admin())
                            {
                                if (!VW_UserPwd_Open(NF_TOPWND, "DOUBLE LOGIN", -1)) return FALSE;
                                if (!VW_UserPwd_Open(NF_TOPWND, "DOUBLE LOGIN2", USR_AUTH_SEARCH)) return FALSE;
                            }
                            else
                            {
                                DAL_get_security_data(&secdata);
                                if (secdata.loginSearchArch && !VW_UserPwd_Open(g_curwnd, "PASSWORD CHECK", -1)) return FALSE;
                            }

                                    if(get_log_ch(kf) >= 0)
                                        ch_mask |= (1 << get_log_ch(kf));

                                    VW_Live_StatusBar_Hide();                                   
                                    nfui_nfobject_hide(g_evt_win);
                                    nfui_page_close(PGID_LIVE_EVENT_LOG, g_evt_win);

                                    vsm_live_stop();
                                    vw_playback_open(NF_TOPWND, vsm_create_livestart_obj(), OPEN_BY_LIVE_LOG);
                                    vsm_playback_start(ch_mask, tv, PLAYBACK_NORMAL);

                                    kf = -1;
                                }
                            }
                        }
                        return TRUE;

                    case KEYPAD_EXIT:
                        {
                            kf = -1;

                            nfui_nfobject_hide(obj);
                            nfui_page_close(PGID_LIVE_EVENT_LOG, obj);
                        }
                        return TRUE;

                    default:
                        break;
                }
            }
            break;

        case GDK_DELETE:
            {
                kf = -1;

                nfui_nfobject_hide(obj);
                nfui_page_close(PGID_LIVE_EVENT_LOG, obj);

                g_curwnd = 0;
            }
            break;

        default:
            break;
    }

    return FALSE;
}

static gboolean returnkey_proc(NFOBJECT *top, GdkEvent *event, gpointer data)
{
    return FALSE;
}

gboolean VW_Create_EventLog(NFWINDOW *parent, gint x, gint y)
{
    NFOBJECT *evt_fixed = NULL;
    NFOBJECT *tbl = NULL;
    NFOBJECT *obj = NULL;

    guint tbl_w[3] = {260, 160, 450};
    gint i, j;
    guint ch_mask = 0;

    GdkPixbuf *left_arrow[NFOBJECT_STATE_COUNT];
    GdkPixbuf *right_arrow[NFOBJECT_STATE_COUNT];

    left_arrow[0] = nfui_get_image_from_file((IMG_N_POP_ARROW_LEFT), NULL);
    left_arrow[1] = nfui_get_image_from_file((IMG_O_POP_ARROW_LEFT), NULL);
    left_arrow[2] = nfui_get_image_from_file((IMG_P_POP_ARROW_LEFT), NULL);
    left_arrow[3] = nfui_get_image_from_file((IMG_D_POP_ARROW_LEFT), NULL);

    right_arrow[0] = nfui_get_image_from_file((IMG_N_POP_ARROW_RIGHT), NULL);
    right_arrow[1] = nfui_get_image_from_file((IMG_O_POP_ARROW_RIGHT), NULL);
    right_arrow[2] = nfui_get_image_from_file((IMG_P_POP_ARROW_RIGHT), NULL);
    right_arrow[3] = nfui_get_image_from_file((IMG_D_POP_ARROW_RIGHT), NULL);

    /* window */
    g_evt_win = (NFOBJECT*)nfui_nfwindow_new(parent, x, y, EVT_SIZE_W, EVT_SIZE_H);
    g_curwnd = g_evt_win;
    nfui_regi_post_event_callback(g_evt_win, post_evt_window_event_cb);
    nfui_nfwindow_use_outside_evt((NFWINDOW*)g_evt_win, TRUE);
    nfui_nfwindow_set_mask((NFWINDOW*)g_evt_win, GDK_BUTTON_PRESS, TRUE);
    nfui_nfwindow_set_returnkey_proc((NFWINDOW*)g_evt_win, returnkey_proc);
    
    gtk_widget_add_events(((NFWINDOW*)g_evt_win)->main_widget, GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_HINT_MASK);

    /* fixed */
    evt_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(evt_fixed, EVT_SIZE_W, EVT_SIZE_H);
    nfui_regi_post_event_callback(evt_fixed, post_evt_fixed_event_cb);
    nfui_nfobject_show(evt_fixed);
    

    /* title */
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("EVENT LOG", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
    nfui_nfobject_set_size(obj, 400, 36);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 24);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)evt_fixed, obj, 4, 4);

    /* table */
    //tbl = (NFOBJECT*)nfui_nftable_new(COL_COUNT, ROW_COUNT, 10, 1, tbl_w, 29);    
    tbl = (NFOBJECT*)nfui_nftable_new(COL_COUNT, ROW_COUNT, 1, 1, tbl_w, 29);   
    nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nftable_set_draw_outline((NFTABLE*)tbl, TRUE);
    nfui_nfobject_show(tbl);
    nfui_nffixed_put((NFFIXED*)evt_fixed, tbl, 14, 54);

    /* log label */
    for(i=0; i<COL_COUNT; i++) {
        for(j=0; j<ROW_COUNT; j++) {
            g_loglb[i][j] = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(389));
            if(i == 0) nfui_nflabel_set_align((NFLABEL*)g_loglb[i][j], NFALIGN_LEFT, 4);
            else       nfui_nflabel_set_align((NFLABEL*)g_loglb[i][j], NFALIGN_LEFT, 10);
            nfui_nflabel_set_drawing_outline((NFLABEL*)g_loglb[i][j], FALSE);
            nfui_nfobject_modify_bg(g_loglb[i][j], NFOBJECT_STATE_NORMAL, COLOR_IDX(388));
            nfui_nfobject_set_size(g_loglb[i][j], tbl_w[i], 29);
            nfui_nfobject_set_data(g_loglb[i][j], "row index", GINT_TO_POINTER(j));
            nfui_regi_post_event_callback(g_loglb[i][j], post_log_label_event_cb);
            nfui_nfobject_show(g_loglb[i][j]);
            nfui_nftable_attach((NFTABLE*)tbl, g_loglb[i][j], i, j);

            if (i == TITLE_COLUMN) {
                nfui_nfobject_support_multi_lang(g_loglb[i][j], FALSE);
            }
        }
    }

    /* button */
    g_lBtn = nfui_nfbutton_new();
    nfui_nfbutton_set_image((NFBUTTON*)g_lBtn, left_arrow);
    nfui_nfobject_set_size(g_lBtn, 64, 34);
    nfui_regi_post_event_callback(g_lBtn, post_left_button_event_cb);
    nfui_nfobject_show(g_lBtn);
    nfui_nffixed_put((NFFIXED*)evt_fixed, g_lBtn, EVT_SIZE_W/2-66, 484);

    g_rBtn = nfui_nfbutton_new();
    nfui_nfbutton_set_image((NFBUTTON*)g_rBtn, right_arrow);
    nfui_nfobject_set_size(g_rBtn, 64, 34);
    nfui_regi_post_event_callback(g_rBtn, post_right_button_event_cb);
    nfui_nfobject_show(g_rBtn);
    nfui_nffixed_put((NFFIXED*)evt_fixed, g_rBtn, EVT_SIZE_W/2+2, 484);

    nfui_nfwindow_add((NFWINDOW*)g_evt_win, evt_fixed);
    nfui_run_main_event_handler(g_evt_win);
    nfui_nfobject_show(g_evt_win);
    nfui_make_key_hierarchy((NFWINDOW*)g_evt_win);

    nfui_nfobject_hide(g_evt_win);

    for (i = 0; i < var_get_ch_count(); ++i)
        ch_mask |= (1 << i);

    g_lctx = scm_open_log_ctx();
    scm_set_log_filter_type(g_lctx, ch_mask, LF_CAT_ALL, 0);
    scm_set_log_filter_type(g_lctx, ch_mask, LF_CAT_RECORD, 1);

    return TRUE;
}

int VW_Destroy_EventLog()
{
    if (!g_curwnd) return 0;
    nfui_nfobject_destroy(g_curwnd);
    return 0;
}

void VW_EventLog_Show()
{
    LOG_DATA_T log_data[ROW_COUNT];
    gint log_count = 0;
    gboolean enable = FALSE;

    change_log_focus(-1, FALSE);

    memset(log_data, 0x00, sizeof(LOG_DATA_T));
    log_count = get_live_log(CUR_LIVE_LOG, log_data, &enable);
    
    nfui_nfobject_disable(g_lBtn);
    nfui_nfobject_disable(g_rBtn);

    update_live_log(log_data, log_count, FALSE);        
    if (enable) nfui_nfobject_enable(g_rBtn);

    nfui_nfobject_show(g_evt_win);
    nfui_page_open(PGID_LIVE_EVENT_LOG, g_evt_win, ssm_get_cur_id(NULL));
}

void VW_EventLog_Hide()
{
    nfui_nfobject_hide(g_evt_win);

    nfui_page_close(PGID_LIVE_EVENT_LOG, g_evt_win);
}

gboolean VW_EventLog_IsShown()
{
    return nfui_nfobject_is_shown(g_evt_win);
}

