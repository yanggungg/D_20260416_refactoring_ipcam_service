#include "support/event_loop.h"
#include "cw_hol_calendar.h"
#include "ix_mem.h"
#include "ix_func.h"
#include "../support/color.h"
#include "scm.h"


#define	BORDER_SIZE				2

static int _is_older_day(int year, int month, int day, HOLIDAYCAL_INFO *hinfo)
{
	int x = ifn_get_totaldays(year, month, day);
	int y = ifn_get_totaldays(hinfo->year, hinfo->month, hinfo->day);

	return (x < y);
}

static int _check_same_date(CWHCALENDAR *cw, int year, int month, int day)
{
	int i;

	for (i = 0; i < cw->hcnt; ++i) {
		if ((cw->hinfo[i].year == year) && (cw->hinfo[i].month == month) && (cw->hinfo[i].day == day)) {
			return 1;
		}
	}

	return 0;
}

static int _check_yoil(CWHCALENDAR *cw, int year, int month, int day)
{
	int i;
	int yoil = ifn_get_yoil(year, month, day);

	for (i = 0; i < cw->hcnt; ++i) {
		if (_is_older_day(year, month, day, &cw->hinfo[i])) continue;
		if (cw->hinfo[i].type != 'B') continue;
		if (cw->hinfo[i].yoil == yoil) { 
			return 1;
		}
	}

	return 0;
}

static int _check_day_of_month(CWHCALENDAR *cw, int year, int month, int day)
{
	int i;

	for (i = 0; i < cw->hcnt; ++i) {
		if (_is_older_day(year, month, day, &cw->hinfo[i])) continue;
		if (cw->hinfo[i].type != 'E') continue;
		if (cw->hinfo[i].day == day) {
			return 1;
		}
	}

	return 0;
}

static int _check_date_of_year(CWHCALENDAR *cw, int year, int month, int day)
{
	int i;

	for (i = 0; i < cw->hcnt; ++i) {
		if (_is_older_day(year, month, day, &cw->hinfo[i])) continue;
		if (cw->hinfo[i].type != 'C') continue;
		if ((cw->hinfo[i].month == month) && (cw->hinfo[i].day == day)) {
			return 1;
		}
	}

	return 0;
}

static int _check_yoil_week_of_month(CWHCALENDAR *cw, int year, int month, int day)
{
	int i;
	int yoil = ifn_get_yoil(year, month, day);
	int week = ((day - 1) / 7) + 1;

	for (i = 0; i < cw->hcnt; ++i) {
		if (_is_older_day(year, month, day, &cw->hinfo[i])) continue;
		if (cw->hinfo[i].type != 'D') continue;
		if ((cw->hinfo[i].yoil == yoil) && (cw->hinfo[i].week == week)) {
			return 1;
		}
	}

	return 0;
}

int _is_holiday(CWHCALENDAR *cw, int year, int month, int day)
{
	int yoil;
	int week;

	if (!ifn_is_valid_date(year, month, day)) {
		printf("Invalid date\n");
		return -1;
	}

	yoil = ifn_get_yoil(year, month, day);
	week = ifn_get_week_of_month(day);

	printf("SKSHIN] is_holi : %d, %d, %d, %d, %d\n",
	year, month, day, yoil, week);

	if (_check_same_date(cw, year, month, day)) return 1;
	if (_check_yoil(cw, year, month, day)) return 1;
	if (_check_day_of_month(cw, year, month, day)) return 1;
	if (_check_date_of_year(cw, year, month, day)) return 1; 
	if (_check_yoil_week_of_month(cw, year, month, day)) return 1;

	return 0;
}

static guint _get_first_wday(gshort year, gshort month, gshort day)
{
	struct tm tm_ptr;
    time_t the_time;
    time_t utime;
    time_t timer;
    struct tm t;
    
    tm_ptr.tm_year = year - 1900;
    tm_ptr.tm_mon  = month - 1;
    tm_ptr.tm_mday = day;
    tm_ptr.tm_hour = 0;
    tm_ptr.tm_min  = 0;
    tm_ptr.tm_sec  = 0;
    tm_ptr.tm_isdst = 0;
    
    utime = mktime(&tm_ptr);
    
    timer = utime;
    localtime_r(&timer, &t);
    
    return t.tm_wday;
}

static guint _get_month_lastday(gshort year, gshort month)
{
	int i;
	struct tm tm_ptr;
    time_t the_time;
    time_t utime;
    
    time_t timer;
    struct tm t;
    
    for(i = 0 ; i <= 5 ; i++)
    {
    	tm_ptr.tm_year = year - 1900;
		tm_ptr.tm_mon  = month - 1;
		tm_ptr.tm_mday = 27 + i;
		tm_ptr.tm_hour = 0;
		tm_ptr.tm_min  = 0;
		tm_ptr.tm_sec  = 0;
		tm_ptr.tm_isdst = 0;
		
		utime = mktime(&tm_ptr);
    	
    	timer = utime + 86400;    	    	
	   	localtime_r(&timer, &t);	
  	
		if(t.tm_mday == 1) break;
    }
    
    timer = utime;    	    	
	localtime_r(&timer, &t);
        
    return t.tm_mday;
}

static int _get_rec_info(CWHCALENDAR *cwwidget)
{
	gint i;

	memset(cwwidget->marksday, 0x00, sizeof(cwwidget->marksday));	
	scm_get_recinfo(cwwidget->dyear, cwwidget->dmonth, cwwidget->chmask, cwwidget->marksday);
		
	return 0;
}

static int _get_holiday_info(CWHCALENDAR *cwwidget)
{
	gint i, ret;

	gint dyear;
	gint dmonth;
	gint dday;
	gint days;
#if 1
	dyear = CWHCLD(cwwidget)->dyear;
	dmonth = CWHCLD(cwwidget)->dmonth;
	dday = CWHCLD(cwwidget)->dday;

	memset(cwwidget->marksholiday, 0x00, sizeof(cwwidget->marksholiday));


	days = ifn_get_days_in_month(dyear, dmonth);

	for(i = 1; i <= days; i++){

		ret = _is_holiday(cwwidget, dyear, dmonth, i/*cday*/);
		if(ret){
			cwwidget->marksholiday[i - 1] = 1;
		}
	}
#endif
	return 0;
}

static void _prvDrawBorder(CWHCALENDAR *cwwidget)
{
	GdkDrawable* drawable;
	GdkGC *gc;

	guint left, top;
	guint width, height;

	nfui_nfobject_get_offset((NFOBJECT*)cwwidget, &left, &top);
	width = CWHCLD(cwwidget)->w_width+1;
	height = CWHCLD(cwwidget)->w_height+1;

	drawable = nfui_nfobject_get_window((NFOBJECT*)cwwidget);
	gc = nfui_nfobject_get_gc((NFOBJECT*)cwwidget);

	if (cwwidget->object.grab_kfocus)
		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(623));
	else if(cwwidget->object.kfocus == NFOBJECT_FOCUS) 
		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(622));
	else
		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(605));

	gdk_draw_rectangle(drawable, gc, TRUE, left-BORDER_SIZE, top-BORDER_SIZE, width+BORDER_SIZE*2, BORDER_SIZE);
	gdk_draw_rectangle(drawable, gc, TRUE, left-BORDER_SIZE, top-BORDER_SIZE, BORDER_SIZE, height+BORDER_SIZE*2);
	gdk_draw_rectangle(drawable, gc, TRUE, left-BORDER_SIZE, top+height, width+BORDER_SIZE*2, BORDER_SIZE);
	gdk_draw_rectangle(drawable, gc, TRUE, left+width, top-BORDER_SIZE, BORDER_SIZE, height+BORDER_SIZE*2);

	nfui_nfobject_gc_unref(gc);
}


static void _draw_holidays(CWHCALENDAR *cwwidget, cairo_t *cr, GdkGC *gc, GdkDrawable *drawable)
{
	int i, j;
	guint x, y;
    	gint holiday, select_day;

	GdkColor color;
	gint bg_color, font_color;
	
	nfui_nfobject_get_offset((NFOBJECT*)cwwidget, &x, &y);
	
	int xpos = x;
	int ypos = CW_HCLD_DAYS_YPOS + cwwidget->col_height + y;
	
	int lastday = _get_month_lastday(CWHCLD(cwwidget)->dyear, CWHCLD(cwwidget)->dmonth);
	char text[5];
	char dispDate[4];
	char *buf_date = 0;

	int cyear = CWHCLD(cwwidget)->cyear;
	int cmonth = CWHCLD(cwwidget)->cmonth;
	int cday = CWHCLD(cwwidget)->cday;

	gdouble text_xpos, text_ypos;


	g_message("func : %s CWHCLD(cwwidget)->cyear:%d, CWHCLD(cwwidget)->cmonth:%d",__func__, CWHCLD(cwwidget)->cyear, CWHCLD(cwwidget)->cmonth);
	g_message("func : %s CWHCLD(cwwidget)->dyear:%d, CWHCLD(cwwidget)->dmonth:%d",__func__, CWHCLD(cwwidget)->dyear, CWHCLD(cwwidget)->dmonth);


	CWHCLD(cwwidget)->dfirst_wday = _get_first_wday(CWHCLD(cwwidget)->dyear, CWHCLD(cwwidget)->dmonth, 1);
	g_message("func : %s CWHCLD(cwwidget)->cfirst_wday:%d",__func__, CWHCLD(cwwidget)->cfirst_wday);
	g_message("func : %s CWHCLD(cwwidget)->dfirst_wday:%d",__func__, CWHCLD(cwwidget)->dfirst_wday);
	
	g_message("func : %s CWHCLD(cwwidget)->cday:%d",__func__, CWHCLD(cwwidget)->cday);
	g_message("func : %s CWHCLD(cwwidget)->dday:%d",__func__, CWHCLD(cwwidget)->dday);

	xpos = CWHCLD(cwwidget)->dfirst_wday * (cwwidget->col_width);
	xpos += x;

	j = CWHCLD(cwwidget)->dfirst_wday;
		
	for(i = 1; i <= lastday ; i++)
	{
		GdkColor color;
		gdouble red, green, blue;

		if(cwwidget->marksholiday[i - 1] == 1)  holiday = 1;
		else                                holiday = 0;

		if( CWHCLD(cwwidget)->cyear == CWHCLD(cwwidget)->dyear &&
				CWHCLD(cwwidget)->cmonth == CWHCLD(cwwidget)->dmonth &&
				CWHCLD(cwwidget)->cday == i )
			select_day = 1;
		else
			select_day = 0;    	

#if 1 
		if (holiday)
		{
				bg_color = 616; 

				if (j == 0)         font_color = 612;
				else if (j == 6)    font_color = 612;
				else                font_color = 612;
#if 0
			if (select_day)
			{
				bg_color = 628; 

				if (j == 0)         font_color = 612;
				else if (j == 6)    font_color = 614;
				else                font_color = 629;
			}
			else
			{
				bg_color = 623; 

				if (j == 0)         font_color = 612;
				else if (j == 6)    font_color = 614;
				else                font_color = 625;
			}
#endif
		}
		else
		{
				bg_color = 616; 

				if (j == 0)         font_color = 617;
				else if (j == 6)    font_color = 617;
				else                font_color = 617;
#if 0
			if (select_day)
			{
				bg_color = 620; 

				if (j == 0)         font_color = 612;
				else if (j == 6)    font_color = 614;
				else                font_color = 621;                
			}
			else
			{
				bg_color = 616; 

				if (j == 0)         font_color = 612;
				else if (j == 6)    font_color = 614;
				else                font_color = 617;
			}
#endif
		}
#endif




#if 0
		if (record_day)
		{
			if (select_day)
			{
				bg_color = 628; 

				if (j == 0)         font_color = 612;
				else if (j == 6)    font_color = 614;
				else                font_color = 629;
			}
			else
			{
				bg_color = 624; 

				if (j == 0)         font_color = 612;
				else if (j == 6)    font_color = 614;
				else                font_color = 625;
			}
		}
		else
		{
			if (select_day)
			{
				bg_color = 620; 

				if (j == 0)         font_color = 612;
				else if (j == 6)    font_color = 614;
				else                font_color = 621;                
			}
			else
			{
				bg_color = 616; 

				if (j == 0)         font_color = 612;
				else if (j == 6)    font_color = 614;
				else                font_color = 617;
			}
		}
#endif
		color = UX_COLOR(bg_color);
		red = (color.red >> 8);
		green = (color.green >> 8);
		blue = (color.blue >> 8);						

		cairo_set_source_rgb(cr, red/255.0, green/255.0, blue/255.0);
		cairo_rectangle(cr, xpos+1, ypos-cwwidget->col_height+1, cwwidget->col_width-2, cwwidget->col_height-2);
		cairo_fill(cr);
		cairo_set_source_rgb(cr, 0, 0, 0);

		if (select_day)
		{
			color = UX_COLOR(623);
			red = (color.red >> 8);
			green = (color.green >> 8);
			blue = (color.blue >> 8);			

			cairo_set_source_rgb(cr, red/255.0, green/255.0, blue/255.0);
			cairo_rectangle(cr, xpos+1, ypos - cwwidget->col_height+1, cwwidget->col_width-1, cwwidget->col_height-1);
			cairo_stroke(cr);
			cairo_set_source_rgb(cr, 0, 0, 0);
		}

		sprintf(text, "%d", i);

		color = UX_COLOR(font_color);
		gdk_gc_set_rgb_fg_color(gc, &color);		
		nfutil_draw_text_with_pango(NULL, NULL, NULL, drawable, NULL, text,
				xpos, ypos-cwwidget->col_height, cwwidget->col_width, cwwidget->col_height, 
				nffont_get_pango_font(NFFONT_MINI_NORMAL_4), &color, NFALIGN_CENTER, 0);

		xpos += (cwwidget->col_width);

		if(j == 6){
			xpos = x;
			ypos += (cwwidget->col_height);
			j = 0;

		}else{
			j++;
		}

	}
}


static void _draw_days(CWHCALENDAR *cwwidget, cairo_t *cr, GdkGC *gc, GdkDrawable *drawable)
{
	int i, j;
	guint x, y;
    	gint record_day, select_day;

	GdkColor color;
	gint bg_color, font_color;
	
	nfui_nfobject_get_offset((NFOBJECT*)cwwidget, &x, &y);
	
	int xpos = x;
	int ypos = CW_HCLD_DAYS_YPOS + cwwidget->col_height + y;
	
	int lastday = _get_month_lastday(CWHCLD(cwwidget)->dyear, CWHCLD(cwwidget)->dmonth);
	char text[5];
	char dispDate[4];
	char *buf_date = 0;

	gdouble text_xpos, text_ypos;


	CWHCLD(cwwidget)->dfirst_wday = _get_first_wday(CWHCLD(cwwidget)->dyear, CWHCLD(cwwidget)->dmonth, 1);

	xpos = CWHCLD(cwwidget)->dfirst_wday * (cwwidget->col_width);
	xpos += x;
	
	j = CWHCLD(cwwidget)->dfirst_wday;
		
	for(i = 1; i <= lastday ; i++)
	{
		GdkColor color;
		gdouble red, green, blue;

		
		if(cwwidget->marksday[i - 1] == 1)  record_day = 1;
		else                                record_day = 0;

		if( CWHCLD(cwwidget)->cyear == CWHCLD(cwwidget)->dyear &&
				CWHCLD(cwwidget)->cmonth == CWHCLD(cwwidget)->dmonth &&
				CWHCLD(cwwidget)->cday == i )
			select_day = 1;
		else
			select_day = 0;    	


#if 1
		if (record_day)
		{
			if (select_day)
			{
				bg_color = 628; 

				if (j == 0)         font_color = 612;
				else if (j == 6)    font_color = 614;
				else                font_color = 629;
			}
			else
			{
				bg_color = 624; 

				if (j == 0)         font_color = 612;
				else if (j == 6)    font_color = 614;
				else                font_color = 625;
			}
		}
		else
		{
			if (select_day)
			{
				bg_color = 620; 

				if (j == 0)         font_color = 612;
				else if (j == 6)    font_color = 614;
				else                font_color = 621;                
			}
			else
			{
				bg_color = 616; 

				if (j == 0)         font_color = 612;
				else if (j == 6)    font_color = 614;
				else                font_color = 617;
			}
		}
#endif
		color = UX_COLOR(bg_color);
		red = (color.red >> 8);
		green = (color.green >> 8);
		blue = (color.blue >> 8);						

		cairo_set_source_rgb(cr, red/255.0, green/255.0, blue/255.0);
		cairo_rectangle(cr, xpos+1, ypos-cwwidget->col_height+1, cwwidget->col_width-2, cwwidget->col_height-2);
		cairo_fill(cr);
		cairo_set_source_rgb(cr, 0, 0, 0);

		if (select_day)
		{
			color = UX_COLOR(623);
			red = (color.red >> 8);
			green = (color.green >> 8);
			blue = (color.blue >> 8);			

			cairo_set_source_rgb(cr, red/255.0, green/255.0, blue/255.0);
			cairo_rectangle(cr, xpos+1, ypos - cwwidget->col_height+1, cwwidget->col_width-1, cwwidget->col_height-1);
			cairo_stroke(cr);
			cairo_set_source_rgb(cr, 0, 0, 0);
		}

		sprintf(text, "%d", i);

		color = UX_COLOR(font_color);
		gdk_gc_set_rgb_fg_color(gc, &color);		
		nfutil_draw_text_with_pango(NULL, NULL, NULL, drawable, NULL, text,
				xpos, ypos-cwwidget->col_height, cwwidget->col_width, cwwidget->col_height, 
				nffont_get_pango_font(NFFONT_MINI_NORMAL_4), &color, NFALIGN_CENTER, 0);

		xpos += (cwwidget->col_width);

		if(j == 6){
			xpos = x;
			ypos += (cwwidget->col_height);
			j = 0;

		}else{
			j++;
		}

	}
}

static void _draw_hol_calendar_paint(CWHCALENDAR *cwwidget)
{
	GdkDrawable *drawable;
	cairo_t *cr;
	GdkGC *gc;
	guint x, y;

	nfui_nfobject_get_offset((NFOBJECT*)cwwidget, &x, &y);
	
	drawable = nfui_nfobject_get_window((NFOBJECT*)cwwidget);
	cr = gdk_cairo_create( drawable );
	gc = nfui_nfobject_get_gc((NFOBJECT*)cwwidget);
	
	gdk_gc_set_rgb_fg_color(GDK_GC(gc), &UX_COLOR(616));
	gdk_draw_rectangle(drawable, gc, TRUE,
							x, y, 
							CWHCLD(cwwidget)->w_width, CWHCLD(cwwidget)->w_height);

	gdk_gc_set_rgb_fg_color(GDK_GC(gc), &UX_COLOR(608));
	
	int i, values = 0;
	for( i = 0 ; i < 7 ; i++, values += (cwwidget->col_height) )
	{
		gdk_draw_line(drawable, gc, 
				x, y + values, 
				x + CWHCLD(cwwidget)->w_width, y + values);
	} 

	values = 0;
	for( i = 0 ; i < 8 ; i++, values += (cwwidget->col_width) )
	{

		gdk_draw_line(drawable, gc, 
				x + values, y, 
				x + values, y + CWHCLD(cwwidget)->w_height);		
	}
	
	_draw_holidays(cwwidget, cr, gc, drawable);
	//_draw_days(cwwidget, cr, gc, drawable);
	
	nfui_nfobject_gc_unref(gc);
	cairo_destroy (cr);
}

static void _draw_calendar_paint(CWHCALENDAR *cwwidget)
{
	GdkDrawable *drawable;
	cairo_t *cr;
	GdkGC *gc;
	guint x, y;

	nfui_nfobject_get_offset((NFOBJECT*)cwwidget, &x, &y);
	
	drawable = nfui_nfobject_get_window((NFOBJECT*)cwwidget);
	cr = gdk_cairo_create( drawable );
	gc = nfui_nfobject_get_gc((NFOBJECT*)cwwidget);
	
	gdk_gc_set_rgb_fg_color(GDK_GC(gc), &UX_COLOR(616));
	gdk_draw_rectangle(drawable, gc, TRUE,
							x, y, 
							CWHCLD(cwwidget)->w_width, CWHCLD(cwwidget)->w_height);

	gdk_gc_set_rgb_fg_color(GDK_GC(gc), &UX_COLOR(608));
	
	int i, values = 0;
	for( i = 0 ; i < 7 ; i++, values += (cwwidget->col_height) )
	{
		gdk_draw_line(drawable, gc, 
				x, y + values, 
				x + CWHCLD(cwwidget)->w_width, y + values);
	} 

	values = 0;
	for( i = 0 ; i < 8 ; i++, values += (cwwidget->col_width) )
	{

		gdk_draw_line(drawable, gc, 
				x + values, y, 
				x + values, y + CWHCLD(cwwidget)->w_height);		
	}
	
	//_draw_days(cwwidget, cr, gc, drawable);
	_draw_holidays(cwwidget, cr, gc, drawable);
	
	nfui_nfobject_gc_unref(gc);
	cairo_destroy (cr);
}

static gint _flip_over_page(CWHCALENDAR *cwwidget, guint year, guint month)
{
	if ((cwwidget->dyear == year) && (cwwidget->dmonth == month))
		return -1;

	cwwidget->is_dmark = FALSE;

	CWHCLD(cwwidget)->dyear = year;
	CWHCLD(cwwidget)->dmonth = month;
	cwwidget->dday = 1;
	_get_rec_info(cwwidget);	
	_draw_hol_calendar_paint(cwwidget);

	return 0;
}

static gint _set_date(CWHCALENDAR *cwwidget, guint year, guint month, guint day)
{
	cwwidget->cyear = year;
	cwwidget->cmonth = month;
	cwwidget->cday = day;
	cwwidget->cfirst_wday = _get_first_wday(CWHCLD(cwwidget)->cyear, CWHCLD(cwwidget)->cmonth, 1);
	
	cwwidget->dyear = CWHCLD(cwwidget)->cyear;
	cwwidget->dmonth = CWHCLD(cwwidget)->cmonth;
	cwwidget->dday = CWHCLD(cwwidget)->cday;
	cwwidget->dfirst_wday = CWHCLD(cwwidget)->cfirst_wday;

	_draw_hol_calendar_paint(cwwidget);

	return 0;
}

static void _button_press(CWHCALENDAR *cwwidget, gint x, gint y)
{
	int i, j;
	int xpos = 0;
	int ypos = CW_HCLD_DAYS_YPOS;
	int lastday = _get_month_lastday(CWHCLD(cwwidget)->dyear, CWHCLD(cwwidget)->dmonth);
	
	xpos = CWHCLD(cwwidget)->dfirst_wday * cwwidget->col_width;
	j = CWHCLD(cwwidget)->dfirst_wday;

	guint xx, yy;
	nfui_nfobject_get_offset((NFOBJECT*)cwwidget, &xx, &yy);
	
	xpos += xx;
	ypos += yy;
	
	for(i = 1; i <= lastday ; i++)
	{
		if( x > xpos && x < (xpos + cwwidget->col_width) &&
			y > ypos && y < (ypos + cwwidget->col_height) )
		{
			cwwidget->is_dmark = TRUE;
			
			CWHCLD(cwwidget)->cmonth = CWHCLD(cwwidget)->dmonth;
			CWHCLD(cwwidget)->cyear = CWHCLD(cwwidget)->dyear;
			
			CWHCLD(cwwidget)->cday = i;
			CWHCLD(cwwidget)->dday = i;
			_draw_hol_calendar_paint(cwwidget);
			nfui_user_signal_emit((NFOBJECT*)cwwidget, NFEVENT_CALENDAR_CHANGED_RELEASE, FALSE);
			
			break;			
		} 

		xpos += cwwidget->col_width;
			
		if(j == 6){
			xpos = xx;
			ypos += cwwidget->col_height;
			j = 0;
		}else{
			j++;
		}
	}
}

static void _prvLeftButtonProc(CWHCALENDAR *cwwidget)
{
	GDate *curDate;

	gint new_day = 0;
	gint new_mon = 0;
	gint new_year = 0;

	gint cmp_date = 0;

	curDate = g_date_new();
	g_date_set_dmy(curDate, cwwidget->cday, cwwidget->cmonth, cwwidget->cyear);
	g_date_subtract_days(curDate, 1);

	new_day = g_date_get_day(curDate);
	new_mon = g_date_get_month(curDate);
	new_year = g_date_get_year(curDate);
	
	g_message("%s, %d, year:%d, month:%d, day:%d", __FUNCTION__, __LINE__, new_year, new_mon, new_day);

	_flip_over_page(cwwidget, new_year, new_mon);
	_set_date(cwwidget, new_year, new_mon, new_day);
	_draw_hol_calendar_paint(cwwidget);
	nfui_user_signal_emit((NFOBJECT*)cwwidget, NFEVENT_CALENDAR_CHANGED_RELEASE, FALSE);

	g_date_free(curDate);
}

static void _prvRightButtonProc(CWHCALENDAR *cwwidget)
{
	GDate *curDate;

	gint new_day = 0;
	gint new_mon = 0;
	gint new_year = 0;

	gint cmp_date = 0;

	curDate = g_date_new();
	g_date_set_dmy(curDate, cwwidget->cday, cwwidget->cmonth, cwwidget->cyear);
	g_date_add_days(curDate, 1);

	new_day = g_date_get_day(curDate);
	new_mon = g_date_get_month(curDate);
	new_year = g_date_get_year(curDate);
	
	g_message("%s, %d, year:%d, month:%d, day:%d", __FUNCTION__, __LINE__, new_year, new_mon, new_day);

	_flip_over_page(cwwidget, new_year, new_mon);
	_set_date(cwwidget, new_year, new_mon, new_day);
	_draw_hol_calendar_paint(cwwidget);
	nfui_user_signal_emit((NFOBJECT*)cwwidget, NFEVENT_CALENDAR_CHANGED_RELEASE, FALSE);

	g_date_free(curDate);
}

static void _prvUpButtonProc(CWHCALENDAR *cwwidget)
{
	GDate *curDate;

	gint new_day = 0;
	gint new_mon = 0;
	gint new_year = 0;

	gint cmp_date = 0;

	curDate = g_date_new();
	g_date_set_dmy(curDate, cwwidget->cday, cwwidget->cmonth, cwwidget->cyear);
	g_date_subtract_days(curDate, 7);

	new_day = g_date_get_day(curDate);
	new_mon = g_date_get_month(curDate);
	new_year = g_date_get_year(curDate);
	
	g_message("%s, %d, year:%d, month:%d, day:%d", __FUNCTION__, __LINE__, new_year, new_mon, new_day);

	_flip_over_page(cwwidget, new_year, new_mon);
	_set_date(cwwidget, new_year, new_mon, new_day);
	_draw_hol_calendar_paint(cwwidget);
	nfui_user_signal_emit((NFOBJECT*)cwwidget, NFEVENT_CALENDAR_CHANGED_RELEASE, FALSE);

	g_date_free(curDate);
}

static void _prvDownButtonProc(CWHCALENDAR *cwwidget)
{
	GDate *curDate;

	gint new_day = 0;
	gint new_mon = 0;
	gint new_year = 0;

	gint cmp_date = 0;

	curDate = g_date_new();
	g_date_set_dmy(curDate, cwwidget->cday, cwwidget->cmonth, cwwidget->cyear);
	g_date_add_days(curDate, 7);

	new_day = g_date_get_day(curDate);
	new_mon = g_date_get_month(curDate);
	new_year = g_date_get_year(curDate);
	
	g_message("%s, %d, year:%d, month:%d, day:%d", __FUNCTION__, __LINE__, new_year, new_mon, new_day);

	_flip_over_page(cwwidget, new_year, new_mon);
	_set_date(cwwidget, new_year, new_mon, new_day);
	_draw_hol_calendar_paint(cwwidget);
	nfui_user_signal_emit((NFOBJECT*)cwwidget, NFEVENT_CALENDAR_CHANGED_RELEASE, FALSE);

	g_date_free(curDate);
}


static gboolean _cw_hcld_holiday_event_handler(CWHCALENDAR *cwwidget, GdkEvent *event, gpointer data)
{
	switch(event->type)	
	{
		case GDK_EXPOSE :
			if(cwwidget->object.kfocus != NFOBJECT_FOCUS)
				cwwidget->object.grab_kfocus = FALSE;
		
			_draw_hol_calendar_paint(cwwidget);
			_prvDrawBorder(cwwidget);
			break;
			
		case GDK_ENTER_NOTIFY :	
			if(nfui_nfobject_is_shown((NFOBJECT*)cwwidget))
                _prvDrawBorder(cwwidget);
//				nfui_signal_emit((NFOBJECT*)cwwidget, GDK_EXPOSE, FALSE);
			break;
			
		case GDK_LEAVE_NOTIFY :
			if(nfui_nfobject_is_shown((NFOBJECT*)cwwidget))
                _prvDrawBorder(cwwidget);
//				nfui_signal_emit((NFOBJECT*)cwwidget, GDK_EXPOSE, FALSE);
			break;
			
		case GDK_2BUTTON_PRESS :
			break;
		case GDK_BUTTON_PRESS :	
			_button_press(cwwidget, event->button.x , event->button.y);
			break;
		case GDK_BUTTON_RELEASE :
			{
			}
			break;
		case GDK_MOTION_NOTIFY :
			break;	
		case NFEVENT_KEYPAD_PRESS :
		case NFEVENT_REMOCON_PRESS :
		{
			GdkEventKey *kevt;
			KEYPAD_KID kpid;

			kevt = (GdkEventKey*)event;
			kpid = (KEYPAD_KID)kevt->keyval;

			switch(kpid)
			{
				case KEYPAD_ENTER:
					if(cwwidget->object.grab_kfocus)
					{
						cwwidget->object.grab_kfocus = FALSE;
					}
					else
					{
						cwwidget->object.grab_kfocus = TRUE;
					}
					nfui_signal_emit((NFOBJECT*)cwwidget, GDK_EXPOSE, TRUE);
					return TRUE;					
 				break;
				case KEYPAD_EXIT:
					if(cwwidget->object.grab_kfocus)
					{
						cwwidget->object.grab_kfocus = FALSE;
						nfui_signal_emit((NFOBJECT*)cwwidget, GDK_EXPOSE, TRUE);
						return TRUE;						
					}
 				break;				
				case KEYPAD_LEFT:
					if(cwwidget->object.grab_kfocus) {
						_prvLeftButtonProc(cwwidget);
						return TRUE;
					}
				break;
				case KEYPAD_RIGHT:
					if(cwwidget->object.grab_kfocus) {
						_prvRightButtonProc(cwwidget);
						return TRUE;
					}
				break;
				case KEYPAD_UP:
					if(cwwidget->object.grab_kfocus) {
						_prvUpButtonProc(cwwidget);
						return TRUE;
					}				
				break;
				case KEYPAD_DOWN:
					if(cwwidget->object.grab_kfocus) {
						_prvDownButtonProc(cwwidget);
						return TRUE;
					}				
				break;			
				default:
 				break;
			}
		}
		break;			
		case GDK_DELETE :	
			break;
		default :
			break;
	}

	return FALSE;
	
}


static gboolean _cw_hcld_event_handler(CWHCALENDAR *cwwidget, GdkEvent *event, gpointer data)
{
	switch(event->type)	
	{
		case GDK_EXPOSE :
			if(cwwidget->object.kfocus != NFOBJECT_FOCUS)
				cwwidget->object.grab_kfocus = FALSE;
		
			_draw_hol_calendar_paint(cwwidget);
			_prvDrawBorder(cwwidget);
			break;
			
		case GDK_ENTER_NOTIFY :	
			if(nfui_nfobject_is_shown((NFOBJECT*)cwwidget))
                _prvDrawBorder(cwwidget);
//				nfui_signal_emit((NFOBJECT*)cwwidget, GDK_EXPOSE, FALSE);
			break;
			
		case GDK_LEAVE_NOTIFY :
			if(nfui_nfobject_is_shown((NFOBJECT*)cwwidget))
                _prvDrawBorder(cwwidget);
//				nfui_signal_emit((NFOBJECT*)cwwidget, GDK_EXPOSE, FALSE);
			break;
			
		case GDK_2BUTTON_PRESS :
			break;
		case GDK_BUTTON_PRESS :	
			_button_press(cwwidget, event->button.x , event->button.y);
			break;
		case GDK_BUTTON_RELEASE :
			{
			}
			break;
		case GDK_MOTION_NOTIFY :
			break;	
		case NFEVENT_KEYPAD_PRESS :
		case NFEVENT_REMOCON_PRESS :
		{
			GdkEventKey *kevt;
			KEYPAD_KID kpid;

			kevt = (GdkEventKey*)event;
			kpid = (KEYPAD_KID)kevt->keyval;

			switch(kpid)
			{
				case KEYPAD_ENTER:
					if(cwwidget->object.grab_kfocus)
					{
						cwwidget->object.grab_kfocus = FALSE;
					}
					else
					{
						cwwidget->object.grab_kfocus = TRUE;
					}
					nfui_signal_emit((NFOBJECT*)cwwidget, GDK_EXPOSE, TRUE);
					return TRUE;					
 				break;
				case KEYPAD_EXIT:
					if(cwwidget->object.grab_kfocus)
					{
						cwwidget->object.grab_kfocus = FALSE;
						nfui_signal_emit((NFOBJECT*)cwwidget, GDK_EXPOSE, TRUE);
						return TRUE;						
					}
 				break;				
				case KEYPAD_LEFT:
					if(cwwidget->object.grab_kfocus) {
						_prvLeftButtonProc(cwwidget);
						return TRUE;
					}
				break;
				case KEYPAD_RIGHT:
					if(cwwidget->object.grab_kfocus) {
						_prvRightButtonProc(cwwidget);
						return TRUE;
					}
				break;
				case KEYPAD_UP:
					if(cwwidget->object.grab_kfocus) {
						_prvUpButtonProc(cwwidget);
						return TRUE;
					}				
				break;
				case KEYPAD_DOWN:
					if(cwwidget->object.grab_kfocus) {
						_prvDownButtonProc(cwwidget);
						return TRUE;
					}				
				break;			
				default:
 				break;
			}
		}
		break;			
		case GDK_DELETE :	
			break;
		default :
			break;
	}

	return FALSE;
	
}



// [Public Member Method and function]

CWHCALENDAR *cw_hcld_new(time_t time, guint col_width, guint col_height)
{
	CWHCALENDAR *cwwidget;

	cwwidget = (CWHCALENDAR*)imalloc(sizeof(CWHCALENDAR));
	
	nfui_nfobject_init((NFOBJECT*)cwwidget);

	cwwidget->w_width = col_width * 7;
	cwwidget->w_height = col_height * 6;
	cwwidget->col_width 	= col_width;
	cwwidget->col_height	= col_height;

	cwwidget->w_width = cwwidget->col_width * 7;  // ���� ����ũ��
	cwwidget->w_height = cwwidget->col_height * 6; // ���� ����ũ��
	cwwidget->object.width = cwwidget->col_width * 7;
	cwwidget->object.height = cwwidget->col_height * 6;

	cwwidget->object.type = NFOBJECT_TYPE_CWHCALENDAR;
	cwwidget->object.use_focus = NFOBJECT_FOCUS_ON;
	cwwidget->object.default_event_handler = _cw_hcld_event_handler;

	cwwidget->spacing_type = NORMAL_SPACING;

	memset(cwwidget->marksday, 0x00, sizeof(cwwidget->marksday));	
	
	cwwidget->is_dmark = FALSE;
	cwwidget->chmask = 0x8000000000000000ll;

	dtf_get_local_day(time, &cwwidget->cyear, &cwwidget->cmonth, &cwwidget->cday);
	cwwidget->cfirst_wday = _get_first_wday(CWHCLD(cwwidget)->cyear, CWHCLD(cwwidget)->cmonth, 1);
	cwwidget->dyear = CWHCLD(cwwidget)->cyear;
	cwwidget->dmonth = CWHCLD(cwwidget)->cmonth;
	cwwidget->dday = CWHCLD(cwwidget)->cday;
	cwwidget->dfirst_wday = CWHCLD(cwwidget)->cfirst_wday;

	_get_rec_info(cwwidget);

	return cwwidget;
	
}

CWHCALENDAR *cw_hcld_holiday_new(time_t time, guint col_width, guint col_height)
{
	CWHCALENDAR *cwwidget;

	cwwidget = (CWHCALENDAR*)imalloc(sizeof(CWHCALENDAR));
	
	nfui_nfobject_init((NFOBJECT*)cwwidget);

	cwwidget->w_width = col_width * 7;
	cwwidget->w_height = col_height * 6;
	cwwidget->col_width 	= col_width;
	cwwidget->col_height	= col_height;

	cwwidget->w_width = cwwidget->col_width * 7;  // ���� ����ũ��
	cwwidget->w_height = cwwidget->col_height * 6; // ���� ����ũ��
	cwwidget->object.width = cwwidget->col_width * 7;
	cwwidget->object.height = cwwidget->col_height * 6;

	cwwidget->object.type = NFOBJECT_TYPE_CWCALENDAR;
	cwwidget->object.use_focus = NFOBJECT_FOCUS_ON;
	//cwwidget->object.default_event_handler = _cw_hcld_event_handler;
	cwwidget->object.default_event_handler = _cw_hcld_holiday_event_handler;

	cwwidget->spacing_type = NORMAL_SPACING;

	memset(cwwidget->marksday, 0x00, sizeof(cwwidget->marksday));	
	
	cwwidget->is_dmark = FALSE;
	cwwidget->chmask = 0x8000000000000000ll;

	dtf_get_local_day(time, &cwwidget->cyear, &cwwidget->cmonth, &cwwidget->cday);
	cwwidget->cfirst_wday = _get_first_wday(CWHCLD(cwwidget)->cyear, CWHCLD(cwwidget)->cmonth, 1);
	cwwidget->dyear = CWHCLD(cwwidget)->cyear;
	cwwidget->dmonth = CWHCLD(cwwidget)->cmonth;
	cwwidget->dday = CWHCLD(cwwidget)->cday;
	cwwidget->dfirst_wday = CWHCLD(cwwidget)->cfirst_wday;

//	_get_holiday_info(cwwidget);

	return cwwidget;
	
}


gint cw_hcld_change_date(CWHCALENDAR *cwwidget, guint year, guint month, guint day)
{
	printf("CWHCALENDAR: %d, %d, %d, %d, %d, %d, %d, %d, %d",
		cwwidget->cyear, year, cwwidget->cmonth, month, cwwidget->cday,day,
		cwwidget->dyear, cwwidget->dmonth, cwwidget->dday);

	if ((cwwidget->dyear == year) && (cwwidget->dmonth == month) &&
		(cwwidget->dday == day))
		return -1;
/*	if ((cwwidget->cyear == year) && (cwwidget->cmonth == month) &&
		(cwwidget->cday == day))
		return -1;*/

	_flip_over_page(cwwidget, year, month);
	_set_date(cwwidget, year, month, day);

	return 0;
}

time_t cw_hcld_set_prev_month(CWHCALENDAR *cwwidget)
{
	time_t timet;
	int year, mon, day;

	year = CWHCLD(cwwidget)->dyear;
	mon = CWHCLD(cwwidget)->dmonth;
	mon--;
	if(mon < 1)	{
		year--;
		mon = 12;
	}
	day = 1;
	timet = ifn_get_gmt_from_date(year, mon, day);
	if (timet < NF_LOWER_TIMELIMIT) return 0;

	cwwidget->is_dmark = FALSE;
	CWHCLD(cwwidget)->dmonth--;
	if(CWHCLD(cwwidget)->dmonth < 1)	{
		CWHCLD(cwwidget)->dyear--;
		CWHCLD(cwwidget)->dmonth = 12;
	}
	
	cwwidget->dday = 1;
	//_get_rec_info(cwwidget);	
	_get_holiday_info(cwwidget);
	_draw_hol_calendar_paint(cwwidget);

	return timet;
}

time_t cw_hcld_set_next_month(CWHCALENDAR *cwwidget)
{
	time_t timet;
	int year, mon, day;

	year = CWHCLD(cwwidget)->dyear;
	mon = CWHCLD(cwwidget)->dmonth;
	mon++;
	if(mon > 12)	{
		year++;
		mon = 1;
	}
	day = 1;
	timet = ifn_get_gmt_from_date(year, mon, day);
	if (timet > NF_UPPER_TIMELIMIT) return 0;

	cwwidget->is_dmark = FALSE;
	CWHCLD(cwwidget)->dmonth++;
	if(CWHCLD(cwwidget)->dmonth > 12) {
		CWHCLD(cwwidget)->dyear++;
		CWHCLD(cwwidget)->dmonth = 1;
	}
	
	cwwidget->dday = 1;
	//_get_rec_info(cwwidget);	
	_get_holiday_info(cwwidget);
	_draw_hol_calendar_paint(cwwidget);

	return timet;
}

guint cw_hcld_get_current_year(CWHCALENDAR *cwwidget)
{
	return cwwidget->dyear;
}

guint cw_hcld_get_current_month(CWHCALENDAR *cwwidget)
{
	return cwwidget->dmonth;
}

guint cw_hcld_get_current_day(CWHCALENDAR *cwwidget)
{
	return cwwidget->dday;
}

int cw_hcld_reload_data(CWHCALENDAR *cwwidget)
{
	_get_rec_info(cwwidget);	
	return 0;
}

int cw_hcld_update(CWHCALENDAR *cwwidget)
{
	_draw_hol_calendar_paint(cwwidget);
	return 0;
}

int cw_hcld_set_chmask(CWHCALENDAR *cwwidget, BITMASK64 chmask)
{
	cwwidget->chmask = chmask;
	return 0;
}

int cw_hcld_get_lastday(CWHCALENDAR *cwwidget)
{
    int lastday;
	lastday = _get_month_lastday(CWHCLD(cwwidget)->dyear, CWHCLD(cwwidget)->dmonth);
	
	return lastday;
}

int cw_hcld_get_days_in_month(CWHCALENDAR *cwwidget, gint year, gint month)
{
    int day_count;
    
	day_count = _get_month_lastday(year, month);
	return day_count;
}

int cw_hcld_get_days_in_year(CWHCALENDAR *cwwidget, gint year)
{
    int i, day_count = 0;

    for (i = 1; i < 13; i++)
    {
    	day_count += _get_month_lastday(CWHCLD(cwwidget)->dyear, i);
    }
    
	return day_count;
}

static int _generate_holiday_info(CWHCALENDAR *cwwidget, HOLIDAYCAL_INFO *hinfo, int hcnt)
{

	memset(cwwidget->hinfo, 0x00, sizeof(cwwidget->hinfo));
	memcpy(cwwidget->hinfo, hinfo, sizeof(HOLIDAYCAL_INFO) * hcnt);	
	cwwidget->hcnt = hcnt;

	_get_holiday_info(cwwidget);

	return 0;
}

int cw_hcld_refresh_holiday(CWHCALENDAR *cwwidget, HOLIDAYCAL_INFO *hinfo, int hcnt)
{
	_generate_holiday_info(cwwidget, hinfo, hcnt);
	nfui_signal_emit((NFOBJECT*)cwwidget, GDK_EXPOSE, FALSE);
}

