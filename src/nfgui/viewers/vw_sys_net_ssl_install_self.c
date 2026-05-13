#include "support/event_loop.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"
#include "ix_mem.h"

#include "uxm.h"
#include "scm.h"
#include "iux_msg.h"

#include "vw_sys_net_ssl_install_popup.h"
#include "vw_sys_net_ssl_load_file_popup.h"
#include "vw_vkeyboard.h"


#define SSL_SUB_FIXED_W                 (800 - 8)
#define SSL_SUB_FIXED_H                 (370 - 68)
#define SSL_LABEL_H                     (40)
#define SSL_DESC_W                      (SSL_SUB_FIXED_W - 20)
#define SSL_SUB_TITLE_W                 (260)

#define SSL_BTN_W                       (200)
#define SSL_CLOSE_BTN_X                 (SSL_SUB_FIXED_W - 2 - SSL_BTN_W)
#define SSL_INSTALL_BTN_X               (SSL_CLOSE_BTN_X - 2 - SSL_BTN_W)
#define SSL_PREV_BTN_X                  (SSL_INSTALL_BTN_X - 2 - SSL_BTN_W)
#define SSL_BTN_Y                       (SSL_SUB_FIXED_H - 55)


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_wait = NULL;


static gboolean post_prev_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *topwin;

    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
        
        nfui_nfobject_hide(g_curwnd);
        nfui_signal_emit(g_curwnd, GDK_EXPOSE, TRUE);
        
        VW_Show_SSL_Installer_main();
    }

    return FALSE;
}

static gboolean post_install_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *topwin;
    gint ret = 0;
    gchar pw[128];

    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        memset(pw, 0x00, sizeof(pw));
        scm_ssl_self_signed_install(IRET_SSL_INSTALL);
        
        g_wait = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
    }

    return FALSE;
}

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

static gboolean post_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
    NFOBJECT *topwin;

    switch(event->type) 
    {
        case GDK_DELETE:
        {
            uxm_unreg_imsg_event(obj, IRET_SSL_INSTALL);    
        }
        break;

        case IRET_SSL_INSTALL:
        {
            gint ret = ((CMM_MESSAGE_T *)data)->param;
            
            if (g_wait) {
                nftool_remove_waitbox(g_wait);
                g_wait = NULL;
            }

            nftool_mbox(g_curwnd, "NOTICE", "Completed the installation of the SSL certificate.", NFTOOL_MB_OK);
            
            topwin = nfui_nfobject_get_top(obj);
            nfui_nfobject_destroy(topwin);
        }
        break;

        default:
        break;
    }

    return FALSE;
}

void vw_ssl_installer_self_page(NFOBJECT *parent)
{
    NFOBJECT *win;
    NFOBJECT *main_fixed;
    NFOBJECT *obj;

    gint pos_x, pos_y;

    g_curwnd = main_fixed = parent;
    nfui_regi_post_event_callback(main_fixed, post_main_fixed_event_handler);

    // <---- DESCRIPTION
    pos_x = 18;
    pos_y = 0;

    obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("Automatically generate and install self-signed certificates.", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL *)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, SSL_DESC_W, SSL_LABEL_H);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)main_fixed, obj, pos_x, pos_y);

    
// <---- PREV BUTTON
    obj = nftool_normal_button_create_type1("PREV", SSL_BTN_W);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, SSL_PREV_BTN_X, SSL_BTN_Y);
    nfui_regi_post_event_callback(obj, post_prev_event_handler);

// <---- INSTALL BUTTON
    obj = nftool_normal_button_create_type1("INSTALL", SSL_BTN_W);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, SSL_INSTALL_BTN_X, SSL_BTN_Y);
    nfui_regi_post_event_callback(obj, post_install_event_handler);

// <---- CLOSE BUTTON
    obj = nftool_normal_button_create_type1("CLOSE", SSL_BTN_W);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)main_fixed, obj, SSL_CLOSE_BTN_X, SSL_BTN_Y);
    nfui_regi_post_event_callback(obj, post_closebutton_event_handler);
    
    uxm_reg_imsg_event(main_fixed, IRET_SSL_INSTALL);   

    return;
}
