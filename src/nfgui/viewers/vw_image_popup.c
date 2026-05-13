#include "nf_afx.h"

#include "nf_network.h"
#include "nf_util_netif.h"
#include "nf_api_ipcam.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "tools/nf_ui_tool.h"

#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"
#include "objects/nfimage.h"

#include "vw_image_popup.h"
#include "ix_mem.h"
#include "scm.h"


#define GAP_UPLINE_IMG          (30)
#define GAP_IMG_URL             (5)
#define GAP_URL_BTN             (15)
#define GAP_BTN_BOTLINE         (15)
#define MENU_GAP                (5)


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_prt_item;
static GdkPixbuf *g_pbuf = NULL;
static gint g_pbuf_w;
static gint g_pbuf_h;
static gint g_wnd_pos_x;
static gint g_wnd_pos_y;
static gint ret;
static gint g_btn_size_h;
static gint g_btn_pos_y;
static gint g_wnd_size_h;
static gint g_wnd_size_w;
static gint g_url_size_h;
static gint g_url_size_w;
static gint g_url_pos_y;


static void _init_global_value(NFOBJECT *parent_item, GdkPixbuf *image, gint wnd_pos_x, gint wnd_pos_y, gint img_w, gint img_h)
{
    g_prt_item = parent_item;
    g_pbuf = image;
    g_pbuf_w = img_w;
    g_pbuf_h = img_h;
    g_wnd_pos_x = wnd_pos_x;
    g_wnd_pos_y = wnd_pos_y;
    g_url_size_h = 30;
    g_url_pos_y = GAP_UPLINE_IMG + g_pbuf_h + GAP_IMG_URL;
    g_btn_size_h = 40;
    g_wnd_size_h = GAP_UPLINE_IMG + g_pbuf_h + GAP_IMG_URL + g_url_size_h + GAP_URL_BTN + g_btn_size_h + GAP_BTN_BOTLINE;
    g_wnd_size_w = g_wnd_size_h;
    g_url_size_w = g_wnd_size_w - 20;
    g_btn_pos_y = g_wnd_size_h - GAP_BTN_BOTLINE - g_btn_size_h;
	
	if (ivsc.vendor_code == 43)
		g_wnd_size_h = GAP_UPLINE_IMG + g_pbuf_h + GAP_IMG_URL + g_url_size_h;
}

static void _get_wnd_position(gint *wnd_pos_x, gint *wnd_pos_y)
{
    gint x, y, w, h;
    
    LiveStart_Popup_Pos(&x, &y);
    LiveStart_Popup_Size(&w, &h);

    *wnd_pos_x = w + 5;
    *wnd_pos_y = y;

}

static gboolean post_main_bg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkGC *gc;	
	GdkDrawable *drawable;
	GdkPixbuf *pbuf;

	if(evt->type == GDK_EXPOSE) 
	{
        drawable = nfui_nfobject_get_window(obj);
        gc = nfui_nfobject_get_gc(obj);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG_NO_LINE, g_wnd_size_w, g_wnd_size_h);
		nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
		if(g_pbuf)
		{
    		nfutil_draw_pixbuf(drawable, gc, g_pbuf, (g_wnd_size_w - g_pbuf_w)/2, GAP_UPLINE_IMG, g_pbuf_w, g_pbuf_h, NFALIGN_CENTER, 0);
		}
        nfui_nfobject_gc_unref(gc);
	}
	else if(evt->type == GDK_DELETE)
    {
        nfui_nfobject_get_size(g_pbuf, &g_pbuf_w, &g_pbuf_h);
        nfui_unref_popup_pixbuf(g_pbuf, g_pbuf_w, g_pbuf_h);
    }
	
	return FALSE;
}

static gboolean post_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	int mx, my, mw, mh;

	if(evt->type == GDK_DELETE)
	{
	    nfui_page_close(PGID_START_SEQURINET_POPUP, g_curwnd);
		g_curwnd = 0;
	}
	else if(evt->type == NFOUTEVT_MOTION_NOTIFY) 
	{
		GdkEventMotion *mevt;
		gint x, y;
		gint px, py;
		gint pedge;

		mevt = (GdkEventMotion*)evt;


		LiveStart_Popup_Pos(&mx, &my);
		LiveStart_Popup_Size(&mw, &mh);

		x = (gint)mevt->x_root - mx;
		y = (gint)mevt->y_root - my;

		px = (gint)mevt->x_root - g_prt_item->x;
		py = (gint)mevt->y_root - g_prt_item->y;
	
		if (px < obj->x) { 	// show on right side	
			if((x < g_prt_item->x || x > (mw + obj->width))) {
				if(nfui_nfobject_is_shown(obj)) {
					nfui_nfobject_hide(obj);
					nfui_page_close(PGID_START_SEQURINET_POPUP, g_curwnd);

					return TRUE;
				}
			}

			if((y < g_prt_item->y || y > (g_prt_item->y + g_prt_item->height))) {
				if(nfui_nfobject_is_shown(obj)) {
					nfui_nfobject_hide(obj);
					nfui_page_close(PGID_START_SEQURINET_POPUP, g_curwnd);

					return TRUE;
				}
			}
		}
		else {
			pedge = obj->x + obj->width + MENU_GAP + mw;

			if((gint)mevt->x_root < obj->x || (gint)mevt->x_root > pedge) {
				if(nfui_nfobject_is_shown(obj)) {
					nfui_nfobject_hide(obj);
					nfui_page_close(PGID_START_SEQURINET_POPUP, g_curwnd);

					return TRUE;
				}
			}

			if((y < g_prt_item->y || y > (g_prt_item->y + g_prt_item->height))) {
				if(nfui_nfobject_is_shown(obj)) {
					nfui_nfobject_hide(obj);
					nfui_page_close(PGID_START_SEQURINET_POPUP, g_curwnd);

					return TRUE;
				}
			}
		}

		return TRUE;
    }
    else if(evt->type == NFOUTEVT_BUTTON_PRESS)
    {
        nfui_nfobject_hide(obj);
        nfui_page_close(PGID_START_SEQURINET_POPUP, g_curwnd);
    }
    else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_EXIT)
		{
			nfui_nfobject_hide(obj);
			nfui_page_close(PGID_START_SEQURINET_POPUP, g_curwnd);
		}
	}

	return FALSE;
}

static gboolean post_detail_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
        NF_NOTIFY_INFO pInfo;
        gchar qr_url[256] = {0,};
		gchar strbuf[128] = {0,};
        gint url_len = 256;
        
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
		{
			return FALSE;
	    }
		nfui_nfobject_hide(g_curwnd);
		nfui_page_close(PGID_START_SEQURINET_POPUP, g_curwnd);
		LiveStart_Popup_Hide();
		
		scm_get_wan_status_event_data(&pInfo);
        if(!pInfo.d.params[0])
        {
			#if defined(_SEQURINET_STRING_FIX)
                strcpy(strbuf, "SEQURINET");
            #else
                strcpy(strbuf, "P2P");
            #endif

            var_get_qr_url(qr_url, url_len);
    	    VW_QR_code_Open(g_curwnd, strbuf, qr_url, 200, 200);
	    }
	    else
	    {
            nftool_mbox(g_curwnd, "NOTICE", "Please check your network connection.", NFTOOL_MB_OK);
        }		
	}

	return FALSE;
}

gboolean VW_Image_Popup_Open(NFWINDOW *parent, NFOBJECT *parent_item, GdkPixbuf *image, guint img_w, guint img_h)
{
	NFOBJECT *main_wnd, *main_fixed;
	NFOBJECT *url_label;
	NFOBJECT *detail_btn;
	gchar qr_url[256] = "http://www.sequrinet.com";
	gint url_len = 256;
	gint wnd_pos_x, wnd_pos_y;

    _get_wnd_position(&wnd_pos_x, &wnd_pos_y);

    _init_global_value(parent_item, image, wnd_pos_x, wnd_pos_y, img_w, img_h);

	main_wnd = (NFOBJECT*)nfui_nfwindow_new(parent, wnd_pos_x, wnd_pos_y, g_wnd_size_w, g_wnd_size_h);
    nfui_nfwindow_use_outside_evt((NFWINDOW*)main_wnd, TRUE);
    nfui_nfwindow_set_mask((NFWINDOW*)main_wnd, GDK_MOTION_NOTIFY, TRUE);
    nfui_nfwindow_set_mask((NFWINDOW*)main_wnd, GDK_BUTTON_PRESS, TRUE);
    gtk_widget_add_events(((NFWINDOW*)main_wnd)->main_widget, GDK_POINTER_MOTION_HINT_MASK);
	nfui_regi_post_event_callback(main_wnd, post_win_event_handler);
	g_curwnd = main_wnd;

	main_fixed = nfui_nffixed_new();
	nfui_nfobject_set_size((NFOBJECT*)main_fixed, g_wnd_size_w, g_wnd_size_h);
	nfui_nfwindow_add((NFWINDOW*)main_wnd, main_fixed);
	nfui_regi_post_event_callback(main_fixed, post_main_bg_event_handler);

    //var_get_qr_url(qr_url, url_len);
	url_label = nfui_nflabel_new_with_pango_font(qr_url, nffont_get_pango_font(NFFONT_MINI_NORMAL_5), COLOR_IDX(340));
	nfui_nfobject_set_size(url_label, g_url_size_w, g_url_size_h);
	nfui_nflabel_set_align(url_label, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(url_label, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(url_label, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_support_multi_lang(url_label, FALSE);
	nfui_nffixed_put((NFFIXED*)main_fixed, url_label, 10, g_url_pos_y);
	//nfui_nfobject_show(url_label);

	if (ivsc.vendor_code != 43)
	{
		detail_btn = nftool_normal_button_create_type1("DETAIL", 120);
		nfui_nfbutton_set_font_alignment((NFBUTTON*)detail_btn, NFALIGN_CENTER, 0);
		nfui_regi_post_event_callback(detail_btn, post_detail_button_event_handler);
		nfui_nffixed_put((NFFIXED*)main_fixed, detail_btn, (g_wnd_size_w - 120)/2, g_btn_pos_y);
		nfui_nfobject_show(detail_btn);
	}

    nfui_nfobject_show(main_fixed);
    nfui_run_main_event_handler(g_curwnd);
    nfui_nfobject_show(main_wnd);
    
    /* set for key navi */
	nfui_make_key_hierarchy((NFWINDOW*)main_wnd);
    nfui_page_open(PGID_START_SEQURINET_POPUP, g_curwnd, nfui_get_last_user());

    nfui_nfobject_hide(g_curwnd);
    nfui_page_close(PGID_START_SEQURINET_POPUP, g_curwnd);
    
	return ret;
}

gint VW_destroy_Image_Popup()
{
    if(!g_curwnd) return -1;

    nfui_nfobject_destroy((NFOBJECT*)g_curwnd);
    return 0;
}

gint VW_Hide_Image_Popup()
{
    if(!g_curwnd) 
    {
        g_message("%s[%d] : WINDOW IS NULL!!!", __FUNCTION__, __LINE__);
        return -1;
    }
    nfui_nfobject_hide((NFOBJECT*)g_curwnd);
    nfui_page_close(PGID_START_SEQURINET_POPUP, g_curwnd);
    
    return 0;
}

gint VW_Show_Image_Popup(GdkPixbuf *pbuf, guint size_w, guint size_h)
{
    if (pbuf)
    {
        g_pbuf = pbuf;
        g_pbuf_w = size_w;
        g_pbuf_h = size_h;
    }
    
    if(!g_curwnd) 
    {
        g_message("%s[%d] : WINDOW IS NULL!!!", __FUNCTION__, __LINE__);
        return -1;
    }
	else if(!nfui_nfobject_is_shown((NFOBJECT*)g_curwnd)) 
	{
    	nfui_nfobject_show((NFOBJECT*)g_curwnd);
    	nfui_page_open(PGID_START_SEQURINET_POPUP, g_curwnd, nfui_get_last_user());
    }
    return 0;
}

