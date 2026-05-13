/*
 * nfpiechart.h
 *
 * Written by Jungkyu. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Feb 4, 2014
 *
 */

#ifndef __NFPIECHART_H__
#define __NFPIECHART_H__

#define MAX_PARC_CATE   (16)

typedef struct {
    NFOBJECT object;

    gchar *pango_font;
    gchar font_name[64];
    guint font_color;
    gchar strLabel[128];

    GdkPixbuf *nbuf;
    guint arcbg_color;  
    guint chart_color[MAX_PARC_CATE];
    GdkPixbuf *arcbg_img;
    GdkPixbuf *chart_img[MAX_PARC_CATE];
    
    gint pie_diam;
    gint punch_diam;

    gint cnt_chart;
    gint max_value; 
    gdouble chart_value[MAX_PARC_CATE];
} NFPIECHART;



////////////////////////////////////////////////////////////
//
// public interfaces
//

NFPIECHART* nfui_nfpiechart_new(gint pie_diameter, gint cnt_chart, gint max_value);
gint nfui_nfpiechart_set_arcbg_color(NFPIECHART *nfpiechart, guint color_idx);
gint nfui_nfpiechart_set_chart_color(NFPIECHART *nfpiechart, gint chart_idx, guint color_idx);
//gint nfui_nfpiechart_set_arcbg_image(NFPIECHART *nfpiechart, GdkPixbuf *pbuf);
//gint nfui_nfpiechart_set_chart_image(NFPIECHART *nfpiechart, gint chart_idx, GdkPixbuf *pbuf);
gint nfui_nfpiechart_set_pango_font(NFPIECHART *nfpiechart, gchar *pfont, guint fg_idx);
gint nfui_nfpiechart_set_punch(NFPIECHART *nfpiechart, gint diameter);
gint nfui_nfpiechart_set_chart_value(NFPIECHART *nfpiechart, gint chart_idx, gdouble value);
gint nfui_nfpiechart_set_max_value(NFPIECHART *nfpiechart, gint max_value);

gint nfui_nfpiechart_set_text(NFPIECHART *nfpiechart, gchar *strLabel);

gint nfui_nfpiechart_get_ratio_value(NFPIECHART *nfpiechart, gint idx);


#endif  // __NFPIECHART_H__

