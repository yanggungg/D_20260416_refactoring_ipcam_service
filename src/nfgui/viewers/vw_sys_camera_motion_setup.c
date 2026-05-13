
#include "nf_afx.h"

#include "vw_sys_camera_main.h"
#include "vw_sys_camera_motion_setup.h"
#include "vw_motion_sensor_act.h"
#include "vw_motion_sensor_setup.h"

#include "tools/nf_ui_tool.h"

#include "support/color.h"
#include "support/nf_ui_font.h"
#include "support/event_loop.h"
#include "support/util.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftab.h"

#include "scm.h"
#include "vsm.h"


DECLARE DBG_SYSTEM

#define DBG_LEVEL       9
#define DBG_MODULE      "LOCAL_VW"


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_tab;


static gboolean pre_subtab_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint cur_page;
	gint new_page;
	mb_type ret;

	if(evt->type == NFEVENT_TAB_BEFORE_CHANGE)
	{
		cur_page = nfui_nftab_get_cur_page((NFTAB*)obj);
		new_page = nfui_nftab_get_new_page((NFTAB*)obj);

		if(cur_page == new_page)	return FALSE;

		switch(cur_page) {
			case 0:
			{
                DMSG(1, "check_motsen_act_data_changed");
            
				if(check_motsen_act_data_changed()) {
					ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);
					if(ret == NFTOOL_MB_OK) save_motsen_act_data_changed();
					else 					restore_motsen_act_data_changed();

					if(ret == NFTOOL_MB_OK) syscam_set_changeflag(TRUE);
				}
				MotSen_Show_Preview();
			}
			break;

			case 1:
			{
                DMSG(1, "check_motsen_setup_data_changed");
            
				if (check_motsen_block_cnt() == 0) {
					nfui_nftab_set_new_page((NFTAB*)obj, cur_page);
					return FALSE;
				}
#if 0
				if (check_motsen_detect_object() == 0) {
					nfui_nftab_set_new_page((NFTAB*)obj, cur_page);
					return FALSE;
				}				
#endif
				if(check_motsen_setup_data_changed()) {
					ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);
					if(ret == NFTOOL_MB_OK) save_motsen_setup_data_changed();
					else 					restore_motsen_setup_data_changed();

					if(ret == NFTOOL_MB_OK) syscam_set_changeflag(TRUE);
				}
				MotSen_Pause_Preview();
			}
			break;

			default:
				break;
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


void init_IPCam_MotSen_page(NFOBJECT *parent)
{
    NFOBJECT *nftab;
    NFOBJECT *tab_page[2];

    const gchar *strImage_h[2] =  {
                (MKB_IMG_SUBTAB_DIR_H_N_300), 
                (MKB_IMG_SUBTAB_DIR_H_S_300)
    };

    const gchar *strTabTitle[] = {"ACTIVATION", "AREA SETUP"};
    const guint colidx[3] = {COLOR_IDX(189), COLOR_IDX(188), COLOR_IDX(188)};

    guint btn_x, btn_y;
    guint i;

    g_curwnd = nfui_nfobject_get_top(parent);

    nfui_nfobject_set_size(parent, (guint)MENU_V_SUBTAB_FIXED_W, (guint)MENU_V_SUBTAB_FIXED_H);
    nfui_nffixed_put((NFFIXED*)(parent->parent), parent, MENU_V_SUBTAB_FIXED_X, MENU_V_SUBTAB_FIXED_Y);
    nfui_regi_pre_event_callback(parent, pre_page_event_handler);

    nftab = nfui_nftab_new(2, (gchar**)strImage_h, 300, 40, NFTAB_DIR_H, strTabTitle, colidx);
    nfui_nftab_set_pango_font((NFTAB*)nftab, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nftab_set_margin(nftab, 10);
    nfui_regi_pre_event_callback(nftab, pre_subtab_event_handler);
    nfui_nfobject_show(nftab);
    nfui_nffixed_put((NFFIXED*)parent, nftab, MENU_V_SUBTAB_PAGE_X, 5);
    g_tab = nftab;

    for(i=0; i<2; i++)
    {
        tab_page[i] = nfui_nffixed_new();
        nfui_nfobject_set_size(tab_page[i], MENU_V_SUBTAB_PAGE_W, MENU_V_SUBTAB_PAGE_H);
        nfui_nftab_regi_page((NFTAB*)nftab, tab_page[i], i);
        nfui_nffixed_put((NFFIXED*)parent, tab_page[i], MENU_V_SUBTAB_PAGE_X, MENU_V_SUBTAB_PAGE_Y);

        nfui_regi_pre_event_callback(tab_page[i], pre_subtab_page_event_handler);
    }

    DMSG(1, "init_MotSenAct_Page");
    init_MotSenAct_Page(tab_page[0]);

    DMSG(1, "init_MotSenSetup_Page");
    init_MotSenSetup_Page(tab_page[1]);

    nfui_nfobject_show(tab_page[0]);
}

gint IPCam_MotSen_cur_tab_page()
{
	return nfui_nftab_get_cur_page((NFTAB*)g_tab);
}

gboolean IPCam_MotSen_tab_in_handler()
{

	return FALSE;
}

gboolean IPCam_MotSen_tab_out_handler()
{
	mb_type ret;
	gchar page_mask = 0;

	if(check_motsen_act_data_changed())		page_mask |= (1 << 0);
	if(check_motsen_setup_data_changed())	page_mask |= (1 << 1);

	if(page_mask != 0) 
	{
		ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);
		if(ret == NFTOOL_MB_OK) {
			if(page_mask & (1 << 0))	save_motsen_act_data_changed();
			if(page_mask & (1 << 1))	save_motsen_setup_data_changed();
		}else if(ret == NFTOOL_MB_CANCEL) {
			if(page_mask & (1 << 0))	restore_motsen_act_data_changed();
			if(page_mask & (1 << 1))	restore_motsen_setup_data_changed();
		}

		if(ret == NFTOOL_MB_OK)
			syscam_set_changeflag(1);
	}

	return TRUE;
}


