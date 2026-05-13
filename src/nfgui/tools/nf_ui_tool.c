
/*************************************************************
 *
 *	Setup Window Related tools...
 * 	Create Setup Window, Decorate Setup window, etc...
 *
 * **********************************************************/

#include "nf_afx.h"

#include "nf_ui_tool.h"

#include "../support/nf_ui_font.h"
#include "../support/nf_ui_image.h"
#include "../support/nf_ui_page_manager.h"
#include "../support/util.h"
#include "../support/event_loop.h"
#include "../support/color.h"
#include "../support/color_conf.h"

#include "../viewers/objects/nfimage.h"
#include "../viewers/objects/nfobject.h"
#include "../viewers/objects/nfwindow.h"
#include "../viewers/objects/nffixed.h"
#include "../viewers/objects/nftab.h"
#include "../viewers/objects/nfbutton.h"
#include "../viewers/objects/nfprogressbar.h"
#include "../viewers/objects/nflabel.h"
#include "../viewers/objects/nfcheckbutton.h"

#include "nf_notify.h"
#include "nf_api_eventlog.h"
#include "iux_afx.h"
#include "scm.h"
#include "uxm.h"
#include "ssm.h"
#include "vw.h"

#include "vw_menu.h"

#define	POPUP_BORDER_SIZE				(DISPLAY_IS_D1 ? 2:4)
#define	POPUP_TITLE_HEIGHT				(DISPLAY_IS_D1 ? 22:56)
#define	POPUP_TITLE_FONT				(nffont_get_pango_font(NFFONT_LARGE_SEMI))
#define	POPUP_LABEL_HEIGHT				(guint)(DISPLAY_IS_D1 ? 16:22)

#define	MENU_DIR_H_TAB_X				(330)
#define	MENU_DIR_H_TAB_Y				(19)
#define	MENU_DIR_H_TAB_W				(260)
#define	MENU_DIR_H_TAB_H		       	(58)

#define	MENU_DIR_A_TAB_X				(487)
#define	MENU_DIR_A_TAB_Y				(19)
#define	MENU_DIR_A_TAB_W				(340)
#define	MENU_DIR_A_TAB_H		       	(58)

#define	MENU_DIR_V_TAB_X				(12)
#define	MENU_DIR_V_TAB_Y				(82)
#define	MENU_DIR_V_TAB_W				(344)
#define	MENU_DIR_V_TAB_H		       	(80)

#define	SETUP_TITLE_ICON_X				(18)
#define	SETUP_TITLE_ICON_Y				(7)

#define	SETUP_TITLE_FONT	    		(nffont_get_pango_font(NFFONT_XLARGE_SEMI_2))
#define	SETUP_TAB_MARGIN	    		(guint)(DISPLAY_IS_D1 ? 10:26)
#define	TAB_FONT_SIZE		    		(DISPLAY_IS_D1 ? NFFONT_LARGE_SEMI:NFFONT_MEDIUM_SEMI)
#define	SETUP_TAB_FONT	    			(nffont_get_pango_font(TAB_FONT_SIZE))

#define SETUP_FIXED_WIDTH				(guint)(DISPLAY_IS_D1 ? 592:1180)
#define SETUP_FIXED_HEIGHT				(guint)(DISPLAY_IS_D1 ? 418:832)

#define	MMENU_BUTTON_COUNT			 	(8)
#define SUB_BTN_MAX					 	(5)

#define UP		(-1)
#define DOWN	(1)


enum {
	SETUP_IMAGE_TITLE = 0,
	SETUP_IMAGE_TOP,
	SETUP_IMAGE_END
};
// OTM
typedef void (*func)(NFOBJECT *, gint);

const func sub_menu_func[] =
{
// skshin
#if 0
	SystemSetupCam_Open, SystemSetupDisp_Open, SystemSetupSound_Open, SystemSetupSystem_Open,
	SystemSetupUser_Open, 	SystemSetupNetwork_Open, SystemSetupEvtSen_Open , SystemSetupDisk_Open
#endif
};

static gint pre_idx = 0; // sub menu - mouse / key  pre foucs postion
static gint focus_idx = 0;
static gint child_num[MMENU_BUTTON_COUNT];
static NFCURSORMENU menu[MMENU_BUTTON_COUNT][SUB_BTN_MAX] = {{NULL,0,0,0,0,0,NULL},};

static gint rm_mbox_sec = 0;
static guint tid = 0;
static gboolean remove_mbox(gpointer data);

static gchar update_msg[256];
static gint g_waiting_timer = 0;
static NFOBJECT *g_wait_obj = 0;

static void prvSendSetupNoti(gint in)
{
	if(in)	nf_notify_fire_params("gui_setup", 1, 0, 0, 0);
	else	nf_notify_fire_params("gui_setup", 0, 0, 0, 0);

//	g_printf("SEND NOTIFY... GUI SETUP MODE IN/OUT...  IS_IN?? [%d]\n", in);
}

static gboolean nffixed_event_cb(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	if(evt->type == GDK_EXPOSE)
	{
		GdkGC *gc;

		GdkDrawable *drawable;
		nfsetup_window_type type;

		NFOBJECT *nflabel = NULL;
        GdkPixbuf *pbuf = NULL;

		drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
		type = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "setup_window_type"));

		gc = gdk_gc_new(drawable);

        if (type == NFSETUP_WINDOW_USER_GUIDE)
        {
            pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG_NO_LINE, 1920, 1080);
            nfutil_draw_pixbuf(drawable, gc, pbuf, 0, 0, -1, -1, NFALIGN_LEFT, 0);
       		nfutil_draw_image(drawable, gc, POPUP_MENU_LINE, 2, 44, -1, -1, NFALIGN_LEFT, 0);
        }
        else
        {
			nfutil_draw_image(drawable, gc, MK_IMG_MENU_BG, 0, 0, -1, -1, NFALIGN_LEFT, 0);
        }

		if (type == NFSETUP_WINDOW_CAMERA)
		{
        	nfutil_draw_image(drawable, gc, MENU_V_PAGE_LEFT_BG, 12, 77, -1, -1, NFALIGN_LEFT, 0);
		    nfutil_draw_image(drawable, gc, IMG_SUB_PAGE_ICON_CAMERA, 18, 7, -1, -1, NFALIGN_LEFT, 0);
			nflabel = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CAMERA", nffont_get_pango_font(NFFONT_XLARGE_NORMAL_1), COLOR_IDX(30));

			if (get_ui_type() != 200)
			{
				gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(12));
				gdk_draw_rectangle(drawable, gc, TRUE, 7, 72, 1920-14, 2);
			}
		}
		else if (type == NFSETUP_WINDOW_DISPLAY)
		{
        	nfutil_draw_image(drawable, gc, MENU_V_PAGE_LEFT_BG, 12, 77, -1, -1, NFALIGN_LEFT, 0);
		    nfutil_draw_image(drawable, gc, IMG_SUB_PAGE_ICON_DISPLAY, 18, 7, -1, -1, NFALIGN_LEFT, 0);
			nflabel = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DISPLAY", nffont_get_pango_font(NFFONT_XLARGE_NORMAL_1), COLOR_IDX(30));

			if (get_ui_type() != 200)
			{
				gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(13));
				gdk_draw_rectangle(drawable, gc, TRUE, 7, 72, 1920-14, 2);
			}
		}
		else if (type == NFSETUP_WINDOW_SOUND)
		{
        	nfutil_draw_image(drawable, gc, MENU_V_PAGE_LEFT_BG, 12, 77, -1, -1, NFALIGN_LEFT, 0);
		    nfutil_draw_image(drawable, gc, IMG_SUB_PAGE_ICON_AUDIO, 18, 7, -1, -1, NFALIGN_LEFT, 0);
			nflabel = (NFOBJECT*)nfui_nflabel_new_with_pango_font("AUDIO", nffont_get_pango_font(NFFONT_XLARGE_NORMAL_1), COLOR_IDX(30));

			if (get_ui_type() != 200)
			{
				gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(14));
				gdk_draw_rectangle(drawable, gc, TRUE, 7, 72, 1920-14, 2);
			}
		}
		else if (type == NFSETUP_WINDOW_SYSTEM)
		{
        	nfutil_draw_image(drawable, gc, MENU_V_PAGE_LEFT_BG, 12, 77, -1, -1, NFALIGN_LEFT, 0);
		    nfutil_draw_image(drawable, gc, IMG_SUB_PAGE_ICON_SYSTEM, 18, 7, -1, -1, NFALIGN_LEFT, 0);
			nflabel = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SYSTEM", nffont_get_pango_font(NFFONT_XLARGE_NORMAL_1), COLOR_IDX(30));

			if (get_ui_type() != 200)
			{
				gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(17));
				gdk_draw_rectangle(drawable, gc, TRUE, 7, 72, 1920-14, 2);
			}
		}
		else if (type == NFSETUP_WINDOW_USER)
		{
        	nfutil_draw_image(drawable, gc, MENU_V_PAGE_LEFT_BG, 12, 77, -1, -1, NFALIGN_LEFT, 0);
		    nfutil_draw_image(drawable, gc, IMG_SUB_PAGE_ICON_USER, 18, 7, -1, -1, NFALIGN_LEFT, 0);
			nflabel = (NFOBJECT*)nfui_nflabel_new_with_pango_font("USER", nffont_get_pango_font(NFFONT_XLARGE_NORMAL_1), COLOR_IDX(30));

			if (get_ui_type() != 200)
			{
				gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(15));
				gdk_draw_rectangle(drawable, gc, TRUE, 7, 72, 1920-14, 2);
			}
		}
		else if (type == NFSETUP_WINDOW_NETWORK)
		{
        	nfutil_draw_image(drawable, gc, MENU_V_PAGE_LEFT_BG, 12, 77, -1, -1, NFALIGN_LEFT, 0);
		    nfutil_draw_image(drawable, gc, IMG_SUB_PAGE_ICON_NETWORK, 18, 7, -1, -1, NFALIGN_LEFT, 0);
			nflabel = (NFOBJECT*)nfui_nflabel_new_with_pango_font("NETWORK", nffont_get_pango_font(NFFONT_XLARGE_NORMAL_1), COLOR_IDX(30));

			if (get_ui_type() != 200)
			{
				gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(16));
				gdk_draw_rectangle(drawable, gc, TRUE, 7, 72, 1920-14, 2);
			}
		}
		else if (type == NFSETUP_WINDOW_EVENT)
		{
        	nfutil_draw_image(drawable, gc, MENU_V_PAGE_LEFT_BG, 12, 77, -1, -1, NFALIGN_LEFT, 0);
		    nfutil_draw_image(drawable, gc, IMG_SUB_PAGE_ICON_EVENT, 18, 7, -1, -1, NFALIGN_LEFT, 0);
			nflabel = (NFOBJECT*)nfui_nflabel_new_with_pango_font("EVENT", nffont_get_pango_font(NFFONT_XLARGE_NORMAL_1), COLOR_IDX(30));

			if (get_ui_type() != 200)
			{
				gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(19));
				gdk_draw_rectangle(drawable, gc, TRUE, 7, 72, 1920-14, 2);
			}
		}
		else if (type == NFSETUP_WINDOW_DISK)
		{
        	nfutil_draw_image(drawable, gc, MENU_V_PAGE_LEFT_BG, 12, 77, -1, -1, NFALIGN_LEFT, 0);
		    nfutil_draw_image(drawable, gc, IMG_SUB_PAGE_ICON_STORAGE, 18, 7, -1, -1, NFALIGN_LEFT, 0);
			nflabel = (NFOBJECT*)nfui_nflabel_new_with_pango_font("STORAGE", nffont_get_pango_font(NFFONT_XLARGE_NORMAL_1), COLOR_IDX(30));

			if (get_ui_type() != 200)
			{
				gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(18));
				gdk_draw_rectangle(drawable, gc, TRUE, 7, 72, 1920-14, 2);
			}
		}
		else if (type == NFSETUP_WINDOW_RECORDING)
		{
        	nfutil_draw_image(drawable, gc, MENU_V_PAGE_LEFT_BG, 12, 77, -1, -1, NFALIGN_LEFT, 0);
		    nfutil_draw_image(drawable, gc, IMG_SUB_PAGE_ICON_RECORD, 18, 7, -1, -1, NFALIGN_LEFT, 0);
			nflabel = (NFOBJECT*)nfui_nflabel_new_with_pango_font("RECORDING", nffont_get_pango_font(NFFONT_XLARGE_NORMAL_1), COLOR_IDX(30));

			if (get_ui_type() != 200)
			{
				gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(22));
				gdk_draw_rectangle(drawable, gc, TRUE, 7, 72, 1920-14, 2);
			}
		}
		else if (type == NFSETUP_WINDOW_ARCHIVING)
		{
		    nfutil_draw_image(drawable, gc, IMG_SUB_PAGE_ICON_ARCHIVE, 18, 7, -1, -1, NFALIGN_LEFT, 0);
			nflabel = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ARCHIVING", nffont_get_pango_font(NFFONT_XLARGE_NORMAL_1), COLOR_IDX(30));
		}
		else if (type == NFSETUP_WINDOW_SEARCH)
		{
		    nfutil_draw_image(drawable, gc, IMG_SUB_PAGE_ICON_SEARCH, 18, 7, -1, -1, NFALIGN_LEFT, 0);
			nflabel = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SEARCH", nffont_get_pango_font(NFFONT_XLARGE_NORMAL_1), COLOR_IDX(30));
		}
		else if (type == NFSETUP_WINDOW_DISK_DELETE)
		{
		    nfutil_draw_image(drawable, gc, IMG_SUB_PAGE_ICON_STORAGE, 18, 7, -1, -1, NFALIGN_LEFT, 0);
			nflabel = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ERASE VIDEO", nffont_get_pango_font(NFFONT_XLARGE_NORMAL_1), COLOR_IDX(30));
		}

		if ((type != NFSETUP_WINDOW_ARCHIVING) && (type != NFSETUP_WINDOW_SEARCH))
		{
		    nfutil_draw_image(drawable, gc, MENU_BG_TOP_LOGO, 854, 10, -1, -1, NFALIGN_LEFT, 0);
		}
/*
		if ((type != NFSETUP_WINDOW_ARCHIVING) && (type != NFSETUP_WINDOW_SEARCH))
		{
			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(TODO_COLOR));
			gdk_draw_rectangle(drawable, gc, TRUE, 0, 72, DISPLAY_ACTIVE_WIDTH, 2);
		}
*/

        if (nflabel)
        {
    		nfui_nfobject_set_size(nflabel, 300, 27);
    		nfui_nflabel_set_align((NFLABEL*)nflabel, NFALIGN_LEFT, 0);
    		nfui_nfobject_use_tooltip(nflabel, FALSE);
    		nfui_nfobject_use_focus(nflabel, NFOBJECT_FOCUS_OFF);
    		nfui_nfobject_show(nflabel);

            switch (get_ui_type())
    		{
    			case 31:  nfui_nffixed_put((NFFIXED*)obj, nflabel, 18, 27); break;	// i3dvr
    			case 32:  nfui_nffixed_put((NFFIXED*)obj, nflabel, 86, 27); break;	// cbc
    			case 46:  nfui_nffixed_put((NFFIXED*)obj, nflabel, 90, 27); break;	// novus
    			case 200: nfui_nffixed_put((NFFIXED*)obj, nflabel, 34, 27); break;	// itx2

    			default:  nfui_nffixed_put((NFFIXED*)obj, nflabel, 90, 27); break;	// itx1
    		}
        }

		g_object_unref(gc);
	}
	else if(evt->type == GDK_DELETE)
	{
		PAGEID pid;
		NFOBJECT *setup_wnd;
		nfsetup_window_type type;

		type = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "setup_window_type"));

        if (type == NFSETUP_WINDOW_USER_GUIDE)
            nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, 1920, 1080);

        if (var_get_vendor_code() != 230)
        {
    		switch (type) {
    		case NFSETUP_WINDOW_SEARCH:
    			scm_put_log(CLOSE_PLAYBACK, 0, 0);
    			break;
    		case NFSETUP_WINDOW_ARCHIVING:
    			scm_put_log(CLOSE_ARCHIVING, 0, 0);
    			break;
    		case NFSETUP_WINDOW_RECORDING:
    			scm_put_log(CLOSE_REC_SETUP, 0, 0);
    			break;
    		default:
    			break;
    		}
		}

		setup_wnd = nfui_nfobject_get_top(obj);
		pid = nfui_get_pid_by_object(setup_wnd);

		nfui_page_close(pid, setup_wnd);

		switch(type)
		{
			case NFSETUP_WINDOW_CAMERA:
			case NFSETUP_WINDOW_DISPLAY:
			case NFSETUP_WINDOW_SOUND:
			case NFSETUP_WINDOW_SYSTEM:
			case NFSETUP_WINDOW_USER:
			case NFSETUP_WINDOW_NETWORK:
			case NFSETUP_WINDOW_EVENT:
			case NFSETUP_WINDOW_DISK:
			case NFSETUP_WINDOW_RECORDING:
				prvSendSetupNoti(0);
				break;

			default:
				break;
		}
	}
	return FALSE;
}

static gboolean pre_dir_h_menu_bg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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
			nfutil_draw_image(drawable, gc, MK_IMG_MENU_H_PAGE_BG, x, y, -1, -1, NFALIGN_LEFT, 0);
	 		nfui_nfobject_gc_unref(gc);
	 	}
		break;

		case GDK_DELETE:

		break;

		default :

		break;
	}

	return FALSE;
}

static gboolean pre_dir_v_menu_bg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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
			nfutil_draw_image(drawable, gc, MK_IMG_MENU_V_PAGE_BG, x, y, -1, -1, NFALIGN_LEFT, 0);

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(10));
			gdk_draw_rectangle(drawable, gc, TRUE, x, y, 1, MENU_V_INNER_Y+MENU_V_INNER_H+20);

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(11));
			gdk_draw_rectangle(drawable, gc, TRUE, x+1, y, 1, MENU_V_INNER_Y+MENU_V_INNER_H+20);

	 		nfui_nfobject_gc_unref(gc);
	 	}
		break;

		case GDK_DELETE:

		break;

		default :

		break;
	}

	return FALSE;
}


NFOBJECT* nftool_create_setup_window(NFWINDOW *parent, nfsetup_window_type type, gint page)
{
	NFOBJECT *nfwin, *nffixed, *nftab;
	NFOBJECT *tab_page[MAX_TAB_CNT];
	PAGEID pgid = PGID_NONE;
	guint i;
	guint disp_mode;
	gint need_noti = 0;

	gchar **strTitle = NULL;
	guint num_tabs = 0;

	guint tab_dir;

	const gchar *strImage_h_340[2] =  {
				(MKB_IMG_TAB_DIR_H_N_340),
				(MKB_IMG_TAB_DIR_H_S_340)
	};

	const gchar *strImage_h_260[2] =  {
				(MKB_IMG_TAB_DIR_H_N_260),
				(MKB_IMG_TAB_DIR_H_S_260)
	};

	const gchar *strImage_v[2] =  {
				(IMG_N_SUB_TAB),
				(IMG_S_SUB_TAB)
	};

	const gchar *guideImage_v[2] =  {
				(IMG_N_GUIDE_TAB),
				(IMG_S_GUIDE_TAB)
	};


	const guint de_colidx[3] = {COLOR_IDX(40), COLOR_IDX(42), COLOR_IDX(41)};

	static const gchar *strCamTitle[MAX_TAB_CNT];
	static const gchar *strDispTitle[MAX_TAB_CNT];
	static const gchar *strSoundTitle[MAX_TAB_CNT];
	static const gchar *strSystemTitle[MAX_TAB_CNT];
	static const gchar *strUserTitle[MAX_TAB_CNT];
	static const gchar *strNetTitle[MAX_TAB_CNT];
	static const gchar *strEvtTitle[MAX_TAB_CNT];
	static const gchar *strDiskTitle[MAX_TAB_CNT];

	static const gchar *strRecordTitle[MAX_TAB_CNT];
	static const gchar *strArchivingTitle[MAX_TAB_CNT];
	static const gchar *strSearchTitle[MAX_TAB_CNT];
	static const gchar *strDeleteTitle[MAX_TAB_CNT];
	static const gchar *strUserGuideTitle[MAX_TAB_CNT];

	if(DISPLAY_IS_D1)	disp_mode = DISPLAY_MODE_D1;
	else				disp_mode = DISPLAY_MODE_4D1;

	if(type == NFSETUP_WINDOW_CAMERA)
	{
	    vw_menu_get_str_sys_menu_sub1(strCamTitle);
		strTitle = (gchar**)(strCamTitle);
		num_tabs = mcf.sys_sub1.cnt;
		pgid = PGID_SETUP;
		need_noti = 1;
	}
	else if(type == NFSETUP_WINDOW_DISPLAY)
	{
	    vw_menu_get_str_sys_menu_sub2(strDispTitle);
		strTitle = (gchar**)(strDispTitle);
		num_tabs = mcf.sys_sub2.cnt;
		pgid = PGID_SETUP;
		need_noti = 1;
	}
	else if(type == NFSETUP_WINDOW_SOUND)
	{
	    vw_menu_get_str_sys_menu_sub3(strSoundTitle);
		strTitle = (gchar**)(strSoundTitle);
		num_tabs = mcf.sys_sub3.cnt;
		pgid = PGID_SETUP;
		need_noti = 1;
	}
	else if(type == NFSETUP_WINDOW_USER)
	{
	    vw_menu_get_str_sys_menu_sub4(strUserTitle);
		strTitle = (gchar**)(strUserTitle);
		num_tabs = mcf.sys_sub4.cnt;
		pgid = PGID_SETUP;
		need_noti = 1;
	}
	else if(type == NFSETUP_WINDOW_NETWORK)
	{
	    vw_menu_get_str_sys_menu_sub5(strNetTitle);
		strTitle = (gchar**)(strNetTitle);
		num_tabs = mcf.sys_sub5.cnt;
		pgid = PGID_SETUP;
		need_noti = 1;
	}
	else if(type == NFSETUP_WINDOW_SYSTEM)
	{
	    vw_menu_get_str_sys_menu_sub6(strSystemTitle);
		strTitle = (gchar**)(strSystemTitle);
		num_tabs = mcf.sys_sub6.cnt;
		pgid = PGID_SETUP;
		need_noti = 1;
	}
	else if(type == NFSETUP_WINDOW_DISK)
	{
	    vw_menu_get_str_sys_menu_sub7(strDiskTitle);
		strTitle = (gchar**)(strDiskTitle);
		num_tabs = mcf.sys_sub7.cnt;
		pgid = PGID_SETUP;
		need_noti = 1;
	}
	else if(type == NFSETUP_WINDOW_EVENT)
	{
	    vw_menu_get_str_sys_menu_sub8(strEvtTitle);
		strTitle = (gchar**)(strEvtTitle);
		num_tabs = mcf.sys_sub8.cnt;
		pgid = PGID_SETUP;
		need_noti = 1;
	}
	else if(type == NFSETUP_WINDOW_RECORDING)
	{
	    vw_menu_get_str_rec_menu(strRecordTitle);
		strTitle = (gchar**)(strRecordTitle);
		num_tabs = mcf.rec.cnt;
		pgid = PGID_RECORD;
	}
	else if(type == NFSETUP_WINDOW_ARCHIVING)
	{
	    vw_menu_get_str_arch_menu(strArchivingTitle);
		strTitle = (gchar**)(strArchivingTitle);
		num_tabs = mcf.arch.cnt;
		pgid = PGID_ARCHIVING;
	}
	else if(type == NFSETUP_WINDOW_SEARCH)
	{
	    vw_menu_get_str_search_menu(strSearchTitle);
		strTitle = (gchar**)(strSearchTitle);
		num_tabs = mcf.search.cnt;
		pgid = PGID_SEARCH;
	}
	else if(type == NFSETUP_WINDOW_DISK_DELETE)
	{
	    vw_menu_get_str_delete_menu(strDeleteTitle);
		strTitle = (gchar**)(strDeleteTitle);
		num_tabs = mcf.del.cnt;
		pgid = PGID_DELETE_DATA;
	}
	else if(type == NFSETUP_WINDOW_USER_GUIDE)
	{
	    vw_menu_get_str_userguide_menu(strUserGuideTitle);
		strTitle = (gchar**)(strUserGuideTitle);
		num_tabs = mcf.userguide.cnt;
		pgid = PGID_USER_GUIDE;
	}

    if (var_get_vendor_code() != 230)
    
{
    	switch (type) {
        	case NFSETUP_WINDOW_SEARCH:
        		scm_put_log(OPEN_PLAYBACK, 0, 0);
        		break;
        	case NFSETUP_WINDOW_ARCHIVING:
        		scm_put_log(OPEN_ARCHIVING, 0, 0);
        		break;
        	case NFSETUP_WINDOW_RECORDING:
        		scm_put_log(OPEN_REC_SETUP, 0, 0);
        		break;
        	default:
        		break;
    	}
	}

	nfwin = (NFOBJECT*)nfui_nfwindow_new(parent, SETUP_WINDOW_POS_X, SETUP_WINDOW_POS_Y, SETUP_WINDOW_WIDTH, SETUP_WINDOW_HEIGHT);
	nfui_nfwindow_use_double_buffer(nfwin);
	nffixed = (NFOBJECT*)nfui_nffixed_new();

	if (type == NFSETUP_WINDOW_SEARCH)
	{
		const guint colidx[3] = {COLOR_IDX(76), COLOR_IDX(78), COLOR_IDX(77)};
		nftab = (NFOBJECT*)nfui_nftab_new(num_tabs, (gchar**)strImage_h_260, (guint)MENU_DIR_H_TAB_W, (guint)MENU_DIR_H_TAB_H, NFTAB_DIR_H, strTitle, (guint*)colidx);
		tab_dir = NFTAB_DIR_H;
	}
	else if (type == NFSETUP_WINDOW_ARCHIVING)
	{
		const guint colidx[3] = {COLOR_IDX(72), COLOR_IDX(74), COLOR_IDX(73)};
		nftab = (NFOBJECT*)nfui_nftab_new(num_tabs, (gchar**)strImage_h_340, (guint)MENU_DIR_A_TAB_W, (guint)MENU_DIR_A_TAB_H, NFTAB_DIR_H, strTitle, (guint*)colidx);
		tab_dir = NFTAB_DIR_H;
	}
	else if (type == NFSETUP_WINDOW_RECORDING)
	{
		const guint colidx[3] = {COLOR_IDX(80), COLOR_IDX(82), COLOR_IDX(81)};
		nftab = (NFOBJECT*)nfui_nftab_new(num_tabs, (gchar**)strImage_v, (guint)MENU_DIR_V_TAB_W, (guint)MENU_DIR_V_TAB_H, NFTAB_DIR_V, strTitle, (guint*)colidx);
		tab_dir = NFTAB_DIR_V;
	}
	else if (type == NFSETUP_WINDOW_CAMERA)
	{
		const guint colidx[3] = {COLOR_IDX(40), COLOR_IDX(42), COLOR_IDX(41)};
		nftab = (NFOBJECT*)nfui_nftab_new(num_tabs, (gchar**)strImage_v, (guint)MENU_DIR_V_TAB_W, (guint)MENU_DIR_V_TAB_H, NFTAB_DIR_V, strTitle, (guint*)colidx);
		tab_dir = NFTAB_DIR_V;
	}
	else if (type == NFSETUP_WINDOW_DISPLAY)
	{
		const guint colidx[3] = {COLOR_IDX(44), COLOR_IDX(46), COLOR_IDX(45)};
		nftab = (NFOBJECT*)nfui_nftab_new(num_tabs, (gchar**)strImage_v, (guint)MENU_DIR_V_TAB_W, (guint)MENU_DIR_V_TAB_H, NFTAB_DIR_V, strTitle, (guint*)colidx);
		tab_dir = NFTAB_DIR_V;
	}
	else if (type == NFSETUP_WINDOW_SOUND)
	{
		const guint colidx[3] = {COLOR_IDX(48), COLOR_IDX(50), COLOR_IDX(49)};
		nftab = (NFOBJECT*)nfui_nftab_new(num_tabs, (gchar**)strImage_v, (guint)MENU_DIR_V_TAB_W, (guint)MENU_DIR_V_TAB_H, NFTAB_DIR_V, strTitle, (guint*)colidx);
		tab_dir = NFTAB_DIR_V;
	}
	else if (type == NFSETUP_WINDOW_USER)
	{
		const guint colidx[3] = {COLOR_IDX(52), COLOR_IDX(54), COLOR_IDX(53)};
		nftab = (NFOBJECT*)nfui_nftab_new(num_tabs, (gchar**)strImage_v, (guint)MENU_DIR_V_TAB_W, (guint)MENU_DIR_V_TAB_H, NFTAB_DIR_V, strTitle, (guint*)colidx);
		tab_dir = NFTAB_DIR_V;
	}
	else if (type == NFSETUP_WINDOW_NETWORK)
	{
		const guint colidx[3] = {COLOR_IDX(56), COLOR_IDX(58), COLOR_IDX(57)};
		nftab = (NFOBJECT*)nfui_nftab_new(num_tabs, (gchar**)strImage_v, (guint)MENU_DIR_V_TAB_W, (guint)MENU_DIR_V_TAB_H, NFTAB_DIR_V, strTitle, (guint*)colidx);
		tab_dir = NFTAB_DIR_V;
	}
	else if (type == NFSETUP_WINDOW_SYSTEM)
	{
		const guint colidx[3] = {COLOR_IDX(60), COLOR_IDX(62), COLOR_IDX(61)};
		nftab = (NFOBJECT*)nfui_nftab_new(num_tabs, (gchar**)strImage_v, (guint)MENU_DIR_V_TAB_W, (guint)MENU_DIR_V_TAB_H, NFTAB_DIR_V, strTitle, (guint*)colidx);
		tab_dir = NFTAB_DIR_V;
	}
	else if (type == NFSETUP_WINDOW_DISK)
	{
		const guint colidx[3] = {COLOR_IDX(64), COLOR_IDX(66), COLOR_IDX(65)};
		nftab = (NFOBJECT*)nfui_nftab_new(num_tabs, (gchar**)strImage_v, (guint)MENU_DIR_V_TAB_W, (guint)MENU_DIR_V_TAB_H, NFTAB_DIR_V, strTitle, (guint*)colidx);
		tab_dir = NFTAB_DIR_V;
	}
	else if (type == NFSETUP_WINDOW_EVENT)
	{
		const guint colidx[3] = {COLOR_IDX(68), COLOR_IDX(70), COLOR_IDX(69)};
		nftab = (NFOBJECT*)nfui_nftab_new(num_tabs, (gchar**)strImage_v, (guint)MENU_DIR_V_TAB_W, (guint)MENU_DIR_V_TAB_H, NFTAB_DIR_V, strTitle, (guint*)colidx);
		tab_dir = NFTAB_DIR_V;
	}
	else if (type == NFSETUP_WINDOW_DISK_DELETE)
	{
		const guint colidx[3] = {COLOR_IDX(64), COLOR_IDX(66), COLOR_IDX(65)};
		nftab = (NFOBJECT*)nfui_nftab_new(num_tabs, (gchar**)strImage_h_340, (guint)MENU_DIR_A_TAB_W, (guint)MENU_DIR_A_TAB_H, NFTAB_DIR_H, strTitle, (guint*)colidx);
		tab_dir = NFTAB_DIR_H;
	}
	else if (type == NFSETUP_WINDOW_USER_GUIDE)
	{
		nftab = (NFOBJECT*)nfui_nftab_new(num_tabs, (gchar**)guideImage_v, (guint)204, (guint)70, NFTAB_DIR_V, strTitle, (guint*)de_colidx);
		tab_dir = NFTAB_DIR_V;
	}
	else
	{
		nftab = (NFOBJECT*)nfui_nftab_new(num_tabs, (gchar**)strImage_v, (guint)MENU_DIR_V_TAB_W, (guint)MENU_DIR_V_TAB_H, NFTAB_DIR_V, strTitle, (guint*)de_colidx);
		tab_dir = NFTAB_DIR_V;
	}

	nfui_nftab_set_pango_font((NFTAB*)nftab, SETUP_TAB_FONT);
	nfui_nftab_set_margin((NFTAB*)nftab, SETUP_TAB_MARGIN);

	nfui_nfobject_set_data(nffixed, "setup_window_type", GUINT_TO_POINTER(type));

	for(i=0; i<num_tabs; i++)
	{
		tab_page[i] = (NFOBJECT*)nfui_nffixed_new();

		if (tab_dir == NFTAB_DIR_H)
		{
			nfui_nfobject_set_size(tab_page[i], (guint)MENU_H_PAGE_W, (guint)MENU_H_PAGE_H);
    		nfui_nfobject_modify_bg(tab_page[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
			nfui_nftab_regi_page((NFTAB*)nftab, tab_page[i], i);
			nfui_nffixed_put((NFFIXED*)nffixed, tab_page[i], MENU_H_PAGE_X, MENU_H_PAGE_Y);
			nfui_regi_pre_event_callback(tab_page[i], pre_dir_h_menu_bg_event_handler);
		}
		else if ((type == NFSETUP_WINDOW_USER_GUIDE) && (tab_dir == NFTAB_DIR_V))
		{
			nfui_nfobject_set_size(tab_page[i], (guint)1673, (guint)971+65-6);
    		nfui_nfobject_modify_bg(tab_page[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
			nfui_nftab_regi_page((NFTAB*)nftab, tab_page[i], i);
			nfui_nffixed_put((NFFIXED*)nffixed, tab_page[i], 13+204+10, 44);
		}
		else
		{
			nfui_nfobject_set_size(tab_page[i], (guint)MENU_V_PAGE_W, (guint)MENU_V_PAGE_H);
    		nfui_nfobject_modify_bg(tab_page[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
			nfui_nftab_regi_page((NFTAB*)nftab, tab_page[i], i);
			nfui_nffixed_put((NFFIXED*)nffixed, tab_page[i], MENU_V_PAGE_X, MENU_V_PAGE_Y);
			nfui_regi_pre_event_callback(tab_page[i], pre_dir_v_menu_bg_event_handler);
		}
	}

//	nfui_nfobject_modify_bg(nffixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(190));  // window bg

	nfui_nfobject_show(nftab);
	nfui_nfobject_show(tab_page[page]);
    nfui_nftab_set_cur_page(nftab, page);

	nfui_nfobject_show(nffixed);
	nfui_nfwindow_add((NFWINDOW*)nfwin, nffixed);

	if (tab_dir == NFTAB_DIR_H)
		nfui_nffixed_put((NFFIXED*)nffixed, nftab, (guint)MENU_DIR_H_TAB_X, (guint)MENU_DIR_H_TAB_Y);
    else if ((type == NFSETUP_WINDOW_USER_GUIDE) && (tab_dir == NFTAB_DIR_V))
		nfui_nffixed_put((NFFIXED*)nffixed, nftab, (guint)13, (guint)56);
	else
		nfui_nffixed_put((NFFIXED*)nffixed, nftab, (guint)MENU_DIR_V_TAB_X, (guint)MENU_DIR_V_TAB_Y);

	nfui_regi_pre_event_callback(nffixed, nffixed_event_cb);

	nfui_run_main_event_handler(nfwin);

	switch(type)
	{
		case NFSETUP_WINDOW_CAMERA:
		case NFSETUP_WINDOW_DISPLAY:
		case NFSETUP_WINDOW_SOUND:
		case NFSETUP_WINDOW_SYSTEM:
		case NFSETUP_WINDOW_USER:
		case NFSETUP_WINDOW_NETWORK:
		case NFSETUP_WINDOW_EVENT:
		case NFSETUP_WINDOW_DISK:
		case NFSETUP_WINDOW_RECORDING:
			prvSendSetupNoti(1);
			break;

		default:
			break;
	}

	nfui_page_open(pgid, nfwin, nfui_get_last_user());

	return nfwin;
}

#if 0
static void prvDrawSideImage(GdkDrawable *drawable)
{
	GdkGC *gc;

	gc = gdk_gc_new(drawable);

	nfutil_draw_image(drawable, gc, IMG_MAINBG9, (DISPLAY_IS_D1 ? 120:236), (DISPLAY_IS_D1 ? 52:102), -1, -1, NFALIGN_LEFT, 0);

	// setup window right
	nfutil_draw_image(drawable, gc, IMG_MAINBG12, (DISPLAY_IS_D1 ? 582:1164), (DISPLAY_IS_D1 ? 52:102), -1, -1, NFALIGN_LEFT, 0);

	// setup window bottom
	nfutil_draw_image(drawable, gc, IMG_MAINBG13, (DISPLAY_IS_D1 ? 0:0), (DISPLAY_IS_D1 ? 402:802), -1, -1, NFALIGN_LEFT, 0);


	// tab button bg(left side)
	nfutil_draw_image(drawable, gc, IMG_MAIN_TEXT_BG, (DISPLAY_IS_D1 ? 0:0), (DISPLAY_IS_D1 ? 52:102),	-1, -1, NFALIGN_LEFT, 0);

	g_object_unref(gc);
}

static gboolean top_page_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	if(evt->type == GDK_EXPOSE)
	{
		GdkDrawable *drawable;
		nfsetup_window_type type;

		drawable = nfui_nfobject_get_window((NFOBJECT*)obj);

		prvDrawSideImage(drawable);
	}
	return FALSE;
}

static gboolean top_tab_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		NFOBJECT *setup_wnd;
		nfsetup_window_type type;

		type = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "setup_window_type"));

		switch (type) {
		case NFSETUP_WINDOW_SEARCH:
			scm_put_log(CLOSE_PLAYBACK, 0, 0);
			break;
		case NFSETUP_WINDOW_ARCHIVING:
			scm_put_log(CLOSE_ARCHIVING, 0, 0);
			break;
		case NFSETUP_WINDOW_RECORDING:
			scm_put_log(CLOSE_REC_SETUP, 0, 0);
			break;
		default:
			break;
		}


		setup_wnd = nfui_nfobject_get_top(obj);
		nfui_page_close(PGID_SETUP, setup_wnd);
		prvSendSetupNoti(0);
	}
	return FALSE;
}

NFOBJECT* nftool_create_top_tab_setup_window(NFWINDOW *parent, nfsetup_window_type type, gint page)
{
	NFOBJECT *nfwin, *nffixed, *top_tab;
	NFOBJECT *top_page[8];
	guint i, j;
	guint disp_mode;
	gint need_noti = 1;


	const gchar *strImage[2] =  {
				(IMG_MAIN_TEXT_00),
				(IMG_MAIN_TEXT)
	};

	const gchar *def_img[2] = {
				(IMG_BTN_CAMERA_NORMAL), 	// DESELECTED
				(IMG_BTN_CAMERA_FOCUS), 	// DESELECTED
	};
	const gchar *nor_img[8] =  {
				(IMG_BTN_CAMERA_NORMAL), 	// DESELECTED
				(IMG_BTN_DISPLAY_NORMAL), 	// DESELECTED
				(IMG_BTN_SOUND_NORMAL), 	// DESELECTED
				(IMG_BTN_SYSTEM_NORMAL), 	// DESELECTED
				(IMG_BTN_USER_NORMAL), 	// DESELECTED
				(IMG_BTN_NETWORK_NORMAL), 	// DESELECTED
				(IMG_BTN_EVENT_NORMAL), 	// DESELECTED
				(IMG_BTN_DISK_NORMAL), 	// DESELECTED
	};

	const gchar *act_img[8] =  {
				(IMG_BTN_CAMERA_FOCUS), 	// DESELECTED
				(IMG_BTN_DISPLAY_FOCUS), 	// DESELECTED
				(IMG_BTN_SOUND_FOCUS), 	// DESELECTED
				(IMG_BTN_SYSTEM_FOCUS), 	// DESELECTED
				(IMG_BTN_USER_FOCUS), 	// DESELECTED
				(IMG_BTN_NETWORK_FOCUS), 	// DESELECTED
				(IMG_BTN_EVENT_FOCUS), 	// DESELECTED
				(IMG_BTN_DISK_FOCUS), 	// DESELECTED
	};

	const guint colidx[3] = {COLOR_IDX(40), COLOR_IDX(42), COLOR_IDX(41)};

	gchar **strTitle = NULL;
	guint num_tabs = 0;

	nfwin = (NFOBJECT*)nfui_nfwindow_new(parent, SETUP_WINDOW_POS_X, SETUP_WINDOW_POS_Y, SETUP_WINDOW_WIDTH, SETUP_WINDOW_HEIGHT);

	nffixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(nffixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));	// window bg
	nfui_regi_pre_event_callback(nffixed, top_tab_fixed_event_cb);
	nfui_nfobject_set_size(nffixed, SETUP_FIXED_WIDTH, SETUP_FIXED_HEIGHT);
	nfui_nfobject_show(nffixed);

	top_tab = (NFOBJECT*)nfui_nftab_new(8, (gchar**)def_img, 74, 52, NFTAB_DIR_H, NULL, (guint*)colidx);
	nfui_nftab_set_init_page(top_tab, type);
	nfui_nftab_set_images(top_tab, 8, (gchar**)nor_img, (gchar**)act_img);
	nfui_nffixed_put((NFFIXED*)nffixed, top_tab, 0, 0);
	nfui_nfobject_show(top_tab);

	for(i=0; i<8; i++)
	{
		top_page[i] = nftool_create_setup_window(parent, i, page);
		nfui_nfobject_set_size(top_page[i], (guint)SETUP_FIXED_WIDTH, (guint)SETUP_FIXED_HEIGHT);
		nfui_nftab_regi_page((NFTAB*)top_tab, top_page[i], i);
		nfui_regi_pre_event_callback(top_page[i], top_page_fixed_event_cb);
		nfui_nfobject_modify_bg(top_page[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(0));	// tab bg
		nfui_nffixed_put((NFFIXED*)nffixed, top_page[i], 0, 58);
	}
	nfui_nfobject_show(top_page[type]);

	nfui_nfwindow_add((NFWINDOW*)nfwin, nffixed);

	nfui_run_main_event_handler(nfwin);

	nfui_page_open(PGID_SETUP, nfwin, nfui_get_last_user());

	prvSendSetupNoti(1);

	return nfwin;
}
#endif

void nftool_destroy_setup_window(NFOBJECT *setup_wnd)
{
	nfui_nfobject_destroy(setup_wnd);
}

NFOBJECT* nftool_get_main_fixed_from_window(NFWINDOW *wnd)
{
	return wnd->child;;
}

NFOBJECT* nftool_get_main_fixed_from_setup_window(NFOBJECT *setup_wnd)
{
	return ((NFWINDOW*)setup_wnd)->child;;
}

NFOBJECT* nftool_get_nftab_from_setup_window(NFOBJECT *setup_wnd)
{
	NFOBJECT *obj = NULL;
	NFFIXED *nffixed = NULL;
	guint i, child_num;

	nffixed = (NFFIXED*)(((NFWINDOW*)setup_wnd)->child);

	child_num = g_slist_length(nffixed->children);

	for(i=0; i<child_num; i++)
	{
		obj = (NFOBJECT*)g_slist_nth_data(nffixed->children, i);
		if(obj->type == NFOBJECT_TYPE_NFTAB)
			break;
	}

	if(i == child_num)	return NULL;

	return obj;
}

NFOBJECT* nftool_get_nftab_from_top_tab_setup_window(NFOBJECT *setup_wnd, nfsetup_window_type type)
{
	NFOBJECT *obj = NULL;
	NFFIXED *nffixed = NULL;
	guint i, child_num;

	nffixed = (NFFIXED*)(((NFWINDOW*)setup_wnd)->child);

	child_num = g_slist_length(nffixed->children);

	for(i=0; i<child_num; i++)
	{
		obj = (NFOBJECT*)g_slist_nth_data(nffixed->children, i);

		if(obj->type == NFOBJECT_TYPE_NFTAB)
			break;
	}
	nffixed = ((NFTAB*)obj)->page[type];
	child_num = g_slist_length(nffixed->children);

	for(i=0; i<child_num; i++)
	{
		obj = (NFOBJECT*)g_slist_nth_data(nffixed->children, i);
		if(obj->type == NFOBJECT_TYPE_NFTAB)
			break;
	}

	if(i == child_num)	return NULL;

	return obj;
}





/**************************************************************
 * POPUP WINDOW
 * ***********************************************************/

//#define	POPUP_TITLE_FONT		PANGO_SANS_BOLD_ITALIC_18


#define	POPUP_CLOSE_BTN_MARGIN	(guint)(DISPLAY_IS_D1 ? 6:12)


static gboolean pre_popup_nffixed_event_cb(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	NFWINDOW *popup;
	GdkDrawable *drawable;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	popup = (NFWINDOW*)(obj->parent);

	if(evt->type == GDK_EXPOSE)
	{
		drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
		nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);

		nfui_nfobject_gc_unref(gc);
	}
	else if(evt->type == GDK_DELETE)
	{
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);

		nfui_page_close(PGID_POPUPWND, (NFOBJECT*)popup);
	}

	return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *topwin;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {
		  return FALSE;
	       }

		topwin = nfui_nfobject_get_top(obj);

		nfui_nfobject_destroy(topwin);
	}

	return FALSE;
}


NFOBJECT* nftool_resize_popup_window(NFOBJECT *nfwin, int w, int h)
{
	nfwin = (NFOBJECT*)nfui_nfwindow_resize(nfwin, w, h);
	if(nfwin == NULL)	return NULL;

	nfui_nfobject_set_size(((NFWINDOW*)nfwin)->child, w, h);
    nfui_signal_emit(((NFWINDOW*)nfwin)->child, GDK_EXPOSE, TRUE);
}

NFOBJECT* nftool_create_popup_window(NFWINDOW *parent, guint x, guint y, guint width, guint height, const gchar *strTitle, gboolean close)
{
	NFOBJECT *nfwin, *nffixed, *nftitle, *close_btn;
	guint btn_w, btn_h;
	GdkPixbuf *btn_image[NFOBJECT_STATE_COUNT];

	nfwin = (NFOBJECT*)nfui_nfwindow_new(parent, x, y, width, height);
	if(nfwin == NULL)	return NULL;
	g_stpcpy(((NFWINDOW*)nfwin)->strTitle, strTitle);

	nffixed =(NFOBJECT*) nfui_nffixed_new();

	nftitle = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle, POPUP_TITLE_FONT, COLOR_IDX(205));
	nfui_nfobject_set_size(nftitle, (width - 8), 38);
	nfui_nflabel_set_align((NFLABEL*)nftitle, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(nftitle, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(nftitle);
	nfui_nffixed_put((NFFIXED*)nffixed, nftitle, 4, 2);

	nfui_nfobject_show(nffixed);
	nfui_nfwindow_add((NFWINDOW*)nfwin, nffixed);
	nfui_regi_pre_event_callback(nffixed, pre_popup_nffixed_event_cb);
	nfui_run_main_event_handler(nfwin);
	nfui_page_open(PGID_POPUPWND, nfwin, nfui_get_last_user());

	return nfwin;
}






/**************************************************************
 * MASSAGE BOX
 * ***********************************************************/
#define	NFMBOX_WIDTH		(guint)(msg_w > 605 ? msg_w:605)
#define	NFMBOX_HEIGHT		(guint)(DISPLAY_IS_D1 ? 310:310)

#define	NFMBOX_X			(guint)((DISPLAY_ACTIVE_WIDTH-NFMBOX_WIDTH)/2)
#define	NFMBOX_Y			(guint)((DISPLAY_ACTIVE_HEIGHT-NFMBOX_HEIGHT-72)/2)

#define NFMBOX_BORDER		(guint)(DISPLAY_IS_D1 ? 4:4)
#define	NFMBOX_GAP			(guint)(DISPLAY_IS_D1 ? 2:2)
#define	NFMBOX_HEAD_SIZE	(guint)(DISPLAY_IS_D1 ? 42:42)

#define	NFMBOX_FIXED_X		(guint)(NFMBOX_BORDER + NFMBOX_GAP)
#define	NFMBOX_FIXED_Y		(guint)(NFMBOX_BORDER + NFMBOX_HEAD_SIZE + NFMBOX_GAP)
#define	NFMBOX_FIXED_WIDTH	(guint)(NFMBOX_WIDTH - ((NFMBOX_BORDER*4))-NFMBOX_GAP)
#define	NFMBOX_FIXED_HEIGHT	(guint)(NFMBOX_HEIGHT - ((NFMBOX_BORDER+NFMBOX_GAP)*3 + NFMBOX_HEAD_SIZE))

#define	NFMBOX_LABEL_X		(NFMBOX_GAP)
#define	NFMBOX_LABEL_WIDTH	(guint)(NFMBOX_FIXED_WIDTH - (NFMBOX_GAP*2))
#define	NFMBOX_LABEL_HEIGHT	(guint)(DISPLAY_IS_D1 ? 88:88)



/**************************************************************
 * MASSAGE BOX EX
 * ***********************************************************/
#define NFMBOXEX_WIDTH        (guint)(msg_w > 605 ? msg_w:605)

#define NFMBOXEX_WIDTH        (guint)(msg_w > 605 ? msg_w:605)
#define NFMBOXEX_HEIGHT       (guint)(DISPLAY_IS_D1 ? 390:390)

#define NFMBOXEX_X            (guint)((DISPLAY_ACTIVE_WIDTH-NFMBOXEX_WIDTH)/2)
#define NFMBOXEX_Y            (guint)((DISPLAY_ACTIVE_HEIGHT-NFMBOXEX_HEIGHT-72)/2)

#define NFMBOXEX_BORDER       (guint)(DISPLAY_IS_D1 ? 4:4)
#define NFMBOXEX_GAP          (guint)(DISPLAY_IS_D1 ? 2:2)
#define NFMBOXEX_HEAD_SIZE    (guint)(DISPLAY_IS_D1 ? 42:42)

#define NFMBOXEX_FIXED_X      (guint)(NFMBOXEX_BORDER + NFMBOXEX_GAP)
#define NFMBOXEX_FIXED_Y      (guint)(NFMBOXEX_BORDER + NFMBOXEX_HEAD_SIZE + NFMBOXEX_GAP)
#define NFMBOXEX_FIXED_WIDTH  (guint)(NFMBOXEX_WIDTH - ((NFMBOXEX_BORDER*4))-NFMBOXEX_GAP)
#define NFMBOXEX_FIXED_HEIGHT (guint)(NFMBOXEX_HEIGHT - ((NFMBOXEX_BORDER+NFMBOXEX_GAP)*3 + NFMBOXEX_HEAD_SIZE))

#define NFMBOXEX_LABEL_X      (NFMBOXEX_GAP)
#define NFMBOXEX_LABEL_WIDTH  (guint)(NFMBOXEX_FIXED_WIDTH - (NFMBOXEX_GAP*2))
#define NFMBOXEX_LABEL_HEIGHT (guint)(DISPLAY_IS_D1 ? 88:88)


static mb_type ret_value;
static guint msg_w = 0;


static void reset_mbox_width(const gchar *msg1, const gchar *msg2, const gchar *msg3, const gchar *msg4)
{
	gint i;
	guint w = 0;

	for(i=0, msg_w = 0; i<4; i++) {
		if(i == 0 && msg1) 	  	w = nfutil_string_width(1, NULL, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), msg1, NORMAL_SPACING);
		else if(i == 1 && msg2)	w = nfutil_string_width(1, NULL, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), msg2, NORMAL_SPACING);
		else if(i == 2 && msg3)	w = nfutil_string_width(1, NULL, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), msg3, NORMAL_SPACING);
		else if(i == 3 && msg4)	w = nfutil_string_width(1, NULL, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), msg4, NORMAL_SPACING);

		if(msg_w < w) msg_w = w;
	}
	msg_w += (NFMBOX_BORDER +  NFMBOX_GAP) + 100;		// 100 is a margin
}

static gboolean _check_waiting_timer(gpointer data)
{
    static gint img_idx = 0;

    if (img_idx >= 6) img_idx = 0;

    NFUTIL_THREADS_ENTER();

    if (img_idx == 0) nfui_nfimage_change_image((NFIMAGE*)g_wait_obj, IMG_GEAR_01);
    else if(img_idx == 1) nfui_nfimage_change_image((NFIMAGE*)g_wait_obj, IMG_GEAR_02);
    else if(img_idx == 2) nfui_nfimage_change_image((NFIMAGE*)g_wait_obj, IMG_GEAR_03);
    else if(img_idx == 3) nfui_nfimage_change_image((NFIMAGE*)g_wait_obj, IMG_GEAR_04);
    else if(img_idx == 4) nfui_nfimage_change_image((NFIMAGE*)g_wait_obj, IMG_GEAR_05);
    else if(img_idx == 5) nfui_nfimage_change_image((NFIMAGE*)g_wait_obj, IMG_GEAR_06);

    nfui_signal_emit(g_wait_obj, GDK_EXPOSE, TRUE);

    NFUTIL_THREADS_LEAVE();

    img_idx++;

    return TRUE;
}

static gint _run_waiting_timer()
{
    if (g_waiting_timer) return 0;

    g_waiting_timer = g_timeout_add(200, _check_waiting_timer, 0);

    return 0;
}

static gint _end_waiting_timer()
{
    if (g_waiting_timer)
    {
        g_source_remove(g_waiting_timer);
        g_wait_obj = 0;
        g_waiting_timer = 0;
    }

    return 0;
}


static gboolean pre_cont_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case GDK_DELETE:
		{
			mb_type type;

			type = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "mbox_type"));

            _end_waiting_timer();

			if(type == NFTOOL_MB_NONE)
				break;

			if(tid)
				g_source_remove(tid);

			//GTK_QUIT();
			gtk_main_quit();
		}
		break;

		default:
			break;
	}

	return FALSE;
}

static gboolean post_ok_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {
			return FALSE;
		}
		ret_value = NFTOOL_MB_OK;

		top  = nfui_nfobject_get_top(obj);

		/*
		if (tid) {
			g_source_remove(tid);
		}
		*/

		nfui_nfobject_destroy(top);
		gtk_main_quit();
	}

	return FALSE;
}

static gboolean post_cancel_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {
			return FALSE;
		}

		ret_value = NFTOOL_MB_CANCEL;
		top  = nfui_nfobject_get_top(obj);

		nfui_nfobject_destroy(top);
		gtk_main_quit();
	}

	return FALSE;
}

static gboolean post_again_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint *check_state;

	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED) {
		check_state = nfui_nfobject_get_data(obj, "check state");

		if(nfui_check_button_get_active(NF_CHECKBUTTON(obj)))
			*check_state = 1;
		else
			*check_state = 0;
	}

	return FALSE;
}

static mb_type prvMBox_new(NFWINDOW *parent, NFOBJECT** wnd, const gchar *strTitle, const gchar *strContent1, mb_type type)
{
	NFOBJECT *mb_wnd = NULL;
	NFOBJECT *mb_fixed = NULL;
	NFOBJECT *fixed1 = NULL;
	NFOBJECT *ok_btn = NULL;
	NFOBJECT *cancel_btn = NULL;
	NFOBJECT *label = NULL;
	NFOBJECT *icon = NULL;

	guint i;

	// mbox width
	reset_mbox_width(strContent1, NULL, NULL, NULL);

	// window
	mb_wnd = nftool_create_popup_window(parent, NFMBOX_X, NFMBOX_Y, NFMBOX_WIDTH, NFMBOX_HEIGHT, strTitle, FALSE);
	mb_fixed = ((NFWINDOW*)mb_wnd)->child;
	nfui_nfobject_modify_bg(mb_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfwindow_set_title(mb_wnd, "MESSAGE BOX");
	nfui_nfobject_show(mb_fixed);

	// fixed
	fixed1 = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed1, NFMBOX_FIXED_WIDTH, NFMBOX_FIXED_HEIGHT);
	nfui_nfobject_show(fixed1);
	nfui_nffixed_put((NFFIXED*)mb_fixed, fixed1, NFMBOX_FIXED_X, NFMBOX_FIXED_Y);
	nfui_regi_pre_event_callback(fixed1, pre_cont_fixed_event_handler);

	// icon
	if(!strcmp(strTitle, "CONFIRM")) 	  icon   = nfui_nfimage_new(IMG_QUESTION_ICON);
	else if(!strcmp(strTitle, "NOTICE"))  icon   = nfui_nfimage_new(IMG_WARNING_ICON);
	else if(!strcmp(strTitle, "WARNING")) icon   = nfui_nfimage_new(IMG_ERROR_ICON);
	else if(!strcmp(strTitle, "ERROR"))   icon   = nfui_nfimage_new(IMG_ERROR_ICON);

	if(icon) {
		nfui_nfobject_show(icon);
		nfui_nffixed_put((NFFIXED*)fixed1, icon, ((NFMBOX_FIXED_WIDTH-64)/2), 0);
	}

	// content
	label = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)label, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(label, NFMBOX_LABEL_WIDTH, NFMBOX_LABEL_HEIGHT);
	nfui_nfobject_use_focus(label, NFOBJECT_FOCUS_OFF);
	nfui_nffixed_put((NFFIXED*)fixed1, label, NFMBOX_LABEL_X, 76);
	nfui_nfobject_show(label);

	nfui_nflabel_set_text((NFLABEL*)label, (gchar*)strContent1);

	nfui_nfobject_set_data(fixed1, "mbox_type", GUINT_TO_POINTER(type));

	if(type == NFTOOL_MB_NONE)
	{
		if (wnd) *wnd = mb_wnd;
		nfui_nfobject_show(mb_wnd);

		return NFTOOL_MB_OK;
	}

	if (type == NFTOOL_MB_AUTO_OK || type == NFTOOL_MB_AUTO_OKCANCEL || type == NFTOOL_MB_SLEEP_AUTO) {
		tid = g_timeout_add(rm_mbox_sec*1000, remove_mbox, mb_wnd);
		nfui_nfobject_set_data(mb_wnd, "timer_id", tid);
	}


	if(type==NFTOOL_MB_OK || type==NFTOOL_MB_OKCANCEL || type == NFTOOL_MB_AUTO_OK || type == NFTOOL_MB_AUTO_OKCANCEL)
	{
		ok_btn = nftool_normal_button_create_type1("OK", 192);
		nfui_nfobject_show(ok_btn);

		nfui_nffixed_put((NFFIXED*)fixed1, ok_btn, (fixed1->width - 192)/2, fixed1->height-NFMBOX_GAP-(44 + 25));

		nfui_regi_post_event_callback(ok_btn, post_ok_button_event_handler);
	}

	if(type==NFTOOL_MB_ERASECANCEL)
	{
		ok_btn = nftool_normal_button_create_type1("OKERASE", 192);
		nfui_nfobject_show(ok_btn);
		nfui_nffixed_put((NFFIXED*)fixed1, ok_btn, (fixed1->width - 192)/2, fixed1->height-NFMBOX_GAP-(44 + 25));
		nfui_regi_post_event_callback(ok_btn, post_ok_button_event_handler);
	}

	if(type==NFTOOL_MB_CANCEL || type==NFTOOL_MB_OKCANCEL || type == NFTOOL_MB_AUTO_OKCANCEL || type==NFTOOL_MB_ERASECANCEL)
	{
		cancel_btn = nftool_normal_button_create_type1("CANCEL", 192);
		nfui_nfobject_show(cancel_btn);

		nfui_nffixed_put((NFFIXED*)fixed1, cancel_btn, (fixed1->width - 192)/2, fixed1->height-NFMBOX_GAP-(44 + 25));

		nfui_regi_post_event_callback(cancel_btn, post_cancel_button_event_handler);
	}

	if(type==NFTOOL_MB_YESNO)
	{
		ok_btn = nftool_normal_button_create_type1("YES", 192);
		nfui_nfobject_show(ok_btn);

		nfui_nffixed_put((NFFIXED*)fixed1, ok_btn, (fixed1->width - 192)/2, fixed1->height-NFMBOX_GAP-(44 + 25));

		nfui_regi_post_event_callback(ok_btn, post_ok_button_event_handler);

		cancel_btn = nftool_normal_button_create_type1("NO", 192);
		nfui_nfobject_show(cancel_btn);

		nfui_nffixed_put((NFFIXED*)fixed1, cancel_btn, (fixed1->width - 192)/2, fixed1->height-NFMBOX_GAP-(44 + 25));

		nfui_regi_post_event_callback(cancel_btn, post_cancel_button_event_handler);
	}

	if(type==NFTOOL_MB_OKCANCEL || type == NFTOOL_MB_YESNO || type == NFTOOL_MB_AUTO_OKCANCEL || type==NFTOOL_MB_ERASECANCEL)
	{
		nfui_nfobject_move(ok_btn, fixed1->width/2 - 192 - 3, fixed1->height-NFMBOX_GAP-(44 + 25));
		nfui_nfobject_move(cancel_btn, fixed1->width/2 + 3, fixed1->height-NFMBOX_GAP-(44 + 25));
	}

	nfui_nfobject_show(mb_wnd);

	nfui_page_close(PGID_POPUPWND, mb_wnd);
	nfui_page_open(PGID_MESSAGEBOX, mb_wnd, nfui_get_last_user());

	nfui_make_key_hierarchy((NFWINDOW*)mb_wnd);

	if(type==NFTOOL_MB_OK || type==NFTOOL_MB_OKCANCEL || type == NFTOOL_MB_AUTO_OK || type == NFTOOL_MB_YESNO || type==NFTOOL_MB_ERASECANCEL)
		nfui_set_key_focus(ok_btn, TRUE);
	else if(type==NFTOOL_MB_CANCEL || type == NFTOOL_MB_AUTO_OKCANCEL)
		nfui_set_key_focus(cancel_btn, TRUE);

	if(type == NFTOOL_MB_OK || type == NFTOOL_MB_AUTO_OK || type == NFTOOL_MB_SLEEP_AUTO)
		ret_value = NFTOOL_MB_OK;
	else
		ret_value = NFTOOL_MB_CANCEL;

	gtk_main();
	//GTK_MAIN();

	nfui_page_close(PGID_MESSAGEBOX, mb_wnd);

	return ret_value;

}

static mb_type prvMBoxEx_new(NFWINDOW *parent, NFOBJECT** wnd, const gchar *strTitle, const gchar *strContent1, mb_type type)
{
    NFOBJECT *mb_wnd = NULL;
    NFOBJECT *mb_fixed = NULL;
    NFOBJECT *fixed1 = NULL;
    NFOBJECT *ok_btn = NULL;
    NFOBJECT *cancel_btn = NULL;
    NFOBJECT *label = NULL;
    NFOBJECT *icon = NULL;

    guint i;

    // mbox width
    reset_mbox_width(strContent1, NULL, NULL, NULL);

    // window
    mb_wnd = nftool_create_popup_window(parent, NFMBOXEX_X, NFMBOXEX_Y, NFMBOXEX_WIDTH, NFMBOXEX_HEIGHT, strTitle, FALSE);
    mb_fixed = ((NFWINDOW*)mb_wnd)->child;
    nfui_nfobject_modify_bg(mb_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfwindow_set_title(mb_wnd, "MESSAGE BOX");
    nfui_nfobject_show(mb_fixed);

    // fixed
    fixed1 = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed1, NFMBOXEX_FIXED_WIDTH, NFMBOXEX_FIXED_HEIGHT);
    nfui_nfobject_show(fixed1);
    nfui_nffixed_put((NFFIXED*)mb_fixed, fixed1, NFMBOXEX_FIXED_X, NFMBOXEX_FIXED_Y);
    nfui_regi_pre_event_callback(fixed1, pre_cont_fixed_event_handler);

    // icon
    if(!strcmp(strTitle, "CONFIRM"))      icon   = nfui_nfimage_new(IMG_QUESTION_ICON);
    else if(!strcmp(strTitle, "NOTICE"))  icon   = nfui_nfimage_new(IMG_WARNING_ICON);
    else if(!strcmp(strTitle, "WARNING")) icon   = nfui_nfimage_new(IMG_ERROR_ICON);
    else if(!strcmp(strTitle, "ERROR"))   icon   = nfui_nfimage_new(IMG_ERROR_ICON);

    if(icon) {
        nfui_nfobject_show(icon);
        nfui_nffixed_put((NFFIXED*)fixed1, icon, ((NFMBOXEX_FIXED_WIDTH-64)/2), 0);
    }

    // content
    label = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)label, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(label, NFMBOXEX_LABEL_WIDTH, NFMBOXEX_LABEL_HEIGHT);
    nfui_nfobject_use_focus(label, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)fixed1, label, NFMBOXEX_LABEL_X, 76);
    nfui_nfobject_show(label);

    nfui_nflabel_set_text((NFLABEL*)label, (gchar*)strContent1);

    nfui_nfobject_set_data(fixed1, "mbox_type", GUINT_TO_POINTER(type));

    if(type == NFTOOL_MB_NONE)
    {
        if (wnd) *wnd = mb_wnd;
        nfui_nfobject_show(mb_wnd);

        return NFTOOL_MB_OK;
    }

    if (type == NFTOOL_MB_AUTO_OK || type == NFTOOL_MB_AUTO_OKCANCEL || type == NFTOOL_MB_SLEEP_AUTO) {
        tid = g_timeout_add(rm_mbox_sec*1000, remove_mbox, mb_wnd);
        nfui_nfobject_set_data(mb_wnd, "timer_id", tid);
    }


    if(type==NFTOOL_MB_OK || type==NFTOOL_MB_OKCANCEL || type == NFTOOL_MB_AUTO_OK || type == NFTOOL_MB_AUTO_OKCANCEL)
    {
        ok_btn = nftool_normal_button_create_type1("OK", 192);
        nfui_nfobject_show(ok_btn);

        nfui_nffixed_put((NFFIXED*)fixed1, ok_btn, (fixed1->width - 192)/2, fixed1->height-NFMBOXEX_GAP-(44 + 25));

        nfui_regi_post_event_callback(ok_btn, post_ok_button_event_handler);
    }

    if(type==NFTOOL_MB_ERASECANCEL)
    {
        ok_btn = nftool_normal_button_create_type1("OKERASE", 192);
        nfui_nfobject_show(ok_btn);
        nfui_nffixed_put((NFFIXED*)fixed1, ok_btn, (fixed1->width - 192)/2, fixed1->height-NFMBOXEX_GAP-(44 + 25));
        nfui_regi_post_event_callback(ok_btn, post_ok_button_event_handler);
    }

    if(type==NFTOOL_MB_CANCEL || type==NFTOOL_MB_OKCANCEL || type == NFTOOL_MB_AUTO_OKCANCEL || type==NFTOOL_MB_ERASECANCEL)
    {
        cancel_btn = nftool_normal_button_create_type1("CANCEL", 192);
        nfui_nfobject_show(cancel_btn);

        nfui_nffixed_put((NFFIXED*)fixed1, cancel_btn, (fixed1->width - 192)/2, fixed1->height-NFMBOXEX_GAP-(44 + 25));

        nfui_regi_post_event_callback(cancel_btn, post_cancel_button_event_handler);
    }

    if(type==NFTOOL_MB_YESNO)
    {
        ok_btn = nftool_normal_button_create_type1("YES", 192);
        nfui_nfobject_show(ok_btn);

        nfui_nffixed_put((NFFIXED*)fixed1, ok_btn, (fixed1->width - 192)/2, fixed1->height-NFMBOXEX_GAP-(44 + 25));

        nfui_regi_post_event_callback(ok_btn, post_ok_button_event_handler);

        cancel_btn = nftool_normal_button_create_type1("NO", 192);
        nfui_nfobject_show(cancel_btn);

        nfui_nffixed_put((NFFIXED*)fixed1, cancel_btn, (fixed1->width - 192)/2, fixed1->height-NFMBOXEX_GAP-(44 + 25));

        nfui_regi_post_event_callback(cancel_btn, post_cancel_button_event_handler);
    }

    if(type==NFTOOL_MB_OKCANCEL || type == NFTOOL_MB_YESNO || type == NFTOOL_MB_AUTO_OKCANCEL || type==NFTOOL_MB_ERASECANCEL)
    {
        nfui_nfobject_move(ok_btn, fixed1->width/2 - 192 - 3, fixed1->height-NFMBOXEX_GAP-(44 + 25));
        nfui_nfobject_move(cancel_btn, fixed1->width/2 + 3, fixed1->height-NFMBOXEX_GAP-(44 + 25));
    }

    nfui_nfobject_show(mb_wnd);

    nfui_page_close(PGID_POPUPWND, mb_wnd);
    nfui_page_open(PGID_MESSAGEBOX, mb_wnd, nfui_get_last_user());

    nfui_make_key_hierarchy((NFWINDOW*)mb_wnd);

    if(type==NFTOOL_MB_OK || type==NFTOOL_MB_OKCANCEL || type == NFTOOL_MB_AUTO_OK || type == NFTOOL_MB_YESNO || type==NFTOOL_MB_ERASECANCEL)
        nfui_set_key_focus(ok_btn, TRUE);
    else if(type==NFTOOL_MB_CANCEL || type == NFTOOL_MB_AUTO_OKCANCEL)
        nfui_set_key_focus(cancel_btn, TRUE);

    if(type == NFTOOL_MB_OK || type == NFTOOL_MB_AUTO_OK || type == NFTOOL_MB_SLEEP_AUTO)
        ret_value = NFTOOL_MB_OK;
    else
        ret_value = NFTOOL_MB_CANCEL;

    gtk_main();
    //GTK_MAIN();

    nfui_page_close(PGID_MESSAGEBOX, mb_wnd);

    return ret_value;

}

static mb_type prvMBox_new2(NFWINDOW *parent, NFOBJECT** wnd, const gchar *strTitle, const gchar *strContent1, mb_type type, NFOBJECT **mboxwnd)
{
	NFOBJECT *mb_wnd = NULL;
	NFOBJECT *mb_fixed = NULL;
	NFOBJECT *fixed1 = NULL;
	NFOBJECT *ok_btn = NULL;
	NFOBJECT *cancel_btn = NULL;
	NFOBJECT *label = NULL;
	NFOBJECT *icon = NULL;

	guint i;


	// mbox width
	reset_mbox_width(strContent1, NULL, NULL, NULL);

	// window
	mb_wnd = nftool_create_popup_window(parent, NFMBOX_X, NFMBOX_Y, NFMBOX_WIDTH, NFMBOX_HEIGHT, strTitle, FALSE);
	*mboxwnd = mb_wnd;
	mb_fixed = ((NFWINDOW*)mb_wnd)->child;
	nfui_nfobject_modify_bg(mb_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfwindow_set_title(mb_wnd, "MESSAGE BOX");
	nfui_nfobject_show(mb_fixed);

	// fixed
	fixed1 = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed1, NFMBOX_FIXED_WIDTH, NFMBOX_FIXED_HEIGHT);
	nfui_nfobject_show(fixed1);
	nfui_nffixed_put((NFFIXED*)mb_fixed, fixed1, NFMBOX_FIXED_X, NFMBOX_FIXED_Y);
	nfui_regi_pre_event_callback(fixed1, pre_cont_fixed_event_handler);

	// icon
	if(!strcmp(strTitle, "CONFIRM")) 	  icon   = nfui_nfimage_new(IMG_QUESTION_ICON);
	else if(!strcmp(strTitle, "NOTICE"))  icon   = nfui_nfimage_new(IMG_WARNING_ICON);
	else if(!strcmp(strTitle, "WARNING")) icon   = nfui_nfimage_new(IMG_ERROR_ICON);
	else if(!strcmp(strTitle, "ERROR"))   icon   = nfui_nfimage_new(IMG_ERROR_ICON);

	if(icon) {
		nfui_nfobject_show(icon);
		nfui_nffixed_put((NFFIXED*)fixed1, icon, ((NFMBOX_FIXED_WIDTH-64)/2), 0);
	}

	// content
	label = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)label, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(label, NFMBOX_LABEL_WIDTH, NFMBOX_LABEL_HEIGHT);
	nfui_nfobject_use_focus(label, NFOBJECT_FOCUS_OFF);
	nfui_nffixed_put((NFFIXED*)fixed1, label, NFMBOX_LABEL_X, 76);
	nfui_nfobject_show(label);

	nfui_nflabel_set_text((NFLABEL*)label, (gchar*)strContent1);

	nfui_nfobject_set_data(fixed1, "mbox_type", GUINT_TO_POINTER(type));

	if(type == NFTOOL_MB_NONE)
	{
		if (wnd) *wnd = mb_wnd;
		nfui_nfobject_show(mb_wnd);

		return NFTOOL_MB_OK;
	}

	if (type == NFTOOL_MB_AUTO_OK || type == NFTOOL_MB_AUTO_OKCANCEL) {
		tid = g_timeout_add(rm_mbox_sec*1000, remove_mbox, mb_wnd);
		nfui_nfobject_set_data(mb_wnd, "timer_id", tid);
	}


	if(type==NFTOOL_MB_OK || type==NFTOOL_MB_OKCANCEL || type == NFTOOL_MB_AUTO_OK || type == NFTOOL_MB_AUTO_OKCANCEL)
	{
		ok_btn = nftool_normal_button_create_type1("OK", 192);
		nfui_nfobject_show(ok_btn);

		nfui_nffixed_put((NFFIXED*)fixed1, ok_btn, (fixed1->width - 192)/2, fixed1->height-NFMBOX_GAP-(44 + 25));

		nfui_regi_post_event_callback(ok_btn, post_ok_button_event_handler);
	}

	if(type==NFTOOL_MB_CANCEL || type==NFTOOL_MB_OKCANCEL || type == NFTOOL_MB_AUTO_OKCANCEL)
	{
		cancel_btn = nftool_normal_button_create_type1("CANCEL", 192);
		nfui_nfobject_show(cancel_btn);

		nfui_nffixed_put((NFFIXED*)fixed1, cancel_btn, (fixed1->width - 192)/2, fixed1->height-NFMBOX_GAP-(44 + 25));

		nfui_regi_post_event_callback(cancel_btn, post_cancel_button_event_handler);
	}

	if(type==NFTOOL_MB_YESNO)
	{
		ok_btn = nftool_normal_button_create_type1("YES", 192);
		nfui_nfobject_show(ok_btn);

		nfui_nffixed_put((NFFIXED*)fixed1, ok_btn, (fixed1->width - 192)/2, fixed1->height-NFMBOX_GAP-(44 + 25));

		nfui_regi_post_event_callback(ok_btn, post_ok_button_event_handler);

		cancel_btn = nftool_normal_button_create_type1("NO", 192);
		nfui_nfobject_show(cancel_btn);

		nfui_nffixed_put((NFFIXED*)fixed1, cancel_btn, (fixed1->width - 192)/2, fixed1->height-NFMBOX_GAP-(44 + 25));

		nfui_regi_post_event_callback(cancel_btn, post_cancel_button_event_handler);
	}

	if(type==NFTOOL_MB_OKCANCEL || type == NFTOOL_MB_YESNO || type == NFTOOL_MB_AUTO_OKCANCEL)
	{
		nfui_nfobject_move(ok_btn, fixed1->width/2 - 192 - 3, fixed1->height-NFMBOX_GAP-(44 + 25));
		nfui_nfobject_move(cancel_btn, fixed1->width/2 + 3, fixed1->height-NFMBOX_GAP-(44 + 25));
	}

	nfui_nfobject_show(mb_wnd);

	nfui_page_close(PGID_POPUPWND, mb_wnd);
	nfui_page_open(PGID_MESSAGEBOX, mb_wnd, nfui_get_last_user());

	nfui_make_key_hierarchy((NFWINDOW*)mb_wnd);

	if(type==NFTOOL_MB_OK || type==NFTOOL_MB_OKCANCEL || type == NFTOOL_MB_AUTO_OK || type == NFTOOL_MB_YESNO)
		nfui_set_key_focus(ok_btn, TRUE);
	else if(type==NFTOOL_MB_CANCEL || type == NFTOOL_MB_AUTO_OKCANCEL)
		nfui_set_key_focus(cancel_btn, TRUE);

	if(type == NFTOOL_MB_OK || type == NFTOOL_MB_AUTO_OK)
		ret_value = NFTOOL_MB_OK;
	else
		ret_value = NFTOOL_MB_CANCEL;

	gtk_main();
	//GTK_MAIN();

	nfui_page_close(PGID_MESSAGEBOX, mb_wnd);

	return ret_value;

}


static mb_type prvMBox_graph_new(NFWINDOW *parent, NFOBJECT** wnd, const gchar *strTitle, const gchar *strContent1, const gchar *strContent2, mb_type type)
{
    NFOBJECT *mb_wnd = NULL;
    NFOBJECT *mb_fixed = NULL;
    NFOBJECT *fixed1 = NULL;
    NFOBJECT *label = NULL;
    NFOBJECT *obj;

    guint img_w, img_h;
    guint pos_x = 0;
    guint pos_y = 0;
    guint i;

    // mbox width

    reset_mbox_width(strContent2, NULL, NULL, NULL);

    // window
	mb_wnd = nftool_create_popup_window(parent, NFMBOX_X, NFMBOX_Y, NFMBOX_WIDTH, NFMBOX_HEIGHT, strTitle, FALSE);
	mb_fixed = ((NFWINDOW*)mb_wnd)->child;
	nfui_nfobject_modify_bg(mb_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfwindow_set_title(mb_wnd, "MESSAGE BOX");
	nfui_nfobject_show(mb_fixed);

	// fixed
	fixed1 = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed1, NFMBOX_FIXED_WIDTH, NFMBOX_FIXED_HEIGHT);
	nfui_nfobject_show(fixed1);
	nfui_nffixed_put((NFFIXED*)mb_fixed, fixed1, NFMBOX_FIXED_X, NFMBOX_FIXED_Y);
	nfui_regi_pre_event_callback(fixed1, pre_cont_fixed_event_handler);

    nfui_get_image_size(IMG_GEAR_01, &img_w, &img_h);

    pos_y = 40;

	obj = (NFOBJECT*)nfui_nfimage_new(IMG_GEAR_01);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, (NFMBOX_FIXED_WIDTH-img_w)/2, pos_y);
	g_wait_obj = obj;

    pos_x = NFMBOX_LABEL_X;
    pos_y += img_h + 30;

    if(!strlen(strContent2))  pos_y += 20;

    label = (NFOBJECT *)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)label, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(label, NFMBOX_LABEL_WIDTH, NFMBOX_LABEL_HEIGHT/4);
    nfui_nfobject_use_focus(label, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)fixed1, label, NFMBOX_LABEL_X, pos_y);
    nfui_nfobject_show(label);

    nfui_nflabel_set_text((NFLABEL*)label, (gchar*)strContent1);

    pos_y += NFMBOX_LABEL_HEIGHT/4+15;

    label = (NFOBJECT *)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)label, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(label, NFMBOX_LABEL_WIDTH, NFMBOX_LABEL_HEIGHT/4);
    nfui_nfobject_use_focus(label, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)fixed1, label, NFMBOX_LABEL_X, pos_y);
    nfui_nfobject_show(label);

    nfui_nflabel_set_text((NFLABEL*)label, (gchar*)strContent2);

    nfui_nfobject_set_data(fixed1, "mbox_type", GUINT_TO_POINTER(type));

    _run_waiting_timer();

    if( type == NFTOOL_MB_NONE)
    {
        if (wnd) *wnd = mb_wnd;
        nfui_nfobject_show(mb_wnd);

        return NFTOOL_MB_OK;
    }

    if(type == NFTOOL_MB_OK || type == NFTOOL_MB_AUTO_OK)
        ret_value = NFTOOL_MB_OK;
    else
        ret_value = NFTOOL_MB_CANCEL;

    return ret_value;
}




static mb_type prvMBox1_new(NFWINDOW *parent, NFOBJECT** wnd, const gchar *strTitle, const gchar *strContent1, const gchar *strContent2, const gchar *strContent3, const gchar *strContent4, mb_type type)
{
	NFOBJECT *mb_wnd = NULL;
	NFOBJECT *mb_fixed = NULL;
	NFOBJECT *fixed1 = NULL;
	NFOBJECT *ok_btn = NULL;
	NFOBJECT *cancel_btn = NULL;

	NFOBJECT *label[4] = {NULL, NULL, NULL, NULL};
	NFOBJECT *icon = NULL;

	guint tmp1, tmp2;
	guint btn_w=0, btn_h=0;
	guint num_line=0;
	guint i;
	guint str_w = 0;
	const guint btn_font_color[NFOBJECT_STATE_COUNT] = {100, 101, 103, 104};
	GdkDrawable *drawable = NULL;

	// mbox width
	reset_mbox_width(strContent1, strContent2, strContent3, strContent4);

	// window
	mb_wnd = nftool_create_popup_window(parent, NFMBOX_X, NFMBOX_Y, NFMBOX_WIDTH, NFMBOX_HEIGHT, strTitle, FALSE);
	mb_fixed = ((NFWINDOW*)mb_wnd)->child;
	nfui_nfwindow_set_title(mb_wnd, "4LINE MESSAGE BOX");
	nfui_nfobject_show(mb_fixed);

	// fixed
	fixed1 = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed1, NFMBOX_FIXED_WIDTH, NFMBOX_FIXED_HEIGHT);
	nfui_nfobject_show(fixed1);
	nfui_nffixed_put((NFFIXED*)mb_fixed, fixed1, NFMBOX_FIXED_X, NFMBOX_FIXED_Y);
	nfui_regi_pre_event_callback(fixed1, pre_cont_fixed_event_handler);

	// icon
	if(!strcmp(strTitle, "NOTICE")) 	  icon   = nfui_nfimage_new(IMG_QUESTION_ICON);
	else if(!strcmp(strTitle, "WARNING")) icon   = nfui_nfimage_new(IMG_WARNING_ICON);
	else if(!strcmp(strTitle, "ERROR"))   icon   = nfui_nfimage_new(IMG_ERROR_ICON);

	if(icon) {
		nfui_nfobject_show(icon);
		nfui_nffixed_put((NFFIXED*)fixed1, icon, (NFMBOX_FIXED_WIDTH-64)/2, 0);
	}

	tmp1 = 76;
	for(i=0; i<4; i++)
	{
		label[i] = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
		nfui_nfobject_set_size(label[i], NFMBOX_LABEL_WIDTH, (NFMBOX_LABEL_HEIGHT/4));
		nfui_nflabel_set_align((NFLABEL *)label[i], NFALIGN_LEFT, 10);
		nfui_nfobject_use_focus(label[i], NFOBJECT_FOCUS_OFF);
		nfui_nffixed_put((NFFIXED*)fixed1, label[i], NFMBOX_LABEL_X, tmp1);
		tmp1 += (NFMBOX_LABEL_HEIGHT/4+5);
	}

	if(strContent1)	num_line++;
	if(strContent2)	num_line++;
	if(strContent3)	num_line++;
	if(strContent4)	num_line++;

	drawable = nfui_nfobject_get_window(label[1]);

	if(num_line == 1)
	{
		nfui_nflabel_set_text((NFLABEL*)label[1], (gchar*)strContent1);
              str_w = nfutil_string_width(1, drawable, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), strContent1, NORMAL_SPACING) ;
              if(str_w > NFMBOX_LABEL_WIDTH)
              {
                 nfui_nflabel_set_spacing((NFLABEL*)label[1],  CONDENSED_SPACING);
              }
		nfui_nfobject_show(label[1]);
	}
	else if(num_line == 2)
	{
		nfui_nflabel_set_text((NFLABEL*)label[0], (gchar*)strContent1);
              str_w = nfutil_string_width(1, drawable, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), strContent1, NORMAL_SPACING) ;
              if(str_w > NFMBOX_LABEL_WIDTH)
              {
                 nfui_nflabel_set_spacing((NFLABEL*)label[0],  CONDENSED_SPACING);
              }

		nfui_nflabel_set_text((NFLABEL*)label[1], (gchar*)strContent2);
              str_w = nfutil_string_width(1, drawable, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), strContent2, NORMAL_SPACING) ;
              if(str_w > NFMBOX_LABEL_WIDTH)
              {
                 nfui_nflabel_set_spacing((NFLABEL*)label[1],  CONDENSED_SPACING);
              }

		nfui_nfobject_show(label[0]);
		nfui_nfobject_show(label[1]);
	}
	else if(num_line == 3)
	{
		nfui_nflabel_set_text((NFLABEL*)label[0], (gchar*)strContent1);
              str_w = nfutil_string_width(1, drawable, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), strContent1, NORMAL_SPACING) ;
              if(str_w > NFMBOX_LABEL_WIDTH)
              {
                 nfui_nflabel_set_spacing((NFLABEL*)label[0],  CONDENSED_SPACING);
              }

		nfui_nflabel_set_text((NFLABEL*)label[1], (gchar*)strContent2);
              str_w = nfutil_string_width(1, drawable, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), strContent2, NORMAL_SPACING) ;
              if(str_w > NFMBOX_LABEL_WIDTH)
              {
                 nfui_nflabel_set_spacing((NFLABEL*)label[1],  CONDENSED_SPACING);
              }

		nfui_nflabel_set_text((NFLABEL*)label[2], (gchar*)strContent3);
              str_w = nfutil_string_width(1, drawable, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), strContent3, NORMAL_SPACING) ;
              if(str_w > NFMBOX_LABEL_WIDTH)
              {
                 nfui_nflabel_set_spacing((NFLABEL*)label[2],  CONDENSED_SPACING);
              }

		nfui_nfobject_show(label[0]);
		nfui_nfobject_show(label[1]);
		nfui_nfobject_show(label[2]);
	}
	else if(num_line == 4)
	{
		nfui_nflabel_set_text((NFLABEL*)label[0], (gchar*)strContent1);
              str_w = nfutil_string_width(1, drawable, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), strContent1, NORMAL_SPACING) ;
              if(str_w > NFMBOX_LABEL_WIDTH)
              {
                 nfui_nflabel_set_spacing((NFLABEL*)label[0],  CONDENSED_SPACING);
              }

		nfui_nflabel_set_text((NFLABEL*)label[1], (gchar*)strContent2);
              str_w = nfutil_string_width(1, drawable, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), strContent2, NORMAL_SPACING) ;
              if(str_w > NFMBOX_LABEL_WIDTH)
              {
                 nfui_nflabel_set_spacing((NFLABEL*)label[1],  CONDENSED_SPACING);
              }

		nfui_nflabel_set_text((NFLABEL*)label[2], (gchar*)strContent3);
              str_w = nfutil_string_width(1, drawable, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), strContent3, NORMAL_SPACING) ;
              if(str_w > NFMBOX_LABEL_WIDTH)
              {
                 nfui_nflabel_set_spacing((NFLABEL*)label[2],  CONDENSED_SPACING);
              }

		nfui_nflabel_set_text((NFLABEL*)label[3], (gchar*)strContent4);
              str_w = nfutil_string_width(1, drawable, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), strContent4, NORMAL_SPACING) ;
              if(str_w > NFMBOX_LABEL_WIDTH)
              {
                 nfui_nflabel_set_spacing((NFLABEL*)label[3],  CONDENSED_SPACING);
              }

		nfui_nfobject_show(label[0]);
		nfui_nfobject_show(label[1]);
		nfui_nfobject_show(label[2]);
		nfui_nfobject_show(label[3]);

	}

	nfui_nfobject_set_data(fixed1, "mbox_type", GUINT_TO_POINTER(type));

	if(type == NFTOOL_MB_NONE)
	{
		*wnd = mb_wnd;
		nfui_nfobject_show(mb_wnd);

		return NFTOOL_MB_OK;
	}


	if(type==NFTOOL_MB_OK || type==NFTOOL_MB_OKCANCEL)
	{
		ok_btn = nftool_normal_button_create_type1("OK", 192);
		nfui_nfobject_show(ok_btn);

		nfui_nffixed_put((NFFIXED*)fixed1, ok_btn, (fixed1->width - 192)/2, fixed1->height-NFMBOX_GAP-(44 + 25));

		nfui_regi_post_event_callback(ok_btn, post_ok_button_event_handler);
	}

	if(type==NFTOOL_MB_CANCEL || type==NFTOOL_MB_OKCANCEL)
	{
		cancel_btn = nftool_normal_button_create_type1("CANCEL", 192);
		nfui_nfobject_show(cancel_btn);

		nfui_nffixed_put((NFFIXED*)fixed1, cancel_btn, (fixed1->width - 192)/2, fixed1->height-NFMBOX_GAP-(44 + 25));

		nfui_regi_post_event_callback(cancel_btn, post_cancel_button_event_handler);
	}

	if(type==NFTOOL_MB_OKCANCEL)
	{
		nfui_nfobject_move(ok_btn, fixed1->width/2 - 192 - 3, fixed1->height-NFMBOX_GAP-(44 + 25));
		nfui_nfobject_move(cancel_btn, fixed1->width/2 + 3, fixed1->height-NFMBOX_GAP-(44 + 25));
	}

	nfui_nfobject_show(mb_wnd);

	nfui_page_close(PGID_POPUPWND, mb_wnd);
	nfui_page_open(PGID_MESSAGEBOX, mb_wnd, nfui_get_last_user());

	nfui_make_key_hierarchy((NFWINDOW*)mb_wnd);

	if(type==NFTOOL_MB_OK || type==NFTOOL_MB_OKCANCEL)
		nfui_set_key_focus(ok_btn, TRUE);
	else if(type==NFTOOL_MB_CANCEL)
		nfui_set_key_focus(cancel_btn, TRUE);

	if(type == NFTOOL_MB_OK)
		ret_value = NFTOOL_MB_OK;
	else
		ret_value = NFTOOL_MB_CANCEL;

	gtk_main();

	nfui_page_close(PGID_MESSAGEBOX, mb_wnd);

	return ret_value;

}
static gboolean update_text(gpointer data)
{
	NFOBJECT *label = NULL;
	gchar str[256];

	label = (NFOBJECT*)data;
	if(rm_mbox_sec == 0)
	{
		ret_value = NFTOOL_MB_OK;

		g_timeout_add(1000, remove_mbox, nfui_nfobject_get_top(label));
		return FALSE;
	}

	memset(str, 0x00, sizeof(str));

	label = (NFLABEL*)data;

	sprintf(str,lookup_string(update_msg),rm_mbox_sec);

	nfui_nflabel_set_text((NFLABEL*)label, str);
	if(!nfui_nfobject_is_shown(label))
		nfui_nfobject_show(label);

	nfui_signal_emit(label, GDK_EXPOSE, FALSE);

	rm_mbox_sec--;

	return TRUE;
}


static mb_type prvMBox2_new(NFWINDOW *parent, NFOBJECT** wnd, const gchar *strTitle, const gchar *strContent1, const gchar *strContent2, const gchar *strContent3, const gchar *strContent4, mb_type type)
{
	NFOBJECT *mb_wnd = NULL;
	NFOBJECT *mb_fixed = NULL;
	NFOBJECT *fixed1 = NULL;
	NFOBJECT *ok_btn = NULL;
	NFOBJECT *cancel_btn = NULL;

	NFOBJECT *label[4] = {NULL, NULL, NULL, NULL};
	NFOBJECT *icon = NULL;

	guint tmp1, tmp2;
	guint btn_w=0, btn_h=0;
	guint num_line=0;
	guint i;
	guint str_w = 0;
	const guint btn_font_color[NFOBJECT_STATE_COUNT] = {100, 101, 103, 104};
	GdkDrawable *drawable = NULL;


	// mbox width
	reset_mbox_width(strContent1, strContent2, strContent3, strContent4);

	// window
	mb_wnd = nftool_create_popup_window(parent, NFMBOX_X, NFMBOX_Y, NFMBOX_WIDTH, NFMBOX_HEIGHT, strTitle, FALSE);
	mb_fixed = ((NFWINDOW*)mb_wnd)->child;
	nfui_nfobject_modify_bg(mb_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfwindow_set_title(mb_wnd, "MESSAGE BOX");
	nfui_nfobject_show(mb_fixed);

	// fixed
	fixed1 = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed1, NFMBOX_FIXED_WIDTH, NFMBOX_FIXED_HEIGHT+10);
	nfui_nfobject_show(fixed1);
	nfui_nffixed_put((NFFIXED*)mb_fixed, fixed1, NFMBOX_FIXED_X, NFMBOX_FIXED_Y);
	nfui_regi_pre_event_callback(fixed1, pre_cont_fixed_event_handler);

	// icon
	if(!strcmp(strTitle, "CONFIRM")) 	  icon   = nfui_nfimage_new(IMG_QUESTION_ICON);
	else if(!strcmp(strTitle, "NOTICE"))  icon   = nfui_nfimage_new(IMG_WARNING_ICON);
	else if(!strcmp(strTitle, "WARNING")) icon   = nfui_nfimage_new(IMG_WARNING_ICON);
	else if(!strcmp(strTitle, "ERROR"))   icon   = nfui_nfimage_new(IMG_ERROR_ICON);

	if(icon) {
		nfui_nfobject_show(icon);
		nfui_nffixed_put((NFFIXED*)fixed1, icon, (NFMBOX_FIXED_WIDTH-64)/2, 0);
	}

	tmp1 = 65;
	for(i=0; i<4; i++)
	{
		label[i] = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
		nfui_nfobject_set_size(label[i], NFMBOX_LABEL_WIDTH, (NFMBOX_LABEL_HEIGHT/4)+3);
		nfui_nflabel_set_align((NFLABEL *)label[i], NFALIGN_LEFT, 10);
		nfui_nfobject_use_focus(label[i], NFOBJECT_FOCUS_OFF);
		nfui_nffixed_put((NFFIXED*)fixed1, label[i], NFMBOX_LABEL_X, tmp1);
		tmp1 += (NFMBOX_LABEL_HEIGHT/4+5);
	}

	if(strContent1)	num_line++;
	if(strContent2)	num_line++;
	if(strContent3)	num_line++;
	if(strContent4)	num_line++;

	drawable = nfui_nfobject_get_window(label[1]);

	if(num_line == 1)
	{
		nfui_nflabel_set_text((NFLABEL*)label[1], (gchar*)strContent1);
		str_w = nfutil_string_width(1, drawable, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), strContent1, NORMAL_SPACING) ;
		if(str_w > NFMBOX_LABEL_WIDTH)
		{
			nfui_nflabel_set_spacing((NFLABEL*)label[1],  CONDENSED_SPACING);
		}
		//nfui_nfobject_show(label[1]);
	}

	else if(num_line == 2)
	{
		nfui_nflabel_set_text((NFLABEL*)label[0], (gchar*)strContent1);
		str_w = nfutil_string_width(1, drawable, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), strContent1, NORMAL_SPACING) ;
		if(str_w > NFMBOX_LABEL_WIDTH)
		{
			nfui_nflabel_set_spacing((NFLABEL*)label[0],  CONDENSED_SPACING);
		}

		nfui_nflabel_set_text((NFLABEL*)label[1], (gchar*)strContent2);
		str_w = nfutil_string_width(1, drawable, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), strContent2, NORMAL_SPACING) ;
		if(str_w > NFMBOX_LABEL_WIDTH)
		{
			nfui_nflabel_set_spacing((NFLABEL*)label[1],  CONDENSED_SPACING);
		}

		nfui_nfobject_show(label[0]);
		//nfui_nfobject_show(label[1]);
	}
	else if(num_line == 3)
	{
		nfui_nflabel_set_text((NFLABEL*)label[0], (gchar*)strContent1);
		str_w = nfutil_string_width(1, drawable, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), strContent1, NORMAL_SPACING) ;
		if(str_w > NFMBOX_LABEL_WIDTH)
		{
			nfui_nflabel_set_spacing((NFLABEL*)label[0],  CONDENSED_SPACING);
		}

		nfui_nflabel_set_text((NFLABEL*)label[1], (gchar*)strContent2);
		str_w = nfutil_string_width(1, drawable, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), strContent2, NORMAL_SPACING) ;
		if(str_w > NFMBOX_LABEL_WIDTH)
		{
			nfui_nflabel_set_spacing((NFLABEL*)label[1],  CONDENSED_SPACING);
		}

		nfui_nflabel_set_text((NFLABEL*)label[2], (gchar*)strContent3);
		str_w = nfutil_string_width(1, drawable, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), strContent3, NORMAL_SPACING) ;
		if(str_w > NFMBOX_LABEL_WIDTH)
		{
			nfui_nflabel_set_spacing((NFLABEL*)label[2],  CONDENSED_SPACING);
		}

		nfui_nfobject_show(label[0]);
		nfui_nfobject_show(label[1]);
		//nfui_nfobject_show(label[2]);
	}
	else if(num_line == 4)
	{
		nfui_nflabel_set_text((NFLABEL*)label[0], (gchar*)strContent1);
		str_w = nfutil_string_width(1, drawable, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), strContent1, NORMAL_SPACING) ;
		if(str_w > NFMBOX_LABEL_WIDTH)
		{
			nfui_nflabel_set_spacing((NFLABEL*)label[0],  CONDENSED_SPACING);
		}

		nfui_nflabel_set_text((NFLABEL*)label[1], (gchar*)strContent2);
		str_w = nfutil_string_width(1, drawable, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), strContent2, NORMAL_SPACING) ;
		if(str_w > NFMBOX_LABEL_WIDTH)
		{
			nfui_nflabel_set_spacing((NFLABEL*)label[1],  CONDENSED_SPACING);
		}

		nfui_nflabel_set_text((NFLABEL*)label[2], (gchar*)strContent3);
		str_w = nfutil_string_width(1, drawable, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), strContent3, NORMAL_SPACING) ;
		if(str_w > NFMBOX_LABEL_WIDTH)
		{
			nfui_nflabel_set_spacing((NFLABEL*)label[2],  CONDENSED_SPACING);
		}

		nfui_nflabel_set_text((NFLABEL*)label[3], (gchar*)strContent4);
		str_w = nfutil_string_width(1, drawable, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), strContent4, NORMAL_SPACING) ;
		if(str_w > NFMBOX_LABEL_WIDTH)
		{
			nfui_nflabel_set_spacing((NFLABEL*)label[3],  CONDENSED_SPACING);
		}

		nfui_nfobject_show(label[0]);
		nfui_nfobject_show(label[1]);
		nfui_nfobject_show(label[2]);
		//nfui_nfobject_show(label[3]);

	}

	nfui_nfobject_set_data(fixed1, "mbox_type", GUINT_TO_POINTER(type));

	if(type == NFTOOL_MB_NONE)
	{
		*wnd = mb_wnd;
		nfui_nfobject_show(mb_wnd);

		return NFTOOL_MB_OK;
	}

	if(type==NFTOOL_MB_OK || type==NFTOOL_MB_OKCANCEL || type == NFTOOL_MB_AUTO_OK || type == NFTOOL_MB_AUTO_OKCANCEL)
	{
		ok_btn = nftool_normal_button_create_type1("OK", 192);
		nfui_nfobject_show(ok_btn);

		nfui_nffixed_put((NFFIXED*)fixed1, ok_btn, (fixed1->width - 192)/2, fixed1->height-NFMBOX_GAP-(44 + 25));

		nfui_regi_post_event_callback(ok_btn, post_ok_button_event_handler);
	}

	if(type==NFTOOL_MB_CANCEL || type==NFTOOL_MB_OKCANCEL || type == NFTOOL_MB_AUTO_OKCANCEL)
	{
		cancel_btn = nftool_normal_button_create_type1("CANCEL", 192);
		nfui_nfobject_show(cancel_btn);

		nfui_nffixed_put((NFFIXED*)fixed1, cancel_btn, (fixed1->width - 192)/2, fixed1->height-NFMBOX_GAP-(44 + 25));

		nfui_regi_post_event_callback(cancel_btn, post_cancel_button_event_handler);
	}

	if(type==NFTOOL_MB_OKCANCEL || type == NFTOOL_MB_AUTO_OKCANCEL)
	{
		nfui_nfobject_move(ok_btn, fixed1->width/2 - 192 - 3, fixed1->height-NFMBOX_GAP-(44 + 25));
		nfui_nfobject_move(cancel_btn, fixed1->width/2 + 3, fixed1->height-NFMBOX_GAP-(44 + 25));
	}

	nfui_nfobject_show(mb_wnd);

	nfui_page_close(PGID_POPUPWND, mb_wnd);
	nfui_page_open(PGID_MESSAGEBOX, mb_wnd, nfui_get_last_user());

	nfui_make_key_hierarchy((NFWINDOW*)mb_wnd);

	if(type==NFTOOL_MB_OK || type==NFTOOL_MB_OKCANCEL || type == NFTOOL_MB_AUTO_OK || type == NFTOOL_MB_AUTO_OKCANCEL)
		nfui_set_key_focus(ok_btn, TRUE);
	else if(type==NFTOOL_MB_CANCEL)
		nfui_set_key_focus(cancel_btn, TRUE);

	if(type == NFTOOL_MB_OK || type == NFTOOL_MB_AUTO_OK)
		ret_value = NFTOOL_MB_OK;
	else
		ret_value = NFTOOL_MB_CANCEL;

	memset(&update_msg, 0x00, sizeof(update_msg));
	g_stpcpy(update_msg, nfui_nflabel_get_text(label[num_line-1]));

	tid = g_timeout_add(1000, update_text, label[num_line-1]);
	nfui_nfobject_set_data(mb_wnd, "timer_id", tid);

	gtk_main();

	nfui_page_close(PGID_MESSAGEBOX, mb_wnd);

	return ret_value;

}


static mb_type prvMBox_check_new(NFWINDOW *parent, NFOBJECT** wnd, const gchar *strTitle, const gchar *strContent1, const gchar *strCheck, gint *check_ret, mb_type type)
{
	NFOBJECT *mb_wnd = NULL;
	NFOBJECT *mb_fixed = NULL;
	NFOBJECT *fixed1 = NULL;
	NFOBJECT *icon = NULL;
	NFOBJECT *label = NULL;
	NFOBJECT *check_btn = NULL;
	NFOBJECT *ok_btn = NULL;
	NFOBJECT *cancel_btn = NULL;

	guint size_w, size_h;
	guint i;
	guint check_gap;

	*check_ret = 0;

	// mbox width
	reset_mbox_width(strContent1, NULL, NULL, NULL);

	// window
	mb_wnd = nftool_create_popup_window(parent, NFMBOX_X, NFMBOX_Y, NFMBOX_WIDTH, NFMBOX_HEIGHT+60, strTitle, FALSE);
	mb_fixed = ((NFWINDOW*)mb_wnd)->child;
	nfui_nfobject_modify_bg(mb_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfwindow_set_title(mb_wnd, "MESSAGE BOX");
	nfui_nfobject_show(mb_fixed);

	// fixed
	fixed1 = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed1, NFMBOX_FIXED_WIDTH, NFMBOX_FIXED_HEIGHT+60);
	nfui_nfobject_show(fixed1);
	nfui_nffixed_put((NFFIXED*)mb_fixed, fixed1, NFMBOX_FIXED_X, NFMBOX_FIXED_Y);
	nfui_regi_pre_event_callback(fixed1, pre_cont_fixed_event_handler);

	// icon
	if(!strcmp(strTitle, "CONFIRM")) 	  icon   = nfui_nfimage_new(IMG_QUESTION_ICON);
	else if(!strcmp(strTitle, "NOTICE"))  icon   = nfui_nfimage_new(IMG_WARNING_ICON);
	else if(!strcmp(strTitle, "WARNING")) icon   = nfui_nfimage_new(IMG_ERROR_ICON);
	else if(!strcmp(strTitle, "ERROR"))   icon   = nfui_nfimage_new(IMG_ERROR_ICON);

	if(icon) {
		nfui_nfobject_show(icon);
		nfui_nffixed_put((NFFIXED*)fixed1, icon, ((NFMBOX_FIXED_WIDTH-64)/2), 0);
	}

	// content
	label = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)label, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(label, NFMBOX_LABEL_WIDTH, NFMBOX_LABEL_HEIGHT);
	nfui_nfobject_use_focus(label, NFOBJECT_FOCUS_OFF);
	nfui_nffixed_put((NFFIXED*)fixed1, label, NFMBOX_LABEL_X, 76);
	nfui_nfobject_show(label);
	nfui_nflabel_set_text((NFLABEL*)label, (gchar*)strContent1);
	nfui_nfobject_set_data(fixed1, "mbox_type", GUINT_TO_POINTER(type));

	size_w = nfutil_string_width(1, NULL, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), strContent1, NORMAL_SPACING);
	check_gap = (NFMBOX_FIXED_WIDTH - size_w)/2;

	check_btn = (NFOBJECT*)nfui_checkbutton_new(*check_ret);
    nfui_check_button_set_skin_type((NFCHECKBUTTON*)check_btn, NFCHECK_TYPE_POPUP_NORMAL);
    nfui_check_get_size((NFCHECKBUTTON*)check_btn, &size_w, &size_h);
	nfui_nfobject_show(check_btn);
	nfui_nffixed_put((NFFIXED*)fixed1, check_btn, check_gap, 76+NFMBOX_LABEL_HEIGHT+8);
	nfui_regi_post_event_callback(check_btn, post_again_check_event_handler);
	nfui_nfobject_set_data(check_btn, "check state", check_ret);

	label = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)label, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(label, NFMBOX_FIXED_WIDTH-check_gap-size_w-4, 27);
	nfui_nfobject_use_focus(label, NFOBJECT_FOCUS_OFF);
	nfui_nffixed_put((NFFIXED*)fixed1, label, check_gap+size_w+4, 76+NFMBOX_LABEL_HEIGHT+8);
	nfui_nfobject_show(label);
	nfui_nflabel_set_text((NFLABEL*)label, (gchar*)strCheck);

	if(type == NFTOOL_MB_OK || type == NFTOOL_MB_OKCANCEL)
	{
		ok_btn = nftool_normal_button_create_type1("OK", 192);
		nfui_nfobject_show(ok_btn);
		nfui_nffixed_put((NFFIXED*)fixed1, ok_btn, (fixed1->width - 192)/2, fixed1->height-NFMBOX_GAP-(44 + 25));
		nfui_regi_post_event_callback(ok_btn, post_ok_button_event_handler);
	}

	if(type == NFTOOL_MB_CANCEL || type == NFTOOL_MB_OKCANCEL)
	{
		cancel_btn = nftool_normal_button_create_type1("CANCEL", 192);
		nfui_nfobject_show(cancel_btn);
		nfui_nffixed_put((NFFIXED*)fixed1, cancel_btn, (fixed1->width - 192)/2, fixed1->height-NFMBOX_GAP-(44 + 25));
		nfui_regi_post_event_callback(cancel_btn, post_cancel_button_event_handler);
	}

	if(type == NFTOOL_MB_OKCANCEL)
	{
		nfui_nfobject_move(ok_btn, fixed1->width/2 - 192 - 3, fixed1->height-NFMBOX_GAP-(44 + 25));
		nfui_nfobject_move(cancel_btn, fixed1->width/2 + 3, fixed1->height-NFMBOX_GAP-(44 + 25));
	}

	nfui_nfobject_show(mb_wnd);

	nfui_page_close(PGID_POPUPWND, mb_wnd);
	nfui_page_open(PGID_MESSAGEBOX, mb_wnd, nfui_get_last_user());

	nfui_make_key_hierarchy((NFWINDOW*)mb_wnd);

	if(type==NFTOOL_MB_OK || type==NFTOOL_MB_OKCANCEL)
		nfui_set_key_focus(ok_btn, TRUE);
	else if(type==NFTOOL_MB_CANCEL)
		nfui_set_key_focus(cancel_btn, TRUE);

	if(type == NFTOOL_MB_OK)
		ret_value = NFTOOL_MB_OK;
	else
		ret_value = NFTOOL_MB_CANCEL;

	gtk_main();

	nfui_page_close(PGID_MESSAGEBOX, mb_wnd);

	return ret_value;

}

static gboolean post_mbConf_button1_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {
			return FALSE;
		}

		top  = nfui_nfobject_get_top(obj);
		ret_value = 0;

		nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean post_mbConf_button2_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {
			return FALSE;
		}

		top  = nfui_nfobject_get_top(obj);
		ret_value = 1;

		nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gint prvMBoxConf_new(NFWINDOW *parent, NFOBJECT** wnd, MBOXCONF *mbConf, mb_type type)
{
#if 0
	NFOBJECT *mb_wnd = NULL;
	NFOBJECT *mb_fixed = NULL;
	NFOBJECT *fixed1 = NULL;
	NFOBJECT *mb_btn = NULL;
	NFOBJECT *mb_lb = NULL;

	GdkPixbuf *btn_image[NFOBJECT_STATE_COUNT];
//	GdkPixbuf**btn_image = NULL;
	GdkDrawable *drawable = NULL;
	const guint btn_font_color[NFOBJECT_STATE_COUNT] = {100, 101, 103, 104};
	guint mbox_h = 0;
	guint mbox_fixed_h = 0;
	guint mbox_lb_h = 0;
	guint str_w = 0;
	guint btn_w=0, btn_h=0;
	guint tmp1 = 10;
	gint i = 0;

	ret_value = NFTOOL_MB_CANCEL;

//	btn_image = (GdkPixbuf**)(imalloc(sizeof(GdkPixbuf*)*NFOBJECT_STATE_COUNT));
//	memset(btn_image, 0x00, sizeof(GdkPixbuf*)*NFOBJECT_STATE_COUNT);

#if defined(__CBC_UI__) || defined(__ITX_UI__)
	btn_image[0] = nfui_get_image_from_file((IMG_BTNBG1), NULL);
	btn_image[1] = nfui_get_image_from_file((IMG_BTNBG2), NULL);
	btn_image[2] = nfui_get_image_from_file((IMG_BTNBG3), NULL);
	btn_image[3] = nfui_get_image_from_file((IMG_BTNBG4), NULL);

	nfui_get_image_size(IMG_BTNBG1, &btn_w, &btn_h);
#else
	btn_image[0] = nfui_get_image_from_file((IMG_BTN1), NULL);
	btn_image[1] = nfui_get_image_from_file((IMG_BTN2), NULL);
	btn_image[2] = nfui_get_image_from_file((IMG_BTN3), NULL);
	btn_image[3] = nfui_get_image_from_file((IMG_BTN4), NULL);

	nfui_get_image_size(IMG_BTN1, &btn_w, &btn_h);
#endif

	mbox_lb_h = (NFMBOX_LABEL_HEIGHT * mbConf->lines) + 20;				// 20: gap of top & bottom
	mbox_fixed_h = (mbox_lb_h + (NFMBOX_GAP * 2) + btn_h);
	mbox_h = (NFMBOX_HEAD_SIZE + ((NFMBOX_BORDER + NFMBOX_GAP) * 2) + mbox_fixed_h);

	if(NFMBOX_HEIGHT > mbox_h) {
		mbox_fixed_h = NFMBOX_FIXED_HEIGHT;
		mbox_h = NFMBOX_HEIGHT;
	}

	/* window */
	mb_wnd = nftool_create_popup_window(parent, NFMBOX_X, NFMBOX_Y, NFMBOX_WIDTH, mbox_h, mbConf->title, mbConf->add_close);
	mb_fixed = ((NFWINDOW*)mb_wnd)->child;
	nfui_nfwindow_set_title(mb_wnd, "CONF MESSAGE BOX");
	nfui_nfobject_show(mb_fixed);

#if defined(__BTYPE_UI__) || defined(__SAMSUNG_UI__) || defined(__NF_700_UI__)
	nfui_regi_post_event_callback(mb_fixed, post_mbox_fixed_event_handler);
#if !defined(__NF_700_UI__)
	if(MBoxTitle_bg == NULL)
		MBoxTitle_bg = nfui_get_image_from_file((IMG_POPUP_MSG_TITLE_BG), NULL);
#endif
	memset(MBoxTitle, 0x00, sizeof(MBoxTitle));
	memcpy(MBoxTitle, mbConf->title, sizeof(MBoxTitle));
#endif

	/* fixed */
	fixed1 = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(fixed1, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_set_size(fixed1, NFMBOX_FIXED_WIDTH, mbox_fixed_h);
	nfui_nfobject_show(fixed1);
	nfui_nffixed_put((NFFIXED*)mb_fixed, fixed1, NFMBOX_FIXED_X, NFMBOX_FIXED_Y);
	nfui_regi_pre_event_callback(fixed1, pre_cont_fixed_event_handler);

	nfui_nfobject_set_data(fixed1, "mbox_type", GUINT_TO_POINTER(type));

	tmp1 = ((mbox_fixed_h/2) - (mbConf->lines * NFMBOX_LABEL_HEIGHT)/2) - btn_h;

	/* label */
	for(i=0; i<mbConf->lines; i++)
	{
		mb_lb = (NFOBJECT*)nfui_nflabel_new_with_pango_font(mbConf->contents[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
		nfui_nfobject_set_size(mb_lb, NFMBOX_LABEL_WIDTH, NFMBOX_LABEL_HEIGHT);
		if(mbConf->align != NFALIGN_CENTER)
			nfui_nflabel_set_align((NFLABEL *)mb_lb, mbConf->align, 4);
		nfui_nfobject_use_focus(mb_lb, NFOBJECT_FOCUS_OFF);
		nfui_nffixed_put((NFFIXED*)fixed1, mb_lb, NFMBOX_LABEL_X, tmp1);
		tmp1 += NFMBOX_LABEL_HEIGHT;

		if(drawable == NULL)
			drawable = nfui_nfobject_get_window(mb_lb);

		str_w = nfutil_string_width(1, drawable, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), mbConf->contents[i], NORMAL_SPACING);
		if(str_w > NFMBOX_LABEL_WIDTH)
		{
			nfui_nflabel_set_spacing((NFLABEL*)mb_lb,  CONDENSED_SPACING);
		}
		nfui_nfobject_show(mb_lb);

#if defined(__SAMSUNG_UI__)
		nfui_nflabel_set_drawing_outline((NFLABEL*)mb_lb, FALSE);
#endif
	}

	/* button */
	if(type == NFTOOL_MB_CONF_1)
	{
		mb_btn = (NFOBJECT*)nfui_nfbutton_new_with_param(btn_image, mbConf->buttonStr[0]);
		nfui_nfbutton_set_pango_font((NFBUTTON*)mb_btn, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), (guint*)btn_font_color);
		nfui_nfobject_set_size(mb_btn, btn_w, btn_h);
		nfui_nfobject_show(mb_btn);

		nfui_nffixed_put((NFFIXED*)fixed1, mb_btn, (fixed1->width - btn_w)/2, fixed1->height-NFMBOX_GAP-btn_h);

		nfui_regi_post_event_callback(mb_btn, post_mbConf_button1_event_handler);
	}
	else if(type == NFTOOL_MB_CONF_2)
	{
		for(i=1; i>=0; i--) {
			mb_btn = (NFOBJECT*)nfui_nfbutton_new_with_param(btn_image, mbConf->buttonStr[i]);
			nfui_nfbutton_set_pango_font((NFBUTTON*)mb_btn, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), (guint*)btn_font_color);
			nfui_nfobject_set_size(mb_btn, btn_w, btn_h);
			nfui_nfobject_show(mb_btn);

			if(i == 0) {
				nfui_regi_post_event_callback(mb_btn, post_mbConf_button1_event_handler);
				nfui_nffixed_put((NFFIXED*)fixed1, mb_btn, (fixed1->width-NFMBOX_GAP)/2 - btn_w, fixed1->height-NFMBOX_GAP-btn_h);
			}else if(i == 1) {
				nfui_regi_post_event_callback(mb_btn, post_mbConf_button2_event_handler);
				nfui_nffixed_put((NFFIXED*)fixed1, mb_btn, (fixed1->width+NFMBOX_GAP)/2, fixed1->height-NFMBOX_GAP-btn_h);
			}
		}
	}


//	nfui_nfobject_set_data(fixed1, "button_image", (gpointer)btn_image);

	nfui_nfobject_show(mb_wnd);

	nfui_page_close(PGID_POPUPWND, mb_wnd);
	nfui_page_open(PGID_MESSAGEBOX, mb_wnd, nfui_get_last_user());

	nfui_make_key_hierarchy((NFWINDOW*)mb_wnd);

	nfui_set_key_focus(mb_btn, TRUE);

	gtk_main();

	nfui_page_close(PGID_MESSAGEBOX, mb_wnd);

	return ret_value;
#endif
//captainnn
//#ifdef	SUPPORT_SMART_SEARCH
	NFOBJECT *mb_wnd = NULL;
	NFOBJECT *mb_fixed = NULL;
	NFOBJECT *fixed1 = NULL;
	NFOBJECT *mb_btn = NULL;
	NFOBJECT *mb_lb = NULL;

	GdkDrawable *drawable = NULL;
	guint mbox_h = 0;
	guint mbox_fixed_h = 0;
	guint mbox_lb_h = 0;
	guint str_w = 0;
	guint tmp1 = 10;
	gint i;

	ret_value = NFTOOL_MB_CANCEL;

	mbox_lb_h = (NFMBOX_LABEL_HEIGHT * mbConf->lines) + 20;				// 20: gap of top & bottom
	mbox_fixed_h = (mbox_lb_h + (NFMBOX_GAP * 2) + 44 + 25);
	mbox_h = (NFMBOX_HEAD_SIZE + ((NFMBOX_BORDER + NFMBOX_GAP) * 2) + mbox_fixed_h);

	if(NFMBOX_HEIGHT > mbox_h) {
		mbox_fixed_h = NFMBOX_FIXED_HEIGHT;
		mbox_h = NFMBOX_HEIGHT;
	}

	/* window */
	mb_wnd = nftool_create_popup_window(parent, NFMBOX_X, NFMBOX_Y, NFMBOX_WIDTH, mbox_h, mbConf->title, mbConf->add_close);
	mb_fixed = ((NFWINDOW*)mb_wnd)->child;
	nfui_nfobject_modify_bg(mb_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfwindow_set_title((NFWINDOW*)mb_wnd, "CONF MESSAGE BOX");
	nfui_nfobject_show(mb_fixed);

#if defined(__BTYPE_UI__) || defined(__SAMSUNG_UI__) || defined(__NF_700_UI__)
	nfui_regi_post_event_callback(mb_fixed, post_mbox_fixed_event_handler);
#if !defined(__NF_700_UI__)
	if(MBoxTitle_bg == NULL)
		MBoxTitle_bg = nfui_get_image_from_file((IMG_POPUP_MSG_TITLE_BG), NULL);
#endif
	memset(MBoxTitle, 0x00, sizeof(MBoxTitle));
	memcpy(MBoxTitle, mbConf->title, sizeof(MBoxTitle));
#endif

	/* fixed */
	fixed1 = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed1, (gint)NFMBOX_FIXED_WIDTH, (gint)mbox_fixed_h);
	nfui_nfobject_show(fixed1);
	nfui_nffixed_put((NFFIXED*)mb_fixed, fixed1, NFMBOX_FIXED_X, NFMBOX_FIXED_Y);
	nfui_regi_pre_event_callback(fixed1, pre_cont_fixed_event_handler);

	NFOBJECT *icon = (NFOBJECT *)nfui_nfimage_new(IMG_QUESTION_ICON);
	if(icon) {
		nfui_nfobject_show(icon);
		nfui_nffixed_put((NFFIXED*)fixed1, icon, ((NFMBOX_FIXED_WIDTH-64)/2), 0);
	}

	nfui_nfobject_set_data(fixed1, "mbox_type", GUINT_TO_POINTER(type));

	tmp1 = 76;

	/* label */
	for(i=0; i<(int)mbConf->lines; i++)
	{
		mb_lb = (NFOBJECT*)nfui_nflabel_new_with_pango_font(mbConf->contents[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
		nfui_nfobject_set_size(mb_lb, (gint)NFMBOX_LABEL_WIDTH, (gint)NFMBOX_LABEL_HEIGHT);
		if(mbConf->align != NFALIGN_CENTER)
			nfui_nflabel_set_align((NFLABEL *)mb_lb, mbConf->align, 4);
		nfui_nfobject_use_focus(mb_lb, NFOBJECT_FOCUS_OFF);
		nfui_nffixed_put((NFFIXED*)fixed1, mb_lb, NFMBOX_LABEL_X, tmp1);
		tmp1 += NFMBOX_LABEL_HEIGHT;

		if(drawable == NULL)
			drawable = nfui_nfobject_get_window(mb_lb);

		str_w = nfutil_string_width(1, drawable, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), mbConf->contents[i], NORMAL_SPACING);
		if(str_w > NFMBOX_LABEL_WIDTH)
		{
			nfui_nflabel_set_spacing((NFLABEL*)mb_lb,  CONDENSED_SPACING);
		}
		nfui_nfobject_show(mb_lb);

#if defined(__SAMSUNG_UI__)
		nfui_nflabel_set_drawing_outline((NFLABEL*)mb_lb, FALSE);
#endif
	}

	/* button */
	if(type == NFTOOL_MB_CONF_1)
	{
		mb_btn = nftool_normal_button_create_type1(mbConf->buttonStr[0], 192);
		nfui_nfobject_show(mb_btn);

		nfui_nffixed_put((NFFIXED*)fixed1, mb_btn, (guint)(fixed1->width - 192)/2, (guint)fixed1->height-NFMBOX_GAP-(44 + 25));

		nfui_regi_post_event_callback(mb_btn, post_mbConf_button1_event_handler);
	}
	else if(type == NFTOOL_MB_CONF_2)
	{
		mb_btn = nftool_normal_button_create_type1(mbConf->buttonStr[0], 192);
		nfui_nfobject_show(mb_btn);
		nfui_regi_post_event_callback(mb_btn, post_mbConf_button1_event_handler);
		nfui_nffixed_put((NFFIXED*)fixed1, mb_btn,
				(guint)fixed1->width/2 - 192 - 3, (guint)fixed1->height-NFMBOX_GAP-(44 + 25));

		mb_btn = nftool_normal_button_create_type1(mbConf->buttonStr[1], 192);
		nfui_nfobject_show(mb_btn);
		nfui_regi_post_event_callback(mb_btn, post_mbConf_button2_event_handler);
		nfui_nffixed_put((NFFIXED*)fixed1, mb_btn,
				(guint)fixed1->width/2 + 3, (guint)fixed1->height-NFMBOX_GAP-(44 + 25));
	}

	nfui_nfobject_show(mb_wnd);

	nfui_page_close(PGID_POPUPWND, mb_wnd);
	nfui_page_open(PGID_MESSAGEBOX, mb_wnd, nfui_get_last_user());

	nfui_make_key_hierarchy((NFWINDOW*)mb_wnd);

	nfui_set_key_focus(mb_btn, TRUE);

	gtk_main();

	nfui_page_close(PGID_MESSAGEBOX, mb_wnd);

	return ret_value;
//#endif
}


static gboolean remove_mbox(gpointer data)
{
	NFOBJECT *obj = NULL;

	obj = (NFOBJECT*)data;

	NFUTIL_THREADS_ENTER();

	nfui_nfobject_destroy(obj);
	ssm_fakestart_auto_logout();

	NFUTIL_THREADS_LEAVE();

	return FALSE;
}

NFOBJECT* nftool_mbox_auto(NFWINDOW *parent, gint sec, const gchar *strTitle, const gchar *strCont1)
{
	NFOBJECT *ret=NULL;

	ssm_fakestop_auto_logout();
	prvMBox_new(parent, &ret, strTitle, strCont1, NFTOOL_MB_NONE);

	g_timeout_add(sec*1000, remove_mbox, ret);

	return ret;
}

NFOBJECT* nftool_mbox_sleep_auto(NFWINDOW *parent, gint sec, const gchar *strTitle, const gchar *strCont1)
{
    NFOBJECT *ret=NULL;

    rm_mbox_sec = sec;

    ssm_fakestop_auto_logout();
    prvMBox_new(parent, &ret, strTitle, strCont1, NFTOOL_MB_SLEEP_AUTO);

    return ret;
}

NFOBJECT* nftool_mbox_auto_ok(NFWINDOW *parent, gint sec, const gchar *strTitle, const gchar *strCont1)
{
	NFOBJECT *ret=NULL;

	rm_mbox_sec = sec;

	ssm_fakestop_auto_logout();
	prvMBox_new(parent, &ret, strTitle, strCont1, NFTOOL_MB_AUTO_OK);

	return ret;
}

NFOBJECT* nftool_mboxEx_auto_ok(NFWINDOW *parent, gint sec, const gchar *strTitle, const gchar *strCont1)
{
    NFOBJECT *ret=NULL;

    rm_mbox_sec = sec;

    ssm_fakestop_auto_logout();
    prvMBoxEx_new(parent, &ret, strTitle, strCont1, NFTOOL_MB_AUTO_OK);

    return ret;
}

NFOBJECT* nftool_mboxEx_ok(NFWINDOW *parent, gint sec, const gchar *strTitle, const gchar *strCont1)
{
    NFOBJECT *ret=NULL;

//    rm_mbox_sec = sec;

    ssm_fakestop_auto_logout();
    prvMBoxEx_new(parent, &ret, strTitle, strCont1, NFTOOL_MB_OK);

    return ret;
}

mb_type nftool_mbox_auto_okcancel(NFWINDOW *parent, gint sec, const gchar *strTitle, const gchar *strCont1)
{
	mb_type rval;
	NFOBJECT *ret=NULL;

	rm_mbox_sec = sec;

	ssm_fakestop_auto_logout();
	rval = prvMBox_new(parent, &ret, strTitle, strCont1, NFTOOL_MB_AUTO_OKCANCEL);

	return rval;
}

mb_type nftool_mbox2_auto_okcancel(NFWINDOW *parent, gint sec, const gchar *strTitle, const gchar *strCont1,  const gchar *strCont2, const gchar *strCont3, const gchar *strCont4)
{
	mb_type rval;
	NFOBJECT *ret=NULL;

	rm_mbox_sec = sec;

	ssm_fakestop_auto_logout();
	rval = prvMBox2_new(parent, &ret, strTitle, strCont1, strCont2, strCont3, strCont4, NFTOOL_MB_AUTO_OKCANCEL);

	return rval;
}

NFOBJECT* nftool_mbox_wait(NFWINDOW *parent, const gchar *strTitle, const gchar *strCont1)
{
	NFOBJECT *ret=NULL;

	ssm_fakestop_auto_logout();
	prvMBox_new(parent, &ret, strTitle, strCont1, NFTOOL_MB_NONE);
	nfui_func_start(FUNCID_WAIT_POPUP, ret);

	return ret;
}

void nftool_remove_waitbox(NFOBJECT *wait_box)
{
	nfui_func_end(FUNCID_WAIT_POPUP, wait_box);
	nfui_nfobject_destroy(wait_box);
	ssm_fakestart_auto_logout();
}

NFOBJECT* nftool_mbox_wait_with_graph(NFWINDOW *parent, const gchar *strTitle, const gchar *strCont1, const gchar *strCont2)
{
    NFOBJECT *ret = NULL;

    ssm_fakestop_auto_logout();
    prvMBox_graph_new(parent, &ret, strTitle, strCont1, strCont2, NFTOOL_MB_NONE);
    nfui_func_start(FUNCID_WAIT_POPUP, ret);

    return ret;
}

mb_type nftool_mbox(NFWINDOW *parent, const gchar *strTitle, const gchar *strContent, mb_type type)
{
	return prvMBox_new(parent, NULL, strTitle, strContent, type);
}

mb_type nftool_mbox_handle(NFWINDOW *parent, const gchar *strTitle, const gchar *strContent, mb_type type, NFOBJECT **mboxwnd)
{
	return prvMBox_new2(parent, NULL, strTitle, strContent, type, mboxwnd);
}

mb_type nftool_mbox_4_line(NFWINDOW *parent, const gchar *strTitle, const gchar *strContent1, const gchar *strContent2, const gchar *strContent3, const gchar *strContent4, mb_type type)
{
	return prvMBox1_new(parent, NULL, strTitle, strContent1, strContent2, strContent3, strContent4, type);
}

gint nftool_mbox_by_conf(NFWINDOW *parent, MBOXCONF *mbConf, mb_type type)
{
	return prvMBoxConf_new(parent, NULL, mbConf, type);
}

///////////////////////////////////////////////////////////////
//
// Newely added by SKSHIN
//

NFOBJECT* vw_mbox_auto(NFWINDOW *parent, gint sec, const gchar *strTitle, IMBXSTR_E strid)
{
	NFOBJECT *ret=NULL;

	g_assert(strid < IMBX_MAX);
	strid = strid < vw_get_count_mbxstr() ? strid : 0;
	prvMBox_new(parent, &ret, strTitle, g_msg_str[strid], NFTOOL_MB_NONE);

	g_timeout_add(sec*1000, remove_mbox, ret);

	return ret;
}

NFOBJECT* vw_mbox_auto_ok(NFWINDOW *parent, gint sec, const gchar *strTitle, IMBXSTR_E strid)
{
	NFOBJECT *ret=NULL;

	rm_mbox_sec = sec;

	g_assert(strid < IMBX_MAX);
	strid = strid < vw_get_count_mbxstr() ? strid : 0;
	prvMBox_new(parent, &ret, strTitle, g_msg_str[strid], NFTOOL_MB_AUTO_OK);

	return ret;
}

mb_type vw_mbox_auto_okcancel(NFWINDOW *parent, gint sec, const gchar *strTitle, IMBXSTR_E strid)
{
	mb_type rval;
	NFOBJECT *ret=NULL;

	rm_mbox_sec = sec;

	g_assert(strid < IMBX_MAX);
	strid = strid < vw_get_count_mbxstr() ? strid : 0;
	rval = prvMBox_new(parent, &ret, strTitle, g_msg_str[strid], NFTOOL_MB_AUTO_OKCANCEL);

	return rval;
}

NFOBJECT* vw_mbox_wait(NFWINDOW *parent, const gchar *strTitle, IMBXSTR_E strid)
{
	NFOBJECT *ret=NULL;

	g_assert(strid < IMBX_MAX);
	strid = strid < vw_get_count_mbxstr() ? strid : 0;
	prvMBox_new(parent, &ret, strTitle, g_msg_str[strid], NFTOOL_MB_NONE);
	nfui_func_start(FUNCID_WAIT_POPUP, ret);

	return ret;
}

void vw_remove_waitbox(NFOBJECT *wait_box)
{
	nfui_func_end(FUNCID_WAIT_POPUP, wait_box);
	nfui_nfobject_destroy(wait_box);
}

mb_type vw_mbox(NFWINDOW *parent, const gchar *strTitle, IMBXSTR_E strid, mb_type type)
{
	g_assert(strid < IMBX_MAX);
	strid = strid < vw_get_count_mbxstr() ? strid : 0;
	return prvMBox_new(parent, NULL, strTitle, g_msg_str[strid], type);
}

mb_type vw_mbox_4_line(NFWINDOW *parent, const gchar *strTitle, IMBXSTR_E strid1, IMBXSTR_E strid2, IMBXSTR_E strid3, IMBXSTR_E strid4, mb_type type)
{
	g_assert(strid1 < IMBX_MAX);
	g_assert(strid2 < IMBX_MAX);
	g_assert(strid3 < IMBX_MAX);
	g_assert(strid4 < IMBX_MAX);
	strid1 = strid1 < vw_get_count_mbxstr() ? strid1 : 0;
	strid2 = strid2 < vw_get_count_mbxstr() ? strid2 : 0;
	strid3 = strid3 < vw_get_count_mbxstr() ? strid3 : 0;
	strid4 = strid4 < vw_get_count_mbxstr() ? strid4 : 0;
	return prvMBox1_new(parent, NULL, strTitle, g_msg_str[strid1], g_msg_str[strid2], g_msg_str[strid3], g_msg_str[strid4], type);
}

mb_type vw_mbox_check(NFWINDOW *parent, const gchar *strTitle, IMBXSTR_E strid, IMBXSTR_E str_check, gint *check_ret, mb_type type)
{
	g_assert(strid < IMBX_MAX);
	g_assert(str_check < IMBX_MAX);
	strid = strid < vw_get_count_mbxstr() ? strid : 0;
	str_check = str_check < vw_get_count_mbxstr() ? str_check : 0;

	g_message("%s, %d", __FUNCTION__, __LINE__);

	return prvMBox_check_new(parent, NULL, strTitle, g_msg_str[strid], g_msg_str[str_check], check_ret, type);
}


/*************************************************************
 * PROGRESS BAR POPUP
 * ***********************************************************/

#define	NFPROG_POP_BASE_HEIGHT	(guint)(DISPLAY_IS_D1 ? 74:74)
#define	NFPROG_POP_FIXED_LEFT	(guint)(DISPLAY_IS_D1 ? 6:10)
#define	NFPROG_POP_FIXED_TOP	(guint)(DISPLAY_IS_D1 ? 54:54)


NFOBJECT* nftool_prog_pop_open(NFWINDOW *parent, const gchar *strTitle, gboolean show_num, guint x, guint y, guint width)
{
	NFOBJECT* prog_wnd;
	NFOBJECT* prog_fixed;
	NFOBJECT* prog;

	GdkPixbuf *prog_img[4];
	guint img_width;
	guint img_height;
	guint win_w;

	NFOBJECT* label;

	prog_img[0] = nfui_get_image_from_file((IMG_PROGRESS_BG), NULL);
	prog_img[1] = nfui_get_image_from_file((IMG_PROGRESS_HEAD), NULL);
	prog_img[2] = nfui_get_image_from_file((IMG_PROGRESS_MIDDLE), NULL);
	prog_img[3] = nfui_get_image_from_file((IMG_PROGRESS_TAIL), NULL);

	prog = (NFOBJECT*)nfui_nfprogressbar_new_with_images(prog_img[0], prog_img[1], prog_img[2], prog_img[3]);
	nfui_nfobject_show(prog);

	nfui_get_image_size(IMG_PROGRESS_BG, &img_width, &img_height);
	win_w = img_width + (NFPROG_POP_FIXED_LEFT * 3);

	if(show_num == TRUE){
		label = (NFOBJECT*)nfui_nflabel_new_with_pango_font("0%", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
		nfui_nfobject_use_focus(label, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_set_size(label, win_w-(NFPROG_POP_FIXED_LEFT*2), 24);

		nfui_nfobject_modify_bg(label, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
		nfui_nfobject_show(label);
	}

	if(show_num == TRUE)
		prog_wnd = nftool_create_popup_window(parent, x, y, win_w, NFPROG_POP_BASE_HEIGHT+img_height+20, strTitle, FALSE);
	else
		prog_wnd = nftool_create_popup_window(parent, x, y, win_w, NFPROG_POP_BASE_HEIGHT+img_height, strTitle, FALSE);

	prog_fixed = ((NFWINDOW*)prog_wnd)->child;
	nfui_nfobject_modify_bg(prog_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(prog_fixed);

	nfui_nfobject_set_data(prog_wnd, "progress_bar", prog);

	if(show_num == TRUE){
		nfui_nfobject_set_data(prog_wnd, "progress_label", label);
		nfui_nffixed_put((NFFIXED*)prog_fixed, prog, NFPROG_POP_FIXED_LEFT, NFPROG_POP_FIXED_TOP+22);
		nfui_nffixed_put((NFFIXED*)prog_fixed, label, NFPROG_POP_FIXED_LEFT, NFPROG_POP_FIXED_TOP-6);
	}
	else
		nfui_nffixed_put((NFFIXED*)prog_fixed, prog, NFPROG_POP_FIXED_LEFT, NFPROG_POP_FIXED_TOP);

	nfui_nfobject_show(prog_wnd);

	return prog_wnd;
}

void nftool_prog_pop_set_rate(NFOBJECT* prog_pop, guint rate)
{
	NFOBJECT* prog;
	NFOBJECT* label;
	gchar temp[10];

	prog = (NFOBJECT*)nfui_nfobject_get_data(prog_pop, "progress_bar");
	if(rate == nfui_nfprogressbar_get_rate((NFPROGRESSBAR*) prog))
		return;

	nfui_nfprogressbar_set_rate((NFPROGRESSBAR*)prog, rate);

	label = (NFOBJECT*)nfui_nfobject_get_data(prog_pop, "progress_label");
	if(label){
		g_sprintf(temp, "%d%%", rate);
		nfui_nflabel_set_text((NFLABEL*)label, temp);
		nfui_signal_emit(label, GDK_EXPOSE, FALSE);
	}

	nfui_signal_emit(prog, GDK_EXPOSE, FALSE);
}

void nftool_prog_pop_close(NFOBJECT* prog_pop)
{
	nfui_nfobject_destroy(prog_pop);
}




/**************************************************************
 * FOR MOUSE CURSOR
 * ***********************************************************/

#define	MCURSOR_WIDTH		(DISPLAY_IS_D1 ? 16:32)
#define	MCURSOR_HEIGHT		(DISPLAY_IS_D1 ? 20:29)

static const guchar srcData_4D1[NF_CURSOR_COUNTS][116] = {
	{																// arrow
		0xfc,	0xff,	0xff,	0xff,
		0xe2,	0xff,	0xff,	0xff,
		0x1d,	0xff,	0xff,	0xff,
		0xfd,	0xf8,	0xff,	0xff,
		0xfd,	0xc7,	0xff,	0xff,
		0xfb,	0x3f,	0xfe,	0xff,
		0xfb,	0xff,	0xf1,	0xff,
		0xfb,	0xff,	0x8f,	0xff,
		0xf7,	0xff,	0x7f,	0xfe,
		0xf7,	0xff,	0x07,	0xfe,
		0xf7,	0x1f,	0xf8,	0xff,
		0xef,	0x7f,	0xfe,	0xff,
		0xef,	0xff,	0xf9,	0xff,
		0xef,	0xfb,	0xe7,	0xff,
		0xdf,	0xfb,	0x9f,	0xff,
		0xdf,	0xf3,	0x7f,	0xfe,
		0xdf,	0xf3,	0xff,	0xf9,
		0xbf,	0xeb,	0xff,	0xe7,
		0xbf,	0xeb,	0xff,	0xf7,
		0xbf,	0xdd,	0xff,	0xfb,
		0x7f,	0xdd,	0xff,	0xfd,
		0x7f,	0xbd,	0xff,	0xfe,
		0x7f,	0xbd,	0x7f,	0xff,
		0xff,	0x7c,	0xbf,	0xff,
		0xff,	0x7c,	0xdf,	0xff,
		0xff,	0xff,	0xee,	0xff,
		0xff,	0xff,	0xf6,	0xff,
		0xff,	0xff,	0xf9,	0xff,
		0xff,	0xff,	0xfd,	0xff
	},
	{
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x80, 	0x01, 	0x00,
		0x00, 	0xc0, 	0x03, 	0x00,
		0x00, 	0xe0, 	0x07, 	0x00,
		0x00, 	0xf0, 	0x0f, 	0x00,
		0x00, 	0xf8, 	0x1f, 	0x00,
		0x00, 	0xfc, 	0x3f, 	0x00,
		0x00, 	0xfe, 	0x7f, 	0x00,
		0x00, 	0x81, 	0x81, 	0x00,
		0x80, 	0x81, 	0x81, 	0x01,
		0xc0, 	0x81, 	0x81, 	0x03,
		0xe0, 	0x81, 	0x81, 	0x07,
		0xf0, 	0x81, 	0x81, 	0x0f,
		0xf8, 	0x81, 	0x81, 	0x1f,
		0xfc, 	0xff, 	0xff, 	0x3f,
		0xf8, 	0x81, 	0x81, 	0x1f,
		0xf0, 	0x81, 	0x81, 	0x0f,
		0xe0, 	0x81, 	0x81, 	0x07,
		0xc0, 	0x81, 	0x81, 	0x03,
		0x80, 	0x81, 	0x81, 	0x01,
		0x00, 	0x81, 	0x81, 	0x00,
		0x00, 	0xfe, 	0x7f, 	0x00,
		0x00, 	0xfc, 	0x3f, 	0xff,
		0x00, 	0xf8, 	0x1f, 	0x00,
		0x00, 	0xf0, 	0x0f, 	0x00,
		0x00, 	0xe0, 	0x07, 	0x00,
		0x00, 	0xc0, 	0x03, 	0x00,
		0x00, 	0x80, 	0x01, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00
	}
#ifdef _CURSOR_DISPLAY_ONOFF //nskim display on/off cursor
	,{
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
	}
#endif

};

static const guchar maskData_4D1[NF_CURSOR_COUNTS][116] = {
	{																// arrow
		0x03,	0x00,	0x00,	0x00,
		0x1f,	0x00,	0x00,	0x00,
		0xfe,	0x00,	0x00,	0x00,
		0xfe,	0x07,	0x00,	0x00,
		0xfe,	0x3f,	0x00,	0x00,
		0xfc,	0xff,	0x01,	0x00,
		0xfc,	0xff,	0x0f,	0x00,
		0xfc,	0xff,	0x7f,	0x00,
		0xf8,	0xff,	0xff,	0x01,
		0xf8,	0xff,	0xff,	0x01,
		0xf8,	0xff,	0x07,	0x00,
		0xf0,	0xff,	0x01,	0x00,
		0xf0,	0xff,	0x07,	0x00,
		0xf0,	0xff,	0x1f,	0x00,
		0xe0,	0xff,	0x7f,	0x00,
		0xe0,	0xff,	0xff,	0x01,
		0xe0,	0xff,	0xff,	0x07,
		0xc0,	0xf7,	0xff,	0x1f,
		0xc0,	0xf7,	0xff,	0x0f,
		0xc0,	0xe3,	0xff,	0x07,
		0x80,	0xe3,	0xff,	0x03,
		0x80,	0xc3,	0xff,	0x01,
		0x80,	0xc3,	0xff,	0x00,
		0x00,	0x83,	0x7f,	0x00,
		0x00,	0x83,	0x3f,	0x00,
		0x00,	0x00,	0x1f,	0x00,
		0x00,	0x00,	0x0f,	0x00,
		0x00,	0x00,	0x06,	0x00,
		0x00,	0x00,	0x04,	0x00,
	},
	{
		0x00, 	0x81, 	0x01, 	0x00,
		0x00, 	0xc0, 	0x03, 	0x00,
		0x00, 	0xe0, 	0x07, 	0x00,
		0x00, 	0xf0, 	0x0f, 	0x00,
		0x00, 	0xf8, 	0x1f, 	0x00,
		0x00, 	0xfc, 	0x3f, 	0x00,
		0x00, 	0xfe, 	0x7f, 	0x00,
		0x00, 	0xff, 	0xff, 	0x00,
		0x80, 	0xff, 	0xff, 	0x01,
		0xc0, 	0xc3, 	0xc3, 	0x03,
		0xe0, 	0xc3, 	0xc3, 	0x07,
		0xf0, 	0xc3, 	0xc3, 	0x0f,
		0xf8, 	0xc3, 	0xc3, 	0x1f,
		0xfc, 	0xff, 	0xff, 	0x3f,
		0xfe, 	0xff, 	0xff, 	0x7f,
		0xfc, 	0xff, 	0xff, 	0x3f,
		0xf8, 	0xc3, 	0xc3, 	0x1f,
		0xf0, 	0xc3, 	0xc3, 	0x0f,
		0xe0, 	0xc3, 	0xc3, 	0x07,
		0xc0, 	0xc3, 	0xc3, 	0x03,
		0x80, 	0xff, 	0xff, 	0x01,
		0x00, 	0xff, 	0xff, 	0x00,
		0x00, 	0xfe, 	0x7f, 	0x00,
		0x00, 	0xfc, 	0x3f, 	0x00,
		0x00, 	0xf8, 	0x1f, 	0x00,
		0x00, 	0xf0, 	0x0f, 	0x00,
		0x00, 	0xe0, 	0x07, 	0x00,
		0x00, 	0xc0, 	0x03, 	0x00,
		0x00, 	0x81, 	0x01, 	0x00,
	}
#ifdef _CURSOR_DISPLAY_ONOFF //nskim display on/off cursor
	,{
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
		0x00, 	0x00, 	0x00, 	0x00,
	}
#endif

};

static const guchar srcData_D1[NF_CURSOR_COUNTS][40] = {
	{																// arrow
		0xFC, 0xFF, 0xFA, 0xFF,
		0xF6, 0xFF, 0xEE, 0xFF,
		0xDE, 0xFF, 0xBE, 0xFF,
		0x7E, 0xFF, 0xFE, 0xFE,
		0xFE, 0xFD, 0xFE, 0xFB,
		0xFE, 0xF7, 0xFE, 0xEF,
		0xFE, 0xDF, 0xFE, 0xBF,
		0x3E, 0x80, 0xDE, 0xFF,
		0xEE, 0xFF, 0xF6, 0xFF,
		0xFA, 0xFF, 0xFC, 0xFF,
	},
	{
		0x00, 0x00, 0x80, 0x01,
		0xc0, 0x03, 0xe0, 0x07,
		0xf0, 0x0f, 0x80, 0x01,
		0x90, 0x09, 0x98, 0x19,
		0x9c, 0x39, 0xfe, 0x7f,
		0xfe, 0x7f, 0x9c, 0x39,
		0x98, 0x19, 0x90, 0x09,
		0x80, 0x01, 0xf0, 0x0f,
		0xe0, 0x07, 0xc0, 0x03,
		0x80, 0x01, 0x00, 0x00
	}
#ifdef _CURSOR_DISPLAY_ONOFF //nskim display on/off cursor
	,{
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00
	}
#endif
};

static const guchar maskData_D1[NF_CURSOR_COUNTS][40] = {
	{																// arrow
		0x03, 0x00, 0x07, 0x00,
		0x0F, 0x00, 0x1F, 0x00,
		0x3F, 0x00, 0x7F, 0x00,
		0xFF, 0x00, 0xFF, 0x01,
		0xFF, 0x03, 0xFF, 0x07,
		0xFF, 0x0F, 0xFF, 0x1F,
		0xFF, 0x3F, 0xFF, 0x7F,
		0xFF, 0x7F, 0x3F, 0x00,
		0x1F, 0x00, 0x0F, 0x00,
		0x07, 0x00, 0x03, 0x00,
	},
	{
		0x80, 0x01, 0xc0, 0x03,
		0xe0, 0x07, 0xf0, 0x0f,
		0xf8, 0x1f, 0xf0, 0x0f,
		0xf8, 0x1f, 0xfc, 0x3f,
		0xfe, 0x7f, 0xff, 0xff,
		0xff, 0xff, 0xfe, 0x7f,
		0xfc, 0x3f, 0xf8, 0x1f,
		0xf0, 0x0f, 0xf8, 0x1f,
		0xf0, 0x0f, 0xe0, 0x07,
		0xc0, 0x03, 0x80, 0x01
	}
#ifdef _CURSOR_DISPLAY_ONOFF //display on/off cursor
   ,{
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00
   }
#endif

};


#if 0 //_ATM_MOUSE_
static guchar invisible_maskData_D1[NF_CURSOR_COUNTS][40] = {
  {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
  },
  {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
  }
};
#endif





#if 0 //_ATM_MOUSE_
void nftool_set_custom_cursor(GdkWindow *wnd, gboolean mouse_connect_flag)
{
	GdkPixmap *src, *mask;
	GdkCursor *mcursor;
	GdkColor fg = colors[COLOR_IDX(TODO_COLOR)]; /* White. */
	GdkColor bg = colors[COLOR_IDX(TODO_COLOR)]; /* Black. */
	guchar *srcData = NULL, *maskData = NULL, *invisible_maskData = NULL;

	guint i;

	if(DISPLAY_IS_D1)
	{
		srcData = (guchar*)srcData_D1[NF_CURSOR_ARROW];
		maskData = (guchar*)maskData_D1[NF_CURSOR_ARROW];
		invisible_maskData = (guchar*)invisible_maskData_D1[NF_CURSOR_ARROW];
	}
	else
	{
		srcData = (guchar*)srcData_4D1[NF_CURSOR_ARROW];
		maskData = (guchar*)maskData_4D1[NF_CURSOR_ARROW];
		invisible_maskData = (guchar*)maskData_4D1[NF_CURSOR_ARROW];
	}

	src = gdk_bitmap_create_from_data(NULL, srcData, MCURSOR_WIDTH, MCURSOR_HEIGHT);

  g_print("<%s> mouse_connect_flag : %d\n", __FUNCTION__, mouse_connect_flag);
	if(mouse_connect_flag)
	{

	  mask = gdk_bitmap_create_from_data(NULL, maskData, MCURSOR_WIDTH, MCURSOR_HEIGHT);
  }
  else
  {
	  mask = gdk_bitmap_create_from_data(NULL, invisible_maskData, MCURSOR_WIDTH, MCURSOR_HEIGHT);
  }
	mcursor = gdk_cursor_new_from_pixmap (src, mask, &fg, &bg, 0, 0);

	gdk_pixmap_unref (src);
	gdk_pixmap_unref (mask);

	gdk_window_set_cursor(wnd, mcursor);

}
#else
#if defined(_CURSOR_DISPLAY_ONOFF) //nskim display on/off cursor
extern int mouse_connected_flag;
extern int hid_joystick_connected_flag;
extern int hid_mode;
#endif

void nftool_set_custom_cursor(GdkWindow *wnd)
{
	GdkPixmap *src, *mask;
	GdkCursor *mcursor;
	GdkColor fg = {0, 0xff00, 0xff00, 0xff00}; /* White. */
	GdkColor bg = {0, 0x1000, 0x1000, 0x1000}; /* Black. */
	guchar *srcData = NULL, *maskData = NULL;

	guint i;
#ifdef _CURSOR_DISPLAY_ONOFF //nskim display on/off cursor
	if( mouse_connected_flag) {
		if(DISPLAY_IS_D1)
		{
			srcData = (guchar*)srcData_D1[NF_CURSOR_ARROW];
			maskData = (guchar*)maskData_D1[NF_CURSOR_ARROW];
		}
		else
		{
			srcData = (guchar*)srcData_4D1[NF_CURSOR_ARROW];
			maskData = (guchar*)maskData_4D1[NF_CURSOR_ARROW];
		}
	} else {	//disconnect mouse
		if(DISPLAY_IS_D1)
		{
			srcData = (guchar*)srcData_D1[NF_CURSOR_NULL];
			maskData = (guchar*)maskData_D1[NF_CURSOR_NULL];
		}
		else
		{
			srcData = (guchar*)srcData_4D1[NF_CURSOR_NULL];
			maskData = (guchar*)maskData_4D1[NF_CURSOR_NULL];
		}
	}

#else
	if(DISPLAY_IS_D1)
	{
		srcData = (guchar*)srcData_D1[NF_CURSOR_ARROW];
		maskData = (guchar*)maskData_D1[NF_CURSOR_ARROW];
	}
	else
	{
		srcData = (guchar*)srcData_4D1[NF_CURSOR_ARROW];
		maskData = (guchar*)maskData_4D1[NF_CURSOR_ARROW];
	}
#endif

	src = gdk_bitmap_create_from_data(NULL, srcData, MCURSOR_WIDTH, MCURSOR_HEIGHT);
	mask = gdk_bitmap_create_from_data(NULL, maskData, MCURSOR_WIDTH, MCURSOR_HEIGHT);

	mcursor = gdk_cursor_new_from_pixmap (src, mask, &fg, &bg, 0, 0);

	gdk_pixmap_unref (src);
	gdk_pixmap_unref (mask);

	gdk_window_set_cursor(wnd, mcursor);

	gdk_cursor_unref(mcursor);
}
#endif

void nftool_change_custom_cursor(GdkWindow *wnd, nfcursor_type type)
{
	GdkPixmap *src, *mask;
	GdkCursor *mcursor;
	GdkColor fg = {0, 0xff00, 0xff00, 0xff00}; /* White. */
	GdkColor bg = {0, 0x1000, 0x1000, 0x1000}; /* Black. */
	guchar *srcData = NULL, *maskData = NULL;

	//Forced Set UI Change Cursor in connected HID_JOYSTIC
	if(hid_joystick_connected_flag)
	{
		if(hid_mode)	// PTZ
			type = NF_CURSOR_FLEUR;
		else			//NVR
			type = NF_CURSOR_ARROW;
	}
#ifdef _CURSOR_DISPLAY_ONOFF //nskim display on/off cursor
	if( mouse_connected_flag || hid_joystick_connected_flag) {
		if(DISPLAY_IS_D1)
		{
			srcData = (guchar*)srcData_D1[type];
			maskData = (guchar*)maskData_D1[type];
		}
		else
		{
			srcData = (guchar*)srcData_4D1[type];
			maskData = (guchar*)maskData_4D1[type];
		}
	} else {	//disconnect mouse
		if(DISPLAY_IS_D1)
		{
			srcData = (guchar*)srcData_D1[NF_CURSOR_NULL];
			maskData = (guchar*)maskData_D1[NF_CURSOR_NULL];
		}
		else
		{
			srcData = (guchar*)srcData_4D1[NF_CURSOR_NULL];
			maskData = (guchar*)maskData_4D1[NF_CURSOR_NULL];
		}
	}

#else
	if(DISPLAY_IS_D1)
	{
		srcData = (guchar*)srcData_D1[type];
		maskData = (guchar*)maskData_D1[type];
	}
	else
	{
		srcData = (guchar*)srcData_4D1[type];
		maskData = (guchar*)maskData_4D1[type];
	}

#endif
	src = gdk_bitmap_create_from_data(NULL, srcData, MCURSOR_WIDTH, MCURSOR_HEIGHT);
	mask = gdk_bitmap_create_from_data(NULL, maskData, MCURSOR_WIDTH, MCURSOR_HEIGHT);

	mcursor = gdk_cursor_new_from_pixmap (src, mask, &fg, &bg, 0, 0);

	gdk_pixmap_unref (src);
	gdk_pixmap_unref (mask);

	gdk_window_set_cursor(wnd, mcursor);

	gdk_cursor_unref(mcursor);
}



/**************************************************************
 * TO SUPPORT TOOLTIP
 * ***********************************************************/
static NFOBJECT *tt_win = NULL;
static gchar tt_font[32] = "Arial bold 18";
static gint tooltipPosition_x;
static gint tooltipPosition_y;

#define	TOOLTIP_HEIGHT	(DISPLAY_IS_D1? 16:32)

#define	CRT_MARGIN_H	(20)
#define	CRT_MARGIN_V	(10)

#define TOOLTIP_MARGIN  6

static void prvGetPosition(NFOBJECT* base_obj, guint *pos_x, guint *pos_y, gint *width, gint *height, const gchar *str, gint conv)
{
	gchar *tooltip = NULL;
	GdkDrawable *drawable = NULL;

	NFOBJECT *win;
	gint win_x, win_y;
	gint obj_x, obj_y;
	guint w_temp;
	gint i, j;

	win = nfui_nfobject_get_top(base_obj);
	nfui_nfobject_get_offset(base_obj, &obj_x, &obj_y);

	win_x = win->x;
	win_y = win->y;

	nftool_tooltip_get_mouse_position(pos_x, pos_y);
//	*pos_x = win->x + obj_x + (base_obj->width/5);
//	*pos_y = win->y + obj_y - TOOLTIP_HEIGHT;

	nfui_nfobject_set_data(tt_win, "tooltip", g_strdup(str));

	drawable = nfui_nfobject_get_window(base_obj);
	w_temp = nfutil_string_width(conv, drawable, tt_font, str, NORMAL_SPACING);

	*width = w_temp + 8;
	*height = TOOLTIP_HEIGHT;

	j = strlen(str);
	for(i=0; i<j; i++)
	{
		if(str[i] == '\n')
		{
			*height += TOOLTIP_HEIGHT;
			*pos_y -= TOOLTIP_HEIGHT;
		}
	}

	if(*pos_x + *width >= DISPLAY_ACTIVE_WIDTH - CRT_MARGIN_H)
		*pos_x = DISPLAY_ACTIVE_WIDTH - CRT_MARGIN_H - *width;

	if(*pos_y < 0)
		*pos_y = win_y + obj_y + base_obj->height;
}

static gboolean post_tt_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gchar *tooltip = NULL;
	gint use_multi = 1;
	GdkDrawable *drawable = NULL;
	GdkGC *gc = NULL;

	switch(evt->type)
	{
		case GDK_EXPOSE:
			tooltip = nfui_nfobject_get_data(tt_win, "tooltip");
			use_multi = nfui_nfobject_get_data(tt_win, "multi_lang");

			nftool_draw_object_border(obj, COLOR_IDX(196), 1);

			drawable = nfui_nfobject_get_window(obj);
			gc = nfui_nfobject_get_gc(obj);

			g_message("obj->type :::::::::::::; = %d \n", obj->type);

			if(use_multi)
			{
				nfutil_draw_text_with_pango(NULL, NULL, NULL, drawable, gc, tooltip, obj->x, obj->y, obj->width, obj->height, tt_font, &UX_COLOR(197), NFALIGN_LEFT, 4);
			}
			else
			{
		        nfutil_draw_text_with_pango_eng(NULL, NULL, NULL, drawable, gc, tooltip, obj->x, obj->y, obj->width, obj->height, tt_font, &UX_COLOR(197), NFALIGN_LEFT, 4);
			}

			nfui_nfobject_gc_unref(gc);
			break;

		default:
			break;
	}

	return FALSE;
}


// SKSHIN
#if 1
void nftool_tooltip_init()
{
	NFOBJECT *tt_fixed = NULL;

	tt_win = (NFOBJECT*)nfui_nfwindow_new(GTK_WINDOW_TOPLEVEL, 0, 0, 1, 1);
	tt_fixed = nfui_nffixed_new();

	nfui_nfwindow_set_modal((NFWINDOW*)tt_win, FALSE);
	gtk_window_set_resizable(((NFWINDOW*)tt_win)->main_widget, TRUE);

#if 0
	nfui_nfobject_modify_bg(tt_win, NFOBJECT_STATE_NORMAL, COLOR_IDX(195));
	nfui_nfobject_modify_bg(tt_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(195));
#else
	nfui_nfobject_modify_bg(tt_win, NFOBJECT_STATE_NORMAL, COLOR_IDX(195));
	nfui_nfobject_modify_bg(tt_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(195));
#endif

	nfui_nfobject_show(tt_fixed);
	nfui_nfwindow_add((NFWINDOW*)tt_win, tt_fixed);

	nfui_regi_post_event_callback(tt_fixed, post_tt_main_fixed_event_handler);
	nfui_run_main_event_handler(tt_win);
}
#endif

void nftool_tooltip_show(NFOBJECT *obj, const gchar *str)
{
	gint x, y, w, h;
	gchar *strTooltip = NULL;
	gchar temp_buf[256] = {NULL, };
	gchar buf[256] = {NULL, };
	guint i, str_cnt;
	gint multi_lang = 1;

	nftool_tooltip_hide();

	if(tt_win == NULL || obj == NULL || obj->use_tooltip == 0)
		return;

	if(str)		strTooltip = str;
	else		strTooltip = nfui_nfobject_get_tooltip(obj);

	if(strTooltip == NULL || strlen(strTooltip) <= 0)
		return;

	str_cnt = strlen(strTooltip);
	strcpy(temp_buf, strTooltip);

	for(i=0; i<str_cnt; i++)
	{
		if(temp_buf[i] == '\n')
			buf[i] = ' ';
		else
			buf[i] = temp_buf[i];
	}

	multi_lang = nfui_nfobject_is_supported_multi_lang(obj);
	prvGetPosition(obj, &x, &y, &w, &h, buf, multi_lang);
	nfui_nfobject_set_data(tt_win, "multi_lang", multi_lang);

	nfui_nfobject_move(tt_win, x, (y-TOOLTIP_HEIGHT));
	nfui_nfobject_set_size(tt_win, w+TOOLTIP_MARGIN, h);

	nfui_nfobject_show(tt_win);
}

void nftool_tooltip_show_with_pos(NFOBJECT *obj, const gchar *str, gint win_x, gint win_y)
{
	gint x, y, w, h;
	gchar *strTooltip = NULL;
	gchar temp_buf[256] = {NULL, };
	gchar buf[256] = {NULL, };
	guint i, str_cnt;
	gint multi_lang = 0;

	nftool_tooltip_hide();

	if(tt_win == NULL || obj == NULL || obj->use_tooltip == 0)
		return;

	if(str)		strTooltip = str;
	else		strTooltip = nfui_nfobject_get_tooltip(obj);

	if(strTooltip == NULL || strlen(strTooltip) <= 0)
		return;

	str_cnt = strlen(strTooltip);
	strcpy(temp_buf, strTooltip);

	for(i=0; i<str_cnt; i++)
	{
		if(temp_buf[i] == '\n')
			buf[i] = ' ';
		else
			buf[i] = temp_buf[i];
	}

	multi_lang = nfui_nfobject_is_supported_multi_lang(obj);
	prvGetPosition(obj, &x, &y, &w, &h, buf, multi_lang);
	nfui_nfobject_set_data(tt_win, "multi_lang", multi_lang);

	nfui_nfobject_move(tt_win, win_x, win_y);
	nfui_nfobject_set_size(tt_win, w+TOOLTIP_MARGIN, h);

	nfui_nfobject_show(tt_win);
}

void nftool_tooltip_show_with_offset(NFOBJECT *obj, const gchar *str, gint off_x, gint off_y)
{
	gint x, y, w, h;
	gchar *strTooltip = NULL;
	gchar temp_buf[256] = {NULL, };
	gchar buf[256] = {NULL, };
	guint i, str_cnt;
	gint multi_lang = 0;

	nftool_tooltip_hide();

	if(tt_win == NULL || obj == NULL || obj->use_tooltip == 0)
		return;

	if(str)		strTooltip = str;
	else		strTooltip = nfui_nfobject_get_tooltip(obj);

	if(strTooltip == NULL || strlen(strTooltip) <= 0)
		return;

	str_cnt = strlen(strTooltip);
	strcpy(temp_buf, strTooltip);

	for(i=0; i<str_cnt; i++)
	{
		if(temp_buf[i] == '\n')
			buf[i] = ' ';
		else
			buf[i] = temp_buf[i];
	}

	multi_lang = nfui_nfobject_is_supported_multi_lang(obj);
	prvGetPosition(obj, &x, &y, &w, &h, buf, multi_lang);
	nfui_nfobject_set_data(tt_win, "multi_lang", multi_lang);

	nfui_nfobject_move(tt_win, x+off_x, y+off_y);
	nfui_nfobject_set_size(tt_win, w+TOOLTIP_MARGIN, h);

	nfui_nfobject_show(tt_win);
}

void nftool_tooltip_hide()
{
	gchar *tooltip = NULL;

	if(tt_win == NULL)
		return;

	tooltip = nfui_nfobject_get_data(tt_win, "tooltip");
	if(tooltip)
	{
		g_free(tooltip);
		nfui_nfobject_set_data(tt_win, "tooltip", NULL);
	}

	nfui_nfobject_hide(tt_win);
}

void nftool_tooltip_set_mouse_position(guint x, guint y)
{
	tooltipPosition_x = x;
	tooltipPosition_y = y;
}

void nftool_tooltip_get_mouse_position(guint *x, guint *y)
{
	*x = tooltipPosition_x;
	*y = tooltipPosition_y;
}



/**************************************************************
 * USEFUL FUNCTIONS..
 * ***********************************************************/

// skshin
#if 0
NFOBJECT* nftool_normal_button_new(const gchar *strLabel)
{
	NFOBJECT* button;

	GdkPixbuf *btn_image[NFOBJECT_STATE_COUNT];
#ifdef __NF_46_UI__
	const guint btn_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(TODO_COLOR), COLOR_IDX(TODO_COLOR), COLOR_IDX(TODO_COLOR), COLOR_IDX(TODO_COLOR)};
#else
	const guint btn_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(TODO_COLOR), COLOR_IDX(TODO_COLOR), COLOR_IDX(TODO_COLOR), COLOR_IDX(TODO_COLOR)};
#endif

	guint btn_width;
	guint btn_height;

	btn_image[NFOBJECT_STATE_NORMAL] = nfui_get_image_from_file(IMG_BTN1, NULL);
	btn_image[NFOBJECT_STATE_PRELIGHT] = nfui_get_image_from_file(IMG_BTN3, NULL);
	btn_image[NFOBJECT_STATE_ACTIVE] = nfui_get_image_from_file(IMG_BTN2, NULL);
	btn_image[NFOBJECT_STATE_DISABLE] = nfui_get_image_from_file(IMG_BTN4, NULL);

	nfui_get_pixbuf_size(btn_image[0], &btn_width, &btn_height);

	button = (NFOBJECT*)nfui_nfbutton_new_with_param(btn_image, strLabel);
	nfui_nfbutton_set_pango_font((NFBUTTON*)button, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), (guint*)btn_font_color);
	nfui_nfobject_set_size(button, btn_width, btn_height);

	return button;
}
#endif

int nftool_create_type1_btn_image(int size)
{
	gchar btn_name_n[32];
	gchar btn_name_o[32];
	gchar btn_name_p[32];
	gchar btn_name_s[32];
	gchar btn_name_d[32];

	g_sprintf(btn_name_n, "MKB_IMG_BTN1_N_%d", size);
	g_sprintf(btn_name_o, "MKB_IMG_BTN1_O_%d", size);
	g_sprintf(btn_name_p, "MKB_IMG_BTN1_P_%d", size);
	g_sprintf(btn_name_s, "MKB_IMG_BTN1_S_%d", size);
	g_sprintf(btn_name_d, "MKB_IMG_BTN1_D_%d", size);

	nf_ui_create_image_button_method(btn_name_n, size, IMG_BTN1_N_L, IMG_BTN1_N_M, IMG_BTN1_N_R);
	nf_ui_create_image_button_method(btn_name_o, size, IMG_BTN1_O_L, IMG_BTN1_O_M, IMG_BTN1_O_R);
	nf_ui_create_image_button_method(btn_name_p, size, IMG_BTN1_P_L, IMG_BTN1_P_M, IMG_BTN1_P_R);
	nf_ui_create_image_button_method(btn_name_s, size, IMG_BTN1_S_L, IMG_BTN1_S_M, IMG_BTN1_S_R);
	nf_ui_create_image_button_method(btn_name_d, size, IMG_BTN1_D_L, IMG_BTN1_D_M, IMG_BTN1_D_R);

	return 0;
}

NFOBJECT* nftool_normal_button_create_type1(const gchar *strLabel, guint size)
{
	NFOBJECT* button;

	GdkPixbuf *btn_image[NFOBJECT_STATE_COUNT];
	const guint btn_font_color[NFOBJECT_STATE_COUNT] = {100, 101, 103, 104};

	guint btn_width;
	guint btn_height;

	gchar btn_name_n[32];
	gchar btn_name_o[32];
	gchar btn_name_p[32];
	gchar btn_name_s[32];
	gchar btn_name_d[32];

	g_sprintf(btn_name_n, "MKB_IMG_BTN1_N_%d", size);
	g_sprintf(btn_name_o, "MKB_IMG_BTN1_O_%d", size);
	g_sprintf(btn_name_p, "MKB_IMG_BTN1_P_%d", size);
	g_sprintf(btn_name_s, "MKB_IMG_BTN1_S_%d", size);
	g_sprintf(btn_name_d, "MKB_IMG_BTN1_D_%d", size);

	nf_ui_create_image_button_method(btn_name_n, size, IMG_BTN1_N_L, IMG_BTN1_N_M, IMG_BTN1_N_R);
	nf_ui_create_image_button_method(btn_name_o, size, IMG_BTN1_O_L, IMG_BTN1_O_M, IMG_BTN1_O_R);
	nf_ui_create_image_button_method(btn_name_p, size, IMG_BTN1_P_L, IMG_BTN1_P_M, IMG_BTN1_P_R);
	nf_ui_create_image_button_method(btn_name_s, size, IMG_BTN1_S_L, IMG_BTN1_S_M, IMG_BTN1_S_R);
	nf_ui_create_image_button_method(btn_name_d, size, IMG_BTN1_D_L, IMG_BTN1_D_M, IMG_BTN1_D_R);

	btn_image[NFOBJECT_STATE_NORMAL] = nfui_get_image_from_memory(btn_name_n);
	btn_image[NFOBJECT_STATE_PRELIGHT] = nfui_get_image_from_memory(btn_name_o);
	btn_image[NFOBJECT_STATE_ACTIVE] = nfui_get_image_from_memory(btn_name_p);
	btn_image[NFOBJECT_STATE_DISABLE] = nfui_get_image_from_memory(btn_name_d);

	nfui_get_pixbuf_size(btn_image[0], &btn_width, &btn_height);

	button = (NFOBJECT*)nfui_nfbutton_new_with_param(btn_image, strLabel);
	nfui_nfbutton_set_pango_font((NFBUTTON*)button, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), (guint*)btn_font_color);
	nfui_nfobject_set_size(button, btn_width, btn_height);

	return button;
}

int nftool_create_type2_btn_image(int size)
{
	gchar btn_name_n[32];
	gchar btn_name_o[32];
	gchar btn_name_p[32];
	gchar btn_name_s[32];
	gchar btn_name_d[32];

	g_sprintf(btn_name_n, "MKB_IMG_BTN2_N_%d", size);
	g_sprintf(btn_name_o, "MKB_IMG_BTN2_O_%d", size);
	g_sprintf(btn_name_p, "MKB_IMG_BTN2_P_%d", size);
	g_sprintf(btn_name_s, "MKB_IMG_BTN2_S_%d", size);
	g_sprintf(btn_name_d, "MKB_IMG_BTN2_D_%d", size);

	nf_ui_create_image_button_method(btn_name_n, size, IMG_BTN2_N_L, IMG_BTN2_N_M, IMG_BTN2_N_R);
	nf_ui_create_image_button_method(btn_name_o, size, IMG_BTN2_O_L, IMG_BTN2_O_M, IMG_BTN2_O_R);
	nf_ui_create_image_button_method(btn_name_p, size, IMG_BTN2_P_L, IMG_BTN2_P_M, IMG_BTN2_P_R);
	nf_ui_create_image_button_method(btn_name_s, size, IMG_BTN2_S_L, IMG_BTN2_S_M, IMG_BTN2_S_R);
	nf_ui_create_image_button_method(btn_name_d, size, IMG_BTN2_D_L, IMG_BTN2_D_M, IMG_BTN2_D_R);

	return 0;
}

NFOBJECT* nftool_normal_button_create_type2(const gchar *strLabel, guint size)
{
	NFOBJECT* button;

	GdkPixbuf *btn_image[NFOBJECT_STATE_COUNT];
	const guint btn_font_color[NFOBJECT_STATE_COUNT] = {105, 106, 108, 109};

	guint btn_width;
	guint btn_height;

	gchar btn_name_n[32];
	gchar btn_name_o[32];
	gchar btn_name_p[32];
	gchar btn_name_s[32];
	gchar btn_name_d[32];

	g_sprintf(btn_name_n, "MKB_IMG_BTN2_N_%d", size);
	g_sprintf(btn_name_o, "MKB_IMG_BTN2_O_%d", size);
	g_sprintf(btn_name_p, "MKB_IMG_BTN2_P_%d", size);
	g_sprintf(btn_name_s, "MKB_IMG_BTN2_S_%d", size);
	g_sprintf(btn_name_d, "MKB_IMG_BTN2_D_%d", size);

	nf_ui_create_image_button_method(btn_name_n, size, IMG_BTN2_N_L, IMG_BTN2_N_M, IMG_BTN2_N_R);
	nf_ui_create_image_button_method(btn_name_o, size, IMG_BTN2_O_L, IMG_BTN2_O_M, IMG_BTN2_O_R);
	nf_ui_create_image_button_method(btn_name_p, size, IMG_BTN2_P_L, IMG_BTN2_P_M, IMG_BTN2_P_R);
	nf_ui_create_image_button_method(btn_name_s, size, IMG_BTN2_S_L, IMG_BTN2_S_M, IMG_BTN2_S_R);
	nf_ui_create_image_button_method(btn_name_d, size, IMG_BTN2_D_L, IMG_BTN2_D_M, IMG_BTN2_D_R);

	btn_image[NFOBJECT_STATE_NORMAL] = nfui_get_image_from_memory(btn_name_n);
	btn_image[NFOBJECT_STATE_PRELIGHT] = nfui_get_image_from_memory(btn_name_o);
	btn_image[NFOBJECT_STATE_ACTIVE] = nfui_get_image_from_memory(btn_name_p);
	btn_image[NFOBJECT_STATE_DISABLE] = nfui_get_image_from_memory(btn_name_d);

	nfui_get_pixbuf_size(btn_image[0], &btn_width, &btn_height);

	button = (NFOBJECT*)nfui_nfbutton_new_with_param(btn_image, strLabel);
	nfui_nfbutton_set_pango_font((NFBUTTON*)button, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), (guint*)btn_font_color);
	nfui_nfobject_set_size(button, btn_width, btn_height);

	return button;
}

int nftool_create_type3_btn_image(int size)
{
	gchar btn_name_n[32];
	gchar btn_name_o[32];
	gchar btn_name_p[32];
	gchar btn_name_s[32];
	gchar btn_name_d[32];

	g_sprintf(btn_name_n, "MKB_IMG_BTN5_N_%d", size);
	g_sprintf(btn_name_o, "MKB_IMG_BTN5_O_%d", size);
	g_sprintf(btn_name_p, "MKB_IMG_BTN5_P_%d", size);
	g_sprintf(btn_name_s, "MKB_IMG_BTN5_S_%d", size);
	g_sprintf(btn_name_d, "MKB_IMG_BTN5_D_%d", size);

	nf_ui_create_image_button_method(btn_name_n, size, IMG_BTN5_N_L, IMG_BTN5_N_M, IMG_BTN5_N_R);
	nf_ui_create_image_button_method(btn_name_o, size, IMG_BTN5_O_L, IMG_BTN5_O_M, IMG_BTN5_O_R);
	nf_ui_create_image_button_method(btn_name_p, size, IMG_BTN5_P_L, IMG_BTN5_P_M, IMG_BTN5_P_R);
	nf_ui_create_image_button_method(btn_name_s, size, IMG_BTN5_S_L, IMG_BTN5_S_M, IMG_BTN5_S_R);
	nf_ui_create_image_button_method(btn_name_d, size, IMG_BTN5_D_L, IMG_BTN5_D_M, IMG_BTN5_D_R);

	return 0;
}

NFOBJECT* nftool_normal_button_create_type3(const gchar *strLabel, guint size)
{
	NFOBJECT* button;

	GdkPixbuf *btn_image[NFOBJECT_STATE_COUNT];
	const guint btn_font_color[NFOBJECT_STATE_COUNT] = {110, 111, 113, 114};

	guint btn_width;
	guint btn_height;

	gchar btn_name_n[32];
	gchar btn_name_o[32];
	gchar btn_name_p[32];
	gchar btn_name_s[32];
	gchar btn_name_d[32];

	g_sprintf(btn_name_n, "MKB_IMG_BTN5_N_%d", size);
	g_sprintf(btn_name_o, "MKB_IMG_BTN5_O_%d", size);
	g_sprintf(btn_name_p, "MKB_IMG_BTN5_P_%d", size);
	g_sprintf(btn_name_s, "MKB_IMG_BTN5_S_%d", size);
	g_sprintf(btn_name_d, "MKB_IMG_BTN5_D_%d", size);

	nf_ui_create_image_button_method(btn_name_n, size, IMG_BTN5_N_L, IMG_BTN5_N_M, IMG_BTN5_N_R);
	nf_ui_create_image_button_method(btn_name_o, size, IMG_BTN5_O_L, IMG_BTN5_O_M, IMG_BTN5_O_R);
	nf_ui_create_image_button_method(btn_name_p, size, IMG_BTN5_P_L, IMG_BTN5_P_M, IMG_BTN5_P_R);
	nf_ui_create_image_button_method(btn_name_s, size, IMG_BTN5_S_L, IMG_BTN5_S_M, IMG_BTN5_S_R);
	nf_ui_create_image_button_method(btn_name_d, size, IMG_BTN5_D_L, IMG_BTN5_D_M, IMG_BTN5_D_R);

	btn_image[NFOBJECT_STATE_NORMAL] = nfui_get_image_from_memory(btn_name_n);
	btn_image[NFOBJECT_STATE_PRELIGHT] = nfui_get_image_from_memory(btn_name_o);
	btn_image[NFOBJECT_STATE_ACTIVE] = nfui_get_image_from_memory(btn_name_p);
	btn_image[NFOBJECT_STATE_DISABLE] = nfui_get_image_from_memory(btn_name_d);

	nfui_get_pixbuf_size(btn_image[0], &btn_width, &btn_height);

	button = (NFOBJECT*)nfui_nfbutton_new_with_param(btn_image, strLabel);
	nfui_nfbutton_set_pango_font((NFBUTTON*)button, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), (guint*)btn_font_color);
	nfui_nfobject_set_size(button, btn_width, btn_height);

	return button;
}

int nftool_create_popup_type1_btn_image(int size)
{
	gchar btn_name_n[32];
	gchar btn_name_o[32];
	gchar btn_name_p[32];
	gchar btn_name_s[32];
	gchar btn_name_d[32];

	g_sprintf(btn_name_n, "MKB_IMG_POPUP_BTN1_N_%d", size);
	g_sprintf(btn_name_o, "MKB_IMG_POPUP_BTN1_O_%d", size);
	g_sprintf(btn_name_p, "MKB_IMG_POPUP_BTN1_P_%d", size);
	g_sprintf(btn_name_s, "MKB_IMG_POPUP_BTN1_S_%d", size);
	g_sprintf(btn_name_d, "MKB_IMG_POPUP_BTN1_D_%d", size);

	nf_ui_create_image_button_method(btn_name_n, size, IMG_POPUP_BTN1_N_L, IMG_POPUP_BTN1_N_M, IMG_POPUP_BTN1_N_R);
	nf_ui_create_image_button_method(btn_name_o, size, IMG_POPUP_BTN1_O_L, IMG_POPUP_BTN1_O_M, IMG_POPUP_BTN1_O_R);
	nf_ui_create_image_button_method(btn_name_p, size, IMG_POPUP_BTN1_P_L, IMG_POPUP_BTN1_P_M, IMG_POPUP_BTN1_P_R);
	nf_ui_create_image_button_method(btn_name_s, size, IMG_POPUP_BTN1_S_L, IMG_POPUP_BTN1_S_M, IMG_POPUP_BTN1_S_R);
	nf_ui_create_image_button_method(btn_name_d, size, IMG_POPUP_BTN1_D_L, IMG_POPUP_BTN1_D_M, IMG_POPUP_BTN1_D_R);

	return 0;
}

NFOBJECT* nftool_normal_button_create_popup_type1(const gchar *strLabel, guint size)
{
	NFOBJECT* button;

	GdkPixbuf *btn_image[NFOBJECT_STATE_COUNT];
	const guint btn_font_color[NFOBJECT_STATE_COUNT] = {210, 211, 213, 214};

	guint btn_width;
	guint btn_height;

	gchar btn_name_n[32];
	gchar btn_name_o[32];
	gchar btn_name_p[32];
	gchar btn_name_s[32];
	gchar btn_name_d[32];

	g_sprintf(btn_name_n, "MKB_IMG_POPUP_BTN1_N_%d", size);
	g_sprintf(btn_name_o, "MKB_IMG_POPUP_BTN1_O_%d", size);
	g_sprintf(btn_name_p, "MKB_IMG_POPUP_BTN1_P_%d", size);
	g_sprintf(btn_name_s, "MKB_IMG_POPUP_BTN1_S_%d", size);
	g_sprintf(btn_name_d, "MKB_IMG_POPUP_BTN1_D_%d", size);

	nf_ui_create_image_button_method(btn_name_n, size, IMG_POPUP_BTN1_N_L, IMG_POPUP_BTN1_N_M, IMG_POPUP_BTN1_N_R);
	nf_ui_create_image_button_method(btn_name_o, size, IMG_POPUP_BTN1_O_L, IMG_POPUP_BTN1_O_M, IMG_POPUP_BTN1_O_R);
	nf_ui_create_image_button_method(btn_name_p, size, IMG_POPUP_BTN1_P_L, IMG_POPUP_BTN1_P_M, IMG_POPUP_BTN1_P_R);
	nf_ui_create_image_button_method(btn_name_s, size, IMG_POPUP_BTN1_S_L, IMG_POPUP_BTN1_S_M, IMG_POPUP_BTN1_S_R);
	nf_ui_create_image_button_method(btn_name_d, size, IMG_POPUP_BTN1_D_L, IMG_POPUP_BTN1_D_M, IMG_POPUP_BTN1_D_R);

	btn_image[NFOBJECT_STATE_NORMAL] = nfui_get_image_from_memory(btn_name_n);
	btn_image[NFOBJECT_STATE_PRELIGHT] = nfui_get_image_from_memory(btn_name_o);
	btn_image[NFOBJECT_STATE_ACTIVE] = nfui_get_image_from_memory(btn_name_p);
	btn_image[NFOBJECT_STATE_DISABLE] = nfui_get_image_from_memory(btn_name_d);

	nfui_get_pixbuf_size(btn_image[0], &btn_width, &btn_height);

	button = (NFOBJECT*)nfui_nfbutton_new_with_param(btn_image, strLabel);
	nfui_nfbutton_set_pango_font((NFBUTTON*)button, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), (guint*)btn_font_color);
	nfui_nfobject_set_size(button, btn_width, btn_height);

	return button;
}

int nftool_create_popup_type2_btn_image(int size)
{
	gchar btn_name_n[32];
	gchar btn_name_o[32];
	gchar btn_name_p[32];
	gchar btn_name_s[32];
	gchar btn_name_d[32];

	g_sprintf(btn_name_n, "MKB_IMG_POPUP_BTN2_N_%d", size);
	g_sprintf(btn_name_o, "MKB_IMG_POPUP_BTN2_O_%d", size);
	g_sprintf(btn_name_p, "MKB_IMG_POPUP_BTN2_P_%d", size);
	g_sprintf(btn_name_s, "MKB_IMG_POPUP_BTN2_S_%d", size);
	g_sprintf(btn_name_d, "MKB_IMG_POPUP_BTN2_D_%d", size);

	nf_ui_create_image_button_method(btn_name_n, size, IMG_POPUP_BTN2_N_L, IMG_POPUP_BTN2_N_M, IMG_POPUP_BTN2_N_R);
	nf_ui_create_image_button_method(btn_name_o, size, IMG_POPUP_BTN2_O_L, IMG_POPUP_BTN2_O_M, IMG_POPUP_BTN2_O_R);
	nf_ui_create_image_button_method(btn_name_p, size, IMG_POPUP_BTN2_P_L, IMG_POPUP_BTN2_P_M, IMG_POPUP_BTN2_P_R);
	nf_ui_create_image_button_method(btn_name_s, size, IMG_POPUP_BTN2_S_L, IMG_POPUP_BTN2_S_M, IMG_POPUP_BTN2_S_R);
	nf_ui_create_image_button_method(btn_name_d, size, IMG_POPUP_BTN2_D_L, IMG_POPUP_BTN2_D_M, IMG_POPUP_BTN2_D_R);

	return 0;
}

NFOBJECT* nftool_normal_button_create_popup_type2(const gchar *strLabel, guint size)
{
	NFOBJECT* button;

	GdkPixbuf *btn_image[NFOBJECT_STATE_COUNT];
	const guint btn_font_color[NFOBJECT_STATE_COUNT] = {215, 216, 217, 219};

	guint btn_width;
	guint btn_height;

	gchar btn_name_n[32];
	gchar btn_name_o[32];
	gchar btn_name_p[32];
	gchar btn_name_s[32];
	gchar btn_name_d[32];

	g_sprintf(btn_name_n, "MKB_IMG_POPUP_BTN2_N_%d", size);
	g_sprintf(btn_name_o, "MKB_IMG_POPUP_BTN2_O_%d", size);
	g_sprintf(btn_name_p, "MKB_IMG_POPUP_BTN2_P_%d", size);
	g_sprintf(btn_name_s, "MKB_IMG_POPUP_BTN2_S_%d", size);
	g_sprintf(btn_name_d, "MKB_IMG_POPUP_BTN2_D_%d", size);

	nf_ui_create_image_button_method(btn_name_n, size, IMG_POPUP_BTN2_N_L, IMG_POPUP_BTN2_N_M, IMG_POPUP_BTN2_N_R);
	nf_ui_create_image_button_method(btn_name_o, size, IMG_POPUP_BTN2_O_L, IMG_POPUP_BTN2_O_M, IMG_POPUP_BTN2_O_R);
	nf_ui_create_image_button_method(btn_name_p, size, IMG_POPUP_BTN2_P_L, IMG_POPUP_BTN2_P_M, IMG_POPUP_BTN2_P_R);
	nf_ui_create_image_button_method(btn_name_s, size, IMG_POPUP_BTN2_S_L, IMG_POPUP_BTN2_S_M, IMG_POPUP_BTN2_S_R);
	nf_ui_create_image_button_method(btn_name_d, size, IMG_POPUP_BTN2_D_L, IMG_POPUP_BTN2_D_M, IMG_POPUP_BTN2_D_R);

	btn_image[NFOBJECT_STATE_NORMAL] = nfui_get_image_from_memory(btn_name_n);
	btn_image[NFOBJECT_STATE_PRELIGHT] = nfui_get_image_from_memory(btn_name_o);
	btn_image[NFOBJECT_STATE_ACTIVE] = nfui_get_image_from_memory(btn_name_p);
	btn_image[NFOBJECT_STATE_DISABLE] = nfui_get_image_from_memory(btn_name_d);

	nfui_get_pixbuf_size(btn_image[0], &btn_width, &btn_height);

	button = (NFOBJECT*)nfui_nfbutton_new_with_param(btn_image, strLabel);
	nfui_nfbutton_set_pango_font((NFBUTTON*)button, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), (guint*)btn_font_color);
	nfui_nfobject_set_size(button, btn_width, btn_height);

	return button;
}

int nftool_create_subtab_type1_btn_image(int size)
{
	gchar btn_name_n[32];
	gchar btn_name_o[32];
	gchar btn_name_p[32];
	gchar btn_name_s[32];
	gchar btn_name_d[32];

	g_sprintf(btn_name_n, "MKB_IMG_SUBTAB_BTN1_N_%d", size);
	g_sprintf(btn_name_o, "MKB_IMG_SUBTAB_BTN1_O_%d", size);
	g_sprintf(btn_name_p, "MKB_IMG_SUBTAB_BTN1_P_%d", size);
	g_sprintf(btn_name_s, "MKB_IMG_SUBTAB_BTN1_S_%d", size);
	g_sprintf(btn_name_d, "MKB_IMG_SUBTAB_BTN1_D_%d", size);

	nf_ui_create_image_button_method(btn_name_n, size, IMG_SUBTAB_BTN1_N_L, IMG_SUBTAB_BTN1_N_M, IMG_SUBTAB_BTN1_N_R);
	nf_ui_create_image_button_method(btn_name_o, size, IMG_SUBTAB_BTN1_O_L, IMG_SUBTAB_BTN1_O_M, IMG_SUBTAB_BTN1_O_R);
	nf_ui_create_image_button_method(btn_name_p, size, IMG_SUBTAB_BTN1_P_L, IMG_SUBTAB_BTN1_P_M, IMG_SUBTAB_BTN1_P_R);
	nf_ui_create_image_button_method(btn_name_s, size, IMG_SUBTAB_BTN1_S_L, IMG_SUBTAB_BTN1_S_M, IMG_SUBTAB_BTN1_S_R);
	nf_ui_create_image_button_method(btn_name_d, size, IMG_SUBTAB_BTN1_D_L, IMG_SUBTAB_BTN1_D_M, IMG_SUBTAB_BTN1_D_R);

	return 0;
}

NFOBJECT* nftool_normal_button_create_subtab_type1(const gchar *strLabel, guint size)
{
	NFOBJECT* button;

	GdkPixbuf *btn_image[NFOBJECT_STATE_COUNT];
	const guint btn_font_color[NFOBJECT_STATE_COUNT] = {900, 901, 902, 904};

	guint btn_width;
	guint btn_height;

	gchar btn_name_n[32];
	gchar btn_name_o[32];
	gchar btn_name_p[32];
	gchar btn_name_s[32];
	gchar btn_name_d[32];

	g_sprintf(btn_name_n, "MKB_IMG_SUBTAB_BTN1_N_%d", size);
	g_sprintf(btn_name_o, "MKB_IMG_SUBTAB_BTN1_O_%d", size);
	g_sprintf(btn_name_p, "MKB_IMG_SUBTAB_BTN1_P_%d", size);
	g_sprintf(btn_name_s, "MKB_IMG_SUBTAB_BTN1_S_%d", size);
	g_sprintf(btn_name_d, "MKB_IMG_SUBTAB_BTN1_D_%d", size);

	nf_ui_create_image_button_method(btn_name_n, size, IMG_SUBTAB_BTN1_N_L, IMG_SUBTAB_BTN1_N_M, IMG_SUBTAB_BTN1_N_R);
	nf_ui_create_image_button_method(btn_name_o, size, IMG_SUBTAB_BTN1_O_L, IMG_SUBTAB_BTN1_O_M, IMG_SUBTAB_BTN1_O_R);
	nf_ui_create_image_button_method(btn_name_p, size, IMG_SUBTAB_BTN1_P_L, IMG_SUBTAB_BTN1_P_M, IMG_SUBTAB_BTN1_P_R);
	nf_ui_create_image_button_method(btn_name_s, size, IMG_SUBTAB_BTN1_S_L, IMG_SUBTAB_BTN1_S_M, IMG_SUBTAB_BTN1_S_R);
	nf_ui_create_image_button_method(btn_name_d, size, IMG_SUBTAB_BTN1_D_L, IMG_SUBTAB_BTN1_D_M, IMG_SUBTAB_BTN1_D_R);

	btn_image[NFOBJECT_STATE_NORMAL] = nfui_get_image_from_memory(btn_name_n);
	btn_image[NFOBJECT_STATE_PRELIGHT] = nfui_get_image_from_memory(btn_name_o);
	btn_image[NFOBJECT_STATE_ACTIVE] = nfui_get_image_from_memory(btn_name_p);
	btn_image[NFOBJECT_STATE_DISABLE] = nfui_get_image_from_memory(btn_name_d);

	nfui_get_pixbuf_size(btn_image[0], &btn_width, &btn_height);

	button = (NFOBJECT*)nfui_nfbutton_new_with_param(btn_image, strLabel);
	nfui_nfbutton_set_pango_font((NFBUTTON*)button, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), (guint*)btn_font_color);
	nfui_nfobject_set_size(button, btn_width, btn_height);

	return button;
}

GdkPixbuf *nftool_create_slider_n_image(int size)
{
	GdkPixbuf *image;
	gchar name_n[32];

	g_sprintf(name_n, "MK_IMG_SLIDER_N_%d", size);

	image = nf_ui_create_image_button_method(name_n, size, IMG_SLIDE_BG_N_L, IMG_SLIDE_BG_N_M, IMG_SLIDE_BG_N_R);

	return image;
}

GdkPixbuf *nftool_create_slider_d_image(int size)
{
	GdkPixbuf *image;
	gchar name_d[32];

	g_sprintf(name_d, "MK_IMG_SLIDER_D_%d", size);

	image = nf_ui_create_image_button_method(name_d, size, IMG_SLIDE_BG_D_L, IMG_SLIDE_BG_D_M, IMG_SLIDE_BG_D_R);

	return image;
}

GdkPixbuf *nftool_create_slider_small_n_image(int size)
{
	GdkPixbuf *image;
	gchar name_n[32];

	g_sprintf(name_n, "MK_IMG_SLIDER_SMALL_N_%d", size);

	image = nf_ui_create_image_button_method(name_n, size, MK_IMG_SLIDE_SMALL_BG_N_L, MK_IMG_SLIDE_SMALL_BG_N_M, MK_IMG_SLIDE_SMALL_BG_N_R);

	return image;
}

GdkPixbuf *nftool_create_slider_small_d_image(int size)
{
	GdkPixbuf *image;
	gchar name_d[32];

	g_sprintf(name_d, "MK_IMG_SLIDER_SMALL_D_%d", size);

	image = nf_ui_create_image_button_method(name_d, size, MK_IMG_SLIDE_SMALL_BG_D_L, MK_IMG_SLIDE_SMALL_BG_D_M, MK_IMG_SLIDE_SMALL_BG_D_R);

	return image;
}

gboolean nftool_cur_language_is_eng()
{
	OsdData osddata;
	gint ret;

	memset(&osddata, 0x00, sizeof(OsdData));
	DAL_get_osd_data(&osddata);

	ret = strcmp(osddata.lang, "ENGLISH");

	if(ret == 0)
		return TRUE;
	else
		return FALSE;
}

gboolean nftool_cur_language_is_japanese()
{
	OsdData osddata;
	gint ret;

	memset(&osddata, 0x00, sizeof(OsdData));
	DAL_get_osd_data(&osddata);

	ret = strcmp(osddata.lang, "JAPANESE");

	if(ret == 0)
		return TRUE;
	else
		return FALSE;
}

gboolean nftool_cur_language_is_korean()
{
	OsdData osddata;
	gint ret;

	memset(&osddata, 0x00, sizeof(OsdData));
	DAL_get_osd_data(&osddata);

	ret = strcmp(osddata.lang, "KOREAN");

	if(ret == 0)
		return TRUE;
	else
		return FALSE;
}



/****************
 * 	1 | 2 | 3
 *	4 | 0 | 5
 *	6 | 7 | 8
 ***************/
gint nftool_calc_distance(NFOBJECT* obj1, NFOBJECT* obj2)
{
	gint dist = 0;



	return dist;
}


void nftool_draw_object_border(NFOBJECT *obj, gint col_idx, gint size)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	gint x, y;

	drawable = nfui_nfobject_get_window(obj);
	gc = nfui_nfobject_get_gc(obj);

	nfui_nfobject_get_offset(obj, &x, &y);

	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(col_idx));

	gdk_draw_rectangle(drawable, gc, TRUE, x, y, obj->width, size);
	gdk_draw_rectangle(drawable, gc, TRUE, x, y, size, obj->height);
	gdk_draw_rectangle(drawable, gc, TRUE, x+obj->width-size, y, size, obj->height);
	gdk_draw_rectangle(drawable, gc, TRUE, x, y+obj->height-size, obj->width, size);

	nfui_nfobject_gc_unref(gc);
}

void nftool_draw_border(GdkDrawable *drawable, gint x, gint y, gint w, gint h, gint col_idx, gint size)
{
	GdkGC *gc;

	gc = gdk_gc_new(drawable);

	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(col_idx));

	gdk_draw_rectangle(drawable, gc, TRUE, x, y, w, size);
	gdk_draw_rectangle(drawable, gc, TRUE, x, y, size, h);
	gdk_draw_rectangle(drawable, gc, TRUE, x+w-size, y, size, h);
	gdk_draw_rectangle(drawable, gc, TRUE, x, y+h-size, w, size);

	g_object_unref(gc);
}



void make_channel_string(gchar *strch[GUI_CHANNEL_CNT], gboolean is_space)
{
  guint i = 0;
  gchar tmp_str[20];

  for(i=0; i<GUI_CHANNEL_CNT; i++)
  {
    memset(tmp_str, 0x00, sizeof(tmp_str));
    if(is_space == TRUE)
    {
      g_sprintf(tmp_str, "CH %d", (i+1));
    }
    else
    {
      g_sprintf(tmp_str, "CH%d", (i+1));
    }
    strch[i] = g_strdup(tmp_str);
  }
}





void free_channel_string(gchar *strch[GUI_CHANNEL_CNT])
{
  guint i = 0;

  for(i=0; i<GUI_CHANNEL_CNT; i++)
  {
    if(strch[i]) g_free(strch[i]);
    strch[i] = NULL;
  }
}


void make_channel_index_string(gchar *strch[GUI_CHANNEL_CNT])
{
  gchar tmp_str[20];
  guint i = 0;

  for(i=0; i<GUI_CHANNEL_CNT; i++)
  {
    memset(tmp_str, 0x00, sizeof(tmp_str));
    g_sprintf(tmp_str, "%d", (i+1));
    strch[i] = g_strdup(tmp_str);
  }
}



/*****************************************************************
 * FOR IP ADDRESS
 * ***************************************************************/
void convertIntToIP(guint *out, guint in)
{
	out[0] = (in >> 24) & 255;
	out[1] = (in >> 16) & 255;
	out[2] = (in >> 8) & 255;
	out[3] = in & 255;
}

guint convertIPToInt(guint ip[4])
{
	guint ret = 0;

	ret += (ip[0] << 24);
	ret += (ip[1] << 16);
	ret += (ip[2] << 8);
	ret += ip[3];

	return ret;
}







#if 0 //_ATM_MOUSE_
gboolean nftool_check_mouse_connect(void)
{
  struct stat stat_buf;
  gchar dev_name[128];

  int mouse_connect_flag = 0;

  memset(&stat_buf, 0x00, sizeof(stat_buf));
  memset(dev_name, 0x00, sizeof(dev_name));

  sprintf(dev_name, "%s", "/dev/input/mouse0");

  if (stat(dev_name, &stat_buf) < 0)
  {
    g_print("Not Connected Mouse \n");
    mouse_connect_flag = 0;
  }
  else
  {
    g_print("Connected Mouse \n");
     mouse_connect_flag = 1;
  }

  return mouse_connect_flag;
}


gboolean nftool_change_mouse_state(gboolean mouse_connect_flag)
{
  gboolean change_state = FALSE;

  if(nfui_mouse_connected_flag != mouse_connect_flag)
  {
    change_state = TRUE;
    nfui_mouse_connected_flag = mouse_connect_flag;
  }

  return change_state;
}



void nfui_set_mouse_connected_flag(gboolean mouse_connect_flag)
{
  nfui_mouse_connected_flag = mouse_connect_flag;
}
#endif

#if 1//defined(_SCREEN_CAPTURE_)
#define FB_WIDTH	1920
#define FB_HEIGHT	1080
#define FB_PATH		"/dev/fb1"
#define JPG_PATH	"/tmp/webra-info/cap.jpg"
gchar fb_rgb_src[FB_WIDTH*FB_HEIGHT*3];

static void process_screen_capture(void){
	gchar *dest = fb_rgb_src;
	FILE *fl = NULL;
	gchar buf[3];
	int i,j;
	GdkPixbuf *fb_pixbuf;

	if((fl = fopen(FB_PATH, "rb")) == NULL){
		g_message("FB open fail");
		return;
	}

	for(i=0; i<FB_HEIGHT; i++){
		for(j=0; j<FB_WIDTH; j++){
			if(fread(buf, 2, 1, fl) < 0)
				g_message("Fread FAIL");
			dest[2]= (buf[0] & 0x1f) << 3;
			dest[1]= ((buf[1] & 0x3) << 6) | ((buf[0] & 0xe0) >> 2);
			dest[0]= (buf[1] & 0x7c) << 1;
			dest += 3;
		}
	}

	fclose(fl);

	fb_pixbuf = gdk_pixbuf_new_from_data(fb_rgb_src, GDK_COLORSPACE_RGB, FALSE, 8, FB_WIDTH, FB_HEIGHT, FB_WIDTH*3, NULL, NULL);

	if(fb_pixbuf == NULL){
		g_message("gdk_pixbuf_new_from_data FAIL");
		return;
	}

	if(gdk_pixbuf_save(fb_pixbuf, JPG_PATH, "jpeg", NULL, "quality", "100", NULL) == FALSE)
		g_message("gdk_pixbuf_save FAIL");

	g_object_unref(fb_pixbuf);

	return;
}

static gboolean screen_capture(gpointer data)
{
	g_message("Screen_capture run");
	process_screen_capture();
	return TRUE;
}

gboolean screen_capture_init(void)
{
	if (!scm_is_enable_capture_mode()) return 0;

	g_timeout_add(10000, screen_capture, NULL);
	return 0;
}

#endif

gint screen_capture_manual()
{
    gchar cmd[2048];
    gchar tmp[256];
    gchar mount_path[256];

    gint media_cnt;
    MEDIA_INFO_T *media_info;
	guint i, cnt = 0;

	media_cnt = 0;
	media_info = scm_new_media_list(&media_cnt);
    if (!media_cnt) return -1;

    g_message("%s, %d, ENTER", __FUNCTION__, __LINE__);

	for (i = 0; i < media_cnt; i++)
	{
		if (scm_get_media_type(media_info[i].id) != MTYPE_USB) continue;

        memset(mount_path, 0x00, sizeof(mount_path));
        scm_get_mounted_path(media_info[i].id, mount_path, 128);

        memset(tmp, 0x00, sizeof(tmp));
		g_sprintf(tmp, "%s/%s", mount_path, "itx_enable_capture");

	    if (!ifn_is_file_exist(tmp)) continue;

        process_screen_capture();
        sync();
        sync();

        memset(tmp, 0x00, sizeof(tmp));
        memset(cmd, 0x00, sizeof(cmd));
        dtf_get_local_datetime_ex(time(0), tmp);
        g_sprintf(cmd, "cp -aR %s %s/itx_enable_capture/%s.jpg", JPG_PATH, mount_path, tmp);
        proxy_system(cmd, 1, 3);
        sync();
        sync();

		nf_dev_buzzer_on();
		sleep(1);
		nf_dev_buzzer_off();

        break;
	}

    g_message("%s, %d, LEAVE", __FUNCTION__, __LINE__);

    return 0;
}
