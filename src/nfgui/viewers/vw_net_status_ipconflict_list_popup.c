#include <string.h>

#include "nf_afx.h"

#include "services/vsm.h"

#include "support/nf_ui_image.h"
#include "support/nf_ui_font.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "support/color.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nfimglabel.h"
#include "objects/nfimage.h"
#include "objects/nflabel.h"
#include "objects/nftile.h"
#include "objects/nfbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nflistbox.h"

#include "tools/nf_ui_tool.h"

#include "scm.h"
#include "ssm.h"
#include "dtf.h"
#include "var.h"
#include "ix_mem.h"

#include "vw_net_status_ipconflict_list_popup.h"

#define LISTBOX_POPUP_WID	    	(625)
#define LISTBOX_POPUP_HEI    	    (325)
#define LISTBOX_POPUP_POS_X			(guint)((DISPLAY_ACTIVE_WIDTH- LISTBOX_POPUP_WID)/2)
#define LISTBOX_POPUP_POS_Y 		(guint)((DISPLAY_ACTIVE_HEIGHT - LISTBOX_POPUP_HEI)/2 - 10)

enum {
    IPCONFLICT_TYPE =0,
    IPCONFLICT_TIME,
    IPCONFLICT_MAC,
    NUM_IPCONFLICT
};

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_ipconflict_obj =0;

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_DELETE)
    {
        g_curwnd = 0;
        gtk_main_quit();
    }

    return FALSE;
}

static gboolean post_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable =NULL;
    GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

    if(evt->type == GDK_EXPOSE)
    {
        drawable = nfui_nfobject_get_window(obj);
        gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);

        nfui_nfobject_gc_unref(gc);     
    }
    else if (evt->type == GDK_DELETE)
	{
    		nfui_nfobject_get_size(obj, &size_w, &size_h);
    		nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
	}
        
    return FALSE;
}

static gboolean post_main_bg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable;
    GdkGC *gc;

    if(evt->type == GDK_EXPOSE)
    {
        drawable = nfui_nfobject_get_window(obj);
        gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_gc_unref(gc);     
    }

    return FALSE;
}

static gboolean update_ipconflict_list()
{
    gchar *strBuf[NUM_IPCONFLICT];
    gint i;

    CONFLICT_INFOR conflict_info;

    memset(&conflict_info, 0x00, sizeof(CONFLICT_INFOR));

    var_get_update_ipconflict_list(&conflict_info);

    nfui_listbox_delete_all((NFLISTBOX*)g_ipconflict_obj);
    
    if(conflict_info.cnt > 19) conflict_info.cnt = 19;

    for(i =0; i< conflict_info.cnt+1; i++)
    {
        strBuf[IPCONFLICT_TYPE] = imalloc(sizeof(gchar)*64);
        strcpy(strBuf[IPCONFLICT_TYPE], conflict_info.conflict_type[i]);
    
        strBuf[IPCONFLICT_TIME] = imalloc(sizeof(gchar)*64);
        strcpy(strBuf[IPCONFLICT_TIME], conflict_info.conflict_time[i]);

        strBuf[IPCONFLICT_MAC] = imalloc(sizeof(gchar*)*64);
        strcpy(strBuf[IPCONFLICT_MAC], conflict_info.conflict_mac_addr[i]);

        nfui_listbox_set_text((NFLISTBOX*)g_ipconflict_obj, strBuf);
        
        ifree(strBuf[IPCONFLICT_TYPE]);
        ifree(strBuf[IPCONFLICT_TIME]);
        ifree(strBuf[IPCONFLICT_MAC]);

        nfui_signal_emit(g_ipconflict_obj, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_refresh_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
       update_ipconflict_list();
    }

    return FALSE;
}

static gboolean post_okbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *topwin;
        if(evt->button.button == MOUSE_RIGTH_BUTTON)
        {
            return FALSE;
        }

        topwin = nfui_nfobject_get_top(obj);

        nfui_nfobject_destroy(topwin);
    }
    
    return FALSE;    
}

void VW_Open_ipconflict_listbox_Popup(NFWINDOW *parent)
{
    NFOBJECT *main_wnd, *main_fixed;
    NFOBJECT *obj;
    guint pos_x, pos_y;
    gint i;

    gchar *tbl_maclist_str[] = {"TYPE", "TIME", "MAC ADDRESS"};
    guint ipconflict_w[NUM_IPCONFLICT] = {80, 250, 250};
    gint fg_color[NFOBJECT_STATE_COUNT];
    gint bg_color[NFOBJECT_STATE_COUNT];
    
    gchar strBuf[32];

    gint size_w, size_h;
    
    main_wnd = (NFOBJECT*)nfui_nfwindow_new(parent, LISTBOX_POPUP_POS_X, LISTBOX_POPUP_POS_Y, LISTBOX_POPUP_WID, LISTBOX_POPUP_HEI);
    g_curwnd = main_wnd;
    nfui_regi_post_event_callback(main_wnd, post_main_win_event_handler);
    nfui_nfobject_modify_bg(main_wnd, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));

    main_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(main_fixed, LISTBOX_POPUP_WID, LISTBOX_POPUP_HEI);
    nfui_regi_post_event_callback(main_fixed, post_fixed_event_cb);
    nfui_nfobject_show(main_fixed);

    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("IP CONFLICT LIST", nffont_get_pango_font(NFFONT_MEDIUM_NORMAL), COLOR_IDX(205));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);    
    nfui_nfobject_set_size(obj, LISTBOX_POPUP_WID-8, 36);
    nfui_nfobject_use_focus(obj,NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj,4, 4);

    pos_x = 10;
    pos_y = 44;  

    for(i=0; i<NUM_IPCONFLICT; i++)
    {
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(tbl_maclist_str[i], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(116));
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(372));
        nfui_nflabel_set_align((NFLABEL*)obj,NFALIGN_CENTER, 10);
        nfui_nfobject_set_size(obj, ipconflict_w[i] -1, 36);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

        pos_x = pos_x + ipconflict_w[i];
    }
    
    fg_color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(389);
    fg_color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(251);
    fg_color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(251);
    fg_color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(389);

    bg_color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(200);
    bg_color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(250);
    bg_color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(250);
    bg_color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(200);  

    pos_x = 10;

    nfui_get_image_size(IMG_N_SCROLL_UP, &size_w, &size_h);
    
    obj = nfui_listbox_new(NUM_IPCONFLICT, ipconflict_w, 30);
    nfui_listbox_set_skin_type(NF_LISTBOX(obj), NFLISTBOX_TYPE_POPUP_1);
    nfui_listbox_set_pango_font(NF_LISTBOX(obj), nffont_get_pango_font(NFFONT_SMALL_SEMI));
    nfui_listbox_set_fg_color(NF_LISTBOX(obj), fg_color);
    nfui_listbox_set_bg_color(NF_LISTBOX(obj), bg_color);
    nfui_listbox_set_column_align(NF_LISTBOX(obj), IPCONFLICT_TYPE, NFALIGN_CENTER);
    nfui_listbox_set_column_align(NF_LISTBOX(obj), IPCONFLICT_TIME, NFALIGN_CENTER);
    nfui_listbox_set_column_align(NF_LISTBOX(obj), IPCONFLICT_MAC, NFALIGN_CENTER);
    nfui_listbox_support_multi_lang(NF_LISTBOX(obj), FALSE);
    nfui_listbox_set_draw_inline(NF_LISTBOX(obj), TRUE, COLOR_IDX(392));
    nfui_nfobject_set_size(obj, ipconflict_w[0]+ipconflict_w[1]+ipconflict_w[2]+size_w, 30*6);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_ON);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y+40);
    g_ipconflict_obj = obj;

    obj = nftool_normal_button_create_type1("REFRESH", 150);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, LISTBOX_POPUP_WID-10-150-5-150, LISTBOX_POPUP_HEI-10-40);
    nfui_regi_post_event_callback(obj, post_refresh_event_handler);

	obj = nftool_normal_button_create_type1("CLOSE", 150);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, LISTBOX_POPUP_WID-10-150, LISTBOX_POPUP_HEI-10-40);
	nfui_regi_post_event_callback(obj, post_okbutton_event_handler);

	update_ipconflict_list();    
	
    nfui_nfwindow_add((NFWINDOW*)main_wnd, main_fixed);
    nfui_run_main_event_handler(main_wnd);
	nfui_nfobject_show(main_wnd);
	nfui_make_key_hierarchy((NFWINDOW*)main_wnd);

	gtk_main();     
}

