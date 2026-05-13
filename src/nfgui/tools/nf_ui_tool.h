#ifndef	__NF_UI_TOOL_H__
#define	__NF_UI_TOOL_H__

/******************************************************************
 *
 *	Setup Window Make/Decorate Tool.
 *
 *
 ******************************************************************/

#include "../viewers/objects/nfobject.h"
#include "../viewers/objects/nfwindow.h"
#include "../support/util.h"

typedef enum {
	NFTOOL_MB_NONE 		= 0,
	NFTOOL_MB_OK		= 1,
	NFTOOL_MB_YES		= 1,
	NFTOOL_MB_CANCEL	= 2,
	NFTOOL_MB_NO		= 2,

	NFTOOL_MB_OKCANCEL,
	NFTOOL_MB_YESNO,
	NFTOOL_MB_CONF_1,
	NFTOOL_MB_CONF_2,
	NFTOOL_MB_AUTO_OK,
	NFTOOL_MB_AUTO_OKCANCEL,
	NFTOOL_MB_ERASECANCEL,
	NFTOOL_MB_SLEEP_AUTO
}mb_type;

typedef enum {
	NFSETUP_WINDOW_CAMERA = 0,
	NFSETUP_WINDOW_DISPLAY,
	NFSETUP_WINDOW_SOUND,
	NFSETUP_WINDOW_USER,
	NFSETUP_WINDOW_NETWORK,
	NFSETUP_WINDOW_SYSTEM,
	NFSETUP_WINDOW_DISK,
	NFSETUP_WINDOW_EVENT,

	NFSETUP_WINDOW_RECORDING,
	NFSETUP_WINDOW_ARCHIVING,
	NFSETUP_WINDOW_SEARCH,

	NFSETUP_WINDOW_DISK_DELETE,
	NFSETUP_WINDOW_USER_GUIDE,

	NFSETUP_WINDOW_END
} nfsetup_window_type;

typedef enum {
	NF_CURSOR_ARROW = 0,
	NF_CURSOR_FLEUR,
#ifdef _CURSOR_DISPLAY_ONOFF //nskim display on/off cursor
	NF_CURSOR_NULL,
#endif		
	NF_CURSOR_COUNTS	
}nfcursor_type;


typedef struct _MBOXCONF {
	gchar *title;
	gchar **contents;
	guint lines;
	gchar **buttonStr;
	gboolean add_close;
	nfalign_type align;
}MBOXCONF;

typedef struct _NFCURSORMENU
{
	NFOBJECT *parent;
	gint x;
	gint y;
	gint w;
	gint h;
	gint page;
	void (*func)(NFOBJECT *, gint);
}NFCURSORMENU, *PNFCURSORMENU; // otm


////////////////////////////////////////////////////////////
//
//
//

int nftool_create_type1_btn_image(int size);
int nftool_create_type2_btn_image(int size);
int nftool_create_type3_btn_image(int size);
int nftool_create_popup_type1_btn_image(int size);
int nftool_create_popup_type2_btn_image(int size);
int nftool_create_subtab_type1_btn_image(int size);

NFOBJECT* nftool_create_setup_window(NFWINDOW *parent, nfsetup_window_type type, gint page);

//NFOBJECT* nftool_create_top_tab_setup_window(NFWINDOW *parent, nfsetup_window_type type, gint page);
void nftool_destroy_setup_window(NFOBJECT *setup_wnd);
NFOBJECT* nftool_get_main_fixed_from_window(NFWINDOW *wnd);
NFOBJECT* nftool_get_main_fixed_from_setup_window(NFOBJECT *setup_wnd);
NFOBJECT* nftool_get_nftab_from_setup_window(NFOBJECT *setup_wnd);
NFOBJECT* nftool_get_nftab_from_top_tab_setup_window(NFOBJECT *setup_wnd, nfsetup_window_type type);
NFOBJECT* nftool_resize_popup_window(NFOBJECT *nfwin, int w, int h);
NFOBJECT* nftool_create_popup_window(NFWINDOW *parent, guint x, guint y, guint width, guint height, const gchar *strTitle, gboolean close);

NFOBJECT* nftool_mbox_auto(NFWINDOW *parent, gint sec, const gchar *strTitle, const gchar *strCont1);
NFOBJECT* nftool_mbox_sleep_auto(NFWINDOW *parent, gint sec, const gchar *strTitle, const gchar *strCont1);
NFOBJECT* nftool_mbox_auto_ok(NFWINDOW *parent, gint sec, const gchar *strTitle, const gchar *strCont1);
mb_type nftool_mbox_auto_okcancel(NFWINDOW *parent, gint sec, const gchar *strTitle, const gchar *strCont1);
mb_type nftool_mbox2_auto_okcancel(NFWINDOW *parent, gint sec, const gchar *strTitle, const gchar *strCont1, const gchar *strCont2, const gchar *strCont3, const gchar *strCont4);
NFOBJECT* nftool_mbox_wait(NFWINDOW *parent, const gchar *strTitle, const gchar *strCont1);
NFOBJECT* nftool_mbox_wait_with_graph(NFWINDOW *parent, const gchar *strTitle, const gchar *strCont1, const gchar *strCont2);
void nftool_remove_waitbox(NFOBJECT *wait_box);
mb_type nftool_mbox(NFWINDOW *parent, const gchar *strTitle, const gchar *strContent, mb_type type);
mb_type nftool_mbox_4_line(NFWINDOW *parent, const gchar *strTitle, const gchar *strContent1, const gchar *strContent2, const gchar *strContent3, const gchar *strContent4, mb_type type);

gint nftool_mbox_by_conf(NFWINDOW *parent, MBOXCONF *mbConf, mb_type type);




NFOBJECT* nftool_prog_pop_open(NFWINDOW *parent, const gchar *strTitle, gboolean show_num, guint x, guint y, guint width);
void nftool_prog_pop_set_rate(NFOBJECT* prog_pop, guint rate);
void nftool_prog_pop_close(NFOBJECT* prog_pop);

#if 0 //_ATM_MOUSE_
void nftool_set_custom_cursor(GdkWindow *wnd,  gboolean mouse_connect_flag);
#else
void nftool_set_custom_cursor(GdkWindow *wnd);
#endif
void nftool_change_custom_cursor(GdkWindow *wnd, nfcursor_type type);

/**************************************************************
 * TO SUPPORT TOOLTIP..
 * ***********************************************************/
void nftool_tooltip_init();
void nftool_tooltip_show(NFOBJECT *obj, const gchar *str);
void nftool_tooltip_show_with_pos(NFOBJECT *obj, const gchar *str, gint win_x, gint win_y);
void nftool_tooltip_show_with_offset(NFOBJECT *obj, const gchar *str, gint off_x, gint off_y);
void nftool_tooltip_hide();
void nftool_tooltip_set_mouse_position(guint x, guint y);
void nftool_tooltip_get_mouse_position(guint *x, guint *y);
void nftool_tooltip_multi_lang_support(gboolean is_support);

/**************************************************************
 * USEFUL FUNCTIONS..
 * ***********************************************************/

#define	SETUP_WINDOW_POS_X				(0)
#define	SETUP_WINDOW_POS_Y				(0)
#define	SETUP_WINDOW_WIDTH				(DISPLAY_ACTIVE_WIDTH)
#define	SETUP_WINDOW_HEIGHT				(DISPLAY_ACTIVE_HEIGHT)

#define	NORMAL_BTN_WIDTH	(DISPLAY_IS_D1 ? 80:158)
#define	NORMAL_BTN_HEIGHT	(DISPLAY_IS_D1 ? 16:30)

#define	MENU_V_PAGE_X					(12+344)
#define	MENU_V_PAGE_Y					(77)
#define	MENU_V_PAGE_W					(20+1512+20)
#define	MENU_V_PAGE_H					(20+903+20+60)

#define MENU_V_INNER_X					(20)
#define MENU_V_INNER_Y					(20)
#define MENU_V_INNER_W					(1512)
#define MENU_V_INNER_H					(903)

#define	MENU_H_PAGE_X					(12)
#define	MENU_H_PAGE_Y					(77)
#define	MENU_H_PAGE_W					(20+1856+20)
#define	MENU_H_PAGE_H					(20+903+20+60)

#define MENU_H_INNER_X					(20)
#define MENU_H_INNER_Y					(20)
#define MENU_H_INNER_W					(1856)
#define MENU_H_INNER_H					(903)

#define	MENU_V_SUBTAB_FIXED_X			(12+344)
#define	MENU_V_SUBTAB_FIXED_Y			(77)
#define	MENU_V_SUBTAB_FIXED_W			(6+18+1505+18+5)
#define	MENU_V_SUBTAB_FIXED_H			(5+40+18+857+18+5+60)

#define	MENU_V_SUBTAB_PAGE_X			(6)
#define	MENU_V_SUBTAB_PAGE_Y			(5+40)
#define	MENU_V_SUBTAB_PAGE_W			(18+1505+18+5)
#define	MENU_V_SUBTAB_PAGE_H			(18+857+18+5+60)

#define MENU_V_SUBTAB_INNER_X			(18)
#define MENU_V_SUBTAB_INNER_Y			(18)
#define MENU_V_SUBTAB_INNER_W			(1505)
#define MENU_V_SUBTAB_INNER_H			(857)

#define	MENU_V_IPCAMSET_SUBTAB_PAGE_X		(6)
#define	MENU_V_IPCAMSET_SUBTAB_PAGE_Y		(5+40+40)
#define	MENU_V_IPCAMSET_SUBTAB_PAGE_W		(18+1505+18+5)
#define	MENU_V_IPCAMSET_SUBTAB_PAGE_H		(18+817+18+5+60)

#define MENU_V_IPCAMSET_SUBTAB_INNER_X	    (18)
#define MENU_V_IPCAMSET_SUBTAB_INNER_Y	    (18)
#define MENU_V_IPCAMSET_SUBTAB_INNER_W	    (1505)
#define MENU_V_IPCAMSET_SUBTAB_INNER_H	    (817)

#define MENU_BTN_WIDTH					(192)
#define MENU_BTN_HEIGHT					(44)
#define MENU_BTN_GAP					(4)

#define MENU_V_BTN_R_START_X			(MENU_V_PAGE_W - MENU_BTN_WIDTH)
#define MENU_V_BTN_R1_X					(MENU_V_BTN_R_START_X)
#define MENU_V_BTN_R2_X					(MENU_V_BTN_R1_X - MENU_BTN_GAP - MENU_BTN_WIDTH)
#define MENU_V_BTN_R3_X					(MENU_V_BTN_R2_X - MENU_BTN_GAP - MENU_BTN_WIDTH)
#define MENU_V_BTN_R4_X					(MENU_V_BTN_R3_X - MENU_BTN_GAP - MENU_BTN_WIDTH)
#define MENU_V_BTN_Y					(MENU_V_PAGE_H - 10 - MENU_BTN_HEIGHT)

#define MENU_V_SUBTAB_BTN_R_START_X		(MENU_V_SUBTAB_PAGE_W - MENU_BTN_WIDTH)
#define MENU_V_SUBTAB_BTN_R1_X			(MENU_V_SUBTAB_BTN_R_START_X)
#define MENU_V_SUBTAB_BTN_R2_X			(MENU_V_SUBTAB_BTN_R1_X - MENU_BTN_GAP - MENU_BTN_WIDTH)
#define MENU_V_SUBTAB_BTN_R3_X			(MENU_V_SUBTAB_BTN_R2_X - MENU_BTN_GAP - MENU_BTN_WIDTH)
#define MENU_V_SUBTAB_BTN_R4_X			(MENU_V_SUBTAB_BTN_R3_X - MENU_BTN_GAP - MENU_BTN_WIDTH)
#define MENU_V_SUBTAB_BTN_Y				(MENU_V_SUBTAB_PAGE_H - 10 - MENU_BTN_HEIGHT)

#define MENU_V_IPCAMSET_SUBTAB_BTN_R_START_X	(MENU_V_IPCAMSET_SUBTAB_PAGE_W - MENU_BTN_WIDTH)
#define MENU_V_IPCAMSET_SUBTAB_BTN_R1_X			(MENU_V_IPCAMSET_SUBTAB_BTN_R_START_X)
#define MENU_V_IPCAMSET_SUBTAB_BTN_R2_X			(MENU_V_IPCAMSET_SUBTAB_BTN_R1_X - MENU_BTN_GAP - MENU_BTN_WIDTH)
#define MENU_V_IPCAMSET_SUBTAB_BTN_R3_X			(MENU_V_IPCAMSET_SUBTAB_BTN_R2_X - MENU_BTN_GAP - MENU_BTN_WIDTH)
#define MENU_V_IPCAMSET_SUBTAB_BTN_R4_X			(MENU_V_IPCAMSET_SUBTAB_BTN_R3_X - MENU_BTN_GAP - MENU_BTN_WIDTH)
#define MENU_V_IPCAMSET_SUBTAB_BTN_Y			(MENU_V_IPCAMSET_SUBTAB_PAGE_H - 10 - MENU_BTN_HEIGHT)

#define MENU_H_BTN_R_START_X			(MENU_H_PAGE_W - MENU_BTN_WIDTH)
#define MENU_H_BTN_R1_X					(MENU_H_BTN_R_START_X)
#define MENU_H_BTN_R2_X					(MENU_H_BTN_R1_X - MENU_BTN_GAP - MENU_BTN_WIDTH)
#define MENU_H_BTN_R3_X					(MENU_H_BTN_R2_X - MENU_BTN_GAP - MENU_BTN_WIDTH)
#define MENU_H_BTN_R4_X					(MENU_H_BTN_R3_X - MENU_BTN_GAP - MENU_BTN_WIDTH)
#define MENU_H_BTN_Y					(MENU_H_PAGE_H - 10 - MENU_BTN_HEIGHT)

GdkPixbuf* nftool_get_pixbuf(nfobject_state status);
NFOBJECT* nftool_normal_button_new(const gchar *strLabel);



gint nftool_calc_distance(NFOBJECT* obj1, NFOBJECT* obj2);

void nftool_draw_object_border(NFOBJECT *obj, gint col_idx, gint size);
void nftool_draw_border(GdkDrawable *drawable, gint x, gint y, gint w, gint h, gint col_idx, gint size);

void make_channel_string(gchar *strch[GUI_CHANNEL_CNT], gboolean is_space);
void free_channel_string(gchar *strch[GUI_CHANNEL_CNT]);
void make_channel_index_string(gchar *strch[GUI_CHANNEL_CNT]);

void convertIntToIP(guint *out, guint in);
guint convertIPToInt(guint ip[4]);

gboolean nftool_cur_language_is_eng(void);
gboolean nftool_cur_language_is_japanese(void);

#if 0 //_ATM_MOUSE_
gboolean nftool_check_mouse_connect(void);
gboolean nftool_change_mouse_state(gboolean mouse_connect_flag);

void nfui_set_mouse_connected_flag(gboolean mouse_connect_flag);
#endif



NFOBJECT* nftool_normal_button_create_type1(const gchar *strLabel, guint size);
NFOBJECT* nftool_normal_button_create_type2(const gchar *strLabel, guint size);
NFOBJECT* nftool_normal_button_create_type3(const gchar *strLabel, guint size);
NFOBJECT* nftool_normal_button_create_popup_type1(const gchar *strLabel, guint size);
NFOBJECT* nftool_normal_button_create_popup_type2(const gchar *strLabel, guint size);
NFOBJECT* nftool_normal_button_create_subtab_type1(const gchar *strLabel, guint size);

GdkPixbuf *nftool_create_slider_n_image(int size);
GdkPixbuf *nftool_create_slider_d_image(int size);

#endif	// __NF_UI_TOOL_H__

