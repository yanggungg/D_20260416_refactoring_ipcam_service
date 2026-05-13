


#ifndef	__NF_TILE_H__
#define	__NF_TILE_H__

/******************************************************************
 *
 *	NFTILE 
 *
 *
 *****************************************************************/



#include "nfobject.h"

G_BEGIN_DECLS


#define NF_TILE(widget)		((NFTILE*)widget)

#define COLOR_COUNT			6


typedef struct _NFTILE 			NFTILE;
typedef struct _NFTILEAREA		NFTileArea;	

typedef enum {
	NFTILE_STATE_NONE 		= 0,
	NFTILE_STATE_NORMAL 	= 1 << 0,
	NFTILE_STATE_FOCUS 		= 1 << 1,
	NFTILE_STATE_SELECT 	= 1 << 2,
	NFTILE_STATE_RESELECT 	= 1 << 3
}NFTileState;

enum {
	NORMAL_STATE_COLOR = 0,
	FOCUS_STATE_COLOR,
	SELECT_STATE_COLOR_1,
	SELECT_STATE_COLOR_2,
	SELECT_STATE_COLOR_3,
	SELECT_STATE_COLOR_4,
	SELECT_STATE_EX_COLOR_24 = 24
};

enum {
	POINTER_IN_TILE = 1 ,
	POINTER_OUT_TILE, 
	POINTER_INNOUT
};

struct _NFTILE {
	NFOBJECT object;
	
	GdkColor color[COLOR_COUNT];
	GdkGC *bg_gc[COLOR_COUNT];
	GdkGC *line_gc[COLOR_COUNT];

	guint row_size;
	guint col_size;
	gint line_border;

	guint select_n;

	NFTileArea **tarea;

	NFTileArea *start_area;
	NFTileArea *end_area;

	GdkPoint outPoints[5];

	gboolean sensitive;
	gboolean pressed;
	gboolean can_reselect;
	gboolean filled;
	gboolean draw_outline;
	gchar innout;
	

	/* For key navi */
	gboolean pushed_enter;

};


struct _NFTILEAREA {
	guint row;
	guint col;

	guint x;
	guint y;
	guint width;
	guint height;

	NFTileState state;
	guint select_color;
	guint preSelect_color;
};



NFOBJECT* nfui_tile_new(guint row, guint col);
void nfui_tile_set_selectArea(NFTILE *tile, guint row1, guint col1, guint row2, guint col2);
void nfui_tile_get_selectArea(NFTILE *tile, guint *row1, guint *col1, guint *row2, guint *col2);
void nfui_tile_set_selectRect(NFTILE *tile, GdkRectangle rect);
void nfui_tile_get_selectRect(NFTILE *tile, GdkRectangle *rect);
void nfui_tile_get_start_area(NFTILE *tile, guint *row, guint *col);
void nfui_tile_get_end_area(NFTILE *tile, guint *row, guint *col);
void nfui_tile_conv_selectArea(NFTILE *tile, guint row1, guint col1, guint row2, guint col2);
void nfui_tile_draw_color(NFTILE *tile, guint color_n, guint row1, guint col1, guint row2, guint col2);
void nfui_tile_no_draw_color(NFTILE *tile, guint color_n, guint row1, guint col1, guint row2, guint col2);

guint nfui_tile_get_cur_state(NFTILE *tile);
void nfui_tile_sensitive(NFTILE *tile, gboolean sensitive);
void nfui_tile_set_color(NFTILE *tile, NFTileState state, GdkColor *state_color); 
void nfui_tile_set_line_color(NFTILE *tile, NFTileState state, GdkColor *state_color); 
void nfui_tile_can_reSelect(NFTILE *tile, gboolean reselect);
gint nfui_tile_get_state(NFTILE *tile, guint row, guint col);
gboolean nfui_tile_drawable(NFTILE *tile);
void nfui_tile_set_fill(NFTILE *tile, gboolean filled);
void nfui_tile_set_drawable_outline(NFTILE *tile, gboolean filled);
void nfui_tile_set_area_color(NFTILE *tile, guint color_n, guint row1, guint col1, guint row2, guint col2);
gint nfui_tile_reset_area_size(NFTILE *tile, guint rows, guint cols);
void nfui_tile_set_line_border(NFTILE *tile, gint border);

gboolean nfui_tile_set_state(NFTILE *tile, NFTileState state, guint row, guint col);
void nfui_tile_draw_area(NFTILE *tile, guint row1, guint col1, guint row2, guint col2);

G_END_DECLS

#endif	// __NF_TILE_H__
