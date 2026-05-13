// vim:ts=4:sw=4
/*******************************************************************************
*  (c) COPYRIGHT 2012 ITX security                                             *
*  ALL RIGHT RESERVED                                                          *
*                                                                              *
*  Jongbin Yim,  jongbina@itxsecurity.com                                      *
********************************************************************************

REVISION HISTORY:

Date       Name           Description
__________ ______________ ______________________________________________________
2012/07/24 Jongbin Yim    Created

................................................................................

DESCRIPTION:

................................................................................
*/

/**
 * @file  vw_vca_internal.h
 * @brief
 */

#ifndef	_VW_VCA_INTERNAL_H_
#define	_VW_VCA_INTERNAL_H_

/*******************************************************************************
 * includes                                                                    *
 ******************************************************************************/

#include <glib.h>

/*******************************************************************************
 * defines                                                                     *
 ******************************************************************************/

//#include "nf_api_vca.h"
#include "ivca_def.h"

/*******************************************************************************
 * typedefs                                                                    *
 ******************************************************************************/

/*******************************************************************************
 * globals                                                                     *
 ******************************************************************************/

/*******************************************************************************
 * locals                                                                      *
 ******************************************************************************/

/*******************************************************************************
 * function prototypes                                                         *
 ******************************************************************************/

typedef enum _DRAW_TYPE{
    DRAW_TARGET  = 0,
    SELECT_TARGET  = 1,
    ERASE_TARGET = 2
} DRAW_TYPE;

gint vw_vca_line_test(gint x, gint y, gint d, ivca_point_t *pt1, ivca_point_t *pt2);

gint vw_vca_polygon_test(gint x, gint y, gint d, gint npts, ivca_point_t *pt);

void vw_vca_draw_str(NFOBJECT *obj, GdkDrawable *drawable, GdkGC *gc,
		gint erase, gchar *str);

void vw_vca_draw_zones(NFOBJECT *obj, GdkDrawable *drawable, GdkGC *gc,
		gint ensel, gint erase, gint *emask, ivca_rule_t *rl,
		gint rx, gint ry, gint zidxc, gint zidxp, gint pidxc, gint pidxp);

void vw_vca_draw_cntrs(NFOBJECT *obj, GdkDrawable *drawable, GdkGC *gc,
		gint ensel, gint erase, gint *emask, ivca_rule_t *rl,
		gint rx, gint ry, gint cidxc, gint cidxp, gchar *font, gint *cval);

void vw_vca_draw_ti(NFOBJECT *obj, GdkDrawable *drawable, GdkGC *gc, gint erase,
		ivca_option_t *og, ivca_option_t *ogp, gint count, ivca_meta_obj_t *ti,
		gint Rw, gint Rh);

gint vw_vca_add_zone(NFOBJECT *parent, ivca_rule_t *r, gint zone_type);
gint vw_vca_add_cntr(NFOBJECT *parent, ivca_rule_t *r);
gint vw_vca_delete_zone(gint idx, ivca_rule_t *r);
gint vw_vca_delete_cntr(gint idx, ivca_rule_t *r);

void vw_vca_draw_target( NFOBJECT *obj, GdkDrawable *drawable, GdkGC *gc,
                         DRAW_TYPE draw_type, ivca_calib_target_t* target);

gint vw_vca_add_target(ivca_calib_target_t *target);
gint vw_vca_targets_estimate(NFOBJECT *parent, gint w, gint h, ivca_calib_t* t);
#endif	/* _VW_VCA_INTERNAL_H_ */

