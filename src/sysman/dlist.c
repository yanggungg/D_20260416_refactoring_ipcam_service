/*
 * dlist.c
 *
 */

#include <stdio.h>
#include "dlist.h"

void dlist_init(struct dlist *head)
{
	((struct dlist *)head)->next = head;	
	((struct dlist *)head)->prev = head;	

	return;
}

static void _add_dlist(struct dlist volatile *prev, struct dlist volatile *node,
	struct dlist volatile *next)
{
	prev->next = node;
	node->next = next;
	node->prev = prev;
	next->prev = node;

	return;
}

// for stack implementation
void dlist_addto_head(struct dlist *head, struct dlist *node)
{
	_add_dlist(head, node, head->next);

	return;
}

// for queue implementation
void dlist_addto_tail(struct dlist *head, struct dlist volatile *node)
{
	_add_dlist(head->prev, node, head);

	return;
}

static void _del_dlist(struct dlist volatile *prev, struct dlist volatile *next)
{
	prev->next = next;
	next->prev = prev;
	
	return;
}

void dlist_del(struct dlist *node)
{
	_del_dlist(node->prev, node->next);

	return;
}

int dlist_empty(struct dlist *head)
{
	return (head->next == head);
}
