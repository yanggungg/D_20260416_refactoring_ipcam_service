/*
 * ixtimeline.c
 * 	- timeline widget
 *	- dependencies :
 *		
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Apr 7, 2011
 *
 */

#include "ixtimeline.h"
#include "iux_afx.h"
#include "nfobject.h"
#include "nfwindow.h"
#include "ixtimeline_internal.h"
#include "ix_func.h"
#include "support/nf_ui_color.h"
#include "support/event_loop.h"
#include "iux_msg.h"
#include "ix_mem.h"
#include "cmm.h"
#include "uxm.h"
#include "evt.h"
#include "var.h"
#include "ix_conf.h"


DECLARE DBG_SYSTEM

#define DBG_LEVEL		DBG_CONF
#define DBG_MODULE		"TML"

#define LBUTTON		1
#define RBUTTON		3

#define ADJ_AREA_SIZE	5
#define _HAS_NO_SECTION(obj)		(!(obj->st_sec & STA_SET))
#define _IS_VERTICAL_TYPE(obj)		(obj->figure.type == TML_VERTICAL)
#define _IS_NOSECTION(obj)			(obj->style & TML_NOSECTION)
#define _IS_PLAYDRGABLE(obj)		(obj->style & TML_PLAYDRGABLE)
#define _IS_NODRAGABLE(obj)			(obj->style & TML_NODRAGABLE)
#define _IS_GHOSTPLAYSTICK(obj)		(obj->style & TML_GHOSTPLAYSTICK)
#define _IS_GHOSTCTI(obj)			(obj->style & TML_GHOSTCTI)
#define _NEED_TO_BGND_GET(obj)		(obj->bgnd_get)

#define RDATA_LOCK()		g_mutex_lock(obj->mtx_rdata)
#define RDATA_UNLOCK()		g_mutex_unlock(obj->mtx_rdata)
#define RDATA_TRY_LOCK()	g_mutex_trylock(obj->mtx_rdata)


/*
 * all time values are presented in UTC since 1970 and kept by time_t.
 *
 
                ruler	  rbar
               
                  |       |
                  |       |
                  v       v
  
   ---                ----------------------- 
	^			13:00 |   |                 |] <------- edge_time (tedge, pedge)
	|				  |   |                 |
	|				  |   |                 |
	|			12:00 |   |                 |
	|				  |   |                 |
	|				  |   |                 |
	|			11:00 |   |                 |
	|				  |   |                 |
	|				  |   |  |              |
	|				  |      |              |
	|				  |      |              |
	|				  |      |              |
	|				  |      |              |
	|                 |>-------------------<|  <-+----- cur time  ( tcur, pcur)
	|				  |      |              |    |
	|				  |      |              |    +----- time stick or stick
	|				  |                     |    |
	|				  |                     |    ------ or play time
	|				  |                     |
	|				  |                     |
	|				  |                     |
	|				  |                     |
	|				  |                     |
	|length 		  |                     |
	|				  |                     |
	|				  |o--------------------|  <------- start time (vertical mode)
	|				  |                     |
	|				  |                     |
	|				  |                     |
	|				  |                     |
	|				  |                     |
	|				  |o--------------------|  <------- end time (vertical mode)
	|				  |                     |
	|				  |                     |
	|				  |                     |
	|				  |                     |
	|				  |                     |
	|				  |                     |
	|				  |                     |
	|				  |                     |
	|				  |                     |
	|				  |                     |
	|				  |                     |
	|				  |                     |
	|				  |                     |
	|				  |                     |
	|				  |                     |
	|				  |                     |
	v				  |                     |] <------ base_time	(tbase, pbase)
   ---                ----------------------- 
                       \___________________/
                               
							     ^
								 |
								 |

							    rba
*/



/*
 * naming convention
 * 
 *  rba : record bar area
 *	rbar : record bar
 *	type : vertical or horizontal 
 *	spp	 : sec per pixel
 *	section : selected area by user
 *	cti : current time indicator
 *	scale up : zoom in
 *	scale down : zoom out
 *	toff : time offset
 *	pos(position) : distance from the base
 *
 */


////////////////////////////////////////////////////////////
//
// private data type
//

#define CNT_LENGTH (LEN_1920 + 1)
#define CNT_SCALE	13
#define MAX_CH		32


typedef enum _WORK_STAT_E {
	READY	= 0,
	STOP	= 1,
	RUN		= 2,
	PAUSE	= 3,
} WORK_STAT_E;

typedef enum _STOP_STAT_E {
	NOTYET	= 0,
	YES		= 1,
} STOP_STAT_E;

typedef enum _STATE_E {
	STA_NOTSET		= 0x00,
	STA_SET			= 0X01,
	STA_SETTING		= 0x02,
	STA_DRAGGING	= 0x04,
} STATE_E;

typedef enum _CLICK_POINT_E {
	OUTSIDE_OF_RBA,
	NO_SECTION,
	INSIDE_OF_SECTION,
	OUTSIDE_OF_START,
	OUTSIDE_OF_END,
	OVER_THE_START,
	OVER_THE_END,
} CLICK_POINT_E;

typedef enum _SEC_UPDATE_E {
	UPDATE_START	= 0x01,
	UPDATE_END		= 0x02,
	UPDATE_BOTH		= 0x03,
	UPDATE_PLAY		= 0x04,
} SEC_UPDATE_E;

struct _IXTIMELINE {
	NFOBJECT object;

	int					init_tml;

	TML_LENGTH_E		len;
	TML_STYLE_E			style;
	int					px_len;
	int					ch_cnt;
	int					ch;

	TML_FIGURE_T		figure;	
	TML_DRW_T			drw;

	time_t				tcur;		// time : cur
	time_t				tplay;		// play time
	time_t 				tbase;
	time_t				tedge;

	time_t				prev_tcur;	// time : previous cur

	time_t				tstart;		// start of section
	time_t				tend;		// end of section

	POS					pcur;		// position : cur
	POS					pplay;		// position : play
	POS					pstart;		// position : start of section
	POS					pend;		// position : end of section
	
	int					idx_spp;	// index of current spp
	int					cnt_valid_spp;
	int					spp_table[CNT_SCALE];		// used spp
	int					intv_table[CNT_SCALE];		// used interval 

	TML_UPDATE_MODE_E	update_mode;
	unsigned int		interval;
	GET_EX_TIME_PROC	get_extime_proc;

	guint				up_timer_id;		// update timer
	guint				nu_timer_id;		// normal update
	guint				as_timer_id;		// reserve auto-up set

	STATE_E				st_stk_play;
	STATE_E				st_stk_start;
	STATE_E				st_stk_end;
	STATE_E				st_sec;
	STATE_E				st_press;
	GdkRectangle		rc_sec;		// window coordination
	GdkPoint			sec_drg_org;
	TML_QRRESULT_T		qrt;

	RLIST				*rdata[MAX_CH];

	GMutex				*mtx_rdata;

	GThread				*thd;
	CMMPORT				cmmpt;
	WORK_STAT_E			work;
	STOP_STAT_E			stopped;
	int					sleep_time;
	int					bgnd_get;
//	int					fullup_cycle;
	time_t				last_norup;
	int					manual_paint;
};

////////////////////////////////////////////////////////////
//
// private variable
//


static int _px_len[CNT_LENGTH] = {
	640,
	675,
	720,
	800,
	864,
	900,
	960,
	1080,
	1152,
	1200,
	1350,
	1440,
	1600,
	1728,
	1800,
	1920
};

static int _intv_mark[CNT_SCALE] = {		// interval between mark
	3600,		// if 24h
	3600,		// 12h
	3600,		// 6h
	3600,		// 4h
	3600,		// 3h
	3600,		// 2h
	3600,		// 1h30m
	1800,		// 1h
	1800,		// 45m
	900,		// 40m
	900,		// 40m
	900,		// 12m30s
	900,		// 2m
};

static int _scale[CNT_SCALE] = {
	86400,		// 24h
	43200,		// 12h
	21600,		// 6h
	14400,		// 4h
	10800,		// 3h
	7200,		// 2h
	5400,		// 1h30m
	3600,		// 1h
	2700,		// 45m
	2400,		// 40m
	1800,		// 30m
	0,			// 12m30s
	0,			// prepared for native spp higher than 1350
};

////////////////////////////////////////////////////////////
//
// private functions 
//

static int _cleanup_thread(IXTIMELINE *obj)
{
	if (obj->cmmpt == 0) return -1;
    if (cmm_mount_off_thread(obj->cmmpt) == -1) return -1; 
    return 0;
}

static int _stop(IXTIMELINE *obj)
{
	obj->work = STOP;
	return 0;
}

static int _run(IXTIMELINE *obj)
{
	obj->stopped = NOTYET;
	obj->work = RUN;
	return 0;
}

static int _wait_run_signal(IXTIMELINE *obj)
{
	while (obj->work == READY) usleep(100000);
	return 0;
}

static int _wait_for_stop(IXTIMELINE *obj)
{
	while (obj->stopped == NOTYET) usleep(100000);
	return 0;
}

static int _is_paused(IXTIMELINE *obj)
{
	return (obj->work == PAUSE || obj->work == STOP);
}

static int _is_work(IXTIMELINE *obj)
{
	return (obj->work == RUN || obj->work == PAUSE);
}

static time_t _pixel_to_toff(IXTIMELINE *obj, int pixel)
{
	return obj->spp_table[obj->idx_spp] * pixel;
}

static time_t _pos_to_timet(IXTIMELINE *obj, POS pos)
{
	return obj->tbase + _pixel_to_toff(obj, pos); 
}

static POS _timet_to_pos(IXTIMELINE *obj, time_t timet)
{
	return (timet - obj->tbase) / obj->spp_table[obj->idx_spp];
}

static int _refill_scale_mark(IXTIMELINE *obj)
{
	int i = 0;
	int no_change = 1;
	time_t next;
	int hour, min, mon, day;
	int prev_hour;
	int intv = obj->intv_table[obj->idx_spp];
	
	memset(obj->drw.sm.txt_date, 0x00, sizeof(obj->drw.sm.txt_date));
	memset(obj->drw.sm.txt_mark, 0x00, sizeof(obj->drw.sm.txt_mark));

	next = ((obj->tbase + (intv - 1)) / intv) * intv;
	obj->drw.sm.pos_mark[i] = _timet_to_pos(obj, next);
	dtf_get_local_hourmin(next, &hour, &min, 0);
	dtf_get_local_day(next, 0, &mon, &day);
	sprintf(obj->drw.sm.txt_mark[i], "%02d:%02d", hour, min);
	sprintf(obj->drw.sm.txt_date[i], "%02d-%02d", mon, day);
	prev_hour = hour;

	++i;

	while (1) {
		next += intv;
		if (next > obj->tedge) break;
		if (i > 23) break;	// max 23
		obj->drw.sm.pos_mark[i] = _timet_to_pos(obj, next);
		dtf_get_local_hourmin(next, &hour, &min, 0);
		dtf_get_local_day(next, 0, &mon, &day);
		sprintf(obj->drw.sm.txt_mark[i], "%02d:%02d", hour, min);
		
		if (prev_hour == 23 && hour == 0) {
			sprintf(obj->drw.sm.txt_date[i], "%02d-%02d", mon, day);
			no_change = 0;
		}
		prev_hour = hour;

		++i;
	}

	if (no_change) sprintf(obj->drw.sm.txt_date[i - 1], "%02d-%02d", mon, day);
	obj->drw.sm.cnt = i;

	return 0;	
}

static time_t _toff_to_pixel(IXTIMELINE *obj, time_t toff)
{
	int ret;
	ret = toff / obj->spp_table[obj->idx_spp];
	return ret;	
}

static int _pos_to_coord_y(IXTIMELINE *obj, POS pos)
{
	return obj->drw.rc_rba.y + ((obj->drw.rc_rba.height - 1) - pos);
}

static int _pos_to_coord_x(IXTIMELINE *obj, POS pos)
{
	return obj->drw.rc_rba.x + pos;
}

static int _coord_to_pos(IXTIMELINE *obj, int x, int y, POS *pos)
{
	int wx, wy;

	nfui_nfobject_get_offset((NFOBJECT *)obj, &wx, &wy);
	if (_IS_VERTICAL_TYPE(obj)) *pos = (POS)(obj->px_len - (y - wy)) - 1;
	else *pos = (POS)(x - wx);
	return 0;
}

static gboolean _is_cti_visible(IXTIMELINE *obj)
{
	return (obj->pcur != -1);
}

static gboolean _is_valid_play_pos(IXTIMELINE *obj)
{
	return (obj->pplay != -1);
}

static gboolean _is_section_set(IXTIMELINE *obj)
{
	return obj->st_sec & STA_SET;
}

static int _get_origin(IXTIMELINE *obj, time_t *torg, POS *porg)
{
	if (_is_section_set(obj)) {
		*torg = obj->tstart + (obj->tend - obj->tstart + 1) / 2;	// + 1 for manipulation
		*porg = obj->pstart + (obj->pend - obj->pstart + 1) / 2;
	}
	else if (_is_valid_play_pos(obj)) {
		*torg = obj->tplay;
		*porg = obj->pplay;
	}
	else {
		*torg = obj->tcur;
		*porg = obj->pcur;
	}
	return 0;
}

static int _is_date_changed(IXTIMELINE *obj)
{
	struct tm old;
	struct tm new;

	localtime_r(&obj->prev_tcur, &old);
	localtime_r(&obj->tcur, &new);
	return (old.tm_mday != new.tm_mday);
}

static int _move_cur_time(IXTIMELINE *obj)
{
	int cspp = obj->spp_table[obj->idx_spp];
	obj->prev_tcur = obj->tcur;
	obj->tcur = obj->tbase + (obj->pcur * cspp);

	if (_is_date_changed(obj))
		evt_send_to_local(INFY_TML_DATE_CHANGED, obj->tcur, 0, obj);
	return 0;
}

static int _reset_scale_range(IXTIMELINE *obj)
{
	int cspp = obj->spp_table[obj->idx_spp];
	time_t torigin;
	POS porigin;

	_get_origin(obj, &torigin, &porigin);	
	obj->tbase = torigin - (porigin * cspp);
	obj->tedge = obj->tbase + (obj->px_len - 1) * cspp;

	obj->prev_tcur = obj->tcur;
	obj->tcur = obj->tbase + (obj->pcur * cspp);
	if (_is_date_changed(obj))
		evt_send_to_local(INFY_TML_DATE_CHANGED, obj->tcur, 0, obj);

	_refill_scale_mark(obj);
	return 0;
}

static int _shift_range(IXTIMELINE *obj, int fract)
{
	int cspp = obj->spp_table[obj->idx_spp];
	int shift;
	if (fract == 0) return 0;

	shift = ((obj->tedge - obj->tbase + cspp) / fract);
	obj->tbase += shift;
	obj->tedge += shift; 
	obj->tcur += shift; 
	obj->pcur = _timet_to_pos(obj, obj->tcur);
	if (_is_date_changed(obj)) 
		evt_send_to_local(INFY_TML_DATE_CHANGED, obj->tcur, 0, obj);

	_refill_scale_mark(obj);
	return 0;
}

static int _reset_range(IXTIMELINE *obj)
{
	int cspp = obj->spp_table[obj->idx_spp];
	time_t torigin = obj->tcur;
	POS porigin = obj->pcur;

	obj->tbase = torigin - (porigin * cspp);
	obj->tedge = obj->tbase + (obj->px_len - 1) * cspp;

	_refill_scale_mark(obj);
	return 0;
}

static int _move_range(IXTIMELINE *obj, time_t base, time_t cur)
{
	int cspp = obj->spp_table[obj->idx_spp];
	obj->tbase = base;
	obj->tedge = obj->tbase + (obj->px_len - 1) * cspp;
	obj->prev_tcur = obj->tcur;
	obj->tcur = cur;

	_refill_scale_mark(obj);
	return 0;
}

static gboolean _is_play_stick_out(IXTIMELINE *obj)
{
	return (obj->tbase > obj->tplay || obj->tedge < obj->tplay);
}

static gboolean _is_start_stick_out(IXTIMELINE *obj)
{
	return (obj->tbase > obj->tstart || obj->tedge < obj->tstart);
}

static gboolean _is_end_stick_out(IXTIMELINE *obj)
{
	return (obj->tbase > obj->tend || obj->tedge < obj->tend);
}

static int _reset_stick(IXTIMELINE *obj)
{
	if (_is_play_stick_out(obj)) obj->pplay = -1;
	else obj->pplay = _timet_to_pos(obj, obj->tplay);
	obj->pstart = _timet_to_pos(obj, obj->tstart);
	obj->pend = _timet_to_pos(obj, obj->tend);
	obj->pcur = _timet_to_pos(obj, obj->tcur);

	return 0;
}

static int _reset_section(IXTIMELINE *obj)
{
	if (_IS_VERTICAL_TYPE(obj)) {
		obj->rc_sec.y = _pos_to_coord_y(obj, obj->pend);
		obj->rc_sec.height = obj->pend - obj->pstart + 1;
	}
	else {
		obj->rc_sec.x = _pos_to_coord_x(obj, obj->pstart);
		obj->rc_sec.width = obj->pend - obj->pstart + 1;
	}
	return 0;
}

static int _set_cur_time(IXTIMELINE *obj, time_t cur)
{
	if (cur == 0) return -1;
	obj->prev_tcur = obj->tcur;
	obj->tcur = cur;
	return 0;
}

static int _recalc_cur_time(IXTIMELINE *obj)
{
	time_t cur;
	cur = _pos_to_timet(obj, obj->pcur);
	_set_cur_time(obj, cur);
	_reset_range(obj);
	return 0;
}

static int _place_cti(IXTIMELINE *obj)
{
	if (obj->pcur < 0) obj->pcur = 0;
	if (obj->pcur > obj->px_len - 1) obj->pcur = obj->px_len - 1;
	return 0;
}

static int _scale_up_max(IXTIMELINE *obj)
{
	obj->idx_spp = obj->cnt_valid_spp - 1;
	_reset_scale_range(obj);
	_reset_stick(obj);
	_reset_section(obj);
	return 0;
}

static int _scale_down_min(IXTIMELINE *obj)
{
	obj->idx_spp = 0;
	_reset_scale_range(obj);
	_reset_stick(obj);
	_reset_section(obj);
	return 0;
}

static int _scale_up(IXTIMELINE *obj)
{
	if (obj->idx_spp + 1 == obj->cnt_valid_spp) return -1;
	++obj->idx_spp;
	_reset_scale_range(obj);
	_reset_stick(obj);
	_reset_section(obj);
	return 0;
}

static int _scale_down(IXTIMELINE *obj)
{
	if (obj->idx_spp - 1 == -1) return -1;
	--obj->idx_spp;
	_reset_scale_range(obj);
	_reset_stick(obj);
	_reset_section(obj);
	return 0;
}

static int _shift_cur_time(IXTIMELINE *obj, int how)		// offset : 1 / how
{
	int cspp = obj->spp_table[obj->idx_spp];
	if (how == 0) return 0;

	obj->prev_tcur = obj->tcur;
	obj->tcur = obj->tcur + ((obj->tedge - obj->tbase + cspp) / how);
	if (_is_date_changed(obj)) 
		evt_send_to_local(INFY_TML_DATE_CHANGED, obj->tcur, 0, obj);
	return 0;
}

static int _shift_cti(IXTIMELINE *obj, int how)
{
	obj->pcur += (obj->px_len / how);
	return 0;
}

static int _shift_cti_t(IXTIMELINE *obj, int offset)
{
	int cspp = obj->spp_table[obj->idx_spp];
	POS how = offset / cspp;
	obj->pcur -= how;
	return 0;
}

static int _repaint(IXTIMELINE *obj)
{
	nfui_signal_emit((NFOBJECT*)obj, GDK_EXPOSE, TRUE);
	return 0;
}

static TML_QRYINFO_T *_make_query_info(IXTIMELINE *obj, QUERY_TYPE_E qt, int shift, int spp) 
{
	TML_QRYINFO_T *info = imalloc(sizeof(TML_QRYINFO_T));

	info->qt = qt;
	info->ch = obj->ch;
	info->intr_step = 80;		// available only interacing mode
	info->shift = shift;
	info->base = obj->tbase;
	info->spp = spp;
	info->count = obj->px_len;
	info->ch_cnt = obj->ch_cnt;
	info->rlist = obj->rdata;
	info->qrt = &obj->qrt; 

	return info;
}

static int _req_data(IXTIMELINE *obj, TML_QRYINFO_T *info)
{
	cmm_send_message(obj->cmmpt, iREQ_TML_GET_DATA, 0, 1, info); 
	return 0;
}

static int _stamp_norup_time(IXTIMELINE *obj, TML_QRYINFO_T *info)
{
	if (info->qt != NORMAL) return -1;
	obj->last_norup = time(0);
	DMSG(9, "NORMAL UP:%u", obj->last_norup);
	return 0;
}

static int _query_data_on_bg(IXTIMELINE *obj, TML_QRYINFO_T *info)
{
	RDATA_LOCK();
	_tml_get_data(info);
	RDATA_UNLOCK();
	_stamp_norup_time(obj, info);

	if (nfui_nfobject_is_shown(obj))
		evt_send_to_local(IRPL_TML_GET_DATA, obj, 0, 0);
	return 0;
}

static int _get_data(IXTIMELINE *obj, QUERY_TYPE_E qt)
{
	TML_QRYINFO_T *info = NULL;
	int ret = -1;
	int spp = obj->spp_table[obj->idx_spp];
	int shift = (obj->tbase - obj->qrt.prev_tbase) / spp;
	info = _make_query_info(obj, qt, shift, spp);

	if (_NEED_TO_BGND_GET(obj)) {
		_req_data(obj, info);
//		obj->bgnd_get = 0;
	}
	else {
		RDATA_LOCK();
		ret = _tml_get_data(info);
		RDATA_UNLOCK();
		_stamp_norup_time(obj, info);
		ifree(info);
	}

	return ret;
}

static int _compose_rba(IXTIMELINE *obj)
{
	_tml_compose_curtain_img(&obj->drw, &obj->figure);
}

static int _core_refresh(IXTIMELINE *obj, QUERY_TYPE_E qt)
{
	int ret;
	ret = _get_data(obj, qt);
	if (ret == 0) _repaint(obj);
	return 0;
}

static int _refresh(IXTIMELINE *obj, QUERY_TYPE_E qt)
{
	int ret;
	if (obj->manual_paint) return 0;
	_core_refresh(obj, qt);
	return 0;
}

static int _is_auto_norup_time(IXTIMELINE *obj)
{
	int delta = (obj->interval / 1000) * 15;
	time_t cur = time(0);
//	return 1; // just for test
	if (cur < obj->last_norup + delta) return 0;
	DMSG(9, "IT'S TIME TO FULL UPDATE, (%u)\n", cur);
	return 1;
}

static gboolean _update_proc(void *data)
{
	IXTIMELINE *obj = (IXTIMELINE *)data;
	time_t newtime;

	if (obj->update_mode == TML_AUTO_UPDATE) {
		if (obj->get_extime_proc) newtime = obj->get_extime_proc();
		else newtime = time(0);

		if (newtime <= 0) {
			printf("ERROR: INVALID update time from VSM\n");
			return TRUE;
		}
		_set_cur_time(obj, newtime);
	}

	if (_is_date_changed(obj))
		evt_send_to_local(INFY_TML_DATE_CHANGED, obj->tcur, 0, obj);

	_reset_range(obj);
	_reset_stick(obj);
	_reset_section(obj);
//	if (++obj->fullup_cycle == 15) { _refresh(obj, NORMAL); obj->fullup_cycle = 0; }
	if (_is_auto_norup_time(obj)) _refresh(obj, NORMAL);
	else _refresh(obj, SHIFT);
	return TRUE;
}

static int _start_update_timer(IXTIMELINE *obj)
{
	if (obj->up_timer_id) return -1;
	//obj->up_timer_id = g_timeout_add(3*1000, _update_proc, obj);	// just for test
	obj->up_timer_id = g_timeout_add(obj->interval, _update_proc, obj);
	return 0;
}

static gboolean _ready_update_timer(void *data)
{
	IXTIMELINE *obj = (IXTIMELINE *)data;
	if (obj->update_mode == TML_AUTO_UPDATE) _start_update_timer(obj);
	_place_cti(obj);
	return FALSE;
}

static int _reserve_autoup_set(IXTIMELINE *obj)
{
	if (obj->as_timer_id) g_source_remove(obj->as_timer_id);
	obj->as_timer_id = g_timeout_add(15000, _ready_update_timer, obj);
	return 0;
}

static int _stop_autoup_timer(IXTIMELINE *obj)
{
	if (obj->up_timer_id) g_source_remove(obj->up_timer_id);
	obj->up_timer_id = 0;
	return;
}

static int _change_interval(IXTIMELINE *obj, unsigned int interval)
{
	int cspp = obj->spp_table[obj->idx_spp];
	_stop_autoup_timer(obj);
	if (interval == 0) obj->interval = cspp * 1000;
	else obj->interval = interval;
	_start_update_timer(obj);
}

static int _reset_update_timer(IXTIMELINE *obj)
{
	if (obj->update_mode == TML_AUTO_UPDATE) {
		_stop_autoup_timer(obj);
		_start_update_timer(obj);
	}
	return 0;
}

static int _init_spp(IXTIMELINE *obj)
{
	int i;
	int j;

	memset(obj->spp_table, 0xFF, sizeof(int) * CNT_SCALE);

	for (i = 0, j = 0; i < CNT_SCALE - 1; ++i) {
		if (_scale[i] && _scale[i] % obj->px_len == 0) {
			obj->spp_table[j] = _scale[i] / obj->px_len;
			obj->intv_table[j] = _intv_mark[i];
			++j;
		}
	}

	obj->idx_spp = 0;
	obj->cnt_valid_spp = j;
	//obj->spp_table[j] = 1;		// native spp at last 
	//obj->cnt_valid_spp = j + 1;	// including native spp

	return 0;
}

static int _set_cur_cti_pos(IXTIMELINE *obj, POS pos)
{
	obj->pcur = pos;
	obj->drw.pcur = pos;
	_recalc_cur_time(obj);
	return 0;
}

static int _init_rbar_buf(IXTIMELINE *obj)
{
	return 0;
}

static int _init_rect_ruler(IXTIMELINE *obj)
{
	int wx, wy;
	nfui_nfobject_get_offset((NFOBJECT *)obj, &wx, &wy);

	if (_IS_VERTICAL_TYPE(obj)) {
		obj->drw.rc_ruler.x = wx;
		obj->drw.rc_ruler.y = wy;
		obj->drw.rc_ruler.width = obj->figure.ruler_brd; 
		obj->drw.rc_ruler.height = obj->object.height;
	}
	else {
		obj->drw.rc_ruler.x = wx;
		obj->drw.rc_ruler.y = wy;
		obj->drw.rc_ruler.width = obj->object.width;
		obj->drw.rc_ruler.height = obj->figure.ruler_brd;
	}
	return 0;
}

static int _init_rect_rba(IXTIMELINE *obj)
{
	int wx, wy;
	nfui_nfobject_get_offset((NFOBJECT *)obj, &wx, &wy);

	if (_IS_VERTICAL_TYPE(obj)) {
		obj->drw.rc_rba.x = wx + obj->drw.rc_ruler.width;
		obj->drw.rc_rba.y = wy;
		obj->drw.rc_rba.width = obj->object.width - obj->drw.rc_ruler.width;
		obj->drw.rc_rba.height = obj->object.height;
	}
	else {
		obj->drw.rc_rba.x = wx;
		obj->drw.rc_rba.y = wy + obj->drw.rc_ruler.height;;
		obj->drw.rc_rba.width = obj->object.width;
		obj->drw.rc_rba.height = obj->object.height - obj->drw.rc_ruler.height;
	}
	return 0;
}

static int _cleanup_rdata(IXTIMELINE *obj)
{
	int ch = var_get_ch_count();
	RDATA_LOCK();
	_tml_free_data(ch, obj->rdata);
	RDATA_UNLOCK();
	return 0;
}

static int _init_drw(IXTIMELINE *obj)
{
	obj->drw.ch_cnt = obj->ch_cnt;
	obj->drw.w = obj->object.width;
	obj->drw.h = obj->object.height;
	obj->drw.px_len = obj->px_len;
	obj->drw.prev_pstart = -1;
	obj->drw.prev_pend = -1;
	obj->drw.ch_brd = obj->figure.ch_brd;

	return 0;
}

static int _stop_timer(IXTIMELINE *obj)
{
	if (obj->nu_timer_id) g_source_remove(obj->nu_timer_id);
	if (obj->up_timer_id) g_source_remove(obj->up_timer_id);
	if (obj->as_timer_id) g_source_remove(obj->as_timer_id);

	obj->nu_timer_id = 0;
	obj->up_timer_id = 0;
	obj->as_timer_id = 0;
	
	return 0;
}

static int _cleanup_gcs(IXTIMELINE *obj)
{
	int i;

	for (i = 0; i < MAX_RECTYPE; ++i) g_object_unref(obj->drw.gc_rbar[i]);
	g_object_unref(obj->drw.gc_cti);
	g_object_unref(obj->drw.gc_playbar);
	g_object_unref(obj->drw.gc_ruler_tx);
	g_object_unref(obj->drw.gc_ruler_bg);
	g_object_unref(obj->drw.gc_date);
	g_object_unref(obj->drw.gc);

	return 0;
}

static int _cleanup_drawables(IXTIMELINE *obj)
{
	g_object_unref(obj->drw.pm_comp);
	g_object_unref(obj->drw.pm_cnvs);
	g_object_unref(obj->drw.pm_blit);

	return 0;
}

static int _cnt = 0;
static int _on_delete(IXTIMELINE *obj, GdkEvent *event, gpointer data)
{
	DMSG(1, "DELETING = %p\n", obj);
	_stop(obj);
	_stop_timer(obj);

	_wait_for_stop(obj);
	_cleanup_rdata(obj);
	g_mutex_free(obj->mtx_rdata);
	uxm_unreg_imsg_event(obj, IRPL_TML_GET_DATA);

	_cleanup_gcs(obj);
	_cleanup_drawables(obj);
	return 0;
}

static int _reload_drawable(IXTIMELINE *obj)
{
	obj->drw.drawable = nfui_nfobject_get_window((NFOBJECT *)obj);
	return 0;
}

static int _restore_drawable(IXTIMELINE *obj)
{
	obj->drw.drawable = nfui_nfobject_get_drawable((NFOBJECT *)obj);
	return 0;
}

static int _init_gcs(IXTIMELINE *obj)
{
	int i;

	obj->drw.gc = gdk_gc_new(obj->drw.drawable);
	obj->drw.gc_ruler_tx = gdk_gc_new(obj->drw.drawable);
	obj->drw.gc_ruler_bg = gdk_gc_new(obj->drw.drawable);
	obj->drw.gc_date = gdk_gc_new(obj->drw.drawable);
	obj->drw.gc_playbar = gdk_gc_new(obj->drw.drawable);
	obj->drw.gc_cti = gdk_gc_new(obj->drw.drawable);

	gdk_gc_set_rgb_fg_color(obj->drw.gc, &obj->figure.cr_bg);
	gdk_gc_set_rgb_fg_color(obj->drw.gc_ruler_tx, &obj->figure.cr_ruler_text);
	gdk_gc_set_rgb_fg_color(obj->drw.gc_ruler_bg, &obj->figure.cr_ruler_bg);
	gdk_gc_set_rgb_fg_color(obj->drw.gc_date, &obj->figure.cr_date);
	gdk_gc_set_rgb_fg_color(obj->drw.gc_playbar, &obj->figure.cr_playbar);
	gdk_gc_set_rgb_fg_color(obj->drw.gc_cti, &obj->figure.cr_cti);

	for (i = 0; i < MAX_RECTYPE; ++i) {
		obj->drw.gc_rbar[i] = gdk_gc_new(obj->drw.drawable);
		gdk_gc_set_rgb_fg_color(obj->drw.gc_rbar[i], &obj->figure.cr_rbar[i]);
	}

	return 0;
}

static int _make_drawables(IXTIMELINE *obj)
{
	int play_w = 0, play_h = 0;
	int ssec_w = 0, ssec_h = 0;
	int esec_w = 0, esec_h = 0;

	if (obj->figure.pb_stk_cti) play_w = gdk_pixbuf_get_width(obj->figure.pb_stk_cti);
	if (obj->figure.pb_stk_cti) play_h = gdk_pixbuf_get_height(obj->figure.pb_stk_cti);
	if (obj->figure.pb_stk_play) play_w = gdk_pixbuf_get_width(obj->figure.pb_stk_play);
	if (obj->figure.pb_stk_play) play_h = gdk_pixbuf_get_height(obj->figure.pb_stk_play);
	if (obj->figure.pb_stk_start) ssec_w = gdk_pixbuf_get_width(obj->figure.pb_stk_start);
	if (obj->figure.pb_stk_start) ssec_h = gdk_pixbuf_get_height(obj->figure.pb_stk_start);
	if (obj->figure.pb_stk_end) esec_w = gdk_pixbuf_get_width(obj->figure.pb_stk_end);
	if (obj->figure.pb_stk_end) esec_h = gdk_pixbuf_get_height(obj->figure.pb_stk_end);

	obj->drw.prev_cti.w = play_w;
	obj->drw.prev_cti.h = play_h;
	obj->drw.prev_play.w = play_w;
	obj->drw.prev_play.h = play_h;
	obj->drw.prev_start.w = play_w;
	obj->drw.prev_start.h = play_h;
	obj->drw.prev_end.w = play_w;
	obj->drw.prev_end.h = play_h;

	obj->drw.pm_comp = gdk_pixmap_new(obj->drw.drawable, obj->drw.w, obj->drw.h, -1);
	obj->drw.pm_cnvs = gdk_pixmap_new(obj->drw.drawable, obj->drw.w, obj->drw.h, -1);
	obj->drw.pm_blit = gdk_pixmap_new(obj->drw.drawable, obj->drw.w, obj->drw.h, -1);

	_tml_init_canvas(&obj->drw, &obj->figure);

	return 0;
}

static void _realize_handler(GtkWidget *widget, gpointer data)
{
	IXTIMELINE *obj = (IXTIMELINE*)data;
	NFOBJECT *top = nfui_nfobject_get_top(obj);

	if (obj->init_tml) return 0;

	nfui_nfobject_get_size(top, &obj->drw.w, &obj->drw.h);
	obj->drw.drawable = nfui_nfobject_get_window((NFOBJECT *)obj);

	_init_gcs(obj);
	_make_drawables(obj);

	obj->init_tml = 1;
}

static int _on_expose(IXTIMELINE *obj, GdkEvent *event, gpointer data)
{
	if (!nfui_nfobject_get_window((NFOBJECT*)obj)) return -1;

	_realize_handler(0, (gpointer)obj);

	_reload_drawable(obj);
	if (RDATA_TRY_LOCK() == FALSE) { 
		_restore_drawable(obj);
		return -1;
	}
	_tml_print_area(&obj->drw, &obj->figure, obj->rdata);
	RDATA_UNLOCK();


	if (!_IS_GHOSTPLAYSTICK(obj))
		_tml_printback_play_stick(&obj->drw, &obj->figure);
	_tml_printback_cti(&obj->drw, &obj->figure);

	_tml_print_section(&obj->drw, &obj->figure, obj->pstart, obj->pend);

	if (!_IS_GHOSTPLAYSTICK(obj))
		_tml_print_play_stick(&obj->drw, &obj->figure, obj->pplay);
	if (!_IS_GHOSTCTI(obj))
		_tml_print_cti(&obj->drw, &obj->figure, obj->pcur);

	_tml_blit(&obj->drw, &obj->figure);		

	_restore_drawable(obj);
	return 0;
}

static int _on_query(IXTIMELINE *obj, GdkEvent *event, gpointer data)
{
	CMM_MESSAGE_T *pmsg = (CMM_MESSAGE_T *)data;
	IXTIMELINE *from = pmsg->param;
	if (from != obj) return;
	_repaint(obj);
	return 0;
}

static STATE_E _get_play_stick_state(IXTIMELINE *obj)
{
	return obj->st_stk_play;
}

static STATE_E _get_start_stick_state(IXTIMELINE *obj)
{
	return obj->st_stk_start;
}

static gboolean _is_play_stick_dragging(IXTIMELINE *obj)
{
	return (_get_play_stick_state(obj) & STA_DRAGGING) ;
}

static gboolean _is_start_stick_dragging(IXTIMELINE *obj)
{
	return (_get_start_stick_state(obj) & STA_DRAGGING) ;
}

static STATE_E _get_end_stick_state(IXTIMELINE *obj)
{
	return obj->st_stk_end;
}

static gboolean _is_end_stick_dragging(IXTIMELINE *obj)
{
	return (_get_end_stick_state(obj) & STA_DRAGGING) ;
}

static int _change_start_stick_state(IXTIMELINE *obj, STATE_E state)
{
	obj->st_stk_start = state;
	return 0;
}

static int _change_end_stick_state(IXTIMELINE *obj, STATE_E state)
{
	obj->st_stk_end = state;
	return 0;
}

static int _change_section_state(IXTIMELINE *obj, STATE_E state)
{
	obj->st_sec = state;
	return 0;
}

static STATE_E _get_section_state(IXTIMELINE *obj)
{
	return obj->st_sec;
}

static gboolean _is_section_dragging(IXTIMELINE *obj)
	{
	return (_get_section_state(obj) & STA_DRAGGING) ;
}

static int _clearbit_section_state(IXTIMELINE *obj, STATE_E bitmask)
{
	obj->st_sec &= ~bitmask;
	return 0;
}

static int _change_play_stick_state(IXTIMELINE *obj, STATE_E state)
{
	obj->st_stk_play = state;
	return 0;
}

static int _clearbit_play_stick_state(IXTIMELINE *obj, STATE_E bitmask)
{
	obj->st_stk_play &= ~bitmask;
	return 0;
}

static int _clearbit_start_stick_state(IXTIMELINE *obj, STATE_E bitmask)
{
	obj->st_stk_start &= ~bitmask;
	return 0;
}

static int _clearbit_end_stick_state(IXTIMELINE *obj, STATE_E bitmask)
{
	obj->st_stk_end &= ~bitmask;
	return 0;
}

static gboolean _is_pressed_in_sec(IXTIMELINE *obj, int x, int y)
{
	if (_get_section_state(obj) != STA_SET) return FALSE;
	return ifn_is_in_rect(&obj->rc_sec, x, y);
}

static gboolean _is_over_start_stick(IXTIMELINE *obj, int x, int y)
{
	POS pos;

	if (_IS_VERTICAL_TYPE(obj)) {
		_coord_to_pos(obj, x, y, &pos);
		if (obj->pstart >= (pos - ADJ_AREA_SIZE) &&
			obj->pstart <= (pos + ADJ_AREA_SIZE))
			return TRUE;
	}
	else {
		_coord_to_pos(obj, x, y, &pos);
		if (obj->pstart >= (pos - ADJ_AREA_SIZE) &&
			obj->pstart <= (pos + ADJ_AREA_SIZE))
			return TRUE;
	}
	return FALSE;
}

static gboolean _is_over_end_stick(IXTIMELINE *obj, int x, int y)
{
	POS pos;

	if (_IS_VERTICAL_TYPE(obj)) {
		_coord_to_pos(obj, x, y, &pos);
		if (obj->pend >= (pos - ADJ_AREA_SIZE) &&
			obj->pend <= (pos + ADJ_AREA_SIZE))
			return TRUE;
	}
	else {
		_coord_to_pos(obj, x, y, &pos);
		if (obj->pend >= (pos - ADJ_AREA_SIZE) &&
			obj->pend <= (pos + ADJ_AREA_SIZE))
			return TRUE;
	}
	return FALSE;
}

static int _show_cti_x(IXTIMELINE *obj)
{
	if (!_IS_GHOSTCTI(obj)) {
		_tml_drawback_cti(&obj->drw, &obj->figure);
		_tml_draw_cti(&obj->drw, &obj->figure, obj->pcur);
	}
	return 0;
}

static int _show_play_stick_x(IXTIMELINE *obj)
{
	if (!_IS_GHOSTPLAYSTICK(obj)) {
		_tml_drawback_play_stick(&obj->drw, &obj->figure);
		_tml_draw_play_stick(&obj->drw, &obj->figure, obj->pplay);
	}
	return 0;
}

static int _show_start_stick_x(IXTIMELINE *obj)
{
	_tml_draw_start_stick(&obj->drw, &obj->figure, obj->pstart);
	return 0;
}

static int _show_section(IXTIMELINE *obj)
{
	_tml_draw_section(&obj->drw, &obj->figure, obj->pstart, obj->pend);
	return 0;
}

static int _hide_start_stick(IXTIMELINE *obj)
{
	_tml_hide_start_stick(&obj->drw, &obj->figure);
	obj->pstart = -1;
	obj->tstart = 0;
	return 0;
}

static gboolean _is_start_stick_visible(IXTIMELINE *obj)
{
	return (obj->tstart != 0);
}

static int _show_end_stick_x(IXTIMELINE *obj)
{
	_tml_draw_end_stick(&obj->drw, &obj->figure, obj->pend);
	return 0;
}

static int _hide_end_stick(IXTIMELINE *obj)
{
	_tml_hide_end_stick(&obj->drw, &obj->figure);
	obj->pend= -1;
	obj->tend = 0;
	return 0;
}

static gboolean _is_end_stick_visible(IXTIMELINE *obj)
{
	return (obj->tend != -1);
}

static int _show_play_stick(IXTIMELINE *obj, int x, int y)
{
	POS pos;
	_coord_to_pos(obj, x, y, &pos);
	obj->pplay = pos;
	obj->tplay = _pos_to_timet(obj, obj->pplay);
	_show_play_stick_x(obj);
	return 0;
}

static int _hide_cti(IXTIMELINE *obj)
{
	obj->pcur = -1;
	obj->prev_tcur = obj->tcur;
	obj->tcur = 0;
	_tml_hide_cti(&obj->drw, &obj->figure);
	return 0;
}

static int _hide_play_stick(IXTIMELINE *obj)
{
	obj->pplay = -1;
	obj->tplay = 0;
	_tml_hide_play_stick(&obj->drw, &obj->figure);
	return 0;
}

static gboolean _is_pressed_in_rba(IXTIMELINE *obj, int x, int y)
{
	return ifn_is_in_rect(&obj->drw.rc_rba, x, y);
}

static CLICK_POINT_E _get_press_pt_type(IXTIMELINE *obj, int x, int y)
{
	if (!_is_pressed_in_rba(obj, x, y)) return OUTSIDE_OF_RBA;
	if (_HAS_NO_SECTION(obj)) return NO_SECTION; 

	if (_is_pressed_in_sec(obj, x, y)) {
		if (_is_over_start_stick(obj, x, y)) return OVER_THE_START;
		else if (_is_over_end_stick(obj, x, y)) return OVER_THE_END;
		else return INSIDE_OF_SECTION;
	}
	else {
		if (_IS_VERTICAL_TYPE(obj)) {
			// just for usability
			if (_is_over_start_stick(obj, x, y)) return OVER_THE_START;
			else if (_is_over_end_stick(obj, x, y)) return OVER_THE_END;

			if (obj->rc_sec.y > y) return OUTSIDE_OF_END;
			else return OUTSIDE_OF_START;
		}
		else {
			// just for usability
			if (_is_over_start_stick(obj, x, y)) return OVER_THE_START;
			else if (_is_over_end_stick(obj, x, y)) return OVER_THE_END;

			if (obj->rc_sec.x > x) return OUTSIDE_OF_START;
			else return OUTSIDE_OF_END;
		}
	}
	
	// must be return before this
	return NO_SECTION;
}

static int _hide_section(IXTIMELINE *obj)
{
	_hide_start_stick(obj);
	_hide_end_stick(obj);
	_tml_hide_section(&obj->drw, &obj->figure);
	return 0;
}

static int _save_org_position(IXTIMELINE *obj, int x, int y)
{
	obj->sec_drg_org.x = x;
	obj->sec_drg_org.y = y;
	return 0;
}

static int _copy_prev_info_to_start(IXTIMELINE *obj)
{
	obj->drw.prev_start.x = obj->drw.prev_play.x;
	obj->drw.prev_start.y = obj->drw.prev_play.y;
	return 0;
}

static int _copy_prev_info_to_end(IXTIMELINE *obj)
{
	obj->drw.prev_end.x = obj->drw.prev_play.x;
	obj->drw.prev_end.y = obj->drw.prev_play.y;
	return 0;
}

static int _init_cti(IXTIMELINE *obj)
{
	obj->drw.prev_cti.x = -1;
	obj->drw.prev_cti.y = -1;
	obj->pcur = -1;
	obj->tcur = 0;
	return 0;
}

static int _init_play_stick(IXTIMELINE *obj)
{
	obj->drw.prev_play.x = -1;
	obj->drw.prev_play.y = -1;
	obj->st_stk_play = STA_NOTSET;
	obj->pplay = -1;
	obj->tplay = 0;
	return 0;
}

static int _init_start_stick(IXTIMELINE *obj)
{
	obj->drw.prev_start.x = -1;
	obj->drw.prev_start.y = -1;
	obj->st_stk_start = STA_NOTSET;
	obj->pstart = -1;
	obj->tstart = 0;
	return 0;
}

static int _init_end_stick(IXTIMELINE *obj)
{
	obj->drw.prev_end.x = -1;
	obj->drw.prev_end.y = -1;
	obj->st_stk_end = STA_NOTSET;
	obj->pend = -1;
	obj->tend = 0;
	return 0;
}

static int _init_section(IXTIMELINE *obj)
{
	if (_IS_VERTICAL_TYPE(obj)) {
		obj->rc_sec.x = obj->drw.rc_rba.x;
		obj->rc_sec.y = _pos_to_coord_y(obj, obj->pplay);
		obj->rc_sec.width = obj->drw.rc_rba.width;
		obj->rc_sec.height = 0;
	}
	else {
		obj->rc_sec.x = _pos_to_coord_x(obj, obj->pplay);
		obj->rc_sec.y = obj->drw.rc_rba.y;
		obj->rc_sec.width = 0,
		obj->rc_sec.height = obj->drw.rc_rba.height;
	}
	return 0;
}

static int _handoff_to_start_stick(IXTIMELINE *obj)
{
	_copy_prev_info_to_start(obj);
	obj->pstart = obj->pplay;
	obj->tstart = obj->tplay;
	obj->pend = obj->pplay;
	obj->tend = obj->tplay;
	
	_show_start_stick_x(obj);
	_show_section(obj);
	_init_play_stick(obj);
	return 0;
}

static int _handoff_to_end_stick(IXTIMELINE *obj)
{
	_copy_prev_info_to_end(obj);
	obj->pstart = obj->pplay;
	obj->tstart = obj->tplay;
	obj->pend = obj->pplay;
	obj->tend = obj->tplay;

	_show_end_stick_x(obj);
	_show_section(obj);
	_init_play_stick(obj);
	return 0;
}

static int _move_section_stick(IXTIMELINE *obj, SEC_UPDATE_E up, POS dpos)
{
	if (_IS_VERTICAL_TYPE(obj)) {
		switch (up) {
		case UPDATE_PLAY:
			obj->pplay += dpos;
			obj->tplay = _pos_to_timet(obj, obj->pplay);
			break;
		case UPDATE_START:
			obj->pstart += dpos;
			obj->tstart = _pos_to_timet(obj, obj->pstart);
			obj->rc_sec.height -= dpos;
			break;
		case UPDATE_END:
			obj->pend += dpos;
			obj->tend = _pos_to_timet(obj, obj->pend);
			obj->rc_sec.y -= dpos;
			obj->rc_sec.height += dpos;
			break;
		case UPDATE_BOTH:
			obj->pstart += dpos;
			obj->tstart = _pos_to_timet(obj, obj->pstart);
			obj->pend += dpos;
			obj->tend = _pos_to_timet(obj, obj->pend);
			obj->rc_sec.y -= dpos;
			break;	
		}
	}
	else {
		switch (up) {
		case UPDATE_PLAY:
			obj->pplay += dpos;
			obj->tplay = _pos_to_timet(obj, obj->pplay);
			break;
		case UPDATE_START:
			obj->pstart += dpos;
			obj->tstart = _pos_to_timet(obj, obj->pstart);
			obj->rc_sec.x += dpos;
			obj->rc_sec.width -= dpos;
			break;
		case UPDATE_END:
			obj->pend += dpos;
			obj->tend = _pos_to_timet(obj, obj->pend);
			obj->rc_sec.width += dpos;
			break;
		case UPDATE_BOTH:
			obj->pstart += dpos;
			obj->tstart = _pos_to_timet(obj, obj->pstart);
			obj->pend += dpos;
			obj->tend = _pos_to_timet(obj, obj->pend);
			obj->rc_sec.x += dpos;
			break;	
		}
	}
	return 0;
}

static gboolean _is_over_position(IXTIMELINE *obj, int x, int y)
{
	int ox, oy;
	int new_sec_x1, new_sec_y1, new_sec_x2, new_sec_y2;

	if (_IS_VERTICAL_TYPE(obj)) {
		oy = y - obj->sec_drg_org.y; 
		new_sec_y1 = obj->rc_sec.y + oy;
		new_sec_y2 = (obj->rc_sec.y + obj->rc_sec.height) + oy;
		if (new_sec_y1 < obj->drw.rc_rba.y) return TRUE; 
		if (new_sec_y2 > obj->drw.rc_rba.y + obj->drw.rc_rba.height) return TRUE;
	}
	else {
		ox = x - obj->sec_drg_org.x;
		new_sec_x1 = obj->rc_sec.x + ox;
		new_sec_x2 = (obj->rc_sec.x + obj->rc_sec.width) + ox;
		if (new_sec_x1 < obj->drw.rc_rba.x) return TRUE; 
		if (new_sec_x2 > obj->drw.rc_rba.x + obj->drw.rc_rba.width) return TRUE;
	}
	return FALSE;
}

static POS _get_cti_delta_pos(IXTIMELINE *obj, int x, int y)
{
	POS pos;
	_coord_to_pos(obj, x, y, &pos);
	return pos - obj->pcur;
}

static POS _get_play_delta_pos(IXTIMELINE *obj, int x, int y)
{
	POS pos;
	_coord_to_pos(obj, x, y, &pos);
	return pos - obj->pplay;
}

static POS _get_start_delta_pos(IXTIMELINE *obj, int x, int y)
{
	POS pos;
	_coord_to_pos(obj, x, y, &pos);
	return pos - obj->pstart;
}

static POS _get_end_delta_pos(IXTIMELINE *obj, int x, int y)
{
	POS pos;
	_coord_to_pos(obj, x, y, &pos);
	return pos - obj->pend;
}

static gboolean _is_valid_movement(IXTIMELINE *obj, SEC_UPDATE_E up, POS dpos)
{
	switch (up) {
	case UPDATE_START:
		if (obj->pstart + dpos >= obj->px_len) return FALSE;
		if (obj->pstart + dpos < 0) return FALSE;
		if (obj->pstart + dpos >= obj->pend) return FALSE;
		break;
	case UPDATE_END:
		if (obj->pend + dpos >= obj->px_len) return FALSE;
		if (obj->pend + dpos < 0) return FALSE;
		if (obj->pend + dpos <= obj->pstart) return FALSE;
		break;
	default: return FALSE;
	}
	return TRUE;
}

static int _move_play_stick(IXTIMELINE *obj, int x, int y)
{
	POS dpos = _get_play_delta_pos(obj, x, y);
	_move_section_stick(obj, UPDATE_PLAY, dpos);
	return 0;
}

static int _apply_play_stick(IXTIMELINE *obj)
{
	_show_play_stick_x(obj);
	evt_send_to_local(INFY_TML_PLAY_CHANGED, obj->tplay, 0, obj);
	return 0;
}

static int _update_play_stick(IXTIMELINE *obj, int x, int y)
{
	_move_play_stick(obj, x, y);
	_apply_play_stick(obj);
	return 0;
}

static int _update_start_stick(IXTIMELINE *obj, int x, int y)
{
	POS dpos = _get_start_delta_pos(obj, x, y);
	if (!_is_valid_movement(obj, UPDATE_START, dpos)) return -1;
	_move_section_stick(obj, UPDATE_START, dpos);
	_show_start_stick_x(obj);
	_show_cti_x(obj);	
	_show_section(obj);
	evt_send_to_local(INFY_TML_START_CHANGED, obj->tstart, 0, obj);
	evt_send_to_local(INFY_TML_SECTION_CHANGED, 0, 0, obj);
	return 0;
}

static int _update_end_stick(IXTIMELINE *obj, int x, int y)
{
	POS dpos = _get_end_delta_pos(obj, x, y);
	if (!_is_valid_movement(obj, UPDATE_END, dpos)) return -1;
	_move_section_stick(obj, UPDATE_END, dpos);
	_show_end_stick_x(obj);
	_show_cti_x(obj);	
	_show_section(obj);
	evt_send_to_local(INFY_TML_END_CHANGED, obj->tend, 0, obj);
	evt_send_to_local(INFY_TML_SECTION_CHANGED, 0, 0, obj);
	return 0;
}

static int _get_sec_drag_delta_pos(IXTIMELINE *obj, int x, int y, POS *dpos)
{
	if (_IS_VERTICAL_TYPE(obj)) *dpos = -(y - obj->sec_drg_org.y);
	else *dpos = x - obj->sec_drg_org.x; 
	return 0;
}

static int _update_section(IXTIMELINE *obj, int x, int y)
{
	POS dpos;
//	if (_is_over_position(obj, x, y)) return -1;
	_get_sec_drag_delta_pos(obj, x, y, &dpos);
	_move_section_stick(obj, UPDATE_BOTH, dpos);
	_tml_drawback_cti(&obj->drw, &obj->figure);
	_show_section(obj);
	if (!_IS_GHOSTCTI(obj)) _tml_draw_cti(&obj->drw, &obj->figure, obj->pcur);
	_save_org_position(obj, x, y);
	evt_send_to_local(INFY_TML_START_CHANGED, obj->tstart, 0, obj);
	evt_send_to_local(INFY_TML_END_CHANGED, obj->tend, 0, obj);
	evt_send_to_local(INFY_TML_SECTION_CHANGED, 0, 0, obj);
	return 0;
}

static gboolean _is_reverse_drag(IXTIMELINE *obj, int x, int y)
{
	POS pos;
	_coord_to_pos(obj, x, y, &pos);
	if (pos < obj->pplay) return TRUE;
	return FALSE;
}

static gboolean _is_section_setting(IXTIMELINE *obj)
{
	return obj->st_sec & STA_SETTING;
}

static int _on_scroll(IXTIMELINE *obj, GdkEvent *event, gpointer data)
{
	switch (event->scroll.direction) {
	case GDK_SCROLL_UP: 
		evt_send_to_local(INFY_TML_SCROLL_UP, 0, 0, obj);
		break;
	case GDK_SCROLL_DOWN: 
		evt_send_to_local(INFY_TML_SCROLL_DOWN, 0, 0, obj);
		break;
	}
	return 0;
}

static int _on_right_btn_pressed(IXTIMELINE *obj, GdkEvent *event, gpointer data)
{

}

static int _drop_press_event(IXTIMELINE *obj)
{
	obj->st_press = STA_NOTSET;
	return 0;
}

static void _clearbit_all(IXTIMELINE *obj)
{
	_clearbit_section_state(obj, STA_SETTING);
	_clearbit_section_state(obj, STA_DRAGGING);
	_clearbit_start_stick_state(obj, STA_DRAGGING);
	_clearbit_end_stick_state(obj, STA_DRAGGING);
	_clearbit_play_stick_state(obj, STA_DRAGGING);
	_drop_press_event(obj);

	if (_is_start_stick_visible(obj) && _is_end_stick_visible(obj)) 
		_change_section_state(obj, STA_SET);
}

static int _on_left_btn_released(IXTIMELINE *obj, GdkEvent *event, gpointer data)
{
	if (obj->st_press == STA_SET) _apply_play_stick(obj);
	_clearbit_all(obj);
	return 0;
}

static int _on_right_btn_released(IXTIMELINE *obj, GdkEvent *event, gpointer data)
{
	_clearbit_play_stick_state(obj, STA_DRAGGING);
	_hide_play_stick(obj);
	_reset_update_timer(obj);
	return 0;
}

static int _on_button_release(IXTIMELINE *obj, GdkEvent *event, gpointer data)
{
	if (event->button.button == LBUTTON) _on_left_btn_released(obj, event, data);
	else if (event->button.button == RBUTTON) _on_right_btn_released(obj, event, data);

	nfui_nfscrolledscr_draw_drawable(obj);
	_reset_update_timer(obj);
}

static int _on_2button_press(IXTIMELINE *obj, GdkEvent *event, gpointer data)
{
	int x = event->button.x;
	int y = event->button.y;
	CLICK_POINT_E pt;
	
	pt = _get_press_pt_type(obj, x, y);
	switch (pt) {
	case NO_SECTION:
	case INSIDE_OF_SECTION:
	case OUTSIDE_OF_START:
	case OUTSIDE_OF_END:
	case OVER_THE_START:
	case OVER_THE_END:
		if (obj->tplay)
			evt_send_to_local(INFY_TML_DOUBLE_CLICKED, obj->tplay, 0, obj);
		else
			evt_send_to_local(INFY_TML_DOUBLE_CLICKED, obj->tstart, 0, obj);

		_clearbit_all(obj);
		break;
	case OUTSIDE_OF_RBA:
		break;
	}
	return 0;
}

static int _keep_press_event(IXTIMELINE *obj)
{
	obj->st_press = STA_SET;
	return 0;
}

static int _on_left_btn_pressed(IXTIMELINE *obj, GdkEvent *event, gpointer data)
{
	int x = event->button.x;
	int y = event->button.y;
	CLICK_POINT_E pt;
	
	if (_IS_NOSECTION(obj)) return 0;
	_compose_rba(obj);

	pt = _get_press_pt_type(obj, x, y);
	switch (pt) {
	case NO_SECTION:
		_move_play_stick(obj, x, y);
		_keep_press_event(obj);
		if (_IS_NODRAGABLE(obj)) break;
		else if (_IS_PLAYDRGABLE(obj)) {
			_change_play_stick_state(obj, STA_DRAGGING);
		}
		else {
			_change_end_stick_state(obj, STA_DRAGGING);
			_change_section_state(obj, STA_SETTING);
		}
		break;
	case INSIDE_OF_SECTION:
		_change_section_state(obj, STA_DRAGGING);
		_save_org_position(obj, x, y);
		break;
	case OUTSIDE_OF_START:
		_hide_section(obj);
		if (!_IS_GHOSTCTI(obj)) _tml_draw_cti(&obj->drw, &obj->figure, obj->pcur);
		_change_section_state(obj, STA_NOTSET);
		evt_send_to_local(INFY_TML_UNSET_SECTION, 0, 0, obj);
		break;
	case OUTSIDE_OF_END:
		_hide_section(obj);
		if (!_IS_GHOSTCTI(obj)) _tml_draw_cti(&obj->drw, &obj->figure, obj->pcur);
		_change_section_state(obj, STA_NOTSET);
		evt_send_to_local(INFY_TML_UNSET_SECTION, 0, 0, obj);
		break;
	case OVER_THE_START:
		_change_start_stick_state(obj, STA_DRAGGING);
		break;
	case OVER_THE_END:
		_change_end_stick_state(obj, STA_DRAGGING);
		break;
	case OUTSIDE_OF_RBA:
		break;
	}
	return 0;
}

static int _on_button_press(IXTIMELINE *obj, GdkEvent *event, gpointer data)
{
	if (obj->update_mode == TML_AUTO_UPDATE) _stop_autoup_timer(obj);
	if (event->button.button == LBUTTON) _on_left_btn_pressed(obj, event, data);
	else if (event->button.button == RBUTTON) _on_right_btn_pressed(obj, event, data);
	return 0;
}

static int _on_motion_notify(IXTIMELINE *obj, GdkEvent *event, gpointer data)
{
	POS pos;
	int x = event->motion.x;
	int y = event->motion.y;

	POS dpos = _get_play_delta_pos(obj, x, y);
	if (dpos == 0) return -1; 

	_drop_press_event(obj);

	if (_is_valid_play_pos(obj) && _is_section_setting(obj)) {
		_init_section(obj);
		_tml_hide_play_stick(&obj->drw, &obj->figure);

		if (_is_reverse_drag(obj, x, y)) {
			_clearbit_end_stick_state(obj, STA_DRAGGING);
			_handoff_to_end_stick(obj);
			_change_start_stick_state(obj, STA_DRAGGING);
		}
		else {
			_handoff_to_start_stick(obj);
		}
		evt_send_to_local(INFY_TML_START_CHANGED, obj->tstart, 0, obj);
		evt_send_to_local(INFY_TML_END_CHANGED, obj->tend, 0, obj);
		evt_send_to_local(INFY_TML_SECTION_CHANGED, 0, 0, obj);
	}

	if (_is_play_stick_dragging(obj)) _update_play_stick(obj, x, y);
	if (_is_start_stick_dragging(obj)) _update_start_stick(obj, x, y);
	if (_is_end_stick_dragging(obj)) _update_end_stick(obj, x, y);
	if (_is_section_dragging(obj)) _update_section(obj, x, y);

	return 0;
}

static gboolean tml_event_handler(IXTIMELINE *obj, GdkEvent *event, gpointer data)
{
	switch (event->type) {
	case GDK_DELETE: 			_on_delete(obj, event, data); break;
	case GDK_EXPOSE: 			_on_expose(obj, event, data); break;
	case GDK_BUTTON_PRESS: 		_on_button_press(obj, event, data); break;
	case GDK_BUTTON_RELEASE: 	_on_button_release(obj, event, data); break;
	case GDK_2BUTTON_PRESS: 	_on_2button_press(obj, event, data); break;
	case GDK_MOTION_NOTIFY:		_on_motion_notify(obj, event, data); break;
	case GDK_SCROLL:			_on_scroll(obj, event, data); break;
	case IRPL_TML_GET_DATA:		_on_query(obj, event, data); break;	
	}

	return FALSE;
}

static int _get_nor_query_count(IXTIMELINE *obj)	// normal update
{
	int cnt;
	CMMPORT cmmpt;
	CMM_MESSAGE_T msg;
	TML_QRYINFO_T *info;
	int i;
	int rqcnt = 0;

	cmmpt = g_thread_self();
	cnt = cmm_get_message_count(cmmpt);

	for (i = 0; i < cnt; ++i) {
		if (cmm_peek_message(&msg, i) == 0) {
			info = (TML_QRYINFO_T *)msg.data;
			if (info->qt == NORMAL) ++rqcnt;
		}
	}

	return rqcnt;
}

static int _process_message(IXTIMELINE *obj, CMM_MESSAGE_T *pmsg)
{
	int cnt;
	CMMPORT cmmpt;
	TML_QRYINFO_T *qinfo;

	switch (pmsg->msgid) {
	case iREQ_TML_GET_DATA:
		cmmpt = g_thread_self();
		qinfo = pmsg->data;
		if (qinfo->qt == NORMAL && _get_nor_query_count(obj) > 0) break;
		_query_data_on_bg(obj, qinfo);
		break;
	}

	if (pmsg->dyn_data && pmsg->data) ifree(pmsg->data);

	// special coding
	// because drawing timeline is not regulary
//	_stop(obj);

	return 0;
}

static void* _proc_get_data(void *arg) 
{
	CMM_MESSAGE_T msg;
	IXTIMELINE *obj = (IXTIMELINE *)arg;

	_wait_run_signal(obj);
	while (_is_work(obj)) {
		usleep(obj->sleep_time);
		
		if (cmm_get_message(&msg) == 0) _process_message(obj, &msg);
		if (_is_paused(obj)) { usleep(20000); continue; }

		// some more...

	}

	_cleanup_thread(obj);
	obj->cmmpt = 0;
	obj->work = READY;
	obj->stopped = YES;

	g_thread_exit(NULL);
}

static int _init_thread(IXTIMELINE *obj)
{
	obj->work = READY;
	obj->stopped = YES;
	g_assert(obj->thd == 0);
	obj->thd = ifn_make_thread(_proc_get_data, obj);
	cmm_mount_on_thread(obj->thd);
	obj->cmmpt = obj->thd;
	obj->sleep_time = 10000;
	return 0;
}

static int _is_out_of_range(IXTIMELINE *obj, time_t timet)
{
	return !(obj->tbase <= timet && timet <= obj->tedge);
}

static int _read_conf(IXTIMELINE *obj)
{
	int ret;
	ret = icf_get_value_by_int("timeline", "dmsg");
	if (ret != -1) DBG_USE(ret);
	return 0;
}

////////////////////////////////////////////////////////////
//
// protected interfaces
//


////////////////////////////////////////////////////////////
//
// public interfaces
//

IXTIMELINE *tml_new(NFOBJECT *parent)
{
	IXTIMELINE *obj;
	NFOBJECT *top = nfui_nfobject_get_top(parent);

	obj = imalloc(sizeof(IXTIMELINE));

	nfui_nfobject_init((NFOBJECT*)obj);
	obj->object.type = NFOBJECT_TYPE_IXTIMELINE;
	obj->object.use_focus = NFOBJECT_FOCUS_ON;
	obj->object.default_event_handler = tml_event_handler;
	obj->object.use_tooltip = 0;

	obj->object.bg_color[NFOBJECT_STATE_NORMAL] = -1;

	obj->pcur = -1;
	obj->pplay = -1;
	obj->pstart = obj->pend = -1;
	obj->tcur = 0;
	obj->prev_tcur = 0;
	obj->tplay = 0;
	obj->tstart = 0;
	obj->tend= 0;

//	g_signal_connect(G_OBJECT(((NFWINDOW *)top)->main_widget),
//		"realize", G_CALLBACK(_realize_handler), obj);

	_read_conf(obj);

	return obj;
}

int tml_init(IXTIMELINE *obj, TML_FIGURE_T *figure, int x, int y, int w, int h, int ch_cnt, int ch)
{
	int tmp;

	memcpy(&obj->figure, figure, sizeof(TML_FIGURE_T));

	obj->object.x = x;
	obj->object.y = y;
	obj->object.width = w;
	obj->object.height = h;

	obj->len = figure->len;
	obj->px_len = _px_len[obj->len];
	obj->ch_cnt = ch_cnt;
	obj->ch = ch;
	obj->mtx_rdata = g_mutex_new();

	_init_spp(obj);
	_init_cti(obj);
	_init_play_stick(obj);
	_init_start_stick(obj);
	_init_end_stick(obj);

	_set_cur_time(obj, time(0));			// default : current system time
	_set_cur_cti_pos(obj, obj->px_len / 2);
	_init_rbar_buf(obj);
	_reset_range(obj);
	
	_init_drw(obj);
	_init_rect_ruler(obj);
	_init_rect_rba(obj);
	_init_section(obj);

	_init_thread(obj);
	_run(obj);
	uxm_reg_imsg_event(obj, IRPL_TML_GET_DATA);

	obj->bgnd_get = 1;
	return 0;
}

int tml_destroy(IXTIMELINE *obj)
{
	nfui_signal_emit(obj, GDK_DELETE, TRUE);
	return 0;
}

int tml_set_update_mode(IXTIMELINE *obj, TML_UPDATE_MODE_E mode)
{
	int cspp = obj->spp_table[obj->idx_spp];
	obj->update_mode = mode;
	if (mode == TML_AUTO_UPDATE) {
		if (obj->interval == 0) obj->interval = cspp * 1000;
		 _start_update_timer(obj);
	}
	else _stop_autoup_timer(obj);
	return 0;
}

int tml_set_update_interval(IXTIMELINE *obj, unsigned int interval)
{
	_change_interval(obj, interval);
	return 0;
}

int tml_set_external_time_proc(IXTIMELINE *obj, GET_EX_TIME_PROC proc)
{
	obj->get_extime_proc = proc;
	obj->last_norup = 0;		// clear, enforce normal update
	return 0;
}

int tml_refresh(IXTIMELINE *obj)
{
	_core_refresh(obj, NORMAL);
	return 0;
}

int tml_update(IXTIMELINE *obj)
{
	time_t newtime;

	if (obj->get_extime_proc) newtime = obj->get_extime_proc();
	else newtime = time(0);
	_set_cur_time(obj, newtime);
	_reset_range(obj);

	if (_is_date_changed(obj))
		evt_send_to_local(INFY_TML_DATE_CHANGED, obj->tcur, 0, obj);

	_reset_stick(obj);
	_reset_section(obj);
	_refresh(obj, NORMAL);
	return 0;
}

int tml_repaint(IXTIMELINE *obj)
{
	_repaint(obj);
	return 0;
}

int tml_zoom_max(IXTIMELINE *obj)
{
	_scale_up_max(obj);
	_refresh(obj, INTERACING);
	_refresh(obj, NORMAL);
	return 0;
}

int tml_zoom_min(IXTIMELINE *obj)
{
	_scale_down_min(obj);
	_refresh(obj, INTERACING);
	_refresh(obj, NORMAL);
	return 0;
}

int tml_zoom_in(IXTIMELINE *obj)
{
	if (_scale_up(obj) == -1) return -1;
	_refresh(obj, INTERACING);
	_refresh(obj, NORMAL);
	return 0;
}

int tml_zoom_out(IXTIMELINE *obj)
{
	if (_scale_down(obj) == -1) return -1;
	_refresh(obj, INTERACING);
	_refresh(obj, NORMAL);
	return 0;
}

int tml_go_back(IXTIMELINE *obj)
{
	_shift_cur_time(obj, -17);	// 17 means 1/17, not hour
	_reset_range(obj);
	_reset_stick(obj);
	_reset_section(obj);
	_refresh(obj, NORMAL);

	_stop_autoup_timer(obj);
	_reserve_autoup_set(obj);
	return 0;
}

int tml_go_ahead(IXTIMELINE *obj)
{
	_shift_cur_time(obj, +17);
	_reset_range(obj);
	_reset_stick(obj);
	_reset_section(obj);
	_refresh(obj, NORMAL);

	_stop_autoup_timer(obj);
	_reserve_autoup_set(obj);
	return 0;
}

int tml_shift(IXTIMELINE *obj, int fract)
{
	_shift_range(obj, fract);
	_reset_stick(obj);
	_reset_section(obj);
	_refresh(obj, NORMAL);

	_stop_autoup_timer(obj);
	_reserve_autoup_set(obj);
	return 0;
}

int tml_slide_down(IXTIMELINE *obj)
{
	_shift_cti(obj, -17);
	_reset_range(obj);
	_reset_stick(obj);
	_reset_section(obj);
	_refresh(obj, NORMAL);

	_stop_autoup_timer(obj);
	_reserve_autoup_set(obj);
	return 0;
}

int tml_slide_up(IXTIMELINE *obj)
{
	_shift_cti(obj, +17);
	_reset_range(obj);
	_reset_stick(obj);
	_reset_section(obj);
	_refresh(obj, NORMAL);

	_stop_autoup_timer(obj);
	_reserve_autoup_set(obj);
	return 0;
}

int tml_slide(IXTIMELINE *obj, int ofs_time)
{
	_shift_cti_t(obj, ofs_time);
	_reset_range(obj);
	_reset_stick(obj);
	_reset_section(obj);
	_refresh(obj, NORMAL);

	_stop_autoup_timer(obj);
	_reserve_autoup_set(obj);
	return 0;
}

int tml_get_cur_time(IXTIMELINE *obj, GTimeVal *cur)
{
	ifn_timet_to_gtv(obj->tcur, cur);
	return 0;
}

time_t tml_get_cur_time_t(IXTIMELINE *obj)
{
	return obj->tcur;
}

int tml_get_section_time(IXTIMELINE *obj, GTimeVal *start, GTimeVal *end)
{
	ifn_timet_to_gtv(obj->tstart, start);
	ifn_timet_to_gtv(obj->tend, end);
	return 0;
}

int tml_get_spp(IXTIMELINE *obj)
{
	int cspp = obj->spp_table[obj->idx_spp];
	return cspp;
}

int tml_change_channel(IXTIMELINE *obj, int ch)
{
	obj->ch = ch;
	_refresh(obj, NORMAL);
	return 0;
}

int tml_reset_style(IXTIMELINE *obj, TML_STYLE_E style)
{
	obj->style &= ~style;
	return 0;
}

TML_STYLE_E tml_get_style(IXTIMELINE *obj)
{
	return obj->style;
}

REC_TYPE_E tml_get_rec_type(IXTIMELINE *obj, int ch, time_t when)
{
	int ret;
	POS pos;
	if (_is_out_of_range(obj, when)) return REC_UNKNOWN;
	pos = _timet_to_pos(obj, when);
	RDATA_LOCK();
	ret = _tml_scan_data(obj->rdata, ch, pos);
	RDATA_UNLOCK();
	return (REC_TYPE_E)ret;
}

time_t tml_get_base_time_t(IXTIMELINE *obj)
{
	return obj->tbase;
}

time_t tml_get_edge_time_t(IXTIMELINE *obj)
{
	return obj->tedge;
}

int tml_change_cur_time(IXTIMELINE *obj, GTimeVal *cur)
{
	_set_cur_time(obj, ifn_gtv_to_timet(cur));
	_reset_range(obj);
	_refresh(obj, NORMAL);
	return 0;
}

int tml_change_cur_time_t(IXTIMELINE *obj, time_t cur)
{
	_set_cur_time(obj, cur);
	_reset_range(obj);
	_refresh(obj, NORMAL);
	return 0;
}

int tml_reset_cur_day(IXTIMELINE *obj, time_t timet)
{
	time_t tbase = ifn_get_local_midnight(timet);
	DMSG(9, "timet = %ld, tbase = %ld\n", timet, tbase);
	_move_range(obj, tbase, timet);
	//_move_range(obj, tbase, tbase);
	_reset_stick(obj);
	_refresh(obj, NORMAL);
	return 0;
}

int tml_reset_cur_day_hour(IXTIMELINE *obj, time_t timet)
{
	time_t tbase = ifn_get_local_midnight_tml(timet);
	DMSG(9, "timet = %ld, tbase = %ld\n", timet, tbase);
	_move_range(obj, tbase, timet);
	//_move_range(obj, tbase, tbase);
	_reset_stick(obj);
	_refresh(obj, NORMAL);
	return 0;
}

int tml_change_cur_day(IXTIMELINE *obj, time_t timet)
{
	time_t tbase = ifn_get_local_midnight(timet);
	_move_range(obj, tbase, timet);
	_reset_stick(obj);
	_refresh(obj, NORMAL);
	return 0;
}

int tml_set_section_time(IXTIMELINE *obj, GTimeVal *start, GTimeVal *end)
{
	obj->tstart = start->tv_sec;
	obj->pstart = _timet_to_pos(obj, obj->tstart);
	obj->tend = end->tv_sec;
	obj->pend = _timet_to_pos(obj, obj->tend);
	_change_section_state(obj, STA_SET);
	_hide_play_stick(obj);
	return 0;
}

int tml_set_section_time_t(IXTIMELINE *obj, time_t start, time_t end)
{
	obj->tstart = start;
	obj->tend = end;
	_reset_stick(obj);
	_reset_section(obj);
	_change_section_state(obj, STA_SET);
	_hide_play_stick(obj);
	return 0;
}

int tml_set_start_time(IXTIMELINE *obj, GTimeVal *start)
{
	int update = 0;

	if (obj->tstart != 0 && obj->tend != 0) update = 1;
	obj->tstart = ifn_gtv_to_timet(start);
	_reset_stick(obj);
	_reset_section(obj);
	return 0;
}

int tml_set_end_time(IXTIMELINE *obj, GTimeVal *end)
{
	int update = 0;

	if (obj->tstart != 0 && obj->tend != 0) update = 1;
	obj->tend = ifn_gtv_to_timet(end);
	_reset_stick(obj);
	_reset_section(obj);
	return 0;
}

int tml_set_start_time_t(IXTIMELINE *obj, time_t start)
{
	int update = 0;

	if (obj->tstart != 0 && obj->tend != 0) update = 1;
	obj->tstart = start;
	_reset_stick(obj);
	_reset_section(obj);
	return 0;
}

int tml_set_end_time_t(IXTIMELINE *obj, time_t end)
{
	int update = 0;

	if (obj->tstart != 0 && obj->tend != 0) update = 1;
	obj->tend = end;
	_reset_stick(obj);
	_reset_section(obj);
	return 0;
}

int tml_set_cti_position(IXTIMELINE *obj, int pos)	// -1 : edge
{
	if (pos < 0) pos = obj->px_len - 1;
	if (pos > obj->px_len) pos = obj->px_len - 1;
	_set_cur_cti_pos(obj, pos);
	return 0;
}

int tml_set_scale_depth(IXTIMELINE *obj, int depth)
{
	if (depth < 0) obj->idx_spp = 0;
	else if (depth >= obj->cnt_valid_spp) obj->idx_spp = obj->cnt_valid_spp - 1;
	else obj->idx_spp = depth;

	_reset_range(obj);
	_reset_stick(obj);
	return 0;
}

int tml_set_style(IXTIMELINE *obj, TML_STYLE_E style)
{
	obj->style |= style;
	return 0;
}

int tml_set_playstick_time_t(IXTIMELINE *obj, time_t tplay)
{
	POS pos;
	if (0){ //tplay < obj->tbase || obj->tedge < tplay) {
		obj->tplay = 0;
		obj->pplay = -1;
	}
	else {
		if (_is_section_set(obj)) {
			obj->tplay = 0;
			obj->pplay = -1;
		}
		else {
			pos = _timet_to_pos(obj, tplay);
			obj->tplay = tplay;
			obj->pplay = pos;
		}
	}
	return 0;
}

int tml_set_manual_paint_mode(IXTIMELINE *obj, int onoff)
{
	obj->manual_paint = onoff;
	return 0;
}

int tml_get_play_pos(IXTIMELINE *obj)
{
	return obj->pplay;
}

int tml_is_set_section(IXTIMELINE *obj)
{
	return obj->tstart != 0 && obj->tend != 0;
}
