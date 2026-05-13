
#include "nf_afx.h"
#include "nf_ui_color.h"

#include "event_loop.h"
#include "nf_ui_page_manager.h"
#include "nf_ui_common_data.h"

#if defined(_ANF_0824CL)
#include "../support/scaler_command.h"
#endif

#include "../viewers/objects/nfwindow.h"
#include "../viewers/objects/nffixed.h"
#include "../viewers/objects/nfbutton.h"
#include "../viewers/objects/nftab.h"
#include "../viewers/objects/nfcheckbutton.h"
#include "../viewers/objects/nfscrolledfixed.h"
#include "../viewers/vw_system_debugging_information.h"
#include "../viewers/vw_sys_camera_ipcam_install_search_ver2.h"

#include "../tools/nf_ui_function.h"
#include "../tools/nf_ui_tool.h"

#include "../modules/cheat.h"

#include "nf_util_device.h"
#include "nf_keyctrl.h"
#include "nf_action.h"
#include "nf_api_param_hw.h"

#ifdef _SUPPORT_CAPTURE_FB
#include "jbshell.h"
#endif  /**/

#include "cmm.h"
#include "uxm.h"
#include "ix_mem.h"
#include "iux_afx.h"
#include "ix_func.h"
#include "ix_conf.h"
#include "ssm.h"


DECLARE DBG_SYSTEM

#define DBG_LEVEL       DBG_CONF
#define DBG_MODULE      "EVTLOOP"


#define KEY_REPEAT_INTERVAL     (300)


#define _user_oper(event) \
        (event->type == GDK_SCROLL ||\
            event->type == GDK_BUTTON_PRESS ||\
            event->type == GDK_2BUTTON_PRESS ||\
            event->type == GDK_BUTTON_RELEASE ||\
            event->type == GDK_MOTION_NOTIFY ||\
            event->type == GDK_LEAVE_NOTIFY ||\
            event->type == GDK_ENTER_NOTIFY ||\
            event->type == GDK_FOCUS_CHANGE ||\
            event->type == NFEVENT_JOG_CHANGE ||\
            event->type == NFEVENT_SHUTTLE_CHANGE ||\
            event->type == NFEVENT_KEYPAD_PRESS ||\
            event->type == NFEVENT_KEYPAD_RELEASE ||\
            event->type == NFEVENT_REMOCON_PRESS ||\
            event->type == NFEVENT_REMOCON_RELEASE)

#define RUN_EVENT_HANDLER(a, b, c)      {   \
if (_ui_lock && _user_oper(b)) { return TRUE; }\
else {\
                                        if((a)->pre_event_handler){ \
                                            if(!((a)->pre_event_handler((a), (b), c)) && (a)->default_event_handler)\
                                                (a)->default_event_handler((a), (b), c);\
                                        }\
                                        else if((a)->default_event_handler){\
                                            (a)->default_event_handler((a), (b), c);\
                                        }\
                                        if(IS_VALID_OBJECT((a)) && (a)->post_event_handler) {\
                                            (a)->post_event_handler((a), (b), c);\
                                        }\
                                    }\
                                }


#define RUN_EVENT_HANDLER2(a, b, c, d, e)       {   \
if (_ui_lock && _user_oper(b)) { return TRUE; }\
else {\
                                                if((a)->pre_event_handler){ \
                                                    if(!((a)->pre_event_handler((a), (b), e)) && (a)->default_event_handler)\
                                                        (c) = (a)->default_event_handler((a), (b), e);\
                                                }\
                                                else if((a)->default_event_handler){\
                                                    (c) = (a)->default_event_handler((a), (b), e);\
                                                }\
                                                if(IS_VALID_OBJECT((a)) && (a)->post_event_handler) {\
                                                    (d) = (a)->post_event_handler((a), (b), e);\
                                                }\
                                            }\
                                        }

/*
 *
 *
 *
 * ##########################################################################*/

#define WIDGET_MAX      512

/* ##########################################################################
 *
 *
 *
 *
 */



#if 0
typedef struct _WIDGET_INFO
{
    int value;  // x or y ctrl coordinate
    int size;   // x or y ctrl size
    int index;  // original order
    int order;  // sorted order
} WIDGET_COORD_INFO;

#else

#define UNIT_PIX_X  ((DISPLAY_ACTIVE_WIDTH / KEYNAV_TABLE_COLUMNS) + 1)
#define UNIT_PIX_Y  ((DISPLAY_ACTIVE_HEIGHT / KEYNAV_TABLE_ROWS) + 1)

typedef struct _WIDGET_INFO
{
    KEYOBJECT *kobj;

    gint start_x, end_x;
    gint start_y, end_y;
} WIDGET_INFO;
#endif


/** TOOLTIP RELATED... **/
#define TOOLTIP_DELAY_TIME      (1000)

struct TOOLTIP_INFO{
    NFOBJECT *object;
    guint open_src;
};

static struct TOOLTIP_INFO tt_info = {NULL, 0};

static int _ui_lock = 0;
static NFOBJECT *pressed_obj=NULL;
static guint repeat_key = 0;
static gboolean repeat_key_proc(gpointer data);
#if defined(__XRPLUS_UI__)
static AUDIO_CH_CHANGE_MODE audio_change_mode = AUDIO_CH_CHANGE_OFF;
#endif

static void prvPaintBGColor(NFOBJECT *obj);
static gboolean prvIsInside(NFOBJECT *obj, gint mx, gint my);
static NFOBJECT* prvFindInsideObject(NFOBJECT *obj, gint mx, gint my);
static void prvVisitAllChildren(NFOBJECT *obj, GdkEvent *evt, gboolean reverse);
static NFOBJECT* prvFindMActObject(NFOBJECT *parent, GdkEvent *evt);
static NFOBJECT* prvFindCurMFocus(NFOBJECT* obj);
static void prvSetMouseFocus(NFOBJECT *obj, gboolean focus);
static gboolean nf_main_event_handler(GtkWidget *widget, GdkEvent *event, gpointer data);

static NFOBJECT* prvIsChildOfContainer(NFOBJECT* object);

static void prvMakeKFocusTree(NFOBJECT *obj, KEYOBJECT *pko, gboolean only_children);
static void prvRealignKFocusByTab(KEYOBJECT *pko);
static KEYOBJECT* prvFindCurKFocus(KEYOBJECT* top_kobject);
static KEYOBJECT* prvFindKeyObjByNfobj(KEYOBJECT* kobj, NFOBJECT* obj);
static KEYOBJECT* prvGetKFocusFromParents(NFOBJECT* obj);
static void prvSetKeyFocus(KEYOBJECT *key_obj, gboolean focus);
static void prvSetKeyFocusAll(KEYOBJECT *key_obj, gboolean focus);
static void prvClearKeyFocus(KEYOBJECT *key_obj);
static void prvMoveKFocus(NFOBJECT* top, KEYPAD_KID dir);
static void prvFreeKObjList(KEYOBJECT *key_obj, gboolean self);

#if 0
static void prvMakeNavTable(KEYOBJECT *keyobject, NFOBJECT* prev_nfobj);
static void prvSortCoordinate(WIDGET_COORD_INFO *info, guint widget_num);
static WIDGET_COORD_INFO *prvSortRows(GSList *widget_array, guint widget_num);
static WIDGET_COORD_INFO *prvSortColumns(GSList *widget_array, guint widget_num);
static gint prvCalcOrder(WIDGET_COORD_INFO *src, WIDGET_COORD_INFO *ref, guint widget_num);
#else
static void prvMakeNavTable(KEYOBJECT *keyobject, NFOBJECT* prev_nfobj);

#endif
static KEYPAD_KID prvKeycodeToKeypadID(KEYCODE key_code);


static int _read_conf()
{
    int ret;
    ret = icf_get_value_by_int("event_loop", "dmsg");
    if (ret != -1) DBG_USE(ret);
    return 0;
}

int init_event_loop()
{
    _read_conf();
    return 0;
}

#ifdef NF_TOOLTIP_ENABLE
static gboolean tooltip_open(gpointer data)
{
    NFOBJECT *obj;

    obj = (NFOBJECT*)data;
    nftool_tooltip_show(obj, NULL);

    tt_info.object = obj;
    tt_info.open_src = 0;

    return FALSE;
}

static void tooltip_close()
{
    nftool_tooltip_hide();

    if(tt_info.open_src)
        g_source_remove(tt_info.open_src);

    tt_info.object = NULL;
    tt_info.open_src = 0;
}
#endif

static void prvPaintBGColor(NFOBJECT *obj)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc = NULL;
    gint x, y;

    NFOBJECT *obj_temp = NULL;

    g_assert(obj);

    if(obj->type == NFOBJECT_TYPE_NFBUTTON)
    {
        if(((NFBUTTON*)obj)->image[obj->status])
            return;
    }

    if(obj->type == NFOBJECT_TYPE_NFTAB)
    {
        return;
    }

    if((obj->type == NFOBJECT_TYPE_NFSCROLLEDFIXED) && (!nfui_nfobject_is_scrolledfixed_usescr(obj)))
    {
        return;
    }    

    if(nfui_nfobject_is_disabled(obj))      //obj->status == NFOBJECT_STATE_DISABLE)
    {
        drawable = nfui_nfobject_get_window(obj);
        gc = nfui_nfobject_get_gc(obj);
        nfui_nfobject_get_offset(obj, &x, &y);

        gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(obj->bg_color[NFOBJECT_STATE_DISABLE]));
        gdk_draw_rectangle(drawable, gc, TRUE, x, y, (gint)(obj->width), (gint)(obj->height));

        nfui_nfobject_gc_unref(gc);

        return;
    }

    obj_temp = obj;

    while(obj_temp->bg_color[NFOBJECT_STATE_NORMAL] == -1)
    {
        if(obj_temp->type == NFOBJECT_TYPE_TOP)
            break;

        if(obj_temp->parent == NULL)
            break;

        obj_temp = obj_temp->parent;
    }

    if(obj_temp->bg_color[NFOBJECT_STATE_NORMAL] != -1)
    {
        drawable = nfui_nfobject_get_window(obj);    
        gc = nfui_nfobject_get_gc(obj);
        nfui_nfobject_get_offset(obj, &x, &y);
        //if (obj->type == NFOBJECT_TYPE_TOP) {
        if (obj->bg_color[NFOBJECT_STATE_NORMAL] != -1) {
            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(obj_temp->bg_color[NFOBJECT_STATE_NORMAL]));
            gdk_draw_rectangle(drawable, gc, TRUE, x, y, (gint)(obj->width), (gint)(obj->height));
        }

        nfui_nfobject_gc_unref(gc);
    }
}

static gboolean prvIsInside(NFOBJECT *obj, gint mx, gint my)
{
    gint left, right, top, bottom;

    NFSCROLLEDFIXED *scrolled_fixed;
    gint scrolled_fixed_x, scrolled_fixed_y;

    g_assert(obj);

    nfui_nfobject_get_offset(obj, &left, &top);

    if (nfui_nfobject_is_scrolledfixed_usescr(obj) && nfui_nfobject_is_scrolledfixed_child(obj))
    {
        scrolled_fixed = nfui_nfobject_get_scrolledfixed(obj);
        nfui_nfobject_get_offset(scrolled_fixed, &scrolled_fixed_x, &scrolled_fixed_y);

        left -= (scrolled_fixed->relative_x - scrolled_fixed_x);
        top -= (scrolled_fixed->relative_y - scrolled_fixed_y);
    }    

    right = left + obj->width;
    bottom = top + obj->height;

    if(mx>=left && mx<right && my>=top && my<bottom) {
        return TRUE;
    }

    return FALSE;
}

static NFOBJECT* prvFindInsideObject(NFOBJECT *obj, gint mx, gint my)
{
    NFOBJECT *child;
    guint i, child_num;

    g_assert(obj);

    switch(obj->type)
    {
        case NFOBJECT_TYPE_TOP :
            child_num = 1;
            child = ((NFWINDOW*)obj)->child;
            g_assert(child);

            if(child->show == NFOBJECT_HIDE)
                return NULL;

            if(prvIsInside(child, mx, my))
                return child;
            else
                return NULL;

        case NFOBJECT_TYPE_NFCALENDAR :
        case NFOBJECT_TYPE_NFCOMBOBOX :
        case NFOBJECT_TYPE_NFFIXED :
        case NFOBJECT_TYPE_NFLISTBOX :
        case NFOBJECT_TYPE_NFSPINBUTTON :
        case NFOBJECT_TYPE_NFTABLE :
        case NFOBJECT_TYPE_NFTIMESPIN :
        case NFOBJECT_TYPE_NFTIMELABEL :
        //case NFOBJECT_TYPE_NFSCROLLEDFIXED :
            //g_message("%s, %d, TYPE:%d", __FUNCTION__, __LINE__, obj->type);

            child_num = g_slist_length(((NFCONTAINER*)obj)->children);

            for(i=0; i<child_num; i++)
            {
                child = (NFOBJECT*)g_slist_nth_data(((NFCONTAINER*)obj)->children, i);
                g_assert(child);

                if(child->show == NFOBJECT_HIDE)
                    continue;

                if(prvIsInside(child, mx, my))
                    return child;
            }
            return NULL;
#if 1
        case NFOBJECT_TYPE_NFSCROLLEDFIXED :
        {
            GSList *children;
            gint left, right, top, bottom;

            nfui_nfobject_get_offset(obj, &left, &top);
            if (((NFSCROLLEDFIXED*)obj)->use_hscroll) {
                right = left + obj->width/2;
                bottom = top + obj->height;
            }
            else {
                right = left + obj->width;
                bottom = top + obj->height/2;
            }

            if (mx >= left && mx < right && my >= top && my < bottom)
            {
                children = ((NFSCROLLEDFIXED*)obj)->top_keylist;
            }
            else
            {
                children = ((NFSCROLLEDFIXED*)obj)->bottom_keylist;
            }

            child_num = g_slist_length(children);

            //g_message("%s, %d, child_num:%d", __FUNCTION__, __LINE__, child_num);

            for(i=0; i<child_num; i++)
            {
                child = (NFOBJECT*)g_slist_nth_data(children, i);
                g_assert(child);

                if(child->show == NFOBJECT_HIDE)
                    continue;

                if(prvIsInside(child, mx, my))
                    return child;
            }
        }
        return NULL;        
#endif
        default :
            return NULL;
    }
}

static void prvMoveScrolledFixed(NFOBJECT *obj)
{
	NFOBJECT *top_win;
	NFSCROLLEDFIXED *scrolled_fixed;
	GdkDrawable *drawable;
	GdkGC *gc;
	gint off_x, off_y;
	
	guint i, child_num;
	NFOBJECT *child;

	top_win = nfui_nfobject_get_top(obj);
	drawable = nfui_nfobject_get_window(top_win);
	scrolled_fixed = (NFSCROLLEDFIXED*)obj;

	nfui_nfobject_get_offset(obj, &off_x, &off_y);

	gc = gdk_gc_new(drawable);	

	gdk_draw_drawable(drawable, gc, scrolled_fixed->scrolledscr, off_x, off_y, 
						off_x, off_y, scrolled_fixed->hscroll_offset, scrolled_fixed->vscroll_offset);

	gdk_draw_drawable(drawable, gc, scrolled_fixed->scrolledscr, scrolled_fixed->relative_x+scrolled_fixed->hscroll_offset, scrolled_fixed->relative_y+scrolled_fixed->vscroll_offset, 
						off_x-scrolled_fixed->hscroll_offset, off_y+scrolled_fixed->vscroll_offset, obj->width-scrolled_fixed->realscr_vmargin-scrolled_fixed->hscroll_offset, obj->height-scrolled_fixed->realscr_hmargin-scrolled_fixed->vscroll_offset);
	g_object_unref(gc);   	
}

static void prvVisitAllChildren(NFOBJECT *obj, GdkEvent *evt, gboolean reverse)
{
    NFWINDOW* top_wnd = NULL;
    NFOBJECT *child;
    guint i, child_num;

    g_assert(obj);

    if((!nfui_nfobject_is_shown(obj)) && \
        (evt->type != GDK_DELETE) && \
        (evt->type != NFEVENT_IPCAMSETUP_DB_SYNC) && \
        (evt->type != NFEVENT_SNMP_RADIO_BUTTON_PRESS) &&\
        (evt->type != NFEVENT_DEBUG_INFOR_DATA_SYNC) &&\
        (evt->type != NFEVENT_VCAREV_COMPONENT_DATA_SYNC) && \
        (evt->type != NFEVENT_VCAREV_COMPONENT_DATA_EXPOSE) && \
        (evt->type != NFEVENT_VCAREV_COMPONENT_PREVIEW_SYNC) && \
        (evt->type != NFEVENT_VCA_STATISTIC_COMPONENT_DATA_SYNC) && \
        (evt->type != NFEVENT_DVA_COMPONENT_DATA_SYNC) && \
        (evt->type != NFEVENT_DVA_COMPONENT_DATA_EXPOSE) && \
        (evt->type != NFEVENT_DVA_COMPONENT_PREVIEW_SYNC))
    {
        if (!nfui_nfobject_is_shown_except(obj, "LIVE_NET_STATUS")) return;
    }

    if(evt->type == GDK_EXPOSE)
    {
        prvPaintBGColor(obj);
    }

    if(reverse == FALSE) {
        RUN_EVENT_HANDLER(obj, evt, NULL)
        if(evt->type == GDK_EXPOSE) nfui_nfscrolledscr_draw_drawable(obj);        
    }

    switch(obj->type)
    {
        case NFOBJECT_TYPE_TOP:
            child = ((NFWINDOW*)obj)->child;
            //g_assert(child);
            if(!child)
            {
              DMSG(1, "child : %p\n", child);
              break;
            }
            prvVisitAllChildren(child, evt, reverse);
            break;

        case NFOBJECT_TYPE_NFTABLE :
        case NFOBJECT_TYPE_NFCALENDAR :
        case NFOBJECT_TYPE_NFCOMBOBOX :
        case NFOBJECT_TYPE_NFFIXED :
        case NFOBJECT_TYPE_NFLISTBOX :
        case NFOBJECT_TYPE_NFTIMESPIN :
        case NFOBJECT_TYPE_NFTIMELABEL :
        case NFOBJECT_TYPE_NFSPINBUTTON :
        // SKSHIN
        case NFOBJECT_TYPE_NFTHUMBNAIL:
        //case NFOBJECT_TYPE_NFSCROLLEDFIXED :   
        {
            child_num = g_slist_length(((NFCONTAINER*)obj)->children);

            for(i=0; i<child_num; i++)
            {
                child = g_slist_nth_data(((NFCONTAINER*)obj)->children, i);
                g_assert(child);

                prvVisitAllChildren(child, evt, reverse);
            }
        }
        break;

		case NFOBJECT_TYPE_NFSCROLLEDFIXED :        
        {
            g_message(">>>>>>>>>>>>PARANGI_DEBUG, %s, %d", __FUNCTION__, __LINE__);
            
            child_num = g_slist_length(((NFSCROLLEDFIXED*)obj)->childrenfull);

            for(i=0; i<child_num; i++)
            {
                child = g_slist_nth_data(((NFSCROLLEDFIXED*)obj)->childrenfull, i);
                g_assert(child);

                prvVisitAllChildren(child, evt, reverse);
            }
        }
        break;

        default :
            break;
    }

    if(reverse == TRUE) {
        RUN_EVENT_HANDLER(obj, evt, NULL)
        if(evt->type == GDK_EXPOSE) nfui_nfscrolledscr_draw_drawable(obj);
    }

    if(evt->type == GDK_DELETE)
    {
        nfui_nfobject_free(obj);
    }
}

static NFOBJECT* prvFindMActObject(NFOBJECT *parent, GdkEvent *evt)
{
    NFOBJECT *object;
    NFOBJECT *child;
    guint mx, my;

    g_assert(parent);

    object = parent;

    if(evt->type == GDK_ENTER_NOTIFY)
    {
        mx = ((GdkEventCrossing*)evt)->x;
        my = ((GdkEventCrossing*)evt)->y;
    }
    else if(evt->type == GDK_MOTION_NOTIFY)
    {
        mx = ((GdkEventMotion*)evt)->x;
        my = ((GdkEventMotion*)evt)->y;
    }
    else if(evt->type == GDK_SCROLL)
    {
        mx = ((GdkEventScroll*)evt)->x;
        my = ((GdkEventScroll*)evt)->y;
    }
    else
    {
        mx = ((GdkEventButton*)evt)->x;
        my = ((GdkEventButton*)evt)->y;
    }

    while(1)
    {
        switch(object->type)
        {
            case NFOBJECT_TYPE_TOP :
            case NFOBJECT_TYPE_NFCALENDAR :
            case NFOBJECT_TYPE_NFCOMBOBOX :
            case NFOBJECT_TYPE_NFFIXED :
			case NFOBJECT_TYPE_NFSCROLLEDFIXED :			
            case NFOBJECT_TYPE_NFLISTBOX :
            case NFOBJECT_TYPE_NFTABLE :
            case NFOBJECT_TYPE_NFTIMESPIN :
            case NFOBJECT_TYPE_NFTIMELABEL :
            case NFOBJECT_TYPE_NFSPINBUTTON :
                child = prvFindInsideObject(object, mx, my);
                break;

            default :
                child = NULL;
                break;
        }

        if(child)
        {
            object = child;
        }
        else
        {
            return object;
        }
    }
    return NULL;
}


static NFOBJECT* prvFindCurMFocus(NFOBJECT* obj)
{
    NFOBJECT *temp;
    guint i, child_num;

    g_assert(obj);

    if(obj->type == NFOBJECT_TYPE_TOP)
        temp = obj;
    else
        temp = nfui_nfobject_get_top(obj);

    g_assert(temp);

    if(temp->mfocus == NFOBJECT_UNFOCUS)
    {
        return NULL;
    }

    temp = ((NFWINDOW*)temp)->child;

    g_assert(temp);

    if(temp->mfocus == NFOBJECT_UNFOCUS)
    {
        return temp->parent;
    }

    while(1)
    {
        switch(temp->type)
        {
            case NFOBJECT_TYPE_NFCALENDAR :
            case NFOBJECT_TYPE_NFCOMBOBOX :
            case NFOBJECT_TYPE_NFFIXED :
			case NFOBJECT_TYPE_NFSCROLLEDFIXED :
            case NFOBJECT_TYPE_NFLISTBOX :
            case NFOBJECT_TYPE_NFSPINBUTTON :
            case NFOBJECT_TYPE_NFTABLE :
            case NFOBJECT_TYPE_NFTIMESPIN :
            case NFOBJECT_TYPE_NFTIMELABEL :
            {
                NFOBJECT *child;

                child_num = g_slist_length(((NFCONTAINER*)temp)->children);

                for(i=0; i<child_num; i++)
                {
                    child = (NFOBJECT*)g_slist_nth_data(((NFCONTAINER*)temp)->children, i);
                    g_assert(child);

                    if(child->mfocus == NFOBJECT_FOCUS)
                    {
                        temp = child;
                        break;
                    }
                }

                if(i == child_num)
                {
                    return temp;
                }
            }
            break;

            default :
                return temp;
        }
    }

    return NULL;
}

static void prvSetMouseFocus(NFOBJECT *obj, gboolean focus)
{
    NFOBJECT* temp;


    if(obj == NULL)
    {
        return;
    }

    temp = obj;

    while(1)
    {
        g_assert(temp);
        g_assert(IS_VALID_OBJECT(temp));

        if(focus)   temp->mfocus = NFOBJECT_FOCUS;
        else        temp->mfocus = NFOBJECT_UNFOCUS;

        if(temp->type == NFOBJECT_TYPE_TOP)
            break;

        temp = temp->parent;
    }

}


gint prvMoveWindow(NFWINDOW* wnd, GdkEvent* event)
{
    static NFWINDOW *prev_wnd = NULL;
    static gint prev_mx = -1;
    static gint prev_my = -1;

    if(event->type == GDK_BUTTON_PRESS)
    {
        GdkEventButton *bevt = NULL;
        NFOBJECT *temp = NULL;
        gint win_x, win_y;

        bevt = (GdkEventButton*)event;

        nfui_nfobject_get_window_pos((NFOBJECT*)wnd, &win_x, &win_y);

        // check for modal wnd
        if((gint)bevt->x_root < win_x) return 0;
        if((gint)bevt->x_root > win_x+((NFOBJECT*)wnd)->width) return 0;
        if((gint)bevt->y_root < win_y) return 0;
        if((gint)bevt->y_root > win_y+((NFOBJECT*)wnd)->height) return 0;

        temp = prvFindMActObject((NFWINDOW*)wnd, event);

        if(temp && temp->use_focus == NFOBJECT_FOCUS_ON)
            return 0;

        if(wnd->moving_area_y_pos == 0) {
            if((gint)bevt->y > wnd->moving_area_size)
                return 0;
        }else {
            if((gint)bevt->y < wnd->moving_area_y_pos || (gint)bevt->y > (wnd->moving_area_size + wnd->moving_area_y_pos))
                return 0;
        }

        nftool_change_custom_cursor(GTK_WIDGET(wnd->main_widget)->window, NF_CURSOR_FLEUR);

        prev_wnd = wnd;
        prev_mx = bevt->x_root;
        prev_my = bevt->y_root;

        return 1;
    }
    else if((event->type == GDK_MOTION_NOTIFY) && (wnd->moving_effect == 1))
    {
        GdkEventButton *bevt;
        gint x, y;

        if(prev_mx < 0 && prev_my < 0)
            return 0;

        if(prev_wnd != wnd)
            return 0;

        bevt = (GdkEventButton*)event;

        x = bevt->x_root - prev_mx;
        y = bevt->y_root - prev_my;

        x += wnd->object.x;
        y += wnd->object.y;

        if (wnd->moving_limit)
        {
            if(x > DISPLAY_ACTIVE_WIDTH-wnd->object.width)      x = DISPLAY_ACTIVE_WIDTH-wnd->object.width;
            if(y > DISPLAY_ACTIVE_HEIGHT-wnd->object.height)    y = DISPLAY_ACTIVE_HEIGHT-wnd->object.height;
            if(x < 0) x = 0;
            if(y < 0) y = 0;
        }
        else
        {
            if(x > DISPLAY_ACTIVE_WIDTH-12) x = DISPLAY_ACTIVE_WIDTH-12;
            if(y > DISPLAY_ACTIVE_HEIGHT-8) y = DISPLAY_ACTIVE_HEIGHT-8;
            if(x < 0) x += 12;
            if(y < 0) y += 8;
        }

        nfui_nfobject_move((NFOBJECT*)wnd, x, y);

        prev_mx = bevt->x_root;
        prev_my = bevt->y_root;

        return 1;
    }
    else if(event->type == GDK_BUTTON_RELEASE)
    {
        GdkEventButton *bevt;
        gint x, y;

        if(prev_mx < 0 && prev_my < 0)
            return 0;

        if(prev_wnd != wnd)
            return 0;

        bevt = (GdkEventButton*)event;

        x = bevt->x_root - prev_mx;
        y = bevt->y_root - prev_my;

        x += wnd->object.x;
        y += wnd->object.y;

        if (wnd->moving_limit)
        {
            if(x > DISPLAY_ACTIVE_WIDTH-wnd->object.width)      x = DISPLAY_ACTIVE_WIDTH-wnd->object.width;
            if(y > DISPLAY_ACTIVE_HEIGHT-wnd->object.height)    y = DISPLAY_ACTIVE_HEIGHT-wnd->object.height;
            if(x < 0) x = 0;
            if(y < 0) y = 0;
        }
        else
        {
            if(x > DISPLAY_ACTIVE_WIDTH-12) x = DISPLAY_ACTIVE_WIDTH-12;
            if(y > DISPLAY_ACTIVE_HEIGHT-8) y = DISPLAY_ACTIVE_HEIGHT-8;
            if(x < 0) x += 12;
            if(y < 0) y += 8;
        }

        nfui_nfobject_move((NFOBJECT*)wnd, x, y);
        //nfui_nfobject_move((NFOBJECT*)wnd, wnd->object.x + x, wnd->object.y + y);

        nftool_change_custom_cursor(GTK_WIDGET(wnd->main_widget)->window, NF_CURSOR_ARROW);

        prev_mx = -1;
        prev_my = -1;
        prev_wnd = NULL;

        return 1;
    }
    else
    {
        return 0;
    }

    return 0;
}

static gboolean prvCloseWindow(KEYOBJECT *top_kobj, GdkEvent *event)
{
    KEYOBJECT *cur_focus = NULL;
    NFOBJECT *exit_wnd = NULL;
    NFOBJECT *top_obj = NULL;
    gboolean close = 1;
    PAGEID pid = PGID_NONE;

    pid = nfui_get_cur_page(&top_obj);
    cur_focus = prvFindCurKFocus(top_kobj);

    /* exception */
    switch(pid)
    {
        case PGID_NONE:
        case PGID_LIVEDISPLAY:
        case PGID_LIVECTRLBAR:
        case PGID_LIVEWCTRLBAR:
        case PGID_PLAYBACK:
        case PGID_PLAYBACK_CONTROL:
        case PGID_PANORAMA:
        case PGID_DIGIZOOM:
        case PGID_MOTION_AREA:
        case PGID_MOTION_AREA_MENU:
        case PGID_MOTION_AREA_SUBMENU:
        case PGID_LIVE_LOG:
        case PGID_PLAYBACK_CTRLBAR:
        case PGID_PLAYBACK_START_MENU:
        case PGID_PLAYBACK_CONTROL_BOX:
        case PGID_PLAYBACK_CONTROL_DISP_BOX:
        case PGID_PLAYBACK_CONTROL_FUNC_BOX:
        case PGID_COMBO_MENU:
        case PGID_SUB_SETUPMENU:
            return FALSE;
    }


            DMSG(1, "");
    if(!cur_focus)  return FALSE;
            DMSG(1, "");
    if((cur_focus->depth!=1) || (cur_focus->object->grab_kfocus))   return FALSE;
            DMSG(1, "");

    exit_wnd = nfui_nfobject_get_top(cur_focus->object);
            DMSG(1, "");
    if(!exit_wnd)   return FALSE;
            DMSG(1, "");

    if(exit_wnd != top_obj)
    {
        DMSG(1, "WINDOW DISMATCH!!!!! --------");
        return FALSE;
    }

            DMSG(1, "");
    if(((NFWINDOW*)exit_wnd)->returnkey_proc)
        close = ((NFWINDOW*)exit_wnd)->returnkey_proc(exit_wnd, event, NULL);

            DMSG(1, "");
    if(!close)  return FALSE;

            DMSG(1, "");
    g_assert(exit_wnd);
    g_assert(IS_VALID_OBJECT(exit_wnd));

    switch(pid)
    {
        case PGID_LIVE_START_MENU:
            {
                NFOBJECT *obj = nfui_get_object_by_pid(pid);
                VW_Hide_Image_Popup();
                nfui_nfobject_hide(obj);
                wnd_send_to_back(obj);
            }
            break;

        case PGID_START_SEQURINET_POPUP:
            {
                NFOBJECT *obj = nfui_get_object_by_pid(pid);
                nfui_nfobject_hide(obj);
                wnd_send_to_back(obj);
            }
            break;

        default:
            nfui_nfobject_destroy(exit_wnd);
            break;
    }

            DMSG(1, "");
    return TRUE;
}


#ifdef _CURSOR_DISPLAY_ONOFF //nskim display on/off cursor
static GSList *gdk_window_list = NULL;
int mouse_connected_flag = 0;
extern int hid_joystick_connected_flag;
extern int hid_mouse_flag;
static int mouse_prev_connected_flag = 0;

/* GtkWindow 파괴 시 gdk_window_list에서 해당 포인터를 제거하여 dangling pointer 방지 */
static void on_gtk_window_destroyed(GtkObject *object, gpointer data)
{
    GdkWindow *gdk_window = (GdkWindow *)data;
    gdk_window_list = g_slist_remove(gdk_window_list, gdk_window);
}

void htm_ui_set_custom_cursor(GtkWindow *wnd)
{
    nfcursor_type type = NF_CURSOR_NULL;  /* 미초기화 방지 */
    GdkWindow *gdk_window = NULL;

    gdk_window = GTK_WIDGET(wnd)->window;

    if (gdk_window == NULL)
    {
        return;
    }

    /* 중복 등록 방지 및 파괴 시 자동 제거 콜백 등록 */
    if (g_slist_find(gdk_window_list, gdk_window) == NULL)
    {
        gdk_window_list = g_slist_append(gdk_window_list, gdk_window);
        g_signal_connect(G_OBJECT(wnd), "destroy",
            G_CALLBACK(on_gtk_window_destroyed), gdk_window);
    }

    if (mouse_connected_flag == 1 || hid_joystick_connected_flag == 1)
        type = NF_CURSOR_ARROW;
    else
        type = NF_CURSOR_NULL;

    nftool_change_custom_cursor(gdk_window, type);

    return;
}

void htm_ui_set_mouse_pointer(void)
{


#if 1
{
{
  int i = 0;
  int gdk_window_list_cnt = 0;
  nfcursor_type type = NF_CURSOR_NULL;  /* 미초기화 방지 */
  GdkWindow *gdk_window = NULL;
  GSList *invalid_list = NULL;

  if (mouse_connected_flag == 1 || hid_joystick_connected_flag == 1)
      type = NF_CURSOR_ARROW;
  else
      type = NF_CURSOR_NULL;

  gdk_window_list_cnt = g_slist_length(gdk_window_list);

  for (i = 0; i < gdk_window_list_cnt; i++)
  {
      gdk_window = g_slist_nth_data(gdk_window_list, i);

      /* NULL이거나 유효하지 않은 GdkWindow는 수집 후 제거 (dangling pointer 방어) */
      if (gdk_window == NULL || !GDK_IS_WINDOW(gdk_window))
      {
          invalid_list = g_slist_append(invalid_list, gdk_window);
          continue;
      }

      nftool_change_custom_cursor(gdk_window, type);
  }

  /* 무효 포인터 일괄 제거 */
  if (invalid_list)
  {
      GSList *iter = invalid_list;
      while (iter)
      {
          gdk_window_list = g_slist_remove(gdk_window_list, iter->data);
          iter = iter->next;
      }
      g_slist_free(invalid_list);
      invalid_list = NULL;
  }

}
}
#else
//XFreePixmap

  if (mouse_connected_flag == 1)
  {
#if 0
    //free_mouse_pointer();
    if(cursor) {
        XFreeCursor(dpy, cursor);
        cursor = NULL;
    }
#endif
  }
  else
  {
#if 0
     if(cursor == 0){
        screen = DefaultScreen(dpy);
        root = RootWindow(dpy, screen);
    //init_hide_mouse_pointer();
    dpy = XOpenDisplay(NULL);
//  cursor = CreateCursorFromFiles(left_ptrmsk_bits, left_ptrmsk_bits);
    cursor = XcursorFilenameLoadCursor(dpy, left_ptrmsk_bits);
    //XDefineCursor(dpy, root, cursor);
     }
#endif
   }
#endif
    return;

}

void nf_ui_check_mouse_connect(void)
{
  struct stat stat_buf;
  gchar dev_name[128];

  int mouse_connect_flag = 0;

  sprintf(dev_name, "%s", "/dev/input/mouse0");

  if (stat(dev_name, &stat_buf) < 0)
  {
//    mouse_connected_flag = 0;
  }
  else
  {
//    mouse_connected_flag = 1;
    mouse_connect_flag = 1;
  }

  sprintf(dev_name, "%s", "/dev/input/mouse1");

  if (stat(dev_name, &stat_buf) < 0)
  {
//    mouse_connected_flag = 0;
  }
  else
  {
//    mouse_connected_flag = 1;
    mouse_connect_flag = 1;
  }

  sprintf(dev_name, "%s", "/dev/input/mouse3");

  if (stat(dev_name, &stat_buf) < 0)
  {
//    mouse_connected_flag = 0;
  }
  else
  {
//    mouse_connected_flag = 1;
    mouse_connect_flag = 1;
  }


  mouse_connected_flag = mouse_connect_flag;

 if(mouse_connected_flag)
 {
//  itx_tw2880_mouse_setactivecursor(0);
 }else
    {
//  itx_tw2880_mouse_inactivecursor();
 }

  if (mouse_prev_connected_flag != mouse_connected_flag)
  {
    htm_ui_set_mouse_pointer();
  }
  if(hid_mouse_flag >= 0)		// hid change state connected
  {
  	if(hid_mouse_flag)
  	{
		if(mouse_connected_flag == 0)
		{
			htm_ui_set_mouse_pointer();
		}
  	}
	else				// hid change state disconnected
	{
		if(mouse_connected_flag == 0)
		{
			htm_ui_set_mouse_pointer();
			hid_mouse_flag = -1;
		}
	}
	hid_mouse_flag = -1;
  }


  mouse_prev_connected_flag = mouse_connected_flag;

}

#endif

static int semi_modal = 0;
static NFWINDOW *modal_wnd = 0;
static NFWINDOW *semimodal_wnd = 0;
//static GdkEventType hook_evt_type = 0;
static guint64 hook_evt_msk = 0;

int nfui_set_semimodal_wnd(NFWINDOW *window)
{
    if (!window) return -1;

    semimodal_wnd = window;
    return 0;
}

int nfui_enable_semi_modal_mode(NFWINDOW *window)
{
    if (!window) return -1;

    semi_modal = 1;
    modal_wnd = window;
    return 0;
}

int nfui_disable_semi_modal_mode()
{
    semi_modal = 0;
    semimodal_wnd = 0;
    return 0;
}

int nfui_hook_evt_in_semi_modal(GdkEventType type)
{
    if ((type < 0) || (type > 63)) {
        g_warning("%s, %d, over : %d", __FUNCTION__, __LINE__, type);
        g_assert(0);
    }

    hook_evt_msk |= (1ULL << type);
    return 0;
}

int nfui_ui_lock()
{
    _ui_lock = 1;
    return 0;
}

int nfui_ui_unlock()
{
    _ui_lock = 0;
    return 0;
}

static gint _check_event_autohide(GdkEvent *event)
{
    if (event->type == GDK_SCROLL ||
        event->type == GDK_BUTTON_PRESS ||
        event->type == GDK_2BUTTON_PRESS ||
        event->type == GDK_BUTTON_RELEASE ||
        event->type == GDK_MOTION_NOTIFY ||
        event->type == NFEVENT_JOG_CHANGE ||
        event->type == NFEVENT_SHUTTLE_CHANGE ||
//      event->type == NFEVENT_KEYPAD_PRESS ||
        event->type == NFEVENT_KEYPAD_RELEASE ||
//      event->type == NFEVENT_REMOCON_PRESS ||
        event->type == NFEVENT_REMOCON_RELEASE)
    {
        if (vw_autohide_occur_event(event) == 0)
            return -1;
    }

    return 0;
}

static gint _check_event_autologout(GdkEvent *event)
{
    NFOBJECT *last_page = NULL;
    PAGEID last_pgid;

    if (event->type == GDK_SCROLL ||
        event->type == GDK_BUTTON_PRESS ||
        event->type == GDK_2BUTTON_PRESS ||
        event->type == GDK_BUTTON_RELEASE ||
        event->type == GDK_MOTION_NOTIFY ||
        event->type == NFEVENT_JOG_CHANGE ||
        event->type == NFEVENT_SHUTTLE_CHANGE ||
//      event->type == NFEVENT_KEYPAD_PRESS ||
        event->type == NFEVENT_KEYPAD_RELEASE ||
//      event->type == NFEVENT_REMOCON_PRESS ||
        event->type == NFEVENT_REMOCON_RELEASE)
    {
        last_pgid = nfui_get_cur_page(&last_page);

        if(last_pgid == PGID_USERPWD_AUTOLOGOUT && !nfui_nfobject_is_shown(last_page))
        {
            nfui_nfobject_show(last_page);
            nffunc_change_led(last_pgid);
            nffunc_led_on(NFLED_RETURN);
            return -1;
        }
    }

    return 0;
}

static gboolean nf_main_event_handler(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    NFOBJECT *old_mfocus = NULL;
    NFOBJECT *active = NULL;
    KEYOBJECT *old_kfocus = NULL;
    KEYOBJECT *act_kfocus = NULL;
    NFOBJECT *object = NULL;

    object = (NFOBJECT*)data;

    g_assert(object);

    if(!(IS_VALID_OBJECT(object)))
    {
      DMSG(1, "INVALID OBJECT!!!! Event->type[%d]\n", event->type);
      return FALSE;
    }

    if (_ui_lock && _user_oper(event)) return TRUE;

    // stop buzzer
    if (event->type == GDK_BUTTON_PRESS)
    {
        nf_action_mouse_untilkey_stop();
    }

    if (event->type == GDK_BUTTON_RELEASE)
    {
        VW_IPCamInstallSearch_Page_ver2_mevt_cb(event);
    }

    if ((event->type == NFEVENT_KEYPAD_PRESS) || (event->type == NFEVENT_REMOCON_PRESS))
    {
        GdkEventKey *kevt;
        KEYPAD_KID kpid;

        kevt = (GdkEventKey*)event;
        kpid = (KEYPAD_KID)kevt->keyval;

        if (kpid == KEYPAD_PANIC)
        {
            scm_toggle_panic_record();
        }

        if (kpid == KEYPAD_SEQ) 
        {
            if (ssm_is_itxdebug_id()) {
                g_message("###yanggungg : %s, %d, ssm_run_auto_logout()", __FUNCTION__, __LINE__);
                ssm_run_auto_logout();
                return FALSE;
            }
        }
    }

    if ((event->type == GDK_BUTTON_PRESS) || (event->type == NFEVENT_KEYPAD_PRESS) || (event->type == NFEVENT_REMOCON_PRESS))
    {
//      if (vsm_untilkey_stop() == 0) return FALSE;
        vsm_untilkey_stop();
    }

#if defined(_SUPPORT_LOGIN_AUTOHIDE)
    if (_check_event_autohide(event) == -1)
        return FALSE;
#endif

    if (_check_event_autologout(event) == -1)
        return FALSE;

    if (semi_modal) {
        if (event->type == GDK_SCROLL ||
            event->type == GDK_BUTTON_PRESS ||
            event->type == GDK_2BUTTON_PRESS ||
            event->type == GDK_BUTTON_RELEASE ||
            event->type == GDK_MOTION_NOTIFY ||
            event->type == GDK_LEAVE_NOTIFY ||
            event->type == GDK_ENTER_NOTIFY ||
            event->type == GDK_FOCUS_CHANGE ||
            event->type == NFEVENT_JOG_CHANGE ||
            event->type == NFEVENT_SHUTTLE_CHANGE ||
            event->type == NFEVENT_KEYPAD_PRESS ||
            event->type == NFEVENT_KEYPAD_RELEASE ||
            event->type == NFEVENT_REMOCON_PRESS ||
            event->type == NFEVENT_REMOCON_RELEASE)
        {

            NFWINDOW* top_wnd = NULL;
            top_wnd = nfui_nfobject_get_top(object);
            int forwarding = 0;

            if (semimodal_wnd == top_wnd) forwarding = 1;

            /* Event at outside of window.. */
            if(hook_evt_msk & (1ULL << event->type))
            {
                RUN_EVENT_HANDLER((NFOBJECT*)modal_wnd, event, NULL)
            }

            if (forwarding == 0) {
                if (((NFWINDOW*)top_wnd)->semi_modal == 0) return FALSE;
            }
        }
    }

    /* Related Key Lock... TT... not good.. at this position.. */
    #ifdef _CURSOR_DISPLAY_ONOFF //nskim display on/off cursor
      if (event->type == GDK_MAP)
      {

        htm_ui_set_custom_cursor(widget);
      }
    #endif

    /* WINDOW MOVING */
    if(widget && (event->type == GDK_BUTTON_PRESS || event->type == GDK_BUTTON_RELEASE || event->type == GDK_MOTION_NOTIFY))
    {
        gint pass;
        NFWINDOW* mtop = NULL;

        mtop = nfui_nfobject_get_top(object);

        g_assert(mtop);

        if(mtop && mtop->moving_area_size)
        {
            pass = prvMoveWindow(mtop, event);

            if(pass)
                return FALSE;
        }
    }

    /* IDLE TIMER RESET */
    if(event->type == GDK_BUTTON_PRESS || event->type == GDK_BUTTON_RELEASE ||
        event->type == GDK_MOTION_NOTIFY || event->type == GDK_KEY_PRESS ||
        event->type == GDK_KEY_RELEASE || event->type == NFEVENT_KEYPAD_PRESS ||
        event->type == NFEVENT_KEYPAD_RELEASE || event->type == NFEVENT_REMOCON_PRESS ||
        event->type == NFEVENT_REMOCON_RELEASE)
    {
//      nfui_idle_timer_reset();
        ssm_reset_auto_logout_timer();
    }

    /* Event at outside of window.. */
    if((widget != NULL) &&
        (widget->window != ((GdkEventAny*)event)->window))
    {
        NFWINDOW* top_wnd = NULL;

        top_wnd = nfui_nfobject_get_top(object);

        g_assert(top_wnd);

        if((top_wnd->outside_evt) && nfui_nfwindow_is_mask(top_wnd, event->type))
        {
            GdkEventType type_cp;

            type_cp = event->type;

            if(event->type == GDK_BUTTON_PRESS)         event->type = NFOUTEVT_BUTTON_PRESS;
            else if(event->type == GDK_BUTTON_RELEASE)  event->type = NFOUTEVT_BUTTON_RELEASE;
            else if(event->type == GDK_2BUTTON_PRESS)   event->type = NFOUTEVT_2BUTTON_PRESS;
            else if(event->type == GDK_MOTION_NOTIFY)   event->type = NFOUTEVT_MOTION_NOTIFY;
            else if(event->type == GDK_SCROLL)          event->type = NFOUTEVT_SCROLL;

            RUN_EVENT_HANDLER((NFOBJECT*)top_wnd, event, NULL)

            event->type = type_cp;

            return FALSE;
        }
        else if(event->type != GDK_KEY_PRESS && event->type != GDK_KEY_RELEASE)
        {

            pressed_obj = NULL;

            return FALSE;
        }
    }

    switch(event->type)
    {
        case GDK_MAP:
            if(pressed_obj && IS_VALID_OBJECT(pressed_obj))
            {
                nfui_signal_emit(pressed_obj, GDK_LEAVE_NOTIFY, TRUE);
                pressed_obj = NULL;
            }
                        break;

        case GDK_EXPOSE :
                prvVisitAllChildren((NFOBJECT*)object, event, FALSE);
            break;

        case GDK_CLIENT_EVENT:          // custom event transpering
            switch (event->client.data.l[0])
            {
                case GDK_EXPOSE:
                {
                    GdkEvent *evt;
                    evt = gdk_event_new(GDK_EXPOSE);
                    prvVisitAllChildren((NFOBJECT*)(event->client.data.l[1]), evt, FALSE);
                    gdk_event_free(evt);
                }
                break;
            }
        break;

        case GDK_SCROLL:
            active = prvFindMActObject(object, event);

            if(active == NULL || nfui_nfobject_is_disabled(active))     //active->status == NFOBJECT_STATE_DISABLE)
                break;

            RUN_EVENT_HANDLER(active, event, NULL)
            break;

        case GDK_BUTTON_PRESS :
        case GDK_2BUTTON_PRESS :
        {
            NFOBJECT *top = NULL;
            KEYOBJECT *top_kobj = NULL;
            NFOBJECT *obj_temp = NULL;
            NFOBJECT *obj_temp2 = NULL;
            KEYOBJECT *kobj_temp = NULL;
            guint sync = 0;

            if(repeat_key)
            {
                g_source_remove(repeat_key);
                repeat_key = 0;
            }

            old_mfocus = prvFindCurMFocus(object);
            active = prvFindMActObject(object, event);

            if(active == NULL || nfui_nfobject_is_disabled(active))     //active->status == NFOBJECT_STATE_DISABLE)
            {
                pressed_obj = NULL;
                break;
            }

            old_kfocus = NULL;
            act_kfocus = NULL;

            top = nfui_nfobject_get_top(object);
            top_kobj = nfui_nfwindow_get_keyobj((NFWINDOW*)top);

            if(top_kobj)
            {
                old_kfocus = prvFindCurKFocus(top_kobj);
                act_kfocus = prvFindKeyObjByNfobj(top_kobj, active);

                if(act_kfocus == NULL)
                    act_kfocus = prvGetKFocusFromParents(active);

                if((act_kfocus == NULL) &&
                    (active->type != NFOBJECT_TYPE_NFBUTTON) &&
                    (active->type != NFOBJECT_TYPE_NFCHECKBUTTON) &&
				    (active->type != NFOBJECT_TYPE_IXTIMELINE) &&
				    (active->type != NFOBJECT_TYPE_NFSCROLLEDFIXED))
                    break;
            }

            if(old_mfocus != active)
            {
                if(old_mfocus) prvSetMouseFocus(old_mfocus, FALSE);
                prvSetMouseFocus(active, TRUE);
            }

            pressed_obj = active;

#if 1   // sync. keyfocus and mousefocus
            if(old_kfocus)
                obj_temp = prvIsChildOfContainer(old_kfocus->object);
            if(act_kfocus)
                obj_temp2 = prvIsChildOfContainer(act_kfocus->object);


            if((old_kfocus == act_kfocus && obj_temp) || (old_kfocus != act_kfocus))
                sync = 1;

            if(sync)
            {
                if(obj_temp && obj_temp == obj_temp2)
                {
                    kobj_temp = act_kfocus->parent;
                    prvClearKeyFocus(old_kfocus);
                    prvSetKeyFocusAll(kobj_temp, TRUE);
                }
                else
                {
                    if(old_kfocus)      prvClearKeyFocus(old_kfocus);
                    prvSetKeyFocusAll(act_kfocus, TRUE);
                }

                if(old_kfocus)
                {
                    // 2011-08-30 modified by seongho
                    if(obj_temp && obj_temp->type != NFOBJECT_TYPE_NFTAB)                           nfui_signal_emit(obj_temp, GDK_EXPOSE, TRUE);
                    else if(old_kfocus->object && old_kfocus->object->type != NFOBJECT_TYPE_NFTAB)  nfui_signal_emit(old_kfocus->object, GDK_EXPOSE, TRUE);
                }
                // 2011-08-30 modified by seongho
                if(act_kfocus && act_kfocus->object && act_kfocus->object->type != NFOBJECT_TYPE_NFTAB) {
                    nfui_signal_emit(act_kfocus->object, GDK_EXPOSE, TRUE);
                }
            }
#endif
            nfui_nfscrolledscr_draw_drawable(active);
            RUN_EVENT_HANDLER(active, event, NULL)
        }
        break;

        case GDK_BUTTON_RELEASE :
            if(repeat_key)
            {
                g_source_remove(repeat_key);
                repeat_key = 0;
            }

            old_mfocus = prvFindCurMFocus(object);
            active = prvFindMActObject(object, event);

            if(pressed_obj != active || !pressed_obj)
            {

                pressed_obj = NULL;
                break;
            }

            if(old_mfocus != active)
            {
                if(old_mfocus)      prvSetMouseFocus(old_mfocus, FALSE);
                prvSetMouseFocus(active, TRUE);
            }

            if(old_mfocus == active) {
                RUN_EVENT_HANDLER(active, event, NULL)
                //nfui_nfscrolledscr_draw_drawable(active);
            }

            pressed_obj = NULL;
            break;

        case GDK_MOTION_NOTIFY :
        {
            NFOBJECT* top;
            NFOBJECT* obj_temp;
            NFOBJECT* obj_temp2;
            KEYOBJECT* top_kobj;

            NFOBJECT* send_leave = NULL;
            GdkEventMotion *m_event;

            top = nfui_nfobject_get_top(object);
            g_assert(top);

            if (!nfui_nfwindow_is_exposed((NFWINDOW*)top)) break;

            m_event = (GdkEventMotion*)event;

#ifdef NF_TOOLTIP_ENABLE
            nftool_tooltip_set_mouse_position((guint)m_event->x_root, (guint)m_event->y_root);
#endif

            if(repeat_key)
            {
                g_source_remove(repeat_key);
                repeat_key = 0;
            }

            old_mfocus = prvFindCurMFocus(object);
            active = prvFindMActObject(object, event);

            if(active == NULL || nfui_nfobject_is_disabled(active))     //active->status == NFOBJECT_STATE_DISABLE)
                break;

            top_kobj = nfui_nfwindow_get_keyobj((NFWINDOW*)top);

            if ((active->type == NFOBJECT_TYPE_NFBUTTON) ||
                (active->type == NFOBJECT_TYPE_NFCHECKBUTTON) ||
                (active->type == NFOBJECT_TYPE_NFTHUMBNAIL))
            {
                obj_temp = prvIsChildOfContainer(active);

                if(top_kobj && obj_temp == NULL)
                {
                    old_kfocus = prvFindCurKFocus(top_kobj);
                    act_kfocus = prvFindKeyObjByNfobj(top_kobj, active);

                    if(old_kfocus && (old_kfocus->object!=active))
                    {
//                      prvClearKeyFocus(old_kfocus);
                        obj_temp2 = prvIsChildOfContainer(old_kfocus->object);

                        if (obj_temp2)
                            nfui_signal_emit(obj_temp2, NFEVENT_CANCEL, FALSE);

                        prvClearKeyFocus(old_kfocus);
                        if(obj_temp2)   send_leave = obj_temp2;
                        else            send_leave = old_kfocus->object;
#ifdef NF_TOOLTIP_ENABLE
                            tooltip_close();
#endif
                        if (send_leave) {
                            nfui_signal_emit(send_leave, GDK_LEAVE_NOTIFY, TRUE);
                            nfui_nfscrolledscr_draw_drawable(send_leave);
                        }
                    }
                }
            }

            if((old_mfocus != active))
            {
                GdkEvent *evt_cp;

                if(old_mfocus)
                {
                    if(old_mfocus != send_leave)
                    {
#ifdef NF_TOOLTIP_ENABLE
                        tooltip_close();
#endif
                        evt_cp = gdk_event_new(GDK_LEAVE_NOTIFY);
                        RUN_EVENT_HANDLER(old_mfocus, evt_cp, NULL )
                        nfui_nfscrolledscr_draw_drawable(old_mfocus);
                        gdk_event_free(evt_cp);
                    }

                    send_leave = NULL;

                    prvSetMouseFocus(old_mfocus, FALSE);
                }


                //TODO : if(active == pressed_obj)  send GDK_BUTTON_PRESSED.. ??
                //          else send GDK_ENTER_NOTIFY ??

                evt_cp = gdk_event_new(GDK_ENTER_NOTIFY);

#ifdef NF_TOOLTIP_ENABLE
				if(active->use_tooltip && active->type != NFOBJECT_TYPE_NFTAB && active->type != NFOBJECT_TYPE_NFLISTBOX)
                {
                    if(tt_info.object == active)
                    {
                        if(tt_info.open_src == 0)
                            tt_info.open_src = g_timeout_add(TOOLTIP_DELAY_TIME, tooltip_open, active);
                    }
                    else
                    {
                        if(tt_info.open_src)
                            g_source_remove(tt_info.open_src);

                        tt_info.open_src = g_timeout_add(TOOLTIP_DELAY_TIME, tooltip_open, active);
                    }
                }
#endif
                RUN_EVENT_HANDLER(active, evt_cp, NULL)
                nfui_nfscrolledscr_draw_drawable(active);
                gdk_event_free(evt_cp);
                prvSetMouseFocus(active, TRUE);
            }

#if 1   // sync. keyfocus and mousefocus
            if(old_kfocus != act_kfocus)
            {
                if(old_kfocus &&
                    (active->type != NFOBJECT_TYPE_NFBUTTON) &&
                    (active->type != NFOBJECT_TYPE_NFCHECKBUTTON) &&
                    (active->type != NFOBJECT_TYPE_NFTHUMBNAIL))
                    prvClearKeyFocus(old_kfocus);

                prvSetKeyFocusAll(act_kfocus, TRUE);

//              if(old_kfocus)  nfui_signal_emit(old_kfocus->object, GDK_EXPOSE, TRUE);
//              if(act_kfocus)  nfui_signal_emit(act_kfocus->object, GDK_EXPOSE, TRUE);
            }
#endif

            RUN_EVENT_HANDLER(active, event, NULL)
        }
        break;

        case GDK_LEAVE_NOTIFY :
        {
            NFOBJECT* top;

            top = nfui_nfobject_get_top(object);
            g_assert(top);

            if (!nfui_nfwindow_is_exposed((NFWINDOW*)top)) break;

            old_mfocus = prvFindCurMFocus(object);

            if(old_mfocus == object)
            {
                prvSetMouseFocus(old_mfocus, FALSE);

            }

#ifdef NF_TOOLTIP_ENABLE
            tooltip_close();
#endif
            RUN_EVENT_HANDLER(object, event, NULL)
            nfui_nfscrolledscr_draw_drawable(object);
        }
            break;

        case GDK_ENTER_NOTIFY :
        {
            NFOBJECT* top;
            KEYOBJECT* top_kobj;
            
            top = nfui_nfobject_get_top(object);
            g_assert(top);

            if (!nfui_nfwindow_is_exposed((NFWINDOW*)top)) break;

            old_mfocus = prvFindCurMFocus(object);
            active = prvFindMActObject(object, event);

            if(old_mfocus) {
                prvSetMouseFocus(old_mfocus, FALSE);

// SKSHIN
#if 0
            {
                GdkWindow *wnd = nfui_nfobject_get_window(old_mfocus);
                gdk_window_invalidate_rect(wnd, 0, FALSE);
            }
#endif

            }

            prvSetMouseFocus(active, TRUE);

            top_kobj = nfui_nfwindow_get_keyobj((NFWINDOW*)top);
            if(top_kobj)
            {
                old_kfocus = prvFindCurKFocus(top_kobj);
                act_kfocus = prvFindKeyObjByNfobj(top_kobj, active);

                if(act_kfocus && (old_kfocus != act_kfocus))
                {
                    if(old_kfocus &&
                        (active->type == NFOBJECT_TYPE_NFBUTTON || active->type == NFOBJECT_TYPE_NFCHECKBUTTON || active->type == NFOBJECT_TYPE_NFTHUMBNAIL))
                    {
                        prvClearKeyFocus(old_kfocus);
                        nfui_signal_emit(old_kfocus->object, GDK_EXPOSE, TRUE);
                        prvSetKeyFocusAll(act_kfocus, TRUE);
                    }
                }
            }

#ifdef NF_TOOLTIP_ENABLE
			if(active->use_tooltip && active->type != NFOBJECT_TYPE_NFTAB && active->type != NFOBJECT_TYPE_NFLISTBOX)
            {
                if(tt_info.object == active)
                {
                    if(tt_info.open_src == 0)
                        tt_info.open_src = g_timeout_add(TOOLTIP_DELAY_TIME, tooltip_open, active);
                }
                else
                {
                    if(tt_info.open_src)
                        g_source_remove(tt_info.open_src);

                    tt_info.open_src = g_timeout_add(TOOLTIP_DELAY_TIME, tooltip_open, active);
                }
            }
#endif

            RUN_EVENT_HANDLER(active, event, NULL)
            nfui_nfscrolledscr_draw_drawable(active);
        }
            break;

        case GDK_FOCUS_CHANGE :
            if(((GdkEventFocus*)event)->in == FALSE)
            {
                GdkEvent* evt_cp;

                old_mfocus = prvFindCurMFocus(object);

                if(old_mfocus)
                {
#ifdef NF_TOOLTIP_ENABLE
                    tooltip_close();
#endif
                    evt_cp = gdk_event_new(GDK_LEAVE_NOTIFY);
                    RUN_EVENT_HANDLER(old_mfocus, evt_cp, NULL)
                    gdk_event_free(evt_cp);

                    prvSetMouseFocus(old_mfocus, FALSE);
                }
            }
            else if(((GdkEventFocus*)event)->in == TRUE)
            {
                ;
            }

            break;


        case NFEVENT_JOG_CHANGE:
        case NFEVENT_SHUTTLE_CHANGE:
        {
            PAGEID pid;
            NFOBJECT *dest;

            if(repeat_key)
            {
                g_source_remove(repeat_key);
                repeat_key = 0;
            }

            pid = nfui_get_cur_page(&dest);

            g_assert(dest);

            RUN_EVENT_HANDLER(dest, event, NULL)
        }
        break;

        case NFEVENT_KEYPAD_PRESS:
        case NFEVENT_KEYPAD_RELEASE:
        case NFEVENT_REMOCON_PRESS:
        case NFEVENT_REMOCON_RELEASE:
        {
            NFOBJECT *top;
            GdkEventKey *kevt;
            KEYPAD_KID kpid;
            gint def_ret=0, post_ret=0;

            KEYOBJECT* top_kobject;
            KEYOBJECT* cur_focus;

            static NFOBJECT *kpressed_obj = NULL;

            kevt = (GdkEventKey*)event;
            kpid = (KEYPAD_KID)kevt->keyval;

#if defined(__XRPLUS_UI__)
            if(audio_change_mode == AUDIO_CH_CHANGE_ON)
            {
                if(kpid > KEYPAD_CH04)
                    audio_change_mode = AUDIO_CH_CHANGE_OFF;
            }
#endif

            DMSG(1, "");

			if ((event->type == NFEVENT_KEYPAD_RELEASE || event->type == NFEVENT_REMOCON_RELEASE) && (kpid == KEYPAD_FREEZE)) {
				screen_capture_manual();
			}
			else if ((event->type == NFEVENT_KEYPAD_RELEASE || event->type == NFEVENT_REMOCON_RELEASE) && (kpid == KEYPAD_CH00)) {
				screen_capture_manual();
			}

            if(event->type == NFEVENT_KEYPAD_RELEASE || event->type == NFEVENT_REMOCON_RELEASE)
            {
                if(repeat_key)
                {
                    g_source_remove(repeat_key);
                    repeat_key = 0;
                }
            }

            top = nfui_nfobject_get_top(object);
            if(top == NULL)
            {
              DMSG(1, "top is NULL, top : %p\n", top);
              return FALSE;
            }
            DMSG(1, "");
            top_kobject = nfui_nfwindow_get_keyobj((NFWINDOW*)top);


            if(top_kobject == NULL)
            {
                if(kpressed_obj && (event->type == NFEVENT_KEYPAD_RELEASE || event->type == NFEVENT_REMOCON_RELEASE))
                    nfui_signal_emit(kpressed_obj, event->type, FALSE);
                kpressed_obj = NULL;

                DMSG(1, "top_kobject is NULL, top : %p\n", top);
                return FALSE;
            }
            DMSG(1, "");

            /* Window Close */
            if((event->type == NFEVENT_KEYPAD_PRESS || event->type == NFEVENT_REMOCON_PRESS) && kpid == KEYPAD_EXIT)
            {
                DMSG(1, "");
                if(prvCloseWindow(top_kobject, event))
                    return FALSE;
            }

            if(kpid == KEYPAD_DEBUG)
            {
                System_Debugging_Information_Open(top_kobject);     // jaeyoung
            }

            if((kpid == KEYPAD_ENTER) || (kpid == KEYPAD_EXIT) || (kpid == KEYPAD_UP) || (kpid == KEYPAD_DOWN) || (kpid == KEYPAD_LEFT) || (kpid == KEYPAD_RIGHT))
            {
                cur_focus = prvFindCurKFocus(top_kobject);

                if(cur_focus)   active = cur_focus->object;
                else            active = NULL;

                if((event->type == NFEVENT_KEYPAD_PRESS) || (event->type == NFEVENT_REMOCON_PRESS))
                {
                    kpressed_obj = active;
                }
                else if((event->type == NFEVENT_KEYPAD_RELEASE) || (event->type == NFEVENT_REMOCON_RELEASE))
                {
                    if(kpressed_obj != active)
                    {
                        if(kpressed_obj && (event->type == NFEVENT_KEYPAD_RELEASE || event->type == NFEVENT_REMOCON_RELEASE))
                            nfui_signal_emit(kpressed_obj, event->type, FALSE);
                        kpressed_obj = NULL;

                        return FALSE;
                    }

                    kpressed_obj = NULL;
                }

                // added by hakeya - 2008/12/22
                RUN_EVENT_HANDLER2(top, event, def_ret, post_ret, NULL)

                if(post_ret==TRUE)  return FALSE;

                if(cur_focus)
                {
                    def_ret = 0;
                    post_ret = 0;

                    if(!nfui_nfobject_is_disabled(active))
                    {
                        RUN_EVENT_HANDLER2(active, event, def_ret, post_ret, NULL);
                    }
                }

                if((event->type == NFEVENT_KEYPAD_PRESS || event->type == NFEVENT_REMOCON_PRESS) && (def_ret==FALSE && post_ret==FALSE)) {
                    prvMoveKFocus(top, kpid);
                }
            }
            else
            {
                RUN_EVENT_HANDLER(top, event, NULL)
            }
        }
        break;

        case NFEVENT_IPCAMSETUP_DB_SYNC :
        case NFEVENT_SNMP_RADIO_BUTTON_PRESS :
        case NFEVENT_DEBUG_INFOR_DATA_SYNC:
        case NFEVENT_VCAREV_COMPONENT_DATA_SYNC :
        case NFEVENT_VCAREV_COMPONENT_DATA_EXPOSE :
        case NFEVENT_VCAREV_COMPONENT_PREVIEW_SYNC :
        case NFEVENT_DVA_COMPONENT_DATA_SYNC :
        case NFEVENT_DVA_COMPONENT_DATA_EXPOSE :
        case NFEVENT_DVA_COMPONENT_PREVIEW_SYNC :        
        case NFEVENT_VCA_STATISTIC_COMPONENT_DATA_SYNC :
                prvVisitAllChildren((NFOBJECT*)object, event, FALSE);
            break;

		case NFEVENT_SCROLLED_FIXED_MOVE :		
				prvMoveScrolledFixed((NFOBJECT*)object);
			break;

        case GDK_DELETE :
#ifdef NF_TOOLTIP_ENABLE
            tooltip_close();
#endif

            if(repeat_key)
            {
                g_source_remove(repeat_key);
                repeat_key = 0;
            }

            /* Come from nfwindow.c 2009/04/03 by hakeya*/
            if(object->type == NFOBJECT_TYPE_TOP)
            {
                NFWINDOW *wnd_temp = NULL;
                wnd_temp = (NFWINDOW*)object;

                if(wnd_temp->key_obj)
                    nfui_free_key_hierarchy(wnd_temp->key_obj);
                wnd_temp->key_obj = NULL;
            }
            /* hakeya end */

            prvVisitAllChildren((NFOBJECT*)object, event, TRUE);
            break;

        default:
            break;
    }

    // To prevent gtk event handler excution
    return TRUE;
}




////////////////////////////////////////////////////////////////////////////////////////
// EXTERNAL INTERFACE
void nfui_run_main_event_handler(NFOBJECT *window)
{
    GtkWidget *widget;

    if(repeat_key)
    {
        g_source_remove(repeat_key);
        repeat_key = 0;
    }

    widget = ((NFWINDOW*)window)->main_widget;

    gtk_widget_set_events(widget, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_SCROLL_MASK);
    g_signal_connect(G_OBJECT(widget), "event", G_CALLBACK(nf_main_event_handler), window);
}


void nfui_regi_pre_event_callback(NFOBJECT *obj, gpointer func)
{
    obj->pre_event_handler = func;
}

void nfui_regi_post_event_callback(NFOBJECT *obj, gpointer func)
{
    obj->post_event_handler = func;
}

void nfui_send_event(NFOBJECT *obj, GdkEvent *event, gboolean loop)
{
    GdkEvent *evt;

    evt = gdk_event_copy(event);

    if(loop == TRUE)
    {
        nf_main_event_handler(NULL, evt, obj);
    }
    else
    {
        if(evt->type != GDK_EXPOSE || nfui_nfobject_is_shown(obj))
            RUN_EVENT_HANDLER(obj, evt, NULL);
    }

    gdk_event_free(evt);
}

void nfui_signal_emit(NFOBJECT *obj, GdkEventType type, gboolean loop)
{
    GdkEvent *evt;

    evt = gdk_event_new(type);

    if(loop == TRUE)
    {
        nf_main_event_handler(NULL, evt, obj);
    }
    else
    {
        if(evt->type != GDK_EXPOSE || nfui_nfobject_is_shown(obj)) {
            RUN_EVENT_HANDLER(obj, evt, NULL);
            if(evt->type == GDK_EXPOSE) nfui_nfscrolledscr_draw_drawable(obj);
        }
    }   

    gdk_event_free(evt);
}

void nfui_signal_emit_data(NFOBJECT *obj, IMSG msgid, gboolean loop, gpointer data)
{
    GdkEventAny *evt;

    if (!IS_VALID_OBJECT(obj)) return;
    evt = (GdkEventAny*)gdk_event_new(GDK_NOTHING);
    evt->type = msgid;
    DMSG(9, "OBJ = [%p], MSGID = [0x%08x]\n", obj, msgid);
    DMSG(9, "PRE HANDLER = [%p]\n", obj->pre_event_handler);
    DMSG(9, "DEFAULT HANDLER = [%p]\n", obj->default_event_handler);
    DMSG(9, "POST HANDLER = [%p]\n", obj->post_event_handler);
    RUN_EVENT_HANDLER(obj, evt, data);
    gdk_event_free(evt);
}

void nfui_user_signal_emit(NFOBJECT *obj, nfevent_type type, gboolean loop)
{
    GdkEvent *evt;

    evt = gdk_event_new(GDK_VISIBILITY_NOTIFY);

    evt->type = type;

    if(loop == TRUE)    nf_main_event_handler(NULL, evt, obj);
    else        RUN_EVENT_HANDLER(obj, evt, NULL);

    evt->type = GDK_VISIBILITY_NOTIFY;

    gdk_event_free(evt);
}


void nfui_left_button_signal_emit(NFOBJECT *obj, GdkEventType type, gboolean loop)
{
    GdkEvent *evt;

    evt = gdk_event_new(type);
    evt->button.button = MOUSE_LEFT_BUTTON;

    if(loop == TRUE)    nf_main_event_handler(NULL, evt, obj);
    else        RUN_EVENT_HANDLER(obj, evt, NULL);

    gdk_event_free(evt);
}

void nfui_key_signal_emit(NFOBJECT *obj, nfevent_type type, KEYPAD_KID kid, gboolean loop)
{
    GdkEventKey *evt = NULL;

    if(type == NFEVENT_KEYPAD_PRESS) {
        evt = gdk_event_new(GDK_KEY_PRESS);
        evt->type = type;
        evt->keyval = kid;
    }

    if(loop == TRUE) nf_main_event_handler(NULL, evt, obj);
    else             RUN_EVENT_HANDLER(obj, evt, NULL);

    gdk_event_free(evt);
}


/****************************************************************
 * KEY FOCUS RELATED...
 * *************************************************************/

static NFOBJECT* prvIsChildOfContainer(NFOBJECT* object)
{
    NFOBJECT* obj_temp = NULL;

    if(!object) return NULL;

    obj_temp = object;

    while(obj_temp)
    {
        if(obj_temp->parent)    obj_temp = obj_temp->parent;
        else    return NULL;

        switch(obj_temp->type)
        {
            case NFOBJECT_TYPE_NFCALENDAR:
            case NFOBJECT_TYPE_NFCOMBOBOX:
            case NFOBJECT_TYPE_NFLISTBOX:
            case NFOBJECT_TYPE_NFTIMESPIN:
            case NFOBJECT_TYPE_NFTIMELABEL :
            case NFOBJECT_TYPE_NFSPINBUTTON:
                return obj_temp;

            default:
                break;
        }
    }

    return NULL;
}

static void prvMakeKFocusTree(NFOBJECT *obj, KEYOBJECT *pko, gboolean only_children)
{
    NFOBJECT *child;
    KEYOBJECT *key_obj = NULL;
    guint i, child_num;

    g_assert(obj);

    if(obj->use_focus == NFOBJECT_FOCUS_ON && nfui_nfobject_is_shown(obj) && !only_children)
    {
        key_obj = imalloc(sizeof(KEYOBJECT));
        key_obj->parent = pko;
        key_obj->object = obj;
        key_obj->depth = pko->depth + 1;
        key_obj->fx = -1;
        key_obj->fy = -1;

        pko->children = g_slist_append(pko->children, key_obj);

    }
    else
    {
        key_obj = pko;

        switch(obj->type)
        {
            case NFOBJECT_TYPE_TOP:
                child = ((NFWINDOW*)obj)->child;
                prvMakeKFocusTree(child, key_obj, FALSE);
                break;

            case NFOBJECT_TYPE_NFCALENDAR :
            case NFOBJECT_TYPE_NFCOMBOBOX :
            case NFOBJECT_TYPE_NFLISTBOX :
            case NFOBJECT_TYPE_NFTIMESPIN :
            case NFOBJECT_TYPE_NFTIMELABEL :
            case NFOBJECT_TYPE_NFSPINBUTTON :
                if(!only_children)
                    break;

            case NFOBJECT_TYPE_NFFIXED :
            case NFOBJECT_TYPE_NFTABLE :
            {
                child_num = g_slist_length(((NFCONTAINER*)obj)->children);

                for(i=0; i<child_num; i++)
                {
                    child = g_slist_nth_data(((NFCONTAINER*)obj)->children, i);
                    g_assert(child);

                    prvMakeKFocusTree(child, key_obj, FALSE);
                }
            }
            break;

			case NFOBJECT_TYPE_NFSCROLLEDFIXED :			
			{
				child_num = g_slist_length(((NFSCROLLEDFIXED*)obj)->children);

				for(i=0; i<child_num; i++)
				{
					child = g_slist_nth_data(((NFSCROLLEDFIXED*)obj)->children, i);
					g_assert(child);

					if ((child->x < 0) || (child->y < 0)) continue;
					if ((child->type != NFOBJECT_TYPE_NFTABLE) 
					    && (child->type != NFOBJECT_TYPE_NFFIXED)
					    && (child->type != NFOBJECT_TYPE_NFTILE)) 
				    {
					    if ((child->x + child->width > obj->width) || (child->y + child->height > obj->height)) continue;
				    }

					prvMakeKFocusTree(child, key_obj, FALSE);
				}
			}
			break;

            default :
                break;
        }
    }
}

static void prvGetTabCountOfChild(KEYOBJECT *kobj, KEYOBJECT **tab, gint *tab_count)
{
    KEYOBJECT *child = NULL;
    gint child_num;
    gint i;

    if(!kobj)   return;

    child_num = g_slist_length(kobj->children);

    for(i=0; i<child_num; i++)
    {
        child = (KEYOBJECT*)g_slist_nth_data(kobj->children, i);
        g_assert(child);

        if(child->object->type == NFOBJECT_TYPE_NFTAB)
        {
            tab[*tab_count] = child;
            (*tab_count)++;
        }

        prvGetTabCountOfChild(child, tab, tab_count);
    }
}

static void prvAddChildren(KEYOBJECT *kobj)
{
    NFOBJECT *cur_page = NULL;
    NFOBJECT *obj_temp = NULL;
    KEYOBJECT *parent = NULL;
    KEYOBJECT *child = NULL;
    gint child_num;
    gint is_child_of_tab;
    gint i;


    if(!kobj)   return;
    if(!kobj->object)   return;
    if(kobj->object->type != NFOBJECT_TYPE_NFTAB)   return;

    cur_page = ((NFTAB*)kobj->object)->page[((NFTAB*)kobj->object)->cur_page];
    parent = kobj->parent;

    child_num = g_slist_length(parent->children);

    for(i=0; i<child_num; i++)
    {
        child = g_slist_nth_data(parent->children, i);
        if(!child)
        {
            DMSG(1, "No child!!");
            break;
        }

        if(child == kobj)   continue;

        obj_temp = child->object;
        is_child_of_tab = 0;

        while(obj_temp->type != NFOBJECT_TYPE_TOP)
        {
            if(obj_temp == cur_page)
            {
                is_child_of_tab = 1;
                break;
            }

            obj_temp = obj_temp->parent;
        }

        if(is_child_of_tab)
        {
            parent->children = g_slist_remove(parent->children, child);
            child->parent = kobj;
            kobj->children = g_slist_append(kobj->children, child);

            i--;
            child_num--;
        }
    }
}

static void prvReassignDepth(KEYOBJECT *kobj)
{
    KEYOBJECT *child = NULL;
    gint child_num;
    gint i;

    child_num = g_slist_length(kobj->children);

    for(i=0; i<child_num; i++)
    {
        child = (KEYOBJECT*)g_slist_nth_data(kobj->children, i);
        g_assert(child);

        child->depth = kobj->depth + 1;

        prvReassignDepth(child);
    }
}

static void prvRealignKFocusByTab(KEYOBJECT *pko)
{
    NFOBJECT *obj = NULL;
    KEYOBJECT *kobj = NULL;
    KEYOBJECT *parent = NULL;
    KEYOBJECT *child = NULL;
    KEYOBJECT *found_tabs[8];
    gint child_num = 0;;
    gint tab_count = 0;
    gint i;

    if(!pko)    return;

    memset(found_tabs, 0, sizeof(found_tabs));

    prvGetTabCountOfChild(pko, found_tabs, &tab_count);

    for(i=0; i<tab_count; i++)
    {
        prvAddChildren(found_tabs[i]);
    }
    prvReassignDepth(pko);
}

static KEYOBJECT* prvFindCurKFocus(KEYOBJECT* top_kobject)
{
    KEYOBJECT *temp;
    KEYOBJECT *child;
    guint i, child_num;


    temp = top_kobject;

    if(temp->depth != 0)
    {
        while(temp->parent)
        {
            temp = temp->parent;
        }

        if(temp->depth != 0)
        {
            DMSG(1, "%s:%d KEYOBJECT is not top!!!", __FILE__, __LINE__);
            return NULL;
        }
    }

    while(1)
    {
        child_num = g_slist_length(temp->children);

        for(i=0; i<child_num; i++)
        {
            child = (KEYOBJECT*)g_slist_nth_data(temp->children, i);
            g_assert(child);

            if(child->object->kfocus == NFOBJECT_FOCUS)
            {
                temp = child;
                break;
            }
        }

        if(i == child_num)
        {
            if(temp->depth == 0)
            {

                return NULL;
            }
            else
            {
                return temp;
            }
        }
    }

    return NULL;
}

static KEYOBJECT* prvFindKeyObjByNfobj(KEYOBJECT* kobj, NFOBJECT* obj)
{
    KEYOBJECT *child = NULL;
    KEYOBJECT *ret = NULL;
    gint child_num = 0;
    gint i = 0;


    if(kobj->object == obj)
        return kobj;

    child_num = g_slist_length(kobj->children);

    for(i=0; i<child_num; i++)
    {
        child = (KEYOBJECT*)g_slist_nth_data(kobj->children, i);
        g_assert(child);

        ret = prvFindKeyObjByNfobj(child, obj);

        if(ret) return ret;
    }

    return NULL;
}

static KEYOBJECT* prvGetKFocusFromParents(NFOBJECT* obj)
{
    NFOBJECT* obj_temp = NULL;
    NFOBJECT* top_obj = NULL;
    KEYOBJECT* top_kobj = NULL;
    KEYOBJECT* ret_kobj = NULL;

    obj_temp = prvIsChildOfContainer(obj);

    if(obj_temp == NULL)
        return NULL;

    top_obj = nfui_nfobject_get_top(obj);
    g_assert(top_obj);

    top_kobj = nfui_nfwindow_get_keyobj((NFWINDOW*)top_obj);

    ret_kobj = prvFindKeyObjByNfobj(top_kobj, obj_temp);

    return ret_kobj;
}

static void prvSetKeyFocus(KEYOBJECT *key_obj, gboolean focus)
{
    KEYOBJECT* temp;
    gint flag;


    if(key_obj == NULL)
    {
        return;
    }

    temp = key_obj;
    flag = 0;

    while(1)
    {
        g_assert(temp);

        if(temp->depth == 0)
            break;

        if(focus)
        {
            temp->object->kfocus = NFOBJECT_FOCUS;
        }
        else
        {
            temp->object->kfocus = NFOBJECT_UNFOCUS;

            switch(temp->object->type)
            {
                case NFOBJECT_TYPE_NFCALENDAR :
                case NFOBJECT_TYPE_NFCOMBOBOX :
                case NFOBJECT_TYPE_NFLISTBOX :
                case NFOBJECT_TYPE_NFSPINBUTTON :
                case NFOBJECT_TYPE_NFTIMESPIN :
                case NFOBJECT_TYPE_NFTIMELABEL :
                    prvFreeKObjList(temp, FALSE);
                    flag = 1;
                    break;

                default:
                    break;
            }

            if(flag)    break;
        }

        temp = temp->parent;
    }

}

static void prvSetKeyFocusAll(KEYOBJECT *key_obj, gboolean focus)
{
    KEYOBJECT *temp = NULL;
    gint col=0, row=0;
    gint col_cnt=0, row_cnt=0;

    if(key_obj == NULL)
        return;

    temp = key_obj;

    if(focus)
    {
        KEYOBJECT *parent = NULL;
        gint find = 0;

        g_assert(temp);

        while(temp->depth>0)
        {
            parent = temp->parent;
            temp->object->kfocus = NFOBJECT_FOCUS;

            col_cnt = parent->cnt_x;
            row_cnt = parent->cnt_y;

            find = 0;

            for(col=0; col<col_cnt; col++)
            {
                for(row=0; row<row_cnt; row++)
                {
                    if(parent->nav_table[row][col] == temp)
                    {
                        find = 1;
                        break;
                    }
                }
                if(find)    break;
            }

            if(!find || col==col_cnt || row==row_cnt)
            {
                DMSG(1, "(key navi) Parent has no [0x%d] key object.", temp);
                return;
            }

            parent->fx = col;
            parent->fy = row;

            temp = parent;
        }
    }
    else
    {
        gint fx, fy;
        gint flag;

        flag = 0;

        while(1)
        {
            if(temp == NULL)
                break;

            if(temp->depth)
            {
                temp->object->kfocus = NFOBJECT_UNFOCUS;

                switch(temp->object->type)
                {
                    case NFOBJECT_TYPE_NFCALENDAR :
                    case NFOBJECT_TYPE_NFCOMBOBOX :
                    case NFOBJECT_TYPE_NFLISTBOX :
                    case NFOBJECT_TYPE_NFSPINBUTTON :
                    case NFOBJECT_TYPE_NFTIMESPIN :
                    case NFOBJECT_TYPE_NFTIMELABEL :
                        prvFreeKObjList(temp, FALSE);
                        flag = 1;
                        break;

                    default:
                        break;
                }
            }

            if(flag)    break;


            if(temp->fy < 0 || temp->fx < 0)
                break;

            fx = temp->fx;
            fy = temp->fy;

            temp->fy = -1;
            temp->fx = -1;

            temp = temp->nav_table[fy][fx];
        }
    }
}

static void prvClearKeyFocus(KEYOBJECT *key_obj)
{
    KEYOBJECT* temp;

    temp = key_obj;

    if(temp->depth > 0)
    {
        while(temp->parent)
        {
            temp = temp->parent;

            if(temp==NULL)
                break;
        }

        if(temp->depth != 0)
        {
            DMSG(1, "(key navi) %s:%d KEYOBJECT is not top!!!", __FILE__, __LINE__);
            return;
        }
    }
    prvSetKeyFocusAll(temp, FALSE);
}

static gint prvSelectEdge(KEYOBJECT *kobj, gint *fx, gint *fy, KEYPAD_KID dir)
{
    gint left, right, top, bottom;
    gint fx_temp, fy_temp;
    gint size = 0;
    gint i;

    fx_temp = kobj->fx;
    fy_temp = kobj->fy;

    for(i=fx_temp; i>0; i--)
    {
        if(kobj->nav_table[fy_temp][i] != kobj->nav_table[fy_temp][i-1])
            break;
    }
    left = i;

    for(i=fx_temp; i<kobj->cnt_x-1; i++)
    {
        if(kobj->nav_table[fy_temp][i] != kobj->nav_table[fy_temp][i+1])
            break;
    }
    right = i;

    for(i=fy_temp; i>0; i--)
    {
        if(kobj->nav_table[i][fx_temp] != kobj->nav_table[i-1][fx_temp])
            break;
    }
    top = i;

    for(i=fy_temp; i<kobj->cnt_y-1; i++)
    {
        if(kobj->nav_table[i][fx_temp] != kobj->nav_table[i+1][fx_temp])
            break;
    }
    bottom = i;


    if(dir==KEYPAD_LEFT || dir==KEYPAD_UP || dir==KEYPAD_DOWN)
    {
        *fx = left;
    }
    else if(dir==KEYPAD_RIGHT)
    {
        *fx = right;
    }

    if(dir==KEYPAD_LEFT || dir==KEYPAD_RIGHT || dir==KEYPAD_UP)
    {
        *fy = top;
    }
    else if(dir==KEYPAD_DOWN)
    {
        *fy = bottom;
    }

    if(dir==KEYPAD_LEFT || dir==KEYPAD_RIGHT)
        size = bottom-top+1;
    else if(dir==KEYPAD_UP || dir==KEYPAD_DOWN)
        size = right-left+1;

    return size;
}

static void prvSetDefaultFocus(KEYOBJECT* top_kobject)
{
    KEYOBJECT* new_focus = NULL;
    NFOBJECT* mfocus = NULL;
    gint x, y;

    if(top_kobject->children == NULL)
        return;

    for(x=0; x<KEYNAV_TABLE_COLUMNS; x++)
    {
        for(y=0; y<KEYNAV_TABLE_ROWS; y++)
        {
            new_focus = top_kobject->nav_table[y][x];

            if(new_focus)   break;
        }

        if(new_focus)   break;
    }

    if(new_focus == NULL)
        return;

    mfocus = prvFindCurMFocus(new_focus->object);

    if(mfocus)
    {
#ifdef NF_TOOLTIP_ENABLE
        tooltip_close();
#endif
        nfui_signal_emit(mfocus, GDK_LEAVE_NOTIFY, TRUE);
    }

    top_kobject->fx = x;
    top_kobject->fy = y;

    prvSetKeyFocus(new_focus, TRUE);

    nfui_user_signal_emit(new_focus->object, NFEVENT_KEYFOCUS_CHANGED, FALSE);
    nfui_signal_emit(new_focus->object, GDK_EXPOSE, TRUE);
}


static gint prvKeyMoveLeftRight(KEYOBJECT *parent, gint *find_x, gint *find_y,  KEYPAD_KID dir)
{
    gint fx, fy;
    gint cnt_x, cnt_y;
    gint size;

    gint x, y;
    gint count;

    gint is_found;

    is_found = 0;

    cnt_x = parent->cnt_x;
    cnt_y = parent->cnt_y;

    size = prvSelectEdge(parent, &fx, &fy, dir);

    if((fx < 0) || (fx >= cnt_x))   return 0;

    count = 0;
    x = fx;

    while(count < cnt_x)
    {
        if(dir == KEYPAD_LEFT)
        {
            x--;
            if(x < 0)   x = cnt_x-1;
        }
        else if(dir == KEYPAD_RIGHT)
        {
            x++;
            if(x >= cnt_x)  x = 0;
        }

        for(y=fy; y<fy+size; y++)
        {
            if(parent->nav_table[y][x])
            {
                if ((parent->nav_table[y][x]->object->status == NFOBJECT_STATE_DISABLE) ||
                    (parent->nav_table[y][x]->object->type == NFOBJECT_TYPE_IXTIMELINE) ||
                    (parent->nav_table[y][x]->object->type == NFOBJECT_TYPE_CWSLIDER) ||
                    (parent->nav_table[y][x]->object->use_hierarchy == NFOBJECT_HIERARCHY_OFF))
                    continue;

                is_found = 1;
                *find_x = x;
                *find_y = y;

                break;
            }
        }

        if(is_found)    return 1;

        count++;
    }

    return 0;
}

static gint prvKeyMoveUpDown(KEYOBJECT *parent, gint *find_x, gint *find_y,  KEYPAD_KID dir)
{
    gint fx, fy;
    gint cnt_x, cnt_y;
    gint size;

    gint y;
    gint count;
    gint offset;
    gint pos1, pos2;
    gint size1, size2;
    gint rblock, lblock;
    gint max;

    gint is_found;

    cnt_x = parent->cnt_x;
    cnt_y = parent->cnt_y;

    size = prvSelectEdge(parent, &fx, &fy, dir);

    if((fy < 0) || (fy >= cnt_y))   return 0;

    count = 0;
    pos1 = fx;
    pos2 = fx + size;
    size1 = fx;
    size2 = cnt_x - fx - size;
    y = fy;

    is_found = 0;

    while(count < cnt_y)
    {

        if(dir == KEYPAD_UP)
        {
            y--;
            if(y < 0)   y = cnt_y - 1;
        }
        else if(dir == KEYPAD_DOWN)
        {
            y++;
            if(y >= cnt_y)  y = 0;
        }

        rblock = 0;
        lblock = 0;

        if(size1 > size)    max = size1;
        else                max = size;

        if(max < size2)     max = size2;

        for(offset=0; offset<max; offset++)
        {
            if(offset < size)
            {
                if(parent->nav_table[y][pos1+offset])
                {
                    if ((parent->nav_table[y][pos1+offset]->object->status != NFOBJECT_STATE_DISABLE) &&
                        (parent->nav_table[y][pos1+offset]->object->type != NFOBJECT_TYPE_IXTIMELINE) &&
                        (parent->nav_table[y][pos1+offset]->object->type != NFOBJECT_TYPE_CWSLIDER) &&
                        (parent->nav_table[y][pos1+offset]->object->use_hierarchy != NFOBJECT_HIERARCHY_OFF))
                    {
                        is_found = 1;
                        *find_x = pos1+offset;
                        *find_y = y;

                        break;
                    }
                }

            }

            if(pos1-offset-1 >= 0 && !lblock)
            {
                if(parent->nav_table[y][pos1-offset-1])
                {
                    if((parent->nav_table[y][pos1-offset-1]->object->status != NFOBJECT_STATE_DISABLE) &&
                        (parent->nav_table[y][pos1-offset-1]->object->type != NFOBJECT_TYPE_IXTIMELINE) &&
                        (parent->nav_table[y][pos1-offset-1]->object->type != NFOBJECT_TYPE_CWSLIDER) &&
                        (parent->nav_table[y][pos1-offset-1]->object->use_hierarchy != NFOBJECT_HIERARCHY_OFF))
                    {
                        gint tmpfind;
                        gint tmpx, tmpy;

                        tmpfind = prvKeyMoveLeftRight(parent, &tmpx, &tmpy, KEYPAD_LEFT);

                        if(tmpfind && (parent->nav_table[y][pos1-offset-1]->object == parent->nav_table[tmpy][tmpx]->object))
                        {
                            lblock = 1;
                        }
                        else
                        {
                            is_found = 1;
                            *find_x = pos1-offset-1;
                            *find_y = y;
                        }

                        break;
                    }
                }
            }

            if(pos2+offset < cnt_x && !rblock)
            {
                if(parent->nav_table[y][pos2+offset])
                {
                    if ((parent->nav_table[y][pos2+offset]->object->status != NFOBJECT_STATE_DISABLE) &&
                        (parent->nav_table[y][pos2+offset]->object->type != NFOBJECT_TYPE_IXTIMELINE) &&
                        (parent->nav_table[y][pos2+offset]->object->type != NFOBJECT_TYPE_CWSLIDER) &&
                        (parent->nav_table[y][pos2+offset]->object->use_hierarchy != NFOBJECT_HIERARCHY_OFF))
                    {
                        gint tmpfind;
                        gint tmpx, tmpy;

                        tmpfind = prvKeyMoveLeftRight(parent, &tmpx, &tmpy, KEYPAD_RIGHT);

                        if(tmpfind && (parent->nav_table[y][pos2+offset]->object == parent->nav_table[tmpy][tmpx]->object))
                        {
                            rblock = 1;
                        }
                        else
                        {
                            is_found = 1;
                            *find_x = pos2+offset;
                            *find_y = y;
                        }

                        break;
                    }
                }
            }
        }

        if(is_found)    return 1;

        count++;
    }

    return 0;
}


static void prvMoveKFocus(NFOBJECT* top, KEYPAD_KID dir)
{
    KEYOBJECT* top_kobject;

    KEYOBJECT* parent;
    KEYOBJECT* cur_focus;
    KEYOBJECT* new_focus;
    NFOBJECT* mfocus = NULL;
    gint fx, fy;
    gint cnt_x, cnt_y;
    gint size;

    gint x, y;
    gint is_found;


    gint find_x, find_y;
    gint count;
    gint offset;
    gint pos1, pos2;
    gint size1, size2;

    if(!nfui_nfobject_is_shown(top))
        return;

    top_kobject = nfui_nfwindow_get_keyobj((NFWINDOW*)top);

    if(top_kobject == NULL)
        return;

    cur_focus = prvFindCurKFocus(top_kobject);

    if(cur_focus == NULL)
    {
        prvSetDefaultFocus(top_kobject);
        return;
    }

    parent = cur_focus->parent;

    is_found = 0;

    switch(dir)
    {
        case KEYPAD_UP:
            is_found = prvKeyMoveUpDown(parent, &find_x, &find_y, KEYPAD_UP);
            break;

        case KEYPAD_DOWN:
            is_found = prvKeyMoveUpDown(parent, &find_x, &find_y, KEYPAD_DOWN);
            break;

        case KEYPAD_LEFT:
            is_found = prvKeyMoveLeftRight(parent, &find_x, &find_y,  KEYPAD_LEFT);
            break;

        case KEYPAD_RIGHT:
            is_found = prvKeyMoveLeftRight(parent, &find_x, &find_y,  KEYPAD_RIGHT);
            break;

        case KEYPAD_ENTER:
            if(!cur_focus->children)
            {
                prvMakeKFocusTree(cur_focus->object, cur_focus, TRUE);
                prvRealignKFocusByTab(cur_focus);
                prvMakeNavTable(cur_focus, NULL);
            }
//          else if(cur_focus->object->type == NFOBJECT_TYPE_NFTAB)
//          {
//              prvMakeNavTable(cur_focus, NULL);
//          }

            for(y=0; y<KEYNAV_TABLE_ROWS; y++)
            {
                for(x=0; x<KEYNAV_TABLE_COLUMNS; x++)
                {
                    new_focus = cur_focus->nav_table[y][x];

                    if (new_focus) {
                        switch (cur_focus->object->type) {
                        case NFOBJECT_TYPE_NFSPINBUTTON:
                            if (new_focus->object->type != NFOBJECT_TYPE_NFLABEL) continue;
                            break;
                        }
                    }


//                  if(new_focus)
                    if(new_focus && !nfui_nfobject_is_disabled(new_focus->object))
                        break;
                }

                if(new_focus)
                    break;
            }


            if(new_focus == NULL)
                return;

//          cur_focus->fx = 0;
//          cur_focus->fy = 0;
            cur_focus->fx = x;
            cur_focus->fy = y;

            prvSetKeyFocus(new_focus, TRUE);

            nfui_user_signal_emit(new_focus->object, NFEVENT_KEYFOCUS_CHANGED, FALSE);
#if 1
            nfui_signal_emit(new_focus->object->parent, GDK_EXPOSE, TRUE);
#else
            nfui_signal_emit(new_focus->object, GDK_EXPOSE, TRUE);
#endif

            return;

        case KEYPAD_EXIT:
            {
                NFOBJECT *cur_obj = NULL;

                new_focus = cur_focus->parent;
                cur_obj = cur_focus->object;

                if(new_focus == NULL || new_focus->depth==0)
                    return;

                prvClearKeyFocus(cur_focus);
                prvSetKeyFocusAll(new_focus, TRUE);

                nfui_signal_emit(cur_obj, GDK_EXPOSE, TRUE);
                nfui_user_signal_emit(new_focus->object, NFEVENT_KEYFOCUS_CHANGED, FALSE);
                nfui_signal_emit(new_focus->object, GDK_EXPOSE, TRUE);

            }
            return;

        default :
            break;
    }

    if(is_found)
        new_focus = cur_focus->parent->nav_table[find_y][find_x];
    else
        new_focus = NULL;


    if(new_focus==NULL || (cur_focus == new_focus))
    {
        if(repeat_key)
        {
            g_source_remove(repeat_key);
            repeat_key = 0;
        }

        nfui_signal_emit(cur_focus->object, GDK_EXPOSE, TRUE);
        return;
    }

    cur_focus->parent->fx = find_x;
    cur_focus->parent->fy = find_y;

    if(new_focus)
    {
        mfocus = prvFindCurMFocus(new_focus->object);
        if(mfocus)
        {
#ifdef NF_TOOLTIP_ENABLE
            tooltip_close();
#endif
            nfui_signal_emit(mfocus, GDK_LEAVE_NOTIFY, TRUE);
        }

        if(cur_focus)
        {
            prvSetKeyFocus(cur_focus, FALSE);
            nfui_signal_emit(cur_focus->object, GDK_EXPOSE, TRUE);
        }

        prvSetKeyFocus(new_focus, TRUE);
        nfui_user_signal_emit(new_focus->object, NFEVENT_KEYFOCUS_CHANGED, FALSE);
        nfui_signal_emit(new_focus->object, GDK_EXPOSE, TRUE);
    }

    if(dir == KEYPAD_LEFT || dir == KEYPAD_RIGHT || dir == KEYPAD_UP || dir == KEYPAD_DOWN)
    {
        if(!repeat_key)
            repeat_key = g_timeout_add(KEY_REPEAT_INTERVAL * 3, repeat_key_proc, GUINT_TO_POINTER(dir));
    }
}

static void prvFreeNavTable(KEYOBJECT *key_obj)
{
    gint x, y;

    if(!key_obj)    return;

    for(y=0; y<KEYNAV_TABLE_ROWS; y++)
    {
        for(x=0; x<KEYNAV_TABLE_COLUMNS; x++)
        {
            key_obj->nav_table[y][x] = NULL;
        }
    }

    key_obj->fx = -1;
    key_obj->fy = -1;
}

static void prvFreeKObjList(KEYOBJECT *key_obj, gboolean self)
{
    KEYOBJECT *temp;
    KEYOBJECT *child;

    guint i, num;

    if(key_obj == NULL)
        return;

    if(key_obj->children)
    {
        num = g_slist_length(key_obj->children);

        for(i=0; i<num; i++)
        {
            child = g_slist_nth_data(key_obj->children, i);
            g_assert(child);

//          DMSG(1, "child[0x%x], child->depth[%d], child->object[0x%x] child->obj->type[%d]\n", child, child->depth, child->object, child->object->type);

            child->object->kfocus = NFOBJECT_UNFOCUS;
            prvFreeKObjList(child, TRUE);
        }

        g_slist_free(key_obj->children);
        key_obj->children = NULL;
    }

    if(self)
    {
        key_obj->object = 0;
        ifree(key_obj);
        key_obj = NULL;
    }
    else
    {
        prvFreeNavTable(key_obj);
    }

}


static void prvSortWidgetInfoByX(WIDGET_INFO *wi, guint size)
{
    WIDGET_INFO temp;
    guint i, j;

    guint y;
    guint temp1, temp2;


    for(i=0; i<size-1; i++)
    {
        for(j=i+1; j<size; j++)
        {
            nfui_nfobject_get_offset(wi[i].kobj->object, &temp1, &y);
            nfui_nfobject_get_offset(wi[j].kobj->object, &temp2, &y);

            if(temp1 > temp2)
            {
                temp = wi[i];
                wi[i] = wi[j];
                wi[j] = temp;
            }
        }
    }
}

static void prvFillWidgetInfo(WIDGET_INFO *wi, guint size)
{
    guint unit_x, unit_y;
    guint off_x, off_y;
    gint temp;
    gint i;

    NFOBJECT *obj = NULL;
    gint a, b;

#if 1
    unit_x = UNIT_PIX_X;
    unit_y = UNIT_PIX_Y;
#else

    if(wi[0].kobj)
    {
        if(wi[0].kobj->depth <= 1)
        {
            unit_x = UNIT_PIX_X;
            unit_y = UNIT_PIX_Y;
        }
        else
        {
            a = wi[0].kobj->parent->object->width;
            b = wi[0].kobj->parent->object->height;

            if(a <= KEYNAV_TABLE_COLUMNS)   unit_x = 1;
            else    unit_x = a / KEYNAV_TABLE_COLUMNS + 1;

            if(b <= KEYNAV_TABLE_ROWS)  unit_y = 1;
            else    unit_y = b / KEYNAV_TABLE_ROWS + 1;
        }
    }
    else
    {
        unit_x = UNIT_PIX_X;
        unit_y = UNIT_PIX_Y;
    }
    #endif

    for(i=0; i<size; i++)
    {
        obj = wi[i].kobj->object;

        nfui_nfobject_get_offset(obj, &off_x, &off_y);

        wi[i].start_x = off_x / unit_x;
        wi[i].start_y = off_y / unit_y;

        wi[i].end_x = (off_x + obj->width) / unit_x;
        wi[i].end_y = (off_y + obj->height) / unit_y;

        temp = (off_x + obj->width) % unit_x;

        if((wi[i].start_x < wi[i].end_x))// && (temp < (unit_x*2/3)))
            (wi[i].end_x)--;
        if(wi[i].start_y < wi[i].end_y)
            (wi[i].end_y)--;
    }
}

static void prvFillNavTable(KEYOBJECT *kobj, WIDGET_INFO *wi, guint size)
{
    guint i;
    guint x, y;


    for(i=0; i<size; i++)
    {
        for(x=wi[i].start_x; x<=wi[i].end_x; x++)
        {
            for(y=wi[i].start_y; y<=wi[i].end_y; y++)
            {
                // ksi_test prvFillNavTable():3442 kobj 0x7ec06ec5b0 y 148598209 x 61187495
                // Got signal 11 [SIGSEGV]
                #if 1
                if (x > 120 || y > 80) {
                    return;
                }
                #endif
                if(kobj->nav_table[y][x])
                    DMSG(1, "(key navi) Navigation table[%d][%d] is no empty!!", y, x);

                kobj->nav_table[y][x] = wi[i].kobj;
            }
        }
    }

    kobj->cnt_x = KEYNAV_TABLE_COLUMNS;
    kobj->cnt_y = KEYNAV_TABLE_ROWS;

    kobj->fx = -1;
    kobj->fy = -1;

}

static void prvMakeNavTable(KEYOBJECT *keyobject, NFOBJECT* prev_nfobj)
{
    WIDGET_INFO wi[WIDGET_MAX];
    KEYOBJECT *child_kobj = NULL;
    gint child_num;
    gint shown_num;
    gint i;


    if(keyobject == NULL)
        return;

    child_num = g_slist_length(keyobject->children);
    shown_num = 0;

    if(child_num == 0)
        return;

    if(child_num > WIDGET_MAX) {
        g_error("%s : child counts > WIDGET_INFO counts ", __FUNCTION__);
        return;
    }

    memset(wi, 0, sizeof(wi));

    for(i=0; i<WIDGET_MAX; i++)
    {
        wi[i].start_x = -1;
        wi[i].start_y = -1;
        wi[i].end_x = -1;
        wi[i].end_y = -1;
    }


    for(i=0; i<child_num; i++)
    {
        child_kobj = g_slist_nth_data(keyobject->children, i);
        g_assert(child_kobj);

        if(!nfui_nfobject_is_shown(child_kobj->object))
            continue;

        prvMakeNavTable(child_kobj, NULL);

        wi[i].kobj = child_kobj;
        shown_num++;
    }

    prvSortWidgetInfoByX(wi, shown_num);
    prvFillWidgetInfo(wi, shown_num);
    prvFillNavTable(keyobject, wi, shown_num);

    if(prev_nfobj)
        nfui_set_key_focus(prev_nfobj, TRUE);
}


int nfui_make_key_hierarchy(NFWINDOW *top)
{
    KEYOBJECT *key_obj = NULL;
    KEYOBJECT *prev_kobj = NULL;
    NFOBJECT *prev_nfobj = NULL;


    key_obj = nfui_nfwindow_get_keyobj((NFWINDOW*)top);

    if(key_obj)
    {
        prev_kobj = prvFindCurKFocus(key_obj);

        if(prev_kobj)
        {
            if(prev_kobj->object)   prev_nfobj = prev_kobj->object;
        }


        prvFreeKObjList(key_obj, TRUE);


        key_obj = NULL;
        prev_kobj = NULL;
    }

    key_obj = imalloc(sizeof(KEYOBJECT));
    if(!key_obj)
    {
        DMSG(1, "%s:%d memory allocate fail..", __FILE__, __LINE__);
        return -1;
    }
    nfui_nfwindow_set_keyobj(top, key_obj);


    key_obj->object = NULL;
    key_obj->parent = NULL;
    key_obj->depth = 0;
    key_obj->fx = -1;
    key_obj->fy = -1;

    prvMakeKFocusTree(top, key_obj, FALSE);
    prvRealignKFocusByTab(key_obj);
    prvMakeNavTable(key_obj, prev_nfobj);


    return 0;
}

void nfui_free_key_hierarchy(KEYOBJECT* key_obj)
{

    prvFreeKObjList(key_obj, TRUE);

}

int nfui_set_key_focus(NFOBJECT *obj, gboolean set)
{
    NFOBJECT* top = NULL;
    KEYOBJECT* top_kobj = NULL;
    KEYOBJECT* key_obj = NULL;

    top = nfui_nfobject_get_top(obj);
    top_kobj = nfui_nfwindow_get_keyobj((NFWINDOW*)top);

    if(top_kobj == NULL)
        return -1;

    key_obj = prvFindKeyObjByNfobj(top_kobj, obj);

    if(key_obj)
    {
        if(set) prvSetKeyFocusAll(key_obj, TRUE);
        else    prvSetKeyFocusAll(key_obj, FALSE);
    }
    else return -2;

    return 0;
}

void nfui_clear_key_focus(NFOBJECT *obj)
{
    NFOBJECT* top = NULL;
    KEYOBJECT* top_kobj = NULL;

    top = nfui_nfobject_get_top(obj);

    if(top)         top_kobj = nfui_nfwindow_get_keyobj((NFWINDOW*)top);
    if(top_kobj)    prvClearKeyFocus(top_kobj);
}

gboolean nfui_is_focus_at_child(NFOBJECT* obj)
{
    NFOBJECT* top = NULL;

    KEYOBJECT* top_kobj = NULL;
    KEYOBJECT* kobj = NULL;
    KEYOBJECT* temp = NULL;

    top = nfui_nfobject_get_top(obj);
    if(!top)    return FALSE;

    top_kobj = nfui_nfwindow_get_keyobj((NFWINDOW*)top);
    if(!top_kobj)   return FALSE;

    kobj = prvFindKeyObjByNfobj(top_kobj, obj);
    if(!kobj)   return FALSE;

    if(kobj->fx<0 && kobj->fy<0)
        return FALSE;

    if(!kobj->nav_table[kobj->fy][kobj->fx])
        return FALSE;

    if(kobj->nav_table[kobj->fy][kobj->fx]->object->kfocus != NFOBJECT_FOCUS)
        return FALSE;

    temp = kobj;
    while(temp->depth)
    {
        if(temp->object->kfocus != NFOBJECT_FOCUS)
            break;

        temp = temp->parent;
    }

    if(temp->depth == 0)    return TRUE;

    return FALSE;
}


NFOBJECT* nfui_get_cur_focus(NFOBJECT *obj)
{
    KEYOBJECT *top_kobj = NULL, *cur_kobj = NULL;
    NFOBJECT *top;
    NFOBJECT *ret = NULL;

    top = nfui_nfobject_get_top(obj);

    top_kobj = nfui_nfwindow_get_keyobj((NFWINDOW*)top);

    if(top_kobj)
    {
        cur_kobj = prvFindCurKFocus(top_kobj);

        if(cur_kobj)
            ret = cur_kobj->object;
    }

    return ret;
}


/*********************************************
 * FRONT KEYPAD INPUT
 ********************************************/

static KEYPAD_KID prvKeycodeToKeypadID(KEYCODE key_code)
{
    KEYPAD_KID kpid;

    if(key_code == KEY_CODE_ARROW_LEFT)             kpid = KEYPAD_LEFT;
    else if(key_code == KEY_CODE_ARROW_RIGHT)       kpid = KEYPAD_RIGHT;
    else if(key_code == KEY_CODE_ARROW_UP)          kpid = KEYPAD_UP;
    else if(key_code == KEY_CODE_ARROW_DOWN)        kpid = KEYPAD_DOWN;
    else if(key_code == KEY_CODE_ENTER)             kpid = KEYPAD_ENTER;
    else if(key_code == KEY_CODE_ESCAPE)            kpid = KEYPAD_EXIT;
    else kpid = KEYPAD_NONE;

    return kpid;
}

static KEYPAD_KID prvKeycodeToJoyID(KEYCODE key_code)
{
    KEYPAD_KID kpid = KEYPAD_NONE;

    if (key_code == JOY_LEFT)               kpid = KEYPAD_LEFT;
    else if (key_code == JOY_RIGHT)         kpid = KEYPAD_RIGHT;
    else if (key_code == JOY_UP)            kpid = KEYPAD_UP;
    else if (key_code == JOY_DOWN)          kpid = KEYPAD_DOWN;
    else if (key_code == JOY_TURN_LEFT)     kpid = RMC_ZOOMIN;
    else if (key_code == JOY_TURN_RIGHT)    kpid = RMC_ZOOMOUT;

    return kpid;
}

static guint ch_prefix = 0;

static guint _prvTransKeyIDbyPrefix(guint prefix, guint code)
{
    guint ret = 0;
    
    if (code == KEYPAD_CH00) 
    {
        switch (prefix)
        {
            case 1:
                ret = KEYPAD_CH10;
            break;
            case 2:
                ret = KEYPAD_CH20;
            break;
            case 3:
                ret = KEYPAD_CH30;
            break;
            default:
                ret = KEYPAD_CH00;
            break;
        }
    }
    else
    {
        if (prefix == 1)
        {
            if ((code >= KEYPAD_CH01) && (code <= KEYPAD_CH06)) 
            {
                ret = code + 10;
            }
            else if ((code >= KEYPAD_CH07) && (code <= KEYPAD_CH09))
            {
                ret = code + 37;
            }
            else if (code == RMC_DEV_ID)
            {
                ret = KEYPAD_DEBUG;
            }
            else
            {
                ret = KEYPAD_NONE;
            }
        }
        else if (prefix == 2)
        {
            if ((code >= KEYPAD_CH01) && (code <= KEYPAD_CH09)) 
            {
                ret = code + 47;
            }
            else
            {
                ret = KEYPAD_NONE;
            }
        }
        else if (prefix == 3)
        {
            if ((code >= KEYPAD_CH01) && (code <= KEYPAD_CH02)) 
            {
                ret = code + 57;
            }
            else
            {
                ret = KEYPAD_NONE;
            }
        }
        else
        {
            ret = code;
        }
    }
    
    return ret;
}

static void prvSendDevSignal(nfevent_type type, guint code)
{
    GdkEventKey* evt = NULL;
    NFOBJECT* top_obj = NULL;
    PAGEID pid = PGID_NONE;
    static guint pre_joy_code = KEYPAD_NONE;

    pid = nfui_get_cur_page(&top_obj);

    g_message("[%s]line(%d), PID[%d]", __FUNCTION__, __LINE__, pid);
    DMSG(1, "PID[%d], TOP OBJECT[%p], OBJ TYPE[%d]\n", pid, top_obj, (top_obj == NULL) ? 0:top_obj->type);
    DMSG(1, "Event Type[%d], Key Code[%x]\n", type, code);


    if(pid == PGID_NONE && !top_obj)        return;

    DMSG(9, "");
    if(type == NFEVENT_KEYPAD_PRESS || type == NFEVENT_REMOCON_PRESS)
    {
        evt = gdk_event_new(GDK_KEY_PRESS);
        evt->type = type;
        evt->keyval = code;

        cheat_set_kpid(evt->keyval);
        DMSG(9, "PRESS] evt type = %x, keyval = %x", evt->type, evt->keyval);
        nf_main_event_handler(NULL, evt, top_obj);

        evt->type = GDK_KEY_PRESS;

    }
    else if(type == NFEVENT_KEYPAD_RELEASE || type == NFEVENT_REMOCON_RELEASE)
    {
        evt = gdk_event_new(GDK_KEY_RELEASE);
        evt->type = type;
        evt->keyval = code;

        DMSG(9, "RELEASE] evt type = %x, keyval = %x", evt->type, evt->keyval);
        nf_main_event_handler(NULL, evt, top_obj);

        evt->type = GDK_KEY_RELEASE;
    }
    else if(type == NFEVENT_485_INPUT)
    {
        if ((code == JOY_LEFT) || (code == JOY_RIGHT) || (code == JOY_UP) || (code == JOY_DOWN) || (code == JOY_TURN_LEFT) || (code == JOY_TURN_RIGHT))
        {
            if (pre_joy_code == code) return;

            evt = gdk_event_new(GDK_KEY_PRESS);

            evt->type = NFEVENT_KEYPAD_PRESS;
            evt->keyval = prvKeycodeToJoyID(code);
            nf_main_event_handler(NULL, evt, top_obj);

            evt->type = GDK_KEY_PRESS;
            pre_joy_code = code;
        }
        else if ((code == JOY_STOP) && (pre_joy_code != KEYPAD_NONE))
        {
            evt = gdk_event_new(GDK_KEY_RELEASE);

            evt->type = NFEVENT_KEYPAD_RELEASE;
            evt->keyval = prvKeycodeToJoyID(pre_joy_code);
            nf_main_event_handler(NULL, evt, top_obj);

            evt->type = GDK_KEY_RELEASE;
            pre_joy_code = KEYPAD_NONE;
        }
        else
        {
            evt = gdk_event_new(GDK_KEY_RELEASE);

            evt->type = NFEVENT_KEYPAD_PRESS;
            evt->keyval = code;
            nf_main_event_handler(NULL, evt, top_obj);

            evt->type = NFEVENT_KEYPAD_RELEASE;
            nf_main_event_handler(NULL, evt, top_obj);

            evt->type = GDK_KEY_RELEASE;
            pre_joy_code = KEYPAD_NONE;
        }
    }
    else
    {
        evt = gdk_event_new(GDK_KEY_PRESS);
        evt->type = type;
        evt->keyval = code;

        nf_main_event_handler(NULL, evt, top_obj);

        evt->type = GDK_KEY_PRESS;
    }

    gdk_event_free((GdkEvent*)evt);
}

static gboolean send_keypad_press_signal(gpointer data)
{
    NFUTIL_THREADS_ENTER();
    prvSendDevSignal(NFEVENT_KEYPAD_PRESS, GPOINTER_TO_UINT(data));
    NFUTIL_THREADS_LEAVE();

    return FALSE;
}

static gboolean send_keypad_release_signal(gpointer data)
{
    NFUTIL_THREADS_ENTER();
    prvSendDevSignal(NFEVENT_KEYPAD_RELEASE, GPOINTER_TO_UINT(data));
    NFUTIL_THREADS_LEAVE();

    return FALSE;
}

static gboolean nfui_keypad_input (GIOChannel *gio, GIOCondition condition, gpointer data)
{
    GIOError ret;
    gchar buff[32];

    gsize buff_ret;

    if (condition & G_IO_HUP)
    {
        DMSG(1, "Read end of pipe died!");
    }

    ret = g_io_channel_read(gio, buff, 1, &buff_ret);
    if (ret != G_IO_ERROR_NONE)
    {
        DMSG(1, "error[%d]", ret);
    }

    DMSG(1, "keypad device[0x%x]\n", buff[0]);

#ifdef _SUPPORT_CAPTURE_FB   /*+20090817, cultfactory*/

    if(buff[0]&0x80) {  /*press*/
        if( (buff[0]&0x3F) == KEYPAD_HOLD ) {
            g_message("capture frame buffer...");
            my_command("dbg_snap_fb 1 0");
        }

        g_timeout_add(10, send_keypad_press_signal, GUINT_TO_POINTER(buff[0]&0x3f));
    }
    else
    {
        g_timeout_add(10, send_keypad_release_signal, GUINT_TO_POINTER(buff[0]&0x3f));
    }
#else   /*org*/
    if(buff[0]&0x80)    g_timeout_add(10, send_keypad_press_signal, GUINT_TO_POINTER(buff[0]&0x3f));
    else                g_timeout_add(10, send_keypad_release_signal, GUINT_TO_POINTER(buff[0]&0x3f));
#endif  /*_SUPPORT_CAPTURE_FB*/

    //g_message("Read %u bytes pressed[%d]  [0x%02x]", buff_ret, buff[0]&0x80 ? 1:0, buff[0]&0x3f );

    return TRUE;
}

gboolean nfui_keypad_init()
{
    gint fd;
    gulong  cb_handle = 0;
    GIOChannel  *gio_chan = NULL;

    fd = nf_dev_open_keypad();

    if(fd < 0)
    {
        return 0;
    }

    gio_chan = g_io_channel_unix_new (fd);
    if(gio_chan)    cb_handle = g_io_add_watch (gio_chan, G_IO_IN | G_IO_HUP, nfui_keypad_input, "keypad");

#if defined(USE_DEV_KEYPAD)
    if(cb_handle)
        nf_dev_keypad_dev_enable();
#endif

    return 1;
}



/*********************************************
 * REMOTE CONTROLLER INPUT
 ********************************************/


static KEYPAD_KID prvRmcIDToKeyID(guint rmcID)
{
    KEYPAD_KID kid;

    switch(rmcID)
    {
#if defined(CHIP_NVT)
        if(!strcmp(nf_api_param_hw_get_rc_type(), "IRC200"))
        {
            case 0x80000000:  kid = KEYPAD_POWER;     break;
            case 0x81000000:  kid = RMC_LOGOUT;       break;
            case 0x82000000:  kid = KEYPAD_PANIC;     break;
            case 0x83000000:  kid = KEYPAD_SEARCH;    break;
            case 0x84000000:  kid = KEYPAD_ARCH;      break;
            case 0x85000000:  kid = KEYPAD_SETUP;     break;
            case 0x86000000:  kid = RMC_ALARM;        break;
            case 0x87000000:  kid = KEYPAD_CH01;      break;
            case 0x88000000:  kid = KEYPAD_CH02;      break;
            case 0x89000000:  kid = KEYPAD_CH03;      break;
            case 0x8a000000:  kid = KEYPAD_CH04;      break;
            case 0x8b000000:  kid = KEYPAD_CH05;      break;
            case 0x8c000000:  kid = KEYPAD_CH06;      break;
            case 0x8d000000:  kid = KEYPAD_CH07;      break;
            case 0x8e000000:  kid = KEYPAD_CH08;      break;
            case 0x8f000000:  kid = KEYPAD_CH09;      break;
            case 0x90000000:  kid = KEYPAD_TEN;       break;
            case 0x91000000:  kid = KEYPAD_CH00;      break;
            case 0x92000000:  kid = RMC_DEV_ID;       break;
            case 0x93000000:  kid = KEYPAD_DISP;      break;
            case 0x94000000:  kid = KEYPAD_SEQ;       break;
            case 0x95000000:  kid = RMC_LOG;          break;
            case 0x96000000:  kid = KEYPAD_FREEZE;    break;
            case 0x97000000:  kid = KEYPAD_LOCK;      break;
            case 0x98000000:  kid = RMC_AUDIO;        break;
            case 0x99000000:  kid = RMC_SNAPSHOT;     break;
            case 0x9a000000:  kid = KEYPAD_REW;       break;
            case 0x9b000000:  kid = KEYPAD_FF;        break;
            case 0x9c000000:  kid = RMC_RESERVE;      break;
            case 0x9d000000:  kid = RMC_BJUMP;        break;
            case 0x9e000000:  kid = KEYPAD_BACKWARD;  break;
            case 0x9f000000:  kid = KEYPAD_PAUSE;     break;
            case 0xa0000000:  kid = RMC_FJUMP;        break;
            case 0xa1000000:  kid = KEYPAD_EXIT;      break;
            case 0xa2000000:  kid = RMC_MENU;         break;
            case 0xa3000000:  kid = KEYPAD_UP;        break;
            case 0xa4000000:  kid = KEYPAD_LEFT;      break;
            case 0xa5000000:  kid = KEYPAD_ENTER;     break;
            case 0xa6000000:  kid = KEYPAD_RIGHT;     break;
            case 0xa7000000:  kid = KEYPAD_DOWN;      break;
            case 0xa8000000:  kid = RMC_ZOOMOUT;      break;
            case 0xa9000000:  kid = RMC_ZOOMIN;       break;
            case 0xaa000000:  kid = RMC_NEAR;         break;
            case 0xab000000:  kid = RMC_FAR;          break;
            case 0xac000000:  kid = RMC_PRESET;       break;
            case 0xad000000:  kid = RMC_AFOCUS;       break;
            case 0xae000000:  kid = KEYPAD_ZOOM;      break;
            case 0xaf000000:  kid = KEYPAD_PTZ;       break;
        }
            //else if(!strcmp(nf_api_param_hw_get_rc_type(), "ICA_ITX"))
        else
        {
        case 0x51:  kid = KEYPAD_POWER;     break;
        case 0x54:  kid = KEYPAD_SETUP;     break;
        case 0x31:  kid = KEYPAD_CH01;      break;
        case 0x32:  kid = KEYPAD_CH02;      break;
        case 0x33:  kid = KEYPAD_CH03;      break;
        case 0x34:  kid = KEYPAD_CH04;      break;
        case 0x35:  kid = KEYPAD_CH05;      break;
        case 0x36:  kid = KEYPAD_CH06;      break;
        case 0x37:  kid = KEYPAD_CH07;      break;
        case 0x38:  kid = KEYPAD_CH08;      break;
        case 0x39:  kid = KEYPAD_CH09;      break;
        case 0x53:  kid = KEYPAD_EXIT;      break;
        case 0x0D:  kid = KEYPAD_ENTER;     break;
        case 0x26:  kid = KEYPAD_UP;        break;
        case 0x28:  kid = KEYPAD_DOWN;      break;
        case 0x25:  kid = KEYPAD_LEFT;      break;
        case 0x27:  kid = KEYPAD_RIGHT;     break;
        case 0xFFFFFFBB:  kid = KEYPAD_PAUSE;     break;
        case 0x56:  kid = RMC_BJUMP;       break;
        case 0x4E:  kid = RMC_FJUMP;        break;
        case 0x44:  kid = KEYPAD_DISP;      break;
        case 0x09:  kid = KEYPAD_SEARCH;    break;
        //case 0x1A:  kid = KEYPAD_PTZ;       break;
        case 0x4D:  kid = KEYPAD_PANIC;     break;
        //case 0x05:  kid = KEYPAD_LOCK;      break;
        case 0x4F:  kid = KEYPAD_SEQ;       break;
        //case 0x09:  kid = KEYPAD_ZOOM;      break;
        case 0x11:  kid = KEYPAD_ARCH;      break;

        // IPX
        //case 0x48:  kid = RMC_LOGOUT;       break;
        //case 0x7D:  kid = RMC_ALARM;        break;
        //case 0x49:  kid = RMC_LOG;          break;
        //case 0x7B:  kid = KEYPAD_FREEZE;    break;
        //case 0x50:  kid = RMC_SNAPSHOT;     break;
        //case 0x54:  kid = RMC_NEAR;         break;
        //case 0x55:  kid = RMC_FAR;          break;
        //case 0x7E:  kid = RMC_PRESET;       break;
        //case 0x53:  kid = RMC_AFOCUS;       break;
        //case 0x06:  kid = RMC_AUDIO;        break;
        //case 0x56:  kid = RMC_RESERVE;      break;
        case 0x49:  kid = KEYPAD_REW;        break;
        case 0x52:  kid = KEYPAD_FF;        break;
        case 0x1b:  kid = RMC_MENU;         break;
        //case 0x58:  kid = RMC_ZOOMIN;       break;
        //case 0x57:  kid = RMC_ZOOMOUT;      break;
        //case 0x02:  kid = RMC_DEV_ID;       break;
        case 0x58:  kid = KEYPAD_TEN;       break;
        case 0x30:  kid = KEYPAD_CH00;      break;
        case 0xFFFFFFBD:  kid = KEYPAD_BACKWARD;  break;
        }
#else
        case 0x00:  kid = KEYPAD_POWER;     break;
        case 0x04:  kid = KEYPAD_SETUP;     break;
        case 0x1C:  kid = KEYPAD_CH01;      break;
        case 0x1D:  kid = KEYPAD_CH02;      break;
        case 0x1E:  kid = KEYPAD_CH03;      break;
        case 0x40:  kid = KEYPAD_CH04;      break;
        case 0x41:  kid = KEYPAD_CH05;      break;
        case 0x42:  kid = KEYPAD_CH06;      break;
        case 0x44:  kid = KEYPAD_CH07;      break;
        case 0x45:  kid = KEYPAD_CH08;      break;
        case 0x46:  kid = KEYPAD_CH09;      break;
        case 0x03:  kid = KEYPAD_EXIT;      break;
        case 0x07:  kid = KEYPAD_ENTER;     break;
        case 0x12:  kid = KEYPAD_UP;        break;
        case 0x13:  kid = KEYPAD_DOWN;      break;
        case 0x14:  kid = KEYPAD_LEFT;      break;
        case 0x16:  kid = KEYPAD_RIGHT;     break;
        case 0x15:  kid = KEYPAD_PAUSE;     break;
        case 0x10:  kid = KEYPAD_REW;       break;
        case 0x11:  kid = KEYPAD_FF;        break;
        case 0x18:  kid = KEYPAD_DISP;      break;
        case 0x19:  kid = KEYPAD_SEARCH;    break;
        case 0x1A:  kid = KEYPAD_PTZ;       break;
        case 0x1B:  kid = KEYPAD_PANIC;     break;
        case 0x05:  kid = KEYPAD_LOCK;      break;
        case 0x08:  kid = KEYPAD_SEQ;       break;
        case 0x09:  kid = KEYPAD_ZOOM;      break;
        case 0x01:  kid = KEYPAD_ARCH;      break;

        // IPX
        case 0x48:  kid = RMC_LOGOUT;       break;
        case 0x7D:  kid = RMC_ALARM;        break;
        case 0x49:  kid = RMC_LOG;          break;
        case 0x7B:  kid = KEYPAD_FREEZE;    break;
        case 0x50:  kid = RMC_SNAPSHOT;     break;
        case 0x54:  kid = RMC_NEAR;         break;
        case 0x55:  kid = RMC_FAR;          break;
        case 0x7E:  kid = RMC_PRESET;       break;
        case 0x53:  kid = RMC_AFOCUS;       break;
        case 0x06:  kid = RMC_AUDIO;        break;
        case 0x56:  kid = RMC_RESERVE;      break;
        case 0x51:  kid = RMC_BJUMP;        break;
        case 0x52:  kid = RMC_FJUMP;        break;
        case 0x6E:  kid = RMC_MENU;         break;
        case 0x58:  kid = RMC_ZOOMIN;       break;
        case 0x57:  kid = RMC_ZOOMOUT;      break;
        case 0x02:  kid = RMC_DEV_ID;       break;
        case 0x0D:  kid = KEYPAD_TEN;       break;
        case 0x47:  kid = KEYPAD_CH00;      break;
        case 0x21:  kid = KEYPAD_BACKWARD;  break;


#if defined(_ANF_0824CL)
        case 0x68:  kid = RMC_MODE;         break;
        case 0x69:  kid = RMC_DVR;          break;
        case 0x67:  kid = RMC_DVI;          break;
        case 0x60:  kid = RMC_COMPONENT;    break;
        case 0x62:  kid = RMC_PIP;          break;
        case 0x61:  kid = RMC_INFO;         break;
        case 0x63:  kid = RMC_VOLUME_UP;    break;
        case 0x64:  kid = RMC_VOLUME_DOWN;  break;
        case 0x65:  kid = RMC_AUDIO;        break;
        case 0x66:  kid = RMC_PIC_FR;       break;
#endif
#endif
        default:    kid = KEYPAD_NONE;      break;
    }

    return kid;
}

static gboolean send_remocon_press_signal(gpointer data)
{
    KEYPAD_KID kid;
    guint rmcID;

    rmcID = GPOINTER_TO_UINT(data);

    // same front panel functions..
#if defined(CHIP_NVT)
    //remocon buzzer for irda
   scm_cntl_beep_irda();
#endif

#if defined(CHIP_NVT)
    if(rmcID==0x58)
    {
        ch_prefix++;
        if (ch_prefix > 3)
            ch_prefix = 3;
        return FALSE;
    }
#else
    if (rmcID == 0x0D) {
        ch_prefix++;
        if (ch_prefix > 3)
            ch_prefix = 3;

        return FALSE;
    }
#endif

    kid = prvRmcIDToKeyID(rmcID);

    if(kid == KEYPAD_NONE) {
        ch_prefix = 0;
        return FALSE;
    }
        
    kid = _prvTransKeyIDbyPrefix(ch_prefix, kid);
    ch_prefix = 0;
    
    if (kid == KEYPAD_NONE) {
        return FALSE;
    }

    g_message("[%s]line(%d)", __FUNCTION__, __LINE__);
    NFUTIL_THREADS_ENTER();
#if 1
    prvSendDevSignal(NFEVENT_REMOCON_PRESS, kid);
#else
    prvSendDevSignal(NFEVENT_KEYPAD_PRESS, kid);
#endif
    NFUTIL_THREADS_LEAVE();

    return FALSE;
}

static gboolean send_remocon_release_signal(gpointer data)
{
    KEYPAD_KID kid;
    guint rmcID;

    rmcID = GPOINTER_TO_UINT(data);

#if defined(CHIP_NVT)
    #if defined(REMOCON_EDIT_DELAY)
        if(rmcID==0x58)
            return FALSE;
    #endif
#else
#if defined(REMOCON_EDIT_DELAY)
    if(rmcID==0x0D)
        return FALSE;
#else
    if(rmcID==0x0D || rmcID==0x7f)
        return FALSE;
#endif
#endif
/*
    if(rmcID == 0x47)
    {
        if(ch_prefix)
            kid = KEYPAD_CH10;
        else
            kid = RMC_NUM_0;
    }
    else
    {*/
        kid = prvRmcIDToKeyID(rmcID);

        if(kid == KEYPAD_NONE)
            return FALSE;
/*
        if(ch_prefix)
        {
            if((kid>=KEYPAD_CH01) && (kid<=KEYPAD_CH06))
            {
                kid += 10;
            }
            else
            {
                ch_prefix = 0;
                return FALSE;
            }
        }
    }

    ch_prefix = 0;
*/
    g_message("[%s]line(%d)", __FUNCTION__, __LINE__);
    NFUTIL_THREADS_ENTER();
#if 1
    prvSendDevSignal(NFEVENT_REMOCON_RELEASE, kid);
#else
    prvSendDevSignal(NFEVENT_KEYPAD_RELEASE, kid);
#endif
    NFUTIL_THREADS_LEAVE();

    return FALSE;
}

#include <linux/input.h>

static gboolean nfui_remocon_input (GIOChannel *gio, GIOCondition condition, gpointer data)
{
#if defined(CHIP_NVT_NA51039) || defined(CHIP_NVT_NT9833x)
    int ret;
    int fd = g_io_channel_unix_get_fd(gio);
	int maxfd = fd + 1;
	struct input_event ie;
	fd_set readfds;
	struct timeval tv;
    struct timeval curr_time;
    guint buff = 0;
    guint rmc_id, tmp_id;
    guint sys_id;
    static int rc_repeat_flag_cnt = 0;

    FD_ZERO(&readfds);
    FD_SET(fd,&readfds);

    tv.tv_sec = 0;
    tv.tv_usec = 10000;

    int state = select(maxfd, &readfds, NULL, NULL, &tv);

    switch (state) {
        case -1:
            return TRUE;
        break;

        case 0:  /* timeout */
        break;

        default:
            if (FD_ISSET(fd, &readfds))
            {
                ret = read(fd, &ie, sizeof (ie));
                if (ret < 0) printf("failed to read fd\n", ret);

                // printf("###yanggungg : %s, %d type(%d) code(%d) value(%08X)\n", __FUNCTION__, __LINE__, ie.type, ie.code, ie.value);
                if (ie.type == 4 && ie.code == 4 && ie.value > 0)
                {
                    if (!strcmp(nf_api_param_hw_get_rc_type(), "ICA_ITX"))
                    {
                        if (ie.value == 32849) buff = 0x51;
                        else if (ie.value == 32852) buff = 0x54;
                        else if (ie.value == 32817) buff = 0x31;
                        else if (ie.value == 32818) buff = 0x32;
                        else if (ie.value == 32819) buff = 0x33;
                        else if (ie.value == 32820) buff = 0x34;
                        else if (ie.value == 32821) buff = 0x35;
                        else if (ie.value == 32822) buff = 0x36;
                        else if (ie.value == 32823) buff = 0x37;
                        else if (ie.value == 32824) buff = 0x38;
                        else if (ie.value == 32825) buff = 0x39;
                        else if (ie.value == 32851) buff = 0x53;
                        else if (ie.value == 32781) buff = 0x0D;
                        else if (ie.value == 32806) buff = 0x26;
                        else if (ie.value == 32808) buff = 0x28;
                        else if (ie.value == 32805) buff = 0x25;
                        else if (ie.value == 32807) buff = 0x27;
                        else if (ie.value == 32955) buff = 0xFFFFFFBB;
                        else if (ie.value == 32854) buff = 0x56;
                        else if (ie.value == 32846) buff = 0x4E;
                        else if (ie.value == 32836) buff = 0x44;
                        else if (ie.value == 32777) buff = 0x09;
                        else if (ie.value == 32845) buff = 0x4D;
                        else if (ie.value == 32847) buff = 0x4F;
                        else if (ie.value == 32785) buff = 0x11;
                        else if (ie.value == 32841) buff = 0x49;
                        else if (ie.value == 32850) buff = 0x52;
                        else if (ie.value == 32795) buff = 0x1b;
                        else if (ie.value == 32856) buff = 0x58;
                        else if (ie.value == 32816) buff = 0x30;
                        else if (ie.value == 32957) buff = 0xFFFFFFBD;

                        g_timeout_add(10, send_remocon_press_signal, GUINT_TO_POINTER(buff));
                        g_timeout_add(10, send_remocon_release_signal, GUINT_TO_POINTER(buff));
                    }
                    else if (!strcmp(nf_api_param_hw_get_rc_type(), "IRC200"))
                    {
                        int press = 0, release = 0;

                        // g_message("remocon ie.value : 0x%x key_val %x rc_id : [0x%x]", ie.value, (ie.value&(0xff << 8)), ie.value&0xff);

                        // check remocon type from bit
                        if ((ie.value & (0xffff << 16)) != 0x05200000) {
                            // g_message("Remocon is not IRC200");
                            return TRUE;
                        }

                        // check remocon id
                        tmp_id = (ie.value & 0xff);
                        rmc_id = ((tmp_id&0xf0)>>4)*10 + (tmp_id&0x0f); // transform to decimal

                        if(rmc_id != nfcd_get_remocon_id())
                            nfcd_set_remocon_id(rmc_id);

                        sys_id = DAL_get_RemoconID();

                        // g_message("remocon id : [%d] sys_id : [%d]", rmc_id, sys_id);

                        if(rmc_id != 0 && (rmc_id != sys_id))
                        {
                            NFOBJECT *top_obj;
                            PAGEID pid = PGID_NONE;
                            pid = nfui_get_cur_page(&top_obj);

                            // g_message(" check val : [0x%x]", (ie.value & (0xff << 8)));
                            if(pid != PGID_RMCID_CONF && (ie.value & (0xff << 8)) != 0x1200)  // (ie.value & (0x12 << 8)) is remocon id key val
                                return TRUE;
                        }

                        // remove remocon id bit
                        ie.value &= ~0xff;
                        // g_message(" check val : [0x%x]", ie.value);

                        // check press or release
                        if (!(ie.value & (1 << 15))) {
                            press = 1;
                        } else {
                            release = 1;
                        }

                        // remove remocon type bit and set 
                        buff = ((ie.value << 16) | 0x80000000);
#if 0//ksi_test 추후 기능 관련 포팅 할때 작업 한다.
                        gettimeofday(&curr_time, 0);
                        nf_dev_remocon_set_repeat_time(&curr_time);
                        if (nf_dev_remocon_get_repeat_flag() == 0) {
                            if (press == 1) {
                                nf_dev_remocon_set_repeat_flag(1);
                            }
                        } else {
                            rc_repeat_flag_cnt++;
                            if (press == 1) {
                                rc_repeat_flag_cnt = 0;
                            }

                            if (rc_repeat_flag_cnt > 75) {
                                rc_repeat_flag_cnt = 0;
                                nf_dev_remocon_set_repeat_flag(0);
                            }
                        }
#endif
                        // if (press) g_timeout_add(10, send_remocon_press_signal, GUINT_TO_POINTER(buff));
                        // if (release) g_timeout_add(10, send_remocon_release_signal, GUINT_TO_POINTER(buff));
                        g_timeout_add(10, send_remocon_press_signal, GUINT_TO_POINTER(buff));
                        g_timeout_add(10, send_remocon_release_signal, GUINT_TO_POINTER(buff));
                    }
                }
            }
        break;
    }
#else
    GIOError ret;
    gchar buff[32];
    guint rmc_id;
    guint sys_id;

    gsize buff_ret;

    if (condition & G_IO_HUP)
    {
        DMSG(1, "Read end of pipe died!");
    }

    ret = g_io_channel_read(gio, buff, 1, &buff_ret);
    if (ret != G_IO_ERROR_NONE)
    {
        DMSG(1, "error[%d]", ret);
    }

    DMSG(1, "remocon device %d [0x%x]\n", buff[0]&0x80, buff[0]&0x7f);
    //g_message("remocon val %d [0x%x]\n", buff[0]&0x80, buff[0]&0x7f);
    //g_message("remocon id [0x%x]\n", buff[2]&0xff);

    // CHECK REMOCON ID
    if(buff[0]&0x80)
    {
        rmc_id = ((buff[2]&0xf0)>>4)*10 + (buff[2]&0x0f);
        if(rmc_id != nfcd_get_remocon_id())
            nfcd_set_remocon_id(rmc_id);
    }
    else
    {
        rmc_id = nfcd_get_remocon_id();
    }

    sys_id = DAL_get_RemoconID();
    if(rmc_id != 0 && (rmc_id != sys_id))
    {
        NFOBJECT *top_obj;
        PAGEID pid = PGID_NONE;
        pid = nfui_get_cur_page(&top_obj);

        if(pid != PGID_RMCID_CONF && (buff[0]&0x7f) != 0x02)
            return TRUE;
    }

    if(buff[0]&0x80)    g_timeout_add(10, send_remocon_press_signal, GUINT_TO_POINTER(buff[0]&0x7f));
    else                g_timeout_add(10, send_remocon_release_signal, GUINT_TO_POINTER(buff[0]&0x7f));

    //g_message("Read %u bytes pressed[%d]  [0x%02x]", buff_ret, buff[0]&0x80 ? 1:0, buff[0]&0x7f );
#endif
    return TRUE;
}

gboolean nfui_remocon_init()
{
    gint fd;
    gulong  cb_handle = 0;
    GIOChannel  *gio_chan = NULL;

    fd = nf_dev_open_remocon();

    if(fd < 0)
    {
        return 0;
    }

    gio_chan = g_io_channel_unix_new (fd);
    if(gio_chan)    cb_handle = g_io_add_watch (gio_chan, G_IO_IN | G_IO_HUP, nfui_remocon_input, "remocon");

#if defined(USE_DEV_REMOCON)
    if(cb_handle)
        nf_dev_remocon_enable(1);
#endif


    return 1;
}



/*********************************************
 * JOG INPUT
 ********************************************/

static gboolean send_jog_signal(gpointer data)
{
    guint code;

    code = GPOINTER_TO_UINT(data);

    NFUTIL_THREADS_ENTER();
    prvSendDevSignal(NFEVENT_JOG_CHANGE, code);
    NFUTIL_THREADS_LEAVE();

    return FALSE;
}

static gboolean nfui_jog_input (GIOChannel *gio, GIOCondition condition, gpointer data)
{
    GIOError ret;
    gchar buff[32];

    gsize buff_ret;

    if (condition & G_IO_HUP)
    {
        DMSG(1, "Read end of pipe died!");
    }

    ret = g_io_channel_read(gio, buff, 1, &buff_ret);
    if (ret != G_IO_ERROR_NONE)
    {
        DMSG(1, "error[%d]", ret);
    }

    DMSG(1, "jog device[0x%x]\n", buff[0]);

    g_timeout_add(10, send_jog_signal, GUINT_TO_POINTER(buff[0]&0xff));

    //g_message("Read %u bytes pressed[%d]  [0x%02x]", buff_ret, buff[0]&0x80 ? 1:0, buff[0]);//buff[0]&0x7f );

    return TRUE;
}



#ifdef USE_DEV_JOG
gboolean nfui_jog_init()
{
    gint fd;
    gulong  cb_handle = 0;
    GIOChannel  *gio_chan = NULL;

    fd = nf_dev_open_jog();

    if(fd < 0)
    {
        return 0;
    }

    gio_chan = g_io_channel_unix_new (fd);
    if(gio_chan)    cb_handle = g_io_add_watch (gio_chan, G_IO_IN | G_IO_HUP, nfui_jog_input, "jog");

    if(cb_handle)   nf_dev_jog_enable(1);



    return 1;
}
#endif




/*********************************************
 * SHUTTLE INPUT
 ********************************************/
static gboolean send_shuttle_signal(gpointer data)
{
    guint code;

    code = GPOINTER_TO_UINT(data);

    NFUTIL_THREADS_ENTER();
    prvSendDevSignal(NFEVENT_SHUTTLE_CHANGE, code);
    NFUTIL_THREADS_LEAVE();

    return FALSE;
}

static gboolean nfui_shuttle_input (GIOChannel *gio, GIOCondition condition, gpointer data)
{
    GIOError ret;
    gchar buff[32];

    gsize buff_ret;

    if (condition & G_IO_HUP)
    {
        DMSG(1, "Read end of pipe died!");
    }

    ret = g_io_channel_read(gio, buff, 1, &buff_ret);
    if (ret != G_IO_ERROR_NONE)
    {
        DMSG(1, "error[%d]", ret);
    }

    DMSG(1, "shuttle device[0x%x]\n", buff[0]);
    g_timeout_add(10, send_shuttle_signal, GUINT_TO_POINTER(buff[0]&0xff));

    //g_message("Read %u bytes pressed[%d]  [0x%02x]", buff_ret, buff[0]&0x80 ? 1:0, buff[0]);//buff[0]&0x7f );

    return TRUE;
}

#ifdef USE_DEV_SHUTTLE
gboolean nfui_shuttle_init()
{
    gint fd;
    gulong  cb_handle = 0;
    GIOChannel  *gio_chan = NULL;

    fd = nf_dev_open_shuttle();

    if(fd < 0)
    {
        return 0;
    }

    gio_chan = g_io_channel_unix_new (fd);
    if(gio_chan)    cb_handle = g_io_add_watch (gio_chan, G_IO_IN | G_IO_HUP, nfui_shuttle_input, "shuttle");

    if(cb_handle)   nf_dev_shuttle_enable(1);



    return 1;
}
#endif

/************************************************************************
 * EXTERNAL KEYBOARD CONTROLER
 * **********************************************************************/

static KEYPAD_KID prv485IDToKeyID(guint ID_485)
{
    KEYPAD_KID kpid;

    switch(ID_485)
    {
        case NF_KEYCTRL_BUTTON_MAP_NUM1:
        case NF_KEYCTRL_BUTTON_MAP_NUM2:
        case NF_KEYCTRL_BUTTON_MAP_NUM3:
        case NF_KEYCTRL_BUTTON_MAP_NUM4:
        case NF_KEYCTRL_BUTTON_MAP_NUM5:
        case NF_KEYCTRL_BUTTON_MAP_NUM6:
        case NF_KEYCTRL_BUTTON_MAP_NUM7:
        case NF_KEYCTRL_BUTTON_MAP_NUM8:
        case NF_KEYCTRL_BUTTON_MAP_NUM9:
        case NF_KEYCTRL_BUTTON_MAP_NUM10:
        case NF_KEYCTRL_BUTTON_MAP_NUM11:
        case NF_KEYCTRL_BUTTON_MAP_NUM12:
        case NF_KEYCTRL_BUTTON_MAP_NUM13:
        case NF_KEYCTRL_BUTTON_MAP_NUM14:
        case NF_KEYCTRL_BUTTON_MAP_NUM15:
        case NF_KEYCTRL_BUTTON_MAP_NUM16:
            kpid = KEYPAD_CH01 + (ID_485 - NF_KEYCTRL_BUTTON_MAP_NUM1);
            break;

        case NF_KEYCTRL_BUTTON_MAP_DISPLAY:     kpid = KEYPAD_DISP;     break;
        case NF_KEYCTRL_BUTTON_MAP_SEQ:         kpid = KEYPAD_SEQ;      break;
        case NF_KEYCTRL_BUTTON_MAP_PANIC:       kpid = KEYPAD_PANIC;    break;
        case NF_KEYCTRL_BUTTON_MAP_ZOOM:        kpid = KEYPAD_ZOOM;     break;
        case NF_KEYCTRL_BUTTON_MAP_LOCK:        kpid = KEYPAD_LOCK;     break;
        case NF_KEYCTRL_BUTTON_MAP_ARCHIVE:     kpid = KEYPAD_ARCH;     break;
        case NF_KEYCTRL_BUTTON_MAP_PTZ:         kpid = KEYPAD_PTZ;      break;
        case NF_KEYCTRL_BUTTON_MAP_SETUP:       kpid = KEYPAD_SETUP;    break;
        case NF_KEYCTRL_BUTTON_MAP_SEARCH:      kpid = KEYPAD_SEARCH;   break;
        case NF_KEYCTRL_BUTTON_MAP_LEFT:        kpid = KEYPAD_LEFT;     break;
        case NF_KEYCTRL_BUTTON_MAP_RIGHT:       kpid = KEYPAD_RIGHT;    break;
        case NF_KEYCTRL_BUTTON_MAP_UP:          kpid = KEYPAD_UP;       break;
        case NF_KEYCTRL_BUTTON_MAP_DOWN:        kpid = KEYPAD_DOWN;     break;
        case NF_KEYCTRL_BUTTON_MAP_PAUSE:       kpid = KEYPAD_PAUSE;    break;
        case NF_KEYCTRL_BUTTON_MAP_RW:          kpid = KEYPAD_BACKWARD; break;
        case NF_KEYCTRL_BUTTON_MAP_FW:          kpid = KEYPAD_FORWARD;  break;
        case NF_KEYCTRL_BUTTON_MAP_FF:          kpid = KEYPAD_FF;       break;
        case NF_KEYCTRL_BUTTON_MAP_RF:          kpid = KEYPAD_REW;      break;
        case NF_KEYCTRL_BUTTON_MAP_ENTER:       kpid = KEYPAD_ENTER;    break;
        case NF_KEYCTRL_BUTTON_MAP_RETURN:      kpid = KEYPAD_EXIT;     break;
        case NF_KEYCTRL_BUTTON_MAP_HOLD:        kpid = KEYPAD_HOLD;     break;
        case NF_KEYCTRL_BUTTON_MAP_POWER:       kpid = KEYPAD_POWER;    break;
        case NF_KEYCTRL_BUTTON_MAP_RELAY:       kpid = KEYPAD_RELAY;    break;

        case NF_KEYCTRL_BUTTON_MAP_LOGOUT:      kpid = RMC_LOGOUT;      break;
        case NF_KEYCTRL_BUTTON_MAP_STOP:        kpid = JOY_STOP;        break;
        case NF_KEYCTRL_BUTTON_MAP_JOYSTIC_LEFT:        kpid = JOY_LEFT;        break;
        case NF_KEYCTRL_BUTTON_MAP_JOYSTIC_RIGHT:       kpid = JOY_RIGHT;       break;
        case NF_KEYCTRL_BUTTON_MAP_JOYSTIC_UP:          kpid = JOY_UP;          break;
        case NF_KEYCTRL_BUTTON_MAP_JOYSTIC_DOWN:        kpid = JOY_DOWN;        break;
        case NF_KEYCTRL_BUTTON_MAP_TURN_LEFT:           kpid = JOY_TURN_LEFT;   break;
        case NF_KEYCTRL_BUTTON_MAP_TURN_RIGHT:          kpid = JOY_TURN_RIGHT;  break;

        default:    kpid = KEYPAD_NONE;     break;
    }

    return kpid;
}

static gboolean send_485_signal(gpointer data)
{
    guint code;
    KEYPAD_KID kpid;

    kpid = prv485IDToKeyID(GPOINTER_TO_UINT(data));

    g_message("%s, %d, kpid:%08X", __FUNCTION__, __LINE__, kpid);

    NFUTIL_THREADS_ENTER();
    prvSendDevSignal(NFEVENT_485_INPUT, kpid);
    NFUTIL_THREADS_LEAVE();

    return FALSE;
}

static void nfui_485_input(gpointer handoff_arg, guint key_data)
{
    DMSG(1, "485 KEY INPUT!! 485 CODE[%x]\n", key_data);

    g_timeout_add(10, send_485_signal, GUINT_TO_POINTER(key_data));
}


gboolean nfui_485_init()
{
    gboolean ret;

    ret = nf_keyctrl_register_handoff(nfui_485_input, NULL);
    DMSG(1, "485 init return %s\n", ret ? "TRUE" : "FALSE");

    return ret;
}


/*********************************************
 * MOUSE INPUT
 ********************************************/
static gboolean nfui_check_mouse_connect(gpointer data)
{
    nf_ui_check_mouse_connect();

    return TRUE;
}

gboolean nfui_mouse_init()
{
    g_timeout_add(500, nfui_check_mouse_connect, NULL);

    return 1;
}


/*****************************************************
 * TIMER RELATED...
 * **************************************************/
#if 0
static GTimer *idle_timer = NULL;

void nfui_idle_timer_init()
{
    if(idle_timer)
    {
        g_timer_reset(idle_timer);
        return;
    }

    idle_timer = g_timer_new();
    usleep(500);

    if(idle_timer)
        g_timer_start(idle_timer);
}

void nfui_idle_timer_reset()
{
    if(idle_timer)
        g_timer_reset(idle_timer);
}

gdouble nfui_get_idle_timer_elapsed()
{
    gdouble elapse = 0;

    elapse = g_timer_elapsed(idle_timer, NULL);

    return elapse;
}

void nfui_idle_timer_destroy()
{
    if(idle_timer)
    {
        g_timer_stop(idle_timer);
        g_timer_destroy(idle_timer);
        idle_timer = NULL;
    }
}
#endif

/********************************************************************
 * KEY PRESS EVENT REPEAT....
 * *****************************************************************/


static gboolean repeat_key_proc_short(gpointer data)
{
    guint kid;

    kid = GPOINTER_TO_UINT(data);

    NFUTIL_THREADS_ENTER();
    prvSendDevSignal(NFEVENT_KEYPAD_PRESS, kid);
    NFUTIL_THREADS_LEAVE();

    return TRUE;
}

static gboolean repeat_key_proc(gpointer data)
{
    guint kid;

    kid = GPOINTER_TO_UINT(data);

    NFUTIL_THREADS_ENTER();
    prvSendDevSignal(NFEVENT_KEYPAD_PRESS, kid);
    NFUTIL_THREADS_LEAVE();

    repeat_key = g_timeout_add(KEY_REPEAT_INTERVAL, repeat_key_proc_short, data);

    return FALSE;
}


/********************************************************************
 * AUDIO CHANNEL CHANGE MODE ....
 * *****************************************************************/
#if defined(__XRPLUS_UI__)
void nfui_set_audio_ch_mode(AUDIO_CH_CHANGE_MODE mode)
{
    if(audio_change_mode != mode)
        audio_change_mode = mode;
}

AUDIO_CH_CHANGE_MODE nfui_get_audio_ch_mode()
{
    return audio_change_mode;
}
#endif

void hid_remote_control_send_signal(nfevent_type type, guint code)
{
	evt_send_to_local(INFY_HID_KPAD_NOTI, type, 0, GUINT_TO_POINTER(code));
}
void nfui_hid_change_cursor_send_signal(nfevent_type type, guint code)
{
	evt_send_to_local(INFY_HID_CURSOR_CHANGE, type, 0, GUINT_TO_POINTER(code));
}

void _emit_signal_hid_keypad_notify(nfevent_type type, guint code)
{
	g_message("%s, %d, type:%d, %x", __FUNCTION__, __LINE__, type, code);

	prvSendDevSignal(type, code);
}
void nfui_signal_hid_mouse_cursor_change(nfevent_type type, guint code)
{
	int i = 0;
	int gdk_window_list_cnt = 0;

	GdkWindow *gdk_window = NULL;

	gdk_window_list_cnt = g_slist_length(gdk_window_list);

	for (i = 0; i < gdk_window_list_cnt; i++)
	{
		gdk_window = g_slist_nth_data(gdk_window_list, i);

		if (gdk_window != NULL)
		{
			if (GDK_IS_WINDOW(gdk_window))
			{
				nftool_change_custom_cursor(gdk_window, type);
			}
		}
	}
}

