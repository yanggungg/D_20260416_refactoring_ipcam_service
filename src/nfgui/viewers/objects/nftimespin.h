#ifndef	__NFTIMESPIN_H__
#define	__NFTIMESPIN_H__


/******************************************************************
 *
 *	NFCALENDAR 
 *
 *
 *****************************************************************/

#include "../../support/util.h"

#include "nfobject.h"
#include "nffixed.h"

#define	NFTS_FONT_STRING_SIZE	64
#define	NFTS_CASHING_KEY_SIZE	64

enum {
	NFTS_NOFOCUS = 0,
	NFTS_FOCUS
};

typedef enum {
	NFTS_YEAR = 0,
	NFTS_MONTH,
	NFTS_DAY,
	NFTS_HOUR,
	NFTS_MIN,
	NFTS_SEC,
	NFTS_APM,
	NFTS_FIELDS,

	NFTS_FIELD_NONE = 100,

} nfts_field_type;

typedef enum {
	NFTS_TM_HIDE = 0,
	NFTS_TM_24H,
	NFTS_TM_12H
} nfts_time_mode;

typedef enum {
	NFTS_DF_HIDE = 0,
	NFTS_DF_YMD,
	NFTS_DF_MDY,
	NFTS_DF_DMY,

	NUM_NFTS_DATE_FORMAT
} nfts_df_type;

typedef	struct {
	NFFIXED fixed;

	NFOBJECT *fields[NFTS_FIELDS];
	NFOBJECT *up_btn;
	NFOBJECT *down_btn;

	NFOBJECT *deco[4];

	guint btn_width;
	guint btn_height;
	
	GTimeVal tvTime;
	GTimeVal pre_tvTime;
	GTimeVal upper_limit;
	GTimeVal lower_limit;

	gint date_lock;

	nfts_df_type date_format;
	nfts_time_mode time_mode;
	nfts_field_type focus;

	nfts_field_type idx[NFTS_FIELDS+1];

	gchar *pango_font;
	gchar font_name[NFTS_FONT_STRING_SIZE];

	gint use_cashing;
	gchar cashing_key[NFTS_CASHING_KEY_SIZE];

	guint bg_color[2];
	guint fg_color[2];

	guint krepeat_src;
	guint mrepeat_src;
	guint rkey_id;

	nfutil_pango_spacing_type spacing_type;
	
} NFTIMESPIN;


NFTIMESPIN* nfui_nftimespin_new(GdkPixbuf **up_img, GdkPixbuf **down_img);
NFTIMESPIN* nfui_nftimespin_new_with_time(GdkPixbuf **up_img, GdkPixbuf **down_img, GTimeVal *tv);
void nfui_nftimespin_get_datetime(NFTIMESPIN *nfts, GTimeVal *tv);
void nfui_nftimespin_set_datetime(NFTIMESPIN *nfts, GTimeVal *tv);
void nfui_nftimespin_set_mode(NFTIMESPIN *nfts, nfts_df_type date_format, nfts_time_mode tm);
void nfui_nftimespin_set_size(NFTIMESPIN *nfts, guint width, guint height);
void nfui_nftimespin_set_pango_font(NFTIMESPIN *nfts, const gchar *pfont);
void nfui_nftimespin_use_pango_cashing(NFTIMESPIN *nfts, gint cashing, gchar *key);
void nfui_nftimespin_set_fg_color(NFTIMESPIN *nfts, guint focus, guint normal);
void nfui_nftimespin_set_bg_color(NFTIMESPIN *nfts, guint focus, guint normal);
void nfui_nftimespin_set_date_lock(NFTIMESPIN *nfts, gboolean lock);
void nfui_nftimespin_set_limit(NFTIMESPIN *nfts, GTimeVal *lower, GTimeVal *upper);


#endif	// __NFTIMESPIN_H__



