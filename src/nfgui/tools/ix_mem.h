/*
 * ix_mem.h
 * 	- memory alloc/free functions
 * 	- just for debugging
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Dec 26, 2010
 *
 */

#ifndef __IX_MEM_H
#define __IX_MEM_H

#include <stdio.h>
#include <stdlib.h>

/*
 * if you use ix_mem module, use the -D_IX_MEM_USE=1 in Makefile
 *
 */
 
#if (_IX_MEM_USE)

int init_imalloc();
int cleanup_imalloc();
void *imalloc(size_t size);
void *irealloc(void *ptr, size_t size);
void ifree(void *ptr);

#else
int init_imalloc();
int cleanup_imalloc();
	#define imalloc		g_malloc0
	#define irealloc	g_realloc
	#define ifree		g_free
#endif

#endif
