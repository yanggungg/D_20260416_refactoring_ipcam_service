#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nfspinbutton.h"
#include "objects/nflabel.h"

#include "vw_sys_disp_spot.h"
#include "vw_sys_disp_spot_edit.h"
#include "vw_sys_disp_spot_setup.h"
#include "vw_sys_disp_spot_conf.h"
#include "vw_vkeyboard.h"
#include "ix_mem.h"


#define MAX_STRING_SIZE		16

#define SE_PP_SIZE_WID			(guint)(448)
#define SE_PP_SIZE_HEI			(guint)(260)



#define SE_PP_POS_X				(guint)((DISPLAY_ACTIVE_WIDTH - SE_PP_SIZE_WID)/2)
#define SE_PP_POS_Y				(guint)((DISPLAY_ACTIVE_HEIGHT - SE_PP_SIZE_HEI)/2)

#define SE_PP_FIXED_SIZE_WID	(guint)(424)
#define SE_PP_FIXED_SIZE_HEI	(guint)(200)
#define SE_PP_FIXED_POS_X		(guint)(12)
#define SE_PP_FIXED_POS_Y		(guint)(48)

enum {
	SEB_MODIFY = 0,
	SEB_SAVE,
	SEB_CANCEL,
	SEB_BUTTONS
};


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *lbSpotTitle;
static NFOBJECT *active;

static SpotData *g_spot_data;

static SpotElementData g_elem_data[MAX_SPOT_ITEMS_NUM];
static guint g_num_items = 0;
static guint ret_val = SPOT_EDIT_RET_CANCEL;
static gboolean g_split_ch;
static guint g_output_ch;

static gboolean post_main_bg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	gint img_w, img_h;

	if(evt->type == GDK_EXPOSE) 
	{
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);
	
		nfui_nfobject_gc_unref(gc);
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


static gboolean post_spottitle_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;
	GdkEventKey *kevt;
	KEYPAD_KID kpid;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{
		gchar *title;
		guint x, y;

		if(kpid == KEYPAD_ENTER)
		{
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);

			x += (obj->width)/2 + top->x;
			y += obj->height + top->y;

		}
		else
		{
	  	   	if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			  return FALSE;
	  	   	}
		
			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += ((GdkEventButton*)evt)->x;
			y += ((GdkEventButton*)evt)->y;

		}

		title = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text(obj), x, y, MAX_STRING_SIZE, VKEY_NORMAL);

		if(title)
		{
			nfui_nflabel_set_text(obj, title);

			ifree(title);
			title = NULL;
		}

		nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

static gboolean post_modifybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top;
		gchar strTemp[32];

		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		top = nfui_nfobject_get_top(obj);

		memset(strTemp, 0, sizeof(strTemp));
		strcpy(strTemp, nfui_nflabel_get_text(lbSpotTitle));

		if(!strcmp(strTemp, ""))
		{
			nftool_mbox(g_curwnd, "NOTICE", "Please input a name.", NFTOOL_MB_OK);
			return FALSE;
		}

		strcpy(g_spot_data->title, strTemp);

		nfui_nfobject_hide(top);

		if(nfui_spin_button_get_index(active))
			g_spot_data->valid_mode = SPOT_MODE_VALID;
		else
			g_spot_data->valid_mode = SPOT_MODE_INVALID;

		SpotConf_Open(g_curwnd, g_elem_data, &g_num_items, g_split_ch, g_output_ch, SPOT_OUT);

		g_memmove(g_spot_data->items, g_elem_data, sizeof(SpotElementData)*32);
		g_spot_data->numItems = g_num_items;

		ret_val = SPOT_EDIT_RET_MODIFY;
		nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean post_savebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top;
		gchar strTemp[32];

		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		top = nfui_nfobject_get_top(obj);

		memset(strTemp, 0, sizeof(strTemp));
		strcpy(strTemp, nfui_nflabel_get_text(lbSpotTitle));

		if(!strcmp(strTemp, ""))
		{
			nftool_mbox(g_curwnd, "NOTICE", "Please input a name.", NFTOOL_MB_OK);
			return FALSE;
		}

		strcpy(g_spot_data->title, strTemp);
		
		nfui_nfobject_hide(top);

		if(nfui_spin_button_get_index(active))
			g_spot_data->valid_mode = SPOT_MODE_VALID;
		else
			g_spot_data->valid_mode = SPOT_MODE_INVALID;

		ret_val = SPOT_EDIT_RET_SAVE;
		nfui_nfobject_destroy(top);
	}

	return FALSE;
}
	
static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *topwin;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
		{
			return FALSE;
	    }
		
		ret_val = SPOT_EDIT_RET_CANCEL;

		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);
	}

	return FALSE;
}

guint SpotEditDlg_Open(NFWINDOW *parent, SpotData *spot_data, guint split_ch, guint output_ch)
{
	NFOBJECT *main_wnd, *main_fixed;
	NFOBJECT *fixed1;
	NFOBJECT *lbTemp;
	NFOBJECT *se_btns[SEB_BUTTONS];

	const gchar *strButton[] = {"MODIFY", "SAVE", "CANCEL"};
	const gchar *strTitle[] = {"SPOT TITLE", "ACTIVATION"};
	const gchar *strOffOn[] = {"OFF", "ON"};

	guint pos_x, pos_y;
	guint i;

	ret_val = SPOT_EDIT_RET_CANCEL;
	g_spot_data = spot_data;

	g_memmove(g_elem_data, spot_data->items, sizeof(SpotElementData) * MAX_SPOT_ITEMS_NUM);
	g_num_items = spot_data->numItems;
	g_split_ch = split_ch;
	g_output_ch = output_ch;


	main_wnd = nftool_create_popup_window(parent, SE_PP_POS_X, SE_PP_POS_Y, SE_PP_SIZE_WID, SE_PP_SIZE_HEI, "EDIT", FALSE);
	nfui_regi_post_event_callback(main_wnd, post_win_event_handler);
	g_curwnd = main_wnd;

	main_fixed = ((NFWINDOW*)main_wnd)->child;

	fixed1 = nfui_nffixed_new();
	nfui_nfobject_set_size(fixed1, SE_PP_FIXED_SIZE_WID, SE_PP_FIXED_SIZE_HEI);
	nfui_nffixed_put((NFFIXED*)main_fixed, fixed1, SE_PP_FIXED_POS_X, SE_PP_FIXED_POS_Y);
	nfui_nfobject_show(fixed1);

	pos_x = 8;
	pos_y = 8;

	for(i = 0; i < 2; i++)
	{
		lbTemp = nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));		
		nfui_nflabel_set_align((NFLABEL*)lbTemp, NFALIGN_LEFT, 0);
		nfui_nflabel_set_spacing((NFLABEL *)lbTemp, SEMI_CONDENSED_SPACING);		
		nfui_nfobject_set_size(lbTemp, 230, 40);
		nfui_nffixed_put((NFFIXED*)fixed1, lbTemp, pos_x, pos_y);
		nfui_nfobject_use_focus(lbTemp, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(lbTemp);
		pos_y += 42;
	}
	
	pos_x = 238;
	pos_y = 8;

	lbSpotTitle = nfui_nflabel_new_text_box(g_spot_data->title, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL *)lbSpotTitle, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nflabel_set_spacing((NFLABEL *)lbSpotTitle, CONDENSED_SPACING);
	nfui_nflabel_set_align(lbSpotTitle, NFALIGN_CENTER, 6);
	nfui_nfobject_support_multi_lang((NFOBJECT*)lbSpotTitle, FALSE);
	nfui_nfobject_set_size(lbSpotTitle, 184, 40);
	nfui_nffixed_put((NFFIXED*)fixed1, lbSpotTitle, pos_x, pos_y);
	nfui_nfobject_show(lbSpotTitle);
	nfui_regi_post_event_callback(lbSpotTitle, post_spottitle_event_handler);

	pos_y += 42;

	if(g_spot_data->valid_mode == SPOT_MODE_VALID)
		active = nfui_spinbutton_new(strOffOn, 2, 1);
	else	
		active = nfui_spinbutton_new(strOffOn, 2, 0);
	nfui_spinbutton_set_skin_type((NFSPINBUTTON*)active, NFSPINBUTTON_TYPE_POPUP_1);
	nfui_nfobject_set_size(active, 184, 40);
	nfui_nffixed_put((NFFIXED*)fixed1, active, pos_x, pos_y);
	nfui_nfobject_show(active);

	for(i = 0; i < SEB_BUTTONS; i++)
	{
		se_btns[i] = nftool_normal_button_create_type1(strButton[i], 160);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(se_btns[i]), NFALIGN_CENTER, 0);	
		nfui_nfobject_show(se_btns[i]);
	}

	nfui_nffixed_put((NFFIXED*)fixed1, se_btns[SEB_MODIFY], 50, 100);
	nfui_nffixed_put((NFFIXED*)fixed1, se_btns[SEB_SAVE], 50, 146);
	nfui_nffixed_put((NFFIXED*)fixed1, se_btns[SEB_CANCEL], 216, 146);
	
	nfui_regi_post_event_callback(se_btns[SEB_MODIFY], post_modifybutton_event_handler);
	nfui_regi_post_event_callback(se_btns[SEB_SAVE], post_savebutton_event_handler);
	nfui_regi_post_event_callback(se_btns[SEB_CANCEL], post_cancelbutton_event_handler);

	nfui_regi_post_event_callback(main_fixed, post_main_bg_event_handler);

	nfui_nfobject_show(main_wnd);

	/* set for key navi */
	nfui_make_key_hierarchy(main_wnd);
	nfui_set_key_focus(lbSpotTitle, TRUE);

	gtk_main();

	return ret_val;
}
