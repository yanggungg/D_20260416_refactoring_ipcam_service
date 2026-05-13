#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nftimespin.h"
#include "objects/nfcheckbutton.h"
#include "objects/nflistbox.h"
#include "objects/nfbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nftimelabel.h"

#include "vw_search_by_event.h"
#include "vw_search_main.h"

#include "vw_live_statusbar.h"
#include "vw_set_date_time.h"
#include "vw_internal.h"
#include "ix_mem.h"
#include "dtf.h"
#include "stm.h"
#include "var.h"
#include "scm.h"

// FIXED1 , FIXED2

#define SBE_FIXED1_X					(0)
#define SBE_FIXED1_Y					(0)
#define SBE_FIXED1_W					(455)
#define SBE_FIXED1_H					(903)

#define SBE_FIXED2_X					(SBE_FIXED1_X+SBE_FIXED1_W)
#define SBE_FIXED2_Y					(SBE_FIXED1_Y)
#define SBE_FIXED2_W					(1401)
#define SBE_FIXED2_H					(SBE_FIXED1_H)

#define	SBE_LABEL_HEIGHT1				(40)
#define	SBE_LABEL_HEIGHT2				(26)
#define	SBE_LABEL_HEIGHT3				(27)

#define	SBE_LOGLABEL_HEIGHT				(38)

// FROM / TO.
#define	SBE_TABLE1_X					(15)
#define	SBE_TABLE1_Y					(34)
#define	SBE_TABLE1_COLUMNS				(3)
#define	SBE_TABLE1_ROWS					(2)

// VIDEO CHANNEL
#define SBE_VDO_CH_X					(SBE_TABLE1_X)
#define SBE_VDO_CH_Y					(126)
#define SBE_VDO_CH_WIDTH1				(180)
#define SBE_VDO_CH_WIDTH2				(230)

// EVENT
#define SBE_EVENT_TITLE_LABEL_X			(SBE_VDO_CH_X)
#define SBE_EVENT_TITLE_LABEL_Y			(SBE_VDO_CH_Y+SBE_LABEL_HEIGHT1+19)

#define	SBE_EVENT_OPTFIXED_X			(11)
#define	SBE_EVENT_OPTFIXED_Y			(SBE_EVENT_TITLE_LABEL_Y+SBE_LABEL_HEIGHT2+3)
#define	SBE_EVENT_OPTFIXED_W			(417)
#define	SBE_EVENT_OPTFIXED_H			(310)

#define	SBE_EVENT_LABEL_WIDTH1			(185)
#define	SBE_EVENT_LABEL_WIDTH2			(185)
#define	SBE_EVENT_LABEL_WIDTH3			(185)

// SEARCH BUTTON
#define	SBE_SEARCH_BUTTON_X				(SBE_EVENT_OPTFIXED_X + 209)
#define	SBE_SEARCH_BUTTON_Y				(SBE_EVENT_OPTFIXED_Y + SBE_EVENT_OPTFIXED_H + 10)

// PREVIEW
#define SBE_PREVIEW_LABEL_X				(16)
#define SBE_PREVIEW_LABEL_Y				(SBE_SEARCH_BUTTON_Y+82)
#define SBE_PREVIEW_LABEL_WIDTH			(220)

#define SBE_PREVIEW_VIDEO_X				(SBE_PREVIEW_LABEL_X)
#define SBE_PREVIEW_VIDEO_Y				(SBE_PREVIEW_LABEL_Y+SBE_LABEL_HEIGHT2+1)
#define SBE_PREVIEW_VIDEO_W				(408) //410
#define SBE_PREVIEW_VIDEO_H				(228) //230

#define SBE_PREVIEW_TIME_X				(SBE_PREVIEW_VIDEO_X)
#define SBE_PREVIEW_TIME_Y				(SBE_PREVIEW_VIDEO_Y+SBE_PREVIEW_VIDEO_H+4)

// EVENT LOG
#define SBE_LOG_LABEL_ROWS				(19)
#define SBE_LOG_HEIGHT_GAP				(4)
#define SBE_LOG_WIDTH_GAP				(2)
#define	SBE_LOG_LEFT_MARGIN				(10)

#define SBE_LOG_X						(0)
#define SBE_LOG_Y						(0)

#define SBE_LOG_TYPE_X					(SBE_LOG_X)
#define SBE_LOG_TYPE_W					(239)

#define SBE_LOG_TIME_X					(SBE_LOG_X + SBE_LOG_TYPE_W+SBE_LOG_WIDTH_GAP)
#define SBE_LOG_TIME_W					(279)

#define SBE_LOG_CONTENTS_X				(SBE_LOG_TIME_X + SBE_LOG_TIME_W+SBE_LOG_WIDTH_GAP)
#define SBE_LOG_CONTENTS_W				(878)

// LOG ORDER
#define SBE_LOG_ORDER_X					(SBE_LOG_X)
#define SBE_LOG_ORDER_Y					(SBE_LOG_Y + (SBE_LOGLABEL_HEIGHT+SBE_LOG_HEIGHT_GAP)*(SBE_LOG_LABEL_ROWS+1) + 6)
#define SBE_LOG_ORDER_W					(180)
#define SBE_LOG_ORDER_CMB_W				(260)

// LOG DIR
#define SBE_LOG_BUTTON_SIZE_W			(84)
#define SBE_LOG_TEXT_SIZE_W				(120)

#define	SBE_LOG_PREV_BUTTON_X			(SBE_LOG_ORDER_X + 556)
#define	SBE_LOG_PREV_BUTTON_Y			(SBE_LOG_ORDER_Y)

#define	SBE_LOG_NEXT_BUTTON_X			(SBE_LOG_PREV_BUTTON_X + SBE_LOG_BUTTON_SIZE_W + SBE_LOG_TEXT_SIZE_W)
#define	SBE_LOG_NEXT_BUTTON_Y			(SBE_LOG_ORDER_Y)

enum {
	DF_YMD = 0,
	DF_MDY,
	DF_DMY,
	NUM_DATE_FORMATS,
};

enum {
	OPT_EVT_ALL 		= 0,
	OPT_EVT_VCA,
	OPT_EVT_ALARM,
	OPT_EVT_MOTION,
	OPT_EVT_VIDEO,
	OPT_EVT_RECORD,
	OPT_EVT_SYSTEM,
	OPT_EVT_STORAGE,
	OPT_EVT_NETWORK,
#if defined(_IPX_MODEL_UX)
	OPT_EVT_IPCAM,
#endif
#if defined(_SUPPORT_TAMPER_SETUP)
	OPT_EVT_TAMPER,
#endif
	OPT_EVT_SETUP,
	OPT_EVT_POS,
	OPT_EVT_MAX
};

typedef enum {
	LOGLABEL_TYPE = 0,
	LOGLABEL_TIME,
	LOGLABEL_CONTENTS,
	LOGLABEL_ALL
} LOGLABEL_E;


#define NORMAL_OUTLINE_COLOR		0
#define FOCUS_OUTLINE_COLOR			146
#define SELECT_OUTLINE_COLOR		147



static NFWINDOW *g_curwnd = 0;
static NFOBJECT *from_obj;
static NFOBJECT *to_obj;
static NFOBJECT *opt_obj[OPT_EVT_MAX];
static NFOBJECT *opt_debug_obj = NULL;
static NFOBJECT *search_obj;
static NFOBJECT *order_obj;
static NFOBJECT *prev_obj;
static NFOBJECT *next_obj;
static NFOBJECT *playback_obj;
static NFOBJECT *log_label[SBE_LOG_LABEL_ROWS][LOGLABEL_ALL];
static NFOBJECT *ch_obj;
static NFOBJECT *from_btn_obj;
static NFOBJECT *to_btn_obj;

static NFOBJECT *preview_obj = NULL;
static NFOBJECT *play_st_obj = NULL;
static NFOBJECT *play_time_obj = NULL;

static guint preview_timer_id;

static gint focused_row = -1;
static gint selected_row = -1;
static GTimeVal	g_log_time[SBE_LOG_LABEL_ROWS];

static gboolean g_init_sbe = TRUE;

static NFOBJECT *fixed1;


static nftl_df_type prvTransDateFormat(gint db_index)
{
	nftl_df_type ret;

	if(db_index == DF_YMD)			ret = NFTL_DF_YMD;
	else if(db_index == DF_MDY)		ret = NFTL_DF_MDY;
	else if(db_index == DF_DMY)		ret = NFTL_DF_DMY;
	else	ret = NFTL_DF_HIDE;

	return ret;
}

static void _dir_prev_button_enable()
{
	nfui_nfobject_enable(prev_obj);
	nfui_signal_emit(prev_obj, GDK_EXPOSE, FALSE);
}

static void _dir_prev_button_disable()
{
	nfui_nfobject_disable(prev_obj);
	nfui_signal_emit(prev_obj, GDK_EXPOSE, FALSE);
}

static void _dir_next_button_enable()
{
	nfui_nfobject_enable(next_obj);
	nfui_signal_emit(next_obj, GDK_EXPOSE, FALSE);
}

static void _dir_next_button_disable()
{
	nfui_nfobject_disable(next_obj);
	nfui_signal_emit(next_obj, GDK_EXPOSE, FALSE);
}

static void _playback_button_enable()
{
	nfui_nfobject_enable(playback_obj);
	nfui_signal_emit(playback_obj, GDK_EXPOSE, FALSE);
}

static void _playback_button_disable()
{
	nfui_nfobject_disable(playback_obj);
	nfui_signal_emit(playback_obj, GDK_EXPOSE, FALSE);
}

static void _order_button_enable()
{
	nfui_nfobject_enable(order_obj);
	nfui_signal_emit(order_obj, GDK_EXPOSE, TRUE);
}

static void _order_button_disable()
{
	nfui_nfobject_disable(order_obj);
	nfui_signal_emit(order_obj, GDK_EXPOSE, TRUE);
}

static void _preview_obj_on(gchar *buf)
{
	nfui_nflabel_set_text((NFLABEL*)preview_obj, buf);
	nfui_nfobject_modify_bg(preview_obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
	nfui_signal_emit(preview_obj, GDK_EXPOSE, FALSE);
}

static void _preview_obj_off(gchar *buf)
{
	nfui_nflabel_set_text((NFLABEL*)preview_obj, buf);
	nfui_nfobject_modify_bg(preview_obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(666));
	nfui_signal_emit(preview_obj, GDK_EXPOSE, FALSE);
}

static void _change_outLine_LogLabel(gint row, gint color)
{
	static GdkGC *gc;
	static GdkDrawable *drawable;
	gint x, y;

	drawable = nfui_nfobject_get_window((NFOBJECT*)log_label[row][LOGLABEL_TYPE]);
	gc = nfui_nfobject_get_gc(log_label[row][LOGLABEL_TYPE]);

	gdk_gc_set_line_attributes(gc,
			2,
			GDK_LINE_SOLID,
			GDK_CAP_NOT_LAST,
			GDK_JOIN_MITER);

	nfui_nfobject_get_offset(log_label[row][LOGLABEL_TYPE], &x, &y);
	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(color));
	gdk_draw_rectangle (drawable, gc, FALSE, x-1, y-1,
			SBE_LOG_TYPE_W+2+SBE_LOG_TIME_W+2+SBE_LOG_CONTENTS_W+2, SBE_LOGLABEL_HEIGHT+2);

	nfui_nfobject_gc_unref(gc);
}

static gint _draw_selectLine(gint row)
{
	if (selected_row == row)
		return -1;

	if (selected_row != -1)
		_change_outLine_LogLabel(selected_row, NORMAL_OUTLINE_COLOR);

	_change_outLine_LogLabel(row, SELECT_OUTLINE_COLOR);
	selected_row = row;

	return 0;
}

static gint _erase_selectLine()
{
	if (selected_row == -1)
		return -1;

	_change_outLine_LogLabel(selected_row, NORMAL_OUTLINE_COLOR);
	selected_row = -1;

	return 0;
}

static gint _draw_focusLine(gint row)
{
	if (focused_row == row)
		return -1;

// leave old focus row
	if ((selected_row != -1) && (selected_row == focused_row))
		_change_outLine_LogLabel(selected_row, SELECT_OUTLINE_COLOR);
	else if (focused_row != -1)
		_change_outLine_LogLabel(focused_row, NORMAL_OUTLINE_COLOR);

// enter new focus row
	_change_outLine_LogLabel(row, FOCUS_OUTLINE_COLOR);
	focused_row = row;

	return 0;
}

static gint _erase_focusLine(gint row)
{
	if (row == selected_row)
		_change_outLine_LogLabel(row, SELECT_OUTLINE_COLOR);
	else
		_change_outLine_LogLabel(row, NORMAL_OUTLINE_COLOR);

	focused_row = -1;

	return 0;
}

static void _set_data_LogLabel(gint index, LOG_DATA_T ld)
{
	guint ch_mask = 0;
	LPR_CHANNEL_T *pc = (LPR_CHANNEL_T *)&ld.p;

	if ((ld.type == LT_MOTION_DETECTION) || (ld.type == LT_TAMPER)
		|| (ld.type == LT_VIDEO_IN) || (ld.type == LT_VIDEO_LOSS)
		|| (ld.type == LT_SYSTEM_POS) || (ld.type == LT_VCA)
		|| (ld.type == LT_RECORD_STARTED) || (ld.type == LT_RECORD_STOPPED))
	{
		ch_mask |= (1 << pc->channel);
	}
	else
	{
		ch_mask = 0xffff;
	}

	g_log_time[index].tv_sec = ld.tvTime.tv_sec - 3;
	nfui_nfobject_set_data(log_label[index][LOGLABEL_CONTENTS], "log chmask", GUINT_TO_POINTER(ch_mask));
}

static gboolean _check_klass_boundary(int type, int klass)
{
	if (g_klass_bound[type][0] == -1 && g_klass_bound[type][1] == -1) return TRUE;
	if (g_klass_bound[type][0] <= klass && klass <= g_klass_bound[type][1]) return TRUE;
	printf("INVALID LOG PARAMETER, (type:%d), (param2:%d)\n", type, klass);
	return FALSE;
}

static void _trans_logData_into_text(LOG_DATA_T ld, gchar *type, gchar *contents)
{
	LPR_KLASS_T *pk = (LPR_KLASS_T*)&ld.p;
	if (!_check_klass_boundary(ld.type, pk->klass)) return;

	switch (ld.type) {
	case LT_SYSTEM_STARTED:
	case LT_SYSTEM_SHUTDOWN:
		sprintf(type, "SYSTEM");
		sprintf(contents, g_system_logstr[ld.type]);
		break;

	case LT_ABNORMAL_SHUTDOWN_DETECTED:
		sprintf(type, "SYSTEM");
		sprintf(contents, g_reboot_logstr[ld.p.cat_reboot.klass]);
		break;

	case LT_SYSTEM_RECOVERED:
		sprintf(type, "SYSTEM");
		sprintf(contents, g_system_logstr[ld.type]);
		break;

	case LT_SYSTEM_TIME_CHANGED:
	case LT_SYSTEM_FORMAT:
		sprintf(type, "SYSTEM");
		sprintf(contents, g_system_logstr[ld.type], ld.p.cat_system.userid);
		break;

	case LT_SYSTEM_SYSLOG:
		sprintf(type, "SYSTEM");
		switch (ld.p.cat_syslog.klass) {
		case 0: case 9: case 10: case 11: case 15: case 16: case 17: case 18: case 19: case 20: case 21: case 22:
			sprintf(contents, g_syslog_logstr[ld.p.cat_syslog.klass], ld.p.cat_syslog.userid);
			break;
		case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 12:
		case 13: case 14:
			sprintf(contents, g_syslog_logstr[ld.p.cat_syslog.klass]);
			break;
		}
		break;

	case LT_SYSTEM_FW_UPGRADE:
		sprintf(type, "SYSTEM");
		switch (ld.p.cat_fwup.klass) {
		case 0:
		case 1:
			sprintf(contents, g_fwup_logstr[ld.p.cat_fwup.klass], ld.p.cat_fwup.userid);
			break;
		default:
			sprintf(contents, g_fwup_logstr[ld.p.cat_fwup.klass]);
			break;
		}
		break;

	case LT_LOCAL_LOG_ON:
		sprintf(type, g_logonoff_logstr[ld.p.cat_logon.klass]);
		sprintf(contents, g_logon_logstr[0], ld.p.cat_logon.userid);
		break;

	case LT_LOCAL_LOG_OFF:
		sprintf(type, g_logonoff_logstr[ld.p.cat_logoff.klass]);
		sprintf(contents, g_logon_logstr[1], ld.p.cat_logoff.userid);
		break;

	case LT_REMOTE_LOG_ON:
		sprintf(type, g_logonoff_logstr[ld.p.cat_rlogon.klass]);
		sprintf(contents, g_logon_logstr[2], ld.p.cat_rlogon.userid, ld.p.cat_rlogon.ipaddr);
		break;

	case LT_REMOTE_LOG_OFF:
		sprintf(type, g_logonoff_logstr[ld.p.cat_rlogoff.klass]);
		sprintf(contents, g_logon_logstr[3], ld.p.cat_rlogoff.userid, ld.p.cat_rlogon.ipaddr);
		break;

	case LT_RECORD_SETUP_CHANGED:
		sprintf(type, "RECORD SETUP");
		sprintf(contents, g_recsetup_logstr[ld.p.cat_recsetup.klass], ld.p.cat_recsetup.userid);
		break;

	case LT_SYSTEM_SETUP_CHANGED:
		sprintf(type, "SYSTEM SETUP");
		sprintf(contents, g_syssetup_logstr[ld.p.cat_syssetup.klass], ld.p.cat_syssetup.userid);
		break;

	case LT_SENSOR_INPUT:
		sprintf(type, "ALARM");
		sprintf(contents, g_alarmevt_logstr[ld.p.cat_sensor.klass], ld.p.cat_sensor.channel + 1);
		break;

	case LT_MOTION_DETECTION:
		sprintf(type, "MOTION");
		sprintf(contents, g_motionevt_logstr[ld.p.cat_motion.klass], ld.p.cat_motion.channel + 1);
		break;

	case LT_VIDEO_IN:
		sprintf(type, "VIDEO");
		sprintf(contents, g_vlossevt_logstr[ld.p.cat_videoloss.klass], ld.p.cat_videoloss.channel + 1);
		break;

	case LT_VIDEO_LOSS:
		sprintf(type, "VIDEO");
		sprintf(contents, g_vlossevt_logstr[ld.p.cat_videoloss.klass], ld.p.cat_videoloss.channel + 1);
		break;

	case LT_TAMPER:
		sprintf(type, "TAMPER");
		sprintf(contents, g_tamper_logstr[ld.p.cat_motion.klass], ld.p.cat_tamper.channel + 1);
		break;

	case LT_SMART_WARNING:
		sprintf(type, "STORAGE");
//		sprintf(contents, g_smart_logstr[0], ld.p.cat_smart.diskid);
		sprintf(contents, g_smart_logstr[0], g_location_logstr[ld.p.cat_smart.klass]);
		break;

	case LT_DISK_EVENT:
		if (ld.p.cat_disk.location > 1) return;
		sprintf(type, "STORAGE");
		switch (ld.p.cat_disk.klass) {
		case 0: case 6: case 7: case 8: case 9:
			sprintf(contents, g_disk_logstr[ld.p.cat_disk.klass]);
			break;
		case 1: case 2: case 3: case 4: case 5:
			sprintf(contents, g_disk_logstr[ld.p.cat_disk.klass], g_location_logstr[ld.p.cat_disk.location]);
			break;
		}
		break;

	case LT_HDD_FULL:
		if (ld.p.cat_diskfull.location > 1) return;
		sprintf(type, "STORAGE");
		sprintf(contents, g_diskfull_logstr[0], g_location_logstr[ld.p.cat_diskfull.location]);
		break;

	case LT_HDD_OW:
		if (ld.p.cat_diskow.location > 1) return;
		sprintf(type, "STORAGE");
		sprintf(contents, g_diskow_logstr[0], g_location_logstr[ld.p.cat_diskow.location]);
		break;

	case LT_RECORD_STARTED:
		sprintf(type, "RECORD");
		sprintf(contents, g_recstart_logstr[ld.p.cat_recstart.klass], ld.p.cat_recstart.channel + 1);
		break;

	case LT_RECORD_STOPPED:
		sprintf(type, "RECORD");
		sprintf(contents, g_recstop_logstr[ld.p.cat_recstop.klass], ld.p.cat_recstop.channel + 1);
		break;

	case LT_SYSTEM_EVENT:
		sprintf(type, "SYSTEM");
		switch (ld.p.cat_sysevt.klass) {
		case 0: case 1: case 2: case 3: case 4: case 5: case 8: 
		case 10: case 12: case 14: case 18: case 20: case 21: case 22: case 23: case 25: case 26:
		case 27: case 31: case 33: case 34: case 35: case 36:
			sprintf(contents, g_sysevt_logstr[ld.p.cat_sysevt.klass], ld.p.cat_sysevt.text);
			break;
		case 6:
			sprintf(contents, g_sysevt_logstr[ld.p.cat_sysevt.klass], ld.p.cat_sysevt.channel + 1);
			break;
        case 7: case 9: case 11: case 13: case 15: case 16: case 17: case 19: case 24:
		case 28: case 29: case 30: case 32: case 37: case 38:
			sprintf(contents, g_sysevt_logstr[ld.p.cat_sysevt.klass]);
			break;
		}
		break;

	case LT_SYSTEM_DEBUG:
		sprintf(type, "DEBUG");
		strcpy(contents, ld.p.cat_debug.text);
		break;

	case LT_NETWORK_EVENT:
		sprintf(type, "NETWORK");
		switch (ld.p.cat_network.klass) {
		case 0: case 1: case 2: case 3: case 22: case 23:
			sprintf(contents, g_network_logstr[ld.p.cat_network.klass]);
			break;
		case 4: case 5: case 6: case 7:
			sprintf(contents, g_network_logstr[ld.p.cat_network.klass], ld.p.cat_network.channel);
			break;
		case 8: case 9: case 10: case 11:
			sprintf(contents, g_network_logstr[ld.p.cat_network.klass], ld.p.cat_network.text);
			break;
		case 12: case 13: case 14: case 15: case 16: case 17: case 18: 	case 19:
			sprintf(contents, g_network_logstr[ld.p.cat_network.klass]);
			break;
		case 20:
			sprintf(contents, g_network_logstr[ld.p.cat_network.klass], ld.p.cat_network.text, ld.p.cat_network.text);
			break;
	    case 21: case 24: case 25:
	        sprintf(contents, g_network_logstr[ld.p.cat_network.klass], ld.p.cat_network.channel, ld.p.cat_network.text);
	        break;
		}
		break;

	case LT_IPCAM:
		sprintf(type, "IP CAMERA");
		sprintf(contents, g_ipcamevt_logstr[ld.p.cat_ipcam.klass], ld.p.cat_ipcam.channel + 1);
		break;

	case LT_VCA:
//		nf_api_vca_get_event_string((void *)ld.p.cat_vca.binary, strbuf);
		sprintf(type, "VCA");
		sprintf(contents, g_vcaevt_logstr[0], ld.p.cat_vca.channel + 1, g_vcalog_str[ld.p.cat_vca.type]);
		break;

	case LT_SYSTEM_POS:
		sprintf(type, "POS/ATM");
		sprintf(contents, g_posevt_logstr[0], ld.p.cat_pos.channel + 1, ld.p.cat_pos.text);
		break;

	default:
		sprintf(contents, "Unknown log type, (type:%d)", ld.type);
		break;
	}

}


static void _update_log_data(gint log_cnt, LOG_DATA_T data[SBE_LOG_LABEL_ROWS])
{
	gint i;
	gchar strType[64];
	gchar strTime[128];
	gchar strCts[1024];

#if 0
// for debugging
	char id[64];
#endif
	for(i = 0; i < SBE_LOG_LABEL_ROWS; i++)
	{
		if (i < log_cnt)
		{
			_set_data_LogLabel(i, data[i]);

			memset(strType, 0x00, sizeof(strType));
			memset(strTime, 0x00, sizeof(strTime));
			memset(strCts, 0x00, sizeof(strCts));

			_trans_logData_into_text(data[i], strType, strCts);
			dtf_get_local_datetime(data[i].tvTime.tv_sec, strTime);

// for debuggin
//			sprintf(id, "%llu", data[i].log_id);
//			nfui_nflabel_set_text((NFLABEL*)log_label[i][LOGLABEL_TYPE], id);

			nfui_nflabel_set_text((NFLABEL*)log_label[i][LOGLABEL_TYPE], strType);
			nfui_nflabel_set_text((NFLABEL*)log_label[i][LOGLABEL_TIME], strTime);
			nfui_nflabel_set_text((NFLABEL*)log_label[i][LOGLABEL_CONTENTS], g_strdelimit(strCts, "\n", ' '));
		}
		else
		{
			g_log_time[i].tv_sec = 0;
			nfui_nflabel_set_text((NFLABEL*)log_label[i][LOGLABEL_TYPE], "");
			nfui_nflabel_set_text((NFLABEL*)log_label[i][LOGLABEL_TIME], "");
			nfui_nflabel_set_text((NFLABEL*)log_label[i][LOGLABEL_CONTENTS], "");
		}

		nfui_signal_emit(log_label[i][LOGLABEL_TYPE], GDK_EXPOSE, FALSE);
		nfui_signal_emit(log_label[i][LOGLABEL_TIME], GDK_EXPOSE, FALSE);
		nfui_signal_emit(log_label[i][LOGLABEL_CONTENTS], GDK_EXPOSE, FALSE);
	}
}

static void _search_log()
{
	LOGCTX log_ctx;
	GTimeVal from_time;
	GTimeVal to_time;
	gboolean next = FALSE;

	LOG_DATA_T log_data[SBE_LOG_LABEL_ROWS];
	gint log_cnt;

	memset(&from_time, 0x00, sizeof(GTimeVal));
	memset(&to_time, 0x00, sizeof(GTimeVal));

	_dir_prev_button_disable();
	_playback_button_disable();
	_erase_selectLine();
	_preview_obj_off("");
	vsm_playback_preview_stop();

	log_ctx = GPOINTER_TO_SIZE(nfui_nfobject_get_data(search_obj, "log ctx"));
	nfui_nftimelabel_get_datetime((NFTIMELABEL*)from_obj, &from_time);
	nfui_nftimelabel_get_datetime((NFTIMELABEL*)to_obj, &to_time);

	log_cnt = scm_get_log(log_ctx, &from_time, &to_time, SBE_LOG_LABEL_ROWS, log_data, &next);
	_update_log_data(log_cnt, log_data);

	if (next) _dir_next_button_enable();
	else _dir_next_button_disable();
}

static void _hide_log()
{
	gint i;

	memset(g_log_time, 0x00, sizeof(GTimeVal)*SBE_LOG_LABEL_ROWS);

	for(i = 0; i < SBE_LOG_LABEL_ROWS; i++) {
		nfui_nflabel_set_text((NFLABEL*)log_label[i][LOGLABEL_TYPE], "");
		nfui_nflabel_set_text((NFLABEL*)log_label[i][LOGLABEL_TIME], "");
		nfui_nflabel_set_text((NFLABEL*)log_label[i][LOGLABEL_CONTENTS], "");

		nfui_signal_emit(log_label[i][LOGLABEL_TYPE], GDK_EXPOSE, FALSE);
		nfui_signal_emit(log_label[i][LOGLABEL_TIME], GDK_EXPOSE, FALSE);
		nfui_signal_emit(log_label[i][LOGLABEL_CONTENTS], GDK_EXPOSE, FALSE);
	}
}

static gint _find_index_matched_logOBJ(NFOBJECT *obj)
{
	gint i;

	for(i = 0; i < SBE_LOG_LABEL_ROWS; i++)
	{
		if ((log_label[i][LOGLABEL_TYPE] == obj)
			|| (log_label[i][LOGLABEL_TIME] == obj)
			|| (log_label[i][LOGLABEL_CONTENTS] == obj))
		{
			break;
		}
	}

	if (i == SBE_LOG_LABEL_ROWS)
	{
		g_warning("%s, %d, not matched", __FUNCTION__, __LINE__);
		g_assert(0);
	}

	return i;
}

static gint _find_column_matched_logOBJ(NFOBJECT *obj, gint row)
{
	gint i;

	for(i = 0; i < LOGLABEL_ALL; i++)
	{
		if (log_label[row][i] == obj)
			break;
	}

	if (i == LOGLABEL_ALL)
	{
		g_warning("%s, %d, not matched", __FUNCTION__, __LINE__);
		g_assert(0);
	}

	return i;
}

static void _evt_to_label_playback_handling(gint index)
{
	guint ch_mask;

	ch_mask = GPOINTER_TO_UINT(nfui_nfobject_get_data(log_label[index][LOGLABEL_CONTENTS], "log chmask"));

/*
    if ((ssm_get_covert_mask() & ch_mask) == ch_mask) {
        _preview_obj_off("COVERT");
        return;
    }
*/

	vsm_playback_preview_stop();
	_preview_obj_off("");

	VW_Search_start_playback();
	vsm_playback_start(ch_mask, g_log_time[index], PLAYBACK_NORMAL);
}

static void _evt_to_label_preview_handling(gint index)
{
	guint ch_mask;
	gint gap_x, gap_y;
	gint shown_as;

	vsm_playback_preview_stop();
	_playback_button_enable();

	ch_mask = GPOINTER_TO_UINT(nfui_nfobject_get_data(log_label[index][LOGLABEL_CONTENTS], "log chmask"));

	if (ch_mask == 0xffff)
	{
		_preview_obj_off("N/A");
	}
	else
	{
        if ((ssm_get_covert_mask() & ch_mask) == ch_mask)
        {
            shown_as = ssm_get_covert_shown_as();

            if (!shown_as)
            {
                _preview_obj_off("NO VIDEO");
            }
            else
            {
                _preview_obj_off("COVERT");
            }

            return;
        }

    	nfui_nfobject_get_offset((NFOBJECT*)fixed1, &gap_x, &gap_y);

		_preview_obj_on("");
		vsm_playback_preview_start(ch_mask, g_log_time[index],
		    gap_x+SBE_PREVIEW_VIDEO_X, gap_y+SBE_PREVIEW_VIDEO_Y, SBE_PREVIEW_VIDEO_W, SBE_PREVIEW_VIDEO_H);
	}
}

static void _change_obj_focus(NFOBJECT* from, NFOBJECT *to)
{
	nfui_set_key_focus(from, FALSE);
	nfui_set_key_focus(to, TRUE);

	nfui_signal_emit(from, GDK_EXPOSE, TRUE);
	nfui_signal_emit(to, GDK_EXPOSE, TRUE);
}

static gboolean post_from_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	guint i;
	int x, y;
	NFOBJECT *top;

	GTimeVal from_tv;
	GTimeVal to_tv;
	GTimeVal temp_tv;

	memset(&temp_tv, 0x00, sizeof(GTimeVal));
	memset(&from_tv, 0x00, sizeof(GTimeVal));
	memset(&to_tv, 0x00, sizeof(GTimeVal));

	if (evt->type == GDK_BUTTON_RELEASE)
	{
  	   	if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		nfui_nfobject_get_offset(obj, &x, &y);
		top = nfui_nfobject_get_top(obj);

		x += top->x + obj->width + 4;
		y += top->y;

		nfui_nftimelabel_get_datetime(from_obj, &from_tv);
		nfui_nftimelabel_get_datetime(to_obj, &to_tv);
		to_tv.tv_sec -= 1;

        temp_tv.tv_sec = VW_Set_DateTime_Open(g_curwnd, "FROM", x, y, from_tv.tv_sec, SDT_TYPE_SEC, NF_LOWER_TIMELIMIT, to_tv.tv_sec);

		if (temp_tv.tv_sec != 0)
		{
			nfui_nftimelabel_set_datetime((NFTIMELABEL*)from_obj, &temp_tv);
			nfui_signal_emit((NFOBJECT*)from_obj, GDK_EXPOSE, TRUE);
		}

        if ((temp_tv.tv_sec != 0) && (from_tv.tv_sec != temp_tv.tv_sec))
		{
			_order_button_disable();
		}
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_UP)
		{
			_change_obj_focus(obj, search_obj);
			return TRUE;
		}
	}

	return FALSE;
}

static gboolean post_to_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	guint i;
	int x, y;
	NFOBJECT *top;

	GTimeVal from_tv;
	GTimeVal to_tv;
	GTimeVal temp_tv;

	memset(&temp_tv, 0x00, sizeof(GTimeVal));
	memset(&from_tv, 0x00, sizeof(GTimeVal));
	memset(&to_tv, 0x00, sizeof(GTimeVal));

	if (evt->type == GDK_BUTTON_RELEASE)
	{
  	   	if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		nfui_nfobject_get_offset(obj, &x, &y);
		top = nfui_nfobject_get_top(obj);

		x += top->x + obj->width + 4;
		y += top->y;

		nfui_nftimelabel_get_datetime(from_obj, &from_tv);
		nfui_nftimelabel_get_datetime(to_obj, &to_tv);
		from_tv.tv_sec += 1;

        temp_tv.tv_sec = VW_Set_DateTime_Open(g_curwnd, "TO", x, y, to_tv.tv_sec, SDT_TYPE_SEC, from_tv.tv_sec, NF_UPPER_TIMELIMIT);

		if (temp_tv.tv_sec != 0)
		{
			nfui_nftimelabel_set_datetime((NFTIMELABEL*)to_obj, &temp_tv);
			nfui_signal_emit((NFOBJECT*)to_obj, GDK_EXPOSE, TRUE);
		}

        if ((temp_tv.tv_sec != 0) && (to_tv.tv_sec != temp_tv.tv_sec))
		{
			_order_button_disable();
		}
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_DOWN)
		{
			_change_obj_focus(obj, ch_obj);
			return TRUE;
		}
	}

	return FALSE;
}

static unsigned int _get_chmask(int ch)
{
	unsigned int chmask = 0;
	if (ch < 0) return 0xFFFFFFFF;
	return chmask |= (1 << ch);
}

static int _change_filter_mask(LOGCTX log_ctx)
{
	gint i;
	gboolean state;
	unsigned int chmask = 0;
	int index;
	int lcat;

	scm_reset_log_filter(log_ctx, LF_NOT_ALL);

	index = nfui_combobox_get_cur_index(ch_obj);
	if (index == 0) {
		for (i = 0; i < var_get_ch_count(); ++i)
			chmask |= _get_chmask(i);
	}
	else {
		chmask = _get_chmask(index - 1);
	}

	for (i = OPT_EVT_ALL; i < OPT_EVT_MAX; i++)
	{
		state = nfui_check_button_get_active((NFCHECKBUTTON*)opt_obj[i]);

		switch (i)
		{
			case OPT_EVT_ALL: 		lcat = LF_CAT_ALL; 		break;
			case OPT_EVT_VCA: 		lcat = LF_CAT_VCA; 		break;
			case OPT_EVT_ALARM: 	lcat = LF_CAT_ALARM; 	break;
			case OPT_EVT_MOTION: 	lcat = LF_CAT_MOTION; 	break;
			case OPT_EVT_VIDEO: 	lcat = LF_CAT_VLOSS; 	break;
			case OPT_EVT_RECORD: 	lcat = LF_CAT_RECORD; 	break;
			case OPT_EVT_SYSTEM: 	lcat = LF_CAT_SYSTEM; 	break;
			case OPT_EVT_STORAGE: 	lcat = LF_CAT_STORAGE; 	break;
			case OPT_EVT_NETWORK: 	lcat = LF_CAT_NETWORK; 	break;
#if defined(_IPX_MODEL_UX)
			case OPT_EVT_IPCAM: 	lcat = LF_CAT_IPCAM; 	break;
#endif
#if defined(_SUPPORT_TAMPER_SETUP)
//			case OPT_EVT_TAMPER: 	lcat = LF_CAT_TAMPER; 	break;
#endif
			case OPT_EVT_SETUP: 	lcat = LF_CAT_SETUP; 	break;
			case OPT_EVT_POS:		lcat = LF_CAT_POS;		break;
		}

		if(state) {
			scm_set_log_filter_type(log_ctx, chmask, lcat, 1);
		}
		else {
			scm_set_log_filter_type(log_ctx, chmask, lcat, 0);
		}
	}

    if (opt_debug_obj)
    {
        state = nfui_check_button_get_active((NFCHECKBUTTON*)opt_debug_obj);

    	if(state) {
    		scm_set_log_filter_type(log_ctx, chmask, LF_CAT_DEBUG, 1);
    	}
    	else {
    		scm_set_log_filter_type(log_ctx, chmask, LF_CAT_DEBUG, 0);
    	}
	}

	return 0;
}

static gboolean post_channel_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_COMBOBOX_CHANGED)
	{
		LOGCTX log_ctx;
		gint index;
		gint ch;

		_order_button_disable();
		log_ctx = GPOINTER_TO_SIZE(nfui_nfobject_get_data(search_obj, "log ctx"));

		index = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));

		if (index == 0)
		{
			_change_filter_mask(log_ctx);

			nfui_nfobject_enable(opt_obj[OPT_EVT_ALL]);
			nfui_nfobject_enable(opt_obj[OPT_EVT_SYSTEM]);
			nfui_nfobject_enable(opt_obj[OPT_EVT_STORAGE]);
			nfui_nfobject_enable(opt_obj[OPT_EVT_NETWORK]);
			nfui_nfobject_enable(opt_obj[OPT_EVT_SETUP]);
			nfui_nfobject_enable(opt_obj[OPT_EVT_POS]);
			if (opt_debug_obj) nfui_nfobject_enable(opt_debug_obj);

			nfui_signal_emit(opt_obj[OPT_EVT_ALL], GDK_EXPOSE, FALSE);
			nfui_signal_emit(opt_obj[OPT_EVT_SYSTEM], GDK_EXPOSE, FALSE);
			nfui_signal_emit(opt_obj[OPT_EVT_STORAGE], GDK_EXPOSE, FALSE);
			nfui_signal_emit(opt_obj[OPT_EVT_NETWORK], GDK_EXPOSE, FALSE);
			nfui_signal_emit(opt_obj[OPT_EVT_SETUP], GDK_EXPOSE, FALSE);
			nfui_signal_emit(opt_obj[OPT_EVT_POS], GDK_EXPOSE, FALSE);
			if (opt_debug_obj) nfui_signal_emit(opt_debug_obj, GDK_EXPOSE, FALSE);
		}
		else
		{

			nfui_check_button_set_active(opt_obj[OPT_EVT_ALL], FALSE);
			nfui_check_button_set_active(opt_obj[OPT_EVT_SYSTEM], FALSE);
			nfui_check_button_set_active(opt_obj[OPT_EVT_STORAGE], FALSE);
			nfui_check_button_set_active(opt_obj[OPT_EVT_NETWORK], FALSE);
			nfui_check_button_set_active(opt_obj[OPT_EVT_SETUP], FALSE);
			if (opt_debug_obj) nfui_check_button_set_active(opt_debug_obj, FALSE);

			_change_filter_mask(log_ctx);

			nfui_nfobject_disable(opt_obj[OPT_EVT_ALL]);
			nfui_nfobject_disable(opt_obj[OPT_EVT_SYSTEM]);
			nfui_nfobject_disable(opt_obj[OPT_EVT_STORAGE]);
			nfui_nfobject_disable(opt_obj[OPT_EVT_NETWORK]);
			nfui_nfobject_disable(opt_obj[OPT_EVT_SETUP]);
			if (opt_debug_obj) nfui_nfobject_disable(opt_debug_obj);

			nfui_signal_emit(opt_obj[OPT_EVT_ALL], GDK_EXPOSE, FALSE);
			nfui_signal_emit(opt_obj[OPT_EVT_SYSTEM], GDK_EXPOSE, FALSE);
			nfui_signal_emit(opt_obj[OPT_EVT_STORAGE], GDK_EXPOSE, FALSE);
			nfui_signal_emit(opt_obj[OPT_EVT_NETWORK], GDK_EXPOSE, FALSE);
			nfui_signal_emit(opt_obj[OPT_EVT_SETUP], GDK_EXPOSE, FALSE);
			if (opt_debug_obj) nfui_signal_emit(opt_debug_obj, GDK_EXPOSE, FALSE);
		}
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_UP)
		{
			_change_obj_focus(obj, to_btn_obj);
			return TRUE;
		}
		else if (kpid == KEYPAD_DOWN)
		{
			_change_obj_focus(obj, opt_obj[OPT_EVT_MOTION]);
			return TRUE;
		}
	}

	return FALSE;
}

static unsigned int _add_ch_to_mask(unsigned int chmask, int ch)
{
	return chmask |= (1 << ch);
}

static unsigned int _remove_ch_to_mask(unsigned int chmask, int ch)
{
	return chmask &= ~(1 << ch);
}

static gint _set_all_chk_btn(NFOBJECT *all, NFOBJECT *unit[], gboolean expose)
{
	gboolean state;
	gint i, start;

	if (!var_get_supported_dmva())
    	start = OPT_EVT_ALARM;
	else
		start = OPT_EVT_VCA;

	for (i = start; i < OPT_EVT_MAX; i++)
	{
	    state = nfui_check_button_get_active((NFCHECKBUTTON*)unit[i]);

		if (!state)
		{
		    if (expose)
                nfui_check_button_set_active((NFCHECKBUTTON*)all, FALSE);
            else
                nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)all, FALSE);
            return 0;
        }
	}

	if (i == OPT_EVT_MAX)
	{
	    if (expose)
    	    nfui_check_button_set_active((NFCHECKBUTTON*)all, TRUE);
	    else
	        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)all, TRUE);
    }

    return 0;
}

static gboolean post_opt_evt_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	int i;
	gboolean state;

	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
		LOGCTX log_ctx;
		gint ch;

		for (i = OPT_EVT_ALL; i < OPT_EVT_MAX; i++)
		{
			if (opt_obj[i] == obj)
				break;
		}

		if (i == OPT_EVT_MAX)
			return FALSE;

		_order_button_disable();


		if(i == OPT_EVT_ALL)
		{
			state = nfui_check_button_get_active((NFCHECKBUTTON*)opt_obj[OPT_EVT_ALL]);

            for(i = 1; i < OPT_EVT_MAX; i++) {
                if (!var_get_supported_dmva() && i == OPT_EVT_VCA) continue;
                if (!ivsc.dfunc.support_posevent && i == OPT_EVT_POS) continue;

                nfui_check_button_set_active((NFCHECKBUTTON*)opt_obj[i], state);
            }
		}
		else
		{
			_set_all_chk_btn(opt_obj[OPT_EVT_ALL], opt_obj, TRUE);
		}

		log_ctx = GPOINTER_TO_SIZE(nfui_nfobject_get_data(search_obj, "log ctx"));

		_change_filter_mask(log_ctx);

	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
		gint i;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		for (i = 0; i < OPT_EVT_MAX; i++)
		{
			if (opt_obj[i] == obj)	break;
		}

		if(kpid == KEYPAD_UP)
		{
		    if (i == OPT_EVT_ALL)
		    {
				_change_obj_focus(obj, ch_obj);
				return TRUE;
		    }
			if (i <= OPT_EVT_MOTION)
			{
				_change_obj_focus(obj, opt_obj[OPT_EVT_ALL]);
				return TRUE;
			}
			else
			{
				_change_obj_focus(obj, opt_obj[i-2]);
				return TRUE;
			}
		}
		else if (kpid == KEYPAD_DOWN)
		{
			if (i == OPT_EVT_ALL)
			{
				_change_obj_focus(obj, opt_obj[OPT_EVT_VIDEO]);
				return TRUE;
			}
			else
			{
				if (i+2 < OPT_EVT_MAX)
					_change_obj_focus(obj, opt_obj[i+2]);
				else 
				{
					if (opt_debug_obj) _change_obj_focus(obj, opt_debug_obj);
					else _change_obj_focus(obj, search_obj);
				}
				return TRUE;
			}
		}
	}

	return FALSE;
}

static gboolean post_opt_debug_evt_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	int i;
	gboolean state;

	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
		LOGCTX log_ctx;
		gint ch;

		_order_button_disable();

		log_ctx = GPOINTER_TO_SIZE(nfui_nfobject_get_data(search_obj, "log ctx"));
		_change_filter_mask(log_ctx);
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
		gint i;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_UP)
		{
			_change_obj_focus(obj, opt_obj[OPT_EVT_SETUP]);
			return TRUE;
		}
		else if (kpid == KEYPAD_DOWN)
		{
			_change_obj_focus(obj, search_obj);
			return TRUE;
		}
	}

	return FALSE;
}

static gboolean post_search_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	guint i;
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if (evt->type == GDK_BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		_search_log();
		_order_button_enable();
	}
	else if (kpid == KEYPAD_UP)
	{
		gint focus_idx = 0;

		focus_idx = OPT_EVT_MAX-1;
		if (focus_idx%2) focus_idx--;
		_change_obj_focus(obj, opt_obj[focus_idx]);
		return TRUE;
	}
	else if (kpid == KEYPAD_DOWN)
	{
		_change_obj_focus(obj, from_btn_obj);
		return TRUE;
	}

	if(evt->type == GDK_DELETE)
	{
		LOGCTX logctx;

		logctx = GPOINTER_TO_SIZE(nfui_nfobject_get_data(search_obj, "log ctx"));
		scm_close_log_ctx(logctx);
	}

	return FALSE;
}

static gboolean pre_log_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;
	gint row, col;
	NFOBJECT* cur_focus;

	if (obj->kfocus == NFOBJECT_FOCUS)
	{
		if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
		{
			kevt = (GdkEventKey*)evt;
			kpid = (KEYPAD_KID)kevt->keyval;

			row = _find_index_matched_logOBJ(obj);
			col = _find_column_matched_logOBJ(obj, row);

			if ((kpid == KEYPAD_LEFT) && (col != LOGLABEL_TYPE))
			{
				cur_focus = nfui_get_cur_focus(obj);
				if (!cur_focus) return FALSE;
				_change_obj_focus(cur_focus, log_label[row][LOGLABEL_TYPE]);
			}
			else if ((kpid == KEYPAD_RIGHT) && (col != LOGLABEL_CONTENTS))
			{
				cur_focus = nfui_get_cur_focus(obj);
				if (!cur_focus) return FALSE;
				_change_obj_focus(cur_focus, log_label[row][LOGLABEL_CONTENTS]);
			}
		}
	}

	return FALSE;
}

static gboolean post_log_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	guint i, ch_mask;
	gint result;

	if (evt->type == GDK_EXPOSE)
	{
		i = _find_index_matched_logOBJ(obj);
		if (obj->kfocus == NFOBJECT_FOCUS)
			_draw_focusLine(i);
		else
			_erase_focusLine(i);
	}

	if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_BUTTON_PRESS || evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		i = _find_index_matched_logOBJ(obj);
		result = _draw_selectLine(i);

		if (g_log_time[i].tv_sec == 0)
		{
			_playback_button_disable();
			_preview_obj_off("N/A");
			vsm_playback_preview_stop();
			return FALSE;
		}

		stm_set_time_t(g_log_time[i].tv_sec);

		if ((evt->type == GDK_2BUTTON_PRESS) || ((kpid == KEYPAD_ENTER) && (result == -1)))
			_evt_to_label_playback_handling(i);
		else
			_evt_to_label_preview_handling(i);
	}
	else if(evt->type == GDK_ENTER_NOTIFY)
	{
		NFOBJECT* cur_focus;

		cur_focus = nfui_get_cur_focus(obj);
		if (!cur_focus)	return FALSE;
		_change_obj_focus(cur_focus, obj);
	}
	else if(kpid == KEYPAD_UP)
	{
		i = _find_index_matched_logOBJ(obj);

		if (i != 0)
		{
			_change_obj_focus(obj, log_label[i-1][LOGLABEL_TYPE]);
			return TRUE;
		}
	}
	else if (kpid == KEYPAD_DOWN)
	{
		i = _find_index_matched_logOBJ(obj);

		if (i+1 < SBE_LOG_LABEL_ROWS)
		{
			_change_obj_focus(obj, log_label[i+1][LOGLABEL_TYPE]);
			return TRUE;
		}
	}

#if 0
	else if(evt->type == GDK_MOTION_NOTIFY)
	{
		i = _find_index_matched_logOBJ(obj);
		_draw_focusLine(i);
	}
	else if(evt->type == GDK_LEAVE_NOTIFY)
	{
		_erase_focusLine();
	}
#endif

	return FALSE;
}

static gboolean post_order_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_COMBOBOX_CHANGED)
	{
		LOGCTX log_ctx;
		gint index;

		log_ctx = GPOINTER_TO_SIZE(nfui_nfobject_get_data(search_obj, "log ctx"));

		index = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));

		if (index == 0)
		{
			scm_set_log_filter_order(log_ctx, LF_LATEST);
			_search_log();
		}
		else
		{
			scm_set_log_filter_order(log_ctx, LF_OLDEST);
			_search_log();
		}
	}

	return FALSE;
}

static gboolean post_prev_log_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gboolean prev = FALSE;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		LOG_DATA_T log_data[SBE_LOG_LABEL_ROWS];
		LOGCTX log_ctx;
		gint log_cnt;

		log_ctx = GPOINTER_TO_SIZE(nfui_nfobject_get_data(search_obj, "log ctx"));

		log_cnt = scm_get_log_prev(log_ctx, SBE_LOG_LABEL_ROWS, log_data, &prev);

		_playback_button_disable();
		_erase_selectLine();
		_update_log_data(log_cnt, log_data);
		_dir_next_button_enable();

		if (!prev) _dir_prev_button_disable();
	}

	return FALSE;
}

static gboolean post_next_log_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gboolean next = FALSE;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		LOG_DATA_T log_data[SBE_LOG_LABEL_ROWS];
		LOGCTX log_ctx;
		gint log_cnt;

		log_ctx = GPOINTER_TO_SIZE(nfui_nfobject_get_data(search_obj, "log ctx"));

		log_cnt = scm_get_log_next(log_ctx, SBE_LOG_LABEL_ROWS, log_data, &next);

		_playback_button_disable();
		_erase_selectLine();
		_update_log_data(log_cnt, log_data);
		_dir_prev_button_enable();

		if (!next) _dir_next_button_disable();
	}

	return FALSE;
}

static gboolean pre_opt_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	switch(evt->type)
	{
		case GDK_EXPOSE:
		{
            gint gap_x, gap_y;

    		drawable = nfui_nfobject_get_window(obj);
            gc = gdk_gc_new(drawable);

            nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);
            nfui_nfobject_get_size(obj, &size_w, &size_h);
            pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_SUB_GROUP_BG, size_w, size_h);
            nfutil_draw_pixbuf(drawable, gc, pbuf, gap_x, gap_y, -1, -1, NFALIGN_LEFT, 0);

            if (opt_debug_obj)
            {
                gdk_gc_set_rgb_fg_color(gc, &UX_COLOR((604)));
                gdk_draw_rectangle(drawable, gc, TRUE, gap_x+8, gap_y+opt_debug_obj->y-10, size_w-16, 1);
            }

    		nfui_nfobject_gc_unref(gc);
    	}
		break;

		case GDK_DELETE:
		{
            nfui_nfobject_get_size(obj, &size_w, &size_h);
            nfui_unref_popup_pixbuf(MK_IMG_SUB_GROUP_BG, size_w, size_h);
        }
		break;

		default :

		break;
	}

	return FALSE;
}

static gboolean _set_preview_time(gpointer data)
{
	GTimeVal tmp;
	gchar strBuf[64];

	memset(&tmp, 0x00, sizeof(GTimeVal));
	tmp = vsm_playback_get_previewtime();

	if (tmp.tv_sec != 0)
	{
		dtf_get_local_datetime(tmp.tv_sec, strBuf);
		nfui_nflabel_set_text((NFLABEL*)play_time_obj, strBuf);
	}
	else
	{
		g_sprintf(strBuf, "");
		nfui_nflabel_set_text((NFLABEL*)play_time_obj, strBuf);
	}

	nfui_signal_emit(play_time_obj, GDK_EXPOSE, FALSE);

	return TRUE;
}

static gboolean _stop_live(void *data)
{
	if (!nfui_nfwindow_find("SEARCH"))	return FALSE;

	vsm_live_stop();
	g_init_sbe = FALSE;

	return FALSE;
}

static gboolean
pre_fixed1_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case GDK_EXPOSE:
		{
			if (g_init_sbe) g_timeout_add(300, _stop_live, 0);

			if (preview_timer_id == 0)
				preview_timer_id = g_timeout_add(500, _set_preview_time, NULL);

	 	}
		break;

		case GDK_DELETE:
		{
			g_init_sbe = TRUE;

			if(preview_timer_id != 0) {
				g_source_remove(preview_timer_id);
				preview_timer_id = 0;
			}
		}
		break;

		default :

		break;
	}

	return FALSE;
}

static gboolean
pre_fixed2_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint x, y;
	static GdkGC *gc;
	static GdkDrawable *drawable;

	switch(evt->type)
	{
		case GDK_EXPOSE:
		{
			if (selected_row != -1)
				_change_outLine_LogLabel(selected_row, COLOR_IDX(SELECT_OUTLINE_COLOR));
	 	}
		break;


		default :

		break;
	}

	return FALSE;
}

static gboolean post_playbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {
			return FALSE;
		}

		if ((selected_row < 0) || (selected_row > SBE_LOG_LABEL_ROWS))
		{
			g_warning("%s, %d", __FUNCTION__, __LINE__);
			return FALSE;
		}

		_evt_to_label_playback_handling(selected_row);
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

		vsm_playback_preview_stop();
		VW_Search_Destroy();
	}

	return FALSE;
}


static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
	    opt_debug_obj = NULL;
		g_curwnd = 0;
	}

	return FALSE;
}


/*
 :  search by event main_fixed.

	  --------------------------------------------
	  |  ---------------------------------------  |
	  |	|			|							| |
	  |	|			|							| |
	  |	|			|							| |
	  |	|			|							| |
	  |	|			|							| |
	  |	|  FIXED1	|		  FIXED2			| |
	  |	|			|							| |
	  |	|			|							| |
	  |	|			|							| |
	  |	|			|							| |
	  |	|			|							| |
	  |	|			|							| |
	  |	 ---------------------------------------  |
	  |											  |
	  |				PARENT						  |
	  |-------------------------------------------|
*/

void vw_init_SearchByEvent_page(NFOBJECT *parent, time_t from_time, time_t to_time)
{
	NFOBJECT *content_fixed;
	NFOBJECT *fixed2;

	NFOBJECT *obj;
	NFOBJECT *opt_fixed;
	NFOBJECT *ntb1;
	NFOBJECT *ntb2;
    NFOBJECT *lbopt[OPT_EVT_MAX];

	GdkPixbuf *datetime_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *prev_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *next_img[NFOBJECT_STATE_COUNT];

	const gchar *strTitle1[] = {"FROM", "TO"};
	const gchar *strCombo[] = {"LATEST", "OLDEST"};
	const gchar *strTable[] = {"TYPE", "TIME", "CONTENTS"};
	const gchar *strButton[] = {"SEARCH", "PREV.", "NEXT", "PLAYBACK", "CLOSE"};

	const gchar *strOption[] = {"ALL",
				"VCA",
				"ALARM",
				"MOTION",
				"VIDEO",
				"RECORD",
				"SYSTEM",
				"STORAGE",
				"NETWORK",
#if defined(_SUPPORT_TAMPER_SETUP)
				"TAMPER",
#endif
#if defined(_IPX_MODEL_UX)
				"IPCAM",
#endif
				"SETUP",
				"POS/ATM"
				};

	gchar *strCh[GUI_CHANNEL_CNT];

	guint width1[SBE_TABLE1_COLUMNS];
	guint size_w, size_h;
	guint pos_x, pos_y;
	guint row, col, i, ch;

	DateTimeData dtdata;
	guint tformat;

	LOGCTX log_ctx;


	g_curwnd = nfui_nfobject_get_top(parent);


	focused_row = -1;
	selected_row = -1;

	memset(g_log_time, 0x00, sizeof(GTimeVal)*SBE_LOG_LABEL_ROWS);

// DB LOAD
// <---- DB LOAD
	memset(&dtdata, 0x00, sizeof(DateTimeData));

	DAL_get_dateTime_data(&dtdata);
	DAL_get_dateTime_format(NULL, &tformat);


// IMAGE LOAD
	datetime_img[0] = nfui_get_image_from_file((IMG_BT_DATETIME_N), NULL);
	datetime_img[1] = nfui_get_image_from_file((IMG_BT_DATETIME_O), NULL);
	datetime_img[2] = nfui_get_image_from_file((IMG_BT_DATETIME_P), NULL);
	datetime_img[3] = nfui_get_image_from_file((IMG_BT_DATETIME_D), NULL);

	prev_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_N_BTN), NULL);
	prev_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_O_BTN), NULL);
	prev_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_P_BTN), NULL);
	prev_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_D_BTN), NULL);

	next_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_N_BTN), NULL);
	next_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_O_BTN), NULL);
	next_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_P_BTN), NULL);
	next_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_D_BTN), NULL);

	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(content_fixed, MENU_H_INNER_W, MENU_H_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);

// FIXED1 , FIXED2
	fixed1 = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed1, SBE_FIXED1_W, SBE_FIXED1_H);
	nfui_nfobject_show(fixed1);
	nfui_nfobject_modify_bg(fixed1, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nffixed_put((NFFIXED*)content_fixed, fixed1, SBE_FIXED1_X, SBE_FIXED1_Y);

	fixed2 = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed2, SBE_FIXED2_W, SBE_FIXED2_H);
	nfui_nfobject_show(fixed2);
	nfui_nfobject_modify_bg(fixed2, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nffixed_put((NFFIXED*)content_fixed, fixed2, SBE_FIXED2_X, SBE_FIXED2_Y);


//	FROM / TO.
	nfui_get_pixbuf_size(datetime_img[0], &size_w, &size_h);

	width1[0] = 80;
	width1[2] = size_w;
	width1[1] = 410-width1[0]-width1[2];

	ntb1 = (NFOBJECT*)nfui_nftable_new(SBE_TABLE1_COLUMNS, SBE_TABLE1_ROWS, 0, 2, width1, SBE_LABEL_HEIGHT1);
	nfui_nfobject_show(ntb1);
	nfui_nfobject_modify_bg(ntb1, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nffixed_put((NFFIXED*)fixed1, ntb1, SBE_TABLE1_X, SBE_TABLE1_Y);

	for(row=0; row<SBE_TABLE1_ROWS; row++)
	{
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font((gchar*)(strTitle1[row]), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)ntb1, obj,  0, row);

		if (row == 0) {
			GTimeVal time = {0, 0};
			time.tv_sec = from_time;
			from_obj = (NFOBJECT*)nfui_nftimelabel_new();
			nfui_nftimelabel_set_fg_color((NFTIMELABEL*)from_obj, COLOR_IDX(129));
			nfui_nftimelabel_set_bg_color((NFTIMELABEL*)from_obj, COLOR_IDX(128));
			nfui_nftimelabel_set_mode((NFTIMELABEL*)from_obj, prvTransDateFormat((gint)(dtdata.dateFormat)), tformat+1);
			nfui_nftimelabel_set_size((NFTIMELABEL*)from_obj, width1[1], SBE_LABEL_HEIGHT1);
			nfui_nftimelabel_set_datetime((NFTIMELABEL*)from_obj, &time);
			nfui_nfobject_use_focus(from_obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(from_obj);
			nfui_nftable_attach((NFTABLE*)ntb1, from_obj,  1, row);
		} else {
			GTimeVal time = {0, 0};
			time.tv_sec = to_time;
			to_obj = (NFOBJECT*)nfui_nftimelabel_new();
			nfui_nftimelabel_set_fg_color((NFTIMELABEL*)to_obj, COLOR_IDX(129));
			nfui_nftimelabel_set_bg_color((NFTIMELABEL*)to_obj, COLOR_IDX(128));
			nfui_nftimelabel_set_mode((NFTIMELABEL*)to_obj, prvTransDateFormat((gint)(dtdata.dateFormat)), tformat+1);
			nfui_nftimelabel_set_size((NFTIMELABEL*)to_obj, width1[1], SBE_LABEL_HEIGHT1);
			nfui_nftimelabel_set_datetime((NFTIMELABEL*)to_obj, &time);
			nfui_nfobject_use_focus(to_obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(to_obj);
			nfui_nftable_attach((NFTABLE*)ntb1, to_obj,  1, row);
		}

		obj = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), datetime_img);
		nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)ntb1, obj,  2, row);

		if (row == 0)
		{
			nfui_regi_post_event_callback(obj, post_from_event_handler);
			from_btn_obj = obj;
		}
		else
		{
			nfui_regi_post_event_callback(obj, post_to_event_handler);
			to_btn_obj = obj;
		}

	}


//	CHANNEL.
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CHANNEL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, SBE_VDO_CH_WIDTH1, SBE_LABEL_HEIGHT1);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, SBE_VDO_CH_X, SBE_VDO_CH_Y);

	for (ch = 0; ch<GUI_CHANNEL_CNT; ch++)
	{
		strCh[ch] = imalloc(sizeof(gchar) * 64);
//		DAL_get_camera_title(strCh[ch], ch);
		sprintf(strCh[ch], "CH %02d", ch+1);
	}

	obj = nfui_combobox_new(strCh, GUI_CHANNEL_CNT, 0);
	ch_obj = obj;
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_combobox_prepend_data(NF_COMBOBOX(obj), "ALL");
	nfui_nfobject_set_size(obj, SBE_VDO_CH_WIDTH2, SBE_LABEL_HEIGHT1);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, SBE_VDO_CH_X+SBE_VDO_CH_WIDTH1, SBE_VDO_CH_Y);
	nfui_regi_post_event_callback(obj, post_channel_event_handler);

	for(ch=0; ch<GUI_CHANNEL_CNT; ch++)
		ifree(strCh[ch]);


// EVENT.
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("EVENT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(193));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, 180, SBE_LABEL_HEIGHT2);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, SBE_EVENT_TITLE_LABEL_X, SBE_EVENT_TITLE_LABEL_Y);


	opt_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(opt_fixed, SBE_EVENT_OPTFIXED_W, SBE_EVENT_OPTFIXED_H);
	nfui_nfobject_show(opt_fixed);
	nfui_nfobject_modify_bg(opt_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(194));
	nfui_nffixed_put((NFFIXED*)fixed1, opt_fixed, SBE_EVENT_OPTFIXED_X, SBE_EVENT_OPTFIXED_Y);

	pos_x = 11;
	pos_y = 17;

	for(i = OPT_EVT_ALL; i < OPT_EVT_MAX; i++)
	{
		if (i % 2 == 0) pos_x = 11;
		else {
			pos_x = 11;
			pos_x += size_w;
			pos_x += SBE_EVENT_LABEL_WIDTH2 + 5;
		}

		opt_obj[i] = (NFOBJECT*)nfui_checkbutton_new(TRUE);
		nfui_check_button_set_skin_type(NF_CHECKBUTTON(opt_obj[i]), NFCHECK_TYPE_NORMAL);
		nfui_check_get_size(opt_obj[i], &size_w, &size_h);
		nfui_nfobject_show(opt_obj[i]);
		nfui_nffixed_put((NFFIXED*)opt_fixed, opt_obj[i], pos_x, pos_y);
		nfui_regi_post_event_callback(opt_obj[i], post_opt_evt_event_handler);

		if (i % 2 == 0)
		{
			pos_x = 11;
			pos_x += size_w;

			if(nftool_cur_language_is_japanese())
                lbopt[i] = (NFOBJECT*)nfui_nflabel_new_with_pango_font((gchar*)(strOption[i]), nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(122));
			else
                lbopt[i] = (NFOBJECT*)nfui_nflabel_new_with_pango_font((gchar*)(strOption[i]), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(122));
            nfui_nflabel_set_align((NFLABEL*)lbopt[i], NFALIGN_LEFT, 4);
            nfui_nfobject_modify_bg(lbopt[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(194));
            nfui_nfobject_set_size(lbopt[i], SBE_EVENT_LABEL_WIDTH1 - size_w, SBE_LABEL_HEIGHT3);
            nfui_nfobject_use_focus(lbopt[i], NFOBJECT_FOCUS_OFF);
            nfui_nfobject_show(lbopt[i]);
            nfui_nffixed_put((NFFIXED*)opt_fixed, lbopt[i], pos_x, pos_y);
		}
		else
		{
			pos_x = 11;
			pos_x += size_w;
			pos_x += SBE_EVENT_LABEL_WIDTH2 + size_w + 5;

			if(nftool_cur_language_is_japanese())
                lbopt[i] = (NFOBJECT*)nfui_nflabel_new_with_pango_font((gchar*)(strOption[i]), nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(122));
			else
                lbopt[i] = (NFOBJECT*)nfui_nflabel_new_with_pango_font((gchar*)(strOption[i]), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(122));
            nfui_nflabel_set_align((NFLABEL*)lbopt[i], NFALIGN_LEFT, 4);
            nfui_nfobject_modify_bg(lbopt[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(194));
            nfui_nfobject_set_size(lbopt[i], SBE_EVENT_LABEL_WIDTH2 - size_w, SBE_LABEL_HEIGHT3);
            nfui_nfobject_use_focus(lbopt[i], NFOBJECT_FOCUS_OFF);
            nfui_nfobject_show(lbopt[i]);
            nfui_nffixed_put((NFFIXED*)opt_fixed, lbopt[i], pos_x, pos_y);

            pos_y += SBE_LABEL_HEIGHT3 + 14;
		}

    	if (i == OPT_EVT_VCA) {
    		if (!var_get_supported_dmva()) {
    			nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(opt_obj[i]), FALSE);
    			nfui_nfobject_hide(opt_obj[i]);
    			nfui_nfobject_hide(lbopt[i]);
    		}
    	}
	}


    if (!ivsc.dfunc.support_posevent) {
        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)opt_obj[OPT_EVT_POS], FALSE);
        nfui_nfobject_hide(lbopt[OPT_EVT_POS]);
        nfui_nfobject_hide(opt_obj[OPT_EVT_POS]);
    }

#if 0
    if (strcmp(nf_sysdb_get_str_nocopy("net.email.testmail"), "choissi@debug.com") == 0)
    {
    	pos_x = 11;
        pos_y += 10;

    	opt_debug_obj = (NFOBJECT*)nfui_checkbutton_new(FALSE);
    	nfui_check_button_set_skin_type(NF_CHECKBUTTON(opt_debug_obj), NFCHECK_TYPE_NORMAL);
    	nfui_check_get_size(opt_debug_obj, &size_w, &size_h);
    	nfui_nfobject_show(opt_debug_obj);
    	nfui_nffixed_put((NFFIXED*)opt_fixed, opt_debug_obj, pos_x, pos_y);
    	nfui_regi_post_event_callback(opt_debug_obj, post_opt_debug_evt_event_handler);

    	pos_x += size_w;

    	if(nftool_cur_language_is_japanese())
    		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DEBUG", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(122));
    	else
    		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DEBUG", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(122));
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
    	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(194));
    	nfui_nfobject_set_size(obj, SBE_EVENT_LABEL_WIDTH1 - size_w, SBE_LABEL_HEIGHT3);
    	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)opt_fixed, obj, pos_x, pos_y);

    	pos_y = SBE_SEARCH_BUTTON_Y;
	}
	else
	{
	    nfui_nfobject_set_size(opt_fixed, SBE_EVENT_OPTFIXED_W, SBE_EVENT_OPTFIXED_H - 40);
	    pos_y = SBE_SEARCH_BUTTON_Y - 40;
	}
#else
	nfui_nfobject_set_size(opt_fixed, SBE_EVENT_OPTFIXED_W, SBE_EVENT_OPTFIXED_H - 40);
	pos_y = SBE_SEARCH_BUTTON_Y - 40;
#endif

// SEARCH BUTTON
	search_obj = nftool_normal_button_create_type3(strButton[0], 203);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(search_obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(search_obj);
	nfui_nffixed_put((NFFIXED*)fixed1, search_obj, SBE_SEARCH_BUTTON_X, pos_y);
	nfui_regi_post_event_callback(search_obj, post_search_event_handler);

	log_ctx = scm_open_log_ctx();
	nfui_nfobject_set_data(search_obj, "log ctx", GSIZE_TO_POINTER(log_ctx));


//	PREVIEW
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PREVIEW", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(662));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, SBE_PREVIEW_LABEL_WIDTH, SBE_LABEL_HEIGHT2);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, SBE_PREVIEW_LABEL_X, SBE_PREVIEW_LABEL_Y);

	/**	TEMP **/
	preview_obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(665));
	nfui_nflabel_set_align((NFLABEL*)preview_obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(preview_obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(666));
	nfui_nfobject_set_size(preview_obj, SBE_PREVIEW_VIDEO_W, SBE_PREVIEW_VIDEO_H);
	nfui_nfobject_use_focus(preview_obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(preview_obj);
	nfui_nffixed_put((NFFIXED*)fixed1, preview_obj, SBE_PREVIEW_VIDEO_X, SBE_PREVIEW_VIDEO_Y);

	/**	TEMP **/
	play_time_obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MINI_SEMI_5), COLOR_IDX(664));
	nfui_nflabel_set_align((NFLABEL*)play_time_obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(play_time_obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(play_time_obj, SBE_PREVIEW_VIDEO_W, SBE_LABEL_HEIGHT2);
	nfui_nfobject_use_focus(play_time_obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(play_time_obj);
	nfui_nffixed_put((NFFIXED*)fixed1, play_time_obj, SBE_PREVIEW_TIME_X, SBE_PREVIEW_TIME_Y);

//	LOG DATA
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("TYPE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
	nfui_nfobject_set_size(obj, SBE_LOG_TYPE_W, SBE_LOGLABEL_HEIGHT);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, SBE_LOG_TYPE_X, SBE_LOG_Y);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("TIME", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
	nfui_nfobject_set_size(obj, SBE_LOG_TIME_W, SBE_LOGLABEL_HEIGHT);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, SBE_LOG_TIME_X, SBE_LOG_Y);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CONTENTS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
	nfui_nfobject_set_size(obj, SBE_LOG_CONTENTS_W, SBE_LOGLABEL_HEIGHT);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, SBE_LOG_CONTENTS_X, SBE_LOG_Y);

	for(i=0; i<SBE_LOG_LABEL_ROWS; i++)
	{
		obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
		nfui_nfobject_support_multi_lang(obj, TRUE);
		nfui_nflabel_set_drawing_outline((NFLABEL*)obj, FALSE);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
		nfui_nfobject_set_size(obj, SBE_LOG_TYPE_W, SBE_LOGLABEL_HEIGHT);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed2, obj, SBE_LOG_TYPE_X, SBE_LOG_Y+((i+1)*(SBE_LOGLABEL_HEIGHT+SBE_LOG_HEIGHT_GAP)));
		nfui_regi_pre_event_callback(obj, pre_log_label_event_handler);
		nfui_regi_post_event_callback(obj, post_log_label_event_handler);
		log_label[i][LOGLABEL_TYPE] = obj;

		obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_THIN));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
		nfui_nfobject_support_multi_lang(obj, FALSE);
		nfui_nflabel_set_drawing_outline((NFLABEL*)obj, FALSE);
		nfui_nfobject_set_size(obj, SBE_LOG_TIME_W, SBE_LOGLABEL_HEIGHT);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed2, obj, SBE_LOG_TIME_X, SBE_LOG_Y+((i+1)*(SBE_LOGLABEL_HEIGHT+SBE_LOG_HEIGHT_GAP)));
		nfui_regi_pre_event_callback(obj, pre_log_label_event_handler);
		nfui_regi_post_event_callback(obj, post_log_label_event_handler);
		log_label[i][LOGLABEL_TIME] = obj;

		obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_THIN));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
		nfui_nfobject_support_multi_lang(obj, FALSE);
		nfui_nflabel_set_drawing_outline((NFLABEL*)obj, FALSE);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
		nfui_nfobject_set_size(obj, SBE_LOG_CONTENTS_W, SBE_LOGLABEL_HEIGHT);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed2, obj, SBE_LOG_CONTENTS_X, SBE_LOG_Y+((i+1)*(SBE_LOGLABEL_HEIGHT+SBE_LOG_HEIGHT_GAP)));
		nfui_regi_pre_event_callback(obj, pre_log_label_event_handler);
		nfui_regi_post_event_callback(obj, post_log_label_event_handler);
		log_label[i][LOGLABEL_CONTENTS] = obj;
	}

//	LOG ORDER
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ORDER BY", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, SBE_LOG_ORDER_W, SBE_LABEL_HEIGHT1);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, SBE_LOG_ORDER_X, SBE_LOG_ORDER_Y);

	obj = nfui_combobox_new(strCombo, 2, 0);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, SBE_LOG_ORDER_CMB_W, SBE_LABEL_HEIGHT1);
	nfui_nfobject_disable(obj);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, SBE_LOG_ORDER_X+SBE_LOG_ORDER_W, SBE_LOG_ORDER_Y);
	nfui_regi_post_event_callback(obj, post_order_event_handler);
	order_obj = obj;

//	LOG DIR
	nfui_get_pixbuf_size(prev_img[0], &size_w, &size_h);

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), prev_img);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_disable(obj);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, SBE_LOG_PREV_BUTTON_X, SBE_LOG_PREV_BUTTON_Y);
	nfui_regi_post_event_callback(obj, post_prev_log_event_handler);
	prev_obj = obj;

	nfui_get_pixbuf_size(next_img[0], &size_w, &size_h);

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), next_img);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_disable(obj);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, SBE_LOG_NEXT_BUTTON_X, SBE_LOG_NEXT_BUTTON_Y);
	nfui_regi_post_event_callback(obj, post_next_log_event_handler);
	next_obj = obj;

// PLAYBACK / CLOSE BUTTON
	obj = nftool_normal_button_create_type1(strButton[3], MENU_BTN_WIDTH);
	nfui_nfobject_disable(obj);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_H_BTN_R2_X, MENU_H_BTN_Y);
	nfui_regi_post_event_callback(obj, post_playbutton_event_handler);
	playback_obj = obj;

	obj = nftool_normal_button_create_type2(strButton[4], MENU_BTN_WIDTH);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_H_BTN_R1_X, MENU_H_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

	nfui_regi_pre_event_callback(opt_fixed, pre_opt_fixed_event_handler);
	nfui_regi_pre_event_callback(fixed1, pre_fixed1_event_handler);
	nfui_regi_pre_event_callback(fixed2, pre_fixed2_event_handler);

	nfui_regi_post_event_callback(parent, post_page_event_handler);
}

gboolean vw_SearchByEvent_tab_out_handler()
{
	_preview_obj_off("");
	vsm_playback_preview_stop();

	return FALSE;
}

gboolean vw_SearchByEvent_tab_in_handler()
{
	g_init_sbe = TRUE;
	return FALSE;
}

gboolean vw_SearchByEvent_tab_show()
{
	vw_SearchByEvent_tab_in_handler();
	return FALSE;
}


gboolean vw_SearchByEvent_set_interval(time_t from_time, time_t to_time)
{
	GTimeVal tmp;
	memset(&tmp, 0x00, sizeof(GTimeVal));

	tmp.tv_sec = from_time;
	nfui_nftimelabel_set_datetime((NFTIMELABEL*)from_obj, &tmp);

	tmp.tv_sec = to_time;
	nfui_nftimelabel_set_datetime((NFTIMELABEL*)to_obj, &tmp);

	return FALSE;
}

