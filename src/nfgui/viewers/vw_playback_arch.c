
#include "nf_afx.h"

#include "nf_api_archive.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_font.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfbutton.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nfcheckbutton.h"
#include "objects/nflabel.h"

#include "vw_vkeyboard.h"

#include "vsm.h"
#include "vw_playback_arch.h"
#include "ix_mem.h"
#include "dtf.h"
#include "ssm.h"
#include "uxm.h"


#define PBA_WIDTH					(953)
#define PBA_HEIGHT					(624)
#define PBA_POS_X					((DISPLAY_ACTIVE_WIDTH - PBA_WIDTH)/4*2)
#define PBA_POS_Y					((DISPLAY_ACTIVE_HEIGHT - PBA_HEIGHT)/2)

#define PBA_UP_GAP					(64)
#define PBA_LEFT_GAP				(26)

#define PBA_LABEL_HEIGHT1			(40)
#define PBA_LABEL_HEIGHT2			(30)

#define PBA_BTN_WIDTH				(174)
#define PBA_BTN_GAP					(6)

#define MAX_TAG_STRING_SIZE			14
#define MAX_MEMO_STRING_SIZE		24

#if defined(GUI_4CH_SUPPORT)
#define PBA_AVI_CH_TABLE_ROW_MAX  1
#define PB_AUDIO_CH_MASK      (0x000F)
#elif defined(GUI_8CH_SUPPORT)
#define PBA_AVI_CH_TABLE_ROW_MAX  2
#define PB_AUDIO_CH_MASK      (0x00FF)
#elif defined(GUI_16CH_SUPPORT)
#define PBA_AVI_CH_TABLE_ROW_MAX  4
#define PB_AUDIO_CH_MASK      (0xFFFF)
#else 
#define PBA_AVI_CH_TABLE_ROW_MAX  8
#define PB_AUDIO_CH_MASK      (0xFFFFFFFF)
#endif


#define PBA_AVI_CH_TABLE_COL_MAX    8


////////////////////////////////////////////////////////////
//
// private data type 
//

enum {
	PA_BTN_RESERVE = 0,
	PA_BTN_CONTINUE,
	PA_BTN_START,
	PA_BTN_STOP,
	PA_BTN_CLOSE,
	PA_BTN_COUNT
};

enum {
	PBA_OPT_LOG,
	PBA_OPT_CODEC,
	PBA_OPT_POSLOG,

	PBA_OPT_CNT
};


typedef struct _PB_ARCH_T {
	BMKWND_E 	mode;
	gboolean 	is_queried;

	NF_ARCH_PB_AVI_PARAM 	param;
	NF_ARCH_AVI_INFO 		info;

	NFOBJECT *tag_obj;
	NFOBJECT *time_beg_obj;
	NFOBJECT *time_end_obj;	
	NFOBJECT *memo_obj;	
	NFOBJECT *size_obj;
	NFOBJECT *chk_ch_obj[32];
	NFOBJECT *chk_opt_obj[PBA_OPT_CNT];	
	NFOBJECT *btn[PA_BTN_COUNT];

	NFOBJECT *msg_box;

	guint ret_val;
} PB_ARCH_T;

static NFWINDOW *g_curwnd = 0;


////////////////////////////////////////////////////////////
//
// private variable
//

static PB_ARCH_T pb_arch;



////////////////////////////////////////////////////////////
//
// private functions
//

static int _update_tag(void)
{
	nfui_nflabel_set_text((NFLABEL*)pb_arch.tag_obj, pb_arch.info.tag);
	nfui_signal_emit(pb_arch.tag_obj, GDK_EXPOSE, TRUE);
}

static int _update_beg(void)
{
	GTimeVal cVal;
	gchar buf[23];

	GUINT64_TO_GTIMEVAL(pb_arch.info.time_beg, cVal);
	dtf_get_local_datetime(cVal.tv_sec, buf);
	nfui_nflabel_set_text((NFLABEL*)pb_arch.time_beg_obj, buf);	
	nfui_signal_emit(pb_arch.time_beg_obj, GDK_EXPOSE, TRUE);
}

static int _update_end(void)
{
	GTimeVal cVal;
	gchar buf[23];

	GUINT64_TO_GTIMEVAL(pb_arch.info.time_end, cVal);
	dtf_get_local_datetime(cVal.tv_sec, buf);
	nfui_nflabel_set_text((NFLABEL*)pb_arch.time_end_obj, buf);
	nfui_signal_emit(pb_arch.time_end_obj, GDK_EXPOSE, TRUE);
}

static int _update_memo(void)
{
	nfui_nflabel_set_text((NFLABEL*)pb_arch.memo_obj, pb_arch.info.memo);
	nfui_signal_emit(pb_arch.memo_obj, GDK_EXPOSE, TRUE);
}

static int _update_size(void)
{
	int i;
	int size_cnt = 0;
	gfloat total_size = 0;
	char buf[32];

	total_size = pb_arch.info.total_size;
	
	while((guint)total_size > 1024) {
		++size_cnt;
		total_size /= 1024;
	}

	memset(buf, 0x00, sizeof(buf));

	if(size_cnt == 0) g_sprintf(buf, "%d KB", 0);
	else if(size_cnt == 1) g_sprintf(buf, "%.1f MB", total_size);
	else if(size_cnt == 2) g_sprintf(buf, "%.1f GB", total_size);
	else if(size_cnt == 3) g_sprintf(buf, "%.1f TB", total_size);
	else g_sprintf(buf, "ERROR");

	nfui_nflabel_set_text((NFLABEL*)pb_arch.size_obj, buf);	
	nfui_signal_emit(pb_arch.size_obj, GDK_EXPOSE, TRUE);
}

static int _update_chk_ch(void)
{
	int cnt;

	for (cnt = 0; cnt < GUI_CHANNEL_CNT; cnt++)
	{
		if(pb_arch.info.channel_mask & (1 << cnt)) 
			nfui_check_button_set_active(NF_CHECKBUTTON(pb_arch.chk_ch_obj[cnt]), TRUE);
		else
			nfui_check_button_set_active(NF_CHECKBUTTON(pb_arch.chk_ch_obj[cnt]), FALSE);
					
		nfui_nfobject_disable(pb_arch.chk_ch_obj[cnt]);
		nfui_signal_emit(pb_arch.chk_ch_obj[cnt], GDK_EXPOSE, TRUE);
	}
}

static int _update_chk_opt(void)
{
	int cnt;
	gboolean tmp;

	for (cnt = 0; cnt < PBA_OPT_CNT; cnt++)
	{
		if (cnt == PBA_OPT_LOG)
		{
			if(pb_arch.info.extra_mask & NF_ARCH_EMASK_LOG) tmp = TRUE;
			else tmp = FALSE;
		}
		else if (cnt == PBA_OPT_CODEC)
		{
			if(pb_arch.info.extra_mask & NF_ARCH_EMASK_CODEC) tmp = TRUE;
			else tmp = FALSE;
		}
		else
		{
			if(pb_arch.info.extra_mask & NF_ARCH_EMASK_POS_LOG) tmp = TRUE;
			else tmp = FALSE;
		}
		
		nfui_check_button_set_active(NF_CHECKBUTTON(pb_arch.chk_opt_obj[cnt]), tmp);
		nfui_nfobject_disable(pb_arch.chk_opt_obj[cnt]);	
		nfui_signal_emit(pb_arch.chk_opt_obj[cnt], GDK_EXPOSE, TRUE);
	}
}

static int _update_btn(int result)
{
	int cnt;

	for (cnt = 0; cnt < PA_BTN_COUNT; cnt++)
	{
		switch (result) {
		case 0:
			if(cnt != PA_BTN_CLOSE)
				nfui_nfobject_disable(pb_arch.btn[cnt]);
			else
				nfui_nfobject_enable(pb_arch.btn[cnt]);			
			break;

		case 1:
			if(cnt != PA_BTN_CONTINUE && cnt != PA_BTN_STOP)
				nfui_nfobject_disable(pb_arch.btn[cnt]);
			else
				nfui_nfobject_enable(pb_arch.btn[cnt]);			
			break;

		case 2:
			if(cnt != PA_BTN_STOP)
				nfui_nfobject_disable(pb_arch.btn[cnt]);
			else
				nfui_nfobject_enable(pb_arch.btn[cnt]);			
			break;
		}
			
		nfui_signal_emit(pb_arch.btn[cnt], GDK_EXPOSE, TRUE);
	}
}

static int _update_ui(int result)
{
	_update_tag();
	_update_beg();
	_update_end();
	_update_memo();
	_update_size();
	_update_chk_ch();
	_update_chk_opt();
	_update_btn(result);	
	
	return 0;
}

static int _query_result_error(gint error)
{
	g_warning("%s %d : nf_arch_pb_query_avi returns FALSE\n", __FUNCTION__, __LINE__);

	switch (error) {
	case QRY_CODE_FULL_LIST:
		nftool_mbox(g_curwnd, "WARNING", "It's unable to start archiving\nbecause the reserved data is full\nor from/to time interval is too long.", NFTOOL_MB_OK);
		break;

	case QRY_CODE_FAIL_LOCK:
		nftool_mbox(g_curwnd, "WARNING", "Fail to reserve because of too many locked data.", NFTOOL_MB_OK);
		break;
		
	case QRY_CODE_INV_COMMAND:
	case QRY_CODE_INV_DEV:
	case QRY_CODE_INV_PARAM:
	case QRY_CODE_INV_MEDIA:
	case QRY_CODE_FAIL:
	case QRY_CODE_FAIL_WRITING:
		nftool_mbox(g_curwnd, "WARNING", "Fail to reserve as internal error.", NFTOOL_MB_OK);
		break;

	default:
		nftool_mbox(g_curwnd, "WARNING", "Unknown error.", NFTOOL_MB_OK);
		break;
	}

	/* set pb_arch.info data */
	strncpy(pb_arch.info.tag, pb_arch.param.tag, sizeof(pb_arch.param.tag));
	strncpy(pb_arch.info.memo, pb_arch.param.memo, sizeof(pb_arch.param.memo));

	pb_arch.info.channel_mask = pb_arch.param.ch_mask;
	pb_arch.info.time_beg = GTIMEVAL_TO_GUINT64(vsm_playback_get_playtime());
	pb_arch.info.time_end = GTIMEVAL_TO_GUINT64(vsm_playback_get_playtime());

	pb_arch.ret_val = PA_RET_STOP;
//	if (scm_is_bookmarking()) scm_exit_bookmark();

	_update_ui(0);

	return 0;
}

static gboolean returnkey_proc(NFOBJECT *top, GdkEvent *event, gpointer data)
{
	if(!nfui_nfobject_is_disabled(pb_arch.btn[PA_BTN_CLOSE])) 
	{
		if (scm_is_bookmarking()) 
			scm_exit_bookmark();
		
		if (pb_arch.ret_val == PA_RET_NONE) 
			pb_arch.ret_val = PA_RET_CLOSE;
		
		return TRUE;
	}
	else
		return FALSE;
}

static gboolean post_reserve_button_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	RSV_CODE_E ret_val = RSV_SUCCESS;

	if(evt->type == GDK_BUTTON_RELEASE) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		ret_val = scm_reserve_bookmark_info();
		switch (ret_val) {
		case RSV_SUCCESS:													// querying is not done
			nftool_mbox(g_curwnd, "NOTICE", "Data is reserved successfully.", NFTOOL_MB_OK);
			nfui_nfobject_disable(pb_arch.btn[PA_BTN_RESERVE]);
			nfui_signal_emit(pb_arch.btn[PA_BTN_RESERVE], GDK_EXPOSE, FALSE);			
			break;

		case RSV_CODE_INV_COMMAND:													// querying is not done
			nftool_mbox(g_curwnd, "NOTICE", "Querying is in progress.", NFTOOL_MB_OK);
			break;

		case RSV_CODE_FULL_LIST:													// List is full
			nftool_mbox(g_curwnd, "NOTICE", "It's unable to reserve the data\nbecause the archiving list is full.", NFTOOL_MB_OK);
			break;

		case RSV_CODE_FAIL_LOCK:
			nftool_mbox(g_curwnd, "NOTICE", "Fail to reserve because of too many locked data.", NFTOOL_MB_OK);
			break;

		case RSV_CODE_INV_DEV:
		case RSV_CODE_INV_PARAM:
		case RSV_CODE_INV_MEDIA:
		case RSV_CODE_FAIL:
			nftool_mbox(g_curwnd, "WARNING", "Fail to reserve as internal error.", NFTOOL_MB_OK);
			break;

		default:													// error
			nftool_mbox(g_curwnd, "WARNING", "Internal error.", NFTOOL_MB_OK);
			break;	
		}

		pb_arch.ret_val = PA_RET_RESERVE;
	}

	return FALSE;
}

static int _close_playback_arch_wnd()
{
	NFOBJECT *topwin;
	NFOBJECT *obj = pb_arch.btn[PA_BTN_CLOSE];

	if (pb_arch.ret_val == PA_RET_NONE) pb_arch.ret_val = PA_RET_CLOSE;
	topwin = nfui_nfobject_get_top(obj);
	nfui_nfobject_destroy(topwin);
	return;
}

static gboolean post_start_button_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint i;
	QRY_CODE_E ret_val = 0;
		
	if(evt->type == GDK_BUTTON_RELEASE) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		if(!strcmp(pb_arch.param.tag, "")) {
			nftool_mbox(g_curwnd, "NOTICE", "The tag field is empty.", NFTOOL_MB_OK);
			return FALSE;
		}

		if(pb_arch.param.ch_mask == 0) {
			nftool_mbox(g_curwnd, "NOTICE", "Please select a channel.", NFTOOL_MB_OK);
			return FALSE;
		}
		
		pb_arch.param.inc_text = 1;
		pb_arch.param.inc_ri = 0;
		pb_arch.param.inc_player = 1;
		ssm_get_cur_id(pb_arch.param.user);

		ret_val = scm_start_bookmark(pb_arch.param);

		switch (ret_val) {
		case QRY_SUCCESS: 
    		pb_arch.ret_val = PA_RET_START;
    		_close_playback_arch_wnd();		
            break;
		case QRY_CODE_FAIL_LOCK:
		case QRY_CODE_FULL_LIST:
			nftool_mbox(g_curwnd, "WARNING", "It's unable to start archiving\nbecause the archiving list is full.", NFTOOL_MB_OK);
			break;

		case QRY_CODE_INV_COMMAND:
		case QRY_CODE_INV_DEV:
		case QRY_CODE_INV_PARAM:
		case QRY_CODE_INV_MEDIA:
		case QRY_CODE_FAIL:
		case QRY_CODE_FAIL_WRITING:
			nftool_mbox(g_curwnd, "WARNING", "Fail to add bookmark as internal error.", NFTOOL_MB_OK);
			break;

		default:
			nftool_mbox(g_curwnd, "WARNING", "Unknown error.", NFTOOL_MB_OK);
			break;
		}
	}

	return FALSE;
}

static gboolean post_stop_button_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	QRY_CODE_E ret_val = 0;

	if(evt->type == GDK_BUTTON_RELEASE) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		ret_val = scm_stop_bookmark();

		switch (ret_val) {
		case QRY_SUCCESS: break;
		case QRY_CODE_FULL_LIST:
		case QRY_CODE_FAIL_LOCK:
			nftool_mbox(g_curwnd, "WARNING", "It's unable to start archiving\nbecause the archiving list is full.", NFTOOL_MB_OK);
			break;

		case QRY_CODE_INV_COMMAND:
		case QRY_CODE_INV_DEV:
		case QRY_CODE_INV_PARAM:
		case QRY_CODE_INV_MEDIA:
		case QRY_CODE_FAIL:
		case QRY_CODE_FAIL_WRITING:
			nftool_mbox(g_curwnd, "WARNING", "Fail to add bookmark as internal error.", NFTOOL_MB_OK);
			break;

		default:
			nftool_mbox(g_curwnd, "WARNING", "Unknown error.", NFTOOL_MB_OK);
			break;
		}


		nfui_nfobject_disable(pb_arch.btn[PA_BTN_CONTINUE]);
		nfui_nfobject_disable(pb_arch.btn[PA_BTN_STOP]);
		nfui_nfobject_enable(pb_arch.btn[PA_BTN_RESERVE]);
		nfui_nfobject_enable(pb_arch.btn[PA_BTN_CLOSE]);

		nfui_signal_emit(pb_arch.btn[PA_BTN_CONTINUE], GDK_EXPOSE, FALSE);
		nfui_signal_emit(pb_arch.btn[PA_BTN_STOP], GDK_EXPOSE, FALSE);
		nfui_signal_emit(pb_arch.btn[PA_BTN_RESERVE], GDK_EXPOSE, FALSE);
		nfui_signal_emit(pb_arch.btn[PA_BTN_CLOSE], GDK_EXPOSE, FALSE);

		pb_arch.ret_val = PA_RET_STOP;
	}

	return FALSE;
}

static gboolean post_cont_button_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
		  return FALSE;
	       }

		pb_arch.is_queried = FALSE;
		pb_arch.ret_val = PA_RET_CONTINUE;

		scm_resume_bookmark();
		_close_playback_arch_wnd();
	}

	return FALSE;
}

static gboolean post_close_button_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) {
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
		if (scm_is_bookmarking()) scm_exit_bookmark();

		_close_playback_arch_wnd();
	}

	return FALSE;
}

static gboolean post_tag_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER) {
		NFOBJECT *top;
		gchar *subject = NULL;
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
	  	   	if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			  return FALSE;
	  	   	}
		
			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += ((GdkEventButton*)evt)->x;
			y += ((GdkEventButton*)evt)->y;
		}
		
		subject = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), x, y, MAX_TAG_STRING_SIZE, VKEY_ALPHANUMERIC);

		if(subject) {
			g_stpcpy(pb_arch.param.tag, subject);

			if (strlen(subject) > 0) {
				nfui_nfobject_enable(pb_arch.btn[PA_BTN_START]);
				nfui_signal_emit(pb_arch.btn[PA_BTN_START], GDK_EXPOSE, TRUE);
			}
			else {
				nfui_nfobject_disable(pb_arch.btn[PA_BTN_START]);
				nfui_signal_emit(pb_arch.btn[PA_BTN_START], GDK_EXPOSE, TRUE);
			}

			nfui_nflabel_set_text((NFLABEL*)obj, subject);
			nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

			ifree(subject);
			subject = NULL;
		}
	}

	return FALSE;
}


static gboolean post_memo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER) {
		NFOBJECT *top;
		gchar *memo = NULL;
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
	  	   	if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			  return FALSE;
	  	   	}
		
			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += ((GdkEventButton*)evt)->x;
			y += ((GdkEventButton*)evt)->y;
		}
		
		memo = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), x, y, MAX_MEMO_STRING_SIZE, VKEY_NORMAL);

		if(memo) {
			g_stpcpy(pb_arch.param.memo, memo);

			nfui_nflabel_set_text((NFLABEL*)obj, memo);
			nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

			ifree(memo);
			memo = NULL;
		}
	}

	return FALSE;
}

static gboolean post_channel_button_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint ch = 0;

	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED) {
		ch = (gint)nfui_nfobject_get_data(obj, "channel");

		if(nfui_check_button_get_active(NF_CHECKBUTTON(obj))) {
			pb_arch.param.ch_mask |= (1 << ch);
			pb_arch.param.audio_mask |= (1 << ch);
		}else {
			pb_arch.param.ch_mask &= ~(1 << ch);
			pb_arch.param.audio_mask &= ~(1 << ch);
		}
	}

	return FALSE;
}


static gboolean post_log_button_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED) {
		if(nfui_check_button_get_active(NF_CHECKBUTTON(obj))) 
			pb_arch.param.inc_log = 1;
		else
			pb_arch.param.inc_log = 0;
	}

	return FALSE;
}

static gboolean post_codec_button_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED) {
		if(nfui_check_button_get_active(NF_CHECKBUTTON(obj))) 
			pb_arch.param.inc_codec = 1;
		else
			pb_arch.param.inc_codec = 0;
	}

	return FALSE;
}

static gboolean post_pos_button_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED) {
		if(nfui_check_button_get_active(NF_CHECKBUTTON(obj))) 
			pb_arch.param.inc_pos_log = 1;
		else
			pb_arch.param.inc_pos_log = 0;
	}

	return FALSE;
}

static void vw_playback_arch_init()
{
	if(pb_arch.ret_val != PA_RET_NONE)
		pb_arch.ret_val = PA_RET_NONE;		

	pb_arch.is_queried = FALSE;
	
	memset(&pb_arch.param, 0x00, sizeof(NF_ARCH_PB_AVI_PARAM));
	pb_arch.param.audio_mask |= PB_AUDIO_CH_MASK;
}

static void vw_playback_arch_get_query_result(void)
{
	if (pb_arch.is_queried == TRUE) return;

	pb_arch.is_queried = TRUE;
	memset(&pb_arch.info, 0x00, sizeof(NF_ARCH_AVI_INFO));
	scm_request_bookmark_info();
}

static gboolean pre_aiWin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE) {
		if(pb_arch.ret_val == PA_RET_RESERVE || 
				pb_arch.ret_val == PA_RET_STOP) {

			if (scm_is_bookmarking()) scm_exit_bookmark();
		}
	}

	return FALSE;
}

static gboolean post_aiWin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE) {
		g_curwnd = 0;
		gtk_main_quit();		
	}

	return FALSE;
}

static gboolean post_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;
	CMM_MESSAGE_T *pmsg;
	NF_ARCH_AVI_INFO *avi;
	int err_code;
	char tibuf[128];
	char msgbuf[1024];
	GTimeVal val;
	
	switch(event->type) {
	case GDK_EXPOSE:
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
    
		nfui_nfobject_gc_unref(gc);

		if (pb_arch.mode == STOP) vw_playback_arch_get_query_result();
		break;

	case GDK_BUTTON_RELEASE:
		break;					

	case GDK_DELETE:
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
	
		uxm_unreg_imsg_event(obj, INFY_QUERY_ERROR);
		uxm_unreg_imsg_event(obj, INFY_QUERY_SUCCESS);
		uxm_unreg_imsg_event(obj, INFY_QUERY_OVER);
		uxm_unreg_imsg_event(obj, INFY_QUERY_NO_VIDEODATA);
		break;

	case INFY_QUERY_ERROR:
		pmsg = (CMM_MESSAGE_T *)data;
		err_code = (int)pmsg->data;
		_query_result_error(err_code);
		break;	

	case INFY_QUERY_NO_VIDEODATA:
		pmsg = (CMM_MESSAGE_T *)data;
		nftool_mbox(g_curwnd, "WARNING", "There is no video data.", NFTOOL_MB_OK);
		pb_arch.ret_val = PA_RET_STOP;
		_update_btn(0);	
		break;

	case INFY_QUERY_OVER:
		pmsg = (CMM_MESSAGE_T *)data;
		memcpy(&pb_arch.info, pmsg->data, sizeof(NF_ARCH_AVI_INFO));
		GUINT64_TO_GTIMEVAL(pb_arch.info.time_end, val);
		dtf_get_local_datetime(val.tv_sec, tibuf);
		g_sprintf(msgbuf, lookup_string("Bookmark operation is completed.\nThe end time is changed to %s,\nbecause limited size of 20GB has been exceeded."), tibuf);
		nftool_mbox(g_curwnd, "WARNING", msgbuf, NFTOOL_MB_OK);
		_update_ui(2);
		break;

	case INFY_QUERY_SUCCESS:
		pmsg = (CMM_MESSAGE_T *)data;
		memcpy(&pb_arch.info, pmsg->data, sizeof(NF_ARCH_AVI_INFO));
		_update_ui(1);
		break;
	}

	return FALSE;

}


////////////////////////////////////////////////////////////
//
// public interfaces
//

guint VW_PlaybackArch_Open(NFWINDOW *parent, BMKWND_E mode)
{
    NFOBJECT *obj[PBA_OPT_CNT];
	NFOBJECT *pbaWin;
	NFOBJECT *main_fixed;
	NFOBJECT *pba_table1;
	NFOBJECT *pba_table2;
	NFOBJECT *lbTemp;
	NFOBJECT *fixed_temp;
	
	gchar *strTitle1[] = {"TAG", "FROM", "TO", "MEMO", "INFO"};
	gchar *strTitle2[] = {"LOG", "POS"};
	gchar *strBtn[] = {"RESERVE", "CONTINUE", "START", "STOP", "CLOSE"};	
	gchar *strCh[GUI_CHANNEL_CNT] = {NULL,};

	guint t_width1[]	= {30, 60, 30, 60, 30, 60, 30, 60};
	guint t_width2[]	= {30, 120, 30, 120, 30, 120};

	guint pos_x, pos_y;
	gint btn_w, btn_h;
	gint i, j, k;

	NFTEXTBOX_TYPE label_skin_type;

	make_channel_string(strCh, FALSE);

	pb_arch.mode = mode;
	
	if (pb_arch.mode == START)
		vw_playback_arch_init();

/* window */
	pbaWin = (NFOBJECT*)nfui_nfwindow_new(parent, (guint)PBA_POS_X, (guint)PBA_POS_Y, (guint)PBA_WIDTH, (guint)PBA_HEIGHT);
	g_curwnd = pbaWin;
	nfui_nfwindow_set_title(pbaWin, "BOOKMARK");
	nfui_regi_pre_event_callback(pbaWin, pre_aiWin_event_handler);
	nfui_regi_post_event_callback(pbaWin, post_aiWin_event_handler);
	nfui_nfwindow_set_returnkey_proc(pbaWin, returnkey_proc);
	
	main_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(main_fixed, (guint)PBA_WIDTH, (guint)PBA_HEIGHT);
	nfui_regi_post_event_callback(main_fixed, post_main_fixed_event_handler);
	nfui_nfobject_show(main_fixed);

/* title */
	lbTemp = (NFOBJECT*)nfui_nflabel_new_with_pango_font("BOOKMARK SETUP", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nflabel_set_align((NFLABEL*)lbTemp, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(lbTemp, 300, 48);
	nfui_nfobject_use_focus(lbTemp, NFOBJECT_FOCUS_OFF);
	nfui_nffixed_put((NFFIXED*)main_fixed, lbTemp, 20, 2);
	//nfui_nffixed_put((NFFIXED*)main_fixed, lbTemp, (PBA_WIDTH-300)/2, 8);
	nfui_nfobject_show(lbTemp);


	if (mode == START)
		label_skin_type = NFTEXTBOX_TYPE_POPUP_INPUT;
	else
		label_skin_type = NFTEXTBOX_TYPE_POPUP_OUTPUT;

/* label */
	pos_x = PBA_LEFT_GAP;
	pos_y = PBA_UP_GAP;
	
	for(i = 0; i < 5; i++) {
		lbTemp = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle1[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
		nfui_nflabel_set_align((NFLABEL*)lbTemp, NFALIGN_LEFT, 0);
		nfui_nfobject_set_size(lbTemp, 96, PBA_LABEL_HEIGHT1);
		nfui_nfobject_use_focus(lbTemp, NFOBJECT_FOCUS_OFF);
		nfui_nffixed_put((NFFIXED*)main_fixed, lbTemp, pos_x, pos_y);
		nfui_nfobject_show(lbTemp);

		switch(i) {
			case 0:
			{
				pb_arch.tag_obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
				nfui_nflabel_set_skin_type((NFLABEL*)pb_arch.tag_obj, label_skin_type);	
				nfui_nflabel_set_spacing((NFLABEL *)pb_arch.tag_obj, CONDENSED_SPACING);
				if (mode == STOP) nfui_nfobject_use_focus(pb_arch.tag_obj, NFOBJECT_FOCUS_OFF);
				nfui_nfobject_support_multi_lang((NFOBJECT*)pb_arch.tag_obj, FALSE);
				nfui_nflabel_use_pango_cashing((NFLABEL*)pb_arch.tag_obj, 0, NULL);
				nfui_nflabel_set_align((NFLABEL*)pb_arch.tag_obj, NFALIGN_LEFT, 10);
				nfui_nfobject_set_size(pb_arch.tag_obj, 427, PBA_LABEL_HEIGHT1);
				nfui_nffixed_put((NFFIXED*)main_fixed, pb_arch.tag_obj, pos_x + 106, pos_y);
				nfui_nfobject_show(pb_arch.tag_obj);
				nfui_regi_post_event_callback(pb_arch.tag_obj, post_tag_event_handler);
			}
			break;
			
			case 1:
			{
				pb_arch.time_beg_obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));		
				nfui_nflabel_set_skin_type((NFLABEL*)pb_arch.time_beg_obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);			
				nfui_nfobject_use_focus(pb_arch.time_beg_obj, NFOBJECT_FOCUS_OFF);
				nfui_nflabel_use_pango_cashing((NFLABEL*)pb_arch.time_beg_obj, 0, NULL);
				nfui_nflabel_set_align((NFLABEL*)pb_arch.time_beg_obj, NFALIGN_LEFT, 10);
				nfui_nfobject_set_size(pb_arch.time_beg_obj, 427, PBA_LABEL_HEIGHT1);
				nfui_nffixed_put((NFFIXED*)main_fixed, pb_arch.time_beg_obj, pos_x + 106, pos_y);
				nfui_nfobject_show(pb_arch.time_beg_obj);
			}
			break;
			
			case 2:
			{
				pb_arch.time_end_obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));	
				nfui_nflabel_set_skin_type((NFLABEL*)pb_arch.time_end_obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);				
				nfui_nfobject_use_focus(pb_arch.time_end_obj, NFOBJECT_FOCUS_OFF);
				nfui_nflabel_use_pango_cashing((NFLABEL*)pb_arch.time_end_obj, 0, NULL);
				nfui_nflabel_set_align((NFLABEL*)pb_arch.time_end_obj, NFALIGN_LEFT, 10);
				nfui_nfobject_set_size(pb_arch.time_end_obj, 427, PBA_LABEL_HEIGHT1);
				nfui_nffixed_put((NFFIXED*)main_fixed, pb_arch.time_end_obj, pos_x + 106, pos_y);
				nfui_nfobject_show(pb_arch.time_end_obj);
			}
			break;
			
			case 3:
			{
				pb_arch.memo_obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
				nfui_nflabel_set_skin_type((NFLABEL*)pb_arch.memo_obj, label_skin_type);	
				if (mode == STOP) nfui_nfobject_use_focus(pb_arch.memo_obj, NFOBJECT_FOCUS_OFF);				
				nfui_nfobject_support_multi_lang((NFOBJECT*)pb_arch.memo_obj, FALSE);
				nfui_nflabel_set_multi_line_type((NFLABEL*)pb_arch.memo_obj, TRUE);
				nfui_nflabel_use_pango_cashing((NFLABEL*)pb_arch.memo_obj, 0, NULL);
				nfui_nflabel_set_align((NFLABEL*)pb_arch.memo_obj, NFALIGN_LEFT, 10);
				nfui_nfobject_set_size(pb_arch.memo_obj, 427, PBA_LABEL_HEIGHT2*5+10);
				nfui_nffixed_put((NFFIXED*)main_fixed, pb_arch.memo_obj, pos_x + 106, pos_y);
				nfui_nfobject_show(pb_arch.memo_obj);
				nfui_regi_post_event_callback(pb_arch.memo_obj, post_memo_event_handler);
			}
			break;
			
			case 4:
			{
				pb_arch.size_obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
				nfui_nflabel_set_skin_type((NFLABEL*)pb_arch.size_obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);			
				nfui_nfobject_use_focus(pb_arch.size_obj, NFOBJECT_FOCUS_OFF);
				nfui_nflabel_use_pango_cashing((NFLABEL*)pb_arch.size_obj, 0, NULL);
				nfui_nflabel_set_align((NFLABEL*)pb_arch.size_obj, NFALIGN_LEFT, 10);
				nfui_nfobject_set_size(pb_arch.size_obj, 427, PBA_LABEL_HEIGHT2*5+10);
				nfui_nffixed_put((NFFIXED*)main_fixed, pb_arch.size_obj, pos_x + 106, pos_y);
				nfui_nfobject_show(pb_arch.size_obj);				
			}
			break;
		}

		if(i >= 3) 
			pos_y += (PBA_LABEL_HEIGHT2*5+10);
		else 
			pos_y += PBA_LABEL_HEIGHT1;

		pos_y += 3;
	}

		
	pos_x = 596;
	pos_y = PBA_UP_GAP;

	lbTemp = (NFOBJECT*)nfui_nflabel_new_with_pango_font("AVI CHANNEL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)lbTemp, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(lbTemp, 270, PBA_LABEL_HEIGHT1);
	nfui_nfobject_use_focus(lbTemp, NFOBJECT_FOCUS_OFF);
	nfui_nffixed_put((NFFIXED*)main_fixed, lbTemp, pos_x, pos_y);
	nfui_nfobject_show(lbTemp);


/* table */
	pos_x = 596;
	pos_y = PBA_UP_GAP + 43;


	pba_table1 = (NFOBJECT*)nfui_nftable_new(PBA_AVI_CH_TABLE_COL_MAX, PBA_AVI_CH_TABLE_ROW_MAX, 0, 0, t_width1, PBA_LABEL_HEIGHT1);
	nfui_nfobject_show(pba_table1);
	nfui_nffixed_put((NFFIXED*)main_fixed, pba_table1, pos_x, pos_y); 

	pos_y += (PBA_LABEL_HEIGHT1 * PBA_AVI_CH_TABLE_ROW_MAX + 40);
	
	pba_table2 = (NFOBJECT*)nfui_nftable_new(6, 2, 0, 0, t_width2, PBA_LABEL_HEIGHT1);
	nfui_nfobject_show(pba_table2);
	nfui_nffixed_put((NFFIXED*)main_fixed, pba_table2, pos_x, pos_y); 

/* checkbutton in table */
	for(i = 0, k = 0; i < PBA_AVI_CH_TABLE_ROW_MAX; i++) {
		for(j = 0; j < 8; j+=2) {
			fixed_temp = (NFOBJECT*)nfui_nffixed_new();
			nfui_nfobject_set_size(fixed_temp, t_width1[0], PBA_LABEL_HEIGHT1);
			nfui_nfobject_show(fixed_temp);
		
			pb_arch.chk_ch_obj[k] = nfui_checkbutton_new(TRUE);
			nfui_nfobject_set_data(pb_arch.chk_ch_obj[k], "channel", GINT_TO_POINTER(k));
			pb_arch.param.ch_mask |= (1 << k);
			nfui_check_button_set_skin_type(NF_CHECKBUTTON(pb_arch.chk_ch_obj[k]), NFCHECK_TYPE_POPUP_NORMAL);
			nfui_check_get_size(pb_arch.chk_ch_obj[k], &btn_w, &btn_h);
			nfui_regi_post_event_callback(pb_arch.chk_ch_obj[k], post_channel_button_handler);
			nfui_nfobject_show(pb_arch.chk_ch_obj[k]);
			nfui_nffixed_put((NFFIXED*)fixed_temp, pb_arch.chk_ch_obj[k], (t_width1[0]-btn_w)/2, (PBA_LABEL_HEIGHT1-btn_h)/2);		

			nfui_nftable_attach((NFTABLE*)pba_table1, 	fixed_temp, (guint)j, (guint)i);

			k++;
		}
	}
	
/* label in table*/
	for(i = 0, k = 0; i < PBA_AVI_CH_TABLE_ROW_MAX; i++) {
		for(j = 1; j < 8; j+=2) {
			lbTemp = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strCh[k++], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(294));
			nfui_nflabel_set_spacing((NFLABEL *)lbTemp, SEMI_CONDENSED_SPACING);
			nfui_nflabel_set_align((NFLABEL*)lbTemp, NFALIGN_LEFT, 0);
			nfui_nfobject_use_focus(lbTemp, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(lbTemp);
			nfui_nftable_attach((NFTABLE*)pba_table1, lbTemp, (guint)j, (guint)i);			
		}
	}
	free_channel_string(strCh);

/* log/pos */
	for(i = 0; i < PBA_OPT_CNT; i++) 
	{
		fixed_temp = (NFOBJECT*)nfui_nffixed_new();
		nfui_nfobject_set_size(fixed_temp, t_width2[0], PBA_LABEL_HEIGHT1);
		nfui_nfobject_show(fixed_temp);
	
		pb_arch.chk_opt_obj[i] = nfui_checkbutton_new(TRUE);
		nfui_check_button_set_skin_type(NF_CHECKBUTTON(pb_arch.chk_opt_obj[i]), NFCHECK_TYPE_POPUP_NORMAL);
		nfui_check_get_size(pb_arch.chk_opt_obj[i], &btn_w, &btn_h);
		nfui_nfobject_show(pb_arch.chk_opt_obj[i]);
		nfui_nffixed_put((NFFIXED*)fixed_temp, pb_arch.chk_opt_obj[i], (t_width2[0]-btn_w)/2, (PBA_LABEL_HEIGHT1-btn_h)/2);		

		if (i == PBA_OPT_LOG) {
			nfui_nftable_attach((NFTABLE*)pba_table2, fixed_temp, 0, 0);
			nfui_regi_post_event_callback(pb_arch.chk_opt_obj[i], post_log_button_handler);
			pb_arch.param.inc_log = 1;
		}
		else if (i == PBA_OPT_CODEC) {
			nfui_nftable_attach((NFTABLE*)pba_table2, fixed_temp, 2, 0);
			nfui_regi_post_event_callback(pb_arch.chk_opt_obj[i], post_codec_button_handler);
			pb_arch.param.inc_codec = 1;
		}
		else if (i == PBA_OPT_POSLOG) {
			nfui_nftable_attach((NFTABLE*)pba_table2, fixed_temp, 0, 1);
			nfui_regi_post_event_callback(pb_arch.chk_opt_obj[i], post_pos_button_handler);
			pb_arch.param.inc_pos_log = 1;
		}

#ifdef _SUPPORT_ARCH_POS_LOG
        if (i == PBA_OPT_POSLOG) {
            if (var_get_vendor_code() == 65) {
                nfui_nfobject_hide(fixed_temp);
    			pb_arch.param.inc_pos_log = 0;
            }
        }
#endif        
	}

	for(i = 0; i < PBA_OPT_CNT; i++)
	{
		if (i == PBA_OPT_LOG)
			obj[i] = (NFOBJECT*)nfui_nflabel_new_with_pango_font("LOG", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(294));
		else if (i == PBA_OPT_CODEC)
			obj[i] = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CODEC", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(294));
		else
			obj[i] = (NFOBJECT*)nfui_nflabel_new_with_pango_font("POS LOG", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(294));
		
		nfui_nflabel_set_spacing((NFLABEL *)obj[i], SEMI_CONDENSED_SPACING);		
		nfui_nflabel_set_align((NFLABEL*)obj[i], NFALIGN_LEFT, 0);
		if (i == PBA_OPT_LOG) nfui_nfobject_set_size(obj[i], 60, PBA_LABEL_HEIGHT1);
		else if (i == PBA_OPT_CODEC) nfui_nfobject_set_size(obj[i], 120, PBA_LABEL_HEIGHT1);

		nfui_nfobject_use_focus(obj[i], NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj[i]);

		if (i == PBA_OPT_LOG)
			nfui_nftable_attach((NFTABLE*)pba_table2, obj[i], 1, 0);
		else if (i == PBA_OPT_CODEC)
			nfui_nftable_attach((NFTABLE*)pba_table2, obj[i], 3, 0);
		else if (i == PBA_OPT_POSLOG)
			nfui_nftable_attach((NFTABLE*)pba_table2, obj[i], 1, 1);

#ifdef _SUPPORT_ARCH_POS_LOG
        if (i == PBA_OPT_POSLOG) {
            if (var_get_vendor_code() == 65) {
                nfui_nfobject_hide(lbTemp);
            }
        }
#endif
	}

    if (!ivsc.dfunc.support_posevent)
    {
        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)pb_arch.chk_opt_obj[PBA_OPT_POSLOG], FALSE);
		nfui_nfobject_hide(pb_arch.chk_opt_obj[PBA_OPT_POSLOG]);
		nfui_nfobject_hide(obj[PBA_OPT_POSLOG]);
		pb_arch.param.inc_pos_log = 0;
    }

/* button */
	pos_x = PBA_LEFT_GAP;
	pos_y = PBA_HEIGHT - 74;

	guint query_result = 1;

	for(i=0; i<PA_BTN_COUNT; i++) {
		if (i == PA_BTN_CLOSE)
			pb_arch.btn[i] = nftool_normal_button_create_type2(strBtn[i], PBA_BTN_WIDTH);
		else		
			pb_arch.btn[i] = nftool_normal_button_create_type1(strBtn[i], PBA_BTN_WIDTH);
		
		nfui_nfobject_disable(pb_arch.btn[i]);
		nfui_nfobject_show(pb_arch.btn[i]);
		nfui_nffixed_put((NFFIXED*)main_fixed, pb_arch.btn[i], pos_x, pos_y);
		
		pos_x += (PBA_BTN_WIDTH+PBA_BTN_GAP);
	}


	nfui_regi_post_event_callback(pb_arch.btn[PA_BTN_RESERVE], post_reserve_button_handler);
	nfui_regi_post_event_callback(pb_arch.btn[PA_BTN_CONTINUE], post_cont_button_handler);
	nfui_regi_post_event_callback(pb_arch.btn[PA_BTN_START], post_start_button_handler);
	nfui_regi_post_event_callback(pb_arch.btn[PA_BTN_STOP], post_stop_button_handler);
	nfui_regi_post_event_callback(pb_arch.btn[PA_BTN_CLOSE], post_close_button_handler);

	nfui_nfwindow_add((NFWINDOW*)pbaWin, main_fixed);
	nfui_run_main_event_handler(pbaWin);
	nfui_nfobject_show(pbaWin);

	if (mode == STOP)
	{
		pb_arch.ret_val = PA_RET_CONTINUE;		// default value
		scm_pause_bookmark();
		uxm_reg_imsg_event(main_fixed, INFY_QUERY_ERROR);
		uxm_reg_imsg_event(main_fixed, INFY_QUERY_SUCCESS);
		uxm_reg_imsg_event(main_fixed, INFY_QUERY_OVER);
		uxm_reg_imsg_event(main_fixed, INFY_QUERY_NO_VIDEODATA);
	}
	else {
		pb_arch.ret_val = PA_RET_CLOSE;			// default value
		nfui_nfobject_enable(pb_arch.btn[PA_BTN_CLOSE]);
	}


/* set for key navi */
	nfui_make_key_hierarchy((NFWINDOW*)pbaWin);
	nfui_set_key_focus(pb_arch.btn[PA_BTN_START], TRUE);

	nfui_page_close(PGID_POPUPWND, pbaWin);
	nfui_page_open(PGID_SEARCH_ARCH, pbaWin, nfui_get_last_user());

	gtk_main();

	nfui_page_close(PGID_SEARCH_ARCH, pbaWin);

	return pb_arch.ret_val;
}

void VW_PlaybackArch_Close()
{
	pb_arch.ret_val = PA_RET_CLOSE;
	if (g_curwnd) nfui_nfobject_destroy(g_curwnd);
}
