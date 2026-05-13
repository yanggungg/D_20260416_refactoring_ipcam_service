#include "nf_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_page_manager.h"
#include "support/color.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfbutton.h"

#include "ssm.h"
#include "vw_playback_main.h"
#include "vw_playback_shortcut_menu.h"
#include "vw_playback_shortcut_submenu_camchange.h"
#include "vw_playback_statusbar.h"
#include "vw_timeline.h"
#include "vw_playback_control_box.h"
#include "uxm.h"
#include "vw_snapshot.h"

#include "nf_api_live.h"


#define MENU_SIZE_W					(289)
#define MENU_SIZE_H					(350)
#define MENU_FRONT_MARGIN			(8)
#define MENU_REAR_MARGIN			(14)
#define MENU_DEF_SPACE 				(MENU_FRONT_MARGIN + MENU_REAR_MARGIN)

#define MENU_BUTTON_SIZE_W			(267)
#define MENU_BUTTON_SIZE_H			(40)
#define MENU_BUTTON_DEF_SPACE		(MENU_TEXT_FRONT_MARGIN + MENU_TEXT_REAR_MARGIN) 
#define MENU_BUTTON_GAP_2			(2)

#define MENU_TEXT_FRONT_MARGIN		(15)
#define MENU_TEXT_REAR_MARGIN		(21)

#define MENU_GAP                    (2)


enum {
	ZOOM_MENU = 0,
	RESERVE_ARCH,
	SNAPSHOT_MENU,
//	PLAYBACK_NORMAL,
	FISHEYE_CTRL,
    CAM_CHANGE,
	FR_REGIST,
	LPR_REGIST,
	SHORTCUT_MENU_CNT
};

struct _SHORTCUT_MENU_CONF {
	guint wsize_w;									// window width 
	guint msize_w;									// button width

	GdkPixbuf *menu_bg;
	GdkPixbuf *menu_img[SHORTCUT_MENU_CNT][NFOBJECT_STATE_COUNT];	// images
	GdkPixbuf *submenu_img[2][NFOBJECT_STATE_COUNT];

	PB_SHORT_CUT_MODE_E mode;
};


typedef struct _SHORTCUT_MENU_CONF MENU_CONF;


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_menu_win = NULL;
static NFOBJECT *g_menu[SHORTCUT_MENU_CNT] = {0, };
static NFOBJECT *g_title;
static NFOBJECT *wait_pop = NULL;

static const gchar *g_strBtn[SHORTCUT_MENU_CNT] = {"ZOOM", "RESERVE START", "SNAPSHOT", "FISHEYE CTRL", "CAM CHANGE", "ADD FACE ", "ADD LICENSE PLATE"};

static gint g_menu_size_h = MENU_SIZE_H;

static gboolean init_menu_conf(MENU_CONF *conf);
static gboolean set_size(MENU_CONF *conf);
static gboolean set_image(MENU_CONF *conf);

static gint get_pressed_button(NFOBJECT *obj);
static void recal_open_position(gint *x, gint *y);
static gboolean post_menu_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data);
static gboolean post_menu_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data);
static gboolean post_menu_window_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data);

static guint g_ch = 0;
static guint g_channel_played = 0;

static gboolean init_menu_conf(MENU_CONF *conf)
{
	memset(conf, 0x00, sizeof(MENU_CONF));

	if(!set_size(conf))  return FALSE;
	if(!set_image(conf)) return FALSE;

	return TRUE;
}

static gboolean set_size(MENU_CONF *conf)
{
	guint str_w = 0;
	guint tmp_w = 0;
	gint i;

	for(i=0; i<SHORTCUT_MENU_CNT; i++) {
		if (!ivsc.dfunc.support_fisheye && i == FISHEYE_CTRL) continue;
		if (!ivsc.dfunc.support_face && i == FR_REGIST) continue;
		if (!ivsc.dfunc.support_license_plate && i == LPR_REGIST) continue;

		tmp_w = nfutil_string_width(1, NULL, nffont_get_pango_font(NFFONT_LARGE_SEMI), g_strBtn[i], NORMAL_SPACING);
		
		if(tmp_w > str_w)
			str_w = tmp_w;	
	}

	if(str_w == 0) return FALSE;

	// set button width
	if(str_w < MENU_BUTTON_SIZE_W) 	conf->msize_w = MENU_BUTTON_SIZE_W;
	else						   	conf->msize_w = (str_w + MENU_BUTTON_DEF_SPACE);
	
	// set window width
	if(conf->msize_w < MENU_SIZE_W) conf->wsize_w = MENU_SIZE_W;
	else						   	conf->wsize_w = (conf->msize_w + MENU_DEF_SPACE);

	return TRUE;
}

static gboolean set_image(MENU_CONF *conf)
{
	guint i;		
	GdkPixbuf *pbArrow[NFOBJECT_STATE_COUNT];

    pbArrow[0] = nfui_get_image_from_file((IMG_CLICK_POP_ARROW_N), NULL);
	pbArrow[1] = nfui_get_image_from_file((IMG_CLICK_POP_ARROW_O), NULL);
	pbArrow[2] = nfui_get_image_from_file((IMG_CLICK_POP_ARROW_P), NULL);
	pbArrow[3] = nfui_get_image_from_file((IMG_CLICK_POP_ARROW_D), NULL);
	

	// button bg
	for (i=0; i<SHORTCUT_MENU_CNT; i++)
	{
		if (!ivsc.dfunc.support_fisheye && i == FISHEYE_CTRL) continue;
		if (!ivsc.dfunc.support_face && i == FR_REGIST) continue;
		if (!ivsc.dfunc.support_license_plate && i == LPR_REGIST) continue;

		conf->menu_img[i][0] = nf_ui_create_image_button_no_alpha(MKB_IMG_PLAYBACK_SHORTCUT_N_BUTTON, conf->msize_w, IMG_N_CLICK_POPUP_MENU_L, IMG_N_CLICK_POPUP_MENU_M, IMG_N_CLICK_POPUP_MENU_R);
		conf->menu_img[i][1] = nf_ui_create_image_button_no_alpha(MKB_IMG_PLAYBACK_SHORTCUT_O_BUTTON, conf->msize_w, IMG_O_CLICK_POPUP_MENU_L, IMG_O_CLICK_POPUP_MENU_M, IMG_O_CLICK_POPUP_MENU_R);
		conf->menu_img[i][2] = nf_ui_create_image_button_no_alpha(MKB_IMG_PLAYBACK_SHORTCUT_P_BUTTON, conf->msize_w, IMG_P_CLICK_POPUP_MENU_L, IMG_P_CLICK_POPUP_MENU_M, IMG_P_CLICK_POPUP_MENU_R);
		conf->menu_img[i][3] = nf_ui_create_image_button_no_alpha(MKB_IMG_PLAYBACK_SHORTCUT_D_BUTTON, conf->msize_w, IMG_D_CLICK_POPUP_MENU_L, IMG_D_CLICK_POPUP_MENU_M, IMG_D_CLICK_POPUP_MENU_R);

		if( i == CAM_CHANGE || i == FISHEYE_CTRL) {
			conf->menu_img[i][0] = nf_ui_create_image_button_no_alpha(MK_IMG_LIVE_SHORTCUT_N_ARR_BUTTON, conf->msize_w, IMG_N_CLICK_POPUP_MENU_L, IMG_N_CLICK_POPUP_MENU_M, IMG_N_CLICK_POPUP_MENU_R);;
			conf->menu_img[i][1] = nf_ui_create_image_button_no_alpha(MK_IMG_LIVE_SHORTCUT_O_ARR_BUTTON, conf->msize_w, IMG_O_CLICK_POPUP_MENU_L, IMG_O_CLICK_POPUP_MENU_M, IMG_O_CLICK_POPUP_MENU_R);;
			conf->menu_img[i][2] = nf_ui_create_image_button_no_alpha(MK_IMG_LIVE_SHORTCUT_P_ARR_BUTTON, conf->msize_w, IMG_P_CLICK_POPUP_MENU_L, IMG_P_CLICK_POPUP_MENU_M, IMG_P_CLICK_POPUP_MENU_R);;
			conf->menu_img[i][3] = nf_ui_create_image_button_no_alpha(MK_IMG_LIVE_SHORTCUT_D_ARR_BUTTON, conf->msize_w, IMG_D_CLICK_POPUP_MENU_L, IMG_D_CLICK_POPUP_MENU_M, IMG_D_CLICK_POPUP_MENU_R);;

			gdk_pixbuf_composite(pbArrow[0], conf->menu_img[i][0], (MENU_BUTTON_SIZE_W - 21), 13, 7, 13, (gfloat)(MENU_BUTTON_SIZE_W - 21), 13.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
			gdk_pixbuf_composite(pbArrow[1], conf->menu_img[i][1], (MENU_BUTTON_SIZE_W - 21), 13, 7, 13, (gfloat)(MENU_BUTTON_SIZE_W - 21), 13.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
			gdk_pixbuf_composite(pbArrow[2], conf->menu_img[i][2], (MENU_BUTTON_SIZE_W - 21), 13, 7, 13, (gfloat)(MENU_BUTTON_SIZE_W - 21), 13.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
			gdk_pixbuf_composite(pbArrow[3], conf->menu_img[i][3], (MENU_BUTTON_SIZE_W - 21), 13, 7, 13, (gfloat)(MENU_BUTTON_SIZE_W - 21), 13.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
		}
	}

	conf->submenu_img[0][0] = nf_ui_create_image_button_no_alpha(MK_IMG_LIVE_SHORTCUT_N_PB_BUTTON, 212, IMG_N_CLICK_POPUP_MENU_L, IMG_N_CLICK_POPUP_MENU_M, IMG_N_CLICK_POPUP_MENU_R);;
	conf->submenu_img[0][1] = nf_ui_create_image_button_no_alpha(MK_IMG_LIVE_SHORTCUT_O_PB_BUTTON, 212, IMG_O_CLICK_POPUP_MENU_L, IMG_O_CLICK_POPUP_MENU_M, IMG_O_CLICK_POPUP_MENU_R);;
	conf->submenu_img[0][2] = nf_ui_create_image_button_no_alpha(MK_IMG_LIVE_SHORTCUT_P_PB_BUTTON, 212, IMG_P_CLICK_POPUP_MENU_L, IMG_P_CLICK_POPUP_MENU_M, IMG_P_CLICK_POPUP_MENU_R);;
	conf->submenu_img[0][3] = nf_ui_create_image_button_no_alpha(MK_IMG_LIVE_SHORTCUT_D_PB_BUTTON, 212, IMG_D_CLICK_POPUP_MENU_L, IMG_D_CLICK_POPUP_MENU_M, IMG_D_CLICK_POPUP_MENU_R);;

	conf->submenu_img[1][0] = nf_ui_create_image_button_no_alpha(MK_IMG_LIVE_SHORTCUT_N_SWT_BUTTON, 72, IMG_N_CLICK_POPUP_MENU_L, IMG_N_CLICK_POPUP_MENU_M, IMG_N_CLICK_POPUP_MENU_R);;
	conf->submenu_img[1][1] = nf_ui_create_image_button_no_alpha(MK_IMG_LIVE_SHORTCUT_O_SWT_BUTTON, 72, IMG_O_CLICK_POPUP_MENU_L, IMG_O_CLICK_POPUP_MENU_M, IMG_O_CLICK_POPUP_MENU_R);;
	conf->submenu_img[1][2] = nf_ui_create_image_button_no_alpha(MK_IMG_LIVE_SHORTCUT_P_SWT_BUTTON, 72, IMG_P_CLICK_POPUP_MENU_L, IMG_P_CLICK_POPUP_MENU_M, IMG_P_CLICK_POPUP_MENU_R);;
	conf->submenu_img[1][3] = nf_ui_create_image_button_no_alpha(MK_IMG_LIVE_SHORTCUT_D_SWT_BUTTON, 72, IMG_D_CLICK_POPUP_MENU_L, IMG_D_CLICK_POPUP_MENU_M, IMG_D_CLICK_POPUP_MENU_R);;

	return TRUE;
}

static void _change_zoombutton(gint status)
{
	NFOBJECT* obj;

	obj = (NFOBJECT*)nfui_nfobject_get_data(g_menu_win, "zoom button");

	if (status)
	{
		if (nfui_nfobject_is_disabled(obj))
		{
			nfui_nfobject_enable(obj);
			nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
		}
	}
	else
	{
		if (!nfui_nfobject_is_disabled(obj))
		{
			nfui_nfobject_disable(obj);
			nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
		}
	}	
}

static void recal_open_position(gint *x, gint *y)
{
	guint size_w = g_menu_win->width;

	if((*x) > (DISPLAY_ACTIVE_WIDTH - size_w)) 		  (*x) -= size_w;
	if((*y) > (DISPLAY_ACTIVE_HEIGHT - g_menu_size_h))  (*y) -= g_menu_size_h;
}

static gint get_pressed_button(NFOBJECT *obj)
{
	gint i = 0;

	while(i < SHORTCUT_MENU_CNT) {
		if(g_menu[i] == obj) 
			return i;

		++i;
	}

	return -1;
}

static guint _get_menu_channel()
{
	return g_ch;
}

static void _set_menu_channel(guint ch)
{
	g_ch = ch;
}

static gboolean post_menu_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    switch(evt->type){
        case GDK_BUTTON_PRESS:
        {
		gint menu_idx;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) 			 
			return FALSE;

		menu_idx = get_pressed_button(obj);

		if(menu_idx < 0) 
			return FALSE;

	    nfui_nfobject_hide(g_menu_win);


		switch(menu_idx) {
		    case CAM_CHANGE:
	        break;
		    case FISHEYE_CTRL:
	        break;			
			case ZOOM_MENU:
			if (vsm_get_omode() == OMODE_NORMAL)
			{
				vw_playback_statusbar_hide();
				VW_Timeline_Hide();
				vw_playback_full_zoom_func();
				vw_playback_statusbar_show();
				VW_Timeline_Show();				
			}
			else
			{
				vw_playback_control_box_Hide();
				vw_playback_full_zoom_func();
				vw_playback_control_box_Show();
			}				
			break;
			case RESERVE_ARCH:
				vw_playback_archive_func();
			break;
			case SNAPSHOT_MENU:
			{
			    gint ch;
			    
			    ch = _get_menu_channel();

                if (vw_playback_snap_func(ch))
                {
    				wait_pop = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
    				uxm_reg_imsg_event(g_menu_win, INFY_CAPTURE_IMAGE);
    				uxm_monitor_on_imsg_event(g_menu_win, INFY_CAPTURE_IMAGE);
                }
            }
			break;
#if 0			
			case PLAYBACK_NORMAL:
			break;
#endif			
			case FR_REGIST:
			{
			    gint ch;
			    
			    ch = _get_menu_channel();
                if (vw_playback_notime_snap_msg_func(ch, INFY_AI_QUICK_REGIST_CAPTURE))
                {
    				wait_pop = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
    				uxm_reg_imsg_event(g_menu_win, INFY_AI_QUICK_REGIST_CAPTURE);
    				uxm_monitor_on_imsg_event(g_menu_win, INFY_AI_QUICK_REGIST_CAPTURE);
                }
			}
			break;

			case LPR_REGIST:
			{
				
			}
			break;

			default: 				
				return FALSE;
		}

		cam_change_enable_obj_pb();
		nfui_nfobject_hide(g_menu_win);
       
	}
	break;

	case GDK_ENTER_NOTIFY:
	case GDK_MOTION_NOTIFY:
        {
            gint menu_idx;

            menu_idx = get_pressed_button(obj);       

            if(menu_idx < 0)
                return FALSE;

            if (menu_idx == CAM_CHANGE) {
				VW_show_cc_submenu_pb();
			}
			else if(menu_idx == FISHEYE_CTRL) {
				VW_show_fisheye_submenu(g_menu[FISHEYE_CTRL]);
			}
        }
        break;

    default :
        break;
    }

	return FALSE;
}

static gboolean post_menu_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if(evt->type == GDK_EXPOSE) {
    	drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
    
		nfui_nfobject_gc_unref(gc);
	}
    else if (evt->type == GDK_DELETE)
    {
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
    }

	return FALSE;
}

static gboolean post_menu_window_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case NFOUTEVT_BUTTON_PRESS:
		{
		    cam_change_enable_obj_pb(); 
			nfui_nfobject_hide(obj);
	    }
		break;

		case INFY_DIV_CHANGE:
		{
			CMM_MESSAGE_T *pmsg;
			gint div;
			
			pmsg = (CMM_MESSAGE_T *)data;			
			div = GPOINTER_TO_INT(pmsg->data);
			
			if (div == VSM_DIV1)
				_change_zoombutton(1);
			else
				_change_zoombutton(0);
		}
		break;

		case INFY_CAPTURE_IMAGE:
		{
			gint result = ((CMM_MESSAGE_T *)data)->param;
			
			uxm_unreg_imsg_event(g_menu_win, INFY_CAPTURE_IMAGE);

			if (wait_pop) {
				nftool_remove_waitbox(wait_pop);
				wait_pop = NULL;
			}

			if (result == 0) 
			{
				CAPTURE_IMAGE_T *image = ((CMM_MESSAGE_T *)data)->data;
				SNAPSHOT_INFO_T info;
	
				info.ch = image->ch;
				info.time = image->time;
				info.size = image->size;
				info.width = image->width;
				info.height = image->height;
				info.buffer = image->buffer;
			
				VW_Snapshot_Open(g_curwnd, &info, SS_MODE_BURN_RESERVE);
			}
			else
				nftool_mbox(g_curwnd, "NOTICE", "SNAPSHOT FAIL", NFTOOL_MB_OK);		

			vsm_playback_play_recover_by_menu_closed();
		}
		break;

		case INFY_AI_QUICK_REGIST_CAPTURE:
		{
			gint result = ((CMM_MESSAGE_T *)data)->param;
			
			uxm_unreg_imsg_event(g_menu_win, INFY_AI_QUICK_REGIST_CAPTURE);

			if (wait_pop) {
				nftool_remove_waitbox(wait_pop);
				wait_pop = NULL;
			}

			if (result == 0) 
			{
				CAPTURE_IMAGE_T *image = ((CMM_MESSAGE_T *)data)->data;
				SNAPSHOT_INFO_T info;
	
				info.ch = image->ch;
				info.time = image->time;
				info.size = image->size;
				info.width = image->width;
				info.height = image->height;
				info.buffer = image->buffer;			
				vw_quick_add_face_popup_open(g_curwnd, info.ch, info.buffer, info.size, TRUE);
			}
			else
				nftool_mbox(g_curwnd, "NOTICE", "SNAPSHOT FAIL", NFTOOL_MB_OK);		

			vsm_playback_play_recover_by_menu_closed();
		}
		break;

		case GDK_DELETE:
		{
			uxm_unreg_imsg_event(g_menu_win, INFY_DIV_CHANGE);			
			uxm_unreg_imsg_event(g_menu_win, INFY_CAPTURE_IMAGE);	
			uxm_unreg_imsg_event(g_menu_win, INFY_AI_QUICK_REGIST_CAPTURE);	

			VW_destroy_camchange_submenu_pb();

			g_menu_win = 0;
			g_curwnd = 0;
		}
		break;
		
		default:
		break;		
	}
	return FALSE;
}


NFOBJECT* vw_playback_shortcut_menu_open(NFWINDOW *parent)
{
	NFOBJECT *menu_fixed = NULL;
	MENU_CONF mconf; 

	gint i;
	guint pos_x, pos_y;
	guint size_w, size_h;
	guint font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(340), 
											COLOR_IDX(341), 
											COLOR_IDX(343), 
											COLOR_IDX(344)};


	if (!ivsc.dfunc.support_fisheye)
		g_menu_size_h = MENU_SIZE_H - 44;
	if (!ivsc.dfunc.support_face)
		g_menu_size_h -= 40;
	if (!ivsc.dfunc.support_license_plate)
		g_menu_size_h -= 40;

	/* init menu configuration */
	if(!init_menu_conf(&mconf))
		return FALSE;

	g_channel_played = 0;

	_set_menu_channel(0);

	/* window */
	g_menu_win = (NFOBJECT*)nfui_nfwindow_new(parent, 0, 0, mconf.wsize_w, g_menu_size_h);
	nfui_nfobject_modify_bg(g_menu_win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_regi_post_event_callback(g_menu_win, post_menu_window_event_cb);
	nfui_nfwindow_use_outside_evt((NFWINDOW*)g_menu_win, TRUE);
	nfui_nfwindow_set_mask((NFWINDOW*)g_menu_win, GDK_BUTTON_PRESS, TRUE);
	g_curwnd = g_menu_win;
	nfui_nfwindow_set_title(g_curwnd, "PLAYBACK SHORTCUT");
	gtk_widget_add_events(((NFWINDOW*)g_menu_win)->main_widget, GDK_POINTER_MOTION_HINT_MASK);


	/* fixed */
	menu_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(menu_fixed, mconf.wsize_w, g_menu_size_h);
	nfui_regi_post_event_callback(menu_fixed, post_menu_fixed_event_cb);
	nfui_nfobject_show(menu_fixed);


	/* label */
	pos_x = MENU_FRONT_MARGIN;

	g_title = (NFOBJECT*)nfui_nflabel_new_with_pango_font("TITLE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
	nfui_nflabel_set_align((NFLABEL*)g_title, NFALIGN_LEFT, MENU_TEXT_FRONT_MARGIN);
	nfui_nfobject_set_size(g_title, mconf.msize_w , (guint)38);
	nfui_nfobject_modify_bg(g_title, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(g_title, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(g_title);
	nfui_nffixed_put((NFFIXED*)menu_fixed, g_title, pos_x, 4);

	/* button */
	pos_y = 49;

	for(i = 0; i < SHORTCUT_MENU_CNT; i++) {
		if (!ivsc.dfunc.support_fisheye && i == FISHEYE_CTRL) continue;
		if (!ivsc.dfunc.support_face && i == FR_REGIST) continue;
		if (!ivsc.dfunc.support_license_plate && i == LPR_REGIST) continue;

	g_assert(GDK_IS_PIXBUF(mconf.menu_img[i][0]));
		g_menu[i] = (NFOBJECT*)nfui_nfbutton_new_with_param(mconf.menu_img[i], g_strBtn[i]);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(g_menu[i]), NFALIGN_LEFT, MENU_TEXT_FRONT_MARGIN);
		nfui_nfbutton_set_pango_font((NFBUTTON*)g_menu[i], nffont_get_pango_font(NFFONT_LARGE_SEMI), (guint*)font_color);
		nfui_nfobject_set_size(g_menu[i], mconf.msize_w, MENU_BUTTON_SIZE_H);
		nfui_regi_post_event_callback(g_menu[i], post_menu_button_event_cb);
		nfui_nfobject_show(g_menu[i]);

		if(i > 0) pos_y += (MENU_BUTTON_SIZE_H + MENU_BUTTON_GAP_2);

		nfui_nffixed_put((NFFIXED*)menu_fixed, g_menu[i], pos_x, pos_y);

		if (i == ZOOM_MENU)
		{
			nfui_nfobject_disable(g_menu[i]);
			nfui_nfobject_set_data(g_menu_win, "zoom button", g_menu[i]);
		}
		else if (i == SNAPSHOT_MENU)
		{
#ifndef _SUPPORT_SNAPSHOT
			nfui_nfobject_disable(g_menu[i]);
#endif
			if(!DAL_get_support_snapshot())
				nfui_nfobject_disable(g_menu[SNAPSHOT_MENU]);
		}
		else if (i == RESERVE_ARCH)
		{
		    if (!ssm_check_access_auth(USR_AUTH_ARCHIVE))
		        nfui_nfobject_disable(g_menu[i]);
		}
	}

	uxm_reg_imsg_event(g_menu_win, INFY_DIV_CHANGE);
	uxm_monitor_on_imsg_event(g_menu_win, INFY_DIV_CHANGE);

	nfui_nfwindow_add((NFWINDOW*)g_menu_win, menu_fixed);
	
	nfui_nfobject_hide(g_menu_win);
	nfui_run_main_event_handler(g_menu_win);

	VW_create_camchange_submenu_pb(g_curwnd, g_menu[CAM_CHANGE]);

	return g_menu_win;
}

static void _get_cc_submenu_sy(gint *sy, gint y)
{
    gint tmp;
	gint cnt;
	gint cc_submenu_h = 0;

	cnt = GUI_CHANNEL_CNT > 16 ? 16 : GUI_CHANNEL_CNT;
	cc_submenu_h = 44 * cnt + 16;

    tmp = *sy;

    tmp = (y + 44 + MENU_SIZE_H) - (cc_submenu_h); 
    
    if(cc_submenu_h > MENU_SIZE_H - 44)
    {
        tmp = y + 44;  
    }

    if(tmp + (cc_submenu_h) > DISPLAY_ACTIVE_HEIGHT) 
    {
        tmp = (y + 44 + MENU_SIZE_H) - (cc_submenu_h); 
    }

    if(tmp + (cc_submenu_h) > DISPLAY_ACTIVE_HEIGHT)
    {
        tmp = DISPLAY_ACTIVE_HEIGHT - (cc_submenu_h);
    }
    
    if(tmp < 0)
    { 
        tmp = 0;
    }
    
    *sy = tmp;
}

void vw_playback_shortcut_menu_show(gint x, gint y, guint ch)
{
	CameraData cam_d;
	int sx,sy;

	CamItxFisheyeData fisheye_data;

	if (g_menu_win == NULL)
	{
		g_message("%s, %d playback shortcut menu window didn't create", __FUNCTION__, __LINE__);
		return;
	}

    if (vsm_get_div() != VSM_DIV1)
		nfui_nfobject_enable(g_menu[CAM_CHANGE]);
	else
		nfui_nfobject_disable(g_menu[CAM_CHANGE]);

	_set_menu_channel(ch);

	recal_open_position(&x, &y);
	nfui_nfobject_move(g_menu_win, (guint)x, (guint)y);

	if (x + MENU_SIZE_W + 220 + MENU_GAP > DISPLAY_ACTIVE_WIDTH) sx = x - (220 + MENU_GAP);
	else sx = (guint)x + MENU_SIZE_W + MENU_GAP;

    _get_cc_submenu_sy(&sy, y);

	if (ivsc.dfunc.support_fisheye) {
		VW_move_fisheye_submenu(sx, (guint)y + (44 * 4) - 3);
	}

    VW_move_cc_submenu_pb(sx, sy);

	DAL_get_camera_data(&cam_d, ch);
	nfui_nflabel_set_text((NFLABEL*)g_title, cam_d.title);

	if (ivsc.dfunc.support_fisheye) {
		nfui_nfobject_disable(g_menu[FISHEYE_CTRL]);

		if (nf_live_fisheye_is_support(ch) && (g_channel_played & (1 << ch)))
		{
			memset(&fisheye_data, 0x00, sizeof(CamItxFisheyeData));
			DAL_get_camera_itx_fisheye_data(&fisheye_data, ch);
			if (fisheye_data.act) nfui_nfobject_enable(g_menu[FISHEYE_CTRL]);
		}
	}

	nfui_nfobject_show(g_menu_win);
	nfui_make_key_hierarchy((NFWINDOW*)g_menu_win);
}

gboolean vw_playback_shortcut_menu_is_shown()
{
	return nfui_nfobject_is_shown(g_menu_win);
}

void vw_playback_shortcut_menu_hide()
{
	if (g_menu_win) nfui_nfobject_hide(g_menu_win);
}

void vw_playback_shortcut_menu_refresh()
{
	nfui_nfobject_hide(g_menu_win);
	nfui_nfobject_show(g_menu_win);
}

void vw_playback_shortcut_change_bookmark_text(gint status)
{
	if (status)
		nfui_nfbutton_set_text((NFLABEL*)g_menu[RESERVE_ARCH], "RESERVE STOP");
	else
		nfui_nfbutton_set_text((NFLABEL*)g_menu[RESERVE_ARCH], "RESERVE START");
}

void vw_playback_shortcut_change_snap(gint enable)
{
#ifdef _SUPPORT_SNAPSHOT
	if (enable) nfui_nfobject_enable(g_menu[SNAPSHOT_MENU]);
	else		nfui_nfobject_disable(g_menu[SNAPSHOT_MENU]);

	if(!DAL_get_support_snapshot())
		nfui_nfobject_disable(g_menu[SNAPSHOT_MENU]);
#endif		
}

void vw_playback_shortcut_set_played(gint ch, gint status)
{
	if (status == NF_PLAY_STATUS_NONE)
	{
		g_channel_played |= (1 << ch);
	}
	else
	{
		g_channel_played &= ~(1 << ch);
	}
}

void  VW_ShortCut_Menu_Pos_pb(int *x, int*y)
{
    *x = g_menu_win->x;
    *y = g_menu_win->y;
}

void VW_ShortCut_Menu_Size_pb(int *w, int *h)
{
    *w = g_menu_win->width;
    *h = g_menu_win->height;
}

/*inline*/ guint get_menu_channel_pb()
{
	return g_ch;
}
