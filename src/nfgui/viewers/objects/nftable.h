#ifndef	__NFTABLE_H__
#define	__NFTABLE_H__

#include "nfobject.h"

#define	NFTABLE_MAX_COUNT	4
#define	NFTABLE_MAX_COLUMNS	64
#define	NFTABLE_MAX_ROWS	64



typedef struct {
#if 0
	NFCONTAINER container;
#else
	NFOBJECT object;
	GSList *children;
#endif

	guint cols;
	guint rows;

	guint col_space;
	guint row_space;

	guint cell_width[NFTABLE_MAX_COLUMNS];
	guint cell_height;

//	guint map[NFTABLE_MAX_COLUMNS*NFTABLE_MAX_ROWS];
	guint map[NFTABLE_MAX_ROWS][NFTABLE_MAX_COLUMNS];

	gboolean draw_outline;
} NFTABLE;


NFTABLE *nfui_nftable_new(guint cols, guint rows, guint col_space, guint row_space, guint *cell_width, guint cell_height);
void nfui_nftable_destroy(NFTABLE *ntb);
void nfui_nftable_attach(NFTABLE *ntb, NFOBJECT *obj, guint col, guint row);
NFOBJECT* nfui_nftable_get_child(NFTABLE *ntb, guint col, guint row);


void nfui_nftable_set_draw_outline(NFTABLE *ntb, gboolean draw);
int nfui_nftable_get_width(NFTABLE *nfb);
int nfui_nftable_get_height(NFTABLE *nfb);

#endif	// __NFTABLE_H__



