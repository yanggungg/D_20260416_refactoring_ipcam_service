#ifndef	__NFTHUMBNAIL_H__
#define	__NFTHUMBNAIL_H__

#include "nfobject.h"
#include "nffixed.h"


#define THUMB_NORMAL_FG					(651)
#define THUMB_FOCUS_FG					(654)
#define THUMB_SELECT_FG					(657)
#define THUMB_DISABLE_FG				(651)

#define THUMB_NORMAL_BG					(649)
#define THUMB_FOCUS_BG					(652)
#define THUMB_SELECT_BG					(655)

#define	NFTHUMBNAIL_MAX_STRING_SIZE			128
#define	NFTHUMBNAIL_FONT_STRING_SIZE		64
#define	NFTHUMBNAIL_CASHING_KEY_SIZE		64

#define NF_THUMBNAIL(widget)		((NFTHUMBNAIL*)widget)

#define THUMBNAIL_LINE_BORDER				1

typedef enum _TH_LB_MODE_E {
	TH_LB_MODE_MIN		= 0,
	TH_LB_MODE_HOUR,
	TH_LB_MODE_DAY,
	TH_LB_MODE_MAX
} TH_LB_MODE_E;

typedef enum _THUMB_STATE_E {
	THUMB_STATE_NORMAL		= 0,
	THUMB_STATE_SELECT
} THUMB_STATE_E;

////////////////////////////////////////////////////////////////
//
// public data type
//

typedef struct {
	NFFIXED 		th_fixed;
	NFOBJECT 		*subject_label;
	NFOBJECT 		*status_label;

	gchar 			strLabel[NFTHUMBNAIL_CASHING_KEY_SIZE];
	gint 			subject_label_h;

	guint 			bg_normal_idx;

	gint 			image_width;
	gint 			image_height;

	THUMB_STATE_E 	state;
	gboolean 		focus_draw;	
} NFTHUMBNAIL;


NFTHUMBNAIL* nfui_nfthumbnail_new(gchar *strLabel, gchar *pfont);
void nfui_nfthumbnail_subject_label_height(NFTHUMBNAIL *thumbnail, guint size_h);
void nfui_nfthumbnail_use_focus_draw(NFTHUMBNAIL *thumbnail, gboolean use);

void nfui_nfthumbnail_image_open();
void nfui_nfthumbnail_image_close();
void nfui_nfthumbnail_get_image();
void nfui_nfthumbnail_set_image_buf(NFTHUMBNAIL *thumbnail, guchar *buf);
void nfui_nfthumbnail_set_channel(NFTHUMBNAIL *thumbnail, gint ch);
void nfui_nfthumbnail_set_period(NFTHUMBNAIL *thumbnail, time_t start_time, time_t end_time);
void nfui_nfthumbnail_get_period(NFTHUMBNAIL *thumbnail, time_t *start_time, time_t *end_time);
void nfui_nfthumbnail_set_image_size(NFTHUMBNAIL *thumbnail, gint width, gint height);
void nfui_nfthumbnail_set_subject_label_text(NFTHUMBNAIL *thumbnail, gchar *strLabel);
gboolean nfui_nfthumbnail_get_image_time(NFTHUMBNAIL *thumbnail, time_t *image_time);
gint nfui_nfthumbnail_get_focused_image_time(NFTHUMBNAIL *thumbnail, time_t *image_time);
gboolean nfui_nfthumbnail_check_running(void);
int nfui_nfthumbnail_wait_for_stop();
THUMB_STATE_E nfui_nfthumbnail_get_state(NFTHUMBNAIL *thumbnail);
void nfui_nfthumbnail_set_state(NFTHUMBNAIL *thumbnail, THUMB_STATE_E state);
gboolean nfui_nfthumbnail_get_focused_start_time(NFTHUMBNAIL *thumbnail, time_t *image_time);

#endif	// __NFTHUMBNAIL_H__
