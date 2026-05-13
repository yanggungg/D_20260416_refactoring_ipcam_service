
/*
 *  scm_arch.c
 *
 *  written by seongho
 *
 */


#include <string.h>

#include "iux_afx.h"

#include "scm.h"
#include "ix_mem.h"
#include "ix_func.h"
#include "evt.h"
#include "qry.h"
#include "mda.h"
#include "wrk.h"
#include "libarch.h"
#include "scm_internal.h"

#define DBG_LEVEL		9
#define DBG_MODULE		"SCM_ARCH"

#define ARCH_DEV_MAX_CNT			4
#define NA				1

/////////////////////////////////////////////////////////////
//
// private data types
//

typedef struct _ARCH_BURN_D {
    CALLID              caller;
	GThread				*burn_t;

	NF_ARCH_PROGRESS 	prog_info;

	int 				status;
	int 				mode;
	unsigned int		prog_rate;
}ARCH_BURN_D;

enum {
	BURN_MODE_NONE = 0,
	BURN_MODE_START,
	BURN_MODE_WAIT_RESTART,
	BURN_MODE_EXIT
};

enum {
	NOTHING = 0,
	QUERYING_STATUS,
	ERASING_STATUS,
	EXTRACTING_STATUS,
	BURNING_STATUS,
	WRITING_STATUS,
	SENDING_STATUS,
};


/////////////////////////////////////////////////////////////
//
// private variables
//

static ARCH_BURN_D	g_burn_d;



/////////////////////////////////////////////////////////////
//
// private functions
//


static /*inline*/ void start_worker_t()
{
	g_burn_d.mode = BURN_MODE_START;
}

static /*inline*/ int is_running_worker_t()
{
	return (g_burn_d.burn_t != NULL ? 1 : 0);
}

static /*inline*/ void exit_worker_t()
{
	g_burn_d.mode = BURN_MODE_EXIT;
}

static void init_burn_d()
{
	memset(&g_burn_d, 0x00, sizeof(ARCH_BURN_D));

	g_burn_d.status = NA;
	g_burn_d.mode = BURN_MODE_NONE;
}

static int _send_to_viewer(IMSG msg, unsigned int param, int status)
{
	switch (msg) {
	case INFY_BURN_QUERYING:
	case INFY_BURN_ERASING:
	case INFY_BURN_EXTRACTING:
	case INFY_BURN_PROG:
		DMSG(1, "msg:%x, status:%x\n", msg, status);
		_scm_send_data_to_viewer(msg, param, status);
		break;
	case INFY_BURN_ERROR:
	case INFY_BURN_SUCCESS:
	case INFY_BURN_CANCEL:
		DMSG(1, "msg:%x, status:%x\n", msg, status);
		_scm_send_data_to_viewer(msg, param, status);
    	if (g_burn_d.caller == LOCAL_CALL) _scm_send_msg_to_viewer(INFY_ARCH_BURN_API_CMPL, 0);
    	else if (g_burn_d.caller == WEB_CALL) _scm_send_msg_to_viewer(INFY_ARCH_BURN_API_CMPL, 1);
		break;

	default: break;
	}

	return 0;
}

static void burn_status_cb(int result, void *context)
{
	DMSG(9, "callback result = %d\n", result);
	g_burn_d.status = result;
}

static void burn_cancel_cb(int result, void *context)
{
	g_burn_d.status = result;

	switch (result) {
	case BRN_SUCCESS:
		_send_to_viewer(INFY_BURN_CANCEL, 0, GINT_TO_POINTER(g_burn_d.status));
		break;

	default:
		_send_to_viewer(INFY_BURN_ERROR, 0, GINT_TO_POINTER(g_burn_d.status));
	}

	exit_worker_t();
}

static int check_burning_progress()
{
	if (nf_arch_get_progress(&g_burn_d.prog_info)) {
		if (g_burn_d.prog_info.total > 0) {
			g_burn_d.prog_rate = ((g_burn_d.prog_info.current * 100) / g_burn_d.prog_info.total);
			DMSG(1, "status : %d, progress rate : %d  %d\n",
				g_burn_d.prog_info.status,
				g_burn_d.prog_info.current * 100 / g_burn_d.prog_info.total,
				g_burn_d.prog_rate);
		}

		switch(g_burn_d.prog_info.status) {
		case QUERYING_STATUS:
			_send_to_viewer(INFY_BURN_QUERYING, 0, GUINT_TO_POINTER(g_burn_d.prog_rate));
			break;

		case ERASING_STATUS:
			_send_to_viewer(INFY_BURN_ERASING, 0, GUINT_TO_POINTER(g_burn_d.prog_rate));
			break;

		case EXTRACTING_STATUS:
			_send_to_viewer(INFY_BURN_EXTRACTING, 0, GUINT_TO_POINTER(g_burn_d.prog_rate));
			break;

		case BURNING_STATUS:
		case WRITING_STATUS:
		case SENDING_STATUS:
			if (mda_is_loosened()) mda_tighten();
			_send_to_viewer(INFY_BURN_PROG, 0, GUINT_TO_POINTER(g_burn_d.prog_rate));
			break;
		}

		return 0;
	}

	return -1;
}

static int check_burning_result()
{
	if (g_burn_d.mode == BURN_MODE_EXIT) {
		nf_arch_burn_end(NULL);
		DMSG(1, "");
		return 0;
	}

	DMSG(1, "burning result = %d\n", g_burn_d.status);
	switch(g_burn_d.status) {
		case BRN_CODE_NEXT_MEDIA:
			_send_to_viewer(INFY_BURN_ERROR, 0, GINT_TO_POINTER(g_burn_d.status));
			return 0;

		case BRN_CODE_INV_COMMAND:
		case BRN_CODE_INV_PARAM:
		case BRN_CODE_FAIL:
		case BRN_CODE_FAIL_WRITING:
		case BRN_CODE_CANCELED:
		case BRN_CODE_INV_DEV:
		case BRN_CODE_INV_MEDIA:
		case BRN_CODE_NOTERASABLE_MEDIA:
		case BRN_CODE_FULL_MEDIA:
		case BRN_CODE_FTP_AUTH:
		case BRN_CODE_FAIL_MULTISESSION:
		case BRN_CODE_NOTSUPP_MULTISESSION:
		case BRN_CODE_FTP_CONN:
		case BRN_CODE_FTP_FAIL:
		case BRN_CODE_UNKNOWN_BUG:
			_send_to_viewer(INFY_BURN_ERROR, 0, GINT_TO_POINTER(g_burn_d.status));
			nf_arch_burn_end(NULL);
			DMSG(1, "");
			return 0;

		case BRN_SUCCESS:
			_send_to_viewer(INFY_BURN_SUCCESS, 0, GINT_TO_POINTER(g_burn_d.status));
			DMSG(1, "");
			nf_arch_burn_end(NULL);
			return 0;

		default:
			DMSG(1, "waiting.... callback...");
			return -1;
			break;
	}

	return -1;
}

static int cleanup_burn_worker_t()
{
	if (mda_is_loosened()) mda_tighten();
	if (cmm_mount_off_thread(g_burn_d.burn_t) == -1)  return -1;
	g_burn_d.burn_t = NULL;
	return 0;
}

static void* burning_run(void *arg)
{
	while(1) {
		g_usleep(1000000);
		if (check_burning_progress() == 0) continue;
		if (check_burning_result() == 0) break;
	}

	cleanup_burn_worker_t();
	g_thread_exit(NULL);
	return NULL;
}

static int create_burn_worker_t()
{
	init_burn_d();

	g_burn_d.burn_t = ifn_make_thread(burning_run, NULL);
	if(cmm_mount_on_thread(g_burn_d.burn_t) < 0) {
		cmm_mount_off_thread(g_burn_d.burn_t);
		cleanup_burn_worker_t();

		return -1;
	}

	start_worker_t();

	return 0;
}

static int _get_bookmark_time_txt(guint64 beg, guint64 end, char *ext, char *buf)
{
	time_t tmp;
	char tbeg[128];
	char tend[128];

	tmp = ifn_convert_guint64_to_timet(beg);
	dtf_get_localtime_text(tmp, YYYYMMDD, H24, tbeg);
	tmp = ifn_convert_guint64_to_timet(end);
	dtf_get_localtime_text(tmp, YYYYMMDD, H24, tend);

	sprintf(buf, "%s: %s - %s", ext, tbeg, tend);
	return 0;
}

static int _get_reserved_data_time(guint16 arch_id, time_t *beg, time_t *end)
{
	NF_ARCH_AVI_INFO data;

	if(!nf_arch_list_get_avi(NF_ARCH_DIR_FORWARD, arch_id, 1, &data, NULL, NULL))
		return -1;

	*beg = ifn_convert_guint64_to_timet(data.time_beg);
	*end = ifn_convert_guint64_to_timet(data.time_end);

	return 0;
}

/////////////////////////////////////////////////////////////
//
// protected interfaces
//

int _scm_save_qry_result(SCM_T *piscm, NF_ARCH_AVI_INFO *result)
{
	memcpy(&piscm->qry_result, result, sizeof(NF_ARCH_AVI_INFO));
	return 0;
}

int _scm_save_snap_qry_result(SCM_T *piscm, NF_ARCH_SNAP_INFO *result)
{
	memcpy(&piscm->qry_snap_result, result, sizeof(NF_ARCH_SNAP_INFO));
	return 0;
}


/////////////////////////////////////////////////////////////
//
// public interfaces - BURNING
//

int scm_start_burning(NF_ARCH_TYPE_E type, guint16 arch_id, MEDIA_ID mid, gboolean isErase)
{
	guchar dev_id;
	GError *err = NULL;
	BRN_CODE_E ret_val = BRN_SUCCESS;
	DateTimeData dtdata;

	if (is_running_worker_t()) return -1;

	if (isErase && mda_is_mounted_media(mid)) mda_loosen(mid);
	dev_id = mda_get_device_id(mid);
	DAL_get_dateTime_data(&dtdata);
	arch_set_timezone((unsigned char)dtdata.timeZone, (unsigned char)dtdata.dst);

	if (!nf_arch_burn_start(type, arch_id, dev_id, isErase, burn_status_cb, NULL, &err)) {
		if (err) {
			ret_val = err->code;
			g_error_free(err);
			return ret_val;
		}
		else return BRN_CODE_FAIL;
	}
	if (create_burn_worker_t() < 0) return -1;

    g_burn_d.caller = IUX_CALLER();
	if (g_burn_d.caller == LOCAL_CALL) _scm_send_msg_to_viewer(INFY_ARCH_BURN_API_BEGIN, 0);
	else if (g_burn_d.caller == WEB_CALL) _scm_send_msg_to_viewer(INFY_ARCH_BURN_API_BEGIN, 1);

	return ret_val;
}

int scm_end_burning()
{
	nf_arch_burn_end(NULL);
	DMSG(1, "");
	if (g_burn_d.caller == LOCAL_CALL) _scm_send_msg_to_viewer(INFY_ARCH_BURN_API_CMPL, 0);
	else if (g_burn_d.caller == WEB_CALL) _scm_send_msg_to_viewer(INFY_ARCH_BURN_API_CMPL, 1);

	return 0;
}

int scm_cancel_burning()
{
	int ret = nf_arch_burn_cancel(burn_cancel_cb, NULL, NULL);
	if (is_running_worker_t()) exit_worker_t();
	return (ret == 1) ? 0 : -1;
}

int scm_set_arch_data_format(ARCH_FORMAT_E format)
{
	if (nf_arch_set_multi(format)) return 0;
	return -1;
}

int scm_set_arch_data_cipher(ARCH_FORMAT_E format, char *pw, int pw_max_len)
{

    NF_ARCH_CIPHER_PARAM cipher;

    memset(&cipher, 0x00, sizeof(NF_ARCH_CIPHER_PARAM));
    
    if (format == ARCH_RAW_EN)
    {
        cipher.enable = 1;
        strncpy(cipher.passwd, pw, pw_max_len);
        strncpy(cipher.iv, cipher.passwd, pw_max_len);
    }
    else
    {
        cipher.enable = 0;
    }

    nf_arch_set_cipher_param(&cipher, NULL);
	
    return 0;
}

