#ifndef _NF_TIMER_H
#define _NF_TIMER_H

#include <glib.h>

gboolean
nf_timer_init();

guint
nf_timer_add( guint interval, GSourceFunc cb_func, gpointer data);

gboolean
nf_timer_remove( guint tag);

#endif

