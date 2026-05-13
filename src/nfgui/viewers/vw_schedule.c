#include "nf_afx.h"
#include "services/scm.h"
#include "tools/nf_ui_tool.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "support/multi_language_support.h"
#include "modules/ssm.h"
#include "uxm.h"
#include "iux_msg.h"
#include "iux_afx.h"

#include "viewers/vw_internal.h"
#include "vw_sys_main.h"
#include "vw_holiday_set.h"
#include "vw_schedule.h"

#include "scm.h"
#include "stm.h"
#include "dtf.h"
#include "smt.h"
#include "ix_func.h"


// CALENDAR FIXED
#define SBT_CAL_FIXED_X                 (8)
#define SBT_CAL_FIXED_Y                 (50)
#define SBT_CAL_FIXED_W                 (916)
#define SBT_CAL_FIXED_H                 (614)

// Cal DATE PREV/NEXT
#define CAL_PREV_BUTTON_X         	(290)
#define CAL_NEXT_BUTTON_X          	(549) 
#define CAL_PREV_NEXT_BUTTON_Y          (696)

// Cal DATE PREV/NEXT LABEL
#define CAL_DATE_LABEL_X         	(320)
#define CAL_DATE_LABEL_Y          	(0) 

// Cal holiday List Max 
#define MAX_ROW				(13)

#define ART2_LIST_LABEL_W		470		//(ART2_NEXT_BTN_X + ART2_PAGE_BTN_SIZE + 406)
#define ART2_LIST_LABEL_H		38		//(ART2_NEXT_BTN_Y)


////////////////////////////////////////////////////////////
//
// private variables
//

static NFWINDOW *g_curwnd = 0;

// Current date
static NFOBJECT *g_cld_text = NULL;

static NFOBJECT *listlbl_obj[MAX_ROW];	// LIST
static NFOBJECT *pagelbl_obj;				// PAGE NUMBER
static NFOBJECT *delbtns_obj[MAX_ROW];		// DELETE ITEM BUTTONS
static CWCALENDAR *cld;

static time_t from_utime, to_utime;

static int hcnt = 0;
static int org_hcnt = 0;
static guint dformat, tformat;

static UsrDefHolidayData holi_data[50];
static UsrDefHolidayData holi_org_data[50];

static int g_cur_page = 1;	// 1 ~ n



////////////////////////////////////////////////////////////
//
// private functions
//
static void _get_date_fomat(int idx, char* buf_date)
{
	int y, m ,d;

	y = holi_data[idx].year;
	m = holi_data[idx].month;
	d = holi_data[idx].day;
	g_message("dformat: %d", dformat);
	switch(dformat){
		case YYYYMMDD:
			sprintf(buf_date, "%04d-%02d-%02d", y, m, d); break;
		case YYMMDD:
			sprintf(buf_date, "%02d-%02d-%02d", m, d, y); break;
		case MMDDYY:
			sprintf(buf_date, "%02d-%02d-%02d", d, m, y); break;
		case MMDDYYYY:
			sprintf(buf_date, "%02d-%02d-%04d", m, y, d); break;
		case DDMMYYYY:
			sprintf(buf_date, "%02d-%02d-%04d", d, m, y); break;
	}

	return buf_date;	
}

static int _get_calendar_day(int *year, int *mon, int *day)
{
    if (year) *year = cw_hcld_get_current_year((CWCALENDAR*)cld);
    if (mon) *mon = cw_hcld_get_current_month((CWCALENDAR*)cld);
    if (day) *day  = cw_hcld_get_current_day((CWCALENDAR*)cld);
    return 0;
}

static int _update_cur_calendar_text(int mon, int year)
{
    char buf[15];
    
    g_sprintf(buf, "%s %d", g_month_str[mon-1], year);

    nfui_nflabel_set_text((NFLABEL*)g_cld_text, buf);
    nfui_signal_emit(g_cld_text, GDK_EXPOSE, FALSE);
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
    
    nfui_nflabel_set_text((NFLABEL*)g_cld_text, buf);
    nfui_signal_emit(g_cld_text, GDK_EXPOSE, FALSE);
    return 0;
}

static int _update_calendar_time(time_t timet)
{
    int year, mon, day;
    if (timet == 0) timet = time(0);
    dtf_get_local_day(timet, &year, &mon, &day);
    cw_hcld_change_date((CWCALENDAR*)cld, year, mon, day);
    return 0;
}

static void _holiday_preserve_data_load_page(gint current_page, gint expose)
{
	gchar buf[100];
	gint i, j, k;
	gint type, week, yoil;
	guint status;
	time_t t_start, t_end;
	GTimeVal tvtemp;
	int size_cnt = 0;
	float total_size = 0;
	char typeStr[32];
	char yoilStr[32];
	char weekStr[32];
	char hol_dateStr[15];
	int start_row;
	int sel = -1;
	
	gchar yoil_str[7][10] = {
		"SUNDAY",
		"MONDAY",
		"TUESDAY",
		"WEDNESDAY",
		"THURSDAY",
		"FRIDAY",
		"SATURDAY"
	};

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

	g_message("__LINE : %d ", __LINE__);

	memset(&tvtemp, 0x00, sizeof(GTimeVal));

	start_row = (g_cur_page - 1) * MAX_ROW;

	g_message("__LINE : %d start_row: %d , g_cur_page : %d", __LINE__, start_row, g_cur_page);

	for(i = start_row, j=0 ; i < (start_row + MAX_ROW) ; i++, j++)
	{

		if(i >= hcnt) {
			memset(buf, 0x00, sizeof(buf));
			
			nfui_nfobject_disable(delbtns_obj[j]);
		}
		else {
			memset(buf, 0x00, sizeof(buf));
			memset(typeStr, 0x00, sizeof(typeStr));
			memset(yoilStr, 0x00, sizeof(yoilStr));
			memset(weekStr, 0x00, sizeof(weekStr));
			
			type = holi_data[i].type;
			week = holi_data[i].week;
			yoil = holi_data[i].yoil;
			
			strcpy(weekStr, ord_str[week-1]);
			strcpy(yoilStr, yoil_str[yoil]);
			_get_date_fomat(i, hol_dateStr);

			if(type == 'A') {
				strcpy(typeStr, hol_pop_str[0]);
				g_sprintf(buf, "%s (%s)", lookup_string(typeStr), hol_dateStr);
			}
			else if(type == 'B'){
				strcpy(typeStr, hol_pop_str[1]);
			g_sprintf(buf, "%s %s(%s)", lookup_string(typeStr), lookup_string(yoilStr), hol_dateStr);

			}
			else if(type == 'C'){
				strcpy(typeStr, hol_pop_str[2]);
				g_sprintf(buf, "%s %s %d%s(%s)", lookup_string(typeStr), lookup_string(mon_str[holi_data[i].month-1]), holi_data[i].day, lookup_string("Day2"), hol_dateStr);

			}
			else if(type == 'D'){
				strcpy(typeStr, hol_pop_str[3]);
				g_sprintf(buf, "%s %s %s(%s)", lookup_string(typeStr), lookup_string(weekStr), lookup_string(yoilStr), hol_dateStr);

			}
			else if(type == 'E' ){
				strcpy(typeStr, hol_pop_str[4]);
				g_sprintf(buf, "%s %d%s(%s)", lookup_string(typeStr), holi_data[i].day, lookup_string("Day2"), hol_dateStr);
			}
			nfui_nfobject_enable(delbtns_obj[j]);
			/*
			printf("---->>> set_holi_data %d, %d, %d, %d, %d, %c idx :%d\n", 
					holi_data[i].year,
					holi_data[i].month,
					holi_data[i].day,
					holi_data[i].yoil,
					holi_data[i].week,
					holi_data[i].type, i);
			*/

		}

		nfui_nflabel_set_text((NFLABEL*)listlbl_obj[j], buf);

		if (expose) nfui_signal_emit(listlbl_obj[j], GDK_EXPOSE, TRUE);
		if (expose) nfui_signal_emit(delbtns_obj[j], GDK_EXPOSE, TRUE);
	}
}

static gint _get_total_page()
{
	gint total_page;

	if (hcnt == 0) return 1;

	total_page = (gint)ceil((gdouble)hcnt / MAX_ROW);
	return total_page;
}

static gint _update_page_label(gint current_page, gint expose)
{
	gchar page_buf[40];

	sprintf(page_buf, "%d/%d", current_page, _get_total_page());
	nfui_nflabel_set_text((NFLABEL*)pagelbl_obj, page_buf);
	if (expose) nfui_signal_emit(pagelbl_obj, GDK_EXPOSE, TRUE);
	return 0;
}

static int _chkSame_holiday_type(UsrDefHolidayData *holi_data, int year, int mon, int day, char type)
{
	int i, ret = 0;
	
	for(i = 0; i < hcnt; i++){
	    if( (holi_data[i].year == year) &&
		(holi_data[i].month  == mon) &&
		(holi_data[i].day  == day) &&
		(holi_data[i].type == type) ){
			ret = 1;
		}
	}
	return ret;
}

static time_t _get_new_calendar_time()
{
    int year, month, day, hour, min, sec;
    time_t tim;

    _get_calendar_day(&year, &month, &day);
    dtf_get_local_hourmin(from_utime, &hour, &min, &sec);
    tim = ifn_get_gmt_from_local(year, month, day, hour, min, sec);
    return tim;
}

static int _del_holiday_calendar(NFOBJECT *cal, int idx)
{
	int i, start_row;
	HOLIDAYCAL_INFO holi_info[50];

	// break the dependency to an NFDAL
	start_row = (g_cur_page-1) * MAX_ROW;	

	for(i = (idx + start_row); i < (hcnt - 1); i++){ 

		holi_data[i].year  = holi_data[i+1].year;
		holi_data[i].month = holi_data[i+1].month;
		holi_data[i].day   = holi_data[i+1].day;
		holi_data[i].yoil  = holi_data[i+1].yoil;
		holi_data[i].week  = holi_data[i+1].week;
		holi_data[i].type  = holi_data[i+1].type;
	}

	if(hcnt > 0)
	{
		hcnt--;
		if (hcnt == 0) {
			memset(holi_data, 0x00, sizeof(holi_data));
		}
	}

	memset(holi_info, 0x00, sizeof(holi_info));
	for (i = 0; i < hcnt; ++i) {
		holi_info[i].year  = holi_data[i].year;
		holi_info[i].month = holi_data[i].month;
		holi_info[i].day   = holi_data[i].day;
		holi_info[i].yoil  = holi_data[i].yoil;
		holi_info[i].week  = holi_data[i].week;
		holi_info[i].type  = holi_data[i].type;
	}

	cw_hcld_refresh_holiday(cal, holi_info, hcnt);	
	return 0;
}


static int _set_holiday_calendar(NFOBJECT *cal)
{
	int i;
	HOLIDAYCAL_INFO holi_info[50];

	for (i = 0; i < hcnt; ++i) {
		holi_info[i].year  = holi_data[i].year;
		holi_info[i].month = holi_data[i].month;
		holi_info[i].day   = holi_data[i].day;
		holi_info[i].yoil  = holi_data[i].yoil;
		holi_info[i].week  = holi_data[i].week;
		holi_info[i].type  = holi_data[i].type;
	}

	cw_hcld_refresh_holiday(cal, holi_info, hcnt);
	return 0;
}

static gboolean post_cld_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	time_t gap = to_utime - from_utime;

	int  year = 0;
	int  mon  = 0;
	int  day  = 0;
	int  week = 0;
	int  dayOfweek;
	int type;
	gchar type_char;
	int prev_totpage;	

	//UsrDefHolidayData tmp[50];

	switch(evt->type) {
		case GDK_BUTTON_PRESS :
			break;

		case NFEVENT_CALENDAR_CHANGED_RELEASE:

			year = cw_hcld_get_current_year(obj);
			mon = cw_hcld_get_current_month(obj);
			day = cw_hcld_get_current_day(obj);
			dayOfweek = ifn_get_yoil(year, mon, day);
			week = ifn_get_week_of_month(day);
			
			_update_cur_calendar_text(mon, year);

			//g_message("--->>> dayOfweek : %d", dayOfweek);
			//memset(tmp, 0x0, sizeof(UsrDefHolidayData)*USR_DEF_HOLIDAY_CNT);
			//memmove(tmp, holi_data, sizeof(UsrDefHolidayData)*USR_DEF_HOLIDAY_CNT);

			if(hcnt == USR_DEF_HOLIDAY_CNT)
			{
				nftool_mbox_auto(g_curwnd, 1, "WARNING", "Up to 50 dates can be specified as holidays.");
			}
			else
			{
				type = VW_Schedule_HolidaySet_Open(g_curwnd, dayOfweek, week, mon, day);
				
				
				if (type == -1) return FALSE;

				type_char = 'A' + type;

				
				if(!_chkSame_holiday_type(&holi_data, year, mon, day, type_char)) 
				{
					prev_totpage = _get_total_page();

					holi_data[hcnt].year  = year;
					holi_data[hcnt].month = mon;
					holi_data[hcnt].day   = day;
					holi_data[hcnt].yoil  = dayOfweek;
					holi_data[hcnt].week  = week;
					holi_data[hcnt].type  = type_char;

						hcnt++;

						_set_holiday_calendar(obj);

					if ((g_cur_page == prev_totpage) && (prev_totpage+1 == _get_total_page())) {
						g_cur_page++;
					}

					_holiday_preserve_data_load_page(g_cur_page, 1);
					_update_page_label(g_cur_page, 1);
				}
				else{
					nftool_mbox_auto(g_curwnd, 1, "ERROR", "The same holiday data exists.");
				}
			}
			break;

		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
			break;

		default:
			break;
	}

	return FALSE;
}

static gboolean vw_art1_cld_prevbtn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    time_t ret;

    if(evt->type == GDK_BUTTON_RELEASE) {
        ret = cw_hcld_set_prev_month((CWCALENDAR*)cld);
        if (ret == 0) return FALSE;
        _update_calendar_text(ret);
    }

    return FALSE;
}

static gboolean vw_art1_cld_nextbtn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    time_t ret;

    if(evt->type == GDK_BUTTON_RELEASE) {
        ret = cw_hcld_set_next_month((CWCALENDAR*)cld);
        if (ret == 0) return FALSE;
        _update_calendar_text(ret);
    }

    return FALSE;
}

static gboolean holidayList_prev_btn_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if (g_cur_page == 1) return FALSE;

		g_cur_page--;
		_holiday_preserve_data_load_page(g_cur_page, 1);
		_update_page_label(g_cur_page, 1);
	}

	return FALSE;
}

static gboolean holidayList_next_btn_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if (g_cur_page >= _get_total_page()) return FALSE;

		g_cur_page++;
		_holiday_preserve_data_load_page(g_cur_page, 1);
		_update_page_label(g_cur_page, 1);
	}

	return FALSE;
}

static gboolean holidayList_delbtns_btn_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint i, j, sel_del_item = -1;
	gint return_delitem_result;
	int pageVal;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		for(i = 0; i < MAX_ROW; i++)
		{
			if(delbtns_obj[i] == obj)
			{
				sel_del_item = i;
				break;
			}
		}
		if(sel_del_item == -1) return FALSE;

		return_delitem_result =  _del_holiday_calendar(cld, sel_del_item);
		if (return_delitem_result != 0) return FALSE;

		if (g_cur_page > _get_total_page()) {
			g_cur_page = _get_total_page();
		} 

		_holiday_preserve_data_load_page(g_cur_page, 1);
		_update_page_label(g_cur_page, 1);
	}
	return FALSE;
}

static gboolean post_cancel_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    int i;
    
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		hcnt = org_hcnt;
		memmove(holi_data, holi_org_data, sizeof(UsrDefHolidayData)*USR_DEF_HOLIDAY_CNT);
		
		cw_hcld_set_chmask(cld, var_get_ch_mask());
		_set_holiday_calendar(cld);
		
		for(i = 0; i < MAX_ROW; i++){
			if(i < hcnt){
				nfui_nfobject_enable(delbtns_obj[i]);
			}else{
				nfui_nflabel_set_text((NFLABEL*)listlbl_obj[i], "");
				nfui_nfobject_disable(delbtns_obj[i]);
			}

			nfui_signal_emit(listlbl_obj[i], GDK_EXPOSE, TRUE);
			nfui_signal_emit(delbtns_obj[i], GDK_EXPOSE, TRUE);
		
		}

		if (g_cur_page > _get_total_page()) {
			g_cur_page = _get_total_page();
		} 

		_holiday_preserve_data_load_page(g_cur_page, 1);
		_update_page_label(g_cur_page, 1);
    }

    return FALSE;
}

static gboolean post_apply_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		gint i;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
		if(hcnt == org_hcnt) return FALSE;

		DAL_set_holiday_count(hcnt);

		for(i = 0; i < USR_DEF_HOLIDAY_CNT; i++){
			if(i < hcnt){
				DAL_set_UsrDefHoliday_Data(&holi_data[i], i);
			}else{
				DAL_default_UsrDefHoliday_Data(i);
			}
		}
		nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_SYS, 0, 0, 0);

		org_hcnt = hcnt;
		memmove(holi_org_data, holi_data, sizeof(UsrDefHolidayData)*USR_DEF_HOLIDAY_CNT);

		nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
		VW_SetupSystem_set_changeflag(1);
	}

    return FALSE;
}

static gboolean post_close_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {   
	    if(evt->type == GDK_BUTTON_RELEASE)
	    {
		    if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

			vw_SysSchedule_out_handler();
			VW_SetupSystem_Destroy(obj);
	    }

    }
    return FALSE;
}

static gboolean post_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case GDK_EXPOSE:
		break;

		case GDK_DELETE: 
		break;

		default : 
		break;
	}

	return FALSE;

}

static gboolean pre_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case GDK_EXPOSE:
		break;

		case GDK_DELETE: 
		break;

		default : 
		break;
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

///////////////////////////////////////////////////////////////////
//
//
//

void vw_Init_SysSchedule_page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *list_fixed;
	NFOBJECT *cal_fixed;

	NFOBJECT *obj;

	GdkPixbuf *arrow_img[2][NFOBJECT_STATE_COUNT];
	GdkPixbuf *prev_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *next_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *delitem_btn_img[NFOBJECT_STATE_COUNT];

	const gchar *strDay[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
	const gchar *listButton[] = {"PREV", "NEXT"};
	const gchar *strButton[] = {"CANCEL", "APPLY", "CLOSE"};

	gint xpos = 966;
	gint ypos = 92;

	char buf[64];
	char buf2[64];

	int year, mon;
	time_t timet;
	int length;
	int c_utime, c_year, c_month, c_day, c_hour, c_min, c_sec;
	int i;
	
	guint size_w, size_h;
	guint pos_x, pos_y;


	memset(holi_data, 0x0, sizeof(UsrDefHolidayData)*USR_DEF_HOLIDAY_CNT);
	memset(holi_org_data, 0x0, sizeof(UsrDefHolidayData)*USR_DEF_HOLIDAY_CNT);

	hcnt = DAL_get_holiday_count();
	DAL_get_dateTime_format(&dformat, &tformat);


	for(i = 0; i < hcnt; i++){
		DAL_get_UsrDefHoliday_Data(&holi_data[i], i);
	}

	org_hcnt = hcnt;
	g_memmove(holi_org_data, holi_data, sizeof(UsrDefHolidayData)*USR_DEF_HOLIDAY_CNT);

	g_cur_page = 1;


	arrow_img[0][0] = nfui_get_image_from_file((IMG_CALENDAR_PRE_N_BUTTON), NULL);
	arrow_img[0][1] = nfui_get_image_from_file((IMG_CALENDAR_PRE_O_BUTTON), NULL);
	arrow_img[0][2] = nfui_get_image_from_file((IMG_CALENDAR_PRE_P_BUTTON), NULL);
	arrow_img[0][3] = nfui_get_image_from_file((IMG_CALENDAR_PRE_D_BUTTON), NULL);

	arrow_img[1][0] = nfui_get_image_from_file((IMG_CALENDAR_NEXT_N_BUTTON), NULL);
	arrow_img[1][1] = nfui_get_image_from_file((IMG_CALENDAR_NEXT_O_BUTTON), NULL);
	arrow_img[1][2] = nfui_get_image_from_file((IMG_CALENDAR_NEXT_P_BUTTON), NULL);
	arrow_img[1][3] = nfui_get_image_from_file((IMG_CALENDAR_NEXT_D_BUTTON), NULL);

	delitem_btn_img[0] = nfui_get_image_from_file((IMG_BTN_N_DELETE), NULL);
	delitem_btn_img[1] = nfui_get_image_from_file((IMG_BTN_O_DELETE), NULL);
	delitem_btn_img[2] = nfui_get_image_from_file((IMG_BTN_P_DELETE), NULL);
	delitem_btn_img[3] = nfui_get_image_from_file((IMG_BTN_D_DELETE), NULL);

	prev_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_N_BTN), NULL);
	prev_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_O_BTN), NULL);
	prev_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_P_BTN), NULL);
	prev_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_D_BTN), NULL);

	next_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_N_BTN), NULL);
	next_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_O_BTN), NULL);
	next_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_P_BTN), NULL);
	next_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_D_BTN), NULL);


	g_curwnd = nfui_nfobject_get_top(parent);

	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);
	nfui_regi_post_event_callback(content_fixed, post_fixed_event_handler);
	
	cal_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(cal_fixed, SBT_CAL_FIXED_W, SBT_CAL_FIXED_H);
	nfui_nfobject_show(cal_fixed);
	nfui_nfobject_modify_bg(cal_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(606));
	nfui_nffixed_put((NFFIXED*)content_fixed, cal_fixed, SBT_CAL_FIXED_X, SBT_CAL_FIXED_Y);
	
	// CALENDAR LABEL
	gint cal_lbl_pos_x[7] = {38, 160, 280, 400, 520, 640, 761};

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
		nfui_nfobject_set_size(obj, 122, 40);
		nfui_nfobject_show(obj);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(605));
		nfui_nffixed_put((NFFIXED*)cal_fixed, obj, cal_lbl_pos_x[i], 50);
	}

	// YEAR / MONTH LABEL
	c_utime     = time(0);
	dtf_get_local_day(c_utime, &c_year, &c_month, &c_day);
	dtf_get_local_hourmin(c_utime, &c_hour, &c_min, 0);

	g_message("---->>> c_year : %d c_month : %d line : %d", c_year, c_month, __LINE__);
	strcpy(buf, &g_month_str[c_month - 1]);
	sprintf(buf2, "%s %d", buf, c_year);
	g_cld_text = (NFOBJECT*)nfui_nflabel_new_with_pango_font(buf2, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(607));
	nfui_nflabel_set_align((NFLABEL*)g_cld_text, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(g_cld_text, (guint)(200) , (guint)(50));
	nfui_nfobject_modify_bg(g_cld_text, NFOBJECT_STATE_NORMAL, COLOR_IDX(606));
	nfui_nfobject_use_focus(g_cld_text, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(g_cld_text);
	nfui_nffixed_put((NFFIXED*)cal_fixed, g_cld_text, (guint)CAL_DATE_LABEL_X+30, (guint)CAL_DATE_LABEL_Y);

	// CALENDAR
	cld = cw_hcld_holiday_new(c_utime, 120, 80);

	cw_hcld_set_chmask(cld, var_get_ch_mask());
	nfui_nfobject_show(cld);
	nfui_nffixed_put((NFFIXED*)cal_fixed, cld, 40, 90);
	nfui_regi_post_event_callback(cld, post_cld_event_cb);

	_set_holiday_calendar(cld);

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), arrow_img[0]);
	nfui_nfobject_set_size(obj, 50, 50);
	nfui_nfobject_show(obj);
	nfui_nffixed_put ((NFFIXED*)cal_fixed, obj, CAL_PREV_BUTTON_X - 20, 7);
	nfui_regi_post_event_callback(obj, vw_art1_cld_prevbtn_event_handler);

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), arrow_img[1]);
	nfui_nfobject_set_size(obj, 50, 50);
	nfui_nfobject_show(obj);
	nfui_nffixed_put ((NFFIXED*)cal_fixed, obj, CAL_NEXT_BUTTON_X + 65, 7);
	nfui_regi_post_event_callback(obj, vw_art1_cld_nextbtn_event_handler);

	// HOLIDAY DATE LIST
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("HOLIDAY DATA LIST", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
	nfui_nflabel_set_drawing_outline((NFLABEL*)obj, FALSE);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, ART2_LIST_LABEL_W, ART2_LIST_LABEL_H);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, xpos, ypos - 42);

	//label_font = NFFONT_MEDIUM_SEMI;

	for(i = 0; i < MAX_ROW; i++ ){
		obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_SUBTAB_OUTPUT);
		nfui_nflabel_set_drawing_outline((NFLABEL*)obj, FALSE);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_set_size(obj, ART2_LIST_LABEL_W, ART2_LIST_LABEL_H);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, xpos, ypos);
		listlbl_obj[i] = obj;

		ypos += 41/*ART2_LIST_LABEL_H_INTERVAL*/;
	}

	// DELETE ITEM BUTTONS

	nfui_get_pixbuf_size(delitem_btn_img[0], &size_w, &size_h);

	ypos = 91;
	// DELETE
	for(i = 0; i < MAX_ROW; i++ )
	{
		obj = (NFOBJECT*)nfui_nfbutton_new();
		delbtns_obj[i] = obj;
		nfui_nfbutton_set_image(NF_BUTTON(obj), delitem_btn_img);
		nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, 950 + 495/*xpos+300*/ , ypos);
		nfui_nfobject_show(obj);
		nfui_regi_post_event_callback(obj, holidayList_delbtns_btn_handler);

		ypos += 41/*ART2_LIST_LABEL_H_INTERVAL*/;

	}

	if(hcnt == 0){
		for(i = 0 ; i < MAX_ROW; i++){
			nfui_nfobject_disable(delbtns_obj[i]);
		}
	}

	// Holiday List PREV
	nfui_get_pixbuf_size(prev_img[0], &size_w, &size_h);

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), prev_img);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, xpos+110/*ART2_PREV_BTN_X*/, 625/*ART2_PREV_BTN_Y*/);
	nfui_regi_post_event_callback(obj, holidayList_prev_btn_handler);

	// Holiday List LABEL
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(121));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, 110, 40);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, xpos+175, 625);
	pagelbl_obj = obj;

	// Holiday List NEXT
	nfui_get_pixbuf_size(next_img[0], &size_w, &size_h);

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), next_img);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, xpos+300/*ART2_NEXT_BTN_X*/, 625/*ART2_NEXT_BTN_Y*/);
	nfui_regi_post_event_callback(obj, holidayList_next_btn_handler);

	pos_x = SBT_CAL_FIXED_X;
	pos_y = SBT_CAL_FIXED_Y + SBT_CAL_FIXED_H + 8;

	pos_y += 240;

	stm_get_time_range_t(&from_utime, &to_utime);

	// below button
	obj = nftool_normal_button_create_type1(strButton[0], MENU_BTN_WIDTH);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R3_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_cancel_event_handler);

	obj = nftool_normal_button_create_type1(strButton[1], MENU_BTN_WIDTH);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R2_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_apply_event_handler);

	obj = nftool_normal_button_create_type2(strButton[2], MENU_BTN_WIDTH);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R1_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_close_event_handler);

	nfui_regi_post_event_callback(parent, post_page_event_handler);

	_holiday_preserve_data_load_page(1, 0);
	_update_page_label(1, 0);
}

gboolean vw_SysSchedule_in_handler()
{
    DAL_get_dateTime_format(&dformat, &tformat);
    _holiday_preserve_data_load_page(1, 0);
	
    return FALSE;
}

gboolean vw_SysSchedule_out_handler()
{
	mb_type ret = 0;
	gint i;

	if(hcnt == org_hcnt) return FALSE;
	
	ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

	if (ret == NFTOOL_MB_OK)
	{		
		DAL_set_holiday_count(hcnt);		

		for (i = 0; i < USR_DEF_HOLIDAY_CNT; i++)
		{
			if(i < hcnt){
				DAL_set_UsrDefHoliday_Data(&holi_data[i], i);
			}else{
				DAL_default_UsrDefHoliday_Data(i);
			}
		}		
		nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_SYS, 0, 0, 0);

		org_hcnt = hcnt;
		memmove(holi_org_data, holi_data, sizeof(UsrDefHolidayData)*USR_DEF_HOLIDAY_CNT);		
		VW_SetupSystem_set_changeflag(1);
	}
	else
	{
		hcnt = org_hcnt;
		memmove(holi_data, holi_org_data, sizeof(UsrDefHolidayData)*USR_DEF_HOLIDAY_CNT);
		
		cw_hcld_set_chmask(cld, var_get_ch_mask());
		_set_holiday_calendar(cld);
		
		for(i = 0; i < MAX_ROW; i++){
			if(i < hcnt){
				nfui_nfobject_enable(delbtns_obj[i]);
			}else{
				nfui_nflabel_set_text((NFLABEL*)listlbl_obj[i], "");
				nfui_nfobject_disable(delbtns_obj[i]);
			}
		}

		if (g_cur_page > _get_total_page()) {
			g_cur_page = _get_total_page();
		} 

		_holiday_preserve_data_load_page(g_cur_page, 0);
		_update_page_label(g_cur_page, 0);
	}

    return FALSE;
}
