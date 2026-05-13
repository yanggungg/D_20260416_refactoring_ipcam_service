#include "nf_afx.h"

#include "scm.h"
#include "iux_msg.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfbutton.h"

#include "modules/ssm.h"

#include "vw_live_snapshot.h"
#include "vw_arch_export.h"
#include "dtf.h"

#define SNAPSHOT_POS_X          ((DISPLAY_ACTIVE_WIDTH-SNAPSHOT_SIZE_W)/2)
#define SNAPSHOT_POS_Y          ((DISPLAY_ACTIVE_HEIGHT-SNAPSHOT_SIZE_H)/2)
#define SNAPSHOT_SIZE_W         (628)
#define SNAPSHOT_SIZE_H         (578)


typedef struct _SNAPSHOT_INFO {
    GTimeVal time;
    guint ch;

    gchar strTime[24];
    gchar strCh[16];
}SNAPSHOT_INFO;


static SNAPSHOT_INFO g_snapInfo;


static NFWINDOW *g_curwnd = 0;

static void init_snapshot_info();
static /*inline*/ void set_snapshot_ch(guint ch);
static /*inline*/ void set_snapshot_time(GTimeVal time);
static /*inline*/ void set_snapshot_string_time(gchar *time);
static /*inline*/ void set_snapshot_ch_title(gchar *title);




static void init_snapshot_info()
{
    memset(&g_snapInfo, 0x00, sizeof(SNAPSHOT_INFO));
}

static /*inline*/ void set_snapshot_ch(guint ch)
{
    g_snapInfo.ch = ch;
}

static /*inline*/ void set_snapshot_time(GTimeVal time)
{
    g_snapInfo.time = time;
}

static /*inline*/ void set_snapshot_string_time(gchar *time)
{
    memset(g_snapInfo.strTime, 0x00, sizeof(g_snapInfo.strTime));
    strcpy(g_snapInfo.strTime, time);
}

static /*inline*/ void set_snapshot_ch_title(gchar *title)
{
    memset(g_snapInfo.strCh, 0x00, sizeof(g_snapInfo.strCh));
    strcpy(g_snapInfo.strCh, title);
}

static gboolean post_export_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE) 
    {
        NF_ARCH_SNAP_PARAM snap_param;
        NF_ARCH_SNAP_INFO snap_info;
        BURN_INFO burn_info;

        if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        // set snapshot param
        snap_param.snap_time = g_snapInfo.time;
        snap_param.ch        = g_snapInfo.ch;

        // TODO : get image info
        snap_param.image_size;
        snap_param.image;

        memset(snap_param.tag, 0x00, sizeof(snap_param.tag));
        g_sprintf(snap_param.tag, "%s %s", g_snapInfo.strCh, g_snapInfo.strTime);

        memset(snap_param.user, 0x00, sizeof(snap_param.user));
        ssm_get_cur_id(snap_param.user);

        g_message("%s [%d] >>>>>>>>>>>>>> tags: %s ", __FUNCTION__, __LINE__, snap_param.tag);
        g_message("%s [%d] >>>>>>>>>>>>>> user: %s ", __FUNCTION__, __LINE__, snap_param.user);
        

        // open export 
        memset(&burn_info, 0x00, sizeof(BURN_INFO));        
        VW_ArchExport_Open(g_curwnd, &burn_info);
    }

    return FALSE;
}


static gboolean post_close_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE) 
    {
        NFOBJECT *top;

        if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;
    
        scm_end_query();

        top = nfui_nfobject_get_top(obj);
        nfui_nfobject_destroy(top);
    }

    return FALSE;
}


static gboolean post_snapshot_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

    switch(evt->type)
    {
        case GDK_EXPOSE:
        {
            drawable = nfui_nfobject_get_window(obj);
            gc = nfui_nfobject_get_gc(obj);

            nfui_nfobject_get_size(obj, &size_w, &size_h);
            pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
            nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);

            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(200));
            gdk_draw_rectangle(drawable, gc, FALSE, 20, 166, 588, 330);
        
            nfui_nfobject_gc_unref(gc);
        }
        break;

        case GDK_DELETE:
        {
            nfui_nfobject_get_size(obj, &size_w, &size_h);
            nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
        }
        break;

        default:
            break;
    }

    return FALSE;
}

static gboolean post_snapshot_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_DELETE) {
        g_curwnd = 0;   
    }

    return FALSE;
}

gboolean VW_Live_Snapshot_Open(NFWINDOW *parent, guint ch, GTimeVal time)
{
    NFOBJECT *stWin;
    NFOBJECT *stFixed;
    NFOBJECT *obj;
    GTimeVal tv_stamp;

    gchar strBuf[STRING_SIZE_64];
    guint dformat = 0, tformat = 0;
    gchar *date_format[][3] = {{"%Y/%m/%d %T", "%m/%d/%Y %T", "%d/%m/%Y %T"},
        {"%Y/%m/%d %I:%M:%S %p", "%m/%d/%Y %I:%M:%S %p", "%d/%m/%Y %I:%M:%S %p"}};

    struct tm tm_time;


    /* init & set info */
    init_snapshot_info();
    set_snapshot_ch(ch);
    set_snapshot_time(time);


    /* window */
    stWin = (NFOBJECT*)nfui_nfwindow_new(parent, SNAPSHOT_POS_X, SNAPSHOT_POS_Y, SNAPSHOT_SIZE_W, SNAPSHOT_SIZE_H);
    g_curwnd = stWin;
    nfui_regi_post_event_callback(stWin, post_snapshot_win_event_handler);


    /* fixed */
    stFixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(stFixed, SNAPSHOT_SIZE_W, SNAPSHOT_SIZE_H);
    nfui_regi_post_event_callback(stFixed, post_snapshot_fixed_event_cb);
    nfui_nfobject_show(stFixed);


    /* title */
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SNAPSHOT", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(TODO_COLOR));
    nfui_nfobject_set_size(obj, 620, 36);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(TODO_COLOR));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)stFixed, obj, 4, 4);



    /* camera title */
    memset(strBuf, 0x00, sizeof(strBuf));
    //DAL_get_camera_title(strBuf, ch);
    var_get_camtitle(strBuf, ch);

    set_snapshot_ch_title(strBuf);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(TODO_COLOR));
    nfui_nfobject_set_size(obj, 300, 22);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_support_multi_lang(obj, FALSE);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(TODO_COLOR));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)stFixed, obj, 20, 60);



    /* timestamp */
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("TIMESTAMP : ", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(TODO_COLOR));
    nfui_nfobject_set_size(obj, 130, 22);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nflabel_use_strip((NFLABEL*)obj, FALSE);
    nfui_nfobject_use_tooltip(obj, FALSE);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(TODO_COLOR));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)stFixed, obj, 20, 100);


    tv_stamp = time;
    dtf_get_local_datetime(tv_stamp.tv_sec, strBuf);
    set_snapshot_string_time(strBuf);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(TODO_COLOR));
    nfui_nfobject_set_size(obj, 300, 22);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(TODO_COLOR));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)stFixed, obj, 160, 100);



    /* preview */
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PREVIEW", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(TODO_COLOR));
    nfui_nfobject_set_size(obj, 300, 22);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(TODO_COLOR));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)stFixed, obj, 20, 140);



    /* button */
    obj = nftool_normal_button_create_type1("EXPORT", 192);
    nfui_regi_post_event_callback(obj, post_export_button_event_cb);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)stFixed, obj, 20, 510);

    if(!ssm_check_access_auth(USR_AUTH_ARCHIVE))
        nfui_nfobject_disable(obj);


    obj = nftool_normal_button_create_type1("OK", 192);
    //nfui_regi_post_event_callback(obj, post_burn_button_event_cb);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)stFixed, obj, 218, 510);

    obj = nftool_normal_button_create_type1("CLOSE", 192);
    nfui_regi_post_event_callback(obj, post_close_button_event_cb);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)stFixed, obj, 416, 510);


    nfui_nfwindow_add((NFWINDOW*)stWin, stFixed);
    nfui_run_main_event_handler(stWin);
    nfui_nfobject_show(stWin);

    nfui_make_key_hierarchy((NFWINDOW*)stWin);

    return TRUE;
}
