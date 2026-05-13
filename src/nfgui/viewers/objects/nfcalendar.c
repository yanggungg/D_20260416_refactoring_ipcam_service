


/*****************************************************************************************
 *	NFCALENDAR
 * **************************************************************************************/

#include "../../support/nf_ui_font.h"
#include "../../support/color.h"
#include "../../support/util.h" 
#include "../../support/event_loop.h"


#include "nfcalendar.h"
#include "ix_mem.h"


#define	CALKEY_REPEAT_INTERVAL		(300)

#define	NFCAL_DEFAULT_SIZE_X		(guint)(DISPLAY_IS_D1 ? 134:268)
#define	NFCAL_DEFAULT_SIZE_Y		(guint)(DISPLAY_IS_D1 ? 148:296)

#define	NFCAL_BORDER				(guint)(DISPLAY_IS_D1 ? 2:4)
#define	NFCAL_INNER_SPACE			6

#define	TITLE_AREA_X				(guint)(NFCAL_BORDER)
#define	TITLE_AREA_Y				(guint)(NFCAL_BORDER)
#define	TITLE_AREA_WIDTH			(guint)(DISPLAY_IS_D1 ? 130:260)
#define	TITLE_AREA_HEIGHT			(guint)(DISPLAY_IS_D1 ? 22:44)

#define	BODY_AREA_X					(guint)(NFCAL_BORDER)
#define	BODY_AREA_Y					(guint)(DISPLAY_IS_D1 ? 24:48)				//(guint)(TITLE_AREA_Y + TITLE_AREA_HEIGHT)
#define	BODY_AREA_WIDTH				(guint)(DISPLAY_IS_D1 ? 130:260)
#define	BODY_AREA_HEIGHT			(guint)(DISPLAY_IS_D1 ? 122:244)

#define	DAY_AREA_X					(guint)(DISPLAY_IS_D1 ? 6:12)				// (guint)(NFCAL_BORDER + NFCAL_INNER_SPACE)
#define	DAY_AREA_Y					(guint)(DISPLAY_IS_D1 ? 38:76)
#define	DAY_AREA_WIDTH				(guint)(DISPLAY_IS_D1 ? 122:244)
#define	DAY_AREA_HEIGHT				(guint)(DISPLAY_IS_D1 ? 104:208)

#define	DAY_AREA_HMARGIN			(guint)(DISPLAY_IS_D1 ? 2:4)
#define	DAY_AREA_VMARGIN			(guint)(DISPLAY_IS_D1 ? 2:4)

#define	TITLE_YEAR_X				(guint)(DISPLAY_IS_D1 ? 30:60)
#define	TITLE_YEAR_Y				(guint)(DISPLAY_IS_D1 ? 2:4)
#define	TITLE_YEAR_WIDHT			(guint)(DISPLAY_IS_D1 ? 68:136)
#define	TITLE_YEAR_HEIGHT			(guint)(DISPLAY_IS_D1 ? 11:22)

#define	TITLE_MON_X					(guint)(DISPLAY_IS_D1 ? 30:60)
#define	TITLE_MON_Y					(guint)(DISPLAY_IS_D1 ? 13:26)
#define	TITLE_MON_WIDTH				(guint)(DISPLAY_IS_D1 ? 72:144)
#define	TITLE_MON_HEIGHT			(guint)(DISPLAY_IS_D1 ? 11:22)

#define	DAY_BTN_WIDTH				(guint)(DISPLAY_IS_D1 ? 16:32)
#define	DAY_BTN_HEIGHT				(guint)(DISPLAY_IS_D1 ? 16:32)

#define	DAY_BTN_COL_SPACE			(guint)(DISPLAY_IS_D1 ? 1:2)
#define	DAY_BTN_ROW_SPACE			(guint)(DISPLAY_IS_D1 ? 1:2)

#define	NFCAL_DEF_LOWER_TIMELIMIT	NF_LOWER_TIMELIMIT	
#define	NFCAL_DEF_UPPER_TIMELIMIT	NF_UPPER_TIMELIMIT	


#define MIN_YEAR 					(2008)
#define MIN_MONTH 					(1)
#define MIN_DAY 					(1)

#define MAX_YEAR  					(2036)
#define MAX_MONTH 					(1)
#define MAX_DAY   					(31)


static const gchar *strMonths[] = {
			"JANUARY",
			"FEBRUARY",
			"MARCH",
			"APRIL",
			"MAY",
			"JUNE",
			"JULY",
			"AUGUST",
			"SEPTEMBER",
			"OCTOBER",
			"NOVEMBER",
			"DECEMBER"
};



static void prvDrawBG(NFCALENDAR* nfcal, GdkGC *gc, GdkDrawable *drawable);
static guint prvGetFirstWeekday(GDate *date);
static void prvSetDaysPosition(NFCALENDAR *nfcal);
static void prvChangeDayState(NFCALENDAR *nfcal, guint day, gboolean focus);
static gint prvHasData(NFCALENDAR *nfcal, guint day);
static void prvFocusCurDate(NFCALENDAR *nfcal);
static void prvLeftButtonProc(NFCALENDAR *nfcal);
static void prvRightButtonProc(NFCALENDAR *nfcal);
static void prvUpButtonProc(NFCALENDAR *nfcal);
static void prvDownButtonProc(NFCALENDAR *nfcal);


static gboolean repeat_key_proc_short(gpointer data)
{
	NFCALENDAR *nfcal;

	nfcal = (NFCALENDAR*)data;

	if(((NFOBJECT*)nfcal)->kfocus == NFOBJECT_UNFOCUS)
	{	
		nfcal->krepeat_src = 0;
		nfui_user_signal_emit((NFOBJECT*)nfcal, NFEVENT_CALENDAR_CHANGED_RELEASE, FALSE);

		return FALSE;
	}

	NFUTIL_THREADS_ENTER();
	if(nfcal->rkey_id == KEYPAD_DOWN)		prvDownButtonProc(nfcal);
	else if(nfcal->rkey_id == KEYPAD_UP)	prvUpButtonProc(nfcal);
	else if(nfcal->rkey_id == KEYPAD_LEFT)	prvLeftButtonProc(nfcal);
	else if(nfcal->rkey_id == KEYPAD_RIGHT)	prvRightButtonProc(nfcal);
	else
	{
		NFUTIL_THREADS_LEAVE();
		return FALSE;
	}

	NFUTIL_THREADS_LEAVE();
	return TRUE;
}

static gboolean repeat_key_proc(gpointer data)
{
	NFCALENDAR *nfcal;

	nfcal = (NFCALENDAR*)data;

	if(((NFOBJECT*)nfcal)->kfocus == NFOBJECT_UNFOCUS)
		return FALSE;

	NFUTIL_THREADS_ENTER();
	if(nfcal->rkey_id == KEYPAD_DOWN)		prvDownButtonProc(nfcal);
	else if(nfcal->rkey_id == KEYPAD_UP)	prvUpButtonProc(nfcal);
	else if(nfcal->rkey_id == KEYPAD_LEFT)	prvLeftButtonProc(nfcal);
	else if(nfcal->rkey_id == KEYPAD_RIGHT)	prvRightButtonProc(nfcal);
	else
	{
		NFUTIL_THREADS_LEAVE();
		return FALSE;
	}

	NFUTIL_THREADS_LEAVE();

	nfcal->krepeat_src = g_timeout_add(CALKEY_REPEAT_INTERVAL, repeat_key_proc_short, nfcal);
	return FALSE;
}

static gint prvIsValidDate(GDate date)
{
	GTimeVal temp, cmpMinTemp, cmpMaxTemp;
	struct tm stTime, cmpMinTime, cmpMaxTime;

	memset(&temp, 0, sizeof(GTimeVal));
	memset(&stTime, 0, sizeof(struct tm));

	memset(&cmpMinTemp, 0, sizeof(GTimeVal));	
	memset(&cmpMinTime, 0, sizeof(struct tm));

	stTime.tm_year = g_date_get_year(&date) - 1900;
	stTime.tm_mon = g_date_get_month(&date) - 1;
	stTime.tm_mday = g_date_get_day(&date);

	temp.tv_sec = NFMKTIME(&stTime, 0);

	cmpMinTime.tm_year = MIN_YEAR-1900;
	cmpMinTime.tm_mon = MIN_MONTH-1;
	cmpMinTime.tm_mday = MIN_DAY;

	cmpMinTemp.tv_sec = NFMKTIME(&cmpMinTime, 0);
	//	if(temp.tv_sec < NFCAL_DEF_LOWER_TIMELIMIT)
	if(temp.tv_sec < cmpMinTemp.tv_sec)
		return -1;

	memset(&cmpMaxTime, 0, sizeof(struct tm));
	memset(&cmpMaxTemp, 0, sizeof(GTimeVal));

	cmpMaxTime.tm_year = MAX_YEAR-1900;
	cmpMaxTime.tm_mon = MAX_MONTH-1;
	cmpMaxTime.tm_mday = MAX_DAY;

	cmpMaxTemp.tv_sec = NFMKTIME(&cmpMaxTime, 0);

	//if(temp.tv_sec > NFCAL_DEF_UPPER_TIMELIMIT)
	if(temp.tv_sec > cmpMaxTemp.tv_sec)
		return 1;

	return 0;
}

static gint prvIsValidTV(GTimeVal tv)
{
	if(tv.tv_sec < NFCAL_DEF_LOWER_TIMELIMIT)
		return -1;

	if(tv.tv_sec > NFCAL_DEF_UPPER_TIMELIMIT)
		return 1;

	return 0;
}

static void prvDrawBorder(NFCALENDAR* nfcal)
{
	GdkDrawable* drawable;
	GdkGC *gc;

	guint left, top;
	gint x, y;
	nfui_nfobject_get_offset((NFOBJECT*)nfcal, &left, &top);

	drawable = nfui_nfobject_get_window((NFOBJECT*)nfcal);
	gc = nfui_nfobject_get_gc((NFOBJECT*)nfcal);

	if(nfcal->fixed.object.kfocus == NFOBJECT_FOCUS)
	{
		if(nfui_is_focus_at_child((NFOBJECT*)nfcal))
			gdk_gc_set_rgb_fg_color(gc, &colors[COLOR_IDX(NOT_CARE)]);
		else
			gdk_gc_set_rgb_fg_color(gc, &colors[COLOR_IDX(NOT_CARE)]);
	}
	else
	{
		gdk_gc_set_rgb_fg_color(gc, &colors[COLOR_IDX(NOT_CARE)]);
	}
#if 0
	gdk_draw_rectangle(drawable, gc, TRUE, (gint)left, (gint)top, (gint)(nfcal->fixed.object.width), (gint)(nfcal->fixed.object.height));
#else
	gdk_draw_rectangle(drawable, gc, TRUE, (gint)left, (gint)top, (gint)(nfcal->fixed.object.width), NFCAL_BORDER);
	gdk_draw_rectangle(drawable, gc, TRUE, (gint)left, (gint)top, NFCAL_BORDER, (gint)(nfcal->fixed.object.height));

	y = top + nfcal->fixed.object.height - NFCAL_BORDER;
	gdk_draw_rectangle(drawable, gc, TRUE, (gint)left, y, (gint)(nfcal->fixed.object.width), NFCAL_BORDER);

	x = left + nfcal->fixed.object.width - NFCAL_BORDER;
	gdk_draw_rectangle(drawable, gc, TRUE, x, (gint)top, NFCAL_BORDER, (gint)(nfcal->fixed.object.height));
#endif

}

static void prvDrawBG(NFCALENDAR* nfcal, GdkGC *gc, GdkDrawable *drawable)
{
	gchar *days[7] = {"S", "M", "T", "W", "T", "F", "S"};
	gchar strTemp[16];
	guint left, top;
	gint x, y, w, h;
	gint i;

	nfui_nfobject_get_offset((NFOBJECT*)nfcal, &left, &top);

	//draw border
	prvDrawBorder(nfcal);

	// draw Title area
	gdk_gc_set_rgb_fg_color(gc, &colors[COLOR_IDX(NOT_CARE)]);
	gdk_draw_rectangle(drawable, gc, TRUE, left+nfcal->rtTitle.x, top+nfcal->rtTitle.y, nfcal->rtTitle.width, nfcal->rtTitle.height);

	memset(strTemp, 0, sizeof(strTemp));
	g_sprintf(strTemp, "%d", g_date_get_year(nfcal->curDate));

	nfutil_draw_text_with_pango("CAL_YEAR", NULL, &colors[COLOR_IDX(NOT_CARE)], drawable, gc, strTemp, left+nfcal->rtYear.x, top+nfcal->rtYear.y, nfcal->rtYear.width, nfcal->rtYear.height, nfcal->pango_font[NFCAL_TITLE], &colors[COLOR_IDX(NOT_CARE)], NFALIGN_CENTER, 0);	

	nfutil_draw_text_with_pango_spacing("CAL_MON", NULL, &colors[COLOR_IDX(NOT_CARE)], drawable, gc, strMonths[g_date_get_month(nfcal->curDate)-1], left+nfcal->rtMonth.x, top+nfcal->rtMonth.y, nfcal->rtMonth.width, nfcal->rtMonth.height, nfcal->pango_font[NFCAL_TITLE], &colors[COLOR_IDX(NOT_CARE)], NFALIGN_CENTER, 0, CONDENSED_SPACING);	

#if 0
	nfutil_draw_text_with_pango("NUMBER", drawable, gc, strTemp, left+nfcal->rtYear.x, top+nfcal->rtYear.y, nfcal->rtYear.width, nfcal->rtYear.height, nfcal->pango_font[NFCAL_TITLE], &colors[COLOR_IDX(NOT_CARE)], NFALIGN_CENTER, 0);	

	nfutil_draw_text_with_pango("NUMBER", drawable, gc, strMonths[g_date_get_month(nfcal->curDate)-1], left+nfcal->rtMonth.x, top+nfcal->rtMonth.y, nfcal->rtMonth.width, nfcal->rtMonth.height, nfcal->pango_font[NFCAL_TITLE], &colors[COLOR_IDX(NOT_CARE)], NFALIGN_CENTER, 0);	
#endif

	// draw Body
	gdk_gc_set_rgb_fg_color(gc, &colors[COLOR_IDX(NOT_CARE)]);
	gdk_draw_rectangle(drawable, gc, TRUE, left+nfcal->rtBody.x, top+nfcal->rtBody.y, nfcal->rtBody.width, nfcal->rtBody.height);

	gdk_gc_set_rgb_fg_color(gc, &colors[COLOR_IDX(NOT_CARE)]);
	gdk_draw_rectangle(drawable, gc, TRUE, left+nfcal->rtDayArea.x, top+nfcal->rtDayArea.y, nfcal->rtDayArea.width, nfcal->rtDayArea.height);
#if defined(__SAMSUNG_UI__)
	gdk_gc_set_rgb_fg_color(gc, &colors[COLOR_IDX(NOT_CARE)]);
	gdk_draw_rectangle(drawable, gc, FALSE, left+nfcal->rtDayArea.x, top+nfcal->rtDayArea.y, nfcal->rtDayArea.width, nfcal->rtDayArea.height);
#endif

	x = left + nfcal->rtDayArea.x + (DISPLAY_IS_D1 ? 1:0);
	y = nfcal->rtDayArea.y - (DISPLAY_IS_D1 ? 13 : 16);
	w = (DISPLAY_IS_D1 ? 17 : 32);
	h = 12;

	for(i=0; i<7; i++) {
		if(i==0)
			nfutil_draw_text_with_pango("CAL_SUN", NULL, &colors[COLOR_IDX(NOT_CARE)], drawable, gc, days[i], x, top + y, w, h, nfcal->pango_font[2], &colors[COLOR_IDX(NOT_CARE)], NFALIGN_CENTER, 0);	
		else if(i == 6)
			nfutil_draw_text_with_pango("CAL_SAT", NULL, &colors[COLOR_IDX(NOT_CARE)],  drawable, gc, days[i], x, top + y, w, h, nfcal->pango_font[2], &colors[COLOR_IDX(NOT_CARE)], NFALIGN_CENTER, 0);	
		else
			nfutil_draw_text_with_pango("CAL_WEEKDAY", NULL, &colors[COLOR_IDX(NOT_CARE)],  drawable, gc, days[i], x, top + y, w, h, nfcal->pango_font[2], &colors[COLOR_IDX(NOT_CARE)], NFALIGN_CENTER, 0);	

		x += w;
	}

#if 0
	for(i=0; i<7; i++) {
		if(i==0)
			nfutil_draw_text_with_pango("NUMBER", drawable, gc, days[i], x, top + y, w, h, nfcal->pango_font[2], &colors[COLOR_IDX(NOT_CARE)], NFALIGN_CENTER, 0);	
		else if(i == 6)
			nfutil_draw_text_with_pango("NUMBER", drawable, gc, days[i], x, top + y, w, h, nfcal->pango_font[2], &colors[COLOR_IDX(NOT_CARE)], NFALIGN_CENTER, 0);	
		else
			nfutil_draw_text_with_pango("NUMBER", drawable, gc, days[i], x, top + y, w, h, nfcal->pango_font[2], &colors[COLOR_IDX(NOT_CARE)], NFALIGN_CENTER, 0);	

		x += w;
	}
#endif
}

static guint prvGetFirstWeekday(GDate *date)
{
	GDate *temp;
	guint wd;

	temp = g_date_new_dmy(1, g_date_get_month(date), g_date_get_year(date));

	wd = g_date_get_weekday(temp) % 7;

	g_date_free(temp);

	return wd;
}

static void prvSetDaysPosition(NFCALENDAR *nfcal)
{
	guint wd;
	guint x, y;
	guint i;
	guint days;

	wd = prvGetFirstWeekday(nfcal->curDate);

	x = nfcal->rtDayArea.x + nfcal->hmargin;
	y = nfcal->rtDayArea.y + nfcal->vmargin;

	days = g_date_get_days_in_month(g_date_get_month(nfcal->curDate), g_date_get_year(nfcal->curDate));

	for(i=0; i<31; i++)
	{
		nfui_nfobject_move(nfcal->day[i], x+(nfcal->day_width+nfcal->hspace)*(wd%7), y+(nfcal->day_height+nfcal->vspace)*(wd/7));

		wd++;

		if(i>=days)	nfui_nfobject_hide(nfcal->day[i]);
		else	nfui_nfobject_show(nfcal->day[i]);
	}
}

static void prvChangeDayState(NFCALENDAR *nfcal, guint day, gboolean focus)
{
	NFOBJECT *obj;
	nfcal_day_state status;

	obj = nfcal->day[day];

	if(nfcal->data_map[day])
	{
		if(focus)	status = NFCAL_DATA_FOCUS;
		else		status = NFCAL_DATA_NOFOCUS;
	}
	else
	{
		if(focus)	status = NFCAL_NODATA_FOCUS;
		else		status = NFCAL_NODATA_NOFOCUS;
	}

	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, nfcal->bg_color[status]);
	nfui_nflabel_modify_fg(obj, nfcal->fg_color[status]);
}

static gint prvHasData(NFCALENDAR *nfcal, guint day)
{
	if(day<0 || day>30)
		return -1;

	return nfcal->data_map[day];
}

static void prvFocusCurDate(NFCALENDAR *nfcal)
{
	prvChangeDayState(nfcal, g_date_get_day(nfcal->curDate)-1, TRUE);
}

static void prvLeftButtonProc(NFCALENDAR *nfcal)
{
	gint cur_day = 0;
	gint new_day = 0;
	gint sub = 0;

	gint prev_mon = 0;
	gint new_mon = 0;

	gint cmp_date = 0;
	GTimeVal tv_temp;

	gint send_event = 0;

	prev_mon = g_date_get_month(nfcal->curDate);
	cur_day = g_date_get_day(nfcal->curDate)-1;

	g_date_subtract_days(nfcal->curDate, 1);
	new_day = g_date_get_day(nfcal->curDate)-1;
	sub = cur_day - new_day;

	new_mon = g_date_get_month(nfcal->curDate);

	prvChangeDayState(nfcal, cur_day, FALSE);
	prvChangeDayState(nfcal, new_day, TRUE);


	cmp_date = prvIsValidDate(*(nfcal->curDate));

	memset(&tv_temp, 0, sizeof(GTimeVal));
	if(cmp_date < 0)
	{
		tv_temp.tv_sec = NFCAL_DEF_UPPER_TIMELIMIT;
		g_date_set_time_val(nfcal->curDate, &tv_temp);
		send_event = 1;
	}
	else if(cmp_date > 0)
	{
		tv_temp.tv_sec = NFCAL_DEF_LOWER_TIMELIMIT;
		g_date_set_time_val(nfcal->curDate, &tv_temp);
		send_event = 1;
	}
	
	if(prev_mon != new_mon || send_event)
		nfui_user_signal_emit((NFOBJECT*)nfcal, NFEVENT_CALENDAR_MONTH_CHANGED, FALSE);

	if(sub>=0 && !send_event)
	{
		nfui_signal_emit((NFOBJECT*)nfcal->day[cur_day], GDK_EXPOSE, TRUE);
		nfui_signal_emit((NFOBJECT*)nfcal->day[new_day], GDK_EXPOSE, TRUE);
	}
	else
	{
		prvSetDaysPosition(nfcal);
		nfui_signal_emit((NFOBJECT*)nfcal, GDK_EXPOSE, TRUE);
	}
}

static void prvRightButtonProc(NFCALENDAR *nfcal)
{
	gint cur_day = 0;
	gint new_day = 0;
	gint sub = 0;

	gint prev_mon = 0;
	gint new_mon = 0;

	gint cmp_date = 0;
	GTimeVal tv_temp;

	gint send_event = 0;

	prev_mon = g_date_get_month(nfcal->curDate);
	cur_day = g_date_get_day(nfcal->curDate)-1;

	g_date_add_days(nfcal->curDate, 1);
	new_day = g_date_get_day(nfcal->curDate)-1;
	sub = new_day - cur_day;

	new_mon = g_date_get_month(nfcal->curDate);

	prvChangeDayState(nfcal, cur_day, FALSE);
	prvChangeDayState(nfcal, new_day, TRUE);

	cmp_date = prvIsValidDate(*(nfcal->curDate));

	memset(&tv_temp, 0, sizeof(GTimeVal));
	if(cmp_date < 0)
	{
		tv_temp.tv_sec = NFCAL_DEF_UPPER_TIMELIMIT;
		g_date_set_time_val(nfcal->curDate, &tv_temp);
		send_event = 1;
	}
	else if(cmp_date > 0)
	{
		tv_temp.tv_sec = NFCAL_DEF_LOWER_TIMELIMIT;
		g_date_set_time_val(nfcal->curDate, &tv_temp);
		send_event = 1;
	}


	if(prev_mon != new_mon || send_event)
		nfui_user_signal_emit((NFOBJECT*)nfcal, NFEVENT_CALENDAR_MONTH_CHANGED, FALSE);

	if(sub>=0 && !send_event)
	{
		nfui_signal_emit((NFOBJECT*)nfcal->day[cur_day], GDK_EXPOSE, TRUE);
		nfui_signal_emit((NFOBJECT*)nfcal->day[new_day], GDK_EXPOSE, TRUE);
	}
	else
	{
		prvSetDaysPosition(nfcal);
		nfui_signal_emit((NFOBJECT*)nfcal, GDK_EXPOSE, TRUE);
	}
}

static void prvUpButtonProc(NFCALENDAR *nfcal)
{
	gint cur_day = 0;
	gint new_day = 0;
	gint sub = 0;

	gint prev_mon = 0;
	gint new_mon = 0;

	gint cmp_date = 0;
	GTimeVal tv_temp;

	gint send_event = 0;

	prev_mon = g_date_get_month(nfcal->curDate);
	cur_day = g_date_get_day(nfcal->curDate)-1;

	g_date_subtract_days(nfcal->curDate, 7);
	new_day = g_date_get_day(nfcal->curDate)-1;
	sub = cur_day - new_day;

	new_mon = g_date_get_month(nfcal->curDate);

	prvChangeDayState(nfcal, cur_day, FALSE);
	prvChangeDayState(nfcal, new_day, TRUE);


	cmp_date = prvIsValidDate(*(nfcal->curDate));

	memset(&tv_temp, 0, sizeof(GTimeVal));
	if(cmp_date < 0)
	{
		tv_temp.tv_sec = NFCAL_DEF_UPPER_TIMELIMIT;
		g_date_set_time_val(nfcal->curDate, &tv_temp);
		send_event = 1;
	}
	else if(cmp_date > 0)
	{
		tv_temp.tv_sec = NFCAL_DEF_LOWER_TIMELIMIT;
		g_date_set_time_val(nfcal->curDate, &tv_temp);
		send_event = 1;
	}

	if(prev_mon != new_mon || send_event)
		nfui_user_signal_emit((NFOBJECT*)nfcal, NFEVENT_CALENDAR_MONTH_CHANGED, FALSE);

	if(sub>=0 && !send_event)
	{
		nfui_signal_emit((NFOBJECT*)nfcal->day[cur_day], GDK_EXPOSE, TRUE);
		nfui_signal_emit((NFOBJECT*)nfcal->day[new_day], GDK_EXPOSE, TRUE);
	}
	else
	{
		prvSetDaysPosition(nfcal);
		nfui_signal_emit((NFOBJECT*)nfcal, GDK_EXPOSE, TRUE);
	}
}
				
static void prvDownButtonProc(NFCALENDAR *nfcal)
{
	gint cur_day = 0;
	gint new_day = 0;
	gint sub = 0;

	gint prev_mon = 0;
	gint new_mon = 0;

	gint cmp_date = 0;
	GTimeVal tv_temp;

	gint send_event = 0;

	prev_mon = g_date_get_month(nfcal->curDate);
	cur_day = g_date_get_day(nfcal->curDate)-1;

	g_date_add_days(nfcal->curDate, 7);
	new_day = g_date_get_day(nfcal->curDate)-1;
	sub = new_day - cur_day;

	new_mon = g_date_get_month(nfcal->curDate);

	prvChangeDayState(nfcal, cur_day, FALSE);
	prvChangeDayState(nfcal, new_day, TRUE);

	cmp_date = prvIsValidDate(*(nfcal->curDate));

	memset(&tv_temp, 0, sizeof(GTimeVal));
	if(cmp_date < 0)
	{
		tv_temp.tv_sec = NFCAL_DEF_UPPER_TIMELIMIT;
		g_date_set_time_val(nfcal->curDate, &tv_temp);
		send_event = 1;
	}
	else if(cmp_date > 0)
	{
		tv_temp.tv_sec = NFCAL_DEF_LOWER_TIMELIMIT;
		g_date_set_time_val(nfcal->curDate, &tv_temp);
		send_event = 1;
	}

	if(prev_mon != new_mon || send_event)
		nfui_user_signal_emit((NFOBJECT*)nfcal, NFEVENT_CALENDAR_MONTH_CHANGED, FALSE);

	if(sub>=0 && !send_event)
	{
		nfui_signal_emit((NFOBJECT*)nfcal->day[cur_day], GDK_EXPOSE, TRUE);
		nfui_signal_emit((NFOBJECT*)nfcal->day[new_day], GDK_EXPOSE, TRUE);
	}
	else
	{
		prvSetDaysPosition(nfcal);
		nfui_signal_emit((NFOBJECT*)nfcal, GDK_EXPOSE, TRUE);
	}
}

static gboolean nfcalendar_event_handler(NFCALENDAR *nfcal, GdkEvent *event, gpointer data)
{
	switch(event->type)
	{
		case GDK_EXPOSE :
		{
			GdkDrawable *drawable;
			GdkGC *gc;

			drawable = nfui_nfobject_get_window((NFOBJECT*)nfcal);
			gc = nfui_nfobject_get_gc((NFOBJECT*)nfcal);
			prvDrawBG(nfcal, gc, drawable);
			prvFocusCurDate(nfcal);
			nfui_nfobject_gc_unref(gc);
		}
		break;

		case GDK_DELETE :
		{
			if(nfcal)
			{
				if(nfcal->krepeat_src)
				{
					g_source_remove(nfcal->krepeat_src);
					nfcal->krepeat_src = 0;
				}

				if(nfcal->curDate)	g_date_free(nfcal->curDate);

				nfcal->curDate = NULL;

			}
			nfcal = NULL;
		}
		break;

		case GDK_ENTER_NOTIFY:
			if(nfui_nfobject_is_shown((NFOBJECT*)nfcal))
				prvDrawBorder(nfcal);
//				nfui_signal_emit((NFOBJECT*)nfcal, GDK_EXPOSE, TRUE);
			break;

		case GDK_LEAVE_NOTIFY:
			if(nfui_nfobject_is_shown((NFOBJECT*)nfcal))
				prvDrawBorder(nfcal);
		case GDK_BUTTON_RELEASE:
		case NFEVENT_KEYPAD_RELEASE:
		case NFEVENT_REMOCON_RELEASE:
			if(nfcal->krepeat_src)
			{
				g_source_remove(nfcal->krepeat_src);
				nfcal->krepeat_src = 0;
			
				nfui_user_signal_emit((NFOBJECT*)nfcal, NFEVENT_CALENDAR_CHANGED_RELEASE, FALSE);
			}
			break;



		default :
			break;
	}

	return FALSE;
}

static gboolean post_datebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFCALENDAR *nfcal;
	gint cmp_date = 0;
	GTimeVal tv_temp;

	nfcal = obj->parent;
	if(evt->type == GDK_BUTTON_PRESS)
	{	
		if(nfcal->krepeat_src)
		{
			g_source_remove(nfcal->krepeat_src);
			nfcal->krepeat_src = 0;
		}

  	   	if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
		   return FALSE;
  	   	}

		prvChangeDayState(nfcal, g_date_get_day(nfcal->curDate)-1, FALSE);

		if(g_slist_nth_data(((NFFIXED*)nfcal)->children, 0) == obj)
		{
			g_date_subtract_years(nfcal->curDate, 1);
		}
		else if(g_slist_nth_data(((NFFIXED*)nfcal)->children, 1) == obj)
		{
			g_date_add_years(nfcal->curDate, 1);
		}
		else if(g_slist_nth_data(((NFFIXED*)nfcal)->children, 2) == obj)
		{
			g_date_subtract_months(nfcal->curDate, 1);
		}
		else if(g_slist_nth_data(((NFFIXED*)nfcal)->children, 3) == obj)
		{
			g_date_add_months(nfcal->curDate, 1);
		}

		cmp_date = prvIsValidDate(*(nfcal->curDate));

		memset(&tv_temp, 0, sizeof(GTimeVal));
		if(cmp_date < 0)
		{
			tv_temp.tv_sec = NFCAL_DEF_UPPER_TIMELIMIT;
			g_date_set_time_val(nfcal->curDate, &tv_temp);
		}
		else if(cmp_date > 0)
		{
			tv_temp.tv_sec = NFCAL_DEF_LOWER_TIMELIMIT;
			g_date_set_time_val(nfcal->curDate, &tv_temp);
		}

		prvSetDaysPosition(nfcal);
		nfui_user_signal_emit((NFOBJECT*)nfcal, NFEVENT_CALENDAR_MONTH_CHANGED, FALSE);
		nfui_user_signal_emit((NFOBJECT*)nfcal, NFEVENT_CALENDAR_CHANGED, FALSE);
//		nfui_signal_emit((NFOBJECT*)nfcal, GDK_EXPOSE, TRUE);
	}
	else if(evt->type == GDK_BUTTON_RELEASE)
	{	
		if(nfcal->krepeat_src)
		{
			g_source_remove(nfcal->krepeat_src);
			nfcal->krepeat_src = 0;

			nfui_user_signal_emit((NFOBJECT*)nfcal, NFEVENT_CALENDAR_CHANGED_RELEASE, FALSE);
		}
	}

	return FALSE;
}

static gboolean post_daylabel_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFCALENDAR *nfcal;

	nfcal = obj->parent;

	if(evt->type == GDK_BUTTON_PRESS)
	{
		gint new_day = 0;
		gint old_day = 0;

		if(nfcal->krepeat_src)
		{
			g_source_remove(nfcal->krepeat_src);
			nfcal->krepeat_src = 0;
		}


		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
		   return FALSE;
  	   	}



		for(new_day=0; new_day<31; new_day++)
		{
			if(obj == nfcal->day[new_day])
				break;
		}

		if(new_day==31 || new_day==g_date_get_day(nfcal->curDate)-1)
			return FALSE;

		old_day = g_date_get_day(nfcal->curDate)-1;
		
		prvChangeDayState(nfcal, old_day, FALSE);
		g_date_set_day(nfcal->curDate, new_day+1);
		prvChangeDayState(nfcal, new_day, TRUE);

		nfui_user_signal_emit((NFOBJECT*)nfcal, NFEVENT_CALENDAR_CHANGED, FALSE);
		nfui_signal_emit((NFOBJECT*)nfcal->day[old_day], GDK_EXPOSE, TRUE);
		nfui_signal_emit((NFOBJECT*)nfcal->day[new_day], GDK_EXPOSE, TRUE);
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		if(nfcal->key_lock)
			return TRUE;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		switch(kpid)
		{
			case KEYPAD_LEFT:
				prvLeftButtonProc(nfcal);
				if(!nfcal->krepeat_src)
				{
					nfcal->rkey_id = KEYPAD_LEFT;
					nfcal->krepeat_src = g_timeout_add(CALKEY_REPEAT_INTERVAL * 3, repeat_key_proc, nfcal);
				}
				break;

			case KEYPAD_RIGHT:
				prvRightButtonProc(nfcal);
				if(!nfcal->krepeat_src)
				{
					nfcal->rkey_id = KEYPAD_RIGHT;
					nfcal->krepeat_src = g_timeout_add(CALKEY_REPEAT_INTERVAL * 3, repeat_key_proc, nfcal);
				}
				break;

			case KEYPAD_UP:
				prvUpButtonProc(nfcal);
				if(!nfcal->krepeat_src)
				{
					nfcal->rkey_id = KEYPAD_UP;
					nfcal->krepeat_src = g_timeout_add(CALKEY_REPEAT_INTERVAL * 3, repeat_key_proc, nfcal);
				}

				break;

			case KEYPAD_DOWN:
				prvDownButtonProc(nfcal);
				if(!nfcal->krepeat_src)
				{
					nfcal->rkey_id = KEYPAD_DOWN;
					nfcal->krepeat_src = g_timeout_add(CALKEY_REPEAT_INTERVAL * 3, repeat_key_proc, nfcal);
				}
				break;

			case KEYPAD_ENTER:
				nfui_set_key_focus((NFOBJECT*)nfcal, FALSE);
				nfui_set_key_focus((NFOBJECT*)nfcal, TRUE);

				nfui_signal_emit((NFOBJECT*)nfcal, GDK_EXPOSE, TRUE);
				break;

			default:
				return FALSE;
		}

		return TRUE;
	}
	else if(evt->type == NFEVENT_KEYPAD_RELEASE || evt->type == NFEVENT_REMOCON_RELEASE)
	{
		if(nfcal->krepeat_src)
		{
			g_source_remove(nfcal->krepeat_src);
			nfcal->krepeat_src = 0;	
			
			nfui_user_signal_emit((NFOBJECT*)nfcal, NFEVENT_CALENDAR_CHANGED_RELEASE, FALSE);
		}

	}

	return FALSE;
}

NFCALENDAR* nfui_nfcalendar_new(GdkPixbuf *pixbuf1[NFOBJECT_STATE_COUNT], GdkPixbuf *pixbuf2[NFOBJECT_STATE_COUNT])
{
	NFCALENDAR *nfcal;
	NFOBJECT *btn_temp;

	GTimeVal curTime;
	struct tm stTime;
	guint btnx[2], btny[2];
	gint btn_w, btn_h;
	gchar strTemp[16];
	guint i;

	if(DISPLAY_IS_D1)
	{
		 btnx[0] = 20;		btnx[1] = 100;
		 btny[0] = 4;		btny[1] = 15;
	}
	else
	{
		 btnx[0] = 40;		btnx[1] = 200;
		 btny[0] = 8;		btny[1] = 28;
	}

	nfcal = (NFCALENDAR*)imalloc(sizeof(NFCALENDAR));
	if(nfcal == NULL)	return NULL;

#if 0
	nfcal->fixed.object.parent = NULL;
	nfcal->fixed.object.x = 0;
	nfcal->fixed.object.y = 0;
	nfcal->fixed.object.width = NFCAL_DEFAULT_SIZE_X;
	nfcal->fixed.object.height = NFCAL_DEFAULT_SIZE_Y;
	nfcal->fixed.object.type = NFOBJECT_TYPE_NFCALENDAR;
	nfcal->fixed.object.show = NFOBJECT_HIDE;
	nfcal->fixed.object.status = NFOBJECT_STATE_NORMAL;
	nfcal->fixed.object.use_focus = NFOBJECT_FOCUS_ON;
	nfcal->fixed.object.kfocus = NFOBJECT_UNFOCUS;
	nfcal->fixed.object.mfocus = NFOBJECT_UNFOCUS;
	nfcal->fixed.object.bg_color[NFOBJECT_STATE_NORMAL] = -1;
	nfcal->fixed.object.bg_color[NFOBJECT_STATE_PRELIGHT] = -1;
	nfcal->fixed.object.bg_color[NFOBJECT_STATE_ACTIVE] = -1;
	nfcal->fixed.object.bg_color[NFOBJECT_STATE_DISABLE] = NFUI_DISABLED_COLOR;
	nfcal->fixed.object.pre_event_handler = NULL;
	nfcal->fixed.object.default_event_handler = nfcalendar_event_handler;
	nfcal->fixed.object.post_event_handler = NULL;
#else

	nfui_nfobject_init((NFOBJECT*)nfcal);

	nfcal->fixed.object.width = NFCAL_DEFAULT_SIZE_X;
	nfcal->fixed.object.height = NFCAL_DEFAULT_SIZE_Y;
	nfcal->fixed.object.type = NFOBJECT_TYPE_NFCALENDAR;
	nfcal->fixed.object.use_focus = NFOBJECT_FOCUS_ON;
	nfcal->fixed.object.default_event_handler = nfcalendar_event_handler;
#endif

	nfcal->use_cashing = 1;

	/* Set default value to Private Data */
	nfcal->curDate = g_date_new();
	g_get_current_time(&curTime);
	stTime = *NFLOCALTIME(&(curTime.tv_sec));
	g_date_set_dmy(nfcal->curDate, stTime.tm_mday, stTime.tm_mon+1, stTime.tm_year+1900);

	nfutil_nfrect_set(&nfcal->rtTitle, TITLE_AREA_X, TITLE_AREA_Y, TITLE_AREA_WIDTH, TITLE_AREA_HEIGHT);
	nfutil_nfrect_set(&nfcal->rtBody, BODY_AREA_X, BODY_AREA_Y, BODY_AREA_WIDTH, BODY_AREA_HEIGHT);	
	nfutil_nfrect_set(&nfcal->rtDayArea, DAY_AREA_X, DAY_AREA_Y, DAY_AREA_WIDTH, DAY_AREA_HEIGHT);

	nfutil_nfrect_set(&nfcal->rtYear, TITLE_YEAR_X, TITLE_YEAR_Y, TITLE_YEAR_WIDHT, TITLE_YEAR_HEIGHT);
	nfutil_nfrect_set(&nfcal->rtMonth, TITLE_MON_X, TITLE_MON_Y, TITLE_MON_WIDTH, TITLE_MON_HEIGHT);

	nfcal->day_width = DAY_BTN_WIDTH;
	nfcal->day_height = DAY_BTN_HEIGHT;
	nfcal->hmargin = DAY_AREA_HMARGIN;
	nfcal->vmargin = DAY_AREA_VMARGIN;
	nfcal->hspace = DAY_BTN_COL_SPACE;
	nfcal->vspace = DAY_BTN_ROW_SPACE;

//	g_sprintf(nfcal->pango_font[0], nffont_get_pango_font(NFFONT_SMALL_SEMI));
//	g_sprintf(nfcal->pango_font[1], nffont_get_pango_font(NFFONT_SMALL_SEMI));
//	g_sprintf(nfcal->pango_font[2], nffont_get_pango_font(NFFONT_SMALL_SEMI));

	if (DISPLAY_IS_D1) {
		g_sprintf(nfcal->font_name[0], "Arial bold 11");
		g_sprintf(nfcal->font_name[1], "Arial bold 11");
		g_sprintf(nfcal->font_name[2], "Arial bold 11");
	}
	else {
		g_sprintf(nfcal->font_name[0], "Arial bold 17");
		g_sprintf(nfcal->font_name[1], "Arial bold 17");
		g_sprintf(nfcal->font_name[2], "Arial bold 16");
	}
	nfcal->pango_font[0] = nfcal->font_name[0];
	nfcal->pango_font[1] = nfcal->font_name[1];
	nfcal->pango_font[2] = nfcal->font_name[2];

	nfcal->bg_color[NFCAL_NODATA_NOFOCUS]	= COLOR_IDX(NOT_CARE);
	nfcal->bg_color[NFCAL_NODATA_FOCUS]		= COLOR_IDX(NOT_CARE);
	nfcal->bg_color[NFCAL_DATA_NOFOCUS]		= COLOR_IDX(NOT_CARE);
	nfcal->bg_color[NFCAL_DATA_FOCUS]		= COLOR_IDX(NOT_CARE);

	nfcal->fg_color[NFCAL_NODATA_NOFOCUS]	= COLOR_IDX(NOT_CARE);
	nfcal->fg_color[NFCAL_NODATA_FOCUS]		= COLOR_IDX(NOT_CARE);
	nfcal->fg_color[NFCAL_DATA_NOFOCUS]		= COLOR_IDX(NOT_CARE);
	nfcal->fg_color[NFCAL_DATA_FOCUS]		= COLOR_IDX(NOT_CARE);

	nfui_get_pixbuf_size(pixbuf1[0], &btn_w, &btn_h);

	for(i=0; i<4; i++)
	{
		btn_temp = nfui_nfbutton_new();
		if(i%2)		nfui_nfbutton_set_image(btn_temp, pixbuf2);
		else		nfui_nfbutton_set_image(btn_temp, pixbuf1);
		nfui_nfobject_set_size(btn_temp, btn_w, btn_h);
		nfui_nfobject_use_focus(btn_temp, NFOBJECT_FOCUS_OFF);
		nfui_nffixed_put((NFFIXED*)nfcal, btn_temp, btnx[i%2], btny[i/2]);
		nfui_nfobject_show(btn_temp);

		nfui_regi_post_event_callback(btn_temp, post_datebutton_event_handler);
	}

	for(i=0; i<31; i++)
	{
		memset(strTemp, 0, sizeof(strTemp));
		g_sprintf(strTemp, "%d", i+1);

		nfcal->day[i] = nfui_nflabel_new_with_pango_font(strTemp, nfcal->pango_font[NFCAL_DAY], nfcal->fg_color[NFCAL_NODATA_NOFOCUS]);
		nfui_nflabel_set_spacing(nfcal->day[i], SEMI_CONDENSED_SPACING);
		nfui_nfobject_set_size(nfcal->day[i], nfcal->day_width, nfcal->day_height);
		nfui_nflabel_set_drawing_outline((NFLABEL*)nfcal->day[i], FALSE);
		nfui_nfobject_modify_bg(nfcal->day[i], NFOBJECT_STATE_NORMAL, nfcal->bg_color[NFCAL_NODATA_NOFOCUS]);
		nfui_nffixed_put((NFFIXED*)nfcal, nfcal->day[i], nfcal->rtDayArea.x+nfcal->hmargin + (nfcal->hspace+nfcal->day_width)*(i%7), nfcal->rtDayArea.y+nfcal->vmargin + (nfcal->vspace+nfcal->day_height)*(i/7));
		nfui_nfobject_show(nfcal->day[i]);

		nfui_regi_post_event_callback(nfcal->day[i], post_daylabel_event_handler);
	}

	prvSetDaysPosition(nfcal);

	return nfcal;
}

void nfui_nfcalendar_set_state(NFCALENDAR *nfcal, guint day, guint data)
{
	if(nfcal->fixed.object.type != NFOBJECT_TYPE_NFCALENDAR)
	{
		return;
	}

	nfcal->data_map[day] = data;

	prvChangeDayState(nfcal, day, FALSE);
}

void nfui_nfcalendar_set_title_pango_font(NFCALENDAR *nfcal, const gchar *title_pfont)
{
	if(nfcal->fixed.object.type != NFOBJECT_TYPE_NFCALENDAR)
	{
		return;
	}

	if (title_pfont) {
		if (nffont_is_system_font(title_pfont))
			nfcal->pango_font[NFCAL_TITLE] = title_pfont;
		else {
			strncpy(nfcal->font_name[NFCAL_TITLE], title_pfont, NFCAL_FONT_STRING_SIZE);
			nfcal->pango_font[NFCAL_TITLE] = nfcal->font_name[NFCAL_TITLE];

		}
	}
//	strncpy(nfcal->pango_font[NFCAL_TITLE], title_pfont, sizeof(nfcal->pango_font));
}

void nfui_nfcalendar_set_day_pango_font(NFCALENDAR *nfcal, const gchar *day_pfont)
{
	if(nfcal->fixed.object.type != NFOBJECT_TYPE_NFCALENDAR)
	{
		return;
	}

	if (day_pfont) {
		if (nffont_is_system_font(day_pfont))
			nfcal->pango_font[NFCAL_DAY] = day_pfont;
		else {
			strncpy(nfcal->font_name[NFCAL_DAY], day_pfont, NFCAL_FONT_STRING_SIZE);
			nfcal->pango_font[NFCAL_DAY] = nfcal->font_name[NFCAL_DAY];

		}
	}
//	strncpy(nfcal->pango_font[NFCAL_DAY], day_pfont, sizeof(nfcal->pango_font));
}

void nfui_nfcalendar_use_pango_cashing(NFCALENDAR *nfcal, gint cashing, gchar *key)
{
	gint i;

	if(nfcal->fixed.object.type != NFOBJECT_TYPE_NFCALENDAR)
	{
		return;
	}

	if(cashing)	nfcal->use_cashing = 1;
	else		nfcal->use_cashing = 0;

	if(key && strlen(key)>0)
		strncpy(nfcal->cashing_key, key, sizeof(nfcal->cashing_key));
	else
		memset(nfcal->cashing_key, 0, sizeof(nfcal->cashing_key));

}

void nfui_nfcalendar_set_date(NFCALENDAR *nfcal, GDate *date)
{
	if(nfcal->fixed.object.type != NFOBJECT_TYPE_NFCALENDAR)
	{
		return;
	}

	if(prvIsValidDate(*date) != 0)
	{
		return;
	}

	g_memmove(nfcal->curDate, date, sizeof(GDate));

	prvSetDaysPosition(nfcal);
}

void nfui_nfcalendar_get_date(NFCALENDAR *nfcal, GDate *date)
{
	if(nfcal->fixed.object.type != NFOBJECT_TYPE_NFCALENDAR)
	{
		return;
	}

	g_memmove(date, nfcal->curDate, sizeof(GDate));
}

#if 0
void nfui_nfcalendar_set_date_from_tv(NFCALENDAR *nfcal, GTimeVal *tv)
{
	if(nfcal->fixed.object.type != NFOBJECT_TYPE_NFCALENDAR)
	{
		P_TYPE_DISMATCH_WARNING(nfcal);
		return;
	}

	g_date_set_time_val(nfcal->curDate, tv);

	prvSetDaysPosition(nfcal);
}

void nfui_nfcalendar_get_date_to_tv(NFCALENDAR *nfcal, GTimeVal *tv)
{
	if(nfcal->fixed.object.type != NFOBJECT_TYPE_NFCALENDAR)
	{
		P_TYPE_DISMATCH_WARNING(nfcal);
		return;
	}

	time_t ttime;
	struct tm st_time;

	memset(&st_time, 0, sizeof(struct tm));
	st_time.tm_year = g_date_get_year(nfcal->curDate)-1900;
	st_time.tm_mon = g_date_get_month(nfcal->curDate)-1;
	st_time.tm_mday = g_date_get_day(nfcal->curDate);

	ttime = mktime(&st_time);

	tv->tv_sec = ttime;
	tv->tv_usec = 0;
}
#endif

void nfui_nfcalendar_set_date_ymd(NFCALENDAR *nfcal, guint year, guint mon, guint day)
{
	if(nfcal->fixed.object.type != NFOBJECT_TYPE_NFCALENDAR)
	{
		return;
	}

	g_date_set_dmy(nfcal->curDate, day, mon, year);

	prvSetDaysPosition(nfcal);
}

void nfui_nfcalendar_get_date_ymd(NFCALENDAR *nfcal, guint *year, guint *mon, guint *day)
{
	if(nfcal->fixed.object.type != NFOBJECT_TYPE_NFCALENDAR)
	{
		return;
	}

	*year = g_date_get_year(nfcal->curDate);
	*mon = g_date_get_month(nfcal->curDate);
	*day = g_date_get_day(nfcal->curDate);
}


void nfui_nfcalendar_set_keylock(NFCALENDAR *nfcal, gint lock)
{
	if(nfcal->fixed.object.type != NFOBJECT_TYPE_NFCALENDAR)
	{
		return;
	}

	nfcal->key_lock = lock;
}

#if 0
void nfui_nfcalendar_set_size(NFCALENDAR *nfcal, guint width, guint height)
{
	gint title_w, title_h;
	gint body_w, body_h;
	gint dayarea_x, dayarea_y;
	gint dayarea_w, dayarea_h;
	gint year_x, year_y;
	gint year_w, year_h;
	gint month_x, month_y;
	gint month_w, month_h;
	
	gint temp;

	if(nfcal->fixed.object.type != NFOBJECT_TYPE_NFCALENDAR)
	{
		P_TYPE_DISMATCH_WARNING(nfcal);
		return;
	}

	nfcal->fixed.object.width = width;
	nfcal->fixed.object.height = height;

	title_w = width - (NFCAL_BORDER * 2);
	title_h = (gint)(height / 5);

	body_w = title_w;
	body_h = height - title_h - (NFCAL_BORDER * 2);

	dayarea_x = NFCAL_BORDER + NFCAL_INNER_SPACE;
	dayarea_y = NFCAL_BORDER + NFCAL_INNER_SPACE + title_h;
	dayarea_w = width - ((NFCAL_BORDER + NFCAL_INNER_SPACE) * 2);
	dayarea_h = body_h - (NFCAL_INNER_SPACE * 2);

	temp = title_h / 4;;

	year_x = NFCAL_BORDER + (title_w / 4);
	year_y = NFCAL_BORDER + (temp / 2);
	year_w = title_w/2;
	year_h = title_h - temp;





	nfutil_nfrect_set(&nfcal->rtTitle,		NFCAL_BORDER,		NFCAL_BORDER,			title_w,		title_h);		//2, 2, 244, 62);
	nfutil_nfrect_set(&nfcal->rtBody,		NFCAL_BORDER,		NFCAL_BORDER+title_h,	body_w,			body_h);		//2, 64, 244, 186);
	nfutil_nfrect_set(&nfcal->rtDayArea,	dayarea_x,			dayarea_y,				dayarea_w,		dayarea_h);		// 8, 70, 232, 174);
	nfutil_nfrect_set(&nfcal->rtYear, 		year_x,				year_y,					year_w,			year_h);		//64, 8, 120, 24);
//	nfutil_nfrect_set(&nfcal->rtMonth, 64, 34, 120, 24);

//	nfcal->day_width = 28;
//	nfcal->day_height = 26;
//	nfcal->hmargin = 2;
//	nfcal->vmargin = 3;
//	nfcal->hspace = 4;
//	nfcal->vspace = 2;

}
#endif


