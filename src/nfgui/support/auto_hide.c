/*
 * auto_hide.c
 *        - dependency :
 *                   
 *
 * Written by JungKyu Park. <parangi@itxsecurity.com>
 * Copyright (c) ITX security, Jan 31, 2012
 *
*/

#include "nf_afx.h"
#include "auto_hide.h"
#include "event_loop.h"
#include "smt.h"
#include "nf_ui_page_manager.h"



/////////////////////////////////////////////////////////////
//
// private variables
//

typedef struct {
    gint run;
	NFOBJECT *object;
	guint check_timer;
    GTimer *elapsed_timer;
    gint hide_sec;
	AUTOHIDE_CB_FUNC show_cb;
	gpointer show_data;
	AUTOHIDE_CB_FUNC hide_cb;
	gpointer hide_data;
} AH_T;

static AH_T iah = {0, };



/////////////////////////////////////////////////////////////
//
// private functions
//

static gboolean _autohide(gpointer data)
{
    NFOBJECT *top_obj = NULL;
    PAGEID pid = PGID_NONE;

    gdouble elapsed_t;

    elapsed_t = g_timer_elapsed(iah.elapsed_timer, NULL);

    if ((gint)elapsed_t > iah.hide_sec)
    {
        pid = nfui_get_cur_page(&top_obj);

        if (nfui_nfobject_is_shown(iah.object) && (iah.object == top_obj))
        {
            nfui_signal_emit(iah.object, WND_HIDE, FALSE);
            nfui_nfobject_hide(iah.object);
            if(iah.hide_cb) iah.hide_cb(iah.hide_data);
        }
    }

    return TRUE;
}

static void _create_timer()
{
    iah.check_timer = g_timeout_add(300, _autohide, NULL);   

    iah.elapsed_timer = g_timer_new();
    g_timer_start(iah.elapsed_timer);
}

static void _delete_timer()
{
    if(iah.check_timer) {
        g_source_remove(iah.check_timer);
        iah.check_timer = 0;
    }

    g_timer_stop(iah.elapsed_timer);
    g_timer_destroy(iah.elapsed_timer);
    iah.elapsed_timer = NULL;
}



/////////////////////////////////////////////////////////////
//
// public interfaces
//

gint vw_autohide_set_obj(NFOBJECT *obj, gint sec)
{
    if (iah.object) return -1;

    g_message("%s, %d, obj:%p", __FUNCTION__, __LINE__, obj);

    memset(&iah, 0x00, sizeof(AH_T));
    iah.run = 1;
    iah.object = obj;
    iah.hide_sec = sec;
    
    _create_timer();
    return 0;
}

gint vw_autohide_set_show_callback(AUTOHIDE_CB_FUNC cb_fxn, gpointer data)
{
    if (!iah.object) return -1;
    
    iah.show_cb = cb_fxn;
    iah.show_data = data;
    return 0;
}


gint vw_autohide_set_hide_callback(AUTOHIDE_CB_FUNC cb_fxn, gpointer data)
{
    if (!iah.object) return -1;

    iah.hide_cb = cb_fxn;
    iah.hide_data = data;
    return 0;
}

gint vw_autohide_unset_obj(NFOBJECT *obj)
{
    if (!iah.object) return -1;
    if (iah.object != obj) return -1;

    g_message("%s, %d, obj:%p", __FUNCTION__, __LINE__, obj);
    _delete_timer();
    iah.object = 0;
    return 0;
}

gint vw_autohide_start()
{
    iah.run = 1;
    g_timer_start(iah.elapsed_timer);
    return 0;
}

gint vw_autohide_stop()
{
    iah.run = 0;
    g_timer_stop(iah.elapsed_timer);
    return 0;
}

gint vw_autohide_occur_event(GdkEvent *event)
{
    if (!iah.object) return -1;
    if (smt_get_service() == SMT_NET_FW_UPGRADE) return -1;
    if (!iah.run) return -1;

    switch(event->type)
    {
        case NFEVENT_KEYPAD_PRESS:
        case NFEVENT_REMOCON_PRESS:
        {
            g_timer_start(iah.elapsed_timer);

            if (nfui_nfobject_is_shown(iah.object) == FALSE)
            {
                return 0;
            }
        }
        break;
    
        case GDK_SCROLL:
        case GDK_BUTTON_PRESS:
        case GDK_2BUTTON_PRESS:
        case GDK_BUTTON_RELEASE:
        case GDK_MOTION_NOTIFY:
        case NFEVENT_KEYPAD_RELEASE:
        case NFEVENT_REMOCON_RELEASE:       
        case NFEVENT_JOG_CHANGE:
        case NFEVENT_SHUTTLE_CHANGE:
        {
            g_timer_start(iah.elapsed_timer);
        
            if (nfui_nfobject_is_shown(iah.object) == FALSE)
            {
                nfui_nfobject_show(iah.object);
                if (iah.show_cb) iah.show_cb(iah.show_data);
                return 0;
            }
        }
        break;      

        default:
        break;
    }

    return 1;
}
