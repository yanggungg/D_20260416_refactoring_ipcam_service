
#ifndef	_VW_CHANGE_COLOR_POPUP_H_
#define	_VW_CHANGE_COLOR_POPUP_H_

typedef struct _VW_COLOR_CONF_T {
    gint        ncolor;
    gint        color_idx[40];
    gint        select;
} VW_COLOR_CONF_T;

typedef struct _VW_COLOR_SHAPE_T {
    gint        x;
    gint        y;    
    gint        cnt_col;
} VW_COLOR_SHAPE_T;

gint vw_change_color_popup_open(NFWINDOW *parent, VW_COLOR_CONF_T *conf, VW_COLOR_SHAPE_T *shape);

#endif	/* _VW_CHANGE_COLOR_POPUP_H_ */

