#ifndef __NFOBJECT_H__
#define __NFOBJECT_H__

#include "nf_afx.h"
#if 0
#define IS_VALID_OBJECT(a)  ((a->type>NFOBJECT_TYPE_NONE && a->type<NUM_NFOBJECT_TYPES) ? 1:0)
#endif

#define IS_VALID_OBJECT(a)  (nfui_nfobject_is_valid(a) ? 1:0)
#define NFOBJECT_MAGIC_NUMBER   (73275)

#define MAX_TOOLTIP_SIZE        (512)

typedef enum {
    NFOBJECT_TYPE_NONE = 0,

    NFOBJECT_TYPE_TOP,
    NFOBJECT_TYPE_NFCALENDAR,
    NFOBJECT_TYPE_NFCOMBOBOX,
    NFOBJECT_TYPE_NFFIXED,
    NFOBJECT_TYPE_NFLISTBOX,
    NFOBJECT_TYPE_NFSPINBUTTON,
    NFOBJECT_TYPE_NFTABLE,
    NFOBJECT_TYPE_NFTIMESPIN,
    NFOBJECT_TYPE_NFTIMELABEL,

    NFOBJECT_TYPE_NFBUTTON,
    NFOBJECT_TYPE_NFCHECKBUTTON,
    NFOBJECT_TYPE_NFHSCALE,
    NFOBJECT_TYPE_NFIMAGE,
    NFOBJECT_TYPE_NFIMGLABEL,
    NFOBJECT_TYPE_NFLOGLABEL,
    NFOBJECT_TYPE_NFTHUMBNAIL,
    NFOBJECT_TYPE_NFIPEDITOR,
    NFOBJECT_TYPE_NFLABEL,
    NFOBJECT_TYPE_NFTAB,
    NFOBJECT_TYPE_NFTILE,
    NFOBJECT_TYPE_NFTIMELINE,
    NFOBJECT_TYPE_NFTIMELINE2,
    NFOBJECT_TYPE_NFPROGRESSBAR,
    NFOBJECT_TYPE_CWCALENDAR,
    NFOBJECT_TYPE_IXTIMELINE,
    NFOBJECT_TYPE_CWSLIDER,
    NFOBJECT_TYPE_CWMAINMENU,
    NFOBJECT_TYPE_CWSUBMENU,
    NFOBJECT_TYPE_DRAWAREA,
    NFOBJECT_TYPE_NFVKLABEL,

	NFOBJECT_TYPE_NFSCROLLEDFIXED,
    NFOBJECT_TYPE_NFPIECHART,
    NFOBJECT_TYPE_NFBARGRAPH,
    NFOBJECT_TYPE_NFANALOGCLOCK,
    NFOBJECT_TYPE_CWHCALENDAR,

//  NFOBJECT_TYPE_REAL,
    NUM_NFOBJECT_TYPES
} nfobject_type;

typedef enum {
    NFOBJECT_STATE_NORMAL = 0,
    NFOBJECT_STATE_PRELIGHT,
    NFOBJECT_STATE_ACTIVE,
    NFOBJECT_STATE_DISABLE,
    NFOBJECT_STATE_COUNT
} nfobject_state;

enum {
    NFOBJECT_HIDE = 0,
    NFOBJECT_SHOW
};

enum {
    NFOBJECT_FOCUS_OFF = 0,
    NFOBJECT_FOCUS_ON
};

enum {
    NFOBJECT_HIERARCHY_OFF = 0,
    NFOBJECT_HIERARCHY_ON
};

enum {
    NFOBJECT_UNFOCUS = 0,
    NFOBJECT_FOCUS,
};

typedef enum {
    KEY_CODE_ENTER          = 0x24,
    KEY_CODE_ESCAPE         = 0x09,
    KEY_CODE_ARROW_UP       = 0xc6,
    KEY_CODE_ARROW_LEFT     = 0xc8,
    KEY_CODE_ARROW_RIGHT    = 0xc9,
    KEY_CODE_ARROW_DOWN     = 0xcc,
} KEYCODE;



typedef enum {
    NFSTY_TRANSPARENT_BG    = 0x00000001,
    NFSTY_NOOUTLINE         = 0x00000002,
} NFSTYLE;  



//#define   KEYNAV_TABLE_COLUMNS    32
//#define   KEYNAV_TABLE_ROWS       32

#define KEYNAV_TABLE_COLUMNS    120
#define KEYNAV_TABLE_ROWS       80

typedef struct _KEY_OBJECT_T    KEYOBJECT;
typedef struct _NFOBJECT_T  NFOBJECT;

struct _KEY_OBJECT_T {
    KEYOBJECT* parent;
    GSList *children;

    NFOBJECT* object;
    guint depth;
    KEYOBJECT* nav_table[KEYNAV_TABLE_ROWS][KEYNAV_TABLE_COLUMNS];
    guint cnt_x;
    guint cnt_y;
    gint fx;    // x of focused child
    gint fy;    // y of focused child
};

struct _NFOBJECT_T {
    guint magic;
    NFOBJECT* parent;

    gint x;
    gint y;
    gint width;
    gint height;
    nfobject_type type;

    nfobject_state status;

    gint bg_color[NFOBJECT_STATE_COUNT];

    guint show;

    guint use_focus;
    guint use_hierarchy;
    guint kfocus;
    guint mfocus;
    gboolean grab_kfocus;

    gint use_tooltip;
    gchar strTooltip[MAX_TOOLTIP_SIZE];

    gboolean multi_lang;

//  gboolean (*key_nav_func)(KEYOBJECT *key_obj, guint dir);

    gboolean (*pre_event_handler)(NFOBJECT *obj, GdkEvent *event, gpointer data);
    gboolean (*default_event_handler)(NFOBJECT *obj, GdkEvent *event, gpointer data);
    gboolean (*post_event_handler)(NFOBJECT *obj, GdkEvent *event, gpointer data);

    void (*tooltip_info)(NFOBJECT *obj, gchar *str, gint *off_x, gint *off_y);

    GSList *user_data;

};


NFOBJECT *nfui_nfobject_new();
void nfui_nfobject_destroy(NFOBJECT *obj);
void nfui_nfobject_free(NFOBJECT* obj);
void nfui_nfobject_init(NFOBJECT* obj);
gboolean nfui_nfobject_is_valid(NFOBJECT *obj);
void nfui_nfobject_show(NFOBJECT *obj);
void nfui_nfobject_hide(NFOBJECT *obj);
GdkWindow *nfui_nfobject_get_window(NFOBJECT *obj);
GdkGC* nfui_nfobject_get_gc(NFOBJECT *obj);
#if 0
    void nfui_nfobject_gc_unref(GdkGC *gc);
#else
    #define nfui_nfobject_gc_unref(x)   ;
#endif

NFOBJECT *nfui_nfobject_get_top(NFOBJECT *obj);
NFOBJECT *nfui_nfobject_get_scrolledfixed(NFOBJECT *obj);
gint nfui_nfobject_is_scrolledfixed_child(NFOBJECT *obj);

void nfui_nfobject_get_offset(NFOBJECT *obj, gint *off_x, gint *off_y);
void nfui_nfobject_modify_bg(NFOBJECT* obj, nfobject_state state, int color_idx);
void nfui_nfobject_set_size(NFOBJECT* obj, gint width, gint height);

gint nfui_nfobject_get_bg_color(NFOBJECT* obj, nfobject_state state);


void nfui_nfobject_set_data(NFOBJECT *obj, const gchar *name, gpointer data);
void nfui_nfobject_set_alloc_data(NFOBJECT *obj, const gchar *name, gpointer data);
gpointer nfui_nfobject_get_data(NFOBJECT *obj, const gchar *name);

void nfui_nfobject_move(NFOBJECT *obj, gint x, gint y);
void nfui_nfobject_get_window_pos(NFOBJECT *obj, gint *x, gint *y);
gboolean nfui_nfobject_is_shown(NFOBJECT* obj);
void nfui_nfobject_set_state(NFOBJECT *obj, nfobject_state state);
nfobject_state nfui_nfobject_get_state(NFOBJECT *obj);

void nfui_nfobject_use_focus(NFOBJECT *obj, guint use);
void nfui_nfobject_use_hierarchy(NFOBJECT *obj, guint use);
void nfui_nfobject_set_kfocus(NFOBJECT *obj, guint ft);

void nfui_nfobject_enable(NFOBJECT* obj);
void nfui_nfobject_disable(NFOBJECT* obj);

gboolean nfui_nfobject_is_disabled(NFOBJECT* obj);
GdkWindow *nfui_nfobject_get_drawable(NFOBJECT *obj);
void nfui_nfobject_get_size(NFOBJECT* obj, gint *width, gint *height);
void nfui_nfobject_use_tooltip(NFOBJECT *obj, gboolean use);
gboolean nfui_nfobject_is_supported_multi_lang(NFOBJECT *obj);
gchar* nfui_nfobject_get_tooltip(NFOBJECT *obj);
void nfui_nfobject_support_multi_lang(NFOBJECT *obj, gboolean support);

gint nfui_nfobject_is_used_tooltip(NFOBJECT *obj);
void nfui_nfobject_set_tooltip(NFOBJECT *obj, const gchar *tooltip_string);
void nfui_nfobject_set_tooltip_info(NFOBJECT *obj, gpointer func);

void ud_free(NFOBJECT *obj);

int nfui_on_backscr(NFOBJECT *obj);
int nfui_off_backscr(NFOBJECT *obj);
int nfui_flip(NFOBJECT *object);
int nfui_rflip(NFOBJECT *object);

guint nfui_nfobject_timeout_add(NFOBJECT *obj, guint interval, GSourceFunc func, gpointer data);

#endif  //__NFOBJECT_H__

