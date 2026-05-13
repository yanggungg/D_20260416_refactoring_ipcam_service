#include "nf_afx.h"
#include "services/scm.h"
#include "tools/nf_ui_tool.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "support/multi_language_support.h"

#include "modules/ssm.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nftimespin.h"
#include "objects/nfcheckbutton.h"
#include "objects/nflistbox.h"
#include "objects/nfspinbutton.h"
#include "objects/nfbutton.h"
#include "objects/nfthumbnail.h"
#include "../viewers/objects/cw_calendar.h"
#include "viewers/objects/nfcombobox.h"

#include "viewers/vw_internal.h"
#include "vw_delete.h"

#include "uxm.h"
#include "iux_msg.h"
#include "iux_afx.h"

#include "vw_set_date_time.h"
#include "viewers/objects/ixtimeline.h"

#include "scm.h"
#include "stm.h"
#include "dtf.h"
#include "smt.h"
#include "qry.h"
#include "ix_func.h"

#define PAGE_FIXED_CNT          4
#define ROW_CNT_PER_PAGE        (GUI_CHANNEL_CNT / PAGE_FIXED_CNT)


#define DBG_LEVEL		9
#define DBG_MODULE		"DELETE_TAB1"

#define	SBT_LABEL_HEIGHT1				(40)
#define	SBT_LABEL_HEIGHT2				(26)

// FIXED1, FIXED2, FIXED3
#define SBT_FIXED1_X					(0)
#define SBT_FIXED1_Y					(0)
#define SBT_FIXED1_W					(440)
#define SBT_FIXED1_H					(903)

#define SBT_FIXED2_X					(SBT_FIXED1_X + SBT_FIXED1_W)
#define SBT_FIXED2_Y					(SBT_FIXED1_Y)
#define SBT_FIXED2_W					(1416)
#define SBT_FIXED2_H                    (419 + 266)

#define SBT_FIXED3_X					(SBT_FIXED2_X)
#define SBT_FIXED3_Y					(SBT_FIXED2_Y+SBT_FIXED2_H)
#define SBT_FIXED3_W					(SBT_FIXED2_W)
#define SBT_FIXED3_H                    (484 - 266)

// CALENDAR FIXED
#define SBT_CAL_FIXED_X					(12)
#define SBT_CAL_FIXED_Y					(34)
#define SBT_CAL_FIXED_W					(410)
#define SBT_CAL_FIXED_H					(380)

#define SBT_CAL_BUTTON_SIZE_W			(130)
#define SBT_CAL_BUTTON_GAP				(46)

#define SBT_CAL_FIRST_BUTTON_X			(51)
#define SBT_CAL_FIRST_BUTTON_Y			(333)

#define SBT_CAL_LAST_BUTTON_X			(SBT_CAL_FIRST_BUTTON_X+SBT_CAL_BUTTON_SIZE_W+SBT_CAL_BUTTON_GAP)
#define SBT_CAL_LAST_BUTTON_Y			(SBT_CAL_FIRST_BUTTON_Y)

//TIMELINE CH BUTTON
#define SBT_TML_CH_CHECK_X				(15)
#define SBT_TML_CH_CHECK_Y				(34)

//TIMELINE CH BUTTON
#define SBT_TML_CH_LABEL_X				(SBT_TML_CH_CHECK_X + 18 + 1)
#define SBT_TML_CH_LABEL_Y				(SBT_TML_CH_CHECK_Y)
#define SBT_TML_CH_LABEL_W				(31)
#define SBT_TML_CH_LABEL_H				(18)

//TIME LINE BUTTON
#define TML_X							(SBT_TML_CH_LABEL_X + 31 + 1)
#define TML_Y							(SBT_TML_CH_LABEL_Y)
#define HEIGHT_RULER					(25)
#define HEIGHT_CH						(18)

#define SBT_TL_ZOOM_BTN_GAP				(1)
#define SBT_TL_BTN_GAP					(10)
#define SBT_TL_DIR_BTN_GAP				(1)

#define SBT_TL_ZOOMOUT_BTN_X			(SBT_TML_CH_CHECK_X + 1096)

//TIME LINE RECORD REASON
#define SBT_TL_REC_BOX_GAP_X			(6)

#define SBT_TL_REC_BOX_Y				(7)
#define SBT_TL_REC_BOX_W				(22)
#define SBT_TL_REC_BOX_H				(22)

#define SBT_TL_REC_TEXT_PANIC_W			(174)//(84)
#define SBT_TL_REC_TEXT_PANIC_X			(SBT_FIXED2_W - SBT_TL_REC_TEXT_PANIC_W)

#define SBT_TL_REC_BOX_PANIC_X			(SBT_TL_REC_TEXT_PANIC_X - SBT_TL_REC_BOX_GAP_X - SBT_TL_REC_BOX_W)
#define SBT_TL_REC_BOX_PANIC_Y			(SBT_TL_REC_BOX_Y)

#define SBT_TL_REC_TEXT_MOT_W			(174)//(105)
#define SBT_TL_REC_TEXT_MOT_X			(SBT_TL_REC_BOX_PANIC_X - SBT_TL_REC_TEXT_MOT_W)

#define SBT_TL_REC_BOX_MOT_X			(SBT_TL_REC_TEXT_MOT_X - SBT_TL_REC_BOX_GAP_X - SBT_TL_REC_BOX_W)
#define SBT_TL_REC_BOX_MOT_Y			(SBT_TL_REC_BOX_Y)

#define SBT_TL_REC_TEXT_ALARM_W			(174)//(94)
#define SBT_TL_REC_TEXT_ALARM_X			(SBT_TL_REC_BOX_MOT_X - SBT_TL_REC_TEXT_ALARM_W)

#define SBT_TL_REC_BOX_ALARM_X			(SBT_TL_REC_TEXT_ALARM_X - SBT_TL_REC_BOX_GAP_X - SBT_TL_REC_BOX_W)
#define SBT_TL_REC_BOX_ALARM_Y			(SBT_TL_REC_BOX_Y)

#define SBT_TL_REC_TEXT_TIME_W			(174)//(153)
#define SBT_TL_REC_TEXT_TIME_X			(SBT_TL_REC_BOX_ALARM_X - SBT_TL_REC_TEXT_TIME_W)

#define SBT_TL_REC_BOX_TIME_X			(SBT_TL_REC_TEXT_TIME_X - SBT_TL_REC_BOX_GAP_X - SBT_TL_REC_BOX_W)
#define SBT_TL_REC_BOX_TIME_Y			(SBT_TL_REC_BOX_Y)

#define SBT_TL_REC_TEXT_PRE_W			(174)
#define SBT_TL_REC_TEXT_PRE_X			(SBT_TL_REC_BOX_TIME_X - SBT_TL_REC_TEXT_PRE_W)

#define SBT_TL_REC_BOX_PRE_X			(SBT_TL_REC_TEXT_PRE_X - SBT_TL_REC_BOX_GAP_X - SBT_TL_REC_BOX_W)
#define SBT_TL_REC_BOX_PRE_Y			(SBT_TL_REC_BOX_Y)




// THUMBNAIL PICTURE
#if defined(GUI_32CH_SUPPORT)
#define THUMB_PIXEL_ALIGNMENT           (16)
#define THUMB_CELL_W        ((THUMB_PIXEL_ALIGNMENT * 10) + 2)
#define THUMB_CELL_H        ((THUMB_PIXEL_ALIGNMENT * 7) + 2)
#define THUMB_COL_CNT       (8)
#elif defined(GUI_16CH_SUPPORT)
#define THUMB_CELL_W		(172)
#define THUMB_CELL_H		(114)
#define THUMB_COL_CNT		(8)

#else
#define THUMB_CELL_W		(342)
#define THUMB_CELL_H        (207) //210
#define THUMB_COL_CNT		(4)
#endif

#define THUMB_CELL_COL_GAP  15 // 2
#define THUMB_RGB_BIT		24
#define DATE_TIME_TEXT_LEN	64



////////////////////////////////////////////////////////////
//
// private data types 
//

enum {
	DF_YMD = 0,
	DF_MDY,
	DF_DMY,

	NUM_DATE_FORMATS,
};


////////////////////////////////////////////////////////////
//
// private variables 
//

static NFWINDOW *g_curwnd = 0;
static gboolean g_noupdate_tml = FALSE;
static IXTIMELINE *tml;

// Current date
static NFOBJECT *cld_text = NULL;

static NFOBJECT *from_datetime_obj;
static NFOBJECT *to_datetime_obj;
static time_t from_utime, to_utime;

static NFOBJECT *fixed2;
static NFOBJECT *left_fixed;
static NFOBJECT *tag;
static NFOBJECT *preview_time_obj;
static NFOBJECT *first_btn;
static NFOBJECT *last_btn;

static CWCALENDAR *cld;
static NFOBJECT *thumb_obj[GUI_CHANNEL_CNT];
static NFOBJECT *ch_sel[GUI_CHANNEL_CNT];
static NFOBJECT *ch_all;

static NFOBJECT *erasebtn_obj;
static NFOBJECT *preview_fixed;

static NFOBJECT *close_obj;

static NFOBJECT *cld_prev;
static NFOBJECT *cld_next;

static NFOBJECT *tml_ref;
	
static guchar ch_image[GUI_CHANNEL_CNT][(THUMB_CELL_H-17-1)*(THUMB_CELL_W-2)*3];
static gboolean g_init_thumb = TRUE;

static NFOBJECT     *wait_pop = NULL;
static guint g_thumb_tmr = 0;

static guint g_thumb_dir = 0;
static NFOBJECT *g_page_fixed[PAGE_FIXED_CNT];
static NFOBJECT *g_lb_page_num;


////////////////////////////////////////////////////////////
//
// private functions 
//


static void _set_thumbnail_time(void)
{
	gint i;
	gint spp;

   	spp = tml_get_spp(tml);

    if (tml_is_set_section(tml))
    {
        
		if (g_thumb_dir == 0x01)	// changed start time 		
		{
    	for (i = 0 ; i < GUI_CHANNEL_CNT ; i++)
	    		nfui_nfthumbnail_set_period(thumb_obj[i], from_utime, from_utime+spp);
		}
		else if (g_thumb_dir == 0x10)	// changed end time 	
		{
	    	for (i = 0 ; i < GUI_CHANNEL_CNT ; i++)
	    		nfui_nfthumbnail_set_period(thumb_obj[i], to_utime, to_utime+spp);
		}
		else	// changed start,end time 
		{
	    	for (i = 0 ; i < GUI_CHANNEL_CNT ; i++)
	    		nfui_nfthumbnail_set_period(thumb_obj[i], to_utime, to_utime+spp);
		}		
    }
    else
    {
    	for (i = 0 ; i < GUI_CHANNEL_CNT ; i++)
    		nfui_nfthumbnail_set_period(thumb_obj[i], from_utime, from_utime+spp);
    }
}

static void _remove_waitbox(void)
{
	if(wait_pop)
	{
		nftool_remove_waitbox(wait_pop);
		wait_pop = NULL;
	}
}

static void _wait_for_thumb_stop(void)
{   
	while (1) {
		usleep(100000);
		if (!nfui_nfthumbnail_check_running()) break;
	}
	return 0;
}

static gint _set_all_chk_btn(NFOBJECT *all, NFOBJECT *unit[], gboolean expose)
{
	gboolean state;
	gint i;

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
	    state = nfui_check_button_get_active((NFCHECKBUTTON*)unit[i]);

		if (!state)
		{
		    if (expose)
                nfui_check_button_set_active((NFCHECKBUTTON*)all, FALSE);
            else
                nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)all, FALSE);
            return 0;
        }
	}

	if (i == GUI_CHANNEL_CNT)
	{
	    if (expose)
    	    nfui_check_button_set_active((NFCHECKBUTTON*)all, TRUE);
	    else
	        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)all, TRUE);
    }

    return 0;
}

static int _get_calendar_day(int *year, int *mon, int *day)
{
	if (year) *year	= cw_cld_get_current_year((CWCALENDAR*)cld);
	if (mon) *mon = cw_cld_get_current_month((CWCALENDAR*)cld);
	if (day) *day  = cw_cld_get_current_day((CWCALENDAR*)cld);
	return 0;
}

static int _update_calendar_text(time_t timet)
{
	char buf[64];
	int year, mon;
	int length;

	ifn_get_local_day(timet, &year, &mon, 0);
	
	length = strlen(g_month_str[mon - 1]);
	g_utf8_strncpy(buf, g_month_str[mon - 1], g_utf8_strlen(g_month_str[mon - 1], -1));
	g_sprintf(&buf[length], " %d", year);

	nfui_nflabel_set_text((NFLABEL*)cld_text, buf);
	nfui_signal_emit(cld_text, GDK_EXPOSE, FALSE);
	return 0;
}

static int _update_calendar_time(time_t timet)
{
	int year, mon, day;
	if (timet == 0) timet = time(0);
	dtf_get_local_day(timet, &year, &mon, &day);
	cw_cld_change_date((CWCALENDAR*)cld, year, mon, day);
	return 0;
}

static guint64 _get_video_size(NF_ARCH_AVI_INFO  *qinfo)
{
	guint64 vsize = 0;
	int i;

	for (i = 0; i < var_get_ch_count(); i++) {
		if (qinfo->channel_mask & (1 << i))
			vsize += qinfo->ch_size[i];	
	}
	
	return vsize;	
}

static int _ready_thumbnail()
{
	NFOBJECT *rate_obj;
	gint i;
	gchar strBuf[10];

	rate_obj = (NFOBJECT*)nfui_nfobject_get_data(preview_fixed, "CMPL RATE");
	memset(strBuf, 0x00, sizeof(strBuf));
	g_sprintf(strBuf, "(0 / %d)", GUI_CHANNEL_CNT);
	nfui_nflabel_set_text((NFLABEL*)rate_obj, strBuf);
	nfui_signal_emit(rate_obj, GDK_EXPOSE, TRUE);

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		if (!nfui_nfobject_is_disabled(thumb_obj[i]))
		{
			nfui_nfobject_disable(thumb_obj[i]);
			nfui_signal_emit(thumb_obj[i], GDK_EXPOSE, TRUE);
		}
	}

	return 0;
}

static int _get_thumbnail()
{
	_set_thumbnail_time();
	nfui_nfthumbnail_get_image();
	return 0;
}

static int _update_thumbnail_rate(NFOBJECT *obj, gint cmpl_cnt)
{
	gchar strBuf[10];

	memset(strBuf, 0x00, sizeof(strBuf));
	g_sprintf(strBuf, "(%d / %d)", cmpl_cnt, GUI_CHANNEL_CNT);
	nfui_nflabel_set_text((NFLABEL*)obj, strBuf);
	nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
}

static int _update_thumbnail_info(NFOBJECT *obj)
{
	nfui_nfobject_enable(obj);
	nfui_signal_emit(obj, GDK_EXPOSE, TRUE);	
}

static void _change_obj_focus(NFOBJECT* from, NFOBJECT *to)
{
	nfui_set_key_focus(from, FALSE);
	nfui_set_key_focus(to, TRUE);

	nfui_signal_emit(from, GDK_EXPOSE, TRUE);
	nfui_signal_emit(to, GDK_EXPOSE, TRUE);
}

static gboolean pre_left_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) 
	{
		case GDK_EXPOSE:
		{
			GdkGC *gc;
			GdkDrawable *drawable;

			gint gap_x, gap_y;

			nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);

			drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
			gc = gdk_gc_new(drawable);

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(604));
			gdk_draw_rectangle(drawable, gc, TRUE, gap_x+obj->width-1, gap_y, 1, obj->height);

			g_object_unref(gc);
		}
		break;
	}

	return FALSE;
}

static unsigned int _get_ch_selecting_mask()
{
	int i;
	unsigned int mask = 0;

	for (i = 0; i < var_get_ch_count(); ++i) { 
		mask |= (((int)nfui_check_button_get_active(ch_sel[i])) << i);
	}
	return mask;
}

static gboolean post_fixed2_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
	case GDK_EXPOSE:
		tml_repaint(tml);
		break;
	}
	return FALSE;
}

static gboolean post_prev_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
    gchar buf[64];
    
    if(evt->type == GDK_BUTTON_RELEASE) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
          return FALSE;
        }

        for (i = 0; i < PAGE_FIXED_CNT; i++) {
            if (nfui_nfobject_is_shown((NFOBJECT*)g_page_fixed[i])) {
                break;
            }
        }

        if (i == PAGE_FIXED_CNT) return FALSE;

        if (i == 0) return FALSE;

        nfui_nfobject_hide(g_page_fixed[i]);
        
        _ready_thumbnail();
        _get_thumbnail();

        i--;
        
        memset(buf, 0x00, sizeof(buf));
        g_sprintf(buf, "%d / %d", i + 1, PAGE_FIXED_CNT);
        nfui_nflabel_set_text(g_lb_page_num, buf);
        nfui_signal_emit(g_lb_page_num, GDK_EXPOSE, TRUE);

        nfui_nfobject_show(g_page_fixed[i]);
        nfui_signal_emit(g_page_fixed[i], GDK_EXPOSE, TRUE);
    	
        nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }

    return FALSE;
}

static gboolean post_next_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
    gchar buf[64];

    if(evt->type == GDK_BUTTON_RELEASE) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
          return FALSE;
        }

        for (i = 0; i < PAGE_FIXED_CNT; i++) {
            if (nfui_nfobject_is_shown((NFOBJECT*)g_page_fixed[i])) {
                break;
            }
        }

        if (i == PAGE_FIXED_CNT) return FALSE;

        if (i == (PAGE_FIXED_CNT - 1)) return FALSE;

        nfui_nfobject_hide(g_page_fixed[i]);
        
        _ready_thumbnail();
        _get_thumbnail();

        i++;
                
        memset(buf, 0x00, sizeof(buf));
        g_sprintf(buf, "%d / %d", i + 1, PAGE_FIXED_CNT);
        nfui_nflabel_set_text(g_lb_page_num, buf);
        nfui_signal_emit(g_lb_page_num, GDK_EXPOSE, TRUE);

        nfui_nfobject_show(g_page_fixed[i]);
        nfui_signal_emit(g_page_fixed[i], GDK_EXPOSE, TRUE);

        nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    }

    return FALSE;
}


static gboolean pre_fixed2_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	unsigned int sel_mask = 0;
	GTimeVal begTime;
	CMM_MESSAGE_T *pmsg;

	memset(&begTime, 0x00, sizeof(GTimeVal));

	switch(evt->type)
	{
		case GDK_EXPOSE:
		{
			GdkGC *gc;
			GdkDrawable *drawable;
			GTimeVal schtime;
			gint gap_x, gap_y;

			nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);
			
			memset(&schtime, 0x00, sizeof(GTimeVal));
			stm_get_time(&schtime);

			drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
			gc = gdk_gc_new(drawable);

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(604));
			gdk_draw_rectangle(drawable, gc, TRUE, gap_x, gap_y+obj->height-1, obj->width, 1);

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(647));
			gdk_draw_rectangle(drawable,
					gc,
					TRUE,
					gap_x+SBT_TL_REC_BOX_PRE_X, gap_y+SBT_TL_REC_BOX_PRE_Y,
					SBT_TL_REC_BOX_W, SBT_TL_REC_BOX_H);

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(639));
			gdk_draw_rectangle(drawable,
					gc,
					TRUE,
					gap_x+SBT_TL_REC_BOX_TIME_X, gap_y+SBT_TL_REC_BOX_TIME_Y,
					SBT_TL_REC_BOX_W, SBT_TL_REC_BOX_H);

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(641));
			gdk_draw_rectangle(drawable,
					gc,
					TRUE,
					gap_x+SBT_TL_REC_BOX_ALARM_X, gap_y+SBT_TL_REC_BOX_ALARM_Y,
					SBT_TL_REC_BOX_W, SBT_TL_REC_BOX_H);

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(643));
			gdk_draw_rectangle(drawable,
					gc,
					TRUE,
					gap_x+SBT_TL_REC_BOX_MOT_X, gap_y+SBT_TL_REC_BOX_MOT_Y,
					SBT_TL_REC_BOX_W, SBT_TL_REC_BOX_H);

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(645));
			gdk_draw_rectangle(drawable,
					gc,
					TRUE,
					gap_x+SBT_TL_REC_BOX_PANIC_X, gap_y+SBT_TL_REC_BOX_PANIC_Y,
					SBT_TL_REC_BOX_W, SBT_TL_REC_BOX_H);

#if 0
			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(630));
			gdk_draw_rectangle(drawable,
					gc,
					TRUE,
					SBT_LEFT_GAP_X + SBT_FIXED2_X + SBT_TML_CH_BTN_X, 
					gap_y + SBT_FIXED2_Y + SBT_TML_CH_BTN_Y - 5,
					31, 6);
#endif

	 		g_object_unref(gc);
	 	}
		break;



	
	case GDK_DELETE:
		break;

		default :

		break;
	}

	return FALSE;
}

static gboolean _show_thumbnail(void *data)
{
	//if (!nfui_nfwindow_find("ARCHIVING")) return FALSE;

	vsm_live_stop();
	nfui_nfthumbnail_image_open();
	_ready_thumbnail();
	_get_thumbnail();
	g_init_thumb = FALSE;
	g_thumb_tmr = 0;
	return FALSE;
}


static gboolean pre_fixed3_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{

		case GDK_EXPOSE:
		break;

		case GDK_DELETE:
			if (g_thumb_tmr) g_source_remove(g_thumb_tmr);
			g_thumb_tmr = 0;
			g_init_thumb = TRUE;
			uxm_unreg_imsg_event(obj, INFY_THUMBNAIL_CMPL_OBJ);		
		break;

		case INFY_THUMBNAIL_CMPL_OBJ:
		{
			gint i;
			NFOBJECT *rate_obj;
			NFOBJECT *cmpl_obj = ((CMM_MESSAGE_T *)data)->data;
			gchar strBuf[10];
            gint s_ch, e_ch, page_idx;

			if (!nfui_nfobject_is_shown(obj))
				return FALSE;

            for (i = 0; i < PAGE_FIXED_CNT; i++)  {
                if (nfui_nfobject_is_shown(g_page_fixed[i])) break;
            }

            s_ch = THUMB_COL_CNT * i;
            e_ch = s_ch + THUMB_COL_CNT;
            
            for(i = s_ch; i < e_ch; i++)
			{
				if (cmpl_obj == thumb_obj[i])
					break;
			}

            if (i == e_ch)
				return FALSE;

            if (nfui_nfobject_is_disabled(thumb_obj[s_ch])
                && (i != s_ch))
				return;

			rate_obj = (NFOBJECT*)nfui_nfobject_get_data(obj, "CMPL RATE");
			_update_thumbnail_rate(rate_obj, i+1);
			_update_thumbnail_info(cmpl_obj);
		}
		break;
		
		default :
		break;
	}

	return FALSE;
}

static int _update_from_to_label(time_t from, time_t to)
{
	char from_buffer[100];
	char to_buffer[100];

	if (from) {
		dtf_get_local_datetime(from, from_buffer);
		nfui_nflabel_set_text((NFLABEL*)from_datetime_obj, from_buffer);
		nfui_signal_emit((NFOBJECT*)from_datetime_obj, GDK_EXPOSE, TRUE);
	}

	if (to) {
		dtf_get_local_datetime(to, to_buffer);
		nfui_nflabel_set_text((NFLABEL*)to_datetime_obj, to_buffer);
		nfui_signal_emit((NFOBJECT*)to_datetime_obj, GDK_EXPOSE, TRUE);
	}

	return 0;
}

static gboolean _is_to_time_displayed()
{
	char *tmp = nfui_nflabel_get_text((NFLABEL*)to_datetime_obj);
	return (strlen(tmp) > 0);
}

static gboolean post_from_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	time_t gt;
	time_t prev = from_utime;

	if(evt->type == GDK_BUTTON_RELEASE) {
		gt = VW_Set_DateTime_Open(g_curwnd, "FROM", 456, 572, from_utime, SDT_TYPE_SEC, NF_LOWER_TIMELIMIT, to_utime - 1);

		if (gt != 0) {	
			from_utime = gt;
			if (to_utime <= from_utime) to_utime = from_utime + 300;
			stm_set_time_t(from_utime);
			_update_calendar_time(from_utime);
			_update_calendar_text(from_utime);

			if (!ifn_is_same_day(prev, from_utime)) {
				tml_zoom_min(tml);
				tml_reset_cur_day(tml, from_utime);
			}

			tml_set_playstick_time_t(tml, from_utime);
			if (tml_is_set_section(tml)) {
				_update_from_to_label(from_utime, to_utime);
				tml_set_section_time_t(tml, from_utime, to_utime); 
			}
			else
				_update_from_to_label(from_utime, 0);

			tml_repaint(tml);
			_ready_thumbnail();
			_get_thumbnail();
		}		
	}
	
	return FALSE;
}

static gboolean post_to_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	time_t gt;
	
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if (!tml_is_set_section(tml)) to_utime = from_utime + 300;
		gt = VW_Set_DateTime_Open(g_curwnd, "TO", 456, 622, to_utime, SDT_TYPE_SEC, from_utime + 1, NF_UPPER_TIMELIMIT);

		if (gt != 0)
		{	
			to_utime = gt;
			if (from_utime >= to_utime) {
				from_utime = to_utime - 300;
				stm_set_time_t(from_utime);
			}
			_update_from_to_label(from_utime, to_utime);
			tml_set_section_time_t(tml, from_utime, to_utime); 
			tml_repaint(tml);
			_ready_thumbnail();
			_get_thumbnail();					
		}		
	}
	
	return FALSE;
}

static gboolean pre_cal_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case GDK_EXPOSE:
		{
			GdkGC *gc;
			GdkDrawable *drawable;
			gint pos_x, pos_y;

			drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
			gc = gdk_gc_new(drawable);

			nfui_nfobject_get_offset(obj, &pos_x, &pos_y);

			nfutil_draw_image(drawable, gc, IMG_CALENDAR_BG, pos_x, pos_y, -1, -1, NFALIGN_LEFT, 0);

	 		g_object_unref(gc);
			
			_update_calendar_time(from_utime);
			_update_calendar_text(from_utime);

			if (tml_is_set_section(tml)) {
				_update_from_to_label(from_utime, to_utime);
				tml_set_section_time_t(tml, from_utime, to_utime); 
			}
	 	}
		break;
		
		default :
		break;
	}

	return FALSE;
}

static gboolean post_allch_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	gint i;
	gboolean act = FALSE;

	if(event->type == NFEVENT_CHECKBUTTON_CHANGED) {
		act = nfui_check_button_get_active(NF_CHECKBUTTON(obj));

        for (i = 0; i < var_get_ch_count(); ++i)
        {
            if (nfui_nfobject_is_disabled(ch_sel[i])) continue;

			if(act) nfui_check_button_set_active(NF_CHECKBUTTON(ch_sel[i]), act);
			else 	nfui_check_button_set_active(NF_CHECKBUTTON(ch_sel[i]), act);
		}
	}

	return FALSE;
}

static gboolean post_ch_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if(event->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
        _set_all_chk_btn(ch_all, ch_sel, TRUE);
	}
	else if(event->type == NFEVENT_KEYPAD_PRESS || event->type == NFEVENT_REMOCON_PRESS) 
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)event;
		kpid = (KEYPAD_KID)kevt->keyval;
		
		if(obj == ch_sel[0]) 
		{
			if(kpid == KEYPAD_UP) 
			{
				_change_obj_focus(obj, ch_all);
				return TRUE;
			}
			else if(kpid == KEYPAD_LEFT) 
			{
				_change_obj_focus(obj, cld_next);
				return TRUE;
			}
		}
		else if(obj == ch_sel[GUI_CHANNEL_CNT - 1])
		{
			if(kpid == KEYPAD_DOWN) 
			{
				_change_obj_focus(obj, tml_ref);
				return TRUE;
			}
		}
	}

	return FALSE;
}


static gboolean vw_art1_cld_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
	}
	
	return FALSE;
}

static int _update_preview_text()
{
// commented out by SKSHIN
// because it is not accurate, if each channels ared recorded asychronously.
// mantis 10618
#if 0
	char buf[128];
	char time_buffer[128];

	dtf_get_local_datetime(from_utime, time_buffer);
	sprintf(buf, ": %s", time_buffer);
	nfui_nflabel_set_text((NFLABEL*)preview_time_obj, buf);
	nfui_signal_emit(preview_time_obj, GDK_EXPOSE, TRUE);
#endif
	return 0;
}

static gboolean vw_art1_cld_first_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	time_t first_t;
	gint year, mon, day;
	time_t gap = to_utime - from_utime;

	if(evt->type == GDK_BUTTON_RELEASE) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		scm_get_record_time(&first_t, 0);
		if (first_t) {

			_update_calendar_time(first_t);
			_update_calendar_text(first_t);

			from_utime = first_t;
			stm_set_time_t(from_utime);
			to_utime = from_utime + gap;
		
			tml_set_manual_paint_mode(tml, 1);
			tml_zoom_min(tml);
			tml_reset_cur_day(tml, from_utime);
			tml_set_playstick_time_t(tml, from_utime);
/*			if (tml_is_set_section(tml)) {
				_update_from_to_label(from_utime, to_utime);
				tml_set_section_time_t(tml, from_utime, to_utime); 
			}
			else _update_from_to_label(from_utime, 0);
*/			
			tml_refresh(tml);
			tml_set_manual_paint_mode(tml, 0);
			_update_preview_text();
			_ready_thumbnail();
			_get_thumbnail();
		}
	}
	
	return FALSE;
}

static gboolean vw_art1_cld_last_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	time_t last_t;
	gint year, mon, day;
	time_t gap = to_utime - from_utime;

	if (evt->type == GDK_BUTTON_RELEASE) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		scm_get_record_time(0, &last_t);
		if (last_t) {

			_update_calendar_time(last_t);
			_update_calendar_text(last_t);

			to_utime = last_t;
			from_utime = to_utime - gap;
			stm_set_time_t(from_utime);
		
			tml_set_manual_paint_mode(tml, 1);
			tml_zoom_min(tml);
			tml_reset_cur_day(tml, from_utime);
			tml_set_playstick_time_t(tml, to_utime);
			if (tml_is_set_section(tml)) {
				_update_from_to_label(from_utime, to_utime);
				tml_set_section_time_t(tml, from_utime, to_utime); 
			}
			else _update_from_to_label(from_utime, 0);
			
			tml_refresh(tml);
			tml_set_manual_paint_mode(tml, 0);
			_update_preview_text();
			_ready_thumbnail();
			_get_thumbnail();
		}
	}
	
	return FALSE;
}

static gboolean vw_art1_zoomout_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	
	if(evt->type == GDK_BUTTON_RELEASE) {
	  	if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
		if (tml_zoom_out(tml) == 0) {
		}
	}
	
	return FALSE;
}

static gboolean vw_art1_zoomin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) {
	  	if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
		if (tml_zoom_in(tml) == 0) {
		}
	}
	
	return FALSE;
}

static gboolean vw_art1_prev_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		tml_go_back(tml);
	}
	
	return FALSE;
}

static gboolean vw_art1_next_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		tml_go_ahead(tml);
	}
	
	return FALSE;
}

static gboolean post_refresh_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) {
	  	if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
		tml_refresh(tml);
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) {
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_UP) 
		{
			_change_obj_focus(obj, ch_sel[GUI_CHANNEL_CNT - 1]);
			return TRUE;
		}
	}

	return FALSE;
}

static gboolean vw_art1_htl_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{

	
	if(evt->type == GDK_BUTTON_RELEASE)
	{
	}

		
	return FALSE;
	
}


static gboolean post_erase_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	char msgbuf[256];
	char from[64] = {0, };
	char to[64] = {0, };
	
	guint64 vsize = 0;
	BITMASK ch_mask = 0;
	CMM_MESSAGE_T *pmsg;

	switch(evt->type) {
	case GDK_BUTTON_RELEASE:
	{
		gint i;
		GError *err = NULL;
		gchar *strTag = NULL;
		gchar *strInfo = NULL;
		mb_type ret;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {
			return FALSE;
		}

		nfui_nflabel_get_text_b(from_datetime_obj, from, 63);
		nfui_nflabel_get_text_b(to_datetime_obj, to, 63);

    	if (strlen(from) == 0 || strlen(to) == 0) {
			nftool_mbox(g_curwnd, "NOTICE","Please check the time of delete section (from, to)", 
								NFTOOL_MB_OK);

			return FALSE;
		}
        _wait_for_thumb_stop();        

		if (!VW_UserPwd_Open(g_curwnd, "PASSWORD CHECK")) return FALSE;

		ch_mask = _get_ch_selecting_mask();
		if (ch_mask == 0) {
			nftool_mbox(g_curwnd, "NOTICE", "There is no selected channel.", NFTOOL_MB_OK);
			return FALSE;
		}

		ret = nftool_mbox(g_curwnd, "CONFIRM", "when delete video, the video recording will be stopped.\nDo you want to continue?", NFTOOL_MB_OKCANCEL);

		if(ret == NFTOOL_MB_CANCEL) return FALSE;

		wait_pop = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "Please wait...", "");

		if (scm_erase_ch(ch_mask, from_utime, to_utime, IRET_SCM_ERASE_CH) == -1)
		{
			_remove_waitbox();
			nftool_mbox(g_curwnd, "ERROR", "It's unable to delete the data due to reserved one.", NFTOOL_MB_OK);
			return FALSE;
		}
	
		nfui_nfobject_disable(erasebtn_obj);
		nfui_signal_emit(erasebtn_obj, GDK_EXPOSE, TRUE);

		nfui_nfobject_disable(close_obj);
		nfui_signal_emit(close_obj, GDK_EXPOSE, TRUE);
	}
		break;

	case IRET_SCM_ERASE_CH:
		pmsg = (CMM_MESSAGE_T *)data;
		_remove_waitbox();

		if (pmsg->param == 0) {
		    _ready_thumbnail();
    		_get_thumbnail();
	    	nftool_mbox(g_curwnd, "NOTICE", "The data is erased successfully.", NFTOOL_MB_OK);
		}
		else {
			nftool_mbox(g_curwnd, "NOTICE", "Failed to erase the data.", NFTOOL_MB_OK);
		}
		nfui_nfobject_enable(erasebtn_obj);
		nfui_signal_emit(erasebtn_obj, GDK_EXPOSE, TRUE);

		nfui_nfobject_enable(close_obj);
		nfui_signal_emit(close_obj, GDK_EXPOSE, TRUE);
		break;

	case GDK_DELETE:
		uxm_unreg_imsg_event(obj, IRET_SCM_ERASE_CH);
		break;
		
	}

	
	return FALSE;
}


static time_t _get_new_calendar_time()
{
	int year, month, day, hour, min, sec;
	time_t tim;

	_get_calendar_day(&year, &month, &day);
	dtf_get_local_hourmin(from_utime, &hour, &min, &sec);
	tim = ifn_get_gmt_from_local(year, month, day, hour, min, sec);
	return tim;
}

static gboolean post_close_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		VW_Delete_Data_Close();
	}
	
	return FALSE;
}

static gboolean post_cld_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	char from_buffer[100];
	char to_buffer[100];
	time_t gap = to_utime - from_utime;
	
	switch(evt->type) {
	case GDK_BUTTON_PRESS :
		break;

	case NFEVENT_CALENDAR_CHANGED_RELEASE:
		from_utime = _get_new_calendar_time();
		to_utime = from_utime + gap;
		stm_set_time_t(from_utime);
		if (tml_is_set_section(tml)) _update_from_to_label(from_utime, to_utime);
		else _update_from_to_label(from_utime, 0);

		_update_calendar_text(from_utime);
		tml_set_manual_paint_mode(tml, 1);
		tml_zoom_min(tml);
		tml_reset_cur_day(tml, from_utime);
		tml_set_playstick_time_t(tml, from_utime);
		tml_refresh(tml);
		tml_set_manual_paint_mode(tml, 0);
		_update_preview_text();
		_ready_thumbnail();
		_get_thumbnail();
		break;
	
	case NFEVENT_KEYPAD_PRESS:
	case NFEVENT_REMOCON_PRESS:
		{
			GdkEventKey *kevt;
			KEYPAD_KID kpid;

			kevt = (GdkEventKey*)evt;
			kpid = (KEYPAD_KID)kevt->keyval;

			if(kpid == KEYPAD_UP) {
				if(!obj->grab_kfocus) {
					_change_obj_focus(obj, cld_prev);
					return TRUE;
				}
			}
		}
		break;
		
	default:
			break;
	}

	return FALSE;
}



static gboolean art1_post_pic_hour_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkGC *gc;
	GdkDrawable *drawable;
	static int cnt = 0;

	switch(evt->type)
	{
		case GDK_EXPOSE:
		{
	 	}
		break;

		case GDK_DELETE:
		{		
		}
		break;

		default :

		break;
	}
	
	
	return FALSE;
}

static int _erase_to_label()
{
	nfui_nflabel_set_text((NFLABEL*)to_datetime_obj, "");
	nfui_signal_emit((NFOBJECT*)to_datetime_obj, GDK_EXPOSE, TRUE);
}

static gboolean post_timeline_event_cb(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	CMM_MESSAGE_T *pmsg = (CMM_MESSAGE_T *)data;
	char label[32];
	int year, mon, day;
	time_t tmp;
	unsigned int sel_mask = 0;
	GTimeVal begTime;

	switch (event->type) {
	case GDK_BUTTON_RELEASE:
		break;

	case GDK_2BUTTON_PRESS:
		break;

	case NFEVENT_TML_RULER_DRAGUP:
		break;

	case NFEVENT_TML_RULER_DRAGDOWN:
		break;

	case GDK_DELETE:
		uxm_unreg_imsg_event(obj, INFY_TML_UNSET_SECTION);
		uxm_unreg_imsg_event(obj, INFY_TML_DATE_CHANGED);
		uxm_unreg_imsg_event(obj, INFY_TML_PLAY_CHANGED);
		uxm_unreg_imsg_event(obj, INFY_TML_START_CHANGED);
		uxm_unreg_imsg_event(obj, INFY_TML_END_CHANGED);
		uxm_unreg_imsg_event(obj, INFY_TML_SECTION_CHANGED);		
		uxm_unreg_imsg_event(obj, INFY_TML_SCROLL_UP);
		uxm_unreg_imsg_event(obj, INFY_TML_SCROLL_DOWN);
		break;

	case INFY_TML_UNSET_SECTION:
		pmsg = (CMM_MESSAGE_T *)data;
		if (obj != pmsg->data) return FALSE;
		if (!nfui_nfobject_is_shown(obj)) return FALSE;
		_erase_to_label();
//		to_utime = from_utime + 300;
//		_update_from_to_label(0, to_utime);
		break;

	case INFY_TML_DATE_CHANGED:
		pmsg = (CMM_MESSAGE_T *)data;
		if (obj != pmsg->data) return FALSE;
		if (!nfui_nfobject_is_shown(obj)) return FALSE;
		tmp = tml_get_cur_time_t(obj);
		_update_calendar_time(tmp);
		_update_calendar_text(tmp);
		break;

	case INFY_TML_PLAY_CHANGED:
		pmsg = (CMM_MESSAGE_T *)data;
		if (obj != pmsg->data) return FALSE;
		if (!nfui_nfobject_is_shown(obj)) return FALSE;
		from_utime = pmsg->param;
		
		//to_utime = from_utime + 300;
		if (from_utime >= to_utime) to_utime = from_utime + 300;
		stm_set_time_t(from_utime);
		_update_from_to_label(from_utime, 0);
		_update_preview_text();
		_ready_thumbnail();
		_get_thumbnail();
		break;

	case INFY_TML_START_CHANGED:
		pmsg = (CMM_MESSAGE_T *)data;
		if (obj != pmsg->data) return FALSE;
		if (!nfui_nfobject_is_shown(obj)) return FALSE;
		from_utime = pmsg->param;
		if (from_utime >= to_utime) to_utime = from_utime + 300;
		stm_set_time_t(from_utime);
		_update_from_to_label(from_utime, to_utime);
		_update_preview_text();	
		g_thumb_dir |= (1 << 0);		
//		_ready_thumbnail();
//		_get_thumbnail();
		break;

	case INFY_TML_END_CHANGED:
		pmsg = (CMM_MESSAGE_T *)data;
		if (obj != pmsg->data) return FALSE;
		if (!nfui_nfobject_is_shown(obj)) return FALSE;
		to_utime = pmsg->param;
		if (from_utime > to_utime) {
			from_utime = to_utime - 300;
			stm_set_time_t(from_utime);
		}
		_update_from_to_label(from_utime, to_utime);
		g_thumb_dir |= (1 << 1);
//		_ready_thumbnail();
//		_get_thumbnail();
		break;

	case INFY_TML_SECTION_CHANGED:
		_ready_thumbnail();
		_get_thumbnail();
		g_thumb_dir = 0;
		break;

	case INFY_TML_SCROLL_UP:
		pmsg = (CMM_MESSAGE_T *)data;
		if (obj != pmsg->data) return FALSE;
		tml_zoom_in(tml);
		break;

	case INFY_TML_SCROLL_DOWN:
		pmsg = (CMM_MESSAGE_T *)data;
		if (obj != pmsg->data) return FALSE;
		tml_zoom_out(tml);
		break;

	/*
	case INFY_TML_DOUBLE_CLICKED:
		pmsg = (CMM_MESSAGE_T *)data;
		if (obj != pmsg->data) return FALSE;
		if (!nfui_nfobject_is_shown(obj)) return FALSE;

		if(!ssm_check_access_auth(USR_AUTH_SEARCH))
			break;

		begTime.tv_sec = pmsg->param;

		//SKSHIN, ugie's module cannot support discontinuous channel playing.
		//sel_mask = _get_ch_selecting_mask();
		//
		sel_mask = var_get_ch_mask(); 
		//vw_archiving_start_playback(OPEN_BY_ARCH_DF);
		vsm_playback_start( sel_mask, 
							begTime, 
							PLAYBACK_NORMAL);


		break;
		*/

	case NFEVENT_KEYPAD_PRESS:
	case NFEVENT_REMOCON_PRESS:
#if 0
		nfui_set_key_focus(obj, FALSE);
		nfui_set_key_focus(cld, TRUE);
		nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
		nfui_signal_emit(cld, GDK_EXPOSE, TRUE);
		break;		
#endif		
		{
			GdkEventKey *kevt;
			KEYPAD_KID kpid;

			kevt = (GdkEventKey*)event;
			kpid = (KEYPAD_KID)kevt->keyval;
			
			if(kpid == KEYPAD_UP) 
			{
				_change_obj_focus(obj, close_obj);

				return TRUE;
			}
			else if (kpid == KEYPAD_DOWN) 
			{
				_change_obj_focus(obj, tml_ref);
				return TRUE;
			}		
			else if (kpid == KEYPAD_LEFT) 
			{
				_change_obj_focus(obj, last_btn);
				return TRUE;
			}		
			else if (kpid == KEYPAD_RIGHT) 
			{
				_change_obj_focus(obj, first_btn);
				return TRUE;
			}					
			else if (kpid == KEYPAD_EXIT) 
			{
				nfui_set_key_focus(obj, FALSE);
				nfui_set_key_focus(cld, TRUE);
				nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
				nfui_signal_emit(cld, GDK_EXPOSE, TRUE);
				return TRUE;
			}					
		}
		break;
	}

	return TRUE;
}

static gboolean vw_art1_cld_prevbtn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	time_t ret;

	if(evt->type == GDK_BUTTON_RELEASE) {
		ret = cw_cld_set_prev_month((CWCALENDAR*)cld);
		if (ret == 0) return FALSE;
		_update_calendar_text(ret);
	} 
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) {
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_DOWN) {
			_change_obj_focus(obj, cld);
			return TRUE;
		}
	}
	
	return FALSE;
}

static gboolean vw_art1_cld_nextbtn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	time_t ret;

	if(evt->type == GDK_BUTTON_RELEASE) {
		ret = cw_cld_set_next_month((CWCALENDAR*)cld);
		if (ret == 0) return FALSE;
		_update_calendar_text(ret);
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) {
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_DOWN) {
			_change_obj_focus(obj, cld);
			return TRUE;
		}
	}
	
	return FALSE;
}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
	}

	return FALSE;
}



///////////////////////////////////////////////////////////////////
//
//
//

void vw_init_del_tab1_page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *fixed3;
    NFOBJECT *main_page_fixed;
    NFOBJECT *page_fixed[PAGE_FIXED_CNT];
    NFOBJECT *page_ntb[PAGE_FIXED_CNT];

	NFOBJECT *cal_fixed;
	NFOBJECT *ht_fixed;

	NFOBJECT *ntb;	
	NFOBJECT *obj;
	NFOBJECT *obj2;
	int c_utime, c_year, c_month, c_day, c_hour, c_min, c_sec;
	char from_buffer[100];
	char to_buffer[100];

	gint page_num, col_num;
	GdkPixbuf *prev_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *next_img[NFOBJECT_STATE_COUNT];

	TML_FIGURE_T figure;

	GdkPixbuf *tl_btn_img[5][NFOBJECT_STATE_COUNT];
	GdkPixbuf *date_btn_img[2][NFOBJECT_STATE_COUNT];
	GdkPixbuf *arrow_img[2][NFOBJECT_STATE_COUNT];
	
	
	const gchar *strDay[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
	const gchar *strRec[] = { "PRE", "CONTINUOUS", "ALARM", "MOTION", "PANIC"};
	const gchar *calButton[] = {"FIRST", "LAST"};
	const gchar *strButton[] = {"EXPORT", "ERASE", "CLOSE"};

	int hei;

	gchar strBuf[16];

	guint size_w, size_h;	
	guint pos_x, pos_y;	
		
	guint ch, i, color_kind = 1;
	guint width[7];
	int zoom_btn_y;

	g_thumb_dir = 0;

	g_curwnd = nfui_nfobject_get_top(parent);

	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(content_fixed, MENU_H_INNER_W, MENU_H_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);
		
	left_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(left_fixed, SBT_FIXED1_W, SBT_FIXED1_H);
	nfui_nfobject_show(left_fixed);
	nfui_nfobject_modify_bg(left_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nffixed_put((NFFIXED*)content_fixed, left_fixed, SBT_FIXED1_X, SBT_FIXED1_Y);
	
	fixed2 = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed2, SBT_FIXED2_W + 1, SBT_FIXED2_H);
	nfui_nfobject_show(fixed2);
	nfui_nfobject_modify_bg(fixed2, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nffixed_put((NFFIXED*)content_fixed, fixed2, SBT_FIXED2_X, SBT_FIXED2_Y);

	fixed3 = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed3, SBT_FIXED3_W + 1, SBT_FIXED3_H);
	nfui_nfobject_show(fixed3);
	nfui_nfobject_modify_bg(fixed3, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nffixed_put((NFFIXED*)content_fixed, fixed3, SBT_FIXED3_X, SBT_FIXED3_Y);
	preview_fixed = fixed3;

	cal_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(cal_fixed, SBT_CAL_FIXED_W, SBT_CAL_FIXED_H);
	nfui_nfobject_show(cal_fixed);
	nfui_nfobject_modify_bg(cal_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nffixed_put((NFFIXED*)left_fixed, cal_fixed, SBT_CAL_FIXED_X, SBT_CAL_FIXED_Y);
	

// CALENDAR LABEL
	gint cal_lbl_pos_x[7] = {47, 93, 137, 182, 226, 269, 315};
	for(i = 0 ; i < 7 ; i++)
	{
		if (i == 0){
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strDay[i],
									nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(609));
		}else if (i == 6){
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strDay[i],
									nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(611));
		}else{
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strDay[i],
									nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(610));
		}
		
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_set_size(obj, 46, 40);
		nfui_nfobject_show(obj);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(605));
		nfui_nffixed_put((NFFIXED*)cal_fixed, obj, cal_lbl_pos_x[i], 43);
		
	}
			
// YEAR / MONTH LABEL
	cld_text = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(607));
	nfui_nfobject_set_size(cld_text, (guint)(282) , (guint)(34));
	nfui_nflabel_set_align((NFLABEL*)cld_text, NFALIGN_CENTER,0);
	nfui_nfobject_modify_bg(cld_text, NFOBJECT_STATE_NORMAL, COLOR_IDX(606));
	nfui_nfobject_use_focus(cld_text, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(cld_text);
	nfui_nffixed_put((NFFIXED*)cal_fixed, cld_text, (guint)(63), (guint)(4));

// CALENDAR
	c_utime 	= time(0);
	dtf_get_local_day(c_utime, &c_year, &c_month, &c_day);
	dtf_get_local_hourmin(c_utime, &c_hour, &c_min, 0);

	cld = cw_cld_new(c_utime, 44, 40);
	nfui_nfobject_show(cld);
	nfui_nffixed_put((NFFIXED*)cal_fixed, cld, 50, 85);
	nfui_regi_post_event_callback(cld, post_cld_event_cb);

// Calendar PREV/NEXT BUTTON
	arrow_img[0][0] = nfui_get_image_from_file((IMG_CALENDAR_PRE_N_BUTTON), NULL);
	arrow_img[0][1] = nfui_get_image_from_file((IMG_CALENDAR_PRE_O_BUTTON), NULL);
	arrow_img[0][2] = nfui_get_image_from_file((IMG_CALENDAR_PRE_P_BUTTON), NULL);
	arrow_img[0][3] = nfui_get_image_from_file((IMG_CALENDAR_PRE_D_BUTTON), NULL);

	arrow_img[1][0] = nfui_get_image_from_file((IMG_CALENDAR_NEXT_N_BUTTON), NULL);
	arrow_img[1][1] = nfui_get_image_from_file((IMG_CALENDAR_NEXT_O_BUTTON), NULL);
	arrow_img[1][2] = nfui_get_image_from_file((IMG_CALENDAR_NEXT_P_BUTTON), NULL);
	arrow_img[1][3] = nfui_get_image_from_file((IMG_CALENDAR_NEXT_D_BUTTON), NULL);

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), arrow_img[0]);
	nfui_nfobject_set_size(obj, 27, 27);
	nfui_nfobject_show(obj);
	nfui_nffixed_put ((NFFIXED*)cal_fixed, obj, 34, 7);
	nfui_regi_post_event_callback(obj, vw_art1_cld_prevbtn_event_handler);
	cld_prev = obj;

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), arrow_img[1]);
	nfui_nfobject_set_size(obj, 27, 27);
	nfui_nfobject_show(obj);
	nfui_nffixed_put ((NFFIXED*)cal_fixed, obj, 349, 7);
	nfui_regi_post_event_callback(obj, vw_art1_cld_nextbtn_event_handler);
	cld_next = obj;

// Calendar FIRST/LAST BUTTON
	obj = nftool_normal_button_create_type3(calButton[0], SBT_CAL_BUTTON_SIZE_W);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put ((NFFIXED*)cal_fixed, obj, SBT_CAL_FIRST_BUTTON_X, SBT_CAL_FIRST_BUTTON_Y);
	nfui_regi_post_event_callback(obj, vw_art1_cld_first_event_handler);
	first_btn = obj;

	obj = nftool_normal_button_create_type3(calButton[1], SBT_CAL_BUTTON_SIZE_W);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put ((NFFIXED*)cal_fixed, obj, SBT_CAL_LAST_BUTTON_X, SBT_CAL_LAST_BUTTON_Y);
	nfui_regi_post_event_callback(obj, vw_art1_cld_last_event_handler);
	last_btn = obj;

	pos_x = SBT_CAL_FIXED_X;
	pos_y = SBT_CAL_FIXED_Y + SBT_CAL_FIXED_H + 8;

// FROM
	pos_y += (SBT_LABEL_HEIGHT1 + 10);

	stm_get_time_range_t(&from_utime, &to_utime);
	dtf_get_local_datetime(from_utime, from_buffer);
	dtf_get_local_datetime(to_utime, to_buffer);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("FROM", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, 80, SBT_LABEL_HEIGHT1);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)left_fixed, obj, pos_x, pos_y);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box(from_buffer, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, 286, SBT_LABEL_HEIGHT1);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)left_fixed, obj, pos_x+80, pos_y);
	from_datetime_obj = obj;
	
	date_btn_img[0][0] = nfui_get_image_from_file((IMG_BT_DATETIME_N), NULL);
	date_btn_img[0][1] = nfui_get_image_from_file((IMG_BT_DATETIME_O), NULL);	
	date_btn_img[0][2] = nfui_get_image_from_file((IMG_BT_DATETIME_P), NULL);	
	date_btn_img[0][3] = nfui_get_image_from_file((IMG_BT_DATETIME_D), NULL);	
	nfui_get_pixbuf_size(date_btn_img[0][0], &size_w, &size_h);
	
	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), date_btn_img[0]);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)left_fixed, obj, pos_x+100+266+2, pos_y);
	nfui_regi_post_event_callback(obj, post_from_event_handler);
	
// TO
	pos_y += (SBT_LABEL_HEIGHT1 + 2);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("TO", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, 80, SBT_LABEL_HEIGHT1);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)left_fixed, obj, pos_x, pos_y);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box(to_buffer, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, 286, SBT_LABEL_HEIGHT1);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)left_fixed, obj, pos_x+80, pos_y);
	to_datetime_obj = obj;
	
	date_btn_img[1][0] = nfui_get_image_from_file((IMG_BT_DATETIME_N), NULL);
	date_btn_img[1][1] = nfui_get_image_from_file((IMG_BT_DATETIME_O), NULL);	
	date_btn_img[1][2] = nfui_get_image_from_file((IMG_BT_DATETIME_P), NULL);	
	date_btn_img[1][3] = nfui_get_image_from_file((IMG_BT_DATETIME_D), NULL);	
	
	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), date_btn_img[1]);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)left_fixed, obj, pos_x+100+266+2, pos_y);
	nfui_regi_post_event_callback(obj, post_to_event_handler);


// RECORD REASON
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strRec[0], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(638));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, SBT_TL_REC_TEXT_PRE_W, SBT_TL_REC_BOX_H + 4);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, SBT_TL_REC_TEXT_PRE_X, SBT_TL_REC_BOX_PRE_Y);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strRec[1], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(638));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, SBT_TL_REC_TEXT_TIME_W, SBT_TL_REC_BOX_H + 4);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, SBT_TL_REC_TEXT_TIME_X, SBT_TL_REC_BOX_TIME_Y);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strRec[2], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(638));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, SBT_TL_REC_TEXT_ALARM_W, SBT_TL_REC_BOX_H + 4);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, SBT_TL_REC_TEXT_ALARM_X, SBT_TL_REC_BOX_ALARM_Y);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strRec[3], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(638));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, SBT_TL_REC_TEXT_MOT_W, SBT_TL_REC_BOX_H + 4);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, SBT_TL_REC_TEXT_MOT_X, SBT_TL_REC_BOX_MOT_Y);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strRec[4], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(638));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, SBT_TL_REC_TEXT_PANIC_W, SBT_TL_REC_BOX_H + 4);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, SBT_TL_REC_TEXT_PANIC_X, SBT_TL_REC_BOX_PANIC_Y);


	memset(&figure, 0x00, sizeof(TML_FIGURE_T));
	figure.len = LEN_1350;
	figure.type = TML_HORIZONTAL;
	figure.cr_bg = UX_COLOR(631);
	figure.cr_rbar[0] = UX_COLOR(630);
	figure.cr_rbar[1] = UX_COLOR(639);
	figure.cr_rbar[2] = UX_COLOR(641);
	figure.cr_rbar[3] = UX_COLOR(643);
	figure.cr_rbar[4].red = 0x0000;
	figure.cr_rbar[4].green = 0x0000;
	figure.cr_rbar[4].blue = 0xff00;
	figure.cr_rbar[5] = UX_COLOR(645);
	figure.cr_rbar[6] = UX_COLOR(647);
	figure.cr_rbar[7] = UX_COLOR(630);
	figure.cr_rbar[8] = UX_COLOR(640);
	figure.cr_rbar[9] = UX_COLOR(642);
	figure.cr_rbar[10] = UX_COLOR(644);
	figure.cr_rbar[11].red = 0x0000;
	figure.cr_rbar[11].green = 0x0000;
	figure.cr_rbar[11].blue = 0xff00;
	figure.cr_rbar[12] = UX_COLOR(646);
	figure.cr_rbar[13] = UX_COLOR(648);
	figure.cr_ruler_bg = UX_COLOR(630);
	figure.cr_ruler_text = UX_COLOR(347);
	figure.cr_playbar = UX_COLOR(358);
	figure.cr_cti = UX_COLOR(360);
	
	figure.ruler_brd = HEIGHT_RULER;
	figure.ch_brd = HEIGHT_CH;
	figure.pb_curtain = nfui_get_image_from_file(HRZ_CURTAIN2, NULL);

	tml = tml_new(parent);
	hei = var_get_ch_count() * HEIGHT_CH + HEIGHT_RULER;
	nfui_nfobject_set_size(tml, 1350, hei);
	nfui_nffixed_put((NFFIXED*)fixed2, tml, TML_X, TML_Y);
	tml_init(tml, &figure, TML_X, TML_Y, 1350, hei, var_get_ch_count(), -1);
	tml_set_cti_position(tml, 0);
	tml_set_style(tml, TML_GHOSTCTI);
	nfui_nfobject_show(tml);
	nfui_regi_post_event_callback(tml, post_timeline_event_cb);

	ch_all = (NFOBJECT*)nfui_checkbutton_new(TRUE);
	nfui_check_button_set_skin_type(NF_CHECKBUTTON(ch_all), NFCHECK_TYPE_SMALL);
	nfui_check_get_size(ch_all, &size_w, &size_h);
	nfui_nfobject_show(ch_all);
	nfui_nffixed_put((NFFIXED*)fixed2, ch_all, SBT_TML_CH_CHECK_X, SBT_TML_CH_CHECK_Y);
	nfui_regi_post_event_callback(ch_all, post_allch_event_handler);

	for (i = 0; i < var_get_ch_count(); ++i)
	{
		ch_sel[i] = (NFOBJECT*)nfui_checkbutton_new(TRUE);
		nfui_check_button_set_skin_type(NF_CHECKBUTTON(ch_sel[i]), NFCHECK_TYPE_SMALL);
		nfui_check_get_size(ch_sel[i], &size_w, &size_h);
		nfui_nfobject_show(ch_sel[i]);
		nfui_nffixed_put((NFFIXED*)fixed2, ch_sel[i], SBT_TML_CH_CHECK_X, SBT_TML_CH_CHECK_Y + HEIGHT_RULER + size_h*i);
		nfui_regi_post_event_callback(ch_sel[i], post_ch_event_handler);
	}

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ALL", nffont_get_pango_font(NFFONT_MINI_SEMI), COLOR_IDX(637));
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(635));
	nfui_nfobject_set_size(obj, SBT_TML_CH_LABEL_W, SBT_TML_CH_LABEL_H);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, SBT_TML_CH_LABEL_X, SBT_TML_CH_LABEL_Y);

	for (i = 0; i < var_get_ch_count(); ++i)
	{ 
		sprintf(strBuf, "%d", i+1);
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MINI_SEMI), COLOR_IDX(637));
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(635));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_set_size(obj, SBT_TML_CH_LABEL_W, SBT_TML_CH_LABEL_H);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed2, obj, SBT_TML_CH_LABEL_X, SBT_TML_CH_LABEL_Y + HEIGHT_RULER + SBT_TML_CH_LABEL_H*i);
	}

	tl_btn_img[0][0] = nfui_get_image_from_file((IMG_SUBMENU_TL_ZOOMOUT_N_BTN), NULL);
	tl_btn_img[0][1] = nfui_get_image_from_file((IMG_SUBMENU_TL_ZOOMOUT_O_BTN), NULL);	
	tl_btn_img[0][2] = nfui_get_image_from_file((IMG_SUBMENU_TL_ZOOMOUT_P_BTN), NULL);	
	tl_btn_img[0][3] = nfui_get_image_from_file((IMG_SUBMENU_TL_ZOOMOUT_D_BTN), NULL);	

	tl_btn_img[1][0] = nfui_get_image_from_file((IMG_SUBMENU_TL_ZOOMIN_N_BTN), NULL);
	tl_btn_img[1][1] = nfui_get_image_from_file((IMG_SUBMENU_TL_ZOOMIN_O_BTN), NULL);	
	tl_btn_img[1][2] = nfui_get_image_from_file((IMG_SUBMENU_TL_ZOOMIN_P_BTN), NULL);	
	tl_btn_img[1][3] = nfui_get_image_from_file((IMG_SUBMENU_TL_ZOOMIN_D_BTN), NULL);	

	tl_btn_img[2][0] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_N_BTN), NULL);
	tl_btn_img[2][1] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_O_BTN), NULL);
	tl_btn_img[2][2] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_P_BTN), NULL);
	tl_btn_img[2][3] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_D_BTN), NULL);

	tl_btn_img[3][0] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_N_BTN), NULL);
	tl_btn_img[3][1] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_O_BTN), NULL);	
	tl_btn_img[3][2] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_P_BTN), NULL);	
	tl_btn_img[3][3] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_D_BTN), NULL);	

	tl_btn_img[4][0] = nfui_get_image_from_file((IMG_N_SYNC_TIME), NULL);
	tl_btn_img[4][1] = nfui_get_image_from_file((IMG_O_SYNC_TIME), NULL);	
	tl_btn_img[4][2] = nfui_get_image_from_file((IMG_P_SYNC_TIME), NULL);	
	tl_btn_img[4][3] = nfui_get_image_from_file((IMG_D_SYNC_TIME), NULL);	

	ch = var_get_ch_count();

	nfui_get_pixbuf_size(tl_btn_img[4][0], &size_w, &size_h);
	pos_x = SBT_TL_ZOOMOUT_BTN_X - (size_w + 10);
	zoom_btn_y = TML_Y + HEIGHT_RULER + (ch * HEIGHT_CH) + 6;

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), tl_btn_img[4]);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, pos_x, zoom_btn_y);
	nfui_regi_post_event_callback(obj, post_refresh_button_event_cb);
	tml_ref = obj;

	pos_x = SBT_TL_ZOOMOUT_BTN_X;

	nfui_get_pixbuf_size(tl_btn_img[0][0], &size_w, &size_h);
	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), tl_btn_img[0]);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, pos_x, zoom_btn_y);
	nfui_regi_post_event_callback(obj, vw_art1_zoomout_event_handler);

	pos_x += size_w + SBT_TL_ZOOM_BTN_GAP;

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), tl_btn_img[1]);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, pos_x, zoom_btn_y);
	nfui_regi_post_event_callback(obj, vw_art1_zoomin_event_handler);
	
	pos_x += size_w + SBT_TL_BTN_GAP;
	nfui_get_pixbuf_size(tl_btn_img[2][0], &size_w, &size_h);

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), tl_btn_img[2]);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, pos_x, zoom_btn_y);
	nfui_regi_post_event_callback(obj, vw_art1_prev_event_handler);
	
	pos_x += size_w + SBT_TL_DIR_BTN_GAP;

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), tl_btn_img[3]);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, pos_x, zoom_btn_y);
	nfui_regi_post_event_callback(obj, vw_art1_next_event_handler);

// PREVIEW TIME LABEL
	size_w = nfutil_string_width(1, NULL, nffont_get_pango_font(NFFONT_SMALL_SEMI), "PREVIEW", NORMAL_SPACING);
	size_w += 10;	// just margin
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PREVIEW", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(660));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, size_w, 22);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed3, obj, 15, 2);

	gchar buf[150];
	memset(buf, '\0', sizeof(buf));
	sprintf(buf, ": %s", from_buffer);

// PREVIEW TIME	
	preview_time_obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", 
									nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(660));
	nfui_nflabel_set_align((NFLABEL*)preview_time_obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(preview_time_obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(preview_time_obj, 300, 42);
	nfui_nfobject_use_focus(preview_time_obj, NFOBJECT_FOCUS_OFF);
	nfui_nffixed_put((NFFIXED*)fixed3, preview_time_obj, size_w + 10, 99);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_SMALL_THIN), COLOR_IDX(661));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, 100, 22);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed3, obj, SBT_FIXED3_W-27-100, 2);
	nfui_nfobject_set_data(fixed3, "CMPL RATE", (gpointer)obj);

	prev_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_N_BTN), NULL);
	prev_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_O_BTN), NULL);
	prev_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_P_BTN), NULL);
	prev_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_D_BTN), NULL);

	next_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_N_BTN), NULL);
	next_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_O_BTN), NULL);
	next_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_P_BTN), NULL);
	next_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_D_BTN), NULL);


    size_w = fixed3->width - 30;

    main_page_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(main_page_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(main_page_fixed, size_w, (THUMB_CELL_H) + 60);
    nfui_nfobject_show(main_page_fixed);
    nfui_nffixed_put((NFFIXED*)fixed3, main_page_fixed, 15, 28);

    for (i = 0; i < PAGE_FIXED_CNT; i++)
    {
        g_page_fixed[i] = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_modify_bg(g_page_fixed[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
        nfui_nfobject_set_size(g_page_fixed[i], size_w, THUMB_CELL_H);
        nfui_nffixed_put((NFFIXED*)main_page_fixed, g_page_fixed[i], 0, 0);
    }
    nfui_nfobject_show(g_page_fixed[0]);

	nfui_get_pixbuf_size(prev_img[0], &size_w, &size_h);
	
	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), prev_img);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_page_fixed, obj, (main_page_fixed->width / 2) - (size_w + 60), main_page_fixed->height - size_h);
	nfui_regi_post_event_callback(obj, post_prev_event_handler);

	memset(strBuf, 0x00, sizeof(strBuf));
	g_sprintf(strBuf, "1 / %d", PAGE_FIXED_CNT);
	
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 100, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_page_fixed, obj, (main_page_fixed->width - 100) / 2, main_page_fixed->height - size_h);
    g_lb_page_num = obj;

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), next_img);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_page_fixed, obj, (main_page_fixed->width / 2) + 60, main_page_fixed->height - size_h);
	nfui_regi_post_event_callback(obj, post_next_event_handler);

// THUMBNAL
    pos_x = 0;
    pos_y = 0;

    page_num = col_num = 0;

	for( i = 0 ; i < GUI_CHANNEL_CNT ; i++ )
	{
		sprintf(strBuf, "CH %d", i+1);	

		thumb_obj[i] = (NFOBJECT*)nfui_nfthumbnail_new(strBuf, nffont_get_pango_font(NFFONT_MINI_SEMI_3));
        nfui_nfthumbnail_subject_label_height((NFTHUMBNAIL*)thumb_obj[i], 17);
        nfui_nfthumbnail_use_focus_draw((NFTHUMBNAIL*)thumb_obj[i], FALSE);
        nfui_nfthumbnail_set_image_buf((NFTHUMBNAIL*)thumb_obj[i], ch_image[i]);
        nfui_nfthumbnail_set_channel((NFTHUMBNAIL*)thumb_obj[i], i);
        nfui_nfthumbnail_set_period((NFTHUMBNAIL*)thumb_obj[i], from_utime, to_utime);
        nfui_nfthumbnail_set_image_size((NFTHUMBNAIL*)thumb_obj[i], (THUMB_CELL_W-2), (THUMB_CELL_H-17-1));
		nfui_nfobject_use_focus(thumb_obj[i], FALSE);
		nfui_nfobject_set_size(thumb_obj[i], THUMB_CELL_W, THUMB_CELL_H);
		nfui_nfobject_show(thumb_obj[i]);
    	nfui_nffixed_put((NFFIXED*)g_page_fixed[page_num], thumb_obj[i], pos_x + ((THUMB_CELL_W + THUMB_CELL_COL_GAP) * col_num), pos_y);
		nfui_regi_post_event_callback(thumb_obj[i], art1_post_pic_hour_event_handler);
		
/*
        if ((i+1) % THUMB_COL_CNT == 0)
		{
            pos_x = 0;
			pos_y += THUMB_CELL_H + 6;
		}
		else
		{
			pos_x += THUMB_CELL_W+THUMB_CELL_COL_GAP;
		}
*/
        col_num++;

        if (col_num == THUMB_COL_CNT) {
            col_num = 0;
            page_num++;
        }
	}

// below button

	obj = nftool_normal_button_create_type1(strButton[1], MENU_BTN_WIDTH);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_H_BTN_R2_X, MENU_H_BTN_Y);
	nfui_regi_post_event_callback(obj, post_erase_event_handler);
	erasebtn_obj = obj;
	

	obj = nftool_normal_button_create_type2(strButton[2], MENU_BTN_WIDTH);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_H_BTN_R1_X, MENU_H_BTN_Y);
	nfui_regi_post_event_callback(obj, post_close_event_handler);
	close_obj = obj;


	nfui_regi_pre_event_callback(fixed2, pre_fixed2_event_handler);
	nfui_regi_pre_event_callback(cal_fixed, pre_cal_fixed_event_handler);
	nfui_regi_pre_event_callback(left_fixed, pre_left_fixed_event_handler);
	nfui_regi_pre_event_callback(fixed3, pre_fixed3_event_handler);
	nfui_regi_post_event_callback(fixed2, post_fixed2_event_handler);	

	nfui_regi_post_event_callback(parent, post_page_event_handler);

	uxm_reg_imsg_event(tml, INFY_TML_UNSET_SECTION);
	uxm_reg_imsg_event(tml, INFY_TML_DATE_CHANGED);
	uxm_reg_imsg_event(tml, INFY_TML_PLAY_CHANGED);
	uxm_reg_imsg_event(tml, INFY_TML_START_CHANGED);
	uxm_reg_imsg_event(tml, INFY_TML_END_CHANGED);
	uxm_reg_imsg_event(tml, INFY_TML_SECTION_CHANGED);	
	uxm_reg_imsg_event(tml, INFY_TML_SCROLL_UP);
	uxm_reg_imsg_event(tml, INFY_TML_SCROLL_DOWN);
	uxm_reg_imsg_event(fixed3, INFY_THUMBNAIL_CMPL_OBJ);
	uxm_reg_imsg_event(erasebtn_obj, IRET_SCM_ERASE_CH);

	if (!g_thumb_tmr) g_thumb_tmr = g_timeout_add(300, _show_thumbnail, 0);

	tml_reset_cur_day(tml, from_utime);
	tml_set_section_time_t(tml, from_utime, to_utime); 
	tml_repaint(tml);

	g_noupdate_tml = FALSE;
}

gboolean vw_del_tab1_out_handler()
{

	return FALSE;
}

gboolean vw_del_tab1_in_handler()
{
	time_t end;

	from_utime = stm_get_time_t();
	to_utime = from_utime + 300;
	
	stm_get_time_range_t(&from_utime, &to_utime);
	//if (end) to_utime = end;
	 
	g_init_thumb = TRUE;
	g_noupdate_tml = TRUE;

	return FALSE;
}

gboolean vw_del_tab1_show()
{
	from_utime = stm_get_time_t();
	tml_zoom_min(tml);
	tml_reset_cur_day(tml, from_utime);

	vw_del_tab1_in_handler();
	return FALSE;
}
