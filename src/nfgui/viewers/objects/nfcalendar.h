#ifndef	__NFCALENDAR_H__
#define	__NFCALENDAR_H__


/******************************************************************
 *
 *	NFCALENDAR 
 *
 *
 *****************************************************************/


#include "nfobject.h"
#include "nffixed.h"
#include "nflabel.h"
#include "nfbutton.h"


#define	NFCAL_FONT_STRING_SIZE	64
#define	NFCAL_CASHING_KEY_SIZE	64

typedef enum {
	NFCAL_NODATA_NOFOCUS = 0,
	NFCAL_NODATA_FOCUS,
	NFCAL_DATA_NOFOCUS,
	NFCAL_DATA_FOCUS
} nfcal_day_state;

typedef enum {
	NFCAL_BUTTON_YEAR_DEC=0,
	NFCAL_BUTTON_YEAR_INC,
	NFCAL_BUTTON_MONTH_DEC,
	NFCAL_BUTTON_MONTH_INC
} nfcal_button_type;

enum {
	NFCAL_TITLE=0,
	NFCAL_DAY
};

typedef struct {
//	NFOBJECT object;
	NFFIXED fixed;

	GDate *curDate;
	NFOBJECT *day[31];

	guint data_map[31];

	nfrect rtTitle;
	nfrect rtBody;
	nfrect rtDayArea;

	nfrect rtYear;
	nfrect rtMonth;

	guint day_width;
	guint day_height;
	guint hmargin;
	guint vmargin;
	guint hspace;
	guint vspace;

	gchar *pango_font[3];
	gchar font_name[3][NFCAL_FONT_STRING_SIZE];

	gint use_cashing;
	gchar cashing_key[NFCAL_CASHING_KEY_SIZE];

	guint bg_color[4];
	guint fg_color[4];

	guint krepeat_src;
	guint rkey_id;

	gint key_lock;

} NFCALENDAR;




NFCALENDAR* nfui_nfcalendar_new(GdkPixbuf **pixbuf1, GdkPixbuf **pixbuf2);
void nfui_nfcalendar_destroy(NFOBJECT *obj);

void nfui_nfcalendar_set_state(NFCALENDAR *nfcal, guint day, guint data);
void nfui_nfcalendar_set_title_pango_font(NFCALENDAR *nfcal, const gchar *title_pfont);
void nfui_nfcalendar_set_day_pango_font(NFCALENDAR *nfcal, const gchar *day_pfont);
void nfui_nfcalendar_use_pango_cashing(NFCALENDAR *nfcal, gint cashing, gchar *key);

void nfui_nfcalendar_set_date(NFCALENDAR *nfcal, GDate *date);
void nfui_nfcalendar_get_date(NFCALENDAR *nfcal, GDate *date);
//void nfui_nfcalendar_set_date_from_tv(NFCALENDAR *nfcal, GTimeVal *tv);
//void nfui_nfcalendar_get_date_to_tv(NFCALENDAR *nfcal, GTimeVal *tv);
void nfui_nfcalendar_set_date_ymd(NFCALENDAR *nfcal, guint year, guint mon, guint day);
void nfui_nfcalendar_get_date_ymd(NFCALENDAR *nfcal, guint *year, guint *mon, guint *day);

void nfui_nfcalendar_set_keylock(NFCALENDAR *nfcal, gint lock);



/*
 
GtkWidget* gtk_nfcalendar_new();
void gtk_nfcalendar_set_cur_date(GtkWidget *widget, GTimeVal cur_time);
void gtk_nfcalendar_get_cur_date(GtkWidget *widget, GDate *date);
void gtk_nfcalendar_set_state(GtkWidget *widget, guint day, guint state);
guint gtk_nfcalendar_get_state(GTimeVal date);

void gtk_nfcalendar_update_days(GtkWidget *widget);
void gtk_nfcalendar_update_all(GtkWidget *widget);

void gtk_nfcalendar_set_label_markup(gchar *markup);
void gtk_nfcalendar_set_property_color(GdkColor *color);

gboolean gtk_nfcalendar_set_arrow_image(GtkWidget *widget, guchar btns, GtkWidget **image, guint width, guint height);
void gtk_nfcalendar_set_size(GtkWidget *widget, guint width, guint height);



 *
 */


#endif	// __NFCALENDAR_H__


