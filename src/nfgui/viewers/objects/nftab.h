#ifndef	__NFTAB_H__
#define	__NFTAB_H__


/******************************************************************
 *
 *	NFTAB 
 *
 *
 *****************************************************************/



#include "nfobject.h"


#define	NFSETUP_MAX_PAGE	16

#define	NFTAB_STATE_NORMAL	0
#define	NFTAB_STATE_ACTIVE	1
#define	NFTAB_STATE_FOCUS	2


typedef enum {
	NFTAB_DIR_V = 0,
	NFTAB_DIR_H
} nftab_dir_type;


typedef struct {
	NFOBJECT object;

	guint pages;

	gint cur_page;
	gint old_page;
	gint new_page;

	guint tab_width;
	guint tab_height;

	guint margin;

	nftab_dir_type direction;

	GdkPixbuf *pmActive[NFSETUP_MAX_PAGE];
	GdkPixbuf *pmNormal[NFSETUP_MAX_PAGE];

//	guint bg_normal;
//	guint bg_active;

	gchar *pango_font;
	gchar font_name[128];
	guint color_idx[4];

	gchar strTitle[NFSETUP_MAX_PAGE][32];
	NFOBJECT *page[NFSETUP_MAX_PAGE];

	gint valid_cnt[NFSETUP_MAX_PAGE];
	gint use_strip;

	gboolean draw_outlines;
	
	guint repeat_key;
	guint rkey_id;

	// For tooltip
	gint index;
	guint delay_src;

} NFTAB;


NFTAB* nfui_nftab_new(guint pages, gchar *images[2], guint width, guint height, nftab_dir_type direction, gchar **strTitle, guint color_idx[3]);
void nfui_nftab_destroy(NFTAB *nftab);
void nfui_nftab_regi_page(NFTAB *nftab, NFOBJECT *page, guint nth);
void nfui_nftab_regi_page_all(NFTAB *nftab, NFOBJECT **page, guint page_cnt);
void nfui_nftab_unregi_page(NFTAB *nftab, guint nth);
void nfui_nftab_set_fg_color(NFTAB *nftab, guint normal, guint active, guint prelight);
void nfui_nftab_set_bg_color(NFTAB *nftab, guint normal, guint active);
void nfui_nftab_set_pango_font(NFTAB *nftab, const gchar *font);
void nfui_nftab_set_margin(NFTAB *nftab, guint margin);
gint nfui_nftab_get_cur_page(NFTAB *nftab);
guint nfui_nftab_get_clicked_page(NFTAB *nftab, guint x, guint y);
void nfui_nftab_set_draw_outlines(NFTAB *nftab, gboolean canDraw);

void nfui_nftab_set_images(NFTAB *nftab, gint cnt, gchar *nor_img[NFSETUP_MAX_PAGE], gchar *act_img[NFSETUP_MAX_PAGE]);

void nfui_nftab_use_strip(NFTAB* nftab, gboolean use);

void nfui_nftab_set_init_page(NFTAB *nftab, guint page);

NFOBJECT* nfui_nftab_get_nth_page(NFTAB *nftab, gint idx);
void nfui_nftab_set_cur_page(NFTAB *nftab, gint page ); //wiggls - otm
void nfui_nftab_change_page(NFTAB *nftab, guint page_num);

gint nfui_nftab_get_new_page(NFTAB *nftab);
gint nfui_nftab_set_new_page(NFTAB *nftab, gint page);

gint nfui_nftab_is_valid_page(NFTAB *nftab, guint nth);

#endif	// __NFTAB_H__

