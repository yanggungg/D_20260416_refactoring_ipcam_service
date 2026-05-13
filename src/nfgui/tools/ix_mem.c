/*
 * ix_mem.c
 * 	- memory alloc/free functions
 * 	- just for debugging
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Dec 26, 2010
 *
 */


#include <glib.h>
#include "ix_mem.h"
#include <assert.h>
#include <execinfo.h>

#define DBG_LEVEL		2
#define DBG_MODULE		"IX_MEM"
#define DMSG(level, format, args...) \
	do { \
		if (DBG_LEVEL && level && DBG_LEVEL >= level) { \
			fprintf(stderr, "[IUX:"DBG_MODULE"] %s():%d: "format"\n", __FUNCTION__, __LINE__, ##args); \
		} \
	} while (0)




#if (_IX_MEM_USE)

#define IML_LOCK()		g_mutex_lock(mtx)
#define IML_UNLOCK()	g_mutex_unlock(mtx)


#define MAX_SLOT		2560

////////////////////////////////////////////////////////////
//
// private variable
//

typedef struct _MINFO_T {
	void		*ptr;
	int			size;
} MINFO_T;

static int			slot_cnt = 0;
static int 			total_cnt = 0;
static unsigned int total_size = 0;
static MINFO_T		*mif;
//static MINFO_T		mif[MAX_SLOT];
static GMutex		*mtx;


////////////////////////////////////////////////////////////
//
// private functions
//

static int _show_backtrace()
{
	void *frame_addrs[16];
	char **frame_strings;
	size_t backtrace_size;
	int i;

	backtrace_size = backtrace(frame_addrs, 16);
	frame_strings = backtrace_symbols(frame_addrs, backtrace_size);
	for (i = 0; i < backtrace_size; ++i) {
		printf("%d: [%p] %s\n", i, frame_addrs[i], frame_strings[i]);
	}
	free(frame_strings);
	return 0;
}

static int _find_empty_slot()
{
	int i;
	for (i = 0; i < slot_cnt; ++i) {
		if (mif[i].ptr == 0) break;
	}
	if (i == slot_cnt) { DMSG(1, "FULL SLOT %d !!\n", slot_cnt); g_assert(0); return -1; }

	return i;
}

static int _find_matched_slot(void *ptr)
{
	int i;
	if (ptr == 0) return -1;

	for (i = 0; i < slot_cnt; ++i) {
		if (mif[i].ptr == ptr) break;
	}
	if (i == slot_cnt) return -1;

	return i;

}

static int _release_slot(int idx)
{
	total_cnt--;
	total_size -= mif[idx].size;
	mif[idx].ptr = 0;
	mif[idx].size = 0;
	return 0;
}

static void _print_cur_state(int type, int req_size, void *p)
{
	if (total_cnt % 10 != 0) return;

	switch (type) {
	case 1:
		DMSG(2, "malloc: tot_size = %u, tot_count = %d, malloc_size = %d, [%p]\n", 
				total_size, total_cnt, req_size, p);
		break;

	case 2:
		DMSG(2, "re-alloc: tot_size = %u, tot_count = %d, ralloc_size = %d, [%p]\n", 
				total_size, total_cnt, req_size, p);
		break;

	case 0:
		DMSG(2, "free: tot_size = %u, tot_count = %d, free_size = %d, [%p]\n", 
				total_size, total_cnt, req_size, p);
		break;
	}
}

static int _check_mif()
{
	if (slot_cnt == total_cnt) {
		mif = realloc(mif, slot_cnt * 2 * sizeof(MINFO_T));
		memset(mif + slot_cnt, 0x00, sizeof(MINFO_T) * slot_cnt);
		slot_cnt *= 2;

		DMSG(1, "EXTENED MIF SLOT : %d\n", slot_cnt);
	}
	return 0;
}


////////////////////////////////////////////////////////////
//
// public interfaces
//
int init_imalloc()
{
	if (g_thread_supported() == FALSE) {
		g_thread_init(NULL);
	}
	
	slot_cnt = MAX_SLOT;
	mif = malloc(sizeof(MINFO_T) * slot_cnt);
	memset(mif, 0x00, sizeof(MINFO_T) * slot_cnt);
	
	mtx = g_mutex_new();
	return 0;
}

int cleanup_imalloc()
{
	g_mutex_free(mtx);
	return 0;
}

void *imalloc(size_t size)
{
	int idx;
	void *ret;

	IML_LOCK();
	idx = _find_empty_slot();
	ret = malloc(size);

	if (ret != NULL) {
		memset(ret, 0x00, size);
		mif[idx].ptr =  ret;
		mif[idx].size = size;
		total_size += size;
		total_cnt++;
		_print_cur_state(1, size, ret);
	}
	else {
		DMSG(1, "size = %d\n", size);
		_show_backtrace();
		g_assert(0);
	}
	IML_UNLOCK();

	_check_mif();

	return ret;
}

void *irealloc(void *ptr, size_t size)
{
	int emp_idx;
	int idx;
	void *ret;

	IML_LOCK();
	idx = _find_matched_slot(ptr);
	ret = realloc(ptr, size);
	if (ret != NULL) {
		if (idx != -1) _release_slot(idx);

		emp_idx = _find_empty_slot();
		mif[emp_idx].ptr =  ret;
		mif[emp_idx].size = size;
		total_size += size;
		total_cnt++;
		_print_cur_state(2, size, ret);
	}
	else {
		DMSG(1, "size = %d\n", size);
		_show_backtrace();
		g_assert(0);
	}

	IML_UNLOCK();

	_check_mif();

	return ret;
}

void ifree(void *ptr)
{
	int idx; 

	IML_LOCK();
	idx = _find_matched_slot(ptr);
	if (idx == -1) {
		DMSG(1, "_ERROR_ : Unknown memomry not to be free, %p\n", ptr);
		IML_UNLOCK();
		_show_backtrace();
		g_assert(0);
		return;
	}
	
	free(ptr);
	total_size -= mif[idx].size;
	total_cnt--;
	_print_cur_state(0, mif[idx].size, ptr);
	mif[idx].size = 0;
	mif[idx].ptr = 0;

	IML_UNLOCK();
}
#else

int init_imalloc()
{
	return 0;
}

int cleanup_imalloc()
{
	return 0;
}
#endif

