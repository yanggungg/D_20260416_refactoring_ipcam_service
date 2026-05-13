#include "nf_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"

#include "tools/nf_ui_tool.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfimage.h"
#include "viewers/objects/nfbutton.h"

#include "vw_sys_user_set_email.h"
#include "vw_vkeyboard.h"
#include "vw_vkeyboard2.h"
#include "ix_mem.h"
#include "uxm.h"

#define SET_EMAIL_PP_SIZE_WID (760)
#define SET_EMAIL_PP_SIZE_HEI (280)
#define SET_EMAIL_PP_POS_X (guint)((DISPLAY_ACTIVE_WIDTH - SET_EMAIL_PP_SIZE_WID) / 2)
#define SET_EMAIL_PP_POS_Y (guint)((DISPLAY_ACTIVE_HEIGHT - SET_EMAIL_PP_SIZE_HEI) / 2 - 10)

#define PP_LABEL_SIZE_WID (320)
#define PP_LABEL_SIZE_HEI (40)
#define PP_VALUE_SIZE_WID (400)

static NFWINDOW *g_curwnd = 0;
static gint g_retVal = 0;
static NFOBJECT *g_n_email = NULL;
static NFOBJECT *g_c_email = NULL;
static gchar *g_out_email = NULL;

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_DELETE)
    {
        g_curwnd = 0;
        gtk_main_quit();
    }
    return FALSE;
}

static gboolean post_email_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkEventKey *kevt;
    KEYPAD_KID kpid = KEYPAD_NONE;

    if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        kevt = (GdkEventKey *)evt;
        kpid = (KEYPAD_KID)kevt->keyval;
    }

    if (evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
    {
        gchar *email = NULL;
        NFOBJECT *top;
        guint x, y;

        if (kpid == KEYPAD_ENTER)
        {
            nfui_nfobject_get_offset(obj, &x, &y);
            top = nfui_nfobject_get_top(obj);

            x += (obj->width) / 2 + top->x;
            y += obj->height + top->y;
        }
        else
        {
            if (evt->button.button == MOUSE_RIGTH_BUTTON)
            {
                return FALSE;
            }

            nfui_nfobject_get_window_pos(obj, &x, &y);

            x += ((GdkEventButton *)evt)->x;
            y += ((GdkEventButton *)evt)->y;
        }

        email = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text(obj), x, y, 63, VKEY_MAIL);

        if (email)
        {
            nfui_nflabel_set_text((NFLABEL *)obj, email);
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
            ifree(email);
            email = NULL;
        }
    }

    return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *topwin;

        if (evt->button.button == MOUSE_RIGTH_BUTTON)
        {
            return FALSE;
        }

        topwin = nfui_nfobject_get_top(obj);
        nfui_nfobject_destroy(topwin);
    }

    return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *topwin;

        if (evt->button.button == MOUSE_RIGTH_BUTTON)
        {
            return FALSE;
        }

        if (!strcmp(nfui_nflabel_get_text((NFLABEL *)g_n_email), ""))
        {
            nftool_mbox(g_curwnd, "ERROR", "Please input new e-mail.", NFTOOL_MB_OK);
            return FALSE;
        }

        if (!strcmp(nfui_nflabel_get_text((NFLABEL *)g_c_email), ""))
        {
            nftool_mbox(g_curwnd, "ERROR", "Please input confirm e-mail.", NFTOOL_MB_OK);
            return FALSE;
        }

        if (strcmp(nfui_nflabel_get_text((NFLABEL *)g_n_email), nfui_nflabel_get_text((NFLABEL *)g_c_email)))
        {
            nftool_mbox(g_curwnd, "ERROR", "E-mail doesn't match confirmation.", NFTOOL_MB_OK);
            return FALSE;
        }
        
        if (strcmp(nfui_nflabel_get_text((NFLABEL*)g_n_email), "") != 0)
        {
            if (nf_mail_send_check_email(nfui_nflabel_get_text((NFLABEL*)g_n_email)) == FALSE)
            {
                nftool_mbox(g_curwnd, "ERROR", "Please check the E-mail.", NFTOOL_MB_OK);
                return FALSE;
            }
        }

        g_stpcpy(g_out_email, nfui_nflabel_get_text((NFLABEL *)g_n_email));

        g_retVal = 1;

        topwin = nfui_nfobject_get_top(obj);
        nfui_nfobject_destroy(topwin);
    }

    return FALSE;
}

static gboolean returnkey_proc(NFOBJECT *top, GdkEvent *event, gpointer data)
{
    return FALSE;
}

gint VW_Open_Set_Email_Popup(NFWINDOW *parent, gchar *new_email, size_t new_email_len)
{
    NFOBJECT *main_wnd, *main_fixed;
    NFOBJECT *obj;
    guint pos_x, pos_y;
    int i;

    g_retVal = 0;
    g_out_email = new_email;
    memset(g_out_email, 0x00, new_email_len);

    main_wnd = nftool_create_popup_window(parent, SET_EMAIL_PP_POS_X, SET_EMAIL_PP_POS_Y, SET_EMAIL_PP_SIZE_WID, SET_EMAIL_PP_SIZE_HEI, "E-MAIL ADDRESS SETUP", TRUE);
    nfui_regi_post_event_callback(main_wnd, post_main_win_event_handler);
    g_curwnd = main_wnd;

    main_fixed = ((NFWINDOW *)main_wnd)->child;

    pos_y = 56;
    pos_x = 20;

    obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("NEW E-MAIL ADDRESS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL *)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, PP_LABEL_SIZE_WID, PP_LABEL_SIZE_HEI);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)main_fixed, obj, pos_x, pos_y);

    pos_x += PP_LABEL_SIZE_WID;

    obj = (NFOBJECT *)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL *)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nfobject_support_multi_lang((NFOBJECT *)obj, FALSE);
    nfui_nfobject_set_size(obj, PP_VALUE_SIZE_WID, PP_LABEL_SIZE_HEI);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)main_fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_email_label_event_handler);
    g_n_email = obj;

    pos_x = 20;
    pos_y += 43;

    obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("CONFIRM E-MAIL ADDRESS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL *)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, PP_LABEL_SIZE_WID, PP_LABEL_SIZE_HEI);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)main_fixed, obj, pos_x, pos_y);

    pos_x += PP_LABEL_SIZE_WID;

    obj = (NFOBJECT *)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL *)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nfobject_support_multi_lang((NFOBJECT *)obj, FALSE);
    nfui_nfobject_set_size(obj, PP_VALUE_SIZE_WID, PP_LABEL_SIZE_HEI);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)main_fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_email_label_event_handler);
    g_c_email = obj;

    pos_y = SET_EMAIL_PP_SIZE_HEI - 70;

    obj = nftool_normal_button_create_type1("APPLY", 120);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)main_fixed, obj, SET_EMAIL_PP_SIZE_WID / 2 - 125, pos_y);
    nfui_regi_post_event_callback(obj, post_applybutton_event_handler);

    obj = nftool_normal_button_create_type1("CANCEL", 120);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)main_fixed, obj, SET_EMAIL_PP_SIZE_WID / 2 + 5, pos_y);
    nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);

    nfui_nfobject_show(main_wnd);
    nfui_make_key_hierarchy(main_wnd);
    nfui_set_key_focus(obj, TRUE);

    gtk_main();

    return g_retVal;
}

gint VW_Close_Set_Email_Popup()
{
    if (!g_curwnd)
        return -1;

    evt_send_to_window("SET EMAIL POPUP", WND_CLOSE, 0, 0, 0);
    //	nfui_nfobject_destroy((NFOBJECT*)g_curwnd);
    return 0;
}
