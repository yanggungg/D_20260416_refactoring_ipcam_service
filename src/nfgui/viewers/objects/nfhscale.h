


#ifndef	__NF_HSCALE_H__
#define	__NF_HSCALE_H__

/******************************************************************
 *
 *	NFHSCALE 
 *
 *
 *****************************************************************/



#include "nfobject.h"


G_BEGIN_DECLS


#define NF_HSCALE(widget)		((NFHSCALE*)widget)


typedef struct _NFHSCALE 		NFHSCALE;

struct _NFHSCALE {
	NFOBJECT object;

	GdkPixbuf *bg_img;
	GdkPixbuf *bar_img;	

	GdkRectangle bar_rec;

	guint div_unit;
	guint step_val;
};


NFOBJECT* nfui_hscale_new();
void nfui_hscale_set_bg_image(NFHSCALE *hscale, GdkPixbuf *image);
void nfui_hscale_set_bar_image(NFHSCALE *hscale, GdkPixbuf *image, guint width, guint height);

void nfui_hscale_set_value(NFHSCALE *hscale, guint step_val);
guint nfui_hscale_get_value(NFHSCALE *hscale);


G_END_DECLS

#endif	// __NF_HSCALE_H__
