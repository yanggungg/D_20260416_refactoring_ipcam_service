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
#include "objects/nflabel.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"

#include "services/vsm.h"

#include "modules/ssm.h"

#include "vw.h"
#include "vw_live_shortcut_menu.h"
#include "vw_live_shortcut_submenu_pb.h"
#include "vw_live_shortcut_submenu_camchange.h"
#include "vw_live_shortcut_submenu_ptz.h"
#include "vw_zoom_pip.h"
#include "vw_timeline.h"
#include "vw_set_date_time.h"
#include "vw_live_statusbar.h"
#include "vw_snapshot.h"
#include "vw_playback_main.h"
#include "uxm.h"
#include "smt.h"

#include "nf_api_live.h"
#include "nf_sysman.h"

#define MENU_SIZE_W					(289)
#define MENU_SIZE_H					(516)
#define MENU_FRONT_MARGIN			(8)
#define MENU_REAR_MARGIN			(14)
#define MENU_DEF_SPACE 				(MENU_FRONT_MARGIN + MENU_REAR_MARGIN)

#define MENU_BUTTON_SIZE_W			(267)
#define MENU_BUTTON_SIZE_H			(40)
#define MENU_BUTTON_DEF_SPACE		(MENU_TEXT_FRONT_MARGIN + MENU_TEXT_REAR_MARGIN) 
#define MENU_BUTTON_GAP_2			(2)

#define MENU_TEXT_FRONT_MARGIN		(15)
#define MENU_TEXT_REAR_MARGIN		(21)

#define MENU_GAP					2




static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_parent = 0;
static NFOBJECT *g_menu_win = NULL;
static NFOBJECT *g_menu[MENU_CNT] = {0, };
static NFOBJECT *g_title;
static NFOBJECT *wait_pop = NULL;


static guint g_freeze_mask = 0;
static guint g_mic_mask = 0;
static guint g_ch = 0;

static const gchar *g_strBtn[MENU_CNT] = {"PLAYBACK", "ZOOM", "SNAPSHOT", "AUDIO ON", "MIC ON", "PTZ CTRL", "ONE PUSH", "FISHEYE CTRL", "CAM CHANGE", "ADD FACE ", "ADD LICENSE PLATE"};

static gint g_menu_size_h = MENU_SIZE_H;

static gboolean init_menu_conf(MENU_CONF *conf);
static gboolean set_size(MENU_CONF *conf);
static gboolean set_image(MENU_CONF *conf);

static gint get_event_button(NFOBJECT *obj);

static void recal_open_position(gint *x, gint *y);

static gboolean freeze_is_on(guint ch);
static void freeze_on(guint ch);
static void freeze_off(guint ch);

static /*inline*/ void set_menu_channel(guint ch);



static /*inline*/ void set_menu_channel(guint ch)
{
	g_ch = ch;
}

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

	for(i=0; i<MENU_CNT; i++) {
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

	// arrow 
	pbArrow[0] = nfui_get_image_from_file((IMG_CLICK_POP_ARROW_N), NULL);
	pbArrow[1] = nfui_get_image_from_file((IMG_CLICK_POP_ARROW_O), NULL);
	pbArrow[2] = nfui_get_image_from_file((IMG_CLICK_POP_ARROW_P), NULL);
	pbArrow[3] = nfui_get_image_from_file((IMG_CLICK_POP_ARROW_D), NULL);

	// button bg
	for (i=0; i<MENU_CNT; i++)
	{
		if (!ivsc.dfunc.support_fisheye && i == FISHEYE_CTRL) continue;
		if (!ivsc.dfunc.support_face && i == FR_REGIST) continue;
		if (!ivsc.dfunc.support_license_plate && i == LPR_REGIST) continue;

		conf->menu_img[i][0] = nf_ui_create_image_button_no_alpha(MK_IMG_LIVE_SHORTCUT_N_BUTTON, conf->msize_w, IMG_N_CLICK_POPUP_MENU_L, IMG_N_CLICK_POPUP_MENU_M, IMG_N_CLICK_POPUP_MENU_R);;
		conf->menu_img[i][1] = nf_ui_create_image_button_no_alpha(MK_IMG_LIVE_SHORTCUT_O_BUTTON, conf->msize_w, IMG_O_CLICK_POPUP_MENU_L, IMG_O_CLICK_POPUP_MENU_M, IMG_O_CLICK_POPUP_MENU_R);;
		conf->menu_img[i][2] = nf_ui_create_image_button_no_alpha(MK_IMG_LIVE_SHORTCUT_P_BUTTON, conf->msize_w, IMG_P_CLICK_POPUP_MENU_L, IMG_P_CLICK_POPUP_MENU_M, IMG_P_CLICK_POPUP_MENU_R);;
		conf->menu_img[i][3] = nf_ui_create_image_button_no_alpha(MK_IMG_LIVE_SHORTCUT_D_BUTTON, conf->msize_w, IMG_D_CLICK_POPUP_MENU_L, IMG_D_CLICK_POPUP_MENU_M, IMG_D_CLICK_POPUP_MENU_R);;

		if(i == PLAYBACK_MENU || i == CAM_CHANGE || i == PTZ_CTRL || i == FISHEYE_CTRL) {
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

	conf->submenu_img[0][0] = nf_ui_create_image_button_no_alpha(MK_IMG_LIVE_SHORTCUT_N_PB_BUTTON, 212, IMG_N_CLICK_POPUP_MENU_L, IMG_N_CLICK_POPUP_MENU_M, IMG_N_CLICK_POPUP_MENU_R);
	conf->submenu_img[0][1] = nf_ui_create_image_button_no_alpha(MK_IMG_LIVE_SHORTCUT_O_PB_BUTTON, 212, IMG_O_CLICK_POPUP_MENU_L, IMG_O_CLICK_POPUP_MENU_M, IMG_O_CLICK_POPUP_MENU_R);
	conf->submenu_img[0][2] = nf_ui_create_image_button_no_alpha(MK_IMG_LIVE_SHORTCUT_P_PB_BUTTON, 212, IMG_P_CLICK_POPUP_MENU_L, IMG_P_CLICK_POPUP_MENU_M, IMG_P_CLICK_POPUP_MENU_R);
	conf->submenu_img[0][3] = nf_ui_create_image_button_no_alpha(MK_IMG_LIVE_SHORTCUT_D_PB_BUTTON, 212, IMG_D_CLICK_POPUP_MENU_L, IMG_D_CLICK_POPUP_MENU_M, IMG_D_CLICK_POPUP_MENU_R);

	conf->submenu_img[1][0] = nf_ui_create_image_button_no_alpha(MK_IMG_LIVE_SHORTCUT_N_SWT_BUTTON, 72, IMG_N_CLICK_POPUP_MENU_L, IMG_N_CLICK_POPUP_MENU_M, IMG_N_CLICK_POPUP_MENU_R);
	conf->submenu_img[1][1] = nf_ui_create_image_button_no_alpha(MK_IMG_LIVE_SHORTCUT_O_SWT_BUTTON, 72, IMG_O_CLICK_POPUP_MENU_L, IMG_O_CLICK_POPUP_MENU_M, IMG_O_CLICK_POPUP_MENU_R);
	conf->submenu_img[1][2] = nf_ui_create_image_button_no_alpha(MK_IMG_LIVE_SHORTCUT_P_SWT_BUTTON, 72, IMG_P_CLICK_POPUP_MENU_L, IMG_P_CLICK_POPUP_MENU_M, IMG_P_CLICK_POPUP_MENU_R);
	conf->submenu_img[1][3] = nf_ui_create_image_button_no_alpha(MK_IMG_LIVE_SHORTCUT_D_SWT_BUTTON, 72, IMG_D_CLICK_POPUP_MENU_L, IMG_D_CLICK_POPUP_MENU_M, IMG_D_CLICK_POPUP_MENU_R);

	conf->submenu_img[2][0] = nf_ui_create_image_button_no_alpha(MK_IMG_LIVE_SHORTCUT_N_FE_BUTTON, 312, IMG_N_CLICK_POPUP_MENU_L, IMG_N_CLICK_POPUP_MENU_M, IMG_N_CLICK_POPUP_MENU_R);
	conf->submenu_img[2][1] = nf_ui_create_image_button_no_alpha(MK_IMG_LIVE_SHORTCUT_O_FE_BUTTON, 312, IMG_O_CLICK_POPUP_MENU_L, IMG_O_CLICK_POPUP_MENU_M, IMG_O_CLICK_POPUP_MENU_R);
	conf->submenu_img[2][2] = nf_ui_create_image_button_no_alpha(MK_IMG_LIVE_SHORTCUT_P_FE_BUTTON, 312, IMG_P_CLICK_POPUP_MENU_L, IMG_P_CLICK_POPUP_MENU_M, IMG_P_CLICK_POPUP_MENU_R);
	conf->submenu_img[2][3] = nf_ui_create_image_button_no_alpha(MK_IMG_LIVE_SHORTCUT_D_FE_BUTTON, 312, IMG_D_CLICK_POPUP_MENU_L, IMG_D_CLICK_POPUP_MENU_M, IMG_D_CLICK_POPUP_MENU_R);

	return TRUE;
}


static void recal_open_position(gint *x, gint *y)
{
	guint size_w = g_menu_win->width;

	if((*x) > (DISPLAY_ACTIVE_WIDTH - size_w)) 		  (*x) -= size_w;
	if((*y) > (DISPLAY_ACTIVE_HEIGHT - g_menu_size_h))  (*y) -= g_menu_size_h;
}

static gint get_event_button(NFOBJECT *obj)
{
	gint i = 0;

	while(i < MENU_CNT) {
		if(g_menu[i] == obj) 
			return i;

		++i;
	}

	return -1;
}

static gboolean freeze_is_on(guint ch) 
{
	if(g_freeze_mask & (1 << ch)) 
		return TRUE;

	return FALSE;
}

static void freeze_on(guint ch)
{
	g_freeze_mask |= (1 << ch);

	//TODO: freeze api
}

static void freeze_off(guint ch)
{
	g_freeze_mask &= ~(1 << ch);

	//TODO: freeze api
}

static gboolean audio_is_on(guint ch)
{
	if(scm_get_cur_live_audio_ch() == ch)
		return TRUE;
	return FALSE;
}

static void audio_on(guint ch)
{
	scm_change_live_audio(ch);
}

static void audio_off(void)
{
	scm_change_live_audio(0xff);
}

static gboolean mic_is_on(guint ch)
{
	if(g_mic_mask & (1 << ch))
		return TRUE;
	return FALSE;
}

static void mic_on(guint ch)
{
	gchar stat[16];
	gint i;

	g_mic_mask |= (1 << ch);

	memset(stat, 0x00, sizeof(stat));
	for(i=0; i<16; i++) {
		if(g_mic_mask & (1 << i))
			stat[i] = 0x01;
	    else stat[i] = 0x00;
	}

	scm_enable_mic_ch(stat, 16);
}

static void mic_off(guint ch)
{
	gchar stat[16];
	gint i;

	g_mic_mask &= ~(1 << ch);

	memset(stat, 0x00, sizeof(stat));
	for(i=0; i<16; i++) {
		if(g_mic_mask & (1 << i))
			stat[i] = 0x01;
	    else stat[i] = 0x00;
	}

	scm_enable_mic_ch(stat, 16);
}

static gboolean post_menu_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch (evt->type) {
		case GDK_BUTTON_PRESS:
			{
				gint menu_idx;

				if(evt->button.button == MOUSE_RIGTH_BUTTON) 			 
					return FALSE;

				menu_idx = get_event_button(obj);
				if(menu_idx < 0) 
					return FALSE;

				switch(menu_idx) {
					case PLAYBACK_MENU:
						break;
				    case CAM_CHANGE:
				        break;

				    case PTZ_CTRL:
				        break;

				    case FISHEYE_CTRL:
				        break;

					case ZOOM_MENU:
						{
#if 0		// v2.0
							guint pos_x = 0, pos_y = 0;
							guint prop = 0;

							nfui_nfobject_hide(g_menu_win);

							vsm_get_zoom_pip_pos(get_menu_channel(), VW_ZoomPIP_Width(), VW_ZoomPIP_Height(), &pos_x, &pos_y);
							prop = (ZOOM_PIP_MODALESS | ZOOM_PIP_MOVING | ZOOM_PIP_LIVE);

							VW_ZoomPIP_Open(g_curwnd, pos_x, pos_y, get_menu_channel(), prop);
#else
							gint x, y;


							nfui_nfobject_hide(g_menu_win);

							if(vsm_get_covert_state(NULL, get_menu_channel())) {
								nftool_mbox(g_curwnd, "ERROR", "Current channel is coverted.",  NFTOOL_MB_OK);
								break;
							}

							x = DISPLAY_ACTIVE_WIDTH - VW_ZoomPIP_Width();
							y = DISPLAY_ACTIVE_HEIGHT - VW_ZoomPIP_Height();

							VW_Live_StatusBar_Hide();
							VW_Timeline_Hide();
							VW_ZoomPIP_Open(g_curwnd, x, y, get_menu_channel(), ZOOM_PIP_NONE);
							
							if (vsm_get_omode() == OMODE_NORMAL)
							{
								VW_Live_StatusBar_Show();
								VW_Timeline_Show();
							}

#endif
						}
						break;

#if 0			// v2.0
					case FREEZE_MENU:
						{
							if(freeze_is_on(get_menu_channel())) {
								freeze_off(get_menu_channel());

								nfui_nfbutton_set_text((NFBUTTON*)g_menu[FREEZE_MENU], "FREEZE ON");
							}else {
								freeze_on(get_menu_channel());

								nfui_nfbutton_set_text((NFBUTTON*)g_menu[FREEZE_MENU], "FREEZE OFF");
							}
						}
						break;
#endif

					case SNAPSHOT_MENU:
						{
							gint retVal;

							if(vsm_get_covert_state(NULL, get_menu_channel())) {
								nfui_nfobject_hide(g_menu_win);
								nftool_mbox(g_curwnd, "ERROR", "Current channel is coverted.",  NFTOOL_MB_OK);
								return FALSE;
							}

							nfui_nfobject_hide(g_menu_win);

							retVal = scm_req_live_capture(INFY_CAPTURE_IMAGE, get_menu_channel());
							if (retVal == 0)
							{
							    wait_pop = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
							    uxm_reg_imsg_event(g_menu_win, INFY_CAPTURE_IMAGE);
							    uxm_monitor_on_imsg_event(g_menu_win, INFY_CAPTURE_IMAGE);							
							}
							else
							{
								nftool_mbox(g_curwnd, "NOTICE", "SNAPSHOT FAIL", NFTOOL_MB_OK);
							}							
						}
						break;

					case AUDIO_MENU:
						{
							if(audio_is_on(get_menu_channel()))
							{
								audio_off();
								nfui_nfbutton_set_text((NFBUTTON*)g_menu[AUDIO_MENU], "AUDIO ON");
							} 
							else 
							{
#if !defined(_HDI_MODEL_UX) && !defined(_DVR_MODEL_UX)
								BITMASK connect = 0;
								connect = scm_get_cam_conn_state();
								if(!(connect & (1 << get_menu_channel()))){					
									nfui_nfobject_hide(g_menu_win);
									nftool_mbox(g_curwnd, "ERROR", "NO VIDEO", NFTOOL_MB_OK);				
								}
								else{
									if(scm_get_audio_in_supp(get_menu_channel()) == -1) {
										nfui_nfobject_hide(g_menu_win);
										nftool_mbox(g_curwnd, "ERROR", "Current channel does not support audio.", NFTOOL_MB_OK);
										break;
									}
								}
#endif								
								audio_on(get_menu_channel());
								nfui_nfbutton_set_text((NFBUTTON*)g_menu[AUDIO_MENU], "AUDIO OFF");
							}
						}
						break;

#if !defined(_HDI_MODEL_UX) && !defined(_DVR_MODEL_UX)
					case MIC_MENU:
						{
							if(mic_is_on(get_menu_channel()))
							{
								mic_off(get_menu_channel());

								nfui_nfbutton_set_text((NFBUTTON*)g_menu[MIC_MENU], "MIC ON");
							} 
							else 
							{
								BITMASK connect = 0;
								connect = scm_get_cam_conn_state();
								if(!(connect & (1 << get_menu_channel()))){					
									nfui_nfobject_hide(g_menu_win);
									nftool_mbox(g_curwnd, "ERROR", "NO VIDEO", NFTOOL_MB_OK);				
								}
								else{
									if(scm_get_mic_out_supp(get_menu_channel()) == -1) {
										nfui_nfobject_hide(g_menu_win);
										nftool_mbox(g_curwnd, "ERROR", "Current channel does not support MIC.", NFTOOL_MB_OK);
										break;
									}
								}
								mic_on(get_menu_channel());
								nfui_nfbutton_set_text((NFBUTTON*)g_menu[MIC_MENU], "MIC OFF");
							}
						}
						break;
#endif
					case ONEPUSH_MENU:
						{
							if(scm_req_ipcam_onepush(get_menu_channel()) < 0) 
							{
								vw_mbox(g_curwnd, "ERROR", IMBX_IPCAM_ONEPUSH_FAIL, NFTOOL_MB_OK);
								return FALSE;
							}
						}
						break;
						
					case FR_REGIST:
						{
							gint retVal;

							if(vsm_get_covert_state(NULL, get_menu_channel())) {
								nfui_nfobject_hide(g_menu_win);
								nftool_mbox(g_curwnd, "ERROR", "Current channel is coverted.",  NFTOOL_MB_OK);
								return FALSE;
							}

							nfui_nfobject_hide(g_menu_win);

							retVal = scm_req_live_capture_without_time(INFY_AI_QUICK_REGIST_CAPTURE, get_menu_channel());
							if (retVal == 0)
							{
							    wait_pop = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
							    uxm_reg_imsg_event(g_menu_win, INFY_AI_QUICK_REGIST_CAPTURE);
							    uxm_monitor_on_imsg_event(g_menu_win, INFY_AI_QUICK_REGIST_CAPTURE);							
							}
							else
							{
								nftool_mbox(g_curwnd, "NOTICE", "SNAPSHOT FAIL", NFTOOL_MB_OK);
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

				cam_change_enable_obj();
				nfui_nfobject_hide(g_menu_win);
			}
			break;

		case GDK_ENTER_NOTIFY:
		case GDK_MOTION_NOTIFY:
			{
				gint menu_idx;

				menu_idx = get_event_button(obj);
				if(menu_idx < 0) 
					return FALSE;

				if(menu_idx == PLAYBACK_MENU){
					VW_show_pb_submenu();
				}
			    else if(menu_idx == CAM_CHANGE){
					VW_show_cc_submenu();
				}
			    else if(menu_idx == PTZ_CTRL){
					VW_show_ptz_submenu();
				}
				else if(menu_idx == FISHEYE_CTRL){
					VW_show_fisheye_submenu(g_menu[FISHEYE_CTRL]);
				}
			}
			break;

		default:
			break;
	}

	return FALSE;
}

static void change_shortcut_status_by_usr_auth()
{
	if((!ssm_check_access_auth(USR_AUTH_AUDIO)) || (!DAL_get_support_audio()))
		nfui_nfobject_disable(g_menu[AUDIO_MENU]);
	else
		nfui_nfobject_enable(g_menu[AUDIO_MENU]);
	nfui_signal_emit(g_menu[AUDIO_MENU], GDK_EXPOSE, FALSE);

#if !defined(_HDI_MODEL_UX) && !defined(_DVR_MODEL_UX)
	if((!ssm_check_access_auth(USR_AUTH_MIC)) || (!DAL_get_support_audio()) || (nf_sysman_get_pba_type() == NF_SYSMAN_PBA_TYPE_B))
		nfui_nfobject_disable(g_menu[MIC_MENU]);
	else
		nfui_nfobject_enable(g_menu[MIC_MENU]);
	nfui_signal_emit(g_menu[MIC_MENU], GDK_EXPOSE, FALSE);
#endif	
}

static void update_audio_on_off_btn()
{
	if(audio_is_on(get_menu_channel()))
		nfui_nfbutton_set_text((NFBUTTON*)g_menu[AUDIO_MENU], "AUDIO OFF");
	else
		nfui_nfbutton_set_text((NFBUTTON*)g_menu[AUDIO_MENU], "AUDIO ON");

	nfui_signal_emit(g_menu[AUDIO_MENU], GDK_EXPOSE, FALSE);

}

static gboolean post_menu_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	switch(evt->type)
	{
		case GDK_EXPOSE:
			{
        		drawable = nfui_nfobject_get_window(obj);
        		gc = nfui_nfobject_get_gc(obj);

                nfui_nfobject_get_size(obj, &size_w, &size_h);
                pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
                nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
            
        		nfui_nfobject_gc_unref(gc);

				change_shortcut_status_by_usr_auth();
			}
			break;

		case GDK_DELETE:
			{
                nfui_nfobject_get_size(obj, &size_w, &size_h);
                nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
			
				uxm_unreg_imsg_event(obj, INFY_USRDB_CHANGE_NOTIFY);
				uxm_unreg_imsg_event(obj, INFY_FACTORY_DEFAULT_NOTIFY);
				uxm_unreg_imsg_event(obj, INFY_SYSTEM_DATA_LOAD_NOTIFY);
				uxm_unreg_imsg_event(obj, INFY_AUDIOCH_NOTIFY);				
				uxm_unreg_imsg_event(obj, INFY_MICOUT_NOTIFY);				
			}
			break;

		case INFY_USRDB_CHANGE_NOTIFY:
		case INFY_FACTORY_DEFAULT_NOTIFY:
		case INFY_SYSTEM_DATA_LOAD_NOTIFY:
			{
				change_shortcut_status_by_usr_auth();
			}
			break;

		case INFY_AUDIOCH_NOTIFY:
			{
				update_audio_on_off_btn();
			}
			break;

		case INFY_MICOUT_NOTIFY:
			{
				NF_NOTIFY_INFO *pInfo = NULL;

				if(data) {
					pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);
					g_mic_mask = pInfo->d.params[0];
				}
			}
			break;

		default:
			break;
	}

	return FALSE;
}

static gboolean post_menu_window_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFOUTEVT_BUTTON_PRESS){       
        cam_change_enable_obj(); 
		nfui_nfobject_hide(obj);
    }
	else if (evt->type == GDK_DELETE) {
		VW_destroy_playback_submenu();
		VW_destroy_camchange_submenu();
		VW_destroy_ptz_submenu();
		if (ivsc.dfunc.support_fisheye) {
			VW_destroy_fisheye_submenu();
		}
		g_curwnd = 0;
	}
	else if (evt->type == INFY_CAPTURE_IMAGE) {
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

		    smt_set_service(SMT_LIVE_SNAPSHOT);
			VW_Snapshot_Open(g_curwnd, &info, SS_MODE_BURN_RESERVE);
            smt_set_service(SMT_LIVE);			
		}
		else
			nftool_mbox(g_curwnd, "NOTICE", "SNAPSHOT FAIL", NFTOOL_MB_OK);		
	}
	else if (evt->type == INFY_AI_QUICK_REGIST_CAPTURE) {
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

		    smt_set_service(SMT_LIVE_SNAPSHOT);
			vw_quick_add_face_popup_open(g_curwnd, info.ch, info.buffer, info.size, TRUE);
            smt_set_service(SMT_LIVE);			
		}
		else
			nftool_mbox(g_curwnd, "NOTICE", "SNAPSHOT FAIL", NFTOOL_MB_OK);
	}

	return FALSE;
}


///////////////////////////////////////////////////////////////////////
//
//
//

gboolean VW_Create_ShortCut_Menu(NFWINDOW *parent)
{
	NFOBJECT *menu_fixed = NULL;
	MENU_CONF mconf; 

	gint i;
	guint pos_x, pos_y;
	guint size_w, size_h;
	guint font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(340), COLOR_IDX(341), COLOR_IDX(343), COLOR_IDX(344)};


	if (!ivsc.dfunc.support_fisheye)
		g_menu_size_h = MENU_SIZE_H - 44;
	if (!ivsc.dfunc.support_face)
		g_menu_size_h -= 40;
	if (!ivsc.dfunc.support_license_plate)
		g_menu_size_h -= 40;

	/* init menu configuration */
	if(!init_menu_conf(&mconf))
		return FALSE;


	/* window */
	g_menu_win = (NFOBJECT*)nfui_nfwindow_new(parent, 0, 0, mconf.wsize_w, g_menu_size_h);
	g_curwnd = g_menu_win;
	g_parent = parent;
	nfui_nfwindow_set_title(g_curwnd, "LIVE SHORTCUT");
	nfui_regi_post_event_callback(g_menu_win, post_menu_window_event_cb);
	nfui_nfwindow_use_outside_evt((NFWINDOW*)g_menu_win, TRUE);
	nfui_nfwindow_set_mask((NFWINDOW*)g_menu_win, GDK_BUTTON_PRESS, TRUE);
	
	gtk_widget_add_events(((NFWINDOW*)g_menu_win)->main_widget, GDK_POINTER_MOTION_HINT_MASK);


	/* fixed */
	menu_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(menu_fixed, mconf.wsize_w, g_menu_size_h);
	nfui_regi_post_event_callback(menu_fixed, post_menu_fixed_event_cb);

	uxm_reg_imsg_event(menu_fixed, INFY_USRDB_CHANGE_NOTIFY);
	uxm_monitor_on_imsg_event(menu_fixed, INFY_USRDB_CHANGE_NOTIFY);
	uxm_reg_imsg_event(menu_fixed, INFY_FACTORY_DEFAULT_NOTIFY);
	uxm_monitor_on_imsg_event(menu_fixed, INFY_FACTORY_DEFAULT_NOTIFY);	
	uxm_reg_imsg_event(menu_fixed, INFY_SYSTEM_DATA_LOAD_NOTIFY);
	uxm_monitor_on_imsg_event(menu_fixed, INFY_SYSTEM_DATA_LOAD_NOTIFY);	
	uxm_reg_imsg_event(menu_fixed, INFY_AUDIOCH_NOTIFY);
	uxm_monitor_on_imsg_event(menu_fixed, INFY_AUDIOCH_NOTIFY);	
	uxm_reg_imsg_event(menu_fixed, INFY_MICOUT_NOTIFY);
	uxm_monitor_on_imsg_event(menu_fixed, INFY_MICOUT_NOTIFY);	
	
	nfui_nfobject_show(menu_fixed);


	/* label */
	pos_x = MENU_FRONT_MARGIN;

	g_title = (NFOBJECT*)nfui_nflabel_new_with_pango_font("TITLE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
	nfui_nflabel_set_align((NFLABEL*)g_title, NFALIGN_LEFT, MENU_TEXT_FRONT_MARGIN);
	nfui_nfobject_support_multi_lang(g_title, FALSE);
	nfui_nfobject_set_size(g_title, mconf.msize_w , (guint)38);
	nfui_nfobject_modify_bg(g_title, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(g_title, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(g_title);
	nfui_nffixed_put((NFFIXED*)menu_fixed, g_title, pos_x, 4);

	/* button */
	pos_y = 49;

	for(i=0; i<MENU_CNT; i++) {
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

#ifndef _SUPPORT_SNAPSHOT
		if (i == SNAPSHOT_MENU)
			nfui_nfobject_disable(g_menu[i]);
#endif
	}

	if(!DAL_get_support_audio())
		nfui_nfobject_disable(g_menu[AUDIO_MENU]);

	if(!DAL_get_support_audio() || (nf_sysman_get_pba_type() == NF_SYSMAN_PBA_TYPE_B))
		nfui_nfobject_disable(g_menu[MIC_MENU]);

	if(!DAL_get_support_snapshot())
		nfui_nfobject_disable(g_menu[SNAPSHOT_MENU]);

	nfui_nfwindow_add((NFWINDOW*)g_menu_win, menu_fixed);
	nfui_nfobject_hide(g_menu_win);
	nfui_run_main_event_handler(g_menu_win);


	// create submenu
	VW_create_playback_submenu(g_curwnd, mconf, g_menu[PLAYBACK_MENU]);
	VW_create_camchange_submenu(g_curwnd, mconf, g_menu[CAM_CHANGE]);
	VW_create_ptz_submenu(g_curwnd, mconf, g_menu[PTZ_CTRL]);

	if (ivsc.dfunc.support_fisheye) {
		VW_create_fisheye_submenu(g_curwnd, mconf, g_menu[FISHEYE_CTRL]);
	}

	return TRUE;
}

int VW_Destroy_ShortCut_Menu()
{
	if (!g_curwnd) return 0;
	nfui_nfobject_destroy(g_curwnd);
	return 0;
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


void VW_ShortCut_Menu_Show(gint x, gint y, guint ch)
{
	gchar strBuf[STRING_SIZE_CAMTITLE];
	gchar *user_id = NULL;
	int sx;
	int sy;

	CamItxFisheyeData fisheye_data;

	// only show when user log-on
	user_id = ssm_get_cur_id(NULL);
	if(!strlen(user_id)) return;

	if(!ssm_check_access_auth(USR_AUTH_SEARCH)) nfui_nfobject_disable(g_menu[PLAYBACK_MENU]);
	else										nfui_nfobject_enable(g_menu[PLAYBACK_MENU]);

	if(!ssm_check_access_auth(USR_AUTH_AUDIO))	nfui_nfobject_disable(g_menu[AUDIO_MENU]);
	else {
		if (DAL_get_support_audio())
			nfui_nfobject_enable(g_menu[AUDIO_MENU]);
		else
			nfui_nfobject_disable(g_menu[AUDIO_MENU]);
	}

	if (DAL_get_support_snapshot())
		nfui_nfobject_enable(g_menu[SNAPSHOT_MENU]);
	else
		nfui_nfobject_disable(g_menu[SNAPSHOT_MENU]);


#if !defined(_HDI_MODEL_UX) && !defined(_DVR_MODEL_UX)
	if(!ssm_check_access_auth(USR_AUTH_MIC)) nfui_nfobject_disable(g_menu[MIC_MENU]);
	else {
		if (DAL_get_support_audio() && (nf_sysman_get_pba_type() == NF_SYSMAN_PBA_TYPE_A))
			nfui_nfobject_enable(g_menu[MIC_MENU]);
		else
			nfui_nfobject_disable(g_menu[MIC_MENU]);
	}
#endif

	if (scm_get_ipcam_onepush_supp(ch) == 0) nfui_nfobject_enable(g_menu[ONEPUSH_MENU]);
	else									 nfui_nfobject_disable(g_menu[ONEPUSH_MENU]);

	if(!ssm_check_access_auth(USR_AUTH_PTZ)) nfui_nfobject_disable(g_menu[PTZ_CTRL]);
	else									 nfui_nfobject_enable(g_menu[PTZ_CTRL]);

	if (ivsc.dfunc.support_fisheye) {
		nfui_nfobject_disable(g_menu[FISHEYE_CTRL]);

		if (nf_live_fisheye_is_support(ch))
		{
			memset(&fisheye_data, 0x00, sizeof(CamItxFisheyeData));
			DAL_get_camera_itx_fisheye_data(&fisheye_data, ch);
			if (fisheye_data.act) nfui_nfobject_enable(g_menu[FISHEYE_CTRL]);
		}
	}

    if (vsm_get_div() != VSM_DIV1)          
        nfui_nfobject_enable(g_menu[CAM_CHANGE]);
    else                                    
        nfui_nfobject_disable(g_menu[CAM_CHANGE]);

	set_menu_channel(ch);

	recal_open_position(&x, &y);
	nfui_nfobject_move(g_menu_win, (guint)x, (guint)y);

	if (x + MENU_SIZE_W + 220 + MENU_GAP > DISPLAY_ACTIVE_WIDTH) sx = x - (220 + MENU_GAP);
	else sx = (guint)x + MENU_SIZE_W + MENU_GAP;

    _get_cc_submenu_sy(&sy, y);
    
	VW_move_pb_submenu(sx, (guint)y + 44);
    VW_move_ptz_submenu(sx, (guint)y + (44 * 6) - 5);

	if (ivsc.dfunc.support_fisheye) {
		VW_move_fisheye_submenu(sx, (guint)y + (44 * 8) - 7);
	}

	if (GUI_CHANNEL_CNT > 16) 
	{
		if (x + MENU_SIZE_W + 310 + 16*20 + MENU_GAP > DISPLAY_ACTIVE_WIDTH) sx = x - (310 + MENU_GAP);
		else sx = (guint)x + MENU_SIZE_W + MENU_GAP;
	}
	else
	{
		if (x + MENU_SIZE_W + 220 + 16*20 + MENU_GAP > DISPLAY_ACTIVE_WIDTH) sx = x - (220 + MENU_GAP);
		else sx = (guint)x + MENU_SIZE_W + MENU_GAP;
	}

	VW_move_cc_submenu(sx, sy);

    memset(strBuf, 0x00, sizeof(strBuf));
    var_get_camtitle(strBuf, ch);	
	nfui_nflabel_set_text((NFLABEL*)g_title, strBuf);

#if 0		// v2.0
	if(freeze_is_on(ch)) nfui_nfbutton_set_text((NFBUTTON*)g_menu[FREEZE_MENU], "FREEZE OFF");
	else				 nfui_nfbutton_set_text((NFBUTTON*)g_menu[FREEZE_MENU], "FREEZE ON");
#endif

	if(audio_is_on(ch)) nfui_nfbutton_set_text((NFBUTTON*)g_menu[AUDIO_MENU], "AUDIO OFF");
	else				nfui_nfbutton_set_text((NFBUTTON*)g_menu[AUDIO_MENU], "AUDIO ON");
#if !defined(_HDI_MODEL_UX) && !defined(_DVR_MODEL_UX)
	if(mic_is_on(ch)) 	nfui_nfbutton_set_text((NFBUTTON*)g_menu[MIC_MENU], "MIC OFF");
	else				nfui_nfbutton_set_text((NFBUTTON*)g_menu[MIC_MENU], "MIC ON");
#endif
	nfui_nfobject_show(g_menu_win);
	nfui_make_key_hierarchy((NFWINDOW*)g_menu_win);
}

gboolean VW_ShortCut_Menu_Is_Shown()
{
	return nfui_nfobject_is_shown(g_menu_win);
}

void VW_ShortCut_Menu_Hide()
{
	if (g_menu_win) {
	    nfui_nfobject_hide(g_menu_win);
	    VW_hide_pb_submenu();
	    VW_hide_cc_submenu();
	    VW_hide_ptz_submenu();
    }
}

void VW_ShortCut_Menu_Refresh()
{
	gchar *user_id = NULL;
	
	// only show when user log-on
	user_id = ssm_get_cur_id(NULL);
	if(!strlen(user_id))
		return;

	nfui_nfobject_hide(g_menu_win);
	nfui_nfobject_show(g_menu_win);
}

int VW_ShortCut_Menu_Pos(int *x, int *y)
{
	*x = g_menu_win->x;
	*y = g_menu_win->y;
	return 0;
}

int VW_ShortCut_Menu_Size(int *w, int *h)
{
	*w = g_menu_win->width;
	*h = g_menu_win->height;
	return 0;
}

/*inline*/ guint get_menu_channel()
{
	return g_ch;
}
