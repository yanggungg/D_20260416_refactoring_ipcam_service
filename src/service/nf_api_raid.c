// #include <gst/gst.h>
#if 0
#include <gnu/targets/std.h>
#include <xdc/std.h>
#include <ti/sdo/ce/Engine.h>
#include <ti/sdo/ce/osal/Memory.h>
#endif
#include "nf_common.h"
#include "nf_api_disk.h"
#include "nf_api_raid.h"
#include "nf_api_archive.h"
#include "nf_api_eventlog.h"
#include "nf_notify.h"

#include "nf_codec_header.h"
#include "libsst.h"
#include "libicmem.h"
#include "unp.h"

#include "nf_watchdog.h"

#include <ctype.h>

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "raid"



/** Static Function Definition **/
static GQuark 
_nf_api_disk_error_quark (void)
{
	return g_quark_from_static_string ( G_LOG_DOMAIN "-error-quark");
}
#define NF_API_RAID_ERROR 	_nf_api_disk_error_quark()

/** Gloval Variable Definition **/

/**
  Function Start!!
**/
gboolean nf_raid_get_ctrl_cnt(u8 *count)
{
	*count = sst_raid_get_ctrl_cnt();
	return 0;
}


//#define SATA_PORT_INVERT

static u8 _ctrl_cnt = 0;

gint nf_raid_get_ctrl_id(NF_RAID_CTRL_ID_E mode)
{
	static int _init = 0;	
	static int _ctrl_in_id = 0, _ctrl_ex_id = 1;

#ifdef SATA_PORT_INVERT	
	if(_init == 0)
	{		
		nf_raid_get_ctrl_cnt(&_ctrl_cnt);
		_init = 1;		
	}
	
	if( _ctrl_cnt >1 ) /* e-sata  jm393 detected!! */
	{
		_ctrl_in_id = 1, 
		_ctrl_ex_id = 0;		
	}else{
		_ctrl_in_id = 0, 
		_ctrl_ex_id = 1;		
	}

#endif 

	if( mode == NF_RAID_CTRL_ID_EXTERNAL )
		return _ctrl_ex_id;
	else {
		g_warning("%s _ctrl_in_id[%d]",__FUNCTION__,_ctrl_in_id);
		return _ctrl_in_id;
	}
	
}
gboolean nf_raid_create(u8 cid, u32 did_mask, u32 raid_level, GError **error)
{
	gint result = 0;
	
	g_message("%s called!! cid[%d] did_mask[0x%08x] raid_level[%d]",__FUNCTION__,  cid, did_mask, raid_level);
		
	result = sst_raid_create(cid, did_mask, raid_level);
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 		

		g_set_error( error, NF_API_RAID_ERROR, result, sst_get_error_string(result) );
	}

	g_message("%s ret[%d]",__FUNCTION__, result);

	return ( result == 0 ) ? 1:0;
}

gboolean nf_raid_delete(u8 cid, u8 rid, GError **error)
{
	gint result = 0;

	g_message("%s called!! cid[%d] rid[%d]",__FUNCTION__, cid, rid);

	result = sst_raid_delete(cid, rid);
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 		

		g_set_error( error, NF_API_RAID_ERROR, result, sst_get_error_string(result) );
	}

	g_message("%s ret[%d]",__FUNCTION__, result);
	
	return ( result == 0 ) ? 1:0;
}

gboolean nf_raid_get_ctrl_info(u8 cid, CONTROLLER *cont, GError **error)
{
	gint result = 0;

	result = sst_raid_get_ctrl_info(cid, cont);
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 		

		g_set_error( error, NF_API_RAID_ERROR, result, sst_get_error_string(result) );
	}

	return ( result == 0 ) ? 1:0;
}

gboolean nf_raid_get_raid_info(u8 cid, RAID_PORT *aRAID, GError **error)
{
	gint result = 0;

	result = sst_raid_get_raid_info(cid, aRAID);
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 		

		g_set_error( error, NF_API_RAID_ERROR, result, sst_get_error_string(result) );
	}

	return ( result == 0 ) ? 1:0;
}

gboolean nf_raid_get_disk_info(u8 cid, SATA_PORT *aSATA, GError **error)
{
	gint result = 0;

	result = sst_raid_get_disk_info(cid, aSATA);
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 		

		g_set_error( error, NF_API_RAID_ERROR, result, sst_get_error_string(result) );
	}

	return ( result == 0 ) ? 1:0;
}

gboolean nf_raid_get_fw_ver(u8 cid, FIRMWARE_INFO *tFwInfo, GError **error)
{
	gint result = 0;

	result = sst_raid_get_fw_ver(cid, tFwInfo);
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 		

		g_set_error( error, NF_API_RAID_ERROR, result, sst_get_error_string(result) );
	}
	
	//nf_debug_hexdump(tFwInfo, sizeof(FIRMWARE_INFO));
	
	return ( result == 0 ) ? 1:0;
}

gboolean nf_raid_upgrade_fw(u8 cid, u8* filename, GError **error)
{
	gint result = 0;

	result = sst_raid_upgrade_fw(cid, filename);
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 		

		g_set_error( error, NF_API_RAID_ERROR, result, sst_get_error_string(result) );
	}

	return ( result == 0 ) ? 1:0;
}

//per ==== -1 means failed during rebuilding.
gboolean nf_raid_get_rebuild_per(u8 cid, u8 rid, u8* per, GError **error)
{
	gint result = 0;

	result = sst_raid_get_rebuild_per(cid, rid, per);
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 		

		g_set_error( error, NF_API_RAID_ERROR, result, sst_get_error_string(result) );
	}

	return ( result == 0 ) ? 1:0;
}

gboolean nf_raid_set_rebuild_priority(u8 cid, u8 rid, u8 pri, GError **error)
{
	gint result = 0;

	result = sst_raid_set_rebuild_priority(cid, rid, pri);
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 		

		g_set_error( error, NF_API_RAID_ERROR, result, sst_get_error_string(result) );
	}

	return ( result == 0 ) ? 1:0;
}

gboolean nf_raid_add_spare_disk(u8 cid, u8 did, u8 rid, GError **error)
{
	gint result = 0;

	result = sst_raid_add_spare_disk(cid, did, rid);
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 		

		g_set_error( error, NF_API_RAID_ERROR, result, sst_get_error_string(result) );
	}

	return ( result == 0 ) ? 1:0;
}
gboolean nf_raid_del_spare_disk(u8 cid, u8 did, u8 rid, GError **error)
{
	gint result = 0;

	result = sst_raid_del_spare_disk(cid, did, rid);
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 		

		g_set_error( error, NF_API_RAID_ERROR, result, sst_get_error_string(result) );
	}

	return ( result == 0 ) ? 1:0;
}
gboolean nf_raid_pause_rebuilding(u8 cid, u8 rid, GError **error)
{
	gint result = 0;

	result = sst_raid_pause_rebuilding(cid, rid);
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 		

		g_set_error( error, NF_API_RAID_ERROR, result, sst_get_error_string(result) );
	}

	return ( result == 0 ) ? 1:0;
}
gboolean nf_raid_resume_rebuilding(u8 cid, u8 rid, GError **error)
{
	gint result = 0;

	result = sst_raid_resume_rebuilding(cid, rid);if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 		

		g_set_error( error, NF_API_RAID_ERROR, result, sst_get_error_string(result) );
	}

	return ( result == 0 ) ? 1:0;
}
gboolean nf_raid_get_smart(u8 cid, u8 did, GError **error)
{
	gint result = 0;

	result = sst_raid_get_smart(cid, did);
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 		

		g_set_error( error, NF_API_RAID_ERROR, result, sst_get_error_string(result) );
	}

	return ( result == 0 ) ? 1:0;
}
gboolean nf_raid_get_status(u8 cid, u8 *nEventCount, EVENT_TYPE *aEventInfo, GError **error)
{
	gint result = 0;
			
	result = sst_raid_get_status(cid, nEventCount, aEventInfo);
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 		

		g_set_error( error, NF_API_RAID_ERROR, result, sst_get_error_string(result) );
	}

	return ( result == 0 ) ? 1:0;
}

gboolean nf_raid_set_auto_rebuild_flag(u8 cid, u8 option, GError **error)
{
	gint result = 0;
			
	result = sst_raid_set_auto_rebuild_flag(cid, option);
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 		
		g_set_error( error, NF_API_RAID_ERROR, result, sst_get_error_string(result) );
	}

	return ( result == 0 ) ? 1:0;
}


/*

18) notify disk_raid

p0 �� ���°�
	0: init or ok
	1: degrade
	2: rebuilding
	3: broken
			
p1 
	2: rebuilding �϶� p1 ������ ����Ʈ 
*/


static void
_raid_mon_thread_func (void *thread_param)
{
	RAID_PORT aRAID;
	gboolean  ret = 0;
	gint is_first_run = 1;	
		
	g_message("%s start", __FUNCTION__);	
	
	g_message("%s(%d) : nf_raid_set_auto_rebuild_flag enable!! start", __FUNCTION__, __LINE__);
	nf_raid_set_auto_rebuild_flag (nf_raid_get_ctrl_id(NF_RAID_CTRL_ID_INTERNAL), 1 /* enable */, NULL /* GError */);
	g_message("%s(%d) : nf_raid_set_auto_rebuild_flag enable!! end", __FUNCTION__, __LINE__);
	
	while(1)
	{	
		g_message("%s run", __FUNCTION__);

#ifdef SATA_PORT_INVERT	
		nf_raid_get_ctrl_cnt(&_ctrl_cnt);
#endif
		
		if( nf_filesystem_is_online() || is_first_run )
		{
			is_first_run = 0;			
			memset( &aRAID, 0x00, sizeof(aRAID));			
			
			ret = nf_raid_get_raid_info(nf_raid_get_ctrl_id(NF_RAID_CTRL_ID_INTERNAL), &aRAID, NULL);
			g_message("%s(%d) : nf_raid_get_raid_info ret[%d] RaidMemberCount[%d] status[%d]", 
					__FUNCTION__, __LINE__, ret, aRAID.RaidMemberCount,  aRAID.RaidStatus);
				
			if( aRAID.RaidMemberCount <= 1 ) {
				g_message("%s(%d) : raid conf not found!", __FUNCTION__, __LINE__);
				g_message("%s(%d) : thread sleep", __FUNCTION__, __LINE__);
				// break;
				while(1) sleep(86400);
			}
			
			if( aRAID.RaidMemberCount > 1 && aRAID.RaidStatus != RAID_STATE_NORMAL /* 3 */ ){
								
				// set smart notify
				if ( nf_notify_get_param0("disk_smart") != 2 /* mirror fail */)
					nf_notify_fire_params("disk_smart", 2, 0, 0, 0);

				// set disk notify
				if( aRAID.RaidStatus == RAID_STATE_BROKEN) {

					if ( nf_notify_get_param0("disk_raid") != 3 )
						nf_notify_fire_params("disk_raid", 3, 0, 0, 0);
				
				} else if( aRAID.RaidStatus == RAID_STATE_DEGRADE) {

					if ( nf_notify_get_param0("disk_raid") != 1 )
						nf_notify_fire_params("disk_raid", 1, 0, 0, 0);

				} else if( aRAID.RaidStatus == RAID_STATE_REBUILDING) {
					
					u8 rebuild_per = 0;					
					nf_raid_get_rebuild_per( nf_raid_get_ctrl_id(NF_RAID_CTRL_ID_INTERNAL), 0, &rebuild_per, NULL);									

					if ( nf_notify_get_param0("disk_raid") != 2
						|| nf_notify_get_param1("disk_raid") != rebuild_per )
						nf_notify_fire_params("disk_raid", 2, rebuild_per, 0, 0);
												
				}
					
			}else if( aRAID.RaidMemberCount > 1 && aRAID.RaidStatus == RAID_STATE_NORMAL  ){
																
				// clear smart notify
				if ( nf_notify_get_param0("disk_smart") == 2 )
					nf_notify_fire_params("disk_smart", 0, 0, 0, 0);

				// clear disk_raid notify
				if ( nf_notify_get_param0("disk_raid") != 0 )
					nf_notify_fire_params("disk_raid", 0, 0, 0, 0);
															
			}			
		}		
		sleep(5*60);
	}	
	g_message("%s end", __FUNCTION__);	
}
    
gboolean nf_raid_init(){
	
	FIRMWARE_INFO tFwInfo;
	gboolean ret;
	GThread *mon_thread = NULL;
	
	g_message("%s(%d) : called!!", __FUNCTION__, __LINE__);
	
	memset( &tFwInfo, 0x00, sizeof(tFwInfo));			
	ret = nf_raid_get_fw_ver(0 , &tFwInfo, NULL);	

	g_message("%s(%d) : raid_fw [0x%04x][0x%04x]  ProductName[%32.32s]", __FUNCTION__, __LINE__, 
			tFwInfo.FirmwareVersion, tFwInfo.FirmwareDateCode, tFwInfo.ProductName);

	nf_debug_hexdump( &tFwInfo, sizeof(tFwInfo));
		
	if(ret && !strncmp( tFwInfo.ProductName, "JMICRON H/W RAID", 16) ){						
		if( tFwInfo.FirmwareVersion == 0x0961
			&& tFwInfo.FirmwareDateCode == 0x0506 ){				
			g_message("%s(%d) : raid fw ok!", __FUNCTION__, __LINE__);
		}else{				
			ret = nf_raid_upgrade_fw(0, "/NFDVR/raidmgr.bin", NULL);
			g_message("%s(%d) : nf_raid_upgrade_fw ret[%d]", __FUNCTION__, __LINE__, ret);
		}
	}
	else{
		g_warning("%s(%d) : raid chip not found!!", __FUNCTION__, __LINE__);
		return 0;
	}	
		
 	mon_thread = g_thread_create(	(GThreadFunc)_raid_mon_thread_func, NULL, 0 /* joinable  */, NULL);	
 	if( !mon_thread )
 		g_warning("%s(%d) : raid_mon_thread_func create fail!!", __FUNCTION__, __LINE__);
 	 		 		 	
	return 1;		
}



#if 1

#include "jbshell.h"

static char gc_jbshell_cmd_help[] = "gc";
static int gc_jbshell_cmd(int argc, char **argv)
{
	u8 cnt;
	if( argc != 1 )
	{
		printf("%s\n",gc_jbshell_cmd_help);
		return -1;
	}

	nf_raid_get_ctrl_cnt(&cnt);

	g_warning("%s cnt[%d]\n", __FUNCTION__, cnt);

	return 0;
}
__commandlist(gc_jbshell_cmd, "gc", gc_jbshell_cmd_help, gc_jbshell_cmd_help);

static char cr_jbshell_cmd_help[] = "cr C[n] D[0,..,4] R0|R1|R10|JBOD|R3|R5";
static int cr_jbshell_cmd(int argc, char **argv)
{
	u8 s[3];
	u8 cid = 0;
	u8 disk_count = 0;
	u8 *str;
	u8 disk_str[15];
	u8 disk[5];
	u8 raid_level = 0;
	u8 raid_type[12];
	u32 did_mask = 0;

	if( argc != 4 )
	{
		printf("%s\n",cr_jbshell_cmd_help);
		return -1;
	}

	/*
	 * Get controller id from argv[1].
	 */
	strncpy((char *)s, (char *)argv[1], 2);
	s[2] = '\0';

	if (toupper(s[0]) != 'C')
	{
		printf("\n(1)Unknown Parameters\n");
		return 0;
	}

	cid = (u8)(s[1] - 0x30);

	/*
	 * Add selecting disks D[0,2,3] -> add disk0, 2, 3.
	 */
	strcpy((char *)disk_str, (char *)argv[2]);
	if (toupper(disk_str[0]) != 'D')
	{
		printf("\n(2)Unknown Parameters\n");
		printf("Usage: CR C0 D[0,1,3] R5 --> create RAID-5 using disk0, 1, 3 on controller 0\n");
		return 0;
	}

	str = disk_str;
	while ((*str != ']') && ((*str != 0)))
	{
		if (((*str - 0x30) >= 0) && ((*str - 0x30) < MAX_DISK_IN_CONTROLLER))
		{
			disk[disk_count] = (u8)(*str - 0x30);
			did_mask |= (1 << disk[disk_count]);
			disk_count++;
		}
		str++;
	}

	/*
	 * Get RAID type from argv[3].
	 */
	strcpy((char *)raid_type, (char *)argv[3]);
	if		(!(strncmp((char *)raid_type, "R0",   2)) || !(strncmp((char *)raid_type, "r0",   2))) raid_level = RAID_LEVEL_0;
	else if	(!(strncmp((char *)raid_type, "R1",   2)) || !(strncmp((char *)raid_type, "r1",   2))) raid_level = RAID_LEVEL_1;
	else if (!(strncmp((char *)raid_type, "JBOD", 4)) || !(strncmp((char *)raid_type, "jbod", 4))) raid_level = RAID_LEVEL_JBOD;
	else if (!(strncmp((char *)raid_type, "R10",  3)) || !(strncmp((char *)raid_type, "r10",  3))) raid_level = RAID_LEVEL_1;
	else if (!(strncmp((char *)raid_type, "R3",   2)) || !(strncmp((char *)raid_type, "r3",   3))) raid_level = RAID_LEVEL_3;
	else if (!(strncmp((char *)raid_type, "R5",   2)) || !(strncmp((char *)raid_type, "r5",   3))) raid_level = RAID_LEVEL_5;
	else if (!(strncmp((char *)raid_type, "CLONE",5)) || !(strncmp((char *)raid_type, "clone",5))) raid_level = RAID_LEVEL_CLONE; 
	else	{ printf("\nIncorrect RAID type\n"); return 0; }

	nf_raid_create(cid, did_mask, raid_level, NULL);
	
	return 0;
}
__commandlist(cr_jbshell_cmd, "cr", cr_jbshell_cmd_help, cr_jbshell_cmd_help);

static char dr_jbshell_cmd_help[] = "dr C[n] R[n]";
static int dr_jbshell_cmd(int argc, char **argv)
{
	u8 s[3];
	u8 cid = 0;
	u8 s1[3];
	u8 rid = 0;
	
	if( argc != 3 )
	{
		printf("%s\n",dr_jbshell_cmd_help);
		return -1;
	}

	/*
	 * Get controller id from argv[1].
	 */
	strncpy((char *)s, (char *)argv[1], 2);
	s[2] = '\0';

	if (toupper(s[0]) != 'C')
	{
		printf("\n(1)Unknown Parameters\n");
		return 0;
	}

	cid = (u8)(s[1] - 0x30);

	/*
	 * Get RAID port id from argv[2].
	 */
	strncpy((char *)s1, (char *)argv[2], 2);
	s1[2] = '\0';

	if (toupper(s1[0]) != 'R')
	{
		printf("\nUnknown Parameters\n");
		return 0;
	}
	rid = (u8)(s1[1] - 0x30);
	
	nf_raid_delete(cid, rid, NULL);

	return 0;
}
__commandlist(dr_jbshell_cmd, "dr", dr_jbshell_cmd_help, dr_jbshell_cmd_help);

static char dc_jbshell_cmd_help[] = "dc  C[n]";
static int dc_jbshell_cmd(int argc, char **argv)
{
	u8 cid = 0;
	u8 s[3];
	u8 s1[3];
	CONTROLLER cont;
	
	if( argc != 2 )
	{
		printf("%s\n",dc_jbshell_cmd_help);
		return -1;
	}

	/*
	 * Get controller id from argv[1].
	 */
	strncpy((char *)s, (char *)argv[1], 2);
	s[2] = '\0';

	if (toupper(s[0]) != 'C')
	{
		printf("\n(1)Unknown Parameters\n");
		return 0;
	}

	cid = (u8)(s[1] - 0x30);
	
	nf_raid_get_ctrl_info(cid, &cont, NULL);

	return 0;
}
__commandlist(dc_jbshell_cmd, "dc", dc_jbshell_cmd_help, dc_jbshell_cmd_help);

static char sr_jbshell_cmd_help[] = "sr C[n]";
static int sr_jbshell_cmd(int argc, char **argv)
{
	u8 cid = 0;
	u8 s[3];
	u8 s1[3];
	RAID_PORT aRAID[MAX_RAID_IN_CONTROLLER];
	
	if( argc != 2 )
	{
		printf("%s\n",sr_jbshell_cmd_help);
		return -1;
	}

	/*
	 * Get controller id from argv[1].
	 */
	strncpy((char *)s, (char *)argv[1], 2);
	s[2] = '\0';

	if (toupper(s[0]) != 'C')
	{
		printf("\n(1)Unknown Parameters\n");
		return 0;
	}

	cid = (u8)(s[1] - 0x30);
	
	nf_raid_get_raid_info(cid, &aRAID, NULL);

	return 0;
}
__commandlist(sr_jbshell_cmd, "sr", sr_jbshell_cmd_help, sr_jbshell_cmd_help);

static char ss_jbshell_cmd_help[] = "ss C[n]";
static int ss_jbshell_cmd(int argc, char **argv)
{
	u8 cid = 0;
	u8 s[3];
	u8 s1[3];
	SATA_PORT aSATA[MAX_DISK_IN_CONTROLLER];
	
	if( argc != 2 )
	{
		printf("%s\n",ss_jbshell_cmd_help);
		return -1;
	}
	
	/*
	 * Get controller id from argv[1].
	 */
	strncpy((char *)s, (char *)argv[1], 2);
	s[2] = '\0';

	if (toupper(s[0]) != 'C')
	{
		printf("\n(1)Unknown Parameters\n");
		return 0;
	}
	
	nf_raid_get_disk_info(cid, &aSATA, NULL);

	return 0;
}
__commandlist(ss_jbshell_cmd, "ss", ss_jbshell_cmd_help, ss_jbshell_cmd_help);

static char sf_jbshell_cmd_help[] = "sf C[n]";
static int sf_jbshell_cmd(int argc, char **argv)
{
	u8 cid = 0;
	u8 s[3];
	u8 s1[3];
	FIRMWARE_INFO tFwInfo;
	if( argc != 2 )
	{
		printf("%s\n",sf_jbshell_cmd_help);
		return -1;
	}

	/*
	 * Get controller id from argv[1].
	 */
	strncpy((char *)s, (char *)argv[1], 2);
	s[2] = '\0';

	if (toupper(s[0]) != 'C')
	{
		printf("\n(1)Unknown Parameters\n");
		return 0;
	}
	
	nf_raid_get_fw_ver(cid, &tFwInfo, NULL);

	return 0;
}
__commandlist(sf_jbshell_cmd, "sf", sf_jbshell_cmd_help, sf_jbshell_cmd_help);

static char uf_jbshell_cmd_help[] = "uf C[n] filename";
static int uf_jbshell_cmd(int argc, char **argv)
{
	u8 cid = 0;
	u8 s[3];
	u8 s1[3];
	
	if( argc != 3 )
	{
		printf("%s\n",uf_jbshell_cmd_help);
		return -1;
	}
	
	/*
	 * Get controller id from argv[1].
	 */
	strncpy((char *)s, (char *)argv[1], 2);
	s[2] = '\0';

	if (toupper(s[0]) != 'C')
	{
		printf("\n(1)Unknown Parameters\n");
		return 0;
	}
	
	nf_raid_upgrade_fw(cid, argv[2], NULL);

	return 0;
}
__commandlist(uf_jbshell_cmd, "uf", uf_jbshell_cmd_help, uf_jbshell_cmd_help);

static char gr_jbshell_cmd_help[] = "gr C[n] R[n]";
static int gr_jbshell_cmd(int argc, char **argv)
{
	u8 cid = 0, rid =0;
	u8 s[3];
	u8 s1[3];
	u8 per=0;
	if( argc != 3 )
	{
		printf("%s\n",gr_jbshell_cmd_help);
		return -1;
	}
	
	/*
	 * Get controller id from argv[1].
	 */
	strncpy((char *)s, (char *)argv[1], 2);
	s[2] = '\0';

	if (toupper(s[0]) != 'C')
	{
		printf("\n(1)Unknown Parameters\n");
		return 0;
	}
	
	/*
	 * Get RAID port id from argv[2].
	 */
	strncpy((char *)s1, (char *)argv[2], 2);
	s1[2] = '\0';

	if (toupper(s1[0]) != 'R')
	{
		printf("\nUnknown Parameters\n");
		return 0;
	}
	rid = (u8)(s1[1] - 0x30);
	
	nf_raid_get_rebuild_per(cid, rid, &per, NULL);
	printf("[RAID] rebuilding per = %u, %u\n", rid, per);
	return 0;
}
__commandlist(gr_jbshell_cmd, "gr",gr_jbshell_cmd_help, gr_jbshell_cmd_help);

static char rp_jbshell_cmd_help[] = "rp C[n] R[n] priority";
static int rp_jbshell_cmd(int argc, char **argv)
{
	u8 cid = 0, rid = 0;
	u8 s[3];
	u8 s1[3];
	u8 per=0;
	
	if( argc != 4 )
	{
		printf("%s\n",rp_jbshell_cmd_help);
		return -1;
	}
	
	/*
	 * Get controller id from argv[1].
	 */
	strncpy((char *)s, (char *)argv[1], 2);
	s[2] = '\0';

	if (toupper(s[0]) != 'C')
	{
		printf("\n(1)Unknown Parameters\n");
		return 0;
	}

	/*
	 * Get RAID port id from argv[2].
	 */
	strncpy((char *)s1, (char *)argv[2], 2);
	s1[2] = '\0';

	if (toupper(s1[0]) != 'R')
	{
		printf("\nUnknown Parameters\n");
		return 0;
	}
	rid = (u8)(s1[1] - 0x30);
	
	nf_raid_set_rebuild_priority(cid, rid, (u8)strtoul(argv[3], NULL, 0), NULL);

	return 0;
}
__commandlist(rp_jbshell_cmd, "rp", rp_jbshell_cmd_help, rp_jbshell_cmd_help);

static char as_jbshell_cmd_help[] = "as C[n] D[n] R[n]";
static int as_jbshell_cmd(int argc, char **argv)
{
	u8 cid = 0;
	u8 s[3];
	u8 s1[3];
	u8 per=0;
	u8 did = 0;
	u8 rid = 0;
	
	if( argc != 4 )
	{
		printf("%s\n",as_jbshell_cmd_help);
		return -1;
	}
	/*
	 * Get controller id from argv[1].
	 */
	strncpy((char *)s1, (char *)argv[1], 2);
	s1[2] = '\0';

	if (toupper(s1[0]) != 'C')
	{
		printf("\nUnknown Parameters\n");
		return 0;
	}
	cid = (u8)(s1[1] - 0x30);
	
	/*
	 * Get SATA port number from argv[2].
	 */
	strncpy((char *)s, (char *)argv[2], 2);
	s[2] = '\0';

	if (toupper(s[0]) != 'D')
	{
		printf("\nUnknown Parameters\n");
		return 0;
	}
	did = (u8)(s[1] - 0x30);
	
	/*
	 * Get RAID port number from argv[3]
	 */
	strncpy((char *)s, (char *)argv[3], 2);
	s[2] = '\0';
	if (toupper(s[0]) != 'R')
	{
		printf("\nUnknown Parameters\n");
		return 0;
	}
	rid = (u8)(s[1] - 0x30);

	nf_raid_add_spare_disk(cid, did, rid, NULL);

	return 0;
}
__commandlist(as_jbshell_cmd, "as", as_jbshell_cmd_help, as_jbshell_cmd_help);

static char ds_jbshell_cmd_help[] = "ds C[n] D[n] R[n]";
static int ds_jbshell_cmd(int argc, char **argv)
{
	u8 cid = 0;
	u8 s[3];
	u8 s1[3];
	u8 per=0;
	u8 did = 0;
	u8 rid = 0;

	if( argc != 4 )
	{
		printf("%s\n",ds_jbshell_cmd_help);
		return -1;
	}
	
	/*
	 * Get controller id from argv[1].
	 */
	strncpy((char *)s1, (char *)argv[1], 2);
	s1[2] = '\0';

	if (toupper(s1[0]) != 'C')
	{
		printf("\nUnknown Parameters\n");
		return 0;
	}
	cid = (u8)(s1[1] - 0x30);
	
	/*
	 * Get SATA port number from argv[2].
	 */
	strncpy((char *)s, (char *)argv[2], 2);
	s[2] = '\0';

	if (toupper(s[0]) != 'D')
	{
		printf("\nUnknown Parameters\n");
		return 0;
	}
	did = (u8)(s[1] - 0x30);
	
	/*
	 * Get RAID port number from argv[3]
	 */
	strncpy((char *)s, (char *)argv[3], 2);
	s[2] = '\0';
	if (toupper(s[0]) != 'R')
	{
		printf("\nUnknown Parameters\n");
		return 0;
	}
	rid = (u8)(s[1] - 0x30);

	nf_raid_del_spare_disk(cid, did, rid, NULL);

	return 0;
}
__commandlist(ds_jbshell_cmd, "ds", ds_jbshell_cmd_help, ds_jbshell_cmd_help);

static char rap_jbshell_cmd_help[] = "rap C[n] R[n]";
static int rap_jbshell_cmd(int argc, char **argv)
{
	u8 cid = 0, rid = 0;
	u8 s[3];
	u8 s1[3];
	u8 per=0;
	
	if( argc != 3 )
	{
		printf("%s\n",rap_jbshell_cmd_help);
		return -1;
	}
	
	/*
	 * Get controller id from argv[1].
	 */
	strncpy((char *)s1, (char *)argv[1], 2);
	s1[2] = '\0';

	if (toupper(s1[0]) != 'C')
	{
		printf("\nUnknown Parameters\n");
		return 0;
	}
	cid = (u8)(s1[1] - 0x30);
	
	/*
	 * Get RAID port id from argv[2].
	 */
	strncpy((char *)s1, (char *)argv[2], 2);
	s1[2] = '\0';

	if (toupper(s1[0]) != 'R')
	{
		printf("\nUnknown Parameters\n");
		return 0;
	}
	rid = (u8)(s1[1] - 0x30);
	
	nf_raid_pause_rebuilding(cid, rid, NULL);

	return 0;
}
__commandlist(rap_jbshell_cmd, "rap", rap_jbshell_cmd_help, rap_jbshell_cmd_help);

static char rar_jbshell_cmd_help[] = "rar C[n] R[n]";
static int rar_jbshell_cmd(int argc, char **argv)
{
	u8 cid = 0, rid = 0;
	u8 s[3];
	u8 s1[3];
	u8 per=0;
	
	if( argc != 3 )
	{
		printf("%s\n",rar_jbshell_cmd_help);
		return -1;
	}
	/*
	 * Get controller id from argv[1].
	 */
	strncpy((char *)s1, (char *)argv[1], 2);
	s1[2] = '\0';

	if (toupper(s1[0]) != 'C')
	{
		printf("\nUnknown Parameters\n");
		return 0;
	}
	cid = (u8)(s1[1] - 0x30);
	
	/*
	 * Get RAID port id from argv[2].
	 */
	strncpy((char *)s1, (char *)argv[2], 2);
	s1[2] = '\0';

	if (toupper(s1[0]) != 'R')
	{
		printf("\nUnknown Parameters\n");
		return 0;
	}
	rid = (u8)(s1[1] - 0x30);
	
	nf_raid_resume_rebuilding(cid, rid, NULL);

	return 0;
}
__commandlist(rar_jbshell_cmd, "rar", rar_jbshell_cmd_help, rar_jbshell_cmd_help);

static char raid_get_status_jbshell_cmd_help[] = "raid_get_status C[n]";
static int raid_get_status_jbshell_cmd(int argc, char **argv)
{
	u8 cid = 0, rid = 0;
	u8 s[3];
	u8 s1[3];
	u8 per=0;
	u8 nEventCount;
	u32 i, j;
	EVENT_TYPE aEventInfo[20];
		
	if( argc != 2 )
	{
		printf("%s\n",raid_get_status_jbshell_cmd_help);
		return -1;
	}
	
	/*
	 * Get controller id from argv[1].
	 */
	strncpy((char *)s1, (char *)argv[1], 2);
	s1[2] = '\0';

	if (toupper(s1[0]) != 'C')
	{
		printf("\nUnknown Parameters\n");
		return 0;
	}
	cid = (u8)(s1[1] - 0x30);
	
	nf_raid_get_status(cid, &nEventCount, &aEventInfo, NULL);
	
	printf("\Event Count = %d\n", nEventCount);
	
	for ( i = 0 ; i < nEventCount	; i++ )
	{
		printf("\nEvent -> CtrlID = %d\n", aEventInfo[j].ControllerID);
		printf("\nEvent -> RaidID = %d\n", aEventInfo[j].RaidID);
		printf("\nEvent -> DiskID = %d\n", aEventInfo[j].DiskID);
	}
	
	return 0;
}
__commandlist(raid_get_status_jbshell_cmd, "raid_get_status", raid_get_status_jbshell_cmd_help, raid_get_status_jbshell_cmd_help);


/*
Valid commands set are:
=======================

 gc ------------------------------------- Get avail JMB39X
 dc C[n] -------------------------------- Display controller info
 sr C[n] -------------------------------- Show avail RAID info
 ss C[n] -------------------------------- Show avail SATA info
 cr C[n] D[0,..,4] R0|R1|R10|JBOD|R3|R5 - Create RAID						//need to check raid_get_status for finishing
 dr C[n] R[n] --------------------------- Delete RAID					//need to check raid_get_status for finishing
 sf C[n] -------------------------------- Show firmware version
 uf C[n] filename ----------------------- Upgrade firmware
 gr C[n] R[n] --------------------------- Get rebuilding percentage
 rp C[n] R[n] priority ------------------ Set rebuild priority
 as C[n] D[n] R[n] ---------------------- Add spare disk
 ds C[n] D[n] R[n] ---------------------- Delete spare disk
 rap C[n] R[n] 0/1 ----------------------- Pause/Resume rebuilding
 rar C[n] R[n] 0/1 ----------------------- Pause/Resume rebuilding
 raid_get_status --------------------------for Monitoring
 */
 
#endif



