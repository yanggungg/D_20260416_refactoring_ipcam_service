/*
 * vw_packet_dump_popup.c
 *
 * Written by Jeongho Yang. <yanggungg@itxsecurity.com>
 * Copyright (c) ITX security, Sep 24, 2015
 *
 */

#include "nf_afx.h" 

#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "modules/cdump.h"

#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfimage.h"
#include "objects/nflabel.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"

#include "vw_packet_dump_popup.h"



//--->STRING
#define STR_WARNING                 "Do not disconnect the storage device during packet dump."

//--->SIZE
#define WND_SIZE_W_PD               (800)
#define WND_SIZE_H_PD               (400)

#define BTN_SIZE_W_PD               (160)
#define BTN_SIZE_H_PD               (50)
#define BTN_GAP_PD                  (4)

//--->POSITION
#define WND_POS_X_PD                ((DISPLAY_ACTIVE_WIDTH - WND_SIZE_W_PD) / 2)
#define WND_POS_Y_PD                ((DISPLAY_ACTIVE_HEIGHT - WND_SIZE_H_PD) / 2)

#define PI_FIXED_POS_X_PD           (12)
#define PI_FIXED_POS_Y_PD           (56)

#define BTN_R_START_POS_X_PD	    (WND_SIZE_W_PD - BTN_SIZE_W_PD)
#define BTN_R1_POS_X_PD				(BTN_R_START_POS_X_PD - BTN_GAP_PD - 12)
#define BTN_R2_POS_X_PD				(BTN_R1_POS_X_PD - BTN_GAP_PD - BTN_SIZE_W_PD)
#define BTN_R3_POS_X_PD				(BTN_R2_POS_X_PD - BTN_GAP_PD - BTN_SIZE_W_PD)
#define BTN_POS_Y_PD				(WND_SIZE_H_PD - 10 - BTN_SIZE_H_PD)


typedef enum _COL_E{
    COL_DEVICE = 0,
    COL_USING,
    COL_STATUS,

    COL_CNT
}COL_E;

typedef enum _BTN_E{
    BTN_REFRESH = 0,
    BTN_START,
    BTN_STOP,
    BTN_CLOSE,

    BTN_CNT
}BTN_E;

static NFOBJECT *g_curwnd;
static NFTABLE *g_table;
static NFOBJECT *g_row2[COL_CNT];
static NFOBJECT *g_btn[BTN_CNT];
static DUMP_STATUS g_dump;
static gint g_timer;

static gint _etc_stop_packet_dump();


static gboolean _etc_display_status_packet_dump(gpointer data)
{
    gchar size[128];

    cdump_packet_dump_get_status(&g_dump);

    sprintf(size, "%lu / %lu (KB)", g_dump.used / 1024, g_dump.size / 1024);

    nfui_nflabel_set_text((NFLABEL*)g_row2[COL_USING], size);
    nfui_signal_emit(g_row2[COL_USING], GDK_EXPOSE, TRUE);
    
    return TRUE;
}

static gint _etc_start_packet_dump()
{
    g_timer = g_timeout_add(1000, _etc_display_status_packet_dump, NULL); 
    
    nfui_nfobject_disable(g_btn[BTN_START]);
    nfui_signal_emit(g_btn[BTN_START], GDK_EXPOSE, TRUE);

    nfui_nfobject_enable(g_btn[BTN_STOP]);
    nfui_signal_emit(g_btn[BTN_STOP], GDK_EXPOSE, TRUE);
    
    nfui_nflabel_set_text((NFLABEL*)g_row2[COL_STATUS], "DUMPING");
    nfui_signal_emit(g_row2[COL_STATUS], GDK_EXPOSE, TRUE);

    return 0;
}

static gint _etc_stop_packet_dump()
{
    if (g_timer)
    {
        g_source_remove(g_timer);
        g_timer = 0;
    }

    nfui_nfobject_enable(g_btn[BTN_START]);
    nfui_signal_emit(g_btn[BTN_START], GDK_EXPOSE, TRUE);

    nfui_nfobject_disable(g_btn[BTN_STOP]);
    nfui_signal_emit(g_btn[BTN_STOP], GDK_EXPOSE, TRUE);

    nfui_nflabel_set_text((NFLABEL*)g_row2[COL_STATUS], "WAITING");
    nfui_signal_emit(g_row2[COL_STATUS], GDK_EXPOSE, TRUE);
    
    return 0;
}

static gboolean post_btn_refresh_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
    }
    
    return FALSE;
}

static gboolean post_btn_start_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
        
        cdump_packet_dump_start();

        _etc_start_packet_dump();
    }
    
    return FALSE;
}

static gboolean post_btn_stop_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        cdump_packet_dump_stop();
        
        _etc_stop_packet_dump();
    }

    return FALSE;
}

static gboolean post_btn_close_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        nfui_nfobject_destroy(g_curwnd);
    }
    
    return FALSE;
}

static gboolean post_fixed_event_handler(NFOBJECT * obj, GdkEvent * evt, gpointer data)
{
    if (evt->type == GDK_DELETE)
    {
        uxm_unreg_imsg_event(obj, INFY_PACKET_DUMP_REMOVE_MEDIA);
    }
    else if (evt->type == INFY_PACKET_DUMP_REMOVE_MEDIA)
    {
        if (g_timer)
        {
            g_source_remove(g_timer);
            g_timer = 0;
        }
        
        nfui_nfobject_destroy(g_curwnd);
    }
    return FALSE;
}

static gboolean post_wnd_event_handler(NFOBJECT * obj, GdkEvent * evt, gpointer data)
{
    if (evt->type == GDK_DELETE)
    {
        g_curwnd = 0;
    }
    
    return FALSE;
}

gint VW_Packet_Dump_Popup(NFWINDOW* parent)
{
    NFOBJECT *wnd = NULL;
    NFOBJECT *fixed = NULL;
    NFOBJECT *table = NULL;
    NFOBJECT *obj = NULL;
    NFOBJECT *btn[BTN_CNT];
    guint pos_x, pos_y;
    gint i;
    gchar buf[128];
    
    guint cell_w[] = {240, 350, 170};
    gchar *col_title[COL_CNT] = {"DEVICE NAME", "STORAGE DEVICE USAGE", "STATUS"};
    gchar *strBtn[BTN_CNT] = {"REFRESH", "START", "STOP", "CLOSE"};


//--->data init
    memset(&g_dump, 0x00, sizeof(DUMP_STATUS));
    cdump_packet_dump_get_status(&g_dump);
    
//--->window
    wnd = nftool_create_popup_window(parent, WND_POS_X_PD, WND_POS_Y_PD, WND_SIZE_W_PD, WND_SIZE_H_PD, "PACKET DUMP", FALSE);
    g_curwnd = wnd;

//--->fixed
    fixed = ((NFWINDOW*)wnd)->child;
    nfui_regi_post_event_callback(fixed, post_fixed_event_handler);

//--->base position
    pos_x = PI_FIXED_POS_X_PD;
    pos_y = PI_FIXED_POS_Y_PD;
    
//--->category label
    pos_x += 4;
    pos_y += 4;
    
    obj = (NFOBJECT *)nfui_nfimage_new(IMG_POPUP_TITLE_BG);
    nfui_nfimage_set_text((NFIMAGE *)obj, "PACKET DUMP");
    nfui_nfimage_set_pango_font((NFIMAGE *)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
    nfui_nfimage_set_font_alignment((NFIMAGE *)obj, NFALIGN_LEFT, 20);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);

//--->table
    pos_y += 60;
    
    table = (NFOBJECT*)nfui_nftable_new(3, 2, 2, 2, cell_w, 45);
    nfui_nftable_set_draw_outline(table, TRUE);
    nfui_nffixed_put((NFFIXED*)fixed, table, pos_x, pos_y);
    nfui_nfobject_show(table);
    g_table = (NFTABLE*)table;

//--->cell contents
    //row1
    for (i = 0; i < COL_CNT; i++)
    {
        obj = (NFLABEL*)nfui_nflabel_new_with_pango_font(col_title[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
        nfui_nfobject_use_focus(obj, FALSE);
        nfui_nfobject_set_size(obj, 260, 50);
        nfui_nftable_attach(table, obj, i, 0);
        nfui_nfobject_show(obj);
    }
    //row2
    for (i = 0; i < COL_CNT; i++)
    {
        obj = (NFLABEL*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, FALSE);
        nfui_nfobject_set_size(obj, 260, 50);
        nfui_nftable_attach(table, obj, i, 1);
        nfui_nfobject_show(obj);
        g_row2[i] = obj;
    }

    memset(buf, 0x00, sizeof(buf));
    cdump_packet_dump_get_usb_title(buf);

    nfui_nflabel_set_text((NFLABEL*)g_row2[COL_DEVICE], buf);
    nfui_nflabel_set_text((NFLABEL*)g_row2[COL_USING], "0 / 0 (KB)");
    nfui_nflabel_set_text((NFLABEL*)g_row2[COL_STATUS], "WAITING");
    
//--->warning label
    pos_y += 105;

    obj = (NFLABEL*)nfui_nflabel_new_with_pango_font(STR_WARNING, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(229));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(obj, FALSE);
    nfui_nflabel_set_spacing((NFLABEL*)obj, SEMI_CONDENSED_SPACING);
    nfui_nfobject_set_size(obj, 780, 40);
    nfui_nffixed_put(fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);

//--->button
    pos_y = BTN_POS_Y_PD;
    
    for (i = 0; i < BTN_CNT; i++)
    {
        btn[i] = nftool_normal_button_create_type1(strBtn[i], 160);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(btn[i]), NFALIGN_CENTER, 0);	
		nfui_nfobject_show(btn[i]);
		g_btn[i] = btn[i];
    }
    nfui_nfobject_hide(btn[BTN_REFRESH]);
    
    if (g_dump.status)
    {
        nfui_nfobject_disable(btn[BTN_START]);
        nfui_nflabel_set_text((NFLABEL*)g_row2[COL_STATUS], "DUMPING");
    }
    else
    {
        nfui_nfobject_disable(btn[BTN_STOP]);
    }
    
    nfui_set_key_focus(btn[BTN_CLOSE], TRUE);
    nfui_nffixed_put((NFFIXED*)fixed, btn[BTN_REFRESH], pos_x, pos_y);
    nfui_nffixed_put((NFFIXED*)fixed, btn[BTN_START], BTN_R3_POS_X_PD, pos_y);
    nfui_nffixed_put((NFFIXED*)fixed, btn[BTN_STOP], BTN_R2_POS_X_PD, pos_y);
    nfui_nffixed_put((NFFIXED*)fixed, btn[BTN_CLOSE], BTN_R1_POS_X_PD, pos_y);

    nfui_regi_post_event_callback(btn[BTN_REFRESH], post_btn_refresh_handler);
    nfui_regi_post_event_callback(btn[BTN_START], post_btn_start_handler);
    nfui_regi_post_event_callback(btn[BTN_STOP], post_btn_stop_handler);
    nfui_regi_post_event_callback(btn[BTN_CLOSE], post_btn_close_handler);

    uxm_reg_imsg_event(fixed, INFY_PACKET_DUMP_REMOVE_MEDIA);

    nfui_nfobject_show(wnd);

    nfui_make_key_hierarchy(wnd);

    return 0;
}

