

#include "../../support/event_loop.h"
#include "../../support/nf_ui_color.h"
#include "../../support/util.h"

#include "../../tools/nf_ui_debug.h"

#include "nftimeline2.h"
#include "ix_mem.h"

#define	NFTL2_BORDER_SIZE		2
#define	NFTL2_HEADLINE_SIZE		12

#define NFTL2_BORDER_COLOR			COLOR_IDX(NOT_CARE)
#define NFTL2_SEL_BORDER_COLOR		COLOR_IDX(NOT_CARE)

#define	NFTL2_DEF_BG_COLOR			COLOR_IDX(NOT_CARE)
#define	NFTL2_DEF_TBAR_COLOR		COLOR_IDX(NOT_CARE)
#define	NFTL2_DEF_DATA_COLOR1		COLOR_IDX(NOT_CARE)
#define	NFTL2_DEF_DATA_COLOR2		COLOR_IDX(NOT_CARE)
#define	NFTL2_DEF_DATA_COLOR3		COLOR_IDX(NOT_CARE)
#define	NFTL2_DEF_DATA_COLOR4		COLOR_IDX(NOT_CARE)
#define	NFTL2_DEF_DATA_COLOR5		COLOR_IDX(NOT_CARE)
#define	NFTL2_DEF_DATA_COLOR6		COLOR_IDX(NOT_CARE)

#define	NFTL2_OFFTOPOS		(6)	



static gboolean nftimeline2_event_handler(NFTIMELINE2 *nftl2, GdkEvent *event, gpointer data)
{
	switch(event->type)
	{
		case GDK_EXPOSE:
		{
			GdkDrawable *drawable = NULL;
			GdkGC *odd_bg = NULL;
			GdkGC *even_bg = NULL;
			GdkGC *gc = NULL;
			GdkFont *time_font = NULL;
			gint off_x, off_y;
			gint x, y, w, h;
			gint tx, ty, tw;
			gint count, temp;
			gint off_pos;

			GTimeVal tvStartTime;
			struct tm tmStartTime;

			guint i, j;

			drawable = nfui_nfobject_get_window((NFOBJECT*)nftl2);

			gc = gdk_gc_new(drawable);
			odd_bg = gdk_gc_new(drawable);
			even_bg = gdk_gc_new(drawable);

			gdk_gc_set_rgb_fg_color(odd_bg, &colors[nftl2->row_color[0]]);
			gdk_gc_set_rgb_fg_color(even_bg, &colors[nftl2->row_color[1]]);

			nfui_nfobject_get_offset((NFOBJECT*)nftl2, &off_x, &off_y);


			if(nftl2->object.kfocus != NFOBJECT_FOCUS)
				nftl2->object.grab_kfocus = FALSE;

			/* Time */
#if 1
			tvStartTime = nftl2->start_time;
			tvStartTime.tv_sec += (nftl2->offset * 3600);

			time_font = nffont_get_font(NFFONT_SMALL_SEMI);

			temp = nftl2->da_w / 12;
			x = off_x + nftl2->da_x;
			y = off_y;
			w = temp;
			h = 12;

			for(i=0; i<12; i++)
			{
				tmStartTime = *NFLOCALTIME(&(tvStartTime.tv_sec));
				tvStartTime.tv_sec += 7200;

				nfutil_draw_text(gc, drawable, g_strdup_printf("%02d", tmStartTime.tm_hour), x, y, w, h, time_font, &colors[COLOR_IDX(NOT_CARE)], NFALIGN_LEFT, 0);

				x += temp;
			}
			g_printf("\n");
#endif


			/* Outline */
			if(nftl2->object.kfocus == NFOBJECT_FOCUS)
			{
				if(nftl2->object.grab_kfocus)
					gdk_gc_set_rgb_fg_color(gc, &colors[NFTL2_SEL_BORDER_COLOR]);
				else
					gdk_gc_set_rgb_fg_color(gc, &colors[NFTL2_BORDER_COLOR]);

				x = off_x;
				y = off_y + NFTL2_HEADLINE_SIZE;
				w = nftl2->object.width;
				h = NFTL2_BORDER_SIZE;
				gdk_draw_rectangle(drawable, gc, TRUE, x, y, w, h);

				x = off_x;
				y = off_y + nftl2->da_y;;
				w = NFTL2_BORDER_SIZE;
				h = nftl2->da_h;
				gdk_draw_rectangle(drawable, gc, TRUE, x, y, w, h);

				x = off_x;
				y = off_y + nftl2->da_y + nftl2->da_h;
				w = nftl2->object.width;
				h = NFTL2_BORDER_SIZE;
				gdk_draw_rectangle(drawable, gc, TRUE, x, y, w, h);

				x = off_x + nftl2->da_x + nftl2->da_w;
				y = off_y + nftl2->da_y;
				w = NFTL2_BORDER_SIZE;
				h = nftl2->da_h;
				gdk_draw_rectangle(drawable, gc, TRUE, x, y, w, h);
			}

			/* Line BG */
			x = off_x + nftl2->da_x;
			y = off_y + nftl2->da_y;
			w = nftl2->da_w;
			h = nftl2->cell_height;

			for(i=0; i<nftl2->rows; i++)
			{
				if(i%2)	gdk_draw_rectangle(drawable, odd_bg, TRUE, x, y, w, h);
				else	gdk_draw_rectangle(drawable, even_bg, TRUE, x, y, w, h);

				y += (nftl2->cell_height + nftl2->row_space);
			}

			/* Line Data */
			x = off_x + nftl2->da_x;
			y = off_y + nftl2->da_y + 4;
			h = nftl2->cell_height - 8;

			off_pos = nftl2->offset * NFTL2_OFFTOPOS;

			if(NFTL2_MAX_COLUMNS <= off_pos + nftl2->columns)
			{
				g_warning("[TIMELINE2] Not enough timeline data");
				return FALSE;
			}

			for(j=0; j<nftl2->rows; j++)
			{
				count = 0;
				temp = nftl2->data[j][off_pos];
				ty = y + ((nftl2->cell_height + nftl2->row_space)*j);

				for(i=0; i<nftl2->columns; i++)
				{
					if(nftl2->data[j][i+off_pos] == temp)
					{
						count++;

						if(i == nftl2->columns-1)
							i++;
						else
							continue;
					}

					if(temp != 0)
					{
						tx = x + (nftl2->cell_width * (i-count));
						tw = nftl2->cell_width * count;

						gdk_gc_set_rgb_fg_color(gc, &colors[nftl2->data_color[temp]]);
						gdk_draw_rectangle(drawable, gc, TRUE, tx, ty, tw, h);
					}

					if(i < nftl2->columns)
					{
						temp = nftl2->data[j][i+off_pos];
						count = 1;
					}
				}
			}

			/* time bar */
			gdk_gc_set_rgb_fg_color(gc, &colors[NFTL2_DEF_TBAR_COLOR]);

			off_pos = nftl2->offset * NFTL2_OFFTOPOS;

			x = off_x + nftl2->da_x + ((off_pos + nftl2->cur_pos)*nftl2->cell_width) - 3;
			y = off_y + nftl2->da_y;
			w = 8;
			h = 2;

			if(x < off_x + nftl2->da_x)
			{
				w = (off_x+nftl2->da_x - x);
				x = off_x + nftl2->da_x;

			}

			if(x + w > off_x + nftl2->da_x + nftl2->da_w)
				w = x + w- (off_x+nftl2->da_x + nftl2->da_w); 

			gdk_draw_rectangle(drawable, gc, TRUE, x, y, w, h);

			x = off_x + nftl2->da_x + ((off_pos + nftl2->cur_pos)*nftl2->cell_width) - 2;
			y += 2;
			w = 6;
			if(x < off_x + nftl2->da_x)
			{
				w = (off_x+nftl2->da_x - x);
				x = off_x + nftl2->da_x;

			}

			if(x + w > off_x + nftl2->da_x + nftl2->da_w)
				w = x + w- (off_x+nftl2->da_x + nftl2->da_w); 

			gdk_draw_rectangle(drawable, gc, TRUE, x, y, w, h);
			x = off_x + nftl2->da_x + ((off_pos + nftl2->cur_pos)*nftl2->cell_width) - 1;
			y += 2;
			w = 4;
			if(x < off_x + nftl2->da_x)
			{
				w = (off_x+nftl2->da_x - x);
				x = off_x + nftl2->da_x;

			}

			if(x + w > off_x + nftl2->da_x + nftl2->da_w)
				w = x + w - (off_x+nftl2->da_x + nftl2->da_w); 
			gdk_draw_rectangle(drawable, gc, TRUE, x, y, w, h);



			x = off_x + nftl2->da_x + ((off_pos + nftl2->cur_pos)*nftl2->cell_width);
			y = off_y + nftl2->da_y;
			w = nftl2->cell_width;
			h = nftl2->da_h;

			gdk_draw_rectangle(drawable, gc, TRUE, x, y, w, h);



			g_object_unref(gc);
			g_object_unref(odd_bg);
			g_object_unref(even_bg);
		}
		break;

		case GDK_MOTION_NOTIFY:
			if(!nftl2->is_pressed)
				break;

		case GDK_BUTTON_PRESS:
		{
			GdkEventButton* bevt;
			gint x, y, w, h;
			gint mx, my;
			gint old_pos;
			gint new_pos;

			bevt = (GdkEventButton*)event;
			mx = bevt->x;
			my = bevt->y;

			old_pos = nftl2->cur_pos;

			nfui_nfobject_get_offset((NFOBJECT*)nftl2, &x, &y);
			x = mx - x;
			y = my - y;

			if(x<nftl2->da_x || x>=nftl2->da_x+nftl2->da_w)
				return FALSE;
			if(y<nftl2->da_y || y>=nftl2->da_y+nftl2->da_h)
				return FALSE;

			new_pos = x/nftl2->cell_width;

			nftl2->cur_pos = new_pos;
			nftl2->is_pressed = 1;

#if 1
			nfui_signal_emit((NFOBJECT*)nftl2, GDK_EXPOSE, TRUE);
#else
			GdkDrawable *drawable = NULL;
			GdkGC *gc = NULL;
			drawable = nfui_nfobject_get_window((NFOBJECT*)nftl2);
			gc = gdk_gc_new(drawable);

			nfui_nfobject_get_offset((NFOBJECT*)nftl2, &x, &y);
			x += nftl2->da_x + (new_pos*nftl2->cell_width);
			y += nftl2->da_y;
			w = nftl2->cell_width;
			h = nftl2->da_h;

			gdk_gc_set_rgb_fg_color(gc, &colors[NFTL2_DEF_TBAR_COLOR]);
			gdk_draw_rectangle(drawable, gc, TRUE, x, y, w, h);

			g_object_unref(gc);
#endif
		}
		break;

		case GDK_BUTTON_RELEASE:
			nftl2->is_pressed = FALSE;
			nfui_user_signal_emit((NFOBJECT*)nftl2, NFEVENT_TIMELINE_CHANGED, FALSE);
			break;

		case GDK_LEAVE_NOTIFY:
			if(nftl2->is_pressed)
			{
				nftl2->is_pressed = FALSE;
				nfui_user_signal_emit((NFOBJECT*)nftl2, NFEVENT_TIMELINE_CHANGED, FALSE);
			}
			break;

		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
		{
			GdkEventKey *kevt;
			KEYPAD_KID kpid;
			guint off_x, off_y;
			gint gap;
			gint i;

			kevt = (GdkEventKey*)event;
			kpid = (KEYPAD_KID)kevt->keyval;

			nfui_nfobject_get_offset((NFOBJECT*)nftl2, &off_x, &off_y);

			if(kpid == KEYPAD_ENTER)
			{
				if(!nftl2->object.grab_kfocus)
				{
					nftl2->object.grab_kfocus = TRUE;
					nfui_signal_emit((NFOBJECT*)nftl2, GDK_EXPOSE, TRUE);
				}
				return TRUE;
			}
			else if(kpid == KEYPAD_EXIT)
			{
				if(!nftl2->object.grab_kfocus) 
					return FALSE;

				nftl2->object.grab_kfocus = FALSE;
				nfui_signal_emit((NFOBJECT*)nftl2, GDK_EXPOSE, TRUE);
				return TRUE;
			}
			else if(kpid == KEYPAD_RIGHT)
			{
				if(!nftl2->object.grab_kfocus) 
					return FALSE;

				if(nftl2->cur_pos == nftl2->columns-1)
					return TRUE;

				nftl2->cur_pos += 2;

				if(nftl2->cur_pos > nftl2->columns-1)
					nftl2->cur_pos = nftl2->columns-1;

				nfui_signal_emit((NFOBJECT*)nftl2, GDK_EXPOSE, TRUE);
				nfui_user_signal_emit((NFOBJECT*)nftl2, NFEVENT_TIMELINE_CHANGED, FALSE);

				return TRUE;

			}
			else if(kpid == KEYPAD_LEFT)
			{
				if(!nftl2->object.grab_kfocus) 
					return FALSE;

				if(nftl2->cur_pos == 0)
					return TRUE;

				nftl2->cur_pos -= 2;

				if(nftl2->cur_pos < 0)
					nftl2->cur_pos = 0;

				nfui_signal_emit((NFOBJECT*)nftl2, GDK_EXPOSE, TRUE);
				nfui_user_signal_emit((NFOBJECT*)nftl2, NFEVENT_TIMELINE_CHANGED, FALSE);

				return TRUE;
			}
			else
			{
				if(nftl2->object.grab_kfocus)
					return TRUE;
			}
		}

		break;

		case GDK_DELETE:
			DP_OBJECT((NFOBJECT*)nftl2, -1);
			break;


		default:
		break;
	}

	return FALSE;
}

NFOBJECT* nfui_nftimeline2_new(guint cell_w, guint cell_h, guint cols, guint rows, guint row_space)
{
	NFTIMELINE2 *timeline2 = NULL;

	if(!cell_w || !cell_h || !cols || !rows)
		return NULL;

	timeline2 = (NFTIMELINE2*)imalloc(sizeof(NFTIMELINE2));

	if(timeline2 == NULL)	{
		g_warning("[NFTIMELINE2] Memory Allocation Fail..");
		return NULL;
	}

	timeline2->object.parent = NULL;
	timeline2->object.x = 0;
	timeline2->object.y = 0;
	timeline2->object.width = cell_w * cols + (NFTL2_BORDER_SIZE * 2);
	timeline2->object.height = (cell_h + row_space) * rows - row_space + NFTL2_HEADLINE_SIZE + (NFTL2_BORDER_SIZE * 2);
	timeline2->object.type = NFOBJECT_TYPE_NFTIMELINE2;
	timeline2->object.status = NFOBJECT_STATE_NORMAL;
	timeline2->object.bg_color[NFOBJECT_STATE_NORMAL] = -1;
	timeline2->object.bg_color[NFOBJECT_STATE_PRELIGHT] = -1;
	timeline2->object.bg_color[NFOBJECT_STATE_ACTIVE] = -1;
	timeline2->object.show = NFOBJECT_HIDE;
	timeline2->object.use_focus = NFOBJECT_FOCUS_ON;
	timeline2->object.kfocus = NFOBJECT_UNFOCUS;
	timeline2->object.mfocus = NFOBJECT_UNFOCUS;
	timeline2->object.grab_kfocus = 0;

	timeline2->object.pre_event_handler = NULL;
	timeline2->object.default_event_handler = nftimeline2_event_handler;
	timeline2->object.post_event_handler = NULL;

	timeline2->cell_width = cell_w;
	timeline2->cell_height = cell_h;
	timeline2->data_height = cell_h-4;

	timeline2->columns = cols;
	timeline2->rows = rows;

	timeline2->row_space = row_space;

	timeline2->cur_pos = 0;
	timeline2->is_pressed = 0;

	timeline2->da_x = NFTL2_BORDER_SIZE;
	timeline2->da_y = NFTL2_HEADLINE_SIZE + NFTL2_BORDER_SIZE;
	timeline2->da_w = timeline2->object.width - (NFTL2_BORDER_SIZE*2);
	timeline2->da_h = timeline2->object.height - timeline2->da_y - NFTL2_BORDER_SIZE;

	memset(&(timeline2->start_time), 0, sizeof(GTimeVal));

	timeline2->offset = 0;
	timeline2->resolution = 0;
	timeline2->data_num = 0;
//	timeline2->pre_data = 0;
//	timeline2->post_data = 0;

	memset(timeline2->data, 0, sizeof(timeline2->data));
	memset(timeline2->row_color, -1, sizeof(gint)*2);
	memset(timeline2->data_color, -1, sizeof(gint)*10);

	

	timeline2->row_color[0] = NFTL2_DEF_BG_COLOR;
	timeline2->row_color[1] = NFTL2_DEF_BG_COLOR;

	timeline2->data_color[0] = NFTL2_DEF_DATA_COLOR1;
	timeline2->data_color[1] = NFTL2_DEF_DATA_COLOR2;
	timeline2->data_color[2] = NFTL2_DEF_DATA_COLOR3;
	timeline2->data_color[3] = NFTL2_DEF_DATA_COLOR4;
	timeline2->data_color[4] = NFTL2_DEF_DATA_COLOR5;
	timeline2->data_color[5] = NFTL2_DEF_DATA_COLOR6;

#if 0

/*********************************************/
/* for test */
	guint k, l, u;

	for(k=0; k<NFTL2_MAX_ROWS; k++)
	{
		for(l=0; l<NFTL2_MAX_COLUMNS; l++)
		{
			if(l<40)		u = k+1;
			else if(l<80)	u = k+2;
			else if(l<120)	u = k+3;
			else if(l<160)	u = k+4;
			else if(l<200)	u = k+5;
			else if(l<240)	u = k+6;
			else if(l<280)	u = k+7;
			else if(l<300)	u = k+8;
			
			timeline2->data[k][l] = u%6;
		}
	}
/***********************************************/
#endif

	DP_OBJECT((NFOBJECT*)timeline2, 1);

	return timeline2;
}



void nfui_nftimeline2_set_data(NFTIMELINE2 *nftl2, gchar *t_elem, GTimeVal start, gint resolution, gint count,  gint offset)
{
	gint x, y;

	if(!nftl2)	return;
	if(!t_elem)	return;

	if(nftl2->object.type != NFOBJECT_TYPE_NFTIMELINE2)
	{
		P_TYPE_DISMATCH_WARNING(nftl2);
		return;
	}

	memset(nftl2->data, 0, sizeof(nftl2->data));

	for(y=0; y<nftl2->rows; y++)
	{
		for(x=0; x<count; x++)
			nftl2->data[y][x] = t_elem[count*y+x];
	}

	nftl2->start_time = start;
	nftl2->offset = offset;
	nftl2->resolution = resolution;
	nftl2->data_num = count;
}

void nfui_nftimeline2_set_position(NFTIMELINE2 *nftl2, gint pos)
{
	if(nftl2->object.type != NFOBJECT_TYPE_NFTIMELINE2)
	{
		P_TYPE_DISMATCH_WARNING(nftl2);
		return;
	}

	nftl2->cur_pos = pos;
}

gint nfui_nftimeline2_get_position(NFTIMELINE2 *nftl2)
{
	if(nftl2->object.type != NFOBJECT_TYPE_NFTIMELINE2)
	{
		P_TYPE_DISMATCH_WARNING(nftl2);
		return 0;
	}

	return nftl2->cur_pos;
}

void nfui_nftimeline2_set_tv(NFTIMELINE2 *nftl2, GTimeVal tv)
{
	time_t gab = 0;

	if(nftl2->object.type != NFOBJECT_TYPE_NFTIMELINE2)
	{
		P_TYPE_DISMATCH_WARNING(nftl2);
		return;
	}

	gab = nftl2->start_time.tv_sec - tv.tv_sec + (nftl2->offset * 3600);

	nftl2->cur_pos = gab / nftl2->resolution;
}

void nfui_nftimeline2_get_tv(NFTIMELINE2 *nftl2, GTimeVal *tv)
{
	if(nftl2->object.type != NFOBJECT_TYPE_NFTIMELINE2)
	{
		P_TYPE_DISMATCH_WARNING(nftl2);
		return;
	}

}

void nfui_nftimeline2_set_offset(NFTIMELINE2 *nftl2, gint offset)
{
	if(nftl2->object.type != NFOBJECT_TYPE_NFTIMELINE2)
	{
		P_TYPE_DISMATCH_WARNING(nftl2);
		return;
	}

	nftl2->offset = offset;
}

gint nfui_nftimeline2_get_offset(NFTIMELINE2 *nftl2, gint offset)
{
	if(nftl2->object.type != NFOBJECT_TYPE_NFTIMELINE2)
	{
		P_TYPE_DISMATCH_WARNING(nftl2);
		return 0;
	}

	return nftl2->offset;
}




