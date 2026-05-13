// vim:ts=4:sw=4
/*******************************************************************************
*  (c) COPYRIGHT 2011 ITX security                                             *
*  ALL RIGHT RESERVED                                                          *
*                                                                              *
*  Jongbin Yim,  jongbina@itxsecurity.com                                      *
********************************************************************************

REVISION HISTORY:

Date       Name           Description
__________ ______________ ______________________________________________________
2011/12/20 Jongbin Yim    Created.

................................................................................
DESCRIPTION:

................................................................................
*/

/**
 * @file  vw_tools.c
 * @brief
 */

/*******************************************************************************
 * includes                                                                    *
 ******************************************************************************/

#include "nf_afx.h"
#include "support/event_loop.h"
#include "support/nf_ui_color.h"
#include "support/nf_ui_image.h"
#include "tools/nf_ui_tool.h"
#include "viewers/objects/nfbutton.h"
#include "viewers/objects/nfcheckbutton.h"
#include "viewers/objects/nfcombobox.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfobject.h"
#include "viewers/objects/nftable.h"
#include "vw_tools.h"

/*******************************************************************************
 * defines                                                                     *
 ******************************************************************************/

/*******************************************************************************
 * typedefs                                                                    *
 ******************************************************************************/

/*******************************************************************************
 * globals                                                                     *
 ******************************************************************************/

/*******************************************************************************
 * locals                                                                      *
 ******************************************************************************/

/*******************************************************************************
 * function declarations                                                       *
 ******************************************************************************/

void
vw_create_group_bg(gchar *name, gint width, gint height)
{
	sprintf(name, "MK_IMG_GROUP_BG_%3d_%3d", width, height);
	//captainnn
	/*
	nf_ui_create_image_popup_method(name, (guint)width, (guint)height,
			SUB_GROUP_BG1, SUB_GROUP_BG2, SUB_GROUP_BG3,
			SUB_GROUP_BG7, SUB_GROUP_BG8, SUB_GROUP_BG9,
			SUB_GROUP_BG4, SUB_GROUP_BG6, NULL, UX_COLOR(194));
			*/
}	/* vw_create_group_bg(... */

void
vw_obj_endis(NFOBJECT *obj, gboolean enable, gboolean expose)
{
	if ( enable )
		nfui_nfobject_enable(obj);
	else
		nfui_nfobject_disable(obj);
	if ( expose )
		nfui_signal_emit(obj, GDK_EXPOSE,
				obj->type == NFOBJECT_TYPE_NFCOMBOBOX);
}	/* vw_obj_endis(... */

/**
 * @brief  Creates a fixed object and put it to the parent fixed object.
 */
NFOBJECT *
vw_fixed_create(NFOBJECT *parent, gint bg, guint show,
		gint x, gint y, gint w, gint h, gpointer precb)
{
	NFOBJECT *fixed;

	fixed = (NFOBJECT *)nfui_nffixed_new();
	if ( !fixed )
		return fixed;

	nfui_nfobject_set_size(fixed, w, h);
	if ( show )
		nfui_nfobject_show(fixed);
	if ( bg >= 0 )
		nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(bg));
	if ( parent )
		nfui_nffixed_put((NFFIXED *)parent, fixed, (guint)x, (guint)y);
	if ( precb )
		nfui_regi_pre_event_callback(fixed, precb);
	return fixed;
}	/* vw_fixed_create(... */


/**
 * @brief  Creates a label object and put it to the parent object.
 *
 * @remark  If w = 0 but strlen(string) != 0, then this function will
 *  compute the width of the label automatically.
 *  If the parent is a fixed object, then (x, y) is coordinate.
 *  If the parent is a table object, then (x, y) is table index(col, row).
 */
NFOBJECT *
vw_label_create(NFOBJECT *parent, gchar *string, nffont_type font,
		nfalign_type align, guint margin, guint focus, guint show,
		gint fg, gint bg, gint x, gint y, gint w, gint h, gpointer postcb)
{
	NFOBJECT *lb;

	lb = (NFOBJECT *)nfui_nflabel_new_with_pango_font(string,
			nffont_get_pango_font(font), (guint)COLOR_IDX(fg));
	if ( !lb )
		return lb;

	nfui_nflabel_set_align((NFLABEL *)lb, align, margin);
	if ( bg >= 0 )
		nfui_nfobject_modify_bg(lb, NFOBJECT_STATE_NORMAL, COLOR_IDX(bg));
	if ( w == 0 && string[0] )
		w = (gint)(nfutil_string_width(0, NULL, nffont_get_pango_font(font),
				string, NORMAL_SPACING) + margin * 2);
	if ( w > 0 && h > 0 )
		nfui_nfobject_set_size(lb, w, h);
	nfui_nfobject_use_focus(lb, focus);
	if ( show )
		nfui_nfobject_show(lb);
	if ( parent ) {
		if ( parent->type == NFOBJECT_TYPE_NFFIXED )
			nfui_nffixed_put((NFFIXED *)parent, lb, (guint)x, (guint)y);
		else if ( parent->type == NFOBJECT_TYPE_NFTABLE )
			nfui_nftable_attach((NFTABLE *)parent, lb, (guint)x, (guint)y);
	}
	if ( postcb )
		nfui_regi_post_event_callback(lb, postcb);
	return lb;
}	/* vw_label_create(... */

NFOBJECT *
vw_ckbutton_lb_create(NFOBJECT *parent, gchar *string,
		gboolean active, gint fg, gint x, gint y, gpointer postcb_ckbtn)
{
	gint ck_w, ck_h;
	NFOBJECT *ck;

	ck = (NFOBJECT *)nfui_checkbutton_new(active);
	if ( !ck )
		return ck;

	nfui_check_button_set_skin_type(NF_CHECKBUTTON(ck), NFCHECK_TYPE_NORMAL);
	nfui_check_get_size(NF_CHECKBUTTON(ck), &ck_w, &ck_h);
	nfui_nfobject_show(ck);
	if ( parent )
		nfui_nffixed_put((NFFIXED *)parent, ck, (guint)x, (guint)y);
	if ( postcb_ckbtn )
		nfui_regi_post_event_callback(ck, postcb_ckbtn);

	if ( ck && string )
		vw_label_create(parent, string, NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 4,
				0, 1, fg, -1, x + ck_w, y, 0, ck_h, NULL);
	return ck;
}	/* vw_ckbutton_lb_create(... */

NFOBJECT *
vw_nmbutton_create(NFOBJECT *parent, gchar *string,
		gint type, gboolean enable, guint x, guint y, guint w, gpointer postcb)
{
	NFOBJECT *btn;

	switch ( type ) {
		case 1:
			btn = nftool_normal_button_create_type1(string, w);
			break;
		case 2:
			btn = nftool_normal_button_create_type2(string, w);
			break;
		case 3:
		default:
			btn = nftool_normal_button_create_type3(string, w);
			break;
	}
	if ( !btn )
		return btn;

	nfui_nfbutton_set_font_alignment(NF_BUTTON(btn), NFALIGN_CENTER, 0);
	nfui_nfobject_show(btn);
	if ( parent )
		nfui_nffixed_put((NFFIXED *)parent, btn, x, y);
	if ( postcb )
		nfui_regi_post_event_callback(btn, postcb);
	if ( !enable )
		nfui_nfobject_disable(btn);
	return btn;
}	/* vw_nmbutton_create(... */

NFOBJECT *
vw_v_scbutton_create(NFOBJECT *parent, gboolean up,
		gint x, gint y, gpointer postcb)
{
	NFOBJECT *btn;
	GdkPixbuf *img[NFOBJECT_STATE_COUNT];

	if ( up ) {
		img[0] = nfui_get_image_from_file(IMG_N_SCROLL_UP, NULL);
		img[1] = nfui_get_image_from_file(IMG_O_SCROLL_UP, NULL);	
		img[2] = nfui_get_image_from_file(IMG_P_SCROLL_UP, NULL);	
		img[3] = nfui_get_image_from_file(IMG_N_SCROLL_UP, NULL);	
	}
	else {
		img[0] = nfui_get_image_from_file(IMG_N_SCROLL_DOWN, NULL);
		img[1] = nfui_get_image_from_file(IMG_O_SCROLL_DOWN, NULL);
		img[2] = nfui_get_image_from_file(IMG_P_SCROLL_DOWN, NULL);
		img[3] = nfui_get_image_from_file(IMG_N_SCROLL_DOWN, NULL);
	}

	btn = (NFOBJECT *)nfui_nfbutton_new_with_param(img, "");
	if ( !btn )
		return btn;

	nfui_nfobject_show(btn);
	if ( parent )
		nfui_nffixed_put((NFFIXED *)parent, btn, (guint)x, (guint)y);
	if ( postcb )
		nfui_regi_post_event_callback(btn, postcb);
	return btn;
}	/* vw_v_scbutton_create(... */

NFOBJECT *
vw_combo_create(NFOBJECT *parent, gchar **str, gint nitem, gint iidx,
		gchar *strlb, NFCOMBOBOX_TYPE type, guint show,
		gint x, gint y, gint w, gint h, gpointer postcb)
{
	NFOBJECT *cmb;

	cmb = nfui_combobox_new(str, nitem, iidx);
	if ( !cmb )
		return cmb;

	if ( strlb )
		nfui_combobox_set_display_string(NF_COMBOBOX(cmb), strlb);
	nfui_combobox_set_skin_type(NF_COMBOBOX(cmb), type);
	nfui_combobox_set_align((NFCOMBOBOX *)cmb, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(cmb, w, h);
	if ( show )
		nfui_nfobject_show(cmb);
	if ( parent )
		nfui_nffixed_put((NFFIXED *)parent, cmb, (guint)x, (guint)y);
	if ( postcb )
		nfui_regi_post_event_callback(cmb, postcb);
	return cmb;
}	/* vw_combo_create(... */

NFOBJECT *
vw_table_create(NFOBJECT *parent, NFOBJECT **lb, gint cols, gint rows,
		gint colsp, gint rowsp, guint *cellw, gint cellh, gchar **strcol,
		gint bg, gint fg1, gint bg1, gint fg2, gint bg2,
		gint x, gint y, gpointer postcb)
{
	gint i, j;
	NFOBJECT *tbl, *obj;

	tbl = (NFOBJECT *)nfui_nftable_new((guint)cols, (guint)rows + 1,
			(guint)colsp, (guint)rowsp, cellw, (guint)cellh);
	if ( !tbl )
		return tbl;

	nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(bg));
	nfui_nfobject_show(tbl);
	nfui_nffixed_put((NFFIXED *)parent, tbl, (guint)x, (guint)y);

	for (j = 0; j < cols; j++)
		vw_label_create(tbl, strcol[j], NFFONT_SMALL_SEMI,
				NFALIGN_CENTER, 0, 0, 1, fg1, bg1, j, 0, -1, -1, NULL);
	for (i = 0; i < rows; i++) {
		for (j = 0; j < cols; j++) {
			obj = vw_label_create(tbl, "", NFFONT_SMALL_SEMI,
					NFALIGN_LEFT, 4, 1, 1, fg2, bg2, j, i + 1, -1, -1,
					postcb);
			nfui_nflabel_set_drawing_outline((NFLABEL *)obj, FALSE);
			nfui_nfobject_set_data(obj, "rowi", GINT_TO_POINTER(i));
			if ( lb )
				lb[i * cols + j] = obj;
		}
	}
	return tbl;
}	/* vw_table_create(... */

