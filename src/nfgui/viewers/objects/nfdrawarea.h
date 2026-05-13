
#ifndef _NF_DRAW_AREA_H_
#define _NF_DRAW_AREA_H_



/******************************************************************
 *
 *	DRAWAREA 
 *
 *
 *****************************************************************/


#include "nfobject.h"

// For Rectangle
#define		MAX_POINTS				(5)


G_BEGIN_DECLS


typedef struct _NFDRAWAREA			NFDRAWAREA;

enum {
	DRAW_NONE_STEP = 0,
	DRAW_START_STEP,	
	DRAW_EDIT_STEP,
	DRAW_MODIFY_STEP,
	DRAW_END_STEP
};

enum {
	NORMAL_STATE = 0,
	FOCUS_STATE,
	SELECT_STATE
};

enum {
	 NONE_SIDE = -1,
	 TOP_LEFT = 0,
	 TOP_MIDDLE,
	 TOP_RIGHT,
	 MIDDLE_RIGHT,
	 BOTTOM_RIGHT,
	 BOTTOM_MIDDLE,
	 BOTTOM_LEFT,
	 MIDDLE_LEFT
};

struct _NFDRAWAREA {
	NFOBJECT object;
	
	GdkPoint points[MAX_POINTS];

	GSList *list;
	guint list_length;

	gint step;
	gint focus_area;
	gint area_state;
	gint resize_side;
};


NFOBJECT* nfui_drawrect_area_new();
gboolean nfui_drawrect_set_rect(NFDRAWAREA* darea, GdkRectangle rect);
gboolean nfui_drawrect_get_rect(NFDRAWAREA* darea, GdkRectangle *rect);

G_END_DECLS


#endif

