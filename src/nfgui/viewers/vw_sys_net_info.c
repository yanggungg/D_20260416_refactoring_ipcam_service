
#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftab.h"

#include "vw_sys_net_main.h"
#include "vw_sys_net_info.h"

#include "vw_sys_net_info_map.h"
#include "vw_sys_net_info_detail.h"


#define NET_STAT_TAB_CNT         (2)

static NFWINDOW *g_curwnd = 0;

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
	
		case GDK_DELETE:
			g_curwnd = 0;
		break;

		default : break;
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

static gboolean pre_nftab_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint cur_page;
	gint new_page;

	switch(evt->type)
	{
		case NFEVENT_TAB_BEFORE_CHANGE:
			cur_page = nfui_nftab_get_cur_page((NFTAB*)obj);
			new_page = nfui_nftab_get_new_page((NFTAB*)obj);

			if(cur_page == new_page)	return FALSE;

			if(cur_page == 0)		return NetInfoMap_tab_out_handler();
			if(new_page == 0)		return NetInfoMap_tab_in_handler();
		break;

	default:
		break;
	}
	return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}
	
		NetInfo_tab_out_handler();
		SystemSetupNetwork_Destroy(obj);
	}

	return FALSE;
}


static void _init_net_info(NFOBJECT *parent)
{
	const gchar *strImage_h[2] =  {
				(MKB_IMG_SUBTAB_DIR_H_N_300), 
				(MKB_IMG_SUBTAB_DIR_H_S_300)
	};

	NFOBJECT *nftab;
	NFOBJECT *tab_page[3];
#if defined(_SEQURINET_STRING_FIX)
	const gchar *strTabTitle[] = {"NETWORK MAP", "DETAIL STATUS", "SEQURINET"};
#else
	const gchar *strTabTitle[] = {"NETWORK MAP", "DETAIL STATUS", "P2P"};
#endif
	const guint colidx[3] = {COLOR_IDX(189), COLOR_IDX(188), COLOR_IDX(188)};

	guint btn_x, btn_y;
	guint i;

	g_curwnd = nfui_nfobject_get_top(parent);

	nfui_nfobject_set_size(parent, (guint)MENU_V_SUBTAB_FIXED_W, (guint)MENU_V_SUBTAB_FIXED_H);
	nfui_nffixed_put((NFFIXED*)(parent->parent), parent, MENU_V_SUBTAB_FIXED_X, MENU_V_SUBTAB_FIXED_Y);
	nfui_regi_pre_event_callback(parent, pre_page_event_handler);

	nftab = nfui_nftab_new(NET_STAT_TAB_CNT, (gchar**)strImage_h, 300, 40, NFTAB_DIR_H, strTabTitle, colidx);
	nfui_nftab_set_pango_font((NFTAB*)nftab, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nftab_set_margin(nftab, 10);
	nfui_nfobject_show(nftab);
	nfui_nffixed_put((NFFIXED*)parent, nftab, MENU_V_SUBTAB_PAGE_X, 5);
	nfui_regi_pre_event_callback(nftab, pre_nftab_event_handler);

	for(i=0; i<NET_STAT_TAB_CNT; i++)
	{
		tab_page[i] = nfui_nffixed_new();
		nfui_nfobject_set_size(tab_page[i], MENU_V_SUBTAB_PAGE_W, MENU_V_SUBTAB_PAGE_H);
		nfui_nftab_regi_page((NFTAB*)nftab, tab_page[i], i);
		nfui_nffixed_put((NFFIXED*)parent, tab_page[i], MENU_V_SUBTAB_PAGE_X, MENU_V_SUBTAB_PAGE_Y);
		nfui_regi_pre_event_callback(tab_page[i], pre_subtab_page_event_handler);
	}

	vw_init_NetInfoMap_Page(tab_page[0]);
	vw_init_NetInfoDetail_Page(tab_page[1]);
	//vw_init_NetInfoSequrinet_Page(tab_page[2]);


	nfui_nfobject_show(tab_page[0]);
}

static void _init_net_info_HDI(NFOBJECT *parent)
{
	vw_init_NetInfoMap_Page(parent);
}

void init_NetInfo_page(NFOBJECT *parent)
{
#if defined(_HDI_MODEL_UX) || defined(_DVR_MODEL_UX)
	_init_net_info_HDI(parent);
#else
	_init_net_info(parent);	
#endif	
}

gboolean NetInfo_tab_out_handler() 
{
	NetInfoMap_tab_out_handler();
	return FALSE;
}


gboolean NetInfo_tab_in_handler() 
{
	NetInfoMap_tab_in_handler();
	return FALSE;
}




