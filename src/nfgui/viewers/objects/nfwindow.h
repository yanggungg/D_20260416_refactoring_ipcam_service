#ifndef	__NFWINDOW_H__
#define	__NFWINDOW_H__


/**********************************************************************************
 *
 *	NFWINDOW Data structure
 *
 * *******************************************************************************/

#include "nfobject.h"


typedef struct {
	NFOBJECT object;

	NFOBJECT *child;
	gchar strTitle[128];
	GtkWidget *main_widget;
	GdkGC *gc;

	KEYOBJECT *key_obj;

	guint is_cursor;

	guint moving_area_size;
	guint moving_area_y_pos;
	gboolean moving_limit;
	gboolean moving_effect;

	guint outside_evt;
	gchar ose_mask[8];

	int	 is_exposed;

	int 		semi_modal;
	int			backscr_use;
	GdkPixmap	*backscr;
	gboolean (*returnkey_proc)(NFOBJECT *obj, GdkEvent *event, gpointer data);
} NFWINDOW;

NFWINDOW *nfui_nfwindow_new(NFWINDOW *parent, guint x, guint y, guint w, guint h);
void nfui_nfwindow_add(NFWINDOW *wnd, NFOBJECT *child);
void nfui_nfwindow_set_destroy_with_parent(NFWINDOW *wnd, gboolean set);
void nfui_nfwindow_set_transient_for(NFWINDOW *wnd1, NFWINDOW *wnd2);
void nfui_nfwindow_set_modal(NFWINDOW *wnd, gboolean modal);

void nfui_nfwindow_set_keyobj(NFWINDOW* wnd, KEYOBJECT *key_obj);
KEYOBJECT* nfui_nfwindow_get_keyobj(NFWINDOW* wnd);

void nfui_nfwindow_set_moving_area_size(NFWINDOW* wnd, guint size);
void nfui_nfwindow_set_moving_area_pos_y(NFWINDOW* wnd, guint y);
void nfui_nfwindow_set_moving_limit(NFWINDOW* wnd, gboolean use);
void nfui_nfwindow_set_moving_effect(NFWINDOW* wnd, gboolean use);

void nfui_nfwindow_set_returnkey_proc(NFWINDOW *wnd, gpointer proc);

void nfui_nfwindow_use_outside_evt(NFWINDOW *wnd, gboolean use);
void nfui_nfwindow_set_mask(NFWINDOW *wnd, guint event_type, gboolean set);
gboolean nfui_nfwindow_is_mask(NFWINDOW* wnd, guint event_type);

int nfui_regi_semi_modal(NFWINDOW *wnd);
void nfui_unregi_semi_modal(NFWINDOW *wnd);
int nfui_nfwindow_use_double_buffer(NFWINDOW *win);
int nfui_nfwindow_set_title(NFWINDOW *win, char *title);

NFWINDOW *nfui_nfwindow_find(char *title);
int nfui_nfwindow_close(NFWINDOW *win);
int nfui_nfwindow_is_exposed(NFWINDOW *win);
NFWINDOW *nfui_nfwindow_resize(NFWINDOW *win, int width, int height);


// special code for linking to wnd module
int nfui_nfwindow_link_wnd();

#endif	// __NFWINDOW_H__


