#include "nf_afx.h"

#include "nf_ptz.h"
#include "nf_api_live.h"

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
#include "objects/nftable.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"
#include "objects/nfcombobox.h"
#include "objects/nfcheckbutton.h"
#include "objects/nflistbox.h"

#include "uxm.h"
#include "scm.h"
#include "ssm.h"
#include "smt.h"
#include "iux_msg.h"

#include "vw_system_fw_up.h"
#include "vw_progress_fwupdate_128MB.h"
#include "vw_vendor_code.h"


#define KANJI_FILE             "jp_kanji_utf8.txt"

#define JP_KJ_WIN_SIZE_W					(446)
#define JP_KJ_WIN_SIZE_H					(448)

#define JP_KJ_WIN_SIZE_X					((DISPLAY_ACTIVE_WIDTH - JP_KJ_WIN_SIZE_W)/2)
#define JP_KJ_WIN_SIZE_Y					((DISPLAY_ACTIVE_HEIGHT - JP_KJ_WIN_SIZE_H)/2)

#define JP_KJ_TITLE_H						(36)

#define JP_KJ_LABEL_H						(40)

// <---- DEVICE
#define JP_KJ_DEV_LABEL_X					(26)
#define JP_KJ_DEV_LABEL_Y					(64)
#define JP_KJ_DEV_LABEL_W					(156)

#define JP_KJ_DEV_CELL_X					(JP_KJ_DEV_LABEL_X + JP_KJ_DEV_LABEL_W + 10)
#define JP_KJ_DEV_CELL_Y					(JP_KJ_DEV_LABEL_Y)
#define JP_KJ_DEV_CELL_W					(427)

// <---- FILE NAME
#define JP_KJ_FILE_LIST_NUM					(7)
#define JP_KJ_FILE_LIST_H					(41)

#define JP_KJ_FILE_LABEL_X					(JP_KJ_DEV_LABEL_X)
#define JP_KJ_FILE_LABEL_Y					(JP_KJ_DEV_LABEL_Y + 3)
#define JP_KJ_FILE_LABEL_W					(JP_KJ_DEV_LABEL_W)

#define JP_KJ_FILE_CELL_X					(10) //(JP_KJ_DEV_CELL_X)
#define JP_KJ_FILE_CELL_Y					(JP_KJ_FILE_LABEL_Y)
#define JP_KJ_FILE_CELL_W					(JP_KJ_DEV_CELL_W)
#define JP_KJ_FILE_CELL_H					(JP_KJ_FILE_LIST_NUM*JP_KJ_FILE_LIST_H)

// <---- UPGRADE, CLOSE BUTTON
#define JP_KJ_LOAD_BTN_X					(45)
#define JP_KJ_LOAD_BTN_Y					(JP_KJ_WIN_SIZE_H - 74)
#define JP_KJ_LOAD_BTN_W					(174)

#define JP_KJ_CLOSE_BTN_X					(JP_KJ_LOAD_BTN_X + JP_KJ_LOAD_BTN_W + 6)
#define JP_KJ_CLOSE_BTN_Y					(JP_KJ_LOAD_BTN_Y)
#define JP_KJ_CLOSE_BTN_W					(174)



static NFWINDOW *g_curwnd = 0;
static NFOBJECT *kanji_list_obj = NULL;
static NFOBJECT *upgrade_obj = NULL;
static NFOBJECT *wait_pop = NULL;
static NFOBJECT *jp_win;


static gchar g_find[256];
static char jp[30000][32];
static char kj[30000][32];
static int g_cnt = 0;

char kanji[256];

static void _update_kanji_list()
{
//	if (nfui_nfobject_is_shown(kanji_list_obj) == FALSE)	return;

	nfui_listbox_delete_all(NF_LISTBOX(kanji_list_obj));

	printf("SKSHIN] update...%d\n", g_cnt);
	int i = 0;
	for (i = 0; i < g_cnt; ++i) {
		printf("SKSHIN] add text...(%s)\n", kj[i]);
		nfui_listbox_set_text_single_column(NF_LISTBOX(kanji_list_obj), kj[i]);
	}
		
	nfui_signal_emit(kanji_list_obj, GDK_EXPOSE, TRUE);
}


static void _update_info_dev_changed(void)
{
	_update_kanji_list();
}

static gint _get_focused_kanji()
{
    if (nfui_nfobject_is_shown(kanji_list_obj) == FALSE) return -1;

    gchar *tmp = nfui_listbox_get_focus_text((NFLISTBOX*)kanji_list_obj, 0);
	if (tmp) {
	    strcpy(kanji, nfui_listbox_get_focus_text((NFLISTBOX*)kanji_list_obj, 0));
	}


    return 0;
}

static gboolean post_okbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *topwin;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == 3)			// mouse right button
			return FALSE;

		gint file_idx = 0;
		gint pos_x, pos_y, width;
		gboolean ret;

		file_idx = nfui_listbox_get_focus_idx((NFLISTBOX*)kanji_list_obj);

		{
			_get_focused_kanji();

			topwin = nfui_nfobject_get_top(obj);
			nftool_destroy_setup_window(topwin);
		}
	}

	return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *topwin;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		topwin = nfui_nfobject_get_top(obj);
		nftool_destroy_setup_window(topwin);
	}

	return FALSE;
}

static gboolean pre_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	int result;

	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	switch(event->type) {
		case GDK_EXPOSE:
		{
    		drawable = nfui_nfobject_get_window(obj);
    		gc = nfui_nfobject_get_gc(obj);

            nfui_nfobject_get_size(obj, &size_w, &size_h);
            pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
            nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
        
    		nfui_nfobject_gc_unref(gc);
    	}
		break;

		case GDK_BUTTON_RELEASE:
		break;					

		case GDK_DELETE:
		{
            nfui_nfobject_get_size(obj, &size_w, &size_h);
            nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
		
		}
		break;
		default:
		break;
	}

	return FALSE;
}

static gboolean pre_mainWin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

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

void VW_JP_Kanji_Open(NFWINDOW *parent, gchar *find, gchar *buf)
{
	NFOBJECT *main_fixed;
	NFOBJECT *obj;

	guint lc_size[] = {JP_KJ_FILE_CELL_W, };
	guint li_size_w, li_size_h;
	guint i;

	gchar *ret = 0;
	memset(kanji, 0x00, sizeof(kanji));
	strcpy(g_find, find);
{
	FILE *fd;
	char buf_jp[32];
	char buf_kj[32];
	char *pp;
	char buf[256];
	int i=0;
	int j = 0;

	fd = fopen("/NFDVR/data/lang/jp_kanji_utf8.txt", "r");
	if (fd == 0) return -1;
//	printf("SKSHIN] fd = %x\n", fd);
	while (!feof(fd)) {
		pp = fgets(buf, 256, fd);
		if (!pp) break;
		sscanf(pp, "%[^,],%s", buf_jp, buf_kj);
		//sscanf(pp, "%s %s", buf_jp, buf_kj);
		//printf("[%d][%d], %s_%s\n", i, j, g_find, buf_jp);
		if (strcmp(g_find, buf_jp) == 0) {
			strcpy(jp[i], buf_jp);
			strcpy(kj[i], buf_kj);
		
			++i;
		}

		++j;
	}
	g_cnt = i;
	printf("SKSHIN] g_cnt = %d\n", g_cnt);
	fclose(fd);

}

// <---- WINDOW
	jp_win = (NFOBJECT*)nfui_nfwindow_new(parent, JP_KJ_WIN_SIZE_X, JP_KJ_WIN_SIZE_Y, JP_KJ_WIN_SIZE_W, JP_KJ_WIN_SIZE_H);
	g_curwnd = jp_win;
	nfui_nfobject_modify_bg(jp_win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_regi_post_event_callback(jp_win, post_main_win_event_handler);

// <---- FIXED
	main_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(main_fixed, JP_KJ_WIN_SIZE_W, JP_KJ_WIN_SIZE_H);
	nfui_regi_pre_event_callback(main_fixed, pre_main_fixed_event_handler);
	nfui_nfobject_show(main_fixed);

// <---- TITLE
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("KANJI LIST", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, 440, JP_KJ_TITLE_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 10, 4);

// <---- KANJI LIST
/*
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("LIST", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, JP_KJ_FILE_LABEL_W, JP_KJ_LABEL_H);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, JP_KJ_FILE_LABEL_X, JP_KJ_FILE_LABEL_Y);
	*/
	
	nfui_get_image_size(IMG_N_SCROLL_UP, &li_size_w, &li_size_h);	
    lc_size[0] -= li_size_w;
	
	kanji_list_obj = nfui_listbox_new(1, lc_size, 40);	
	nfui_listbox_set_skin_type(NF_LISTBOX(kanji_list_obj), NFLISTBOX_TYPE_POPUP_1);
	nfui_listbox_set_pango_font(NF_LISTBOX(kanji_list_obj), nffont_get_pango_font(NFFONT_SMALL_SEMI));
    nfui_listbox_support_multi_lang(NF_LISTBOX(kanji_list_obj), FALSE);
	nfui_nfobject_set_size(kanji_list_obj, JP_KJ_FILE_CELL_W, JP_KJ_FILE_CELL_H);
	nfui_nfobject_modify_bg(kanji_list_obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
	nfui_nfobject_use_focus(kanji_list_obj, NFOBJECT_FOCUS_ON);
	nfui_nfobject_show(kanji_list_obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, kanji_list_obj, JP_KJ_FILE_CELL_X, JP_KJ_FILE_CELL_Y);


// <---- OK BUTTON
	upgrade_obj = nftool_normal_button_create_type1("OK", JP_KJ_LOAD_BTN_W);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(upgrade_obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(upgrade_obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, upgrade_obj, JP_KJ_LOAD_BTN_X, JP_KJ_LOAD_BTN_Y);
	nfui_regi_post_event_callback(upgrade_obj, post_okbutton_event_handler);

// <---- CLOSE BUTTON
	obj = nftool_normal_button_create_type1("CLOSE", JP_KJ_CLOSE_BTN_W);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, JP_KJ_CLOSE_BTN_X, JP_KJ_CLOSE_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

	
	nfui_regi_pre_event_callback(jp_win, pre_mainWin_event_handler);
	nfui_nfwindow_add((NFWINDOW*)jp_win, main_fixed);
	nfui_run_main_event_handler(jp_win);
	nfui_nfobject_show(jp_win);

	_update_info_dev_changed();
	
	/* set for key navi */
	nfui_make_key_hierarchy((NFWINDOW*)jp_win);
	nfui_set_key_focus(upgrade_obj, TRUE);

	nfui_page_close(PGID_POPUPWND, jp_win);
	nfui_page_open(PGID_SYS_FWUP, jp_win, NULL);


	gtk_main();

	nfui_page_close(PGID_SYS_FWUP, jp_win);

	strcpy(buf, kanji);
	return ;
}




