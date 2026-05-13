#ifndef	__NFIPEDITOR_H__
#define	__NFIPEDITOR_H__


#include "nfobject.h"
#include "nflabel.h"

typedef struct {
	NFOBJECT object;

	guint ip[4];

	gchar *pango_font;
	gchar font_name[NFLABEL_FONT_STRING_SIZE];

	gint use_cashing;
	gchar cashing_key[NFLABEL_CASHING_KEY_SIZE];

	guint fg_color;

} NFIPEDITOR;

NFIPEDITOR* nfui_nfipeditor_new();
NFIPEDITOR* nfui_nfipeditor_new_with_ip(guint ip1, guint ip2, guint ip3, guint ip4);
void nfui_nfipeditor_set_ip(NFIPEDITOR *ipe, guint ip1, guint ip2, guint ip3, guint ip4);
void nfui_nfipeditor_set_ip_array(NFIPEDITOR *ipe, guint ip[4]);
void nfui_nfipeditor_set_ip_array_no_expose(NFIPEDITOR *ipe, guint ip[4]);
void nfui_nfipeditor_get_ip(NFIPEDITOR *ipe, guint *ip);
void nfui_nfipeditor_get_ip_string(NFIPEDITOR *ipe, gchar *ip);

void nfui_nfipeditor_set_pango_font(NFIPEDITOR *ipe, const gchar *pfont, guint fg_color);
void nfui_nfipeditor_use_pango_cashing(NFIPEDITOR *ipe, gint cashing, gchar *key);

#endif	// __NFIPEDITOR_H__

