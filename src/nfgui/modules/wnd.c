/*
 * wnd.c
 * 	- window manager
 *	- dependencies :
 *		
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Aug 9, 2011
 *
 */

#include "wnd.h"
#include "ix_mem.h"
#include "ix_func.h"
#include "iux_afx.h"
#include "../support/event_loop.h"
#include "evt.h"
#include "ix_conf.h"


DECLARE DBG_SYSTEM

#define DBG_LEVEL		DBG_CONF
#define DBG_MODULE		"WND"


////////////////////////////////////////////////////////////
//
// private data type 
//

typedef GList WLIST;

typedef struct _WINFO_T {
	NFWINDOW 			*hwnd;
	char				title[64];

	int					dstr_level;
	int					level;
	struct _WINFO_T		*parent;
	WLIST				*child;
} WINFO_T;

typedef struct _WND_T {
	int			mst_zorder;
	int			cnt_visible_wnd;

	NFWINDOW			*dstr_req;
	int					dstr_retkey;
	NFWINDOW			*dstr_wnd;
	GtkWindow	*top_wnd;		// top_wnd is not NFWINDOW type
	WINFO_T 	*top;
} WND_T;



////////////////////////////////////////////////////////////
//
// private variable
//

static WND_T iwnd;


// just for debugging
static char tab_buf[128];
static int tab_pos = 0;



////////////////////////////////////////////////////////////
//
// private functions
//

static int _inc_tab()
{
	int i;

	++tab_pos;
	if (tab_pos > 127) tab_pos = 127;
	for (i = 0; i < tab_pos; ++i)
		tab_buf[i] = '\t';
	tab_buf[i] = 0;
	return 0;
}

static int _dec_tab()
{
	int i;

	--tab_pos;
	if (tab_pos < 0) tab_pos = 0;
	for (i = 0; i < tab_pos; ++i)
		tab_buf[i] = '\t';
	tab_buf[i] = 0;
	return 0;
}

static char *_get_tab_buf()
{
	return tab_buf;
}

static int _print_tree(WLIST *list)
{
	WLIST *plist = NULL;
	WINFO_T *winfo = NULL;
	char buf[256];

	_inc_tab();
	for (plist = list; plist; plist = g_list_next(plist)) {
		winfo = (WINFO_T *)plist->data;
		
		sprintf(buf, "%s[%p] \"%s\", [%d]\n", _get_tab_buf(), winfo->hwnd, winfo->title, winfo->level);
		printf(buf);
		if (winfo->child) _print_tree(winfo->child);
	}
	_dec_tab();
	return 0;
}

static int _emit_event(WLIST *list, GdkEventType type)
{
	WLIST *plist = NULL;
	WINFO_T *winfo = NULL;

	for (plist = list; plist; plist = g_list_next(plist)) {
		winfo = (WINFO_T *)plist->data;
		
		nfui_signal_emit((NFOBJECT*)winfo->hwnd, type, FALSE);	
		if (winfo->child) _emit_event(winfo->child, type);
	}
	return 0;
}

static int _broadcast_event(WND_T *piwnd, GdkEventType type)
{
	WINFO_T *top = piwnd->top;

	if (!top) return -1;
	_emit_event(top->child, type);
	return 0;
}

static int _display_tree(WND_T *piwnd)
{
	char buf[256];
	WINFO_T *top = piwnd->top;

	if (DBG_CONF < 2) return -1;

	if (!top) return -1;
	printf("============================================================================================\n");
	sprintf(buf, "%s[%p] \"%s\"\n", "", top->hwnd, top->title);
	printf(buf);
	_print_tree(top->child);
	printf("============================================================================================\n");
	return 0;
}

static int _init_wnd(WND_T *piwnd, GtkWindow *top_wnd)
{
	WINFO_T *tinfo;

	piwnd->mst_zorder = 1;
	piwnd->cnt_visible_wnd = 0;
	piwnd->top_wnd = top_wnd;
	
	tinfo = imalloc(sizeof(WINFO_T));
	tinfo->parent = 0;
	tinfo->hwnd = NF_TOPWND;	// because top_wnd is not NFWIDNOW type.
	piwnd->top = tinfo;
	return 0;
}

static WINFO_T *_scan_wlist_h(WLIST *list, NFWINDOW *wnd)
{
	WLIST *plist = NULL;
	WINFO_T *winfo = NULL;
	WINFO_T *ret = NULL;

	for (plist = list; plist; plist = g_list_next(plist)) {
		winfo = (WINFO_T *)plist->data;
		if (winfo->hwnd == wnd) { ret = winfo; break; }
		else {
			if (winfo->child) {
				ret = _scan_wlist_h(winfo->child, wnd);
				if (ret) break;
			}
		}
	}

	return ret;
}

static WINFO_T *_scan_wlist_t(WLIST *list, char *title)
{
	WLIST *plist = NULL;
	WINFO_T *winfo = NULL;
	WINFO_T *ret = NULL;

	for (plist = list; plist; plist = g_list_next(plist)) {
		winfo = (WINFO_T *)plist->data;
		if (strcmp(winfo->title, title) == 0) { ret = winfo; break; }
		else {
			if (winfo->child) {
				ret = _scan_wlist_t(winfo->child, title);
				if (ret) break;
			}
		}
	}

	return ret;
}
static WINFO_T *_find_winfo(WND_T *piwnd, NFWINDOW *wnd)
{
	WINFO_T *top = piwnd->top;
	if (!top) return 0;
	if (top->hwnd == wnd) return top;
	return _scan_wlist_h(top->child, wnd);
}

static WINFO_T *_search_winfo(WINFO_T *top, char *title)
{
	if (strcmp(top->title, title) == 0) return top;
	return _scan_wlist_t(top->child, title);
}

static int _is_registered(WND_T *piwnd, NFWINDOW *wnd)
{
	WINFO_T *top = piwnd->top;
	if (!top) return 1;
	if (top->hwnd == wnd) return 1;
	return _scan_wlist_h(top->child, wnd) ? 1 : 0;
}

static int _add_wnd(WND_T *piwnd, NFWINDOW *parent, NFWINDOW *wnd)
{
	WINFO_T *winfo;
	WINFO_T *prwinfo;

	prwinfo = _find_winfo(piwnd, parent);

	// some widgets don't have a parent which is NFWINDOW type
	if (!prwinfo) {
		DMSG(3, "Unknown parent. (%p, %p)\n", parent, wnd); 
		return -1; 
	}

	winfo = imalloc(sizeof(WINFO_T));
	winfo->hwnd = wnd;
	winfo->parent = prwinfo;
	winfo->level = gtk_main_level();	// safe code
	winfo->dstr_level = -1;

	prwinfo->child = g_list_append(prwinfo->child, winfo);

	DMSG(3, "ADD WND, parent = %p, wnd = %p\n", parent, wnd);

	return 0;
}

static int _delete_winfo(WINFO_T *parent, WINFO_T *child)
{
	WINFO_T *info;
	WLIST *plist;
	
	if (!parent) return -1;
	for (plist = parent->child; plist; plist = g_list_next(plist)) {
		info = (WINFO_T *)plist->data;
		if (info == child) {
			parent->child = g_list_delete_link(parent->child, plist);
			DMSG(3, "cnt = %d\n", g_list_length(parent->child));
			return 0;
		}
	}
	return -1;
}

static int _delete_wnd(WND_T *piwnd, NFWINDOW *wnd)
{
	int ret;
	WINFO_T *winfo = _find_winfo(piwnd, wnd);
	if (!winfo) return -1;

	DMSG(3, "wnd = %p, parent = %p\n", wnd, winfo->parent->hwnd);
	DMSG(3, "WND TITLE = [%s]\n", winfo->title);
	ret = _delete_winfo(winfo->parent, winfo);
	if (ret == 0) ifree(winfo);
	return 0;
}

static int _close_child(WND_T *piwnd, WINFO_T *winfo, int glv)
{
	WINFO_T *info;
	WLIST *plist;
	int level;

	plist = winfo->child;
	while (1) {
		if (!plist) break;
		DMSG(3, "plist = %p, glv = %d\n", plist, glv);
		info = (WINFO_T *)plist->data;

		if (info->child) _close_child(piwnd, info, glv);
		
		level = wnd_get_level(info->hwnd);
		DMSG(3, "LEVEL: %d, GLV:%d\n", level, glv);
		if (level == glv) {
			if (info->hwnd != piwnd->dstr_wnd) {
				nfui_signal_emit(info->hwnd, GDK_DELETE, TRUE);
			}
			plist = winfo->child;
		}
		else {
			plist = g_list_next(plist);
		}
	}

	return 0;
}

static int _broadcast_predelete(WND_T *piwnd, WINFO_T *winfo)
{
	WINFO_T *info;
	WLIST *plist;

	plist = winfo->child;
	while (1) {
		if (!plist) break;
		info = (WINFO_T *)plist->data;
		nfui_signal_emit(info->hwnd, WND_PRE_CLOSE, FALSE);
		if (_is_registered(piwnd, info->hwnd)) {
			if (info->child) _broadcast_predelete(piwnd, info);
		}
		plist = g_list_next(plist);
	}

	return 0;
}

static int _close_wnd(WND_T *piwnd, NFWINDOW *wnd)
{
	int level;
	int cnt;
	int glv;
	WINFO_T *winfo = _find_winfo(piwnd, wnd);
	if (!winfo) return -1;

	level = wnd_get_level(winfo->hwnd);
	DMSG(3, "winfo = %d, level = %d, GLOBAL LEVEL= %d\n", winfo->dstr_level, level, gtk_main_level());

	if (winfo->dstr_level == -1) {		// when first entered
		winfo->dstr_level = gtk_main_level();
		piwnd->dstr_wnd = winfo->hwnd;
		DMSG(3, "SENDING WND_PRE_CLOSE.........%p\n", winfo->hwnd);
		nfui_signal_emit(winfo->hwnd, WND_PRE_CLOSE, FALSE);

		// if all child window and itself have been already destroyed in WND_PRE_CLOSE message.
		if (!_is_registered(piwnd, wnd)) {
			evt_send_to_widget(piwnd->dstr_req, WND_CLOSED, piwnd->dstr_retkey, 0, 0);
			return 0;
		}
		_broadcast_predelete(piwnd, winfo);
	}
	else if (winfo->dstr_level < winfo->level) {
		DMSG(3, "WND CLOSING: %p, CHILD COUNT = %d\n", winfo->hwnd, g_list_length(winfo->child));
		nfui_signal_emit(winfo->hwnd, GDK_DELETE, TRUE);
		piwnd->dstr_wnd = 0;
		evt_send_to_widget(piwnd->dstr_req, WND_CLOSED, piwnd->dstr_retkey, 0, 0);
		return 0;
	}

	_close_child(piwnd, winfo, winfo->dstr_level);
	--winfo->dstr_level;		// from top level

	DMSG(3, "SENDING WND_CLOSE....%p, %p\n", piwnd->dstr_wnd, piwnd->dstr_req);
	evt_send_to_widget(piwnd->dstr_wnd, WND_CLOSE, piwnd->dstr_req, 0, 0);

	return 0;
}

static int _read_conf(WND_T *piwnd)
{
	int ret;
	ret = icf_get_value_by_int("wnd", "dmsg");
	if (ret != -1) DBG_USE(ret);
	return 0;
}


////////////////////////////////////////////////////////////
//
// public interfaces
//

int wnd_init(GtkWindow *top_wnd)
{
	_init_wnd(&iwnd, top_wnd);
	_read_conf(&iwnd);
	return 0;
}

int wnd_cleanup()
{
	_close_wnd(&iwnd, iwnd.top->hwnd);
	return 0;
}

int wnd_register(NFWINDOW *parent, NFWINDOW *wnd)
{
	if (_is_registered(&iwnd, wnd)) {
		DMSG(1, "It's already registerd.\n");
		_display_tree(&iwnd);
		return -1;
	}
	_add_wnd(&iwnd, parent, wnd);
	_display_tree(&iwnd);
	return 0;
}

int wnd_close(NFWINDOW *wnd, NFWINDOW *req, int retkey)
{
	WINFO_T *winfo = _find_winfo(&iwnd, wnd);
	if (!winfo) { 
		DMSG(1, "INVALID = %p\n", wnd); 
		return -1; 
	}
	iwnd.dstr_req = req;
	iwnd.dstr_retkey = retkey;
	_close_wnd(&iwnd, wnd);
	_display_tree(&iwnd);
	return 0;
}

int wnd_delete(NFWINDOW *wnd)
{
	_delete_wnd(&iwnd, wnd);
	_display_tree(&iwnd);
	return 0;
}

int wnd_set_level(NFWINDOW *wnd)
{
	WINFO_T *winfo = _find_winfo(&iwnd, wnd);
	if (!winfo) { 
		DMSG(1, "INVALID = %p\n", wnd); 
		return -1; 
	}
	winfo->level = gtk_main_level();	
	DMSG(3, "SET LEVEL = %p, %d\n", winfo->hwnd, winfo->level);
	_display_tree(&iwnd);
	return 0;
}

int wnd_set_title(NFWINDOW *wnd, char *title)
{
	WINFO_T *winfo = _find_winfo(&iwnd, wnd);
	if (!winfo) { 
		DMSG(1, "COUDN'T FIND THE TITLE", 0); 
		return -1; 
	}
	strcpy(winfo->title, title);
	_display_tree(&iwnd);
	DMSG(1, "WINDOW TITLE = [%s]\n", title);
	return 0;
}

NFWINDOW *wnd_find_window(const char *name)
{
	WINFO_T *winfo;
	WINFO_T *top = iwnd.top;

	if (!top) return -1;
	winfo = _search_winfo(top, name);
	if (!winfo) return 0;
	return winfo->hwnd;
}

int wnd_get_level(NFWINDOW *wnd)
{
	WINFO_T *winfo = _find_winfo(&iwnd, wnd);
	if (!winfo) return -1;
	return winfo->level;
}

int wnd_broadcast_event(GdkEventType type)
{
	DMSG(1, "");
	_broadcast_event(&iwnd, type);
	return 0;
}
