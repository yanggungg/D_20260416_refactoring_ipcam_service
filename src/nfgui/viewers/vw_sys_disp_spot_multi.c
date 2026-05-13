#include "nf_afx.h"
#include "nfdal.h"

#include "support/event_loop.h"
#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"
#include "support/nf_ui_page_manager.h"

#include "tools/nf_ui_tool.h"
#include "tools/ix_mem.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nftab.h"
#include "objects/nflabel.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfbutton.h"

#include "vw_sys_disp_main.h"

#include "vw_sys_disp_spot_multi.h"

#include "vw_sys_disp_spot_tab1.h"
#include "vw_sys_disp_spot_tab2.h"
#include "vw_sys_disp_spot_tab3.h"
#include "vw_sys_disp_spot_tab4.h"

#include "scm.h"
#include "smt.h"

#include "nf_sysman.h"

#define MAX_PAGE_CNT        (4)
#define PAGE_CNT            (SPOTSD_PORT_CNT + SPOTAUX_PORT_CNT)

enum {
	TYPE_SPOT = 0,
	TYPE_AUX  = 1
};

typedef void (*INIT_FXN)(NFOBJECT*, guint, gint, gint, guint);
typedef gboolean (*CHECK_FXN)();
typedef void (*SAVE_FXN)();
typedef void (*RESTORE_FXN)();

typedef struct _PAGE_CONF
{
    gchar           title[64];
    gint            spot_type;
    gint            port_cnt;
    gint            max_div;
    guint           output_ch;
    gint            data_idx;
    
	INIT_FXN	    init_fxn;
	CHECK_FXN	    check_fxn;
	SAVE_FXN	    save_fxn;
	RESTORE_FXN     restore_fxn;
} PAGE_CONF;

    
static NFWINDOW *g_curwnd = 0;
static PAGE_CONF g_page_conf[MAX_PAGE_CNT];


static gint _init_spot_page()
{
#if defined(_SUPPORT_ORDER_SPOT_AUX)
    gint tab_pos[] = {0, 1, 2, 3};
#else
    gint tab_pos[] = {2, 3, 0, 1};
#endif
    gchar *title[] = {SPOTSD_TAB1_TITLE, SPOTSD_TAB2_TITLE, SPOTAUX_TAB1_TITLE, SPOTAUX_TAB2_TITLE};
    gint spot_type[] = {TYPE_SPOT, TYPE_SPOT, TYPE_AUX, TYPE_AUX};
    gint port_cnt[] = {SPOTSD_TAB1_PORT_CNT, SPOTSD_TAB2_PORT_CNT, SPOTAUX_TAB1_PORT_CNT, SPOTAUX_TAB2_PORT_CNT};    
    gint max_div[] = {SPOTSD_TAB1_MAX_DIV, SPOTSD_TAB2_MAX_DIV, SPOTAUX_TAB1_MAX_DIV, SPOTAUX_TAB2_MAX_DIV};
    guint output_ch[] = {SPOTSD_TAB1_OUTPUT_CH, SPOTSD_TAB2_OUTPUT_CH, SPOTAUX_TAB1_OUTPUT_CH, SPOTAUX_TAB2_OUTPUT_CH};
    gint data_idx[] = {SPOTSD_TAB1_DATA_IDX, SPOTSD_TAB2_DATA_IDX, SPOTAUX_TAB1_DATA_IDX, SPOTAUX_TAB2_DATA_IDX};
    gint i, pos, cnt = 0;

    memset(g_page_conf, 0x00, sizeof(PAGE_CONF)*MAX_PAGE_CNT);

    for (i = 0; i < MAX_PAGE_CNT; i++)
    {
        pos = tab_pos[i];
        
        if (port_cnt[pos] == 0) continue;

        strcpy(g_page_conf[cnt].title, title[pos]);
        g_page_conf[cnt].spot_type = spot_type[pos];
        g_page_conf[cnt].port_cnt = port_cnt[pos];
        g_page_conf[cnt].max_div = max_div[pos];        
        g_page_conf[cnt].output_ch = output_ch[pos];
        g_page_conf[cnt].data_idx = data_idx[pos];

        cnt++;
    }

	g_page_conf[0].init_fxn = init_DispSpot_tab1_page;
	g_page_conf[0].check_fxn = check_DispSpot_tab1_changed;
	g_page_conf[0].save_fxn = save_DispSpot_tab1_data;
	g_page_conf[0].restore_fxn = restore_DispSpot_tab1_data;    
	g_page_conf[1].init_fxn = init_DispSpot_tab2_page;
	g_page_conf[1].check_fxn = check_DispSpot_tab2_changed;
	g_page_conf[1].save_fxn = save_DispSpot_tab2_data;
	g_page_conf[1].restore_fxn = restore_DispSpot_tab2_data;
	g_page_conf[2].init_fxn = init_DispSpot_tab3_page;
	g_page_conf[2].check_fxn = check_DispSpot_tab3_changed;
	g_page_conf[2].save_fxn = save_DispSpot_tab3_data;
	g_page_conf[2].restore_fxn = restore_DispSpot_tab3_data;
	g_page_conf[3].init_fxn = init_DispSpot_tab4_page;
	g_page_conf[3].check_fxn = check_DispSpot_tab4_changed;
	g_page_conf[3].save_fxn = save_DispSpot_tab4_data;
	g_page_conf[3].restore_fxn = restore_DispSpot_tab4_data;

    return 0;
}

static gboolean _proc_escape(void *data)
{
    NFOBJECT *popup = (NFOBJECT *)data;

    nftool_remove_waitbox(popup);
    
    smt_set_service(SMT_SHUTDOWN);
    scm_shutdown_system(IRET_SCM_SHUTDOWN_SPOT_CHANGE);
    scm_notify_to_system("dvr_status", NF_DVR_STATUS_SHUTDOWN);

    nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");

    return FALSE;
}

static gboolean pre_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    switch(evt->type)
    {
        case GDK_EXPOSE:
        {
            GdkGC *gc;
            GdkDrawable *drawable;
            guint x, y;

            drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
            gc = nfui_nfobject_get_gc(obj);

            nfui_nfobject_get_offset(obj, &x, &y);
            nfutil_draw_image(drawable, gc, MK_IMG_MENU_V_SUBTAB_FIXED_BG, x, y, -1, -1, NFALIGN_LEFT, 0);

            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(10));
            gdk_draw_rectangle(drawable, gc, TRUE, x, y, 1, MENU_V_SUBTAB_FIXED_H-60);

            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(11));
            gdk_draw_rectangle(drawable, gc, TRUE, x+1, y, 1, MENU_V_SUBTAB_FIXED_H-60);

            nfui_nfobject_gc_unref(gc);
        }
        break;

        case IRET_SCM_SHUTDOWN_SPOT_CHANGE:
        {
            scm_reboot_system(RR_SPOT_CHANGE, 0);
        }
        break;

        case GDK_DELETE:
        {
            uxm_unreg_imsg_event(g_curwnd, IRET_SCM_SHUTDOWN_SPOT_CHANGE);
            g_curwnd = 0;
           
        }
        break;

        default :
            break;
    }

    return FALSE;
}

static gboolean pre_subtab_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint cur_page;
    gint new_page;
    mb_type ret;

    if(evt->type == NFEVENT_TAB_BEFORE_CHANGE)
    {
        cur_page = nfui_nftab_get_cur_page((NFTAB*)obj);
        new_page = nfui_nftab_get_new_page((NFTAB*)obj);

        if (cur_page == new_page) return FALSE;

        if (g_page_conf[cur_page].check_fxn())
        {
            ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

            if(ret == NFTOOL_MB_OK) 
            {
                g_page_conf[cur_page].save_fxn();
                sysdisp_set_changeflag(1);  
            }       
            else 
            {
                g_page_conf[cur_page].restore_fxn();
            }
        }
    }

    return FALSE;
}

static gboolean pre_subtab_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_EXPOSE) {
        GdkGC *gc;
        GdkDrawable *drawable;
        guint x, y;

        drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
        gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_offset(obj, &x, &y);
        nfutil_draw_image(drawable, gc, MK_IMG_MENU_V_SUBTAB_PAGE_BG, x, y, -1, -1, NFALIGN_LEFT, 0);
        nfui_nfobject_gc_unref(gc);
    }

    return FALSE;
}

void init_DispSpot_multi_page(NFOBJECT *parent)
{
    const gchar *strImage_h[2] =  {
                (MKB_IMG_SUBTAB_DIR_H_N_300), 
                (MKB_IMG_SUBTAB_DIR_H_S_300)
    };

    NFOBJECT *nftab;
    NFOBJECT *tab_page[PAGE_CNT];
    NFOBJECT *obj;

    gchar **strTabTitle = NULL;
    gchar strTemp[16];
    const guint colidx[3] = {COLOR_IDX(189), COLOR_IDX(188), COLOR_IDX(188)};

    guint btn_x, btn_y;
    gint i;
    //gint data_idx[] = {SPOTSD_TAB1_DATA_IDX, SPOTSD_TAB2_DATA_IDX, SPOTAUX_TAB1_DATA_IDX, SPOTAUX_TAB2_DATA_IDX};

    g_curwnd = nfui_nfobject_get_top(parent);

    nfui_nfobject_set_size(parent, (guint)MENU_V_SUBTAB_FIXED_W, (guint)MENU_V_SUBTAB_FIXED_H);
    nfui_nffixed_put((NFFIXED*)(parent->parent), parent, MENU_V_SUBTAB_FIXED_X, MENU_V_SUBTAB_FIXED_Y);
    nfui_regi_pre_event_callback(parent, pre_page_event_handler);

    _init_spot_page();
    
    strTabTitle = (gchar*)imalloc(sizeof(gchar*) * PAGE_CNT);

    for (i = 0; i < PAGE_CNT; i++)
    {       
        strTabTitle[i] = (gchar*)imalloc(sizeof(gchar*) * 64);
        g_stpcpy(strTabTitle[i], g_page_conf[i].title);
    }

    nftab = nfui_nftab_new(PAGE_CNT, (gchar**)strImage_h, 300, 40, NFTAB_DIR_H, strTabTitle, colidx);
    nfui_nftab_set_pango_font((NFTAB*)nftab, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nftab_set_margin(nftab, 10);
    nfui_regi_pre_event_callback(nftab, pre_subtab_event_handler);
    nfui_nfobject_show(nftab);
    nfui_nffixed_put((NFFIXED*)parent, nftab, MENU_V_SUBTAB_PAGE_X, 5);

    for (i = 0; i < PAGE_CNT; i++)
    {    
        obj = nfui_nffixed_new();
        nfui_nfobject_set_size(obj, MENU_V_SUBTAB_PAGE_W, MENU_V_SUBTAB_PAGE_H);
        nfui_nftab_regi_page((NFTAB*)nftab, obj, i);
        nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_PAGE_X, MENU_V_SUBTAB_PAGE_Y);
        nfui_regi_pre_event_callback(obj, pre_subtab_page_event_handler);
        tab_page[i] = obj;
        
        g_page_conf[i].init_fxn(obj, g_page_conf[i].data_idx, g_page_conf[i].port_cnt, g_page_conf[i].max_div, g_page_conf[i].output_ch);        
        //data_idx += g_page_conf[i].port_cnt;
    }

    nfui_nfobject_show(tab_page[0]);
    uxm_reg_imsg_event(g_curwnd, IRET_SCM_SHUTDOWN_SPOT_CHANGE);

    for (i = 0; i < PAGE_CNT; i++)
    {
        ifree(strTabTitle[i]);
    }

    ifree(strTabTitle);
    strTabTitle = NULL;    
}

gboolean Spot_multi_tab_out_handler()
{
    mb_type ret;
    gint i, is_changed = 0;

    for (i = 0; i < PAGE_CNT; i++)
    {
        if (g_page_conf[i].check_fxn()) is_changed = 1;
    }

    if(!is_changed) return FALSE;

    ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

    if (ret == NFTOOL_MB_OK) 
    {
        for (i = 0; i < PAGE_CNT; i++)
        {
            g_page_conf[i].save_fxn();
        }  

        sysdisp_set_changeflag(1);
    }
    else
    {
        for (i = 0; i < PAGE_CNT; i++)
        {
            g_page_conf[i].restore_fxn();
        }  
    }

    return FALSE;
}


