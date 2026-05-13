

#ifndef	__NF_CHECK_BUTTON_H__
#define	__NF_CHECK_BUTTON_H__

/******************************************************************
 *
 *	NFCHECKBUTTON 
 *
 *
 *****************************************************************/



#include "nfobject.h"
#include "../support/util.h"

G_BEGIN_DECLS

#define NF_CHECKBUTTON(widget)		((NFCHECKBUTTON*)widget)

#define	NFCHECK_MAX_STRING_SIZE		128
#define	NFCHECK_FONT_STRING_SIZE	64
#define	NFCHECK_CASHING_KEY_SIZE	64

enum {
	NFCHECK_NORMAL_INACTIVE = 0,
	NFCHECK_NORMAL_ACTIVE,
	NFCHECK_FOCUS_INACTIVE,
	NFCHECK_FOCUS_ACTIVE,
	NFCHECK_DISABLE_INACTIVE,
	NFCHECK_DISABLE_ACTIVE,

	NFCHECK_STATES,
};

enum {
	NFCHECK_INACTIVE_NORMAL = 0,
	NFCHECK_INACTIVE_FOCUS,
	NFCHECK_INACTIVE_SELECT,
	NFCHECK_INACTIVE_DISABLE,
	NFCHECK_INACTIVE_STATES
};

enum {
	NFCHECK_ACTIVE_NORMAL = 0,
	NFCHECK_ACTIVE_FOCUS,
	NFCHECK_ACTIVE_SELECT,
	NFCHECK_ACTIVE_DISABLE,	
	NFCHECK_ACTIVE_STATES
};

typedef enum {
	NFCHECK_TYPE_UNDEF 		    = 0,
	NFCHECK_TYPE_NORMAL 		= 1,
	NFCHECK_TYPE_SMALL			= 2,
	NFCHECK_TYPE_POPUP_NORMAL 	= 3,
	NFCHECK_TYPE_POPUP_SMALL	= 4,
	NFCHECK_TYPE_SUBTAB_NORMAL 	= 5,
	NFCHECK_TYPE_SUBTAB_SMALL	= 6,
	NFCHECK_TYPE_DEF            = 7,	
	NFCHECK_TYPE_MAX,
} NFCHECK_TYPE;

typedef struct _NFCHECKBUTTON	 NFCHECKBUTTON;

struct _NFCHECKBUTTON {
	NFOBJECT object;
	
	GdkPixbuf *inactive_image[NFCHECK_INACTIVE_STATES];
	GdkPixbuf *active_image[NFCHECK_ACTIVE_STATES];
	
	guint checked;
//	gboolean sensitive;

	gchar *pango_font;
	gchar font_name[NFCHECK_FONT_STRING_SIZE];
	gint use_cashing;

	gchar cashing_key[NFCHECK_CASHING_KEY_SIZE];
	gint valid_cnt;

	gchar strLabel[NFCHECK_MAX_STRING_SIZE];
	guint inactive_font_color[NFOBJECT_STATE_COUNT];
	guint active_font_color[NFOBJECT_STATE_COUNT];

	nfalign_type font_align;
	guint font_margin;

	nfutil_pango_spacing_type spacing_type;
	gint skin_type;	
};


NFOBJECT* nfui_checkbutton_new(gboolean active);
void nfui_check_button_set_skin_type(NFCHECKBUTTON *check, NFCHECK_TYPE type);
void nfui_check_button_set_inactive_image(NFCHECKBUTTON *check, GdkPixbuf **img);
void nfui_check_button_set_active_image(NFCHECKBUTTON *check, GdkPixbuf **img);

gboolean nfui_check_button_get_active(NFCHECKBUTTON *check);
void nfui_check_button_set_active(NFCHECKBUTTON *check, gboolean active);
void nfui_check_button_set_active_no_expose(NFCHECKBUTTON *check, gboolean active);
void nfui_check_button_sensitive(NFCHECKBUTTON *check, gboolean sensitive);

void nfui_check_set_text(NFCHECKBUTTON *check, const gchar *strLabel);
gchar* nfui_check_get_text(NFCHECKBUTTON *check);
void nfui_check_set_active_pango_font(NFCHECKBUTTON *check, const gchar *pfont, guint *font_color);
void nfui_check_set_inactive_pango_font(NFCHECKBUTTON *check, const gchar *pfont, guint *font_color);
void nfui_check_use_pango_cashing(NFCHECKBUTTON *check, gint cashing, gchar *key);
void nfui_check_set_font_alignment(NFCHECKBUTTON *check, nfalign_type align, guint margin);


G_END_DECLS

#endif	// __NF_CHECK_BUTTON_H__
