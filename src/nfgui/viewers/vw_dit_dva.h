/*
 * vw_dit_dva.h
 *	- dependency :
 *		vw
 *		dit
 *
 * Written by Jungkyu. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Feb 15, 2019
 *
 */

#ifndef	_VW_DIT_DVA_H
#define	_VW_DIT_DVA_H

#include "dit.h"

typedef struct _DVA_DP {
	GdkDrawable 	*drawable;
	GdkGC 			*gc;
	GdkRectangle	plt_area;
    gint            degree;
} DVA_DP;

typedef struct _DVA_CLON {
	gint dic_cnt;	
	DICPTR *pdics;
} DVA_CLON;

typedef struct _DVA_VMAP {
    int             use;
    int             x;
    int             y;
    int             w;
    int             h;    
} DVA_VMAP;



////////////////////////////////////////////////////////////
//
// public interfaces
//

int vw_dit_display_set_dva_ditid(DITID id);
DITID vw_dit_display_get_dva_ditid();
int vw_dit_display_set_dva_vmap(DVA_VMAP *vmap);

int vw_dit_display_get_dva_diclist(DVA_CLON *clon);
int vw_dit_display_free_dva_diclist(DVA_CLON clon);
int vw_dit_display_compare_dva_diclist(DVA_CLON pre_clon, DVA_CLON post_clon);

int vw_dit_display_get_dva_region(DVA_DP *pdp, DVA_CLON clon, GdkRegion *region);
int vw_dit_display_dva_erase(DVA_DP *pdp, DVA_CLON pre_clon, DVA_CLON post_clon);
int vw_dit_display_dva_draw(DVA_DP *pdp, DVA_CLON clon);

#endif	/* _VW_DIT_DVA_H */
