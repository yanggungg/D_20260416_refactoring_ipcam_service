
#include "iux_msg.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"
#include "support/multi_language_support.h"

#include "tools/nf_ui_tool.h"

#include "services/scm.h"

#include "modules/ssm.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nftimespin.h"
#include "objects/nfcheckbutton.h"
#include "objects/nflistbox.h"
#include "objects/nfspinbutton.h"
#include "objects/nfbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nfimage.h"

#include "vw_sys_net_info_detail_popup.h"
#include "nf_api_ipcam.h"
#if defined(_IPX_MODEL_UX)
#include "nf_ipcam_defs.h"
#else
#include "ipx_cam_api.h"
#endif

#include "vsm.h"
#include "uxm.h"




#define DETAIL_WIN_W			(1111)
#define DETAIL_WIN_H			(789)
#define DETAIL_WIN_X			(guint)((DISPLAY_ACTIVE_WIDTH - DETAIL_WIN_W)/2)
#define DETAIL_WIN_Y			(guint)((DISPLAY_ACTIVE_HEIGHT - DETAIL_WIN_H)/2)

#define DETAIL_CONFIG_FIXED_X	(20)
#define DETAIL_CONFIG_FIXED_Y	(63)
#define DETAIL_CONFIG_FIXED_W	(505)
#define DETAIL_CONFIG_FIXED_H	(445)

#define DETAIL_DIAG_FIXED_X		(DETAIL_CONFIG_FIXED_X)
#define DETAIL_DIAG_FIXED_Y		(DETAIL_CONFIG_FIXED_Y + DETAIL_CONFIG_FIXED_H + 15)
#define DETAIL_DIAG_FIXED_W		(DETAIL_CONFIG_FIXED_W)
#define DETAIL_DIAG_FIXED_H		(216)

#define DETAIL_INFO_FIXED_X		(DETAIL_CONFIG_FIXED_X + DETAIL_CONFIG_FIXED_W + 20)
#define DETAIL_INFO_FIXED_Y		(DETAIL_CONFIG_FIXED_Y)
#define DETAIL_INFO_FIXED_W		(522)
#define DETAIL_INFO_FIXED_H		(262)

#define DETAIL_PREVIEW_FIXED_X	(DETAIL_INFO_FIXED_X)
#define DETAIL_PREVIEW_FIXED_Y	(DETAIL_INFO_FIXED_Y + DETAIL_INFO_FIXED_H)
#define DETAIL_PREVIEW_FIXED_W	(DETAIL_INFO_FIXED_W)
#define DETAIL_PREVIEW_FIXED_H	(382)

enum {
	CONFIG_CAM_NAME = 0,
	CONFIG_IP_ADDR,
	CONFIG_USER_NAME,
	CONFIG_PASS,
	CONFIG_HTTP,
	CONFIG_RTSP,
	CONFIG_PROTOCOL,
	CONFIG_VIDEO_CH,
	NUM_CONFIG_COLS
};

enum {
	DIAG_ALARM_IN = 0,
	DIAG_ALARM_OUT,
	DIAG_IPCAM,
	NUM_DIAG_COLS
};

enum {
	INFO_MODEL = 0,
	INFO_MAC,
	INFO_FW_VER,
	INFO_ALARM_IN,
	INFO_ALARM_OUT,
	INFO_VIDEO,	
	INFO_AUDIO_IN,	
	INFO_AUDIO_OUT,	
	NUM_INFO_COLS
};

enum {
	PREVIEW_CONNECT_TEST = 0,
	PREVIEW_ROTATE,
	NUM_PREVIEW_COLS
};

static NFIPCamConfInfo	ipcam_config;
static NFIPCamModelInfo	ipcam_model;
static NFWINDOW *g_curwnd = 0;


static gboolean _detail_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if(event->type == GDK_EXPOSE)
	{
    	drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
    
		nfui_nfobject_gc_unref(gc);
	} 
    else if (event->type == GDK_DELETE)
    {
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
    }
	
	return FALSE;
}

static gboolean _detail_main_win_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if(event->type == GDK_DELETE) 
	{
		g_curwnd = 0;
		gtk_main_quit();
	}
	return FALSE;
}

static gboolean _detail_preview_fixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if(event->type == GDK_EXPOSE)
	{
		guint obj_x, obj_y;
		guint win_x, win_y;

		gint ch;
		guint ch_mask = 0;

		nfui_nfobject_get_window_pos(obj, &win_x, &win_y);
		nfui_nfobject_get_offset(obj, &obj_x, &obj_y);

		ch = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "detail channel"));

		ch_mask |= (1 << ch);		
		vsm_live_preview_start(ch_mask, win_x+obj_x+18, win_y+obj_y+60, 411, 226);		
	} 
	else if(event->type == GDK_DELETE) 
	{
		vsm_live_preview_stop();
	}
	
	return FALSE;
}


static gboolean 
_ipcam_combo_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if(event->type == NFEVENT_COMBOBOX_CHANGED) 
	{
		char *pdata = nfui_combobox_get_value(obj);

		printf("=================================================== \n");
		printf("=================================================== \n");
		printf("=================================================== \n");
		printf("=================================================== \n");
		printf("NFEVENT_COMBOBOX_CHANGED %s , CH : %d \n", pdata, ipcam_model.ch);
		

		if(!strcmp("0", pdata))
		{
			printf("000000000000000000000000000000000000000000 \n");
			nf_ipcam_set_rotation(ipcam_model.ch, 0, NULL, NULL, NULL);
		}
		else
		{
			printf("nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn \n");
			nf_ipcam_set_rotation(ipcam_model.ch, 1, NULL, NULL, NULL);
		}

		printf("=================================================== \n");
		printf("=================================================== \n");
		printf("=================================================== \n");
		printf("=================================================== \n");
		
	}

	return FALSE;
	
}

static gboolean 
_ipcam_reset_btn_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	static NFOBJECT *wait_mbox = NULL;

	switch(event->type) {
		case GDK_BUTTON_RELEASE:
			{
				if(event->button.button == MOUSE_RIGTH_BUTTON)
					return FALSE;

				printf("=================================================== \n");
				printf("=================================================== \n");
				printf("=================================================== \n");
				printf("CH : %d \n", ipcam_model.ch);
				printf("=================================================== \n");
				printf("=================================================== \n");
				printf("=================================================== \n");

				if(!wait_mbox)
					wait_mbox = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");

				if(scm_cam_reset(ipcam_model.ch) < 0) 
				{
					g_message("%s [%d] : scm_cam_reset returns -1", __FUNCTION__, __LINE__);

					nftool_remove_waitbox((NFOBJECT*)wait_mbox);
					wait_mbox = NULL;

				// SKSHIN
				//	nftool_mbox(g_curwnd, "ERROR", "Reset failed.", NFTOOL_MB_OK);

					return FALSE;
				}
			}
			break;

		case INFY_IPCAM_RESET_BEGIN:
			break;

		case INFY_IPCAM_RESET_PENDING:
			break;
            
        case INFY_IPCAM_RESET_END:
        case INFY_IPCAM_RESET_REQ_FAIL:
			{
				if(wait_mbox) {
					nftool_remove_waitbox((NFOBJECT*)wait_mbox);
					wait_mbox = NULL;
				}

				// SKSHIN
				//nftool_mbox(g_curwnd, "ERROR", "Reset failed.", NFTOOL_MB_OK);
			}
			break;

		case INFY_IPCAM_RESET_TIMEOUT:
			{
				if(wait_mbox) {
					nftool_remove_waitbox((NFOBJECT*)wait_mbox);
					wait_mbox = NULL;
				}

				nftool_mbox(g_curwnd, "ERROR", "Reset timeout.", NFTOOL_MB_OK);
			}
			break;

		case INFY_PND_NOTIFY:
			{
				NF_NOTIFY_INFO *pInfo = NULL;

				if(data) {
					pInfo = ((NF_NOTIFY_INFO *)data);

					if(ipcam_model.ch != pInfo->d.params[1])
						break;

					switch(pInfo->d.params[0]) {
						case PND_TYPE_VIDEO_START:
							if(wait_mbox) {
								nftool_remove_waitbox((NFOBJECT*)wait_mbox);
								wait_mbox = NULL;
							}
							break;

						case PND_TYPE_UNKNOWN:
						case PND_TYPE_UNSUPPORTED:
						case PND_TYPE_CONNECTION_FAIL:
						case PND_TYPE_LOGIN_FAIL:
						case PND_TYPE_CONFIG_FAIL:
							if(wait_mbox) {
								nftool_remove_waitbox((NFOBJECT*)wait_mbox);
								wait_mbox = NULL;
							}

							// SKSHIN
							//nftool_mbox(g_curwnd, "ERROR", "Reset failed.", NFTOOL_MB_OK);
							break;
					}
				}
			}
			break;

		case GDK_DELETE:
			uxm_unreg_imsg_event(obj, INFY_IPCAM_RESET_BEGIN);
			uxm_unreg_imsg_event(obj, INFY_IPCAM_RESET_END);
			uxm_unreg_imsg_event(obj, INFY_IPCAM_RESET_PENDING);
			uxm_unreg_imsg_event(obj, INFY_IPCAM_RESET_REQ_FAIL);
			uxm_unreg_imsg_event(obj, INFY_IPCAM_RESET_TIMEOUT);
			uxm_unreg_imsg_event(obj, INFY_PND_NOTIFY);
			break;

		default:
			break;
	}
	
	return FALSE;
}

static gboolean _detail_okbtn_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *topwin;

	if(event->type == GDK_BUTTON_RELEASE)
	{
		if(event->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}



	
		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);	
	}
	
	return FALSE;
}

static gboolean _detail_cancelbtn_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *topwin;

	if(event->type == GDK_BUTTON_RELEASE)
	{
		if(event->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}
	
		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);	
	}
	
	return FALSE;
}

///////////////////////////////////////////////////////////////////
//
//
//
void open_vw_sys_net_detail_popup(NFWINDOW *parent, gint ch)
{
	NFOBJECT *win = NULL;
	NFOBJECT *main_fixed = NULL;
	NFOBJECT *config_fixed = NULL;
	NFOBJECT *diag_fixed = NULL;
	NFOBJECT *info_fixed = NULL;
	NFOBJECT *preview_fixed = NULL;	
	NFOBJECT *obj = NULL;

	GSList *slist = NULL;
	GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];

	CameraData camdata;
	
	const gchar *config_str[] = {"CAMERA NAME", "IP ADDRESS", "USER NAME", "PASSWORD", 
								"HTTP PORT", "RTSP PORT", "PROTOCOL", "VIDEO CHANNEL"};
	const gchar *protocol_radio_str[] = {"TCP", "HTTP", "UDP", "UDP MULTICAST"};
	
	const gchar *diag_str[] = {"ALARM IN STATUS", "ALARM OUT TEST", "IP CAMERA RESET"};
	const gchar *info_str[] = {"MODEL", "MAC ADDRESS", "F/W VERSION", "ALARM INPUT", 
								"ALARM OUT", "VIDEO", "AUDIO", "AUDIO OUTPUT",};
	
	const gchar *preview_str[] = {"CONNECTION TEST", "ROTATE IMAGE"};
	const gchar *combo_str[] = {"0", "180"};

	guint font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(394), COLOR_IDX(394), COLOR_IDX(393), COLOR_IDX(394)};


	gint i, j;
	gint pos_x, pos_y;
	gint size_w, size_h;

	gint ret_val;

	radio_img[0] = nfui_get_image_from_file((IMG_N_POPUP_RADIO_OFF), NULL);
	radio_img[1] = nfui_get_image_from_file((IMG_O_POPUP_RADIO_ON), NULL);
	radio_img[2] = nfui_get_image_from_file((IMG_P_POPUP_RADIO_ON), NULL);
	radio_img[3] = nfui_get_image_from_file((IMG_D_POPUP_RADIO_OFF), NULL);

	DAL_get_camera_data(&camdata, ch);
	printf("=================================================== \n");
	printf("=================================================== \n");
	printf("=================================================== \n");
	printf("CH : %d \n", ch);
	printf("=================================================== \n");
	printf("=================================================== \n");
	printf("=================================================== \n");

	ipcam_model.ch = ch;
	ret_val = nf_ipcam_get_model_info(ch, &ipcam_model, NULL);
	
	printf("=================================================== \n");
	printf("=================================================== \n");
	printf("=================================================== \n");
	printf("CH : %d , ipcam_model.ch : %d \n", ch, ipcam_model.ch);
	printf("=================================================== \n");
	printf("=================================================== \n");
	printf("=================================================== \n");	


	if (ret_val != 1)
		g_warning("%s, %d, warning_ret :%d", __FUNCTION__, __LINE__, ret_val);

	ret_val = nf_ipcam_get_config_info(ch, &ipcam_config, NULL);

	if (ret_val != 1)
		g_warning("%s, %d, warning_ret :%d", __FUNCTION__, __LINE__, ret_val);

	win = (NFOBJECT*)nfui_nfwindow_new(parent, DETAIL_WIN_X, DETAIL_WIN_Y, DETAIL_WIN_W, DETAIL_WIN_H);
	g_curwnd = win;
	nfui_regi_post_event_callback(win, _detail_main_win_event_handler);

	main_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(main_fixed, DETAIL_WIN_W, DETAIL_WIN_H);
	nfui_regi_post_event_callback(main_fixed, _detail_main_fixed_event_handler);
	nfui_nfobject_show(main_fixed);

	config_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(config_fixed, DETAIL_CONFIG_FIXED_W, DETAIL_CONFIG_FIXED_H);
	nfui_nffixed_put((NFFIXED*)main_fixed, config_fixed, DETAIL_CONFIG_FIXED_X, DETAIL_CONFIG_FIXED_Y);
	nfui_nfobject_show(config_fixed);

	diag_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(diag_fixed, DETAIL_DIAG_FIXED_W, DETAIL_DIAG_FIXED_H);
	nfui_nffixed_put((NFFIXED*)main_fixed, diag_fixed, DETAIL_DIAG_FIXED_X, DETAIL_DIAG_FIXED_Y);	
	nfui_nfobject_show(diag_fixed);

	info_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(info_fixed, DETAIL_INFO_FIXED_W, DETAIL_INFO_FIXED_H);
	nfui_nffixed_put((NFFIXED*)main_fixed, info_fixed, DETAIL_INFO_FIXED_X, DETAIL_INFO_FIXED_Y);	
	nfui_nfobject_show(info_fixed);

	preview_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(preview_fixed, DETAIL_PREVIEW_FIXED_W, DETAIL_PREVIEW_FIXED_H);
	nfui_regi_post_event_callback(preview_fixed, _detail_preview_fixed_event_handler);
	nfui_nffixed_put((NFFIXED*)main_fixed, preview_fixed, DETAIL_PREVIEW_FIXED_X, DETAIL_PREVIEW_FIXED_Y);
	nfui_nfobject_show(preview_fixed);

	nfui_nfobject_set_data(preview_fixed, "detail channel", GINT_TO_POINTER(ch));

// <---- MAIN FIXED TITLE
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("IP CAMERA CONFIGURATION", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, DETAIL_WIN_W-47, 36);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 20, 4);


// <---- CONFIGURATION FIXED
	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "CONFIGURATION");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)config_fixed, obj, 0, 0);

	pos_x = 0;
	pos_y = 61;

	for (i = 0; i < NUM_CONFIG_COLS; i++)
	{
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(config_str[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
		nfui_nfobject_set_size(obj, 200, 40);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 17);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)config_fixed, obj, pos_x, pos_y);

		pos_x += 200;

		if (i == CONFIG_CAM_NAME)
		{
			obj = (NFOBJECT*)nfui_nflabel_new_text_box(camdata.title, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
			nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
			nfui_nfobject_support_multi_lang(obj, FALSE);
			nfui_nfobject_set_size(obj, 220, 40);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)config_fixed, obj, pos_x, pos_y);	
		}
		else if (i == CONFIG_IP_ADDR)
		{	
			obj = (NFOBJECT*)nfui_nflabel_new_text_box(ipcam_config.ipaddr, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
			nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
			nfui_nfobject_set_size(obj, 220, 40);
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
			nfui_nfobject_use_focus(obj, FALSE);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)config_fixed, obj, pos_x, pos_y);

			pos_x += 230;

#if 0		// seongho
			obj = nftool_normal_button_create_type3("PING", 80);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)config_fixed, obj, pos_x, pos_y);
#endif
		}
		else if (i == CONFIG_USER_NAME)
		{
			obj = (NFOBJECT*)nfui_nflabel_new_text_box(ipcam_config.username, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
			nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);			
			nfui_nfobject_set_size(obj, 220, 40);
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
			nfui_nfobject_use_focus(obj, FALSE);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)config_fixed, obj, pos_x, pos_y);
		}
		else if (i == CONFIG_PASS)
		{
			guint len;
			gchar str[16];

			memset(str, 0x00, sizeof(str));
#if 1 	// ipx mantis. 9741.
			len = 10;
#else
			len = strlen(ipcam_config.password);
#endif			


			for (j = 0; j < len; j++)
			{
				strcat(str, "*");
			}
		
			obj = (NFOBJECT*)nfui_nflabel_new_text_box(str, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
			nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
			nfui_nfobject_set_size(obj, 220, 40);
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
			nfui_nfobject_use_focus(obj, FALSE);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)config_fixed, obj, pos_x, pos_y);
		}
		else if (i == CONFIG_HTTP)
		{
			gchar str[8];

			sprintf(str, "%d", ipcam_config.http_port);
		
			obj = (NFOBJECT*)nfui_nflabel_new_text_box(str, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
			nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
			nfui_nfobject_set_size(obj, 220, 40);
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
			nfui_nfobject_use_focus(obj, FALSE);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)config_fixed, obj, pos_x, pos_y);
		}
		else if (i == CONFIG_RTSP)
		{
			gchar str[8];

			sprintf(str, "%d", ipcam_config.rtsp_port);
		
			obj = (NFOBJECT*)nfui_nflabel_new_text_box(str, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
			nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
			nfui_nfobject_set_size(obj, 220, 40);
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
			nfui_nfobject_use_focus(obj, FALSE);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)config_fixed, obj, pos_x, pos_y);
		}
		else if (i == CONFIG_PROTOCOL)
		{
			gint group_x, group_y;
			
			group_x = pos_x;
			group_y = pos_y;
			
			nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);
		
			for(j = 0; j < 4; j++) {
				obj = (NFOBJECT*)nfui_nfbutton_new();
				nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
				nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
				nfui_nfobject_use_focus(obj, FALSE);
				nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
				nfui_nfobject_show(obj);

				if(j == 0) {
					slist = nfui_radio_button_get_group(NF_BUTTON(obj));
					nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);
				} else 	   {
					nfui_nfobject_disable(obj);
					nfui_radio_button_add_group(NF_BUTTON(obj), slist);
				}

				nfui_nffixed_put((NFFIXED*)config_fixed, obj, group_x, group_y+(40-size_h)/2);

				group_x += (size_w + 4);

				obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(protocol_radio_str[j], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
				nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
				if (j == 3)	nfui_nfobject_set_size(obj, 180, 40);
				else		nfui_nfobject_set_size(obj, 80, 40);
				nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
				nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
				nfui_nfobject_show(obj);
				nfui_nffixed_put((NFFIXED*)config_fixed, obj, group_x, group_y);

				if (j == 1)
				{
					group_x = pos_x;
					group_y += 40;
				}
				else
				{
					group_x += (80 + 4);
				}				
			}
		}
		else		// CONFIG_VIDEO_CH
		{
			gchar buf[4];

			nfui_get_image_size(IMG_N_CH_SQ_ACTIVE, &size_w, &size_h);
		
			for(j = 0; j < ipcam_config.video; j++) 
			{
				g_sprintf(buf, "%d", j + 1);
				//g_sprintf(buf, "%d", ch + 1);

				obj = nfui_nfimage_new(IMG_N_CH_SQ_ACTIVE);
				nfui_nfimage_set_text((NFIMAGE*)obj, buf);
				nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(417));
				nfui_nfobject_use_focus(obj, FALSE);
				nfui_nfobject_show(obj);
				nfui_nffixed_put((NFFIXED*)config_fixed, obj, pos_x + ((size_w+2)*j), pos_y+(40-size_h)/2);
			}
		}

		pos_x = 0;

		if (i == CONFIG_PROTOCOL)
			pos_y += (43*2);
		else
			pos_y += 43;
	}


// <---- DIAGNOSTIC TOOLS FIXED
	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "DIAGNOSTIC TOOLS");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)diag_fixed, obj, 0, 0);

	pos_x = 0;
	pos_y = 61;

	for (i = 0; i < NUM_DIAG_COLS; i++)
	{
		if(nftool_cur_language_is_japanese())
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(diag_str[i], nffont_get_pango_font(NFFONT_MINI_SEMI_5), COLOR_IDX(292));
		else
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(diag_str[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));		
		nfui_nfobject_set_size(obj, 250, 40);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 17);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)diag_fixed, obj, pos_x, pos_y);

		pos_x += 250;

		if (i == DIAG_ALARM_IN)
		{
			gchar buf[4];

			nfui_get_image_size(IMG_CH_ALARM_OFF, &size_w, &size_h);

			for(j = 0; j < ipcam_config.alarm_in; j++) {
				g_sprintf(buf, "%d", j + 1);
		
				obj = nfui_nfimage_new(IMG_CH_ALARM_ON);
				nfui_nfimage_set_text((NFIMAGE*)obj, buf);
				nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(394));
				nfui_nfobject_use_focus(obj, FALSE);
				nfui_nfobject_show(obj);

				nfui_nffixed_put((NFFIXED*)diag_fixed, obj, pos_x + ((size_w+2)*j), pos_y+(40-size_h)/2);
			}
		}
		else if (i == DIAG_ALARM_OUT)
		{
			gchar buf[4];

			nfui_get_image_size(IMG_N_CH_SQ_ACTIVE, &size_w, &size_h);
		
			for(j = 0; j < ipcam_config.alarm_out; j++) {
				g_sprintf(buf, "%d", j + 1);
				obj = nfui_nfimage_new(IMG_N_CH_SQ_ACTIVE);
				nfui_nfimage_set_text((NFIMAGE*)obj, buf);
				nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(394));
				nfui_nfobject_use_focus(obj, FALSE);
				nfui_nfobject_show(obj);		
				nfui_nffixed_put((NFFIXED*)diag_fixed, obj, pos_x + ((size_w+2)*j), pos_y+(40-size_h)/2);
			}
		}
		else
		{	
			obj = nftool_normal_button_create_popup_type1("RESET", 150);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)diag_fixed, obj, pos_x, pos_y);
			nfui_regi_post_event_callback(obj, _ipcam_reset_btn_event_handler);			

			
			uxm_reg_imsg_event(obj, INFY_IPCAM_RESET_BEGIN);
			uxm_reg_imsg_event(obj, INFY_IPCAM_RESET_END);
			uxm_reg_imsg_event(obj, INFY_IPCAM_RESET_PENDING);
			uxm_reg_imsg_event(obj, INFY_IPCAM_RESET_REQ_FAIL);
			uxm_reg_imsg_event(obj, INFY_IPCAM_RESET_TIMEOUT);
			uxm_reg_imsg_event(obj, INFY_PND_NOTIFY);
		}

		pos_x = 0;
		pos_y += 43;
	}


// <---- CAMERA INFO FIXED
	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "CAMERA INFO");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)info_fixed, obj, 0, 0);

	pos_x = 0;
	pos_y = 61;

	for (i = 0; i < NUM_INFO_COLS; i++)
	{
		gchar buf[128];

		memset(buf, 0x00, sizeof(buf));

		if (i == INFO_MODEL)
		{
			g_sprintf(buf, "%s : %s", lookup_string(info_str[i]), ipcam_model.name);
		}
		else if (i == INFO_MAC)
		{
			g_sprintf(buf, "%s : %02x:%02x:%02x:%02x:%02x:%02x", lookup_string(info_str[i]), 
				ipcam_model.mac[0], ipcam_model.mac[1], ipcam_model.mac[2],
				ipcam_model.mac[3], ipcam_model.mac[4], ipcam_model.mac[5]);
		}
		else if (i == INFO_FW_VER)
		{
			g_sprintf(buf, "%s : %s", lookup_string(info_str[i]), ipcam_model.swver);
		}
		else if (i == INFO_ALARM_IN)
		{
			g_sprintf(buf, "%s : %d", lookup_string(info_str[i]), ipcam_config.alarm_in);
		}
		else if (i == INFO_ALARM_OUT)
		{
			g_sprintf(buf, "%s : %d", lookup_string(info_str[i]), ipcam_config.alarm_out);
		}
		else if (i == INFO_VIDEO)
		{
			g_sprintf(buf, "%s : %d", lookup_string(info_str[i]), ipcam_config.video);
		}
		else if (i == INFO_AUDIO_IN)
		{
			g_sprintf(buf, "%s : %d", lookup_string(info_str[i]), ipcam_config.audio_in);
		}
		else 	//INFO_AUDIO_OUT
		{
			g_sprintf(buf, "%s : %d", lookup_string(info_str[i]), ipcam_config.alarm_out);
		}
	
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(buf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(293));
		nfui_nfobject_set_size(obj, 450, 24);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 17);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)info_fixed, obj, pos_x, pos_y);

		pos_y += 24;
	}

// <---- PREVIEW FIXED
	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "PREVIEW");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)preview_fixed, obj, 0, 0);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_PRG_IDX(UX_COLOR_000000));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
	nfui_nfobject_set_size(obj, 406, 226);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)preview_fixed, obj, 18, 60);

	obj = nftool_normal_button_create_type1("OK", 192);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 360, 739);
	nfui_regi_post_event_callback(obj, _detail_okbtn_event_handler);

	obj = nftool_normal_button_create_type1("CANCEL", 192);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 360+192+4, 739);
	nfui_regi_post_event_callback(obj, _detail_cancelbtn_event_handler);
	
	nfui_nfwindow_add((NFWINDOW*)win, main_fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);
	nfui_make_key_hierarchy(win);

	nfui_page_open(PGID_IPCAM_PROP, win, ssm_get_cur_id(NULL));

	gtk_main();

	nfui_page_close(PGID_IPCAM_PROP, win);
}


