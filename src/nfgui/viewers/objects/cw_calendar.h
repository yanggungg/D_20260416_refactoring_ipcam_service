#ifndef __CW_CARLENDAR_H__
#define __CW_CARLENDAR_H__

#include "nfobject.h"
#include "../../support/util.h"

#include <gtk/gtk.h>
#include <cairo.h>
#include <math.h>
#include <string.h>
#include "iux_types.h"

G_BEGIN_DECLS

#define CWCLD(cwwidget) ((CWCALENDAR*)cwwidget)

#define CW_CLD_WIDTH 288
#define CW_CLD_HEIGHT 247

//#define CW_CLD_WIDTH 280
//#define CW_CLD_HEIGHT 240

#define CW_CLD_COL_WIDTH 40
#define CW_CLD_ROW_HEIGHT 40
#define CW_CLD_DAYS_YPOS 0

#define CW_CLD_CAIRO_RGB_BLACK() cairo_set_source_rgb(cr, 0, 0, 0)
#define CW_CLD_CAIRO_RGB_WHITE() cairo_set_source_rgb(cr, 255, 255, 255)
#define CW_CLD_CAIRO_RGB_TODAY() cairo_set_source_rgb(cr, 0.5, 0.6, 0.6)
#define CW_CLD_CAIRO_RGB_RECORD() cairo_set_source_rgb(cr, 183/255, 205/255, 1)

typedef struct _CWCALENDAR CWCALENDAR;

struct _CWCALENDAR {
	// Public method
	NFOBJECT object;
	nfutil_pango_spacing_type spacing_type;

	gshort w_width;
	gshort w_height;
	guint col_width;
	guint col_height;

	// current
	guint cyear;
	guint cmonth;
	guint cday;
	guint cfirst_wday;
		
	/// display
	guint dyear;
	guint dmonth;
	guint dday;
	guint dfirst_wday;
	gboolean is_dmark;

	BITMASK64 chmask;

	gchar marksday[31];
	
};

CWCALENDAR *cw_cld_new(time_t time, guint col_width, guint col_height);

gint cw_cld_change_date(CWCALENDAR *cwwidget, guint year, guint month, guint day);
time_t cw_cld_set_prev_month(CWCALENDAR *cwwidget);
time_t cw_cld_set_next_month(CWCALENDAR *cwwidget);

guint cw_cld_get_current_year(CWCALENDAR *cwwidget);
guint cw_cld_get_current_month(CWCALENDAR *cwwidget);
guint cw_cld_get_current_day(CWCALENDAR *cwwidget);

int cw_cld_reload_data(CWCALENDAR *cwwidget);
int cw_cld_update(CWCALENDAR *cwwidget);
int cw_cld_set_chmask(CWCALENDAR *cwwidget, BITMASK64 chmask);

int cw_cld_get_lastday(CWCALENDAR *cwwidget);
int cw_cld_get_days_in_month(CWCALENDAR *cwwidget, gint year, gint month);
int cw_cld_get_days_in_year(CWCALENDAR *cwwidget, gint year);

G_END_DECLS

#endif























