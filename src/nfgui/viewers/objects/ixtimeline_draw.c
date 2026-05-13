/*
 * ixtimeline_draw.c
 * 	- timeline widget drawing module
 *	- dependencies :
 *		gtk
 *		
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Apr 7, 2011
 *
 */

#include "ixtimeline.h"
#include "ixtimeline_internal.h"
#include "iux_afx.h"
#include "support/nf_ui_color.h"
#include "support/color.h"
#include "support/util.h"
#include "support/nf_ui_font.h"


#define DBG_LEVEL		0
#define DBG_MODULE		"WG_TML"


#define OFFSET_TEXT_DISPLAY	4
#define OFFSET_BAR_DISPLAY	4


////////////////////////////////////////////////////////////
//
// private data type
//

typedef enum _RBAR_TYPE_E {
	RB_NONE			= 0,
	RB_CONTINUOUS	= 1,
	RB_ALARM		= 2,
	RB_MOTION		= 3,
	RB_PANIC		= 4,
	MBX_RB_TYPE,
} RBAR_TYPE_E;


////////////////////////////////////////////////////////////
//
// private variables 
//


static int _drawback_stick_h(TML_DRW_T *, STICK_INFO *);
static int _drawback_stick_v(TML_DRW_T *, STICK_INFO *);
static int (*_drawback_stick[2])(TML_DRW_T *, STICK_INFO *) = {
	_drawback_stick_h,
	_drawback_stick_v,
};

static int _draw_stick_h(TML_DRW_T *, STICK_INFO *, GdkPixbuf *, POS, int);
static int _draw_stick_v(TML_DRW_T *, STICK_INFO *, GdkPixbuf *, POS, int);
static int (*_draw_stick[2])(TML_DRW_T *, STICK_INFO *, GdkPixbuf *, POS, int) = {
	_draw_stick_h,
	_draw_stick_v,
};

static int _print_stick_h(TML_DRW_T *, STICK_INFO *, GdkPixbuf *, POS, int);
static int _print_stick_v(TML_DRW_T *, STICK_INFO *, GdkPixbuf *, POS, int);
static int (*_print_stick[2])(TML_DRW_T *, STICK_INFO *, GdkPixbuf *, POS, int) = {
	_print_stick_h,
	_print_stick_v,
};

static int _hide_stick_h(TML_DRW_T *, STICK_INFO *);
static int _hide_stick_v(TML_DRW_T *, STICK_INFO *);
static int (*_hide_stick[2])(TML_DRW_T *, STICK_INFO *) = {
	_hide_stick_h,
	_hide_stick_v,
};

static int _draw_rbar_h(TML_DRW_T *, TML_FIGURE_T *, int, RLIST *);
static int _draw_rbar_v(TML_DRW_T *, TML_FIGURE_T *, int, RLIST *);
static int (*_draw_rbar[2])(TML_DRW_T *, TML_FIGURE_T *, int, RLIST *) = {
	_draw_rbar_h,
	_draw_rbar_v,
};

static int _draw_ruler_h(TML_DRW_T *, TML_FIGURE_T *);
static int _draw_ruler_v(TML_DRW_T *, TML_FIGURE_T *);
static int (*_draw_ruler[2])(TML_DRW_T *, TML_FIGURE_T *) = {
	_draw_ruler_h,
	_draw_ruler_v,
};

static int _draw_section_h(TML_DRW_T *, GdkPixbuf *, POS, POS);
static int _draw_section_v(TML_DRW_T *, GdkPixbuf *, POS, POS);
static int (*_draw_section[2])(TML_DRW_T *, GdkPixbuf *, POS, POS) = {
	_draw_section_h,
	_draw_section_v,
};

static int _print_section_h(TML_DRW_T *, GdkPixbuf *, POS, POS);
static int _print_section_v(TML_DRW_T *, GdkPixbuf *, POS, POS);
static int (*_print_section[2])(TML_DRW_T *, GdkPixbuf *, POS, POS) = {
	_print_section_h,
	_print_section_v,
};

static int _hide_section_h(TML_DRW_T *);
static int _hide_section_v(TML_DRW_T *);
static int (*_hide_section[2])(TML_DRW_T *) = {
	_hide_section_h,
	_hide_section_v,
};

static int _print_canvus_h(TML_DRW_T *);
static int _print_canvus_v(TML_DRW_T *);
static int (*_print_canvus[2])(TML_DRW_T *) = {
	_print_canvus_h,
	_print_canvus_v,
};

static int _blit_h(TML_DRW_T *);
static int _blit_v(TML_DRW_T *);
static int (*_blit[2])(TML_DRW_T *) = {
	_blit_h,
	_blit_v,
};

////////////////////////////////////////////////////////////
//
// private functions
//
static int _draw_bar(GdkDrawable *drawable, GdkGC *gc, int x, int y, int w, int h)
{
	gdk_draw_rectangle(drawable, gc, TRUE, x, y, w, h);
	return 0;
}

static int _draw_ruler_h(TML_DRW_T *drw, TML_FIGURE_T *figure)
{
	int i;
	int x = drw->pcur;
/*	GdkPoint pt[3];

	pt[0].x = drw->rc_rba.x + x - 4;
	pt[0].y = drw->rc_rba.y - 5;
	pt[1].x = drw->rc_rba.x + x;
	pt[1].y = drw->rc_rba.y;
	pt[2].x = drw->rc_rba.x + x + 5;
	pt[2].y = drw->rc_rba.y - 5;
*/
	_draw_bar(drw->pm_cnvs, drw->gc_ruler_bg, 
			drw->rc_ruler.x, drw->rc_ruler.y, 
			drw->rc_ruler.width, drw->rc_ruler.height);

	for (i = 0; i < drw->sm.cnt; ++i) {
		gdk_draw_line(drw->pm_cnvs, drw->gc_ruler_tx,
				drw->rc_ruler.x + drw->sm.pos_mark[i], drw->rc_ruler.y + 0,
				drw->rc_ruler.x + drw->sm.pos_mark[i], drw->rc_ruler.y + 18);

		nfutil_draw_text_with_pango(NULL, NULL, NULL, drw->pm_cnvs, drw->gc_ruler_tx, 
				drw->sm.txt_mark[i],
				// 5 means just offset
				drw->rc_ruler.x + drw->sm.pos_mark[i], drw->rc_ruler.y + 5,
				46, 11,
				nffont_get_pango_font(NFFONT_MINI_SEMI_1),
				&figure->cr_ruler_text, NFALIGN_CENTER, 0);

	}

	gdk_draw_line(drw->pm_cnvs, drw->gc_ruler_tx,
			drw->rc_ruler.x, drw->rc_ruler.y + 18,
			drw->rc_ruler.x + drw->rc_ruler.width - 1, drw->rc_ruler.y + 18);

//	gdk_draw_polygon(drw->drawable, drw->gc_cti,
//						TRUE, pt, 3);
}

static int _draw_ruler_v(TML_DRW_T *drw, TML_FIGURE_T *figure)
{
	int i;
	int ybtm = drw->rc_ruler.y + drw->rc_ruler.height - 1;
	int y = (drw->rc_rba.height - 1) - drw->pcur;
/*	GdkPoint pt[3];

	pt[0].x = drw->rc_rba.x - 5;
	pt[0].y = drw->rc_rba.y + y - 4;
	pt[1].x = drw->rc_rba.x;
	pt[1].y = drw->rc_rba.y + y;
	pt[2].x = drw->rc_rba.x - 5;
	pt[2].y = drw->rc_rba.y + y + 4;
*/

	_draw_bar(drw->pm_cnvs, drw->gc_ruler_bg,
			drw->rc_ruler.x, drw->rc_ruler.y, 
			drw->rc_ruler.width, drw->rc_ruler.height);

	gdk_draw_line(drw->pm_cnvs, drw->gc_ruler_tx,
			drw->rc_ruler.x + 39, drw->rc_ruler.y, 
			drw->rc_ruler.x + 39, ybtm);
	
	for (i = 0; i < drw->sm.cnt; ++i) {
		nfutil_draw_text_with_pango(NULL, NULL, NULL, drw->pm_cnvs, drw->gc_ruler_tx, 
				drw->sm.txt_mark[i],
				// 5 means just offset
				drw->rc_ruler.x - 5, ybtm - drw->sm.pos_mark[i] - OFFSET_TEXT_DISPLAY,
				46, 11,
				nffont_get_pango_font(NFFONT_MINI_SEMI_1),
				&figure->cr_ruler_text, NFALIGN_CENTER, 0);


	if (strlen(drw->sm.txt_date[i]))
		nfutil_draw_text_with_pango(NULL, NULL, NULL, drw->pm_cnvs, drw->gc_date,
				drw->sm.txt_date[i],
				// 5 means just offset
				drw->rc_ruler.x - 5, ybtm - drw->sm.pos_mark[i] - OFFSET_TEXT_DISPLAY - 10,
				46, 11,
				nffont_get_pango_font(NFFONT_MINI_NORMAL),
				&figure->cr_date, NFALIGN_CENTER, 0);

		gdk_draw_line(drw->pm_cnvs, drw->gc_ruler_tx,
				drw->rc_ruler.x + 40, ybtm - drw->sm.pos_mark[i],
				drw->rc_ruler.x + 45, ybtm - drw->sm.pos_mark[i]);
	}

//	gdk_draw_polygon(drw->pm_cnvs, drw->gc_cti,
//			TRUE, pt, 3);
	return 0;
}

static int _draw_rbar_h(TML_DRW_T *drw, TML_FIGURE_T *figure, int ch, RLIST *list)
{
	int y, h;
	RLIST *plist;
	SZ_RBAR_T *rbar;
	int offset = 0;

	y = drw->rc_rba.y + ch * drw->ch_brd;
	h = drw->ch_brd - 2;

	for (plist = list; plist; plist = g_list_next(plist)) {
		rbar = (SZ_RBAR_T *)(plist->data);

		offset = (rbar->intr == INTERACED) ? 7 : 0;
		_draw_bar(drw->pm_cnvs, drw->gc_rbar[rbar->reason + offset],
				drw->rc_rba.x + rbar->px_s, y,
				rbar->px_e - rbar->px_s + 1, h);
	}
	return 0;
}

static int _draw_rbar_v(TML_DRW_T *drw, TML_FIGURE_T *figure, int ch, RLIST *list)
{
	int x, w;
	RLIST *plist;
	SZ_RBAR_T *rbar;
	int offset = 0;

	x = drw->rc_rba.x + ch * drw->ch_brd;
	w = drw->ch_brd - 2;

	if (drw->ch_cnt == 16) x += ((ch / 4) * 2);

	for (plist = list; plist; plist = g_list_next(plist)) {
		rbar = (SZ_RBAR_T *)(plist->data);

		offset = (rbar->intr == INTERACED) ? 7 : 0;
		_draw_bar(drw->pm_cnvs, drw->gc_rbar[rbar->reason + offset],
				x, drw->rc_rba.y + (drw->rc_rba.height - 1) - rbar->px_e,
				w, rbar->px_e - rbar->px_s + 1);
	}
	return 0;
}

static int _core_draw_stick_h(TML_DRW_T *drw, STICK_INFO *prev, GdkPixbuf *pbuf, POS pos, GdkDrawable *gdkdrw, int stick_type)
{
	int w = 1;
	int h = drw->rc_rba.height;
	int x = pos;
	GdkPoint pt[3];

	pt[0].x = drw->rc_rba.x + x - 4;
	pt[0].y = drw->rc_rba.y - 5;
	pt[1].x = drw->rc_rba.x + x;
	pt[1].y = drw->rc_rba.y;
	pt[2].x = drw->rc_rba.x + x + 5;
	pt[2].y = drw->rc_rba.y - 5;

#if 0
	if (prev->x >= 0 && prev->y >= 0) {
		gdk_draw_pixmap(gdkdrw, drw->gc, drw->pm_cnvs,
				prev->x, prev->y, prev->x, prev->y, w, h);

		gdk_draw_pixmap(gdkdrw, drw->gc, drw->pm_cnvs,
				prev->x - 4, prev->y - 5, prev->x - 4, prev->y - 5, 9, 5);
	}
#endif

	switch (stick_type) {
	case 0:
		gdk_draw_line(gdkdrw, drw->gc_playbar,
				drw->rc_rba.x + x, drw->rc_rba.y, 
				drw->rc_rba.x + x, drw->rc_rba.y + h - 1);

		gdk_draw_polygon(gdkdrw, drw->gc_playbar, TRUE, pt, 3);
		break;
	case 1:
		gdk_draw_line(gdkdrw, drw->gc_cti,
				drw->rc_rba.x + x, drw->rc_rba.y, 
				drw->rc_rba.x + x, drw->rc_rba.y + h - 1);

		gdk_draw_polygon(gdkdrw, drw->gc_cti, TRUE, pt, 3);
		break;
	}


	prev->x = drw->rc_rba.x + x;			// 5 means overlapped area
	prev->y = drw->rc_rba.y;
	prev->w = w;
	prev->h = h;

	return 0;
}

static int _core_drawback_stick_h(TML_DRW_T *drw, STICK_INFO *prev, GdkDrawable *gdkdrw)
{
	int w = 1;
	int h = drw->rc_rba.height;
	int x;

	if (prev->x >= 0 && prev->y >= 0) {
		gdk_draw_pixmap(gdkdrw, drw->gc, drw->pm_cnvs,
				prev->x, prev->y, prev->x, prev->y, w, h);

		w = 9;
		x = prev->x - 4;
		if (x < drw->rc_rba.x) {
			x = drw->rc_rba.x;
			w = 5;
		}
		if (x >= drw->rc_rba.x + drw->px_len) w = 5;

		gdk_draw_pixmap(gdkdrw, drw->gc, drw->pm_cnvs,
				x, prev->y - 5, x, prev->y - 5, w, 5);
	}

	return 0;
}

static int _draw_stick_h(TML_DRW_T *drw, STICK_INFO *prev, GdkPixbuf *pbuf, POS pos, int stick_type)
{
	_core_draw_stick_h(drw, prev, pbuf, pos, drw->drawable, stick_type);
	return 0;
}

static int _drawback_stick_h(TML_DRW_T *drw, STICK_INFO *prev)
{
	_core_drawback_stick_h(drw, prev, drw->drawable);
	return 0;
}

static int _print_stick_h(TML_DRW_T *drw, STICK_INFO *prev, GdkPixbuf *pbuf, POS pos, int stick_type)
{
	_core_draw_stick_h(drw, prev, pbuf, pos, drw->pm_blit, stick_type);
	return 0;
}

static int _printback_stick_h(TML_DRW_T *drw, STICK_INFO *prev)
{
	_core_drawback_stick_h(drw, prev, drw->pm_blit);
	return 0;
}

static int _draw_stick_h2(TML_DRW_T *drw, STICK_INFO *prev, GdkPixbuf *pbuf, POS pos)
{
	int w = gdk_pixbuf_get_width(pbuf);
	int h = gdk_pixbuf_get_height(pbuf);
	int x = pos;

	x -= OFFSET_BAR_DISPLAY; 

	if (prev->x >= 0 && prev->y >= 0) {
		gdk_draw_pixmap(drw->drawable, drw->gc, drw->pm_cnvs,
				prev->x, prev->y, prev->x, prev->y, w, h);
	}

	gdk_draw_pixbuf(drw->drawable, drw->gc, pbuf, 
			0, 0, 
			drw->rc_rba.x + x, drw->rc_rba.y,
			w, h,
			GDK_RGB_DITHER_NORMAL, 0, 0);

	prev->x = drw->rc_rba.x + x;			// 5 means overlapped area
	prev->y = drw->rc_rba.y;
	prev->w = w;
	prev->h = h;

	return 0;
}

static int _core_draw_stick_v(TML_DRW_T *drw, STICK_INFO *prev, GdkPixbuf *pbuf, POS pos, GdkDrawable *gdkdrw, int stick_type)
{
	int w = drw->rc_rba.width;
	int h = 1;
	int y = (drw->rc_rba.height - 1) - pos;
	GdkPoint pt[3];

	pt[0].x = drw->rc_rba.x - 5;
	pt[0].y = drw->rc_rba.y + y - 4;
	pt[1].x = drw->rc_rba.x;
	pt[1].y = drw->rc_rba.y + y;
	pt[2].x = drw->rc_rba.x - 5;
	pt[2].y = drw->rc_rba.y + y + 4;

#if 0
	if (prev->x >= 0 && prev->y >= 0) {
		gdk_draw_pixmap(gdkdrw, drw->gc, drw->pm_cnvs,
				prev->x, prev->y, prev->x, prev->y, w, h);

		gdk_draw_pixmap(gdkdrw, drw->gc, drw->pm_cnvs,
				prev->x - 5, prev->y - 4, prev->x - 5, prev->y - 4, 5, 8);
	}
#endif
	switch (stick_type) {
	case 0:
		gdk_draw_line(gdkdrw, drw->gc_playbar,
				drw->rc_rba.x, drw->rc_rba.y + y, 
				drw->rc_rba.x + w - 1, drw->rc_rba.y + y);

		gdk_draw_polygon(gdkdrw, drw->gc_playbar, TRUE, pt, 3);
		break;
	case 1:
		gdk_draw_line(gdkdrw, drw->gc_cti,
				drw->rc_rba.x, drw->rc_rba.y + y, 
				drw->rc_rba.x + w - 1, drw->rc_rba.y + y);

		gdk_draw_polygon(gdkdrw, drw->gc_cti, TRUE, pt, 3);
		break;
	}


	prev->x = drw->rc_rba.x;
	prev->y = drw->rc_rba.y + y;
	prev->w = w;
	prev->h = h;

	return 0;
}

static int _core_drawback_stick_v(TML_DRW_T *drw, STICK_INFO *prev, GdkDrawable *gdkdrw)
{
	int w = drw->rc_rba.width;
	int h = 1;
	int y;

	if (prev->x >= 0 && prev->y >= 0) {
		gdk_draw_pixmap(gdkdrw, drw->gc, drw->pm_cnvs,
				prev->x, prev->y, prev->x, prev->y, w, h);

		h = 9;
		y = prev->y - 4;
		if (y < drw->rc_rba.y) {
			y = drw->rc_rba.y;
			h = 4;
		}
		if (y >= drw->rc_rba.y + drw->px_len) h = 5;

		gdk_draw_pixmap(gdkdrw, drw->gc, drw->pm_cnvs,
				prev->x - 5, y, prev->x - 5, prev->y - 4, 5, h);
	}

	return 0;
}


static int _draw_stick_v(TML_DRW_T *drw, STICK_INFO *prev, GdkPixbuf *pbuf, POS pos, int stick_type)
{
	_core_draw_stick_v(drw, prev, pbuf, pos, drw->drawable, stick_type);
	return 0;
}

static int _drawback_stick_v(TML_DRW_T *drw, STICK_INFO *prev)
{
	_core_drawback_stick_v(drw, prev, drw->drawable);
	return 0;
}

static int _print_stick_v(TML_DRW_T *drw, STICK_INFO *prev, GdkPixbuf *pbuf, POS pos, int stick_type)
{
	_core_draw_stick_v(drw, prev, pbuf, pos, drw->pm_blit, stick_type);
	return 0;
}

static int _printback_stick_v(TML_DRW_T *drw, STICK_INFO *prev)
{
	_core_drawback_stick_v(drw, prev, drw->pm_blit);
	return 0;
}

static int _draw_stick_v2(TML_DRW_T *drw, STICK_INFO *prev, GdkPixbuf *pbuf, POS pos)
{
	int w = gdk_pixbuf_get_width(pbuf);
	int h = gdk_pixbuf_get_height(pbuf);
	int y = (drw->rc_rba.height - 1) - pos;

	y -= OFFSET_BAR_DISPLAY; 

	if (prev->x >= 0 && prev->y >= 0) {
		gdk_draw_pixmap(drw->drawable, drw->gc, drw->pm_cnvs,
				prev->x, prev->y, prev->x, prev->y, w, h);
	}

	gdk_draw_pixbuf(drw->drawable, drw->gc, pbuf, 
			0, 0, 
			drw->rc_rba.x - 6, drw->rc_rba.y + y,
			w, h,
			GDK_RGB_DITHER_NORMAL, 0, 0);

	prev->x = drw->rc_rba.x - 6;			// 5 means overlapped area
	prev->y = drw->rc_rba.y + y;
	prev->w = w;
	prev->h = h;

	return 0;
}

static int _hide_stick_h(TML_DRW_T *drw, STICK_INFO *prev)
{
	if (prev->x != -1) {
		gdk_draw_pixmap(drw->drawable, drw->gc, drw->pm_cnvs,
				prev->x, prev->y, prev->x, prev->y, prev->w, prev->h);

		gdk_draw_pixmap(drw->drawable, drw->gc, drw->pm_cnvs,
				prev->x - 4, prev->y - 5, prev->x - 4, prev->y - 5, 9, 5);
	}
	prev->x = -1;
	prev->y = -1;
	return 0;
}

static int _hide_stick_h2(TML_DRW_T *drw, STICK_INFO *prev)
{
	if (prev->x != -1) {
		gdk_draw_pixmap(drw->drawable, drw->gc, drw->pm_cnvs,
				prev->x, prev->y, prev->x, prev->y, prev->w, prev->h);
	}
	prev->x = -1;
	prev->y = -1;
	return 0;
}

static int _hide_stick_v(TML_DRW_T *drw, STICK_INFO *prev)
{
	if (prev->x != -1) {
		gdk_draw_pixmap(drw->drawable, drw->gc, drw->pm_cnvs,
				prev->x, prev->y, prev->x, prev->y, prev->w, prev->h);

		gdk_draw_pixmap(drw->drawable, drw->gc, drw->pm_cnvs,
				prev->x - 5, prev->y - 4, prev->x - 5, prev->y - 4, 5, 9);
	}
	prev->x = -1;
	prev->y = -1;
	return 0;
}

static int _hide_stick_v2(TML_DRW_T *drw, STICK_INFO *prev)
{
	if (prev->x != -1) {
		gdk_draw_pixmap(drw->drawable, drw->gc, drw->pm_cnvs,
				prev->x, prev->y, prev->x, prev->y, prev->w, prev->h);
	}
	prev->x = -1;
	prev->y = -1;
	return 0;
}

static int _pos_to_coord_x(TML_DRW_T *drw, POS pos)
{
	return drw->rc_rba.x + pos;
}

static int _rpos_to_coord_y(TML_DRW_T *drw, RPOS rpos)
{
	return drw->rc_rba.y + rpos;
}

static int _curtain_h(TML_DRW_T *drw, int x, int w)
{
	gdk_draw_pixmap(drw->drawable, drw->gc, drw->pm_comp,
			x, drw->rc_rba.y,
			x, drw->rc_rba.y,
			w, drw->rc_rba.height);
	return 0;
}

static int _curtain_print_h(TML_DRW_T *drw, int x, int w)
{
	gdk_draw_pixmap(drw->pm_blit, drw->gc, drw->pm_comp,
			x, drw->rc_rba.y,
			x, drw->rc_rba.y,
			w, drw->rc_rba.height);
	return 0;
}

static int _uncurtain_h(TML_DRW_T *drw, int x, int w)
{
	gdk_draw_pixmap(drw->drawable, drw->gc, drw->pm_cnvs, 
			x, drw->rc_rba.y, 
			x, drw->rc_rba.y, 
			w, drw->rc_rba.height);
	return 0;
}

static int _curtain_v(TML_DRW_T *drw, int y, int h)
{
	gdk_draw_pixmap(drw->drawable, drw->gc, drw->pm_comp,
			drw->rc_rba.x, y,
			drw->rc_rba.x, y,
			drw->rc_rba.width, h);
	return 0;
}

static int _curtain_print_v(TML_DRW_T *drw, int y, int h)
{
	gdk_draw_pixmap(drw->pm_blit, drw->gc, drw->pm_comp,
			drw->rc_rba.x, y,
			drw->rc_rba.x, y,
			drw->rc_rba.width, h);
	return 0;
}

static int _uncurtain_v(TML_DRW_T *drw, int y, int h)
{
	gdk_draw_pixmap(drw->drawable, drw->gc, drw->pm_cnvs, 
			drw->rc_rba.x, y, 
			drw->rc_rba.x, y, 
			drw->rc_rba.width, h);
	return 0;
}

static int _draw_section_h(TML_DRW_T *drw, GdkPixbuf *pb_ctn, POS start, POS end)
{
	POS xe, xs, prev_s, prev_e;
	int w;
	int x;

	if (start == end) {						// section is not exist
		drw->prev_pstart = start;
		drw->prev_pend = end;
	}

	xe = end;
	xs = start;
	prev_s = drw->prev_pstart;
	prev_e = drw->prev_pend;

	if (prev_s > xe) prev_s = xe;			// exeption
	
	if (prev_e < xe) {						// end stick is moved to right 
		w = xe - prev_e;
		x = _pos_to_coord_x(drw, prev_e);		
		_curtain_h(drw, x, w);
	}
	else if (prev_e > xe) {
		w = prev_e - xe;
		x = _pos_to_coord_x(drw, xe);
		_uncurtain_h(drw, x, w);
	}

	if (prev_s < xs) {						// start stick is moved to right
		w = xs - prev_s;
		x = _pos_to_coord_x(drw, prev_s);
		_uncurtain_h(drw, x, w);
	}
	else if (prev_s > xs) {
		w = prev_s - xs;		
		x = _pos_to_coord_x(drw, xs);		
		_curtain_h(drw, x, w);
	}

	drw->prev_pstart = start;
	drw->prev_pend = end;
	return 0;
}

static int _draw_section_v(TML_DRW_T *drw, GdkPixbuf *pb_ctn, POS start, POS end)
{
	RPOS ye, ys, prev_s, prev_e;
	int h;
	int y;

	if (start == end) {						// section is not exist
		drw->prev_pstart = start;
		drw->prev_pend = end;
	}

	ye = (drw->rc_rba.height - 1) - end;
	ys = (drw->rc_rba.height - 1) - start;
	prev_s = (drw->rc_rba.height - 1) - drw->prev_pstart;
	prev_e = (drw->rc_rba.height - 1) -drw->prev_pend;

	if (prev_s < ye) prev_s = ye;			// exeption
	
	if (prev_e < ye) {						// end stick is moved to down
		h = ye - prev_e;
		y = _rpos_to_coord_y(drw, prev_e);		
		_uncurtain_v(drw, y, h);
	}
	else if (prev_e > ye) {
		h = prev_e - ye;
		y = _rpos_to_coord_y(drw, ye);
		_curtain_v(drw, y, h);
	}

	if (prev_s < ys) {						// start stick is moved to down
		h = ys - prev_s;
		y = _rpos_to_coord_y(drw, prev_s);
		_curtain_v(drw, y, h);
	}
	else if (prev_s > ys) {
		h = prev_s - ys;		
		y = _rpos_to_coord_y(drw, ys);		
		_uncurtain_v(drw, y, h);
	}

	drw->prev_pstart = start;
	drw->prev_pend = end;
	return 0;
}

static int _print_section_h(TML_DRW_T *drw, GdkPixbuf *pb_ctn, POS start, POS end)
{
	int x = _pos_to_coord_x(drw, start);
	_curtain_print_h(drw, x, end - start);
	drw->prev_pstart = start;
	drw->prev_pend = end;
	return 0;
}

static int _print_section_v(TML_DRW_T *drw, GdkPixbuf *pb_ctn, POS start, POS end)
{
	int y = _rpos_to_coord_y(drw, (drw->rc_rba.height - 1) - end);
	_curtain_print_v(drw, y, end - start);
	drw->prev_pstart = start;
	drw->prev_pend = end;
	return 0;
}

static int _hide_section_h(TML_DRW_T *drw)
{
	int x = _pos_to_coord_x(drw, drw->prev_pstart);
	_uncurtain_h(drw, x, drw->prev_pend - drw->prev_pstart);
	return 0;
}

static int _hide_section_v(TML_DRW_T *drw)
{
	int y = _rpos_to_coord_y(drw, (drw->rc_rba.height - 1) - drw->prev_pend);
	_uncurtain_v(drw, y, drw->prev_pend - drw->prev_pstart);
	return 0;
}

static int _compose_curtain_img(TML_DRW_T *drw, GdkPixbuf *pb_ctn)
{
	gdk_draw_pixmap(drw->pm_comp, drw->gc, drw->pm_cnvs,
			drw->rc_rba.x, drw->rc_rba.y,
			drw->rc_rba.x, drw->rc_rba.y, drw->rc_rba.width, drw->rc_rba.height);

	gdk_draw_pixbuf(drw->pm_comp, drw->gc, pb_ctn,
			0, 0, 
			drw->rc_rba.x, drw->rc_rba.y,
			drw->rc_rba.width, drw->rc_rba.height,
			GDK_RGB_DITHER_NONE, 0, 0);
}

static int _print_canvus_h(TML_DRW_T *drw)
{
	gdk_draw_pixmap(drw->pm_blit, drw->gc, drw->pm_cnvs,
			drw->rc_ruler.x, drw->rc_ruler.y,
			drw->rc_ruler.x, drw->rc_ruler.y,
			drw->rc_ruler.width,
			drw->rc_ruler.height + drw->rc_rba.height);
	return 0;
}

static int _print_canvus_v(TML_DRW_T *drw)
{
	gdk_draw_pixmap(drw->pm_blit, drw->gc, drw->pm_cnvs,
			drw->rc_ruler.x, drw->rc_ruler.y,
			drw->rc_ruler.x, drw->rc_ruler.y,
			drw->rc_ruler.width + drw->rc_rba.width,
			drw->rc_ruler.height);
	return 0;
}

static int _blit_h(TML_DRW_T *drw)
{
	gdk_draw_pixmap(drw->drawable, drw->gc, drw->pm_blit,
			drw->rc_ruler.x, drw->rc_ruler.y,
			drw->rc_ruler.x, drw->rc_ruler.y,
			drw->rc_ruler.width,
			drw->rc_ruler.height + drw->rc_rba.height);
	return 0;
}

static int _blit_v(TML_DRW_T *drw)
{
	gdk_draw_pixmap(drw->drawable, drw->gc, drw->pm_blit,
			drw->rc_ruler.x, drw->rc_ruler.y,
			drw->rc_ruler.x, drw->rc_ruler.y,
			drw->rc_ruler.width + drw->rc_rba.width,
			drw->rc_ruler.height);
	return 0;
}

////////////////////////////////////////////////////////////
//
// protected interfaces
//

int _tml_print_area(TML_DRW_T *drw, TML_FIGURE_T *figure, RLIST *rlist[])
{
	int i;
	_draw_ruler[figure->type](drw, figure);

	for (i = 0; i < drw->ch_cnt; ++i) 
		_draw_rbar[figure->type](drw, figure, i, rlist[i]);

	_print_canvus[figure->type](drw);
	_compose_curtain_img(drw, figure->pb_curtain);
	return 0;
}

int _tml_blit(TML_DRW_T *drw, TML_FIGURE_T *figure)
{
	_blit[figure->type](drw);
	return 0;
}

int _tml_drawback_play_stick(TML_DRW_T *drw, TML_FIGURE_T *figure)
{
	_drawback_stick[figure->type](drw, &drw->prev_play);
	return 0;
}

int _tml_drawback_cti(TML_DRW_T *drw, TML_FIGURE_T *figure)
{
	_drawback_stick[figure->type](drw, &drw->prev_cti);
	return 0;
}

int _tml_draw_play_stick(TML_DRW_T *drw, TML_FIGURE_T *figure, POS pos)
{
	if (pos < 0) return -1;
	if (pos > drw->px_len - 1) return -1;
	_draw_stick[figure->type](drw, &drw->prev_play, figure->pb_stk_play, pos, 0);
	return 0;
}

int _tml_draw_cti(TML_DRW_T *drw, TML_FIGURE_T *figure, POS pos)
{
	if (pos < 0) return -1;
	if (pos > drw->px_len - 1) return -1;
	_draw_stick[figure->type](drw, &drw->prev_cti, figure->pb_stk_cti, pos, 1);
	return 0;
}

int _tml_print_play_stick(TML_DRW_T *drw, TML_FIGURE_T *figure, POS pos)
{
	if (pos < 0) return -1;
	if (pos > drw->px_len - 1) return -1;
	_print_stick[figure->type](drw, &drw->prev_play, figure->pb_stk_play, pos, 0);
	return 0;
}

int _tml_print_cti(TML_DRW_T *drw, TML_FIGURE_T *figure, POS pos)
{
	if (pos < 0) return -1;
	if (pos > drw->px_len - 1) return -1;
	_print_stick[figure->type](drw, &drw->prev_cti, figure->pb_stk_cti, pos, 1);
	return 0;
}

int _tml_printback_play_stick(TML_DRW_T *drw, TML_FIGURE_T *figure)
{
	_drawback_stick[figure->type](drw, &drw->prev_play);
	return 0;
}

int _tml_printback_cti(TML_DRW_T *drw, TML_FIGURE_T *figure)
{
	_drawback_stick[figure->type](drw, &drw->prev_cti);
	return 0;
}

int _tml_draw_start_stick(TML_DRW_T *drw, TML_FIGURE_T *figure, POS pos)
{
	if (pos < 0) return -1;
	if (pos > drw->px_len - 1) return -1;
//	_draw_stick[figure->type](drw, &drw->prev_start, figure->pb_stk_start, pos);
	return 0;
}

int _tml_draw_end_stick(TML_DRW_T *drw, TML_FIGURE_T *figure, POS pos)
{
	if (pos < 0) return -1;
	if (pos > drw->px_len - 1) return -1;
//	_draw_stick[figure->type](drw, &drw->prev_end, figure->pb_stk_end, pos);
	return 0;
}

int _tml_hide_play_stick(TML_DRW_T *drw, TML_FIGURE_T *figure)
{
	_hide_stick[figure->type](drw, &drw->prev_play);
	return 0;
}

int _tml_hide_cti(TML_DRW_T *drw, TML_FIGURE_T *figure)
{
	_hide_stick[figure->type](drw, &drw->prev_cti);
	return 0;
}

int _tml_hide_start_stick(TML_DRW_T *drw, TML_FIGURE_T *figure)
{
//	_hide_stick[figure->type](drw, &drw->prev_start);
	return 0;
}

int _tml_hide_end_stick(TML_DRW_T *drw, TML_FIGURE_T *figure)
{
//	_hide_stick[figure->type](drw, &drw->prev_end);
	return 0;
}

int _tml_draw_section(TML_DRW_T *drw, TML_FIGURE_T *figure, POS start, POS end)
{
	if (start < 0) start = 0; 
	if (end < 0) end = 0;
	if (start > drw->px_len - 1) start = drw->px_len - 1;
	if (end > drw->px_len - 1) end = drw->px_len - 1;
	_draw_section[figure->type](drw, figure->pb_curtain, start, end);
	return 0;
}

int _tml_print_section(TML_DRW_T *drw, TML_FIGURE_T *figure, POS start, POS end)
{
	if (start < 0) start = 0; 
	if (end < 0) end = 0;
	if (start > drw->px_len - 1) start = drw->px_len - 1;
	if (end > drw->px_len - 1) end = drw->px_len - 1;
	_print_section[figure->type](drw, figure->pb_curtain, start, end);
	return 0;
}

int _tml_hide_section(TML_DRW_T *drw, TML_FIGURE_T *figure)
{
	_hide_section[figure->type](drw);
	return 0;
}

int _tml_compose_curtain_img(TML_DRW_T *drw, TML_FIGURE_T *figure)
{
	_compose_curtain_img(drw, figure->pb_curtain);
	return 0;
}

int _tml_init_canvas(TML_DRW_T *drw, TML_FIGURE_T *figure)
{
	_draw_bar(drw->pm_cnvs, drw->gc,
			drw->rc_rba.x, drw->rc_rba.y, 
			drw->rc_rba.width, drw->rc_rba.height);

	_draw_bar(drw->pm_cnvs, drw->gc,
			drw->rc_ruler.x, drw->rc_ruler.y, 
			drw->rc_ruler.width, drw->rc_ruler.height);
}

int _tml_invalidate(TML_DRW_T *drw)
{
	gdk_window_invalidate_rect(drw->drawable, 0, FALSE);	
	return 0;
}


