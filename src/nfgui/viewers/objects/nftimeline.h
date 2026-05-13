



#ifndef	__NF_TIMELINE_H__
#define	__NF_TIMELINE_H__

/******************************************************************
 *
 *	NFTIMELINE 
 *
 *
 *****************************************************************/


#ifndef _GUIDEVP
#include "nf_api_play.h"
#endif	// _GUIDEVP

#include "nfobject.h"


G_BEGIN_DECLS


#define	TIMEBAR_POINTS					6 
#define EVENT_TYPE_COUNT				6

#define NF_TIMELINE(widget)		((NFTIMELINE*)widget)


typedef struct _NFTIMELINE 			NFTIMELINE;

struct _NFTIMELINE {
	NFOBJECT object;

	guint cell_width;
	guint cell_height;

	guint data_height;


	guint columns;
	guint rows;
	guint row_space;
	guint cur_pos;

	gint da_x;
	gint da_y;
	gint da_w;
	gint da_h;

	gint row_color[2];
//	gint data_color[10];


	GdkGC *odd_gc;
	GdkGC *even_gc;

	GdkGC *bg_gc;
	GdkGC *tbar_gc;
	GdkGC *sbar_gc[EVENT_TYPE_COUNT];
#if defined(__SAMSUNG_UI__)
	GdkGC *inline_gc;
#endif

	gboolean is_pressed;

	gchar *t_data;
};



//NFOBJECT* nfui_timeline_new(guint channel);
NFOBJECT* nfui_timeline_new(guint cell_w, guint cell_h, guint cols, guint rows, guint row_space);
void nfui_timeline_set_data(NFTIMELINE *timeline, gchar *t_elem);

void nfui_timeline_set_position(NFTIMELINE *timeline, guint count);
guint nfui_timeline_get_position(NFTIMELINE *timeline);


void nfui_timeline_set_row_color(NFTIMELINE *timeline, gint odd_color_idx, gint even_color_idx);
void nfui_timeline_set_data_height(NFTIMELINE *timeline, gint data_height);

G_END_DECLS

#endif	// __NF_TIMELINE_H__


