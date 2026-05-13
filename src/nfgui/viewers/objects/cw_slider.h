#ifndef __CW_SLIDER_H__
#define __CW_SLIDER_H__

#include "nfobject.h"
#include "../../support/util.h"

#include <gtk/gtk.h>
#include <cairo.h>
#include <math.h>
#include <string.h>
//#include "cw_util.h"

G_BEGIN_DECLS

#define CWSL(cwwidget) ((CWSLIDER*)cwwidget)


typedef struct _CWSLIDER CWSLIDER;

struct _CWSLIDER {
	NFOBJECT object;
	nfutil_pango_spacing_type spacing_type;

	guint w_width;
	guint w_height;
	gint slider_value;
	gboolean is_draging;
	
	GdkPixbuf *bg_pixbuf;
	gint bg_width;
	gint bg_height;

	GdkPixbuf *bg_d_pixbuf;
	gint bg_d_width;
	gint bg_d_height;

	GdkPixbuf *slider_n_pixbuf;
	gint slider_n_width;
	gint slider_n_height;

	GdkPixbuf *slider_o_pixbuf;
	gint slider_o_width;
	gint slider_o_height;

	GdkPixbuf *slider_s_pixbuf;
	gint slider_s_width;
	gint slider_s_height;

	GdkPixbuf *slider_d_pixbuf;
	gint slider_d_width;
	gint slider_d_height;
	
	gint min;
	gint max;
	gint counts;

	gint slider_kind;

	gint bg_color_r;
	gint bg_color_g;
	gint bg_color_b;
	
};

CWSLIDER *cw_slider_new(gint slider_value, guint width, guint height);
CWSLIDER *cw_slider_small_new(gint slider_value, guint width, guint height);
void cw_slider_update(CWSLIDER *cwwidget);

void cw_slider_set_range(CWSLIDER *cwwidget, gint min, gint max, gint counts);

gint cw_slider_get_value(CWSLIDER *cwwidget);
gint cw_slider_set_value(CWSLIDER *cwwidget, gint slider_value);

void cw_slider_set_bg_color(CWSLIDER *cwwidget, gint r, gint g, gint b);
	
G_END_DECLS

#endif








