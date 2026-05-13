#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfspinbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nfbutton.h"
#include "objects/nfimage.h"

#include "scm.h"

#include "vw_sys_main.h"
#include "vw_sys_license.h"
#include "vw_sys_license_devinfo.h"
#include "vw_sys_license_entercode.h"

#define TABLE_LEFT          (28)
#define TABLE_TOP           (42)

#define PAGE_FIXED_CNT      3
#define ROW_CNT_PER_PAGE    ((GUI_CHANNEL_CNT + 1) / PAGE_FIXED_CNT)

enum
{
    LIC_TITLE = 0,
    LIC_LICENSE,
    LIC_ACQUIRED,
    LIC_ADD,
    LIC_DETIAL,

    LIC_COL_CNT
};

static LIC_DEVINFO_T g_devinfo;

static LicenseData g_org_licdata;
static LicenseData g_licdata;

static LicenseData g_org_cam_licdata[GUI_CHANNEL_CNT];
static LicenseData g_cam_licdata[GUI_CHANNEL_CNT];

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_wait_pop = NULL;
static NFOBJECT *g_ntb[PAGE_FIXED_CNT];
static NFOBJECT *g_enter[GUI_CHANNEL_CNT];
static NFOBJECT *g_page_fixed[PAGE_FIXED_CNT];
static NFOBJECT *g_lb_page_num;

static gint _init_lic_db()
{
    memset(&g_org_licdata, 0x00, sizeof(LicenseData));
    memset(&g_licdata, 0x00, sizeof(LicenseData));

    if (DAL_get_license_data(&g_licdata))
    {
        memset(&g_licdata, 0x00, sizeof(LicenseData));
    }
    else
    {
        memmove(&g_org_licdata, &g_licdata, sizeof(LicenseData));
    }

    return 0;
}

static gint _init_cam_lic_db()
{
    gint i;

    memset(&g_org_cam_licdata, 0x00, sizeof(LicenseData) * GUI_CHANNEL_CNT);
    memset(&g_cam_licdata, 0x00, sizeof(LicenseData) * GUI_CHANNEL_CNT);

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        scm_license_get_cam_license(i, &g_cam_licdata[i]);
    }

    memmove(&g_org_cam_licdata, &g_cam_licdata, sizeof(LicenseData) * GUI_CHANNEL_CNT);

    return 0;
}

static gint _init_local_info()
{
    DAL_get_sysInfo_data(&g_devinfo.sys_info);
    scm_get_sys_netinfo(&g_devinfo.netif_info);

    return 0;
}

static gint _init_cam_info()
{
    gint i;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        scm_get_cam_profile(i, &g_devinfo.profile[i]);
        var_get_camtitle(g_devinfo.camtitle[i], i);
    }

    return 0;
}

static void _init_dev_info()
{
    memset(&g_devinfo, 0x00, sizeof(LIC_DEVINFO_T));

    _init_local_info();
    _init_cam_info();
}

static void _init_local_table()
{
    NFOBJECT *child;
    gint tb_idx, i, j;

    for (j = LIC_LICENSE; j <= LIC_ACQUIRED; j++)
    {
        child = nfui_nftable_get_child((NFTABLE *)g_ntb[0], j, 0);
        nfui_nflabel_set_text((NFLABEL *)child, "-");
    }
}

static void _init_cam_table()
{
    NFOBJECT *child;
    gint tb_idx, i, j;
    gint page_num = 0;
    gint row_num = 1;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        for (j = LIC_LICENSE; j <= LIC_ACQUIRED; j++)
        {
            child = nfui_nftable_get_child((NFTABLE *)g_ntb[page_num], j, row_num);

            if (g_devinfo.profile[i].connected)
            {
                nfui_nflabel_set_text((NFLABEL *)child, "-");
            }
            else
            {
                nfui_nflabel_set_text((NFLABEL *)child, "N/A");
            }
        }

        row_num++;

        if (row_num == ROW_CNT_PER_PAGE)
        {
            row_num = 0;
            page_num++;
        }
    }
}

static void _init_table()
{
    _init_local_table();
    _init_cam_table();
}

static void _renew_dev_info()
{
    _init_dev_info();
}

static void _renew_lic_info()
{
    _init_lic_db();
    _init_cam_lic_db();
}

static gint _is_exist_license(gint ch, gchar *input_key)
{
    gint i;

    if (ch == -1)
    {
        for (i = 0; i < g_licdata.count; i++)
        {
            if (strcmp(g_licdata.key_data[i].key, input_key) == 0)
            {
                g_message("[%s, %d] ch : %d, key : %s, License is exist!", __FUNCTION__, __LINE__, ch, input_key);
                return 1;
            }
        }
    }
    else
    {
        for (i = 0; i < g_cam_licdata[ch].count; i++)
        {
            if (strcmp(g_cam_licdata[ch].key_data[i].key, input_key) == 0)
            {
                g_message("[%s, %d] ch : %d, key : %s, License is exist!", __FUNCTION__, __LINE__, ch, input_key);
                return 1;
            }
        }
    }

    return 0;
}

static gint _is_maximum_license(gint ch)
{
    if ((ch == -1 && g_licdata.count >= MAX_LICENSE_CNT) ||
        (ch >= 0 && g_cam_licdata[ch].count >= MAX_LICENSE_CNT))
    {
        g_message("[%s, %d] CH : %d, License is full!", __FUNCTION__, __LINE__, ch);
        return 1;
    }

    return 0;
}

static gint _get_page_num(gint ch)
{
    return (ch + 1) / ROW_CNT_PER_PAGE;
}

static gint _get_tb_row(gint ch)
{
    return (ch + 1) % ROW_CNT_PER_PAGE;
}

static gint _set_license_str(gint ch, gchar *name, gint expose)
{
    NFOBJECT *obj;
    gchar *str = NULL, *tmp_str = NULL;
    gchar buf[256];
    gint i;
    gint tb_row = _get_tb_row(ch);
    gint page_num = _get_page_num(ch);

    memset(buf, 0x00, sizeof(buf));

    obj = nfui_nftable_get_child(g_ntb[page_num], LIC_LICENSE, tb_row);
    str = nfui_nflabel_get_text((NFLABEL *)obj);

    if (!name)
    {
        strcpy(buf, "-");
    }
    else
    {
        if (!str || !strcmp(str, "-") || !strcmp(str, "N/A"))
        {
            strcpy(buf, lookup_string(name));
        }
        else
        {
            sprintf(buf, "%s, %s", str, lookup_string(name));
        }
    }

    nfui_nflabel_set_text((NFLABEL *)obj, buf);

    if (expose)
    {
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }

    return 0;
}

static gint _set_acquired_date_str(gint ch, time_t acquired_time, gint expose)
{
    NFOBJECT *obj;
    gchar acquired_date[32];
    gint i;
    gint tb_row = _get_tb_row(ch);
    gint page_num = _get_page_num(ch);

    memset(&acquired_date, 0x00, sizeof(acquired_date));

    obj = nfui_nftable_get_child(g_ntb[page_num], LIC_ACQUIRED, tb_row);

    if (acquired_time == 0)
    {
        nfui_nflabel_set_text((NFLABEL *)obj, "-");
    }
    else
    {
        dtf_get_local_date(acquired_time, acquired_date);
        nfui_nflabel_set_text((NFLABEL *)obj, acquired_date);
    }

    if (expose)
    {
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }

    return 0;
}

static gint _update_local_dev_table(LicenseData *licdata, gint expose)
{
    NF_SYSDB_LICENSE_INFO lic_info;
    gint ret, i, j;

    if (licdata->count == 0)
    {
        _set_license_str(-1, NULL, expose);
        _set_acquired_date_str(-1, 0, expose);
    }
    else
    {
        for (i = 0; i < licdata->count; i++)
        {
            memset(&lic_info, 0x00, sizeof(NF_SYSDB_LICENSE_INFO));

            ret = scm_license_decoding_key(-1, licdata->key_data[i].key, &lic_info);

            if (!ret)
            {
                _set_license_str(-1, lic_info.name, expose);
                _set_acquired_date_str(-1, licdata->key_data[i].acquired_date, expose);
            }
        }
    }

    return 0;
}

static gint _update_cam_table(gint ch, LicenseData *cam_licdata, gint expose)
{
    NF_SYSDB_LICENSE_INFO cam_lic_info;
    gint i, ret;

    if (g_devinfo.profile[ch].connected)
    {
        memset(&cam_lic_info, 0x00, sizeof(NF_SYSDB_LICENSE_INFO));

        if (cam_licdata->count == 0)
        {
            _set_license_str(ch, NULL, expose);
            _set_acquired_date_str(ch, 0, expose);
        }
        else
        {
            for (i = 0; i < cam_licdata->count; i++)
            {
                ret = scm_license_decoding_key(ch, cam_licdata->key_data[i].key, &cam_lic_info);

                if (ret == 0)
                {
                    _set_license_str(ch, cam_lic_info.name, expose);
                    _set_acquired_date_str(ch, cam_licdata->key_data[i].acquired_date, expose);
                }
            }
        }

        nfui_nfobject_enable(g_enter[ch]);

        if (expose)
        {
            nfui_signal_emit(g_enter[ch], GDK_EXPOSE, TRUE);
        }
    }
}

static gboolean post_devinfobutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        VW_License_DevInfo_Popup_Open(g_curwnd, &g_devinfo);
    }

    return FALSE;
}

static gboolean post_renewbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    mb_type ret;
    gint i;

    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        ret = nftool_mbox(g_curwnd, "CONFIRM", "All unsaved license information will be initialized.\nDo you continue?", NFTOOL_MB_YESNO);
        if (ret == NFTOOL_MB_CANCEL)
            return FALSE;

        _renew_dev_info();
        _renew_lic_info();

        _init_table();

        _update_local_dev_table(&g_licdata, 1);

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            _update_cam_table(i, &g_cam_licdata[i], 1);
        }
    }

    return FALSE;
}

static gboolean post_serverbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NF_SYSDB_LICENSE_INFO recive_key[MAX_LICENSE_CNT];
    CAM_PROFILE_T prof;
    gint i, j, lic_cnt = -1;
    time_t acquired_time = 0;

    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        if (g_wait_pop)
        {
            nftool_remove_waitbox(g_wait_pop);
            g_wait_pop = NULL;
        }

        g_wait_pop = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "Getting license from server...", "");

        scm_license_get_from_server(IRET_COMPLETED_GETTING_LICENSE);
    }
    else if (evt->type == IRET_COMPLETED_GETTING_LICENSE)
    {
        LICENSE_KEY_T *lic_data = ((CMM_MESSAGE_T *)data)->data;
        NF_SYSDB_LICENSE_INFO lic_info;
        gint i, j, ch;
        gint ret = ((CMM_MESSAGE_T *)data)->param;

        if (g_wait_pop)
        {
            nftool_remove_waitbox(g_wait_pop);
            g_wait_pop = NULL;
        }

        if (ret == -2)
        {
            nftool_mbox(g_curwnd, "ERROR", "It can't connect license server.", NFTOOL_MB_OK);
            return FALSE;
        }
        else
        {
            nftool_mbox(g_curwnd, "NOTICE", "Complete!", NFTOOL_MB_OK);
        }

        g_message("[%s, %d] IRET_COMPLETED_GETTING_LICENSE", __FUNCTION__, __LINE__);

        for (i = 0; i < GUI_CHANNEL_CNT + 1; i++)
        {
            for (j = 0; j < lic_data[i].key_count; j++)
            {
                memset(&lic_info, 0x00, sizeof(NF_SYSDB_LICENSE_INFO));
                ch = i - 1;

                if (_is_maximum_license(ch))
                    continue;

                if (_is_exist_license(ch, lic_data[i].lic_info[j].key))
                    continue;

                if (scm_license_decoding_key(ch, lic_data[i].lic_info[j].key, &lic_info))
                    continue;

                _set_license_str(ch, lic_info.name, 1);

                acquired_time = ifn_get_local_timet();
                _set_acquired_date_str(ch, acquired_time, 1);

                if (ch == -1)
                {
                    strcpy(g_licdata.key_data[g_licdata.count].key, lic_data[i].lic_info[j].key);
                    g_licdata.key_data[g_licdata.count].acquired_date = acquired_time;
                    g_licdata.count++;
                }
                else
                {
                    strcpy(g_cam_licdata[ch].key_data[g_cam_licdata[ch].count].key, lic_data[i].lic_info[j].key);
                    g_cam_licdata[ch].key_data[g_cam_licdata[ch].count].acquired_date = acquired_time;
                    g_cam_licdata[ch].count++;
                }
            }
        }
    }
    else if (evt->type == GDK_DELETE)
    {
        uxm_unreg_imsg_event(obj, IRET_COMPLETED_GETTING_LICENSE);
    }

    return FALSE;
}

static gboolean post_detailbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;
    }

    return FALSE;
}

static gboolean post_add_licensebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NF_SYSDB_LICENSE_INFO out_key;
    gint ch;
    time_t acquired_time = 0;
    gchar *code = NULL;
    gchar buf[256];

    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        memset(&out_key, 0x00, sizeof(NF_SYSDB_LICENSE_INFO));

        ch = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "ch"));

        code = VW_License_EnterCode_Popup(g_curwnd, ch, &out_key, &g_licdata, g_cam_licdata);

        if (code)
        {
            _set_license_str(ch, out_key.name, 1);

            acquired_time = ifn_get_local_timet();
            _set_acquired_date_str(ch, acquired_time, 1);

            if (ch == -1)
            {
                strcpy(g_licdata.key_data[g_licdata.count].key, code);
                g_licdata.key_data[g_licdata.count].acquired_date = acquired_time;
                g_licdata.count++;
            }
            else
            {
                strcpy(g_cam_licdata[ch].key_data[g_cam_licdata[ch].count].key, code);
                g_cam_licdata[ch].key_data[g_cam_licdata[ch].count].acquired_date = acquired_time;
                g_cam_licdata[ch].count++;
            }

            ifree(code);
            code = NULL;
        }
    }

    return FALSE;
}

static gboolean post_prev_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
    gchar buf[64];

    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON)
        {
            return FALSE;
        }

        for (i = 0; i < PAGE_FIXED_CNT; i++)
        {
            if (nfui_nfobject_is_shown((NFOBJECT *)g_page_fixed[i]))
            {
                break;
            }
        }

        if (i == PAGE_FIXED_CNT)
            return FALSE;

        if (i == 0)
            return FALSE;

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

        nfui_make_key_hierarchy((NFWINDOW *)nfui_nfobject_get_top(obj));

        nfui_flip(obj);
        nfui_off_backscr(obj);
    }

    return FALSE;
}

static gboolean post_next_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
    gchar buf[64];

    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON)
        {
            return FALSE;
        }

        for (i = 0; i < PAGE_FIXED_CNT; i++)
        {
            if (nfui_nfobject_is_shown((NFOBJECT *)g_page_fixed[i]))
            {
                break;
            }
        }

        if (i == PAGE_FIXED_CNT)
            return FALSE;

        if (i == (PAGE_FIXED_CNT - 1))
            return FALSE;

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

        nfui_make_key_hierarchy((NFWINDOW *)nfui_nfobject_get_top(obj));

        nfui_flip(obj);
        nfui_off_backscr(obj);
    }

    return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *child;
    gint i;

    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        if (memcmp(&g_licdata, &g_org_licdata, sizeof(LicenseData)))
        {
            memmove(&g_licdata, &g_org_licdata, sizeof(LicenseData));

            _init_local_table();
            _update_local_dev_table(&g_licdata, 1);
        }

        if (memcmp(&g_cam_licdata, &g_org_cam_licdata, sizeof(LicenseData) * GUI_CHANNEL_CNT))
        {
            memmove(&g_cam_licdata, &g_org_cam_licdata, sizeof(LicenseData) * GUI_CHANNEL_CNT);

            _init_cam_table();
            for (i = 0; i < GUI_CHANNEL_CNT; i++)
            {
                _update_cam_table(i, &g_cam_licdata[i], 1);
            }
        }
    }

    return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
    gint changed_local = 0, changed_cam = 0;

    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        if (memcmp(&g_licdata, &g_org_licdata, sizeof(LicenseData)))
            changed_local = 1;
            
        if (memcmp(&g_cam_licdata, &g_org_cam_licdata, sizeof(LicenseData) * GUI_CHANNEL_CNT))
            changed_cam = 1;

        if (changed_local || changed_cam)
        {
            if (changed_local)
            {
                memmove(&g_org_licdata, &g_licdata, sizeof(LicenseData));
                DAL_set_license_data(&g_licdata);
                VW_SetupSystem_set_changeflag(1);
            }

            if (changed_cam)
            {
                memmove(&g_org_cam_licdata, &g_cam_licdata, sizeof(LicenseData) * GUI_CHANNEL_CNT);

                for (i = 0; i < GUI_CHANNEL_CNT; i++)
                {
                    scm_license_set_cam_license(i, &g_cam_licdata[i]);
                }
            }

            nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
        }
    }

    return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        VW_SysLicense_tab_out_handler();
        VW_SetupSystem_Destroy(obj);
    }

    return FALSE;
}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_DELETE)
    {
        g_curwnd = 0;
    }

    return FALSE;
}

void VW_Init_SysLicense_page(NFOBJECT *parent)
{
    NFOBJECT *content_fixed;
    NFOBJECT *fixed;
    NFOBJECT *ntb;
    NFOBJECT *obj;
    NFOBJECT *main_page_fixed;
    NFOBJECT *page_fixed[PAGE_FIXED_CNT];
    NFOBJECT *page_ntb[PAGE_FIXED_CNT];

    guint width[6];
    gint pos_x, pos_y;
    gint size_w, size_h;
    guint i;

    gchar strBuf[32];

    gint ret;
    gint page_num, row_num;
    GdkPixbuf *prev_img[NFOBJECT_STATE_COUNT];
    GdkPixbuf *next_img[NFOBJECT_STATE_COUNT];

    _init_lic_db();
    _init_cam_lic_db();
    _init_dev_info();

    prev_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_N_BTN), NULL);
    prev_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_O_BTN), NULL);
    prev_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_P_BTN), NULL);
    prev_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_D_BTN), NULL);

    next_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_N_BTN), NULL);
    next_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_O_BTN), NULL);
    next_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_P_BTN), NULL);
    next_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_D_BTN), NULL);

    g_curwnd = nfui_nfobject_get_top(parent);

    content_fixed = (NFOBJECT *)nfui_nffixed_new();
    nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(content_fixed, MENU_V_INNER_W, MENU_V_INNER_H);
    nfui_nfobject_show(content_fixed);
    nfui_nffixed_put((NFFIXED *)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);

    pos_x = TABLE_LEFT;
    pos_y = TABLE_TOP;

    width[LIC_TITLE] = 200;
    width[LIC_LICENSE] = 600;
    width[LIC_ACQUIRED] = 240;
    width[LIC_ADD] = 200;
    width[LIC_DETIAL] = 120;

    ntb = nfui_nftable_new(LIC_COL_CNT, 1, 1, 1, width, 40);
    nfui_nfobject_show(ntb);
    nfui_nffixed_put(content_fixed, ntb, pos_x, pos_y);

    obj = nfui_nflabel_new_with_pango_font("LICENSE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE *)ntb, obj, LIC_LICENSE, 0);

    obj = nfui_nflabel_new_with_pango_font("ACQUIRED DATE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE *)ntb, obj, LIC_ACQUIRED, 0);

    size_w = 0;
    for (i = 0; i < LIC_COL_CNT; i++)
    {
        size_w += width[i];
    }

    pos_y += 41;

    main_page_fixed = (NFOBJECT *)nfui_nffixed_new();
    nfui_nfobject_modify_bg(main_page_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(main_page_fixed, size_w, ROW_CNT_PER_PAGE * (40 + 1) + 80);
    nfui_nfobject_show(main_page_fixed);
    nfui_nffixed_put((NFFIXED *)content_fixed, main_page_fixed, pos_x, pos_y);

    for (i = 0; i < PAGE_FIXED_CNT; i++)
    {
        g_page_fixed[i] = (NFOBJECT *)nfui_nffixed_new();
        nfui_nfobject_modify_bg(g_page_fixed[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
        nfui_nfobject_set_size(g_page_fixed[i], size_w, ROW_CNT_PER_PAGE * (40 + 1));
        nfui_nffixed_put((NFFIXED *)main_page_fixed, g_page_fixed[i], 0, 0);

        page_ntb[i] = (NFOBJECT *)nfui_nftable_new(LIC_COL_CNT, ROW_CNT_PER_PAGE, 1, 1, width, 40);
        nfui_nfobject_show(page_ntb[i]);
        nfui_nffixed_put((NFFIXED *)g_page_fixed[i], page_ntb[i], 0, 0);
        g_ntb[i] = page_ntb[i];
    }
    nfui_nfobject_show(g_page_fixed[0]);

    nfui_get_pixbuf_size(prev_img[0], &size_w, &size_h);

    obj = (NFOBJECT *)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(obj), prev_img);
    nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)main_page_fixed, obj, (main_page_fixed->width / 2) - (size_w + 60), main_page_fixed->height - size_h);
    nfui_regi_post_event_callback(obj, post_prev_event_handler);

    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, "1 / %d", PAGE_FIXED_CNT);

    obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
    nfui_nflabel_set_align((NFLABEL *)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 100, (guint)size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)main_page_fixed, obj, (main_page_fixed->width - 100) / 2, main_page_fixed->height - size_h);
    g_lb_page_num = obj;

    obj = (NFOBJECT *)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(obj), next_img);
    nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)main_page_fixed, obj, (main_page_fixed->width / 2) + 60, main_page_fixed->height - size_h);
    nfui_regi_post_event_callback(obj, post_next_event_handler);

    page_num = row_num = 0;

//===> LOCAL DEVICE
#if defined(_IPX_MODEL_UX)
    obj = nfui_nflabel_new_with_pango_font("NVR", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
#else
    obj = nfui_nflabel_new_with_pango_font("DVR", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
#endif
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE *)page_ntb[page_num], obj, LIC_TITLE, row_num);

    obj = (NFOBJECT *)nfui_nflabel_new_text_box("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL *)obj, NFTEXTBOX_TYPE_OUTPUT);
    nfui_nfobject_use_focus(obj, FALSE);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE *)page_ntb[page_num], obj, LIC_LICENSE, row_num);

    obj = (NFOBJECT *)nfui_nflabel_new_text_box("-", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL *)obj, NFTEXTBOX_TYPE_OUTPUT);
    nfui_nfobject_use_focus(obj, FALSE);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE *)page_ntb[page_num], obj, LIC_ACQUIRED, row_num);

    obj = nftool_normal_button_create_type3("ADD LICENSE", 200);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE *)page_ntb[page_num], obj, LIC_ADD, row_num);
    nfui_regi_post_event_callback(obj, post_add_licensebutton_event_handler);
    nfui_nfobject_set_data(obj, "ch", GINT_TO_POINTER(-1));

    obj = nftool_normal_button_create_type3("DETAIL", 120);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    // nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE *)page_ntb[page_num], obj, LIC_DETIAL, row_num);
    nfui_regi_post_event_callback(obj, post_detailbutton_event_handler);

    row_num++;
    //===> CAMERA
    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        memset(strBuf, 0x00, sizeof(strBuf));
        var_get_camtitle(strBuf, i);

        obj = nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE *)page_ntb[page_num], obj, LIC_TITLE, row_num);

        obj = (NFOBJECT *)nfui_nflabel_new_text_box("N/A", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
        nfui_nflabel_set_skin_type((NFLABEL *)obj, NFTEXTBOX_TYPE_OUTPUT);
        nfui_nfobject_use_focus(obj, FALSE);
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE *)page_ntb[page_num], obj, LIC_LICENSE, row_num);

        obj = (NFOBJECT *)nfui_nflabel_new_text_box("N/A", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
        nfui_nflabel_set_skin_type((NFLABEL *)obj, NFTEXTBOX_TYPE_OUTPUT);
        nfui_nfobject_use_focus(obj, FALSE);
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE *)page_ntb[page_num], obj, LIC_ACQUIRED, row_num);

        obj = nftool_normal_button_create_type3("ADD LICENSE", 200);
        nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE *)page_ntb[page_num], obj, LIC_ADD, row_num);
        nfui_regi_post_event_callback(obj, post_add_licensebutton_event_handler);
        nfui_nfobject_set_data(obj, "ch", GINT_TO_POINTER(i));
        if (!g_devinfo.profile[i].connected)
            nfui_nfobject_disable(obj);
        g_enter[i] = obj;

        obj = nftool_normal_button_create_type3("DETAIL", 120);
        nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
        // nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE *)page_ntb[page_num], obj, LIC_DETIAL, row_num);
        nfui_regi_post_event_callback(obj, post_detailbutton_event_handler);

        row_num++;

        if (row_num == ROW_CNT_PER_PAGE)
        {
            row_num = 0;
            page_num++;
        }
    }

    _update_local_dev_table(&g_licdata, 0);

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        _update_cam_table(i, &g_cam_licdata[i], 0);
    }

    pos_y += main_page_fixed->height;

    obj = nftool_normal_button_create_type3("ADD AUTOMATICALLY", 250);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)content_fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_serverbutton_event_handler);

    uxm_reg_imsg_event(obj, IRET_COMPLETED_GETTING_LICENSE);

    pos_x += 250 + 4;

    obj = nftool_normal_button_create_type3("RENEW", 250);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    // nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)content_fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_renewbutton_event_handler);

    pos_x = (TABLE_LEFT + width[0] + width[1] + width[2] + width[3] + 3) - 250;

    obj = nftool_normal_button_create_type3("DEVICE INFORMATION", 250);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)content_fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_devinfobutton_event_handler);

    obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)parent, obj, MENU_V_BTN_R3_X, MENU_V_BTN_Y);
    nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);

    obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)parent, obj, MENU_V_BTN_R2_X, MENU_V_BTN_Y);
    nfui_regi_post_event_callback(obj, post_applybutton_event_handler);

    obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)parent, obj, MENU_V_BTN_R1_X, MENU_V_BTN_Y);
    nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

    nfui_regi_post_event_callback(parent, post_page_event_handler);
}

gboolean VW_SysLicense_tab_out_handler(void)
{
    NFOBJECT *obj;
    mb_type ret;
    gint i;
    gint changed_local = 0, changed_cam = 0;

    if (memcmp(&g_licdata, &g_org_licdata, sizeof(LicenseData)))
        changed_local = 1;
    if (memcmp(&g_cam_licdata, &g_org_cam_licdata, sizeof(LicenseData) * GUI_CHANNEL_CNT))
        changed_cam = 1;

    if (changed_local || changed_cam)
    {
        ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

        if (ret == NFTOOL_MB_OK)
        {
            if (changed_local)
            {
                memmove(&g_org_licdata, &g_licdata, sizeof(LicenseData));
                DAL_set_license_data(&g_licdata);
                VW_SetupSystem_set_changeflag(1);
            }

            if (changed_cam)
            {
                memmove(&g_org_cam_licdata, &g_cam_licdata, sizeof(LicenseData) * GUI_CHANNEL_CNT);

                for (i = 0; i < GUI_CHANNEL_CNT; i++)
                {
                    scm_license_set_cam_license(i, &g_cam_licdata[i]);
                }
            }
        }
        else
        {
            if (changed_local)
            {
                memmove(&g_licdata, &g_org_licdata, sizeof(LicenseData));

                _init_local_table();
                _update_local_dev_table(&g_licdata, 0);
            }

            if (changed_cam)
            {
                memmove(&g_cam_licdata, &g_org_cam_licdata, sizeof(LicenseData) * GUI_CHANNEL_CNT);

                _init_cam_table();
                for (i = 0; i < GUI_CHANNEL_CNT; i++)
                {
                    _update_cam_table(i, &g_cam_licdata[i], 0);
                }
            }
        }
    }

    return FALSE;
}
