/*
 * scm_raid.c
 * 	- 
 *	- dependencies :
 *		
 *
 * Written by seongho jeong
 * Copyright (c) ITX security, Sep 24, 2012
 *
 */

#include "iux_afx.h"
#include "ix_mem.h"
#include "ix_func.h"
#include "scm_internal.h"
#include "scm.h" 


#define SUPPORTED_CONTROLLER_CNT	1
#define EVENTINFO_BUF_SIZE			20

//#define NOT_SUPPORT_API		1	

////////////////////////////////////////////////////////////
//
// public data type 
//

typedef struct _DISK_RAID_DATA {
	int 			count;
	CONTROLLER		controller[SUPPORTED_CONTROLLER_CNT];
	DISK_CAPINFO_T	capinfo[SUPPORTED_CONTROLLER_CNT];
}DISK_RAID_DATA;

static DISK_RAID_DATA	rd;
//TODO: renaming
static DISK_RAIDINFO_T  rc[2];		// easy raid configuration 
static DISK_RAIDINFO_T  rc2[2];		// advanced raid configuration 


////////////////////////////////////////////////////////////
//
// private functions
//


static int get_disk_raid_ctrl()
{
	DISK_CAPINFO_T cap_info[2];
	CONTROLLER ctrl[SUPPORTED_CONTROLLER_CNT];
	guchar ctrl_cnt;
	gint disk_group;
	gint i, j;

	g_message("%s(%d) : called!!", __FUNCTION__, __LINE__);

	memset(&rd, 0x00, sizeof(DISK_RAID_DATA));
	memset(rc, 0x00, sizeof(DISK_RAIDINFO_T)*2);
	memset(rc2, 0x00, sizeof(DISK_RAIDINFO_T)*2);

	nf_raid_get_ctrl_cnt(&ctrl_cnt); 
	g_message("%s(%d) : nf_raid_get_ctrl_cnt[%d]", __FUNCTION__, __LINE__, ctrl_cnt);
	
	if(ctrl_cnt <= 0) return -1; 
	else 			  rd.count = ctrl_cnt;
	
	for(i=0; i<1 /* SUPPORTED_CONTROLLER_CNT */; i++) {
		nf_raid_get_ctrl_info((guchar)i, &ctrl[i], NULL);
	}

	for(i=0; i<SUPPORTED_CONTROLLER_CNT; i++) {
		scm_get_disk_capinfo(i, &cap_info[i]);

		g_printf("%s : \ndisk group: %d\ndisk count: %d\ndisk size: %d\n", 
				__FUNCTION__, 
				i, 
				cap_info[i].tdisk_count,
				cap_info[i].tsize
				);

		for(j=0; j<5; j++)  {
			g_printf("disk[%d] model: %s \n", 
					j,
					cap_info[i].disk_unit[j].model
					);
		}

		/*
		if(i == 0 && strstr(cap_info[i].disk_unit[0].model, "RAID")) 	// internal
			disk_group = 0;
		else if(i == 1 && strstr(cap_info[i].disk_unit[0].model, "RAID"))  // external
			disk_group = 1;
		*/
	}

	/*
	if(rd.count == 1)
		memcpy(&rd.controller[disk_group], &ctrl[0], sizeof(CONTROLLER));
		*/
	memcpy(&rd.capinfo[0], &cap_info[0], sizeof(DISK_CAPINFO_T));

	if(rd.count == 1) {
		//if(cap_info[1].tdisk_count)				// external
			//memcpy(&rd.controller[1], &ctrl[0], sizeof(CONTROLLER));
		//else									// internal
	g_message("%s(%d) : tdisk_count[%d]", __FUNCTION__, __LINE__, rd.capinfo[0].tdisk_count);
			memcpy(&rd.controller[0], &ctrl[0], sizeof(CONTROLLER));
	}
	else if(rd.count > 1){
		memcpy(rd.controller, ctrl, sizeof(ctrl));
	}
	else
		return -1;

	return 0;
}

static int get_disk_raid_info(DISK_RAIDINFO_T *disk_rinfo)
{
	gint i, j, k;
	gint rid;
	gint mode;
	gint sata_port;
	
	g_message("%s(%d) : called!!", __FUNCTION__, __LINE__);
	
	for(i=0; i<SUPPORTED_CONTROLLER_CNT; i++) {
		for(rid=0, j=0; rid<MAX_RAID_IN_CONTROLLER; rid++) {
			if(!rd.controller[i].Raid[rid].RaidMemberCount || rd.controller[i].Raid[rid].RaidMemberCount <= 1)
				continue;

			disk_rinfo[i].mode |= (1 << rd.controller[i].Raid[rid].RaidLevel); // all raid mode
			disk_rinfo[i].raid_cnt = rid+1; // raid cnt
			disk_rinfo[i].rinfo[rid].raid_id = rid; // raid id
			disk_rinfo[i].rinfo[rid].raid_mode = rd.controller[i].Raid[rid].RaidLevel; // raid mode
			disk_rinfo[i].rinfo[rid].member_count = rd.controller[i].Raid[rid].RaidMemberCount;	// member count
			disk_rinfo[i].rinfo[rid].capacity = (rd.controller[i].Raid[rid].Capacity * 32)/1024; // capacity
			
			// RAID10
			if( disk_rinfo[i].rinfo[rid].raid_mode == RAID_LEVEL_1 
				&& disk_rinfo[i].rinfo[rid].member_count > 2)
				disk_rinfo[i].rinfo[rid].capacity /= 2;
					
			disk_rinfo[i].rinfo[rid].status = rd.controller[i].Raid[rid].RaidStatus; // status

			for(k=0; k<rd.controller[i].Raid[rid].RaidMemberCount; k++) {
				sata_port = rd.controller[i].Raid[rid].RaidMember[k].SATAPort;

				if (rd.controller[i].Raid[rid].RaidMember[k].Ready == 1)	
				disk_rinfo[i].rinfo[rid].sata_index[k] = rd.controller[i].Raid[rid].RaidMember[k].SATAPort; // sata index
				else
					disk_rinfo[i].rinfo[rid].sata_index[k] = -1;
					
				disk_rinfo[i].rinfo[rid].sata_capacity[k] = (rd.controller[i].Sata[sata_port].Capacity*32)/1024; // sata capacity
#if 0
				if(disk_rinfo[i].rinfo[rid].raid_mode == RAID_LEVEL_1)
					disk_rinfo[i].rinfo[rid].unused[k] = ((rd.controller[i].Sata[sata_port].Capacity*32)/1024) - disk_rinfo[i].rinfo[rid].capacity; // unused
				else
					disk_rinfo[i].rinfo[rid].unused[k] = 0; // unused
				
				if(disk_rinfo[i].rinfo[rid].unused[k] < 0) disk_rinfo[i].rinfo[rid].unused[k] *= -1;
				g_printf(">>>>>>> sata[%d] capa: %d\n", k, (rd.controller[i].Sata[sata_port].Capacity*32/1024));
#endif

				strcpy(disk_rinfo[i].rinfo[rid].model[k], rd.controller[i].Sata[sata_port].ModelName);	// model name
			}
			
			nf_raid_get_rebuild_per((guchar)i, (guchar)rid, &disk_rinfo[i].rinfo[rid].rebuild, NULL); // rebuild percent


			//TODO: need ??
			//j += rd.controller[i].Raid[rid].RaidMemberCount;
		}
	}

#if 0
	for(i=0; i<SUPPORTED_CONTROLLER_CNT; i++) {
		g_printf("\nmode: %d\n", disk_rinfo[i].mode);
		for(j=0; j<5; j++) {
			g_printf("[%d]\nraid cnt: %d\nraid mode: %d\nmember cnt: %d\nmodel: %s %s %s %s %s\ncapacity: %d\nunused: %d %d %d %d %d\nstatus: %d\nrebuild: %d\nsata_index: %d %d %d %d %d\n\n", 
				j, 
				disk_rinfo[i].raid_cnt, 
				disk_rinfo[i].rinfo[j].raid_mode, 
				disk_rinfo[i].rinfo[j].member_count, 
				disk_rinfo[i].rinfo[j].model[0], 
				disk_rinfo[i].rinfo[j].model[1], 
				disk_rinfo[i].rinfo[j].model[2], 
				disk_rinfo[i].rinfo[j].model[3], 
				disk_rinfo[i].rinfo[j].model[4], 
				disk_rinfo[i].rinfo[j].capacity, 
				disk_rinfo[i].rinfo[j].unused[0], 
				disk_rinfo[i].rinfo[j].unused[1], 
				disk_rinfo[i].rinfo[j].unused[2], 
				disk_rinfo[i].rinfo[j].unused[3], 
				disk_rinfo[i].rinfo[j].unused[4], 
				disk_rinfo[i].rinfo[j].status, 
				disk_rinfo[i].rinfo[j].rebuild,
				disk_rinfo[i].rinfo[j].sata_index[0],
				disk_rinfo[i].rinfo[j].sata_index[1],
				disk_rinfo[i].rinfo[j].sata_index[2],
				disk_rinfo[i].rinfo[j].sata_index[3],
				disk_rinfo[i].rinfo[j].sata_index[4]
				);
		}
	}
#endif 
	
	if(disk_rinfo[0].raid_cnt == 0 && disk_rinfo[1].raid_cnt == 0)
		return -1;
	
	// copy for advanced conf
	if(disk_rinfo[0].raid_cnt != 0) memcpy(&rc2[0], &disk_rinfo[0], sizeof(DISK_RAIDINFO_T));
	//if(disk_rinfo[1].raid_cnt != 0) memcpy(&rc2[1], &disk_rinfo[1], sizeof(DISK_RAIDINFO_T));
    g_message("%s(%d) : PASS!!", __FUNCTION__, __LINE__);
	return 0;
}

static gint make_disk_internal_raid_info()
{
	DISK_RAIDINFO_T tmp_info;
	//guint64 d_size = 0;
	guint d_size = 0;
	gint d_cnt;
	gint r_cnt;
	gint d_idx;
	gint i, j, k;

	g_message("%s(%d) : called!!", __FUNCTION__, __LINE__);

	memset(&tmp_info, 0x00, sizeof(DISK_RAIDINFO_T));
	tmp_info.mode |= RAID_CONF_MODE;			// RAID CONFIGURATION MODE

	// raid 1 
	if((d_cnt = rd.capinfo[0].tdisk_count) >= 2) {

		tmp_info.mode |= (1 << 1);
		tmp_info.raid_cnt = rd.capinfo[0].tdisk_count/2;
		
		r_cnt = tmp_info.raid_cnt;

		for(i=0, d_idx=0; i<tmp_info.raid_cnt; i++) {

			for(k=0; k<2; k++) {
				if(k == 0)
					d_size = rd.controller[0].Sata[k+(i*2)].Capacity;
				else if(d_size > rd.controller[0].Sata[k+(i*2)].Capacity)
					d_size = rd.controller[0].Sata[k+(i*2)].Capacity;
			}

			tmp_info.rinfo[i].raid_id = i;
			tmp_info.rinfo[i].raid_mode = RAID_LEVEL_1;
			tmp_info.rinfo[i].member_count = 2;
			tmp_info.rinfo[i].capacity = (d_size*32)/1024;
			tmp_info.rinfo[i].status = -1;

			for(j=0; j<2; j++, d_idx++) {
				strcpy(&tmp_info.rinfo[i].model[j], rd.controller[0].Sata[d_idx].ModelName);
#if 0
				tmp_info.rinfo[i].unused[j] = rd.controller[0].Sata[d_idx].Capacity - d_size;
				tmp_info.rinfo[i].unused[j] *= 32;
				tmp_info.rinfo[i].unused[j] /=1024;
#else
				tmp_info.rinfo[i].unused[j]	= 0;
#endif				
				tmp_info.rinfo[i].sata_index[j] = d_idx;
				tmp_info.rinfo[i].sata_capacity[j] = (rd.controller[0].Sata[d_idx].Capacity*32)/1024;
			}

		}
	}

	// raid 5 
	if((d_cnt = rd.capinfo[0].tdisk_count) >= 3) {

		for(i=0; i<d_cnt; i++) {
			if(i == 0)
				d_size = rd.controller[0].Sata[i].Capacity;
			else if(d_size > rd.controller[0].Sata[i].Capacity)
				d_size = rd.controller[0].Sata[i].Capacity;
		}

		tmp_info.mode |= (1 << 5);
		tmp_info.raid_cnt += 1;


		tmp_info.rinfo[r_cnt].raid_id = 0;
		tmp_info.rinfo[r_cnt].raid_mode = RAID_LEVEL_5;
		tmp_info.rinfo[r_cnt].member_count = d_cnt;
		//tmp_info.rinfo[r_cnt].capacity = (d_size*32)/1024;
		tmp_info.rinfo[r_cnt].capacity = (d_cnt - 1) * d_size;
		tmp_info.rinfo[r_cnt].capacity *= 32;
		tmp_info.rinfo[r_cnt].capacity /= 1024;
		tmp_info.rinfo[r_cnt].status = -1;

		for(j=0, d_idx=0; j<d_cnt; j++, d_idx++) {
			strcpy(&tmp_info.rinfo[r_cnt].model[j], rd.controller[0].Sata[d_idx].ModelName);
			tmp_info.rinfo[r_cnt].unused[j] = 0;
			//tmp_info.rinfo[r_cnt].unused[j] = (rd.controller[0].Sata[j].Capacity*32)/1024 - tmp_info.rinfo[r_cnt].capacity;
			//tmp_info.rinfo[r_cnt].unused[j] = rd.controller[0].Sata[j].Capacity - d_size;
			//tmp_info.rinfo[r_cnt].unused[j] *= 32;
			//tmp_info.rinfo[r_cnt].unused[j] /= 1024;
			tmp_info.rinfo[r_cnt].sata_index[j] = d_idx;
			tmp_info.rinfo[r_cnt].sata_capacity[j] = (rd.controller[0].Sata[j].Capacity*32)/1024;
		}

	}


	//memcpy(rinfo, &tmp_info, sizeof(DISK_RAIDINFO_T));
	memcpy(&rc[0], &tmp_info, sizeof(DISK_RAIDINFO_T)); // copy for easy conf

	return 0;
}

static gint make_disk_external_raid_info()
{
	DISK_RAIDINFO_T tmp_info;
	//guint64 d_size = 0;
	guint d_size = 0;
	gint d_cnt;
	gint r_cnt;
	gint d_idx;
	gint i, j, k;
	
	g_message("%s(%d) : called!!", __FUNCTION__, __LINE__);

	memset(&tmp_info, 0x00, sizeof(DISK_RAIDINFO_T));
	tmp_info.mode |= RAID_CONF_MODE;			// RAID CONFIGURATION MODE

	// raid 1 
	if((d_cnt = rd.capinfo[1].tdisk_count) >= 2) {

		tmp_info.mode |= (1 << 1);
		tmp_info.raid_cnt = rd.capinfo[1].tdisk_count/2;
		r_cnt = tmp_info.raid_cnt;

		for(i=0, d_idx=0; i<tmp_info.raid_cnt; i++) {

			for(k=0; k<2; k++) {
				if(k == 0)
					d_size = rd.controller[1].Sata[k+(i*2)].Capacity;
				else if(d_size > rd.controller[1].Sata[k+(i*2)].Capacity)
					d_size = rd.controller[1].Sata[k+(i*2)].Capacity;
			}

			tmp_info.rinfo[i].raid_id = i;
			tmp_info.rinfo[i].raid_mode = RAID_LEVEL_1;
			tmp_info.rinfo[i].member_count = 2;
			tmp_info.rinfo[i].capacity = (d_size*32)/1024;
			tmp_info.rinfo[i].status = -1;

			for(j=0; j<2; j++, d_idx++) {
				strcpy(&tmp_info.rinfo[i].model[j], rd.controller[1].Sata[d_idx].ModelName);
				//tmp_info.rinfo[i].unused[j] = (rd.controller[1].Sata[d_idx].Capacity*32)/1024 - tmp_info.rinfo[i].capacity;
				tmp_info.rinfo[i].unused[j] = rd.controller[1].Sata[d_idx].Capacity - d_size;
				tmp_info.rinfo[i].unused[j] *= 32;
				tmp_info.rinfo[i].unused[j] /= 1024;
				tmp_info.rinfo[i].sata_index[j] = d_idx;
				tmp_info.rinfo[i].sata_capacity[j] = (rd.controller[1].Sata[d_idx].Capacity*32)/1024;
			}
		}
	}


	// raid 5 
	if((d_cnt = rd.capinfo[1].tdisk_count) >= 3) {

		for(i=0; i<d_cnt; i++) {
			if(i == 0)
				d_size = rd.controller[1].Sata[i].Capacity;
			else if(d_size > rd.controller[1].Sata[i].Capacity)
				d_size = rd.controller[1].Sata[i].Capacity;
		}


		tmp_info.mode |= (1 << 5);
		tmp_info.raid_cnt += 1;

		tmp_info.rinfo[r_cnt].raid_id = 0;
		tmp_info.rinfo[r_cnt].raid_mode = RAID_LEVEL_5;
		tmp_info.rinfo[r_cnt].member_count = d_cnt;
		//tmp_info.rinfo[r_cnt].capacity = (d_size*32)/1024;
		tmp_info.rinfo[r_cnt].capacity = (d_cnt - 1) * d_size;
		tmp_info.rinfo[r_cnt].capacity *= 32;
		tmp_info.rinfo[r_cnt].capacity /= 1024;
		tmp_info.rinfo[r_cnt].status = -1;

		for(j=0, d_idx=0; j<d_cnt; j++, d_idx++) {
			strcpy(&tmp_info.rinfo[r_cnt].model[j], rd.controller[1].Sata[j].ModelName);
			tmp_info.rinfo[r_cnt].unused[j] = 0;
			//tmp_info.rinfo[r_cnt].unused[j] = (rd.controller[1].Sata[j].Capacity*32)/1024 - tmp_info.rinfo[r_cnt].capacity;
			//tmp_info.rinfo[r_cnt].unused[j] = rd.controller[1].Sata[j].Capacity - d_size;
			//tmp_info.rinfo[r_cnt].unused[j] *= 32;
			//tmp_info.rinfo[r_cnt].unused[j] /= 1024;
			tmp_info.rinfo[r_cnt].sata_index[j] = d_idx;
			tmp_info.rinfo[r_cnt].sata_capacity[j] = (rd.controller[1].Sata[j].Capacity*32)/1024;
		}

	}

	//memcpy(rinfo, &tmp_info, sizeof(DISK_RAIDINFO_T));
	memcpy(&rc[1], &tmp_info, sizeof(DISK_RAIDINFO_T)); // copy for easy conf

	return 0;
}

static gint make_disk_raid_info()
{
	
	g_message("%s(%d) : called!!", __FUNCTION__, __LINE__);
	
	make_disk_internal_raid_info();
	//make_disk_external_raid_info();

	return 0;
}


static gint get_raid_configuration(gint raid_mode, STRG_TYPE_E strg_type, DISK_RAIDINFO_T *rconf) 
{
	gint i, j;

	g_message("%s(%d) : called!!", __FUNCTION__, __LINE__);
	
	if(!rconf)
		return -1;
	
	if(rc[strg_type].raid_cnt == 0) 
		return -1;

	for(i=0, j=0; i<rc[strg_type].raid_cnt; i++) {
		if(rc[strg_type].rinfo[i].raid_mode == raid_mode) {

			memcpy(&rconf->rinfo[j++], &rc[strg_type].rinfo[i], sizeof(DISK_RAID_T));

			rconf->mode |= RAID_CONF_MODE;
			rconf->raid_cnt = j;
		}
	}

	g_printf("%s: rconf->mode: %d\nrconf->raid_cnt: %d\n", __FUNCTION__, rconf->mode, rconf->raid_cnt);
	
	return 0;
}

static gint get_raid_advanced_configuration(STRG_TYPE_E strg_type, DISK_RAIDINFO_T *rconf)
{
	gint i, j;

	g_message("%s(%d) : called!!", __FUNCTION__, __LINE__);

	if(!rconf)
		return -1;

	if(rc2[strg_type].raid_cnt == 0) 
		return -1;

	if(memcpy(rconf, &rc2[strg_type], sizeof(DISK_RAIDINFO_T)))
		memcpy(rconf, &rc2[strg_type], sizeof(DISK_RAIDINFO_T));

	g_printf("%s: rconf->mode: %d rconf->raid_cnt: %d\n", __FUNCTION__, rconf->mode, rconf->raid_cnt);
	return 0;
}

static gboolean _proc_get_raid_status(gpointer data)
{
	TRANSACTION_E tra = (TRANSACTION_E)data;
	guint ctrl_id = GPOINTER_TO_UINT(iscm.chart[tra].void_data);
	EVENT_TYPE aEventInfo[EVENTINFO_BUF_SIZE];
	guchar evtCnt;
	gint i, raid_status;
	IMSG ret_msg;

	g_message("%s(%d) : called!!", __FUNCTION__, __LINE__);

	evtCnt = 0;
	raid_status = RAID_CONFIGURING;
	memset(aEventInfo, 0x00, sizeof(aEventInfo));

	nf_raid_get_status((guchar)ctrl_id, &evtCnt, aEventInfo, NULL);

	if(evtCnt != 0) {
		for(i=0; i<evtCnt; i++) {
		printf("%s : ctrl_id: %d\nevtCnt: %d\ninfo index: %d\nevt type: %d\nctrl id: %d\nraid id: %d\ndisk id: %d\nraid status: %d\n",
				__FUNCTION__, ctrl_id, evtCnt, i, aEventInfo[i].EventType, aEventInfo[i].ControllerID, aEventInfo[i].RaidID, aEventInfo[i].DiskID, raid_status);
		}
	}

	if(evtCnt != 0) 
	{
		printf(">>>>>>>>>>>>> %s [%d] evtinfo raidid %d >>>>>>>>>>>\n", __FUNCTION__, __LINE__, aEventInfo[0].RaidID);

		if(aEventInfo[0].EventType == NF_RAID_DISKSTATE_RAIDOWNERCHANGE 	  // 21
				|| aEventInfo[0].EventType == NF_RAID_DISKSTATE_NONRAIDTORAID)	// 22
		{ 
			raid_status = RAID_CONF_SUCCESS;
			
			ret_msg = iscm.chart[tra].ret_msg;
    			_scm_send_msg_to_viewer(ret_msg, 0);
			//scm_send_to_viewer(ret_msg, 0, 0, 0);
			return FALSE;
		}
		else if(aEventInfo[0].EventType == NF_RAID_DISKSTATE_ERROR) 
		{
			raid_status = RAID_CONF_FAIL;
    			_scm_send_msg_to_viewer(ret_msg, -1);
			//scm_send_to_viewer(ret_msg, -1, 0, 0);
			return FALSE;
		}
	}

	return TRUE;
}


int _scm_work_create_raid(SCM_T *piscm, TRANSACTION_E tra)
{
/*
	STRG_TYPE_E strg_type;
	DISK_RAIDINFO_T disk_rinfo;
	guint disk_mask = 0;
	guint ctrl_id = 0;
	guint raid_level = 0;
	gint i, j, k;*/
	
	EVENT_TYPE aEventInfo[EVENTINFO_BUF_SIZE];
	STRG_TYPE_E strg_type;
	DISK_RAIDINFO_T disk_rinfo;
	guint disk_mask = 0;
	guint raid_level = 0;
	guchar ctrl_id = 0;
	guchar evtCnt;
	gint raid_status;
	gint i, j, k, q;
	gint h = 0;
	
	g_message("%s(%d) : called!!", __FUNCTION__, __LINE__);
	
	strg_type = iscm.chart[tra].char_data;

	if(iscm.chart[tra].int_data == RAID_1_MODE)
		scm_get_disk_raid1_conf(strg_type, &disk_rinfo);
	else if(iscm.chart[tra].int_data == RAID_5_MODE)
		scm_get_disk_raid5_conf(strg_type, &disk_rinfo);

	i=0;	
//	for(i=0; i<disk_rinfo.raid_cnt; i++) 
	{
		g_message("%s(%d) :  >>[%d] disk_rinfo->mode %d raid_cnt: %d", __FUNCTION__, __LINE__, 
					i, disk_rinfo.mode, disk_rinfo.raid_cnt);
				
		if(!(disk_rinfo.mode & RAID_CONF_MODE)) {
			g_warning("%s(%d) : not RAID_CONF_MODE", __FUNCTION__, __LINE__);
			return -1;
		}

		evtCnt = 0;
		raid_status = RAID_CONFIGURING;
		memset(aEventInfo, 0x00, sizeof(aEventInfo));

		if(strg_type == INTERNAL && rd.count == 2) ctrl_id = 0;
		else if(strg_type == INTERNAL && rd.count == 1) ctrl_id = 0;
//		else if(strg_type == EXTERNAL && rd.count == 1) ctrl_id = 0;
//		else if(strg_type == EXTERNAL && rd.count == 2) ctrl_id = 1;
		else {
			g_warning("%s(%d) : controller id error!!  strg_type[%d] rd.count[%d]", __FUNCTION__, __LINE__, 
						strg_type, rd.count);
			return -1;
		}

		disk_mask = 0;
		for(q=0; q<disk_rinfo.raid_cnt; q++) {
			for(j=0; j<disk_rinfo.rinfo[q].member_count; j++) {
				disk_mask |= (1 << disk_rinfo.rinfo[q].sata_index[j]);
				g_message("%s(%d) : q[%d]j[%d] sata port[%d]", __FUNCTION__, __LINE__,
								 q,j, disk_rinfo.rinfo[q].sata_index[j]);
			}
		}

		raid_level = 0;
		g_message("%s(%d) : disk_rinfo->rinfo[%d].raid_mode:%d   disk_mask:0x%08x", __FUNCTION__, __LINE__, 
					i, disk_rinfo.rinfo[i].raid_mode, disk_mask );
					
		if(disk_rinfo.mode & RAID_CONF_MODE) {
			if(disk_rinfo.rinfo[i].raid_mode == RAID1_CONF_ADD 
					|| disk_rinfo.rinfo[i].raid_mode == RAID_LEVEL_1) {
				raid_level = RAID_LEVEL_1;
				g_message("%s(%d) : set RAID_LEVEL_1", __FUNCTION__, __LINE__);
			}
			else if(disk_rinfo.rinfo[i].raid_mode == RAID5_CONF_ADD
					|| disk_rinfo.rinfo[i].raid_mode == RAID_LEVEL_5) {
				raid_level = RAID_LEVEL_5;
				g_message("%s(%d) : set RAID_LEVEL_5", __FUNCTION__, __LINE__);
			}
		}
		else {
			raid_level = disk_rinfo.rinfo[i].raid_mode;
			g_message("%s(%d) : raid_level[%d]", __FUNCTION__, __LINE__, raid_level);
		}

		if(!nf_raid_create(ctrl_id, disk_mask, raid_level, NULL)) {
			g_warning("%s(%d) : raid create fail", __FUNCTION__, __LINE__);
			
			raid_status = RAID_CONF_FAIL;
    			_scm_send_msg_to_viewer(iscm.chart[tra].ret_msg, -1);
			//scm_send_to_viewer(iscm.chart[tra].ret_msg, -1, 0, 0);
			g_warning("%s(%d) : RAID_CONF_FAIL", __FUNCTION__, __LINE__);
			
			return -1;
		}

		g_message("%s(%d) : disk_mask[0x%08x] raid level[%d] ctrl id[%d] raid_status[%d]", __FUNCTION__, __LINE__,
				disk_mask, raid_level, ctrl_id, raid_status );

		i = 30;	// timeout
		while( --i) {

#if 0			
			int ret;

			memset( aEventInfo, 0x00, sizeof(aEventInfo));			
			evtCnt = 0;

			ret = nf_raid_get_status(ctrl_id, &evtCnt, aEventInfo, NULL);
			g_message("%s(%d) : ret[%d] evtCnt[%d] retry_cnt[%d]", __FUNCTION__, __LINE__, ret, evtCnt, i);

			for(h=0; h<evtCnt; h++) {
				g_message("%s(%d) : idx[%d]  EVENT type[%d]ctrl[%d]raid[%d]disk[%d]", __FUNCTION__, __LINE__, 
					h, aEventInfo[h].EventType, aEventInfo[h].ControllerID, 
					aEventInfo[h].RaidID, aEventInfo[h].DiskID );
			
				if(aEventInfo[h].EventType == NF_RAID_DISKSTATE_RAIDOWNERCHANGE		  // 21
						|| aEventInfo[h].EventType == NF_RAID_DISKSTATE_NONRAIDTORAID) 	// 22
				{	
					raid_status = RAID_CONF_SUCCESS;
					scm_send_to_viewer(iscm.chart[tra].ret_msg, 0, 0, 0);
					g_message("%s(%d) : RAID_CONF_SUCCESS", __FUNCTION__, __LINE__);
					return 0;
				}
				else if(aEventInfo[h].EventType == NF_RAID_DISKSTATE_ERROR) 
				{					
					raid_status = RAID_CONF_FAIL;
					scm_send_to_viewer(iscm.chart[tra].ret_msg, -1, 0, 0);
					g_warning("%s(%d) : RAID_CONF_FAIL", __FUNCTION__, __LINE__);
					return -1;
				}

			}
#endif		
			g_message("%s(%d) : sleep i[%d]", __FUNCTION__, __LINE__, i);	
			g_usleep(1 * 1000000);	
		} 	// while
	}// for		

	raid_status = RAID_CONF_SUCCESS;
  	_scm_send_msg_to_viewer(iscm.chart[tra].ret_msg, 0);
	//scm_send_to_viewer(iscm.chart[tra].ret_msg, 0, 0, 0);
	g_message("%s(%d) : RAID_CONF_SUCCESS", __FUNCTION__, __LINE__);

	return 0;
}

int _scm_work_delete_raid(SCM_T *piscm, TRANSACTION_E tra)
{
	gint ret = -1, i, h;
	IMSG ret_msg;

	EVENT_TYPE aEventInfo[EVENTINFO_BUF_SIZE];
	guchar evtCnt;
	guchar ctrl_id = 0;

	int loop_out = 0;

	g_message("%s(%d) : called!!", __FUNCTION__, __LINE__);
	
	ret = nf_raid_delete(ctrl_id, 0, NULL);
	g_message("%s(%d) : nf_raid_delete ret: %d\n", __FUNCTION__, __LINE__, ret);		
	if(ret == 1 )
	{

		i = 30;	// timeout
		while( --i) {

#if 0
			int ret; 
						
			memset( aEventInfo, 0x00, sizeof(aEventInfo));			
			evtCnt = 0;
				
			ret = nf_raid_get_status(ctrl_id, &evtCnt, aEventInfo, NULL);
			g_message("%s(%d) : ret[%d] evtCnt[%d] retry_cnt[%d]", __FUNCTION__, __LINE__, ret, evtCnt, i);
				
			for(h=0; h<evtCnt; h++) {
				g_message("%s(%d) : idx[%d]  EVENT type[%d]ctrl[%d]raid[%d]disk[%d]", __FUNCTION__, __LINE__, 
						h, aEventInfo[h].EventType, aEventInfo[h].ControllerID, 
						aEventInfo[h].RaidID, aEventInfo[h].DiskID );
	
				if(aEventInfo[h].EventType == NF_RAID_DISKSTATE_RAIDTONONRAID) 	// 23
				{ 
					g_message("%s(%d) : RAID_CONF_SUCCESS", __FUNCTION__, __LINE__);
					loop_out = 1; 
					break; 
				}
				else if(aEventInfo[h].EventType == NF_RAID_DISKSTATE_ERROR) 
				{
					g_warning("%s(%d) : RAID_CONF_FAIL", __FUNCTION__, __LINE__);
					loop_out = 1; 
					break;	
				}
			}
				
			if(loop_out) break;
#endif
			g_message("%s(%d) : sleep i[%d]", __FUNCTION__, __LINE__, i);	
			sleep(1);
		}
			
	}else{
		// need to fail condition
		g_warning("%s(%d) : API RAID_CONF_FAIL", __FUNCTION__, __LINE__);
		g_warning("%s(%d) : API RAID_CONF_FAIL", __FUNCTION__, __LINE__);
		g_warning("%s(%d) : API RAID_CONF_FAIL", __FUNCTION__, __LINE__);
		
	}

    if (tra != TRA_NONE)
    {
    	ret_msg = iscm.chart[tra].ret_msg;
  	_scm_send_msg_to_viewer(ret_msg, 0);
    	//scm_send_to_viewer(ret_msg, 0, 0, 0);
    }
		
	return 0;
}

int _raid_upgrade_fw(SCM_T *piscm, TRANSACTION_E tra)
{	
	gint ret = -1;
	IMSG ret_msg;

	g_message("%s(%d) : called!!", __FUNCTION__, __LINE__);

	ret = nf_raid_upgrade_fw(0, "/NFDVR/raidmgr.bin", NULL);
	printf("%s,  ret: %d\n", __FUNCTION__, ret);
	
	ret_msg = iscm.chart[tra].ret_msg;
  	_scm_send_msg_to_viewer(ret_msg, ret);
	//scm_send_to_viewer(ret_msg, ret, 0, 0);
}

////////////////////////////////////////////////////////////
//
// protected interfaces
//



////////////////////////////////////////////////////////////
//
// public interfaces
//


int scm_get_disk_raidinfo(DISK_RAIDINFO_T *disk_rinfo)
{
#ifdef NOT_SUPPORT_API
	return 0;
#else

	g_message("%s(%d) : called!!", __FUNCTION__, __LINE__);
	if(!disk_rinfo) return -1;

	if(get_disk_raid_ctrl() < 0) 
	{
	    return -1;
    }

	g_message("%s(%d) : called!!", __FUNCTION__, __LINE__);
	if(get_disk_raid_info(disk_rinfo) < 0)  {
		make_disk_raid_info();
		return -1;
	}

	return 0;
#endif	
}

int scm_get_disk_raid1_conf(STRG_TYPE_E strg_type, DISK_RAIDINFO_T *disk_rinfo)
{

	g_message("%s(%d) : called!!", __FUNCTION__, __LINE__);

	if(!disk_rinfo) return -1;
	
	if(get_raid_configuration(1, strg_type, disk_rinfo) < 0)
		return -1;

	//g_message(">>>>>>>>>>>>> %s [%d] >>>>>>>>>>>", __FUNCTION__, __LINE__);
	return 0;
}

int scm_get_disk_raid5_conf(STRG_TYPE_E strg_type, DISK_RAIDINFO_T *disk_rinfo)
{

	g_message("%s(%d) : called!!", __FUNCTION__, __LINE__);

	if(!disk_rinfo) return -1;

	if(get_raid_configuration(5, strg_type, disk_rinfo) < 0)
		return -1;

	//g_message(">>>>>>>>>>>>> %s [%d] >>>>>>>>>>>", __FUNCTION__, __LINE__);
	return 0;
}

int scm_get_disk_raid_advanced_conf(STRG_TYPE_E strg_type, DISK_RAIDINFO_T *disk_rinfo)
{
	g_message("%s(%d) : called!!", __FUNCTION__, __LINE__);

	if(!disk_rinfo) return -1;
	
	if(get_raid_advanced_configuration(strg_type, disk_rinfo) < 0)
		return -1;

	//g_message(">>>>>>>>>>>>> %s [%d] >>>>>>>>>>>", __FUNCTION__, __LINE__);
	return 0;
}


int scm_conf_raid(STRG_TYPE_E strg_type, DISK_RAIDINFO_T *disk_rinfo)
{
	EVENT_TYPE aEventInfo[EVENTINFO_BUF_SIZE];
	guint disk_mask = 0;
	guint raid_level = 0;
	guchar ctrl_id = 0;
	guchar evtCnt;
	gint raid_status;
	gint i, j, k;
	gint h = 0;
	

	g_message("%s(%d) : called!!", __FUNCTION__, __LINE__);

	if(!disk_rinfo) 
		return -1;
	
	for(i=0; i<disk_rinfo->raid_cnt; i++) 
	{
		printf(">>>>>>>>>>>>> %s [%d] >>[%d] disk_rinfo->mode %d raid_cnt: %d>>>>>>>>>\n", __FUNCTION__, __LINE__, i, disk_rinfo->mode, disk_rinfo->raid_cnt);
		if(!(disk_rinfo->mode & RAID_CONF_MODE))
			continue;

		evtCnt = 0;
		raid_status = RAID_CONFIGURING;
		memset(aEventInfo, 0x00, sizeof(aEventInfo));

		if(strg_type == INTERNAL && rd.count == 2) ctrl_id = 0;
		else if(strg_type == INTERNAL && rd.count == 1) ctrl_id = 0;
		else if(strg_type == EXTERNAL && rd.count == 1) ctrl_id = 0;
		else if(strg_type == EXTERNAL && rd.count == 2) ctrl_id = 1;
		else {
			g_warning("controller id error!!");
			return -1;
		}

		disk_mask = 0;
		for(j=0; j<disk_rinfo->rinfo[i].member_count; j++) 
			disk_mask |= (1 << disk_rinfo->rinfo[i].sata_index[j]);

		raid_level = 0;
		printf("::::::::::::::::::::::: disk_rinfo->rinfo[%d].raid_mode: %d\n", i, disk_rinfo->rinfo[i].raid_mode);
		if(disk_rinfo->mode & RAID_CONF_MODE) {
			if(disk_rinfo->rinfo[i].raid_mode == RAID1_CONF_ADD 
					|| disk_rinfo->rinfo[i].raid_mode == RAID_LEVEL_1) {
				raid_level = RAID_LEVEL_1;
			}
			else if(disk_rinfo->rinfo[i].raid_mode == RAID5_CONF_ADD
					|| disk_rinfo->rinfo[i].raid_mode == RAID_LEVEL_5) {
				raid_level = RAID_LEVEL_5;
			}
		}
		else {
			raid_level = disk_rinfo->rinfo[i].raid_mode;
		}

		if(raid_level == 0) {
			if(!nf_raid_delete(ctrl_id, 0, NULL)) {
				g_warning("%s : raid delete fail", __FUNCTION__);
				return -1;
			}
		}
		else {
			//if(!nf_raid_create(ctrl_id, disk_mask, raid_level, NULL)) {
				g_warning("%s : raid create fail", __FUNCTION__);
				return -1;
			//}
		}

		printf("%s\ndisk_mask: %d\nraid level: %d\nctrl id %d\nraid_status: %d\nfor-loop cnt: %d\n",
				__FUNCTION__,
				disk_mask,
				raid_level,
				ctrl_id,
				raid_status, 
				i
			  );

		while(1) {

			nf_raid_get_status(ctrl_id, &evtCnt, aEventInfo, NULL);

			if(evtCnt != 0) {
				for(h=0; h<evtCnt; h++) {
				printf("%s : ctrl_id: %d\nevtCnt: %d\ninfo index: %d\nevt type: %d\nctrl id: %d\nraid id: %d\ndisk id: %d\nraid level: %d\nraid status: %d\n",
						__FUNCTION__,
						ctrl_id,
						evtCnt,
						h,
						aEventInfo[h].EventType,
						aEventInfo[h].ControllerID,
						aEventInfo[h].RaidID,
						aEventInfo[h].DiskID,
						raid_level,
						raid_status
					  );
				}
			}
			
#if 1
			if(evtCnt != 0) 
			{
				printf(">>>>>>>>>>>>> %s [%d] raid level %d evtinfo raidid %d >>>>>>>>>>>\n", __FUNCTION__, __LINE__, raid_level, aEventInfo[0].RaidID);
				if(raid_level == 0) 
				{	
					// raid delete status
					//if(disk_rinfo->rinfo[i].raid_id == aEventInfo[evtCnt-1].RaidID) {
					if(aEventInfo[0].EventType == NF_RAID_DISKSTATE_RAIDTONONRAID) 	// 23
					{ 
						raid_status = RAID_CONF_SUCCESS;
						break;
					}
					else if(aEventInfo[0].EventType == NF_RAID_DISKSTATE_ERROR) 
					{
						raid_status = RAID_CONF_FAIL;
						return -1;
					}
					//	}
				}
				else 
				{					
					printf(">>>>>>>>>>>>> %s [%d] raid level %d evtinfo raidid %d >>>>>>>>>>>\n", __FUNCTION__, __LINE__, raid_level, aEventInfo[0].RaidID);
					// raid create status
					{
						if(aEventInfo[0].EventType == NF_RAID_DISKSTATE_RAIDOWNERCHANGE		  // 21
								|| aEventInfo[0].EventType == NF_RAID_DISKSTATE_NONRAIDTORAID) 	// 22
						{ 
							raid_status = RAID_CONF_SUCCESS;
							break;
						}
						else if(aEventInfo[0].EventType == NF_RAID_DISKSTATE_ERROR) 
						{
							raid_status = RAID_CONF_FAIL;
							return -1;
						}
					}
				}
			}
#endif
			g_usleep(1 * 1000000);	

		} 	// while

		printf(">>>>>>>>>>>>> %s [%d] status %d i %d>>>>>>>>>>>\n", __FUNCTION__, __LINE__, raid_status, i);
	}	// for

	printf(">>>>>>>>>>>>> %s [%d] >>>>>>>>>>>\n", __FUNCTION__, __LINE__);

	return 0;
}

int scm_get_disk_easy_raid_conf(STRG_TYPE_E strg_type, DISK_RAIDINFO_T *disk_rinfo)
{
	g_message("%s(%d) : called!!", __FUNCTION__, __LINE__);

	if(!disk_rinfo) return -1;

	memcpy(disk_rinfo, &rc[strg_type], sizeof(DISK_RAIDINFO_T));

	return 0;
}

int scm_get_disk_advanced_raid_conf(STRG_TYPE_E strg_type, DISK_RAIDINFO_T *disk_rinfo)
{

	g_message("%s(%d) : called!!", __FUNCTION__, __LINE__);

	if(!disk_rinfo) return -1;

	memcpy(disk_rinfo, &rc2[strg_type], sizeof(DISK_RAIDINFO_T));

	return 0;
}

int scm_add_raid_disk(STRG_TYPE_E strg_type, guint raid_mode, guchar disk_mask, DISK_RAIDINFO_T *disk_rinfo)
{
	//guint64 d_size = 0;
	guint d_size = 0;
	gint rcnt;
	gint i, j;

	g_message("%s(%d) : called!!", __FUNCTION__, __LINE__);

	if(!disk_rinfo) return -1;
	
	rc2[strg_type].mode |= RAID_CONF_MODE;
	rc2[strg_type].raid_cnt += 1; 

	rcnt = rc2[strg_type].raid_cnt - 1;
	rc2[strg_type].rinfo[rcnt].raid_id = rcnt;
	if(raid_mode & RAID_1_MODE) rc2[strg_type].rinfo[rcnt].raid_mode = RAID1_CONF_ADD;	// RAID1_CONF_ADD = 11
	else if(raid_mode & RAID_5_MODE) rc2[strg_type].rinfo[rcnt].raid_mode = RAID5_CONF_ADD; // RAID5_CONF_ADD = 51 

	memset(rc2[strg_type].rinfo[rcnt].sata_index, -1, sizeof(gint)*MAX_DISK_IN_RAID);
	for(i=0, j=0; i<5; i++) {
		if(disk_mask & (1 << i)) {
			if(d_size == 0) {
				//d_size = rd.capinfo[strg_type].disk_unit[i].size;
				d_size = rd.controller[strg_type].Sata[i].Capacity;
			}
			//else if(d_size > rd.capinfo[strg_type].disk_unit[i].size) {
			else if(d_size > rd.controller[strg_type].Sata[i].Capacity) {
				//d_size = rd.capinfo[strg_type].disk_unit[i].size;
				d_size = rd.controller[strg_type].Sata[i].Capacity;
			}

			//strcpy(&rc2[strg_type].rinfo[rcnt].model[j], rd.capinfo[strg_type].disk_unit[i].model);
			strcpy(&rc2[strg_type].rinfo[rcnt].model[j], rd.controller[strg_type].Sata[i].ModelName);
			rc2[strg_type].rinfo[rcnt].member_count += 1;
			rc2[strg_type].rinfo[rcnt].sata_index[j++] = i;
			rc2[strg_type].rinfo[rcnt].capacity = (d_size*32)/1024;
			rc2[strg_type].rinfo[rcnt].status = -1;
		}
	}

		for(i=0, j=0; i<5; i++) {
			if(disk_mask & (1 << i)) {
				if(raid_mode & RAID_1_MODE) {
					//d_size = rd.capinfo[strg_type].disk_unit[i].size;
					d_size = (rd.controller[strg_type].Sata[i].Capacity*32)/1024;
					rc2[strg_type].rinfo[rcnt].unused[j++] = d_size - rc2[strg_type].rinfo[rcnt].capacity;
				}
				else {
					rc2[strg_type].rinfo[rcnt].unused[j++] = 0;
				}
			}
		}

	memcpy(disk_rinfo, &rc2[strg_type], sizeof(DISK_RAIDINFO_T));

//	for(i=0; i<2; i++) {
		g_printf("%s\nmode: %d\n", __FUNCTION__, rc2[strg_type].mode);
		for(j=0; j<5; j++) {
			g_printf("[%d]\nraid cnt: %d\nraid id: %d\nraid mode: %d\nmember cnt: %d\nmodel: %s %s %s %s %s\ncapacity: %d\nunused: %d %d %d %d %d\nstatus: %d\nrebuild: %d\nsata_index: %d %d %d %d %d\n\n", 
				j, 
				rc2[strg_type].raid_cnt, 
				rc2[strg_type].rinfo[j].raid_id, 
				rc2[strg_type].rinfo[j].raid_mode, 
				rc2[strg_type].rinfo[j].member_count, 
				rc2[strg_type].rinfo[j].model[0], 
				rc2[strg_type].rinfo[j].model[1], 
				rc2[strg_type].rinfo[j].model[2], 
				rc2[strg_type].rinfo[j].model[3], 
				rc2[strg_type].rinfo[j].model[4], 
				rc2[strg_type].rinfo[j].capacity, 
				rc2[strg_type].rinfo[j].unused[0], 
				rc2[strg_type].rinfo[j].unused[1], 
				rc2[strg_type].rinfo[j].unused[2], 
				rc2[strg_type].rinfo[j].unused[3], 
				rc2[strg_type].rinfo[j].unused[4], 
				rc2[strg_type].rinfo[j].status, 
				rc2[strg_type].rinfo[j].rebuild,
				rc2[strg_type].rinfo[j].sata_index[0],
				rc2[strg_type].rinfo[j].sata_index[1],
				rc2[strg_type].rinfo[j].sata_index[2],
				rc2[strg_type].rinfo[j].sata_index[3],
				rc2[strg_type].rinfo[j].sata_index[4]
				);
		}
//	}
	return 0;
}

int scm_clear_raid_conf()
{
	gint i, j, k;
	gint raid_cnt;

	g_message("%s(%d) : called!!", __FUNCTION__, __LINE__);

	// advanced conf data
	for(i=0; i<SUPPORTED_CONTROLLER_CNT; i++) {
		raid_cnt = rc2[i].raid_cnt;
		if(raid_cnt == 0)
			continue;

		for(j=0; j<raid_cnt; j++) {
			g_message("%s [%d] >>>>> strg_type %d raid id %d mode %d rinfo[%d].raid_mode %d>>>>>>", 
					__FUNCTION__, __LINE__, 
					i, 
					j,
					rc2[i].mode,
					j,
					rc2[i].rinfo[j].raid_mode	
					);

			if(rc2[i].rinfo[j].raid_mode == RAID1_CONF_DELETE) {		// RAID1_CONF_DELETE = 12
				rc2[i].mode = RAID_1_MODE;
				rc2[i].rinfo[j].raid_mode = RAID_LEVEL_1;
			}
			else if (rc2[i].rinfo[j].raid_mode == RAID5_CONF_DELETE) {	// RAID5_CONF_DELETE = 52
				rc2[i].mode = RAID_5_MODE;
				rc2[i].rinfo[j].raid_mode = RAID_LEVEL_5;
			}

			if((rc2[i].rinfo[j].raid_mode != RAID_LEVEL_1) && (rc2[i].rinfo[j].raid_mode != RAID_LEVEL_5)) {

				memset(&rc2[i].rinfo[j], 0x00, sizeof(DISK_RAID_T));

				rc2[i].raid_cnt -= 1;
				rc2[i].mode &= ~RAID_CONF_MODE;
			}
		}
	}

	#if 1
	for(i=0; i<2; i++) {
	g_printf("%s\nmode: %d\n", __FUNCTION__, rc2[i].mode);
	for(j=0; j<5; j++) {
		g_printf("[%d]\nraid cnt: %d\nraid id: %d\nraid mode: %d\nmember cnt: %d\nmodel: %s %s %s %s %s\ncapacity: %d\nunused: %d %d %d %d %d\nstatus: %d\nrebuild: %d\nsata_index: %d %d %d %d %d\n\n", 
				j, 
				rc2[i].raid_cnt, 
				rc2[i].rinfo[j].raid_id, 
				rc2[i].rinfo[j].raid_mode, 
				rc2[i].rinfo[j].member_count, 
				rc2[i].rinfo[j].model[0], 
				rc2[i].rinfo[j].model[1], 
				rc2[i].rinfo[j].model[2], 
				rc2[i].rinfo[j].model[3], 
				rc2[i].rinfo[j].model[4], 
				rc2[i].rinfo[j].capacity, 
				rc2[i].rinfo[j].unused[0], 
				rc2[i].rinfo[j].unused[1], 
				rc2[i].rinfo[j].unused[2], 
				rc2[i].rinfo[j].unused[3], 
				rc2[i].rinfo[j].unused[4], 
				rc2[i].rinfo[j].status, 
				rc2[i].rinfo[j].rebuild,
				rc2[i].rinfo[j].sata_index[0],
				rc2[i].rinfo[j].sata_index[1],
				rc2[i].rinfo[j].sata_index[2],
				rc2[i].rinfo[j].sata_index[3],
				rc2[i].rinfo[j].sata_index[4]
					);
	}
	}
	#endif


	return 0;
}

int scm_get_sata_info(STRG_TYPE_E strg_type, SATA_INFO_T *sata_info)
{
	gint i;
	gchar *offset;

	g_message("%s(%d) : called!!", __FUNCTION__, __LINE__);

	if(!sata_info) return -1;

	for(i=0; i<MAX_DISK_IN_RAID; i++) {
		if(rd.controller[strg_type].Sata[i].ModelName) {
			strcpy(sata_info->model_name[i], rd.controller[strg_type].Sata[i].ModelName);
			offset = strchr(sata_info->model_name[i], ' '); 
			if(offset) sprintf(offset, "%c", '\0');
		}

		if(rd.controller[strg_type].Sata[i].Capacity) 
			sata_info->capacity[i] = (rd.controller[strg_type].Sata[i].Capacity*32)/1024;
	}

	return 0;
}

int scm_create_raid1(STRG_TYPE_E strg_type, IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_CREATE_RAID;

	g_message("%s(%d) : called!!", __FUNCTION__, __LINE__);
	
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());
	iscm.chart[tra].int_data = RAID_1_MODE;
	iscm.chart[tra].char_data = strg_type;

	_scm_work_service_stop(&iscm, tra, RS_DISK);

	return 0;
}

int scm_create_raid5(STRG_TYPE_E strg_type, IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_CREATE_RAID;

	g_message("%s(%d) : called!!", __FUNCTION__, __LINE__);
	
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());
	iscm.chart[tra].int_data = RAID_5_MODE;
	iscm.chart[tra].char_data = strg_type;

	_scm_work_service_stop(&iscm, tra, RS_DISK);

	return 0;
}

int scm_delete_raid(IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_DELETE_RAID;

	g_message("%s(%d) : called!!", __FUNCTION__, __LINE__);

	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());

	_scm_work_service_stop(&iscm, tra, RS_DISK);

	return 0;
}

int scm_jm_update(IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_JM_UPDATE;

	g_message("%s(%d) : called!!", __FUNCTION__, __LINE__);

	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());

	_scm_work_service_stop(&iscm, tra, RS_DISK);

	return 0;
}

int scm_get_jm_fw_ver(FIRMWARE_INFO *tFwInfo)
{	
	g_message("%s(%d) : called!!", __FUNCTION__, __LINE__);

	return nf_raid_get_fw_ver(0, tFwInfo, NULL);
}

int scm_get_disk_raid_mode()
{
    return rc[INTERNAL].mode;
}



