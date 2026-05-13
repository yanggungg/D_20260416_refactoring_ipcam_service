/*
 * vw_dvabx_component.h
 *
 * Written by Jungkyu. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, June 14, 2014
 *
 */

#ifndef	__DVABOX_COMPONENT_H
#define __DVABOX_COMPONENT_H

#include "dit.h"
#include "vw_dit_dva.h"
#include "nf_ipcam_defs.h"

typedef int (*COMPONENT_ACTION_CALLBACK)(gpointer user_data);


#define DEFAULT_COMPONENT_WIDTH         (525)
#define DEFAULT_COMPONENT_HEIGHT        (921)

#define VIDEO_COMPONENT_16_9_W          (16*58)
#define VIDEO_COMPONENT_16_9_H          (9*58)

#define VIDEO_COMPONENT_4_3_W           (12*58)
#define VIDEO_COMPONENT_4_3_H           (9*58)

#define VIDEO_COMPONENT_WIDTH           (VIDEO_COMPONENT_16_9_W)
#define VIDEO_COMPONENT_HEIGHT          (VIDEO_COMPONENT_16_9_H)

#define DVA_COMPONENT_DATA              "dva_component_data"
#define DVA_COMPONENT_ACTION            "dva_component_action"

#define ALGOTYPE_DETECTOR               "mot"
#define ALGOTYPE_FACE                   "fr"
#define ALGOTYPE_PLATENO                "lpr"

enum
{
    RTYPE_LINE       = 0,
    RTYPE_AREA,
    RTYPE_COUNTER,
};

enum
{
    NAME_ZONE = 0,
    NAME_COUNTER,
    NAME_CALIBRATION,

    NAME_TYPE_CNT
};


typedef struct _COMPONENT_PREVIEW_T {
    gint        type;           // 0 : live, 1 : playback
    time_t      play_from;      // playback from time
    time_t      play_to;        // playback to time
    gint        play_mode;      // playback mode, 0 : show frame, 1 : meta, 2 : preview
    gint        ch;
    gint        onoff;
    gint        bg_color;
    gint        calb_onoff;
} COMPONENT_PREVIEW_T;

typedef struct _COMPONENT_RULEDISP_T {
    gint        force_update;
    gint        block_update;
    gint        delay_update;
    gint        delay_max;    
    gint        delay_cnt;        
    DVA_CLON    clon;    
} COMPONENT_RULEDISP_T;

typedef struct _COMPONENT_CALIBRATION_T {
    gint        select_icon;
    gint        icon_count;
    gfloat      icon_height;            // cm
    gint        pause_video;
    gfloat      camera_height;
    gint        camera_tilt;
    gint        param_valid;
} COMPONENT_CALIBRATION_T;

typedef struct _COMPONENT_LINE_T {
    gchar       name[32];
    gint        active;
    gint        all_detect_obj;
    gchar       interest_obj[256];
    gint        forward;
    gint        reverse;
    gint        display_color_idx;
    gint        use_filter_color;
    gint        filter_color_idx;
    gint        filter_color_percnt;
    gint        use_filter_size;
    gfloat      filter_min_size_w;
    gfloat      filter_max_size_w;
    gfloat      filter_min_size_h;
    gfloat      filter_max_size_h;
    gint        use_filter_speed;
    gfloat      filter_min_speed;
    gfloat      filter_max_speed;
    gint        c_threshold;
    gchar       event_audio[8][256];
} COMPONENT_LINE_T;

typedef struct _COMPONENT_AREA_T {
    gchar       name[32];
    gint        active;
    gint        all_detect_obj;
    gchar       interest_obj[256];
    gint        intrusion;    
    gint        enter;
    gint        exit;
    gint        removed;
    gint        loitering;
    gint        stopped;
    gint        display_color_idx;
    gint        stopped_val;
    gint        removed_val;
    gint        loitering_val;
    gint        use_filter_color;
    gint        filter_color_idx;
    gint        filter_color_percnt;
    gint        use_filter_size;
    gfloat      filter_min_size_w;
    gfloat      filter_max_size_w;
    gfloat      filter_min_size_h;
    gfloat      filter_max_size_h;
    gint        use_filter_speed;
    gfloat      filter_min_speed;
    gfloat      filter_max_speed;
    gint        c_threshold;
    gchar       event_audio[8][256];
} COMPONENT_AREA_T;

typedef struct _COMPONENT_COUNTER_T {
    gchar       name[32];
    gint        active;
    gint        display_color_idx;
    gint        use_counter_event;
    gint        counter_event_val;
    gint        use_reset_value;
    gint        up_source;
    gint        down_source;

    gchar       source_list[16][32];
} COMPONENT_COUNTER_T;

typedef struct _COMPONENT_OPTION_T {
    gint        shadow_removal;
    gint        track_reference;
    gint        minimum_object;
    gfloat      minimum_w;              // mm
    gfloat      minimum_h;              // mm
    gint        static_filter;
    gint        static_filter_sense;
    gint        disp_obj_box;
    gint        disp_obj_traj;
    gint        disp_obj_id;
    gint        disp_obj_w;
    gint        disp_obj_h;
    gint        disp_obj_speed;
    gint        disp_rules;
} COMPONENT_OPTION_T;

typedef struct _DVA_COMPONENT_DATA_T {
    gint                    act_license;
    gint                    act_capable;
    gint                    en_engine;

	gint                    max_ratio_w;
	gint                    max_ratio_h;
	gint                    stream_ratio_w;
	gint                    stream_ratio_h;
	gint                    corridor_mode;

    gint                    aibox_alive;
    guint                   aibox_addr;
    gchar                   aibox_owner[32];
    gchar                   algorithm_value[256];
    gchar                   algorithm_text[256];
    gchar                   algorithm_type[256];
    
    gint                    disable_event;    
    NFOBJECT                *video_fixed;
    
    gint                    check_config;
    COMPONENT_PREVIEW_T     preview;
    COMPONENT_RULEDISP_T    disp_rule;

    gint                    unit_setup;

    gint                    skip_calibration;
    COMPONENT_CALIBRATION_T calibration;

    gint                    zone_id;        // get vaa itx module. init : -1
    gint                    cntr_id;        // get vaa itx module. init : -1

    gint                    rule_type;      // 0 : RTYPE_LINE, 1 : RTYPE_AREA, 2 : RTYPE_COUNTER
    gint                    use_zone;
    gint                    use_counter;
    
    COMPONENT_LINE_T        line;
    COMPONENT_AREA_T        area;
    COMPONENT_COUNTER_T     counter;
    
    COMPONENT_OPTION_T      option;
} DVA_COMPONENT_DATA_T;

typedef struct _VCA_COMPONENT_ACTION_T {
    COMPONENT_ACTION_CALLBACK       calibration_add;
    COMPONENT_ACTION_CALLBACK       calibration_delete;
    COMPONENT_ACTION_CALLBACK       calibration_reset;
    COMPONENT_ACTION_CALLBACK       calibration_zoom;
    COMPONENT_ACTION_CALLBACK       calibration_pause;  
    COMPONENT_ACTION_CALLBACK       calibration_height;  

    COMPONENT_ACTION_CALLBACK       rule_type_cb;
    COMPONENT_ACTION_CALLBACK       rule_act_cb;

    COMPONENT_ACTION_CALLBACK       line_name_cb;
    COMPONENT_ACTION_CALLBACK       line_dis_color_cb;
    COMPONENT_ACTION_CALLBACK       line_filter_color_cb;
    COMPONENT_ACTION_CALLBACK       line_connected_counter_cb;
    
    COMPONENT_ACTION_CALLBACK       area_name_cb;
    COMPONENT_ACTION_CALLBACK       area_dis_color_cb;
    COMPONENT_ACTION_CALLBACK       area_filter_color_cb;
    COMPONENT_ACTION_CALLBACK       area_connected_counter_cb;

    COMPONENT_ACTION_CALLBACK       counter_use_cb;
    COMPONENT_ACTION_CALLBACK       counter_name_cb;
    COMPONENT_ACTION_CALLBACK       counter_dis_color_cb;

    COMPONENT_ACTION_CALLBACK       option_cb;
    COMPONENT_ACTION_CALLBACK       option_default_cb;
} DVA_COMPONENT_ACTION_T;

enum {
    COLOR_FF0000 = 0xff,
    COLOR_FF3F00 = 0x3fff,
    COLOR_FF7F00 = 0x7fff,
    COLOR_FFBF00 = 0xbfff,
    COLOR_FFFF00 = 0xffff,
    COLOR_BFFF00 = 0xffbf,
    COLOR_202020 = 0x202020,

    COLOR_7FFF00 = 0xff7f,
    COLOR_3FFF00 = 0xff3f,
    COLOR_00FF00 = 0xff00,
    COLOR_00FF3F = 0x3fff00,
    COLOR_00FF7F = 0x7fff00,
    COLOR_00FFBF = 0xbfff00,
    COLOR_606060 = 0x606060,

    COLOR_00FFFF = 0xffff00,
    COLOR_00BFFF = 0xffbf00,
    COLOR_007FFF = 0xff7f00,
    COLOR_003FFF = 0xff3f00,
    COLOR_0000FF = 0xff0000,
    COLOR_3F00FF = 0xff003f,
    COLOR_A0A0A0 = 0xa0a0a0,

    COLOR_7F00FF = 0xff007f,
    COLOR_BF00FF = 0xff00bf,
    COLOR_FF00FF = 0xff00ff,
    COLOR_FF00BF = 0xbf00ff,
    COLOR_FF007F = 0x7f00ff,
    COLOR_FF003F = 0x3f00ff,
    COLOR_FFFFFF = 0xffffff
};




////////////////////////////////////////////////////////////
//
// public interfaces
//



////////////////////////////////////////////////////////////
// VIDEO COMPONENT

gint vw_dvabx_video_component_open(NFOBJECT *parent, guint opt);
gint vw_dvabx_video_component_show();
gint vw_dvabx_video_component_hide();
gint vw_dvabx_video_component_sync_data(NFOBJECT *parent);
gint vw_dvabx_video_component_expose(NFOBJECT *parent);


////////////////////////////////////////////////////////////
// UNIT SETUP COMPONENT

enum {
    OPT_UNIT_SETUP_HELP         = 0,
    
};

gint vw_dvabx_unit_setup_component_open(NFOBJECT *parent, guint opt);
gint vw_dvabx_unit_setup_component_show();
gint vw_dvabx_unit_setup_component_hide();
gint vw_dvabx_unit_setup_component_sync_data(NFOBJECT *parent);
gint vw_dvabx_unit_setup_component_expose(NFOBJECT *parent);


////////////////////////////////////////////////////////////
// 3D CALIBRATION SETUP COMPONENT (use)

enum {
    OPT_CALIBRATION_SETUP_HELP         = 0,
    
};

gint vw_dvabx_calibration_setup_component_open(NFOBJECT *parent, guint opt);
gint vw_dvabx_calibration_setup_component_show();
gint vw_dvabx_calibration_setup_component_hide();
gint vw_dvabx_calibration_setup_component_sync_data(NFOBJECT *parent);
gint vw_dvabx_calibration_setup_component_expose(NFOBJECT *parent);


////////////////////////////////////////////////////////////
// 3D CALIBRATION ABRZ COMPONENT (calibration icon add/delete/reset, zoom)

gint vw_dvabx_calibration_adrz_component_open(NFOBJECT *parent, guint opt);
gint vw_dvabx_calibration_adrz_component_show();
gint vw_dvabx_calibration_adrz_component_hide();
gint vw_dvabx_calibration_adrz_component_sync_data(NFOBJECT *parent);
gint vw_dvabx_calibration_adrz_component_expose_data(NFOBJECT *parent);
gint vw_dvabx_calibration_adrz_component_expose(NFOBJECT *parent);


////////////////////////////////////////////////////////////
// 3D CALIBRATION RESULT COMPONENT (calibration result)

gint vw_dvabx_calibration_result_component_open(NFOBJECT *parent, guint opt);
gint vw_dvabx_calibration_result_component_show();
gint vw_dvabx_calibration_result_component_hide();
gint vw_dvabx_calibration_result_component_sync_data(NFOBJECT *parent);
gint vw_dvabx_calibration_result_component_expose(NFOBJECT *parent);


////////////////////////////////////////////////////////////
// RULE-SETUP COMPONENT (select rule, line or area, etc..)

enum {
    OPT_RULE_SETUP_NONE = 0,
    OPT_RULE_SETUP_LINE,
    OPT_RULE_SETUP_AREA,
    OPT_RULE_SETUP_COUNTER,
    OPT_RULE_SETUP_HELP, 
};

gint vw_dvabx_rule_setup_component_open(NFOBJECT *parent, guint opt);
gint vw_dvabx_rule_setup_component_show();
gint vw_dvabx_rule_setup_component_hide();
gint vw_dvabx_rule_setup_component_sync_data(NFOBJECT *parent);
gint vw_dvabx_rule_setup_component_expose(NFOBJECT *parent);


////////////////////////////////////////////////////////////
// RULE-LINE COMPONENT (rule line setup)
enum {
    OPT_LINE_ACTIVE_CHECK   = 0,
    OPT_LINE_CONNECTED_COUNTER,
    OPT_LINE_SETUP_HELP,
};

gint vw_dvabx_rule_line_component_open(NFOBJECT *parent, guint opt);
gint vw_dvabx_rule_line_component_show();
gint vw_dvabx_rule_line_component_hide();
gint vw_dvabx_rule_line_component_sync_data(NFOBJECT *parent);
gint vw_dvabx_rule_line_component_expose(NFOBJECT *parent);


////////////////////////////////////////////////////////////
// RULE-AREA COMPONENT (rule area setup)
enum {
    OPT_AREA_ACTIVE_CHECK   =0,
    OPT_AREA_CONNECTED_COUNTER,
    OPT_AREA_SETUP_HELP,
};

gint vw_dvabx_rule_area_component_open(NFOBJECT *parent, guint opt);
gint vw_dvabx_rule_area_component_show();
gint vw_dvabx_rule_area_component_hide();
gint vw_dvabx_rule_area_component_sync_data(NFOBJECT *parent);
gint vw_dvabx_rule_area_component_expose(NFOBJECT *parent);


////////////////////////////////////////////////////////////
// COUNTER COMPONENT (counter setup)
enum {
    OPT_COUNTER_USE_RADIO  = 0,
    OPT_COUNTER_ACTIVE_CHECK,
    OPT_COUNTER_SOURCE_COMBO,
    OPT_COUNTER_SETUP_HELP,
};

gint vw_dvabx_counter_component_open(NFOBJECT *parent, guint opt);
gint vw_dvabx_counter_component_show();
gint vw_dvabx_counter_component_hide();
gint vw_dvabx_counter_component_sync_data(NFOBJECT *parent);
gint vw_dvabx_counter_component_expose(NFOBJECT *parent);


////////////////////////////////////////////////////////////
// OPTION COMPONENT (option setup)
enum {
    OPT_OPTION_SHOW_UNIT  = 0,
    OPT_OPTION_SHOW_ENGINE,
    OPT_OPTION_SHOW_OSD,
};

gint vw_dvabx_option_component_open(NFOBJECT *parent, guint opt);
gint vw_dvabx_option_component_show();
gint vw_dvabx_option_component_hide();
gint vw_dvabx_option_component_sync_data(NFOBJECT *parent);
gint vw_dvabx_option_component_expose(NFOBJECT *parent);


#endif

