#include <glib.h>
#include <assert.h>
#include "iux_afx.h"

#include "objects/nflabel.h"
#include "objects/nfwindow.h"
#include "ix_func.h"
#include "thumbnail_manager.h"
#include "uxm.h"
#include "nf_api_play.h"
#include "evt.h"
#include "smt.h"
#include "vsm.h"


#define DBG_LEVEL		0
#define DBG_MODULE		"THUMBNAIL"


#define TH_MAX					(150)
#define THUMBNAIL_RGB_BIT		(24)


////////////////////////////////////////////////////////////
//
// protected data type 
//

typedef struct _SLOT_T {
	NFTHUMBNAIL 	*obj;
	guchar 			*image;

	gint			ch;					// channel
	time_t 			s_time;				// start time
	time_t 			e_time;				// end time
	
	time_t 			f_time;				// find time
	gboolean		result;				// find result
	gboolean		covert;				// covert on/off
} SLOT_T;

typedef struct _THUMBNAIL_T {
	SLOT_T 			slot[TH_MAX];
	GMutex 			*lock;
	TH_INST_E		inst;
	TH_STATE_E 		state;
	guint           covert_mask;
	gboolean        covert_shownas;
} THUMBNAIL_T;



////////////////////////////////////////////////////////////
//
// private variable
//

static THUMBNAIL_T ith;
volatile static gint geo_width, geo_height;


////////////////////////////////////////////////////////////
//
// private interfaces 
//


static void _slot_init(gint i)
{
	ith.slot[i].obj = NULL;
	ith.slot[i].image = NULL;

	ith.slot[i].ch = -1;
	ith.slot[i].s_time = 0;
	ith.slot[i].e_time = 0;
	
	ith.slot[i].f_time = 0;
	ith.slot[i].result = FALSE;
	ith.slot[i].covert = FALSE;	
}

static void _manager_init()
{
	gint i;

	for (i = 0; i < TH_MAX; i++)
	{
		_slot_init(i);
	}

	ith.lock = g_mutex_new();
	ith.inst = TH_CLOSE;
	ith.state = TH_STOP;

}

static gint _find_slot_id(NFTHUMBNAIL *thumbnail)
{
	gint i;

	for (i = 0; i < TH_MAX; i++)
	{
		if (ith.slot[i].obj == thumbnail)
			break;
	}

	return i;
}

static void _manager_lock()
{
	g_mutex_lock (ith.lock);
}

static void _manager_unlock()
{
	g_mutex_unlock (ith.lock);
}

static TH_INST_E _get_inst(void)
{
	return ith.inst;
}

static TH_STATE_E _get_state(void)
{
	return ith.state;
}

static NFTHUMBNAIL *_get_thumbnail_obj(guint i)
{
    if (!ith.slot[i].obj) return 0;
    if (!nfui_nfobject_is_valid(ith.slot[i].obj)) return 0;

	return ith.slot[i].obj;
}

static guchar *_get_image_buf(guint i)
{
	return ith.slot[i].image;
}

static gint _check_valid_obj(SLOT_T *slot)
{
    if (!slot->obj) return -1;
    if (!nfui_nfobject_is_valid(slot->obj)) return -1;
	if (!nfui_nfobject_is_shown(slot->obj)) return -1;

    return 0;
}

static gint _set_thumbnail_geometry(SLOT_T *slot, gint *geo_w, gint *geo_h)
{
    gint width, height;

    if (!slot->obj) return -1;
    if (!nfui_nfobject_is_valid(slot->obj)) return -1;

    width = slot->obj->image_width; 
    height = slot->obj->image_height;

	if ((width < 1) || (height < 1))
	{
		DMSG(1, "thumbnail size fail");
        return -1;
	}

	if (width%2 == 1)
	{
		DMSG(1, "make even number");
        return -1;
	}

	if (width < 160) width = 160;	// for resol 1280x720, 1024x768
	if (height < 96) height = 96;	// for resol 1280x720, 1024x768

    if ((*geo_w == width) && (*geo_h == height)) return 0;

    if (!nf_play_set_thumbnail_geometry(0, 0, width, height))
	{
		DMSG(1, "warning geometry change fail");
        return -1;
	}

	*geo_w = width;
	*geo_h = height;

	g_usleep(30000);

	return 0;
}

static gint _get_thumbnail_image(SLOT_T *slot, gint geo_w, gint geo_h)
{
	GTimeVal s_tv = {0, 0};
	GTimeVal e_tv = {0, 0};
	GTimeVal data_tv = {0, 0};
	gboolean ret_val;

	s_tv.tv_sec = slot->s_time;
	e_tv.tv_sec = slot->e_time;

	if (vsm_get_vmode() == VMODE_PB)
	{
		g_message("%s, %d", __FUNCTION__, __LINE__);
    	slot->f_time = 0;
    	slot->result = FALSE;
    	slot->covert = FALSE; 
	}
    else if (ith.covert_mask & (1 << slot->ch))
    {
    	slot->f_time = 0;
    	slot->result = FALSE;
    	slot->covert = TRUE;    	
    }
    else
    {
    	ret_val = nf_play_get_thumbnail(slot->ch+32, s_tv, e_tv, geo_w, geo_h, THUMBNAIL_RGB_BIT, (gpointer)slot->image, &data_tv);
    	slot->f_time = data_tv.tv_sec;
    	slot->result = ret_val;
    	slot->covert = FALSE;    	
    }    
	
	return 0;
}

static gint _send_result(SLOT_T *slot)
{
    evt_send_to_local(INFY_THUMBNAIL_CMPL_OBJ, 0, 0, slot->obj);
	
	return 0;
}

static void _send_expose_signal(NFTHUMBNAIL *obj, NFOBJECT *receive_obj)
{
	GdkEvent *evt;
	GdkWindow *dwnd;
	NFOBJECT  *nfwin;
	
	nfwin = nfui_nfobject_get_top((NFOBJECT *)obj);

	gdk_threads_enter();
	
	evt = gdk_event_new(GDK_CLIENT_EVENT);
	dwnd = ((NFWINDOW *)nfwin)->main_widget->window;
	g_object_ref(dwnd);
	evt->client.window = dwnd;
	evt->client.data_format = 32; 
	evt->client.data.l[0] = GDK_EXPOSE;
	evt->client.data.l[1] = receive_obj;
	evt->client.data.l[2] = 0;
	evt->client.data.l[3] = 0;
	gdk_event_put(evt);
	gdk_event_free(evt);
	
	gdk_threads_leave();

}


static int _thumbnail_manager_run()
{
	guint i;

	_manager_init();

	while(1)
	{
		if (_get_inst() == TH_CLOSE)
		{
			if (_get_state() == TH_CHANGE)
				thumbnail_manager_set_state(TH_STOP);			
		
			g_usleep(30000);
			continue;
		}

		if (_get_state() == TH_STOP)
		{
			g_usleep(30000);
			continue;
		}

		if (_get_state() == TH_CHANGE)
		{
			thumbnail_manager_set_state(TH_START);
		}

        geo_width = 0;
        geo_height = 0;

        ith.covert_mask = ssm_get_covert_mask();
        ith.covert_shownas = ssm_get_covert_shown_as();

		for (i = 0; i < TH_MAX; i++)
		{
			if (_get_inst() == TH_CLOSE) break;

            if (_check_valid_obj(&ith.slot[i]) == -1) continue;
            if (_set_thumbnail_geometry(&ith.slot[i], &geo_width, &geo_height) == -1) continue;
	
            _get_thumbnail_image(&ith.slot[i], geo_width, geo_height);
            _send_result(&ith.slot[i]);

			g_usleep(30000);
			
			if (_get_state() == TH_CHANGE) break;			
		}

		if (_get_state() != TH_CHANGE)
		{
			thumbnail_manager_set_state(TH_STOP);
			evt_send_to_local(INFY_THUMBNAIL_TOTALLY_CMPL, 0, 0, 0);
		}
	}

	return 0;
}


static void _thumbnail_manager_priority_change()
{
	int policy;
	struct sched_param sched;
	pthread_t thread;

#if 0
	policy = SCHED_FIFO;
	thread = pthread_self();
	sched.sched_priority = sched_get_priority_min(policy);
	g_message("%s pthread_setschedparam ret = %d", __FUNCTION__, pthread_setschedparam (thread, policy, &sched));
	g_message("%s pthread_getschedparam ret = %d", __FUNCTION__, pthread_getschedparam (thread, &policy, &sched));
	g_message("%s XXX ugienf set realtime policy = %d", __FUNCTION__, policy);
#endif	
}


static int thumbnail_manager_main()
{
	_thumbnail_manager_priority_change();
	_thumbnail_manager_run();
	
	return 0;
}


////////////////////////////////////////////////////////////
//
//	public interfaces
//

void start_thumbnail_manager()
{
	GError *error = NULL;
	GThread *thread = NULL;

	thread = g_thread_create ((GThreadFunc) thumbnail_manager_main, NULL, FALSE, &error);

	if (thread == NULL) {
		g_warning ("Error thread create: %s", error->message);
		g_error_free (error);
	}	

}


void thumbnail_manager_object_connect(NFTHUMBNAIL *thumbnail)
{
	gint i;

	i = _find_slot_id(NULL);

	if (i == TH_MAX)
	{
		DMSG(1, "%d, did't have empty slot_id", __LINE__);
		g_assert(0);
	}


//	DMSG(1, "%d, new slot id : %d, %p", __LINE__, i, thumbnail);
	ith.slot[i].obj = thumbnail;
}

void thumbnail_manager_obj_disconnect(NFTHUMBNAIL *thumbnail)
{
	gint i;

	i = _find_slot_id(thumbnail);

	if (i == TH_MAX)
	{
		DMSG(1, "%d, did't correct slot_id", __LINE__);
		g_assert(0);
	}

//	DMSG(1, "%d, remove slot id : %d, %p", __LINE__, i, thumbnail);
	_slot_init(i);
}

void thumbnail_manager_set_inst(TH_INST_E inst)
{
	ith.inst = inst;

//	DMSG(1, "%s, %d, inst:%d", __FUNCTION__, __LINE__, ith.inst);
}

void thumbnail_manager_set_state(TH_STATE_E state)
{
	_manager_lock();

	ith.state = state;

	_manager_unlock();

//	DMSG(1, "%s, %d, state:%d", __FUNCTION__, __LINE__, state);
}

void thumbnail_manager_attach_image_buf(NFTHUMBNAIL *thumbnail, guchar *buf)
{
	gint i;

	i = _find_slot_id(thumbnail);

	if (i == TH_MAX)
	{
		DMSG(1, "%d, did't correct slot_id", __LINE__);
		g_assert(0);
	}

//	DMSG(1, "%d, image buf attach slot id : %d, %p", __LINE__, i, buf);
	ith.slot[i].image = buf;
}

void thumbnail_manager_attach_ch(NFTHUMBNAIL *thumbnail, gint ch)
{
	gint i;

	i = _find_slot_id(thumbnail);

	if (i == TH_MAX)
	{
		DMSG(1, "%d, did't correct slot_id", __LINE__);
		g_assert(0);
	}
	
	ith.slot[i].ch = ch;
}

void thumbnail_manager_attach_period(NFTHUMBNAIL *thumbnail, time_t start_time, time_t end_time)
{
	gint i;

	i = _find_slot_id(thumbnail);

	if (i == TH_MAX)
	{
		DMSG(1, "%d, did't correct slot_id", __LINE__);
		g_assert(0);
	}
	
	ith.slot[i].s_time = start_time;
	ith.slot[i].e_time = end_time;
}

void thumbnail_manager_get_period(NFTHUMBNAIL *thumbnail, time_t *start_time, time_t *end_time)
{
	gint i;

	i = _find_slot_id(thumbnail);

	if (i == TH_MAX)
	{
		DMSG(1, "%d, did't correct slot_id", __LINE__);
		g_assert(0);
	}
	
	*start_time = ith.slot[i].s_time;
	*end_time = ith.slot[i].e_time;
}

gboolean thumbnail_manager_get_result(NFTHUMBNAIL *thumbnail)
{
	gint i;

	i = _find_slot_id(thumbnail);

	if (i == TH_MAX)
	{
		DMSG(1, "%d, did't correct slot_id", __LINE__);
		g_assert(0);
	}

	return ith.slot[i].result;
}

gboolean thumbnail_manager_get_covert(NFTHUMBNAIL *thumbnail)
{
	gint i;

	i = _find_slot_id(thumbnail);

	if (i == TH_MAX)
	{
		DMSG(1, "%d, did't correct slot_id", __LINE__);
		g_assert(0);
	}

	return ith.slot[i].covert;
}

gboolean thumbnail_manager_get_covert_shown_as()
{
    return ith.covert_shownas;
}

gboolean thumbnail_manager_get_time(NFTHUMBNAIL *thumbnail, time_t *img_tv)
{
	gint i;
	guint ch;

	i = _find_slot_id(thumbnail);

	if (i == TH_MAX)
	{
		DMSG(1, "%d, did't correct slot_id", __LINE__);
		g_assert(0);
	}

	if (ith.slot[i].result)
	{
		*img_tv = ith.slot[i].f_time;
		return TRUE;
	}
	else
	{
		DMSG(1, "%d, wrong focus", __LINE__);
		return FALSE;
	}
	
}

// 0 : fail - no image
// 1 : success.
// 2 : thumbnail loading
gint thumbnail_manager_get_focused_time(NFTHUMBNAIL *thumbnail, time_t *img_tv)
{
	gint i;
	guint ch;

	i = _find_slot_id(thumbnail);

	if (i == TH_MAX)
	{
		DMSG(1, "%d, did't correct slot_id", __LINE__);
		g_assert(0);
	}

	if (ith.slot[i].result)
	{
		if (_get_state() != TH_STOP)
			return 2;

		*img_tv = ith.slot[i].f_time;
		return 1;
	}

	return 0;	
}

gboolean thumbnail_manager_get_start_time(NFTHUMBNAIL *thumbnail, time_t *img_tv)
{
	gint i;
	guint ch;

	i = _find_slot_id(thumbnail);

	if (i == TH_MAX)
	{
		DMSG(1, "%d, did't correct slot_id", __LINE__);
		g_assert(0);
	}

	*img_tv = ith.slot[i].s_time;
	return TRUE;	
}

gboolean thumbnail_manager_is_running()
{
	if (_get_state() != TH_STOP)
		return TRUE;
	else
		return FALSE;
}

// UX THREAD RUNNING
void thumbnail_manager_own_image_load(NFTHUMBNAIL *thumbnail)
{
	GdkDrawable 	*drawable;
	GdkGC 			*gc;

	NFTHUMBNAIL *obj;
	guchar *buf;
	
	gint i;
	gint image_pos_x, image_pos_y;

	i = _find_slot_id(thumbnail);

	if (i == TH_MAX)
	{
		DMSG(1, "%d, did't correct slot_id", __LINE__);
		g_assert(0);
	}

	obj = _get_thumbnail_obj(i);
	buf = _get_image_buf(i);

    if (!obj) return;
    if (!buf) return;

	drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
	gc = nfui_nfobject_get_gc((NFOBJECT*)obj);

	nfui_nfobject_get_offset((NFOBJECT*)obj, &image_pos_x, &image_pos_y);

	image_pos_x += THUMBNAIL_LINE_BORDER;
	image_pos_y += obj->subject_label_h + THUMBNAIL_LINE_BORDER;

	if (geo_width != obj->image_width || geo_height != obj->image_height)
	{
		// 원본 이미지의 Pixbuf 생성
		GdkPixbuf *src_pixbuf = gdk_pixbuf_new_from_data(
			buf,
			GDK_COLORSPACE_RGB,
			FALSE,
			8,
			geo_width,
			geo_height,
			geo_width * 3,
			NULL,
			NULL
		);

		if (!src_pixbuf) {
			nfui_nfobject_gc_unref(gc);
			return;
		}

		// 스케일된 Pixbuf 생성
		GdkPixbuf *scaled_pixbuf = gdk_pixbuf_scale_simple(
			src_pixbuf,
			obj->image_width,
			obj->image_height,
			GDK_INTERP_BILINEAR
		);

		if (scaled_pixbuf) {
			gdk_draw_pixbuf(
				drawable,
				gc,
				scaled_pixbuf,
				0, 0,
				image_pos_x, image_pos_y,
				obj->image_width, obj->image_height,
				GDK_RGB_DITHER_NONE,
				0, 0
			);
			g_object_unref(scaled_pixbuf);
		}

		g_object_unref(src_pixbuf);
		nfui_nfobject_gc_unref(gc);
	}
	else
	{
		gdk_draw_rgb_image(drawable, 
							gc, 
							image_pos_x, 
							image_pos_y, 
							obj->image_width,
							obj->image_height,
							GDK_RGB_DITHER_NONE,
							buf,
							(obj->image_width)*3 );

		nfui_nfobject_gc_unref(gc);
	}
}
