#ifndef	__NFLABEL_H__
#define	__NFLABEL_H__


/******************************************************************
 *
 *	NFLABEL 
 *
 *
 *****************************************************************/

#include "nfobject.h"
#include "../../support/util.h"

#define	NF_LABEL(widget)			((NFLABEL *)widget)

#define	NFLABEL_MAX_STRING_SIZE		512
#define	NFLABEL_FONT_STRING_SIZE	64
#define	NFLABEL_CASHING_KEY_SIZE	64

typedef struct {
	NFOBJECT object;

	gchar *pango_font;
	gchar font_name[NFLABEL_FONT_STRING_SIZE];

	gint use_cashing;
	gchar cashing_key[NFLABEL_CASHING_KEY_SIZE];

	gchar strLabel[NFLABEL_MAX_STRING_SIZE];
	gint valid_cnt;
	gint use_strip;

	guint shadow;
	guint useNumber;
	gint intLabel;
	guint fg_color_idx[NFOBJECT_STATE_COUNT];
	nfalign_type align;
	guint margin;
	
	gboolean draw_outline;
	gboolean invisible;
	gboolean is_vk;

	nfutil_pango_spacing_type spacing_type;

	gboolean multi_line_type;

	unsigned int style;
	gint skin_type;
	
} NFLABEL;

typedef enum {
	NFTEXTBOX_TYPE_UNUSED 			= 0,
	NFTEXTBOX_TYPE_UNDEF 			= 1,	
	NFTEXTBOX_TYPE_INPUT 			= 2,
	NFTEXTBOX_TYPE_OUTPUT			= 3,
	NFTEXTBOX_TYPE_POPUP_INPUT 		= 4,
	NFTEXTBOX_TYPE_POPUP_OUTPUT		= 5,
	NFTEXTBOX_TYPE_SUBTAB_INPUT 	= 6,
	NFTEXTBOX_TYPE_SUBTAB_OUTPUT	= 7,	
	NFTEXTBOX_TYPE_MAX,
} NFTEXTBOX_TYPE;

NFLABEL* nfui_nflabel_new(gchar *strLabel);
void nfui_nflabel_destroy(NFLABEL *nflabel);
void nfui_nflabel_set_text(NFLABEL *nflabel, gchar *strLabel);
gchar* nfui_nflabel_get_text(NFLABEL *nflabel);
void nfui_nflabel_use_number(NFLABEL *nflabel, gboolean use);
void nfui_nflabel_set_number(NFLABEL *nflabel, gint num);
gint nfui_nflabel_get_number(NFLABEL *nflabel);
gint nfui_nflabel_get_strlen(NFLABEL *nflabel);
void nfui_nflabel_set_align(NFLABEL *nflabel, nfalign_type align, guint margin);
void nfui_nflabel_modify_fg(NFLABEL *nflabel, guint fg_idx);
void nfui_nflabel_set_shadow(NFLABEL *nflabel, gboolean set);
#if 0
void nfui_nflabel_draw_text_without_expose(NFLABEL *nflabel, const gchar *strLabel);
#endif
void nfui_nflabel_set_drawing_outline(NFLABEL *nflabel, gboolean draw_outline);
void nfui_nflabel_set_invisible(NFLABEL *nflabel, gboolean invisible);
gboolean nfui_nflabel_get_invisible(NFLABEL *nflabel);


NFLABEL* nfui_nflabel_new_with_pango_font(gchar *strLabel, gchar *pfont, guint fg_idx);
void nfui_nflabel_set_pango_font(NFLABEL *nflabel, gchar *pfont, guint fg_idx);
void nfui_nflabel_use_pango_cashing(NFLABEL *nflabel, gint cashing, gchar *key);
void nfui_nflabel_set_spacing(NFLABEL *nflabel, nfutil_pango_spacing_type spacing_type);
void nfui_nflabel_set_multi_line_type(NFLABEL *nflabel, gboolean multi_line_type);

void nfui_nflabel_use_strip(NFLABEL* nflabel, gboolean use);
void nfui_nflabel_set_upper_mode(NFLABEL* nflabel, gboolean use);

void nfui_nflabel_set_style(NFLABEL* nflabel, NFSTYLE style);

guint nfui_nflabel_get_pango_font_color(NFLABEL *nflabel);

NFLABEL* nfui_nflabel_new_with_pango_font_and_vk(gchar *strLabel, gchar *pfont, guint fg_idx);

int nfui_nflabel_get_text_b(NFLABEL *nflabel, char *buf, int len);

NFLABEL* nfui_nflabel_new_text_box(gchar *strLabel, gchar *pfont);
void nfui_nflabel_set_skin_type(NFLABEL* nflabel, NFTEXTBOX_TYPE type);
void nfui_nflabel_set_fg_color(NFLABEL *nflabel, gint *fg_color);
#endif	// __NFLABEL_H__


