/*
    nfbargraph.h 

    Written by P.JaeYoung  << ITX security, Nov 11, 2014 >> 
*/

#ifndef __NFBARGRAPH_H__
#define __NFBARGRAPH_H__

#include "gdk/gdk.h"

#define MAX_BAR_GRAPH_CNT (40)


typedef struct _BARDATA BARDATA;

struct _BARDATA {

    gchar bar_text[16];
    
};

typedef enum _GRAPH_TYPE{

    BAR_GRAPH = 0,
    LINE_GRAPH,
    DOT_GRAPH,
    
}GRAPH_TYPE;

typedef struct{
    NFOBJECT object;

    gchar *pango_font;
    gchar *bar_pango_font; 
    gchar font_name[64];
    guint bar_font_color;
    guint font_color;
    gchar strLabel[128];

    

    guint bar_color;
    guint line_color;

    gboolean draw_line;
    gboolean draw_line_value;
    gboolean draw_bar_value;
    //guint barbg_color;

    GdkPixbuf *nbuf;
    
    gint  value_font;
    guint value_font_color;
    
    guint chart_color;
    guint chart_bg_color;
    guint bar_focus_color;
    
    GdkPixbuf *bar_img;
    GdkPixbuf *chart_img;
    GdkPixbuf *line_img;

    gint bar_width;

    gint chart_width;
    gint chart_height;

    gint pos_x;
    gint pos_y;
    
    gint bar_filled; //[MAX_BAR_GRAPH_CNT];

    gint bar_value[MAX_BAR_GRAPH_CNT];        
    gint bar_data[MAX_BAR_GRAPH_CNT];
    BARDATA bar_info[MAX_BAR_GRAPH_CNT];  // bar data Ã³¸®.
        
    gint bar_height[MAX_BAR_GRAPH_CNT];
    gint bar_max_value;
    gint bar_cnt;

    gint chart_gap; 
    gint bar_gap;

    gint init_bar_gap;
    
    gint cur_pos_value;
    gint cur_pos_idx;

    gint graph_type;
    //gint is_pressed;
    
} NFBARGRAPH;

NFBARGRAPH* nfui_nfbargraph_new(gint width, gint height);

////////////  set  /////////
gint nfui_nfbargraph_set_barbg_color(NFBARGRAPH *nfbargraph, guint color_idx);
gint nfui_nfbargraph_set_chart_color(NFBARGRAPH *nfbargraph, guint color_idx);
//gint nfui_nfbargraph_set_chart_bg_color(NFBARGRAPH *nfbargraph, guint color_idx);
gint nfui_nfbargraph_set_line_color(NFBARGRAPH *nfbargraph, guint color_idx);
gint nfui_nfbargraph_set_bar_focus_color(NFBARGRAPH *nfbargraph, guint fg_idx);

gint nfui_nfbargraph_set_bar_value_draw(NFBARGRAPH *nfbargraph, gboolean draw);

gint nfui_nfbargraph_set_barbg_filled(NFBARGRAPH *nfbargraph, gboolean filled);

gint nfui_nfbargraph_set_pango_font(NFBARGRAPH *nfbargraph, gchar *pfont, guint fg_idx);
gint nfui_nfbargraph_set_bar_pango_font(NFBARGRAPH *nfbargraph, gchar *pfont, guint fg_idx);
gint nfui_nfbargraph_set_bar_data_text(NFBARGRAPH *nfbargraph, const gchar *bar_string, guint idx);
//gint nfui_nfbargraph_set_chart_text(NFBARGRAPH *nfbargraph, gchar *strLabel);

gint nfui_nfbargraph_set_bar_size(NFBARGRAPH *nfbargraph, gint b_width);
gint nfui_nfbargraph_set_bar_value(NFBARGRAPH *nfbargraph,gint bar_value, gint bar_idx);
gint nfui_nfbargraph_set_bar_max_cnt(NFBARGRAPH *nfbargraph, gint max_cnt);
gint nfui_nfbargraph_set_bar_data_text_int(NFBARGRAPH *nfbargraph, gint bar_data, gint bar_idx);

//gint nfui_nfbargraph_set_max_value(NFBARGRAPH *nfbargraph, gint max_value);

gint nfui_nfbargraph_set_chart_line_gap_draw(NFBARGRAPH *nfbargraph, gint gap_size, gboolean draw_line, gboolean draw_value);
//gint nfui_nfbargraph_set_chart_gap_value_draw(NFBARGRAPH * nfbargraph, gint gap_size);

gint nfui_nfbargraph_set_bar_gap(NFBARGRAPH *nfbargraph, gint bar_gap);
gint nfui_nfbargraph_set_bar_init_gap(NFBARGRAPH *nfbargraph, gint init_bar_gap);

gint nfui_nfbargraph_set_bar_focus(NFBARGRAPH *nfbargraph, gboolean focus_use);

//////////  get ///////

gint nfui_nfbargraph_get_bar_focus_index(NFBARGRAPH *nfbargraph);
gint nfui_nfbargraph_get_bar_focus_value(NFBARGRAPH *nfbargraph);
gint nfui_nfbargraph_get_bar_idx_value(NFBARGRAPH *nfbargraph, gint i);
gint nfui_nfbargraph_get_bar_cnt(NFBARGRAPH *nfbargraph);

//////////// delete   /////////////  

gint nfui_nfbargraph_del_all_bar(NFBARGRAPH *nfbargraph);

gint nfui_nfbargraph_change_graph_type(NFBARGRAPH *nfbargraph, GRAPH_TYPE type);



#endif // __NFBARGRAPH_H__


