

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

#include "vw_live_ptz_rmc_prst.h"


#define	PRP_WIDTH		                (280)
#define	PRP_HEIGHT                      (127)

#define	PRP_POS_X		                ((DISPLAY_ACTIVE_WIDTH-PRP_WIDTH)/2)
#define	PRP_POS_Y                       ((DISPLAY_ACTIVE_HEIGHT-PRP_HEIGHT)/2)

#define GET_NUM_KEY_SCOPE(kpid)         (kpid >= KEYPAD_CH01 && kpid <= KEYPAD_CH09)


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_prst_label;

static gint prst_num = 0;


static gint input_label(gint num)
{
    gchar strBuf[8];
    gint tmp_prst_num;

    if (prst_num)
        tmp_prst_num = prst_num*10 + num;        
    else
        tmp_prst_num = num;

    if (tmp_prst_num > 255) return -1;

    g_message("%s, %d, num:%d", __FUNCTION__, __LINE__, tmp_prst_num);

    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, "%d", tmp_prst_num);
    nfui_nflabel_set_text(g_prst_label, strBuf);
    nfui_signal_emit(g_prst_label, GDK_EXPOSE, TRUE);

    prst_num = tmp_prst_num;

    return 0;
}

static gboolean post_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint input_num;

	switch(evt->type)
	{
		case GDK_EXPOSE:
		break;

		case NFEVENT_KEYPAD_RELEASE:
		case NFEVENT_REMOCON_RELEASE:
		{
        	GdkEventKey *kevt;
        	KEYPAD_KID kpid;
    		
        	kevt = (GdkEventKey*)evt;
        	kpid = (KEYPAD_KID)kevt->keyval;

			if (GET_NUM_KEY_SCOPE(kpid))
			{
				input_num = kpid - KEYPAD_CH01 + 1;
                input_label(input_num);
			}
            else if (kpid == KEYPAD_CH00)
            {
                input_label(0);
            }			
            else
            {
        		switch(kpid)
        		{
        			case KEYPAD_ENTER:
    				{
                        nfui_nfobject_destroy(obj);
    				}
    				break;			
        			case KEYPAD_EXIT:
    				{
                        prst_num = 0;
                        nfui_nfobject_destroy(obj);
                        return TRUE;
    				}
    				break;                    
        			default:
    				break;
        		}
            }
        }
		break;

		case GDK_DELETE:
			g_curwnd = 0;		
			gtk_main_quit();
		break;

		default:
			break;
	}

	return FALSE;
}

static gboolean post_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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
        }
        break;

		default:
		break;
	}

	return FALSE;
}

static gboolean returnkey_proc(NFOBJECT *top, GdkEvent *event, gpointer data)
{
	return FALSE;
}

gint VW_PtzCtrl_rmc_prst_Open(NFWINDOW *parent)
{
    NFOBJECT *win;
	NFOBJECT *fixed;
	NFOBJECT *obj;

    prst_num = 0;

	win = (NFOBJECT*)nfui_nfwindow_new(parent, PRP_POS_X, PRP_POS_Y, PRP_WIDTH, PRP_HEIGHT);
	nfui_regi_post_event_callback(win, post_win_event_handler);
	nfui_nfwindow_set_returnkey_proc((NFWINDOW*)win, returnkey_proc);	
	g_curwnd = win;
	
	fixed =(NFOBJECT*) nfui_nffixed_new();
	nfui_regi_post_event_callback(fixed, post_fixed_event_handler);
	nfui_nfobject_show(fixed);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PRESET", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, 272, 36);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 4, 4);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("NO.", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(obj, 80, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 13, 60);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 20);
	nfui_nfobject_set_size(obj, 150, 40);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 100, 60);
    g_prst_label = obj;

	nfui_nfwindow_add((NFWINDOW*)win, fixed);

	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);
	nfui_make_key_hierarchy((NFWINDOW*)win);

	nfui_page_open(PGID_PTZ_RMC_PRST, win, nfui_get_last_user());

	gtk_main();

	nfui_page_close(PGID_PTZ_RMC_PRST, win);

	return prst_num;
}

