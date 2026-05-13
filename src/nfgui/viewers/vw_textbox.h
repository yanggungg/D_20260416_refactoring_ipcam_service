/*
 *
 *
 */

#ifndef _TBOX_H
#define _TBOX_H

#include <glib.h> 
#include <glib/gprintf.h>

#include <gtk/gtk.h>

#if !defined(_OTM_MODEL) && !defined(_SNF_MODEL)
#include <glade/glade.h>
#include <glade/glade-build.h>
#endif


typedef gpointer TBOX;


TBOX open_textbox(guint pos_x, guint pos_y);
void show_text(TBOX tbox, const gchar *format, ...);
void close_textbox(TBOX tbox);
void show_text_idle(TBOX tbox, gboolean empty);
void delete_idle_text(TBOX tbox); 
 



#endif
