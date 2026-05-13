
/*
 * vw_live_statusbar.c
 *
 * written by seongho
 *
 */



#include <time.h>

#include "nf_afx.h"

#include "nf_sysman.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_page_manager.h"
#include "support/nf_ui_color.h"
#include "support/color.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"
#include "tools/nf_ui_function.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfbutton.h"
#include "viewers/objects/nfcheckbutton.h"
#include "viewers/objects/nfimage.h"
#include "viewers/objects/nfimglabel.h"

#include "modules/ssm.h"
#include "modules/stm.h"

#include "vw_live_statusbar.h"
#include "vw_live_start_menu.h"
#include "vw_live_event_log.h"
#include "vw_live_audio_input.h"
#include "vw_live_audio_output.h"
#include "vw_live_ptz_main.h"
#include "vw_live_alarm_status.h"
#include "vw_live_disk_status.h"
#include "vw_live_div_popup.h"
#include "vw_live_net_status.h"
#include "vw_live_net_ipcam_popup.h"
#include "vw_snapshot.h"
#include "vw_remote_ctrl_id.h"
#include "vw_search_main.h"
#include "vw_archiving.h"
#include "vw_userpwd.h"
#include "vw_system_setup.h"
#include "vw_change_account_popup.h"
#include "vw_packet_dump_popup.h"

#include "vw_timeline.h"
#include "vw_zoom_pip.h"
#include "vw_menu.h"

#include "vsm.h"
#include "uxm.h"
#include "cmm.h"
#include "vw_vwnd.h"
#include "iux_afx.h"
#include "vw.h"
#include "dtf.h"
#include "ssm.h"
#include "smt.h"
#include "evt.h"
#include "wnd.h"


#define DBG_LEVEL       9
#define DBG_MODULE      "VW_LIVE_STATUS"



#define CTRL_WIN_SIZE_W             DISPLAY_ACTIVE_WIDTH
#define CTRL_WIN_SIZE_H             (108)
#define CTRL_WIN_POS_X              (0)
#define CTRL_WIN_POS_Y              (DISPLAY_ACTIVE_HEIGHT - CTRL_WIN_SIZE_H)

#define TIMELABEL_SIZE_W            (197)
#define TIMELABEL_SIZE_H            (35)

// AM/PM label size
#define AMPM_LABEL_W                (70)
#define AMPM_LABEL_H                (35)

#if defined(GUI_4CH_SUPPORT)
#define GET_CHANNEL_KEY_SCOPE(kpid)         (kpid >= KEYPAD_CH01 && kpid <= KEYPAD_CH04)
#elif defined(GUI_8CH_SUPPORT)
#define GET_CHANNEL_KEY_SCOPE(kpid)         (kpid >= KEYPAD_CH01 && kpid <= KEYPAD_CH08)
#elif defined(GUI_16CH_SUPPORT)
#define GET_CHANNEL_KEY_SCOPE(kpid)         (kpid >= KEYPAD_CH01 && kpid <= KEYPAD_CH16)
#else
#define GET_CHANNEL_KEY_SCOPE(kpid)         ((kpid >= KEYPAD_CH01 && kpid <= KEYPAD_CH16) || (kpid >= KEYPAD_CH17 && kpid <= KEYPAD_CH32))
#endif

typedef struct _LIVE_STATUBAR_DATA {
    guint dformat;
    guint tformat;
    guint on_time;

    GTimer *timer;
    guint timer_id;

    guchar lmode;
    guint   tml_disp_mode;
}LIVE_STATUBAR_D;

enum {
    T24_HOUR_FORMAT = 0,
    T12_HOUR_FORMAT
};

enum {
    ALARM_OFF = 0,
    ALARM_ON_1,
    ALARM_ON_2,
    ALARM_STATUS_COUNT
};

enum {
    NET_CONNECTED = 0,
    NET_ERROR,
    NET_CONFLICT,
    NET_STATUS_COUNT
};

enum {
    DISK_NORMAL = 0,
    DISK_NEED_CHECK,
    DISK_FULL,
    DISK_NO,
    DISK_OVERWRITE,
    DISK_RAID_WARNING,
    DISK_SMART_ERR,
    DISK_STATUS_COUNT
};

enum {
    FULLMODE_BTN = 0,
    DIV_1_BTN,
    DIV_4_BTN,
#if !defined(GUI_4CH_SUPPORT)
    DIV_9_BTN,
#endif
#if defined(GUI_16CH_SUPPORT)
    DIV_16_BTN,
#endif
#if defined(GUI_32CH_SUPPORT)
    DIV_16_BTN,
    DIV_32_BTN,
#endif
    SEQ_BTN,
    OSD_BTN,
    PTZ_BTN,
    ZOOM_BTN,
    LOG_BTN,
    AUDIO_BTN,
    MIC_BTN,
    PANIC_BTN,
    ALARM_BTN,
    NET_BTN,
    DISK_BTN,
    MENU_BTNS
};

enum {
    ALWAYS_ON_MODE = 0,
    AUTO_HIDE_MODE,
    TIME_ON_MODE,
};

static UserManageData userdata;

static NFWINDOW *g_curwnd = 0;
static LIVE_STATUBAR_D g_ls_d;

static guint t_timer_id = 0;
static guint conflict_status =0;

static NFOBJECT *g_ctrl_win = NULL;
static NFOBJECT* g_date_label;
static NFOBJECT* g_time_label;
static NFOBJECT* g_ampm_label;
static NFOBJECT* g_menu = NULL;
static NFOBJECT* g_user_btn = NULL;
static NFOBJECT* g_menuObj[MENU_BTNS];
static NFOBJECT* wait_pop = NULL;

static GdkPixbuf *g_net_img[NET_STATUS_COUNT][NFOBJECT_STATE_COUNT];
static GdkPixbuf *g_alm_img[ALARM_STATUS_COUNT][NFOBJECT_STATE_COUNT];
static GdkPixbuf *g_disk_img[DISK_STATUS_COUNT][NFOBJECT_STATE_COUNT];
static GdkPixbuf *g_div_img[8][NFOBJECT_STATE_COUNT];



static void init_data_from_DAL();
static void create_menu_viewer(NFWINDOW *parent);
static gboolean update_time(gpointer data);
static void update_user_label();
static gboolean get_date_time(gchar *strTime);
static gboolean ctrl_win_post_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data);
static void show_live();
static void hide_live();
static void live_full_mode();
static void live_normal_mode();
static gboolean display_alarm_status(gpointer data);
static void reset_alarm_status(gpointer data);
static gboolean display_net_status(gint status, gint val);
static gboolean display_disk_status(gint status, gint val, gint r_status);

static guint get_on_time_mode();
static guint get_time_format();
static void start_on_time_timer();
static void stop_on_time_timer();
static void reset_on_time_timer();
static gboolean hide_statusbar(gpointer data);
static void set_live_mode(guchar mode);
static guchar get_live_mode();


static void init_data_from_DAL()
{
    memset(&g_ls_d, 0x00, sizeof(LIVE_STATUBAR_D));

    // statusbar on time
    g_ls_d.on_time = DAL_get_live_status_on_time();
}

static guint get_on_time_mode()
{
    return g_ls_d.on_time;
}

static guint get_time_format()
{
    return g_ls_d.tformat;
}

static void start_on_time_timer()
{
    // auto hide
    if(get_on_time_mode() == AUTO_HIDE_MODE)
        return;

    if(g_ls_d.timer == NULL)
        g_ls_d.timer = g_timer_new();

    g_timer_start(g_ls_d.timer);

    if(!g_ls_d.timer_id)
        g_ls_d.timer_id = g_timeout_add(1000, hide_statusbar, NULL);
}

static void stop_on_time_timer()
{
    g_timer_stop(g_ls_d.timer);

    if(g_ls_d.timer_id) {
        g_source_remove(g_ls_d.timer_id);
        g_ls_d.timer_id = 0;
    }
}

static void reset_on_time_timer()
{
    // auto hide
    if(get_on_time_mode() == AUTO_HIDE_MODE)
        return;

    if(get_on_time_mode())
        g_timer_start(g_ls_d.timer);
}

static void set_live_mode(guchar mode)
{
    g_ls_d.lmode = mode;
}

static guchar get_live_mode()
{
    return g_ls_d.lmode;
}

static gboolean hide_statusbar(gpointer data)
{
    gdouble elapsed_t;
    PAGEID cur_pgid;


    if(g_ls_d.timer) elapsed_t = g_timer_elapsed(g_ls_d.timer, NULL);
    cur_pgid = nfui_get_cur_page(NULL);

    //g_message("%s : elapsed time %.2f on_timer: %d ", __FUNCTION__, elapsed_t, g_ls_d.on_time);
    //g_message("%s ::::::::::::: current page id: %d ", __FUNCTION__, cur_pgid);

    // always & auto hide
    if(get_on_time_mode() == ALWAYS_ON_MODE || get_on_time_mode() == AUTO_HIDE_MODE) {
        // FIXME: to find a better way
        {
            if(cur_pgid != PGID_LIVECTRLBAR)
                return TRUE;
        }

        if(g_ls_d.timer) g_timer_stop(g_ls_d.timer);
        g_ls_d.timer_id = 0;

        if(get_on_time_mode() == ALWAYS_ON_MODE && g_ctrl_win->show == NFOBJECT_HIDE) {
            NFUTIL_THREADS_ENTER();
            nfui_nfobject_show(g_ctrl_win);
            NFUTIL_THREADS_LEAVE();
        }

        if(get_on_time_mode() == AUTO_HIDE_MODE && g_ctrl_win->show == NFOBJECT_SHOW) {
            NFUTIL_THREADS_ENTER();
            nfui_nfobject_hide(g_ctrl_win);
            NFUTIL_THREADS_LEAVE();
        }

        return FALSE;
    }

    if(cur_pgid != PGID_LIVECTRLBAR) {
        g_timer_start(g_ls_d.timer);
        return TRUE;
    }

    if((guint)elapsed_t >= g_ls_d.on_time) {
        if(g_ctrl_win->show == NFOBJECT_SHOW) {
            NFUTIL_THREADS_ENTER();
            nfui_nfobject_hide(g_ctrl_win);
            NFUTIL_THREADS_LEAVE();
        }

        g_timer_start(g_ls_d.timer);
    }

    return TRUE;
}

static int destroy_menu_viewer()
{
/*  LiveStart_Popup_Menu_Destroy();
    VW_Destroy_EventLog();
    VW_Destroy_AudioInput();
    VW_Destroy_AudioOutput();
    VW_Destroy_AlarmStatus();
    VW_Destroy_NetStatus();
    VW_Live_Net_IPCam_Popup_Close();
*/
    return 0;
}

static void create_menu_viewer(NFWINDOW *parent)
{
    LiveStart_Popup_Menu(parent);
    VW_Create_EventLog(parent, 725, 431);
    VW_Create_AudioInput(parent, 1020, 808);
    VW_Create_AudioOutput(parent, 1100, 724);
    VW_Create_AlarmStatus(parent, 790, 454);
    VW_Create_NetStatus(parent);
    VW_Live_Net_IPCam_Popup_Open(parent);

    if (scm_is_qc_mode() == 0)
        vw_qc_test_create_new(parent);
}

static void create_menu_viewer_HDI(NFWINDOW *parent)
{
    LiveStart_Popup_Menu(parent);
    VW_Create_EventLog(parent, 725, 431);
    VW_Create_AudioInput(parent, 1020, 808);
    VW_Create_AudioOutput(parent, 1100, 724);
    VW_Create_AlarmStatus(parent, 790, 534);
    VW_Create_NetStatus(parent);
    VW_Live_Net_IPCam_Popup_Open(parent);

    if (scm_is_qc_mode() == 0)
        vw_qc_test_create_new(parent);
}

static gboolean change_time_obj()
{
    static guint tformat = 100;
    NFOBJECT *parent;

    if(tformat != get_time_format()) {
        tformat = get_time_format();

        if(get_time_format() == T12_HOUR_FORMAT) {
#if 0
            nfui_nflabel_set_align((NFLABEL*)g_time_label, NFALIGN_LEFT, 30);
            nfui_nfobject_set_size(g_time_label, (TIMELABEL_SIZE_W - AMPM_LABEL_W), AMPM_LABEL_H);
#else
            nfui_nfimage_set_font_alignment((NFIMAGE*)g_time_label, NFALIGN_LEFT, 30);
            nfui_nfobject_set_size(g_time_label, (TIMELABEL_SIZE_W - AMPM_LABEL_W), AMPM_LABEL_H);
#endif
        }else {
#if 0
            nfui_nflabel_set_align((NFLABEL*)g_time_label, NFALIGN_CENTER, 0);
            nfui_nfobject_set_size(g_time_label, TIMELABEL_SIZE_W, TIMELABEL_SIZE_H);
#else
            nfui_nfimage_set_font_alignment((NFIMAGE*)g_time_label, NFALIGN_CENTER, 0);
            nfui_nfobject_set_size(g_time_label, TIMELABEL_SIZE_W, TIMELABEL_SIZE_H);
#endif
        }

        if(get_time_format() == T12_HOUR_FORMAT) {
            nfui_nfobject_set_size(g_ampm_label, AMPM_LABEL_W, AMPM_LABEL_H);
            nfui_nffixed_move((NFFIXED*)g_ampm_label->parent, g_ampm_label, (guint)(1694 + TIMELABEL_SIZE_W - AMPM_LABEL_W), (guint)(19 + TIMELABEL_SIZE_H));
        } else  {
            //nfui_nflabel_set_text((NFLABEL*)g_ampm_label, "");
            nfui_nfimage_set_text((NFLABEL*)g_ampm_label, "");
            nfui_nfobject_set_size(g_ampm_label, 1, 1);
            nfui_nffixed_move((NFFIXED*)g_ampm_label->parent, g_ampm_label, (guint)(1694 + TIMELABEL_SIZE_W), (guint)(19 + TIMELABEL_SIZE_H));

            //nfui_signal_emit(g_ampm_label, GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_ampm_label, GDK_EXPOSE, FALSE);
        }

        return TRUE;
    }
    return FALSE;
}


static gboolean update_time(gpointer data)
{
    gchar dateBuf[16];
    gchar timeBuf[16];
    gchar *preBuf;

    memset(dateBuf, 0x00, sizeof(dateBuf));
    memset(timeBuf, 0x00, sizeof(timeBuf));

    NFUTIL_THREADS_ENTER();

    change_time_obj();

    dtf_get_current_date(dateBuf);
    dtf_get_current_time(timeBuf);
#if 0
    preBuf = nfui_nflabel_get_text((NFLABEL*)g_time_label);
#else
    preBuf = nfui_nfimage_get_text((NFIMAGE*)g_time_label);
#endif

    if(!preBuf) {
        nfui_nfimage_set_text((NFIMAGE*)g_date_label, dateBuf);
        nfui_nfimage_set_text((NFIMAGE*)g_time_label, timeBuf);

        nfui_signal_emit(g_date_label, GDK_EXPOSE, FALSE);
        nfui_signal_emit(g_time_label, GDK_EXPOSE, FALSE);

        NFUTIL_THREADS_LEAVE();
        return TRUE;
    }

    if (strcmp(timeBuf, preBuf) != 0)
    {
#if 0
        nfui_nflabel_set_text((NFLABEL*)g_time_label, timeBuf);
#else
        nfui_nfimage_set_text((NFIMAGE*)g_time_label, timeBuf);
#endif

        //nfui_signal_emit(g_time_label, GDK_EXPOSE, FALSE);
        nfui_signal_emit(g_time_label, GDK_EXPOSE, FALSE);
    }

    preBuf = nfui_nfimage_get_text((NFIMAGE*)g_date_label);
    if (strcmp(dateBuf, preBuf) != 0)
    {
#if 0
        nfui_nflabel_set_text((NFLABEL*)g_date_label, dateBuf);
#else
        nfui_nfimage_set_text((NFIMAGE*)g_date_label, dateBuf);
#endif

        //nfui_signal_emit(g_date_label, GDK_EXPOSE, FALSE);
        nfui_signal_emit(g_date_label, GDK_EXPOSE, FALSE);
    }


    NFUTIL_THREADS_LEAVE();

    return TRUE;
}

static void update_user_label()
{
    gchar *text_btn = NULL;
    gchar *user = NULL;

    if(DAL_get_username_on_off())
    {
        if(DAL_get_passwd_change_enable() && smt_get_service() != SMT_LOGOUT)
        {
            nfui_nfobject_enable(g_user_btn);
        }
        else
        {
            nfui_nfobject_disable(g_user_btn);
        }
        text_btn = nfui_nfbutton_get_text((NFBUTTON*)g_user_btn);

        if(text_btn)
        {
            user = ssm_get_cur_id(NULL);
            if(user)
            {
                if(strcmp(user, text_btn))
                {
                    nfui_nfobject_use_tooltip(g_user_btn, TRUE);
                    nfui_nfbutton_set_text((NFBUTTON*)g_user_btn, user);
                    nfui_nfobject_support_multi_lang((NFOBJECT*)g_user_btn, FALSE);
                    nfui_signal_emit(g_user_btn, GDK_EXPOSE, FALSE);
                }
            }
        }
    }
    else
    {
        nfui_nfobject_disable(g_user_btn);
        nfui_nfbutton_set_text((NFBUTTON*)g_user_btn, " ");
        nfui_signal_emit(g_user_btn, GDK_EXPOSE, FALSE);
    }
}

static void show_live()
{
    VW_Live_StatusBar_Show();
    VW_Timeline_Show();
}


static void hide_live()
{
    VW_Timeline_Hide();
    VW_Live_StatusBar_Hide();
}

static void live_full_mode()
{
    vsm_change_full_mode();
    hide_live();

    // start timer
    start_on_time_timer();
    set_live_mode(0);
}

static void live_normal_mode()
{
    vsm_change_normal_mode();
    show_live();

    // stop timer
    stop_on_time_timer();
    set_live_mode(1);
}

static gint _proc_pw_change(gint id_idx, gchar *new_passwd)
{
    UserManageData userdata;
    gint ret;
    guint opt = 0;
    guint max_ch = 0;

    if (DAL_get_enahnced_password())
    {
        opt |= (1 << OPT_ENAHNCED_PASSWORD);
        max_ch = NFUI_MAX_ENHANCED_PW_SIZE;
    }
    else
    {
        max_ch = NFUI_MAX_PW_SIZE;
    }

    memset(&userdata, 0x00, sizeof(UserManageData));
    DAL_get_userManage_data(&userdata, id_idx);

    ret = VW_Open_account_change_Popup(g_curwnd, "CHANGE PASSWORD", opt, userdata.id, userdata.pw, new_passwd, max_ch, 1);
    if (ret <= 0) return ret;
    if (strcmp(userdata.pw, new_passwd) == 0) return 0;

    userdata.pw_last_changed = time(0);
    g_stpcpy(userdata.pw, new_passwd);

    DAL_set_userManage_data(userdata, id_idx);
    DAL_save_setup_db(NFSETUP_WINDOW_USER);
    nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");

    return 1;
}

static void reset_alarm_status(gpointer data)
{
    nfui_nfbutton_set_image(NF_BUTTON(g_menuObj[ALARM_BTN]), g_alm_img[ALARM_OFF]);
    nfui_signal_emit(g_menuObj[ALARM_BTN], GDK_EXPOSE, FALSE);
}

static gboolean display_alarm_status(gpointer data)
{
    static gint status = ALARM_OFF;

    if(status == ALARM_OFF)         status = ALARM_ON_1;
    else if(status == ALARM_ON_1)   status = ALARM_ON_2;
    else if(status == ALARM_ON_2)   status = ALARM_ON_1;

    nfui_nfbutton_set_image(NF_BUTTON(g_menuObj[ALARM_BTN]), g_alm_img[status]);
    nfui_signal_emit(g_menuObj[ALARM_BTN], GDK_EXPOSE, FALSE);

    return TRUE;
}

static gboolean display_net_status(gint status, gint val)
{
    static gint img_status = NET_ERROR;
    static GdkPixbuf *pbBG = NULL;

    if(conflict_status) return FALSE;
    if(status < 0 || status > NET_CONFLICT)
        return FALSE;

    if(img_status != status) {
        img_status = status;

        nfui_nfbutton_set_image(NF_BUTTON(g_menuObj[NET_BTN]), g_net_img[status]);
        nfui_signal_emit(g_menuObj[NET_BTN], GDK_EXPOSE, FALSE);

        if(!pbBG) {
            pbBG = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 33, 22);

            gdk_pixbuf_copy_area(g_net_img[NET_CONNECTED][0],
                    38, 27,
                    33, 22,
                    pbBG,
                    0, 0);
        }
    }

    if(val > 0) {
        GdkDrawable *drawable = NULL;
        GdkGC *gc = NULL;
        gchar strBuf[8];
        gint str_pos_x;

        drawable = nfui_nfobject_get_window(g_menuObj[NET_BTN]);
        gc = gdk_gc_new(drawable);

        if(pbBG && img_status == NET_CONNECTED)
            nfutil_draw_pixbuf(drawable, gc, pbBG, 1491+38, 19+27, 33, 22, NFALIGN_LEFT, 0);

        if (val >= 10)  str_pos_x = 1534;
        else            str_pos_x = 1539;

        g_sprintf(strBuf, "%d", val);

        nfutil_draw_text_with_pango_outline(NULL, NULL, NULL, drawable, gc, strBuf, str_pos_x, 24, 92, 70, nffont_get_pango_font(NFFONT_SMALL_SEMI), &UX_COLOR(330), NFALIGN_LEFT, 0);

        if(gc)
            g_object_unref(gc);
    }

    return TRUE;
}

static gboolean display_disk_status(gint status, gint val, gint r_status)
{
    static gint img_status = 0;
    static gint r_img_status = 0;
    static GdkPixbuf *pbBG = NULL;
    GdkDrawable *drawable = NULL;
    GdkGC* gc = NULL;
    gchar strBuf[8];
    gint i = 0;

    if(!pbBG) {
        pbBG = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 50, 28);
        gdk_pixbuf_copy_area(g_disk_img[DISK_NORMAL][0], 48, 24, 50, 28, pbBG, 0, 0);
    }
    drawable = nfui_nfobject_get_window(g_menuObj[DISK_BTN]);
    gc = gdk_gc_new(drawable);

    if(r_status == RST_DEGRADE || r_status == RST_BROKEN)
    {
        if(r_img_status != r_status){
            r_img_status = r_status;
            nfui_nfbutton_set_image(NF_BUTTON(g_menuObj[DISK_BTN]), g_disk_img[DISK_RAID_WARNING]);
            nfui_signal_emit(g_menuObj[DISK_BTN], GDK_EXPOSE, FALSE);
        }
        nfutil_draw_pixbuf(drawable, gc, pbBG, 1632, 43, 50, 28, NFALIGN_LEFT, 0);

        if (status & (1 << DISK_OVERWRITE)) {
            nfutil_draw_text_with_pango_outline(NULL, NULL, NULL, drawable, gc, "OW", 1636, 24, 110, 70, nffont_get_pango_font(NFFONT_SMALL_SEMI), &UX_COLOR(332), NFALIGN_LEFT, 0);
        }
        else if (status & (1 << DISK_FULL))
        {
            for(i=0; i<(val%2 ? (val+1)/2 : val/2); i++)
                nfutil_draw_image(drawable, gc, IMG_DISK_BAR_FULL, (1631 + i), 43, 1, 26, NFALIGN_LEFT, 0);

            nfutil_draw_text_with_pango_outline(NULL, NULL, NULL, drawable, gc, "100%", 1632, 24, 110, 70, nffont_get_pango_font(NFFONT_SMALL_SEMI), &UX_COLOR(334), NFALIGN_LEFT, 0);
        }
        else
        {
            for(i=0; i<(val%2 ? (val+1)/2 : val/2); i++)
                nfutil_draw_image(drawable, gc, IMG_DISK_BAR_NORMAL, (1631 + i), 43, 1, 26, NFALIGN_LEFT, 0);

            g_sprintf(strBuf, "%d%%", val);
            nfutil_draw_text_with_pango_outline(NULL, NULL, NULL, drawable, gc, strBuf, 1636, 24, 110, 70, nffont_get_pango_font(NFFONT_SMALL_SEMI), &UX_COLOR(332), NFALIGN_LEFT, 0);
        }
    }
    else if (status & (1 << DISK_NEED_CHECK))
    {
        if(img_status != status || r_img_status != r_status) {
            img_status = status;
            r_img_status = r_status;
            nfui_nfbutton_set_image(NF_BUTTON(g_menuObj[DISK_BTN]), g_disk_img[DISK_NEED_CHECK]);
            nfui_signal_emit(g_menuObj[DISK_BTN], GDK_EXPOSE, FALSE);
        }
        nfutil_draw_pixbuf(drawable, gc, pbBG, 1632, 43, 50, 28, NFALIGN_LEFT, 0);

        if (status & (1 << DISK_OVERWRITE)) {
            nfutil_draw_text_with_pango_outline(NULL, NULL, NULL, drawable, gc, "OW", 1636, 24, 110, 70, nffont_get_pango_font(NFFONT_SMALL_SEMI), &UX_COLOR(332), NFALIGN_LEFT, 0);
        }
        else if (status & (1 << DISK_FULL))
        {
            for(i=0; i<(val%2 ? (val+1)/2 : val/2); i++)
                nfutil_draw_image(drawable, gc, IMG_DISK_BAR_FULL, (1631 + i), 43, 1, 26, NFALIGN_LEFT, 0);

            nfutil_draw_text_with_pango_outline(NULL, NULL, NULL, drawable, gc, "100%", 1632, 24, 110, 70, nffont_get_pango_font(NFFONT_SMALL_SEMI), &UX_COLOR(334), NFALIGN_LEFT, 0);
        }
        else
        {
            for(i=0; i<(val%2 ? (val+1)/2 : val/2); i++)
                nfutil_draw_image(drawable, gc, IMG_DISK_BAR_NORMAL, (1631 + i), 43, 1, 26, NFALIGN_LEFT, 0);

            g_sprintf(strBuf, "%d%%", val);
            nfutil_draw_text_with_pango_outline(NULL, NULL, NULL, drawable, gc, strBuf, 1636, 24, 110, 70, nffont_get_pango_font(NFFONT_SMALL_SEMI), &UX_COLOR(332), NFALIGN_LEFT, 0);
        }
    }
    else if (status & (1 << DISK_SMART_ERR))
    {
        if(img_status != status || r_img_status != r_status) {
            img_status = status;
            r_img_status = r_status;
            nfui_nfbutton_set_image(NF_BUTTON(g_menuObj[DISK_BTN]), g_disk_img[DISK_NEED_CHECK]);
            nfui_signal_emit(g_menuObj[DISK_BTN], GDK_EXPOSE, FALSE);
        }
        nfutil_draw_pixbuf(drawable, gc, pbBG, 1632, 43, 50, 28, NFALIGN_LEFT, 0);

        if (status & (1 << DISK_OVERWRITE)) {
            nfutil_draw_text_with_pango_outline(NULL, NULL, NULL, drawable, gc, "OW", 1636, 24, 110, 70, nffont_get_pango_font(NFFONT_SMALL_SEMI), &UX_COLOR(332), NFALIGN_LEFT, 0);
        }
        else if (status & (1 << DISK_FULL))
        {
            for(i=0; i<(val%2 ? (val+1)/2 : val/2); i++)
                nfutil_draw_image(drawable, gc, IMG_DISK_BAR_FULL, (1631 + i), 43, 1, 26, NFALIGN_LEFT, 0);

            nfutil_draw_text_with_pango_outline(NULL, NULL, NULL, drawable, gc, "100%", 1632, 24, 110, 70, nffont_get_pango_font(NFFONT_SMALL_SEMI), &UX_COLOR(334), NFALIGN_LEFT, 0);
        }
        else
        {
            for(i=0; i<(val%2 ? (val+1)/2 : val/2); i++)
                nfutil_draw_image(drawable, gc, IMG_DISK_BAR_NORMAL, (1631 + i), 43, 1, 26, NFALIGN_LEFT, 0);

            g_sprintf(strBuf, "%d%%", val);
            nfutil_draw_text_with_pango_outline(NULL, NULL, NULL, drawable, gc, strBuf, 1636, 24, 110, 70, nffont_get_pango_font(NFFONT_SMALL_SEMI), &UX_COLOR(332), NFALIGN_LEFT, 0);
        }
    }
    else if (status & (1 << DISK_OVERWRITE))
    {
        if(img_status != status || r_img_status != r_status) {
            img_status = status;
            r_img_status = r_status;
            nfui_nfbutton_set_image(NF_BUTTON(g_menuObj[DISK_BTN]), g_disk_img[DISK_NORMAL]);
            nfui_signal_emit(g_menuObj[DISK_BTN], GDK_EXPOSE, FALSE);
        }
        nfutil_draw_pixbuf(drawable, gc, pbBG, 1632, 43, 50, 28, NFALIGN_LEFT, 0);
        nfutil_draw_text_with_pango_outline(NULL, NULL, NULL, drawable, gc, "OW", 1636, 24, 110, 70, nffont_get_pango_font(NFFONT_SMALL_SEMI), &UX_COLOR(332), NFALIGN_LEFT, 0);
    }
    else if (status & (1 << DISK_FULL))
    {
        if(img_status != status || r_img_status != r_status) {
            img_status = status;
            r_img_status = r_status;
            nfui_nfbutton_set_image(NF_BUTTON(g_menuObj[DISK_BTN]), g_disk_img[DISK_FULL]);
            nfui_signal_emit(g_menuObj[DISK_BTN], GDK_EXPOSE, FALSE);
        }
        nfutil_draw_pixbuf(drawable, gc, pbBG, 1632, 43, 50, 28, NFALIGN_LEFT, 0);

        for(i=0; i<(val%2 ? (val+1)/2 : val/2); i++)
            nfutil_draw_image(drawable, gc, IMG_DISK_BAR_FULL, (1631 + i), 43, 1, 26, NFALIGN_LEFT, 0);

        nfutil_draw_text_with_pango_outline(NULL, NULL, NULL, drawable, gc, "100%", 1632, 24, 110, 70, nffont_get_pango_font(NFFONT_SMALL_SEMI), &UX_COLOR(334), NFALIGN_LEFT, 0);
    }
    else if (status & (1 << DISK_NO))
    {
        if(img_status != status || r_img_status != r_status) {
            img_status = status;
            r_img_status = r_status;

        nfui_nfbutton_set_image(NF_BUTTON(g_menuObj[DISK_BTN]), g_disk_img[DISK_NO]);
            nfui_signal_emit(g_menuObj[DISK_BTN], GDK_EXPOSE, FALSE);
        }
    }
    else if (status & (1 << DISK_NORMAL))
    {
        if(img_status != status || r_img_status != r_status) {
            img_status = status;
            r_img_status = r_status;
            nfui_nfbutton_set_image(NF_BUTTON(g_menuObj[DISK_BTN]), g_disk_img[DISK_NORMAL]);
            nfui_signal_emit(g_menuObj[DISK_BTN], GDK_EXPOSE, FALSE);
        }
        nfutil_draw_pixbuf(drawable, gc, pbBG, 1632, 43, 50, 28, NFALIGN_LEFT, 0);

        for(i=0; i<(val%2 ? (val+1)/2 : val/2); i++)
            nfutil_draw_image(drawable, gc, IMG_DISK_BAR_NORMAL, (1631 + i), 43, 1, 26, NFALIGN_LEFT, 0);

        g_sprintf(strBuf, "%d%%", val);
        nfutil_draw_text_with_pango_outline(NULL, NULL, NULL, drawable, gc, strBuf, 1636, 24, 110, 70, nffont_get_pango_font(NFFONT_SMALL_SEMI), &UX_COLOR(332), NFALIGN_LEFT, 0);
    }

    g_object_unref(gc);

    return TRUE;
}

static gboolean ready_shutdown(gpointer data)
{
    smt_set_service(SMT_SHUTDOWN);

    scm_shutdown_system(IRET_SCM_SHUTDOWN2);
    scm_notify_to_system("dvr_status", NF_DVR_STATUS_SHUTDOWN);

    NFUTIL_THREADS_ENTER();
    nftool_remove_waitbox((NFOBJECT*)data);

    nftool_mbox_wait(g_curwnd, "NOTICE", "You can power off the system.");
    NFUTIL_THREADS_LEAVE();

    return FALSE;
}

static gboolean cleanup_new_opening(void *data)
{
    if (vsm_get_omode() == OMODE_FULL)
    {
        // always mode
        if(VW_Live_StatusBar_On_Time() != 0)
            VW_Live_StatusBar_Hide();
        VW_Timeline_Hide();
    }

    return FALSE;
}

static gint _find_relay_event(KEYPAD_KID kpid, gint *active_out)
{
    static gint active = -1;
    static gint key_cnt = 0;

    g_message("%s, %d, active:%d, key_cnt:%d,", __FUNCTION__, __LINE__, active, key_cnt);

    switch (key_cnt)
    {
        case 0:
        {
            if (kpid == KEYPAD_RELAY) {
                active = 1;
                key_cnt++;
                return 0;
            }
        }
        break;

        case 1:
        {
            if ((active == 1) && (GET_CHANNEL_KEY_SCOPE(kpid))) {
                *active_out = active;
                active = -1;
                key_cnt = 0;
                return 0;
            }
            else if (kpid == KEYPAD_RELAY) {
                active = 0;
                key_cnt++;
                return 0;
            }
        }
        break;

        case 2:
        {
            if ((active == 0) && (GET_CHANNEL_KEY_SCOPE(kpid))) {
                *active_out = active;
                active = -1;
                key_cnt = 0;
                return 0;
            }
        }
        break;
    }

    active = -1;
    key_cnt = 0;

    return -1;
}

static gint _set_relay_out(gint ch, gint onoff)
{
    EA_AlmOutData alout;

    if (ch >= GUI_CHANNEL_CNT) return 0;

    g_message("%s, %d, ch:%d, onoff:%d", __FUNCTION__, __LINE__, ch, onoff);

    memset(&alout, 0x00, sizeof(EA_AlmOutData));
    DAL_get_almOut_data(&alout, ch);
    nf_action_relay_test(onoff, ch, alout.oper);

    return 0;
}

static gboolean post_ptz_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    guint ch;
    if(evt->type == GDK_BUTTON_RELEASE) {
        // full mode
        if(get_live_mode() == 0)
            reset_on_time_timer();

        if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        if(!nfui_nfobject_is_shown(g_ctrl_win))
            return FALSE;

        ch = vsm_get_focused_channel();
        
        if(vsm_get_covert_state(NULL, ch)) {
            nftool_mbox(g_curwnd, "ERROR", "Current channel is coverted.",  NFTOOL_MB_OK);
            return FALSE;
        }


        hide_live();

        VW_Live_Ptz_Main_Open(g_curwnd, ch);

        if (vsm_get_omode() == OMODE_NORMAL)
        {
            show_live();
        }

    }
    else if(evt->type == GDK_BUTTON_PRESS
            || evt->type == GDK_MOTION_NOTIFY)
    {
        // full mode
        if(get_live_mode() == 0)
            reset_on_time_timer();
    }
    return FALSE;
}

static gboolean post_zoom_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    guint ch;
    gint x, y;

    if(evt->type == GDK_BUTTON_RELEASE) {
        // full mode
        if(get_live_mode() == 0)
            reset_on_time_timer();

        if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        if(!nfui_nfobject_is_shown(g_ctrl_win))
            return FALSE;

        ch = vsm_get_focused_channel();
        if(vsm_get_covert_state(NULL, ch)) {
            nftool_mbox(g_curwnd, "ERROR", "Current channel is coverted.",  NFTOOL_MB_OK);
            return FALSE;
        }

        hide_live();

        x = DISPLAY_ACTIVE_WIDTH - VW_ZoomPIP_Width();
        y = DISPLAY_ACTIVE_HEIGHT - VW_ZoomPIP_Height();

        VW_ZoomPIP_Open(g_curwnd, x, y, ch, ZOOM_PIP_NONE);

        if (vsm_get_omode() == OMODE_NORMAL)
        {
            show_live();
        }

    }
    else if(evt->type == GDK_BUTTON_PRESS
            || evt->type == GDK_MOTION_NOTIFY)
    {
        // full mode
        if(get_live_mode() == 0)
            reset_on_time_timer();
    }
    return FALSE;
}

static gboolean post_log_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) {
		// full mode
		if(get_live_mode() == 0)
			reset_on_time_timer();

        if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        if(!nfui_nfobject_is_shown(g_ctrl_win))
            return FALSE;

        if(VW_EventLog_IsShown()) VW_EventLog_Hide();
        else                      VW_EventLog_Show();
    }
    else if(evt->type == GDK_BUTTON_PRESS
            || evt->type == GDK_MOTION_NOTIFY)
    {
        // full mode
        if(get_live_mode() == 0)
            reset_on_time_timer();
    }

    return FALSE;
}

static gboolean post_audio_input_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE) {
        // full mode
        if(get_live_mode() == 0)
            reset_on_time_timer();

        if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        if(!nfui_nfobject_is_shown(g_ctrl_win))
            return FALSE;

        if(VW_AudioInput_IsShown()) VW_AudioInput_Hide();
        else                        VW_AudioInput_Show();
    }
    else if(evt->type == GDK_BUTTON_PRESS
            || evt->type == GDK_MOTION_NOTIFY)
    {
        // full mode
        if(get_live_mode() == 0)
            reset_on_time_timer();
    }

    return FALSE;
}

static gboolean post_audio_output_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE) {
        // full mode
        if(get_live_mode() == 0)
            reset_on_time_timer();

        if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        if(!nfui_nfobject_is_shown(g_ctrl_win))
            return FALSE;

        if(VW_AudioOutput_IsShown()) VW_AudioOutput_Hide();
        else                         VW_AudioOutput_Show(1100, 724);
    }
    else if(evt->type == GDK_BUTTON_PRESS
            || evt->type == GDK_MOTION_NOTIFY)
    {
        // full mode
        if(get_live_mode() == 0)
            reset_on_time_timer();
    }

    return FALSE;
}

static gboolean post_panic_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        if (get_live_mode() == 0) reset_on_time_timer();

        scm_toggle_panic_record();
    }
    else if(evt->type == INFY_PANIC_REC_STATUS)
    {
        // g_message("%s, %d, param:%d", __FUNCTION__, __LINE__, ((CMM_MESSAGE_T *)data)->param);

        if (((CMM_MESSAGE_T *)data)->param)
            nfui_check_button_set_active((NFCHECKBUTTON*)obj, TRUE);
        else
            nfui_check_button_set_active((NFCHECKBUTTON*)obj, FALSE);
    }

    return FALSE;
}

static gboolean post_alarm_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    static guint timer_id = 0;
    NF_NOTIFY_INFO *pInfo = NULL;

    switch(evt->type) {
        case GDK_BUTTON_RELEASE:
            {
                // full mode
                if(get_live_mode() == 0)
                    reset_on_time_timer();

                if(evt->button.button == MOUSE_RIGTH_BUTTON)
                    return FALSE;

                if(!nfui_nfobject_is_shown(g_ctrl_win))
                    return FALSE;

                VW_AlarmStatus_Show();

                if(timer_id) {
                    g_source_remove(timer_id);
                    timer_id = 0;
                    vsm_turn_off_alarm_status();
                }
            }
            break;

        case INFY_DISK_FULL_NOTIFY: // 4102
        case INFY_SENSOR_NOTIFY:    // 4119
        case INFY_MOTION_NOTIFY:    // 4120
        case INFY_VLOSS_NOTIFY:     // 4099
        case INFY_ALARM_NOTIFY:     // 4172
        case INFY_AI_KEEP_ALIVE_NOTIFY:
            {
                gint i;

                if(data)
                    pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);

                if(pInfo == NULL)
                    break;

                //g_message("%s [%d] ::::::::::::evt->type = %d, alarm on = %d param 0: %d , param 1: %d ",
                //      __FUNCTION__, __LINE__,
                //      evt->type, run_timer,
                //      pInfo->d.params[0],
                //      pInfo->d.params[1]);

                //g_message("%s [%d] :::::::::::: alarm on = %d",
                //      __FUNCTION__, __LINE__,
                //      run_timer);

                VW_AlarmStatus_Set_Noti(evt->type, pInfo);
            }
            break;

        case INFY_SYSFAN_NOTIFY:
        case INFY_TEMPERATURE_NOTIFY:
        case INFY_POE_NOTIFY:
        case INFY_POE_HUB_NOTIFY:
        case INFY_WRITEFAIL_NOTIFY:
        case INFY_EXHAUST_NOTIFY:
        case INFY_NODISK_NOTIFY:
        case INFY_SMART_WARN_NOTIFY:
        case INFY_SMART_ERROR_NOTIFY:
        case INFY_WAN_NOTIFY:
        case INFY_DDNS_NOTIFY:
        case INFY_NETLOGINFAIL_NOTIFY:
        case INFY_SET_IP_CONFLICT_NOTIFY:
        case INFY_CAM_IP_CONFLICT_NOTIFY:
            {
                if(data)
                    pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);

                if(pInfo == NULL)
                    break;

                VW_AlarmStatus_Set_Noti(evt->type, pInfo);
            }
            break;

        case INFY_BUZZER_NOTIFY:
            {
                if(data)
                    pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);

                if(pInfo == NULL)
                    break;

                g_message("%d .......%d........ buzzer noti", __LINE__, pInfo->d.params[0]);
                if(pInfo->d.params[0])
                {
                    if(timer_id == 0)
                    {
                        if(VW_AlarmStatus_IsShown())
                            break;

                        display_alarm_status(NULL);
                        timer_id = g_timeout_add_full(G_PRIORITY_DEFAULT, 500, display_alarm_status, NULL, reset_alarm_status);
                        vsm_turn_on_alarm_status();
                    }
                }
                else
                {
                    if(timer_id)
                    {
                        g_source_remove(timer_id);
                        timer_id = 0;
                        vsm_turn_off_alarm_status();
                    }
                }
            }
            break;

        case GDK_BUTTON_PRESS:
        case GDK_MOTION_NOTIFY:
            {
                // full mode
                if(get_live_mode() == 0)
                    reset_on_time_timer();
            }
            break;

        case GDK_DELETE:
        {
            // alarm event
            uxm_unreg_imsg_event(obj, INFY_SENSOR_NOTIFY);
            uxm_unreg_imsg_event(obj, INFY_MOTION_NOTIFY);
            uxm_unreg_imsg_event(obj, INFY_VLOSS_NOTIFY);
            uxm_unreg_imsg_event(obj, INFY_ALARM_NOTIFY);
            uxm_unreg_imsg_event(obj, INFY_BUZZER_NOTIFY);
            uxm_unreg_imsg_event(obj, INFY_AI_KEEP_ALIVE_NOTIFY);

            // system event
            uxm_unreg_imsg_event(obj, INFY_SYSFAN_NOTIFY);
            uxm_unreg_imsg_event(obj, INFY_TEMPERATURE_NOTIFY);
            uxm_unreg_imsg_event(obj, INFY_POE_NOTIFY);
            uxm_unreg_imsg_event(obj, INFY_POE_HUB_NOTIFY);

            // disk event
            uxm_unreg_imsg_event(obj, INFY_WRITEFAIL_NOTIFY);
            uxm_unreg_imsg_event(obj, INFY_EXHAUST_NOTIFY);
            uxm_unreg_imsg_event(obj, INFY_NODISK_NOTIFY);
            uxm_unreg_imsg_event(obj, INFY_SMART_WARN_NOTIFY);
            uxm_unreg_imsg_event(obj, INFY_SMART_ERROR_NOTIFY);
            uxm_unreg_imsg_event(obj, INFY_DISK_FULL_NOTIFY);

            // network event
            uxm_unreg_imsg_event(obj, INFY_WAN_NOTIFY);
            uxm_unreg_imsg_event(obj, INFY_DDNS_NOTIFY);
            uxm_unreg_imsg_event(obj, INFY_NETLOGINFAIL_NOTIFY);
            uxm_unreg_imsg_event(obj, INFY_SET_IP_CONFLICT_NOTIFY);
            uxm_unreg_imsg_event(obj, INFY_CAM_IP_CONFLICT_NOTIFY);
        }
        break;

        default:
            break;
    }

    return FALSE;
}

static gboolean post_network_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    static gint net_status = NET_ERROR;
    static gint net_count = -1;
    gint new_net_count = -1;
    NF_NOTIFY_INFO *pInfo = NULL;

    switch(evt->type) {
        case GDK_EXPOSE:
        case GDK_BUTTON_PRESS:
        case GDK_ENTER_NOTIFY:
        case GDK_LEAVE_NOTIFY:
        case NFEVENT_KEYPAD_PRESS:
        case NFEVENT_KEYPAD_RELEASE:
        case NFEVENT_REMOCON_PRESS:
        case NFEVENT_REMOCON_RELEASE:
            // full mode
            if(get_live_mode() == 0)
                reset_on_time_timer();

            if(!nfui_nfobject_is_shown(g_ctrl_win))
                return FALSE;

			display_net_status(net_status, net_count);
        break;

        case GDK_BUTTON_RELEASE:
        {
            display_net_status(net_status, net_count);

            if(evt->button.button == MOUSE_RIGTH_BUTTON)
                return FALSE;

            if(!nfui_nfobject_is_shown(g_ctrl_win))
                return FALSE;

            if(VW_NetStatus_IsShown()) VW_NetStatus_Hide();
            else                       VW_NetStatus_Show();
        }
            break;

        case INFY_NET_NOTIFY:
            {
                pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);
                if(pInfo != NULL) {
                g_message("%s ::::::::::::::: RECEIVE NET NOTIFY:::%d::", __FUNCTION__, pInfo->d.params[0]);

                    if(net_status == NET_CONFLICT)
                        break;

					new_net_count = scm_get_net_session_count();

                    if(net_count == new_net_count)
                        break;

                    net_count = new_net_count;

                    if(net_count > 0) net_status = NET_CONNECTED;
                    else              net_status = NET_ERROR;

                    display_net_status(net_status, net_count);
                }else {
                    if(net_status == NET_CONFLICT)
                        break;

                    net_status = NET_ERROR;
                    net_count = -1;

                    display_net_status(net_status, net_count);
                }
            }
            break;

        case INFY_SET_IP_CONFLICT_NOTIFY:
        case INFY_CAM_IP_CONFLICT_NOTIFY:
            {
                NF_NOTIFY_INFO *pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);

                if( pInfo->d.params[0])
                {
                    net_count = -1;
                    net_status = NET_CONFLICT;
                    
                    display_net_status(net_status, net_count);
                    conflict_status = 1;
                }
                else
                {
                    conflict_status = 0;
                    net_count = scm_get_net_session_count();
                    if (net_count > 0) {
                        net_status = NET_CONNECTED;
                    }
                    else {
                        net_status = NET_ERROR;
                    }
                    display_net_status(net_status, net_count);
                }
            }
            break;

        case INFY_NET_RXTX:
            {
#if 0
                pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);
                if(pInfo != NULL)
                    g_message("%s ::::::::::::::: RECEIVE RXTX NOTIFY:::%d::", __FUNCTION__, pInfo->d.params[3]);
                if(pInfo != NULL) {
                    if(pInfo->d.params[3] == 0) net_status = NET_ERROR;
                    else                        net_status = NET_CONNECTED;
                    net_count = 0;

                    display_net_status(net_status, net_count);
                }
#endif
            }
            break;

        case GDK_DELETE:
            uxm_unreg_imsg_event(obj, INFY_NET_NOTIFY);
            uxm_unreg_imsg_event(obj, INFY_NET_RXTX);
            uxm_unreg_imsg_event(obj, INFY_SET_IP_CONFLICT_NOTIFY);
            uxm_unreg_imsg_event(obj, INFY_CAM_IP_CONFLICT_NOTIFY);
        break;

        default:
            break;
    }

    return FALSE;
}

static gboolean post_disk_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    static gint disk_status = -1;
    static gint raid_status = 0;
    static gint disk_val = 0;
    NF_NOTIFY_INFO *pInfo = NULL;

    // check init status
    if(disk_status < 0) {
        NF_NOTIFY_INFO info;
        NF_NOTIFY_INFO r_info;

        memset(&info, 0, sizeof(NF_NOTIFY_INFO));
        memset(&r_info, 0, sizeof(NF_NOTIFY_INFO));
        disk_status = 0;

        if(!scm_get_smart_event_data(&info)) {              // disk smart error
            if(info.d.params[0])
                disk_status |= (1 << DISK_NEED_CHECK);
        }

        if(!scm_get_smart_reqchk_event_data(&info)) {       // disk smart warning
            if(info.d.params[0])
                disk_status |= (1 << DISK_NEED_CHECK);
        }

        if(!scm_get_disk_raid_event_data(&r_info)){
            raid_status = r_info.d.params[0];
        }

        if (DAL_get_disk_write_mode())
            disk_status |= (1 << DISK_OVERWRITE);
    }

    switch(evt->type) {
        case GDK_EXPOSE:
        case GDK_BUTTON_PRESS:
        case GDK_ENTER_NOTIFY:
        case GDK_LEAVE_NOTIFY:
        case NFEVENT_KEYPAD_PRESS:
        case NFEVENT_KEYPAD_RELEASE:
        case NFEVENT_REMOCON_PRESS:
        case NFEVENT_REMOCON_RELEASE:
            // full mode
            if(get_live_mode() == 0)
                reset_on_time_timer();

            if(!nfui_nfobject_is_shown(g_ctrl_win))
                return FALSE;

            display_disk_status(disk_status, disk_val, raid_status);

            break;

        case GDK_BUTTON_RELEASE:
            {
                display_disk_status(disk_status, disk_val, raid_status);

                if(evt->button.button == MOUSE_RIGTH_BUTTON)
                    return FALSE;

                if(!nfui_nfobject_is_shown(g_ctrl_win))
                    return FALSE;

                VW_Create_DiskStatus(g_curwnd);
            }
            break;

        case INFY_DISK_USAGE_NOTIFY:
            {
                pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);
                disk_status |= (1 << DISK_NORMAL);
                disk_val = (((gfloat)pInfo->d.params[0]/(gfloat)pInfo->d.params[1]) * 100);
                display_disk_status(disk_status, disk_val, raid_status);
            }
            break;

        case INFY_DISK_FULL_NOTIFY:
            {
                pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);

                if(pInfo->d.params[0])
                    disk_status |= (1 << DISK_FULL);
                else
                    disk_status &= ~(1 << DISK_FULL);

                display_disk_status(disk_status, 100, raid_status);
            }
            break;

        case INFY_DISK_OW_NOTIFY:
            {
                pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);

                if (DAL_get_disk_write_mode())
                {
                    disk_status |= (1 << DISK_OVERWRITE);
                    display_disk_status(disk_status, disk_val, raid_status);
                }
                else
                {
                    disk_status &= ~(1 << DISK_OVERWRITE);
                    display_disk_status(disk_status, disk_val, raid_status);
                }
            }
            break;

        case INFY_SMART_WARN_NOTIFY:
            {
                pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);

                if(pInfo->d.params[0])
                {
                    disk_status |= (1 << DISK_NEED_CHECK);
                }
                else
                {
                    disk_status &= ~(1 << DISK_NEED_CHECK);
                }
                display_disk_status(disk_status, disk_val, raid_status);
            }
            break;

        case INFY_SMART_ERROR_NOTIFY:
            {
                pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);

                if(pInfo->d.params[0])
                {
                    disk_status |= (1 << DISK_SMART_ERR);
                }
                else
                {
                    disk_status &= ~(1 << DISK_SMART_ERR);
                }
                display_disk_status(disk_status, disk_val, raid_status);
            }
            break;

        case INFY_DISK_RAID_CHANGE_STATUS_NOTIFY:
            {
                pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);

                if (pInfo->d.params[0] >= RST_NORMAL && pInfo->d.params[0] <= RST_BROKEN){
                    raid_status = pInfo->d.params[0];
                    display_disk_status(disk_status, disk_val, raid_status);
                }
            }
            break;

        case INFY_NODISK_NOTIFY:
            {
                pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);

                if(pInfo->d.params[0])
                {
                    disk_status |= (1 << DISK_NO);
                    display_disk_status(disk_status, disk_val, raid_status);
                }
            }
            break;

        case INFY_DSKDB_CHANGE_NOTIFY:
            {
                scm_req_disk_ow_event_data();
            }
            break;

        case GDK_DELETE:
            uxm_unreg_imsg_event(obj, INFY_DISK_USAGE_NOTIFY);
            uxm_unreg_imsg_event(obj, INFY_DISK_FULL_NOTIFY);
            uxm_unreg_imsg_event(obj, INFY_SMART_WARN_NOTIFY);
            uxm_unreg_imsg_event(obj, INFY_SMART_ERROR_NOTIFY);
            uxm_unreg_imsg_event(obj, INFY_DISK_OW_NOTIFY);
            uxm_unreg_imsg_event(obj, INFY_NODISK_NOTIFY);
            uxm_unreg_imsg_event(obj, INFY_DSKDB_CHANGE_NOTIFY);
            uxm_unreg_imsg_event(obj, INFY_DISK_RAID_CHANGE_STATUS_NOTIFY);
            break;

        default:
            break;
    }

    return FALSE;
}

static gboolean _live_start_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        // full mode
        if(get_live_mode() == 0)
            reset_on_time_timer();

        if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        if(nfui_nfobject_is_shown(g_ctrl_win))
            LiveStart_Popup_Refresh();

    }
    else if(evt->type == GDK_BUTTON_PRESS || evt->type == GDK_MOTION_NOTIFY)
    {
        // full mode
        if(get_live_mode() == 0)
            reset_on_time_timer();
    }

    return FALSE;
}

static gboolean _change_pw_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gchar new_passwd[STRING_SIZE_16+1];

    if(evt->type == GDK_BUTTON_RELEASE)
    {
        gint id_idx;
        gint i, cnt;

        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        cnt = DAL_get_user_count();

        for (i = 0; i < cnt; i++)
        {
            memset(&userdata, 0x00, sizeof(UserManageData));
            DAL_get_userManage_data(&userdata, i);
            if (strcmp(userdata.id, nfui_nfbutton_get_text((NFBUTTON*)g_user_btn)) == 0) break;
        }

        if (i == cnt) return FALSE;

        id_idx = i;
        _proc_pw_change(id_idx, new_passwd);
    }

    return FALSE;
}


static gboolean _live_full_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		static gint full = 0;

		if(evt->button.button == MOUSE_RIGTH_BUTTON)
        {
            return FALSE;
        }

        if(!nfui_nfobject_is_shown(g_ctrl_win))
            return FALSE;

        if(full ^= 1)
        {
            live_normal_mode();

            nfui_nfbutton_set_image((NFBUTTON*)obj, g_div_img[7]);
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
        }
        else
        {
            live_full_mode();

            nfui_nfbutton_set_image((NFBUTTON*)obj, g_div_img[0]);
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

            g_ls_d.tml_disp_mode = DAL_get_live_timeline_on_mode();
            if (g_ls_d.tml_disp_mode == TLINE_ALWAYS_ON) VW_Timeline_Show();
            else VW_Timeline_Hide();
        }
	}
	else if(evt->type == GDK_BUTTON_PRESS
			|| evt->type == GDK_MOTION_NOTIFY)
	{
		// full mode
		if(get_live_mode() == 0)
			reset_on_time_timer();
	}

    return FALSE;
}

static gboolean _live_div1_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        guint ch;

        // full mode
        if(get_live_mode() == 0)
            reset_on_time_timer();

        if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        if(!nfui_nfobject_is_shown(g_ctrl_win))
            return FALSE;

		vsm_change_sfc_cstlayout_next(VSM_DIV1);
		var_set_active_layout(-1);
		
	}
	else if(evt->type == GDK_BUTTON_PRESS
			|| evt->type == GDK_MOTION_NOTIFY)
	{
		// full mode
		if(get_live_mode() == 0)
			reset_on_time_timer();
	}

    return FALSE;
}

static gboolean _live_div4_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        // full mode
        if(get_live_mode() == 0)
            reset_on_time_timer();

        if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        if(!nfui_nfobject_is_shown(g_ctrl_win))
            return FALSE;

		vsm_change_sfc_cstlayout_next(VSM_DIV4);
		var_set_active_layout(-1);
	}
	else if(evt->type == GDK_BUTTON_PRESS
			|| evt->type == GDK_MOTION_NOTIFY)
	{
		// full mode
		if(get_live_mode() == 0)
			reset_on_time_timer();
	}

    return FALSE;
}

static gboolean _live_div9_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        // full mode
        if(get_live_mode() == 0)
            reset_on_time_timer();

        if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        if(!nfui_nfobject_is_shown(g_ctrl_win))
            return FALSE;

		vsm_change_sfc_cstlayout_next(VSM_DIV9);
		var_set_active_layout(-1);
	}
	else if(evt->type == GDK_BUTTON_PRESS
			|| evt->type == GDK_MOTION_NOTIFY)
	{
		// full mode
		if(get_live_mode() == 0)
			reset_on_time_timer();
	}

    return FALSE;
}

static gboolean _live_div16_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        // full mode
        if(get_live_mode() == 0)
            reset_on_time_timer();

        if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        if(!nfui_nfobject_is_shown(g_ctrl_win))
            return FALSE;

		vsm_change_sfc_cstlayout_next(VSM_DIV16);
		var_set_active_layout(-1);
	}
	else if(evt->type == GDK_BUTTON_PRESS
			|| evt->type == GDK_MOTION_NOTIFY)
	{
		// full mode
		if(get_live_mode() == 0)
			reset_on_time_timer();
	}

    return FALSE;
}

static gboolean _live_div36_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        // full mode
        if(get_live_mode() == 0)
            reset_on_time_timer();

        if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        if(!nfui_nfobject_is_shown(g_ctrl_win))
            return FALSE;

		vsm_change_sfc_cstlayout_next(VSM_DIV36);
		var_set_active_layout(-1);
	}
	else if(evt->type == GDK_BUTTON_PRESS
			|| evt->type == GDK_MOTION_NOTIFY)
	{
		// full mode
		if(get_live_mode() == 0)
			reset_on_time_timer();
	}

    return FALSE;
}

static gboolean _live_seq_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        guint pos_x = obj->x;

        // full mode
        if(get_live_mode() == 0)
            reset_on_time_timer();

        if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        if(!nfui_nfobject_is_shown(g_ctrl_win))
            return FALSE;

#if (defined(GUI_4CH_SUPPORT) || defined(_NOT_SUPPORT_SPC_DIV)) && !defined(_SUPPORT_USER_DIV)
        vsm_live_cmd_sequence();
#else
        VW_Create_Div_Popup_Create(g_curwnd, pos_x);
#endif

		// full mode
		if(get_live_mode() == 0) {
			reset_on_time_timer();
		}
	}
	else if(evt->type == GDK_BUTTON_PRESS
			|| evt->type == GDK_MOTION_NOTIFY)
	{
		// full mode
		if(get_live_mode() == 0)
			reset_on_time_timer();
	}

    return FALSE;
}

static gboolean _live_osd_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{

    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
		gboolean ret = FALSE;
        if(!nfui_nfobject_is_shown(g_ctrl_win))
            return FALSE;

        ret = nfui_check_button_get_active((NFCHECKBUTTON*)obj);

		if(!ret)	vsm_osd_on();
		else   		vsm_osd_off();
	}
	else if(evt->type == GDK_BUTTON_PRESS
			|| evt->type == GDK_BUTTON_RELEASE
			|| evt->type == GDK_MOTION_NOTIFY)
	{
		// full mode
		if(get_live_mode() == 0)
			reset_on_time_timer();
	}

    return FALSE;
}

static gint _is_user_logout()
{
    gchar user[32];

    memset(user, 0x00, sizeof(user));
    ssm_get_cur_id(user);

    if (!strlen(user)) return 1;

    return 0;
}

static gboolean ctrl_win_post_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
    switch(event->type)
    {
        case GDK_EXPOSE:
            break;

        // TODO:handling key events
        case NFEVENT_KEYPAD_PRESS:
        case NFEVENT_REMOCON_PRESS:
            {
                GdkEventKey *kevt;
                KEYPAD_KID kpid;
                NFWINDOW *fnd;
                gint relay_act = -1;

                kevt = (GdkEventKey*)event;
                kpid = (KEYPAD_KID)kevt->keyval;

                if ((fnd = wnd_find_window("LIVE SHORTCUT SUB PB"))) {
                    nfui_nfobject_hide((NFOBJECT *)fnd);
                }

                if ((fnd = wnd_find_window("LIVE SHORTCUT"))) {
                    nfui_nfobject_hide((NFOBJECT *)fnd);
                }

                if (_find_relay_event(kpid, &relay_act) == 0)
                {
                    if ((relay_act == 0) || (relay_act == 1))
                        _set_relay_out(kpid, relay_act);

                    return FALSE;
                }

                if (GET_CHANNEL_KEY_SCOPE(kpid) || (kpid == KEYPAD_DISP))
                {
#ifdef ENABLE_IPCAM_ZIG
                    if(vsm_get_div() == VSM_DIV1)
                        vsm_ipcam_zig_event_press(kpid);
                    else
                        vsm_keypad_event(kpid);
#else
                    if (_is_user_logout()) return FALSE;

                    vsm_keypad_event(kpid);
#endif
                    var_set_active_layout(-1);
                }
                else
                {
                    if (ssm_get_cur_idx() < 0)
                    {
                        if (kpid != KEYPAD_RIGHT && kpid != KEYPAD_DOWN && kpid != KEYPAD_EXIT && kpid != RMC_MENU) break;
                    }

#ifdef ENABLE_IPCAM_ZIG
                    vsm_ipcam_zig_event_press(kpid);
                    return FALSE;
#endif
                    switch(kpid) {
                        case KEYPAD_POWER:
                        {
                            //NFOBJECT *wait_pop = NULL;

                            if(!ssm_check_access_auth(USR_AUTH_SHUTDOWN))
                                break;

							if(VW_UserPwd_Open(g_curwnd, "SHUTDOWN", -1))
                            {
                                smt_set_service(SMT_SHUTDOWN);

                                scm_shutdown_system(IRET_SCM_SHUTDOWN2);
                                scm_notify_to_system("dvr_status", NF_DVR_STATUS_SHUTDOWN);

                                wait_pop = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
                                //g_timeout_add(100, ready_shutdown, (gpointer)wait_pop);
                            }
                        }
                        break;

                        case KEYPAD_LOCK:
                            {
								VW_UserPwd_Open(g_curwnd, "KEY LOCK", -1);
                            }
                            break;

                        case KEYPAD_SEQ:
                            {
                                vsm_live_cmd_sequence();
                            }
                            break;

                        case KEYPAD_SETUP:
                            {
                                if(!ssm_check_access_auth(USR_AUTH_SYS_SETUP))
                                    break;

                                cleanup_new_opening(NULL);

                                VW_MainMenu_Open(g_curwnd);
                                VW_Timeline_ChangeMode(TL_LIVE);
                            }
                            break;

/*
                        case KEYPAD_PANIC:
                            {
                                gboolean is_panic;

                                is_panic = nfui_check_button_get_active((NFCHECKBUTTON*)g_menuObj[PANIC_BTN]);

                                if (is_panic)   nfui_check_button_set_active((NFCHECKBUTTON*)g_menuObj[PANIC_BTN], FALSE);
                                else            nfui_check_button_set_active((NFCHECKBUTTON*)g_menuObj[PANIC_BTN], TRUE);

                                scm_toggle_panic_record();
                            }
                            break;
*/
                        case KEYPAD_PTZ:
                            {
                                guint ch;
                                gint x, y;

                                if (!ssm_check_access_auth(USR_AUTH_PTZ)) break;

                                ch = vsm_get_focused_channel();
                                if(vsm_get_covert_state(NULL, ch)) {
                                    nftool_mbox(g_curwnd, "ERROR", "Current channel is coverted.",  NFTOOL_MB_OK);
                                    return FALSE;
                                }

                                hide_live();

                                if (mcf.sys_sub1.menu_pos[SYS_SUB1_PTZ_SETUP] != -1)
                                    VW_Live_Ptz_Main_Open(g_curwnd, ch);

                                if (vsm_get_omode() == OMODE_NORMAL)
                                {
                                    show_live();
                                }

                            }
                            break;

                        case KEYPAD_ZOOM:
                            {
                                guint ch;
                                gint x, y;

                                ch = vsm_get_focused_channel();
                                if(vsm_get_covert_state(NULL, ch)) {
                                    nftool_mbox(g_curwnd, "ERROR", "Current channel is coverted.",  NFTOOL_MB_OK);
                                    return FALSE;
                                }

                                hide_live();

                                x = DISPLAY_ACTIVE_WIDTH - VW_ZoomPIP_Width();
                                y = DISPLAY_ACTIVE_HEIGHT - VW_ZoomPIP_Height();

                                VW_ZoomPIP_Open(g_curwnd, x, y, ch, ZOOM_PIP_NONE);

                                if (vsm_get_omode() == OMODE_NORMAL)
                                {
                                    show_live();
                                }
                            }
                            break;

                        case RMC_SNAPSHOT:
                            {
                                NF_NOTIFY_INFO info;
                                guint ch;
                                gint retVal;

                                if(!DAL_get_support_snapshot()) return FALSE;

                                ch = vsm_get_focused_channel();
                                scm_get_vloss_data(&info);

                                if (info.d.params[0] & (1 << ch))
                                {
                                    nftool_mbox(g_curwnd, "NOTICE", "SNAPSHOT FAIL", NFTOOL_MB_OK);
                                    return FALSE;
                                }

                                if (vsm_get_covert_state(NULL, ch))
                                {
                                    nftool_mbox(g_curwnd, "ERROR", "Current channel is coverted.", NFTOOL_MB_OK);
                                    return FALSE;
                                }

                                retVal = scm_req_live_capture(INFY_CAPTURE_IMAGE, ch);
								if (retVal == 0)
								{
									wait_pop = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
									uxm_reg_imsg_event(g_ctrl_win, INFY_CAPTURE_IMAGE);
									uxm_monitor_on_imsg_event(g_ctrl_win, INFY_CAPTURE_IMAGE);
								}
								else
								{
									nftool_mbox(g_curwnd, "NOTICE", "SNAPSHOT FAIL", NFTOOL_MB_OK);
								} 
                            }
                            break;


                        case KEYPAD_ARCH:
                            {
                                SecurityData secdata;
								gboolean use_dl = 0;

                                if(!ssm_check_access_auth(USR_AUTH_ARCHIVE))
                                    break;

                                DAL_get_use_double_login(&use_dl);

                                if (use_dl && !ssm_is_admin())
                                {
                                    if (!VW_UserPwd_Open(NF_TOPWND, "DOUBLE LOGIN", -1)) return FALSE;
                                    if (!VW_UserPwd_Open(NF_TOPWND, "DOUBLE LOGIN2", USR_AUTH_ARCHIVE)) return FALSE;
                                }
                                else
                                {
                                DAL_get_security_data(&secdata);
                            		if (secdata.loginSearchArch && !VW_UserPwd_Open(g_curwnd, "PASSWORD CHECK", -1)) return FALSE;
                                }

                                cleanup_new_opening(NULL);

                                stm_set_time_by_sys();
                                VW_Archiving_Open(g_curwnd, vsm_create_livestart_obj(), 0);
                            }
                            break;

                        case KEYPAD_SEARCH:
                            {
                                time_t cur_time;
                                SecurityData secdata;
								gboolean use_dl = 0;

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

                                cleanup_new_opening(NULL);

                                time(&cur_time);
                                stm_set_time_t(cur_time-360);
                                stm_set_endtime_t(cur_time-60);

                                VW_Search_Open(g_curwnd, 0, vsm_create_livestart_obj());
                            }
                            break;

                        case KEYPAD_LEFT:
                            {
                                // full mode
                                if(nfui_nfobject_is_shown(obj)) {
                                    if(get_live_mode() == 0)
                                        reset_on_time_timer();
                                }
                            }
                            break;

                        case KEYPAD_RIGHT:
                            {
                                // full mode
                                if(nfui_nfobject_is_shown(obj)) {
                                    if(get_live_mode() == 0)
                                        reset_on_time_timer();
                                }

                                if(!nfui_nfobject_is_shown(obj)) {
                                    // auto hide
                                    if(VW_Timeline_get_disp_mode() == TLINE_AUTO_HIDE) {
                                        if(!VW_Timeline_IsShown())
                                            VW_Timeline_Show();
                                    }
                                }
                            }
                            break;

                        case KEYPAD_UP:
                            {
                                // full mode
                                if(nfui_nfobject_is_shown(obj)) {
                                    if(get_live_mode() == 0)
                                        reset_on_time_timer();
                                }
                            }
                            break;

                        case KEYPAD_DOWN:
                            {
                                // full mode
                                if(nfui_nfobject_is_shown(obj)) {
                                    if(get_live_mode() == 0)
                                        reset_on_time_timer();
                                }

                                if(!nfui_nfobject_is_shown(obj))
                                    nfui_nfobject_show(obj);
                            }
                            break;

                        case KEYPAD_ENTER:
                            {
                                // full mode
                                if(nfui_nfobject_is_shown(obj)) {
                                    if(get_live_mode() == 0)
                                        reset_on_time_timer();
                                }
                            }
                            break;

                        case KEYPAD_EXIT:
                            {
                                // full mode
                                if(get_live_mode() == 0) {
                                    // auto hide
                                    if(get_on_time_mode() == AUTO_HIDE_MODE) {
                                        if(nfui_nfobject_is_shown(obj))
                                            nfui_nfobject_hide(obj);
                                    }

                                    // auto hide
                                    if(VW_Timeline_get_disp_mode() == TLINE_AUTO_HIDE) {
                                        if(VW_Timeline_IsShown())
                                            VW_Timeline_Hide();
                                    }
                                }
                                VW_Hide_Image_Popup();
                            }
                            break;

                        case RMC_DEV_ID:
                            {
                                RmCtrlID_Open(g_curwnd);
                            }
                            break;

                        case RMC_LOGOUT:
                            {
                                scm_put_log(CLOSE_LIVE, 0, 0);
                                ssm_logout();

                                VW_Live_StatusBar_Menu_Disable();
                                VW_Live_StatusBar_Set_User("");
                                LiveStart_Popup_Menu_Disable();

                                vsm_set_covert_by_logout();
                                smt_set_service(SMT_LOGOUT);
                            }
                            break;

                        case RMC_LOG:
                            {
                                // full mode
                                if(get_live_mode() == 0)
                                    reset_on_time_timer();

                                if(!nfui_nfobject_is_shown(g_ctrl_win))
                                    nfui_nfobject_show(g_ctrl_win);

                                if(VW_EventLog_IsShown()) VW_EventLog_Hide();
                                else                      VW_EventLog_Show();
                            }
                            break;

                        case RMC_ALARM:
                            {
                                // full mode
                                if(get_live_mode() == 0)
                                    reset_on_time_timer();

                                if(!nfui_nfobject_is_shown(g_ctrl_win))
                                    nfui_nfobject_show(g_ctrl_win);

                                VW_AlarmStatus_Show();
                            }
                            break;

                        case RMC_MENU:
                            {
                                // full mode
                                if(get_live_mode() == 0)
                                    reset_on_time_timer();

                                if(!nfui_nfobject_is_shown(g_ctrl_win))
                                    nfui_nfobject_show(g_ctrl_win);

                                LiveStart_Popup_Refresh();
                            }
                            break;

                        case RMC_AUDIO:
                            {
                                if (!DAL_get_support_audio())
                                    break;

                                if(!ssm_check_access_auth(USR_AUTH_AUDIO))
                                    break;

                                // full mode
                                if(get_live_mode() == 0)
                                    reset_on_time_timer();

                                if(!nfui_nfobject_is_shown(g_ctrl_win))
                                    nfui_nfobject_show(g_ctrl_win);

                                if(VW_AudioInput_IsShown()) VW_AudioInput_Hide();
                                else                        VW_AudioInput_Show();
                            }
                            break;

                        default:
                            break;
                    }
                }
            }
            break;

        case NFEVENT_KEYPAD_RELEASE:
        case NFEVENT_REMOCON_RELEASE:
            {
                GdkEventKey *kevt;
                KEYPAD_KID kpid;

                kevt = (GdkEventKey*)event;
                kpid = (KEYPAD_KID)kevt->keyval;

#ifdef ENABLE_IPCAM_ZIG
                vsm_ipcam_zig_event_release(kpid);
                return FALSE;
#endif
            }
            break;

        case INFY_DSPDB_CHANGE_NOTIFY:
            {
                // change statusbar on mode /////////////////////////////////
                g_ls_d.on_time = DAL_get_live_status_on_time();

                if (vsm_get_omode() == OMODE_FULL) {
                    if(get_on_time_mode())
                        reset_on_time_timer();

                    // time on mode
                    if(get_on_time_mode() > AUTO_HIDE_MODE)
                        nfui_nfobject_hide(g_ctrl_win);

                    // other mode
                    if(get_on_time_mode() == ALWAYS_ON_MODE || get_on_time_mode() == AUTO_HIDE_MODE) {
                        if(!g_ls_d.timer_id)
                            g_source_remove(g_ls_d.timer_id);

                        g_ls_d.timer_id = g_timeout_add(1000, hide_statusbar, NULL);
                    }

                }
            }
            break;

        case IRET_SCM_SHUTDOWN2:
            {
                if (wait_pop)
                {
                    nftool_remove_waitbox(wait_pop);
                }
                nftool_mbox_wait(g_curwnd, "NOTICE", "You can power off the system.");
            }
            break;

        case GDK_DELETE:
            {
                destroy_menu_viewer();
                uxm_unreg_imsg_event(obj, IRET_SCM_SHUTDOWN2);
                uxm_unreg_imsg_event(obj, INFY_DSPDB_CHANGE_NOTIFY);
            }
            break;

        case INFY_CAPTURE_IMAGE:
            {
                gint result = ((CMM_MESSAGE_T *)data)->param;
                uxm_unreg_imsg_event(g_ctrl_win, INFY_CAPTURE_IMAGE);

                if (wait_pop)
                {
                    nftool_remove_waitbox(wait_pop);
                    wait_pop = NULL;
                }

                if (result == 0)
                {
                    CAPTURE_IMAGE_T *image = ((CMM_MESSAGE_T *)data)->data;
                    SNAPSHOT_INFO_T info;

                    info.ch = image->ch;
                    info.time = image->time;
                    info.size = image->size;
                    info.width = image->width;
                    info.height = image->height;
                    info.buffer = image->buffer;

                    VW_Snapshot_Open(g_curwnd, &info, SS_MODE_BURN_RESERVE);
                }else
                {
                    nftool_mbox(g_curwnd, "NOTICE", "SNAPSHOT FAIL", NFTOOL_MB_OK);
                }

                return FALSE;
            }
            break;

        default:
            break;
    }

    return FALSE;
}

static void change_menu_status_by_usr_auth()
{
    if((!ssm_check_access_auth(USR_AUTH_AUDIO)) || (!DAL_get_support_audio()))
    {
        nfui_nfobject_disable(g_menuObj[AUDIO_BTN]);
    }
    else
    {
        nfui_nfobject_enable(g_menuObj[AUDIO_BTN]);
    }

    nfui_signal_emit(g_menuObj[AUDIO_BTN], GDK_EXPOSE, FALSE);

#if !defined(_HDI_MODEL_UX) && !defined(_DVR_MODEL_UX)
    if((!ssm_check_access_auth(USR_AUTH_MIC)) || (!DAL_get_support_audio()) || (nf_sysman_get_pba_type() == NF_SYSMAN_PBA_TYPE_B))
    {
        nfui_nfobject_disable(g_menuObj[MIC_BTN]);
    }
    else
    {
        nfui_nfobject_enable(g_menuObj[MIC_BTN]);
    }

    nfui_signal_emit(g_menuObj[MIC_BTN], GDK_EXPOSE, FALSE);
#endif

    if(!ssm_check_access_auth(USR_AUTH_PTZ))
    {
        nfui_nfobject_disable(g_menuObj[PTZ_BTN]);
    }
    else
    {
        nfui_nfobject_enable(g_menuObj[PTZ_BTN]);
    }

    nfui_signal_emit(g_menuObj[PTZ_BTN], GDK_EXPOSE, FALSE);
}

static gboolean ctrl_fixed_post_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
    switch(event->type)
    {
        case GDK_EXPOSE:
            {
                GdkDrawable *drawable = NULL;
                GdkGC* gc = NULL;

                drawable = nfui_nfobject_get_window(obj);
                gc = nfui_nfobject_get_gc(obj);

                nfutil_draw_image(drawable, gc, IMG_STATUSBAR_BG,
                    (gint)(obj->x), (gint)(obj->y), (gint)(obj->width), (gint)(obj->height), NFALIGN_LEFT, 0);

                update_user_label();

                nfui_nfobject_gc_unref(gc);

                change_menu_status_by_usr_auth();

                // full mode
                if(get_live_mode() == 0) {
                    start_on_time_timer();
                }
            }
            break;

        case GDK_BUTTON_PRESS:
        case GDK_BUTTON_RELEASE:
        case GDK_MOTION_NOTIFY:
            {
                // full mode
                if(get_live_mode() == 0)
                    reset_on_time_timer();
            }
            break;

        case INFY_USRDB_CHANGE_NOTIFY:
        case INFY_FACTORY_DEFAULT_NOTIFY:
        case INFY_SYSTEM_DATA_LOAD_NOTIFY:
            {
                update_user_label();
                change_menu_status_by_usr_auth();
            }
            break;

        case GDK_DELETE:
            {
                g_source_remove(t_timer_id);
                nfui_page_close(PGID_LIVECTRLBAR, obj);

                g_timer_destroy(g_ls_d.timer);
                uxm_unreg_imsg_event(obj, INFY_USRDB_CHANGE_NOTIFY);
                uxm_unreg_imsg_event(obj, INFY_FACTORY_DEFAULT_NOTIFY);
                uxm_unreg_imsg_event(obj, INFY_SYSTEM_DATA_LOAD_NOTIFY);
            }
            break;

        default:
            break;
    }

    return FALSE;
}


static gboolean start_btn_post_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
    // for test
//  if(event->type == GDK_BUTTON_PRESS)
//      SetUpMenu_Open();

    return FALSE;
}

static gboolean _qc_test_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        if(!nfui_nfobject_is_shown(g_ctrl_win))
            return FALSE;

        if (vw_qc_test_is_shown_new()) vw_qc_test_hide_new();
        else                       vw_qc_test_show_new();
    }

    return FALSE;
}

static gint _make_admin_button_image(GdkPixbuf **pbuf, gint x, gint y, gint size_w, gint size_h)
{
    gchar btn_name_n[32];
    gchar btn_name_o[32];
    gchar btn_name_p[32];
    gchar btn_name_d[32];

    GdkPixbuf *bar_pbuf;
    GdkPixbuf *icon_pbuf;

    g_sprintf(btn_name_n, "MKB_IMG_ADMIN_BTN1_N_%d", size_w);
    g_sprintf(btn_name_o, "MKB_IMG_ADMIN_BTN1_O_%d", size_w);
    g_sprintf(btn_name_p, "MKB_IMG_ADMIN_BTN1_P_%d", size_w);
    g_sprintf(btn_name_d, "MKB_IMG_ADMIN_BTN1_D_%d", size_w);

    nf_ui_create_image_button_method(btn_name_n, size_w, IMG_BTN1_N_L, IMG_BTN1_N_M, IMG_BTN1_N_R);
    nf_ui_create_image_button_method(btn_name_o, size_w, IMG_BTN1_O_L, IMG_BTN1_O_M, IMG_BTN1_O_R);
    nf_ui_create_image_button_method(btn_name_p, size_w, IMG_BTN1_P_L, IMG_BTN1_P_M, IMG_BTN1_P_R);
    nf_ui_create_image_button_method(btn_name_d, size_w, IMG_BTN1_N_L, IMG_BTN1_N_M, IMG_BTN1_N_R);

    pbuf[0] = nfui_get_image_from_memory(btn_name_n);
    pbuf[1] = nfui_get_image_from_memory(btn_name_o);
    pbuf[2] = nfui_get_image_from_memory(btn_name_p);
    pbuf[3] = nfui_get_image_from_memory(btn_name_d);

    bar_pbuf = nfui_get_image_from_file(IMG_STATUSBAR_BG, NULL);
    gdk_pixbuf_copy_area(bar_pbuf, x, y, size_w, size_h, pbuf[0], 0, 0);

    icon_pbuf = nfui_get_image_from_file(IMG_ADMIN_STATUS_N, NULL);
    gdk_pixbuf_copy_area(icon_pbuf, 0, 0, 35, 35, pbuf[0], 2, 5);

    icon_pbuf = nfui_get_image_from_file(IMG_ADMIN_STATUS_O, NULL);
    gdk_pixbuf_copy_area(icon_pbuf, 0, 0, 35, 35, pbuf[1], 2, 5);

    icon_pbuf = nfui_get_image_from_file(IMG_ADMIN_STATUS_P, NULL);
    gdk_pixbuf_copy_area(icon_pbuf, 0, 0, 35, 35, pbuf[2], 2, 5);

    bar_pbuf = nfui_get_image_from_file(IMG_STATUSBAR_BG, NULL);
    gdk_pixbuf_copy_area(bar_pbuf, x, y, size_w, size_h, pbuf[3], 0, 0);

    icon_pbuf = nfui_get_image_from_file(IMG_ADMIN_STATUS_N, NULL);
    gdk_pixbuf_copy_area(icon_pbuf, 0, 0, 35, 35, pbuf[3], 2, 5);

    return 0;
}

NFOBJECT* VW_Live_StatusBar_Open(NFWINDOW *parent)
{
    NFOBJECT *ctrl_fixed1;
    NFOBJECT *nfwidget;

    NF_NOTIFY_INFO info;

    gint size_w, size_h;
    guint pos_x = 0;
    guint pwr_btn_w, pwr_btn_h;
    guint main_btn_w, main_btn_h;
    gint i, j;
    gboolean is_panic;

    time_t init_time;
    struct tm time_info;

    GdkPixbuf *start_img[NFOBJECT_STATE_COUNT];
    GdkPixbuf *user_img[NFOBJECT_STATE_COUNT];
    GdkPixbuf *osd_inact_img[4];
    GdkPixbuf *osd_act_img[4];
    GdkPixbuf *ptz_img[4];
    GdkPixbuf *zoom_img[NFOBJECT_STATE_COUNT];
    GdkPixbuf *log_img[NFOBJECT_STATE_COUNT];
    GdkPixbuf *audin_img[NFOBJECT_STATE_COUNT];
    GdkPixbuf *audout_img[NFOBJECT_STATE_COUNT];
    GdkPixbuf *panic_inact_img[4];
    GdkPixbuf *panic_act_img[4];

    GdkPixbuf *pbBG = NULL;
    GdkPixbuf *bgImg;

    guint btn_fg[NFOBJECT_STATE_COUNT] = {COLOR_IDX(324), COLOR_IDX(325), COLOR_IDX(327), COLOR_IDX(328)};
    guint btn_fg1[NFOBJECT_STATE_COUNT] = {COLOR_IDX(329), COLOR_IDX(101), COLOR_IDX(103), COLOR_IDX(104)};
    guint btn_fg2[NFOBJECT_STATE_COUNT] = {COLOR_IDX(329), COLOR_IDX(101), COLOR_IDX(103), COLOR_IDX(329)};

    /* load images */
    start_img[0] = nfui_get_image_from_file((IMG_BTN_N_START), NULL);
    start_img[1] = nfui_get_image_from_file((IMG_BTN_O_START), NULL);
    start_img[2] = nfui_get_image_from_file((IMG_BTN_P_START), NULL);
    start_img[3] = nfui_get_image_from_file((IMG_BTN_D_START), NULL);
#if 1
    g_div_img[0][0] = nfui_get_image_from_file((IMG_N_DIV_NORMAL), NULL);
    g_div_img[0][1] = nfui_get_image_from_file((IMG_O_DIV_NORMAL), NULL);
    g_div_img[0][2] = nfui_get_image_from_file((IMG_P_DIV_NORMAL), NULL);
    g_div_img[0][3] = nfui_get_image_from_file((IMG_D_DIV_NORMAL), NULL);

    g_div_img[1][0] = nfui_get_image_from_file((IMG_N_DIV_1), NULL);
    g_div_img[1][1] = nfui_get_image_from_file((IMG_O_DIV_1), NULL);
    g_div_img[1][2] = nfui_get_image_from_file((IMG_P_DIV_1), NULL);
    g_div_img[1][3] = nfui_get_image_from_file((IMG_D_DIV_1), NULL);

    g_div_img[2][0] = nfui_get_image_from_file((IMG_N_DIV_4), NULL);
    g_div_img[2][1] = nfui_get_image_from_file((IMG_O_DIV_4), NULL);
    g_div_img[2][2] = nfui_get_image_from_file((IMG_P_DIV_4), NULL);
    g_div_img[2][3] = nfui_get_image_from_file((IMG_D_DIV_4), NULL);

    g_div_img[3][0] = nfui_get_image_from_file((IMG_N_DIV_9), NULL);
    g_div_img[3][1] = nfui_get_image_from_file((IMG_O_DIV_9), NULL);
    g_div_img[3][2] = nfui_get_image_from_file((IMG_P_DIV_9), NULL);
    g_div_img[3][3] = nfui_get_image_from_file((IMG_D_DIV_9), NULL);

    g_div_img[4][0] = nfui_get_image_from_file((IMG_N_DIV_16), NULL);
    g_div_img[4][1] = nfui_get_image_from_file((IMG_O_DIV_16), NULL);
    g_div_img[4][2] = nfui_get_image_from_file((IMG_P_DIV_16), NULL);
    g_div_img[4][3] = nfui_get_image_from_file((IMG_D_DIV_16), NULL);

    g_div_img[5][0] = nfui_get_image_from_file((IMG_N_DIV_32), NULL);
    g_div_img[5][1] = nfui_get_image_from_file((IMG_O_DIV_32), NULL);
    g_div_img[5][2] = nfui_get_image_from_file((IMG_P_DIV_32), NULL);
    g_div_img[5][3] = nfui_get_image_from_file((IMG_D_DIV_32), NULL);

#if (defined(GUI_4CH_SUPPORT) || defined(_NOT_SUPPORT_SPC_DIV)) && !defined(_SUPPORT_USER_DIV)
    g_div_img[6][0] = nfui_get_image_from_file((IMG_N_POPUP_DIV_SEQ), NULL);
    g_div_img[6][1] = nfui_get_image_from_file((IMG_O_POPUP_DIV_SEQ), NULL);
    g_div_img[6][2] = nfui_get_image_from_file((IMG_P_POPUP_DIV_SEQ), NULL);
    g_div_img[6][3] = nfui_get_image_from_file((IMG_D_POPUP_DIV_SEQ), NULL);
#else
    g_div_img[6][0] = nfui_get_image_from_file((IMG_N_DIV_SEQ), NULL);
    g_div_img[6][1] = nfui_get_image_from_file((IMG_O_DIV_SEQ), NULL);
    g_div_img[6][2] = nfui_get_image_from_file((IMG_P_DIV_SEQ), NULL);
    g_div_img[6][3] = nfui_get_image_from_file((IMG_D_DIV_SEQ), NULL);
#endif

    g_div_img[7][0] = nfui_get_image_from_file((IMG_N_DIV_FULL), NULL);
    g_div_img[7][1] = nfui_get_image_from_file((IMG_O_DIV_FULL), NULL);
    g_div_img[7][2] = nfui_get_image_from_file((IMG_P_DIV_FULL), NULL);
    g_div_img[7][3] = nfui_get_image_from_file((IMG_D_DIV_FULL), NULL);
#else
    g_div_img[0][0] = nfui_get_image_from_file((IMG_N_BTN_PTZ), NULL);
    g_div_img[0][1] = nfui_get_image_from_file((IMG_P_BTN_PTZ), NULL);
    g_div_img[0][2] = nfui_get_image_from_file((IMG_O_BTN_PTZ), NULL);
    g_div_img[0][3] = nfui_get_image_from_file((IMG_D_BTN_PTZ), NULL);

    g_div_img[1][0] = nfui_get_image_from_file((IMG_N_BTN_PTZ), NULL);
    g_div_img[1][1] = nfui_get_image_from_file((IMG_P_BTN_PTZ), NULL);
    g_div_img[1][2] = nfui_get_image_from_file((IMG_O_BTN_PTZ), NULL);
    g_div_img[1][3] = nfui_get_image_from_file((IMG_D_BTN_PTZ), NULL);

    g_div_img[2][0] = nfui_get_image_from_file((IMG_N_BTN_PTZ), NULL);
    g_div_img[2][1] = nfui_get_image_from_file((IMG_P_BTN_PTZ), NULL);
    g_div_img[2][2] = nfui_get_image_from_file((IMG_O_BTN_PTZ), NULL);
    g_div_img[2][3] = nfui_get_image_from_file((IMG_D_BTN_PTZ), NULL);

    g_div_img[3][0] = nfui_get_image_from_file((IMG_N_BTN_PTZ), NULL);
    g_div_img[3][1] = nfui_get_image_from_file((IMG_P_BTN_PTZ), NULL);
    g_div_img[3][2] = nfui_get_image_from_file((IMG_O_BTN_PTZ), NULL);
    g_div_img[3][3] = nfui_get_image_from_file((IMG_D_BTN_PTZ), NULL);

    g_div_img[4][0] = nfui_get_image_from_file((IMG_N_BTN_PTZ), NULL);
    g_div_img[4][1] = nfui_get_image_from_file((IMG_P_BTN_PTZ), NULL);
    g_div_img[4][2] = nfui_get_image_from_file((IMG_O_BTN_PTZ), NULL);
    g_div_img[4][3] = nfui_get_image_from_file((IMG_D_BTN_PTZ), NULL);

    g_div_img[5][0] = nfui_get_image_from_file((IMG_N_BTN_PTZ), NULL);
    g_div_img[5][1] = nfui_get_image_from_file((IMG_P_BTN_PTZ), NULL);
    g_div_img[5][2] = nfui_get_image_from_file((IMG_O_BTN_PTZ), NULL);
    g_div_img[5][3] = nfui_get_image_from_file((IMG_D_BTN_PTZ), NULL);

    g_div_img[6][0] = nfui_get_image_from_file((IMG_N_DIV_NORMAL), NULL);
    g_div_img[6][1] = nfui_get_image_from_file((IMG_O_DIV_NORMAL), NULL);
    g_div_img[6][2] = nfui_get_image_from_file((IMG_P_DIV_NORMAL), NULL);
    g_div_img[6][3] = nfui_get_image_from_file((IMG_D_DIV_NORMAL), NULL);
#endif


    osd_inact_img[0] = nfui_get_image_from_file(IMG_N_OSD_OFF, NULL);
    osd_inact_img[1] = nfui_get_image_from_file(IMG_O_OSD_OFF, NULL);
    osd_inact_img[2] = nfui_get_image_from_file(IMG_P_OSD_OFF, NULL);
    osd_inact_img[3] = nfui_get_image_from_file(IMG_D_OSD_OFF, NULL);

    osd_act_img[0] = nfui_get_image_from_file(IMG_N_OSD_ON, NULL);
    osd_act_img[1] = nfui_get_image_from_file(IMG_O_OSD_ON, NULL);
    osd_act_img[2] = nfui_get_image_from_file(IMG_P_OSD_ON, NULL);
    osd_act_img[3] = nfui_get_image_from_file(IMG_D_OSD_ON, NULL);

    ptz_img[0] = nfui_get_image_from_file((IMG_N_BTN_PTZ), NULL);
    ptz_img[1] = nfui_get_image_from_file((IMG_O_BTN_PTZ), NULL);
    ptz_img[2] = nfui_get_image_from_file((IMG_P_BTN_PTZ), NULL);
    ptz_img[3] = nfui_get_image_from_file((IMG_D_BTN_PTZ), NULL);

    zoom_img[0] = nfui_get_image_from_file((IMG_N_BTN_ZOOM), NULL);
    zoom_img[1] = nfui_get_image_from_file((IMG_O_BTN_ZOOM), NULL);
    zoom_img[2] = nfui_get_image_from_file((IMG_P_BTN_ZOOM), NULL);
    zoom_img[3] = nfui_get_image_from_file((IMG_D_BTN_ZOOM), NULL);

    log_img[0] = nfui_get_image_from_file((IMG_N_BTN_LOG), NULL);
    log_img[1] = nfui_get_image_from_file((IMG_O_BTN_LOG), NULL);
    log_img[2] = nfui_get_image_from_file((IMG_P_BTN_LOG), NULL);
    log_img[3] = nfui_get_image_from_file((IMG_D_BTN_LOG), NULL);

    audin_img[0] = nfui_get_image_from_file((IMG_N_BTN_AUDIO_IN), NULL);
    audin_img[1] = nfui_get_image_from_file((IMG_O_BTN_AUDIO_IN), NULL);
    audin_img[2] = nfui_get_image_from_file((IMG_P_BTN_AUDIO_IN), NULL);
    audin_img[3] = nfui_get_image_from_file((IMG_D_BTN_AUDIO_IN), NULL);

    audout_img[0] = nfui_get_image_from_file((IMG_N_BTN_AUDIO_OUT), NULL);
    audout_img[1] = nfui_get_image_from_file((IMG_O_BTN_AUDIO_OUT), NULL);
    audout_img[2] = nfui_get_image_from_file((IMG_P_BTN_AUDIO_OUT), NULL);
    audout_img[3] = nfui_get_image_from_file((IMG_D_BTN_AUDIO_OUT), NULL);

    panic_inact_img[0] = nfui_get_image_from_file((IMG_N_PANIC_INACT), NULL);
    panic_inact_img[1] = nfui_get_image_from_file((IMG_O_PANIC_INACT), NULL);
    panic_inact_img[2] = nfui_get_image_from_file((IMG_P_PANIC_INACT), NULL);
    panic_inact_img[3] = nfui_get_image_from_file((IMG_D_PANIC_INACT), NULL);

    panic_act_img[0] = nfui_get_image_from_file((IMG_N_PANIC_ACT), NULL);
    panic_act_img[1] = nfui_get_image_from_file((IMG_O_PANIC_ACT), NULL);
    panic_act_img[2] = nfui_get_image_from_file((IMG_P_PANIC_ACT), NULL);
    panic_act_img[3] = nfui_get_image_from_file((IMG_D_PANIC_ACT), NULL);

    g_net_img[0][0] = nfui_get_image_from_file((IMG_N_NET), NULL);
    g_net_img[0][1] = nfui_get_image_from_file((IMG_O_NET), NULL);
    g_net_img[0][2] = nfui_get_image_from_file((IMG_P_NET), NULL);
    g_net_img[0][3] = nfui_get_image_from_file((IMG_S_NET), NULL);

    g_net_img[1][0] = nfui_get_image_from_file((IMG_N_NET_ERROR), NULL);
    g_net_img[1][1] = nfui_get_image_from_file((IMG_O_NET_ERROR), NULL);
    g_net_img[1][2] = nfui_get_image_from_file((IMG_P_NET_ERROR), NULL);
    g_net_img[1][3] = nfui_get_image_from_file((IMG_S_NET_ERROR), NULL);

    g_net_img[2][0] = nfui_get_image_from_file((IMG_N_NET_CONFLICT), NULL);
    g_net_img[2][1] = nfui_get_image_from_file((IMG_O_NET_CONFLICT), NULL);
    g_net_img[2][2] = nfui_get_image_from_file((IMG_P_NET_CONFLICT), NULL);
    g_net_img[2][3] = nfui_get_image_from_file((IMG_S_NET_CONFLICT), NULL);

    g_alm_img[0][0] = nfui_get_image_from_file((IMG_N_ALARM_OFF), NULL);
    g_alm_img[0][1] = nfui_get_image_from_file((IMG_O_ALARM_OFF), NULL);
    g_alm_img[0][2] = nfui_get_image_from_file((IMG_P_ALARM_OFF), NULL);
    g_alm_img[0][3] = nfui_get_image_from_file((IMG_S_ALARM_OFF), NULL);

    g_alm_img[1][0] = nfui_get_image_from_file((IMG_N_ALARM_ON_1), NULL);
    g_alm_img[1][1] = nfui_get_image_from_file((IMG_O_ALARM_ON_1), NULL);
    g_alm_img[1][2] = nfui_get_image_from_file((IMG_P_ALARM_ON_1), NULL);
    g_alm_img[1][3] = nfui_get_image_from_file((IMG_S_ALARM_ON_1), NULL);

    g_alm_img[2][0] = nfui_get_image_from_file((IMG_N_ALARM_ON_2), NULL);
    g_alm_img[2][1] = nfui_get_image_from_file((IMG_O_ALARM_ON_2), NULL);
    g_alm_img[2][2] = nfui_get_image_from_file((IMG_P_ALARM_ON_2), NULL);
    g_alm_img[2][3] = nfui_get_image_from_file((IMG_S_ALARM_ON_2), NULL);

    g_disk_img[0][0] = nfui_get_image_from_file((IMG_N_DISK_NORMAL), NULL);
    g_disk_img[0][1] = nfui_get_image_from_file((IMG_O_DISK_NORMAL), NULL);
    g_disk_img[0][2] = nfui_get_image_from_file((IMG_P_DISK_NORMAL), NULL);
    g_disk_img[0][3] = nfui_get_image_from_file((IMG_S_DISK_NORMAL), NULL);

    g_disk_img[1][0] = nfui_get_image_from_file((IMG_N_DISK_NEED_CHECK), NULL);
    g_disk_img[1][1] = nfui_get_image_from_file((IMG_O_DISK_NEED_CHECK), NULL);
    g_disk_img[1][2] = nfui_get_image_from_file((IMG_P_DISK_NEED_CHECK), NULL);
    g_disk_img[1][3] = nfui_get_image_from_file((IMG_S_DISK_NEED_CHECK), NULL);

    g_disk_img[2][0] = nfui_get_image_from_file((IMG_N_DISK_FULL), NULL);
    g_disk_img[2][1] = nfui_get_image_from_file((IMG_O_DISK_FULL), NULL);
    g_disk_img[2][2] = nfui_get_image_from_file((IMG_P_DISK_FULL), NULL);
    g_disk_img[2][3] = nfui_get_image_from_file((IMG_S_DISK_FULL), NULL);

    g_disk_img[3][0] = nfui_get_image_from_file((IMG_N_DISK_NO), NULL);
    g_disk_img[3][1] = nfui_get_image_from_file((IMG_O_DISK_NO), NULL);
    g_disk_img[3][2] = nfui_get_image_from_file((IMG_P_DISK_NO), NULL);
    g_disk_img[3][3] = nfui_get_image_from_file((IMG_S_DISK_NO), NULL);

    g_disk_img[DISK_RAID_WARNING][0] = nfui_get_image_from_file((IMG_N_DISK_RAID_WARNING), NULL);
    g_disk_img[DISK_RAID_WARNING][1] = nfui_get_image_from_file((IMG_O_DISK_RAID_WARNING), NULL);
    g_disk_img[DISK_RAID_WARNING][2] = nfui_get_image_from_file((IMG_P_DISK_RAID_WARNING), NULL);
    g_disk_img[DISK_RAID_WARNING][3] = nfui_get_image_from_file((IMG_S_DISK_RAID_WARNING), NULL);


    /* create menu viewer */
#if defined(_HDI_MODEL_UX) || defined(_DVR_MODEL_UX)
    create_menu_viewer_HDI(g_curwnd);
#else
    create_menu_viewer(g_curwnd);
#endif
    /* init data */
    init_data_from_DAL();


    /* ctrl window */
    g_ctrl_win = (NFOBJECT*)nfui_nfwindow_new(parent, (guint)CTRL_WIN_POS_X, (guint)CTRL_WIN_POS_Y, (guint)CTRL_WIN_SIZE_W, (guint)CTRL_WIN_SIZE_H);

    g_curwnd = g_ctrl_win;
    nfui_nfwindow_set_title(g_ctrl_win, "LIVE STATUSBAR");
    gtk_window_set_modal(GTK_WINDOW(((NFWINDOW*)g_ctrl_win)->main_widget), FALSE);
    nfui_regi_post_event_callback(g_ctrl_win, ctrl_win_post_event_handler);

    uxm_reg_imsg_event(g_ctrl_win, INFY_DSPDB_CHANGE_NOTIFY);
    uxm_reg_imsg_event(g_ctrl_win, IRET_SCM_SHUTDOWN2);
    uxm_monitor_on_imsg_event(g_ctrl_win, INFY_DSPDB_CHANGE_NOTIFY);
    uxm_monitor_on_imsg_event(g_ctrl_win, IRET_SCM_SHUTDOWN2);


    /* fixed */
    ctrl_fixed1 = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(ctrl_fixed1, CTRL_WIN_SIZE_W, CTRL_WIN_SIZE_H);
    nfui_regi_post_event_callback(ctrl_fixed1, ctrl_fixed_post_event_handler);
    nfui_nfobject_show(ctrl_fixed1);

    uxm_reg_imsg_event(ctrl_fixed1, INFY_USRDB_CHANGE_NOTIFY);
    uxm_monitor_on_imsg_event(ctrl_fixed1, INFY_USRDB_CHANGE_NOTIFY);
    uxm_reg_imsg_event(ctrl_fixed1, INFY_FACTORY_DEFAULT_NOTIFY);
    uxm_monitor_on_imsg_event(ctrl_fixed1, INFY_FACTORY_DEFAULT_NOTIFY);
    uxm_reg_imsg_event(ctrl_fixed1, INFY_SYSTEM_DATA_LOAD_NOTIFY);
    uxm_monitor_on_imsg_event(ctrl_fixed1, INFY_SYSTEM_DATA_LOAD_NOTIFY);

    /* menu */
    pos_x += 8;
    nfui_get_image_size(IMG_BTN_N_START, &size_w, &size_h);

    nfwidget = (NFOBJECT*)nfui_nfbutton_new();
    g_menu = nfwidget;
    nfui_nfbutton_set_text(NF_BUTTON(nfwidget), "MENU");
    nfui_nfbutton_set_font_alignment(NF_BUTTON(nfwidget), NFALIGN_CENTER, 0);
    nfui_nfbutton_set_image(NF_BUTTON(nfwidget), start_img);
    nfui_nfbutton_set_pango_font(NF_BUTTON(nfwidget), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), btn_fg);
    nfui_nfobject_set_size(nfwidget, (guint)size_w, (guint)size_h);
//  nfui_regi_post_event_callback(nfwidget, start_btn_post_event_handler);
    nfui_nfobject_show(nfwidget);
    nfui_nffixed_put((NFFIXED*)ctrl_fixed1, nfwidget, pos_x, 8);
    nfui_regi_post_event_callback(nfwidget, _live_start_button_event_handler);


    /* CHANGE PASSWORD BUTTON */
    pos_x += (size_w + 6);

    _make_admin_button_image(user_img, pos_x, (ctrl_fixed1->height-44)/2, 200, 44);

    nfwidget = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_text(NF_BUTTON(nfwidget), "");
    nfui_nfbutton_set_font_alignment(NF_BUTTON(nfwidget), NFALIGN_LEFT, 38);
    nfui_nfbutton_set_image(NF_BUTTON(nfwidget), user_img);
    nfui_nfbutton_set_pango_font(NF_BUTTON(nfwidget), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), btn_fg2);
    nfui_nfobject_set_size(nfwidget, (guint)200, (guint)44);
    nfui_nfobject_support_multi_lang((NFOBJECT*)nfwidget, FALSE);
    nfui_nfobject_show(nfwidget);
    nfui_nfobject_disable(nfwidget);
    nfui_nffixed_put((NFFIXED*)ctrl_fixed1, nfwidget, pos_x, (ctrl_fixed1->height-44)/2);
    nfui_regi_post_event_callback(nfwidget, _change_pw_button_event_handler);
    g_user_btn = nfwidget;

    if (scm_is_qc_mode() == 0) {
        nfui_nfobject_set_size(nfwidget, (guint)120, (guint)44);
        pos_x += (120);
    }
    else {
        pos_x += (140 + 4 + 65);
    }
    
    nfui_get_image_size(IMG_N_DIV_FULL, &size_w, &size_h);

    for(i=0, j=0; i<7; i++) {
#if defined(GUI_4CH_SUPPORT)
        if ((i == 3) || (i == 4))
            continue;
#elif defined(GUI_8CH_SUPPORT)
        if (i == 4)
            continue;
#endif

#if !defined(GUI_4CH_SUPPORT) && !defined(_NOT_SUPPORT_SPC_DIV) || defined(_SUPPORT_USER_DIV)
        if(i == 6)
            nfui_get_image_size(IMG_N_DIV_SEQ, &size_w, &size_h);
#endif

        nfwidget = (NFOBJECT*)nfui_nfbutton_new();
        nfui_nfbutton_set_image(NF_BUTTON(nfwidget), g_div_img[i]);
        nfui_nfobject_set_size(nfwidget, (guint)size_w, (guint)size_h);
        nfui_nfobject_show(nfwidget);
        nfui_nfobject_disable(nfwidget);
        nfui_nffixed_put((NFFIXED*)ctrl_fixed1, nfwidget, pos_x, 19);

        if (i == 0)
        {
            nfui_nfobject_set_tooltip(nfwidget, "SCREEN MODE");
            nfui_regi_post_event_callback(nfwidget, _live_full_button_event_handler);
        }
        else if (i == 1)
        {
            nfui_nfobject_set_tooltip(nfwidget, "1 SPLIT SCREEN");
            nfui_regi_post_event_callback(nfwidget, _live_div1_button_event_handler);
        }
        else if (i == 2)
        {
            nfui_nfobject_set_tooltip(nfwidget, "4 SPLIT SCREEN");
            nfui_regi_post_event_callback(nfwidget, _live_div4_button_event_handler);
        }
        else if (i == 3)
        {
            nfui_nfobject_set_tooltip(nfwidget, "9 SPLIT SCREEN");
            nfui_regi_post_event_callback(nfwidget, _live_div9_button_event_handler);
        }
        else if (i == 4)
        {
            nfui_nfobject_set_tooltip(nfwidget, "16 SPLIT SCREEN");
            nfui_regi_post_event_callback(nfwidget, _live_div16_button_event_handler);
        }
        else if (i == 5)
        {
            nfui_nfobject_set_tooltip(nfwidget, "36 SPLIT SCREEN");
            nfui_regi_post_event_callback(nfwidget, _live_div36_button_event_handler);
        }
        else if (i == 6)
        {
#if !defined(GUI_4CH_SUPPORT) && !defined(_NOT_SUPPORT_SPC_DIV) || defined(_SUPPORT_USER_DIV)
            nfui_nfobject_set_tooltip(nfwidget, "6 SPLIT SCREEN / 8 SPLIT SCREEN / SEQUENCE");
#else
            nfui_nfobject_set_tooltip(nfwidget, "SEQUENCE");
#endif
            nfui_regi_post_event_callback(nfwidget, _live_seq_button_event_handler);
        }
        g_menuObj[j++] = nfwidget;

        pos_x += (size_w + 1);
    }


    // TODO: use toggle button ??
    // osd
    nfui_get_image_size(IMG_N_OSD_OFF, &size_w, &size_h);

    nfwidget = (NFOBJECT*)nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type((NFCHECKBUTTON*)nfwidget, NFCHECK_TYPE_DEF);
    nfui_check_button_set_inactive_image((NFCHECKBUTTON*)nfwidget, osd_inact_img);
    nfui_check_button_set_active_image((NFCHECKBUTTON*)nfwidget, osd_act_img);
    nfui_nfobject_set_size(nfwidget, size_w, size_h);
    nfui_nfobject_set_tooltip(nfwidget, "DISPLAY");
    nfui_regi_post_event_callback(nfwidget, _live_osd_button_event_handler);
    nfui_nfobject_show(nfwidget);
    nfui_nfobject_disable(nfwidget);
    nfui_nffixed_put((NFFIXED*)ctrl_fixed1, nfwidget, pos_x, 19);
    g_menuObj[OSD_BTN] = nfwidget;

    if (scm_is_qc_mode() == 0)
    {
        nfui_get_image_size(IMG_BTN_N_START, &size_w, &size_h);

        pos_x = 947-100;

        nfwidget = (NFOBJECT*)nfui_nfbutton_new();
        nfui_nfbutton_set_text(NF_BUTTON(nfwidget), "QC");
        nfui_nfbutton_set_font_alignment(NF_BUTTON(nfwidget), NFALIGN_CENTER, 0);
        nfui_nfbutton_set_image(NF_BUTTON(nfwidget), start_img);
        nfui_nfbutton_set_pango_font(NF_BUTTON(nfwidget), nffont_get_pango_font(NFFONT_LARGE_SEMI), btn_fg);
        nfui_nfobject_set_size(nfwidget, (guint)size_w, (guint)size_h);
        nfui_nfobject_show(nfwidget);
        nfui_nffixed_put((NFFIXED*)ctrl_fixed1, nfwidget, pos_x, 8);
        nfui_regi_post_event_callback(nfwidget, _qc_test_button_event_handler);
    }

    // ptz / zoom / log
    pos_x = 947;
    nfui_get_image_size(IMG_N_BTN_PTZ, &size_w, &size_h);

    for(i=0; i<3; i++) {
        nfwidget = (NFOBJECT*)nfui_nfbutton_new();
        if(i == 0)      nfui_nfbutton_set_image(NF_BUTTON(nfwidget), ptz_img);
        else if(i == 1) nfui_nfbutton_set_image(NF_BUTTON(nfwidget), zoom_img);
        else if(i == 2) nfui_nfbutton_set_image(NF_BUTTON(nfwidget), log_img);
        nfui_nfobject_set_size(nfwidget, (guint)size_w, (guint)size_h);
        nfui_nfobject_show(nfwidget);
        nfui_nfobject_disable(nfwidget);
        nfui_nffixed_put((NFFIXED*)ctrl_fixed1, nfwidget, pos_x, 19);

        pos_x += (size_w + 1);

        if(i == 0)
        {
            nfui_nfobject_set_tooltip(nfwidget, "PTZ");
            nfui_regi_post_event_callback(nfwidget, post_ptz_event_cb);
        }
        else if(i == 1)
        {
            nfui_nfobject_set_tooltip(nfwidget, "ZOOM");
            nfui_regi_post_event_callback(nfwidget, post_zoom_event_cb);
        }
        else if(i == 2)
        {
            nfui_nfobject_set_tooltip(nfwidget, "EVENT LOG");
            nfui_regi_post_event_callback(nfwidget, post_log_event_cb);
        }

        g_menuObj[i + PTZ_BTN] = nfwidget;
    }

    // audio / mic
    pos_x += 19;

    for(i=0; i<2; i++) {
        nfwidget = (NFOBJECT*)nfui_nfbutton_new();
        if(i == 0)      nfui_nfbutton_set_image(NF_BUTTON(nfwidget), audin_img);
        else if(i == 1) nfui_nfbutton_set_image(NF_BUTTON(nfwidget), audout_img);
        nfui_nfobject_set_size(nfwidget, (guint)size_w, (guint)size_h);
        nfui_nfobject_show(nfwidget);
        nfui_nfobject_disable(nfwidget);

        nfui_nffixed_put((NFFIXED*)ctrl_fixed1, nfwidget, pos_x, 19);

        pos_x += (size_w + 1);

        if(i == 0)
        {
            nfui_nfobject_set_tooltip(nfwidget, "AUDIO INPUT");
            nfui_regi_post_event_callback(nfwidget, post_audio_input_event_cb);
        }
        else if(i == 1)
        {
            nfui_nfobject_set_tooltip(nfwidget, "AUDIO OUTPUT");
            nfui_regi_post_event_callback(nfwidget, post_audio_output_event_cb);
        }

        g_menuObj[i + AUDIO_BTN] = nfwidget;
    }

#if defined(_NOT_SUPPORT_MIC)
    // hide audio out
    nfui_nfobject_hide(g_menuObj[MIC_BTN]);
#endif

    // panic
    pos_x += 19;

    is_panic = scm_get_panic_record();

    nfwidget = (NFOBJECT*)nfui_checkbutton_new(is_panic);
    nfui_check_button_set_skin_type((NFCHECKBUTTON*)nfwidget, NFCHECK_TYPE_DEF);
    nfui_check_button_set_inactive_image((NFCHECKBUTTON*)nfwidget, panic_inact_img);
    nfui_check_button_set_active_image((NFCHECKBUTTON*)nfwidget, panic_act_img);
    nfui_nfobject_set_size(nfwidget, size_w, size_h);
    nfui_nfobject_set_tooltip(nfwidget, "PANIC RECORD");
    nfui_regi_post_event_callback(nfwidget, post_panic_event_cb);
    nfui_nfobject_show(nfwidget);
    nfui_nfobject_disable(nfwidget);
    nfui_nffixed_put((NFFIXED*)ctrl_fixed1, nfwidget, pos_x, 19);

    g_menuObj[PANIC_BTN] = nfwidget;

    uxm_reg_imsg_event(nfwidget, INFY_PANIC_REC_STATUS);
    uxm_monitor_on_imsg_event(nfwidget, INFY_PANIC_REC_STATUS);

    // alarm
    pos_x += (size_w + 10);

    nfwidget = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(nfwidget), g_alm_img[ALARM_OFF]);
    nfui_nfobject_set_tooltip(nfwidget, "ALARM STATUS");
    nfui_nfobject_show(nfwidget);
    nfui_regi_post_event_callback(nfwidget, post_alarm_event_cb);
    nfui_nffixed_put((NFFIXED*)ctrl_fixed1, nfwidget, pos_x, 19);

    g_menuObj[ALARM_BTN] = nfwidget;

    // alarm event
    uxm_reg_imsg_event(nfwidget, INFY_SENSOR_NOTIFY);
    uxm_reg_imsg_event(nfwidget, INFY_MOTION_NOTIFY);
    uxm_reg_imsg_event(nfwidget, INFY_VLOSS_NOTIFY);
    uxm_reg_imsg_event(nfwidget, INFY_ALARM_NOTIFY);
    uxm_reg_imsg_event(nfwidget, INFY_BUZZER_NOTIFY);
    uxm_reg_imsg_event(nfwidget, INFY_AI_KEEP_ALIVE_NOTIFY);

    uxm_monitor_on_imsg_event(nfwidget, INFY_SENSOR_NOTIFY);
    uxm_monitor_on_imsg_event(nfwidget, INFY_MOTION_NOTIFY);
    uxm_monitor_on_imsg_event(nfwidget, INFY_VLOSS_NOTIFY);
    uxm_monitor_on_imsg_event(nfwidget, INFY_ALARM_NOTIFY);
    uxm_monitor_on_imsg_event(nfwidget, INFY_BUZZER_NOTIFY);
    uxm_monitor_on_imsg_event(nfwidget, INFY_AI_KEEP_ALIVE_NOTIFY);

    // system event
    uxm_reg_imsg_event(nfwidget, INFY_SYSFAN_NOTIFY);
    uxm_reg_imsg_event(nfwidget, INFY_TEMPERATURE_NOTIFY);
    uxm_reg_imsg_event(nfwidget, INFY_POE_NOTIFY);
    uxm_reg_imsg_event(nfwidget, INFY_POE_HUB_NOTIFY);

    uxm_monitor_on_imsg_event(nfwidget, INFY_SYSFAN_NOTIFY);
    uxm_monitor_on_imsg_event(nfwidget, INFY_TEMPERATURE_NOTIFY);
    uxm_monitor_on_imsg_event(nfwidget, INFY_POE_NOTIFY);
    uxm_monitor_on_imsg_event(nfwidget, INFY_POE_HUB_NOTIFY);

    // disk event
    uxm_reg_imsg_event(nfwidget, INFY_WRITEFAIL_NOTIFY);
    uxm_reg_imsg_event(nfwidget, INFY_EXHAUST_NOTIFY);
    uxm_reg_imsg_event(nfwidget, INFY_NODISK_NOTIFY);
    uxm_reg_imsg_event(nfwidget, INFY_DISK_FULL_NOTIFY);
    uxm_reg_imsg_event(nfwidget, INFY_SMART_WARN_NOTIFY);
    uxm_reg_imsg_event(nfwidget, INFY_SMART_ERROR_NOTIFY);

    uxm_monitor_on_imsg_event(nfwidget, INFY_WRITEFAIL_NOTIFY);
    uxm_monitor_on_imsg_event(nfwidget, INFY_EXHAUST_NOTIFY);
    uxm_monitor_on_imsg_event(nfwidget, INFY_NODISK_NOTIFY);
    uxm_monitor_on_imsg_event(nfwidget, INFY_DISK_FULL_NOTIFY);
    uxm_monitor_on_imsg_event(nfwidget, INFY_SMART_WARN_NOTIFY);
    uxm_monitor_on_imsg_event(nfwidget, INFY_SMART_ERROR_NOTIFY);

    // network event
    uxm_reg_imsg_event(nfwidget, INFY_WAN_NOTIFY);
    uxm_reg_imsg_event(nfwidget, INFY_DDNS_NOTIFY);
    uxm_reg_imsg_event(nfwidget, INFY_NETLOGINFAIL_NOTIFY);
    uxm_reg_imsg_event(nfwidget, INFY_SET_IP_CONFLICT_NOTIFY);
    uxm_reg_imsg_event(nfwidget, INFY_CAM_IP_CONFLICT_NOTIFY);

    uxm_monitor_on_imsg_event(nfwidget, INFY_WAN_NOTIFY);
    uxm_monitor_on_imsg_event(nfwidget, INFY_DDNS_NOTIFY);
    uxm_monitor_on_imsg_event(nfwidget, INFY_NETLOGINFAIL_NOTIFY);
    uxm_monitor_on_imsg_event(nfwidget, INFY_SET_IP_CONFLICT_NOTIFY);
    uxm_monitor_on_imsg_event(nfwidget, INFY_CAM_IP_CONFLICT_NOTIFY);

    nfui_get_image_size(IMG_N_ALARM_OFF, &size_w, &size_h);

    // network
    pos_x += (size_w + 1);

    nfwidget = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(nfwidget), g_net_img[NET_ERROR]);
    nfui_nfobject_set_tooltip(nfwidget, "NETWORK STATUS");
    nfui_nfobject_show(nfwidget);
    nfui_regi_post_event_callback(nfwidget, post_network_event_cb);
    nfui_nffixed_put((NFFIXED*)ctrl_fixed1, nfwidget, pos_x, 19);

    g_menuObj[NET_BTN] = nfwidget;

    uxm_reg_imsg_event(nfwidget, INFY_NET_NOTIFY);
    uxm_reg_imsg_event(nfwidget, INFY_NET_RXTX);
    uxm_reg_imsg_event(nfwidget, INFY_SET_IP_CONFLICT_NOTIFY);
    uxm_reg_imsg_event(nfwidget, INFY_CAM_IP_CONFLICT_NOTIFY);

    uxm_monitor_on_imsg_event(nfwidget, INFY_NET_NOTIFY);
    uxm_monitor_on_imsg_event(nfwidget, INFY_NET_RXTX);
    uxm_monitor_on_imsg_event(nfwidget, INFY_SET_IP_CONFLICT_NOTIFY);
    uxm_monitor_on_imsg_event(nfwidget, INFY_CAM_IP_CONFLICT_NOTIFY);


    // disk
    nfui_get_image_size(IMG_N_NET, &size_w, &size_h);
    pos_x += (size_w + 1);

    nfwidget = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(nfwidget), g_disk_img[DISK_NORMAL]);
    nfui_nfobject_set_tooltip(nfwidget, "DISK STATUS");
    nfui_nfobject_show(nfwidget);
    nfui_regi_post_event_callback(nfwidget, post_disk_event_cb);
    nfui_nffixed_put((NFFIXED*)ctrl_fixed1, nfwidget, pos_x, 19);

    g_menuObj[DISK_BTN] = nfwidget;

    uxm_reg_imsg_event(nfwidget, INFY_DISK_USAGE_NOTIFY);
    uxm_reg_imsg_event(nfwidget, INFY_DISK_FULL_NOTIFY);
    uxm_reg_imsg_event(nfwidget, INFY_SMART_WARN_NOTIFY);
    uxm_reg_imsg_event(nfwidget, INFY_SMART_ERROR_NOTIFY);
    uxm_reg_imsg_event(nfwidget, INFY_DISK_OW_NOTIFY);
    uxm_reg_imsg_event(nfwidget, INFY_NODISK_NOTIFY);
    uxm_reg_imsg_event(nfwidget, INFY_DISK_RAID_CHANGE_STATUS_NOTIFY);
    uxm_reg_imsg_event(nfwidget, INFY_DSKDB_CHANGE_NOTIFY);

    uxm_monitor_on_imsg_event(nfwidget, INFY_DISK_USAGE_NOTIFY);
    uxm_monitor_on_imsg_event(nfwidget, INFY_DISK_FULL_NOTIFY);
    uxm_monitor_on_imsg_event(nfwidget, INFY_SMART_WARN_NOTIFY);
    uxm_monitor_on_imsg_event(nfwidget, INFY_SMART_ERROR_NOTIFY);
    uxm_monitor_on_imsg_event(nfwidget, INFY_DISK_OW_NOTIFY);
    uxm_monitor_on_imsg_event(nfwidget, INFY_NODISK_NOTIFY);
    uxm_monitor_on_imsg_event(nfwidget, INFY_DISK_RAID_CHANGE_STATUS_NOTIFY);
    uxm_monitor_on_imsg_event(nfwidget, INFY_DSKDB_CHANGE_NOTIFY);

#if 0
    nfui_get_image_size(IMG_N_DISK_NORMAL, &size_w, &size_h);
    pos_x += (size_w + 1);

    nfwidget = (NFOBJECT*)nfui_nfimage_new((IMG_BACKUP_STATUS));
    nfui_nfobject_show(nfwidget);
    nfui_nffixed_put((NFFIXED*)ctrl_fixed1, nfwidget, pos_x, 19);
#endif

    nfui_get_image_size(IMG_N_DISK_NORMAL, &size_w, &size_h);
    pos_x += size_w;


    // active ctrlbar time
#if 0
    g_date_label = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(338));
    nfui_nflabel_use_pango_cashing((NFLABEL*)g_date_label, 0, NULL);
    nfui_nflabel_set_align((NFLABEL*)g_date_label, NFALIGN_CENTER, 0);
    nfui_nfobject_use_tooltip(g_date_label, FALSE);
    nfui_nflabel_use_strip((NFLABEL*)g_date_label, FALSE);
    nfui_nfobject_modify_bg(g_date_label, NFOBJECT_STATE_NORMAL, COLOR_IDX(TODO_COLOR));
    nfui_nfobject_use_focus(g_date_label, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(g_date_label, TIMELABEL_SIZE_W, TIMELABEL_SIZE_H);
    nfui_nfobject_show(g_date_label);
    nfui_nffixed_put((NFFIXED*)ctrl_fixed1, g_date_label, (guint)pos_x, (guint)19);
#else
    bgImg = nfui_get_image_from_file(IMG_STATUSBAR_BG, NULL);
    pbBG = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, TIMELABEL_SIZE_W, TIMELABEL_SIZE_H);
    gdk_pixbuf_copy_area(bgImg, pos_x, 19, TIMELABEL_SIZE_W, TIMELABEL_SIZE_H, pbBG, 0, 0);

    g_date_label = nfui_nfimage_new_pixbuf(pbBG);
    nfui_nfimage_set_pango_font((NFIMAGE*)g_date_label, nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(338));
    nfui_nfimage_set_font_alignment((NFIMAGE*)g_date_label, NFALIGN_CENTER, 0);
    nfui_nfobject_use_focus(g_date_label, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(g_date_label);
    nfui_nffixed_put((NFFIXED*)ctrl_fixed1, g_date_label, (guint)pos_x, (guint)19);
#endif


#if 0
    g_time_label = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(338));
    nfui_nflabel_set_spacing((NFLABEL*)g_time_label, SEMI_CONDENSED_SPACING);
    nfui_nflabel_use_pango_cashing((NFLABEL*)g_time_label, 0, NULL);
    if(get_time_format())   nfui_nflabel_set_align((NFLABEL*)g_time_label, NFALIGN_LEFT, 30);
    else                    nfui_nflabel_set_align((NFLABEL*)g_time_label, NFALIGN_CENTER, 0);
    nfui_nfobject_use_tooltip(g_time_label, FALSE);
    nfui_nflabel_use_strip((NFLABEL*)g_time_label, FALSE);
    nfui_nfobject_modify_bg(g_time_label, NFOBJECT_STATE_NORMAL, COLOR_IDX(TODO_COLOR));
    nfui_nfobject_use_focus(g_time_label, NFOBJECT_FOCUS_OFF);
    if(get_time_format())   nfui_nfobject_set_size(g_time_label, (TIMELABEL_SIZE_W - AMPM_LABEL_W), AMPM_LABEL_H);
    else                    nfui_nfobject_set_size(g_time_label, TIMELABEL_SIZE_W, TIMELABEL_SIZE_H);
    nfui_nfobject_show(g_time_label);
    nfui_nffixed_put((NFFIXED*)ctrl_fixed1, g_time_label, (guint)pos_x, (guint)(19 + TIMELABEL_SIZE_H));
#else
    pbBG = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, TIMELABEL_SIZE_W, TIMELABEL_SIZE_H);
    gdk_pixbuf_copy_area(bgImg, pos_x, (19+TIMELABEL_SIZE_H), TIMELABEL_SIZE_W, TIMELABEL_SIZE_H, pbBG, 0, 0);

    g_time_label = nfui_nfimage_new_pixbuf(pbBG);
    nfui_nfimage_set_pango_font((NFIMAGE*)g_time_label, nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(338));
    if(get_time_format())   nfui_nfimage_set_font_alignment((NFIMAGE*)g_time_label, NFALIGN_LEFT, 30);
    else                    nfui_nfimage_set_font_alignment((NFIMAGE*)g_time_label, NFALIGN_CENTER, 0);
    nfui_nfobject_use_focus(g_time_label, NFOBJECT_FOCUS_OFF);
    if(get_time_format())   nfui_nfobject_set_size(g_time_label, (TIMELABEL_SIZE_W - AMPM_LABEL_W), AMPM_LABEL_H);
    else                    nfui_nfobject_set_size(g_time_label, TIMELABEL_SIZE_W, TIMELABEL_SIZE_H);
    nfui_nfobject_show(g_time_label);
    nfui_nffixed_put((NFFIXED*)ctrl_fixed1, g_time_label, (guint)pos_x, (guint)(19 + TIMELABEL_SIZE_H));
#endif

#if 0
    g_ampm_label = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(338));
    nfui_nflabel_set_spacing((NFLABEL*)g_ampm_label, SEMI_CONDENSED_SPACING);
    nfui_nflabel_use_pango_cashing((NFLABEL*)g_ampm_label, 0, NULL);
    nfui_nflabel_set_align((NFLABEL*)g_ampm_label, NFALIGN_LEFT, 0);
    nfui_nfobject_use_tooltip(g_ampm_label, FALSE);
    nfui_nfobject_modify_bg(g_ampm_label, NFOBJECT_STATE_NORMAL, COLOR_IDX(TODO_COLOR));
    nfui_nfobject_use_focus(g_ampm_label, NFOBJECT_FOCUS_OFF);
    if(get_time_format()) {
        nfui_nfobject_set_size(g_ampm_label, AMPM_LABEL_W, AMPM_LABEL_H);
        nfui_nffixed_put((NFFIXED*)ctrl_fixed1, g_ampm_label, (guint)(pos_x + TIMELABEL_SIZE_W - AMPM_LABEL_W), (guint)(19 + TIMELABEL_SIZE_H));
    } else        {
        nfui_nfobject_set_size(g_ampm_label, 1, 1);
        nfui_nffixed_put((NFFIXED*)ctrl_fixed1, g_ampm_label, (guint)(pos_x + TIMELABEL_SIZE_W), (guint)(19 + TIMELABEL_SIZE_H));
    }
    nfui_nfobject_show(g_ampm_label);
#else
    pbBG = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, AMPM_LABEL_W, AMPM_LABEL_H);
    gdk_pixbuf_copy_area(bgImg, (pos_x + TIMELABEL_SIZE_W - AMPM_LABEL_W), (19+TIMELABEL_SIZE_H), AMPM_LABEL_W, AMPM_LABEL_H, pbBG, 0, 0);

    g_ampm_label = nfui_nfimage_new_pixbuf(pbBG);
    nfui_nfimage_set_pango_font((NFIMAGE*)g_ampm_label, nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(338));
    nfui_nfimage_set_font_alignment((NFIMAGE*)g_ampm_label, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(g_ampm_label, NFOBJECT_FOCUS_OFF);
    if(get_time_format()) {
        nfui_nfobject_set_size(g_ampm_label, AMPM_LABEL_W, AMPM_LABEL_H);
        nfui_nffixed_put((NFFIXED*)ctrl_fixed1, g_ampm_label, (guint)(pos_x + TIMELABEL_SIZE_W - AMPM_LABEL_W), (guint)(19 + TIMELABEL_SIZE_H));
    } else        {
        nfui_nfobject_set_size(g_ampm_label, 1, 1);
        nfui_nffixed_put((NFFIXED*)ctrl_fixed1, g_ampm_label, (guint)(pos_x + TIMELABEL_SIZE_W), (guint)(19 + TIMELABEL_SIZE_H));
    }
    nfui_nfobject_show(g_ampm_label);
#endif


    nfui_nfwindow_add((NFWINDOW*)g_ctrl_win, ctrl_fixed1);
    nfui_run_main_event_handler(g_ctrl_win);
    nfui_nfobject_show(g_ctrl_win);
    nfui_make_key_hierarchy((NFWINDOW*)g_ctrl_win);

    if(get_on_time_mode())
        nfui_nfobject_hide(g_ctrl_win);

    nfui_page_open(PGID_LIVECTRLBAR, g_ctrl_win, ssm_get_cur_id(NULL));

    t_timer_id = g_timeout_add(300, update_time, NULL);

    scm_req_disk_full_data();
    scm_req_disk_usage_data();
    scm_req_disk_ow_event_data();
    scm_req_disk_smart_event_data();
    scm_req_smart_event_data();
    scm_req_nodisk_event_data();
    scm_req_exhaust_event_data();
    scm_req_net_status_data();

    if (scm_is_qc_mode() == 0)
    {
        live_normal_mode();
    }

    return g_ctrl_win;
}

int VW_Live_StatusBar_Close()
{
    if (!g_curwnd) return 0;
    nfui_nfobject_destroy(g_curwnd);
    return 0;
}

void VW_Live_StatusBar_Show()
{
    g_return_if_fail(g_ctrl_win != NULL);

    // full mode && not alway mode
    if(get_live_mode() == 0 && get_on_time_mode() != ALWAYS_ON_MODE)
        reset_on_time_timer();

//  if(ssm_check_access_auth(USR_AUTH_AUDIO)) nfui_nfobject_enable(g_menuObj[AUDIO_BTN]);
//  else                                      nfui_nfobject_disable(g_menuObj[AUDIO_BTN]);

//  if(ssm_check_access_auth(USR_AUTH_MIC)) nfui_nfobject_enable(g_menuObj[MIC_BTN]);
//  else                                    nfui_nfobject_disable(g_menuObj[MIC_BTN]);

    if(!nfui_nfobject_is_shown(g_ctrl_win)) {
        nfui_nfobject_show(g_ctrl_win);
//  }else {
//      nfui_signal_emit(g_menuObj[AUDIO_BTN], GDK_EXPOSE, FALSE);
//      nfui_signal_emit(g_menuObj[MIC_BTN], GDK_EXPOSE, FALSE);
    }
}


void VW_Live_StatusBar_Hide()
{
    g_return_if_fail(g_ctrl_win != NULL);

    if(nfui_nfobject_is_shown(g_ctrl_win))
        nfui_nfobject_hide(g_ctrl_win);
}


gboolean VW_Live_StatusBar_IsShown()
{
    g_return_if_fail(g_ctrl_win != NULL);

    return nfui_nfobject_is_shown(g_ctrl_win);
}

gboolean VW_Live_StatusBar_IsInArea(guint x, guint y)
{
    if((x >= CTRL_WIN_POS_X) && (x <= CTRL_WIN_SIZE_W)) {
        if((y >= (DISPLAY_ACTIVE_HEIGHT - 20)) && (y <= CTRL_WIN_POS_Y + CTRL_WIN_SIZE_H)) {
            return TRUE;
        }
    }

    return FALSE;
}

void VW_Live_StatusBar_Set_User(gchar *user)
{
    if(DAL_get_username_on_off())
    {
        nfui_nfbutton_set_text((NFBUTTON*)g_user_btn, user);
    }
    else
    {
        nfui_nfbutton_set_text((NFBUTTON*)g_user_btn, " ");
    }

    nfui_signal_emit(g_user_btn, GDK_EXPOSE, TRUE);
}


void VW_Live_StatusBar_Menu_Enable()
{
    gint i;

    for(i=0; i<ALARM_BTN; i++)
    {
        if(i == AUDIO_BTN && !ssm_check_access_auth(USR_AUTH_AUDIO))
        {
            nfui_nfobject_disable(g_menuObj[i]);
            continue;
        }

        if(i == AUDIO_BTN && !(DAL_get_support_audio()))
        {
            nfui_nfobject_disable(g_menuObj[i]);
            continue;
        }


#if defined(_NOT_SUPPORT_MIC)
        if(i == MIC_BTN) continue;
#else
        if(i == MIC_BTN && !ssm_check_access_auth(USR_AUTH_MIC)) {
            nfui_nfobject_disable(g_menuObj[i]);
            continue;
        }
#endif

        if(i == MIC_BTN && (!(DAL_get_support_audio()) || (nf_sysman_get_pba_type() == NF_SYSMAN_PBA_TYPE_B)))
        {
            nfui_nfobject_disable(g_menuObj[i]);
            continue;
        }

        if (mcf.sys_sub1.menu_pos[SYS_SUB1_PTZ_SETUP] == -1)
        {
            if(i == PTZ_BTN) continue;
        }

        if(i == PTZ_BTN && !ssm_check_access_auth(USR_AUTH_PTZ)) {
            nfui_nfobject_disable(g_menuObj[i]);
            continue;
        }

        nfui_nfobject_enable(g_menuObj[i]);
    }

    for(i=0; i<ALARM_BTN; i++)
        nfui_signal_emit(g_menuObj[i], GDK_EXPOSE, TRUE);

    if (DAL_get_passwd_change_enable() && DAL_get_username_on_off())
    {
        nfui_nfobject_enable(g_user_btn);   //change password button
        nfui_signal_emit(g_user_btn, GDK_EXPOSE, TRUE);
    }
    else
    {
        nfui_nfobject_disable(g_user_btn);
        nfui_signal_emit(g_user_btn, GDK_EXPOSE, TRUE);
    }
}

void VW_Live_StatusBar_Menu_Disable()
{
    gint i;

    for(i=0; i<ALARM_BTN; i++) 
    {
        nfui_nfobject_disable(g_menuObj[i]);
        
        if (i == AUDIO_BTN && ssm_check_access_auth(USR_AUTH_AUDIO)) 
            nfui_nfobject_enable(g_menuObj[i]);
        
        if (i == MIC_BTN && ssm_check_access_auth(USR_AUTH_MIC)) 
            nfui_nfobject_enable(g_menuObj[i]);            
    }

    for(i=0; i<ALARM_BTN; i++)
        nfui_signal_emit(g_menuObj[i], GDK_EXPOSE, TRUE);

    nfui_nfobject_disable(g_user_btn);  //change password button
    nfui_signal_emit(g_user_btn, GDK_EXPOSE, TRUE);

}

guint VW_Live_StatusBar_On_Time()
{
    return get_on_time_mode();
}


gchar* VW_get_User_Name()
{
    gchar *user_str =0;

    user_str =  nfui_nfbutton_get_text((NFBUTTON *) g_user_btn);

    return user_str;
}
