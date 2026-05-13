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
2012/07/24 Jongbin Yim    Created.

................................................................................
DESCRIPTION:

................................................................................
*/

/**
 * @file  vw_vca_internal.c
 * @brief  This file contains implementation of commonly used functions for
 *  VCA & smart search.
 */

/*******************************************************************************
 * includes                                                                    *
 ******************************************************************************/

#include "nf_afx.h"
#include "support/event_loop.h"
#include "support/nf_ui_color.h"
#include "support/nf_ui_image.h"
#include "tools/nf_ui_tool.h"
#include "viewers/objects/nfbutton.h"
#include "viewers/objects/nfcheckbutton.h"
#include "viewers/objects/nfcombobox.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfobject.h"

#include "nf_meta_data.h"
#include "vw_vca_internal.h"

#include <math.h>

//captainnn
#include "ivca_def.h"
#include "libivcam.h"
#include "libicalib.h"
#include "nf_api_ipcam.h"

/*******************************************************************************
 * defines                                                                     *
 ******************************************************************************/
#define	N_COLORS		28

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

/**
 * @brief  Tests whether a point (x, y) is near line or not.
 *
 * @param[in] x  x coordinate of the test point.
 * @param[in] y  y coordinate of the test point.
 * @param[in] d  Tolerance. (distance)
 * @param[in] pt1  The first point of the line.
 * @param[in] pt2  The second point of the line.
 *
 * @return
 *  - @c 1 if the point is closer than d.
 *  - @c 0 otherwise.
 */
gint
vw_vca_line_test(gint x, gint y, gint d, ivca_point_t *pt1, ivca_point_t *pt2)
{
	gint l[3];
	long long d2;

	if ( ((pt1->x - d <= x && x <= pt2->x + d) ||
			(pt2->x - d <= x && x <= pt1->x + d)) &&
			((pt1->y - d <= y && y <= pt2->y + d) ||
			(pt2->y - d <= y && y <= pt1->y + d)) ) {
		/* Compute line. */
		l[0] = pt1->y - pt2->y;
		l[1] = pt2->x - pt1->x;
		l[2] = pt1->x * pt2->y - pt2->x * pt1->y;
		/* Compute distance. */
		d2 = (long long)(l[0] * x + l[1] * y + l[2]) *
				(l[0] * x + l[1] * y + l[2]) / (l[0] * l[0] + l[1] * l[1]);

		if ( d2 <= d * d )	/* Square of distance. */
			return 1;
	}
	return 0;
}	/* vw_vca_line_test(... */

/**
 * @brief  Tests whether a point (x, y) is inside polygon or not.
 *
 * @param[in] x  x coordinate of the test point.
 * @param[in] y  y coordinate of the test point.
 * @param[in] d  Tolerance. (distance)
 * @param[in] npts  Number of points in a polygon.
 * @param[in] pt  Points of the polygon. (length = npts)
 *
 * @return
 *  - @c 0 if the point is exterior.
 *  - @c 1 if the point is interior.
 */
 
//static guint8 color8[3] = {0x00, 0x80, 0xFF};
static GdkColor color_template[N_COLORS] = {
	{0, 0xFF00, 0x0000, 0x0000},
	{0, 0xFF00, 0x3F00, 0x0000},
	{0, 0xFF00, 0x7F00, 0x0000},
	{0, 0xFF00, 0xBF00, 0x0000},
	{0, 0xFF00, 0xFF00, 0x0000},
	{0, 0xBF00, 0xFF00, 0x0000},
	{0, 0x2000, 0x2000, 0x2000},
	
	{0, 0x7F00, 0xFF00, 0x0000},
	{0, 0x3F00, 0xFF00, 0x0000},
	{0, 0x0000, 0xFF00, 0x0000},
	{0, 0x0000, 0xFF00, 0x3F00},
	{0, 0x0000, 0xFF00, 0x7F00},
	{0, 0x0000, 0xFF00, 0xBF00},
	{0, 0x6000, 0x6000, 0x6000},
	
	{0, 0x0000, 0xFF00, 0xFF00},
	{0, 0x0000, 0xBF00, 0xFF00},
	{0, 0x0000, 0x7F00, 0xFF00},
	{0, 0x0000, 0x3F00, 0xFF00},
	{0, 0x0000, 0x0000, 0xFF00},
	{0, 0x3F00, 0x0000, 0xFF00},
	{0, 0xa000, 0xa000, 0xa000},
	
	{0, 0x7F00, 0x0000, 0xFF00},
	{0, 0xBF00, 0x0000, 0xFF00},
	{0, 0xFF00, 0x0000, 0xFF00},
	{0, 0xFF00, 0x0000, 0xBF00},
	{0, 0xFF00, 0x0000, 0x7F00},
	{0, 0xFF00, 0x0000, 0x3F00},
	{0, 0xFF00, 0xFF00, 0xFF00}
};



gint
vw_vca_polygon_test(gint x, gint y, gint d, gint npts, ivca_point_t *pt)
{
	gint i, j, left = 0, dotp;

	for (i = 0, j = npts - 1; i < npts; j = i, i++) {
		if ( (pt[i].y < y && y <= pt[j].y) || (pt[j].y < y && y <= pt[i].y) ) {
			/* Check the sign of dot(l, x). */
			dotp = (pt[j].y - pt[i].y) * x + (pt[i].x - pt[j].x) * y +
					pt[j].x * pt[i].y - pt[i].x * pt[j].y;
			if ( pt[j].y < pt[i].y )
				dotp = -dotp;
			if ( dotp < 0 )
				left++;
		}
	}
	if ( d ) {
		for (i = 0, j = npts - 1; i < npts; j = i, i++) {
			if ( vw_vca_line_test(x, y, d, &pt[j], &pt[i]) ) {
				left = 1;
				break;
			}
		}
	}
	return left & 1;	/* Odd count means (x, y) is interior point. */
}	/* vw_vca_polygon_test(... */

static ivca_point_t arrow[7] = {
		{-4,-15}, {-4, 6}, {-9, 6}, {0, 16}, {10, 6}, {5, 6}, {5, -15}
};

static void
_draw_line_dir(NFOBJECT *obj, GdkDrawable *drawable, GdkGC *gc,
		ivca_point_t pt1, ivca_point_t pt2, gboolean fill,
		gint rx, gint ry, gint ofs_x, gint ofs_y)
{
	gint mptx = ((pt1.x + pt2.x) *obj->width /(1920*2)) >> 1;
	gint mpty = ((pt1.y + pt2.y) *obj->height /(1080*2)) >> 1;
	gint dx = (pt2.x - pt1.x)*obj->width /(1920*2), dy = (pt2.y - pt1.y) *obj->height /(1080*2);
	gint x, y, i;
	float l = sqrtf((float)(dx * dx + dy * dy));
	GdkPoint arr[7];

	for (i = 0; i < 7; i++) {
		x = arrow[i].x;
		y = arrow[i].y;

		/* Rotate and shift arrow */
		arr[i].x = (gint)((float)((x * dx - y * dy) *(1920*2) /obj->width * 4 /
				((1920*2) /obj->width +  (1080*2)/obj->height)) / l) + mptx;
		arr[i].y = (gint)((float)((y * dx + x * dy) * (1080*2)/obj->height * 4 /
				((1920*2) /obj->width +  (1080*2)/obj->height)) / l) + mpty;

		x = MIN(arr[i].x, obj->width - 5);
		y = MIN(arr[i].y, obj->height - 5);
		arr[i].x = MAX(x, 5) + ofs_x;
		arr[i].y = MAX(y, 5) + ofs_y;
	}

	gdk_draw_polygon(drawable, gc, fill, arr, 7);
}	/* _draw_line_dir(... */

/**
 * @brief  Draws zones.
 */
void
vw_vca_draw_str(NFOBJECT *obj, GdkDrawable *drawable, GdkGC *gc,
		gint erase, gchar *str)
{
	gint i, k, n, x, y, ofs_x, ofs_y, issel;
	gchar *font = nffont_get_pango_font(NFFONT_MEDIUM_NORMAL);
	gchar sbuf[64];
	gint w,h;
	GdkColor *pcolor  = &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_FF0000));

	//if ( t->ntargets > 0 ) {
	nfui_nfobject_get_offset(obj, &ofs_x, &ofs_y);

	strcpy(sbuf, str);
	w = (gint)nfutil_string_width(0, drawable, font, sbuf, 0);
	h = (gint)nfutil_string_height(drawable, font, sbuf, 0);
	x = 0;
	y = 0;
	x = MIN(x, obj->width - 3 - w);
	x = MAX(x, 3) + ofs_x;
	y = MIN(y - h, obj->height - 3 - h);
	y = MAX(y, 3) + ofs_y;
	if ( erase )
		gdk_draw_rectangle(drawable, gc, TRUE, x, y, w, h);
	else
		nfutil_draw_text_with_pango(NULL, NULL, NULL, drawable, gc,
				sbuf, x, y, w, h, font, pcolor, NFALIGN_LEFT, 0);
}	/* vw_vca_draw_zones(... */


/**
 * @brief  Draws zones.
 */

static GdkPoint _rotate_point(GdkPoint point, GdkPoint datum_point, gdouble cost, gdouble sint)
{
    GdkPoint returned;


    returned.x = (int)ceil(((point.x - datum_point.x) * cost) +  ((point.y - datum_point.y) * sint) + datum_point.x);
    returned.y = (int)ceil(((datum_point.x - point.x) * sint) +  ((point.y - datum_point.y) * cost) + datum_point.y);

    return returned;
}

void vw_vca_draw_target( NFOBJECT *obj, GdkDrawable *drawable, GdkGC *gc,
                         DRAW_TYPE draw_type, ivca_calib_target_t *target)
{
	gint x, y, ofs_x, ofs_y, issel;
	static GdkPoint points[IVCA_MAX_PTSPERZONE];	/* Display coordinate. */
	GdkColor color = {0, 0, 0, 0};
	gchar *font = nffont_get_pango_font(NFFONT_MINI_SEMI_5);
	gchar sbuf[64];

    // draw person
    gfloat distance;
    gfloat diameter;
    GdkPoint person_points[IVCA_MAX_PTSPERZONE];	/* Display coordinate. */
    GdkPoint rotate_points[IVCA_MAX_PTSPERZONE];	/* Display coordinate. */
    GdkPoint center;	/* Display coordinate. */
    int i;

	//if ( t->ntargets > 0 ) {
	nfui_nfobject_get_offset(obj, &ofs_x, &ofs_y);
	//}
	//for (k = 0; k < t->ntargets; k++) {
		//c = &t->targetlist[k];
		issel =1;

        //draw_target
        switch(draw_type)
        {
            case DRAW_TARGET:
                {
                    // nomal target
                    color.red = (guint16)0x80<<8;
                    color.green = (guint16)0xff<<8;
                    color.blue = (guint16)0xff<<8;
                }
                break;
            case SELECT_TARGET:
                {
                    // select target
                    color.red = (guint16)0xff<<8;
                    color.green = (guint16)0x00<<8;
                    color.blue = (guint16)0x80<<8;
                }
                break;
            default:
                break;
        }


		gdk_gc_set_rgb_fg_color(gc, &color);
		//gdk_gc_set_line_attributes(gc, issel ? 6 : 3,
				//GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);

        /* Convert coordinate. */
		points[0].x = target->pt[0].x + ofs_x;
		points[0].y = target->pt[0].y + ofs_y;
		points[1].x = target->pt[1].x + ofs_x;
		points[1].y = target->pt[1].y + ofs_y;

		/* Draw line segments. */
		//gdk_draw_line(drawable, gc, points[0].x,
		//		points[0].y, points[1].x, points[1].y);

		/* Draw a person*/
        distance = sqrt(pow(points[0].x - points[1].x, 2) + pow(points[0].y - points[1].y, 2));
        diameter = distance * 0.15;

        center.x = points[0].x;
        center.y = points[0].y + (diameter / 2);

        center = _rotate_point(center, points[0], (points[1].y - points[0].y) / distance, (points[1].x - points[0].x) / distance);

        gdk_draw_arc(drawable, gc, TRUE, center.x  - (diameter / 2), center.y  - (diameter / 2), diameter, diameter, 0, 64 * 360);

        person_points[0].x = points[0].x - distance * 0.13; 
        person_points[0].y = points[0].y + distance * 0.19; 

        person_points[1].x = points[0].x - (diameter / 2); 
        person_points[1].y = points[0].y + diameter; 

        person_points[2].x = points[0].x; 
        person_points[2].y = points[0].y + (distance / 4); 

        person_points[3].x = points[0].x + (diameter / 2); 
        person_points[3].y = points[0].y + diameter; 

        person_points[4].x = points[0].x + distance * 0.13; 
        person_points[4].y = points[0].y + distance * 0.19; 

        person_points[5].x = points[0].x + distance * 0.12; 
        person_points[5].y = points[0].y + distance * 0.52; 

        person_points[6].x = points[0].x + distance * 0.094; 
        person_points[6].y = points[0].y + distance * 0.57; 

        person_points[7].x = points[0].x + distance * 0.036; 
        person_points[7].y = points[0].y + distance; 

        person_points[8].x = points[0].x - distance * 0.036; 
        person_points[8].y = points[0].y + distance; 

        person_points[9].x = points[0].x - distance * 0.094; 
        person_points[9].y = points[0].y + distance * 0.57; 

        person_points[10].x = points[0].x - distance * 0.12; 
        person_points[10].y = points[0].y + distance * 0.52; 


        /* angle */
        for (i = 0 ; i < 11; i++)
            rotate_points[i] = _rotate_point(person_points[i], points[0], (points[1].y - points[0].y) / distance, (points[1].x - points[0].x) / distance);

        gdk_draw_polygon(drawable, gc, TRUE, rotate_points, 11);

		/* Draw points. */
		if ( issel ) {
			x = points[0].x;
			y = points[0].y;
			gdk_gc_set_line_attributes(gc, 2,
					GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
			gdk_gc_set_rgb_fg_color(gc, &color);
			gdk_draw_rectangle(drawable, gc, TRUE, x - 6, y - 6, 12, 12);
			if (draw_type == 2 ) {
				gdk_gc_set_line_attributes(gc, 2,
						GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
				if ( !draw_type )
					gdk_gc_set_rgb_fg_color(gc,
							&UX_COLOR(COLOR_PRG_IDX(UX_COLOR_FFFFFF)));
				gdk_draw_rectangle(drawable, gc, FALSE,
						x - 6, y - 6, 12, 12);
			}
		}

        /* Draw line */
        /*
        if(draw_type == SELECT_TARGET)
        {
            color.red = (guint16)0x80<<8;
            color.green = (guint16)0xff<<8;
            color.blue = (guint16)0xff<<8;

            gdk_gc_set_rgb_fg_color(gc, &color);
            gdk_gc_set_line_attributes(gc, issel ? 3 : 3,
				GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);

            gdk_draw_line(drawable, gc, points[0].x,
                    points[0].y, points[1].x, points[1].y);
        }
        */


	//}
}	/* vw_vca_draw_target(... */



/**
 * @brief  Draws zones.
 */
void
vw_vca_draw_targets(NFOBJECT *obj, GdkDrawable *drawable, GdkGC *gc,
		gint erase, ivca_calib_t *t,
		gint rx, gint ry,gint cidx,gint cidx_p)
{
	gint i, k, n, x, y, ofs_x, ofs_y, issel;
	ivca_calib_target_t *c;
	static GdkPoint points[IVCA_MAX_PTSPERZONE];	/* Display coordinate. */
	GdkColor color = {0, 0, 0, 0};
	gchar *font = nffont_get_pango_font(NFFONT_MINI_SEMI_5);
	gchar sbuf[64];

    // draw person
    gfloat distance;
    gfloat diameter;
    GdkPoint person_points[IVCA_MAX_PTSPERZONE];	/* Display coordinate. */
    GdkPoint rotate_points[IVCA_MAX_PTSPERZONE];	/* Display coordinate. */
    GdkPoint center;	/* Display coordinate. */

	//if ( t->ntargets > 0 ) {
	nfui_nfobject_get_offset(obj, &ofs_x, &ofs_y);
	//}
	for (k = 0; k < t->ntargets; k++) {
		c = &t->targetlist[k];
		issel =1;
		if ( !erase ) {
			color.red = (guint16)0x80<<8;
			color.green = (guint16)0xff<<8;
			color.blue = (guint16)0xff<<8;
		}
		if(!erase && k == cidx){
			color.red = (guint16)0xff<<8;
			color.green = (guint16)0x00<<8;
			color.blue = (guint16)0x80<<8;
		}
		gdk_gc_set_rgb_fg_color(gc, &color);
		gdk_gc_set_line_attributes(gc, issel ? 6 : 3,
				GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);

		/* Convert coordinate. */
		points[0].x = c->pt[0].x *obj->width /(1920*2) + ofs_x;
		points[0].y = c->pt[0].y *obj->height /(1080*2) + ofs_y;
		points[1].x = c->pt[1].x *obj->width /(1920*2) + ofs_x;
		points[1].y = c->pt[1].y *obj->height /(1080*2) + ofs_y;

		/* Draw line segments. */
		//gdk_draw_line(drawable, gc, points[0].x,
		//		points[0].y, points[1].x, points[1].y);

		/* Draw a person*/
        distance = sqrt(pow(points[0].x - points[1].x, 2) + pow(points[0].y - points[1].y, 2));
        diameter = distance * 0.15;

        center.x = points[0].x;
        center.y = points[0].y + (diameter / 2);

        center = _rotate_point(center, points[0], (points[1].y - points[0].y) / distance, (points[1].x - points[0].x) / distance);

        gdk_draw_arc(drawable, gc, TRUE, center.x  - (diameter / 2), center.y  - (diameter / 2), diameter, diameter, 0, 64 * 360);

        person_points[0].x = points[0].x - distance * 0.13; 
        person_points[0].y = points[0].y + distance * 0.19; 

        person_points[1].x = points[0].x - (diameter / 2); 
        person_points[1].y = points[0].y + diameter; 

        person_points[2].x = points[0].x; 
        person_points[2].y = points[0].y + (distance / 4); 

        person_points[3].x = points[0].x + (diameter / 2); 
        person_points[3].y = points[0].y + diameter; 

        person_points[4].x = points[0].x + distance * 0.13; 
        person_points[4].y = points[0].y + distance * 0.19; 

        person_points[5].x = points[0].x + distance * 0.12; 
        person_points[5].y = points[0].y + distance * 0.52; 

        person_points[6].x = points[0].x + distance * 0.094; 
        person_points[6].y = points[0].y + distance * 0.57; 

        person_points[7].x = points[0].x + distance * 0.036; 
        person_points[7].y = points[0].y + distance; 

        person_points[8].x = points[0].x - distance * 0.036; 
        person_points[8].y = points[0].y + distance; 

        person_points[9].x = points[0].x - distance * 0.094; 
        person_points[9].y = points[0].y + distance * 0.57; 

        person_points[10].x = points[0].x - distance * 0.12; 
        person_points[10].y = points[0].y + distance * 0.52; 


        /* angle */
        for (i = 0 ; i < 11; i++)
            rotate_points[i] = _rotate_point(person_points[i], points[0], (points[1].y - points[0].y) / distance, (points[1].x - points[0].x) / distance);

        gdk_draw_polygon(drawable, gc, TRUE, rotate_points, 11);




		/* Draw points. */
		if ( issel ) {
			x = points[0].x;
			y = points[0].y;
			
			gdk_gc_set_line_attributes(gc, 2, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
			gdk_gc_set_rgb_fg_color(gc, &color);
			gdk_draw_rectangle(drawable, gc, TRUE, x - 6, y - 6, 12, 12);
			
			if (erase) 
			{
				gdk_gc_set_line_attributes(gc, 2, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
			}
			else
			{
				gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_FFFFFF)));
    			gdk_draw_rectangle(drawable, gc, FALSE, x - 6, y - 6, 12, 12);
			}
		}
	}
}	/* vw_vca_draw_zones(... */


/**
 * @brief  Draws zones.
 */
void
vw_vca_draw_zones(NFOBJECT *obj, GdkDrawable *drawable, GdkGC *gc,
		gint ensel, gint erase, gint *emask, ivca_rule_t *rl,
		gint rx, gint ry, gint zidxc, gint zidxp, gint pidxc, gint pidxp)
{
	gint i, k, n, x, y, ofs_x, ofs_y, issel;
	ivca_zone_t *z;
	static GdkPoint points[IVCA_MAX_PTSPERZONE];	/* Display coordinate. */
	GdkColor color = {0, 0, 0, 0};
	gchar *font = nffont_get_pango_font(NFFONT_MEDIUM_SEMI);
	gchar sbuf[64];
	gint ymin,m,w,h;

	//if ( rl->nzones > 0 ) {
	nfui_nfobject_get_offset(obj, &ofs_x, &ofs_y);
	if ( rl->nzones > 0 ) {
		n = (ensel && zidxc >= 0) ? zidxc : -1;
	}
	for (k = 0; k < rl->nzones; k++) {
		if ( ++n >= rl->nzones )
			n = 0;
		if ( erase && !emask[n] )
			continue;
		z = &rl->zonelist[n];
		issel = ensel && (n == zidxc || (erase && n == zidxp));
		if ( !erase ) {
			color.red = (guint16)(z->color[0] << 8);
			color.green = (guint16)(z->color[1] << 8);
			color.blue = (guint16)(z->color[2] << 8);
		}
		gdk_gc_set_rgb_fg_color(gc, &color);
		gdk_gc_set_line_attributes(gc, issel ? 6 : 3,
				GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);

		ymin = (1080*2);
		m = 0;
		/* Convert coordinate. */
		for (i = 0; i < z->npts; i++) {
		//	points[i].x = z->pt[i].x / rx + ofs_x;
		//	points[i].y = z->pt[i].y / ry + ofs_y;
			points[i].x = z->pt[i].x *obj->width /(1920*2) + ofs_x;
			points[i].y = z->pt[i].y *obj->height /(1080*2) + ofs_y;
			if(points[i].y < ymin){
				ymin = points[i].y;
				m = i;
			}
			
		}

		/* Draw line segments. */
		switch ( z->type ) {
			case IVCA_RT_LINE:
				gdk_draw_line(drawable, gc, points[0].x,
						points[0].y, points[1].x, points[1].y);
				_draw_line_dir(obj, drawable, gc, z->pt[0], z->pt[1],
						issel && !(erase && n == zidxc && n != zidxp),
						rx, ry, ofs_x, ofs_y);
				break;
			case IVCA_RT_AREA:
				gdk_draw_polygon(drawable, gc, FALSE, points, z->npts);
				break;
		}

		
		/* Draw name. */
		if (1) {
	
			memset(sbuf, 0x00, sizeof(sbuf));
			sprintf(sbuf, "%s", z->name);
			w = (gint)nfutil_string_width(0, drawable, font, sbuf, 0);
			h = (gint)nfutil_string_height(drawable, font, sbuf, 0);
			x = MIN(points[m].x, obj->width - 3 - w + ofs_x);
			x = MAX(x, ofs_x);
			y = MIN(points[m].y - h -5 , obj->height - 3 - h + ofs_y);
			y = MAX(y, ofs_y);
			if ( erase )
				gdk_draw_rectangle(drawable, gc, TRUE, x, y, w, h);
			else
				nfutil_draw_text_with_pango(NULL, NULL, NULL, drawable, gc,
						sbuf, x, y, w, h, font, &color, NFALIGN_LEFT, 0);
		}

		/* Draw points. */
		if ( issel ) {
			for (i = 0; i < z->npts; i++) {
				x = points[i].x;
				y = points[i].y;
				gdk_gc_set_line_attributes(gc, 2,
						GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
				gdk_gc_set_rgb_fg_color(gc, &color);
				gdk_draw_rectangle(drawable, gc, TRUE, x - 6, y - 6, 12, 12);
				if ( i == pidxc || (erase && i == pidxp) ) {
					gdk_gc_set_line_attributes(gc, 2,
							GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
					if ( !erase )
						gdk_gc_set_rgb_fg_color(gc,
								&UX_COLOR(COLOR_PRG_IDX(UX_COLOR_FFFFFF)));
					gdk_draw_rectangle(drawable, gc, FALSE,
							x - 6, y - 6, 12, 12);
				}
			}
		}
	}

}	/* vw_vca_draw_zones(... */

/**
 * @brief  Draws counters.
 */
void
vw_vca_draw_cntrs(NFOBJECT *obj, GdkDrawable *drawable, GdkGC *gc,
		gint ensel, gint erase, gint *emask, ivca_rule_t *rl,
		gint rx, gint ry, gint cidxc, gint cidxp, gchar *font, gint *cval)
{
	gint i, k, n, x, y, w, h, l, wd, ofs_x, ofs_y, issel;
	ivca_cntr_t *c;
	gchar sbuf[64];
	GdkPoint points[4];
	GdkColor color = {0, 0, 0, 0};

	

	if ( rl->ncntrs > 0 ) {
		nfui_nfobject_get_offset(obj, &ofs_x, &ofs_y);
		n = (ensel && cidxc >= 0) ? cidxc : -1;
		//printf("n %d ofs_x %d ofs_y %d  \n",n,ofs_x,ofs_y);
	}
	for (k = 0; k < rl->ncntrs; k++) {
		if ( ++n >= rl->ncntrs )
			n = 0;
		
		//printf("n %d rl->ncntrs %d \n",n,rl->ncntrs);
		
		if ( erase && !emask[n] )
			continue;
		c = &rl->cntrlist[n];
		issel = ensel && (n == cidxc || (erase && n == cidxp));
		if ( !erase ) {
			color.red = (guint16)(c->color[0] << 8);
			color.green = (guint16)(c->color[1] << 8);
			color.blue = (guint16)(c->color[2] << 8);
		}

		//printf("issel %d rl->ncntrs %d \n",issel,rl->ncntrs);
		
		gdk_gc_set_rgb_fg_color(gc, &color);
		gdk_gc_set_line_attributes(gc, issel ? 6 : 3,
				GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
		

		for (i = 0; i < 4; i++) {
			points[i].x = c->pt[i].x *obj->width /(1920*2) + ofs_x;
			points[i].y = c->pt[i].y *obj->height /(1080*2) + ofs_y;
			
		//printf("x %d y %d  points %d %d \n",c->pt[i].x,c->pt[i].y,points[i].x,points[i].y);
		}

		/* Draw rectangle. */
		gdk_draw_polygon(drawable, gc, FALSE, points, 4);

		//printf("issel %d \n",issel);
		/* Draw name. */
		x = points[1].x;
		y = points[1].y;
		h = (gint)nfutil_string_height(drawable, font, c->name, 0);

		//printf("issel %d  h %d!!\n",issel,h);
		#if 1
		/* Draw value. */
		sprintf(sbuf, "%d", cval[n]);
		if(erase)
			sprintf(sbuf, "0000000");
		w = (gint)nfutil_string_width(0, drawable, font, sbuf, 0);
		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_000000)));
		
		if ( erase )
			gdk_draw_rectangle(drawable, gc, TRUE, x+ 9, y+ 9, w, h);
		else
			nfutil_draw_text_with_pango(NULL, NULL, NULL, drawable, gc, sbuf,
				x + 9, y + 9, w, h, font, &color, NFALIGN_LEFT, 0);
		#endif
#if 0
		w = (gint)nfutil_string_width(0, drawable, font, c->name, 0);
		strncpy(sbuf, c->name, sizeof(sbuf));
		if ( w > points[2].x - points[1].x ) {
			l = (gint)strlen(sbuf);
			wd = (gint)nfutil_string_width(0, drawable, font, "...", 0);
			for ( ; w > points[2].x - points[1].x - wd && l > 0; l--) {
				sbuf[l - 1] = '\0';
				w = (gint)nfutil_string_width(0, drawable, font, sbuf, 0);
			}
			strcpy(&sbuf[l], "...");
			w = (gint)nfutil_string_width(0, drawable, font, sbuf, 0);
		}
		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_000000)));
		gdk_draw_rectangle(drawable, gc, TRUE, x, y - h - 3, w, h);
		if ( !erase )
			nfutil_draw_text_with_pango(NULL, NULL, NULL, drawable, gc, sbuf,
					x, y - h - 3, w, h, font, &color, NFALIGN_LEFT, 0);
#else
		/* Draw name. */
		if (1) {
	
			memset(sbuf, 0x00, sizeof(sbuf));
			sprintf(sbuf, "%s", c->name);
			w = (gint)nfutil_string_width(0, drawable, font, sbuf, 0);
			//h = (gint)nfutil_string_height(drawable, font, sbuf, 0);
			x = MIN(x, obj->width - 3 - w + ofs_x);
			x = MAX(x, ofs_x);
			y = MIN(y - h -5 , obj->height - 3 - h + ofs_y);
			y = MAX(y, ofs_y);
			if ( erase )
				gdk_draw_rectangle(drawable, gc, TRUE, x, y, w, h);
			else
				nfutil_draw_text_with_pango(NULL, NULL, NULL, drawable, gc,
						sbuf, x, y, w, h, font, &color, NFALIGN_LEFT, 0);
		}

#endif

	}
}	/* vw_vca_draw_cntrs(... */

/**
 * @brief  Draws track information.
 */
void
vw_vca_draw_ti(NFOBJECT *obj, GdkDrawable *drawable, GdkGC *gc, gint erase,
		ivca_option_t *og, ivca_option_t *ogp, gint count, ivca_meta_obj_t *ti,
		gint Rw, gint Rh)
{
	gint i, n, x, y, w, h, ofs_x, ofs_y;
	gchar sbuf[64];
	static GdkPoint points[IVCAM_MAX_PTS_PER_OBJ];
	GdkColor *pcolor;
	gchar *font = nffont_get_pango_font(NFFONT_MINI_SEMI_5);

	if ( count ) {
		nfui_nfobject_get_offset(obj, &ofs_x, &ofs_y);
		if ( erase )
			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_000000)));
	}
	for (n = 0; n < count; n++) {
		x = MIN(ti[n].rc.x * obj->width / Rw, obj->width - 2);
		y = MIN(ti[n].rc.y * obj->height / Rh, obj->height - 2);
		pcolor = ti[n].nevents ? &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_FF0000)) :
				&UX_COLOR(COLOR_PRG_IDX(UX_COLOR_00FF00));
		if ( !erase )
			gdk_gc_set_rgb_fg_color(gc, pcolor);
		gdk_gc_set_line_attributes(gc, 2,
				GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);

		/* Draw bounding box. */
		if ( og->sw_obj_bb|| (erase && ogp->sw_obj_bb) ) {
			w = ti[n].rc.w * obj->width / Rw;
			h = ti[n].rc.h * obj->height / Rh;
			if ( x <= obj->width - 3 && y <= obj->height - 3 &&
					x + w >= 3 && y + h >= 3 ) {
				if ( x + w > obj->width - 4 )
					w = obj->width - 3 - x;
				if ( y + h > obj->height - 4 )
					h = obj->height - 3 - y;
				if ( x < 3 ) {
					w = x + w - 3;
					x = 3;
				}
				if ( y < 3 ) {
					h = y + h - 3;
					y = 3;
				}
				gdk_draw_rectangle(drawable, gc, FALSE,
						x + ofs_x, y + ofs_y, w, h);
			}
		}

		/* Draw id. */
		if ( og->sw_obj_id|| (erase && ogp->sw_obj_id) ) {
			sprintf(sbuf, "%u", ti[n].id);
			//TODO We have to reduce below calls...
			w = (gint)nfutil_string_width(0, drawable, font, sbuf, 0);
			h = (gint)nfutil_string_height(drawable, font, sbuf, 0);
			x = MIN(x, obj->width - 3 - w);
			x = MAX(x, 3) + ofs_x;
			y = MIN(y - h, obj->height - 3 - h);
			y = MAX(y, 3) + ofs_y;
			if ( erase )
				gdk_draw_rectangle(drawable, gc, TRUE, x, y, w, h);
			else
				nfutil_draw_text_with_pango(NULL, NULL, NULL, drawable, gc,
						sbuf, x, y, w, h, font, pcolor, NFALIGN_LEFT, 0);
		}
#if 0
		/* Draw trajectory. */
		if ( og->sw_obj_tr|| (erase && ogp->sw_obj_tr) ) {
			for (i = 0; i < ti[n].nevents; i++) {
				points[i].x = ti[n].cents[i].x * obj->width / Rw;
				points[i].x = MIN(points[i].x, obj->width - 2) + ofs_x;
				points[i].y = ti[n].cents[i].y * obj->height / Rh;
				points[i].y = MIN(points[i].y, obj->height - 2) + ofs_y;
			}
			gdk_gc_set_line_attributes(gc, 1,
					GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
			gdk_draw_lines(drawable, gc, points, ti[n].nevents);
		}
		#endif
	}
}	/* vw_vca_draw_ti(... */

/**
 * @brief  Draws track information.
 */
void
vw_vca_draw_meta(NFOBJECT *obj, GdkDrawable *drawable, GdkGC *gc, gint erase,
		ivca_option_t *og, ivca_option_t *ogp, gint count, ivca_meta_obj_t *meta,
		gint Rw, gint Rh)
{
	gint i, n, x, y, w, h, ofs_x, ofs_y;
	gchar sbuf[64];
	static GdkPoint points[IVCAM_MAX_PTS_PER_OBJ];
	GdkColor *pcolor;
	gchar *font = nffont_get_pango_font(NFFONT_MINI_SEMI_5);

	if ( count ) {
		nfui_nfobject_get_offset(obj, &ofs_x, &ofs_y);
		if ( erase )
			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_000000)));
	}
//	printf("vw_vca_draw_meta count %d\n",count);
	for (n = 0; n < count; n++) {
		x = MIN(meta[n].rc.x * obj->width / Rw, obj->width - 2);
		y = MIN(meta[n].rc.y * obj->height / Rh, obj->height - 2);
		pcolor = meta[n].nevents ? &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_FF0000)) :
				&UX_COLOR(COLOR_PRG_IDX(UX_COLOR_00FF00));
		if ( !erase )
			gdk_gc_set_rgb_fg_color(gc, pcolor);
		gdk_gc_set_line_attributes(gc, 2,
				GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);

		/* Draw bounding box. */
//		if ( og->show_bb || (erase && ogp->show_bb) ) {
			w = meta[n].rc.w * obj->width / Rw;
			h = meta[n].rc.h * obj->height / Rh;
			if ( x <= obj->width - 3 && y <= obj->height - 3 &&
					x + w >= 3 && y + h >= 3 ) {
				if ( x + w > obj->width - 4 )
					w = obj->width - 3 - x;
				if ( y + h > obj->height - 4 )
					h = obj->height - 3 - y;
				if ( x < 3 ) {
					w = x + w - 3;
					x = 3;
				}
				if ( y < 3 ) {
					h = y + h - 3;
					y = 3;
				}
				gdk_draw_rectangle(drawable, gc, FALSE,
						x + ofs_x, y + ofs_y, w, h);
			}
//		}

		/* Draw id. */
//		if ( og->show_id || (erase && ogp->show_id) ) {
			sprintf(sbuf, "%u", meta[n].id);
//	printf("vw_vca_draw_meta id %d\n",meta[n].id);
			//TODO We have to reduce below calls...
			w = (gint)nfutil_string_width(0, drawable, font, sbuf, 0);
			h = (gint)nfutil_string_height(drawable, font, sbuf, 0);
			x = MIN(x, obj->width - 3 - w);
			x = MAX(x, 3) + ofs_x;
			y = MIN(y - h, obj->height - 3 - h);
			y = MAX(y, 3) + ofs_y;
			if ( erase )
				gdk_draw_rectangle(drawable, gc, TRUE, x, y, w, h);
			else
				nfutil_draw_text_with_pango(NULL, NULL, NULL, drawable, gc,
						sbuf, x, y, w, h, font, pcolor, NFALIGN_LEFT, 0);
//		}
#if 0
		/* Draw trajectory. */
		if ( og->show_tr || (erase && ogp->show_tr) ) {
			for (i = 0; i < ti[n].ncents; i++) {
				points[i].x = ti[n].cents[i].x * obj->width / Rw;
				points[i].x = MIN(points[i].x, obj->width - 2) + ofs_x;
				points[i].y = ti[n].cents[i].y * obj->height / Rh;
				points[i].y = MIN(points[i].y, obj->height - 2) + ofs_y;
			}
			gdk_gc_set_line_attributes(gc, 1,
					GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
			gdk_draw_lines(drawable, gc, points, ti[n].ncents);
		}
		#endif
	}
}	/* vw_vca_draw_ti(... */

/**
 * @brief  Draws track information.
 */
void
vw_vca_draw_meta2(NFOBJECT *obj, GdkDrawable *drawable, GdkGC *gc, gint erase,
		ivca_option_t *og, ivca_option_t *ogp, gint count, ivcam_obj_t*meta,
		gint Rw, gint Rh, gint draw)
{
	gint i, n, x, y, w, h, ofs_x, ofs_y;
	gchar sbuf[64];
	static GdkPoint points[IVCAM_MAX_PTS_PER_OBJ];
	GdkColor *pcolor;
	gchar *font = nffont_get_pango_font(NFFONT_MINI_SEMI_5);

	ivca_meta_obj_t * pObj;
	ivcam_obj_t * ivcam_obj;

	if ( count ) {
		nfui_nfobject_get_offset(obj, &ofs_x, &ofs_y);
		if ( erase )
			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_000000)));
	}

	
	//printf("vw_vca_draw_meta count %d\n",count);
	for (n = 0; n < count; n++) {
		
		ivcam_obj = &meta[n];
		pObj = &ivcam_obj->mobj;
		
		x = MIN(pObj->rc.x * obj->width / Rw, obj->width - 2);
		y = MIN(pObj->rc.y * obj->height / Rh, obj->height - 2);
		pcolor = pObj->nevents ? &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_FF0000)) :
				&UX_COLOR(COLOR_PRG_IDX(UX_COLOR_00FF00));
		if ( !erase )
			gdk_gc_set_rgb_fg_color(gc, pcolor);
		gdk_gc_set_line_attributes(gc, 2,
				GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);

		/* Draw bounding box. */
		if ( (draw && og->sw_obj_bb) || (erase && ogp->sw_obj_bb)) {
			w = pObj->rc.w * obj->width / Rw;
			h = pObj->rc.h * obj->height / Rh;
			if ( x <= obj->width - 3 && y <= obj->height - 3 &&
					x + w >= 3 && y + h >= 3 ) {
				if ( x + w > obj->width - 4 )
					w = obj->width - 3 - x;
				if ( y + h > obj->height - 4 )
					h = obj->height - 3 - y;
				if ( x < 3 ) {
					w = x + w - 3;
					x = 3;
				}
				if ( y < 3 ) {
					h = y + h - 3;
					y = 3;
				}
				gdk_draw_rectangle(drawable, gc, FALSE,
						x + ofs_x, y + ofs_y, w, h);
			}
		}

		/* Draw id. */
		if (  (draw && og->sw_obj_id)|| (erase && ogp->sw_obj_id) ) {
			sprintf(sbuf, "%u", pObj->id);
			if(erase)
				sprintf(sbuf, "0000000000000");	
	//printf("vw_vca_draw_meta id %d\n",pObj->id);
			//TODO We have to reduce below calls...
			w = (gint)nfutil_string_width(0, drawable, font, sbuf, 0);
			h = (gint)nfutil_string_height(drawable, font, sbuf, 0);
			x = MIN(x, obj->width - 3 - w);
			x = MAX(x, 3) + ofs_x;
			y = MIN(y - h, obj->height - 3 - h);
			y = MAX(y, 3) + ofs_y;
			if ( erase )
				gdk_draw_rectangle(drawable, gc, TRUE, x, y, w, h);
			else
				nfutil_draw_text_with_pango(NULL, NULL, NULL, drawable, gc,
						sbuf, x, y, w, h, font, pcolor, NFALIGN_LEFT, 0);
		}
#if 1
		/* draw width */
		if (  (draw && og->sw_obj_w3d)|| (erase && ogp->sw_obj_w3d) ) {
			float width;
			width = (float)pObj->width3d/256;
			sprintf(sbuf, "W : %.1f m", width);
			if(erase)
				sprintf(sbuf, "0000000000000");	
			w = (gint)nfutil_string_width(0, drawable, font, sbuf, 0);
			h = (gint)nfutil_string_height(drawable, font, sbuf, 0);
			x = MIN(x, obj->width - 3 - w);
			x = MAX(x, 3) + ofs_x;
			y = MIN(y - h, obj->height - 3 - h);
			y = MAX(y, 3) + ofs_y;
			if ( erase )
				gdk_draw_rectangle(drawable, gc, TRUE, x, y, w, h);
			else
				nfutil_draw_text_with_pango(NULL, NULL, NULL, drawable, gc,
						sbuf, x, y, w, h, font, pcolor, NFALIGN_LEFT, 0);
		}
		/* draw height */
		if (  (draw && og->sw_obj_h3d)|| (erase && ogp->sw_obj_h3d) ) {
			float height;
			height = (float)pObj->height3d/256;
			sprintf(sbuf, "H : %.1f m", height);
			if(erase)
				sprintf(sbuf, "0000000000000");	
			w = (gint)nfutil_string_width(0, drawable, font, sbuf, 0);
			h = (gint)nfutil_string_height(drawable, font, sbuf, 0);
			x = MIN(x, obj->width - 3 - w);
			x = MAX(x, 3) + ofs_x;
			y = MIN(y - h, obj->height - 3 - h);
			y = MAX(y, 3) + ofs_y;
			if ( erase )
				gdk_draw_rectangle(drawable, gc, TRUE, x, y, w, h);
			else
				nfutil_draw_text_with_pango(NULL, NULL, NULL, drawable, gc,
						sbuf, x, y, w, h, font, pcolor, NFALIGN_LEFT, 0);
		}
		/* draw speed */
		if (  (draw && og->sw_obj_s3d)|| (erase && ogp->sw_obj_s3d) ) {
			float speed;
			speed = (float)pObj->speed3d/256;
			sprintf(sbuf, "S : %.1f km/h", speed);
			if(erase)
				sprintf(sbuf, "0000000000000");		
			w = (gint)nfutil_string_width(0, drawable, font, sbuf, 0);
			h = (gint)nfutil_string_height(drawable, font, sbuf, 0);
			x = MIN(x, obj->width - 3 - w);
			x = MAX(x, 3) + ofs_x;
			y = MIN(y - h, obj->height - 3 - h);
			y = MAX(y, 3) + ofs_y;
			if ( erase )
				gdk_draw_rectangle(drawable, gc, TRUE, x, y, w, h);
			else
				nfutil_draw_text_with_pango(NULL, NULL, NULL, drawable, gc,
						sbuf, x, y, w, h, font, pcolor, NFALIGN_LEFT, 0);
		}
		#endif
#if 1
		/* Draw trajectory. */
		if ( (draw && og->sw_obj_tr)|| (erase && ogp->sw_obj_tr) ) {
			for (i = 0; i < ivcam_obj->npts; i++) {
				points[i].x = ivcam_obj->traj[i].x * obj->width / Rw;
				points[i].x = MIN(points[i].x, obj->width - 2) + ofs_x;
				points[i].y = ivcam_obj->traj[i].y * obj->height / Rh;
				points[i].y = MIN(points[i].y, obj->height - 2) + ofs_y;
				if(points[i].x < ofs_x)
					points[i].x  = ofs_x + 2;
				if(points[i].y < ofs_y)
					points[i].y  = ofs_y + 2;
			}
			gdk_gc_set_line_attributes(gc, 1,
					GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
			gdk_draw_lines(drawable, gc, points, ivcam_obj->npts);
		}
		#endif
	}
}	/* vw_vca_draw_ti(... */


static gchar *add_conf_contents[] = {"Select the Zone Type"};
static gchar *add_conf_btnstr[] = {"Line","Area"};
static MBOXCONF add_conf_mbconf = {
	"CONFIRM",
	add_conf_contents,
	1,
	add_conf_btnstr,
	FALSE,
	NFALIGN_CENTER
};


gint
vw_vca_add_zone(NFOBJECT *parent, ivca_rule_t *r, gint zone_type)
{
	gint type=zone_type, i, idx;
	ivca_zone_t *z;
	GdkColor color = {0, 0, 0, 0};

	if ( r->nzones >= IVCA_MAX_ZONES ) {
		nftool_mbox((NFWINDOW *)parent, "ERROR",
				"Cannot add more zones.", NFTOOL_MB_OK);
		return -1;
	}

	if( type != IVCA_RT_LINE && type != IVCA_RT_AREA ) {
		type = nftool_mbox_by_conf((NFWINDOW *)parent,
				&add_conf_mbconf, NFTOOL_MB_CONF_2);
		if ( type != IVCA_RT_LINE && type != IVCA_RT_AREA ) {
			nftool_mbox((NFWINDOW *)parent, "ERROR",
					"Cannot add a zone.\nType of the zone is invalid.",
					NFTOOL_MB_OK);
			return -1;
		}
	}

	/* Get a new idx. */
	for (idx = 0; idx < r->nzones; idx++)
		if ( r->zonelist[idx].id != idx )
			break;
	for (i = r->nzones, z = &r->zonelist[i]; i > idx; i--, z--)
		memcpy(z, z - 1, sizeof(*z));

	/* Initialize. */
	memset(z, 0, sizeof(*z));
	z->id = (gint16)idx;
	z->type = (guint8)type;
	z->active = 1;

    color = color_template[rand() % N_COLORS];
    z->color[0] = (guint8)(color.red >> 8);
    z->color[1] = (guint8)(color.green >> 8);
    z->color[2] = (guint8)(color.blue >> 8);

	//z->color[0] = color8[rand() % 2];
	//z->color[1] = color8[rand() % 2];
	//z->color[2] = color8[rand() % 2];

	if ( z->color[0] == 0 && z->color[1] == 0 && z->color[2] == 0 )
		z->color[rand() % 3] = 0xFF;

	/* Add default points. */
	//TODO
	//Remove 12, 9 --> should be computed from processing resolution.
	//
	if ( type == IVCA_RT_LINE ) {
		z->npts = 2;
		z->pt[0].x = 120 * 12; z->pt[0].y = 140 * 9;
		z->pt[1].x = 200 * 12; z->pt[1].y = 100 * 9;
	}
	else {
		z->npts = 4;
		z->pt[0].x = 120 * 12; z->pt[0].y = 140 * 9;
		z->pt[1].x = 120 * 12; z->pt[1].y = 100 * 9;
		z->pt[2].x = 200 * 12; z->pt[2].y = 100 * 9;
		z->pt[3].x = 200 * 12; z->pt[3].y = 140 * 9;
	}

	sprintf(z->name, "Zone%d", idx+1);
	r->nzones++;
	return idx;
}	/* vw_vca_add_zone(... */

gint
vw_vca_add_cntr(NFOBJECT *parent, ivca_rule_t *r)
{
	gint i, idx;
	ivca_cntr_t *c;
	GdkColor color;

	if ( r->ncntrs >= IVCA_MAX_CNTRS ) {
		nftool_mbox((NFWINDOW *)parent, "ERROR",
				"Cannot add more counters.", NFTOOL_MB_OK);
		return -1;
	}
	//printf("ncntrs %d \n",r->ncntrs);

	/* Get a new idx. */
	for (idx = 0; idx < r->ncntrs; idx++)
		if ( r->cntrlist[idx].id != idx )
			break;
	for (i = r->ncntrs, c = &r->cntrlist[i]; i > idx; i--, c--)
		memcpy(c, c - 1, sizeof(*c));

	/* Initialize. */
	memset(c, 0, sizeof(*c));
	c->id = (gint16)idx;
	c->active = 1;
	c->zid_up = c->zid_dn = -1;

    color = color_template[rand() % N_COLORS];
    c->color[0] = (guint8)(color.red >> 8);
    c->color[1] = (guint8)(color.green >> 8);
    c->color[2] = (guint8)(color.blue >> 8);

	//c->color[0] = color8[rand() % 3];
	//c->color[1] = color8[rand() % 3];
	//c->color[2] = color8[rand() % 3];
	if ( c->color[0] == 0 && c->color[1] == 0 && c->color[2] == 0 )
		c->color[rand() % 3] = 0xFF;

	/* Add default points. */
	//TODO
	//Remove 12, 9 --> should be computed from processing resolution.
	//
	c->pt[0].x = 135 * 12; c->pt[0].y = 128 * 9;
	c->pt[1].x = 135 * 12; c->pt[1].y = 112 * 9;
	c->pt[2].x = 185 * 12; c->pt[2].y = 112 * 9;
	c->pt[3].x = 185 * 12; c->pt[3].y = 128 * 9;

	//captainnn
	//c->pt[0].x = 384; c->pt[0].y = 144*3;
	//c->pt[1].x = 384; c->pt[1].y = 144*2;
	//c->pt[2].x = 384*2; c->pt[2].y = 144*2;
	//c->pt[3].x = 384*2; c->pt[3].y = 144*3;	


	sprintf(c->name, "Counter%d", idx + 1);
	r->ncntrs++;
	return idx;
}	/* vw_vca_add_cntr(... */

gint
vw_vca_delete_zone(gint idx, ivca_rule_t *r)
{
	gint i;
	ivca_zone_t *z;
	ivca_cntr_t *c;

	if ( idx < 0 || idx >= r->nzones )
		return -1;
	z = &r->zonelist[idx];

	/* Remove from count sources. */
	for (c = r->cntrlist, i = 0; i < r->ncntrs; i++, c++) {
		if ( c->zid_up == z->id )
			c->zid_up = -1;
		if ( c->zid_dn == z->id )
			c->zid_dn = -1;
	}

	/* Remove from the zone list. */
	for (i = idx; i < r->nzones - 1; i++, z++)
		memcpy(z, z + 1, sizeof(*z));
	memset(z, 0, sizeof(*z));

	return idx - (idx >= --r->nzones);
}	/* vw_vca_delete_zone(... */

gint
vw_vca_delete_cntr(gint idx, ivca_rule_t *r)
{
	gint i;
	ivca_cntr_t *c;

	if ( idx < 0 || idx >= r->ncntrs )
		return -1;
	c = &r->cntrlist[idx];

	/* Remove from the counter list. */
	for (i = idx; i < r->ncntrs - 1; i++, c++)
		memcpy(c, c + 1, sizeof(*c));
	memset(c, 0, sizeof(*c));

	idx -= idx >= --r->ncntrs;
	return idx;
}	/* vw_vca_delete_cntr(... */

gint
vw_vca_add_cal_target(NFOBJECT *parent, ivca_calib_t* t)
{
	gint i, idx;
	ivca_calib_target_t* c;

	if(t->ntargets >= IVCA_MAX_CALIB_TARGETS){
		nftool_mbox((NFWINDOW *)parent, "ERROR",
				"Cannot add more targtet.", NFTOOL_MB_OK);
		return -1;
	}

	/* Get a new idx. */
	idx = t->ntargets;
	c = &t->targetlist[idx];

	/* Initialize. */
	memset(c, 0, sizeof(*c));
	c->height = 175;
	c->pt[0].x = 1920;
	c->pt[0].y = 950;
	c->pt[1].x = 1920;
	c->pt[1].y = 1210;

	t->ntargets++;
	return idx;
}	/* vw_vca_add_zone(... */

gint vw_vca_cal_estimate(NFOBJECT *parent, ivca_calib_t* t, gint ch)
{
	gint i, idx, res;
	gint cap, cur;

	nf_ipcam_get_resol(ch, 1, &cap , &cur, NULL);

	printf("cal resol  %x\n",cur );

	if(cur == NF_IPCAM_RES_320x180){
		t->p_width = 320;
		t->p_height = 180;
	}
	else{
		t->p_width = 640;
		t->p_height = 360;
	}
	
	//change pt 
	if(cur == NF_IPCAM_RES_320x180){
		for(i=0; i< t->ntargets; i++){
			t->targetlist[i].pt[0].x = t->targetlist[i].pt[0].x/12;
			t->targetlist[i].pt[0].y = t->targetlist[i].pt[0].y/12;
			t->targetlist[i].pt[1].x = t->targetlist[i].pt[1].x/12;
			t->targetlist[i].pt[1].y = t->targetlist[i].pt[1].y/12;
		}
	}
	else{
		for(i=0; i< t->ntargets; i++){
			t->targetlist[i].pt[0].x = t->targetlist[i].pt[0].x/6;
			t->targetlist[i].pt[0].y = t->targetlist[i].pt[0].y/6;
			t->targetlist[i].pt[1].x = t->targetlist[i].pt[1].x/6;
			t->targetlist[i].pt[1].y = t->targetlist[i].pt[1].y/6;
		}
	}
	
	
	res = icalib_process(t);
	
	//change pt 
	if(cur == NF_IPCAM_RES_320x180){
		for(i=0; i< t->ntargets; i++){
			t->targetlist[i].pt[0].x = t->targetlist[i].pt[0].x*12;
			t->targetlist[i].pt[0].y = t->targetlist[i].pt[0].y*12;
			t->targetlist[i].pt[1].x = t->targetlist[i].pt[1].x*12;
			t->targetlist[i].pt[1].y = t->targetlist[i].pt[1].y*12;
		}
	}
	else{
		for(i=0; i< t->ntargets; i++){
			t->targetlist[i].pt[0].x = t->targetlist[i].pt[0].x*6;
			t->targetlist[i].pt[0].y = t->targetlist[i].pt[0].y*6;
			t->targetlist[i].pt[1].x = t->targetlist[i].pt[1].x*6;
			t->targetlist[i].pt[1].y = t->targetlist[i].pt[1].y*6;
		}
	}
	
	if(res<0){
//		nftool_mbox((NFWINDOW *)parent, "ERROR",
//				"Some targets are invalid. Fix invalid targets or add more targets.", NFTOOL_MB_OK);
		return -1;
	}
	//t->focal = 2*atan(960/t->focal)*180/3.14159;
	//t->focal = t->focal/6;
	
	return 0;
}	/* vw_vca_cal_estimate(... */

gint
vw_vca_del_cal_target(ivca_calib_t* t, gint idx)
{
	gint i;
	ivca_calib_target_t* c;

	if(idx < 0 || idx > IVCA_MAX_CALIB_TARGETS)
		return -1;

	c = &t->targetlist[idx];

	/* Remove from the target list. */
	for (i = idx; i < t->ntargets - 1; i++, c++)
		memcpy(c, c + 1, sizeof(*c));
	memset(c, 0, sizeof(*c));

	idx -= idx >= --t->ntargets;
	return idx;
}	/* vw_vca_add_zone(... */

gint vw_vca_add_target(ivca_calib_target_t *target)
{
	/* Initialize. */
	memset(target, 0, sizeof(*target));
	target->height = 175;
	target->pt[0].x = 960;
	target->pt[0].y = 475;
	target->pt[1].x = 960;
	target->pt[1].y = 605;

}	/* vw_vca_add_zone(... */

