/*
 * nfpiechart.c
 *
 * Written by Jungkyu. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Feb 4, 2014
 *
 */

#include "../../support/util.h"
#include "../../support/color.h"

#include "ix_mem.h"
#include "nfobject.h"
#include "nfpiechart.h"




////////////////////////////////////////////////////////////
//
// private data types
//

#define CRATE(cval, mval)       ((cval*100/mval) * 64 * (-3.6))





////////////////////////////////////////////////////////////
//
// private variable
//

static gint _draw_chart_arc(NFPIECHART *nfpiechart, GdkDrawable *drawable, GdkGC *gc, gint off_x, gint off_y)
{
    gint i;
    gint ang1 = 0;
    gint ang2 = 360*64; 

    gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(nfpiechart->arcbg_color));
    gdk_draw_arc(drawable, gc, TRUE, off_x, off_y, nfpiechart->pie_diam, nfpiechart->pie_diam, ang1, ang2);

    ang1 = 90*64;
    ang2 = 0;

    for (i = 0; i < nfpiechart->cnt_chart; i++)
    {   
        ang2 = CRATE(nfpiechart->chart_value[i], nfpiechart->max_value);
        
        gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(nfpiechart->chart_color[i]));
        gdk_draw_arc(drawable, gc, TRUE, off_x, off_y, nfpiechart->pie_diam, nfpiechart->pie_diam, ang1, ang2);

        ang1 += ang2;
    }

    return 0;
}

static gint _draw_penetrate_arc(NFPIECHART *nfpiechart, GdkDrawable *drawable, GdkGC *gc, gint off_x, gint off_y)
{
    gint ang1 = 0;
    gint ang2 = 360*64;
    gint bg_color;

    if (!nfpiechart->punch_diam) return -1;

    bg_color = nfui_nfobject_get_bg_color((NFOBJECT*)nfpiechart, NFOBJECT_STATE_NORMAL);

    gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_color));
    gdk_draw_arc(drawable, gc, TRUE, off_x, off_y, nfpiechart->punch_diam, nfpiechart->punch_diam, ang1, ang2);

    return 0;
}

static gint _draw_text(NFPIECHART *nfpiechart, GdkDrawable *drawable, GdkGC *gc, gint off_x, gint off_y)
{
    gint valid_cnt = 0;
    gchar strTemp[128];
    gint bg_color;

    if (!strlen(nfpiechart->strLabel)) return -1;

    bg_color = nfui_nfobject_get_bg_color((NFOBJECT*)nfpiechart, NFOBJECT_STATE_NORMAL);
    
//  valid_cnt = nfutil_string_get_valid_count(0, drawable, nfpiechart->pango_font, nfpiechart->strLabel, nfpiechart->punch_diam);
    nfutil_draw_short_text_eng(0, 0, &UX_COLOR(bg_color), drawable, gc, nfpiechart->strLabel, valid_cnt, 
        off_x, off_y, nfpiechart->punch_diam, nfpiechart->punch_diam, nfpiechart->pango_font, &UX_COLOR(nfpiechart->font_color), 0, NFALIGN_CENTER, 0, 0, 0);
    
    return 0;
}

////////////////////////////////////////////////////////////
//
// private interfaces 
//

static gboolean nfpiechart_event_handler(NFPIECHART *nfpiechart, GdkEvent *event, gpointer data)
{
    switch(event->type)
    {
        case GDK_EXPOSE:
        {          
            GdkDrawable *drawable;
            GdkGC *gc;
            gint i;
            gint off_x, off_y;
       
            drawable = nfui_nfobject_get_window((NFOBJECT*)nfpiechart);
            gc = nfui_nfobject_get_gc((NFOBJECT*)nfpiechart);


            nfui_nfobject_get_offset((NFOBJECT*)nfpiechart, &off_x, &off_y);

            off_x += (nfpiechart->object.width - nfpiechart->pie_diam)/2;
            off_y += (nfpiechart->object.height - nfpiechart->pie_diam)/2;
            _draw_chart_arc(nfpiechart, drawable, gc, off_x, off_y);

            off_x += (nfpiechart->pie_diam - nfpiechart->punch_diam)/2;
            off_y += (nfpiechart->pie_diam - nfpiechart->punch_diam)/2;
            _draw_penetrate_arc(nfpiechart, drawable, gc, off_x, off_y);

            _draw_text(nfpiechart, drawable, gc, off_x, off_y);

            nfui_nfobject_gc_unref(gc);         
        }
        break;

        case GDK_DELETE:
        {
            if (nfpiechart->nbuf) g_object_unref(nfpiechart->nbuf);
        }
        break;

        default :
        break;
    }

    return FALSE;
}







////////////////////////////////////////////////////////////
//
// public interfaces
//


NFPIECHART* nfui_nfpiechart_new(gint pie_diameter, gint cnt_chart, gint max_value)
{
    NFPIECHART *nfpiechart;
    gint i;

    nfpiechart = (NFPIECHART*)imalloc(sizeof(NFPIECHART));
    
    if ((nfpiechart == NULL) || (max_value == 0))
    {
        g_warning("[NFPIECHART CREATE ERROR]\n");
        return NULL;
    }

    nfui_nfobject_init((NFOBJECT*)nfpiechart);

    nfpiechart->object.width = 100;
    nfpiechart->object.height = 100;
    nfpiechart->object.type = NFOBJECT_TYPE_NFPIECHART;
    nfpiechart->object.default_event_handler = nfpiechart_event_handler;

    nfpiechart->pie_diam = pie_diameter;
    nfpiechart->cnt_chart = cnt_chart;
    nfpiechart->max_value = max_value;

    nfpiechart->arcbg_color = COLOR_PRG_IDX(UX_COLOR_808080);

    for (i = 0; i < MAX_PARC_CATE; i++)
    {
        nfpiechart->chart_color[i] = COLOR_PRG_IDX(UX_COLOR_0000FF);
    }

    return nfpiechart;
}

gint nfui_nfpiechart_set_arcbg_color(NFPIECHART *nfpiechart, guint color_idx)
{
    if (nfpiechart->object.type != NFOBJECT_TYPE_NFPIECHART) return -1;

    nfpiechart->arcbg_color = color_idx;
    return 0;
}

gint nfui_nfpiechart_set_chart_color(NFPIECHART *nfpiechart, gint chart_idx, guint color_idx)
{
    if (nfpiechart->object.type != NFOBJECT_TYPE_NFPIECHART) return -1;

    nfpiechart->chart_color[chart_idx] = color_idx;
    return 0;
}

/*
gint nfui_nfpiechart_set_arcbg_image(NFPIECHART *nfpiechart, GdkPixbuf *pbuf)
{
    if (nfpiechart->object.type != NFOBJECT_TYPE_NFPIECHART) return -1;
    
    nfpiechart->bg_img = pbuf;
    return 0;
}

gint nfui_nfpiechart_set_chart_image(NFPIECHART *nfpiechart, gint chart_idx, GdkPixbuf *pbuf)
{
    if (nfpiechart->object.type != NFOBJECT_TYPE_NFPIECHART) return -1;

    nfpiechart->chart_img[chart_idx] = pbuf;
    return 0;
}
*/

gint nfui_nfpiechart_set_pango_font(NFPIECHART *nfpiechart, gchar *pfont, guint fg_idx)
{
    if (nfpiechart->object.type != NFOBJECT_TYPE_NFPIECHART) return -1;

    if (pfont) 
    {
        if (nffont_is_system_font(pfont))
        {
            nfpiechart->pango_font = pfont;
        }
        else 
        {
            strncpy(nfpiechart->font_name, pfont, sizeof(nfpiechart->font_name));
            nfpiechart->pango_font = nfpiechart->font_name;
        }
    }

    nfpiechart->font_color = fg_idx;

    return 0;
}

gint nfui_nfpiechart_set_punch(NFPIECHART *nfpiechart, gint diameter)
{
    if (nfpiechart->object.type != NFOBJECT_TYPE_NFPIECHART) return -1;

    nfpiechart->punch_diam = diameter;
    return 0;
}

gint nfui_nfpiechart_set_chart_value(NFPIECHART *nfpiechart, gint chart_idx, gdouble value)
{
    if (nfpiechart->object.type != NFOBJECT_TYPE_NFPIECHART) return -1;


    nfpiechart->chart_value[chart_idx] = value;

    return 0;
}

gint nfui_nfpiechart_set_max_value(NFPIECHART *nfpiechart, gint max_value)
{
    if(nfpiechart->object.type != NFOBJECT_TYPE_NFPIECHART) return -1;


    nfpiechart->max_value = max_value;


    return 0;
}

gint nfui_nfpiechart_set_text(NFPIECHART *nfpiechart, gchar *strLabel)
{
    if (nfpiechart->object.type != NFOBJECT_TYPE_NFPIECHART) return -1;

    if (strLabel) strcpy(nfpiechart->strLabel, strLabel);
    return 0;
}


gint nfui_nfpiechart_get_ratio_value(NFPIECHART *nfpiechart, gint idx)
{
    if(nfpiechart->object.type != NFOBJECT_TYPE_NFPIECHART) return -1;
    gint ratio_value = 0;

    if(nfpiechart->max_value == 0) return -1;
    
    if(!nfpiechart->chart_value[idx]) 
    {
        ratio_value =((gdouble)nfpiechart->chart_value[idx]/(gdouble)nfpiechart->max_value)*100;
    }

    return ratio_value;
}



