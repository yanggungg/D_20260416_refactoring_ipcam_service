#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_function.h"
#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"
#include "objects/nflistbox.h"
#include "objects/nfimage.h"

#include "uxm.h"
#include "scm.h"
#include "ssm.h"
#include "smt.h"
#include "ix_mem.h"
#include "iux_msg.h"

#include "vw_sys_main.h"
#include "vw_vkeyboard.h"
#include "vw_supported_ipcam.h"



#define LIST_PATH			"./src/nfgui/ipcam-list.txt"
#define MODEL_STRING_SIZE   128



static NFWINDOW *g_curwnd = 0;

static NFOBJECT *g_lbox;
static NFOBJECT *g_searchlb;

static gchar **g_strModel;




static gboolean read_list()
{
	FILE *fp = NULL;
	char s[8];
	char *sp;
	int cnt;
	int i;

	fp = fopen(LIST_PATH, "r");
	if(fp == NULL) {
		g_warning("%s : fopen returns NULL", __FUNCTION__);
		return FALSE;
	}

	// list count
	if(fgets(s, 7, fp)) {
		cnt = atoi(s);

		g_strModel = (gchar**)imalloc(sizeof(gchar*)*((guint)cnt));
		for(i=0; i<cnt; i++) {
			g_strModel[i] = (gchar*)imalloc(sizeof(gchar) * MODEL_STRING_SIZE);
		}
	}

	// get list
	i = 0;
	while(!feof(fp)) {
		if(fgets(g_strModel[i], MODEL_STRING_SIZE-1, fp)) {
			sp = strchr(g_strModel[i], '\n');
			strcpy(sp, "\0");

			i++;
		}
	}

	fclose(fp);
	return TRUE;
}

static gboolean show_all_list(gboolean expose)
{
	gint i=0;
	
	nfui_listbox_delete_all(NF_LISTBOX(g_lbox));

	while(g_strModel[i]) {
		nfui_listbox_set_text((NFLISTBOX*)g_lbox, &(g_strModel[i]));
		i++;
	}

	if(expose)
		nfui_signal_emit(g_lbox, GDK_EXPOSE, TRUE);

	return TRUE;
}

static gboolean show_search_list(gchar *str, gboolean expose)
{
	gint i=0;
	gchar *msg[] = {"NO MATCH LIST.", };


	nfui_listbox_delete_all(NF_LISTBOX(g_lbox));

	while(g_strModel[i]) {
		if(strstr(g_strModel[i], str))
			nfui_listbox_set_text((NFLISTBOX*)g_lbox, &(g_strModel[i]));

		i++;
	}

	if(expose)
		nfui_signal_emit(g_lbox, GDK_EXPOSE, TRUE);

	return TRUE;
}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		gint i=0;

		g_curwnd = 0;

		if (g_strModel) {
			while(g_strModel[i]) {
				ifree(g_strModel[i++]);
			}
			ifree(g_strModel);
			g_strModel = NULL;
		}
	}

	return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;				
			
		VW_SetupSystem_Destroy(obj);
	}

	return FALSE;
}

static gboolean post_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;
	gchar *memo = NULL;

	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;
	guint x, y;

	if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if (evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{
		if(kpid == KEYPAD_ENTER)
		{
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);

			x += (obj->width)/2 + top->x;
			y += obj->height + top->y;
		}
		else
		{
			if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += ((GdkEventButton*)evt)->x;
			y += ((GdkEventButton*)evt)->y;
		}

		memo = VirtualKey_Open(g_curwnd, "", x, y, 32, VKEY_NORMAL);

		if(memo) {
			nfui_nflabel_set_text((NFLABEL*)obj, memo);
			nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

			ifree(memo);
			memo = NULL;
		}

	}

	return FALSE;
}

static gboolean post_search_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		gchar *str;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;				
			
		str = nfui_nflabel_get_text((NFLABEL*)g_searchlb);
		if(strlen(str))
			show_search_list(str, TRUE);
	}

	return FALSE;
}

static gboolean post_all_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;				

		nfui_nflabel_set_text((NFLABEL*)g_searchlb, "");
		nfui_signal_emit(g_searchlb, GDK_EXPOSE, FALSE);

		show_all_list(TRUE);
	}

	return FALSE;
}

void VW_Init_SysSupporteIPCam_page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *sch_btn, *all_btn;
	NFOBJECT *obj;

	guint lc_size[] = {800, };
	guint li_size_w, li_size_h;



	g_curwnd = nfui_nfobject_get_top(parent);
	nfui_regi_post_event_callback(parent, post_page_event_handler);

	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(content_fixed, MENU_V_INNER_W, MENU_V_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);	


	// TITLE 
	obj = nfui_nfimage_new(IMG_TITLE_BG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "SUPPORTED IPCAM LIST");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, 8, 0);



	// MODEL LABEL
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("IPCAM MODEL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, 220, 40);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, 28, 61);


	obj= nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
	nfui_nflabel_set_align(obj, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, 250, 40);
	nfui_nfobject_show(obj);
	nfui_regi_post_event_callback(obj, post_label_event_handler);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, 248, 61);
	g_searchlb = obj;

	obj = nftool_normal_button_create_subtab_type1("SEARCH", 100);
	nfui_regi_post_event_callback(obj, post_search_button_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, 518, 61);
	sch_btn = obj;

	obj = nftool_normal_button_create_subtab_type1("ALL", 100);
	nfui_regi_post_event_callback(obj, post_all_button_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, 628, 61);
	all_btn = obj;


	// LIST-BOX
	nfui_get_image_size(IMG_N_SCROLL_UP, &li_size_w, &li_size_h);
    lc_size[0] -= li_size_w;

	obj = nfui_listbox_new(1, lc_size, 40);	
	nfui_listbox_set_skin_type(NF_LISTBOX(obj), NFLISTBOX_TYPE_1);
	nfui_listbox_set_pango_font(NF_LISTBOX(obj), nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_listbox_support_multi_lang(NF_LISTBOX(obj), FALSE);
	nfui_nfobject_set_size(obj, 800, 720);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_ON);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, 28, 140);
	g_lbox = obj;



// <---- CANCEL, APPLY, CLOSE BUTTON
	obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R3_X, MENU_V_BTN_Y);
	nfui_nfobject_disable(obj);

	obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R2_X, MENU_V_BTN_Y);
	nfui_nfobject_disable(obj);

	obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R1_X, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

	// read file
	if(read_list())
	{
		show_all_list(FALSE);
	}
	else
	{
		nfui_nfobject_disable(g_searchlb);
		nfui_nfobject_disable(g_lbox);
		nfui_nfobject_disable(sch_btn);
		nfui_nfobject_disable(all_btn);
	}
}

gboolean VW_SupportedIPCam_tab_out_handler(void)
{
	return FALSE;
}
