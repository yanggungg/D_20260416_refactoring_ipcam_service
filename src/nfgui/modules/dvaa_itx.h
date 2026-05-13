/*
 * dvaa_itx.h
 *  - deeplearning video analytics agent
 *  - dependencies :
 *      
 *
 * Written by Jungkyu. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Feb 15, 2019
 *
 */


#ifndef __DVAA_ITX_H
#define __DVAA_ITX_H

#include "nf_afx.h"
#include "nfdal.h"
#include "dit.h"
#include "dvaa.h"
#include "libivcam.h"
#include "vw_dva.h"
#include "nf_api_dlva.h"

typedef int ITX_ZONEID;
typedef int ITX_CNTRID;
typedef int ITX_CALBID;
typedef int ITX_EXTZONEID;
typedef int ITX_FRZONEID;
typedef int ITX_LPRZONEID;

typedef enum _ITX_ALGOTYPE_E {
	ITX_ALGOTYPE_DETECTOR	    = 0,
	ITX_ALGOTYPE_FACE		    = 1,
	ITX_ALGOTYPE_PLATENO	    = 2,
	ITX_ALGOTYPE_MAX,
} ITX_ALGOTYPE_E;

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
	ITX_RULETYPE_INTRUSION	    = 9,
    ITX_RULETYPE_GENERIC	    = 10,
	ITX_RULETYPE_MAX,

	ITX_RULETYPE_NONE			= 99,
} ITX_RULETYPE_E;

typedef enum _DIT_PAGE_E {
	DIT_PAGE_ZONE       = 0,
	DIT_PAGE_CNTR       = 1,
	DIT_PAGE_CALB       = 2,
    DIT_PAGE_EXTZONE    = 3,
    DIT_PAGE_FRZONE     = 4,
    DIT_PAGE_LPRZONE    = 5,
	DIT_PAGE_META       = 9,
} DIT_PAGE_E;

typedef struct _ITX_DVAZONE_SHAPE {
    IGPOINT         pt[MAX_PT];
    int             ptcnt;
    IGPOINT         dir_pt[MAX_PT];         // read only
    int             dir_ptcnt;    
    char            name[32];
    int             color_idx;
} ITX_DVAZONE_SHAPE;

typedef struct _ITX_DVAZONE_CONF {
    ITX_ZONEID      zoneid;     // read only
    int             type;       // read only, line : 0. area : 1
    int             use_zone;
    int             focus;    

    int             active;
    int             all_detect_obj;
    char            interest_obj[256];
    int             forward;
    int             reverse;    
    int             intrusion;
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
    int             c_threshold;
    char            event_audio[8][256];
} ITX_DVAZONE_CONF;

typedef struct _ITX_DVACNTR_SHAPE {
    IGPOINT         pt[MAX_PT];
    int             ptcnt;
    char            name[32];
    int             color_idx;
} ITX_DVACNTR_SHAPE;

typedef struct _ITX_DVACNTR_CONF {
    ITX_CNTRID      cntrid;     // read only
    int             use_cntr;
    int             focus;    

    int             active;  
    int             use_counter_event;
    int             counter_event_val;
    int             use_reset_value;

    int             source_up;
    int             source_down;
} ITX_DVACNTR_CONF;

typedef struct _ITX_DVARULE_PROP {
    gint            en_engine;
    gint            unit;
    
	gboolean        en_shadowrm;
    guint           track_ref;
    gboolean        en_usecalib;
    guint           min_width3d;
	guint           min_height3d;
    gboolean        en_static_filter;
    gint            static_filter_sense;

	gboolean 	    sw_obj_bb;
	gboolean 	    sw_obj_tr;
	gboolean 	    sw_obj_id;
	gboolean 	    sw_obj_w3d;
	gboolean 	    sw_obj_h3d;
	gboolean 	    sw_obj_s3d;
	gboolean 	    sw_rule;
} ITX_DVARULE_PROP;

typedef struct _ITX_FRZONE_SHAPE {
    IGPOINT         pt[MAX_PT];
    int             ptcnt;
    IGPOINT         dir_pt[MAX_PT];         // read only
    int             dir_ptcnt;    
    char            name[32];
    int             color_idx;
} ITX_FRZONE_SHAPE;

typedef struct _ITX_FRZONE_CONF {
    ITX_FRZONEID    zoneid;     // read only
    int             type;       // read only, line : 0. area : 1
    int             use_zone;
    int             focus;    

    int             active;
    unsigned int    triggerid;  // aibox or aicam trigger id 
} ITX_FRZONE_CONF;

typedef struct _ITX_FRRULE_PROP {
	gboolean 	    sw_obj_bb;
	gboolean 	    sw_obj_grpname;    
	gboolean 	    sw_rule;
} ITX_FRRULE_PROP;

typedef struct _ITX_LPRZONE_SHAPE {
    IGPOINT         pt[MAX_PT];
    size_t          ptcnt;
    IGPOINT         dir_pt[MAX_PT];         // read only
    int             dir_ptcnt;    
    char            name[32];
    int             color_idx;
} ITX_LPRZONE_SHAPE;

typedef struct _ITX_LPRZONE_CONF {
    ITX_LPRZONEID   zoneid;     // read only
    int             type;       // read only, line : 0. area : 1
    int             use_zone;
    int             focus;    

    int             active;
    unsigned int    triggerid;  // aibox or aicam trigger id
} ITX_LPRZONE_CONF;

typedef struct _ITX_LPRRULE_PROP {
	gboolean 	    sw_obj_bb;
	gboolean 	    sw_obj_grpname;    
	gboolean 	    sw_rule;
} ITX_LPRRULE_PROP;

typedef struct _ITX_DVACALB_SHAPE {
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
} ITX_DVACALB_SHAPE;

typedef struct _ITX_DVACALB_CONF {
    ITX_CALBID      calbid;     // read only
    int             use_calb;
    int             focus;
    
    int             height;
} ITX_DVACALB_CONF;

typedef struct _ITX_DVACALB_RESULT {
    int             paramvalid;
	float           focal;
	float           cam_height;
	float           cam_tilt;   
	int             p_width;
	int             p_height;
} ITX_DVACALB_RESULT;

typedef struct _ITX_DVAEOSD_CONF {
	guint display_mode;
	guint object_color;
	guint rule_color;
	guint event_color;
	guint line_width;
	guint line_transparency;
	gchar object_type[256];
} ITX_DVAEOSD_CONF;

typedef struct _ITX_TRBOX {
    IGRECT          rect;
    unsigned int    code;
    int             style;
} ITX_TRBOX;

typedef struct _DVAAE_POINT {
	int		        x;
	int		        y;
} DVAAE_POINT;

#define MAX_PT_ITX  16






////////////////////////////////////////////////////////////
//
// public functions
//


int dvaa_itx_init(int ch, DVAAID *pdvaa, DITID *pdit);
int dvaa_itx_pb_init(int ch, DVAAID *pdvaa, DITID *pdit);
DVAAID dvaa_itx_create(int ch);
int dvaa_itx_destroy(DVAAID id);
int dvaa_itx_reload(DVAAID id);
int dvaa_itx_enable(DVAAID id);
int dvaa_itx_disable(DVAAID id);
int dvaa_itx_enable_strule(DVAAID id);
int dvaa_itx_disable_strule(DVAAID id);
int dvaa_itx_active_external_rule(DVAAID id);
int dvaa_itx_inactive_external_rule(DVAAID id);
int dvaa_itx_raiseup(DVAAID id);
int dvaa_itx_link_dit(DVAAID id, DITID ditid);
int dvaa_itx_unlink_dit(DVAAID id, DITID ditid);
int dvaa_itx_is_dit_linked(DVAAID id);
DITID dvaa_itx_get_dit(DVAAID id);
int dvaa_itx_load_db(DVAAID id);
int dvaa_itx_save_db(DVAAID id);
int dvaa_itx_export_db(DVAAID id, DvaBxData *dvabxdata);

int dvaa_itx_detector_add_zone_line_default_template(DVAAID id, ITX_ZONEID zone);
int dvaa_itx_detector_add_zone_area_default_template(DVAAID id, ITX_ZONEID zone);
int dvaa_itx_detector_get_zone_shape(DVAAID id, ITX_ZONEID zone, ITX_DVAZONE_SHAPE *shape);
int dvaa_itx_detector_set_zone_shape(DVAAID id, ITX_ZONEID zone, ITX_DVAZONE_SHAPE *shape); 
int dvaa_itx_detector_get_zone_conf(DVAAID id, ITX_ZONEID zone, ITX_DVAZONE_CONF *conf);
int dvaa_itx_detector_set_zone_conf(DVAAID id, ITX_ZONEID zone, ITX_DVAZONE_CONF *conf);
int dvaa_itx_detector_get_zone_confs_all(DVAAID id, ITX_DVAZONE_CONF conf[16], int *cnt);
ITX_ZONEID dvaa_itx_detector_find_zone(DVAAID id, int x, int y, float scale_x, float scale_y);

int dvaa_itx_detector_add_cntr_default_template(DVAAID id, ITX_CNTRID cntr);
int dvaa_itx_detector_get_cntr_shape(DVAAID id, ITX_CNTRID cntr, ITX_DVACNTR_SHAPE *shape);
int dvaa_itx_detector_set_cntr_shape(DVAAID id, ITX_CNTRID cntr, ITX_DVACNTR_SHAPE *shape); 
int dvaa_itx_detector_get_cntr_conf(DVAAID id, ITX_CNTRID cntr, ITX_DVACNTR_CONF *conf);
int dvaa_itx_detector_set_cntr_conf(DVAAID id, ITX_CNTRID cntr, ITX_DVACNTR_CONF *conf);
int dvaa_itx_detector_get_cntr_confs_all(DVAAID id, ITX_DVACNTR_CONF conf[16], int *cnt);
ITX_CNTRID dvaa_itx_detector_find_cntr(DVAAID id, int x, int y, float scale_x, float scale_y, int degree);

int dvaa_itx_detector_get_rule_prop(DVAAID id, ITX_DVARULE_PROP *prop);
int dvaa_itx_detector_set_rule_prop(DVAAID id, ITX_DVARULE_PROP *prop);

int dvaa_itx_detector_add_calb_default_template(DVAAID id, ITX_CALBID calb);
int dvaa_itx_detector_get_calb_shape(DVAAID id, ITX_CALBID calb, ITX_DVACALB_SHAPE *shape);
int dvaa_itx_detector_set_calb_shape(DVAAID id, ITX_CALBID calb, ITX_DVACALB_SHAPE *shape); 
int dvaa_itx_detector_get_calb_conf(DVAAID id, ITX_CALBID calb, ITX_DVACALB_CONF *conf);
int dvaa_itx_detector_set_calb_conf(DVAAID id, ITX_CALBID calb, ITX_DVACALB_CONF *conf);
int dvaa_itx_detector_get_calb_confs_all(DVAAID id, ITX_DVACALB_CONF  conf[32], int *cnt);
ITX_CALBID dvaa_itx_detector_find_calb(DVAAID id, int x, int y, float scale_x, float scale_y);

int dvaa_itx_detector_get_calb_result(DVAAID id, ITX_DVACALB_RESULT *res);
int dvaa_itx_detector_set_calb_result(DVAAID id, ITX_DVACALB_RESULT *res);

int dvaa_itx_detector_get_eosd_conf(DVAAID id, ITX_DVAEOSD_CONF *conf);
int dvaa_itx_detector_set_eosd_conf(DVAAID id, ITX_DVAEOSD_CONF *conf);

int dvaa_itx_face_add_zone_line_default_template(DVAAID id, ITX_FRZONEID zone);
int dvaa_itx_face_add_zone_area_default_template(DVAAID id, ITX_FRZONEID zone);
int dvaa_itx_face_get_zone_shape(DVAAID id, ITX_FRZONEID zone, ITX_FRZONE_SHAPE *shape);
int dvaa_itx_face_set_zone_shape(DVAAID id, ITX_FRZONEID zone, ITX_FRZONE_SHAPE *shape); 
int dvaa_itx_face_get_zone_conf(DVAAID id, ITX_FRZONEID zone, ITX_FRZONE_CONF *conf);
int dvaa_itx_face_set_zone_conf(DVAAID id, ITX_FRZONEID zone, ITX_FRZONE_CONF *conf);
int dvaa_itx_face_get_zone_confs_all(DVAAID id, ITX_FRZONE_CONF conf[16], int *cnt);
ITX_FRZONEID dvaa_itx_face_find_zone(DVAAID id, int x, int y);

int dvaa_itx_face_get_rule_prop(DVAAID id, ITX_FRRULE_PROP *prop);
int dvaa_itx_face_set_rule_prop(DVAAID id, ITX_FRRULE_PROP *prop);

int dvaa_itx_plateno_add_zone_line_default_template(DVAAID id, ITX_LPRZONEID zone);
int dvaa_itx_plateno_add_zone_area_default_template(DVAAID id, ITX_LPRZONEID zone);
int dvaa_itx_plateno_get_zone_shape(DVAAID id, ITX_LPRZONEID zone, ITX_LPRZONE_SHAPE *shape);
int dvaa_itx_plateno_set_zone_shape(DVAAID id, ITX_LPRZONEID zone, ITX_LPRZONE_SHAPE *shape); 
int dvaa_itx_plateno_get_zone_conf(DVAAID id, ITX_LPRZONEID zone, ITX_LPRZONE_CONF *conf);
int dvaa_itx_plateno_set_zone_conf(DVAAID id, ITX_LPRZONEID zone, ITX_LPRZONE_CONF *conf);
int dvaa_itx_plateno_get_zone_confs_all(DVAAID id, ITX_LPRZONE_CONF conf[16], int *cnt);
ITX_LPRZONEID dvaa_itx_plateno_find_zone(DVAAID id, int x, int y);

int dvaa_itx_plateno_get_rule_prop(DVAAID id, ITX_LPRRULE_PROP *prop);
int dvaa_itx_plateno_set_rule_prop(DVAAID id, ITX_LPRRULE_PROP *prop);

int dvaa_itx_activate_rule(DVAAID id, DIT_PAGE_E page);
int dvaa_itx_activate_all_rule(DVAAID id);
int dvaa_itx_deactivate_rule(DVAAID id, DIT_PAGE_E page);
int dvaa_itx_deactivate_all_rule(DVAAID id);
int dvaa_itx_activate_calb(DVAAID id);
int dvaa_itx_deactivate_calb(DVAAID id);
int dvaa_itx_activate_meta(DVAAID id, DVAA_META_E meta);
int dvaa_itx_activate_all_meta(DVAAID id);
int dvaa_itx_deactivate_meta(DVAAID id, DVAA_META_E meta);
int dvaa_itx_deactivate_all_meta(DVAAID id);
int dvaa_itx_deactivate_all(DVAAID id);

ITX_CNTRID dvaa_itx_get_counted_rule(DVAAID id, unsigned short countid);
int dvaa_itx_parse_event(DVAAID vaaid, ai_rule_event_t *pevt, int *zid, int *cid);
int dvaa_itx_timer_proc(DVAAID vaaid);
int dvaa_itx_all_external_rules_proc(DVAAID id, int cnt, aibox_rule_data *prules);
int dvaa_itx_lpr_external_rules_proc(DVAAID id, int cnt, aibox_rule_data *prules);
int dvaa_itx_fr_external_rules_proc(DVAAID id, int cnt, aibox_rule_data *prules);
int dvaa_itx_make_zone_blinking(DVAAID vaaid, ITX_ZONEID zone);



////////////////////////////////////////////////////////////
//
// protected functions  - meta data
//

int dvaa_itx_show_meta(DVAAID id, gint cnt, ai_obj_t* data);



////////////////////////////////////////////////////////////
//
// protected functions  - event handler
//
int _dvaa_itx_event_set_vmap(DVA_VMAP_INFO *vmapInfo, gpointer user_data);
int _dvaa_event_get_highlight_point(IGPOINT *figure_pt, int figure_cnt);
int _dvaa_event_trans_x_vpoint(int vcnvs, int rpoint, int rcnvs);
int _dvaa_event_trans_y_vpoint(int vcnvs, int rpoint, int rcnvs);
int _dvaa_event_get_cnvs_size(DVAAID dvaaid, int *cnvs_w, int *cnvs_h);
int _dvaa_event_find_figure_point(int mpt_x, int mpt_y, IGPOINT *figure_pt, int figure_cnt, int *point_index);
int _dvaa_event_move_highlight_mpt(int mpt_x, int mpt_y, DVAAE_POINT dmpt, IGPOINT *figure_pt, int figure_cnt);
int _dvaa_event_move_highlight_point(int mpt_x, int mpt_y, int pt_idx, IGPOINT *figure_pt, int figure_cnt);
int _dvaa_event_check_valid_point(int cnvs_w, int cnvs_h, IGPOINT *figure_pt, int figure_cnt, float scale_x, float scale_y);
int _dvaa_event_check_valid_point_degree(int cnvs_w, int cnvs_h, IGPOINT *figure_pt, int figure_cnt, float scale_x, float scale_y, int degree);
int _dvaa_event_get_valid_point(int cnvs_w, int cnvs_h, IGPOINT *figure_pt, int figure_cnt, float scale_x, float scale_y);
int _dvaa_event_get_valid_point_degree(int cnvs_w, int cnvs_h, IGPOINT *figure_pt, int figure_cnt, float scale_x, float scale_y, int degree);
int _dvaa_event_add_point(int mpt_x, int mpt_y, IGPOINT *figure_pt, int *figure_cnt);
int _dvaa_event_add_point_next_pt(int mpt_x, int mpt_y, IGPOINT *figure_pt, int *figure_cnt);
int _dvaa_event_del_point(int mpt_x, int mpt_y, IGPOINT *figure_pt, int *figure_cnt);
int _dvaa_itx_link_event(DVAAID id);

int dvaa_itx_remove_highlight(DVAAID dvaaid);
// int _dvaa_itx_event_set_vmap(DVA_VMAP_INFO *vmapInfo, gpointer user_data);

int _dvaa_itx_detector_event_mouse_left_press(DVA_SCREEN_INFO *scrInfo, DVA_MEVENT_PT *evt_pt, gpointer user_data);
int _dvaa_itx_detector_event_mouse_left_2press(DVA_SCREEN_INFO *scrInfo, DVA_MEVENT_PT *evt_pt, gpointer user_data);
int _dvaa_itx_detector_event_mouse_right_press(DVA_SCREEN_INFO *scrInfo, DVA_MEVENT_PT *evt_pt, gpointer user_data);
int _dvaa_itx_detector_event_mouse_right_2press(DVA_SCREEN_INFO *scrInfo, DVA_MEVENT_PT *evt_pt, gpointer user_data);
int _dvaa_itx_detector_event_mouse_left_drag(DVA_SCREEN_INFO *scrInfo, DVA_MEVENT_PT *evt_pt, gpointer user_data);
int _dvaa_itx_detector_event_select_ruleid(int type, int select_ruleid, gpointer user_data);
int _dvaa_itx_detector_remove_highlight(DVAAID dvaaid);

int _dvaa_itx_face_event_mouse_left_press(DVA_SCREEN_INFO *scrInfo, DVA_MEVENT_PT *evt_pt, gpointer user_data);
int _dvaa_itx_face_event_mouse_left_2press(DVA_SCREEN_INFO *scrInfo, DVA_MEVENT_PT *evt_pt, gpointer user_data);
int _dvaa_itx_face_event_mouse_right_press(DVA_SCREEN_INFO *scrInfo, DVA_MEVENT_PT *evt_pt, gpointer user_data);
int _dvaa_itx_face_event_mouse_right_2press(DVA_SCREEN_INFO *scrInfo, DVA_MEVENT_PT *evt_pt, gpointer user_data);
int _dvaa_itx_face_event_mouse_left_drag(DVA_SCREEN_INFO *scrInfo, DVA_MEVENT_PT *evt_pt, gpointer user_data);
int _dvaa_itx_face_event_select_ruleid(int type, int select_ruleid, gpointer user_data);
int _dvaa_itx_face_remove_highlight(DVAAID dvaaid);

int _dvaa_itx_plateno_event_mouse_left_press(DVA_SCREEN_INFO *scrInfo, DVA_MEVENT_PT *evt_pt, gpointer user_data);
int _dvaa_itx_plateno_event_mouse_left_2press(DVA_SCREEN_INFO *scrInfo, DVA_MEVENT_PT *evt_pt, gpointer user_data);
int _dvaa_itx_plateno_event_mouse_right_press(DVA_SCREEN_INFO *scrInfo, DVA_MEVENT_PT *evt_pt, gpointer user_data);
int _dvaa_itx_plateno_event_mouse_right_2press(DVA_SCREEN_INFO *scrInfo, DVA_MEVENT_PT *evt_pt, gpointer user_data);
int _dvaa_itx_plateno_event_mouse_left_drag(DVA_SCREEN_INFO *scrInfo, DVA_MEVENT_PT *evt_pt, gpointer user_data);
int _dvaa_itx_plateno_event_select_ruleid(int type, int select_ruleid, gpointer user_data);
int _dvaa_itx_plateno_remove_highlight(DVAAID dvaaid);

int dvaa_itx_notify_event(NF_NOTIFY_INFO *data);
int dvaa_itx_notify_generic_event(NF_NOTIFY_INFO *data);
int dvaa_itx_notify_track_info(NF_NOTIFY_INFO *data);
int dvaa_itx_notify_meta_data(NF_NOTIFY_INFO *data);
int dvaa_itx_notify_counter_info(NF_NOTIFY_INFO *data);
int dvaa_itx_notify_analyze_event(NF_NOTIFY_INFO *data);

#endif
