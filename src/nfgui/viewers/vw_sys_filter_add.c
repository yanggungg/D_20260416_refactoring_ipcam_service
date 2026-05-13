#include "nf_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_font.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"
#include "support/color.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nfspinbutton.h"
#include "objects/nflabel.h"
#include "objects/nfcombobox.h"
#include "objects/nfipeditor.h"

#include "vw_sys_net_ipfilter_internal.h"
#include "vw_ip_editor_popup.h"

#include "ix_mem.h"

#define MAX_ID_STRING_SIZE		10

#define MAX_EMAIL_STRING_SIZE	63

#define FILTER_PP_SIZE_WID		(guint)(DISPLAY_IS_D1 ? 210:631)
#define FILTER_PP_SIZE_HEI		(guint)(DISPLAY_IS_D1 ? 184:255)

#define FILTER_PP_POS_X			(guint)((DISPLAY_ACTIVE_WIDTH- FILTER_PP_SIZE_WID)/2)
#define FILTER_PP_POS_Y 			(guint)((DISPLAY_ACTIVE_HEIGHT - FILTER_PP_SIZE_HEI)/2 - 10)

#define FILTER_FIXED_SIZE_WID		(guint)(DISPLAY_IS_D1 ? 198:588)
#define FILTER_FIXED_SIZE_HEI		(guint)(DISPLAY_IS_D1 ? 149:195)

#define PP_LABEL_SIZE_HEI		(guint)(DISPLAY_IS_D1 ? 16:40)
#define PP_VLABEL_SIZE_WID		(guint)(DISPLAY_IS_D1 ? 96:190)
#define PP_VLABEL_SIZE_HEI		(guint)(DISPLAY_IS_D1 ? 16:40)

#define	SUBJECT_LABEL_LEFT			(28)
#define	SUBJECT_LABEL_TOP			(32)
#define	SUBJECT_LABEL_WIDTH			(280)
#define	SUBJECT_LABEL_HEIGHT		(40)

#define	VALUE_LABEL_WIDTH			(278)


enum {
	UAB_OK = 0,
	UAB_CANCEL,
	UAB_BUTTONS
};


static NFWINDOW *g_curwnd = 0;

static IPFilterListData *org_filterdata;
static IPFilterListData filterdata;

static guint retVal = 0;

static guint typeToIndex(guint type)
{
	guint idx = 0;
	if(type	== TYPE_NET_A)
		idx=0;
	else if(type == TYPE_NET_B)
		idx=1;
	else if(type == TYPE_NET_C)
		idx=2;
	else if(type == TYPE_IP_ADDR)
		idx=3;
	
	return idx;
}

static guint indexToType(guint idx)
{
	guint type;

	switch(idx)
	{
		case 0:
		type = TYPE_NET_A;
		break;
		case 1:
		type = TYPE_NET_B;
		break;
		case 2:
		type = TYPE_NET_C;
		break;
		case 3:
		type = TYPE_IP_ADDR;
		break;
	}
	
	return type;
}

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

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
		gtk_main_quit();
	}
	return FALSE;
}

static gboolean post_type_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_COMBOBOX_CHANGED)
	{
		guint idx;

		idx = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
		filterdata.type = indexToType(idx);

	}
	return FALSE;
}



static gboolean post_ipe_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint result;
	IP_EDITOR_T ip_editor_data = {0, };
	gint x, y;
	gint i;


	if(evt->type == GDK_2BUTTON_PRESS)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;
		
		nfui_nfobject_get_offset(obj, &x, &y);

		nfui_nfipeditor_get_ip((NFIPEDITOR*)obj, ip_editor_data.field);
		result = vw_ip_editor_popup_open(g_curwnd, x+obj->width+4, y, &ip_editor_data);		

		if (result == 0)
		{
			nfui_nfipeditor_set_ip_array((NFIPEDITOR*)obj, ip_editor_data.field);
			for(i=0; i<4; i++)
				filterdata.listAddr[i] = ip_editor_data.field[i];
		}
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_ENTER)
		{
			nfui_nfobject_get_offset(obj, &x, &y);

			nfui_nfipeditor_get_ip((NFIPEDITOR*)obj, ip_editor_data.field);
			result = vw_ip_editor_popup_open(g_curwnd, x+obj->width+4, y, &ip_editor_data);		

			if (result == 0)
			{
				nfui_nfipeditor_set_ip_array((NFIPEDITOR*)obj, ip_editor_data.field);
				for(i=0; i<4; i++)
					filterdata.listAddr[i] = ip_editor_data.field[i];
			}
		}
	}

	return FALSE;
}


static gboolean post_okbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *topwin;
		gint i; 
		
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
		{
			return FALSE;
		}
		
		if(memcmp(org_filterdata, &filterdata, sizeof(IPFilterListData)))
		{
			memcpy(org_filterdata, &filterdata, sizeof(IPFilterListData));
		}

		retVal = 1;
		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);

	}

	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *topwin;
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		retVal = 0;

		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);
	}

	return FALSE;
}

guint FilterAddDlg_Open(NFWINDOW *parent, char *Title, IPFilterListData *filter)
{
	NFOBJECT *main_wnd, *main_fixed;
	NFOBJECT *fixed1;
	NFOBJECT *btns[UAB_BUTTONS];
	NFOBJECT *obj;
	guint pos_x, pos_y;
	guint i;
	guint idx;

	const gchar *strButton[] = {"OK", "CANCEL"};
	const gchar *strTitle[] = {"TYPE", "ADDRESS"};
	const gchar *strType[] = {STR_NET_A, STR_NET_B, STR_NET_C, STR_IP_ADDR};
	retVal =0;

	main_wnd = nftool_create_popup_window(parent, FILTER_PP_POS_X, FILTER_PP_POS_Y, FILTER_PP_SIZE_WID, FILTER_PP_SIZE_HEI, Title, TRUE);

	g_curwnd = main_wnd;
	nfui_regi_post_event_callback(main_wnd, post_main_win_event_handler);

	if(filter->type == TYPE_NONE)
		filter->type = TYPE_NET_A;

	org_filterdata = filter;
	memset(&filterdata, 0, sizeof(IPFilterListData));
	memcpy(&filterdata, org_filterdata, sizeof(IPFilterListData));


	main_fixed = ((NFWINDOW*)main_wnd)->child;

	fixed1 = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed1, FILTER_FIXED_SIZE_WID, FILTER_FIXED_SIZE_HEI);

	nfui_nffixed_put((NFFIXED*)main_fixed, fixed1, (guint)(DISPLAY_IS_D1 ? 6:12), (guint)(DISPLAY_IS_D1 ? 28:48));
	nfui_nfobject_show(fixed1);

	pos_y = SUBJECT_LABEL_TOP;


	// title
	for(i=0; i<2; i++)
	{
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
		nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH, SUBJECT_LABEL_HEIGHT);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed1, obj, SUBJECT_LABEL_LEFT, pos_y);
		
		pos_y += (SUBJECT_LABEL_HEIGHT + 2);
	}
	
	pos_x = SUBJECT_LABEL_LEFT + SUBJECT_LABEL_WIDTH + 3;
	pos_y = SUBJECT_LABEL_TOP;

	// value
	obj = (NFOBJECT*)nfui_combobox_new(strType, 4, 0);
	nfui_combobox_set_skin_type((NFCOMBOBOX*)obj, NFCOMBOBOX_TYPE_POPUP_1);
	nfui_nfobject_set_size(obj, VALUE_LABEL_WIDTH, SUBJECT_LABEL_HEIGHT);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
	nfui_nfobject_show(obj);
	idx = typeToIndex(filterdata.type);
	nfui_combobox_set_index((NFCOMBOBOX*)obj, idx);
	nfui_regi_post_event_callback(obj, post_type_event_handler);


	pos_y += (SUBJECT_LABEL_HEIGHT + 2);
	obj = (NFOBJECT*)nfui_nfipeditor_new_with_ip( filter->listAddr[0],
	                                                   filter->listAddr[1],
	                                                   filter->listAddr[2],
	                                                   filter->listAddr[3]);

	nfui_nfipeditor_set_pango_font(obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(129));
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(128));
	nfui_nfobject_set_size(obj, 278, 40);
	nfui_regi_post_event_callback(obj, post_ipe_event_handler);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
	nfui_nfobject_show(obj);


	for(i=0; i<2; i++)
	{
		btns[i] = nftool_normal_button_create_type1(strButton[i], 152);
		nfui_nfobject_show(btns[i]);
	}
	nfui_nffixed_put((NFFIXED*)fixed1, btns[UAB_OK], 145, 150);
	nfui_nffixed_put((NFFIXED*)fixed1, btns[UAB_CANCEL], 315, 150);

	nfui_regi_post_event_callback(main_fixed, post_main_bg_event_handler);
	nfui_regi_post_event_callback(btns[UAB_OK], post_okbutton_event_handler);
	nfui_regi_post_event_callback(btns[UAB_CANCEL], post_cancelbutton_event_handler);

	nfui_nfobject_show(main_wnd);

	/* set for key navi */
	nfui_make_key_hierarchy(main_wnd);
	nfui_set_key_focus(btns[0], TRUE);

	nfui_page_open(PGID_FILTER_ADD, main_wnd, nfui_get_last_user());

	gtk_main();

	nfui_page_close(PGID_FILTER_ADD, main_wnd);

	g_message("%s, %d, retVal:%d", __FUNCTION__, __LINE__, retVal);

	return retVal;
}


