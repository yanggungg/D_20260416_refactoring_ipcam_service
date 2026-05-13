#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"
#include "support/nf_ui_common_data.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfimage.h"
#include "objects/nfcombobox.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfbutton.h"

#include "vw_set_date_time.h"
#include "ix_func.h"


#define SDT_WIN_SIZE_W				(485)
#define SDT_WIN_SIZE_H				(289)

#define SDT_TITLE_H					(36)

#define SDT_LEFT_GAP				(33)

#define SDT_LABEL_Y					(60)
#define SDT_LABEL_W					(63)
#define SDT_LABEL_H					(22)

#define SDT_CELL_W					(SDT_LABEL_W)
#define SDT_CELL_H					(40)

#define SDT_BTN_W					(192)	

#define SDT_OK_BTN_X				(44)	
#define SDT_OK_BTN_Y				(SDT_WIN_SIZE_H - 68)	

#define SDT_CANCEL_BTN_X			(SDT_OK_BTN_X+SDT_BTN_W+6)	
#define SDT_CANCEL_BTN_Y			(SDT_OK_BTN_Y)		

#define	SDT_LOWER_TIMELIMIT			NF_LOWER_TIMELIMIT	
#define	SDT_UPPER_TIMELIMIT			NF_UPPER_TIMELIMIT	

#define	DAY_TO_SEC					((time_t)86400)
#define	HOUR_TO_SEC					((time_t)3600)
#define	MIN_TO_SEC					((time_t)60)



enum {
	SET_DT_YEAR = 0,
	SET_DT_MON,
	SET_DT_DAY,
	SET_DT_HOUR,
	SET_DT_MIN,
	SET_DT_SEC,
	NUM_SET_DT,
};


typedef struct _DATETIME_T
{
	time_t lower;	
	time_t upper;	

	time_t tmp;	
	time_t ret;	
} DATETIME_T;

static DATETIME_T idt;

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *time_label[NUM_SET_DT];
static NFOBJECT *time_cell[NUM_SET_DT];

static NFOBJECT *time_up[NUM_SET_DT];
static NFOBJECT *time_down[NUM_SET_DT];



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

	return (mktime(tm) - timezone);
}


static void prvRefreshDateTime()
{
	guint temp;
	struct tm stTime;
	gchar strTemp[8];

	guint i;
	gint year, mon, day, hour, min, sec;

	//ifn_get_local_day(idt.tmp, &year, &mon, &day);
	//ifn_get_local_hourmin(idt.tmp, &hour, &min, &sec);
    ifn_get_gmt_datetime(idt.tmp, &year, &mon, &day, &hour, &min, &sec);

	memset(strTemp, 0, sizeof(strTemp));

	g_sprintf(strTemp, "%04d", year);		nfui_nflabel_set_text((NFLABEL*)(time_cell[SET_DT_YEAR]),	strTemp);
	g_sprintf(strTemp, "%02d", mon);		nfui_nflabel_set_text((NFLABEL*)(time_cell[SET_DT_MON]),	strTemp);
	g_sprintf(strTemp, "%02d", day);		nfui_nflabel_set_text((NFLABEL*)(time_cell[SET_DT_DAY]),	strTemp);

	g_sprintf(strTemp, "%02d", hour);		nfui_nflabel_set_text((NFLABEL*)(time_cell[SET_DT_HOUR]),	strTemp);
	g_sprintf(strTemp, "%02d", min);		nfui_nflabel_set_text((NFLABEL*)(time_cell[SET_DT_MIN]),	strTemp);
	g_sprintf(strTemp, "%02d", sec);		nfui_nflabel_set_text((NFLABEL*)(time_cell[SET_DT_SEC]),	strTemp);

	for (i = 0; i < NUM_SET_DT; i++)
		nfui_signal_emit((NFOBJECT*)time_cell[i], GDK_EXPOSE, TRUE);
}

static void _process_up(NFOBJECT *obj)
{
	NFOBJECT *topwin;
	struct tm stTime;

	gmtime_r(&idt.tmp, &stTime);

	if(obj == time_up[SET_DT_YEAR])
	{
		stTime.tm_year++;
		idt.tmp = mktimeWithCheck(&stTime);
		
	} else if (obj == time_up[SET_DT_MON]) {
		if(stTime.tm_mon == 11)
		{
			stTime.tm_mon = 0;
			stTime.tm_year++;
		}
		else
			stTime.tm_mon++;

		idt.tmp = mktimeWithCheck(&stTime);
		
	} else if (obj == time_up[SET_DT_DAY]) {
		idt.tmp += DAY_TO_SEC;
		
	} else if (obj == time_up[SET_DT_HOUR]) {
		idt.tmp += HOUR_TO_SEC;

	} else if (obj == time_up[SET_DT_MIN]) {
		idt.tmp += MIN_TO_SEC;

	} else if (obj == time_up[SET_DT_SEC]) {
		idt.tmp++;

	}		

	if(idt.tmp > idt.upper)
		idt.tmp = idt.upper;
	else if(idt.tmp < idt.lower)
		idt.tmp = idt.lower;

	prvRefreshDateTime();	
}

static void _process_down(NFOBJECT *obj)
{
	NFOBJECT *topwin;

	struct tm stTime;
	
	gmtime_r(&idt.tmp, &stTime);

	if(obj == time_down[SET_DT_YEAR])
	{
		if(stTime.tm_year)	stTime.tm_year--;
		else				idt.tmp = 0;

		idt.tmp = mktimeWithCheck(&stTime);
		
	} else if (obj == time_down[SET_DT_MON]) {
		if(stTime.tm_mon == 0)
		{
			if(stTime.tm_year)
			{
				stTime.tm_mon = 11;
				stTime.tm_year--;
			}
			else
				idt.tmp = 0;
		}
		else
			stTime.tm_mon--;

		idt.tmp = mktimeWithCheck(&stTime);
		
	} else if (obj == time_down[SET_DT_DAY]) {
		if(idt.tmp > DAY_TO_SEC) 
			idt.tmp -= DAY_TO_SEC;
		else
			idt.tmp = 0;
		
	} else if (obj == time_down[SET_DT_HOUR]) {
		if(idt.tmp > HOUR_TO_SEC)
			idt.tmp -= HOUR_TO_SEC;
		else
			idt.tmp = 0;
			
	} else if (obj == time_down[SET_DT_MIN]) {
		if(idt.tmp > MIN_TO_SEC)
			idt.tmp -= MIN_TO_SEC;
		else
			idt.tmp = 0;
			
	} else if (obj == time_down[SET_DT_SEC]) {
		if(idt.tmp)
			(idt.tmp)--;
		else
			idt.tmp = 0;
	}

	if(idt.tmp < idt.lower)
		idt.tmp = idt.lower;
	else if(idt.tmp > idt.upper)
		idt.tmp = idt.upper;

	prvRefreshDateTime();	
}

static gboolean post_upbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		_process_up(obj);
	}

	return FALSE;
}


static gboolean post_downbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		_process_down(obj);		
	}

	return FALSE;
}

static gboolean post_time_cell_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_SCROLL)
	{
		gint i;
		GdkEventScroll *sevt;

		sevt = (GdkEventScroll*)evt;
		
		for (i = 0; i < NUM_SET_DT; i++)
		{
			if (obj == time_cell[i]) break;
		}

		if (i == NUM_SET_DT) return FALSE;

		if(sevt->direction == GDK_SCROLL_UP) {	
			_process_up(time_up[i]);
		}

		if(sevt->direction == GDK_SCROLL_DOWN) {
			_process_down(time_down[i]);
		}
	}

	return FALSE;
}

static gboolean post_okbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *topwin;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		g_message("%s, %d time change", __FUNCTION__, __LINE__);
		
		g_message("year:%s, month:%s, day:%s, hour:%s, min:%s, sec:%s", 
			nfui_nflabel_get_text((NFLABEL*)(time_cell[SET_DT_YEAR])),
			nfui_nflabel_get_text((NFLABEL*)(time_cell[SET_DT_MON])),
			nfui_nflabel_get_text((NFLABEL*)(time_cell[SET_DT_DAY])),
			nfui_nflabel_get_text((NFLABEL*)(time_cell[SET_DT_HOUR])),
			nfui_nflabel_get_text((NFLABEL*)(time_cell[SET_DT_MIN])),
			nfui_nflabel_get_text((NFLABEL*)(time_cell[SET_DT_SEC])));

		idt.ret = idt.tmp;

		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);	
	
	}

	return FALSE;
}


static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *topwin;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}
	
		idt.ret = 0;
		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);	

	}

	return FALSE;
}


static gboolean post_window_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case GDK_EXPOSE:

		break;

		case GDK_DELETE:
			g_curwnd = 0;
			gtk_main_quit();
		break;
	}

	return FALSE;
}

static gboolean post_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if(evt->type == GDK_EXPOSE) {
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
    
		nfui_nfobject_gc_unref(gc);
	}
    else if (evt->type == GDK_DELETE)
    {
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
    }

	return FALSE;
}

time_t VW_Set_DateTime_Open_Ver2(NFWINDOW *parent, gchar *title, gint sdt_x, gint sdt_y, time_t c_time, SDT_TYPE_E type, time_t start_time, time_t end_time)
{
	NFOBJECT *sdt_win;
	NFOBJECT *sdt_fixed;

	NFOBJECT *obj;
	NFOBJECT *ok_btn;

	GdkPixbuf *up_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *down_img[NFOBJECT_STATE_COUNT];

	const gchar *sdt_str[] = {"YEAR", "MONTH1", "DAY", "HOUR", "MIN", "SEC"};

	gchar strTemp[8];

	guint pos_x, pos_y;
	guint size_w, size_h;

	guint type_num = NUM_SET_DT;
	guint type_gap_x = 0;

	guint i;
	gint year, mon, day, hour, min, sec;

	if (c_time < start_time)
	{
		g_warning("%s, %d", __FUNCTION__, __LINE__);
		c_time = start_time;
	}
	else if (c_time > end_time)
	{
		g_warning("%s, %d", __FUNCTION__, __LINE__);
		c_time = end_time;
	}

	if (sdt_x < 0) sdt_x = 0;
	if (sdt_y < 0) sdt_y = 0;
	if (sdt_x + SDT_WIN_SIZE_W > 1920) sdt_x = 1920 - SDT_WIN_SIZE_W;
	if (sdt_y + SDT_WIN_SIZE_H > 1080) sdt_y = 1080 - SDT_WIN_SIZE_H;

// <---- IMAGE LOAD
	up_img[0] = nfui_get_image_from_file((IMG_N_DATETIME_UP), NULL);
	up_img[1] = nfui_get_image_from_file((IMG_O_DATETIME_UP), NULL);
	up_img[2] = nfui_get_image_from_file((IMG_P_DATETIME_UP), NULL);
	up_img[3] = nfui_get_image_from_file((IMG_D_DATETIME_UP), NULL);

	down_img[0] = nfui_get_image_from_file((IMG_N_DATETIME_DOWN), NULL);
	down_img[1] = nfui_get_image_from_file((IMG_O_DATETIME_DOWN), NULL);
	down_img[2] = nfui_get_image_from_file((IMG_P_DATETIME_DOWN), NULL);
	down_img[3] = nfui_get_image_from_file((IMG_D_DATETIME_DOWN), NULL);

	nfui_get_pixbuf_size(up_img[0], &size_w, &size_h);

	memset(strTemp, 0, sizeof(strTemp));
	memset(&idt, 0, sizeof(DATETIME_T));

	idt.tmp = c_time;
	idt.ret = c_time;
	
	idt.lower = start_time;
	idt.upper = end_time;
			
	
// <---- WINDOW
	sdt_win = (NFOBJECT*)nfui_nfwindow_new(parent, sdt_x, sdt_y, SDT_WIN_SIZE_W, SDT_WIN_SIZE_H);
	g_curwnd = sdt_win;
	nfui_nfwindow_set_title(sdt_win, "SYSTEM - DATE/TIME SET POPUP");
	nfui_nfobject_modify_bg(sdt_win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_regi_post_event_callback(sdt_win, post_window_event_cb);

// <---- FIXED
	sdt_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(sdt_fixed, SDT_WIN_SIZE_W, SDT_WIN_SIZE_H);
	nfui_regi_post_event_callback(sdt_fixed, post_fixed_event_cb);
	nfui_nfobject_show(sdt_fixed);

// <---- TITLE
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(title, nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, 412, SDT_TITLE_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)sdt_fixed, obj, SDT_LEFT_GAP, 4);


	if (type == SDT_TYPE_YEAR)
	{
		type_num = SET_DT_YEAR;
		type_gap_x = (412 - SDT_LABEL_W)/2;
	}
	else if (type == SDT_TYPE_MON)
	{
		type_num = SET_DT_MON;
		type_gap_x = (412 - (SDT_LABEL_W*2 + 5))/2;
	}
	else if (type == SDT_TYPE_DAY)
	{
		type_num = SET_DT_DAY;
		type_gap_x = (412 - (SDT_LABEL_W*3 + 5*2))/2;
	}
	else if (type == SDT_TYPE_HOUR)
	{
		type_num = SET_DT_HOUR;
		type_gap_x = (412 - (SDT_LABEL_W*4 + 5*2 + 14))/2;
	}
	else if (type == SDT_TYPE_MIN)
	{
		type_num = SET_DT_MIN;
		type_gap_x = (412 - (SDT_LABEL_W*5 + 5*3 + 14))/2;
	}

	pos_x = SDT_LEFT_GAP + type_gap_x;
	pos_y = SDT_LABEL_Y;	

	ifn_get_gmt_day(c_time, &year, &mon, &day);
	ifn_get_gmt_hourmin(c_time, &hour, &min, &sec);

	for (i = 0; i < NUM_SET_DT; i++)
	{

// <---- LABEL
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(sdt_str[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
		nfui_nfobject_set_size(obj, SDT_LABEL_W, SDT_LABEL_H);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)sdt_fixed, obj, pos_x, pos_y);


// <---- TIME UP BUTTON
		pos_y += SDT_LABEL_H + 2;
	
		time_up[i] = nfui_nfbutton_new();
		nfui_nfbutton_set_image((NFBUTTON*)time_up[i], up_img);
		nfui_nfobject_show(time_up[i]);
		nfui_nffixed_put((NFFIXED*)sdt_fixed, time_up[i], pos_x, pos_y);
		nfui_regi_post_event_callback(time_up[i], post_upbutton_event_handler);


// <---- TIME DISPLAY CELL
		pos_y += size_h + 2;

		if (i == SET_DT_YEAR)			g_sprintf(strTemp, "%04d", year);
		else if (i == SET_DT_MON)		g_sprintf(strTemp, "%02d", mon);
		else if (i == SET_DT_DAY)		g_sprintf(strTemp, "%02d", day);
		else if (i == SET_DT_HOUR)		g_sprintf(strTemp, "%02d", hour);
		else if (i == SET_DT_MIN)		g_sprintf(strTemp, "%02d", min);
		else if (i == SET_DT_SEC)		g_sprintf(strTemp, "%02d", sec);

		time_cell[i] = (NFOBJECT*)nfui_nflabel_new_text_box(strTemp, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)time_cell[i], NFTEXTBOX_TYPE_POPUP_INPUT);
		nfui_nfobject_set_size(time_cell[i], SDT_CELL_W, SDT_CELL_H);
		nfui_nflabel_set_align((NFLABEL*)time_cell[i], NFALIGN_CENTER, 0);
		nfui_nfobject_use_focus(time_cell[i], NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(time_cell[i]);
		nfui_nffixed_put((NFFIXED*)sdt_fixed, time_cell[i], pos_x, pos_y);
		nfui_regi_post_event_callback(time_cell[i], post_time_cell_event_handler);

// <---- TIME DOWN BUTTON
		pos_y += SDT_CELL_H + 2;

		time_down[i] = nfui_nfbutton_new();
		nfui_nfbutton_set_image((NFBUTTON*)time_down[i], down_img);
		nfui_nfobject_show(time_down[i]);
		nfui_nffixed_put((NFFIXED*)sdt_fixed, time_down[i], pos_x, pos_y);
		nfui_regi_post_event_callback(time_down[i], post_downbutton_event_handler);

		if ( i > type_num)
		{
			nfui_nfobject_hide(obj);
			nfui_nfobject_hide(time_up[i]);
			nfui_nfobject_hide(time_cell[i]);
			nfui_nfobject_hide(time_down[i]);			
		}

		if (i == SET_DT_DAY)
			pos_x += SDT_LABEL_W+14;
		else
			pos_x += SDT_LABEL_W+5;

		pos_y = SDT_LABEL_Y;

	}

// <---- OK, CANCEL BUTTON
	obj = nftool_normal_button_create_type1("OK", SDT_BTN_W);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)sdt_fixed, obj, SDT_OK_BTN_X, SDT_OK_BTN_Y);
	nfui_regi_post_event_callback(obj, post_okbutton_event_handler);
	ok_btn = obj;

	obj = nftool_normal_button_create_type1("CANCEL", SDT_BTN_W);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)sdt_fixed, obj, SDT_CANCEL_BTN_X, SDT_CANCEL_BTN_Y);
	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);


	nfui_nfwindow_add((NFWINDOW*)sdt_win, sdt_fixed);
	nfui_run_main_event_handler(sdt_win);
	nfui_nfobject_show(sdt_win);

	nfui_make_key_hierarchy(sdt_win);
	nfui_set_key_focus(ok_btn, TRUE);
	nfui_page_open(PGID_SET_DATE_TIME_POPUP, sdt_win, nfui_get_last_user());
	gtk_main();

	nfui_page_close(PGID_SET_DATE_TIME_POPUP, sdt_win);
	return idt.ret;
}

