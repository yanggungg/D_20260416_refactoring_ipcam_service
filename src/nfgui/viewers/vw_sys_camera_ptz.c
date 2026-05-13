
#include "nf_afx.h"

#include "tools/nf_ui_tool.h"
#include "tools/ix_mem.h"

#include "support/util.h"
#include "support/color.h"
#include "support/event_loop.h"

#include "modules/log.h"
#include "services/scm.h"

#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"
#include "objects/nfspinbutton.h"
#include "objects/nfcheckbutton.h"
#include "objects/nflabel.h"
#include "objects/nfbutton.h"
#include "objects/nfimage.h"
#include "objects/nfimglabel.h"

#include "vw_vkeyboard_num.h"

#include "vw_sys_camera_main.h"
#include "vw_sys_camera_ptz.h"

#define PAGE_FIXED_CNT          2
#define ROW_CNT_PER_PAGE        (GUI_CHANNEL_CNT / PAGE_FIXED_CNT)

#define NUM_PS_COLUMNS  5

#define PS_ROW_CH_CNT    GUI_CHANNEL_CNT

#define PS_ROWS         (guint)(GUI_CHANNEL_CNT)
#define PS_COL_SPACE    (guint)(2)
#define PS_ROW_SPACE    (guint)(1)

#define PS_TABLE_LEFT   (8)
#define PS_TABLE_TOP    (39)

#define PS_LABEL_HEIGHT (guint)(40)

#define PS_DATA_DB_CH_MAX      GUI_CHANNEL_CNT


enum {
    PSB_CANCEL = 0,
    PSB_APPLY,
    PSB_CLOSE,
    PSB_BUTTONS
};

enum {
    PTZ_PROTO_PELCO_P = 0,
    PTZ_PROTO_PELCO_D,
    PTZ_PROTO_MESA_DOME,
    PTZ_PROTO_D_MAX,
    PTZ_PROTO_FESTRAXLL_2,
    PTZ_PROTO_GANZ_PT_V3_2,

    NUM_PTZ_PROTOCOLS
};

#if 0
const gchar *strProto[NUM_PTZ_PROTOCOLS] = {
    "PELCO D",
    "PELCO P",
    "MESA-DOME",
    "D-MAX",
    "FESTRAXLL_2",
    "GANZ_PT_V3_2"
};
#else
gchar **strProto = NULL;
gint ptzProto_cnt = 0;
#endif

static PtzData ptzdata[PS_DATA_DB_CH_MAX];
static PtzData org_ptzdata[PS_DATA_DB_CH_MAX];

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *cam_ch[GUI_CHANNEL_CNT];
static NFOBJECT *addr[GUI_CHANNEL_CNT];
static NFOBJECT *proto[GUI_CHANNEL_CNT];
static NFOBJECT *baud[GUI_CHANNEL_CNT];
static NFOBJECT *rs485[GUI_CHANNEL_CNT];
static NFOBJECT *g_page_fixed[PAGE_FIXED_CNT];
static NFOBJECT *g_lb_page_num;

static void enable_ptz_confObj(guint ch, gboolean expose);
static void disable_ptz_confObj(guint ch, gboolean expose);


static void prvSetDataToObjects(PtzData *data)
{
    gint idx;
    guint i;

    for(i=0; i<PS_ROW_CH_CNT; i++)
    {
        idx = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)addr[i]);
        if(idx != data[i].addr) {
            nfui_spin_button_set_index((NFSPINBUTTON*)addr[i], data[i].addr);

            if(data[i].rs485) enable_ptz_confObj(i, TRUE);
            else              disable_ptz_confObj(i, TRUE);
        }

        idx = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)proto[i]);
        if(idx != data[i].proto) {
            nfui_spin_button_set_index((NFSPINBUTTON*)proto[i], data[i].proto);

            if(data[i].rs485) enable_ptz_confObj(i, TRUE);
            else              disable_ptz_confObj(i, TRUE);
        }

        idx = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)baud[i]);
        if(idx != data[i].baud) {
            nfui_spin_button_set_index((NFSPINBUTTON*)baud[i], data[i].baud);

            if(data[i].rs485) enable_ptz_confObj(i, TRUE);
            else              disable_ptz_confObj(i, TRUE);
        }

        idx = nfui_check_button_get_active((NFCHECKBUTTON*)rs485[i]);
        if(idx != data[i].rs485) {
            nfui_check_button_set_active((NFCHECKBUTTON*)rs485[i], data[i].rs485);

            if(data[i].rs485) enable_ptz_confObj(i, TRUE);
            else              disable_ptz_confObj(i, TRUE);
        }
    }
}

static void prvLoadDataFromObjects()
{
    guint i;

    for(i=0; i<PS_ROW_CH_CNT; i++)
    {
        ptzdata[i].addr = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)addr[i]);
        ptzdata[i].proto = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)proto[i]);
        ptzdata[i].baud = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)baud[i]);

        ptzdata[i].rs485 = nfui_check_button_get_active((NFCHECKBUTTON*)rs485[i]);
    }
}

static void prvLoadChDataFromObjects(guint ch)
{
    ptzdata[ch].addr = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)addr[ch]);
    ptzdata[ch].proto = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)proto[ch]);
    ptzdata[ch].baud = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)baud[ch]);

    ptzdata[ch].rs485 = nfui_check_button_get_active((NFCHECKBUTTON*)rs485[ch]);
}

static void enable_ptz_confObj(guint ch, gboolean expose)
{
    nfui_nfobject_enable(addr[ch]);
    nfui_nfobject_enable(proto[ch]);
    nfui_nfobject_enable(baud[ch]);

    if(expose) {
        nfui_signal_emit(addr[ch], GDK_EXPOSE, TRUE);
        nfui_signal_emit(proto[ch], GDK_EXPOSE, TRUE);
        nfui_signal_emit(baud[ch], GDK_EXPOSE, TRUE);
    }
}

static void disable_ptz_confObj(guint ch, gboolean expose)
{
    nfui_nfobject_disable(addr[ch]);
    nfui_nfobject_disable(proto[ch]);
    nfui_nfobject_disable(baud[ch]);

    if(expose) {
        nfui_signal_emit(addr[ch], GDK_EXPOSE, TRUE);
        nfui_signal_emit(proto[ch], GDK_EXPOSE, TRUE);
        nfui_signal_emit(baud[ch], GDK_EXPOSE, TRUE);
    }
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

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    int i;

    switch(evt->type)
    {
        case GDK_DELETE:
            {
                if(strProto != NULL) {
                    g_message(" %s :::::::::::: GDK_DELETE", __FUNCTION__);
                    for (i = 0; i < ptzProto_cnt; ++i) {
                        ifree(strProto[i]);
                    }
                    ifree(strProto); 
                    strProto = NULL;
                }
                g_curwnd = 0;
            }
            break;

        default :
            break;
    }

    return FALSE;

}

static gboolean post_detailbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{

    if(evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top;
        guint ch = 0, idx = 0;

        if(evt->button.button == MOUSE_RIGTH_BUTTON) 
            return FALSE;

        top = nfui_nfobject_get_top(obj);
        ch = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "button_index"));

        prvLoadChDataFromObjects(ch);

//      PtzPropertyDlg_Open(g_curwnd, ch, ptzdata);
    }

    return FALSE;
}

static gboolean post_tourbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{

    if(evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *top;
        guint ch = 0, idx = 0;

        if(evt->button.button == MOUSE_RIGTH_BUTTON) 
            return FALSE;

        top = nfui_nfobject_get_top(obj);
        ch = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "button_index"));

//        PtzTourConf_Open(g_curwnd, ch, ptzdata);
    }

    return FALSE;
}

static gboolean post_rs485_chk_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED) {
        gboolean state;
        guint ch = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "channel"));

        state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);
        if(state) enable_ptz_confObj(ch, TRUE);
        else      disable_ptz_confObj(ch, TRUE);
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
    
        for(i=0; i<PS_ROW_CH_CNT; i++)
        {
            if(org_ptzdata[i].addr != ptzdata[i].addr)  nfui_signal_emit(addr[i], GDK_EXPOSE, TRUE);
            if(org_ptzdata[i].proto != ptzdata[i].proto)nfui_signal_emit(proto[i], GDK_EXPOSE, TRUE);
            if(org_ptzdata[i].baud != ptzdata[i].baud)  nfui_signal_emit(baud[i], GDK_EXPOSE, TRUE);
            if(org_ptzdata[i].rs485 != ptzdata[i].rs485) nfui_signal_emit(rs485[i], GDK_EXPOSE, TRUE);
        }

        g_memmove(ptzdata, org_ptzdata, sizeof(PtzData)*PS_DATA_DB_CH_MAX);

        prvSetDataToObjects(org_ptzdata);
    }

    return FALSE;
}


static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        guint i;

        if(evt->button.button == MOUSE_RIGTH_BUTTON) 
            return FALSE;

        prvLoadDataFromObjects();

        if(memcmp(org_ptzdata, ptzdata, sizeof(PtzData)*PS_DATA_DB_CH_MAX))
        {
            scm_put_log(CHANGE_CAM_PTZ, 0, 0);

            g_memmove(org_ptzdata, ptzdata, sizeof(PtzData)*PS_DATA_DB_CH_MAX);
            DAL_set_ptz_data_all(ptzdata, PS_DATA_DB_CH_MAX);
            DAL_notify_fire_DB_change(NF_SYSDB_CATE_CAM);  

            nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");

            syscam_set_changeflag(1);
        }
    }

    return FALSE;
}


static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) 
            return FALSE;
    
        PtzSetup_tab_out_handler();

        SystemSetupCam_Destroy(obj);
    }

    return FALSE;
}


static gboolean post_addr_spin_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_2BUTTON_PRESS)
    {
        gint numTemp;
        gint x, y;

        if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        nfui_nfobject_get_window_pos(obj, &x, &y);

        x += (gint)((GdkEventButton*)evt)->x;
        y += (gint)((GdkEventButton*)evt)->y;

        numTemp = NumberKey_Open(g_curwnd, nfui_spin_button_get_index((NFSPINBUTTON*)obj), (guint)x, (guint)y, 255);

        if(numTemp>=0) 
            nfui_spin_button_set_index((NFSPINBUTTON*)obj, (guint)numTemp);

        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }
    return FALSE;
}

void init_PtzSetup_page(NFOBJECT *parent)
{
    NFOBJECT *content_fixed;
    NFOBJECT *ntb;
    NFOBJECT *title_object[NUM_PS_COLUMNS];
    NFOBJECT *detail[GUI_CHANNEL_CNT];
    NFOBJECT *tour[GUI_CHANNEL_CNT];
    NFOBJECT *ptzsetup_btns[PSB_BUTTONS];
    NFOBJECT *lbTemp;
    NFOBJECT *fixed_temp;
    NFOBJECT *obj;
    NFOBJECT *main_page_fixed;
    NFOBJECT *page_fixed[PAGE_FIXED_CNT];
    NFOBJECT *page_ntb[PAGE_FIXED_CNT];

    GdkPixbuf *pbCamImage[32];
    const gchar *strTitle[NUM_PS_COLUMNS] = {"CHANNEL", "RS485", "ID", "PROTOCOL", "BAUD RATE"};
    const gchar *strBaud[] = {"2400", "4800", "9600", "19200", "38400", "57600", "115200"};
    gchar *strNone[] = {"NONE"};
    gchar strBuf[STRING_SIZE_CAMTITLE];

    gint size_w, size_h;
    guint width[NUM_PS_COLUMNS];
    guint i, j;
	gint page_num, row_num;
	GdkPixbuf *prev_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *next_img[NFOBJECT_STATE_COUNT];


    g_curwnd = (NFWINDOW*)nfui_nfobject_get_top(parent);

    content_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(content_fixed, MENU_V_INNER_W, MENU_V_INNER_H);
    nfui_nfobject_show(content_fixed);
    nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);


    width[0] = 250;
    width[1] = 121;
    width[2] = 200;
    width[3] = 250;
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


    // table
    ntb = (NFOBJECT*)nfui_nftable_new(NUM_PS_COLUMNS, 1, PS_COL_SPACE, PS_ROW_SPACE, width, PS_LABEL_HEIGHT);
    nfui_nfobject_show(ntb);
    nfui_nffixed_put((NFFIXED*)content_fixed, ntb, PS_TABLE_LEFT, PS_TABLE_TOP);
    

    // table row 0
    for(i=0; i<NUM_PS_COLUMNS; i++)
    {
        title_object[i] = nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(163));
        nfui_nfobject_modify_bg(title_object[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(162));
        nfui_nfobject_use_focus(title_object[i], NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(title_object[i]);
        nfui_nftable_attach((NFTABLE*)ntb, title_object[i], i, 0);
    }
    
    size_w = 0;
    for (i = 0; i < NUM_PS_COLUMNS; i++) {
        size_w += width[i];
    }

    main_page_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(main_page_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(main_page_fixed, size_w, ROW_CNT_PER_PAGE * (40 + 1) + 80);
    nfui_nfobject_show(main_page_fixed);
    nfui_nffixed_put((NFFIXED*)content_fixed, main_page_fixed, PS_TABLE_LEFT, PS_TABLE_TOP+PS_LABEL_HEIGHT+1);

    for (i = 0; i < PAGE_FIXED_CNT; i++)
    {
        g_page_fixed[i] = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_modify_bg(g_page_fixed[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
        nfui_nfobject_set_size(g_page_fixed[i], size_w, ROW_CNT_PER_PAGE * (40 + 1));
        nfui_nffixed_put((NFFIXED*)main_page_fixed, g_page_fixed[i], 0, 0);

        page_ntb[i] = (NFOBJECT*)nfui_nftable_new(NUM_PS_COLUMNS, ROW_CNT_PER_PAGE, 2, 1, width, 40);
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

    // set data
#if 1
    ptzProto_cnt = scm_get_protocol_cnt();
    g_message("======= supported protocol count : %d ", ptzProto_cnt);
#else
    ptzProto_cnt = NUM_PTZ_PROTOCOLS;
#endif
    if(ptzProto_cnt > 0) {
        strProto = imalloc(sizeof(gchar*) * ptzProto_cnt);
        for(i=0; i<ptzProto_cnt; i++) 
            strProto[i] = imalloc(sizeof(gchar) * MAX_PTZ_PROTO_STR_LENGTH);

        for(i=0; i<ptzProto_cnt; i++) {
            if(DAL_get_ptz_protocol_name(strProto[i], i) == 0) {
                // ERROR
                for (j = 0; j < ptzProto_cnt; ++j) {
                    ifree(strProto[j]);
                }
                ifree(strProto);
                strProto = NULL;

                ptzProto_cnt = 0;
                break;
            }
        }
    //  for(i=0; i<ptzProto_cnt; i++) 
    //      g_message("::::::::::::: %s ", strProto[i]);
    }

    memset(ptzdata, 0x00, sizeof(PtzData)*PS_DATA_DB_CH_MAX);   
    memset(org_ptzdata, 0x00, sizeof(PtzData)*PS_DATA_DB_CH_MAX);

    for(i=0; i<PS_DATA_DB_CH_MAX; i++)
    {
        if(DAL_get_ptz_data(&ptzdata[i], i))
            return;
    }

    for(i=0; i<PS_ROW_CH_CNT; i++)
    {
        //DAL_get_camera_title(strBuf, i);
        var_get_camtitle(strBuf, i);

        cam_ch[i] = (NFOBJECT*)nfui_nfimglabel_new(pbCamImage[i], strBuf);
        nfui_nfimglabel_set_align((NFIMGLABEL*)cam_ch[i], NFALIGN_LEFT);
        nfui_nfimglabel_set_pango_font((NFIMGLABEL*)cam_ch[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
        nfui_nfobject_support_multi_lang(cam_ch[i], FALSE);        
        nfui_nfobject_use_focus(cam_ch[i], NFOBJECT_FOCUS_OFF);
        nfui_nfobject_modify_bg(cam_ch[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        nfui_nfobject_show(cam_ch[i]);
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], cam_ch[i], 0, row_num);

        fixed_temp = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_modify_bg(fixed_temp, NFOBJECT_STATE_NORMAL, COLOR_IDX(910));
        //nfui_nfobject_set_size(fixed_temp, action_w[i], 40);
        nfui_nfobject_show(fixed_temp);
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], fixed_temp, 1, row_num);

        obj = (NFOBJECT*)nfui_checkbutton_new(ptzdata[i].rs485);
        nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
        nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (width[1]-size_w-4)/2, (PS_LABEL_HEIGHT-size_h)/2);
        nfui_regi_post_event_callback(obj, post_rs485_chk_event_handler);
        nfui_nfobject_set_data(obj, "channel", GUINT_TO_POINTER(i));
        rs485[i] = obj;

        addr[i] = nfui_spinbutton_new_value_with_range(ptzdata[i].addr, 0, 255, 1);
        nfui_spinbutton_set_skin_type((NFSPINBUTTON*)addr[i], NFSPINBUTTON_TYPE_1);
        nfui_nfobject_show(addr[i]);
        nfui_regi_post_event_callback(addr[i], post_addr_spin_button_event_handler);
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], addr[i], 2, row_num);

        if(ptzProto_cnt > 0) proto[i] = nfui_spinbutton_new((gchar**)strProto, ptzProto_cnt, ptzdata[i].proto);
        else                 proto[i] = nfui_spinbutton_new(strNone, 1, 0);
        nfui_spinbutton_set_skin_type((NFSPINBUTTON*)proto[i], NFSPINBUTTON_TYPE_1);
        nfui_spin_button_set_spacing((NFSPINBUTTON*)proto[i], SEMI_CONDENSED_SPACING);
        nfui_nfobject_support_multi_lang((NFOBJECT*)proto[i], FALSE);
        nfui_nfobject_show(proto[i]);
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], proto[i], 3, row_num);

        baud[i] = nfui_spinbutton_new(strBaud, 7, ptzdata[i].baud);
        nfui_spinbutton_set_skin_type((NFSPINBUTTON*)baud[i], NFSPINBUTTON_TYPE_1);
        nfui_nfobject_show(baud[i]);
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], baud[i], 4, row_num);

        row_num++;

        if (row_num == ROW_CNT_PER_PAGE) {
            row_num = 0;
            page_num++;
        }
    }

    for(i=0; i<PS_ROW_CH_CNT; i++) {
        if(ptzdata[i].rs485) enable_ptz_confObj(i, FALSE);
        else                 disable_ptz_confObj(i, FALSE);
    }



    ptzsetup_btns[0] = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(ptzsetup_btns[0]), NFALIGN_CENTER, 0);   
    nfui_nfobject_show(ptzsetup_btns[0]);
    nfui_nffixed_put((NFFIXED*)parent, ptzsetup_btns[0], MENU_V_BTN_R3_X, MENU_V_BTN_Y);

    ptzsetup_btns[1] = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(ptzsetup_btns[1]), NFALIGN_CENTER, 0);
    nfui_nfobject_show(ptzsetup_btns[1]);
    nfui_nffixed_put((NFFIXED*)parent, ptzsetup_btns[1], MENU_V_BTN_R2_X, MENU_V_BTN_Y);

    ptzsetup_btns[2] = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(ptzsetup_btns[2]), NFALIGN_CENTER, 0);   
    nfui_nfobject_show(ptzsetup_btns[2]);
    nfui_nffixed_put((NFFIXED*)parent, ptzsetup_btns[2], MENU_V_BTN_R1_X, MENU_V_BTN_Y);

    nfui_regi_pre_event_callback(content_fixed, mainbg_event_handler);
    nfui_regi_post_event_callback(ptzsetup_btns[PSB_CANCEL], post_cancelbutton_event_handler);
    nfui_regi_post_event_callback(ptzsetup_btns[PSB_APPLY], post_applybutton_event_handler);
    nfui_regi_post_event_callback(ptzsetup_btns[PSB_CLOSE], post_closebutton_event_handler);

    nfui_regi_post_event_callback(parent, post_page_event_handler);

    uxm_reg_imsg_event(content_fixed, INFY_CAMDB_CHANGE_NOTIFY);
    uxm_monitor_on_imsg_event(content_fixed, INFY_CAMDB_CHANGE_NOTIFY);
    uxm_reg_imsg_event(content_fixed, INFY_USRDB_CHANGE_NOTIFY);
    uxm_monitor_on_imsg_event(content_fixed, INFY_USRDB_CHANGE_NOTIFY);
    
    g_memmove(org_ptzdata, ptzdata, sizeof(PtzData)*PS_DATA_DB_CH_MAX);
}



gboolean PtzSetup_tab_out_handler()
{
    mb_type ret;
    guint i;
    
    prvLoadDataFromObjects();

    if(!memcmp(org_ptzdata, ptzdata, sizeof(PtzData)*PS_DATA_DB_CH_MAX))
        return FALSE;
    
    ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

    if(ret == NFTOOL_MB_OK)
    {
        scm_put_log(CHANGE_CAM_PTZ, 0, 0);

        g_memmove(org_ptzdata, ptzdata, sizeof(PtzData)*PS_DATA_DB_CH_MAX);
        DAL_set_ptz_data_all(ptzdata, PS_DATA_DB_CH_MAX);
        DAL_notify_fire_DB_change(NF_SYSDB_CATE_CAM);  
        
        syscam_set_changeflag(1);
    }
    else if(ret == NFTOOL_MB_CANCEL)
    {
        g_memmove(ptzdata, org_ptzdata, sizeof(PtzData)*PS_DATA_DB_CH_MAX);

        prvSetDataToObjects(org_ptzdata);       
    }

    return FALSE;
}

gboolean PtzSetup_tab_in_handler()
{
    return FALSE;
}

