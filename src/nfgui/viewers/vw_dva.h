/*
 * vw_dva.h
 *
 *	- dependencies :
 *		viewer
 *
 * Written by Jungkyu. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Feb 15, 2019
 *
 */

#ifndef	_VW_DVA_H_
#define	_VW_DVA_H_

typedef enum _DVA_MEVENT_E {
	DVA_MEVENT_LEFT_PRESS 	= 0,
	DVA_MEVENT_LEFT_2PRESS 	= 1,
	DVA_MEVENT_RIGHT_PRESS 	= 2,
	DVA_MEVENT_RIGHT_2PRESS = 3,
	DVA_MEVENT_RELEASE 		= 4,
	DVA_MEVENT_DRAG 		= 5,
	DVA_MEVENT_MOVE 		= 6,
	DVA_MEVENT_ADD_POINT	= 7,
	DVA_MEVENT_DEL_POINT	= 8,	
	DVA_MEVENT_MAX
} DVA_MEVENT_E;

typedef struct _DVA_SCREEN_INFO_T {
	gint	x;
	gint	y;
	gint	w;
	gint	h;	
	float	scale_x;
	float	scale_y;
	gint	degree;
} DVA_SCREEN_INFO;

typedef struct _DVA_VMAP_INFO_T {
    gint    use;
	gint	x;
	gint	y;
	gint	w;
	gint	h;	
} DVA_VMAP_INFO;

typedef struct _DVA_MEVENT_PT_T {
	gint 	x;
	gint 	y;	
} DVA_MEVENT_PT;


typedef int (*DVA_MEVENT_CB_FUNC)(DVA_SCREEN_INFO *scrInfo, DVA_MEVENT_PT *evt_pt, gpointer user_data);
typedef int (*DVA_VMAP_CB_FUNC)(DVA_VMAP_INFO *vmapInfo, gpointer user_data);

typedef int (*DVA_SELECT_CB_FUNC)(int select_ruleid, gpointer user_data);
typedef int (*DVA_SELECT2_CB_FUNC)(int type, int select_ruleid, gpointer user_data);

#endif	/* _VW_VCA_H_ */

