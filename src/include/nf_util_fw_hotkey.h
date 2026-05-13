#ifndef __NF_UTIL_FW_HOTKEY_H__
#define __NF_UTIL_FW_HOTKEY_H__

#define FW_UPGRADE_HEY_FILE_NAME			"20110.1.1.100.nbn"
#define FW_UPGRADE_HKEY_MAX_BUF				256
#define FW_UPGRADE_HKEY_DEV_NAME			"/dev/sd%c1"
#define FW_UPGRADE_HKEY_MOUNT_DIR_NAME		"/mnt/usb_fwup"
#define FW_UPGRADE_HKEY_MOUNT_FS_TYPE		"vfat"

gboolean nf_fw_hotkey_upgrade(void);

#endif

