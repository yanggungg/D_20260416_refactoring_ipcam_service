/*
 * vw_vca_rev_option.c
 *
 * Written by Eunhye. <eun@itxsecurity.com>
 * Copyright (c) ITX security, June 10, 2014
 *
 */

#include <glib.h>
#include "iux_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_page_manager.h"
#include "support/nf_ui_color.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "modules/ssm.h"
#include "modules/smt.h"
#include "modules/var.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nfimage.h"
#include "objects/nflabel.h"
#include "objects/nfvklabel.h"
#include "objects/nftab.h"
#include "objects/nfcheckbutton.h"

#include "vw_vca_rev_prop_internal.h"




////////////////////////////////////////////////////////////
//
// private data types
//





////////////////////////////////////////////////////////////
//
// private variable
//
static NFOBJECT *g_curwnd = NULL;
static NFOBJECT *g_fixed = NULL;



////////////////////////////////////////////////////////////
//
// private interfaces 
//





////////////////////////////////////////////////////////////
//
// handler
//




////////////////////////////////////////////////////////////
//
// protected interfaces 
//

gint _vca_rev_option_page(NFOBJECT *parent)
{
	g_curwnd = (NFWINDOW *)nfui_nfobject_get_top(parent);

    return 0;
}

void _vca_rev_option_show(NFOBJECT *parent)
{
    nfui_nfobject_show(parent);
}

void _vca_rev_option_hide(NFOBJECT *parent)
{
	NFOBJECT *topwin;

	topwin = nfui_nfobject_get_top(parent);
	
	nfui_nfobject_hide(topwin);

}


