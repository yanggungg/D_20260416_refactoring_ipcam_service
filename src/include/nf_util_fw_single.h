#ifndef __NF_UTIL_FW_SINGLE_H__
#define __NF_UTIL_FW_SINGLE_H__

#define NF_FW_INSTALL_FILE_PROG_PATH	"/tmp"
#if defined(CHIP_NVT)
	#define NF_FW_INSTALL_FILE_PROG_BG		"progress_bg.uyvy"
	#define NF_FW_INSTALL_FILE_PROG_TXT		"progress_txt.uyvy"
	#define NF_FW_INSTALL_FILE_PROG_FAIL	"progress_fail.uyvy"
#else
	#define NF_FW_INSTALL_FILE_PROG_BG		"progress_bg.bmp"
	#define NF_FW_INSTALL_FILE_PROG_TXT		"progress_txt.bmp"
	#define NF_FW_INSTALL_FILE_PROG_FAIL	"progress_fail.bmp"
#endif
#define NF_FW_INSTALL_FILE_DIR			"/NFDVR/data/install" 

#if defined(CONFIG_FWUPGRADE_NET_DOWN)
#define NF_FW_NET_DOWN_FILE_NAME		"/common/uploaded_fw.bin"
#define NF_FW_NET_DOWN_DIR_NAME			"/common/"
#endif

#define NF_FW_CHECK_TRY_DISK_CNT		16

#if 0
typedef enum _NF_FW_PRGT_PRE_E
{
	NF_FW_PRGT_PRE_START                = 0,
	NF_FW_PRGT_PRE_THREAD_CREAT         = 1,
	NF_FW_PRGT_PRE_SEQ                  = 2,
	NF_FW_PRGT_PRE_FILE_OPEN            = 3,
	NF_FW_PRGT_PRE_FILE_READ            = 4,
	NF_FW_PRGT_PRE_S_MAGIC_CHECK        = 5
} NF_FW_PRGT_PRE_E;
#endif

typedef enum _NF_FWUP_MAGIC_KEY_E
{
	NF_FWUP_MAGIC_KEY_NET				= 0xFFFFFFFE,
	NF_FWUP_MAGIC_KEY_RAMDISK			= 0xFFFFFFFF,
} NF_FWUP_MAGIC_KEY;

typedef enum _NF_FWUP_STATUS_E
{
	NF_FWUP_STATUS_DONE						= 0,
	NF_FWUP_STATUS_FAIL						= 1,
	NF_FWUP_STATUS_NET_FAIL					= 2,
	NF_FWUP_STATUS_RAM_FAIL					= 3,
	NF_FWUP_STATUS_NET_DOWN_FAIL			= 4,
	NF_FWUP_STATUS_UPGRADE					= 0x10,
	NF_FWUP_STATUS_UPGRADE_DONE				= 0x20,
	NF_FWUP_STATUS_UPGRADE_FAIL				= 0x21,
	NF_FWUP_STATUS_DB_FAIL					= 0x22,
	NF_FWUP_STATUS_UPGRADE_NET				= 0x30,
	NF_FWUP_STATUS_UPGRADE_NET_DONE			= 0x40,
	NF_FWUP_STATUS_UPGRADE_NET_FAIL			= 0x41,
	NF_FWUP_STATUS_NET_DB_FAIL				= 0x42,
	NF_FWUP_STATUS_UPGRADE_RAM				= 0x50,
	NF_FWUP_STATUS_UPGRADE_RAM_DONE			= 0x60,
	NF_FWUP_STATUS_UPGRADE_RAM_FAIL			= 0x61,
	NF_FWUP_STATUS_RAM_DB_FAIL				= 0x62,
	NF_FWUP_STATUS_UPGRADE_NET_DOWN         = 0x80,
	NF_FWUP_STATUS_UPGRADE_NET_DOWN_DONE    = 0x81,
	NF_FWUP_STATUS_UPGRADE_NET_DOWN_FAIL    = 0x82,

} NF_FWUP_STATUS_E;

gboolean nf_fw_update_clear(void);
guint nf_fw_update_check(void);
void nf_fw_update_check_clear(void);
gboolean nf_fw_update_verify(char* data_path);
gboolean nf_fw_data_backup(const gchar *mnt_path);
gboolean nf_fw_write_upgrade_info(gchar *fw_filename);
#if defined(CONFIG_FWUPGRADE_NET_DOWN)
	gboolean nf_fw_chk_version(char *filename);
#endif

#endif

