
#include "nf_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "objects/nffixed.h"
#include "objects/nflabel.h"
#include "objects/nfbutton.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfspinbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nfimage.h"

#include "vw_event_noti_evt.h"
#include "vw_event_email_noti.h"
#include "vw_event_email_edit.h"

#include "vw_evt_act_main.h"
#include "log.h"
#include "scm.h"


enum {
	FREQ_IMMEDIATELY = 0,
	FREQ_1_MIN,
	FREQ_5_MIN,
	FREQ_10_MIN,
	FREQ_15_MIN,
	FREQ_30_MIN,
	FREQ_60_MIN,

	NUM_FREQS
};

static EA_EvtNotiEmailData g_emd;
static EA_EvtNotiEmailData g_oemd;

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_freqObj;
static NFOBJECT *g_jpegChk = 0;
static NFOBJECT *g_vlinkChk = 0;
static NFOBJECT *g_all[EMAIL_SCHEDULING_MAX_CNT];
static NFOBJECT *g_day[EMAIL_SCHEDULING_MAX_CNT][7];
static NFOBJECT *g_from_h[EMAIL_SCHEDULING_MAX_CNT];
static NFOBJECT *g_from_m[EMAIL_SCHEDULING_MAX_CNT];
static NFOBJECT *g_to_h[EMAIL_SCHEDULING_MAX_CNT];
static NFOBJECT *g_to_m[EMAIL_SCHEDULING_MAX_CNT];
static NFOBJECT *g_server_obj[2];

static gint g_pre_from_h[EMAIL_SCHEDULING_MAX_CNT];
static gint g_pre_from_m[EMAIL_SCHEDULING_MAX_CNT];
static gint g_pre_to_h[EMAIL_SCHEDULING_MAX_CNT];
static gint g_pre_to_m[EMAIL_SCHEDULING_MAX_CNT];

static NFOBJECT *g_al_switch;
static NFOBJECT *g_al_switch_port;
static gint supp_al_switch = 0;
static gint g_pre_al_switch = 0;



static void init_em_data()
{
	memset(&g_emd, 0x00, sizeof(EA_EvtNotiEmailData));
	memset(&g_oemd, 0x00, sizeof(EA_EvtNotiEmailData));

	DAL_get_evtNoti_email_data(&g_emd);
	g_memmove(&g_oemd, &g_emd, sizeof(EA_EvtNotiEmailData));
}

static void set_em_data()
{
	g_memmove(&g_oemd, &g_emd, sizeof(EA_EvtNotiEmailData));

	DAL_set_evtNoti_email_data(g_emd);

	scm_put_log(CHANGE_EVT_NOTI, 0, 0);
}

static guint prvIndexToFreq(gint index)
{
	guint ret = 0;

	switch(index)
	{
		case FREQ_IMMEDIATELY:		ret = 0;	break;
		case FREQ_1_MIN:		ret = 1;	break;
		case FREQ_5_MIN:		ret = 5;	break;
		case FREQ_10_MIN:		ret = 10;	break;
		case FREQ_15_MIN:		ret = 15;	break;
		case FREQ_30_MIN:		ret = 30;	break;
		case FREQ_60_MIN:		ret = 60;	break;
		default:				ret = 0;	break;
	}

	return ret;
}

static gint prvFreqToIndex(guint freq)
{
	gint ret = 0;

	if(freq == 0)		ret = FREQ_IMMEDIATELY;
	else if(freq < 5)		ret = FREQ_1_MIN;
	else if(freq < 10)	ret = FREQ_5_MIN;
	else if(freq < 15)	ret = FREQ_10_MIN;
	else if(freq < 30)	ret = FREQ_15_MIN;
	else if(freq < 60)	ret = FREQ_30_MIN;
	else				ret = FREQ_60_MIN;

	return ret;
}

static gint _set_pre_from_to_time(gint idx, gint from_h, gint from_m, gint to_h, gint to_m)
{
    g_pre_from_h[idx] = from_h;
    g_pre_from_m[idx] = from_m;
    g_pre_to_h[idx] = to_h;
    g_pre_to_m[idx] = to_m;

    return 0;
}

static gint _get_cmp_from_to_time(gint idx)
{
    gint from_h, from_m, to_h, to_m;
    gint from, to;

    from_h = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_from_h[idx]);
    from_m = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_from_m[idx]);
    to_h = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_to_h[idx]);
    to_m = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_to_m[idx]);

    from = (from_h * 60) + from_m;
    to = (to_h * 60) + to_m;

    if (from == to) return 0;
    else if (from > to) return -1;
    else if (from < to) return 1;

    return 0;
}

static gint _set_active_timeobj(gint idx, gint expose)
{
    gint i;
    gint act = 0;

    for (i = 0; i < 7; i++)
    {
        act = nfui_check_button_get_active((NFCHECKBUTTON*)g_day[idx][i]);
        if (act) break;
    }
    if (i == 7)
    {
        nfui_nfobject_disable(g_from_h[idx]);
        nfui_nfobject_disable(g_from_m[idx]);
        nfui_nfobject_disable(g_to_h[idx]);
        nfui_nfobject_disable(g_to_m[idx]);
    }
    else
    {
        nfui_nfobject_enable(g_from_h[idx]);
        nfui_nfobject_enable(g_from_m[idx]);
        nfui_nfobject_enable(g_to_h[idx]);
        nfui_nfobject_enable(g_to_m[idx]);
    }

    if (expose)
    {
        nfui_signal_emit(g_from_h[idx], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_from_m[idx], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_to_h[idx], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_to_m[idx], GDK_EXPOSE, TRUE);
    }

    return 0;
}

static gint _trans_data_to_val(gint data, gchar *val)
{
    gint buf;

    memset(val, 0x00, sizeof(16));

    if (data == 16)
    {
#ifdef ENABLE_ARI_PANIC
        strcpy(val, "ARI");
#else
        sprintf(val, "AI 1");
#endif
    }
    else
    {
        sprintf(val, "AI %d", (data + 1));
    }

    return 0;
}

static gint _trans_val_to_data(gchar *val)
{
    gint data;
    gint i;
    gchar buf[16];

    if (strcmp(val, "ARI") == 0) {
        data = 16;
    }
    else {
        for (i = 0; i < 16; i++)
        {
            sprintf(buf, "AI %d", (i + 1));
            data = i;
            if (strcmp(val, buf) == 0) break;
        }
    }

    return data;
}

static gint _set_all_chk_btn(gint sched_num, gboolean expose)
{
	gboolean state;
	gint i;

	for (i = 0; i < 7; i++)
	{
	    state = nfui_check_button_get_active((NFCHECKBUTTON*)g_day[sched_num][i]);

		if (!state)
		{
            nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_all[sched_num], FALSE);

		    if (expose)
                nfui_signal_emit(g_all[sched_num], GDK_EXPOSE, TRUE);

            return 0;
        }
	}

	if (i == 7)
	{
        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_all[sched_num], TRUE);

	    if (expose)
            nfui_signal_emit(g_all[sched_num], GDK_EXPOSE, TRUE);
    }

    return 0;
}

static void set_data_to_obj(gint expose)
{
	guint index;
	gint i, j;
	gchar buf[16];

	index = prvFreqToIndex(g_emd.frequency);
	nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_freqObj, index);

	if(!DAL_get_support_snapshot())
		nfui_nfobject_disable(g_jpegChk);

	nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_jpegChk, g_emd.include_jpeg);
    if (supp_al_switch) {
        nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_al_switch, g_emd.al_switch);
        _trans_data_to_val(g_emd.al_switch_port, buf);
        nfui_combobox_set_data_no_expose((NFCOMBOBOX *) g_al_switch_port, buf);
    }

    if (expose) {
        nfui_signal_emit(g_freqObj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_jpegChk, GDK_EXPOSE, TRUE);
		if (supp_al_switch) {
    		nfui_signal_emit(g_al_switch, GDK_EXPOSE, TRUE);
    		nfui_signal_emit(g_al_switch_port, GDK_EXPOSE, TRUE);
		}
    }

#if defined(_SUPPORT_DUAL_SMTPSERVER)
	nfui_radio_button_set_toggled(NF_BUTTON(g_server_obj[g_emd.serv]), TRUE);
	if (expose) {
		nfui_signal_emit(g_server_obj[0], GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_server_obj[1], GDK_EXPOSE, TRUE);
	}
#endif

    for (i = 0; i < EMAIL_SCHEDULING_MAX_CNT; i++)
    {
        for(j = 0; j < 7; j++)
        {
            nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_day[i][j], (g_emd.sched[i].wday & (1 << j)));
            if (expose) nfui_signal_emit(g_day[i][j], GDK_EXPOSE, TRUE);
        }
        nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_from_h[i], g_emd.sched[i].from_h);
        nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_from_m[i], g_emd.sched[i].from_m);
        nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_to_h[i], g_emd.sched[i].to_h);
        nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_to_m[i], g_emd.sched[i].to_m);

        if (expose)
        {
            nfui_signal_emit(g_from_h[i], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_from_m[i], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_to_h[i], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_to_m[i], GDK_EXPOSE, TRUE);
        }

        _set_active_timeobj(i, expose);
        _set_all_chk_btn(i, expose);
    }
}

static void get_data_from_obj()
{
	guint index;
	gint i, j;

	index = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_freqObj);
	g_emd.frequency = prvIndexToFreq((gint)index);
	g_emd.include_jpeg = nfui_check_button_get_active((NFCHECKBUTTON*)g_jpegChk);
	if (supp_al_switch) {
    	g_emd.al_switch = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_al_switch);
    	g_emd.al_switch_port = _trans_val_to_data(nfui_combobox_get_value((NFCOMBOBOX*)g_al_switch_port));
	}

#if defined(_SUPPORT_DUAL_SMTPSERVER)
	for (i = 0; i < 2; i++)
	{
		if (nfui_radio_button_get_toggled((NFBUTTON*)g_server_obj[i])) g_emd.serv = i;
	}
#endif

    for (i = 0; i < EMAIL_SCHEDULING_MAX_CNT; i++)
    {
        for (j = 0; j < 7; j++)
        {
            if (nfui_check_button_get_active((NFCHECKBUTTON*)g_day[i][j])) {
                g_emd.sched[i].wday |= (1 << j);
            }
            else {
                g_emd.sched[i].wday &= ~(1 << j);
            }
        }
        g_emd.sched[i].from_h = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_from_h[i]);
        g_emd.sched[i].from_m = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_from_m[i]);
        g_emd.sched[i].to_h = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_to_h[i]);
        g_emd.sched[i].to_m = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_to_m[i]);
    }
}

static gboolean post_from_to_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        gint from_h, from_m, to_h, to_m;
        gint idx, res;

        idx = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "time_check"));

        res = _get_cmp_from_to_time(idx);

        if (res == 0)
        {
            from_h = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_from_h[idx]);
            from_m = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_from_m[idx]);
            to_h = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_to_h[idx]);
            to_m = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_to_m[idx]);

            _set_pre_from_to_time(idx, from_h, from_m, to_h, to_m);
        }
        else if (res == -1)
        {
            nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_from_h[idx], g_pre_from_h[idx]);
            nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_from_m[idx], g_pre_from_m[idx]);
            nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_to_h[idx], g_pre_to_h[idx]);
            nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_to_m[idx], g_pre_to_m[idx]);

            nfui_signal_emit(g_from_h[idx], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_from_m[idx], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_to_h[idx], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_to_m[idx], GDK_EXPOSE, TRUE);
        }
        else if (res == 1)
        {
            from_h = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_from_h[idx]);
            from_m = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_from_m[idx]);
            to_h = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_to_h[idx]);
            to_m = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_to_m[idx]);

            _set_pre_from_to_time(idx, from_h, from_m, to_h, to_m);
        }
    }

    return FALSE;
}

static gboolean post_al_switch_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint idx;

    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        idx = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        if ((idx == 1) && (g_pre_al_switch == 0)) {
            nftool_mbox(g_curwnd, "WARNING", "When Alarm Monitoring is set to DISARM, any motion event and alarm input event e-mail notifications will not be sent.\n"
                                             "This disarm function operates outside of other alarm schedule settings and event e-mail notifications\n"
                                             "will not be sent by the system until the switch is set back to ARM.", NFTOOL_MB_OK);
        }
        g_pre_al_switch = idx;

        if (idx) nfui_nfobject_enable(g_al_switch_port);
        else     nfui_nfobject_disable(g_al_switch_port);

        nfui_signal_emit(g_al_switch_port, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_edit_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint i;
		gchar addr[EMAIL_COUNT][EMAIL_STRING_LENGTH];

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {
			return FALSE;
		}

		memset(addr, 0x00, sizeof(gchar) * EMAIL_COUNT * EMAIL_STRING_LENGTH);
		g_memmove(addr, g_emd.address, sizeof(gchar) * EMAIL_COUNT * EMAIL_STRING_LENGTH);

		VW_EvtNoti_Email_Edit(g_curwnd, addr);
		//for(i=0; i<EMAIL_COUNT; i++)
		//g_message("%s :::::::::::::::::::::::::%d  %s ::::::::::", __FUNCTION__, i, addr[i]);

		if(memcmp(g_emd.address, addr, sizeof(gchar) * EMAIL_COUNT * EMAIL_STRING_LENGTH)) {
			g_memmove(g_emd.address, addr, sizeof(gchar) * EMAIL_COUNT * EMAIL_STRING_LENGTH);

			g_emd.count = 0;
			for(i=0; i<EMAIL_COUNT; i++) {
				if(strlen(addr[i]))
					g_emd.count += 1;
			}
		}

		//for(i=0; i<EMAIL_COUNT; i++)
		//g_message("%s :::::::::::::::::::::::::%d  %s ::::::::::", __FUNCTION__, i, g_emd.address[i]);
	}
	return FALSE;
}

static gboolean post_start_time_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        gint idx;

        idx = nfui_spin_button_get_index(obj);

        g_emd.from_time = idx;
    }

    return FALSE;
}

static gboolean post_end_time_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        gint idx;

        idx = nfui_spin_button_get_index(obj);

        g_emd.to_time = idx;
    }

    return FALSE;
}

static gboolean post_allcheck_button_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        gint sched_num, i, act;

        sched_num = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "all_check"));
        act = nfui_check_button_get_active((NFCHECKBUTTON*)obj);

        for (i = 0; i < 7; i++)
        {
            nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_day[sched_num][i], act);
            nfui_signal_emit(g_day[sched_num][i], GDK_EXPOSE, TRUE);
        }

        _set_active_timeobj(sched_num, 1);
    }

    return FALSE;
}

static gboolean post_daycheck_button_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        gint idx;

        idx = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "day_check"));

        _set_active_timeobj(idx, 1);
        _set_all_chk_btn(idx, 1);
    }

    return FALSE;
}

static gboolean post_cancel_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {
			return FALSE;
		}

		g_memmove(&g_emd, &g_oemd, sizeof(EA_EvtNotiEmailData));

		set_data_to_obj(1);
		g_pre_al_switch = g_emd.al_switch;

        if (supp_al_switch) {
            if (g_emd.al_switch) nfui_nfobject_enable(g_al_switch_port);
            else     nfui_nfobject_disable(g_al_switch_port);

            nfui_signal_emit(g_al_switch_port, GDK_EXPOSE, TRUE);
        }

	}
	return FALSE;
}

static gboolean post_apply_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
	    gint i;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {
			return FALSE;
		}

        for (i = 0; i < EMAIL_SCHEDULING_MAX_CNT; i++)
        {
            if (_get_cmp_from_to_time(i) == -1)
            {
                nftool_mbox(g_curwnd, "ERROR", "Start time cannot be set later than the end time.", NFTOOL_MB_OK);
                return FALSE;
            }
        }

		get_data_from_obj();

		if(memcmp(&g_emd, &g_oemd, sizeof(EA_EvtNotiEmailData))) {
			set_em_data();

			nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");

			event_act_data_changed(TRUE);
		}
	}
	return FALSE;
}

static gboolean post_close_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {
			return FALSE;
		}

		VW_EvtNotiEvt_tab_out_handler();
		VW_Evt_Act_Destroy(obj);
	}

	return FALSE;
}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
	}

	return FALSE;
}


void VW_Init_EvtNoti_Email_Page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *chk_fixed;
	NFOBJECT *obj;
	NFOBJECT *fixed_temp;

	const gchar *strFrq[] = {
		"IMMEDIATELY",
		"1 MIN",
		"5 MIN",
		"10 MIN",
		"15 MIN",
		"30 MIN",
		"60 MIN"
	};

	gchar *strdayTitle[] = {"ALL", "SUN","MON","TUE","WED","THU","FRI","SAT", "FROM", "TO"};
	const gchar *strTime[] = { "00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "10", "11", "12", "13", "14",
							   "15", "16", "17", "18", "19", "20", "21", "22", "23", "24", "25", "26", "27", "28", "29",
							   "30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "40", "41", "42", "43", "44",
							   "45", "46", "47", "48", "49", "50", "51", "52", "53", "54", "55", "56", "57", "58", "59"};
    gchar *strMode[] = {"NOT USED", "USE"};
    gchar *strPort[] = {"ARI"};

	GSList *slist = NULL;
	GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];
	gchar strBuf[32];

	guint chk_w, chk_h;
	gint pos_x, pos_y;
	guint size_w, size_h;
	gint i, j;


    supp_al_switch = ivsc.dfunc.support_al_switch;

	g_curwnd = nfui_nfobject_get_top(parent);

	// init data
	init_em_data();
	g_pre_al_switch = g_emd.al_switch;

	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);

	pos_x = 27;
	pos_y = 13;

	/* title */
	obj = nfui_nfimage_new(IMG_TITLE_BG2);
	nfui_nfimage_set_text((NFIMAGE*)obj, "E-MAIL NOTIFICATION");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	pos_y += 61;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ADD NEW E-MAIL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nfobject_set_size(obj, 414, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	pos_x += 454;

	obj = nftool_normal_button_create_subtab_type1("EDIT", 220);
	nfui_regi_post_event_callback(obj, post_edit_button_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	pos_x = 27;
	pos_y += 42;

#if defined(_SUPPORT_DUAL_SMTPSERVER)
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("MAIL SERVER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 414, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	radio_img[0] = nfui_get_image_from_file((IMG_N_SUBTAB_RADIO_OFF), NULL);
	radio_img[1] = nfui_get_image_from_file((IMG_O_SUBTAB_RADIO_ON), NULL);
	radio_img[2] = nfui_get_image_from_file((IMG_P_SUBTAB_RADIO_ON), NULL);
	radio_img[3] = nfui_get_image_from_file((IMG_D_SUBTAB_RADIO_OFF), NULL);

	pos_x += 464;

	nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

	for (i = 0; i < 2; i++)
	{
		obj = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
		nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
		nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y + (40-size_h)/2);
		g_server_obj[i] = obj;

		if (i == g_emd.serv)
			nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);

		if (i == 0) 	slist = nfui_radio_button_get_group(NF_BUTTON(obj));
		else nfui_radio_button_add_group(NF_BUTTON(obj), slist);

		pos_x += 40;

		g_sprintf(strBuf, "%2d", i+1);

		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_set_size(obj, 60, 40);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

		pos_x += 60;
	}

	pos_x = 27;
	pos_y += 42;
#endif

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("MINIMUM E-MAIL FREQUENCY", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_set_size(obj, 414, 40);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	pos_x += 454;

	g_freqObj = nfui_combobox_new(strFrq, NUM_FREQS, prvFreqToIndex(g_emd.frequency));
	nfui_combobox_set_skin_type(NF_COMBOBOX(g_freqObj), NFCOMBOBOX_TYPE_SUBTAB_1);
	nfui_combobox_set_align(NF_COMBOBOX(g_freqObj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(g_freqObj, 220, 40);
	nfui_nfobject_show(g_freqObj);
	nfui_nffixed_put((NFFIXED*)content_fixed, g_freqObj, pos_x, pos_y);

	pos_x = 27;
	pos_y += 42;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("INCLUDE SNAPSHOT IMAGE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nfobject_set_size(obj, 414, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	pos_x += 454;

	chk_fixed = nfui_nffixed_new();
	nfui_nfobject_set_size(chk_fixed, 220, 40);
	nfui_nfobject_modify_bg(chk_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(128));
	nfui_nfobject_show(chk_fixed);
	nfui_nffixed_put((NFFIXED*)content_fixed, chk_fixed, pos_x, pos_y);

	obj = nfui_checkbutton_new(g_emd.include_jpeg);
	nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
	nfui_check_get_size(obj, &chk_w, &chk_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)chk_fixed, obj, (220-chk_w)/2, (40-chk_h)/2);
	g_jpegChk = obj;

	if(!DAL_get_support_snapshot())
		nfui_nfobject_disable(g_jpegChk);

    if (supp_al_switch)
    {
    	pos_x = 27;
    	pos_y += 42;

    	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ARM/DISARM SWITCH", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    	nfui_nfobject_set_size(obj, 414, 40);
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
    	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

    	pos_x += 454;

    	obj = nfui_combobox_new(strMode, 2, g_emd.al_switch);
    	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_SUBTAB_1);
    	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
    	nfui_nfobject_set_size(obj, 220, 40);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
    	nfui_regi_post_event_callback(obj, post_al_switch_event_handler);
    	g_al_switch = obj;

    	pos_x = 27;
    	pos_y += 42;

    	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ARM/DISARM PORT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    	nfui_nfobject_set_size(obj, 414, 40);
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
    	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

    	pos_x += 454;

#ifdef ENABLE_ARI_PANIC
    	obj = nfui_combobox_new(strPort, 1, 0);
#else
        obj = nfui_combobox_new(NULL, 0, 0);
#endif
    	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_SUBTAB_1);
    	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
    	nfui_nfobject_set_size(obj, 220, 40);
    	nfui_nfobject_show(obj);
    	nfui_nfobject_disable(obj);
    	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
    	g_al_switch_port = obj;

    	for (i = 0; i < var_get_dvr_alarmIn_cnt(); i++)
    	{
    		g_sprintf(strBuf, "AI %d", (i + 1));

		    nfui_combobox_append_data((NFCOMBOBOX*)obj, strBuf);
		}

		_trans_data_to_val(g_emd.al_switch_port, strBuf);
		nfui_combobox_set_data_no_expose((NFCOMBOBOX*)obj, strBuf);

		if (g_emd.al_switch) nfui_nfobject_enable(g_al_switch_port);
    }

    pos_y += 40 + 15;

    obj = nfui_nfimage_new(IMG_TITLE_BG2);
	nfui_nfimage_set_text((NFIMAGE*)obj, "E-MAIL NOTIFICATION SCHEDULE SETUP");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, 27, pos_y);

    pos_y += 50;
    pos_x = 481;

    for(i = 0; i < 9; i++)
    {
        if (i == 8)
        {
            sprintf(strBuf, "%s ~ %s", lookup_string("FROM"), lookup_string("TO"));
            obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
        }
        else
        {
            obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strdayTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
        }
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

        if (i == 8) //time
        {
            nfui_nfobject_set_size(obj, 280, 40);
            //pos_x += 132 + 20;
        }
        else
        {
            nfui_nfobject_set_size(obj, 60, 40);
            if (i == 7)
                pos_x += 60 + 32;
            else
                pos_x += 60 + 2;
        }
    }

    pos_y += 41;

    for (j = 0; j < EMAIL_SCHEDULING_MAX_CNT; j++)
    {
        pos_x = 27;

        g_sprintf(strBuf, "%s %d", lookup_string("SCHEDULE."), j + 1);
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
        nfui_nfobject_set_size(obj, 414, 40);
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

        pos_x = 481;

        fixed_temp = (NFOBJECT *)nfui_nffixed_new();
        nfui_nfobject_modify_bg(fixed_temp, NFOBJECT_STATE_NORMAL, COLOR_IDX(128));
        nfui_nfobject_set_size(fixed_temp, 60, 40);
        nfui_nfobject_show(fixed_temp);
        nfui_nffixed_put((NFFIXED*)content_fixed, fixed_temp, pos_x, pos_y);

        obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
        nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
        nfui_check_get_size(obj, &size_w, &size_h);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (guint)(60 - size_w)/2, (guint)(40-size_h)/2);
        g_all[j] = obj;

        nfui_nfobject_set_data(obj, "all_check", GUINT_TO_POINTER(j));
        nfui_regi_post_event_callback(obj, post_allcheck_button_handler);

        pos_x += 60 + 2;

        for(i = 0; i < 7; i++)  //checkbutton
        {
            fixed_temp = (NFOBJECT *)nfui_nffixed_new();
            nfui_nfobject_modify_bg(fixed_temp, NFOBJECT_STATE_NORMAL, COLOR_IDX(128));
            nfui_nfobject_set_size(fixed_temp, 60, 40);
            nfui_nfobject_show(fixed_temp);
            nfui_nffixed_put((NFFIXED*)content_fixed, fixed_temp, pos_x, pos_y);

            obj = (NFOBJECT*)nfui_checkbutton_new((g_emd.sched[j].wday & (1 << i)));
            nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
            nfui_check_get_size(obj, &size_w, &size_h);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (guint)(60 - size_w)/2, (guint)(40-size_h)/2);
            g_day[j][i] = obj;

            nfui_nfobject_set_data(obj, "day_check", GUINT_TO_POINTER(j));
            nfui_regi_post_event_callback(obj, post_daycheck_button_handler);

            pos_x += 60+2;
        }

        pos_x += 30;

        obj = (NFOBJECT*)nfui_combobox_new(strTime, 24, g_emd.sched[j].from_h);
    	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_SUBTAB_1);
    	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
    	nfui_nfobject_set_size(obj, 60, 40);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
    	nfui_nfobject_set_data(obj, "time_check", GUINT_TO_POINTER(j));
    	//nfui_regi_post_event_callback(obj, post_from_to_event_handler);
    	g_from_h[j] = obj;

        pos_x += 60;

    	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(":", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    	nfui_nfobject_set_size(obj, 10, 40);
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

    	pos_x += 10;

        obj = (NFOBJECT*)nfui_combobox_new(strTime, 60, g_emd.sched[j].from_m);
    	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_SUBTAB_1);
    	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
    	nfui_nfobject_set_size(obj, 60, 40);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
    	nfui_nfobject_set_data(obj, "time_check", GUINT_TO_POINTER(j));
    	//nfui_regi_post_event_callback(obj, post_from_to_event_handler);
    	g_from_m[j] = obj;

    	pos_x += 60;

    	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("~", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    	nfui_nfobject_set_size(obj, 20, 40);
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

    	pos_x += 20;

        obj = (NFOBJECT*)nfui_combobox_new(strTime, 24, g_emd.sched[j].to_h);
    	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_SUBTAB_1);
    	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
    	nfui_nfobject_set_size(obj, 60, 40);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
    	nfui_nfobject_set_data(obj, "time_check", GUINT_TO_POINTER(j));
    	//nfui_regi_post_event_callback(obj, post_from_to_event_handler);
    	g_to_h[j] = obj;

        pos_x += 60;

    	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(":", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    	nfui_nfobject_set_size(obj, 10, 40);
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

    	pos_x += 10;

        obj = (NFOBJECT*)nfui_combobox_new(strTime, 60, g_emd.sched[j].to_m);
    	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_SUBTAB_1);
    	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
    	nfui_nfobject_set_size(obj, 60, 40);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
    	nfui_nfobject_set_data(obj, "time_check", GUINT_TO_POINTER(j));
    	//nfui_regi_post_event_callback(obj, post_from_to_event_handler);
    	g_to_m[j] = obj;

        pos_x = 481;
    	pos_y += 42;

    	_set_active_timeobj(j, 0);
    	_set_all_chk_btn(j, 0);
    	_set_pre_from_to_time(j, g_emd.sched[j].from_h, g_emd.sched[j].from_m, g_emd.sched[j].to_h, g_emd.sched[j].to_m);
    }

	/* button */
	obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R3_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_cancel_button_event_handler);

	obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R2_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_apply_button_event_handler);

	obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R1_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_close_button_event_handler);

	nfui_regi_post_event_callback(parent, post_page_event_handler);
}

gboolean check_evt_noti_email_changed()
{
    gint i;
    guint err_idx = 0;

    for (i = 0; i < EMAIL_SCHEDULING_MAX_CNT; i++)
    {
        if (_get_cmp_from_to_time(i) == -1)
        {
            err_idx |= (1 << i);
        }
    }

    if (err_idx)
    {
        nftool_mbox(g_curwnd, "ERROR", "Start time cannot be set later than the end time.", NFTOOL_MB_OK);

        for (i = 0; i < EMAIL_SCHEDULING_MAX_CNT; i++)
        {
            if (err_idx & (1 << i))
            {
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_from_h[i], g_pre_from_h[i]);
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_from_m[i], g_pre_from_m[i]);
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_to_h[i], g_pre_to_h[i]);
                nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_to_m[i], g_pre_to_m[i]);
            }
        }
    }

	get_data_from_obj();

	if(!memcmp(&g_emd, &g_oemd, sizeof(EA_EvtNotiEmailData)))
		return FALSE;

	return TRUE;
}

void save_evt_noti_email_data()
{
	set_em_data();
}

void restore_evt_noti_email_data()
{
	g_memmove(&g_emd, &g_oemd, sizeof(EA_EvtNotiEmailData));

	set_data_to_obj(0);
	g_pre_al_switch = g_emd.al_switch;

    if (supp_al_switch) {
        if (g_emd.al_switch) nfui_nfobject_enable(g_al_switch_port);
        else     nfui_nfobject_disable(g_al_switch_port);
    }
}
