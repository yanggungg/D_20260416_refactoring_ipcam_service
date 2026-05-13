
#include "nf_afx.h"
#include "iux_afx.h"
#include "../viewers/objects/nfobject.h"
#include "event_loop.h"

#include "nf_ui_page_manager.h"
//#include "../nf_ui_livelog.h"
#include "../tools/nf_ui_function.h"
#if defined(__XRPLUS_UI__)
//#include "../nf_ui_popup_menu_xrplus_combo.h"
#else
//#include "../nf_ui_popup_menu.h"
#endif


#define DBG_LEVEL		0
#define DBG_MODULE		"PAGEMAN"


#define	MAX_OPEN_PAGES	16
#define	MAX_RUN_FUNCTIONS	8

enum {
	GC_NORMAL = 0,
	GC_BG_BURN,
	GC_NO_DISK,
	GC_PANIC,
	NUM_GLOBAL_CONDITIONS,
};


typedef	struct _FUNC_STACK_DATA_T	FUNC_STACK_DATA;
typedef	struct _PAGE_STACK_DATA_T	PAGE_STACK_DATA;

struct _FUNC_STACK_DATA_T {
	FUNCID	fid;
	NFOBJECT* fobj;
};

static FUNC_STACK_DATA G_FSD[MAX_RUN_FUNCTIONS];
static gint g_func_top = 0;


struct _PAGE_STACK_DATA_T {
	PAGEID		pid;
	NFOBJECT*	pobj;
	gchar		user_name[64];
};

static PAGE_STACK_DATA G_PSD[MAX_OPEN_PAGES];
static gint g_stack_top = 0;

#ifdef USE_NEW_MODE_CONTROL
////////////////////////////////////////
//	test		        			////
static int prvLoadScenarioXML();	////	
static void prvTestCode();			////
////////////////////////////////////////
#endif	//USE_NEW_MODE_CONTROL


static void prvPrintPageStack()
{
	gint i;

	for(i=MAX_OPEN_PAGES-1; i>=0; i--)
	{
		if(G_PSD[i].user_name != NULL)
			DMSG(1, "STACK[%d] : PGID[%d], OBJECT[%p], USER[%s]\n", i, G_PSD[i].pid, G_PSD[i].pobj, G_PSD[i].user_name);
		else
			DMSG(1, "STACK[%d] : PGID[%d], OBJECT[%p], USER[]\n", i, G_PSD[i].pid, G_PSD[i].pobj);
	}
}

void nfui_page_manager_init()
{
	guint i;

	for(i=0; i<MAX_OPEN_PAGES; i++)
	{
		G_PSD[i].pid = PGID_NONE;
		G_PSD[i].pobj = NULL;
		memset(&(G_PSD[i].user_name), 0, sizeof(gchar)*64);
	}
	g_stack_top = 0;


	for(i=0; i<MAX_RUN_FUNCTIONS; i++)
	{
		G_FSD[i].fid = FUNCID_NONE;
		G_FSD[i].fobj = NULL;
	}
	g_func_top = 0;


//	prvLoadScenarioXML();
}

void nfui_func_start(FUNCID fid, NFOBJECT* obj)
{
	if(g_func_top >= MAX_RUN_FUNCTIONS)
	{
		g_warning("FUNC STACK is full!!\n");
		return;
	}

	G_FSD[g_func_top].fid = fid;
	G_FSD[g_func_top].fobj = obj;

	g_func_top++;
}

void nfui_func_end(FUNCID fid, NFOBJECT *obj)
{
	gint i, j;

	if(g_func_top <= 0)
	{
		g_warning("FUNC STACK is empty!!\n");
		return;
	}

	for(i=0; i<g_func_top; i++)
	{
		if(G_FSD[i].fid == fid && G_FSD[i].fobj == obj)
			break;
	}

	if(i == g_func_top)
	{
		g_warning("Func stack has no data same with [%d : %p]\n", fid, obj);
		return;
	}

	for(j=i; j<g_func_top-1; j++)
		G_FSD[j] = G_FSD[j+1];

	// SKSHIN
//	nfui_idle_timer_reset();

	g_func_top--;

	G_FSD[g_func_top].fid = FUNCID_NONE;
	G_FSD[g_func_top].fobj = NULL; 
}

gint nfui_get_func_count()
{
	return g_func_top;
}

FUNCID nfui_func_get_fid()
{
    if(g_func_top <= 0)
    {
    	g_warning("FUNC STACK is empty!!\n");
    	return FUNCID_NONE;
    }

   return G_FSD[g_func_top].fid;
    
}



void nfui_page_open(PAGEID pid, NFOBJECT* pobj, const gchar* user)
{
	gint i;

	if(g_stack_top >= MAX_OPEN_PAGES)
	{
		g_warning("PAGE STACK is full!!\n");
		prvPrintPageStack();

		return;
	}

// skshin
#if 0
	if(g_stack_top == DEFAULT_OPEN_PAGES) 
	{
		LiveLog_Hide_Auto();
	}
#endif

	G_PSD[g_stack_top].pid = pid;
	G_PSD[g_stack_top].pobj = pobj;
	if(user != NULL)
		g_sprintf(G_PSD[g_stack_top].user_name, "%s", user);

	DMSG(1, "TO ADD : PGID[%d], OBJ[%p], User[%s]\n", G_PSD[g_stack_top].pid, G_PSD[g_stack_top].pobj, G_PSD[g_stack_top].user_name);
	g_stack_top++;

	DMSG(1, "STACK DATA ADDED... count[%d]\n", g_stack_top);

	i = g_stack_top - 1;
	while(!nffunc_change_led(G_PSD[i].pid))
	{
		if(i==0)	break;
		i--;
	}
}

void nfui_page_close(PAGEID pid, NFOBJECT* pobj)
{
	gint i;

	for(i=g_stack_top-1; i>=0; i--) {
		if(G_PSD[i].pid == pid) {
			if(pobj == NULL || G_PSD[i].pobj == pobj)
				break;
		}
	}

	if(i<0)	return;


	if(i == g_stack_top-1)
	{
		G_PSD[i].pid = PGID_NONE;
		G_PSD[i].pobj = NULL;
	}
	else
	{
		gint k;

		for(k=i; k<g_stack_top-1; k++)
		{
			DMSG(1, "i[%d], k[%d], g_stack_top[%d]\n", i, k, g_stack_top);
			if(k+1 == MAX_OPEN_PAGES)
			{
				G_PSD[k].pid = PGID_NONE;
				G_PSD[k].pobj = NULL;
			}
			else
			{
				G_PSD[k].pid = G_PSD[k+1].pid;
				G_PSD[k].pobj = G_PSD[k+1].pobj;
			}
		}
	}

	g_stack_top--;

#if 0
	gdk_window_invalidate_rect(GdkWindow *window, GdkRectangle *rect, gboolean invalidate_children);

	if(g_stack_top > 0)
		nfui_signal_emit(G_PSD[g_stack_top-1].pobj, GDK_EXPOSE, TRUE);
#endif

	DMSG(1, "STACK DATA REMOVED... count[%d]\n", g_stack_top);
	
	if(g_stack_top <= 0)
	{
		g_warning("PAGE STACK is empty!!\n");

		return;
	}

// skshin
#if 0
	if(g_stack_top == DEFAULT_OPEN_PAGES)
		LiveLog_Show_Auto();

#endif

	i = g_stack_top - 1;
	while(!nffunc_change_led(G_PSD[i].pid))
	{
		if(i==0)	break;
		i--;
	}
}

PAGEID nfui_get_cur_page(NFOBJECT **cur_page)
{
	if(g_stack_top == 0)
	{
		*cur_page = NULL;
		return PGID_NONE;
	}

	if(cur_page)
		*cur_page = G_PSD[g_stack_top-1].pobj;

	return G_PSD[g_stack_top-1].pid;
}

PAGEID nfui_get_pid_by_object(NFOBJECT *object)
{
	gint i;

	for(i=g_stack_top-1; i>=0; i--)
	{
		if(G_PSD[i].pobj == object)
			break;
	}

	if(i<0)	return PGID_NONE;

	return G_PSD[i].pid;
}

NFOBJECT* nfui_get_object_by_pid(PAGEID pid)
{
	gint i;

	for(i=g_stack_top-1; i>=0; i--)
	{
		if(G_PSD[i].pid == pid)
			break;
	}

	if(i<0)	return NULL;

	return G_PSD[i].pobj;
}

guint nfui_get_open_page_count()
{
	return g_stack_top;
}

NFOBJECT* nfui_get_nth_page(gint i)
{
	if(i>=g_stack_top || i<0)
		return NULL;

	return G_PSD[i].pobj;
}

PAGEID nfui_get_nth_pgid(gint i)
{	
	if(i>=g_stack_top || i<0)
		return PGID_NONE;

	return G_PSD[i].pid;

}


gchar* nfui_get_page_user(PAGEID pid, NFOBJECT *obj)
{
	gint i;

	for(i=g_stack_top-1; i>=0; i--)
	{
		if(G_PSD[i].pid == pid)
		{
			if(G_PSD[i].pobj == obj)
				break;
		}
	}

	if(i<0)	return NULL;

	return G_PSD[i].user_name;
}

gchar *nfui_get_last_user()
{
	gint i;

	for(i=g_stack_top-1; i>=0; i--)
	{
		if(strlen(G_PSD[i].user_name))
			return G_PSD[i].user_name;
	}

	return NULL;
}

void nfui_set_user_def_pages(const gchar *user_name)
{
	gint i;

	if(g_stack_top < DEFAULT_OPEN_PAGES)
		return;

	if(strlen(user_name) <= 0)
		return;
	
	for(i=0; i<DEFAULT_OPEN_PAGES; i++)
	{
		memset(G_PSD[i].user_name, 0, sizeof(G_PSD[i].user_name));
		g_stpcpy(G_PSD[i].user_name, user_name);
	}
}

int wnd_bring_to_front_by_id(PAGEID pid)
{
	int i;
	PAGE_STACK_DATA tmp;

	for (i = g_stack_top - 1; i >= 0; --i)
		if (G_PSD[i].pid == pid) break;

	if (i < 0) return -1;
	tmp = G_PSD[i];

	for (++i ; i < g_stack_top - 1; ++i)
		G_PSD[i - 1] = G_PSD[i];
	
	G_PSD[i] = tmp;	

	return 0;
}

int wnd_bring_to_front(NFOBJECT *obj)
{
	int i;
	PAGE_STACK_DATA tmp;
	if (obj == NULL) return -1;

	for (i = g_stack_top - 1; i >= 0; --i) 
		if(G_PSD[i].pobj == obj) break;

	if (i < 0) return -1;
	tmp = G_PSD[i];

	for (++i ; i < g_stack_top - 1; ++i)
		G_PSD[i - 1] = G_PSD[i];
	
	G_PSD[i] = tmp;	

	return 0;
}

int wnd_send_to_back(NFOBJECT *obj)
{
	int i;
	PAGE_STACK_DATA tmp;
	if (obj == NULL) return -1;

	for (i = g_stack_top - 1; i >= 0; --i) 
		if(G_PSD[i].pobj == obj) break;

	if (i < 0) return -1;
	tmp = G_PSD[i];

	for (i = g_stack_top - 1; i > 0; --i)
		G_PSD[i] = G_PSD[i - 1];
	
	G_PSD[i] = tmp;	

	return 0;
}

