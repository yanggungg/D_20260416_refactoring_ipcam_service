#include "support/nf_ui_image.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"
#include "objects/nfcombobox.h"
#include "objects/nfcheckbutton.h"
#include "objects/nflistbox.h"
#include "objects/nfimage.h"

#include "ix_mem.h"
#include "modules/ssm.h"

#include "uxm.h"
#include "scm.h"
#include "smt.h"
#include "iux_msg.h"

#include "vw_sys_net_ssl_install_popup.h"
#include "vw_sys_net_ssl_load_file_popup.h"


#define SSL_WIN_SIZE_W                  (800)
#define SSL_WIN_SIZE_H                  (370)
#define SSL_WIN_POS_X                   ((DISPLAY_ACTIVE_WIDTH - SSL_WIN_SIZE_W) / 2)
#define SSL_WIN_POS_Y                   ((DISPLAY_ACTIVE_HEIGHT - SSL_WIN_SIZE_H) / 2)

#define SSL_SUB_FIXED_W                 (SSL_WIN_SIZE_W - 8)
#define SSL_SUB_FIXED_H                 (SSL_WIN_SIZE_H - 68)
#define SSL_LABEL_H                     (40)
#define SSL_DESC_W                      (SSL_SUB_FIXED_W - 20)
#define SSL_SUB_TITLE_W                 (260)

#define SSL_BTN_W                       (200)
#define SSL_CLOSE_BTN_X                 (SSL_SUB_FIXED_W - 2 - SSL_BTN_W)
#define SSL_NEXT_BTN_X                  (SSL_CLOSE_BTN_X - 2 - SSL_BTN_W)
#define SSL_BTN_Y                       (SSL_SUB_FIXED_H - 55)

enum {
    BUILT_IN = 0,
    CA_SIGNED,
    SELF_SIGNED
};

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_sub_fixed[4];
static NFOBJECT *g_radio_btn[3] = {0,};


static gboolean post_next_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *topwin;
    gint i;
    gint val;

    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;
            
        for (i = 0; i < 3; i++)
        {
            val = nfui_radio_button_get_toggled(NF_BUTTON(g_radio_btn[i]));
            if (val) break;
        }
        
        nfui_nfobject_hide(g_sub_fixed[0]);
        nfui_signal_emit(g_sub_fixed[0], GDK_EXPOSE, TRUE);
        
        if (i == BUILT_IN)
        {
            nfui_nfobject_show(g_sub_fixed[1]);
            nfui_signal_emit(g_sub_fixed[1], GDK_EXPOSE, TRUE);
        }
        else if (i == CA_SIGNED)
        {
            nftool_mbox(g_curwnd, "WARNING", "Recommend to check the issuing company's certificate installation manual before installing the certificate.", NFTOOL_MB_OK);
            nfui_nfobject_show(g_sub_fixed[2]);
            nfui_signal_emit(g_sub_fixed[2], GDK_EXPOSE, TRUE);
        }
        else
        {
            nfui_nfobject_show(g_sub_fixed[3]);
            nfui_signal_emit(g_sub_fixed[3], GDK_EXPOSE, TRUE);
        }
        
        nfui_make_key_hierarchy((NFWINDOW*)g_curwnd);
    }

    return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *topwin;

    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        topwin = nfui_nfobject_get_top(obj);
        nfui_nfobject_destroy(topwin);
    }

    return FALSE;
}

static gboolean pre_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
    NFOBJECT *topwin;
    GdkDrawable *drawable = NULL;
    GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

    switch (event->type)
    {
        case GDK_EXPOSE:
        {
            drawable = nfui_nfobject_get_window(obj);
            gc = nfui_nfobject_get_gc(obj);

            nfui_nfobject_get_size(obj, &size_w, &size_h);
            pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
            nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);

            nfui_nfobject_gc_unref(gc);
        }
        break;

        case GDK_BUTTON_RELEASE:
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

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_DELETE)
    {
        g_curwnd = 0;
        gtk_main_quit();
    }

    return FALSE;
}

static void vw_ssl_installer_main_page(NFOBJECT *parent)
{
    NFOBJECT *main_fixed;
    NFOBJECT *obj;
    
    gint pos_x, pos_y;
	gint size_w, size_h;
    gint i;
    
	GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];
	GSList *slist = NULL;
    
    gchar *strTitle[] = {"BUILT-IN CERTIFICATE", "CA SIGNED CERTIFICATE", "SELF SIGNED CERTIFICATE"};
    
    main_fixed = parent;
    
	/* radio button */
	radio_img[0] = nfui_get_image_from_file((IMG_N_POPUP_RADIO_OFF), NULL);
	radio_img[1] = nfui_get_image_from_file((IMG_O_POPUP_RADIO_ON), NULL);
	radio_img[2] = nfui_get_image_from_file((IMG_P_POPUP_RADIO_ON), NULL);
	radio_img[3] = nfui_get_image_from_file((IMG_D_POPUP_RADIO_OFF), NULL);

    // <---- DESCRIPTION
    pos_x = 18;
    pos_y = 0;

    obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("Select the type of certificate you want to install.\nNote! Existing certificates will be deleted.", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL *)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, SSL_DESC_W, 70);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)main_fixed, obj, pos_x, pos_y);

	nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

    // <---- BUILT-IN
    pos_y += 70 + 15;

	nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

    for (i = 0; i < 3; i++)
    {
        obj = (NFOBJECT *)nfui_nfbutton_new();
        nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
        nfui_nfbutton_set_drawing_outline((NFBUTTON *)obj, FALSE);
        nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
        if (i != BUILT_IN) nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED *)main_fixed, obj, pos_x, pos_y);
        g_radio_btn[i] = obj;

        if (i == CA_SIGNED)
        {
            slist = nfui_radio_button_get_group(NF_BUTTON(obj));
            nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);
        }
        else
        {
            nfui_radio_button_add_group(NF_BUTTON(obj), slist);
        }

        /* label */
        obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
        nfui_nfobject_set_size(obj, SSL_SUB_TITLE_W, SSL_LABEL_H);
        nfui_nflabel_set_align((NFLABEL *)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        if (i != BUILT_IN) nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED *)main_fixed, obj, pos_x + 40, pos_y - ((SSL_LABEL_H - size_h) / 2));
        
        if (i != BUILT_IN) pos_y += SSL_LABEL_H + 2;
    }

    // <---- NEXT BUTTON
    obj = nftool_normal_button_create_type1("NEXT", SSL_BTN_W);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)main_fixed, obj, SSL_NEXT_BTN_X, SSL_BTN_Y);
    nfui_regi_post_event_callback(obj, post_next_event_handler);

    // <---- CLOSE BUTTON
    obj = nftool_normal_button_create_type1("CLOSE", SSL_BTN_W);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)main_fixed, obj, SSL_CLOSE_BTN_X, SSL_BTN_Y);
    nfui_regi_post_event_callback(obj, post_closebutton_event_handler);
}

gboolean VW_SSL_Installer_Open(NFWINDOW *parent)
{
    NFOBJECT *win;
    NFOBJECT *main_fixed;
    NFOBJECT *sub_fixed;
    NFOBJECT *obj;

    gint pos_x, pos_y;
    gint i;

    
    // <---- WINDOW
    win = (NFOBJECT *)nfui_nfwindow_new(parent, SSL_WIN_POS_X, SSL_WIN_POS_Y, SSL_WIN_SIZE_W, SSL_WIN_SIZE_H);
    nfui_regi_post_event_callback(win, post_main_win_event_handler);
    g_curwnd = win;

    // <---- FIXED
    main_fixed = (NFOBJECT *)nfui_nffixed_new();
    nfui_nfobject_set_size(main_fixed, SSL_WIN_SIZE_W, SSL_WIN_SIZE_H);
    nfui_nfobject_modify_bg(main_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_regi_pre_event_callback(main_fixed, pre_main_fixed_event_handler);
    nfui_nfobject_show(main_fixed);

    // <---- TITLE
    obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("INSTALL SSL CERTIFICATE", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
    nfui_nfobject_set_size(obj, SSL_WIN_SIZE_W - 8, 36);
    nfui_nflabel_set_align((NFLABEL *)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)main_fixed, obj, 4, 4);
    
    for (i = 0; i < 4; i++)
    {
        sub_fixed = (NFOBJECT *)nfui_nffixed_new();
        nfui_nfobject_set_size(sub_fixed, SSL_SUB_FIXED_W, SSL_SUB_FIXED_H);
        nfui_nfobject_modify_bg(sub_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nffixed_put((NFFIXED *)main_fixed, sub_fixed, 4, 64);
        g_sub_fixed[i] = sub_fixed;
    }
    
    vw_ssl_installer_main_page(g_sub_fixed[0]);
    vw_ssl_installer_builtin_page(g_sub_fixed[1]);
    vw_ssl_installer_ca_page(g_sub_fixed[2]);
    vw_ssl_installer_self_page(g_sub_fixed[3]);

    nfui_nfobject_show(g_sub_fixed[0]);

    nfui_nfwindow_add((NFWINDOW *)win, main_fixed);
    nfui_run_main_event_handler(win);
    nfui_nfobject_show(win);

    /* set for key navi */
    nfui_make_key_hierarchy((NFWINDOW *)win);

    nfui_page_open(PGID_INSTALL_SSL_CERTIFICATE_POPUP, win, ssm_get_cur_id(NULL));

    gtk_main();

    nfui_page_close(PGID_INSTALL_SSL_CERTIFICATE_POPUP, win);

    return 0;
}

gboolean VW_Show_SSL_Installer_main()
{
    if (!g_sub_fixed[0]) return FALSE;
    
    nfui_nfobject_show(g_sub_fixed[0]);
    nfui_signal_emit(g_sub_fixed[0], GDK_EXPOSE, TRUE);
}
