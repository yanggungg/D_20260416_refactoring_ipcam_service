#include "nf_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfscrolledfixed.h"
#include "objects/nflabel.h"
#include "objects/nftable.h"

#include "services/scm.h"

#include "vw_sys_license_devinfo.h"



#define LDI_WND_POS_X           ((DISPLAY_ACTIVE_WIDTH - LDI_WND_SIZE_W)/2)
#define LDI_WND_POS_Y           ((DISPLAY_ACTIVE_HEIGHT - LDI_WND_SIZE_H)/2)
#define LDI_WND_SIZE_W          (1128)
#if defined(GUI_4CH_SUPPORT)
#define LDI_WND_SIZE_H          (420)
#elif defined(GUI_8CH_SUPPORT)
#define LDI_WND_SIZE_H          (590)       
#else
#define LDI_WND_SIZE_H          (930)
#define LDI_WND_SIZE_W          (1188)
#endif


static LIC_DEVINFO_T *g_devinfo = NULL;

static NFWINDOW *g_curwnd;
static NFOBJECT *g_ntb;



static void _set_cam_info()
{
    NFOBJECT *obj;
    gchar strBuf[32];
    gint i;


    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        if (!g_devinfo->profile[i].connected) continue;
        
        obj = nfui_nftable_get_child((NFTABLE*)g_ntb, 1, i+2);
        nfui_nflabel_set_text((NFLABEL*)obj, g_devinfo->profile[i].model.name);

        obj = nfui_nftable_get_child((NFTABLE*)g_ntb, 2, i+2);
        nfui_nflabel_set_text((NFLABEL*)obj, g_devinfo->profile[i].model.swver);

        obj = nfui_nftable_get_child((NFTABLE*)g_ntb, 3, i+2);
    	memset(strBuf, 0x00, sizeof(strBuf));
    	g_sprintf(strBuf, "%02x:%02x:%02x:%02x:%02x:%02x", g_devinfo->profile[i].model.mac[0], g_devinfo->profile[i].model.mac[1], g_devinfo->profile[i].model.mac[2],
					                                       g_devinfo->profile[i].model.mac[3], g_devinfo->profile[i].model.mac[4], g_devinfo->profile[i].model.mac[5]);	
        nfui_nflabel_set_text((NFLABEL*)obj, strBuf);
    }
}

static void _set_nvr_info()
{
    NFOBJECT *obj;
    gchar strBuf[32];

    obj = nfui_nftable_get_child((NFTABLE*)g_ntb, 1, 1);
    nfui_nflabel_set_text((NFLABEL*)obj, g_devinfo->sys_info.model);

    obj = nfui_nftable_get_child((NFTABLE*)g_ntb, 2, 1);
    nfui_nflabel_set_text((NFLABEL*)obj,  g_devinfo->sys_info.swVer);

    obj = nfui_nftable_get_child((NFTABLE*)g_ntb, 3, 1);
	memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, "%02x:%02x:%02x:%02x:%02x:%02x",
                (guchar)g_devinfo->netif_info.mac_addr[0],
                (guchar)g_devinfo->netif_info.mac_addr[1],
                (guchar)g_devinfo->netif_info.mac_addr[2],
                (guchar)g_devinfo->netif_info.mac_addr[3],
                (guchar)g_devinfo->netif_info.mac_addr[4],
                (guchar)g_devinfo->netif_info.mac_addr[5]);
    nfui_nflabel_set_text((NFLABEL*)obj, strBuf);
}

static gboolean post_close_event_handler(NFOBJECT * obj, GdkEvent * evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        nfui_nfobject_destroy(g_curwnd);
    }

    return FALSE;
}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_DELETE)
    {
        g_curwnd = 0;
        gtk_main_quit();
    }

    return FALSE;
}

void VW_License_DevInfo_Popup_Open(NFOBJECT *parent, LIC_DEVINFO_T *devinfo)
{
    NFOBJECT *wnd;
    NFOBJECT *fixed;
    NFOBJECT *scrolled_fixed;
    NFOBJECT *ntb;
    NFOBJECT *obj;

    gint pos_x, pos_y;
    gint size_w, size_h;
    guint i;
    guint width[4];
    gchar strBuf[32];


    g_devinfo = devinfo;

    wnd = nftool_create_popup_window((NFWINDOW*)parent, LDI_WND_POS_X, LDI_WND_POS_Y, LDI_WND_SIZE_W, LDI_WND_SIZE_H, "DEVICE INFORMATION", FALSE);
    nfui_regi_post_event_callback(wnd, post_page_event_handler);
    g_curwnd = wnd;

    fixed = ((NFWINDOW*)wnd)->child;

    width[0] = 200;
    width[1] = 300;
    width[2] = 300;
    width[3] = 300;

    pos_x = 4 + 10;
    pos_y = 44 + 15;

    scrolled_fixed = (NFOBJECT*)nfui_nfscrolledfixed_new(NFSCROLLED_POLICY_NEVER, NFSCROLLED_POLICY_AUTOMATIC);
    nfui_nfscrolledfixed_set_skin_type((NFSCROLLEDFIXED*)scrolled_fixed, NFSCROLLEDFIXED_TYPE_POPUP_1);
    nfui_nfscrolledfixed_set_vscroll_speed((NFSCROLLEDFIXED*)scrolled_fixed, (LDI_WND_SIZE_H - 155)/3, (LDI_WND_SIZE_H - 155)/10, (LDI_WND_SIZE_H - 155)/20);
    nfui_nfscrolledfixed_set_vscroll_offset((NFSCROLLEDFIXED*)scrolled_fixed, 0);
    nfui_nfobject_set_size(scrolled_fixed, LDI_WND_SIZE_W - 20, LDI_WND_SIZE_H - 155);
    nfui_nfobject_show(scrolled_fixed);
    nfui_nffixed_put((NFFIXED*)fixed, scrolled_fixed, pos_x, pos_y);

    ntb = nfui_nftable_new(4, GUI_CHANNEL_CNT+2, 1, 1, width, 40);
    nfui_nfobject_show(ntb);
    nfui_nfscrolledfixed_put((NFSCROLLEDFIXED*)scrolled_fixed, ntb, 0, 0);
    g_ntb = ntb;

    obj = nfui_nflabel_new_with_pango_font("MODEL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb, obj, 1, 0);

    obj = nfui_nflabel_new_with_pango_font("FW VERSION", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb, obj, 2, 0);

    obj = nfui_nflabel_new_with_pango_font("MAC ADDRESS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb, obj, 3, 0);

//===> LOCAL DEVICE       
#if defined (_IPX_MODEL_UX)
    obj = nfui_nflabel_new_with_pango_font("NVR", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
#else
    obj = nfui_nflabel_new_with_pango_font("DVR", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
#endif
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb, obj, 0, 1);

    obj = (NFOBJECT*)nfui_nflabel_new_text_box("N/A", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
    nfui_nfobject_use_focus(obj, FALSE);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb, obj, 1, 1);

    obj = (NFOBJECT*)nfui_nflabel_new_text_box("N/A", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
    nfui_nfobject_use_focus(obj, FALSE);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb, obj, 2, 1);

    obj = (NFOBJECT*)nfui_nflabel_new_text_box("N/A", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
    nfui_nfobject_use_focus(obj, FALSE);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb, obj, 3, 1);

//===> CAMERA
    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        memset(strBuf, 0x00, sizeof(strBuf));
        var_get_camtitle(strBuf, i);
//        g_sprintf(strBuf, "CH %d", i+1);
    
        obj = nfui_nflabel_new_with_pango_font(g_devinfo->camtitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)ntb, obj, 0, i+2);

        obj = (NFOBJECT*)nfui_nflabel_new_text_box("N/A", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
        nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
        nfui_nfobject_use_focus(obj, FALSE);
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)ntb, obj, 1, i+2);

        obj = (NFOBJECT*)nfui_nflabel_new_text_box("N/A", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
        nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
        nfui_nfobject_use_focus(obj, FALSE);
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)ntb, obj, 2, i+2);

        obj = (NFOBJECT*)nfui_nflabel_new_text_box("N/A", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
        nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
        nfui_nfobject_use_focus(obj, FALSE);
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)ntb, obj, 3, i+2);
    }

    _set_nvr_info();
    _set_cam_info();

    pos_x = (LDI_WND_SIZE_W - 160) / 2;
    pos_y = LDI_WND_SIZE_H - 40 - 15;

    obj = nftool_normal_button_create_type2("CLOSE", 160);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_close_event_handler);

    nfui_nfobject_show(wnd);
    nfui_make_key_hierarchy((NFWINDOW*)wnd);

    gtk_main();
    
    return;
}
