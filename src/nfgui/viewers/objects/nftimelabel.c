


#include "nftimelabel.h"
#include "nffixed.h"
#include "nfbutton.h"
#include "nflabel.h"

#include "../../support/event_loop.h"
#include "../../support/util.h"
#include "../../support/color.h"

#include "ix_mem.h"


#define	TSPINKEY_REPEAT_INTERVAL		(300)

#define	NFTL_DEF_LOWER_TIMELIMIT	NF_LOWER_TIMELIMIT	
#define	NFTL_DEF_UPPER_TIMELIMIT	NF_UPPER_TIMELIMIT	

#define NFTL_LINE_BORDER			2

#define	IS_VALID_NFTL_FIELD(a)		((a>=NFTL_YEAR && a<NFTL_FIELDS) ? 1:0)


#define	DAY_TO_SEC		((time_t)86400)
#define	HOUR_TO_SEC		((time_t)3600)
#define	MIN_TO_SEC		((time_t)60)



static void prvDrawOutLines(NFOBJECT *obj)
{
	NFTIMELABEL *nftl;
	GdkDrawable *drawable = NULL;
	GdkGC *line_gc;
	guint pos_x, pos_y;
	gint line_gap;

	drawable = nfui_nfobject_get_window(obj);

	/* outline */
	line_gc = nfui_nfobject_get_gc(obj);

	if(nfui_is_focus_at_child(obj))
		gdk_gc_set_rgb_fg_color(line_gc, &UX_COLOR(147));
	else
		gdk_gc_set_rgb_fg_color(line_gc, &UX_COLOR(146));

	gdk_gc_set_line_attributes(line_gc,
			NFTL_LINE_BORDER,
			GDK_LINE_SOLID,
			GDK_CAP_NOT_LAST,
			GDK_JOIN_MITER);

	nfui_nfobject_get_offset(obj, &pos_x, &pos_y);
	line_gap = NFTL_LINE_BORDER - 1;

	gdk_draw_rectangle(drawable,
					line_gc,
					FALSE,
					pos_x + line_gap,
					pos_y + line_gap,
					obj->width - (line_gap * 2),
					obj->height - (line_gap * 2));

	nfui_nfobject_gc_unref(line_gc);

}

static void _set_text_expose(NFOBJECT *obj, gchar *strBuf)
{
    if(strcmp(strBuf, nfui_nflabel_get_text(obj))!= 0)
    {
        nfui_nflabel_set_text((NFLABEL*)obj, strBuf);
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }
}

static void prvRefreshDateTime_expose(NFTIMELABEL *nftl)
{
    guint temp;
	struct tm stTime;
	gchar strTemp[8];

	stTime = *NFLOCALTIME(&(nftl->tvTime.tv_sec));

	memset(strTemp, 0, sizeof(strTemp));

	g_sprintf(strTemp, "%04d", stTime.tm_year+1900);	_set_text_expose((NFLABEL*)(nftl->fields[NFTL_YEAR]),	strTemp);
	g_sprintf(strTemp, "%02d", stTime.tm_mon+1);		_set_text_expose((NFLABEL*)(nftl->fields[NFTL_MONTH]),	strTemp);
	g_sprintf(strTemp, "%02d", stTime.tm_mday);			_set_text_expose((NFLABEL*)(nftl->fields[NFTL_DAY]),	strTemp);

	temp = stTime.tm_hour;
	if(nftl->time_mode == NFTL_TM_12H)
	{
		if(temp>12)		temp -= 12;
		else if(!temp)	temp = 12;

	}

	g_sprintf(strTemp, "%02d", temp);			_set_text_expose((NFLABEL*)(nftl->fields[NFTL_HOUR]),	strTemp);
	g_sprintf(strTemp, "%02d", stTime.tm_min);	_set_text_expose((NFLABEL*)(nftl->fields[NFTL_MIN]),	strTemp);
	g_sprintf(strTemp, "%02d", stTime.tm_sec);	_set_text_expose((NFLABEL*)(nftl->fields[NFTL_SEC]),	strTemp);

	nfui_nflabel_set_text((NFLABEL*)(nftl->fields[NFTL_APM]), (stTime.tm_hour<12)? "AM":"PM");
}

static void prvRefreshDateTime(NFTIMELABEL *nftl)
{
	guint temp;
	struct tm stTime;
	gchar strTemp[8];

	stTime = *NFLOCALTIME(&(nftl->tvTime.tv_sec));

	memset(strTemp, 0, sizeof(strTemp));

	g_sprintf(strTemp, "%04d", stTime.tm_year+1900);	nfui_nflabel_set_text((NFLABEL*)(nftl->fields[NFTL_YEAR]),	strTemp);
	g_sprintf(strTemp, "%02d", stTime.tm_mon+1);		nfui_nflabel_set_text((NFLABEL*)(nftl->fields[NFTL_MONTH]),	strTemp);
	g_sprintf(strTemp, "%02d", stTime.tm_mday);			nfui_nflabel_set_text((NFLABEL*)(nftl->fields[NFTL_DAY]),	strTemp);

	temp = stTime.tm_hour;
	if(nftl->time_mode == NFTL_TM_12H)
	{
		if(temp>12)		temp -= 12;
		else if(!temp)	temp = 12;

	}

	g_sprintf(strTemp, "%02d", temp);			nfui_nflabel_set_text((NFLABEL*)(nftl->fields[NFTL_HOUR]),	strTemp);
	g_sprintf(strTemp, "%02d", stTime.tm_min);	nfui_nflabel_set_text((NFLABEL*)(nftl->fields[NFTL_MIN]),	strTemp);
	g_sprintf(strTemp, "%02d", stTime.tm_sec);	nfui_nflabel_set_text((NFLABEL*)(nftl->fields[NFTL_SEC]),	strTemp);

	nfui_nflabel_set_text((NFLABEL*)(nftl->fields[NFTL_APM]), (stTime.tm_hour<12)? "AM":"PM");
}

static void prvFillIndex(NFTIMELABEL *nftl)
{
	guint i = 0;

	if(nftl->date_format != NFTL_DF_HIDE)
	{
		if(nftl->date_format == NFTL_DF_YMD)
		{
			nftl->idx[i++] = NFTL_YEAR;
			nftl->idx[i++] = NFTL_MONTH;
			nftl->idx[i++] = NFTL_DAY;
		}
		else if(nftl->date_format == NFTL_DF_MDY)
		{
			nftl->idx[i++] = NFTL_MONTH;
			nftl->idx[i++] = NFTL_DAY;
			nftl->idx[i++] = NFTL_YEAR;
		}
		else if(nftl->date_format == NFTL_DF_DMY)
		{
			nftl->idx[i++] = NFTL_DAY;
			nftl->idx[i++] = NFTL_MONTH;
			nftl->idx[i++] = NFTL_YEAR;
		}
	}

	if(nftl->time_mode != NFTL_TM_HIDE)
	{
		nftl->idx[i++] = NFTL_HOUR;
		nftl->idx[i++] = NFTL_MIN;
		nftl->idx[i++] = NFTL_SEC;

		if(nftl->time_mode == NFTL_TM_12H)
		{
			nftl->idx[i++] = NFTL_APM;
		}
	}

	nftl->idx[NFTL_FIELDS] = i;
}


static void prvSetPosition(NFTIMELABEL *nftl)
{
	guint tot_len = 0;
	guint dig_pix = 0; 
	guint tmp_dig_pix = 0;
	guint pos_x;

	guint i;
	guint num[4];

	GdkDrawable *drawable;

	drawable = nfui_nfobject_get_window(nftl);

	for (i=0; i<10; i++)
	{
		sprintf(num, "%d", i);
		tmp_dig_pix = nfutil_string_width(0, drawable, nftl->pango_font, num, NORMAL_SPACING);

		if (tmp_dig_pix > dig_pix)
			dig_pix = tmp_dig_pix;
	}

	if(nftl->time_mode == NFTL_TM_12H)
	{
		tot_len = nfutil_string_width(0, drawable, nftl->pango_font, "AM", NORMAL_SPACING);
	}

	if((nftl->date_format != NFTL_DF_HIDE) && (nftl->time_mode != NFTL_TM_HIDE))
	{
		tot_len += dig_pix*14;
		tot_len += nfutil_string_width(0, drawable, nftl->pango_font, "-", NORMAL_SPACING)*2;
		tot_len += nfutil_string_width(0, drawable, nftl->pango_font, ":", NORMAL_SPACING)*2;
		tot_len += nfutil_string_width(0, drawable, nftl->pango_font, " ", NORMAL_SPACING);
	}
	else if((nftl->date_format != NFTL_DF_HIDE) && (nftl->time_mode == NFTL_TM_HIDE))
	{
		tot_len += dig_pix*8;
		tot_len += nfutil_string_width(0, drawable, nftl->pango_font, "-", NORMAL_SPACING)*2;
	}
	else if((nftl->date_format == NFTL_DF_HIDE) && (nftl->time_mode != NFTL_TM_HIDE))
	{
		tot_len += dig_pix*6;
		tot_len += nfutil_string_width(0, drawable, nftl->pango_font, "-", NORMAL_SPACING)*2;
	}


	pos_x = nftl->fixed.object.width;

	if(pos_x < tot_len)	pos_x = 0;
	else	pos_x -= tot_len;

	pos_x /= 2;

	if(nftl->date_format == NFTL_DF_YMD)
	{
		nfui_nfobject_move(nftl->fields[NFTL_YEAR], pos_x, 2);		pos_x += dig_pix*4;
		nfui_nfobject_move(nftl->deco[0], pos_x, 2);				pos_x += nfutil_string_width(0, drawable, nftl->pango_font, "-", NORMAL_SPACING);
		nfui_nfobject_move(nftl->fields[NFTL_MONTH], pos_x, 2);		pos_x += dig_pix*2;
		nfui_nfobject_move(nftl->deco[1], pos_x, 2);				pos_x += nfutil_string_width(0, drawable, nftl->pango_font, "-", NORMAL_SPACING);
		nfui_nfobject_move(nftl->fields[NFTL_DAY], pos_x, 2);		pos_x += dig_pix*2;
	}
	else if(nftl->date_format == NFTL_DF_MDY)
	{
		nfui_nfobject_move(nftl->fields[NFTL_MONTH], pos_x, 2);		pos_x += dig_pix*2;
		nfui_nfobject_move(nftl->deco[0], pos_x, 2);				pos_x += nfutil_string_width(0, drawable, nftl->pango_font, "-", NORMAL_SPACING);
		nfui_nfobject_move(nftl->fields[NFTL_DAY], pos_x, 2);		pos_x += dig_pix*2;
		nfui_nfobject_move(nftl->deco[1], pos_x, 2);				pos_x += nfutil_string_width(0, drawable, nftl->pango_font, "-", NORMAL_SPACING);
		nfui_nfobject_move(nftl->fields[NFTL_YEAR], pos_x, 2);		pos_x += dig_pix*4;
	}
	else if(nftl->date_format == NFTL_DF_DMY)
	{
		nfui_nfobject_move(nftl->fields[NFTL_DAY], pos_x, 2);		pos_x += dig_pix*2;
		nfui_nfobject_move(nftl->deco[0], pos_x, 2);				pos_x += nfutil_string_width(0, drawable, nftl->pango_font, "-", NORMAL_SPACING);
		nfui_nfobject_move(nftl->fields[NFTL_MONTH], pos_x, 2);		pos_x += dig_pix*2;
		nfui_nfobject_move(nftl->deco[1], pos_x, 2);				pos_x += nfutil_string_width(0, drawable, nftl->pango_font, "-", NORMAL_SPACING);
		nfui_nfobject_move(nftl->fields[NFTL_YEAR], pos_x, 2);		pos_x += dig_pix*4;
	}

	if(nftl->date_format != NFTL_DF_HIDE)
		pos_x += nfutil_string_width(0, drawable, nftl->pango_font, " ", NORMAL_SPACING);

	if(nftl->time_mode != NFTL_TM_HIDE)
	{
		nfui_nfobject_move(nftl->fields[NFTL_HOUR], pos_x, 2);		pos_x += dig_pix*2;
		nfui_nfobject_move(nftl->deco[2], pos_x, 2);				pos_x += nfutil_string_width(0, drawable, nftl->pango_font, ":", NORMAL_SPACING);
		nfui_nfobject_move(nftl->fields[NFTL_MIN], pos_x, 2);		pos_x += dig_pix*2;
		nfui_nfobject_move(nftl->deco[3], pos_x, 2);				pos_x += nfutil_string_width(0, drawable, nftl->pango_font, ":", NORMAL_SPACING);
		nfui_nfobject_move(nftl->fields[NFTL_SEC], pos_x, 2);		pos_x += dig_pix*2;

		if(nftl->time_mode == NFTL_TM_12H)
		{
			pos_x += nfutil_string_width(0, drawable, nftl->pango_font, " ", NORMAL_SPACING);
			nfui_nfobject_move(nftl->fields[NFTL_APM], pos_x, 2);
		}
	}

	nfui_nfobject_set_size(nftl->fields[NFTL_YEAR], dig_pix*4, nftl->fixed.object.height-4);
	nfui_nfobject_set_size(nftl->fields[NFTL_MONTH], dig_pix*2, nftl->fixed.object.height-4);
	nfui_nfobject_set_size(nftl->fields[NFTL_DAY], dig_pix*2, nftl->fixed.object.height-4);
	nfui_nfobject_set_size(nftl->fields[NFTL_HOUR], dig_pix*2, nftl->fixed.object.height-4);
	nfui_nfobject_set_size(nftl->fields[NFTL_MIN], dig_pix*2, nftl->fixed.object.height-4);
	nfui_nfobject_set_size(nftl->fields[NFTL_SEC], dig_pix*2, nftl->fixed.object.height-4);
	nfui_nfobject_set_size(nftl->fields[NFTL_APM], (guint)nfutil_string_width(0, drawable, nftl->pango_font, "AM", NORMAL_SPACING), (guint)(nftl->fixed.object.height-4));

	nfui_nfobject_set_size(nftl->deco[0], (guint)nfutil_string_width(0, drawable, nftl->pango_font, "-", NORMAL_SPACING), nftl->fixed.object.height-4);
	nfui_nfobject_set_size(nftl->deco[1], (guint)nfutil_string_width(0, drawable, nftl->pango_font, "-", NORMAL_SPACING), nftl->fixed.object.height-4);
	nfui_nfobject_set_size(nftl->deco[2], (guint)nfutil_string_width(0, drawable, nftl->pango_font, ":", NORMAL_SPACING), nftl->fixed.object.height-4);
	nfui_nfobject_set_size(nftl->deco[3], (guint)nfutil_string_width(0, drawable, nftl->pango_font, ":", NORMAL_SPACING), nftl->fixed.object.height-4);

} 

static gboolean nftimelabel_event_handler(NFTIMELABEL *nftl, GdkEvent *event, gpointer data)
{
	switch(event->type)
	{
		case GDK_EXPOSE:
		{
			GdkDrawable *drawable;
			GdkGC *gc;
			guint x, y, w, h;

			drawable = nfui_nfobject_get_window((NFOBJECT*)nftl);
			gc = nfui_nfobject_get_gc((NFOBJECT*)nftl);

			nfui_nfobject_get_offset((NFOBJECT*)nftl, &x, &y);
			w = ((NFOBJECT*)nftl)->width;
			h = ((NFOBJECT*)nftl)->height;

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(nftl->bg_color));
			gdk_draw_rectangle(drawable, gc, TRUE, (gint)x, (gint)y, (gint)w, (gint)h);
			nfui_nfobject_gc_unref(gc);

			if(nftl->fixed.object.kfocus == NFOBJECT_FOCUS) 
				prvDrawOutLines((NFOBJECT*)nftl);
		}
		break;

		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
 			break;

		case GDK_ENTER_NOTIFY:
		case GDK_LEAVE_NOTIFY:
			if(nfui_nfobject_is_shown((NFOBJECT*)nftl))
				nfui_signal_emit((NFOBJECT*)nftl, GDK_EXPOSE, TRUE);
				
		case GDK_BUTTON_RELEASE:
		case NFEVENT_KEYPAD_RELEASE:
		case NFEVENT_REMOCON_RELEASE: 
			break;

		case GDK_DELETE:
 			break;

		default:
			break;

	}

	return FALSE;
}

NFTIMELABEL *nfui_nftimelabel_new()
{
	NFTIMELABEL *nftl;
	guint x, y;
	guint i;


	nftl = (NFTIMELABEL*)imalloc(sizeof(NFTIMELABEL));
	if(!nftl)	return NULL;

	nfui_nfobject_init((NFOBJECT*)nftl);

	nftl->fixed.object.width = 210; //142;
	nftl->fixed.object.height = 22;
	nftl->fixed.object.type = NFOBJECT_TYPE_NFTIMELABEL;
	nftl->fixed.object.use_focus = NFOBJECT_FOCUS_ON;
	nftl->fixed.object.default_event_handler = nftimelabel_event_handler;
	nftl->use_cashing = 1;

	g_get_current_time(&(nftl->tvTime));
	nftl->tvTime.tv_usec = 0;
	memcpy(&nftl->pre_tvTime, &nftl->tvTime, sizeof(GTimeVal));

	memset(&(nftl->upper_limit), 0, sizeof(GTimeVal));
	memset(&(nftl->lower_limit), 0, sizeof(GTimeVal));

	nftl->upper_limit.tv_sec = NFTL_DEF_UPPER_TIMELIMIT; 
	nftl->lower_limit.tv_sec = NFTL_DEF_LOWER_TIMELIMIT;

	nftl->date_lock = 0;
	nftl->date_format = NFTL_DF_YMD;
	nftl->time_mode = NFTL_TM_24H;
 
	strncpy(nftl->font_name, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), sizeof(nftl->font_name));
	nftl->pango_font = nftl->font_name;

	nftl->bg_color = -1;
	nftl->fg_color = COLOR_IDX(139);

	for(i=0; i<4; i++)
	{
		if(i/2 == 0)	nftl->deco[i] = nfui_nflabel_new_with_pango_font("-", nftl->pango_font, nftl->fg_color);
		else			nftl->deco[i] = nfui_nflabel_new_with_pango_font(":", nftl->pango_font, nftl->fg_color);
    
		nfui_nflabel_set_spacing((NFLABEL*)(nftl->deco[i]), nftl->spacing_type);
		nfui_nflabel_modify_fg(nftl->deco[i], nftl->fg_color);
		nfui_nflabel_set_drawing_outline((NFLABEL*)(nftl->deco[i]), FALSE);
		nfui_nfobject_modify_bg(nftl->deco[i], NFOBJECT_STATE_NORMAL, nftl->bg_color);	
		nfui_nfobject_use_tooltip(nftl->deco[i], FALSE);

		nfui_nfobject_use_focus(nftl->deco[i], NFOBJECT_FOCUS_OFF);
		nfui_nffixed_put((NFFIXED*)nftl, nftl->deco[i], 0, 0);

		nfui_nfobject_show(nftl->deco[i]);
	}


	for(i=NFTL_YEAR; i<NFTL_FIELDS; i++)
	{
		nftl->fields[i] = nfui_nflabel_new_with_pango_font("", nftl->pango_font, nftl->fg_color);
   		nfui_nflabel_set_spacing((NFLABEL*)(nftl->fields[i]), nftl->spacing_type);
		nfui_nflabel_set_drawing_outline((NFLABEL*)(nftl->fields[i]), FALSE);
		nfui_nflabel_modify_fg(nftl->fields[i], nftl->fg_color);
		nfui_nfobject_modify_bg(nftl->fields[i], NFOBJECT_STATE_NORMAL, nftl->bg_color);
		nfui_nfobject_use_tooltip(nftl->fields[i], FALSE);

		nfui_nffixed_put((NFFIXED*)nftl, nftl->fields[i], 0, 0);

		nfui_nfobject_show(nftl->fields[i]);
	}

	nfui_nftimelabel_set_mode(nftl, nftl->date_format, nftl->time_mode);

	return nftl;
}

void nfui_nftimelabel_get_datetime(NFTIMELABEL *nftl, GTimeVal *tv)
{
	if(nftl->fixed.object.type != NFOBJECT_TYPE_NFTIMELABEL)
	{
		return;
	}

	*tv = nftl->tvTime;
}

void nfui_nftimelabel_set_datetime(NFTIMELABEL *nftl, GTimeVal *tv)
{
	if(nftl->fixed.object.type != NFOBJECT_TYPE_NFTIMELABEL)
	{
		return;
	}

	nftl->tvTime = *tv;
	memcpy(&nftl->pre_tvTime, &nftl->tvTime, sizeof(GTimeVal));

	prvRefreshDateTime(nftl);
}

void nfui_nftimelabel_set_datetime_expose(NFTIMELABEL *nftl, GTimeVal *tv)
{
    if(nftl->fixed.object.type != NFOBJECT_TYPE_NFTIMELABEL)
    {
        return ;
    }

    nftl->tvTime = *tv;
    memcpy(&nftl->pre_tvTime, &nftl->tvTime, sizeof(GTimeVal));
    
    prvRefreshDateTime_expose(nftl);
}

void nfui_nftimelabel_set_mode(NFTIMELABEL *nftl, nftl_df_type date_format, nftl_time_mode tm)
{
	guint i;

	if(nftl->fixed.object.type != NFOBJECT_TYPE_NFTIMELABEL)
	{
		return;
	}

	if(date_format == NFTL_DF_HIDE && tm == NFTL_TM_HIDE)
	{
		g_message("Both Date and time can not bo hide..");
		return;
	}

	nftl->date_format = date_format;
	nftl->time_mode = tm;

	if(tm == NFTL_TM_24H)		nfui_nfobject_hide(nftl->fields[NFTL_APM]);
	else if(tm == NFTL_TM_12H)	nfui_nfobject_show(nftl->fields[NFTL_APM]);


	if((date_format != NFTL_DF_HIDE) && (tm != NFTL_TM_HIDE))
	{
		for(i=NFTL_YEAR; i<=NFTL_SEC; i++)
		{
			nfui_nfobject_show(nftl->fields[i]);
		}
		
		nfui_nfobject_show(nftl->deco[0]);
		nfui_nfobject_show(nftl->deco[1]);
		nfui_nfobject_show(nftl->deco[2]);
		nfui_nfobject_show(nftl->deco[3]);

	}
	else if((date_format != NFTL_DF_HIDE) && (tm == NFTL_TM_HIDE))
	{
		for(i=NFTL_YEAR; i<=NFTL_DAY; i++)
		{
			nfui_nfobject_show(nftl->fields[i]);
		}

		for(i=NFTL_HOUR; i<=NFTL_APM; i++)
		{
			nfui_nfobject_hide(nftl->fields[i]);
		}

		nfui_nfobject_show(nftl->deco[0]);
		nfui_nfobject_show(nftl->deco[1]);
		nfui_nfobject_hide(nftl->deco[2]);
		nfui_nfobject_hide(nftl->deco[3]);

	}
	else if((date_format == NFTL_DF_HIDE) && (tm != NFTL_TM_HIDE))
	{
		for(i=NFTL_YEAR; i<=NFTL_DAY; i++)
		{
			nfui_nfobject_hide(nftl->fields[i]);
		}

		for(i=NFTL_HOUR; i<=NFTL_SEC; i++)
		{
			nfui_nfobject_show(nftl->fields[i]);
		}

		nfui_nfobject_hide(nftl->deco[0]);
		nfui_nfobject_hide(nftl->deco[1]);
		nfui_nfobject_show(nftl->deco[2]);
		nfui_nfobject_show(nftl->deco[3]);
	}

	prvFillIndex(nftl);
	prvSetPosition(nftl);
	prvRefreshDateTime(nftl);
}

void nfui_nftimelabel_set_size(NFTIMELABEL *nftl, guint width, guint height)
{
	if(nftl->fixed.object.type != NFOBJECT_TYPE_NFTIMELABEL)
	{
		return;
	}

	nftl->fixed.object.width = width;
	nftl->fixed.object.height = height;

	prvSetPosition(nftl);
}

void nfui_nftimelabel_set_pango_font(NFTIMELABEL *nftl, const gchar *pfont)
{
	guint i = 0;

	if(nftl->fixed.object.type != NFOBJECT_TYPE_NFTIMELABEL)
	{
		return;
	}

//	strncpy(nftl->pango_font, pfont, sizeof(nftl->pango_font));

	if (pfont) {
		if (nffont_is_system_font(pfont))
			nftl->pango_font = pfont;
		else {
			strncpy(nftl->font_name, pfont, sizeof(nftl->font_name));
			nftl->pango_font = nftl->font_name;
		}
	}

	for(i=NFTL_YEAR; i<NFTL_FIELDS; i++)
	{
		nfui_nflabel_set_pango_font((NFLABEL*)nftl->fields[i], nftl->pango_font, nftl->fg_color);
		nfui_nflabel_set_spacing((NFLABEL*)(nftl->fields[i]), nftl->spacing_type);
	}

	for(i=0; i<4; i++)
	{
	  nfui_nflabel_set_pango_font((NFLABEL*)nftl->deco[i], nftl->pango_font, nftl->fg_color);
	  nfui_nflabel_set_spacing((NFLABEL*)(nftl->deco[i]), nftl->spacing_type);
	}
}

void nfui_nftimelabel_use_pango_cashing(NFTIMELABEL *nftl, gint cashing, gchar *key)
{
	gint i;

	if(nftl->fixed.object.type != NFOBJECT_TYPE_NFTIMELABEL)
	{
		return;
	}

	if(cashing)	nftl->use_cashing = 1;
	else		nftl->use_cashing = 0;

	if(key && strlen(key)>0)
		strncpy(nftl->cashing_key, key, sizeof(nftl->cashing_key));
	else
		memset(nftl->cashing_key, 0, sizeof(nftl->cashing_key));

	for(i=NFTL_YEAR; i<NFTL_FIELDS; i++)
		nfui_nflabel_use_pango_cashing(nftl->fields[i], cashing, key);
}

void nfui_nftimelabel_set_fg_color(NFTIMELABEL *nftl, guint fg_index)
{
	guint i;

	if(nftl->fixed.object.type != NFOBJECT_TYPE_NFTIMELABEL)
	{
		return;
	}

	nftl->fg_color = fg_index;

	for(i=0; i<4; i++)
		nfui_nflabel_modify_fg(nftl->deco[i], nftl->fg_color);

	for(i=NFTL_YEAR; i<NFTL_FIELDS; i++)
		nfui_nflabel_modify_fg(nftl->fields[i], nftl->fg_color);
}

void nfui_nftimelabel_set_bg_color(NFTIMELABEL *nftl, guint bg_index)
{
	guint i;

	if(nftl->fixed.object.type != NFOBJECT_TYPE_NFTIMELABEL)
	{
		return;
	}

	nftl->bg_color = bg_index;

	for(i=0; i<4; i++)
		nfui_nfobject_modify_bg(nftl->deco[i], NFOBJECT_STATE_NORMAL, nftl->bg_color);

	for(i=NFTL_YEAR; i<NFTL_FIELDS; i++)
		nfui_nfobject_modify_bg(nftl->fields[i], NFOBJECT_STATE_NORMAL, nftl->bg_color);
}

void nfui_nftimelabel_set_date_lock(NFTIMELABEL *nftl, gboolean lock)
{
	if(!nftl)	return;

	if(nftl->fixed.object.type != NFOBJECT_TYPE_NFTIMELABEL)
	{
		return;
	}

	if(lock)	nftl->date_lock = 1;
	else		nftl->date_lock = 0;
}

void nfui_nftimelabel_set_limit(NFTIMELABEL *nftl, GTimeVal *lower, GTimeVal *upper)
{
	if(nftl->fixed.object.type != NFOBJECT_TYPE_NFTIMELABEL)
	{
		return;
	}

	if(lower)	nftl->lower_limit = *lower;
	else		nftl->lower_limit.tv_sec = NFTL_DEF_LOWER_TIMELIMIT;

	if(upper)	nftl->upper_limit = *upper;
	else		nftl->upper_limit.tv_sec = NFTL_DEF_UPPER_TIMELIMIT;

	nftl->lower_limit.tv_usec = 0;
	nftl->upper_limit.tv_usec = 0;
}

void nfui_nftimelabel_refresh_datetime(NFTIMELABEL *nftl)
{
	prvRefreshDateTime(nftl);
}


