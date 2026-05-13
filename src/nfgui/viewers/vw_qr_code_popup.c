#include <string.h>
#include "nf_afx.h"

#include "nf_network.h"
#include "nf_util_netif.h"
#include "nf_api_ipcam.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "tools/nf_ui_tool.h"

#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"
#include "objects/nfimage.h"
#include "objects/nftab.h"
#include "objects/nfcheckbutton.h"

#include "ix_mem.h"
#include "scm.h"

#include "vw_qr_code_popup.h"
#include "vw_sys_net_ddns.h"
#include "vw_wizard_init.h"


#define PI_WND_SIZE_WID			(guint)(1040)//(guint)(480 + g_qrimg_w)
#define PI_WND_SIZE_HEI			(guint)(QR_IMAGE_POS_Y + QR_FIXED_SIZE_H + EXP_FIXED_SIZE_H + CHECK_BTN_SIZE_H + CHECK_OK_GAP + OK_BTN_HEIGHT + OK_BOTTOM_GAP + 20)
#define PI_WND_POS_X			(guint)((DISPLAY_ACTIVE_WIDTH - PI_WND_SIZE_WID)/2)
#define PI_WND_POS_Y			(guint)((DISPLAY_ACTIVE_HEIGHT - PI_WND_SIZE_HEI)/2)

#define TAB_POS_Y               (50)
#define TAB_SIZE_H              (43)
#define TAB_SIZE_W              (208)
#define TAB_QR_GAP              (30)

#define TABPAGE_SIZE_W          (PI_WND_SIZE_WID - 20)
#define TABPAGE_SIZE_H          (TAB_QR_GAP + QR_FIXED_SIZE_H + EXP_FIXED_SIZE_H)
#define TABPAGE_POS_Y           (TAB_POS_Y + TAB_SIZE_H)

#define QR_IMAGE_POS_Y          (TAB_POS_Y + TAB_SIZE_H + TAB_QR_GAP)
#define QR_FIXED_POS_Y          (TAB_QR_GAP)
#define QR_FIXED_SIZE_H         (g_qrimg_h + URL_LABEL_SIZE_H + QR_URL_GAP)
#define QR_URL_GAP              (7)
#define URL_LABEL_POS_Y         (g_qrimg_h + QR_URL_GAP)
#define URL_LABEL_SIZE_H        (35)

#define EXP_FIXED_POS_Y         (QR_FIXED_POS_Y + QR_FIXED_SIZE_H + 20)
#define EXP_LINE_COUNT          (4)
#define EXP_TITLE_LABEL_GAP     (20)
#define EXP_LABEL_SIZE_H        (40)
#define EXP_FIXED_SIZE_H        (TAB_SIZE_H + EXP_TITLE_LABEL_GAP + (EXP_LABEL_SIZE_H * EXP_LINE_COUNT))

#define CHECK_BTN_SIZE_H        (EXP_LABEL_SIZE_H)
#define CHECK_BTN_POS_Y         (OK_BTN_POS_Y - CHECK_BTN_SIZE_H - CHECK_OK_GAP)
#define CHECK_OK_GAP            (20)

#define OK_BTN_WIDTH			(162)
#define OK_BTN_HEIGHT			(44)
#define OK_BOTTOM_GAP           (20)
#define OK_BTN_POS_X	        ((PI_WND_SIZE_WID - OK_BTN_WIDTH)/2)
#define OK_BTN_POS_Y		    (PI_WND_SIZE_HEI - OK_BOTTOM_GAP - OK_BTN_HEIGHT)

enum{
    TAB_MOBILE = 0,
    TAB_WEB,
    TAB_NUM
};

static WIZARD_USERDATA_T *g_wizard_data;

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_res;
static NFOBJECT *g_okbtn;
static NFTAB *g_subtab;
static GdkPixbuf *g_qrimg = NULL;
static gchar *g_qr_url;
static gint g_qrimg_w = 0;
static gint g_qrimg_h = 0;


static void _make_qrcode_image(gchar *qr_url, guint qr_img_w, guint qr_img_h)
{
    GdkPixbuf *tpix;
    gint size_w, size_h;

    size_w = qr_img_w;
    size_h = qr_img_h;
    
    scm_set_qrcode_use_URL("/tmp/itx", qr_url);
    
    tpix = gdk_pixbuf_new_from_file("/tmp/itx.png", NULL);
    if(qr_img_w == -1 || qr_img_h == -1){
        nfui_get_pixbuf_size(tpix, &size_w, &size_h);
    }
    g_qrimg = gdk_pixbuf_scale_simple(tpix, size_w, size_h, GDK_INTERP_HYPER);
}

static void _make_dvr_id(gchar *buf)
{
    gchar mac[32];
    gint mac_len = 32;

    scm_get_mac_addr_str(mac, mac_len);
    strcat(buf, mac);

}

static gboolean post_main_bg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkGC *gc;	
	GdkDrawable *drawable;

	if(evt->type == GDK_EXPOSE) 
	{
        drawable = nfui_nfobject_get_window(obj);
        gc = nfui_nfobject_get_gc(obj);
		gdk_draw_pixbuf(drawable, gc, g_qrimg, 0, 0, (PI_WND_SIZE_WID-g_qrimg_w)/2, QR_IMAGE_POS_Y, g_qrimg_w, g_qrimg_h, GDK_RGB_DITHER_NONE, 0, 0);				
        nfui_nfobject_gc_unref(gc);
	}
	return FALSE;
}

static gboolean post_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
		if(evt->type == GDK_DELETE)
		{
			g_curwnd = 0;
			g_object_unref(g_qrimg);
			gtk_main_quit();
		}
	
		return FALSE;

}

static gboolean post_checkbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        gboolean active;
        
        active = nfui_check_button_get_active((NFCHECKBUTTON*)obj);
        if(active)
        {
            nfui_nfobject_enable(g_okbtn);
            nfui_signal_emit(g_okbtn, GDK_EXPOSE, TRUE);
        }
        else
        {
            nfui_nfobject_disable(g_okbtn);
            nfui_signal_emit(g_okbtn, GDK_EXPOSE, TRUE);
        }
    }
    
    return FALSE;
}

static gboolean post_okbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
		{
			return FALSE;
	    }
		nfui_nfobject_destroy(g_curwnd);			
	}

	return FALSE;
}

static gboolean pre_page_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_EXPOSE) {
		GdkDrawable *drawable;
		GdkColor line_color = UX_COLOR(392);
		GdkGC *line_gc;

		drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
		line_gc = nfui_nfobject_get_gc((NFOBJECT*)obj);
		gdk_gc_set_rgb_fg_color(line_gc, &line_color);

		gdk_gc_set_line_attributes(line_gc,
				1,
				GDK_LINE_SOLID,
				GDK_CAP_NOT_LAST,
				GDK_JOIN_BEVEL);

		gdk_draw_rectangle(drawable,
				line_gc,
				FALSE,
				obj->x, obj->y,
				obj->width, obj->height);

		nfui_nfobject_gc_unref(line_gc);
	}

	return FALSE;
}

static gboolean pre_subtab_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint cur_page;
	gint old_page;
    gint active;
    
	if(evt->type == NFEVENT_TAB_BEFORE_CHANGE)
	{
		cur_page = nfui_nftab_get_cur_page((NFTAB*)obj);
		old_page = nfui_nftab_get_old_page((NFTAB*)obj);

    	if(cur_page == old_page) return FALSE;
		switch(cur_page) {
			case TAB_MOBILE:	// 
				break;

			case TAB_WEB:	// 
				break;

			default:
				break;
		}

	}
	return FALSE;
}

static gint init_tab_mobile(NFOBJECT *parent)
{
	NFOBJECT *qr_fixed, *exp_fixed;
	NFOBJECT *url_label, *exp_label;
	NFOBJECT *obj;
	gint pos_y;
	gint i;
#if defined(_SEQURINET_STRING_FIX)
	gchar *explanation[] = {
          "1. Please scan the QR code above with a QR code scanner or directly install the mobile app.", 
          "2. After installing the mobile app, please login to Sequrinet. If you are not already registered,", 
          "please sign up for a free account.", 
          "3. Please register a recording device. You may easily register devices with the QR code above."
    };
#else
	gchar *explanation[] = {
          "1. Please scan the QR code above with a QR code scanner or directly install the mobile app.", 
          "2. After installing the mobile app, please login to P2P. If you are not already registered,", 
          "please sign up for a free account.", 
          "3. Please register a recording device. You may easily register devices with the QR code above."
    };
#endif
    gchar *url = "http://www.sequrinet.com";

	qr_fixed = nfui_nffixed_new();
	nfui_nfobject_set_size(qr_fixed, TABPAGE_SIZE_W, QR_FIXED_SIZE_H);
	nfui_nffixed_put((NFFIXED*)parent, qr_fixed, 0, QR_FIXED_POS_Y);
	
	exp_fixed = nfui_nffixed_new();
	nfui_nfobject_set_size(exp_fixed, TABPAGE_SIZE_W, EXP_FIXED_SIZE_H);
	nfui_nffixed_put((NFFIXED*)parent, exp_fixed, 0, EXP_FIXED_POS_Y);

    /* Draw image */
	nfui_regi_post_event_callback(parent, post_main_bg_event_handler);

    /* labels */
	url_label = nfui_nflabel_new_with_pango_font(url, nffont_get_pango_font(NFFONT_MINI_NORMAL_5), COLOR_IDX(340));
	nfui_nfobject_set_size(url_label, TABPAGE_SIZE_W- 20, URL_LABEL_SIZE_H);
	nfui_nflabel_set_align(url_label, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(url_label, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
	nfui_nfobject_use_focus(url_label, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_support_multi_lang(url_label, FALSE);
	nfui_nfobject_set_tooltip(url_label, url);
	nfui_nffixed_put((NFFIXED*)qr_fixed, url_label, 10, URL_LABEL_POS_Y);
	//nfui_nfobject_show(url_label);

    for(i = 0; i < EXP_LINE_COUNT; i++)
    {
    	exp_label = nfui_nflabel_new_with_pango_font(explanation[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(340));
    	nfui_nfobject_set_size(exp_label, TABPAGE_SIZE_W - 20, EXP_LABEL_SIZE_H);
    	if(i == 2){
    	    nfui_nflabel_set_align(exp_label, NFALIGN_LEFT, 52);
	    }
    	else{
        	nfui_nflabel_set_align(exp_label, NFALIGN_LEFT, 30);
    	}
    	nfui_nfobject_modify_bg(exp_label, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
    	nfui_nfobject_use_focus(exp_label, NFOBJECT_FOCUS_OFF);
    	nfui_nffixed_put((NFFIXED*)exp_fixed, exp_label, 10, (i * EXP_LABEL_SIZE_H) /*+ EXP_TITLE_LABEL_GAP + TAB_SIZE_H*/);
    	nfui_nfobject_show(exp_label);
	}


	nfui_nfobject_show(qr_fixed);
	nfui_nfobject_show(exp_fixed);

    return 0;
}

static gint init_tab_web(NFOBJECT *parent)
{   
	NFOBJECT *qr_fixed, *exp_fixed;
	NFOBJECT *url_label, *id_label, *exp_label;
	NFOBJECT *obj;
	gint pos_y, pos_x;
	gint i;
#if defined(_SEQURINET_STRING_FIX)
	gchar *explanation[] = {
          "1. Connect to Sequrinet through the above URL.", 
          "2. Sign in to Sequrinet (if you do not have an account please sign up for a free account).",
          "3. After logging in, you may easily connect to recording devices through Sequrinet", 
          "by entering the DVR/NVR ID.", 
          "4. If you have a Sequrinet ID you can also connect to your devices through the mobile app as well."
    };
#else
	gchar *explanation[] = {
          "1. Connect to P2P through the above URL.", 
          "2. Sign in to P2P (if you do not have an account please sign up for a free account).",
          "3. After logging in, you may easily connect to recording devices through P2P", 
          "by entering the DVR/NVR ID.", 
          "4. If you have a P2P ID you can also connect to your devices through the mobile app as well."
    };
#endif
    gchar *url = "http://www.sequrinet.com";
    gchar dvrid[64] = "DVR/NVR ID : ";

	qr_fixed = nfui_nffixed_new();
	nfui_nfobject_set_size(qr_fixed, TABPAGE_SIZE_W, QR_FIXED_SIZE_H);
	nfui_nffixed_put((NFFIXED*)parent, qr_fixed, 0, QR_FIXED_POS_Y);
	
	exp_fixed = nfui_nffixed_new();
	nfui_nfobject_set_size(exp_fixed, TABPAGE_SIZE_W, EXP_FIXED_SIZE_H);
	nfui_nffixed_put((NFFIXED*)parent, exp_fixed, 0, EXP_FIXED_POS_Y - 20);

    pos_y = 50;
    pos_x = 10;
    
    /* labels */
	url_label = nfui_nflabel_new_with_pango_font(url, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(340));
	nfui_nfobject_set_size(url_label, TABPAGE_SIZE_W- 20, URL_LABEL_SIZE_H);
	nfui_nflabel_set_align(url_label, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(url_label, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
	nfui_nfobject_use_focus(url_label, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_support_multi_lang(url_label, FALSE);
	nfui_nfobject_set_tooltip(url_label, url);
	nfui_nffixed_put((NFFIXED*)qr_fixed, url_label, pos_x, pos_y);
	nfui_nfobject_show(url_label);

	pos_y += 60;
	_make_dvr_id(dvrid);

	id_label = nfui_nflabel_new_with_pango_font(dvrid, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(340));
	nfui_nfobject_set_size(id_label, TABPAGE_SIZE_W- 20, URL_LABEL_SIZE_H);
	nfui_nflabel_set_align(id_label, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(id_label, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
	nfui_nfobject_use_focus(id_label, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_support_multi_lang(id_label, FALSE);
	nfui_nfobject_set_tooltip(id_label, dvrid);
	nfui_nffixed_put((NFFIXED*)qr_fixed, id_label, pos_x, pos_y);
	nfui_nfobject_show(id_label);

    for(i = 0; i < 5; i++)
    {
    	exp_label = nfui_nflabel_new_with_pango_font(explanation[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(340));
    	nfui_nfobject_set_size(exp_label, TABPAGE_SIZE_W - 20, EXP_LABEL_SIZE_H);
    	if(i == 3){
    	    nfui_nflabel_set_align(exp_label, NFALIGN_LEFT, 52);
	    }
    	else{
        	nfui_nflabel_set_align(exp_label, NFALIGN_LEFT, 30);
    	}
    	nfui_nfobject_modify_bg(exp_label, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
    	nfui_nfobject_use_focus(exp_label, NFOBJECT_FOCUS_OFF);
    	nfui_nffixed_put((NFFIXED*)exp_fixed, exp_label, 10, (i * EXP_LABEL_SIZE_H) /*+ EXP_TITLE_LABEL_GAP + TAB_SIZE_H*/);
    	nfui_nfobject_show(exp_label);
	}


	nfui_nfobject_show(qr_fixed);
	nfui_nfobject_show(exp_fixed);

    return 0;
}


gboolean VW_QR_code_Open(NFWINDOW *parent, gchar *title, gchar *qr_url, guint qr_img_w, guint qr_img_h)
{
	NFOBJECT *main_wnd;
	NFOBJECT *main_fixed;
	NFOBJECT *tab;
	NFOBJECT *tabPage[2];
	NFOBJECT *obj;
	NFOBJECT *btn;
	gint i;
    const gchar *strImage_h[2] =  {
				(MKB_IMG_TAB_POP_DIR_H_N_208), 
				(MKB_IMG_TAB_POP_DIR_H_S_208)
	};
    gchar *strCheck = "I have confirmed all of the above.";
	const gchar *strTitle[] = {
				"MOBILE USE", 
				"WEB USE"
	};
	const guint colidx[3] = {COLOR_IDX(287), COLOR_IDX(289), COLOR_IDX(288)};


    /* make_image */
    _make_qrcode_image(qr_url, qr_img_w, qr_img_h);
	nfui_get_pixbuf_size(g_qrimg, &g_qrimg_w, &g_qrimg_h);

    /* window */
	main_wnd = nftool_create_popup_window(parent, PI_WND_POS_X, PI_WND_POS_Y, PI_WND_SIZE_WID, PI_WND_SIZE_HEI, title, FALSE);
	nfui_nfwindow_set_title(main_wnd, "SEQURINET QR CODE");
	nfui_regi_post_event_callback(main_wnd, post_win_event_handler);
	g_curwnd = main_wnd;

    /* fixed */
	main_fixed = ((NFWINDOW*)main_wnd)->child;

	/* tab */
	tab = (NFOBJECT*)nfui_nftab_new(1, strImage_h, TAB_SIZE_W, TAB_SIZE_H, NFTAB_DIR_H, strTitle, colidx);
	nfui_nftab_set_pango_font((NFTAB*)tab, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nftab_set_margin((NFTAB*)tab, 10);
	nfui_regi_pre_event_callback(tab, pre_subtab_event_handler);
	nfui_nfobject_show(tab);
	nfui_nffixed_put((NFFIXED*)main_fixed, tab, 10, TAB_POS_Y);
	g_subtab = (NFTAB*)tab;

	for(i=0; i<1; i++) {
		tabPage[i] = (NFOBJECT*)nfui_nffixed_new();
		nfui_nfobject_set_size(tabPage[i], TABPAGE_SIZE_W, TABPAGE_SIZE_H);
		nfui_nfobject_modify_bg(tabPage[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
		nfui_nftab_regi_page((NFTAB*)tab, tabPage[i], i);
		nfui_nffixed_put((NFFIXED*)main_fixed, tabPage[i], 10, TABPAGE_POS_Y);

		nfui_regi_pre_event_callback(tabPage[i], pre_page_event_cb);
	}
    init_tab_mobile(tabPage[0]);
    //init_tab_web(tabPage[1]);
    nfui_nfobject_show(tabPage[0]);

	/* check button */
	obj = nfui_checkbutton_new(FALSE);
	nfui_check_button_set_skin_type((NFCHECKBUTTON*)obj, NFCHECK_TYPE_POPUP_NORMAL);
	nfui_nfobject_set_size(obj, 30, CHECK_BTN_SIZE_H);
	nfui_regi_post_event_callback(obj, post_checkbutton_event_handler);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 40 ,CHECK_BTN_POS_Y);
	nfui_nfobject_show(obj);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strCheck, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(340));
	nfui_nfobject_set_size(obj, TABPAGE_SIZE_W - 100, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 50+30, CHECK_BTN_POS_Y - 5);
	nfui_nfobject_show(obj);


	/* button */
	btn = nftool_normal_button_create_type1("OK", OK_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(btn), NFALIGN_CENTER, 0);	
	nfui_nffixed_put((NFFIXED*)main_fixed, btn, OK_BTN_POS_X, OK_BTN_POS_Y);
	nfui_regi_post_event_callback(btn, post_okbutton_event_handler);
	nfui_nfobject_disable(btn);
	nfui_nfobject_show(btn);
	g_okbtn = btn;

	nfui_nfobject_show(main_wnd);

	/* set for key navi */
	nfui_make_key_hierarchy(main_wnd);

	gtk_main();

	return FALSE;
}

gint VW_QR_code_Close()
{
	if (!g_curwnd) return -1;

    evt_send_to_window("SEQURINET QR CODE", WND_CLOSE, 0, 0, 0);

    return 0;
}
