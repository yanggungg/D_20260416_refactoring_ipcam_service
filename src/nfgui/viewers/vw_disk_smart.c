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
#include "objects/nfbutton.h"

#include "vw_disk_main.h"
#include "vw_disk_smart.h"
#include "vw_disk_internal_smart.h"
#include "vw_disk_external_smart.h"



static NFWINDOW *g_curwnd = 0;


static gboolean pre_subtab_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint cur;
	gint new;
	mb_type ret;

	if(evt->type == NFEVENT_TAB_BEFORE_CHANGE) 
	{
		cur = nfui_nftab_get_cur_page((NFTAB*)obj);
		new = nfui_nftab_get_new_page((NFTAB*)obj);

		if(cur == new) return FALSE;

		if(cur == 0) {
			if(check_int_smart_data_changed()) {
				ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);
				if(ret == NFTOOL_MB_OK) save_int_smart_data();
				else					restore_int_smart_data();
			}
		}

		if(new == 1) 
			reset_check_interval();
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

void VW_Init_DiskSmart_Page(NFOBJECT *parent)
{
	NFOBJECT *nftab;
	NFOBJECT *tab_page[2];
	NFOBJECT *obj;

	const gchar *strTabTitle[] = {"INTERNAL DISKS", "EXTERNAL STORAGE"};
	const gchar *strTabHImg[2] =  {
				(MKB_IMG_SUBTAB_DIR_H_N_300), 
				(MKB_IMG_SUBTAB_DIR_H_S_300)
	};
	const guint colidx[3] = {COLOR_IDX(189), COLOR_IDX(188), COLOR_IDX(188)};
	
	gint i, tab_cnt;


	g_curwnd = nfui_nfobject_get_top(parent);

	nfui_nfobject_set_size(parent, (guint)MENU_V_SUBTAB_FIXED_W, (guint)MENU_V_SUBTAB_FIXED_H);
	nfui_nffixed_put((NFFIXED*)(parent->parent), parent, MENU_V_SUBTAB_FIXED_X, MENU_V_SUBTAB_FIXED_Y);
	nfui_regi_pre_event_callback(parent, pre_page_event_handler);

    if (var_get_supported_ext_disk())   tab_cnt = 2;
    else                                tab_cnt = 1;

	nftab = nfui_nftab_new(tab_cnt, (gchar**)strTabHImg, 300, 40, NFTAB_DIR_H, strTabTitle, colidx);
	nfui_nftab_set_pango_font((NFTAB*)nftab, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nftab_set_margin(nftab, 10);
	nfui_regi_pre_event_callback(nftab, pre_subtab_event_handler);
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

	VW_Init_DiskInternal_Smart_Page(tab_page[0]);
	if (var_get_supported_ext_disk())
    	VW_Init_DiskExternal_Smart_Page(tab_page[1]);

	nfui_nfobject_show(tab_page[0]);
}

gboolean VW_DiskSmart_tab_out_handler()
{
	mb_type ret;
	gchar page_mask = 0;

	if(check_int_smart_data_changed())		page_mask |= (1 << 0);
	if (var_get_supported_ext_disk())
    	if(check_ext_smart_data_changed())		page_mask |= (1 << 1);

	if(page_mask != 0) {
		ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);
		if(ret == NFTOOL_MB_OK) {
			if(page_mask & (1 << 0))	save_int_smart_data();
			if (var_get_supported_ext_disk())
    			if(page_mask & (1 << 1))	save_ext_smart_data();
		}else if(ret == NFTOOL_MB_CANCEL) {
			if(page_mask & (1 << 0))	restore_int_smart_data();
			if (var_get_supported_ext_disk())
    			if(page_mask & (1 << 1))	restore_ext_smart_data();
		}

		if(ret == NFTOOL_MB_OK)
			disk_data_changed(TRUE);
	}
	return FALSE;
}

gboolean display_disk_smart(gboolean expose)
{
	display_internal_disk_smart(expose);
	if (var_get_supported_ext_disk())
    	display_external_disk_smart(expose);

	return TRUE;
}
