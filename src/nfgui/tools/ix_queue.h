#ifndef __IX_QUEUE_H__
#define __IX_QUEUE_H__

#include <glib.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct _IXQueue IXQueue;

struct _IXQueue {
	GMutex *mutex;
	GQueue *q;
};

IXQueue* ix_queue_new ();

void ix_queue_async_push (IXQueue *ixqueue, gpointer data);
gpointer ix_queue_async_pop (IXQueue *ixqueue);
gpointer ix_queue_async_stack_pop (IXQueue *ixqueue);

void ix_queue_sync_push (IXQueue *ixqueue, gpointer data);
gpointer ix_queue_sync_pop (IXQueue *ixqueue);
gpointer ix_queue_sync_stack_pop (IXQueue *ixqueue);
gpointer ix_queue_peek(IXQueue *ixqueue, int nth);

void ix_queue_async_delete_all (IXQueue *ixqueue);
void ix_queue_sync_delete_all (IXQueue *ixqueue);

gboolean ix_queue_is_empty (IXQueue *ixqueue);
gboolean ix_queue_sync_is_empty (IXQueue *ixqueue);
gint ix_queue_get_sizeof (IXQueue *ixqueue);
gint ix_queue_sync_get_sizeof (IXQueue *ixqueue);

void ix_queue_free(IXQueue *ixqueue);

#endif // __IX_QUEUE_H__
























