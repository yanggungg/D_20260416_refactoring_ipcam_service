

/*****************************************************************************************
 *
 *
 *	NFTAB Related...
 *
 *
 * **************************************************************************************/

#include "../../support/event_loop.h"
#include "../../support/util.h"
#include "../../support/nf_ui_font.h"
#include "../../support/color.h"
#include "../../support/multi_language_support.h"

#include "../../tools/nf_ui_tool.h"

#include "nfwindow.h"

#include "nftab.h"
#include "ix_mem.h"

#define	TAB_FONT_SIZE			(DISPLAY_IS_D1 ? NFFONT_XLARGE_SEMI:NFFONT_XLARGE_SEMI)
#define	DEFAULT_TAB_FONT		(nffont_get_pango_font(TAB_FONT_SIZE))
//#define	DEFAULT_TAB_FONT		(nffont_get_pango_font(NFFONT_LARGE_SEMI))

#define TAB_LINE_BORDER			2

#define	TABKEY_REPEAT_INTERVAL		(500)
#if 0 //defined(__SAMSUNG_UI__) // small letter
#define TAB_SPACING                 NORMAL_SPACING
#else
#define TAB_SPACING                 SEMI_CONDENSED_SPACING
#endif

#define	TOOLTIP_DELAY_TIME		(1000)

#ifdef NF_TOOLTIP_ENABLE
static struct TOOLTIP_INFO {
	NFOBJECT *object;
	guint open_src;
};

static struct TOOLTIP_INFO tt_info = {NULL, 0};
static gchar tooltip_str[512];

static gboolean prvKeypadPressed(NFTAB *nftab, KEYPAD_KID dir);


static void nftab_tooltip_set_string(NFTAB *nftab)
{
	gchar *utf_str = NULL;
	gchar cp_str[512];
	gint i;

	utf_str = lookup_string(nftab->strTitle[nftab->index]);
	if(utf_str == NULL)
		utf_str = nftab->strTitle[nftab->index];

	g_memmove(cp_str, utf_str, sizeof(cp_str));
	
	i=0;
	while(cp_str[i] != NULL)
	{
		if(cp_str[i] == '\n')
			cp_str[i] = ' ';
		i++;
	}

	strcpy(tooltip_str, cp_str);
}

static gboolean nftab_tooltip_open(gpointer data)
{
	NFOBJECT *obj;
	
	obj = (NFOBJECT*)data;
	
	nftool_tooltip_show(obj, tooltip_str);

	tt_info.object = obj;
	tt_info.open_src = 0;

	return FALSE;
}

static void nftab_tooltip_close()
{
	nftool_tooltip_hide();

	if(tt_info.open_src)
		g_source_remove(tt_info.open_src);

	tt_info.object = NULL;
	tt_info.open_src = 0;
}
#endif


static gboolean repeat_key_proc(gpointer data)
{
	NFTAB *nftab;

	nftab = (NFTAB*)data;
	
	NFUTIL_THREADS_ENTER();
	prvKeypadPressed(nftab, nftab->rkey_id);
	NFUTIL_THREADS_LEAVE();

	return TRUE;
}

static void nftab_draw_outlines(NFTAB *nftab)
{
	GdkDrawable *drawable = NULL;
	GdkPoint points[5];
	GdkGC *line_gc;
	guint pos_x, pos_y;
	gint line_gap = 0;
	gint line_width;

	drawable = nfui_nfobject_get_window((NFOBJECT*)nftab);
	nfui_nfobject_get_offset((NFOBJECT*)nftab, &pos_x, &pos_y);

	/* outline */
	line_gc = nfui_nfobject_get_gc((NFOBJECT*)nftab);

	gdk_gc_set_rgb_fg_color(line_gc, &UX_COLOR(0));
	gdk_gc_set_line_attributes(line_gc,
			TAB_LINE_BORDER,
			GDK_LINE_SOLID,
			GDK_CAP_NOT_LAST,
			GDK_JOIN_MITER);

	line_gap = TAB_LINE_BORDER - 1;

	if(nftab->direction == NFTAB_DIR_H)
		pos_x += ((nftab->tab_width+2) * nftab->cur_page);
	else
		pos_y += (nftab->tab_height * nftab->cur_page);
/*	
	points[0].x = pos_x + line_gap;
	points[0].y = pos_y + line_gap;
	points[1].x = pos_x + nftab->tab_width - line_gap;
	points[1].y = pos_y + line_gap;
	points[2].x = pos_x + nftab->tab_width - line_gap;
	points[2].y = pos_y + nftab->tab_height - line_gap;
	points[3].x = pos_x + line_gap;
	points[3].y = pos_y + nftab->tab_height - line_gap;
	points[4].x = pos_x + line_gap;
	points[4].y = pos_y + line_gap;

	gdk_draw_lines(drawable,
			line_gc,
			points,
			5);
*/
	gdk_draw_rectangle(drawable,
					line_gc,
					FALSE,
					pos_x + line_gap,
					pos_y + line_gap,
					nftab->tab_width - (line_gap * 2),
					nftab->tab_height - (line_gap * 2));

	nfui_nfobject_gc_unref(line_gc);
}

static guint prvGetClickedTabNum(NFTAB *nftab, guint mx, guint my)
{
	guint pressed_page;
	guint off_x, off_y;
	guint pos;

	nfui_nfobject_get_offset((NFOBJECT*)nftab, &off_x, &off_y);

	if(nftab->direction == NFTAB_DIR_H)
	{
		pos = mx - off_x;
		pressed_page = pos/(nftab->tab_width+2);
	}
	else
	{
		pos = my - off_y;
		pressed_page = pos/nftab->tab_height;
	}


	return pressed_page;
}


static void prvTabClicked(NFTAB *nftab, guint page_num)
{
	guint px, py;
	GdkDrawable *drawable;
	GdkGC *gc;

	if(page_num == nftab->cur_page)
		return;

	drawable = nfui_nfobject_get_window((NFOBJECT*)nftab);
	gc = nfui_nfobject_get_gc((NFOBJECT*)nftab);

	nftab->old_page = nftab->cur_page;
	nftab->cur_page = page_num;

	nfui_nfobject_get_offset((NFOBJECT*)nftab, &px, &py);

	if(nftab->direction == NFTAB_DIR_H)		px += (nftab->tab_width+2) * nftab->cur_page;
	else	py += nftab->tab_height * nftab->cur_page;
	
	if(nftab->pmActive[nftab->cur_page])
	{
		nfutil_draw_pixbuf(drawable, gc, nftab->pmActive[nftab->cur_page], px, py, nftab->tab_width, nftab->tab_height, NFALIGN_LEFT, 0);

#if 0
		nfutil_draw_text_with_pango_spacing("NFTAB_ACTIVE", nftab->pmActive, NULL,  
		                                    drawable, gc, nftab->strTitle[nftab->cur_page], 
		                                    px, py, nftab->tab_width, nftab->tab_height, 
		                                    nftab->pango_font, &UX_COLOR(nftab->color_idx[NFTAB_STATE_ACTIVE]], 
		                                    NFALIGN_LEFT, nftab->margin, TAB_SPACING);
#else
		nfutil_draw_short_text("NFTAB_ACTIVE", nftab->pmActive[nftab->cur_page], NULL,  drawable, gc, nftab->strTitle[nftab->cur_page], nftab->valid_cnt[nftab->cur_page], px, py, nftab->tab_width, nftab->tab_height, nftab->pango_font, &UX_COLOR(nftab->color_idx[NFTAB_STATE_ACTIVE]), NULL, NFALIGN_LEFT, nftab->margin, 0, SEMI_CONDENSED_SPACING);
#endif
	}
	else
	{
		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(nftab->object.bg_color[NFOBJECT_STATE_ACTIVE]));
		gdk_draw_rectangle(drawable, gc, TRUE, px, py, nftab->tab_width, nftab->tab_height);

#if 0
		nfutil_draw_text_with_pango_spacing("NFTAB_ACTIVE", NULL, &UX_COLOR(nftab->object.bg_color[NFOBJECT_STATE_ACTIVE]], 
		                                    drawable, gc, nftab->strTitle[nftab->cur_page], 
		                                    px, py, nftab->tab_width, nftab->tab_height, 
		                                    nftab->pango_font, &UX_COLOR(nftab->color_idx[NFTAB_STATE_ACTIVE]],
		                                    NFALIGN_LEFT, nftab->margin, TAB_SPACING);
#else
		nfutil_draw_short_text("NFTAB_ACTIVE", NULL, &UX_COLOR(nftab->object.bg_color[NFOBJECT_STATE_ACTIVE]), drawable, gc, nftab->strTitle[nftab->cur_page], nftab->valid_cnt[nftab->cur_page], px, py, nftab->tab_width, nftab->tab_height, nftab->pango_font, &UX_COLOR(nftab->color_idx[NFTAB_STATE_ACTIVE]), NULL, NFALIGN_LEFT, nftab->margin, 0, TAB_SPACING);
#endif
	}
#if 0
	nfutil_draw_text_with_pango("NFTAB_ACTIVE", drawable, gc, nftab->strTitle[nftab->cur_page], px, py, nftab->tab_width, nftab->tab_height, nftab->pango_font, &UX_COLOR(nftab->color_idx[NFTAB_STATE_ACTIVE]], NFALIGN_LEFT, nftab->margin);
#endif

	nfui_nfobject_get_offset((NFOBJECT*)nftab, &px, &py);
	
	if(nftab->direction == NFTAB_DIR_H)		px += (nftab->tab_width+2) * nftab->old_page;
	else		py += nftab->tab_height * nftab->old_page;
	
	if(nftab->pmNormal[nftab->old_page])
	{
		nfutil_draw_pixbuf(drawable, gc, nftab->pmNormal[nftab->old_page], px, py, nftab->tab_width, nftab->tab_height, NFALIGN_LEFT, 0);

#if 0
		nfutil_draw_text_with_pango_spacing("NFTAB_NORMAL", nftab->pmNormal, NULL,
		                                    drawable, gc, nftab->strTitle[nftab->old_page], 
		                                    px, py, nftab->tab_width, nftab->tab_height,
		                                    nftab->pango_font, &UX_COLOR(nftab->color_idx[NFTAB_STATE_NORMAL]], 
		                                    NFALIGN_LEFT, nftab->margin, TAB_SPACING);
#else
		nfutil_draw_short_text("NFTAB_ACTIVE", nftab->pmNormal[nftab->old_page], NULL,  drawable, gc, nftab->strTitle[nftab->old_page], nftab->valid_cnt[nftab->old_page], px, py, nftab->tab_width, nftab->tab_height, nftab->pango_font, &UX_COLOR(nftab->color_idx[NFTAB_STATE_NORMAL]), NULL, NFALIGN_LEFT, nftab->margin, 0, SEMI_CONDENSED_SPACING);
#endif
	}
	else
	{
		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(nftab->object.bg_color[NFOBJECT_STATE_NORMAL]));
		gdk_draw_rectangle(drawable, gc, TRUE, px, py, nftab->tab_width, nftab->tab_height);

#if 0
		nfutil_draw_text_with_pango_spacing("NFTAB_NORMAL", NULL, &UX_COLOR(nftab->object.bg_color[NFOBJECT_STATE_NORMAL]], 
		                                    drawable, gc, nftab->strTitle[nftab->old_page], 
		                                    px, py, nftab->tab_width, nftab->tab_height, 
		                                    nftab->pango_font, &UX_COLOR(nftab->color_idx[NFTAB_STATE_NORMAL]], 
		                                    NFALIGN_LEFT, nftab->margin, TAB_SPACING);
#else
		nfutil_draw_short_text("NFTAB_ACTIVE", NULL, &UX_COLOR(nftab->object.bg_color[NFOBJECT_STATE_NORMAL]), drawable, gc, nftab->strTitle[nftab->old_page], nftab->valid_cnt[nftab->old_page], px, py, nftab->tab_width, nftab->tab_height, nftab->pango_font, &UX_COLOR(nftab->color_idx[NFTAB_STATE_NORMAL]), NULL, NFALIGN_LEFT, nftab->margin, 0, TAB_SPACING);
#endif
	}
#if 0
	nfutil_draw_text_with_pango("NFTAB_NORMAL", drawable, gc, nftab->strTitle[nftab->old_page], px, py, nftab->tab_width, nftab->tab_height, nftab->pango_font, &UX_COLOR(nftab->color_idx[NFTAB_STATE_NORMAL]], NFALIGN_LEFT, nftab->margin);
#endif



	if(nftab->page[nftab->old_page])	nfui_nfobject_hide(nftab->page[nftab->old_page]);
	if(nftab->page[nftab->cur_page])	nfui_nfobject_show(nftab->page[nftab->cur_page]);

//	gtk_main_iteration_do(TRUE);

	nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(nftab));
	nfui_signal_emit(nftab->page[nftab->cur_page], GDK_EXPOSE, TRUE);

	if(nftab->draw_outlines) {
		if(nftab->object.kfocus == NFOBJECT_FOCUS) 
			nfui_signal_emit(nftab, GDK_EXPOSE, FALSE);
	}

	nfui_nfobject_gc_unref(gc);
}

static void prvGetBtnPos(NFTAB *nftab, gint index, gint *pos_x, gint *pos_y)
{
	if(nftab->direction == NFTAB_DIR_H)
	{
		*pos_x = (nftab->tab_width+2) * index;
		*pos_y = 0;
	}
	else
	{
		*pos_x = 0;
		*pos_y = nftab->tab_height * index;
	}
}

static void prvNftabDraw(NFTAB *nftab)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	GdkPixbuf *pm;
	guint col_idx;
	guint bg_col;
	guint x, y;
	guint off_x, off_y;
	guint i;
	gchar strBuf[16];

	drawable = nfui_nfobject_get_window((NFOBJECT*)nftab);
	gc = nfui_nfobject_get_gc((NFOBJECT*)nftab);

	nfui_nfobject_get_offset((NFOBJECT*)nftab, &off_x, &off_y);

	if(nftool_cur_language_is_eng())
	{
		nftab->use_strip = 0;
	}

	if(nftab->use_strip)
	{
		for(i=0; i<nftab->pages; i++)
		{
			if(nftab->valid_cnt[i] < 0)
				nftab->valid_cnt[i] = nfutil_string_get_valid_count(1, drawable, nftab->pango_font, nftab->strTitle[i], nftab->tab_width - nftab->margin);
		}
	}
	else
	{
		for(i=0; i<nftab->pages; i++)
		{
			nftab->valid_cnt[i] = 0;
		}
	}

	for(i=0; i<nftab->pages; i++)
	{
		memset(strBuf, 0, sizeof(strBuf));

		if(i == nftab->cur_page)
		{
			pm = nftab->pmActive[i];
			col_idx = nftab->color_idx[NFTAB_STATE_ACTIVE];
			bg_col = nftab->object.bg_color[NFOBJECT_STATE_ACTIVE];
			g_sprintf(strBuf, "%s", "NFTAB_ACTIVE");
		}
		else
		{
			pm = nftab->pmNormal[i];
			col_idx = nftab->color_idx[NFTAB_STATE_NORMAL];
			bg_col = nftab->object.bg_color[NFOBJECT_STATE_NORMAL];
			g_sprintf(strBuf, "%s", "NFTAB_NORMAL");
		}
		
		// added by seongho
		if(nftab->page[i] == NULL) 
		{
			memset(strBuf, 0, sizeof(strBuf));

			pm = nftab->pmNormal[i];
			col_idx = COLOR_IDX(979);
			bg_col = NFUI_DISABLED_COLOR;
			g_sprintf(strBuf, "%s", "NFTAB_DISABLE");
		}

#if 0
		if(nftab->direction == NFTAB_DIR_H)
		{
			x = nftab->tab_width * i;
			y = 0;
		}
		else
		{
			x = 0;
			y = nftab->tab_height * i;
		}
#else
		prvGetBtnPos(nftab, i, &x, &y);
#endif

		x += off_x;
		y += off_y;

		if(pm)
		{
			nfutil_draw_pixbuf(drawable, gc, pm, x, y, nftab->tab_width, nftab->tab_height, NFALIGN_LEFT, 0);

#if 0
			nfutil_draw_text_with_pango_spacing(strBuf, pm, NULL,  
			                                    drawable, gc, nftab->strTitle[i], 
			                                    x, y, nftab->tab_width, nftab->tab_height, 
			                                    nftab->pango_font, &UX_COLOR(col_idx), 
			                                    NFALIGN_LEFT, nftab->margin, TAB_SPACING);
#else
			nfutil_draw_short_text(strBuf, pm, NULL, drawable, gc, nftab->strTitle[i], nftab->valid_cnt[i], x, y, nftab->tab_width, nftab->tab_height, nftab->pango_font, &UX_COLOR(col_idx), NULL, NFALIGN_LEFT, nftab->margin, 0, TAB_SPACING);
#endif
		}
		else
		{
			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_col));
			gdk_draw_rectangle(drawable, gc, TRUE, x, y, nftab->tab_width, nftab->tab_height);
#if 0
			nfutil_draw_text_with_pango_spacing(strBuf, NULL, &UX_COLOR(bg_col], 
			                                    drawable, gc, nftab->strTitle[i], 
			                                    x, y, nftab->tab_width, nftab->tab_height, 
			                                    nftab->pango_font, &UX_COLOR(col_idx], 
			                                    NFALIGN_LEFT, nftab->margin, TAB_SPACING);
#else
			nfutil_draw_short_text(strBuf, NULL, &UX_COLOR(bg_col), drawable, gc, nftab->strTitle[i], nftab->valid_cnt[i], x, y, nftab->tab_width, nftab->tab_height, nftab->pango_font, &UX_COLOR(col_idx), NULL, NFALIGN_LEFT, nftab->margin, 0, TAB_SPACING);
#endif
		}
#if 0
		nfutil_draw_text_with_pango(strBuf, drawable, gc, nftab->strTitle[i], x, y, nftab->tab_width, nftab->tab_height, nftab->pango_font, &UX_COLOR(col_idx], NFALIGN_LEFT, nftab->margin);
#endif
	}

	if(nftab->draw_outlines) {
		if(nftab->object.kfocus == NFOBJECT_FOCUS) 
			nftab_draw_outlines(nftab);
	}

	nfui_nfobject_gc_unref(gc);
}

static gboolean prvKeypadPressed(NFTAB *nftab, KEYPAD_KID dir)
{
	KEYPAD_KID kpid1, kpid2;
	gint i;

	// SKSHIN
//	nfui_idle_timer_reset();

	if(nftab->repeat_key && nftab->rkey_id != dir)
	{
		g_source_remove(nftab->repeat_key);
		nftab->repeat_key = 0;
	}

	if(nftab->direction == NFTAB_DIR_V)
	{
		kpid1 = KEYPAD_UP;
		kpid2 = KEYPAD_DOWN;
	}
	else
	{
		kpid1 = KEYPAD_LEFT;
		kpid2 = KEYPAD_RIGHT;
	}

// check all unregi state
	for (i = 0; i < nftab->pages; i++)
	{
		if (nftab->page[i]) 
			break;
	}

	if (i == nftab->pages) return FALSE;

	i = nftab->cur_page;

	if(dir == kpid1)
	{
		if(!nftab->repeat_key)
		{
			nftab->rkey_id = dir;
//			nftab->repeat_key = g_timeout_add(TABKEY_REPEAT_INTERVAL, repeat_key_proc, nftab);
		}

#if 1		
		do {
			i -= 1;
			if (i < 0) i = nftab->pages-1;			
		} while (!nftab->page[i]);

		if (i == nftab->cur_page) return FALSE;

		nftab->new_page = i;		
#else
		if(nftab->cur_page == 0)
			nftab->new_page = nftab->pages - 1;
		else
			nftab->new_page = nftab->cur_page - 1;
#endif

		// added by seongho
		if(nftab->page[nftab->new_page] == NULL) 
			return FALSE;

		nfui_user_signal_emit((NFOBJECT*)nftab, NFEVENT_TAB_BEFORE_CHANGE, FALSE);

		nfui_on_backscr(nftab);
		nfui_rflip(nftab);

		prvTabClicked(nftab, nftab->new_page);

		nfui_flip(nftab);
		nfui_off_backscr(nftab);

		return TRUE;
	}
	else if(dir == kpid2)
	{
		if(!nftab->repeat_key)
		{
			nftab->rkey_id = dir;
//			nftab->repeat_key = g_timeout_add(TABKEY_REPEAT_INTERVAL, repeat_key_proc, nftab);
		}

#if 1
		do {
			i += 1;
			if (i == nftab->pages) i = 0;			
		} while (!nftab->page[i]);

		if (i == nftab->cur_page) return FALSE;

		nftab->new_page = i;	
#else
		if(nftab->cur_page == nftab->pages - 1)
			nftab->new_page = 0;
		else
			nftab->new_page = nftab->cur_page + 1;
#endif
		
		// added by seongho
		if(nftab->page[nftab->new_page] == NULL) 
			return FALSE;

		nfui_user_signal_emit((NFOBJECT*)nftab, NFEVENT_TAB_BEFORE_CHANGE, FALSE);

		nfui_on_backscr(nftab);
		nfui_rflip(nftab);
		
		prvTabClicked(nftab, nftab->new_page);

		nfui_flip(nftab);
		nfui_off_backscr(nftab);

		return TRUE;
	}
	else if(dir == KEYPAD_EXIT || dir == KEYPAD_ENTER)
	{
#if 0	// by hakeya 2009-06-10..
		nfui_signal_emit(nftab->page[nftab->cur_page], GDK_EXPOSE, TRUE);
#endif
	}

	return FALSE;
}

static gboolean nftab_event_handler(NFTAB *nftab, GdkEvent *event, gpointer data)
{
	switch(event->type)
	{
		case GDK_EXPOSE:
			{
				prvNftabDraw(nftab);
			}
			break;

		case GDK_ENTER_NOTIFY:
			if(nftab->draw_outlines && nfui_nfobject_is_shown((NFOBJECT*)nftab))
				nfui_signal_emit((NFOBJECT*)nftab, GDK_EXPOSE, FALSE);

#ifdef NF_TOOLTIP_ENABLE
			if(tt_info.open_src)
				g_source_remove(tt_info.open_src);

			nftab_tooltip_close();

			if(nfui_nfobject_is_used_tooltip((NFOBJECT*)nftab))
			{
				nftab_tooltip_set_string(nftab);
				tt_info.open_src = g_timeout_add(TOOLTIP_DELAY_TIME, nftab_tooltip_open, nftab);
			}
#endif
			break;

		case GDK_LEAVE_NOTIFY:
			if(nftab->draw_outlines && nfui_nfobject_is_shown((NFOBJECT*)nftab))
				nfui_signal_emit((NFOBJECT*)nftab, GDK_EXPOSE, FALSE);
			
#ifdef NF_TOOLTIP_ENABLE
			nftab->index = -1;
			nftab_tooltip_close();
#endif
			break;

		case GDK_BUTTON_PRESS :
			if(nftab->repeat_key)
			{
				g_source_remove(nftab->repeat_key);
				nftab->repeat_key = 0;
			}

	  	   	if(event->button.button == MOUSE_RIGTH_BUTTON) {					
			  return FALSE;
	  	   	}

			nftab->new_page = prvGetClickedTabNum(nftab, ((GdkEventButton*)event)->x, ((GdkEventButton*)event)->y);
			// added by seongho
			if(nftab->page[nftab->new_page] == NULL) {
				break;
			}

			if (nftab->new_page == nftab->cur_page)
				break;

			nfui_user_signal_emit((NFOBJECT*)nftab, NFEVENT_TAB_BEFORE_CHANGE, FALSE);
			nfui_on_backscr(nftab);
			nfui_rflip(nftab);
			
//			nfui_user_signal_emit((NFOBJECT*)nftab, NFEVENT_TAB_BEFORE_CHANGE, FALSE);
			prvTabClicked(nftab, nftab->new_page);
			
			nfui_flip(nftab);
			nfui_off_backscr(nftab);

			nfui_user_signal_emit((NFOBJECT*)nftab, NFEVENT_TAB_CHANGED, FALSE);			
			break;

		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
		{
			GdkEventKey *kevt;
			KEYPAD_KID dir;

			kevt = (GdkEventKey*)event;
			dir = (KEYPAD_KID)kevt->keyval;

			return prvKeypadPressed(nftab, dir);
		}
		break;

		case GDK_MOTION_NOTIFY:
		{
			gint pidx;
			gint mx, my;
			gint off_x, off_y;

			if(nftab->repeat_key)
			{
				g_source_remove(nftab->repeat_key);
				nftab->repeat_key = 0;
			}

			mx = ((GdkEventMotion*)event)->x;
			my = ((GdkEventMotion*)event)->y;

			pidx = prvGetClickedTabNum(nftab, mx, my);

			if(pidx < 0 || pidx > nftab->pages || nftab->page[pidx] == NULL)
				return FALSE;

#if 1
			if(pidx == nftab->index)
			{
				return FALSE;
			}

			if(nftab->direction == NFTAB_DIR_H)
			{
				off_x = (nftab->tab_width+2) * pidx;
				off_y = 0;
			}
			else
			{
				off_x = 0;
				off_y = nftab->tab_height * pidx;
			}
			
#endif
			nftab->index = pidx;

#ifdef NF_TOOLTIP_ENABLE
			if(tt_info.open_src)
				g_source_remove(tt_info.open_src);

			nftab_tooltip_close();
			
			if(nfui_nfobject_is_used_tooltip((NFOBJECT*)nftab))
			{
#ifdef	SHOW_TOOLTIP_ONLY_STRIP_STRING
				if(nftab->valid_cnt[nftab->index] != 0)
				{
					nftab_tooltip_set_string(nftab);
					tt_info.open_src = g_timeout_add(TOOLTIP_DELAY_TIME, nftab_tooltip_open, nftab);
				}
#else	//#ifdef	SHOW_TOOLTIP_ONLY_STRIP_STRING
				nftab_tooltip_set_string(nftab);
				tt_info.open_src = g_timeout_add(TOOLTIP_DELAY_TIME, nftab_tooltip_open, nftab);
#endif	//#ifdef	SHOW_TOOLTIP_ONLY_STRIP_STRING
			}
#endif
		}
		break;

		case GDK_BUTTON_RELEASE:
		case NFEVENT_KEYPAD_RELEASE:
		case NFEVENT_REMOCON_RELEASE:
			if(nftab->repeat_key)
			{
				g_source_remove(nftab->repeat_key);
				nftab->repeat_key = 0;
			}
			break;

		case GDK_DELETE :
#ifdef NF_TOOLTIP_ENABLE
			nftab->index = -1;
			nftab_tooltip_close();
#endif

			if(nftab->repeat_key)
			{
				g_source_remove(nftab->repeat_key);
				nftab->repeat_key = 0;
			}

			break;

		default:
			break;
	}

	return FALSE;
}


#ifdef NF_TOOLTIP_ENABLE
static void nftab_tooltip_info(NFOBJECT *obj, gchar *tooltip, gint *off_x, gint *off_y)
{
	NFTAB *nftab;

	nftab = (NFTAB*)obj;

	if(nftab->direction == NFTAB_DIR_H)
	{
		*off_x = (nftab->tab_width+2) * nftab->index;
		*off_y = 0;
	}
	else
	{
		*off_x = 0;
		*off_y = nftab->tab_height * nftab->index;
	}

	strncpy(tooltip, nftab->strTitle[nftab->index], sizeof(gchar)*strlen(nftab->strTitle[nftab->index]));
}
#endif

NFTAB* nfui_nftab_new(guint pages, gchar *images[2], guint width, guint height, nftab_dir_type direction, gchar **strTitle, guint color_idx[3])
{
	NFTAB *nftab;
	guint i;

	nftab = (NFTAB*)imalloc(sizeof(NFTAB));
	if(nftab == NULL)	return NULL;

	nfui_nfobject_init((NFOBJECT*)nftab);

	if(direction == NFTAB_DIR_V)
	{
		nftab->object.width = width;
		nftab->object.height = height * pages;
	}
	else
	{
		nftab->object.width = width * pages;
		nftab->object.height = height;
	}
	
	nftab->object.type = NFOBJECT_TYPE_NFTAB;
	nftab->object.use_focus = NFOBJECT_FOCUS_ON;	//OFF;
	nftab->object.bg_color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(0);
	nftab->object.bg_color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(0);
	nftab->object.default_event_handler = nftab_event_handler;
	nftab->object.use_tooltip = 1;

	nftab->pages = pages;
	nftab->cur_page = 0;
	nftab->old_page = -1;
	nftab->tab_width = width;
	nftab->tab_height = height;
	nftab->margin = 0;
	nftab->direction = direction;
	nftab->draw_outlines = FALSE;
	
	for(i=0; i<NFSETUP_MAX_PAGE; i++)
		nftab->valid_cnt[i] = -1;

	nftab->use_strip = 1;

	if(images)
	{
		if(images[0])	nftab->pmNormal[0] = nfui_get_image_from_file(images[0], NULL);
		if(images[1])	nftab->pmActive[0] = nfui_get_image_from_file(images[1], NULL);
	}
	else
	{
		nftab->pmNormal[0] = NULL;
		nftab->pmActive[0] = NULL;
	}

	for(i=0; i<NFSETUP_MAX_PAGE; i++)
	{
		nftab->pmNormal[i] = nftab->pmNormal[0];
		nftab->pmActive[i] = nftab->pmActive[0];

		nftab->valid_cnt[i] = -1;
	}

	strcpy(nftab->font_name, DEFAULT_TAB_FONT);
	nftab->pango_font = nffont_get_pango_font(TAB_FONT_SIZE);

	nftab->color_idx[0] = color_idx[0];
	nftab->color_idx[1] = color_idx[1];
	nftab->color_idx[2] = color_idx[2];

	nftab->index = -1;

	for(i=0; i<pages; i++)
	{
		if(strTitle)
		g_stpcpy(nftab->strTitle[i], strTitle[i]);
		else
			g_stpcpy(nftab->strTitle[i], "");
	}

	return nftab;
}


void nfui_nftab_destroy(NFTAB *nftab)
{

}

void nfui_nftab_regi_page(NFTAB *nftab, NFOBJECT *page, guint nth)
{
	if(nftab->object.type != NFOBJECT_TYPE_NFTAB)
	{
		return;
	}

	nftab->page[nth] = page;
}

void nfui_nftab_regi_page_all(NFTAB *nftab, NFOBJECT **page, guint page_cnt)
{
	guint i;

	if(nftab->object.type != NFOBJECT_TYPE_NFTAB)
	{
		return;
	}

	for(i=0; i<page_cnt; i++)
	{
		nftab->page[i] = page[i];
	}
}

void nfui_nftab_unregi_page(NFTAB *nftab, guint nth)
{
	if(nftab->object.type != NFOBJECT_TYPE_NFTAB)
	{
		return;
	}
	
	if(nftab->page[nth] != NULL) 
		nftab->page[nth] = NULL;
}

NFOBJECT* nfui_nftab_get_nth_page(NFTAB *nftab, gint idx)
{
	if(nftab->object.type != NFOBJECT_TYPE_NFTAB)
	{
		return;
	}

	return nftab->page[idx];
}

void nfui_nftab_set_fg_color(NFTAB *nftab, guint normal, guint active, guint prelight)
{
	if(nftab->object.type != NFOBJECT_TYPE_NFTAB)
	{
		return;
	}

	nftab->color_idx[0] = normal;
	nftab->color_idx[1] = active;
	nftab->color_idx[2] = prelight;
}

void nfui_nftab_set_bg_color(NFTAB *nftab, guint normal, guint active)
{
	if(nftab->object.type != NFOBJECT_TYPE_NFTAB)
	{
		return;
	}

//	nftab->bg_normal = normal;
//	nftab->bg_active = active;
	nftab->object.bg_color[NFOBJECT_STATE_NORMAL] = normal;
	nftab->object.bg_color[NFOBJECT_STATE_ACTIVE] = active;
}


void nfui_nftab_set_pango_font(NFTAB *nftab, const gchar *pfont)
{
	if(nftab->object.type != NFOBJECT_TYPE_NFTAB)
	{
		return;
	}

//	g_stpcpy(nftab->pango_font, font);
	if (pfont) {
		if (nffont_is_system_font(pfont))
			nftab->pango_font = pfont;
		else {
			strncpy(nftab->font_name, pfont, sizeof(nftab->font_name));
			nftab->pango_font = nftab->font_name;
		}
	}
}

void nfui_nftab_set_margin(NFTAB *nftab, guint margin)
{
	if(nftab->object.type != NFOBJECT_TYPE_NFTAB)
	{
		return;
	}

	nftab->margin = margin;
}

gint nfui_nftab_get_old_page(NFTAB *nftab)
{
	if(nftab->object.type != NFOBJECT_TYPE_NFTAB)
	{
		return -1;
	}

	return nftab->old_page;
}

void nfui_nftab_set_cur_page(NFTAB *nftab, gint page ) //wiggls - otm
{
	if(nftab->object.type != NFOBJECT_TYPE_NFTAB)
	{
		return -1;
	}

	nftab->cur_page = page;
}

gint nfui_nftab_get_cur_page(NFTAB *nftab)
{
	if(nftab->object.type != NFOBJECT_TYPE_NFTAB)
	{
		return -1;
	}

	return nftab->cur_page;
}

gint nfui_nftab_get_new_page(NFTAB *nftab)
{
	if(nftab->object.type != NFOBJECT_TYPE_NFTAB)
	{
		return -1;
	}

	return nftab->new_page;
}

gint nfui_nftab_set_new_page(NFTAB *nftab, gint page)
{
	if(nftab->object.type != NFOBJECT_TYPE_NFTAB)
	{
		return -1;
	}

	nftab->new_page = page;

	return 0;
}

guint nfui_nftab_get_clicked_page(NFTAB *nftab, guint x, guint y)
{
	if(nftab->object.type != NFOBJECT_TYPE_NFTAB)
	{
		return 0;
	}

	return prvGetClickedTabNum(nftab, x, y);
}

void nfui_nftab_set_draw_outlines(NFTAB *nftab, gboolean canDraw)
{
	if(nftab->object.type != NFOBJECT_TYPE_NFTAB)
	{
		return 0;
	}

	if(nftab->draw_outlines != canDraw)
		nftab->draw_outlines = canDraw;
}

void nfui_nftab_use_strip(NFTAB* nftab, gboolean use)
{
	if(nftab->object.type != NFOBJECT_TYPE_NFTAB)
	{
		return;
	}

	if(use)	nftab->use_strip = 1;
	else	nftab->use_strip = 0;
}

void nfui_nftab_set_images(NFTAB *nftab, gint cnt, gchar *nor_img[NFSETUP_MAX_PAGE], gchar *act_img[NFSETUP_MAX_PAGE])
{
	gint i;

	if(nftab->object.type != NFOBJECT_TYPE_NFTAB)
	{
		return;
	}

	for(i=0; i<cnt; i++)
	{
		if(nor_img[i])	nftab->pmNormal[i] = nfui_get_image_from_file(nor_img[i], NULL);
		if(act_img[i])	nftab->pmActive[i] = nfui_get_image_from_file(act_img[i], NULL);
	}
}

void nfui_nftab_set_init_page(NFTAB *nftab, guint page)
{
	if(nftab->object.type != NFOBJECT_TYPE_NFTAB)
	{
		return;
	}
	
	nftab->cur_page = page;
}

void nfui_nftab_change_page(NFTAB *nftab, guint page_num)
{
	prvTabClicked((NFTAB*)nftab, page_num);
}

gint nfui_nftab_is_valid_page(NFTAB *nftab, guint nth)
{
	if (nftab->object.type != NFOBJECT_TYPE_NFTAB)
	{
		return -1;
	}

	if (!nftab->page[nth]) return 0;

	return 1;
}


