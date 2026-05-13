#include "nf_afx.h"


#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "tools/nf_ui_tool.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfimage.h"
#include "viewers/objects/nfbutton.h"

#include "vw_install_mode_warning_popup.h"

//#define INSTALL_WARNING_PP_SIZE_WID		(1015)
//#define INSTALL_WARNING_PP_SIZE_HEI		(450)
//#define INSTALL_WARNING_PP_POS_X		((DISPLAY_ACTIVE_WIDTH- INSTALL_WARNING_PP_SIZE_WID)/2)
//#define INSTALL_WARNING_PP_POS_Y 		((DISPLAY_ACTIVE_HEIGHT - INSTALL_WARNING_PP_SIZE_HEI)/2 - 10)

//#define INSTALL_WARNING_PP_LABEL_WID	(INSTALL_WARNING_PP_SIZE_WID - 40)
#define INSTALL_WARNING_PP_LABEL_POS_X	(20)

#define WARN_STR1       "Selecting 'OPEN MODE' opens all network ports, including camera connection ports."
#define WARN_STR2		"Therefore, the transmission quality of the camera and recording quality may be affected\nby other telecommunications equipment and/or the network environment."
#define WARN_STR3       "Also, using this mode requires the user to have knowledge of network equipment\nand network building/operating experience."
#define WARN_STR4       "Pressing 'CONTINUE' will restart the system and change network settings.\nDo you want to continue?"

static NFWINDOW *g_curwnd = 0;
static mb_type g_retVal = 0;

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
	    g_curwnd = 0;
		gtk_main_quit();
	}
	return FALSE;
}

static gboolean post_main_bg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	gint img_w, img_h;

	if(evt->type == GDK_EXPOSE) 
	{
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

		nfui_nfobject_gc_unref(gc);
	}

	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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

static gboolean post_continuebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *topwin;
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		g_retVal = 1;
		topwin = nfui_nfobject_get_top(obj);

		nfui_nfobject_destroy(topwin);
	}
	return FALSE;
}



mb_type VW_Open_OpenMode_Warning_Popup(NFWINDOW *parent)
{
	g_retVal = 0;
	gint str_h, pos_y;
	gint win_x, win_y;
	gint win_w, win_h;
	NFOBJECT *main_wnd, *main_fixed;
	NFOBJECT *obj = NULL;
	NFOBJECT *icon = NULL;

	win_w = 1015;
	win_h = 470;
	win_x = (DISPLAY_ACTIVE_WIDTH- win_w)/2;
	win_y = (DISPLAY_ACTIVE_HEIGHT - win_h)/2 - 10;


	main_wnd = nftool_create_popup_window(parent, win_x, win_y, win_w, win_h, "WARNING", TRUE);
	nfui_regi_post_event_callback(main_wnd, post_main_win_event_handler);
	g_curwnd = main_wnd;

	main_fixed = ((NFWINDOW*)main_wnd)->child;
	nfui_regi_post_event_callback(main_fixed, post_main_bg_event_handler);

	icon   = nfui_nfimage_new(IMG_ERROR_ICON);
	if(icon) 
	{
		nfui_nfobject_show(icon);
		nfui_nffixed_put((NFFIXED*)main_fixed, icon, ((win_w-64)/2), 55);
	}
	pos_y = 150;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(WARN_STR1, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	str_h = nfutil_string_height(NULL, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), WARN_STR1, NORMAL_SPACING) + 6;
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_set_size(obj, win_w-40, str_h);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, INSTALL_WARNING_PP_LABEL_POS_X, pos_y);
	pos_y += str_h;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(WARN_STR2, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	str_h = nfutil_string_height(NULL, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), WARN_STR2, NORMAL_SPACING) + 6;
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_set_size(obj, win_w-40, str_h);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, INSTALL_WARNING_PP_LABEL_POS_X, pos_y);
	pos_y += str_h;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(WARN_STR3, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	str_h = nfutil_string_height(NULL, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), WARN_STR3, NORMAL_SPACING) + 6;
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_set_size(obj, win_w-40, str_h);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, INSTALL_WARNING_PP_LABEL_POS_X, pos_y);
	pos_y += str_h;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(WARN_STR4, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	str_h = nfutil_string_height(NULL, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), WARN_STR4, NORMAL_SPACING) + 6;
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_set_size(obj, win_w-40, str_h);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, INSTALL_WARNING_PP_LABEL_POS_X, pos_y);


	pos_y = win_h - 70;
	obj = nftool_normal_button_create_type1("CONTINUE", 220);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 280, pos_y);
	nfui_regi_post_event_callback(obj, post_continuebutton_event_handler);

	obj = nftool_normal_button_create_type1("CANCEL", 220);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 510, pos_y);
	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);

	nfui_nfobject_show(main_wnd);
	nfui_make_key_hierarchy(main_wnd);
	nfui_set_key_focus(obj, TRUE);


	gtk_main();

	return g_retVal;
}

mb_type VW_Open_CloseMode_Warning_Popup(NFWINDOW *parent)
{
	g_retVal = 0;
	gint str_h, pos_y;
	gint win_x, win_y;
	gint win_w, win_h;
	NFOBJECT *main_wnd, *main_fixed;
	NFOBJECT *obj = NULL;
	NFOBJECT *icon = NULL;

	win_w = 815;
	win_h = 320;
	win_x = (DISPLAY_ACTIVE_WIDTH- win_w)/2;
	win_y = (DISPLAY_ACTIVE_HEIGHT - win_h)/2 - 10;

	main_wnd = nftool_create_popup_window(parent, win_x, win_y, win_w, win_h, "WARNING", TRUE);
	nfui_regi_post_event_callback(main_wnd, post_main_win_event_handler);

	main_fixed = ((NFWINDOW*)main_wnd)->child;
	nfui_regi_post_event_callback(main_fixed, post_main_bg_event_handler);

	icon   = nfui_nfimage_new(IMG_ERROR_ICON);
	if(icon) 
	{
		nfui_nfobject_show(icon);
		nfui_nffixed_put((NFFIXED*)main_fixed, icon, ((win_w-64)/2), 55);
	}
	pos_y = 150;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(WARN_STR4, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	str_h = nfutil_string_height(NULL, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), WARN_STR4, NORMAL_SPACING) + 6;
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_set_size(obj, win_w-40, str_h);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, INSTALL_WARNING_PP_LABEL_POS_X, pos_y);

	pos_y = win_h - 70;

	obj = nftool_normal_button_create_type1("CONTINUE", 220);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, win_w/2 - 225, pos_y);
	nfui_regi_post_event_callback(obj, post_continuebutton_event_handler);

	obj = nftool_normal_button_create_type1("CANCEL", 220);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, win_w/2 + 5, pos_y);
	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);
	
	nfui_nfobject_show(main_wnd);
	nfui_make_key_hierarchy(main_wnd);
	nfui_set_key_focus(obj, TRUE);


	gtk_main();

	return g_retVal;
}



mb_type VW_Open_InstallMode_Warning_Popup(NFWINDOW *parent, gint mode)
{
	mb_type ret = 0;

	if(mode == POPUP_MODE_OPEN)
		ret = VW_Open_OpenMode_Warning_Popup(parent);
	else
		ret = VW_Open_CloseMode_Warning_Popup(parent);
	
	return ret;
}

