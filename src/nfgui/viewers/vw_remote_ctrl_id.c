

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



#define	RMCID_WIDTH		(DISPLAY_IS_D1 ? 394:394)
#define	RMCID_HEIGHT	(DISPLAY_IS_D1 ? 127:127)

#define RMCID_SHOW_INTERVAL			(5000)



static NFOBJECT *g_rmcWin;



static gboolean close_rmc(gpointer data)
{
	nfui_nfobject_destroy(g_rmcWin);

	return FALSE;
}


static gboolean pre_main_bg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
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
		
			g_rmcWin = NULL;
			gtk_main_quit();
		}
		break;

		default:
			break;
	}

	return FALSE;
}

void RmCtrlID_Open(NFWINDOW *parent)
{
	NFOBJECT *nffixed, *lbRmcID, *lbTemp;
	guint remocon_id;
	gchar strBuf[8];
	gint x, y;

	if(g_rmcWin)
		return;

	remocon_id = nfcd_get_remocon_id();

	x = ((DISPLAY_ACTIVE_WIDTH - RMCID_WIDTH)/4)*2;
	y = (DISPLAY_ACTIVE_HEIGHT - RMCID_HEIGHT)/2;

	g_rmcWin = (NFOBJECT*)nfui_nfwindow_new(parent, x, y, RMCID_WIDTH, RMCID_HEIGHT);
	if(g_rmcWin == NULL)	return;
	
	nffixed =(NFOBJECT*) nfui_nffixed_new();

	nfui_nfobject_show(nffixed);

	nfui_nfwindow_add((NFWINDOW*)g_rmcWin, nffixed);

	/* title */
	lbTemp = (NFOBJECT*)nfui_nflabel_new_with_pango_font("REMOCON ID", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(lbTemp, 386, 36);
	nfui_nflabel_set_align((NFLABEL*)lbTemp, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(lbTemp, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(lbTemp);
	nfui_nffixed_put((NFFIXED*)nffixed, lbTemp, 4, 4);


	lbTemp = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ID", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(lbTemp, 30, 40);
	nfui_nfobject_use_focus(lbTemp, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(lbTemp);

	nfui_nffixed_put((NFFIXED*)nffixed, lbTemp, 13, 60);


	memset(strBuf, 0x00, sizeof(strBuf));
	if(remocon_id < 10) g_sprintf(strBuf, "0%d", remocon_id);
	else				g_sprintf(strBuf, "%d", remocon_id);

	lbRmcID = (NFOBJECT*)nfui_nflabel_new_text_box(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)lbRmcID, NFTEXTBOX_TYPE_POPUP_OUTPUT);
	nfui_nfobject_set_size(lbRmcID, 310, 40);
	nfui_nfobject_use_focus(lbRmcID, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(lbRmcID);


	nfui_nffixed_put((NFFIXED*)nffixed, lbRmcID, 60, 60);

	nfui_regi_pre_event_callback(nffixed, pre_main_bg_event_handler);

	nfui_run_main_event_handler(g_rmcWin);
	nfui_nfobject_show(g_rmcWin);
	nfui_make_key_hierarchy((NFWINDOW*)g_rmcWin);

	nfui_page_open(PGID_RMCID_CONF, g_rmcWin, nfui_get_last_user());

	g_timeout_add(RMCID_SHOW_INTERVAL, close_rmc, NULL);

	gtk_main();

	nfui_page_close(PGID_RMCID_CONF, g_rmcWin);
}
