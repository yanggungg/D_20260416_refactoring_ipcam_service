/*
 * vaa_itx.h
 *  - video analytics agent
 *  - dependencies :
 *      
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, May 13, 2013
 *
 */


#ifndef __VAA_ITX_H
#define __VAA_ITX_H

#include "nf_afx.h"
#include "nfdal.h"
#include "dit.h"
#include "vaa.h"
#include "libivcam.h"
#include "vw_vca.h"

typedef int ITX_ZONEID;
typedef int ITX_CNTRID;
typedef int ITX_CALBID;

typedef enum _ITX_RULETYPE_E {
	ITX_RULETYPE_REVERSE	    = 0,
	ITX_RULETYPE_FOWARD		    = 1,
	ITX_RULETYPE_ENTER		    = 2,
	ITX_RULETYPE_EXIT		    = 3,
	ITX_RULETYPE_STOPPED		= 4,
	ITX_RULETYPE_REMOVED		= 5,
	ITX_RULETYPE_LOITERED		= 6,
	ITX_RULETYPE_FALL		    = 7,
	ITX_RULETYPE_COUNTER		= 8,
	ITX_RULETYPE_MAX,

	ITX_RULETYPE_NONE			= 99,
} ITX_RULETYPE_E;

typedef enum _DIT_PAGE_E {
	DIT_PAGE_ZONE       = 0,
	DIT_PAGE_CNTR       = 1,
	DIT_PAGE_CALB       = 2,
	DIT_PAGE_META       = 9,	
} DIT_PAGE_E;

typedef struct _ITX_VAZONE_SHAPE {
    IGPOINT         pt[MAX_PT];
    int             ptcnt;
    IGPOINT         dir_pt[MAX_PT];         // read only
    int             dir_ptcnt;    
    char            name[32];
    int             color_idx;
} ITX_VAZONE_SHAPE;

typedef struct _ITX_VAZONE_CONF {
    ITX_ZONEID      zoneid;     // read only
    int             type;       // read only, line : 0. area : 1
    int             use_zone;
    int             focus;    

    int             active;
    int             forward;
    int             reverse;    
    int             enter;
    int             exit;
    int             removed;
    int             loitering;
    int             stopped;
    int             cfg_dir;
    int             cfg_stop_time;
    int             cfg_remove_time;
    int             cfg_loiter_time;
    int             use_filter_color;
    int             use_filter_size;
    int             use_filter_speed;
    int             filter_color_idx;    
    int             filter_color_prct;    
    int             filter_width_from;
    int             filter_width_to;    
    int             filter_height_from;
    int             filter_height_to;
    int             filter_speed_from;
    int             filter_speed_to;
} ITX_VAZONE_CONF;

typedef struct _ITX_VACNTR_SHAPE {
    IGPOINT         pt[MAX_PT];
    int             ptcnt;
    char            name[32];
    int             color_idx;
} ITX_VACNTR_SHAPE;

typedef struct _ITX_VACNTR_CONF {
    ITX_CNTRID      cntrid;     // read only
    int             use_cntr;
    int             focus;    

    int             active;  
    int             use_counter_event;
    int             counter_event_val;
    int             use_reset_value;

    int             source_up;
    int             source_down;
} ITX_VACNTR_CONF;

typedef struct _ITX_VARULE_PROP {
    gint            unit;
    
	gboolean        en_shadowrm;
    guint           track_ref;
    gboolean        en_usecalib;
    guint           min_width3d;
	guint           min_height3d;

	gboolean 	    sw_obj_bb;
	gboolean 	    sw_obj_tr;
	gboolean 	    sw_obj_id;
	gboolean 	    sw_obj_w3d;
	gboolean 	    sw_obj_h3d;
	gboolean 	    sw_obj_s3d;
	gboolean 	    sw_rule;
} ITX_VARULE_PROP;

typedef struct _ITX_VACALB_SHAPE {
    IGPOINT         pt[MAX_PT];
    int             ptcnt;
    char            value[32];    
    IGPOINT         iupp_pt[MAX_PT];
    int             iupp_w;
    int             iupp_h;
    IGPOINT         ilow_pt[MAX_PT];
    int             ilow_ptcnt;    
    int             color_idx;
    int             iupp_color_idx;        
    int             ilow_color_idx;   
} ITX_VACALB_SHAPE;

typedef struct _ITX_VACALB_CONF {
    ITX_CALBID      calbid;     // read only
    int             use_calb;
    int             focus;
    
    int             height;
} ITX_VACALB_CONF;

typedef struct _ITX_VACALB_RESULT {
    int             paramvalid;
	float           focal;
	float           cam_height;
	float           cam_tilt;   
	int             p_width;
	int             p_height;
} ITX_VACALB_RESULT;

typedef struct _ITX_TRBOX {
    IGRECT          rect;
    unsigned int    code;
    int             style;
} ITX_TRBOX;

#define MAX_PT_ITX  16

////////////////////////////////////////////////////////////
//
// public functions
//


int vaa_itx_init(int ch, VAAID *pvaa, DITID *pdit);
int vaa_itx_pb_init(int ch, VAAID *pvaa, DITID *pdit);
VAAID vaa_itx_create();
int vaa_itx_destroy(VAAID id);
int vaa_itx_reload(VAAID id);
int vaa_itx_disable(VAAID id);
int vaa_itx_enable(VAAID id);
int vaa_itx_is_enabled(VAAID id);
int vaa_itx_raiseup(VAAID id);
int vaa_itx_link_dit(VAAID id, DITID ditid);
int vaa_itx_unlink_dit(VAAID id, DITID ditid);
int vaa_itx_is_dit_linked(VAAID id);
DITID vaa_itx_get_dit(VAAID id);
int vaa_itx_load_db(VAAID id);
int vaa_itx_save_db(VAAID id);
int vaa_itx_export_db(VAAID id, VCAData *vcadata);

int vaa_itx_add_zone_line_default_template(VAAID id, ITX_ZONEID zone);
int vaa_itx_add_zone_area_default_template(VAAID id, ITX_ZONEID zone);
int vaa_itx_get_zone_shape(VAAID id, ITX_ZONEID zone, ITX_VAZONE_SHAPE *shape);
int vaa_itx_set_zone_shape(VAAID id, ITX_ZONEID zone, ITX_VAZONE_SHAPE *shape); 
int vaa_itx_get_zone_conf(VAAID id, ITX_ZONEID zone, ITX_VAZONE_CONF *conf);
int vaa_itx_set_zone_conf(VAAID id, ITX_ZONEID zone, ITX_VAZONE_CONF *conf);
int vaa_itx_get_zone_confs_all(VAAID id, ITX_VAZONE_CONF conf[16], int *cnt);
ITX_ZONEID vaa_itx_find_zone(VAAID id, int x, int y);

int vaa_itx_add_cntr_default_template(VAAID id, ITX_CNTRID cntr);
int vaa_itx_get_cntr_shape(VAAID id, ITX_CNTRID cntr, ITX_VACNTR_SHAPE *shape);
int vaa_itx_set_cntr_shape(VAAID id, ITX_CNTRID cntr, ITX_VACNTR_SHAPE *shape); 
int vaa_itx_get_cntr_conf(VAAID id, ITX_CNTRID cntr, ITX_VACNTR_CONF *conf);
int vaa_itx_set_cntr_conf(VAAID id, ITX_CNTRID cntr, ITX_VACNTR_CONF *conf);
int vaa_itx_get_cntr_confs_all(VAAID id, ITX_VACNTR_CONF conf[16], int *cnt);
ITX_CNTRID vaa_itx_find_cntr(VAAID id, int x, int y);

int vaa_itx_get_rule_prop(VAAID id, ITX_VARULE_PROP *prop);
int vaa_itx_set_rule_prop(VAAID id, ITX_VARULE_PROP *prop);

int vaa_itx_add_calb_default_template(VAAID id, ITX_CALBID calb);
int vaa_itx_get_calb_shape(VAAID id, ITX_CALBID calb, ITX_VACALB_SHAPE *shape);
int vaa_itx_set_calb_shape(VAAID id, ITX_CALBID calb, ITX_VACALB_SHAPE *shape); 
int vaa_itx_get_calb_conf(VAAID id, ITX_CALBID calb, ITX_VACALB_CONF *conf);
int vaa_itx_set_calb_conf(VAAID id, ITX_CALBID calb, ITX_VACALB_CONF *conf);
int vaa_itx_get_calb_confs_all(VAAID id, ITX_VACALB_CONF  conf[32], int *cnt);
ITX_CALBID vaa_itx_find_calb(VAAID id, int x, int y);

int vaa_itx_get_calb_result(VAAID id, ITX_VACALB_RESULT *res);
int vaa_itx_set_calb_result(VAAID id, ITX_VACALB_RESULT *res);

int vaa_itx_activate_rule(VAAID id, DIT_PAGE_E page);
int vaa_itx_activate_all_rule(VAAID id);
int vaa_itx_deactivate_rule(VAAID id, DIT_PAGE_E page);
int vaa_itx_deactivate_all_rule(VAAID id);
int vaa_itx_activate_calb(VAAID id);
int vaa_itx_deactivate_calb(VAAID id);
int vaa_itx_activate_meta(VAAID id, VAA_META_E meta);
int vaa_itx_activate_all_meta(VAAID id);
int vaa_itx_deactivate_meta(VAAID id, VAA_META_E meta);
int vaa_itx_deactivate_all_meta(VAAID id);
int vaa_itx_deactivate_all(VAAID id);

ITX_CNTRID vaa_itx_get_counted_rule(VAAID id, unsigned short countid);
int vaa_itx_parse_event(VAAID vaaid, ivca_rule_event_t *pevt, int *zid, int *cid);
int vaa_itx_timer_proc(VAAID vaaid);
int vaa_itx_make_zone_blinking(VAAID vaaid, ITX_ZONEID zone);



////////////////////////////////////////////////////////////
//
// protected functions  - meta data
//

int vaa_itx_show_meta(VAAID id, gint cnt, ivcam_obj_t* data);



////////////////////////////////////////////////////////////
//
// protected functions  - event handler
//

gint vaa_itx_event_set_vmap(VCA_VMAP_INFO *vmapInfo, gpointer user_data);
gint vaa_itx_event_mouse_left_press(VCA_SCREEN_INFO *scrInfo, VCA_MEVENT_PT *evt_pt, gpointer user_data);
gint vaa_itx_event_mouse_left_2press(VCA_SCREEN_INFO *scrInfo, VCA_MEVENT_PT *evt_pt, gpointer user_data);
gint vaa_itx_event_mouse_right_press(VCA_SCREEN_INFO *scrInfo, VCA_MEVENT_PT *evt_pt, gpointer user_data);
gint vaa_itx_event_mouse_right_2press(VCA_SCREEN_INFO *scrInfo, VCA_MEVENT_PT *evt_pt, gpointer user_data);
gint vaa_itx_event_mouse_release(VCA_SCREEN_INFO *scrInfo, VCA_MEVENT_PT *evt_pt, gpointer user_data);
gint vaa_itx_event_mouse_left_drag(VCA_SCREEN_INFO *scrInfo, VCA_MEVENT_PT *evt_pt, gpointer user_data);
gint vaa_itx_event_mouse_add_point(VCA_SCREEN_INFO *scrInfo, VCA_MEVENT_PT *evt_pt, gpointer user_data);
gint vaa_itx_event_mouse_del_point(VCA_SCREEN_INFO *scrInfo, VCA_MEVENT_PT *evt_pt, gpointer user_data);
gint vaa_itx_event_select_ruleid(int type, int select_ruleid, gpointer user_data);

int vaa_itx_notify_vca_event(NF_NOTIFY_INFO *data);
int vaa_itx_notify_vca_track_info(NF_NOTIFY_INFO *data);
int vaa_itx_notify_vca_meta_data(NF_NOTIFY_INFO *data);
int vaa_itx_notify_vca_counter_info(NF_NOTIFY_INFO *data);
int vaa_itx_notify_vca_analyze_event(NF_NOTIFY_INFO *data);

#endif
