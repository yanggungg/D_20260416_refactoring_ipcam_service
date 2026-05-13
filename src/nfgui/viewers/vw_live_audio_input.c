
#include "nf_afx.h"


#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"

#include "tools/nf_ui_tool.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "objects/nfcheckbutton.h"
#include "viewers/objects/nfcombobox.h"

#include "services/scm.h"
#include "modules/var.h"

#include "vw_live_audio_input.h"
#include "uxm.h"
#include "ssm.h"

#define AUDIO_INPUT_SIZE_W					(414)
#define AUDIO_INPUT_SIZE_H					(160)

#define AUDIO_OFF							(0xff)

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_aiWin = NULL;
static NFOBJECT *g_aiCombo = NULL;
static NFOBJECT *g_aiChk = NULL;

static void update_audio_input_combo(void)
{
	int cur_ch = scm_get_cur_live_audio_ch();

	if (cur_ch == 0xff)
		nfui_combobox_set_index_no_expose(g_aiCombo, 0);
	else
		nfui_combobox_set_index_no_expose(g_aiCombo, cur_ch+1);

	nfui_signal_emit(g_aiCombo, GDK_EXPOSE, TRUE);	
}

static gboolean post_check_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gboolean state;
	
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
		state = nfui_check_button_get_active((NFCHECKBUTTON*)g_aiChk);
        var_set_full_scr_audio(state);
    }

    return FALSE;
}

static gboolean post_combo_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {
		gint ch_idx;

		ch_idx = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
#if !defined(_HDI_MODEL_UX) && !defined(_DVR_MODEL_UX)
		if(ch_idx > 0){			
			BITMASK connect = 0;
			connect = scm_get_cam_conn_state();

			if(!(connect & (1 << (ch_idx-1)))){					
				nftool_mbox(g_curwnd, "ERROR", "NO VIDEO", NFTOOL_MB_OK);				
			}
			else{
				if(scm_get_audio_in_supp(ch_idx-1) == -1)
					nftool_mbox(g_curwnd, "ERROR", "Current channel does not support audio.", NFTOOL_MB_OK);
			}
		}
#endif		
		if (ch_idx)
			scm_change_live_audio((ch_idx - 1));
		else
			scm_change_live_audio(AUDIO_OFF);
	}

	return FALSE;
}

static gboolean post_ai_window_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case NFOUTEVT_BUTTON_PRESS:
			{
				nfui_nfobject_hide(obj);
				nfui_page_close(PGID_LIVE_AUDIO_INPUT, obj);
			}
			break;

		case INFY_CAMDB_CHANGE_NOTIFY:
		case INFY_USRDB_CHANGE_NOTIFY:		
			{
				gchar camTitle[STRING_SIZE_CAMTITLE];
				gint i;

				nfui_combobox_remove_all((NFCOMBOBOX*)g_aiCombo);
				nfui_combobox_append_data((NFCOMBOBOX*)g_aiCombo, "OFF");

				for(i=1; i<=var_get_ch_count(); i++) {
					//DAL_get_camera_title(camTitle , (guint)i-1);
					var_get_camtitle(camTitle , (guint)i-1);

					nfui_combobox_append_data((NFCOMBOBOX*)g_aiCombo, camTitle);
				}

				update_audio_input_combo();
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
					nfui_page_close(PGID_LIVE_AUDIO_INPUT, obj);
				}
			}
			break;

		case GDK_DELETE:
			g_curwnd = 0;
			uxm_unreg_imsg_event(obj, INFY_CAMDB_CHANGE_NOTIFY);
			uxm_unreg_imsg_event(obj, INFY_USRDB_CHANGE_NOTIFY);			
			break;

		default:
			break;
	}

	return FALSE;
}

static gboolean post_ai_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	switch(evt->type) {
		case GDK_EXPOSE:
			{
        		drawable = nfui_nfobject_get_window(obj);
        		gc = nfui_nfobject_get_gc(obj);

                nfui_nfobject_get_size(obj, &size_w, &size_h);
                pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
                nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
            
        		nfui_nfobject_gc_unref(gc);
        	}
			break;

		case GDK_DELETE:
			{
                nfui_nfobject_get_size(obj, &size_w, &size_h);
                nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);	
			
				uxm_unreg_imsg_event(obj, INFY_AUDIOCH_NOTIFY);
			}
			break;

		case INFY_AUDIOCH_NOTIFY:			
			{			
				update_audio_input_combo();
			}
			break;

		default:
			break;
	}

	return FALSE;
}

static gboolean returnkey_proc(NFOBJECT *top, GdkEvent *event, gpointer data)
{
	return FALSE;
}

gboolean VW_Create_AudioInput(NFWINDOW *parent, gint x, gint y)
{
	NFOBJECT *aiFixed = NULL;
	NFOBJECT *obj = NULL;

	gint size_w, size_h;

	gint i;
	gint cam_ch = var_get_ch_count();
	gchar camTitle[STRING_SIZE_CAMTITLE];


	/* window */
	g_aiWin = (NFOBJECT*)nfui_nfwindow_new(parent, x, y, AUDIO_INPUT_SIZE_W, AUDIO_INPUT_SIZE_H);
	g_curwnd = g_aiWin;
	nfui_regi_post_event_callback(g_aiWin, post_ai_window_event_cb);
	nfui_nfwindow_use_outside_evt((NFWINDOW*)g_aiWin, TRUE);
	nfui_nfwindow_set_mask((NFWINDOW*)g_aiWin, GDK_BUTTON_PRESS, TRUE);
	nfui_nfwindow_set_returnkey_proc((NFWINDOW*)g_aiWin, returnkey_proc);
	
	gtk_widget_add_events(((NFWINDOW*)g_aiWin)->main_widget, GDK_POINTER_MOTION_HINT_MASK);

	uxm_reg_imsg_event(g_aiWin, INFY_CAMDB_CHANGE_NOTIFY);
	uxm_monitor_on_imsg_event(g_aiWin, INFY_CAMDB_CHANGE_NOTIFY);
	uxm_reg_imsg_event(g_aiWin, INFY_USRDB_CHANGE_NOTIFY);
	uxm_monitor_on_imsg_event(g_aiWin, INFY_USRDB_CHANGE_NOTIFY);

	/* fixed */
	aiFixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(aiFixed, AUDIO_INPUT_SIZE_W, AUDIO_INPUT_SIZE_H);
	nfui_regi_post_event_callback(aiFixed, post_ai_fixed_event_cb);
	nfui_nfobject_show(aiFixed);

	uxm_reg_imsg_event(aiFixed, INFY_AUDIOCH_NOTIFY);
	uxm_monitor_on_imsg_event(aiFixed, INFY_AUDIOCH_NOTIFY);

	/* title */
	if(nftool_cur_language_is_japanese())
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("AUDIO INPUT CHANNEL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
	else
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("AUDIO INPUT CHANNEL", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, 330, 36);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 23);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)aiFixed, obj, 4, 4);


	/* */
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CHANNEL", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(obj, 102, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)aiFixed, obj, 23, 60);


	/* combo */
	obj = nfui_combobox_new(NULL, 0, 0);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
	nfui_nfobject_set_size(obj, 245, 40);
	nfui_regi_post_event_callback(obj, post_combo_event_cb);
	nfui_nfobject_show(obj);

	nfui_nffixed_put((NFFIXED*)aiFixed, obj, 139, 60);
	g_aiCombo = obj;
	
	// set cam title
	nfui_combobox_append_data((NFCOMBOBOX*)obj, "OFF");
	for(i=1; i<=cam_ch; i++) {
		//DAL_get_camera_title(camTitle , (guint)i-1);
		var_get_camtitle(camTitle , (guint)i-1);
		
		nfui_combobox_append_data((NFCOMBOBOX*)obj, camTitle);
	}

	if (scm_get_cur_live_audio_ch() != 0xff)
		nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, (guint)scm_get_cur_live_audio_ch()+1);
	
	obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
	nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
	nfui_regi_post_event_callback(obj, post_check_event_cb);

	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)aiFixed, obj, 100, 112);
	g_aiChk = obj;

	if(nftool_cur_language_is_japanese())
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("LINK TO FULL SCREEN", nffont_get_pango_font(NFFONT_SMALL_SEMI_1), COLOR_IDX(294));
	else
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("LINK TO FULL SCREEN", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(294));	
	nfui_nfobject_set_size(obj, 250, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nflabel_use_strip((NFLABEL*)obj, TRUE);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_use_tooltip(obj, TRUE);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)aiFixed, obj, 139, 107);


	nfui_nfwindow_add((NFWINDOW*)g_aiWin, aiFixed);
	nfui_run_main_event_handler(g_aiWin);
	nfui_nfobject_show(g_aiWin);
	nfui_make_key_hierarchy((NFWINDOW*)g_aiWin);

	nfui_nfobject_hide(g_aiWin);
	
	return TRUE;
}


int VW_Destroy_AudioInput()
{
	if (!g_curwnd) return 0;
	nfui_nfobject_destroy(g_curwnd);
	return 0;
}

void VW_AudioInput_Show()
{
	//TODO
	//nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_aiCombo, (guint)scm_get_cur_live_audio_ch());

	nfui_nfobject_show(g_aiWin);

	nfui_page_open(PGID_LIVE_AUDIO_INPUT, g_aiWin, ssm_get_cur_id(NULL));
}

void VW_AudioInput_Hide()
{
	nfui_nfobject_hide(g_aiWin);

	nfui_page_close(PGID_LIVE_AUDIO_INPUT, g_aiWin);
}

gboolean VW_AudioInput_IsShown()
{
	return nfui_nfobject_is_shown(g_aiWin);
}

/*
void VW_AudioInput_Change_CH(guint ch)
{
	if(nfui_check_button_get_active((NFCHECKBUTTON*)g_aiChk))
	{
		nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_aiCombo, ch + 1);

//		if(scm_get_cur_live_audio_ch() != (ch))
//			scm_change_live_audio((ch));
}
}*/

