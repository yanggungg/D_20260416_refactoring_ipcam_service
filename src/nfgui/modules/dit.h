/*
 * dit.h
 * 	- drawing information table
 *	- dependencies :
 *		
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, May 13, 2013
 *
 */


#ifndef __DIT_H
#define __DIT_H

#include <glib.h>
#include "iux_types.h"
#include "ix_func.h"

#define MAX_PT		128


////////////////////////////////////////////////////////////
//
// public data type
//

#if defined(__ITXGUI64)
typedef unsigned long int	DITID;
#else
typedef unsigned int	DITID;
#endif

typedef enum _DIC_TYPE_E {
	DIC_FIG		= 0,
	DIC_IMG		= 1,
	DIC_TXT		= 2,
	DIC_ARC 	= 3,
	DIC_DIR		= 4,	
	DIC_MAX
} DIC_TYPE_E;

typedef struct _IGBOX {
	int		x1;
	int		y1;
	int		x2;
	int		y2;
} IGBOX;

typedef struct _IGRGB {
	char	r;
	char	g;
	char	b;
	char	a;
} IGRGB;

typedef union _IGCOLOR {
	unsigned int	u;		// 32bit
	int				i;		// index
	IGRGB			s;		// struct
} IGCOLOR;

typedef struct _IGPOINT {
	int		x;
	int		y;
	int		dx;				// display resolution 
	int		dy;				// display resolution 
	BITMASK	opt;
} IGPOINT;

typedef struct _IGPOLYGON {
	int			cnt;
	IGPOINT		pt[64];
	BITMASK	opt;
} IGPOLYGON;

typedef struct _IG_IMG_HOLDER {
	int		type;		// 0: text(image name), 1: etc
	int		w;
	int		h;
	void	*ptr;
} IG_IMG_HOLDER;

typedef struct _FIGURE_INFO {
	int			cnt;
	IGPOINT		pt[MAX_PT];
	int 		wi;		// width		, in pixel
	int 		st;		// style		, 0 : polygon, 1 : line, 9 : hide
	int			op;		// opacity		, not used
	int			vt;		// vertices		, 0 : hide vertices, 1 : show vertices
	int			fi;		// filled		, 0 : not filled, 1 : filled
	int			nr;
	IGCOLOR 	cl;
	char 		txt[64];
} FIGURE_INFO;

typedef struct _DIRECTION_INFO {
	int			cnt;
	IGPOINT		ln[2];
	IGPOINT		pt[MAX_PT];
	int 		wi;		// width		, in pixel
	int 		st;		// style		, 0 : polygon, 1 : line, 9 : hide
	int			op;		// opacity		, not used
	int			vt;		// vertices		, 0 : hide vertices, 1 : show vertices
	int			fi;		// filled		, 0 : not filled, 1 : filled
	int			nr;
	IGCOLOR 	cl;
	char 		txt[64];
} DIRECTION_INFO;

typedef struct _IMAGE_INFO {
	IGPOINT		pt;
	int			w;
	int			h;
	int			nr;
	IG_IMG_HOLDER	img;
} IMAGE_INFO;

typedef struct _TEXT_INFO {
	IGPOINT		pt;
	//IGPOINT		dt;         // margin           , of display resolution 
    int         layout_psx;  // x layout position  , -1 : left of pt, 0 : center of pt, 1 : right of pt
    int         layout_psy;  // y layout position  , -1 : up of pt, 0 : center of pt, 1 : bottom of pt

    int         align;
	int			size;       
	char 		txt[64];
	int			nr;
	IGCOLOR 	cl;         // font color 
} TEXT_INFO;

typedef struct _ARC_INFO {
	IGPOINT		pt;
	int			w;
	int			h;
	int			fi;		// filled		, 0 : not filled, 1 : filled
	int			nr;
	IGCOLOR 	cl;
} ARC_INFO;

typedef union _DIC_CONTENTS {
	FIGURE_INFO		f;
	DIRECTION_INFO	d;
	IMAGE_INFO		i;
	TEXT_INFO		t;
	ARC_INFO		a;	
} DIC_CONTENTS;

typedef struct _DIC {
	DIC_TYPE_E		type;
	int				en;		// enable or disable	, 0 : enable, 1 : disable
	int				page;
	IGRECT			area;
	int				zorder;
	DIC_CONTENTS	ctns;
} DIC;

typedef DIC*			DICPTR;



////////////////////////////////////////////////////////////
//
// public interfaces
//

DITID dit_create(int cnt_page, int cnvs_w, int cnvs_h);
int dit_destroy(DITID id);

int dit_enable(DITID id);
int dit_disable(DITID id);
int dit_is_enabled(DITID id);

int dit_enable_page(DITID id, int page);
int dit_disable_page(DITID id, int page);
int dit_disable_all_page(DITID id);
int dit_page_is_enabled(DITID id, int page);

int dit_get_cnvs_info(DITID id, int *w, int *h);
int dit_get_page_count(DITID id);

DICPTR dit_add_dic(DITID id, int page, DIC_TYPE_E type, DIC_CONTENTS *ctns);
int dit_remove_dic(DITID id, int page, DICPTR dic);

int dit_clear_page(DITID id, int page);

DICPTR *dit_new_dic_list(DITID id, int *count, IGRECT *invalid);
DICPTR *dit_new_dic_list_under_pt(DITID id, IGPOINT pt, int *count, IGRECT *invalid);


int dit_free_dic_list(DICPTR *ditlist);

#endif
