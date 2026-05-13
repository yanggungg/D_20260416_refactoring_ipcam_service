
#include "nf_afx.h"


#include "../support/event_loop.h"
#include "../support/nf_ui_font.h"
#include "../support/nf_ui_image.h"
#include "../support/nf_ui_page_manager.h"
#include "../support/nf_ui_color.h"
#include "../support/color.h"
#include "../support/util.h"

#include "../tools/nf_ui_tool.h"

#include "modules/ssm.h"

#include "../viewers/objects/nfobject.h"
#include "../viewers/objects/nfwindow.h"
#include "../viewers/objects/nffixed.h"
#include "../viewers/objects/nflabel.h"
#include "../viewers/objects/nfbutton.h"
#include "../viewers/objects/nfcheckbutton.h"
#include "../viewers/objects/nfimage.h"

#include "vw_search_main.h"
#include "vw_playback_start_menu.h"



#define STAUSBAR_H					(108)

#define START_MENU_SIZE_W			(323)
#define START_MENU_SIZE_H			(160)
#define START_MENU_POS_X			(0)
#define START_MENU_POS_Y			(DISPLAY_ACTIVE_HEIGHT-STAUSBAR_H-START_MENU_SIZE_H-4)

#define START_BUTTON_SIZE_W			(300)

#define MENU_FRONT_MARGIN			(8)
#define MENU_REAR_MARGIN			(15)

#define MENU_DEF_SPACE 				(MENU_FRONT_MARGIN + MENU_REAR_MARGIN)

#define MENU_TEXT_FRONT_MARGIN		(40)
#define MENU_TEXT_REAR_MARGIN		(0)

#define MENU_BUTTON_DEF_SPACE 		(MENU_TEXT_FRONT_MARGIN + MENU_TEXT_REAR_MARGIN)


enum {
	SEARCH_MENU = 0,
	ARCH_MENU,
	LIVE_MENU,
	START_MENU_CNT
};

typedef struct _MENU_CONF {
	guint wsize_w;									// window width 
	guint msize_w;									// button width

	GdkPixbuf *menu_img[START_MENU_CNT][NFOBJECT_STATE_COUNT];	// images

	NFOBJECT *start_win;
} MENU_CONF;

static NFWINDOW *g_curwnd = 0;

static MENU_CONF conf;
static const gchar *g_strBtn[START_MENU_CNT] = {"SEARCH", "ARCHIVING", "LIVE"};

static NFOBJECT *g_menu[START_MENU_CNT];

static gboolean set_size(MENU_CONF *pconf);
static gboolean set_image(MENU_CONF *pconf);
static gboolean post_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data);
static gboolean post_window_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data);



static gboolean init_menu_conf(MENU_CONF *pconf)
{
	memset(pconf, 0x00, sizeof(MENU_CONF));

	if(!set_size(pconf))  return FALSE;
	if(!set_image(pconf)) return FALSE;

	return TRUE;
}

static gboolean set_size(MENU_CONF *pconf)
{
	guint str_w = 0;
	guint tmp_w = 0;
	gint i;

	for(i=0; i<START_MENU_CNT; i++) {
		tmp_w = nfutil_string_width(1, NULL, nffont_get_pango_font(NFFONT_LARGE_SEMI), g_strBtn[i], NORMAL_SPACING);
		
		if(tmp_w > str_w)
			str_w = tmp_w;	
	}

	if(str_w == 0) return FALSE;

	// set button width
	if(str_w < START_BUTTON_SIZE_W) 	pconf->msize_w = START_BUTTON_SIZE_W;
	else						   	pconf->msize_w = (str_w + MENU_BUTTON_DEF_SPACE);
	
	// set window width
	if(pconf->msize_w < START_MENU_SIZE_W) pconf->wsize_w = START_MENU_SIZE_W;
	else						   	pconf->wsize_w = (pconf->msize_w + MENU_DEF_SPACE);

	return TRUE;
}

static gboolean set_image(MENU_CONF *pconf)
{
	guint i;

    // button bg
	for (i=0; i<START_MENU_CNT; i++)
	{
		pconf->menu_img[i][0] = nf_ui_create_image_button_no_alpha(MKB_IMG_PLAYBACK_START_N_BUTTON, pconf->msize_w, IMG_N_CLICK_POPUP_MENU_L, IMG_N_CLICK_POPUP_MENU_M, IMG_N_CLICK_POPUP_MENU_R);;
		pconf->menu_img[i][1] = nf_ui_create_image_button_no_alpha(MKB_IMG_PLAYBACK_START_O_BUTTON, pconf->msize_w, IMG_O_CLICK_POPUP_MENU_L, IMG_O_CLICK_POPUP_MENU_M, IMG_O_CLICK_POPUP_MENU_R);;
		pconf->menu_img[i][2] = nf_ui_create_image_button_no_alpha(MKB_IMG_PLAYBACK_START_P_BUTTON, pconf->msize_w, IMG_P_CLICK_POPUP_MENU_L, IMG_P_CLICK_POPUP_MENU_M, IMG_P_CLICK_POPUP_MENU_R);;
		pconf->menu_img[i][3] = nf_ui_create_image_button_no_alpha(MKB_IMG_PLAYBACK_START_D_BUTTON, pconf->msize_w, IMG_D_CLICK_POPUP_MENU_L, IMG_D_CLICK_POPUP_MENU_M, IMG_D_CLICK_POPUP_MENU_R);;
	}
	
	return TRUE;
}



static gboolean _search_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *parent = NULL;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		vw_playback_out_search_open();
	}	
	return FALSE;
}

static gboolean _arch_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *parent = NULL;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;
	
		vw_playback_out_arch_open();
	}
		
	return FALSE;
}

static gboolean _live_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *parent = NULL;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		vw_playback_out_live_open();
	}	
	return FALSE;
}



static gboolean post_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if (evt->type == GDK_EXPOSE) 
	{
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
    
		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(200));

		gdk_draw_rectangle(drawable,
				gc,
				TRUE,
				8, 54,
				300, 1);

		gdk_draw_rectangle(drawable,
				gc,
				TRUE,
				8, 106,
				300, 1);

		nfui_nfobject_gc_unref(gc);
	}
	else if (evt->type == GDK_DELETE)
	{
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
	}

	return FALSE;
}

static gboolean post_window_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case GDK_EXPOSE:
		{
		}
		break;

		case NFOUTEVT_BUTTON_PRESS:
		{
			if(evt->button.button == 3)
				return FALSE;

			nfui_nfobject_hide(obj);
			nfui_page_close(PGID_PLAYBACK_START_MENU, conf.start_win);				
		}
		break;

		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
		{
			GdkEventKey *kevt;
			KEYPAD_KID kpid;

			kevt = (GdkEventKey*)evt;
			kpid = (KEYPAD_KID)kevt->keyval;

			if (kpid == KEYPAD_EXIT)
			{
				nfui_nfobject_hide(obj);
				nfui_page_close(PGID_PLAYBACK_START_MENU, conf.start_win);
				return TRUE;				
			}
		}
		break;
		
		case GDK_DELETE:
			g_curwnd = 0;
			nfui_page_close(PGID_PLAYBACK_START_MENU, conf.start_win);
		break;
	}

	return FALSE;
}



///////////////////////////////////////////////////////////////////////
//
//
//
//

NFOBJECT* vw_playback_start_menu_open(NFWINDOW *parent)
{
	NFOBJECT *fixed1;
	NFOBJECT *nfwidget;
	gint size_w, size_h;
	guint i, pos_x = 0, pos_y = 0;
	GdkPixbuf *icon_img[START_MENU_CNT][NFOBJECT_STATE_COUNT];
	guint font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(340), 
											COLOR_IDX(341), 
											COLOR_IDX(343), 
											COLOR_IDX(344)};

	init_menu_conf(&conf);

	icon_img[SEARCH_MENU][0] = nfui_get_image_from_file((IMG_N_START_MENU_SEARCH), NULL);
	icon_img[SEARCH_MENU][1] = nfui_get_image_from_file((IMG_O_START_MENU_SEARCH), NULL);
	icon_img[SEARCH_MENU][2] = nfui_get_image_from_file((IMG_P_START_MENU_SEARCH), NULL);
	icon_img[SEARCH_MENU][3] = nfui_get_image_from_file((IMG_D_START_MENU_SEARCH), NULL);

	icon_img[ARCH_MENU][0] = nfui_get_image_from_file((IMG_N_START_MENU_ARCH), NULL);
	icon_img[ARCH_MENU][1] = nfui_get_image_from_file((IMG_O_START_MENU_ARCH), NULL);
	icon_img[ARCH_MENU][2] = nfui_get_image_from_file((IMG_P_START_MENU_ARCH), NULL);
	icon_img[ARCH_MENU][3] = nfui_get_image_from_file((IMG_D_START_MENU_ARCH), NULL);

	icon_img[LIVE_MENU][0] = nfui_get_image_from_file((IMG_N_START_MENU_LIVE), NULL);
	icon_img[LIVE_MENU][1] = nfui_get_image_from_file((IMG_O_START_MENU_LIVE), NULL);
	icon_img[LIVE_MENU][2] = nfui_get_image_from_file((IMG_P_START_MENU_LIVE), NULL);
	icon_img[LIVE_MENU][3] = nfui_get_image_from_file((IMG_D_START_MENU_LIVE), NULL);

	/* window */
	conf.start_win = (NFOBJECT*)nfui_nfwindow_new(parent, 
				(guint)START_MENU_POS_X, (guint)START_MENU_POS_Y, (guint)START_MENU_SIZE_W, (guint)START_MENU_SIZE_H);
	g_curwnd = conf.start_win;
	nfui_regi_post_event_callback(conf.start_win, post_window_event_cb);
	nfui_nfobject_modify_bg(conf.start_win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfwindow_use_outside_evt((NFWINDOW*)conf.start_win, TRUE);
	nfui_nfwindow_set_mask((NFWINDOW*)conf.start_win, GDK_BUTTON_PRESS, TRUE);


	/* fixed */
	fixed1 = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed1, START_MENU_SIZE_W, START_MENU_SIZE_H);
	nfui_regi_post_event_callback(fixed1, post_fixed_event_cb);
	nfui_nfobject_show(fixed1);


	pos_x = 8;
	pos_y = 8;


	for (i = 0; i < START_MENU_CNT; i++)
	{
		nfwidget = (NFOBJECT*)nfui_nfbutton_new_with_param(conf.menu_img[i], g_strBtn[i]);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(nfwidget), NFALIGN_LEFT, (DISPLAY_IS_D1 ? 4 : 10));
		nfui_nfbutton_set_pango_font((NFBUTTON*)nfwidget, nffont_get_pango_font(NFFONT_LARGE_SEMI), (guint*)font_color);
		nfui_nfbutton_set_icon_image((NFBUTTON*)nfwidget, icon_img[i]);
		nfui_nfobject_set_size(nfwidget, START_BUTTON_SIZE_W, MENU_TEXT_FRONT_MARGIN);
		nfui_nfobject_show(nfwidget);
		nfui_nffixed_put((NFFIXED*)fixed1, nfwidget, pos_x, pos_y);

		if (i == SEARCH_MENU)
			nfui_regi_post_event_callback(nfwidget, _search_button_event_handler);
		else if (i == ARCH_MENU)
			nfui_regi_post_event_callback(nfwidget, _arch_button_event_handler);
		else if (i == LIVE_MENU)
			nfui_regi_post_event_callback(nfwidget, _live_button_event_handler);

		pos_y += 52;	

		g_menu[i] = nfwidget;
	}


	nfui_nfwindow_add((NFWINDOW*)conf.start_win, fixed1);
	nfui_run_main_event_handler(conf.start_win);
	nfui_nfobject_show(conf.start_win);	
	nfui_make_key_hierarchy((NFWINDOW*)conf.start_win);	
	nfui_set_key_focus(g_menu[LIVE_MENU], TRUE);
	
	nfui_nfobject_hide(conf.start_win);
	
	return conf.start_win;
}


gboolean vw_playback_start_menu_refresh(void)
{
	gint i, j;
	gint auth[] = {USR_AUTH_SEARCH, 
				USR_AUTH_ARCHIVE};

	for (i = 0, j = 0; i < START_MENU_CNT; i++) {
		if(i != LIVE_MENU) {
			if(!ssm_check_access_auth(auth[j++]))
				nfui_nfobject_disable(g_menu[i]);
			else
				nfui_nfobject_enable(g_menu[i]);
		}
	}

	if (nfui_nfobject_is_shown(conf.start_win))
	{
		nfui_nfobject_hide(conf.start_win);	
		nfui_page_close(PGID_PLAYBACK_START_MENU, conf.start_win);
	}
	else
	{
		nfui_nfobject_show(conf.start_win);
		nfui_page_open(PGID_PLAYBACK_START_MENU, conf.start_win, nfui_get_last_user());	
	}
}

