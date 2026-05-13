/*
 * log.c
 * 	- log 
 * 	- instance type
 * 		: multiple instance
 *	- dependency :
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Feb 25, 2011
 *
 */

#include "log.h"
#include "iux_afx.h"
#include "ix_mem.h"
#include "var.h"
#include "scm.h"
#include <memory.h>
#include "nf_meta_data.h"

#include "ivca_def.h"
#include "nf_api_pos_eventlog.h"
#include "nf_api_dva_eventlog.h"

#define DBG_LEVEL		9
#define DBG_MODULE		"LOG"


/* FOR LOG SEARCH */

#define LC_SYS		((LT_MASK_SYSTEM_STARTED) | (LT_MASK_SYSTEM_SHUTDOWN) | (LT_MASK_ABNORMAL_SHUTDOWN_DETECTED) | (LT_MASK_SYSTEM_RECOVERED)| (LT_MASK_SYSTEM_TIME_CHANGED) | (LT_MASK_SYSTEM_FW_UPGRADE) | (LT_MASK_SYSTEM_FORMAT) | (LT_MASK_SYSTEM_CHECKDISK) | (LT_MASK_SYSTEM_EVENT))
#define LC_LOGIN	((LT_MASK_LOCAL_LOG_ON) | (LT_MASK_LOCAL_LOG_OFF) | (LT_MASK_REMOTE_LOG_ON) | (LT_MASK_REMOTE_LOG_OFF))

#define LC_SYSTEM	(LC_SYS | LC_LOGIN)
#define LC_SETUP	((LT_MASK_RECORD_SETUP_CHANGED) | (LT_MASK_SYSTEM_SETUP_CHANGED))
#define LC_ALARM	((LT_MASK_SENSOR_INPUT))
#define LC_MOTION	((LT_MASK_MOTION_DETECTION))
#define LC_VIDEO	((LT_MASK_VIDEO_IN) | (LT_MASK_VIDEO_LOSS))

//#define LC_UEVT		((LT_MASK_USER_EVENT))
#define LC_TAMPER		((LT_MASK_TAMPER))

#define LC_SMART	((LT_MASK_SMART_WARNING) | (LT_MASK_HDD_OVER_TEMP) | (LT_MASK_HDD_FULL) | (LT_MASK_HDD_OW))
#define LC_RECORD	((LT_MASK_RECORD_STARTED) | (LT_MASK_RECORD_STOPPED))
#define LC_NETWORK 	(LT_MASK_NETWORK_EVENT)
#define LC_IPCAM	(LT_MASK_IPCAM)

#define LC_VCA		(LT_MASK_VCA)
#define LC_DVA		(LT_MASK_DVA)
#define LC_POS		(LT_MASK_SYSTEM_POS)
#define LC_DEBUG	(LT_MASK_SYSTEM_DEBUG)


//#define LC_ALLTYPE	(LC_SYSTEM | LC_SETUP | LC_ALARM | LC_MOTION | LC_VIDEO | LC_UEVT | LC_SMART | LC_RECORD | LC_NETWORK | LC_IPCAM)
#define LC_ALLTYPE	(LC_SYSTEM | LC_SETUP | LC_ALARM | LC_MOTION | LC_VIDEO | LC_TAMPER | LC_SMART | LC_RECORD | LC_NETWORK | LC_IPCAM | LC_VCA | LC_POS)

////////////////////////////////////////////////////////////
//
// private data type 
//



////////////////////////////////////////////////////////////
//
// private variable
//

static int ichannel;

static unsigned int ilog_type[] = {
	LC_ALLTYPE,
	LC_ALARM,
	LC_MOTION,
	LC_VIDEO,
	LC_RECORD,
	LC_SYSTEM,
	LC_SMART,
	LC_NETWORK,
	LC_IPCAM,
	LC_SETUP,
	LC_TAMPER,
	LC_VCA,
	LC_POS,
	LC_DEBUG,
	LC_DVA	
};


static const guchar ilog_p1mask_size[] = {                                   
	LOG_P1T_MASKSIZE_NA,        /* SYSTEM_STARTED */                       
	LOG_P1T_MASKSIZE_NA,        /* SYSTEM_SHUTDOWN */                      
	LOG_P1T_MASKSIZE_NA,        /* ABNORMAL_SHUTDOWN_DETECTED */           
	LOG_P1T_MASKSIZE_NA,        /* SYSTEM_RECOVERED */                     
	LOG_P1T_MASKSIZE_WHO,       /* TIME_CHANGED */                         
	LOG_P1T_MASKSIZE_WHO,       /* FW_UPGRADE */                           
	LOG_P1T_MASKSIZE_WHO,       /* FORMAT */                               
	LOG_P1T_MASKSIZE_WHO,       /* CHECKDISK */                            
	LOG_P1T_MASKSIZE_WHO,       /* LOCAL_LOG_ON */                         
	LOG_P1T_MASKSIZE_WHO,       /* LOCAL_LOG_OFF */                        
	LOG_P1T_MASKSIZE_WHO,       /* REMOTE_LOG_ON */                        
	LOG_P1T_MASKSIZE_WHO,       /* REMOTE_LOG_OFF */                       
	LOG_P1T_MASKSIZE_WHO,       /* RECORD_SETUP_CHANGED */                 
	LOG_P1T_MASKSIZE_WHO,       /* SYSTEM_SETUP_CHANGED */                 
	LOG_P1T_MASKSIZE_CHAN,      /* SENSOR_INPUT */                         
	LOG_P1T_MASKSIZE_CHAN,      /* MOTION_DETECTION */                     
	LOG_P1T_MASKSIZE_CHAN,		/* VIDEO_IN */                             
	LOG_P1T_MASKSIZE_CHAN,		/* VIDEO_LOSS */                           
	LOG_P1T_MASKSIZE_CHAN,		/* TAMPER */                           
	LOG_P1T_MASKSIZE_NA,		/* SMART_WARNING */    
	LOG_P1T_MASKSIZE_NA,		/* DISK_EVENT */     //2011-06-30                   
	LOG_P1T_MASKSIZE_NA,		/* HDD_FULL */                             
	LOG_P1T_MASKSIZE_NA,		/* HDD_OW */
	LOG_P1T_MASKSIZE_CHAN,		/* RECORD_STARTED */
	LOG_P1T_MASKSIZE_CHAN,		/* RECORD_STOPPED */
	
	LOG_P1T_MASKSIZE_NA,		/* SYSTEM_EVENT */                 
	LOG_P1T_MASKSIZE_NA,		/* SYSTEM_DEBUG */                 
	LOG_P1T_MASKSIZE_CHAN,		/* POS LOG */                 
	LOG_P1T_MASKSIZE_NA,		/* NETWORK_EVENT */	//2011-06-30
	LOG_P1T_MASKSIZE_CHAN,		/* IPCAM */
	LOG_P1T_MASKSIZE_CHAN,		/* VCA */
	LOG_P1T_MASKSIZE_CHAN		/* DVA */

};

static const guint ilog_p1mask_ofs[] = {                                   
	LOG_P1_MASKOFS_SYSTEM_STARTED,              /* SYSTEM_STARTED */       
	LOG_P1_MASKOFS_SYSTEM_SHUTDOWN,             /* SYSTEM_SHUTDOWN */      
	LOG_P1_MASKOFS_ABNORMAL_SHUTDOWN_DETECTED,  /* ABNORMAL_SHUTDOWN_DETECTED */
	LOG_P1_MASKOFS_SYSTEM_RECOVERED,            /* SYSTEM_RECOVERED */     
	LOG_P1_MASKOFS_TIME_CHANGED,                /* TIME_CHANGED */
	LOG_P1_MASKOFS_FW_UPGRADE,                  /* FW_UPGRADE */           
	LOG_P1_MASKOFS_FORMAT,                      /* FORMAT */
	LOG_P1_MASKOFS_CHECKDISK,                   /* CHECKDISK */
	LOG_P1_MASKOFS_LOCAL_LOG_ON,                /* LOCAL_LOG_ON */
	LOG_P1_MASKOFS_LOCAL_LOG_OFF,               /* LOCAL_LOG_OFF */        
	LOG_P1_MASKOFS_REMOTE_LOG_ON,               /* REMOTE_LOG_ON */
	LOG_P1_MASKOFS_REMOTE_LOG_OFF,              /* REMOTE_LOG_OFF */       
	LOG_P1_MASKOFS_RECORD_SETUP_CHANGED,        /* RECORD_SETUP_CHANGED */
	LOG_P1_MASKOFS_SYSTEM_SETUP_CHANGED,        /* SYSTEM_SETUP_CHANGED */ 
	LOG_P1_MASKOFS_SENSOR_INPUT,                /* SENSOR_INPUT */
	LOG_P1_MASKOFS_MOTION_DETECTION,            /* MOTION_DETECTION */     
	LOG_P1_MASKOFS_VIDEO_IN,                    /* VIDEO_IN */
	LOG_P1_MASKOFS_VIDEO_LOSS,                  /* VIDEO_LOSS */
	LOG_P1_MASKOFS_TAMPER,                  /* TAMPER */
	LOG_P1_MASKOFS_SMART_WARNING,               /* SMART_WARNING */
	LOG_P1_MASKOFS_DISK_EVENT,					/* DISK_EVENT */		//2011-06-30
	LOG_P1_MASKOFS_HDD_FULL,                    /* HDD_FULL */
	LOG_P1_MASKOFS_HDD_OW,                      /* HDD_OW */
	LOG_P1_MASKOFS_RECORD_STARTED,              /* RECORD_STARTED */
	LOG_P1_MASKOFS_RECORD_STOPPED,              /* RECORD_STOPPED */
	LOG_P1_MASKOFS_SYSTEM_EVENT,				/* SYSTEM_EVENT */
	LOG_P1_MASKOFS_SYSTEM_DEBUG,				/* SYSTEM DEBUG LOG */
	LOG_P1_MASKOFS_SYSTEM_POS,					/* POS LOG */
	LOG_P1_MASKOFS_NETWORK_EVENT,				/* NETWORK_EVENT */	//2011-06-30
	LOG_P1_MASKOFS_IPCAM,						/* IPCAM */
	LOG_P1_MASKOFS_VCA,							/* VCA */
	LOG_P1_MASKOFS_DVA							/* DVA */
};

enum {
	LTIDX_SENSOR_INPUT,
	LTIDX_MOTION_DETECTION,
	LTIDX_VIDEO_IN,
	LTIDX_VIDEO_LOSS,
	LTIDX_TAMPER,
	LTIDX_RECORD_STARTED,
	LTIDX_RECORD_STOPPED,
	LTIDX_SYSTEM_EVENT,
	LTIDX_NETWORK,
	LTIDX_IPCAM,
	LTIDX_VCA,
	LTIDX_POS,
	LTIDX_DVA,
};

static guint ino_offset_evtmask[] = {
	LT_SENSOR_INPUT,
	LT_MOTION_DETECTION,
	LT_VIDEO_IN,
	LT_VIDEO_LOSS,
	LT_TAMPER,
	LT_RECORD_STARTED,
	LT_RECORD_STOPPED,
	LT_SYSTEM_EVENT,
	LT_NETWORK_EVENT,
	LT_IPCAM,
	LT_VCA,
	LT_SYSTEM_POS,
	LT_DVA,
#define		CNT_LT_EVTMASK_HAS_CH	13
};

static guint vca_type[] = {
	0,
	IVCA_ET_DIR_POS,
	IVCA_ET_DIR_NEG,
	IVCA_ET_ENTER,
	IVCA_ET_EXIT,
	IVCA_ET_STOPPED,
	IVCA_ET_ABANDONED,
	IVCA_ET_REMOVED,
	IVCA_ET_LOITERED,
	IVCA_ET_FALL,
	IVCA_ET_COUNTER,
	IVCA_ET_TAMPER,
#define CNT_VCALOG		12
};
		
static guint dva_type[] = {
	0,
	IDVA_ET_INTRUSION_DETECTION,
	IDVA_ET_ILLEGAL_PARKING,
	IDVA_ET_LPR,
#define CNT_DVALOG		4
};

////////////////////////////////////////////////////////////
//
// private functions
//
static guchar _get_param1mask_size(log_evt_type_e type)
{
	return ilog_p1mask_size[type];
}

static guint _get_param1mask_ofs(log_evt_type_e type)
{   
	return ilog_p1mask_ofs[type];
}   

static int _unset_overch(NF_LOG_PARAM *pp, guint evt_type)
{
	int ch;
	int cnt = var_get_ch_count();
	guint no_offset_evtmask;
	int mask_offset;
	int mask_size;

	no_offset_evtmask= ino_offset_evtmask[evt_type];
	mask_offset = _get_param1mask_ofs(no_offset_evtmask);
	mask_size = _get_param1mask_size(no_offset_evtmask);

	for (ch = cnt; ch < mask_size * 8; ++ch) {
		pp->param1_mask[mask_offset + ch / 8] &= ~(1 << (ch % 8));
	}

	return 0;
}

static int _doset_evtmaskbit_has_ch(NF_LOG_PARAM *pp, guint evt_type, int ch)
{
	guint no_offset_evtmask;
	int mask_offset;
	
	no_offset_evtmask= ino_offset_evtmask[evt_type];
	mask_offset = _get_param1mask_ofs(no_offset_evtmask);
	pp->param1_mask[mask_offset + ch / 8] |= (1 << (ch % 8));

	_unset_overch(pp, evt_type);
	return 0;
}

static int _doset_evtmask_has_ch(NF_LOG_PARAM *pp, unsigned int chmask, guint evt_type)
{
/*
	guint no_offset_evtmask;
	int mask_offset;
	int mask_size;

	no_offset_evtmask= ino_offset_evtmask[evt_type];
	mask_offset = _get_param1mask_ofs(no_offset_evtmask);
	mask_size = _get_param1mask_size(no_offset_evtmask);
	memset(&pp->param1_mask[mask_offset], 0xFF, mask_size);
*/

	int i;
	int cnt = var_get_ch_count();
	for (i = 0; i < cnt; ++i) {
		if (chmask & (1 << i)) _doset_evtmaskbit_has_ch(pp, evt_type, i);
	}

	return 0;
}

static int _unset_evtmaskbit_has_ch(NF_LOG_PARAM *pp, guint evt_type, int ch)
{
	
	guint no_offset_evtmask;
	int mask_offset;

	no_offset_evtmask= ino_offset_evtmask[evt_type];
	mask_offset = _get_param1mask_ofs(no_offset_evtmask);
	pp->param1_mask[mask_offset + ch / 8] &= ~(1 << (ch % 8));

	_unset_overch(pp, evt_type);
	return 0;
}

static int _unset_evtmask_has_ch(NF_LOG_PARAM *pp, unsigned int chmask, guint evt_type)
{
/*	
	guint no_offset_evtmask;
	int mask_offset;
	int mask_size;

	no_offset_evtmask= ino_offset_evtmask[evt_type];
	mask_offset = _get_param1mask_ofs(no_offset_evtmask);
	mask_size = _get_param1mask_size(no_offset_evtmask);
	memset(&pp->param1_mask[mask_offset], 0x00, mask_size);
*/

	int i;
	int cnt = var_get_ch_count();
	for (i = 0; i < cnt; ++i) {
		if (chmask & (1 << i)) _unset_evtmaskbit_has_ch(pp, evt_type, i);
	}
	return 0;
}

static int _unset_all_mask(NF_LOG_PARAM *pp)
{
	char *mask_ofs;
	int mask_len;
	int i;

	memset(&pp->param1_mask, 0x00, sizeof(pp->param1_mask));
	pp->type_mask = 0;

	return 0;
}

static int _doset_all_mask(NF_LOG_PARAM *pp)
{
	char *mask_ofs;
	int mask_len;
	int i;
	unsigned int chmask = var_get_ch_mask();

	//memset(&pp->param1_mask, 0xFF, sizeof(pp->param1_mask));
	memset(&pp->param1_mask, 0x00, sizeof(pp->param1_mask));
	pp->type_mask = LC_ALLTYPE;

	for (i = 0; i < CNT_LT_EVTMASK_HAS_CH; ++i) 
		_doset_evtmask_has_ch(pp, chmask, i);

	return 0;
}

static int _init_param(NF_LOG_PARAM *pp)
{
	memset(pp, 0x00, sizeof(NF_LOG_PARAM));
	pp->log_id = 0;
	pp->mode = NF_LOG_PARAM_MODE_TIME;
	pp->direction = LF_LATEST;
	_doset_all_mask(pp);
	return 0;
}

static int _print_log_param_mask(NF_LOG_PARAM *param)
{
	int i, j, pos = 0;
	unsigned char k;
	unsigned char temp_mask[512];
	printf("========================================================================================\n");
	printf("     0 1 2 3 4 5 6 7 8 9  0 1 2 3 4 5 6 7 8 9  0 1 2 3 4 5 6 7 8 9  0 1 2 3 4 5 6 7 8 9\n");
	printf("----------------------------------------------------------------------------------------\n");
	
	memset(temp_mask, 0x00, sizeof(temp_mask));
	memcpy(temp_mask, param->param1_mask, LOG_P1_MASK_SIZE);

	printf("000] ");
	for (j = 0; j < 16; ++j) {
		for (i = 0; i < 32; ++i) {
			if (i != 0 && i % 10 == 0) printf(" ");
			k = (unsigned char)temp_mask[pos];
			printf("%02X", k);
			++pos;
		}
		printf("\n");
		printf("%03d] ", pos);
	}
	printf("----------------------------------------------------------------------------------------\n");
	printf("========================================================================================\n");
}

static int _print_log_result(NF_LOG_RESULT_HEADER *result, SYSREC_DATA_T *psysrec)
{
	int i;
	for (i = 0; i < result->result_count; ++i)
		printf("log id = (%llu)\n", psysrec[i].log_id);
}

static int _get_sysrec(NF_LOG_PARAM *param, NF_LOG_RESULT_HEADER *result, SYSREC_DATA_T *psysrec)
{
// for debugging	
//_print_log_param_mask(param);

	gboolean ret = nf_eventlog_get(param, result, psysrec, NULL);

// for_debugging
//if (ret == TRUE) _print_log_result(result, psysrec);

	return (ret == TRUE ? 0 : -1);	
}

static int _parse_vca_type(unsigned int type)
{
	int i;
	for (i = 1; i < CNT_VCALOG; ++i) {
		if ((vca_type[i] == IVCA_ET_COUNTER) || (vca_type[i] == IVCA_ET_TAMPER))
		{
			if (type & vca_type[i]) return i;
		}
		else if (vca_type[i] == type) return i;
		
	}
	return 0;
}

static int _parse_dva_type(unsigned int type)
{
	/*
	int i;
	for (i = 1; i < CNT_DVALOG; ++i) {
		if (vca_type[i] == IDVA_ET_COUNTER)
		{
			if (type & dva_type[i]) return i;
		}
		else if (dva_type[i] == type) return i;
		
	}*/
	return 0;
}

static int _parse_pos_type(char *buff, LPR_POS_T *tdata)
{
	NF_POS_LOG_DATA *pos = (NF_POS_LOG_DATA *)buff;

	switch (pos->pos_log_type) {
	case NF_POS_LOG_TYPE_ITEM:
		break;
	case NF_POS_LOG_TYPE_TOTAL:
		break;
	case NF_POS_LOG_TYPE_TEXT:
		memcpy(tdata->text, pos->Text.text, 50);
		break;
	case NF_POS_LOG_TYPE_RAW:
		break;
	case NF_POS_LOG_TYPE_NR:
		break;
	}

	return 0;
}

static int _parse_log(SYSREC_DATA_T *ps, LOG_DATA_T *log)
{
	char *p;
	int len;

	switch (ps->type) {
	case LT_SYSTEM_STARTED:
	case LT_SYSTEM_SHUTDOWN:
		break;

	case LT_ABNORMAL_SHUTDOWN_DETECTED:
	    log->p.cat_reboot.klass = ps->param2;
		break;

	case LT_SYSTEM_RECOVERED:
		break;

	case LT_SYSTEM_TIME_CHANGED:
	case LT_SYSTEM_FORMAT:
		strcpy(log->p.cat_system.userid, ps->text);
		break;

	case LT_SYSTEM_SYSLOG:
	    log->p.cat_syslog.klass = ps->param2;
		strcpy(log->p.cat_syslog.userid, ps->text);
		break;

	case LT_SYSTEM_FW_UPGRADE:
	    log->p.cat_fwup.klass = ps->param2;
		strcpy(log->p.cat_fwup.userid, ps->text);
		break;

	case LT_LOCAL_LOG_ON:
		log->p.cat_logon.klass = ps->param2;
		strcpy(log->p.cat_logon.userid, ps->text);
		break;

	case LT_LOCAL_LOG_OFF:
		log->p.cat_logoff.klass = ps->param2;
		strcpy(log->p.cat_logoff.userid, ps->text);
		break;

	case LT_REMOTE_LOG_ON:
		log->p.cat_rlogon.klass = ps->param2;
		p = strchr(ps->text, ':');
		if (p) {
			len = p - ps->text;
			strncpy(log->p.cat_rlogon.userid, ps->text, len);
			log->p.cat_rlogon.userid[len] = 0;
			strcpy(log->p.cat_rlogon.ipaddr, p + 1);
		}
		else strcpy(log->p.cat_rlogon.userid, ps->text);
		break;

	case LT_REMOTE_LOG_OFF:
		log->p.cat_rlogoff.klass = ps->param2;
		p = strchr(ps->text, ':');
		if (p) {
			len = p - ps->text;
			strncpy(log->p.cat_rlogon.userid, ps->text, len);
			log->p.cat_rlogon.userid[len] = 0;
			strcpy(log->p.cat_rlogon.ipaddr, p + 1);
		}
		else strcpy(log->p.cat_rlogon.userid, ps->text);
		break;

	case LT_RECORD_SETUP_CHANGED:
		log->p.cat_recsetup.klass = ps->param2;
		p = strchr(ps->text, ':');
		if (p) {
			len = p - ps->text;
			strncpy(log->p.cat_rlogon.userid, ps->text, len);
			log->p.cat_rlogon.userid[len] = 0;
			strcpy(log->p.cat_rlogon.ipaddr, p + 1);
		}
		else strcpy(log->p.cat_rlogon.userid, ps->text);
		break;

	case LT_SYSTEM_SETUP_CHANGED:
		log->p.cat_syssetup.klass = ps->param2;
		p = strchr(ps->text, ':');
		if (p) {
			len = p - ps->text;
			strncpy(log->p.cat_rlogon.userid, ps->text, len);
			log->p.cat_rlogon.userid[len] = 0;
			strcpy(log->p.cat_rlogon.ipaddr, p + 1);
		}
		else strcpy(log->p.cat_rlogon.userid, ps->text);
		break;

	case LT_SENSOR_INPUT:
		log->p.cat_sensor.channel = ps->param1;
		log->p.cat_sensor.klass = ps->param2;
		break;

	case LT_MOTION_DETECTION:
		log->p.cat_motion.channel = ps->param1;
		log->p.cat_motion.klass = ps->param2;
		break;

	case LT_VIDEO_IN:
		log->p.cat_videoloss.channel = ps->param1;
		log->p.cat_videoloss.klass = 0;
		break;

	case LT_VIDEO_LOSS:
		log->p.cat_videoloss.channel = ps->param1;
		log->p.cat_videoloss.klass = 1;
		break;

	case LT_TAMPER:
		log->p.cat_tamper.channel = ps->param1;
		log->p.cat_tamper.klass = ps->param2;
		break;

	case LT_SMART_WARNING:
//		strcpy(log->p.cat_smart.diskid, ps->text);
		log->p.cat_smart.klass = ps->param2;
		break;

	case LT_DISK_EVENT:
		log->p.cat_disk.location = ps->param1;
		log->p.cat_disk.klass = ps->param2;
		break;

	case LT_HDD_FULL:
		log->p.cat_diskfull.location = ps->param1;
		break;

	case LT_HDD_OW:
		log->p.cat_diskow.location = ps->param1;
		break;

	case LT_RECORD_STARTED:
		log->p.cat_recstart.channel = ps->param1;
		log->p.cat_recstart.klass = ps->param2;
		break;

	case LT_RECORD_STOPPED:
		log->p.cat_recstop.channel = ps->param1;
		log->p.cat_recstop.klass = ps->param2;
		break;

	case LT_SYSTEM_EVENT:
		log->p.cat_sysevt.channel = ps->param1;
		log->p.cat_sysevt.klass = ps->param2;
		strcpy(log->p.cat_sysevt.text, ps->text);
		break;

	case LT_NETWORK_EVENT:
	{
	    //log->p.cat_sysevt.channel = ps->param1;
        //log->p.cat_sysevt.klass = ps->param2;
	    gchar str_buf[32];
	    memset(str_buf, 0x00, sizeof(str_buf));
	    log->p.cat_network.klass = (int)ps->param2;
		
        p = strchr(ps->text, ',');
        if(p){
            len = p - ps->text;
            strncpy(str_buf, ps->text, len);
            log->p.cat_network.channel = atoi(str_buf)+1;
            strcpy(log->p.cat_network.text, p+1);
        }
        else strcpy(log->p.cat_network.text, ps->text);
    }		
		break;

	case LT_SYSTEM_DEBUG:
		log->p.cat_debug.klass = ps->param2;
        strcpy(log->p.cat_debug.text, ps->text);
		break;

	case LT_SYSTEM_POS:
		log->p.cat_pos.channel = ps->param1;
		_parse_pos_type(ps->text, &log->p.cat_pos); 
		break;

	case LT_IPCAM:
		log->p.cat_ipcam.channel = ps->param1;
		log->p.cat_ipcam.klass = ps->param2;
		break;

	case LT_VCA:
		log->p.cat_vca.klass = ps->param2;
		log->p.cat_vca.channel = ps->param1;
		log->p.cat_vca.type = _parse_vca_type(((ivca_rule_event_t *)ps->text)->type);
//		memcpy(log->p.cat_vca.binary, ps->text, sizeof(ivca_rule_event_t));
		break;
		
	case LT_DVA:
		log->p.cat_dva.klass = ps->param2;
		log->p.cat_dva.channel = ps->param1;
//		log->p.cat_dva.type = _parse_dva_type(((idva_rule_event_t *)ps->text)->type);
		memcpy(log->p.cat_dva.binary, ps->text, sizeof(log->p.cat_dva.binary));
		break;
		
	default:
		break;
	}
	
	log->type = ps->type;

	return 0;
}

static int _make_log(SYSREC_DATA_T *psysrec, LOG_DATA_T *log, int cnt)
{
	int i;	

	for (i = 0; i < cnt; i++) {
		GUINT64_TO_GTIMEVAL(psysrec[i].timestamp, log[i].tvTime);
		log[i].log_id = psysrec[i].log_id;
		_parse_log(&psysrec[i], &log[i]);	
	}
	
	return 0;
}

static int _make_log_r(SYSREC_DATA_T *psysrec, LOG_DATA_T *log, int cnt)
{
	int i;
	int j;

	for (i = 0, j = cnt - 1; i < cnt; i++, j--) {
		GUINT64_TO_GTIMEVAL(psysrec[i].timestamp, log[j].tvTime);
		log[j].log_id = psysrec[i].log_id;

		_parse_log(&psysrec[i], &log[j]);	
	}
	
	return 0;
}

static int _set_nextid(LOGX_T *logx, guint64 logid)
{
	logx->param.log_id = logid;
	return 0;	
}

static int _set_search_mode(LOGX_T *logx, NF_LOG_PARAM_MODE_E mode)
{
	logx->param.mode = mode;	
	return 0;
}

static int _set_search_order(LOGX_T *logx, LF_ORDER_E order)
{
	logx->param.direction = order;	
	if (logx->param.direction == LF_OLDEST) {
		logx->param.time_search = logx->param.time_begin; 
	}
	else {
		logx->param.time_search = logx->param.time_end; 
	}
	return 0;
}

static int _set_search_time(LOGX_T *logx, GTimeVal *start, GTimeVal *end)
{
	logx->param.time_begin = *start;
	logx->param.time_end = *end;
	DMSG(1, "BEGIN TIME = (%ld), END TIME = (%ld)\n", logx->param.time_begin.tv_sec, logx->param.time_end.tv_sec);

	if (logx->param.direction == LF_OLDEST) {
		logx->param.time_search = logx->param.time_begin; 
		DMSG(1, "(OLDETEST) SEARCH TIME = %ld\n", logx->param.time_search);
	}
	else {
		logx->param.time_search = logx->param.time_end; 
		DMSG(1, "(LATEST) SEARCH TIME = %ld\n", logx->param.time_search);
	}
	return 0;
}

static int _prepare_data_pool(LOGX_T *logx, int count)
{
	if (logx->count == count) {
		if (!logx->data) {
			logx->data = imalloc(sizeof(LOG_DATA_T) * count);
			logx->sysrec = imalloc(sizeof(SYSREC_DATA_T) * count);
		}
	}
	else {
		logx->data = irealloc(logx->data, sizeof(LOG_DATA_T) * count);
		logx->sysrec = irealloc(logx->sysrec, sizeof(SYSREC_DATA_T) * count);
	}

	logx->count = 0;
	memset(logx->data, 0x00, sizeof(LOG_DATA_T) * count);
	memset(logx->sysrec, 0x00, sizeof(SYSREC_DATA_T) * count);
	return 0;
}

static int _set_request_count(LOGX_T *logx, int count)
{
	logx->param.request_count = count;
	return 0;
}

static LF_ORDER_E _set_prev_order(LOGX_T *logx)
{
	LF_ORDER_E cur = logx->param.direction;
	if (logx->param.direction == LF_OLDEST) logx->param.direction = LF_LATEST;
	else logx->param.direction = LF_OLDEST;
	return cur;
}

static int _set_next_order(LOGX_T *logx)
{
	return 0;
}

static int _save_logid(LOGX_T *logx)
{
	if (logx->count == 0) return -1;
	logx->page_top_logid = logx->data[0].log_id;
	logx->page_bottom_logid = logx->data[logx->count - 1].log_id;
	return 0;
}

static int _unset_type_mask(LOGX_T *logx, LF_CAT_E lcat)
{
	logx->param.type_mask &= ~(ilog_type[lcat]);
	return 0;
}

static int _get_tlog_sysrec(NF_POS_LOG_PARAM *param, NF_POS_LOG_RESULT_HEADER *result, TLOG_SYSREC_DATA_T *psysrec)
{
// for debugging	
//_print_log_param_mask(param);

	gboolean ret = nf_pos_eventlog_get(param, result, psysrec);

// for_debugging
//if (ret == TRUE) _print_log_result(result, psysrec);

	return (ret == TRUE ? 0 : -1);	
}

static int _parse_tlog(TLOG_SYSREC_DATA_T *ps, TLOG_DATA_T *log)
{
    log->p.cat_pos.channel = ps->chan;
    _parse_pos_type(ps, &log->p.cat_pos);    
	return 0;
}

static int _make_tlog(TLOG_SYSREC_DATA_T *psysrec, TLOG_DATA_T *log, int cnt)
{
	int i;

	for (i = 0; i < cnt; i++) {
		GUINT64_TO_GTIMEVAL(psysrec[i].timestamp, log[i].tvTime);
		log[i].log_id = psysrec[i].log_id;

		_parse_tlog(&psysrec[i], &log[i]);	
	}
	
	return 0;
}

static int _make_tlog_r(TLOG_SYSREC_DATA_T *psysrec, TLOG_DATA_T *log, int cnt)
{
	int i;
	int j;

	for (i = 0, j = cnt - 1; i < cnt; i++, j--) {
		GUINT64_TO_GTIMEVAL(psysrec[i].timestamp, log[j].tvTime);
		log[j].log_id = psysrec[i].log_id;

		_parse_tlog(&psysrec[i], &log[j]);	
	}
	
	return 0;
}

static int _set_tlog_nextid(TLOGX_T *tlogx, guint64 logid)
{
	tlogx->param.log_param.log_id = logid;
	return 0;	
}

static int _set_tlog_search_mode(TLOGX_T *tlogx, NF_LOG_PARAM_MODE_E mode)
{
	tlogx->param.log_param.mode = mode;	
	return 0;
}

static int _set_tlog_search_order(TLOGX_T *tlogx, LF_ORDER_E order)
{
	tlogx->param.log_param.direction = order;	
	if (tlogx->param.log_param.direction == LF_OLDEST) {
		tlogx->param.log_param.time_search = tlogx->param.log_param.time_begin; 
	}
	else {
		tlogx->param.log_param.time_search = tlogx->param.log_param.time_end; 
	}
	return 0;
}

static int _set_tlog_search_time(TLOGX_T *tlogx, GTimeVal *start, GTimeVal *end)
{
	tlogx->param.log_param.time_begin = *start;
	tlogx->param.log_param.time_end = *end;
	DMSG(1, "BEGIN TIME = (%ld), END TIME = (%ld)\n", tlogx->param.log_param.time_begin.tv_sec, tlogx->param.log_param.time_end.tv_sec);

	if (tlogx->param.log_param.direction == LF_OLDEST) {
		tlogx->param.log_param.time_search = tlogx->param.log_param.time_begin; 
		DMSG(1, "(OLDETEST) SEARCH TIME = %ld\n", tlogx->param.log_param.time_search);
	}
	else {
		tlogx->param.log_param.time_search = tlogx->param.log_param.time_end; 
		DMSG(1, "(LATEST) SEARCH TIME = %ld\n", tlogx->param.log_param.time_search);
	}
	return 0;
}

static int _prepare_tlog_data_pool(TLOGX_T *tlogx, int count)
{
	if (tlogx->count == count) {
		if (!tlogx->data) {
			tlogx->data = imalloc(sizeof(TLOG_DATA_T) * count);
			tlogx->sysrec = imalloc(sizeof(TLOG_SYSREC_DATA_T) * count);
		}
	}
	else {
		tlogx->data = irealloc(tlogx->data, sizeof(TLOG_DATA_T) * count);
		tlogx->sysrec = irealloc(tlogx->sysrec, sizeof(TLOG_SYSREC_DATA_T) * count);
	}

	tlogx->count = 0;
	memset(tlogx->data, 0x00, sizeof(TLOG_DATA_T) * count);
	memset(tlogx->sysrec, 0x00, sizeof(TLOG_SYSREC_DATA_T) * count);
	return 0;
}

static int _set_tlog_request_count(TLOGX_T *tlogx, int count)
{
	tlogx->param.log_param.request_count = count;
	return 0;
}

static LF_ORDER_E _set_tlog_prev_order(TLOGX_T *tlogx)
{
	LF_ORDER_E cur = tlogx->param.log_param.direction;
	if (tlogx->param.log_param.direction == LF_OLDEST) tlogx->param.log_param.direction = LF_LATEST;
	else tlogx->param.log_param.direction = LF_OLDEST;
	return cur;
}

static int _set_tlog_next_order(TLOGX_T *tlogx)
{
	return 0;
}

static int _save_tlog_logid(TLOGX_T *tlogx)
{
	if (tlogx->count == 0) return -1;
	tlogx->page_top_logid = tlogx->data[0].log_id;
	tlogx->page_bottom_logid = tlogx->data[tlogx->count - 1].log_id;
	return 0;
}

static int _unset_tlog_type_mask(TLOGX_T *tlogx, LF_CAT_E lcat)
{
	tlogx->param.log_param.type_mask &= ~(ilog_type[lcat]);
	return 0;
}

static int _get_dlog_sysrec(NF_DVA_LOG_PARAM *param, NF_DVA_LOG_RESULT_HEADER *result, SYSREC_DATA_T *psysrec)
{
// for debugging	
//_print_log_param_mask(param);

	gboolean ret = nf_dva_eventlog_get(param, result, psysrec);

// for_debugging
//if (ret == TRUE) _print_log_result(result, psysrec);

	return (ret == TRUE ? 0 : -1);	
}

static int _make_dlog(SYSREC_DATA_T *psysrec, LOG_DATA_T *log, int cnt)
{
	int i;

	for (i = 0; i < cnt; i++) {	
		GUINT64_TO_GTIMEVAL(psysrec[i].timestamp, log[i].tvTime);
		log[i].log_id = psysrec[i].log_id;

		_parse_log(&psysrec[i], &log[i]);	
	}
	
	return 0;
}

static int _make_dlog_r(SYSREC_DATA_T *psysrec, LOG_DATA_T *log, int cnt)
{
	int i;
	int j;

	for (i = 0, j = cnt - 1; i < cnt; i++, j--) {
		GUINT64_TO_GTIMEVAL(psysrec[i].timestamp, log[j].tvTime);
		log[j].log_id = psysrec[i].log_id;

		_parse_log(&psysrec[i], &log[j]);	
	}
	
	return 0;
}

static int _set_dlog_nextid(DLOGX_T *dlogx, guint64 logid)
{
	dlogx->param.log_param.log_id = logid;
	return 0;	
}

static int _set_dlog_search_mode(DLOGX_T *dlogx, NF_LOG_PARAM_MODE_E mode)
{
	dlogx->param.log_param.mode = mode;	
	return 0;
}

static int _set_dlog_search_order(DLOGX_T *dlogx, LF_ORDER_E order)
{
	dlogx->param.log_param.direction = order;	
	if (dlogx->param.log_param.direction == LF_OLDEST) {
		dlogx->param.log_param.time_search = dlogx->param.log_param.time_begin; 
	}
	else {
		dlogx->param.log_param.time_search = dlogx->param.log_param.time_end; 
	}
	return 0;
}

static int _set_dlog_search_time(DLOGX_T *dlogx, GTimeVal *start, GTimeVal *end)
{
	dlogx->param.log_param.time_begin = *start;
	dlogx->param.log_param.time_end = *end;
	DMSG(1, "BEGIN TIME = (%ld), END TIME = (%ld)\n", dlogx->param.log_param.time_begin.tv_sec, dlogx->param.log_param.time_end.tv_sec);

	if (dlogx->param.log_param.direction == LF_OLDEST) {
		dlogx->param.log_param.time_search = dlogx->param.log_param.time_begin; 
		DMSG(1, "(OLDETEST) SEARCH TIME = %ld\n", dlogx->param.log_param.time_search);
	}
	else {
		dlogx->param.log_param.time_search = dlogx->param.log_param.time_end; 
		DMSG(1, "(LATEST) SEARCH TIME = %ld\n", dlogx->param.log_param.time_search);
	}
	return 0;
}

static int _prepare_dlog_data_pool(DLOGX_T *dlogx, int count)
{
	if (dlogx->count == count) {
		if (!dlogx->data) {
			dlogx->data = imalloc(sizeof(LOG_DATA_T) * count);
			dlogx->sysrec = imalloc(sizeof(SYSREC_DATA_T) * count);
		}
	}
	else {
		dlogx->data = irealloc(dlogx->data, sizeof(LOG_DATA_T) * count);
		dlogx->sysrec = irealloc(dlogx->sysrec, sizeof(SYSREC_DATA_T) * count);
	}

	dlogx->count = 0;
	memset(dlogx->data, 0x00, sizeof(LOG_DATA_T) * count);
	memset(dlogx->sysrec, 0x00, sizeof(SYSREC_DATA_T) * count);
	return 0;
}

static int _set_dlog_request_count(DLOGX_T *dlogx, int count)
{
	dlogx->param.log_param.request_count = count;
	return 0;
}

static LF_ORDER_E _set_dlog_prev_order(DLOGX_T *dlogx)
{
	LF_ORDER_E cur = dlogx->param.log_param.direction;
	if (dlogx->param.log_param.direction == LF_OLDEST) dlogx->param.log_param.direction = LF_LATEST;
	else dlogx->param.log_param.direction = LF_OLDEST;
	return cur;
}

static int _set_dlog_next_order(DLOGX_T *dlogx)
{
	return 0;
}

static int _save_dlog_logid(DLOGX_T *dlogx)
{
	if (dlogx->count == 0) return -1;
	dlogx->page_top_logid = dlogx->data[0].log_id;
	dlogx->page_bottom_logid = dlogx->data[dlogx->count - 1].log_id;
	return 0;
}

static int _unset_dlog_type_mask(DLOGX_T *dlogx, LF_CAT_E lcat)
{
	dlogx->param.log_param.type_mask &= ~(ilog_type[lcat]);
	return 0;
}



////////////////////////////////////////////////////////////
//
// public interfaces
//



////////////////////////////////////////////////////////////
// General log

LOGX_T *logx_create(int channel)
{
	LOGX_T *logx;

	logx = imalloc(sizeof(LOGX_T));
	memset(logx, 0x00, sizeof(LOGX_T));
	_init_param(&logx->param);

	return logx;
}

int logx_destroy(LOGX_T *logx)
{
	if (logx->data) ifree(logx->data);
	memset(logx, 0x00, sizeof(LOGX_T));
	ifree(logx);

	return 0;	
}

int logx_reset_log_filter(LOGX_T *logx, LF_RESET_E rtype)
{
	_init_param(&logx->param);

	if (rtype == LF_ALL) _doset_all_mask(&logx->param);
	else _unset_all_mask(&logx->param);

	return 0;
}

int logx_set_log_filter_ch(LOGX_T *logx, int ch, int onoff)
{
	int i;
	
	for (i = 0; i < CNT_LT_EVTMASK_HAS_CH; ++i) {
		if (onoff == 1) _doset_evtmaskbit_has_ch(&logx->param, i, ch);
		else _unset_evtmaskbit_has_ch(&logx->param, i, ch);
	}

	_unset_type_mask(logx, LF_CAT_SYSTEM);
	_unset_type_mask(logx, LF_CAT_STORAGE);
	_unset_type_mask(logx, LF_CAT_SETUP);
	_unset_type_mask(logx, LF_CAT_NETWORK);

	return 0;
}

int logx_set_log_filter_type(LOGX_T *logx, unsigned int chmask, LF_CAT_E lcat, int onoff)
{
	switch (lcat) {
	case LF_CAT_ALL:
		if (onoff == 1)	{
			_doset_all_mask(&logx->param);
		}
		else {
			_unset_all_mask(&logx->param);
		}
		break;

	case LF_CAT_ALARM:
		if (onoff == 1)	{
			_doset_evtmask_has_ch(&logx->param, chmask, LTIDX_SENSOR_INPUT);
		}
		else {
			_unset_evtmask_has_ch(&logx->param, chmask, LTIDX_SENSOR_INPUT);
		}
		break;	

	case LF_CAT_MOTION:
		if (onoff == 1)	{
			_doset_evtmask_has_ch(&logx->param, chmask, LTIDX_MOTION_DETECTION);
		}
		else {
			_unset_evtmask_has_ch(&logx->param, chmask, LTIDX_MOTION_DETECTION);
		}
		break;

	case LF_CAT_VLOSS:
		if (onoff == 1) {
			_doset_evtmask_has_ch(&logx->param, chmask, LTIDX_VIDEO_IN);
			_doset_evtmask_has_ch(&logx->param, chmask, LTIDX_VIDEO_LOSS);
		}
		else {
			_unset_evtmask_has_ch(&logx->param, chmask, LTIDX_VIDEO_IN);
			_unset_evtmask_has_ch(&logx->param, chmask, LTIDX_VIDEO_LOSS);
		}
		break;

	case LF_CAT_RECORD:
		if (onoff == 1) {
			_doset_evtmask_has_ch(&logx->param, chmask, LTIDX_RECORD_STARTED);
			_doset_evtmask_has_ch(&logx->param, chmask, LTIDX_RECORD_STOPPED);
		}
		else {
			_unset_evtmask_has_ch(&logx->param, chmask, LTIDX_RECORD_STARTED);
			_unset_evtmask_has_ch(&logx->param, chmask, LTIDX_RECORD_STOPPED);
		}
		break;

	case LF_CAT_SYSTEM:
		// following codes are special code for messy log protocol
		if (onoff == 1) {
			_doset_evtmask_has_ch(&logx->param, chmask, LTIDX_SYSTEM_EVENT);
		}
		else {
			_unset_evtmask_has_ch(&logx->param, chmask, LTIDX_SYSTEM_EVENT);
		}
		break;

	case LF_CAT_STORAGE:
	case LF_CAT_SETUP:
		break;

	case LF_CAT_NETWORK:
		if (onoff == 1) {
			_doset_evtmask_has_ch(&logx->param, chmask, LTIDX_NETWORK);
		}
		else {
			_unset_evtmask_has_ch(&logx->param, chmask, LTIDX_NETWORK);
		}
		break;

	case LF_CAT_TAMPER:
		if (onoff == 1)	{
			_doset_evtmask_has_ch(&logx->param, chmask, LTIDX_TAMPER);
		}
		else {
			_unset_evtmask_has_ch(&logx->param, chmask, LTIDX_TAMPER);
		}
		break;	

	case LF_CAT_IPCAM:
		if (onoff == 1) {
			_doset_evtmask_has_ch(&logx->param, chmask, LTIDX_IPCAM);
		}
		else {
			_unset_evtmask_has_ch(&logx->param, chmask, LTIDX_IPCAM);
		}
		break;

	case LF_CAT_VCA:
		if (onoff == 1) {
			_doset_evtmask_has_ch(&logx->param, chmask, LTIDX_VCA);
		}
		else {
			_unset_evtmask_has_ch(&logx->param, chmask, LTIDX_VCA);
		}
		break;

	case LF_CAT_DVA:
		if (onoff == 1) {
			_doset_evtmask_has_ch(&logx->param, chmask, LTIDX_DVA);
		}
		else {
			_unset_evtmask_has_ch(&logx->param, chmask, LTIDX_DVA);
		}
		break;

	case LF_CAT_POS:
		if (onoff == 1) {
			_doset_evtmask_has_ch(&logx->param, chmask, LTIDX_POS);
		}
		else {
			_unset_evtmask_has_ch(&logx->param, chmask, LTIDX_POS);
		}
		break;

	default:
		break;
	}

	if (onoff == 1) logx->param.type_mask |= ilog_type[lcat];
	else logx->param.type_mask &= ~(ilog_type[lcat]);

	return 0;
}

int logx_get_log(LOGX_T *logx, GTimeVal *start, GTimeVal *end, int count, LOG_DATA_T *log)
{
	NF_LOG_RESULT_HEADER result;
	
	_prepare_data_pool(logx, count);

	_set_request_count(logx, count);
	_set_search_mode(logx, NF_LOG_PARAM_MODE_TIME);
	_set_search_time(logx, start, end); 

	if (_get_sysrec(&logx->param, &result, logx->sysrec) == -1) return -1;

	_make_log(logx->sysrec, logx->data, result.result_count); 
	logx->count = result.result_count;

	_save_logid(logx);

	memcpy(log, logx->data, sizeof(LOG_DATA_T) * result.result_count);
	return result.result_count;
}

int logx_get_log_next(LOGX_T *logx, int count, LOG_DATA_T *log)
{
	NF_LOG_RESULT_HEADER result;
	guint64 logid;

	if (logx->count == 0) logid = logx->page_bottom_logid;
	else logid = logx->data[logx->count - 1].log_id;

	_prepare_data_pool(logx, count);
	
	_set_request_count(logx, count);
	_set_search_mode(logx, NF_LOG_PARAM_MODE_LOGID);
	_set_nextid(logx, logid); 

	if (_get_sysrec(&logx->param, &result, logx->sysrec) == -1) return -1;

	_make_log(logx->sysrec, logx->data, result.result_count); 
	logx->count = result.result_count;

	_save_logid(logx);

	memcpy(log, logx->data, sizeof(LOG_DATA_T) * result.result_count);
	return result.result_count;
}

int logx_has_log_next_next(LOGX_T *logx, int count)
{
	NF_LOG_RESULT_HEADER result;
	guint64 logid;

	if (logx->count == 0) return 0;
	else logid = logx->data[logx->count - 1].log_id;

	_prepare_data_pool(logx, count);
	
	_set_request_count(logx, count);
	_set_search_mode(logx, NF_LOG_PARAM_MODE_LOGID);
	_set_nextid(logx, logid); 

	if (_get_sysrec(&logx->param, &result, logx->sysrec) == -1) return 0;
	return result.result_count > 0 ? 1 : 0;
}

int logx_get_log_prev(LOGX_T *logx, int count, LOG_DATA_T *log)
{
	NF_LOG_RESULT_HEADER result;
	guint64 logid;
	LF_ORDER_E cur;

	if (logx->count == 0) logid = logx->page_top_logid;
	else logid = logx->data[0].log_id;

	_prepare_data_pool(logx, count);

	_set_request_count(logx, count);
	_set_search_mode(logx, NF_LOG_PARAM_MODE_LOGID);
	cur = _set_prev_order(logx);
	_set_nextid(logx, logid); 

	if (_get_sysrec(&logx->param, &result, logx->sysrec) == -1) return -1;

	_make_log_r(logx->sysrec, logx->data, result.result_count); 
	logx->count = result.result_count;

	logx->param.direction = cur;
	_save_logid(logx);

	memcpy(log, logx->data, sizeof(LOG_DATA_T) * result.result_count);
	return result.result_count;
}

int logx_has_log_prev_prev(LOGX_T *logx, int count)
{
	NF_LOG_RESULT_HEADER result;
	guint64 logid;
	LF_ORDER_E cur;

	if (logx->count == 0) return 0;
	else logid = logx->data[0].log_id;

	_prepare_data_pool(logx, count);

	_set_request_count(logx, count);
	_set_search_mode(logx, NF_LOG_PARAM_MODE_LOGID);
	cur = _set_prev_order(logx);
	_set_nextid(logx, logid); 

	if (_get_sysrec(&logx->param, &result, logx->sysrec) == -1) return 0;
	logx->param.direction = cur;
	return result.result_count > 0 ? 1 : 0;
}

int logx_set_log_filter_order(LOGX_T *logx, LF_ORDER_E order)
{
	return _set_search_order(logx, order);
}

int logx_put_log(PUTLOG_TYPE_E type, int param1, int param2, char *text)		// param2 is just dummy
{
	int cat = _LCAT(type);
	int sub = _LSUB(type);
	if(!nf_eventlog_put_param(NULL, cat, param1, sub, text))
		g_warning("[%d] : %s returns FALSE", __LINE__, __FUNCTION__); 

	return 0;
}



////////////////////////////////////////////////////////////
// pos log

TLOGX_T *tlogx_create(int channel)
{
	TLOGX_T *tlogx;

	tlogx = imalloc(sizeof(TLOGX_T));
	memset(tlogx, 0x00, sizeof(TLOGX_T));
	_init_param(&tlogx->param.log_param);
	tlogx->param.log_param.type_mask = ilog_type[LF_CAT_POS];

	return tlogx;
}

int tlogx_destroy(TLOGX_T *tlogx)
{
	if (tlogx->data) ifree(tlogx->data);
	memset(tlogx, 0x00, sizeof(TLOGX_T));
	ifree(tlogx);

	return 0;	
}

int tlogx_reset_tlog_filter(TLOGX_T *tlogx)
{
	memset(&tlogx->param, 0x00, sizeof(NF_POS_LOG_PARAM));
	_init_param(&tlogx->param.log_param);
	tlogx->param.log_param.type_mask = ilog_type[LF_CAT_POS];

	return 0;
}

int tlogx_set_tlog_filter_ch(TLOGX_T *tlogx, int ch, int onoff)
{
	int i;
	
	if (onoff == 1) _doset_evtmaskbit_has_ch(&tlogx->param.log_param, LTIDX_POS, ch);
	else _unset_evtmaskbit_has_ch(&tlogx->param.log_param, LTIDX_POS, ch);

	return 0;
}

int tlogx_set_tlog_filter_text(TLOGX_T *tlogx, unsigned int *key_info, gboolean match_case, gboolean match_whole)
{
    int i;
    
	tlogx->param.search_cond = NF_POS_LOG_SEARCH_COND_NA;
	tlogx->param.match_case = (guchar)match_case;
	tlogx->param.match_whole = (guchar)match_whole;
	for (i = 0; i < MAX_STR_NUM_CNT; i++) {
        strcpy(tlogx->param.search_str[i], ((SEARCH_KEY_T*)key_info)->keyword[i]);   
//        g_message("###yanggungg : %s, %d, tlogx->param.search_str[i] : %s", __FUNCTION__, __LINE__, tlogx->param.search_str[i]);
    }
    
	for (i = 0; i < MAX_STR_OPER_CNT; i++) {
        tlogx->param.operation[i] = ((SEARCH_KEY_T*)key_info)->oper[i];   
//        g_message("###yanggungg : %s, %d, tlogx->param.operation[i] : %d", __FUNCTION__, __LINE__, tlogx->param.operation[i]);
    }

	return 0;
}

int tlogx_get_tlog(TLOGX_T *tlogx, GTimeVal *start, GTimeVal *end, int count, LOG_DATA_T *log)
{
	NF_POS_LOG_RESULT_HEADER result;
	
	_prepare_tlog_data_pool(tlogx, count);

	_set_tlog_request_count(tlogx, count);
	_set_tlog_search_mode(tlogx, NF_LOG_PARAM_MODE_TIME);
	_set_tlog_search_time(tlogx, start, end); 

	if (_get_tlog_sysrec(&tlogx->param, &result, tlogx->sysrec) == -1) return -1;

	_make_tlog(tlogx->sysrec, tlogx->data, result.log_result_header.result_count); 
	tlogx->count = result.log_result_header.result_count;

	_save_tlog_logid(tlogx);

	memcpy(log, tlogx->data, sizeof(TLOG_DATA_T) * result.log_result_header.result_count);
	return result.log_result_header.result_count;
}

int tlogx_get_tlog_next(TLOGX_T *tlogx, int count, LOG_DATA_T *log)
{
	NF_POS_LOG_RESULT_HEADER result;
	guint64 logid;

	if (tlogx->count == 0) logid = tlogx->page_bottom_logid;
	else logid = tlogx->data[tlogx->count - 1].log_id;

	_prepare_tlog_data_pool(tlogx, count);
	
	_set_tlog_request_count(tlogx, count);
	_set_tlog_search_mode(tlogx, NF_LOG_PARAM_MODE_LOGID);
	_set_tlog_nextid(tlogx, logid); 

	if (_get_tlog_sysrec(&tlogx->param, &result, tlogx->sysrec) == -1) return -1;

	_make_tlog(tlogx->sysrec, tlogx->data, result.log_result_header.result_count); 
	tlogx->count = result.log_result_header.result_count;

	_save_tlog_logid(tlogx);

	memcpy(log, tlogx->data, sizeof(TLOG_DATA_T) * result.log_result_header.result_count);
	return result.log_result_header.result_count;
}

int tlogx_has_tlog_next_next(TLOGX_T *tlogx, int count)
{
	NF_POS_LOG_RESULT_HEADER result;
	guint64 logid;

	if (tlogx->count == 0) return 0;
	else logid = tlogx->data[tlogx->count - 1].log_id;

	_prepare_tlog_data_pool(tlogx, count);
	
	_set_tlog_request_count(tlogx, count);
	_set_tlog_search_mode(tlogx, NF_LOG_PARAM_MODE_LOGID);
	_set_tlog_nextid(tlogx, logid); 

	if (_get_tlog_sysrec(&tlogx->param, &result, tlogx->sysrec) == -1) return 0;
	return result.log_result_header.result_count > 0 ? 1 : 0;
}

int tlogx_get_tlog_prev(TLOGX_T *tlogx, int count, LOG_DATA_T *log)
{
	NF_POS_LOG_RESULT_HEADER result;
	guint64 logid;
	LF_ORDER_E cur;

	if (tlogx->count == 0) logid = tlogx->page_top_logid;
	else logid = tlogx->data[0].log_id;

	_prepare_tlog_data_pool(tlogx, count);

	_set_tlog_request_count(tlogx, count);
	_set_tlog_search_mode(tlogx, NF_LOG_PARAM_MODE_LOGID);
	cur = _set_tlog_prev_order(tlogx);
	_set_tlog_nextid(tlogx, logid); 

	if (_get_tlog_sysrec(&tlogx->param, &result, tlogx->sysrec) == -1) return -1;

	_make_tlog_r(tlogx->sysrec, tlogx->data, result.log_result_header.result_count); 
	tlogx->count = result.log_result_header.result_count;

	tlogx->param.log_param.direction = cur;
	_save_tlog_logid(tlogx);

	memcpy(log, tlogx->data, sizeof(TLOG_DATA_T) * result.log_result_header.result_count);
	return result.log_result_header.result_count;
}

int tlogx_has_tlog_prev_prev(TLOGX_T *tlogx, int count)
{
	NF_POS_LOG_RESULT_HEADER result;
	guint64 logid;
	LF_ORDER_E cur;

	if (tlogx->count == 0) return 0;
	else logid = tlogx->data[0].log_id;

	_prepare_tlog_data_pool(tlogx, count);

	_set_tlog_request_count(tlogx, count);
	_set_tlog_search_mode(tlogx, NF_LOG_PARAM_MODE_LOGID);
	cur = _set_tlog_prev_order(tlogx);
	_set_tlog_nextid(tlogx, logid); 

	if (_get_tlog_sysrec(&tlogx->param, &result, tlogx->sysrec) == -1) return 0;
	tlogx->param.log_param.direction = cur;
	return result.log_result_header.result_count > 0 ? 1 : 0;
}

int tlogx_set_tlog_filter_order(TLOGX_T *tlogx, LF_ORDER_E order)
{
	return _set_tlog_search_order(tlogx, order);
}



////////////////////////////////////////////////////////////
// dva log

DLOGX_T *dlogx_create(int channel)
{
	DLOGX_T *dlogx;

	dlogx = imalloc(sizeof(DLOGX_T));
	memset(dlogx, 0x00, sizeof(DLOGX_T));
	_init_param(&dlogx->param.log_param);
	dlogx->param.log_param.type_mask = ilog_type[LF_CAT_DVA];

	return dlogx;
}

int dlogx_destroy(DLOGX_T *dlogx)
{
	if (dlogx->data) ifree(dlogx->data);
	memset(dlogx, 0x00, sizeof(DLOGX_T));
	ifree(dlogx);

	return 0;	
}

int dlogx_reset_dlog_filter(DLOGX_T *dlogx)
{
	memset(&dlogx->param, 0x00, sizeof(NF_DVA_LOG_PARAM));
	_init_param(&dlogx->param.log_param);
	dlogx->param.log_param.type_mask = ilog_type[LF_CAT_DVA];

	return 0;
}

int dlogx_set_dlog_filter_ch(DLOGX_T *dlogx, int ch, int onoff)
{
	int i;
	
	if (onoff == 1) _doset_evtmaskbit_has_ch(&dlogx->param.log_param, LTIDX_DVA, ch);
	else _unset_evtmaskbit_has_ch(&dlogx->param.log_param, LTIDX_DVA, ch);

	return 0;
}

int dlogx_set_dlog_filter_algorithm(DLOGX_T *dlogx, guint algorithm)
{
	dlogx->param.dva_type_mask = algorithm;
	dlogx->param.match_case = 1;
	dlogx->param.match_whole = 0;

	return 0;
}

int dlogx_set_dlog_filter_event(DLOGX_T *dlogx, guint event_mask)
{
	dlogx->param.event_mask = event_mask;
	return 0;
}

int dlogx_set_dlog_filter_sub(DLOGX_T *dlogx, gchar oper[2], gboolean match_case, gboolean match_whole)
{
	dlogx->param.operation[0] = oper[0];
	dlogx->param.operation[1] = oper[1];
	dlogx->param.match_case = match_case;
	dlogx->param.match_whole = match_whole;

    return 0;
}

int dlogx_set_dlog_filter_evt_text(DLOGX_T *dlogx, gchar *evt)
{
	dlogx->param.caption_list = evt;
	if	(dlogx->param.caption_list == NULL)	dlogx->param.caption_list_len = 0;
	else	dlogx->param.caption_list_len = strlen(evt);

    return 0;
}

int dlogx_set_dlog_filter_group_mask(DLOGX_T *dlogx, guint group_mask)
{
	dlogx->param.lpr_grp_mask = group_mask;
	return 0;
}

int dlogx_set_dlog_filter_text(DLOGX_T *dlogx, gchar *key_str, gchar *key_str1, gchar *key_str2)
{
    g_snprintf(dlogx->param.search_str[0], sizeof(dlogx->param.search_str[0]), "%s", key_str);
	dlogx->param.search_str_len[0] = strlen(key_str);

	g_snprintf(dlogx->param.search_str[1], sizeof(dlogx->param.search_str[1]), "%s", key_str1);
	dlogx->param.search_str_len[1] = strlen(key_str1);

	g_snprintf(dlogx->param.search_str[2], sizeof(dlogx->param.search_str[2]), "%s", key_str2);
	dlogx->param.search_str_len[2] = strlen(key_str2);

    return 0;
}

int dlogx_set_dlog_filter_enable(DLOGX_T *dlogx, gchar *key_str, guint name_search, guint group_search, guint gender_search)
{  
	//g_snprintf(dlogx->param.search_str, sizeof(dlogx->param.search_str), "%s", key_str);
	if (name_search)
		dlogx->param.search_str_enable = 1;
	if (group_search)
		dlogx->param.search_str1_enable = 1;
	if (gender_search)
		dlogx->param.search_str2_enable = 1;
	if (group_search == 2 || group_search == 3)
		dlogx->param.fr_unassinged_group_enable = 1;
	if (group_search >= 3)
		dlogx->param.fr_unregistered_enable = 1;
	return 0;
}

int dlogx_get_dlog(DLOGX_T *dlogx, GTimeVal *start, GTimeVal *end, int count, LOG_DATA_T *log)
{
	NF_DVA_LOG_RESULT_HEADER result;
	
	_prepare_dlog_data_pool(dlogx, count);

	_set_dlog_request_count(dlogx, count);
	_set_dlog_search_mode(dlogx, NF_LOG_PARAM_MODE_TIME);
	_set_dlog_search_time(dlogx, start, end); 

	if (_get_dlog_sysrec(&dlogx->param, &result, dlogx->sysrec) == -1) return -1;

	_make_dlog(dlogx->sysrec, dlogx->data, result.log_result_header.result_count); 
	dlogx->count = result.log_result_header.result_count;

	_save_dlog_logid(dlogx);

	memcpy(log, dlogx->data, sizeof(LOG_DATA_T) * result.log_result_header.result_count);
	return result.log_result_header.result_count;
}

int dlogx_get_dlog_next(DLOGX_T *dlogx, int count, LOG_DATA_T *log)
{
	NF_DVA_LOG_RESULT_HEADER result;
	guint64 logid;

	if (dlogx->count == 0) logid = dlogx->page_bottom_logid;
	else logid = dlogx->data[dlogx->count - 1].log_id;

	_prepare_dlog_data_pool(dlogx, count);
	
	_set_dlog_request_count(dlogx, count);
	_set_dlog_search_mode(dlogx, NF_LOG_PARAM_MODE_LOGID);
	_set_dlog_nextid(dlogx, logid); 

	if (_get_dlog_sysrec(&dlogx->param, &result, dlogx->sysrec) == -1) return -1;

	_make_dlog(dlogx->sysrec, dlogx->data, result.log_result_header.result_count); 
	dlogx->count = result.log_result_header.result_count;

	_save_dlog_logid(dlogx);

	memcpy(log, dlogx->data, sizeof(LOG_DATA_T) * result.log_result_header.result_count);
	return result.log_result_header.result_count;
}

int dlogx_has_dlog_next_next(DLOGX_T *dlogx, int count)
{
	NF_DVA_LOG_RESULT_HEADER result;
	guint64 logid;

	if (dlogx->count == 0) return 0;
	else logid = dlogx->data[dlogx->count - 1].log_id;

	_prepare_dlog_data_pool(dlogx, count);
	
	_set_dlog_request_count(dlogx, count);
	_set_dlog_search_mode(dlogx, NF_LOG_PARAM_MODE_LOGID);
	_set_dlog_nextid(dlogx, logid); 

	if (_get_dlog_sysrec(&dlogx->param, &result, dlogx->sysrec) == -1) return 0;
	return result.log_result_header.result_count > 0 ? 1 : 0;
}

int dlogx_get_dlog_prev(DLOGX_T *dlogx, int count, LOG_DATA_T *log)
{
	NF_DVA_LOG_RESULT_HEADER result;
	guint64 logid;
	LF_ORDER_E cur;

	if (dlogx->count == 0) logid = dlogx->page_top_logid;
	else logid = dlogx->data[0].log_id;

	_prepare_dlog_data_pool(dlogx, count);

	_set_dlog_request_count(dlogx, count);
	_set_dlog_search_mode(dlogx, NF_LOG_PARAM_MODE_LOGID);
	cur = _set_dlog_prev_order(dlogx);
	_set_dlog_nextid(dlogx, logid); 

	if (_get_dlog_sysrec(&dlogx->param, &result, dlogx->sysrec) == -1) return -1;

	_make_dlog_r(dlogx->sysrec, dlogx->data, result.log_result_header.result_count); 
	dlogx->count = result.log_result_header.result_count;

	dlogx->param.log_param.direction = cur;
	_save_dlog_logid(dlogx);

	memcpy(log, dlogx->data, sizeof(LOG_DATA_T) * result.log_result_header.result_count);
	return result.log_result_header.result_count;
}

int dlogx_has_dlog_prev_prev(DLOGX_T *dlogx, int count)
{
	NF_DVA_LOG_RESULT_HEADER result;
	guint64 logid;
	LF_ORDER_E cur;

	if (dlogx->count == 0) return 0;
	else logid = dlogx->data[0].log_id;

	_prepare_dlog_data_pool(dlogx, count);

	_set_dlog_request_count(dlogx, count);
	_set_dlog_search_mode(dlogx, NF_LOG_PARAM_MODE_LOGID);
	cur = _set_dlog_prev_order(dlogx);
	_set_dlog_nextid(dlogx, logid); 

	if (_get_dlog_sysrec(&dlogx->param, &result, dlogx->sysrec) == -1) return 0;
	dlogx->param.log_param.direction = cur;
	return result.log_result_header.result_count > 0 ? 1 : 0;
}

int dlogx_set_dlog_filter_order(DLOGX_T *dlogx, LF_ORDER_E order)
{
	return _set_dlog_search_order(dlogx, order);
}

