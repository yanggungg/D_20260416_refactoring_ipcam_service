// vim:ts=4:sw=4
/*******************************************************************************
*  (c) COPYRIGHT 2012 ITX security                                             *
*  ALL RIGHT RESERVED                                                          *
*                                                                              *
*  Jongbin Yim,  jongbina@itxsecurity.com                                      *
********************************************************************************

REVISION HISTORY:

Date       Name           Description
__________ ______________ ______________________________________________________
2012/07/18 Jongbin Yim    Created.

................................................................................
DESCRIPTION:

................................................................................
*/

/**
 * @file  vw_vca_option_config.c
 * @brief  This file contains VCA option setup routines.
 */

/*******************************************************************************
 * includes                                                                    *
 ******************************************************************************/

#include "nf_afx.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"
#include "tools/nf_ui_tool.h"
#include "viewers/objects/nfcheckbutton.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nfwindow.h"
#include "vw_tools.h"
#include "vw_vca_option_config.h"

/*******************************************************************************
 * defines                                                                     *
 ******************************************************************************/

#define	OPT_WIN_X		(guint)((DISPLAY_ACTIVE_WIDTH - OPT_WIN_W) / 2)
#define	OPT_WIN_Y		(guint)((DISPLAY_ACTIVE_HEIGHT - OPT_WIN_H - 72) / 2)
#define	OPT_WIN_W		540
#define	OPT_WIN_H		420

#define	OPT_FXD_X		12
#define	OPT_FXD_Y		52
#define	OPT_FXD_W		(OPT_WIN_W - 12 * 2)
#define	OPT_FXD_H		(OPT_WIN_H - OPT_FXD_Y - 12)

#define	OPT_BTN1_X		((OPT_FXD_W - 10) / 2 - OPT_BTN_W)
#define	OPT_BTN2_X		((OPT_FXD_W + 10) / 2)
#define	OPT_BTN_Y		(OPT_FXD_H - OPT_FXD_Y)
#define	OPT_BTN_W		160

#define	NOPTS			3
#define	NOPTS1			4

/*******************************************************************************
 * typedefs                                                                    *
 ******************************************************************************/

/*******************************************************************************
 * globals                                                                     *
 ******************************************************************************/

/*******************************************************************************
 * locals                                                                      *
 ******************************************************************************/

static NFOBJECT *main_wnd;
static NFOBJECT *chk[NOPTS];
static NFOBJECT *radio[NOPTS1];
static NFOBJECT *btn_ok;
static NFOBJECT *btn_cancel;
static NFOBJECT *chk_vca_opt;
static NFOBJECT *radio_vca_opt[2];

static gboolean retval = FALSE;
static gboolean val[NOPTS];
static gboolean val1[NOPTS1];
static gboolean en_shadow_removal;
static gboolean track_refernce[2];

/*******************************************************************************
 * function declarations                                                       *
 ******************************************************************************/

static gboolean
post_mainwin_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if ( evt->type == GDK_DELETE )
		gtk_main_quit();
	return FALSE;
}

static gboolean
post_btn_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint i;

	if ( evt->type == GDK_BUTTON_RELEASE &&
			evt->button.button == MOUSE_LEFT_BUTTON ) {
		retval = obj == btn_ok;
		if ( retval ) {
			for (i = 0; i < NOPTS; i++)
				val[i] = nfui_check_button_get_active(NF_CHECKBUTTON(chk[i]));
		}
		nfui_nfobject_destroy(main_wnd);
	}
	return FALSE;
}

static gboolean
post_btn_cb_vca(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint i;

	if ( evt->type == GDK_BUTTON_RELEASE &&
			evt->button.button == MOUSE_LEFT_BUTTON ) {
		retval = obj == btn_ok;
		if ( retval ) {
			en_shadow_removal = nfui_check_button_get_active(NF_CHECKBUTTON(chk_vca_opt));
		}
		nfui_nfobject_destroy(main_wnd);
	}
	return FALSE;
}

static gboolean radio_opt_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_RADIO_GET_FOCUS_ALWAYS)
	{
		gint index, ret_val;

		index = nfui_radio_button_get_index(obj);

		switch(index)
		{
			case 0:
				val1[0] = TRUE;
				val1[1] = FALSE;
				val1[2] = FALSE;
				val1[3] = FALSE;			
			break;
			case 1:
				val1[0] = FALSE;
				val1[1] = TRUE;
				val1[2] = FALSE;
				val1[3] = FALSE;
			break;
			case 2:
				val1[0] = FALSE;
				val1[1] = FALSE;
				val1[2] = TRUE;
				val1[3] = FALSE;
			break;
			case 3:
				val1[0] = FALSE;
				val1[1] = FALSE;
				val1[2] = FALSE;
				val1[3] = TRUE;
			break;
			
			default:
				g_assert(0);
			break;
		}
		
		
		nfui_clear_key_focus(radio[index]);
		nfui_set_key_focus(radio[index], TRUE);
	
		nfui_radio_button_set_toggled(radio[index], TRUE);
		nfui_signal_emit(radio[index], GDK_EXPOSE, FALSE);		
	}

	return FALSE;
}

static gboolean radio_vca_opt_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_RADIO_GET_FOCUS_ALWAYS)
	{
		gint index, ret_val;

		index = nfui_radio_button_get_index(obj);

		switch(index)
		{
			case 0:
				track_refernce[0] = TRUE;
				track_refernce[1] = FALSE;
			break;
			case 1:
				track_refernce[0] = FALSE;
				track_refernce[1] = TRUE;
			break;
			
			default:
				g_assert(0);
			break;
		}
		
		
		nfui_clear_key_focus(radio_vca_opt[index]);
		nfui_set_key_focus(radio_vca_opt[index], TRUE);
	
		nfui_radio_button_set_toggled(radio_vca_opt[index], TRUE);
		nfui_signal_emit(radio_vca_opt[index], GDK_EXPOSE, FALSE);		
	}

	return FALSE;
}


gboolean
VW_VCA_Display_Option_Config_Open(NFWINDOW *parent, ivca_option_t*option)
{
	NFOBJECT *main_fixed, *fixed1, *obj;
	gint i;
	gchar *strOption[NOPTS] = {
		//"Enable Shadow Removal",
		//"Enable Event Snapshot",
		"Display Object Bounding Boxes",
		"Display Object Trajectories",
		"Display Rules"
	};
	gchar *strOption1[NOPTS1] = {
		"Display Object Ids",
		"Display Object Width (3D)",
		"Display Object Height (3D)",
		"Display Object Speed (3D)"
	};
	
	GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];
	gint size_w, size_h;
	GSList *slist = NULL;
	
	radio_img[0] = nfui_get_image_from_file((IMG_N_POPUP_RADIO_OFF), NULL);
	radio_img[1] = nfui_get_image_from_file((IMG_O_POPUP_RADIO_ON), NULL);
	radio_img[2] = nfui_get_image_from_file((IMG_P_POPUP_RADIO_ON), NULL);
	radio_img[3] = nfui_get_image_from_file((IMG_D_POPUP_RADIO_OFF), NULL);

	nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

	main_wnd = (NFOBJECT *)nftool_create_popup_window(parent,
			OPT_WIN_X, OPT_WIN_Y, OPT_WIN_W, OPT_WIN_H, "DISPLAY OPTIONS", FALSE);
	nfui_regi_post_event_callback(main_wnd, post_mainwin_cb);

	main_fixed = ((NFWINDOW *)main_wnd)->child;

	fixed1 = vw_fixed_create(main_fixed, -1, 1,
			OPT_FXD_X, OPT_FXD_Y, OPT_FXD_W, OPT_FXD_H, NULL);

	nfui_nfobject_show(main_fixed);
	nfui_nfobject_show(main_wnd);

//	val[0] = option->en_shadowrm;
//	val[1] = option->en_snapshot;
	val[0] = option->sw_obj_bb;
	val[1] = option->sw_obj_tr;
	val[2] = option->sw_rule;
	
	val1[0] = option->sw_obj_id;
	val1[1] = option->sw_obj_w3d;
	val1[2] = option->sw_obj_h3d;
	val1[3] = option->sw_obj_s3d;

	/* Check boxes. */
	for (i = 0; i < NOPTS; i++)
		chk[i] = vw_ckbutton_lb_create(fixed1, strOption[i], val[i],
				292, 10, 15 + i * 40, NULL);

	for (i=0 ; i< NOPTS1;i++){
		radio[i] = (NFOBJECT*)nfui_nfbutton_new();
		
		nfui_nfbutton_set_image(radio[i], radio_img);
		nfui_nfbutton_set_drawing_outline(radio[i], FALSE);
		nfui_nfobject_set_size(radio[i], (guint)size_w, (guint)size_h);
		nfui_nfobject_show(radio[i]);
		nfui_regi_post_event_callback(radio[i], radio_opt_cb);

		
		if (val1[i])
			nfui_radio_button_set_toggled(radio[i], TRUE);

		if (i == 0)
			slist = nfui_radio_button_get_group(radio[i]);
		else
			nfui_radio_button_add_group(radio[i], slist);
		
		nfui_nffixed_put(fixed1, radio[i], 10, 15 + NOPTS * 40 + i*40);

		vw_label_create(fixed1, strOption1[i], NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 4,
				0, 1, 292, -1, 10 + size_w, 15 + NOPTS * 40 + i*40, 0, size_h, NULL);

	}

	/* OK, CANCEL buttons. */
	btn_ok = vw_nmbutton_create(fixed1, "OK", 1, TRUE,
			OPT_BTN1_X, OPT_BTN_Y, OPT_BTN_W, post_btn_cb);
	btn_cancel = vw_nmbutton_create(fixed1, "CANCEL", 1, TRUE,
			OPT_BTN2_X, OPT_BTN_Y, OPT_BTN_W, post_btn_cb);

	nfui_make_key_hierarchy((NFWINDOW *)main_wnd);
	nfui_set_key_focus(btn_ok, TRUE);

	gtk_main();

	nfui_page_close(PGID_POPUPWND, main_wnd);

	if ( retval == TRUE ) {
		//option->en_shadowrm = val[0];
		//option->en_snapshot = val[1];
		option->sw_obj_bb = val[0];
		option->sw_obj_tr = val[1];
		option->sw_rule = val[2];
		option->sw_obj_id = val1[0];
		option->sw_obj_w3d = val1[1];
		option->sw_obj_h3d = val1[2];
		option->sw_obj_s3d = val1[3];
	}
	return retval;
}	

gboolean
VW_VCA_Option_Config_Open(NFWINDOW *parent, ivca_option_t*option)
{
	NFOBJECT *main_fixed, *fixed1, *obj;
	gint i;
	gchar *strOption[2] = {
		"Enable Shadow Removal",
		"Track Reference"
	};
	gchar *strOption1[2] = {
		"Centroid",
		"Ground Point"
	};
	
	GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];
	gint size_w, size_h;
	GSList *slist = NULL;
	
	radio_img[0] = nfui_get_image_from_file((IMG_N_POPUP_RADIO_OFF), NULL);
	radio_img[1] = nfui_get_image_from_file((IMG_O_POPUP_RADIO_ON), NULL);
	radio_img[2] = nfui_get_image_from_file((IMG_P_POPUP_RADIO_ON), NULL);
	radio_img[3] = nfui_get_image_from_file((IMG_D_POPUP_RADIO_OFF), NULL);

	nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

	main_wnd = (NFOBJECT *)nftool_create_popup_window(parent,
			OPT_WIN_X, OPT_WIN_Y, OPT_WIN_W, OPT_WIN_H, "VCA OPTIONS", FALSE);
	nfui_regi_post_event_callback(main_wnd, post_mainwin_cb);

	main_fixed = ((NFWINDOW *)main_wnd)->child;

	fixed1 = vw_fixed_create(main_fixed, -1, 1,
			OPT_FXD_X, OPT_FXD_Y, OPT_FXD_W, OPT_FXD_H, NULL);

	nfui_nfobject_show(main_fixed);
	nfui_nfobject_show(main_wnd);

	en_shadow_removal = option->en_shadowrm;

	if(option->track_ref){
		track_refernce[0] =0;
		track_refernce[1] =1;
	}
	else{
		track_refernce[0] =1;
		track_refernce[1] =0;
	}

	/* Check boxes. */
	chk_vca_opt = vw_ckbutton_lb_create(fixed1, strOption[0], en_shadow_removal,
				292, 10, 15, NULL);
	
	vw_label_create(fixed1, strOption[1], NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 4,
				0, 1, 292, -1, 10 , 15 + 40, 0, size_h, NULL);

	for (i=0 ; i< 2;i++){
		radio_vca_opt[i] = (NFOBJECT*)nfui_nfbutton_new();
		
		nfui_nfbutton_set_image(radio_vca_opt[i], radio_img);
		nfui_nfbutton_set_drawing_outline(radio_vca_opt[i], FALSE);
		nfui_nfobject_set_size(radio_vca_opt[i], (guint)size_w, (guint)size_h);
		nfui_nfobject_show(radio_vca_opt[i]);
		nfui_regi_post_event_callback(radio_vca_opt[i], radio_vca_opt_cb);

		if (track_refernce[i])
			nfui_radio_button_set_toggled(radio_vca_opt[i], TRUE);

		if (i == 0)
			slist = nfui_radio_button_get_group(radio_vca_opt[i]);
		else
			nfui_radio_button_add_group(radio_vca_opt[i], slist);
		
		nfui_nffixed_put(fixed1, radio_vca_opt[i], 10, 15 + 40*2 + i*40);

		vw_label_create(fixed1, strOption1[i], NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 4,
				0, 1, 292, -1, 10 + size_w, 15 + 40*2+ i*40, 0, size_h, NULL);

	}

	/* OK, CANCEL buttons. */
	btn_ok = vw_nmbutton_create(fixed1, "OK", 1, TRUE,
			OPT_BTN1_X, OPT_BTN_Y, OPT_BTN_W, post_btn_cb_vca);
	btn_cancel = vw_nmbutton_create(fixed1, "CANCEL", 1, TRUE,
			OPT_BTN2_X, OPT_BTN_Y, OPT_BTN_W, post_btn_cb_vca);

	nfui_make_key_hierarchy((NFWINDOW *)main_wnd);
	nfui_set_key_focus(btn_ok, TRUE);

	gtk_main();

	nfui_page_close(PGID_POPUPWND, main_wnd);

	if ( retval == TRUE ) {
	       option->en_shadowrm = en_shadow_removal;
		option->track_ref = track_refernce[1];
	}
	return retval;
}	/* VW_VCA_Option_Config_Open(... */


