
#include "nf_afx.h"

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

#include "vw_sys_camera_main.h"
#include "vw_sys_camera_privacy.h"
#include "vw_sys_camera_privacy_act.h"
#include "vw_sys_camera_privacy_area.h"


DECLARE DBG_SYSTEM

#define DBG_LEVEL       0
#define DBG_MODULE      "LOCAL_VW"


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_tab;

static PrivacyData g_org_privacy_data[GUI_CHANNEL_CNT];
static PrivacyData g_privacy_data[GUI_CHANNEL_CNT];


static gboolean pre_subtab_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint cur_page;
    gint new_page;
    mb_type ret;

    if(evt->type == NFEVENT_TAB_BEFORE_CHANGE)
    {
        cur_page = nfui_nftab_get_cur_page((NFTAB*)obj);
        new_page = nfui_nftab_get_new_page((NFTAB*)obj);

        if(cur_page == new_page)    return FALSE;

        switch(cur_page) 
        {
            case 0:
            {
                DMSG(1, "PrivacyMask_Act_tab_out_handler");
            
                PrivacyMask_Act_tab_out_handler();    
            }
            break;

            case 1:
            {
                DMSG(1, "PrivacyMask_Area_tab_out_handler");
                
                PrivacyMask_Area_tab_out_handler();
            }       
            break;

            default:
            break;
        }

        switch(new_page) 
        {
            case 0:
            {
                DMSG(1, "PrivacyMask_Act_tab_in_handler");
            
                PrivacyMask_Act_tab_in_handler();    
            }
            break;

            case 1:
            {
                DMSG(1, "PrivacyMask_Area_tab_in_handler");
                
                PrivacyMask_Area_tab_in_handler();
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


void init_PrivacyMask_page(NFOBJECT *parent)
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

    memset(g_privacy_data, 0x00, sizeof(PrivacyData)*GUI_CHANNEL_CNT);
    memset(g_org_privacy_data, 0x00, sizeof(PrivacyData)*GUI_CHANNEL_CNT);

    for (i=0; i<GUI_CHANNEL_CNT; i++)
    {
        DAL_get_privacy_data(&g_privacy_data[i], i);
    }

    g_memmove(g_org_privacy_data, g_privacy_data, sizeof(PrivacyData)*GUI_CHANNEL_CNT);

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

    _set_data_PrivacyMask_Act(g_privacy_data);
    _set_data_PrivacyMask_Area(g_privacy_data);

    DMSG(1, "init_PrivacyMask_Act_Page");
    init_PrivacyMask_Act_Page(tab_page[0]);

    DMSG(1, "init_PrivacyMask_Area_Page");
    init_PrivacyMask_Area_Page(tab_page[1]);

    nfui_nfobject_show(tab_page[0]);
}

gint PrivacyMask_get_cur_tab_page()
{
	return nfui_nftab_get_cur_page((NFTAB*)g_tab);
}

gboolean PrivacyMask_tab_in_handler()
{
    if (PrivacyMask_get_cur_tab_page() == 0)
        PrivacyMask_Act_tab_in_handler();    
    else if (PrivacyMask_get_cur_tab_page() == 1)
        PrivacyMask_Area_tab_in_handler();

    return FALSE;
}


gboolean PrivacyMask_tab_out_handler()
{
    if (PrivacyMask_get_cur_tab_page() == 0)
        PrivacyMask_Act_tab_out_handler();    
    else if (PrivacyMask_get_cur_tab_page() == 1)
        PrivacyMask_Area_tab_out_handler();      

    return FALSE;
}

gint _is_changed_data_PrivacyMask()
{
    if (memcmp(g_org_privacy_data, g_privacy_data, sizeof(PrivacyData)*GUI_CHANNEL_CNT))
        return 1;

    return 0;
}


gint _save_data_PrivacyMask()
{
	if (memcmp(g_org_privacy_data, g_privacy_data, sizeof(PrivacyData)*GUI_CHANNEL_CNT)) 
	{	
		g_memmove(g_org_privacy_data, g_privacy_data, sizeof(PrivacyData)*GUI_CHANNEL_CNT);
		
    	DAL_set_privacy_data_all(g_privacy_data, GUI_CHANNEL_CNT);
    	scm_put_log(CHANGE_PRIVACY_MASK, 0, 0);		

		nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
	}

    return 0;
}


gint _restore_data_PrivacyMask()
{
	if (memcmp(g_org_privacy_data, g_privacy_data, sizeof(PrivacyData)*GUI_CHANNEL_CNT)) 
	{
		g_memmove(g_privacy_data, g_org_privacy_data, sizeof(PrivacyData)*GUI_CHANNEL_CNT);
	}

    return 0;
}


