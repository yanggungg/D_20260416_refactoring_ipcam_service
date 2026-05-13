/*
 * vaa_s1_internal.h
 * 	- video analytics agent internal
 *	- dependencies :
 *		DIT	
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, May 13, 2013
 *
 */

#ifndef __VAA_S1_INTERNAL_H
#define __VAA_S1_INTERNAL_H

#include "nfdal.h"
#include "dit.h"
#include "vaa_s1.h"

#define MAX_RULE	32
#define MAX_PATT	16
#define MAX_META	4
#define MAX_DIC		4
#define MAX_TBOX	256

#define MASK_FIGURE		0x000F
#define MASK_NOFIGURE	0x0000
#define MASK_LINE		0x0001
#define MASK_AREA		0x0002
#define MASK_COUNT		0x0010
#define MASK_DIRCTRL	0x0020

#define _IS_NOFIGURE(pr)	((pr->type & MASK_FIGURE) == 0x0000)
#define _IS_LINE(pr)		((pr->type & MASK_FIGURE) == MASK_LINE)
#define _IS_AREA(pr)		((pr->type & MASK_FIGURE) == MASK_AREA)

#define _IS_CNT_LINE(pr)	((	(pr->type & MASK_FIGURE) == MASK_LINE) && \
								(pr->type & MASK_COUNT))
#define _IS_CNT_AREA(pr)	((	(pr->type & MASK_FIGURE) == MASK_AREA) && \
								(pr->type & MASK_COUNT))
#define _IS_DIRCTRL(pr)		(pr->type & MASK_DIRCTRL)

typedef struct _DB_LINK {
	char	lz;
	char	lc;
} DB_LINK;

typedef struct _DIRINFO {
	IGPOINT	pt;
//	char 	*img_name;
} DIRINFO;

typedef struct _VARULE {
	int				occupied;
	PATTERN_E		patt;	
	DB_LINK			dlink;
	BITMASK			type;			// 0x0001 : line
									// 0x0002 : area
									// 0x0010 : link to counter


	int				sty_highlight;

	int				blink_step;
	int				blink_en;

	DIRINFO			dirinfo;

	VCAZone			zn_data;
	VCACntr			ct_data;

	DICPTR			dic[MAX_DIC];
} VARULE;

typedef struct _VAPATT {
	int				use;
	PATTERN_E		patt;
	int				dipage;
} VAPATT;

typedef struct _VAMETA {
	int				use;
	VAA_META_E		meta;
	int				dipage;
} VAMETA;

typedef struct _VATRACKBOX {
	DICPTR			dic;
	int 			age;
} VATRACKBOX;

typedef struct _DBSLOT {
	char			zn_data[IVCA_MAX_ZONES];
	char			ct_data[IVCA_MAX_CNTRS];
} DBSLOT;

typedef struct _S1VAA_T {
	int				ch;
	int				evt_linked;
	PATTERN_E		evt_linked_patt;
	DITID			ditid;
	DBSLOT			used;

	VCAPropData		prop;
	int				zn_count;
	int				ct_count;

	VCAData      	db;

	int				cnt_patt;
	VAPATT			vapatt[MAX_PATT];
	int				cnt_meta;
	VAMETA			vameta[MAX_META];
	int				cnt_rule;
	VARULE			varule[MAX_RULE];
	int				cnt_tbox;
	VATRACKBOX		vatbox[MAX_TBOX];
} S1VAA_T;


////////////////////////////////////////////////////////////
//
// protected interfaces 
//

int _vaa_init_figure(FIGURE_INFO *info);
int _vaa_construct_figure(IGPOINT *pt, int cnt, FIGURE_INFO *info);
int _vaa_construct_figure_rect(IGRECT *rect, FIGURE_INFO *info);

int _vaa_s1_link_event(VAAID id);

#endif
