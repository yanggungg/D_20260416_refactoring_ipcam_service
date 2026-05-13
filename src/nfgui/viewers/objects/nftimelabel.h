#ifndef	__NFTIMELABEL_H__
#define	__NFTIMELABEL_H__


/******************************************************************
 *
 *	NFCALENDAR 
 *
 *
 *****************************************************************/

#include "../../support/util.h"

#include "nfobject.h"
#include "nffixed.h"

#define	NFTL_FONT_STRING_SIZE	64
#define	NFTL_CASHING_KEY_SIZE	64

typedef enum {
	NFTL_YEAR = 0,
	NFTL_MONTH,
	NFTL_DAY,
	NFTL_HOUR,
	NFTL_MIN,
	NFTL_SEC,
	NFTL_APM,
	NFTL_FIELDS,

	NFTL_FIELD_NONE = 100,

} nftl_field_type;

typedef enum {
	NFTL_TM_HIDE = 0,
	NFTL_TM_24H,
	NFTL_TM_12H
} nftl_time_mode;

typedef enum {
	NFTL_DF_HIDE = 0,
	NFTL_DF_YMD,
	NFTL_DF_MDY,
	NFTL_DF_DMY,

	NUM_NFTL_DATE_FORMAT
} nftl_df_type;

typedef	struct {
	NFFIXED fixed;

	NFOBJECT *fields[NFTL_FIELDS];

	NFOBJECT *deco[4];
	
	GTimeVal tvTime;
	GTimeVal pre_tvTime;
	GTimeVal upper_limit;
	GTimeVal lower_limit;

	gint date_lock;

	nftl_df_type date_format;
	nftl_time_mode time_mode;

	nftl_field_type idx[NFTL_FIELDS+1];

	gchar *pango_font;
	gchar font_name[NFTL_FONT_STRING_SIZE];

	gint use_cashing;
	gchar cashing_key[NFTL_CASHING_KEY_SIZE];

	guint bg_color;
	guint fg_color;

	nfutil_pango_spacing_type spacing_type;
	
} NFTIMELABEL;


NFTIMELABEL* nfui_nftimelabel_new(void);

void nfui_nftimelabel_get_datetime(NFTIMELABEL *ntfl, GTimeVal *tv);
void nfui_nftimelabel_set_datetime(NFTIMELABEL *ntfl, GTimeVal *tv);
void nfui_nftimelabel_set_datetime_expose(NFTIMELABEL *nftl, GTimeVal *tv);
void nfui_nftimelabel_set_mode(NFTIMELABEL *ntfl, nftl_df_type date_format, nftl_time_mode tm);
void nfui_nftimelabel_set_size(NFTIMELABEL *ntfl, guint width, guint height);
void nfui_nftimelabel_set_pango_font(NFTIMELABEL *ntfl, const gchar *pfont);
void nfui_nftimelabel_use_pango_cashing(NFTIMELABEL *ntfl, gint cashing, gchar *key);
void nfui_nftimelabel_set_fg_color(NFTIMELABEL *ntfl, guint fg_index);
void nfui_nftimelabel_set_bg_color(NFTIMELABEL *ntfl, guint bg_index);
void nfui_nftimelabel_set_date_lock(NFTIMELABEL *ntfl, gboolean lock);
void nfui_nftimelabel_set_limit(NFTIMELABEL *ntfl, GTimeVal *lower, GTimeVal *upper);
void nfui_nftimelabel_refresh_datetime(NFTIMELABEL *nftl);

#endif	// __NFTIMELABEL_H__



