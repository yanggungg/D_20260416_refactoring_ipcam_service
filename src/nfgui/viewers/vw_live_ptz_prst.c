
#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_function.h"
#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"
#include "objects/nfcombobox.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfimage.h"

#include "nf_ipcam_defs.h"

#include "vw_live_ptz_main.h"
#include "vw_live_ptz_internal.h"
#include "vw_live_ptz_scan_popup.h"
#include "vw_live_ptz_tour_popup.h"

#include "ix_mem.h"
#include "nfdal.h"
#include "vsm.h"
#include "ssm.h"
#include "smt.h"
#include "scm.h"
#include "vw.h"
#include "uxm.h"

#include "vw_vkeyboard.h"
#include "vw_vkeyboard_num.h"

#define PRESET_NAME_LEN                 (16)
#define ROW_COUNT					    (MAX_PRESET_COUNT)

#define PRST_LABEL_H                    (40)

#define PRST_CAM_LABEL_X                (20)
#define PRST_CAM_LABEL_Y                (17)
#define PRST_CAM_LABEL_W                (80)

#define PRST_CAM_COMBO_W                (268)

#define PRST_NO_LABEL_X                 (PRST_CAM_LABEL_X+17)
#define PRST_NO_LABEL_W                 (83)
#define PRST_NAME_LABEL_W               (227)

#define PRST_NO_INPUT_W                 (60)
#define PRST_NAME_INPUT_W               (211)

#define PRST_TOUR_COMBO_W               (159)

enum {
	NUM_COLUMN = 0,
	NAME_COLUMN,
	RUN_COLUMN,
	DEL_COLUMN,
	COL_COUNT
};

static NFOBJECT *g_ch_obj;
static NFOBJECT *g_sync_obj;
static NFOBJECT *g_number_obj;
static NFOBJECT *g_numberup_obj;
static NFOBJECT *g_numberdown_obj;
static NFOBJECT *g_name_obj;
static NFOBJECT *g_set_obj;
static NFOBJECT *g_bookmark_obj;
static NFOBJECT *g_run_obj;
static NFOBJECT *g_list[ROW_COUNT][COL_COUNT];
static NFOBJECT *g_mode_obj = 0;
static NFOBJECT *g_onoff_obj[2];
static NFOBJECT *g_stg_obj;

static GSList *radioList = NULL;

static NFWINDOW *g_curwnd = 0;

typedef struct _PRESET_INFO_T {
	guint 	    number;
	gchar 	    name[32];
} PRESET_INFO;

static GSList *g_preset_list = NULL;

static gchar *strCh[GUI_CHANNEL_CNT];
static gint retVal = 0;

static PtzData *g_ptzData;
static PtzPresetData *g_presetData;
static PtzScanData *g_scanData;
static PtzTourData *g_tourData;

static EA_AlmSenData *g_asd;
static EA_MotSenData *g_msd;
static EA_VLossData *g_vld;
static EA_PosData *g_psd;

static guint g_ch;


static gint _init_supported_func(gint ch)
{
	gint support;
	gint i;

	support = scm_get_ipcam_preset_supp(ch);

	if (!support)
	{
		nfui_nfobject_enable((NFOBJECT*)g_sync_obj);
		nfui_nfobject_enable((NFOBJECT*)g_number_obj);
		nfui_nfobject_enable((NFOBJECT*)g_numberup_obj);
		nfui_nfobject_enable((NFOBJECT*)g_numberdown_obj);
		nfui_nfobject_enable((NFOBJECT*)g_name_obj);
		nfui_nfobject_enable((NFOBJECT*)g_run_obj);
		nfui_nfobject_enable((NFOBJECT*)g_set_obj);
		nfui_nfobject_enable((NFOBJECT*)g_bookmark_obj);
		if (g_mode_obj) nfui_nfobject_enable((NFOBJECT*)g_mode_obj);
		nfui_nfobject_enable((NFOBJECT*)g_onoff_obj[0]);
		nfui_nfobject_enable((NFOBJECT*)g_onoff_obj[1]);
		nfui_nfobject_enable((NFOBJECT*)g_stg_obj);

		for (i = 0; i < ROW_COUNT; i++)
		{
			if (i < g_presetData->cnt) {
				nfui_nfobject_enable((NFOBJECT*)g_list[i][RUN_COLUMN]);
				nfui_nfobject_enable((NFOBJECT*)g_list[i][DEL_COLUMN]);
			}
			else {
				nfui_nfobject_disable((NFOBJECT*)g_list[i][RUN_COLUMN]);
				nfui_nfobject_disable((NFOBJECT*)g_list[i][DEL_COLUMN]);
			}
		}
	}
	else
	{
		nfui_nfobject_disable((NFOBJECT*)g_sync_obj);
		nfui_nfobject_disable((NFOBJECT*)g_number_obj);
		nfui_nfobject_disable((NFOBJECT*)g_numberup_obj);
		nfui_nfobject_disable((NFOBJECT*)g_numberdown_obj);
		nfui_nfobject_disable((NFOBJECT*)g_name_obj);
		nfui_nfobject_disable((NFOBJECT*)g_run_obj);
		nfui_nfobject_disable((NFOBJECT*)g_set_obj);
		nfui_nfobject_disable((NFOBJECT*)g_bookmark_obj);
		if (g_mode_obj) nfui_nfobject_disable((NFOBJECT*)g_mode_obj);
		nfui_nfobject_disable((NFOBJECT*)g_onoff_obj[0]);
		nfui_nfobject_disable((NFOBJECT*)g_onoff_obj[1]);
		nfui_nfobject_disable((NFOBJECT*)g_stg_obj);

		for (i = 0; i < ROW_COUNT; i++)
		{
			nfui_nfobject_disable((NFOBJECT*)g_list[i][RUN_COLUMN]);
			nfui_nfobject_disable((NFOBJECT*)g_list[i][DEL_COLUMN]);
		}
	}

	return 0;
}

static gint _init_ptzMode(guint mode)
{
	if (nfui_nfobject_is_disabled(g_onoff_obj[0])) return -1;

    if (mode == PTZ_MODE_SCAN)
    {
        if (g_mode_obj) nfui_combobox_set_index_no_expose(NF_COMBOBOX(g_mode_obj), 0);
        nfui_radio_button_set_toggled(NF_BUTTON(g_onoff_obj[0]), TRUE);
    }
    else if (mode == PTZ_MODE_TOUR)
    {
        if (g_mode_obj) nfui_combobox_set_index_no_expose(NF_COMBOBOX(g_mode_obj), 1);
        nfui_radio_button_set_toggled(NF_BUTTON(g_onoff_obj[0]), TRUE);
    }
    else
    {
        if (g_mode_obj) nfui_combobox_set_index_no_expose(NF_COMBOBOX(g_mode_obj), 0);
        nfui_radio_button_set_toggled(NF_BUTTON(g_onoff_obj[1]), TRUE);
    }

    return 0;
}

static gint _new_preset_list(guint channel)
{
    guint i;
    PRESET_INFO *info;

    for (i = 0; i < g_presetData->cnt; i++)
    {
    	info = imalloc(sizeof(PRESET_INFO));

        info->number = g_presetData->number[i];
        strcpy(info->name, g_presetData->name[i]);
		g_preset_list = g_slist_append(g_preset_list, info);
    }

    return 0;
}

static gint _free_preset_list()
{
    PRESET_INFO *info;
    gint i, cnt;

	cnt = g_slist_length(g_preset_list);

    if (cnt)
    {
        for (i = 0; i < cnt; i++)
        {
    		info = g_slist_nth_data(g_preset_list, i);
    		g_free(info);
    		info = NULL;
        }
    }

    if (g_preset_list)
    	g_slist_free(g_preset_list);

	g_preset_list = NULL;

    return 0;
}

static gint _sync_preset_onvif(gint ch, onvif_bookmark *info)
{
    gint i, cnt;

	g_presetData->cnt = info->bookmark_cnt;

	for (i = 0; i < ROW_COUNT; i++)
    {
        if (i < info->bookmark_cnt)
        {
            g_presetData->number[i] = info->bookmark_preset_number[i];
            g_sprintf(g_presetData->name[i], "PRESET-%d", info->bookmark_preset_number[i]);
        }
        else
        {
            g_presetData->number[i] = 0;
            memset(g_presetData->name[i], 0x00, sizeof(gchar)*32);
        }
    }

	return 0;
}

static gint _sync_preset_db()
{
    PRESET_INFO *info;
    gint i, cnt;

	cnt = g_slist_length(g_preset_list);
    g_presetData->cnt = cnt;

    for (i = 0; i < ROW_COUNT; i++)
    {
        if (i < cnt)
        {
    		info = g_slist_nth_data(g_preset_list, i);

            g_presetData->number[i] = info->number;
            strcpy(g_presetData->name[i], info->name);
        }
        else
        {
            g_presetData->number[i] = 0;
            memset(g_presetData->name[i], 0x00, sizeof(gchar)*32);
        }
    }

	return 0;
}

static gint _del_preset_scan_tour(gint num)
{
    gint i;

//check scan
    for (i = 0; i < 2; i++)
    {
        if (g_scanData->number[i] == num)
            g_scanData->number[i] = 0;
    }

//check tour
    for (i = 0; i < MAX_TOUR_COUNT; i++)
    {
        if (g_tourData->number[i] == num)
            g_tourData->number[i] = 0;
    }

    return 0;
}

static gint _del_preset_event(gint num)
{
    gint i, j;

//check alarm sensor
    for (i = 0; i < var_get_alarmIn_cnt(); i++)
    {
        for (j = 0; j < GUI_CHANNEL_CNT; j++)
        {
            if (g_asd[i].preset_num[j] == num)
                g_asd[i].preset_num[j] = 0;
        }
    }

//check motion sensor
    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        for (j = 0; j < GUI_CHANNEL_CNT; j++)
        {
            if (g_msd[i].preset_num[j] == num)
                g_msd[i].preset_num[j] = 0;
        }
    }

//check video loss
    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        for (j = 0; j < GUI_CHANNEL_CNT; j++)
        {
            if (g_vld[i].preset_num[j] == num)
                g_vld[i].preset_num[j] = 0;
        }
    }

//check pos event
    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        for (j = 0; j < GUI_CHANNEL_CNT; j++)
        {
            if (g_psd[i].preset_num[j] == num)
                g_psd[i].preset_num[j] = 0;
        }
    }


    return 0;
}

static gint _is_exist_preset_num(gint num)
{
	PRESET_INFO *info;
	gint i, cnt;

	cnt = g_slist_length(g_preset_list);

	for (i = 0; i < cnt; i++)
	{
		info = g_slist_nth_data(g_preset_list, i);
		if (info->number == num) return 1;
	}

	return 0;
}

static gint _add_preset_list(gint num, char *name)
{
    PRESET_INFO *info;
    gint i, cnt;

	cnt = g_slist_length(g_preset_list);

    if (cnt == ROW_COUNT) return -1;

    if (cnt)
    {
		for (i = 0; i < cnt; i++)
		{
			info = g_slist_nth_data(g_preset_list, i);

            if (info->number == num)
            {
                strcpy(info->name, name);
                return 0;
            }
		}

		for (i = 0; i < cnt; i++)
		{
			info = g_slist_nth_data(g_preset_list, i);

            if (info->number > num)
                break;
		}

    	info = imalloc(sizeof(PRESET_INFO));

        info->number = num;
        strcpy(info->name, name);
		g_preset_list = g_slist_insert(g_preset_list, info, i);
    }
    else
    {
    	info = imalloc(sizeof(PRESET_INFO));

        info->number = num;
        strcpy(info->name, name);
		g_preset_list = g_slist_append(g_preset_list, info);
    }

    return 0;
}

static gint _modify_preset_list(gint num, char *name)
{
    PRESET_INFO *info;
    gint i, cnt;

	cnt = g_slist_length(g_preset_list);

	for (i = 0; i < cnt; i++)
	{
		info = g_slist_nth_data(g_preset_list, i);

        if (info->number == num)
        {
            strcpy(info->name, name);
            return 0;
        }
	}

    return 0;
}

static gint _del_preset_list(gint num)
{
    PRESET_INFO *info;
    gint i, cnt;

	cnt = g_slist_length(g_preset_list);

	for (i = 0; i < cnt; i++)
	{
		info = g_slist_nth_data(g_preset_list, i);

        if (info->number == num)
        {
            g_preset_list = g_slist_delete_link(g_preset_list, g_slist_nth(g_preset_list, i));
            break;
        }
	}

    return 0;
}

static gint _display_preset_list()
{
    gint i;
    gchar strBuf[8];

    for (i = 0; i < ROW_COUNT; i++)
    {
        if (i < g_presetData->cnt)
        {
            g_sprintf(strBuf, "%d", g_presetData->number[i]);
            nfui_nflabel_set_text((NFLABEL *)g_list[i][NUM_COLUMN], strBuf);
            nfui_nflabel_set_text((NFLABEL *)g_list[i][NAME_COLUMN], g_presetData->name[i]);
            nfui_nfobject_enable(g_list[i][RUN_COLUMN]);
            nfui_nfobject_enable(g_list[i][DEL_COLUMN]);
        }
        else
        {
            nfui_nflabel_set_text((NFLABEL *)g_list[i][NUM_COLUMN], "");
            nfui_nflabel_set_text((NFLABEL *)g_list[i][NAME_COLUMN], "");
            nfui_nfobject_disable(g_list[i][RUN_COLUMN]);
            nfui_nfobject_disable(g_list[i][DEL_COLUMN]);
        }
    }
}

static gint _get_preset_name(gint num, gchar *strBuf)
{
    gint i;

	for (i = 0; i < g_presetData->cnt; i++)
	{
        if (g_presetData->number[i] == num)
        {
            strcpy(strBuf, g_presetData->name[i]);
            return 0;
        }
	}

    g_sprintf(strBuf, "PRESET-%d", num);

	return 0;
}

static gint _change_obj_focus(NFOBJECT* from, NFOBJECT *to)
{
	nfui_set_key_focus(from, FALSE);
	nfui_set_key_focus(to, TRUE);

	nfui_signal_emit(from, GDK_EXPOSE, TRUE);
	nfui_signal_emit(to, GDK_EXPOSE, TRUE);

	return 0;
}

static gboolean _post_sync_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    onvif_bookmark bookmark_info;
	gint i, ret;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		ret = nftool_mbox(g_curwnd, "CONFIRM", "Sync and import camera preset information.\nPreset names in the favorites list will display as initial values while syncing.\nContinue to sync?", NFTOOL_MB_OKCANCEL);
		if (ret == NFTOOL_MB_CANCEL) return FALSE;

		memset(&bookmark_info, 0x00, sizeof(onvif_bookmark));
		ret = nf_ipcam_get_onvif_bookmark_presets(g_ch, &bookmark_info);
		if (ret != IPCAM_SETUP_RTN_DONE) {
			nftool_mbox(g_curwnd, "WARNING", "Synchronization failed.", NFTOOL_MB_OK);
			return FALSE;
		}

        g_message("%s, %d, cnt:%d", __FUNCTION__, __LINE__, bookmark_info.bookmark_cnt);

		printf("[BOOKMARK - CH%02d]  ", g_ch);
		for (i = 0; i < 16; i++)
		{
			printf("[%d:%d]", i, bookmark_info.bookmark_preset_number[i]);
		}
		printf("\n");

        _sync_preset_onvif(g_ch, &bookmark_info);
		_free_preset_list();
		_new_preset_list(g_ch);
        _display_preset_list();

        for (i = 0; i < ROW_COUNT; i++)
        {
            nfui_signal_emit(g_list[i][NUM_COLUMN], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_list[i][NAME_COLUMN], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_list[i][RUN_COLUMN], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_list[i][DEL_COLUMN], GDK_EXPOSE, TRUE);
        }

		nftool_mbox_auto_ok(g_curwnd, 5, "NOTICE", "Synchronization is complete and the list has been updated.");
	}

    return FALSE;
}

static gboolean _post_no_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;
	gint numTemp, numRet;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{
		NFOBJECT *top;
		gint x, y;
        gchar strBuf[PRESET_NAME_LEN+1];

		if(kpid == KEYPAD_ENTER)
		{
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);

			x += (obj->width)/2 + top->x;
			y += obj->height + top->y;
		}
		else
		{
	  	   	if(evt->button.button == MOUSE_RIGTH_BUTTON)
				return FALSE;

			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += ((GdkEventButton*)evt)->x;
			y += ((GdkEventButton*)evt)->y;
		}

		numTemp = nfui_nflabel_get_number((NFLABEL*)obj);
		numRet = NumberKey_Open(g_curwnd, numTemp, x, y, 255);

        if ((numRet < 1) || (numRet > 255)) return FALSE;

        nfui_nflabel_set_number((NFLABEL*)obj, numRet);
		nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

        memset(strBuf, 0x00, sizeof(strBuf));
        _get_preset_name(numRet, strBuf);
        nfui_nflabel_set_text((NFLABEL*)g_name_obj, strBuf);
        nfui_signal_emit(g_name_obj, GDK_EXPOSE, TRUE);
	}
	else if (evt->type == GDK_SCROLL)
	{
        gint number;
		GdkEventScroll *sevt;
        gchar strBuf[PRESET_NAME_LEN+1];

		sevt = (GdkEventScroll*)evt;
        number = nfui_nflabel_get_number((NFLABEL*)obj);

		if (sevt->direction == GDK_SCROLL_UP) {
            if (number == 255)  number = 1;
            else                number += 1;
		}
		else if (sevt->direction == GDK_SCROLL_DOWN) {
            if (number == 1)    number = 255;
            else                number -= 1;
		}

        nfui_nflabel_set_number((NFLABEL*)obj, number);
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

        memset(strBuf, 0x00, sizeof(strBuf));
        _get_preset_name(number, strBuf);
        nfui_nflabel_set_text((NFLABEL*)g_name_obj, strBuf);
        nfui_signal_emit(g_name_obj, GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

static gboolean _post_no_up_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint number;
    gchar strBuf[PRESET_NAME_LEN+1];

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

        number = nfui_nflabel_get_number((NFLABEL*)g_number_obj);

        if (number == 255)  number = 1;
        else                number += 1;

        nfui_nflabel_set_number((NFLABEL*)g_number_obj, number);
        nfui_signal_emit(g_number_obj, GDK_EXPOSE, TRUE);

        memset(strBuf, 0x00, sizeof(strBuf));
        _get_preset_name(number, strBuf);
        nfui_nflabel_set_text((NFLABEL*)g_name_obj, strBuf);
        nfui_signal_emit(g_name_obj, GDK_EXPOSE, TRUE);
	}

    return FALSE;
}

static gboolean _post_no_down_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint number;
    gchar strBuf[PRESET_NAME_LEN+1];

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

        memset(strBuf, 0x00, sizeof(strBuf));
        number = nfui_nflabel_get_number((NFLABEL*)g_number_obj);

        if (number == 1)    number = 255;
        else                number -= 1;

        nfui_nflabel_set_number((NFLABEL*)g_number_obj, number);
        nfui_signal_emit(g_number_obj, GDK_EXPOSE, TRUE);

        _get_preset_name(number, strBuf);
        nfui_nflabel_set_text((NFLABEL*)g_name_obj, strBuf);
        nfui_signal_emit(g_name_obj, GDK_EXPOSE, TRUE);
	}

    return FALSE;
}

static gboolean _post_name_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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
		NFOBJECT *top;
		gchar *strTemp;
		guint x, y;

		if(kpid == KEYPAD_ENTER)
		{
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);

			x += (obj->width)/2 + top->x;
			y += obj->height + top->y;
		}
		else
		{
	  	   	if(evt->button.button == MOUSE_RIGTH_BUTTON)
				return FALSE;

			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += ((GdkEventButton*)evt)->x;
			y += ((GdkEventButton*)evt)->y;
		}

		strTemp = VirtualKey_Open(g_curwnd, "", x, y, PRESET_NAME_LEN, VKEY_ITXSTYLE_TITLE);

		if (!strTemp) return FALSE;

		nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
		nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
		ifree(strTemp);
		strTemp = NULL;
	}

	return FALSE;
}

static gboolean _post_dir_run_btn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint number;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

        number = nfui_nflabel_get_number((NFLABEL*)g_number_obj);
        scm_run_ptz_cmd_goto(number);
	}

    return FALSE;
}

static gboolean _post_set_btn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint number;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        number = nfui_nflabel_get_number((NFLABEL*)g_number_obj);
        scm_set_ptz_preset(number);
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
        gint i, enable_idx = -1;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if (!g_mode_obj) return FALSE;

		if (kpid == KEYPAD_DOWN)
		{
            if (nfui_nfobject_is_disabled(g_list[0][RUN_COLUMN]))
            {
                _change_obj_focus(obj, g_mode_obj);
    			return TRUE;
            }
		}
	}

    return FALSE;
}

static gboolean _post_bookmark_btn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint number;
    gchar *str;
    gint i;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        number = nfui_nflabel_get_number((NFLABEL*)g_number_obj);
        str = nfui_nflabel_get_text((NFLABEL*)g_name_obj);

		if (_is_exist_preset_num(number)) return FALSE;
		
			if (g_slist_length(g_preset_list) == 16) {
				nftool_mbox(g_curwnd, "NOTICE", "This device can save up to 16 favorites.\nYou may set an unspecified preset as a favorite by clicking the Settings button.", NFTOOL_MB_OK);
				return FALSE;
			}

        scm_set_ptz_preset(number);
        _add_preset_list(number, str);
        _sync_preset_db();
        _display_preset_list();

        for (i = 0; i < ROW_COUNT; i++)
        {
            nfui_signal_emit(g_list[i][NUM_COLUMN], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_list[i][NAME_COLUMN], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_list[i][RUN_COLUMN], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_list[i][DEL_COLUMN], GDK_EXPOSE, TRUE);
        }
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
        gint i, enable_idx = -1;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if (!g_mode_obj) return FALSE;

		if (kpid == KEYPAD_DOWN)
		{
            if (nfui_nfobject_is_disabled(g_list[0][RUN_COLUMN]))
            {
                _change_obj_focus(obj, g_mode_obj);
    			return TRUE;
            }
		}
	}

    return FALSE;
}

static gboolean _post_run_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, num;
    gchar *numBuf;

	if(evt->type == GDK_BUTTON_RELEASE) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

        for (i = 0; i < ROW_COUNT; i++)
        {
            if (g_list[i][RUN_COLUMN] == obj)
                break;
        }

        numBuf = nfui_nflabel_get_text((NFLABEL *)g_list[i][NUM_COLUMN]);
        num = atoi(numBuf);

        scm_run_ptz_cmd_goto(num);
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
        gint i, enable_idx = -1;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if (kpid == KEYPAD_DOWN)
		{
            for (i = 0; i < ROW_COUNT; i++)
            {
                if (g_list[i][RUN_COLUMN] == obj)
                    break;
            }

            if (i+1 != ROW_COUNT)
            {
                if (nfui_nfobject_is_disabled(g_list[i+1][RUN_COLUMN]))
                    _change_obj_focus(obj, g_stg_obj);
                else
                    _change_obj_focus(obj, g_list[i+1][RUN_COLUMN]);
            }
            else
                _change_obj_focus(obj, g_stg_obj);

			return TRUE;
		}
	}

	return FALSE;
}

static gboolean _post_del_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, num;
    gchar *numBuf;

	if(evt->type == GDK_BUTTON_RELEASE) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

        for (i = 0; i < ROW_COUNT; i++)
        {
            if (g_list[i][DEL_COLUMN] == obj)
                break;
        }

        numBuf = nfui_nflabel_get_text((NFLABEL *)g_list[i][NUM_COLUMN]);
        num = atoi(numBuf);

//        scm_unset_ptz_preset(num);
        _del_preset_list(num);
        _sync_preset_db();
        _del_preset_scan_tour(num);
        _del_preset_event(num);
        _display_preset_list();

        for (i = 0; i < ROW_COUNT; i++)
        {
            nfui_signal_emit(g_list[i][NUM_COLUMN], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_list[i][NAME_COLUMN], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_list[i][RUN_COLUMN], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_list[i][DEL_COLUMN], GDK_EXPOSE, TRUE);
        }
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
        gint i, enable_idx = -1;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if (kpid == KEYPAD_DOWN)
		{
            for (i = 0; i < ROW_COUNT; i++)
            {
                if (g_list[i][DEL_COLUMN] == obj)
                    break;
            }

            if (i+1 != ROW_COUNT)
            {
                if (nfui_nfobject_is_disabled(g_list[i+1][DEL_COLUMN]))
                    _change_obj_focus(obj, g_stg_obj);
                else
                    _change_obj_focus(obj, g_list[i+1][DEL_COLUMN]);
            }
            else
                _change_obj_focus(obj, g_stg_obj);

			return TRUE;
		}
	}

	return FALSE;
}

static gboolean post_mode_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint mode;

	if (evt->type == NFEVENT_COMBOBOX_CHANGED)
	{
        mode = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));

        if (mode == 0)
        {
            if (g_ptzData->mode == PTZ_MODE_SCAN)
                nfui_radio_button_set_toggled(NF_BUTTON(g_onoff_obj[0]), TRUE);
            else
                nfui_radio_button_set_toggled(NF_BUTTON(g_onoff_obj[1]), TRUE);
        }
        else
        {
            if (g_ptzData->mode == PTZ_MODE_TOUR)
                nfui_radio_button_set_toggled(NF_BUTTON(g_onoff_obj[0]), TRUE);
            else
                nfui_radio_button_set_toggled(NF_BUTTON(g_onoff_obj[1]), TRUE);
        }

        nfui_signal_emit(g_onoff_obj[0], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_onoff_obj[1], GDK_EXPOSE, TRUE);

        _live_ptz_main_cmd_pattern(g_ch);
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
        gint i, enable_idx = -1;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if (kpid == KEYPAD_UP)
		{
            for (i = 0; i < ROW_COUNT; i++)
            {
                if (nfui_nfobject_is_disabled(g_list[i][RUN_COLUMN]))
                    break;

                enable_idx = i;
            }

            if (enable_idx == -1)
    			_change_obj_focus(obj, g_set_obj);
            else
    			_change_obj_focus(obj, g_list[enable_idx][RUN_COLUMN]);

			return TRUE;
		}
	}

	return FALSE;
}

static gboolean post_onoff_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_RADIO_GET_FOCUS)
	{
		gint index;
		gint mode;

		index = nfui_radio_button_get_index((NFBUTTON*)obj);

        if (index == 0)
        {
			if (g_mode_obj) mode = nfui_combobox_get_cur_index(NF_COMBOBOX(g_mode_obj));
			else mode = 1;

            if (mode == 0)  g_ptzData->mode = PTZ_MODE_SCAN;
            else            g_ptzData->mode = PTZ_MODE_TOUR;
        }
        else
            g_ptzData->mode = PTZ_MODE_OFF;

        _live_ptz_main_cmd_pattern(g_ch);
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
        gint i, enable_idx = -1;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if (kpid == KEYPAD_UP)
		{
            for (i = 0; i < ROW_COUNT; i++)
            {
                if (nfui_nfobject_is_disabled(g_list[i][RUN_COLUMN]))
                    break;

                enable_idx = i;
            }

            if (enable_idx == -1)
    			_change_obj_focus(obj, g_set_obj);
            else
    			_change_obj_focus(obj, g_list[enable_idx][RUN_COLUMN]);

			return TRUE;
		}
	}

	return FALSE;
}

static gboolean _post_stg_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		gint mode;
		gint retVal;

		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		if (g_mode_obj) mode = nfui_combobox_get_cur_index(NF_COMBOBOX(g_mode_obj));
		else mode = 1;

        if (mode == 0)
            retVal = VW_PtzCtrl_Scan_Open(g_curwnd, g_presetData, g_scanData);
        else
            retVal = VW_PtzCtrl_Tour_Open(g_curwnd, g_presetData, g_tourData);

        if (retVal) _live_ptz_main_cmd_pattern(g_ch);
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
        gint i, enable_idx = -1;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if (kpid == KEYPAD_UP)
		{
            for (i = 0; i < ROW_COUNT; i++)
            {
                if (nfui_nfobject_is_disabled(g_list[i][DEL_COLUMN]))
                    break;

                enable_idx = i;
            }

            if (enable_idx == -1)
    			_change_obj_focus(obj, g_set_obj);
            else
    			_change_obj_focus(obj, g_list[enable_idx][DEL_COLUMN]);

			return TRUE;
		}
	}

	return FALSE;
}

static gboolean _change_channel_delay(gpointer data)
{
	gint ch = GPOINTER_TO_INT(data);
	_live_ptz_main_change_channel(ch, 1);
	return FALSE;
}

static gboolean _post_channel_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint ch;
	gint i, j;

	if (evt->type == NFEVENT_COMBOBOX_CHANGED)
	{
        ch = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));
//        if (g_ch != ch) _live_ptz_main_change_channel(ch, 1);
        if (g_ch != ch) g_timeout_add(150, _change_channel_delay, GINT_TO_POINTER(ch));
	}
	else if(evt->type == INFY_CAM_TITLE_NOTIFY)
	{
		ch = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

		nfui_combobox_remove_all((NFCOMBOBOX*)obj);

		for (i = 0; i<GUI_CHANNEL_CNT; i++) {
			j = sprintf(strCh[i], "CH%d - ", i+1);

			//DAL_get_camera_title(strCh[i]+j, i);
			var_get_camtitle(strCh[i]+j, i);
			nfui_combobox_append_data((NFCOMBOBOX*)obj, strCh[i]);
		}
		nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, (guint)ch);
		nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
	}
	else if(evt->type == GDK_DELETE)
	{
		uxm_unreg_imsg_event(obj, INFY_CAM_TITLE_NOTIFY);
	}

	return FALSE;
}

static gboolean _post_prst_fixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
    gint i;

    if (event->type == GDK_DELETE)
    {
        _free_preset_list();

		for(i=0; i<GUI_CHANNEL_CNT; i++)
			ifree(strCh[i]);

		radioList = NULL;
    }

	return FALSE;
}

gint _live_ptz_prst_open(NFOBJECT *parent, NFOBJECT *prst_fixed)
{
	NFOBJECT *tbl = NULL;
	NFOBJECT *obj = NULL;

	guint tbl_w[COL_COUNT] = {PRST_NO_INPUT_W, PRST_NAME_INPUT_W, 42, 42};
	gint i, j;
    guint pos_x, pos_y;
    guint size_w, size_h;

	GdkPixbuf *spin_up[NFOBJECT_STATE_COUNT];
	GdkPixbuf *spin_down[NFOBJECT_STATE_COUNT];
	GdkPixbuf *goto_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *sync_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *del_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *on_img[4];
	GdkPixbuf *off_img[4];
	GdkPixbuf *stg_img[4];

    GdkPixbuf *arrow_img;
	gchar *mode[2] =  {"SCAN", "TOUR"};

	const guint font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(210), COLOR_IDX(211), COLOR_IDX(213), COLOR_IDX(214)};

    g_curwnd = parent;

	spin_up[0] = nfui_get_image_from_file((IMG_N_POPUP_SPIN_UP), NULL);
	spin_up[1] = nfui_get_image_from_file((IMG_O_POPUP_SPIN_UP), NULL);
	spin_up[2] = nfui_get_image_from_file((IMG_P_POPUP_SPIN_UP), NULL);
	spin_up[3] = nfui_get_image_from_file((IMG_D_POPUP_SPIN_UP), NULL);

	spin_down[0] = nfui_get_image_from_file((IMG_N_POPUP_SPIN_DOWN), NULL);
	spin_down[1] = nfui_get_image_from_file((IMG_O_POPUP_SPIN_DOWN), NULL);
	spin_down[2] = nfui_get_image_from_file((IMG_P_POPUP_SPIN_DOWN), NULL);
	spin_down[3] = nfui_get_image_from_file((IMG_D_POPUP_SPIN_DOWN), NULL);

	sync_img[0] = nfui_get_image_from_file(("bt_pop_sync_n.png"), NULL);
	sync_img[1] = nfui_get_image_from_file(("bt_pop_sync_o.png"), NULL);
	sync_img[2] = nfui_get_image_from_file(("bt_pop_sync_p.png"), NULL);
	sync_img[3] = nfui_get_image_from_file(("bt_pop_sync_d.png"), NULL);

	goto_img[0] = nfui_get_image_from_file((IMG_BT_POP_GOTO_N), NULL);
	goto_img[1] = nfui_get_image_from_file((IMG_BT_POP_GOTO_O), NULL);
	goto_img[2] = nfui_get_image_from_file((IMG_BT_POP_GOTO_P), NULL);
	goto_img[3] = nfui_get_image_from_file((IMG_BT_POP_GOTO_D), NULL);

	del_img[0] = nfui_get_image_from_file((IMG_BT_POP_DEL_N), NULL);
	del_img[1] = nfui_get_image_from_file((IMG_BT_POP_DEL_O), NULL);
	del_img[2] = nfui_get_image_from_file((IMG_BT_POP_DEL_P), NULL);
	del_img[3] = nfui_get_image_from_file((IMG_BT_POP_DEL_D), NULL);

	on_img[0] = nfui_get_image_from_file(IMG_N_PTZ_TOUR_ON, NULL);
	on_img[1] = nfui_get_image_from_file(IMG_O_PTZ_TOUR_ON, NULL);
	on_img[2] = nfui_get_image_from_file(IMG_S_PTZ_TOUR_ON, NULL);
	on_img[3] = nfui_get_image_from_file(IMG_D_PTZ_TOUR_ON, NULL);

	off_img[0] = nfui_get_image_from_file(IMG_N_PTZ_TOUR_OFF, NULL);
	off_img[1] = nfui_get_image_from_file(IMG_O_PTZ_TOUR_OFF, NULL);
	off_img[2] = nfui_get_image_from_file(IMG_S_PTZ_TOUR_OFF, NULL);
	off_img[3] = nfui_get_image_from_file(IMG_D_PTZ_TOUR_OFF, NULL);

	stg_img[0] = nfui_get_image_from_file(IMG_BT_POP_SETTING_N, NULL);
	stg_img[1] = nfui_get_image_from_file(IMG_BT_POP_SETTING_O, NULL);
	stg_img[2] = nfui_get_image_from_file(IMG_BT_POP_SETTING_P, NULL);
	stg_img[3] = nfui_get_image_from_file(IMG_BT_POP_SETTING_D, NULL);

	for (i = 0; i<GUI_CHANNEL_CNT; i++)
	{
		strCh[i] = imalloc(sizeof(gchar) * (STRING_SIZE_CAMTITLE+8));
		j = sprintf(strCh[i], "CH%d - ", i+1);
		//DAL_get_camera_title(strCh[i]+j, i);
		var_get_camtitle(strCh[i]+j, i);
	}

    pos_x = PRST_CAM_LABEL_X;
    pos_y = PRST_CAM_LABEL_Y;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CH", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(obj, PRST_CAM_LABEL_W, PRST_LABEL_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)prst_fixed, obj, pos_x, pos_y);

    pos_x += PRST_CAM_LABEL_W;

	obj = nfui_combobox_new(strCh, GUI_CHANNEL_CNT, 0);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
	nfui_nfobject_set_size(obj, PRST_CAM_COMBO_W, PRST_LABEL_H);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)prst_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, _post_channel_combo_event_handler);
    g_ch_obj = obj;

	uxm_reg_imsg_event(obj, INFY_CAM_TITLE_NOTIFY);
	uxm_monitor_on_imsg_event(obj, INFY_CAM_TITLE_NOTIFY);

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), sync_img);
	nfui_nfobject_disable(obj);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)prst_fixed, obj, pos_x+PRST_CAM_COMBO_W+1, pos_y);
	nfui_regi_post_event_callback(obj, _post_sync_button_event_handler);
	g_sync_obj = obj;

    pos_x = PRST_CAM_LABEL_X;
    pos_y += (PRST_LABEL_H + 18);

	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG3);
	nfui_nfimage_set_text((NFIMAGE*)obj, "PRESET");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)prst_fixed, obj, pos_x, pos_y);

    pos_x = PRST_NO_LABEL_X;
    pos_y += (PRST_LABEL_H + 4);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("NO.", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(obj, PRST_NO_LABEL_W, 34);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)prst_fixed, obj, pos_x, pos_y);

    pos_x += (PRST_NO_LABEL_W + 5);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("NAME", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(obj, PRST_NAME_LABEL_W, 34);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)prst_fixed, obj, pos_x, pos_y);

    pos_x = PRST_NO_LABEL_X;
    pos_y += 34;

	nfui_get_pixbuf_size(spin_up[0], &size_w, &size_h);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_SMALL_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nflabel_use_number((NFLABEL*)obj, TRUE);
	nfui_nflabel_set_number((NFLABEL*)obj, 1);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, PRST_NO_LABEL_W-size_w, PRST_LABEL_H);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)prst_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, _post_no_event_handler);
    g_number_obj = obj;

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), spin_up);
	nfui_nfobject_set_size(obj, size_w, size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)prst_fixed, obj, pos_x+PRST_NO_LABEL_W-size_w, pos_y);
	nfui_regi_post_event_callback(obj, _post_no_up_event_handler);
	g_numberup_obj = obj;

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), spin_down);
	nfui_nfobject_set_size(obj, size_w, size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)prst_fixed, obj, pos_x+PRST_NO_LABEL_W-size_w, pos_y+size_h);
	nfui_regi_post_event_callback(obj, _post_no_down_event_handler);
	g_numberdown_obj = obj;

    pos_x += (PRST_NO_LABEL_W + 5);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_SMALL_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, PRST_NAME_LABEL_W, PRST_LABEL_H);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)prst_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, _post_name_event_handler);
    g_name_obj = obj;

    pos_x += (PRST_NAME_LABEL_W + 1);

   	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), goto_img);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)prst_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, _post_dir_run_btn_event_handler);
    g_run_obj = obj;

    pos_x = PRST_NO_LABEL_X;
    pos_y += (PRST_LABEL_H + 5);

	obj = nftool_normal_button_create_popup_type1("SET", 358/2-1);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)prst_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, _post_set_btn_event_handler);
    g_set_obj = obj;

	obj = nftool_normal_button_create_popup_type1("FAVORITE", 358/2-1);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)prst_fixed, obj, pos_x+358/2+2, pos_y);
	nfui_regi_post_event_callback(obj, _post_bookmark_btn_event_handler);
    g_bookmark_obj = obj;

    pos_y += (PRST_LABEL_H);

    nfui_get_image_size(IMG_PTZ_PRESET_ARROW, &size_w, &size_h);

	obj = nfui_nfimage_new(IMG_PTZ_PRESET_ARROW);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)prst_fixed, obj, pos_x+(tbl_w[0]+tbl_w[1]+tbl_w[2]+tbl_w[3]+3-size_w)/2, pos_y+(25-size_h)/2);

    pos_y += 25;

	tbl = (NFOBJECT*)nfui_nftable_new(COL_COUNT, ROW_COUNT+1, 1, 1, tbl_w, PRST_LABEL_H);
	nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(tbl);
	nfui_nffixed_put((NFFIXED*)prst_fixed, tbl, pos_x, pos_y);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("NO.", nffont_get_pango_font(NFFONT_SMALL_SEMI_1), COLOR_IDX(116));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, NUM_COLUMN, 0);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("NAME", nffont_get_pango_font(NFFONT_SMALL_SEMI_1), COLOR_IDX(116));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)tbl, obj, NAME_COLUMN, 0);

    for (i = 0; i < ROW_COUNT; i++)
    {
    	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(231));
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
    	nfui_nftable_attach((NFTABLE*)tbl, obj, NUM_COLUMN, i+1);
    	nfui_nfobject_show(obj);
    	g_list[i][NUM_COLUMN] = obj;

    	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(231));
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 2);
    	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
    	nfui_nftable_attach((NFTABLE*)tbl, obj, NAME_COLUMN, i+1);
    	nfui_nfobject_show(obj);
    	g_list[i][NAME_COLUMN] = obj;

       	obj = (NFOBJECT*)nfui_nfbutton_new();
    	nfui_nfbutton_set_image(NF_BUTTON(obj), goto_img);
    	nfui_nfobject_disable(obj);
    	nfui_nfobject_show(obj);
    	nfui_nftable_attach((NFTABLE*)tbl, obj, RUN_COLUMN, i+1);
    	nfui_regi_post_event_callback(obj, _post_run_button_event_handler);
    	g_list[i][RUN_COLUMN] = obj;

       	obj = (NFOBJECT*)nfui_nfbutton_new();
    	nfui_nfbutton_set_image(NF_BUTTON(obj), del_img);
    	nfui_nfobject_disable(obj);
    	nfui_nfobject_show(obj);
    	nfui_nftable_attach((NFTABLE*)tbl, obj, DEL_COLUMN, i+1);
    	nfui_regi_post_event_callback(obj, _post_del_button_event_handler);
    	g_list[i][DEL_COLUMN] = obj;
    }


    pos_x = PRST_CAM_LABEL_X;
    pos_y += (PRST_LABEL_H*(ROW_COUNT+1) + 1*ROW_COUNT + 25);

	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG3);
    if (strcmp(nf_sysdb_get_str_nocopy("sys.info.vendor"), "S1") != 0)
	nfui_nfimage_set_text((NFIMAGE*)obj, "SCAN / TOUR");
	else
		nfui_nfimage_set_text((NFIMAGE*)obj, "TOUR");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)prst_fixed, obj, pos_x, pos_y);

    pos_x = PRST_NO_LABEL_X;
    pos_y += (PRST_LABEL_H + 8);

    if (strcmp(nf_sysdb_get_str_nocopy("sys.info.vendor"), "S1") != 0)
    {
	obj = nfui_combobox_new(mode, 2, 0);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, PRST_TOUR_COMBO_W, PRST_LABEL_H);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)prst_fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_mode_event_handler);
    g_mode_obj = obj;
    }
    else
    {
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("TOUR", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 3);
		nfui_nfobject_set_size(obj, PRST_TOUR_COMBO_W, PRST_LABEL_H);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)prst_fixed, obj, pos_x, pos_y);
    }

    pos_x += (PRST_TOUR_COMBO_W + 3);

	nfui_get_pixbuf_size(on_img[0], &size_w, &size_h);

	obj = (NFOBJECT*)nfui_nfbutton_new_with_param(on_img, "ON");
	nfui_nfbutton_set_pango_font((NFBUTTON*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), (guint*)font_color);
	nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)prst_fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_onoff_event_handler);
    radioList = nfui_radio_button_get_group(NF_BUTTON(obj));
    g_onoff_obj[0] = obj;

    pos_x += (size_w);

	nfui_get_pixbuf_size(off_img[0], &size_w, &size_h);

	obj = (NFOBJECT*)nfui_nfbutton_new_with_param(off_img, "OFF");
	nfui_nfbutton_set_pango_font((NFBUTTON*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), (guint*)font_color);
	nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)prst_fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_onoff_event_handler);
    nfui_radio_button_add_group(NF_BUTTON(obj), radioList);
    g_onoff_obj[1] = obj;

    pos_x += (size_w+2);

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), stg_img);
	nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)prst_fixed, obj, pos_x, pos_y);
   	nfui_regi_post_event_callback(obj, _post_stg_button_event_handler);
    g_stg_obj = obj;

	nfui_regi_post_event_callback(prst_fixed, _post_prst_fixed_event_handler);

    return 0;
}

gint _live_ptz_prst_set_data(PtzData *ptz_data, PtzPresetData *preset_data, PtzScanData *scan_data, PtzTourData *tour_data)
{
    g_ptzData = ptz_data;
    g_presetData = preset_data;
    g_scanData = scan_data;
    g_tourData = tour_data;

    return 0;
}

gint _live_ptz_prst_set_evt_data(EA_AlmSenData *almsen_data, EA_MotSenData *motsen_data, EA_VLossData *vloss_data, EA_PosData *pos_data)
{
    g_asd = almsen_data;
    g_msd = motsen_data;
    g_vld = vloss_data;
    g_psd = pos_data;

    return 0;
}

gint _live_ptz_prst_init_channel(guint channel)
{
    gchar strBuf[PRESET_NAME_LEN+1];

    nfui_combobox_set_index_no_expose(NF_COMBOBOX(g_ch_obj), channel);

    _new_preset_list(channel);
    _display_preset_list();

    _get_preset_name(1, strBuf);
    nfui_nflabel_set_text((NFLABEL*)g_name_obj, strBuf);

	_init_supported_func(channel);
    _init_ptzMode(g_ptzData->mode);

    g_ch = channel;

    return 0;
}

gint _live_ptz_prst_change_channel(guint channel, gint expose)
{
    gint i;
    gchar strBuf[PRESET_NAME_LEN+1];

    _free_preset_list();

    nfui_combobox_set_index_no_expose(NF_COMBOBOX(g_ch_obj), channel);

    _new_preset_list(channel);
    _display_preset_list();

    nfui_nflabel_set_number((NFLABEL*)g_number_obj, 1);

    _get_preset_name(1, strBuf);
    nfui_nflabel_set_text((NFLABEL*)g_name_obj, strBuf);

	_init_supported_func(channel);
    _init_ptzMode(g_ptzData->mode);

    if (expose)
    {
	    nfui_signal_emit(g_ch_obj, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_sync_obj, GDK_EXPOSE, TRUE);
	    nfui_signal_emit(g_number_obj, GDK_EXPOSE, TRUE);
	    nfui_signal_emit(g_numberup_obj, GDK_EXPOSE, TRUE);
	    nfui_signal_emit(g_numberdown_obj, GDK_EXPOSE, TRUE);
	    nfui_signal_emit(g_name_obj, GDK_EXPOSE, TRUE);

	    for (i = 0; i < ROW_COUNT; i++)
	    {
	        nfui_signal_emit(g_list[i][NUM_COLUMN], GDK_EXPOSE, TRUE);
	        nfui_signal_emit(g_list[i][NAME_COLUMN], GDK_EXPOSE, TRUE);
	        nfui_signal_emit(g_list[i][RUN_COLUMN], GDK_EXPOSE, TRUE);
	        nfui_signal_emit(g_list[i][DEL_COLUMN], GDK_EXPOSE, TRUE);
	    }

	    nfui_signal_emit(g_run_obj, GDK_EXPOSE, TRUE);
	    nfui_signal_emit(g_set_obj, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_bookmark_obj, GDK_EXPOSE, TRUE);
	    if (g_mode_obj) nfui_signal_emit(g_mode_obj, GDK_EXPOSE, TRUE);
	    nfui_signal_emit(g_onoff_obj[0], GDK_EXPOSE, TRUE);
	    nfui_signal_emit(g_onoff_obj[1], GDK_EXPOSE, TRUE);
	    nfui_signal_emit(g_stg_obj, GDK_EXPOSE, TRUE);
    }

    g_ch = channel;

    return 0;
}


