
#include "nf_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "objects/nffixed.h"
#include "objects/nflabel.h"
#include "objects/nftable.h"
#include "objects/nftab.h"
#include "objects/nfbutton.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfspinbutton.h"

#include "vw_s1_vca_setup_main.h"
#include "vw_s1_vca_internal.h"
#include "vw_dit_vca.h"
#include "vw_modify_point_popup.h"

#include "scm.h"
#include "uxm.h"

#define MOUSE_LEFT_BUTTON						(1)
#define MOUSE_RIGHT_BUTTON						(3)


typedef struct _S1_VCA_MEVENT_CALLBACK
{
	VCA_MEVENT_CB_FUNC 	cb_func;
	gpointer 			user_data;
} S1_VCA_MEVENT_CALLBACK;

static S1_VCA_MEVENT_CALLBACK g_mevent_cb[VCA_MEVENT_MAX] = {0, };

static NFWINDOW *g_curwnd = 0;
static gint g_cur_ch = 0;
static NFOBJECT *g_video_fixed = 0;
static NFOBJECT *g_video_label = 0;

static guint g_draw_tid = 0;

static gint g_va_read = 0;
static gint g_va_write = 0;

static VCA_SCREEN_INFO scr_info;
static VCA_MEVENT_PT mevt_pt;

static gint pre_dic_cnt = 0;
static DICPTR *pre_pdics = 0;

static gint g_force_update = 0;

	
static gboolean _draw_vca_rule(gpointer data)
{
	VCA_DP dp;
	VCA_CLON preClon;
	VCA_CLON postClon;	
	guint x, y;
	gint width, height;

	if (!g_va_write) return TRUE;
	if (!g_video_fixed) return TRUE;
	if (!nfui_nfobject_is_shown(g_video_fixed)) return TRUE;
	
	dp.drawable = nfui_nfobject_get_window(g_video_fixed);
	if (!dp.drawable) return TRUE;

	dp.gc = nfui_nfobject_get_gc(g_video_fixed);
	nfui_nfobject_get_offset(g_video_fixed, &dp.plt_area.x, &dp.plt_area.y);
	nfui_nfobject_get_size(g_video_fixed, &dp.plt_area.width, &dp.plt_area.height);

	preClon.dic_cnt = pre_dic_cnt;
	preClon.pdics = pre_pdics;		
	postClon.dic_cnt = 0;
	postClon.pdics = 0;	
	vw_dit_display_get_vca_diclist(&postClon);	

	if ((vw_dit_display_compare_vca_diclist(preClon, postClon) == 1) && (g_force_update != 1)) {
		vw_dit_display_free_vca_diclist(preClon);
		pre_dic_cnt = postClon.dic_cnt;
		pre_pdics = postClon.pdics;
    	nfui_nfobject_gc_unref(dp.gc);	
		return TRUE;
	}
	
	vw_dit_display_vca_erase(&dp, preClon, postClon);
	vw_dit_display_free_vca_diclist(preClon);

	vw_dit_display_vca_draw(&dp, postClon);
	pre_dic_cnt = postClon.dic_cnt;
	pre_pdics = postClon.pdics;

	g_force_update = 0;

	nfui_nfobject_gc_unref(dp.gc);

	return TRUE;
}

static gboolean _init_vca_rule(gpointer data)
{
	if (!g_draw_tid) g_draw_tid = g_timeout_add(40, _draw_vca_rule, 0);
	return FALSE;
}

static gboolean post_video_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_EXPOSE) 
	{	
		g_force_update = 1;
	}
	else if (evt->type == INFY_VCA_SELECT_MODIFY_POINT)
	{
		CMM_MESSAGE_T *pmsg;
		gint retVal = 0;
		guint enable = 0x03;	// enable add, del point
		
		pmsg = (CMM_MESSAGE_T *)data;

		if (pmsg->param == 4) enable &= ~(1 << DEL_POINT);
		if (pmsg->param == 8) enable &= ~(1 << ADD_POINT);

		retVal = VW_modify_point_popup_open(g_curwnd, mevt_pt.x, mevt_pt.y, enable);

		if (retVal == ADD_POINT) 		
			g_mevent_cb[VCA_MEVENT_ADD_POINT].cb_func(&scr_info, &mevt_pt, g_mevent_cb[VCA_MEVENT_ADD_POINT].user_data);
		else if (retVal == DEL_POINT) 
			g_mevent_cb[VCA_MEVENT_DEL_POINT].cb_func(&scr_info, &mevt_pt, g_mevent_cb[VCA_MEVENT_DEL_POINT].user_data);
	}
	else if (evt->type == GDK_DELETE)  
	{
		VCA_CLON clon;
	
		g_curwnd = 0;
		g_video_fixed = 0;

		clon.dic_cnt = pre_dic_cnt;
		clon.pdics = pre_pdics;	

		vw_dit_display_free_vca_diclist(clon);	
		pre_dic_cnt = 0;
		pre_pdics = 0;	

		g_force_update = 0;

		if (g_draw_tid) 
		{
			g_source_remove(g_draw_tid);
			g_draw_tid = 0;
		}

		uxm_unreg_imsg_event(obj, INFY_VCA_SELECT_MODIFY_POINT);		
	}

	return FALSE;
}

static gboolean post_video_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint property = -1;

	if (!g_va_read) return FALSE;

	if (evt->type == GDK_EXPOSE) 
	{	
		g_message(">>>>>>>>>>> %s, %d", __FUNCTION__, __LINE__);
	}
	else if (evt->type == GDK_BUTTON_PRESS)
	{
		GdkEventButton *bevent; 

		bevent = (GdkEventButton*)evt;

		if (bevent->button == MOUSE_LEFT_BUTTON) property = VCA_MEVENT_LEFT_PRESS;
		else if (bevent->button == MOUSE_RIGHT_BUTTON) property = VCA_MEVENT_RIGHT_PRESS;

		nfui_nfobject_get_offset(obj, &scr_info.x, &scr_info.y);
		nfui_nfobject_get_size(obj, &scr_info.w, &scr_info.h);
		mevt_pt.x = (gint)bevent->x;
		mevt_pt.y = (gint)bevent->y;

		g_force_update = 1;
	}
	else if (evt->type == GDK_MOTION_NOTIFY)
	{
		GdkEventMotion *mevent;

		mevent = (GdkEventMotion*)evt;

		if (mevent->state & GDK_BUTTON1_MASK) property = VCA_MEVENT_DRAG;
		else property = VCA_MEVENT_MOVE;

		nfui_nfobject_get_offset(obj, &scr_info.x, &scr_info.y);
		nfui_nfobject_get_size(obj, &scr_info.w, &scr_info.h);
		mevt_pt.x = (gint)mevent->x;
		mevt_pt.y = (gint)mevent->y;
	}	
	else if (evt->type == GDK_BUTTON_RELEASE)
	{
		GdkEventButton *bevent; 

		bevent = (GdkEventButton*)evt;

		property = VCA_MEVENT_RELEASE;

		nfui_nfobject_get_offset(obj, &scr_info.x, &scr_info.y);
		nfui_nfobject_get_size(obj, &scr_info.w, &scr_info.h);
		mevt_pt.x = (gint)bevent->x;
		mevt_pt.y = (gint)bevent->y;	

		g_force_update = 1;
	}
	else if (evt->type == GDK_2BUTTON_PRESS)
	{
		GdkEventButton *bevent; 

		bevent = (GdkEventButton*)evt;

		if (bevent->button == MOUSE_LEFT_BUTTON) property = VCA_MEVENT_LEFT_2PRESS;
		else if (bevent->button == MOUSE_RIGHT_BUTTON) property = VCA_MEVENT_RIGHT_2PRESS;

		nfui_nfobject_get_offset(obj, &scr_info.x, &scr_info.y);
		nfui_nfobject_get_size(obj, &scr_info.w, &scr_info.h);
		mevt_pt.x = (gint)bevent->x;
		mevt_pt.y = (gint)bevent->y;

		g_force_update = 1;
	}	

	if ((property >= 0) && (g_mevent_cb[property].cb_func))
	{
		g_mevent_cb[property].cb_func(&scr_info, &mevt_pt, g_mevent_cb[property].user_data);
	}

	return FALSE;
}


NFOBJECT* _S1_VCA_RuleVideo_Page(NFOBJECT *parent, gint page_x, gint page_y, gint page_w, gint page_h)
{
	NFOBJECT *fixed;
	NFOBJECT *obj;	

	g_cur_ch = 0;
	g_va_read = 0;
	g_va_write = 0;

	g_curwnd = (NFWINDOW *)nfui_nfobject_get_top(parent);

	fixed = (NFOBJECT*)nfui_nffixed_new();
//	nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(fixed, page_w, page_h);
	nfui_nfobject_show(fixed);
	nfui_nffixed_put((NFFIXED*)parent, fixed, page_x, page_y);
	nfui_regi_post_event_callback(fixed, post_video_fixed_event_handler);	

	g_video_fixed = fixed;

	uxm_reg_imsg_event(fixed, INFY_VCA_SELECT_MODIFY_POINT);		

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(662));
	nfui_nflabel_set_drawing_outline((NFLABEL*)obj, FALSE);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
	nfui_nfobject_set_size(obj, page_w, page_h);
	nfui_nfobject_use_hierarchy(obj, NFOBJECT_HIERARCHY_OFF);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 0, 0);
	nfui_regi_post_event_callback(obj, post_video_label_event_handler);	
	nfui_nfobject_show(obj);
	g_video_label = obj;

	g_timeout_add(1500, _init_vca_rule, 0);

	return g_video_fixed;
}

gint _S1_VCA_RuleVideo_set_channel(gint ch)
{
	gint i;
	VCA_CLON clon;

	if (!g_video_fixed) return -1;

	clon.dic_cnt = pre_dic_cnt;
	clon.pdics = pre_pdics;
	vw_dit_display_free_vca_diclist(clon);	
	
	pre_dic_cnt = 0;
	pre_pdics = 0;	
	g_cur_ch = ch;	
	return 0;
}

gint _S1_VCA_RuleVideo_set_preview_on()
{
	if (!g_video_fixed) return -1;

	nfui_nfobject_modify_bg(g_video_label, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
	nfui_signal_emit(g_video_label, GDK_EXPOSE, TRUE);
	return 0;
}

gint _S1_VCA_RuleVideo_set_preview_off()
{
	if (!g_video_fixed) return -1;

	nfui_nfobject_modify_bg(g_video_label, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_808080));
	nfui_signal_emit(g_video_label, GDK_EXPOSE, TRUE);
	return 0;
}

gint _S1_VCA_RuleVideo_set_mevent(gint read, gint write)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	gint x, y, width, height;
	
	if (!g_video_fixed) return -1;

	g_va_read = read;
	g_va_write = write;

	if (!nfui_nfobject_is_shown(g_video_fixed)) return 0;

	drawable = nfui_nfobject_get_window(g_video_fixed);
	if (!drawable) return TRUE;

	gc = nfui_nfobject_get_gc(g_video_fixed);
	nfui_nfobject_get_offset(g_video_fixed, &x, &y);
	nfui_nfobject_get_size(g_video_fixed, &width, &height);
	
	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_000000)));
	gdk_draw_rectangle(drawable, gc, TRUE, x, y, width, height);	
	nfui_nfobject_gc_unref(gc);		
	return 0;
}

gint VW_S1_VCA_attach_mevent(VCA_MEVENT_E mevt_type, VCA_MEVENT_CB_FUNC mevent_cb, gpointer user_data)
{
	if (mevt_type >= VCA_MEVENT_MAX) return -1;

	g_mevent_cb[mevt_type].cb_func = mevent_cb;
	g_mevent_cb[mevt_type].user_data = user_data;
	return 0;
}

