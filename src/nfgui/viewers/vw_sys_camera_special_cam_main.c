#include "nf_afx.h"

#include "tools/nf_ui_tool.h"
#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/util.h"
#include "support/color.h"

#include "objects/nftab.h"
#include "objects/nfobject.h"
#include "objects/nfbutton.h"
#include "objects/nffixed.h"
#include "objects/nflabel.h"
#include "objects/nfimglabel.h"
#include "objects/nfcombobox.h"

#include "nf_api_live.h"
#include "nf_api_ipcam.h"
#include "nf_api_cam.h"

#include "scm.h"
#include "vsm.h"
#include "ix_mem.h"

#include "vw_sys_camera_special_cam_main.h"


#define CAM_SETUP_TITLE_X               (MENU_V_SUBTAB_PAGE_X)
#define CAM_SETUP_TITLE_Y               (2)
#define CAM_SETUP_TITLE_WIDTH           (240)
#define CAM_SETUP_TITLE_HEIGHT          (40)

enum {
	SPECIAL_CAM_TAB_FISHEYE = 0,
	SPECIAL_CAM_TAB_MAX
};

static gchar *strCh[GUI_CHANNEL_CNT];

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_ch_obj = 0;
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

        if(cur_page == new_page)    return FALSE;

        switch(cur_page) 
        {
            case 0:
            {
                VW_SpecialCam_FishEye_tab_out_handler();
            }
            break;

            case 1:
            {
            }       
            break;

            default:
            break;
        }

        switch(new_page) 
        {
            case 0:
            {
            }
            break;

            case 1:
            {
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

void VW_init_Special_Cam_Setup_page(NFOBJECT *parent)
{
	const gchar *strImage_h[2] =  {
				(MKB_IMG_SUBTAB_DIR_H_N_250),
				(MKB_IMG_SUBTAB_DIR_H_S_250)
	};

	NFOBJECT *nftab;
	NFOBJECT *tab_page[SPECIAL_CAM_TAB_MAX];
	NFOBJECT *obj;

	const gchar *strTabTitle[SPECIAL_CAM_TAB_MAX] = {"FISH EYE"};
	const guint colidx[3] = {COLOR_IDX(189), COLOR_IDX(188), COLOR_IDX(188)};
	guint i, j;
	guint size_w, size_h;

	g_curwnd = nfui_nfobject_get_top(parent);

	nfui_nfobject_set_size(parent, (guint)MENU_V_SUBTAB_FIXED_W, (guint)MENU_V_SUBTAB_FIXED_H);
	nfui_nffixed_put((NFFIXED*)(parent->parent), parent, MENU_V_SUBTAB_FIXED_X, MENU_V_SUBTAB_FIXED_Y);
	nfui_regi_pre_event_callback(parent, pre_page_event_handler);

	nftab = nfui_nftab_new(SPECIAL_CAM_TAB_MAX, (gchar**)strImage_h, 250, 40, NFTAB_DIR_H, strTabTitle, colidx);
	nfui_nftab_set_pango_font((NFTAB*)nftab, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nftab_set_margin(nftab, 10);
	nfui_nfobject_show(nftab);
	nfui_nffixed_put((NFFIXED*)parent, nftab, MENU_V_SUBTAB_PAGE_X, 5);
	nfui_regi_pre_event_callback(nftab, pre_subtab_event_handler);
	g_tab = nftab;

	for(i = 0; i < SPECIAL_CAM_TAB_MAX; i++)
	{
		tab_page[i] = nfui_nffixed_new();
		nfui_nfobject_set_size(tab_page[i], MENU_V_SUBTAB_PAGE_W, MENU_V_SUBTAB_PAGE_H);
		nfui_nftab_regi_page((NFTAB*)nftab, tab_page[i], i);
		nfui_nffixed_put((NFFIXED*)parent, tab_page[i], MENU_V_SUBTAB_PAGE_X, MENU_V_SUBTAB_PAGE_Y);
		nfui_regi_pre_event_callback(tab_page[i], pre_subtab_page_event_handler);
	}

	VW_init_SpecialCam_FishEye_Page(tab_page[SPECIAL_CAM_TAB_FISHEYE]);

	nfui_nfobject_show(tab_page[0]);
}

void VW_Special_Cam_Tab_Out_Handler()
{
    VW_SpecialCam_FishEye_tab_out_handler();
}

