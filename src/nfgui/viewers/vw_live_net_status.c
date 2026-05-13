
#include "nf_afx.h"


#include "../service/ddns2_manager.h"

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
#include "viewers/objects/nfbutton.h"
#include "viewers/objects/nfimage.h"
#include "viewers/objects/nftable.h"
#include "viewers/objects/nflistbox.h"

#include "ix_func.h"
#include "ix_mem.h"

#include "vw_live_net_status.h"
#include "vw_live_net_ipcam_popup.h"
#include "vw_system_self_start.h"
#include "vw_qr_code_popup.h"
#include "vw_net_status_ipconflict_list_popup.h"
#include "vw_packet_dump_popup.h"

#include "scm.h"
#include "uxm.h"
#include "ssm.h"
#include "dtf.h"
#include "vsm.h"


#include "nf_util_netif.h"
#include "nf_network.h"
#include "nf_ipcam_defs.h"

#include "vw.h"
#include "vw_internal.h"


#define NET_STATUS_SIZE_W           (1533)
#define NET_STATUS_SIZE_H           (957)
#define NET_STATUS_POS_X            ((DISPLAY_ACTIVE_WIDTH - NET_STATUS_SIZE_W)/2)
#define NET_STATUS_POS_Y            (((DISPLAY_ACTIVE_HEIGHT - 108) - NET_STATUS_SIZE_H)/2)

#define MAX_SHOWN_CLIENT            (8)

#define MAP_LINE_PORT_X             (10)
#define MAP_LINE_PORT_Y             (5)

#define MAP_LINE_PORT_GAP_W         (23)
#define MAP_LINE_GAP_H              (8)

#define INTERNET_IMAGE_X            (182)
#define INTERNET_IMAGE_Y            (214)

#define GATEWAY_IMAGE_X             (440)
#define GATEWAY_IMAGE_Y             (214)

#define CAM_IMAGE_ROW1_X            (1000)

#if defined(GUI_16CH_SUPPORT) && defined(_SUPPORT_HUB) && (!defined(_IPX_1648P4E) && !defined(_IPX_1648M4E))
#define INTERNET_IMAGE_X            (182)
#define INTERNET_IMAGE_Y            (174)

#define GATEWAY_IMAGE_X             (440)
#define GATEWAY_IMAGE_Y             (INTERNET_IMAGE_Y)

#define DVR_IMAGE_X                 (690)
#define DVR_IMAGE_Y                 (230)

#define HUB_IMAGE_X                 (DVR_IMAGE_X-22)
#define HUB_IMAGE_Y                 (DVR_IMAGE_Y+220)

#define CAM_IMAGE_ROW1_X            (1000)
#elif defined(GUI_16CH_SUPPORT)

#define DVR_IMAGE_X                 (690)
#define DVR_IMAGE_Y                 (237)

#define CAM_IMAGE_ROW1_X            (1000)
#elif defined(GUI_32CH_SUPPORT)

#define DVR_IMAGE_X                 (690)
#define DVR_IMAGE_Y                 (100)

#define CAM_IMAGE_ROW1_X            (600)
#else

#define DVR_IMAGE_X                 (690)
#define DVR_IMAGE_Y                 (270)

#define CAM_IMAGE_ROW1_X            (1000)
#endif

#define CAM_IMAGE_GAP_Y             (100)

#if defined (GUI_4CH_SUPPORT)
#define MAP_NUM_CAM_COLS            (4)
#else
#define MAP_NUM_CAM_COLS            (8)
#endif

#if defined(GUI_4CH_SUPPORT)
#define NET_STATUS_DVR_IMAGE        (IMG_NET_DVR_4)
#elif defined(GUI_8CH_SUPPORT)
#define NET_STATUS_DVR_IMAGE        (IMG_NET_DVR_8)
#else
#if defined(_SUPPORT_HUB) && (!defined(_IPX_1648P4E) && !defined(_IPX_1648M4E))
#define NET_STATUS_DVR_IMAGE        (IMG_NET_DVR_8)
#define NET_STATUS_HUB_IMAGE        (IMG_NET_HUB_8)
#elif defined(_IPX_1648P4E) || defined(_IPX_1648M4E) 
#define NET_STATUS_DVR_IMAGE        (IMG_NET_DVR_16)
#elif defined(GUI_32CH_SUPPORT)
#define NET_STATUS_DVR_IMAGE        (IMG_NET_SWITCH)
#else
#define NET_STATUS_DVR_IMAGE        (IMG_NET_SWITCH)
#endif
#endif

#if defined(GUI_4CH_SUPPORT)
#define GET_CHANNEL_KEY_SCOPE(kpid)         (kpid >= KEYPAD_CH01 && kpid <= KEYPAD_CH04)
#elif defined(GUI_8CH_SUPPORT)
#define GET_CHANNEL_KEY_SCOPE(kpid)         (kpid >= KEYPAD_CH01 && kpid <= KEYPAD_CH08)
#else
#define GET_CHANNEL_KEY_SCOPE(kpid)         (kpid >= KEYPAD_CH01 && kpid <= KEYPAD_CH16)
#endif

#if defined(_IPX_0824P3)||defined(_IPX_1648P3) || defined(_IPX_32P4E) || defined(_IPX_1648P4E) || defined(_IPX_0824P4E) || defined(_IPX_1648M4E) || defined(_IPX_0824M4E) || defined(_IPX_0412M4E)  || defined(_IPX_32M4E) || defined(_IPX_32P5)
#define PER_PORT		(30)
#else
#define PER_PORT		(15)
#endif

#define STR_SSN_LIVE                "LIVE"
#define STR_SSN_PLAY                "PLAYBACK"
#define STR_SSN_ARCH                "ARCHIVING"
#define STR_SSN_NOTHING             "NOTHING"

enum {
    PORT_WAN = 0,
    PORT_LAN,
    PORT_IPCAM1,
    PORT_IPCAM2,
    PORT_IPCAM3,
    PORT_IPCAM4,
#if !defined(GUI_4CH_SUPPORT)
    PORT_IPCAM5,
    PORT_IPCAM6,
    PORT_IPCAM7,
    PORT_IPCAM8,
#if !defined(GUI_8CH_SUPPORT)
#if defined(_SUPPORT_HUB) && (!defined(_IPX_1648P4E) && !defined(_IPX_1648M4E))
    PORT_HUB_WAN,
    PORT_HUB_LAN,
#endif
    PORT_IPCAM9,
    PORT_IPCAM10,
    PORT_IPCAM11,
    PORT_IPCAM12,
    PORT_IPCAM13,
    PORT_IPCAM14,
    PORT_IPCAM15,
    PORT_IPCAM16,
#if !defined(GUI_16CH_SUPPORT)    
    PORT_IPCAM17,
    PORT_IPCAM18,
    PORT_IPCAM19,
    PORT_IPCAM20,
    PORT_IPCAM21,
    PORT_IPCAM22,
    PORT_IPCAM23,
    PORT_IPCAM24,
    PORT_IPCAM25,
    PORT_IPCAM26,
    PORT_IPCAM27, 
    PORT_IPCAM28,
    PORT_IPCAM29,
    PORT_IPCAM30,
    PORT_IPCAM31,
    PORT_IPCAM32,
#endif
#endif
#endif
    NUM_PORT
};


enum {
    PROPERTY_IP = 0,
	PROPERTY_IPV6,
    PROPERTY_MAC,
    PROPERTY_DDNS,
    PROPERTY_RTSP,
    PROPERTY_WEB,
    PROPERTY_DDNS_STATUS,
    PROPERTY_EXTERNAL,
    PROPERTY_SEQURINET,
    NUM_PROPERTY_ROWS
};

enum {
    IPCONFLICT_TYPE =0,
    IPCONFLICT_TIME,
    IPCONFLICT_MAC,
    NUM_IPCONFLICT
};


enum {
    CLIENT_FIELD_ID = 0,
    CLIENT_FIELD_GROUP,
    CLIENT_FIELD_IP,
    CLIENT_FIELD_ACTION,
    NUM_CLIENT_FIELD_COLS
};

typedef enum {
    NET_MAP_CONNECT = 0,
    NET_MAP_DISCONNECT,
    NET_MAP_ERROR,
    NET_MAP_CONNECTING,
    NET_MAP_FOCUS
} NET_MAP_E;

#if defined(GUI_16CH_SUPPORT) && defined(_SUPPORT_HUB) && (!defined(_IPX_1648P4E) && !defined(_IPX_1648M4E))
#define _TRANS_CH(ch)                       (ch < 8 ? PORT_IPCAM1+ch : PORT_IPCAM9+ch-8)
#else
#define _TRANS_CH(ch)                       (PORT_IPCAM1+ch)
#endif

static CONFLICT_INFOR g_setip;


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_net_win = NULL;

static NFOBJECT *port_obj[NUM_PORT];
static NFOBJECT *internet_obj = NULL;
static NFOBJECT *gateway_obj = NULL;
static NFOBJECT *dvr_obj = NULL;
static NFOBJECT *hub_obj = NULL;
static NFOBJECT *inas_obj = NULL;
static NFOBJECT *switch_obj[4];
static NFOBJECT *cam_obj[GUI_CHANNEL_CNT];

static NFOBJECT *property[NUM_PROPERTY_ROWS];

static NFOBJECT *g_ipconflict_obj;
static NFOBJECT *g_ipconflict_text_obj[3];
static NFOBJECT *clientList_obj;
static NFOBJECT *disconnect_obj[MAX_SHOWN_CLIENT];

static NFOBJECT *wait_pop = NULL;

static NFOBJECT *g_poe_tot_obj = NULL;
static NFOBJECT *g_poe_hub_tot_obj = NULL;
static NFOBJECT *g_poe_port_obj[GUI_CHANNEL_CNT];
static NFOBJECT *g_update_obj = NULL;
static NFOBJECT *g_detail_obj = NULL;
static NFOBJECT *g_setip_obj = NULL;
static NFOBJECT *g_camip_obj[GUI_CHANNEL_CNT];

static NFOBJECT *g_hub_macaddr = NULL;
static NFOBJECT *g_hub_fwver = NULL;
static NFOBJECT *g_btn_packet = NULL;
static NFOBJECT *g_fix_packet = NULL;

static guint repeat_draw_timer = 0;
static guint start_point = 0;

static guint repeat_draw_timer2 = 0;
static guint start_point2 = 0;

enum {
    CHEAT_FW_UP         = 0,
    CHEAT_FACT_DEFAULT  = 1,
    CHEAT_INSTALL_MODE  = 2,
    CHEAT_SHOW_HUB_INFO  = 3,
    CHEAT_REMOVE_HUB_FAIL  = 4,
    CHEAT_MODE
};

#define FW_UP_CHEAT_SIZE            (3)
#define FACT_DEFAULT_CHEAT_SIZE     (4)
#define INSTALL_CHEAT_SIZE          (3)
#define HUB_INFO_CHEAT_SIZE         (4)
#define REMOVE_HUB_FAIL_CHEAT_SIZE  (4)

static guint cheat_code_Irc200[CHEAT_MODE][8] = {
    { KEYPAD_FREEZE, KEYPAD_LOCK, RMC_AUDIO, 0, 0, 0, 0, 0},
    { RMC_SNAPSHOT, RMC_RESERVE, KEYPAD_REW, KEYPAD_FF, 0, 0, 0, 0},
    { KEYPAD_DISP, KEYPAD_SEQ, RMC_LOG, 0, 0, 0, 0, 0},
    { KEYPAD_CH01, KEYPAD_CH02, KEYPAD_CH03, KEYPAD_CH04, 0, 0, 0, 0},
    { KEYPAD_CH04, KEYPAD_CH02, KEYPAD_CH00, KEYPAD_FREEZE, 0, 0, 0, 0}
};

static guint cheat_code2_IcaItx[CHEAT_MODE][8] = {
    { KEYPAD_SEARCH, KEYPAD_SETUP, KEYPAD_DISP, 0, 0, 0, 0, 0},
    { KEYPAD_ARCH, KEYPAD_SEQ, KEYPAD_REW, KEYPAD_FF, 0, 0, 0, 0},
    { KEYPAD_DISP, KEYPAD_SEQ, RMC_MENU, 0, 0, 0, 0, 0},
    { KEYPAD_CH01, KEYPAD_CH02, KEYPAD_CH03, KEYPAD_CH04, 0, 0, 0, 0},
    { KEYPAD_CH04, KEYPAD_CH02, KEYPAD_CH00, KEYPAD_SEARCH, 0, 0, 0, 0}
};

static int cheat_mode = -1;
static int key_count = 0;

static gboolean _draw_bg_color(NFOBJECT *obj)
{
    GdkDrawable *drawable;
    GdkGC * gc;
    gint size_w, size_h, off_x, off_y;

    drawable = nfui_nfobject_get_window(obj);
    gc = nfui_nfobject_get_gc(obj);

    nfui_nfobject_get_size(obj, &size_w, &size_h);
    nfui_nfobject_get_offset(obj, &off_x, &off_y);

    gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_IDX(200)));
    gdk_draw_rectangle(drawable, gc, TRUE, off_x, off_y, size_w, size_h);

    nfui_nfobject_gc_unref(gc);

    return FALSE;
}

static void _upgrade_camera_fw()
{
    // TODO: UPGRADE CAMERA F/W

    VW_SystemRestart_Open(g_curwnd);
}

static void _factory_default_camera()
{
    gint ret_val;

    ret_val = vw_mbox(g_curwnd, "QUESTION", IMBX_CAMERA_FACTORY_DEFAULT, NFTOOL_MB_OKCANCEL);

    if (ret_val == NFTOOL_MB_OK)
        scm_request_ipcam_factory_default();
}

static void _entered_camera_install_mode()
{
    g_message("%s, %d, called", __FUNCTION__, __LINE__);

    VW_Camera_InstallMode_Open(g_curwnd);
}

static gint _set_hub_info()
{
    gchar strBuf[128];
    unsigned char macaddr[6];
    gchar fwver[64];

    memset(strBuf, 0x00, sizeof(strBuf));
    memset(macaddr, 0x00, sizeof(macaddr));

    if (nf_ipcam_hub_get_macaddr(macaddr)) {
        g_sprintf(strBuf, "%s : %02x:%02x:%02x:%02x:%02x:%02x", lookup_string("MAC ADDRESS"),
        macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
    }
    nfui_nflabel_set_text((NFLABEL*)g_hub_macaddr, strBuf);

    memset(strBuf, 0x00, sizeof(strBuf));
    memset(fwver, 0x00, sizeof(fwver));

    if (nf_ipcam_hub_get_fwver(fwver)) {
        g_sprintf(strBuf, "%s : %s", lookup_string("HUB F/W VERSION"), fwver);
    }
    nfui_nflabel_set_text((NFLABEL*)g_hub_fwver, strBuf);

    return 1;
}

static void _show_hub_info()
{
    g_message("%s, %d, called", __FUNCTION__, __LINE__);

    nfui_nfobject_show(g_hub_macaddr);
    nfui_nfobject_show(g_hub_fwver);
    nfui_signal_emit(g_hub_macaddr, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_hub_fwver, GDK_EXPOSE, TRUE);
}

static gint _remove_hub_fail_info()
{
    gchar strbox[256];
    gchar cmd[128];
    gint ret_val;

    memset(strbox, 0x00, sizeof(strbox));
    g_sprintf(strbox, "%s (%s)", lookup_string("Do you want to delete?"), "hub_fwup_fail_info*");

    ret_val = nftool_mbox(g_curwnd, "CONFIRM", strbox, NFTOOL_MB_OKCANCEL);
    if (ret_val == NFTOOL_MB_CANCEL) return -1;

    g_message("%s, %d, remove hub_fwup_fail_info*", __FUNCTION__, __LINE__);

    memset(cmd, 0x00, sizeof(cmd));
    sprintf(cmd, "rm -f %s*", "/NFDVR/data/hub_fw_image/hub_fwup_fail_info");
    proxy_system(cmd, 1, 3);
    return 0;
}

static gint _get_cheat_val(KEYPAD_KID kpid)
{
    gint i;
    gchar *rc_type = nf_api_param_hw_get_rc_type();

    if (key_count == 0)
    {
        for (i = 0; i < CHEAT_MODE; i++)
        {
            if ((!strcmp(rc_type, "IRC200")) && (cheat_code_Irc200[i][key_count] == kpid))
            {
                cheat_mode = i;
                key_count++;
                break;
            }
            else if ((!strcmp(rc_type, "ICA_ITX")) && (cheat_code2_IcaItx[i][key_count] == kpid))
            {
                cheat_mode = i;
                key_count++;
                break;
            }
        }
    }
    else
    {
        if ((cheat_mode >= 0) && (cheat_mode < CHEAT_MODE))
        {
            if ((!strcmp(rc_type, "IRC200")) && (cheat_code_Irc200[cheat_mode][key_count] == kpid))
            {
                key_count++;
            }
            else if ((!strcmp(rc_type, "ICA_ITX")) && (cheat_code2_IcaItx[cheat_mode][key_count] == kpid))
            {
                key_count++;
            }
            else
            {
                cheat_mode = -1;
                key_count = 0;
            }
        }
        else
        {
            cheat_mode = -1;
            key_count = 0;
        }
    }

    g_message("%s, %d, cheat_mode:%d, key_count:%d", __FUNCTION__, __LINE__, cheat_mode, key_count);

    if ((cheat_mode == CHEAT_FW_UP) && (key_count == FW_UP_CHEAT_SIZE))
    {
        cheat_mode = -1;
        key_count = 0;
        return CHEAT_FW_UP;
    }

    if ((cheat_mode == CHEAT_FACT_DEFAULT) && (key_count == FACT_DEFAULT_CHEAT_SIZE))
    {
        cheat_mode = -1;
        key_count = 0;
        return CHEAT_FACT_DEFAULT;
    }

    if ((cheat_mode == CHEAT_INSTALL_MODE) && (key_count == INSTALL_CHEAT_SIZE))
    {
        cheat_mode = -1;
        key_count = 0;
        return CHEAT_INSTALL_MODE;
    }

    if ((cheat_mode == CHEAT_SHOW_HUB_INFO) && (key_count == HUB_INFO_CHEAT_SIZE))
    {
        cheat_mode = -1;
        key_count = 0;
        return CHEAT_SHOW_HUB_INFO;
    }

    if ((cheat_mode == CHEAT_REMOVE_HUB_FAIL) && (key_count == REMOVE_HUB_FAIL_CHEAT_SIZE))
    {
        cheat_mode = -1;
        key_count = 0;
        return CHEAT_REMOVE_HUB_FAIL;
    }

    return -1;
}

static guint _get_type_client_status(gchar *str)
{
    SESSION_TYPE_E type = SSN_NOTHING;

    if (!strcmp(STR_SSN_LIVE, str))         return SSN_LIVE;
    else if (!strcmp(STR_SSN_PLAY, str))    return SSN_PLAY;
    else if (!strcmp(STR_SSN_ARCH, str))    return SSN_ARCH;
    else if (!strcmp(STR_SSN_NOTHING, str))    return SSN_NOTHING;

    return type;
}

static gint _get_str_client_status(SESSION_TYPE_E type, gchar *status)
{
    if (type == SSN_LIVE)           strcpy(status, STR_SSN_LIVE);
    else if (type == SSN_PLAY)      strcpy(status, STR_SSN_PLAY);
    else if (type == SSN_ARCH)      strcpy(status, STR_SSN_ARCH);
    else if (type == SSN_NOTHING)   strcpy(status, STR_SSN_NOTHING);

    return 0;
}

#if defined(GUI_16CH_SUPPORT) && defined(_SUPPORT_HUB) && (!defined(_IPX_1648P4E) && !defined(_IPX_1648M4E))
static gboolean _repeat_draw_hub_line(gpointer data)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc = NULL;
    gint8 dash_style[] = {5, 5};

    gint start_x, start_y;
    gint end_x, end_y;
    GdkPoint points[4];

    drawable = nfui_nfobject_get_window(internet_obj);
    if (drawable == NULL) return TRUE;

    gc = nfui_nfobject_get_gc(internet_obj);
    if (gc == NULL) return TRUE;

    if (start_point2)
        start_point2 = 0;
    else
        start_point2 = 5;

    start_x = (gint)port_obj[PORT_LAN]->x + (gint)port_obj[PORT_LAN]->width/2;
    start_y = (gint)dvr_obj->y + (gint)dvr_obj->height;

    end_x = (gint)port_obj[PORT_HUB_WAN]->x + (gint)port_obj[PORT_HUB_WAN]->width/2;
    end_y = (gint)hub_obj->y;

    points[0].x = start_x;
    points[0].y = start_y;
    points[1].x = start_x;
    points[1].y = start_y+(end_y-start_y)/2;
    points[2].x = end_x;
    points[2].y = start_y+(end_y-start_y)/2;
    points[3].x = end_x;
    points[3].y = end_y;

    gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(200));
    gdk_gc_set_line_attributes(gc, 2, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);

    gdk_draw_lines(drawable, gc, points, 4);

    gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(403));
    gdk_gc_set_line_attributes(gc, 2, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_ROUND);
    gdk_gc_set_dashes (gc, start_point2, dash_style, G_N_ELEMENTS(dash_style));

    gdk_draw_lines(drawable, gc, points, 4);

    nfui_nfobject_gc_unref(gc);

    return TRUE;
}

static void _repeat_draw_hub_connecting_line()
{
    if (repeat_draw_timer2)
    {
        g_source_remove(repeat_draw_timer2);
        repeat_draw_timer2 = 0;
    }

    repeat_draw_timer2 = g_timeout_add(100, _repeat_draw_hub_line, NULL);
}

static void _stop_draw_hub_connecting_line()
{
    if (repeat_draw_timer2)
    {
        g_source_remove(repeat_draw_timer2);
        repeat_draw_timer2 = 0;
    }
}
#endif

static gboolean _repeat_draw_ipcam_line(gpointer data)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc = NULL;
    gint8 dash_style[] = {5, 5};

    gint repeat_ch;
    guint ch_mask;

    ch_mask = GPOINTER_TO_UINT(data);

    if (!ch_mask)
    {
        g_message("%s, %d", __FUNCTION__, __LINE__);
        g_assert(0);
    }

    drawable = nfui_nfobject_get_window(internet_obj);
    if (drawable == NULL) return TRUE;

    gc = nfui_nfobject_get_gc(internet_obj);
    if (gc == NULL) return TRUE;

    if (start_point)
        start_point = 0;
    else
        start_point = 5;

    for (repeat_ch = 0; repeat_ch < GUI_CHANNEL_CNT; repeat_ch++)
    {
        if(ch_mask & (1 << repeat_ch))
        {
            GdkPoint points[4];

            gint start_x, start_y;
            gint end_x, end_y;
            gint size_w, size_h;

            nfui_get_image_size(IMG_NET_PORT_OFF, &size_w, &size_h);

#if defined (GUI_32CH_SUPPORT)
            start_x = (gint)port_obj[_TRANS_CH(repeat_ch)]->x + (gint)port_obj[_TRANS_CH(repeat_ch)]->width/2;
            if (repeat_ch < 16)
                start_y = switch_obj[0]->y;
            else
                start_y = switch_obj[2]->y + switch_obj[2]->height;

            points[0].x = start_x;
            points[0].y = start_y;
            
            points[1].x = start_x;

            if (repeat_ch < 16)
            {
                if (repeat_ch < 4) points[1].y = start_y - 15 - (repeat_ch * 6);
                else if (repeat_ch < 8) points[1].y = start_y - 33 + ((repeat_ch-4) * 6);
                else if (repeat_ch < 12) points[1].y = start_y - 15 - ((repeat_ch-8) * 6);
                else points[1].y = start_y - 33 + ((repeat_ch-12) * 6);
            }
            else
            {
                if (repeat_ch < 20) points[1].y = start_y + 15 + ((repeat_ch-16) * 6);
                else if (repeat_ch < 24) points[1].y = start_y + 33 - ((repeat_ch-20) * 6);
                else if (repeat_ch < 28) points[1].y = start_y + 15 + ((repeat_ch-24) * 6);
                else points[1].y = start_y + 33 - ((repeat_ch-28) * 6);
            }

            end_x = (gint)cam_obj[repeat_ch]->x + (gint)cam_obj[repeat_ch]->width/2;

            points[2].x = end_x;
            points[2].y = points[1].y;

            if (repeat_ch < 16)
                end_y = (gint)cam_obj[repeat_ch]->y + cam_obj[repeat_ch]->height;
            else
                end_y = (gint)cam_obj[repeat_ch]->y;
            
            points[3].x = end_x;
            points[3].y = end_y;
#else
            if (repeat_ch < 8)
            {
                start_x = (gint)port_obj[_TRANS_CH(repeat_ch)]->x + size_w/2;
                start_y = (gint)dvr_obj->y;

                points[0].x = start_x;
                points[0].y = start_y;
                points[1].x = start_x;
                points[1].y = start_y - 15 - MAP_LINE_GAP_H*(MAP_NUM_CAM_COLS-repeat_ch);

                nfui_get_image_size(IMG_NET_CAMERA_OFF, &size_w, &size_h);

                end_x = (gint)cam_obj[repeat_ch]->x + size_w/2;
                end_y = (gint)cam_obj[repeat_ch]->y + size_h;

                points[2].x = end_x;
                points[2].y = start_y - 15 - MAP_LINE_GAP_H*(MAP_NUM_CAM_COLS-repeat_ch);
                points[3].x = end_x;
                points[3].y = end_y;
            }
            else
            {
                gint tmp_ch = repeat_ch - 8;

#if defined(GUI_16CH_SUPPORT) && defined(_SUPPORT_HUB) && (!defined(_IPX_1648P4E) && !defined(_IPX_1648M4E))
                start_x = (gint)port_obj[_TRANS_CH(repeat_ch)]->x + size_w/2;
                start_y = (gint)hub_obj->y;

                points[0].x = start_x;
                points[0].y = start_y;
                points[1].x = start_x;
                points[1].y = start_y - 15 - MAP_LINE_GAP_H*(MAP_NUM_CAM_COLS-tmp_ch);

                nfui_get_image_size(IMG_NET_CAMERA_OFF, &size_w, &size_h);

                end_x = (gint)cam_obj[repeat_ch]->x + size_w/2;
                end_y = (gint)cam_obj[repeat_ch]->y + size_h;

                points[2].x = end_x;
                points[2].y = start_y - 15 - MAP_LINE_GAP_H*(MAP_NUM_CAM_COLS-tmp_ch);
                points[3].x = end_x;
                points[3].y = end_y;
#else
                start_x = (gint)port_obj[_TRANS_CH(repeat_ch)]->x + size_w/2;
                start_y = (gint)dvr_obj->y + (gint)dvr_obj->height;

                points[0].x = start_x;
                points[0].y = start_y;
                points[1].x = start_x;
                points[1].y = start_y + 15 + MAP_LINE_GAP_H*(MAP_NUM_CAM_COLS-tmp_ch);

                nfui_get_image_size(IMG_NET_CAMERA_OFF, &size_w, &size_h);

                end_x = (gint)cam_obj[repeat_ch]->x + size_w/2;
                end_y = (gint)cam_obj[repeat_ch]->y;

                points[2].x = end_x;
                points[2].y = start_y + 15 + MAP_LINE_GAP_H*(MAP_NUM_CAM_COLS-tmp_ch);
                points[3].x = end_x;
                points[3].y = end_y;
#endif
            }
#endif // defined (GUI_32CH_SUPPORT)

            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(200));
            gdk_gc_set_line_attributes(gc, 2, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);

            gdk_draw_lines(drawable, gc, points, 4);

            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(403));
            gdk_gc_set_line_attributes(gc, 2, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_ROUND);
            gdk_gc_set_dashes (gc, start_point, dash_style, G_N_ELEMENTS(dash_style));

            gdk_draw_lines(drawable, gc, points, 4);
        }
    }

    nfui_nfobject_gc_unref(gc);

    return TRUE;
}

static void _repeat_draw_ipcam_connecting_line(gint repeat_ch)
{
    guint ch_mask;

    ch_mask = GPOINTER_TO_UINT(nfui_nfobject_get_data(dvr_obj, "connect status"));

    if (!(ch_mask & (1 << repeat_ch)))
    {
        ch_mask |= (1 << repeat_ch);
        nfui_nfobject_set_data(dvr_obj, "connect status", GUINT_TO_POINTER(ch_mask));

        if (repeat_draw_timer)
        {
            g_source_remove(repeat_draw_timer);
            repeat_draw_timer = 0;
        }

        repeat_draw_timer = g_timeout_add(100, _repeat_draw_ipcam_line, GUINT_TO_POINTER(ch_mask));
    }
}

static void _stop_draw_ipcam_connecting_line(gint stop_ch)
{
    guint ch_mask;

    ch_mask = GPOINTER_TO_UINT(nfui_nfobject_get_data(dvr_obj, "connect status"));

    if (ch_mask & (1 << stop_ch))
    {
        ch_mask &= ~(1 << stop_ch);
        nfui_nfobject_set_data(dvr_obj, "connect status", GUINT_TO_POINTER(ch_mask));

        if (repeat_draw_timer)
        {
            g_source_remove(repeat_draw_timer);
            repeat_draw_timer = 0;

            if (ch_mask)
                repeat_draw_timer = g_timeout_add(100, _repeat_draw_ipcam_line, GUINT_TO_POINTER(ch_mask));
        }
    }
}

static void _draw_map_line(const GdkPoint *points, gint n_point, NET_MAP_E status)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc = NULL;
    gint8 dash_style[] = {5, 5};

    drawable = nfui_nfobject_get_window(internet_obj);
    if (drawable == NULL) return;

    gc = nfui_nfobject_get_gc(internet_obj);
    if (gc == NULL) return;


    if (status == NET_MAP_CONNECTING)
    {
        gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(403));
        gdk_gc_set_line_attributes(gc, 2, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_ROUND);
        gdk_gc_set_dashes (gc, 0, dash_style, G_N_ELEMENTS(dash_style));
    }
    else
    {
        if (status == NET_MAP_CONNECT)
            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(400));
        else if (status == NET_MAP_DISCONNECT)
            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(401));
        else if (status == NET_MAP_ERROR)
            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(402));
        else if (status == NET_MAP_FOCUS)
            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(404));

        gdk_gc_set_line_attributes(gc, 2, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);
    }

    gdk_draw_lines(drawable, gc, points, n_point);
    nfui_nfobject_gc_unref(gc);

}

static void _set_internet_line_status(NET_MAP_E status)
{
    GdkPoint points[2];

    gint start_x, start_y;
    gint end_x, end_y;

    start_x = (gint)internet_obj->x + (gint)internet_obj->width;
    start_y = (gint)internet_obj->y + (gint)internet_obj->height/2;

    end_x = (gint)gateway_obj->x;
    end_y = start_y;

    points[0].x = start_x;
    points[0].y = start_y;
    points[1].x = end_x;
    points[1].y = end_y;

    _draw_map_line(points, 2, status);
}

static void _set_gateway_line_status(NET_MAP_E status)
{
    gint start_x, start_y;
    gint end_x, end_y;
    gint i;

#if defined (GUI_32CH_SUPPORT)    
    GdkPoint points[4][3];

    for (i = 0; i < 4; i++)
    {
        start_x = (gint)gateway_obj->x + (gint)gateway_obj->width;
        if (i == 0)
            start_y = (gint)gateway_obj->y + (gint)gateway_obj->height/2 - 12;
        else if (i == 1)    
            start_y = (gint)gateway_obj->y + (gint)gateway_obj->height/2 - 4;
        else if (i == 2)
            start_y = (gint)gateway_obj->y + (gint)gateway_obj->height/2 + 12;
        else
            start_y = (gint)gateway_obj->y + (gint)gateway_obj->height/2 + 4;

        points[i][0].x = start_x;
        points[i][0].y = start_y;

        points[i][1].x = switch_obj[i]->x + 16;
        points[i][1].y = points[i][0].y;

        end_x = points[i][1].x;
        if (i < 2)
            end_y = switch_obj[i]->y + switch_obj[i]->height;
        else
            end_y = switch_obj[i]->y;

        points[i][2].x = end_x;
        points[i][2].y = end_y;

        _draw_map_line(points[i], 3, status);
    }
#else
    GdkPoint points[2];

    start_x = (gint)gateway_obj->x + (gint)gateway_obj->width;
    start_y = (gint)gateway_obj->y + (gint)gateway_obj->height/2;

    end_x = (gint)port_obj[PORT_WAN]->x;
    end_y = start_y;

    points[0].x = start_x;
    points[0].y = start_y;
    points[1].x = end_x;
    points[1].y = end_y;

    _draw_map_line(points, 2, status);
#endif
}

static void _set_inas_line_status(NET_MAP_E status)
{
    GdkPoint points[3];

    gint start_x, start_y;
    gint end_x, end_y;

    start_x = (gint)inas_obj->x + (gint)inas_obj->width;
    start_y = (gint)inas_obj->y + 30;

    end_x = (gint)port_obj[PORT_LAN]->x + (gint)port_obj[PORT_LAN]->width/2;
    end_y = (gint)port_obj[PORT_LAN]->y + (gint)port_obj[PORT_LAN]->height;

    points[0].x = start_x;
    points[0].y = start_y;
    points[1].x = end_x;
    points[1].y = start_y;
    points[2].x = end_x;
    points[2].y = end_y;

    _draw_map_line(points, 3, status);
}

#if defined(GUI_16CH_SUPPORT) && defined(_SUPPORT_HUB) && (!defined(_IPX_1648P4E) && !defined(_IPX_1648M4E))
static void _set_hub_line_status(NET_MAP_E status)
{
    GdkPoint points[4];

    gint start_x, start_y;
    gint end_x, end_y;

    start_x = (gint)port_obj[PORT_LAN]->x + (gint)port_obj[PORT_LAN]->width/2;
    start_y = (gint)dvr_obj->y + (gint)dvr_obj->height;

    end_x = (gint)port_obj[PORT_HUB_WAN]->x + (gint)port_obj[PORT_HUB_WAN]->width/2;
    end_y = (gint)hub_obj->y;

    points[0].x = start_x;
    points[0].y = start_y;
    points[1].x = start_x;
    points[1].y = start_y+(end_y-start_y)/2;
    points[2].x = end_x;
    points[2].y = start_y+(end_y-start_y)/2;
    points[3].x = end_x;
    points[3].y = end_y;

    _draw_map_line(points, 4, status);
}
#endif

static void _set_ipcam_line_status(gint ch, NET_MAP_E status)
{
    GdkPoint points[4];

    gint start_x, start_y;
    gint end_x, end_y;
    
#if defined (GUI_32CH_SUPPORT)
        start_x = (gint)port_obj[_TRANS_CH(ch)]->x + (gint)port_obj[_TRANS_CH(ch)]->width/2;
        if (ch < 16)
            start_y = switch_obj[0]->y;
        else
            start_y = switch_obj[2]->y + switch_obj[2]->height;

        points[0].x = start_x;
        points[0].y = start_y;
        
        points[1].x = start_x;

        if (ch < 16)
        {
            if (ch < 4) points[1].y = start_y - 15 - (ch * 6);
            else if (ch < 8) points[1].y = start_y - 33 + ((ch-4) * 6);
            else if (ch < 12) points[1].y = start_y - 15 - ((ch-8) * 6);
            else points[1].y = start_y - 33 + ((ch-12) * 6);
        }
        else
        {
            if (ch < 20) points[1].y = start_y + 15 + ((ch-16) * 6);
            else if (ch < 24) points[1].y = start_y + 33 - ((ch-20) * 6);
            else if (ch < 28) points[1].y = start_y + 15 + ((ch-24) * 6);
            else points[1].y = start_y + 33 - ((ch-28) * 6);
        }

        end_x = (gint)cam_obj[ch]->x + (gint)cam_obj[ch]->width/2;

        points[2].x = end_x;
        points[2].y = points[1].y;

        if (ch < 16)
            end_y = (gint)cam_obj[ch]->y + (gint)cam_obj[ch]->height;
        else
            end_y = (gint)cam_obj[ch]->y;
        
        points[3].x = end_x;
        points[3].y = end_y;
#else
    if (ch < 8)
    {
        start_x = (gint)port_obj[_TRANS_CH(ch)]->x + (gint)port_obj[_TRANS_CH(ch)]->width/2;
        start_y = (gint)dvr_obj->y;

        points[0].x = start_x;
        points[0].y = start_y;
        points[1].x = start_x;
        points[1].y = start_y - 15 - MAP_LINE_GAP_H*(MAP_NUM_CAM_COLS-ch);

        end_x = (gint)cam_obj[ch]->x + (gint)cam_obj[ch]->width/2;
        end_y = (gint)cam_obj[ch]->y + (gint)cam_obj[ch]->height;

        points[2].x = end_x;
        points[2].y = start_y - 15 - MAP_LINE_GAP_H*(MAP_NUM_CAM_COLS-ch);
        points[3].x = end_x;
        points[3].y = end_y;
    }
    else
    {
        gint tmp_ch = ch - 8;

#if defined(GUI_16CH_SUPPORT) && defined(_SUPPORT_HUB) && (!defined(_IPX_1648P4E) && !defined(_IPX_1648M4E))
        start_x = (gint)port_obj[_TRANS_CH(ch)]->x + (gint)port_obj[_TRANS_CH(ch)]->width/2;
        start_y = (gint)hub_obj->y;

        points[0].x = start_x;
        points[0].y = start_y;
        points[1].x = start_x;
        points[1].y = start_y - 15 - MAP_LINE_GAP_H*(MAP_NUM_CAM_COLS-tmp_ch);

        end_x = (gint)cam_obj[ch]->x + (gint)cam_obj[ch]->width/2;
        end_y = (gint)cam_obj[ch]->y + (gint)cam_obj[ch]->height;

        points[2].x = end_x;
        points[2].y = start_y - 15 - MAP_LINE_GAP_H*(MAP_NUM_CAM_COLS-tmp_ch);
        points[3].x = end_x;
        points[3].y = end_y;
#else
        start_x = (gint)port_obj[_TRANS_CH(ch)]->x + (gint)port_obj[_TRANS_CH(ch)]->width/2;
        start_y = (gint)dvr_obj->y + (gint)dvr_obj->height;

        points[0].x = start_x;
        points[0].y = start_y;
        points[1].x = start_x;
        points[1].y = start_y + 15 + MAP_LINE_GAP_H*(MAP_NUM_CAM_COLS-tmp_ch);

        end_x = (gint)cam_obj[ch]->x + (gint)cam_obj[ch]->width/2;
        end_y = (gint)cam_obj[ch]->y;

        points[2].x = end_x;
        points[2].y = start_y + 15 + MAP_LINE_GAP_H*(MAP_NUM_CAM_COLS-tmp_ch);
        points[3].x = end_x;
        points[3].y = end_y;
#endif
    }
#endif  // defined (GUI_32CH_SUPPORT)

    _draw_map_line(points, 4, status);
}

static void _expose_network_line_status()
{
    gint ch;
    guint status;

    status = GPOINTER_TO_UINT(nfui_nfobject_get_data(internet_obj, "connect status"));
    _set_internet_line_status(status);

    status = GPOINTER_TO_UINT(nfui_nfobject_get_data(gateway_obj, "connect status"));
    _set_gateway_line_status(status);

//  status = GPOINTER_TO_UINT(nfui_nfobject_get_data(inas_obj, "connect status"));
//  _set_inas_line_status(status);

#if defined(GUI_16CH_SUPPORT) && defined(_SUPPORT_HUB) && (!defined(_IPX_1648P4E) && !defined(_IPX_1648M4E))
    status = GPOINTER_TO_UINT(nfui_nfobject_get_data(hub_obj, "connect status"));
    _set_hub_line_status(status);
#endif

    for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
    {
        status = GPOINTER_TO_UINT(nfui_nfobject_get_data(cam_obj[ch], "connect status"));
        _set_ipcam_line_status(ch, status);
    }
}

static void _change_port_obj(gint port_index, NET_MAP_E status)
{
    if (status != NET_MAP_FOCUS)
        nfui_nfobject_set_data(port_obj[port_index], "connect status", GUINT_TO_POINTER(status));

    if (status == NET_MAP_CONNECT)
        nfui_nfimage_change_image((NFIMAGE*)port_obj[port_index], IMG_NET_PORT_ON);
    else if (status == NET_MAP_DISCONNECT)
        nfui_nfimage_change_image((NFIMAGE*)port_obj[port_index], IMG_NET_PORT_OFF);
    else if (status == NET_MAP_ERROR)
        nfui_nfimage_change_image((NFIMAGE*)port_obj[port_index], IMG_NET_PORT_ERROR);
    else if (status == NET_MAP_CONNECTING)
        nfui_nfimage_change_image((NFIMAGE*)port_obj[port_index], IMG_NET_PORT_OFF);
    else if (status == NET_MAP_FOCUS)
        nfui_nfimage_change_image((NFIMAGE*)port_obj[port_index], IMG_NET_PORT_FOCUS);
}

static void _change_cam_obj(gint ch, NET_MAP_E status)
{
    if (status != NET_MAP_FOCUS)
        nfui_nfobject_set_data(cam_obj[ch], "connect status", GUINT_TO_POINTER(status));

    if (status == NET_MAP_CONNECT)
        nfui_nfimage_change_image((NFIMAGE*)cam_obj[ch], IMG_NET_CAMERA_ON);
    else if (status == NET_MAP_DISCONNECT)
        nfui_nfimage_change_image((NFIMAGE*)cam_obj[ch], IMG_NET_CAMERA_OFF);
    else if (status == NET_MAP_ERROR)
        nfui_nfimage_change_image((NFIMAGE*)cam_obj[ch], IMG_NET_CAMERA_ERROR);
    else if (status == NET_MAP_CONNECTING)
        nfui_nfimage_change_image((NFIMAGE*)cam_obj[ch], IMG_NET_CAMERA_CONNECTING);
    else if (status == NET_MAP_FOCUS)
        nfui_nfimage_change_image((NFIMAGE*)cam_obj[ch], IMG_NET_CAMERA_FOCUS);
}

static gint _set_sequrinet_status(gboolean enable)
{
    if(!var_get_supported_sequrinet()) return 0;

    if(enable)
    {
        nfui_nflabel_set_text((NFLABEL*)property[PROPERTY_SEQURINET], "OK");
        nfui_signal_emit((NFLABEL*)property[PROPERTY_SEQURINET], GDK_EXPOSE, TRUE);
        nfui_nfobject_enable(g_detail_obj);
        nfui_signal_emit(g_detail_obj, GDK_EXPOSE, TRUE);
    }
    else
    {
        nfui_nflabel_set_text((NFLABEL*)property[PROPERTY_SEQURINET], "DISABLE");
        nfui_signal_emit((NFLABEL*)property[PROPERTY_SEQURINET], GDK_EXPOSE, TRUE);
        nfui_nfobject_disable(g_detail_obj);
        nfui_signal_emit(g_detail_obj, GDK_EXPOSE, TRUE);
    }

    return 0;
}

static void _set_net_status_property()
{
    IPSetupData ipdata;
	IPv6Data ipv6data;
    NF_NETIF_GET_INFO netif_info;

    NF_NOTIFY_INFO info;
    NF_DDNS_STATUS ddns_status;

    gchar strTemp[64];
    gchar strTemp1[256];
    gboolean auth_sequrinet;
	gboolean sequrinet_enable = FALSE;

    DAL_get_ipSetup_data(&ipdata);
	DAL_get_ipv6_data(&ipv6data);
    scm_get_sys_netinfo(&netif_info);
    scm_get_ddns_status_event_data(&info);

// IP ADDRESS
    memset(strTemp, 0x00, sizeof(strTemp));

    sprintf(strTemp, "%d.%d.%d.%d",
            ((netif_info.ipaddr & 0xff000000)>>24), ((netif_info.ipaddr & 0xff0000)>>16),
            ((netif_info.ipaddr & 0xff00)>>8), (netif_info.ipaddr & 0xff));

    if (ipdata.dhcp)
    {
        strcat(strTemp, "(DHCP)");
    }
    nfui_nflabel_set_text((NFLABEL*)property[PROPERTY_IP], strTemp);

// IPv6 ADDRESS
	memset(strTemp, 0x00, sizeof(strTemp));

    sprintf(strTemp, netif_info.ipv6_linklocal);
    if (ipv6data.use == 2)
        strcat(strTemp, "(AUTO)");

	nfui_nflabel_set_text((NFLABEL*)property[PROPERTY_IPV6], strTemp);


// MAC ADDRESS
    memset(strTemp, 0x00, sizeof(strTemp));
    g_sprintf(strTemp,"%02x-%02x-%02x-%02x-%02x-%02x",
                (guchar)netif_info.mac_addr[0],
                (guchar)netif_info.mac_addr[1],
                (guchar)netif_info.mac_addr[2],
                (guchar)netif_info.mac_addr[3],
                (guchar)netif_info.mac_addr[4],
                (guchar)netif_info.mac_addr[5]);

    nfui_nflabel_set_text((NFLABEL*)property[PROPERTY_MAC], strTemp);

// DDNS ADDRESS
    memset(strTemp1, 0x00, sizeof(strTemp1));
    scm_get_dvr_addr_str(strTemp1, 256);
    if (strcmp(strTemp1, "N/A") != 0) ifn_tolower(strTemp1);
    nfui_nflabel_set_text((NFLABEL*)property[PROPERTY_DDNS], strTemp1);

// RTSP SERVICE PORT
    memset(strTemp, 0x00, sizeof(strTemp));
    g_sprintf(strTemp, "%d", ipdata.rtspport);
    nfui_nflabel_set_text((NFLABEL*)property[PROPERTY_RTSP], strTemp);

// WEB SERVICE PORT
    memset(strTemp, 0x00, sizeof(strTemp));
    g_sprintf(strTemp, "%d", ipdata.webPort);
    nfui_nflabel_set_text((NFLABEL*)property[PROPERTY_WEB], strTemp);

// DDNS UPDATE STATUS
/*
    memset(strTemp, 0x00, sizeof(strTemp));

    if (var_get_supported_ddns()) {
        if (info.d.params[0] == 0)
        {
            memset(strTemp, 0x00, sizeof(strTemp));
            ddns_get_status(&ddns_status);

            dtf_get_local_datetime(ddns_status.update_success_tv.tv_sec, strTemp);
            nfui_nflabel_set_text((NFLABEL*)property[PROPERTY_DDNS_STATUS], strTemp);
            nfui_nflabel_set_pango_font((NFLABEL*)property[PROPERTY_DDNS_STATUS], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(389));
        }
        else
        {
            nfui_nflabel_set_text((NFLABEL*)property[PROPERTY_DDNS_STATUS], "FAIL");
            nfui_nflabel_set_pango_font((NFLABEL*)property[PROPERTY_DDNS_STATUS], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(399));
        }
    }
    else {
        nfui_nflabel_set_text((NFLABEL*)property[PROPERTY_DDNS_STATUS], "N/A");
        nfui_nflabel_set_pango_font((NFLABEL*)property[PROPERTY_DDNS_STATUS], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(389));
    }
*/

// EXTERNAL IP ADDRESS
    scm_req_wan_ip(INFY_DISP_WAN_IP);

// SEQURINET
    scm_get_wan_status_event_data(&info);
    DAL_get_Sequrinet_Status(&sequrinet_enable);
    auth_sequrinet = ssm_check_access_auth(USR_AUTH_SEQURINET);

    if (auth_sequrinet)
    {
        if(!info.d.params[0] & sequrinet_enable){
        _set_sequrinet_status(TRUE);
    }
    else{
        _set_sequrinet_status(FALSE);
    }
    }
    else
    {
        _set_sequrinet_status(FALSE);
    }
}

static gboolean _set_net_connect_clients()
{
    SESSION_LIST_T *session_list = NULL;
    gint i = 0, j, session_cnt;

    UserManageData userdata;
    guint tmp, user_cnt, box_cnt;
    gint is_matched;

    gchar login_group[10];
    gchar *strBuf[NUM_CLIENT_FIELD_COLS];

    memset(login_group, 0x00, sizeof(login_group));
    ssm_get_cur_group(login_group);

    tmp = GPOINTER_TO_UINT(nfui_nfobject_get_data(clientList_obj, "client count"));

    session_list = scm_new_net_session_list(&session_cnt);

    if ((tmp == 0) && (session_cnt == 0)) return FALSE;

    if (tmp == session_cnt)
    {
        for (i = 0; i < session_cnt; i++)
        {
            gchar *str_id;
            gchar *str_addr;
            gchar *str_act;
            SESSION_TYPE_E type;

            str_id = nfui_listbox_get_text_of_list((NFLISTBOX*)clientList_obj, i, CLIENT_FIELD_ID);
            str_addr = nfui_listbox_get_text_of_list((NFLISTBOX*)clientList_obj, i, CLIENT_FIELD_IP);
            str_act = nfui_listbox_get_text_of_list((NFLISTBOX*)clientList_obj, i, CLIENT_FIELD_ACTION);

            if (!str_id) break;
            if (!str_addr) break;
            if (!str_act) break;

            type = _get_type_client_status(str_act);

            if (strcmp(session_list[i].user_id, str_id)) break;
            if (strcmp(session_list[i].addr, str_addr)) break;
            if (session_list[i].type != type) break;
        }

        if (i == session_cnt)
        {
            scm_free_net_session_list(session_list);
            return FALSE;
        }
    }

    nfui_listbox_delete_all((NFLISTBOX*)clientList_obj);

    for (i = 0; i < MAX_SHOWN_CLIENT; i++)
        nfui_nfobject_disable(disconnect_obj[i]);

    if (session_list)
    {
        user_cnt = DAL_get_user_count();

        for (i = 0; i < session_cnt; i++)
        {
            is_matched = 0;

            for (j = 0; j < user_cnt; j++)
            {
                DAL_get_userManage_data(&userdata, j);

                if (strcmp(userdata.id, session_list[i].user_id) == 0)
                {
                    is_matched = 1;
                        break;
                    }
            }

            if (is_matched)
            {
                strBuf[CLIENT_FIELD_ID] = imalloc(sizeof(gchar)*128);
                strcpy(strBuf[CLIENT_FIELD_ID], userdata.id);

                strBuf[CLIENT_FIELD_GROUP] = imalloc(sizeof(gchar)*128);
                strcpy(strBuf[CLIENT_FIELD_GROUP], userdata.group);

                strBuf[CLIENT_FIELD_IP] = imalloc(sizeof(gchar)*128);
                strcpy(strBuf[CLIENT_FIELD_IP], session_list[i].addr);

                strBuf[CLIENT_FIELD_ACTION] = imalloc(sizeof(gchar)*128);
                _get_str_client_status(session_list[i].type, strBuf[CLIENT_FIELD_ACTION]);

                nfui_listbox_set_text((NFLISTBOX*)clientList_obj, strBuf);
				nfui_nfobject_support_multi_lang(clientList_obj, FALSE);

                ifree(strBuf[CLIENT_FIELD_ID]);
                ifree(strBuf[CLIENT_FIELD_GROUP]);
                ifree(strBuf[CLIENT_FIELD_IP]);
                ifree(strBuf[CLIENT_FIELD_ACTION]);

                if (i < MAX_SHOWN_CLIENT)
                {
                    if ((strcmp(userdata.group, "ADMIN") != 0) && (strcmp(login_group, "ADMIN") == 0))
                        nfui_nfobject_enable(disconnect_obj[i]);
                }
            }
        }

        scm_free_net_session_list(session_list);
    }

    nfui_nfobject_set_data(clientList_obj, "client count", GUINT_TO_POINTER(session_cnt));

    return TRUE;
}

static void _refresh_disconnect_button()
{
    gchar login_group[10];
    gchar *str_group;
    gint i;
    gboolean is_enable;

    memset(login_group, 0x00, sizeof(login_group));
    ssm_get_cur_group(login_group);

    for (i = 0; i < MAX_SHOWN_CLIENT; i++)
        nfui_nfobject_disable(disconnect_obj[i]);

    if (strcmp(login_group, "ADMIN") != 0) return FALSE;

    for (i = 0; i < MAX_SHOWN_CLIENT; i++)
    {
        str_group = nfui_listbox_get_text_of_box((NFLISTBOX*)clientList_obj, i, CLIENT_FIELD_GROUP);

        if (!str_group) continue;

        if (strcmp(str_group, "ADMIN") != 0)
        {
            nfui_nfobject_enable(disconnect_obj[i]);
        }
    }
}

static void _update_network_data()
{
    IPSetupData ipdata;
    NF_NETIF_GET_INFO netif_info;
	NF_NOTIFY_INFO info;	

    gchar strTemp[64];
    gchar strTemp1[256];
    gboolean auth_sequrinet;
	gboolean sequrinet_enable = FALSE;

    DAL_get_ipSetup_data(&ipdata);
    nf_netif_get_info(&netif_info);

    memset(strTemp, 0x00, sizeof(strTemp));
    sprintf(strTemp, "%d.%d.%d.%d",
            ((netif_info.ipaddr & 0xff000000)>>24), ((netif_info.ipaddr & 0xff0000)>>16),
            ((netif_info.ipaddr & 0xff00)>>8), (netif_info.ipaddr & 0xff));

    if (ipdata.dhcp)
        strcat(strTemp, "(DHCP)");

    nfui_nflabel_set_text((NFLABEL*)property[PROPERTY_IP], strTemp);
    nfui_signal_emit(property[PROPERTY_IP], GDK_EXPOSE, TRUE);

// DDNS ADDRESS
    memset(strTemp1, 0x00, sizeof(strTemp1));
    scm_get_dvr_addr_str(strTemp1, 256);
    if (strcmp(strTemp1, "N/A") != 0) ifn_tolower(strTemp1);
    nfui_nflabel_set_text((NFLABEL*)property[PROPERTY_DDNS], strTemp1);
    nfui_signal_emit(property[PROPERTY_DDNS], GDK_EXPOSE, TRUE);

// RTSP SERVICE PORT
    memset(strTemp, 0x00, sizeof(strTemp));
    g_sprintf(strTemp, "%d", ipdata.rtspport);
    nfui_nflabel_set_text((NFLABEL*)property[PROPERTY_RTSP], strTemp);
    nfui_signal_emit(property[PROPERTY_RTSP], GDK_EXPOSE, TRUE);

// WEB SERVICE PORT
    memset(strTemp, 0x00, sizeof(strTemp));
    g_sprintf(strTemp, "%d", ipdata.webPort);
    nfui_nflabel_set_text((NFLABEL*)property[PROPERTY_WEB], strTemp);
    nfui_signal_emit(property[PROPERTY_WEB], GDK_EXPOSE, TRUE);

// SEQURINET
    scm_get_wan_status_event_data(&info);
    DAL_get_Sequrinet_Status(&sequrinet_enable);
    auth_sequrinet = ssm_check_access_auth(USR_AUTH_SEQURINET);

    if (auth_sequrinet)
    {
        if(!info.d.params[0] & sequrinet_enable){
            _set_sequrinet_status(TRUE);
        }
        else{
            _set_sequrinet_status(FALSE);
        }
    }
    else
    {
        _set_sequrinet_status(FALSE);
    }

    if (!var_get_supported_ddns()) nfui_nflabel_set_text((NFLABEL*)property[PROPERTY_DDNS_STATUS], "N/A");
    else if (!DAL_is_ddns_on()) nfui_nflabel_set_text((NFLABEL*)property[PROPERTY_DDNS_STATUS], "OFF");
    else nfui_nflabel_set_text((NFLABEL*)property[PROPERTY_DDNS_STATUS], "Please wait...");
    nfui_nflabel_set_pango_font((NFLABEL*)property[PROPERTY_DDNS_STATUS], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(389));
    nfui_signal_emit(property[PROPERTY_DDNS_STATUS], GDK_EXPOSE, TRUE);

    if (!var_get_supported_ddns()) nfui_nfobject_disable(g_update_obj);
    else if (!DAL_is_ddns_on()) nfui_nfobject_disable(g_update_obj);
    else nfui_nfobject_enable(g_update_obj);
    nfui_signal_emit(g_update_obj, GDK_EXPOSE, TRUE);
}

static void _update_poe_port_data(NFOBJECT *obj, gint ch, gfloat val)
{
    gchar strTemp[10];
    EA_SysSysData ssd;

    memset(strTemp, 0x00, sizeof(strTemp));
    if (val == 0)       g_sprintf(strTemp, "%dW", 0);
    else if (val < 10)  g_sprintf(strTemp, "%.1fW", val);
    else                g_sprintf(strTemp, "%dW", (gint)val);

    DAL_get_SysSys_Data(&ssd, POE_FAIL_EVT_DATA);

    if ((val*100/PER_PORT) < ssd.failPoe)
        nfui_nflabel_set_pango_font((NFLABEL*)obj, nffont_get_pango_font(NFFONT_MINI_SEMI_1), COLOR_IDX(389));
    else
        nfui_nflabel_set_pango_font((NFLABEL*)obj, nffont_get_pango_font(NFFONT_MINI_SEMI_1), COLOR_IDX(399));

    nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
}

static gboolean post_cam_obj_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    guint ch;
    guint status;
    guint pos_x, pos_y, off_x, off_y;
    guint size_w, size_h;

    if ((evt->type == GDK_ENTER_NOTIFY) || (evt->type == GDK_LEAVE_NOTIFY)) {

        for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
        {
            if (cam_obj[ch] == obj)
                break;
        }

        status = GPOINTER_TO_UINT(nfui_nfobject_get_data(cam_obj[ch], "connect status"));

        if (evt->type == GDK_ENTER_NOTIFY)
        {
//          if ((status == NET_MAP_CONNECT) || (status == NET_MAP_ERROR))
            if (status == NET_MAP_CONNECT)
            {
                _set_ipcam_line_status(ch, NET_MAP_FOCUS);
                _change_cam_obj(ch, NET_MAP_FOCUS);
                _change_port_obj(_TRANS_CH(ch), NET_MAP_FOCUS);

                nfui_signal_emit(cam_obj[ch], GDK_EXPOSE, TRUE);
                nfui_signal_emit(port_obj[_TRANS_CH(ch)], GDK_EXPOSE, TRUE);

                nfui_nfobject_get_offset(obj, &off_x, &off_y);
                off_x += NET_STATUS_POS_X;
                off_y += NET_STATUS_POS_Y;

                if (ch < 8) {
                    pos_x = off_x + obj->width;
                    pos_y = off_y + obj->height;
                }
                else if (ch < 16) {
                    pos_x = off_x - 400;
                    pos_y = off_y + obj->height;
                }
                else if (ch < 24) {
                    pos_x = off_x + obj->width;
                    pos_y = off_y - 250;
                }
                else {
                    pos_x = off_x - 400;
                    pos_y = off_y - 250;
                }

                VW_Live_Net_IPCam_Popup_Show(ch, pos_x, pos_y);
            }
        }
        else    // GDK_LEAVE_NOTIFY
        {
            _set_ipcam_line_status(ch, status);
            _change_cam_obj(ch, status);
            _change_port_obj(_TRANS_CH(ch), status);

            nfui_signal_emit(cam_obj[ch], GDK_EXPOSE, TRUE);
            nfui_signal_emit(port_obj[_TRANS_CH(ch)], GDK_EXPOSE, TRUE);

            VW_Live_Net_IPCam_Popup_Hide();
        }
    }

    return FALSE;
}

static gboolean post_ipconflict_list_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE){
        if(evt->button.button == MOUSE_RIGTH_BUTTON){
            return FALSE;
        }
    }
    return FALSE;
}

static gboolean update_set_ipconflict_list()
{
    GTimeVal tv;
    time_t cur_time;
    gchar time_to_text[64];
    gchar mac_str[64];
    gint i, j;
    gint ret;

    memset(&tv, 0x00, sizeof(GTimeVal));
    memset(time_to_text, 0x00, sizeof(time_to_text));
    memset(mac_str, 0x00, sizeof(mac_str));

    g_get_current_time(&tv);
    tv.tv_usec =0;

    i = g_setip.cnt;
    if(i > 19) i=19;

    ret = scm_net_conflict_get_ipset(mac_str);

    if(ret){
        ifn_get_localtime_text(tv.tv_sec, YYYYMMDD, H24, time_to_text);

        for(j=i; j>0; j--){
            strcpy(g_setip.conflict_type[j], g_setip.conflict_type[j-1]);
            strcpy(g_setip.conflict_time[j], g_setip.conflict_time[j-1]);
            strcpy(g_setip.conflict_mac_addr[j], g_setip.conflict_mac_addr[j-1]);
        }

        strcpy(g_setip.conflict_type[0],"SYSTEM");
        strcpy(g_setip.conflict_mac_addr[0], mac_str);
        strcpy(g_setip.conflict_time[0], time_to_text);

        g_setip.conflict |= (1<<0);

        var_set_update_ipconflict_list(&g_setip);

        if(g_setip.cnt > 19) return FALSE;
        else    g_setip.cnt++;
    }

    return FALSE;
}

static gboolean update_cam_ipconflict_list(gint ch)
{
    GTimeVal tv;
    time_t cur_time;
    gchar time_to_text[64];
    gchar mac_str[64];
    gchar strbuf[64];
    gint i, j;
    gint ret;

    memset(&tv, 0x00, sizeof(GTimeVal));
    memset(time_to_text, 0x00, sizeof(time_to_text));
    memset(mac_str, 0x00, sizeof(mac_str));
    memset(strbuf, 0x00, sizeof(strbuf));


    g_get_current_time(&tv);
    tv.tv_usec =0;

    i = g_setip.cnt;
    if(i > 19) i=19;

    ret = scm_net_conflict_get_ipcam(ch ,mac_str);

    if(ret){
        ifn_get_localtime_text(tv.tv_sec, YYYYMMDD, H24, time_to_text);

        for(j=i; j>0; j--){
            strcpy(g_setip.conflict_type[j], g_setip.conflict_type[j-1]);
            strcpy(g_setip.conflict_time[j], g_setip.conflict_time[j-1]);
            strcpy(g_setip.conflict_mac_addr[j], g_setip.conflict_mac_addr[j-1]);
        }

        g_sprintf(strbuf,"CAM%d",ch+1);

        strcpy(g_setip.conflict_type[0], strbuf);
        strcpy(g_setip.conflict_time[0], time_to_text);
        strcpy(g_setip.conflict_mac_addr[0], mac_str);

        g_setip.conflict |= (1<<(ch+1));

        var_set_update_ipconflict_list(&g_setip);

        if(g_setip.cnt > 19) return FALSE;
        else    g_setip.cnt++;
    }

    return FALSE;
}

static gboolean post_update_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *top;

    if(evt->type == GDK_BUTTON_RELEASE) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {
            return FALSE;
        }

        if(!wait_pop)
            wait_pop = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");

        // scm_register_ddns -> INFY_DDNS_NOTIFY -> INFY_GET_DDNS_STATUS
        scm_register_ddns(IRET_SCM_REG_DDNS);
    }
    return FALSE;
}

static gboolean post_client_list_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gchar login_group[10];
    gchar *str_group;
    gint i;
    gboolean is_enable;

    if (evt->type == NFEVENT_LISTBOX_CHANGED)
    {
        memset(login_group, 0x00, sizeof(login_group));
        ssm_get_cur_group(login_group);

        if (strcmp(login_group, "ADMIN") != 0) return FALSE;

        for (i = 0; i < MAX_SHOWN_CLIENT; i++)
        {
            str_group = nfui_listbox_get_text_of_box((NFLISTBOX*)obj, i, CLIENT_FIELD_GROUP);

            if (!str_group) continue;

            if (!strcmp(str_group, "ADMIN"))
            {
                if (!nfui_nfobject_is_disabled(disconnect_obj[i]))
                {
                    nfui_nfobject_disable(disconnect_obj[i]);
                    nfui_signal_emit(disconnect_obj[i], GDK_EXPOSE, TRUE);
                }
            }
            else
            {
                if (nfui_nfobject_is_disabled(disconnect_obj[i]))
                {
                    nfui_nfobject_enable(disconnect_obj[i]);
                    nfui_signal_emit(disconnect_obj[i], GDK_EXPOSE, TRUE);
                }
            }
        }
    }

    return FALSE;
}

static gboolean post_disconnect_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gchar *user_id = 0;
    gint i, idx;
    mb_type ret = -1;

    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        ret = vw_mbox(g_curwnd, "QUESTION", IMBX_REMOTE_DISCONNECT, NFTOOL_MB_OKCANCEL);
        if (ret == NFTOOL_MB_CANCEL)    return FALSE;

        for (i = 0; i < MAX_SHOWN_CLIENT; i++)
        {
            if (disconnect_obj[i] == obj) break;
        }

        idx = nfui_listbox_get_index_by_box_row((NFLISTBOX*)clientList_obj, i);

        user_id = nfui_listbox_get_text_of_list((NFLISTBOX*)clientList_obj, i, CLIENT_FIELD_ID);
        if (user_id) scm_disconnect_remote_user(user_id);
    }

    return FALSE;
}

static gboolean post_ok_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *top;

    if(evt->type == GDK_BUTTON_RELEASE) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {
            return FALSE;
        }

        nfui_nfobject_hide(g_hub_macaddr);
        nfui_nfobject_hide(g_hub_fwver);

        top = nfui_nfobject_get_top(obj);
        nfui_nfobject_hide(top);
        nfui_on_backscr(top);
        nfui_rflip(top);        
        nfui_page_close(PGID_LIVE_NET_STATUS, top);
    }

    return FALSE;
}

static gboolean post_self_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
		VW_System_Self_Start_Open(g_curwnd);
	}

	return FALSE;
}

static gboolean post_ipconflict_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
        VW_Open_ipconflict_listbox_Popup(g_curwnd);

    }

    return FALSE;
}


static gboolean post_detail_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gchar qr_url[256] = {0,};
	gchar strbuf[128] = {0,};
    gint url_len = 256;
    NF_NOTIFY_INFO pInfo;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
	    if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

	    scm_get_wan_status_event_data(&pInfo);
        if(!pInfo.d.params[0])
        {
			#if defined(_SEQURINET_STRING_FIX)
                strcpy(strbuf, "SEQURINET");
            #else
                strcpy(strbuf, "P2P");
            #endif

            var_get_qr_url(qr_url, url_len);
			VW_QR_code_Open(g_curwnd, strbuf, qr_url, 200, 200);
	    }
	    else
	    {
            nftool_mbox(g_curwnd, "NOTICE", "Please check your network connection.", NFTOOL_MB_OK);
        }
	}

	return FALSE;
}

static gboolean post_btn_packet_dump_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        VW_Packet_Dump_Popup(g_curwnd);
    }

    return FALSE;
}

static gboolean post_net_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc = NULL;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;
    gint status;

    if(evt->type == GDK_EXPOSE) {
        GdkPoint points[2];
        gint i;

        drawable = nfui_nfobject_get_window(obj);
        gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);

        nfui_nfobject_gc_unref(gc);

        points[0].x = obj->x + 1210;
        points[0].y = obj->y + 512;
        points[1].x = points[0].x + 85;
        points[1].y = points[0].y;


        for (i = 0; i < 4; i++)     // LEGEND
        {
            _draw_map_line(points, 2, i);

            points[0].y += 24;
            points[1].y = points[0].y;
        }

        _expose_network_line_status();
    }
    else if (evt->type == INFY_DISP_WAN_IP)
    {
        gchar strTemp1[256];
        gchar strTemp2[40];

        memset(strTemp1, 0x00, sizeof(strTemp1));
        memset(strTemp2, 0x00, sizeof(strTemp2));

        scm_get_dvr_addr_str(strTemp1, 256);
        if (strcmp(strTemp1, "N/A") != 0) ifn_tolower(strTemp1);
        nfui_nflabel_set_text((NFLABEL*)property[PROPERTY_DDNS], strTemp1);
        nfui_signal_emit(property[PROPERTY_DDNS], GDK_EXPOSE, TRUE);

        var_get_external_addr(strTemp2, 40);
        nfui_nflabel_set_text((NFLABEL*)property[PROPERTY_EXTERNAL], strTemp2);
        nfui_signal_emit(property[PROPERTY_EXTERNAL], GDK_EXPOSE, TRUE);
    }
    else if(evt->type == INFY_SET_IP_CONFLICT_NOTIFY)
    {
        NF_NOTIFY_INFO *pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);

        if( pInfo->d.params[0])
        {
            nfui_nfobject_show(g_setip_obj);
            update_set_ipconflict_list();
            nfui_signal_emit(g_setip_obj, GDK_EXPOSE, TRUE);
        }
        else
        {
            g_setip.conflict &= !(1<<0);
            var_set_conflict_infor(0);
            nfui_nfobject_hide(g_setip_obj);
            nfui_signal_emit(g_setip_obj, GDK_EXPOSE, TRUE);
            _draw_bg_color(g_setip_obj);
        }
    }
    else if(evt->type == INFY_CAM_IP_CONFLICT_NOTIFY)
    {
        int ch;
        NF_NOTIFY_INFO *pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);

        ch = pInfo->d.params[1];

        if(pInfo->d.params[0])
        {
            nfui_nfobject_hide(g_poe_port_obj[ch]);
            nfui_signal_emit(g_poe_port_obj[ch], GDK_EXPOSE, TRUE);

            nfui_nfobject_show(g_camip_obj[ch]);
            update_cam_ipconflict_list(ch);
            nfui_signal_emit(g_camip_obj[ch], GDK_EXPOSE, TRUE);
        }
        else
        {
            g_setip.conflict &= !(1<<(ch+1));
            var_set_conflict_infor(ch+1);

            _draw_bg_color(g_camip_obj[ch]);
            nfui_nfobject_hide(g_camip_obj[ch]);
            nfui_signal_emit(g_camip_obj[ch], GDK_EXPOSE, TRUE);

            nfui_nfobject_show(g_poe_port_obj[ch]);
            nfui_signal_emit(g_poe_port_obj[ch], GDK_EXPOSE, TRUE);
        }
    }
    else if (evt->type == INFY_PND_NOTIFY)
    {
        gint ch;
        NF_NOTIFY_INFO *notify_info = (NF_NOTIFY_INFO *)data;

        ch = notify_info->d.params[1];

        VW_Live_Net_IPCam_Popup_Hide();

        if (notify_info->d.params[0] == PND_TYPE_VIDEO_START)
        {
            _change_cam_obj(ch, NET_MAP_CONNECT);
            _change_port_obj(_TRANS_CH(ch), NET_MAP_CONNECT);
            _stop_draw_ipcam_connecting_line(ch);
            _set_ipcam_line_status(ch, NET_MAP_CONNECT);
        }
        else if (notify_info->d.params[0] == PND_TYPE_UNPLUGGED)
        {
            _change_cam_obj(ch, NET_MAP_DISCONNECT);
            _change_port_obj(_TRANS_CH(ch), NET_MAP_DISCONNECT);
            _stop_draw_ipcam_connecting_line(ch);
            _set_ipcam_line_status(ch, NET_MAP_DISCONNECT);
        }
        else if (notify_info->d.params[0] == PND_TYPE_PLUGGED)
        {
            _change_cam_obj(ch, NET_MAP_CONNECTING);
            _change_port_obj(_TRANS_CH(ch), NET_MAP_CONNECTING);
            _repeat_draw_ipcam_connecting_line(ch);
        }
        else if ((notify_info->d.params[0] == PND_TYPE_UNKNOWN)
                || (notify_info->d.params[0] == PND_TYPE_UNSUPPORTED)
                || (notify_info->d.params[0] == PND_TYPE_CONNECTION_FAIL)
                || (notify_info->d.params[0] == PND_TYPE_LOGIN_FAIL)
                || (notify_info->d.params[0] == PND_TYPE_CONFIG_FAIL))
        {
            _change_cam_obj(ch, NET_MAP_ERROR);
            _change_port_obj(_TRANS_CH(ch), NET_MAP_ERROR);
            _stop_draw_ipcam_connecting_line(ch);
            _set_ipcam_line_status(ch, NET_MAP_ERROR);
        }

        nfui_signal_emit(cam_obj[ch], GDK_EXPOSE, TRUE);
        nfui_signal_emit(port_obj[_TRANS_CH(ch)], GDK_EXPOSE, TRUE);
    }
    else if (evt->type == INFY_PND_RATE_NOTIFY)
    {
        gint ch;
        guint status;
        NF_NOTIFY_INFO *notify_info = (NF_NOTIFY_INFO *)data;

        ch = notify_info->d.params[1];
        status = GPOINTER_TO_UINT(nfui_nfobject_get_data(cam_obj[ch], "connect status"));

        if (status == NET_MAP_CONNECTING)
            return FALSE;

        _change_cam_obj(ch, NET_MAP_CONNECTING);
        _change_port_obj(_TRANS_CH(ch), NET_MAP_CONNECTING);
        _repeat_draw_ipcam_connecting_line(ch);

        nfui_signal_emit(cam_obj[ch], GDK_EXPOSE, TRUE);
        nfui_signal_emit(port_obj[_TRANS_CH(ch)], GDK_EXPOSE, TRUE);
    }
    else if (evt->type == INFY_PND_HUB_NOTIFY)
    {
        NF_NOTIFY_INFO *notify_info = (NF_NOTIFY_INFO *)data;

        if (notify_info->d.params[1] == IPX_HUB_STATUS_LINKED)
        {
#if defined(GUI_16CH_SUPPORT) && defined(_SUPPORT_HUB) && (!defined(_IPX_1648P4E) && !defined(_IPX_1648M4E))
            nfui_nfobject_set_data(hub_obj, "connect status", GUINT_TO_POINTER(NET_MAP_CONNECT));

            _change_port_obj(PORT_LAN, NET_MAP_CONNECT);
            _change_port_obj(PORT_HUB_WAN, NET_MAP_CONNECT);
            nfui_signal_emit(port_obj[PORT_LAN], GDK_EXPOSE, TRUE);
            nfui_signal_emit(port_obj[PORT_HUB_WAN], GDK_EXPOSE, TRUE);

            _stop_draw_hub_connecting_line();
            _set_hub_line_status(NET_MAP_CONNECT);
#endif

            _set_hub_info();
            nfui_signal_emit(g_hub_macaddr, GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_hub_fwver, GDK_EXPOSE, TRUE);
        }
        else if (notify_info->d.params[1] == IPX_HUB_STATUS_UNLINKED)
        {
#if defined(GUI_16CH_SUPPORT) && defined(_SUPPORT_HUB) && (!defined(_IPX_1648P4E) && !defined(_IPX_1648M4E))
            nfui_nfobject_set_data(hub_obj, "connect status", GUINT_TO_POINTER(NET_MAP_DISCONNECT));

            _change_port_obj(PORT_LAN, NET_MAP_DISCONNECT);
            _change_port_obj(PORT_HUB_WAN, NET_MAP_DISCONNECT);
            nfui_signal_emit(port_obj[PORT_LAN], GDK_EXPOSE, TRUE);
            nfui_signal_emit(port_obj[PORT_HUB_WAN], GDK_EXPOSE, TRUE);

            _stop_draw_hub_connecting_line();
            _set_hub_line_status(NET_MAP_DISCONNECT);
#endif
            _set_hub_info();
            nfui_signal_emit(g_hub_macaddr, GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_hub_fwver, GDK_EXPOSE, TRUE);
        }
        else if (notify_info->d.params[1] == IPX_HUB_STATUS_CONNECTING)
        {
            guint status;

#if defined(GUI_16CH_SUPPORT) && defined(_SUPPORT_HUB) && (!defined(_IPX_1648P4E) && !defined(_IPX_1648M4E))
            status = GPOINTER_TO_UINT(nfui_nfobject_get_data(hub_obj, "connect status"));

            if (status == NET_MAP_CONNECTING) return FALSE;

            nfui_nfobject_set_data(hub_obj, "connect status", GUINT_TO_POINTER(status));

            _change_port_obj(PORT_LAN, NET_MAP_CONNECTING);
            _change_port_obj(PORT_HUB_WAN, NET_MAP_CONNECTING);
            nfui_signal_emit(port_obj[PORT_LAN], GDK_EXPOSE, TRUE);
            nfui_signal_emit(port_obj[PORT_HUB_WAN], GDK_EXPOSE, TRUE);

            _repeat_draw_hub_connecting_line();
#endif
        }
    }
    else if (evt->type == INFY_WAN_NOTIFY)
    {
        NF_NOTIFY_INFO *pInfo;
        guint status;
        gboolean auth_sequrinet;
        gboolean sequrinet_enable = FALSE;

        pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);

        if (pInfo->d.params[0]) status = NET_MAP_ERROR;     //failed
        else    status = NET_MAP_CONNECT;                   //ok

        nfui_nfobject_set_data(internet_obj, "connect status", GUINT_TO_POINTER(status));
        auth_sequrinet = ssm_check_access_auth(USR_AUTH_SEQURINET);
        DAL_get_Sequrinet_Status(&sequrinet_enable);

        if (status == NET_MAP_CONNECT)
        {
            scm_req_wan_ip(INFY_DISP_WAN_IP);
            if (auth_sequrinet && sequrinet_enable)
            _set_sequrinet_status(TRUE);
            else
                _set_sequrinet_status(FALSE);
        }
        else
        {
            _set_sequrinet_status(FALSE);
        }

        _set_internet_line_status(status);

    }
    else if (evt->type == INFY_DDNS_NOTIFY)
    {
        NF_NOTIFY_INFO *pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);

        if (!var_get_supported_ddns()) return FALSE;
        if (!DAL_is_ddns_on()) return FALSE;

        if(wait_pop) {
            nftool_remove_waitbox((NFOBJECT*)wait_pop);
            wait_pop = NULL;
        }

        g_message("%s, %d, INFY_DDNS_NOTIFY result:%d", __FUNCTION__, __LINE__, pInfo->d.params[0]);

        if (pInfo->d.params[0] == 0)
        {
            nfui_nflabel_set_text((NFLABEL*)property[PROPERTY_DDNS_STATUS], "Please wait...");
            nfui_nflabel_set_pango_font((NFLABEL*)property[PROPERTY_DDNS_STATUS], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(389));
            nfui_nfobject_disable(g_update_obj);
            nfui_signal_emit(g_update_obj, GDK_EXPOSE, TRUE);

            scm_req_ddns_status(INFY_GET_DDNS_STATUS);
        }
        else
        {
            nfui_nflabel_set_text((NFLABEL*)property[PROPERTY_DDNS_STATUS], "ACCESSING");
            nfui_nflabel_set_pango_font((NFLABEL*)property[PROPERTY_DDNS_STATUS], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(389));
        }

        nfui_signal_emit(property[PROPERTY_DDNS_STATUS], GDK_EXPOSE, TRUE);
    }
    else if (evt->type == INFY_GET_DDNS_STATUS)
    {
        NF_DDNS_STATUS *status = ((CMM_MESSAGE_T *)data)->data;
        gchar strTemp[64];

        memset(strTemp, 0x00, sizeof(strTemp));
        dtf_get_local_datetime(status->update_success_tv.tv_sec, strTemp);
        g_message("%s, %d, update_time:%s", __FUNCTION__, __LINE__, strTemp);

        nfui_nflabel_set_text((NFLABEL*)property[PROPERTY_DDNS_STATUS], strTemp);
        nfui_nflabel_set_pango_font((NFLABEL*)property[PROPERTY_DDNS_STATUS], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(389));
        nfui_nfobject_enable(g_update_obj);

        nfui_signal_emit(property[PROPERTY_DDNS_STATUS], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_update_obj, GDK_EXPOSE, TRUE);
    }
    else if (evt->type == IRET_SCM_REG_DDNS)
    {

    }
    else if (evt->type == INFY_NET_NOTIFY)
    {
        gint i;

        if (_set_net_connect_clients())
        {
            nfui_signal_emit(clientList_obj, GDK_EXPOSE, TRUE);

            for (i = 0; i < MAX_SHOWN_CLIENT; i++)
                nfui_signal_emit(disconnect_obj[i], GDK_EXPOSE, TRUE);
        }
    }
    else if ((evt->type == INFY_NETDB_CHANGE_NOTIFY)
        || (evt->type == INFY_IP_CHANGED_NOTIFY))
    {
        _update_network_data();
    }
    else if (evt->type == INFY_NET_RXTX)
    {
        NF_NOTIFY_INFO *pInfo;
        guint status;

        pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);

        if (pInfo->d.params[3])         //online
        {
            status = NET_MAP_CONNECT;
            _change_port_obj(PORT_WAN, NET_MAP_CONNECT);
        }
        else                            //offline
        {
            status = NET_MAP_DISCONNECT;
            _change_port_obj(PORT_WAN, NET_MAP_DISCONNECT);
        }

        nfui_nfobject_set_data(gateway_obj, "connect status", GUINT_TO_POINTER(status));
        _set_gateway_line_status(status);
        nfui_signal_emit(port_obj[PORT_WAN], GDK_EXPOSE, TRUE);
    }
    else if (evt->type == INFY_POE_NOTIFY)
    {
        NF_NOTIFY_INFO *pInfo;
        gfloat val;

        gchar strSum[10];
        gchar strMax[10];
        gchar strPoe[256];
        SysManageData sdata;

        pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);
        val = (float)pInfo->d.params[3]/1000.0;

        DAL_get_sysManage_data(&sdata);

        memset(strSum, 0x00, sizeof(strSum));
        memset(strMax, 0x00, sizeof(strMax));
        memset(strPoe, 0x00, sizeof(strPoe));

        if (val == 0)
            g_sprintf(strSum, "%dW", 0);
        else if (val < 10)
            g_sprintf(strSum, "%.1fW", val);
        else
            g_sprintf(strSum, "%dW", (gint)val);

        g_sprintf(strMax, "%dW", sdata.poeLimt);
        g_sprintf(strPoe, g_poe_str[0], strSum, strMax);
        nfui_nflabel_set_text((NFLABEL*)g_poe_tot_obj, strPoe);

        if (pInfo->d.params[0] == 0)
            nfui_nflabel_set_pango_font((NFLABEL*)g_poe_tot_obj, nffont_get_pango_font(NFFONT_MINI_SEMI_1), COLOR_IDX(389));
        else
            nfui_nflabel_set_pango_font((NFLABEL*)g_poe_tot_obj, nffont_get_pango_font(NFFONT_MINI_SEMI_1), COLOR_IDX(399));

        nfui_signal_emit(g_poe_tot_obj, GDK_EXPOSE, TRUE);
    }
    else if (evt->type == INFY_POE_HUB_NOTIFY)
    {
#if defined(GUI_16CH_SUPPORT) && defined(_SUPPORT_HUB) && (!defined(_IPX_1648P4E) && !defined(_IPX_1648M4E))
        NF_NOTIFY_INFO *pInfo;
        gfloat val;

        gchar strSum[10];
        gchar strMax[10];
        gchar strPoe[256];
        SysManageData sdata;

        pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);
        val = (float)pInfo->d.params[3]/1000.0;

        DAL_get_sysManage_data(&sdata);

        memset(strSum, 0x00, sizeof(strSum));
        memset(strMax, 0x00, sizeof(strMax));
        memset(strPoe, 0x00, sizeof(strPoe));

        if (val == 0)
            g_sprintf(strSum, "%dW", 0);
        else if (val < 10)
            g_sprintf(strSum, "%.1fW", val);
        else
            g_sprintf(strSum, "%dW", (gint)val);

        g_sprintf(strMax, "%dW", sdata.poeHubLimt);
        g_sprintf(strPoe, g_poe_str[0], strSum, strMax);
        nfui_nflabel_set_text((NFLABEL*)g_poe_hub_tot_obj, strPoe);

        if (pInfo->d.params[0] == 0)
            nfui_nflabel_set_pango_font((NFLABEL*)g_poe_hub_tot_obj, nffont_get_pango_font(NFFONT_MINI_SEMI_1), COLOR_IDX(389));
        else
            nfui_nflabel_set_pango_font((NFLABEL*)g_poe_hub_tot_obj, nffont_get_pango_font(NFFONT_MINI_SEMI_1), COLOR_IDX(399));

        nfui_signal_emit(g_poe_hub_tot_obj, GDK_EXPOSE, TRUE);
#endif
    }
    else if (evt->type == INFY_POE_PORT_NOTIFY)
    {
        NF_NOTIFY_INFO *pInfo;
        gint ch;
        gfloat val;

        pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);
        ch = pInfo->d.params[0];
        val = pInfo->d.params[3]/1000.0;

        _update_poe_port_data(g_poe_port_obj[ch], ch, val);
        nfui_signal_emit(g_poe_port_obj[ch], GDK_EXPOSE, TRUE);
    }
    else if (evt->type == INFY_PACKET_DUMP_INSERT_MEDIA)
    {
        nfui_nfobject_show(g_btn_packet);
        nfui_signal_emit(g_btn_packet, GDK_EXPOSE, TRUE);
    }
    else if (evt->type == INFY_PACKET_DUMP_REMOVE_MEDIA)
    {
        nfui_nfobject_hide(g_btn_packet);
        nfui_signal_emit(g_fix_packet, GDK_EXPOSE, TRUE);
    }
    else if (evt->type == INFY_USER_LOGON)
    {
        NF_NOTIFY_INFO pInfo;
        gboolean auth_sequrinet;

        scm_get_wan_status_event_data(&pInfo);
        auth_sequrinet = ssm_check_access_auth(USR_AUTH_SEQURINET);

        if (auth_sequrinet)
        {
            if(!pInfo.d.params[0]){
                _set_sequrinet_status(TRUE);
            }
            else{
                _set_sequrinet_status(FALSE);
            }
        }
        else
        {
            _set_sequrinet_status(FALSE);
        }
    }
    else if (evt->type == INFY_CAPTURE_NETWORK_MAP_SCREEN)
    {
        IMSG ret_msg = GPOINTER_TO_UINT(((CMM_MESSAGE_T *)data)->data);
        guint manual_req = ((CMM_MESSAGE_T *)data)->param;
        static time_t prev_capture_time = 0;
        guint interval = 10;

        GdkPixbuf *org_pixbuf;
        GdkPixbuf *crop_nbuf;
        GdkPixbuf *scale_nbuf;
        FILE *fp;

        gchar *jpeg_buff;
        gint jpeg_len;

        if (manual_req) interval = 2;

        if (prev_capture_time + interval <= time(0))
        {
            prev_capture_time = time(0);

            drawable = nfui_nfobject_get_window(obj);
            org_pixbuf = gdk_pixbuf_get_from_drawable(NULL, drawable, NULL, 0, 0, 0, 0, NET_STATUS_SIZE_W, NET_STATUS_SIZE_H);

            crop_nbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, NET_STATUS_SIZE_W-20, NET_STATUS_SIZE_H-120);
            gdk_pixbuf_copy_area(org_pixbuf, 10, 50, NET_STATUS_SIZE_W-20, NET_STATUS_SIZE_H-120, crop_nbuf, 0, 0);
            scale_nbuf = gdk_pixbuf_scale_simple(crop_nbuf, (NET_STATUS_SIZE_W-20), (NET_STATUS_SIZE_H-120), GDK_INTERP_BILINEAR);

            if (manual_req) g_message(">>>>>> manual_req, network_map_screen_save");
            else g_message(">>>>>> auto_req, network_map_screen_save");

            if (gdk_pixbuf_save(scale_nbuf, "/tmp/network_map_screen.jpg", "jpeg", NULL, "quality", "100", NULL) == FALSE) {
                g_message("gdk_pixbuf_save FAIL");
            }

            if (org_pixbuf) g_object_unref(org_pixbuf);
            if (crop_nbuf) g_object_unref(crop_nbuf);
            if (scale_nbuf) g_object_unref(scale_nbuf);
        }

        fp = fopen("/tmp/network_map_screen.jpg", "r");

        if(fp) {		
            fseek(fp, 0, SEEK_END);  
            jpeg_len = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            jpeg_buff = imalloc(jpeg_len);
            fread(jpeg_buff, 1, jpeg_len, fp);
            fclose(fp);

            g_message(">>>>>> network_map_screen_success buff:%p, len:%d", jpeg_buff, jpeg_len);
            cmm_send_message(CMMPT_WEB, ret_msg, jpeg_len, 1, jpeg_buff);
        }
        else {
            g_message(">>>>>> network_map_screen_fail");
            cmm_send_message(CMMPT_WEB, ret_msg, 0, 0, 0);
        }

    }
    else if (evt->type == GDK_DELETE)
    {
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);

        if (repeat_draw_timer)
        {
            g_source_remove(repeat_draw_timer);
            repeat_draw_timer = 0;
        }

        if (repeat_draw_timer2)
        {
            g_source_remove(repeat_draw_timer2);
            repeat_draw_timer2 = 0;
        }

        uxm_unreg_imsg_event(obj, INFY_DISP_WAN_IP);
        uxm_unreg_imsg_event(obj, INFY_PND_NOTIFY);
        uxm_unreg_imsg_event(obj, INFY_PND_RATE_NOTIFY);
        uxm_unreg_imsg_event(obj, INFY_PND_HUB_NOTIFY);
        uxm_unreg_imsg_event(obj, INFY_POE_HUB_NOTIFY);
        uxm_unreg_imsg_event(obj, INFY_WAN_NOTIFY);
        uxm_unreg_imsg_event(obj, INFY_NET_NOTIFY);
        uxm_unreg_imsg_event(obj, INFY_NETDB_CHANGE_NOTIFY);
        uxm_unreg_imsg_event(obj, INFY_NET_RXTX);
        uxm_unreg_imsg_event(obj, INFY_IP_CHANGED_NOTIFY);
        uxm_unreg_imsg_event(obj, INFY_SET_IP_CONFLICT_NOTIFY);
        uxm_unreg_imsg_event(obj, INFY_CAM_IP_CONFLICT_NOTIFY);
        uxm_unreg_imsg_event(obj, INFY_POE_NOTIFY);
        uxm_unreg_imsg_event(obj, INFY_POE_PORT_NOTIFY);

        uxm_unreg_imsg_event(obj, IRET_SCM_REG_DDNS);
        uxm_unreg_imsg_event(obj, INFY_DDNS_NOTIFY);
        uxm_unreg_imsg_event(obj, INFY_GET_DDNS_STATUS);
        uxm_unreg_imsg_event(obj, INFY_PACKET_DUMP_INSERT_MEDIA);
        uxm_unreg_imsg_event(obj, INFY_PACKET_DUMP_REMOVE_MEDIA);
        uxm_unreg_imsg_event(obj, INFY_USER_LOGON);
        uxm_unreg_imsg_event(obj, INFY_CAPTURE_NETWORK_MAP_SCREEN);


        nfui_page_close(PGID_LIVE_NET_STATUS, g_net_win);
    }

    return FALSE;
}

static gboolean post_net_window_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkEventKey *kevt;
    KEYPAD_KID kpid = KEYPAD_NONE;

    switch(evt->type)
    {
        case NFOUTEVT_BUTTON_PRESS:
        {
            nfui_nfobject_hide(g_hub_macaddr);
            nfui_nfobject_hide(g_hub_fwver);

            nfui_nfobject_hide(obj);
            nfui_on_backscr(obj);
            nfui_rflip(obj);
            nfui_page_close(PGID_LIVE_NET_STATUS, obj);
        }
        break;

        case NFEVENT_KEYPAD_PRESS:
        case NFEVENT_REMOCON_PRESS:
        {
            gint val;

            kevt = (GdkEventKey*)evt;
            kpid = (KEYPAD_KID)kevt->keyval;

            if (kpid == KEYPAD_EXIT)
            {
                        VW_Live_Net_IPCam_Popup_Hide();
                        VW_NetStatus_Hide();
                    return TRUE;
            }

            val = _get_cheat_val(kpid);

            if (val == CHEAT_FW_UP)
            {
                _upgrade_camera_fw();
            }
            else if (val == CHEAT_FACT_DEFAULT)
            {
                if (DAL_get_cam_install_mode() != 1)
                {
                    _factory_default_camera();
                }
            }
            else if (val == CHEAT_INSTALL_MODE)
            {
                _entered_camera_install_mode();
            }
            else if (val == CHEAT_SHOW_HUB_INFO)
            {
                _show_hub_info();
            }
            else if (val == CHEAT_REMOVE_HUB_FAIL)
            {
                _remove_hub_fail_info();
            }
        }
        break;

        case GDK_DELETE:
            g_curwnd = 0;
            break;

        default:
        break;
    }

    return FALSE;
}

static void _network_status_init()
{
    gint ch;
    guint conn_ipcam, count;
    NF_NOTIFY_INFO info;
    gint ipcam_st[32];

    NF_UTIL_POE_PORT_INFO poe_info;
    gfloat val;

#if 0
    memset(&info, 0, sizeof(NF_NOTIFY_INFO));
    scm_get_wan_status_event_data(&info);

    if ((info.d.params[0] == 0) || (info.d.params[0] == 1))
    {
        nfui_nfobject_set_data(internet_obj, "connect status", GUINT_TO_POINTER(NET_MAP_CONNECT));
    }
    else
    {
        nfui_nfobject_set_data(internet_obj, "connect status", GUINT_TO_POINTER(NET_MAP_DISCONNECT));
    }
#endif

    nfui_nfobject_set_data(internet_obj, "connect status", GUINT_TO_POINTER(NET_MAP_ERROR));
    nfui_nfobject_set_data(gateway_obj, "connect status", GUINT_TO_POINTER(NET_MAP_DISCONNECT));
    _change_port_obj(PORT_WAN, NET_MAP_DISCONNECT);

//  nfui_nfobject_set_data(inas_obj, "connect status", GUINT_TO_POINTER(NET_MAP_CONNECT));
//  _change_port_obj(PORT_LAN, NET_MAP_CONNECT);

#if defined(GUI_16CH_SUPPORT) && defined(_SUPPORT_HUB) && (!defined(_IPX_1648P4E) && !defined(_IPX_1648M4E))
    nfui_nfobject_set_data(hub_obj, "connect status", GUINT_TO_POINTER(NET_MAP_DISCONNECT));
    _change_port_obj(PORT_LAN, NET_MAP_DISCONNECT);
    _change_port_obj(PORT_HUB_WAN, NET_MAP_DISCONNECT);
#endif

    conn_ipcam = scm_get_cam_conn_state();
    nf_ipcam_get_pnd_status(ipcam_st);

    for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
    {
        if (conn_ipcam & (1 << ch))
        {
            nfui_nfobject_set_data(cam_obj[ch], "connect status", GUINT_TO_POINTER(NET_MAP_CONNECT));
            _change_port_obj(_TRANS_CH(ch), NET_MAP_CONNECT);
            _change_cam_obj(ch, NET_MAP_CONNECT);
        }
        else
        {
            if ((ipcam_st[ch] == PND_TYPE_UNKNOWN)
                || (ipcam_st[ch] == PND_TYPE_UNSUPPORTED)
                || (ipcam_st[ch] == PND_TYPE_CONNECTION_FAIL)
                || (ipcam_st[ch] == PND_TYPE_LOGIN_FAIL)
                || (ipcam_st[ch] == PND_TYPE_CONFIG_FAIL))
            {
                nfui_nfobject_set_data(cam_obj[ch], "connect status", GUINT_TO_POINTER(NET_MAP_ERROR));
                _change_port_obj(_TRANS_CH(ch), NET_MAP_ERROR);
                _change_cam_obj(ch, NET_MAP_ERROR);
            }
            else
            {
                nfui_nfobject_set_data(cam_obj[ch], "connect status", GUINT_TO_POINTER(NET_MAP_DISCONNECT));
                _change_port_obj(_TRANS_CH(ch), NET_MAP_DISCONNECT);
                _change_cam_obj(ch, NET_MAP_DISCONNECT);
            }
        }
    }

    _set_net_status_property();

    memset(&info, 0, sizeof(NF_NOTIFY_INFO));
    scm_get_net_status_data(&info);
    count = info.d.params[0];
    nfui_nfobject_set_data(clientList_obj, "client count", GUINT_TO_POINTER(count));

    if (count > 0)
        _set_net_connect_clients();

    memset(&poe_info, 0, sizeof(NF_UTIL_POE_PORT_INFO));
    scm_get_poe_port_info(&poe_info);

    for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
    {
        val = poe_info.info[ch].consumption/1000.0;
        _update_poe_port_data(g_poe_port_obj[ch], ch, val);
    }
}

static gboolean returnkey_proc(NFOBJECT *top, GdkEvent *event, gpointer data)
{
    return FALSE;
}

static gint _make_detail_button_image(GdkPixbuf **pbuf, gint size_w, gint size_h)
{
	gchar btn_name_n[32];
	gchar btn_name_o[32];
	gchar btn_name_p[32];
	gchar btn_name_d[32];

    GdkPixbuf *bar_pbuf;
    GdkPixbuf *icon_pbuf;

	g_sprintf(btn_name_n, "MKB_IMG_DETAIL_BTN1_N_%d", size_w);
	g_sprintf(btn_name_o, "MKB_IMG_DETAIL_BTN1_O_%d", size_w);
	g_sprintf(btn_name_p, "MKB_IMG_DETAIL_BTN1_P_%d", size_w);
	g_sprintf(btn_name_d, "MKB_IMG_DETAIL_BTN1_D_%d", size_w);

	nf_ui_create_image_button_method2(btn_name_n, size_w, size_h, IMG_BTN1_N_L, IMG_BTN1_N_M, IMG_BTN1_N_R);
	nf_ui_create_image_button_method2(btn_name_o, size_w, size_h, IMG_BTN1_O_L, IMG_BTN1_O_M, IMG_BTN1_O_R);
	nf_ui_create_image_button_method2(btn_name_p, size_w, size_h, IMG_BTN1_P_L, IMG_BTN1_P_M, IMG_BTN1_P_R);
    nf_ui_create_image_button_method2(btn_name_d, size_w, size_h, IMG_BTN1_N_L, IMG_BTN1_N_M, IMG_BTN1_N_R);

    pbuf[0] = nfui_get_image_from_memory(btn_name_n);
    pbuf[1] = nfui_get_image_from_memory(btn_name_o);
    pbuf[2] = nfui_get_image_from_memory(btn_name_p);
    pbuf[3] = nfui_get_image_from_memory(btn_name_d);

    return 0;
}



////////////////////////////////////////////////////////////////////
//
//
//

gboolean VW_Create_NetStatus(NFWINDOW *parent)
{
    NFOBJECT *netWin = NULL;
    NFOBJECT *netFixed = NULL;
    NFOBJECT *prop_t = NULL;
    NFOBJECT *ddns_status_t = NULL;
    NFOBJECT *ipconflict_status_t = NULL;
    NFOBJECT *sequrinet_t = NULL;
    NFOBJECT *obj = NULL;

    gint fg_color[NFOBJECT_STATE_COUNT];
    gint bg_color[NFOBJECT_STATE_COUNT];
    gint install_mode = DAL_get_cam_install_mode();

    GdkPixbuf *update_btn[NFOBJECT_STATE_COUNT];
    GdkPixbuf *ipconflict_btn[NFOBJECT_STATE_COUNT];
    GdkPixbuf *dis_btn[NFOBJECT_STATE_COUNT];
    GdkPixbuf *detail_btn[NFOBJECT_STATE_COUNT];
    guint property_table_w[2] = {293, 302};
    guint ddns_table_w[2] = {260, 42};
    guint sequrinet_table_w[2] = {260, 42};
    guint client_w[NUM_CLIENT_FIELD_COLS] = {182, 182, 182, 181};
    const guint btn_font_color[NFOBJECT_STATE_COUNT] = {100, 101, 103, 104};
    gchar *strImage[2] = {"INTERNET", "GATEWAY"};
    gchar *strLegend[5] = {"CONNECT", "DISCONNECT", "ERROR", "CONNECTING", "FOCUS"};
#if defined(_SEQURINET_STRING_FIX)
    gchar *strProp[NUM_PROPERTY_ROWS] = {"IP ADDRESS", "IPv6 ADDRESS", "MAC ADDRESS", "DDNS ADDRESS",
            "RTSP SERVICE PORT", "WEB SERVICE PORT", "DDNS UPDATE STATUS", "EXTERNAL IP ADDRESS", "SEQURINET"};
#else
    gchar *strProp[NUM_PROPERTY_ROWS] = {"IP ADDRESS", "IPv6 ADDRESS", "MAC ADDRESS", "DDNS ADDRESS",
            "RTSP SERVICE PORT", "WEB SERVICE PORT", "DDNS UPDATE STATUS", "EXTERNAL IP ADDRESS", "P2P"};
#endif
    gchar strBuf[64];

    gint i, pos_x, pos_y, tmp_x;
    gint size_w, size_h;

    update_btn[0] = nfui_get_image_from_file((IMG_BTN_N_POPUP_UPDATE), NULL);
    update_btn[1] = nfui_get_image_from_file((IMG_BTN_O_POPUP_UPDATE), NULL);
    update_btn[2] = nfui_get_image_from_file((IMG_BTN_P_POPUP_UPDATE), NULL);
    update_btn[3] = nfui_get_image_from_file((IMG_BTN_D_POPUP_UPDATE), NULL);

    ipconflict_btn[0] = nfui_get_image_from_file((IMG_BTN_N_IPCONFLICT), NULL);
    ipconflict_btn[1] = nfui_get_image_from_file((IMG_BTN_N_IPCONFLICT), NULL);
    ipconflict_btn[2] = nfui_get_image_from_file((IMG_BTN_N_IPCONFLICT), NULL);
    ipconflict_btn[3] = nfui_get_image_from_file((IMG_BTN_D_IPCONFLICT), NULL);

    dis_btn[0] = nfui_get_image_from_file((IMG_BTN_N_POPUP_DISCONNECT), NULL);
    dis_btn[1] = nfui_get_image_from_file((IMG_BTN_O_POPUP_DISCONNECT), NULL);
    dis_btn[2] = nfui_get_image_from_file((IMG_BTN_P_POPUP_DISCONNECT), NULL);
    dis_btn[3] = nfui_get_image_from_file((IMG_BTN_D_POPUP_DISCONNECT), NULL);

    /* window */
    netWin = (NFOBJECT*)nfui_nfwindow_new(parent, NET_STATUS_POS_X, NET_STATUS_POS_Y, NET_STATUS_SIZE_W, NET_STATUS_SIZE_H);
    g_curwnd = netWin;
    nfui_regi_post_event_callback(netWin, post_net_window_event_cb);
    nfui_nfwindow_set_title((NFWINDOW*)netWin, "LIVE_NET_STATUS");
    nfui_nfwindow_use_double_buffer((NFWINDOW*)netWin);
    nfui_nfwindow_use_outside_evt((NFWINDOW*)netWin, TRUE);
    nfui_nfwindow_set_mask((NFWINDOW*)netWin, GDK_BUTTON_PRESS, TRUE);
    nfui_nfwindow_set_returnkey_proc((NFWINDOW*)netWin, returnkey_proc);

    /* fixed */
    netFixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(netFixed, NET_STATUS_SIZE_W, NET_STATUS_SIZE_H);
    nfui_regi_post_event_callback(netFixed, post_net_fixed_event_cb);
    nfui_nfobject_show(netFixed);

    /* title */
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("NETWORK STATUS", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
    nfui_nfobject_set_size(obj, 330, 36);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 19);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)netFixed, obj, 4, 4);

    pos_x = INTERNET_IMAGE_X;
    pos_y = INTERNET_IMAGE_Y;

    internet_obj = (NFOBJECT*)nfui_nfimage_new(IMG_NET_INTERNET);
    nfui_nfobject_show(internet_obj);
    nfui_nffixed_put((NFFIXED*)netFixed, internet_obj, pos_x, pos_y);

    nfui_get_image_size(IMG_NET_INTERNET, &size_w, &size_h);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strImage[0], nffont_get_pango_font(NFFONT_MINI_SEMI_4), COLOR_IDX(398));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 120, 24);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)netFixed, obj, pos_x, pos_y+size_h+4);

    pos_x = GATEWAY_IMAGE_X;
    pos_y = GATEWAY_IMAGE_Y;

    gateway_obj  = (NFOBJECT*)nfui_nfimage_new(IMG_NET_GATEWAY);
    nfui_nfobject_show(gateway_obj);
    nfui_nffixed_put((NFFIXED*)netFixed, gateway_obj, pos_x, pos_y);

    nfui_get_image_size(IMG_NET_GATEWAY, &size_w, &size_h);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strImage[1], nffont_get_pango_font(NFFONT_MINI_SEMI_4), COLOR_IDX(398));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 200, 24);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)netFixed, obj, pos_x - 54, pos_y+size_h+4);

//  inas_obj       = (NFOBJECT*)nfui_nfimage_new(IMG_NET_NAS);
//  nfui_nfobject_hide(inas_obj);
//  nfui_nffixed_put((NFFIXED*)netFixed, inas_obj, 620, 420);

    dvr_obj        = (NFOBJECT*)nfui_nfimage_new(NET_STATUS_DVR_IMAGE);
#if !defined(GUI_32CH_SUPPORT)    
    nfui_nfobject_show(dvr_obj);
    
    pos_x = DVR_IMAGE_X;
    pos_y = DVR_IMAGE_Y;
    nfui_nffixed_put((NFFIXED*)netFixed, dvr_obj, pos_x, pos_y);
#else
    nfui_get_image_size(IMG_NET_CAMERA_OFF, &size_w, &size_h);
    
    pos_x = 600 + (((size_w*8)+(4*7) - 227)/2) - 7;//DVR_IMAGE_X;
    pos_y = DVR_IMAGE_Y + (size_h*2);

    for (i = 0; i < 4; i++)
    {
        switch_obj[i] = (NFOBJECT*)nfui_nfimage_new(NET_STATUS_DVR_IMAGE);
        nfui_nfobject_show(switch_obj[i]);
        nfui_nffixed_put((NFFIXED*)netFixed, switch_obj[i], pos_x, pos_y);

        if (i == 0) pos_x = (600+((size_w+4)*8) - 7) + (((size_w*8)+(4*7) - 227)/2);
        if (i == 1) {pos_x = 600 + (((size_w*8)+(4*7) - 227)/2) - 7; pos_y += size_h * 2 + 30;}
        if (i == 2) pos_x = (600+((size_w+4)*8) - 7) + (((size_w*8)+(4*7) - 227)/2);
    }
#endif        

    nfui_get_image_size(NET_STATUS_DVR_IMAGE, &size_w, &size_h);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MINI_SEMI_4), COLOR_IDX(389));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(187));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 400, 24);
#if !defined (GUI_32CH_SUPPORT)
    nfui_nfobject_show(obj);
#endif    
    nfui_nffixed_put((NFFIXED*)netFixed, obj, pos_x+size_w+4, pos_y+(size_h-24)/2);
    g_poe_tot_obj = obj;

    nfui_get_image_size(IMG_NET_PORT_OFF, &size_w, &size_h);

#if defined(GUI_16CH_SUPPORT) && defined(_SUPPORT_HUB) && (!defined(_IPX_1648P4E) && !defined(_IPX_1648M4E))
    pos_x += MAP_LINE_PORT_X;
    pos_y += MAP_LINE_PORT_Y;

    for (i = 0; i <= PORT_IPCAM8; i++)
    {
        port_obj[i] = (NFOBJECT*)nfui_nfimage_new(IMG_NET_PORT_OFF);
        nfui_nfobject_show(port_obj[i]);
        nfui_nffixed_put((NFFIXED*)netFixed, port_obj[i], pos_x, pos_y);

        if (i < PORT_IPCAM1)
            pos_x += (size_w + 10);
        else
            pos_x += (size_w + 4);
    }

    pos_x = HUB_IMAGE_X;
    pos_y = HUB_IMAGE_Y;

    hub_obj        = (NFOBJECT*)nfui_nfimage_new(NET_STATUS_HUB_IMAGE);
    nfui_nfobject_show(hub_obj);
    nfui_nffixed_put((NFFIXED*)netFixed, hub_obj, pos_x, pos_y);

    nfui_get_image_size(NET_STATUS_HUB_IMAGE, &size_w, &size_h);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MINI_SEMI_4), COLOR_IDX(389));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(187));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 400, 24);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)netFixed, obj, pos_x+size_w+4, pos_y+(size_h-24)/2);
    g_poe_hub_tot_obj = obj;

    pos_x += 32;
    pos_y += MAP_LINE_PORT_Y;

    nfui_get_image_size(IMG_NET_PORT_OFF, &size_w, &size_h);

    for (i = PORT_HUB_WAN; i < NUM_PORT; i++)
    {
        port_obj[i] = (NFOBJECT*)nfui_nfimage_new(IMG_NET_PORT_OFF);
        nfui_nfobject_show(port_obj[i]);
        nfui_nffixed_put((NFFIXED*)netFixed, port_obj[i], pos_x, pos_y);

        if (i < PORT_IPCAM9)
            pos_x += (size_w + 10);
        else
            pos_x += (size_w + 4);
    }

#elif defined(GUI_16CH_SUPPORT)
    pos_x += MAP_LINE_PORT_X;
    pos_y += (MAP_LINE_PORT_Y + size_h + 14);

    for (i = 0; i <= PORT_LAN; i++)
    {
        port_obj[i] = (NFOBJECT*)nfui_nfimage_new(IMG_NET_PORT_OFF);
        nfui_nfobject_show(port_obj[i]);
        nfui_nffixed_put((NFFIXED*)netFixed, port_obj[i], pos_x, pos_y);

        pos_x += (size_w + 10);
    }

    tmp_x = pos_x;
    pos_y = DVR_IMAGE_Y + MAP_LINE_PORT_Y;

    for (i = PORT_IPCAM1; i < NUM_PORT; i++)
    {
        port_obj[i] = (NFOBJECT*)nfui_nfimage_new(IMG_NET_PORT_OFF);
        nfui_nfobject_show(port_obj[i]);
        nfui_nffixed_put((NFFIXED*)netFixed, port_obj[i], pos_x, pos_y);

        pos_x += (size_w + 4);

        if (i == PORT_IPCAM8)
        {
            pos_x = tmp_x;
            pos_y += size_h + 14;
        }
    }
#elif defined(GUI_32CH_SUPPORT)
    pos_x += MAP_LINE_PORT_X;
    pos_y += (MAP_LINE_PORT_Y + size_h + 14);

    for (i = 0; i <= PORT_LAN; i++)
    {
        port_obj[i] = (NFOBJECT*)nfui_nfimage_new(IMG_NET_PORT_OFF);
//        nfui_nfobject_show(port_obj[i]);
        nfui_nffixed_put((NFFIXED*)netFixed, port_obj[i], pos_x, pos_y);

        pos_x += (size_w + 10);
    }
    
    pos_x = switch_obj[0]->x + 32;
    pos_y = switch_obj[0]->y + 9;

    for (i = PORT_IPCAM1; i <= PORT_IPCAM8; i++)
    {
        port_obj[i] = (NFOBJECT*)nfui_nfimage_new(IMG_NET_PORT_OFF);
        nfui_nfobject_show(port_obj[i]);
        nfui_nffixed_put((NFFIXED*)netFixed, port_obj[i], pos_x, pos_y);

        if (i == PORT_IPCAM4) pos_x += (size_w + 9);
        else pos_x += (size_w + 3);
    }

    pos_x = switch_obj[1]->x + 32;
    pos_y = switch_obj[1]->y + 9;

    for (i = PORT_IPCAM9; i <= PORT_IPCAM16; i++)
    {
        port_obj[i] = (NFOBJECT*)nfui_nfimage_new(IMG_NET_PORT_OFF);
        nfui_nfobject_show(port_obj[i]);
        nfui_nffixed_put((NFFIXED*)netFixed, port_obj[i], pos_x, pos_y);

        if (i == PORT_IPCAM12) pos_x += (size_w + 9);
        else pos_x += (size_w + 3);
    }

    pos_x = switch_obj[2]->x + 32;
    pos_y = switch_obj[2]->y + 9;

    for (i = PORT_IPCAM17; i <= PORT_IPCAM24; i++)
    {
        port_obj[i] = (NFOBJECT*)nfui_nfimage_new(IMG_NET_PORT_OFF);
        nfui_nfobject_show(port_obj[i]);
        nfui_nffixed_put((NFFIXED*)netFixed, port_obj[i], pos_x, pos_y);

        if (i == PORT_IPCAM20) pos_x += (size_w + 9);
        else pos_x += (size_w + 3);
    }

    pos_x = switch_obj[3]->x + 32;
    pos_y = switch_obj[3]->y + 9;

    for (i = PORT_IPCAM25; i <= PORT_IPCAM32; i++)
    {
        port_obj[i] = (NFOBJECT*)nfui_nfimage_new(IMG_NET_PORT_OFF);
        nfui_nfobject_show(port_obj[i]);
        nfui_nffixed_put((NFFIXED*)netFixed, port_obj[i], pos_x, pos_y);

        if (i == PORT_IPCAM28) pos_x += (size_w + 9);
        else pos_x += (size_w + 3);
    }
#else
    pos_x += MAP_LINE_PORT_X;
    pos_y += MAP_LINE_PORT_Y;

    for (i = 0; i < NUM_PORT; i++)
    {
        port_obj[i] = (NFOBJECT*)nfui_nfimage_new(IMG_NET_PORT_OFF);
        nfui_nfobject_show(port_obj[i]);
        nfui_nffixed_put((NFFIXED*)netFixed, port_obj[i], pos_x, pos_y);

        if (i < PORT_IPCAM1)
            pos_x += (size_w + 10);
        else
            pos_x += (size_w + 4);
        }
#endif

    nfui_get_image_size(IMG_NET_CAMERA_OFF, &size_w, &size_h);

    pos_x = CAM_IMAGE_ROW1_X;
#if !defined(GUI_32CH_SUPPORT)    
    pos_y = DVR_IMAGE_Y - CAM_IMAGE_GAP_Y - size_h;
#else
    pos_y = DVR_IMAGE_Y;
#endif

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        cam_obj[i] = (NFOBJECT*)nfui_nfimage_new(IMG_NET_CAMERA_OFF);
        nfui_nfobject_modify_bg(cam_obj[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(187));
        nfui_nfobject_show(cam_obj[i]);
        nfui_nffixed_put((NFFIXED*)netFixed, cam_obj[i], pos_x, pos_y);
        nfui_regi_post_event_callback(cam_obj[i], post_cam_obj_event_cb);

        obj = (NFOBJECT*)nfui_nfbutton_new();
        nfui_nfbutton_set_image(NF_BUTTON(obj), ipconflict_btn);
        nfui_nfobject_hide(obj);
        nfui_nffixed_put((NFFIXED*)netFixed, obj, pos_x, pos_y-32);
        g_camip_obj[i] = obj;
        nfui_nfobject_set_data(g_camip_obj[i],"cam_obj_channel", GUINT_TO_POINTER(i));

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MINI_SEMI_1), COLOR_IDX(389));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(187));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(obj, size_w, 24);
        nfui_nfobject_show(obj);

        if (install_mode != 1)
        {
#if defined(GUI_16CH_SUPPORT) && defined(_SUPPORT_HUB) && (!defined(_IPX_1648P4E) && !defined(_IPX_1648M4E))
        nfui_nffixed_put((NFFIXED*)netFixed, obj, pos_x, pos_y-24);
#else
        if (i < 8)  nfui_nffixed_put((NFFIXED*)netFixed, obj, pos_x, pos_y-24);
        else        nfui_nffixed_put((NFFIXED*)netFixed, obj, pos_x, pos_y+size_h);
#endif
        }

        g_poe_port_obj[i] = obj;
        
#if defined(GUI_32CH_SUPPORT)
        if (i == 15)
        {
            pos_x = CAM_IMAGE_ROW1_X;
            pos_y += size_h * 6 + 30;
        }
#else
        if (i == 7)
        {
#if defined(GUI_16CH_SUPPORT) && defined(_SUPPORT_HUB) && (!defined(_IPX_1648P4E) && !defined(_IPX_1648M4E))
            pos_x = CAM_IMAGE_ROW1_X;
            pos_y = HUB_IMAGE_Y - CAM_IMAGE_GAP_Y - size_h;
#else
            nfui_get_image_size(NET_STATUS_DVR_IMAGE, &size_w, &size_h);
            pos_x = CAM_IMAGE_ROW1_X;
            pos_y = DVR_IMAGE_Y + size_h + CAM_IMAGE_GAP_Y;
#endif
            nfui_get_image_size(IMG_NET_CAMERA_OFF, &size_w, &size_h);
        }
#endif        
        else
        {
            pos_x += (size_w + 4);
        }
    }


// CHEAT HUB INFO
    pos_x = 700;
    pos_y = 500;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MINI_SEMI_1), COLOR_IDX(389));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(187));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 400, 24);
    nfui_nfobject_hide(obj);
    nfui_nffixed_put((NFFIXED*)netFixed, obj, pos_x, pos_y);
    g_hub_macaddr = obj;

    pos_y += 28;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MINI_SEMI_1), COLOR_IDX(389));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(187));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 400, 24);
    nfui_nfobject_hide(obj);
    nfui_nffixed_put((NFFIXED*)netFixed, obj, pos_x, pos_y);
    g_hub_fwver = obj;

    pos_x = 36;
    pos_y = 382;

    pos_x = 36;

    nfui_get_image_size(IMG_N_SCROLL_UP, &size_w, &size_h);

    fg_color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(389);
    fg_color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(251);
    fg_color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(251);
    fg_color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(389);

    bg_color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(200);
    bg_color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(250);
    bg_color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(250);
    bg_color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(200);

// LEGEND
    pos_x = 1300;
    pos_y = 500;

    for(i = 0; i < 4; i++) {
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strLegend[i], nffont_get_pango_font(NFFONT_MINI_SEMI_4), COLOR_IDX(398));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(obj, 200, 24);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)netFixed, obj, pos_x, pos_y);
        pos_y += 24;
    }

// PROPERTY
    pos_x = 36;
	pos_y = 602;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PROPERTY", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
    nfui_nfobject_set_size(obj, 253, 30);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 8);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)netFixed, obj, pos_x, pos_y - 30);

    if(var_get_supported_sequrinet()){
        prop_t = (NFOBJECT*)nfui_nftable_new(2, NUM_PROPERTY_ROWS, 1, 1, property_table_w, 30);
    }
    else{
        prop_t = (NFOBJECT*)nfui_nftable_new(2, NUM_PROPERTY_ROWS-1, 1, 1, property_table_w, 30);
    }
    nfui_nftable_set_draw_outline((NFTABLE*)prop_t, TRUE);
    nfui_nfobject_show(prop_t);
    nfui_nffixed_put((NFFIXED*)netFixed, prop_t, pos_x, pos_y);

    pos_x += property_table_w[0];
    pos_x += property_table_w[1];

    obj = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(obj), ipconflict_btn);
    nfui_nfobject_hide(obj);
    nfui_nffixed_put((NFFIXED*)netFixed, obj, (gint)gateway_obj->x + 155, (gint)gateway_obj->y + (gint)gateway_obj->height/2 - 35);
    g_setip_obj = obj;

    for(i = 0; i < NUM_PROPERTY_ROWS; i++) {
        if(i == PROPERTY_SEQURINET && !var_get_supported_sequrinet())
        {
            continue;
        }
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strProp[i], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(206));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)prop_t, obj,  0, (guint)i);

        if (i == PROPERTY_DDNS_STATUS)
        {
            ddns_status_t = (NFOBJECT*)nfui_nftable_new(2, NUM_PROPERTY_ROWS, 1, 1, ddns_table_w, 30);
            nfui_nfobject_show(ddns_status_t);
            nfui_nftable_attach((NFTABLE*)prop_t, ddns_status_t,  1, (guint)i);

            property[i] = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(389));
            nfui_nflabel_set_align((NFLABEL*)property[i], NFALIGN_CENTER, 0);
            nfui_nfobject_modify_bg(property[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(187));
            nfui_nfobject_use_focus(property[i], NFOBJECT_FOCUS_OFF);
            nfui_nfobject_show(property[i]);
            nfui_nftable_attach((NFTABLE*)ddns_status_t, property[i], 0, 0);

            if (!var_get_supported_ddns()) nfui_nflabel_set_text((NFLABEL*)property[i], "N/A");
            else if (!DAL_is_ddns_on()) nfui_nflabel_set_text((NFLABEL*)property[i], "OFF");
            else nfui_nflabel_set_text((NFLABEL*)property[i], "Please wait...");

            obj = (NFOBJECT*)nfui_nfbutton_new();
            nfui_nfbutton_set_image(NF_BUTTON(obj), update_btn);
            nfui_nfobject_show(obj);
            nfui_nftable_attach((NFTABLE*)ddns_status_t, obj, 1, 0);
            nfui_regi_post_event_callback(obj, post_update_event_cb);
            g_update_obj = obj;

            if (!var_get_supported_ddns()) nfui_nfobject_disable(obj);
            else if (!DAL_is_ddns_on()) nfui_nfobject_disable(obj);
        }
        else if(i == PROPERTY_SEQURINET)
        {
            sequrinet_t = (NFOBJECT*)nfui_nftable_new(2, 1, 1, 1, sequrinet_table_w, 30);
            nfui_nfobject_show(sequrinet_t);
            nfui_nftable_attach((NFTABLE*)prop_t, sequrinet_t,  1, (guint)i);

            property[i] = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(389));
            nfui_nflabel_set_align((NFLABEL*)property[i], NFALIGN_CENTER, 25);
            nfui_nfobject_modify_bg(property[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(187));
            nfui_nfobject_use_focus(property[i], NFOBJECT_FOCUS_OFF);
            nfui_nfobject_show(property[i]);
            nfui_nftable_attach((NFTABLE*)sequrinet_t, property[i], 0, 0);

            obj = nftool_normal_button_create_popup_type2("...", sequrinet_table_w[1]);
            nfui_nfbutton_set_spacing((NFBUTTON*)obj, CONDENSED_SPACING);
            nfui_nfobject_show(obj);
            nfui_nftable_attach((NFTABLE*)sequrinet_t, obj, 1, 0);
            nfui_regi_post_event_callback(obj, post_detail_button_event_cb);
            g_detail_obj = obj;
        }
        else
        {
            property[i] = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(389));
            nfui_nflabel_set_align((NFLABEL*)property[i], NFALIGN_CENTER, 0);
            nfui_nfobject_modify_bg(property[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(187));
            nfui_nfobject_use_focus(property[i], NFOBJECT_FOCUS_OFF);
            nfui_nfobject_show(property[i]);
            nfui_nftable_attach((NFTABLE*)prop_t, property[i],  1, (guint)i);

            if (i == PROPERTY_EXTERNAL) nfui_nflabel_set_text((NFLABEL*)property[i], "Please wait...");
        }
    }


// CONNECTED CLIENTS
    pos_x += 36;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CONNECTED CLIENTS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
    nfui_nfobject_set_size(obj, 300, 30);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 8);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)netFixed, obj, pos_x, pos_y - 30);

    obj = nfui_listbox_new(NUM_CLIENT_FIELD_COLS, client_w, 30);
    nfui_listbox_set_skin_type(NF_LISTBOX(obj), NFLISTBOX_TYPE_POPUP_1);
    nfui_listbox_set_pango_font(NF_LISTBOX(obj), nffont_get_pango_font(NFFONT_SMALL_SEMI));
    nfui_listbox_set_fg_color(NF_LISTBOX(obj), fg_color);
    nfui_listbox_set_bg_color(NF_LISTBOX(obj), bg_color);
    nfui_listbox_set_column_align(NF_LISTBOX(obj), CLIENT_FIELD_ID, NFALIGN_LEFT);
    nfui_listbox_set_column_align(NF_LISTBOX(obj), CLIENT_FIELD_GROUP, NFALIGN_CENTER);
    nfui_listbox_set_column_align(NF_LISTBOX(obj), CLIENT_FIELD_IP, NFALIGN_CENTER);
    nfui_listbox_set_column_align(NF_LISTBOX(obj), CLIENT_FIELD_ACTION, NFALIGN_CENTER);
    nfui_listbox_set_draw_inline(NF_LISTBOX(obj), TRUE, COLOR_IDX(392));
    nfui_listbox_support_multi_lang(NF_LISTBOX(obj), FALSE);
    nfui_nfobject_set_size(obj, client_w[0]+client_w[1]+client_w[2]+client_w[3]+size_w, 30*MAX_SHOWN_CLIENT);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_ON);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)netFixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_client_list_event_cb);
    clientList_obj = obj;

    for (i = 0; i < MAX_SHOWN_CLIENT; i++)
    {
        obj = (NFOBJECT*)nfui_nfbutton_new();
        nfui_nfbutton_set_image(NF_BUTTON(obj), dis_btn);
        nfui_nfobject_disable(obj);
        nfui_nfobject_show(obj);
        nfui_regi_post_event_callback(obj, post_disconnect_event_cb);
        nfui_nffixed_put((NFFIXED*)netFixed, obj, pos_x+client_w[0]+client_w[1]+client_w[2]+client_w[3]+size_w+4, pos_y+30*i);
        disconnect_obj[i] = obj;
    }

	/* button */
#if defined(_SUPPORT_SELF_DIAGNOSIS)
	obj = nftool_normal_button_create_type1("NVR SELF-DIAGNOSIS", 230);
	nfui_regi_post_event_callback(obj, post_self_button_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)netFixed, obj, 36, 888);
#endif

    obj = nftool_normal_button_create_type1("IP CONFLICT LIST",220);
    nfui_regi_post_event_callback(obj, post_ipconflict_button_event_cb);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)netFixed, obj, 36+230+10, 888);

	obj = nftool_normal_button_create_type1("OK", 192);
	nfui_regi_post_event_callback(obj, post_ok_button_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)netFixed, obj, 670, 888);

    nfui_nfobject_get_size(obj, &size_w, &size_h);

    obj = nfui_nffixed_new();
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, size_w, size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)netFixed, obj, 670 + 570, 888);
    g_fix_packet = obj;

	obj = nftool_normal_button_create_type1("PACKET DUMP", 192);
	nfui_regi_post_event_callback(obj, post_btn_packet_dump_event_cb);
	nfui_nfobject_hide(obj);
	nfui_nffixed_put((NFFIXED*)g_fix_packet, obj, 0, 0);
    g_btn_packet = obj;

    uxm_reg_imsg_event(netFixed, INFY_DISP_WAN_IP);
    uxm_reg_imsg_event(netFixed, INFY_PND_NOTIFY);
    uxm_reg_imsg_event(netFixed, INFY_PND_RATE_NOTIFY);
    uxm_reg_imsg_event(netFixed, INFY_PND_HUB_NOTIFY);
    uxm_reg_imsg_event(netFixed, INFY_POE_HUB_NOTIFY);
    uxm_reg_imsg_event(netFixed, INFY_WAN_NOTIFY);
    uxm_reg_imsg_event(netFixed, INFY_NET_NOTIFY);
    uxm_reg_imsg_event(netFixed, INFY_NETDB_CHANGE_NOTIFY);
    uxm_reg_imsg_event(netFixed, INFY_NET_RXTX);
    uxm_reg_imsg_event(netFixed, INFY_IP_CHANGED_NOTIFY);
    uxm_reg_imsg_event(netFixed, INFY_SET_IP_CONFLICT_NOTIFY);
    uxm_reg_imsg_event(netFixed, INFY_CAM_IP_CONFLICT_NOTIFY);
    uxm_reg_imsg_event(netFixed, INFY_POE_NOTIFY);
    uxm_reg_imsg_event(netFixed, INFY_POE_PORT_NOTIFY);
    uxm_reg_imsg_event(netFixed, INFY_DDNS_NOTIFY);
    uxm_reg_imsg_event(netFixed, INFY_GET_DDNS_STATUS);
    uxm_reg_imsg_event(netFixed, IRET_SCM_REG_DDNS);
    uxm_reg_imsg_event(netFixed, INFY_PACKET_DUMP_INSERT_MEDIA);
    uxm_reg_imsg_event(netFixed, INFY_PACKET_DUMP_REMOVE_MEDIA);
    uxm_reg_imsg_event(netFixed, INFY_USER_LOGON);
    uxm_reg_imsg_event(netFixed, INFY_CAPTURE_NETWORK_MAP_SCREEN);    

    uxm_monitor_on_imsg_event(netFixed, INFY_DISP_WAN_IP);
    uxm_monitor_on_imsg_event(netFixed, INFY_PND_NOTIFY);
    uxm_monitor_on_imsg_event(netFixed, INFY_PND_RATE_NOTIFY);
    uxm_monitor_on_imsg_event(netFixed, INFY_PND_HUB_NOTIFY);
    uxm_monitor_on_imsg_event(netFixed, INFY_POE_HUB_NOTIFY);
    uxm_monitor_on_imsg_event(netFixed, INFY_WAN_NOTIFY);
    uxm_monitor_on_imsg_event(netFixed, INFY_NET_NOTIFY);
    uxm_monitor_on_imsg_event(netFixed, INFY_NETDB_CHANGE_NOTIFY);
    uxm_monitor_on_imsg_event(netFixed, INFY_NET_RXTX);
    uxm_monitor_on_imsg_event(netFixed, INFY_IP_CHANGED_NOTIFY);
    uxm_monitor_on_imsg_event(netFixed, INFY_SET_IP_CONFLICT_NOTIFY);
    uxm_monitor_on_imsg_event(netFixed, INFY_CAM_IP_CONFLICT_NOTIFY);
    uxm_monitor_on_imsg_event(netFixed, INFY_POE_NOTIFY);
    uxm_monitor_on_imsg_event(netFixed, INFY_POE_PORT_NOTIFY);
    uxm_monitor_on_imsg_event(netFixed, INFY_DDNS_NOTIFY);
    uxm_monitor_on_imsg_event(netFixed, INFY_GET_DDNS_STATUS);
    uxm_monitor_on_imsg_event(netFixed, INFY_PACKET_DUMP_INSERT_MEDIA);
    uxm_monitor_on_imsg_event(netFixed, INFY_PACKET_DUMP_REMOVE_MEDIA);
    uxm_monitor_on_imsg_event(netFixed, INFY_USER_LOGON);
    uxm_monitor_on_imsg_event(netFixed, INFY_CAPTURE_NETWORK_MAP_SCREEN);  

    nfui_nfwindow_add((NFWINDOW*)netWin, netFixed);
    nfui_run_main_event_handler(netWin);
    nfui_nfobject_show(netWin);
    nfui_nfobject_hide(netWin);

    _network_status_init();
    _set_hub_info();

    nfui_on_backscr(netWin);
    nfui_rflip(netWin);
    nfui_signal_emit(netFixed, GDK_EXPOSE, TRUE);

    scm_req_net_rxtx_status_data();
    scm_req_wan_status_data();
    scm_req_pnd_hub_event_data();
    scm_req_ddns_status_data();

    g_net_win = netWin;

    return TRUE;
}

int VW_Destroy_NetStatus()
{
    if (!g_curwnd) return 0;
    nfui_nfobject_destroy(g_curwnd);
    return 0;
}

void VW_NetStatus_Show()
{
    _refresh_disconnect_button();

    nfui_flip(g_net_win);
    nfui_off_backscr(g_net_win);
    nfui_nfobject_show(g_net_win);
    nfui_make_key_hierarchy((NFWINDOW*)g_net_win);
    nfui_page_open(PGID_LIVE_NET_STATUS, g_net_win, ssm_get_cur_id(NULL));

    cheat_mode = -1;
    key_count = 0;
}

void VW_NetStatus_Hide()
{
    nfui_nfobject_hide(g_hub_macaddr);
    nfui_nfobject_hide(g_hub_fwver);

    nfui_nfobject_hide(g_net_win);
    nfui_on_backscr(g_net_win);
    nfui_rflip(g_net_win);
    nfui_page_close(PGID_LIVE_NET_STATUS, g_net_win);
}

gboolean VW_NetStatus_IsShown()
{
    return nfui_nfobject_is_shown(g_net_win);
}
