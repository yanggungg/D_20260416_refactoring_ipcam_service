
#ifndef _LIST_H_
#define _LIST_H_

#include <stddef.h>

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * These are non-NULL pointers that will result in page faults
 * under normal circumstances, used to verify that nobody uses
 * non-initialized list entries.
 */
#define LIST_POISON1  ((struct list_head *) 0x00100100)
#define LIST_POISON2  ((struct list_head *) 0x00200200)

/*
 * Simple doubly linked list implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */

struct list_head {
	struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)

#define INIT_LIST_HEAD(ptr) do { \
	(ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

static __inline void list_init(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static __inline void __list_add(struct list_head *newe,
			      struct list_head *prev, struct list_head *next)
{
	next->prev = newe;
	newe->next = next;
	newe->prev = prev;
	prev->next = newe;
}

/**
 * list_add - add a new entry
 * @newe: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static __inline void list_add(struct list_head *newe, struct list_head *head)
{
	__list_add(newe, head, head->next);
}

/**
 * list_add_head - add a new entry
 * @newe: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static __inline void list_add_head(struct list_head *newe, struct list_head *head)
{
	__list_add(newe, head, head->next);
}

/**
 * list_add_tail - add a new entry
 * @newe: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static __inline void list_add_tail(struct list_head *newe, struct list_head *head)
{
	__list_add(newe, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static __inline void __list_del(struct list_head * prev, struct list_head * next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty on entry does not return true after this, the entry is
 * in an undefined state.
 */
static __inline void list_del(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
	entry->next = LIST_POISON1;
	entry->prev = LIST_POISON2;
}

/**
 * list_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
static __inline void list_del_init(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
	INIT_LIST_HEAD(entry);
}

/**
 * list_move_head - delete from one list and add as another's head
 * @list: the entry to move
 * @head: the head that will precede our entry
 */
static __inline void list_move_head(struct list_head *list, struct list_head *head)
{
	__list_del(list->prev, list->next);
	list_add_head(list, head);
}

/**
 * list_move_tail - delete from one list and add as another's tail
 * @list: the entry to move
 * @head: the head that will follow our entry
 */
static __inline void list_move_tail(struct list_head *list, struct list_head *head)
{
	__list_del(list->prev, list->next);
	list_add_tail(list, head);
}

/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static __inline int list_empty(const struct list_head *head)
{
	return head->next == head;
}

static __inline struct list_head *list_head(struct list_head *list)
{
	return list->next;
}

static __inline struct list_head *list_tail(struct list_head *list)
{
	return list->prev;
}

static __inline void __list_splice(struct list_head *list,
				 struct list_head *head)
{
	struct list_head *first = list->next;
	struct list_head *last = list->prev;
	struct list_head *at = head->next;

	first->prev = head;
	head->next = first;

	last->next = at;
	at->prev = last;
}

/**
 * list_splice_init - join two lists and reinitialise the emptied list.
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * The list at @list is reinitialised
 */
static __inline void list_splice_init(struct list_head *list,
				    struct list_head *head)
{
	if (!list_empty(list)) {
		__list_splice(list, head);
		INIT_LIST_HEAD(list);
	}
}

/**
 * list_splice - join two lists
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static __inline void list_splice(struct list_head *list, struct list_head *head)
{
	if (!list_empty(list))
		__list_splice(list, head);
}

#ifndef	container_of
#define	container_of(ptr, type, member)	\
	((type *)((char *)(ptr) - offsetof(type, member)))
#endif	/* container_of */

/**
 * list_entry - get the struct for this entry
 * @ptr:	the &struct list_head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 */
#define list_entry(ptr, type, member) container_of(ptr, type, member)

/**
 * list_for_each	-	iterate over a list
 * @pos:	the &struct list_head to use as a loop counter.
 * @head:	the head for your list.
 */
#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * list_for_each_prev	-	iterate over a list backwards
 * @pos:	the &struct list_head to use as a loop counter.
 * @head:	the head for your list.
 */
#define list_for_each_prev(pos, head) \
	for (pos = (head)->prev; pos != (head); pos = pos->prev)

/**
 * list_for_each_safe	-	iterate over a list safe against removal of list entry
 * @pos:	the &struct list_head to use as a loop counter.
 * @n:		another &struct list_head to use as temporary storage.
 * @head:	the head for your list.
 */
#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

/**
 * list_for_each_entry	-	iterate over list of given type
 * @pos:	the type * to use as a loop counter.
 * @head:	the head for your list.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 */
#define list_for_each_entry(pos, head, type, member)			\
	for (pos = list_entry((head)->next, type, member);			\
		&pos->member != (head); 								\
		pos = list_entry(pos->member.next, type, member))

/**
 * list_for_each_entry_safe	-	iterate over list of given type safe against
 * removal of list entry
 * @pos:	the type * to use as a loop counter.
 * @n:		another type * to use as temporary storage.
 * @head:	the head for your list.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 */
#define list_for_each_entry_safe(pos, n, head, type, member)	\
	for (pos = list_entry((head)->next, type, member),			\
		n = list_entry(pos->member.next, type, member);		\
		&pos->member != (head); 								\
		pos = n, n = list_entry(pos->member.next, type, member))

/**
 * list_for_each_entry_reverse - iterate backwards over list of given type.
 * @pos:	the type * to use as a loop counter.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define list_for_each_entry_reverse(pos, head, type, member)	\
	for (pos = list_entry((head)->prev, type, member);			\
		&pos->member != (head); 								\
		pos = list_entry(pos->member.prev, type, member))


static __inline int list_size(struct list_head *head)
{
	int count = 0;
	struct list_head *l;

	list_for_each(l, head)
		count++;
	return count;
}

/*
 * more(a, b) function returns 1 if (a) > (b)
 */
static __inline void list_add_ordered(struct list_head *newe,
		struct list_head *head,
		int (*more)(struct list_head *, struct list_head *))
{
	struct list_head *l;

	list_for_each(l, head)
		if ( more(newe, l) )
			break;
	__list_add(newe, l->prev, l);
}

#ifdef	__cplusplus
}
#endif
#endif	/* _LIST_H_ */

