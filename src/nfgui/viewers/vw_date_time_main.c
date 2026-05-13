#include "nf_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "objects/nffixed.h"
#include "objects/nftab.h"

#include "vw_sys_main.h"
#include "vw_date_time_main.h"
#include "vw_date_time.h"
#include "vw_schedule.h"
#include "vw_desc.h"

static NFWINDOW *g_curwnd;
static NFOBJECT *g_nftab;



static void _out_handler_date_time_main(gint page)
{
    if (page == 0)
    {
        VW_DateTime_tab_out_handler();
    }
    else if (page == 1)
    {
        vw_SysSchedule_out_handler();
    }
}

static void _in_handler_date_time_main(gint page)
{
    if (page == 0)
    {

    }
    else if (page == 1)
    {
	vw_SysSchedule_in_handler();
    }   
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

		if(cur_page == new_page) return FALSE;
		

		_out_handler_date_time_main(cur_page);
		_in_handler_date_time_main(new_page);
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

static gboolean pre_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case GDK_EXPOSE :
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

		case GDK_DELETE:
			g_curwnd = 0;
		break;

		default :
			break;
	}

	return FALSE;
}

void init_DateTime_Main_page(NFOBJECT *parent)
{
	NFOBJECT *nftab;
	NFOBJECT *tab_page[2];
	gint i;
	gint menu_cnt;

	const gchar *strTabTitle[] = {"DATE / TIME", "HOLIDAY"};
	const gchar *strImage_h[2] =  {
				(MKB_IMG_SUBTAB_DIR_H_N_300), 
				(MKB_IMG_SUBTAB_DIR_H_S_300)
	};
	const guint colidx[3] = {COLOR_IDX(189), COLOR_IDX(188), COLOR_IDX(188)};


	if (ivsc.dfunc.support_usrdef_holiday) menu_cnt = 2;
	else menu_cnt = 1;

	g_curwnd = nfui_nfobject_get_top(parent);

	nfui_nfobject_set_size(parent, (guint)MENU_V_SUBTAB_FIXED_W, (guint)MENU_V_SUBTAB_FIXED_H);
	nfui_nffixed_put((NFFIXED*)(parent->parent), parent, MENU_V_SUBTAB_FIXED_X, MENU_V_SUBTAB_FIXED_Y);	
	nfui_regi_pre_event_callback(parent, pre_page_event_handler);

	nftab = nfui_nftab_new(menu_cnt, (gchar**)strImage_h, 300, 40, NFTAB_DIR_H, strTabTitle, colidx);
	g_nftab = nftab;
	nfui_nftab_set_pango_font((NFTAB*)nftab, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nftab_set_margin(nftab, 10);
	nfui_regi_pre_event_callback(nftab, pre_subtab_event_handler);	
	nfui_nfobject_show(nftab);
	nfui_nffixed_put((NFFIXED*)parent, nftab, MENU_V_SUBTAB_PAGE_X, 5);

	for(i=0; i<menu_cnt; i++)
	{
		tab_page[i] = nfui_nffixed_new();
		nfui_nfobject_set_size(tab_page[i], MENU_V_SUBTAB_PAGE_W, MENU_V_SUBTAB_PAGE_H);
		nfui_nftab_regi_page((NFTAB*)nftab, tab_page[i], i);
		nfui_nffixed_put((NFFIXED*)parent, tab_page[i], MENU_V_SUBTAB_PAGE_X, MENU_V_SUBTAB_PAGE_Y);
		nfui_regi_pre_event_callback(tab_page[i], pre_subtab_page_event_handler);
	}
	
	VW_Init_SysDateTime_page(tab_page[0]);

	if (ivsc.dfunc.support_usrdef_holiday) {
		vw_Init_SysSchedule_page(tab_page[1]);
	}
	
	nfui_nfobject_show(tab_page[0]);
	
    return;
}

gboolean DateTime_Main_tab_in_handler()
{
	gint cur_page;	
	cur_page = nfui_nftab_get_cur_page((NFTAB*)g_nftab);

	if( cur_page == 0 )
	{
	}
	else if( cur_page == 1 )
	{
	}
	
	return FALSE;
}

gboolean DateTime_Main_tab_out_handler()
{
	gint cur_page;	
	cur_page = nfui_nftab_get_cur_page((NFTAB*)g_nftab);	

	if( cur_page == 0 )
	{
		VW_DateTime_tab_out_handler();
	}
	else if (cur_page == 1)
	{
		vw_SysSchedule_out_handler();
	}
    
	return FALSE;
}

