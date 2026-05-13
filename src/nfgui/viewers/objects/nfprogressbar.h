#ifndef	__NFPROGRESSBAR_H__
#define	__NFPROGRESSBAR_H__



typedef struct {
	NFOBJECT object;

	GdkPixbuf *img;
	GdkPixbuf *bg;
	GdkPixbuf *head;
	GdkPixbuf *tail;
	GdkPixbuf *draw;

	guint img_width;
	guint bg_width;
	guint head_width;
	guint tail_width;
	guint img_height;

	guint margin;
	guint img_num;

	gint rate;
	guint filled_cnt;

} NFPROGRESSBAR;

NFPROGRESSBAR* nfui_nfprogressbar_new(GdkPixbuf *img, guint width);
NFPROGRESSBAR* nfui_nfprogressbar_new_with_images(GdkPixbuf *bg, GdkPixbuf *head, GdkPixbuf *body, GdkPixbuf *tail);
void nfui_nfprogressbar_destroy(NFPROGRESSBAR* nfprog);
void nfui_nfprogressbar_set_rate(NFPROGRESSBAR* nfprog, gint rate);
guint nfui_nfprogressbar_get_rate(NFPROGRESSBAR* nfprog);



#endif	// __NFPROGRESSBAR_H__




