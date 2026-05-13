
#include "support/color.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/event_loop.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nfcheckbutton.h"
#include "objects/nflabel.h"
#include "objects/nftable.h"
#include "objects/nfvklabel.h"
#include "objects/nfcombobox.h"
#include "objects/nfimage.h"

#include "vw_fisheye_viewtype_add_popup.h"
#include "ix_mem.h"

#include "nf_api_live.h"


#define POPUP_SIZE_WID		    (1300)
#define POPUP_SIZE_HEI          (920)


static NFWINDOW *g_curwnd = 0;
static gint g_curr_ch = 0;


static gint _start_preview()
{
    guint win_x, win_y;
	guint ch_mask = 0;

    nfui_nfobject_get_window_pos((NFOBJECT*)g_curwnd, &win_x, &win_y);

	ch_mask |= (1 << g_curr_ch); 
	vsm_live_preview_start(ch_mask, win_x+20, win_y+64, 16*52, 16*52);
    return 0;
}

static gint _stop_preview()
{
    vsm_live_preview_stop();
    return 0;
}

static gint _print_fisheye_video_param()
{
	NF_FISHEYE_VIDEO_PARAM param;

	memset(&param, 0x00, sizeof(NF_FISHEYE_VIDEO_PARAM));
	nf_live_fisheye_get_video_param(0, &param);

//	g_message("%s, %d, enable:%d", __FUNCTION__, __LINE__, param.enable);
	g_message("%s, %d, mnt_type:%d", __FUNCTION__, __LINE__, param.mnt_type);
	g_message("%s, %d, view_type:%d", __FUNCTION__, __LINE__, param.view_type);

	return 0;
}

static gint _set_fisheye_video_param(gint enable, NF_FISHEYE_VIEW_TYPE view_type)
{
	NF_FISHEYE_VIDEO_PARAM param;

	memset(&param, 0x00, sizeof(NF_FISHEYE_VIDEO_PARAM));
	param.mnt_type = NF_FISHEYE_MOUNT_GROUND;
//	param.enable = enable;
	param.view_type = view_type;

	nf_live_fisheye_set_video_param(0, &param);
	return 0;
}

static gboolean post_view_type_radio_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_RADIO_GET_FOCUS_ALWAYS)
	{
		gint index;

		index = nfui_radio_button_get_index((NFBUTTON*)obj);

		if (index == 0) _set_fisheye_video_param(0, NF_FISHEYE_VIEW_SIGLE);
		else if (index == 1) _set_fisheye_video_param(1, NF_FISHEYE_VIEW_SIGLE);
		else if (index == 2) _set_fisheye_video_param(1, NF_FISHEYE_VIEW_QUAD); 
		else if (index == 3) _set_fisheye_video_param(1, NF_FISHEYE_VIEW_PANORAMA);
	}

	return FALSE;
}

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_DELETE)
	{
        _stop_preview();
		gtk_main_quit();
	}

	return FALSE;
}

static gboolean post_ok_button_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *topwin;

	if (event->type == GDK_BUTTON_RELEASE)
	{
		if (event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
	
		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);	
	}
	
	return FALSE;
}

static gboolean post_cancel_button_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *topwin;

	if (event->type == GDK_BUTTON_RELEASE)
	{
		if (event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
	
		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);	
	}
	
	return FALSE;
}

gint fisheye_viewtype_add_popup_open(NFOBJECT *parent, gint ch)
{
    NFOBJECT *main_wnd;
    NFOBJECT *main_fixed;
    NFOBJECT *tmp_fixed;   
    NFOBJECT *obj;

    GSList *slist = NULL;
    GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];

    gint pos_x, pos_y;
    guint size_w, size_h;


	radio_img[0] = nfui_get_image_from_file((IMG_N_POPUP_RADIO_OFF), NULL);
	radio_img[1] = nfui_get_image_from_file((IMG_O_POPUP_RADIO_ON), NULL);
	radio_img[2] = nfui_get_image_from_file((IMG_P_POPUP_RADIO_ON), NULL);
	radio_img[3] = nfui_get_image_from_file((IMG_D_POPUP_RADIO_OFF), NULL);

	_print_fisheye_video_param();

    g_curr_ch = ch;

	main_wnd = nftool_create_popup_window(parent, (1920-POPUP_SIZE_WID)/2, (1080-POPUP_SIZE_HEI)/2, POPUP_SIZE_WID, POPUP_SIZE_HEI, "FISHEYE VIEW TYPE", TRUE);
	nfui_regi_post_event_callback(main_wnd, post_main_win_event_handler);
	g_curwnd = (NFWINDOW*)main_wnd;

	main_fixed = ((NFWINDOW*)main_wnd)->child;

	pos_x = 20;
	pos_y = 64;

	tmp_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(tmp_fixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
	nfui_nfobject_set_size(tmp_fixed, 16*52-2, 16*52-2);
	nfui_nfobject_show(tmp_fixed);
	nfui_nffixed_put((NFFIXED*)main_fixed, tmp_fixed, pos_x, pos_y);

	pos_x += 16*52 + 40; 
	pos_y = 86;

    obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG3);
    nfui_nfimage_set_text((NFIMAGE*)obj, "NAME");
    nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
    nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

    pos_y += 46;

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nfobject_set_size(obj, 340, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put(main_fixed, obj, pos_x+20, pos_y);    

    pos_y += 90;

    obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG3);
    nfui_nfimage_set_text((NFIMAGE*)obj, "VIEW TYPE");
    nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
    nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

    pos_y += 42;

    nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

    obj = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
    nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
    nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+20+(40-size_w)/2, pos_y+(40-size_h)/2);
	nfui_regi_post_event_callback(obj, post_view_type_radio_event_handler);

    nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);
    slist = nfui_radio_button_get_group(NF_BUTTON(obj));

	obj = nfui_nflabel_new_with_pango_font("ORIGINAL VIEW", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 300, 40);
	nfui_nfobject_show(obj);		
	nfui_nffixed_put(main_fixed, obj, pos_x+60, pos_y);

    pos_y += 41;

    obj = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
    nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
    nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+20+(40-size_w)/2, pos_y+(40-size_h)/2);
	nfui_regi_post_event_callback(obj, post_view_type_radio_event_handler);

    nfui_radio_button_add_group(NF_BUTTON(obj), slist);

	obj = nfui_nflabel_new_with_pango_font("SINGLE VIEW", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 300, 40);
	nfui_nfobject_show(obj);		
	nfui_nffixed_put(main_fixed, obj, pos_x+60, pos_y);

    pos_y += 41;

    obj = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
    nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
    nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+20+(40-size_w)/2, pos_y+(40-size_h)/2);
	nfui_regi_post_event_callback(obj, post_view_type_radio_event_handler);

    nfui_radio_button_add_group(NF_BUTTON(obj), slist);

	obj = nfui_nflabel_new_with_pango_font("QUAD VIEW", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 300, 40);
	nfui_nfobject_show(obj);		
	nfui_nffixed_put(main_fixed, obj, pos_x+60, pos_y);

    pos_y += 41;

    obj = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
    nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
    nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+20+(40-size_w)/2, pos_y+(40-size_h)/2);
	nfui_regi_post_event_callback(obj, post_view_type_radio_event_handler);

    nfui_radio_button_add_group(NF_BUTTON(obj), slist);

	obj = nfui_nflabel_new_with_pango_font("PANORAMA VIEW", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 300, 40);
	nfui_nfobject_show(obj);		
	nfui_nffixed_put(main_fixed, obj, pos_x+60, pos_y);       

	obj = nftool_normal_button_create_type1("OK", 174);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, POPUP_SIZE_WID-20-174-8-174, POPUP_SIZE_HEI-66);
	nfui_regi_post_event_callback(obj, post_ok_button_event_handler);

	obj = nftool_normal_button_create_type1("CANCEL", 174);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, POPUP_SIZE_WID-20-174, POPUP_SIZE_HEI-66);
	nfui_regi_post_event_callback(obj, post_cancel_button_event_handler);

	nfui_nfobject_show(main_wnd);
	nfui_make_key_hierarchy(main_wnd);

	nfui_page_open(PGID_POPUPWND, main_wnd, nfui_get_last_user());
 
    _start_preview();

	gtk_main();

	nfui_page_close(PGID_POPUPWND, main_wnd);

    return 0;
}
