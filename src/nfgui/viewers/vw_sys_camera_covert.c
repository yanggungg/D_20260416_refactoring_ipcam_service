#include <string.h>
#include "nf_afx.h"
#include "vw_sys_camera_main.h"
#include "vw_sys_camera_covert.h"

#include "nf_notify.h"

#include "tools/nf_ui_tool.h"

#include "support/color.h"
#include "support/nf_ui_font.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"

#include "objects/nfobject.h"
#include "objects/nfbutton.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfimage.h"
#include "objects/nfimglabel.h"
#include "objects/nfspinbutton.h"
#include "objects/nfcombobox.h"

#include "vw_vkeyboard.h"
#include "nfdal.h"
#include "scm.h"

#define PAGE_FIXED_CNT          4

#define CAM_SETUP_COLUMNS       5
#define CAM_SETUP_ROWS          8

#define CAM_SETUP_TABLE_LEFT    (8)
#define CAM_SETUP_TABLE_TOP     (39)

#define CAM_SETUP_LABEL_HEIGHT  (40)

enum {
    CSB_CANCEL = 0,
    CSB_APPLY,
    CSB_CLOSE,
    CSB_BUTTONS
};

#define GROUP_COUNT     (MAX_GROUP_COUNT + 1)

static CameraData camdata[GUI_CHANNEL_CNT];
static CameraData org_camdata[GUI_CHANNEL_CNT];
static UserAuthData authdata[GROUP_COUNT];
static UserAuthData org_authdata[GROUP_COUNT];

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *cam_ch[GUI_CHANNEL_CNT];
static NFOBJECT *disp_title;
static NFOBJECT *g_page_fixed[PAGE_FIXED_CNT];
static NFOBJECT *g_lb_page_num;

static NFOBJECT *admin[GUI_CHANNEL_CNT];
static NFOBJECT *manager[GUI_CHANNEL_CNT];
static NFOBJECT *user[GUI_CHANNEL_CNT];
static NFOBJECT *logoff[GUI_CHANNEL_CNT];

static NFOBJECT *admin_disp;
static NFOBJECT *manager_disp;
static NFOBJECT *user_disp;
static NFOBJECT *logoff_disp;



static void prvSetDataToObjects()
{
    guint i;
    gint idx;

    for(i=0; i<GUI_CHANNEL_CNT; i++)
    {
        nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)admin[i], camdata[i].admin);
        nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)manager[i], camdata[i].manager);
        nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)user[i], camdata[i].user);
        nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)logoff[i], camdata[i].logoff);
        
    }

    nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)admin_disp, authdata[0].cvt_disp);
    nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)manager_disp, authdata[1].cvt_disp);
    nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)user_disp, authdata[2].cvt_disp);
    nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)logoff_disp, authdata[3].cvt_disp);
        
}

static void prvLoadDataFromObjects()
{
    guint i;

    for(i = 0 ; i < GUI_CHANNEL_CNT ; i++)
    {
        camdata[i].admin    = nfui_spin_button_get_index((NFSPINBUTTON*)(admin[i]));
        camdata[i].manager  = nfui_spin_button_get_index((NFSPINBUTTON*)(manager[i]));
        camdata[i].user     = nfui_spin_button_get_index((NFSPINBUTTON*)(user[i]));
        camdata[i].logoff   = nfui_spin_button_get_index((NFSPINBUTTON*)(logoff[i]));
    }

    authdata[0].cvt_disp    = nfui_spin_button_get_index((NFSPINBUTTON*)(admin_disp));
    authdata[1].cvt_disp    = nfui_spin_button_get_index((NFSPINBUTTON*)(manager_disp));
    authdata[2].cvt_disp    = nfui_spin_button_get_index((NFSPINBUTTON*)(user_disp));
    authdata[3].cvt_disp    = nfui_spin_button_get_index((NFSPINBUTTON*)(logoff_disp));
}

static gboolean post_prev_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
    gchar buf[64];
    
    if(evt->type == GDK_BUTTON_RELEASE) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
          return FALSE;
        }

        for (i = 0; i < PAGE_FIXED_CNT; i++) {
            if (nfui_nfobject_is_shown((NFOBJECT*)g_page_fixed[i])) {
                break;
            }
        }

        if (i == PAGE_FIXED_CNT) return FALSE;

        if (i == 0) return FALSE;
        
		nfui_on_backscr(obj);
		nfui_rflip(obj);

        nfui_nfobject_hide(g_page_fixed[i]);

        i--;
        
        memset(buf, 0x00, sizeof(buf));
        g_sprintf(buf, "%d / %d", i + 1, PAGE_FIXED_CNT);
        nfui_nflabel_set_text(g_lb_page_num, buf);
        nfui_signal_emit(g_lb_page_num, GDK_EXPOSE, TRUE);

        nfui_nfobject_show(g_page_fixed[i]);
        nfui_signal_emit(g_page_fixed[i], GDK_EXPOSE, TRUE);

        nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    	
		nfui_flip(obj);
		nfui_off_backscr(obj);
    }

    return FALSE;
}

static gboolean post_next_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
    gchar buf[64];

    if(evt->type == GDK_BUTTON_RELEASE) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
          return FALSE;
        }

        for (i = 0; i < PAGE_FIXED_CNT; i++) {
            if (nfui_nfobject_is_shown((NFOBJECT*)g_page_fixed[i])) {
                break;
            }
        }

        if (i == PAGE_FIXED_CNT) return FALSE;

        if (i == (PAGE_FIXED_CNT - 1)) return FALSE;
        
		nfui_on_backscr(obj);
		nfui_rflip(obj);

        nfui_nfobject_hide(g_page_fixed[i]);

        i++;
                
        memset(buf, 0x00, sizeof(buf));
        g_sprintf(buf, "%d / %d", i + 1, PAGE_FIXED_CNT);
        nfui_nflabel_set_text(g_lb_page_num, buf);
        nfui_signal_emit(g_lb_page_num, GDK_EXPOSE, TRUE);

        nfui_nfobject_show(g_page_fixed[i]);
        nfui_signal_emit(g_page_fixed[i], GDK_EXPOSE, TRUE);

        nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));

		nfui_flip(obj);
		nfui_off_backscr(obj);
    }

    return FALSE;
}

static gboolean post_on_off_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_COMBOBOX_CHANGED) {
        NFOBJECT **tmpObj;
        guint objIdx;
        gint idx;
        gint i;

        objIdx = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "title index"));
        switch(objIdx) {
            case 1: tmpObj = admin; break;
            case 2: tmpObj = manager; break;
            case 3: tmpObj = user; break;
            case 4: tmpObj = logoff; break;
            default: return FALSE;
        }

        idx = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        for(i=0; i<GUI_CHANNEL_CNT; i++) 
            nfui_spin_button_set_index((NFSPINBUTTON*)tmpObj[i], (guint)idx);

        for(i=0; i<GUI_CHANNEL_CNT; i++) 
            nfui_signal_emit(tmpObj[i], GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean mainbg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    switch(evt->type)
    {
        case GDK_EXPOSE :
        break;

        case INFY_CAMDB_CHANGE_NOTIFY:
        case INFY_USRDB_CHANGE_NOTIFY:
        {
            gint i;
            gchar strBuf[STRING_SIZE_CAMTITLE];

            for (i = 0; i < GUI_CHANNEL_CNT; i++)
            {
                memset(strBuf, 0x00, sizeof(strBuf));
                var_get_camtitle(strBuf, i);
                nfui_nfimglabel_set_text((NFIMGLABEL*)cam_ch[i], strBuf);
                nfui_signal_emit(cam_ch[i], GDK_EXPOSE, TRUE);
            }
        }
        break;
        
        case GDK_DELETE:
            uxm_unreg_imsg_event(obj, INFY_CAMDB_CHANGE_NOTIFY);        
            uxm_unreg_imsg_event(obj, INFY_USRDB_CHANGE_NOTIFY);        
        break;
            
        default :
            break;
    }

    return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        guint i;
        
        if(evt->button.button == MOUSE_RIGTH_BUTTON) 
            return FALSE;

        g_memmove(camdata, org_camdata, sizeof(CameraData)*GUI_CHANNEL_CNT);
        g_memmove(authdata, org_authdata, sizeof(UserAuthData)*GROUP_COUNT);
        prvSetDataToObjects();

        for(i=0; i<GUI_CHANNEL_CNT; i++)
        {
            nfui_signal_emit(admin[i], GDK_EXPOSE, TRUE);
            nfui_signal_emit(manager[i], GDK_EXPOSE, TRUE);
            nfui_signal_emit(user[i], GDK_EXPOSE, TRUE);
            nfui_signal_emit(logoff[i], GDK_EXPOSE, TRUE);
        }

        nfui_signal_emit(admin_disp, GDK_EXPOSE, TRUE);
        nfui_signal_emit(manager_disp, GDK_EXPOSE, TRUE);
        nfui_signal_emit(user_disp, GDK_EXPOSE, TRUE);
        nfui_signal_emit(logoff_disp, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    int edited = 0;

    if(evt->type == GDK_BUTTON_RELEASE)
    {
        guint i;

        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
          return FALSE;
        }

        prvLoadDataFromObjects();

        if(memcmp(org_camdata, camdata, sizeof(CameraData)*GUI_CHANNEL_CNT))
        {           
            scm_put_log(CHANGE_CAM_CVRT, 0, 0);
            edited |= 0x01;

            g_memmove(org_camdata, camdata, sizeof(CameraData)*GUI_CHANNEL_CNT);
            DAL_set_covert_data_all(camdata, GUI_CHANNEL_CNT);
        }

        if(memcmp(org_authdata, authdata, sizeof(UserAuthData)*GROUP_COUNT))
        {           
            scm_put_log(CHANGE_CAM_CVRT, 0, 0);
            edited |= 0x02;

            g_memmove(org_authdata, authdata, sizeof(UserAuthData)*GROUP_COUNT);
            DAL_set_userAuth_data_all(authdata, GROUP_COUNT);

        }

        if (edited) {
            nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
            syscam_set_changeflag(edited);
        }
    }
    return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
    
        CamCovert_tab_out_handler();

        SystemSetupCam_Destroy(obj);
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




void init_CamCovert_page(NFOBJECT *parent)
{
    NFOBJECT *content_fixed;
    NFOBJECT *ntb;
    NFOBJECT *title_object[CAM_SETUP_COLUMNS];
    NFOBJECT *camsetup_btns[CSB_BUTTONS];
    NFOBJECT *obj;
    NFOBJECT *main_page_fixed;
    NFOBJECT *page_fixed[PAGE_FIXED_CNT];
    NFOBJECT *page_ntb[PAGE_FIXED_CNT];

    GdkPixbuf *pbCamImage[32];
    const gchar *strButton[] = {"CANCEL", "APPLY", "CLOSE"};
    const gchar *strTitle[CAM_SETUP_COLUMNS] = {"CHANNEL", "ADMIN", "MANAGER", "USER", "LOG OUT"};
    const gchar *strTitle2[CAM_SETUP_COLUMNS] = {"", "ADMIN", "MANAGER", "USER", "LOG OUT"};
    const gchar *strOffOn[] = {"OFF", "ON"};
    const gchar *strDisplay[] = {"NO VIDEO", "COVERT"};

    gchar strBuf[STRING_SIZE_64];

    guint width[CAM_SETUP_COLUMNS];
    guint i;
    int title_ypos;
	gint size_w, size_h;
	gint page_num, row_num;
	GdkPixbuf *prev_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *next_img[NFOBJECT_STATE_COUNT];

    g_curwnd = nfui_nfobject_get_top(parent);

    width[0] = 250;
	width[1] = 200;
	width[2] = 200;
	width[3] = 200;
	width[4] = 200;
    
// CAMERA IMAGE LOAD
    pbCamImage[0]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_01, NULL); 
    pbCamImage[1]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_02, NULL); 
    pbCamImage[2]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_03, NULL); 
    pbCamImage[3]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_04, NULL); 
    pbCamImage[4]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_05, NULL);         
    pbCamImage[5]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_06, NULL); 
    pbCamImage[6]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_07, NULL); 
    pbCamImage[7]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_08, NULL); 
    pbCamImage[8]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_09, NULL); 
    pbCamImage[9]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_10, NULL);     
    pbCamImage[10] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_11, NULL); 
    pbCamImage[11] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_12, NULL); 
    pbCamImage[12] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_13, NULL); 
    pbCamImage[13] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_14, NULL); 
    pbCamImage[14] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_15, NULL);         
    pbCamImage[15] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_16, NULL);
    pbCamImage[16] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_17, NULL); 
    pbCamImage[17] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_18, NULL); 
    pbCamImage[18] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_19, NULL); 
    pbCamImage[19] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_20, NULL); 
    pbCamImage[20] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_21, NULL);         
    pbCamImage[21] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_22, NULL); 
    pbCamImage[22] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_23, NULL); 
    pbCamImage[23] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_24, NULL); 
    pbCamImage[24] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_25, NULL); 
    pbCamImage[25] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_26, NULL);     
    pbCamImage[26] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_27, NULL); 
    pbCamImage[27] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_28, NULL); 
    pbCamImage[28] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_29, NULL); 
    pbCamImage[29] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_30, NULL); 
    pbCamImage[30] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_31, NULL);             
    pbCamImage[31] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_32, NULL);    

	prev_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_N_BTN), NULL);
	prev_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_O_BTN), NULL);
	prev_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_P_BTN), NULL);
	prev_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_D_BTN), NULL);

	next_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_N_BTN), NULL);
	next_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_O_BTN), NULL);
	next_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_P_BTN), NULL);
	next_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_D_BTN), NULL);


    memset(camdata, 0x00, sizeof(CameraData)*GUI_CHANNEL_CNT);
    memset(org_camdata, 0x00, sizeof(CameraData)*GUI_CHANNEL_CNT);

    memset(authdata, 0x00, sizeof(UserAuthData)*GROUP_COUNT);
    memset(org_authdata, 0x00, sizeof(UserAuthData)*GROUP_COUNT);

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        DAL_get_covert_data(&camdata[i], i);
    }

    for (i = 0; i < GROUP_COUNT; i++)
    {
        DAL_get_userAuth_data(&(authdata[i]), i);    
    }


    content_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(content_fixed, MENU_V_INNER_W, MENU_V_INNER_H);
    nfui_nfobject_show(content_fixed);
    nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);

    ntb = (NFOBJECT*)nfui_nftable_new ( CAM_SETUP_COLUMNS, 1, 2, 1, width, CAM_SETUP_LABEL_HEIGHT);                                     
    nfui_nfobject_show(ntb);
    nfui_nffixed_put((NFFIXED*)content_fixed, ntb, CAM_SETUP_TABLE_LEFT, CAM_SETUP_TABLE_TOP);    

    for (i = 0; i < CAM_SETUP_COLUMNS; i++)
    {
        if (i == 0) 
        {
            title_object[i] = (NFOBJECT*)nfui_nflabel_new_with_pango_font((gchar*)(strTitle[i]), 
            nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));     
            nfui_nfobject_modify_bg(title_object[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
            nfui_nfobject_use_focus(title_object[i], NFOBJECT_FOCUS_OFF);
            nfui_nfobject_show(title_object[i]);
        }
        else 
        {
            title_object[i] = nfui_combobox_new(0, 0, 0);
            nfui_combobox_set_skin_type(NF_COMBOBOX(title_object[i]), NFCOMBOBOX_TYPE_2);
            nfui_combobox_set_align(NF_COMBOBOX(title_object[i]), NFALIGN_CENTER, 0);
            nfui_nfobject_set_size(title_object[i], width[i], CAM_SETUP_LABEL_HEIGHT);
            nfui_nfobject_show(title_object[i]);
            nfui_nfobject_set_data(title_object[i], "title index", GUINT_TO_POINTER(i));
            nfui_regi_post_event_callback(title_object[i], post_on_off_all_event_handler);

            nfui_combobox_append_data(NF_COMBOBOX(title_object[i]), lookup_string(strOffOn[0]));
            nfui_combobox_append_data(NF_COMBOBOX(title_object[i]), lookup_string(strOffOn[1]));

            if (i == 4)     
                nfui_combobox_set_display_string(NF_COMBOBOX(title_object[i]), lookup_string(strTitle[i]));
            else
                nfui_combobox_set_display_string(NF_COMBOBOX(title_object[i]), strTitle[i]);
        }
        nfui_nftable_attach((NFTABLE*)ntb, title_object[i], i, 0);

        if (i != 0)
        {
            nfui_nfobject_support_multi_lang(title_object[i], FALSE);
        }
        else 
            nfui_nfobject_support_multi_lang(title_object[i], TRUE);
    }

    main_page_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(main_page_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(main_page_fixed, nfui_nftable_get_width((NFTABLE*)ntb), (CAM_SETUP_ROWS * 40) + 80);
    nfui_nfobject_show(main_page_fixed);
    nfui_nffixed_put((NFFIXED*)content_fixed, main_page_fixed, CAM_SETUP_TABLE_LEFT, CAM_SETUP_TABLE_TOP+CAM_SETUP_LABEL_HEIGHT+1);
    
    for (i = 0; i < PAGE_FIXED_CNT; i++)
    {
        g_page_fixed[i] = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_modify_bg(g_page_fixed[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
        nfui_nfobject_set_size(g_page_fixed[i], nfui_nftable_get_width((NFTABLE*)ntb), 320);
        nfui_nffixed_put((NFFIXED*)main_page_fixed, g_page_fixed[i], 0, 0);

        page_ntb[i] = (NFOBJECT*)nfui_nftable_new (CAM_SETUP_COLUMNS, CAM_SETUP_ROWS, 2, 1, width, CAM_SETUP_LABEL_HEIGHT);
        nfui_nfobject_show(page_ntb[i]);
        nfui_nffixed_put((NFFIXED*)g_page_fixed[i], page_ntb[i], 0, 0);
    }
    nfui_nfobject_show(g_page_fixed[0]);

	nfui_get_pixbuf_size(prev_img[0], &size_w, &size_h);
	
	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), prev_img);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_page_fixed, obj, (main_page_fixed->width / 2) - (size_w + 60), main_page_fixed->height - size_h);
	nfui_regi_post_event_callback(obj, post_prev_event_handler);

	memset(strBuf, 0x00, sizeof(strBuf));
	g_sprintf(strBuf, "1 / %d", PAGE_FIXED_CNT);
	
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 100, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_page_fixed, obj, (main_page_fixed->width - 100) / 2, main_page_fixed->height - size_h);
    g_lb_page_num = obj;
    
	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), next_img);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_page_fixed, obj, (main_page_fixed->width / 2) + 60, main_page_fixed->height - size_h);
	nfui_regi_post_event_callback(obj, post_next_event_handler);

    page_num = row_num = 0;
    
    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        var_get_camtitle(strBuf, i);

        cam_ch[i] = (NFOBJECT*)nfui_nfimglabel_new(pbCamImage[i], strBuf);
        nfui_nfimglabel_set_align((NFIMGLABEL*)cam_ch[i], NFALIGN_LEFT);
        nfui_nfimglabel_set_pango_font((NFIMGLABEL*)cam_ch[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
        nfui_nfobject_support_multi_lang(cam_ch[i], FALSE);
        nfui_nfobject_use_focus(cam_ch[i], NFOBJECT_FOCUS_OFF);
        nfui_nfobject_modify_bg(cam_ch[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        nfui_nfobject_show(cam_ch[i]);
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], cam_ch[i], 0, row_num);

        row_num++;

        if (row_num == CAM_SETUP_ROWS) {
            row_num = 0;
            page_num++;
        }
    }

    page_num = row_num = 0; 
        
    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {   
        admin[i] = (NFOBJECT*)nfui_spinbutton_new((gchar**)strOffOn, 2, (gint)(camdata[i].admin));
        manager[i] = (NFOBJECT*)nfui_spinbutton_new((gchar**)strOffOn, 2, (gint)(camdata[i].manager));
        user[i] = (NFOBJECT*)nfui_spinbutton_new((gchar**)strOffOn, 2, (gint)(camdata[i].user));
        logoff[i] = (NFOBJECT*)nfui_spinbutton_new((gchar**)strOffOn, 2, (gint)(camdata[i].logoff));
        
        nfui_spinbutton_set_skin_type((NFSPINBUTTON*)admin[i], NFSPINBUTTON_TYPE_1);
        nfui_spinbutton_set_skin_type((NFSPINBUTTON*)manager[i], NFSPINBUTTON_TYPE_1);
        nfui_spinbutton_set_skin_type((NFSPINBUTTON*)user[i], NFSPINBUTTON_TYPE_1);
        nfui_spinbutton_set_skin_type((NFSPINBUTTON*)logoff[i], NFSPINBUTTON_TYPE_1);

        nfui_nfobject_show(admin[i]);
        nfui_nfobject_show(manager[i]);
        nfui_nfobject_show(user[i]);
        nfui_nfobject_show(logoff[i]);
        
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], admin[i], 1, row_num);
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], manager[i], 2, row_num);
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], user[i], 3, row_num);
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], logoff[i], 4, row_num);

        row_num++;

        if (row_num == CAM_SETUP_ROWS) {
            row_num = 0;
            page_num++;
        }
    }

////

    title_ypos = main_page_fixed->y + main_page_fixed->height + 80;

    ntb = (NFOBJECT*)nfui_nftable_new ( CAM_SETUP_COLUMNS, 2, 2, 1, width, CAM_SETUP_LABEL_HEIGHT);                                     
    nfui_nfobject_show(ntb);
    nfui_nffixed_put((NFFIXED*)content_fixed, ntb, CAM_SETUP_TABLE_LEFT, title_ypos);
    
    for (i = 0; i < CAM_SETUP_COLUMNS; i++)
    {
        if (i == 4) title_object[i] = (NFOBJECT*)nfui_nflabel_new_with_pango_font((gchar*)(lookup_string(strTitle2[i])),nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
        else title_object[i] = (NFOBJECT*)nfui_nflabel_new_with_pango_font(((gchar*)strTitle2[i]), 
                                    nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));     
        if (i != 0) nfui_nfobject_support_multi_lang(title_object[i], FALSE);
        else nfui_nfobject_support_multi_lang(title_object[i], TRUE);
        nfui_nfobject_modify_bg(title_object[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        nfui_nfobject_use_focus(title_object[i], NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(title_object[i]);
        nfui_nftable_attach((NFTABLE*)ntb, title_object[i], i, 0);
    }

    for (i = 0; i < 1; i++)
    {
        sprintf(strBuf, "SHOWN AS");

        disp_title = nfui_nflabel_new_with_pango_font(strBuf, 
                                    nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));     
        nfui_nfobject_modify_bg(disp_title, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        nfui_nfobject_use_focus(disp_title, NFOBJECT_FOCUS_OFF);
        nfui_nflabel_set_align((NFOBJECT*)disp_title, NFALIGN_LEFT, 10); 
        
        admin_disp  = (NFOBJECT*)nfui_spinbutton_new((gchar**)strDisplay, 2, (gint)(authdata[0].cvt_disp));
        manager_disp    = (NFOBJECT*)nfui_spinbutton_new((gchar**)strDisplay, 2, (gint)(authdata[1].cvt_disp));
        user_disp   = (NFOBJECT*)nfui_spinbutton_new((gchar**)strDisplay, 2, (gint)(authdata[2].cvt_disp));
        logoff_disp     = (NFOBJECT*)nfui_spinbutton_new((gchar**)strDisplay, 2, (gint)(authdata[3].cvt_disp));
        
        nfui_spinbutton_set_skin_type((NFSPINBUTTON*)admin_disp, NFSPINBUTTON_TYPE_1);
        nfui_spinbutton_set_skin_type((NFSPINBUTTON*)manager_disp, NFSPINBUTTON_TYPE_1);
        nfui_spinbutton_set_skin_type((NFSPINBUTTON*)user_disp, NFSPINBUTTON_TYPE_1);
        nfui_spinbutton_set_skin_type((NFSPINBUTTON*)logoff_disp, NFSPINBUTTON_TYPE_1);

        nfui_nfobject_show(disp_title);
        nfui_nfobject_show(admin_disp);
        nfui_nfobject_show(manager_disp);
        nfui_nfobject_show(user_disp);
        nfui_nfobject_show(logoff_disp);

        nfui_nftable_attach((NFTABLE*)ntb, disp_title, 0, i+1);
        nfui_nftable_attach((NFTABLE*)ntb, admin_disp, 1, i+1);
        nfui_nftable_attach((NFTABLE*)ntb, manager_disp, 2, i+1);
        nfui_nftable_attach((NFTABLE*)ntb, user_disp, 3, i+1);
        nfui_nftable_attach((NFTABLE*)ntb, logoff_disp, 4, i+1);
    }


////
    camsetup_btns[0] = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(camsetup_btns[0]), NFALIGN_CENTER, 0);   
    nfui_nfobject_show(camsetup_btns[0]);
    nfui_nffixed_put((NFFIXED*)parent, camsetup_btns[0], MENU_V_BTN_R3_X, MENU_V_BTN_Y);

    camsetup_btns[1] = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(camsetup_btns[1]), NFALIGN_CENTER, 0);   
    nfui_nfobject_show(camsetup_btns[1]);
    nfui_nffixed_put((NFFIXED*)parent, camsetup_btns[1], MENU_V_BTN_R2_X, MENU_V_BTN_Y);

    camsetup_btns[2] = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(camsetup_btns[2]), NFALIGN_CENTER, 0);   
    nfui_nfobject_show(camsetup_btns[2]);
    nfui_nffixed_put((NFFIXED*)parent, camsetup_btns[2], MENU_V_BTN_R1_X, MENU_V_BTN_Y);

    nfui_regi_pre_event_callback(content_fixed, mainbg_event_handler);
    nfui_regi_post_event_callback(camsetup_btns[0], post_cancelbutton_event_handler);
    nfui_regi_post_event_callback(camsetup_btns[1], post_applybutton_event_handler);
    nfui_regi_post_event_callback(camsetup_btns[2], post_closebutton_event_handler);

    nfui_regi_post_event_callback(parent, post_page_event_handler);
        
    uxm_reg_imsg_event(content_fixed, INFY_CAMDB_CHANGE_NOTIFY);
    uxm_monitor_on_imsg_event(content_fixed, INFY_CAMDB_CHANGE_NOTIFY);
    uxm_reg_imsg_event(content_fixed, INFY_USRDB_CHANGE_NOTIFY);
    uxm_monitor_on_imsg_event(content_fixed, INFY_USRDB_CHANGE_NOTIFY);     
    
    g_memmove(org_camdata, camdata, sizeof(CameraData)*GUI_CHANNEL_CNT);
    g_memmove(org_authdata, authdata, sizeof(UserAuthData)*GROUP_COUNT);
}

gboolean CamCovert_tab_in_handler()
{

    return FALSE;
}

gboolean CamCovert_tab_out_handler()
{
    mb_type ret;
    guint i;
    int edited = 0;

    prvLoadDataFromObjects();

    if(!memcmp(org_camdata, camdata, sizeof(CameraData)*GUI_CHANNEL_CNT) &&
        !memcmp(org_authdata, authdata, sizeof(UserAuthData)*GROUP_COUNT))
        return FALSE;

    ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", 
                            NFTOOL_MB_OKCANCEL);

    if(ret == NFTOOL_MB_OK)
    {
        if(memcmp(org_camdata, camdata, sizeof(CameraData)*GUI_CHANNEL_CNT))
        {
            scm_put_log(CHANGE_CAM_CVRT, 0, 0);
            edited |= 0x01;

            g_memmove(org_camdata, camdata, sizeof(CameraData)*GUI_CHANNEL_CNT);
            DAL_set_covert_data_all(camdata, GUI_CHANNEL_CNT);
        }

        if(memcmp(org_authdata, authdata, sizeof(UserAuthData)*GROUP_COUNT))
        {
            scm_put_log(CHANGE_CAM_CVRT, 0, 0);
            edited |= 0x02;

            g_memmove(org_authdata, authdata, sizeof(UserAuthData)*GROUP_COUNT);
            DAL_set_userAuth_data_all(authdata, GROUP_COUNT);   

        }

        if (edited) {
            syscam_set_changeflag(edited);
        }
    }
    else if(ret == NFTOOL_MB_CANCEL)
    {
        g_memmove(camdata, org_camdata, sizeof(CameraData)*GUI_CHANNEL_CNT);
        g_memmove(authdata, org_authdata, sizeof(UserAuthData)*GROUP_COUNT);
        prvSetDataToObjects();
    }

    return FALSE;
}





    
