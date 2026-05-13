#include "ix_queue.h"

static void ix_queue_delete_element (gpointer element, gpointer user_data);

// [ Private Member Function ]

static void
ix_queue_delete_element (gpointer element, gpointer user_data)
{
	g_queue_remove ((GQueue *)user_data, element);
}

// [ Public Member Function ]
IXQueue* ix_queue_new()
{
	IXQueue *ixqueue;

	if (g_thread_supported() == FALSE) {
		g_thread_init(NULL);
	}
	
	ixqueue = g_new(IXQueue, 1);
	
	ixqueue->mutex = g_mutex_new ();
	ixqueue->q = g_queue_new();
		
	return ixqueue;
}


void 
ix_queue_async_push(IXQueue *ixqueue, gpointer data)
{
	g_queue_push_head(ixqueue->q, data);
}

gpointer ix_queue_async_pop(IXQueue *ixqueue)
{
	gpointer data;
	data = g_queue_pop_tail(ixqueue->q);
	
	return data;
}

gpointer ix_queue_async_stack_pop (IXQueue *ixqueue)
{
	gpointer data;
	data = g_queue_pop_head(ixqueue->q);
	
	return data;
}

void ix_queue_sync_push(IXQueue *ixqueue, gpointer data)
{
	g_mutex_lock (ixqueue->mutex);
	g_queue_push_head(ixqueue->q, data);
	g_mutex_unlock (ixqueue->mutex);
}

gpointer ix_queue_sync_pop(IXQueue *ixqueue)
{
	gpointer data;
	
	g_mutex_lock (ixqueue->mutex);
	data = g_queue_pop_tail(ixqueue->q);
	g_mutex_unlock (ixqueue->mutex);
	
	return data;
}

gpointer ix_queue_peek(IXQueue *ixqueue, int nth)
{
	gpointer data;
	
	g_mutex_lock (ixqueue->mutex);
	data = g_queue_peek_nth(ixqueue->q, nth);
	g_mutex_unlock (ixqueue->mutex);
	
	return data;
}

gpointer ix_queue_sync_stack_pop (IXQueue *ixqueue)
{
	gpointer data;
	
	g_mutex_lock (ixqueue->mutex);
	data = g_queue_pop_head(ixqueue->q);
	g_mutex_unlock (ixqueue->mutex);
	
	return data;
}

void
ix_queue_async_delete_all (IXQueue *ixqueue)
{
	g_queue_foreach (ixqueue->q, ix_queue_delete_element, ixqueue->q);
}

void
ix_queue_sync_delete_all (IXQueue *ixqueue)
{
	g_mutex_lock (ixqueue->mutex);
	g_queue_foreach (ixqueue->q, ix_queue_delete_element, ixqueue->q);
	g_mutex_unlock (ixqueue->mutex);
}

gboolean ix_queue_is_empty (IXQueue *ixqueue)
{
	return g_queue_is_empty(ixqueue->q);
}

gboolean ix_queue_sync_is_empty (IXQueue *ixqueue)
{
	gboolean ret;
	g_mutex_lock (ixqueue->mutex);
	ret = g_queue_is_empty(ixqueue->q);
	g_mutex_unlock (ixqueue->mutex);
	return ret;
}

gint ix_queue_get_sizeof (IXQueue *ixqueue)
{
	return g_queue_get_length(ixqueue->q);
}

gint ix_queue_sync_get_sizeof (IXQueue *ixqueue)
{
	gint ret;
	g_mutex_lock (ixqueue->mutex);
	ret = g_queue_get_length(ixqueue->q);
	g_mutex_unlock (ixqueue->mutex);
	return ret;
}


void ix_queue_free (IXQueue *ixqueue)
{
	g_mutex_free(ixqueue->mutex);
	g_queue_free(ixqueue->q);
	g_free(ixqueue);
	
}



