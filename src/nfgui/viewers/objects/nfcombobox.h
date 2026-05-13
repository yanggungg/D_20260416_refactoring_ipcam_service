


#ifndef __NF_COMBO_BOX_H__
#define __NF_COMBO_BOX_H__

/******************************************************************
 *
 *  NFCOMBOBOX 
 *
 *
 *****************************************************************/


#include "../../support/util.h"

#include "nfobject.h"
#include "nffixed.h"


G_BEGIN_DECLS

#define NF_COMBOBOX(widget)     ((NFCOMBOBOX*)widget)

#define MAX_COLORMAP    (128)
#define MAX_COLORIDX        (16)

typedef struct _NFCOMBOBOX      NFCOMBOBOX;
typedef struct _SCROLLBAR       SCROLLBAR;

struct _SCROLLBAR {
    GdkPixbuf *pb_bar;

    gint interval;
    gint size_h;

    gint total_step;
    gint cur_step;
};

struct _NFCOMBOBOX {
    NFFIXED combo_fixed;

    NFOBJECT *button;
    NFOBJECT *label;
    NFOBJECT *submenu;
    NFOBJECT *scroll_up;
    NFOBJECT *scroll_down;

    SCROLLBAR scroll;

    GdkGC *bg_gc;
    
    //GdkColor *menu_color;

    GSList *data;
    GSList *item;

    gint colormap[MAX_COLORMAP];
    gint multi_fgcolor[MAX_COLORIDX][NFOBJECT_STATE_COUNT];
    gint multi_bgcolor[MAX_COLORIDX][NFOBJECT_STATE_COUNT];

    GdkPixbuf *icon;

    gint index;
    guint timer_id;
    gboolean sensitive;

    nfalign_type align;
    gint margin;

    gint display_cnt;

    gboolean unchange_string;

    nfutil_pango_spacing_type spacing_type;
    gint skin_type;
};

typedef enum {
    NFCOMBOBOX_TYPE_UNDEF           = 0,
    NFCOMBOBOX_TYPE_1               = 1,
    NFCOMBOBOX_TYPE_2               = 2,
    NFCOMBOBOX_TYPE_POPUP_1         = 3,
    NFCOMBOBOX_TYPE_POPUP_2         = 4,
    NFCOMBOBOX_TYPE_SUBTAB_1        = 5,
    NFCOMBOBOX_TYPE_SUBTAB_2        = 6,    
    NFCOMBOBOX_TYPE_POPUP_SMALL_1   = 7,
    NFCOMBOBOX_TYPE_POPUP_SMALL_2   = 8,    
    NFCOMBOBOX_TYPE_MAX,
} NFCOMBOBOX_TYPE;


NFOBJECT* nfui_combobox_new(gchar *data[], gint cnt, gint init);
void nfui_combobox_set_skin_type(NFCOMBOBOX *combo, NFCOMBOBOX_TYPE type);
gchar* nfui_combobox_get_value(NFCOMBOBOX *combo);

gboolean nfui_combobox_prepend_data(NFCOMBOBOX *combo, gchar *text);
gboolean nfui_combobox_append_data(NFCOMBOBOX *combo, gchar *text);
gint nfui_combobox_get_cur_index(NFCOMBOBOX *combo);

void nfui_combobox_set_data(NFCOMBOBOX *combo, const gchar *text);
void nfui_combobox_set_data_no_expose(NFCOMBOBOX *combo, const gchar *text);
void nfui_combobox_set_index(NFCOMBOBOX *combo, guint idx);
void nfui_combobox_set_index_no_expose(NFCOMBOBOX *combo, guint idx);

void nfui_combobox_remove_data(NFCOMBOBOX *combo, const gchar *data);
void nfui_combobox_remove_all(NFCOMBOBOX *combo);

//void nfui_combobox_set_menu_color(NFCOMBOBOX *combo, GdkColor *color);

void nfui_combobox_sensitive(NFCOMBOBOX *combo, gboolean sensitive);

void nfui_combobox_set_align(NFCOMBOBOX *combo, nfalign_type align, gint margin);
void nfui_combobox_set_pango_font(NFCOMBOBOX *combo, gchar *pfont);
void nfui_combobox_set_menu_bg_color(NFCOMBOBOX *combo, gchar *pfont);
gboolean nfui_combobox_set_icon_image(NFCOMBOBOX *combo, gchar *path);

void nfui_combobox_use_pango_cashing(NFCOMBOBOX *combo, gint cashing, gchar *key);
void nfui_combobox_set_spacing(NFCOMBOBOX *combo, nfutil_pango_spacing_type spacing_type);
void nfui_combobox_set_display_string(NFCOMBOBOX *combo, gchar *str);
NFOBJECT *nfui_combobox_get_item_object(NFCOMBOBOX *combo, gint item_idx);
gint nfui_combobox_set_multi_color(NFCOMBOBOX *combo, gint color_idx, gint *fg_color, gint *bg_color);
gint nfui_combobox_set_colormap(NFCOMBOBOX *combo, gint item_idx, gint color_idx);
gint nfui_combobox_set_display_count(NFCOMBOBOX *combo, gint cnt);

G_END_DECLS


#endif  // __NF_COMBO_BOX_H__
