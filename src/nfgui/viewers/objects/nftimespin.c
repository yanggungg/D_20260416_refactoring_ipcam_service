


#include "nftimespin.h"
#include "nffixed.h"
#include "nfbutton.h"
#include "nflabel.h"

#include "../../support/event_loop.h"
#include "../../support/util.h"
#include "../../support/color.h"

#include "ix_mem.h"


#define	TSPINKEY_REPEAT_INTERVAL		(300)

#define	NFTS_DEF_LOWER_TIMELIMIT	NF_LOWER_TIMELIMIT	
#define	NFTS_DEF_UPPER_TIMELIMIT	NF_UPPER_TIMELIMIT

#if defined(__SAMSUNG_UI__)
#define NFTS_LINE_BORDER			1
#else
#define NFTS_LINE_BORDER			2
#endif

#define	IS_VALID_NFTS_FIELD(a)		((a>=NFTS_YEAR && a<NFTS_FIELDS) ? 1:0)


#define	DAY_TO_SEC		((time_t)86400)
#define	HOUR_TO_SEC		((time_t)3600)
#define	MIN_TO_SEC		((time_t)60)

static void prvChangeFocus(NFTIMESPIN *nfts, nfts_field_type new_focus);
static void prvUpButtonProc(NFTIMESPIN* nfts);
static void prvDownButtonProc(NFTIMESPIN* nfts);
static void prvLeftButtonProc(NFTIMESPIN* nfts);
static void prvRightButtonProc(NFTIMESPIN* nfts);

static gboolean repeat_key_proc_short(gpointer data)
{
	NFTIMESPIN *nfts;

	nfts = (NFTIMESPIN*)data;

	if(((NFOBJECT*)nfts)->kfocus == NFOBJECT_UNFOCUS)
	{
		nfts->krepeat_src = 0;
		nfts->mrepeat_src = 0;
	
		if(nfts->rkey_id == KEYPAD_UP || nfts->rkey_id == KEYPAD_DOWN)
		{
			nfui_user_signal_emit((NFOBJECT*)nfts, NFEVENT_TIMESPIN_CHANGED, FALSE);
			nfui_signal_emit((NFOBJECT*)nfts, GDK_EXPOSE, TRUE);
		}

		return FALSE;
	}

	NFUTIL_THREADS_ENTER();

	if(nfts->rkey_id == KEYPAD_DOWN)		prvDownButtonProc(nfts);
	else if(nfts->rkey_id == KEYPAD_UP)		prvUpButtonProc(nfts);
	else if(nfts->rkey_id == KEYPAD_LEFT)	prvLeftButtonProc(nfts);
	else if(nfts->rkey_id == KEYPAD_RIGHT)	prvRightButtonProc(nfts);
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
	NFTIMESPIN *nfts;

	nfts = (NFTIMESPIN*)data;

	if(((NFOBJECT*)nfts)->kfocus == NFOBJECT_UNFOCUS)
		return FALSE;

	NFUTIL_THREADS_ENTER();

	if(nfts->rkey_id == KEYPAD_DOWN)		prvDownButtonProc(nfts);
	else if(nfts->rkey_id == KEYPAD_UP)		prvUpButtonProc(nfts);
	else if(nfts->rkey_id == KEYPAD_LEFT)	prvLeftButtonProc(nfts);
	else if(nfts->rkey_id == KEYPAD_RIGHT)	prvRightButtonProc(nfts);
	else
	{
		NFUTIL_THREADS_LEAVE();
		return FALSE;
	}

	NFUTIL_THREADS_LEAVE();

	if(nfts->mrepeat_src)
		nfts->mrepeat_src = g_timeout_add(TSPINKEY_REPEAT_INTERVAL, repeat_key_proc_short, nfts);
	else if(nfts->krepeat_src)
		nfts->krepeat_src = g_timeout_add(TSPINKEY_REPEAT_INTERVAL, repeat_key_proc_short, nfts);

	return FALSE;
}

static void prvDrawOutLines(NFOBJECT *obj)
{
	NFTIMESPIN *nfts;
	GdkDrawable *drawable = NULL;
	GdkGC *line_gc;
	guint pos_x, pos_y;
	gint line_gap;
	gint line_width;

	drawable = nfui_nfobject_get_window(obj);

	/* outline */
	line_gc = nfui_nfobject_get_gc(obj);

	if(nfui_is_focus_at_child(obj))
		gdk_gc_set_rgb_fg_color(line_gc, &UX_COLOR(137));
	else
		gdk_gc_set_rgb_fg_color(line_gc, &UX_COLOR(136));

	gdk_gc_set_line_attributes(line_gc,
			NFTS_LINE_BORDER,
			GDK_LINE_SOLID,
			GDK_CAP_NOT_LAST,
			GDK_JOIN_MITER);

	nfui_nfobject_get_offset(obj, &pos_x, &pos_y);
	line_gap = NFTS_LINE_BORDER - 1;
	line_width = obj->width - ((NFTIMESPIN*)obj)->up_btn->width;

#if defined(__SAMSUNG_UI__)
	gdk_draw_rectangle(drawable,
					line_gc,
					FALSE,
					pos_x,
					pos_y,
					line_width - 1,
					obj->height - 1);
#else
	gdk_draw_rectangle(drawable,
					line_gc,
					FALSE,
					pos_x + line_gap,
					pos_y + line_gap,
					line_width - (line_gap * 2),
					obj->height - (line_gap * 2));
#endif
	nfui_nfobject_gc_unref(line_gc);

}

static void prvChangeFocus(NFTIMESPIN *nfts, nfts_field_type new_focus)
{
	if(nfts->focus == new_focus)
		return;

	if(IS_VALID_NFTS_FIELD(nfts->focus))
	{
		// change old focus to normal mode.
		nfui_nfobject_modify_bg(nfts->fields[nfts->focus], NFOBJECT_STATE_NORMAL, nfts->bg_color[NFTS_NOFOCUS]);
		nfui_nflabel_modify_fg((NFLABEL*)(nfts->fields[nfts->focus]), nfts->fg_color[NFTS_NOFOCUS]);
	}

	if(IS_VALID_NFTS_FIELD(new_focus))
	{
		nfui_nfobject_modify_bg(nfts->fields[new_focus], NFOBJECT_STATE_NORMAL, nfts->bg_color[NFTS_FOCUS]);
		nfui_nflabel_modify_fg((NFLABEL*)(nfts->fields[new_focus]), nfts->fg_color[NFTS_FOCUS]);
	}

	nfts->focus = new_focus;

	nfui_signal_emit((NFOBJECT*)nfts, GDK_EXPOSE, TRUE);
}

static void prvRefreshDateTime(NFTIMESPIN *nfts)
{
	guint temp;
	struct tm stTime;
	gchar strTemp[8];

	stTime = *NFLOCALTIME(&(nfts->tvTime.tv_sec));
#if 0
	nfui_nflabel_set_text((NFLABEL*)(nfts->fields[NFTS_YEAR]),	g_strdup_printf("%04d", stTime.tm_year+1900));
	nfui_nflabel_set_text((NFLABEL*)(nfts->fields[NFTS_MONTH]),	g_strdup_printf("%02d", stTime.tm_mon+1));
	nfui_nflabel_set_text((NFLABEL*)(nfts->fields[NFTS_DAY]),	g_strdup_printf("%02d", stTime.tm_mday));
#endif
	memset(strTemp, 0, sizeof(strTemp));

	g_sprintf(strTemp, "%04d", stTime.tm_year+1900);	nfui_nflabel_set_text((NFLABEL*)(nfts->fields[NFTS_YEAR]),	strTemp);
	g_sprintf(strTemp, "%02d", stTime.tm_mon+1);		nfui_nflabel_set_text((NFLABEL*)(nfts->fields[NFTS_MONTH]),	strTemp);
	g_sprintf(strTemp, "%02d", stTime.tm_mday);			nfui_nflabel_set_text((NFLABEL*)(nfts->fields[NFTS_DAY]),	strTemp);

	temp = stTime.tm_hour;
	if(nfts->time_mode == NFTS_TM_12H)
	{
		if(temp>12)		temp -= 12;
		else if(!temp)	temp = 12;

	}
#if 0
	nfui_nflabel_set_text((NFLABEL*)(nfts->fields[NFTS_HOUR]),	g_strdup_printf("%02d", temp));
	nfui_nflabel_set_text((NFLABEL*)(nfts->fields[NFTS_MIN]),	g_strdup_printf("%02d", stTime.tm_min));
	nfui_nflabel_set_text((NFLABEL*)(nfts->fields[NFTS_SEC]),	g_strdup_printf("%02d", stTime.tm_sec));
#endif

	g_sprintf(strTemp, "%02d", temp);			nfui_nflabel_set_text((NFLABEL*)(nfts->fields[NFTS_HOUR]),	strTemp);
	g_sprintf(strTemp, "%02d", stTime.tm_min);	nfui_nflabel_set_text((NFLABEL*)(nfts->fields[NFTS_MIN]),	strTemp);
	g_sprintf(strTemp, "%02d", stTime.tm_sec);	nfui_nflabel_set_text((NFLABEL*)(nfts->fields[NFTS_SEC]),	strTemp);

	nfui_nflabel_set_text((NFLABEL*)(nfts->fields[NFTS_APM]), (stTime.tm_hour<12)? "AM":"PM");
}

static void prvFillIndex(NFTIMESPIN *nfts)
{
	guint i = 0;

	if(nfts->date_format != NFTS_DF_HIDE)
	{
		if(nfts->date_format == NFTS_DF_YMD)
		{
			nfts->idx[i++] = NFTS_YEAR;
			nfts->idx[i++] = NFTS_MONTH;
			nfts->idx[i++] = NFTS_DAY;
		}
		else if(nfts->date_format == NFTS_DF_MDY)
		{
			nfts->idx[i++] = NFTS_MONTH;
			nfts->idx[i++] = NFTS_DAY;
			nfts->idx[i++] = NFTS_YEAR;
		}
		else if(nfts->date_format == NFTS_DF_DMY)
		{
			nfts->idx[i++] = NFTS_DAY;
			nfts->idx[i++] = NFTS_MONTH;
			nfts->idx[i++] = NFTS_YEAR;
		}
	}

	if(nfts->time_mode != NFTS_TM_HIDE)
	{
		nfts->idx[i++] = NFTS_HOUR;
		nfts->idx[i++] = NFTS_MIN;
		nfts->idx[i++] = NFTS_SEC;

		if(nfts->time_mode == NFTS_TM_12H)
		{
			nfts->idx[i++] = NFTS_APM;
		}
	}

	nfts->idx[NFTS_FIELDS] = i;
}

static guint prvGetCurFocusIdx(NFTIMESPIN *nfts)
{
	guint idx;

	for(idx=0; idx<NFTS_FIELDS; idx++)
	{
		if(nfts->idx[idx] == nfts->focus)
			break;
	}

	if(idx == NFTS_FIELDS)
		idx = 0;

	return idx;
}

static void prvSetPosition(NFTIMESPIN *nfts)
{
	guint tot_len = 0;
	guint dig_pix = 0; 
	guint tmp_dig_pix = 0;
	guint pos_x;

	guint i;
	guint num[4];

	GdkDrawable *drawable;

	drawable = nfui_nfobject_get_window(nfts);

	nfui_nfobject_move(nfts->up_btn, nfts->fixed.object.width-nfts->btn_width, nfts->fixed.object.height/2 - nfts->btn_height);
	nfui_nfobject_move(nfts->down_btn, nfts->fixed.object.width-nfts->btn_width, nfts->fixed.object.height/2);

//g_assert(GDK_IS_DRAWABLE(drawable));
	for (i=0; i<10; i++)
	{
		sprintf(num, "%d", i);
		tmp_dig_pix = nfutil_string_width(0, drawable, nfts->pango_font, num, NORMAL_SPACING);

		if (tmp_dig_pix > dig_pix)
			dig_pix = tmp_dig_pix;
	}

	if(nfts->time_mode == NFTS_TM_12H)
	{
		tot_len = nfutil_string_width(0, drawable, nfts->pango_font, "AM", NORMAL_SPACING);
	}

	if((nfts->date_format != NFTS_DF_HIDE) && (nfts->time_mode != NFTS_TM_HIDE))
	{
		tot_len += dig_pix*14;
		tot_len += nfutil_string_width(0, drawable, nfts->pango_font, "/", NORMAL_SPACING)*2;
		tot_len += nfutil_string_width(0, drawable, nfts->pango_font, ":", NORMAL_SPACING)*2;
		tot_len += nfutil_string_width(0, drawable, nfts->pango_font, " ", NORMAL_SPACING);
	}
	else if((nfts->date_format != NFTS_DF_HIDE) && (nfts->time_mode == NFTS_TM_HIDE))
	{
		tot_len += dig_pix*8;
		tot_len += nfutil_string_width(0, drawable, nfts->pango_font, "/", NORMAL_SPACING)*2;
	}
	else if((nfts->date_format == NFTS_DF_HIDE) && (nfts->time_mode != NFTS_TM_HIDE))
	{
		tot_len += dig_pix*6;
		tot_len += nfutil_string_width(0, drawable, nfts->pango_font, "/", NORMAL_SPACING)*2;
	}


	pos_x = nfts->fixed.object.width - nfts->btn_width;

	if(pos_x < tot_len)	pos_x = 0;
	else	pos_x -= tot_len;

	pos_x /= 2;

	if(nfts->date_format == NFTS_DF_YMD)
	{
		nfui_nfobject_move(nfts->fields[NFTS_YEAR], pos_x, 2);		pos_x += dig_pix*4;
		nfui_nfobject_move(nfts->deco[0], pos_x, 2);				pos_x += nfutil_string_width(0, drawable, nfts->pango_font, "/", NORMAL_SPACING);
		nfui_nfobject_move(nfts->fields[NFTS_MONTH], pos_x, 2);		pos_x += dig_pix*2;
		nfui_nfobject_move(nfts->deco[1], pos_x, 2);				pos_x += nfutil_string_width(0, drawable, nfts->pango_font, "/", NORMAL_SPACING);
		nfui_nfobject_move(nfts->fields[NFTS_DAY], pos_x, 2);		pos_x += dig_pix*2;
	}
	else if(nfts->date_format == NFTS_DF_MDY)
	{
		nfui_nfobject_move(nfts->fields[NFTS_MONTH], pos_x, 2);		pos_x += dig_pix*2;
		nfui_nfobject_move(nfts->deco[0], pos_x, 2);				pos_x += nfutil_string_width(0, drawable, nfts->pango_font, "/", NORMAL_SPACING);
		nfui_nfobject_move(nfts->fields[NFTS_DAY], pos_x, 2);		pos_x += dig_pix*2;
		nfui_nfobject_move(nfts->deco[1], pos_x, 2);				pos_x += nfutil_string_width(0, drawable, nfts->pango_font, "/", NORMAL_SPACING);
		nfui_nfobject_move(nfts->fields[NFTS_YEAR], pos_x, 2);		pos_x += dig_pix*4;
	}
	else if(nfts->date_format == NFTS_DF_DMY)
	{
		nfui_nfobject_move(nfts->fields[NFTS_DAY], pos_x, 2);		pos_x += dig_pix*2;
		nfui_nfobject_move(nfts->deco[0], pos_x, 2);				pos_x += nfutil_string_width(0, drawable, nfts->pango_font, "/", NORMAL_SPACING);
		nfui_nfobject_move(nfts->fields[NFTS_MONTH], pos_x, 2);		pos_x += dig_pix*2;
		nfui_nfobject_move(nfts->deco[1], pos_x, 2);				pos_x += nfutil_string_width(0, drawable, nfts->pango_font, "/", NORMAL_SPACING);
		nfui_nfobject_move(nfts->fields[NFTS_YEAR], pos_x, 2);		pos_x += dig_pix*4;
	}

	if(nfts->date_format != NFTS_DF_HIDE)
		pos_x += nfutil_string_width(0, drawable, nfts->pango_font, " ", NORMAL_SPACING);

	if(nfts->time_mode != NFTS_TM_HIDE)
	{
		nfui_nfobject_move(nfts->fields[NFTS_HOUR], pos_x, 2);		pos_x += dig_pix*2;
		nfui_nfobject_move(nfts->deco[2], pos_x, 2);				pos_x += nfutil_string_width(0, drawable, nfts->pango_font, ":", NORMAL_SPACING);
		nfui_nfobject_move(nfts->fields[NFTS_MIN], pos_x, 2);		pos_x += dig_pix*2;
		nfui_nfobject_move(nfts->deco[3], pos_x, 2);				pos_x += nfutil_string_width(0, drawable, nfts->pango_font, ":", NORMAL_SPACING);
		nfui_nfobject_move(nfts->fields[NFTS_SEC], pos_x, 2);		pos_x += dig_pix*2;

		if(nfts->time_mode == NFTS_TM_12H)
		{
			pos_x += nfutil_string_width(0, drawable, nfts->pango_font, " ", NORMAL_SPACING);
			nfui_nfobject_move(nfts->fields[NFTS_APM], pos_x, 2);
		}
	}

	nfui_nfobject_set_size(nfts->fields[NFTS_YEAR], dig_pix*4, nfts->fixed.object.height-4);
	nfui_nfobject_set_size(nfts->fields[NFTS_MONTH], dig_pix*2, nfts->fixed.object.height-4);
	nfui_nfobject_set_size(nfts->fields[NFTS_DAY], dig_pix*2, nfts->fixed.object.height-4);
	nfui_nfobject_set_size(nfts->fields[NFTS_HOUR], dig_pix*2, nfts->fixed.object.height-4);
	nfui_nfobject_set_size(nfts->fields[NFTS_MIN], dig_pix*2, nfts->fixed.object.height-4);
	nfui_nfobject_set_size(nfts->fields[NFTS_SEC], dig_pix*2, nfts->fixed.object.height-4);
	nfui_nfobject_set_size(nfts->fields[NFTS_APM], (guint)nfutil_string_width(0, drawable, nfts->pango_font, "AM", NORMAL_SPACING), (guint)(nfts->fixed.object.height-4));

	nfui_nfobject_set_size(nfts->deco[0], (guint)nfutil_string_width(0, drawable, nfts->pango_font, "/", NORMAL_SPACING), nfts->fixed.object.height-4);
	nfui_nfobject_set_size(nfts->deco[1], (guint)nfutil_string_width(0, drawable, nfts->pango_font, "/", NORMAL_SPACING), nfts->fixed.object.height-4);
	nfui_nfobject_set_size(nfts->deco[2], (guint)nfutil_string_width(0, drawable, nfts->pango_font, ":", NORMAL_SPACING), nfts->fixed.object.height-4);
	nfui_nfobject_set_size(nfts->deco[3], (guint)nfutil_string_width(0, drawable, nfts->pango_font, ":", NORMAL_SPACING), nfts->fixed.object.height-4);

}

static time_t mktimeWithCheck (struct tm *tm)
{
	guint8 days[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
	
	if((((tm->tm_year%4) == 0) && tm->tm_year%100 != 0) || (tm->tm_year%400 == 0))
	{
		days[tm->tm_mon]++;
	}

	if(tm->tm_mday > days[tm->tm_mon])
	{
		tm->tm_mday = days[tm->tm_mon];
	}

	return mktime(tm);
}

static void prvUpButtonProc(NFTIMESPIN* nfts)
{
	struct tm stTime;
	struct tm stNFTime_temp1;
	struct tm stNFTime_temp2;
	gboolean needReset = FALSE;
	guint8 days[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
	time_t dst_interval = 0;


	if(nfts->focus < NFTS_YEAR || nfts->focus > NFTS_APM)
	{
		prvChangeFocus(nfts, nfts->idx[0]);
		return;
	}

	if((nfts->focus!=NFTS_APM) && (nfts->tvTime.tv_sec == nfts->upper_limit.tv_sec))
	{
		if(nfts->krepeat_src || nfts->mrepeat_src)
		{
			nfts->mrepeat_src = 0;
			nfts->krepeat_src = 0;
			nfts->rkey_id = 0;
		}
		else 
		{
			nfts->tvTime.tv_sec = nfts->lower_limit.tv_sec;
			nfts->tvTime.tv_usec = 0;
		}

		prvRefreshDateTime(nfts);
		nfui_user_signal_emit((NFOBJECT*)nfts, NFEVENT_TIMESPIN_CHANGED_PRESS, FALSE);

		nfui_signal_emit((NFOBJECT*)nfts, GDK_EXPOSE, TRUE);
		return;
	}

	stTime = *LOCALTIME_R(&(nfts->tvTime.tv_sec));
	stNFTime_temp1 = *NFLOCALTIME(&(nfts->tvTime.tv_sec));

	switch(nfts->focus)
	{
		case NFTS_YEAR:	
			stTime.tm_year++;
			nfts->tvTime.tv_sec = mktimeWithCheck(&stTime);
			break;

		case NFTS_MONTH:
			if(stTime.tm_mon == 11)
			{
				stTime.tm_mon = 0;
				stTime.tm_year++;

				if(nfts->krepeat_src || nfts->mrepeat_src)
				{
					if(!needReset)
						needReset = TRUE;
				}
			}
			else
			{
				stTime.tm_mon++;
			}
			nfts->tvTime.tv_sec = mktimeWithCheck(&stTime);
			break;

		case NFTS_DAY:
			nfts->tvTime.tv_sec += DAY_TO_SEC;

			if(stTime.tm_mon == 1)
			{
				if(stTime.tm_mday >= days[stTime.tm_mon])
				{
					if((((stTime.tm_year%4) == 0) && stTime.tm_year%100 != 0) || (stTime.tm_year%400 == 0))
					{
						days[stTime.tm_mon]++;
					}

					if(stTime.tm_mday == days[stTime.tm_mon])
					{
						if(nfts->krepeat_src || nfts->mrepeat_src)
						{
							if(!needReset)
								needReset = TRUE;
						}
					}
				}
			}
			else 
			{
				if(stTime.tm_mday == days[stTime.tm_mon]) 
				{
					if(nfts->krepeat_src || nfts->mrepeat_src)
					{
						if(!needReset)
							needReset = TRUE;
					}
				}
			}
			break;

		case NFTS_HOUR:
			nfts->tvTime.tv_sec += HOUR_TO_SEC;

			if(stTime.tm_hour == 23) 
			{
				if(nfts->krepeat_src || nfts->mrepeat_src)
				{
					if(!needReset)
						needReset = TRUE;
				}
			}
			break;

		case NFTS_MIN:
			nfts->tvTime.tv_sec += MIN_TO_SEC;

			if(stTime.tm_min == 59) 
			{
				if(nfts->krepeat_src || nfts->mrepeat_src)
				{
					if(!needReset)
						needReset = TRUE;
				}
			}
			break;

		case NFTS_SEC:
			(nfts->tvTime.tv_sec)++;

			if(stTime.tm_sec == 59) 
			{
				if(nfts->krepeat_src || nfts->mrepeat_src)
				{
					if(!needReset)
						needReset = TRUE;
				}
			}
			break;

		case NFTS_APM:
			if(stTime.tm_hour >= 12)
			{
				nfts->tvTime.tv_sec -= (DAY_TO_SEC / 2);

				if(nfts->krepeat_src || nfts->mrepeat_src)
				{
					if(!needReset) 
					{
						nfts->tvTime.tv_sec += (DAY_TO_SEC / 2);
						needReset = TRUE;
					}
				}
			}
			else
			{
				nfts->tvTime.tv_sec += (DAY_TO_SEC / 2);

				if(nfts->krepeat_src || nfts->mrepeat_src)
				{
					if(!needReset) 
					{
						nfts->tvTime.tv_sec -= (DAY_TO_SEC / 2);
						needReset = TRUE;
					}
				}
			}
			break;

		default :
			return;
	}

	if(needReset)
	{
		nfts->mrepeat_src = 0;
		nfts->krepeat_src = 0;
		nfts->rkey_id = 0;
	}

	dst_interval = GET_DST_INTERVAL();

	if(nfts->focus==NFTS_YEAR || nfts->focus==NFTS_MONTH || nfts->focus==NFTS_DAY || nfts->focus==NFTS_APM)
	{
		stNFTime_temp2 = *NFLOCALTIME(&(nfts->tvTime.tv_sec));

		if(stNFTime_temp1.tm_isdst != stNFTime_temp2.tm_isdst)
		{
			if(stNFTime_temp1.tm_isdst)
				nfts->tvTime.tv_sec += dst_interval;
			else
				nfts->tvTime.tv_sec -= dst_interval;
		}
	}

	if(nfts->date_lock && (nfts->date_format==NFTS_DF_HIDE))
	{
		stNFTime_temp2 = *NFLOCALTIME(&(nfts->tvTime.tv_sec));

		if(stNFTime_temp2.tm_mday != stNFTime_temp1.tm_mday)
		{
			nfts->tvTime.tv_sec -= DAY_TO_SEC;

			stNFTime_temp1 = *NFLOCALTIME(&(nfts->tvTime.tv_sec));
			
			if(stNFTime_temp1.tm_isdst && !stNFTime_temp2.tm_isdst)
				nfts->tvTime.tv_sec -= dst_interval;
			else if(!stNFTime_temp1.tm_isdst && stNFTime_temp2.tm_isdst)
				nfts->tvTime.tv_sec += dst_interval;
		}
	} 

	if(nfts->tvTime.tv_sec > nfts->upper_limit.tv_sec)
	{
		nfts->tvTime.tv_sec = nfts->upper_limit.tv_sec;
		nfts->tvTime.tv_usec = 0;
	}
	else if(nfts->tvTime.tv_sec < nfts->lower_limit.tv_sec)
	{
		nfts->tvTime.tv_sec = nfts->lower_limit.tv_sec;
		nfts->tvTime.tv_usec = 0;
	}

	prvRefreshDateTime(nfts);
	nfui_user_signal_emit((NFOBJECT*)nfts, NFEVENT_TIMESPIN_CHANGED_PRESS, FALSE);

	nfui_signal_emit((NFOBJECT*)nfts, GDK_EXPOSE, TRUE);
}

static void prvDownButtonProc(NFTIMESPIN* nfts)
{
	struct tm stTime;
	struct tm stNFTime_temp1;
	struct tm stNFTime_temp2;
	gboolean needReset = FALSE;
	time_t dst_interval = 0;

	if(nfts->focus < NFTS_YEAR || nfts->focus > NFTS_APM)
	{
		prvChangeFocus(nfts, nfts->idx[0]);
		return;
	}

	if((nfts->focus!=NFTS_APM) && (nfts->tvTime.tv_sec == nfts->lower_limit.tv_sec))
	{
		if(nfts->krepeat_src || nfts->mrepeat_src)
		{
			nfts->mrepeat_src = 0;
			nfts->krepeat_src = 0;
			nfts->rkey_id = 0;
		}
		else 
		{
			nfts->tvTime.tv_sec = nfts->upper_limit.tv_sec;
			nfts->tvTime.tv_usec = 0;
		}

		prvRefreshDateTime(nfts);

		nfui_user_signal_emit((NFOBJECT*)nfts, NFEVENT_TIMESPIN_CHANGED_PRESS, FALSE);

		nfui_signal_emit((NFOBJECT*)nfts, GDK_EXPOSE, TRUE);
		return;
	}

	stTime = *LOCALTIME_R(&(nfts->tvTime.tv_sec));
	stNFTime_temp1 = *NFLOCALTIME(&(nfts->tvTime.tv_sec));

	switch(nfts->focus)
	{
		case NFTS_YEAR:
			if(stTime.tm_year)	stTime.tm_year--;
			else				nfts->tvTime.tv_sec = 0;

			nfts->tvTime.tv_sec = mktimeWithCheck(&stTime);
			break;

		case NFTS_MONTH:
			if(stTime.tm_mon == 0)
			{
				if(stTime.tm_year)
				{
					stTime.tm_mon = 11;
					stTime.tm_year--;
				
					if(nfts->krepeat_src || nfts->mrepeat_src)
					{
						if(!needReset)
							needReset = TRUE;
					}
				}
				else
				{
					nfts->tvTime.tv_sec = 0;
				}
			}
			else
			{
				stTime.tm_mon--;

			}

			nfts->tvTime.tv_sec = mktimeWithCheck(&stTime);
			break;

		case NFTS_DAY:
			if(nfts->tvTime.tv_sec > DAY_TO_SEC) 
			{
				nfts->tvTime.tv_sec -= DAY_TO_SEC;
			}
			else
			{
				nfts->tvTime.tv_sec = 0;
			}

			if(stTime.tm_mday == 1) 
			{
				if(nfts->krepeat_src || nfts->mrepeat_src)
				{
					if(!needReset)
						needReset = TRUE;
				}
			}
			break;

		case NFTS_HOUR:
			if(nfts->tvTime.tv_sec > HOUR_TO_SEC)
				nfts->tvTime.tv_sec -= HOUR_TO_SEC;
			else
				nfts->tvTime.tv_sec = 0;
			
			if(stTime.tm_hour == 0) 
			{
				if(nfts->krepeat_src || nfts->mrepeat_src)
				{
					if(!needReset)
						needReset = TRUE;
				}
			}
			break;

		case NFTS_MIN:
			if(nfts->tvTime.tv_sec > MIN_TO_SEC)
				nfts->tvTime.tv_sec -= MIN_TO_SEC;
			else
				nfts->tvTime.tv_sec = 0;
			
			if(stTime.tm_min == 0) 
			{
				if(nfts->krepeat_src || nfts->mrepeat_src)
				{
					if(!needReset)
						needReset = TRUE;
				}
			}
			break;

		case NFTS_SEC:
			if(nfts->tvTime.tv_sec)
				(nfts->tvTime.tv_sec)--;
			else
				nfts->tvTime.tv_sec = 0;

			if(stTime.tm_sec == 0) 
			{
				if(nfts->krepeat_src || nfts->mrepeat_src)
				{
					if(!needReset)
						needReset = TRUE;
				}
			}
			break;

		case NFTS_APM:
			if(stTime.tm_hour >= 12)
			{
				nfts->tvTime.tv_sec -= (DAY_TO_SEC / 2);

				if(nfts->krepeat_src || nfts->mrepeat_src)
				{
					if(!needReset) 
					{
						nfts->tvTime.tv_sec += (DAY_TO_SEC / 2);
						needReset = TRUE;
					}
				}
			}
			else
			{
				nfts->tvTime.tv_sec += (DAY_TO_SEC / 2);

				if(nfts->krepeat_src || nfts->mrepeat_src)
				{
					if(!needReset) 
					{
						nfts->tvTime.tv_sec -= (DAY_TO_SEC / 2);
						needReset = TRUE;
					}
				}
			}
			break;

		default :
			return;
	}

	if(needReset)
	{
		nfts->mrepeat_src = 0;
		nfts->krepeat_src = 0;
		nfts->rkey_id = 0;
	}

	dst_interval = GET_DST_INTERVAL();


	if(nfts->focus==NFTS_YEAR || nfts->focus==NFTS_MONTH || nfts->focus==NFTS_DAY || nfts->focus==NFTS_APM)
	{
		stNFTime_temp2 = *NFLOCALTIME(&(nfts->tvTime.tv_sec));

		if(stNFTime_temp1.tm_isdst != stNFTime_temp2.tm_isdst)
		{
			if(stNFTime_temp1.tm_isdst)
				nfts->tvTime.tv_sec += dst_interval;
			else
				nfts->tvTime.tv_sec -= dst_interval;
		}
	}


	if(nfts->date_lock && (nfts->date_format==NFTS_DF_HIDE))
	{
		stNFTime_temp2 = *NFLOCALTIME(&(nfts->tvTime.tv_sec));

		if(stNFTime_temp2.tm_mday != stNFTime_temp1.tm_mday)
		{
			nfts->tvTime.tv_sec += DAY_TO_SEC;

			stNFTime_temp1 = *NFLOCALTIME(&(nfts->tvTime.tv_sec));
			
			if(stNFTime_temp1.tm_isdst && !stNFTime_temp2.tm_isdst)
				nfts->tvTime.tv_sec -= dst_interval;
			else if(!stNFTime_temp1.tm_isdst && stNFTime_temp2.tm_isdst)
				nfts->tvTime.tv_sec += dst_interval;
		}
	} 

	if(nfts->tvTime.tv_sec < nfts->lower_limit.tv_sec)
	{
		nfts->tvTime.tv_sec = nfts->lower_limit.tv_sec;
		nfts->tvTime.tv_usec = 0;
	}
	else if(nfts->tvTime.tv_sec > nfts->upper_limit.tv_sec)
	{
		nfts->tvTime.tv_sec = nfts->upper_limit.tv_sec;
		nfts->tvTime.tv_usec = 0;
	}

	prvRefreshDateTime(nfts);
	nfui_user_signal_emit((NFOBJECT*)nfts, NFEVENT_TIMESPIN_CHANGED_PRESS, FALSE);

	nfui_signal_emit((NFOBJECT*)nfts, GDK_EXPOSE, TRUE);
}

static void prvLeftButtonProc(NFTIMESPIN* nfts)
{
	gint new_focus;
	guint focus_idx = 0;

	focus_idx = prvGetCurFocusIdx(nfts);

	if(nfts->focus == NFTS_FIELD_NONE)
	{
		new_focus = nfts->idx[0];
	}
	else
	{
		if(focus_idx == 0)	
		{
			if(nfts->krepeat_src) 
			{
				new_focus = nfts->idx[0];
				nfts->krepeat_src = 0;
				nfts->rkey_id = 0;
			}
			else
			{
				new_focus = nfts->idx[nfts->idx[NFTS_FIELDS]-1];
			}
		}
		else	
		{
			new_focus = nfts->idx[focus_idx-1];
		}
	}

	prvChangeFocus(nfts, new_focus);
}

static void prvRightButtonProc(NFTIMESPIN* nfts)
{
	gint new_focus;
	guint focus_idx = 0;

	focus_idx = prvGetCurFocusIdx(nfts);

	if(focus_idx >= nfts->idx[NFTS_FIELDS]-1)
	{
		if(nfts->krepeat_src) 
		{
			new_focus = focus_idx;
			nfts->krepeat_src = 0;
			nfts->rkey_id = 0;
		}
		else
		{
			new_focus = nfts->idx[0];
		}
	}
	else
	{
		new_focus = nfts->idx[focus_idx+1];
	}

	prvChangeFocus(nfts, new_focus);
}

static gboolean post_upbtn_event_handler(NFOBJECT *object, GdkEvent *event, gpointer data)
{
	NFTIMESPIN *nfts;

	nfts = (NFTIMESPIN*)(object->parent);

	if(event->type == GDK_BUTTON_PRESS)
	{
		if(nfts->krepeat_src)
		{
			g_source_remove(nfts->krepeat_src);
			nfts->krepeat_src = 0;

			if(nfts->rkey_id == KEYPAD_UP || nfts->rkey_id == KEYPAD_DOWN)
			{
				nfui_user_signal_emit((NFOBJECT*)nfts, NFEVENT_TIMESPIN_CHANGED, FALSE);
				nfui_signal_emit((NFOBJECT*)nfts, GDK_EXPOSE, TRUE);
			}
		}

  	   	if(event->button.button == MOUSE_RIGTH_BUTTON) {					
		  return FALSE;
  	   	}
		

		prvUpButtonProc(nfts);

		if(!nfts->mrepeat_src)
		{
			nfts->rkey_id = KEYPAD_UP;
			nfts->mrepeat_src = g_timeout_add(TSPINKEY_REPEAT_INTERVAL * 3, repeat_key_proc, nfts);
		}
	}
	else if(event->type == GDK_BUTTON_RELEASE || event->type == GDK_LEAVE_NOTIFY)
	{
		if(nfts->krepeat_src)
		{
			g_source_remove(nfts->krepeat_src);
			nfts->krepeat_src = 0;

			if(nfts->rkey_id == KEYPAD_UP || nfts->rkey_id == KEYPAD_DOWN)
			{
				nfui_user_signal_emit((NFOBJECT*)nfts, NFEVENT_TIMESPIN_CHANGED, FALSE);
				nfui_signal_emit((NFOBJECT*)nfts, GDK_EXPOSE, TRUE);
			}
		}

		if(nfts->mrepeat_src)
		{
			g_source_remove(nfts->mrepeat_src);
			nfts->mrepeat_src = 0;

			nfui_user_signal_emit((NFOBJECT*)nfts, NFEVENT_TIMESPIN_CHANGED, FALSE);
			nfui_signal_emit((NFOBJECT*)nfts, GDK_EXPOSE, TRUE);
		}
	}

	return FALSE;
}

static gboolean post_downbtn_event_handler(NFOBJECT *object, GdkEvent *event, gpointer data)
{
	NFTIMESPIN *nfts;

	nfts = (NFTIMESPIN*)(object->parent);

	if(event->type == GDK_BUTTON_PRESS)
	{
		if(nfts->krepeat_src)
		{
			g_source_remove(nfts->krepeat_src);
			nfts->krepeat_src = 0;

			if(nfts->rkey_id == KEYPAD_UP || nfts->rkey_id == KEYPAD_DOWN)
			{
				nfui_user_signal_emit((NFOBJECT*)nfts, NFEVENT_TIMESPIN_CHANGED, FALSE);
				nfui_signal_emit((NFOBJECT*)nfts, GDK_EXPOSE, TRUE);
			}
		}

  	   	if(event->button.button == MOUSE_RIGTH_BUTTON) {					
		  return FALSE;
  	   	}

		prvDownButtonProc(nfts);

		if(!nfts->mrepeat_src)
		{
			nfts->rkey_id = KEYPAD_DOWN;
			nfts->mrepeat_src = g_timeout_add(TSPINKEY_REPEAT_INTERVAL * 3, repeat_key_proc, nfts);
		}
	}
	else if(event->type == GDK_BUTTON_RELEASE || event->type == GDK_LEAVE_NOTIFY)
	{	
		if(nfts->krepeat_src)
		{
			g_source_remove(nfts->krepeat_src);
			nfts->krepeat_src = 0;

			if(nfts->rkey_id == KEYPAD_UP || nfts->rkey_id == KEYPAD_DOWN)
			{
				nfui_user_signal_emit((NFOBJECT*)nfts, NFEVENT_TIMESPIN_CHANGED, FALSE);
				nfui_signal_emit((NFOBJECT*)nfts, GDK_EXPOSE, TRUE);
			}
		}

		if(nfts->mrepeat_src)
		{
			g_source_remove(nfts->mrepeat_src);
			nfts->mrepeat_src = 0;

			nfui_user_signal_emit((NFOBJECT*)nfts, NFEVENT_TIMESPIN_CHANGED, FALSE);
			nfui_signal_emit((NFOBJECT*)nfts, GDK_EXPOSE, TRUE);
		}
	}

	return FALSE;
}

static gboolean post_dtfields_event_handler(NFOBJECT *object, GdkEvent *event, gpointer data)
{
	NFTIMESPIN *nfts;
	guint i;

	nfts = (NFTIMESPIN*)(object->parent);

	if(event->type == GDK_BUTTON_PRESS)
	{
		if(nfts->krepeat_src)
		{
			g_source_remove(nfts->krepeat_src);
			nfts->krepeat_src = 0;

			if(nfts->rkey_id == KEYPAD_UP || nfts->rkey_id == KEYPAD_DOWN)
			{
				nfui_user_signal_emit((NFOBJECT*)nfts, NFEVENT_TIMESPIN_CHANGED, FALSE);
				nfui_signal_emit((NFOBJECT*)nfts, GDK_EXPOSE, TRUE);
			}
		}


  	   	if(event->button.button == MOUSE_RIGTH_BUTTON) {					
		  return FALSE;
  	   	}

		for(i=NFTS_YEAR; i<NFTS_FIELDS; i++)
		{
			if(object == nfts->fields[i])
				break;
		}

		if(i != NFTS_FIELDS)	prvChangeFocus(nfts, i);
	}
	else if(event->type == NFEVENT_KEYPAD_PRESS || event->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		if(nfts->mrepeat_src)
		{
			g_source_remove(nfts->mrepeat_src);
			nfts->mrepeat_src = 0;

			nfui_user_signal_emit((NFOBJECT*)nfts, NFEVENT_TIMESPIN_CHANGED, FALSE);
			nfui_signal_emit((NFOBJECT*)nfts, GDK_EXPOSE, TRUE);
		}

		kevt = (GdkEventKey*)event;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_UP) 
		{
//			nfui_signal_emit(nfts->up_btn, GDK_BUTTON_PRESS, FALSE);
			prvUpButtonProc(nfts);

			if(!nfts->krepeat_src)
			{
				nfts->rkey_id = KEYPAD_UP;
				nfts->krepeat_src = g_timeout_add(TSPINKEY_REPEAT_INTERVAL * 3, repeat_key_proc, nfts);
			}
			return TRUE;
		}
		else if(kpid == KEYPAD_DOWN) 
		{
//			nfui_signal_emit(nfts->down_btn, GDK_BUTTON_PRESS, FALSE);
			prvDownButtonProc(nfts);

			if(!nfts->krepeat_src)
			{
				nfts->rkey_id = KEYPAD_DOWN;
				nfts->krepeat_src = g_timeout_add(TSPINKEY_REPEAT_INTERVAL * 3, repeat_key_proc, nfts);
			}
			return TRUE;
		}
		else if(kpid == KEYPAD_LEFT) 
		{
			prvLeftButtonProc(nfts);
			if(!nfts->krepeat_src)
			{
				nfts->rkey_id = KEYPAD_LEFT;
				nfts->krepeat_src = g_timeout_add(TSPINKEY_REPEAT_INTERVAL * 3, repeat_key_proc, nfts);
			}

			return TRUE;
		}
		else if(kpid == KEYPAD_RIGHT) 
		{
			prvRightButtonProc(nfts);
			if(!nfts->krepeat_src)
			{
				nfts->rkey_id = KEYPAD_RIGHT;
				nfts->krepeat_src = g_timeout_add(TSPINKEY_REPEAT_INTERVAL * 3, repeat_key_proc, nfts);
			}

			return TRUE;
		}
		else if(kpid == KEYPAD_ENTER)
		{
			if(memcmp(&nfts->tvTime, &nfts->pre_tvTime, sizeof(GTimeVal))) 
				memcpy(&nfts->pre_tvTime, &nfts->tvTime, sizeof(GTimeVal));

			nfui_set_key_focus((NFOBJECT*)nfts, FALSE);
			nfui_set_key_focus((NFOBJECT*)nfts, TRUE);

			prvChangeFocus(nfts, NFTS_FIELD_NONE);

			return TRUE;
		}
		else if(kpid == KEYPAD_EXIT)
		{
			if(memcmp(&nfts->tvTime, &nfts->pre_tvTime, sizeof(GTimeVal))) {
				memcpy(&nfts->tvTime, &nfts->pre_tvTime, sizeof(GTimeVal));

				prvRefreshDateTime(nfts);

				nfui_user_signal_emit((NFOBJECT*)nfts, NFEVENT_TIMESPIN_CHANGED, FALSE);
			}

			prvChangeFocus(nfts, NFTS_FIELD_NONE);
		}
	}
	else if(event->type == NFEVENT_KEYPAD_RELEASE || event->type == NFEVENT_REMOCON_RELEASE || event->type == GDK_LEAVE_NOTIFY)
	{
		if(nfts->krepeat_src)
		{
			g_source_remove(nfts->krepeat_src);
			nfts->krepeat_src = 0;

			if(nfts->rkey_id == KEYPAD_UP || nfts->rkey_id == KEYPAD_DOWN)
			{
				nfui_user_signal_emit((NFOBJECT*)nfts, NFEVENT_TIMESPIN_CHANGED, FALSE);
				nfui_signal_emit((NFOBJECT*)nfts, GDK_EXPOSE, TRUE);
			}
		}

		if(nfts->mrepeat_src)
		{
			g_source_remove(nfts->mrepeat_src);
			nfts->mrepeat_src = 0;

			nfui_user_signal_emit((NFOBJECT*)nfts, NFEVENT_TIMESPIN_CHANGED, FALSE);
			nfui_signal_emit((NFOBJECT*)nfts, GDK_EXPOSE, TRUE);
		}
	}

	return FALSE;
}

static gboolean nftimespin_event_handler(NFTIMESPIN *nfts, GdkEvent *event, gpointer data)
{
	switch(event->type)
	{
		case GDK_EXPOSE:
		{
			GdkDrawable *drawable;
			GdkGC *gc;
			guint x, y, w, h;

			drawable = nfui_nfobject_get_window((NFOBJECT*)nfts);
			gc = nfui_nfobject_get_gc((NFOBJECT*)nfts);

			nfui_nfobject_get_offset((NFOBJECT*)nfts, &x, &y);
			w = ((NFOBJECT*)nfts)->width;
			h = ((NFOBJECT*)nfts)->height;

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(nfts->bg_color[NFTS_NOFOCUS]));
			gdk_draw_rectangle(drawable, gc, TRUE, (gint)x, (gint)y, (gint)w, (gint)h);
			nfui_nfobject_gc_unref(gc);

#if defined(__SAMSUNG_UI__)
			prvDrawOutLines((NFOBJECT*)nfts);

			if(nfts->fixed.object.kfocus == NFOBJECT_UNFOCUS) 
				prvChangeFocus(nfts, NFTS_FIELD_NONE);
#else
			if(nfts->fixed.object.kfocus == NFOBJECT_FOCUS) 
				prvDrawOutLines((NFOBJECT*)nfts);
			else
				prvChangeFocus(nfts, NFTS_FIELD_NONE);
#endif

		}
		break;

		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
		{
			GdkEventKey *kevt;
			KEYPAD_KID kpid;

			if(nfts->mrepeat_src)
			{
				g_source_remove(nfts->mrepeat_src);
				nfts->mrepeat_src = 0;

				nfui_user_signal_emit((NFOBJECT*)nfts, NFEVENT_TIMESPIN_CHANGED, FALSE);
				nfui_signal_emit((NFOBJECT*)nfts, GDK_EXPOSE, TRUE);
			}

			kevt = (GdkEventKey*)event;
			kpid = (KEYPAD_KID)kevt->keyval;

			if(kpid == KEYPAD_ENTER)
			{
				if(nfts->focus < NFTS_YEAR || nfts->focus > NFTS_APM)
				{
					prvChangeFocus(nfts, nfts->idx[0]);
					return FALSE;
				}
			}
		}

		case GDK_ENTER_NOTIFY:
			if(nfui_nfobject_is_shown((NFOBJECT*)nfts))
				nfui_signal_emit((NFOBJECT*)nfts, GDK_EXPOSE, TRUE);
			break;

		case GDK_LEAVE_NOTIFY:
			if(nfui_nfobject_is_shown((NFOBJECT*)nfts))
				nfui_signal_emit((NFOBJECT*)nfts, GDK_EXPOSE, TRUE);
		case GDK_BUTTON_RELEASE:
		case NFEVENT_KEYPAD_RELEASE:
		case NFEVENT_REMOCON_RELEASE:
			if(nfts->mrepeat_src)
			{
				g_source_remove(nfts->mrepeat_src);
				nfts->mrepeat_src = 0;

				nfui_user_signal_emit((NFOBJECT*)nfts, NFEVENT_TIMESPIN_CHANGED, FALSE);
				nfui_signal_emit((NFOBJECT*)nfts, GDK_EXPOSE, TRUE);
			}
			if(nfts->krepeat_src)
			{
				g_source_remove(nfts->krepeat_src);
				nfts->krepeat_src = 0;

				if(nfts->rkey_id == KEYPAD_UP || nfts->rkey_id == KEYPAD_DOWN)
				{
					nfui_user_signal_emit((NFOBJECT*)nfts, NFEVENT_TIMESPIN_CHANGED, FALSE);
					nfui_signal_emit((NFOBJECT*)nfts, GDK_EXPOSE, TRUE);
				}
			}
			break;

		case GDK_DELETE:
			if(nfts)
			{
				if(nfts->mrepeat_src)
				{
					g_source_remove(nfts->mrepeat_src);
					nfts->mrepeat_src = 0;
				}
				if(nfts->krepeat_src)
				{
					g_source_remove(nfts->krepeat_src);
					nfts->krepeat_src = 0;
				}

			}
			break;

		default:
			break;

	}

	return FALSE;
}

NFTIMESPIN *nfui_nftimespin_new(GdkPixbuf **up_img, GdkPixbuf **down_img)
{
	NFTIMESPIN *nfts;
	guint x, y;
	guint i;


	nfts = (NFTIMESPIN*)imalloc(sizeof(NFTIMESPIN));
	if(!nfts)	return NULL;

#if 0
	nfts->fixed.object.parent = NULL;
	nfts->fixed.object.x = 0;
	nfts->fixed.object.y = 0;
	nfts->fixed.object.width = 210; //142;
	nfts->fixed.object.height = 22;
	nfts->fixed.object.type = NFOBJECT_TYPE_NFTIMESPIN;
	nfts->fixed.object.status = NFOBJECT_STATE_NORMAL;
	nfts->fixed.object.show = NFOBJECT_HIDE;
	nfts->fixed.object.use_focus = NFOBJECT_FOCUS_ON;
	nfts->fixed.object.kfocus = NFOBJECT_UNFOCUS;
	nfts->fixed.object.mfocus = NFOBJECT_UNFOCUS;
	nfts->fixed.object.bg_color[NFOBJECT_STATE_NORMAL] = -1;
	nfts->fixed.object.bg_color[NFOBJECT_STATE_PRELIGHT] = -1;
	nfts->fixed.object.bg_color[NFOBJECT_STATE_ACTIVE] = -1;
	nfts->fixed.object.bg_color[NFOBJECT_STATE_DISABLE] = NFUI_DISABLED_COLOR;
	nfts->fixed.object.pre_event_handler = NULL;
	nfts->fixed.object.default_event_handler = nftimespin_event_handler;
	nfts->fixed.object.post_event_handler = NULL;
#else
	nfui_nfobject_init((NFOBJECT*)nfts);

	nfts->fixed.object.width = 210; //142;
	nfts->fixed.object.height = 22;
	nfts->fixed.object.type = NFOBJECT_TYPE_NFTIMESPIN;
	nfts->fixed.object.use_focus = NFOBJECT_FOCUS_ON;
	nfts->fixed.object.default_event_handler = nftimespin_event_handler;
#endif

	nfts->use_cashing = 1;

	g_get_current_time(&(nfts->tvTime));
	nfts->tvTime.tv_usec = 0;
	memcpy(&nfts->pre_tvTime, &nfts->tvTime, sizeof(GTimeVal));

	memset(&(nfts->upper_limit), 0, sizeof(GTimeVal));
	memset(&(nfts->lower_limit), 0, sizeof(GTimeVal));

	nfts->upper_limit.tv_sec = NFTS_DEF_UPPER_TIMELIMIT; 
	nfts->lower_limit.tv_sec = NFTS_DEF_LOWER_TIMELIMIT;

	nfts->date_lock = 0;
	nfts->date_format = NFTS_DF_YMD;
	nfts->time_mode = NFTS_TM_24H;
	nfts->focus = NFTS_FIELD_NONE;

	strncpy(nfts->font_name, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), sizeof(nfts->font_name));
	nfts->pango_font = nfts->font_name;

	nfts->bg_color[NFTS_NOFOCUS] = -1;
	nfts->bg_color[NFTS_FOCUS] = COLOR_IDX(130);		

	nfts->fg_color[NFTS_NOFOCUS] = COLOR_IDX(129);
	nfts->fg_color[NFTS_FOCUS] = COLOR_IDX(131);

  nfts->spacing_type = SEMI_CONDENSED_SPACING;
  
	nfui_get_pixbuf_size(up_img[0], &(nfts->btn_width), &(nfts->btn_height));

	for(i=0; i<4; i++)
	{
		if(i/2 == 0)	nfts->deco[i] = nfui_nflabel_new_with_pango_font("/", nfts->pango_font, nfts->fg_color[NFTS_NOFOCUS]);
		else			nfts->deco[i] = nfui_nflabel_new_with_pango_font(":", nfts->pango_font, nfts->fg_color[NFTS_NOFOCUS]);
    
    nfui_nflabel_set_spacing((NFLABEL*)(nfts->deco[i]), nfts->spacing_type);
		nfui_nflabel_modify_fg(nfts->deco[i], nfts->fg_color[NFTS_NOFOCUS]);
		nfui_nflabel_set_drawing_outline((NFLABEL*)(nfts->deco[i]), FALSE);
		nfui_nfobject_modify_bg(nfts->deco[i], NFOBJECT_STATE_NORMAL, nfts->bg_color[NFTS_NOFOCUS]);
		nfui_nfobject_use_tooltip(nfts->deco[i], FALSE);

		nfui_nfobject_use_focus(nfts->deco[i], NFOBJECT_FOCUS_OFF);
		nfui_nffixed_put((NFFIXED*)nfts, nfts->deco[i], 0, 0);

		nfui_nfobject_show(nfts->deco[i]);
	}


	for(i=NFTS_YEAR; i<NFTS_FIELDS; i++)
	{
		nfts->fields[i] = nfui_nflabel_new_with_pango_font("", nfts->pango_font, nfts->fg_color[NFTS_NOFOCUS]);
//		nfui_nflabel_use_pango_cashing(nfts->fields[i], 0, NULL);
    nfui_nflabel_set_spacing((NFLABEL*)(nfts->fields[i]), nfts->spacing_type);
		nfui_nflabel_set_drawing_outline((NFLABEL*)(nfts->fields[i]), FALSE);
		nfui_nflabel_modify_fg(nfts->fields[i], nfts->fg_color[NFTS_NOFOCUS]);
		nfui_nfobject_modify_bg(nfts->fields[i], NFOBJECT_STATE_NORMAL, nfts->bg_color[NFTS_NOFOCUS]);
		nfui_nfobject_use_tooltip(nfts->fields[i], FALSE);

		nfui_nffixed_put((NFFIXED*)nfts, nfts->fields[i], 0, 0);

		nfui_nfobject_show(nfts->fields[i]);

		nfui_regi_post_event_callback(nfts->fields[i], post_dtfields_event_handler);
	}

	nfts->up_btn = nfui_nfbutton_new();
	nfui_nfbutton_set_image(nfts->up_btn, up_img);
	nfui_nfobject_use_focus(nfts->up_btn, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(nfts->up_btn, nfts->btn_width, nfts->btn_height);
	x = nfts->fixed.object.width - nfts->up_btn->width;
	y = nfts->fixed.object.height/2-nfts->up_btn->height;
	nfui_nffixed_put((NFFIXED*)nfts, nfts->up_btn, x, y);
	nfui_nfobject_show(nfts->up_btn);

	nfts->down_btn = nfui_nfbutton_new();
	nfui_nfbutton_set_image(nfts->down_btn, down_img);
	nfui_nfobject_use_focus(nfts->down_btn, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(nfts->down_btn, nfts->btn_width, nfts->btn_height);
	y = nfts->fixed.object.height/2;
	nfui_nffixed_put((NFFIXED*)nfts, nfts->down_btn, x, y);
	nfui_nfobject_show(nfts->down_btn);

	nfui_regi_post_event_callback(nfts->up_btn, post_upbtn_event_handler);
	nfui_regi_post_event_callback(nfts->down_btn, post_downbtn_event_handler);


	nfui_nftimespin_set_mode(nfts, nfts->date_format, nfts->time_mode);

	return nfts;
}

NFTIMESPIN* nfui_nftimespin_new_with_time(GdkPixbuf **up_img, GdkPixbuf **down_img, GTimeVal *tv)
{
	NFTIMESPIN *nfts;

	nfts = nfui_nftimespin_new(up_img, down_img);
	if(nfts == NULL)
		return NULL;

	nfts->tvTime = *tv;
	memcpy(&nfts->pre_tvTime, &nfts->tvTime, sizeof(GTimeVal));

	return nfts;
}

void nfui_nftimespin_get_datetime(NFTIMESPIN *nfts, GTimeVal *tv)
{
	if(nfts->fixed.object.type != NFOBJECT_TYPE_NFTIMESPIN)
	{
		return;
	}

	*tv = nfts->tvTime;
//	tv->tv_sec = mktime(&(nfts->stTime));
//	tv->tv_usec = 0;
}

void nfui_nftimespin_set_datetime(NFTIMESPIN *nfts, GTimeVal *tv)
{
	if(nfts->fixed.object.type != NFOBJECT_TYPE_NFTIMESPIN)
	{
		return;
	}

	nfts->tvTime = *tv;
	memcpy(&nfts->pre_tvTime, &nfts->tvTime, sizeof(GTimeVal));

	prvRefreshDateTime(nfts);
}

void nfui_nftimespin_set_mode(NFTIMESPIN *nfts, nfts_df_type date_format, nfts_time_mode tm)
{
	guint i;

	if(nfts->fixed.object.type != NFOBJECT_TYPE_NFTIMESPIN)
	{
		return;
	}

	if(date_format == NFTS_DF_HIDE && tm == NFTS_TM_HIDE)
	{
		g_message("Both Date and time can not bo hide..");
		return;
	}

	nfts->date_format = date_format;
	nfts->time_mode = tm;

	if(tm == NFTS_TM_24H)		nfui_nfobject_hide(nfts->fields[NFTS_APM]);
	else if(tm == NFTS_TM_12H)	nfui_nfobject_show(nfts->fields[NFTS_APM]);


	if((date_format != NFTS_DF_HIDE) && (tm != NFTS_TM_HIDE))
	{
		for(i=NFTS_YEAR; i<=NFTS_SEC; i++)
		{
			nfui_nfobject_show(nfts->fields[i]);
		}
		
		nfui_nfobject_show(nfts->deco[0]);
		nfui_nfobject_show(nfts->deco[1]);
		nfui_nfobject_show(nfts->deco[2]);
		nfui_nfobject_show(nfts->deco[3]);

	}
	else if((date_format != NFTS_DF_HIDE) && (tm == NFTS_TM_HIDE))
	{
		for(i=NFTS_YEAR; i<=NFTS_DAY; i++)
		{
			nfui_nfobject_show(nfts->fields[i]);
		}

		for(i=NFTS_HOUR; i<=NFTS_APM; i++)
		{
			nfui_nfobject_hide(nfts->fields[i]);
		}

		nfui_nfobject_show(nfts->deco[0]);
		nfui_nfobject_show(nfts->deco[1]);
		nfui_nfobject_hide(nfts->deco[2]);
		nfui_nfobject_hide(nfts->deco[3]);

	}
	else if((date_format == NFTS_DF_HIDE) && (tm != NFTS_TM_HIDE))
	{
		for(i=NFTS_YEAR; i<=NFTS_DAY; i++)
		{
			nfui_nfobject_hide(nfts->fields[i]);
		}

		for(i=NFTS_HOUR; i<=NFTS_SEC; i++)
		{
			nfui_nfobject_show(nfts->fields[i]);
		}

		nfui_nfobject_hide(nfts->deco[0]);
		nfui_nfobject_hide(nfts->deco[1]);
		nfui_nfobject_show(nfts->deco[2]);
		nfui_nfobject_show(nfts->deco[3]);
	}

	prvFillIndex(nfts);
	prvSetPosition(nfts);
	prvRefreshDateTime(nfts);
}

void nfui_nftimespin_set_size(NFTIMESPIN *nfts, guint width, guint height)
{
	if(nfts->fixed.object.type != NFOBJECT_TYPE_NFTIMESPIN)
	{
		return;
	}

	nfts->fixed.object.width = width;
	nfts->fixed.object.height = height;

	prvSetPosition(nfts);
}

void nfui_nftimespin_set_pango_font(NFTIMESPIN *nfts, const gchar *pfont)
{
	guint i = 0;

	if(nfts->fixed.object.type != NFOBJECT_TYPE_NFTIMESPIN)
	{
		return;
	}

//	strncpy(nfts->pango_font, pfont, sizeof(nfts->pango_font));

	if (pfont) {
		if (nffont_is_system_font(pfont))
			nfts->pango_font = pfont;
		else {
			strncpy(nfts->font_name, pfont, sizeof(nfts->font_name));
			nfts->pango_font = nfts->font_name;
		}
	}

	for(i=NFTS_YEAR; i<NFTS_FIELDS; i++)
	{
		nfui_nflabel_set_pango_font((NFLABEL*)nfts->fields[i], nfts->pango_font, nfts->fg_color[NFTS_NOFOCUS]);
		nfui_nflabel_set_spacing((NFLABEL*)(nfts->fields[i]), nfts->spacing_type);
	}

	for(i=0; i<4; i++)
	{
	  nfui_nflabel_set_pango_font((NFLABEL*)nfts->deco[i], nfts->pango_font, nfts->fg_color[NFTS_NOFOCUS]);
	  nfui_nflabel_set_spacing((NFLABEL*)(nfts->deco[i]), nfts->spacing_type);
	}
}

void nfui_nftimespin_use_pango_cashing(NFTIMESPIN *nfts, gint cashing, gchar *key)
{
	gint i;

	if(nfts->fixed.object.type != NFOBJECT_TYPE_NFTIMESPIN)
	{
		return;
	}

	if(cashing)	nfts->use_cashing = 1;
	else		nfts->use_cashing = 0;

	if(key && strlen(key)>0)
		strncpy(nfts->cashing_key, key, sizeof(nfts->cashing_key));
	else
		memset(nfts->cashing_key, 0, sizeof(nfts->cashing_key));

	for(i=NFTS_YEAR; i<NFTS_FIELDS; i++)
		nfui_nflabel_use_pango_cashing(nfts->fields[i], cashing, key);
}

void nfui_nftimespin_set_fg_color(NFTIMESPIN *nfts, guint focus, guint normal)
{
	if(nfts->fixed.object.type != NFOBJECT_TYPE_NFTIMESPIN)
	{
		return;
	}

	nfts->fg_color[NFTS_NOFOCUS] = normal;
	nfts->fg_color[NFTS_FOCUS] = focus;
}

void nfui_nftimespin_set_bg_color(NFTIMESPIN *nfts, guint focus, guint normal)
{
	if(nfts->fixed.object.type != NFOBJECT_TYPE_NFTIMESPIN)
	{
		return;
	}

	nfts->bg_color[NFTS_NOFOCUS] = normal;
	nfts->bg_color[NFTS_FOCUS] = focus;

	((NFOBJECT*)nfts)->bg_color[NFOBJECT_STATE_NORMAL] = normal;
	// nfui_nfobject_modify_bg((NFOBJECT*)nfts, NFOBJECT_STATE_NORMAL, normal);
}


void nfui_nftimespin_set_date_lock(NFTIMESPIN *nfts, gboolean lock)
{
	if(!nfts)	return;

	if(nfts->fixed.object.type != NFOBJECT_TYPE_NFTIMESPIN)
	{
		return;
	}

	if(lock)	nfts->date_lock = 1;
	else		nfts->date_lock = 0;
}

void nfui_nftimespin_set_limit(NFTIMESPIN *nfts, GTimeVal *lower, GTimeVal *upper)
{
	if(nfts->fixed.object.type != NFOBJECT_TYPE_NFTIMESPIN)
	{
		return;
	}

	if(lower)	nfts->lower_limit = *lower;
	else		nfts->lower_limit.tv_sec = NFTS_DEF_LOWER_TIMELIMIT;

	if(upper)	nfts->upper_limit = *upper;
	else		nfts->upper_limit.tv_sec = NFTS_DEF_UPPER_TIMELIMIT;

	nfts->lower_limit.tv_usec = 0;
	nfts->upper_limit.tv_usec = 0;
}




