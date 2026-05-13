
#ifndef __NF_LISTBOX_H__
#define __NF_LISTBOX_H__

/******************************************************************
 *
 *  NFLISTBOX 
 *
 *
 *****************************************************************/



#include "nfobject.h"
#include "nffixed.h"

G_BEGIN_DECLS

#define NF_LISTBOX(widget)      ((NFLISTBOX*)widget)

#define NFLBOX_FONT_STRING_SIZE (64)
#define NFLBOX_CASHING_KEY_SIZE (64)

#define SUPP_MAX_COLS           (20)

#define LISTBOX_COLOR_KEY       ".color_key"
#define LISTBOX_IMG_KEY         ".png"

typedef enum {
    SCROLL_UP,
    SCROLL_DOWN
} NFScrollType;

typedef enum {
    NFLISTBOX_TYPE_UNDEF        = 0,
    NFLISTBOX_TYPE_1            = 1,
    NFLISTBOX_TYPE_2            = 2,
    NFLISTBOX_TYPE_POPUP_1      = 3,
    NFLISTBOX_TYPE_POPUP_2      = 4,
    NFLISTBOX_TYPE_SUBTAB_1     = 5,
    NFLISTBOX_TYPE_SUBTAB_2     = 6,    
    NFLISTBOX_TYPE_MAX
} NFLISTBOX_TYPE;

typedef struct _NFLISTBOX           NFLISTBOX;
typedef struct _NFLISTBOXINFO       NFLISTBOXINFO;
typedef struct _SCROLL              SCROLL;

struct _SCROLL {
    GdkPixbuf *scroll;
    GdkPixbuf *normal;
    GdkPixbuf *focus;
    GdkPixbuf *active;  
    gint width;
    gint size;
    gint color;

    gint total_step;
    gint cur_step;

    gint interval;
    gint cur_pos;

    float pps;
};

struct _NFLISTBOX {
    NFFIXED         lbox_fixed;

    NFOBJECT        *up_btn;
    NFOBJECT        *down_btn;

    GSList          *slist;
    NFLISTBOXINFO   *boxInfo;
    SCROLL          st_scroll;
    
    GdkPixmap       *normal_reservedscr;
    GdkPixmap       *disable_reservedscr;
    GdkPixmap       *normal_reservedrow;
    GdkPixmap       *active_reservedrow;

    guint           update_delay_timer;
    gint            update_delete_cnt;
    gint            press_scroll;
    
    gint            skin_type;
    guint           font_color[NFOBJECT_STATE_COUNT];
    guint           bg_color[NFOBJECT_STATE_COUNT];
    
    guint           column;
    guint           *col_w;
    guint           row_height;
    gint            box_row_cnt;
    gint            text_row_cnt;
    
    gchar           *pango_font;
    gchar           font_name[NFLBOX_FONT_STRING_SIZE];
    nfalign_type    align[SUPP_MAX_COLS];           
    gboolean        multi_lang; 
    
    gboolean        draw_inline;
    guint           inline_color;
    gboolean        draw_outline;
    gboolean        use_infocus;
    
    gint            update_data;    
	guint           tmr_tooltip;
	gboolean        use_tooltip;
};


struct _NFLISTBOXINFO {
    GdkRectangle    col_rect[SUPP_MAX_COLS];
    gboolean        focus;
    gint            index;
};

NFOBJECT* nfui_listbox_new(guint column, guint *col_size, guint height);
void nfui_listbox_set_skin_type(NFLISTBOX *lbox, NFLISTBOX_TYPE type);
void nfui_listbox_set_arrow_image(NFLISTBOX *lbox, NFScrollType type, GdkPixbuf **image, guint width, guint height);
void nfui_listbox_set_pango_font(NFLISTBOX *lbox, const gchar *pfont);
void nfui_listbox_set_fg_color(NFLISTBOX *lbox, gint *fg_color);
void nfui_listbox_set_bg_color(NFLISTBOX *lbox, gint *bg_color);
void nfui_listbox_set_column_bg_color(NFLISTBOX *lbox, gint idx, gint bg_color);
void nfui_listbox_set_draw_inline(NFLISTBOX *lbox, gboolean draw, guint colorIdx);
gint nfui_listbox_set_column_align(NFLISTBOX *lbox, gint col, nfalign_type align);
void nfui_listbox_support_multi_lang(NFLISTBOX *lbox, gboolean support);
void nfui_listbox_support_tooltip(NFLISTBOX *lbox, gboolean support);
void nfui_listbox_set_text(NFLISTBOX *lbox, gchar *str[]);
void nfui_listbox_set_prepend_text(NFLISTBOX *lbox, gchar *str[]);
void nfui_listbox_set_text_single_column(NFLISTBOX *lbox, gchar *str);
void nfui_listbox_modify_text_by_index(NFLISTBOX *lbox, gchar *str[], guint index);
void nfui_listbox_set_focus(NFLISTBOX *lbox, guint index, gboolean focus);
void nfui_listbox_delete(NFLISTBOX *lbox, guint idx);
void nfui_listbox_delete_all(NFLISTBOX *lbox);
gchar* nfui_listbox_get_focus_text(NFLISTBOX *lbox, gint col);
gint nfui_listbox_get_focus_idx(NFLISTBOX *lbox);
gint nfui_listbox_get_index_by_box_row(NFLISTBOX *lbox, gint box_row);
gint nfui_listbox_get_box_count(NFLISTBOX *lbox);
gchar* nfui_listbox_get_text_of_list(NFLISTBOX *lbox, gint row, gint col);
gchar* nfui_listbox_get_text_of_box(NFLISTBOX *lbox, gint row, gint col);
void nfui_listbox_set_drawing_outline(NFLISTBOX *lbox, gboolean draw_outline);
void nfui_listbox_set_use_infocus_box(NFLISTBOX *lbox, gboolean use);

G_END_DECLS

#endif  // __NF_LISTBOX_H__
