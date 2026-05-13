/*
 * vw_dvabx_live_log.c
 *
 * Written by JungKyu. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Feb 15, 2019
 *
 */

#include <glib.h>
#include "iux_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_page_manager.h"
#include "support/nf_ui_color.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "modules/ssm.h"
#include "modules/smt.h"
#include "modules/var.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nfimage.h"
#include "objects/nflabel.h"
#include "objects/nfvklabel.h"
#include "objects/nftab.h"
#include "objects/nfcheckbutton.h"
#include "viewers/objects/nflistbox.h"

#include "vw_dvabx_component.h"
#include "vw_dvabx_prop_internal.h"

#include "dvaa.h"
#include "dvaa_itx.h"



////////////////////////////////////////////////////////////
//
// private data types
//
/* Log table. */

#define	_LOG_COLS	    	        5
#define	_LOG_ROWS		            19
#define _LOG_TBL_GAP                1
#define	_LOG_TBL_X	            	10
#define	_LOG_TBL_Y	            	10
#define	_LOG_LABEL_W	        	155+170+153+(_LOG_TBL_GAP*2)
#define	_LOG_LABEL_H	        	36

enum {
	LOGLABEL_RULE_COLOR = 0,
	LOGLABEL_RULE_NAME,
	LOGLABEL_EVENT_ICON,
	LOGLABEL_EVENT_TYPE,
	LOGLABEL_TIME,
	LOGLABEL_ALL
};	




////////////////////////////////////////////////////////////
//
// private variable
//
static NFOBJECT *g_curwnd = NULL;
static NFOBJECT *g_parent = NULL;

static NFOBJECT *g_loglist_obj;




////////////////////////////////////////////////////////////
//
// private interfaces 
//

static gint _get_rule_name_string(DVAAID dvaaid, gint zone_id, gchar *str)
{
    ITX_DVAZONE_SHAPE shape;

    dvaa_itx_detector_get_zone_shape(dvaaid, zone_id, &shape);
    strcpy(str, shape.name);
    
    return 0;
}

static gint _get_rule_color_string(DVAAID dvaaid, gint zone_id, gchar *str)
{
    ITX_DVAZONE_SHAPE shape;

    dvaa_itx_detector_get_zone_shape(dvaaid, zone_id, &shape);
    g_sprintf(str, "%d.color_key", shape.color_idx);
    
    return 0;
}

static gint _get_cntr_name_string(DVAAID dvaaid, gint cntr_id, gchar *str)
{
    ITX_DVACNTR_SHAPE shape;
   
    dvaa_itx_detector_get_cntr_shape(dvaaid, cntr_id, &shape);
    strcpy(str, shape.name);
    
    return 0;
}

static gint _get_cntr_color_string(DVAAID dvaaid, gint cntr_id, gchar *str)
{
    ITX_DVACNTR_SHAPE shape;

    dvaa_itx_detector_get_cntr_shape(dvaaid, cntr_id, &shape);
    g_sprintf(str, "%d.color_key", shape.color_idx);
    
    return 0;
}

static gint _get_event_icon_string(guint type, gchar *str)
{
    if (type & IVCA_ET_DIR_POS)         strcpy(str, IMG_LIST_DIRECTION_FORWARD_N);
    else if (type & IVCA_ET_DIR_NEG)    strcpy(str, IMG_LIST_DIRECTION_REVERSE_N);
    else if (type & IVCA_ET_INTRUSION)  strcpy(str, IMG_LIST_INTRUSION_N);
    else if (type & IVCA_ET_ENTER)      strcpy(str, IMG_LIST_ENTER_N);
    else if (type & IVCA_ET_EXIT)       strcpy(str, IMG_LIST_EXIT_N);
    else if (type & IVCA_ET_STOPPED)    strcpy(str, IMG_LIST_STOP_N);
    else if (type & IVCA_ET_REMOVED)    strcpy(str, IMG_LIST_REMOVE_N);
    else if (type & IVCA_ET_LOITERED)   strcpy(str, IMG_LIST_LOITERING_N);
    else if (type & IVCA_ET_COUNTER)    strcpy(str, IMG_LIST_COUNT_N);  
            
    return 0;
}

static gint _get_event_type_string(guint type, gchar *str)
{
    if (type & IVCA_ET_DIR_POS)         strcpy(str, "VCA-FORWARD");
    else if (type & IVCA_ET_DIR_NEG)    strcpy(str, "VCA-REVERSE");
    else if (type & IVCA_ET_INTRUSION)  strcpy(str, "INTRUSION");
    else if (type & IVCA_ET_ENTER)      strcpy(str, "VCA-ENTER");
    else if (type & IVCA_ET_EXIT)       strcpy(str, "VCA-EXIT");
    else if (type & IVCA_ET_STOPPED)    strcpy(str, "VCA-STOPPED");
    else if (type & IVCA_ET_REMOVED)    strcpy(str, "VCA-REMOVED");
    else if (type & IVCA_ET_LOITERED)   strcpy(str, "VCA-LOITERING");       
    else if (type & IVCA_ET_COUNTER)    strcpy(str, "VCA-COUNTER");       
       
    return 0;
}

static gint _get_timestamp_string(time_t time, gchar *str)
{
    dtf_get_local_time(time, str);  
        
    return 0;
}


////////////////////////////////////////////////////////////
//
// handler
//

static gboolean post_livelog_list_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == INFY_AI_EVENT_NOTIFY)
    {
    	NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        DVAAID dvaaid;

        NF_NOTIFY_INFO *pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);
    	ai_rule_event_t *pevt;
    	gint *p;

        gchar *log_list[LOGLABEL_ALL];
        gchar strBuf[64];
        gint i, j;

    	p = pInfo->p.ptr;
    	pevt = p + 2;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
    	
    	if (p[0] >= 16) return FALSE;
    	if (p[0] != component_data->preview.ch) return FALSE;

		for(i = 0; i < p[1]; i++, pevt++) 
		{
            if (nfui_listbox_get_box_count((NFLISTBOX*)obj) > 200)
            {
                nfui_listbox_delete((NFLISTBOX*)obj, 0);
            }

            dvaaid = dvaa_get_dvaaid(component_data->preview.ch);

        	if (pevt->type & IVCA_ET_COUNTER) 
        	{
                memset(strBuf, 0x00, sizeof(strBuf));
                _get_cntr_color_string(dvaaid, pevt->rule_id, strBuf);
                log_list[LOGLABEL_RULE_COLOR] = g_strdup(strBuf);

                memset(strBuf, 0x00, sizeof(strBuf));
                _get_cntr_name_string(dvaaid, pevt->rule_id, strBuf);
                log_list[LOGLABEL_RULE_NAME] = g_strdup(strBuf);
        	}
        	else
        	{
        	    memset(strBuf, 0x00, sizeof(strBuf));
                _get_rule_color_string(dvaaid, pevt->rule_id, strBuf);
                log_list[LOGLABEL_RULE_COLOR] = g_strdup(strBuf);

                memset(strBuf, 0x00, sizeof(strBuf));
                _get_rule_name_string(dvaaid, pevt->rule_id, strBuf);
                log_list[LOGLABEL_RULE_NAME] = g_strdup(strBuf);
        	}

            memset(strBuf, 0x00, sizeof(strBuf));
            _get_event_icon_string(pevt->type, strBuf);
            log_list[LOGLABEL_EVENT_ICON] = g_strdup(strBuf);

            memset(strBuf, 0x00, sizeof(strBuf));
            _get_event_type_string(pevt->type, strBuf);
            log_list[LOGLABEL_EVENT_TYPE] = g_strdup(strBuf);

            memset(strBuf, 0x00, sizeof(strBuf));        
            _get_timestamp_string(pevt->timestamp, strBuf);
            log_list[LOGLABEL_TIME] = g_strdup(strBuf);

            nfui_listbox_set_text((NFLISTBOX*)obj, log_list);
        
            for (j = 0; j < LOGLABEL_ALL; j++)
            {
                ifree(log_list[j]);
            }
		}

        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }        
    else if (evt->type == GDK_DELETE)
    {
        uxm_unreg_imsg_event(obj, INFY_AI_EVENT_NOTIFY);        
    }

    return FALSE;
}



////////////////////////////////////////////////////////////
//
// protected interfaces 
//

#if 0
gint _dvabx_live_log_page(NFOBJECT *parent)
{
    NFOBJECT *fixed;
    NFOBJECT *obj;

	gint i, j, tbl_x, tbl_y;
	gchar *tbl_livelog_str[] = {"RULE NAME", "EVENT TYPE", "TIME"};
	guint tbl_livelog_w[5] = {155, 170, 153};
    gint opt;
    
    g_parent = parent;
	g_curwnd = (NFWINDOW *)nfui_nfobject_get_top(g_parent);

    gint fg_color[NFOBJECT_STATE_COUNT];
    gint bg_color[NFOBJECT_STATE_COUNT];

    tbl_x = _LOG_TBL_X;
    tbl_y = _LOG_TBL_Y;

	//	LOG TABLE

    fg_color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(389);
    fg_color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(251);
    fg_color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(251);
    fg_color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(389);

    bg_color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(200);
    bg_color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(250);
    bg_color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(250);
    bg_color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(200);  
    
	for(i = 0; i < 3; i++)
	{
    	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(tbl_livelog_str[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 10);
    	nfui_nfobject_set_size(obj, tbl_livelog_w[i] - 1, _LOG_LABEL_H);
    	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)g_parent, obj, tbl_x, tbl_y);
    	
    	tbl_x = tbl_x + tbl_livelog_w[i];
    }
    
    tbl_x = _LOG_TBL_X;
    tbl_y = _LOG_TBL_Y + _LOG_LABEL_H;

    tbl_livelog_w[LOGLABEL_RULE_COLOR] = 40;
    tbl_livelog_w[LOGLABEL_RULE_NAME] = 115;
    tbl_livelog_w[LOGLABEL_EVENT_ICON] = 40;
    tbl_livelog_w[LOGLABEL_EVENT_TYPE] = 130;    
    tbl_livelog_w[LOGLABEL_TIME] = 153;

    obj = nfui_listbox_new(LOGLABEL_ALL, tbl_livelog_w, _LOG_LABEL_H);
    nfui_listbox_set_skin_type(NF_LISTBOX(obj), NFLISTBOX_TYPE_POPUP_1);
    nfui_listbox_set_pango_font(NF_LISTBOX(obj), nffont_get_pango_font(NFFONT_SMALL_SEMI));
    nfui_listbox_set_fg_color(NF_LISTBOX(obj), fg_color);
    nfui_listbox_set_bg_color(NF_LISTBOX(obj), bg_color);   
    nfui_listbox_set_column_align(NF_LISTBOX(obj), LOGLABEL_RULE_NAME, NFALIGN_LEFT);
    nfui_listbox_set_column_align(NF_LISTBOX(obj), LOGLABEL_EVENT_TYPE, NFALIGN_CENTER);
    nfui_listbox_set_column_align(NF_LISTBOX(obj), LOGLABEL_TIME, NFALIGN_CENTER);
    nfui_listbox_set_draw_inline(NF_LISTBOX(obj), TRUE, COLOR_IDX(392));
    nfui_listbox_support_multi_lang(NF_LISTBOX(obj), FALSE);
    nfui_listbox_support_tooltip(NF_LISTBOX(obj), FALSE);
    nfui_nfobject_set_size(obj, _LOG_LABEL_W + 25, _LOG_LABEL_H*_LOG_ROWS);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_ON);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, tbl_x, tbl_y);  
    nfui_regi_post_event_callback(obj, post_livelog_list_event_cb);  
    g_loglist_obj = obj;

    uxm_reg_imsg_event(obj, INFY_AI_EVENT_NOTIFY);
    uxm_monitor_on_imsg_event(obj, INFY_AI_EVENT_NOTIFY);

    return 0;
}

void _dvabx_live_log_show(NFOBJECT *parent)
{
    nfui_nfobject_show(parent);
}

void _dvabx_live_log_hide(NFOBJECT *parent)
{
	NFOBJECT *topwin;

	topwin = nfui_nfobject_get_top(parent);	
	nfui_nfobject_hide(topwin);
}

gint _dvabx_live_log_clear()
{
    nfui_listbox_delete_all((NFLISTBOX*)g_loglist_obj);
    nfui_signal_emit(g_loglist_obj, GDK_EXPOSE, TRUE);

    return 0;
}
#endif
