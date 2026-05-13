/**********************************************************************************
 *
 *  NFOBJECT
 *  
 * *******************************************************************************/


#include "nfobject.h"
#include "nfwindow.h"

#if 1
#include "nfcalendar.h"
#include "nffixed.h"
#include "nfscrolledfixed.h"
#include "nflistbox.h"
#include "nfspinbutton.h"
#include "nfcombobox.h"
#include "nftable.h"
#include "nftimespin.h"
#include "nftimelabel.h"
#include "nfbutton.h"
#include "nfcheckbutton.h"
#include "nfhscale.h"
#include "nfimage.h"
#include "nfimglabel.h"
#include "nfipeditor.h"
#include "nflabel.h"
#include "nftab.h"
#include "nftile.h"
#include "nftimeline.h"
#include "nfprogressbar.h"
#include "nfloglabel.h"
#include "nfthumbnail.h"
#include "nfdrawarea.h"
#include "nfpiechart.h"
#include "nfanalogclock.h"

// new widget
#include "cw_calendar.h"
#include "cw_hol_calendar.h"
#include "cw_mainmenu.h"
#include "cw_slider.h"
#include "cw_submenu.h"
#include "ixtimeline.h"
#include "nfvklabel.h"
#include "nfbargraph.h"

#endif

#include "../../support/event_loop.h"
#include "../../support/nf_ui_color.h"

#include "ix_mem.h"
#include "iux_afx.h"


#define DBG_LEVEL       9
#define DBG_MODULE      "NFOBJECT"
                                            
#define DBG_NFOBJECT    1
#define MAX_SLOT		(2560*6)

static NFOBJECT *g_dbgslot[MAX_SLOT];
static int g_used_slot = 0;


typedef struct {
    gchar name[128];
    gboolean dyn_data;
    gpointer data;
} USER_DATA;


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
	for (i = 0; i < MAX_SLOT; ++i) {
		if (g_dbgslot[i] == 0) break;
	}
	if (i == MAX_SLOT) { DMSG(1, "FULL SLOT %d !!\n", MAX_SLOT); g_assert(0); return -1; }

	return i;
}

static int _find_matched_slot(void *ptr)
{
	int i;
	if (ptr == 0) return -1;

	for (i = 0; i < MAX_SLOT; ++i) {
		if (g_dbgslot[i] == ptr) break;
	}
	if (i == MAX_SLOT) return -1;

	return i;
}

static int _insert_obj_slot(NFOBJECT *obj)
{
	int idx;

    if (ivsc.dfunc.support_dbg_obj == 0) return -1;

	idx = _find_empty_slot();
	g_dbgslot[idx] = obj;
    g_used_slot++;

    if (g_used_slot % 200 == 0) {
        DMSG(1, "used_slot_cnt : %d\n", g_used_slot);
    }
	
	return 0;
}

static int _delete_obj_slot(NFOBJECT *obj)
{
	int idx; 

    if (ivsc.dfunc.support_dbg_obj == 0) return -1;

	idx = _find_matched_slot(obj);
	if (idx == -1) {
		DMSG(1, "_ERROR_ : Unknown object pointer, %p\n", obj);
		_show_backtrace();
		g_assert(0);
		return -1;
	}
	
	g_dbgslot[idx] = 0;
    g_used_slot--;	

    if (g_used_slot % 200 == 0) {
        DMSG(1, "used_slot_cnt : %d\n", g_used_slot);
    }
    
    return 0;
}

static int _check_obj_slot(NFOBJECT *obj)
{
	int idx; 

    if (ivsc.dfunc.support_dbg_obj == 0) return -1;

	idx = _find_matched_slot(obj);
	if (idx == -1) {
		DMSG(1, "_ERROR_ : Unknown object pointer, %p\n", obj);
		_show_backtrace();
		g_assert(0);
		return -1;
	}

    return 0;
}

static void _signal_remove_timeout(GtkWidget *window, gpointer tmr)
{
    DMSG(1, "_NOTICE_TMR_ :  timer id - %u", GPOINTER_TO_UINT(tmr));
    g_source_remove(GPOINTER_TO_UINT(tmr));
} 

void nfui_nfobject_init(NFOBJECT* obj)
{
    obj->magic = NFOBJECT_MAGIC_NUMBER;
    obj->parent = NULL;

    obj->x = 0;
    obj->y = 0;
    obj->width = 0;
    obj->height = 0;
    obj->type = NFOBJECT_TYPE_NONE;

    obj->status = NFOBJECT_STATE_NORMAL;

    obj->bg_color[NFOBJECT_STATE_NORMAL] = -1;
    obj->bg_color[NFOBJECT_STATE_ACTIVE] = -1;
    obj->bg_color[NFOBJECT_STATE_PRELIGHT] = -1;
    obj->bg_color[NFOBJECT_STATE_DISABLE] = NFUI_DISABLED_COLOR;

    obj->show = NFOBJECT_HIDE;
    obj->use_tooltip = 0;

    obj->multi_lang = TRUE;

    obj->use_focus = NFOBJECT_FOCUS_OFF;
    obj->use_hierarchy = NFOBJECT_HIERARCHY_ON;
    
    obj->kfocus = NFOBJECT_UNFOCUS;
    obj->mfocus = NFOBJECT_UNFOCUS;
    obj->grab_kfocus = FALSE;

    _insert_obj_slot(obj);
}

void nfui_nfobject_free(NFOBJECT* obj)
{
    if(!nfui_nfobject_is_valid(obj))
    {
        DMSG(1, "########## OBJECT TYPE[%d] #######\n", obj->type);
        return;
    }
#if 0 // ??? wiggls-test
    if (obj->parent) {
        NFOBJECT *parent = obj->parent;

        if (parent->type == NFOBJECT_TYPE_TOP )
        {
            if (((NFWINDOW *)parent)->child == obj)
                ((NFWINDOW *)parent)->child = NULL;
        }
    }
#endif
    switch(obj->type)
    {
        case NFOBJECT_TYPE_TOP:             ud_free(obj); memset(obj, 0, sizeof(NFWINDOW));         ifree(obj); break;
        case NFOBJECT_TYPE_NFCALENDAR:      ud_free(obj); memset(obj, 0, sizeof(NFCALENDAR));       ifree(obj); break;
        case NFOBJECT_TYPE_NFFIXED:         ud_free(obj); memset(obj, 0, sizeof(NFFIXED));          ifree(obj); break;
		case NFOBJECT_TYPE_NFSCROLLEDFIXED:	ud_free(obj); memset(obj, 0, sizeof(NFSCROLLEDFIXED));	ifree(obj);	break;		
        case NFOBJECT_TYPE_NFLISTBOX:       ud_free(obj); memset(obj, 0, sizeof(NFLISTBOX));        ifree(obj); break;
        case NFOBJECT_TYPE_NFSPINBUTTON:    ud_free(obj); memset(obj, 0, sizeof(NFSPINBUTTON));     ifree(obj); break;  

        case NFOBJECT_TYPE_NFTABLE:         ud_free(obj); memset(obj, 0, sizeof(NFTABLE));          ifree(obj); break;  
        case NFOBJECT_TYPE_NFTIMESPIN:      ud_free(obj); memset(obj, 0, sizeof(NFTIMESPIN));       ifree(obj); break;  
        case NFOBJECT_TYPE_NFTIMELABEL:     ud_free(obj); memset(obj, 0, sizeof(NFTIMELABEL));      ifree(obj); break;  
        case NFOBJECT_TYPE_NFBUTTON:        ud_free(obj); memset(obj, 0, sizeof(NFBUTTON));         ifree(obj); break;  
        case NFOBJECT_TYPE_NFCHECKBUTTON:   ud_free(obj); memset(obj, 0, sizeof(NFCHECKBUTTON));    ifree(obj); break;  
        case NFOBJECT_TYPE_NFHSCALE:        ud_free(obj); memset(obj, 0, sizeof(NFHSCALE));         ifree(obj); break;  

        case NFOBJECT_TYPE_NFIMAGE:         ud_free(obj); memset(obj, 0, sizeof(NFIMAGE));          ifree(obj); break;  
        case NFOBJECT_TYPE_NFIMGLABEL:      ud_free(obj); memset(obj, 0, sizeof(NFIMGLABEL));       ifree(obj); break;  
        case NFOBJECT_TYPE_NFLOGLABEL:      ud_free(obj); memset(obj, 0, sizeof(NFLOGLABEL));       ifree(obj); break;  
        case NFOBJECT_TYPE_NFTHUMBNAIL:     ud_free(obj); memset(obj, 0, sizeof(NFTHUMBNAIL));      ifree(obj); break;  
        case NFOBJECT_TYPE_NFIPEDITOR:      ud_free(obj); memset(obj, 0, sizeof(NFIPEDITOR));       ifree(obj); break;  
        case NFOBJECT_TYPE_NFLABEL:         ud_free(obj); memset(obj, 0, sizeof(NFLABEL));          ifree(obj); break;  
        case NFOBJECT_TYPE_NFTAB:           ud_free(obj); memset(obj, 0, sizeof(NFTAB));            ifree(obj); break;  

        case NFOBJECT_TYPE_NFTILE:          ud_free(obj); memset(obj, 0, sizeof(NFTILE));           ifree(obj); break;  
        case NFOBJECT_TYPE_NFTIMELINE:      ud_free(obj); memset(obj, 0, sizeof(NFTIMELINE));       ifree(obj); break;  
        case NFOBJECT_TYPE_NFPROGRESSBAR:   ud_free(obj); memset(obj, 0, sizeof(NFPROGRESSBAR));    ifree(obj); break;  
        case NFOBJECT_TYPE_NFCOMBOBOX:      ud_free(obj); memset(obj, 0, sizeof(NFCOMBOBOX));       ifree(obj); break;  
        case NFOBJECT_TYPE_CWCALENDAR:      ud_free(obj); memset(obj, 0, sizeof(CWCALENDAR));       ifree(obj); break;  
        case NFOBJECT_TYPE_CWHCALENDAR:     ud_free(obj); memset(obj, 0, sizeof(CWHCALENDAR));      ifree(obj); break;  
        case NFOBJECT_TYPE_CWMAINMENU:      ud_free(obj); memset(obj, 0, sizeof(CWMAINMENU));       ifree(obj); break;  
        case NFOBJECT_TYPE_CWSUBMENU:       ud_free(obj); memset(obj, 0, sizeof(CWSUBMENU));        ifree(obj); break;  
        case NFOBJECT_TYPE_IXTIMELINE:      ud_free(obj); ifree(obj);   break;  
        case NFOBJECT_TYPE_CWSLIDER:        ud_free(obj); memset(obj, 0, sizeof(CWSLIDER));         ifree(obj); break;  

        case NFOBJECT_TYPE_DRAWAREA:        ud_free(obj); memset(obj, 0, sizeof(NFDRAWAREA));       ifree(obj); break;  
        case NFOBJECT_TYPE_NFVKLABEL:       ud_free(obj); memset(obj, 0, sizeof(NFVKLABEL));        ifree(obj); break;  
        case NFOBJECT_TYPE_NFPIECHART:      ud_free(obj); memset(obj, 0, sizeof(NFPIECHART));       ifree(obj); break;  
        case NFOBJECT_TYPE_NFANALOGCLOCK:   ud_free(obj); memset(obj, 0, sizeof(NFANALOGCLOCK));    ifree(obj); break;          
        case NFOBJECT_TYPE_NFBARGRAPH:      ud_free(obj); memset(obj, 0, sizeof(NFBARGRAPH));       ifree(obj); break;

        default:
        break;
    }

    _delete_obj_slot(obj);    
}

gboolean nfui_nfobject_is_valid(NFOBJECT *obj)
{
    if(obj->magic != NFOBJECT_MAGIC_NUMBER)
    {
        return FALSE;
    }

    if((obj->type <= NFOBJECT_TYPE_NONE) || (obj->type >= NUM_NFOBJECT_TYPES))
    {
        return FALSE;
    }

    return TRUE;
}

NFOBJECT *nfui_nfobject_new()
{
    NFOBJECT *obj;

    obj = (NFOBJECT*)imalloc(sizeof(NFOBJECT));
    obj->magic = NFOBJECT_MAGIC_NUMBER;
    obj->type = NFOBJECT_TYPE_NONE;
    obj->bg_color[NFOBJECT_STATE_NORMAL] = -1;
    obj->bg_color[NFOBJECT_STATE_PRELIGHT] = -1;
    obj->bg_color[NFOBJECT_STATE_ACTIVE] = -1;

    return obj;
}


void nfui_nfobject_destroy(NFOBJECT *obj)
{
    _check_obj_slot(obj);
    
    nfui_signal_emit(obj, GDK_DELETE, TRUE);
}

void nfui_nfobject_show(NFOBJECT *obj)
{
    _check_obj_slot(obj);

    obj->show = NFOBJECT_SHOW;

    if(obj->type == NFOBJECT_TYPE_TOP) {
        if(!GTK_WIDGET_APP_PAINTABLE(((NFWINDOW*)obj)->main_widget)) {
            gtk_widget_realize(((NFWINDOW*)obj)->main_widget);
            gdk_window_set_back_pixmap(((NFWINDOW*)obj)->main_widget->window, NULL, FALSE);
        }
        gtk_widget_show(((NFWINDOW*)obj)->main_widget);
    }
}

void nfui_nfobject_hide(NFOBJECT *obj)
{
    _check_obj_slot(obj);
    
    obj->show = NFOBJECT_HIDE;
    
#if 1
    if(obj->type == NFOBJECT_TYPE_TOP) {
        gtk_widget_hide(((NFWINDOW*)obj)->main_widget);
        ((NFWINDOW*)obj)->is_exposed = 0;
    }
#else
    if(obj->type == NFOBJECT_TYPE_TOP)
    {
        gtk_widget_hide(((NFWINDOW*)obj)->main_widget);
        nfui_clear_key_focus(obj);
    }
    else
    {
        nfui_set_key_focus(obj, FALSE);
    }
#endif
}

GdkWindow *nfui_nfobject_get_drawable(NFOBJECT *obj)
{
    NFOBJECT *parent;
    NFSCROLLEDFIXED *scrolled_fixed;

    _check_obj_slot(obj);

    parent = obj;
        
    while(parent->type != NFOBJECT_TYPE_TOP)
    {
        parent = (NFOBJECT*)(parent->parent); 
        
        if(!parent) 
            return NULL;

		if (parent->type == NFOBJECT_TYPE_NFSCROLLEDFIXED)
		{
			if (nfui_nfobject_is_scrolledfixed_usescr(obj) && nfui_nfobject_is_scrolledfixed_child(obj))
			{
			    scrolled_fixed = (NFSCROLLEDFIXED*)parent;
                return (scrolled_fixed->scrolledscr);
			}		
		}             
    }

    // skshin
    return (((NFWINDOW*)parent)->main_widget->window);
}

GdkWindow *nfui_nfobject_get_window(NFOBJECT *obj)
{
    NFOBJECT *parent;
    NFSCROLLEDFIXED *scrolled_fixed;

    _check_obj_slot(obj);

    parent = obj;

    while(parent->type != NFOBJECT_TYPE_TOP)
    {
        parent = (NFOBJECT*)(parent->parent); 
        
        if(!parent) 
            return NULL;

		if (parent->type == NFOBJECT_TYPE_NFSCROLLEDFIXED)
		{
			if (nfui_nfobject_is_scrolledfixed_usescr(obj) && nfui_nfobject_is_scrolledfixed_child(obj))
			{
			    scrolled_fixed = (NFSCROLLEDFIXED*)parent;
                return (scrolled_fixed->scrolledscr);
			}		
		}     
    }

    // skshin
    if (((NFWINDOW*)parent)->backscr_use == 1) {
        if (((NFWINDOW*)parent)->backscr)
            return (((NFWINDOW*)parent)->backscr);
        else
            return (((NFWINDOW*)parent)->main_widget->window);
        }
    else
        return (((NFWINDOW*)parent)->main_widget->window);
    }

GdkWindow *nfui_nfobject_get_scrolledscr(NFOBJECT *obj)
{
    NFOBJECT *parent;
	NFSCROLLEDFIXED *scrolled_fixed;

    parent = obj;
        
    while(parent->type != NFOBJECT_TYPE_TOP)
    {
        parent = (NFOBJECT*)(parent->parent); 
        
        if(!parent) 
            return NULL;

		if (parent->type == NFOBJECT_TYPE_NFSCROLLEDFIXED)
		{
			if (nfui_nfobject_is_scrolledfixed_usescr(obj) && nfui_nfobject_is_scrolledfixed_child(obj))
			{
			    scrolled_fixed = (NFSCROLLEDFIXED*)parent;
				return scrolled_fixed->scrolledscr;
			}		
		}
    }

    return NULL;
}

int nfui_on_backscr(NFOBJECT *obj)
{
    NFOBJECT *parent;
    parent = obj;
    int w, h;
    NFWINDOW *tp;
        
    _check_obj_slot(obj);
        
    while(parent->type != NFOBJECT_TYPE_TOP)
    {
        parent = (NFOBJECT*)(parent->parent); 
        
        if(!parent) return -1;
    }

    tp = (NFWINDOW *)parent;
    if (tp->backscr) {
        // skshin
        w = parent->width;
        h = parent->height;
        ((NFWINDOW*)parent)->backscr_use = 1;
    }
    return 0;
}

int nfui_off_backscr(NFOBJECT *obj)
{
    NFOBJECT *parent;
    NFWINDOW *tp;

    _check_obj_slot(obj);

    parent = obj;
    while(parent->type != NFOBJECT_TYPE_TOP)
    {
        parent = (NFOBJECT*)(parent->parent); 
        if(!parent) return -1;
    
    }

    tp = (NFWINDOW *)parent;
    if (tp->backscr) {
        // skshin
        ((NFWINDOW*)parent)->backscr_use = 0;
    }
    return 0;
}

int nfui_flip(NFOBJECT *object)
{
    NFOBJECT *parent;
    NFWINDOW *tp;

    _check_obj_slot(object);

    parent = object;
        
    while(parent->type != NFOBJECT_TYPE_TOP)
    {
        parent = (NFOBJECT*)(parent->parent); 
        
        if(!parent) return -1;
    }

    if (((NFWINDOW*)parent)->backscr_use == 1) {

    tp = (NFWINDOW *)parent;
    if (tp->backscr) {
        // skshin
        GdkDrawable *drawable = nfui_nfobject_get_drawable(tp);
        GdkDrawable *backscr = nfui_nfobject_get_window(tp);
        GdkGC *gc;
        gc = gdk_gc_new(drawable);

        // backscr --> drawable
        gdk_draw_drawable(drawable, gc, backscr, 0, 0, 0, 0, -1, -1);
        g_object_unref(gc);
//      g_object_unref(backscr);
//      g_object_unref(drawable);
    }
        return 0;
    }

    return -1;
}

int nfui_rflip(NFOBJECT *object)
{
    NFOBJECT *parent;
    NFWINDOW *tp;

    _check_obj_slot(object);

    parent = object;
        
    while(parent->type != NFOBJECT_TYPE_TOP)
    {
        parent = (NFOBJECT*)(parent->parent); 
        
        if(!parent) return -1;
    }

    tp = (NFWINDOW *)parent;
    if (tp->backscr) {
        // skshin
        GdkDrawable *drawable = nfui_nfobject_get_drawable(tp);
        GdkDrawable *backscr = nfui_nfobject_get_window(tp);
        GdkGC *gc;
        gc = gdk_gc_new(drawable);

        // drawable --> backscr
        gdk_draw_drawable(backscr, gc, drawable, 0, 0, 0, 0, -1, -1);
        g_object_unref(gc);
        //  g_object_unref(backscr);
        //  g_object_unref(drawable);
    }

    return 0;
}


gint nfui_nfobject_get_clip_rect(NFOBJECT *obj, GdkRectangle *rect)
{
	NFOBJECT *top = NULL;	
	
    top = nfui_nfobject_get_top(obj);
    nfui_nfobject_get_offset(top, &rect->x, &rect->y);

    rect->width = top->width;
    rect->height = top->height;        

	return 0;
}

gint nfui_nfobject_get_clip_scrolledfixed_rect(NFOBJECT *obj, GdkRectangle *rect)
{
	NFOBJECT *top = NULL;	
	NFSCROLLEDFIXED *scrolled_fixed;

	if (!nfui_nfobject_is_scrolledfixed_child(obj)) return -1;

    top = nfui_nfobject_get_scrolledfixed(obj);
    scrolled_fixed = (NFSCROLLEDFIXED*)top;

    nfui_nfobject_get_offset(top, &rect->x, &rect->y);

    rect->width = top->width - scrolled_fixed->realscr_vmargin;
    rect->height = top->height - scrolled_fixed->realscr_hmargin;

	return 0;
}

gint nfui_nfobject_get_clip_scrolledscr_rect(NFOBJECT *obj, GdkRectangle *rect)
{
	NFOBJECT *top = NULL;	
	NFSCROLLEDFIXED *scrolled_fixed;

    if (!nfui_nfobject_is_scrolledfixed_usescr(obj)) return -1;
	if (!nfui_nfobject_is_scrolledfixed_child(obj)) return -1;

    top = nfui_nfobject_get_scrolledfixed(obj);
    scrolled_fixed = (NFSCROLLEDFIXED*)top;

	nfui_nfobject_get_offset(top, &rect->x, &rect->y);

    rect->width = scrolled_fixed->scrolledscr_width;
    rect->height = scrolled_fixed->scrolledscr_height;

	return 0;
}

GdkGC* nfui_nfobject_get_gc(NFOBJECT *obj)
{
	NFWINDOW *nfwin = NULL;
	NFSCROLLEDFIXED *scrolled_fixed;
	GdkRectangle rect;
    GdkGC *gc;

    _check_obj_slot(obj);

    if (nfui_nfobject_is_scrolledfixed_usescr(obj) && nfui_nfobject_is_scrolledfixed_child(obj))
    {
        scrolled_fixed = nfui_nfobject_get_scrolledfixed(obj);

        if(scrolled_fixed && !(scrolled_fixed->gc))
            scrolled_fixed->gc = gdk_gc_new(scrolled_fixed->scrolledscr);
                
        nfui_nfobject_get_clip_scrolledscr_rect(obj, &rect);
        gdk_gc_set_clip_rectangle(scrolled_fixed->gc, &rect);

        gc = scrolled_fixed->gc;
    }
    else
    {
        nfwin = nfui_nfobject_get_top(obj);

        if(nfwin && !(nfwin->gc))
            nfwin->gc = gdk_gc_new(nfwin->main_widget->window);

        nfui_nfobject_get_clip_rect(obj, &rect);
        gdk_gc_set_clip_rectangle(nfwin->gc, &rect);	

        gc = nfwin->gc;
    }

	return gc;
}

GdkGC* nfui_nfobject_get_scrolledscr_gc(NFOBJECT *obj)
{
	NFOBJECT *top = NULL;	
	NFWINDOW *nfwin = NULL;
	NFSCROLLEDFIXED *scrolled_fixed;
	GdkRectangle rect;

    _check_obj_slot(obj);

    if (!nfui_nfobject_is_scrolledfixed_usescr(obj)) return -1;
	if (!nfui_nfobject_is_scrolledfixed_child(obj)) return -1;

    top = nfui_nfobject_get_scrolledfixed(obj);
    scrolled_fixed = (NFSCROLLEDFIXED*)top;

    if(scrolled_fixed && !(scrolled_fixed->gc))
        scrolled_fixed->gc = gdk_gc_new(scrolled_fixed->scrolledscr);
        	
	nfui_nfobject_get_clip_scrolledscr_rect(obj, &rect);
	gdk_gc_set_clip_rectangle(scrolled_fixed->gc, &rect);	
	return scrolled_fixed->gc;
}

NFOBJECT *nfui_nfobject_get_top(NFOBJECT *obj)
{   
    NFOBJECT *parent;

    _check_obj_slot(obj);

    parent = obj;
    
    while(parent->type != NFOBJECT_TYPE_TOP)
    {
        if(parent->parent == NULL)
            return NULL;

        parent = (NFOBJECT*)(parent->parent); 
    }

    return parent;
}

NFOBJECT *nfui_nfobject_get_scrolledfixed(NFOBJECT *obj)
{	
	NFOBJECT *parent = 0;
	NFSCROLLEDFIXED *scrolled_fixed;

    _check_obj_slot(obj);

	parent = obj;
	
	while(parent->type != NFOBJECT_TYPE_NFSCROLLEDFIXED)
	{
		if(parent->parent == NULL) return NULL;

		parent = (NFOBJECT*)(parent->parent);
	}

    scrolled_fixed = (NFSCROLLEDFIXED*)parent;
    if (nfui_nfscrolledfixed_is_scrollbtn(scrolled_fixed, obj) == 1) return NULL;

	return parent;
}

gint nfui_nfobject_is_scrolledfixed_usescr(NFOBJECT *obj)
{	
	NFOBJECT *parent = 0;
	NFSCROLLEDFIXED *scrolled_fixed;

    _check_obj_slot(obj);

	parent = obj;
	
	while(parent->type != NFOBJECT_TYPE_NFSCROLLEDFIXED)
	{
		if(parent->parent == NULL) return 0;

		parent = (NFOBJECT*)(parent->parent);
	}

    scrolled_fixed = (NFSCROLLEDFIXED*)parent;
    if (scrolled_fixed->use_vscroll || scrolled_fixed->use_hscroll) return 1;

    return 0;
}

gint nfui_nfobject_is_scrolledfixed_child(NFOBJECT *obj)
{
	NFOBJECT *parent = 0;
	NFSCROLLEDFIXED *scrolled_fixed;

    _check_obj_slot(obj);

	parent = obj;
	
	while(parent->type != NFOBJECT_TYPE_NFSCROLLEDFIXED)
	{
		if(parent->parent == NULL) return 0;

		parent = (NFOBJECT*)(parent->parent);
	}

    scrolled_fixed = (NFSCROLLEDFIXED*)parent;
    if (obj == scrolled_fixed) return 0;
    if (nfui_nfscrolledfixed_is_scrollbtn(scrolled_fixed, obj) == 1) return 0;

    return 1;
}

void nfui_nfobject_get_offset(NFOBJECT *obj, gint *off_x, gint *off_y)
{
    gint x, y;
    NFOBJECT *obj_temp;

    _check_obj_slot(obj);

    if(obj->type == NFOBJECT_TYPE_TOP)
    {
        *off_x = 0;
        *off_y = 0;

        return;
    }

    if(!(obj->parent))
    {
        *off_x = obj->x;
        *off_y = obj->y;

        return;
    }

    x = obj->x;
    y = obj->y;

    obj_temp = obj->parent;

    while(obj_temp->type != NFOBJECT_TYPE_TOP)
    {
        x += obj_temp->x;
        y += obj_temp->y;

        if(obj_temp->parent == NULL)
        {
            DMSG(1, "ERROR!! obj_temp has no parent!!\n");
            return;
        }
        obj_temp = obj_temp->parent;
    }

    *off_x = x;
    *off_y = y;
}

void nfui_nfobject_modify_bg(NFOBJECT* obj, nfobject_state state, int color_idx)
{
    _check_obj_slot(obj);

    if (color_idx != -1) {
        if(obj->type == NFOBJECT_TYPE_TOP)
            gtk_widget_modify_bg(((NFWINDOW*)obj)->main_widget, GTK_STATE_NORMAL, &UX_COLOR(color_idx));
    }

    obj->bg_color[state] = color_idx;
}


void nfui_nfobject_set_size(NFOBJECT* obj, gint width, gint height)
{
    _check_obj_slot(obj);

    obj->width = width;
    obj->height = height;

    if(obj->type == NFOBJECT_TYPE_TOP)      //for tooltip
    {
        NFWINDOW *win;
        win = (NFWINDOW*)obj;

        win->child->width = width;
        win->child->height = height;

        gtk_window_resize(win->main_widget, width, height);
    }
    
}

void nfui_nfobject_get_size(NFOBJECT* obj, gint *width, gint *height)
{
    _check_obj_slot(obj);

    *width = obj->width;
    *height = obj->height;
}

gint nfui_nfobject_get_bg_color(NFOBJECT* obj, nfobject_state state)
{
    NFOBJECT *tmp = obj;

    _check_obj_slot(obj);

    while(tmp->bg_color[state] < 0)
    {
        if(tmp->parent)
            tmp = tmp->parent;
        else
            break;
    }

    return (tmp->bg_color[state]);
}

void ud_free(NFOBJECT *obj)
{
    gint len = -1;
    USER_DATA *temp;
    int i;

    _check_obj_slot(obj);
    
    len = g_slist_length(obj->user_data);
    for(i=0; i<len; i++)
    {
        temp = g_slist_nth_data(obj->user_data, i);
        if (temp->dyn_data && temp->data) ifree(temp->data);
        ifree(temp);
    }
    g_slist_free(obj->user_data);
    obj->user_data = 0;
}

void nfui_nfobject_set_data(NFOBJECT *obj, const gchar *name, gpointer data)
{
    USER_DATA *ud;
    USER_DATA *temp;
    gint len = -1;
    guint i;

    _check_obj_slot(obj);

    len = g_slist_length(obj->user_data);

    for(i=0; i<len; i++)
    {
        temp = g_slist_nth_data(obj->user_data, i);

        if(!g_ascii_strcasecmp(name, temp->name))
        {
            if (temp->dyn_data && temp->data) ifree(temp->data);
            temp->dyn_data = 0;
            temp->data = data;
            return;
        }
    }

    ud = (USER_DATA*)imalloc(sizeof(USER_DATA));
    g_snprintf(ud->name, sizeof(ud->name)-1, "%s", name);
    ud->dyn_data = 0;
    ud->data = data;

    obj->user_data = g_slist_append(obj->user_data, ud);
}

void nfui_nfobject_set_alloc_data(NFOBJECT *obj, const gchar *name, gpointer data)
{
    USER_DATA *ud;
    USER_DATA *temp;
    gint len = -1;
    guint i;

    _check_obj_slot(obj);

    len = g_slist_length(obj->user_data);

    for(i=0; i<len; i++)
    {
        temp = g_slist_nth_data(obj->user_data, i);

        if(!g_ascii_strcasecmp(name, temp->name))
        {
            if (temp->dyn_data && temp->data) ifree(temp->data);
            temp->dyn_data = 1;            
            temp->data = data;
            return;
        }
    }

    ud = (USER_DATA*)imalloc(sizeof(USER_DATA));
    g_snprintf(ud->name, sizeof(ud->name)-1, "%s", name);
    ud->dyn_data = 1;
    ud->data = data;
    
    obj->user_data = g_slist_append(obj->user_data, ud);
}

gpointer nfui_nfobject_get_data(NFOBJECT *obj, const gchar *name)
{
    guint i, num;
    USER_DATA *ud;
    gint len = 0;
    
    _check_obj_slot(obj);
    
    if(!nfui_nfobject_is_valid(obj))
    {
        g_warning("%s, %d, obj:%p, name:%s", __FUNCTION__, __LINE__, obj, name);
        g_warning("%s, %d, not valid object", __FUNCTION__, __LINE__);
        return NULL;
    }

    len = g_slist_length(obj->user_data);
    
    for(i=0; i<len; i++)
    {
        ud = (USER_DATA*)g_slist_nth_data(obj->user_data, i);

        if(!g_ascii_strcasecmp(ud->name, name))
            return ud->data;
    }

    return NULL;
}

void nfui_nfobject_move(NFOBJECT *obj, gint x, gint y)
{
    obj->x = x;
    obj->y = y;

    _check_obj_slot(obj);

    if(obj->type == NFOBJECT_TYPE_TOP)
    {
        NFWINDOW *win;
        win = (NFWINDOW*)obj;

        gtk_window_move(win->main_widget, x, y);
    }
}

void nfui_nfobject_get_window_pos(NFOBJECT *obj, gint *x, gint *y)
{
    GdkWindow *wnd;

    _check_obj_slot(obj);

    wnd = nfui_nfobject_get_window(obj);

    gdk_window_get_origin(wnd, x, y);
}

gboolean nfui_nfobject_is_shown(NFOBJECT* obj)
{
    NFOBJECT* temp;

    _check_obj_slot(obj);

    temp = obj;

    while(1)
    {
        if(temp==NULL)
            return FALSE;

        if(temp->show == NFOBJECT_HIDE)
            return FALSE;

        if(temp->type == NFOBJECT_TYPE_TOP)
            break;

        temp = temp->parent;
    }

    return TRUE;
}

gboolean nfui_nfobject_is_shown_except(NFOBJECT* obj, gchar *wndtitle)
{
    NFWINDOW* top_wnd = NULL;
    NFOBJECT* temp;

    _check_obj_slot(obj);

    temp = obj;

    while(1)
    {
        if(temp==NULL)
            return FALSE;

        if(temp->type == NFOBJECT_TYPE_TOP)
        {
            top_wnd = (NFWINDOW*)temp;
            if (strcmp(top_wnd->strTitle, wndtitle) == 0) break;
        }

        if(temp->show == NFOBJECT_HIDE)
            return FALSE;

        if(temp->type == NFOBJECT_TYPE_TOP)
            break;

        temp = temp->parent;
    }

    return TRUE;
}

void nfui_nfobject_set_state(NFOBJECT *obj, nfobject_state state)
{
    _check_obj_slot(obj);

    obj->status = state;
}

nfobject_state nfui_nfobject_get_state(NFOBJECT *obj)
{
    _check_obj_slot(obj);

    return obj->status;
}

void nfui_nfobject_use_focus(NFOBJECT *obj, guint use)
{
    _check_obj_slot(obj);

    obj->use_focus = use;
}

void nfui_nfobject_use_hierarchy(NFOBJECT *obj, guint use)
{
    obj->use_hierarchy = use;
}

void nfui_nfobject_set_kfocus(NFOBJECT *obj, guint ft)
{
    _check_obj_slot(obj);

    obj->kfocus = ft;
}

void nfui_nfobject_enable(NFOBJECT *obj)
{
    NFOBJECT *child;
    guint child_num;
    gint i;

    _check_obj_slot(obj);

    obj->status = NFOBJECT_STATE_NORMAL;

    switch(obj->type) {
        case NFOBJECT_TYPE_NFLISTBOX :
        case NFOBJECT_TYPE_NFCOMBOBOX :
        case NFOBJECT_TYPE_NFSPINBUTTON :
            {
                child_num = g_slist_length(((NFCONTAINER*)obj)->children);

                for(i=0; i<child_num; i++) {
                    child = g_slist_nth_data(((NFCONTAINER*)obj)->children, i);
                    child->status = NFOBJECT_STATE_NORMAL;
                }
            }
            break;

        default:
            break;
    }
}

void nfui_nfobject_disable(NFOBJECT *obj)
{
    NFOBJECT *child;
    guint child_num;
    gint i;

    _check_obj_slot(obj);

    obj->status = NFOBJECT_STATE_DISABLE;

    switch(obj->type) {
        case NFOBJECT_TYPE_NFLISTBOX :
        case NFOBJECT_TYPE_NFCOMBOBOX :
        case NFOBJECT_TYPE_NFSPINBUTTON :
            {
                child_num = g_slist_length(((NFCONTAINER*)obj)->children);

                for(i=0; i<child_num; i++) {
                    child = g_slist_nth_data(((NFCONTAINER*)obj)->children, i);
                    child->status = NFOBJECT_STATE_DISABLE;
                }
            }
            break;

        default:
            break;
    }
}

gboolean nfui_nfobject_is_disabled(NFOBJECT* obj)
{
#if 0
    if(obj->status == NFOBJECT_STATE_DISABLE)
        return TRUE;
    else
        return FALSE;
#endif
    NFOBJECT* temp;

    _check_obj_slot(obj);

    temp = obj;

    while(1)
    {
        if(temp==NULL)
            return FALSE;

        if(temp->status == NFOBJECT_STATE_DISABLE)
            return TRUE;

        if(temp->type == NFOBJECT_TYPE_TOP)
            break;

        temp = temp->parent;
    }

    return FALSE;
}

void nfui_nfobject_use_tooltip(NFOBJECT *obj, gboolean use)
{
    _check_obj_slot(obj);

    if(use)     obj->use_tooltip = 1;
    else        obj->use_tooltip = 0;
}

gint nfui_nfobject_is_used_tooltip(NFOBJECT *obj)
{
    _check_obj_slot(obj);

    return obj->use_tooltip;
}

void nfui_nfobject_set_tooltip(NFOBJECT *obj, const gchar *tooltip_string)
{
    gint length;

    _check_obj_slot(obj);

    memset(obj->strTooltip, 0, sizeof(obj->strTooltip));
    
    length = strlen(tooltip_string);

    if(length > 0)
        strncpy(obj->strTooltip, tooltip_string, sizeof(gchar)*length);

    obj->use_tooltip = 1;
}

gchar* nfui_nfobject_get_tooltip(NFOBJECT *obj)
{
    _check_obj_slot(obj);

    return obj->strTooltip;
}

void nfui_nfobject_set_tooltip_info(NFOBJECT *obj, gpointer func)
{
    _check_obj_slot(obj);

    obj->tooltip_info = func;
}

void nfui_nfobject_support_multi_lang(NFOBJECT *obj, gboolean support)
{
    _check_obj_slot(obj);

    obj->multi_lang = support;
}

gboolean nfui_nfobject_is_supported_multi_lang(NFOBJECT *obj)
{
    _check_obj_slot(obj);

    return obj->multi_lang;
}

guint nfui_nfobject_timeout_add(NFOBJECT *obj, guint interval, GSourceFunc func, gpointer data)
{
    NFWINDOW *win;
    guint tmr_id;

    if (!obj) return 0;

    win = nfui_nfobject_get_top(obj);
    if (!win) return 0;

    tmr_id = g_timeout_add(interval, func, data);
    g_signal_connect(win->main_widget, "destroy", G_CALLBACK(_signal_remove_timeout), GUINT_TO_POINTER(tmr_id));
    DMSG(1, "_NOTICE_TMR_ :  timer id - %u", tmr_id);
    return tmr_id;
}

gint nfui_nfscrolledscr_draw_drawable(NFOBJECT *obj)
{
	NFOBJECT *top_win;
	NFSCROLLEDFIXED *scrolled_fixed;
	GdkDrawable *drawable;
	GdkGC *gc;

    GdkRectangle rect;

    gint scrolled_fixed_x, scrolled_fixed_y;
    gint off_x, off_y;

    _check_obj_slot(obj);

    if (!obj) return -1;
    if (!nfui_nfobject_is_scrolledfixed_usescr(obj)) return -1;
    if (!nfui_nfobject_is_scrolledfixed_child(obj)) return -1;

	top_win = nfui_nfobject_get_top(obj);
    if (!top_win) return -1;

	drawable = nfui_nfobject_get_window(top_win);
    if (!drawable) return -1;

    scrolled_fixed = nfui_nfobject_get_scrolledfixed(obj);
    if (!scrolled_fixed) return -1;

//    g_message("%s, %d, obj_type:%d, width:%d, height:%d", __FUNCTION__, __LINE__, obj->type, obj->width, obj->height);

    nfui_nfobject_get_offset(scrolled_fixed, &scrolled_fixed_x, &scrolled_fixed_y);
	nfui_nfobject_get_offset(obj, &off_x, &off_y);

    gc = nfui_nfobject_get_gc(obj);
    nfui_nfobject_get_clip_scrolledfixed_rect(obj, &rect);
    gdk_gc_set_clip_rectangle(gc, &rect);	

	gdk_draw_drawable(drawable, gc, scrolled_fixed->scrolledscr, off_x, off_y,
                        off_x-(scrolled_fixed->relative_x-scrolled_fixed_x), off_y-(scrolled_fixed->relative_y-scrolled_fixed_y), obj->width, obj->height);

    if ((scrolled_fixed->hscroll_offset != 0) || (scrolled_fixed->vscroll_offset != 0))
    {
        gdk_draw_drawable(drawable, gc, scrolled_fixed->scrolledscr, scrolled_fixed_x, scrolled_fixed_y, 
                            scrolled_fixed_x, scrolled_fixed_y, ((NFOBJECT*)scrolled_fixed)->width, scrolled_fixed->vscroll_offset);

        gdk_draw_drawable(drawable, gc, scrolled_fixed->scrolledscr, scrolled_fixed_x, scrolled_fixed_y, 
                            scrolled_fixed_x, scrolled_fixed_y, ((NFOBJECT*)scrolled_fixed)->width, scrolled_fixed->vscroll_offset);
    }

    return 0;
}


gint nfui_nfscrolledscr_capture(NFOBJECT *obj)
{
	NFSCROLLEDFIXED *scrolled_fixed;
    GdkPixbuf *nbuf;
    GdkColormap *cmap;

    scrolled_fixed = nfui_nfobject_get_scrolledfixed(obj);
    if (!scrolled_fixed) return -1;

    cmap = gdk_window_get_colormap(scrolled_fixed->scrolledscr);
    nbuf = gdk_pixbuf_get_from_drawable(NULL, scrolled_fixed->scrolledscr, cmap, 0, 0, 0, 0, scrolled_fixed->scrolledscr_width, scrolled_fixed->scrolledscr_height);

    if (nbuf) {
        if(gdk_pixbuf_save(nbuf, "/NFDVR/scroll_fixed.jpg", "jpeg", NULL, "quality", "100", NULL) == FALSE)
            g_message("gdk_pixbuf_save FAIL");

        g_object_unref(nbuf);                
    }

    return 0;
}
