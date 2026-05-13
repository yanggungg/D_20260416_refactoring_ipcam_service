
#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"

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
#include "../viewers/objects/cw_calendar.h"
#include "viewers/objects/nfcombobox.h"

#include "vw_arch_date_popup.h"
#include "ix_func.h"

#define ARCHDATE_POPUP_SIZE_W	485
#define ARCHDATE_POPUP_SIZE_H	289

NFOBJECT *dt_lbl[6];

static time_t c_utime;
static gint dt_year, dt_month, dt_day, dt_hour, dt_min, dt_sec;
static gint dt_lastday;
static char buffer[100];

static gint kind_date_value;

static void arch_popup_updown_btn_event_value(gint num);
static void arch_popup_set_date_update();

static gboolean 
arch_date_win_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	return FALSE;
}

static gboolean 
arch_up_btn0_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
		arch_popup_updown_btn_event_value(ARCH_DT_POP_UP0); 
	
	return FALSE;
}

static gboolean 
arch_up_btn1_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
		arch_popup_updown_btn_event_value(ARCH_DT_POP_UP1); 
	return FALSE;
}

static gboolean 
arch_up_btn2_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
		arch_popup_updown_btn_event_value(ARCH_DT_POP_UP2); 
	
	return FALSE;
}

static gboolean 
arch_up_btn3_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
		arch_popup_updown_btn_event_value(ARCH_DT_POP_UP3); 
	return FALSE;
}

static gboolean 
arch_up_btn4_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
		arch_popup_updown_btn_event_value(ARCH_DT_POP_UP4); 
	return FALSE;
}

static gboolean 
arch_up_btn5_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
		arch_popup_updown_btn_event_value(ARCH_DT_POP_UP5); 
	
	return FALSE;
}

static gboolean 
arch_down_btn0_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
		arch_popup_updown_btn_event_value(ARCH_DT_POP_DOWN0); 
	
	return FALSE;
}

static gboolean 
arch_down_btn1_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
		arch_popup_updown_btn_event_value(ARCH_DT_POP_DOWN1); 
	
	return FALSE;
}

static gboolean 
arch_down_btn2_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
		arch_popup_updown_btn_event_value(ARCH_DT_POP_DOWN2); 
	
	return FALSE;
}

static gboolean 
arch_down_btn3_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
		arch_popup_updown_btn_event_value(ARCH_DT_POP_DOWN3); 
	
	return FALSE;
}

static gboolean 
arch_down_btn4_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
		arch_popup_updown_btn_event_value(ARCH_DT_POP_DOWN4); 
	
	return FALSE;
}

static gboolean 
arch_down_btn5_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
		arch_popup_updown_btn_event_value(ARCH_DT_POP_DOWN5); 
	
	return FALSE;
}

static gboolean
arch_dt_popup_okbtn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;
	
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
		gtk_main_quit();
	}
		
	return FALSE;
}

static gboolean
arch_dt_popup_cancelbtn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static void 
arch_popup_set_date_update()
{
	gint i;

	c_utime = ifn_get_gmt_from_local(dt_year, dt_month, dt_day, dt_hour, dt_min, dt_sec);
	dtf_get_local_day(c_utime, &dt_year, &dt_month, &dt_day);
	dtf_get_local_hourmin(c_utime, &dt_hour, &dt_min, &dt_sec);

	sprintf(buffer, "%04d", dt_year); nfui_nflabel_set_text(dt_lbl[0], buffer);
	sprintf(buffer, "%02d", dt_month); nfui_nflabel_set_text(dt_lbl[1], buffer);
	sprintf(buffer, "%02d", dt_day); nfui_nflabel_set_text(dt_lbl[2], buffer);
	sprintf(buffer, "%02d", dt_hour); nfui_nflabel_set_text(dt_lbl[3], buffer);
	sprintf(buffer, "%02d", dt_min); nfui_nflabel_set_text(dt_lbl[4], buffer);
	sprintf(buffer, "%02d", dt_sec); nfui_nflabel_set_text(dt_lbl[5], buffer);
	
	for( i = 0 ; i < 6 ; i++ ) nfui_signal_emit(dt_lbl[i], GDK_EXPOSE, TRUE);
}

static void 
arch_popup_set_lastday_update()
{
	gint i;
	time_t utime;
	guint day;
	
	utime = ifn_get_gmt_from_local(dt_year, dt_month, 1, 0, 0, 0);
	
	for( i = 1 ; i < 32 ; i++ ) {
		utime += 86400;				// next day
		ifn_get_gmt_day(utime, 0, 0, &day);

		if(day == 1) break;
		else dt_lastday = day;
	}
}

static void 
arch_popup_updown_btn_event_value(gint num)
{
	switch(num) {
	case ARCH_DT_POP_UP0 :
		if(dt_year < 2030) dt_year++;
		else dt_year = 1970;
		break;

	case ARCH_DT_POP_DOWN0 :
		if(dt_year > 1971) dt_year--;
		else dt_year = 2030;
		break;	

	case ARCH_DT_POP_UP1 :
		if(dt_month < 12) dt_month++;
		else dt_month = 1;
		break;

	case ARCH_DT_POP_DOWN1 :
		if(dt_month > 1) dt_month--;
		else dt_month = 12;
		break;
		
	case ARCH_DT_POP_UP2 :
		arch_popup_set_lastday_update();
		if(dt_day < dt_lastday) dt_day++;
		else dt_day = 1;
		break;

	case ARCH_DT_POP_DOWN2 :
		arch_popup_set_lastday_update();
		if(dt_day > 1) dt_day--;
		else dt_day = dt_lastday;
		break;
		
	case ARCH_DT_POP_UP3 :
		if (dt_hour < 23) dt_hour++;
		else dt_hour = 0;
		break;

	case ARCH_DT_POP_DOWN3 :
		if (dt_hour > 0) dt_hour--;
		else dt_hour = 23;
		break;
		
	case ARCH_DT_POP_UP4 :
		if (dt_min < 59) dt_min++;
		else dt_min = 0;
		break;

	case ARCH_DT_POP_DOWN4 :
		if (dt_min > 0) dt_min--;
		else dt_min = 59;
		break;
		
	case ARCH_DT_POP_UP5 :
		if (dt_sec < 59) dt_sec++;
		else dt_sec = 0;
		break;

	case ARCH_DT_POP_DOWN5 :
		if (dt_sec > 0) dt_sec--;
		else dt_sec = 59;
		break;
	}
	arch_popup_set_date_update();
}



static gboolean 
arch_date_fixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *top;
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if(event->type == GDK_EXPOSE)
	{
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);

		nfui_nfobject_gc_unref(gc);
	} 
	else if(event->type == GDK_DELETE) 
	{
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);

		top = nfui_nfobject_get_top(obj);
		nfui_page_close(PGID_USERPWD, top);
	}
	
	return FALSE;
}

time_t open_vw_date_popup(NFWINDOW *parent, char *title, guint xpos, guint ypos, time_t p_utime)
//open_vw_arch_date_popup(char *title, guint xpos, guint ypos, NFOBJECT *p_date_obj, guint p_utime, gint kind)
{
	NFOBJECT *win 	= NULL;
	NFOBJECT *fixed = NULL;
	NFOBJECT *obj 	= NULL;
	
	GdkPixbuf *updown_btn_img[2][NFOBJECT_STATE_COUNT];

	gint size_w, size_h;
	gint i;
	
// WINDOW
	win = (NFOBJECT*)nfui_nfwindow_new(parent, 
										xpos, ypos, 
										ARCHDATE_POPUP_SIZE_W, ARCHDATE_POPUP_SIZE_H
										);

	nfui_regi_post_event_callback(win, arch_date_win_event_handler);

	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, ARCHDATE_POPUP_SIZE_W, ARCHDATE_POPUP_SIZE_H);
	nfui_regi_post_event_callback(fixed, arch_date_fixed_event_handler);
	nfui_nfobject_show(fixed);

// TITLE LABEL
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(title, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), CR_FG_PWND_TITLE);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	//nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, NF_COLOR_CODE_5A708F);
	// NF_COLOR_CODE_405571
	nfui_nfobject_set_size(obj, 70, 30);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, ARCHDATE_POPUP_SIZE_W / 2 - 35, 10);

// DATE TIME LABEL / UP / DOWN
	updown_btn_img[0][0] = nfui_get_image_from_file(("bt_datetime_up_n.png"), NULL);
	updown_btn_img[0][1] = nfui_get_image_from_file(("bt_datetime_up_o.png"), NULL);
	updown_btn_img[0][2] = nfui_get_image_from_file(("bt_datetime_up_p.png"), NULL);
	updown_btn_img[0][3] = nfui_get_image_from_file(("bt_datetime_up_d.png"), NULL);

	updown_btn_img[1][0] = nfui_get_image_from_file(("bt_datetime_down_n.png"), NULL);
	updown_btn_img[1][1] = nfui_get_image_from_file(("bt_datetime_down_o.png"), NULL);
	updown_btn_img[1][2] = nfui_get_image_from_file(("bt_datetime_down_p.png"), NULL);
	updown_btn_img[1][3] = nfui_get_image_from_file(("bt_datetime_down_d.png"), NULL);
	
	gint w_xpos[6] = {33, 101, 169, 251, 319, 387};
	char w_label[6][10] = {"YEAR","MON","DAY","HOUR","MIN","SEC"};

	gint size_w1, size_h1, size_w2, size_h2;
	nfui_get_pixbuf_size(updown_btn_img[0][0], &size_w1, &size_h1);
	nfui_get_pixbuf_size(updown_btn_img[1][0], &size_w2, &size_h2);

	c_utime 	= p_utime;
	dtf_get_local_day(c_utime, &dt_year, &dt_month, &dt_day);
	dtf_get_local_hourmin(c_utime, &dt_hour, &dt_min, &dt_sec);

// dbg_printf("%d-%d-%d %d:%d:%d \n", 
//									dt_year, dt_month, dt_day, 
//									dt_hour, dt_min, dt_sec);
	for( i = 0 ; i < 6 ; i++ )
	{
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(w_label[i], 
									nffont_get_pango_font(NFFONT_MEDIUM_SEMI), 
									CR_FG_NR_CELL_TYP1);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_set_size(obj, 63, 22);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed, obj, w_xpos[i], 82);

		// UP BUTTONS
		obj = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), updown_btn_img[0]);
		nfui_nfobject_set_size(obj, (guint)size_w1, (guint)size_h1);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed, obj, w_xpos[i], 106);

		if( i == 0 ) nfui_regi_post_event_callback(obj, arch_up_btn0_event_handler);
		if( i == 1 ) nfui_regi_post_event_callback(obj, arch_up_btn1_event_handler);
		if( i == 2 ) nfui_regi_post_event_callback(obj, arch_up_btn2_event_handler);
		if( i == 3 ) nfui_regi_post_event_callback(obj, arch_up_btn3_event_handler);
		if( i == 4 ) nfui_regi_post_event_callback(obj, arch_up_btn4_event_handler);
		if( i == 5 ) nfui_regi_post_event_callback(obj, arch_up_btn5_event_handler);

		// TEXT LABEL
		if( i == 0 ) sprintf(buffer, "%04d", dt_year);
		if( i == 1 ) sprintf(buffer, "%02d", dt_month);
		if( i == 2 ) sprintf(buffer, "%02d", dt_day);
		if( i == 3 ) sprintf(buffer, "%02d", dt_hour);
		if( i == 4 ) sprintf(buffer, "%02d", dt_min);
		if( i == 5 ) sprintf(buffer, "%02d", dt_sec);

		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(buffer, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), NF_COLOR_CODE_106296);
		dt_lbl[i] = obj;
		nfui_nflabel_set_align((NFLABEL*)obj , NFALIGN_CENTER, 0);
		nfui_nfobject_modify_bg(obj , NFOBJECT_STATE_NORMAL, CR_FG_NR_CELL_TYP1);
		nfui_nfobject_set_size(obj , 63, 40);
		nfui_nfobject_use_focus(obj , NFOBJECT_FOCUS_OFF);
		
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed, obj , w_xpos[i], 142);

		// DOWN BUTTONS
		obj = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), updown_btn_img[1]);
		nfui_nfobject_set_size(obj, (guint)size_w2, (guint)size_h2);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed, obj, w_xpos[i], 186);
		
		if( i == 0 ) nfui_regi_post_event_callback(obj, arch_down_btn0_event_handler);
		if( i == 1 ) nfui_regi_post_event_callback(obj, arch_down_btn1_event_handler);
		if( i == 2 ) nfui_regi_post_event_callback(obj, arch_down_btn2_event_handler);
		if( i == 3 ) nfui_regi_post_event_callback(obj, arch_down_btn3_event_handler);
		if( i == 4 ) nfui_regi_post_event_callback(obj, arch_down_btn4_event_handler);
		if( i == 5 ) nfui_regi_post_event_callback(obj, arch_down_btn5_event_handler);

	}

	obj = nftool_normal_button_create_type1("OK", 192);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 44, 235);
	nfui_regi_post_event_callback(obj, arch_dt_popup_okbtn_event_handler);

	obj = nftool_normal_button_create_type1("CANCEL", 192);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 242, 235);
	nfui_regi_post_event_callback(obj, arch_dt_popup_cancelbtn_event_handler);
		
	nfui_nfwindow_add((NFWINDOW*)win, fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);

	gtk_main();
	return ifn_get_gmt_from_local(dt_year, dt_month, dt_day, 
			dt_hour, dt_min, dt_sec);
}

void destroy_vw_arch_date_popup()
{
}




