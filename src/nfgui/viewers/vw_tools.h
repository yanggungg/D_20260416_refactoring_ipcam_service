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
2011/12/20 Jongbin Yim    Created

................................................................................

DESCRIPTION:

................................................................................
*/

/**
 * @file  vw_tools.h
 * @brief
 */

#ifndef	_VW_TOOLS_H_
#define	_VW_TOOLS_H_

/*******************************************************************************
 * includes                                                                    *
 ******************************************************************************/

#include <glib.h>
#include "objects/nfcombobox.h"

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
 * function prototypes                                                         *
 ******************************************************************************/

void vw_create_group_bg(gchar *name, gint width, gint height);
void vw_obj_endis(NFOBJECT *obj, gboolean enable, gboolean expose);
NFOBJECT *vw_fixed_create(NFOBJECT *parent, gint bg, guint show,
		gint x, gint y, gint w, gint h, gpointer precb);
NFOBJECT *vw_label_create(NFOBJECT *parent, gchar *string, nffont_type font,
		nfalign_type align, guint margin, guint focus, guint show,
		gint fg, gint bg, gint x, gint y, gint w, gint h, gpointer postcb);
NFOBJECT *vw_ckbutton_lb_create(NFOBJECT *parent, gchar *string,
		gboolean active, gint fg, gint x, gint y, gpointer postcb_ckbtn);
NFOBJECT *vw_nmbutton_create(NFOBJECT *parent, gchar *string,
		gint type, gboolean enable, guint x, guint y, guint w, gpointer postcb);
NFOBJECT *vw_v_scbutton_create(NFOBJECT *parent, gboolean up,
		gint x, gint y, gpointer postcb);
NFOBJECT *vw_combo_create(NFOBJECT *parent, gchar **str, gint nitem, gint iidx,
		gchar *strlb, NFCOMBOBOX_TYPE type, guint show,
		gint x, gint y, gint w, gint h, gpointer postcb);
NFOBJECT *vw_table_create(NFOBJECT *parent, NFOBJECT **lb, gint cols, gint rows,
		gint colsp, gint rowsp, guint *cellw, gint cellh, gchar **strcol,
		gint bg, gint fg1, gint bg1, gint fg2, gint bg2,
		gint x, gint y, gpointer postcb);

#endif	/* _VW_TOOLS_H_ */

