#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfspinbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nfbutton.h"
#include "objects/nfimage.h"

#include "vw_change_color_popup.h"


#define WIN_MIN_WIDTH       (350)

#define CUR_COLOR_W         (60)
#define SEL_COLOR_W         (40)


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_curcolor_obj;
static NFOBJECT *g_selcolor_obj[20];

static VW_COLOR_CONF_T *g_color_conf = 0;
static gint g_select_idx = 0;
static gint g_ret_val = 0;


static gboolean post_sel_color_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	GdkDrawable *drawable;
	GdkGC *gc;
    gint x, y;
    gint i;

    if(event->type == GDK_BUTTON_RELEASE)
	{
        for (i = 0; i < g_color_conf->ncolor; i++)
        {
            if (g_selcolor_obj[i] == obj) break;
        }  

    	nfui_nfobject_modify_bg(g_curcolor_obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(g_color_conf->color_idx[i]));
        nfui_signal_emit(g_curcolor_obj, GDK_EXPOSE, TRUE);

        g_select_idx = i;
	}

	return FALSE;
}

static gboolean post_ok_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;

    if (evt->type == GDK_BUTTON_RELEASE)
    {
        g_color_conf->select = g_select_idx;
        g_ret_val = 1;    

		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);        
    }

    return FALSE;
}

static gboolean post_cancel_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;

    if (evt->type == GDK_BUTTON_RELEASE)
    {
        g_ret_val = 0;        

		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
    }

    return FALSE;
}

static gboolean post_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_DELETE)
    {
		g_curwnd = 0;
		gtk_main_quit();
    }

    return FALSE;
}

static gboolean post_fixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *top;
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if(event->type == GDK_EXPOSE)
	{
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);

		nfui_nfobject_gc_unref(gc);
	} 
	else if(event->type == GDK_DELETE) 
	{
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
 	}
	
	return FALSE;
}

gint vw_change_color_popup_open(NFWINDOW *parent, VW_COLOR_CONF_T *conf, VW_COLOR_SHAPE_T *shape)
{
    NFOBJECT *win;
    NFOBJECT *fixed;
    NFOBJECT *table;
    NFOBJECT *obj;

	guint tbl_w[20] = {0, };

    gint win_x, win_y;
    gint win_size_w, win_size_h;
    gint table_w, table_h;
    gint i;


    if (!conf->ncolor) return -1;    
    if (!shape->cnt_col) return -1;
    if (shape->cnt_col > 20) return -1;
    if (shape->cnt_col > conf->ncolor) return -1;

    g_color_conf = conf;
    g_select_idx = conf->select;

    g_ret_val = 0;

    for (i = 0; i < shape->cnt_col; i++)
    {
        tbl_w[i] = SEL_COLOR_W;
    }       

	table = (NFOBJECT*)nfui_nftable_new(shape->cnt_col, 1 + (conf->ncolor-1)/shape->cnt_col, 10, 10, tbl_w, 40);
	nfui_nfobject_modify_bg(table, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(table);

    table_w = nfui_nftable_get_width((NFTABLE*)table);
    table_h = nfui_nftable_get_height((NFTABLE*)table);


    win_size_w = (table_w+60) < 350 ? 350 : (table_w+60);
    win_size_h = table_h + 220; 

    win_x = shape->x;
    if (shape->x+win_size_w > 1920) win_x = 1920 - win_size_w;
    
    win_y = shape->y;
    if (shape->y+win_size_h > 1080) win_y = 1080 - win_size_h;
        
    win = (NFOBJECT*)nfui_nfwindow_new(parent, win_x, win_y, win_size_w, win_size_h);
    nfui_regi_post_event_callback(win, post_win_event_handler);
    g_curwnd = win;

    fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed, win_size_w, win_size_h);
    nfui_regi_post_event_callback(fixed, post_fixed_event_handler);
    nfui_nfobject_show(fixed);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("COLOR SELECTOR", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 11);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 300, 36);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, 4, 4);    

	obj = (NFOBJECT *)nfui_nflabel_new("");
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(conf->color_idx[g_select_idx]));
	nfui_nfobject_set_size(obj, CUR_COLOR_W, CUR_COLOR_W);
	nfui_nfobject_use_focus(obj, FALSE);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED *)fixed, obj, (win_size_w-CUR_COLOR_W)/2, 60);
	g_curcolor_obj = obj;

    nfui_nffixed_put((NFFIXED*)fixed, table, (win_size_w-table_w)/2, 140);

    for (i = 0; i < conf->ncolor; i++)
    {
    	obj = (NFOBJECT *)nfui_nflabel_new("");
    	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(conf->color_idx[i]));
    	nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)table, obj, i%shape->cnt_col, i/shape->cnt_col);
        nfui_regi_post_event_callback(obj, post_sel_color_event_handler);
    	g_selcolor_obj[i] = obj;
    }       

    obj = nftool_normal_button_create_type1("OK", 150);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, win_size_w/2-155, win_size_h-60);
	nfui_regi_post_event_callback(obj, post_ok_button_event_cb);
	
	obj = nftool_normal_button_create_type1("CANCEL", 150);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, win_size_w/2+5, win_size_h-60);
	nfui_regi_post_event_callback(obj, post_cancel_button_event_cb);

	nfui_nfwindow_add((NFWINDOW*)win, fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);

	nfui_make_key_hierarchy((NFWINDOW*)win);
	nfui_set_key_focus(obj, TRUE);

	nfui_page_open(PGID_POPUPWND, win, nfui_get_last_user());

	gtk_main();

	nfui_page_close(PGID_POPUPWND, win);

    return g_ret_val;
}

