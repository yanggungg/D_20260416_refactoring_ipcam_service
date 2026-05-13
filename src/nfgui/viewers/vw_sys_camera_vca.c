// vim:ts=4:sw=4
/*******************************************************************************
*  (c) COPYRIGHT 2012 ITX security                                             *
*  ALL RIGHT RESERVED                                                          *
*                                                                              *
*  Jongbin Yim,  jongbina@itxsecurity.com                                      *
********************************************************************************

REVISION HISTORY:

Date       Name           Description
__________ ______________ ______________________________________________________
2012/03/19 Jongbin Yim    Created.

................................................................................
DESCRIPTION:

................................................................................
*/

/**
 * @file  vw_sys_camera_vca.c
 * @brief
 */

/*******************************************************************************
 * includes                                                                    *
 ******************************************************************************/

#include "nf_afx.h"
#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nfspinbutton.h"
#include "objects/nflabel.h"
#include "objects/nfimglabel.h"
#include "objects/nfbutton.h"
#include "objects/nftab.h"
#include "support/nf_ui_color.h"
#include "support/nf_ui_font.h"
#include "support/event_loop.h"
#include "support/util.h"
#include "tools/nf_ui_tool.h"
#include "scm.h"
#include "vsm.h"
#include "vw_sys_camera_main.h"
#include "vw_sys_camera_vca.h"
#include "vw_sys_camera_vca_schd.h"
#include "vw_sys_camera_vca_prop.h"
#include "vw_tools.h"

/*******************************************************************************
 * defines                                                                     *
 ******************************************************************************/

/*******************************************************************************
 * typedefs                                                                    *
 ******************************************************************************/

/*******************************************************************************
 * globals                                                                     *
 ******************************************************************************/

/*******************************************************************************
 * locals                                                                      *
 ******************************************************************************/

static NFWINDOW *g_curwnd = NULL;

static NFOBJECT *tab_page[2];

char g_vca_channel = 0;

char g_draw = 0;

extern char g_mode;

char g_erase = 0;

/*******************************************************************************
 * function declarations                                                       *
 ******************************************************************************/

static gboolean
pre_subtab_page_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	gint x, y;

	if ( evt->type == GDK_EXPOSE ) {
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);
		nfui_nfobject_get_offset(obj, &x, &y);
			nfutil_draw_image(drawable, gc, MK_IMG_MENU_V_SUBTAB_PAGE_BG,
				x, y, -1, -1, NFALIGN_LEFT, 0);
		nfui_nfobject_gc_unref(gc);
		
		if(obj == tab_page[0]){
			if(g_mode ==0)
				g_draw = 1;
			else if(g_mode == 1)
				g_draw = 2;
			//g_draw = TRUE;
			//g_mode = 0;
			g_erase = 0;
		}
		else{
			g_draw = 0;
			g_erase = 1;
		}
		
		//printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@pre_subtab_page_cb %d !!\n ",g_draw);
		
	}
	return FALSE;
}

static gboolean
pre_page_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkGC *gc;
	GdkDrawable *drawable;
	gint x, y;

	switch ( evt->type ) {
		case GDK_EXPOSE:
			drawable = nfui_nfobject_get_window(obj);
			gc = nfui_nfobject_get_gc(obj);

			nfui_nfobject_get_offset(obj, &x, &y);
			nfutil_draw_image(drawable, gc, MK_IMG_MENU_V_SUBTAB_FIXED_BG,
					x, y, -1, -1, NFALIGN_LEFT, 0);
			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(10));
			gdk_draw_rectangle(drawable, gc, TRUE, x, y, 1,
					MENU_V_SUBTAB_FIXED_H - 60);
			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(11));
			gdk_draw_rectangle(drawable, gc, TRUE, x + 1, y, 1,
					MENU_V_SUBTAB_FIXED_H - 60);

			nfui_nfobject_gc_unref(gc);
			break;
		case GDK_DELETE:
			g_curwnd = NULL;
			break;
		default:
			break;
	}
	return FALSE;
}

static gint _start_preview(gint ch)
{
	vsm_live_preview_start(1 << ch, //384,90,_MNVD_LBL_W,_MNVD_LBL_H);
         384,// MENU_V_SUBTAB_FIXED_X + MENU_V_SUBTAB_PAGE_X + MENU_V_SUBTAB_INNER_X, 
         144,// + MENU_V_SUBTAB_FIXED_Y + MENU_V_SUBTAB_PAGE_Y + MENU_V_SUBTAB_INNER_Y, 
        960, 
        540);
    
    return 0;
}

gint VCA_start_preview()
{
	_start_preview(0);
    return 0;
}

static gint _stop_preview()
{
    vsm_live_preview_stop();
    return 0;
}

void
VW_VCACfg_init_page(NFOBJECT *parent)
{
	static gchar *strImage_h[2] = {
			MKB_IMG_SUBTAB_DIR_H_N_300, MKB_IMG_SUBTAB_DIR_H_S_300
	};
	static gchar *strTabTitle[] = {"PROPERTY", "SCHEDULE"};
	static guint tabcolor[3] = {COLOR_IDX(189), COLOR_IDX(188), COLOR_IDX(188)};

	guint i;
	NFOBJECT *tab;

	g_curwnd = (NFWINDOW *)nfui_nfobject_get_top(parent);

	nfui_nfobject_set_size(parent, MENU_V_SUBTAB_FIXED_W,
			MENU_V_SUBTAB_FIXED_H);
	nfui_nffixed_put((NFFIXED *)parent->parent, parent,
			MENU_V_SUBTAB_FIXED_X, MENU_V_SUBTAB_FIXED_Y);
	nfui_regi_pre_event_callback(parent, pre_page_cb);

	tab = (NFOBJECT *)nfui_nftab_new(2, strImage_h, 300, 40, NFTAB_DIR_H,
			strTabTitle, tabcolor);
	nfui_nftab_set_pango_font((NFTAB *)tab,
			nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nftab_set_margin((NFTAB *)tab, 10);
	nfui_nfobject_show(tab);
	nfui_nffixed_put((NFFIXED *)parent, tab, MENU_V_SUBTAB_PAGE_X, 5);

	for (i = 0; i < 2; i ++) {
		tab_page[i] = (NFOBJECT *)nfui_nffixed_new();
		nfui_nfobject_set_size(tab_page[i], MENU_V_SUBTAB_PAGE_W,
				MENU_V_SUBTAB_PAGE_H);
		nfui_nftab_regi_page((NFTAB *)tab, tab_page[i], i);
		nfui_nffixed_put((NFFIXED *)parent, tab_page[i],
				MENU_V_SUBTAB_PAGE_X, MENU_V_SUBTAB_PAGE_Y);
		nfui_regi_pre_event_callback(tab_page[i], pre_subtab_page_cb);
	}

	//captainnn0
	VW_VCACfg_Prop_init_page(tab_page[0]);
	VW_VCACfg_Schd_init_page(tab_page[1]);

	nfui_nfobject_show(tab_page[0]);
}	/* VW_VCACfg_init_page(... */

gboolean
VW_VCACfg_tab_in_handler(void)
{
	g_draw = 1;
	g_mode = 0;
	g_erase= 0;
	_start_preview(g_vca_channel);
	nf_meta_data_display_live_on(g_vca_channel,1);
	return FALSE;
}	/* VW_VCACfg_tab_in_handler(... */

gboolean
VW_VCACfg_tab_out_handler(void)
{
	guint changed_va_rule = 0;
	guint changed_va_schd = 0;
	mb_type ret;

	g_erase = 1;
	g_draw =0;
	g_mode = 0;
		printf("00000000000 VW_VCACfg_tab_out_handler 000000000000p\n");
	
	_stop_preview();
	
	if ( vw_vca_check_prop_data_changed() )
		changed_va_rule = 1;
	if ( vw_vca_check_schd_data_changed() )
		changed_va_schd = 1;
	if ( changed_va_rule == 0 && changed_va_schd ==0)
		return FALSE;

	ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\n"
			"Do you want to save?", NFTOOL_MB_OKCANCEL);

	if ( ret == NFTOOL_MB_OK ) {
		scm_put_log(CHANGE_CAM_VCA, 0, 0);
		if(changed_va_rule)
			vw_vca_save_prop_data();
		if(changed_va_schd)
			vw_vca_save_schd_data();

		syscam_set_changeflag(1);
	}
	else if ( ret == NFTOOL_MB_CANCEL ) {
		/* Restore original data. */
		vw_vca_restore_prop_data(FALSE);
		vw_vca_restore_schd_data(FALSE);
	}

	return FALSE;
}	/* VW_VCACfg_tab_out_handler(... */

