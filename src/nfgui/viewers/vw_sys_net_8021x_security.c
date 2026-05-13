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
#include "objects/nfcombobox.h"

#include "vw_vkeyboard.h"
#include "vw_sys_net_security.h"
#include "vw_sys_net_8021x_security.h"
#include "vw_sys_net_ssl_load_file_popup.h"
#include "vw_sys_net_ssl_info.h"

static IEEE8021xData nacdata;
static IEEE8021xData org_nacdata;

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_waitpop = NULL;
static NFOBJECT *g_8021xuse = NULL;
static NFOBJECT *g_eap_type = NULL;
static NFOBJECT *g_eapol_ver = NULL;
static NFOBJECT *g_8021xid = NULL;
static NFOBJECT *g_8021xpw = NULL;
static NFOBJECT *g_lb_cert[3];
static NFOBJECT *g_find_cert[3];
static NFOBJECT *g_del_cert[3];
static NFOBJECT *g_info_cert[2];
static NFOBJECT *g_lb_cl_pw = NULL;



static void _set_cert_to_object(gint expose)
{
    gchar filename[256];
    gint len = 0;
    gint i;
    
    for (i = 0; i < CERT_TYPE_CNT; i++)
    {
        len = scm_8021x_get_cert_filename(i, filename, sizeof(filename));
        if (len) {
            nfui_nflabel_set_text((NFLABEL*)g_lb_cert[i], filename);
            if (expose) {
                nfui_signal_emit(g_lb_cert[i], GDK_EXPOSE, TRUE);
            }
        }
        else {
            nfui_nflabel_set_text((NFLABEL*)g_lb_cert[i], "No certificate.");
            
            if (expose) {
                nfui_signal_emit(g_lb_cert[i], GDK_EXPOSE, TRUE);
            }
        }
    }
}

static void _set_obj_by_cert(CERT_TYPE_E type, gint expose)
{
    gchar *cert_name;
    
    cert_name = nfui_nflabel_get_text((NFLABEL*)g_lb_cert[type]);
    
    if (strcmp(cert_name, "No certificate.")) {
        nfui_nfobject_enable(g_del_cert[type]);
        if (type != CERT_TYPE_CLIENT_KEY) nfui_nfobject_enable(g_info_cert[type]);
        
        if (expose) {
            nfui_signal_emit(g_del_cert[type], GDK_EXPOSE, TRUE);
            if (type != CERT_TYPE_CLIENT_KEY) nfui_signal_emit(g_info_cert[type], GDK_EXPOSE, TRUE);
        }
    }
    else {
        nfui_nfobject_disable(g_del_cert[type]);
        if (type != CERT_TYPE_CLIENT_KEY) nfui_nfobject_disable(g_info_cert[type]);
        
        if (expose) {
            nfui_signal_emit(g_del_cert[type], GDK_EXPOSE, TRUE);
            if (type != CERT_TYPE_CLIENT_KEY) nfui_signal_emit(g_info_cert[type], GDK_EXPOSE, TRUE);
        }
    }
}

static void _set_obj_by_use(gint use, gint expose)
{
    int i;
    
    if (use)
    {
        nfui_nfobject_enable(g_eap_type);
        nfui_nfobject_enable(g_eapol_ver);
        nfui_nfobject_enable(g_8021xid);
        nfui_nfobject_enable(g_8021xpw);
        
        for (i = 0; i < CERT_TYPE_CNT; i++) {
            nfui_nfobject_enable(g_find_cert[i]);
            _set_obj_by_cert(i, expose);
        }
            
        nfui_nfobject_enable(g_lb_cl_pw);
        
    }
    else
    {
        nfui_nfobject_disable(g_eap_type);
        nfui_nfobject_disable(g_eapol_ver);
        nfui_nfobject_disable(g_8021xid);
        nfui_nfobject_disable(g_8021xpw);
        
        for (i = 0; i < CERT_TYPE_CNT; i++) {
            nfui_nfobject_disable(g_find_cert[i]);
            nfui_nfobject_disable(g_del_cert[i]);
            if (i != CERT_TYPE_CLIENT_KEY) nfui_nfobject_disable(g_info_cert[i]);
        }
            
        nfui_nfobject_disable(g_lb_cl_pw);
    }
    
    if (expose)
    {
        nfui_signal_emit(g_eap_type, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_eapol_ver, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_8021xid, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_8021xpw, GDK_EXPOSE, TRUE);
        
        for (i = 0; i < CERT_TYPE_CNT; i++) {
            nfui_signal_emit(g_find_cert[i], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_del_cert[i], GDK_EXPOSE, TRUE);
            if (i != CERT_TYPE_CLIENT_KEY) nfui_signal_emit(g_info_cert[i], GDK_EXPOSE, TRUE);
        }
            
        nfui_signal_emit(g_lb_cl_pw, GDK_EXPOSE, TRUE);
    }
}

static void _get_val_from_object()
{
    nacdata.use = nfui_spin_button_get_index((NFSPINBUTTON*)g_8021xuse);
    nacdata.eap_type = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_eap_type);
    nacdata.eapol_ver = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_eapol_ver);
    strcpy(nacdata.id, nfui_nflabel_get_text((NFLABEL*)g_8021xid));
    strcpy(nacdata.pw, nfui_nflabel_get_text((NFLABEL*)g_8021xpw));
    strcpy(nacdata.prikey_pw, nfui_nflabel_get_text((NFLABEL*)g_lb_cl_pw));
}

static void _set_val_to_object(gint expose)
{
    nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)g_8021xuse, nacdata.use);
    nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_eap_type, nacdata.eap_type);
    nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_eapol_ver, nacdata.eapol_ver);
    nfui_nflabel_set_text((NFLABEL*)g_8021xid, nacdata.id);
    nfui_nflabel_set_text((NFLABEL*)g_8021xpw, nacdata.pw);
    nfui_nflabel_set_text((NFLABEL*)g_lb_cl_pw, nacdata.prikey_pw);
    
    if (expose)
    {
        nfui_signal_emit(g_8021xuse, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_eap_type, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_eapol_ver, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_8021xid, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_8021xpw, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_lb_cl_pw, GDK_EXPOSE, TRUE);
    }
}

static gboolean post_use_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        gint use;
        
        use = nfui_spin_button_get_index((NFSPINBUTTON*)obj);
        
        _set_obj_by_use(use, 1);
    }
    
    return FALSE;
}

static gboolean post_cert_info_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        SSL_CERT_INFO_T info;
        CERT_TYPE_E cert_type = 0;

        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
            return FALSE;
        }
            
        memset(&info, 0x00, sizeof(info));
        cert_type = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "CERT_TYPE"));

        scm_8021x_get_cert_info(cert_type, &info);

        VW_SSL_Information_Open(g_curwnd, &info);
    }
    
    return FALSE;
}

static gboolean post_delete_cert_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        CERT_TYPE_E cert_type = 0;
        mb_type ret = NFTOOL_MB_NONE;
        gint del = 0;
        
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
            return FALSE;
        }
        
        ret = nftool_mbox(g_curwnd, "WARNING", "Deleting a certificate may result in loss of network connectivity.\nWould you like to continue?", NFTOOL_MB_OKCANCEL);
        if (ret == NFTOOL_MB_CANCEL) return FALSE;
            
        cert_type = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "CERT_TYPE"));

        del = scm_8021x_delete_cert(cert_type);
        if (del) {
            _set_cert_to_object(1);
            _set_obj_by_cert(cert_type, 1);
        }
    }
    
    return FALSE;
}

static gboolean post_find_cert_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *top;
    gchar title[64];
    gchar dir[256];
    gchar fname[256];
    gchar fullpath[512];
    guint x, y;
    gint ret = 0;
    CERT_TYPE_E cert_type = 0;
    
    
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
        cert_type = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "CERT_TYPE"));
        
        if (cert_type == CERT_TYPE_CA)
            sprintf(title, "FIND CA CERTIFICATE");
        else if (cert_type == CERT_TYPE_CLIENT)
            sprintf(title, "FIND CLIENT CERTIFICATE");
        else
            sprintf(title, "FIND CLIENT KEY");

        x = ((DISPLAY_ACTIVE_WIDTH - 852) / 2);
        y = ((DISPLAY_ACTIVE_HEIGHT - 598) / 2);
        
        ret = VW_SSL_Load_File_Open(g_curwnd, title, x, y, dir, fname, 1);
        if (!ret) return FALSE;

        if (strchr(fname, ' ')) {
            nftool_mbox(g_curwnd, "ERROR", "Files with spaces in the file name cannot be used.", NFTOOL_MB_OK);
            return FALSE;
        }

        memset(fullpath, 0x00, sizeof(fullpath));

        g_sprintf(fullpath, "%s/%s", dir, fname);
        g_message("###yanggungg : %s, %d, fullpath : %s", __FUNCTION__, __LINE__, fullpath);
        
        ret = scm_8021x_upload_cert_temp(fullpath, cert_type);
        if (ret < 0) {
            nftool_mbox(g_curwnd, "ERROR", "The certificate could not be found.", NFTOOL_MB_OK);
            return FALSE;
        }
        
        nfui_nflabel_set_text((NFLABEL*)g_lb_cert[cert_type], fname);
        nfui_signal_emit(g_lb_cert[cert_type], GDK_EXPOSE, TRUE);
        
        nfui_nfobject_enable(g_del_cert[cert_type]);
        nfui_signal_emit(g_del_cert[cert_type], GDK_EXPOSE, TRUE);
        
        if (g_info_cert[cert_type]) {
            nfui_nfobject_enable(g_info_cert[cert_type]);
            nfui_signal_emit(g_info_cert[cert_type], GDK_EXPOSE, TRUE);
        }
    }

    return FALSE;
}

static gboolean post_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkEventKey *kevt;
    KEYPAD_KID kpid = KEYPAD_NONE;

    if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;
    }
    else if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER )
    {
        NFOBJECT *top;
        gchar *str;
        guint x,y;

        if(kpid == KEYPAD_ENTER)
        {
            nfui_nfobject_get_offset(obj, &x, &y);
            top = nfui_nfobject_get_top(obj);

            x += (obj->width)/2 + top->x;
            y += obj->height + top->y;
        }
        else
        {
            if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

            nfui_nfobject_get_window_pos(obj, &x,&y);

            x += ((GdkEventButton*)evt)->x;
            y += ((GdkEventButton*)evt)->y;

            str = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text(obj), x, y, 32, VKEY_NORMAL);

            if(str)
            {
                nfui_nflabel_set_text(obj, str);

                ifree(str);
                str = NULL;
            }

            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
        }
    }
    
    return FALSE;
}

static gboolean post_cancel_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        gint i;
        
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
            return FALSE;
        }

        _get_val_from_object();

        if(memcmp(&org_nacdata, &nacdata, sizeof(IEEE8021xData)))
        {
            if (nacdata.use != org_nacdata.use) {
                _set_obj_by_use(org_nacdata.use, 1);
            }
            g_memmove(&nacdata, &org_nacdata, sizeof(IEEE8021xData));
            _set_val_to_object(1);
        }
        
        if (scm_8021x_check_changed())
        {
            scm_8021x_cancel_cert();
            _set_cert_to_object(1);
            
            for (i = 0; i < CERT_TYPE_CNT; i++)
                _set_obj_by_cert(i, 1);
        }
    }

    return FALSE;
}

static gboolean post_apply_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
            return FALSE;
        }
        
        gint is_changed = 0;
        gint ret = 0;
    
        _get_val_from_object();

        if(memcmp(&org_nacdata, &nacdata, sizeof(IEEE8021xData)))
        {
            g_memmove(&org_nacdata, &nacdata, sizeof(IEEE8021xData));
            DAL_set_8021x_data(&nacdata);

            sysnet_set_changeflag(1);
            
            is_changed = 1;
        }
        
        if (scm_8021x_check_changed()) {
            scm_8021x_apply_cert();
            is_changed = 1;
        }
        
        if (is_changed) {
            g_waitpop = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "Network server is restarting.\nPlease wait...", "");
            scm_apply_netinfo_by_db(IRET_SCM_APPLY_NETINFO);
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
    if (evt->type == GDK_DELETE)
    {
        uxm_unreg_imsg_event(obj, IRET_SCM_APPLY_NETINFO);
        uxm_unreg_imsg_event(obj, IRET_SCM_APPLY_NETINFO2);
    }
    else if (evt->type == IRET_SCM_APPLY_NETINFO || evt->type == IRET_SCM_APPLY_NETINFO2)
    {
        if (g_waitpop) {
            nftool_remove_waitbox(g_waitpop);
            g_waitpop = NULL;
		}
        
        if (evt->type == IRET_SCM_APPLY_NETINFO) nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
        if (evt->type == IRET_SCM_APPLY_NETINFO2) gtk_main_quit();
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

void VW_Init_Security_8021x_Page(NFOBJECT *parent)
{
    NFOBJECT *content_fixed;
    NFOBJECT *obj;

    gchar *strOffOn[] = {"OFF", "ON"};
    gchar *strEaptype[] = {"EAP-TLS", "EAP-MD5"};
    gchar *strEapol_ver[] = {"1", "2", "3"};

    gint pos_y, pos_x;
    guint i;

    guint inactive_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(900), COLOR_IDX(901), COLOR_IDX(902), COLOR_IDX(904)};
    guint active_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(903), COLOR_IDX(901), COLOR_IDX(902), COLOR_IDX(904)};

    g_waitpop = NULL;
    g_curwnd = nfui_nfobject_get_top(parent);

    memset(&nacdata, 0x00, sizeof(IEEE8021xData));
    memset(&org_nacdata, 0x00, sizeof(IEEE8021xData));

    DAL_get_8021x_data(&nacdata);
    g_memmove(&org_nacdata, &nacdata, sizeof(IEEE8021xData));
    
    /* FIXED */
    content_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
    nfui_nfobject_show(content_fixed);
    nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);

    pos_x = 27;
    pos_y = 13;
    
    /* TITLE BAR */
    obj = nfui_nfimage_new(IMG_TITLE_BG2);
    nfui_nfimage_set_text((NFIMAGE*)obj, "IEEE 802.1X SETTINGS");
    nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
    nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

    pos_y += 40 + 21;
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("IEEE 802.1X", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nfobject_set_size(obj, 400, 40);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

    obj = (NFOBJECT*)nfui_spinbutton_new((gchar**)strOffOn, 2, nacdata.use);
    nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_1);
    nfui_spin_button_set_align((NFSPINBUTTON*)obj, NFALIGN_CENTER, 0);
    nfui_spin_button_set_spacing((NFSPINBUTTON*)obj, CONDENSED_SPACING);
    nfui_nfobject_set_size(obj, 400, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x + 400, pos_y);
    nfui_regi_post_event_callback(obj, post_use_event_handler);
    g_8021xuse = obj;

    pos_y += 40 + 2;
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("EAP TYPE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nfobject_set_size(obj, 400, 40);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
    
    obj = (NFOBJECT*)nfui_combobox_new(strEaptype, 2, nacdata.eap_type);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);	
    nfui_nfobject_set_size(obj, 400, 40);
	nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x + 400, pos_y);
    // nfui_regi_post_event_callback(obj, post_personal_combo_event_handler);
    g_eap_type = obj;

    pos_y += 40 + 2;
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("EAPOL VERSION", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nfobject_set_size(obj, 400, 40);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
    
    obj = (NFOBJECT*)nfui_combobox_new(strEapol_ver, 2, nacdata.eapol_ver);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);	
    nfui_nfobject_set_size(obj, 400, 40);
	nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x + 400, pos_y);
    // nfui_regi_post_event_callback(obj, post_personal_combo_event_handler);
    g_eapol_ver = obj;

    pos_y += 40 + 2;
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ID", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nfobject_set_size(obj, 400, 40);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
    
    obj = nfui_nflabel_new_text_box(nacdata.id, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
    nfui_nflabel_set_spacing((NFLABEL*)obj, NORMAL_SPACING);
    nfui_nfobject_set_size(obj, 400, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x + 400, pos_y);
    nfui_regi_post_event_callback(obj, post_label_event_handler);
    g_8021xid = obj;

    pos_y += 40 + 2;
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PASSWORD", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nfobject_set_size(obj, 400, 40);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
    
    obj = nfui_nflabel_new_text_box(nacdata.pw, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
    nfui_nflabel_set_spacing((NFLABEL*)obj, NORMAL_SPACING);
    nfui_nflabel_set_invisible((NFLABEL*)obj, 1);
    nfui_nfobject_set_size(obj, 400, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x + 400, pos_y);
    nfui_regi_post_event_callback(obj, post_label_event_handler);
    g_8021xpw = obj;
    
//CERTIFICATE SETTINGS
    pos_y += 40 + 80;
    
    obj = nfui_nfimage_new(IMG_TITLE_BG2);
    nfui_nfimage_set_text((NFIMAGE*)obj, "CERTIFICATE SETTINGS");
    nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
    nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
    
    pos_y += 40 + 21;
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CA CERTIFICATE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nfobject_set_size(obj, 400, 40);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
    
    obj = nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
    nfui_nflabel_set_spacing((NFLABEL*)obj, NORMAL_SPACING);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 400, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x + 400, pos_y);
    g_lb_cert[CERT_TYPE_CA] = obj;
    
    obj = nftool_normal_button_create_subtab_type1("FIND", 100);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x + 800 + 2, pos_y);
    nfui_regi_post_event_callback(obj, post_find_cert_event_handler);
    nfui_nfobject_set_data(obj, "CERT_TYPE", GINT_TO_POINTER(CERT_TYPE_CA));
    g_find_cert[CERT_TYPE_CA] = obj;
        
    obj = nftool_normal_button_create_subtab_type1("DEL", 100);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x + 900 + 3, pos_y);
    nfui_regi_post_event_callback(obj, post_delete_cert_event_handler);
    nfui_nfobject_set_data(obj, "CERT_TYPE", GINT_TO_POINTER(CERT_TYPE_CA));
    g_del_cert[CERT_TYPE_CA] = obj;
        
    obj = nftool_normal_button_create_subtab_type1("INFO", 100);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x + 1000 + 4, pos_y);
    nfui_regi_post_event_callback(obj, post_cert_info_event_handler);
    nfui_nfobject_set_data(obj, "CERT_TYPE", GINT_TO_POINTER(CERT_TYPE_CA));
    g_info_cert[CERT_TYPE_CA] = obj;

    pos_y += 40 + 2;
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CLIENT CERTIFICATE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nfobject_set_size(obj, 400, 40);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
    
    obj = nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
    nfui_nflabel_set_spacing((NFLABEL*)obj, NORMAL_SPACING);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 400, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x + 400, pos_y);
    g_lb_cert[CERT_TYPE_CLIENT] = obj;
        
    obj = nftool_normal_button_create_subtab_type1("FIND", 100);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x + 800 + 2, pos_y);
    nfui_regi_post_event_callback(obj, post_find_cert_event_handler);
    nfui_nfobject_set_data(obj, "CERT_TYPE", GINT_TO_POINTER(CERT_TYPE_CLIENT));
    g_find_cert[CERT_TYPE_CLIENT] = obj;

    obj = nftool_normal_button_create_subtab_type1("DEL", 100);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x + 900 + 3, pos_y);
    nfui_regi_post_event_callback(obj, post_delete_cert_event_handler);
    nfui_nfobject_set_data(obj, "CERT_TYPE", GINT_TO_POINTER(CERT_TYPE_CLIENT));
    g_del_cert[CERT_TYPE_CLIENT] = obj;

    obj = nftool_normal_button_create_subtab_type1("INFO", 100);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x + 1000 + 4, pos_y);
    nfui_regi_post_event_callback(obj, post_cert_info_event_handler);
    nfui_nfobject_set_data(obj, "CERT_TYPE", GINT_TO_POINTER(CERT_TYPE_CLIENT));
    g_info_cert[CERT_TYPE_CLIENT] = obj;

    pos_y += 40 + 2;
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CLIENT KEY", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nfobject_set_size(obj, 400, 40);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
    
    obj = nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
    nfui_nflabel_set_spacing((NFLABEL*)obj, NORMAL_SPACING);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 400, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x + 400, pos_y);
    g_lb_cert[CERT_TYPE_CLIENT_KEY] = obj;
        
    obj = nftool_normal_button_create_subtab_type1("FIND", 100);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x + 800 + 2, pos_y);
    nfui_regi_post_event_callback(obj, post_find_cert_event_handler);
    nfui_nfobject_set_data(obj, "CERT_TYPE", GINT_TO_POINTER(CERT_TYPE_CLIENT_KEY));
    g_find_cert[CERT_TYPE_CLIENT_KEY] = obj;

    obj = nftool_normal_button_create_subtab_type1("DEL", 100);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x + 900 + 3, pos_y);
    nfui_regi_post_event_callback(obj, post_delete_cert_event_handler);
    nfui_nfobject_set_data(obj, "CERT_TYPE", GINT_TO_POINTER(CERT_TYPE_CLIENT_KEY));
    g_del_cert[CERT_TYPE_CLIENT_KEY] = obj;

    pos_y += 40 + 2;
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CLIENT KEY PASSWORD", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nfobject_set_size(obj, 400, 40);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
    
    obj = nfui_nflabel_new_text_box(nacdata.prikey_pw, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
    nfui_nflabel_set_spacing((NFLABEL*)obj, NORMAL_SPACING);
    nfui_nflabel_set_invisible((NFLABEL*)obj, 1);
    nfui_nfobject_set_size(obj, 400, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x + 400, pos_y);
    nfui_regi_post_event_callback(obj, post_label_event_handler);
    g_lb_cl_pw = obj;
    
    // if (strlen(nacdata.prikey_pw)) {
    //     nfui_nfobject_enable(obj);
    // }
    // else {
    //     nfui_nfobject_disable(obj);
    // }


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
    
    uxm_reg_imsg_event(content_fixed, IRET_SCM_APPLY_NETINFO);
	uxm_reg_imsg_event(content_fixed, IRET_SCM_APPLY_NETINFO2);
    
    _set_cert_to_object(0);
    _set_obj_by_use(nacdata.use, 0);
}

gboolean check_net_security_8021x_changed()
{
    _get_val_from_object();
    
    if(memcmp(&org_nacdata, &nacdata, sizeof(IEEE8021xData)))
        return TRUE;
    
    if (scm_8021x_check_changed())
        return TRUE;

    return FALSE;
}

void save_net_security_8021x_data()
{
    g_memmove(&org_nacdata, &nacdata, sizeof(IEEE8021xData));
    DAL_set_8021x_data(&nacdata);
    
    scm_8021x_apply_cert();
    
    g_waitpop = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "Network server is restarting.\nPlease wait...", "");
    scm_apply_netinfo_by_db(IRET_SCM_APPLY_NETINFO2);

    gtk_main();
}

void restore_net_security_8021x_data()
{
    if (nacdata.use != org_nacdata.use) {
        _set_obj_by_use(org_nacdata.use, 0);
    }

    g_memmove(&nacdata, &org_nacdata, sizeof(IEEE8021xData));
    _set_val_to_object(0);
    
    scm_8021x_cancel_cert();
    _set_cert_to_object(0);
}
