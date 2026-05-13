/*
 * nfanalogclock.h
 *
 * Written by Jungkyu. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Feb 26, 2014
 *
 */


#ifndef __NFANALOGCLOCK_H__
#define __NFANALOGCLOCK_H__

typedef struct {
    NFOBJECT object;

    time_t timet;

    gchar *pango_font;
    gchar font_name[64];
    guint font_color;

    GdkPixmap *bg;
    GdkPixbuf *bg_img;  

    guint border_color;
    guint arcbg_color;  
    guint sec_color;
    guint min_color;
    guint hour_color;

    gint diameter;
    
} NFANALOGCLOCK;



NFANALOGCLOCK* nfui_nfanalogclock_new(gint clock_diameter);
gint nfui_nfanalogclock_set_border_color(NFANALOGCLOCK *nfanaclock, guint color_idx);
gint nfui_nfanalogclock_set_archbg_color(NFANALOGCLOCK *nfanaclock, guint color_idx);
gint nfui_nfanalogclock_set_sec_color(NFANALOGCLOCK *nfanaclock, guint color_idx);
gint nfui_nfanalogclock_set_min_color(NFANALOGCLOCK *nfanaclock, guint color_idx);
gint nfui_nfanalogclock_set_hour_color(NFANALOGCLOCK *nfanaclock, guint color_idx);
gint nfui_nfanalogclock_set_time(NFANALOGCLOCK *nfanaclock, time_t timet);
gint nfui_nfanalogclock_get_time(NFANALOGCLOCK *nfanaclock, time_t *timet);


//gint nfui_nfanalogclock_set_bg_image(NFANALOGCLOCK *nfanaclock, GdkPixbuf *pbuf);

#endif  // __NFANALOGCLOCK_H__

