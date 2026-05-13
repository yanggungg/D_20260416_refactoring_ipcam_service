

#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"
#include "support/event_loop.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"

#include "vw_sys_disp_sequence.h"
#include "vw_sys_disp_sequence_conf.h"
#include "vw_sys_disp_sequence_setup.h"
#include "vw_sys_disp_sequence_conf_submenu.h"

#define	CONF_WIDTH			(guint)(DISPLAY_IS_D1 ? 644:810)
#define	CONF_HEIGHT			(guint)(DISPLAY_IS_D1 ? 400:869)

#define CONF_POS_X      	((DISPLAY_ACTIVE_WIDTH - CONF_WIDTH)/2)
#define	CONF_POS_Y			((DISPLAY_ACTIVE_HEIGHT - CONF_HEIGHT)/2)

#define	CONF_MENU_WIDTH		(guint)(DISPLAY_IS_D1 ? 100:200)
#define	CONF_MENU_HEIGHT	(guint)(DISPLAY_IS_D1 ? 76:168)

#define CONF_BTN_POS_Y		(CONF_HEIGHT - 64)

// 48X  +2 ...
#define	CONF_CELL_WIDTH		(guint)(DISPLAY_IS_D1 ? 75:150)
#define	CONF_BORDER_SIZE	(guint)(DISPLAY_IS_D1 ? 2:4)


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *conf_fixed[16];

static SeqElementData *g_elem_data;
static gboolean g_edit_enable = FALSE;
static guint g_disp_mode;
static guint *g_num_items;
static gint cur_focus = -1;

static guint template1[NUM_DISP_MODE][1][4] = {
	{
		{1, 1, 71, 71},
	},
	{
		{1, 1, 146, 146},
	}
};

static guint template4[NUM_DISP_MODE][4][4] = {
	{
		{1, 1, 35, 35,},
		{37, 1, 35, 35,},
		{1, 37, 35, 35,},
		{37, 37, 35, 35,}
	},
	{
		{1, 1, 72, 72,},
		{75, 1, 72, 72,},
		{1, 75, 72, 72,},
		{75, 75, 72, 72,}
	}
};

static guint template9[NUM_DISP_MODE][9][4] = {
	{
		{1,		1,		23, 23},
		{25,	1,		23, 23},
		{49,	1,		23, 23},
		{1,		25,		23, 23},
		{25,	25,		23, 23},
		{49,	25,		23, 23},
		{1,		49,		23, 23},
		{25,	49,		23, 23},
		{49,	49,		23, 23}
	},
	{
		{1,		1,		47, 47},
		{50,	1,		47, 47},
		{99,	1,		47, 47},
		{1,		50,		47, 47},
		{50,	50,		47, 47},
		{99,	50,		47, 47},
		{1,		99,		47, 47},
		{50,	99,		47, 47},
		{99,	99,		47, 47}
	}
};

static guint template16[NUM_DISP_MODE][16][4] = {
	{
		{1,		1,		17, 17},
		{19,	1,		17, 17},
		{37,	1,		17, 17},
		{55,	1,		17, 17},
		{1,		19,		17, 17},
		{19,	19,		17, 17},
		{37,	19,		17, 17},
		{55,	19,		17, 17},
		{1,		37,		17, 17},
		{19,	37,		17, 17},
		{37,	37,		17, 17},
		{55,	37,		17, 17},
		{1,		55,		17, 17},
		{19,	55,		17, 17},
		{37,	55,		17, 17},
		{55,	55,		17, 17}
	},
	{
		{1,		1,		35, 35},
		{37,	1,		35, 35},
		{74,	1,		35, 35},
		{111,	1,		35, 35},
		{1,		37,		35, 35},
		{37,	37,		35, 35},
		{74,	37,		35, 35},
		{111,	37,		35, 35},
		{1,		74,		35, 35},
		{37,	74,		35, 35},
		{74,	74,		35, 35},
		{111,	74,		35, 35},
		{1,		111,	35, 35},
		{37,	111,	35, 35},
		{74,	111,	35, 35},
		{111,	111,	35, 35}
	}
};

static guint template36[NUM_DISP_MODE][36][4] = {
	{
		{1,		1,		11, 11},
		{13,	1,		11, 11},
		{25,	1,		11, 11},
		{37,	1,		11, 11},
		{49,	1,		11, 11},
		{61,	1,		11, 11},
		
		{1,		13,		11, 11},
		{13,	13,		11, 11},
		{25,	13,		11, 11},
		{37,	13,		11, 11},
		{49,	13,		11, 11},
		{61,	13,		11, 11},
		
		{1,		25,		11, 11},
		{13,	25,		11, 11},
		{25,	25,		11, 11},
		{37,	25,		11, 11},
		{49,	25,		11, 11},
		{61,	25,		11, 11},
		
		{1,		37,		11, 11},
		{13,	37,		11, 11},
		{25,	37,		11, 11},
		{37,	37,		11, 11},
		{49,	37,		11, 11},
		{61,	37,		11, 11},
		
		{1,		49,		11, 11},
		{13,	49,		11, 11},
		{25,	49,		11, 11},
		{37,	49,		11, 11},
		{49,	49,		11, 11},
		{61,	49,		11, 11},
		
		{1,		61,		11, 11},
		{13,	61,		11, 11},
		{25,	61,		11, 11},
		{37,	61,		11, 11},
		{49,	61,		11, 11},
		{61,	61,		11, 11},
	},
	{
		{1,		1,		23, 23},
		{25,	1,		23, 23},
		{49,	1,		23, 23},
		{74,	1,		23, 23},
		{99,	1,		23, 23},
		{123,	1,		23, 23},
		
		{1,		25,		23, 23},
		{25,	25,		23, 23},
		{49,	25,		23, 23},
		{74,	25,		23, 23},
		{99,	25,		23, 23},
		{123,	25,		23, 23},
		
		{1,		49,		23, 23},
		{25,	49,		23, 23},
		{49,	49,		23, 23},
		{74,	49,		23, 23},
		{99,	49,		23, 23},
		{123,	49,		23, 23},
		
		{1,		74,		23, 23},
		{25,	74,		23, 23},
		{49,	74,		23, 23},
		{74,	74,		23, 23},
		{99,	74,		23, 23},
		{123,	74,		23, 23},
		
		{1,		99,		23, 23},
		{25,	99,		23, 23},
		{49,	99,		23, 23},
		{74,	99,		23, 23},
		{99,	99,		23, 23},
		{123,	99,		23, 23},
		
		{1,		123,		23, 23},
		{25,	123,		23, 23},
		{49,	123,		23, 23},
		{74,	123,		23, 23},
		{99,	123,		23, 23},
		{123,	123,		23, 23},
	}
};

static guint template6[NUM_DISP_MODE][6][4] = {
	{
		{1,		1,		47,	47},
		{49,	1,		23, 23},
		{49,	25,		23, 23},
		{1,		49,		23, 23},
		{25,	49,		23, 23},
		{49,	49,		23, 23}
	},
	{
		{1,		1,		96,	96},
		{99,	1,		47, 47},
		{99,	50,		47, 47},
		{1,		99,		47, 47},
		{50,	99,		47, 47},
		{99,	99,		47, 47}
	}
};

static guint template8[NUM_DISP_MODE][8][4] = {
	{
		{1,		1,		53, 53},
		{55,	1,		17, 17},
		{55,	19,		17, 17},
		{55,	37,		17, 17},
		{1,		55,		17, 17},
		{19,	55,		17, 17},
		{37,	55,		17, 17},
		{55,	55,		17, 17}
	},
	{
		{1,		1,		108, 108},
		{111,	1,		35, 35},
		{111,	37,		35, 35},
		{111,	74,		35, 35},
		{1,		111,	35, 35},
		{37,	111,	35, 35},
		{74,	111,	35, 35},
		{111,	111,	35, 35}
	}
};


static void prvDrawCell(NFOBJECT *obj, guint type, guint *chmap)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	guint i;
	guint off_x, off_y;
	guint x, y, w, h;
	gchar strBuf[16];

	drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
	gc = nfui_nfobject_get_gc(obj);

	nfui_nfobject_get_offset(obj, &off_x, &off_y);

	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(738));
	gdk_draw_rectangle(drawable, gc, TRUE, off_x, off_y, CONF_CELL_WIDTH, CONF_CELL_WIDTH);
	
	off_x++;
	off_y++;

	if(type == SCR_DIV_TYPE1)
	{
		x = off_x+template1[g_disp_mode][0][0];
		y = off_y+template1[g_disp_mode][0][1];
		w = template1[g_disp_mode][0][2];
		h = template1[g_disp_mode][0][3];

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(737));
		gdk_draw_rectangle(drawable, gc, TRUE, x, y, w, h);

		sprintf(strBuf, "%d", chmap[0]+1);
		nfutil_draw_text_with_pango("SEQ_CONT", NULL, &UX_COLOR(737), drawable, gc, strBuf, x, y, w, h, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), &UX_COLOR(739), NFALIGN_CENTER, 0);
		
	}
	else if(type == SCR_DIV_TYPE4)
	{
		for(i=0; i<4; i++)
		{
			x = off_x+template4[g_disp_mode][i][0];
			y = off_y+template4[g_disp_mode][i][1];
			w = template4[g_disp_mode][i][2];
			h = template4[g_disp_mode][i][3];

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(737));
			gdk_draw_rectangle(drawable, gc, TRUE, x, y, w, h);

			sprintf(strBuf, "%d", chmap[i]+1);
			nfutil_draw_text_with_pango("SEQ_CONT",NULL, &UX_COLOR(737), drawable, gc, strBuf, x, y, w, h, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), &UX_COLOR(739), NFALIGN_CENTER, 0);
		}
	}
#ifndef GUI_4CH_SUPPORT
	else if(type == SCR_DIV_TYPE9)
	{
		for(i=0; i<9; i++)
		{
			x = off_x+template9[g_disp_mode][i][0];
			y = off_y+template9[g_disp_mode][i][1];
			w = template9[g_disp_mode][i][2];
			h = template9[g_disp_mode][i][3];

			if(i >= GUI_CHANNEL_CNT) break;

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(737));
			gdk_draw_rectangle(drawable, gc, TRUE, x, y, w, h);

			sprintf(strBuf, "%d", chmap[i]+1);
			nfutil_draw_text_with_pango("SEQ_CONT",NULL, &UX_COLOR(737), drawable, gc, strBuf, x, y, w, h, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), &UX_COLOR(739), NFALIGN_CENTER, 0);
		}
	}
#ifndef GUI_8CH_SUPPORT
	else if(type == SCR_DIV_TYPE16)
	{
		for(i=0; i<16; i++)
		{
			x = off_x+template16[g_disp_mode][i][0];
			y = off_y+template16[g_disp_mode][i][1];
			w = template16[g_disp_mode][i][2];
			h = template16[g_disp_mode][i][3];

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(737));
			gdk_draw_rectangle(drawable, gc, TRUE, x, y, w, h);

			sprintf(strBuf, "%d", chmap[i]+1);
			nfutil_draw_text_with_pango("SEQ_CONT", NULL, &UX_COLOR(737), drawable, gc, strBuf, x, y, w, h, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), &UX_COLOR(739), NFALIGN_CENTER, 0);
		}
	}
#ifndef GUI_16CH_SUPPORT
	else if(type == SCR_DIV_TYPE36)
	{
		for(i=0; i<36; i++)
		{
			x = off_x+template36[g_disp_mode][i][0];
			y = off_y+template36[g_disp_mode][i][1];
			w = template36[g_disp_mode][i][2];
			h = template36[g_disp_mode][i][3];

			if(i >= GUI_CHANNEL_CNT) break;

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(737));
			gdk_draw_rectangle(drawable, gc, TRUE, x, y, w, h);

			sprintf(strBuf, "%d", chmap[i]+1);
			nfutil_draw_text_with_pango("SEQ_CONT", NULL, &UX_COLOR(737), drawable, gc, strBuf, x, y, w, h, nffont_get_pango_font(NFFONT_SMALL_SEMI), &UX_COLOR(739), NFALIGN_CENTER, 0);
		}
	}
#endif
#endif	
#ifndef _NOT_SUPPORT_SPC_DIV
	else if(type == SCR_DIV_TYPE6)
	{
		for(i=0; i<6; i++)
		{
			x = off_x+template6[g_disp_mode][i][0];
			y = off_y+template6[g_disp_mode][i][1];
			w = template6[g_disp_mode][i][2];
			h = template6[g_disp_mode][i][3];

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(737));
			gdk_draw_rectangle(drawable, gc, TRUE, x, y, w, h);

			sprintf(strBuf, "%d", chmap[i]+1);
			nfutil_draw_text_with_pango("SEQ_CONT", NULL, &UX_COLOR(737), drawable, gc, strBuf, x, y, w, h, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), &UX_COLOR(739), NFALIGN_CENTER, 0);
		}
	}
	else if(type == SCR_DIV_TYPE8)
	{
		for(i=0; i<8; i++)
		{
			x = off_x+template8[g_disp_mode][i][0];
			y = off_y+template8[g_disp_mode][i][1];
			w = template8[g_disp_mode][i][2];
			h = template8[g_disp_mode][i][3];

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(737));
			gdk_draw_rectangle(drawable, gc, TRUE, x, y, w, h);

			sprintf(strBuf, "%d", chmap[i]+1);
			nfutil_draw_text_with_pango("SEQ_CONT", NULL, &UX_COLOR(737), drawable, gc, strBuf, x, y, w, h, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), &UX_COLOR(739), NFALIGN_CENTER, 0);
		}
	}
#endif
#endif
	
	nfui_nfobject_gc_unref(gc);
}

static void prvDrawCell_add(NFOBJECT *obj)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	guint off_x, off_y;
	guint x, y, w, h;

	drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
	gc = nfui_nfobject_get_gc(obj);

	nfui_nfobject_get_offset(obj, &off_x, &off_y);

	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(741));
	gdk_draw_rectangle(drawable, gc, TRUE, off_x, off_y, CONF_CELL_WIDTH, CONF_CELL_WIDTH);

	off_x++;
	off_y++;

	x = off_x+template1[g_disp_mode][0][0];
	y = off_y+template1[g_disp_mode][0][1];
	w = template1[g_disp_mode][0][2];
	h = template1[g_disp_mode][0][3];

	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(740));
	gdk_draw_rectangle(drawable, gc, TRUE, x, y, w, h);

	nfui_nfobject_gc_unref(gc);
}

static gboolean _refresh_seq_config_item(gpointer data)
{
	NFOBJECT *top;
	gint i;

	for (i = 0; i < 16; i++)
	{
		if (i <*g_num_items)
			nfui_nfobject_use_focus(conf_fixed[i], NFOBJECT_FOCUS_ON);
		else
			nfui_nfobject_use_focus(conf_fixed[i], NFOBJECT_FOCUS_OFF);
	
		nfui_signal_emit(conf_fixed[i], GDK_EXPOSE, TRUE);
	}

	top = nfui_nfobject_get_top(conf_fixed[0]);
	nfui_make_key_hierarchy((NFWINDOW*)top);		

	return FALSE;
}

static void _process_popup_ret_val(gint val)
{
	guint ret;
	gint i;

	if (val == 0)			// modify
	{
		if(cur_focus<0 || cur_focus>=16)
			return;
	
		ret = SequenceSetupDlg_Open(g_curwnd, &g_elem_data[cur_focus], FALSE);
		if(ret)	nfui_signal_emit(conf_fixed[cur_focus], GDK_EXPOSE, TRUE);
	}
	else if (val == 1)		// delete
	{
		if(*g_num_items<=0 || cur_focus<0 || cur_focus>=16)
			return;
	
		for(i=cur_focus; i<(*g_num_items); i++)
			g_memmove(&g_elem_data[i], &g_elem_data[i+1], sizeof(SeqElementData));

		(*g_num_items)--;
		cur_focus = -1;

		g_timeout_add(50, _refresh_seq_config_item, NULL);
	}
}

static gboolean pre_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
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
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
    
		nfui_nfobject_gc_unref(gc);
	}
    else if (evt->type == GDK_DELETE)
    {
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
    }
	
	return FALSE;
}

static gboolean pre_conf_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	guint x, y;
	gint i;

	if(evt->type == GDK_EXPOSE)
	{
		for(i=0; i<16; i++)
		{
			if(conf_fixed[i] == obj)
				break;
		}

		if(i>=0 && i<16)
		{
			if (i < *g_num_items)
				prvDrawCell(obj, g_elem_data[i].type, g_elem_data[i].conf);
			else
				prvDrawCell_add(obj);

			if ((i==cur_focus) && (i < *g_num_items))
				nftool_draw_object_border(obj, 741, 2);
		}
	}
	else if(evt->type == GDK_BUTTON_PRESS || evt->type == NFEVENT_KEYFOCUS_CHANGED)
	{
		gint temp;

		for(i=0; i<16; i++)
		{
			if(conf_fixed[i] == obj)
				break;
		}

		temp = cur_focus;

		if(i>=0 && i<16)	cur_focus = i;
		else				return FALSE;
			
		drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
		gc = nfui_nfobject_get_gc(obj);

		if(temp>=0 && temp<16)
			nfui_signal_emit(conf_fixed[temp], GDK_EXPOSE, TRUE);

		nfui_signal_emit(conf_fixed[cur_focus], GDK_EXPOSE, TRUE);

		nfui_nfobject_gc_unref(gc);
	}
	else if(evt->type == GDK_2BUTTON_PRESS)
	{
		// open modify window
		guint ret;
		GdkEventButton *bevent; 
		
		bevent = (GdkEventButton *)evt;
		if(evt->button.button != 1)	// 3 == LEFT BUTTON of Mouse
			return FALSE;

		if(g_edit_enable == FALSE)
		{
			nftool_mbox(g_curwnd, "NOTICE", "The default value cannot be changed.",
								NFTOOL_MB_OK);
			return FALSE;
		}

		if(cur_focus<0 || cur_focus>=(*g_num_items))
			return FALSE;

		ret = SequenceSetupDlg_Open(g_curwnd, &g_elem_data[cur_focus], FALSE);

		if(ret)
		{
			nfui_signal_emit(conf_fixed[cur_focus], GDK_EXPOSE, TRUE);
		}
	}
	else if(evt->type == GDK_BUTTON_RELEASE)
	{
		GdkEventButton *bevent; 
		gint ret_val;
		
		bevent = (GdkEventButton *)evt;
		if(evt->button.button != 3)	// 3 == RIGHT BUTTON of Mouse
			return FALSE;

		if(g_edit_enable == FALSE)
		{
			nftool_mbox(g_curwnd, "NOTICE", "The default value cannot be changed.",
								NFTOOL_MB_OK);
			return FALSE;
		}

		for(i=0; i<16; i++)
		{
			if(conf_fixed[i] == obj)
				break;
		}

		if (i >= (*g_num_items)) return FALSE;

		nfui_nfobject_get_window_pos(obj, &x, &y);

		x += bevent->x;
		y += bevent->y;

		if(DISPLAY_ACTIVE_WIDTH < x+CONF_MENU_WIDTH)
			x = DISPLAY_ACTIVE_WIDTH - CONF_MENU_WIDTH;
		if(DISPLAY_ACTIVE_HEIGHT < y+CONF_MENU_HEIGHT)
			y = DISPLAY_ACTIVE_HEIGHT - CONF_MENU_HEIGHT;

		ret_val = VW_Create_Seq_SubMenu(g_curwnd, x, y);
		_process_popup_ret_val(ret_val);		
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid = KEYPAD_NONE;
		guint wx, wy;
		gint ret_val;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid != KEYPAD_ENTER)
			return FALSE;

		if(g_edit_enable == FALSE)
		{
			nftool_mbox(g_curwnd, "NOTICE", "The default value cannot be changed.",
								NFTOOL_MB_OK);
			return FALSE;
		}
		
		for(i=0; i<16; i++)
		{
			if(conf_fixed[i] == obj)
				break;
		}

		if (i >= (*g_num_items)) return FALSE;

		nfui_nfobject_get_window_pos(obj, &wx, &wy);
		nfui_nfobject_get_offset(obj, &x, &y);

		x += wx + (obj->width/2);
		y += wy + (obj->height/2);

		if(DISPLAY_ACTIVE_WIDTH < x+CONF_MENU_WIDTH)
			x = DISPLAY_ACTIVE_WIDTH - CONF_MENU_WIDTH;

		ret_val = VW_Create_Seq_SubMenu(g_curwnd, x, y);
		_process_popup_ret_val(ret_val);
		
		return TRUE;
	}

	return FALSE;
}

static gboolean post_add_btn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top;
		guint ret;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
		  return FALSE;
	       }


		if(g_edit_enable == FALSE)
		{
			nftool_mbox(g_curwnd, "NOTICE", "The default value cannot be changed.", NFTOOL_MB_OK);
			return FALSE;
		}

		if(*g_num_items >= 16)
		{
			nftool_mbox_auto(g_curwnd, 1, "NOTICE", "The number of items less than 16\nare available.");
			return FALSE;
		}

		ret = SequenceSetupDlg_Open(g_curwnd, &g_elem_data[*g_num_items], TRUE);
    
		if(ret)
		{
			(*g_num_items)++;
			nfui_nffixed_set_drawing_outline((NFFIXED*)conf_fixed[*g_num_items-1], TRUE);
			nfui_nfobject_use_focus(conf_fixed[*g_num_items-1], NFOBJECT_FOCUS_ON);
			nfui_signal_emit(conf_fixed[*g_num_items-1], GDK_EXPOSE, TRUE);

			top = nfui_nfobject_get_top(conf_fixed[0]);
			nfui_make_key_hierarchy((NFWINDOW*)top);
		}
	}

	return FALSE;
}
	
static gboolean post_delete_all_btn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top;
		guint i;
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
		{					
			return FALSE;
		}

		for (i=0; i<16; i++)
		{
			cur_focus = i;
			_process_popup_ret_val(1);	
		}

		cur_focus = -1;
	}

	return FALSE;
}

static gboolean post_close_btn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		top = nfui_nfobject_get_top(obj);

		nfui_nfobject_destroy(top);
	}

	return FALSE;
}


static gboolean post_win_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	if (evt->type == GDK_DELETE) {
		g_curwnd = 0;
		gtk_main_quit();
	}
	return FALSE;
}
	
void SequenceConf_Open(NFWINDOW *parent, SeqElementData *elem_data, guint *num_items, gboolean edit_enable)
{
	NFOBJECT *main_win, *main_fixed;
	NFOBJECT *add_btn, *del_btn, *close_btn;
	guint i;


	if(DISPLAY_IS_D1)
		g_disp_mode = DISPLAY_MODE_D1;
	else
		g_disp_mode = DISPLAY_MODE_4D1;

	cur_focus = -1;

	g_elem_data = elem_data;
	g_num_items = num_items;
	g_edit_enable = edit_enable;

	main_win = nfui_nfwindow_new(parent, CONF_POS_X, CONF_POS_Y, CONF_WIDTH, CONF_HEIGHT);
	main_fixed = nfui_nffixed_new();
	nfui_nfobject_show(main_fixed);
	nfui_nfwindow_add((NFWINDOW*)main_win, main_fixed);
	nfui_regi_post_event_callback(main_win, post_win_event_handler);
	g_curwnd = main_win;

	for(i=0; i<16; i++)
	{
		conf_fixed[i] = nfui_nffixed_new();
		nfui_nfobject_set_size(conf_fixed[i], CONF_CELL_WIDTH, CONF_CELL_WIDTH);
		if (i < *g_num_items) 	{
			nfui_nfobject_use_focus(conf_fixed[i], NFOBJECT_FOCUS_ON);
			nfui_nffixed_set_drawing_outline((NFFIXED*)conf_fixed[i], TRUE);
		} else				{
			nfui_nfobject_use_focus(conf_fixed[i], NFOBJECT_FOCUS_OFF);
			nfui_nffixed_set_drawing_outline((NFFIXED*)conf_fixed[i], FALSE);
		}
		nfui_nffixed_put((NFFIXED*)main_fixed, conf_fixed[i], 30+200*(i%4), 30+200*(i/4));
		nfui_regi_pre_event_callback(conf_fixed[i], pre_conf_fixed_event_handler);
		nfui_nfobject_show(conf_fixed[i]);
	}

	add_btn = nftool_normal_button_create_type1("ADD VIEW TYPE", 192);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(add_btn), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(add_btn);

	del_btn = nftool_normal_button_create_type1("DELETE ALL", 192);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(add_btn), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(del_btn);

	close_btn = nftool_normal_button_create_type2("CLOSE", 192);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(close_btn), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(close_btn);

	nfui_nffixed_put((NFFIXED*)main_fixed, add_btn, CONF_WIDTH/2 - 96 - 198, CONF_BTN_POS_Y);
	nfui_nffixed_put((NFFIXED*)main_fixed, del_btn, CONF_WIDTH/2 - 96, CONF_BTN_POS_Y);
	nfui_nffixed_put((NFFIXED*)main_fixed, close_btn, CONF_WIDTH/2 + 96 + 6, CONF_BTN_POS_Y);

	nfui_regi_post_event_callback(add_btn, post_add_btn_event_handler);
	nfui_regi_post_event_callback(del_btn, post_delete_all_btn_event_handler);
	nfui_regi_post_event_callback(close_btn, post_close_btn_event_handler);

	nfui_regi_pre_event_callback(main_fixed, pre_main_fixed_event_handler);


	nfui_run_main_event_handler(main_win);

	nfui_nfobject_show(main_win);

	/* set for key navi */
	nfui_make_key_hierarchy((NFWINDOW*)main_win);
	if(*num_items)	
	{
    	cur_focus = 0;
	    nfui_set_key_focus(conf_fixed[cur_focus], TRUE);
    }
	else 
	    nfui_set_key_focus(add_btn, TRUE);

	nfui_page_open(PGID_SEQ_CONF, main_win, nfui_get_last_user());

	gtk_main();

	nfui_page_close(PGID_SEQ_CONF, main_win);
}


