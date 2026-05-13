#include "../tools/nf_ui_tool.h"

#include "../support/nf_ui_image.h"
#include "../support/event_loop.h"

#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfobject.h"
#include "objects/nfbutton.h"
#include "objects/nfimage.h"
#include "objects/nflabel.h"
#include "objects/nfspinbutton.h"
#include "objects/nfcombobox.h"

#include "vw_sys_net_main.h"

//===> POSITION
#define RTP_CATE_TITLE_X						(8)
#define RTP_CATE_TITLE_Y						(0)

#define RTP_ITEM_TITLE_X                        (RTP_CATE_TITLE_X + 20)

//===> SIZE
#define RTP_ITEM_TITLE_H                        (40)
#define RTP_ITEM_TITLE_W                        (300)
#define RTP_ITEM_CELL_W                         (400)
#define RTP_ITEM_CELL_H                         (RTP_ITEM_TITLE_H)

#define SHOW_RTPDATA

static RtpData rtpdata;
static RtpData org_rtpdata;

static NFWINDOW *g_curwnd;

static NFOBJECT *g_rtpsport;
static NFOBJECT *g_rtpeport;
static NFOBJECT *g_audio_back_mode;
static NFOBJECT *g_audio_back_port;
static NFOBJECT *g_audio_back_port2;



static gint _show_rtpData(RtpData *data)
{
#ifdef SHOW_RTPDATA
    g_message("%s, %d", __FUNCTION__, __LINE__);
    g_message("rtpsport : %d", data->rtpsport);
    g_message("rtpeport : %d", data->rtpeport);
    g_message("audio_backch_mode : %d", data->audio_backch_mode);
    g_message("audio_backch_port : %d", data->audio_backch_port);
#endif

    return 0;
}

static gint _init_rtpData()
{
    memset(&rtpdata, 0x00, sizeof(RtpData));
    memset(&org_rtpdata, 0x00, sizeof(RtpData));

    if (DAL_get_rtp_data(&rtpdata))
    {
        g_message("%s, %d, RTP DATA LOAD FAIL!!", __FUNCTION__, __LINE__);
        memset(&rtpdata, 0x00, sizeof(RtpData));

        return -1;
    }

    _show_rtpData(&rtpdata);

    g_memmove(&org_rtpdata, &rtpdata, sizeof(RtpData));

    return 0;
}

static gint _set_data_to_obj(RtpData *data, gint expose)
{
    _show_rtpData(data);

    nfui_nflabel_set_number((NFLABEL*)g_rtpsport, data->rtpsport);
    nfui_nflabel_set_number((NFLABEL*)g_rtpeport, data->rtpeport);
    nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)g_audio_back_mode, data->audio_backch_mode);
    nfui_nflabel_set_number((NFLABEL*)g_audio_back_port, data->audio_backch_port);
    nfui_nflabel_set_number((NFLABEL*)g_audio_back_port2, data->audio_backch_port+1);

    if (data->audio_backch_mode)
    {
        nfui_nfobject_disable(g_audio_back_port);
        nfui_nfobject_disable(g_audio_back_port2);
    }
    else
    {
        nfui_nfobject_enable(g_audio_back_port);
        nfui_nfobject_enable(g_audio_back_port2);
    }

    if (expose)
    {
        nfui_signal_emit(g_rtpsport, GDK_EXPOSE, FALSE);
        nfui_signal_emit(g_rtpeport, GDK_EXPOSE, FALSE);
        nfui_signal_emit(g_audio_back_mode, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_audio_back_port, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_audio_back_port2, GDK_EXPOSE, TRUE);
    }
}

static gint _get_data_from_obj(RtpData *data)
{
    data->rtpsport = nfui_nflabel_get_number((NFLABEL*)g_rtpsport);
    data->rtpeport = nfui_nflabel_get_number((NFLABEL*)g_rtpeport);
    data->audio_backch_mode = nfui_spin_button_get_index((NFSPINBUTTON*)g_audio_back_mode);
    data->audio_backch_port = nfui_nflabel_get_number((NFLABEL*)g_audio_back_port);

    _show_rtpData(data);

    return 0;
}

static gboolean _check_port_validity(NFOBJECT *obj, int port_num)
{
    IPSetupData ipdata;
	gint pnum_temp;
	gint sport, eport;
	gint i, j;

    memset(&ipdata, 0x00, sizeof(IPSetupData));

	DAL_get_ipSetup_data(&ipdata);

	sport = nfui_nflabel_get_number((NFLABEL*)g_rtpsport);
	eport = nfui_nflabel_get_number((NFLABEL*)g_rtpeport);

    if (obj == g_rtpsport)
    {
    	if (port_num >= eport)
    	{
    	    nftool_mbox(g_curwnd, "NOTICE", "Initial port number must be lower than the final port number.", NFTOOL_MB_OK);
    	    return FALSE;
    	}

		if (port_num <= 1024) {
			nftool_mbox(g_curwnd, "NOTICE", "Port number must be larger than 1024.", NFTOOL_MB_OK);
			return FALSE;
		}
	}
	else if (obj == g_rtpeport)
	{
    	if (sport >= port_num)
    	{
    	    nftool_mbox(g_curwnd, "NOTICE", "Final port number must be higher than the initial port number.", NFTOOL_MB_OK);
    	    return FALSE;
    	}

    	if (port_num >= 65536) {
    		nftool_mbox(g_curwnd, "NOTICE", "The port number must be less than 65536.", NFTOOL_MB_OK);
    		return FALSE;
    	}
	}
	else if (obj == g_audio_back_port)
	{

    	if (port_num == ipdata.rtspport)
    	{
    		nftool_mbox(g_curwnd, "NOTICE", "It's a port number used already.", NFTOOL_MB_OK);
    	    return FALSE;
    	}

    	if (port_num == ipdata.webPort)
    	{
    		nftool_mbox(g_curwnd, "NOTICE", "It's a port number used already.", NFTOOL_MB_OK);
    	    return FALSE;
    	}

		if (port_num <= 1024) {
			nftool_mbox(g_curwnd, "NOTICE", "Port number must be larger than 1024.", NFTOOL_MB_OK);
			return FALSE;
		}

    	if (port_num >= 65535) {
    		nftool_mbox(g_curwnd, "NOTICE", "The port number must be less than 65535.", NFTOOL_MB_OK);
    		return FALSE;
    	}
	}

	return TRUE;
}

static gboolean post_audioback_mode_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_SPINBUTTON_CHANGED)
	{
	    if (nfui_spin_button_get_index((NFSPINBUTTON*)obj) == 1)
	    {
	        nfui_nfobject_disable(g_audio_back_port);
	        nfui_nfobject_disable(g_audio_back_port2);
	    }
	    else
	    {
	        nfui_nfobject_enable(g_audio_back_port);
	        nfui_nfobject_enable(g_audio_back_port2);
	    }
	    nfui_signal_emit(g_audio_back_port, GDK_EXPOSE, FALSE);
	    nfui_signal_emit(g_audio_back_port2, GDK_EXPOSE, FALSE);
    }

	return FALSE;
}

static gboolean post_port_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{
		NFOBJECT *top = NULL;
		guint x, y;
		gint numTemp;
		gint numRet;

		if(kpid == KEYPAD_ENTER)
		{
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);

			x += (obj->width)/2 + top->x;
			y += obj->height + top->y;
		}
		else
		{
			if(evt->button.button == MOUSE_RIGTH_BUTTON) {
				return FALSE;
			}

			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += ((GdkEventButton*)evt)->x;
			y += ((GdkEventButton*)evt)->y;
		}

		numTemp = nfui_nflabel_get_number((NFLABEL*)obj);
		numRet = NumberKey_Open(g_curwnd, numTemp, x, y, 65535);

		if (numRet == -1) return FALSE;

		if (_check_port_validity(obj, numRet))
		{
			nfui_nflabel_set_number((NFLABEL*)obj, numRet);
			nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
			if (obj == g_audio_back_port)
			{
			    nfui_nflabel_set_number((NFLABEL*)g_audio_back_port2, numRet+1);
			    nfui_signal_emit(g_audio_back_port2, GDK_EXPOSE, TRUE);
			}
		}
	}

	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {
			return FALSE;
		}

		g_memmove(&rtpdata, &org_rtpdata, sizeof(RtpData));

		_set_data_to_obj(&rtpdata, 1);
    }

	return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint i;

	switch (evt->type)
	{
    	case GDK_BUTTON_RELEASE:
    	{
    		if(evt->button.button == MOUSE_RIGTH_BUTTON) {
    			return FALSE;
    		}

    	    _get_data_from_obj(&rtpdata);

    		if(memcmp(&org_rtpdata, &rtpdata, sizeof(RtpData)))
    		{
    			gint is_changed = 0;

    			//is_changed = prvIsIPChanged();
    			DAL_set_rtp_data(&rtpdata);

    			if (is_changed) {
    				//wait_pop = nftool_mbox_wait(g_curwnd, "WAIT", "Network server is restarting.\nPlease wait...");
    				//DAL_save_setup_db(NFSETUP_WINDOW_NETWORK);
    				//scm_apply_netinfo_by_db(IRET_SCM_APPLY_NETINFO);
    			}
    			else {
    				nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");

                    //scm_put_log(CHANGE_NET_IP, 0, 0);
    				sysnet_set_changeflag(1);
    			}

    			g_memmove(&org_rtpdata, &rtpdata, sizeof(RtpData));
    		}
    	}
    	break;
    }

	return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {
			return FALSE;
		}

		Rtp_tab_out_handler();

		SystemSetupNetwork_Destroy(obj);
	}

	return FALSE;
}

void init_NetRtp_Rtp_Page(NFOBJECT *parent)
{
    NFOBJECT *content_fixed;
    NFOBJECT *obj;

    guint pos_x, pos_y;
    gint fg_color[] = {COLOR_IDX(128), COLOR_IDX(131), COLOR_IDX(133), COLOR_IDX(135)};

    gchar *mode[] = {"MANUAL", "AUTO"};


    _init_rtpData();

	g_curwnd = nfui_nfobject_get_top(parent);

	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);

////////////////////////////////////////////////////
//	RTP
//
    pos_x = RTP_CATE_TITLE_X;
    pos_y = RTP_CATE_TITLE_Y;

	obj = nfui_nfimage_new(IMG_TITLE_BG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "UDP");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

//===> PORT RANGE
    pos_x = RTP_ITEM_TITLE_X;
    pos_y += RTP_ITEM_TITLE_H + 21;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PORT RANGE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nfobject_set_size(obj, RTP_ITEM_TITLE_W, RTP_ITEM_TITLE_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	pos_x += RTP_ITEM_TITLE_W;

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nflabel_use_number((NFLABEL*)obj, TRUE);
	nfui_nfobject_set_size(obj, (RTP_ITEM_CELL_W/2) - 15, RTP_ITEM_CELL_H);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_port_event_handler);
	g_rtpsport = obj;

    pos_x += ((RTP_ITEM_CELL_W/2) - 15) + 5;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("~", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nfobject_set_size(obj, 20, RTP_ITEM_TITLE_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	pos_x += 20 + 5;

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nflabel_use_number((NFLABEL*)obj, TRUE);
	nfui_nfobject_set_size(obj, (RTP_ITEM_CELL_W/2) - 15, RTP_ITEM_CELL_H);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_port_event_handler);
	g_rtpeport = obj;

////////////////////////////////////////////////////
//	AUDIO BACK CHANNEL
//
    pos_x = RTP_CATE_TITLE_X;
    pos_y += RTP_ITEM_CELL_H + 60;

	obj = nfui_nfimage_new(IMG_TITLE_BG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "AUDIO BACK CHANNEL");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

//===> OPERATION MODE
    pos_x = RTP_ITEM_TITLE_X;
    pos_y += RTP_ITEM_TITLE_H + 21;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("MODE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nfobject_set_size(obj, RTP_ITEM_TITLE_W, RTP_ITEM_TITLE_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

    pos_x += RTP_ITEM_TITLE_W;

	obj = nfui_spinbutton_new(mode, 2, 0);
	nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_1);
	nfui_nflabel_set_align((NFSPINBUTTON*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, RTP_ITEM_CELL_W, RTP_ITEM_CELL_H);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_audioback_mode_event_handler);
	g_audio_back_mode = obj;

//===> PORT
    pos_x = RTP_ITEM_TITLE_X;
    pos_y += RTP_ITEM_TITLE_H + 2;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PORT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nfobject_set_size(obj, RTP_ITEM_TITLE_W, RTP_ITEM_TITLE_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	pos_x += RTP_ITEM_TITLE_W;

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nflabel_use_number((NFLABEL*)obj, TRUE);
	nfui_nfobject_set_size(obj, (RTP_ITEM_CELL_W/2) - 15, RTP_ITEM_CELL_H);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_port_event_handler);
	g_audio_back_port = obj;

    pos_x += ((RTP_ITEM_CELL_W/2) - 15) + 5;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("~", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nfobject_set_size(obj, 20, RTP_ITEM_TITLE_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	pos_x += 20 + 5;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(128));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nflabel_use_number((NFLABEL*)obj, TRUE);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(138));
	nfui_nflabel_set_fg_color((NFLABEL*)obj, fg_color);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_DISABLE, COLOR_IDX(134));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, (RTP_ITEM_CELL_W/2) - 15, RTP_ITEM_CELL_H);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
	g_audio_back_port2 = obj;

//===> MENU BUTTON
	obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R3_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);

	obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R2_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_applybutton_event_handler);

	obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R1_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

	_set_data_to_obj(&rtpdata, 0);

    return;
}

gboolean Rtp_tab_out_handler()
{
    mb_type ret;

    _get_data_from_obj(&rtpdata);

    if(!memcmp(&org_rtpdata, &rtpdata, sizeof(RtpData))) return FALSE;

	ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

	if(ret == NFTOOL_MB_OK)
	{
	    DAL_set_rtp_data(&rtpdata);

	    g_memmove(&org_rtpdata, &rtpdata, sizeof(RtpData));
	}
	else
	{
    	g_memmove(&rtpdata, &org_rtpdata, sizeof(RtpData));

    	_set_data_to_obj(&rtpdata, 0);
	}

	return FALSE;
}
