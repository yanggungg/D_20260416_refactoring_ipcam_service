#include "support/event_loop.h"
#include "nf_afx.h"
#include "vw_holiday_set.h"

#define HOLIDAY_WIN_SIZE_W					(652)
#define HOLIDAY_WIN_SIZE_H					(448)

#define HOLIDAY_WIN_SIZE_X					((DISPLAY_ACTIVE_WIDTH - HOLIDAY_WIN_SIZE_W)/2)
#define HOLIDAY_WIN_SIZE_Y					((DISPLAY_ACTIVE_HEIGHT - HOLIDAY_WIN_SIZE_H)/2)

#define HOLIDAY_TITLE_H						(36)

// <---- OK, CLOSE BUTTON
#define HOLIDAY_OK_BTN_X					(146)
#define HOLIDAY_OK_BTN_Y					(HOLIDAY_WIN_SIZE_H - 74)
#define HOLIDAY_OK_BTN_W					(174)

#define HOLIDAY_CLOSE_BTN_X					(HOLIDAY_OK_BTN_X + HOLIDAY_OK_BTN_W + 6)
#define HOLIDAY_CLOSE_BTN_Y					(HOLIDAY_OK_BTN_Y)
#define HOLIDAY_CLOSE_BTN_W					(174)

#define SUN 0
#define MON 1
#define TUE 2
#define WED 3
#define THU 4
#define FRI 5
#define SAT 6

#define FIRST 1
#define SECOND 2
#define THIRD 3
#define FOURTH 4
#define FIFTH 5
#define SIXTH 6

#define HOLIDAY_DATE_LEN 8

#define	SUBJECT_LABEL_WIDTH			(400)

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *holiday_obj = NULL;
static NFOBJECT *holiday_set_obj = NULL;
static NFOBJECT *wait_pop = NULL;
static NFOBJECT *holiday_up_win;
static NFOBJECT *prog_pop = NULL;
static NFOBJECT *cld_text = NULL;
static NFOBJECT *dt_obj;
static NFOBJECT *from_datetime_obj;
static NFOBJECT *to_datetime_obj;

static NFOBJECT *holi_type_obj[5];

static NFOBJECT *cld_prev;
static time_t from_utime, to_utime;
static int g_ret = 0;
static time_t cur_time = 0;  



static gboolean post_okbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint i, res, idx;
	NFOBJECT *topwin = NULL;
	gint comboIdx = 0;
	UsrDefHolidayData holiTmp;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)			// mouse right button
			return FALSE;

		for (i = 0; i <5; i++)
		{
			if (nfui_radio_button_get_toggled((NFBUTTON*)holi_type_obj[i])) g_ret = i;
		}

		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);
	}

	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *topwin = NULL;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		g_ret = -1;
		topwin = nfui_nfobject_get_top(obj);
		nftool_destroy_setup_window(topwin);
	}

	return FALSE;
}

static gboolean pre_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	switch(event->type) {
		case GDK_EXPOSE:
    		drawable = nfui_nfobject_get_window(obj);
    		gc = nfui_nfobject_get_gc(obj);

            nfui_nfobject_get_size(obj, &size_w, &size_h);
            pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
            nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
        
    		nfui_nfobject_gc_unref(gc);		
			break;

		case GDK_BUTTON_RELEASE:
			break;					

		case GDK_DELETE:
            nfui_nfobject_get_size(obj, &size_w, &size_h);
            nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);		
			break;

		default:
			break;
	}

	return FALSE;
}

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
		gtk_main_quit();
	}

	return FALSE;
}

gint VW_Schedule_HolidaySet_Open(NFWINDOW *parent, int dayOfWeek, int week, int mon,int day)
{
	NFOBJECT  *main_fixed;
	NFOBJECT  *obj;
	NFOBJECT  *cal_fixed;
	GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];  

	guint li_size_w, li_size_h;
	guint i;
	int c_utime, c_year, c_month, c_day, c_hour, c_min, c_sec;
	int year, month, hour, min, sec;
	time_t tim;
	time_t cur_utime;
	int strSel;
	char cur_buffer[100];
	char weekStr[50];
	char dayOfWeekStr[50];
	char comboString[4][50];
	gint size_w, size_h;
	GSList *slist = NULL;
	gchar strRadio[128];
	
	gchar hol_pop_str[5][30] = {
		"Does not repeat",
		"Weekly on",
		"Annually on",
		"Monthly on the",
		"Every Month"
	};

	gchar mon_str[12][64] = {
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

	gchar ord_str[5][10] = {
		"First",
		"Second",
		"Third",
		"Fourth",
		"Fifth"
	};

	gchar yoil_str[7][10] = {
		"SUNDAY",
		"MONDAY",
		"TUESDAY",
		"WEDNESDAY",
		"THURSDAY",
		"FRIDAY",
		"SATURDAY"
	};


	g_ret = -1;
	
	radio_img[0] = nfui_get_image_from_file((IMG_N_POPUP_RADIO_OFF), NULL);
	radio_img[1] = nfui_get_image_from_file((IMG_O_POPUP_RADIO_ON), NULL);
	radio_img[2] = nfui_get_image_from_file((IMG_P_POPUP_RADIO_ON), NULL);
	radio_img[3] = nfui_get_image_from_file((IMG_D_POPUP_RADIO_OFF), NULL);

	// <---- WINDOW
	holiday_up_win = (NFOBJECT*)nfui_nfwindow_new(parent, HOLIDAY_WIN_SIZE_X, HOLIDAY_WIN_SIZE_Y, HOLIDAY_WIN_SIZE_W, HOLIDAY_WIN_SIZE_H);
	g_curwnd = holiday_up_win;
	nfui_nfobject_modify_bg(holiday_up_win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_regi_post_event_callback(holiday_up_win, post_main_win_event_handler);

	nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

	// <---- FIXED
	main_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(main_fixed, HOLIDAY_WIN_SIZE_W, HOLIDAY_WIN_SIZE_H);
	nfui_regi_pre_event_callback(main_fixed, pre_main_fixed_event_handler);
	nfui_nfobject_show(main_fixed);

	// <---- TITLE
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ADD NEW HOLIDAY", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, 500, HOLIDAY_TITLE_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 76, 4);

	strcpy(dayOfWeekStr, yoil_str[dayOfWeek]);
	strcpy(weekStr, ord_str[week-1]);

	int pos_x = 60;
	int pos_y = 60;

	for (i = 0; i < 5; i++) 
	{
		obj = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
		nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
		nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
		nfui_nfobject_show(obj);

		if (i == 0) 
		{
			slist = nfui_radio_button_get_group(NF_BUTTON(obj));
		} 
		else 
		{
			nfui_radio_button_add_group(NF_BUTTON(obj), slist);
		}

		if (i == 0)
			nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);

		nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
		holi_type_obj[i] = obj;

		switch (i) 
		{
			case 0: sprintf(strRadio,"%s", hol_pop_str[i]); 			     break;
			case 1: sprintf(strRadio,"%s %s", lookup_string(hol_pop_str[i]), lookup_string(dayOfWeekStr));	     break;
			case 2: sprintf(strRadio,"%s %s %d%s", lookup_string(hol_pop_str[i]), lookup_string(mon_str[mon-1]), day, lookup_string("Day2"));   break;
			case 3: sprintf(strRadio,"%s %s %s", lookup_string(hol_pop_str[i]), lookup_string(weekStr), lookup_string(dayOfWeekStr)); break;
			case 4: sprintf(strRadio,"%s %d%s", lookup_string(hol_pop_str[i]), day, lookup_string("Day2"));			     break;
		}

		//RADIO LABEL
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strRadio, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
		nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH, 27);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x + size_w + 10, pos_y);

		pos_y += (guint)size_h + 10;
	}

	// <---- OK BUTTON
	holiday_set_obj = nftool_normal_button_create_type1("OK", HOLIDAY_OK_BTN_W);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(holiday_set_obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(holiday_set_obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, holiday_set_obj, HOLIDAY_OK_BTN_X, HOLIDAY_OK_BTN_Y);
	nfui_regi_post_event_callback(holiday_set_obj, post_okbutton_event_handler);

	// <---- CANCEL BUTTON
	obj = nftool_normal_button_create_type1("CANCEL", HOLIDAY_CLOSE_BTN_W);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, HOLIDAY_CLOSE_BTN_X, HOLIDAY_CLOSE_BTN_Y);
	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);

	nfui_nfwindow_add((NFWINDOW*)holiday_up_win, main_fixed);
	nfui_run_main_event_handler(holiday_up_win);
	nfui_nfobject_show(holiday_up_win);

	gtk_main();

	g_message("--->> Func : %s, g_ret: %d", __func__, g_ret);
	return g_ret;	
}

