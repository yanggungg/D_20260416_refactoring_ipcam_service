#ifndef __CW_HCARLENDAR_H__
#define __CW_HCARLENDAR_H__

#include "nfobject.h"
#include "../../support/util.h"

#include <gtk/gtk.h>
#include <cairo.h>
#include <math.h>
#include <string.h>
#include "iux_types.h"

G_BEGIN_DECLS

#define CWHCLD(cwwidget) ((CWHCALENDAR*)cwwidget)

#define CW_HCLD_WIDTH 288
#define CW_HCLD_HEIGHT 247

//#define CW_HCLD_WIDTH 280
//#define CW_HCLD_HEIGHT 240

#define CW_HCLD_COL_WIDTH 40
#define CW_HCLD_ROW_HEIGHT 40
#define CW_HCLD_DAYS_YPOS 0

#define CW_HCLD_CAIRO_RGB_BLACK() cairo_set_source_rgb(cr, 0, 0, 0)
#define CW_HCLD_CAIRO_RGB_WHITE() cairo_set_source_rgb(cr, 255, 255, 255)
#define CW_HCLD_CAIRO_RGB_TODAY() cairo_set_source_rgb(cr, 0.5, 0.6, 0.6)
#define CW_HCLD_CAIRO_RGB_RECORD() cairo_set_source_rgb(cr, 183/255, 205/255, 1)

typedef struct _HOLIDAYCAL_INFO { 
	int year;
	int month;
	int day;
	int yoil;
	int week;
	char type;
} HOLIDAYCAL_INFO; 

typedef struct _CWHCALENDAR CWHCALENDAR;

struct _CWHCALENDAR {
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
	
	gchar marksholiday[31];

	HOLIDAYCAL_INFO hinfo[50];
	int	hcnt;
};

CWHCALENDAR *cw_hcld_new(time_t time, guint col_width, guint col_height);

gint cw_hcld_change_date(CWHCALENDAR *cwwidget, guint year, guint month, guint day);
time_t cw_hcld_set_prev_month(CWHCALENDAR *cwwidget);
time_t cw_hcld_set_next_month(CWHCALENDAR *cwwidget);

guint cw_hcld_get_current_year(CWHCALENDAR *cwwidget);
guint cw_hcld_get_current_month(CWHCALENDAR *cwwidget);
guint cw_hcld_get_current_day(CWHCALENDAR *cwwidget);

int cw_hcld_reload_data(CWHCALENDAR *cwwidget);
int cw_hcld_update(CWHCALENDAR *cwwidget);
int cw_hcld_set_chmask(CWHCALENDAR *cwwidget, BITMASK64 chmask);

int cw_hcld_get_lastday(CWHCALENDAR *cwwidget);
int cw_hcld_get_days_in_month(CWHCALENDAR *cwwidget, gint year, gint month);
int cw_hcld_get_days_in_year(CWHCALENDAR *cwwidget, gint year);

G_END_DECLS

#endif























