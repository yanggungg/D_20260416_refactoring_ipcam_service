


#include "../../support/util.h"
#include "../../support/color.h"

#include "nfobject.h"
#include "nfprogressbar.h"
#include "ix_mem.h"

static gboolean nfprog_event_handler(NFPROGRESSBAR *nfprog, GdkEvent *event, gpointer data)
{
	switch(event->type)
	{
		case GDK_EXPOSE:
		{
			GdkDrawable *drawable;
			GdkGC *gc;

			guint fill_cnt;
			guint x, y;
			guint pos_x = 0;
			guint i;


			drawable = nfui_nfobject_get_window((NFOBJECT*)nfprog);
			gc = nfui_nfobject_get_gc((NFOBJECT*)nfprog);

			nfui_nfobject_get_offset((NFOBJECT*)nfprog, &x, &y);

			fill_cnt = (nfprog->rate * nfprog->img_num) / 100;
			
			if(nfprog->bg && nfprog->head && nfprog->tail && nfprog->img)
			{
				// restart drawing
				if(nfprog->filled_cnt > fill_cnt) {
					if(nfprog->draw) {
						g_object_unref(nfprog->draw);
						nfprog->draw = NULL;
					}
					nfprog->filled_cnt = 0;
				}

				if(nfprog->draw == NULL)
					nfprog->draw = gdk_pixbuf_copy(nfprog->bg);

				if(fill_cnt != 0) 
				{
					// head
					if(fill_cnt != 0 && nfprog->filled_cnt == 0) {
						gdk_pixbuf_copy_area(nfprog->head,
								0, 0,
								nfprog->head_width,
								nfprog->img_height,
								nfprog->draw,
								0, 0);
					}

					if(fill_cnt != nfprog->filled_cnt) {
						// body
						pos_x = nfprog->head_width;
						pos_x += (nfprog->filled_cnt * nfprog->img_width);

						for(i=0; i<(fill_cnt - nfprog->filled_cnt); i++) {
							gdk_pixbuf_copy_area(nfprog->img,
									0, 0,
									nfprog->img_width,
									nfprog->img_height,
									nfprog->draw,
									pos_x, 0);

							pos_x += nfprog->img_width;
						}

						// tail
						if((pos_x + nfprog->tail_width) > nfprog->bg_width)
							pos_x -= ((pos_x + nfprog->tail_width) - nfprog->bg_width);

						gdk_pixbuf_copy_area(nfprog->tail,
								0, 0,
								nfprog->tail_width,
								nfprog->img_height,
								nfprog->draw,
								pos_x, 0);

						nfprog->filled_cnt = fill_cnt;
					}
				}

				nfutil_draw_pixbuf(drawable, gc, nfprog->draw, (gint)x, (gint)y, (gint)nfprog->bg_width, (gint)nfprog->img_height, NFALIGN_LEFT, nfprog->margin);
			}
			else 
			{
				if(fill_cnt < nfprog->img_num && nfprog->rate)
					fill_cnt++;

				for(i=0; i<fill_cnt; i++)
				{
					nfutil_draw_pixbuf(drawable, gc, nfprog->img, (gint)(2*i*nfprog->img_width+x), (gint)y+2, (gint)nfprog->img_width, (gint)nfprog->img_height, NFALIGN_LEFT, nfprog->margin);
				}
			}

			nfui_nfobject_gc_unref(gc);
		}
		break;

		case GDK_ENTER_NOTIFY:
		case GDK_LEAVE_NOTIFY:
#if 0
			if(nfui_nfobject_is_shown((NFOBJECT*)nfprog))
				nfui_signal_emit((NFOBJECT*)nfprog, GDK_EXPOSE, TRUE);
#endif
			break;

		case GDK_DELETE :
			if(nfprog)
			{
				if(nfprog->draw) {
					g_object_unref(nfprog->draw);
					nfprog->draw = NULL;
				}
			}
			break;

		default:
			break;
	}

	return FALSE;
}

NFPROGRESSBAR* nfui_nfprogressbar_new(GdkPixbuf *img, guint width)
{
	NFPROGRESSBAR* nfprog;
	guint temp;

	nfprog = (NFPROGRESSBAR*)imalloc(sizeof(NFPROGRESSBAR));
	if(nfprog == NULL)
	{
		g_warning("[NFPROGRESSBAR CREATE ERROR]\n");
		return NULL;
	}

	nfprog->img = img;

	nfui_get_pixbuf_size(img, &(nfprog->img_width), &(nfprog->img_height));

	temp = width / nfprog->img_width;
	if(temp%2)
	{
		nfprog->img_num = temp/2 + 1;
		nfprog->margin = (width % nfprog->img_width) / 2;
	}
	else
	{
		nfprog->img_num = temp/2;
		nfprog->margin = (width % nfprog->img_width + nfprog->img_width) / 2;
	}

	nfprog->rate = 0;

	nfui_nfobject_init((NFOBJECT*)nfprog);

	nfprog->object.width = width;
	nfprog->object.height = nfprog->img_height+4;
	nfprog->object.type = NFOBJECT_TYPE_NFPROGRESSBAR;
#if 0
	nfprog->object.bg_color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(TODO_COLOR);
	nfprog->object.bg_color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(TODO_COLOR);
	nfprog->object.bg_color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(TODO_COLOR);
#endif
	nfprog->object.default_event_handler = nfprog_event_handler;

	return nfprog;
}

NFPROGRESSBAR* nfui_nfprogressbar_new_with_images(GdkPixbuf *bg, GdkPixbuf *head, GdkPixbuf *body, GdkPixbuf *tail)
{
	NFPROGRESSBAR* nfprog;
	gint size_h = 0;
	guint temp;

	nfprog = (NFPROGRESSBAR*)imalloc(sizeof(NFPROGRESSBAR));
	if(nfprog == NULL)
	{
		g_warning("[NFPROGRESSBAR CREATE ERROR]\n");
		return NULL;
	}

	nfprog->bg   = bg;
	nfprog->head = head;
	nfprog->tail = tail;
	nfprog->img  = body;

	// body size
	nfui_get_pixbuf_size(nfprog->img, &(nfprog->img_width), &(nfprog->img_height));

	// head size
	nfui_get_pixbuf_size(nfprog->head, &(nfprog->head_width), &size_h);

	// tail size
	nfui_get_pixbuf_size(nfprog->tail, &(nfprog->tail_width), &size_h);

	// bg size
	nfui_get_pixbuf_size(nfprog->bg, &nfprog->bg_width, &size_h);
	temp = nfprog->bg_width - (nfprog->head_width + nfprog->tail_width);

	nfprog->img_num = temp / nfprog->img_width;
	if(temp % nfprog->img_width)
		nfprog->img_num += 1;

	nfui_nfobject_init((NFOBJECT*)nfprog);

	nfprog->object.width = nfprog->bg_width;
	nfprog->object.height = (guint)size_h;
	nfprog->object.type = NFOBJECT_TYPE_NFPROGRESSBAR;
#if 0
	nfprog->object.bg_color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(TODO_COLOR);
	nfprog->object.bg_color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(TODO_COLOR);
	nfprog->object.bg_color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(TODO_COLOR);
#endif
	nfprog->object.default_event_handler = nfprog_event_handler;

	return nfprog;
}

void nfui_nfprogressbar_destroy(NFPROGRESSBAR* nfprog)
{
}

void nfui_nfprogressbar_set_rate(NFPROGRESSBAR* nfprog, gint rate)
{
	if(nfprog->object.type != NFOBJECT_TYPE_NFPROGRESSBAR)
	{
		return;
	}

	if(rate<0 || rate>100) {
		g_warning("[PROGRESSBAR WARNING] Wrong rate! %s : value[%d]\n", __func__, rate);
		return;
	}

	nfprog->rate = rate;

	if(rate == 100)
		nfprog->filled_cnt = 0;
}


guint nfui_nfprogressbar_get_rate(NFPROGRESSBAR* nfprog)
{
	if(nfprog->object.type != NFOBJECT_TYPE_NFPROGRESSBAR)
	{
		return 0;
	}

	return nfprog->rate;
}
