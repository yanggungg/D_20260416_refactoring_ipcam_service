/*
 * vw_vca.h
 *
 *	- dependencies :
 *		viewer
 *
 * Written by JUNGKYU PARK. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, May 16, 2013
 *
 */

#ifndef	_VW_VCA_H_
#define	_VW_VCA_H_


typedef enum _VCA_MEVENT_E {
	VCA_MEVENT_LEFT_PRESS 	= 0,
	VCA_MEVENT_LEFT_2PRESS 	= 1,
	VCA_MEVENT_RIGHT_PRESS 	= 2,
	VCA_MEVENT_RIGHT_2PRESS = 3,
	VCA_MEVENT_RELEASE 		= 4,
	VCA_MEVENT_DRAG 		= 5,
	VCA_MEVENT_MOVE 		= 6,
	VCA_MEVENT_ADD_POINT	= 7,
	VCA_MEVENT_DEL_POINT	= 8,	
	VCA_MEVENT_MAX
} VCA_MEVENT_E;

typedef struct _VCA_SCREEN_INFO_T {
	gint	x;
	gint	y;
	gint	w;
	gint	h;	
} VCA_SCREEN_INFO;

typedef struct _VCA_VMAP_INFO_T {
    gint    use;
	gint	x;
	gint	y;
	gint	w;
	gint	h;	
} VCA_VMAP_INFO;

typedef struct _VCA_MEVENT_PT_T {
	gint 	x;
	gint 	y;	
} VCA_MEVENT_PT;


typedef int (*VCA_MEVENT_CB_FUNC)(VCA_SCREEN_INFO *scrInfo, VCA_MEVENT_PT *evt_pt, gpointer user_data);
typedef int (*VCA_VMAP_CB_FUNC)(VCA_VMAP_INFO *vmapInfo, gpointer user_data);

typedef int (*VCA_SELECT_CB_FUNC)(int select_ruleid, gpointer user_data);
typedef int (*VCA_SELECT2_CB_FUNC)(int type, int select_ruleid, gpointer user_data);

#endif	/* _VW_VCA_H_ */

