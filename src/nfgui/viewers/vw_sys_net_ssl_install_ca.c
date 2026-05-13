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
static NFOBJECT *g_fload_obj[2] = {NULL,};
static NFOBJECT *g_fname_obj[2] = {NULL,};
static NFOBJECT *g_pass_obj = NULL;
static NFOBJECT *g_wait = NULL;
static gchar g_fullpath[2][512];



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
        
        if (obj == g_fload_obj[0])
                sprintf(title, "FIND PRIVATE KEY");
        else if (obj == g_fload_obj[1])
                sprintf(title, "FIND PUBLIC KEY");

        x = ((DISPLAY_ACTIVE_WIDTH - 852) / 2);
        y = ((DISPLAY_ACTIVE_HEIGHT - 598) / 2);
        
        ret = VW_SSL_Load_File_Open(g_curwnd, title, x, y, dir, fname, SSL_MODE_CA);
        if (!ret) return FALSE;

        if (strchr(fname, ' ')) {
            nftool_mbox(g_curwnd, "ERROR", "Files with spaces in the file name cannot be used.", NFTOOL_MB_OK);
            return FALSE;
        }

        if (obj == g_fload_obj[0]) {
            memset(g_fullpath[0], 0x00, sizeof(g_fullpath[0]));

            g_sprintf(g_fullpath[0], "%s/%s", dir, fname);
            g_message("###yanggungg : %s, %d, fullpath : %s", __FUNCTION__, __LINE__, g_fullpath[0]);

            if (scm_is_private_key(g_fullpath[0]) != 1) {
                nftool_mbox(g_curwnd, "ERROR", "This file is not a private key file.", NFTOOL_MB_OK);
                memset(g_fullpath[0], 0x00, sizeof(g_fullpath[0]));
                return FALSE;
            }
            
            nfui_nflabel_set_text((NFLABEL*)g_fname_obj[0], fname);
            nfui_signal_emit(g_fname_obj[0], GDK_EXPOSE, FALSE);

            if (scm_check_private_key_pass(g_fullpath[0])) {
                nfui_nflabel_set_text(g_pass_obj, "");
                nfui_nfobject_enable(g_pass_obj);
            }
            else {
                nfui_nflabel_set_text(g_pass_obj, "");
                nfui_nfobject_disable(g_pass_obj);
            }
            nfui_signal_emit(g_pass_obj, GDK_EXPOSE, TRUE);
        }
        else if (obj == g_fload_obj[1]) {
            memset(g_fullpath[1], 0x00, sizeof(g_fullpath[1]));

            g_sprintf(g_fullpath[1], "%s/%s", dir, fname);
            g_message("###yanggungg : %s, %d, fullpath : %s", __FUNCTION__, __LINE__, g_fullpath[1]);

            if (scm_is_public_key(g_fullpath[1]) != 1) {
                nftool_mbox(g_curwnd, "ERROR", "This file is not a public key file.", NFTOOL_MB_OK);
                memset(g_fullpath[1], 0x00, sizeof(g_fullpath[1]));
                return FALSE;
            }
            nfui_nflabel_set_text((NFLABEL*)g_fname_obj[1], fname);
            nfui_signal_emit(g_fname_obj[1], GDK_EXPOSE, FALSE);
        }
    }

    return FALSE;
}

static gboolean post_pw_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{
		gchar *pw;
		NFOBJECT *top;
		guint x, y;

		if(kpid == KEYPAD_ENTER)
		{
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);

			x += (obj->width)/2 + top->x;
			y += obj->height + top->y;
		}
		else
		{
	  	   	if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			  return FALSE;
	  	   	}
		
			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += ((GdkEventButton*)evt)->x;
			y += ((GdkEventButton*)evt)->y;
		}

		pw = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text(obj), x, y, 64, VKEY_PASSWORD);

		if(pw)
		{
			nfui_nflabel_set_text((NFLABEL*)obj, pw);

			ifree(pw);
			pw = NULL;
		}

		nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

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

        if (nfui_nflabel_get_strlen(g_fname_obj[0]) == 0) {
            nftool_mbox(g_curwnd, "ERROR", "Private key file could not be found.", NFTOOL_MB_OK);
            return FALSE;
        }
        else if (nfui_nflabel_get_strlen(g_fname_obj[1]) == 0) {
            nftool_mbox(g_curwnd, "ERROR", "Public key file could not be found.", NFTOOL_MB_OK);
            return FALSE;
        }
        else if (!nfui_nfobject_is_disabled(g_pass_obj) && nfui_nflabel_get_strlen(g_pass_obj) == 0) {
            nftool_mbox(g_curwnd, "ERROR", "Please enter your private key password.", NFTOOL_MB_OK);
            return FALSE;
        }

        memset(pw, 0x00, sizeof(pw));
        strcpy(pw, nfui_nflabel_get_text(g_pass_obj));
        ret = scm_ssl_install_certificate(g_fullpath, pw, IRET_SSL_INSTALL);
        if (ret == -1) {
            nftool_mbox(g_curwnd, "ERROR", "Certificate file could not be found.", NFTOOL_MB_OK);
            return FALSE;
        }
        else if (ret == -2) {
            nftool_mbox(g_curwnd, "ERROR", "It is not a valid private key file.", NFTOOL_MB_OK);
            return FALSE;
        }
        else if (ret == -3) {
            nftool_mbox(g_curwnd, "ERROR", "Private key password is incorrect.", NFTOOL_MB_OK);
            return FALSE;
        }
        else if (ret == -4) {
            nftool_mbox(g_curwnd, "ERROR", "Failed to copy the certificate file.", NFTOOL_MB_OK);
            return FALSE;
        }
        
        g_wait = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");

//      topwin = nfui_nfobject_get_top(obj);
//      nfui_nfobject_destroy(topwin);
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
            uxm_unreg_imsg_event(obj, IRET_SSL_INSTALL);    
        }
        break;

        case INFY_MEDIA_STATUS_CHANGED:
            nfui_nflabel_set_text((NFLABEL*)g_fname_obj[0], "");
            nfui_nflabel_set_text((NFLABEL*)g_fname_obj[1], "");
            nfui_nflabel_set_text((NFLABEL*)g_pass_obj, "");
            nfui_nfobject_disable(g_pass_obj);

            nfui_signal_emit(g_fname_obj[0], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_fname_obj[1], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_pass_obj, GDK_EXPOSE, TRUE);

            memset(g_fullpath, 0x00, sizeof(g_fullpath));
        break;

        case IRET_SSL_INSTALL:
        {
            gint ret = ((CMM_MESSAGE_T *)data)->param;
            
            if (g_wait) {
                nftool_remove_waitbox(g_wait);
                g_wait = NULL;
            }

            if (ret != 0) nftool_mbox(g_curwnd, "ERROR", "Failed to install SSL certificate.", NFTOOL_MB_OK);
            else {
                nftool_mbox(g_curwnd, "NOTICE", "Completed the installation of the SSL certificate.", NFTOOL_MB_OK);
                
                topwin = nfui_nfobject_get_top(obj);
                nfui_nfobject_destroy(topwin);
            }
        }
        break;

        default:
        break;
    }

    return FALSE;
}

void vw_ssl_installer_ca_page(NFOBJECT *parent)
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

    obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("Certificates issued through the certification authority can be installed.", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL *)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, SSL_DESC_W, SSL_LABEL_H);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)main_fixed, obj, pos_x, pos_y);

// <---- PRIVATE KEY
    pos_y += SSL_LABEL_H + 15;
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PRIVATE KEY", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, SSL_SUB_TITLE_W, SSL_LABEL_H);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

    pos_x += SSL_SUB_TITLE_W;
    
    obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 5);
    nfui_nfobject_set_size(obj, 400, SSL_LABEL_H);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
    g_fname_obj[0] = obj;

    pos_x += 400 + 2;
    
    obj = nftool_normal_button_create_popup_type1("...", 70);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_find_file_event_handler);
    g_fload_obj[0] = obj;
    
// <---- PRIVATE KEY PASSWORD
    pos_x = 18;
    pos_y += SSL_LABEL_H + 2;

    if (nftool_cur_language_is_japanese())
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PRIVATE KEY PASSWORD", nffont_get_pango_font(NFFONT_SMALL_SEMI_1), COLOR_IDX(292));
    else    
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PRIVATE KEY PASSWORD", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, SSL_SUB_TITLE_W, SSL_LABEL_H);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

    pos_x += SSL_SUB_TITLE_W;
    
    obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 5);
    nfui_nflabel_set_invisible((NFLABEL*)obj, TRUE);
    nfui_nfobject_set_size(obj, 400, SSL_LABEL_H);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_ON);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_pw_label_event_handler);
    nfui_nfobject_disable(obj);
    g_pass_obj = obj;

// <---- PUBLIC KEY
    pos_x = 18;
    pos_y += SSL_LABEL_H + 2;
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PUBLIC KEY", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, SSL_SUB_TITLE_W, SSL_LABEL_H);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

    pos_x += SSL_SUB_TITLE_W;
    
    obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 5);
    nfui_nfobject_set_size(obj, 400, SSL_LABEL_H);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
    g_fname_obj[1] = obj;

    pos_x += 400 + 2;
    obj = nftool_normal_button_create_popup_type1("...", 70);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_find_file_event_handler);
    g_fload_obj[1] = obj;


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
    
    uxm_reg_imsg_event(main_fixed, INFY_MEDIA_STATUS_CHANGED);  
    uxm_reg_imsg_event(main_fixed, IRET_SSL_INSTALL);   
    
    return;
}
