#include "support/event_loop.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"

#include "uxm.h"
#include "scm.h"

#include "vw_sys_net_ssl_dcv.h"
#include "vw_sys_net_ssl_load_file_popup.h"


#define SSL_WIN_SIZE_W                  (800)
#define SSL_WIN_SIZE_H                  (370)
#define SSL_WIN_POS_X                   ((DISPLAY_ACTIVE_WIDTH - SSL_WIN_SIZE_W) / 2)
#define SSL_WIN_POS_Y                   ((DISPLAY_ACTIVE_HEIGHT - SSL_WIN_SIZE_H) / 2)

#define SSL_DESC_W                      (SSL_WIN_SIZE_W - 20)
#define SSL_LABEL_H                     (40)
#define SSL_SUB_TITLE_W                 (150)

#define SSL_BTN_W                       (200)
#define SSL_CLOSE_BTN_X                 (SSL_WIN_SIZE_W - 10 - SSL_BTN_W)
#define SSL_INSTALL_BTN_X               (SSL_CLOSE_BTN_X - 2 - SSL_BTN_W)
#define SSL_BTN_Y                       (SSL_WIN_SIZE_H - 55)

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_fname_obj = NULL;
static gchar g_fullpath[512];



static gboolean post_find_file_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *top;
    gchar title[64];
    gchar dir[512];
    gchar fname[256];
    guint x, y;
    gboolean ret;
    
    
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        nfui_nfobject_get_window_pos(obj, &x, &y);

        x += ((GdkEventButton*)evt)->x;
        y += ((GdkEventButton*)evt)->y;

        if(!x && !y)
        {
            nfui_nfobject_get_offset(obj, &x, &y);
            top = nfui_nfobject_get_top(obj);

            x += (obj->width)/2 + top->x;
            y += obj->height + top->y;
        }

        memset(title, 0x00, sizeof(title));
        memset(dir, 0x00, sizeof(dir));
        memset(fname, 0x00, sizeof(fname));
        
        sprintf(title, "FIND DCV FILE");

        x = ((DISPLAY_ACTIVE_WIDTH - 852) / 2);
        y = ((DISPLAY_ACTIVE_HEIGHT - 598) / 2);
        
        ret = VW_SSL_Load_File_Open(g_curwnd, title, x, y, dir, fname, SSL_MODE_DCV);
        if (!ret) return FALSE;

        if (strchr(fname, ' ')) {
            nftool_mbox(g_curwnd, "ERROR", "Files with spaces in the file name cannot be used.", NFTOOL_MB_OK);
            return FALSE;
        }

        memset(g_fullpath, 0x00, sizeof(g_fullpath));

        g_sprintf(g_fullpath, "%s/%s", dir, fname);
        g_message("###yanggungg : %s, %d, fullpath : %s", __FUNCTION__, __LINE__, g_fullpath);
        
        nfui_nflabel_set_text((NFLABEL*)g_fname_obj, fname);
        nfui_signal_emit(g_fname_obj, GDK_EXPOSE, FALSE);
    }

    return FALSE;
}

static gboolean post_upload_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *topwin;
    gint ret = 0;

    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        if (nfui_nflabel_get_strlen(g_fname_obj) == 0) {
            nftool_mbox(g_curwnd, "ERROR", "File could not be found.", NFTOOL_MB_OK);
            return FALSE;
        }
        
        ret = scm_ssl_install_dcv_file(g_fullpath);
        
        if (ret == 0)
        {
            nftool_mbox(g_curwnd, "NOTICE", "The file has been successfully uploaded.\nPlease proceed with issuing SSL certificates.", NFTOOL_MB_OK);
            return FALSE;
        }
        else
        {
            nftool_mbox(g_curwnd, "ERROR", "File upload failed.", NFTOOL_MB_OK);
            return FALSE;
        }
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
            uxm_unreg_imsg_event(obj, INFY_MEDIA_STATUS_CHANGED);   
        }
        break;

        case INFY_MEDIA_STATUS_CHANGED:
            nfui_nflabel_set_text((NFLABEL*)g_fname_obj, "");

            nfui_signal_emit(g_fname_obj, GDK_EXPOSE, TRUE);

            memset(g_fullpath, 0x00, sizeof(g_fullpath));
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

void VW_SSL_Dcv_Open(NFOBJECT *parent)
{
    NFOBJECT *win;
    NFOBJECT *main_fixed;
    NFOBJECT *obj;

    gint pos_x, pos_y;


    win = nftool_create_popup_window((NFWINDOW*)parent, SSL_WIN_POS_X, SSL_WIN_POS_Y, SSL_WIN_SIZE_W, SSL_WIN_SIZE_H, "DCV FILE UPLOAD", FALSE);
    nfui_regi_post_event_callback(win, post_main_win_event_handler);
    g_curwnd = win;
    
    main_fixed = (NFWINDOW*)g_curwnd->child;
    nfui_regi_post_event_callback(main_fixed, post_main_fixed_event_handler);

// <---- PRIVATE KEY
    pos_x = 18;
    pos_y = 64;
    
    obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("Certificates issued through the certification authority can be installed.", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL *)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, SSL_DESC_W, SSL_LABEL_H);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)main_fixed, obj, pos_x, pos_y);

// <---- PRIVATE KEY
    pos_y += SSL_LABEL_H + 15;
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("FILE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, SSL_SUB_TITLE_W, SSL_LABEL_H);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

    pos_x += SSL_SUB_TITLE_W;
    
    obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 5);
    nfui_nfobject_set_size(obj, 500, SSL_LABEL_H);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
    g_fname_obj = obj;

    pos_x += 500 + 2;
    
    obj = nftool_normal_button_create_popup_type1("FIND", 100);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_find_file_event_handler);

// <---- INSTALL BUTTON
    obj = nftool_normal_button_create_type1("UPLOAD", SSL_BTN_W);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, SSL_INSTALL_BTN_X, SSL_BTN_Y);
    nfui_regi_post_event_callback(obj, post_upload_event_handler);

// <---- CLOSE BUTTON
    obj = nftool_normal_button_create_type1("CLOSE", SSL_BTN_W);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)main_fixed, obj, SSL_CLOSE_BTN_X, SSL_BTN_Y);
    nfui_regi_post_event_callback(obj, post_closebutton_event_handler);
    
    uxm_reg_imsg_event(main_fixed, INFY_MEDIA_STATUS_CHANGED);  

    nfui_nfobject_show(win);

    /* set for key navi */
    nfui_make_key_hierarchy((NFWINDOW *)win);

    gtk_main();
    
    return;
}
