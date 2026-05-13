/*
 * ixtimeline.h
 * 	- timeline widget
 *	- dependencies :
 *		
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Apr 7, 2011
 *
 */

#ifndef __IXTIMELINE_H
#define __IXTIMELINE_H

#include <gtk/gtk.h>
#include "ix_func.h"
#include "nfobject.h"

////////////////////////////////////////////////////////////
//
// public data type
//

typedef enum _REC_TYPE_E {
	REC_UNKNOWN		= -1,
	REC_NOTHING		= 0,
	REC_TIMER		= 1,
	REC_ALARM		= 2,
	REC_MOTION		= 3,
	REC_USEREVT		= 4,
	REC_PANIC		= 5,
	REC_PREREC		= 6,
	REC_NOTHING_I	= 7,
	REC_TIMER_I		= 8,
	REC_ALARM_I		= 9,
	REC_MOTION_I	= 10,
	REC_USEREVT_I	= 11,
	REC_PANIC_I		= 12,
	REC_PREREC_I	= 13,
	MAX_RECTYPE,
} REC_TYPE_E;

typedef enum _TML_STYLE_E {
	TML_NOSECTION		= 0x01,
	TML_PLAYDRGABLE		= 0x02,
	TML_NODRAGABLE		= 0X04,
	TML_GHOSTPLAYSTICK	= 0X08,
	TML_GHOSTCTI		= 0X10,
} TML_STYLE_E;

typedef enum _TML_TYPE_E {
	TML_HORIZONTAL 	= 0,
	TML_VERTICAL
} TML_TYPE_E;

typedef enum _TML_LENGTH_E {
	LEN_640		= 0,
	LEN_675,
	LEN_720,
	LEN_800,
	LEN_864,
	LEN_900,
	LEN_960,
	LEN_1080,
	LEN_1152,
	LEN_1200,
	LEN_1350,
	LEN_1440,
	LEN_1600,
	LEN_1728,
	LEN_1800,
	LEN_1920,
} TML_LENGTH_E;

typedef time_t (*GET_EX_TIME_PROC)(void);

typedef struct _TML_FIGURE_T {
	TML_TYPE_E		type;
	TML_LENGTH_E 	len;
	GdkColor		cr_bg;
	GdkColor		cr_rbar[MAX_RECTYPE];		// reference to the above enumeration value
	GdkColor		cr_section;
	GdkColor		cr_balloon_bg;
	GdkColor		cr_balloon_fg;
	GdkColor		cr_ruler_text;
	GdkColor		cr_ruler_bg;
	GdkColor		cr_date;
	GdkColor		cr_playbar;
	GdkColor		cr_cti;
	int				ruler_brd;				// breadth of ruler,
	int				ch_brd;					// breadth of each channel
	int 			font_size;
	char 			font_name[64];
	FM_DATE_E		fm_date;
	FM_TIME_E		fm_time;
	GdkPixbuf		*pb_stk_cti;			// cti
	GdkPixbuf		*pb_stk_play;			// play time
	GdkPixbuf		*pb_stk_start;			// start of section
	GdkPixbuf		*pb_stk_end;			// end of section
	GdkPixbuf		*pb_stk_drag;			// drag img to start of section
	GdkPixbuf		*pb_curtain;;			// selecting curtain

} TML_FIGURE_T;

typedef enum _TML_UPDATE_MODE_E {
	TML_MANUAL_UPDATE		= 0,
	TML_AUTO_UPDATE			= 1,
} TML_UPDATE_MODE_E;


typedef struct _IXTIMELINE IXTIMELINE;



////////////////////////////////////////////////////////////
//
// public interfaces
//

IXTIMELINE *tml_new(NFOBJECT *parent);
int tml_init(IXTIMELINE *obj, TML_FIGURE_T *figure, int x, int y, int w, int h, int ch_cnt, int ch);
int tml_destroy(IXTIMELINE *obj);
int tml_set_update_mode(IXTIMELINE *obj, TML_UPDATE_MODE_E mode);
int tml_set_update_interval(IXTIMELINE *obj, unsigned int interval);
int tml_set_external_time_proc(IXTIMELINE *obj, GET_EX_TIME_PROC proc);
int tml_refresh(IXTIMELINE *obj);
int tml_update(IXTIMELINE *obj);
int tml_repaint(IXTIMELINE *obj);
int tml_zoom_max(IXTIMELINE *obj);
int tml_zoom_min(IXTIMELINE *obj);
int tml_zoom_in(IXTIMELINE *obj);
int tml_zoom_out(IXTIMELINE *obj);
int tml_go_back(IXTIMELINE *obj);
int tml_go_ahead(IXTIMELINE *obj);
int tml_shift(IXTIMELINE *obj, int fract);
int tml_slide_down(IXTIMELINE *obj);
int tml_slide_up(IXTIMELINE *obj);
int tml_slide(IXTIMELINE *obj, int ofs_time);
int tml_get_cur_time(IXTIMELINE *obj, GTimeVal *cur);
time_t tml_get_cur_time_t(IXTIMELINE *obj);
int tml_get_section_time(IXTIMELINE *obj, GTimeVal *start, GTimeVal *end);
int tml_get_spp(IXTIMELINE *obj);
int tml_reset_cur_day(IXTIMELINE *obj, time_t timet);
int tml_change_channel(IXTIMELINE *obj, int ch);
int tml_reset_style(IXTIMELINE *obj, TML_STYLE_E style);
TML_STYLE_E tml_get_style(IXTIMELINE *obj);
REC_TYPE_E tml_get_rec_type(IXTIMELINE *obj, int ch, time_t when);
time_t tml_get_base_time_t(IXTIMELINE *obj);
time_t tml_get_edge_time_t(IXTIMELINE *obj);
int tml_change_cur_time(IXTIMELINE *obj, GTimeVal *cur);
int tml_change_cur_time_t(IXTIMELINE *obj, time_t cur);
int tml_change_cur_day(IXTIMELINE *obj, time_t timet);
int tml_set_section_time(IXTIMELINE *obj, GTimeVal *start, GTimeVal *end);
int tml_set_section_time_t(IXTIMELINE *obj, time_t start, time_t end);
int tml_set_start_time(IXTIMELINE *obj, GTimeVal *start);
int tml_set_end_time(IXTIMELINE *obj, GTimeVal *end);
int tml_set_start_time_t(IXTIMELINE *obj, time_t start);
int tml_set_end_time_t(IXTIMELINE *obj, time_t end);
int tml_set_cti_position(IXTIMELINE *obj, int pos);	// -1 : edge
int tml_set_scale_depth(IXTIMELINE *obj, int depth);
int tml_set_style(IXTIMELINE *obj, TML_STYLE_E style);
int tml_set_playstick_time_t(IXTIMELINE *obj, time_t tplay);
int tml_set_manual_paint_mode(IXTIMELINE *obj, int onoff);
int tml_get_play_pos(IXTIMELINE *obj);
int tml_is_set_section(IXTIMELINE *obj);

#endif
