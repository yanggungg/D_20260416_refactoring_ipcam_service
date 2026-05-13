
#include "nf_common.h"

#include "support/nf_ui_color.h"
#include "support/nf_ui_font.h"
#include "support/event_loop.h"
#include "support/color.h"
#include "support/nf_ui_page_manager.h"

#include "nfwindow.h"
#include "nffixed.h"
#include "nfbutton.h"
#include "nflabel.h"
#include "nfcombobox.h"
#include "nfscrolledfixed.h"
#include "ix_mem.h"
#include "iux_afx.h"


#define DBG_LEVEL		9
#define DBG_MODULE		"NFCOMBO"



#define COMBOBOX_LINE_BORDER					2
#define COMBOBOX_ADDED_SPACE					8
#define COMBOBOX_TEXT_LEFT_MARGIN				4
#define COMBOBOX_TEXT_ALIGN						NFALIGN_LEFT

#define COMBOBOX_SUBMENU_MARGIN					4

#define COMBOBOX_REPEAT_TIME					(300)


typedef enum {
	SCROLL_TOP,
	SCROLL_BOTTOM,
	SCROLL_UP,
	SCROLL_DOWN
}ScrollDir;


static gboolean nfcombobox_event_handler(NFCOMBOBOX *combo, GdkEvent *event, gpointer data);
static gboolean nfcombobox_button_cb(NFOBJECT *button, GdkEvent *event, gpointer data);
static gboolean nfcombobox_label_cb(NFOBJECT *label, GdkEvent *event, gpointer data);
static void nfcombobox_set_gc(NFCOMBOBOX *combo);
static void nfcombobox_draw_bg(NFCOMBOBOX *combo);
static void nfcombobox_set_child_position(NFCOMBOBOX *combo);
static void nfcombobox_draw_outlines(NFOBJECT *obj);
static void nfcombobox_draw_submenu_outline(NFOBJECT *obj);

static gboolean nfcombobox_create_submenu(NFCOMBOBOX *combo);
static gboolean nfcombmbox_submenu_event_cb(NFOBJECT *obj, GdkEvent *event, gpointer data);
static gboolean nfcombmbox_submenu_fixed_event_cb(NFOBJECT *obj, GdkEvent *event, gpointer data);
static gint nfcombobox_get_text_w_size(NFCOMBOBOX *combo);
static gboolean nfcombobox_menu_button_cb(NFOBJECT *button, GdkEvent *event, gpointer data);
static gboolean nfcombobox_get_submenu_position(NFCOMBOBOX *combo, gint *x, gint *y);
static gboolean nfcombobox_get_submenu_size(NFCOMBOBOX *combo, gint *w, gint *h);
static gboolean nfcombobox_get_real_size_n_position(NFCOMBOBOX *combo, gint *x, gint *y, gint *w, gint *h);
static gboolean nfcombobox_scroll_button_cb(NFOBJECT *button, GdkEvent *event, gpointer data);
static void nfcombobox_init_submenu_item(NFCOMBOBOX *combo);
static gboolean nfcombobox_submenu_key_scroll_cb(gpointer data);
static gboolean nfcombobox_submenu_scroll_cb(gpointer data);
static gboolean nfcombobox_scroll_menu_items(NFCOMBOBOX *combo, ScrollDir dir);
static void nfcombobox_create_scroll_bar(NFCOMBOBOX *combo);
static gboolean nfcombobox_draw_scroll_bar(NFCOMBOBOX *combo);
static void nfcombobox_reset_submenu(NFCOMBOBOX *combo);
static void nfcombobox_change_child_position(NFCOMBOBOX *combo);


static void  _set_combobox_type_1(NFCOMBOBOX *combo)
{
	GdkPixbuf *dropdown_img[NFOBJECT_STATE_COUNT];
	gint color[NFOBJECT_STATE_COUNT];	
	gint size_w, size_h;

	nfui_nflabel_set_pango_font((NFLABEL*)(combo->label), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(149));
	color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(149);
	color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(151);
	color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(153);
	color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(157);
    nfui_nflabel_set_fg_color((NFLABEL*)(combo->label), color);
	
	nfui_nfobject_modify_bg((NFOBJECT*)combo, NFOBJECT_STATE_NORMAL, COLOR_IDX(148));		
	nfui_nfobject_modify_bg((NFOBJECT*)combo, NFOBJECT_STATE_DISABLE, COLOR_IDX(156));		
	nfui_nfobject_modify_bg((NFOBJECT*)combo->label, NFOBJECT_STATE_DISABLE, COLOR_IDX(156));		

	color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(155);
	color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(159);
	color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(159);
	color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(157);
	nfui_nfbutton_set_pango_font(((NFBUTTON*)combo->button), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), color);

	color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(154);
	color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(158);
	color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(158);
	color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(156);
	nfui_nfbutton_set_bg_color(((NFBUTTON*)combo->button), color);	

	dropdown_img[0] = nfui_get_image_from_file((IMG_N_DROPDOWN_01), NULL);
	dropdown_img[1] = nfui_get_image_from_file((IMG_O_DROPDOWN_01), NULL);
	dropdown_img[2] = nfui_get_image_from_file((IMG_P_DROPDOWN_01), NULL);
	dropdown_img[3] = nfui_get_image_from_file((IMG_D_DROPDOWN_01), NULL);

	nfui_get_pixbuf_size(dropdown_img[0], &size_w, &size_h);

	nfui_nfbutton_set_image(NF_BUTTON(combo->button), dropdown_img);
	nfui_nfobject_set_size(combo->button, size_w, size_h);

	combo->skin_type = NFCOMBOBOX_TYPE_1;
}

static void  _set_combobox_type_2(NFCOMBOBOX *combo)
{
	GdkPixbuf *dropdown_img[NFOBJECT_STATE_COUNT];
	gint color[NFOBJECT_STATE_COUNT];	
	gint size_w, size_h;

	nfui_nflabel_set_pango_font((NFLABEL*)(combo->label), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(163));
	color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(163);
	color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(165);
	color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(167);
	color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(171);
    nfui_nflabel_set_fg_color((NFLABEL*)(combo->label), color);
	
	nfui_nfobject_modify_bg((NFOBJECT*)combo, NFOBJECT_STATE_NORMAL, COLOR_IDX(162));		
	nfui_nfobject_modify_bg((NFOBJECT*)combo, NFOBJECT_STATE_DISABLE, COLOR_IDX(170));		
	nfui_nfobject_modify_bg((NFOBJECT*)combo->label, NFOBJECT_STATE_DISABLE, COLOR_IDX(170));		

	color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(169);
	color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(173);
	color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(173);
	color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(171);
	nfui_nfbutton_set_pango_font(((NFBUTTON*)combo->button), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), color);

	color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(168);
	color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(172);
	color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(172);
	color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(170);
	nfui_nfbutton_set_bg_color(((NFBUTTON*)combo->button), color);

	dropdown_img[0] = nfui_get_image_from_file((IMG_N_DROPDOWN_02), NULL);
	dropdown_img[1] = nfui_get_image_from_file((IMG_O_DROPDOWN_02), NULL);
	dropdown_img[2] = nfui_get_image_from_file((IMG_P_DROPDOWN_02), NULL);
	dropdown_img[3] = nfui_get_image_from_file((IMG_D_DROPDOWN_02), NULL);

	nfui_get_pixbuf_size(dropdown_img[0], &size_w, &size_h);

	nfui_nfbutton_set_image(NF_BUTTON(combo->button), dropdown_img);
	nfui_nfobject_set_size(combo->button, size_w, size_h);

	combo->skin_type = NFCOMBOBOX_TYPE_2;
}

static void  _set_combobox_type_popup_1(NFCOMBOBOX *combo)
{
	GdkPixbuf *dropdown_img[NFOBJECT_STATE_COUNT];
	gint color[NFOBJECT_STATE_COUNT];	
	gint size_w, size_h;

	nfui_nflabel_set_pango_font((NFLABEL*)(combo->label), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(241));
	color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(241);
	color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(243);
	color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(245);
	color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(249);
    nfui_nflabel_set_fg_color((NFLABEL*)(combo->label), color);
	
	nfui_nfobject_modify_bg((NFOBJECT*)combo, NFOBJECT_STATE_NORMAL, COLOR_IDX(240));		
	nfui_nfobject_modify_bg((NFOBJECT*)combo, NFOBJECT_STATE_DISABLE, COLOR_IDX(248));		
	nfui_nfobject_modify_bg((NFOBJECT*)combo->label, NFOBJECT_STATE_DISABLE, COLOR_IDX(248));		

	color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(247);
	color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(251);
	color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(251);
	color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(249);
	nfui_nfbutton_set_pango_font(((NFBUTTON*)combo->button), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), color);

	color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(246);
	color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(250);
	color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(250);
	color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(248);
	nfui_nfbutton_set_bg_color(((NFBUTTON*)combo->button), color);	

	dropdown_img[0] = nfui_get_image_from_file((IMG_N_POPUP_DROPDOWN_01), NULL);
	dropdown_img[1] = nfui_get_image_from_file((IMG_O_POPUP_DROPDOWN_01), NULL);
	dropdown_img[2] = nfui_get_image_from_file((IMG_P_POPUP_DROPDOWN_01), NULL);
	dropdown_img[3] = nfui_get_image_from_file((IMG_D_POPUP_DROPDOWN_01), NULL);

	nfui_get_pixbuf_size(dropdown_img[0], &size_w, &size_h);

	nfui_nfbutton_set_image(NF_BUTTON(combo->button), dropdown_img);
	nfui_nfobject_set_size(combo->button, size_w, size_h);

	combo->skin_type = NFCOMBOBOX_TYPE_POPUP_1;
}

static void  _set_combobox_type_popup_2(NFCOMBOBOX *combo)
{
	GdkPixbuf *dropdown_img[NFOBJECT_STATE_COUNT];
	gint color[NFOBJECT_STATE_COUNT];	
	gint size_w, size_h;

	nfui_nflabel_set_pango_font((NFLABEL*)(combo->label), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(255));
	color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(255);
	color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(257);
	color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(259);
	color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(263);
    nfui_nflabel_set_fg_color((NFLABEL*)(combo->label), color);
	
	nfui_nfobject_modify_bg((NFOBJECT*)combo, NFOBJECT_STATE_NORMAL, COLOR_IDX(254));		
	nfui_nfobject_modify_bg((NFOBJECT*)combo, NFOBJECT_STATE_DISABLE, COLOR_IDX(262));		
	nfui_nfobject_modify_bg((NFOBJECT*)combo->label, NFOBJECT_STATE_DISABLE, COLOR_IDX(262));		

	color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(261);
	color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(265);
	color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(265);
	color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(263);
	nfui_nfbutton_set_pango_font(((NFBUTTON*)combo->button), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), color);

	color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(260);
	color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(264);
	color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(264);
	color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(262);
	nfui_nfbutton_set_bg_color(((NFBUTTON*)combo->button), color);

	dropdown_img[0] = nfui_get_image_from_file((IMG_N_POPUP_DROPDOWN_02), NULL);
	dropdown_img[1] = nfui_get_image_from_file((IMG_O_POPUP_DROPDOWN_02), NULL);
	dropdown_img[2] = nfui_get_image_from_file((IMG_P_POPUP_DROPDOWN_02), NULL);
	dropdown_img[3] = nfui_get_image_from_file((IMG_D_POPUP_DROPDOWN_02), NULL);

	nfui_get_pixbuf_size(dropdown_img[0], &size_w, &size_h);

	nfui_nfbutton_set_image(NF_BUTTON(combo->button), dropdown_img);
	nfui_nfobject_set_size(combo->button, size_w, size_h);

	combo->skin_type = NFCOMBOBOX_TYPE_POPUP_2;
}

static void  _set_combobox_type_subtab_1(NFCOMBOBOX *combo)
{
	GdkPixbuf *dropdown_img[NFOBJECT_STATE_COUNT];
	gint color[NFOBJECT_STATE_COUNT];	
	gint size_w, size_h;

	nfui_nflabel_set_pango_font((NFLABEL*)(combo->label), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(931));
	color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(931);
	color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(933);
	color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(935);
	color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(939);
    nfui_nflabel_set_fg_color((NFLABEL*)(combo->label), color);
	
	nfui_nfobject_modify_bg((NFOBJECT*)combo, NFOBJECT_STATE_NORMAL, COLOR_IDX(930));		
	nfui_nfobject_modify_bg((NFOBJECT*)combo, NFOBJECT_STATE_DISABLE, COLOR_IDX(938));		
	nfui_nfobject_modify_bg((NFOBJECT*)combo->label, NFOBJECT_STATE_DISABLE, COLOR_IDX(938));		

	color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(937);
	color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(941);
	color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(941);
	color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(939);
	nfui_nfbutton_set_pango_font(((NFBUTTON*)combo->button), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), color);

	color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(936);
	color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(940);
	color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(940);
	color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(938);
	nfui_nfbutton_set_bg_color(((NFBUTTON*)combo->button), color);	

	dropdown_img[0] = nfui_get_image_from_file((IMG_N_SUBTAB_DROPDOWN_01), NULL);
	dropdown_img[1] = nfui_get_image_from_file((IMG_O_SUBTAB_DROPDOWN_01), NULL);
	dropdown_img[2] = nfui_get_image_from_file((IMG_P_SUBTAB_DROPDOWN_01), NULL);
	dropdown_img[3] = nfui_get_image_from_file((IMG_D_SUBTAB_DROPDOWN_01), NULL);

	nfui_get_pixbuf_size(dropdown_img[0], &size_w, &size_h);

	nfui_nfbutton_set_image(NF_BUTTON(combo->button), dropdown_img);
	nfui_nfobject_set_size(combo->button, size_w, size_h);

	combo->skin_type = NFCOMBOBOX_TYPE_SUBTAB_1;
}

static void  _set_combobox_type_subtab_2(NFCOMBOBOX *combo)
{
	GdkPixbuf *dropdown_img[NFOBJECT_STATE_COUNT];
	gint color[NFOBJECT_STATE_COUNT];	
	gint size_w, size_h;

	nfui_nflabel_set_pango_font((NFLABEL*)(combo->label), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(945));
	color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(945);
	color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(947);
	color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(949);
	color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(953);
    nfui_nflabel_set_fg_color((NFLABEL*)(combo->label), color);
	
	nfui_nfobject_modify_bg((NFOBJECT*)combo, NFOBJECT_STATE_NORMAL, COLOR_IDX(944));		
	nfui_nfobject_modify_bg((NFOBJECT*)combo, NFOBJECT_STATE_DISABLE, COLOR_IDX(952));		
	nfui_nfobject_modify_bg((NFOBJECT*)combo->label, NFOBJECT_STATE_DISABLE, COLOR_IDX(952));		

	color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(951);
	color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(955);
	color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(955);
	color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(953);
	nfui_nfbutton_set_pango_font(((NFBUTTON*)combo->button), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), color);

	color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(950);
	color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(954);
	color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(954);
	color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(952);
	nfui_nfbutton_set_bg_color(((NFBUTTON*)combo->button), color);

	dropdown_img[0] = nfui_get_image_from_file((IMG_N_SUBTAB_DROPDOWN_02), NULL);
	dropdown_img[1] = nfui_get_image_from_file((IMG_O_SUBTAB_DROPDOWN_02), NULL);
	dropdown_img[2] = nfui_get_image_from_file((IMG_P_SUBTAB_DROPDOWN_02), NULL);
	dropdown_img[3] = nfui_get_image_from_file((IMG_D_SUBTAB_DROPDOWN_02), NULL);

	nfui_get_pixbuf_size(dropdown_img[0], &size_w, &size_h);

	nfui_nfbutton_set_image(NF_BUTTON(combo->button), dropdown_img);
	nfui_nfobject_set_size(combo->button, size_w, size_h);

	combo->skin_type = NFCOMBOBOX_TYPE_SUBTAB_2;
}

static void  _set_combobox_type_popup_small_1(NFCOMBOBOX *combo)
{
	GdkPixbuf *dropdown_img[NFOBJECT_STATE_COUNT];
	gint color[NFOBJECT_STATE_COUNT];	
	gint size_w, size_h;

	nfui_nflabel_set_pango_font((NFLABEL*)(combo->label), nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(241));
	color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(241);
	color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(243);
	color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(245);
	color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(249);
    nfui_nflabel_set_fg_color((NFLABEL*)(combo->label), color);
	
	nfui_nfobject_modify_bg((NFOBJECT*)combo, NFOBJECT_STATE_NORMAL, COLOR_IDX(240));		
	nfui_nfobject_modify_bg((NFOBJECT*)combo, NFOBJECT_STATE_DISABLE, COLOR_IDX(248));		
	nfui_nfobject_modify_bg((NFOBJECT*)combo->label, NFOBJECT_STATE_DISABLE, COLOR_IDX(248));		

	color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(247);
	color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(251);
	color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(251);
	color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(249);
	nfui_nfbutton_set_pango_font(((NFBUTTON*)combo->button), nffont_get_pango_font(NFFONT_SMALL_SEMI), color);

	color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(246);
	color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(250);
	color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(250);
	color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(248);
	nfui_nfbutton_set_bg_color(((NFBUTTON*)combo->button), color);	

	dropdown_img[0] = nfui_get_image_from_file((MK_IMG_N_POPUP_DROPDOWN_SMALL_01), NULL);
	dropdown_img[1] = nfui_get_image_from_file((MK_IMG_O_POPUP_DROPDOWN_SMALL_01), NULL);
	dropdown_img[2] = nfui_get_image_from_file((MK_IMG_P_POPUP_DROPDOWN_SMALL_01), NULL);
	dropdown_img[3] = nfui_get_image_from_file((MK_IMG_D_POPUP_DROPDOWN_SMALL_01), NULL);

	nfui_get_pixbuf_size(dropdown_img[0], &size_w, &size_h);

	nfui_nfbutton_set_image(NF_BUTTON(combo->button), dropdown_img);
	nfui_nfobject_set_size(combo->button, size_w, size_h);

	combo->skin_type = NFCOMBOBOX_TYPE_POPUP_SMALL_1;
}

static void  _set_combobox_type_popup_small_2(NFCOMBOBOX *combo)
{
	GdkPixbuf *dropdown_img[NFOBJECT_STATE_COUNT];
	gint color[NFOBJECT_STATE_COUNT];	
	gint size_w, size_h;

	nfui_nflabel_set_pango_font((NFLABEL*)(combo->label), nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(255));
	color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(255);
	color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(257);
	color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(259);
	color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(263);
    nfui_nflabel_set_fg_color((NFLABEL*)(combo->label), color);
	
	nfui_nfobject_modify_bg((NFOBJECT*)combo, NFOBJECT_STATE_NORMAL, COLOR_IDX(254));		
	nfui_nfobject_modify_bg((NFOBJECT*)combo, NFOBJECT_STATE_DISABLE, COLOR_IDX(262));		
	nfui_nfobject_modify_bg((NFOBJECT*)combo->label, NFOBJECT_STATE_DISABLE, COLOR_IDX(262));		

	color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(261);
	color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(265);
	color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(265);
	color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(263);
	nfui_nfbutton_set_pango_font(((NFBUTTON*)combo->button), nffont_get_pango_font(NFFONT_SMALL_SEMI), color);

	color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(260);
	color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(264);
	color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(264);
	color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(262);
	nfui_nfbutton_set_bg_color(((NFBUTTON*)combo->button), color);

	dropdown_img[0] = nfui_get_image_from_file((MK_IMG_N_POPUP_DROPDOWN_SMALL_02), NULL);
	dropdown_img[1] = nfui_get_image_from_file((MK_IMG_O_POPUP_DROPDOWN_SMALL_02), NULL);
	dropdown_img[2] = nfui_get_image_from_file((MK_IMG_P_POPUP_DROPDOWN_SMALL_02), NULL);
	dropdown_img[3] = nfui_get_image_from_file((MK_IMG_D_POPUP_DROPDOWN_SMALL_02), NULL);

	nfui_get_pixbuf_size(dropdown_img[0], &size_w, &size_h);

	nfui_nfbutton_set_image(NF_BUTTON(combo->button), dropdown_img);
	nfui_nfobject_set_size(combo->button, size_w, size_h);

	combo->skin_type = NFCOMBOBOX_TYPE_POPUP_SMALL_2;
}

static gboolean nfcombobox_event_handler(NFCOMBOBOX *combo, GdkEvent *event, gpointer data)
{
	gchar *str = NULL;
	gint pos_x, pos_y, size_w, size_h;

	//DMSG(1, "###################  event->type : %d ###################", event->type);
	g_return_val_if_fail(combo != NULL, FALSE);
	g_assert(combo->skin_type);

	switch(event->type) {
		case GDK_EXPOSE:
			if(!combo->bg_gc) 
				nfcombobox_set_gc(combo);

			if(combo->bg_gc) 
				nfcombobox_draw_bg(combo);

			if(nfui_nfobject_is_supported_multi_lang(combo))
				nfui_nfobject_support_multi_lang(combo->label, TRUE);
			else
				nfui_nfobject_support_multi_lang(combo->label, FALSE);

			if(combo->spacing_type != NORMAL_SPACING) 
				nfui_nflabel_set_spacing((NFLABEL*)(combo->label), combo->spacing_type);

			if(combo->data) {
				if(!combo->unchange_string) {
					str = (gchar*)g_slist_nth_data(combo->data, (guint)combo->index);
					if(str)
						nfui_nflabel_set_text((NFLABEL*)(combo->label), str);
				}
			}

			// SKSHIN
			//if(!((NFFIXED*)combo)->children)
			//	nfcombobox_set_child_position(combo);

			nfcombobox_change_child_position(combo);
			break;

		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
			{
				GdkEventKey *kevt;
				KEYPAD_KID kpid;
				NFOBJECT *item;

				kevt = (GdkEventKey*)event;
				kpid = (KEYPAD_KID)kevt->keyval;

				//DMSG(1, "################### keypad press [%d]###################", kpid);
				if(kpid == KEYPAD_ENTER) {
					if(((NFOBJECT*)combo)->kfocus == NFOBJECT_FOCUS) {
						if(combo->submenu) {
							pos_x = pos_y = 0;
							size_w = combo->submenu->width;
							size_h = combo->submenu->height;
						
							nfcombobox_get_submenu_position(combo, &pos_x, &pos_y);
							nfcombobox_get_real_size_n_position(combo, &pos_x, &pos_y, &size_w, &size_h);

							if(combo->submenu->x != pos_x || combo->submenu->y != pos_y)
								nfui_nfobject_move(combo->submenu, pos_x, pos_y);
						
							if(nfui_nfobject_is_shown(combo->submenu) == FALSE) {
								nfui_nfobject_show(combo->submenu);

								nfui_make_key_hierarchy((NFWINDOW*)combo->submenu);

								item = g_slist_nth_data(combo->item, 0);
								nfui_set_key_focus(item, TRUE);

								nfui_page_open(PGID_COMBO_MENU, combo->submenu, nfui_get_last_user());
							}else {
								nfui_make_key_hierarchy((NFWINDOW*)combo->submenu);

								item = g_slist_nth_data(combo->item, 0);
								nfui_set_key_focus(item, TRUE);

								nfui_page_open(PGID_COMBO_MENU, combo->submenu, nfui_get_last_user());

								nfui_signal_emit(combo->submenu, GDK_EXPOSE, TRUE);
							}
						}
					}
				}else if(kpid == KEYPAD_EXIT) {
					if(((NFOBJECT*)combo)->kfocus == NFOBJECT_FOCUS) {
						if(combo->submenu) {
							if(nfui_nfobject_is_shown(combo->submenu) == TRUE) {
								nfcombobox_init_submenu_item(combo);

								nfui_nfobject_hide(combo->submenu);
								nfui_page_close(PGID_COMBO_MENU, combo->submenu);

								return TRUE;
							}
						}
					}
				}
#if 0
				else if(kpid == KEYPAD_UP
						|| kpid == KEYPAD_DOWN
						|| kpid == KEYPAD_LEFT
						|| kpid == KEYPAD_RIGHT) {
					if(nfui_nfobject_is_shown(combo->submenu) == TRUE) {
						nfcombobox_init_submenu_item(combo);

						nfui_nfobject_hide(combo->submenu);

						nfui_page_close(PGID_COMBO_MENU, combo->submenu);
					}
				}
#endif
			}
			break;
		case GDK_LEAVE_NOTIFY:
			{
				if(nfui_nfobject_is_shown((NFOBJECT*)combo))
					nfui_signal_emit((NFOBJECT*)combo, GDK_EXPOSE, TRUE);
			}
			break;
		case GDK_DELETE:
		{
			guint cnt;
			guint i;
			gchar *strTemp;

			if(combo->bg_gc)
				g_object_unref(combo->bg_gc);

			//if(combo->menu_color)
			//	gdk_color_free(combo->menu_color);	
			
			if(combo->timer_id) {
				g_source_remove(combo->timer_id);
				combo->timer_id = 0;
			}
					
			if(combo->data)
			{
				cnt = g_slist_length(combo->data);

				for(i=0; i<cnt; i++)
				{
					strTemp = (gchar*)g_slist_nth_data(combo->data, i);
					g_free(strTemp);
					strTemp = NULL;
				}
			
				g_slist_free(combo->data);
				combo->data = NULL;

			}
			
			if(combo->item)
			{
				g_slist_free(combo->item);
				combo->item = NULL;
			}

			if(combo->submenu) {
				nfui_page_close(PGID_COMBO_MENU, combo->submenu);
				nfui_nfobject_destroy(combo->submenu);
				combo->submenu = NULL;
			}

			if(combo) {
				//g_free(combo);
				//combo = NULL;
			}

			// SKSHIN
			/*
			if(!((NFFIXED*)combo)->children) {
				if(combo->label) {
					nfui_nfobject_destroy(combo->label);
					combo->label = NULL;
				}
				if(combo->button) {
					nfui_nfobject_destroy(combo->button);
					combo->button = NULL;
				}
			}
			*/

		}
		break;

		default:
			break;
	}

	return FALSE;
}


static void nfcombobox_set_gc(NFCOMBOBOX *combo)
{
	GdkDrawable *drawable = NULL;

	g_return_if_fail(combo != NULL);

	drawable = nfui_nfobject_get_window((NFOBJECT*)combo);

	if(drawable) {
		combo->bg_gc = gdk_gc_new(drawable);
		gdk_gc_set_rgb_fg_color(combo->bg_gc, &UX_COLOR(combo->combo_fixed.object.bg_color[NFOBJECT_STATE_NORMAL]));
	}else
		DMSG(1, "combobox drawable is NULL.");

}


static void nfcombobox_set_child_position(NFCOMBOBOX *combo)
{
	guint w, h;

	g_return_if_fail(combo != NULL);


	w = combo->combo_fixed.object.width;	
	h = combo->combo_fixed.object.height;	

	/* label */
	nfui_nfobject_set_size(combo->label, w-(combo->button->width)-4, h-4);
	nfui_nffixed_put((NFFIXED*)combo, combo->label, 2, 2);

	/* button */
	nfui_nffixed_put((NFFIXED*)combo, combo->button, w - combo->button->width, 0);

}

static void nfcombobox_change_child_position(NFCOMBOBOX *combo)
{
	guint w, h;

	g_return_if_fail(combo != NULL);


	w = combo->combo_fixed.object.width;	
	h = combo->combo_fixed.object.height;	

	/* label */
	nfui_nfobject_set_size(combo->label, w-(combo->button->width)-4, h-4);

	((NFOBJECT *)combo->label)->x = 2;
	((NFOBJECT *)combo->label)->y = 2;

	((NFOBJECT *)combo->button)->x = w - combo->button->width;
	((NFOBJECT *)combo->button)->y = 0;

}


static void nfcombobox_draw_bg(NFCOMBOBOX *combo)
{
	GdkDrawable *drawable = NULL;
	guint px, py;
	GdkRectangle rect;

	g_return_if_fail(combo != NULL);

	if(nfui_nfobject_is_disabled((NFOBJECT*)combo))
		return;

	drawable = nfui_nfobject_get_window((NFOBJECT*)combo);
	nfui_nfobject_get_offset((NFOBJECT*)combo, &px, &py);

	nfui_nfobject_get_clip_rect((NFOBJECT*)combo, &rect);
	gdk_gc_set_clip_rectangle(combo->bg_gc, &rect);	

	if(drawable)	
		gdk_draw_rectangle(drawable,
				combo->bg_gc,
				TRUE,
				(gint)px,
				(gint)py,
				(gint)(combo->combo_fixed.object.width - combo->button->width),
				(gint)(combo->combo_fixed.object.height));
	else
		DMSG(1, "combobox drawable is NULL.");

	if(combo->combo_fixed.object.kfocus == NFOBJECT_FOCUS) 
		nfcombobox_draw_outlines((NFOBJECT*)combo);
}

static void nfcombobox_draw_outlines(NFOBJECT *obj)
{
	GdkDrawable *drawable = NULL;
	GdkPoint points[5];
	GdkGC *line_gc;
	guint pos_x, pos_y;
	gint line_gap;
	gint line_width;

	drawable = nfui_nfobject_get_window(obj);

	/* outline */
	line_gc = nfui_nfobject_get_gc(obj);

	if(nfui_is_focus_at_child(obj))
		gdk_gc_set_rgb_fg_color(line_gc, &UX_COLOR(161));
	else
		gdk_gc_set_rgb_fg_color(line_gc, &UX_COLOR(160));

	gdk_gc_set_line_attributes(line_gc,
			COMBOBOX_LINE_BORDER,
			GDK_LINE_SOLID,
			GDK_CAP_NOT_LAST,
			GDK_JOIN_MITER);

	nfui_nfobject_get_offset(obj, &pos_x, &pos_y);
	line_gap = COMBOBOX_LINE_BORDER - 1;
	//line_width = obj->width;
	line_width = obj->width - NF_COMBOBOX(obj)->button->width;

	points[0].x = pos_x + line_gap;
	points[0].y = pos_y + line_gap;
	points[1].x = pos_x + line_width - line_gap;
	points[1].y = pos_y + line_gap;
	points[2].x = pos_x + line_width - line_gap;
	points[2].y = pos_y + obj->height - line_gap;
	points[3].x = pos_x + line_gap;
	points[3].y = pos_y + obj->height - line_gap;
	points[4].x = pos_x + line_gap;
	points[4].y = pos_y + line_gap;

	gdk_draw_lines(drawable,
			line_gc,
			points,
			5);

	nfui_nfobject_gc_unref(line_gc);
}

static void nfcombobox_draw_submenu_outline(NFOBJECT *obj)
{
	GdkDrawable *drawable = NULL;
	GdkGC *line_gc;
	GdkPoint points[5];
	gint pos_x, pos_y;

	drawable = nfui_nfobject_get_window(obj);
	line_gc = nfui_nfobject_get_gc(obj);
	gdk_gc_set_rgb_fg_color(line_gc, &UX_COLOR(161));

	gdk_gc_set_line_attributes(line_gc,
			COMBOBOX_LINE_BORDER,
			GDK_LINE_SOLID,
			GDK_CAP_NOT_LAST,
			GDK_JOIN_MITER);

	nfui_nfobject_get_offset(obj, &pos_x, &pos_y);

	points[0].x = pos_x + 1;
	points[0].y = pos_y + 1;
	points[1].x = pos_x + 1;
	points[1].y = pos_y + obj->height - 1;
	points[2].x = pos_x + obj->width - 1;
	points[2].y = pos_y + obj->height - 1;
	points[3].x = pos_x + obj->width - 1;
	points[3].y = pos_y + 1;
	points[4].x = pos_x + 1;
	points[4].y = pos_y + 1;

	gdk_draw_lines(drawable,
			line_gc,
			points,
			5);

	nfui_nfobject_gc_unref(line_gc);
}

static void nfcombobox_reset_submenu(NFCOMBOBOX *combo)
{
	if(combo == NULL) return;
	if(combo->submenu == NULL) return;

	if(nfui_nfobject_is_shown(combo->submenu)) 
	{
		nfcombobox_init_submenu_item(combo);
		nfui_nfobject_hide(combo->submenu);
		nfui_page_close(PGID_COMBO_MENU, combo->submenu);		

		nfui_clear_key_focus(combo->submenu);
		nfui_set_key_focus(combo, FALSE);
		nfui_set_key_focus(combo, TRUE);
	}

	nfui_nfobject_destroy(combo->submenu);
	combo->submenu = NULL;
}


static gboolean nfcombobox_button_cb(NFOBJECT *button, GdkEvent *event, gpointer data)
{
	NFCOMBOBOX *combo;
	NFOBJECT *item;
	gint pos_x, pos_y, size_w, size_h;

	combo = NF_COMBOBOX(button->parent);
//DMSG(1, "################### event->type %d  ###################", event->type);

	switch(event->type) {
		case GDK_EXPOSE:
			{			
				if(combo->data && !combo->submenu) 
					nfcombobox_create_submenu(combo);
			}
			break;

		case GDK_BUTTON_RELEASE:
			{		
				if(!combo->sensitive)
					return FALSE;

				if(event->button.button == MOUSE_RIGTH_BUTTON) 					
					return FALSE;

				if(combo->submenu) 
				{			
					if(nfui_nfobject_is_shown(combo->submenu) == FALSE) 
					{
						//DMSG(1, "################### button release ###################");
						pos_x = pos_y = 0;
						size_w = combo->submenu->width;
						size_h = combo->submenu->height;

						nfcombobox_get_submenu_position(combo, &pos_x, &pos_y);
						nfcombobox_get_real_size_n_position(combo, &pos_x, &pos_y, &size_w, &size_h);

						if(combo->submenu->x != pos_x || combo->submenu->y != pos_y)
							nfui_nfobject_move(combo->submenu, pos_x, pos_y);

						//if(combo->submenu->width != size_w || combo->submenu->height != size_h)
						//	nfui_nfobject_set_size(combo->submenu, size_w, size_h);

						nfui_nfobject_show(combo->submenu);
						nfui_make_key_hierarchy((NFWINDOW*)combo->submenu);

						item = g_slist_nth_data(combo->item, 0);
						if(item == NULL) 
							return FALSE;

						nfui_set_key_focus(item, TRUE);

						nfui_page_open(PGID_COMBO_MENU, combo->submenu, nfui_get_last_user());

					}
				}
			}
			break;
		default:
			break;
	}

	return FALSE;
}

static gint nfcombobox_get_text_w_size(NFCOMBOBOX *combo)
{
	GdkDrawable *drawable = NULL;
	gchar *data = NULL;
	gint tmp_w, w = 0;
	gint i;

	drawable = nfui_nfobject_get_window((NFOBJECT*)combo);
	for(i=0; i<g_slist_length(combo->data); i++) {
		data = (gchar*)g_slist_nth_data(combo->data, i);
		tmp_w = nfutil_string_width(1, drawable, ((NFLABEL*)(combo->label))->pango_font, data, combo->spacing_type);

		if(tmp_w > w)
			w = tmp_w;
	}

	return w;
}

static gboolean nfcombobox_get_real_size_n_position(NFCOMBOBOX *combo, gint *x, gint *y, gint *w, gint *h)
{
	gint tmp_h;
	gint tmp_y;
	gint margin = 40;

	tmp_h = *h;
	tmp_y = *y;


	/* x, width */
	if((gint)DISPLAY_ACTIVE_WIDTH < *w) {
		*x = 0;
		*w = (gint)DISPLAY_ACTIVE_WIDTH;
	}

	if((*x + *w) > (gint)DISPLAY_ACTIVE_WIDTH) 
		*x = (*x + *w) - (gint)DISPLAY_ACTIVE_WIDTH;

	
	/* y, height */
	if((*y + *h) > (gint)DISPLAY_ACTIVE_HEIGHT) {
		if (*y - (*h + ((NFOBJECT*)combo)->height) > 0) {
			*y -= (*h + ((NFOBJECT*)combo)->height);
		}
		else {
			*h = (gint)DISPLAY_ACTIVE_HEIGHT - *y;
		}
	}

	*h -= (*h % ((NFOBJECT*)combo)->height);
	
	if(*h <= 0 || *h < ((NFOBJECT*)combo)->height) {
		*h = tmp_h;
		*y -= (*h + ((NFOBJECT*)combo)->height);
		
		if (*y < 0) {		    
		    *h -= abs(*y) + margin;        	
		    *h -= (*h % ((NFOBJECT*)combo)->height);      
		    
		    *y = 0 + margin;		
	    }	
    }

	return TRUE;
}

static gboolean nfcombobox_get_submenu_position(NFCOMBOBOX *combo, gint *x, gint *y)
{
	NFOBJECT *top;
    NFSCROLLEDFIXED *scrolled_fixed;
    gint scrolled_fixed_x, scrolled_fixed_y;
	gint off_x, off_y, ori_x, ori_y;

	off_x = off_y = ori_x = ori_y = 0;

	top = nfui_nfobject_get_top((NFOBJECT*)combo);
	nfui_nfobject_get_window_pos((NFOBJECT*)top, &ori_x, &ori_y);
	nfui_nfobject_get_offset((NFOBJECT*)combo, &off_x, &off_y);

    if (nfui_nfobject_is_scrolledfixed_usescr(combo) && nfui_nfobject_is_scrolledfixed_child(combo))
    {
        scrolled_fixed = nfui_nfobject_get_scrolledfixed(combo);
        nfui_nfobject_get_offset(scrolled_fixed, &scrolled_fixed_x, &scrolled_fixed_y);

        off_x -= (scrolled_fixed->relative_x - scrolled_fixed_x);
        off_y -= (scrolled_fixed->relative_y - scrolled_fixed_y);
    }    

	*x += (gint)ori_x + off_x;
	*y += ((gint)ori_y + combo->combo_fixed.object.height + off_y);

	return TRUE;
}

static gboolean nfcombobox_get_submenu_size(NFCOMBOBOX *combo, gint *w, gint *h)
{
	guint item_cnt = 0;

	if(w != NULL) {
		*w = nfcombobox_get_text_w_size(combo);
		*w += COMBOBOX_ADDED_SPACE;
	}

	if(h != NULL) {
		*h = ((NFOBJECT*)combo)->height;

		item_cnt = g_slist_length(combo->data);

		if ((combo->display_cnt != -1) && (combo->display_cnt < item_cnt)) {
			item_cnt = combo->display_cnt;
		}

		*h *= item_cnt;
	}

	if(w == NULL && h == NULL)
		return FALSE;

	return TRUE;
}

static gboolean nfcombobox_create_submenu(NFCOMBOBOX *combo)
{
	NFOBJECT* fixed;
	NFOBJECT* btn;
	GdkDrawable *drawable = NULL;
	gchar *data = NULL;
	guint item_cnt = 0;
	gint pos_x, pos_y, size_w, size_h;
	guint i;
	gint scroll_w = 0, scroll_h = 0;
	gint icon_w = 0, icon_h = 0;
	gint submenu_margin = COMBOBOX_SUBMENU_MARGIN;
	gint color_idx;

	GdkPixbuf *scroll_up[NFOBJECT_STATE_COUNT];
	GdkPixbuf *scroll_down[NFOBJECT_STATE_COUNT];


	g_return_val_if_fail(combo != NULL, FALSE);


	item_cnt = g_slist_length(combo->data);
	if(item_cnt == 0)
		return TRUE;

#if 0
	if(combo->submenu) {
		nfui_nfobject_destroy(combo->submenu);	
		combo->submenu = NULL;
	}
#endif

	pos_x = pos_y = size_w = size_h = 0;
	nfcombobox_get_submenu_position(combo, &pos_x, &pos_y);
	nfcombobox_get_submenu_size(combo, &size_w, &size_h);
	nfcombobox_get_real_size_n_position(combo, &pos_x, &pos_y, &size_w, &size_h);

//	DMSG(1, "########### pos_x: %d, pos_y: %d, size_w: %d, size_h: %d \n", pos_x, pos_y, size_w, size_h);

	if(combo->icon != NULL) 
		nfui_get_pixbuf_size(combo->icon, &icon_w, &icon_h);

	if(size_h < (item_cnt * ((NFOBJECT*)combo)->height)) {
		scroll_up[0] = nfui_get_image_from_file((IMG_SCROLL_UP_N), NULL);
		scroll_up[1] = nfui_get_image_from_file((IMG_SCROLL_UP_O), NULL);	
		scroll_up[2] = nfui_get_image_from_file((IMG_SCROLL_UP_P), NULL);	
		scroll_up[3] = nfui_get_image_from_file((IMG_SCROLL_UP_N), NULL);	

		scroll_down[0] = nfui_get_image_from_file((IMG_SCROLL_DOWN_N), NULL);
		scroll_down[1] = nfui_get_image_from_file((IMG_SCROLL_DOWN_O), NULL);
		scroll_down[2] = nfui_get_image_from_file((IMG_SCROLL_DOWN_P), NULL);
		scroll_down[3] = nfui_get_image_from_file((IMG_SCROLL_DOWN_N), NULL);

		nfui_get_pixbuf_size(scroll_up[0], (gint*)&scroll_w, (gint*)&scroll_h);
	}

	/* window */
	if((size_w + scroll_w + icon_w) < ((NFOBJECT*)combo)->width)
		size_w = ((NFOBJECT*)combo)->width;
	else
		size_w += (scroll_w + icon_w);
	
	combo->submenu = (NFOBJECT*)nfui_nfwindow_new(combo, pos_x, pos_y, size_w + submenu_margin, size_h + submenu_margin);

	nfui_nfobject_modify_bg(combo->submenu, NFOBJECT_STATE_NORMAL, combo->combo_fixed.object.bg_color[NFOBJECT_STATE_NORMAL]);
	nfui_regi_post_event_callback(combo->submenu, nfcombmbox_submenu_event_cb);
	nfui_nfwindow_use_outside_evt((NFWINDOW*)combo->submenu, TRUE);
	nfui_nfwindow_set_mask((NFWINDOW*)combo->submenu, GDK_BUTTON_PRESS, TRUE);
	nfui_nfobject_hide(combo->submenu);

	nfui_nfobject_set_data(combo->submenu, "parent", combo);

	/* fixed */
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, size_w + submenu_margin, size_h + submenu_margin);
	nfui_regi_post_event_callback(fixed, nfcombmbox_submenu_fixed_event_cb);
	nfui_nfobject_show(fixed);

	/* scroll */
	if(size_h < (item_cnt * ((NFOBJECT*)combo)->height)) {
		for(i=0; i<2; i++) {
			btn = nfui_nfbutton_new();
			nfui_nfbutton_set_font_alignment(NF_BUTTON(btn), NFALIGN_CENTER, 0);
			nfui_nfbutton_set_spacing(NF_BUTTON(btn), combo->spacing_type);
			if(i == 0) 	nfui_nfbutton_set_image(NF_BUTTON(btn), scroll_up);
			else	 	nfui_nfbutton_set_image(NF_BUTTON(btn), scroll_down);
			nfui_nfobject_set_size(btn, scroll_w, scroll_h);
			nfui_nfobject_use_focus(btn, NFOBJECT_FOCUS_OFF);
			nfui_regi_post_event_callback(btn, nfcombobox_scroll_button_cb);
			nfui_nfobject_show(btn);

			if(i == 0) {
				combo->scroll_up = btn;
				nfui_nffixed_put((NFFIXED*)fixed, combo->scroll_up, (size_w - scroll_w + (submenu_margin/2)), (submenu_margin/2));
			}else {
				combo->scroll_down = btn;
				nfui_nffixed_put((NFFIXED*)fixed, combo->scroll_down, (size_w - scroll_w + (submenu_margin/2)), (size_h - scroll_h + (submenu_margin/2)));
			}
		}
	}

	/* menu items */
	for(i=0; i<item_cnt; i++) {
		if(((i * ((NFOBJECT*)combo)->height)) >= size_h)
			break;

		data = (gchar*)g_slist_nth_data(combo->data, i);
		btn = (NFOBJECT*)nfui_nfbutton_new_no_image(combo->button->bg_color, data);
		nfui_nfbutton_set_pango_font(NF_BUTTON(btn), ((NFLABEL*)(combo->label))->pango_font, ((NFBUTTON*)combo->button)->font_color);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(btn), COMBOBOX_TEXT_ALIGN, COMBOBOX_TEXT_LEFT_MARGIN);
		nfui_nfbutton_set_spacing(NF_BUTTON(btn), combo->spacing_type);
		if(combo->icon != NULL) 
			nfui_nfbutton_set_icon_image((NFBUTTON*)btn, combo->icon);
		if(!nfui_nfobject_is_supported_multi_lang(combo)) 
			nfui_nfobject_support_multi_lang(btn, FALSE);
		nfui_nfobject_set_size(btn, (size_w - scroll_w), ((NFOBJECT*)combo)->height);
		nfui_nfobject_use_tooltip(btn, FALSE);
		nfui_regi_post_event_callback(btn, nfcombobox_menu_button_cb);
		nfui_nfobject_show(btn);
		nfui_nffixed_put((NFFIXED*)fixed, btn, (submenu_margin/2), (i * ((NFOBJECT*)combo)->height) + (submenu_margin/2));

		if ((i < MAX_COLORMAP) && (combo->colormap[i] != -1))
		{
			color_idx = combo->colormap[i];
			nfui_nfbutton_set_pango_font((NFBUTTON*)btn, ((NFLABEL*)(combo->label))->pango_font, combo->multi_fgcolor[color_idx]);			
			nfui_nfbutton_set_bg_color((NFBUTTON*)btn, combo->multi_bgcolor[color_idx]);
		}

		combo->item = g_slist_append(combo->item, (gpointer)btn);
		nfui_nfobject_set_data((NFOBJECT*)btn, "index", GINT_TO_POINTER(i));
	}

	/* init scroll-bar */
	if(combo->scroll_up && combo->scroll_down) {
		combo->scroll.size_h = (size_h - (scroll_h * 2)) / (item_cnt - i + 1);
		combo->scroll.interval = (size_h - (scroll_h * 2)) / (item_cnt - i + 1);
		combo->scroll.total_step = (item_cnt - i);
		combo->scroll.cur_step = 0;

		nfcombobox_create_scroll_bar(combo);
	}


	nfui_nfwindow_add((NFWINDOW*)combo->submenu, fixed);

	nfui_run_main_event_handler(combo->submenu);

	return TRUE;
}

static gboolean nfcombmbox_submenu_fixed_event_cb(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFCOMBOBOX *combo;
	//DMSG(1, "################### event type : %d ###################", event->type);

	combo = (NFCOMBOBOX*)nfui_nfobject_get_data(obj->parent, "parent");

	if(event->type == GDK_EXPOSE) {
		if(combo->submenu && nfui_nfobject_is_shown(combo->submenu) == TRUE) {
			nfcombobox_draw_submenu_outline(obj);

			if(combo->scroll_up && combo->scroll_down) {
				DMSG(1, "GDK_EXPOSE");
				nfcombobox_draw_scroll_bar(combo);
			}
		}
	}

	return FALSE;
}

static gboolean nfcombmbox_submenu_event_cb(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFCOMBOBOX *combo;
	//DMSG(1, "################### event type : %d ###################", event->type);

	combo = (NFCOMBOBOX*)nfui_nfobject_get_data(obj, "parent");

	switch(event->type) {
		case GDK_BUTTON_PRESS:
			if(event->button.button == MOUSE_RIGTH_BUTTON) 					
				return FALSE;
		case NFOUTEVT_BUTTON_PRESS:
			{
				if(nfui_is_focus_at_child((NFOBJECT*)combo)) {
					nfui_set_key_focus((NFOBJECT*)combo, FALSE);
					nfui_set_key_focus((NFOBJECT*)combo, TRUE);

					nfui_signal_emit((NFOBJECT*)combo, GDK_EXPOSE, TRUE);
				}

				if(nfui_nfobject_is_shown(obj)) {
					nfcombobox_init_submenu_item(combo);

					nfui_nfobject_hide(obj);
					nfui_page_close(PGID_COMBO_MENU, obj);

					if(!combo->scroll_up && !combo->scroll_down) {
						nfui_clear_key_focus(obj);
						nfui_set_key_focus((NFOBJECT*)combo, FALSE);
						nfui_set_key_focus((NFOBJECT*)combo, TRUE);

						nfui_signal_emit((NFOBJECT*)combo, GDK_EXPOSE, TRUE);
					}
				}
			}
			break;
		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
			{
				GdkEventKey *kevt;
				KEYPAD_KID kpid;
				NFOBJECT *cur_focus;
				NFOBJECT *item;
				NFOBJECT *tmp_obj;
				guint i = 0;

				kevt = (GdkEventKey*)event;
				kpid = (KEYPAD_KID)kevt->keyval;
				//DMSG(1, "################### keypad press [%d]###################", kpid);

				if(kpid == KEYPAD_UP) {
					if(!combo->scroll_up)
						break;

					cur_focus = nfui_get_cur_focus(obj);
					//DMSG(1, "######### slist index : %d #############", g_slist_index(combo->item, (gconstpointer)cur_focus));
					item = (NFOBJECT*)g_slist_nth_data(combo->item, 0);

					if(cur_focus == item) {
						while(1) {
							tmp_obj = (NFOBJECT*)g_slist_nth_data(combo->item, i++);

							if(!tmp_obj) break;

							if(tmp_obj == cur_focus) continue;

							if(tmp_obj->use_focus == NFOBJECT_FOCUS_OFF) break;

							nfui_nfobject_use_focus(tmp_obj, NFOBJECT_FOCUS_OFF);
						}

						nfui_set_key_focus(cur_focus, TRUE);

						if(!nfcombobox_scroll_menu_items(combo, SCROLL_UP)) {
							if(combo->timer_id != 0) {
								g_source_remove(combo->timer_id);
								combo->timer_id = 0;

								break;
							}

							nfui_nfobject_use_focus(cur_focus, NFOBJECT_FOCUS_OFF);
							nfui_set_key_focus(cur_focus, FALSE);

							item = (NFOBJECT*)g_slist_nth_data(combo->item, (g_slist_length(combo->item)) - 1);

							nfui_nfobject_use_focus(item, NFOBJECT_FOCUS_ON);
							nfui_set_key_focus(item, TRUE);

							nfcombobox_scroll_menu_items(combo, SCROLL_BOTTOM); 
						}else {
							if(!combo->timer_id)
								combo->timer_id = g_timeout_add(COMBOBOX_REPEAT_TIME, nfcombobox_submenu_key_scroll_cb, (gpointer)combo->scroll_up);
						}

						nfui_make_key_hierarchy((NFWINDOW*)obj);
					}else {
						if(item->use_focus == TRUE)
							return FALSE;

						item = (NFOBJECT*)g_slist_nth_data(combo->item, (g_slist_length(combo->item)) - 1);

						if(cur_focus == item) {
							if(combo->timer_id != 0) {
								g_source_remove(combo->timer_id);
								combo->timer_id = 0;
							}

							while(1) {
								tmp_obj = (NFOBJECT*)g_slist_nth_data(combo->item, i++);

								if(!tmp_obj) break;

								nfui_nfobject_use_focus(tmp_obj, NFOBJECT_FOCUS_ON);
							}

							nfui_make_key_hierarchy((NFWINDOW*)obj);
						}
					}

				}else if(kpid == KEYPAD_DOWN) {
					if(!combo->scroll_down)
						break;

					cur_focus = nfui_get_cur_focus(obj);
					//DMSG(1, "######### slist index : %d %d#############", g_slist_index(combo->item, (gconstpointer)cur_focus), g_slist_length(combo->item));
					item = (NFOBJECT*)g_slist_nth_data(combo->item, (g_slist_length(combo->item) - 1));

					if(cur_focus == item) {
						while(1) {
							tmp_obj = (NFOBJECT*)g_slist_nth_data(combo->item, i++);

							if(!tmp_obj) break;

							if(tmp_obj == cur_focus) continue;

							if(tmp_obj->use_focus == NFOBJECT_FOCUS_OFF) break;

							nfui_nfobject_use_focus(tmp_obj, NFOBJECT_FOCUS_OFF);
						}

						nfui_set_key_focus(cur_focus, TRUE);

						if(!nfcombobox_scroll_menu_items(combo, SCROLL_DOWN)) {
							if(combo->timer_id != 0) {
								g_source_remove(combo->timer_id);
								combo->timer_id = 0;

								break;
							}

							nfui_nfobject_use_focus(cur_focus, NFOBJECT_FOCUS_OFF);
							nfui_set_key_focus(cur_focus, FALSE);

							item = (NFOBJECT*)g_slist_nth_data(combo->item, 0);

							nfui_nfobject_use_focus(item, NFOBJECT_FOCUS_ON);
							nfui_set_key_focus(item, TRUE);

							nfcombobox_scroll_menu_items(combo, SCROLL_TOP);
						}else {
							if(!combo->timer_id)
								combo->timer_id = g_timeout_add(COMBOBOX_REPEAT_TIME, nfcombobox_submenu_key_scroll_cb, (gpointer)combo->scroll_down);
						}

						nfui_make_key_hierarchy((NFWINDOW*)obj);
					}else {
						if(item->use_focus == TRUE)
							return FALSE;

						item = (NFOBJECT*)g_slist_nth_data(combo->item, 0);

						if(cur_focus == item) {
							if(combo->timer_id != 0) {
								g_source_remove(combo->timer_id);
								combo->timer_id = 0;
							}

							while(1) {
								tmp_obj = (NFOBJECT*)g_slist_nth_data(combo->item, i++);

								if(!tmp_obj) break;

								nfui_nfobject_use_focus(tmp_obj, NFOBJECT_FOCUS_ON);
							}

							nfui_make_key_hierarchy((NFWINDOW*)obj);
						}
					}
				}else if(kpid == KEYPAD_EXIT) {
					if(nfui_nfobject_is_shown(obj)) {
						nfcombobox_init_submenu_item(combo);

						nfui_nfobject_hide(obj);
						nfui_page_close(PGID_COMBO_MENU, obj);

						nfui_clear_key_focus(obj);
						nfui_set_key_focus(combo, FALSE);
						nfui_set_key_focus(combo, TRUE);

						nfui_signal_emit(combo, GDK_EXPOSE, TRUE);

						return TRUE;
					}
				}
			}
			break;
		case NFEVENT_KEYPAD_RELEASE:
		case NFEVENT_REMOCON_RELEASE:
			{
				//DMSG(1, "################### keypad release ###################");
				if(combo->timer_id != 0) {
					g_source_remove(combo->timer_id);
					combo->timer_id = 0;
				}
			}
			break;
		default:
			break;
	}

	return FALSE;
}


static gboolean nfcombobox_label_cb(NFOBJECT *label, GdkEvent *event, gpointer data)
{
	NFCOMBOBOX *combo;
	NFOBJECT *item;
	gint pos_x, pos_y, size_w, size_h;

	combo = NF_COMBOBOX(label->parent);
	//DMSG(1, "################### event->type: %d ###################", event->type);
	
	// SKSHIN
	if (!combo) {
/*		printf("############################## ERROR ............\n");
		printf("############################## ERROR ............\n");
		printf("############################## ERROR ............\n");
		printf("############################## ERROR ............\n");
		printf("############################## ERROR ............\n");*/
		return FALSE;
	}

	switch(event->type) {
		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
			{
				GdkEventKey *kevt;
				KEYPAD_KID kpid;
				NFOBJECT *item;

				kevt = (GdkEventKey*)event;
				kpid = (KEYPAD_KID)kevt->keyval;

				//DMSG(1, "################### keypad press [%d]###################", kpid);
				switch(kpid) {
					case KEYPAD_ENTER:
						{
							if(((NFOBJECT*)combo)->kfocus == NFOBJECT_FOCUS) {
								if(combo->submenu) {
									if(nfui_nfobject_is_shown(combo->submenu) == FALSE) {
										pos_x = pos_y = 0;
										size_w = combo->submenu->width;
										size_h = combo->submenu->height;
									
										nfcombobox_get_submenu_position(combo, &pos_x, &pos_y);
										nfcombobox_get_real_size_n_position(combo, &pos_x, &pos_y, &size_w, &size_h);

										if(combo->submenu->x != pos_x || combo->submenu->y != pos_y)
											nfui_nfobject_move(combo->submenu, pos_x, pos_y);
									
										nfui_nfobject_show(combo->submenu);
										nfui_make_key_hierarchy((NFWINDOW*)combo->submenu);
										item = g_slist_nth_data(combo->item, 0);
										nfui_set_key_focus(item, TRUE);
										nfui_page_open(PGID_COMBO_MENU, combo->submenu, nfui_get_last_user());
									}
								}
							}
						}
						break;
					case KEYPAD_EXIT:
						{
							if(((NFOBJECT*)combo)->kfocus == NFOBJECT_FOCUS) {
								if(combo->submenu) {
									if(nfui_nfobject_is_shown(combo->submenu) == TRUE) {
										nfcombobox_init_submenu_item(combo);

										nfui_nfobject_hide(combo->submenu);
										nfui_page_close(PGID_COMBO_MENU, combo->submenu);
									}
								}
							}
						}
						break;
					default:
						break;
				}
			}
			break;

		case GDK_BUTTON_PRESS:
			{
				if(event->button.button == MOUSE_RIGTH_BUTTON) 					
					return FALSE;

				if(nfui_is_focus_at_child((NFOBJECT*)combo)) {
					nfui_set_key_focus((NFOBJECT*)combo, FALSE);
					nfui_set_key_focus((NFOBJECT*)combo, TRUE);

					nfui_signal_emit((NFOBJECT*)combo, GDK_EXPOSE, TRUE);
				}
			}
			break;

		case GDK_BUTTON_RELEASE:
			{
				if(event->button.button == MOUSE_RIGTH_BUTTON) 					
					return FALSE;

				/*
				if(nfui_is_focus_at_child((NFOBJECT*)combo)) {
					nfui_set_key_focus((NFOBJECT*)combo, FALSE);
					nfui_set_key_focus((NFOBJECT*)combo, TRUE);

					nfui_signal_emit((NFOBJECT*)combo, GDK_EXPOSE, TRUE);
				}
				*/

				if(combo->submenu) {
					if(nfui_nfobject_is_shown(combo->submenu) == FALSE) {
						pos_x = pos_y = 0;
						size_w = combo->submenu->width;
						size_h = combo->submenu->height;
					
						nfcombobox_get_submenu_position(combo, &pos_x, &pos_y);
						nfcombobox_get_real_size_n_position(combo, &pos_x, &pos_y, &size_w, &size_h);

						if(combo->submenu->x != pos_x || combo->submenu->y != pos_y)
							nfui_nfobject_move(combo->submenu, pos_x, pos_y);
					
						nfui_nfobject_show(combo->submenu);
						nfui_make_key_hierarchy((NFWINDOW*)combo->submenu);
						item = g_slist_nth_data(combo->item, 0);
						nfui_set_key_focus(item, TRUE);
						nfui_page_open(PGID_COMBO_MENU, combo->submenu, nfui_get_last_user());
					}
				}
			}
			break;

		default:
			break;
	}

	return FALSE;
}

static gboolean nfcombobox_menu_button_cb(NFOBJECT *button, GdkEvent *event, gpointer data)
{
	NFCOMBOBOX *combo;
	gchar *str;
	GSList *tmp;

	combo = (NFCOMBOBOX*)nfui_nfobject_get_data(button->parent->parent, "parent");
	//DMSG(1, "################### event->type : %d ###################", event->type);

	switch(event->type) {
		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
			{
				GdkEventKey *kevt;
				KEYPAD_KID kpid;

				kevt = (GdkEventKey*)event;
				kpid = (KEYPAD_KID)kevt->keyval;
				//DMSG(1, "################### keypad press [%d]###################", kpid);

				if(kpid != KEYPAD_ENTER) 
					break;

				nfui_set_key_focus((NFOBJECT*)combo, FALSE);
				nfui_set_key_focus((NFOBJECT*)combo, TRUE);
				nfui_signal_emit((NFOBJECT*)combo, GDK_EXPOSE, TRUE);
			}
		case GDK_BUTTON_RELEASE:
			{
				if(event->button.button == MOUSE_RIGTH_BUTTON) 					
					return FALSE;

				str = nfui_nfbutton_get_text((NFBUTTON*)button);
				if(!combo->unchange_string)
					nfui_nflabel_set_text((NFLABEL*)(combo->label), str);

				nfui_nfobject_hide(button->parent->parent);

				nfui_page_close(PGID_COMBO_MENU, button->parent->parent);

				if(nfui_is_focus_at_child((NFOBJECT*)combo)) {
					nfui_set_key_focus((NFOBJECT*)combo, FALSE);
					nfui_set_key_focus((NFOBJECT*)combo, TRUE);
				}

				nfcombobox_draw_bg(combo);
				nfui_signal_emit(combo->label, GDK_EXPOSE, TRUE);

				/* set index */
				combo->index = GPOINTER_TO_INT(nfui_nfobject_get_data((NFOBJECT*)button, "index"));
				
				nfcombobox_init_submenu_item(combo);

				nfui_set_key_focus(button, FALSE);
				nfui_user_signal_emit((NFOBJECT*)combo, NFEVENT_COMBOBOX_CHANGED, FALSE);
			}
			break;
		case GDK_SCROLL:
			{
				GdkEventScroll *sevt;

				sevt = (GdkEventScroll*)event;
				if(sevt->direction == GDK_SCROLL_UP)
					nfcombobox_scroll_menu_items(combo, SCROLL_UP); 
				else if(sevt->direction == GDK_SCROLL_DOWN)
					nfcombobox_scroll_menu_items(combo, SCROLL_DOWN); 
			}
			break;
		default:
			break;
	}

	return FALSE;
}

static void nfcombobox_init_submenu_item(NFCOMBOBOX *combo)
{
	NFBUTTON *button;
	gchar *str;
	guint i = 0;
	gint color_idx;
	
	if(!combo->scroll_up && !combo->scroll_down)
		return;

	combo->scroll.cur_step = 0;

	while(1) 
	{
		button = g_slist_nth_data(combo->item, i);
		if(button == NULL) break;

		if ((i < MAX_COLORMAP) && (combo->colormap[i] != -1))
		{
			color_idx = combo->colormap[i];
			nfui_nfbutton_set_pango_font((NFBUTTON*)button, ((NFLABEL*)(combo->label))->pango_font, combo->multi_fgcolor[color_idx]);			
			nfui_nfbutton_set_bg_color((NFBUTTON*)button, combo->multi_bgcolor[color_idx]);
		}

		str = (gchar*)g_slist_nth_data(combo->data, i);	
		nfui_nfbutton_set_text(button, str);	

		nfui_nfobject_use_focus((NFOBJECT*)button, NFOBJECT_FOCUS_ON);

		if (i == 0) 	nfui_set_key_focus((NFOBJECT*)button, TRUE);
		else 		nfui_set_key_focus((NFOBJECT*)button, FALSE);

		nfui_nfobject_set_data((NFOBJECT*)button, "index", GINT_TO_POINTER(i));
		i++;
	}
}

static gboolean nfcombobox_submenu_key_scroll_cb(gpointer data)
{
	NFCOMBOBOX *combo = NULL;

	NFUTIL_THREADS_ENTER();

	combo = (NFCOMBOBOX*)nfui_nfobject_get_data(((NFOBJECT*)data)->parent->parent, "parent");

	if((NFOBJECT*)data == combo->scroll_up) {
		nfui_key_signal_emit((NFOBJECT*)combo->submenu, NFEVENT_KEYPAD_PRESS, KEYPAD_UP, TRUE);
	} else if((NFOBJECT*)data == combo->scroll_down) {
		nfui_key_signal_emit((NFOBJECT*)combo->submenu, NFEVENT_KEYPAD_PRESS, KEYPAD_DOWN, TRUE);
	}

	NFUTIL_THREADS_LEAVE();

	return TRUE;
}

static gboolean nfcombobox_submenu_scroll_cb(gpointer data)
{
	NFCOMBOBOX *combo = NULL;

	NFUTIL_THREADS_ENTER();

	combo = (NFCOMBOBOX*)nfui_nfobject_get_data(((NFOBJECT*)data)->parent->parent, "parent");

	if((NFOBJECT*)data == combo->scroll_up) {
	 	if(!nfcombobox_scroll_menu_items(combo, SCROLL_UP)) {
			NFUTIL_THREADS_LEAVE();
			return FALSE;
		}
	} else if((NFOBJECT*)data == combo->scroll_down) {
	 	if(!nfcombobox_scroll_menu_items(combo, SCROLL_DOWN)) {
			NFUTIL_THREADS_LEAVE();
			return FALSE;
		}
	}

	NFUTIL_THREADS_LEAVE();

	return TRUE;
}

static gboolean nfcombobox_scroll_menu_items(NFCOMBOBOX *combo, ScrollDir dir)
{
	NFOBJECT *button;
	GSList *tmp;
	gint start_idx = 0;
	guint start_item = 0;
	gchar *str;
	gint color_idx;

	if(combo->scroll_up == NULL || combo->scroll_down == NULL)
		return FALSE;

	switch(dir) {
		case SCROLL_TOP:
			{
				while(1) 
				{
					button = (NFOBJECT*)g_slist_nth_data(combo->item, start_item++);
					if(!button) break;

					if ((start_idx < MAX_COLORMAP) && (combo->colormap[start_idx] != -1))
					{
						color_idx = combo->colormap[start_idx];
						nfui_nfbutton_set_pango_font((NFBUTTON*)button, ((NFLABEL*)(combo->label))->pango_font, combo->multi_fgcolor[color_idx]);			
						nfui_nfbutton_set_bg_color((NFBUTTON*)button, combo->multi_bgcolor[color_idx]);
					}

					str = (gchar*)g_slist_nth_data(combo->data, (guint)start_idx);
					nfui_nfbutton_set_text((NFBUTTON*)button, str);
					nfui_signal_emit(button, GDK_EXPOSE, TRUE);
					
					nfui_nfobject_set_data((NFOBJECT*)button, "index", GINT_TO_POINTER(start_idx));
					start_idx++;
				}

				combo->scroll.cur_step = 0;
				nfcombobox_draw_scroll_bar(combo);
			}
			break;
		case SCROLL_BOTTOM:
			{
				start_item = g_slist_length(combo->item);
				start_item -= 1;

				start_idx = g_slist_length(combo->data);
				start_idx -= 1;

				while(1) 
				{
					button = (NFOBJECT*)g_slist_nth_data(combo->item, start_item--);
					if(!button) break;

					if ((start_idx < MAX_COLORMAP) && (combo->colormap[start_idx] != -1))
					{
						color_idx = combo->colormap[start_idx];
						nfui_nfbutton_set_pango_font((NFBUTTON*)button, ((NFLABEL*)(combo->label))->pango_font, combo->multi_fgcolor[color_idx]);			
						nfui_nfbutton_set_bg_color((NFBUTTON*)button, combo->multi_bgcolor[color_idx]);
					}

					str = (gchar*)g_slist_nth_data(combo->data, (guint)start_idx);
					nfui_nfbutton_set_text((NFBUTTON*)button, str);
					nfui_signal_emit(button, GDK_EXPOSE, TRUE);

					nfui_nfobject_set_data((NFOBJECT*)button, "index", GINT_TO_POINTER(start_idx));
					start_idx--;
				}

				combo->scroll.cur_step = combo->scroll.total_step;
				nfcombobox_draw_scroll_bar(combo);
			}
			break;
		case SCROLL_UP:
		case SCROLL_DOWN:
			{
				button = (NFOBJECT*)g_slist_nth_data(combo->item, 0);
				start_idx = GPOINTER_TO_INT(nfui_nfobject_get_data((NFOBJECT*)button, "index"));

				if (start_idx < 0) return FALSE;

				if (dir == SCROLL_UP) 
				{
					if (--start_idx < 0) return FALSE;

					while(1) 
					{
						button = (NFOBJECT*)g_slist_nth_data(combo->item, start_item++);
						if(!button) break;

						if ((start_idx < MAX_COLORMAP) && (combo->colormap[start_idx] != -1))
						{
							color_idx = combo->colormap[start_idx];
							nfui_nfbutton_set_pango_font((NFBUTTON*)button, ((NFLABEL*)(combo->label))->pango_font, combo->multi_fgcolor[color_idx]);			
							nfui_nfbutton_set_bg_color((NFBUTTON*)button, combo->multi_bgcolor[color_idx]);
						}
					
						str = (gchar*)g_slist_nth_data(combo->data, (guint)start_idx);
						nfui_nfbutton_set_text((NFBUTTON*)button, str);
						nfui_signal_emit(button, GDK_EXPOSE, TRUE);

						nfui_nfobject_set_data((NFOBJECT*)button, "index", GINT_TO_POINTER(start_idx));
						start_idx++;
					}

					if (combo->scroll.cur_step > 0) 
					{
						combo->scroll.cur_step--;
						nfcombobox_draw_scroll_bar(combo);
					}
				}
				else 
				{
					if ((g_slist_length(combo->data) - (guint)start_idx) == g_slist_length(combo->item)) return FALSE;

					start_idx++;

					while(1) 
					{
						button = (NFOBJECT*)g_slist_nth_data(combo->item, start_item++);
						if(!button) break;

						if ((start_idx < MAX_COLORMAP) && (combo->colormap[start_idx] != -1))
						{
							color_idx = combo->colormap[start_idx];
							nfui_nfbutton_set_pango_font((NFBUTTON*)button, ((NFLABEL*)(combo->label))->pango_font, combo->multi_fgcolor[color_idx]);			
							nfui_nfbutton_set_bg_color((NFBUTTON*)button, combo->multi_bgcolor[color_idx]);
						}
					
						str = (gchar*)g_slist_nth_data(combo->data, (guint)start_idx);
						nfui_nfbutton_set_text((NFBUTTON*)button, str);
						nfui_signal_emit(button, GDK_EXPOSE, TRUE);

						nfui_nfobject_set_data((NFOBJECT*)button, "index", GINT_TO_POINTER(start_idx));
						start_idx++;
					}

					if (combo->scroll.cur_step < combo->scroll.total_step) 
					{
						combo->scroll.cur_step++;
						nfcombobox_draw_scroll_bar(combo);
					}
				}
			}
			break;
	}

	return TRUE;
}

static void nfcombobox_create_scroll_bar(NFCOMBOBOX *combo)
{
	GdkDrawable *drawable = NULL;

	drawable = nfui_nfobject_get_window((NFOBJECT*)combo);

	if(drawable) {
		GdkPixbuf *bar[3];
		GdkGC *gc = NULL;
		gint scroll_w, scroll_h;
		gint bar_w, bar_h;

		scroll_w = combo->scroll_up->width;
		scroll_h = combo->scroll.size_h;

		combo->scroll.pb_bar = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, scroll_w, scroll_h);
		gdk_pixbuf_fill(combo->scroll.pb_bar, 0x000000);

		bar[0] = nfui_get_image_from_file((IMG_N_SCROLLBAR_TOP), NULL);
		bar[1] = nfui_get_image_from_file((IMG_N_SCROLLBAR_MID), NULL);
		bar[2] = nfui_get_image_from_file((IMG_N_SCROLLBAR_BOT), NULL);

		nfui_get_pixbuf_size(bar[0], &bar_w, &bar_h);

		gdk_pixbuf_composite(bar[0], combo->scroll.pb_bar, 0, 0, bar_w, bar_h, 0.0, 0.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
		gdk_pixbuf_composite(bar[1], combo->scroll.pb_bar, 0, bar_h, bar_w, (scroll_h - (bar_h * 2)), 0.0, 0.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
		gdk_pixbuf_composite(bar[2], combo->scroll.pb_bar, 0, (bar_h + (scroll_h - (bar_h * 2))), bar_w, bar_h, 0.0, 0.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
	}
}

static gboolean nfcombobox_draw_scroll_bar(NFCOMBOBOX *combo)
{
	GdkDrawable *drawable = NULL;
	GdkGC *bg = NULL;
	gint x, y, w, h;
	gint submenu_margin = COMBOBOX_SUBMENU_MARGIN;
	

	if(combo->scroll.cur_step < 0 || combo->scroll.cur_step > combo->scroll.total_step)
		return FALSE;

	drawable = nfui_nfobject_get_window((NFOBJECT*)combo->submenu);
	if(drawable == NULL) {
		DMSG(1, "drawable is null");
		return FALSE;
	}

	nfui_nfobject_get_offset((NFOBJECT*)combo->submenu, &x, &y);

	bg = gdk_gc_new(drawable);
	gdk_gc_set_rgb_fg_color(bg, &UX_COLOR(combo->combo_fixed.object.bg_color[NFOBJECT_STATE_NORMAL]));

	// draw bg
	x += combo->submenu->width - combo->scroll_up->width - (submenu_margin/2);
	y += combo->scroll_up->height + (submenu_margin/2);
	w = combo->scroll_up->width;
	h = combo->submenu->height - combo->scroll_up->height - combo->scroll_down->height;

	gdk_draw_rectangle(drawable, bg, TRUE, x, y, w, h);
	
	g_object_unref(bg);

	// darw bar
	if(combo->scroll.cur_step <= combo->scroll.total_step) {

		bg = gdk_gc_new(drawable);

		if(combo->scroll.cur_step == combo->scroll.total_step) {
			y = (combo->submenu->height - combo->scroll_down->height - combo->scroll.size_h);
			y += (submenu_margin/2);
		}
		else
			y += (combo->scroll.cur_step * combo->scroll.interval);

		nfutil_draw_pixbuf(drawable, bg, combo->scroll.pb_bar, x, y, w, combo->scroll.size_h, NFALIGN_LEFT, 0);

		g_object_unref(bg);
	}

	return TRUE;
}

static gboolean nfcombobox_scroll_button_cb(NFOBJECT *button, GdkEvent *event, gpointer data)
{
	NFCOMBOBOX *combo;
	//DMSG(1, "################### event->type : %d###################", event->type);

	combo = (NFCOMBOBOX*)nfui_nfobject_get_data(((NFOBJECT*)button)->parent->parent, "parent");

	switch(event->type) {
		case GDK_BUTTON_PRESS:
			{
				if(event->button.button == MOUSE_RIGTH_BUTTON) 					
					return FALSE;

				if(combo->scroll_up == button) {
					nfcombobox_scroll_menu_items(combo, SCROLL_UP); 
				}else if(combo->scroll_down == button) {
					nfcombobox_scroll_menu_items(combo, SCROLL_DOWN); 
				}

				if(!combo->timer_id)
					combo->timer_id = g_timeout_add(COMBOBOX_REPEAT_TIME, nfcombobox_submenu_scroll_cb, (gpointer)button);
			}
			break;
		case GDK_BUTTON_RELEASE:
			if(event->button.button == MOUSE_RIGTH_BUTTON) 					
				return FALSE;

				if(combo->timer_id != 0) {
					g_source_remove(combo->timer_id);
					combo->timer_id = 0;
				}
			break;
		case GDK_ENTER_NOTIFY:
			{
				//timer_id = g_timeout_add(100, nfcombobox_submenu_scroll_cb, (gpointer)button);
			}
			break;

		case GDK_LEAVE_NOTIFY:
			{
				if(combo->timer_id != 0) {
					g_source_remove(combo->timer_id);
					combo->timer_id = 0;
				}
			}
			break;	

		case GDK_DELETE:
			{
				if(combo->scroll_up == button) {
					if(combo->scroll_up)
						combo->scroll_up = NULL;
				}else if(combo->scroll_down == button)  {
					if(combo->scroll_down)
						combo->scroll_down = NULL;
				}

				if(combo->scroll.pb_bar)
				{
					g_object_unref(combo->scroll.pb_bar);
					combo->scroll.pb_bar = NULL;
				}
			}
			break;

		default:
			break;
	}

	return FALSE;
}


NFOBJECT* nfui_combobox_new(gchar *data[], gint cnt, gint init)
{
	NFCOMBOBOX *combo;
	gint i;

	g_return_val_if_fail(cnt >= 0, NULL);

	combo = (NFCOMBOBOX*)imalloc(sizeof(NFCOMBOBOX));
	if(!combo) {	
		DMSG(1, "NFComboBox alloc error...");
		return NULL;
	}

#if 0
	combo->combo_fixed.object.parent = NULL;
	combo->combo_fixed.object.x = 0;
	combo->combo_fixed.object.y = 0;
	combo->combo_fixed.object.width = 140;
	combo->combo_fixed.object.height = 22;
	combo->combo_fixed.object.type = NFOBJECT_TYPE_NFCOMBOBOX;
	combo->combo_fixed.object.show = NFOBJECT_HIDE;
	combo->combo_fixed.object.use_focus = NFOBJECT_FOCUS_ON;
	combo->combo_fixed.object.kfocus = NFOBJECT_UNFOCUS;
	combo->combo_fixed.object.mfocus = NFOBJECT_UNFOCUS;
	combo->combo_fixed.object.bg_color[NFOBJECT_STATE_NORMAL] = -1;
	combo->combo_fixed.object.bg_color[NFOBJECT_STATE_PRELIGHT] = -1;
	combo->combo_fixed.object.bg_color[NFOBJECT_STATE_ACTIVE] = -1;
	combo->combo_fixed.object.pre_event_handler = NULL;
	combo->combo_fixed.object.default_event_handler = nfcombobox_event_handler;
	combo->combo_fixed.object.post_event_handler = NULL;
#else
	nfui_nfobject_init((NFOBJECT*)combo);

	combo->combo_fixed.object.width = 140;
	combo->combo_fixed.object.height = 22;
	combo->combo_fixed.object.type = NFOBJECT_TYPE_NFCOMBOBOX;
	combo->combo_fixed.object.use_focus = NFOBJECT_FOCUS_ON;
	combo->combo_fixed.object.default_event_handler = nfcombobox_event_handler;
#endif
	
	//combo->menu_color = NULL;
	combo->sensitive = TRUE;
	combo->icon = NULL;

	/* label */
	combo->label = (NFOBJECT*)nfui_nflabel_new("");
	nfui_nflabel_set_drawing_outline((NFLABEL*)(combo->label), FALSE);
	nfui_nflabel_set_align((NFLABEL*)combo->label, COMBOBOX_TEXT_ALIGN, COMBOBOX_TEXT_LEFT_MARGIN);
	nfui_regi_post_event_callback(combo->label, (gpointer)nfcombobox_label_cb);
	nfui_nfobject_show(combo->label); 

	/* button */
	combo->button = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfobject_use_focus(combo->button, NFOBJECT_FOCUS_ON);
	nfui_regi_post_event_callback(combo->button, (gpointer)nfcombobox_button_cb);
	nfui_nfobject_show(combo->button); 

	// SKSHIN
	nfcombobox_set_child_position(combo);

	if(data != NULL) {
		for(i=0; i<cnt; i++) {
			if(data[i])
				combo->data = g_slist_append(combo->data, g_strdup(data[i]));
		}
	}else {
		combo->data = NULL;
	}

	for(i=0; i<MAX_COLORMAP; i++)
	{
		combo->colormap[i] = -1;
	}

	combo->index = init;
	combo->align = NFALIGN_CENTER;
	combo->margin = 0;
	combo->spacing_type = NORMAL_SPACING;
	combo->unchange_string = FALSE;
	combo->skin_type = NFCOMBOBOX_TYPE_UNDEF;
	combo->display_cnt = -1;

	return (NFOBJECT*)combo;
}

void nfui_combobox_set_skin_type(NFCOMBOBOX *combo, NFCOMBOBOX_TYPE type)
{
	g_return_val_if_fail(combo != NULL, NULL);

	if(combo->combo_fixed.object.type != NFOBJECT_TYPE_NFCOMBOBOX)
	{
		return NULL;
	}

	if (type == NFCOMBOBOX_TYPE_1)
		_set_combobox_type_1(combo);
	else if (type == NFCOMBOBOX_TYPE_2)
		_set_combobox_type_2(combo);
	else if (type == NFCOMBOBOX_TYPE_POPUP_1)
		_set_combobox_type_popup_1(combo);
	else if (type == NFCOMBOBOX_TYPE_POPUP_2)
		_set_combobox_type_popup_2(combo);		
	else if (type == NFCOMBOBOX_TYPE_SUBTAB_1)
		_set_combobox_type_subtab_1(combo);		
	else if (type == NFCOMBOBOX_TYPE_SUBTAB_2)
		_set_combobox_type_subtab_2(combo);				
	else if (type == NFCOMBOBOX_TYPE_POPUP_SMALL_1)
		_set_combobox_type_popup_small_1(combo);
	else if (type == NFCOMBOBOX_TYPE_POPUP_SMALL_2)
		_set_combobox_type_popup_small_2(combo);

	nfcombobox_change_child_position(combo);
}

gchar* nfui_combobox_get_value(NFCOMBOBOX *combo)
{
	g_return_val_if_fail(combo != NULL, NULL);
	g_return_val_if_fail(combo->data != NULL, NULL);

	if(combo->combo_fixed.object.type != NFOBJECT_TYPE_NFCOMBOBOX)
	{
		return NULL;
	}

	if(combo->index < 0)	return NULL;

	return (gchar*)g_slist_nth_data(combo->data, (guint)combo->index);
}

gboolean nfui_combobox_prepend_data(NFCOMBOBOX *combo, gchar *text)
{
	g_return_val_if_fail(combo != NULL, FALSE);
	g_return_val_if_fail(text != NULL, FALSE);

	if(combo->combo_fixed.object.type != NFOBJECT_TYPE_NFCOMBOBOX)
	{
		return FALSE;
	}

	combo->data = g_slist_prepend(combo->data, g_strdup(text));
	
	nfcombobox_reset_submenu(combo);

	return TRUE;
}

gboolean nfui_combobox_append_data(NFCOMBOBOX *combo, gchar *text)
{
	GSList *list;

	g_return_val_if_fail(combo != NULL, FALSE);
	g_return_val_if_fail(text != NULL, FALSE);

	if(combo->combo_fixed.object.type != NFOBJECT_TYPE_NFCOMBOBOX)
	{
		return FALSE;
	}

	if(combo->data == NULL)
		combo->index = 0;

	combo->data = g_slist_append(combo->data, g_strdup(text));
	list = g_slist_last(combo->data);

	nfcombobox_reset_submenu(combo);

	return TRUE;
}


gint nfui_combobox_get_cur_index(NFCOMBOBOX *combo)
{
	g_return_val_if_fail(combo != NULL, -1);

	if(combo->combo_fixed.object.type != NFOBJECT_TYPE_NFCOMBOBOX)
	{
		return -1;
	}

	return combo->index;
}

void nfui_combobox_set_data(NFCOMBOBOX *combo, const gchar *text)
{	
	guint len;
	guint i;

	g_return_if_fail(combo != NULL);
	g_return_if_fail(combo->data != NULL);

	if(combo->combo_fixed.object.type != NFOBJECT_TYPE_NFCOMBOBOX)
	{
		return;
	}

	len = g_slist_length(combo->data);

	for(i=0; i<len; i++)
	{
		if(!strcmp(text, g_slist_nth_data(combo->data, i)))
			break;
	}

	if(i != len)
	{
		combo->index = i;
		nfui_signal_emit((NFOBJECT*)combo, GDK_EXPOSE, TRUE);
	}
	else
	{
		DMSG(1, "[ERROR] No matched data!!\n");
		g_assert(0);		
	}
}

void nfui_combobox_set_data_no_expose(NFCOMBOBOX *combo, const gchar *text)
{	
	guint len;
	guint i;

	g_return_if_fail(combo != NULL);
	g_return_if_fail(combo->data != NULL);

	if(combo->combo_fixed.object.type != NFOBJECT_TYPE_NFCOMBOBOX)
	{
		return;
	}

	len = g_slist_length(combo->data);

	for(i=0; i<len; i++)
	{
		if(!strcmp(text, g_slist_nth_data(combo->data, i)))
			break;
	}

	if(i != len)
		combo->index = i;
	else
	{
		DMSG(1, "set data : %s", text);

		for(i = 0; i < len; i++)
			DMSG(1, "vaild data : %s", g_slist_nth_data(combo->data, i));
	
		DMSG(1, "[ERROR] No matched data!!\n");
		g_assert(0);
	}
}

void nfui_combobox_set_index(NFCOMBOBOX *combo, guint idx)
{
	guint len;

	g_return_if_fail(combo != NULL);
	g_return_if_fail(combo->data != NULL);

	if(combo->combo_fixed.object.type != NFOBJECT_TYPE_NFCOMBOBOX)
	{
		return;
	}

	len = g_slist_length(combo->data);

	if(idx >= len)
	{
		g_printf("[ERROR] index > length of list!\n");
		return;
	}

	combo->index = idx;

	nfui_signal_emit((NFOBJECT*)combo, GDK_EXPOSE, FALSE);

	// added by hakeya. 2008-10-23
	nfui_user_signal_emit((NFOBJECT*)combo, NFEVENT_COMBOBOX_CHANGED, FALSE);
}

void nfui_combobox_set_index_no_expose(NFCOMBOBOX *combo, guint idx)
{
	guint len;

	g_return_if_fail(combo != NULL);
	g_return_if_fail(combo->data != NULL);

	if(combo->combo_fixed.object.type != NFOBJECT_TYPE_NFCOMBOBOX)
	{
		return;
	}

	len = g_slist_length(combo->data);

	if(idx >= len)
	{
		g_printf("[ERROR] index > length of list!\n");
		return;
	}

	combo->index = idx;
}

void nfui_combobox_remove_data(NFCOMBOBOX *combo, const gchar *data)
{
	guint cnt;
	guint i;
	gchar *strTemp;

	g_return_if_fail(combo != NULL);
	g_return_if_fail(combo->data != NULL);

	if(combo->combo_fixed.object.type != NFOBJECT_TYPE_NFCOMBOBOX)
	{
		return;
	}

	cnt = g_slist_length(combo->data);

	for(i=0; i<cnt; i++) {
		strTemp = (gchar*)g_slist_nth_data(combo->data, i);
		if(!strcmp(strTemp, data)) {
			g_free(strTemp);
			strTemp = NULL;

			// FIXME
			combo->data = g_slist_delete_link(combo->data, g_slist_nth(combo->data, i));

			if(g_slist_length(combo->data) == 0)			
				combo->index = -1;
			else
				combo->index = 0;

			if(combo->index < 0) {
				nfui_nflabel_set_text((NFLABEL*)(combo->label), "");
				nfui_signal_emit(combo->label, GDK_EXPOSE, FALSE);
			}

			return;
		}
	}

	nfcombobox_reset_submenu(combo);

}

void nfui_combobox_remove_all(NFCOMBOBOX *combo)
{
	guint cnt;
	guint i;
	gchar *strTemp;

	g_return_if_fail(combo != NULL);
	g_return_if_fail(combo->data != NULL);


	if(combo->combo_fixed.object.type != NFOBJECT_TYPE_NFCOMBOBOX)
	{
		return;
	}

	cnt = g_slist_length(combo->data);
	if (cnt == 0) return;

	for(i=0; i<cnt; i++)
	{
		strTemp = (gchar*)g_slist_nth_data(combo->data, i);
		g_free(strTemp);
		strTemp = NULL;
	}

	if (combo->data)
		g_slist_free(combo->data);

	combo->data = NULL;
	combo->index = -1;

	if (combo->item)
		g_slist_free(combo->item);

	combo->item = NULL;


	nfui_nflabel_set_text((NFLABEL*)(combo->label), "");
	nfui_signal_emit(combo->label, GDK_EXPOSE, FALSE);

	nfcombobox_reset_submenu(combo);

}

#if 0
void nfui_combobox_set_menu_color(NFCOMBOBOX *combo, GdkColor *color)
{
	if(combo->combo_fixed.object.type != NFOBJECT_TYPE_NFCOMBOBOX)
	{
		return;
	}

	if(combo->menu_color)
		gdk_color_free(combo->menu_color);

	combo->menu_color = gdk_color_copy(color);
	gtk_widget_modify_bg(combo->popup_widget, GTK_STATE_NORMAL, combo->menu_color);
}
#endif

void nfui_combobox_sensitive(NFCOMBOBOX *combo, gboolean sensitive)
{
	g_return_if_fail(combo != NULL); 
	
	if(combo->combo_fixed.object.type != NFOBJECT_TYPE_NFCOMBOBOX)
	{
		return;
	}

	if(combo->sensitive != sensitive)	
		combo->sensitive = sensitive;
}

void nfui_combobox_set_align(NFCOMBOBOX *combo, nfalign_type align, gint margin)
{
	if(combo->combo_fixed.object.type != NFOBJECT_TYPE_NFCOMBOBOX)
	{
		return;
	}

	combo->align = align;
	combo->margin = margin;

	nfui_nflabel_set_align((NFLABEL*)(combo->label), align, margin);
}

void nfui_combobox_set_pango_font(NFCOMBOBOX *combo, gchar *pfont)
{
	g_return_if_fail(pfont);

	if(combo->combo_fixed.object.type != NFOBJECT_TYPE_NFCOMBOBOX)
	{
		return;
	}

	if (combo->skin_type == NFCOMBOBOX_TYPE_UNDEF) g_assert(0);

	if (combo->skin_type == NFCOMBOBOX_TYPE_1)
		nfui_nflabel_set_pango_font((NFLABEL*)(combo->label), pfont, COLOR_IDX(149));
	else if (combo->skin_type == NFCOMBOBOX_TYPE_2)
		nfui_nflabel_set_pango_font((NFLABEL*)(combo->label), pfont, COLOR_IDX(163));
	else if ((combo->skin_type == NFCOMBOBOX_TYPE_POPUP_1) || (combo->skin_type == NFCOMBOBOX_TYPE_POPUP_SMALL_1))
		nfui_nflabel_set_pango_font((NFLABEL*)(combo->label), pfont, COLOR_IDX(241));
	else if ((combo->skin_type == NFCOMBOBOX_TYPE_POPUP_2) || (combo->skin_type == NFCOMBOBOX_TYPE_POPUP_SMALL_2))
		nfui_nflabel_set_pango_font((NFLABEL*)(combo->label), pfont, COLOR_IDX(255));		
	else if (combo->skin_type == NFCOMBOBOX_TYPE_SUBTAB_1)
		nfui_nflabel_set_pango_font((NFLABEL*)(combo->label), pfont, COLOR_IDX(931));
	else if (combo->skin_type == NFCOMBOBOX_TYPE_SUBTAB_2)
		nfui_nflabel_set_pango_font((NFLABEL*)(combo->label), pfont, COLOR_IDX(945));		
		
}

void nfui_combobox_set_menu_pango_font(NFCOMBOBOX *combo, gchar *pfont)
{
	gint color[NFOBJECT_STATE_COUNT];	

	if(combo->combo_fixed.object.type != NFOBJECT_TYPE_NFCOMBOBOX)
	{
		return;
	}

	if (combo->skin_type == NFCOMBOBOX_TYPE_UNDEF) g_assert(0);

	if (combo->skin_type == NFCOMBOBOX_TYPE_1)
	{
		color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(155);
		color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(159);
		color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(159);
		color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(157);
	}
	else if (combo->skin_type == NFCOMBOBOX_TYPE_2)
	{
		color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(169);
		color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(173);
		color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(173);
		color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(171);
	}
	else if ((combo->skin_type == NFCOMBOBOX_TYPE_POPUP_1) || (combo->skin_type == NFCOMBOBOX_TYPE_POPUP_SMALL_1))
	{
		color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(247);
		color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(251);
		color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(251);
		color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(249);
	}
	else if ((combo->skin_type == NFCOMBOBOX_TYPE_POPUP_2) || (combo->skin_type == NFCOMBOBOX_TYPE_POPUP_SMALL_2))
	{
    	color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(261);
    	color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(265);
    	color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(265);
    	color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(263);
	}
	else if (combo->skin_type == NFCOMBOBOX_TYPE_SUBTAB_1)
	{
    	color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(937);
    	color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(941);
    	color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(941);
    	color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(939);
	}
	else if (combo->skin_type == NFCOMBOBOX_TYPE_SUBTAB_2)
	{
    	color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(951);
    	color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(955);
    	color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(955);
    	color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(953);
	}

	nfui_nfbutton_set_pango_font(((NFBUTTON*)combo->button), pfont, color);
}

void nfui_combobox_use_pango_cashing(NFCOMBOBOX *combo, gint cashing, gchar *key)
{
	if(combo->combo_fixed.object.type != NFOBJECT_TYPE_NFCOMBOBOX)
	{
		return;
	}

	nfui_nflabel_use_pango_cashing((NFLABEL*)(combo->label), cashing, key);
}

void nfui_combobox_set_spacing(NFCOMBOBOX *combo, nfutil_pango_spacing_type spacing_type)
{
	if(combo->combo_fixed.object.type != NFOBJECT_TYPE_NFCOMBOBOX)
	{
		return;
	}

	combo->spacing_type = spacing_type;
}

gboolean nfui_combobox_set_icon_image(NFCOMBOBOX *combo, gchar *path)
{
	g_return_val_if_fail(path != NULL, FALSE);

	if(combo->combo_fixed.object.type != NFOBJECT_TYPE_NFCOMBOBOX)
	{
		return FALSE;
	}

	combo->icon = nfui_get_image_from_file(path, NULL); 

	if(combo->icon == NULL) {
		DMSG(1, "%s was not found..", path);

		return FALSE;
	}

	return TRUE;
}

void nfui_combobox_set_display_string(NFCOMBOBOX *combo, gchar *str)
{
	if(combo->combo_fixed.object.type != NFOBJECT_TYPE_NFCOMBOBOX)
	{
		return;
	}

	if(combo->label) {
		nfui_nflabel_set_text((NFLABEL*)(combo->label), str);

		combo->unchange_string = TRUE;
	}
}

NFOBJECT *nfui_combobox_get_item_object(NFCOMBOBOX *combo, gint item_idx)
{
	NFOBJECT *item;

	if(combo->combo_fixed.object.type != NFOBJECT_TYPE_NFCOMBOBOX)
	{
		return 0;
	}

	if (item_idx >= g_slist_length(combo->item)) return 0;

	item = (NFOBJECT*)g_slist_nth_data(combo->item, item_idx);
	return item;
}

gint nfui_combobox_set_multi_color(NFCOMBOBOX *combo, gint color_idx, gint *fg_color, gint *bg_color)
{
	if(combo->combo_fixed.object.type != NFOBJECT_TYPE_NFCOMBOBOX) return -1;
	if (color_idx >= MAX_COLORIDX) return -1;

	combo->multi_fgcolor[color_idx][0] = fg_color[0];
	combo->multi_fgcolor[color_idx][1] = fg_color[1];
	combo->multi_fgcolor[color_idx][2] = fg_color[2];
	combo->multi_fgcolor[color_idx][3] = fg_color[3];

	combo->multi_bgcolor[color_idx][0] = bg_color[0];
	combo->multi_bgcolor[color_idx][1] = bg_color[1];
	combo->multi_bgcolor[color_idx][2] = bg_color[2];
	combo->multi_bgcolor[color_idx][3] = bg_color[3];
	
	return 0;
}

gint nfui_combobox_set_colormap(NFCOMBOBOX *combo, gint item_idx, gint color_idx)
{
	NFOBJECT *button;
	guint i = 0;
	
	if(combo->combo_fixed.object.type != NFOBJECT_TYPE_NFCOMBOBOX) return -1;
	if (item_idx >= MAX_COLORMAP) return -1;
	if (color_idx >= MAX_COLORIDX) return -1;

	combo->colormap[item_idx] = color_idx;

	while(1) 
	{
		button = g_slist_nth_data(combo->item, i);
		if(button == NULL) break;

		if (item_idx == GPOINTER_TO_INT(nfui_nfobject_get_data((NFOBJECT*)button, "index")))
		{
			nfui_nfbutton_set_pango_font((NFBUTTON*)button, ((NFLABEL*)(combo->label))->pango_font, combo->multi_fgcolor[color_idx]);			
			nfui_nfbutton_set_bg_color((NFBUTTON*)button, combo->multi_bgcolor[color_idx]);
			break;
		}

		i++;
	}
	
	return 0;
}

gint nfui_combobox_set_display_count(NFCOMBOBOX *combo, gint cnt)
{
	if(combo->combo_fixed.object.type != NFOBJECT_TYPE_NFCOMBOBOX) return -1;

	combo->display_cnt = cnt;
	return 0;
}
