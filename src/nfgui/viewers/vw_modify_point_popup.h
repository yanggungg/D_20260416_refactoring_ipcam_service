#ifndef _VW_MODIFY_POINT_POPUP_H_
#define _VW_MODIFY_POINT_POPUP_H_

enum {
	ADD_POINT	= 0,
	DEL_POINT	= 1,
};

gint VW_modify_point_popup_open(NFWINDOW *parent, gint pos_x, gint pos_y, guint enable);

#endif

