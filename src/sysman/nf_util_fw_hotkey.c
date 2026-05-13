#include "nf_common.h"
#include "nf_util_fw.h"
#include "nf_util_fw_hotkey.h"

gboolean nf_fw_hotkey_upgrade(void)
{
	gint ret=0, i=0;
	guint src_filelen=0, dest_filelen=0;
	gchar dev_name[FW_UPGRADE_HKEY_MAX_BUF]={0, };
	gchar fw_filename[FW_UPGRADE_HKEY_MAX_BUF]={0, };
	FILE *fp=NULL;
	struct stat f_stat;

	ret = mkdir(FW_UPGRADE_HKEY_MOUNT_DIR_NAME, 0755);
	if(ret != 0)
		g_warning("%s %s exists..", __FUNCTION__, FW_UPGRADE_HKEY_MOUNT_DIR_NAME);

	umount(FW_UPGRADE_HKEY_MOUNT_DIR_NAME);

	for(i='a';i<='z';++i)
	{
		sprintf(dev_name, FW_UPGRADE_HKEY_DEV_NAME, i);
		if((fp=fopen(dev_name, "r")) == NULL)
		{
			g_warning("%s Device Open Error!!! USB Device Name [%s]",
					__FUNCTION__, dev_name);
		}
		else {
			g_message("%s fw_filename : [%s]", __FUNCTION__, dev_name);
			break;
		}
	}

	g_return_val_if_fail(fp != NULL, 0);
	fclose(fp);

	if((ret = mount(dev_name, FW_UPGRADE_HKEY_MOUNT_DIR_NAME,
							FW_UPGRADE_HKEY_MOUNT_FS_TYPE , 0, 0)) !=0)
	{
		g_warning("%s mount error!!! ret [%d]", __FUNCTION__, ret);
		return FALSE;
	}else{
		g_message("%s mounted [%s]->[%s]",__FUNCTION__,
					dev_name, FW_UPGRADE_HKEY_MOUNT_DIR_NAME);
	}


	snprintf(fw_filename, sizeof(fw_filename), "%s/%s",
				FW_UPGRADE_HKEY_MOUNT_DIR_NAME,
				FW_UPGRADE_HEY_FILE_NAME);

	if(stat(fw_filename, &f_stat) == -1)
	{
		g_warning("%s Cannot find src fw_filename [%s]", __FUNCTION__, fw_filename);
		ret = FALSE;
		//goto ret_func;
	}
	src_filelen = (guint)f_stat.st_size;
	g_message("%s fwfile[%s] len[%d]", __FUNCTION__, fw_filename, src_filelen);

	nf_fw_start_upgrade_beep();
	nf_fw_start_upgrade(fw_filename);
	g_usleep(500000*2);
	nf_fw_finish_upgrade_beep();

	return TRUE;
}

