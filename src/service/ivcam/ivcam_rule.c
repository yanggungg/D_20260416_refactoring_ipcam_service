// vim:ts=4:sw=4
/*******************************************************************************
*  (c) COPYRIGHT 2013 ITX security                                             *
*  ALL RIGHT RESERVED                                                          *
*                                                                              *
*  Jongbin Yim,  jongbina@itxsecurity.com                                      *
********************************************************************************

REVISION HISTORY:

Date       Name           Description
__________ ______________ ______________________________________________________
2013/02/25 Jongbin Yim    Created.

................................................................................
DESCRIPTION:

................................................................................
*/

/**
 * @file  ivcam_rule.c
 * @brief  This file contains implementation of rule detection from meta data.
 * @author  Jongbin Yim.
 * @data  2013/02/25
 */

/*******************************************************************************
 * includes                                                                    *
 ******************************************************************************/

#include <string.h>
#include <stdlib.h>
#include "ivcam_rule.h"
#include "ivcam_tracker.h"
#include "list.h"

/*******************************************************************************
 * defines                                                                     *
 ******************************************************************************/

#ifndef	max
#define	max(x, y)		((x) > (y) ? (x) : (y))
#endif

#ifndef	min
#define	min(x, y)		((x) < (y) ? (x) : (y))
#endif

#ifndef	max3
#define max3(x, y, z)	((x) > max(y, z) ? (x) : max(y, z))
#endif

#ifndef	min3
#define min3(x, y, z)	((x) < min(y, z) ? (x) : min(y, z))
#endif

#define	_SZBIN_C		(384 / IVCA_NRBIN_HSVHIST_C)
#define	_SZBIN_G		(256 / IVCA_NRBIN_HSVHIST_G)

#define	_HSV2IDX(H, S, V, i)							\
{														\
	if ( (S) < 25 || (V) < 40 )	/* Gray. */				\
		(i) = IVCA_NRBIN_HSVHIST_C + (V) / _SZBIN_G;	\
	else {						/* Color. */			\
		(i) = ((H) + _SZBIN_C / 2) / _SZBIN_C;			\
		if ( (i) >= IVCA_NRBIN_HSVHIST_C )		 		\
			(i) -= IVCA_NRBIN_HSVHIST_C;		  		\
	}										  			\
}

#define	_HSV2IDX_V2(H, S, V, i, T)						\
{														\
	if ( (S) < 25 || (V) < 40 )	/* Gray. */				\
		(i) = IVCA_NRBIN_HSVHIST_C + (V) / _SZBIN_G;	\
	else {						/* Color. */			\
		i = T[H];										\
	}										  			\
}

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
 * function declarations                                                       *
 ******************************************************************************/

/*
 * H: 0-383. (corresponds to 0-359 degree)
 * S: 0-128. (corresponds to 0-1)
 * V: 0-255. (corresponds to 0-1)
 */
static void
_rgb2hsv(ivca_u08_t *rgb, ivca_u16_t *hsv)
{
	int R = rgb[0], G = rgb[1], B = rgb[2];
	int H, V = max3(R, G, B), m = min3(R, G, B), S = V ? (V - m) * 128 / V : 0;

	if ( S == 0 )
		H = 0;
	else if ( V == R ) {
		H = 64 * (G - B) / (V - m);
		if ( H < 0 )
			H += 384;
	}
	else if ( V == G )
		H = 128 + 64 * (B - R) / (V - m);
	else
		H = 256 + 64 * (R - G) / (V - m);

	hsv[0] = H;
	hsv[1] = S;
	hsv[2] = V;
}	/* _rgb2hsv(... */

/**
 * @brief  Tests whether a point (x, y) is inside polygon or not.
 *
 * @remark  If lines between points are already computed, then they can be used.
 *
 * @param[in] x  x coordinate of the test point.
 * @param[in] y  y coordinate of the test point.
 * @param[in] npts  Number of points in a polygon.
 * @param[in] pt  Points of the polygon. (length = npts)
 *
 * @return
 *  - 0 if the point is exterior.
 *  - 1 if the point is interior.
 */
static ivca_s32_t
_point_polygon_test(ivca_s16_t x, ivca_s16_t y,
		ivca_s32_t npts, ivca_point_t *pt)
{
	ivca_s32_t i, j, left = 0, dotp;
	ivca_point_t *p = pt;

	for (i = 0, j = npts - 1; i < npts; j = i, i++) {
		if ( (p[i].y < y && y <= p[j].y) || (p[j].y < y && y <= p[i].y) ) {
			/* Check the sign of dot(l, x). */
			dotp = (p[j].y - p[i].y) * x + (p[i].x - p[j].x) * y +
					p[j].x * p[i].y - p[i].x * p[j].y;
			if ( p[j].y < p[i].y )
				dotp = -dotp;
			if ( dotp < 0 )
				left++;
		}
	}
	return left & 1;	/* Odd count means (x, y) is interior point */
}	/* _point_polygon_test(... */

static ivca_s32_t
ai_zone_check_area_by_presence(ai_ivcam_track_obj_t *obj, ivca_zone_t *z)
{
	ivca_u16_t cidx = (obj->t_tail + NR_REFP_HIST - 1) % NR_REFP_HIST;

	return _point_polygon_test(obj->traj[cidx].x,
			obj->traj[cidx].y, z->npts, z->pt);
}	/* zone_check_area(... */

/**
 * @brief  Checks and update the crossing state of an object for a zone.
 *  (for area type)
 *
 * @param[in] obj  Poniter to the track object.
 * @param[in] z  Pointer to the zone.
 * @param[in] index  Index of the zone.
 */
static void
ai_zone_check_area_by_crossing(ai_ivcam_track_obj_t *obj, ivca_zone_t *z,
		ivca_s32_t index)
{
	ivca_s32_t i, j;
	ivca_u16_t cidx = (obj->t_tail + NR_REFP_HIST - 1) % NR_REFP_HIST;
	ivca_u16_t pidx = (obj->t_tail + NR_REFP_HIST - 2) % NR_REFP_HIST;
	ivca_s32_t l1[3], l2[3];
	ivca_s32_t p1, p2, pp1, pp2;
	ivca_s32_t opt1x, opt1y, opt2x, opt2y;

	if ( obj->npts < 2 )
		return;

	opt1x = obj->traj[cidx].x;
	opt1y = obj->traj[cidx].y;
	opt2x = obj->traj[pidx].x;
	opt2y = obj->traj[pidx].y;

	if ( opt1x == opt2x && opt1y == opt2y )
		return;

	/* Compute recent trajectory line. */
	l2[0] = opt1y - opt2y;
	l2[1] = opt2x - opt1x;
	l2[2] = opt1x * opt2y - opt2x * opt1y;

	for (i = 0, j = z->npts - 1; i < z->npts; j = i, i++) {
		/* Compute line by cross(p1, p2). */
		// compute zone line XXX --> pre-compute...
		l1[0] = z->pt[j].y - z->pt[i].y;
		l1[1] = z->pt[i].x - z->pt[j].x;
		l1[2] = z->pt[j].x * z->pt[i].y - z->pt[i].x * z->pt[j].y;

		p1 = l1[0] * opt1x + l1[1] * opt1y + l1[2];
		p2 = l1[0] * opt2x + l1[1] * opt2y + l1[2];

		/* Check when opt1 point is not on the zone line. */
		if ( p1 != 0 && (ivca_s64_t)p1 * p2 <= 0 ) {
			pp1 = l2[0] * z->pt[j].x + l2[1] * z->pt[j].y + l2[2];
			pp2 = l2[0] * z->pt[i].x + l2[1] * z->pt[i].y + l2[2];

			if ( (ivca_s64_t)pp1 * pp2 <= 0 ) {
				/* Line crossing for segment (j,i) occurred. */
				obj->zstate[index].cnt_on = 0;
				obj->zstate[index].cnt_off = 0;
				obj->zstate[index].state = p1 > 0 ?
						IVCA_ET_ENTER : IVCA_ET_EXIT;
				break;
			}
		}
	}
}	/* zone_check_by_crossing(... */


static void
ai_zone_check_line(ai_ivcam_track_obj_t *obj, ivca_zone_t *z, ivca_s32_t index)
{
	ivca_u16_t cidx = (obj->t_tail + NR_REFP_HIST - 1) % NR_REFP_HIST;
	ivca_u16_t pidx = (obj->t_tail + NR_REFP_HIST - 2) % NR_REFP_HIST;
	ivca_s32_t l1[3], l2[3];
	ivca_s32_t p1, p2, pp1, pp2;
	ivca_s32_t opt1x, opt1y, opt2x, opt2y;

	/* Skip if this object is not matched or stopped.*/
	if ( /*!obj->match ||*/ obj->npts < 2 )
		return;

	opt1x = obj->traj[cidx].x;
	opt1y = obj->traj[cidx].y;
	opt2x = obj->traj[pidx].x;
	opt2y = obj->traj[pidx].y;

	if ( opt1x != opt2x || opt1y != opt2y ) {
		/* Compute line by cross(p1, p2) */
		// compute zone line XXX --> pre-compute...
		l1[0] = z->pt[0].y - z->pt[1].y;
		l1[1] = z->pt[1].x - z->pt[0].x;
		l1[2] = z->pt[0].x * z->pt[1].y - z->pt[1].x * z->pt[0].y;

		p1 = l1[0] * opt1x + l1[1] * opt1y + l1[2];
		p2 = l1[0] * opt2x + l1[1] * opt2y + l1[2];

		/* Check when opt1 point is not on the zone line. */
		if ( p1 != 0 && (ivca_s64_t)p1 * p2 <= 0 ) {
			/* Compute recent trajectory line */
			l2[0] = opt1y - opt2y;
			l2[1] = opt2x - opt1x;
			l2[2] = opt1x * opt2y - opt2x * opt1y;

			pp1 = l2[0] * z->pt[0].x + l2[1] * z->pt[0].y + l2[2];
			pp2 = l2[0] * z->pt[1].x + l2[1] * z->pt[1].y + l2[2];

			if ( (ivca_s64_t)pp1 * pp2 <= 0 ) {
				obj->zstate[index].cnt_on = 0;
				obj->zstate[index].state = p1 > 0 ?
						IVCA_ET_DIR_POS : IVCA_ET_DIR_NEG;
			}
		}
	}
}	/* zone_check_line(... */

static ivca_s32_t
ai_zone_check_size(ai_ivcam_track_obj_t *obj, ivca_zone_t *z)
{
	ai_meta_obj_t *mobj = &obj->mobj;
	ivca_u32_t gw, gh;

	//printf("ai_zone_check_size ID %d %d %d %d %d %d %d \n",mobj->id,mobj->width3d,mobj->height3d,z->size_min[0],z->size_max[0],z->size_min[1],z->size_max[1]);
	if(mobj->width3d == 0 && mobj->height3d == 0)
		return 0;

	/* Convert to mm, Q.0. */
	gw = (mobj->width3d * 1000) >> 8;
	gh = (mobj->height3d * 1000) >> 8;
	gw = min(gw, IVCA_WIDTH3D_MAX);
	gh = min(gh, IVCA_HEIGHT3D_MAX);

	return gw >= z->size_min[0] && gw <= z->size_max[0] &&
			gh >= z->size_min[1] && gh <= z->size_max[1];
}	

static ivca_s32_t
ai_zone_check_speed(ai_ivcam_track_obj_t *obj, ivca_zone_t *z)
{
	ai_meta_obj_t *mobj = &obj->mobj;
	ivca_u32_t gs;

	/* Convert to Q.0. */
	gs = mobj->speed3d >> 8;

	return gs >= z->speed_min && gs <= z->speed_max;
}	


/**
 * @brief  Tests whether an object is inside a zone or not.
 *
 * @param[in] obj  Pointer to the track object.
 * @param[in] z  Pointer to the zone.
 *
 * @return
 *  - c 0 if the object is outside of the zone.
 *  - c 1 if the object is inside of the zone.
 */
static ivca_s32_t
zone_check_area(ivcam_track_obj_t *obj, ivca_zone_t *z)
{
	ivca_u16_t cidx = (obj->t_tail + NR_REFP_HIST - 1) % NR_REFP_HIST;

	return _point_polygon_test(obj->traj[cidx].x,
			obj->traj[cidx].y, z->npts, z->pt);
}	/* zone_check_area(... */

/**
 * @brief  Checks and updates the line crossing state of an object.
 *
 * @param[in] obj  Poniter to the track object.
 * @param[in] z  Pointer to the zone.
 * @param[in] index  Index of the zone.
 */
static void
zone_check_line(ivcam_track_obj_t *obj, ivca_zone_t *z, ivca_s32_t index)
{
	ivca_u16_t cidx = (obj->t_tail + NR_REFP_HIST - 1) % NR_REFP_HIST;
	ivca_u16_t pidx = (obj->t_tail + NR_REFP_HIST - 2) % NR_REFP_HIST;
	ivca_s32_t l1[3], l2[3];
	ivca_s32_t p1, p2, pp1, pp2;
	ivca_s32_t opt1x, opt1y, opt2x, opt2y;

	/* Skip if this object is not matched or stopped.*/
	if ( /*!obj->match ||*/ obj->npts < 2 )
		return;

	opt1x = obj->traj[cidx].x;
	opt1y = obj->traj[cidx].y;
	opt2x = obj->traj[pidx].x;
	opt2y = obj->traj[pidx].y;

	if ( opt1x != opt2x || opt1y != opt2y ) {
		/* Compute line by cross(p1, p2) */
		// compute zone line XXX --> pre-compute...
		l1[0] = z->pt[0].y - z->pt[1].y;
		l1[1] = z->pt[1].x - z->pt[0].x;
		l1[2] = z->pt[0].x * z->pt[1].y - z->pt[1].x * z->pt[0].y;

		p1 = l1[0] * opt1x + l1[1] * opt1y + l1[2];
		p2 = l1[0] * opt2x + l1[1] * opt2y + l1[2];

		/* Check when opt1 point is not on the zone line. */
		if ( p1 != 0 && (ivca_s64_t)p1 * p2 <= 0 ) {
			/* Compute recent trajectory line */
			l2[0] = opt1y - opt2y;
			l2[1] = opt2x - opt1x;
			l2[2] = opt1x * opt2y - opt2x * opt1y;

			pp1 = l2[0] * z->pt[0].x + l2[1] * z->pt[0].y + l2[2];
			pp2 = l2[0] * z->pt[1].x + l2[1] * z->pt[1].y + l2[2];

			if ( (ivca_s64_t)pp1 * pp2 <= 0 ) {
				obj->zstate[index].cnt_on = 0;
				obj->zstate[index].state = p1 > 0 ?
						IVCA_ET_DIR_POS : IVCA_ET_DIR_NEG;
			}
		}
	}
}	/* zone_check_line(... */

ivca_u16_t _h2idx[384] = { 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
	2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 
	5, 5, 5, 5, 5, 5, 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
};

/**
 * @brief  Tests whether the color of an object is similar to the specified
 *  color or not.
 *
 * @param[in] hdata  Pointer to data of the indexed HSV histogram of the target.
 * @param[in] z  Pointer to the zone.
 *
 * @return
 *  - c 0 if the color is different.
 *  - c 1 if the color is similar.
 */
static ivca_s32_t
zone_check_color(ivca_u16_t *hdata, ivca_zone_t *z)
{
	ivca_s32_t hidx, score;
#if EN_REVISED_CFILTER == 0
	ivca_s32_t s, e, d, ew;
#endif
	ivca_u16_t hsv[3];
	ivca_u08_t sense = z->ecolor_sens;

	if ( sense > IVCA_ECOLOR_SENS_MAX )
		sense = IVCA_ECOLOR_SENS_MAX;
	_rgb2hsv(z->ecolor, hsv);

#if EN_REVISED_CFILTER == 1
	_HSV2IDX_V2(hsv[0], hsv[1], hsv[2], hidx, _h2idx);

	score = hdata[hidx];
	return ((score * 100) >> 14) > 40 - 20 * sense / IVCA_ECOLOR_SENS_MAX;
#else
	_HSV2IDX(hsv[0], hsv[1], hsv[2], hidx);

	ew = max(sense - 3, 0) * 4 * IVCA_NRBIN_HSVHIST / 28 / 4;
	s = hidx * 4 - ew;
	e = (hidx + 1) * 4 - 1 + ew;
	if ( hidx >= IVCA_NRBIN_HSVHIST_C ) {	/* Gray. */
		s = max(s, IVCA_NRBIN_HSVHIST_C * 4);
		e = min(e, IVCA_NRBIN_HSVHIST * 4 - 1);
	}
	/* Compute score. */
	for (score = 0; s <= e; s += d) {
		d = min(4 - (s & (4 - 1)), e - s + 1);
		score += hdata[s / 4] * d / 4;
	}

	return ((score * 100) >> 14) > 60 - 50 * sense / IVCA_ECOLOR_SENS_MAX;
#endif
}	/* zone_check_color(... */

/**
 * @brief  Tests whether the size of an object is in the range or not.
 *  (3D information)
 *
 * @param[in] obj  Pointer to the track object.
 * @param[in] z  Pointer to the zone.
 *
 * @return
 *  - 0 if size of the object is not in the range.
 *  - 1 if size of the object is in the range.
 */
static ivca_s32_t
zone_check_size(ivcam_track_obj_t *obj, ivca_zone_t *z)
{
	ivca_meta_obj_t *mobj = &obj->mobj;
	ivca_u32_t gw, gh;

	/* Convert to mm, Q.0. */
	gw = (mobj->width3d * 1000) >> 8;
	gh = (mobj->height3d * 1000) >> 8;
	gw = min(gw, IVCA_WIDTH3D_MAX);
	gh = min(gh, IVCA_HEIGHT3D_MAX);

	return gw >= z->size_min[0] && gw <= z->size_max[0] &&
			gh >= z->size_min[1] && gh <= z->size_max[1];
}	/* zone_check_size(... */

/**
 * @brief  Tests whether the speed of an object is in the range or not.
 *  (3D information)
 *
 * @param[in] obj  Pointer to the track object.
 * @param[in] z  Pointer to the zone.
 *
 * @return
 *  - 0 if speed of the object is not in the range.
 *  - 1 if speed of the object is in the range.
 */
static ivca_s32_t
zone_check_speed(ivcam_track_obj_t *obj, ivca_zone_t *z)
{
	ivca_meta_obj_t *mobj = &obj->mobj;
	ivca_u32_t gs;

	/* Convert to Q.0. */
	gs = mobj->speed3d >> 8;

	return gs >= z->speed_min && gs <= z->speed_max;
}	/* zone_check_speed(... */


static void
set_event(ivca_rule_event_t *event, ivca_u32_t type,
		ivca_s32_t oid, ivca_s16_t zid, ivca_u08_t oclass,
		ivca_u08_t ch, ivca_rect_t *rect)
{
	event->type = type;
	event->object_id = oid;
	event->rule_id = zid;
	event->object_class = oclass;
	event->ch = ch;
	if ( rect )
		event->rc = *rect;
	else
		event->rc.x = event->rc.y = event->rc.w = event->rc.h = 0;
	event->timestamp = 0;
	event->snap_size = 0;
	event->snapshot = NULL;
}	/* set_event(... */



ivca_s32_t
ai_ivcam_rule_detect(ai_ivcam_tracker_t *tracker, ivca_rule_t *rules,
		ivca_s32_t nmaxevents, ai_rule_event_t *eventlist,
		ai_meta_hdr_t *pMTH, ivca_s08_t* object_filter)
{
	ivca_s32_t ce = 0, i, ecount[IVCA_MAX_ZONES];
	ivca_u32_t type;//, cnt_debounce = 2 + (pMTH->framerate > 15);
	ai_ivcam_track_obj_t *obj;
	ivca_zone_t *z;
	ai_ivcam_zone_state_t *zs;

	ivca_s08_t filter[256];

	ivca_s32_t static_flag =0;

	/* Check objects currently being tracked. */
	memset(ecount, 0, sizeof(ecount));
	list_for_each_entry(obj, &tracker->tlist, ai_ivcam_track_obj_t, list) {
			
		/* Check for all of zones. */
		for (i = 0, z = rules->zonelist, zs = obj->zstate;
				i < rules->nzones; i++, z++, zs++) {
			if ( !z->active )
				continue;

			/* object filter */
			if(z->all_detect_obj == 0){
				memcpy(filter, object_filter+(256*i),256 );
				if(!(strstr(filter,obj->mobj.object_class)))
							continue;	
			}

			/* Method to check is dependent to the type of each zone. */
			type = 0;
			if ( z->type == IVCA_RT_LINE ) {
				ai_zone_check_line(obj, &rules->zonelist[i], i);
				if ( zs->state &&
					!obj->mobj.isstatic &&
					obj->match) {
					type = z->enabled & zs->state;
					zs->state = 0;
				}
			}
			else if ( z->type == IVCA_RT_AREA ) {

#if 0
				/* Check by crossing for enter & exit events. */
				if ( z->enabled & (IVCA_ET_ENTER | IVCA_ET_EXIT) )
					ai_zone_check_area_by_crossing(obj, z, i);
				
				if ( ai_zone_check_area_by_presence(obj, z) ) {
					zs->cnt_off = 0;
					zs->cnt_on++;
					if ( (zs->state & IVCA_ET_ENTER)
						&& obj->match) {
						type = z->enabled & IVCA_ET_ENTER;
						zs->state &= ~IVCA_ET_ENTER;
					}
					//printf("obj ID %d timestamp %d loitering_start %d loiter_time %d zs->state %d \n",obj->mobj.id, pMTH->timestamp,zs->loitering_start,z->loiter_time,zs->state);
					if ( !(zs->state & IVCA_ET_STOPPED) &&
							obj->match &&
							(zs->stopped_start >0) ) {
						ivca_u16_t cidx = (obj->t_tail + NR_REFP_HIST - 1) % NR_REFP_HIST;
							
						if(abs(zs->stopped_start_x - obj->traj[cidx].x) >  (obj->rect.w)/30 ||
							abs(zs->stopped_start_y - obj->traj[cidx].y) > (obj->rect.h)/30){
							zs->stopped_start = 0;
						}
						else{
							if(pMTH->timestamp - zs->stopped_start >= z->stop_time){
								type = z->enabled & IVCA_ET_STOPPED;
								zs->state |= IVCA_ET_STOPPED;
							}
						}
					}
					if ( !(zs->state & IVCA_ET_LOITERED) &&
							!(zs->state & IVCA_ET_STOPPED) &&
							obj->match &&
							(zs->loitering_start >0) &&
							pMTH->timestamp - zs->loitering_start >= z->loiter_time){
						type = z->enabled & IVCA_ET_LOITERED;
						zs->state |= IVCA_ET_LOITERED;
						//printf("LOITER detect type %d zs->state %d  nmaxevents %d\n",type,zs->state,nmaxevents);
					}
					if ( !(zs->state & IVCA_ET_REMOVED) &&
							obj->static_del_time >0 && obj->static_object &&
							pMTH->timestamp - obj->static_del_time >= z->remove_time ) {
						type = z->enabled & IVCA_ET_REMOVED;
						zs->state |= IVCA_ET_REMOVED;
						obj->removed = 1;
						obj->static_del_time = pMTH->timestamp - 55;
					}
					if(zs->loitering_start == 0)
						zs->loitering_start = pMTH->timestamp;
					if(zs->stopped_start == 0){
						ivca_u16_t cidx = (obj->t_tail + NR_REFP_HIST - 1) % NR_REFP_HIST;
						
						zs->stopped_start = pMTH->timestamp;
						zs->stopped_start_x = obj->traj[cidx].x;
						zs->stopped_start_y = obj->traj[cidx].y;
					}
				}
				else {
					zs->cnt_on = 0;
					zs->loitering_start = 0;
					zs->stopped_start = 0;
					if ( (zs->state & IVCA_ET_EXIT)) {
						type = z->enabled & IVCA_ET_EXIT;
						zs->state =0;
					}
					else if ( zs->state)
						zs->state =0;
				}
#else
			
			/* Check by crossing for enter & exit events. */
			if ( z->enabled & (IVCA_ET_ENTER | IVCA_ET_EXIT) )
				ai_zone_check_area_by_crossing(obj, z, i);
				
			if ( ai_zone_check_area_by_presence(obj, z) ) {
					zs->cnt_off = 0;
					zs->cnt_on++;

					if ( (zs->state & IVCA_ET_ENTER)
						&& obj->match) {
						type |= z->enabled & IVCA_ET_ENTER;
						zs->state &= ~IVCA_ET_ENTER;
					}
					
					if ( !(zs->state & IVCA_ET_INTRUSION)
						&& !obj->mobj.isstatic
						&& obj->match) {
						type |= z->enabled & IVCA_ET_INTRUSION;
						zs->state |= IVCA_ET_INTRUSION;
					}
					//printf("obj ID %d timestamp %d loitering_start %d loiter_time %d zs->state %d \n",obj->mobj.id, pMTH->timestamp,zs->loitering_start,z->loiter_time,zs->state);
					if ( !(zs->state & IVCA_ET_STOPPED) &&
							z->enabled & IVCA_ET_STOPPED &&
							obj->match &&
							!obj->mobj.isstatic &&
							(zs->stopped_start >0) ) {
						ivca_u16_t cidx = (obj->t_tail + NR_REFP_HIST - 1) % NR_REFP_HIST;
							
						if(abs(zs->stopped_start_x - obj->traj[cidx].x) >  (obj->rect.w)/20 ||
							abs(zs->stopped_start_y - obj->traj[cidx].y) > (obj->rect.h)/20){
							zs->stopped_start = 0;
						}
						else{
							if(pMTH->timestamp - zs->stopped_start >= z->stop_time){
								type |= z->enabled & IVCA_ET_STOPPED;
								zs->state |= IVCA_ET_STOPPED;
							}
						}
					}
					if ( !(zs->state & IVCA_ET_LOITERED) &&
							!(zs->state & IVCA_ET_STOPPED) &&
							(z->enabled & IVCA_ET_LOITERED) &&
							obj->match &&
							!obj->mobj.isstatic &&
							(zs->loitering_start >0) &&
							pMTH->timestamp - zs->loitering_start >= z->loiter_time){
						type |= z->enabled & IVCA_ET_LOITERED;
						zs->state |= IVCA_ET_LOITERED;
						//printf("LOITER detect type %d zs->state %d  nmaxevents %d\n",type,zs->state,nmaxevents);
					}
					if ( !(zs->state & IVCA_ET_REMOVED) &&
							(z->enabled & IVCA_ET_REMOVED) &&
							obj->static_del_time >0 && obj->static_object &&
							pMTH->timestamp - obj->static_del_time >= z->remove_time ) {
						type |= z->enabled & IVCA_ET_REMOVED;
						zs->state |= IVCA_ET_REMOVED;
						obj->removed = 1;
						obj->static_del_time = pMTH->timestamp - 55;
						obj->mobj.isstatic = 0;
					}
					if(zs->loitering_start == 0)
						zs->loitering_start = pMTH->timestamp;
					if(zs->stopped_start == 0){
						ivca_u16_t cidx = (obj->t_tail + NR_REFP_HIST - 1) % NR_REFP_HIST;
						
						zs->stopped_start = pMTH->timestamp;
						zs->stopped_start_x = obj->traj[cidx].x;
						zs->stopped_start_y = obj->traj[cidx].y;
					}
				}
				else {
					zs->cnt_on = 0;
					zs->loitering_start = 0;
					zs->stopped_start = 0;
					
					if ( (zs->state & IVCA_ET_EXIT)) {
						type |= z->enabled & IVCA_ET_EXIT;
						zs->state =0;
					}
					else if ( zs->state)
						zs->state =0;
				}

#endif
			}

			if ( !type )
				continue;
			#if 0
			/* Check filters. */
			/* Color. */
			if ( z->enabled & IVCA_ET_COLOR ) {
				if ( !obj->hvalid || !zone_check_color(obj->hdata, z) )
					type = 0;
			}
			#endif
			/* Size. */
			if ( (z->enabled & IVCA_ET_SIZE)) {
				if ( !ai_zone_check_size(obj, z) )
					type = 0;
			}
			/* Class. */
			if ( z->enabled & IVCA_ET_CLASS ) {
				//TODO
			}
			/* Speed. */
			if ( (z->enabled & IVCA_ET_SPEED)) {
				if ( !ai_zone_check_speed(obj, z) )
					type = 0;
			}
			/* confidence */
			if ((ivca_u32_t)(obj->mobj.confidence*100) < z->c_threshold) {
				type = 0;
			}	

			if ( type ) {
				if ( obj->zevent_count < 32767 )
					obj->zevent_count++;
				if ( ce < nmaxevents ){
					ai_rule_event_t *event;
					ivca_u32_t check_rules[8] = {IVCA_ET_DIR_POS,IVCA_ET_DIR_NEG,IVCA_ET_ENTER,IVCA_ET_EXIT,IVCA_ET_STOPPED,IVCA_ET_REMOVED,IVCA_ET_LOITERED,IVCA_ET_INTRUSION};
					ivca_s32_t k;
					
					for(k =0; k < 8; k++){
						if(!(type & check_rules[k]))
							continue;
						if( ce >= nmaxevents )
							break;						
						
						event = &eventlist[ce++];
						event->type = check_rules[k];
						event->object_id = obj->mobj.id;
						event->rule_id = z->id;
						memcpy(event->object_class ,obj->mobj.object_class,64);
						memcpy(event->topic ,pMTH->topic,64);
						event->ch = tracker->id;
						event->bbx_position[0]=obj->mobj.bbx_position[0];
						event->bbx_position[1]=obj->mobj.bbx_position[1];
						event->bbx_position[2]=obj->mobj.bbx_position[2];
						event->bbx_position[3]=obj->mobj.bbx_position[3];
						event->timestamp = pMTH->timestamp;
						event->timestampl = pMTH->timestampl;
						event->process_time = pMTH->process_time;
						event->confidence = obj->mobj.confidence;
						
						ecount[i]++;
					}				
				}
			}
		}
	}
#if 0
	/* Check counters. */
	for (i = 0, c = rules->cntrlist; i < rules->ncntrs; i++, c++) {
		if ( !c->active )
			continue;
		oldcval = c->value;

		/* Count. */
		for (j = 0, z = rules->zonelist; j < rules->nzones; j++, z++) {
			if ( !z->active || !ecount[j] )
				continue;
			if ( z->id == c->zid_up )
				c->value += ecount[j];
			if ( z->id == c->zid_dn )
				c->value -= ecount[j];
		}

		/* Check event. */
		if ( (c->enabled & IVCA_ET_COUNTER) &&
				((oldcval < c->evalue && c->value >= c->evalue) ||
				(oldcval > c->evalue && c->value <= c->evalue)) &&
				ce < nmaxevents ) {
			ai_rule_event_t *event;

			event = &eventlist[ce++];
			event->type = IVCA_ET_COUNTER;
			event->object_id = -1;
			event->rule_id = c->id;
			memset(event->object_class,0x00,64);
			memset(event->topic,0x00,64);
			event->ch = tracker->id;
			event->bbx_position[0]=0;
			event->bbx_position[1]=0;
			event->bbx_position[2]=0;
			event->bbx_position[3]=0;
			event->timestamp = pMTH->timestamp;
			event->timestampl = pMTH->timestampl;
			event->process_time = pMTH->process_time;
			event->confidence = 0;
					
			if ( c->resetalert )
				c->value = 0;		/* Reset counter after alert. */
		}
	}
#endif
	return ce;
}	/* ivcam_rule_detect(... */



/**
 * @brief  Detects rule events for meta data.
 *
 * @param[in] tracker  Pointer to the meta data tracker.
 * @param[in] rules  Pointer to the rule for detection.
 * @param[in] nmaxevents  Maximum number of events.
 * @param[out] eventlist  Output buffer for rule event list.
 *  Caller must provide this buffer.
 * @param[in] pMTH  Pointer to the meta data header.
 *
 * @return  Non negative detected number of events. 0 if there is no event.
 *  If detected > nmaxevents, this function will return nmaxevents.
 */
ivca_s32_t
ivcam_rule_detect(ivcam_tracker_t *tracker, ivca_rule_t *rules,
		ivca_s32_t nmaxevents, ivca_rule_event_t *eventlist,
		ivca_meta_hdr_t *pMTH)
{
	ivca_s32_t ce = 0, i, j, ecount[IVCA_MAX_ZONES], oldcval;
	ivca_u32_t type, cnt_debounce = 2 + (pMTH->framerate > 15);
	ivcam_track_obj_t *obj;
	ivca_zone_t *z;
	ivca_cntr_t *c;
	ivcam_zone_state_t *zs;

	/* Check objects currently being tracked. */
	memset(ecount, 0, sizeof(ecount));
	list_for_each_entry(obj, &tracker->tlist, ivcam_track_obj_t, list) {
		/* Check for all of zones. */
		for (i = 0, z = rules->zonelist, zs = obj->zstate;
				i < rules->nzones; i++, z++, zs++) {
			if ( !z->active )
				continue;

			/* Method to check is dependent to the type of each zone. */
			type = 0;
			if ( z->type == IVCA_RT_LINE ) {
				zone_check_line(obj, &rules->zonelist[i], i);
				if ( zs->state && ++(zs->cnt_on) == cnt_debounce ) {
					type = z->enabled & zs->state;
					zs->state = 0;
				}
			}
			else if ( z->type == IVCA_RT_AREA ) {
				if ( zone_check_area(obj, z) ) {
					zs->cnt_off = 0;
					zs->cnt_on++;
					if ( !(zs->state & IVCA_ET_ENTER) &&
							zs->cnt_on == cnt_debounce ) {
						type = z->enabled & IVCA_ET_ENTER;
						zs->state |= IVCA_ET_ENTER;
					}
					else if ( obj->mobj.isstatic != IVCA_SR_REMOVED &&
							!(zs->state & IVCA_ET_LOITERED) &&
							zs->cnt_on >= z->loiter_time * pMTH->framerate ) {
						type = z->enabled & IVCA_ET_LOITERED;
						zs->state |= IVCA_ET_LOITERED;
					}
					else if ( obj->mobj.isstatic == IVCA_SR_STOPPED &&
							!(zs->state & IVCA_ET_STOPPED) &&
							zs->cnt_on >= z->stop_time * pMTH->framerate ) {
						type = z->enabled & IVCA_ET_STOPPED;
						zs->state |= IVCA_ET_STOPPED;
					}
					else if ( obj->mobj.isstatic == IVCA_SR_REMOVED &&
							!(zs->state & IVCA_ET_REMOVED) &&
							zs->cnt_on >= z->remove_time * pMTH->framerate ) {
						type = z->enabled & IVCA_ET_REMOVED;
						zs->state |= IVCA_ET_REMOVED;
					}
				}
				else {
					zs->cnt_on = 0;
					if ( zs->state && ++(zs->cnt_off) == cnt_debounce ) {
						type = z->enabled & IVCA_ET_EXIT;
						zs->state = 0;
					}
				}
			}

			if ( !type )
				continue;
			/* Check filters. */
			/* Color. */
			if ( z->enabled & IVCA_ET_COLOR ) {
				if ( !obj->hvalid || !zone_check_color(obj->hdata, z) )
					type = 0;
			}
			/* Size. */
			if ( (z->enabled & IVCA_ET_SIZE) && obj->mobj.valid3d ) {
				if ( !zone_check_size(obj, z) )
					type = 0;
			}
			/* Class. */
			if ( z->enabled & IVCA_ET_CLASS ) {
				//TODO
			}
			/* Speed. */
			if ( (z->enabled & IVCA_ET_SPEED) && obj->mobj.valid3d ) {
				if ( !zone_check_speed(obj, z) )
					type = 0;
			}

			if ( type ) {
				if ( obj->zevent_count < 32767 )
					obj->zevent_count++;
				ecount[i]++;
				if ( ce < nmaxevents )
					set_event(&eventlist[ce++], type, obj->mobj.id, z->id,
							IVCA_CLASS_UNKNOWN, tracker->id, &obj->mobj.rc);
			}
		}
	}

	/* Check counters. */
	for (i = 0, c = rules->cntrlist; i < rules->ncntrs; i++, c++) {
		if ( !c->active )
			continue;
		oldcval = c->value;

		/* Count. */
		for (j = 0, z = rules->zonelist; j < rules->nzones; j++, z++) {
			if ( !z->active || !ecount[j] )
				continue;
			if ( z->id == c->zid_up )
				c->value += ecount[j];
			if ( z->id == c->zid_dn )
				c->value -= ecount[j];
		}

		/* Check event. */
		if ( (c->enabled & IVCA_ET_COUNTER) &&
				((oldcval < c->evalue && c->value >= c->evalue) ||
				(oldcval > c->evalue && c->value <= c->evalue)) &&
				ce < nmaxevents ) {
			set_event(&eventlist[ce++], IVCA_ET_COUNTER, -1, c->id,
					IVCA_CLASS_NA, tracker->id, NULL);
			if ( c->resetalert )
				c->value = 0;		/* Reset counter after alert. */
		}
	}

	return ce;
}	/* ivcam_rule_detect(... */

