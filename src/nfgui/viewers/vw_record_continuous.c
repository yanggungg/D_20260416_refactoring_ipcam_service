#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftab.h"

#include "vw_record_main.h"
#include "vw_record_data_internal.h"
#include "vw_record_continuous.h"
#include "vw_record_continuous_param.h"
#include "vw_record_continuous_sched.h"


#define REC_CONT_TAB_WIDTH				(300)
#define REC_CONT_TAB_HEIGHT				(40)

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

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
		  return FALSE;
	       }
	
		VW_RecContinuous_tab_out_handler();
		VW_RecordSetup_Destroy(obj);
	}

	return FALSE;
}


//////////////////////////////////////////////////////////////////////
//
//
//


void VW_Init_RecContinuous_Page(NFOBJECT *parent)
{
	const gchar *strImage_h[2] =  {
				(MKB_IMG_SUBTAB_DIR_H_N_300), 
				(MKB_IMG_SUBTAB_DIR_H_S_300)
	};


	NFOBJECT *nftab;
	NFOBJECT *tab_page[2];

	const gchar *strTabTitle[] = {"SIZE/FPS/QUALITY", "SCHEDULE"};
	const guint colidx[3] = {COLOR_IDX(189), COLOR_IDX(188), COLOR_IDX(188)};

	guint btn_x, btn_y;
	guint i;


	g_curwnd = nfui_nfobject_get_top(parent);

	nfui_nfobject_set_size(parent, (guint)MENU_V_SUBTAB_FIXED_W, (guint)MENU_V_SUBTAB_FIXED_H);
	nfui_nffixed_put((NFFIXED*)(parent->parent), parent, MENU_V_SUBTAB_FIXED_X, MENU_V_SUBTAB_FIXED_Y);
	nfui_regi_pre_event_callback(parent, pre_page_event_handler);

	nftab = nfui_nftab_new(2, (gchar**)strImage_h, REC_CONT_TAB_WIDTH, REC_CONT_TAB_HEIGHT, NFTAB_DIR_H, strTabTitle, colidx);
	nfui_nftab_set_pango_font((NFTAB*)nftab, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nftab_set_margin(nftab, 10);
	nfui_nfobject_show(nftab);
	nfui_nffixed_put((NFFIXED*)parent, nftab, MENU_V_SUBTAB_PAGE_X, 5);

	for(i=0; i<2; i++)
	{
		tab_page[i] = nfui_nffixed_new();
		nfui_nfobject_set_size(tab_page[i], MENU_V_SUBTAB_PAGE_W, MENU_V_SUBTAB_PAGE_H);
		nfui_nftab_regi_page((NFTAB*)nftab, tab_page[i], i);
		nfui_nffixed_put((NFFIXED*)parent, tab_page[i], MENU_V_SUBTAB_PAGE_X, MENU_V_SUBTAB_PAGE_Y);
		nfui_regi_pre_event_callback(tab_page[i], pre_subtab_page_event_handler);
	}

	VW_Init_RecContParam_Page(tab_page[0]);
	VW_Init_RecContSched_Page(tab_page[1]);

	nfui_nfobject_show(tab_page[0]);
}

gboolean VW_RecContinuous_tab_out_handler()
{
	mb_type ret;
	ChangedDataType type = REC_DATA_NOT_CHANGED;
	guint i,j;

	if((type = is_changed_continuous_data())) {
		ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

		if(ret == NFTOOL_MB_OK)
		{
			set_record_continuous_db(type);
			vw_send_notify_change_record_data();
		}
		else
		{
			get_record_continuous_db(VW_RecContParam_get_sched_day(), 0);
			VW_RecContParam_data_reloaded(TRUE);
		}
	}

	return FALSE;
}




