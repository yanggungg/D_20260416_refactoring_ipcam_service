#ifndef	__NFLOG_LABEL_H__
#define	__NFLOG_LABEL_H__

#include "nfobject.h"
#include "nffixed.h"


#define	NFLOGLABEL_MAX_STRING_SIZE		128
//#define	NFLOGLABEL_FONT_STRING_SIZE		64
#define	NFLOGLABEL_CASHING_KEY_SIZE		256

#define NF_LOGLABEL(widget)		((NFLOGLABEL*)widget)

typedef enum {
	LOGLABEL_TYPE = 0,
	LOGLABEL_TIME,
	LOGLABEL_SETBY,
	LOGLABEL_ALL
} LOGLABEL_E;	


typedef struct {
	NFFIXED log_fixed;
	
	NFOBJECT *label[LOGLABEL_ALL];

	GdkGC *bg_gc;

	gchar strLabel[LOGLABEL_ALL][NFLOGLABEL_CASHING_KEY_SIZE];
	guint label_width[LOGLABEL_ALL];

	guint label_fg_idx;
	guint label_bg_idx;
	
	nfalign_type align;
	guint margin;

  	nfutil_pango_spacing_type spacing_type;

  	GTimeVal log_time;

} NFLOGLABEL;


NFLOGLABEL* nfui_nfloglabel_new(gchar *pfont, guint fg_idx, guint bg_idx);
void nfui_nfloglabel_label_size_width(NFLOGLABEL *nfloglabel, LOGLABEL_E label_idx, guint size_w);
void nfui_nfloglabel_label_bg(NFLOGLABEL *nfloglabel, guint color_idx);
#endif	// __NFLOG_LABEL_H__


