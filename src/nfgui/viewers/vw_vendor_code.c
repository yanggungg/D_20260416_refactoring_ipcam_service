

#include "nf_afx.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nflabel.h"

#include "support/event_loop.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_common_data.h"
#include "support/nf_ui_page_manager.h"

#include "vw_remote_ctrl_id.h"



#define	VENDOR_WIDTH		(DISPLAY_IS_D1 ? 394:394)
#define	VENDOR_HEIGHT	(DISPLAY_IS_D1 ? 127:127)

#define VENDOR_SHOW_INTERVAL			(2000)



static NFOBJECT *lbVendor;
static NFOBJECT *g_curwnd;

static int vendor = -1;
static char vcodebuf[4] = { 0, };
static int len = 0;
static int vc = 0;


static gboolean close_rmc(gpointer data)
{
	nfui_nfobject_destroy(g_curwnd);

	return FALSE;
}

static int keymap[10][2] = {
	{ KEYPAD_CH01, 1},
	{ KEYPAD_CH02, 2},
	{ KEYPAD_CH03, 3},
	{ KEYPAD_CH04, 4},
	{ KEYPAD_CH05, 5},
	{ KEYPAD_CH06, 6},
	{ KEYPAD_CH07, 7},
	{ KEYPAD_CH08, 8},
	{ KEYPAD_CH09, 9},
	{ RMC_NUM_0,   0},
};


static int _get_numeric_key_value(KEYPAD_KID kid)
{
	int i;
	for (i = 0; i < 10; ++i) 
		if (keymap[i][0] == kid) return keymap[i][1];
	return -1;
}

static int _convert_to_int(char *vcode, int len)
{
	int value = 0;
	int unit = 1;
	int kval;
	int i;

	for (i = 0; i < len; ++i) {
		
		// keypad dependent
		kval = _get_numeric_key_value(vcode[len - i - 1]);
		if (kval == -1) continue;

		value = value + (kval * unit);
		unit *= 10;
	}

	return value;
}

static gboolean _proc_exit(void *data)
{
	nfui_nfobject_destroy(g_curwnd);
	return FALSE;
}

static gboolean post_mainWin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid;
	char vcstr[8];

	memset(vcstr, 0x00, sizeof(vcstr));

	switch(evt->type) {
	case NFEVENT_KEYPAD_RELEASE:
	case NFEVENT_REMOCON_RELEASE:
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if (kpid == KEYPAD_ENTER) { 
			vendor = vc;
			g_timeout_add(100, _proc_exit, 0);
			return FALSE;
		}
		break;
	}


}

static gboolean pre_mainWin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid;
	char vcstr[8];

	memset(vcstr, 0x00, sizeof(vcstr));

	switch(evt->type) {
	case NFEVENT_KEYPAD_PRESS:
	case NFEVENT_REMOCON_PRESS:
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

/*		if (kpid == KEYPAD_ENTER) { 
			vendor = vc;
			nfui_nfobject_destroy(g_curwnd);
			return FALSE;
		}
*/		
		if (len == 4) {
			len = 0;
			vc = 0;
		}

		vcodebuf[len++] = kpid;
		vc = _convert_to_int(vcodebuf, len);
		sprintf(vcstr, "%d", vc);

		nfui_nflabel_set_text((NFLABEL*)lbVendor, vcstr);
		nfui_signal_emit(lbVendor, GDK_EXPOSE, TRUE);

		break;

	case GDK_DELETE:
		g_curwnd = NULL;
		gtk_main_quit();
		break;

	}

	return FALSE;
}


static gboolean pre_main_bg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	switch(evt->type)
	{
		case GDK_EXPOSE:
		{
    		drawable = nfui_nfobject_get_window(obj);
    		gc = nfui_nfobject_get_gc(obj);

            nfui_nfobject_get_size(obj, &size_w, &size_h);
            pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
            nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
        
    		nfui_nfobject_gc_unref(gc);
    	}
		break;

        case GDK_DELETE:
        {
            nfui_nfobject_get_size(obj, &size_w, &size_h);
            nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
        }
        break;

		default:
			break;
	}

	return FALSE;
}

int VW_VendorCode_Open(NFWINDOW *parent)
{
	NFOBJECT *nffixed, *lbTemp;
	gchar strBuf[8];
	gint x, y;

	vendor = -1;
	len = 0;
	vc = 0;

	x = ((DISPLAY_ACTIVE_WIDTH - VENDOR_WIDTH)/4)*2;
	y = (DISPLAY_ACTIVE_HEIGHT - VENDOR_HEIGHT)/2;

	g_curwnd = (NFOBJECT*)nfui_nfwindow_new(parent, x, y, VENDOR_WIDTH, VENDOR_HEIGHT);
	if(g_curwnd == NULL)	return -1;
	nfui_regi_pre_event_callback(g_curwnd, pre_mainWin_event_handler);
	nfui_regi_post_event_callback(g_curwnd, post_mainWin_event_handler);
	
	nffixed =(NFOBJECT*) nfui_nffixed_new();

	nfui_nfobject_show(nffixed);

	nfui_nfwindow_add((NFWINDOW*)g_curwnd, nffixed);

	/* title */
	lbTemp = (NFOBJECT*)nfui_nflabel_new_with_pango_font("VENDOR CODE", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(lbTemp, 386, 36);
	nfui_nflabel_set_align((NFLABEL*)lbTemp, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(lbTemp, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(lbTemp);
	nfui_nffixed_put((NFFIXED*)nffixed, lbTemp, 4, 4);


	lbTemp = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CODE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(lbTemp, 30, 40);
	nfui_nfobject_use_focus(lbTemp, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(lbTemp);

	nfui_nffixed_put((NFFIXED*)nffixed, lbTemp, 23, 60);


	memset(strBuf, 0x00, sizeof(strBuf));

	lbVendor = (NFOBJECT*)nfui_nflabel_new_text_box(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)lbVendor, NFTEXTBOX_TYPE_POPUP_OUTPUT);
	nfui_nfobject_set_size(lbVendor, 280, 40);
//	nfui_nfobject_use_focus(lbVendor, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(lbVendor);
	nfui_nffixed_put((NFFIXED*)nffixed, lbVendor, 80, 60);

	nfui_regi_pre_event_callback(nffixed, pre_main_bg_event_handler);
	nfui_run_main_event_handler(g_curwnd);
	nfui_nfobject_show(g_curwnd);
	nfui_make_key_hierarchy((NFWINDOW*)g_curwnd);

	nfui_set_key_focus(lbVendor, TRUE);

	nfui_page_open(PGID_VENDOR_CODE, g_curwnd, nfui_get_last_user());

//	g_timeout_add(VENDOR_SHOW_INTERVAL, close_rmc, NULL);

	gtk_main();

	nfui_page_close(PGID_VENDOR_CODE, g_curwnd);

	return vendor;
}
