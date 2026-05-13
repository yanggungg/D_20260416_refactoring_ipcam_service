#include "nf_afx.h"

#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "support/color.h"
#include "support/multi_language_support.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfspinbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nfimage.h"

#include "vw_sys_net_main.h"
#include "nf_api_eventlog.h"
#include "nf_network.h"
#include "vw_sys_net_ddns.h"

#include "vw_vkeyboard.h"

#include "../service/ddns2_manager.h"
#include "nf_api_disk.h"
#include "scm.h"
#include "uxm.h"
#include "ix_mem.h"

#include "viewers/vw_sys_net_info_map.h"

#define MAX_STRING_SIZE             (63)
#define MAX_DOMAIN_STRING           (127)

#define ND_LABLE_LEFT               (48)
#define ND_LABLE_TOP                (42)

#define ND_LABLE_WIDTH              (300)
#define ND_LABLE_HEIGHT             (40)

#define ND_CELL_WIDTH               (400)
#define ND_CELL_HEIGHT              (ND_LABLE_HEIGHT)

#define ND_COL_SPACE                (2)
#define ND_ROW_SPACE                (2)

typedef enum
{
    ND_DDNS_ROW = 0,
    ND_DDNS_SERVER_ROW,
    ND_DDNS_NAME_ROW,
    ND_DDNS_ID_ROW,
    ND_DDNS_PWD_ROW,
    ND_DDNS_ADDR_ROW,
    ND_DDNS_STATUS_ROW,
    ND_DDNS_P2P_ROW,
    NUM_ND_ROWS,
}
nd_row_type;

typedef enum
{
    UNIMO_CHK_ROW = 0,
    UNIMO_SERVER_ROW,
    UNIMO_PORT_ROW,
    UNIMO_STATUS_ROW,
    NUM_UNIMO_ROWS,
}
unimo_row_type;

enum {
    NDB_CHECK_NAME = 0,
    NDB_TEST_REGISTER,
    NDB_TEST_CONNECT,
    UNIMO_CHECK_REGISTER,
    UNIMO_STATUS_REFRESH,
};

enum {
    NDB_CANCEL,
    NDB_APPLY,
    NDB_CLOSE,
};

typedef struct _DDNS_PARAM_T {
    DDNS_INFO_T info;
    gchar       addr[STRING_SIZE_256];
} DDNS_PARAM_T;

typedef struct _DDNS_NET_T {
    DDNS_CFG_T      cfg;
    DDNS_PARAM_T    param;
    DDNS_PARAM_T    org_param;
} DDNS_NET_T;

static NFOBJECT *wait_pop = NULL;
guint msg_timer_id = 0;
static guint status_timer_id = 0;
static guint g_status_unimo = 0;
static gboolean g_chk_unimo = 0;
static char g_status_unimo_str[STRING_SIZE_64];

static DDNSData ddnsdata;
static DDNSData org_ddnsdata;
static UNIMOData unimodata;
static UNIMOData org_unimodata;
static DDNS_NET_T g_ddnsNet[MAX_SUPPORT_DDNS];

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *value[NUM_ND_ROWS] = {0, };
static NFOBJECT *unimo_value[NUM_UNIMO_ROWS] = {0, };
static NFOBJECT *tst_btn[2] = {0, };
static NFOBJECT *uni_btn = 0;
static NFOBJECT *g_sequrinet;
static gboolean *g_org_status = FALSE;
static gboolean *g_status = FALSE;


static gint _init_ddns_info()
{
    gint i;

    memset(&ddnsdata, 0x00, sizeof(DDNSData));
    memset(&org_ddnsdata, 0x00, sizeof(DDNSData));
    memset(&g_ddnsNet, 0x00, sizeof(DDNS_NET_T)*MAX_SUPPORT_DDNS);

    for (i = 0; i < NUM_ND_ROWS; i++)
        value[i] = 0;

    for (i = 0; i < 2; i++)
        tst_btn[i] = 0;

    DAL_get_ddns_data(&ddnsdata);
    ifn_tolower(ddnsdata.server);

    for (i = 0; i < var_get_ddns_cnt(); i++)
    {
        var_get_ddns_cfg(&g_ddnsNet[i].cfg, i);
        ifn_tolower(g_ddnsNet[i].cfg.server);

        strcpy(g_ddnsNet[i].param.info.server, g_ddnsNet[i].cfg.server);

        if (!strcmp(g_ddnsNet[i].param.info.server, ddnsdata.server))
        {
            strcpy(g_ddnsNet[i].param.info.hostname, ddnsdata.host_name);
            strcpy(g_ddnsNet[i].param.info.id, ddnsdata.id);
            strcpy(g_ddnsNet[i].param.info.pwd, ddnsdata.passwd);
        }
        else
        {
            scm_get_mac_addr_str(g_ddnsNet[i].param.info.hostname, 64);
        }

		if (!strcmp(g_ddnsNet[i].param.info.server, "s1.co.kr"))
            g_sprintf(g_ddnsNet[i].param.addr, "%s", g_ddnsNet[i].param.info.hostname);
		else if (!strcmp(g_ddnsNet[i].param.info.server, "no-ip.com")) 
			g_sprintf(g_ddnsNet[i].param.addr, "%s", g_ddnsNet[i].param.info.hostname);
		else
			g_sprintf(g_ddnsNet[i].param.addr, "%s.%s", g_ddnsNet[i].param.info.hostname, g_ddnsNet[i].param.info.server);

        ifn_tolower(g_ddnsNet[i].param.addr);

        memcpy(&g_ddnsNet[i].org_param, &g_ddnsNet[i].param, sizeof(DDNS_PARAM_T));
    }

    return 0;
}

static gint _init_unimo_info()
{
	int i;
	memset(&unimodata, 0x00, sizeof(UNIMOData));
    memset(&org_unimodata, 0x00, sizeof(UNIMOData));

	for (i=0; i < NUM_UNIMO_ROWS; i++)
        unimo_value[i] = 0;

	DAL_get_Unimo_Data(&unimodata);

	return 0;
}

static gint _sync_ddns_server_field(gint idx)
{
    if (ddnsdata.enable)
    {
        nfui_nfobject_enable(value[ND_DDNS_SERVER_ROW]);
    }
    else
    {
        nfui_nfobject_disable(value[ND_DDNS_SERVER_ROW]);
    }

    if (var_get_ddns_cnt() >= 1) {
        nfui_combobox_set_index_no_expose((NFCOMBOBOX*)value[ND_DDNS_SERVER_ROW], idx);
    }

    return 0;
}

static gint _sync_ddns_host_field(gint idx)
{
    if (!value[ND_DDNS_NAME_ROW]) return -1;
    if (!tst_btn[0]) return -1;

    if ((g_ddnsNet[idx].cfg.on_nvr) && (ddnsdata.enable))
    {
		nfui_nfobject_enable(tst_btn[0]);
		
		if(strcmp(g_ddnsNet[idx].param.info.server, "udrdns.net"))
			nfui_nfobject_enable(value[ND_DDNS_NAME_ROW]);
		else
			nfui_nfobject_disable(value[ND_DDNS_NAME_ROW]);
    }
    else
    {
        nfui_nfobject_disable(value[ND_DDNS_NAME_ROW]);
        nfui_nfobject_disable(tst_btn[0]);
    }

    nfui_nflabel_set_text((NFLABEL*)value[ND_DDNS_NAME_ROW], g_ddnsNet[idx].param.info.hostname);

    return 0;
}

static gint _sync_ddns_id_field(gint idx)
{
    if (!value[ND_DDNS_ID_ROW]) return -1;

    if ((g_ddnsNet[idx].cfg.on_id) && (ddnsdata.enable))
    {
        nfui_nfobject_enable(value[ND_DDNS_ID_ROW]);
    }
    else
    {
        nfui_nfobject_disable(value[ND_DDNS_ID_ROW]);
    }

    nfui_nflabel_set_text((NFLABEL*)value[ND_DDNS_ID_ROW], g_ddnsNet[idx].param.info.id);

    return 0;
}

static gint _sync_ddns_pwd_field(gint idx)
{
    if (!value[ND_DDNS_PWD_ROW]) return -1;

    if ((g_ddnsNet[idx].cfg.on_pwd) && (ddnsdata.enable))
    {
        nfui_nfobject_enable(value[ND_DDNS_PWD_ROW]);
    }
    else
    {
        nfui_nfobject_disable(value[ND_DDNS_PWD_ROW]);
    }

    nfui_nflabel_set_text((NFLABEL*)value[ND_DDNS_PWD_ROW], g_ddnsNet[idx].param.info.pwd);

    return 0;
}

static gint _sync_ddns_addr_field(gint idx)
{
    if (!value[ND_DDNS_ADDR_ROW]) return -1;
    if (!tst_btn[1]) return -1;

    if ((g_ddnsNet[idx].cfg.on_mac) && (ddnsdata.enable))
    {
        nfui_nfobject_enable(value[ND_DDNS_ADDR_ROW]);
        nfui_nfobject_enable(tst_btn[1]);
    }
    else
    {
        nfui_nfobject_disable(value[ND_DDNS_ADDR_ROW]);
        nfui_nfobject_disable(tst_btn[1]);
    }

    nfui_nflabel_set_text((NFLABEL*)value[ND_DDNS_ADDR_ROW], g_ddnsNet[idx].param.addr);

    return 0;
}

static gint _refresh_ddns_addr(gint idx)
{
    gchar strBuf[256];

    memset(strBuf, 0x00, sizeof(strBuf));

    if (!strcmp(g_ddnsNet[idx].param.info.server, "s1.co.kr"))
        g_sprintf(strBuf, "%s", g_ddnsNet[idx].param.info.hostname);  
	else if (!strcmp(g_ddnsNet[idx].param.info.server, "no-ip.com"))
		g_sprintf(strBuf, "%s", g_ddnsNet[idx].param.info.hostname);
	else
		g_sprintf(strBuf, "%s.%s", g_ddnsNet[idx].param.info.hostname, g_ddnsNet[idx].cfg.server);    

    strcpy(g_ddnsNet[idx].param.addr, strBuf);
    return 0;
}

static gint _get_current_cfg_idx()
{
    gint idx;

    if (var_get_ddns_cnt() == 1) idx = 0;
    else idx = nfui_combobox_get_cur_index((NFCOMBOBOX*)value[ND_DDNS_SERVER_ROW]);

    return idx;
}

static void _unimo_status_update()
{
    gint ret = 0;

    g_status_unimo = nf_ddns_get_unimo_checker_status();
    
    switch (g_status_unimo)
    {
        case UNIMO_CHECK_RES_SUCCESS:
            strcpy(g_status_unimo_str, "SUCCESS");
        break;
		case UNIMO_CHECK_RES_INITIALIZE:
			strcpy(g_status_unimo_str, "INITIALIZING");
		break;
        case UNIMO_CHECK_RES_INTERNAL_ERR:
            strcpy(g_status_unimo_str, "INTERNAL ERROR");
        break;
	    case UNIMO_CHECK_RES_NETWORK_ERR:
            strcpy(g_status_unimo_str, "NETWORK ERROR");
        break;
	    case UNIMO_CHECK_RES_DATA_SEND_ERR:
            strcpy(g_status_unimo_str, "DATA SEND ERROR");
        break;
	    case UNIMO_CHECK_RES_DATA_RECV_ERR:         // 데이터 수신 오류
            strcpy(g_status_unimo_str, "DATA RECEIVE ERROR");
        break;
	    case UNIMO_CHECK_RES_FAIL_ERR:                       // 실패
            strcpy(g_status_unimo_str, "FAIL");
        break;
	    case UNIMO_CHECK_RES_BADAUTH_ERR:             // 권한 없음
            strcpy(g_status_unimo_str, "BAD AUTHORITY ERROR");
        break;
	    case UNIMO_CHECK_RES_UNKNOWN_ERR:           // 알수없는 오류
            strcpy(g_status_unimo_str, "UNKNOWN ERROR");
        break;
    }
	 
}

static gboolean post_unimo_chk_onoff_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        gint idx = 0;

        unimodata.enable = nfui_spin_button_get_index((NFSPINBUTTON*)obj);
        
        if(unimodata.enable)
        {
            for (idx=UNIMO_SERVER_ROW; idx<NUM_UNIMO_ROWS; idx++)
            {
                nfui_nfobject_enable(unimo_value[idx]);
                nfui_signal_emit(unimo_value[idx], GDK_EXPOSE, TRUE);
            }
            
            nfui_nfobject_enable(uni_btn);
            nfui_signal_emit(uni_btn, GDK_EXPOSE, TRUE);
        }
        else
        {
            for (idx=UNIMO_SERVER_ROW; idx<NUM_UNIMO_ROWS; idx++)
            {
                nfui_nfobject_disable(unimo_value[idx]);
                nfui_signal_emit(unimo_value[idx], GDK_EXPOSE, TRUE);
            }
            
            nfui_nfobject_disable(uni_btn);
            nfui_signal_emit(uni_btn, GDK_EXPOSE, TRUE);
        }
    }

    return FALSE;
}

static gboolean post_unimo_field_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkEventKey *kevt;
    KEYPAD_KID kpid = KEYPAD_NONE;
    gint max_string_size = MAX_STRING_SIZE;

    if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;
    }

    if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
    {
        NFOBJECT *top;
        gchar *strTemp = NULL;
        gint numTemp = 0;
		gint numRet = -1;
        guint x, y;
        gint idx;
        gchar buf[256];

        if(kpid == KEYPAD_ENTER)
        {
            nfui_nfobject_get_offset(obj, &x, &y);
            top = nfui_nfobject_get_top(obj);

            x += (obj->width)/2 + top->x;
            y += obj->height + top->y;
        }
        else
        {
            if(evt->button.button == MOUSE_RIGTH_BUTTON)
            {
                return FALSE;
            }

            nfui_nfobject_get_window_pos(obj, &x, &y);

            x += ((GdkEventButton*)evt)->x;
            y += ((GdkEventButton*)evt)->y;
        }

		if(obj == unimo_value[UNIMO_SERVER_ROW])
			strTemp = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), x, y, max_string_size, VKEY_NORMAL);
		else
        {
            numTemp = nfui_nflabel_get_number((NFLABEL*)obj);
		    numRet = NumberKey_Open(g_curwnd, numTemp, x, y, 65535);
        }

        if(strTemp)
        {
            if (strlen(strTemp) == 0) return FALSE;
            if (obj == unimo_value[UNIMO_SERVER_ROW])
            {
                nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
                nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
            }
            
            strcpy(unimodata.server, strTemp);
            ifree(strTemp);
            strTemp = NULL;
        }
        else if(numRet != -1)
        {
            if (obj == unimo_value[UNIMO_PORT_ROW]) 
            {
                nfui_nflabel_set_number((NFLABEL*)obj, numRet);
                nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
            }
            unimodata.port = numRet;
        }
    }

    return FALSE;
}

static gboolean post_unimo_register_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint ret = 0;
	gint idx = 0;

    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		wait_pop = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
		
		scm_register_unimo_with_info(&unimodata, IRET_SCM_REG_UNIMO);
    }

    return FALSE;
}

static gboolean post_sequrinet_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_SPINBUTTON_CHANGED)
	{
		g_status = nfui_spin_button_get_index((NFSPINBUTTON*)obj);
		ddnsdata.p2p_enable = g_status;
	}

	return FALSE;
}

static gboolean post_ddns_onoff_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        gint idx = 0;

        ddnsdata.enable = nfui_spin_button_get_index((NFSPINBUTTON*)obj);

        idx = _get_current_cfg_idx();

        if (_sync_ddns_server_field(idx) == 0) {
            nfui_signal_emit(value[ND_DDNS_SERVER_ROW], GDK_EXPOSE, TRUE);
        }

        if (_sync_ddns_host_field(idx) == 0) {
            nfui_signal_emit(value[ND_DDNS_NAME_ROW], GDK_EXPOSE, TRUE);
            nfui_signal_emit(tst_btn[0], GDK_EXPOSE, TRUE);
        }

        if (_sync_ddns_id_field(idx) == 0) {
            nfui_signal_emit(value[ND_DDNS_ID_ROW], GDK_EXPOSE, TRUE);
        }

        if (_sync_ddns_pwd_field(idx) == 0) {
            nfui_signal_emit(value[ND_DDNS_PWD_ROW], GDK_EXPOSE, TRUE);
        }

        if (_sync_ddns_addr_field(idx) == 0) {
            nfui_signal_emit(value[ND_DDNS_ADDR_ROW], GDK_EXPOSE, TRUE);
            nfui_signal_emit(tst_btn[1], GDK_EXPOSE, TRUE);
        }
    }

    return FALSE;
}

static gboolean post_ddns_server_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        gint idx;

        idx = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        if (_sync_ddns_host_field(idx) == 0) {
            nfui_signal_emit(value[ND_DDNS_NAME_ROW], GDK_EXPOSE, TRUE);
            nfui_signal_emit(tst_btn[0], GDK_EXPOSE, TRUE);
        }

        if (_sync_ddns_id_field(idx) == 0) {
            nfui_signal_emit(value[ND_DDNS_ID_ROW], GDK_EXPOSE, TRUE);
        }

        if (_sync_ddns_pwd_field(idx) == 0) {
            nfui_signal_emit(value[ND_DDNS_PWD_ROW], GDK_EXPOSE, TRUE);
        }

        if (_sync_ddns_addr_field(idx) == 0) {
            nfui_signal_emit(value[ND_DDNS_ADDR_ROW], GDK_EXPOSE, TRUE);
            nfui_signal_emit(tst_btn[1], GDK_EXPOSE, TRUE);
        }
    }

    return FALSE;
}

static gboolean post_ddns_field_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkEventKey *kevt;
    KEYPAD_KID kpid = KEYPAD_NONE;
    gint max_string_size = MAX_STRING_SIZE;

    if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;
    }

    if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
    {
        NFOBJECT *top;
        gchar *strTemp;
        guint x, y;
        gint idx;
        gchar buf[256];

        if(kpid == KEYPAD_ENTER)
        {
            nfui_nfobject_get_offset(obj, &x, &y);
            top = nfui_nfobject_get_top(obj);

            x += (obj->width)/2 + top->x;
            y += obj->height + top->y;
        }
        else
        {
            if(evt->button.button == MOUSE_RIGTH_BUTTON)
            {
                return FALSE;
            }

            nfui_nfobject_get_window_pos(obj, &x, &y);

            x += ((GdkEventButton*)evt)->x;
            y += ((GdkEventButton*)evt)->y;
        }

        if (obj == value[ND_DDNS_NAME_ROW])
        {
            if (!strcmp(nfui_nflabel_get_text((NFLABEL*)obj), "FUJIKO.BIZ"))
                max_string_size = 24;
        }

		if(obj == value[ND_DDNS_PWD_ROW])
			strTemp = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), x, y, max_string_size, VKEY_PASSWORD);
		else
        	strTemp = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), x, y, max_string_size, VKEY_ALPHANUMERIC);

        if(strTemp)
        {
            if(strlen(strTemp) == 0) return FALSE;

            idx = _get_current_cfg_idx();

            if (obj == value[ND_DDNS_NAME_ROW])
            {
                strcpy(g_ddnsNet[idx].param.info.hostname, strTemp);
                _refresh_ddns_addr(idx);

                nfui_nflabel_set_text((NFLABEL*)value[ND_DDNS_ADDR_ROW], g_ddnsNet[idx].param.addr);
                nfui_signal_emit(value[ND_DDNS_ADDR_ROW], GDK_EXPOSE, TRUE);
            }
            else if (obj == value[ND_DDNS_ID_ROW])
            {
                strcpy(g_ddnsNet[idx].param.info.id, strTemp);
            }
            else if (obj == value[ND_DDNS_PWD_ROW])
            {
                strcpy(g_ddnsNet[idx].param.info.pwd, strTemp);
            }

            nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

            ifree(strTemp);
            strTemp = NULL;
        }
    }
    return FALSE;
}

static gboolean post_p2p_onoff_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        ddnsdata.p2p_enable = nfui_spin_button_get_index((NFSPINBUTTON*)obj);
    }

    return FALSE;
}

static gboolean post_register_test_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint ret = 0;
    gint idx;
    DDNS_INFO_T info;

    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        idx = _get_current_cfg_idx();

        if (g_ddnsNet[idx].cfg.on_nvr)
        {
        if (!strlen(g_ddnsNet[idx].param.info.hostname)) {
            nftool_mbox(g_curwnd, "NOTICE", "DVR name is empty\nFill and save the DVR name.", NFTOOL_MB_OK);
            return FALSE;
        }
        }

        if (g_ddnsNet[idx].cfg.on_id)
        {
            if (!strlen(g_ddnsNet[idx].param.info.id)) {
                nftool_mbox(g_curwnd, "NOTICE", "User name is empty\nFill and save the user name.", NFTOOL_MB_OK);
                return FALSE;
            }
        }

        if (g_ddnsNet[idx].cfg.on_pwd)
        {
            if (!strlen(g_ddnsNet[idx].param.info.pwd)) {
                nftool_mbox(g_curwnd, "NOTICE", "password is empty\nFill and save the password.", NFTOOL_MB_OK);
                return FALSE;
            }
        }

        wait_pop = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "Please wait...", "");
        scm_register_ddns_with_info(&g_ddnsNet[idx].param.info, IRET_SCM_REG_DDNS);
    }

    return FALSE;
}

static gboolean post_connection_test_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gchar *strTemp;
    gint idx;

    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        idx = _get_current_cfg_idx();

        if (g_ddnsNet[idx].cfg.on_nvr)
        {
        if (!strlen(g_ddnsNet[idx].param.info.hostname)) {
            nftool_mbox(g_curwnd, "NOTICE", "DVR name is empty\nFill and save the DVR name.", NFTOOL_MB_OK);
            return FALSE;
        }
        }

        if (g_ddnsNet[idx].cfg.on_id)
        {
            if (!strlen(g_ddnsNet[idx].param.info.id)) {
                nftool_mbox(g_curwnd, "NOTICE", "User name is empty\nFill and save the user name.", NFTOOL_MB_OK);
                return FALSE;
            }
        }

        if (g_ddnsNet[idx].cfg.on_pwd)
        {
            if (!strlen(g_ddnsNet[idx].param.info.pwd)) {
                nftool_mbox(g_curwnd, "NOTICE", "password is empty\nFill and save the password.", NFTOOL_MB_OK);
                return FALSE;
            }
        }

		if (!strcmp(g_ddnsNet[idx].param.info.server, "no-ip.com")) 
		{
			g_sprintf(g_ddnsNet[idx].param.addr, "%s", g_ddnsNet[idx].param.info.hostname);
		}
        wait_pop = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "Please wait...", "");
        scm_test_ddns_with_info(&g_ddnsNet[idx].param.info, IRET_SCM_TST_DDNS);
    }
    return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, init_ddns;

    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON)
        {
            return FALSE;
        }

        g_memmove(&ddnsdata, &org_ddnsdata, sizeof(DDNSData));
        g_memmove(&unimodata, &org_unimodata, sizeof(UNIMOData));

        for (i = 0; i < var_get_ddns_cnt(); i++)
        {
            memcpy(&g_ddnsNet[i].param, &g_ddnsNet[i].org_param, sizeof(DDNS_PARAM_T));

            if (!strcmp(g_ddnsNet[i].param.info.server, ddnsdata.server))
                init_ddns = i;
        }

        nfui_spin_button_set_index_no_expose(value[ND_DDNS_ROW], ddnsdata.enable);
        nfui_signal_emit(value[ND_DDNS_ROW], GDK_EXPOSE, TRUE);

        if (_sync_ddns_server_field(init_ddns) == 0) {
            nfui_signal_emit(value[ND_DDNS_SERVER_ROW], GDK_EXPOSE, TRUE);
        }

        if (_sync_ddns_host_field(init_ddns) == 0) {
            nfui_signal_emit(value[ND_DDNS_NAME_ROW], GDK_EXPOSE, TRUE);
            nfui_signal_emit(tst_btn[0], GDK_EXPOSE, TRUE);
        }

        if (_sync_ddns_id_field(init_ddns) == 0) {
            nfui_signal_emit(value[ND_DDNS_ID_ROW], GDK_EXPOSE, TRUE);
        }

        if (_sync_ddns_pwd_field(init_ddns) == 0) {
            nfui_signal_emit(value[ND_DDNS_PWD_ROW], GDK_EXPOSE, TRUE);
        }

        if (_sync_ddns_addr_field(init_ddns) == 0) {
            nfui_signal_emit(value[ND_DDNS_ADDR_ROW], GDK_EXPOSE, TRUE);
            nfui_signal_emit(tst_btn[1], GDK_EXPOSE, TRUE);
        }

        if (var_get_supported_sequrinet())
        {
            g_status = g_org_status;
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)g_sequrinet, g_status);
            nfui_signal_emit(g_sequrinet, GDK_EXPOSE, TRUE);
        }

        if(g_chk_unimo)
		{
			if (unimodata.enable)
			{
				nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)unimo_value[UNIMO_CHK_ROW], unimodata.enable);

				nfui_nflabel_set_text((NFLABEL*)unimo_value[UNIMO_SERVER_ROW], unimodata.server);

				nfui_nflabel_set_number((NFLABEL*)unimo_value[UNIMO_PORT_ROW], unimodata.port);

				for (i=UNIMO_SERVER_ROW; i<NUM_UNIMO_ROWS; i++)
				{
					nfui_nfobject_enable(unimo_value[i]);
					nfui_signal_emit(unimo_value[i], GDK_EXPOSE, TRUE);
				}
				
				nfui_signal_emit(unimo_value[UNIMO_CHK_ROW], GDK_EXPOSE, TRUE);
				nfui_signal_emit(uni_btn, GDK_EXPOSE, TRUE);
			}
			else
			{
				nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)unimo_value[UNIMO_CHK_ROW], unimodata.enable);

				nfui_nflabel_set_text((NFLABEL*)unimo_value[UNIMO_SERVER_ROW], unimodata.server);

				nfui_nflabel_set_number((NFLABEL*)unimo_value[UNIMO_PORT_ROW], unimodata.port);

				for (i=UNIMO_SERVER_ROW; i<NUM_UNIMO_ROWS; i++)
				{
					nfui_nfobject_disable(unimo_value[i]);
					nfui_signal_emit(unimo_value[i], GDK_EXPOSE, TRUE);
				}
				
				nfui_signal_emit(unimo_value[UNIMO_CHK_ROW], GDK_EXPOSE, TRUE);
				nfui_signal_emit(uni_btn, GDK_EXPOSE, TRUE);
			}
		}
    }

    return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, idx;
    gint ddns_changed = 0, sequrinet_changed = 0, unimo_changed = 0;

    if (evt->type == GDK_BUTTON_RELEASE)    {

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        idx = _get_current_cfg_idx();

        g_stpcpy(ddnsdata.server, g_ddnsNet[idx].param.info.server);
        g_stpcpy(ddnsdata.host_name, g_ddnsNet[idx].param.info.hostname);
        g_stpcpy(ddnsdata.id, g_ddnsNet[idx].param.info.id);
        g_stpcpy(ddnsdata.passwd, g_ddnsNet[idx].param.info.pwd);

        if (memcmp(&org_ddnsdata, &ddnsdata, sizeof(DDNSData))) ddns_changed = 1;
        if (g_org_status != g_status) sequrinet_changed = 1;
        if (memcmp(&org_unimodata, &unimodata, sizeof(UNIMOData))) unimo_changed = 1;

		if (ddns_changed) {
			g_memmove(&org_ddnsdata, &ddnsdata, sizeof(DDNSData));			
            DAL_set_ddns_data(&ddnsdata);

			scm_put_log(CHANGE_NET_DDNS, 0, 0);			

			NetInfoMap_update_ddns_addr();

            for (i = 0; i < var_get_ddns_cnt(); i++)
            	memcpy(&g_ddnsNet[i].org_param, &g_ddnsNet[i].param, sizeof(DDNS_PARAM_T));			
		}

		if (sequrinet_changed)
		{
		    if (var_get_supported_sequrinet())
		    {
		        g_org_status = g_status;
		        DAL_set_Sequrinet_Status(g_status);
		        NetInfoMap_update_sequrinet_status();
		    }
		}

        if (unimo_changed)
		{
			g_memmove(&org_unimodata, &unimodata, sizeof(UNIMOData));	
			DAL_set_Unimo_Data(&unimodata);
			nf_ddns_register_unimo_checker(NULL, 0);
			_unimo_status_update();
			nfui_nflabel_set_text((NFLABEL*)unimo_value[UNIMO_STATUS_ROW], g_status_unimo_str);
			nfui_signal_emit(unimo_value[UNIMO_STATUS_ROW], GDK_EXPOSE, TRUE);
		}

		if (ddns_changed || sequrinet_changed || unimo_changed) {
			sysnet_set_changeflag(1);
			nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
        }
    }

    return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE) {

        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        Ddns_tab_out_handler();
        SystemSetupNetwork_Destroy(obj);
    }

    return FALSE;
}

static gboolean _proc_display_msg(void *data)
{
    if (msg_timer_id) g_source_remove(msg_timer_id);
    nftool_mbox(g_curwnd, "NOTICE", "The DVR NAME is newely loaded.", NFTOOL_MB_OK);
    sysnet_set_changeflag(1);

    return FALSE;
}

static int _make_ddns_result_msg(int ddns_ret, char *title, char *msg)
{
    switch (ddns_ret) {
        case DDNS2_RES_OK_GOOD:
        case FUJIKODNS_RES_OK :
            sprintf(title, "NOTICE");
            sprintf(msg, "It has been registered successfully.");
        break;
        case DDNS2_RES_NOTFOUND_ERR:
            sprintf(title, "ERROR");
            sprintf(msg, "It's an invalid URL.");
        break;
        case DDNS2_RES_NOTACCEPT_MSG_ERR:
            sprintf(title, "ERROR");
            sprintf(msg, "It's an invalid owner name.");
        break;
        case DDNS2_RES_HOST_CONFLICT_ERR:
            sprintf(title, "ERROR");
            sprintf(msg, "The host name is conflicted.");
        break;
        case FUJIKODNS_RES_BADAUTH_ERROR:
            sprintf(title, "NOTICE");
            sprintf(msg, "Already Registered");
        break;
        default:
            sprintf(title, "ERROR");
            sprintf(msg, lookup_string("The operation occured an error as a code %d."), ddns_ret);
        break;
    }
    return 0;
}

static int _on_reg_unimo(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	int ret = ((CMM_MESSAGE_T *)data)->param;
	char *alias = ((CMM_MESSAGE_T *)data)->data;
	char title[32];
	char msg[256];

	if (wait_pop) {
		nftool_remove_waitbox(wait_pop);
		wait_pop = NULL;
	}

	_unimo_status_update();

	if (unimodata.enable)
	{
		nfui_nflabel_set_text((NFLABEL*)unimo_value[UNIMO_STATUS_ROW], g_status_unimo_str);
		nfui_signal_emit(unimo_value[UNIMO_STATUS_ROW], GDK_EXPOSE, TRUE);
	}

	if (g_status_unimo == 0) 
	{
		nftool_mbox(g_curwnd, "NOTICE", "It has been registered successfully.", NFTOOL_MB_OK);
	}
	else nftool_mbox(g_curwnd, "NOTICE", "UNIMO CHECKER registration is failed.", NFTOOL_MB_OK);
	  
	return 0;
}

static int _on_reg_ddns(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    int ret = ((CMM_MESSAGE_T *)data)->param;
    char *alias = ((CMM_MESSAGE_T *)data)->data;
    char title[32];
    char msg[256];

    if (wait_pop) {
        nftool_remove_waitbox(wait_pop);
        wait_pop = NULL;
    }

    _make_ddns_result_msg(ret, title, msg);
    nftool_mbox(g_curwnd, title, msg, NFTOOL_MB_OK);
    return 0;
}

static int _on_tst_ddns(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    int ret = ((CMM_MESSAGE_T *)data)->param;

    if (wait_pop) {
        nftool_remove_waitbox(wait_pop);
        wait_pop = NULL;
    }

    if (ret == 0) nftool_mbox(g_curwnd, "NOTICE", "DDNS connection test is success.", NFTOOL_MB_OK);
    else nftool_mbox(g_curwnd, "NOTICE", "DDNS connection test is failed.", NFTOOL_MB_OK);

    return 0;
}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable;
    GdkGC *gc;
    gint x, y;

    switch (evt->type) {
        case GDK_EXPOSE :
        break;

        case GDK_DELETE:
            uxm_unreg_imsg_event(obj, IRET_SCM_REG_DDNS);
            uxm_unreg_imsg_event(obj, IRET_SCM_TST_DDNS);
            uxm_unreg_imsg_event(obj, IRET_SCM_REG_UNIMO);

            if(status_timer_id)
            {
                g_source_remove(status_timer_id);
                status_timer_id = 0;
            }
        break;

        case IRET_SCM_REG_DDNS:
            _on_reg_ddns(obj, evt, data);
        break;

        case IRET_SCM_TST_DDNS:
            _on_tst_ddns(obj, evt, data);
        break;

        case IRET_SCM_REG_UNIMO:
            _on_reg_unimo(obj, evt, data);
        break;

        default :
        break;
    }

    return FALSE;
}

static gboolean pre_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_DELETE)
    {
        g_curwnd = 0;
    }

    return FALSE;
}

static int _make_ddns_status_msg(int ddns_ret, char *msg)
{
    switch (ddns_ret) {
        case 0:
            sprintf(msg, "Initializing");
        break;
        case 1:
            sprintf(msg, "Success");
        break;
        case S1_DDNS_ERR_NETORK:
            sprintf(msg, "Not connected to the network.");
        break;
        case S1_DDNS_ERR_EXTERN_IP:
            sprintf(msg, "Failed to resolve external ip.");
        break;
        case S1_DDNS_ERR_UPDATE:
            sprintf(msg, "Failed to update DDNS.");
        break;
        case S1_DDNS_STOP:
            sprintf(msg, "Stopped");
        break;
        default:
            sprintf(msg, lookup_string("The operation occured an error as a code %d."), ddns_ret);
        break;
    }
    return 0;
}

static gboolean _ddns_status_update(gpointer data)
{
    char msg[512];
    int ddns_ret;
    NFOBJECT *status;

    status = (NFOBJECT *)data;

    if(status)
    {
        ddns_ret = nf_ddns_get_status();
        _make_ddns_status_msg( ddns_ret, msg);

        nfui_nflabel_set_text((NFLABEL*)status, msg);
        nfui_signal_emit(status, GDK_EXPOSE, FALSE);
    }

    return TRUE;
}

void init_NetDDNS_page(NFOBJECT *parent)
{
    NFOBJECT *content_fixed;
    NFOBJECT *ntb;
    NFOBJECT *obj;

    const gchar *strButton1[] = {"CHECK DVR NAME", "DDNS REGISTRATION TEST", "DDNS CONNECTION TEST", "UNIMO REGISTRATION", "UNIMO STATUS UPDATE"};
    const gchar *strButton2[] = {"CANCEL", "APPLY", "CLOSE"};

    const gchar *strTitle[] = {"DDNS", "DDNS SERVER", "NVR NAME", "USER NAME", "PASSWORD", "NVR ADDRESS"};
    const gchar *unimo_strTitle[] = {"UNIMO CHECKER", "UNIMO SERVER", "UNIMO SERVICE PORT", "UNIMO CHECKER STATUS"};
    const gchar *strOffOn[] = {"OFF", "ON"};
	gchar *strAble[] = {"DISABLE", "ENABLE"};

    guint width1[3];
    gint tbl_pos_y = 0;
	gchar strbuf[128] = {0,};

    gint init_ddns = 0;
    gint make_id_row = 0, make_pwd_row = 0;
    gint make_nvr_row = 0, make_mac_row = 0;
    gint make_status_row = 0;
    gint make_row_cnt;

    gint i, cnt;
    gint idx;
    gint size_w, size_h;

    g_curwnd = nfui_nfobject_get_top(parent);

    _init_ddns_info();

    for (i = 0; i < var_get_ddns_cnt(); i++)
    {
        if (!strcmp(g_ddnsNet[i].param.info.server, ddnsdata.server))
            init_ddns = i;

        if (g_ddnsNet[i].cfg.on_id)     make_id_row = 1;
        if (g_ddnsNet[i].cfg.on_pwd)    make_pwd_row = 1;
        if (g_ddnsNet[i].cfg.on_nvr)    make_nvr_row = 1;
        if (g_ddnsNet[i].cfg.on_mac)    make_mac_row = 1;
        if (g_ddnsNet[i].cfg.on_status) make_status_row = 1;
    }

    width1[0] = ND_LABLE_WIDTH;
    width1[1] = ND_CELL_WIDTH;
    width1[2] = 322;
    tbl_pos_y = ND_LABLE_TOP;

    content_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(content_fixed, MENU_V_INNER_W, MENU_V_INNER_H);
    nfui_nfobject_show(content_fixed);
    nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);

	obj = nfui_nfimage_new(IMG_TITLE_BG2);
	nfui_nfimage_set_text((NFIMAGE*)obj, "DDNS SETTINGS");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, 28, tbl_pos_y);


//<-------- MAKE DDNS ON/OFF

    tbl_pos_y += 40 + 20;

    ntb = (NFOBJECT*)nfui_nftable_new(2, 1, ND_COL_SPACE, ND_ROW_SPACE, width1, ND_LABLE_HEIGHT);
    nfui_nfobject_show(ntb);
    nfui_nfobject_modify_bg(ntb, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nffixed_put((NFFIXED*)content_fixed, ntb, ND_LABLE_LEFT, tbl_pos_y);

    obj = nfui_nflabel_new_with_pango_font(strTitle[ND_DDNS_ROW], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb, obj, 0, 0);

    obj = nfui_spinbutton_new(strOffOn, 2, ddnsdata.enable);
    nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_1);
    nfui_regi_post_event_callback(obj, post_ddns_onoff_event_handler);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb, obj, 1, 0);
    value[ND_DDNS_ROW] = obj;

    tbl_pos_y += (ND_LABLE_HEIGHT+ND_ROW_SPACE)*1 + ND_LABLE_HEIGHT;


//<-------- MAKE DDNS SERVER ~ DVR ADDRESS

    cnt = 0;
    make_row_cnt = 1 + make_id_row + make_pwd_row + make_nvr_row + make_mac_row;

    ntb = (NFOBJECT*)nfui_nftable_new(3, make_row_cnt, ND_COL_SPACE, ND_ROW_SPACE, width1, ND_LABLE_HEIGHT);
    nfui_nfobject_show(ntb);
    nfui_nfobject_modify_bg(ntb, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nffixed_put((NFFIXED*)content_fixed, ntb, ND_LABLE_LEFT, tbl_pos_y);


//// DDNS SERVER

    obj = nfui_nflabel_new_with_pango_font(strTitle[ND_DDNS_SERVER_ROW], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb, obj, 0, cnt);

    if (var_get_ddns_cnt() > 1)
    {
        obj = nfui_combobox_new(NULL, 0, 0);
        nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
        nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
        nfui_regi_post_event_callback(obj, post_ddns_server_combo_event_handler);

        for (i = 0; i < var_get_ddns_cnt(); i++)
            nfui_combobox_append_data(NF_COMBOBOX(obj), g_ddnsNet[i].param.info.server);

        nfui_combobox_set_index_no_expose(NF_COMBOBOX(obj), init_ddns);
    }
    else
    {
        if (!strcmp(ddnsdata.server, "s1.co.kr"))
            obj = (NFOBJECT*)nfui_nflabel_new_text_box(ddnsdata.serverip, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
        else
        obj = (NFOBJECT*)nfui_nflabel_new_text_box(ddnsdata.server, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));

        nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
        nfui_nflabel_set_spacing((NFLABEL *)obj, SEMI_CONDENSED_SPACING);
        nfui_nfobject_use_focus(obj, 0);
    }

    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb, obj, 1, cnt++);
    value[ND_DDNS_SERVER_ROW] = obj;


//// DVR NAME

    if (make_nvr_row) {
        obj = nfui_nflabel_new_with_pango_font(strTitle[ND_DDNS_NAME_ROW], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)ntb, obj, 0, cnt);

        obj = (NFOBJECT*)nfui_nflabel_new_text_box(g_ddnsNet[init_ddns].param.info.hostname, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
        nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
        nfui_nflabel_set_spacing((NFLABEL *)obj, SEMI_CONDENSED_SPACING);
        nfui_nfobject_support_multi_lang(obj, FALSE);
        nfui_regi_post_event_callback(obj, post_ddns_field_event_handler);
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)ntb, obj, 1, cnt);
        value[ND_DDNS_NAME_ROW] = obj;

        obj = nftool_normal_button_create_type3(strButton1[NDB_TEST_REGISTER], 322);
        nfui_nfobject_show(obj);
        nfui_regi_post_event_callback(obj, post_register_test_event_handler);
        nfui_nftable_attach((NFTABLE*)ntb, obj, 2, cnt++);
        tst_btn[0] = obj;
    }


//// USER NAME

    if (make_id_row) {
        obj = nfui_nflabel_new_with_pango_font(strTitle[ND_DDNS_ID_ROW], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)ntb, obj, 0, cnt);

        obj = (NFOBJECT*)nfui_nflabel_new_text_box(g_ddnsNet[init_ddns].param.info.id, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
        nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
        nfui_nflabel_set_spacing((NFLABEL *)obj, SEMI_CONDENSED_SPACING);
        nfui_nfobject_support_multi_lang(obj, FALSE);
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)ntb, obj, 1, cnt++);
        nfui_regi_post_event_callback(obj, post_ddns_field_event_handler);
        value[ND_DDNS_ID_ROW] = obj;
    }


//// PASSWORD

    if (make_pwd_row) {
        obj = nfui_nflabel_new_with_pango_font(strTitle[ND_DDNS_PWD_ROW], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)ntb, obj, 0, cnt);

        obj = (NFOBJECT*)nfui_nflabel_new_text_box(g_ddnsNet[init_ddns].param.info.pwd, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
        nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
        nfui_nflabel_set_spacing((NFLABEL *)obj, SEMI_CONDENSED_SPACING);
        nfui_nflabel_set_invisible((NFLABEL*)obj, TRUE);
        nfui_nflabel_use_strip((NFLABEL*)obj, TRUE);
        nfui_nfobject_support_multi_lang(obj, FALSE);
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)ntb, obj, 1, cnt++);
        nfui_regi_post_event_callback(obj, post_ddns_field_event_handler);
        value[ND_DDNS_PWD_ROW] = obj;
    }


//// DVR ADDRESS

    if (make_mac_row) {
        obj = nfui_nflabel_new_with_pango_font(strTitle[ND_DDNS_ADDR_ROW], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)ntb, obj, 0, cnt);

		if(!strcmp(g_ddnsNet[i].param.info.server, "no-ip.com"))
			obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		else
        obj = (NFOBJECT*)nfui_nflabel_new_text_box(g_ddnsNet[init_ddns].param.addr, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
        nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
        nfui_nflabel_set_spacing((NFLABEL *)obj, SEMI_CONDENSED_SPACING);
        nfui_nfobject_support_multi_lang(obj, FALSE);
        nfui_nfobject_use_focus(obj, 0);
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)ntb, obj, 1, cnt);
        value[ND_DDNS_ADDR_ROW] = obj;

        obj = nftool_normal_button_create_type3(strButton1[NDB_TEST_CONNECT], 322);
        nfui_nfobject_show(obj);
        nfui_regi_post_event_callback(obj, post_connection_test_event_handler);
        nfui_nftable_attach((NFTABLE*)ntb, obj, 2, cnt++);
        tst_btn[1] = obj;
    }

    if(make_status_row) {
        char msg[512];
        int ddns_ret;

        tbl_pos_y += (ND_LABLE_HEIGHT+ND_ROW_SPACE)*make_row_cnt;
       
        obj = nfui_nflabel_new_with_pango_font("DDNS STATUS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(obj, ND_LABLE_WIDTH, 40);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)content_fixed, obj, ND_LABLE_LEFT, tbl_pos_y);

        ddns_ret = nf_ddns_get_status();
        _make_ddns_status_msg( ddns_ret, msg);

        obj = (NFOBJECT*)nfui_nflabel_new_text_box(msg, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
        nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
        nfui_nfobject_set_size(obj, ND_CELL_WIDTH + ND_COL_SPACE + 322, 40);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)content_fixed, obj, ND_LABLE_LEFT+ND_LABLE_WIDTH+ND_COL_SPACE, tbl_pos_y);
        value[ND_DDNS_STATUS_ROW] = obj;

        status_timer_id = g_timeout_add(3000, _ddns_status_update, obj);
    }

//// SEQURINET
    nfui_nfobject_get_size(ntb, &size_w, &size_h);
    tbl_pos_y += size_h;

	if (var_get_supported_sequrinet())
    {
	    DAL_get_Sequrinet_Status(&g_status);
	    g_org_status = g_status;
	    
        tbl_pos_y += ND_LABLE_HEIGHT + 20;

		memset(strbuf, 0x00, sizeof(strbuf));
        #if defined(_SEQURINET_STRING_FIX)
            strcpy(strbuf,  "SEQURINET SETTINGS");
        #else
            strcpy(strbuf,  "P2P SETTINGS");
        #endif
        
		obj = nfui_nfimage_new(IMG_TITLE_BG2);
		nfui_nfimage_set_text((NFIMAGE*)obj, "P2P SETTINGS");
		nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
		nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
		nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)content_fixed, obj, 28, tbl_pos_y);

    	tbl_pos_y += 40 + 20;

		memset(strbuf, 0x00, sizeof(strbuf));
        #if defined(_SEQURINET_STRING_FIX)
            strcpy(strbuf, "SEQURINET");
        #else
            if(ivsc.vendor_code != 28)  strcpy(strbuf, "P2P");
            else    strcpy(strbuf, "P2P Cloud Service");
        #endif

    	obj = nfui_nflabel_new_with_pango_font(strbuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(obj, ND_LABLE_WIDTH, 40);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)content_fixed, obj, ND_LABLE_LEFT, tbl_pos_y);

		obj = nfui_spinbutton_new(strAble, 2, g_status);
        nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_1);
        nfui_nfobject_set_size(obj, ND_CELL_WIDTH, 40);
        nfui_nfobject_show(obj);
		nfui_regi_post_event_callback(obj, post_sequrinet_event_handler);
        nfui_nffixed_put((NFFIXED*)content_fixed, obj, ND_LABLE_LEFT+ND_LABLE_WIDTH, tbl_pos_y);
		g_sequrinet = obj;

		tbl_pos_y += 40;
    }

    if (ivsc.vendor_code == 43)
		g_chk_unimo = TRUE;
    
    if (g_chk_unimo)
	{
        _init_unimo_info();

        //<-------- MAKE UNIMO CHECKER
		tbl_pos_y += ND_LABLE_HEIGHT + 20;

		obj = nfui_nfimage_new(IMG_TITLE_BG2);
		nfui_nfimage_set_text((NFIMAGE*)obj, "UNIMO CHECKER SETTINGS");
		nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
		nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, 28, tbl_pos_y);

        //<-------- MAKE UNIMO CHECKER ON/OFF
    
		tbl_pos_y += 40 + 20;

		ntb = (NFOBJECT*)nfui_nftable_new(3, 1, ND_COL_SPACE, ND_ROW_SPACE, width1, ND_LABLE_HEIGHT);
		nfui_nfobject_show(ntb);
		nfui_nfobject_modify_bg(ntb, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
		nfui_nffixed_put((NFFIXED*)content_fixed, ntb, ND_LABLE_LEFT, tbl_pos_y);

		obj = nfui_nflabel_new_with_pango_font(unimo_strTitle[UNIMO_CHK_ROW], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)ntb, obj, 0, 0);

		obj = nfui_spinbutton_new(strOffOn, 2, unimodata.enable);
		nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_1);
		nfui_regi_post_event_callback(obj, post_unimo_chk_onoff_event_handler);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)ntb, obj, 1, 0);
		tbl_pos_y += ND_LABLE_HEIGHT + 20;
		unimo_value[UNIMO_CHK_ROW] = obj;

        //<-------- MAKE UNIMO SERVER ~ UNIMO CHECKER STATUS

		cnt = 0;

		ntb = (NFOBJECT*)nfui_nftable_new(3, 3, ND_COL_SPACE, ND_ROW_SPACE, width1, ND_LABLE_HEIGHT);
		nfui_nfobject_show(ntb);
		nfui_nfobject_modify_bg(ntb, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
		nfui_nffixed_put((NFFIXED*)content_fixed, ntb, ND_LABLE_LEFT, tbl_pos_y);

		tbl_pos_y += ND_LABLE_HEIGHT + 20;

        //// UNIMO SERVER

		obj = nfui_nflabel_new_with_pango_font(unimo_strTitle[UNIMO_SERVER_ROW], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)ntb, obj, 0, cnt);

		obj = (NFOBJECT*)nfui_nflabel_new_text_box(unimodata.server, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
		nfui_nflabel_set_spacing((NFLABEL *)obj, SEMI_CONDENSED_SPACING);
		nfui_nfobject_support_multi_lang(obj, FALSE);
		nfui_regi_post_event_callback(obj, post_unimo_field_event_handler);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)ntb, obj, 1, cnt);
		unimo_value[UNIMO_SERVER_ROW] = obj;

		obj = nftool_normal_button_create_type3(strButton1[UNIMO_CHECK_REGISTER], 322);
		nfui_nfobject_show(obj);
		nfui_regi_post_event_callback(obj, post_unimo_register_event_handler);
		nfui_nftable_attach((NFTABLE*)ntb, obj, 2, cnt++);
		uni_btn = obj;

        //// UNIMO PORT

		obj = nfui_nflabel_new_with_pango_font(unimo_strTitle[UNIMO_PORT_ROW], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)ntb, obj, 0, cnt);

		obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_use_number(obj, TRUE);
		nfui_nflabel_set_number(obj, unimodata.port);
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
		nfui_nfobject_support_multi_lang(obj, FALSE);
		nfui_regi_post_event_callback(obj, post_unimo_field_event_handler);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)ntb, obj, 1, cnt++);
		unimo_value[UNIMO_PORT_ROW] = obj;

        //// UNIMO STATUS

    	_unimo_status_update();

		obj = nfui_nflabel_new_with_pango_font(unimo_strTitle[UNIMO_STATUS_ROW], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)ntb, obj, 0, cnt);

		obj = (NFOBJECT*)nfui_nflabel_new_text_box(g_status_unimo_str, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
		nfui_nflabel_set_spacing((NFLABEL *)obj, SEMI_CONDENSED_SPACING);
		nfui_nfobject_use_focus(obj, 0);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)ntb, obj, 1, cnt);
		unimo_value[UNIMO_STATUS_ROW] = obj;

        if (unimodata.enable)
		{
			nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)unimo_value[UNIMO_CHK_ROW], unimodata.enable);

			nfui_nflabel_set_text((NFLABEL*)unimo_value[UNIMO_SERVER_ROW], unimodata.server);

			nfui_nflabel_set_number((NFLABEL*)unimo_value[UNIMO_PORT_ROW], unimodata.port);

			for (i=UNIMO_SERVER_ROW; i<NUM_UNIMO_ROWS; i++)
			{
				nfui_nfobject_enable(unimo_value[i]);
				nfui_signal_emit(unimo_value[i], GDK_EXPOSE, TRUE);
			}
			
            nfui_nfobject_enable(uni_btn);
			nfui_signal_emit(unimo_value[UNIMO_CHK_ROW], GDK_EXPOSE, TRUE);
			nfui_signal_emit(uni_btn, GDK_EXPOSE, TRUE);
		}
		else
		{
			nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)unimo_value[UNIMO_CHK_ROW], unimodata.enable);

			nfui_nflabel_set_text((NFLABEL*)unimo_value[UNIMO_SERVER_ROW], unimodata.server);

			nfui_nflabel_set_number((NFLABEL*)unimo_value[UNIMO_PORT_ROW], unimodata.port);

			for (i=UNIMO_SERVER_ROW; i<NUM_UNIMO_ROWS; i++)
			{
				nfui_nfobject_disable(unimo_value[i]);
				nfui_signal_emit(unimo_value[i], GDK_EXPOSE, TRUE);
			}
			
            nfui_nfobject_disable(uni_btn);
			nfui_signal_emit(unimo_value[UNIMO_CHK_ROW], GDK_EXPOSE, TRUE);
			nfui_signal_emit(uni_btn, GDK_EXPOSE, TRUE);
		}
    }
//// CANCEL, APPLY, CLOSE BUTTON

    obj = nftool_normal_button_create_type1(strButton2[NDB_CANCEL], MENU_BTN_WIDTH);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R3_X, MENU_V_BTN_Y);
    nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);

    obj = nftool_normal_button_create_type1(strButton2[NDB_APPLY], MENU_BTN_WIDTH);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R2_X, MENU_V_BTN_Y);
    nfui_regi_post_event_callback(obj, post_applybutton_event_handler);

    obj = nftool_normal_button_create_type2(strButton2[NDB_CLOSE], MENU_BTN_WIDTH);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R1_X, MENU_V_BTN_Y);
    nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

    nfui_regi_post_event_callback(parent, post_page_event_handler);

    g_memmove(&org_ddnsdata, &ddnsdata, sizeof(DDNSData));
    g_memmove(&org_unimodata, &unimodata, sizeof(UNIMOData));
    
    nfui_regi_pre_event_callback(content_fixed, pre_page_event_handler);

    idx = _get_current_cfg_idx();

    _sync_ddns_server_field(idx);
    _sync_ddns_host_field(idx);
    _sync_ddns_id_field(idx);
    _sync_ddns_pwd_field(idx);
    _sync_ddns_addr_field(idx);

    uxm_reg_imsg_event(parent, IRET_SCM_REG_DDNS);
    uxm_reg_imsg_event(parent, IRET_SCM_TST_DDNS);
    uxm_reg_imsg_event(parent, IRET_SCM_REG_UNIMO);
}

gboolean Ddns_tab_out_handler()
{
    mb_type ret;
    gint i, idx;
    gint init_ddns;
    gint ddns_changed = 0, sequrinet_changed = 0, unimo_changed = 0;

    idx = _get_current_cfg_idx();

    g_stpcpy(ddnsdata.server, g_ddnsNet[idx].param.info.server);
    g_stpcpy(ddnsdata.host_name, g_ddnsNet[idx].param.info.hostname);
    g_stpcpy(ddnsdata.id, g_ddnsNet[idx].param.info.id);
    g_stpcpy(ddnsdata.passwd, g_ddnsNet[idx].param.info.pwd);

	if (memcmp(&org_ddnsdata, &ddnsdata, sizeof(DDNSData))) ddns_changed = 1;
	if (g_org_status != g_status) sequrinet_changed = 1;
    if (memcmp(&org_unimodata, &unimodata, sizeof(UNIMOData))) unimo_changed = 1;

	if (ddns_changed == 0 && sequrinet_changed == 0 && unimo_changed == 0) return FALSE;

    ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

    if(ret == NFTOOL_MB_OK)
    {
	    if (ddns_changed)
	    {
        g_memmove(&org_ddnsdata, &ddnsdata, sizeof(DDNSData));
        DAL_set_ddns_data(&ddnsdata);

        scm_put_log(CHANGE_NET_DDNS, 0, 0);

    		NetInfoMap_update_ddns_addr();

        for (i = 0; i < var_get_ddns_cnt(); i++)
            	memcpy(&g_ddnsNet[i].org_param, &g_ddnsNet[i].param, sizeof(DDNS_PARAM_T));			
    	}

    	if (sequrinet_changed)
    	{
    	    g_org_status = g_status;
    	    
    	    if (var_get_supported_sequrinet()) {
        	    DAL_set_Sequrinet_Status(g_status);
        	    NetInfoMap_update_sequrinet_status();
    	    }
    	}

        if (unimo_changed)
    	{
			g_memmove(&org_unimodata, &unimodata, sizeof(UNIMOData));
			DAL_set_Unimo_Data(&unimodata);
			nf_ddns_register_unimo_checker(NULL, 0);
			_unimo_status_update();
			nfui_nflabel_set_text((NFLABEL*)unimo_value[UNIMO_STATUS_ROW], g_status_unimo_str);
			nfui_signal_emit(unimo_value[UNIMO_STATUS_ROW], GDK_EXPOSE, TRUE);
    	}

		sysnet_set_changeflag(1);
    }
    else if(ret == NFTOOL_MB_CANCEL)
    {
	    if (ddns_changed)
	    {
        g_memmove(&ddnsdata, &org_ddnsdata, sizeof(DDNSData));

        for (i = 0; i < var_get_ddns_cnt(); i++)
        {
            memcpy(&g_ddnsNet[i].param, &g_ddnsNet[i].org_param, sizeof(DDNS_PARAM_T));

            if (!strcmp(g_ddnsNet[i].param.info.server, ddnsdata.server))
                init_ddns = i;
        }

        nfui_spin_button_set_index_no_expose(value[ND_DDNS_ROW], ddnsdata.enable);

        _sync_ddns_server_field(init_ddns);
        _sync_ddns_host_field(init_ddns);
        _sync_ddns_id_field(init_ddns);
        _sync_ddns_pwd_field(init_ddns);
        _sync_ddns_addr_field(init_ddns);
        }

        if (sequrinet_changed)
        {
            g_status = g_org_status;

            if (var_get_supported_sequrinet())    
                nfui_spin_button_set_index_no_expose(g_sequrinet, g_status);
        }

        if (unimo_changed && g_chk_unimo)
        {
            g_memmove(&unimodata, &org_unimodata, sizeof(UNIMOData));
			nfui_spin_button_set_index_no_expose(value[UNIMO_CHK_ROW], unimodata.enable);
			nfui_nflabel_set_text((NFLABEL*)unimo_value[UNIMO_SERVER_ROW], unimodata.server);
			nfui_nflabel_set_number((NFLABEL*)unimo_value[UNIMO_PORT_ROW], unimodata.port);
        }
    }

    return FALSE;

}
