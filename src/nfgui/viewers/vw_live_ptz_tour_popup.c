
#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_function.h"
#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"
#include "objects/nfcombobox.h"
#include "objects/nfcheckbutton.h"

#include "vw_live_ptz_tour_popup.h"
#include "ix_mem.h"
#include "nfdal.h"
#include "vsm.h"
#include "ssm.h"
#include "smt.h"
#include "scm.h"
#include "vw.h"
#include "vw_internal.h"
#include "uxm.h"

#define PT_LABEL_HEI                    (40)

#define PT_WIN_POS_X(win_w)             ((1920-win_w)/2)
#define PT_WIN_POS_Y(win_h)             ((1080-win_h)/2)

#define PT_OK_BTN_W			    	    (192)
#define PT_OK_BTN_X(win_w)			    (win_w/2-PT_OK_BTN_W-5)

#define PT_CANCEL_BTN_W			        (192)
#define PT_CANCEL_BTN_X(win_w)		    (win_w/2+5)

enum {
	STEP_COLUMN = 0,
	PRESET_COLUMN,
	DWELL_COLUMN,
	COL_COUNT
};

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_win = NULL;

static gint retVal = 0;

static NFOBJECT *g_preset_obj[MAX_TOUR_COUNT];
static NFOBJECT *g_dwell_obj[MAX_TOUR_COUNT];

static PtzPresetData *g_presetData;
static PtzTourData *g_org_tourData;
static PtzTourData g_tourData;

static _init_tour_data()
{
    gint i, j;
    gchar strBuf[32];

// DWELL init
    for (i = 1; i <= 20; i++)
    {
        memset(strBuf, 0x00, sizeof(strBuf));
        g_sprintf(strBuf, g_sec_str[0], i);

        for (j = 0; j < MAX_TOUR_COUNT; j++)
            nfui_combobox_append_data(NF_COMBOBOX(g_dwell_obj[j]), strBuf);
    }
    
    for (j = 0; j < MAX_TOUR_COUNT; j++)   
        nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_dwell_obj[j], g_tourData.dwell[j]-1);

        
// PRESET init
    for (j = 0; j < MAX_TOUR_COUNT; j++)
    {
        nfui_combobox_append_data(NF_COMBOBOX(g_preset_obj[j]), "NONE");

        if (g_tourData.number[j] == 0)
        {
            nfui_combobox_set_index_no_expose(NF_COMBOBOX(g_preset_obj[j]), 0);
            nfui_nfobject_disable(g_dwell_obj[j]);        
        }
    }
    
    for (i = 0; i < g_presetData->cnt; i++)
    {
        memset(strBuf, 0x00, sizeof(strBuf));
        g_sprintf(strBuf, "%d - %s", g_presetData->number[i], g_presetData->name[i]);

        for (j = 0; j < MAX_TOUR_COUNT; j++)
        {
            nfui_combobox_append_data(NF_COMBOBOX(g_preset_obj[j]), strBuf);

            if (g_presetData->number[i] == g_tourData.number[j])
                nfui_combobox_set_index_no_expose(NF_COMBOBOX(g_preset_obj[j]), i+1);
        }
    }

    return 0;
}

static guint _get_preset_num(gint idx)
{
   guint number;

    if (idx == 0)
        number = 0;
    else
        number = g_presetData->number[idx-1];    

    return number;
}

static gboolean _post_preset_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, index;
    
	if (evt->type == NFEVENT_COMBOBOX_CHANGED) 
	{	
        for (i = 0; i < MAX_TOUR_COUNT; i++)
        {
            if (g_preset_obj[i] == obj)
                break;
        }

        index = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));	

        g_tourData.number[i] = _get_preset_num(index);

        if (index) nfui_nfobject_enable(g_dwell_obj[i]);
        else       nfui_nfobject_disable(g_dwell_obj[i]);        
        nfui_signal_emit(g_dwell_obj[i], GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

static gboolean _post_dwell_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, index;
    
	if (evt->type == NFEVENT_COMBOBOX_CHANGED) 
	{	
        for (i = 0; i < MAX_TOUR_COUNT; i++)
        {
            if (g_dwell_obj[i] == obj)
                break;
        }

        index = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));	
        g_tourData.dwell[i] = index+1;
	}

	return FALSE;
}

static gboolean _post_ok_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;

	if(evt->type == GDK_BUTTON_RELEASE) 
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

        if (memcmp(g_org_tourData, &g_tourData, sizeof(PtzTourData)))
        {
            g_memmove(g_org_tourData, &g_tourData, sizeof(PtzTourData));
            retVal = 1;
        }
        else
            retVal = 0;

		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean _post_cancel_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;

	if(evt->type == GDK_BUTTON_RELEASE) 
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

        g_memmove(&g_tourData, g_org_tourData, sizeof(PtzTourData));
        retVal = 0;

		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean post_main_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if (evt->type == GDK_EXPOSE) 
	{
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
    
		nfui_nfobject_gc_unref(gc);
	}
	else if (evt->type == GDK_DELETE) 
	{      
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
	
		gtk_main_quit();
	}

	return FALSE;
}

gint VW_PtzCtrl_Tour_Open(NFWINDOW *parent, PtzPresetData *preset_data, PtzTourData *tour_data)
{
	NFOBJECT *win = NULL;
	NFOBJECT *main_fixed = NULL;
	NFOBJECT *tbl = NULL;
	NFOBJECT *obj = NULL;

	guint tbl_w[COL_COUNT] = {100, 380, 140};
	gint i;
    gint pos_x, pos_y;
    gint win_width = 0, win_height = 0;
    gchar strBuf[8];

    for (i = 0; i < COL_COUNT; i++)
    {
        win_width += tbl_w[i];
        win_width += 1;
    }   

    win_width += 40;
    win_height = 60+(MAX_TOUR_COUNT+1)*PT_LABEL_HEI+MAX_TOUR_COUNT*1+82;

	win = (NFOBJECT*)nfui_nfwindow_new(parent, PT_WIN_POS_X(win_width), PT_WIN_POS_Y(win_height), win_width, win_height);
	g_curwnd = win;

	main_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(main_fixed, win_width, win_height);
	nfui_nfobject_show(main_fixed);
	nfui_regi_post_event_callback(main_fixed, post_main_fixed_event_cb);	

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PTZ TOUR", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, win_width-8, 36);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 24);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 4, 4);

	tbl = (NFOBJECT*)nfui_nftable_new(COL_COUNT, MAX_TOUR_COUNT+1, 2, 1, tbl_w, PT_LABEL_HEI);	
	nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(tbl);
	nfui_nffixed_put((NFFIXED*)main_fixed, tbl, 20, 60);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("STEP", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));	
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, STEP_COLUMN, 0);
	
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PRESET", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));	
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, PRESET_COLUMN, 0);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DWELL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));	
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, DWELL_COLUMN, 0);

    for (i = 0; i < MAX_TOUR_COUNT; i++)
    {
        memset(strBuf, 0x00, sizeof(strBuf));
        g_sprintf(strBuf, "%d", i+1);
    	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));	
    	nfui_nfobject_show(obj);
    	nfui_nftable_attach((NFTABLE*)tbl, obj, STEP_COLUMN, i+1);
        
    	obj = nfui_combobox_new(NULL, 0, 0);
    	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
    	nfui_nfobject_show(obj);
    	nfui_nftable_attach((NFTABLE*)tbl, obj, PRESET_COLUMN, i+1);
    	nfui_regi_post_event_callback(obj, _post_preset_event_handler);	
    	g_preset_obj[i] = obj;

    	obj = nfui_combobox_new(NULL, 0, 0);
    	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
    	nfui_nfobject_show(obj);
    	nfui_nftable_attach((NFTABLE*)tbl, obj, DWELL_COLUMN, i+1);
    	nfui_regi_post_event_callback(obj, _post_dwell_event_handler);	
    	g_dwell_obj[i] = obj;    
    }

	obj = nftool_normal_button_create_type1("OK", PT_OK_BTN_W);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, PT_OK_BTN_X(win_width), win_height-24-44);
	nfui_regi_post_event_callback(obj, _post_ok_button_event_handler);	

	obj = nftool_normal_button_create_type1("CANCEL", PT_CANCEL_BTN_W);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, PT_CANCEL_BTN_X(win_width), win_height-24-44);
	nfui_regi_post_event_callback(obj, _post_cancel_button_event_handler);	

    g_presetData = preset_data;
    g_org_tourData = tour_data;    
    g_memmove(&g_tourData, tour_data, sizeof(PtzTourData));  

    _init_tour_data();

	nfui_nfwindow_add((NFWINDOW*)win, main_fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);
	nfui_make_key_hierarchy((NFWINDOW*)win);

	nfui_page_open(PGID_PTZ_TOUR_POPUP, win, nfui_get_last_user());

    gtk_main();

	nfui_page_close(PGID_PTZ_TOUR_POPUP, win);

    return retVal;    
}

