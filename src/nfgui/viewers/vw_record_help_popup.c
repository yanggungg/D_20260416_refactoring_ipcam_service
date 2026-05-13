
#include "nf_afx.h"


#include "../support/event_loop.h"
#include "../support/nf_ui_font.h"
#include "../support/nf_ui_image.h"
#include "../support/color.h"
#include "../support/util.h"
#include "../support/multi_language_support.h"

#include "../tools/nf_ui_tool.h"

#include "../viewers/objects/nfobject.h"
#include "../viewers/objects/nfwindow.h"
#include "../viewers/objects/nffixed.h"
#include "../viewers/objects/nflabel.h"
#include "../viewers/objects/nfbutton.h"
#include "../viewers/objects/nfimage.h"
#include "../viewers/objects/nftable.h"

#include "support/nf_ui_page_manager.h"
#include "vw_record_help_popup.h"

#define HELP_WIN_H			(260)

/*
#define HELP_STR1 "If you want to set the time interval, drag the mouse to the time table at the top of the screen and a recording information setting window should appear."
#define HELP_STR2 "In the recording information settings window, you can set the desired resolution and quality, FPS, audio recording, and whether to save or not."
#define HELP_STR3 "If there is a different setting value, the time table will be displayed in a different color."
#define HELP_STR4 "If you hover the mouse pointer over the top of the time table, the corresponding time zone information will be displayed for the recording settings."
#define HELP_STR5 "Change the mode of schedule in operation mode page to change hourly, daily setup."

#define HELP_STR_CNT (5)
*/

static NFWINDOW *g_curwnd = 0;
//static gchar bg_img_name[64];

static gboolean post_main_wnd_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
		gtk_main_quit();
	}

	return FALSE;
}

static gboolean post_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;
	switch(evt->type)
	{
		case GDK_EXPOSE :
		{
			drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
			gc = nfui_nfobject_get_gc(obj);	

			nfui_nfobject_get_size(obj, &size_w, &size_h);
			pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
			nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
		}
		break;

		case GDK_DELETE:
		{
			nfui_nfobject_get_size(obj, &size_w, &size_h);
			nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
		}
		break;
			
		default :
			break;
	}

	return FALSE;
}

static gboolean _post_exit_btn_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *topwin;

	if(event->type == GDK_BUTTON_RELEASE)
	{
		if(event->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);	
	}
	
	return FALSE;
}

gint vw_record_help_popup_open(NFWINDOW *parent, gint mx, gint my, gchar **strhelp, gint strCnt)
{
    NFOBJECT *win = NULL;
    NFOBJECT *fixed;
    NFOBJECT *obj;

	gint i;
	gint pos_x, pos_y;	
    gint win_w = 0, max_str_w = 0;		
	//gchar *strhelp[HELP_STR_CNT] = {HELP_STR1, HELP_STR2, HELP_STR3, HELP_STR4, HELP_STR5};
    gchar strTmp[10];

	GdkColor bg_color1 = UX_COLOR(200);


    for (i = 0; i < strCnt; i++)
    {
        max_str_w = nfutil_string_width(1, NULL, nffont_get_pango_font(NFFONT_SMALL_SEMI), strhelp[i], NORMAL_SPACING);
    
        if (max_str_w > win_w)
            win_w = max_str_w;
    }

    win_w += 80;   

    if (mx + win_w >= 1920) win_w = 1920 - mx;

	/*
	memset(bg_img_name, 0x00, sizeof(bg_img_name));
	g_sprintf(bg_img_name, "MK_IMG_HELP_POPUP_BG_%d_%d", win_w, HELP_WIN_H);

	nf_ui_create_image_popup_method(bg_img_name, win_w, HELP_WIN_H, 
			POPUP_MENU_BG1, POPUP_MENU_BG2, POPUP_MENU_BG3,
			POPUP_MENU_BG7, POPUP_MENU_BG8, POPUP_MENU_BG9,
			POPUP_MENU_BG4, POPUP_MENU_BG6, NULL, bg_color1);
	*/

	win = (NFOBJECT*)nfui_nfwindow_new(parent, mx, my-HELP_WIN_H, (guint)win_w, (guint)HELP_WIN_H);
	nfui_nfobject_modify_bg(win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_regi_post_event_callback(win, post_main_wnd_event_handler);	
	g_curwnd = win;

	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, win_w, HELP_WIN_H);
	nfui_nfobject_show(fixed);
	nfui_regi_post_event_callback(fixed, post_main_fixed_event_handler);

	pos_x = 10;
	pos_y = 16;

    for (i = 0; i < strCnt; i++)
    {     
    	pos_x = 10;
    
        memset(strTmp, 0x00, sizeof(strTmp));
        g_sprintf(strTmp, "%d. ", i+1);

    	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTmp, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(206));
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    	nfui_nfobject_set_size(obj, 36, 28);
    	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);

        pos_x += 36;
    
    	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strhelp[i], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(206));
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
    	nfui_nfobject_set_size(obj, win_w-60, 28);
    	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    	pos_y += 28;
    }

	obj = nftool_normal_button_create_type1("EXIT", 174);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, (win_w-174)/2, HELP_WIN_H-65);
	nfui_regi_post_event_callback(obj, _post_exit_btn_event_handler);

	nfui_nfwindow_add((NFWINDOW*)win, fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);

	nfui_make_key_hierarchy(win);
    nfui_set_key_focus(obj, TRUE);

	nfui_page_open(PGID_RECORD_HELP_POPUP, win, nfui_get_last_user());

	gtk_main();

	nfui_page_close(PGID_RECORD_HELP_POPUP, win);	
}

