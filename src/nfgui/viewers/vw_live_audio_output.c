
#include "nf_afx.h"


#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"


#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfbutton.h"
#include "viewers/objects/nfcheckbutton.h"


#include "services/scm.h"

#include "vw_live_audio_output.h"
#include "uxm.h"
#include "nf_ui_tool.h"
#include "ssm.h"


#define AUDIO_OUTPUT_SIZE_W					(383)

#ifdef GUI_32CH_SUPPORT
#define AUDIO_OUTPUT_CH						(32)
#define AUDIO_OUTPUT_SIZE_H					(322)
#elif GUI_16CH_SUPPORT
#define AUDIO_OUTPUT_CH						(16)
#define AUDIO_OUTPUT_SIZE_H					(244)
#elif defined(GUI_8CH_SUPPORT)
#define AUDIO_OUTPUT_CH						(8)
#define AUDIO_OUTPUT_SIZE_H					(244)
#else
#define AUDIO_OUTPUT_CH						(4)
#define AUDIO_OUTPUT_SIZE_H					(244)
#endif


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_aoWin = NULL;
static NFOBJECT *g_chk[AUDIO_OUTPUT_CH];

static gchar g_mstat[AUDIO_OUTPUT_CH];



static gboolean post_onoff_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_RADIO_GET_FOCUS) {
		gint index;
		gint i;

		index = nfui_radio_button_get_index((NFBUTTON*)obj);

		if(index == 0) 	scm_turnon_mic();
		else		 	scm_turnoff_mic();

		for(i=0; i<AUDIO_OUTPUT_CH; i++) {
			if(!nfui_nfobject_is_disabled(g_chk[i])) {
				if(nfui_check_button_get_active(NF_CHECKBUTTON(g_chk[i]))) {
					if(index == 0) g_mstat[i] = 0x01;
					else		   g_mstat[i] = 0x00;
				}
			}
		}

		scm_enable_mic_ch(g_mstat, AUDIO_OUTPUT_CH); 
	}

	return FALSE;
}

static gboolean post_chk_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED) {
		gboolean act;
		gint ch;

		act = nfui_check_button_get_active((NFCHECKBUTTON*)obj);
		ch = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "CHANNEL"));

		if(scm_get_mic_state() && act) g_mstat[ch] = 0x01;
		else						   g_mstat[ch] = 0x00;

		scm_enable_mic_ch(g_mstat, AUDIO_OUTPUT_CH);
	}

	return FALSE;
}

static gboolean post_select_all_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_PRESS) {
		gint i;

		for(i=0; i<AUDIO_OUTPUT_CH; i++) {
			if(!nfui_nfobject_is_disabled(g_chk[i])) {
				if(!nfui_check_button_get_active(NF_CHECKBUTTON(g_chk[i]))) {
					nfui_check_button_set_active(NF_CHECKBUTTON(g_chk[i]), TRUE);

					if(scm_get_mic_state()) g_mstat[i] = 0x01;
					else					g_mstat[i] = 0x00;
				}
			}
		}
		scm_enable_mic_ch(g_mstat, AUDIO_OUTPUT_CH); 
	}


	return FALSE;
}

static gboolean post_deselect_all_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_PRESS) {
		gint i;

		for(i=0; i<AUDIO_OUTPUT_CH; i++) {
			if(!nfui_nfobject_is_disabled(g_chk[i])) {
				if(nfui_check_button_get_active(NF_CHECKBUTTON(g_chk[i]))) {
					nfui_check_button_set_active(NF_CHECKBUTTON(g_chk[i]), FALSE);

					if(scm_get_mic_state()) 
						g_mstat[i] = 0x00;
				}
			}
		}
		if(scm_get_mic_state())
			scm_enable_mic_ch(g_mstat, AUDIO_OUTPUT_CH); 
	}

	return FALSE;
}

static gboolean post_ao_window_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case NFOUTEVT_BUTTON_PRESS:
			{
				nfui_nfobject_hide(obj);
				nfui_page_close(PGID_LIVE_AUDIO_OUTPUT, obj);
			}
			break;

		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
			{
				GdkEventKey *kevt;
				KEYPAD_KID kpid;

				kevt = (GdkEventKey*)evt;
				kpid = (KEYPAD_KID)kevt->keyval;

				if(kpid == KEYPAD_EXIT) {
					nfui_nfobject_hide(obj);
					nfui_page_close(PGID_LIVE_AUDIO_OUTPUT, obj);
				}
			}
			break;

		case GDK_DELETE:
			g_curwnd = 0;
			break;

		default:
			break;
	}

	return FALSE;
}

static void change_aout_status()
{
	guint i;

	for(i=0; i<AUDIO_OUTPUT_CH; i++)
	{
		if(scm_get_mic_out_supp(i) == 0)
			nfui_nfobject_enable(g_chk[i]);			
		else
			nfui_nfobject_disable(g_chk[i]);	

		nfui_user_signal_emit(g_chk[i], GDK_EXPOSE, FALSE);
	}
}

static gboolean post_ao_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if(evt->type == GDK_EXPOSE) 
	{
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
        
		nfui_nfobject_gc_unref(gc);

		change_aout_status();
	}
	else if(evt->type == GDK_DELETE)
	{
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);	
	
		uxm_unreg_imsg_event(obj, INFY_VLOSS_NOTIFY);
		uxm_unreg_imsg_event(obj, INFY_MICOUT_NOTIFY);
	}
	else if(evt->type == INFY_VLOSS_NOTIFY)
	{
		change_aout_status();
	}
	else if(evt->type == INFY_MICOUT_NOTIFY)
	{
		NF_NOTIFY_INFO *pInfo = NULL;
		guint mask = 0;
		gint i;

		if(nfui_nfobject_is_shown(obj))
			return FALSE;

		if(data) {
			pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);
			mask = pInfo->d.params[0];

			for(i=0; i<AUDIO_OUTPUT_CH; i++) {
				if(mask & (1 << i)) {
					nfui_check_button_set_active(NF_CHECKBUTTON(g_chk[i]), TRUE);
					g_mstat[i] = 0x01;
				}else {
					nfui_check_button_set_active(NF_CHECKBUTTON(g_chk[i]), FALSE);
					g_mstat[i] = 0x00;
				}
			}
		}
	}

	return FALSE;
}

static gboolean returnkey_proc(NFOBJECT *top, GdkEvent *event, gpointer data)
{
	return FALSE;
}

gboolean VW_Create_AudioOutput(NFWINDOW *parent, gint x, gint y)
{
	NFOBJECT *aoFixed = NULL;
	NFOBJECT *obj = NULL;
	GSList *slist = NULL;
	gint size_w, size_h;
	gchar buf[4];
	gint i;

	GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *chk_inact_img[NFCHECK_INACTIVE_STATES];
	GdkPixbuf *chk_act_img[NFCHECK_ACTIVE_STATES];

	guint inactive_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(414), COLOR_IDX(415), COLOR_IDX(416), COLOR_IDX(418)};
	guint active_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(417), COLOR_IDX(431), COLOR_IDX(432), COLOR_IDX(418)};

	// init mic state val
	memset(g_mstat, 0x00, sizeof(g_mstat));


	/* window */
	g_aoWin = (NFOBJECT*)nfui_nfwindow_new(parent, x, y, AUDIO_OUTPUT_SIZE_W, AUDIO_OUTPUT_SIZE_H);
	g_curwnd = g_aoWin;
	nfui_regi_post_event_callback(g_aoWin, post_ao_window_event_cb);
	nfui_nfwindow_use_outside_evt((NFWINDOW*)g_aoWin, TRUE);
	nfui_nfwindow_set_mask((NFWINDOW*)g_aoWin, GDK_BUTTON_PRESS, TRUE);
	nfui_nfwindow_set_returnkey_proc((NFWINDOW*)g_aoWin, returnkey_proc);
	
	gtk_widget_add_events(((NFWINDOW*)g_aoWin)->main_widget, GDK_POINTER_MOTION_HINT_MASK);

	/* fixed */
	aoFixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(aoFixed, AUDIO_OUTPUT_SIZE_W, AUDIO_OUTPUT_SIZE_H);
	nfui_regi_post_event_callback(aoFixed, post_ao_fixed_event_cb);
	nfui_nfobject_show(aoFixed);

	uxm_reg_imsg_event(aoFixed, INFY_VLOSS_NOTIFY);
	uxm_monitor_on_imsg_event(aoFixed, INFY_VLOSS_NOTIFY);

	uxm_reg_imsg_event(aoFixed, INFY_MICOUT_NOTIFY);
	uxm_monitor_on_imsg_event(aoFixed, INFY_MICOUT_NOTIFY);

	/* title */
	if(nftool_cur_language_is_japanese())
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("AUDIO OUTPUT CHANNELS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
	else
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("AUDIO OUTPUT CHANNELS", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, 330, 36);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 23);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)aoFixed, obj, 4, 4);


	/* radio button */
	radio_img[0] = nfui_get_image_from_file((IMG_N_POPUP_RADIO_OFF), NULL);
	radio_img[1] = nfui_get_image_from_file((IMG_O_POPUP_RADIO_ON), NULL);
	radio_img[2] = nfui_get_image_from_file((IMG_P_POPUP_RADIO_ON), NULL);
	radio_img[3] = nfui_get_image_from_file((IMG_D_POPUP_RADIO_OFF), NULL);

	nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

	for(i=0; i<2; i++) {
		obj = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
		nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);

		nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
		nfui_regi_post_event_callback(obj, post_onoff_event_cb);
		nfui_nfobject_show(obj);

		if(i == 0) {
			slist = nfui_radio_button_get_group(NF_BUTTON(obj));
			nfui_nffixed_put((NFFIXED*)aoFixed, obj, 64, 64);

			if(scm_get_mic_state())
				nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);
		} else 	   {
			nfui_radio_button_add_group(NF_BUTTON(obj), slist);
			nfui_nffixed_put((NFFIXED*)aoFixed, obj, 211, 64);

			if(!scm_get_mic_state())
				nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);
		}



		/* label */
		if(i == 0)
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ON", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
		else
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("OFF", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
		nfui_nfobject_set_size(obj, 100, 27);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		if(i == 0) nfui_nffixed_put((NFFIXED*)aoFixed, obj, 97, 64);
		else 	   nfui_nffixed_put((NFFIXED*)aoFixed, obj, 244, 64);
	}


	/* ch */
	chk_inact_img[0] = nfui_get_image_from_file((IMG_N_CH_SQ_INACTIVE), NULL);
	chk_inact_img[1] = nfui_get_image_from_file((IMG_O_CH_SQ_INACTIVE), NULL);
	chk_inact_img[2] = nfui_get_image_from_file((IMG_P_CH_SQ_INACTIVE), NULL);
	chk_inact_img[3] = nfui_get_image_from_file((IMG_D_CH_SQ_INACTIVE), NULL);

	chk_act_img[0] = nfui_get_image_from_file((IMG_N_CH_SQ_ACTIVE), NULL);
	chk_act_img[1] = nfui_get_image_from_file((IMG_O_CH_SQ_ACTIVE), NULL);
	chk_act_img[2] = nfui_get_image_from_file((IMG_P_CH_SQ_ACTIVE), NULL);
	chk_act_img[3] = nfui_get_image_from_file((IMG_D_CH_SQ_ACTIVE), NULL);

	for(i=0; i<AUDIO_OUTPUT_CH; i++) {
		g_sprintf(buf, "%d", i + 1);

		g_chk[i] = (NFOBJECT*)nfui_checkbutton_new(FALSE);
		nfui_check_button_set_skin_type(NF_CHECKBUTTON(g_chk[i]), NFCHECK_TYPE_DEF);
		nfui_check_button_set_inactive_image(NF_CHECKBUTTON(g_chk[i]), chk_inact_img);
		nfui_check_button_set_active_image(NF_CHECKBUTTON(g_chk[i]), chk_act_img);		
		nfui_check_set_text(NF_CHECKBUTTON(g_chk[i]), buf);
		nfui_check_set_inactive_pango_font(NF_CHECKBUTTON(g_chk[i]), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), inactive_font_color);
		nfui_check_set_active_pango_font(NF_CHECKBUTTON(g_chk[i]), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), active_font_color);
		nfui_nfobject_set_size(g_chk[i], 32, 32);
		nfui_nfobject_set_data(g_chk[i], "CHANNEL", GINT_TO_POINTER(i));
		nfui_regi_post_event_callback(g_chk[i], post_chk_event_cb);
		nfui_nfobject_show(g_chk[i]);

		nfui_nffixed_put((NFFIXED*)aoFixed, g_chk[i], 53 + (34 * (i % 8)), 111 + (34 * (i / 8)));
	}

	/* select/deselect */
	obj = nftool_normal_button_create_popup_type2("SELECT ALL", 150);
	nfui_nfbutton_set_spacing((NFBUTTON*)obj, CONDENSED_SPACING);
	nfui_regi_post_event_callback(obj, post_select_all_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)aoFixed, obj, 37, AUDIO_OUTPUT_SIZE_H - 62);

	obj = nftool_normal_button_create_popup_type2("DESELECT ALL", 150);
	nfui_nfbutton_set_spacing((NFBUTTON*)obj, ULTRA_CONDENSED_SPACING);
	nfui_regi_post_event_callback(obj, post_deselect_all_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)aoFixed, obj, 189, AUDIO_OUTPUT_SIZE_H - 62);
	


	nfui_nfwindow_add((NFWINDOW*)g_aoWin, aoFixed);
	nfui_run_main_event_handler(g_aoWin);
	nfui_nfobject_show(g_aoWin);
	nfui_make_key_hierarchy((NFWINDOW*)g_aoWin);

	nfui_nfobject_hide(g_aoWin);


	return TRUE;
}

int VW_Destroy_AudioOutput()
{
	if (!g_curwnd) return 0;
	nfui_nfobject_destroy(g_curwnd);
	return 0;
}

void VW_AudioOutput_Show(gint x, gint y)
{
    if (GUI_CHANNEL_CNT  > 16)
        nfui_nfobject_move(g_aoWin, x, y - 78);
    else
        nfui_nfobject_move(g_aoWin, x, y);
        
	nfui_nfobject_show(g_aoWin);
	nfui_page_open(PGID_LIVE_AUDIO_OUTPUT, g_aoWin, ssm_get_cur_id(NULL));
}

void VW_AudioOutput_Hide()
{
	nfui_nfobject_hide(g_aoWin);

	nfui_page_close(PGID_LIVE_AUDIO_OUTPUT, g_aoWin);
}

gboolean VW_AudioOutput_IsShown()
{
	return nfui_nfobject_is_shown(g_aoWin);
}


