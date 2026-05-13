#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nftimespin.h"
#include "objects/nfcheckbutton.h"
#include "objects/nflistbox.h"
#include "objects/nfspinbutton.h"
#include "objects/nfbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nfthumbnail.h"
#include "objects/cw_calendar.h"

#include "vw_internal.h"
#include "vw_live_statusbar.h"
#include "vw_set_date_time.h"

#include "vw_search_main.h"
#include "vw_search_by_thumbnail.h"

#include "viewers/objects/ixtimeline.h"
#include "stm.h"
#include "dtf.h"
#include "uxm.h"
#include "var.h"
#include "ix_mem.h"


#define	SBTH_LABEL_HEIGHT1				(40)
#define	SBTH_LABEL_HEIGHT2				(26)


// FIXED1, FIXED2, (MIN FIXED / HOURS FIXED / MON FIXED)
#define SBTH_FIXED1_X					(0)
#define SBTH_FIXED1_Y					(0)
#define SBTH_FIXED1_W					(462)
#define SBTH_FIXED1_H					(903)

#define SBTH_FIXED2_X					(SBTH_FIXED1_X + SBTH_FIXED1_W)
#define SBTH_FIXED2_Y					(SBTH_FIXED1_Y)
#define SBTH_FIXED2_W					(1394)
#define SBTH_FIXED2_H					(106)

#define SBTH_MIN_FIXED_X				(SBTH_FIXED2_X)
#define SBTH_MIN_FIXED_Y				(SBTH_FIXED2_Y+SBTH_FIXED2_H)
#define SBTH_MIN_FIXED_W				(SBTH_FIXED2_W)
#define SBTH_MIN_FIXED_H				(797)

#define SBTH_HOUR_FIXED_X				(SBTH_FIXED2_X)
#define SBTH_HOUR_FIXED_Y				(SBTH_FIXED2_Y+SBTH_FIXED2_H)
#define SBTH_HOUR_FIXED_W				(SBTH_FIXED2_W)
#define SBTH_HOUR_FIXED_H				(797)

#define SBTH_DAY_FIXED_X				(SBTH_FIXED2_X)
#define SBTH_DAY_FIXED_Y				(SBTH_FIXED2_Y+SBTH_FIXED2_H)
#define SBTH_DAY_FIXED_W				(SBTH_FIXED2_W)
#define SBTH_DAY_FIXED_H				(797)


// CALENDAR FIXED
#define SBTH_CAL_FIXED_X				(13)
#define SBTH_CAL_FIXED_Y				(34)
#define SBTH_CAL_FIXED_W				(410)
#define SBTH_CAL_FIXED_H				(380)

#define SBTH_CAL_BUTTON_SIZE_W			(130)
#define SBTH_CAL_BUTTON_GAP				(46)

#define SBTH_CAL_FIRST_BUTTON_X			(51)
#define SBTH_CAL_FIRST_BUTTON_Y			(333)

#define SBTH_CAL_LAST_BUTTON_X			(SBTH_CAL_FIRST_BUTTON_X+SBTH_CAL_BUTTON_SIZE_W+SBTH_CAL_BUTTON_GAP)
#define SBTH_CAL_LAST_BUTTON_Y			(SBTH_CAL_FIRST_BUTTON_Y)


// CHANNEL, DATE/TIME, INTERVAL
#define	SBTH_TABLE1_COLUMNS				(2)
#define	SBTH_TABLE1_ROWS				(3)

#define	SBTH_TABLE1_GAP_Y				(2)

#define	SBTH_TABLE1_WIDTH				(410)
#define	SBTH_TABLE1_HEIGHT				(SBTH_LABEL_HEIGHT1*SBTH_TABLE1_ROWS + SBTH_TABLE1_GAP_Y*(SBTH_TABLE1_ROWS-1))

#define	SBTH_TABLE1_X					(SBTH_CAL_FIXED_X)
#define	SBTH_TABLE1_Y					(SBTH_CAL_FIXED_Y + SBTH_CAL_FIXED_H + 8)

#define	SBTH_TABLE1_COLUMN1_W			(150)
#define	SBTH_TABLE1_COLUMN2_W			(SBTH_TABLE1_WIDTH - SBTH_TABLE1_COLUMN1_W)


// PREVIEW
#define SBTH_PREVIEW_LABEL_X			(SBTH_CAL_FIXED_X+3)
#define SBTH_PREVIEW_LABEL_Y			(SBTH_TABLE1_Y + SBTH_TABLE1_HEIGHT + 24)
#define SBTH_PREVIEW_LABEL_WIDTH		(220)

#define SBTH_PREVIEW_VIDEO_X			(SBTH_CAL_FIXED_X+3)
#define SBTH_PREVIEW_VIDEO_Y			(SBTH_PREVIEW_LABEL_Y+SBTH_LABEL_HEIGHT2+3) //+2
#define SBTH_PREVIEW_VIDEO_W			(408) // 410
#define SBTH_PREVIEW_VIDEO_H			(228) //230

#define SBTH_PREVIEW_STATUS_W			(200)
#define SBTH_PREVIEW_STATUS_H			(30)
#define SBTH_PREVIEW_STATUS_X			(SBTH_PREVIEW_VIDEO_X + (SBTH_PREVIEW_VIDEO_W-SBTH_PREVIEW_STATUS_W)/2)
#define SBTH_PREVIEW_STATUS_Y			(SBTH_PREVIEW_VIDEO_Y + (SBTH_PREVIEW_VIDEO_H-SBTH_PREVIEW_STATUS_H)/2)

#define SBTH_PREVIEW_TIME_X				(SBTH_PREVIEW_VIDEO_X)
#define SBTH_PREVIEW_TIME_Y				(SBTH_PREVIEW_VIDEO_Y+SBTH_PREVIEW_VIDEO_H+4)


// TIMELINE
#define TML_X							(24)
#define TML_Y							(34)
#define TML_W							(1350)
#define HEIGHT_RULER					(25)
#define HEIGHT_CH						(18)

//TIMELINE CH BUTTON
#define SBT_TML_CH_BTN_X				(1)
#define SBT_TML_CH_BTN_Y				(83)
#define SBT_TML_CH_BTN_H				(18)

//TIME LINE RECORD REASON
#define SBTH_TL_REC_BOX_GAP_X			(6)

#define SBTH_TL_REC_BOX_Y				(7)
#define SBTH_TL_REC_BOX_W				(22)
#define SBTH_TL_REC_BOX_H				(22)

#define SBTH_TL_REC_TEXT_PANIC_W		(174)//(84)
#define SBTH_TL_REC_TEXT_PANIC_X		(TML_X + TML_W - SBTH_TL_REC_TEXT_PANIC_W)

#define SBTH_TL_REC_BOX_PANIC_X			(SBTH_TL_REC_TEXT_PANIC_X - SBTH_TL_REC_BOX_GAP_X - SBTH_TL_REC_BOX_W)
#define SBTH_TL_REC_BOX_PANIC_Y			(SBTH_TL_REC_BOX_Y)

#define SBTH_TL_REC_TEXT_MOT_W			(174)//(105)
#define SBTH_TL_REC_TEXT_MOT_X			(SBTH_TL_REC_BOX_PANIC_X - SBTH_TL_REC_TEXT_MOT_W)

#define SBTH_TL_REC_BOX_MOT_X			(SBTH_TL_REC_TEXT_MOT_X - SBTH_TL_REC_BOX_GAP_X - SBTH_TL_REC_BOX_W)
#define SBTH_TL_REC_BOX_MOT_Y			(SBTH_TL_REC_BOX_Y)

#define SBTH_TL_REC_TEXT_ALARM_W		(174)//(94)
#define SBTH_TL_REC_TEXT_ALARM_X		(SBTH_TL_REC_BOX_MOT_X - SBTH_TL_REC_TEXT_ALARM_W)

#define SBTH_TL_REC_BOX_ALARM_X			(SBTH_TL_REC_TEXT_ALARM_X - SBTH_TL_REC_BOX_GAP_X - SBTH_TL_REC_BOX_W)
#define SBTH_TL_REC_BOX_ALARM_Y			(SBTH_TL_REC_BOX_Y)

#define SBTH_TL_REC_TEXT_TIME_W			(174)//(153)
#define SBTH_TL_REC_TEXT_TIME_X			(SBTH_TL_REC_BOX_ALARM_X - SBTH_TL_REC_TEXT_TIME_W)

#define SBTH_TL_REC_BOX_TIME_X			(SBTH_TL_REC_TEXT_TIME_X - SBTH_TL_REC_BOX_GAP_X - SBTH_TL_REC_BOX_W)
#define SBTH_TL_REC_BOX_TIME_Y			(SBTH_TL_REC_BOX_Y)

#define SBTH_TL_REC_TEXT_PRE_W			(174)
#define SBTH_TL_REC_TEXT_PRE_X			(SBTH_TL_REC_BOX_TIME_X - SBTH_TL_REC_TEXT_PRE_W)

#define SBTH_TL_REC_BOX_PRE_X			(SBTH_TL_REC_TEXT_PRE_X - SBTH_TL_REC_BOX_GAP_X - SBTH_TL_REC_BOX_W)
#define SBTH_TL_REC_BOX_PRE_Y			(SBTH_TL_REC_BOX_Y)

/*
// TIMELINE PREV/NEXT
#define TML_BTN_PREV_X					(1323)
#define TML_BTN_PREV_Y					(TML_Y + HEIGHT_RULER + HEIGHT_CH + 6)
#define TML_BTN_PREV_W					27
#define TML_BTN_PREV_H					27

#define TML_BTN_NEXT_X					(TML_BTN_PREV_X + TML_BTN_PREV_W + 6)
#define TML_BTN_NEXT_Y					TML_BTN_PREV_Y
#define TML_BTN_NEXT_W					TML_BTN_PREV_W
#define TML_BTN_NEXT_H					TML_BTN_PREV_H
*/

#define THUMB_PIXEL_ALIGNMENT			(16)

// THUMBNAIL TITLE.
#define THUMB_TITLE_X					(24)
#define THUMB_TITLE_Y					(2)
#define THUMB_TITLE_W					(400)

// THUMBNAIL PICTURE - MINUTES.
#define THUMB_MIN_CELL_COL				(9)
#define THUMB_MIN_CELL_ROW				(7)
#define THUMB_MIN_CELL_MAX				(60)

#define THUMB_MIN_CELL_W				((THUMB_PIXEL_ALIGNMENT * 9) + 2)
#define THUMB_MIN_CELL_H				((THUMB_PIXEL_ALIGNMENT * 6) + 1) // 101
#define THUMB_MIN_DS_W					(162)	// for double scale
#define THUMB_MIN_DS_H					(117)	// for double scale

#define THUMB_MIN_CELL_COL_GAP			(4)
#define THUMB_MIN_CELL_ROW_GAP			(8) //6

#define THUMB_MIN_LABEL_X				(THUMB_TITLE_X)
#define THUMB_MIN_LABEL_Y				(THUMB_TITLE_Y + SBTH_LABEL_HEIGHT2 + 2)
#define THUMB_MIN_LABEL_W				(THUMB_MIN_CELL_W)
#define THUMB_MIN_LABEL_H				(17)

#define THUMB_MIN_PIC_X					(THUMB_MIN_LABEL_X)
#define THUMB_MIN_PIC_Y					(THUMB_MIN_LABEL_Y + THUMB_MIN_LABEL_H)
#define THUMB_MIN_PIC_W					(THUMB_MIN_CELL_W)
#define THUMB_MIN_PIC_H					(THUMB_MIN_CELL_H - THUMB_MIN_LABEL_H)


// THUMBNAIL PICTURE - HOURS.
#define THUMB_HOUR_CELL_COL				(6)
#define THUMB_HOUR_CELL_ROW				(5)
#define THUMB_HOUR_CELL_MAX				(25)

#define THUMB_HOUR_CELL_W				((THUMB_PIXEL_ALIGNMENT * 14) + 2)
#define THUMB_HOUR_CELL_H				((THUMB_PIXEL_ALIGNMENT * 9) + 1)

#define THUMB_HOUR_CELL_COL_GAP			(2) // 2
#define THUMB_HOUR_CELL_ROW_GAP			(6)

#define THUMB_HOUR_LABEL_X				(THUMB_TITLE_X)
#define THUMB_HOUR_LABEL_Y				(THUMB_TITLE_Y + SBTH_LABEL_HEIGHT2 + 2)
#define THUMB_HOUR_LABEL_W				(THUMB_HOUR_CELL_W)
#define THUMB_HOUR_LABEL_H				(17)

#define THUMB_HOUR_PIC_X				(THUMB_HOUR_LABEL_X)
#define THUMB_HOUR_PIC_Y				(THUMB_HOUR_LABEL_Y + THUMB_HOUR_LABEL_H)
#define THUMB_HOUR_PIC_W				(THUMB_HOUR_CELL_W)
#define THUMB_HOUR_PIC_H				(THUMB_HOUR_CELL_H - THUMB_HOUR_LABEL_H)


// THUMBNAIL PICTURE - DAYS.
#define THUMB_DAY_CELL_COL				(7)
#define THUMB_DAY_CELL_ROW				(6)
#define THUMB_DAY_CELL_MAX				(31)

#define THUMB_DAY_CELL_W				((THUMB_PIXEL_ALIGNMENT * 12) + 2)
#define THUMB_DAY_CELL_H				((THUMB_PIXEL_ALIGNMENT * 9) + 1)

#define THUMB_DAY_CELL_COL_GAP			(2) // 2
#define THUMB_DAY_CELL_ROW_GAP			(8) //7

#define THUMB_DAY_LABEL_X				(THUMB_TITLE_X)
#define THUMB_DAY_LABEL_Y				(THUMB_TITLE_Y + SBTH_LABEL_HEIGHT2 + 2)
#define THUMB_DAY_LABEL_W				(THUMB_DAY_CELL_W)
#define THUMB_DAY_LABEL_H				(17)

#define THUMB_DAY_PIC_X					(THUMB_DAY_LABEL_X)
#define THUMB_DAY_PIC_Y					(THUMB_DAY_LABEL_Y + THUMB_DAY_LABEL_H)
#define THUMB_DAY_PIC_W					(THUMB_DAY_CELL_W)
#define THUMB_DAY_PIC_H					(THUMB_DAY_CELL_H - THUMB_DAY_LABEL_H)

#define DATE_TIME_TEXT_LEN				(64)


////////////////////////////////////////////////////////////
//
// private data types
//


enum {
	SBTH_SET_CH = 0,
	SBTH_SET_DT,
	SBTH_SET_INV,
	NUM_SBTH_SET_MAX,
};

enum {
	THUMB_MODE_MIN = 0,
	THUMB_MODE_HOUR,
	THUMB_MODE_DAY,
	THUMB_MODE_MAX,
};


////////////////////////////////////////////////////////////
//
// private variables
//

static NFWINDOW *g_curwnd = 0;
static gchar g_recInfo[31];
static int g_need_update_preview = 0;

static NFOBJECT *dt_obj;
//static NFOBJECT *tml_ch_obj;
static NFOBJECT *g_cal_dir[2];
static NFOBJECT *g_cal;
static NFOBJECT *g_cal_text;
static NFOBJECT *g_first_btn;
static NFOBJECT *g_last_btn;

static NFOBJECT *g_prev_btn;
static NFOBJECT *g_next_btn;

static NFOBJECT *min_fixed;
static NFOBJECT *hour_fixed;
static NFOBJECT *day_fixed;

static gboolean g_init_thumb = TRUE;
static NFOBJECT *ch_obj = NULL;
static NFOBJECT *dt_obj = NULL;
static NFOBJECT *inv_obj = NULL;

static NFOBJECT *playbtn_obj = NULL;
static NFOBJECT *preview_obj = NULL;
static NFOBJECT *play_st_obj = NULL;
static NFOBJECT *play_time_obj = NULL;
static NFOBJECT *thumb_obj[THUMB_MODE_MAX][60];

static guchar min_image[THUMB_MIN_CELL_MAX][(THUMB_MIN_DS_H-17-1)*(THUMB_MIN_DS_W-2)*3] = {0,};
static guchar hour_image[THUMB_HOUR_CELL_MAX][(THUMB_HOUR_CELL_H-17-1)*(THUMB_HOUR_CELL_W-2)*3] = {0,};
static guchar day_image[THUMB_DAY_CELL_MAX][(THUMB_DAY_CELL_H-17-1)*(THUMB_DAY_CELL_W-2)*3] = {0,};

static time_t thumbnail_hour;
static time_t search_time;
static guint preview_timer_id;
static IXTIMELINE *tml;
static NFOBJECT *fixed2;
static NFOBJECT *fixed1;
static guint days[12] = {31,28,31,30,31,30,31,31,30,31,30,31};


////////////////////////////////////////////////////////////
//
// private functions
//

static gint _set_select_bg_color(NFOBJECT *obj)
{
	gint i, thumb_max_cnt;
	gint index;

	if (nfui_nfthumbnail_get_state((NFTHUMBNAIL*)obj) == THUMB_STATE_SELECT)
		return -1;

	index = nfui_combobox_get_cur_index(NF_COMBOBOX(inv_obj));

	if (index == THUMB_MODE_MIN)
		thumb_max_cnt = THUMB_MIN_CELL_MAX;
	else if (index == THUMB_MODE_HOUR)
		thumb_max_cnt = GPOINTER_TO_INT(nfui_nfobject_get_data(hour_fixed, "THUMB CNT"));
	else
		thumb_max_cnt = GPOINTER_TO_INT(nfui_nfobject_get_data(day_fixed, "THUMB CNT"));

	for (i = 0; i < thumb_max_cnt; i++)
	{
		if (nfui_nfthumbnail_get_state((NFTHUMBNAIL*)thumb_obj[index][i]) == THUMB_STATE_SELECT)
			nfui_nfthumbnail_set_state((NFTHUMBNAIL*)thumb_obj[index][i], THUMB_STATE_NORMAL);
	}

	nfui_nfthumbnail_set_state((NFTHUMBNAIL*)obj, THUMB_STATE_SELECT);

	return 0;
}

static gint _unset_select_bg_color()
{
	gint i, thumb_max_cnt;
	gint index;

	index = nfui_combobox_get_cur_index(NF_COMBOBOX(inv_obj));

	if (index == THUMB_MODE_MIN)
		thumb_max_cnt = THUMB_MIN_CELL_MAX;
	else if (index == THUMB_MODE_HOUR)
		thumb_max_cnt = GPOINTER_TO_INT(nfui_nfobject_get_data(hour_fixed, "THUMB CNT"));
	else
		thumb_max_cnt = GPOINTER_TO_INT(nfui_nfobject_get_data(day_fixed, "THUMB CNT"));

	for (i = 0; i < thumb_max_cnt; i++)
	{
		if (nfui_nfthumbnail_get_state((NFTHUMBNAIL*)thumb_obj[index][i]) == THUMB_STATE_SELECT)
			nfui_nfthumbnail_set_state((NFTHUMBNAIL*)thumb_obj[index][i], THUMB_STATE_NORMAL);
	}


	return 0;
}

static void _calc_hours_count(time_t time)
{
	time_t pre_t, post_t;
	gint year, mon, day;
	gint pre_isdst, post_isdst;
	guint cnt;

	dtf_get_local_day(time, &year, &mon, &day);
	post_t = ifn_get_gmt_from_local(year, mon, day, 0, 0, 0);
	pre_isdst = ifn_is_in_dst(post_t);

	post_t += (3600 * 24 - 1);
	post_isdst = ifn_is_in_dst(post_t);

	if ((pre_isdst == 0) && (post_isdst == 1))
		cnt = 23;
	else if ((pre_isdst == 1) && (post_isdst == 0))
		cnt = 25;
	else
		cnt = 24;

	nfui_nfobject_set_data(hour_fixed, "THUMB CNT",  GINT_TO_POINTER(cnt));
}

static void _calc_days_count(time_t time)
{
	guint i, year, mon;

	gint days[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

	dtf_get_local_day(time, &year, &mon, 0);

	if (mon == 2)
	{
		if((((year%4) == 0) && year%100 != 0) || (year%400 == 0))
		{
			days[mon-1]++;
		}
	}

	nfui_nfobject_set_data(day_fixed, "THUMB CNT",  GINT_TO_POINTER(days[mon-1]));
}

static void _preview_obj_on(gchar *buf)
{
	nfui_nflabel_set_text((NFLABEL*)preview_obj, buf);
	nfui_nfobject_modify_bg(preview_obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
	nfui_signal_emit(preview_obj, GDK_EXPOSE, TRUE);
}

static void _preview_obj_off(gchar *buf)
{
	nfui_nflabel_set_text((NFLABEL*)preview_obj, buf);
	nfui_nfobject_modify_bg(preview_obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(666));
	nfui_signal_emit(preview_obj, GDK_EXPOSE, TRUE);
}

static void _playback_button_enable()
{
	nfui_nfobject_enable(playbtn_obj);
	nfui_signal_emit(playbtn_obj, GDK_EXPOSE, FALSE);
}

static void _playback_button_disable()
{
	nfui_nfobject_disable(playbtn_obj);
	nfui_signal_emit(playbtn_obj, GDK_EXPOSE, FALSE);
}

static void _change_min_thumbnail_fixed()
{
	gint i;

	if (nfui_nfobject_is_shown(hour_fixed))
	{
		for (i = 0; i < THUMB_HOUR_CELL_MAX; i++)
			nfui_nfobject_hide(thumb_obj[THUMB_MODE_HOUR][i]);

		nfui_nfobject_hide(hour_fixed);
		nfui_signal_emit(hour_fixed, GDK_EXPOSE, TRUE);
	}

	if (nfui_nfobject_is_shown(day_fixed))
	{
		for (i = 0; i < THUMB_DAY_CELL_MAX; i++)
			nfui_nfobject_hide(thumb_obj[THUMB_MODE_DAY][i]);

		nfui_nfobject_hide(day_fixed);
		nfui_signal_emit(day_fixed, GDK_EXPOSE, TRUE);
	}

	if (!nfui_nfobject_is_shown(min_fixed))
	{
		for (i = 0; i < THUMB_MIN_CELL_MAX; i++)
		{
			nfui_nfobject_show(thumb_obj[THUMB_MODE_MIN][i]);
			nfui_nfobject_disable(thumb_obj[THUMB_MODE_MIN][i]);
		}

		nfui_nfobject_show(min_fixed);
		nfui_signal_emit(min_fixed, GDK_EXPOSE, TRUE);
	}
}

static void _change_hour_thumbnail_fixed()
{
	gint i, thumb_cnt;

	if (nfui_nfobject_is_shown(min_fixed))
	{
		for (i = 0; i < THUMB_MIN_CELL_MAX; i++)
			nfui_nfobject_hide(thumb_obj[THUMB_MODE_MIN][i]);

		nfui_nfobject_hide(min_fixed);
		nfui_signal_emit(min_fixed, GDK_EXPOSE, TRUE);
	}

	if (nfui_nfobject_is_shown(day_fixed))
	{
		for (i = 0; i < THUMB_DAY_CELL_MAX; i++)
			nfui_nfobject_hide(thumb_obj[THUMB_MODE_DAY][i]);

		nfui_nfobject_hide(day_fixed);
		nfui_signal_emit(day_fixed, GDK_EXPOSE, TRUE);
	}

	if (!nfui_nfobject_is_shown(hour_fixed))
	{
		thumb_cnt = GPOINTER_TO_INT(nfui_nfobject_get_data(hour_fixed, "THUMB CNT"));

		for (i = 0; i < thumb_cnt; i++)
		{
			nfui_nfobject_show(thumb_obj[THUMB_MODE_HOUR][i]);
			nfui_nfobject_disable(thumb_obj[THUMB_MODE_HOUR][i]);
		}

		nfui_nfobject_show(hour_fixed);
		nfui_signal_emit(hour_fixed, GDK_EXPOSE, TRUE);
	}
}

static void _change_day_thumbnail_fixed()
{
	gint i, thumb_cnt;

	if (nfui_nfobject_is_shown(min_fixed))
	{
		for (i = 0; i < THUMB_MIN_CELL_MAX; i++)
			nfui_nfobject_hide(thumb_obj[THUMB_MODE_MIN][i]);

		nfui_nfobject_hide(min_fixed);
		nfui_signal_emit(min_fixed, GDK_EXPOSE, TRUE);
	}

	if (nfui_nfobject_is_shown(hour_fixed))
	{
		for (i = 0; i < THUMB_HOUR_CELL_MAX; i++)
			nfui_nfobject_hide(thumb_obj[THUMB_MODE_HOUR][i]);

		nfui_nfobject_hide(hour_fixed);
		nfui_signal_emit(hour_fixed, GDK_EXPOSE, TRUE);
	}

	if (!nfui_nfobject_is_shown(day_fixed))
	{
		thumb_cnt = GPOINTER_TO_INT(nfui_nfobject_get_data(day_fixed, "THUMB CNT"));

		for (i = 0; i < thumb_cnt; i++)
		{
			nfui_nfobject_show(thumb_obj[THUMB_MODE_DAY][i]);
			nfui_nfobject_disable(thumb_obj[THUMB_MODE_DAY][i]);
		}

		nfui_nfobject_show(day_fixed);
		nfui_signal_emit(day_fixed, GDK_EXPOSE, TRUE);
	}
}

static void _reset_hours_thumb_count()
{
	gint i, thumb_cnt;
	gboolean fixed_update = FALSE;

	thumb_cnt = GPOINTER_TO_INT(nfui_nfobject_get_data(hour_fixed, "THUMB CNT"));

	for (i = 0; i < THUMB_HOUR_CELL_MAX; i++)
	{
		if (i < thumb_cnt)
		{
			if (!nfui_nfobject_is_shown(thumb_obj[THUMB_MODE_HOUR][i]))
			{
				nfui_nfobject_show(thumb_obj[THUMB_MODE_HOUR][i]);
				nfui_signal_emit(thumb_obj[THUMB_MODE_HOUR][i], GDK_EXPOSE, TRUE);
				fixed_update = TRUE;
			}
		}
		else
		{
			if (nfui_nfobject_is_shown(thumb_obj[THUMB_MODE_HOUR][i]))
			{
				nfui_nfobject_hide(thumb_obj[THUMB_MODE_HOUR][i]);
				nfui_signal_emit(thumb_obj[THUMB_MODE_HOUR][i], GDK_EXPOSE, TRUE);
				fixed_update = TRUE;
			}
		}
	}

	if (fixed_update)
	{
		nfui_signal_emit(hour_fixed, GDK_EXPOSE, TRUE);
		VW_Search_key_remake();
	}
}

static void _reset_days_thumb_count()
{
	gint i, thumb_cnt;
	gboolean fixed_update = FALSE;

	thumb_cnt = GPOINTER_TO_INT(nfui_nfobject_get_data(day_fixed, "THUMB CNT"));

	for (i = 0; i < THUMB_DAY_CELL_MAX; i++)
	{
		if (i < thumb_cnt)
		{
			if (!nfui_nfobject_is_shown(thumb_obj[THUMB_MODE_DAY][i]))
			{
				nfui_nfobject_show(thumb_obj[THUMB_MODE_DAY][i]);
				nfui_signal_emit(thumb_obj[THUMB_MODE_DAY][i], GDK_EXPOSE, TRUE);
				fixed_update = TRUE;
			}
		}
		else
		{
			if (nfui_nfobject_is_shown(thumb_obj[THUMB_MODE_DAY][i]))
			{
				nfui_nfobject_hide(thumb_obj[THUMB_MODE_DAY][i]);
				nfui_signal_emit(thumb_obj[THUMB_MODE_DAY][i], GDK_EXPOSE, TRUE);
				fixed_update = TRUE;
			}
		}
	}

	if (fixed_update)
	{
		nfui_signal_emit(day_fixed, GDK_EXPOSE, TRUE);
		VW_Search_key_remake();
	}
}

static void _set_interval_min_thumbnail()
{
	gint i, ch, is_dst;
	time_t s_time, e_time;
	time_t thm_t;
	int year, mon, day, hour;

	ch = nfui_combobox_get_cur_index(NF_COMBOBOX(ch_obj));

#if 1
	thm_t = (search_time/3600) * 3600;
#else
	dtf_get_local_day(search_time, &year, &mon, &day);
	dtf_get_local_hourmin(search_time, &hour, 0, 0);

	thm_t = ifn_get_gmt_from_local(year, mon, day, hour, 0, 0);
#endif

	s_time = thm_t + 1;
	e_time = s_time + 59;

	for (i = 0; i < THUMB_MIN_CELL_MAX; i++)
	{
		nfui_nfthumbnail_set_channel((NFTHUMBNAIL*)thumb_obj[THUMB_MODE_MIN][i], ch);
		nfui_nfthumbnail_set_period((NFTHUMBNAIL*)thumb_obj[THUMB_MODE_MIN][i], s_time, e_time);

		s_time += 60;
		e_time += 60;
	}
}

static void _set_interval_hour_thumbnail()
{
	gint i, ch;
	time_t s_time, e_time;
	time_t thm_t;
	int year, mon, day, hour, min, sec;
	gint thumb_cnt;

	thumb_cnt = GPOINTER_TO_INT(nfui_nfobject_get_data(hour_fixed, "THUMB CNT"));
	dtf_get_local_day(search_time, &year, &mon, &day);

	thm_t = ifn_get_gmt_from_local(year, mon, day, 0, 0, 0);

	s_time = thm_t + 1;
	e_time = s_time + 3599;

	ch = nfui_combobox_get_cur_index(NF_COMBOBOX(ch_obj));

	for (i = 0; i < thumb_cnt; i++)
	{
		nfui_nfthumbnail_set_channel((NFTHUMBNAIL*)thumb_obj[THUMB_MODE_HOUR][i], ch);
		nfui_nfthumbnail_set_period((NFTHUMBNAIL*)thumb_obj[THUMB_MODE_HOUR][i], s_time, e_time);

		s_time += 3600;
		e_time += 3600;
	}
}

static void _set_interval_day_thumbnail()
{
	gint i, ch;
	time_t s_time, e_time;
	time_t thm_t;
	int year, mon;
	gint thumb_cnt;

	thumb_cnt = GPOINTER_TO_INT(nfui_nfobject_get_data(day_fixed, "THUMB CNT"));
	dtf_get_local_day(search_time, &year, &mon, 0);

	ch = nfui_combobox_get_cur_index(NF_COMBOBOX(ch_obj));

	for (i = 0; i < thumb_cnt; i++)
	{
		thm_t = ifn_get_gmt_from_local(year, mon, i+1, 0, 0, 0);
		s_time = thm_t + 1;

		thm_t = ifn_get_gmt_from_local(year, mon, i+1, 23, 59, 59);
		e_time = thm_t + 1;

		nfui_nfthumbnail_set_channel((NFTHUMBNAIL*)thumb_obj[THUMB_MODE_DAY][i], ch);
		nfui_nfthumbnail_set_period((NFTHUMBNAIL*)thumb_obj[THUMB_MODE_DAY][i], s_time, e_time);
	}
}

static int _ready_min_thumbnail()
{
	NFOBJECT *rate_obj;
	gint i;
	gchar strBuf[10];

	rate_obj = (NFOBJECT*)nfui_nfobject_get_data(min_fixed, "CMPL RATE");
	memset(strBuf, 0x00, sizeof(strBuf));
	g_sprintf(strBuf, "(0 / %d)", THUMB_MIN_CELL_MAX);
	nfui_nflabel_set_text((NFLABEL*)rate_obj, strBuf);
	nfui_signal_emit(rate_obj, GDK_EXPOSE, TRUE);
	_unset_select_bg_color();
	_preview_obj_off("");

	for (i = 0; i < THUMB_MIN_CELL_MAX; i++)
	{
		if (!nfui_nfobject_is_disabled(thumb_obj[THUMB_MODE_MIN][i]))
		{
			nfui_nfobject_disable(thumb_obj[THUMB_MODE_MIN][i]);
			nfui_signal_emit(thumb_obj[THUMB_MODE_MIN][i], GDK_EXPOSE, TRUE);
		}
	}

	return 0;
}

static int _ready_hour_thumbnail()
{
	NFOBJECT *rate_obj;
	gint i;
	gchar strBuf[10];
	gint thumb_cnt;

	rate_obj = (NFOBJECT*)nfui_nfobject_get_data(hour_fixed, "CMPL RATE");
	thumb_cnt = GPOINTER_TO_INT(nfui_nfobject_get_data(hour_fixed, "THUMB CNT"));

	memset(strBuf, 0x00, sizeof(strBuf));
	g_sprintf(strBuf, "(0 / %d)", thumb_cnt);
	nfui_nflabel_set_text((NFLABEL*)rate_obj, strBuf);
	nfui_signal_emit(rate_obj, GDK_EXPOSE, TRUE);
	_unset_select_bg_color();
	_preview_obj_off("");

	for (i = 0; i < thumb_cnt; i++)
	{
		if (!nfui_nfobject_is_disabled(thumb_obj[THUMB_MODE_HOUR][i]))
		{
			nfui_nfobject_disable(thumb_obj[THUMB_MODE_HOUR][i]);
			nfui_signal_emit(thumb_obj[THUMB_MODE_HOUR][i], GDK_EXPOSE, TRUE);
		}
	}

	return 0;
}

static int _ready_day_thumbnail()
{
	NFOBJECT *rate_obj;
	gint i;
	gchar strBuf[10];
	gint thumb_cnt;

	rate_obj = (NFOBJECT*)nfui_nfobject_get_data(day_fixed, "CMPL RATE");
	thumb_cnt = GPOINTER_TO_INT(nfui_nfobject_get_data(day_fixed, "THUMB CNT"));

	memset(strBuf, 0x00, sizeof(strBuf));
	g_sprintf(strBuf, "(0 / %d)", thumb_cnt);
	nfui_nflabel_set_text((NFLABEL*)rate_obj, strBuf);
	nfui_signal_emit(rate_obj, GDK_EXPOSE, TRUE);
	_unset_select_bg_color();
	_preview_obj_off("");

	for (i = 0; i < thumb_cnt; i++)
	{
		if (!nfui_nfobject_is_disabled(thumb_obj[THUMB_MODE_DAY][i]))
		{
			nfui_nfobject_disable(thumb_obj[THUMB_MODE_DAY][i]);
			nfui_signal_emit(thumb_obj[THUMB_MODE_DAY][i], GDK_EXPOSE, TRUE);
		}
	}

	return 0;
}

static int _ready_thumbnail()
{
	int index;

	index = nfui_combobox_get_cur_index(NF_COMBOBOX(inv_obj));
	switch (index) {
		case THUMB_MODE_MIN:
			_ready_min_thumbnail();
			break;
		case THUMB_MODE_HOUR:
			_ready_hour_thumbnail();
			break;
		case THUMB_MODE_DAY:
			_ready_day_thumbnail();
			break;
	}
	_playback_button_disable();
	vsm_playback_preview_stop();
	return 0;
}

static int _get_thumbnail()
{
	int i, index;

	index = nfui_combobox_get_cur_index(NF_COMBOBOX(inv_obj));
	switch (index) {
		case THUMB_MODE_MIN:
			_set_interval_min_thumbnail();
			break;
		case THUMB_MODE_HOUR:
			_set_interval_hour_thumbnail();
			break;
		case THUMB_MODE_DAY:
			_set_interval_day_thumbnail();
			break;
	}

	nfui_nfthumbnail_get_image();
	return 0;
}

static gboolean _show_thumbnail(void *data)
{
	if (!nfui_nfwindow_find("SEARCH")) return FALSE;

	vsm_live_stop();
	nfui_nfthumbnail_image_open();
	_ready_thumbnail();
	_get_thumbnail();
	g_init_thumb = FALSE;

	return FALSE;
}

static NFOBJECT* _get_selected_thumbnail_obj()
{
	gint i, thumb_max_cnt;
	gint index;

	index = nfui_combobox_get_cur_index(NF_COMBOBOX(inv_obj));

	if (index == THUMB_MODE_MIN)
		thumb_max_cnt = THUMB_MIN_CELL_MAX;
	else if (index == THUMB_MODE_HOUR)
		thumb_max_cnt = GPOINTER_TO_INT(nfui_nfobject_get_data(hour_fixed, "THUMB CNT"));
	else
		thumb_max_cnt = GPOINTER_TO_INT(nfui_nfobject_get_data(day_fixed, "THUMB CNT"));

	for (i = 0; i < thumb_max_cnt; i++)
	{
		if (nfui_nfthumbnail_get_state((NFTHUMBNAIL*)thumb_obj[index][i]) == THUMB_STATE_SELECT)
			return thumb_obj[index][i];
	}

	return NULL;
}

static int _update_thumbnail_rate(NFOBJECT *obj, gint cmpl_cnt, gint max_cnt)
{
	gchar strBuf[10];

	memset(strBuf, 0x00, sizeof(strBuf));
	g_sprintf(strBuf, "(%d / %d)", cmpl_cnt, max_cnt);
	nfui_nflabel_set_text((NFLABEL*)obj, strBuf);
	nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
}

static int _update_thumbnail_info(NFOBJECT *obj, guint type)
{
	gchar time_str[64];

	time_t img_time;
	gboolean ret_val;

	memset(time_str, 0x00, sizeof(time_str));
	ret_val = nfui_nfthumbnail_get_image_time((NFTHUMBNAIL*)obj, &img_time);

	if (ret_val)
	{
		gint hour, min, sec;

		if (type == THUMB_MODE_DAY)
			dtf_get_local_datetime(img_time, time_str);
		else
			dtf_get_local_time(img_time, time_str);

		nfui_nfthumbnail_set_subject_label_text((NFTHUMBNAIL*)obj, time_str);
	}
	else
	{
		g_sprintf(time_str, "-- : -- : --");
		nfui_nfthumbnail_set_subject_label_text((NFTHUMBNAIL*)obj, time_str);
	}

	nfui_nfobject_enable(obj);
	nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
}

static void _get_text_datetime_thumbnail_time(guint type, gchar *strTime)
{
	gchar tmp[16];
	gint year, mon, day, hour, min, sec;

	memset(tmp, 0x00, sizeof(tmp));
	dtf_get_local_day(search_time, &year, &mon, &day);
	dtf_get_local_hourmin(search_time, &hour, &min, &sec);

	switch (type) {
	case THUMB_MODE_MIN:
		dtf_get_thumbnail_date(year, mon, day, tmp);
		g_sprintf(strTime, "%s  %02dH", tmp, hour);
		break;

	case THUMB_MODE_HOUR:
		dtf_get_thumbnail_date(year, mon, day, strTime);
		break;

	case THUMB_MODE_DAY:
		dtf_get_thumbnail_date(year, mon, 0, strTime);
		break;
	}

}

static gboolean _thumbnail_pic_event_preview_handling(NFOBJECT *obj)
{
	GTimeVal stime = {0, 0};
	time_t tmp_time;
	gint year, mon, day, hour, min, sec;

	static int cnt = 0;
	gint preview_ch;
	guint ch_mask = 0;
	gint index;
	gint result;
	gint gap_x, gap_y;
	gint shown_as;

	vsm_playback_preview_stop();

	preview_ch = nfui_combobox_get_cur_index(NF_COMBOBOX(ch_obj));

    if (ssm_get_covert_mask() & (1 << preview_ch)) {
        shown_as = ssm_get_covert_shown_as();

        if (!shown_as)
        {
            _preview_obj_off("NO VIDEO");
        }
        else
        {
            _preview_obj_off("COVERT");
        }
		return FALSE;
    }

	result = nfui_nfthumbnail_get_focused_image_time((NFTHUMBNAIL*)obj, &tmp_time);

	if (result == 0)
	{
   		_preview_obj_off("N/A");
		return FALSE;
	}
	else if (result == 2)
	{
    	_preview_obj_off("Please wait...");
		return FALSE;
	}

	search_time = tmp_time;
	stm_set_time_t(search_time);

	dtf_get_local_day(tmp_time, &year, &mon, &day);
	dtf_get_local_hourmin(tmp_time, &hour, &min, &sec);

	index = nfui_combobox_get_cur_index(NF_COMBOBOX(inv_obj));

	switch (index)
	{
		case THUMB_MODE_MIN:
			stime.tv_sec = (tmp_time/60)*60 + 1;
//			stime.tv_sec = ifn_get_gmt_from_local(year, mon, day, hour, min, 1);
		break;

		case THUMB_MODE_HOUR:
			stime.tv_sec = (tmp_time/3600)*3600 + 1;
//			stime.tv_sec = ifn_get_gmt_from_local(year, mon, day, hour, 0, 1);
		break;

		case THUMB_MODE_DAY:
			stime.tv_sec = ifn_get_gmt_from_local(year, mon, day, 0, 0, 1);
		break;
	}

	nfui_nfobject_get_offset((NFOBJECT*)fixed1, &gap_x, &gap_y);
	ch_mask |= (1 << preview_ch);

	vsm_playback_preview_start(ch_mask, stime,
			gap_x+SBTH_PREVIEW_VIDEO_X, gap_y+SBTH_PREVIEW_VIDEO_Y, SBTH_PREVIEW_VIDEO_W, SBTH_PREVIEW_VIDEO_H);

	_preview_obj_on("");

	return TRUE;
}

static gboolean _thumbnail_pic_event_playback_handling(NFOBJECT *obj)
{
	GTimeVal stime = {0, 0};
	time_t tmp_time;
//	gint year, mon, day, hour, min, sec;

	gchar dtBuf[DATE_TIME_TEXT_LEN];
	static int cnt = 0;
	gint index;
	gint result;
	gint shown_as;

	guint ch;

	ch = nfui_combobox_get_cur_index(NF_COMBOBOX(ch_obj));

    if (ssm_get_covert_mask() & (1 << ch)) {
        shown_as = ssm_get_covert_shown_as();

        if (!shown_as)
        {
            _preview_obj_off("NO VIDEO");
        }
        else
        {
            _preview_obj_off("COVERT");
        }
        return FALSE;
    }

	result = nfui_nfthumbnail_get_focused_image_time((NFTHUMBNAIL*)obj, &tmp_time);

	if (result != 1)
		return FALSE;

//	dtf_get_local_day(tmp_time, &year, &mon, &day);
//	dtf_get_local_hourmin(tmp_time, &hour, &min, &sec);

	index = nfui_combobox_get_cur_index(NF_COMBOBOX(inv_obj));

	_preview_obj_off("");
	vsm_playback_preview_stop();

	switch (index)
	{
		case THUMB_MODE_MIN:
		{
//			stime.tv_sec = ifn_get_gmt_from_local(year, mon, day, hour, min, 1);
			stime.tv_sec = (tmp_time/60)*60 +1;

			VW_Search_start_playback();
			vsm_playback_start((1 << ch), stime, PLAYBACK_NORMAL);
		}
		break;

		case THUMB_MODE_HOUR:
		{
			search_time = tmp_time;
			stm_set_time_t(search_time);
			_set_interval_min_thumbnail();

			nfui_combobox_set_index_no_expose(NF_COMBOBOX(inv_obj), THUMB_MODE_MIN);
			nfui_signal_emit((NFOBJECT*)inv_obj, GDK_EXPOSE, TRUE);

			_get_text_datetime_thumbnail_time(THUMB_MODE_MIN, dtBuf);
			nfui_nflabel_set_text((NFLABEL*)dt_obj, dtBuf);
			nfui_signal_emit((NFOBJECT*)dt_obj, GDK_EXPOSE, TRUE);

			_change_min_thumbnail_fixed();
			VW_Search_key_remake();
			_ready_thumbnail();
			_get_thumbnail();
		}
		break;

		case THUMB_MODE_DAY:
		{
			search_time = tmp_time;
			stm_set_time_t(search_time);
			_set_interval_hour_thumbnail();

			nfui_combobox_set_index_no_expose(NF_COMBOBOX(inv_obj), THUMB_MODE_HOUR);
			nfui_signal_emit((NFOBJECT*)inv_obj, GDK_EXPOSE, TRUE);

			_get_text_datetime_thumbnail_time(THUMB_MODE_HOUR, dtBuf);
			nfui_nflabel_set_text((NFLABEL*)dt_obj, dtBuf);
			nfui_signal_emit((NFOBJECT*)dt_obj, GDK_EXPOSE, TRUE);

			_change_hour_thumbnail_fixed();
			VW_Search_key_remake();
			_ready_thumbnail();
			_get_thumbnail();
		}
		break;
	}

	return TRUE;
}

static gboolean _set_preview_time(gpointer data)
{
	GTimeVal tmp;
	gchar strBuf[64];
	gchar *getBuf;

	memset(&tmp, 0x00, sizeof(GTimeVal));
	tmp = vsm_playback_get_previewtime();

	if (tmp.tv_sec != 0)
		dtf_get_local_datetime(tmp.tv_sec, strBuf);
	else
		g_sprintf(strBuf, "");

	getBuf = nfui_nflabel_get_text((NFLABEL*)play_time_obj);

	if (strcmp(getBuf, strBuf) != 0)
	{
		nfui_nflabel_set_text((NFLABEL*)play_time_obj, strBuf);
		nfui_signal_emit(play_time_obj, GDK_EXPOSE, TRUE);
	}

	return TRUE;
}


static int _get_calendar_day(int *year, int *mon, int *day)
{
	if (year) *year	= cw_cld_get_current_year((CWCALENDAR*)g_cal);
	if (mon) *mon = cw_cld_get_current_month((CWCALENDAR*)g_cal);
	if (day) *day  = cw_cld_get_current_day((CWCALENDAR*)g_cal);
	return 0;
}

static int _update_calendar_text(time_t timet)
{
	char buf[64];
	int year, mon;
	int length;

	ifn_get_local_day(timet, &year, &mon, 0);

	length = strlen(g_month_str[mon - 1]);
	g_utf8_strncpy(buf, g_month_str[mon - 1], g_utf8_strlen(g_month_str[mon - 1], -1));
	g_sprintf(&buf[length], " %d", year);

	nfui_nflabel_set_text((NFLABEL*)g_cal_text, buf);
	nfui_signal_emit(g_cal_text, GDK_EXPOSE, TRUE);
	return 0;
}

static int _update_datetime_label()
{
	gchar dtBuf[DATE_TIME_TEXT_LEN];
	int index = nfui_combobox_get_cur_index(NF_COMBOBOX(inv_obj));
	_get_text_datetime_thumbnail_time(index, dtBuf);
	nfui_nflabel_set_text((NFLABEL*)dt_obj, dtBuf);
	nfui_signal_emit((NFOBJECT*)dt_obj, GDK_EXPOSE, TRUE);
	return 0;
}

static int _update_calendar_time(time_t timet)
{
	int year, mon, day;
	if (timet == 0) timet = time(0);
	dtf_get_local_day(timet, &year, &mon, &day);
	cw_cld_change_date((CWCALENDAR*)g_cal, year, mon, day);
	return 0;
}

static time_t _get_new_calendar_time()
{
	int year, month, day, hour, min, sec;
	int index;
	time_t tim;

	_get_calendar_day(&year, &month, &day);
	dtf_get_local_hourmin(search_time, &hour, &min, &sec);
	tim = ifn_get_gmt_from_local(year, month, day, hour, min, sec);

	return tim;
}

static void _change_obj_focus(NFOBJECT* from, NFOBJECT *to)
{
	nfui_set_key_focus(from, FALSE);
	nfui_set_key_focus(to, TRUE);

	nfui_signal_emit(from, GDK_EXPOSE, TRUE);
	nfui_signal_emit(to, GDK_EXPOSE, TRUE);
}

static gboolean post_prev_cal_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	time_t ret;

	if(evt->type == GDK_BUTTON_RELEASE) {
		ret = cw_cld_set_prev_month((CWCALENDAR*)g_cal);
		if (ret == 0) return FALSE;
		_update_calendar_text(ret);
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_UP)
		{
			_change_obj_focus(obj, inv_obj);
			return TRUE;
		}
		else if(kpid == KEYPAD_DOWN)
		{
			_change_obj_focus(obj, g_cal);
			return TRUE;
		}
	}

	return FALSE;
}

static gboolean post_next_cal_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	time_t ret;

	if(evt->type == GDK_BUTTON_RELEASE) {
		ret = cw_cld_set_next_month((CWCALENDAR*)g_cal);
		if (ret == 0) return FALSE;
		_update_calendar_text(ret);
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_UP)
		{
			_change_obj_focus(obj, inv_obj);
			return TRUE;
		}
		else if(kpid == KEYPAD_DOWN)
		{
			_change_obj_focus(obj, g_cal);
			return TRUE;
		}
	}

	return FALSE;
}

static int _control_shift_button()
{
	int hours;

	hours = ifn_get_hours_in_day(search_time, 0, 0);
	if (hours > 24) {
		nfui_nfobject_show(g_prev_btn);
		nfui_nfobject_show(g_next_btn);

		nfui_signal_emit(g_prev_btn, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_next_btn, GDK_EXPOSE, TRUE);
	}
	else {
		nfui_nfobject_hide(g_prev_btn);
		nfui_nfobject_hide(g_next_btn);

//		nfui_signal_emit(g_prev_btn, GDK_EXPOSE, TRUE);
		nfui_signal_emit(fixed2, GDK_EXPOSE, TRUE);
	}

	return 0;
}

static gboolean post_first_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	time_t first_time;

	if(evt->type == GDK_BUTTON_RELEASE) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		first_time = VW_Search_Get_Record_FirstTime();
		if (first_time != 0) {

			_update_calendar_time(first_time);
			_update_calendar_text(first_time);

			search_time = first_time;
			stm_set_time_t(search_time);
    		if (nfui_combobox_get_cur_index(NF_COMBOBOX(inv_obj)) == THUMB_MODE_DAY)
    		{
    			_calc_days_count(search_time);
    			_reset_days_thumb_count();
    		}
    		else if (nfui_combobox_get_cur_index(NF_COMBOBOX(inv_obj)) == THUMB_MODE_HOUR)
    		{
    			_calc_hours_count(search_time);
    			_reset_hours_thumb_count();
    		}
			_update_datetime_label();
			tml_reset_cur_day(tml, search_time);
			_control_shift_button();
			tml_set_playstick_time_t(tml, search_time);
			tml_repaint(tml);

			_ready_thumbnail();
			_get_thumbnail();
		}
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_UP)
		{
			_change_obj_focus(obj, g_cal);
			return TRUE;
		}
		else if (kpid == KEYPAD_DOWN)
		{
			_change_obj_focus(obj, ch_obj);
			return TRUE;
		}
	}

	return FALSE;
}

static gboolean post_last_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	time_t last_time;

	if(evt->type == GDK_BUTTON_RELEASE) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		last_time = VW_Search_Get_Record_LastTime();
		if (last_time != 0) {

			_update_calendar_time(last_time);
			_update_calendar_text(last_time);

			search_time = last_time;
			stm_set_time_t(search_time);
    		if (nfui_combobox_get_cur_index(NF_COMBOBOX(inv_obj)) == THUMB_MODE_DAY)
    		{
    			_calc_days_count(search_time);
    			_reset_days_thumb_count();
    		}
    		else if (nfui_combobox_get_cur_index(NF_COMBOBOX(inv_obj)) == THUMB_MODE_HOUR)
    		{
    			_calc_hours_count(search_time);
    			_reset_hours_thumb_count();
    		}
			_update_datetime_label();
			tml_reset_cur_day(tml, search_time);
			_control_shift_button();
			tml_set_playstick_time_t(tml, search_time);
			tml_repaint(tml);

			_ready_thumbnail();
			_get_thumbnail();
		}
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_UP)
		{
			_change_obj_focus(obj, g_cal);
			return TRUE;
		}
		else if (kpid == KEYPAD_DOWN)
		{
			_change_obj_focus(obj, ch_obj);
			return TRUE;
		}
	}

	return FALSE;
}

static gboolean post_prev_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	time_t base;
	int hour;

	if(evt->type == GDK_BUTTON_RELEASE) {
	  	if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
		base = tml_get_base_time_t(tml);
		base -= 3600;		// prev 1 hour
		dtf_get_local_hourmin(base, &hour, 0, 0);
		if (hour == 23) return FALSE;		// prev day
		tml_shift(tml, -24);
	}
	return FALSE;
}

static gboolean post_next_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	time_t edge;
	int hour;

	if(evt->type == GDK_BUTTON_RELEASE) {
	  	if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
		edge = tml_get_edge_time_t(tml);
		edge += 3600;	// next 1 hour
		dtf_get_local_hourmin(edge, &hour, 0, 0);
		if (hour == 0) return FALSE;		// next day
		tml_shift(tml, +24);
	}
	return FALSE;
}

static gboolean post_calendar_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CALENDAR_CHANGED_RELEASE) {
		search_time = _get_new_calendar_time();
		stm_set_time_t(search_time);
		if (nfui_combobox_get_cur_index(NF_COMBOBOX(inv_obj)) == THUMB_MODE_DAY)
		{
			_calc_days_count(search_time);
			_reset_days_thumb_count();
		}
		else if (nfui_combobox_get_cur_index(NF_COMBOBOX(inv_obj)) == THUMB_MODE_HOUR)
		{
			_calc_hours_count(search_time);
			_reset_hours_thumb_count();
		}
		_update_datetime_label();
		_update_calendar_text(search_time);
		tml_reset_cur_day(tml, search_time);
		_control_shift_button();
		_ready_thumbnail();
		_get_thumbnail();
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if (((CWCALENDAR*)obj)->object.grab_kfocus) return FALSE;

		if(kpid == KEYPAD_UP)
		{
			_change_obj_focus(obj, g_cal_dir[1]);
			return TRUE;
		}
		else if (kpid == KEYPAD_DOWN)
		{
			_change_obj_focus(obj, g_last_btn);
			return TRUE;
		}
	}

	return FALSE;
}

/*
static int _update_tml_ch(int ch)
{
	char strBuf[16];
	sprintf(strBuf, "%d", ch + 1);
	nfui_nfbutton_set_text(tml_ch_obj, strBuf);
	nfui_signal_emit(tml_ch_obj, GDK_EXPOSE, FALSE);
	return 0;
}
*/

static gboolean post_channel_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	int ch;

	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {
		ch = nfui_combobox_get_cur_index(NF_COMBOBOX(ch_obj));
		tml_change_channel(tml, ch);
		_update_datetime_label();
//		_update_tml_ch(ch);
		_ready_thumbnail();
		_get_thumbnail();
 	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_UP)
		{
			_change_obj_focus(obj, g_last_btn);
			return TRUE;
		}
		else if (kpid == KEYPAD_DOWN)
		{
			_change_obj_focus(obj, dt_obj);
			return TRUE;
		}
	}

	return FALSE;
}

static gboolean post_date_time_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;
	NFOBJECT *top;
	time_t tt;
	int year, month, day;
	int x, y;
	int index;
	char dtBuf[DATE_TIME_TEXT_LEN];


	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) {
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if (evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER) {

		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		nfui_nfobject_get_offset(obj, &x, &y);
		top = nfui_nfobject_get_top(obj);

		x += top->x + obj->width + 4;
		y += top->y;

		index = nfui_combobox_get_cur_index(NF_COMBOBOX(inv_obj));

		switch (index) {
		case THUMB_MODE_MIN:
			tt = VW_Set_DateTime_Open(g_curwnd, "THUMBNAIL DATE/TIME", x, y, search_time,
					SDT_TYPE_HOUR, NF_LOWER_TIMELIMIT, NF_UPPER_TIMELIMIT);
			if ((tt == 0) || (search_time == tt))
				return FALSE;
			search_time = tt;
			thumbnail_hour = tt;
			break;

		case THUMB_MODE_HOUR:
			tt = VW_Set_DateTime_Open(g_curwnd, "THUMBNAIL DATE/TIME", x, y, search_time,
					SDT_TYPE_DAY, NF_LOWER_TIMELIMIT, NF_UPPER_TIMELIMIT);
			if ((tt == 0) || (search_time == tt))
				return FALSE;
			search_time = tt;
			_calc_hours_count(search_time);
			_reset_hours_thumb_count();
			break;

		case THUMB_MODE_DAY:
			tt = VW_Set_DateTime_Open(g_curwnd, "THUMBNAIL DATE/TIME", x, y, search_time,
					SDT_TYPE_MON, NF_LOWER_TIMELIMIT, NF_UPPER_TIMELIMIT);

			if ((tt == 0) || (search_time == tt))
				return FALSE;
			search_time = tt;
			_calc_days_count(search_time);
			_reset_days_thumb_count();
			break;
		}

		_update_calendar_time(search_time);
		_update_calendar_text(search_time);
		_update_datetime_label();
		tml_reset_cur_day(tml, search_time);
		_control_shift_button();
		stm_set_time_t(search_time);
		_ready_thumbnail();
		_get_thumbnail();
	}
	else if(kpid == KEYPAD_UP)
	{
		_change_obj_focus(obj, ch_obj);
		return TRUE;
	}
	else if (kpid == KEYPAD_DOWN)
	{
		_change_obj_focus(obj, inv_obj);
		return TRUE;
	}

    return FALSE;
}

static gboolean post_interval_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint index;
	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {

		index = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));
		switch (index) {
		case THUMB_MODE_MIN:
			_change_min_thumbnail_fixed();
			_ready_thumbnail();
			tml_refresh(tml);
			break;

		case THUMB_MODE_HOUR:
			_calc_hours_count(search_time);
			_change_hour_thumbnail_fixed();
			_ready_thumbnail();
			tml_refresh(tml);
			break;

		case THUMB_MODE_DAY:
			_calc_days_count(search_time);
			_change_day_thumbnail_fixed();
			_ready_thumbnail();
			tml_refresh(tml);
			break;
		}

		_update_datetime_label();
		VW_Search_key_remake();
		_get_thumbnail();
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if (kpid == KEYPAD_DOWN)
		{
			_change_obj_focus(obj, g_cal_dir[1]);
			return TRUE;
		}
	}

	return FALSE;
}


static gboolean post_pic_min_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	gint i;
	time_t timet = 0;
	gint result;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_BUTTON_PRESS || evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		for (i = 0; i < THUMB_MIN_CELL_MAX; i++)
		{
			if (thumb_obj[THUMB_MODE_MIN][i] == obj)
				break;
		}

		if(i == THUMB_MIN_CELL_MAX) return FALSE;

		result = _set_select_bg_color(obj);

		if (evt->type == GDK_2BUTTON_PRESS)
			_thumbnail_pic_event_playback_handling(thumb_obj[THUMB_MODE_MIN][i]);
		else if ((kpid == KEYPAD_ENTER) && (result == -1))
		{
			_change_obj_focus(obj, g_cal);
			_thumbnail_pic_event_playback_handling(thumb_obj[THUMB_MODE_MIN][i]);
		}
		else
			_thumbnail_pic_event_preview_handling(thumb_obj[THUMB_MODE_MIN][i]);

		nfui_nfthumbnail_get_focused_start_time(thumb_obj[THUMB_MODE_MIN][i], &timet);

		stm_set_time_t(timet);
		tml_set_playstick_time_t(tml, timet);
		tml_repaint(tml);
	}
	else if(kpid == KEYPAD_UP)
	{
		gint row, col;

		for (i = 0; i < THUMB_MIN_CELL_MAX; i++)
		{
			if (thumb_obj[THUMB_MODE_MIN][i] == obj)
				break;
		}

		row = i/THUMB_MIN_CELL_COL;
		col = i%THUMB_MIN_CELL_COL;

		if (row == 0)
		{
			_change_obj_focus(obj, playbtn_obj);
			return TRUE;
		}
		else
		{
			i = (row-1)*THUMB_MIN_CELL_COL+col;
			_change_obj_focus(obj, thumb_obj[THUMB_MODE_MIN][i]);
			return TRUE;
		}
	}
	else if (kpid == KEYPAD_DOWN)
	{
		gint row, col;

		for (i = 0; i < THUMB_MIN_CELL_MAX; i++)
		{
			if (thumb_obj[THUMB_MODE_MIN][i] == obj)
				break;
		}

		row = i/THUMB_MIN_CELL_COL;
		col = i%THUMB_MIN_CELL_COL;
		i = (row+1)*THUMB_MIN_CELL_COL+col;

		if (i < THUMB_MIN_CELL_MAX)
		{
			_change_obj_focus(obj, thumb_obj[THUMB_MODE_MIN][i]);
			return TRUE;
		}
	}

	return FALSE;
}


static gboolean post_pic_hour_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	static int cnt = 0;
	gint i;
	time_t timet = 0;
	gint result;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_BUTTON_PRESS || evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		for (i = 0; i < THUMB_HOUR_CELL_MAX; i++)
		{
			if (thumb_obj[THUMB_MODE_HOUR][i] == obj)
				break;
		}

		result = _set_select_bg_color(obj);

		if (evt->type == GDK_2BUTTON_PRESS)
			_thumbnail_pic_event_playback_handling(thumb_obj[THUMB_MODE_HOUR][i]);
		else if ((kpid == KEYPAD_ENTER) && (result == -1))
		{
			_change_obj_focus(obj, g_cal);
			_thumbnail_pic_event_playback_handling(thumb_obj[THUMB_MODE_HOUR][i]);
		}
		else
			_thumbnail_pic_event_preview_handling(thumb_obj[THUMB_MODE_HOUR][i]);

		nfui_nfthumbnail_get_focused_start_time(thumb_obj[THUMB_MODE_HOUR][i], &timet);

		stm_set_time_t(timet);
		tml_set_playstick_time_t(tml, timet);
		tml_repaint(tml);
	}
	else if(kpid == KEYPAD_UP)
	{
		gint row, col;

		for (i = 0; i < THUMB_HOUR_CELL_MAX; i++)
		{
			if (thumb_obj[THUMB_MODE_HOUR][i] == obj)
				break;
		}

		row = i/THUMB_HOUR_CELL_COL;
		col = i%THUMB_HOUR_CELL_COL;

		if (row == 0)
		{
			_change_obj_focus(obj, playbtn_obj);
			return TRUE;
		}
		else
		{
			i = (row-1)*THUMB_HOUR_CELL_COL+col;
			_change_obj_focus(obj, thumb_obj[THUMB_MODE_HOUR][i]);
			return TRUE;
		}
	}
	else if (kpid == KEYPAD_DOWN)
	{
		gint row, col, max_cnt;

		for (i = 0; i < THUMB_HOUR_CELL_MAX; i++)
		{
			if (thumb_obj[THUMB_MODE_HOUR][i] == obj)
				break;
		}

		row = i/THUMB_HOUR_CELL_COL;
		col = i%THUMB_HOUR_CELL_COL;
		i = (row+1)*THUMB_HOUR_CELL_COL+col;

		max_cnt = GPOINTER_TO_INT(nfui_nfobject_get_data(hour_fixed, "THUMB CNT"));

		if (i < max_cnt)
		{
			_change_obj_focus(obj, thumb_obj[THUMB_MODE_HOUR][i]);
			return TRUE;
		}
	}

	return FALSE;
}


static gboolean post_pic_day_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	static int cnt = 0;
	gint i;
	gint result;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_BUTTON_PRESS || evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		for (i = 0; i < THUMB_DAY_CELL_MAX; i++)
		{
			if (thumb_obj[THUMB_MODE_DAY][i] == obj)
				break;
		}

		result = _set_select_bg_color(obj);

		if (evt->type == GDK_2BUTTON_PRESS)
			_thumbnail_pic_event_playback_handling(thumb_obj[THUMB_MODE_DAY][i]);
		else if ((kpid == KEYPAD_ENTER) && (result == -1))
		{
			_change_obj_focus(obj, g_cal);
			_thumbnail_pic_event_playback_handling(thumb_obj[THUMB_MODE_DAY][i]);
		}
		else
			_thumbnail_pic_event_preview_handling(thumb_obj[THUMB_MODE_DAY][i]);

		_update_calendar_time(search_time);
		_update_calendar_text(search_time);
		_update_datetime_label();
		stm_set_time_t(search_time);
		tml_reset_cur_day(tml, search_time);
		_control_shift_button();
	}
	else if(kpid == KEYPAD_UP)
	{
		gint row, col;

		for (i = 0; i < THUMB_DAY_CELL_MAX; i++)
		{
			if (thumb_obj[THUMB_MODE_DAY][i] == obj)
				break;
		}

		row = i/THUMB_DAY_CELL_COL;
		col = i%THUMB_DAY_CELL_COL;

		if (row == 0)
		{
			_change_obj_focus(obj, playbtn_obj);
			return TRUE;
		}
		else
		{
			i = (row-1)*THUMB_DAY_CELL_COL+col;
			_change_obj_focus(obj, thumb_obj[THUMB_MODE_DAY][i]);
			return TRUE;
		}
	}
	else if (kpid == KEYPAD_DOWN)
	{
		gint row, col, max_cnt;

		for (i = 0; i < THUMB_DAY_CELL_MAX; i++)
		{
			if (thumb_obj[THUMB_MODE_DAY][i] == obj)
				break;
		}

		row = i/THUMB_DAY_CELL_COL;
		col = i%THUMB_DAY_CELL_COL;
		i = (row+1)*THUMB_DAY_CELL_COL+col;

		if (i < THUMB_DAY_CELL_MAX)
		{
			_change_obj_focus(obj, thumb_obj[THUMB_MODE_DAY][i]);
			return TRUE;
		}
	}

	return FALSE;
}

static gboolean pre_cal_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case GDK_EXPOSE:
		{
			GdkGC *gc;
			GdkDrawable *drawable;
			gint pos_x, pos_y;

			drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
			gc = gdk_gc_new(drawable);

			nfui_nfobject_get_offset(obj, &pos_x, &pos_y);
			nfutil_draw_image(drawable, gc, IMG_CALENDAR_BG, pos_x, pos_y, -1, -1, NFALIGN_LEFT, 0);

	 		g_object_unref(gc);

			_update_calendar_time(search_time);
			_update_calendar_text(search_time);
			_update_datetime_label();
			// SKSHIN, 0722
//			stm_set_time_t(search_time);
	 	}
		break;

		case GDK_DELETE:

		break;

		default :

		break;
	}

	return FALSE;
}

static gboolean pre_fixed1_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case GDK_EXPOSE:
		{
			GdkGC *gc;
			GdkDrawable *drawable;
			gint gap_x, gap_y;

			nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);

			drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
			gc = gdk_gc_new(drawable);

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(604));
			gdk_draw_rectangle(drawable, gc, TRUE, gap_x+obj->width-1, gap_y, 1, obj->height);

			if (preview_timer_id == 0)
				preview_timer_id = g_timeout_add(500, _set_preview_time, NULL);

			// skshin
	 		g_object_unref(gc);
		}
		break;

		case GDK_DELETE:
		{
			if(preview_timer_id != 0) {
				g_source_remove(preview_timer_id);
				preview_timer_id = 0;
			}
		}
		break;

		default :

		break;
	}

	return FALSE;
}


static int _play()
{
	time_t tmp_time;
	GTimeVal searchTime = {0, 0};
	gint year, mon, day, hour, min, sec;
	gint index;
	guint ch, ch_mask = 0;
	NFOBJECT *selected_thumb;

	if (nfui_nfthumbnail_check_running()) return -1;

	_preview_obj_off("");
	vsm_playback_preview_stop();

	selected_thumb = _get_selected_thumbnail_obj();

	if (selected_thumb) {
		dtf_get_local_day(search_time, &year, &mon, &day);
		dtf_get_local_hourmin(search_time, &hour, &min, &sec);

		index = nfui_combobox_get_cur_index(NF_COMBOBOX(inv_obj));

		switch (index)
		{
			case THUMB_MODE_MIN:
				searchTime.tv_sec = (search_time/60)*60;
//				searchTime.tv_sec = ifn_get_gmt_from_local(year, mon, day, hour, min, 0);
			break;

			case THUMB_MODE_HOUR:
				searchTime.tv_sec = (search_time/3600)*3600;
//				searchTime.tv_sec = ifn_get_gmt_from_local(year, mon, day, hour, 0, 0);
			break;

			case THUMB_MODE_DAY:
				searchTime.tv_sec = ifn_get_gmt_from_local(year, mon, day, 0, 0, 0);
			break;
		}
	}
	else {
		searchTime.tv_sec = search_time;
#if 0
		dtf_get_local_day(search_time, &year, &mon, &day);
		dtf_get_local_hourmin(search_time, &hour, &min, &sec);

		index = nfui_combobox_get_cur_index(NF_COMBOBOX(inv_obj));

		switch (index)  {
		case THUMB_MODE_MIN:
			searchTime.tv_sec = (search_time/3600)*3600;
//			searchTime.tv_sec = ifn_get_gmt_from_local(year, mon, day, hour, 0, 0);
			break;

		case THUMB_MODE_HOUR:
			searchTime.tv_sec = ifn_get_gmt_from_local(year, mon, day, 0, 0, 0);
			break;

		case THUMB_MODE_DAY:
			searchTime.tv_sec = ifn_get_gmt_from_local(year, mon, 1, 0, 0, 0);
			break;
		}
#endif
	}

	ch = nfui_combobox_get_cur_index(NF_COMBOBOX(ch_obj));
	ch_mask |= (1 << ch);

	VW_Search_start_playback();
	vsm_playback_start(ch_mask, searchTime, PLAYBACK_NORMAL);

	return 0;
}

static gboolean pre_fixed2_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkGC *gc;
	GdkDrawable *drawable;
	GTimeVal schtime;
	CMM_MESSAGE_T *pmsg;
	gint gap_x, gap_y;

	switch (evt->type) {
	case GDK_EXPOSE:
		nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);

		memset(&schtime, 0x00, sizeof(GTimeVal));

		stm_get_time(&schtime);

		drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
		gc = gdk_gc_new(drawable);

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(604));
		gdk_draw_rectangle(drawable, gc, TRUE, gap_x, gap_y+obj->height-1, obj->width, 1);

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(647));
		gdk_draw_rectangle(drawable,
				gc,
				TRUE,
					gap_x+SBTH_TL_REC_BOX_PRE_X, gap_y+SBTH_TL_REC_BOX_PRE_Y,
				SBTH_TL_REC_BOX_W, SBTH_TL_REC_BOX_H);

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(639));
		gdk_draw_rectangle(drawable,
				gc,
				TRUE,
					gap_x+SBTH_TL_REC_BOX_TIME_X, gap_y+SBTH_TL_REC_BOX_TIME_Y,
				SBTH_TL_REC_BOX_W, SBTH_TL_REC_BOX_H);

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(641));
		gdk_draw_rectangle(drawable,
				gc,
				TRUE,
					gap_x+SBTH_TL_REC_BOX_ALARM_X, gap_y+SBTH_TL_REC_BOX_ALARM_Y,
				SBTH_TL_REC_BOX_W, SBTH_TL_REC_BOX_H);

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(643));
		gdk_draw_rectangle(drawable,
				gc,
				TRUE,
					gap_x+SBTH_TL_REC_BOX_MOT_X, gap_y+SBTH_TL_REC_BOX_MOT_Y,
				SBTH_TL_REC_BOX_W, SBTH_TL_REC_BOX_H);

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(645));
		gdk_draw_rectangle(drawable,
				gc,
				TRUE,
					gap_x+SBTH_TL_REC_BOX_PANIC_X, gap_y+SBTH_TL_REC_BOX_PANIC_Y,
				SBTH_TL_REC_BOX_W, SBTH_TL_REC_BOX_H);

	#if 0
			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(630));
		gdk_draw_rectangle(drawable,
				gc,
				TRUE,
				SBTH_LEFT_GAP_X + SBTH_FIXED2_X + SBT_TML_CH_BTN_X,
				SBTH_UP_GAP_Y + SBTH_FIXED2_Y + SBT_TML_CH_BTN_Y - 5,
				32, 5);
	#endif

		g_object_unref(gc);


		break;

	case GDK_DELETE:
		break;

	}

	return FALSE;
}

static NFOBJECT *_get_thumb_obj(int intv_idx, time_t timet)
{
	time_t s_time, e_time;
	int day, hour, min;

	dtf_get_local_day(timet, 0, 0, &day);
	dtf_get_local_hourmin(timet, &hour, &min, 0);

	switch (intv_idx) {
		case 0:
			return thumb_obj[THUMB_MODE_MIN][min];
		break;
		case 2:
			return thumb_obj[THUMB_MODE_DAY][day-1];
		break;
		case 1:
		{
			gint i, thumb_cnt;

			thumb_cnt = GPOINTER_TO_INT(nfui_nfobject_get_data(hour_fixed, "THUMB CNT"));

			for (i = 0; i < thumb_cnt; i++)
			{
				nfui_nfthumbnail_get_period((NFTHUMBNAIL*)thumb_obj[THUMB_MODE_HOUR][i], &s_time, &e_time);

				if ((s_time <= timet) && (timet <= e_time))
					return thumb_obj[THUMB_MODE_HOUR][i];
			}
		}
		break;
	}
	return 0;
}

static int _change_thumbnail_state(int intv_idx, time_t timet)
{
	NFOBJECT *obj;
	obj = _get_thumb_obj(intv_idx, search_time);
	_set_select_bg_color(obj);
	return 0;
}

static gboolean post_min_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	int index;

	switch(evt->type)
	{
		case GDK_EXPOSE:
		{
			if (g_init_thumb) g_timeout_add(300, _show_thumbnail, 0);
		}
		break;

		case GDK_DELETE:
			g_init_thumb = TRUE;
			uxm_unreg_imsg_event(obj, INFY_THUMBNAIL_CMPL_OBJ);
			uxm_unreg_imsg_event(obj, INFY_THUMBNAIL_TOTALLY_CMPL);
		break;

		case INFY_THUMBNAIL_CMPL_OBJ:
		{
			gint i;
			NFOBJECT *rate_obj;
			NFOBJECT *cmpl_obj = ((CMM_MESSAGE_T *)data)->data;
			gchar strBuf[10];
			gchar time_str[64];

			time_t img_time;
			gboolean ret_val;

			if (!nfui_nfobject_is_shown(obj))
				return FALSE;

			for(i = 0; i < THUMB_MIN_CELL_MAX; i++)
			{
				if (cmpl_obj == thumb_obj[THUMB_MODE_MIN][i])
					break;
			}

			if (i == THUMB_MIN_CELL_MAX)
				return FALSE;

			if (nfui_nfobject_is_disabled(thumb_obj[THUMB_MODE_MIN][0])
				&& (i != 0))
				return;

			rate_obj = (NFOBJECT*)nfui_nfobject_get_data(obj, "CMPL RATE");
			_update_thumbnail_rate(rate_obj, i+1, THUMB_MIN_CELL_MAX);
			_update_thumbnail_info(cmpl_obj, THUMB_MODE_MIN);

			if (i+1 == THUMB_MIN_CELL_MAX)
				_playback_button_enable();
		}
		break;

		case INFY_THUMBNAIL_TOTALLY_CMPL:
		{
			NFOBJECT *selected_thumb;

			if (!nfui_nfobject_is_shown(obj))
				return FALSE;

			selected_thumb = _get_selected_thumbnail_obj();

			if (selected_thumb)
				_thumbnail_pic_event_preview_handling(selected_thumb);
		}
		break;

		default :

		break;
	}

	return FALSE;
}

static gboolean post_hour_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	static gboolean init_thumb = TRUE;

	switch(evt->type)
	{
		case GDK_EXPOSE:
		{
#if 1
			if (g_init_thumb) g_timeout_add(300, _show_thumbnail, 0);
#else
			if (g_init_thumb)
			{
				_ready_thumbnail();
				_get_thumbnail();
				g_init_thumb = FALSE;
			}
#endif
		}
		break;

		case GDK_DELETE:
			g_init_thumb = TRUE;
			uxm_unreg_imsg_event(obj, INFY_THUMBNAIL_CMPL_OBJ);
			uxm_unreg_imsg_event(obj, INFY_THUMBNAIL_TOTALLY_CMPL);
		break;

		case INFY_THUMBNAIL_CMPL_OBJ:
		{
			gint i, thumb_cnt;
			NFOBJECT *rate_obj;
			NFOBJECT *cmpl_obj = ((CMM_MESSAGE_T *)data)->data;
			gchar strBuf[10];

			if (!nfui_nfobject_is_shown(obj))
				return FALSE;

			thumb_cnt = GPOINTER_TO_INT(nfui_nfobject_get_data(hour_fixed, "THUMB CNT"));

			for(i = 0; i < thumb_cnt; i++)
			{
				if (cmpl_obj == thumb_obj[THUMB_MODE_HOUR][i])
					break;
			}

			if (i == thumb_cnt)
				return FALSE;

			if (nfui_nfobject_is_disabled(thumb_obj[THUMB_MODE_HOUR][0])
				&& (i != 0))
				return;

			rate_obj = (NFOBJECT*)nfui_nfobject_get_data(obj, "CMPL RATE");
			_update_thumbnail_rate(rate_obj, i+1, thumb_cnt);
			_update_thumbnail_info(cmpl_obj, THUMB_MODE_HOUR);

			if (i+1 == thumb_cnt)
				_playback_button_enable();
		}
		break;

		case INFY_THUMBNAIL_TOTALLY_CMPL:
		{
			NFOBJECT *selected_thumb;

			if (!nfui_nfobject_is_shown(obj))
				return FALSE;

			selected_thumb = _get_selected_thumbnail_obj();

			if (selected_thumb)
				_thumbnail_pic_event_preview_handling(selected_thumb);
		}
		break;

		default :

		break;
	}

	return FALSE;
}

static gboolean post_day_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	static gboolean init_thumb = TRUE;

	switch(evt->type)
	{
		case GDK_EXPOSE:
		{
#if 1
			if (g_init_thumb) g_timeout_add(300, _show_thumbnail, 0);
#else
			if (g_init_thumb)
			{
				_ready_thumbnail();
				_get_thumbnail();
				g_init_thumb = FALSE;
			}
#endif
		}
		break;

		case GDK_DELETE:
			g_init_thumb = TRUE;
			uxm_unreg_imsg_event(obj, INFY_THUMBNAIL_CMPL_OBJ);
			uxm_unreg_imsg_event(obj, INFY_THUMBNAIL_TOTALLY_CMPL);
		break;

		case INFY_THUMBNAIL_CMPL_OBJ:
		{
			gint i, thumb_cnt;
			NFOBJECT *rate_obj;
			NFOBJECT *cmpl_obj = ((CMM_MESSAGE_T *)data)->data;
			gchar strBuf[10];

			if (!nfui_nfobject_is_shown(obj))
				return FALSE;

			thumb_cnt = GPOINTER_TO_INT(nfui_nfobject_get_data(day_fixed, "THUMB CNT"));

			for(i = 0; i < thumb_cnt; i++)
			{
				if (cmpl_obj == thumb_obj[THUMB_MODE_DAY][i])
					break;
			}

			if (i == thumb_cnt)
				return FALSE;

			if (nfui_nfobject_is_disabled(thumb_obj[THUMB_MODE_DAY][0])
				&& (i != 0))
				return;

			rate_obj = (NFOBJECT*)nfui_nfobject_get_data(obj, "CMPL RATE");
			_update_thumbnail_rate(rate_obj, i+1, thumb_cnt);
			_update_thumbnail_info(cmpl_obj, THUMB_MODE_DAY);

			if (i+1 == thumb_cnt)
				_playback_button_enable();
		}
		break;

		case INFY_THUMBNAIL_TOTALLY_CMPL:
		{
			NFOBJECT *selected_thumb;

			if (!nfui_nfobject_is_shown(obj))
				return FALSE;

			selected_thumb = _get_selected_thumbnail_obj();

			if (selected_thumb)
				_thumbnail_pic_event_preview_handling(selected_thumb);
		}
		break;

		default :

		break;
	}

	return FALSE;
}

static gboolean post_timeline_event_cb(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	CMM_MESSAGE_T *pmsg = (CMM_MESSAGE_T *)data;
	char label[32];
	gint pre_hour, post_hour;
	gint pre_isdst, post_isdst;

	time_t cur_utime;
	int index;

	switch (event->type) {
	case GDK_BUTTON_RELEASE:
		break;

	case GDK_2BUTTON_PRESS:
		break;

	case NFEVENT_TML_RULER_DRAGUP:
		break;

	case NFEVENT_TML_RULER_DRAGDOWN:
		break;

	case GDK_DELETE:
		uxm_unreg_imsg_event(obj, INFY_TML_DATE_CHANGED);
		uxm_unreg_imsg_event(obj, INFY_TML_PLAY_CHANGED);
		uxm_unreg_imsg_event(obj, INFY_TML_START_CHANGED);
		uxm_unreg_imsg_event(obj, INFY_TML_END_CHANGED);
		uxm_unreg_imsg_event(obj, INFY_TML_SCROLL_UP);
		uxm_unreg_imsg_event(obj, INFY_TML_SCROLL_DOWN);
		uxm_unreg_imsg_event(obj, INFY_TML_DOUBLE_CLICKED);
		break;

	case INFY_TML_DOUBLE_CLICKED:
		pmsg = (CMM_MESSAGE_T *)data;
		if (obj != pmsg->data) return FALSE;
		if (!nfui_nfobject_is_shown(obj)) return FALSE;
		_play();
		break;

	case INFY_TML_DATE_CHANGED:
		pmsg = (CMM_MESSAGE_T *)data;
		if (obj != pmsg->data) return FALSE;
		if (!nfui_nfobject_is_shown(obj)) return FALSE;
		break;

	case INFY_TML_PLAY_CHANGED:
	{
		NFOBJECT *selected_thumb;

		pmsg = (CMM_MESSAGE_T *)data;
		if (obj != pmsg->data) return FALSE;
		if (!nfui_nfobject_is_shown(obj)) return FALSE;

		cur_utime = pmsg->param;
//		selected_thumb = _get_selected_thumbnail_obj();

		dtf_get_local_hourmin(search_time, &pre_hour, 0, 0);
		dtf_get_local_hourmin(cur_utime, &post_hour, 0, 0);

		pre_isdst = ifn_is_in_dst(search_time);
		post_isdst = ifn_is_in_dst(cur_utime);

		search_time = pmsg->param;
		stm_set_time_t(search_time);
		_update_datetime_label();

		index = nfui_combobox_get_cur_index(NF_COMBOBOX(inv_obj));

		if (index == THUMB_MODE_MIN)
		{
			if ((pre_hour != post_hour) || (pre_isdst != post_isdst))
			{
				_ready_thumbnail();
				_get_thumbnail();
				_change_thumbnail_state(index, search_time);
			}
			else
			{
				_change_thumbnail_state(index, search_time);
				selected_thumb = _get_selected_thumbnail_obj();
				if (selected_thumb)	_thumbnail_pic_event_preview_handling(selected_thumb);
			}
		}
		else if (index == THUMB_MODE_HOUR)
		{
			_change_thumbnail_state(index, search_time);
			selected_thumb = _get_selected_thumbnail_obj();
			if (selected_thumb)	_thumbnail_pic_event_preview_handling(selected_thumb);
		}
	}
	break;

	case INFY_TML_END_CHANGED:
		pmsg = (CMM_MESSAGE_T *)data;
		if (obj != pmsg->data) return FALSE;
		break;

	case INFY_TML_SCROLL_UP:
		pmsg = (CMM_MESSAGE_T *)data;
		if (obj != pmsg->data) return FALSE;
		tml_zoom_in(tml);
		break;

	case INFY_TML_SCROLL_DOWN:
		pmsg = (CMM_MESSAGE_T *)data;
		if (obj != pmsg->data) return FALSE;
		tml_zoom_out(tml);
		break;

	case NFEVENT_KEYPAD_PRESS:
	case NFEVENT_REMOCON_PRESS:
		{
			GdkEventKey *kevt;
			KEYPAD_KID kpid;

			kevt = (GdkEventKey*)event;
			kpid = (KEYPAD_KID)kevt->keyval;

			if(kpid == KEYPAD_UP)
			{
				_change_obj_focus(obj, playbtn_obj);
				return TRUE;
			}
			else if (kpid == KEYPAD_DOWN)
			{
				gint index;

				index = nfui_combobox_get_cur_index(NF_COMBOBOX(inv_obj));
				_change_obj_focus(obj, thumb_obj[index][0]);
				return TRUE;
			}
			else if (kpid == KEYPAD_LEFT)
			{
				_change_obj_focus(obj, g_cal_dir[1]);
				return TRUE;
			}
			else if (kpid == KEYPAD_RIGHT)
			{
				_change_obj_focus(obj, g_cal_dir[0]);
				return TRUE;
			}
			else if (kpid == KEYPAD_EXIT)
			{
				nfui_set_key_focus(obj, FALSE);
				nfui_set_key_focus(g_cal, TRUE);
				nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
				nfui_signal_emit(g_cal, GDK_EXPOSE, TRUE);
				return TRUE;
			}
		}
		break;
	}

	return TRUE;
}

static gboolean post_playbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
		_play();
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if (kpid == KEYPAD_DOWN)
		{
			gint index;

			index = nfui_combobox_get_cur_index(NF_COMBOBOX(inv_obj));
			_change_obj_focus(obj, thumb_obj[index][0]);
			return TRUE;
		}
	}

	return FALSE;
}

static gboolean post_ch_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{

	return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {
			return FALSE;
		}

		vsm_playback_preview_stop();
		VW_Search_Destroy();
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if (kpid == KEYPAD_DOWN)
		{
			gint index;

			index = nfui_combobox_get_cur_index(NF_COMBOBOX(inv_obj));
			_change_obj_focus(obj, thumb_obj[index][0]);
			return TRUE;
		}
	}

	return FALSE;
}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
	}

	return FALSE;
}



////////////////////////////////////////////////////////////
//
// public interfaces
//


/*
 :  search by thumbnail main_fixed.

	  --------------------------------------------
	  |  ---------------------------------------  |
	  |	|			|							| |
	  |	|			|		FIXED2				| |
	  |	|			|---------------------------| |
	  |	|			|							| |
	  |	|			|							| |
	  |	|  FIXED1	|							| |
	  |	|			|		MIN_FIXED 	or		| |
	  |	|			|		HOUR_FIXED 	or		| |
	  |	|			|		DAY_FIXED 			| |
	  |	|			|							| |
	  |	|			|							| |
	  |	|			|							| |
	  |	 ---------------------------------------  |
	  |											  |
	  |				PARENT						  |
	  |-------------------------------------------|
*/



void vw_init_SearchByThumbnail_page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;

	NFOBJECT *cal_fixed;
	NFOBJECT *ntb1;
	NFOBJECT *ntb;

	NFOBJECT *obj;

	TML_FIGURE_T figure;

	guint size_w, size_h;
	guint pos_x, pos_y;

	guint col, row;
	guint i, ch;
	gint length;

//	GdkPixbuf *ch_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *datetime_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *tl_btn_img[4][NFOBJECT_STATE_COUNT];
	GdkPixbuf *arrow_img[2][NFOBJECT_STATE_COUNT];

	guint width1[SBTH_TABLE1_COLUMNS];
	guint week_w[7];
//	guint font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(TODO_COLOR), COLOR_IDX(TODO_COLOR), COLOR_IDX(TODO_COLOR), COLOR_IDX(TODO_COLOR)};

	const gchar *strDay[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
	const gchar *strTitle[] = {"CHANNEL", "DATE/TIME", "INTERVAL"};
	const gchar *strInterval[] = {"MINUTES", "HOURS", "DAYS"};
	const gchar *strRec[] = { "PRE", "CONTINUOUS", "ALARM", "MOTION", "PANIC"};
	const gchar *calButton[] = {"FIRST", "LAST"};
	const gchar *strButton[] = {"PLAYBACK", "CLOSE"};

	gchar *strCh[GUI_CHANNEL_CNT];
	gchar strBuf[64];

	time_t s_time;
	time_t e_time;
	gint year, month, day, hour, min, sec;


	g_curwnd = nfui_nfobject_get_top(parent);


	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(content_fixed, MENU_H_INNER_W, MENU_H_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);

	// set calendar data
	stm_get_time_range_t(0, &search_time);

	dtf_get_local_day(search_time, &year, &month, &day);
	dtf_get_local_hourmin(search_time, &hour, &min, &sec);

// <---- FIXED
	fixed1 = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed1, SBTH_FIXED1_W, SBTH_FIXED1_H);
	nfui_nfobject_show(fixed1);
	nfui_nfobject_modify_bg(fixed1, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nffixed_put((NFFIXED*)content_fixed, fixed1, SBTH_FIXED1_X, SBTH_FIXED1_Y);

	fixed2 = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed2, SBTH_FIXED2_W, SBTH_FIXED2_H);
	nfui_nfobject_show(fixed2);
	nfui_nfobject_modify_bg(fixed2, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nffixed_put((NFFIXED*)content_fixed, fixed2, SBTH_FIXED2_X, SBTH_FIXED2_Y);

	min_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(min_fixed, SBTH_MIN_FIXED_W, SBTH_MIN_FIXED_H);
	nfui_nfobject_modify_bg(min_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nffixed_put((NFFIXED*)content_fixed, min_fixed, SBTH_MIN_FIXED_X, SBTH_MIN_FIXED_Y);

	hour_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(hour_fixed, SBTH_HOUR_FIXED_W, SBTH_HOUR_FIXED_H);
	nfui_nfobject_modify_bg(hour_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nffixed_put((NFFIXED*)content_fixed, hour_fixed, SBTH_HOUR_FIXED_X, SBTH_HOUR_FIXED_Y);

	day_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(day_fixed, SBTH_DAY_FIXED_W, SBTH_DAY_FIXED_H);
	nfui_nfobject_modify_bg(day_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nffixed_put((NFFIXED*)content_fixed, day_fixed, SBTH_DAY_FIXED_X, SBTH_DAY_FIXED_Y);


// CALENDAR
	cal_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(cal_fixed, SBTH_CAL_FIXED_W, SBTH_CAL_FIXED_H);
	nfui_nfobject_show(cal_fixed);
	nfui_nfobject_modify_bg(cal_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nffixed_put((NFFIXED*)fixed1, cal_fixed, SBTH_CAL_FIXED_X, SBTH_CAL_FIXED_Y);

	// move button
	arrow_img[0][0] = nfui_get_image_from_file((IMG_CALENDAR_PRE_N_BUTTON), NULL);
	arrow_img[0][1] = nfui_get_image_from_file((IMG_CALENDAR_PRE_O_BUTTON), NULL);
	arrow_img[0][2] = nfui_get_image_from_file((IMG_CALENDAR_PRE_P_BUTTON), NULL);
	arrow_img[0][3] = nfui_get_image_from_file((IMG_CALENDAR_PRE_D_BUTTON), NULL);

	arrow_img[1][0] = nfui_get_image_from_file((IMG_CALENDAR_NEXT_N_BUTTON), NULL);
	arrow_img[1][1] = nfui_get_image_from_file((IMG_CALENDAR_NEXT_O_BUTTON), NULL);
	arrow_img[1][2] = nfui_get_image_from_file((IMG_CALENDAR_NEXT_P_BUTTON), NULL);
	arrow_img[1][3] = nfui_get_image_from_file((IMG_CALENDAR_NEXT_D_BUTTON), NULL);

	for(i=0; i<2; i++) {
		obj = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), arrow_img[i]);
		nfui_nfobject_set_size(obj, 27, 27);
		nfui_nfobject_show(obj);

		if(i == 0) {
			nfui_regi_post_event_callback(obj, post_prev_cal_event_handler);
			nfui_nffixed_put((NFFIXED*)cal_fixed, obj, 34, 7);
			g_cal_dir[0] = obj;
		}else 	   {
			nfui_regi_post_event_callback(obj, post_next_cal_event_handler);
			nfui_nffixed_put((NFFIXED*)cal_fixed, obj, 349, 7);
			g_cal_dir[1] = obj;
		}
	}

	// calendar month/year
	length = strlen(g_month_str[month - 1]);
	g_utf8_strncpy(strBuf, g_month_str[month - 1], g_utf8_strlen(g_month_str[month - 1], -1));
	g_sprintf(&strBuf[length], " %d", year);

	g_cal_text = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(607));
	nfui_nfobject_set_size(g_cal_text, (guint)(282) , (guint)(34));
	nfui_nflabel_set_align((NFLABEL*)g_cal_text, NFALIGN_CENTER,0);
	nfui_nfobject_modify_bg(g_cal_text, NFOBJECT_STATE_NORMAL, COLOR_IDX(606));
	nfui_nfobject_use_focus(g_cal_text, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(g_cal_text);
	nfui_nffixed_put((NFFIXED*)cal_fixed, g_cal_text, (guint)(63), (guint)(4));

	// CALENDAR LABEL
	gint cal_lbl_pos_x[7] = {47, 93, 137, 182, 226, 269, 315};
	for(i = 0 ; i < 7 ; i++)
	{
		if (i == 0){
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strDay[i],
										nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(609));
		}else if (i == 6){
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strDay[i],
										nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(611));
		}else{
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strDay[i],
										nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(610));
		}

		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_set_size(obj, 46, 40);
		nfui_nfobject_show(obj);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(605));
		nfui_nffixed_put((NFFIXED*)cal_fixed, obj, cal_lbl_pos_x[i], 43);
	}

	g_cal = (NFOBJECT*)cw_cld_new(search_time, 44, 40);
	nfui_regi_post_event_callback(g_cal, post_calendar_event_cb);
	nfui_nfobject_show(g_cal);
	nfui_nffixed_put((NFFIXED*)cal_fixed, g_cal, 50, 85);
	//nfui_nffixed_put((NFFIXED*)cal_fixed, g_cal, 65, 85);

	obj = nftool_normal_button_create_type3(calButton[0], SBTH_CAL_BUTTON_SIZE_W);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_regi_post_event_callback(obj, post_first_button_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)cal_fixed, obj, SBTH_CAL_FIRST_BUTTON_X, SBTH_CAL_FIRST_BUTTON_Y);
	g_first_btn = obj;

	obj = nftool_normal_button_create_type3(calButton[1], SBTH_CAL_BUTTON_SIZE_W);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_regi_post_event_callback(obj, post_last_button_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)cal_fixed, obj, SBTH_CAL_LAST_BUTTON_X, SBTH_CAL_LAST_BUTTON_Y);
	g_last_btn = obj;

//	LEFT SIDE - CHANNEL, DATE/TIME, INTERVAL
	width1[0] = SBTH_TABLE1_COLUMN1_W;
	width1[1] = SBTH_TABLE1_COLUMN2_W;

	ntb1 = (NFOBJECT*)nfui_nftable_new(SBTH_TABLE1_COLUMNS, SBTH_TABLE1_ROWS, 0, 2, width1, SBTH_LABEL_HEIGHT1);
	nfui_nfobject_show(ntb1);
	nfui_nfobject_modify_bg(ntb1, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nffixed_put((NFFIXED*)fixed1, ntb1, SBTH_TABLE1_X, SBTH_TABLE1_Y);

	for (ch = 0; ch<GUI_CHANNEL_CNT; ch++)
	{
		strCh[ch] = imalloc(sizeof(gchar) * 64);
//		DAL_get_camera_title(strCh[ch], ch);
		sprintf(strCh[ch], "CAM %d", ch+1);
	}

	for (i = 0; i < NUM_SBTH_SET_MAX; i++)
	{
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)ntb1, obj,  0, i);

		if (i == SBTH_SET_CH)		// CHANNEL
		{
			ch_obj = nfui_combobox_new(strCh, GUI_CHANNEL_CNT, 0);
			nfui_combobox_set_skin_type(NF_COMBOBOX(ch_obj), NFCOMBOBOX_TYPE_1);
			nfui_combobox_set_align(NF_COMBOBOX(ch_obj), NFALIGN_LEFT, 5);
			nfui_nfobject_show(ch_obj);
			nfui_nftable_attach((NFTABLE*)ntb1, ch_obj,  1, SBTH_SET_CH);
			nfui_regi_post_event_callback(ch_obj, post_channel_event_handler);

			for(ch=0; ch<GUI_CHANNEL_CNT; ch++)
				ifree(strCh[ch]);
		}
		else if (i == SBTH_SET_DT)		// DATE-TIME
		{
			gchar dtBuf[DATE_TIME_TEXT_LEN];

			memset(dtBuf, 0x00, DATE_TIME_TEXT_LEN);
			_get_text_datetime_thumbnail_time(THUMB_MODE_HOUR, dtBuf);

			dt_obj = (NFOBJECT*)nfui_nflabel_new_text_box(dtBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
			nfui_nflabel_set_skin_type((NFLABEL*)dt_obj, NFTEXTBOX_TYPE_INPUT);
			nfui_nflabel_set_align((NFLABEL*)dt_obj, NFALIGN_LEFT, 5);
			nfui_nfobject_show(dt_obj);
			nfui_nftable_attach((NFTABLE*)ntb1, dt_obj,  1, SBTH_SET_DT);
			nfui_regi_post_event_callback(dt_obj, post_date_time_event_handler);
		}
		else if (i == SBTH_SET_INV)		// INTERVAL
		{
			inv_obj = nfui_combobox_new(strInterval, 3, THUMB_MODE_HOUR);
			nfui_combobox_set_skin_type(NF_COMBOBOX(inv_obj), NFCOMBOBOX_TYPE_1);
			nfui_combobox_set_align(NF_COMBOBOX(inv_obj), NFALIGN_LEFT, 5);
			nfui_nfobject_show(inv_obj);
			nfui_nftable_attach((NFTABLE*)ntb1, inv_obj,  1, SBTH_SET_INV);
			nfui_regi_post_event_callback(inv_obj, post_interval_event_handler);
		}
	}


//	PREVIEW
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PREVIEW", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(662));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, SBTH_PREVIEW_LABEL_WIDTH, SBTH_LABEL_HEIGHT2);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, SBTH_PREVIEW_LABEL_X, SBTH_PREVIEW_LABEL_Y);

	/**	TEMP **/
	preview_obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(665));
	nfui_nflabel_set_align((NFLABEL*)preview_obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(preview_obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(666));
	nfui_nfobject_set_size(preview_obj, SBTH_PREVIEW_VIDEO_W, SBTH_PREVIEW_VIDEO_H);
	nfui_nfobject_use_focus(preview_obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(preview_obj);
	nfui_nffixed_put((NFFIXED*)fixed1, preview_obj, SBTH_PREVIEW_VIDEO_X, SBTH_PREVIEW_VIDEO_Y);


	/**	TEMP **/
	play_time_obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MINI_SEMI_5), COLOR_IDX(664));
	nfui_nflabel_set_align((NFLABEL*)play_time_obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(play_time_obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(play_time_obj, SBTH_PREVIEW_VIDEO_W, SBTH_LABEL_HEIGHT2);
	nfui_nfobject_use_focus(play_time_obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(play_time_obj);
	nfui_nffixed_put((NFFIXED*)fixed1, play_time_obj, SBTH_PREVIEW_TIME_X, SBTH_PREVIEW_TIME_Y);

// RECORD REASON
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strRec[0], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(638));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, SBTH_TL_REC_TEXT_PRE_W, SBTH_TL_REC_BOX_H + 4);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, SBTH_TL_REC_TEXT_PRE_X, SBTH_TL_REC_BOX_PRE_Y);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strRec[1], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(638));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, SBTH_TL_REC_TEXT_TIME_W, SBTH_TL_REC_BOX_H + 4);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, SBTH_TL_REC_TEXT_TIME_X, SBTH_TL_REC_BOX_TIME_Y);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strRec[2], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(638));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, SBTH_TL_REC_TEXT_ALARM_W, SBTH_TL_REC_BOX_H + 4);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, SBTH_TL_REC_TEXT_ALARM_X, SBTH_TL_REC_BOX_ALARM_Y);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strRec[3], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(638));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, SBTH_TL_REC_TEXT_MOT_W, SBTH_TL_REC_BOX_H + 4);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, SBTH_TL_REC_TEXT_MOT_X, SBTH_TL_REC_BOX_MOT_Y);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strRec[4], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(638));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, SBTH_TL_REC_TEXT_PANIC_W, SBTH_TL_REC_BOX_H + 4);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, SBTH_TL_REC_TEXT_PANIC_X, SBTH_TL_REC_BOX_PANIC_Y);


	memset(&figure, 0x00, sizeof(TML_FIGURE_T));
	figure.len = LEN_1350;
	figure.type = TML_HORIZONTAL;
	figure.cr_bg = UX_COLOR(631);
	figure.cr_rbar[0] = UX_COLOR(630);
	figure.cr_rbar[1] = UX_COLOR(639);
	figure.cr_rbar[2] = UX_COLOR(641);
	figure.cr_rbar[3] = UX_COLOR(643);
	figure.cr_rbar[4].red = 0x0000;
	figure.cr_rbar[4].green = 0x0000;
	figure.cr_rbar[4].blue = 0xff00;
	figure.cr_rbar[5] = UX_COLOR(645);
	figure.cr_rbar[6] = UX_COLOR(647);
	figure.cr_rbar[7] = UX_COLOR(630);
	figure.cr_rbar[8] = UX_COLOR(640);
	figure.cr_rbar[9] = UX_COLOR(642);
	figure.cr_rbar[10] = UX_COLOR(644);
	figure.cr_rbar[11].red = 0x0000;
	figure.cr_rbar[11].green = 0x0000;
	figure.cr_rbar[11].blue = 0xff00;
	figure.cr_rbar[12] = UX_COLOR(646);
	figure.cr_rbar[13] = UX_COLOR(648);
	figure.cr_ruler_bg = UX_COLOR(630);
	figure.cr_ruler_text = UX_COLOR(347);

	figure.cr_date.red = 0xff00;
	figure.cr_date.green = 0xff00;
	figure.cr_date.blue = 0x0000;
	figure.cr_playbar = UX_COLOR(358);
	figure.cr_cti = UX_COLOR(360);
	figure.ruler_brd = HEIGHT_RULER;
	figure.ch_brd = HEIGHT_CH;

	figure.pb_curtain = nfui_get_image_from_file(HRZ_CURTAIN, NULL);

	tml = tml_new(parent);
	nfui_nfobject_set_size(tml, TML_W, 42);
	nfui_nffixed_put((NFFIXED*)fixed2, tml, TML_X, TML_Y);
	tml_init(tml, &figure, TML_X, TML_Y, TML_W, 42, 1, 0);
	tml_set_cti_position(tml, 0);
	tml_set_style(tml, TML_NODRAGABLE);
	tml_set_style(tml, TML_GHOSTCTI);
	nfui_nfobject_show(tml);
	nfui_regi_post_event_callback(tml, post_timeline_event_cb);


/*
	ch_img[0] = nfui_get_image_from_file((IMG_N_TML_CH), NULL);
	ch_img[1] = nfui_get_image_from_file((IMG_N_TML_CH), NULL);
	ch_img[2] = nfui_get_image_from_file((IMG_N_TML_CH), NULL);
	ch_img[3] = nfui_get_image_from_file((IMG_N_TML_CH), NULL);
	nfui_get_pixbuf_size(ch_img[0], &size_w, &size_h);
	sprintf(strBuf, "%s", "1");
	obj = (NFOBJECT*)nfui_nfbutton_new();
	tml_ch_obj = obj;
	nfui_nfbutton_set_text(obj, strBuf);
	nfui_nfbutton_set_pango_font(obj, nffont_get_pango_font(NFFONT_MINI_SEMI), font_color);
	nfui_nfbutton_set_image(NF_BUTTON(obj), ch_img);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, SBT_TML_CH_BTN_X, SBT_TML_CH_BTN_Y);
	nfui_regi_post_event_callback(obj, post_ch_event_handler);
*/

	obj = (NFOBJECT*)nfui_nfbutton_new();
	g_prev_btn = obj;
	nfui_nfbutton_set_image(NF_BUTTON(obj), arrow_img[0]);
	nfui_nfobject_set_size(obj, 27, 27);
	//nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, 0, 0);
	nfui_regi_post_event_callback(obj, post_prev_button_event_cb);

	obj = (NFOBJECT*)nfui_nfbutton_new();
	g_next_btn = obj;
	nfui_nfbutton_set_image(NF_BUTTON(obj), arrow_img[1]);
	nfui_nfobject_set_size(obj, 27, 27);
	//nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, 0, 0);
	nfui_regi_post_event_callback(obj, post_next_button_event_cb);

// THUMBNAIL PICTURE - MINUTES
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("THUMBNAIL : MINUTES", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(660));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, THUMB_TITLE_W, SBTH_LABEL_HEIGHT2);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)min_fixed, obj, THUMB_TITLE_X, THUMB_TITLE_Y);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(661));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, 100, SBTH_LABEL_HEIGHT2);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)min_fixed, obj, SBTH_MIN_FIXED_W-100-2, THUMB_TITLE_Y);
	nfui_nfobject_set_data(min_fixed, "CMPL RATE", (gpointer)obj);

	pos_x = THUMB_MIN_LABEL_X;
	pos_y = THUMB_MIN_LABEL_Y;

	s_time = search_time + 1;
	e_time = s_time + 59;

	for (row = 0; row < THUMB_MIN_CELL_ROW; row++)
	{
		for (col = 0; col < THUMB_MIN_CELL_COL; col++)
		{
			if (col+THUMB_MIN_CELL_COL*row < THUMB_MIN_CELL_MAX)
			{
				i = col+THUMB_MIN_CELL_COL*row;

				thumb_obj[THUMB_MODE_MIN][i] = (NFOBJECT*)nfui_nfthumbnail_new("", nffont_get_pango_font(NFFONT_MINI_SEMI_3));
				nfui_nfthumbnail_subject_label_height((NFTHUMBNAIL*)thumb_obj[THUMB_MODE_MIN][i], 17);
				nfui_nfthumbnail_set_image_buf((NFTHUMBNAIL*)thumb_obj[THUMB_MODE_MIN][i], min_image[i]);
				nfui_nfthumbnail_set_channel((NFTHUMBNAIL*)thumb_obj[THUMB_MODE_MIN][i], 0);
				nfui_nfthumbnail_set_period((NFTHUMBNAIL*)thumb_obj[THUMB_MODE_MIN][i], s_time, e_time);
				nfui_nfthumbnail_set_image_size((NFTHUMBNAIL*)thumb_obj[THUMB_MODE_MIN][i], THUMB_MIN_CELL_W-2, THUMB_MIN_CELL_H-17-2);
				nfui_nfobject_set_size(thumb_obj[THUMB_MODE_MIN][i], THUMB_MIN_CELL_W, THUMB_MIN_CELL_H);
				nfui_nfobject_hide(thumb_obj[THUMB_MODE_MIN][i]);
				nfui_regi_post_event_callback(thumb_obj[THUMB_MODE_MIN][i], post_pic_min_event_handler);
				nfui_nffixed_put((NFFIXED*)min_fixed, thumb_obj[THUMB_MODE_MIN][i], pos_x, pos_y);

				pos_x += THUMB_MIN_CELL_W+THUMB_MIN_CELL_COL_GAP;
        		s_time += 60;
        		e_time += 60;
			}
		}

		pos_x = THUMB_MIN_LABEL_X;
		pos_y += THUMB_MIN_CELL_H+THUMB_MIN_CELL_ROW_GAP;
	}

// THUMBNAIL PICTURE - HOURS
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("THUMBNAIL : HOURS", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(660));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, THUMB_TITLE_W, SBTH_LABEL_HEIGHT2);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)hour_fixed, obj, THUMB_TITLE_X, THUMB_TITLE_Y);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(661));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, 100, SBTH_LABEL_HEIGHT2);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)hour_fixed, obj, SBTH_HOUR_FIXED_W-100-10, THUMB_TITLE_Y);
	nfui_nfobject_set_data(hour_fixed, "THUMB CNT",  GINT_TO_POINTER(THUMB_HOUR_CELL_MAX));
	nfui_nfobject_set_data(hour_fixed, "CMPL RATE", obj);

	pos_x = THUMB_HOUR_LABEL_X;
	pos_y = THUMB_HOUR_LABEL_Y;

	s_time = search_time + 1;
	e_time = s_time + 3599;

	for (row = 0; row < THUMB_HOUR_CELL_ROW; row++)
	{
		for (col = 0; col < THUMB_HOUR_CELL_COL; col++)
		{
			if (col+THUMB_HOUR_CELL_COL*row < THUMB_HOUR_CELL_MAX)
			{
				i = col+THUMB_HOUR_CELL_COL*row;

				thumb_obj[THUMB_MODE_HOUR][i] = (NFOBJECT*)nfui_nfthumbnail_new("", nffont_get_pango_font(NFFONT_MINI_SEMI_3));
				nfui_nfthumbnail_subject_label_height((NFTHUMBNAIL*)thumb_obj[THUMB_MODE_HOUR][i], 17);
				nfui_nfthumbnail_set_image_buf((NFTHUMBNAIL*)thumb_obj[THUMB_MODE_HOUR][i], hour_image[i]);
				nfui_nfthumbnail_set_channel((NFTHUMBNAIL*)thumb_obj[THUMB_MODE_HOUR][i], 0);
				nfui_nfthumbnail_set_period((NFTHUMBNAIL*)thumb_obj[THUMB_MODE_HOUR][i], s_time, e_time);
				nfui_nfthumbnail_set_image_size((NFTHUMBNAIL*)thumb_obj[THUMB_MODE_HOUR][i], THUMB_HOUR_CELL_W-2, THUMB_HOUR_CELL_H-17-2);
				nfui_nfobject_set_size(thumb_obj[THUMB_MODE_HOUR][i], THUMB_HOUR_CELL_W, THUMB_HOUR_CELL_H);
				nfui_nfobject_hide(thumb_obj[THUMB_MODE_HOUR][i]);
				nfui_regi_post_event_callback(thumb_obj[THUMB_MODE_HOUR][i], post_pic_hour_event_handler);
				nfui_nffixed_put((NFFIXED*)hour_fixed, thumb_obj[THUMB_MODE_HOUR][i], pos_x, pos_y);

				pos_x += THUMB_HOUR_CELL_W+THUMB_HOUR_CELL_COL_GAP;
				s_time += 3600;
				e_time += 3600;
			}
		}

		pos_x = THUMB_HOUR_LABEL_X;
		pos_y += THUMB_HOUR_CELL_H+THUMB_HOUR_CELL_ROW_GAP;
	}


// THUMBNAIL PICTURE - DAYS
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("THUMBNAIL : DAYS", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(660));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, THUMB_TITLE_W, SBTH_LABEL_HEIGHT2);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)day_fixed, obj, THUMB_TITLE_X, THUMB_TITLE_Y);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(661));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, 100, SBTH_LABEL_HEIGHT2);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)day_fixed, obj, SBTH_DAY_FIXED_W-100, THUMB_TITLE_Y);
	nfui_nfobject_set_data(day_fixed, "CMPL RATE", (gpointer)obj);
	nfui_nfobject_set_data(day_fixed, "THUMB CNT",  GINT_TO_POINTER(THUMB_DAY_CELL_MAX));

	pos_x = THUMB_DAY_LABEL_X;
	pos_y = THUMB_DAY_LABEL_Y;

	s_time = search_time + 1;
	e_time = s_time + 86399;

	for (row = 0; row < THUMB_DAY_CELL_ROW; row++)
	{
		for (col = 0; col < THUMB_DAY_CELL_COL; col++)
		{
			if (col+THUMB_DAY_CELL_COL*row < THUMB_DAY_CELL_MAX)
			{
				i = col+THUMB_DAY_CELL_COL*row;

				thumb_obj[THUMB_MODE_DAY][i] = (NFOBJECT*)nfui_nfthumbnail_new("", nffont_get_pango_font(NFFONT_MINI_SEMI_3));
				nfui_nfthumbnail_subject_label_height((NFTHUMBNAIL*)thumb_obj[THUMB_MODE_DAY][i], 17);
				nfui_nfthumbnail_set_image_buf((NFTHUMBNAIL*)thumb_obj[THUMB_MODE_DAY][i], day_image[i]);
				nfui_nfthumbnail_set_channel((NFTHUMBNAIL*)thumb_obj[THUMB_MODE_DAY][i], 0);
				nfui_nfthumbnail_set_period((NFTHUMBNAIL*)thumb_obj[THUMB_MODE_DAY][i], s_time, e_time);
				nfui_nfthumbnail_set_image_size((NFTHUMBNAIL*)thumb_obj[THUMB_MODE_DAY][i], THUMB_DAY_CELL_W-2, THUMB_DAY_CELL_H-17-2);
				nfui_nfobject_set_size(thumb_obj[THUMB_MODE_DAY][i], THUMB_DAY_CELL_W, THUMB_DAY_CELL_H);
				nfui_nfobject_hide(thumb_obj[THUMB_MODE_DAY][i]);
				nfui_regi_post_event_callback(thumb_obj[THUMB_MODE_DAY][i], post_pic_day_event_handler);
				nfui_nffixed_put((NFFIXED*)day_fixed, thumb_obj[THUMB_MODE_DAY][i], pos_x, pos_y);

				pos_x += THUMB_DAY_CELL_W+THUMB_DAY_CELL_COL_GAP;
				s_time += 86400;
				e_time += 86400;
			}
		}

		pos_x = THUMB_DAY_LABEL_X;
		pos_y += THUMB_DAY_CELL_H+THUMB_DAY_CELL_ROW_GAP;
	}

// PLAYBACK / CLOSE BUTTON
	obj = nftool_normal_button_create_type1(strButton[0], MENU_BTN_WIDTH);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_H_BTN_R2_X, MENU_H_BTN_Y);
	nfui_regi_post_event_callback(obj, post_playbutton_event_handler);
	playbtn_obj = obj;

	obj = nftool_normal_button_create_type2(strButton[1], MENU_BTN_WIDTH);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_H_BTN_R1_X, MENU_H_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

	nfui_regi_pre_event_callback(cal_fixed, pre_cal_fixed_event_handler);
	nfui_regi_pre_event_callback(fixed1, pre_fixed1_event_handler);
	nfui_regi_pre_event_callback(fixed2, pre_fixed2_event_handler);

	nfui_regi_post_event_callback(min_fixed, post_min_fixed_event_handler);
	nfui_regi_post_event_callback(hour_fixed, post_hour_fixed_event_handler);
	nfui_regi_post_event_callback(day_fixed, post_day_fixed_event_handler);

	nfui_regi_post_event_callback(parent, post_page_event_handler);

	nfui_nfobject_hide(min_fixed);
	nfui_nfobject_hide(day_fixed);
	nfui_nfobject_show(hour_fixed);

	uxm_reg_imsg_event(tml, INFY_TML_DATE_CHANGED);
	uxm_reg_imsg_event(tml, INFY_TML_PLAY_CHANGED);
	uxm_reg_imsg_event(tml, INFY_TML_START_CHANGED);
	uxm_reg_imsg_event(tml, INFY_TML_END_CHANGED);
	uxm_reg_imsg_event(tml, INFY_TML_SCROLL_UP);
	uxm_reg_imsg_event(tml, INFY_TML_SCROLL_DOWN);
	uxm_reg_imsg_event(tml, INFY_TML_DOUBLE_CLICKED);

	uxm_reg_imsg_event(min_fixed, INFY_THUMBNAIL_CMPL_OBJ);
	uxm_reg_imsg_event(min_fixed, INFY_THUMBNAIL_TOTALLY_CMPL);
	uxm_reg_imsg_event(hour_fixed, INFY_THUMBNAIL_CMPL_OBJ);
	uxm_reg_imsg_event(hour_fixed, INFY_THUMBNAIL_TOTALLY_CMPL);
	uxm_reg_imsg_event(day_fixed, INFY_THUMBNAIL_CMPL_OBJ);
	uxm_reg_imsg_event(day_fixed, INFY_THUMBNAIL_TOTALLY_CMPL);

	tml_reset_cur_day(tml, search_time);
	tml_set_playstick_time_t(tml, search_time);
	_control_shift_button();
}

gboolean vw_SearchByThumbnail_tab_out_handler()
{
	guint i;

	if (nfui_combobox_get_cur_index(NF_COMBOBOX(inv_obj)) == THUMB_MODE_DAY)
	{
		for (i = 0; i < THUMB_DAY_CELL_MAX ; i++)
			nfui_nfobject_hide(thumb_obj[THUMB_MODE_DAY][i]);
	}
	else if (nfui_combobox_get_cur_index(NF_COMBOBOX(inv_obj)) == THUMB_MODE_HOUR)
	{
		for (i = 0; i < THUMB_HOUR_CELL_MAX ; i++)
			nfui_nfobject_hide(thumb_obj[THUMB_MODE_HOUR][i]);
	}

	_preview_obj_off("");
	vsm_playback_preview_stop();

	return FALSE;
}

gboolean vw_SearchByThumbnail_tab_in_handler()
{
	gint i, thumb_cnt;

	search_time = stm_get_time_t();
	g_init_thumb = TRUE;

	tml_reset_cur_day(tml, search_time);
	tml_set_playstick_time_t(tml, search_time);
	tml_repaint(tml);

	_control_shift_button();

	if (nfui_combobox_get_cur_index(NF_COMBOBOX(inv_obj)) == THUMB_MODE_DAY)
	{
		_calc_days_count(search_time);
		thumb_cnt = GPOINTER_TO_INT(nfui_nfobject_get_data(day_fixed, "THUMB CNT"));

		for (i = 0; i < thumb_cnt; i++)
			nfui_nfobject_show(thumb_obj[THUMB_MODE_DAY][i]);
	}
	else if (nfui_combobox_get_cur_index(NF_COMBOBOX(inv_obj)) == THUMB_MODE_HOUR)
	{
		_calc_hours_count(search_time);
		thumb_cnt = GPOINTER_TO_INT(nfui_nfobject_get_data(hour_fixed, "THUMB CNT"));

		for (i = 0; i < thumb_cnt; i++)
			nfui_nfobject_show(thumb_obj[THUMB_MODE_HOUR][i]);
	}

	return FALSE;
}

gboolean vw_SearchByThumbnail_tab_show()
{
	search_time = stm_get_time_t();
	tml_zoom_min(tml);
	tml_reset_cur_day(tml, search_time);

	vw_SearchByThumbnail_tab_in_handler();
	return FALSE;
}
