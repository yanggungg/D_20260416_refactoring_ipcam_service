/*
 * vw_vca_rev_calibration.c
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
static NFOBJECT *g_parent = NULL;




////////////////////////////////////////////////////////////
//
// private interfaces 
//





////////////////////////////////////////////////////////////
//
// handler
//
static gboolean post_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_EXPOSE:
			break;

		case GDK_DELETE:
			g_curwnd = 0;
			break;

		default:
			break;
	}

	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 					
			return FALSE;
	}

	return FALSE;
}

static gboolean post_nextbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 					
			return FALSE;
		_vca_rev_calibration_result(g_parent);
		_vca_rev_calibration_hide(g_fixed);
	}

	return FALSE;
}





////////////////////////////////////////////////////////////
//
// protected interfaces 
//

gint _vca_rev_calibration_page(NFOBJECT *parent)
{
    NFOBJECT *obj;
    NFOBJECT *fixed;
	NFOBJECT *btn_cancel;
	NFOBJECT *btn_next;

    g_parent = parent;
	g_curwnd = (NFWINDOW *)nfui_nfobject_get_top(g_parent);

	fixed = (NFOBJECT*)nfui_nffixed_new();
	//nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(400));
	nfui_nfobject_set_size(fixed, g_parent->width, g_parent->height - (VCA_REV_BTN_H + VCA_REV_BTN_GAP));
	nfui_nfobject_show(fixed);
	nfui_nffixed_put((NFFIXED*)g_parent, fixed, 0, 0);
	nfui_regi_pre_event_callback(fixed, post_fixed_event_handler);
	g_fixed = fixed;

	obj = (NFOBJECT*)nftool_normal_button_create_subtab_type1("CANCEL", VCA_REV_BTN_W);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)g_parent, obj, VCA_REV_WIDTH_GAP, VCA_REV_BTN1_Y);
	btn_cancel = obj;

	obj = (NFOBJECT*)nftool_normal_button_create_subtab_type1("NEXT", VCA_REV_BTN_W);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_regi_post_event_callback(obj, post_nextbutton_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)g_parent, obj, VCA_REV_BTN_X, VCA_REV_BTN1_Y);
	btn_next = obj;

    return 0;
}

void _vca_rev_calibration_show(NFOBJECT *parent)
{
    nfui_nfobject_show(parent);
}

void _vca_rev_calibration_hide(NFOBJECT *parent)
{
	NFOBJECT *topwin;

	topwin = nfui_nfobject_get_top(parent);
	
	nfui_nfobject_hide(topwin);

}


