
/*
 *  scm_snap.c
 *
 *  written by parangi
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

#define DBG_LEVEL       9
#define DBG_MODULE      "SCM_CAPTURE"



/////////////////////////////////////////////////////////////
//
// private data types
//



/////////////////////////////////////////////////////////////
//
// private variables
//



/////////////////////////////////////////////////////////////
//
// private functions
//

static int _proc_get_live_capture(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
    SCM_T *piscm = (SCM_T *)pmsg->param;
    int ch, timeimg;
    int ack_ret = 0;
    guint cur_time = 0;
    CAPTURE_IMAGE_T *image = imalloc(sizeof(CAPTURE_IMAGE_T));
    DateTimeData dtdata;

    CMMACK_T cmmack;
    TRANSACTION_E tra;

    DMSG(9, "");
    wrk_get_cmmack(wrkid, &cmmack);
    tra = (TRANSACTION_E)cmmack.data;
    ch = piscm->chart[tra].int_data;
    timeimg = piscm->chart[tra].char_data;

    DAL_get_dateTime_data(&dtdata);
#ifdef _SUPPORT_SNAPSHOT
    ack_ret = nf_live_get_jpeg_snapshot(ch, &image->width, &image->height, &image->size, &image->buffer, timeimg, dtdata.dst, &cur_time);
#endif

    image->ch = ch;
    image->time = cur_time;
    piscm->chart[tra].result = (void *)image;

    return ack_ret == 1 ? 0 : -1;
}

static int _proc_get_live_capture_without_stream(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
    SCM_T *piscm = (SCM_T *)pmsg->param;
    int ch, timeimg;
    int ack_ret = 0;
    guint cur_time = 0;
    CAPTURE_IMAGE_T *image = imalloc(sizeof(CAPTURE_IMAGE_T));
    DateTimeData dtdata;

    CMMACK_T cmmack;
    TRANSACTION_E tra;

    DMSG(9, "");
    wrk_get_cmmack(wrkid, &cmmack);
    tra = (TRANSACTION_E)cmmack.data;
    ch = piscm->chart[tra].int_data;
    timeimg = piscm->chart[tra].char_data;

    DAL_get_dateTime_data(&dtdata);
#ifdef _SUPPORT_SNAPSHOT
#if defined(_IPX_1648M4) || defined(_IPX_1648M4E) || defined(_IPX_32M4E) || defined(_IPX_1648P4E)|| defined(_IPX_32P4E) || defined(_IPX_32P5)
    ack_ret = nf_live_get_jpeg_snapshot(ch, &image->width, &image->height, &image->size, &image->buffer, timeimg, dtdata.dst, &cur_time);
#else
    ack_ret = nf_live_get_jpeg_snapshot(ch, &image->width, &image->height, &image->size, &image->buffer, timeimg, dtdata.dst, &cur_time);
#endif
#endif

    image->ch = ch;
    image->time = cur_time;
    piscm->chart[tra].result = (void *)image;

    return ack_ret == 1 ? 0 : -1;
}

static int _proc_get_playback_capture(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
    SCM_T *piscm = (SCM_T *)pmsg->param;
    int ch, timeimg;
    int ack_ret = 0;
    guint cur_time = 0;
    CAPTURE_IMAGE_T *image = imalloc(sizeof(CAPTURE_IMAGE_T));
    DateTimeData dtdata;

    CMMACK_T cmmack;
    TRANSACTION_E tra;

    DMSG(9, "");
    wrk_get_cmmack(wrkid, &cmmack);
    tra = (TRANSACTION_E)cmmack.data;
    ch = piscm->chart[tra].int_data;
    timeimg = piscm->chart[tra].char_data;

    DAL_get_dateTime_data(&dtdata);
#ifdef _SUPPORT_SNAPSHOT
    ack_ret = nf_play_get_jpeg_snapshot(ch, &image->width, &image->height, &image->size, &image->buffer, timeimg, dtdata.dst, &cur_time);
#endif

    image->ch = ch;
    image->time = cur_time;
    piscm->chart[tra].result = (void *)image;

    return ack_ret == 1 ? 0 : -1;
}

static int _scm_cleanup_capture_wrk(SCM_T *piscm)
{
    wrk_destroy_worker(piscm->wrk_capture);
    piscm->wrk_capture = 0;
    return 0;
}

/////////////////////////////////////////////////////////////
//
// protected interfaces
//

HANDLER int _scm_on_capture_work_cmpl(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
    WRK_ID wrk_id = (WRK_ID)pmsg->data;
    TRANSACTION_E tra;
    CMMACK_T cmmack;
    int result = pmsg->param;

    wrk_get_cmmack(wrk_id, &cmmack);
    tra = (TRANSACTION_E)cmmack.data;

    switch (tra) {
    case TRA_CAPTURE:
        DMSG(9, "");
        _scm_finalize_tra(piscm, tra, result);
        _scm_cleanup_capture_wrk(piscm);
        break;
    }
    return 0;
}


/////////////////////////////////////////////////////////////
//
// public interfaces
//

int scm_req_live_capture(IMSG ret_msg, int ch)
{
    TRANSACTION_E tra = TRA_CAPTURE;
    CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_CAPTURE_IMAGE, (void *)tra };
    if (iscm.wrk_capture) return -1;

    DMSG(9, "");
    _scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());
    iscm.chart[tra].int_data = ch;
    iscm.chart[tra].char_data = 1;

    iscm.wrk_capture = wrk_create_worker(_proc_get_live_capture, &cmmack);
    wrk_run_once_param(iscm.wrk_capture, IMSG_NONE, &iscm, 0, 0);
    return 0;
}

int scm_req_live_capture_without_time(IMSG ret_msg, int ch)
{
    TRANSACTION_E tra = TRA_CAPTURE;
    CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_CAPTURE_IMAGE, (void *)tra };
    if (iscm.wrk_capture) return -1;

    DMSG(9, "");
    _scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());
    iscm.chart[tra].int_data = ch;
    iscm.chart[tra].char_data = 0;

    iscm.wrk_capture = wrk_create_worker(_proc_get_live_capture, &cmmack);
    wrk_run_once_param(iscm.wrk_capture, IMSG_NONE, &iscm, 0, 0);
    return 0;
}

int scm_req_live_capture_without_stream(IMSG ret_msg, int ch)
{
    TRANSACTION_E tra = TRA_CAPTURE;
    CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_CAPTURE_IMAGE, (void *)tra };

    if (iscm.wrk_capture) return -1;

    _scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());
    iscm.chart[tra].int_data = ch;
    iscm.chart[tra].char_data = 0;

    iscm.wrk_capture = wrk_create_worker(_proc_get_live_capture_without_stream, &cmmack);
    wrk_run_once_param(iscm.wrk_capture, IMSG_NONE, &iscm, 0, 0);
    return 0;
}

int scm_req_playback_capture(IMSG ret_msg, int ch)
{
    TRANSACTION_E tra = TRA_CAPTURE;
    CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_CAPTURE_IMAGE, (void *)tra };
    if (iscm.wrk_capture) return -1;

    DMSG(9, "");
    _scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());
    iscm.chart[tra].int_data = ch;
    iscm.chart[tra].char_data = 1;

    iscm.wrk_capture = wrk_create_worker(_proc_get_playback_capture, &cmmack);
    wrk_run_once_param(iscm.wrk_capture, IMSG_NONE, &iscm, 0, 0);
    return 0;
}

int scm_req_playback_capture_without_time(IMSG ret_msg, int ch)
{
    TRANSACTION_E tra = TRA_CAPTURE;
    CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_CAPTURE_IMAGE, (void *)tra };
    if (iscm.wrk_capture) return -1;

    DMSG(9, "");
    _scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());
    iscm.chart[tra].int_data = ch;
    iscm.chart[tra].char_data = 0;

    iscm.wrk_capture = wrk_create_worker(_proc_get_playback_capture, &cmmack);
    wrk_run_once_param(iscm.wrk_capture, IMSG_NONE, &iscm, 0, 0);
    return 0;
}

int scm_req_netkwork_map_capture_auto(IMSG ret_msg)
{
    evt_send_to_local(INFY_CAPTURE_NETWORK_MAP_SCREEN, 0, 0, GUINT_TO_POINTER(ret_msg));
    return 0;
}

int scm_req_netkwork_map_capture_manual(IMSG ret_msg)
{
    evt_send_to_local(INFY_CAPTURE_NETWORK_MAP_SCREEN, 1, 0, GUINT_TO_POINTER(ret_msg));
    return 0;
}
