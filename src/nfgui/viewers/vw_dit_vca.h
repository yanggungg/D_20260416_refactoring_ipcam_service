/*
 * vw_dit_vca.h
 *	- dependency :
 *		vw
 *		dit
 *
 * Written by Jungkyu. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Feb 15, 2019
 *
 */

#ifndef	_VW_DIT_VCA_H
#define	_VW_DIT_VCA_H

#include "dit.h"

typedef struct _VCA_DP {
	GdkDrawable 	*drawable;
	GdkGC 			*gc;
	GdkRectangle	plt_area;			
} VCA_DP;

typedef struct _VCA_CLON {
	gint dic_cnt;	
	DICPTR *pdics;
} VCA_CLON;

typedef struct _VCA_VMAP {
    int             use;
    int             x;
    int             y;
    int             w;
    int             h;    
} VCA_VMAP;



////////////////////////////////////////////////////////////
//
// public interfaces
//

int vw_dit_display_set_vca_ditid(DITID id);
int vw_dit_display_set_vca_vmap(VCA_VMAP *vmap);

int vw_dit_display_get_vca_diclist(VCA_CLON *clon);
int vw_dit_display_free_vca_diclist(VCA_CLON clon);
int vw_dit_display_compare_vca_diclist(VCA_CLON pre_clon, VCA_CLON post_clon);

int vw_dit_display_get_vca_region(VCA_DP *pdp, VCA_CLON clon, GdkRegion *region);
int vw_dit_display_vca_erase(VCA_DP *pdp, VCA_CLON pre_clon, VCA_CLON post_clon);
int vw_dit_display_vca_draw(VCA_DP *pdp, VCA_CLON clon);

#endif	/* _VW_DIT_VCA_H */
