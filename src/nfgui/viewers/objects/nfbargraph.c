/*
    nfbargraph.c 

    Written by P.JaeYoung  << ITX security, Nov 11, 2014 >> 
*/

#include "../../support/util.h"
#include "../../support/color.h"
#include "../../support/nf_ui_font.h"
#include "../../support/event_loop.h"

#include "ix_mem.h"
#include "nfobject.h"
#include "nfbargraph.h"

#define CHART_SPACE_W        (60)
#define CHART_SPACE_W_R      (10)
#define CHART_SPACE_H        (30)
#define CHART_SPACE_B_H      (10)
#define LINE_VALUE_SPACE     (5)
#define BAR_SPACE_H          (10)
#define DATA_SIZE_H          (10)

#define CAHRT_LINE_SIZE      (4)
#define LINE_SIZE            (1)

#define DOT_SIZE             (8)

#define MOUSE_LEFT_BUTTON    (1)
#define MOUSE_RIGHT_BUTTON	 (3)

static gint _get_bargraph_width(NFBARGRAPH *nfbargraph, gint cnt, gint *bar_gap, gint init_gap, gint *bar_width, gint chart_width)
{
    gint width = 0;
    gint over_size_w = 0;
    gdouble tmp_w, tmp_g;
    gint draw_chart_size =0;

    draw_chart_size = chart_width - CHART_SPACE_W;
    
    width = (init_gap+((*bar_width+*bar_gap)*cnt));

    if(width > draw_chart_size){
        over_size_w = width - draw_chart_size;

        tmp_w = (double)(*bar_width)/(double)((*bar_width)+(*bar_gap));
        tmp_g = (double)(*bar_gap)/(double)((*bar_width)+(*bar_gap));

        tmp_w = (tmp_w*(double)over_size_w)/cnt;
        tmp_g = (tmp_g*(double)over_size_w)/cnt;
        
        *bar_width -= tmp_w;
        *bar_gap -= tmp_g;

        nfbargraph->bar_width = *bar_width;
        nfbargraph->bar_gap = *bar_gap;
        
    }
    
    return 0;
}

static gint _get_bargraph_height(NFBARGRAPH *nfbargraph, gint max_value,gint cnt)
{
    gint i;

    if(max_value == 0)
        max_value = 45;

    
 
    for( i = 0 ; i< cnt ; i++)    
    {
        nfbargraph->bar_height[i] = ((double)nfbargraph->bar_value[i]/(double)max_value)*((double)(((nfbargraph->chart_height-BAR_SPACE_H)-CHART_SPACE_H)-CHART_SPACE_B_H));
    }        
            
    return 0;
}

static gint _get_bar_max_value(NFBARGRAPH *nfbargraph)
{
    gint i;
    gint temp=0;

    for( i=0 ; i< nfbargraph->bar_cnt ; i++)
    {
        if( temp < nfbargraph->bar_value[i])
        {
            temp = nfbargraph->bar_value[i];
        }
    }

    nfbargraph->bar_max_value = temp;

    return temp;
}

static gint _get_chart_gap(NFBARGRAPH *nfbargraph)
{
    gint max_value=0;
    gint gap_size=0;
    
    max_value = nfbargraph->bar_max_value; 

    if((max_value/4)<10)
    {
         gap_size = (max_value/4);
    }
    else if((max_value/4)<100)
    {           
         gap_size = ((max_value/4)/10)*10;    

         if(((max_value/4)%gap_size) > 4)
         {
            gap_size +=5;
         }
    }    
    else if((max_value/4)<1000)
    {
         gap_size = ((max_value/4)/100)*100;    

         if(((max_value/4)%gap_size) > 49)
         {
            gap_size +=50;
         }
    }     
    else if((max_value/4)<10000)
    {
         gap_size = ((max_value/4)/1000)*1000;

         if(((max_value/4)%gap_size) > 499)
         {
            gap_size +=500;
         }
    }     
    else if((max_value/4)<100000)
    {
         gap_size = ((max_value/4)/10000)*10000;

         if(((max_value/4)%gap_size) > 4999)
         {
            gap_size +=5000;
         }
    }

    if(gap_size == 0 )
    {
        if(max_value)
            gap_size = max_value;
        else
            gap_size = 10;
    }

    return gap_size;   
}

static gint _draw_bar_value(NFBARGRAPH *nfbargraph, GdkDrawable *drawable,GdkGC *gc, gint pos_x, gint pos_y,gint idx)
{
    GdkFont *font;
    gchar font_type;
    gchar temp_value[16];
    gint temp_size=0;
    gint i = 0;
   
    if(!nfbargraph->draw_bar_value || !nfbargraph->bar_value[idx]) return -1;
                
    sprintf(temp_value,"%d",nfbargraph->bar_value[idx]);

    temp_size = nfutil_string_width(0, drawable, nffont_get_pango_font(NFFONT_MINI_NORMAL_3), temp_value, 0);

    if(nfbargraph->graph_type == BAR_GRAPH)
    {
        pos_x = (pos_x + (nfbargraph->bar_width/2)) - (temp_size/2);  
    }
    else
    {
        pos_x = (pos_x - (temp_size/2));  
    }
    
    nfutil_draw_short_text_eng(0, 0, 0, drawable, gc, temp_value,0, pos_x, pos_y,0, 0, nfbargraph->bar_pango_font, &UX_COLOR(nfbargraph->font_color), 0, NFALIGN_LEFT, 0, 0, 0);
            
    return 0;
}

static gint _draw_bar_text(NFBARGRAPH *nfbargraph, GdkDrawable *drawable, GdkGC *gc, gint pos_x,gint pos_y,gint idx)
{
    GdkFont *font;
    gchar temp_value[32];
    gint temp_size=0,i=0;

    if(strlen(nfbargraph->bar_info[idx].bar_text))  strcpy(temp_value,nfbargraph->bar_info[idx].bar_text);
    else if(nfbargraph->bar_data[idx])              sprintf(temp_value,"%02d",nfbargraph->bar_data[idx]);
    else                                            sprintf(temp_value,"%02d",idx+1);

    temp_size = nfutil_string_width(0, drawable, nffont_get_pango_font(NFFONT_MINI_NORMAL_3), temp_value, 0);

    if(nfbargraph->graph_type == BAR_GRAPH)
    {
        pos_x = (pos_x + (nfbargraph->bar_width/2)) - (temp_size/2);
    }
    else
    {
        pos_x = (pos_x - (temp_size/2)); 
    }
       
    if(nfui_nfobject_is_supported_multi_lang(nfbargraph))
	    nfutil_draw_short_text(0, 0, 0, drawable, gc, temp_value,0, pos_x, pos_y-DATA_SIZE_H,0, 0, nfbargraph->pango_font,&UX_COLOR(nfbargraph->font_color), 0, NFALIGN_LEFT, 0, 0, 0);
    else
        nfutil_draw_short_text_eng(0, 0, 0, drawable, gc, temp_value,0, pos_x, pos_y-DATA_SIZE_H,0, 0, nfbargraph->pango_font,&UX_COLOR(nfbargraph->font_color), 0, NFALIGN_LEFT, 0, 0, 0);
    
    return 0;
}

static gint _draw_bargraph(NFBARGRAPH *nfbargraph, GdkDrawable *drawable, GdkGC *gc, gint off_x, gint off_y)
{
    gint i;
    gboolean filled =0;
    gint width =0 ,height =0; 
    gint pos_x=0, pos_y =0, gap =0;
    gint init_x_gap =0;
    gint bar_cnt;
    gint p_size = 0;
    
    width = nfbargraph->bar_width;
    filled = nfbargraph->bar_filled;
    gap = nfbargraph->bar_gap;
    bar_cnt =nfbargraph->bar_cnt;
    init_x_gap = nfbargraph->init_bar_gap;
    p_size = DOT_SIZE/2;
  
    _get_bargraph_width(nfbargraph, nfbargraph->bar_cnt,&gap, init_x_gap, &width, nfbargraph->chart_width);
    _get_bargraph_height(nfbargraph, nfbargraph->bar_max_value, bar_cnt);

    pos_x = off_x + CHART_SPACE_W;
    pos_y = (off_y + nfbargraph->chart_height)- CHART_SPACE_H;  

    gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(nfbargraph->chart_color));
    gdk_draw_rectangle(drawable, gc, TRUE, pos_x, pos_y, (nfbargraph->chart_width-CHART_SPACE_W)-CHART_SPACE_W_R, CAHRT_LINE_SIZE);
    gdk_draw_rectangle(drawable, gc, TRUE, pos_x, off_y, CAHRT_LINE_SIZE, nfbargraph->chart_height-CHART_SPACE_H);
    
    pos_x = pos_x+init_x_gap;
     
    for ( i = 0; i< bar_cnt; i++)
    {
        height = nfbargraph->bar_height[i];

        gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(nfbargraph->bar_color));
        
        //if(nfbargraph->cur_pos_idx == i)  // focus 된 그래프 색 확인을 위한. 
        //    gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_FF00FF)));

        _draw_bar_text(nfbargraph, drawable, gc, pos_x, pos_y+CHART_SPACE_H, i);        
        _draw_bar_value(nfbargraph, drawable, gc, pos_x, (pos_y-height)-BAR_SPACE_H, i);
        
        if(nfbargraph->graph_type == BAR_GRAPH)
        {
            gdk_draw_rectangle(drawable, gc, filled, pos_x, pos_y-height, width, height);   
        }
        else if(nfbargraph->graph_type == LINE_GRAPH)
        {
            gdk_gc_set_line_attributes(gc, DOT_SIZE, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);
            gdk_draw_line(drawable, gc, pos_x - p_size, pos_y - height, pos_x + p_size, pos_y - height);  // draw point
           
            if(i+1 == bar_cnt)
                return 0;   

            gdk_gc_set_line_attributes(gc, 4, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);
            gdk_draw_line(drawable, gc, pos_x, pos_y - height, pos_x + width + gap, pos_y - nfbargraph->bar_height[i+1]);
        }      
        else if(nfbargraph->graph_type == DOT_GRAPH)
        {
            gdk_gc_set_line_attributes(gc, DOT_SIZE, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);
            gdk_draw_line(drawable, gc, pos_x - p_size, pos_y - height, pos_x + p_size, pos_y - height);
        }

        gdk_gc_set_line_attributes(gc, 1, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);
           
        pos_x += width + gap;
    }
    
    return 0;
}


static gint _draw_line_value(NFBARGRAPH *nfbargraph, GdkDrawable *drawable, GdkGC *gc, gint x, gint y,gint line_value)
{
    GdkFont *font;
    gchar temp_value[32];
    gint temp_size=0, i=0;
  
    if(!nfbargraph->draw_line_value)     return -1;

    memset(temp_value, 0x00, sizeof(temp_value));
    snprintf(temp_value,sizeof(temp_value)-1,"%d",line_value);
    temp_size = nfutil_string_width(0, drawable, nfbargraph->pango_font, temp_value, 0);

    if (temp_size > CHART_SPACE_W-LINE_VALUE_SPACE) 
    {
        memset(temp_value, 0x00, sizeof(temp_value));
        line_value /= 1000;
        snprintf(temp_value,sizeof(temp_value)-1,"%dk",line_value);
    }
    else 
    {
        x = (x+CHART_SPACE_W)-temp_size-LINE_VALUE_SPACE;
    }
    
    //g_message("%s, %d, temp_value:%s", __FUNCTION__, __LINE__, temp_value);
    nfutil_draw_short_text_eng(0, 0, 0, drawable, gc, temp_value,0, x, y, 0, 0, nfbargraph->pango_font, &UX_COLOR(nfbargraph->font_color), 0, NFALIGN_LEFT, 0, 0, 0);
    
    return 0;
}

static gint _draw_chart_gap(NFBARGRAPH *nfbargraph,GdkDrawable *drawable,GdkGC *gc,gint off_x,gint off_y)
{
    gint i;
    gint gap_cnt=0;
    gint x=0,y=0;
    gdouble gap_size_h=0;
    gint gap_value=0;
    gint gap;
    gint graph_size_h=0;

    if(nfbargraph->chart_gap)
        gap = nfbargraph->chart_gap;
    else
        gap = _get_chart_gap(nfbargraph);

    graph_size_h = ((nfbargraph->chart_height-CHART_SPACE_H)-BAR_SPACE_H)-CHART_SPACE_B_H;

    if(nfbargraph->bar_max_value ==0) 
           gap_size_h = ((double)graph_size_h*(double)gap)/45;
    else
           gap_size_h = ((double)graph_size_h*(double)gap)/(double)nfbargraph->bar_max_value;

    gap_cnt = graph_size_h/gap_size_h;
  
    x = off_x + CHART_SPACE_W;
    y = off_y + nfbargraph->chart_height - CHART_SPACE_H; 

    _draw_line_value(nfbargraph, drawable, gc ,x-CHART_SPACE_W,y,gap_value); //  0 value

    
    for(i =0 ; i<gap_cnt ; i++)
    {
        y -= gap_size_h;
        gap_value += gap;
        
        gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(nfbargraph->line_color));

        
        if(nfbargraph->draw_line)
            gdk_draw_rectangle(drawable, gc, TRUE, x, y, (nfbargraph->chart_width-CHART_SPACE_W)-CHART_SPACE_W_R, LINE_SIZE);

        _draw_line_value(nfbargraph, drawable, gc ,x-CHART_SPACE_W,y,gap_value);
    }   

    return 0;   
}

static gboolean _2button_press_dot(NFBARGRAPH *nfbargraph, GdkEvent *event)
{
    GdkDrawable *drawable;
    GdkGC *gc;
    gint i;
    gint height;
    guint offx, offy;
    gint p_size = DOT_SIZE/2;
    gint signal = 0;

    gint mot_x = 0, mot_y = 0;

    mot_x = ((GdkEventMotion*)event)->x;
    mot_y = ((GdkEventMotion*)event)->y;

    nfui_nfobject_get_offset((NFOBJECT*)nfbargraph, &offx, &offy);

    drawable = nfui_nfobject_get_window((NFOBJECT*)nfbargraph);
    gc = nfui_nfobject_get_gc((NFOBJECT*)nfbargraph);

    offx = offx + CHART_SPACE_W + nfbargraph->init_bar_gap;
    offy = (offy + nfbargraph->chart_height) - CHART_SPACE_H;

    nfbargraph->cur_pos_idx = -1;

    for( i = 0; i < nfbargraph->bar_cnt; i++)
    {
        height = nfbargraph->bar_height[i];

        if((mot_x > offx - p_size) && (mot_x < (offx + p_size)) && (mot_y > (offy - height - p_size)) && (mot_y < offy - height + p_size))
        {
            signal = 1;
            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_IDX(688)));

            if(nfbargraph->object.use_focus == NFOBJECT_FOCUS_ON)
            {
                gdk_gc_set_line_attributes(gc, DOT_SIZE, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);
                gdk_draw_line(drawable, gc, offx - p_size, offy - height, offx + p_size, offy - height);  // draw point   
            }

            nfbargraph->cur_pos_value = nfbargraph->bar_value[i];
            nfbargraph->cur_pos_idx = i;
        }
        else
        {
            if(nfbargraph->cur_pos_idx == i)
            {
                offx += (nfbargraph->bar_width) + (nfbargraph->bar_gap);

                continue;
            }

            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(nfbargraph->bar_color));
            gdk_gc_set_line_attributes(gc, DOT_SIZE, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);
            gdk_draw_line(drawable, gc, offx - p_size, offy - height, offx + p_size, offy - height);  // draw point
        }

        gdk_gc_set_line_attributes(gc, 1, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);

        offx += (nfbargraph->bar_width) + (nfbargraph->bar_gap);
    }

    if(signal) nfui_user_signal_emit((NFOBJECT*)nfbargraph, NFEVENT_BARGRAPH_2BUTTON_PRESS, FALSE);
    
    return FALSE;
}

static gboolean _2button_press_bar(NFBARGRAPH *nfbargraph, GdkEvent *event)
{
    GdkDrawable *drawable;
    GdkGC *gc;
    gint i;
    gint height;
    guint offx, offy;

    gint signal=0;

    gint mot_x=0, mot_y=0;
    
    mot_x = ((GdkEventMotion*)event)->x;
    mot_y = ((GdkEventMotion*)event)->y; 
    
    nfui_nfobject_get_offset((NFOBJECT*)nfbargraph, &offx, &offy);    

    drawable = nfui_nfobject_get_window((NFOBJECT *)nfbargraph);
    gc = nfui_nfobject_get_gc((NFOBJECT *)nfbargraph);

    offx = offx + CHART_SPACE_W + nfbargraph->init_bar_gap;
    offy = (offy + nfbargraph->chart_height) - CHART_SPACE_H;

    nfbargraph->cur_pos_idx = -1;
            
    for( i = 0; i< nfbargraph->bar_cnt; i++)
    {
        height = nfbargraph->bar_height[i];
        
        if((mot_x > offx) && (mot_x < (offx+nfbargraph->bar_width)) && (mot_y > (offy-height)) && (mot_y < offy)){

            signal = 1;               
            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_IDX(688)));
            
            if(nfbargraph->object.use_focus == NFOBJECT_FOCUS_ON)
                gdk_draw_rectangle(drawable, gc, TRUE, offx, offy-height, nfbargraph->bar_width, height);

            nfbargraph->cur_pos_value = nfbargraph->bar_value[i];
            nfbargraph->cur_pos_idx = i;
         
        }
        else{
        
            if (nfbargraph->cur_pos_idx == i){
                offx += (nfbargraph->bar_width)+ (nfbargraph->bar_gap);                                 
                continue;
            }                
            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(nfbargraph->bar_color));
            gdk_draw_rectangle(drawable, gc, nfbargraph->bar_filled, offx, offy-height, nfbargraph->bar_width, height);
        }
        offx += (nfbargraph->bar_width)+ (nfbargraph->bar_gap);                                 
    }

    if(signal) nfui_user_signal_emit((NFOBJECT*)nfbargraph, NFEVENT_BARGRAPH_2BUTTON_PRESS, FALSE);

    return FALSE;
}

static gboolean _draw_press_dot(NFBARGRAPH *nfbargraph, GdkEvent *event)
{
    GdkDrawable *drawable;
    GdkGC *gc;
    gint i;
    gint height;
    guint offx, offy;
    gint signal = 0;

    gint p_size = DOT_SIZE/2;

    gint mot_x = 0, mot_y = 0;

    mot_x = ((GdkEventMotion*)event)->x;
    mot_y = ((GdkEventMotion*)event)->y;

    nfui_nfobject_get_offset((NFOBJECT*)nfbargraph, &offx, &offy);

    drawable = nfui_nfobject_get_window((NFOBJECT*)nfbargraph);
    gc = nfui_nfobject_get_gc((NFOBJECT*)nfbargraph);

    offx = offx + CHART_SPACE_W + nfbargraph->init_bar_gap;
    offy = (offy + nfbargraph->chart_height) - CHART_SPACE_H;

    nfbargraph->cur_pos_idx = -1;

    for( i = 0; i < nfbargraph->bar_cnt; i++)
    {
        height = nfbargraph->bar_height[i];

        if((mot_x > offx - p_size ) && (mot_x < offx + p_size) && (mot_y > offy - height - p_size) && (mot_y < offy - height + p_size))
        {
            signal = 1;

            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_IDX(688)));

            if(nfbargraph->object.use_focus == NFOBJECT_FOCUS_ON)
            {
                gdk_gc_set_line_attributes(gc, DOT_SIZE, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);
                gdk_draw_line(drawable, gc, offx - p_size, offy - height, offx + p_size, offy - height);  // draw point
            }

            nfbargraph->cur_pos_value = nfbargraph->bar_value[i];
            nfbargraph->cur_pos_idx = i;
        }
        else
        {
            if(nfbargraph->cur_pos_idx == i)
            {
                offx += (nfbargraph->bar_width) + (nfbargraph->bar_gap);

                continue;
            }

            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(nfbargraph->bar_color));
            gdk_gc_set_line_attributes(gc, DOT_SIZE, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);
            gdk_draw_line(drawable, gc, offx - p_size, offy - height, offx + p_size, offy - height);  // draw point
        }

        gdk_gc_set_line_attributes(gc, 1, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);

        offx += (nfbargraph->bar_width) + (nfbargraph->bar_gap);
    }

    if(signal)  nfui_user_signal_emit((NFOBJECT*)nfbargraph, NFEVENT_BARGRAPH_SELECTED, FALSE);

    return FALSE;
}

static gboolean _draw_press_bar(NFBARGRAPH *nfbargraph, GdkEvent *event)
{   
    GdkDrawable *drawable;
    GdkGC *gc;
    gint i;
    gint height;
    guint offx, offy;
    gint signal=0;

    gint mot_x=0, mot_y=0;
    
    mot_x = ((GdkEventMotion*)event)->x;
    mot_y = ((GdkEventMotion*)event)->y; 
    
    nfui_nfobject_get_offset((NFOBJECT*)nfbargraph, &offx, &offy);    

    drawable = nfui_nfobject_get_window((NFOBJECT *)nfbargraph);
    gc = nfui_nfobject_get_gc((NFOBJECT *)nfbargraph);

    offx = offx + CHART_SPACE_W + nfbargraph->init_bar_gap;
    offy = (offy + nfbargraph->chart_height) - CHART_SPACE_H;

    nfbargraph->cur_pos_idx = -1;
        
    for( i = 0; i< nfbargraph->bar_cnt; i++)
    {
        height = nfbargraph->bar_height[i];
        
        if((mot_x > offx) && (mot_x < (offx+nfbargraph->bar_width)) && (mot_y > (offy-height)) && (mot_y < offy)){

            signal =1;
            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_IDX(688)));
            
            if(nfbargraph->object.use_focus == NFOBJECT_FOCUS_ON)
                gdk_draw_rectangle(drawable, gc, TRUE, offx, offy-height, nfbargraph->bar_width, height);
                
            nfbargraph->cur_pos_value = nfbargraph->bar_value[i];
            nfbargraph->cur_pos_idx = i;

        }
        else{
        
            if (nfbargraph->cur_pos_idx == i){
                offx += (nfbargraph->bar_width)+ (nfbargraph->bar_gap);                                 
                continue;
            }            
            
            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(nfbargraph->bar_color));
            gdk_draw_rectangle(drawable, gc, nfbargraph->bar_filled, offx, offy-height, nfbargraph->bar_width, height);
        }
        
        offx += (nfbargraph->bar_width)+ (nfbargraph->bar_gap);                 
    }

    if(signal)  nfui_user_signal_emit((NFOBJECT*)nfbargraph, NFEVENT_BARGRAPH_SELECTED, FALSE);

    return FALSE;
}

static gboolean _draw_focus_dot(NFBARGRAPH *nfbargraph, GdkEvent *event)
{
    GdkDrawable *drawable;
    GdkGC *gc;
    gint i;
    gint height;
    guint offx, offy;
    gint signal = 0;
    gint p_size = 0;

    p_size = DOT_SIZE/2;

    gint mot_x = 0, mot_y = 0;

    mot_x = ((GdkEventMotion*)event)->x;
    mot_y = ((GdkEventMotion*)event)->y;

    nfui_nfobject_get_offset((NFOBJECT*)nfbargraph, &offx, &offy);

    drawable = nfui_nfobject_get_window((NFOBJECT *)nfbargraph);
    gc = nfui_nfobject_get_gc((NFOBJECT *)nfbargraph);

    offx = offx + CHART_SPACE_W + nfbargraph->init_bar_gap;
    offy = (offy + nfbargraph->chart_height) - CHART_SPACE_H;

    for( i = 0; i < nfbargraph->bar_cnt; i++)
    {
        height = nfbargraph->bar_height[i];

        if((mot_x > offx - p_size) && (mot_x < (offx + p_size)) && (mot_y > (offy - height - p_size)) && ( mot_y < offy - height + p_size))
        {
            signal = 1;

            if (nfbargraph->cur_pos_idx == i)
            {
                offx += (nfbargraph->bar_width) + (nfbargraph->bar_gap);

                continue;
            }

            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(nfbargraph->bar_focus_color));

            if(nfbargraph->object.use_focus == NFOBJECT_FOCUS_ON)
            {
                gdk_gc_set_line_attributes(gc, DOT_SIZE, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);
                gdk_draw_line(drawable, gc, offx - p_size, offy - height, offx + p_size, offy - height);  // draw point
            }
        }
        else
        {
            if(nfbargraph->cur_pos_idx == i)
            {
                offx += (nfbargraph->bar_width) + (nfbargraph->bar_gap);

                continue;
            }

            gdk_gc_set_line_attributes(gc, DOT_SIZE, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);
            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(nfbargraph->bar_color));
            gdk_draw_line(drawable, gc, offx- p_size, offy - height, offx + p_size, offy - height);

        }

        gdk_gc_set_line_attributes(gc, 1, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);

        offx += (nfbargraph->bar_width) + (nfbargraph->bar_gap);
    }

    if(signal)  nfui_user_signal_emit((NFOBJECT*)nfbargraph, NFEVENT_BARGRAPH_MOTION_NOTIFY, FALSE);

    return FALSE;
}

static gboolean _draw_focus_bar(NFBARGRAPH *nfbargraph, GdkEvent *event)
{

    GdkDrawable *drawable;
    GdkGC *gc;
    gint i;
    gint height;
    guint offx, offy;
    gint signal=0;

    gint mot_x=0, mot_y=0;
    
    mot_x = ((GdkEventMotion*)event)->x;
    mot_y = ((GdkEventMotion*)event)->y; 
    
    nfui_nfobject_get_offset((NFOBJECT*)nfbargraph, &offx, &offy);    

    drawable = nfui_nfobject_get_window((NFOBJECT *)nfbargraph);
    gc = nfui_nfobject_get_gc((NFOBJECT *)nfbargraph);

    offx = offx + CHART_SPACE_W + nfbargraph->init_bar_gap;
    offy = (offy + nfbargraph->chart_height) - CHART_SPACE_H;
        
    for( i = 0; i< nfbargraph->bar_cnt; i++)
    {
        height = nfbargraph->bar_height[i];
        
        if((mot_x > offx) && (mot_x < (offx+nfbargraph->bar_width)) && (mot_y > (offy-height)) && (mot_y < offy))
        {
            signal =1;
            if (nfbargraph->cur_pos_idx == i){
                offx += (nfbargraph->bar_width)+ (nfbargraph->bar_gap);
                continue;
            }
            
            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(nfbargraph->bar_focus_color));
            
            if(nfbargraph->object.use_focus == NFOBJECT_FOCUS_ON)
                gdk_draw_rectangle(drawable, gc, TRUE, offx, offy-height, nfbargraph->bar_width, height);
        }
        else{
        
            if (nfbargraph->cur_pos_idx == i){
                offx += (nfbargraph->bar_width)+ (nfbargraph->bar_gap);                                 
                continue;
            }            
            
            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(nfbargraph->bar_color));
            gdk_draw_rectangle(drawable, gc, nfbargraph->bar_filled, offx, offy-height, nfbargraph->bar_width, height);
        }
        
        offx += (nfbargraph->bar_width)+ (nfbargraph->bar_gap);                                 
    }

    if(signal)  nfui_user_signal_emit((NFOBJECT*)nfbargraph, NFEVENT_BARGRAPH_MOTION_NOTIFY, FALSE);

    return FALSE;
}

static gboolean _draw_chart_clear(NFBARGRAPH *nfbargraph, GdkDrawable *drawable, GdkGC *gc, gint off_x, gint off_y, gint size_w, gint size_h)
{  
    gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(nfbargraph->chart_bg_color));
    gdk_draw_rectangle(drawable, gc, TRUE, off_x, off_y, size_w, size_h);

    return FALSE;
}

static gboolean _get_draw_disable_color(NFBARGRAPH *nfbargraph)
{
    if(nfbargraph->object.status == NFOBJECT_STATE_DISABLE){
        nfbargraph->line_color = COLOR_IDX(124);
        nfbargraph->bar_color = COLOR_IDX(124);
    }    
    return FALSE;
}

static gboolean _check_bar_focus(NFBARGRAPH *nfbargraph)
{

    if(!nfui_nfobject_is_shown((NFOBJECT*)nfbargraph))
        nfbargraph->cur_pos_idx = -1;
        
    return FALSE;
}


static gint _draw_none_bargraph(NFBARGRAPH *nfbargraph,GdkDrawable *drawable,GdkGC *gc,gint off_x,gint off_y)
{
	gint gap_value =0;
	gint gap =10;
	gint graph_size_h =0;
	gdouble gap_size_h = 0;
	gdouble max_value = 45;
	gint i;
	gint gap_cnt;
	gint x,y;

	/////////////

	gint width = 0, height =0;
	gint pos_x =0, pos_y=0;
	gint bar_cnt =24, gap_x = 30, init_x_gap =30;

	width = nfbargraph->bar_width;

	_get_bargraph_width(nfbargraph, bar_cnt, &gap_x, init_x_gap, &width, nfbargraph->chart_width);

	pos_x = off_x + CHART_SPACE_W;
	pos_y = (off_y + nfbargraph->chart_height)- CHART_SPACE_H;

    gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(nfbargraph->chart_color));
    gdk_draw_rectangle(drawable, gc, TRUE, pos_x, pos_y, (nfbargraph->chart_width-CHART_SPACE_W)-CHART_SPACE_W_R, CAHRT_LINE_SIZE);
    gdk_draw_rectangle(drawable, gc, TRUE, pos_x, off_y, CAHRT_LINE_SIZE, nfbargraph->chart_height-CHART_SPACE_H);

	pos_x = pos_x + init_x_gap;

	//////////////

	graph_size_h = ((nfbargraph->chart_height-CHART_SPACE_H)-BAR_SPACE_H)-CHART_SPACE_B_H;
	gap_size_h = ((double)graph_size_h*(double)gap)/max_value;
	gap_cnt = graph_size_h/gap_size_h;

	x= off_x + CHART_SPACE_W;
	y= off_y + nfbargraph->chart_height - CHART_SPACE_H;

	_draw_line_value(nfbargraph, drawable, gc, x-CHART_SPACE_W,y, gap_value); // 0 value

	for(i=0; i<gap_cnt; i++)
	{
		y-= gap_size_h;
		gap_value += gap;

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(nfbargraph->line_color));
		gdk_draw_rectangle(drawable, gc, TRUE, x, y, (nfbargraph->chart_width-CHART_SPACE_W)-CHART_SPACE_W_R, LINE_SIZE);

		_draw_line_value(nfbargraph, drawable, gc, x-CHART_SPACE_W, y, gap_value);
	}

	for(i =0; i<bar_cnt; i++)
	{
		_draw_bar_text(nfbargraph, drawable, gc, pos_x, pos_y+CHART_SPACE_H, i);

		pos_x += width + gap_x;
		
	}

	return 0;
}


static gboolean nfbargraph_event_handler(NFBARGRAPH *nfbargraph, GdkEvent *event, gpointer data)
{  
    switch(event->type)
    {
        case GDK_EXPOSE:
        {
            NFOBJECT *top;
            GdkDrawable *drawable;
            GdkGC *gc;
            gint i;
            gint off_x,off_y;
            
            drawable = nfui_nfobject_get_window((NFOBJECT *)nfbargraph);
            gc = nfui_nfobject_get_gc((NFOBJECT *)nfbargraph);

            nfui_nfobject_get_offset((NFOBJECT*)nfbargraph, &off_x, &off_y);

            _get_draw_disable_color(nfbargraph);            
            _get_bar_max_value(nfbargraph);

            /*if(!)
            {
                _draw_chart_clear(nfbargraph,drawable, gc, off_x, off_y, nfbargraph->chart_width, nfbargraph->chart_height);
                _draw_none_bargraph(nfbargraph, drawable, gc, off_x, off_y);
            }
            else
            */

            _check_bar_focus(nfbargraph);

            _draw_chart_clear(nfbargraph,drawable, gc, off_x, off_y, nfbargraph->chart_width, nfbargraph->chart_height);
            _draw_chart_gap(nfbargraph, drawable, gc, off_x, off_y);    
            _draw_bargraph(nfbargraph, drawable, gc ,off_x, off_y);
                
            nfui_nfobject_gc_unref(gc);
        }
        break;

        case GDK_MOTION_NOTIFY:
        {
            if(event->button.button == MOUSE_RIGHT_BUTTON)
                return FALSE;

            if(nfbargraph->graph_type == BAR_GRAPH)
                _draw_focus_bar(nfbargraph, event);
            else
                _draw_focus_dot(nfbargraph, event);
                
        }
        break;        
        
        case GDK_BUTTON_PRESS:
        {
            if(event->button.button == MOUSE_RIGHT_BUTTON)
                return FALSE;

            if(nfbargraph->graph_type == BAR_GRAPH)             
                _draw_press_bar(nfbargraph, event);
            else
                _draw_press_dot(nfbargraph, event);                
        }
        break;
        
        case GDK_2BUTTON_PRESS:
        {   
            if(event->button.button == MOUSE_RIGHT_BUTTON)
                return FALSE;

            if(nfbargraph->graph_type == BAR_GRAPH)
                _2button_press_bar(nfbargraph, event);       
            else
                _2button_press_dot(nfbargraph, event);
        }
        break;
        
        case GDK_DELETE:
        {
            if(nfbargraph->nbuf) g_object_unref(nfbargraph->nbuf);
            nfbargraph->cur_pos_idx = -1;
        }
        break;

        default:
        break;
        
    }
    return FALSE;
}



NFBARGRAPH* nfui_nfbargraph_new(gint width, gint height)
{
    NFBARGRAPH *nfbargraph;
       
    gint i;
    
    nfbargraph = (NFBARGRAPH*)imalloc(sizeof(NFBARGRAPH));
    
    if ((nfbargraph == NULL) || (width ==0) ||(height == 0))
    {
        g_warning(" [NFBARGRAPH CREATE ERROR] \n");
        return NULL;
    }

    nfui_nfobject_init((NFOBJECT*)nfbargraph);

    if(width<1 || height<1){   
        nfbargraph->object.width = 100;
        nfbargraph->object.height = 100;
    }
    else{ 
        nfbargraph->object.width = width;
        nfbargraph->object.height = height;
    }
    nfbargraph->object.type = NFOBJECT_TYPE_NFBARGRAPH;
    nfbargraph->object.use_focus = NFOBJECT_FOCUS_ON;
    nfbargraph->object.default_event_handler = nfbargraph_event_handler;

    nfbargraph->graph_type = BAR_GRAPH;

    nfbargraph->chart_width = nfbargraph->object.width;
    nfbargraph->chart_height = nfbargraph->object.height;
    nfbargraph->bar_filled = TRUE;
    nfbargraph->draw_bar_value = FALSE;
    nfbargraph->draw_line_value = FALSE;
    nfbargraph->draw_line = FALSE;

    nfbargraph->pango_font = nffont_get_pango_font(NFFONT_MINI_NORMAL_3);
    nfbargraph->bar_pango_font = nffont_get_pango_font(NFFONT_MINI_NORMAL_3);

    nfbargraph->chart_color = COLOR_IDX(667);
    nfbargraph->line_color = COLOR_IDX(668);
    nfbargraph->bar_color = COLOR_IDX(686);
    nfbargraph->bar_focus_color = COLOR_IDX(687);
    
    nfbargraph->chart_bg_color = COLOR_IDX(0);   
    
    nfbargraph->cur_pos_value = -1;
    nfbargraph->cur_pos_idx = -1;

    nfbargraph->bar_width = 30;
    nfbargraph->bar_filled = 1;
    
    nfbargraph->init_bar_gap = 5;
    nfbargraph->bar_gap = 15;

    nfbargraph->chart_gap = 0;

    return nfbargraph;
}

gint nfui_nfbargraph_set_chart_color(NFBARGRAPH * nfbargraph, guint color_idx)
{
    if(nfbargraph->object.type != NFOBJECT_TYPE_NFBARGRAPH) return -1;

    nfbargraph->chart_color = color_idx;
    return 0;
}

gint nfui_nfbargraph_set_chart_bg_color(NFBARGRAPH *nfbargraph, guint color_idx)
{
    if(nfbargraph->object.type != NFOBJECT_TYPE_NFBARGRAPH) return -1;
    nfbargraph->chart_bg_color = color_idx;

    return 0;
}

gint nfui_nfbargraph_set_line_color(NFBARGRAPH *nfbargraph, guint color_idx)
{
    if(nfbargraph->object.type != NFOBJECT_TYPE_NFBARGRAPH) return -1;

    nfbargraph->line_color = color_idx;
    return 0;
}

gint nfui_nfbargraph_set_bar_focus_color(NFBARGRAPH *nfbargraph, guint fg_idx)
{
    if(nfbargraph->object.type != NFOBJECT_TYPE_NFBARGRAPH) return -1;

    nfbargraph->bar_focus_color = fg_idx;
    return 0;  
}

gint nfui_nfbargraph_set_bar_value_draw(NFBARGRAPH *nfbargraph, gboolean draw)
{
    if(nfbargraph->object.type != NFOBJECT_TYPE_NFBARGRAPH) return -1;

    nfbargraph->draw_bar_value = draw;
    return 0;
}

gint nfui_nfbargraph_set_barbg_color(NFBARGRAPH * nfbargraph, guint color_idx)
{   
    if(nfbargraph->object.type != NFOBJECT_TYPE_NFBARGRAPH) return -1;

    nfbargraph->bar_color = color_idx;
    return 0;
}

gint nfui_nfbargraph_set_barbg_filled(NFBARGRAPH * nfbargraph, gboolean filled)
{
    if(nfbargraph->object.type != NFOBJECT_TYPE_NFBARGRAPH) return -1;

    nfbargraph->bar_filled = filled;
    return 0;
}

gint nfui_nfbargraph_set_bar_size(NFBARGRAPH * nfbargraph, gint b_width)
{
    if(nfbargraph->object.type != NFOBJECT_TYPE_NFBARGRAPH) return -1;

    nfbargraph->bar_width = b_width; 
    return 0;
}

gint nfui_nfbargraph_set_bar_value(NFBARGRAPH *nfbargraph,gint bar_value, gint bar_idx)
{   
    if(nfbargraph->object.type != NFOBJECT_TYPE_NFBARGRAPH) return -1;

    if(bar_idx < 0) return -1;

    if(bar_value < 0 )
        nfbargraph->bar_value[bar_idx] = 0;
    else 
        nfbargraph->bar_value[bar_idx] = bar_value;

    if(nfbargraph->bar_cnt < bar_idx+1)
        nfbargraph->bar_cnt = bar_idx+1;

    if(nfbargraph->cur_pos_idx == bar_idx)
        nfbargraph->cur_pos_value = nfbargraph->bar_value[bar_idx];
        
    return 0;
}

gint nfui_nfbargraph_set_bar_max_cnt(NFBARGRAPH *nfbargraph, gint max_cnt)
{
    if(nfbargraph->object.type != NFOBJECT_TYPE_NFBARGRAPH) return -1;
    
    if(max_cnt <0 ) return -1;

    nfbargraph->bar_cnt = max_cnt;

    return 0;
}
gint nfui_nfbargraph_set_bar_data_text_int(NFBARGRAPH *nfbargraph, gint bar_data, gint bar_idx)
{
    if(nfbargraph->object.type != NFOBJECT_TYPE_NFBARGRAPH) return -1;

    nfbargraph->bar_data[bar_idx] = bar_data;

    return 0;
}

gint nfui_nfbargraph_set_chart_text(NFBARGRAPH * nfbargraph, gchar *strLabel)
{
    if(nfbargraph->object.type != NFOBJECT_TYPE_NFBARGRAPH) return -1;

    if(strLabel) strcpy(nfbargraph->strLabel, strLabel);
    return 0;
}

gint nfui_nfbargraph_set_bar_data_text(NFBARGRAPH *nfbargraph, const gchar *bar_string, guint idx)
{
    gint length;
    
    if(nfbargraph->object.type != NFOBJECT_TYPE_NFBARGRAPH) return -1;

    length = strlen(bar_string);

    if(length > 0) {
        memset(nfbargraph->bar_info[idx].bar_text, 0x00, sizeof(nfbargraph->bar_info[idx].bar_text));
        strncpy(nfbargraph->bar_info[idx].bar_text, bar_string, sizeof(gchar)*length);
    }
        
    return 0;
}

gint nfui_nfbargraph_set_bar_pango_font(NFBARGRAPH *nfbargraph, gchar *pfont, guint fg_idx)
{
    if(nfbargraph->object.type != NFOBJECT_TYPE_NFBARGRAPH) return -1;

    if(pfont)
    {
        if(nffont_is_system_font(pfont))
        {
            nfbargraph->bar_pango_font = pfont;
        }
        else
        {
            strncpy(nfbargraph->font_name, pfont, sizeof(nfbargraph->font_name));
            nfbargraph->bar_pango_font = nfbargraph->font_name;
        }
    }
    nfbargraph->bar_font_color = fg_idx;

    return 0;
}

gint nfui_nfbargraph_set_pango_font(NFBARGRAPH * nfbargraph, gchar *pfont, guint fg_idx)
{
    if(nfbargraph->object.type != NFOBJECT_TYPE_NFBARGRAPH) return -1;

    if(pfont)
    {
        if(nffont_is_system_font(pfont))
        {
            nfbargraph->pango_font = pfont;
        }
        else
        {
            strncpy(nfbargraph->font_name, pfont, sizeof(nfbargraph->font_name));
            nfbargraph->pango_font = nfbargraph->font_name;
        }
    }

    nfbargraph->font_color = fg_idx;
    
    return 0;
}

/*gint nfui_nfbargraph_set_max_value(NFBARGRAPH * nfbargraph, gint max_value)
{
    if(nfbargraph->object.type != NFOBJECT_TYPE_NFBARGRAPH) return -1;
    
    nfbargraph->bar_max_value = max_value;
    return 0;
}*/

/*gint nfui_nfbargraph_set_chart_gap_draw(NFBARGRAPH * nfbargraph, gint gap_size)
{
    if(nfbargraph->object.type != NFOBJECT_TYPE_NFBARGRAPH) return -1;
    
    nfbargraph->chart_gap = gap_size;
    return 0;
}*/

gint nfui_nfbargraph_set_chart_line_gap_draw(NFBARGRAPH *nfbargraph, gint gap_size, gboolean draw_line, gboolean draw_value)
{
    if(nfbargraph->object.type != NFOBJECT_TYPE_NFBARGRAPH) return -1;

    nfbargraph->chart_gap = gap_size; 
    nfbargraph->draw_line = draw_line;
    nfbargraph->draw_line_value = draw_value;
    
    return 0;
}

gint nfui_nfbargraph_set_bar_gap(NFBARGRAPH *nfbargraph, gint bar_gap)
{
    if(nfbargraph->object.type != NFOBJECT_TYPE_NFBARGRAPH) return -1;

    nfbargraph->bar_gap = bar_gap;
    return 0;
}

gint nfui_nfbargraph_set_bar_init_gap(NFBARGRAPH *nfbargraph, gint init_bar_gap)
{
    if(nfbargraph->object.type != NFOBJECT_TYPE_NFBARGRAPH) return -1;

    if(init_bar_gap <5) init_bar_gap = 5;
    
    nfbargraph->init_bar_gap = init_bar_gap;
    return 0;
}

gint nfui_nfbargraph_set_bar_focus(NFBARGRAPH *nfbargraph, gboolean focus_use)
{
    if(nfbargraph->object.type != NFOBJECT_TYPE_NFBARGRAPH) return -1;

    nfbargraph->object.use_focus = focus_use;
    return 0;
}

gint nfui_nfbargraph_get_bar_focus_value(NFBARGRAPH *nfbargraph)
{
    if(nfbargraph->object.type != NFOBJECT_TYPE_NFBARGRAPH) return -1;

    return nfbargraph->cur_pos_value;
}

gint nfui_nfbargraph_get_bar_focus_index(NFBARGRAPH *nfbargraph)
{
    if(nfbargraph->object.type != NFOBJECT_TYPE_NFBARGRAPH) return -1;

    return nfbargraph->cur_pos_idx;
}

gint nfui_nfbargraph_get_bar_idx_value(NFBARGRAPH *nfbargraph, gint i)
{
    if(nfbargraph->object.type != NFOBJECT_TYPE_NFBARGRAPH) return -1;

    return nfbargraph->bar_value[i];
}

gint nfui_nfbargraph_get_bar_cnt(NFBARGRAPH *nfbargraph)
{
    if(nfbargraph->object.type != NFOBJECT_TYPE_NFBARGRAPH) return -1;

    return nfbargraph->bar_cnt;
}

gint nfui_nfbargraph_del_all_bar(NFBARGRAPH *nfbargraph)
{
    if(nfbargraph->object.type != NFOBJECT_TYPE_NFBARGRAPH) return -1;
    
    guint i;
    
    for( i= 0; i < nfbargraph->bar_cnt; i++)
    {
        nfbargraph->bar_value[i] = 0;
        nfbargraph->bar_data[i] = 0;
        strncpy(nfbargraph->bar_info[i].bar_text, 0, sizeof(nfbargraph->bar_info[i].bar_text));
    }
    nfbargraph->bar_cnt = 0;
    
    
    return 0;
}

gint nfui_nfbargraph_change_graph_type(NFBARGRAPH *nfbargraph, GRAPH_TYPE type)
{
    if(nfbargraph->object.type != NFOBJECT_TYPE_NFBARGRAPH) return -1;

    nfbargraph->graph_type = type;

    return 0;
}

