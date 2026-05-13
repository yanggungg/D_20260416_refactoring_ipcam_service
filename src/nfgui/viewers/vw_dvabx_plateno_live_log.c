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

#include "nf_meta_data.h"



////////////////////////////////////////////////////////////
//
// private data types
//

typedef struct _TESTLOG_DATA_T {
    GdkPixbuf   *event_pbuf;
} TESTLOG_DATA_T;





////////////////////////////////////////////////////////////
//
// private variable
//
static NFOBJECT *g_curwnd = NULL;
static NFOBJECT *g_parent = NULL;

static GList *g_testlog_list = 0;



////////////////////////////////////////////////////////////
//
// private interfaces 
//

static int _free_pixbuf_data(gpointer data, gpointer user_data)
{
    TESTLOG_DATA_T *testlog_data = (TESTLOG_DATA_T*)data;

    if (!data) return -1;

    if (testlog_data->event_pbuf) g_object_unref(testlog_data->event_pbuf);
	ifree(data);
	return 0;
}

static GdkPixbuf *_get_target_pixbuf(gint jpeg_w, gint jpeg_h, GdkPixbuf *org_pixbuf, gfloat coords[4])
{
	GdkDrawable *drawable = NULL;

    gfloat top_x = 0 , top_y = 0;
    gfloat bottom_x = 0, bottom_y = 0;

	gfloat obj_w, obj_h;
	gfloat objrate;

    GdkPixbuf *src_pixbuf = org_pixbuf;
	GdkPixbuf *copy_pixbuf = NULL;
	GdkPixbuf *dst_pixbuf = NULL;

	gint snap_width = jpeg_w;
	gint snap_height = jpeg_h;

	top_x = MAX(coords[0], 0);
	top_y = MAX(coords[1], 0);
	bottom_x = MIN(coords[2], 1);
	bottom_y = MIN(coords[3], 1);

	obj_w = snap_width*(bottom_x-top_x);
	obj_h = snap_height*(bottom_y-top_y);

	if (71/obj_w >= 71/obj_h) objrate = 71/obj_h;
	else objrate = 71/obj_w;

    g_message("%s, %d, w:%d, h:%d", __FUNCTION__, __LINE__, (gint)((bottom_x-top_x)*snap_width), (gint)((bottom_y-top_y)*snap_height));

	copy_pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, (gint)((bottom_x-top_x)*snap_width), (gint)((bottom_y-top_y)*snap_height));
	gdk_pixbuf_fill (copy_pixbuf, 0x000000ff);
	gdk_pixbuf_copy_area(src_pixbuf, (gint)(top_x*snap_width), (gint)(top_y*snap_height), 
			(gint)((bottom_x-top_x)*snap_width), (gint)((bottom_y-top_y)*snap_height), copy_pixbuf, 0, 0); 	

	dst_pixbuf = gdk_pixbuf_scale_simple(copy_pixbuf, (gint)(objrate*obj_w), (gint)(objrate*obj_h), GDK_INTERP_HYPER); 

	g_object_unref(copy_pixbuf);	
	return dst_pixbuf;
}

static gint _insert_lpr_pixbuf(gint ch, gint cnt, ai_lpr_event_t *pevt)
{
    GInputStream *stream = 0;
	GdkPixbuf *npixbuf = 0;
	gint jpeg_w, jpeg_h;
	gint jpeg_size;
	gchar *out_buffer = 0;
	gboolean retVal;
    gfloat coords[4];

	GTimeVal stime, etime, ftime;

    TESTLOG_DATA_T *testlog_data;
    gint buff_cnt, i;
    GList *plist;

    gint img_w, img_h;

	stime.tv_sec = (glong)pevt[0].timestamp;
	stime.tv_usec = (glong)pevt[0].timestampl;
	etime.tv_sec = stime.tv_sec+1;
	etime.tv_usec = stime.tv_usec;

	retVal = nf_play_get_thumbnail_jpeg(ch, stime, etime, &jpeg_w, &jpeg_h, &jpeg_size, &out_buffer, &ftime);
	if (retVal) {
		stream = g_memory_input_stream_new_from_data(out_buffer, jpeg_size, NULL);
		npixbuf = gdk_pixbuf_new_from_stream(stream, NULL, NULL);
	}

	for (i = 0; i < cnt; i++)
	{
        buff_cnt = g_list_length(g_testlog_list);
        if (buff_cnt > 200) 
        {
            plist = g_list_nth(g_testlog_list, 0);
            testlog_data = (TESTLOG_DATA_T*)plist->data;

            if (testlog_data->event_pbuf) g_object_unref(testlog_data->event_pbuf);

            ifree(plist->data);
            g_testlog_list = g_list_delete_link(g_testlog_list, plist);
        }

		testlog_data = imalloc(sizeof(TESTLOG_DATA_T));

        if (npixbuf) 
        {
            img_w = gdk_pixbuf_get_width(npixbuf);
            img_h = gdk_pixbuf_get_height(npixbuf);
            g_message("%s, %d, w:%d, h:%d", __FUNCTION__, __LINE__, img_w, img_h);

            coords[0] = pevt[i].bbx_position[0];
            coords[1] = pevt[i].bbx_position[1];
            coords[2] = pevt[i].bbx_position[2];
            coords[3] = pevt[i].bbx_position[3];
            testlog_data->event_pbuf = _get_target_pixbuf(img_w, img_h, npixbuf, coords);
        }

        g_testlog_list = g_list_append(g_testlog_list, testlog_data);
	}

	if (out_buffer) free(out_buffer);
	if (npixbuf) g_object_unref(npixbuf);
	if (stream) g_object_unref(stream);

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
    if ((evt->type == GDK_EXPOSE) || (evt->type == GDK_BUTTON_PRESS) || (evt->type == NFEVENT_LISTBOX_CHANGED))
    {
        GdkDrawable *drawable = NULL;
        GdkGC *gc;

        TESTLOG_DATA_T *testlog_data;
        gint i, list_idx;
        gint gap_x, gap_y;

        gint img_w, img_h;

        drawable = nfui_nfobject_get_window(obj);
        gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);

        nfui_listbox_get_index_by_box_row((NFLISTBOX*)obj, i);

        for (i = 0; i < ((NFLISTBOX*)obj)->box_row_cnt; i++)
        {
            list_idx = nfui_listbox_get_index_by_box_row((NFLISTBOX*)obj, i);
            if (list_idx >= g_list_length(g_testlog_list)) break;

            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(200));
            gdk_draw_rectangle(drawable, gc, TRUE, gap_x+1, gap_y+1, 71, 71);	

            testlog_data = (TESTLOG_DATA_T*)g_list_nth_data(g_testlog_list, list_idx);
            if (testlog_data->event_pbuf) {
                img_w = gdk_pixbuf_get_width(testlog_data->event_pbuf);
                img_h = gdk_pixbuf_get_height(testlog_data->event_pbuf);
                gdk_draw_pixbuf(drawable, gc, testlog_data->event_pbuf, 0, 0, gap_x+2+(71-img_w)/2, gap_y+2+(71-img_h)/2, -1, -1, GDK_RGB_DITHER_NONE, 0, 0);
            }

            gap_y += 75;
        }

        nfui_nfobject_gc_unref(gc);
    }
    else if (evt->type == INFY_DELAY_AI_PLATENO_EVENT)
    {
    	NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        DVAAID dvaaid;

    	gint *p = ((CMM_MESSAGE_T *)data)->data;
    	ai_lpr_event_t *pevt;

        gchar strBuf[64];
        gint i, j;

        gchar *log_list[3];

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        pevt = p + 2;

        if (p[0] >= 16) return FALSE;
        if (p[0] != component_data->preview.ch) return FALSE;

        _insert_lpr_pixbuf(p[0], p[1], pevt);

		for(i = 0; i < p[1]; i++, pevt++) 
		{
            if (nfui_listbox_get_box_count((NFLISTBOX*)obj) > 200) {
                nfui_listbox_delete((NFLISTBOX*)obj, 0);
            }

            log_list[0] = 0;

            memset(strBuf, 0x00, sizeof(strBuf));
            snprintf(strBuf, sizeof(strBuf), "%s", pevt->lp_text);
            log_list[1] = g_strdup(strBuf);

            memset(strBuf, 0x00, sizeof(strBuf));        
            _get_timestamp_string(pevt->timestamp, strBuf);
            log_list[2] = g_strdup(strBuf);

            nfui_listbox_set_text((NFLISTBOX*)obj, log_list);
        
            for (j = 0; j < 3; j++)
            {
                if (log_list[j]) ifree(log_list[j]);
            }
        }

        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }
    else if (evt->type == GDK_DELETE)
    {
        g_list_foreach(g_testlog_list, _free_pixbuf_data, 0);
        g_list_free(g_testlog_list);
        g_testlog_list = 0;

        uxm_unreg_imsg_event(obj, INFY_DELAY_AI_PLATENO_EVENT);        
    }        

    return FALSE;
}



////////////////////////////////////////////////////////////
//
// protected interfaces 
//

#if 0
gint _dvabx_plateno_live_log_page(NFOBJECT *parent)
{
    NFOBJECT *fixed;
    NFOBJECT *obj;

    gint fg_color[NFOBJECT_STATE_COUNT];
    gint bg_color[NFOBJECT_STATE_COUNT];

	gint i, j, pos_x, pos_y;
	gchar *tbl_livelog_str[] = {"EVENT", "TIME"};
	guint tbl_livelog_w[3] = {350, 130};

    g_parent = parent;
	g_curwnd = (NFWINDOW *)nfui_nfobject_get_top(g_parent);

    pos_x = 10;
    pos_y = 10;

    fg_color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(389);
    fg_color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(251);
    fg_color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(251);
    fg_color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(389);

    bg_color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(200);
    bg_color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(250);
    bg_color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(250);
    bg_color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(200);  
    
	for(i = 0; i < 2; i++)
	{
    	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(tbl_livelog_str[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 10);
    	nfui_nfobject_set_size(obj, tbl_livelog_w[i] - 1, 36);
    	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)g_parent, obj, pos_x, pos_y);
    	
    	pos_x = pos_x + tbl_livelog_w[i];
    }
    
    pos_x = 10;
    pos_y += 36;

    tbl_livelog_w[0] = 75;
    tbl_livelog_w[1] = 274;
    tbl_livelog_w[2] = 130;

    obj = nfui_listbox_new(3, tbl_livelog_w, 75);
    nfui_listbox_set_skin_type(NF_LISTBOX(obj), NFLISTBOX_TYPE_POPUP_1);
    nfui_listbox_set_pango_font(NF_LISTBOX(obj), nffont_get_pango_font(NFFONT_MINI_SEMI_4));
    nfui_listbox_set_fg_color(NF_LISTBOX(obj), fg_color);
    nfui_listbox_set_bg_color(NF_LISTBOX(obj), bg_color);   
    nfui_listbox_set_column_align(NF_LISTBOX(obj), 0, NFALIGN_LEFT);
    nfui_listbox_set_column_align(NF_LISTBOX(obj), 1, NFALIGN_LEFT);
    nfui_listbox_set_column_align(NF_LISTBOX(obj), 2, NFALIGN_CENTER);
    nfui_listbox_set_draw_inline(NF_LISTBOX(obj), TRUE, COLOR_IDX(392));
    nfui_listbox_support_multi_lang(NF_LISTBOX(obj), FALSE);
    nfui_listbox_support_tooltip(NF_LISTBOX(obj), FALSE);
    nfui_nfobject_set_size(obj, 480 + 25, 75*9);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, pos_x, pos_y);  
    nfui_regi_post_event_callback(obj, post_livelog_list_event_cb);  

    uxm_reg_imsg_event(obj, INFY_DELAY_AI_PLATENO_EVENT);
    uxm_monitor_on_imsg_event(obj, INFY_DELAY_AI_PLATENO_EVENT);

    return 0;
}

void _dvabx_plateno_live_log_show(NFOBJECT *parent)
{
    nfui_nfobject_show(parent);
}

void _dvabx_plateno_live_log_hide(NFOBJECT *parent)
{
	NFOBJECT *topwin;

	topwin = nfui_nfobject_get_top(parent);	
	nfui_nfobject_hide(topwin);
}

gint _dvabx_plateno_live_log_clear()
{


    return 0;
}
#endif
