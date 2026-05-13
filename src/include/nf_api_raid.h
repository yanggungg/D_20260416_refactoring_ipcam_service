#ifndef __NF_API_RAID_H__
#define __NF_API_RAID_H__

#include "libsst.h"

enum NF_RAID_EVENTTYPE
{
	NF_RAID_CONTROLLER_CHANGED_PLUGIN = 1,
	NF_RAID_CONTROLLER_CHANGED_UNPLUGGED,
	NF_RAID_RAIDSTATE_NORMAL_PLUGIN,
	NF_RAID_RAIDSTATE_NORMAL_FROM_REBUILDING, 
	NF_RAID_RAIDSTATE_BROKEN_PLUGIN,
	NF_RAID_RAIDSTATE_BROKEN_UNPLUGGED,
	NF_RAID_RAIDSTATE_BROKEN_FROM_REBUILDING,  
	NF_RAID_RAIDSTATE_DEGRADE_PLUGIN,
	NF_RAID_RAID_RAIDSTATE_DEGRADE_UNPLUGGED,
	NF_RAID_RAIDSTATE_DEGRADE_FROM_REBUILDING, 
	NF_RAID_RAID_RAIDSTATE_REBUILDING_PLUGIN,
	NF_RAID_RAIDSTATE_REBUILDING_UNPLUGGED,
	NF_RAID_RAIDSTATE_REBUILDING_INPROGRESS, 
	NF_RAID_RAIDSTATE_BACKUP_PLUGIN,
	NF_RAID_RAIDSTATE_CHANGED_REBUILD_PRIORITY,
	NF_RAID_RAIDSTATE_CHANGED_STANDBY_TIMER,
	NF_RAID_RAIDSTATE_CHANGED_PASSWORD,
	NF_RAID_DISKSTATE_PLUGIN,
	NF_RAID_DISKSTATE_UNPLUGGED,
	NF_RAID_DISKSTATE_ERROR,
	NF_RAID_DISKSTATE_RAIDOWNERCHANGE, 
	NF_RAID_DISKSTATE_NONRAIDTORAID,
	NF_RAID_DISKSTATE_RAIDTONONRAID,
	NF_RAID_DISKSTATE_NONRAIDTOSPARE,
	NF_RAID_DISKSTATE_SPARETONONRAID,
	NF_RAID_STATE_CONTROLLERCHANGED,
	NF_RAID_STATE_RAIDCONFIGURE,
	NF_RAID_MAX_EVENTTYPE
};

/************************************************************************
 * RAID level.
 ************************************************************************/
 
/* RAID in level 0 (stripe) */
#define RAID_LEVEL_0						0

/* RAID in level 1 (mirror) or level 10 (4 disks) */
#define RAID_LEVEL_1						1

/* RAID in JBOD */
#define RAID_LEVEL_JBOD					2

/* RAID in level 3 */
#define RAID_LEVEL_3						3

/* RAID in CLONE */
#define RAID_LEVEL_CLONE					4

/* RAID in level 5 */
#define RAID_LEVEL_5						5


/************************************************************************
 * RAID state..
 ************************************************************************/
/* The RAID is in broken state */
#define RAID_STATE_BROKEN					0

/* The RAID is in degrade state */
#define RAID_STATE_DEGRADE      			1

/* The RAID is in rebuilding state */
#define RAID_STATE_REBUILDING				2

/* The RAID is in normal state */
#define RAID_STATE_NORMAL					3

/* The RAID is in migration or expansion state */
#define RAID_STATE_EXPANSION				4

/* The RAID is in backup (rebuilding) state */
#define RAID_STATE_BACKUP					5

/* This RAID is an AES RAID but can't be accessed */
#define RAID_STATE_AES_ERROR				6

/************************************************************************
 * SATA port type.
 ************************************************************************/
/* This port is NULL. No any device is attached */
#define SATA_TYPE_BROKEN					0

/* This port is a pure hard disk */
#define SATA_TYPE_HDD						1

/* This port is a RAID drive */
#define SATA_TYPE_RAID						2

/* This port is a pure optical drive */
#define SATA_TYPE_ODD						3

/* This port is bad or can't be used  */
#define SATA_TYPE_BAD						4

/* This port is turned off */
#define SATA_TYPE_OFF						6

/* This port is used as host */
#define SATA_TYPE_HOST						7

/************************************************************************
 * SATA page state.
 ************************************************************************/
/* This page doesn't use */
#define SATA_PAGE_STATE_NULL				0

/* This page is valid */
#define SATA_PAGE_STATE_VALID				1

/* This page is hooked to Port Multipiler */
#define SATA_PAGE_STATE_HOOK_PM				2

/* This page is used as a spare drive */
#define SATA_PAGE_STATE_SPARE				3

/* This page is bad and can't be used */
#define SATA_PAGE_STATE_BAD					4

/* This page is going to repair */
#define SATA_PAGE_STATE_REPAIR				5

/* This page cannot add to RAID engine */
#define SATA_PAGE_STATE_RAID_FULL			6

/* This page cannot add to Port Multipiler */
#define SATA_PAGE_STATE_PORT_FULL			7


gboolean nf_raid_get_ctrl_cnt(u8 *count);

typedef enum _NF_RAID_CTRL_ID_E {
	NF_RAID_CTRL_ID_INTERNAL	= 0 ,
	NF_RAID_CTRL_ID_EXTERNAL	= 1 ,
	NF_RAID_CTRL_ID_NR			= 2
} NF_RAID_CTRL_ID_E;

gint nf_raid_get_ctrl_id(NF_RAID_CTRL_ID_E mode);

gboolean nf_raid_create(u8 cid, u32 did_mask, u32 raid_level, GError **error);
gboolean nf_raid_delete(u8 cid, u8 rid, GError **error);
gboolean nf_raid_get_ctrl_info(u8 cid, CONTROLLER *cont, GError **error);
gboolean nf_raid_get_raid_info(u8 cid, RAID_PORT *aRAID, GError **error);
gboolean nf_raid_get_disk_info(u8 cid, SATA_PORT *aSATA, GError **error);
gboolean nf_raid_get_fw_ver(u8 cid, FIRMWARE_INFO *tFwInfo, GError **error);
gboolean nf_raid_upgrade_fw(u8 cid, u8* filename, GError **error);
gboolean nf_raid_get_rebuild_per(u8 cid, u8 rid, u8* per, GError **error);
gboolean nf_raid_set_rebuild_priority(u8 cid, u8 rid, u8 pri, GError **error);
gboolean nf_raid_add_spare_disk(u8 cid, u8 did, u8 rid, GError **error);
gboolean nf_raid_del_spare_disk(u8 cid, u8 did, u8 rid, GError **error);
gboolean nf_raid_pause_rebuilding(u8 cid, u8 rid, GError **error);
gboolean nf_raid_resume_rebuilding(u8 cid, u8 rid, GError **error);
gboolean nf_raid_get_smart(u8 cid, u8 did, GError **error);
gboolean nf_raid_get_status(u8 cid, u8 *nEventCount, EVENT_TYPE *aEventInfo, GError **error);



#endif
