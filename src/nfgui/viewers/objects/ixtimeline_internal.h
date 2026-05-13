/*
 * ixtimeline_internal.h
 * 	- timeline internal header file
 *	- dependency :
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Apr 10, 2011
 *
 */

#ifndef __IXTIMELINE_INTERNAL_H
#define __IXTIMELINE_INTERNAL_H

#include "ixtimeline.h"

////////////////////////////////////////////////////////////
//
// protected data type
//

#define INTERACED	1

typedef GList		RLIST;

typedef int			POS;		// offset from the base, not window coordination
typedef POS 		RPOS;		// offset from the edge

typedef struct _SZ_RBAR_T {
	char			reason;		// record reason
	char			intr;
	char			reserved[2];
	short		 	px_s;		// start
	short 			px_e;		// end
} SZ_RBAR_T;

typedef struct _SCALE_MARK_T {
	int		 		cnt;
	POS				pos_mark[24];
	char			txt_mark[24][8];
	char			txt_date[24][8];
} SCALE_MARK_T;

typedef struct _STICK_INFO {
	int				x;
	int				y;
	int				w;
	int				h;
} STICK_INFO;

typedef struct _TML_DRW_T {			// drawing information
	GdkDrawable 	*drawable;
	GdkRectangle	rc_ruler;		// window coordination
	GdkRectangle	rc_rba;			// window coordination
	SCALE_MARK_T	sm;
	int				px_len;
	int				w;
	int				h;
	int				ch_cnt;
	int				ch_brd;
	POS				pcur;
	POS				prev_pstart;
	POS				prev_pend;
	GdkGC			*gc;
	GdkGC			*gc_ruler_tx;
	GdkGC			*gc_date;
	GdkGC			*gc_playbar;
	GdkGC			*gc_cti;
	GdkGC			*gc_ruler_bg;
	GdkGC			*gc_rbar[MAX_RECTYPE];
	STICK_INFO		prev_cti;
	STICK_INFO		prev_play;
	STICK_INFO		prev_start;
	STICK_INFO		prev_end;
	STICK_INFO		prev_drag;
	GdkPixmap		*pm_comp;
	GdkPixmap		*pm_cnvs;
	GdkPixmap		*pm_blit;
} TML_DRW_T;

typedef enum _QUERY_TYPE_E {
	NORMAL		= 0,
	SHIFT	 	= 1,
	INTERACING	= 2,
} QUERY_TYPE_E;

typedef struct _TML_QRRESULT_T {
	int				mag;
	time_t 			org_tbase;
	int				org_spp;
	int				org_count;
	time_t			prev_tbase;		// previous base time
	int				prev_spp;
	int				prev_count;
} TML_QRRESULT_T;

typedef struct _TML_QRYINFO_T {
	QUERY_TYPE_E	qt;
	int				intr_step;
	int				shift;
	int 			ch;
	time_t 			base;
	int 			spp;
	int 			count;
	int 			ch_cnt;
	RLIST 			**rlist;
	TML_QRRESULT_T	*qrt;
} TML_QRYINFO_T;

#if 0
typedef struct _TML_QUERY_T {

	QUERY_TYPE_E	qt;
	int				intr_step;
	int				shift;

	// don't set below data when called _tml_get_data
	int				mag;
	time_t 			org_tbase;
	int				org_spp;
	int				org_count;
	time_t			prev_tbase;		// previous base time
	int				prev_spp;
	int				prev_count;
} TML_QUERY_T;
#endif


// Data

int _tml_get_data(TML_QRYINFO_T *qi);
int _tml_free_data(int ch_cnt, RLIST *rlist[]);
int _tml_scan_data(RLIST *rlist[], int ch, POS pos);


// Draw

int _tml_print_area(TML_DRW_T *drw, TML_FIGURE_T *figure, RLIST *rlist[]);
int _tml_blit(TML_DRW_T *drw, TML_FIGURE_T *figure);
int _tml_drawback_play_stick(TML_DRW_T *drw, TML_FIGURE_T *figure);
int _tml_drawback_cti(TML_DRW_T *drw, TML_FIGURE_T *figure);
int _tml_draw_play_stick(TML_DRW_T *drw, TML_FIGURE_T *figure, POS pos);
int _tml_draw_cti(TML_DRW_T *drw, TML_FIGURE_T *figure, POS pos);
int _tml_print_play_stick(TML_DRW_T *drw, TML_FIGURE_T *figure, POS pos);
int _tml_print_cti(TML_DRW_T *drw, TML_FIGURE_T *figure, POS pos);
int _tml_printback_play_stick(TML_DRW_T *drw, TML_FIGURE_T *figure);
int _tml_printback_cti(TML_DRW_T *drw, TML_FIGURE_T *figure);
int _tml_draw_start_stick(TML_DRW_T *drw, TML_FIGURE_T *figure, POS pos);
int _tml_draw_end_stick(TML_DRW_T *drw, TML_FIGURE_T *figure, POS pos);
int _tml_hide_play_stick(TML_DRW_T *drw, TML_FIGURE_T *figure);
int _tml_hide_cti(TML_DRW_T *drw, TML_FIGURE_T *figure);
int _tml_hide_start_stick(TML_DRW_T *drw, TML_FIGURE_T *figure);
int _tml_hide_end_stick(TML_DRW_T *drw, TML_FIGURE_T *figure);
int _tml_draw_section(TML_DRW_T *drw, TML_FIGURE_T *figure, POS start, POS end);
int _tml_print_section(TML_DRW_T *drw, TML_FIGURE_T *figure, POS start, POS end);
int _tml_hide_section(TML_DRW_T *drw, TML_FIGURE_T *figure);
int _tml_compose_curtain_img(TML_DRW_T *drw, TML_FIGURE_T *figure);
int _tml_init_canvas(TML_DRW_T *drw, TML_FIGURE_T *figure);
int _tml_invalidate(TML_DRW_T *drw);




#endif
