#ifndef	__NFIMAGE_H__
#define	__NFIMAGE_H__

#include "nfobject.h"

#define	NFIMAGE_MAX_STRING_SIZE		128
#define	NFIMAGE_FONT_STRING_SIZE	64
#define	NFIMAGE_CASHING_KEY_SIZE	64


typedef struct {
	NFOBJECT object;

	GdkPixbuf *pixbuf;

	nfalign_type align;
	guint margin;

	gchar *pango_font;
	gchar font_name[NFIMAGE_FONT_STRING_SIZE];
	gint use_cashing;

	gchar cashing_key[NFIMAGE_CASHING_KEY_SIZE];
	gint valid_cnt;

	gchar strLabel[NFIMAGE_MAX_STRING_SIZE];
	guint font_color;

	nfalign_type font_align;
	guint font_margin;

	nfutil_pango_spacing_type spacing_type;
} NFIMAGE;




NFIMAGE* nfui_nfimage_new(const gchar *strImage);
NFIMAGE* nfui_nfimage_new_pixbuf(GdkPixbuf *pf);
void nfui_nfimage_destroy(NFIMAGE *image);
gboolean nfui_nfimage_change_image(NFIMAGE *nfimage, const gchar *strImage);

void nfui_nfimage_set_text(NFIMAGE *nfimage, const gchar *strLabel);
gchar* nfui_nfimage_get_text(NFIMAGE *nfimage);
void nfui_nfimage_set_pango_font(NFIMAGE *nfimage, const gchar *pfont, guint font_color);
void nfui_nfimage_use_pango_cashing(NFIMAGE *nfimage, gint cashing, gchar *key);
void nfui_nfimage_set_font_alignment(NFIMAGE *nfimage, nfalign_type align, guint margin);
void nfui_nfimage_set_font_color(NFIMAGE *nfimage, guint font_color);



#endif	// __NFIMAGE_H__

