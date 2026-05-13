/*
 * scm_spot.c
 *  - scm spot sequence service
 *  - dependency :
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Feb 21, 2012
 *
 */

#include "scm_internal.h"
#include "nf_object.h"
#include "../modules/wrk.h"
#include "scm.h"
#include "iux_afx.h"
#include "ix_mem.h"
#include "nfdal.h"
#include "ix_func.h"
#include "spt.h"

#define DBG_LEVEL       9
#define DBG_MODULE      "SCM_SPOT"

#define IS_SUCCESS()            (result == 0)


////////////////////////////////////////////////////////////
//
// public data type 
//


////////////////////////////////////////////////////////////
//
// private functions
//

static gboolean _proc_spot_service(void *data)
{
    SPOT_SERVICE_PROC service_proc;
    service_proc = spt_get_procedure(data);
    service_proc(data);
    return TRUE;
}

static int _send_spot_msg(SPT_ID sptid, SPOT_CMD_T *cmd)
{
    _scm_put_message(iREQ_SPOT_CMD, 0, 1, cmd); 
    return 0;
}

static int _on_updated(SCM_T *piscm, SPOT_CMD_T *pcmd)
{
    if (pcmd->spottype == SPOT_TYPE_SD)
        spt_push_updating(piscm->spot_sd[pcmd->spch]);
    else if (pcmd->spottype == AUX_TYPE_SD)
        spt_push_updating(piscm->aux_sd[pcmd->spch]);
    else if (pcmd->spottype == SPOT_TYPE_HD)
        spt_push_updating(piscm->spot_hd[pcmd->spch]);
    else if (pcmd->spottype == SPOT_TYPE_DUAL)
        spt_push_updating(piscm->spot_dual[pcmd->spch]);
        
    return 0;
}

static int _on_config(SCM_T *piscm, SPOT_CMD_T *pcmd)
{
    if (pcmd->spottype == SPOT_TYPE_SD)
        spt_display_spot(piscm->spot_sd[pcmd->spch], &pcmd->scr, pcmd->dwell);
    else if (pcmd->spottype == AUX_TYPE_SD)
        spt_display_spot(piscm->aux_sd[pcmd->spch], &pcmd->scr, pcmd->dwell);
    else if (pcmd->spottype == SPOT_TYPE_HD)
        spt_display_spot(piscm->spot_hd[pcmd->spch], &pcmd->scr, pcmd->dwell);
    else if (pcmd->spottype == SPOT_TYPE_DUAL)
        spt_display_spot(piscm->spot_dual[pcmd->spch], &pcmd->scr, pcmd->dwell);

    return 0;
}

static int _on_sequence(SCM_T *piscm, SPOT_CMD_T *pcmd)
{
    if (pcmd->spottype == SPOT_TYPE_SD)
        spt_control_sequence(piscm->spot_sd[pcmd->spch], pcmd->param);
    else if (pcmd->spottype == AUX_TYPE_SD)
        spt_control_sequence(piscm->aux_sd[pcmd->spch], pcmd->param);
    else if (pcmd->spottype == SPOT_TYPE_HD)
        spt_control_sequence(piscm->spot_hd[pcmd->spch], pcmd->param);
    else if (pcmd->spottype == SPOT_TYPE_DUAL)
        spt_control_sequence(piscm->spot_dual[pcmd->spch], pcmd->param);

    return 0;
}

static int _on_osd(SCM_T *piscm, SPOT_CMD_T *pcmd)
{
    if (pcmd->spottype == SPOT_TYPE_SD)
        spt_control_osd(piscm->spot_sd[pcmd->spch], pcmd->param);
    else if (pcmd->spottype == AUX_TYPE_SD)
        spt_control_osd(piscm->aux_sd[pcmd->spch], pcmd->param);
    else if (pcmd->spottype == SPOT_TYPE_HD)
        spt_control_osd(piscm->spot_hd[pcmd->spch], pcmd->param);
    else if (pcmd->spottype == SPOT_TYPE_DUAL)
        spt_control_osd(piscm->spot_dual[pcmd->spch], pcmd->param);

    return 0;
}

////////////////////////////////////////////////////////////
//
// protected interfaces
//

int _scm_init_spot_service(SCM_T *piscm)
{
    int i;

    if (ivsc.dfunc.support_spot == 0 && ivsc.dfunc.dualmonitor.support == 0) return -1;

    for (i = 0; i < SPOTSD_PORT_CNT; i++)
    {
        DMSG(1, "");
        piscm->spot_sd[i] = (SPT_ID)spt_create(i, SPOT_TYPE_SD);
        _scm_add_timeout(piscm, 500, _proc_spot_service, piscm->spot_sd[i]);
        usleep(200000);
    }

    for (i = 0; i < SPOTAUX_PORT_CNT; i++)
    {
        DMSG(1, "");
        piscm->aux_sd[i] = (SPT_ID)spt_create(i, AUX_TYPE_SD);
        _scm_add_timeout(piscm, 500, _proc_spot_service, piscm->aux_sd[i]);
        usleep(200000);
    }
    
    for (i = 0; i < SPOTHD_PORT_CNT; i++)
    {
        DMSG(1, "");
        piscm->spot_hd[i] = (SPT_ID)spt_create(i, SPOT_TYPE_HD);
        _scm_add_timeout(piscm, 500, _proc_spot_service, piscm->spot_hd[i]);
        usleep(200000);
    }

    for (i = 0; i < SPOTDUAL_PORT_CNT; i++)
    {
        DMSG(1, "");
        piscm->spot_dual[i] = (SPT_ID)spt_create(i, SPOT_TYPE_DUAL);
        _scm_add_timeout(piscm, 500, _proc_spot_service, piscm->spot_dual[i]);
        usleep(200000);
    }

    return 0;
}

int _scm_push_spot_updating(SCM_T *piscm)
{
    SPOT_CMD_T *cmd;
    int spch;

    if (ivsc.dfunc.support_spot == 0 && ivsc.dfunc.dualmonitor.support == 0) return -1;

    for (spch = 0; spch < SPOTSD_PORT_CNT; spch++)
    {
        cmd = imalloc(sizeof(SPOT_CMD_T));
        cmd->type = SPOT_UPDATED;
        cmd->spch = spch;
        cmd->spottype = SPOT_TYPE_SD;           
        _send_spot_msg(iscm.spot_sd[spch], cmd);
    }
    
    for (spch = 0; spch < SPOTAUX_PORT_CNT; spch++)
    {
        cmd = imalloc(sizeof(SPOT_CMD_T));
        cmd->type = SPOT_UPDATED;
        cmd->spch = spch;
        cmd->spottype = AUX_TYPE_SD;            
        _send_spot_msg(iscm.aux_sd[spch], cmd);
    }
    
    for (spch = 0; spch < SPOTHD_PORT_CNT; spch++)
    {
        cmd = imalloc(sizeof(SPOT_CMD_T));
        cmd->type = SPOT_UPDATED;
        cmd->spch = spch;
        cmd->spottype = SPOT_TYPE_HD;
        _send_spot_msg(iscm.spot_hd[spch], cmd);
    }

    for (spch = 0; spch < SPOTDUAL_PORT_CNT; spch++)
    {
        cmd = imalloc(sizeof(SPOT_CMD_T));
        cmd->type = SPOT_UPDATED;
        cmd->spch = spch;
        cmd->spottype = SPOT_TYPE_DUAL;
        _send_spot_msg(iscm.spot_dual[spch], cmd);
    }
        
    return 0;
}

HANDLER int _scm_on_spot_command(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
    SPOT_CMD_T *cmd = (SPOT_CMD_T *)pmsg->data;

    if (ivsc.dfunc.support_spot == 0 && ivsc.dfunc.dualmonitor.support == 0) return -1;

    switch (cmd->type) {
    case SPOT_UPDATED:  _on_updated(piscm, cmd); break;
    case SPOT_CONFIG:   _on_config(piscm, cmd); break;
    case SPOT_SEQ_CTRL: _on_sequence(piscm, cmd); break;
    case SPOT_OSD_CTRL: _on_osd(piscm, cmd); break;
    }

    return 0;
}

////////////////////////////////////////////////////////////
//
// public interfaces
//

int scm_display_spot_SD(int spch, SPOT_SCREEN_T *spot_scr)
{

    if (spch >= SPOTSD_PORT_CNT) return 0;

    SPOT_CMD_T *cmd = imalloc(sizeof(SPOT_CMD_T));

    cmd->type = SPOT_CONFIG;
    cmd->spch = spch;
    cmd->spottype = SPOT_TYPE_SD;       
    cmd->dwell = 0;
    cmd->scr = *spot_scr;
    _send_spot_msg(iscm.spot_sd[spch], cmd);
    return 0;
}

int scm_display_single_spot_SD(int spch, int ch)
{

    if (spch >= SPOTSD_PORT_CNT) return 0;

    SPOT_CMD_T *cmd = imalloc(sizeof(SPOT_CMD_T));

    cmd->type = SPOT_CONFIG;
    cmd->spch = spch;
    cmd->spottype = SPOT_TYPE_SD;       
    cmd->dwell = 0;
    cmd->scr.type = SCR_1DIV;
    cmd->scr.conf[0] = ch;
    _send_spot_msg(iscm.spot_sd[spch], cmd);
    return 0;
}

int scm_popup_spot_SD(int spch, SPOT_SCREEN_T *spot_scr, SECOND_D dwell)
{

    if (spch >= SPOTSD_PORT_CNT) return 0;

    SPOT_CMD_T *cmd = imalloc(sizeof(SPOT_CMD_T));

    cmd->type = SPOT_CONFIG;
    cmd->spch = spch;
    cmd->spottype = SPOT_TYPE_SD;       
    cmd->dwell = dwell;
    cmd->scr = *spot_scr;
    _send_spot_msg(iscm.spot_sd[spch], cmd);
    return 0;
}

int scm_popup_single_spot_SD(int spch, int ch, SECOND_D dwell)
{

    if (spch >= SPOTSD_PORT_CNT) return 0;

    SPOT_CMD_T *cmd = imalloc(sizeof(SPOT_CMD_T));

    cmd->type = SPOT_CONFIG;
    cmd->spch = spch;
    cmd->spottype = SPOT_TYPE_SD;       
    cmd->dwell = dwell;
    cmd->scr.type = SCR_1DIV;
    cmd->scr.conf[0] = ch;
    _send_spot_msg(iscm.spot_sd[spch], cmd);
    return 0;
}

int scm_start_spot_sequence_SD(int spch)
{

    if (spch >= SPOTSD_PORT_CNT) return 0;

    SPOT_CMD_T *cmd = imalloc(sizeof(SPOT_CMD_T));

    cmd->type = SPOT_SEQ_CTRL;
    cmd->spch = spch;
    cmd->spottype = SPOT_TYPE_SD;       
    cmd->param = 1;
    _send_spot_msg(iscm.spot_sd[spch], cmd);
    return 0;
}

int scm_stop_spot_sequence_SD(int spch)
{

    if (spch >= SPOTSD_PORT_CNT) return 0;

    SPOT_CMD_T *cmd = imalloc(sizeof(SPOT_CMD_T));

    cmd->type = SPOT_SEQ_CTRL;
    cmd->spch = spch;
    cmd->spottype = SPOT_TYPE_SD;       
    cmd->param = 0;
    _send_spot_msg(iscm.spot_sd[spch], cmd);
    return 0;
}

int scm_show_spot_osd_SD(int spch)
{

    if (spch >= SPOTSD_PORT_CNT) return 0;

    SPOT_CMD_T *cmd = imalloc(sizeof(SPOT_CMD_T));

    cmd->type = SPOT_OSD_CTRL;
    cmd->spch = spch;
    cmd->spottype = SPOT_TYPE_SD;   
    cmd->param = 1;
    _send_spot_msg(iscm.spot_sd[spch], cmd);
    return 0;
}

int scm_hide_spot_osd_SD(int spch)
{

    if (spch >= SPOTSD_PORT_CNT) return 0;

    SPOT_CMD_T *cmd = imalloc(sizeof(SPOT_CMD_T));

    cmd->type = SPOT_OSD_CTRL;
    cmd->spch = spch;
    cmd->spottype = SPOT_TYPE_SD;   
    cmd->param = 0;
    _send_spot_msg(iscm.spot_sd[spch], cmd);
    return 0;
}

int scm_enable_spot_SD(int spch)
{
    spt_enable_spot(iscm.spot_sd[spch]);
    
    return 0;
}

int scm_disable_spot_SD(int spch)
{
    spt_disable_spot(iscm.spot_sd[spch]);

    return 0;
}

int scm_display_aux_SD(int spch, SPOT_SCREEN_T *spot_scr)
{
    if (spch >= SPOTAUX_PORT_CNT) return 0;

    SPOT_CMD_T *cmd = imalloc(sizeof(SPOT_CMD_T));

    cmd->type = SPOT_CONFIG;
    cmd->spch = spch;
    cmd->spottype = AUX_TYPE_SD;        
    cmd->dwell = 0;
    cmd->scr = *spot_scr;
    _send_spot_msg(iscm.aux_sd[spch], cmd);
    return 0;
}

int scm_display_single_aux_SD(int spch, int ch)
{
    if (spch >= SPOTAUX_PORT_CNT) return 0;

    SPOT_CMD_T *cmd = imalloc(sizeof(SPOT_CMD_T));

    cmd->type = SPOT_CONFIG;
    cmd->spch = spch;
    cmd->spottype = AUX_TYPE_SD;        
    cmd->dwell = 0;
    cmd->scr.type = SCR_1DIV;
    cmd->scr.conf[0] = ch;
    _send_spot_msg(iscm.aux_sd[spch], cmd);
    return 0;
}

int scm_popup_aux_SD(int spch, SPOT_SCREEN_T *spot_scr, SECOND_D dwell)
{
    if (spch >= SPOTAUX_PORT_CNT) return 0;

    SPOT_CMD_T *cmd = imalloc(sizeof(SPOT_CMD_T));

    cmd->type = SPOT_CONFIG;
    cmd->spch = spch;
    cmd->spottype = AUX_TYPE_SD;        
    cmd->dwell = dwell;
    cmd->scr = *spot_scr;
    _send_spot_msg(iscm.aux_sd[spch], cmd);
    return 0;
}

int scm_popup_single_aux_SD(int spch, int ch, SECOND_D dwell)
{
    if (spch >= SPOTAUX_PORT_CNT) return 0;

    SPOT_CMD_T *cmd = imalloc(sizeof(SPOT_CMD_T));

    cmd->type = SPOT_CONFIG;
    cmd->spch = spch;
    cmd->spottype = AUX_TYPE_SD;        
    cmd->dwell = dwell;
    cmd->scr.type = SCR_1DIV;
    cmd->scr.conf[0] = ch;
    _send_spot_msg(iscm.aux_sd[spch], cmd);
    return 0;
}

int scm_start_aux_sequence_SD(int spch)
{
    if (spch >= SPOTAUX_PORT_CNT) return 0;

    SPOT_CMD_T *cmd = imalloc(sizeof(SPOT_CMD_T));

    cmd->type = SPOT_SEQ_CTRL;
    cmd->spch = spch;
    cmd->spottype = AUX_TYPE_SD;        
    cmd->param = 1;
    _send_spot_msg(iscm.aux_sd[spch], cmd);
    return 0;
}

int scm_stop_aux_sequence_SD(int spch)
{
    if (spch >= SPOTAUX_PORT_CNT) return 0;

    SPOT_CMD_T *cmd = imalloc(sizeof(SPOT_CMD_T));

    cmd->type = SPOT_SEQ_CTRL;
    cmd->spch = spch;
    cmd->spottype = AUX_TYPE_SD;        
    cmd->param = 0;
    _send_spot_msg(iscm.aux_sd[spch], cmd);
    return 0;
}

int scm_show_aux_osd_SD(int spch)
{
    if (spch >= SPOTAUX_PORT_CNT) return 0;

    SPOT_CMD_T *cmd = imalloc(sizeof(SPOT_CMD_T));

    cmd->type = SPOT_OSD_CTRL;
    cmd->spch = spch;
    cmd->spottype = AUX_TYPE_SD;    
    cmd->param = 1;
    _send_spot_msg(iscm.aux_sd[spch], cmd);
    return 0;
}

int scm_hide_aux_osd_SD(int spch)
{
    if (spch >= SPOTAUX_PORT_CNT) return 0;

    SPOT_CMD_T *cmd = imalloc(sizeof(SPOT_CMD_T));

    cmd->type = SPOT_OSD_CTRL;
    cmd->spch = spch;
    cmd->spottype = AUX_TYPE_SD;    
    cmd->param = 0;
    _send_spot_msg(iscm.aux_sd[spch], cmd);
    return 0;
}

int scm_enable_aux_SD(int spch)
{
    spt_enable_spot(iscm.aux_sd[spch]);
    
    return 0;
}

int scm_disable_aux_SD(int spch)
{
    spt_disable_spot(iscm.aux_sd[spch]);

    return 0;
}


int scm_display_spot_HD(int spch, SPOT_SCREEN_T *spot_scr)
{
    if (spch >= SPOTHD_PORT_CNT) return 0;

    SPOT_CMD_T *cmd = imalloc(sizeof(SPOT_CMD_T));

    cmd->type = SPOT_CONFIG;
    cmd->spch = spch;
    cmd->spottype = SPOT_TYPE_HD;
    cmd->dwell = 0;
    cmd->scr = *spot_scr;
    _send_spot_msg(iscm.spot_hd[spch], cmd);
    return 0;
}

int scm_display_single_spot_HD(int spch, int ch)
{
    SPOT_CMD_T *cmd = imalloc(sizeof(SPOT_CMD_T));

    if (spch >= SPOTHD_PORT_CNT) return 0;

    cmd->type = SPOT_CONFIG;
    cmd->spch = spch;
    cmd->spottype = SPOT_TYPE_HD;
    cmd->dwell = 0;
    cmd->scr.type = SCR_1DIV;
    cmd->scr.conf[0] = ch;
    _send_spot_msg(iscm.spot_hd[spch], cmd);
    return 0;
}

int scm_popup_spot_HD(int spch, SPOT_SCREEN_T *spot_scr, SECOND_D dwell)
{
    if (spch >= SPOTHD_PORT_CNT) return 0;

    SPOT_CMD_T *cmd = imalloc(sizeof(SPOT_CMD_T));

    cmd->type = SPOT_CONFIG;
    cmd->spch = spch;
    cmd->spottype = SPOT_TYPE_HD;
    cmd->dwell = dwell;
    cmd->scr = *spot_scr;
    _send_spot_msg(iscm.spot_hd[spch], cmd);
    return 0;
}

int scm_popup_single_spot_HD(int spch, int ch, SECOND_D dwell)
{
    if (spch >= SPOTHD_PORT_CNT) return 0;

    SPOT_CMD_T *cmd = imalloc(sizeof(SPOT_CMD_T));

    cmd->type = SPOT_CONFIG;
    cmd->spch = spch;
    cmd->spottype = SPOT_TYPE_HD;
    cmd->dwell = dwell;
    cmd->scr.type = SCR_1DIV;
    cmd->scr.conf[0] = ch;
    _send_spot_msg(iscm.spot_hd[spch], cmd);
    return 0;
}

int scm_start_spot_sequence_HD(int spch)
{
    if (spch >= SPOTHD_PORT_CNT) return 0;

    SPOT_CMD_T *cmd = imalloc(sizeof(SPOT_CMD_T));

    cmd->type = SPOT_SEQ_CTRL;
    cmd->spch = spch;
    cmd->spottype = SPOT_TYPE_HD;
    cmd->param = 1;
    _send_spot_msg(iscm.spot_hd[spch], cmd);
    return 0;
}

int scm_stop_spot_sequence_HD(int spch)
{
    if (spch >= SPOTHD_PORT_CNT) return 0;

    SPOT_CMD_T *cmd = imalloc(sizeof(SPOT_CMD_T));

    cmd->type = SPOT_SEQ_CTRL;
    cmd->spch = spch;
    cmd->spottype = SPOT_TYPE_HD;   
    cmd->param = 0;
    _send_spot_msg(iscm.spot_hd[spch], cmd);
    return 0;
}

int scm_show_spot_osd_HD(int spch)
{
    if (spch >= SPOTHD_PORT_CNT) return 0;

    SPOT_CMD_T *cmd = imalloc(sizeof(SPOT_CMD_T));

    cmd->type = SPOT_OSD_CTRL;
    cmd->spch = spch;
    cmd->spottype = SPOT_TYPE_HD;   
    cmd->param = 1;
    _send_spot_msg(iscm.spot_hd[spch], cmd);
    return 0;
}

int scm_hide_spot_osd_HD(int spch)
{
    if (spch >= SPOTHD_PORT_CNT) return 0;

    SPOT_CMD_T *cmd = imalloc(sizeof(SPOT_CMD_T));

    cmd->type = SPOT_OSD_CTRL;
    cmd->spch = spch;
    cmd->spottype = SPOT_TYPE_HD;
    cmd->param = 0;
    _send_spot_msg(iscm.spot_hd[spch], cmd);
    return 0;
}

int scm_display_spot_DUAL(int spch, SPOT_SCREEN_T *spot_scr)
{
    if (spch >= SPOTDUAL_PORT_CNT) return 0;

    SPOT_CMD_T *cmd = imalloc(sizeof(SPOT_CMD_T));

    cmd->type = SPOT_CONFIG;
    cmd->spch = spch;
    cmd->spottype = SPOT_TYPE_DUAL;
    cmd->dwell = 0;
    cmd->scr = *spot_scr;
    _send_spot_msg(iscm.spot_dual[spch], cmd);
    return 0;
}

int scm_display_single_spot_DUAL(int spch, int ch)
{
    SPOT_CMD_T *cmd = imalloc(sizeof(SPOT_CMD_T));

    if (spch >= SPOTDUAL_PORT_CNT) return 0;

    cmd->type = SPOT_CONFIG;
    cmd->spch = spch;
    cmd->spottype = SPOT_TYPE_DUAL;
    cmd->dwell = 0;
    cmd->scr.type = SCR_1DIV;
    cmd->scr.conf[0] = ch;
    _send_spot_msg(iscm.spot_hd[spch], cmd);
    return 0;
}

int scm_popup_spot_DUAL(int spch, SPOT_SCREEN_T *spot_scr, SECOND_D dwell)
{
    if (spch >= SPOTDUAL_PORT_CNT) return 0;

    SPOT_CMD_T *cmd = imalloc(sizeof(SPOT_CMD_T));

    cmd->type = SPOT_CONFIG;
    cmd->spch = spch;
    cmd->spottype = SPOT_TYPE_DUAL;
    cmd->dwell = dwell;
    cmd->scr = *spot_scr;
    _send_spot_msg(iscm.spot_dual[spch], cmd);
    return 0;
}

int scm_popup_single_spot_DUAL(int spch, int ch, SECOND_D dwell)
{
    if (spch >= SPOTDUAL_PORT_CNT) return 0;

    SPOT_CMD_T *cmd = imalloc(sizeof(SPOT_CMD_T));

    cmd->type = SPOT_CONFIG;
    cmd->spch = spch;
    cmd->spottype = SPOT_TYPE_DUAL;
    cmd->dwell = dwell;
    cmd->scr.type = SCR_1DIV;
    cmd->scr.conf[0] = ch;
    _send_spot_msg(iscm.spot_dual[spch], cmd);
    return 0;
}

int scm_start_spot_sequence_DUAL(int spch)
{
    if (spch >= SPOTDUAL_PORT_CNT) return 0;

    SPOT_CMD_T *cmd = imalloc(sizeof(SPOT_CMD_T));

    cmd->type = SPOT_SEQ_CTRL;
    cmd->spch = spch;
    cmd->spottype = SPOT_TYPE_DUAL;
    cmd->param = 1;
    _send_spot_msg(iscm.spot_dual[spch], cmd);
    return 0;
}

int scm_stop_spot_sequence_DUAL(int spch)
{
    if (spch >= SPOTDUAL_PORT_CNT) return 0;

    SPOT_CMD_T *cmd = imalloc(sizeof(SPOT_CMD_T));

    cmd->type = SPOT_SEQ_CTRL;
    cmd->spch = spch;
    cmd->spottype = SPOT_TYPE_DUAL; 
    cmd->param = 0;
    _send_spot_msg(iscm.spot_dual[spch], cmd);
    return 0;
}

int scm_show_spot_osd_DUAL(int spch)
{
    if (spch >= SPOTDUAL_PORT_CNT) return 0;

    SPOT_CMD_T *cmd = imalloc(sizeof(SPOT_CMD_T));

    cmd->type = SPOT_OSD_CTRL;
    cmd->spch = spch;
    cmd->spottype = SPOT_TYPE_DUAL; 
    cmd->param = 1;
    _send_spot_msg(iscm.spot_dual[spch], cmd);
    return 0;
}

int scm_hide_spot_osd_DUAL(int spch)
{
    if (spch >= SPOTDUAL_PORT_CNT) return 0;

    SPOT_CMD_T *cmd = imalloc(sizeof(SPOT_CMD_T));

    cmd->type = SPOT_OSD_CTRL;
    cmd->spch = spch;
    cmd->spottype = SPOT_TYPE_DUAL;
    cmd->param = 0;
    _send_spot_msg(iscm.spot_dual[spch], cmd);
    return 0;
}

