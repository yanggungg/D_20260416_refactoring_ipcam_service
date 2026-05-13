#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfobject.h"
#include "objects/nftile.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"

#include "tools/nf_ui_function.h"

#include "vw_motion_draw.h"
#include "vsm.h"


#define  DRAW_SIZE      (2)



////////////////////////////////////////////////////////////
//
// private data type 
//

typedef struct _MDRAW_T {
    gint        plt_x;
    gint        plt_y;
    gint        plt_w;
    gint        plt_h;
	NFOBJECT	*obj;
	gint        ch;    
	gint        rows;
	gint        cols;
	guint		color;
    gint        day_start;
    gint        day_end;
    gint        day_sense;
    gint        night_sense;	
	guchar		s_area[1024];	
	guchar		prev_data[1024];
	guchar		curr_data[1024];
} MDRAW_T;



////////////////////////////////////////////////////////////
//
// private variable
//

static guint motion_tid = 0;
static MDRAW_T mdraw;



////////////////////////////////////////////////////////////
//
// private functions
//

static gint _draw_rect(gint x, gint y, gint w, gint h)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc = NULL;

	if (!mdraw.obj) return -1;
	
	drawable = nfui_nfobject_get_window(mdraw.obj);
	if(!drawable) return -1;
	
	gc = gdk_gc_new(drawable);

	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(mdraw.color));
	gdk_draw_rectangle(drawable, gc, TRUE, x, y, w, h);	

	g_object_unref(gc);	

	return 0;
}

static gint _erase_rect(gint x, gint y, gint w, gint h)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc = NULL;

	if (!mdraw.obj) return -1;
	
	drawable = nfui_nfobject_get_window(mdraw.obj);
	if(!drawable) return -1;

	gc = gdk_gc_new(drawable);

	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_000000)));
	gdk_draw_rectangle(drawable, gc, TRUE, x, y, w, h);	

	g_object_unref(gc);	

	return 0;
}

static gint _is_changed_data(gint row, gint col)
{
    if (mdraw.curr_data[row*mdraw.cols+col] == mdraw.prev_data[row*mdraw.cols+col])
        return -1;

    return 0;
}

static gint _is_selected_area(gint row, gint col)
{
    if (mdraw.s_area[row*mdraw.cols+col] != '1') return -1;

    return 0;
}

static gint _check_sensitivity(gint row, gint col)
{
	gint val, hour;
	gint ret;

	dtf_get_local_hourmin(time(0), &hour, 0, 0);

    val = (gint)mdraw.curr_data[row*mdraw.cols+col];
   
    if ((hour < mdraw.day_start) || (hour > mdraw.day_end))     // night time
    {
        // sensitivity range : 1~10, val : 0~10
        if (val >= (11-mdraw.night_sense)) ret = 1;
        else ret = 0;
    }
    else                                                        // day time
    {
        // sensitivity range : 1~10, val : 0~10
        if (val >= (11-mdraw.day_sense)) ret = 1;
        else ret = 0;
    }

    return ret;
}

static gboolean _update_motion_raw(gpointer data)
{
	gint row, col;
	gint init_x, init_y, cell_w, cell_h;	
	guint x, y, w, h;
	gint ret;

    gint draw_w, draw_h;

    scm_get_mraw_data(mdraw.ch, mdraw.curr_data);

	if (!mdraw.cols) return TRUE;
	if (!mdraw.rows) return TRUE;

	cell_w = mdraw.plt_w/mdraw.cols;
	cell_h = mdraw.plt_h/mdraw.rows;	

    draw_w = cell_w/DRAW_SIZE;
    draw_h = cell_h/DRAW_SIZE;

	init_x = mdraw.plt_x + (cell_w-draw_w)/2;
	init_y = mdraw.plt_y + (cell_h-draw_h)/2;

	x = init_x;
	y = init_y;
	w = draw_w;
	h = draw_h;

	for(row = 0; row < mdraw.rows; row++)
	{		
		for(col = 0; col < mdraw.cols; col++)
		{
            if ((_is_changed_data(row, col) == 0) && (_is_selected_area(row, col) == 0))
            {                
                if (_check_sensitivity(row, col))
                    ret = _draw_rect(x, y, w, h);
                else
                    ret = _erase_rect(x, y, w, h);

                if (ret == 0)
                    mdraw.prev_data[row*mdraw.cols+col] = mdraw.curr_data[row*mdraw.cols+col];
            }

			x += cell_w;
		}

		x = init_x;
		y += cell_h;		
	}	
	
	return TRUE;
}

static void _clear_motion_raw(gpointer data)
{
	gint row, col;
	gint init_x, init_y, cell_w, cell_h;	
	guint x, y, w, h;
	gint ret;

    gint draw_w, draw_h;

	if (!mdraw.cols) return TRUE;
	if (!mdraw.rows) return TRUE;

	cell_w = mdraw.plt_w/mdraw.cols;
	cell_h = mdraw.plt_h/mdraw.rows;	

    draw_w = cell_w/DRAW_SIZE;
    draw_h = cell_h/DRAW_SIZE;

	init_x = mdraw.plt_x + (cell_w-draw_w)/2;
	init_y = mdraw.plt_y + (cell_h-draw_h)/2;

	x = init_x;
	y = init_y;
	w = draw_w;
	h = draw_h;

	for(row = 0; row < mdraw.rows; row++)
	{		
		for(col = 0; col < mdraw.cols; col++)
		{
            _erase_rect(x, y, w, h);
			x += cell_w;
		}

		x = init_x;
		y += cell_h;		
	}	
}

static gint _start_motion_draw()
{
    if (!motion_tid)
        motion_tid = g_timeout_add_full(G_PRIORITY_DEFAULT, 100, _update_motion_raw, NULL, _clear_motion_raw);
    
	return 0;
}

static gint _stop_motion_draw()
{
	if (motion_tid) 
	{
	    g_source_remove(motion_tid);
    	motion_tid = 0;
    }
	return 0;    
}



////////////////////////////////////////////////////////////
//
// public interfaces
//

void VW_MotionDraw_init(NFWINDOW *parent)
{
	memset(&mdraw, 0x00, sizeof(MDRAW_T));
    mdraw.ch = -1;
}

void VW_MotionDraw_finalize()
{
	memset(&mdraw, 0x00, sizeof(MDRAW_T));
    mdraw.ch = -1;    
    _stop_motion_draw();    
}

gint VW_MotionDraw_set_plt_position(gint pos_x, gint pos_y, gint width, gint height)
{
    mdraw.plt_x = pos_x;
    mdraw.plt_y = pos_y;
    mdraw.plt_w = width;
    mdraw.plt_h = height;

    return 0;    
}

gint VW_MotionDraw_set_obj(gint ch, NFOBJECT *mdraw_obj)
{
    mdraw.ch = ch;
	mdraw.obj = mdraw_obj;
	mdraw.color = COLOR_PRG_IDX(UX_COLOR_0000FF);
	scm_get_motion_size(ch, &mdraw.rows, &mdraw.cols);
	return 0;
}

gint VW_MotionDraw_set_selectArea(guchar area[1024])
{
	memcpy(mdraw.s_area, area, sizeof(guchar)*1024);
    return 0;
}

gint VW_MotionDraw_set_daytime(gint start, gint end)
{
    mdraw.day_start = start;
	mdraw.day_end = end;
    return 0;
}

gint VW_MotionDraw_set_sense(gint day_sense, gint night_sense)
{
    mdraw.day_sense = day_sense;
	mdraw.night_sense = night_sense;
    return 0;
}

gint VW_MotionDraw_start()
{
    _start_motion_draw();
    return 0;
}

gint VW_MotionDraw_stop()
{
	memset(mdraw.s_area, 0x00, sizeof(guchar)*1024);
	memset(mdraw.prev_data, 0x00, sizeof(guchar)*1024);
	memset(mdraw.curr_data, 0x00, sizeof(guchar)*1024);
    _stop_motion_draw();
    return 0;
}


