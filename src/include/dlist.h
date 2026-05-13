/*
 * dlist.h
 *
 */
 
#ifndef __DLIST_H
#define __DLIST_H

#include <stddef.h>

#define offset(type, member) (unsigned long)(&((type *)0)->member)
#define dlist_entry(ptr, type, member) \
	((type *)((char *)(ptr) - offset(type, member)))

struct dlist {
	volatile struct dlist *next;
	volatile struct dlist *prev;
};

/*
 * Initialize list
 */
void dlist_init(struct dlist *);

/*
 * add node on list
 */
void dlist_addto_head(struct dlist *, struct dlist *);	// for stack
void dlist_addto_tail(struct dlist *, struct dlist volatile *);	// for queue

/*
 * delete node on list
 */
void dlist_del(struct dlist *);

/*
 * release list
 */
void dlist_exit(struct dlist *);

int dlist_empty(struct dlist *);

#endif
