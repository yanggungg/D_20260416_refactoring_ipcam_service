

#ifndef	__NF_TIMELINE2_H__
#define	__NF_TIMELINE2_H__

#include "nfobject.h"


#define	NFTL2_MAX_COLUMNS	300
#define	NFTL2_MAX_ROWS		36


typedef struct {
	NFOBJECT object;

	guint cell_width;
	guint cell_height;
	guint data_height;


	guint columns;
	guint rows;

	guint row_space;

	guint cur_pos;
	guint is_pressed;

	gint da_x;
	gint da_y;
	gint da_w;
	gint da_h;

	GTimeVal start_time;
	guint resolution;
	guint data_num;

	gint offset;
//	guint pre_data;
//	guint post_data;

	guint data[NFTL2_MAX_ROWS][NFTL2_MAX_COLUMNS];

	gint row_color[2];	// 0:odd, 1:even
	gint data_color[10];
} NFTIMELINE2;




NFOBJECT* nfui_nftimeline2_new(guint cell_w, guint cell_h, guint cols, guint rows, guint row_space);

void nfui_nftimeline2_set_data(NFTIMELINE2 *nftl2, gchar *t_elem, GTimeVal start, gint resolution, gint count,  gint offset);

void nfui_nftimeline2_set_position(NFTIMELINE2 *nftl2, gint pos);
gint nfui_nftimeline2_get_position(NFTIMELINE2 *nftl2);
void nfui_nftimeline2_set_tv(NFTIMELINE2 *nftl2, GTimeVal tv);
void nfui_nftimeline2_get_tv(NFTIMELINE2 *nftl2, GTimeVal *tv);

void nfui_nftimeline2_set_offset(NFTIMELINE2 *nftl2, gint offset);
gint nfui_nftimeline2_get_offset(NFTIMELINE2 *nftl2, gint offset);






#endif




