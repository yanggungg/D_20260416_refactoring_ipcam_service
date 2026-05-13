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
#include "viewers/objects/nfspinbutton.h"
#include "viewers/objects/cw_slider.h"

#include "vw_slider_set_value_popup.h"


#define SLIDER_SET_PP_SIZE_WID		(560)
#define SLIDER_SET_PP_SIZE_HEI		(250)
#define SLIDER_SET_PP_POS_X			(guint)((DISPLAY_ACTIVE_WIDTH - SLIDER_SET_PP_SIZE_WID)/2)
#define SLIDER_SET_PP_POS_Y 		(guint)((DISPLAY_ACTIVE_HEIGHT - SLIDER_SET_PP_SIZE_HEI)/2 - 10)

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_slider = 0;
static NFOBJECT *g_spin = 0;

static guint g_opt;
static gint *g_value = 0;

static gint g_retVal = 0;


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

static gboolean post_slider_event_handler (NFOBJECT *obj, GdkEvent *event, gpointer data)
{
    gchar strBuf[8];   

	switch(event->type) 
	{
		case GDK_BUTTON_PRESS :
		break;

		case GDK_LEAVE_NOTIFY :
		case GDK_BUTTON_RELEASE :
		case NFEVENT_CWSLIDER_CHANGED_RELEASE :		
		{
			*g_value = cw_slider_get_value(obj);
			
		    memset(strBuf, 0x00, sizeof(strBuf));
		    g_sprintf(strBuf, "%d", *g_value);			
			nfui_spin_button_set_text((NFSPINBUTTON*)g_spin, strBuf);
			nfui_signal_emit(g_spin, GDK_EXPOSE, TRUE);			
 		}
		break;		

		default : 
		break;
	}
	
	return FALSE;
	
}

static gboolean post_spin_event_handler (NFOBJECT *obj, GdkEvent *event, gpointer data)
{	
	if (event->type == NFEVENT_SPINBUTTON_CHANGED)
	{
		*g_value = nfui_spin_button_get_value((NFSPINBUTTON*)obj);
		cw_slider_set_value(g_slider, *g_value);
		nfui_signal_emit(g_slider, GDK_EXPOSE, TRUE);     
	}
	
	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *topwin;
		
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;			

		g_retVal = 0;

		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);
	}
	return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *topwin;
		gchar *pw;
		
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;			

		g_retVal = 1;

		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);
	}
	return FALSE;
}

gint VW_open_slider_set_value_popup(NFWINDOW *parent, gchar *title, guint opt, gint min, gint max, gint *value)
{
	NFOBJECT *main_wnd, *main_fixed;
	NFOBJECT *obj;
	guint pos_x, pos_y;
	guint wnd_w, wnd_h;
	int i;

    gchar strBuf[32];   

	wnd_w = SLIDER_SET_PP_SIZE_WID;
	wnd_h = SLIDER_SET_PP_SIZE_HEI;

 	g_value = value;
	g_opt = opt;

	main_wnd = nftool_create_popup_window(parent, SLIDER_SET_PP_POS_X, SLIDER_SET_PP_POS_Y, wnd_w, wnd_h, title, TRUE);
	nfui_regi_post_event_callback(main_wnd, post_main_win_event_handler);
	g_curwnd = main_wnd;

	main_fixed = ((NFWINDOW*)main_wnd)->child;
	nfui_regi_post_event_callback(main_fixed, post_main_bg_event_handler);

	pos_x = 20;
	pos_y = 70;

	memset(strBuf, 0x00, sizeof(strBuf));
	g_sprintf(strBuf, "[%d]", min);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MINI_SEMI_4), COLOR_IDX(389));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));			
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 80, 24);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

	memset(strBuf, 0x00, sizeof(strBuf));
	g_sprintf(strBuf, "[%d]", max);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MINI_SEMI_4), COLOR_IDX(389));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));			
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 80, 24);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+300, pos_y);

	pos_x += 40;
	pos_y += 30;

	obj = (NFOBJECT*)cw_slider_new(*value, 300, 40);
	cw_slider_set_range((CWSLIDER*)obj, min, max, max-min+1); 
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));		
	nfui_nfobject_set_size(obj, 300, 40);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_slider_event_handler);
	nfui_nfobject_show(obj);
	g_slider = obj;	

	pos_x += 350;

	obj = nfui_spinbutton_new_value_with_range(*value, min, max, 1);
    nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_POPUP_1);
	nfui_nfobject_set_size(obj, 120, 40);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);	
	nfui_regi_post_event_callback(obj, post_spin_event_handler);	
	nfui_nfobject_show(obj);	
	g_spin = obj;	

	pos_x = 0;
	pos_y = wnd_h - 70;

	obj = nftool_normal_button_create_type1("APPLY", 120);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, SLIDER_SET_PP_SIZE_WID/2-125, pos_y);
	nfui_regi_post_event_callback(obj, post_applybutton_event_handler);

	obj = nftool_normal_button_create_type1("CANCEL", 120);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, SLIDER_SET_PP_SIZE_WID/2+5, pos_y);
	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);
	
	nfui_nfobject_show(main_wnd);
	nfui_make_key_hierarchy(main_wnd);
	nfui_set_key_focus(obj, TRUE);

	gtk_main();

	return g_retVal;
}

