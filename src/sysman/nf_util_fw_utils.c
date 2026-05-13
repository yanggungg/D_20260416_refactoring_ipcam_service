#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>	//for mmap
#include <fcntl.h>		//for open
#include <unistd.h>		//for lseek
#include <stdio.h>
#include <dirent.h>		//for opendir
#include <sys/mount.h>

#include "nf_common.h"
#include "nf_util_fw.h"
#include "nf_util_fw_utils.h"
#include "nf_util_flash.h"

#define DEBUG_JBSHELL_FLASH

#ifdef DEBUG_JBSHELL_FLASH
	#include "jbshell.h"
#endif

void nf_fw_imgh_print(image_header_t *img_header)
{
	g_print("==================================================================\n");
	g_message("ih_magic [%08x]", img_header->ih_magic);              /* Image Header Magic Number    */
	g_message("ih_hcrc  [%08x]", img_header->ih_hcrc);               /* Image Header CRC Checksum    */
	g_message("ih_time  [%08x]", img_header->ih_time);               /* Image Creation Timestamp     */
	g_message("ih_size  [%d]", fw_ntohl(img_header->ih_size));       /* Image Data Size              */
	g_message("ih_load  [%08x]", fw_ntohl(img_header->ih_load));     /* Data  Load  Address          */
	g_message("ih_ep    [%08x]", fw_ntohl(img_header->ih_ep));       /* Entry Point Address      */
	g_message("ih_dcrc  [%08x]", img_header->ih_dcrc);               /* Image Data CRC Checksum      */
	g_message("ih_os    [%x]", img_header->ih_os);                   /* Operating System             */
	g_message("ih_arch  [%x]", img_header->ih_arch);                 /* CPU architecture             */
	g_message("ih_type  [%x]", img_header->ih_type);                 /* Image Type                   */
	g_message("ih_comp  [%x]", img_header->ih_comp);                 /* Compression Type             */
	g_message("ih_name  [%s]", img_header->ih_name);                 /* Image Name                   */
	g_print("==================================================================\n");
}

void hexa_print(guchar *dataBuf, guint offs)
{
	gint i;

	for (i = 0; i < NAND_LARGE_BLOCK_SIZE_PAGE; i += 16) {
		g_print("0x%08x: %02x %02x %02x %02x %02x %02x %02x "
				"%02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
				offs + (guint)i,  dataBuf[i],
				dataBuf[i+1], dataBuf[i+2],
				dataBuf[i+3], dataBuf[i+4],
				dataBuf[i+5], dataBuf[i+6],
				dataBuf[i+7], dataBuf[i+8],
				dataBuf[i+9], dataBuf[i+10],
				dataBuf[i+11], dataBuf[i+12],
				dataBuf[i+13], dataBuf[i+14],
				dataBuf[i+15]);

	}
}

void hexa_print_with_oob(guchar *dataBuf, guint offs)
{
	gint i;

	for (i = 0; i < NAND_LARGE_BLOCK_SIZE_PAGE + NAND_LARGE_BLOCK_SIZE_OOB; i += 16) {
		g_print("0x%08x: %02x %02x %02x %02x %02x %02x %02x "
				"%02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
				offs + (guint)i,  dataBuf[i],
				dataBuf[i+1], dataBuf[i+2],
				dataBuf[i+3], dataBuf[i+4],
				dataBuf[i+5], dataBuf[i+6],
				dataBuf[i+7], dataBuf[i+8],
				dataBuf[i+9], dataBuf[i+10],
				dataBuf[i+11], dataBuf[i+12],
				dataBuf[i+13], dataBuf[i+14],
				dataBuf[i+15]);

				if(i == 2047)
					g_print("================================================\n");

	}
}

/**
	@brief                          firmware list
	@param[in]  path                directory path
	@return     gboolean            %TRUE on success, %FALSE if an error occurred
*/
gboolean nf_fw_ls(gchar *path)
{
	DIR *dp;
	struct dirent *dirp;
	struct stat f_stat;
	gint ret;
	gchar filename[FW_UPGRADE_MAX_BUFFER]={0,};

	if(!(dp = opendir(path)))
	{
		g_warning("%s Directory open error!!! Directory namd [%s]",
					__FUNCTION__, path);
		return FALSE;
	}

	g_message("%s\nFirmware Image Directory Name [%s]\n"
			"   Filename                      Size""               Time\n"
			"==========================================================================================",
			 __FUNCTION__, path);

	while((dirp=readdir(dp)) != NULL)
	{
		if(strcmp(dirp->d_name,".") == 0 || strcmp(dirp->d_name,"..") == 0)
			continue;

		if((strlen(dirp->d_name)+ strlen(path)+1) > FW_UPGRADE_MAX_BUFFER)
		{
			g_warning("%s filename is too long.. max length [%d] filename [%s/%s] file length [%d]\n",
							__FUNCTION__, FW_UPGRADE_MAX_BUFFER, path, dirp->d_name,
							strlen(path)+strlen(dirp->d_name)+1);
			continue;
		}

		sprintf(filename,"%s/%s", path, dirp->d_name);

		if(stat(filename, &f_stat) == -1)
		{
			g_warning("%s can't stat [%s]", __FUNCTION__, filename);
			continue;
		}

		if(S_ISDIR(f_stat.st_mode))
			continue;
		else
		{
			//compare string
			if(strncmp(dirp->d_name, FW_UPGRADE_NAME_TITLE, 11) == 0)
				g_print("[%-35s]        [%-15ld]         [%-16ld]\n", filename,  f_stat.st_size, f_stat.st_ctime);

		}   //end if
	}

	g_print("==========================================================================================\n");

	closedir(dp);
	return TRUE;
}

#if defined(DEBUG_JBSHELL_FLASH)
static char nf_fw_ls_help [] = "fw_ls [Directory]";
static int fw_ls(int argc, char **argv)
{
	if ( argc < 2 ) {
		printf("Invalid arguments\n%s\n", nf_fw_ls_help);
		return -1;
	}

	nf_fw_ls(argv[1]);

	return 0;
}
__commandlist(fw_ls, "fw_ls", "fw_ls [Directory]", nf_fw_ls_help);
#endif

