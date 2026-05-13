#ifndef	__NFIMAGE_LABEL_H__
#define	__NFIMAGE_LABEL_H__

#include "nfobject.h"


#define	NFIMGLABEL_MAX_STRING_SIZE		128
#define	NFIMGLABEL_FONT_STRING_SIZE		64
#define	NFIMGLABEL_CASHING_KEY_SIZE		64

typedef struct {
	NFOBJECT object;

	GdkPixbuf *pixbuf;

	gchar *pango_font;
	gchar font_name[NFIMGLABEL_FONT_STRING_SIZE];
	gint use_cashing;
	gchar cashing_key[NFIMGLABEL_CASHING_KEY_SIZE];

	gchar strLabel[NFIMGLABEL_MAX_STRING_SIZE];
	guint fg_color_idx;
	gint valid_cnt;
	guint shadow;

	guint use_strip;

  	nfutil_pango_spacing_type spacing_type;

	nfalign_type img_align;

	gboolean draw_outline;
} NFIMGLABEL;


NFIMGLABEL* nfui_nfimglabel_new(GdkPixbuf *pmImage, const gchar *strLabel);
void nfui_nfimglabel_destroy(NFIMGLABEL *image);
void nfui_nfimglabel_set_text(NFIMGLABEL *nfimglabel, gchar *strLabel);
void nfui_nfimglabel_set_pango_font(NFIMGLABEL *nfimglabel, const gchar *pfont, guint fg_idx);
void nfui_nfimglabel_use_pango_cashing(NFIMGLABEL *nfimglabel, gint cashing, gchar *key);
void nfui_nfimglabel_set_spacing(NFIMGLABEL *nfimglabel, nfutil_pango_spacing_type spacing_type);
//void nfui_nfimglabel_set_drawing_outline(NFIMGLABEL *nfimglabel, gboolean draw_outline);
void nfui_nfimglabel_use_strip(NFIMGLABEL* nflabel, gboolean use);
void nfui_nfimglabel_set_align(NFIMGLABEL* nfimglabel, nfalign_type align);
gchar* nfui_nfimglabel_get_text(NFIMGLABEL *nfimglabel);
void nfui_nfimglabel_set_shadow(NFIMGLABEL *nfimglabel, gboolean set);
#endif	// __NFIMAGE_LABEL_H__


