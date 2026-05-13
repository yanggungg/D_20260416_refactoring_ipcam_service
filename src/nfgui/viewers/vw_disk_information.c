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

#include "services/scm.h"

#include "vw_disk_main.h"
#include "vw_disk_information.h"
#include "vw_disk_internal_information.h"
#include "vw_disk_external_information.h"

#include "dtf.h"


static NFWINDOW *g_curwnd = 0;


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

void VW_Init_DiskInfo_Page(NFOBJECT *parent)
{
	NFOBJECT *nftab;
	NFOBJECT *tab_page[2];
	gint i, tab_cnt;

	const gchar *strTabTitle[] = {"INTERNAL DISKS", "EXTERNAL STORAGE"};
	const gchar *strTabHImg[2] =  {
		(MKB_IMG_SUBTAB_DIR_H_N_300), 
		(MKB_IMG_SUBTAB_DIR_H_S_300)
	};
	const guint colidx[3] = {COLOR_IDX(189), COLOR_IDX(188), COLOR_IDX(188)};

	g_curwnd = nfui_nfobject_get_top(parent);

	nfui_nfobject_set_size(parent, (guint)MENU_V_SUBTAB_FIXED_W, (guint)MENU_V_SUBTAB_FIXED_H);
	nfui_nffixed_put((NFFIXED*)(parent->parent), parent, MENU_V_SUBTAB_FIXED_X, MENU_V_SUBTAB_FIXED_Y);
	nfui_regi_pre_event_callback(parent, pre_page_event_handler);

    if (var_get_supported_ext_disk())   tab_cnt = 2;
    else                                tab_cnt = 1;

	nftab = nfui_nftab_new(tab_cnt, (gchar**)strTabHImg, 300, 40, NFTAB_DIR_H, strTabTitle, colidx);
	nfui_nftab_set_pango_font((NFTAB*)nftab, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nftab_set_margin(nftab, 10);
	nfui_nfobject_show(nftab);
	nfui_nffixed_put((NFFIXED*)parent, nftab, MENU_V_SUBTAB_PAGE_X, 5);

	for(i = 0; i < tab_cnt; i++)
	{
		tab_page[i] = nfui_nffixed_new();
		nfui_nfobject_set_size(tab_page[i], MENU_V_SUBTAB_PAGE_W, MENU_V_SUBTAB_PAGE_H);
		nfui_nftab_regi_page((NFTAB*)nftab, tab_page[i], i);
		nfui_nffixed_put((NFFIXED*)parent, tab_page[i], MENU_V_SUBTAB_PAGE_X, MENU_V_SUBTAB_PAGE_Y);
		nfui_regi_pre_event_callback(tab_page[i], pre_subtab_page_event_handler);

	}

	VW_Init_DiskInternal_Info(tab_page[0]);
    if (var_get_supported_ext_disk())
    	VW_Init_DiskExternal_Info(tab_page[1]);

	nfui_nfobject_show(tab_page[0]);
}

gboolean VW_DiskInfo_tab_out_handler()
{
	return FALSE;
}

gboolean display_disk_inforamtion(gboolean expose)
{
	display_internal_disk_information(expose);
    if (var_get_supported_ext_disk())
    	display_external_disk_information(expose);

	return TRUE;
}
