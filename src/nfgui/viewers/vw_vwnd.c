
/*
 * vw_vwnd.c
 *
 * written by seongho
 *
 */


#include <math.h>
#include "vw_vwnd.h"

#include "support/color.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_color.h"
#include "support/event_loop.h"
#include "objects/nfbutton.h"

#include "support/util.h"
#include "support/nf_ui_page_manager.h"
#include "support/multi_language_support.h"

#include "tools/nf_ui_tool.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nfprogressbar.h"
#include "viewers/objects/nfimglabel.h"
#include "viewers/objects/nftable.h"

#include "vw_live_audio_input.h"
#include "vw_campwd.h"
#include "vw_dit_vca.h"
#include "vw_dit_dva.h"

#include "uxm.h"
#include "pos.h"
#include "smt.h"

#include "nf_api_live.h"

#define DBG_LEVEL       0
#define DBG_MODULE      "VWND"

#define MIN_SCALE_SIZE(a, b)                   (((a) < (b)) ? (a) : (b))

//#define DBG_DRAW_EXPOSE_REGION
//#define DBG_DRAW_AREA
//#define SUPPORT_WIN_SWITCH
//#define DBG_TEXT_ENABLE

#define VWND_POS_X                              (0)
#define VWND_POS_Y                              (0)

#define VWND_BORDER_SIZE                        (2)
#define VWND_FOCUS_BOX_SIZE                     (3)

//#define VWND_TITLE_WIDTH                      (230)
#if defined(GUI_32CH_SUPPORT)
#define VWND_VIDEO_WIDTH                        (280)
#define VWND_ALARM_WIDTH                        (280)
#else
#define VWND_VIDEO_WIDTH                        (300)
#define VWND_ALARM_WIDTH                        (300)
#endif
#define VWND_USER_WIDTH                         (300)
#define VWND_TIME_WIDTH                         (400)
#define VWND_CONNECTING_WIDTH                   (350)

#define VWND_LOGINFAIL_WIDTH                    (200)
#define VWND_LOGINFAIL_HEIGHT                   (34)

#define VWND_CABLE_TEST_WIDTH                   (200)
#define VWND_CABLE_TEST_HEIGHT                  (34)

#define VWND_DISK_FULL_GAP_X                    (56)
#define VWND_DISK_FULL_GAP_Y                    (80)
#define VWND_DISK_FULL_SIZE_W                   (50)
#define VWND_DISK_FULL_SIZE_H                   (20)

#define VWND_TITLE_GAP_Y                        (40)
#define VWND_SEQ_POS_Y                          (20)
#define VWND_PND_RATE_GAP_Y                     (64 - 15)
#define VWND_ICON_POS_Y                         (10)
#define VWND_ICON_GAP_2                         (2)
#define VWND_ICON_GAP_20                        (20)
#define VWND_ICON_GAP_30                        (30)
#define VWND_CONNECTING_GAP_Y                   (30 - 15)
#define VWND_CONN_BTN_GAP_Y                     (42)

#define MOUSE_LEFT_BUTTON                       (1)
#define MOUSE_RIGHT_BUTTON                      (3)

#if defined(_IPX_0412M4) || defined(_IPX_0824M4) || defined(_IPX_0824P4E) || \
	defined(_IPX_0412M4E) || defined(_IPX_0824M4E) || \
    defined(_IPX_3296P5)
#define VIDEO_BG_COLOR                          COLOR_IDX(3)    //color code 101010
#else
#define VIDEO_BG_COLOR                          COLOR_PRG_IDX(UX_COLOR_808080)
#endif

#define BORDER_ALPHA                            COLOR_PRG_IDX(UX_COLOR_000000)
#define BORDER_WHITE                            COLOR_PRG_IDX(UX_COLOR_FFFFFF)
#define BORDER_GRAY                             COLOR_PRG_IDX(UX_COLOR_394E4A)
#define BORDER_YELLOW                           COLOR_PRG_IDX(UX_COLOR_FFFF00)
#define BORDER_BLUE                             COLOR_PRG_IDX(UX_COLOR_0000FF)
#define BORDER_GREEN                            COLOR_PRG_IDX(UX_COLOR_00FF00)
#define BORDER_RED                              COLOR_PRG_IDX(UX_COLOR_FF0000)


#define FONT_H                              (40)

enum {
    LGD_PRE_REC_ICON = 0,
    LGD_TIMER_REC_ICON,
    LGD_ALARM_REC_ICON,
    LGD_MOTION_REC_ICON,
    LGD_PANIC_REC_ICON,
    LGD_MOTION_DETECT_ICON,
    LGD_SPEAKER_ICON,
#if !defined(_HDI_MODEL_UX) && !defined(_DVR_MODEL_UX)
    LGD_MIC_ICON,
#endif
    CNT_LEGEND
};

#define LEGEND_COL_WIDTH                        (310)

typedef struct _VWND_DIVSIZE {
    guint row;
    guint col;
}VWNDDIVSIZE;

typedef struct _VWND_ICONSIZE {
    gint img_w;
    gint img_h;
}VWNDICONSIZE;

typedef struct _VWND_LOGOINFO {
    gint img_w;
    gint img_h;
    GdkPixbuf *pbuf;
}VWNDLOGOINFO;

typedef struct _VWND_SWITCH{
    gint is_support;
    gint pressed;
    gint moved;
    gint from_x;
    gint from_y;
    gint from_win;
    gint to_x;
    gint to_y;
    gint to_win;
}VWND_SWITCH;

typedef struct _VWND_FISHEYE{
    gint is_support;
    gint pressed;
    gint moved;
    gint wpos;
    gint view_idx;
    gint from_x;
    gint from_y;
    gint to_x;
    gint to_y;
}VWND_FISHEYE;

typedef enum {
    VWND_REC_ICON = 0,
    VWND_SEQ_ICON,
    VWND_MOT_DETECT_ICON,
    VWND_FREEZE_ICON,
    VWND_SPEAKER_ICON,
    VWND_MIC_ICON,
    VWND_ALARM_ICON,
    VWND_NET_ICON,
    VWND_DISK_ICON,
    VWND_ARCH_ICON,
    VWND_PSTATUS_ICON,
    VWND_VCA_ICON,
    VWND_DLVA_ICON,
    VWND_DLVACNT_ICON,
    VWND_DVABX_ICON,
    VWND_ICON_CNT
}VWndIconIndex;

typedef enum {
    VWND_LLOGO = 0,     // legend logo
    VWND_VLOGO_100,     // video logo 100
    VWND_VLOGO_190,     // video logo 190
    VWND_LOGO_CNT
}VWndLogoIndex;

enum {
    SWITCH_STEP_NONE    = 0,
    SWITCH_STEP_PRESS   = 1 << 1,
    SWITCH_STEP_MOVE    = 1 << 2,
    SWITCH_STEP_RELEASE = 1 << 3
};

typedef enum _RES_E {
    RES_E_NONE,
    RES_E_1280xN = 0,
    RES_E_720PxN,
    RES_E_REST,
} RES_E;


static VWND_T *pvwnd = NULL;

static NFOBJECT *g_vwnd = NULL;
static NFOBJECT *g_vw_fixed = NULL;
static NFOBJECT *g_chk_btn[VSM_WIN_MAX] = {0,};

static GdkPixbuf *retry_img[NFOBJECT_STATE_COUNT];

static GdkDrawable *g_vw_drawable = NULL;
static GdkGC *g_vw_gc = NULL;

static VWNDDIVSIZE g_div_size[VSM_DIV_MAX]= {{1, 1}, {2, 2}, {3, 3}, {4, 4}, {3, 3}, {4, 4}, {6, 6}, {6, 6}};
static VWNDICONSIZE g_icon_size[VWND_ICON_CNT] = {0,};
static VWNDLOGOINFO g_logo_info[VWND_LOGO_CNT] = {0,};

static VWND_SWITCH g_switch;
static VWND_FISHEYE g_fisheye;

static gboolean user_event = FALSE;
static gboolean right_press_event = FALSE;

static GdkPixbuf *g_win_snap[VSM_WIN_MAX] = {0, };

static void init_icon_size();
static gboolean vwnd_draw_text(gchar *str, gint x, gint y, gint layout_color, gint font_color, nffont_type font_type, nfutil_pango_spacing_type spacing, gboolean multi_lang_support);
static void vwnd_get_position(VSM_DIV_E dtype, VSM_ID_E wpos, gint *x, gint *y);
static void vwnd_get_size(VSM_DIV_E dtype, VSM_ID_E wpos, gint *width, gint *height);
static VSM_ID_E vwnd_get_event_position(VSM_DIV_E dtype, gint x, gint y);
static gboolean vw_fixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data);
static gboolean post_login_fail_btn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
static gboolean vwnd_switch_channel_step(guchar step);
static gint _is_valid_win(gint mx, gint my);


/*
 * Internal functions..
 */

static /*inline*/ gint ICON_SIZE_W(VWndIconIndex index)
{
    return g_icon_size[index].img_w;
}


static /*inline*/ gint ICON_SIZE_H(VWndIconIndex index)
{
    return g_icon_size[index].img_h;
}

static /*inline*/ gint LOGO_SIZE_W(VWndLogoIndex index)
{
    return g_logo_info[index].img_w;
}


static /*inline*/ gint LOGO_SIZE_H(VWndLogoIndex index)
{
    return g_logo_info[index].img_h;
}

static void init_icon_size()
{
    GdkPixbuf *pbuf;

    nfui_get_image_size(IMG_TIMER_REC_ICON, &g_icon_size[VWND_REC_ICON].img_w, &g_icon_size[VWND_REC_ICON].img_h);
    nfui_get_image_size(IMG_SEQUECE_ICON, &g_icon_size[VWND_SEQ_ICON].img_w, &g_icon_size[VWND_SEQ_ICON].img_h);
    nfui_get_image_size(IMG_MOTION_DETECT_ICON, &g_icon_size[VWND_MOT_DETECT_ICON].img_w, &g_icon_size[VWND_MOT_DETECT_ICON].img_h);
    nfui_get_image_size(IMG_FREEZE_REC_ICON, &g_icon_size[VWND_FREEZE_ICON].img_w, &g_icon_size[VWND_FREEZE_ICON].img_h);
    nfui_get_image_size(IMG_SPEAKER_ICON, &g_icon_size[VWND_SPEAKER_ICON].img_w, &g_icon_size[VWND_SPEAKER_ICON].img_h);
    nfui_get_image_size(IMG_MIC_ICON, &g_icon_size[VWND_MIC_ICON].img_w, &g_icon_size[VWND_MIC_ICON].img_h);
    nfui_get_image_size(IMG_F_ALARM_OFF, &g_icon_size[VWND_ALARM_ICON].img_w, &g_icon_size[VWND_ALARM_ICON].img_h);
    nfui_get_image_size(IMG_F_NET_NORMAL, &g_icon_size[VWND_NET_ICON].img_w, &g_icon_size[VWND_NET_ICON].img_h);
    nfui_get_image_size(IMG_F_DISK_NORMAL, &g_icon_size[VWND_DISK_ICON].img_w, &g_icon_size[VWND_DISK_ICON].img_h);
    nfui_get_image_size(IMG_F_BACKUP_STATUS, &g_icon_size[VWND_ARCH_ICON].img_w, &g_icon_size[VWND_ARCH_ICON].img_h);
    nfui_get_image_size(IMG_PLAY_STATUS_FULL_STOP, &g_icon_size[VWND_PSTATUS_ICON].img_w, &g_icon_size[VWND_PSTATUS_ICON].img_h);

    pbuf = nfui_get_image_from_file((IMG_LIVE_VA_ICON_01), NULL);
    if (pbuf) {
        nfui_get_pixbuf_size(pbuf, &g_icon_size[VWND_VCA_ICON].img_w, &g_icon_size[VWND_VCA_ICON].img_h);
    }

    pbuf = nfui_get_image_from_file((IMG_LIVE_VA_ICON_01), NULL);
    if (pbuf) {
        nfui_get_pixbuf_size(pbuf, &g_icon_size[VWND_DLVA_ICON].img_w, &g_icon_size[VWND_DLVA_ICON].img_h);
    }    

    pbuf = nfui_get_image_from_file("live_human_count.png", NULL);
    if (pbuf) {
        nfui_get_pixbuf_size(pbuf, &g_icon_size[VWND_DLVACNT_ICON].img_w, &g_icon_size[VWND_DLVACNT_ICON].img_h);
    }

    pbuf = nfui_get_image_from_file((IMG_LIVE_VA_ICON_01), NULL);
    if (pbuf) {
        nfui_get_pixbuf_size(pbuf, &g_icon_size[VWND_DVABX_ICON].img_w, &g_icon_size[VWND_DVABX_ICON].img_h);
    }
}

static void init_switch()
{
#if 1 // _SUPPORT_WIN_SWITCH
    g_switch.is_support = 1;
#else
    g_switch.is_support = 0;
#endif
    g_switch.pressed = 0;
    g_switch.moved = 0;
    g_switch.from_x = -1;
    g_switch.from_y = -1;
    g_switch.from_win = -1;
    g_switch.to_x = -1;
    g_switch.to_y = -1;
    g_switch.to_win = -1;
}

static void init_fisheye()
{
#if 1 // _SUPPORT_WIN_SWITCH
    g_fisheye.is_support = 1;
#else
    g_fisheye.is_support = 0;
#endif
    g_fisheye.pressed = 0;
    g_fisheye.wpos = -1;
    g_fisheye.view_idx = -1;
    g_fisheye.moved = 0;
    g_fisheye.from_x = -1;
    g_fisheye.from_y = -1;
    g_fisheye.to_x = -1;
    g_fisheye.to_y = -1;
}

static void init_logo_info()
{
    GdkPixbuf *pbuf;

    pbuf = nfui_get_image_from_file((IMG_LEGEND_LOGO), NULL);
    if (pbuf) {
        nfui_get_pixbuf_size(pbuf, &g_logo_info[VWND_LLOGO].img_w, &g_logo_info[VWND_LLOGO].img_h);
        g_logo_info[VWND_LLOGO].pbuf = pbuf;
    }

    pbuf = nfui_get_image_from_file((IMG_VIDEO_LOGO_100), NULL);
    if (pbuf) {
        nfui_get_pixbuf_size(pbuf, &g_logo_info[VWND_VLOGO_100].img_w, &g_logo_info[VWND_VLOGO_100].img_h);
        g_logo_info[VWND_VLOGO_100].pbuf = pbuf;
    }

    pbuf = nfui_get_image_from_file((IMG_VIDEO_LOGO_190), NULL);
    if (pbuf) {
        nfui_get_pixbuf_size(pbuf, &g_logo_info[VWND_VLOGO_190].img_w, &g_logo_info[VWND_VLOGO_190].img_h);
        g_logo_info[VWND_VLOGO_190].pbuf = pbuf;
    }
}

static gboolean vwnd_draw_text(gchar *str, gint x, gint y, gint layout_color, gint font_color, nffont_type font_type, nfutil_pango_spacing_type spacing, gboolean multi_lang_support)
{
    PangoLayout *layout = NULL;
    PangoFontDescription *desc = NULL;
    PangoAttrList *attrs = NULL;
    PangoAttribute *attr = NULL;

    cairo_t *cr;

    if(!g_ascii_strcasecmp(str, "")) return FALSE;

    cr = gdk_cairo_create(g_vw_drawable);
    layout = pango_cairo_create_layout(cr);

    if (multi_lang_support && lookup_string(str))
    {
        pango_layout_set_text(layout, lookup_string(str), -1);
    }
    else
    {
        pango_layout_set_text(layout, str, -1);
    }

    desc = pango_font_description_from_string(nffont_get_pango_font(font_type));
    pango_layout_set_font_description(layout, desc);

    if (spacing != NORMAL_SPACING && str != NULL)
    {
        attrs = pango_layout_get_attributes(layout);
        if(attrs == NULL)
        {
            attrs = pango_attr_list_new();
            pango_layout_set_attributes(layout, attrs);
            pango_attr_list_unref(attrs);
        }
        attr = pango_attr_letter_spacing_new(nfutil_pango_get_spacing(spacing));
        attr->start_index = 0;

        if (lookup_string(str))
            attr->end_index = strlen(lookup_string(str))*sizeof(gchar);
        else
            attr->end_index = strlen(str)*sizeof(gchar);

        pango_attr_list_change(attrs, attr);
    }

    if (layout_color != -1)
    {
        gdk_cairo_set_source_color(cr, &UX_COLOR(layout_color));
        cairo_move_to (cr, x, y-1);
        pango_cairo_show_layout(cr, layout);
        cairo_move_to (cr, x+1, y);
        pango_cairo_show_layout(cr, layout);
        cairo_move_to (cr, x, y+1);
        pango_cairo_show_layout(cr, layout);
        cairo_move_to (cr, x-1, y);
        pango_cairo_show_layout(cr, layout);
        cairo_move_to (cr, x-1, y-1);
        pango_cairo_show_layout(cr, layout);
        cairo_move_to (cr, x+1, y-1);
        pango_cairo_show_layout(cr, layout);
        cairo_move_to (cr, x+1, y+1);
        pango_cairo_show_layout(cr, layout);
        cairo_move_to (cr, x-1, y+1);
        pango_cairo_show_layout(cr, layout);
    }

    if (font_color != -1)
    {
        gdk_cairo_set_source_color(cr, &UX_COLOR(font_color));
        cairo_move_to (cr, x, y);
        pango_cairo_show_layout(cr, layout);
    }

    cairo_destroy(cr);

    pango_font_description_free(desc);
    g_object_unref(layout);

    return TRUE;
}

static gboolean vwnd_draw_text_old(gchar *str, gint x, gint y, gint layout_color, gint font_color, nffont_type font_type, nfutil_pango_spacing_type spacing, gboolean multi_lang_support)
{
    GtkWidget *widget;
    PangoLayout *layout = NULL;
    PangoFontDescription *desc = NULL;
    PangoAttrList *attrs = NULL;
    PangoAttribute *attr = NULL;
    gchar* p = NULL;

    widget = ((NFWINDOW*)g_vwnd)->main_widget;

    if(!g_ascii_strcasecmp(str, "")) return FALSE;

    if(multi_lang_support)
        p = lookup_string(str);

    if(p)
    {
        layout = gtk_widget_create_pango_layout(widget, p);
    }
    else
    {
        layout = gtk_widget_create_pango_layout(widget, str);
    }

    desc = pango_font_description_from_string(nffont_get_pango_font(font_type));
    //pango_font_description_set_size(desc, PANGO_SCALE * (gint)(DISPLAY_IS_D1 ? 12 : 20));
    pango_layout_set_font_description(layout, desc);


    if(spacing != NORMAL_SPACING && str != NULL)
    {
        attrs = pango_layout_get_attributes(layout);
        if(attrs == NULL)
        {
            attrs = pango_attr_list_new();
            pango_layout_set_attributes(layout, attrs);
            pango_attr_list_unref(attrs);
        }
        attr = pango_attr_letter_spacing_new(nfutil_pango_get_spacing(spacing));
        attr->start_index = 0;
        if(p)
        {
            attr->end_index = strlen(p)*sizeof(gchar);
        }
        else
        {
            attr->end_index = strlen(str)*sizeof(gchar);
        }
        pango_attr_list_change(attrs, attr);
    }

    gdk_gc_set_rgb_fg_color(g_vw_gc, &UX_COLOR(layout_color));

    gdk_draw_layout(widget->window, g_vw_gc, x,     y-1,    layout);
    gdk_draw_layout(widget->window, g_vw_gc, x+1,   y,      layout);
    gdk_draw_layout(widget->window, g_vw_gc, x,     y+1,    layout);
    gdk_draw_layout(widget->window, g_vw_gc, x-1,   y,      layout);

    gdk_draw_layout(widget->window, g_vw_gc, x-1,   y-1,    layout);
    gdk_draw_layout(widget->window, g_vw_gc, x+1,   y-1,    layout);
    gdk_draw_layout(widget->window, g_vw_gc, x+1,   y+1,    layout);
    gdk_draw_layout(widget->window, g_vw_gc, x-1,   y+1,    layout);

    gdk_gc_set_rgb_fg_color(g_vw_gc, &UX_COLOR(font_color));
    gdk_draw_layout(widget->window, g_vw_gc, x, y, layout);

    g_object_unref(layout);
    pango_font_description_free(desc);

    return TRUE;
}

static gboolean vwnd_clear_area(gint x, gint y, gint width, gint height)
{
    gdk_gc_set_rgb_fg_color(g_vw_gc, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_000000)));
    gdk_draw_rectangle(g_vw_drawable, g_vw_gc, TRUE, x, y, width, height);
    return TRUE;
}

static gboolean vwnd_paint_area(gint x, gint y, gint width, gint height, guint color)
{
    gdk_gc_set_rgb_fg_color(g_vw_gc, &UX_COLOR(color));
    gdk_draw_rectangle(g_vw_drawable, g_vw_gc, TRUE, x, y, width, height);
    return TRUE;
}

static void _set_redraw_region(GdkEventExpose *expose_event)
{
    if (!expose_event) return;
    if (!expose_event->region) return;

    if (pvwnd->expose_region)
    {
        gdk_region_destroy (pvwnd->expose_region);
    }

    pvwnd->expose_region = gdk_region_copy(expose_event->region);
}

static gint _check_redraw_rect(GdkRectangle *area)
{
    GdkOverlapType type;

    if (!pvwnd->expose_region) return 1;

    type = gdk_region_rect_in(pvwnd->expose_region, area);

    if (type == GDK_OVERLAP_RECTANGLE_OUT) {
        return 1;
    }

    return 0;
}

static gint _control_position_for_resolution(VSM_DIV_E dtype, VSM_ID_E wpos, gint *x, gint *y)
{
    gchar strRes[32];

    scm_get_video_resolution(strRes);

    if (strstr(strRes, "800")) return -1;

    if (dtype == VSM_DIV6)
    {
        if (wpos == VSM_WIN_ID2)    wpos = VSM_WIN_ID3;
        else if (wpos > VSM_WIN_ID2)    wpos += 3;
    }
    else if (dtype == VSM_DIV8)
    {
        if (wpos == VSM_WIN_ID2)        wpos = VSM_WIN_ID4;
        else if (wpos == VSM_WIN_ID3)   wpos = VSM_WIN_ID8;
        else if (wpos > VSM_WIN_ID3)        wpos += 8;
    }

    if (strstr(strRes, "1280") || strstr(strRes, "720p"))
    {
        if (((dtype == VSM_DIV6) ||(dtype == VSM_DIV9)) && (vsm_get_omode() == OMODE_FULL))
        {
            switch(wpos)
            {
                case VSM_WIN_ID2:   case VSM_WIN_ID5:   case VSM_WIN_ID8:
                    *x += 5;
                break;
                case VSM_WIN_ID3:   case VSM_WIN_ID6:   case VSM_WIN_ID9:
                    *x -= 5;
                break;
                default:
                break;
            }
        }
    }
    else if (strstr(strRes, "800"))
    {
        if (((dtype == VSM_DIV6) ||(dtype == VSM_DIV9)) && (vsm_get_omode() == OMODE_FULL))
        {
            switch(wpos)
            {
                case VSM_WIN_ID2:   case VSM_WIN_ID5:   case VSM_WIN_ID8:
                    *x -= 8;
                break;
                case VSM_WIN_ID3:   case VSM_WIN_ID6:   case VSM_WIN_ID9:
                    *x += 6;
                break;
                default:
                break;
            }
        }
        else if (((dtype == VSM_DIV8) ||(dtype == VSM_DIV16)) && (vsm_get_omode() == OMODE_NORMAL))
        {
            switch(wpos)
            {
                case VSM_WIN_ID2:   case VSM_WIN_ID6:   case VSM_WIN_ID10: case VSM_WIN_ID14:
                case VSM_WIN_ID4:   case VSM_WIN_ID8:   case VSM_WIN_ID12: case VSM_WIN_ID16:
                    *x -= 10;
                break;
                default:
                break;
            }
        }
    }

    return 0;
}

static gint _control_size_for_resolution(VSM_DIV_E dtype, VSM_ID_E wpos, gint *width, gint *height)
{
    gchar strRes[32];

    scm_get_video_resolution(strRes);

    if (strstr(strRes, "800")) return -1;

    if (dtype == VSM_DIV6)
    {
        if (wpos == VSM_WIN_ID2)    wpos = VSM_WIN_ID3;
        else if (wpos > VSM_WIN_ID2) wpos += 3;
    }
    else if (dtype == VSM_DIV8)
    {
        if (wpos == VSM_WIN_ID2)        wpos = VSM_WIN_ID4;
        else if (wpos == VSM_WIN_ID3)   wpos = VSM_WIN_ID8;
        else if (wpos > VSM_WIN_ID3)        wpos += 8;
    }

    if (strstr(strRes, "1280") || strstr(strRes, "720p"))
    {
        if (((dtype == VSM_DIV6) ||(dtype == VSM_DIV9)) && (vsm_get_omode() == OMODE_FULL))
        {
            if ((dtype == VSM_DIV6) && (wpos == VSM_WIN_ID1))
            {
                *width -= 10;
                return 0;
            }

            switch(wpos)
            {
                case VSM_WIN_ID1:   case VSM_WIN_ID4:   case VSM_WIN_ID7:
                    *width += 5;
                break;
                case VSM_WIN_ID2:   case VSM_WIN_ID5:   case VSM_WIN_ID8:
                    *width -= 12;
                break;
                case VSM_WIN_ID3:   case VSM_WIN_ID6:   case VSM_WIN_ID9:
                    *width += 3;
                break;
                default:
                break;
            }
        }
    }
    else if (strstr(strRes, "800"))
    {
        if (((dtype == VSM_DIV6) ||(dtype == VSM_DIV9)) && (vsm_get_omode() == OMODE_FULL))
        {
            if ((dtype == VSM_DIV6) && (wpos == VSM_WIN_ID1))
            {
                *width += 4;
                return 0;
            }

            switch(wpos) {
                case VSM_WIN_ID1:   case VSM_WIN_ID4:   case VSM_WIN_ID7:
                case VSM_WIN_ID3:   case VSM_WIN_ID6:   case VSM_WIN_ID9:
                    *width -= 8;
                break;
                case VSM_WIN_ID2:   case VSM_WIN_ID5:   case VSM_WIN_ID8:
                    *width += 12;
                break;
                default:
                break;
            }
        }
        else if (((dtype == VSM_DIV8) ||(dtype == VSM_DIV16)) && (vsm_get_omode() == OMODE_NORMAL))
        {
            if ((dtype == VSM_DIV8) && (wpos == VSM_WIN_ID1))
            {
                *width -= 12;
                return 0;
            }

            switch(wpos)
            {
                case VSM_WIN_ID1:   case VSM_WIN_ID5:   case VSM_WIN_ID9: case VSM_WIN_ID13:
                case VSM_WIN_ID3:   case VSM_WIN_ID7:   case VSM_WIN_ID11: case VSM_WIN_ID15:
                    *width -= 10;
                break;
                case VSM_WIN_ID2:   case VSM_WIN_ID6:   case VSM_WIN_ID10: case VSM_WIN_ID14:
                case VSM_WIN_ID4:   case VSM_WIN_ID8:   case VSM_WIN_ID12: case VSM_WIN_ID16:
                    *width += 8;
                break;
                default:
                break;
            }
        }
    }

    return 0;
}

static gint _control_col_border_for_resolution(VSM_DIV_E dtype, gint line_num, gint *x1, gint *x2)
{
    gchar strRes[32];

    scm_get_video_resolution(strRes);

    if (strstr(strRes, "800")) return -1;

    if (strstr(strRes, "1280") || strstr(strRes, "720p"))
    {
        if (((dtype == VSM_DIV6) ||(dtype == VSM_DIV9)) && (vsm_get_omode() == OMODE_FULL))
        {
            //if (dtype == VSM_DIV6) return -1;

            if (line_num == 0)
            {
                *x1 += 5, *x2 += 5;
            }
            else if (line_num == 1)
            {
                *x1 -= 12, *x2 -= 12;
            }
        }
    }
    else if (strstr(strRes, "800"))
    {
        if (((dtype == VSM_DIV6) ||(dtype == VSM_DIV9)) && (vsm_get_omode() == OMODE_FULL))
        {
            if (line_num == 0)
            {
                *x1 -= 8, *x2 -= 8;
            }
            else if (line_num == 1)
            {
                *x1 += 14, *x2 += 14;
            }
        }
        else if (((dtype == VSM_DIV8) ||(dtype == VSM_DIV16)) && (vsm_get_omode() == OMODE_NORMAL))
        {
            if (line_num == 0)
            {
                *x1 -= 12, *x2 -= 12;
            }
            else if (line_num == 1)
            {
                *x1 += 12, *x2 += 12;
            }
            else if (line_num == 2)
            {
                *x1 -= 12, *x2 -= 12;
            }
        }
    }

    return 0;
}

static gint _control_row_border_for_resolution(VSM_DIV_E dtype, gint line_num, gint *y1, gint *y2)
{
    return 0;
}

static gint _control_row_border_for_align(VSM_DIV_E dtype, gint *height)
{
	gchar strRes[32];

    scm_get_video_resolution(strRes);

	// IPXM4 = 4 align
	if (strstr(strRes, "1080"))
	{
		if( dtype == VSM_DIV4 )
		{
			if(vsm_get_omode() == OMODE_NORMAL)
				*height += 2;
		}
		else if( dtype == VSM_DIV8 )
		{
			if(vsm_get_omode() == OMODE_NORMAL)
				*height += 1;
			else if(vsm_get_omode() == OMODE_FULL)
				*height += 1;
		}
	}
	else if (strstr(strRes, "1280") || strstr(strRes, "720p"))
	{
		if( dtype == VSM_DIV4 )
		{
			if(vsm_get_omode() == OMODE_NORMAL)
				*height += 2;
		}
		else if( dtype == VSM_DIV8 )
		{
			if(vsm_get_omode() == OMODE_NORMAL)
				*height += 1;
			else if(vsm_get_omode() == OMODE_FULL)
				*height += 1;
		}
	}

	return 0;
}

static void vwnd_get_position(VSM_DIV_E dtype, VSM_ID_E wpos, gint *x, gint *y)
{
    gint w, h;
    gint row, col;

    *x = VWND_POS_X;
    *y = VWND_POS_Y;

    if(dtype == VSM_DIV1 || wpos == VSM_WIN_ID1)
        return;

    vwnd_get_size(dtype, wpos, &w, &h);

    if(dtype == VSM_DIV6) {
        if(wpos == VSM_WIN_ID2)      wpos = VSM_WIN_ID3;
        else                         wpos += 3;
    } else if(dtype == VSM_DIV8) {
        if(wpos == VSM_WIN_ID2)      wpos = VSM_WIN_ID4;
        else if(wpos == VSM_WIN_ID3) wpos = VSM_WIN_ID8;
        else                         wpos += 8;
    }

    row = wpos / g_div_size[dtype].row;
    col = wpos % g_div_size[dtype].col;

    *x += (w * col);
    *y += (h * row);
}

static void vwnd_get_size(VSM_DIV_E dtype, VSM_ID_E wpos, gint *width, gint *height)
{
    if(dtype == VSM_DIV1) {
        *width = pvwnd->plt_w;
        *height = pvwnd->plt_h;
    }else {
        *width = (pvwnd->plt_w / g_div_size[dtype].col);
        *height = (pvwnd->plt_h / g_div_size[dtype].row);

		_control_row_border_for_align(dtype, height);

        if(dtype == VSM_DIV6) {
            if(wpos == VSM_WIN_ID1) {
                *width *= 2;
                *height *= 2;
            }
        }else if(dtype == VSM_DIV8) {
            if(wpos == VSM_WIN_ID1) {
                *width *= 3;
                *height *= 3;
            }
        }
    }
}


static VSM_ID_E vwnd_get_event_position(VSM_DIV_E dtype, gint x, gint y)
{
    VSM_DIV_E tmp_div = dtype;

    gint w = 0, h = 0;
    gint row, col;

    VSM_ID_E tmp_win_id, win_id;

    if (dtype == VSM_DIV6)  tmp_div = VSM_DIV9;
    if (dtype == VSM_DIV8)  tmp_div = VSM_DIV16;

    vwnd_get_size(tmp_div, VSM_WIN_ID1, &w, &h);

    row = y / h;
    col = x / w;

    tmp_win_id = (row * g_div_size[dtype].row) + col;

    if (dtype == VSM_DIV6)
    {
        switch(tmp_win_id)
        {
            case 0: case 1: case 3: case 4:
                win_id = 0;
            break;
            case 2: win_id = 1; break;
            case 5: win_id = 2; break;
            case 6: win_id = 3; break;
            case 7: win_id = 4; break;
            case 8: win_id = 5; break;
            default:
                g_assert(0);
            break;
        }
    }
    else if (dtype == VSM_DIV8)
    {
        switch(tmp_win_id)
        {
            case 0: case 1: case 2: case 4: case 5:
            case 6: case 8: case 9: case 10:
                win_id = 0;
            break;
            case 3:     win_id = 1; break;
            case 7:     win_id = 2; break;
            case 11:    win_id = 3; break;
            case 12:    win_id = 4; break;
            case 13:    win_id = 5; break;
            case 14:    win_id = 6; break;
            case 15:    win_id = 7; break;
            default:
                g_assert(0);
            break;
        }
    }
    else
        win_id = tmp_win_id;

    return win_id;
}

static gint vwnd_get_fisheye_view_index(VSM_DIV_E dtype, gint x, gint y)
{
    CamItxFisheyeData fisheye_data;
    NF_FISHEYE_VIDEO_PARAM vparam;
    gint idx = -1;

    VSM_ID_E wpos;
    gint ch;
    gint win_x, win_y;
    gint win_w, win_h;
    
    if (!_is_valid_win(x, y)) return -1;

#if defined(GUI_8CH_SUPPORT)
    if (pvwnd->dtype == VSM_DIV9)
    {
        if ((x >= pvwnd->plt_w/3*2)&&(y >= pvwnd->plt_h/3*2)) return -1;
    }
#endif

    wpos = vwnd_get_event_position(dtype, x, y);
    ch = vsm_get_current_ch(wpos);

    if (!nf_live_fisheye_is_support(ch)) return -1;

    vwnd_get_position(dtype, wpos, &win_x, &win_y);
    vwnd_get_size(dtype, wpos, &win_w, &win_h);

	memset(&fisheye_data, 0x00, sizeof(CamItxFisheyeData));
	DAL_get_camera_itx_fisheye_data(&fisheye_data, ch);
	if (!fisheye_data.act) return -1;

    memset(&vparam, 0x00, sizeof(NF_FISHEYE_VIDEO_PARAM));
    nf_live_fisheye_get_video_param(ch, &vparam);

    if (vparam.view_type == NF_FISHEYE_VIEW_SIGLE)
    {
        idx = 0;
    }
    else if (vparam.view_type == NF_FISHEYE_VIEW_QUAD)
    {
        x -= win_x;
        y -= win_y;

        if ((x < win_w/2) && (y < win_h/2)) idx = 0;
        if ((x > win_w/2) && (y < win_h/2)) idx = 1;
        if ((x < win_w/2) && (y > win_h/2)) idx = 2;
        if ((x > win_w/2) && (y > win_h/2)) idx = 3;
    }
    else if (vparam.view_type == NF_FISHEYE_VIEW_PANORAMA)
    {
        idx = 0;
    }

    return idx;
}

static void _cmd_border(VWND_T *p, gint color_index)
{
    gint x1, y1, x2, y2;
    gint w = 0, h = 0;
    guint i;
    VSM_DIV_E dtype = p->dtype;
    guint border_color = 0;
	gchar strRes[32];

    switch(color_index)
    {
        case -1:
            border_color = BORDER_ALPHA;
        break;
        case 0:
            border_color = BORDER_WHITE;
        break;
        case 1:
            border_color = BORDER_GRAY;
        break;
        case 2:
            border_color = BORDER_YELLOW;
        break;
        case 3:
            border_color = BORDER_BLUE;
        break;
        case 4:
            border_color = BORDER_GREEN;
        break;
        case 5:
            border_color = BORDER_RED;
        break;
    }

    gdk_gc_set_rgb_fg_color(g_vw_gc, &UX_COLOR(border_color));

    scm_get_video_resolution(strRes);

	if ((strstr(strRes, "720"))|| (strstr(strRes, "1024")))
		gdk_gc_set_line_attributes(g_vw_gc, 5, GDK_LINE_SOLID, GDK_CAP_NOT_LAST, GDK_JOIN_MITER);
    else
		gdk_gc_set_line_attributes(g_vw_gc, VWND_BORDER_SIZE, GDK_LINE_SOLID, GDK_CAP_NOT_LAST, GDK_JOIN_MITER);

    w = p->plt_w / g_div_size[dtype].col;
    h = p->plt_h / g_div_size[dtype].row;

	_control_row_border_for_align(dtype, &h);
    x1 = x2 = 0;

    for (i = 0; i < g_div_size[dtype].col-1; i++)
    {
        y1 = 0;
        y2 = p->plt_h;

        if (dtype == VSM_DIV6)
        {
            if (i < 1) y1 = h*2;
        }
        else if (dtype == VSM_DIV8)
        {
            if (i < 2) y1 = h*3;
        }
        else if (dtype == VSM_DIV36)
        {
            if (i > 1) y2 -= h;
        }

        x1 += w;
        x2 += w;

        //_control_col_border_for_resolution(dtype, i, &x1, &x2);
        gdk_draw_line(g_vw_drawable, g_vw_gc, x1, y1, x2, y2);

//      g_message("%s, %d, x1:%d, y1:%d, x2:%d, y2:%d", __FUNCTION__, __LINE__, x1, y1, x2, y2);
    }

    y1 = y2 = 0;

    for(i=0; i<g_div_size[dtype].row-1; i++)
    {
        x1 = 0;
        x2 = p->plt_w;

        if (dtype == VSM_DIV6)
        {
            if (i < 1) x1 = w*2;
        }
        else if (dtype == VSM_DIV8)
        {
            if (i < 2) x1 = w*3;
        }

		y1 += h;
		y2 += h;

		//_control_row_border_for_resolution(dtype, i, &y1, &y2);
		gdk_draw_line(g_vw_drawable, g_vw_gc, x1, y1, x2, y2);

// 		g_message("%s, %d, x1:%d, y1:%d, x2:%d, y2:%d", __FUNCTION__, __LINE__, x1, y1, x2, y2);
    }
}

static gboolean vwnd_draw_border(VWND_T *p)
{
    if (!p->border) return FALSE;

    DMSG(1, "__VWND_DRAW_BORDER");

    _cmd_border(p, (gint)p->border_color);
    return TRUE;
}

static gboolean vwnd_erase_border(VWND_T *p)
{
    _cmd_border(p, -1);
    return TRUE;
}

static void _cmd_focus_box(VWND_T *p, VSM_ID_E wpos, guint color_index)
{
    gint x, y;
    gint width, height;
    VSM_DIV_E dtype = p->dtype;

    vwnd_get_position(dtype, wpos, &x, &y);
    //_control_position_for_resolution(dtype, wpos, &x, &y);

    vwnd_get_size(dtype, wpos, &width, &height);
    //_control_size_for_resolution(dtype, wpos, &width, &height);

    gdk_gc_set_rgb_fg_color(g_vw_gc, &UX_COLOR(color_index));
    gdk_gc_set_line_attributes(g_vw_gc, VWND_FOCUS_BOX_SIZE, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);
    gdk_draw_rectangle(g_vw_drawable, g_vw_gc, FALSE, x, y, width, height);
}

static gboolean vwnd_draw_focus_box(VWND_T *p, VSM_ID_E wpos)
{
    if (!p->winfo[wpos].is_focus) return FALSE;

    DMSG(1, "__VWND_DRAW_FOCUS_BOX");

    _cmd_focus_box(p, wpos, COLOR_IDX(305));
    return TRUE;
}

static gboolean vwnd_draw_focus_move_from(VWND_T *p, gint wpos)
{
    if (!g_switch.moved) return FALSE;
    if (g_switch.from_win != wpos) return FALSE;

    DMSG(1, "__VWND_DRAW_FOCUS_MOVE_FROM");

    _cmd_focus_box(p, wpos, COLOR_IDX(306));
    return TRUE;
}

static gboolean vwnd_draw_focus_move_to(VWND_T *p, gint wpos)
{
    gint x, y;
    gint width, height;
    VSM_DIV_E dtype = p->dtype;

    if (!g_switch.moved) return FALSE;
    if (g_switch.to_win != wpos) return FALSE;

    DMSG(1, "__VWND_DRAW_FOCUS_MOVE_TO");

    vwnd_get_position(dtype, wpos, &x, &y);
    //_control_position_for_resolution(dtype, wpos, &x, &y);

    vwnd_get_size(dtype, wpos, &width, &height);
    //_control_size_for_resolution(dtype, wpos, &width, &height);

    gdk_gc_set_rgb_fg_color(g_vw_gc, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_000000)));
    gdk_gc_set_line_attributes(g_vw_gc, VWND_FOCUS_BOX_SIZE, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);
    gdk_draw_rectangle(g_vw_drawable, g_vw_gc, FALSE, x, y, width, height);

    gdk_gc_set_rgb_fg_color(g_vw_gc, &UX_COLOR(COLOR_IDX(306)));
    gdk_gc_set_line_attributes (g_vw_gc, VWND_FOCUS_BOX_SIZE, GDK_LINE_ON_OFF_DASH, 0, 0);
    gdk_gc_set_dashes (g_vw_gc, 0, "\4\4", 2);
    gdk_draw_rectangle(g_vw_drawable, g_vw_gc, FALSE, x, y, width, height);

    return TRUE;
}

static gboolean vwnd_erase_focus_box(VWND_T *p, VSM_ID_E wpos)
{
    _cmd_focus_box(p, wpos, COLOR_PRG_IDX(UX_COLOR_000000));
    return TRUE;
}

static void _set_postion_cam_title(VWND_T *p, VSM_ID_E wpos)
{
    guint string_w = 0, string_h = 0;
    gint x = 0, y = 0, w = 0, h = 0;
    VSM_DIV_E dtype = p->dtype;

    vwnd_get_position(dtype, wpos, &x, &y);

    x += 16;
    y += 16;

//  string_w = VWND_TITLE_WIDTH;
    string_w = p->winfo[wpos].micIcon_area.x - x;
    string_h = FONT_H;

    p->winfo[wpos].title_area.x = x;
    p->winfo[wpos].title_area.y = y;
    p->winfo[wpos].title_area.width = string_w;
    p->winfo[wpos].title_area.height = string_h;

#ifdef DBG_DRAW_AREA
    vwnd_paint_area(x, y, string_w, string_h, COLOR_PRG_IDX(UX_COLOR_FF0000));
#endif
}

static gboolean vwnd_draw_cam_title(VWND_T *p, VSM_ID_E wpos)
{
    nfutil_pango_spacing_type spacing = NORMAL_SPACING;
    gint x = 0, y = 0;
    guint string_w = 0, string_h = 0;
    VSM_DIV_E dtype = p->dtype;
    gchar titleBuf[VWND_TEXT_LEN+1];
    gint valid_cnt, tmp_w;

    if (p->osd_off) return FALSE;
    if (!strlen(p->winfo[wpos].title_text)) return FALSE;
    if (_check_redraw_rect(&p->winfo[wpos].title_area)) return FALSE;

    DMSG(1, "__VWND_DRAW_CAM_TITLE");

    vwnd_get_position(dtype, wpos, &x, &y);

    memset(titleBuf, 0x00, sizeof(titleBuf));

    string_w = nfutil_string_width(0, g_vw_drawable, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), p->winfo[wpos].title_text, spacing);

    if (string_w >= p->winfo[wpos].title_area.width)
    {
        tmp_w = nfutil_string_width(0, g_vw_drawable, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), "...", spacing);
        valid_cnt = nfutil_string_get_valid_count(0, g_vw_drawable, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), p->winfo[wpos].title_text, p->winfo[wpos].title_area.width-tmp_w);
        g_utf8_strncpy(titleBuf, p->winfo[wpos].title_text, valid_cnt);
        strcat(titleBuf, "...");
    }
    else
        strcpy(titleBuf, p->winfo[wpos].title_text);

    x += 16;
    y += 16;

    vwnd_draw_text(titleBuf, x, y, COLOR_IDX(308), COLOR_IDX(307), NFFONT_MEDIUM_SEMI, spacing, FALSE);

    return TRUE;
}

static void _set_postion_video_status(VWND_T *p, VSM_ID_E wpos)
{
    guint string_w = 0, string_h = 0;
    guint image_w = 0, image_h = 0;
    gint x = 0, y = 0, w = 0, h = 0;
    VSM_DIV_E dtype = p->dtype;

    vwnd_get_position(dtype, wpos, &x, &y);
    vwnd_get_size(dtype, wpos, &w, &h);

    string_w = VWND_VIDEO_WIDTH;
    string_h = FONT_H;

    x += ((w - string_w) / 2);
    y += ((h - string_h) / 2);

    p->winfo[wpos].video_area.x = x;
    p->winfo[wpos].video_area.y = y;
    p->winfo[wpos].video_area.width = string_w;
    p->winfo[wpos].video_area.height = string_h;

#ifdef DBG_DRAW_AREA
    vwnd_paint_area(x, y, string_w, string_h, COLOR_PRG_IDX(UX_COLOR_FF0000));
#endif

    if ((p->dtype == VSM_DIV1)
        || ((p->dtype == VSM_DIV6) && (wpos == VSM_WIN_ID1))
        || ((p->dtype == VSM_DIV8) && (wpos == VSM_WIN_ID1)))
    {
        if (!g_logo_info[VWND_VLOGO_190].pbuf) return;

        vwnd_get_position(dtype, wpos, &x, &y);

        image_w = LOGO_SIZE_W(VWND_VLOGO_190);
        image_h = LOGO_SIZE_H(VWND_VLOGO_190);

        x += ((w - image_w) / 2);
        y += ((h - image_h) / 2);
    }
    else
    {
        if (!g_logo_info[VWND_VLOGO_100].pbuf) return;

        vwnd_get_position(dtype, wpos, &x, &y);

        image_w = LOGO_SIZE_W(VWND_VLOGO_100);
        image_h = LOGO_SIZE_H(VWND_VLOGO_100);

        x += ((w - image_w) / 2);
        y += ((h - image_h) / 2);
    }

    p->winfo[wpos].vlogo_area.x = x;
    p->winfo[wpos].vlogo_area.y = y;
    p->winfo[wpos].vlogo_area.width = image_w;
    p->winfo[wpos].vlogo_area.height = image_h;

#ifdef DBG_DRAW_AREA
    vwnd_paint_area(x, y, image_w, image_h, COLOR_PRG_IDX(UX_COLOR_FF0000));
#endif
}

static gboolean vwnd_draw_video_status(VWND_T *p, VSM_ID_E wpos)
{
    guint string_w = 0, string_h = 0;
    gint x = 0, y = 0, w = 0, h = 0;
    VSM_DIV_E dtype = p->dtype;
    VIDEO_ST vst = p->winfo[wpos].video_st;
    gchar text[VWND_TEXT_LEN+1];
    
    if (vst == VST_NONE) return FALSE;
    if ((_check_redraw_rect(&p->winfo[wpos].video_area))
        && (_check_redraw_rect(&p->winfo[wpos].vlogo_area)))
        return FALSE;

    DMSG(1, "__VWND_DRAW_VIDEO_TEXT");

    vwnd_get_position(dtype, wpos, &x, &y);
    vwnd_get_size(dtype, wpos, &w, &h);

    memset(text, 0x00, sizeof(text));

    switch(vst) {
        case VST_NO_VIDEO:
        {
            if ((p->dtype == VSM_DIV1)
                || ((p->dtype == VSM_DIV6) && (wpos == VSM_WIN_ID1))
                || ((p->dtype == VSM_DIV8) && (wpos == VSM_WIN_ID1)))
            {
                if (g_logo_info[VWND_VLOGO_190].pbuf)
                {
                    x += ((w - LOGO_SIZE_W(VWND_VLOGO_190)) / 2);
                    y += ((h - LOGO_SIZE_H(VWND_VLOGO_190)) / 2);

                    nfutil_draw_pixbuf(g_vw_drawable, g_vw_gc, g_logo_info[VWND_VLOGO_190].pbuf, x, y, LOGO_SIZE_W(VWND_VLOGO_190), LOGO_SIZE_H(VWND_VLOGO_190), NFALIGN_CENTER, 0);
                    return TRUE;
                }
            }
            else
            {
                if (g_logo_info[VWND_VLOGO_100].pbuf)
                {
                    x += ((w - LOGO_SIZE_W(VWND_VLOGO_100)) / 2);
                    y += ((h - LOGO_SIZE_H(VWND_VLOGO_100)) / 2);

                    nfutil_draw_pixbuf(g_vw_drawable, g_vw_gc, g_logo_info[VWND_VLOGO_100].pbuf, x, y, LOGO_SIZE_W(VWND_VLOGO_100), LOGO_SIZE_H(VWND_VLOGO_100), NFALIGN_CENTER, 0);
                    return TRUE;
                }
            }

            strcpy(text, STRING_NO_VIDEO);
        }
        break;
        case VST_VLOSS:
            strcpy(text, STRING_VLOSS);
        break;
        case VST_NO_REC:
            strcpy(text, STRING_NO_REC);
        break;
        case VST_END_VIDEO:
            strcpy(text, STRING_END_VIDEO);
        break;
        case VST_COVERT:
            strcpy(text, STRING_COVERT);
        break;
        case VST_OVERLAPPED:
            strcpy(text, STRING_OVERLAPPED);
        break;
        case VST_UNKNOWN_DEV:
            strcpy(text, STRING_UNKNOWN_DEV);
        break;
        case VST_UNSUPPORT_CAM:
            strcpy(text, STRING_UNSUPPORT_CAM);
        break;
        case VST_FAIL_CONNECT:
            strcpy(text, STRING_FAIL_CONNECT);
        break;
        case VST_FAIL_LOGIN:
            strcpy(text, STRING_FAIL_LOGIN);
        break;
        case VST_FAIL_CONFIG:
            strcpy(text, STRING_FAIL_CONFIG);
        break;
    }

    string_w = nfutil_string_width(1, g_vw_drawable, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), text, NORMAL_SPACING);
    string_h = nfutil_string_height(g_vw_drawable, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), text, NORMAL_SPACING);

    x += ((w - string_w) / 2);
    y += ((h - string_h) / 2);

    vwnd_draw_text(text, x, y, COLOR_IDX(308), COLOR_IDX(307), NFFONT_MEDIUM_SEMI, NORMAL_SPACING, TRUE);

    return TRUE;
}

static void _set_postion_alarm_text(VWND_T *p, VSM_ID_E wpos)
{
    guint string_w = 0, string_h = 0;
    gint x = 0, y = 0, w = 0, h = 0;
    VSM_DIV_E dtype = p->dtype;

    vwnd_get_position(dtype, wpos, &x, &y);
    vwnd_get_size(dtype, wpos, &w, &h);

    string_w = VWND_ALARM_WIDTH;
    string_h = FONT_H;

    x += ((w - string_w) / 2);
    y += (h - string_h);

    p->winfo[wpos].alarm_area.x = x;
    p->winfo[wpos].alarm_area.y = y;
    p->winfo[wpos].alarm_area.width = string_w;
    p->winfo[wpos].alarm_area.height = string_h;

#ifdef DBG_DRAW_AREA
    vwnd_paint_area(x, y, string_w, string_h, COLOR_PRG_IDX(UX_COLOR_FF0000));
#endif
}

static gboolean vwnd_draw_alarm_text(VWND_T *p, VSM_ID_E wpos)
{
    guint string_w = 0, string_h = 0;
    gint x = 0, y = 0, w = 0, h = 0;
    VSM_DIV_E dtype = p->dtype;
    gchar *text = p->winfo[wpos].alarm_text;

    if (p->osd_off) return FALSE;
    if (!strlen(text)) return FALSE;
    if (_check_redraw_rect(&p->winfo[wpos].alarm_area)) return FALSE;

    DMSG(1, "__VWND_DRAW_ALARM_TEXT");

    vwnd_get_position(dtype, wpos, &x, &y);
    vwnd_get_size(dtype, wpos, &w, &h);

    string_w = nfutil_string_width(0, g_vw_drawable, nffont_get_pango_font(NFFONT_LARGE_SEMI), text, NORMAL_SPACING);
    string_h = nfutil_string_height(g_vw_drawable, nffont_get_pango_font(NFFONT_LARGE_SEMI), text, NORMAL_SPACING);

    x += ((w - string_w) / 2);
    y += (h - string_h);

    vwnd_draw_text(text, x, y, COLOR_IDX(308), COLOR_IDX(307), NFFONT_LARGE_SEMI, NORMAL_SPACING, TRUE);

    return TRUE;
}

static void _set_postion_rec_icon(VWND_T *p, VSM_ID_E wpos)
{
    gint x = 0, y = 0, w = 0, h = 0;
    RECORD_ICON icon = p->winfo[wpos].rec_mode;
    VSM_DIV_E dtype = p->dtype;

    vwnd_get_position(dtype, wpos, &x, &y);
    vwnd_get_size(dtype, wpos, &w, &h);

    x += (w - ICON_SIZE_W(VWND_SEQ_ICON)/2 - ICON_SIZE_W(VWND_REC_ICON));
    y += VWND_ICON_POS_Y;

    p->winfo[wpos].recIcon_area.x = x;
    p->winfo[wpos].recIcon_area.y = y;
    p->winfo[wpos].recIcon_area.width = ICON_SIZE_W(VWND_REC_ICON);
    p->winfo[wpos].recIcon_area.height = ICON_SIZE_H(VWND_REC_ICON);

#ifdef DBG_DRAW_AREA
    vwnd_paint_area(x, y, ICON_SIZE_W(VWND_REC_ICON), ICON_SIZE_H(VWND_REC_ICON), COLOR_PRG_IDX(UX_COLOR_FF0000));
#endif
}

static gboolean vwnd_draw_rec_icon(VWND_T *p, VSM_ID_E wpos)
{
    gint x = 0, y = 0, w = 0, h = 0;
    RECORD_ICON icon = p->winfo[wpos].rec_mode;
    VSM_DIV_E dtype = p->dtype;

    g_return_val_if_fail(icon >= REC_ICON_NONE, FALSE);
    g_return_val_if_fail(icon <= REC_ICON_PANIC, FALSE);

    if (p->osd_off) return FALSE;
    if (!p->winfo[wpos].rec_mode) return FALSE;
    if (_check_redraw_rect(&p->winfo[wpos].recIcon_area)) return FALSE;

    DMSG(1, "__VWND_DRAW_REC_ICON");

    vwnd_get_position(dtype, wpos, &x, &y);
    vwnd_get_size(dtype, wpos, &w, &h);

    x += (w - ICON_SIZE_W(VWND_SEQ_ICON)/2 - ICON_SIZE_W(VWND_REC_ICON));
    y += VWND_ICON_POS_Y;

    switch(icon) {
        case REC_ICON_PRE:
            nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_PRE_REC_ICON, (gint)x, (gint)y, ICON_SIZE_W(VWND_REC_ICON), ICON_SIZE_H(VWND_REC_ICON), NFALIGN_LEFT, 0);
            break;
        case REC_ICON_CONT:
            nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_TIMER_REC_ICON, (gint)x, (gint)y, ICON_SIZE_W(VWND_REC_ICON), ICON_SIZE_H(VWND_REC_ICON), NFALIGN_LEFT, 0);
            break;
        case REC_ICON_ALARM:
            nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_ALARM_REC_ICON, (gint)x, (gint)y, ICON_SIZE_W(VWND_REC_ICON), ICON_SIZE_H(VWND_REC_ICON), NFALIGN_LEFT, 0);
            break;
        case REC_ICON_MOT:
            nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_MOTION_REC_ICON, (gint)x, (gint)y, ICON_SIZE_W(VWND_REC_ICON), ICON_SIZE_H(VWND_REC_ICON), NFALIGN_LEFT, 0);
            break;
        case REC_ICON_PANIC:
            nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_PANIC_REC_ICON, (gint)x, (gint)y, ICON_SIZE_W(VWND_REC_ICON), ICON_SIZE_H(VWND_REC_ICON), NFALIGN_LEFT, 0);
            break;
    }

    return TRUE;
}

static void _set_postion_speaker_icon(VWND_T *p, VSM_ID_E wpos)
{
    gint x = 0, y = 0, w = 0, h = 0;
    VSM_DIV_E dtype = p->dtype;

    vwnd_get_position(dtype, wpos, &x, &y);
    vwnd_get_size(dtype, wpos, &w, &h);

    x += (w - ICON_SIZE_W(VWND_SEQ_ICON)/2 - ICON_SIZE_W(VWND_REC_ICON));
    x -= (ICON_SIZE_W(VWND_SPEAKER_ICON) + VWND_ICON_GAP_2);
    y += VWND_ICON_POS_Y;

    p->winfo[wpos].audioIcon_area.x = x;
    p->winfo[wpos].audioIcon_area.y = y;
    p->winfo[wpos].audioIcon_area.width = ICON_SIZE_W(VWND_SPEAKER_ICON);
    p->winfo[wpos].audioIcon_area.height = ICON_SIZE_H(VWND_SPEAKER_ICON);

#ifdef DBG_DRAW_AREA
    vwnd_paint_area(x, y, ICON_SIZE_W(VWND_SPEAKER_ICON), ICON_SIZE_H(VWND_SPEAKER_ICON), COLOR_PRG_IDX(UX_COLOR_FF0000));
#endif
}

static gboolean vwnd_draw_speaker_icon(VWND_T *p, VSM_ID_E wpos)
{
    gint x = 0, y = 0, w = 0, h = 0;
    VSM_DIV_E dtype = p->dtype;

    if (p->osd_off) return FALSE;
    if (!p->winfo[wpos].audio) return FALSE;
    if (_check_redraw_rect(&p->winfo[wpos].audioIcon_area)) return FALSE;

    DMSG(1, "__VWND_DRAW_SPEAKER_ICON");

    vwnd_get_position(dtype, wpos, &x, &y);
    vwnd_get_size(dtype, wpos, &w, &h);

    x += (w - ICON_SIZE_W(VWND_SEQ_ICON)/2 - ICON_SIZE_W(VWND_REC_ICON));
    x -= (ICON_SIZE_W(VWND_SPEAKER_ICON) + VWND_ICON_GAP_2);
    y += VWND_ICON_POS_Y;

    nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_SPEAKER_ICON, x, y, ICON_SIZE_W(VWND_SPEAKER_ICON), ICON_SIZE_H(VWND_SPEAKER_ICON), NFALIGN_LEFT, 0);

    return TRUE;
}

static void _set_postion_mic_icon(VWND_T *p, VSM_ID_E wpos)
{
    gint x = 0, y = 0, w = 0, h = 0;
    VSM_DIV_E dtype = p->dtype;

    vwnd_get_position(dtype, wpos, &x, &y);
    vwnd_get_size(dtype, wpos, &w, &h);

    x += (w - ICON_SIZE_W(VWND_SEQ_ICON)/2 - ICON_SIZE_W(VWND_REC_ICON) - VWND_ICON_GAP_2 - ICON_SIZE_W(VWND_SPEAKER_ICON));
    x -= (ICON_SIZE_W(VWND_MIC_ICON) + VWND_ICON_GAP_2);
    y += VWND_ICON_POS_Y;

    p->winfo[wpos].micIcon_area.x = x;
    p->winfo[wpos].micIcon_area.y = y;
    p->winfo[wpos].micIcon_area.width = ICON_SIZE_W(VWND_MIC_ICON);
    p->winfo[wpos].micIcon_area.height = ICON_SIZE_H(VWND_MIC_ICON);

#ifdef DBG_DRAW_AREA
    vwnd_paint_area(x, y, ICON_SIZE_W(VWND_MIC_ICON), ICON_SIZE_H(VWND_MIC_ICON), COLOR_PRG_IDX(UX_COLOR_FF0000));
#endif
}

static gboolean vwnd_draw_mic_icon(VWND_T *p, VSM_ID_E wpos)
{
    gint x = 0, y = 0, w = 0, h = 0;
    VSM_DIV_E dtype = p->dtype;
    gboolean audio_on = p->winfo[wpos].audio;

    if (p->osd_off) return FALSE;
    if (!p->winfo[wpos].mic) return FALSE;

    if (audio_on)
    {
        if (_check_redraw_rect(&p->winfo[wpos].micIcon_area)) return FALSE;
    }
    else
    {
        if (_check_redraw_rect(&p->winfo[wpos].audioIcon_area)) return FALSE;
    }

    DMSG(1, "__VWND_DRAW_MIC_ICON");

    vwnd_get_position(dtype, wpos, &x, &y);
    vwnd_get_size(dtype, wpos, &w, &h);

    x += (w - ICON_SIZE_W(VWND_SEQ_ICON)/2 - ICON_SIZE_W(VWND_REC_ICON));
    if(audio_on) x -= (VWND_ICON_GAP_2 + ICON_SIZE_W(VWND_SPEAKER_ICON));
    x -= (ICON_SIZE_W(VWND_MIC_ICON) + VWND_ICON_GAP_2);
    y += VWND_ICON_POS_Y;

    nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_MIC_ICON, x, y, ICON_SIZE_W(VWND_MIC_ICON), ICON_SIZE_H(VWND_MIC_ICON), NFALIGN_LEFT, 0);

    return TRUE;
}

static void _set_postion_motion_icon(VWND_T *p, VSM_ID_E wpos)
{
    gint x = 0, y = 0, w = 0, h = 0;
    VSM_DIV_E dtype = p->dtype;

    vwnd_get_position(dtype, wpos, &x, &y);
    vwnd_get_size(dtype, wpos, &w, &h);

    x += (w - ICON_SIZE_W(VWND_SEQ_ICON)/2 - ICON_SIZE_W(VWND_MOT_DETECT_ICON));
    y += (VWND_ICON_POS_Y + ICON_SIZE_H(VWND_REC_ICON) + VWND_ICON_GAP_2);

    p->winfo[wpos].motionIcon_area.x = x;
    p->winfo[wpos].motionIcon_area.y = y;
    p->winfo[wpos].motionIcon_area.width = ICON_SIZE_W(VWND_MOT_DETECT_ICON);
    p->winfo[wpos].motionIcon_area.height = ICON_SIZE_H(VWND_MOT_DETECT_ICON);

#ifdef DBG_DRAW_AREA
    vwnd_paint_area(x, y, ICON_SIZE_W(VWND_MOT_DETECT_ICON), ICON_SIZE_H(VWND_MOT_DETECT_ICON), COLOR_PRG_IDX(UX_COLOR_FF0000));
#endif
}

static gboolean vwnd_draw_motion_icon(VWND_T *p, VSM_ID_E wpos)
{
    gint x = 0, y = 0, w = 0, h = 0;
    VSM_DIV_E dtype = p->dtype;

    if (p->osd_off) return FALSE;
    if (!p->winfo[wpos].motion) return FALSE;
    if (_check_redraw_rect(&p->winfo[wpos].motionIcon_area)) return FALSE;

    DMSG(1, "__VWND_DRAW_MOTION_ICON");

    vwnd_get_position(dtype, wpos, &x, &y);
    vwnd_get_size(dtype, wpos, &w, &h);

    x += (w - ICON_SIZE_W(VWND_SEQ_ICON)/2 - ICON_SIZE_W(VWND_MOT_DETECT_ICON));
    y += (VWND_ICON_POS_Y + ICON_SIZE_H(VWND_REC_ICON) + VWND_ICON_GAP_2);

    nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_MOTION_DETECT_ICON, x, y, ICON_SIZE_W(VWND_MOT_DETECT_ICON), ICON_SIZE_H(VWND_MOT_DETECT_ICON), NFALIGN_LEFT, 0);

    return TRUE;
}

static void _set_postion_freeze_icon(VWND_T *p, VSM_ID_E wpos)
{
    gint x = 0, y = 0, w = 0, h = 0;
    VSM_DIV_E dtype = p->dtype;

    vwnd_get_position(dtype, wpos, &x, &y);
    vwnd_get_size(dtype, wpos, &w, &h);

    x += (w - ICON_SIZE_W(VWND_SEQ_ICON)/2 - ICON_SIZE_W(VWND_FREEZE_ICON));
    y += (VWND_ICON_POS_Y + ICON_SIZE_H(VWND_REC_ICON) + VWND_ICON_GAP_2);
    y += (ICON_SIZE_H(VWND_MOT_DETECT_ICON) + VWND_ICON_GAP_2);

    p->winfo[wpos].freezeIcon_area.x = x;
    p->winfo[wpos].freezeIcon_area.y = y;
    p->winfo[wpos].freezeIcon_area.width = ICON_SIZE_W(VWND_FREEZE_ICON);
    p->winfo[wpos].freezeIcon_area.height = ICON_SIZE_H(VWND_FREEZE_ICON);

#ifdef DBG_DRAW_AREA
    vwnd_paint_area(x, y, ICON_SIZE_W(VWND_FREEZE_ICON), ICON_SIZE_H(VWND_FREEZE_ICON), COLOR_PRG_IDX(UX_COLOR_FF0000));
#endif
}

static gboolean vwnd_draw_freeze_icon(VWND_T *p, VSM_ID_E wpos)
{
    gint x = 0, y = 0, w = 0, h = 0;
    VSM_DIV_E dtype = p->dtype;

    if (p->osd_off) return FALSE;
    if (!p->winfo[wpos].freeze) return FALSE;
    if (_check_redraw_rect(&p->winfo[wpos].freezeIcon_area)) return FALSE;

    DMSG(1, "__VWND_DRAW_FREEZE_ICON");

    vwnd_get_position(dtype, wpos, &x, &y);
    vwnd_get_size(dtype, wpos, &w, &h);

    x += (w - ICON_SIZE_W(VWND_SEQ_ICON)/2 - ICON_SIZE_W(VWND_FREEZE_ICON));
    y += (VWND_ICON_POS_Y + ICON_SIZE_H(VWND_REC_ICON) + VWND_ICON_GAP_2);
    y += (ICON_SIZE_H(VWND_MOT_DETECT_ICON) + VWND_ICON_GAP_2);

    nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_FREEZE_REC_ICON, x, y, ICON_SIZE_W(VWND_FREEZE_ICON), ICON_SIZE_H(VWND_FREEZE_ICON), NFALIGN_LEFT, 0);

    return TRUE;
}

static void _set_postion_pnd_rate(VWND_T *p, VSM_ID_E wpos)
{
    GdkPixbuf* prog_img;
    guint bg_width, bg_height;
    gint win_x, win_y, win_w, win_h;
    VSM_DIV_E dtype = p->dtype;
    gint x = 0, y = 0;

    prog_img = nfui_get_image_from_file((IMG_CONNECTING_PROGRESS_16DIV_BG), NULL);

    nfui_get_pixbuf_size(prog_img, &bg_width, &bg_height);
    vwnd_get_position(dtype, wpos, &win_x, &win_y);
    vwnd_get_size(dtype, wpos, &win_w, &win_h);

    x = win_x+(win_w-bg_width)/2;
    y = win_y+(win_h-bg_height)/2+VWND_PND_RATE_GAP_Y;

    p->winfo[wpos].pndPrg_area.x = x;
    p->winfo[wpos].pndPrg_area.y = y;
    p->winfo[wpos].pndPrg_area.width = bg_width;
    p->winfo[wpos].pndPrg_area.height = bg_height;

#ifdef DBG_DRAW_AREA
    vwnd_paint_area(x, y, bg_width, bg_height, COLOR_PRG_IDX(UX_COLOR_FF0000));
#endif
}

static gboolean vwnd_draw_pnd_rate(VWND_T *p, VSM_ID_E wpos)
{
    GdkPixbuf* rate_pixbuf;
    GdkPixbuf* prog_img[4];
    guint bg_width, head_width, body_width, tail_width;
    guint bg_height, body_height;
    guint img_num, fill_cnt;
    guint i, rate_x;
    gint win_x, win_y, win_w, win_h;
    gint rate = p->winfo[wpos].pnd_rate;
    VSM_DIV_E dtype = p->dtype;
    gint x = 0, y = 0;

    if (!rate) return FALSE;
    if (_check_redraw_rect(&p->winfo[wpos].pndPrg_area)) return FALSE;

    prog_img[0] = nfui_get_image_from_file((IMG_CONNECTING_PROGRESS_16DIV_BG), NULL);
    prog_img[1] = nfui_get_image_from_file((IMG_CONNECTING_PROGRESS_HEAD), NULL);
    prog_img[2] = nfui_get_image_from_file((IMG_CONNECTING_PROGRESS_MIDDLE), NULL);
    prog_img[3] = nfui_get_image_from_file((IMG_CONNECTING_PROGRESS_TAIL), NULL);

    nfui_get_pixbuf_size(prog_img[0], &bg_width, &bg_height);
    nfui_get_pixbuf_size(prog_img[1], &head_width, &body_height);
    nfui_get_pixbuf_size(prog_img[2], &body_width, &body_height);
    nfui_get_pixbuf_size(prog_img[3], &tail_width, &body_height);

    vwnd_get_position(dtype, wpos, &win_x, &win_y);
    vwnd_get_size(dtype, wpos, &win_w, &win_h);

    fill_cnt = (bg_width-head_width-tail_width)*rate/100;

    if (fill_cnt == 0) return FALSE;

    DMSG(1, "__VWND_DRAW_PND_RATE");

    x = win_x+(win_w-bg_width)/2;
    y = win_y+(win_h-bg_height)/2+VWND_PND_RATE_GAP_Y;

    rate_pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, bg_width, bg_height);
    gdk_pixbuf_fill (rate_pixbuf, 0x00000000);

// bg img draw
    gdk_pixbuf_composite(prog_img[0], rate_pixbuf, 0, 0, bg_width, bg_height, 0, 0, 1, 1, GDK_INTERP_BILINEAR, 255);

// head img draw
    gdk_pixbuf_composite(prog_img[1], rate_pixbuf, 0, 0, head_width, body_height, 0, 0, 1, 1, GDK_INTERP_BILINEAR, 255);
    rate_x = head_width;

// body img draw
    gdk_pixbuf_composite(prog_img[2], rate_pixbuf, rate_x, 0, fill_cnt, body_height, rate_x, 0, 1, 1, GDK_INTERP_BILINEAR, 255);
    rate_x += fill_cnt;

// tail img draw
    gdk_pixbuf_composite(prog_img[3], rate_pixbuf, rate_x, 0, tail_width, body_height, rate_x, 0, 1, 1, GDK_INTERP_BILINEAR, 255);

    nfutil_draw_pixbuf(g_vw_drawable, g_vw_gc, rate_pixbuf, x, y, bg_width, bg_height, NFALIGN_LEFT, 0);

    g_object_unref(rate_pixbuf);

    return TRUE;
}

static void _set_postion_pnd_wait(VWND_T *p, VSM_ID_E wpos)
{
    GdkPixbuf* gear_img;
    guint img_width, img_height;
    gint win_x, win_y, win_w, win_h;
    VSM_DIV_E dtype = p->dtype;
    gint x = 0, y = 0;

    gear_img = nfui_get_image_from_file((IMG_GEAR_01), NULL);

    nfui_get_pixbuf_size(gear_img, &img_width, &img_height);
    vwnd_get_position(dtype, wpos, &win_x, &win_y);
    vwnd_get_size(dtype, wpos, &win_w, &win_h);

    x = win_x+(win_w-img_width)/2;
    y = win_y+(win_h-img_height)/2;

    p->winfo[wpos].pndPrg_area.x = x;
    p->winfo[wpos].pndPrg_area.y = y;
    p->winfo[wpos].pndPrg_area.width = img_width;
    p->winfo[wpos].pndPrg_area.height = img_height;

#ifdef DBG_DRAW_AREA
    vwnd_paint_area(x, y, img_width, img_height, COLOR_PRG_IDX(UX_COLOR_FF0000));
#endif
}

static gboolean vwnd_draw_pnd_wait(VWND_T *p, VSM_ID_E wpos)
{
    GdkPixbuf* prog_img;
    gint win_x, win_y, win_w, win_h;
    gint rate = p->winfo[wpos].pnd_rate;
    VSM_DIV_E dtype = p->dtype;
    gint x = 0, y = 0;
    guint g_w, g_h;

    if (!rate) return FALSE;
    if (_check_redraw_rect(&p->winfo[wpos].pndPrg_area)) return FALSE;

    rate %= 6;

    switch(rate) {
        case 0: prog_img = nfui_get_image_from_file((IMG_GEAR_01), NULL); break;
        case 1: prog_img = nfui_get_image_from_file((IMG_GEAR_02), NULL); break;
        case 2: prog_img = nfui_get_image_from_file((IMG_GEAR_03), NULL); break;
        case 3: prog_img = nfui_get_image_from_file((IMG_GEAR_04), NULL); break;
        case 4: prog_img = nfui_get_image_from_file((IMG_GEAR_05), NULL); break;
        case 5: prog_img = nfui_get_image_from_file((IMG_GEAR_06), NULL); break;
    }

    nfui_get_pixbuf_size(prog_img, &g_w, &g_h);

    vwnd_get_position(dtype, wpos, &win_x, &win_y);
    vwnd_get_size(dtype, wpos, &win_w, &win_h);

    x = win_x+(win_w-g_w)/2;
    y = win_y+(win_h-g_h)/2;

    nfutil_draw_pixbuf(g_vw_drawable, g_vw_gc, prog_img, x, y, g_w, g_h, NFALIGN_LEFT, 0);

    return TRUE;
}

static void _set_postion_connecting(VWND_T *p, VSM_ID_E wpos)
{
    GdkPixbuf* gear_img;
    guint img_width, img_height;
    gint win_x, win_y, win_w, win_h;
    VSM_DIV_E dtype = p->dtype;
    gint x = 0, y = 0;
    guint string_w = 0;
    gchar strBuf[VWND_TEXT_LEN+1];

    memset(strBuf, 0x00, VWND_TEXT_LEN+1);
    g_sprintf(strBuf, "CONNECTING...");


    gear_img = nfui_get_image_from_file((IMG_GEAR_01), NULL);

    nfui_get_pixbuf_size(gear_img, &img_width, &img_height);
    vwnd_get_position(dtype, wpos, &win_x, &win_y);
    vwnd_get_size(dtype, wpos, &win_w, &win_h);
    string_w = nfutil_string_width(1, g_vw_drawable, nffont_get_pango_font(NFFONT_LARGE_SEMI), strBuf, NORMAL_SPACING);

    x = ((win_w - string_w)/2) + win_x;
    y = win_y+(win_h -img_height) / 2 + img_height + VWND_CONNECTING_GAP_Y;

    p->winfo[wpos].connecting_area.x = x;
    p->winfo[wpos].connecting_area.y = y;
    p->winfo[wpos].connecting_area.width = VWND_CONNECTING_WIDTH;
    p->winfo[wpos].connecting_area.height = FONT_H;

#ifdef DBG_DRAW_AREA
    vwnd_paint_area(x, y, VWND_CONNECTING_WIDTH, FONT_H, COLOR_PRG_IDX(UX_COLOR_FF0000));
#endif
}

static gboolean vwnd_draw_connecting_text(VWND_T *p, VSM_ID_E wpos)
{
    gchar strBuf[VWND_TEXT_LEN+1];
    VSM_DIV_E dtype = p->dtype;
    guint img_width, img_height;
    gint win_x, win_y, win_w, win_h;
    gint x = 0, y = 0;
    GdkPixbuf* gear_img;
    guint string_w = 0;

    if ( !p->winfo[wpos].is_connecting ) return FALSE;
    if (_check_redraw_rect(&p->winfo[wpos].connecting_area)) return FALSE;

    vwnd_get_position(dtype, wpos, &win_x, &win_y);
    vwnd_get_size(dtype, wpos, &win_w, &win_h);

    memset(strBuf, 0x00, VWND_TEXT_LEN+1);
    g_sprintf(strBuf, "CONNECTING...");

    gear_img = nfui_get_image_from_file((IMG_GEAR_01), NULL);

    nfui_get_pixbuf_size(gear_img, &img_width, &img_height);
    string_w = nfutil_string_width(1, g_vw_drawable, nffont_get_pango_font(NFFONT_LARGE_SEMI), strBuf, NORMAL_SPACING);


    x = ((win_w - string_w)/2) + win_x;
    y = win_y+(win_h -img_height) / 2 + img_height + VWND_CONNECTING_GAP_Y;

    vwnd_draw_text(strBuf, x, y, COLOR_IDX(308), COLOR_IDX(307), NFFONT_LARGE_SEMI, NORMAL_SPACING, TRUE);

    return TRUE;
}

static gboolean post_login_fail_btn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE) 
    {
        gint wid = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "ch-num"));
        gint ch = vsm_get_current_ch(wid);
        gint x, y, w, h, pos_x, pos_y;
        gint over_w, over_h;

        vwnd_get_position(pvwnd->dtype, wid, &x, &y);
        vwnd_get_size(pvwnd->dtype, wid, &w, &h);

        DMSG(9, "win_x : %d, win_y : %d, win_w : %d, win_h : %d", x, y, w, h);
        DMSG(9, "org_x : %d, org_y : %d", (x + ((w - CAM_PASSWD_SIZE_WID)/2)), (y + ((h - CAM_PASSWD_SIZE_HEI)/2)));
        DMSG(9, "disp_h : %d, disp_w : %d", DISPLAY_ACTIVE_HEIGHT, DISPLAY_ACTIVE_WIDTH);
        //pos_x
        if ((x + ((w - CAM_PASSWD_SIZE_WID)/2)) <= 0) {
            DMSG(9, "pos_x set to 1");
            pos_x = 1;
        }
        else if (((x + (w - CAM_PASSWD_SIZE_WID)/2) + CAM_PASSWD_SIZE_WID) >= DISPLAY_ACTIVE_WIDTH) {
            over_w = abs((w - CAM_PASSWD_SIZE_WID));
            DMSG(9, "over_w : %d", over_w);
            pos_x = x - over_w;
        }
        else {
            pos_x = x + (w - CAM_PASSWD_SIZE_WID)/2;
        }

        //pos_y
        if ((y + (h - CAM_PASSWD_SIZE_HEI)/2) <= 0) {
            DMSG(9, "pos_y set to 1");
            pos_y = 1;
        }
        else if ((y + ((h - CAM_PASSWD_SIZE_HEI)/2)) + CAM_PASSWD_SIZE_HEI > DISPLAY_ACTIVE_HEIGHT) {
            over_h = abs((h - CAM_PASSWD_SIZE_HEI));
            DMSG(9, "over_h : %d", over_h);
            pos_y = y - over_h;
        }
        else {
            pos_y = y + (h - CAM_PASSWD_SIZE_HEI)/2;
        }
        
        DMSG(9, "last_x : %d, last_y : %d", pos_x, pos_y);
        VW_CamPwd_Open((NFWINDOW*)obj, pos_x, pos_y, ch);
    }
    return FALSE;
}

static void _set_postion_pnd_login_fail(VWND_T *p, VSM_ID_E wpos)
{
    gint win_x, win_y, win_w, win_h;
    VSM_DIV_E dtype = p->dtype;
    gint x = 0, y = 0;

    vwnd_get_position(dtype, wpos, &win_x, &win_y);
    vwnd_get_size(dtype, wpos, &win_w, &win_h);

    x = win_x + (win_w-VWND_LOGINFAIL_WIDTH)/2;
    y = win_y + (win_h-FONT_H)/2 + FONT_H;

    p->winfo[wpos].loginFail_area.x = x;
    p->winfo[wpos].loginFail_area.y = y;
    p->winfo[wpos].loginFail_area.width = VWND_LOGINFAIL_WIDTH;
    p->winfo[wpos].loginFail_area.height = VWND_LOGINFAIL_HEIGHT;

#ifdef DBG_DRAW_AREA
    vwnd_paint_area(x, y, VWND_LOGINFAIL_WIDTH, VWND_LOGINFAIL_HEIGHT, COLOR_PRG_IDX(UX_COLOR_FF0000));
#endif
}

static gboolean vwnd_draw_login_fail(VWND_T *p, VSM_ID_E wpos)
{
    gint win_x, win_y, win_w, win_h;
    gint x, y;

    VSM_DIV_E dtype = p->dtype;

    if (_check_redraw_rect(&p->winfo[wpos].loginFail_area)) return FALSE;

    if ( !p->winfo[wpos].login_fail) return FALSE;

    vwnd_get_position(dtype, wpos, &win_x, &win_y);
    vwnd_get_size(dtype, wpos, &win_w, &win_h);

    x = win_x + (win_w-VWND_LOGINFAIL_WIDTH)/2;
    y = win_y + (win_h-FONT_H)/2 + FONT_H;

    nfui_nffixed_put((NFFIXED*)g_vw_fixed, g_chk_btn[wpos], x, y);
    nfui_nfobject_show(g_chk_btn[wpos]);

    return TRUE;
}

static void _set_postion_seq_icon(VWND_T *p)
{
    guint string_w = 0, string_h = 0;
    guint x, y;

    x = ((pvwnd->plt_w - ICON_SIZE_W(VWND_SEQ_ICON)) / 2);
    y = VWND_SEQ_POS_Y;
    string_w = ICON_SIZE_W(VWND_SEQ_ICON);
    string_h = ICON_SIZE_H(VWND_SEQ_ICON);

    p->seqIcon_area.x = x;
    p->seqIcon_area.y = y;
    p->seqIcon_area.width = ICON_SIZE_W(VWND_SEQ_ICON);
    p->seqIcon_area.height = ICON_SIZE_H(VWND_SEQ_ICON);

#ifdef DBG_DRAW_AREA
    vwnd_paint_area(x, y, ICON_SIZE_W(VWND_SEQ_ICON), ICON_SIZE_H(VWND_SEQ_ICON), COLOR_PRG_IDX(UX_COLOR_FF0000));
#endif
}

static gboolean vwnd_draw_seq_icon(VWND_T *p)
{
    gint x = 0, y = 0;

    if (!p->seq_icon) return FALSE;
    if (_check_redraw_rect(&p->seqIcon_area)) return FALSE;

    DMSG(1, "__VWND_DRAW_SEQ_ICON");

    x = ((pvwnd->plt_w - ICON_SIZE_W(VWND_SEQ_ICON)) / 2);
    y = VWND_SEQ_POS_Y;

    nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_SEQUECE_ICON, x, y, ICON_SIZE_W(VWND_SEQ_ICON), ICON_SIZE_H(VWND_SEQ_ICON), NFALIGN_LEFT, 0);

    return TRUE;
}

static void _set_postion_user_text(VWND_T *p)
{
    guint string_w = 0, string_h = 0;
    guint x, y;

    x = 30;
    y = MODE_FULL_ACTIVE_H-70;
    string_w = VWND_USER_WIDTH;
    string_h = FONT_H;

    p->user_area.x = x;
    p->user_area.y = y;
    p->user_area.width = string_w;
    p->user_area.height = string_h;

#ifdef DBG_DRAW_AREA
    vwnd_paint_area(x, y, string_w, string_h, COLOR_PRG_IDX(UX_COLOR_FF0000));
#endif
}

static gboolean vwnd_draw_user_text(VWND_T *p)
{
    guint string_w = 0, string_h = 0;
    gint x = 0, y = 0;
    gchar *text = p->user;

    if (!strlen(text)) return FALSE;
    if (_check_redraw_rect(&p->user_area)) return FALSE;

    DMSG(1, "__VWND_DRAW_USER_TEXT");

    x = 30;
    y = MODE_FULL_ACTIVE_H-70;
    string_w = nfutil_string_width(0, g_vw_drawable, nffont_get_pango_font(NFFONT_LARGE_SEMI), text, NORMAL_SPACING);
    string_h = nfutil_string_height(g_vw_drawable, nffont_get_pango_font(NFFONT_LARGE_SEMI), text, NORMAL_SPACING);

    vwnd_draw_text(text, x, y, COLOR_IDX(308), COLOR_IDX(307), NFFONT_LARGE_SEMI, NORMAL_SPACING, FALSE);

    return TRUE;
}

static void _set_postion_time(VWND_T *p)
{
    guint string_w = 0, string_h = 0;
    guint x, y;

    string_w = VWND_TIME_WIDTH;
    string_h = FONT_H;

    x = (MODE_FULL_ACTIVE_W - string_w)/2;
    y = MODE_FULL_ACTIVE_H - 70;

    p->time_area.x = x;
    p->time_area.y = y;
    p->time_area.width = string_w;
    p->time_area.height = string_h;

#ifdef DBG_DRAW_AREA
    vwnd_paint_area(x, y, string_w, string_h, COLOR_PRG_IDX(UX_COLOR_FF0000));
#endif
}

static gboolean vwnd_draw_time(VWND_T *p)
{
    guint x, y;
    guint string_w = 0, string_h = 0;
    gchar *text = p->time_text;

    if (!strlen(text)) return FALSE;
    if (_check_redraw_rect(&p->time_area)) return FALSE;

    DMSG(1, "__VWND_DRAW_TIME");

    string_w = nfutil_string_width(0, g_vw_drawable, nffont_get_pango_font(NFFONT_XLARGE_SEMI), text, NORMAL_SPACING);
    string_h = nfutil_string_height(g_vw_drawable, nffont_get_pango_font(NFFONT_XLARGE_SEMI), text, NORMAL_SPACING);

    x = (MODE_FULL_ACTIVE_W - string_w)/2;
    y = MODE_FULL_ACTIVE_H - 70;

    vwnd_draw_text(text, x, y, COLOR_IDX(308), COLOR_IDX(307), NFFONT_XLARGE_SEMI, NORMAL_SPACING, TRUE);

    return TRUE;
}

static void _set_postion_play_status(VWND_T *p)
{
    guint x, y;

    x = 30;
    y = MODE_FULL_ACTIVE_H - 89;

    p->playST_area.x = x;
    p->playST_area.y = y;
    p->playST_area.width = ICON_SIZE_W(VWND_PSTATUS_ICON);
    p->playST_area.height = ICON_SIZE_H(VWND_PSTATUS_ICON);

#ifdef DBG_DRAW_AREA
    vwnd_paint_area(x, y, ICON_SIZE_W(VWND_PSTATUS_ICON), ICON_SIZE_H(VWND_PSTATUS_ICON), COLOR_PRG_IDX(UX_COLOR_FF0000));
#endif
}

static gboolean vwnd_draw_play_status(VWND_T *p)
{
    guint x, y;
    DIR_RATE_E status = p->play_st;

    if (!status) return FALSE;
    if (_check_redraw_rect(&p->playST_area)) return FALSE;

    DMSG(1, "__VWND_DRAW_PLAY_STATUS");

    x = 30;
    y = MODE_FULL_ACTIVE_H - 89;

    if (status == DR_BWD_001)       nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_PLAY_STATUS_FULL_BWD_001, x, y, -1, -1, NFALIGN_LEFT, 0);
    else if (status == DR_BWD_002)  nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_PLAY_STATUS_FULL_BWD_002, x, y, -1, -1, NFALIGN_LEFT, 0);
    else if (status == DR_BWD_004)  nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_PLAY_STATUS_FULL_BWD_004, x, y, -1, -1, NFALIGN_LEFT, 0);
    else if (status == DR_BWD_008)  nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_PLAY_STATUS_FULL_BWD_008, x, y, -1, -1, NFALIGN_LEFT, 0);
    else if (status == DR_BWD_016)  nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_PLAY_STATUS_FULL_BWD_016, x, y, -1, -1, NFALIGN_LEFT, 0);
    else if (status == DR_BWD_032)  nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_PLAY_STATUS_FULL_BWD_032, x, y, -1, -1, NFALIGN_LEFT, 0);
    else if (status == DR_BWD_064)  nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_PLAY_STATUS_FULL_BWD_064, x, y, -1, -1, NFALIGN_LEFT, 0);
    else if (status == DR_BWD_128)  nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_PLAY_STATUS_FULL_BWD_128, x, y, -1, -1, NFALIGN_LEFT, 0);
    else if (status == DR_FWD_001)  nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_PLAY_STATUS_FULL_FWD_001, x, y, -1, -1, NFALIGN_LEFT, 0);
    else if (status == DR_FWD_002)  nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_PLAY_STATUS_FULL_FWD_002, x, y, -1, -1, NFALIGN_LEFT, 0);
    else if (status == DR_FWD_004)  nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_PLAY_STATUS_FULL_FWD_004, x, y, -1, -1, NFALIGN_LEFT, 0);
    else if (status == DR_FWD_008)  nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_PLAY_STATUS_FULL_FWD_008, x, y, -1, -1, NFALIGN_LEFT, 0);
    else if (status == DR_FWD_016)  nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_PLAY_STATUS_FULL_FWD_016, x, y, -1, -1, NFALIGN_LEFT, 0);
    else if (status == DR_FWD_032)  nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_PLAY_STATUS_FULL_FWD_032, x, y, -1, -1, NFALIGN_LEFT, 0);
    else if (status == DR_FWD_064)  nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_PLAY_STATUS_FULL_FWD_064, x, y, -1, -1, NFALIGN_LEFT, 0);
    else if (status == DR_FWD_128)  nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_PLAY_STATUS_FULL_FWD_128, x, y, -1, -1, NFALIGN_LEFT, 0);
    else if (status == DR_BWD_SLOW_002)     nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_PLAY_STATUS_FULL_SLOW_BWD_002, x, y, -1, -1, NFALIGN_LEFT, 0);
    else if (status == DR_BWD_SLOW_004)     nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_PLAY_STATUS_FULL_SLOW_BWD_004, x, y, -1, -1, NFALIGN_LEFT, 0);
    else if (status == DR_BWD_SLOW_008)     nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_PLAY_STATUS_FULL_SLOW_BWD_008, x, y, -1, -1, NFALIGN_LEFT, 0);
    else if (status == DR_BWD_SLOW_016)     nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_PLAY_STATUS_FULL_SLOW_BWD_016, x, y, -1, -1, NFALIGN_LEFT, 0);
    else if (status == DR_BWD_SLOW_032)     nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_PLAY_STATUS_FULL_SLOW_BWD_032, x, y, -1, -1, NFALIGN_LEFT, 0);
    else if (status == DR_BWD_SLOW_064)     nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_PLAY_STATUS_FULL_SLOW_BWD_064, x, y, -1, -1, NFALIGN_LEFT, 0);
    else if (status == DR_BWD_SLOW_128)     nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_PLAY_STATUS_FULL_SLOW_BWD_128, x, y, -1, -1, NFALIGN_LEFT, 0);
    else if (status == DR_FWD_SLOW_002)     nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_PLAY_STATUS_FULL_SLOW_FWD_002, x, y, -1, -1, NFALIGN_LEFT, 0);
    else if (status == DR_FWD_SLOW_004)     nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_PLAY_STATUS_FULL_SLOW_FWD_004, x, y, -1, -1, NFALIGN_LEFT, 0);
    else if (status == DR_FWD_SLOW_008)     nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_PLAY_STATUS_FULL_SLOW_FWD_008, x, y, -1, -1, NFALIGN_LEFT, 0);
    else if (status == DR_FWD_SLOW_016)     nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_PLAY_STATUS_FULL_SLOW_FWD_016, x, y, -1, -1, NFALIGN_LEFT, 0);
    else if (status == DR_FWD_SLOW_032)     nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_PLAY_STATUS_FULL_SLOW_FWD_032, x, y, -1, -1, NFALIGN_LEFT, 0);
    else if (status == DR_FWD_SLOW_064)     nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_PLAY_STATUS_FULL_SLOW_FWD_064, x, y, -1, -1, NFALIGN_LEFT, 0);
    else if (status == DR_FWD_SLOW_128)     nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_PLAY_STATUS_FULL_SLOW_FWD_128, x, y, -1, -1, NFALIGN_LEFT, 0);
    else if (status == DR_BWD_NEXT_FRAME)   nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_PLAY_STATUS_FULL_NEXT_BWD, x, y, -1, -1, NFALIGN_LEFT, 0);
    else if (status == DR_FWD_NEXT_FRAME)   nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_PLAY_STATUS_FULL_NEXT_FWD, x, y, -1, -1, NFALIGN_LEFT, 0);
    else if (status == DR_PAUSE)    nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_PLAY_STATUS_FULL_PAUSE, x, y, -1, -1, NFALIGN_LEFT, 0);
    else if (status == DR_STOP)     nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_PLAY_STATUS_FULL_STOP, x, y, -1, -1, NFALIGN_LEFT, 0);
    else g_message("%s, %d, %08X not yet", __FUNCTION__, __LINE__, status);

    return TRUE;
}

static void _set_postion_alarm_status(VWND_T *p)
{
    guint x, y;

    x = MODE_FULL_ACTIVE_W- 421;
    y = MODE_FULL_ACTIVE_H - 89;

    p->alarmST_area.x = x;
    p->alarmST_area.y = y;
    p->alarmST_area.width = ICON_SIZE_W(VWND_ALARM_ICON);
    p->alarmST_area.height = ICON_SIZE_H(VWND_ALARM_ICON);

#ifdef DBG_DRAW_AREA
    vwnd_paint_area(x, y, ICON_SIZE_W(VWND_ALARM_ICON), ICON_SIZE_H(VWND_ALARM_ICON), COLOR_PRG_IDX(UX_COLOR_FF0000));
#endif
}

static gboolean vwnd_draw_alarm_status(VWND_T *p)
{
    guint x, y;
    ALARM_ST status = p->alarm_st;

    if (!status) return FALSE;
    if (_check_redraw_rect(&p->alarmST_area)) return FALSE;

    DMSG(1, "__VWND_DRAW_ALARM_STATUS");

    x = MODE_FULL_ACTIVE_W- 421;
    y = MODE_FULL_ACTIVE_H - 89;

    if (status == AST_OFF)
        nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_F_ALARM_OFF, x, y, -1, -1, NFALIGN_LEFT, 0);
    else        //ALARM_ST_ON
        nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_F_ALARM_ON_1, x, y, -1, -1, NFALIGN_LEFT, 0);

    return TRUE;
}

static void _set_postion_network_status(VWND_T *p)
{
    guint x, y;

    x = MODE_FULL_ACTIVE_W- 350;
    y = MODE_FULL_ACTIVE_H - 89;

    p->netST_area.x = x;
    p->netST_area.y = y;
    p->netST_area.width = ICON_SIZE_W(VWND_NET_ICON);
    p->netST_area.height = ICON_SIZE_H(VWND_NET_ICON);

#ifdef DBG_DRAW_AREA
    vwnd_paint_area(x, y, ICON_SIZE_W(VWND_NET_ICON), ICON_SIZE_H(VWND_NET_ICON), COLOR_PRG_IDX(UX_COLOR_FF0000));
#endif
}

static gboolean vwnd_draw_network_status(VWND_T *p)
{
    guint x, y;
    gchar strBuf[10];
    NET_ST status = p->net_st;
    gint cnt = p->net_cnt;
    gint str_pos_x;

    if (!status) return FALSE;
    if (_check_redraw_rect(&p->netST_area)) return FALSE;

    DMSG(1, "__VWND_DRAW_NETWORK_STATUS");

    memset(strBuf, 0x00, sizeof(strBuf));

    x = MODE_FULL_ACTIVE_W- 350;
    y = MODE_FULL_ACTIVE_H - 89;

    if (status == NST_ERROR)
    {
        nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_F_NET_ERROR, x, y, -1, -1, NFALIGN_LEFT, 0);
    }
    else if(status == NST_CONFLICT)
    {
        nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_F_NET_IPCONFLICT, x, y, -1, -1, NFALIGN_LEFT, 0);
    }
    else    // NET_ST_NORMAL
    {
        nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_F_NET_NORMAL, x, y, -1, -1, NFALIGN_LEFT, 0);

        if (cnt)
        {
            if (cnt >= 10)  str_pos_x = MODE_FULL_ACTIVE_W - 305;
            else            str_pos_x = MODE_FULL_ACTIVE_W - 300;

            g_sprintf(strBuf, "%d", cnt);
            vwnd_draw_text(strBuf, str_pos_x, (MODE_FULL_ACTIVE_H - 60), COLOR_IDX(308), COLOR_IDX(307), NFFONT_SMALL_SEMI, NORMAL_SPACING, TRUE);
        }
    }

    return TRUE;
}

static void _set_postion_disk_status(VWND_T *p)
{
    guint x, y;

    x = MODE_FULL_ACTIVE_W- 257;
    y = MODE_FULL_ACTIVE_H - 89;

    p->diskST_area.x = x;
    p->diskST_area.y = y;
    p->diskST_area.width = ICON_SIZE_W(VWND_DISK_ICON);
    p->diskST_area.height = ICON_SIZE_H(VWND_DISK_ICON);

#ifdef DBG_DRAW_AREA
    vwnd_paint_area(x, y, ICON_SIZE_W(VWND_DISK_ICON), ICON_SIZE_H(VWND_DISK_ICON), COLOR_PRG_IDX(UX_COLOR_FF0000));
#endif
}

static gboolean vwnd_draw_disk_status(VWND_T *p)
{
    guint x, y;
    gint i = 0;
    gchar strBuf[10];
    DISK_ST status = p->disk_st;
    RAID_ST r_status = p->raid_st;
    gint usage = p->disk_usage;

    if (!status) return FALSE;
    if (_check_redraw_rect(&p->diskST_area)) return FALSE;

    DMSG(1, "__VWND_DRAW_DISK_STATUS");

    memset(strBuf, 0x00, sizeof(strBuf));

    x = MODE_FULL_ACTIVE_W- 257;
    y = MODE_FULL_ACTIVE_H - 89;

    if(r_status == RST_DEGRADE || r_status == RST_BROKEN)
    {
        nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_F_DISK_RAID_WARNING, x, y, -1, -1, NFALIGN_LEFT, 0);

        if (status & (1 << DST_OW))
            vwnd_draw_text("OW", (MODE_FULL_ACTIVE_W - 200), (MODE_FULL_ACTIVE_H - 61), COLOR_IDX(308), COLOR_IDX(307), NFFONT_SMALL_SEMI, NORMAL_SPACING, TRUE);
        else if (status & (1 << DST_FULL))
        {
            for(i=0; i<(usage%2 ? (usage+1)/2 : usage/2); i++)
                nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_DISK_BAR_FULL, (MODE_FULL_ACTIVE_W - 210 + i), (MODE_FULL_ACTIVE_H - 65), 1, 26, NFALIGN_LEFT, 0);

            vwnd_draw_text("100%", (MODE_FULL_ACTIVE_W - 210), (MODE_FULL_ACTIVE_H - 61), COLOR_IDX(308), COLOR_IDX(307), NFFONT_SMALL_SEMI, NORMAL_SPACING, TRUE);
        }
        else
        {
            for(i=0; i<(usage%2 ? (usage+1)/2 : usage/2); i++)
                nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_DISK_BAR_FULL, (MODE_FULL_ACTIVE_W - 210 + i), (MODE_FULL_ACTIVE_H - 65), 1, 26, NFALIGN_LEFT, 0);

            g_sprintf(strBuf, "%d%%", usage);
            vwnd_draw_text(strBuf, (MODE_FULL_ACTIVE_W - 200), (MODE_FULL_ACTIVE_H - 61), COLOR_IDX(308), COLOR_IDX(307), NFFONT_SMALL_SEMI, NORMAL_SPACING, TRUE);
        }
    }
    else if (status & (1 << DST_NEEDCHECK))
    {
        nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_F_DISK_NEEDCHECK, x, y, -1, -1, NFALIGN_LEFT, 0);

        if (status & (1 << DST_OW))
            vwnd_draw_text("OW", (MODE_FULL_ACTIVE_W - 200), (MODE_FULL_ACTIVE_H - 61), COLOR_IDX(308), COLOR_IDX(307), NFFONT_SMALL_SEMI, NORMAL_SPACING, TRUE);
        else if (status & (1 << DST_FULL))
        {
            for(i=0; i<(usage%2 ? (usage+1)/2 : usage/2); i++)
                nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_DISK_BAR_FULL, (MODE_FULL_ACTIVE_W - 210 + i), (MODE_FULL_ACTIVE_H - 65), 1, 26, NFALIGN_LEFT, 0);

            vwnd_draw_text("100%", (MODE_FULL_ACTIVE_W - 210), (MODE_FULL_ACTIVE_H - 61), COLOR_IDX(308), COLOR_IDX(307), NFFONT_SMALL_SEMI, NORMAL_SPACING, TRUE);
        }
        else
        {
            for(i=0; i<(usage%2 ? (usage+1)/2 : usage/2); i++)
                nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_DISK_BAR_FULL, (MODE_FULL_ACTIVE_W - 210 + i), (MODE_FULL_ACTIVE_H - 65), 1, 26, NFALIGN_LEFT, 0);

            g_sprintf(strBuf, "%d%%", usage);
            vwnd_draw_text(strBuf, (MODE_FULL_ACTIVE_W - 200), (MODE_FULL_ACTIVE_H - 61), COLOR_IDX(308), COLOR_IDX(307), NFFONT_SMALL_SEMI, NORMAL_SPACING, TRUE);
        }
    }
    else if (status & (1 << DST_SMT_ERR))
    {
        nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_F_DISK_NEEDCHECK, x, y, -1, -1, NFALIGN_LEFT, 0);

        if (status & (1 << DST_OW))
            vwnd_draw_text("OW", (MODE_FULL_ACTIVE_W - 200), (MODE_FULL_ACTIVE_H - 61), COLOR_IDX(308), COLOR_IDX(307), NFFONT_SMALL_SEMI, NORMAL_SPACING, TRUE);
        else if (status & (1 << DST_FULL))
        {
            for(i=0; i<(usage%2 ? (usage+1)/2 : usage/2); i++)
                nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_DISK_BAR_FULL, (MODE_FULL_ACTIVE_W - 210 + i), (MODE_FULL_ACTIVE_H - 65), 1, 26, NFALIGN_LEFT, 0);

            vwnd_draw_text("100%", (MODE_FULL_ACTIVE_W - 210), (MODE_FULL_ACTIVE_H - 61), COLOR_IDX(308), COLOR_IDX(307), NFFONT_SMALL_SEMI, NORMAL_SPACING, TRUE);
        }
        else
        {
            for(i=0; i<(usage%2 ? (usage+1)/2 : usage/2); i++)
                nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_DISK_BAR_FULL, (MODE_FULL_ACTIVE_W - 210 + i), (MODE_FULL_ACTIVE_H - 65), 1, 26, NFALIGN_LEFT, 0);

            g_sprintf(strBuf, "%d%%", usage);
            vwnd_draw_text(strBuf, (MODE_FULL_ACTIVE_W - 200), (MODE_FULL_ACTIVE_H - 61), COLOR_IDX(308), COLOR_IDX(307), NFFONT_SMALL_SEMI, NORMAL_SPACING, TRUE);
        }
    }
    else if (status & (1 << DST_OW))
    {
        nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_F_DISK_NORMAL, x, y, -1, -1, NFALIGN_LEFT, 0);
        vwnd_draw_text("OW", (MODE_FULL_ACTIVE_W - 200), (MODE_FULL_ACTIVE_H - 61), COLOR_IDX(308), COLOR_IDX(307), NFFONT_SMALL_SEMI, NORMAL_SPACING, TRUE);
    }
    else if (status & (1 << DST_FULL))
    {
        nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_F_DISK_FULL, x, y, -1, -1, NFALIGN_LEFT, 0);

        for(i=0; i<(usage%2 ? (usage+1)/2 : usage/2); i++)
            nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_DISK_BAR_FULL, (MODE_FULL_ACTIVE_W - 210 + i), (MODE_FULL_ACTIVE_H - 65), 1, 26, NFALIGN_LEFT, 0);

        vwnd_draw_text("100%", (MODE_FULL_ACTIVE_W - 210), (MODE_FULL_ACTIVE_H - 61), COLOR_IDX(308), COLOR_IDX(307), NFFONT_SMALL_SEMI, NORMAL_SPACING, TRUE);
    }
    else if (status & (1 << DST_NO))
    {
        nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_F_DISK_NO, x, y, -1, -1, NFALIGN_LEFT, 0);
    }
    else if (status & (1 << DST_NORMAL))
    {
        g_sprintf(strBuf, "%d%%", usage);
        nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_F_DISK_NORMAL, x, y, -1, -1, NFALIGN_LEFT, 0);

        for(i=0; i<(usage%2 ? (usage+1)/2 : usage/2); i++)
            nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_DISK_BAR_NORMAL, (MODE_FULL_ACTIVE_W - 210 + i), (MODE_FULL_ACTIVE_H - 65), 1, 26, NFALIGN_LEFT, 0);

        vwnd_draw_text(strBuf, (MODE_FULL_ACTIVE_W - 200), (MODE_FULL_ACTIVE_H - 61), COLOR_IDX(308), COLOR_IDX(307), NFFONT_SMALL_SEMI, NORMAL_SPACING, TRUE);
    }

    return TRUE;
}

static void _set_postion_arch_status(VWND_T *p)
{
    guint x, y;

    x = MODE_FULL_ACTIVE_W- 146;
    y = MODE_FULL_ACTIVE_H - 89;

    p->archST_area.x = x;
    p->archST_area.y = y;
    p->archST_area.width = ICON_SIZE_W(VWND_ARCH_ICON);
    p->archST_area.height = ICON_SIZE_H(VWND_ARCH_ICON);

#ifdef DBG_DRAW_AREA
    vwnd_paint_area(x, y, ICON_SIZE_W(VWND_ARCH_ICON), ICON_SIZE_H(VWND_ARCH_ICON), COLOR_PRG_IDX(UX_COLOR_FF0000));
#endif
}

static gboolean vwnd_draw_arch_status(VWND_T *p)
{
    guint x, y;
    gchar strBuf[10];
    ARCH_ST status = p->arch_st;
    gint cnt = p->arch_cnt;

    if (!status) return FALSE;
    if (_check_redraw_rect(&p->archST_area)) return FALSE;

    DMSG(1, "__VWND_DRAW_ARCH_STATUS");

    memset(strBuf, 0x00, sizeof(strBuf));

    x = MODE_FULL_ACTIVE_W- 146;
    y = MODE_FULL_ACTIVE_H - 89;

    nfutil_draw_image(g_vw_drawable, g_vw_gc, IMG_F_BACKUP_STATUS, x, y, -1, -1, NFALIGN_LEFT, 0);
    g_sprintf(strBuf, "%d%%", cnt);

    return TRUE;
}

static void _set_postion_clondev_text(VWND_T *p)
{
    p->clondev_area.x = 100;
    p->clondev_area.y = 100;
    p->clondev_area.width = 1720;
    p->clondev_area.height = 80;

#ifdef DBG_DRAW_AREA
    vwnd_paint_area(p->clondev_area.x, p->clondev_area.x, p->clondev_area.width, p->clondev_area.height, COLOR_PRG_IDX(UX_COLOR_FF0000));
#endif
}

static gboolean vwnd_draw_clondev_text(VWND_T *p)
{
    guint x, y;
    guint string_w = 0, string_h = 0;
    gint valid_cnt, tmp_w;
    gint is_draw = p->clondev;
    gchar strBuf[256];

    if (!is_draw) return FALSE;
    if (_check_redraw_rect(&p->clondev_area)) return FALSE;

    DMSG(1, "__VWND_DRAW_CLONDEV_TEXT");

    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, lookup_string("Some features are limited due to suspected replicas."));

    string_w = nfutil_string_width(0, g_vw_drawable, nffont_get_pango_font(NFFONT_LARGE_SEMI_1), strBuf, NORMAL_SPACING);
    string_h = nfutil_string_height(g_vw_drawable, nffont_get_pango_font(NFFONT_LARGE_SEMI_1), strBuf, NORMAL_SPACING);

    if (string_w > 1720) {
        tmp_w = nfutil_string_width(0, g_vw_drawable, nffont_get_pango_font(NFFONT_LARGE_SEMI_1), "...", NORMAL_SPACING);
        valid_cnt = nfutil_string_get_valid_count(0, g_vw_drawable, nffont_get_pango_font(NFFONT_LARGE_SEMI_1), strBuf, 1720-tmp_w);
        g_utf8_strncpy(strBuf, strBuf, valid_cnt);
        strcat(strBuf, "...");

        x = 0;
    }
    else {
        x = (1720-string_w)/2 + 100;
    }

    y = 100;

    vwnd_draw_text(strBuf, x, y, COLOR_IDX(308), COLOR_PRG_IDX(UX_COLOR_FF0000), NFFONT_LARGE_SEMI_1, NORMAL_SPACING, FALSE);

    return TRUE;
}

static void _set_postion_pos_text(VWND_T *p, VSM_ID_E wpos)
{
    gint x = 0, y = 0, w = 0, h = 0;
    VSM_DIV_E dtype = p->dtype;
    gint i, ey;

    gint font_h = 40;

    if (p->pos_cf.font_type == 0)       font_h = 38;
    else if (p->pos_cf.font_type == 1)  font_h = 44;
    else if (p->pos_cf.font_type == 2)  font_h = 50;

    vwnd_get_position(dtype, wpos, &x, &y);
    vwnd_get_size(dtype, wpos, &w, &h);

    x += 20;
    y += 20;
    w -= 40;
    h -= 40;

    if (p->plt_h == 1080)
    {
        if (dtype == VSM_DIV1) h -= 88;
        else if ((dtype == VSM_DIV4) && (wpos >= 2)) h -= 88;
        else if ((dtype == VSM_DIV6) && (wpos >= 3)) h -= 88;
        else if ((dtype == VSM_DIV8) && (wpos >= 4)) h -= 88;
        else if ((dtype == VSM_DIV9) && (wpos >= 6)) h -= 88;
        else if ((dtype == VSM_DIV16) && (wpos >= 13)) h -= 88;
    }

    ey = y + h;

    memset(p->winfo[wpos].pos_area, 0x00, sizeof(GdkRectangle)*VWND_POS_LINE);

    for (i = 0; i < VWND_POS_LINE; i++)
    {
        p->winfo[wpos].pos_area[i].x = x;
        p->winfo[wpos].pos_area[i].y = y;
        p->winfo[wpos].pos_area[i].width = w;
        p->winfo[wpos].pos_area[i].height = font_h;

        y += font_h;

        if (y+font_h > ey) break;
    }

    p->winfo[wpos].pos_row = i+1;

#ifdef DBG_DRAW_AREA
    vwnd_paint_area(p->winfo[wpos].pos_area[0].x, p->winfo[wpos].pos_area[0].y, w, font_h, COLOR_PRG_IDX(UX_COLOR_FF0000));
#endif
}

static gboolean vwnd_draw_pos_text(VWND_T *p, VSM_ID_E wpos, gint row)
{
    VSM_DIV_E dtype = p->dtype;
    gchar strBuf[128];
    gint i, string_w;
    gint valid_cnt, tmp_w;
    gint pos_x;

    gint font_type = NFFONT_LARGE_SEMI;

    if (dtype == VSM_DIV9) return FALSE;
    if (dtype == VSM_DIV16) return FALSE;
    if ((dtype == VSM_DIV6) && (wpos > 0)) return FALSE;
    if ((dtype == VSM_DIV8) && (wpos > 0)) return FALSE;

    if (!p->pos_onoff) return FALSE;
    if (!strlen(p->winfo[wpos].pos_text[row])) return FALSE;
    if (_check_redraw_rect(&p->winfo[wpos].pos_area[row])) return FALSE;

    if (p->pos_cf.font_type == 0)       font_type = NFFONT_SMALL_SEMI;
    else if (p->pos_cf.font_type == 1)  font_type = NFFONT_LARGE_SEMI;
    else if (p->pos_cf.font_type == 2)  font_type = NFFONT_XLARGE_SEMI;

    if ((dtype == VSM_DIV6) || (dtype == VSM_DIV8))
    {
        if (font_type > NFFONT_LARGE_SEMI) font_type = NFFONT_LARGE_SEMI;
    }
    else if (dtype == VSM_DIV4)
    {
        if (font_type > NFFONT_SMALL_SEMI) font_type = NFFONT_SMALL_SEMI;
    }

    string_w = nfutil_string_width(0, g_vw_drawable, nffont_get_pango_font(font_type), p->winfo[wpos].pos_text[row], SEMI_CONDENSED_SPACING);

    if (string_w >= p->winfo[wpos].pos_area[row].width)
    {
        tmp_w = nfutil_string_width(0, g_vw_drawable, nffont_get_pango_font(font_type), "...", SEMI_CONDENSED_SPACING);
        valid_cnt = nfutil_string_get_valid_count(0, g_vw_drawable, nffont_get_pango_font(font_type), p->winfo[wpos].pos_text[row], p->winfo[wpos].pos_area[row].width-tmp_w);
        g_utf8_strncpy(strBuf, p->winfo[wpos].pos_text[row], valid_cnt);
        strcat(strBuf, "...");

        string_w = nfutil_string_width(0, g_vw_drawable, nffont_get_pango_font(font_type), strBuf, SEMI_CONDENSED_SPACING);
    }
    else
        strcpy(strBuf, p->winfo[wpos].pos_text[row]);

    if (p->pos_cf.align == 1) pos_x = p->winfo[wpos].pos_area[row].x + p->winfo[wpos].pos_area[row].width - string_w;
    else pos_x = p->winfo[wpos].pos_area[row].x;

    vwnd_draw_text(strBuf, pos_x, p->winfo[wpos].pos_area[row].y,
        -1, COLOR_IDX(p->winfo[wpos].pos_color[row]), font_type, SEMI_CONDENSED_SPACING, TRUE);

    return TRUE;
}

static void _set_position_dlva_counter(VWND_T *p, VSM_ID_E wpos, gint cnt)
{
    gint x = 0, y = 0, w = 0, h = 0;
    VSM_DIV_E dtype = p->dtype;

    vwnd_get_position(dtype, wpos, &x, &y);
    vwnd_get_size(dtype, wpos, &w, &h);

    x += ICON_SIZE_W(VWND_SEQ_ICON)/2;
    y += (VWND_TITLE_GAP_Y + FONT_H + VWND_ICON_GAP_2);

    if((dtype == VSM_DIV8) || (dtype==VSM_DIV16))
    {
        x += (cnt / 3) * (ICON_SIZE_W(VWND_DLVACNT_ICON) + VWND_ICON_GAP_2);
        y += (ICON_SIZE_H(VWND_DLVACNT_ICON) + VWND_ICON_GAP_2) * (cnt % 3);

        p->winfo[wpos].dlva_cntr_area[cnt].x = x;
        p->winfo[wpos].dlva_cntr_area[cnt].y = y;
        p->winfo[wpos].dlva_cntr_area[cnt].width = ICON_SIZE_W(VWND_DLVACNT_ICON);
        p->winfo[wpos].dlva_cntr_area[cnt].height = ICON_SIZE_H(VWND_DLVACNT_ICON);
    }
    else if(dtype != VSM_DIV1)
    {
        x += (cnt / 5) * (ICON_SIZE_W(VWND_DLVACNT_ICON) + VWND_ICON_GAP_2);
        y += (ICON_SIZE_H(VWND_DLVACNT_ICON) + VWND_ICON_GAP_2) * (cnt % 5);

        p->winfo[wpos].dlva_cntr_area[cnt].x = x;
        p->winfo[wpos].dlva_cntr_area[cnt].y = y;
        p->winfo[wpos].dlva_cntr_area[cnt].width = ICON_SIZE_W(VWND_DLVACNT_ICON);
        p->winfo[wpos].dlva_cntr_area[cnt].height = ICON_SIZE_H(VWND_DLVACNT_ICON);
    }
    else
    {
        y += (ICON_SIZE_H(VWND_DLVACNT_ICON) + VWND_ICON_GAP_2)*cnt;

        p->winfo[wpos].dlva_cntr_area[cnt].x = x;
        p->winfo[wpos].dlva_cntr_area[cnt].y = y;
        p->winfo[wpos].dlva_cntr_area[cnt].width = ICON_SIZE_W(VWND_DLVACNT_ICON);
        p->winfo[wpos].dlva_cntr_area[cnt].height = ICON_SIZE_H(VWND_DLVACNT_ICON);
    }

#ifdef DBG_DRAW_AREA
    vwnd_paint_area(x, y, ICON_SIZE_W(VWND_DLVACNT_ICON), ICON_SIZE_H(VWND_DLVACNT_ICON), COLOR_PRG_IDX(UX_COLOR_FF0000));
#endif
}

static void _draw_dlva_counter_icon(gint x, gint y, gint icon, gchar *str)
{
    gchar img_name[64];

    gint string_w;
    gint font_type = NFFONT_LARGE_SEMI;

    memset(img_name, 0x00, sizeof(img_name));

    switch(icon) {
        case DLVA_CNTR_ICON_HUMAN:      strcpy(img_name, "live_human_count.png"); break;
        case DLVA_CNTR_ICON_VEHICLE:    strcpy(img_name, "live_vehicle_count.png"); break;
        case DLVA_CNTR_ICON_ANIMAL:     strcpy(img_name, "live_animal_count.png"); break;
        default: break;
    }

    if (strlen(img_name))
        nfutil_draw_image(g_vw_drawable, g_vw_gc, img_name, (gint)x, (gint)y, ICON_SIZE_W(VWND_DLVACNT_ICON), ICON_SIZE_H(VWND_DLVACNT_ICON), NFALIGN_LEFT, 0);

    string_w = nfutil_string_width(0, g_vw_drawable, nffont_get_pango_font(NFFONT_MINI_SEMI_5), str, SEMI_CONDENSED_SPACING);
    vwnd_draw_text(str, (gint)x+(53-string_w)/2, (gint)y+27, COLOR_IDX(308), COLOR_PRG_IDX(UX_COLOR_FFFFFF), NFFONT_MINI_SEMI_5, SEMI_CONDENSED_SPACING, TRUE);
}

static gboolean vwnd_draw_dlva_counter(VWND_T *p, VSM_ID_E wpos, gint cnt)
{
    gint x = 0, y = 0, w = 0, h = 0;
    DLVA_CNTR_ICON icon = p->winfo[wpos].dlva_cntr_icon[cnt];
    gint strBuf[32];
    VSM_DIV_E dtype = p->dtype;

    g_return_val_if_fail(icon >= DLVA_CNTR_ICON_NONE, FALSE);
    g_return_val_if_fail(icon <= DLVA_CNTR_ICON_NUM, FALSE);

    if (p->osd_off) return FALSE;
    if (icon == DLVA_CNTR_ICON_NONE) return FALSE;
    if (_check_redraw_rect(&p->winfo[wpos].dlva_cntr_area[cnt])) return FALSE;

    DMSG(1, "__VWND_DRAW_DLVA_COUNTER_ICON : idx-%d, icon-%d, val-%d", cnt, icon, p->winfo[wpos].dlva_cntr_val[cnt]);

    vwnd_get_position(dtype, wpos, &x, &y);
    vwnd_get_size(dtype, wpos, &w, &h);

    x += ICON_SIZE_W(VWND_SEQ_ICON)/2;
    y += (VWND_TITLE_GAP_Y + FONT_H + VWND_ICON_GAP_2);

    if((dtype == VSM_DIV8) || (dtype==VSM_DIV16))
    {
        x += (cnt / 3) * (ICON_SIZE_W(VWND_DLVACNT_ICON) + VWND_ICON_GAP_2);
        y += (ICON_SIZE_H(VWND_DLVACNT_ICON) + VWND_ICON_GAP_2) * (cnt % 3);

        p->winfo[wpos].dlva_cntr_area[cnt].x = x;
        p->winfo[wpos].dlva_cntr_area[cnt].y = y;
        p->winfo[wpos].dlva_cntr_area[cnt].width = ICON_SIZE_W(VWND_DLVACNT_ICON);
        p->winfo[wpos].dlva_cntr_area[cnt].height = ICON_SIZE_H(VWND_DLVACNT_ICON);
    }
    else if(dtype != VSM_DIV1)
    {
        x += (cnt / 5) * (ICON_SIZE_W(VWND_DLVACNT_ICON) + VWND_ICON_GAP_2);
        y += (ICON_SIZE_H(VWND_DLVACNT_ICON) + VWND_ICON_GAP_2) * (cnt % 5);

        p->winfo[wpos].dlva_cntr_area[cnt].x = x;
        p->winfo[wpos].dlva_cntr_area[cnt].y = y;
        p->winfo[wpos].dlva_cntr_area[cnt].width = ICON_SIZE_W(VWND_DLVACNT_ICON);
        p->winfo[wpos].dlva_cntr_area[cnt].height = ICON_SIZE_H(VWND_DLVACNT_ICON);
    }
    else
    {
        y += (ICON_SIZE_H(VWND_DLVACNT_ICON) + VWND_ICON_GAP_2)*cnt;

        p->winfo[wpos].dlva_cntr_area[cnt].x = x;
        p->winfo[wpos].dlva_cntr_area[cnt].y = y;
        p->winfo[wpos].dlva_cntr_area[cnt].width = ICON_SIZE_W(VWND_DLVACNT_ICON);
        p->winfo[wpos].dlva_cntr_area[cnt].height = ICON_SIZE_H(VWND_DLVACNT_ICON);
    }

    memset(strBuf, 0x00, sizeof(strBuf));
    if (p->winfo[wpos].dlva_cntr_val[cnt] > 999) g_sprintf(strBuf, "%s", "999+");
    else g_sprintf(strBuf, "%d", p->winfo[wpos].dlva_cntr_val[cnt]);
    _draw_dlva_counter_icon(x, y, icon, strBuf);

    return TRUE;
}

static void _set_postion_itx_vca(VWND_T *p, VSM_ID_E wpos, gint cnt)
{
    gint x = 0, y = 0, w = 0, h = 0;
    VSM_DIV_E dtype = p->dtype;

    vwnd_get_position(dtype, wpos, &x, &y);
    vwnd_get_size(dtype, wpos, &w, &h);

    x += ICON_SIZE_W(VWND_SEQ_ICON)/2;
    y += (VWND_TITLE_GAP_Y + FONT_H + VWND_ICON_GAP_2);

    if((dtype == VSM_DIV8) || (dtype==VSM_DIV16))
    {
        x += (cnt / 3) * (ICON_SIZE_W(VWND_VCA_ICON) + VWND_ICON_GAP_2);
        y += (ICON_SIZE_H(VWND_VCA_ICON) + VWND_ICON_GAP_2) * (cnt % 3);
    }
    else if ((dtype == VSM_DIV6) || (dtype==VSM_DIV9))
    {
        x += (cnt / 4) * (ICON_SIZE_W(VWND_VCA_ICON) + VWND_ICON_GAP_2);
        y += (ICON_SIZE_H(VWND_VCA_ICON) + VWND_ICON_GAP_2) * (cnt % 4);
    }
    else if(dtype != VSM_DIV1)
    {
        x += (cnt / 5) * (ICON_SIZE_W(VWND_VCA_ICON) + VWND_ICON_GAP_2);
        y += (ICON_SIZE_H(VWND_VCA_ICON) + VWND_ICON_GAP_2) * (cnt % 5);
    }
    else
    {
        y += (ICON_SIZE_H(VWND_VCA_ICON) + VWND_ICON_GAP_2)*cnt;
    }

    p->winfo[wpos].vca_area[cnt].x = x;
    p->winfo[wpos].vca_area[cnt].y = y;
    p->winfo[wpos].vca_area[cnt].width = ICON_SIZE_W(VWND_VCA_ICON);
    p->winfo[wpos].vca_area[cnt].height = ICON_SIZE_H(VWND_VCA_ICON);

#ifdef DBG_DRAW_AREA
    vwnd_paint_area(x, y, ICON_SIZE_W(VWND_VCA_ICON), ICON_SIZE_H(VWND_VCA_ICON), COLOR_PRG_IDX(UX_COLOR_FF0000));
#endif
}

static void _set_postion_s1_vca(VWND_T *p, VSM_ID_E wpos, gint cnt)
{
  gint x = 0, y = 0, w = 0, h = 0;
  VSM_DIV_E dtype = p->dtype;

  vwnd_get_position(dtype, wpos, &x, &y);
  vwnd_get_size(dtype, wpos, &w, &h);

  x += ICON_SIZE_W(VWND_SEQ_ICON)/2;
  y += (VWND_TITLE_GAP_Y + FONT_H + VWND_ICON_GAP_2);
  y += (ICON_SIZE_H(VWND_VCA_ICON) + VWND_ICON_GAP_2)*cnt;

  p->winfo[wpos].vca_area[cnt].x = x;
  p->winfo[wpos].vca_area[cnt].y = y;
  p->winfo[wpos].vca_area[cnt].width = ICON_SIZE_W(VWND_VCA_ICON) + 200;
  p->winfo[wpos].vca_area[cnt].height = ICON_SIZE_H(VWND_VCA_ICON);

#ifdef DBG_DRAW_AREA
  vwnd_paint_area(x, y, ICON_SIZE_W(VWND_VCA_ICON), ICON_SIZE_H(VWND_VCA_ICON), COLOR_PRG_IDX(UX_COLOR_FF0000));
#endif
}

static void _draw_itx_vca_icon(gint x, gint y, gint icon)
{
    gchar img_name[64];

    memset(img_name, 0x00, sizeof(img_name));

    switch(icon) {
        case ITX_VCA_ICON_DIR_NEG_E:    strcpy(img_name, IMG_LIVE_DIRECTION_REVERSE_N); break;
        case ITX_VCA_ICON_DIR_NEG_N:    strcpy(img_name, IMG_LIVE_DIRECTION_REVERSE_D); break;
        case ITX_VCA_ICON_DIR_POS_E:    strcpy(img_name, IMG_LIVE_DIRECTION_FORWARD_N); break;
        case ITX_VCA_ICON_DIR_POS_N:    strcpy(img_name, IMG_LIVE_DIRECTION_FORWARD_D); break;
        case ITX_VCA_ICON_ENTER_E:      strcpy(img_name, IMG_LIVE_ENTER_N);             break;
        case ITX_VCA_ICON_ENTER_N:      strcpy(img_name, IMG_LIVE_ENTER_D);             break;
        case ITX_VCA_ICON_EXIT_E:       strcpy(img_name, IMG_LIVE_EXIT_N);              break;
        case ITX_VCA_ICON_EXIT_N:       strcpy(img_name, IMG_LIVE_EXIT_D);              break;
        case ITX_VCA_ICON_STOP_E:       strcpy(img_name, IMG_LIVE_STOP_N);              break;
        case ITX_VCA_ICON_STOP_N:       strcpy(img_name, IMG_LIVE_STOP_D);              break;
        case ITX_VCA_ICON_REMOVE_E:     strcpy(img_name, IMG_LIVE_REMOVE_N);            break;
        case ITX_VCA_ICON_REMOVE_N:     strcpy(img_name, IMG_LIVE_REMOVE_D);            break;
        case ITX_VCA_ICON_LOITER_E:     strcpy(img_name, IMG_LIVE_LOITERING_N);         break;
        case ITX_VCA_ICON_LOITER_N:     strcpy(img_name, IMG_LIVE_LOITERING_D);         break;
        case ITX_VCA_ICON_FALL_E:       strcpy(img_name, IMG_LIVE_FALL_N);              break;
        case ITX_VCA_ICON_FALL_N:       strcpy(img_name, IMG_LIVE_FALL_D);              break;
        case ITX_VCA_ICON_COUNT_E:      strcpy(img_name, IMG_LIVE_COUNT_N);             break;
        case ITX_VCA_ICON_COUNT_N:      strcpy(img_name, IMG_LIVE_COUNT_D);             break;
    }

    if (strlen(img_name))
        nfutil_draw_image(g_vw_drawable, g_vw_gc, img_name, (gint)x, (gint)y, ICON_SIZE_W(VWND_VCA_ICON), ICON_SIZE_H(VWND_VCA_ICON), NFALIGN_LEFT, 0);
}

static void _draw_s1_vca_icon(gint x, gint y, gint icon)
{
    gchar img_name[64];

    memset(img_name, 0x00, sizeof(img_name));

  switch(icon) {
    case S1_VCA_ICON_INVASION_N:   strcpy(img_name, IMG_LIVE_VA_ICON_D_01); break;
    case S1_VCA_ICON_INVASION_E:   strcpy(img_name, IMG_LIVE_VA_ICON_01);    break;
    case S1_VCA_ICON_LOITERING_N:  strcpy(img_name, IMG_LIVE_VA_ICON_D_02); break;
    case S1_VCA_ICON_LOITERING_E:  strcpy(img_name, IMG_LIVE_VA_ICON_02);    break;
    case S1_VCA_ICON_ABANDON_N:      strcpy(img_name, IMG_LIVE_VA_ICON_D_07); break;
    case S1_VCA_ICON_ABANDON_E:      strcpy(img_name, IMG_LIVE_VA_ICON_07);   break;
    case S1_VCA_ICON_STEAL_N:    strcpy(img_name, IMG_LIVE_VA_ICON_D_08); break;
    case S1_VCA_ICON_STEAL_E:    strcpy(img_name, IMG_LIVE_VA_ICON_08);   break;
    case S1_VCA_ICON_TOPPLE_N:    strcpy(img_name, IMG_LIVE_VA_ICON_D_09); break;
    case S1_VCA_ICON_TOPPLE_E:    strcpy(img_name, IMG_LIVE_VA_ICON_09);   break;
    case S1_VCA_ICON_FENCE_N:    strcpy(img_name, IMG_LIVE_VA_ICON_D_03); break;
    case S1_VCA_ICON_FENCE_E:    strcpy(img_name, IMG_LIVE_VA_ICON_03);   break;
    case S1_VCA_ICON_COUNT_N:    strcpy(img_name, IMG_LIVE_VA_ICON_D_04); break;
    case S1_VCA_ICON_COUNT_E:    strcpy(img_name, IMG_LIVE_VA_ICON_04);   break;
    case S1_VCA_ICON_TAMPERING_N:  strcpy(img_name, IMG_LIVE_VA_ICON_D_06); break;
    case S1_VCA_ICON_TAMPERING_E:  strcpy(img_name, IMG_LIVE_VA_ICON_06);   break;
    case S1_VCA_ICON_PRIVACY_N:      strcpy(img_name, IMG_LIVE_VA_ICON_D_10); break;
    case S1_VCA_ICON_PRIVACY_E:      strcpy(img_name, IMG_LIVE_VA_ICON_10);   break;
  }

    if (strlen(img_name))
        nfutil_draw_image(g_vw_drawable, g_vw_gc, img_name, (gint)x, (gint)y, ICON_SIZE_W(VWND_VCA_ICON), ICON_SIZE_H(VWND_VCA_ICON), NFALIGN_LEFT, 0);
}

static gboolean vwnd_draw_itx_vca(VWND_T *p, VSM_ID_E wpos, gint cnt)
{
    gint x = 0, y = 0, w = 0, h = 0;
    ITX_VCA_ICON icon = p->winfo[wpos].vca_itx_icon[cnt];
    VSM_DIV_E dtype = p->dtype;

    g_return_val_if_fail(icon >= ITX_VCA_ICON_NONE, FALSE);
    g_return_val_if_fail(icon <= ITX_VCA_ICON_NUM, FALSE);

    if (p->osd_off) return FALSE;
    if (icon == ITX_VCA_ICON_NONE) return FALSE;
    if (_check_redraw_rect(&p->winfo[wpos].vca_area[cnt])) return FALSE;

    DMSG(1, "__VWND_DRAW_VCA_ICON : %d", cnt);

    vwnd_get_position(dtype, wpos, &x, &y);
    vwnd_get_size(dtype, wpos, &w, &h);

    x += ICON_SIZE_W(VWND_SEQ_ICON)/2;
    y += (VWND_TITLE_GAP_Y + FONT_H + VWND_ICON_GAP_2);

    if((dtype == VSM_DIV8) || (dtype==VSM_DIV16))
    {
        x += (cnt / 3) * (ICON_SIZE_W(VWND_VCA_ICON) + VWND_ICON_GAP_2);
        y += (ICON_SIZE_H(VWND_VCA_ICON) + VWND_ICON_GAP_2) * (cnt % 3);
    }
    else if ((dtype == VSM_DIV6) || (dtype==VSM_DIV9))
    {
        x += (cnt / 4) * (ICON_SIZE_W(VWND_VCA_ICON) + VWND_ICON_GAP_2);
        y += (ICON_SIZE_H(VWND_VCA_ICON) + VWND_ICON_GAP_2) * (cnt % 4);
    }    
    else if(dtype != VSM_DIV1)
    {
        x += (cnt / 5) * (ICON_SIZE_W(VWND_VCA_ICON) + VWND_ICON_GAP_2);
        y += (ICON_SIZE_H(VWND_VCA_ICON) + VWND_ICON_GAP_2) * (cnt % 5);
    }
    else
    {
        y += (ICON_SIZE_H(VWND_VCA_ICON) + VWND_ICON_GAP_2)*cnt;
    }

    p->winfo[wpos].vca_area[cnt].x = x;
    p->winfo[wpos].vca_area[cnt].y = y;
    p->winfo[wpos].vca_area[cnt].width = ICON_SIZE_W(VWND_VCA_ICON);
    p->winfo[wpos].vca_area[cnt].height = ICON_SIZE_H(VWND_VCA_ICON);

    _draw_itx_vca_icon(x, y, icon);

    return TRUE;
}

static gboolean vwnd_draw_s1_vca(VWND_T *p, VSM_ID_E wpos, gint cnt)
{
    gint x = 0, y = 0, w = 0, h = 0;
    S1_VCA_ICON icon = p->winfo[wpos].vca_s1_icon[cnt];
    VSM_DIV_E dtype = p->dtype;

    g_return_val_if_fail(icon >= S1_VCA_ICON_NONE, FALSE);
    g_return_val_if_fail(icon <= S1_VCA_ICON_PRIVACY_E, FALSE);

    if (p->osd_off) return FALSE;
    if (icon == S1_VCA_ICON_NONE) return FALSE;
    if (_check_redraw_rect(&p->winfo[wpos].vca_area[cnt])) return FALSE;

  DMSG(1, "__VWND_DRAW_VCA_ICON : %d", cnt);

    vwnd_get_position(dtype, wpos, &x, &y);
    vwnd_get_size(dtype, wpos, &w, &h);

    x += ICON_SIZE_W(VWND_SEQ_ICON)/2;
    y += (VWND_TITLE_GAP_Y + FONT_H + VWND_ICON_GAP_2);
    y += (ICON_SIZE_H(VWND_VCA_ICON) + VWND_ICON_GAP_2)*cnt;

    _draw_s1_vca_icon(x, y, icon);

    x += ICON_SIZE_W(VWND_VCA_ICON);
    x += 4;
    y += 4;
    vwnd_draw_text(p->winfo[wpos].vca_text[cnt], x, y, COLOR_IDX(308), COLOR_PRG_IDX(UX_COLOR_FF0000), NFFONT_MEDIUM_SEMI, NORMAL_SPACING, TRUE);
    return TRUE;
}

static gboolean vwnd_draw_vcaline(VWND_T *p, VSM_ID_E wpos)
{
    VSM_DIV_E dtype = p->dtype;
    VCA_DP dp;
    guint x, y;
    gint width, height;
    VCA_CLON clon;

    if (!p->winfo[wpos].vcaline) return FALSE;
    if (p->winfo[wpos].vca_updating) return FALSE;

    DMSG(1, "__VWND_DRAW_VCA_RULE");

    dp.drawable = g_vw_drawable;
    dp.gc = g_vw_gc;

    vwnd_get_position(dtype, wpos, &x, &y);
    vwnd_get_size(dtype, wpos, &width, &height);

    dp.plt_area.x = x;
    dp.plt_area.y = y;
    dp.plt_area.width = width;
    dp.plt_area.height = height;

    clon.dic_cnt = p->winfo[wpos].vca_dic_cnt;
    clon.pdics = p->winfo[wpos].vca_pdics;
    vw_dit_display_vca_draw(&dp, clon);

    return TRUE;
}

static void _set_postion_itx_dvabx(VWND_T *p, VSM_ID_E wpos, gint cnt)
{
    gint x = 0, y = 0, w = 0, h = 0;
    VSM_DIV_E dtype = p->dtype;

    gint i;

    vwnd_get_position(dtype, wpos, &x, &y);
    vwnd_get_size(dtype, wpos, &w, &h);

    if ((dtype == VSM_DIV32) || (dtype==VSM_DIV36))
    {
        x += 25;
        y += h - ((ICON_SIZE_H(VWND_DVABX_ICON) + VWND_ICON_GAP_2) * 2);
    }
    else
    {
        x += ICON_SIZE_W(VWND_SEQ_ICON)/2;
        y += (VWND_TITLE_GAP_Y + FONT_H + VWND_ICON_GAP_2);
    }

    if ((dtype == VSM_DIV32) || (dtype==VSM_DIV36))
    {
        x += (cnt / 2) * (ICON_SIZE_W(VWND_DVABX_ICON) + VWND_ICON_GAP_2);
        y += (ICON_SIZE_H(VWND_DVABX_ICON) + VWND_ICON_GAP_2) * (cnt % 2);
    }
    else if ((dtype == VSM_DIV8) || (dtype==VSM_DIV16))
    {
        x += (cnt / 3) * (ICON_SIZE_W(VWND_DVABX_ICON) + VWND_ICON_GAP_2);
        y += (ICON_SIZE_H(VWND_DVABX_ICON) + VWND_ICON_GAP_2) * (cnt % 3);
    }
    else if ((dtype == VSM_DIV6) || (dtype==VSM_DIV9))
    {
        x += (cnt / 4) * (ICON_SIZE_W(VWND_DVABX_ICON) + VWND_ICON_GAP_2);
        y += (ICON_SIZE_H(VWND_DVABX_ICON) + VWND_ICON_GAP_2) * (cnt % 4);
    }    
    else if (dtype != VSM_DIV1)
    {
        x += (cnt / 5) * (ICON_SIZE_W(VWND_DVABX_ICON) + VWND_ICON_GAP_2);
        y += (ICON_SIZE_H(VWND_DVABX_ICON) + VWND_ICON_GAP_2) * (cnt % 5);
    }
    else
    {
        y += (ICON_SIZE_H(VWND_DVABX_ICON) + VWND_ICON_GAP_2)*cnt;
    }

    p->winfo[wpos].dvabx_area[cnt].x = x;
    p->winfo[wpos].dvabx_area[cnt].y = y;
    p->winfo[wpos].dvabx_area[cnt].width = ICON_SIZE_W(VWND_DVABX_ICON);
    p->winfo[wpos].dvabx_area[cnt].height = ICON_SIZE_H(VWND_DVABX_ICON);

#ifdef DBG_DRAW_AREA
    vwnd_paint_area(x, y, ICON_SIZE_W(VWND_DVABX_ICON), ICON_SIZE_H(VWND_DVABX_ICON), COLOR_PRG_IDX(UX_COLOR_FF0000));
#endif
}

static void _draw_itx_dvabx_icon(gint x, gint y, gint icon)
{
    gchar img_name[64];

    memset(img_name, 0x00, sizeof(img_name));

    switch(icon) {
        case ITX_DVABX_ICON_DIR_NEG_E:    strcpy(img_name, IMG_LIVE_DIRECTION_REVERSE_N); break;
        case ITX_DVABX_ICON_DIR_NEG_N:    strcpy(img_name, IMG_LIVE_DIRECTION_REVERSE_D); break;
        case ITX_DVABX_ICON_DIR_POS_E:    strcpy(img_name, IMG_LIVE_DIRECTION_FORWARD_N); break;
        case ITX_DVABX_ICON_DIR_POS_N:    strcpy(img_name, IMG_LIVE_DIRECTION_FORWARD_D); break;
        case ITX_DVABX_ICON_INTRUSION_E:  strcpy(img_name, IMG_LIVE_INTRUSION_N);         break;
        case ITX_DVABX_ICON_INTRUSION_N:  strcpy(img_name, IMG_LIVE_INTRUSION_D);         break;        
        case ITX_DVABX_ICON_ENTER_E:      strcpy(img_name, IMG_LIVE_ENTER_N);             break;
        case ITX_DVABX_ICON_ENTER_N:      strcpy(img_name, IMG_LIVE_ENTER_D);             break;
        case ITX_DVABX_ICON_EXIT_E:       strcpy(img_name, IMG_LIVE_EXIT_N);              break;
        case ITX_DVABX_ICON_EXIT_N:       strcpy(img_name, IMG_LIVE_EXIT_D);              break;
        case ITX_DVABX_ICON_STOP_E:       strcpy(img_name, IMG_LIVE_STOP_N);              break;
        case ITX_DVABX_ICON_STOP_N:       strcpy(img_name, IMG_LIVE_STOP_D);              break;
        case ITX_DVABX_ICON_REMOVE_E:     strcpy(img_name, IMG_LIVE_REMOVE_N);            break;
        case ITX_DVABX_ICON_REMOVE_N:     strcpy(img_name, IMG_LIVE_REMOVE_D);            break;
        case ITX_DVABX_ICON_LOITER_E:     strcpy(img_name, IMG_LIVE_LOITERING_N);         break;
        case ITX_DVABX_ICON_LOITER_N:     strcpy(img_name, IMG_LIVE_LOITERING_D);         break;
        case ITX_DVABX_ICON_FALL_E:       strcpy(img_name, IMG_LIVE_FALL_N);              break;
        case ITX_DVABX_ICON_FALL_N:       strcpy(img_name, IMG_LIVE_FALL_D);              break;
        case ITX_DVABX_ICON_COUNT_E:      strcpy(img_name, IMG_LIVE_COUNT_N);             break;
        case ITX_DVABX_ICON_COUNT_N:      strcpy(img_name, IMG_LIVE_COUNT_D);             break;
        case ITX_DVABX_ICON_GENERIC_E:    strcpy(img_name, IMG_LIVE_GENERIC_N);           break;
        case ITX_DVABX_ICON_GENERIC_N:    strcpy(img_name, IMG_LIVE_GENERIC_D);           break;
    }

    if (strlen(img_name))
        nfutil_draw_image(g_vw_drawable, g_vw_gc, img_name, (gint)x, (gint)y, ICON_SIZE_W(VWND_DVABX_ICON), ICON_SIZE_H(VWND_DVABX_ICON), NFALIGN_LEFT, 0);
}

static gboolean vwnd_draw_itx_dvabx(VWND_T *p, VSM_ID_E wpos, gint cnt)
{
    gint x = 0, y = 0, w = 0, h = 0;
    ITX_DVABX_ICON icon = p->winfo[wpos].dvabx_itx_icon[cnt];
    VSM_DIV_E dtype = p->dtype;

    gint i;

    g_return_val_if_fail(icon >= ITX_DVABX_ICON_NONE, FALSE);
    g_return_val_if_fail(icon <= ITX_DVABX_ICON_NUM, FALSE);

    if (p->osd_off) return FALSE;
    if (icon == ITX_DVABX_ICON_NONE) return FALSE;
    if (_check_redraw_rect(&p->winfo[wpos].dvabx_area[cnt])) return FALSE;

    DMSG(1, "__VWND_DRAW_DVABX_ICON : %d", cnt);

    vwnd_get_position(dtype, wpos, &x, &y);
    vwnd_get_size(dtype, wpos, &w, &h);

    if ((dtype == VSM_DIV32) || (dtype==VSM_DIV36))
    {
        x += 25;
        y += h - ((ICON_SIZE_H(VWND_DVABX_ICON) + VWND_ICON_GAP_2) * 2);
    }
    else
    {
        x += ICON_SIZE_W(VWND_SEQ_ICON)/2;
        y += (VWND_TITLE_GAP_Y + FONT_H + VWND_ICON_GAP_2);
    }

    if ((dtype == VSM_DIV32) || (dtype==VSM_DIV36))
    {
        x += (cnt / 2) * (ICON_SIZE_W(VWND_DVABX_ICON) + VWND_ICON_GAP_2);
        y += (ICON_SIZE_H(VWND_DVABX_ICON) + VWND_ICON_GAP_2) * (cnt % 2);
    }
    else if ((dtype == VSM_DIV8) || (dtype==VSM_DIV16))
    {
        x += (cnt / 3) * (ICON_SIZE_W(VWND_DVABX_ICON) + VWND_ICON_GAP_2);
        y += (ICON_SIZE_H(VWND_DVABX_ICON) + VWND_ICON_GAP_2) * (cnt % 3);
    }
    else if ((dtype == VSM_DIV6) || (dtype==VSM_DIV9))
    {
        x += (cnt / 4) * (ICON_SIZE_W(VWND_DVABX_ICON) + VWND_ICON_GAP_2);
        y += (ICON_SIZE_H(VWND_DVABX_ICON) + VWND_ICON_GAP_2) * (cnt % 4);
    }
    else if (dtype != VSM_DIV1)
    {
        x += (cnt / 5) * (ICON_SIZE_W(VWND_DVABX_ICON) + VWND_ICON_GAP_2);
        y += (ICON_SIZE_H(VWND_DVABX_ICON) + VWND_ICON_GAP_2) * (cnt % 5);
    }
    else
    {
        y += (ICON_SIZE_H(VWND_DVABX_ICON) + VWND_ICON_GAP_2)*cnt;
    }

    p->winfo[wpos].dvabx_area[cnt].x = x;
    p->winfo[wpos].dvabx_area[cnt].y = y;
    p->winfo[wpos].dvabx_area[cnt].width = ICON_SIZE_W(VWND_DVABX_ICON);
    p->winfo[wpos].dvabx_area[cnt].height = ICON_SIZE_H(VWND_DVABX_ICON);

    _draw_itx_dvabx_icon(x, y, icon);

    return TRUE;
}

static gboolean vwnd_draw_dvabxline(VWND_T *p, VSM_ID_E wpos)
{
    VSM_DIV_E dtype = p->dtype;
    DVA_DP dp;
    guint x, y;
    gint width, height;
    DVA_CLON clon;

    if (!p->winfo[wpos].dvabxline) return FALSE;
    if (p->winfo[wpos].dvabx_updating) return FALSE;
    if (nf_live_fisheye_get_enable() == vsm_get_current_ch(wpos)) return FALSE;

    DMSG(1, "__VWND_DRAW_DVABX_RULE");

    dp.drawable = g_vw_drawable;
    dp.gc = g_vw_gc;
    dp.degree = 0;

    vwnd_get_position(dtype, wpos, &x, &y);
    vwnd_get_size(dtype, wpos, &width, &height);

    if (p->winfo[wpos].corridor_mode)
    {
        dp.plt_area.width = height * p->winfo[wpos].stream_ratio_h/p->winfo[wpos].stream_ratio_w;
        dp.plt_area.height = height;
        dp.plt_area.x = (width - dp.plt_area.width)/2;
        dp.plt_area.y = y;

        if (p->winfo[wpos].corridor_mode == 1) dp.degree = 90;
        else dp.degree = 270;
    }
    else
    {
        dp.plt_area.x = x;
        dp.plt_area.y = y;
        dp.plt_area.width = width;
        dp.plt_area.height = height;
    }

    clon.dic_cnt = p->winfo[wpos].dvabx_dic_cnt;
    clon.pdics = p->winfo[wpos].dvabx_pdics;
    vw_dit_display_dva_draw(&dp, clon);

    return TRUE;
}


static void _draw_legend_icon(gchar *img_name, gchar *text, gint x, gint y)
{
    gint img_w = 0, img_h = 0;
    gint string_h;
    GdkRectangle area;

    area.x = x;
    area.y = y;
    area.width = LEGEND_COL_WIDTH;
    area.height = 53;

    if (_check_redraw_rect(&area)) return;

    nfutil_draw_image(g_vw_drawable, g_vw_gc, img_name, x, y, -1, -1, NFALIGN_LEFT, 0);

    string_h = nfutil_string_height(g_vw_drawable, nffont_get_pango_font(NFFONT_LARGE_SEMI), text, NORMAL_SPACING);
    x += (53 + 10);
    y += (53 - string_h)/2;
    vwnd_draw_text(text, x, y, COLOR_IDX(308), COLOR_IDX(307), NFFONT_MEDIUM_SEMI, NORMAL_SPACING, TRUE);
}

static void _draw_legend(gint pos_x, gint pos_y)
{
    gchar *str[CNT_LEGEND] = {
                    "PRE REC",
                    "CONTINUOUS REC",
                    "AI/ALARM REC",
                    "MOTION REC",
                    "PANIC REC",
//                  "FREEZE",
                    "MOTION SENSING",
                    "SPEAKER",
#if !defined(_HDI_MODEL_UX) && !defined(_DVR_MODEL_UX)
                    "MIC"
#endif
                    };

    gchar *img[CNT_LEGEND] = {
                    IMG_PRE_REC_ICON,
                    IMG_TIMER_REC_ICON,
                    IMG_ALARM_REC_ICON,
                    IMG_MOTION_REC_ICON,
                    IMG_PANIC_REC_ICON,
//                  IMG_FREEZE_REC_ICON,
                    IMG_MOTION_DETECT_ICON,
                    IMG_SPEAKER_ICON,
#if !defined(_HDI_MODEL_UX) && !defined(_DVR_MODEL_UX)
                    IMG_MIC_ICON
#endif
                    };

    gint win_w, win_h;
    gint width, height;
    gint item_x = 0, item_y = 0;
    gint i;

    if (g_logo_info[VWND_LLOGO].pbuf)
    {
        vwnd_get_size(VSM_DIV9, VSM_WIN_ID9, &win_w, &win_h);
        nfui_get_pixbuf_size(g_logo_info[VWND_LLOGO].pbuf, &width, &height);

        pos_x += (win_w-width)/2;
        pos_y += (win_h-height)/2;

        nfutil_draw_pixbuf(g_vw_drawable, g_vw_gc, g_logo_info[VWND_LLOGO].pbuf, pos_x, pos_y, width, height, NFALIGN_LEFT, 0);
    }
    else
    {
        item_x = pos_x + 10;
        item_y = pos_y + 10;

        for(i = 0; i < CNT_LEGEND; i++)
        {
            if (!var_get_supported_audio()) 
            {
                if (i == LGD_SPEAKER_ICON) continue;
#if !defined(_HDI_MODEL_UX) && !defined(_DVR_MODEL_UX)
                if (i == LGD_MIC_ICON) continue;
#endif
            }
            _draw_legend_icon(img[i], str[i], item_x, item_y);

            item_y += 57;

            if (i == 4) {
                item_x = pos_x + 10 + LEGEND_COL_WIDTH + 4;
                item_y = pos_y + 10;
            }
        }
    }
}

static void vwnd_draw_empty_win(VWND_T *p)
{
    gint win_x = 0, win_y = 0;

    if (GUI_CHANNEL_CNT != 8) return;
    if (p->dtype != VSM_DIV9) return;
    if ((vsm_get_vmode() == VMODE_PB) || (vsm_get_vmode() == VMODE_NONE)) return;

    vwnd_get_position(VSM_DIV9, VSM_WIN_ID9, &win_x, &win_y);
    _draw_legend(win_x, win_y);
}

static void vwnd_draw_switch_snap(VWND_T *p)
{
    gint x = 0, y = 0;
    guint string_w = 0, string_h = 0;
    gchar titleBuf[VWND_TEXT_LEN+1];
    gint valid_cnt, tmp_w;

    if (g_switch.from_win == -1) return;
    if (p->switch_box.width == 0) return;
    if (p->switch_box.height == 0) return;
    if (_check_redraw_rect(&p->switch_box)) return FALSE;

    gdk_gc_set_rgb_fg_color(g_vw_gc, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_394E4A)));
    gdk_draw_rectangle(g_vw_drawable, g_vw_gc, TRUE, p->switch_box.x, p->switch_box.y, p->switch_box.width + 4, p->switch_box.height + 4);

    if (g_win_snap[g_switch.from_win])
    {
        gdk_draw_pixbuf(g_vw_drawable, g_vw_gc, g_win_snap[g_switch.from_win],
          0, 0,
          p->switch_box.x + ((p->switch_box.width - gdk_pixbuf_get_width(g_win_snap[g_switch.from_win])) / 2) + 2,
          p->switch_box.y + ((p->switch_box.height - gdk_pixbuf_get_height(g_win_snap[g_switch.from_win])) / 2) + 2,
          -1, -1, GDK_RGB_DITHER_NONE, 0, 0);
    }
    else
    {
        gdk_gc_set_rgb_fg_color(g_vw_gc, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_808080)));
        gdk_draw_rectangle(g_vw_drawable, g_vw_gc, TRUE, p->switch_box.x+2, p->switch_box.y+2, p->switch_box.width-4, p->switch_box.height-4);
    }

    memset(titleBuf, 0x00, sizeof(titleBuf));

    string_w = nfutil_string_width(0, g_vw_drawable, nffont_get_pango_font(NFFONT_LARGE_SEMI), p->winfo[g_switch.from_win].title_text, NORMAL_SPACING);

    if (string_w >= p->switch_box.width-16)
    {
        tmp_w = nfutil_string_width(0, g_vw_drawable, nffont_get_pango_font(NFFONT_LARGE_SEMI), "...", NORMAL_SPACING);
        valid_cnt = nfutil_string_get_valid_count(0, g_vw_drawable, nffont_get_pango_font(NFFONT_LARGE_SEMI), p->winfo[g_switch.from_win].title_text, p->switch_box.width-16-tmp_w);
        g_utf8_strncpy(titleBuf, p->winfo[g_switch.from_win].title_text, valid_cnt);
        strcat(titleBuf, "...");
    }
    else
        strcpy(titleBuf, p->winfo[g_switch.from_win].title_text);

    vwnd_draw_text(titleBuf, p->switch_box.x+16+2, p->switch_box.y+16+2,
        COLOR_IDX(308), COLOR_IDX(307), NFFONT_LARGE_SEMI, NORMAL_SPACING, FALSE);
}

static void vwnd_draw_video_image(VWND_T *p)
{
    gint x = 0, y = 0;
    guint string_w = 0, string_h = 0;
    gchar titleBuf[VWND_TEXT_LEN+1];
    gint valid_cnt, tmp_w;

    if (p->vimage_cf.box.width == 0) return;
    if (p->vimage_cf.box.height == 0) return;
    if (_check_redraw_rect(&p->vimage_cf.box)) return FALSE;

    gdk_gc_set_rgb_fg_color(g_vw_gc, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_394E4A)));
    gdk_draw_rectangle(g_vw_drawable, g_vw_gc, TRUE, p->vimage_cf.box.x, p->vimage_cf.box.y, p->vimage_cf.box.width, p->vimage_cf.box.height);

    if (p->vimage_cf.pbuf)
    {
        gdk_draw_pixbuf(g_vw_drawable, g_vw_gc, p->vimage_cf.pbuf,
                        0, 0,
                        p->vimage_cf.box.x + ((p->vimage_cf.box.width - gdk_pixbuf_get_width(p->vimage_cf.pbuf)) / 2) + 2,
                        p->vimage_cf.box.y + ((p->vimage_cf.box.height - gdk_pixbuf_get_height(p->vimage_cf.pbuf)) / 2) + 2,
                        -1, -1, GDK_RGB_DITHER_NONE, 0, 0);
    }
    else
    {
        gdk_gc_set_rgb_fg_color(g_vw_gc, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_808080)));
        gdk_draw_rectangle(g_vw_drawable, g_vw_gc, TRUE, p->vimage_cf.box.x+2, p->vimage_cf.box.y+2, p->vimage_cf.box.width-4, p->vimage_cf.box.height-4);
    }

    memset(titleBuf, 0x00, sizeof(titleBuf));

    string_w = nfutil_string_width(0, g_vw_drawable, nffont_get_pango_font(NFFONT_SMALL_SEMI_1), p->vimage_cf.title, NORMAL_SPACING);

    if (string_w >= p->vimage_cf.box.width-40)
    {
        tmp_w = nfutil_string_width(0, g_vw_drawable, nffont_get_pango_font(NFFONT_SMALL_SEMI_1), "...", NORMAL_SPACING);
        valid_cnt = nfutil_string_get_valid_count(0, g_vw_drawable, nffont_get_pango_font(NFFONT_SMALL_SEMI_1), p->vimage_cf.title, p->vimage_cf.box.width-40-tmp_w);
        g_utf8_strncpy(titleBuf, p->vimage_cf.title, valid_cnt);
        strcat(titleBuf, "...");
    }
    else
        strcpy(titleBuf, p->vimage_cf.title);

    vwnd_draw_text(titleBuf, p->vimage_cf.box.x+20, p->vimage_cf.box.y+20,
        COLOR_IDX(308), COLOR_IDX(307), NFFONT_SMALL_SEMI_1, NORMAL_SPACING, FALSE);
}

static void vwnd_draw_ptzarrow(VWND_T *p)
{
    if (p->arrow_ptcnt == 0) return;

    gdk_gc_set_rgb_fg_color(g_vw_gc, &UX_COLOR(COLOR_IDX(305)));
    gdk_gc_set_line_attributes(g_vw_gc, 1, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);
    gdk_draw_polygon(g_vw_drawable, g_vw_gc, TRUE, p->arrow_pt, p->arrow_ptcnt);
}

static void _set_postion_dbg_text(VWND_T *p, VSM_ID_E wpos)
{
    gint x = 0, y = 0, w = 0, h = 0;
    gint i;
    VSM_DIV_E dtype = p->dtype;

    vwnd_get_position(dtype, wpos, &x, &y);
    vwnd_get_size(dtype, wpos, &w, &h);

    x += 10;
    y += 10;
    w -= 20;
    h -= 20;

    for (i = 0; i < 5; i++)
    {
        p->winfo[wpos].debug_area.x = x;
        p->winfo[wpos].debug_area.y = y;
        p->winfo[wpos].debug_area.width = w;
        p->winfo[wpos].debug_area.height = h;

#ifdef DBG_DRAW_AREA
        vwnd_paint_area(x, y, w, h, COLOR_PRG_IDX(UX_COLOR_FF0000));
#endif
    }
}

static gboolean vwnd_draw_dbg_text(VWND_T *p, VSM_ID_E wpos)
{
    guint string_w = 0, string_h = 0;
    gint x = 0, y = 0, w = 0, h = 0;
    gint i;
    VSM_DIV_E dtype = p->dtype;

    if (_check_redraw_rect(&p->winfo[wpos].debug_area)) return FALSE;

    DMSG(1, "__VWND_DRAW_DBG_TEXT");

    vwnd_get_position(dtype, wpos, &x, &y);
    vwnd_get_size(dtype, wpos, &w, &h);

    x += 10;
    y += 10;
    string_w = w - 20;
    string_h = FONT_H;

    for (i = 0; i < VWND_DEBUG_LINE; i++)
    {
        vwnd_draw_text(p->winfo[wpos].debug_text[i], x, y, COLOR_IDX(308), COLOR_PRG_IDX(UX_COLOR_00FF00), NFFONT_LARGE_SEMI, SEMI_CONDENSED_SPACING, TRUE);
        y += (string_h+4);
    }

    return TRUE;
}

static void vwnd_hide_login_fail_button(VWND_T *p)
{
    gint i;

    for(i = 0; i < GUI_CHANNEL_CNT; i++) {

        if (p->winfo[i].login_fail) continue;

        if(nfui_nfobject_is_shown(g_chk_btn[i])) {

            nfui_nfobject_hide(g_chk_btn[i]);
            VirtualKey_Close();
            VW_CamPwd_Close(i);
        }
    }
}

static gboolean vwnd_switch_channel_step(guchar step)
{
    static guchar ex_step = SWITCH_STEP_NONE;

    switch(step) {
        case SWITCH_STEP_NONE:
            break;

        case SWITCH_STEP_PRESS:
            {
                if(ex_step == SWITCH_STEP_NONE) {
                    ex_step |= SWITCH_STEP_PRESS;
                    return TRUE;
                }
            }
            break;

        case SWITCH_STEP_MOVE:
            {
                if(ex_step == SWITCH_STEP_PRESS) {
                    ex_step |= SWITCH_STEP_MOVE;
                    return TRUE;
                }
            }
            break;

        case SWITCH_STEP_RELEASE:
            {
                if((ex_step == SWITCH_STEP_PRESS + SWITCH_STEP_MOVE)) {
                    ex_step = SWITCH_STEP_NONE;
                    return TRUE;
                }else {
                    ex_step = SWITCH_STEP_NONE;
                }
            }
            break;
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

static gint _is_valid_win(gint mx, gint my)
{
    gint valid = 1;
    gint win_id = 0;

#if defined(GUI_8CH_SUPPORT)
    win_id = vwnd_get_event_position(pvwnd->dtype, mx, my);

    if (pvwnd->dtype == VSM_DIV9)
    {
        if (win_id == 8) valid = 0;
        }
#endif

#if defined(GUI_32CH_SUPPORT)
    win_id = vwnd_get_event_position(pvwnd->dtype, mx, my);

    if (pvwnd->dtype == VSM_DIV36)
    {
        if (win_id >= 32) valid = 0;
    }
#endif

    return valid;
}

static gint _check_draw_focus_win(gint mx, gint my)
{
    VSM_ID_E pos;

    pos = vwnd_get_event_position(pvwnd->dtype, mx, my);

    if (pos < GUI_CHANNEL_CNT) {
        vsm_draw_focus_win(pos);
    }

    return 0;
}

static gint _check_draw_fisheye_view(gint mx, gint my)
{
    gint pos;

    pos = vwnd_get_fisheye_view_index(pvwnd->dtype, mx, my);
    if (pos == -1) return -1;

    if (pos < GUI_CHANNEL_CNT) {
        //vsm_draw_focus_win(pos);
    }

    return 0;
}

static gint _check_va_select_icon(gint mx, gint my)
{
    VSM_ID_E pos;
    IGRECT rect;
    gint i;

    if (pvwnd->dtype == VSM_DIV1)
    {
        pos = vwnd_get_event_position(pvwnd->dtype, mx, my);

        for (i = 0; i < VWND_VCA_MAX_CNT; i++)
        {
            rect.x = pvwnd->winfo[pos].vca_area[i].x;
            rect.y = pvwnd->winfo[pos].vca_area[i].y;
            rect.w = ICON_SIZE_W(VWND_VCA_ICON);
            rect.h = ICON_SIZE_H(VWND_VCA_ICON);

            if (ifn_is_in_igrect(&rect, (gint)mx, (gint)my))
            {
                vvm_set_vca_select_icon(pos, i);
                vvm_set_dvabx_select_icon(pos, i);
            }
        }
    }

    return 0;
}

static gint _proc_press_switch_win(gint mx, gint my)
{
    CamItxFisheyeData fisheye_data;
    VSM_ID_E pos;
    gint ch;

    if (pvwnd->dtype == VSM_DIV1) return -1;
    if (!_is_valid_win(mx, my)) return -1;

#if defined(GUI_8CH_SUPPORT)
    if (pvwnd->dtype == VSM_DIV9)
    {
        if ((mx >= pvwnd->plt_w/3*2)&&(my >= pvwnd->plt_h/3*2)) return -1;
    }
#endif

    pos = vwnd_get_event_position(pvwnd->dtype, mx, my);
    ch = vsm_get_current_ch(pos);

    if (nf_live_fisheye_is_support(ch))
    {
        memset(&fisheye_data, 0x00, sizeof(CamItxFisheyeData));
        DAL_get_camera_itx_fisheye_data(&fisheye_data, ch);
        if (fisheye_data.act) return -1;
    }

    g_switch.from_x = mx;
    g_switch.from_y = my;
    g_switch.from_win = vwnd_get_event_position(pvwnd->dtype, mx, my);
    g_switch.to_x = -1;
    g_switch.to_y = -1;
    g_switch.to_win = -1;

    g_switch.pressed = 1;
    g_switch.moved = 0;

    evt_send_to_local(INFY_VWND_SWITCH_PRESS, 0, 0, 0);

    return 0;
}

static gint _proc_move_switch_win(gint mx, gint my)
{
    gint i, win;

    if (g_switch.pressed == 0) return -1;

    if (g_switch.moved == 0)
    {
        if ((abs(mx - g_switch.from_x) < 5) && (abs(my - g_switch.from_y) < 5)) return -1;

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            nfui_nfobject_disable(g_chk_btn[i]);
            nfui_signal_emit(g_chk_btn[i], GDK_EXPOSE, TRUE);
        }

        for (win = 0; win < VSM_WIN_MAX; win++)
        {
            if (pvwnd->winfo[win].is_focus)
            {
                pvwnd->winfo[win].is_focus = FALSE;
            }
        }

        g_switch.moved = 1;
        //g_switch.pressed = 0;
        vwnd_draw_focus_move_from(pvwnd, g_switch.from_win);
        nftool_change_custom_cursor(GTK_WIDGET(((NFWINDOW*)g_vwnd)->main_widget)->window, NF_CURSOR_FLEUR);
    }

    // for debounce
    if ((abs(g_switch.to_x-mx) < 10) && (abs(g_switch.to_y-my) < 10)) return FALSE;

    if (g_switch.to_win != vwnd_get_event_position(pvwnd->dtype, mx, my))
    {
        if (g_switch.to_win != -1)
        {
            vwnd_erase_focus_box(pvwnd, g_switch.to_win);
            vwnd_draw_border(pvwnd);
            vwnd_draw_focus_move_from(pvwnd, g_switch.from_win);

            g_switch.to_win = g_switch.from_win;
        }

        if (_is_valid_win(mx, my))
        {
            g_switch.to_win = vwnd_get_event_position(pvwnd->dtype, mx, my);
        }

        vwnd_draw_focus_move_to(pvwnd, g_switch.to_win);
    }

    g_switch.to_x = mx;
    g_switch.to_y = my;

    evt_send_to_local(INFY_VWND_SWITCH_MOVE, 0, 0, 0);

    return 0;
}

static gint _proc_release_switch_win()
{
    gint i;

    if (g_switch.pressed == 0) return -1;

    if (g_switch.moved == 1)
    {
        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            nfui_nfobject_enable(g_chk_btn[i]);
            nfui_signal_emit(g_chk_btn[i], GDK_EXPOSE, TRUE);
        }

        vwnd_erase_focus_box(pvwnd, g_switch.from_win);

        if (g_switch.from_win != g_switch.to_win)
        {
            vwnd_erase_focus_box(pvwnd, g_switch.to_win);
            vwnd_draw_border(pvwnd);
            vsm_switch_channel(vsm_get_current_ch(g_switch.from_win), vsm_get_current_ch(g_switch.to_win));
        }
        else if (g_switch.to_win != -1)
        {
            vwnd_draw_border(pvwnd);
            pvwnd->winfo[g_switch.to_win].is_focus = TRUE;
            vwnd_draw_focus_box(pvwnd, g_switch.to_win);
        }
        else
        {
            vwnd_draw_border(pvwnd);
            pvwnd->winfo[g_switch.from_win].is_focus = TRUE;
            vwnd_draw_focus_box(pvwnd, g_switch.from_win);
        }

        nftool_change_custom_cursor(GTK_WIDGET(((NFWINDOW*)g_vwnd)->main_widget)->window, NF_CURSOR_ARROW);
    }

    g_switch.from_x = -1;
    g_switch.from_y = -1;
    g_switch.from_win = -1;
    g_switch.to_x = -1;
    g_switch.to_y = -1;
    g_switch.to_win = -1;
    g_switch.pressed = 0;
    g_switch.moved = 0;

    evt_send_to_local(INFY_VWND_SWITCH_RELEASE, 0, 0, 0);

    return 0;
}

static gint _proc_cancel_switch_win()
{
    gint i;

    if (g_switch.pressed == 0) return -1;

    if (g_switch.moved == 1)
    {
        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            nfui_nfobject_enable(g_chk_btn[i]);
            nfui_signal_emit(g_chk_btn[i], GDK_EXPOSE, TRUE);
        }

        vwnd_erase_focus_box(pvwnd, g_switch.from_win);
        vwnd_erase_focus_box(pvwnd, g_switch.to_win);
        vwnd_draw_border(pvwnd);

        pvwnd->winfo[g_switch.from_win].is_focus = TRUE;
        vwnd_draw_focus_box(pvwnd, g_switch.from_win);

        nftool_change_custom_cursor(GTK_WIDGET(((NFWINDOW*)g_vwnd)->main_widget)->window, NF_CURSOR_ARROW);
    }

    g_switch.from_x = -1;
    g_switch.from_y = -1;
    g_switch.from_win = -1;
    g_switch.to_x = -1;
    g_switch.to_y = -1;
    g_switch.to_win = -1;
    g_switch.pressed = 0;
    g_switch.moved = 0;

    evt_send_to_local(INFY_VWND_SWITCH_RELEASE, 0, 0, 0);

    return 0;
}

static gint _proc_press_fisheye_view(gint mx, gint my)
{
    CamItxFisheyeData fisheye_data;
    VSM_ID_E pos;

    gint ch;

    if (!_is_valid_win(mx, my)) return -1;

#if defined(GUI_8CH_SUPPORT)
    if (pvwnd->dtype == VSM_DIV9)
    {
        if ((mx >= pvwnd->plt_w/3*2)&&(my >= pvwnd->plt_h/3*2)) return -1;
    }
#endif

    pos = vwnd_get_event_position(pvwnd->dtype, mx, my);
    ch = vsm_get_current_ch(pos);

    if (!nf_live_fisheye_is_support(ch)) return -1;

	memset(&fisheye_data, 0x00, sizeof(CamItxFisheyeData));
	DAL_get_camera_itx_fisheye_data(&fisheye_data, ch);
	if (!fisheye_data.act) return -1;

    //pos = vwnd_get_event_position(pvwnd->dtype, (gint)bevent->x, (gint)bevent->y);

    g_fisheye.from_x = mx;
    g_fisheye.from_y = my;
    g_fisheye.to_x = -1;
    g_fisheye.to_y = -1;

    g_fisheye.pressed = 1;
    g_fisheye.moved = 0;

    g_fisheye.wpos = vwnd_get_event_position(pvwnd->dtype, mx, my);
    g_fisheye.view_idx = vwnd_get_fisheye_view_index(pvwnd->dtype, mx, my);

    return 0;
}

static gint _proc_move_fisheye_view(gint mx, gint my)
{
    NF_FISHEYE_VIDEO_PARAM vparam;
    NF_FISHEYE_PTZ_PARAM pparam;
    NF_FISHEYE_PTZ_LIMIT plimit;
    gint ch;

    if (g_fisheye.pressed == 0) return -1;

    if (g_fisheye.moved == 0)
    {
        if ((abs(mx - g_fisheye.from_x) < 5) && (abs(my - g_fisheye.from_y) < 5)) return -1;

        g_fisheye.moved = 1;
        nftool_change_custom_cursor(GTK_WIDGET(((NFWINDOW*)g_vwnd)->main_widget)->window, NF_CURSOR_FLEUR);
    }

    if ((abs(mx-g_fisheye.from_x) < 2) && (abs(my-g_fisheye.from_y) < 2)) return FALSE; 

    ch = vsm_get_current_ch(g_fisheye.wpos);

    if (!nf_live_fisheye_is_support(ch)) return -1;

    memset(&pparam, 0x00, sizeof(NF_FISHEYE_PTZ_PARAM));
    nf_live_fisheye_get_ptz_param(ch, &pparam);
    //g_message("%s, %d, max_view:%d", __FUNCTION__, __LINE__, pparam.max_view);
    //g_message("%s, %d, pan:%f, tilt:%f, zoom:%f, roll:%f", __FUNCTION__, __LINE__, pparam.view[0].pan, pparam.view[0].tilt, pparam.view[0].zoom, pparam.view[0].roll);

    pparam.view[g_fisheye.view_idx].pan += (float)(mx-g_fisheye.from_x)*(-0.03f);
    pparam.view[g_fisheye.view_idx].tilt += (float)(my-g_fisheye.from_y)*(0.03f); 

    //g_message("%s, %d, view_idx:%d, pan:%f, tilt:%f", __FUNCTION__, __LINE__, g_fisheye.view_idx, pparam.view[0].pan, pparam.view[0].tilt);   

    nf_live_fisheye_get_ptz_limit(ch, &plimit);
    if (pparam.view[g_fisheye.view_idx].pan < plimit.pan_min) pparam.view[g_fisheye.view_idx].pan = plimit.pan_max;
    if (pparam.view[g_fisheye.view_idx].pan > plimit.pan_max) pparam.view[g_fisheye.view_idx].pan = plimit.pan_min;    
    if (pparam.view[g_fisheye.view_idx].tilt < plimit.tilt_min) pparam.view[g_fisheye.view_idx].tilt = plimit.tilt_min;
    if (pparam.view[g_fisheye.view_idx].tilt > plimit.tilt_max) pparam.view[g_fisheye.view_idx].tilt = plimit.tilt_max;    

    nf_live_fisheye_set_ptz_param(ch, &pparam);  
    feye_set_ptz_param_data(ch, &pparam);

    g_fisheye.from_x = mx;
    g_fisheye.from_y = my;

    return 0;
}

static gint _proc_zoom_fisheye_view(gint mx, gint my, gint inout)
{
    NF_FISHEYE_PTZ_PARAM pparam;
    NF_FISHEYE_PTZ_LIMIT plimit;
    gint wpos, vpos, ch;

    if (!_is_valid_win(mx, my)) return -1;

#if defined(GUI_8CH_SUPPORT)
    if (pvwnd->dtype == VSM_DIV9)
    {
        if ((mx >= pvwnd->plt_w/3*2)&&(my >= pvwnd->plt_h/3*2)) return -1;
    }
#endif

    wpos = vwnd_get_event_position(pvwnd->dtype, mx, my);
    ch = vsm_get_current_ch(wpos);

    if (!nf_live_fisheye_is_support(ch)) return -1;

    vpos = vwnd_get_fisheye_view_index(pvwnd->dtype, mx, my);

    memset(&pparam, 0x00, sizeof(NF_FISHEYE_PTZ_PARAM));
    nf_live_fisheye_get_ptz_param(ch, &pparam);
    //g_message("%s, %d, max_view:%d", __FUNCTION__, __LINE__, pparam.max_view);
    //g_message("%s, %d, pan:%f, tilt:%f, zoom:%f, roll:%f", __FUNCTION__, __LINE__, pparam.view[1].pan, pparam.view[1].tilt, pparam.view[1].zoom, pparam.view[1].roll);

    if (inout == 0) pparam.view[vpos].zoom -= (float)pparam.view[vpos].zoom*0.02f;
    else pparam.view[vpos].zoom += (float)pparam.view[vpos].zoom*0.02f;

    nf_live_fisheye_get_ptz_limit(ch, &plimit);
    if (pparam.view[vpos].zoom < plimit.zoom_min) pparam.view[vpos].zoom = plimit.zoom_min;
    if (pparam.view[vpos].zoom > plimit.zoom_max) pparam.view[vpos].zoom = plimit.zoom_max;

    nf_live_fisheye_set_ptz_param(ch, &pparam);  
    feye_set_ptz_param_data(ch, &pparam);
    return 0;
}

static gint _proc_release_fisheye_view()
{
    gint i;

    if (g_fisheye.pressed == 0) return -1;

    if (g_fisheye.moved == 1) {
        nftool_change_custom_cursor(GTK_WIDGET(((NFWINDOW*)g_vwnd)->main_widget)->window, NF_CURSOR_ARROW);
    }

    g_fisheye.from_x = -1;
    g_fisheye.from_y = -1;
    g_fisheye.to_x = -1;
    g_fisheye.to_y = -1;
    g_fisheye.pressed = 0;
    g_fisheye.moved = 0;

    g_fisheye.wpos = -1;
    g_fisheye.view_idx = -1;

    return 0;
}

static gint _proc_cancel_fisheye_view()
{
    gint i;

    if (g_fisheye.pressed == 0) return -1;

    if (g_fisheye.moved == 1) {
        nftool_change_custom_cursor(GTK_WIDGET(((NFWINDOW*)g_vwnd)->main_widget)->window, NF_CURSOR_ARROW);
    }

    g_fisheye.from_x = -1;
    g_fisheye.from_y = -1;
    g_fisheye.to_x = -1;
    g_fisheye.to_y = -1;
    g_fisheye.pressed = 0;
    g_fisheye.moved = 0;

    g_fisheye.wpos = -1;
    g_fisheye.view_idx = -1;

    return 0;
}

static void _set_item_position(VWND_T *p)
{
    guint win;
    gint i;
#if defined(GUI_8CH_SUPPORT)
	const unsigned int nr_channel[] = {1, 4, 6, 8, 8, 16, 32, 36};
#elif defined(GUI_32CH_SUPPORT)
	const unsigned int nr_channel[] = {1, 4, 6, 8, 9, 16, 32, 32};
#else
	const unsigned int nr_channel[] = {1, 4, 6, 8, 9, 16, 32, 36};
#endif

#ifndef DBG_TEXT_ENABLE
    for (win = 0; win < nr_channel[p->dtype]; win++)
    {
        _set_postion_video_status(p, win);
        _set_postion_alarm_text(p, win);
        _set_postion_rec_icon(p, win);
        _set_postion_speaker_icon(p, win);
        _set_postion_mic_icon(p, win);
        _set_postion_motion_icon(p, win);
        _set_postion_freeze_icon(p, win);
        _set_postion_pnd_wait(p, win);
        _set_postion_pnd_login_fail(p, win);
        _set_postion_connecting(p, win);
        _set_postion_cam_title(p, win);
        _set_postion_pos_text(p, win);

        for (i = 0; i < 3; i++)
        {
            _set_position_dlva_counter(p, win, i);
        }  

        for (i = 0; i < VWND_VCA_MAX_CNT; i++)
        {
            _set_postion_itx_vca(p, win, i);
            _set_postion_s1_vca(p, win, i);
        }

        for (i = 0; i < VWND_DVABX_MAX_CNT; i++)
        {
            _set_postion_itx_dvabx(p, win, i);
        }        
    }

    _set_postion_seq_icon(p);
    _set_postion_user_text(p);
    _set_postion_time(p);
    _set_postion_play_status(p);
    _set_postion_alarm_status(p);
    _set_postion_network_status(p);
    _set_postion_disk_status(p);
    _set_postion_arch_status(p);
    _set_postion_clondev_text(p);
#else
    for (win = 0; win < nr_channel[p->dtype]; win++)
    {
        _set_postion_video_status(p, win);
        _set_postion_rec_icon(p, win);
        _set_postion_pnd_wait(p, win);
        _set_postion_pnd_login_fail(p, win);
        _set_postion_connecting(p, win);
        _set_postion_dbg_text(p, win);
    }

    _set_postion_time(p);
#endif
}

static void vwnd_redraw_display(VWND_T *p)
{
    guint win;
    gint i;
    OsdData osddata;

    DAL_get_osd_data(&osddata);

#if defined(GUI_8CH_SUPPORT)
	const unsigned int nr_channel[] = {1, 4, 6, 8, 8, 16, 32, 36};
#elif defined(GUI_32CH_SUPPORT)
	const unsigned int nr_channel[] = {1, 4, 6, 8, 9, 16, 32, 32};
#else
	const unsigned int nr_channel[] = {1, 4, 6, 8, 9, 16, 32, 36};
#endif

    g_return_if_fail(p != NULL);

    if (p->fill == FILL_OFF) return;

#ifndef DBG_TEXT_ENABLE
    vwnd_draw_border(p);

    for (win = 0; win < nr_channel[p->dtype]; win++)
    {
        DMSG(1, "__VWND_WIN_ID_%d", win);
        vwnd_draw_focus_box(p, win);
        vwnd_draw_focus_move_from(p, win);
        vwnd_draw_focus_move_to(p, win);
        vwnd_draw_vcaline(p, win);
        vwnd_draw_dvabxline(p, win);
        vwnd_draw_video_status(p, win);
        vwnd_draw_alarm_text(p, win);
        vwnd_draw_rec_icon(p, win);
        vwnd_draw_speaker_icon(p, win);
        vwnd_draw_mic_icon(p, win);
        vwnd_draw_motion_icon(p, win);
        vwnd_draw_freeze_icon(p, win);
        vwnd_draw_pnd_wait(p, win);
        vwnd_draw_login_fail(p, win);
        vwnd_draw_connecting_text(p, win);
        vwnd_draw_cam_title(p, win);

        for (i = 0; i < p->winfo[win].pos_row; i++)
        {
            vwnd_draw_pos_text(p, win, i);
        }

        for (i = 0; i < 3; i++)
        {
            vwnd_draw_dlva_counter(p, win, i);
        }        

        for (i = 0; i < VWND_VCA_MAX_CNT; i++)
        {
            vwnd_draw_itx_vca(p, win, i);
            vwnd_draw_s1_vca(p, win, i);
        }

        for (i = 0; i < VWND_DVABX_MAX_CNT; i++)
        {
            vwnd_draw_itx_dvabx(p, win, i);
        }        
    }

    if(osddata.sideicon == TRUE)    vwnd_draw_empty_win(p);
    vwnd_draw_seq_icon(p);
    vwnd_draw_user_text(p);
    vwnd_draw_time(p);
    vwnd_draw_play_status(p);
    if(osddata.statusicon == TRUE)
    {
    vwnd_draw_alarm_status(p);
    vwnd_draw_network_status(p);
    vwnd_draw_disk_status(p);
    }
    vwnd_draw_arch_status(p);
    vwnd_draw_clondev_text(p);
    vwnd_draw_ptzarrow(p);
    vwnd_draw_switch_snap(p);
    vwnd_draw_video_image(p);
#else
    vwnd_draw_border(p);

    for (win = 0; win < nr_channel[p->dtype]; win++)
    {
        DMSG(1, "__VWND_WIN_ID_%d", win);
        vwnd_draw_focus_box(p, win);
        vwnd_draw_video_status(p, win);
        vwnd_draw_rec_icon(p, win);
        vwnd_draw_pnd_wait(p, win);
        vwnd_draw_login_fail(p, win);
        vwnd_draw_connecting_text(p, win);
        vwnd_draw_dbg_text(p, win);
    }

    if(osddata.sideicon == TRUE)    vwnd_draw_empty_win(p);
    vwnd_draw_time(p);
#endif
}


static gboolean vw_fixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
    if (event->type == GDK_EXPOSE)
    {
        if(g_vw_drawable == NULL)
            g_vw_drawable = nfui_nfobject_get_window(g_vwnd);

        if(g_vw_gc == NULL) {
            if(g_vw_drawable != NULL)
                g_vw_gc = gdk_gc_new(g_vw_drawable);
        }

#ifdef DBG_DRAW_AREA
        _set_item_position(pvwnd);
#endif

        vwnd_hide_login_fail_button(pvwnd);

        _set_redraw_region((GdkEventExpose *)event);
        vwnd_redraw_display(pvwnd);
    }
    else if (event->type == GDK_DELETE)
    {
        if(g_vw_gc != NULL)
            g_object_unref(g_vw_gc);

        uxm_unreg_imsg_event(obj, INFY_VWND_RUN_ALL_EVENT);
        uxm_unreg_imsg_event(obj, INFY_VWND_STOP_ALL_EVENT);
        uxm_unreg_imsg_event(obj, INFY_VWND_RUN_RIGHT_PRESS);
        uxm_unreg_imsg_event(obj, INFY_VWND_STOP_RIGHT_PRESS);
        uxm_unreg_imsg_event(obj, INFY_VWND_CHANGE_BORDER);
        uxm_unreg_imsg_event(obj, INFY_VWND_DRAW_FOCUS);
        uxm_unreg_imsg_event(obj, INFY_VWND_ERASE_FOCUS);
        uxm_unreg_imsg_event(obj, INFY_VWND_DRAW_NEW_VCA);
        uxm_unreg_imsg_event(obj, INFY_VWND_DRAW_NEW_DVABX);
        uxm_unreg_imsg_event(obj, INFY_VWND_SWITCH_PRESS);
        uxm_unreg_imsg_event(obj, INFY_VWND_SWITCH_MOVE);
        uxm_unreg_imsg_event(obj, INFY_VWND_SWITCH_RELEASE);
        uxm_unreg_imsg_event(obj, INFY_VWND_SWITCH_CANCEL);
        uxm_unreg_imsg_event(obj, INFY_VWND_DRAW_PTZARROW);
        uxm_unreg_imsg_event(obj, INFY_VWND_ERASE_PTZARROW);
        uxm_unreg_imsg_event(obj, INFY_VWND_DRAW_VIDEO_IMAGE);
        uxm_unreg_imsg_event(obj, INFY_VWND_ERASE_VIDEO_IMAGE);

        uxm_unreg_imsg_event(obj, INFY_PACKET_DUMP_INSERT_MEDIA);

        nfui_page_close(PGID_LIVEDISPLAY, g_vwnd);
    }
    else if (event->type == INFY_VWND_RUN_ALL_EVENT)
    {
        user_event = TRUE;
    }
    else if (event->type == INFY_VWND_STOP_ALL_EVENT)
    {
        user_event = FALSE;
    }
    else if (event->type == INFY_VWND_RUN_RIGHT_PRESS)
    {
        right_press_event = TRUE;
    }
    else if (event->type == INFY_VWND_STOP_RIGHT_PRESS)
    {
        right_press_event = FALSE;
    }
    else if (event->type == INFY_VWND_CHANGE_BORDER)
    {
        vwnd_erase_border(pvwnd);
        vwnd_draw_border(pvwnd);
    }
    else if (event->type == INFY_VWND_DRAW_FOCUS)
    {
        gchar win = ((CMM_MESSAGE_T *)data)->param;
        vwnd_draw_focus_box(pvwnd, win);
    }
    else if (event->type == INFY_VWND_ERASE_FOCUS)
    {
        gchar win = ((CMM_MESSAGE_T *)data)->param;
        vwnd_erase_focus_box(pvwnd, win);
        vwnd_draw_border(pvwnd);
    }
    else if (event->type == INFY_VWND_SWITCH_PRESS)
    {
        guint covert_mask = 0;

        covert_mask = ssm_get_covert_mask();

        if (ivsc.dfunc.support_advanced_chswitch == 0) return FALSE;
        if (g_switch.from_win == -1) return FALSE;
        if (covert_mask & (1 << vsm_get_current_ch(g_switch.from_win))) return FALSE;

        if (g_win_snap[g_switch.from_win] == 0)
        {
            if (vsm_get_vmode() == VMODE_LV)
            {
                NF_NOTIFY_INFO buf;
                scm_get_vloss_data(&buf);
                if (buf.d.params[0] & (1 << vsm_get_current_ch(g_switch.from_win))) return FALSE;

                scm_req_live_capture_without_time(INFY_CAMSWITCH_CAPTURE_IMAGE, vsm_get_current_ch(g_switch.from_win));
            }
            else
            {
                scm_req_playback_capture_without_time(INFY_CAMSWITCH_CAPTURE_IMAGE, vsm_get_current_ch(g_switch.from_win));
            }
            uxm_reg_imsg_event(obj, INFY_CAMSWITCH_CAPTURE_IMAGE);
        }
    }
    else if (event->type == INFY_VWND_SWITCH_MOVE)
    {
        GdkRectangle tmp;
        GdkRegion *region = 0;

        if (ivsc.dfunc.support_advanced_chswitch == 0) return FALSE;
        if (g_switch.from_win == -1) return FALSE;

        if ((pvwnd->switch_box.width != 0)
&& (pvwnd->switch_box.height != 0))
        {
            tmp.x = pvwnd->switch_box.x - 4;
            tmp.y = pvwnd->switch_box.y - 4;
            tmp.width = pvwnd->switch_box.width + 8;
            tmp.height = pvwnd->switch_box.height + 8;
            region = gdk_region_rectangle(&tmp);
        }

        if (((pvwnd->dtype == VSM_DIV6) || (pvwnd->dtype == VSM_DIV8)) && (g_switch.from_win == VSM_WIN_ID1))
        {
            vwnd_get_size(pvwnd->dtype, VSM_WIN_ID2, &pvwnd->switch_box.width, &pvwnd->switch_box.height);

            pvwnd->switch_box.x = g_switch.to_x - pvwnd->switch_box.width/2;
            pvwnd->switch_box.y = g_switch.to_y - pvwnd->switch_box.height/2;
            pvwnd->switch_box.width = pvwnd->switch_box.width;
            pvwnd->switch_box.height = pvwnd->switch_box.height;
        }
        else
        {
            vwnd_get_position(pvwnd->dtype, g_switch.from_win, &pvwnd->switch_box.x, &pvwnd->switch_box.y);
            vwnd_get_size(pvwnd->dtype, g_switch.from_win, &pvwnd->switch_box.width, &pvwnd->switch_box.height);

            pvwnd->switch_box.x -= g_switch.from_x;
            pvwnd->switch_box.x += g_switch.to_x;
            pvwnd->switch_box.y -= g_switch.from_y;
            pvwnd->switch_box.y += g_switch.to_y;
        }

        pvwnd->switch_box.x -= 2;
        pvwnd->switch_box.y -= 2;
        pvwnd->switch_box.width += 4;
        pvwnd->switch_box.height += 4;

        tmp.x = pvwnd->switch_box.x - 4;
        tmp.y = pvwnd->switch_box.y - 4;
        tmp.width = pvwnd->switch_box.width + 8;
        tmp.height = pvwnd->switch_box.height + 8;

        if (region) {
            gdk_region_union_with_rect(region, &tmp);
        }
        else {
            region = gdk_region_rectangle(&tmp);
        }

        gdk_window_invalidate_region(GTK_WIDGET(((NFWINDOW*)g_vwnd)->main_widget)->window, region, TRUE);
        gdk_region_destroy(region);
    }
    else if (event->type == INFY_VWND_SWITCH_RELEASE)
    {
        GdkRectangle tmp;
        GdkRegion *region;
        gint i;

        if (ivsc.dfunc.support_advanced_chswitch == 0) return FALSE;

        for (i = 0; i < VSM_WIN_MAX; i++)
        {
            if (g_win_snap[i])
            {
                g_object_unref(g_win_snap[i]);
                g_win_snap[i] = 0;
            }
        }

        if (pvwnd->switch_box.width == 0) return FALSE;
        if (pvwnd->switch_box.height == 0) return FALSE;

        tmp.x = pvwnd->switch_box.x - 4;
        tmp.y = pvwnd->switch_box.y - 4;
        tmp.width = pvwnd->switch_box.width + 8;
        tmp.height = pvwnd->switch_box.height + 8;
        region = gdk_region_rectangle(&tmp);
        gdk_window_invalidate_region(GTK_WIDGET(((NFWINDOW*)g_vwnd)->main_widget)->window, region, TRUE);
        gdk_region_destroy(region);

        memset(&pvwnd->switch_box, 0x00, sizeof(GdkRectangle));
    }
    else if (event->type == INFY_VWND_SWITCH_CANCEL)
    {
        _proc_cancel_switch_win();
    }    
    else if (event->type == INFY_VWND_DRAW_NEW_VCA)
    {
        gchar wpos = ((CMM_MESSAGE_T *)data)->param;
        VSM_DIV_E dtype = pvwnd->dtype;

        VCA_DP dp;
        VCA_CLON preClon;
        VCA_CLON postClon;

        GdkRegion *pre_region;
        GdkRegion *post_region;

        dp.drawable = g_vw_drawable;
        dp.gc = g_vw_gc;
        vwnd_get_position(dtype, wpos, &dp.plt_area.x, &dp.plt_area.y);
        vwnd_get_size(dtype, wpos, &dp.plt_area.width, &dp.plt_area.height);

        preClon.dic_cnt = pvwnd->winfo[wpos].vca_dic_cnt;
        preClon.pdics = pvwnd->winfo[wpos].vca_pdics;
        postClon.dic_cnt = 0;
        postClon.pdics = 0;
        vw_dit_display_get_vca_diclist(&postClon);

        if (vw_dit_display_compare_vca_diclist(preClon, postClon) == 1) {
            vw_dit_display_free_vca_diclist(preClon);
            pvwnd->winfo[wpos].vca_dic_cnt = postClon.dic_cnt;
            pvwnd->winfo[wpos].vca_pdics = postClon.pdics;
            return FALSE;
        }

        pvwnd->winfo[wpos].vca_dic_cnt = postClon.dic_cnt;
        pvwnd->winfo[wpos].vca_pdics = postClon.pdics;

        pre_region = gdk_region_new();
        post_region = gdk_region_new();

        vw_dit_display_get_vca_region(&dp, preClon, pre_region);
        vw_dit_display_get_vca_region(&dp, postClon, post_region);

        gdk_region_union(post_region, pre_region);
        if (gdk_region_empty(post_region)) {
            gdk_region_union_with_rect(post_region, &dp.plt_area);
        }

        gdk_window_invalidate_region(GTK_WIDGET(((NFWINDOW*)g_vwnd)->main_widget)->window, post_region, TRUE);
        gdk_region_destroy(pre_region);
        gdk_region_destroy(post_region);

        vw_dit_display_free_vca_diclist(preClon);
    }
    else if (event->type == INFY_VWND_DRAW_NEW_DVABX)
    {
        gchar wpos = ((CMM_MESSAGE_T *)data)->param;
        VSM_DIV_E dtype = pvwnd->dtype;

        DVA_DP dp;
        DVA_CLON preClon;
        DVA_CLON postClon;

        GdkRegion *pre_region;
        GdkRegion *post_region;

        dp.drawable = g_vw_drawable;
        dp.gc = g_vw_gc;
        dp.degree = 0;

        if (pvwnd->winfo[wpos].corridor_mode)
        {
            gint x, y, width, height;

            vwnd_get_position(dtype, wpos, &x, &y);
            vwnd_get_size(dtype, wpos, &width, &height);

            dp.plt_area.width = height * pvwnd->winfo[wpos].stream_ratio_h/pvwnd->winfo[wpos].stream_ratio_w;
            dp.plt_area.height = height;
            dp.plt_area.x = (width - dp.plt_area.width)/2;
            dp.plt_area.y = y;

            if (pvwnd->winfo[wpos].corridor_mode == 1) dp.degree = 90;
            else dp.degree = 270;
        }
        else
        {
            vwnd_get_position(dtype, wpos, &dp.plt_area.x, &dp.plt_area.y);
            vwnd_get_size(dtype, wpos, &dp.plt_area.width, &dp.plt_area.height);
        }

        preClon.dic_cnt = pvwnd->winfo[wpos].dvabx_dic_cnt;
        preClon.pdics = pvwnd->winfo[wpos].dvabx_pdics;
        postClon.dic_cnt = 0;
        postClon.pdics = 0;
        vw_dit_display_get_dva_diclist(&postClon);

        if (vw_dit_display_compare_dva_diclist(preClon, postClon) == 1) {
            vw_dit_display_free_dva_diclist(preClon);
            pvwnd->winfo[wpos].dvabx_dic_cnt = postClon.dic_cnt;
            pvwnd->winfo[wpos].dvabx_pdics = postClon.pdics;
            return FALSE;
        }

        pvwnd->winfo[wpos].dvabx_dic_cnt = postClon.dic_cnt;
        pvwnd->winfo[wpos].dvabx_pdics = postClon.pdics;

        pre_region = gdk_region_new();
        post_region = gdk_region_new();

        vw_dit_display_get_dva_region(&dp, preClon, pre_region);
        vw_dit_display_get_dva_region(&dp, postClon, post_region);

        gdk_region_union(post_region, pre_region);
        if (gdk_region_empty(post_region)) {
            gdk_region_union_with_rect(post_region, &dp.plt_area);
        }

        gdk_window_invalidate_region(GTK_WIDGET(((NFWINDOW*)g_vwnd)->main_widget)->window, post_region, TRUE);
        gdk_region_destroy(pre_region);
        gdk_region_destroy(post_region);

        vw_dit_display_free_dva_diclist(preClon);
    }
    else if (event->type == INFY_VWND_DRAW_PTZARROW)
    {
        GdkPoint *p = (GdkPoint*)((CMM_MESSAGE_T*)data)->data;
        int cnt = ((CMM_MESSAGE_T*)data)->param;
        GdkRegion *region;

        memcpy(pvwnd->arrow_pt, p, sizeof(GdkPoint)*cnt);
        pvwnd->arrow_ptcnt = cnt;

        region = gdk_region_polygon(pvwnd->arrow_pt, cnt, GDK_WINDING_RULE);
        gdk_window_invalidate_region(GTK_WIDGET(((NFWINDOW*)g_vwnd)->main_widget)->window, region, TRUE);
        gdk_region_destroy(region);
    }
    else if (event->type == INFY_VWND_ERASE_PTZARROW)
    {
        GdkRegion *region;
        GdkRectangle rect;

        if (pvwnd->arrow_ptcnt == 0) return FALSE;

        region = gdk_region_polygon(pvwnd->arrow_pt, pvwnd->arrow_ptcnt, GDK_WINDING_RULE);

        rect.x = pvwnd->arrow_pt[0].x;
        rect.y = pvwnd->arrow_pt[0].y;
        rect.width = 1920/4;
        rect.height = 1080/4;
        gdk_region_union_with_rect(region, &rect);

        memset(pvwnd->arrow_pt, 0x00, sizeof(GdkPoint)*32);
        pvwnd->arrow_ptcnt = 0;
        gdk_window_invalidate_region(GTK_WIDGET(((NFWINDOW*)g_vwnd)->main_widget)->window, region, TRUE);
        gdk_region_destroy(region);
    }
    else if (event->type == INFY_VWND_DRAW_VIDEO_IMAGE)
    {
        VIDEO_IMAGE_T *vimage_cf = ((CMM_MESSAGE_T *)data)->data;
        GdkRectangle tmp;
        GdkRegion *region = 0;

        if (pvwnd->vimage_cf.pbuf)
        {
            g_object_unref(pvwnd->vimage_cf.pbuf);
        }

        if ((pvwnd->vimage_cf.box.width != 0) && (pvwnd->vimage_cf.box.height != 0))
        {
            tmp.x = pvwnd->vimage_cf.box.x - 4;
            tmp.y = pvwnd->vimage_cf.box.y - 4;
            tmp.width = pvwnd->vimage_cf.box.width + 8;
            tmp.height = pvwnd->vimage_cf.box.height + 8;
            region = gdk_region_rectangle(&tmp);
        }

        memcpy(&pvwnd->vimage_cf, vimage_cf, sizeof(VIDEO_IMAGE_T));

        tmp.x = pvwnd->vimage_cf.box.x - 4;
        tmp.y = pvwnd->vimage_cf.box.y - 4;
        tmp.width = pvwnd->vimage_cf.box.width + 8;
        tmp.height = pvwnd->vimage_cf.box.height + 8;

        if (region) {
            gdk_region_union_with_rect(region, &tmp);
        }
        else {
            region = gdk_region_rectangle(&tmp);
        }

        gdk_window_invalidate_region(GTK_WIDGET(((NFWINDOW*)g_vwnd)->main_widget)->window, region, TRUE);
        gdk_region_destroy(region);
    }
    else if (event->type == INFY_VWND_ERASE_VIDEO_IMAGE)
    {
        GdkRegion *region;
        GdkRectangle rect;

        if (pvwnd->vimage_cf.pbuf) {
            g_object_unref(pvwnd->vimage_cf.pbuf);
        }

        if (pvwnd->vimage_cf.box.width == 0) return FALSE;
        if (pvwnd->vimage_cf.box.height == 0) return FALSE;

        rect.x = pvwnd->vimage_cf.box.x - 4;
        rect.y = pvwnd->vimage_cf.box.y - 4;
        rect.width = pvwnd->vimage_cf.box.width + 8;
        rect.height = pvwnd->vimage_cf.box.height + 8;
        region = gdk_region_rectangle(&rect);

        memset(&pvwnd->vimage_cf, 0x00, sizeof(VIDEO_IMAGE_T));

        gdk_window_invalidate_region(GTK_WIDGET(((NFWINDOW*)g_vwnd)->main_widget)->window, region, TRUE);
        gdk_region_destroy(region);
    }
    else if (event->type == INFY_CAMSWITCH_CAPTURE_IMAGE)
    {
        gint result = ((CMM_MESSAGE_T *)data)->param;
        CAPTURE_IMAGE_T *image = ((CMM_MESSAGE_T *)data)->data;
        GInputStream *stream;
        gint width, height;

        GdkRectangle rect;
        GdkRegion *region = 0;

        gint scaled_w, scaled_h;
        gdouble ratio_w, ratio_h, ratio;

        uxm_unreg_imsg_event(obj, INFY_CAMSWITCH_CAPTURE_IMAGE);

        if (result != 0) return FALSE;

        if (g_switch.from_win != -1)
        {
            if (g_win_snap[g_switch.from_win])
            {
                g_object_unref(g_win_snap[g_switch.from_win]);
            }

            if (((pvwnd->dtype == VSM_DIV6) || (pvwnd->dtype == VSM_DIV8)) && (g_switch.from_win == VSM_WIN_ID1))
            {
                vwnd_get_size(pvwnd->dtype, VSM_WIN_ID2, &width, &height);
            }
            else
            {
                vwnd_get_size(pvwnd->dtype, g_switch.from_win, &width, &height);
            }

            stream = g_memory_input_stream_new_from_data(image->buffer, image->size, NULL);

            ratio_w = (gdouble)width / (gdouble)image->width;
            ratio_h = (gdouble)height / (gdouble)image->height;

            ratio = MIN_SCALE_SIZE(ratio_w, ratio_h);

            scaled_w = (gint)(image->width * ratio);
            scaled_h = (gint)(image->height * ratio);
            if (scaled_w > scaled_h) {
                    scaled_w = width;
            } else {
                    scaled_h = height;
            }

            g_win_snap[g_switch.from_win] = gdk_pixbuf_new_from_stream_at_scale(stream, scaled_w, scaled_h, FALSE, NULL, NULL);
            g_object_unref(stream);
        }

        if ((pvwnd->switch_box.width != 0) && (pvwnd->switch_box.height != 0))
        {
            rect.x = pvwnd->switch_box.x - 4;
            rect.y = pvwnd->switch_box.y - 4;
            rect.width = pvwnd->switch_box.width + 8;
            rect.height = pvwnd->switch_box.height + 8;
            region = gdk_region_rectangle(&rect);

            gdk_window_invalidate_region(GTK_WIDGET(((NFWINDOW*)g_vwnd)->main_widget)->window, region, TRUE);
            gdk_region_destroy(region);
        }
    }
    else if (event->type == INFY_PACKET_DUMP_INSERT_MEDIA)
    {
        if (smt_get_service() != SMT_LIVE) return FALSE;

        VW_Packet_Dump_Popup(g_vwnd);
    }
    else
    {
        if (!user_event) return FALSE;

        switch(event->type)
        {
            case GDK_BUTTON_PRESS:
            {
                GdkEventButton *bevent;
                VSM_ID_E pos;
                gint i;

                bevent = (GdkEventButton *)event;
                if ((bevent->x > pvwnd->plt_w) || (bevent->y > pvwnd->plt_h)) return FALSE;

                if (_is_user_logout()) return FALSE;
                if (!_is_valid_win((gint)bevent->x, (gint)bevent->y)) return FALSE;

                if (bevent->button == MOUSE_LEFT_BUTTON)
                {
                    _check_draw_focus_win((gint)bevent->x, (gint)bevent->y);
                    _check_draw_fisheye_view((gint)bevent->x, (gint)bevent->y);
                    _check_va_select_icon((gint)bevent->x, (gint)bevent->y);
                    _proc_press_switch_win((gint)bevent->x, (gint)bevent->y);
                    _proc_press_fisheye_view((gint)bevent->x, (gint)bevent->y);
                }
                else if ((right_press_event) && (bevent->button == MOUSE_RIGHT_BUTTON))
                {
                    _proc_cancel_switch_win();
                    _proc_cancel_fisheye_view();

                    pos = vwnd_get_event_position(pvwnd->dtype, (gint)bevent->x, (gint)bevent->y);
                    vsm_show_shortcut_menu((guint)event->button.x, (guint)event->button.y, pos);
                }
            }
            break;

            case GDK_2BUTTON_PRESS:
            {
                GdkEventButton *bevent;
                VSM_ID_E pos;
                guint ch;
				DIR_RATE_E prev_rate;

                bevent = (GdkEventButton *)event;
                if (bevent->button == MOUSE_RIGHT_BUTTON) break;
                if ((bevent->x > pvwnd->plt_w) || (bevent->y > pvwnd->plt_h)) return FALSE;

                if (_is_user_logout()) return FALSE;
                if (!_is_valid_win((gint)bevent->x, (gint)bevent->y)) return FALSE;

                _proc_cancel_switch_win();
                _proc_cancel_fisheye_view();

				if(vsm_get_vmode() == VMODE_PB)
					prev_rate = vsm_playback_get_dir_rate();

                pos = vwnd_get_event_position(pvwnd->dtype, (gint)bevent->x, (gint)bevent->y);
                vsm_change_sfc_by_2button_press(pos);

				if(vsm_get_vmode() == VMODE_PB)
				{
					if( (prev_rate==DR_PAUSE) || (prev_rate==DR_BWD_NEXT_FRAME) || (prev_rate==DR_FWD_NEXT_FRAME) )
						vsm_playback_change_dir_rate(DIR_FWD);
				}

                var_set_active_layout(-1);
            }
            break;

            case GDK_BUTTON_RELEASE:
            {
                GdkEventButton *bevent;

                bevent = (GdkEventButton*)event;
                if (bevent->button == MOUSE_RIGHT_BUTTON) break;
                if ((bevent->x > pvwnd->plt_w) || (bevent->y > pvwnd->plt_h)) return FALSE;

                _proc_release_switch_win();
                _proc_release_fisheye_view();
            }
            break;

            case GDK_LEAVE_NOTIFY:
            {
                _proc_cancel_switch_win();
            }
            break;

            case GDK_MOTION_NOTIFY:
            {
                GdkEventMotion *mevent;

                mevent = (GdkEventMotion*)event;
                if (((gint)mevent->x > pvwnd->plt_w) || ((gint)mevent->y > pvwnd->plt_h)) return FALSE;

                if (mevent->state & GDK_BUTTON1_MASK)
                {
                    _proc_move_switch_win(mevent->x, mevent->y);
                    _proc_move_fisheye_view(mevent->x, mevent->y);
                }
                else
                {
                    vsm_mouse_motion_detect((guint)mevent->x, (guint)mevent->y);
                }
            }
            break;

            case GDK_SCROLL:
            {
                GdkEventScroll *sevent;

                sevent = (GdkEventScroll*)event;

                if (sevent->direction == GDK_SCROLL_UP) {	
                    _proc_zoom_fisheye_view(sevent->x, sevent->y, 0);
                }
                else if (sevent->direction == GDK_SCROLL_DOWN) {
                    _proc_zoom_fisheye_view(sevent->x, sevent->y, 1);
                }
            }
            break;

            case NFEVENT_KEYPAD_PRESS:
            case NFEVENT_REMOCON_PRESS:
            {
                GdkEventKey *kevt;
                KEYPAD_KID kpid;

                kevt = (GdkEventKey*)event;
                kpid = (KEYPAD_KID)kevt->keyval;

                if (_is_user_logout()) return FALSE;

//              vsm_cmd_key_event(kpid);
            }
            break;

            case NFEVENT_KEYPAD_RELEASE:
            case NFEVENT_REMOCON_RELEASE:
            {
                GdkEventKey *kevt;
                KEYPAD_KID kpid;

                kevt = (GdkEventKey*)event;
                kpid = (KEYPAD_KID)kevt->keyval;

                if (_is_user_logout()) return FALSE;

//              vsm_cmd_key_event(kpid);
            }
            break;

            default:
            break;

        }
    }

    return FALSE;
}

/*
 * External functions..
 */

gint vwnd_switch_win_is_moved()
{
    return  g_switch.moved;
}

gint vwnd_switch_win_is_pressed()
{
    return g_switch.pressed;
}

void vwnd_get_position_info(VSM_DIV_E dtype, gchar win_id, gint *x, gint *y)
{
    vwnd_get_position(dtype, win_id, x, y);
}

void vwnd_get_size_info(VSM_DIV_E dtype, gchar win_id, gint *w, gint *h)
{
    vwnd_get_size(dtype, win_id, w, h);
}

NFOBJECT* get_live_object()
{
    return g_vwnd;
}

GtkWidget *vwnd_get_main_widget(void)
{
    g_return_val_if_fail(g_vwnd != NULL, NULL);

    return ((NFWINDOW*)g_vwnd)->main_widget;
}

void vwnd_set_sfc_mode(VWND_T *p)
{
    g_return_if_fail(p != NULL);

    pvwnd = p;
}

void vwnd_init(NFWINDOW *parent)
{
    gint i;
    gint vct_ch;
    gint size_w, size_h;
    g_message("%s, %d, called", __FUNCTION__, __LINE__);

    /* to get icon size */
    init_icon_size();
    init_switch();
    init_fisheye();
    init_logo_info();

    g_vwnd = (NFOBJECT*)nfui_nfwindow_new(parent, (guint)VWND_POS_X, (guint)VWND_POS_Y, (guint)DISPLAY_ACTIVE_WIDTH, (guint)DISPLAY_ACTIVE_HEIGHT);
    gtk_window_set_modal(GTK_WINDOW(((NFWINDOW*)g_vwnd)->main_widget), FALSE);
    nfui_nfobject_modify_bg(g_vwnd, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));

    g_vw_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(g_vw_fixed, (guint)DISPLAY_ACTIVE_WIDTH, (guint)DISPLAY_ACTIVE_HEIGHT);
    nfui_nfobject_modify_bg(g_vw_fixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
    nfui_regi_pre_event_callback(g_vw_fixed, vw_fixed_event_handler);

    for(i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        g_chk_btn[i] = nftool_normal_button_create_popup_type2("LOG IN", VWND_LOGINFAIL_WIDTH);
        nfui_regi_post_event_callback(g_chk_btn[i], post_login_fail_btn_event_handler);
        nfui_nfobject_set_data(g_chk_btn[i], "ch-num", GINT_TO_POINTER(i));
    }

    nfui_nfwindow_add((NFWINDOW*)g_vwnd, g_vw_fixed);
    nfui_nfobject_show(g_vw_fixed);

    nfui_run_main_event_handler(g_vwnd);
    nfui_nfobject_hide(g_vwnd);

    nftool_set_custom_cursor(GTK_WIDGET(((NFWINDOW*)g_vwnd)->main_widget)->window);
    nfui_page_open(PGID_LIVEDISPLAY, g_vwnd, "ADMIN");

    uxm_reg_imsg_event(g_vw_fixed, INFY_VWND_RUN_ALL_EVENT);
    uxm_reg_imsg_event(g_vw_fixed, INFY_VWND_STOP_ALL_EVENT);
    uxm_reg_imsg_event(g_vw_fixed, INFY_VWND_RUN_RIGHT_PRESS);
    uxm_reg_imsg_event(g_vw_fixed, INFY_VWND_STOP_RIGHT_PRESS);
    uxm_reg_imsg_event(g_vw_fixed, INFY_VWND_CHANGE_BORDER);
    uxm_reg_imsg_event(g_vw_fixed, INFY_VWND_DRAW_FOCUS);
    uxm_reg_imsg_event(g_vw_fixed, INFY_VWND_ERASE_FOCUS);
    uxm_reg_imsg_event(g_vw_fixed, INFY_VWND_SWITCH_PRESS);
    uxm_reg_imsg_event(g_vw_fixed, INFY_VWND_SWITCH_MOVE);
    uxm_reg_imsg_event(g_vw_fixed, INFY_VWND_SWITCH_RELEASE);
    uxm_reg_imsg_event(g_vw_fixed, INFY_VWND_SWITCH_CANCEL);
    uxm_reg_imsg_event(g_vw_fixed, INFY_VWND_DRAW_NEW_VCA);
    uxm_reg_imsg_event(g_vw_fixed, INFY_VWND_DRAW_NEW_DVABX);
    uxm_reg_imsg_event(g_vw_fixed, INFY_VWND_DRAW_PTZARROW);
    uxm_reg_imsg_event(g_vw_fixed, INFY_VWND_ERASE_PTZARROW);
    uxm_reg_imsg_event(g_vw_fixed, INFY_VWND_DRAW_VIDEO_IMAGE);
    uxm_reg_imsg_event(g_vw_fixed, INFY_VWND_ERASE_VIDEO_IMAGE);

    uxm_reg_imsg_event(g_vw_fixed, INFY_PACKET_DUMP_INSERT_MEDIA);
}

void vwnd_show()
{
    nfui_nfobject_show(g_vwnd);
}

void vwnd_repaint()
{
    nfui_signal_emit(g_vwnd, GDK_EXPOSE, TRUE);
}

void vwnd_set_item_position()
{
    _set_item_position(pvwnd);
}

void vwnd_set_item_position_connecting(gint ch)
{
	_set_postion_connecting(pvwnd, ch);
}

void vwnd_set_blind()
{
    nfui_nfobject_modify_bg(g_vw_fixed, NFOBJECT_STATE_NORMAL, VIDEO_BG_COLOR);
}

void vwnd_unset_blind()
{
    nfui_nfobject_modify_bg(g_vw_fixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
}
