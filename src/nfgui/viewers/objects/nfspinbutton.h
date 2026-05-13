

#ifndef	__NF_SPINBUTTON_H__
#define	__NF_SPINBUTTON_H__

/******************************************************************
 *
 *	NFSPINBUTTON 
 *
 *
 *****************************************************************/


#include "../../support/util.h"

#include "nfobject.h"
#include "nffixed.h"


G_BEGIN_DECLS


#define NF_SPINBUTTON(widget)		((NFSPINBUTTON*)widget)


typedef struct _NFSPINBUTTON 		NFSPINBUTTON;

typedef enum {
	SPIN_ARROW_UP,
	SPIN_ARROW_DOWN
}NFSpinArrowType;

struct _NFSPINBUTTON {
	NFFIXED spin_fixed;
	
	GtkAdjustment *adjustment;
	
	NFOBJECT *label;
	NFOBJECT *up_btn;
	NFOBJECT *down_btn;
	
	GdkGC *bg_gc;

#if 0
	GdkFont *font;
	guint font_color;
#endif

	GSList *data;

	gint pre_index;
	gint index;
	gboolean sensitive;

    gint digits;
    
	nfalign_type align;
	guint margin;

	guint krepeat_src;
	guint mrepeat_src;
	guint rkey_id;

	gboolean draw_outline;

	nfutil_pango_spacing_type spacing_type;
       guint only_enterkey_action;                     //for digimerge when audio select
	gint skin_type;	
};

typedef enum {
	NFSPINBUTTON_TYPE_UNDEF 		= 0,
	NFSPINBUTTON_TYPE_1		 		= 1,
	NFSPINBUTTON_TYPE_POPUP_1		= 2,
	NFSPINBUTTON_TYPE_SUBTAB_1		= 3,	
	NFSPINBUTTON_TYPE_POPUP_SMALL_1	= 4,	
	NFSPINBUTTON_TYPE_MAX,
} NFSPINBUTTON_TYPE;

NFOBJECT* nfui_spinbutton_new(gchar *data[], gint cnt, gint init);
NFOBJECT* nfui_spinbutton_new_index_with_range(gint init_index, gdouble min, gdouble max, gdouble step);
NFOBJECT* nfui_spinbutton_new_value_with_range(gdouble init_value, gdouble min, gdouble max, gdouble step);

void nfui_spinbutton_set_skin_type(NFSPINBUTTON *spin, NFSPINBUTTON_TYPE type);
gboolean nfui_spin_button_set_text(NFSPINBUTTON *spin, gchar* text);
gboolean nfui_spin_button_set_text_no_expose(NFSPINBUTTON *spin, gchar* text);
gboolean nfui_spin_button_set_data(NFSPINBUTTON *spin, gchar* data[], guint cnt);
gboolean nfui_spin_button_set_data_no_expose(NFSPINBUTTON *spin, gchar* data[], guint cnt);
gboolean nfui_spin_button_set_index(NFSPINBUTTON *spin, gint index);
gboolean nfui_spin_button_set_index_no_expose(NFSPINBUTTON *spin, gint index);
gchar* nfui_spin_button_get_text(NFSPINBUTTON *spin);
gint nfui_spin_button_get_value(NFSPINBUTTON *spin);

gint nfui_spin_button_get_index(NFSPINBUTTON *spin);
void nfui_spin_button_set_align(NFSPINBUTTON *spin, nfalign_type align, guint margin);

void nfui_spin_button_sensitive(NFSPINBUTTON *spin, gboolean sensitive);

guint nfui_spin_button_remove_data(NFSPINBUTTON *spin, const gchar *data);
void nfui_spin_button_remove_all(NFSPINBUTTON *spin);
gboolean nfui_spin_button_append_data(NFSPINBUTTON *spin, gchar *text);
gint nfui_spin_button_get_cnt(NFSPINBUTTON *spin);

void nfui_spin_button_set_pango_font(NFSPINBUTTON *spin, gchar *pfont);
void nfui_spin_button_use_pango_cashing(NFSPINBUTTON *spin, gint cashing, gchar *key);
void nfui_spin_button_set_spacing(NFSPINBUTTON *spin, nfutil_pango_spacing_type spacing_type);


void nfui_spin_button_set_only_enterkey_action(NFSPINBUTTON *spin, guint only_enterkey_action);
NFOBJECT *nfui_spin_button_get_label(NFSPINBUTTON *spin);
gboolean nfui_spin_button_set_range(NFSPINBUTTON *spin, gdouble min, gdouble max);

G_END_DECLS

#endif	// __NF_SPINBUTTON_H__
