#include "nf_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_page_manager.h"
#include "support/color.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfscrolledfixed.h"
#include "objects/nflabel.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"

#include "services/vsm.h"

#include "modules/ssm.h"

#include "vw_vwnd.h"
#include "vw_live_shortcut_menu.h"
#include "vw_zoom_pip.h"
#include "vw_timeline.h"
#include "vw_set_date_time.h"
#include "vw_live_statusbar.h"

#include "uxm.h"

#define MENU_TEXT_FRONT_MARGIN      (15)
#define MENU_GAP                    2

#define VIDEO_IMAGE_W               (16*20)
#define VIDEO_IMAGE_H               (9*20)

#define MIN_SCALE_SIZE(a, b)                   (((a) < (b)) ? (a) : (b))

static NFOBJECT *g_parent;
static NFOBJECT *g_prt_item;

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_ccWin;
static NFOBJECT *g_sel_ch[GUI_CHANNEL_CNT];

static gint g_vimage_channel = -1;
static GdkRectangle g_image_box;

static guint g_captue_tmr = 0;


static gboolean _request_captue_image(gpointer data)
{
    gint ch = GPOINTER_TO_INT(data);
    gint ret;

    ret = scm_req_live_capture_without_stream(INFY_SHORTCUT_PIP_CAPTURE_IMAGE, ch);
    if (ret == -1) return TRUE;

    return FALSE;
}

static gboolean post_cc_sub_win_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    int mx, my, mw, mh;

    if (evt->type == NFOUTEVT_MOTION_NOTIFY) 
    {
        GdkEventMotion *mevt;
        GdkRegion *region = 0;
        GdkRectangle rect;

        gboolean is_point_in;

        mevt = (GdkEventMotion*)evt;

        rect.x = g_parent->x-4;
        rect.y = g_parent->y + g_prt_item->y;
        rect.width = g_parent->width+8;
        rect.height = g_prt_item->height;
        region = gdk_region_rectangle(&rect);

        if ((g_image_box.width != 0) && (g_image_box.height != 0))
        {
            memcpy(&rect, &g_image_box, sizeof(GdkRectangle));
            rect.x -= 6;
            rect.width += 12;
            gdk_region_union_with_rect(region, &rect);
        }
        
        is_point_in = gdk_region_point_in(region, (gint)mevt->x_root, (gint)mevt->y_root);
        gdk_region_destroy(region);
        
        if (is_point_in == FALSE) 
        {
            g_vimage_channel = -1;            
            memset(&g_image_box, 0x00, sizeof(GdkRectangle));        
            evt_send_to_local(INFY_VWND_ERASE_VIDEO_IMAGE, 0, 0, 0);
        
            nfui_nfobject_hide(obj);
            return TRUE;
        }
    }
    else if (evt->type == NFOUTEVT_BUTTON_PRESS) 
    {
        GdkEventButton *bevt;    
        GdkRegion *region = 0;
        GdkRectangle rect;
        
        gboolean is_point_in = FALSE;
        gint from_ch = -1, to_ch = -1;

        bevt = (GdkEventButton*)evt;

        if ((g_image_box.width != 0) && (g_image_box.height != 0))
        { 
            region = gdk_region_rectangle(&g_image_box);

            is_point_in = gdk_region_point_in(region, (gint)bevt->x_root, (gint)bevt->y_root);
            gdk_region_destroy(region);

            from_ch = get_menu_channel();
            to_ch = g_vimage_channel;
        }

        g_vimage_channel = -1;
        memset(&g_image_box, 0x00, sizeof(GdkRectangle));
        evt_send_to_local(INFY_VWND_ERASE_VIDEO_IMAGE, 0, 0, 0);
    
        cam_change_enable_obj();
        nfui_nfobject_hide(obj);
        VW_ShortCut_Menu_Hide();

        if ((is_point_in == TRUE) && (to_ch != -1)) {
            vsm_switch_channel(from_ch, to_ch);
        }
    }
    else if (evt->type == INFY_SHORTCUT_PIP_CAPTURE_IMAGE)
    {   
        gint result = ((CMM_MESSAGE_T *)data)->param;
        CAPTURE_IMAGE_T *image = ((CMM_MESSAGE_T *)data)->data;
        GInputStream *stream;

        VIDEO_IMAGE_T *vimage_cf;
        gint pos_x, pos_y; 

        gint scaled_w, scaled_h;
        gdouble ratio_w, ratio_h, ratio;

        if (g_vimage_channel == -1) return FALSE;
        if (image->ch != g_vimage_channel) return FALSE;
        if (result != 0) return FALSE;

        pos_x = obj->x + obj->width+6;        
        pos_y = obj->y + g_sel_ch[image->ch]->y;

        if (g_parent->x > obj->x) {
            pos_x = obj->x - (VIDEO_IMAGE_W+4) - 6;
        }

        if (pos_y+VIDEO_IMAGE_H+4 > obj->y+obj->height) {
            pos_y = obj->y + obj->height - (VIDEO_IMAGE_H+4);
        }

        vimage_cf = imalloc(sizeof(VIDEO_IMAGE_T));
        memcpy(&vimage_cf->box, &g_image_box, sizeof(GdkRectangle));
        var_get_camtitle(vimage_cf->title, image->ch);

        stream = g_memory_input_stream_new_from_data(image->buffer, image->size, NULL);

        ratio_w = (gdouble)VIDEO_IMAGE_W / (gdouble)image->width;
        ratio_h = (gdouble)VIDEO_IMAGE_H / (gdouble)image->height;

        ratio = MIN_SCALE_SIZE(ratio_w, ratio_h);

        scaled_w = (gint)(image->width * ratio);
        scaled_h = (gint)(image->height * ratio);
        if (scaled_w > scaled_h) {
                scaled_w = VIDEO_IMAGE_W;
        } else {
                scaled_h = VIDEO_IMAGE_H;
        }

        vimage_cf->pbuf = gdk_pixbuf_new_from_stream_at_scale(stream, scaled_w, scaled_h, FALSE, NULL, NULL);
        g_object_unref(stream);
   
        evt_send_to_local(INFY_VWND_DRAW_VIDEO_IMAGE, 0, 1, vimage_cf);        
    }        
    else if (evt->type == GDK_DELETE) 
    {
        g_vimage_channel = -1;
        memset(&g_image_box, 0x00, sizeof(GdkRectangle));
        evt_send_to_local(INFY_VWND_ERASE_VIDEO_IMAGE, 0, 0, 0);

        uxm_unreg_imsg_event(obj, INFY_SHORTCUT_PIP_CAPTURE_IMAGE);
        g_curwnd = 0;
    }

    return FALSE;
}

static gboolean post_cc_sub_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc; 
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h; 

    if (evt->type == GDK_EXPOSE)
    {
        drawable = nfui_nfobject_get_window(obj);
        gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1,NFALIGN_LEFT,0);

        nfui_nfobject_gc_unref(gc);
    }
    else if(evt->type == GDK_DELETE)
    {       
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
    }

    return FALSE;
}

static gboolean post_cc_sub_btn_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_PRESS)
    {
        guint to_ch;
        guint cur_ch;

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        g_vimage_channel = -1;    
        memset(&g_image_box, 0x00, sizeof(GdkRectangle));        
        evt_send_to_local(INFY_VWND_ERASE_VIDEO_IMAGE, 0, 0, 0);

        nfui_nfobject_hide(g_ccWin);
        cam_change_enable_obj();        
        VW_ShortCut_Menu_Hide();

        cur_ch = get_menu_channel();
        to_ch = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj,"button index")); 
        
        vsm_switch_channel(cur_ch, to_ch);
    }
    else if (evt->type == GDK_ENTER_NOTIFY)
    {
        VIDEO_IMAGE_T *vimage_cf = imalloc(sizeof(VIDEO_IMAGE_T));
        NF_NOTIFY_INFO buf;
        guint covert_mask = 0;

        NFOBJECT *top;
        gint pos_x, pos_y;
        gint ret;

        if (ivsc.dfunc.support_advanced_chswitch == 0) return FALSE; 

        g_vimage_channel = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "button index")); 

        top = nfui_nfobject_get_top(obj);
        pos_x = top->x + top->width+6;        
        pos_y = top->y + obj->y;

        if (g_parent->x > top->x) {
            pos_x = top->x - (VIDEO_IMAGE_W+4) - 6;
        }

        if (pos_y+VIDEO_IMAGE_H+4 > top->y+top->height) {
            pos_y = top->y + top->height - (VIDEO_IMAGE_H+4);
        }

        g_image_box.x = pos_x;
        g_image_box.y = pos_y;
        g_image_box.width = VIDEO_IMAGE_W+4;
        g_image_box.height = VIDEO_IMAGE_H+4;
        memcpy(&vimage_cf->box, &g_image_box, sizeof(GdkRectangle));
        var_get_camtitle(vimage_cf->title, g_vimage_channel);
        evt_send_to_local(INFY_VWND_DRAW_VIDEO_IMAGE, 0, 1, vimage_cf);

        scm_get_vloss_data(&buf);
        covert_mask = ssm_get_covert_mask();
   
        if (buf.d.params[0] & (1 << g_vimage_channel)) return FALSE;
        if (covert_mask & (1 << g_vimage_channel)) return FALSE;
        
        if (g_captue_tmr) {
            g_source_remove(g_captue_tmr);
            g_captue_tmr = 0;
        }

        g_captue_tmr = g_timeout_add(200, _request_captue_image, GINT_TO_POINTER(g_vimage_channel));
    }
    else if (evt->type == GDK_LEAVE_NOTIFY)
    {
//      VIDEO_IMAGE_T *vimage_cf = imalloc(sizeof(VIDEO_IMAGE_T));
    
//      g_vimage_channel = -1;        
//      memcpy(&vimage_cf->box, &g_image_box, sizeof(GdkRectangle));     
//      evt_send_to_local(INFY_VWND_DRAW_VIDEO_IMAGE, 0, 1, vimage_cf);
    }
    
    return FALSE;
}

void VW_create_camchange_submenu(NFWINDOW *parent, MENU_CONF menu_conf, NFOBJECT *parent_item)
{
    NFOBJECT *scrolled_fixed;
    NFOBJECT *fixed;
    NFOBJECT *obj;
    NFOBJECT *cc_tbl;
   
    guint font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(340), COLOR_IDX(341), COLOR_IDX(343), COLOR_IDX(344)};
    guint win_w;
    guint width =212;

    guint i;
    guint cur_ch;
    gint row_cnt = 0;
    
    gchar buf[64];

    g_parent = parent;
    g_prt_item = parent_item;

    if (GUI_CHANNEL_CNT > 16) 
    {
        row_cnt = 16;
        win_w = 310;
    }
    else
    {
        row_cnt = GUI_CHANNEL_CNT;
        win_w = 220;        
    }

    // window 
    g_ccWin = (NFOBJECT*)nfui_nfwindow_new(parent, 0, 0, win_w, 44*row_cnt+16);
    nfui_nfwindow_use_outside_evt((NFWINDOW*)g_ccWin, TRUE);
    nfui_nfwindow_set_mask((NFWINDOW*)g_ccWin, GDK_MOTION_NOTIFY, TRUE);
    nfui_nfwindow_set_mask((NFWINDOW*)g_ccWin, GDK_BUTTON_PRESS, TRUE);
    nfui_nfwindow_set_title((NFWINDOW*)g_ccWin, "LIVE SHORTCUT SUB CC");
    nfui_regi_post_event_callback(g_ccWin, post_cc_sub_win_event_cb);    
    uxm_reg_imsg_event(g_ccWin, INFY_SHORTCUT_PIP_CAPTURE_IMAGE);
    g_curwnd = g_ccWin;

    gtk_widget_add_events(((NFWINDOW*)g_ccWin)->main_widget, GDK_POINTER_MOTION_HINT_MASK);

    fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed, win_w, 44*row_cnt+16);
    nfui_nfobject_show(fixed);
    nfui_regi_post_event_callback(fixed, post_cc_sub_fixed_event_handler);

    scrolled_fixed = (NFOBJECT*)nfui_nfscrolledfixed_new(NFSCROLLED_POLICY_NEVER, NFSCROLLED_POLICY_AUTOMATIC);
    nfui_nfscrolledfixed_set_skin_type((NFSCROLLEDFIXED*)scrolled_fixed, NFSCROLLEDFIXED_TYPE_1);
    nfui_nfscrolledfixed_set_vscroll_speed((NFSCROLLEDFIXED*)scrolled_fixed, (44*row_cnt)/3, 80, (44*row_cnt)/5);
    nfui_nfobject_modify_bg(scrolled_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(scrolled_fixed, win_w-8, 44*row_cnt);
    nfui_nfobject_show(scrolled_fixed);
    nfui_nffixed_put((NFFIXED*)fixed, scrolled_fixed, 4, 8);
    //nfui_regi_post_event_callback(scrolled_fixed, post_cc_sub_fixed_event_handler);


    cc_tbl = nfui_nftable_new(1, GUI_CHANNEL_CNT, 0, 2, &width, 40);
    nfui_nfobject_show(cc_tbl);
    nfui_nfscrolledfixed_put((NFSCROLLEDFIXED*)scrolled_fixed, cc_tbl, 0, 0);

    for (i = 0; i < GUI_CHANNEL_CNT; i++) 
    {
        memset(buf,0x00,sizeof(buf));
        var_get_camtitle(buf, i);       
    
        obj = (NFOBJECT*)nfui_nfbutton_new_with_param(menu_conf.submenu_img[0], buf);        
        nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_LEFT, MENU_TEXT_FRONT_MARGIN);
        nfui_nfbutton_set_pango_font((NFBUTTON*)obj, nffont_get_pango_font(NFFONT_LARGE_SEMI), (guint*)font_color);
        nfui_nfobject_support_multi_lang(obj, FALSE);
        nfui_nfobject_show(obj);
        nfui_nfobject_set_data(obj, "button index", GUINT_TO_POINTER(i));
        nfui_nftable_attach((NFTABLE*)cc_tbl, obj, 0, i);
        nfui_regi_post_event_callback(obj, post_cc_sub_btn_event_cb);
        g_sel_ch[i] = obj;
    }

    nfui_nfwindow_add((NFWINDOW*)g_ccWin, fixed);
    nfui_nfobject_hide(g_ccWin);
    nfui_run_main_event_handler(g_ccWin);
}

int VW_destroy_camchange_submenu()
{
    if(!g_curwnd) return 0;
    
    nfui_nfobject_destroy(g_curwnd);
    return 0;
}

int VW_move_cc_submenu(int x, int y)
{
    nfui_nfobject_move(g_ccWin, x, y);
}

void VW_show_cc_submenu()
{
    guint x, y;
    gint cur_ch;

    gchar buf[STRING_SIZE_64];
    gint i;

    if (!nfui_nfobject_is_shown(g_ccWin))
    {
        cur_ch = get_menu_channel();
        nfui_nfobject_disable(g_sel_ch[cur_ch]);

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {            
            memset(buf,0x00,sizeof(buf));
            var_get_camtitle(buf, i);
            nfui_nfbutton_set_text(g_sel_ch[i], buf);
        }
        
        nfui_nfobject_show(g_ccWin);
    }
}

void VW_destroy_image_box()
{
    memset(&g_image_box, 0x00, sizeof(GdkRectangle));
    evt_send_to_local(INFY_VWND_ERASE_VIDEO_IMAGE, 0, 0, 0);
}

void VW_hide_cc_submenu()
{
    if (nfui_nfobject_is_shown(g_ccWin)) {
        nfui_nfobject_hide(g_ccWin);
        VW_destroy_image_box();
    }
}

void cam_change_enable_obj()
{
    gint i;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        if(nfui_nfobject_is_disabled(g_sel_ch[i]))  nfui_nfobject_enable(g_sel_ch[i]);
    }
}

