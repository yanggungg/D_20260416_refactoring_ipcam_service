
/**********************************************************************************
 *
 *	NFWINDOW
 *
 * *******************************************************************************/

#include <gtk/gtk.h>
#include "nfwindow.h"
#include "../../tools/nf_ui_tool.h"
#include "ix_mem.h"
#include "wnd.h"
#include "../../support/event_loop.h"
#include "cmm.h"
#include "evt.h"
#include "iux_afx.h"

#define DBG_LEVEL		0
#define DBG_MODULE		"NFWINDOW"


static gboolean _link_wnd = FALSE;

int nfui_nfwindow_link_wnd()		// link to wnd module
{
	_link_wnd = TRUE;
	return 1;
}

static gboolean _is_linked()
{
	return _link_wnd;
}


////////////////////////////////////////////////////////////////////////////////////
//

static gboolean nfwindow_event_handler(NFWINDOW *nfwin, GdkEvent *event, gpointer data)
{
	if(event->type == GDK_MAP)
	{
#if 0 //_ATM_MOUSE_
    gboolean mouse_connect_state = FALSE;

		//(!(nfwin->is_cursor))
		{
		  mouse_connect_state = nftool_check_mouse_connect();
	    if(mouse_connect_state == TRUE)
	    {
		    nfwin->is_cursor = 1;
	    }
	    else
	    {
		    nfwin->is_cursor = 0;
	    }
	    nftool_set_custom_cursor(nfui_nfobject_get_window(nfwin), mouse_connect_state);
		}
#else
		if(!(nfwin->is_cursor))
		{
			// added by hakeya. 2008-09-01
//			nftool_set_custom_cursor(nfui_nfobject_get_window(nfwin));
			nfwin->is_cursor = 1;
		}
#endif
	}
	else if(event->type == GDK_EXPOSE)
	{
#if 0 //_ATM_MOUSE_
	  gboolean mouse_connect_state = FALSE;

		//if(!(nfwin->is_cursor))
		{
		  mouse_connect_state = nftool_check_mouse_connect();

	    if(mouse_connect_state == TRUE)
	    {
		    nfwin->is_cursor = 1;
	    }
	    else
	    {
		    nfwin->is_cursor = 0;
	    }
	    nftool_set_custom_cursor(nfui_nfobject_get_window(nfwin), mouse_connect_state);
		}
#else
		nfwin->is_exposed = 1;

		if(!(nfwin->is_cursor))
		{
			// added by hakeya. 2008-09-01
//			nftool_set_custom_cursor(nfui_nfobject_get_window(nfwin));
			nfwin->is_cursor = 1;
		}
#endif
	}
	else if(event->type == GDK_DELETE)
	{
		/* moved at event_loop.c 2009/04/03 */
		/* if(nfwin->key_obj)	nfui_free_key_hierarchy(nfwin->key_obj);*/
		/* nfwin->key_obj = NULL;*/

		if(nfwin)
		{
			if(nfwin->gc)
			{
				g_object_unref(nfwin->gc);
				nfwin->gc = NULL;
			}

			if (_is_linked()) wnd_delete(nfwin);

			if(nfwin->main_widget)
			{
				gtk_widget_destroy(nfwin->main_widget);
				nfwin->main_widget = NULL;
			}

			if (nfwin->backscr) g_object_unref(nfwin->backscr);
		}
	}
	else if(event->type == WND_CREATED) {
		DMSG(1, "WND_CREATED, %p\n", nfwin);
		if(nfwin) {
			if (_is_linked()) wnd_set_level(nfwin);
		}
	}
	else if(event->type == WND_CLOSE) {
		NFWINDOW *req = ((CMM_MESSAGE_T *)data)->param;
		int retkey = (int)((CMM_MESSAGE_T *)data)->data;
		DMSG(1, "WND_CLOSE, %p, %p\n", nfwin, req);
		if(nfwin) {
			if (_is_linked()) wnd_close(nfwin, req, retkey);
		}
	}


	return FALSE;
}

#ifdef _CURSOR_DISPLAY_ONOFF
extern int mouse_connected_flag; //nskim_df08 display on/off cursor
#endif
NFWINDOW *nfui_nfwindow_new(NFWINDOW *parent, guint x, guint y, guint w, guint h)
{
	NFWINDOW *wnd;

	wnd = (NFWINDOW*)imalloc(sizeof(NFWINDOW));
	if(wnd == NULL)	return NULL;

//	wnd->strTitle....

	//wnd->main_widget = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	wnd->main_widget = gtk_window_new(GTK_WINDOW_POPUP);
	wnd->child = NULL;

	wnd->is_cursor = 0;
	wnd->semi_modal = 0;

	if(wnd->main_widget == NULL)
	{
		ifree(wnd);
		wnd = NULL;

		return NULL;
	}

//	wnd->gc = gdk_gc_new(wnd->main_widget->window);
#ifdef _CURSOR_DISPLAY_ONOFF //nskim display on/off cursor
  struct stat stat_buf;
  gchar dev_name[128];
  sprintf(dev_name, "%s", "/dev/input/mouse0");

  if (stat(dev_name, &stat_buf) < 0)
  {
    sprintf(dev_name, "%s", "/dev/input/mouse1");
	  if (stat(dev_name, &stat_buf) < 0) {
	  	    sprintf(dev_name, "%s", "/dev/input/mouse3");
		  if (stat(dev_name, &stat_buf) < 0) {
			    mouse_connected_flag = 0;
		  }else {
		    mouse_connected_flag = 1;
		  }
	  }else {
		    mouse_connected_flag = 1;
	  }
  }
  else
  {
    mouse_connected_flag = 1;
  }
#endif


	gtk_window_move(GTK_WINDOW(wnd->main_widget), x, y);
	gtk_widget_set_size_request(wnd->main_widget, w, h);
	gtk_widget_set_app_paintable(wnd->main_widget, FALSE);


	gtk_window_set_modal(GTK_WINDOW (wnd->main_widget), TRUE);
	gtk_window_set_resizable(GTK_WINDOW (wnd->main_widget), FALSE);
	gtk_window_set_decorated(GTK_WINDOW (wnd->main_widget), FALSE);


	// set object..
#if 0
	wnd->object.parent = NULL;
	wnd->object.x = x;
	wnd->object.y = y;
	wnd->object.width = w;
	wnd->object.height = h;
	wnd->object.type = NFOBJECT_TYPE_TOP;
	wnd->object.show = NFOBJECT_HIDE;
	wnd->object.use_focus = NFOBJECT_FOCUS_OFF;
	wnd->object.kfocus = NFOBJECT_UNFOCUS;
	wnd->object.mfocus = NFOBJECT_UNFOCUS;
	wnd->object.bg_color[NFOBJECT_STATE_NORMAL] = -1;
	wnd->object.bg_color[NFOBJECT_STATE_PRELIGHT] = -1;
	wnd->object.bg_color[NFOBJECT_STATE_ACTIVE] = -1;
	wnd->object.pre_event_handler = NULL;
	wnd->object.default_event_handler = nfwindow_event_handler;
	wnd->object.post_event_handler = NULL;
#else
	nfui_nfobject_init((NFOBJECT*)wnd);

	wnd->object.x = x;
	wnd->object.y = y;
	wnd->object.width = w;
	wnd->object.height = h;
	wnd->object.type = NFOBJECT_TYPE_TOP;
	wnd->object.default_event_handler = nfwindow_event_handler;

	wnd->backscr_use = 0;
	wnd->backscr = 0;
#endif

	if (_is_linked()) {
		wnd_register(parent, wnd);
		evt_send_to_widget(wnd, WND_CREATED, 0, 0, 0);
	}

	return wnd;
}

void nfui_nfwindow_add(NFWINDOW *wnd, NFOBJECT *child)
{
	if(wnd->object.type != NFOBJECT_TYPE_TOP)
	{
		return;
	}

	child->x = 0;
	child->y = 0;
	child->width = wnd->object.width;
	child->height = wnd->object.height;
	child->parent = (gpointer)wnd;
	wnd->child = child;
}

void nfui_nfwindow_set_destroy_with_parent(NFWINDOW *wnd, gboolean set)
{
	GtkWidget *real_wnd;

	if(wnd->object.type != NFOBJECT_TYPE_TOP)
	{
		return;
	}

	real_wnd = wnd->main_widget;

	gtk_window_set_destroy_with_parent(GTK_WINDOW(real_wnd), set);
}

void nfui_nfwindow_set_transient_for(NFWINDOW *wnd1, NFWINDOW *wnd2)
{
	GtkWidget *real1, *real2;

	if(wnd1->object.type != NFOBJECT_TYPE_TOP)
	{
		return;
	}

	if(wnd2->object.type != NFOBJECT_TYPE_TOP)
	{
		return;
	}

	real1 = wnd1->main_widget;
	real2 = wnd2->main_widget;

	gtk_window_set_transient_for(GTK_WINDOW(real1), GTK_WINDOW(real2));
}

void nfui_nfwindow_set_modal(NFWINDOW *wnd, gboolean modal)
{
	if(wnd->object.type != NFOBJECT_TYPE_TOP)
	{
		return;
	}

	gtk_window_set_modal(GTK_WINDOW(wnd->main_widget), modal);
}

void nfui_nfwindow_set_keyobj(NFWINDOW* wnd, KEYOBJECT *key_obj)
{
	if(!wnd)
	{
		DMSG(1, "NFWINDOW is NULL..");
		return;
	}

	if(wnd->object.type != NFOBJECT_TYPE_TOP)
	{
		return;
	}

	wnd->key_obj = key_obj;
}

KEYOBJECT* nfui_nfwindow_get_keyobj(NFWINDOW* wnd)
{
	if(!wnd)
	{
		DMSG(1, "NFWINDOW is NULL..");
		return NULL;
	}

	if(wnd->object.type != NFOBJECT_TYPE_TOP)
	{
		return NULL;
	}

	return wnd->key_obj;
}

void nfui_nfwindow_set_moving_area_size(NFWINDOW* wnd, guint size)
{
	if(!wnd)
	{
		DMSG(1, "WINDOW is NULL...");
		return;
	}

	if(wnd->object.type != NFOBJECT_TYPE_TOP)
	{
		return;
	}

	wnd->moving_area_size = size;
}

void nfui_nfwindow_set_moving_area_pos_y(NFWINDOW* wnd, guint y)
{
	if(!wnd)
	{
		DMSG(1, "WINDOW is NULL...");
		return;
	}

	if(wnd->object.type != NFOBJECT_TYPE_TOP)
	{
		return;
	}

	wnd->moving_area_y_pos = y;
}

void nfui_nfwindow_set_moving_limit(NFWINDOW* wnd, gboolean use)
{
	if(!wnd)
	{
		DMSG(1, "WINDOW is NULL...");
		return;
	}

	if(wnd->object.type != NFOBJECT_TYPE_TOP)
	{
		return;
	}

	wnd->moving_limit = use;
}

void nfui_nfwindow_set_moving_effect(NFWINDOW* wnd, gboolean use)
{
	if(!wnd)
	{
		DMSG(1, "WINDOW is NULL...");
		return;
	}

	if(wnd->object.type != NFOBJECT_TYPE_TOP)
	{
		return;
	}

	wnd->moving_effect = use;
}

void nfui_nfwindow_set_returnkey_proc(NFWINDOW *wnd, gpointer proc)
{
	if(!wnd)
	{
		DMSG(1, "WINDOW is NULL...");
		return;
	}

	if(wnd->object.type != NFOBJECT_TYPE_TOP)
	{
		return;
	}

	wnd->returnkey_proc = proc;
}

void nfui_nfwindow_use_outside_evt(NFWINDOW *wnd, gboolean use)
{
	if(!wnd)
	{
		DMSG(1, "WINDOW is NULL...");
		return;
	}

	if(wnd->object.type != NFOBJECT_TYPE_TOP)
	{
		return;
	}

	if(use)		wnd->outside_evt = 1;
	else		wnd->outside_evt = 0;
}

void nfui_nfwindow_set_mask(NFWINDOW *wnd, guint event_type, gboolean set)
{
	guint idx;
	guint bit;

	if(!wnd)
	{
		DMSG(1, "WINDOW is NULL...");
		return;
	}

	if(wnd->object.type != NFOBJECT_TYPE_TOP)
	{
		return;
	}

	idx = event_type / (sizeof(gchar)*8);
	bit = event_type % (sizeof(gchar)*8);

	if(set)	wnd->ose_mask[idx] |= (1<<bit);
	else	wnd->ose_mask[idx] &= ~(1<<bit);
}

gboolean nfui_nfwindow_is_mask(NFWINDOW* wnd, guint event_type)
{
	guint idx;
	guint bit;

	if(!wnd)
	{
		DMSG(1, "WINDOW is NULL...");
		return;
	}

	if(wnd->object.type != NFOBJECT_TYPE_TOP)
	{
		return;
	}

	idx = event_type / (sizeof(gchar)*8);
	bit = event_type % (sizeof(gchar)*8);

	return (wnd->ose_mask[idx] & (1<<bit));
}

int nfui_regi_semi_modal(NFWINDOW *wnd)
{

	NFWINDOW *top_wnd;
	if(!wnd)
	{
		DMSG(1, "WINDOW is NULL...");
		return -1;
	}

	if(wnd->object.type != NFOBJECT_TYPE_TOP)
	{
		return -1;
	}

	top_wnd = nfui_nfobject_get_top(wnd);
	top_wnd->semi_modal = 1;

	nfui_set_semimodal_wnd(top_wnd);
}

void nfui_unregi_semi_modal(NFWINDOW *wnd)
{
	NFWINDOW *top_wnd;
	if(!wnd)
	{
		DMSG(1, "WINDOW is NULL...");
		return;
	}

	if(wnd->object.type != NFOBJECT_TYPE_TOP)
	{
		return;
	}

	top_wnd = nfui_nfobject_get_top(wnd);
	top_wnd->semi_modal = 0;
}

int nfui_nfwindow_use_double_buffer(NFWINDOW *win)
{
	int w = win->object.width;
	int h = win->object.height;
	GdkWindow *rootwin;
	rootwin = gdk_get_default_root_window();
	win->backscr = gdk_pixmap_new(rootwin, w, h, -1);
	return 0;
}

int nfui_nfwindow_unuse_double_buffer(NFWINDOW *win)
{
	if (win->backscr) g_object_unref(win->backscr);
	win->backscr = 0;
	return 0;
}

int nfui_nfwindow_set_title(NFWINDOW *win, char *title)
{
	strcpy(win->strTitle, title);
	if (_is_linked()) wnd_set_title(win, title);
	return 0;
}

NFWINDOW *nfui_nfwindow_find(char *title)
{
	if (!_is_linked()) return 0;
	return wnd_find_window(title);
}

int nfui_nfwindow_close(NFWINDOW *win)
{
	if (_is_linked())
		evt_send_to_widget(win, WND_CLOSE, 0, 0, 0);
	return 0;
}

int nfui_nfwindow_is_exposed(NFWINDOW *win)
{
	return win->is_exposed;
}

NFWINDOW *nfui_nfwindow_resize(NFWINDOW *win, int width, int height)
{
	gtk_widget_set_size_request(win->main_widget, width, height);
	gtk_window_set_resizable(win->main_widget, TRUE);
	gtk_window_resize(win->main_widget, width, height);
	win->object.width = width;
	win->object.height = height;
	return win;
}
