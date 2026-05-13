/*
 * nfanalogclock.c
 *
 * Written by Jungkyu. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Feb 26, 2014
 *
 */

#include "../../support/util.h"
#include "../../support/color.h"
#include <math.h>

#include "ix_mem.h"
#include "nfobject.h"
#include "nfanalogclock.h"




////////////////////////////////////////////////////////////
//
// private data types
//





////////////////////////////////////////////////////////////
//
// private variable
//

static gint _draw_arc_colorbg(NFANALOGCLOCK *nfanaclock, GdkDrawable *drawable, GdkGC *gc)
{
    GdkPoint end_pt;
    
    gint ang1 = 0;
    gint ang2 = 360*64;
    gint bg_col_idx;
    
    gint subarch, border;

    gint line_len;
    gint i;
    
    bg_col_idx = nfui_nfobject_get_bg_color((NFOBJECT*)nfanaclock, NFOBJECT_STATE_NORMAL);
    gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_col_idx));
    gdk_draw_rectangle(drawable, gc, TRUE, 0, 0, nfanaclock->diameter, nfanaclock->diameter);

    gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(nfanaclock->border_color));
    gdk_draw_arc(drawable, gc, TRUE, 0, 0, nfanaclock->diameter, nfanaclock->diameter, ang1, ang2);

    subarch = nfanaclock->diameter-8;

    gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(nfanaclock->arcbg_color));
    gdk_draw_arc(drawable, gc, TRUE, 4, 4, subarch, subarch, ang1, ang2);

    gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(nfanaclock->font_color));
    gdk_gc_set_line_attributes(gc, 2, GDK_LINE_SOLID, GDK_CAP_NOT_LAST, GDK_JOIN_MITER);

    line_len = (nfanaclock->diameter-8)/2;
    
    for (i = 0; i < 12; i++)
    {
        if ((i == 0) || (i == 3) || (i == 6) || (i == 9)) continue;
    
        end_pt.x = cosf((i*30-90)*M_PI/180) * line_len +  nfanaclock->diameter/2; 
        end_pt.y = sinf((i*30-90)*M_PI/180) * line_len + nfanaclock->diameter/2; 

        gdk_draw_line(drawable, gc, nfanaclock->diameter/2, nfanaclock->diameter/2, end_pt.x, end_pt.y);
    }

    subarch = nfanaclock->diameter*9/10;

    gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(nfanaclock->arcbg_color));
    gdk_draw_arc(drawable, gc, TRUE, (nfanaclock->diameter-subarch)/2, (nfanaclock->diameter-subarch)/2, subarch, subarch, ang1, ang2);

    return 0;
}

static gint _draw_clock_text(NFANALOGCLOCK *nfanaclock, GdkDrawable *drawable, GdkGC *gc)
{
    gint text_w, text_h;
    gint pos_x, pos_y;

// 12
    text_w = nfutil_string_width(0, drawable, nfanaclock->pango_font, "12", NORMAL_SPACING);
    text_h = nfutil_string_height(drawable, nfanaclock->pango_font, "12", NORMAL_SPACING);
    pos_x = (nfanaclock->diameter - text_w)/2;
    pos_y = 4;

    nfutil_draw_short_text(0, NULL, &UX_COLOR(nfanaclock->arcbg_color), drawable, gc, "12", 0, pos_x, pos_y, text_w, text_h, 
        nfanaclock->pango_font, &UX_COLOR(nfanaclock->font_color), NULL, NFALIGN_CENTER, 0, 0, SEMI_CONDENSED_SPACING); 

// 3
    text_w = nfutil_string_width(0, drawable, nfanaclock->pango_font, "3", NORMAL_SPACING);
    text_h = nfutil_string_height(drawable, nfanaclock->pango_font, "3", NORMAL_SPACING);
    pos_x = nfanaclock->diameter - text_w - 4;
    pos_y = (nfanaclock->diameter - text_h)/2;

    nfutil_draw_short_text(0, NULL, &UX_COLOR(nfanaclock->arcbg_color), drawable, gc, "3", 0, pos_x, pos_y, text_w, text_h, 
        nfanaclock->pango_font, &UX_COLOR(nfanaclock->font_color), NULL, NFALIGN_RIGHT, 0, 0, SEMI_CONDENSED_SPACING);  

// 6
    text_w = nfutil_string_width(0, drawable, nfanaclock->pango_font, "6", NORMAL_SPACING);
    text_h = nfutil_string_height(drawable, nfanaclock->pango_font, "6", NORMAL_SPACING);
    pos_x = (nfanaclock->diameter - text_w)/2;
    pos_y = nfanaclock->diameter - text_h - 4;

    nfutil_draw_short_text(0, NULL, &UX_COLOR(nfanaclock->arcbg_color), drawable, gc, "6", 0, pos_x, pos_y, text_w, text_h, 
        nfanaclock->pango_font, &UX_COLOR(nfanaclock->font_color), NULL, NFALIGN_CENTER, 0, 0, SEMI_CONDENSED_SPACING); 

// 9
    text_w = nfutil_string_width(0, drawable, nfanaclock->pango_font, "9", NORMAL_SPACING);
    text_h = nfutil_string_height(drawable, nfanaclock->pango_font, "9", NORMAL_SPACING);
    pos_x = 4;
    pos_y = (nfanaclock->diameter - text_h)/2;

    nfutil_draw_short_text(0, NULL, &UX_COLOR(nfanaclock->arcbg_color), drawable, gc, "9", 0, pos_x, pos_y, text_w, text_h, 
        nfanaclock->pango_font, &UX_COLOR(nfanaclock->font_color), NULL, NFALIGN_LEFT, 0, 0, SEMI_CONDENSED_SPACING);   
        
    return 0;
}

static gint _draw_clock_bg(NFANALOGCLOCK *nfanaclock, GdkDrawable *drawable, GdkGC *gc, gint off_x, gint off_y)
{
    if (nfanaclock->bg_img)
    {

    }
    else
    {
        if (!nfanaclock->bg)
        {   
            nfanaclock->bg = gdk_pixmap_new(drawable, nfanaclock->diameter, nfanaclock->diameter, -1);
            
            _draw_arc_colorbg(nfanaclock, nfanaclock->bg, gc);
            _draw_clock_text(nfanaclock, nfanaclock->bg, gc);
        }

        off_x += (nfanaclock->object.width - nfanaclock->diameter)/2;
        off_y += (nfanaclock->object.height - nfanaclock->diameter)/2;

        gdk_draw_drawable(drawable, gc, nfanaclock->bg, 0, 0, off_x, off_y, -1, -1);
    }
        
    return 0;
}

static gint _draw_clock_min(NFANALOGCLOCK *nfanaclock, GdkDrawable *drawable, GdkGC *gc, gint off_x, gint off_y)
{
    GdkPoint end_pt;
    
    gint x, y;
    gint i, min, sec;
    gint degree, line_len;

    off_x += nfanaclock->object.width/2;
    off_y += nfanaclock->object.height/2;

    dtf_get_local_hourmin(nfanaclock->timet, 0, &min, &sec);

    gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(nfanaclock->min_color));
    gdk_gc_set_line_attributes(gc, 6, GDK_LINE_SOLID, GDK_CAP_NOT_LAST, GDK_JOIN_MITER);

    degree = min*6 + sec/10;
    line_len = nfanaclock->diameter/2;
    line_len = line_len*8/10;

    end_pt.x = cosf((degree-90)*M_PI/180) * line_len + off_x; 
    end_pt.y = sinf((degree-90)*M_PI/180) * line_len + off_y; 
    gdk_draw_line(drawable, gc, off_x, off_y, end_pt.x, end_pt.y);

    degree += 180;
    degree = degree > 360 ? (degree-360) : degree;
    line_len /= 8;

    end_pt.x = cosf((degree-90)*M_PI/180) * line_len + off_x; 
    end_pt.y = sinf((degree-90)*M_PI/180) * line_len + off_y; 
    gdk_draw_line(drawable, gc, off_x, off_y, end_pt.x, end_pt.y);

    return 0;
}

static gint _draw_clock_hour(NFANALOGCLOCK *nfanaclock, GdkDrawable *drawable, GdkGC *gc, gint off_x, gint off_y)
{
    GdkPoint end_pt;

    gint x, y;
    gint i, hour, min;
    gint degree, line_len;
    
    off_x += nfanaclock->object.width/2;
    off_y += nfanaclock->object.height/2;   

    dtf_get_local_hourmin(nfanaclock->timet, &hour, &min, 0);

    gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(nfanaclock->hour_color));
    gdk_gc_set_line_attributes(gc, 10, GDK_LINE_SOLID, GDK_CAP_NOT_LAST, GDK_JOIN_MITER);

    degree = hour*30 + min/2;
    line_len = nfanaclock->diameter/2;
    line_len = line_len*5/10;
    
    end_pt.x = cosf((degree-90)*M_PI/180) * line_len + off_x; 
    end_pt.y = sinf((degree-90)*M_PI/180) * line_len + off_y; 
    gdk_draw_line(drawable, gc, off_x, off_y, end_pt.x, end_pt.y);

    degree += 180;
    degree = degree > 360 ? (degree-360) : degree;
    line_len /= 8;

    end_pt.x = cosf((degree-90)*M_PI/180) * line_len + off_x; 
    end_pt.y = sinf((degree-90)*M_PI/180) * line_len + off_y; 
    gdk_draw_line(drawable, gc, off_x, off_y, end_pt.x, end_pt.y);
    
    return 0;
}

static gint _draw_clock_sec(NFANALOGCLOCK *nfanaclock, GdkDrawable *drawable, GdkGC *gc, gint off_x, gint off_y)
{
    GdkPoint end_pt;

    gint x, y;
    gint i, sec;
    gint degree, line_len;
    
    gint ang1 = 0;
    gint ang2 = 360*64;

    off_x += nfanaclock->object.width/2;
    off_y += nfanaclock->object.height/2;   

    dtf_get_local_hourmin(nfanaclock->timet, 0, 0, &sec);

    gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(nfanaclock->sec_color));
    gdk_gc_set_line_attributes(gc, 2, GDK_LINE_SOLID, GDK_CAP_NOT_LAST, GDK_JOIN_MITER);

    degree = sec*6;
    line_len = nfanaclock->diameter/2;
    line_len = line_len*8/10;
    
    end_pt.x = cosf((degree-90)*M_PI/180) * line_len + off_x; 
    end_pt.y = sinf((degree-90)*M_PI/180) * line_len + off_y; 
    gdk_draw_line(drawable, gc, off_x, off_y, end_pt.x, end_pt.y);

    degree += 180;
    degree = degree > 360 ? (degree-360) : degree;
    line_len /= 8;
    
    end_pt.x = cosf((degree-90)*M_PI/180) * line_len + off_x; 
    end_pt.y = sinf((degree-90)*M_PI/180) * line_len + off_y; 
    gdk_draw_line(drawable, gc, off_x, off_y, end_pt.x, end_pt.y);

    off_x -= 5;
    off_y -= 5;
    gdk_draw_arc(drawable, gc, TRUE, off_x, off_y, 10, 10, ang1, ang2);

    return 0;
}




////////////////////////////////////////////////////////////
//
// private interfaces 
//

static gboolean nfanalogclock_event_handler(NFANALOGCLOCK *nfanaclock, GdkEvent *event, gpointer data)
{
    switch(event->type)
    {
        case GDK_EXPOSE:
        {
            GdkDrawable *drawable;
            GdkGC *gc;
            gint i;
            gint off_x, off_y;
        
            drawable = nfui_nfobject_get_window((NFOBJECT*)nfanaclock);
            gc = nfui_nfobject_get_gc((NFOBJECT*)nfanaclock);

            nfui_nfobject_get_offset((NFOBJECT*)nfanaclock, &off_x, &off_y);
        
            _draw_clock_bg(nfanaclock, drawable, gc, off_x, off_y);
            _draw_clock_min(nfanaclock, drawable, gc, off_x, off_y);
            _draw_clock_hour(nfanaclock, drawable, gc, off_x, off_y);
            _draw_clock_sec(nfanaclock, drawable, gc, off_x, off_y);

            nfui_nfobject_gc_unref(gc);
        }
        break;

        case GDK_DELETE:
        {
            if (nfanaclock->bg) 
            {
                g_object_unref(nfanaclock->bg);
                nfanaclock->bg = NULL;
            }
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


NFANALOGCLOCK* nfui_nfanalogclock_new(gint clock_diameter)
{
    NFANALOGCLOCK *nfanaclock;
    gint i;

    nfanaclock = (NFANALOGCLOCK*)imalloc(sizeof(NFANALOGCLOCK));
    
    if(nfanaclock == NULL)
    {
        g_warning("[NFANALOGCLOCK CREATE ERROR]\n");
        return NULL;
    }

    nfui_nfobject_init((NFOBJECT*)nfanaclock);

    nfanaclock->object.width = 100;
    nfanaclock->object.height = 100;
    nfanaclock->object.type = NFOBJECT_TYPE_NFANALOGCLOCK;
    nfanaclock->object.default_event_handler = nfanalogclock_event_handler;

    nfanaclock->diameter = clock_diameter;

    nfanaclock->font_color = COLOR_IDX(116);
    g_sprintf(nfanaclock->font_name, "Arial bold 19");
    nfanaclock->pango_font = nfanaclock->font_name;

    nfanaclock->border_color = COLOR_IDX(115);
    nfanaclock->arcbg_color = COLOR_IDX(230);
    nfanaclock->sec_color = COLOR_IDX(267);
    nfanaclock->min_color = COLOR_IDX(116);
    nfanaclock->hour_color = COLOR_IDX(116);

    return nfanaclock;
}

gint nfui_nfanalogclock_set_border_color(NFANALOGCLOCK *nfanaclock, guint color_idx)
{
    if (nfanaclock->object.type != NFOBJECT_TYPE_NFANALOGCLOCK) return -1;

    nfanaclock->border_color = color_idx;

    return 0;
}

gint nfui_nfanalogclock_set_archbg_color(NFANALOGCLOCK *nfanaclock, guint color_idx)
{
    if (nfanaclock->object.type != NFOBJECT_TYPE_NFANALOGCLOCK) return -1;

    nfanaclock->arcbg_color = color_idx;

    return 0;
}

gint nfui_nfanalogclock_set_sec_color(NFANALOGCLOCK *nfanaclock, guint color_idx)
{
    if (nfanaclock->object.type != NFOBJECT_TYPE_NFANALOGCLOCK) return -1;

    nfanaclock->sec_color = color_idx;

    return 0;
}

gint nfui_nfanalogclock_set_min_color(NFANALOGCLOCK *nfanaclock, guint color_idx)
{
    if (nfanaclock->object.type != NFOBJECT_TYPE_NFANALOGCLOCK) return -1;

    nfanaclock->min_color = color_idx;

    return 0;
}

gint nfui_nfanalogclock_set_hour_color(NFANALOGCLOCK *nfanaclock, guint color_idx)
{
    if (nfanaclock->object.type != NFOBJECT_TYPE_NFANALOGCLOCK) return -1;

    nfanaclock->hour_color = color_idx;

    return 0;
}

gint nfui_nfanalogclock_set_pango_font(NFANALOGCLOCK *nfanaclock, gchar *pfont, guint fg_idx)
{
    if (nfanaclock->object.type != NFOBJECT_TYPE_NFANALOGCLOCK) return -1;

    if (pfont) 
    {
        if (nffont_is_system_font(pfont))
        {
            nfanaclock->pango_font = pfont;
        }
        else 
        {
            strncpy(nfanaclock->font_name, pfont, sizeof(nfanaclock->font_name));
            nfanaclock->pango_font = nfanaclock->font_name;
        }
    }

    nfanaclock->font_color = fg_idx;

    return 0;
}

gint nfui_nfanalogclock_set_time(NFANALOGCLOCK *nfanaclock, time_t timet)
{
    if (nfanaclock->object.type != NFOBJECT_TYPE_NFANALOGCLOCK) return -1;

    nfanaclock->timet = timet;

    return 0;   
}

gint nfui_nfanalogclock_get_time(NFANALOGCLOCK *nfanaclock, time_t *timet)
{
    if (nfanaclock->object.type != NFOBJECT_TYPE_NFANALOGCLOCK) return -1;

    *timet = nfanaclock->timet;

    return 0;   
}

