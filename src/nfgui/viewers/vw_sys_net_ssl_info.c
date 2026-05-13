#include "support/event_loop.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"

#include "uxm.h"
#include "scm.h"

#include "vw_sys_net_ssl_info.h"


#define SSL_WIN_SIZE_W                  (800)
#define SSL_WIN_SIZE_H                  (370)
#define SSL_WIN_POS_X                   ((DISPLAY_ACTIVE_WIDTH - SSL_WIN_SIZE_W) / 2)
#define SSL_WIN_POS_Y                   ((DISPLAY_ACTIVE_HEIGHT - SSL_WIN_SIZE_H) / 2)

#define SSL_LABEL_H                     (40)
#define SSL_SUB_TITLE_W                 (200)

#define SSL_BTN_W                       (200)
#define SSL_CLOSE_BTN_X                 (SSL_WIN_SIZE_W - 10 - SSL_BTN_W)
#define SSL_BTN_Y                       (SSL_WIN_SIZE_H - 55)

static NFWINDOW *g_curwnd = 0;


static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *topwin;

    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        topwin = nfui_nfobject_get_top(obj);
        nfui_nfobject_destroy(topwin);
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

void VW_SSL_Information_Open(NFOBJECT *parent, SSL_CERT_INFO_T *cert_info)
{
    NFOBJECT *win;
    NFOBJECT *main_fixed;
    NFOBJECT *obj;
    
    SSL_CERT_INFO_T info;

    gint pos_x, pos_y;


    memset(&info, 0x00, sizeof(info));
    g_memmove(&info, cert_info, sizeof(SSL_CERT_INFO_T));

    win = nftool_create_popup_window((NFWINDOW*)parent, SSL_WIN_POS_X, SSL_WIN_POS_Y, SSL_WIN_SIZE_W, SSL_WIN_SIZE_H, "CERTIFICATE INFORMATION", FALSE);
    nfui_regi_post_event_callback(win, post_main_win_event_handler);
    g_curwnd = win;
    
    main_fixed = (NFWINDOW*)g_curwnd->child;

// <---- PRIVATE KEY
    pos_x = 18;
    pos_y = 74;
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ISSUER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, SSL_SUB_TITLE_W, SSL_LABEL_H);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

    pos_x += SSL_SUB_TITLE_W;
    
    obj = (NFOBJECT*)nfui_nflabel_new_text_box(info.issuer, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 5);
    nfui_nfobject_set_size(obj, 572, SSL_LABEL_H);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
    
// <---- PRIVATE KEY PASSWORD
    pos_x = 18;
    pos_y += SSL_LABEL_H + 2;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SUBJECT", nffont_get_pango_font(NFFONT_SMALL_SEMI_1), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, SSL_SUB_TITLE_W, SSL_LABEL_H);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

    pos_x += SSL_SUB_TITLE_W;
    
    obj = (NFOBJECT*)nfui_nflabel_new_text_box(info.subject, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 5);
    nfui_nfobject_set_size(obj, 572, SSL_LABEL_H);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

    pos_x = 18;
    pos_y += SSL_LABEL_H + 2;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("VALID FROM", nffont_get_pango_font(NFFONT_SMALL_SEMI_1), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, SSL_SUB_TITLE_W, SSL_LABEL_H);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

    pos_x += SSL_SUB_TITLE_W;
    
    obj = (NFOBJECT*)nfui_nflabel_new_text_box(info.from, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 5);
    nfui_nfobject_set_size(obj, 266, SSL_LABEL_H);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
    
    pos_x += 266;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("~", nffont_get_pango_font(NFFONT_SMALL_SEMI_1), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 40, SSL_LABEL_H);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

    pos_x += 40;
    
    obj = (NFOBJECT*)nfui_nflabel_new_text_box(info.to, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 5);
    nfui_nfobject_set_size(obj, 266, SSL_LABEL_H);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);


// <---- CLOSE BUTTON
    obj = nftool_normal_button_create_type1("CLOSE", SSL_BTN_W);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)main_fixed, obj, SSL_CLOSE_BTN_X, SSL_BTN_Y);
    nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

    nfui_nfobject_show(win);

    /* set for key navi */
    nfui_make_key_hierarchy((NFWINDOW *)win);

    gtk_main();
    
    return;
}
