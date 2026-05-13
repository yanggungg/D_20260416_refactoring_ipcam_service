#include "nf_afx.h"

#include "support/nf_ui_color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "support/color.h"
#include "support/nf_ui_page_manager.h"

#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nflabel.h"
#include "objects/nfbutton.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfspinbutton.h"

#include "tools/nf_ui_tool.h"

#include "vw_sys_disp_sequence.h"
#include "vw_sys_disp_sequence_setup.h"

#include "support/nf_ui_image.h"

#define PP_SIZE_WID				(578)
#define PP_SIZE_HEI				(575)

#define PP_POS_X				(guint)((DISPLAY_ACTIVE_WIDTH - PP_SIZE_WID)/2)
#define PP_POS_Y				(guint)((DISPLAY_ACTIVE_HEIGHT - PP_SIZE_HEI)/2)

#define	CHANNEL_VIEW_WIDTH		(273)
#define	CHANNEL_VIEW_HEIGHT		(273)
#define	CHANNEL_COMBO_WIDTH		((CHANNEL_VIEW_WIDTH-10)/6-6)

#if defined(GUI_4CH_SUPPORT)
#define SCR_DIV_DEFAULT_TYPE    (SCR_DIV_TYPE4)
#define DISP_TYPE8_IMG_IDX      (4)
#define DISP_TYPE6_IMG_IDX      (5)
#define SCR_DIV_TYPE9_CH_CNT    (9)
#elif defined(GUI_8CH_SUPPORT)
#define SCR_DIV_DEFAULT_TYPE    (SCR_DIV_TYPE9)
#define DISP_TYPE8_IMG_IDX      (3)
#define DISP_TYPE6_IMG_IDX      (4)
#define SCR_DIV_TYPE9_CH_CNT    (8)
#elif defined(GUI_16CH_SUPPORT)
#define SCR_DIV_DEFAULT_TYPE    (SCR_DIV_TYPE16)
#define DISP_TYPE8_IMG_IDX      (4)
#define DISP_TYPE6_IMG_IDX      (5)
#define SCR_DIV_TYPE9_CH_CNT    (9)
#elif defined(GUI_32CH_SUPPORT)
#define SCR_DIV_DEFAULT_TYPE    (SCR_DIV_TYPE36)
#define DISP_TYPE8_IMG_IDX      (5)
#define DISP_TYPE6_IMG_IDX      (6)
#define SCR_DIV_TYPE9_CH_CNT    (9)
#endif

enum {
	SSB_OK = 0,
	SSB_CANCEL,
	SSB_BUTTONS
};

static GSList *radioList = NULL;

static gint g_curType = -1;
#ifdef GUI_8CH_SUPPORT
static nfrect g_rtRect[GUI_CHANNEL_CNT+1];
#elif GUI_32CH_SUPPORT
static nfrect g_rtRect[GUI_CHANNEL_CNT+4];
#else
static nfrect g_rtRect[GUI_CHANNEL_CNT];
#endif


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *ch_area;

static NFOBJECT *div_btn[NUM_SCR_DIV_TYPES];
static NFOBJECT *sel_ch[GUI_CHANNEL_CNT];
static NFOBJECT *ss_btns[SSB_BUTTONS];

static SeqElementData *g_elem_data;
static guint ret=0;

static void prvCalcRect(nfrect *arRect, guint div_type)
{
	guint x, y;
	guint unit_w, unit_h;
	guint show_num;
	guint i;

	switch(div_type)
	{
		case SCR_DIV_TYPE1:
			nfutil_nfrect_set(&arRect[0], 0, 0, CHANNEL_VIEW_WIDTH, CHANNEL_VIEW_HEIGHT);
			show_num = 1;
			break;

		case SCR_DIV_TYPE4:
			unit_w = (CHANNEL_VIEW_WIDTH-2)/2;
			unit_h = (CHANNEL_VIEW_HEIGHT-2)/2;

			for(i=0; i<4; i++)
			{
				x = (unit_w+2)*(i%2);
				y = (unit_h+2)*(i/2);
				nfutil_nfrect_set(&arRect[i], x, y, unit_w, unit_h); 
			}
			show_num = 4;

			break;
#ifndef GUI_4CH_SUPPORT
		case SCR_DIV_TYPE9:
			unit_w = (CHANNEL_VIEW_WIDTH-4)/3;
			unit_h = (CHANNEL_VIEW_HEIGHT-4)/3;

			for(i=0; i<9; i++)
			{
				x = (unit_w+2)*(i%3);
				y = (unit_h+2)*(i/3);
				nfutil_nfrect_set(&arRect[i], x, y, unit_w, unit_h); 
			}
			show_num = 9;
			break;
#ifndef GUI_8CH_SUPPORT
		case SCR_DIV_TYPE16:
			unit_w = (CHANNEL_VIEW_WIDTH-6)/4;
			unit_h = (CHANNEL_VIEW_HEIGHT-6)/4;

			for(i=0; i<16; i++)
			{
				x = (unit_w+2)*(i%4);
				y = (unit_h+2)*(i/4);
				nfutil_nfrect_set(&arRect[i], x, y, unit_w, unit_h); 
			}
			
			show_num = 16;
			break;
#ifndef GUI_16CH_SUPPORT
		case SCR_DIV_TYPE36:
			unit_w = (CHANNEL_VIEW_WIDTH-10)/6;
			unit_h = (CHANNEL_VIEW_HEIGHT-10)/6;

			for(i=0; i<36; i++)
			{
				x = (unit_w+2)*(i%6) + 1;
				y = (unit_h+2)*(i/6) + 1;
				nfutil_nfrect_set(&arRect[i], x, y, unit_w, unit_h); 
			}
			
			show_num = 32;
			break;
#endif
		case SCR_DIV_TYPE8:
		{
			guint j;

			unit_w = (CHANNEL_VIEW_WIDTH-6)/4;
			unit_h = (CHANNEL_VIEW_HEIGHT-6)/4;

			j = 0;
			for(i=0; i<16; i++)
			{
				if((i%4<3) && (i/4<3) && (i!=0))	continue;

				x = (unit_w+2)*(i%4);
				y = (unit_h+2)*(i/4);
				
				if(i==0)	nfutil_nfrect_set(&arRect[j++], x, y, (unit_w+2)*3-2, (unit_h+2)*3-2); 
				else		nfutil_nfrect_set(&arRect[j++], x, y, unit_w, unit_h); 

			}
			show_num = 8;
		}
		break;

		case SCR_DIV_TYPE6:
		{
			guint j;

			unit_w = (CHANNEL_VIEW_WIDTH-4)/3;
			unit_h = (CHANNEL_VIEW_HEIGHT-4)/3;

			j = 0;
			for(i=0; i<9; i++)
			{
				if((i%3<2) && (i/3<2) && (i!=0))	continue;

				x = (unit_w+2)*(i%3);
				y = (unit_h+2)*(i/3);
				
				if(i==0)	nfutil_nfrect_set(&arRect[j++], x, y, unit_w*2+2, unit_h*2+2); 
				else		nfutil_nfrect_set(&arRect[j++], x, y, unit_w, unit_h); 

			}
			show_num = 6;
		}
		break;
#endif		
	}

	for(i=0; i<GUI_CHANNEL_CNT; i++)
	{
		if(i<show_num)	nfui_nfobject_show(sel_ch[i]);
		else			nfui_nfobject_hide(sel_ch[i]);

		x = 19 + g_rtRect[i].x + (g_rtRect[i].width - CHANNEL_COMBO_WIDTH)/2;
		y = 59 + g_rtRect[i].y + g_rtRect[i].height/2 - 20;

		nfui_nfobject_move(sel_ch[i], x, y);
	}
}

static gboolean 
prvKPadEvtProcess(NFOBJECT *obj, GdkEvent *evt)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid;
	NFOBJECT* new_focus = NULL;

	kevt = (GdkEventKey*)evt;
	kpid = (KEYPAD_KID)kevt->keyval;

	switch(kpid)
	{
		case KEYPAD_LEFT:	new_focus = nfui_nfobject_get_data(obj, "left");	break;
		case KEYPAD_RIGHT:	new_focus = nfui_nfobject_get_data(obj, "right");	break;
		case KEYPAD_UP:		new_focus = nfui_nfobject_get_data(obj, "up");		break;
		case KEYPAD_DOWN:	new_focus = nfui_nfobject_get_data(obj, "down");	break;
		default:	return FALSE;
	}

	if(new_focus)
	{
		nfui_set_key_focus(obj, FALSE);
		nfui_set_key_focus(new_focus, TRUE);

		nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
		nfui_signal_emit(new_focus, GDK_EXPOSE, TRUE);
	}

	return TRUE;
}

static void prvMakeChNavTable()
{
	gint i;
	gint temp;


	switch(g_curType)
	{
		case SCR_DIV_TYPE1:
			nfui_nfobject_set_data(sel_ch[0], "left", div_btn[1]);
			nfui_nfobject_set_data(sel_ch[0], "right", NULL);
			nfui_nfobject_set_data(sel_ch[0], "up", NULL);
			nfui_nfobject_set_data(sel_ch[0], "down", ss_btns[0]);
			break;

		case SCR_DIV_TYPE4:
#ifndef GUI_4CH_SUPPORT					
		case SCR_DIV_TYPE9:
#ifndef GUI_8CH_SUPPORT		
		case SCR_DIV_TYPE16:
		#ifndef GUI_16CH_SUPPORT
    		case SCR_DIV_TYPE36:
		#endif
#endif
#endif
		{
			gint max;
			gint div;
			NFOBJECT *obj1 = NULL;
			NFOBJECT *obj2 = NULL;

			if(g_curType == SCR_DIV_TYPE4)			{	max=4;	div=2;	}
#ifndef GUI_4CH_SUPPORT
			else if(g_curType == SCR_DIV_TYPE9)		{	max=SCR_DIV_TYPE9_CH_CNT;	div=3;	}
#ifndef GUI_8CH_SUPPORT
			else if(g_curType == SCR_DIV_TYPE16)	{	max=16;	div=4;	}
#ifndef GUI_16CH_SUPPORT
			else if(g_curType == SCR_DIV_TYPE36)	{	max=32;	div=6;	}
#endif
#endif
#endif
			else	break;

			for(i=0; i<max; i++)
			{
				temp = i%div;
				if(!temp)
				{
					obj1 = div_btn[1];
					obj2 = sel_ch[i+1];
				}
				else if(temp==div-1)
				{
					obj1 = sel_ch[i-1];
					obj2 = NULL;
				}
				else
				{
					obj1 = sel_ch[i-1];
					if(i+1 < GUI_CHANNEL_CNT)
						obj2 = sel_ch[i+1];
					else
						obj2 = NULL;
				}

				nfui_nfobject_set_data(sel_ch[i], "left", obj1);
				nfui_nfobject_set_data(sel_ch[i], "right", obj2);

				temp = i/div;
				if(!temp)
				{
					obj1 = NULL;
					obj2 = sel_ch[i+div];
				}
				else if(temp==div-1)
				{
					obj1 = sel_ch[i-div];
					obj2 = ss_btns[0];
				}
				else
				{
					obj1 = sel_ch[i-div];
					if(i+div < GUI_CHANNEL_CNT)
						obj2 = sel_ch[i+div];
					else
						obj2 = ss_btns[0];
				}

				nfui_nfobject_set_data(sel_ch[i], "up", obj1);
				nfui_nfobject_set_data(sel_ch[i], "down", obj2);
			}
		}
		break;
#ifndef GUI_4CH_SUPPORT
		case SCR_DIV_TYPE8:
			for(i=0; i<8; i++)
			{
				if(i==0)
				{
					nfui_nfobject_set_data(sel_ch[0], "left", div_btn[1]);
					nfui_nfobject_set_data(sel_ch[0], "right", sel_ch[1]);
					nfui_nfobject_set_data(sel_ch[0], "up", NULL);
					nfui_nfobject_set_data(sel_ch[0], "down", sel_ch[4]);
				}
				else if(i>=1 && i<4)
				{
					nfui_nfobject_set_data(sel_ch[i], "left", sel_ch[0]);
					nfui_nfobject_set_data(sel_ch[i], "right", NULL);
					if(i==1)	nfui_nfobject_set_data(sel_ch[i], "up", NULL);
					else		nfui_nfobject_set_data(sel_ch[i], "up", sel_ch[i-1]);
					if(i==3)	nfui_nfobject_set_data(sel_ch[i], "down", sel_ch[i+4]);
					else		nfui_nfobject_set_data(sel_ch[i], "down", sel_ch[i+1]);
				}
				else if(i>=4 && i<7)
				{
					if(i==4)	nfui_nfobject_set_data(sel_ch[i], "left", div_btn[1]);
					else		nfui_nfobject_set_data(sel_ch[i], "left", sel_ch[i-1]);
					nfui_nfobject_set_data(sel_ch[i], "right", sel_ch[i+1]);
					nfui_nfobject_set_data(sel_ch[i], "up", sel_ch[0]);
					nfui_nfobject_set_data(sel_ch[i], "down", ss_btns[0]);
				}
				else if(i==7)
				{
					nfui_nfobject_set_data(sel_ch[i], "left", sel_ch[i-1]);
					nfui_nfobject_set_data(sel_ch[i], "right", NULL);
					nfui_nfobject_set_data(sel_ch[i], "up", sel_ch[i-4]);
					nfui_nfobject_set_data(sel_ch[i], "down", ss_btns[0]);
				}
			}
			break;
		
		case SCR_DIV_TYPE6:
			for(i=0; i<6; i++)
			{
				if(i==0)
				{
					nfui_nfobject_set_data(sel_ch[0], "left", div_btn[1]);
					nfui_nfobject_set_data(sel_ch[0], "right", sel_ch[1]);
					nfui_nfobject_set_data(sel_ch[0], "up", NULL);
					nfui_nfobject_set_data(sel_ch[0], "down", sel_ch[3]);
				}
				else if(i==1 || i==2)
				{
					nfui_nfobject_set_data(sel_ch[i], "left", sel_ch[0]);
					nfui_nfobject_set_data(sel_ch[i], "right", NULL);
					if(i==1)	nfui_nfobject_set_data(sel_ch[i], "up", NULL);
					else		nfui_nfobject_set_data(sel_ch[i], "up", sel_ch[i-1]);
					if(i==2)	nfui_nfobject_set_data(sel_ch[i], "down", sel_ch[i+3]);
					else		nfui_nfobject_set_data(sel_ch[i], "down", sel_ch[i+1]);
				}
				else if(i==3 || i==4)
				{
					if(i==3)	nfui_nfobject_set_data(sel_ch[i], "left", div_btn[1]);
					else		nfui_nfobject_set_data(sel_ch[i], "left", sel_ch[i-1]);
					nfui_nfobject_set_data(sel_ch[i], "right", sel_ch[i+1]);
					nfui_nfobject_set_data(sel_ch[i], "up", sel_ch[0]);
					nfui_nfobject_set_data(sel_ch[i], "down", ss_btns[0]);
				}
				else if(i==5)
				{
					nfui_nfobject_set_data(sel_ch[i], "left", sel_ch[i-1]);
					nfui_nfobject_set_data(sel_ch[i], "right", NULL);
					nfui_nfobject_set_data(sel_ch[i], "up", sel_ch[i-3]);
					nfui_nfobject_set_data(sel_ch[i], "down", ss_btns[0]);
				}
			}
			break;
#endif

		default:
			break;
	}
}

static gboolean 
post_div_btn1_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_RADIO_GET_FOCUS)
	{
		guint i;
		NFOBJECT *top = NULL;

		if(g_curType == SCR_DIV_TYPE1)
			return FALSE;

		g_curType = SCR_DIV_TYPE1;
		prvCalcRect(g_rtRect, g_curType);
		
		top = nfui_nfobject_get_top(obj);
		nfui_make_key_hierarchy(top);

		//nfui_spin_button_set_index_no_expose(sel_ch[0], 0);
		nfui_nflabel_set_number((NFLABEL*)sel_ch[0], 1);

		nfui_signal_emit(ch_area, GDK_EXPOSE, TRUE);
		//prvMakeChNavTable();
	}
#if 0
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		return prvKPadEvtProcess(obj, evt);
	}
#endif

	return FALSE;
}

static gboolean 
post_div_btn4_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_RADIO_GET_FOCUS)
	{
		guint i;
		NFOBJECT *top = NULL;

		if(g_curType == SCR_DIV_TYPE4)
			return FALSE;

		g_curType = SCR_DIV_TYPE4;
		prvCalcRect(g_rtRect, g_curType);
		
		top = nfui_nfobject_get_top(obj);
		nfui_make_key_hierarchy(top);

		for(i=0; i<4; i++)
			nfui_nflabel_set_number((NFLABEL*)sel_ch[i], (i + 1));
			//nfui_spin_button_set_index_no_expose(sel_ch[i], i);

		nfui_signal_emit(ch_area, GDK_EXPOSE, TRUE);
		//prvMakeChNavTable();
	}
#if 0
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		return prvKPadEvtProcess(obj, evt);
	}
#endif

	return FALSE;

}


#ifndef GUI_4CH_SUPPORT
#ifdef GUI_8CH_SUPPORT
#define DIV9_SEL_CH_CNT     (8)
#else
#define DIV9_SEL_CH_CNT     (9)
#endif
static gboolean 
post_div_btn9_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_RADIO_GET_FOCUS)
	{
		
		
		guint i;
		NFOBJECT *top = NULL;

		if(g_curType == SCR_DIV_TYPE9)
			return FALSE;

		g_curType = SCR_DIV_TYPE9;
		prvCalcRect(g_rtRect, g_curType);
		
		top = nfui_nfobject_get_top(obj);
		nfui_make_key_hierarchy(top);

		for(i=0; i<DIV9_SEL_CH_CNT; i++)
			nfui_nflabel_set_number((NFLABEL*)sel_ch[i], (i + 1));
			//nfui_spin_button_set_index_no_expose(sel_ch[i], i);

		nfui_signal_emit(ch_area, GDK_EXPOSE, TRUE);
		//prvMakeChNavTable();
	}
#if 0
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		return prvKPadEvtProcess(obj, evt);
	}
#endif

	return FALSE;

}

#ifndef GUI_8CH_SUPPORT
static gboolean post_div_btn16_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_RADIO_GET_FOCUS)
	{
		guint i;
		NFOBJECT *top = NULL;

		if(g_curType == SCR_DIV_TYPE16)
			return FALSE;

		g_curType = SCR_DIV_TYPE16;
		prvCalcRect(g_rtRect, g_curType);
		
		top = nfui_nfobject_get_top(obj);
		nfui_make_key_hierarchy(top);

		for(i=0; i<16; i++)
			nfui_nflabel_set_number((NFLABEL*)sel_ch[i], (i + 1));
			//nfui_spin_button_set_index_no_expose(sel_ch[i], i);

		nfui_signal_emit(ch_area, GDK_EXPOSE, TRUE);
		//prvMakeChNavTable();
	}
#if 0
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		return prvKPadEvtProcess(obj, evt);
	}
#endif

	return FALSE;

}
#ifndef GUI_16CH_SUPPORT
static gboolean post_div_btn36_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_RADIO_GET_FOCUS)
	{
		guint i;
		NFOBJECT *top = NULL;

		if(g_curType == SCR_DIV_TYPE36)
			return FALSE;

		g_curType = SCR_DIV_TYPE36;
		prvCalcRect(g_rtRect, g_curType);
		
		top = nfui_nfobject_get_top(obj);
		nfui_make_key_hierarchy((NFWINDOW*)top);

		for(i = 0; i < 32; i++)
			nfui_nflabel_set_number((NFLABEL*)sel_ch[i], i + 1);

		nfui_signal_emit(ch_area, GDK_EXPOSE, TRUE);
	}
#if 0
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		return prvKPadEvtProcess(obj, evt);
	}
#endif

	return FALSE;

}
#endif
#endif
#endif

static gboolean post_div_btn8_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_RADIO_GET_FOCUS)
	{
		guint i;
		NFOBJECT *top = NULL;

		if(g_curType == SCR_DIV_TYPE8)
			return FALSE;

		g_curType = SCR_DIV_TYPE8;
		prvCalcRect(g_rtRect, g_curType);
		
		top = nfui_nfobject_get_top(obj);
		nfui_make_key_hierarchy(top);

		for(i=0; i<8; i++)
			nfui_nflabel_set_number((NFLABEL*)sel_ch[i], (i + 1));
			//nfui_spin_button_set_index_no_expose(sel_ch[i], i);

		nfui_signal_emit(ch_area, GDK_EXPOSE, TRUE);
		//prvMakeChNavTable();
	}
#if 0
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		return prvKPadEvtProcess(obj, evt);
	}
#endif

	return FALSE;
}

static gboolean 
post_div_btn6_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_RADIO_GET_FOCUS)
	{
		guint i;
		NFOBJECT *top = NULL;

		if(g_curType == SCR_DIV_TYPE6)
			return FALSE;

		g_curType = SCR_DIV_TYPE6;
		prvCalcRect(g_rtRect, g_curType);
		
		top = nfui_nfobject_get_top(obj);
		nfui_make_key_hierarchy(top);

		for(i=0; i<6; i++)
			nfui_nflabel_set_number((NFLABEL*)sel_ch[i], (i + 1));
			//nfui_spin_button_set_index_no_expose(sel_ch[i], i);

		nfui_signal_emit(ch_area, GDK_EXPOSE, TRUE);
		//prvMakeChNavTable();
	}
#if 0
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		return prvKPadEvtProcess(obj, evt);
	}
#endif

	return FALSE;
}
#endif

static void
change_sel_ch(gint dir)
{
	gint i;
	gint cnt;
	gint ch;
	
	switch(g_curType) {
		case SCR_DIV_TYPE1: cnt = 1; break;
		case SCR_DIV_TYPE4: cnt = 4; break;
#if !defined(GUI_4CH_SUPPORT)
		case SCR_DIV_TYPE6: cnt = 6; break;
		case SCR_DIV_TYPE8: cnt = 8; break;
		case SCR_DIV_TYPE9: cnt = SCR_DIV_TYPE9_CH_CNT; break;

#if !defined(GUI_8CH_SUPPORT)
		case SCR_DIV_TYPE16: cnt = 16; break;
		#if !defined (GUI_16CH_SUPPORT)
		case SCR_DIV_TYPE36: cnt = 32; break;
		#endif
#endif

#endif
	}

	for(i=0; i<cnt; i++) {
		ch = (guint)nfui_nflabel_get_number((NFLABEL*)sel_ch[i]);

		if(dir > 0) 
		{
			if(++ch > GUI_CHANNEL_CNT) 
				ch = 1;
		}
		else 
		{
			if(--ch < 1) 
				ch = GUI_CHANNEL_CNT;
		}

		nfui_nflabel_set_number((NFLABEL*)sel_ch[i], ch);
	}

	for(i=0; i<cnt; i++) 
		nfui_signal_emit(sel_ch[i], GDK_EXPOSE, FALSE);
}

static gboolean 
post_sel_ch_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
#if 1
	if (evt->type == GDK_SCROLL)
	{
		GdkEventScroll *sevt;

		sevt = (GdkEventScroll*)evt;
		if(sevt->direction == GDK_SCROLL_UP) {
			change_sel_ch(-1);
		}

		if(sevt->direction == GDK_SCROLL_DOWN) {
			change_sel_ch(1);
		}
	}
#else
	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		return prvKPadEvtProcess(obj, evt);
	}
#endif

	return FALSE;
}

static gboolean 
post_prev_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) 
		change_sel_ch(-1);
	
	return FALSE;
}

static gboolean 
post_next_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) 
		change_sel_ch(1);
	
	return FALSE;
}

static gboolean post_main_bg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;
        
	if(evt->type == GDK_EXPOSE) 
	{
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
    
		nfui_nfobject_gc_unref(gc);
	}
	else if(evt->type == GDK_DELETE)
	{
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
	
		if(radioList)	g_slist_free(radioList);
		radioList = NULL;
	}

	return FALSE;
}

static gboolean post_type_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;
	guint x, y;

	if(evt->type == GDK_EXPOSE) 
	{
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_offset(obj, &x, &y);	
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_TAB_BG, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, x, y, -1, -1, NFALIGN_LEFT, 0);
    
		nfui_nfobject_gc_unref(gc);
	}
	else if(evt->type == GDK_DELETE)
	{
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_TAB_BG, size_w, size_h);
	}
	
	return FALSE;
}

static gboolean post_config_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;
	guint x, y;

	if(evt->type == GDK_EXPOSE) 
	{
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_offset(obj, &x, &y);	
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_TAB_BG, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, x, y, -1, -1, NFALIGN_LEFT, 0);
    
		switch(g_curType)
		{
			case SCR_DIV_TYPE1:
				nfutil_draw_image(drawable, gc, IMG_SEQ_SETUP_BG_1, x+17, y+57, -1, -1, NFALIGN_LEFT, 0);
			break;
			case SCR_DIV_TYPE4:
				nfutil_draw_image(drawable, gc, IMG_SEQ_SETUP_BG_4, x+17, y+57, -1, -1, NFALIGN_LEFT, 0);
			break;
#ifndef GUI_4CH_SUPPORT
			case SCR_DIV_TYPE9:
				nfutil_draw_image(drawable, gc, IMG_SEQ_SETUP_BG_9, x+17, y+57, -1, -1, NFALIGN_LEFT, 0);				
			break;
#ifndef GUI_8CH_SUPPORT
			case SCR_DIV_TYPE16:
				nfutil_draw_image(drawable, gc, IMG_SEQ_SETUP_BG_16, x+17, y+57, -1, -1, NFALIGN_LEFT, 0);				
			break;
#ifndef GUI_16CH_SUPPORT
			case SCR_DIV_TYPE36:
				nfutil_draw_image(drawable, gc, IMG_SEQ_SETUP_BG_32, x+17, y+57, -1, -1, NFALIGN_LEFT, 0);				
			break;
#endif		
#endif		
			case SCR_DIV_TYPE8:
				nfutil_draw_image(drawable, gc, IMG_SEQ_SETUP_BG_8, x+17, y+57, -1, -1, NFALIGN_LEFT, 0);				
			break;
			case SCR_DIV_TYPE6:
				nfutil_draw_image(drawable, gc, IMG_SEQ_SETUP_BG_6, x+17, y+57, -1, -1, NFALIGN_LEFT, 0);				
			break;
#endif		
		}

		nfui_nfobject_gc_unref(gc);
	}
	else if (evt->type == GDK_SCROLL)
	{
		GdkEventScroll *sevt;

		sevt = (GdkEventScroll*)evt;
		if(sevt->direction == GDK_SCROLL_UP) {
			change_sel_ch(-1);
		}

		if(sevt->direction == GDK_SCROLL_DOWN) {
			change_sel_ch(1);
		}
	}
	else if (evt->type == GDK_DELETE)
	{
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_TAB_BG, size_w, size_h);
	}

	return FALSE;
}

static gboolean 
post_okbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *topwin;
		guint i, j;
		guint cnt=0;
		guint ch[32];

		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
		{
			return FALSE;
		}

		topwin = nfui_nfobject_get_top(obj);

		g_elem_data->type = g_curType;

		if(g_curType == SCR_DIV_TYPE1)			cnt = 1;
		else if(g_curType == SCR_DIV_TYPE4)		cnt = 4;
#ifndef GUI_4CH_SUPPORT
		else if(g_curType == SCR_DIV_TYPE9)		cnt = SCR_DIV_TYPE9_CH_CNT;
#ifndef GUI_8CH_SUPPORT
		else if(g_curType == SCR_DIV_TYPE16)	cnt = 16;
#ifndef GUI_16CH_SUPPORT
		else if(g_curType == SCR_DIV_TYPE36)	cnt = 32;
#endif		
#endif		
		else if(g_curType == SCR_DIV_TYPE8)		cnt = 8;
		else if(g_curType == SCR_DIV_TYPE6)		cnt = 6;
#endif
		else	return FALSE;
		

		for(i=0; i<cnt; i++)
		{
			//ch[i] = nfui_spin_button_get_index(sel_ch[i]);
			ch[i] = (guint)nfui_nflabel_get_number((NFLABEL*)sel_ch[i]);
			ch[i] -= 1;
		}

		if(cnt > 1)
		{
			for(i=0; i<cnt-1; i++)
			{
				for(j=i+1; j<cnt; j++)
					if(ch[i] == ch[j])	break;

				if(j!=cnt && ch[i]==ch[j])	break;
			}

			if(i!=cnt-1 || j!=cnt)
			{
				nftool_mbox(g_curwnd, "NOTICE", "There is a channel conflict.\nCheck it again.", NFTOOL_MB_OK);
				return FALSE;
			}
		}

		for(i=0; i<cnt; i++)
			g_elem_data->conf[i] = ch[i];

		nfui_nfobject_destroy(topwin);

		ret = 1;
	}
	return FALSE;
}

static gboolean 
post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *topwin;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
		{
			return FALSE;
		}


		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);

		ret = 0;
	}
	return FALSE;
}

static gboolean post_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_DELETE) {
		g_curwnd = 0;
		gtk_main_quit();
	}
	return FALSE;
}

guint SequenceSetupDlg_Open(NFWINDOW *parent, SeqElementData *elem_data, gboolean add)
{
	NFOBJECT *main_wnd, *main_fixed;
	NFOBJECT *fixed1, *fixed2;
	NFOBJECT *lbTemp;
	NFOBJECT *obj;

	GdkPixbuf *type_image[NUM_SCR_DIV_TYPES][NFOBJECT_STATE_COUNT];
	GdkPixbuf *prev_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *next_img[NFOBJECT_STATE_COUNT];
	
	const gchar *strButton[] = {"CONFIRM", "CANCEL"};

	guint pos_x, pos_y;
	gint btn_w, btn_h;
	guint i;


	ret = 0;
	g_elem_data = elem_data;

	type_image[0][0] = nfui_get_image_from_file((IMG_SEQ_TYPE1_N), NULL);
	type_image[0][1] = nfui_get_image_from_file((IMG_SEQ_TYPE1_S), NULL);
	type_image[0][2] = nfui_get_image_from_file((IMG_SEQ_TYPE1_S), NULL);
	type_image[0][3] = nfui_get_image_from_file((IMG_SEQ_TYPE1_S), NULL);
	
	type_image[1][0] = nfui_get_image_from_file((IMG_SEQ_TYPE4_N), NULL);
	type_image[1][1] = nfui_get_image_from_file((IMG_SEQ_TYPE4_S), NULL);
	type_image[1][2] = nfui_get_image_from_file((IMG_SEQ_TYPE4_S), NULL);
	type_image[1][3] = nfui_get_image_from_file((IMG_SEQ_TYPE4_S), NULL);

#ifndef GUI_4CH_SUPPORT
	
	type_image[2][0] = nfui_get_image_from_file((IMG_SEQ_TYPE9_N), NULL);	
	type_image[2][1] = nfui_get_image_from_file((IMG_SEQ_TYPE9_S), NULL);
	type_image[2][2] = nfui_get_image_from_file((IMG_SEQ_TYPE9_S), NULL);
	type_image[2][3] = nfui_get_image_from_file((IMG_SEQ_TYPE9_S), NULL);

#ifndef GUI_8CH_SUPPORT
	
	type_image[3][0] = nfui_get_image_from_file((IMG_SEQ_TYPE16_N), NULL);
	type_image[3][1] = nfui_get_image_from_file((IMG_SEQ_TYPE16_S), NULL);
	type_image[3][2] = nfui_get_image_from_file((IMG_SEQ_TYPE16_S), NULL);
	type_image[3][3] = nfui_get_image_from_file((IMG_SEQ_TYPE16_S), NULL);
#ifndef GUI_16CH_SUPPORT
	type_image[4][0] = nfui_get_image_from_file((IMG_SEQ_TYPE32_N), NULL);
	type_image[4][1] = nfui_get_image_from_file((IMG_SEQ_TYPE32_S), NULL);
	type_image[4][2] = nfui_get_image_from_file((IMG_SEQ_TYPE32_S), NULL);
	type_image[4][3] = nfui_get_image_from_file((IMG_SEQ_TYPE32_S), NULL);
#endif
#endif

	type_image[DISP_TYPE8_IMG_IDX][0] = nfui_get_image_from_file((IMG_SEQ_TYPE8_N), NULL);
	type_image[DISP_TYPE8_IMG_IDX][1] = nfui_get_image_from_file((IMG_SEQ_TYPE8_S), NULL);
	type_image[DISP_TYPE8_IMG_IDX][2] = nfui_get_image_from_file((IMG_SEQ_TYPE8_S), NULL);
	type_image[DISP_TYPE8_IMG_IDX][3] = nfui_get_image_from_file((IMG_SEQ_TYPE8_S), NULL);
	
	type_image[DISP_TYPE6_IMG_IDX][0] = nfui_get_image_from_file((IMG_SEQ_TYPE6_N), NULL);
	type_image[DISP_TYPE6_IMG_IDX][1] = nfui_get_image_from_file((IMG_SEQ_TYPE6_S), NULL);
	type_image[DISP_TYPE6_IMG_IDX][2] = nfui_get_image_from_file((IMG_SEQ_TYPE6_S), NULL);
	type_image[DISP_TYPE6_IMG_IDX][3] = nfui_get_image_from_file((IMG_SEQ_TYPE6_S), NULL);
#endif

	prev_img[0] = nfui_get_image_from_file((IMG_N_POP_ARROW_LEFT), NULL);
	prev_img[1] = nfui_get_image_from_file((IMG_O_POP_ARROW_LEFT), NULL);	
	prev_img[2] = nfui_get_image_from_file((IMG_P_POP_ARROW_LEFT), NULL);	
	prev_img[3] = nfui_get_image_from_file((IMG_D_POP_ARROW_LEFT), NULL);	

	next_img[0] = nfui_get_image_from_file((IMG_N_POP_ARROW_RIGHT), NULL);
	next_img[1] = nfui_get_image_from_file((IMG_O_POP_ARROW_RIGHT), NULL);	
	next_img[2] = nfui_get_image_from_file((IMG_P_POP_ARROW_RIGHT), NULL);	
	next_img[3] = nfui_get_image_from_file((IMG_D_POP_ARROW_RIGHT), NULL);	

	main_wnd = nftool_create_popup_window(parent, PP_POS_X, PP_POS_Y, PP_SIZE_WID, PP_SIZE_HEI, "", FALSE);
	nfui_regi_post_event_callback(main_wnd, post_win_event_handler);
	main_fixed = ((NFWINDOW*)main_wnd)->child;
	g_curwnd = main_wnd;

	fixed1 = nfui_nffixed_new();
	nfui_nfobject_set_size(fixed1, 212, 422);
	nfui_nffixed_put((NFFIXED*)main_fixed, fixed1, 15, 76);
	nfui_regi_post_event_callback(fixed1, post_type_fixed_event_handler);	
	nfui_nfobject_show(fixed1);

	fixed2 = nfui_nffixed_new();
	nfui_nfobject_set_size(fixed2, 308, 422);
	nfui_nffixed_put((NFFIXED*)main_fixed, fixed2, 255, 76);
	nfui_regi_post_event_callback(fixed2, post_config_fixed_event_handler);	
	nfui_nfobject_show(fixed2);
	ch_area = fixed2;

	lbTemp = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SEQUENCE SETUP", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
	nfui_nflabel_set_align((NFLABEL*)lbTemp, NFALIGN_CENTER, 23);
	nfui_nfobject_set_size(lbTemp, PP_SIZE_WID-30, 36);
	nfui_nffixed_put((NFFIXED*)main_fixed, lbTemp, 15, 4);
	nfui_nfobject_use_focus(lbTemp, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(lbTemp);

	lbTemp = nfui_nflabel_new_with_pango_font("VIEW TYPE", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)lbTemp, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(lbTemp, 212, 22);
	nfui_nffixed_put((NFFIXED*)main_fixed, lbTemp, 37, 50);
	nfui_nfobject_use_focus(lbTemp, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(lbTemp);

	lbTemp = nfui_nflabel_new_with_pango_font("VIEW CONFIGURE", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)lbTemp, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(lbTemp, 308, 22);
	nfui_nffixed_put((NFFIXED*)main_fixed, lbTemp, 277, 50);
	nfui_nfobject_use_focus(lbTemp, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(lbTemp);

	for(i=0; i<NUM_SCR_DIV_TYPES; i++)
	{
		nfui_get_pixbuf_size(type_image[i][0], &btn_w, &btn_h);
		
		if(i%2)		pos_x = 113;
		else		pos_x = 18;

		pos_y = (20) + (80+16)*(i/2);

		div_btn[i] = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(div_btn[i]), type_image[i]);
		nfui_nfobject_set_size(div_btn[i], btn_w, btn_h);
		nfui_nffixed_put((NFFIXED*)fixed1, div_btn[i], pos_x, pos_y);
		nfui_nfobject_show(div_btn[i]);

		if(i==0)	 radioList = nfui_radio_button_get_group(NF_BUTTON(div_btn[i]));
		else		nfui_radio_button_add_group(NF_BUTTON(div_btn[i]), radioList);
	}

	for(i=0; i<GUI_CHANNEL_CNT; i++)
	{
		sel_ch[i] = nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(743));
		nfui_nflabel_set_align((NFLABEL*)sel_ch[i], NFALIGN_CENTER, 0);
		nfui_nflabel_use_number((NFLABEL*)sel_ch[i], TRUE);
		nfui_nflabel_set_number((NFLABEL*)sel_ch[i], (i + 1));
		nfui_nfobject_modify_bg(sel_ch[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(742));
		nfui_nfobject_set_size(sel_ch[i], CHANNEL_COMBO_WIDTH, 40);
		nfui_nfobject_use_focus(sel_ch[i], NFOBJECT_FOCUS_OFF);
		nfui_nfobject_use_tooltip(sel_ch[i], FALSE);
		nfui_nffixed_put((NFFIXED*)fixed2, sel_ch[i], 0, 0);
		nfui_regi_post_event_callback(sel_ch[i], post_sel_ch_event_handler);
	}

	//free_channel_string(strChannel);

// < > BUTTONS
	nfui_get_pixbuf_size(prev_img[0], &btn_w, &btn_h);
	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), prev_img);
	nfui_nfobject_set_size(obj, (guint)btn_w, (guint)btn_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, 84, 340); 
	nfui_regi_post_event_callback(obj, post_prev_event_handler);

	nfui_get_pixbuf_size(next_img[0], &btn_w, &btn_h);
	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), next_img);
	nfui_nfobject_set_size(obj, (guint)btn_w, (guint)btn_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, 164, 340); 
	nfui_regi_post_event_callback(obj, post_next_event_handler);

	pos_x = 94;

	for(i=0; i<SSB_BUTTONS; i++)
	{
		ss_btns[i] = nftool_normal_button_create_type1(strButton[i], 192);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(ss_btns[i]),NFALIGN_CENTER,0);
		nfui_nfobject_show(ss_btns[i]);
		nfui_nffixed_put((NFFIXED*)main_fixed, ss_btns[i], pos_x, PP_SIZE_HEI-62);
		pos_x += 198;
	}

	if(add)
	{
		g_curType = SCR_DIV_DEFAULT_TYPE;
		nfui_radio_button_set_toggled(NF_BUTTON(div_btn[g_curType]), TRUE);
		prvCalcRect(g_rtRect, g_curType);
	}
	else
	{
		g_curType = g_elem_data->type;
		nfui_radio_button_set_toggled(NF_BUTTON(div_btn[g_curType]), TRUE);
		prvCalcRect(g_rtRect, g_curType);

		for(i=0; i<GUI_CHANNEL_CNT; i++)
		{
			//nfui_spin_button_set_index_no_expose(sel_ch[i], g_elem_data->conf[i]);
			nfui_nflabel_set_number((NFLABEL*)sel_ch[i], (g_elem_data->conf[i] + 1));
		}
	}

	// VIEW TYPE div_btn EVENT
	nfui_regi_post_event_callback(div_btn[0], post_div_btn1_event_handler);
	nfui_regi_post_event_callback(div_btn[1], post_div_btn4_event_handler);
#ifndef GUI_4CH_SUPPORT
	nfui_regi_post_event_callback(div_btn[2], post_div_btn9_event_handler);
#ifndef GUI_8CH_SUPPORT
	nfui_regi_post_event_callback(div_btn[3], post_div_btn16_event_handler);  
        #ifndef GUI_16CH_SUPPORT
            nfui_regi_post_event_callback(div_btn[4], post_div_btn36_event_handler);  
        #endif
#endif	
	nfui_regi_post_event_callback(div_btn[DISP_TYPE8_IMG_IDX], post_div_btn8_event_handler);
	nfui_regi_post_event_callback(div_btn[DISP_TYPE6_IMG_IDX], post_div_btn6_event_handler);
#endif
	nfui_regi_post_event_callback(main_fixed, post_main_bg_event_handler);
	nfui_regi_post_event_callback(ss_btns[SSB_OK], post_okbutton_event_handler);
	nfui_regi_post_event_callback(ss_btns[SSB_CANCEL], post_cancelbutton_event_handler);

	nfui_nfobject_show(main_wnd);

	/* set for key navi */
	nfui_make_key_hierarchy(main_wnd);
	nfui_set_key_focus(ss_btns[0], TRUE);

#if 0
#if	defined(GUI_4CH_SUPPORT)
	nfui_nfobject_set_data(div_btn[0], "right", div_btn[1]);
	nfui_nfobject_set_data(div_btn[0], "down", ss_btns[0]);

	nfui_nfobject_set_data(div_btn[1], "left", div_btn[0]);
	nfui_nfobject_set_data(div_btn[1], "right", sel_ch[0]);
	nfui_nfobject_set_data(div_btn[1], "down", ss_btns[0]);
#elif defined(GUI_8CH_SUPPORT)
	nfui_nfobject_set_data(div_btn[0], "right", div_btn[1]);
	nfui_nfobject_set_data(div_btn[0], "down", div_btn[2]);

	nfui_nfobject_set_data(div_btn[1], "left", div_btn[0]);
	nfui_nfobject_set_data(div_btn[1], "right", sel_ch[0]);
	nfui_nfobject_set_data(div_btn[1], "down", div_btn[3]);

	nfui_nfobject_set_data(div_btn[2], "right", div_btn[3]);
	nfui_nfobject_set_data(div_btn[2], "up", div_btn[0]);
	nfui_nfobject_set_data(div_btn[2], "down", div_btn[4]);

	nfui_nfobject_set_data(div_btn[3], "left", div_btn[2]);
	nfui_nfobject_set_data(div_btn[3], "right", sel_ch[0]);
	nfui_nfobject_set_data(div_btn[3], "up", div_btn[1]);
	nfui_nfobject_set_data(div_btn[3], "down", ss_btns[0]);

	nfui_nfobject_set_data(div_btn[4], "right", div_btn[3]);
	nfui_nfobject_set_data(div_btn[4], "up", div_btn[2]);
	nfui_nfobject_set_data(div_btn[4], "down", ss_btns[0]);
#else
	nfui_nfobject_set_data(div_btn[0], "right", div_btn[1]);
	nfui_nfobject_set_data(div_btn[0], "down", div_btn[2]);

	nfui_nfobject_set_data(div_btn[1], "left", div_btn[0]);
	nfui_nfobject_set_data(div_btn[1], "right", sel_ch[0]);
	nfui_nfobject_set_data(div_btn[1], "down", div_btn[3]);

	nfui_nfobject_set_data(div_btn[2], "right", div_btn[3]);
	nfui_nfobject_set_data(div_btn[2], "up", div_btn[0]);
	nfui_nfobject_set_data(div_btn[2], "down", div_btn[4]);

	nfui_nfobject_set_data(div_btn[3], "left", div_btn[2]);
	nfui_nfobject_set_data(div_btn[3], "right", sel_ch[0]);
	nfui_nfobject_set_data(div_btn[3], "up", div_btn[1]);
	nfui_nfobject_set_data(div_btn[3], "down", div_btn[5]);

	nfui_nfobject_set_data(div_btn[4], "right", div_btn[5]);
	nfui_nfobject_set_data(div_btn[4], "up", div_btn[2]);
	nfui_nfobject_set_data(div_btn[4], "down", ss_btns[0]);

	nfui_nfobject_set_data(div_btn[5], "left", div_btn[4]);
	nfui_nfobject_set_data(div_btn[5], "right", sel_ch[0]);
	nfui_nfobject_set_data(div_btn[5], "up", div_btn[3]);
	nfui_nfobject_set_data(div_btn[5], "down", ss_btns[0]);
#endif

	prvMakeChNavTable();
#endif

	nfui_page_close(PGID_POPUPWND, main_wnd);
	nfui_page_open(PGID_SEQ_SETUP, main_wnd, nfui_get_last_user());

	gtk_main();

	nfui_page_close(PGID_SEQ_SETUP, main_wnd);
	
	return ret;
}



