#include <string.h>

#include "support/color.h"

#include "vw_textbox.h"

#define TBOX_SIZE_W					(guint)(DISPLAY_IS_D1 ? 500:900)
#define TBOX_SIZE_H					(guint)(DISPLAY_IS_D1 ? 260:420)

#define TBOX_IDLE_CNT				12

static guchar dCnt = 0;

static gboolean
tbox_view_cb(GtkWidget *widget, GdkEvent *event, gpointer data) 
{
	switch(event->type) {
		case GDK_NOTHING:
		case GDK_EXPOSE:
		case GDK_DELETE:
		case GDK_DESTROY:
			return FALSE;
		default:
			return TRUE;
	}
}


TBOX open_textbox(guint pos_x, guint pos_y)
{
	GtkWidget *tbox_win;
	GtkWidget *tbox_view;
	GtkTextBuffer *buffer;
	GtkTextIter iter;
	
	/* window */
	tbox_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_modal(GTK_WINDOW(tbox_win), TRUE);
	gtk_window_set_type_hint(GTK_WINDOW(tbox_win), GDK_WINDOW_TYPE_HINT_SPLASHSCREEN);
	gtk_window_move(GTK_WINDOW(tbox_win), (gint)pos_x, (gint)pos_y);
	gtk_container_set_border_width(GTK_CONTAINER(tbox_win), 4);
#if 1
	gtk_widget_set_size_request(tbox_win, TBOX_SIZE_W, TBOX_SIZE_H);
#else
	gtk_widget_set_size_request(tbox_win, TBOX_SIZE_W, -1);
#endif

#if defined(__XRPLUS_UI__) || defined(__SAMSUNG_UI__) || defined(__I3DVR_UI__) || defined(__OTM_UI__) 
	gtk_widget_modify_bg(tbox_win, GTK_STATE_NORMAL, &UX_COLOR(NOT_CARE));
#elif defined(__BTYPE_UI__) || defined(__CTYPE_UI__) || defined(__NF_700_UI__) || defined(__NF_46_UI__)
	gtk_widget_modify_bg(tbox_win, GTK_STATE_NORMAL, &UX_COLOR(NOT_CARE));
#elif defined(__STL_UI__)
	gtk_widget_modify_bg(tbox_win, GTK_STATE_NORMAL, &UX_COLOR(NOT_CARE));
#elif defined(__OTM_UI__)
	gtk_widget_modify_bg(tbox_win, GTK_STATE_NORMAL, &UX_COLOR(NOT_CARE));
#else 
	gtk_widget_modify_bg(tbox_win, GTK_STATE_NORMAL, &UX_COLOR(NOT_CARE));
#endif

	/* text view */
	tbox_view = gtk_text_view_new();
	gtk_text_view_set_editable(GTK_TEXT_VIEW(tbox_view), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(tbox_view), FALSE);
#if defined(__STL_UI__) 
	gtk_widget_modify_base(tbox_view, GTK_STATE_NORMAL, &UX_COLOR(NOT_CARE));
#elif defined(__NF_700_UI__)
	gtk_widget_modify_base(tbox_view, GTK_STATE_NORMAL, &UX_COLOR(NOT_CARE));
#elif defined(__OTM_UI__)
	gtk_widget_modify_base(tbox_view, GTK_STATE_NORMAL, &UX_COLOR(NOT_CARE));
#else
	gtk_widget_modify_base(tbox_view, GTK_STATE_NORMAL, &UX_COLOR(NOT_CARE));
#endif
#if defined(__ATYPE_UI__) 
	gtk_widget_modify_text(tbox_view, GTK_STATE_NORMAL, &UX_COLOR(NOT_CARE));
#elif defined(__CTYPE_UI__)
	gtk_widget_modify_text(tbox_view, GTK_STATE_NORMAL, &UX_COLOR(NOT_CARE));
#else
	gtk_widget_modify_text(tbox_view, GTK_STATE_NORMAL, &UX_COLOR(NOT_CARE));
#endif
	g_signal_connect(G_OBJECT(tbox_view), "event", G_CALLBACK(tbox_view_cb), NULL);

	/* text buffer */
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tbox_view));
	gtk_text_buffer_get_end_iter(buffer, &iter);
	gtk_text_buffer_create_tag(buffer, "big",
			"size", (guint)(DISPLAY_IS_D1 ? (10 * PANGO_SCALE):(20 * PANGO_SCALE)), 
			NULL);
	gtk_text_buffer_create_tag(buffer, "big_gap_before_line",
			"pixels_above_lines", 4, 
			NULL);
	gtk_text_buffer_create_tag(buffer, "big_gap_after_line",
			"pixels_below_lines", 4, 
			NULL);
	gtk_text_buffer_create_tag(buffer, "wide_margins",
			"left_margin", 10,
			"right_margin", 10,
			NULL);
	gtk_text_buffer_create_mark(buffer, "end_position",
								&iter, FALSE);

	gtk_widget_show(tbox_view);

	gtk_container_add(GTK_CONTAINER(tbox_win), tbox_view);
	gtk_widget_show(tbox_win);
	
	return tbox_view;
}

void
show_text(TBOX tbox, const gchar *format, ...)
{
	GtkTextBuffer *buffer;
	GtkTextIter iter;
	const gchar *fmt;
	gchar buf[128];
	gint ival;
	GString *string = NULL;
	va_list ap;

	g_return_if_fail(G_IS_OBJECT(tbox));


	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tbox));
	gtk_text_buffer_get_end_iter(buffer, &iter); 

	memset(buf, 0, strlen(buf));
		
	string = g_string_sized_new(128);

	va_start(ap, format);
	for(fmt = format; *fmt; fmt++) {
		if(*fmt != '%'){
			string = g_string_append_c(string, *fmt);
			continue;
		}
		
		switch(*++fmt) {
			case 'd':
				ival = va_arg(ap, gint);
				g_sprintf(buf, "%d", ival);
				string = g_string_append(string, buf);
				break;
			case 's':
				string = g_string_append(string, va_arg(ap, gchar*));
				break;
		}
	}	

	gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, 
											string->str, -1, 
											"big", 
											"big_gap_before_line", 
											"big_gap_after_line", 
											"wide_margins", 
											NULL);
	
	g_string_free(string, TRUE);

	va_end(ap);
}

#if 1

void 
show_text_idle(TBOX tbox, gboolean empty)
{
	GtkTextBuffer *buffer;
	GtkTextIter start, end;
	gchar dot[3] = " .";
	gchar empty_str[3] = "  ";
	gint e_offset = 0;
	gint lines = 0;

	gchar *strTemp;

	if(empty)	strTemp = empty_str;
	else		strTemp = dot;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tbox));
	gtk_text_buffer_get_end_iter(buffer, &end); 
	
	if(!empty && (dCnt > TBOX_IDLE_CNT)) {
		lines = gtk_text_buffer_get_line_count(buffer);
		e_offset = gtk_text_iter_get_offset(&end);

		gtk_text_buffer_get_iter_at_line(buffer, &start, lines);
		gtk_text_iter_set_offset(&start, (gint)(e_offset - ((TBOX_IDLE_CNT * 2) + 2)));		

		gtk_text_buffer_delete(buffer, &start, &end);

		end = start;

		dCnt = 0;
	}

	gtk_text_buffer_insert_with_tags_by_name(buffer, &end, 
			strTemp, -1, 
			"big", 
			"big_gap_before_line", 
			"big_gap_after_line", 
			"wide_margins", 
			NULL);

	if(!empty) dCnt++;
}
#else
void 
show_text_idle(TBOX tbox, gboolean empty)
{
	GtkTextBuffer *buffer;
	GtkTextIter start, end;
	gchar dot[3] = " .";
	gchar empty_str[3] = "  ";
	gchar cCnt;

	gchar *strTemp;

	if(empty)	strTemp = empty_str;
	else		strTemp = dot;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tbox));
	cCnt = gtk_text_buffer_get_char_count(buffer);
	gtk_text_buffer_get_end_iter(buffer, &end); 

	if(cCnt > 100) {
		gtk_text_buffer_get_iter_at_line(buffer, &start, cCnt);

		gtk_text_buffer_delete(buffer, &start, &end);

		end = start;
	}

	gtk_text_buffer_insert_with_tags_by_name(buffer, &end, 
			strTemp, -1, 
			"big", 
			"big_gap_before_line", 
			"big_gap_after_line", 
			"wide_margins", 
			NULL);
}
#endif

void 
delete_idle_text(TBOX tbox) 
{
	GtkTextBuffer *buffer;
	GtkTextIter start, end;
	gchar cCnt;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tbox));
	cCnt = gtk_text_buffer_get_char_count(buffer);

	gtk_text_buffer_get_iter_at_line(buffer, &start, cCnt);
	gtk_text_buffer_get_end_iter(buffer, &end); 
	gtk_text_buffer_delete(buffer, &start, &end);

	if(dCnt != 0) dCnt = 0;
}


void 
close_textbox(TBOX tbox)
{
	GtkWidget *parent;

	parent = gtk_widget_get_parent(GTK_WIDGET(tbox));

	gtk_widget_destroy(parent);
}



