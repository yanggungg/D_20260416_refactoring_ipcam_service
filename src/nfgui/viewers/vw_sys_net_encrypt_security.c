
#include "nf_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "scm.h"

#include "objects/nffixed.h"
#include "objects/nflabel.h"
#include "objects/nfbutton.h"
#include "objects/nfimage.h"
#include "objects/nfspinbutton.h"

#include "vw_sys_net_security.h"
#include "vw_sys_net_ssl_install_popup.h"
#include "vw_sys_net_ssl_info.h"
#include "vw_sys_net_ssl_dcv.h"

#define ENCRYPT_TITLE_LEFT          (27)

enum{
    ENCRYPT_ROW_ENABLE= 0,
    ENCRYPT_ROW_METHOD,
    //
    ENCRYPT_ROW_HTTPS,
    ENCRYPT_ROW_HTTPS_METHOD,
    //
    ENCRYPT_ROW_NUM
};

enum{
    METHOD_SEED_128 = 0,

    METHOD_NUM
};

static EncryptData encryptdata;
static EncryptData org_encryptdata;

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *value_object[ENCRYPT_ROW_NUM];
static NFOBJECT *g_waitpop = NULL;

static const gchar* strMethod[METHOD_NUM] = {
    "SEED_128",
};

static gint convMethodToIndex(gchar *strIndex)
{
    guint i = 0;

    for(i=0 ; i< METHOD_NUM; i++)
    {
        if(!strcmp(strIndex, strMethod[i]))
        {
            return i;
        }
    }
    return 0;
}

static gboolean post_ssl_upload_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
            return FALSE;
        }
        
        VW_SSL_Dcv_Open(g_curwnd);
    }
    
    return FALSE;
}

static gboolean post_ssl_info_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        SSL_CERT_INFO_T info;
        
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
            return FALSE;
        }
        
        if (!scm_ssl_is_exist_certificate())
        {
            nftool_mbox(g_curwnd, "ERROR", "No SSL certificate was found.\nPlease install the SSL certificate.", NFTOOL_MB_OK);
            return FALSE;
        }
                    
        memset(&info, 0x00, sizeof(info));
        scm_ssl_get_cert_info(&info);
        
        VW_SSL_Information_Open(g_curwnd, &info);
    }
    
    return FALSE;
}

static gboolean post_ssl_install_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
            return FALSE;
        }
        
        VW_SSL_Installer_Open(g_curwnd);
    }
    
    return FALSE;
}

static gboolean post_ssl_delete_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    mb_type ret;
    
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
            return FALSE;
        }

        ret = nftool_mbox(g_curwnd, "CONFIRM", "Are you sure to delete the SSL certificate?", NFTOOL_MB_OKCANCEL);
        if (ret == NFTOOL_MB_CANCEL) return FALSE;

        if (scm_ssl_delete_certificate(IRET_SSL_DELETE) == -1) {
            nftool_mbox(g_curwnd, "ERROR", "Self-signed certificates cannot be deleted.", NFTOOL_MB_OK);
            return FALSE;
        }

        g_waitpop = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
    }
    
    return FALSE;
}

static gboolean post_cancel_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        guint i;

        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
            return FALSE;
        }

        encryptdata.srtspon = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)(value_object[ENCRYPT_ROW_ENABLE]));
//      g_stpcpy(encryptdata.srtsp_method, nfui_spin_button_get_text((NFSPINBUTTON*)(value_object[ENCRYPT_ROW_METHOD])));
        encryptdata.httpson = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)(value_object[ENCRYPT_ROW_HTTPS]));
        encryptdata.httpauth_method = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)(value_object[ENCRYPT_ROW_HTTPS_METHOD]));

        if(memcmp(&org_encryptdata, &encryptdata, sizeof(EncryptData)))
        {
            g_memmove(&encryptdata, &org_encryptdata, sizeof(EncryptData));

            nfui_spin_button_set_index((NFSPINBUTTON*)(value_object[ENCRYPT_ROW_ENABLE]), encryptdata.srtspon);
//          nfui_spin_button_set_text((NFSPINBUTTON*)(value_object[ENCRYPT_ROW_METHOD]), encryptdata.srtsp_method);
            nfui_spin_button_set_index((NFSPINBUTTON*)(value_object[ENCRYPT_ROW_HTTPS]), encryptdata.httpson);
            nfui_spin_button_set_index((NFSPINBUTTON*)(value_object[ENCRYPT_ROW_HTTPS_METHOD]), encryptdata.httpauth_method);

            for(i=0; i<ENCRYPT_ROW_NUM; i++)
            {
                nfui_signal_emit(value_object[i], GDK_EXPOSE, TRUE);
            }
        }

    }

    return FALSE;
}

static gboolean post_apply_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        guint index;

        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
            return FALSE;
        }

        encryptdata.srtspon = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)(value_object[ENCRYPT_ROW_ENABLE]));
//      g_stpcpy(encryptdata.srtsp_method, nfui_spin_button_get_text((NFSPINBUTTON*)(value_object[ENCRYPT_ROW_METHOD])));
        encryptdata.httpson = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)(value_object[ENCRYPT_ROW_HTTPS]));
        encryptdata.httpauth_method = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)(value_object[ENCRYPT_ROW_HTTPS_METHOD]));

        if(memcmp(&org_encryptdata, &encryptdata, sizeof(EncryptData)))
        {
            g_memmove(&org_encryptdata, &encryptdata, sizeof(EncryptData));
            DAL_set_encrypt_data(&encryptdata);

            nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");

            sysnet_set_changeflag(1);

        }
    }

    return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
            return FALSE;
        }

        NetSecurity_tab_out_handler();
        SystemSetupNetwork_Destroy(obj);
    }

    return FALSE;
}

static gboolean post_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_DELETE)
    {
        g_curwnd = 0;

        uxm_unreg_imsg_event(obj, IRET_SSL_DELETE);
    }
    else if (evt->type == IRET_SSL_DELETE)
    {
        if (g_waitpop) {
            nftool_remove_waitbox(g_waitpop);
            g_waitpop = NULL;
        }
        
        nftool_mbox(g_curwnd, "NOTICE", "SSL certificate has been deleted.", NFTOOL_MB_OK);
    }

    return FALSE;
}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_DELETE)
    {
        g_curwnd = 0;
    }

    return FALSE;
}


void VW_Init_Security_Encryption_Page(NFOBJECT *parent)
{
    NFOBJECT *content_fixed;
    NFOBJECT *obj;
    NFOBJECT *subject_object[ENCRYPT_ROW_NUM];

    const gchar *title_name[] = {"RTSP SERVICE PORT", "WEB SERVICE PORT"};
    const gchar *strTitle[] = {
                    "RTSP ENCRYPTION ENABLE",
                    "RTSP ENCRYPTION METHOD",
                    
                    "HTTPS ENABLE",
                    "HTTP AUTHENTICATION METHOD"
    };

    gint title_ypos[2] = { 13, 245};
    gint label_ypos[4] = { 74, 116, 319, 361};
    gint pos_y;

    const gchar *strOffOn[] = {"OFF", "ON"};
    const gchar *strMethod2[] = {"BASIC", "DIGEST"};

    guint i;

    guint inactive_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(900), COLOR_IDX(901), COLOR_IDX(902), COLOR_IDX(904)};
    guint active_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(903), COLOR_IDX(901), COLOR_IDX(902), COLOR_IDX(904)};

    g_waitpop = NULL;
    g_curwnd = nfui_nfobject_get_top(parent);

    /* FIXED */
    content_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
    nfui_nfobject_show(content_fixed);
    nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);

    memset(&encryptdata, 0x00, sizeof(EncryptData));
    memset(&org_encryptdata, 0x00, sizeof(EncryptData));

    DAL_get_encrypt_data(&encryptdata);
    g_memmove(&org_encryptdata, &encryptdata, sizeof(EncryptData));

    /* TITLE BAR */
    for( i = 0 ; i < 2 ; i++ )
    {
        obj = nfui_nfimage_new(IMG_TITLE_BG2);
        nfui_nfimage_set_text((NFIMAGE*)obj, title_name[i]);
        nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
        nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)content_fixed, obj, ENCRYPT_TITLE_LEFT, title_ypos[i]);
    }

    value_object[ENCRYPT_ROW_ENABLE] = (NFOBJECT*)nfui_spinbutton_new((gchar**)strOffOn, 2, (gint)encryptdata.srtspon);
    value_object[ENCRYPT_ROW_HTTPS] = (NFOBJECT*)nfui_spinbutton_new((gchar**)strOffOn, 2, (gint)encryptdata.httpson);
    value_object[ENCRYPT_ROW_HTTPS_METHOD]  = (NFOBJECT*)nfui_spinbutton_new((gchar**)strMethod2, 2, encryptdata.httpauth_method);

    value_object[ENCRYPT_ROW_METHOD] = (NFOBJECT*)nfui_nflabel_new_text_box(strMethod[METHOD_SEED_128], nffont_get_pango_font(NFFONT_MEDIUM_SEMI));

    for(i = 0 ; i < ENCRYPT_ROW_NUM; i++)
    {
        subject_object[i] = (NFOBJECT*)nfui_nflabel_new_with_pango_font((gchar*)strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
        nfui_nfobject_set_size(subject_object[i], 400, 40);
        nfui_nflabel_set_align((NFLABEL*)subject_object[i], NFALIGN_LEFT, 20);
        nfui_nfobject_modify_bg(subject_object[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
        nfui_nfobject_use_focus(subject_object[i], NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(subject_object[i]);
        nfui_nffixed_put((NFFIXED*)content_fixed, subject_object[i], ENCRYPT_TITLE_LEFT, label_ypos[i]);

        //value
        if (i == ENCRYPT_ROW_METHOD)
        {
            nfui_nflabel_set_skin_type((NFLABEL*)value_object[i], NFTEXTBOX_TYPE_SUBTAB_OUTPUT);
            nfui_nflabel_set_align((NFLABEL*)value_object[i], NFALIGN_CENTER, 0);
            nfui_nfobject_use_focus(value_object[i], NFOBJECT_FOCUS_OFF);
        }
        else
        {
        nfui_spinbutton_set_skin_type((NFSPINBUTTON*)value_object[i], NFSPINBUTTON_TYPE_1);
        nfui_spin_button_set_align((NFSPINBUTTON*)value_object[i], NFALIGN_CENTER, 0);
        nfui_spin_button_set_spacing((NFSPINBUTTON*)value_object[i], CONDENSED_SPACING);
        }
        
        // skshin
//      if (i == ENCRYPT_ROW_HTTPS_METHOD) nfui_nfobject_support_multi_lang(value_object[i], FALSE);
        
        nfui_nfobject_set_size(value_object[i], 250, 40);
        nfui_nfobject_show(value_object[i]);
        nfui_nffixed_put((NFFIXED*)content_fixed, value_object[i], ENCRYPT_TITLE_LEFT +400, label_ypos[i]);
    }

    obj = nfui_nfimage_new(IMG_TITLE_BG2);
    nfui_nfimage_set_text((NFIMAGE*)obj, "SSL CERTIFICATE");
    nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
    nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, ENCRYPT_TITLE_LEFT, 490);
        
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("WEB SERVER TYPE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nfobject_set_size(obj, 400, 40);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, ENCRYPT_TITLE_LEFT, 551);

#if defined(_IPX_MODEL_UX)    
    obj = (NFOBJECT*)nfui_nflabel_new_text_box("NGINX", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
#else   
    obj = (NFOBJECT*)nfui_nflabel_new_text_box("LIGHTTPD", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
#endif  
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_SUBTAB_OUTPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 502, 40);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, ENCRYPT_TITLE_LEFT + 400, 551);

    pos_y = 593 + 15;
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("INSTALL / DELETE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nfobject_set_size(obj, 400, 40);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, ENCRYPT_TITLE_LEFT, pos_y);

    obj = nftool_normal_button_create_subtab_type1("INSTALL", 250);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, ENCRYPT_TITLE_LEFT + 400, pos_y);
    nfui_regi_post_event_callback(obj, post_ssl_install_event_handler);

    obj = nftool_normal_button_create_subtab_type1("DELETE", 250);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, ENCRYPT_TITLE_LEFT + 400 + 2 + 250, pos_y);
    nfui_regi_post_event_callback(obj, post_ssl_delete_event_handler);

    pos_y += 42;

    obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("INFORMATION", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nfobject_set_size(obj, 400, 40);
    nfui_nflabel_set_align((NFLABEL *)obj, NFALIGN_LEFT, 20);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)content_fixed, obj, ENCRYPT_TITLE_LEFT, pos_y);

    obj = nftool_normal_button_create_subtab_type1("SHOW", 250);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)content_fixed, obj, ENCRYPT_TITLE_LEFT + 400, pos_y);
    nfui_regi_post_event_callback(obj, post_ssl_info_event_handler);

    pos_y += 42;

    obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("DCV(HTTP)", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nfobject_set_size(obj, 400, 40);
    nfui_nflabel_set_align((NFLABEL *)obj, NFALIGN_LEFT, 20);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)content_fixed, obj, ENCRYPT_TITLE_LEFT, pos_y);

    obj = nftool_normal_button_create_subtab_type1("UPLOAD", 250);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)content_fixed, obj, ENCRYPT_TITLE_LEFT + 400, pos_y);
    nfui_regi_post_event_callback(obj, post_ssl_upload_event_handler);

    /* button */
    obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R3_X, MENU_V_SUBTAB_BTN_Y);
    nfui_regi_post_event_callback(obj, post_cancel_button_event_handler);

    obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R2_X, MENU_V_SUBTAB_BTN_Y);
    nfui_regi_post_event_callback(obj, post_apply_button_event_handler);

    obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R1_X, MENU_V_SUBTAB_BTN_Y);
    nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

    nfui_regi_post_event_callback(parent, post_page_event_handler);
    nfui_regi_post_event_callback(content_fixed, post_fixed_event_handler);

    uxm_reg_imsg_event(content_fixed, IRET_SSL_DELETE);
}

gboolean check_net_security_encryption_changed()
{
    encryptdata.srtspon = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)(value_object[ENCRYPT_ROW_ENABLE]));
//  g_stpcpy(encryptdata.srtsp_method, nfui_spin_button_get_text((NFSPINBUTTON*)(value_object[ENCRYPT_ROW_METHOD])));
    encryptdata.httpson = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)(value_object[ENCRYPT_ROW_HTTPS]));
    encryptdata.httpauth_method = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)(value_object[ENCRYPT_ROW_HTTPS_METHOD]));

    if(memcmp(&org_encryptdata, &encryptdata, sizeof(EncryptData)))
        return TRUE;

    return FALSE;
}

void save_net_security_encryption_data()
{
    encryptdata.srtspon = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)(value_object[ENCRYPT_ROW_ENABLE]));
//  g_stpcpy(encryptdata.srtsp_method, nfui_spin_button_get_text((NFSPINBUTTON*)(value_object[ENCRYPT_ROW_METHOD])));
    encryptdata.httpson = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)(value_object[ENCRYPT_ROW_HTTPS]));
    encryptdata.httpauth_method = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)(value_object[ENCRYPT_ROW_HTTPS_METHOD]));

    if(memcmp(&org_encryptdata, &encryptdata, sizeof(EncryptData)))
    {
            g_memmove(&org_encryptdata, &encryptdata, sizeof(EncryptData));
            DAL_set_encrypt_data(&encryptdata);
    }
}

void restore_net_security_encryption_data()
{
    g_memmove(&encryptdata, &org_encryptdata, sizeof(EncryptData));

    nfui_spin_button_set_index((NFSPINBUTTON*)(value_object[ENCRYPT_ROW_ENABLE]), encryptdata.srtspon);
//  nfui_spin_button_set_text((NFSPINBUTTON*)(value_object[ENCRYPT_ROW_METHOD]), encryptdata.srtsp_method);
    nfui_spin_button_set_index((NFSPINBUTTON*)(value_object[ENCRYPT_ROW_HTTPS]), encryptdata.httpson);
    nfui_spin_button_set_index((NFSPINBUTTON*)(value_object[ENCRYPT_ROW_HTTPS_METHOD]), encryptdata.httpauth_method);
}
