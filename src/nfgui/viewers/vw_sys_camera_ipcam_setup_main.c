#include "nf_afx.h"

#include "tools/nf_ui_tool.h"
#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/util.h"
#include "support/color.h"
#include "support/multi_language_support.h"

#include "objects/nftab.h"
#include "objects/nfobject.h"
#include "objects/nfbutton.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfimglabel.h"
#include "objects/nfcombobox.h"
#include "objects/nfspinbutton.h"
#include "objects/cw_slider.h"

#include "nf_api_ipcam.h"
#include "nf_api_cam.h"
#include "scm.h"
#include "vsm.h"
#include "ix_mem.h"

#include "vw.h"
#include "vw_vkeyboard.h"

#include "nf_api_live.h"

#include "vw_sys_camera_main.h"
#include "vw_sys_camera_ipcam_setup_main.h"
#include "vw_sys_camera_ipcam_internal.h"
#include "vw_sys_camera_ipcam_roi.h"


//DECLARE DBG_SYSTEM

#define DBG_LEVEL       9
#define DBG_MODULE      "LOCAL_VW"


#define CAM_SETUP_TITLE_X               (MENU_V_SUBTAB_PAGE_X)
#define CAM_SETUP_TITLE_Y               (2)
#define CAM_SETUP_TITLE_WIDTH           (240)
#define CAM_SETUP_TITLE_HEIGHT          (40)

NFOBJECT *g_on_off_expose_obj;
NFOBJECT *g_on_off_image_obj;
gboolean g_toggle[GUI_CHANNEL_CNT];

enum {
	IPCAM_SETUP_TAB_IMAGE = 0,
	IPCAM_SETUP_TAB_EXPOSURE,
//	IPCAM_SETUP_TAB_PTZ,
    IPCAM_SETUP_TAB_FOCUS,
    IPCAM_SETUP_TAB_ROI,
	IPCAM_SETUP_TAB_STREAM,
	IPCAM_SETUP_TAB_CONFIG,
	IPCAM_SETUP_TAB_MAX
};

static gchar *strCh[GUI_CHANNEL_CNT];

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_ch_obj = 0;
static NFOBJECT *g_model_obj = 0;
static NFOBJECT *g_tab;


static CAM_PROFILE_T g_prof[GUI_CHANNEL_CNT];

static void _update_camera_name(gint ch)
{
    gchar buf[128];

    memset(buf, 0x00, sizeof(buf));

	if (g_prof[ch].connected)
        sprintf(buf, "%s%s", "|   ", g_prof[ch].model.name);

	nfui_nflabel_set_text(g_model_obj, buf);
}

static gint _start_preview(gint ch)
{
	vsm_live_preview_start(1 << ch,
        MENU_V_SUBTAB_FIXED_X+MENU_V_IPCAMSET_SUBTAB_PAGE_X+MENU_V_IPCAMSET_SUBTAB_INNER_X,
        MENU_V_SUBTAB_FIXED_Y+MENU_V_IPCAMSET_SUBTAB_PAGE_Y+MENU_V_IPCAMSET_SUBTAB_INNER_Y,
        VIDEO_W,
        VIDEO_H);

    return 0;
}

static gint _stop_preview()
{
    vsm_live_preview_stop();
    return 0;
}

static gint _show_video_obj()
{
    IPCamImageSet_video_show();
    IPCamExposure_video_show();

    return 0;
}

static gint _hide_video_obj()
{
    IPCamImageSet_video_hide();
    IPCamExposure_video_hide();

    return 0;
}

static void _in_handler_ipcam_setup_main(gint page)
{
    gint ch = nfui_combobox_get_cur_index(g_ch_obj);

    if (page == IPCAM_SETUP_TAB_IMAGE)
    {
        DMSG(1, "IPCamImageSet_tab_in_handler");

        _start_preview(ch);
        _show_video_obj();
         IPCamImageSet_tab_in_handler();
    }
    else if (page == IPCAM_SETUP_TAB_EXPOSURE)
    {
        DMSG(1, "IPCamExposure_tab_in_handler");

        _start_preview(ch);
        _show_video_obj();
         IPCamExposure_tab_in_handler();
    }
    else if (page == IPCAM_SETUP_TAB_FOCUS)
    {
        DMSG(1, "IPCamFocus_tab_in_handler");

        IPCamFocus_tab_in_handler();
    }
    else if (page == IPCAM_SETUP_TAB_ROI)
    {
        DMSG(1, "IPCamROI_tab_in_handler");

        IPCamROI_tab_in_handler();
    }
    else if (page == IPCAM_SETUP_TAB_CONFIG)
    {
        DMSG(1, "IPCamDirectConfig_tab_in_handler");

         IPCamDirectConfig_tab_in_handler();
    }
    else if (page == IPCAM_SETUP_TAB_STREAM)
    {
        DMSG(1, "IPCamVideoStream_tab_in_handler");

         IPCamVideoStream_tab_in_handler();
    }
}

static void _out_handler_ipcam_setup_main(gint page)
{
    if (page == IPCAM_SETUP_TAB_IMAGE)
    {
        DMSG(1, "IPCamImageSet_tab_out_handler");

        _stop_preview();
        _hide_video_obj();
         IPCamImageSet_tab_out_handler();
    }
    else if (page == IPCAM_SETUP_TAB_EXPOSURE)
    {
        DMSG(1, "IPCamExposure_tab_out_handler");

        _stop_preview();
        _hide_video_obj();
         IPCamExposure_tab_out_handler();
    }
    else if (page == IPCAM_SETUP_TAB_FOCUS)
    {
        DMSG(1,"IPCamFocus_tab_out_handler");

        IPCamFocus_tab_out_handler();
    }
    else if (page == IPCAM_SETUP_TAB_ROI)
    {
        DMSG(1, "IPCamROI_tab_out_handler");

        IPCamROI_tab_out_handler();
    }
    else if (page == IPCAM_SETUP_TAB_CONFIG)
    {
        DMSG(1, "IPCamDirectConfig_tab_out_handler");

         IPCamDirectConfig_tab_out_handler();
    }
    else if (page == IPCAM_SETUP_TAB_STREAM)
    {
        DMSG(1, "IPCamVideoStream_tab_out_handler");

         IPCamVideoStream_tab_out_handler();
    }
}


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
			nfutil_draw_image(drawable, gc, MK_IMG_MENU_V_IPCAMSET_SUBTAB_FIXED_BG, x, y, -1, -1, NFALIGN_LEFT, 0);

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(10));
			gdk_draw_rectangle(drawable, gc, TRUE, x, y, 1, MENU_V_SUBTAB_FIXED_H-60);

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(11));
			gdk_draw_rectangle(drawable, gc, TRUE, x+1, y, 1, MENU_V_SUBTAB_FIXED_H-60);

			nfui_nfobject_gc_unref(gc);
		}
		break;

		case GDK_DELETE:
		{
            gint i;

            _destory_category_label();

    		for (i = 0; i < GUI_CHANNEL_CNT; i++)
    		{
	    		ifree(strCh[i]);
                _destory_row_info(i);
            }

			g_curwnd = 0;
		}
		break;

		default : break;
	}

	return FALSE;
}

static gboolean pre_subtab_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint cur_page;
	gint new_page;
	mb_type ret;

	if(evt->type == NFEVENT_TAB_BEFORE_CHANGE)
	{
    	gint cur_page;
    	gint new_page;

		cur_page = nfui_nftab_get_cur_page((NFTAB*)obj);
		new_page = nfui_nftab_get_new_page((NFTAB*)obj);

		if(cur_page == new_page)	return FALSE;

        _out_handler_ipcam_setup_main(cur_page);
        _in_handler_ipcam_setup_main(new_page);
    }

	return FALSE;
}

static gboolean pre_subtab_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_EXPOSE) {
		GdkGC *gc;
		GdkDrawable *drawable;
		guint x, y;

		drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
		gc = nfui_nfobject_get_gc(obj);

		nfui_nfobject_get_offset(obj, &x, &y);
		nfutil_draw_image(drawable, gc, MK_IMG_MENU_V_IPCAMSET_SUBTAB_PAGE_BG, x, y, -1, -1, NFALIGN_LEFT, 0);
		nfui_nfobject_gc_unref(gc);
	}

	return FALSE;
}

static gboolean _post_channel_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint ch;

	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {
        ch = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        _update_camera_name(ch);
        nfui_signal_emit(g_model_obj, GDK_EXPOSE, TRUE);

        scm_init_ptz_param(ch);

        IPCamImageSet_update_channel(ch);
        IPCamExposure_update_channel(ch);
//      IPCamPtz_update_channel();
        IPCamFocus_update_channel(ch);
        IPCamROI_update_channel(ch);
        IPCamVideoStream_update_channel(ch);
        IPCamDirectConfig_update_channel(ch);

    	if (nfui_nftab_get_cur_page((NFTAB*)g_tab) == IPCAM_SETUP_TAB_ROI) {
		 	IPCamROI_start_preview(ch);
		}
    	else {
			_start_preview(ch);
		}
	}
	else if ((evt->type == INFY_CAMDB_CHANGE_NOTIFY) || (evt->type == INFY_USRDB_CHANGE_NOTIFY)) {
		gchar strCh[STRING_SIZE_CAMTITLE+8];
		gint ch = nfui_combobox_get_cur_index(obj);
		gint i, j;

		nfui_combobox_remove_all((NFCOMBOBOX*)obj);

		for (i = 0; i<GUI_CHANNEL_CNT; i++) {
			memset(strCh, 0, (STRING_SIZE_CAMTITLE+8));
			j = sprintf(strCh, "%s%d - ", lookup_string("CH"), i+1);

			//DAL_get_camera_title(&strCh[j], (guint)i);
			var_get_camtitle(&strCh[j], (guint)i);
			nfui_combobox_append_data((NFCOMBOBOX*)obj, strCh);
		}
		nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, ch);
		nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
	}
	else if(evt->type == GDK_DELETE)  {
		uxm_unreg_imsg_event(obj, INFY_CAMDB_CHANGE_NOTIFY);
		uxm_unreg_imsg_event(obj, INFY_USRDB_CHANGE_NOTIFY);
	}

	return FALSE;
}


void init_IPCamSetup_page(NFOBJECT *parent)
{
	const gchar *strImage_h[2] =  {
				(MKB_IMG_SUBTAB_DIR_H_N_250),
				(MKB_IMG_SUBTAB_DIR_H_S_250)
	};

	NFOBJECT *nftab;
	NFOBJECT *tab_page[IPCAM_SETUP_TAB_MAX];
	NFOBJECT *obj;

	const gchar *strTabTitle[IPCAM_SETUP_TAB_MAX] = {"IMAGE SETTINGS",
	                                                 "EXPOSURE",
	                                                 "FOCUS COMPENSATION",
	                                                 "ROI SETTINGS",
	                                                 "STREAM SETUP",
	                                                 "DIRECT CONFIGURE"};
	const guint colidx[3] = {COLOR_IDX(189), COLOR_IDX(188), COLOR_IDX(188)};
	guint i, j;
	guint size_w, size_h;

	g_curwnd = nfui_nfobject_get_top(parent);

	nfui_nfobject_set_size(parent, (guint)MENU_V_SUBTAB_FIXED_W, (guint)MENU_V_SUBTAB_FIXED_H);
	nfui_nffixed_put((NFFIXED*)(parent->parent), parent, MENU_V_SUBTAB_FIXED_X, MENU_V_SUBTAB_FIXED_Y);
	nfui_regi_pre_event_callback(parent, pre_page_event_handler);

	for (i = 0; i<GUI_CHANNEL_CNT; i++)
	{
		strCh[i] = imalloc(sizeof(gchar) * (STRING_SIZE_CAMTITLE+8));
		j = sprintf(strCh[i], "%s%d - ", lookup_string("CH"), i+1);
		//DAL_get_camera_title(strCh[i]+j, i);
		var_get_camtitle(strCh[i]+j, i);

        memset(&g_prof[i], 0x00, sizeof(CAM_PROFILE_T));
        scm_get_cam_profile(i, &g_prof[i]);

        _update_ipcam_profile(i);
	}

    _make_category_label();
    _load_ipcam_db();
    _set_drawing_profile();

	obj = nfui_combobox_new(strCh, GUI_CHANNEL_CNT, 0);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
	nfui_nfobject_set_size(obj, CAM_SETUP_TITLE_WIDTH, CAM_SETUP_TITLE_HEIGHT);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, CAM_SETUP_TITLE_X, CAM_SETUP_TITLE_Y);
	nfui_regi_post_event_callback(obj, _post_channel_event_handler);
	uxm_reg_imsg_event(obj, INFY_CAMDB_CHANGE_NOTIFY);
	uxm_monitor_on_imsg_event(obj, INFY_CAMDB_CHANGE_NOTIFY);
	uxm_reg_imsg_event(obj, INFY_USRDB_CHANGE_NOTIFY);
	uxm_monitor_on_imsg_event(obj, INFY_USRDB_CHANGE_NOTIFY);
    g_ch_obj = obj;

	obj = nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(139));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(1));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 400, CAM_SETUP_TITLE_HEIGHT);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, CAM_SETUP_TITLE_X+CAM_SETUP_TITLE_WIDTH, CAM_SETUP_TITLE_Y);
    g_model_obj = obj;

    _update_camera_name(0);
    scm_init_ptz_param(0);

	nftab = nfui_nftab_new(IPCAM_SETUP_TAB_MAX, (gchar**)strImage_h, 250, 40, NFTAB_DIR_H, strTabTitle, colidx);
	nfui_nftab_set_pango_font((NFTAB*)nftab, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nftab_set_margin(nftab, 10);
	nfui_nfobject_show(nftab);
	nfui_nffixed_put((NFFIXED*)parent, nftab, MENU_V_SUBTAB_PAGE_X, 45);
	nfui_regi_pre_event_callback(nftab, pre_subtab_event_handler);
	g_tab = nftab;

	for(i = 0; i < IPCAM_SETUP_TAB_MAX; i++)
	{
		tab_page[i] = nfui_nffixed_new();
		nfui_nfobject_set_size(tab_page[i], MENU_V_IPCAMSET_SUBTAB_PAGE_W, MENU_V_IPCAMSET_SUBTAB_PAGE_H);
		nfui_nftab_regi_page((NFTAB*)nftab, tab_page[i], i);
		nfui_nffixed_put((NFFIXED*)parent, tab_page[i], MENU_V_IPCAMSET_SUBTAB_PAGE_X, MENU_V_IPCAMSET_SUBTAB_PAGE_Y);
		nfui_regi_pre_event_callback(tab_page[i], pre_subtab_page_event_handler);
	}

    DMSG(1, "VW_Init_IPCamImageSet_Page");
	VW_Init_IPCamImageSet_Page(tab_page[IPCAM_SETUP_TAB_IMAGE], 0, g_prof);

    DMSG(1, "VW_Init_IPCamExposure_Page");
	VW_Init_IPCamExposure_Page(tab_page[IPCAM_SETUP_TAB_EXPOSURE], 0, g_prof);

    DMSG(1, "VW_Init_IPCamPtz_Page");
//	VW_Init_IPCamPtz_Page(tab_page[IPCAM_SETUP_TAB_PTZ]);

    DMSG(1, "VW_Init_IPCamFocus_page");
    VW_Init_IPCamFocus_Page(tab_page[IPCAM_SETUP_TAB_FOCUS], 0);

    DMSG(1, "VW_Init_IPCamROI_Page");
	VW_Init_IPCamROI_Page(tab_page[IPCAM_SETUP_TAB_ROI], 0);

    DMSG(1, "VW_Init_IPCamVideoStream_Page");
	VW_Init_IPCamVideoStream_Page(tab_page[IPCAM_SETUP_TAB_STREAM], 0);

    DMSG(1, "VW_Init_IPCamDirectConfig_Page");
	VW_Init_IPCamDirectConfig_Page(tab_page[IPCAM_SETUP_TAB_CONFIG], 0, g_prof);

	nfui_nfobject_show(tab_page[0]);
}

static gboolean _init_preview(gpointer data)
{
    _start_preview(0);
//    _show_video_obj();
	return FALSE;
}

gint IPCamSetup_start_preview()
{
    _start_preview(0);
    _show_video_obj();
//	g_timeout_add(2000, _init_preview, 0);
	return 0;
}

gboolean IPCamSetup_tab_in_handler()
{
	gint ch = nfui_combobox_get_cur_index(g_ch_obj);

	if (nfui_nftab_get_cur_page((NFTAB*)g_tab) == IPCAM_SETUP_TAB_IMAGE)
    {
        DMSG(1, "IPCamImageSet_tab_in_handler");

        _start_preview(ch);
        _show_video_obj();
         IPCamImageSet_tab_in_handler();
    }
    else if (nfui_nftab_get_cur_page((NFTAB*)g_tab) == IPCAM_SETUP_TAB_EXPOSURE)
    {
        DMSG(1, "IPCamExposure_tab_in_handler");

    _start_preview(ch);
    _show_video_obj();
         IPCamExposure_tab_in_handler();
    }
    else if (nfui_nftab_get_cur_page((NFTAB*)g_tab) == IPCAM_SETUP_TAB_FOCUS)
    {
        DMSG(1, "IPCamFocus_tab_in_handler");

        IPCamFocus_tab_in_handler();
    }
    else if (nfui_nftab_get_cur_page((NFTAB*)g_tab) == IPCAM_SETUP_TAB_ROI)
    {
        DMSG(1, "IPCamROI_tab_in_handler");

        IPCamROI_tab_in_handler();
    }
    else if (nfui_nftab_get_cur_page((NFTAB*)g_tab) == IPCAM_SETUP_TAB_CONFIG)
    {
        DMSG(1, "IPCamDirectConfig_tab_in_handler");

         IPCamDirectConfig_tab_in_handler();
    }
    else if (nfui_nftab_get_cur_page((NFTAB*)g_tab) == IPCAM_SETUP_TAB_STREAM)
    {
        DMSG(1, "IPCamVideoStream_tab_in_handler");

         IPCamVideoStream_tab_in_handler();
    }

	return FALSE;
}

gboolean IPCamSetup_tab_out_handler()
{
	mb_type ret;

	if (nfui_nftab_get_cur_page((NFTAB*)g_tab) == IPCAM_SETUP_TAB_IMAGE)
    {
        DMSG(1, "IPCamImageSet_tab_out_handler");

    _stop_preview();
        _hide_video_obj();
         IPCamImageSet_tab_out_handler();
    }
    else if (nfui_nftab_get_cur_page((NFTAB*)g_tab) == IPCAM_SETUP_TAB_EXPOSURE)
    {
        DMSG(1, "IPCamExposure_tab_out_handler");

        _stop_preview();
        _hide_video_obj();
         IPCamExposure_tab_out_handler();
    }
    else if (nfui_nftab_get_cur_page((NFTAB*)g_tab) == IPCAM_SETUP_TAB_FOCUS)
    {
        DMSG(1,"IPCamFocus_tab_out_handler");

        IPCamFocus_tab_out_handler();
    }
    else if (nfui_nftab_get_cur_page((NFTAB*)g_tab) == IPCAM_SETUP_TAB_ROI)
    {
        DMSG(1, "IPCamROI_tab_out_handler");

        IPCamROI_tab_out_handler();
    }
    else if (nfui_nftab_get_cur_page((NFTAB*)g_tab) == IPCAM_SETUP_TAB_CONFIG)
    {
        DMSG(1, "IPCamDirectConfig_tab_out_handler");

         IPCamDirectConfig_tab_out_handler();
    }
    else if (nfui_nftab_get_cur_page((NFTAB*)g_tab) == IPCAM_SETUP_TAB_STREAM)
	{
        DMSG(1, "IPCamVideoStream_tab_out_handler");

         IPCamVideoStream_tab_out_handler();
    }

	return FALSE;
}

guint _get_copy_chmask(gint src_ch)
{
    guint copy_mask, dst_mask = 0;
    gint i;

    if (g_prof[src_ch].connected)
    {
        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            if (i != src_ch)
            {
                if ((strcmp(g_prof[src_ch].model.name, g_prof[i].model.name) == 0) && (strcmp(g_prof[src_ch].model.swver, g_prof[i].model.swver) == 0))
                {
                    dst_mask |= (1 << i);
                }
            }
        }
    }

    copy_mask = VW_Copy_Setting_camsetup(g_curwnd, src_ch, dst_mask, g_prof);

    return copy_mask;
}

void init_poe_on_off(gint ch)
{
    gint is_fail;

    DAL_get_cam_poe_onoff(&g_toggle[ch],ch);

    scm_set_ipcam_poe_onoff(ch,g_toggle[ch]);
    if(nf_live_get_poe_port_status() & (1 << ch))
        g_toggle[ch] = 1;
	else
	    g_toggle[ch] = 0;
}

void set_camera_toggle_on_off(gint ch)
{
    if(g_toggle[ch])
    {
        nfui_nfbutton_set_text((NFBUTTON *) g_on_off_image_obj, "POE POWER OFF");
        nfui_nfbutton_set_text((NFBUTTON *) g_on_off_expose_obj, "POE POWER OFF");
		nfui_signal_emit(g_on_off_image_obj, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_on_off_expose_obj, GDK_EXPOSE, TRUE);
    }
    else
    {
        nfui_nfbutton_set_text((NFBUTTON *) g_on_off_expose_obj,"POE POWER ON");
        nfui_nfbutton_set_text((NFBUTTON *) g_on_off_image_obj, "POE POWER ON");
		nfui_signal_emit(g_on_off_image_obj, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_on_off_expose_obj, GDK_EXPOSE, TRUE);
    }
}
