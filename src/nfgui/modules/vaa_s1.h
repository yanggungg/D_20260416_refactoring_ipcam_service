/*
 * vaa_s1.h
 * 	- video analytics agent
 *	- dependencies :
 *		
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, May 13, 2013
 *
 */


#ifndef __VAA_S1_H
#define __VAA_S1_H

#include "nf_afx.h"
#include "nfdal.h"
#include "dit.h"
#include "vaa.h"
#include "libivcam.h"
#include "vw_vca.h"

typedef int RULEID;

typedef enum _PATTERN_E {
	PATTERN_INVASION		= 0,
	PATTERN_LOITERING		= 1,
	PATTERN_ABANDON			= 2,
	PATTERN_STEAL			= 3,
	PATTERN_TOPPLE			= 4,
	PATTERN_FENCE			= 5,
	PATTERN_COUNT			= 6,
	PATTERN_TAMPERING		= 7,
	PATTERN_PRIVACY			= 8,
	PATTERN_MAX,
	PATTERN_NONE			= 99,
} PATTERN_E;

typedef struct _VARULE_SHAPE {
	IGPOINT	pt[MAX_PT];
	int 	ptcnt;
	IGRECT	dirpos;			// read only
} VARULE_SHAPE;

typedef struct _VARULE_CONF {
	RULEID		ruleid;		// read only
	PATTERN_E	patt;	

	int			use_rule;
	int			use_counter;

	int			cfg_sens;
	int			cfg_time;
	int			cfg_dir;

	int			sty_highlight;

	BITMASK		cap_dirctrl;
} VARULE_CONF;

typedef struct _TRBOX {
	IGRECT			rect;
	unsigned int	code;
	int				style;
} TRBOX;

#define MAX_PT_S1	8

////////////////////////////////////////////////////////////
//
// public functions
//


int vaa_s1_init(int ch, VAAID *pvaa, DITID *pdit);
VAAID vaa_s1_create();
int vaa_s1_destroy(VAAID id);
int vaa_s1_reload(VAAID id);
int vaa_s1_disable(VAAID id);
int vaa_s1_enable(VAAID id);
int vaa_s1_is_enabled(VAAID id);

int vaa_s1_raiseup(VAAID id);

//int vaa_s1_is_event_linked(VAAID id);

int vaa_s1_link_dit(VAAID id, DITID ditid);
int vaa_s1_unlink_dit(VAAID id, DITID ditid);
int vaa_s1_is_dit_linked(VAAID id);
DITID vaa_s1_get_dit(VAAID id);


int vaa_s1_activate_pattern(VAAID id, PATTERN_E patt);
int vaa_s1_activate_all_pattern(VAAID id);
int vaa_s1_switch_pattern(VAAID id, PATTERN_E patt);
int vaa_s1_deactivate_pattern(VAAID id, PATTERN_E patt);
int vaa_s1_deactivate_all_pattern(VAAID id);
int vaa_s1_activate_meta(VAAID id, VAA_META_E meta);
int vaa_s1_activate_all_meta(VAAID id);
int vaa_s1_deactivate_meta(VAAID id, VAA_META_E meta);
int vaa_s1_deactivate_all_meta(VAAID id);
int vaa_s1_deactivate_all(VAAID id);


int vaa_s1_load_db(VAAID id);
int vaa_s1_save_db(VAAID id);

int vaa_s1_get_rule_count(VAAID id);
RULEID vaa_s1_find_rule(VAAID id, int x, int y);

int vaa_s1_set_selecting_margin(VAAID id, int pixel);

/*
int vaa_s1_hilight_rule(VAAID id, RULEID rule, bool onoff, bool repaint);
int vaa_s1_is_hilighted(VAAID id, RULEID rule);
int vaa_s1_is_hilighted_by_xy(VAAID id, int x, int y);

int vaa_s1_repaint(VAAID id);
*/

int vaa_s1_get_rule_shape(VAAID id, RULEID rule, VARULE_SHAPE *shape);
int vaa_s1_set_rule_shape(VAAID id, RULEID rule, VARULE_SHAPE *shape); 

int vaa_s1_get_rule_conf(VAAID id, RULEID rule, VARULE_CONF *conf);
int vaa_s1_set_rule_conf(VAAID id, RULEID rule, VARULE_CONF *conf);

int vaa_s1_get_rule_confs(VAAID id, PATTERN_E page, VARULE_CONF conf[16], int *cnt);
int vaa_s1_set_rule_confs(VAAID id, PATTERN_E page, VARULE_CONF conf[16], int cnt);

int vaa_s1_get_rule_confs_all(VAAID id, VARULE_CONF conf[16], int *cnt);

int vaa_s1_remove_rule(VAAID id, RULEID rule);

RULEID vaa_s1_get_counted_rule(VAAID id, unsigned short countid);
int	vaa_s1_parse_event(VAAID vaaid, ivca_rule_event_t *pevt, int *zid, int *cid);
int vaa_s1_timer_proc(VAAID vaaid);
int vaa_s1_make_blinking(VAAID vaaid, PATTERN_E patt);

////////////////////////////////////////////////////////////
//
// protected functions	- meta data
//

int vaa_s1_show_meta(VAAID id, gint cnt, ivcam_obj_t* data);



////////////////////////////////////////////////////////////
//
// protected functions	- event handler
//

gint vaa_s1_event_mouse_left_press(VCA_SCREEN_INFO *scrInfo, VCA_MEVENT_PT *evt_pt, gpointer user_data);
gint vaa_s1_event_mouse_left_2press(VCA_SCREEN_INFO *scrInfo, VCA_MEVENT_PT *evt_pt, gpointer user_data);
gint vaa_s1_event_mouse_right_press(VCA_SCREEN_INFO *scrInfo, VCA_MEVENT_PT *evt_pt, gpointer user_data);
gint vaa_s1_event_mouse_right_2press(VCA_SCREEN_INFO *scrInfo, VCA_MEVENT_PT *evt_pt, gpointer user_data);
gint vaa_s1_event_mouse_release(VCA_SCREEN_INFO *scrInfo, VCA_MEVENT_PT *evt_pt, gpointer user_data);
gint vaa_s1_event_mouse_left_drag(VCA_SCREEN_INFO *scrInfo, VCA_MEVENT_PT *evt_pt, gpointer user_data);
gint vaa_s1_event_mouse_add_point(VCA_SCREEN_INFO *scrInfo, VCA_MEVENT_PT *evt_pt, gpointer user_data);
gint vaa_s1_event_mouse_del_point(VCA_SCREEN_INFO *scrInfo, VCA_MEVENT_PT *evt_pt, gpointer user_data);
gint vaa_s1_event_select_ruleid(int select_ruleid, gpointer user_data);

int vaa_s1_notify_vca_event(NF_NOTIFY_INFO *data);
int vaa_s1_notify_vca_track_info(NF_NOTIFY_INFO *data);
int vaa_s1_notify_vca_meta_data(NF_NOTIFY_INFO *data);
int vaa_s1_notify_vca_counter(NF_NOTIFY_INFO *data);

#endif
