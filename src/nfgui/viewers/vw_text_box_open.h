/*
 * vw_text_box_open.h
 *	- dependencies :
 *		
 *
 * Written by Suelbee lee. <suelbeelee@itxsecurity.com>
 * Copyright (c) ITX security, July 5, 2013
 *
 */


#ifndef _TEXT_BOX_OPEN_H
#define _TEXT_BOX_OPEN_H

#include "objects/nfobject.h"

enum TB_TYPE_E {
	TB_TYPE_LIST		= 0,
	TB_TYPE_LABEL		= 1,	
	TB_TYPE_MAX
};

enum TB_OPT_E {
	TB_OPT_AUTO_LF		= 0,
	TB_OPT_MAX
};

typedef struct _TEXT_BOX_INFO{
	guint win_w;
	guint win_h;
	gint win_x;
	gint win_y;
	gint    type;
	gchar *line;
} TEXT_BOX_INFO;

typedef struct _TEXT_BOX_CONTROL {
	IMSG    add;
	IMSG    clear;
	IMSG    modify;	
} TEXT_BOX_CONTROL;


gint VW_TextBox_Open(NFOBJECT *parent, gchar *title, TEXT_BOX_INFO *box_info, TEXT_BOX_CONTROL *box_control, guint opt);

#endif

