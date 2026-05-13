
#ifndef	__NFBUTTON_H__
#define	__NFBUTTON_H__

/******************************************************************
 *
 *	NFBUTTON 
 *
 *
 *****************************************************************/


#include "nfobject.h"
#include "../../support/util.h"

G_BEGIN_DECLS

#define NF_BUTTON(widget)		((NFBUTTON*)widget)

#define	NFBUTTON_MAX_STRING_SIZE	128
#define	NFBUTTON_FONT_STRING_SIZE	64
#define	NFBUTTON_CASHING_KEY_SIZE	64

typedef struct _NFBUTTON 		NFBUTTON;
typedef struct _ICONPOSITION	ICONPOSITION;

struct _ICONPOSITION {
	gint x;
	gint y;

	gint right_margin;
	gint left_margin;
};

struct _NFBUTTON {
	NFOBJECT object;
	
	guint status;

	GdkPixbuf *image[NFOBJECT_STATE_COUNT];
	GdkPixbuf *icon[NFOBJECT_STATE_COUNT];

	gchar *pango_font;
	gchar font_name[NFBUTTON_FONT_STRING_SIZE];
	gint use_cashing;

	gchar cashing_key[NFBUTTON_CASHING_KEY_SIZE];
	gint valid_cnt;

	gchar strLabel[NFBUTTON_MAX_STRING_SIZE];
	guint font_color[NFOBJECT_STATE_COUNT];
	nfalign_type font_align;
	guint font_margin;

	gboolean sensitive;

	gboolean draw_outline;

	/* For Radio Button */
	gboolean toggled;

	GSList *group;

	nfutil_pango_spacing_type spacing_type;

	ICONPOSITION	icon_pos;
};


NFBUTTON* nfui_nfbutton_new();
NFBUTTON* nfui_nfbutton_new_with_param(GdkPixbuf **img, const gchar *strLabel);
NFBUTTON* nfui_nfbutton_new_no_image(gint *bg_color, const gchar *strLabel);
void nfui_nfbutton_set_image(NFBUTTON *button, GdkPixbuf **img);
void nfui_nfbutton_set_icon_image(NFBUTTON *button, GdkPixbuf **icon);
void nfui_nfbutton_set_icon_position(NFBUTTON *button, ICONPOSITION position);
void nfui_nfbutton_set_text(NFBUTTON *button, const gchar *strLabel);
void nfui_nfbutton_set_pango_font(NFBUTTON *button, const gchar *pfont, guint *font_color);
void nfui_nfbutton_use_pango_cashing(NFBUTTON *button, gint cashing, gchar *key);
void nfui_nfbutton_set_bg_color(NFBUTTON *button, gint *bg_color);

void nfui_nfbutton_set_font_alignment(NFBUTTON *button, nfalign_type align, guint margin);
gchar *nfui_nfbutton_get_text(NFBUTTON *button);
void nfui_nfbutton_sensitive(NFBUTTON *button, gboolean sensitive);
void nfui_nfbutton_destroy(NFBUTTON *button);

/* For Radio Button */
GSList* nfui_radio_button_get_group(NFBUTTON *button);
void nfui_radio_button_add_group(NFBUTTON *button, GSList *group);

void nfui_radio_button_set_toggled(NFBUTTON *button, gboolean toggled);
void nfui_radio_button_unset_toggled(NFBUTTON *button);
gboolean nfui_radio_button_get_toggled(NFBUTTON *button);
gint nfui_radio_button_get_index(NFBUTTON *button);
void nfui_nfbutton_set_drawing_outline(NFBUTTON *button, gboolean draw_outline);

void nfui_nfbutton_set_spacing(NFBUTTON *button, nfutil_pango_spacing_type spacing_type);
void nfui_nfbutton_set_status(NFBUTTON *button, nfobject_state status);


G_END_DECLS

#endif	// __NFBUTTON_H__
