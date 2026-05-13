#include <fcntl.h>      //for open
#include <unistd.h>     //for lseek
#include <stdio.h>

#include "nf_util_fw.h"
#include "nf_util_fw_crc.h"
#if defined(SUPPORT_STORAGE_NAND)
#else
#include "nf_util_mmc.h"
#include "nf_util_ext4.h"
#endif

#include "nf_util_fw_crc_tbl.c"

#define DO1(buf) crc = crc_table[((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8);
#define DO2(buf)  DO1(buf); DO1(buf);
#define DO4(buf)  DO2(buf); DO2(buf);
#define DO8(buf)  DO4(buf); DO4(buf);

#if defined(SUPPORT_STORAGE_NAND)
#define NF_FW_TURNING_CRC_CHECK             // 20121113

gulong fwup_chk_crc(gulong crc, gint mtd_num, guint len, gboolean is_header, gint img_type, gint prgt_type)
{
	#if defined(NF_FW_TURNING_CRC_CHECK)
		guchar dataBuf[NAND_ERASE_SIZE] = {0,};
	#else
		guchar dataBuf[NAND_LARGE_BLOCK_SIZE_PAGE] = {0,};
	#endif
	guchar oobBuf[NAND_LARGE_BLOCK_SIZE_OOB] = {0,};
	guchar *ptr=NULL;
	guint crc_read_cnt=0, offs=0, actual_len=0;
	gboolean state=TRUE, block_state=FALSE, remain_read_state=FALSE;
	gint filesys_page_cnt=0, tot_blk_cnt=0, cur_blk_cnt=0, cur_len=0, remain=0;

	crc = crc ^ 0xffffffffL;

	///////////////////////////////////////////////////
	/**
		Bug Fix... 20120302
		Actual Len is Real Filesize
		len is crc check len!!
	**/
//  g_message("%s mtd_num[%d] len[%d] is_header[%d] type[%d]", __FUNCTION__, mtd_num, len, is_header, img_type);

	if(is_header)
		actual_len = (len + sizeof(image_header_t));        // Actual Len.. To remain len calculate
	else
		actual_len = len;

#if !defined(SUPPORT_UBIFS)
	if(img_type == NF_FW_TYPE_FILESYS)
	{
		filesys_page_cnt = (gint)(actual_len / (FW_UPGRADE_NAND_PAGE_SIZE+FW_UPGRADE_NAND_OOB_SIZE));
		len = (guint)(filesys_page_cnt * FW_UPGRADE_NAND_PAGE_SIZE);
	}
#endif

	if((actual_len % NAND_ERASE_SIZE) == 0)
		tot_blk_cnt = (gint)(actual_len / NAND_ERASE_SIZE);
	else
	{
		tot_blk_cnt = (gint)((actual_len / NAND_ERASE_SIZE) + 1);

		#if defined(NF_FW_TURNING_CRC_CHECK)
			remain = (gint)(actual_len % NAND_ERASE_SIZE);
		#else
			remain = (gint)(actual_len % NAND_LARGE_BLOCK_SIZE_PAGE);
		#endif
		if(remain < 8)
			remain_read_state = TRUE;
	}
	///////////////////////////////////////////////////

	while (len >= 8)
	{
		if(img_type == NF_FW_TYPE_FILESYS)
		{
			if((cur_len % 0x100000) == 0)       // 1M
				g_message("%s Crc Check Dubug .. Current[%d]", __FUNCTION__, cur_len);
		}

		if((cur_len % NAND_ERASE_SIZE) == 0)
		{
			cur_blk_cnt++;
			nf_fw_set_prgt_state((gshort)prgt_type, NF_FW_PRGT_IMG_CRC_CHECK, FW_UPGRADE_PRGT_OK,
									(guint)cur_blk_cnt, (guint)tot_blk_cnt);
			#if 0
				g_usleep(33000);
			#else
				g_usleep(3300);
			#endif
		}
		else
		{
			if(block_state == FALSE)
			{
				if(len < NAND_ERASE_SIZE)
				{
					cur_blk_cnt++;
					nf_fw_set_prgt_state((gshort)prgt_type, NF_FW_PRGT_IMG_CRC_CHECK, FW_UPGRADE_PRGT_OK,
										(guint)cur_blk_cnt, (guint)tot_blk_cnt);
				}
				block_state = TRUE;
			}
		}

		#if defined(NF_FW_TURNING_CRC_CHECK)
		if((crc_read_cnt % NAND_ERASE_SIZE) == 0)
		#else
		if((crc_read_cnt % NAND_LARGE_BLOCK_SIZE_PAGE) == 0)
		#endif
		{
			memset(dataBuf, 0x0, sizeof(dataBuf));
			#if defined(NF_FW_TURNING_CRC_CHECK)
			if(nf_flash_read_block((gint)mtd_num, offs, dataBuf) == 0)
			#else
			if(nf_flash_read((gint)mtd_num, offs, 0, dataBuf, NULL) == 0)
			#endif
			{
				g_warning("%s Nand Flash Read Error!!!", __FUNCTION__);
				return crc ^ 0xffffffffL;
			}

			if(state)
			{
				if(is_header)
				{
					ptr = dataBuf + FW_UPGRADE_IMG_HEADER_SIZE;
					crc_read_cnt += FW_UPGRADE_IMG_HEADER_SIZE;
				}
				else
				{
					ptr = dataBuf;
					crc_read_cnt = 0;
				}

				state = FALSE;
			}
			else
			{
				ptr = dataBuf;
				crc_read_cnt = 0;
			}

			#if defined(NF_FW_TURNING_CRC_CHECK)
				offs += NAND_ERASE_SIZE;
			#else
				offs += NAND_LARGE_BLOCK_SIZE_PAGE;
			#endif
		}

		DO8(ptr);
		len -= 8;
		crc_read_cnt +=8;
		cur_len += 8;

		// g_usleep(0);
	}

	if (len) do {

		g_message("Remain Length CRC Calculate.. Len[%d]", len);

		if(remain_read_state == TRUE)
		{
			g_message("You Must Remainig Data Read!!! Remain Len[%d]", len);

			if(nf_flash_read((gint)mtd_num, offs, 0, dataBuf, NULL) == 0)
			{
				g_warning("%s Nand Flash Read Error!!! Mtd[%d] Offset[0x%08x]", __FUNCTION__, mtd_num, offs);
				return crc ^ 0xffffffffL;
			}

			ptr = dataBuf;
			remain_read_state = FALSE;
		}

		DO1(ptr);

		g_usleep(3300);

	} while (--len);

	return crc ^ 0xffffffffL;
}
#else

#define EMMC_SIZE_BLK_RD_CNT    20
#define EMMC_SIZE_BLK_RD        (EMMC_SIZE_BLK * EMMC_SIZE_BLK_RD_CNT)

#define EMMC_SIZE_RD_CRC        0x100000
gulong fwup_chk_crc(gulong crc, gint mtd_num, guint len, gboolean is_header, gint img_type, gint prgt_type)
{
	guchar *dataBuf=NULL;
	guchar *ptr=NULL;
	guint crc_read_cnt=0, offs=0, actual_len=0;
	gboolean state=TRUE, block_state=FALSE, remain_read_state=FALSE;
	gint filesys_page_cnt=0, tot_blk_cnt=0, cur_blk_cnt=0, cur_len=0, remain=0;

	dataBuf=(unsigned char *)malloc(EMMC_SIZE_RD_CRC);
	crc = crc ^ 0xffffffffL;
	
	///////////////////////////////////////////////////
	/**
		Bug Fix... 20120302
		Actual Len is Real Filesize
		len is crc check len!!
	**/
	#if 0
		g_message("%s mtd_num[%d] len[%d] is_header[%d] type[%d]", __FUNCTION__, mtd_num, len, is_header, img_type);
	#endif
	if(is_header)
		actual_len = (len + sizeof(image_header_t));        // Actual Len.. To remain len calculate
	else
		actual_len = len;
		
	#if !defined(SUPPORT_UBIFS)
		if(img_type == NF_FW_TYPE_FILESYS)
		{
			filesys_page_cnt = (gint)(actual_len / (FW_UPGRADE_NAND_PAGE_SIZE+FW_UPGRADE_NAND_OOB_SIZE));
			len = (guint)(filesys_page_cnt * FW_UPGRADE_NAND_PAGE_SIZE);
		}   
	#endif  

	if((actual_len % EMMC_SIZE_RD_CRC) == 0)
		tot_blk_cnt = (gint)(actual_len / EMMC_SIZE_RD_CRC);
	else
	{
		tot_blk_cnt = (gint)((actual_len / EMMC_SIZE_RD_CRC) + 1);

		remain = (gint)(actual_len % EMMC_SIZE_RD_CRC);
		if(remain < 8)
			remain_read_state = TRUE;
	}

	while (len >= 8)
	{
		if(img_type == NF_FW_TYPE_FILESYS)
		{
			if((cur_len % 0x100000) == 0)       // 1M
				g_message("%s Crc Check Dubug .. Current[%d]", __FUNCTION__, cur_len);
		}       
	
		if((cur_len % EMMC_SIZE_RD_CRC) == 0)
		{
			cur_blk_cnt++;
			nf_fw_set_prgt_state((gshort)prgt_type, NF_FW_PRGT_IMG_CRC_CHECK, FW_UPGRADE_PRGT_OK,
									(guint)cur_blk_cnt, (guint)tot_blk_cnt);
			g_usleep(100);
		}
		else
		{
			if(block_state == FALSE)
			{
				if(len < EMMC_SIZE_RD_CRC)
				{
					cur_blk_cnt++;
					nf_fw_set_prgt_state((gshort)prgt_type, NF_FW_PRGT_IMG_CRC_CHECK, FW_UPGRADE_PRGT_OK, (guint)cur_blk_cnt, 
										(guint)tot_blk_cnt);
				}
				block_state = TRUE;
			}
		}

		if((crc_read_cnt % EMMC_SIZE_RD_CRC) == 0)
		{
			memset(dataBuf, 0x0, EMMC_SIZE_RD_CRC);
			if(nf_mmc_read(mtd_num, offs, dataBuf, EMMC_SIZE_RD_CRC) == 0)
			{
				g_warning("%s Nand Flash Read Error!!!", __FUNCTION__);
				return crc ^ 0xffffffffL;
			}

			if(state)
			{
				if(is_header)
				{
					ptr = dataBuf + FW_UPGRADE_IMG_HEADER_SIZE;
					crc_read_cnt += FW_UPGRADE_IMG_HEADER_SIZE;
				}
				else
				{
					ptr = dataBuf;
					crc_read_cnt = 0;
				}

				state = FALSE;
			}
			else
			{
				ptr = dataBuf;
				crc_read_cnt = 0;
			}

			offs+=EMMC_SIZE_RD_CRC;
		}

		DO8(ptr);
		len -= 8;
		crc_read_cnt +=8;
		cur_len += 8;

//      g_usleep(10);
		// g_usleep(0);
	}

	if (len) do {

		g_message("Remain Length CRC Calculate.. Len[%d]", len);

		if(remain_read_state == TRUE)
		{
			g_message("You Must Remainig Data Read!!! Remain Len[%d]", len);

			if(nf_mmc_read(mtd_num, offs, dataBuf, EMMC_SIZE_RD_CRC) == 0)
			{
				g_warning("%s MMC Read Error!!! Mtd[%d] Offset[0x%08x]", __FUNCTION__, mtd_num, offs);
				return crc ^ 0xffffffffL;
			}

			ptr = dataBuf;
			remain_read_state = FALSE;
		}

		DO1(ptr);

		g_usleep(100);

	} while (--len);

	free(dataBuf);

	return crc ^ 0xffffffffL;
}

gulong fwup_chk_crc_ext4fs(FILE *fp, guint location_fs, gulong crc, gint mtd_num, guint len, gboolean is_header,
										gint img_type, gint prgt_type)
{
	guchar *dataBuf=NULL;
	guchar *ptr=NULL;
	guint crc_read_cnt=0, crc_chk_cnt=0, offs=0, actual_len=0;
	gboolean state=TRUE, block_state=FALSE, remain_read_state=FALSE;
	gint filesys_page_cnt=0, tot_blk_cnt=0, cur_blk_cnt=0, cur_len=0, remain=0;

	chunk_header_t chunk;
	sparse_header_t header;
	int i=0;
	unsigned long location=0, location_fd=0, tmp=0;
	unsigned int chunk_len=0;
	int cnt_start=0, blkcnt=0;
	unsigned char *data=NULL;
	unsigned int size=0;
	unsigned long long dense_len=0, sparse_len=0;
	unsigned int blk=0;
	char str_blk[64]={0 ,};
	int fd=0;
	unsigned offset_crc=0;

	int test=0;

	dataBuf=(unsigned char *)malloc(EMMC_SIZE_RD_CRC);

	sprintf(str_blk, MMC_DEV_NAME, mtd_num+1);
	if ((fd = open(str_blk, O_RDWR)) < 0)
	{
		printf("MMC%d Device Open Error\n", mtd_num);
		printf("Device %s\n", str_blk);

		return crc ^ 0xffffffffL;
	}


	///////////////////////////////////////////////////
	/**
		Bug Fix... 20120302
		Actual Len is Real Filesize
		len is crc check len!!
	**/
	#if 0
		g_message("%s mtd_num[%d] len[%d] is_header[%d] type[%d]", __FUNCTION__, mtd_num, len, is_header, img_type);
	#endif

	if(is_header) {
		fseek(fp, (long int)location_fs, SEEK_SET);
	}
	else {
		fseek(fp, (long int)(location_fs + FW_UPGRADE_IMG_HEADER_SIZE), SEEK_SET);
	}

	location=(long unsigned int)ftell(fp);
	if(fread(&header, sizeof(sparse_header_t), 1, fp) != 1)
	{
		printf("%s Cannot Read Header\n", __FUNCTION__);
		goto fwupgrade_crc_check_fail;
	}

	fseek(fp, (long int)location, SEEK_SET);
	#if 0
		if (!is_sparse_image(&header)) {
			printf("Invalid sparse format.\n");
			return FALSE;
		}
	#endif

	/* skip the sparse header,to visit first chunk */
	fseek(fp, (long int)header.file_hdr_sz, SEEK_CUR);

	#if 0
		print_header_info(&header);
	#endif

	actual_len=len;
	if((actual_len % EMMC_SIZE_RD_CRC) == 0)
		tot_blk_cnt = (gint)(actual_len / EMMC_SIZE_RD_CRC);
	else
	{
		tot_blk_cnt = (gint)((actual_len / EMMC_SIZE_RD_CRC) + 1);

		remain = (gint)(actual_len % EMMC_SIZE_RD_CRC);
		if(remain < 8)
			remain_read_state = TRUE;
	}

	crc = crc ^ 0xffffffffL;
	for (i = 0; i < (int)header.total_chunks; i++)
	{
		location=(long unsigned int)ftell(fp);
		if(fread(&chunk, sizeof(chunk_header_t), 1, fp) != 1)
		{
			printf("%s Cannot Read Chunk Header\n", __FUNCTION__);
			goto fwupgrade_crc_check_fail;
		}

		fseek(fp, (long int)location, SEEK_SET);
		location=(long unsigned int)ftell(fp);
	//  printf("--------> location_chunk2 %d\n", location);

		/* go to next chunk's data */
		fseek(fp, header.chunk_hdr_sz, SEEK_CUR);
		location=(long unsigned int)ftell(fp);

		switch (chunk.chunk_type) {
		case CHUNK_TYPE_RAW:

			/* to calculate the length of each chunk */
			chunk_len = chunk.chunk_sz * header.blk_sz;

			dense_len += chunk_len;
			sparse_len += chunk_len;


			#if 0
				printf("chunk_type_raw start 0x%08x blkcnt 0x%08x\n", blk, (chunk_len >> EMMC_BLKSIZE_SHIFT));
			#endif

			location=(long unsigned int)ftell(fp);
			blkcnt=(int)(chunk_len >> EMMC_BLKSIZE_SHIFT);


			size=(unsigned int)(blkcnt * EMMC_SIZE_BLK);
			data=(unsigned char *)malloc(size);

			tmp=(long unsigned int)ftell(fp) - location_fs;

			location_fd=(blk * EMMC_SIZE_BLK);
			if (lseek (fd, (__off_t)location_fd, SEEK_SET) < 0)
			{
				printf("lseek error!!\n");
				free(data);
				goto fwupgrade_crc_check_fail;
			}

			if(read(fd, data, size) < 0)
			{
				printf("%s Cannot Read MMC Data For CRC\n", __FUNCTION__);
				free(data);
				goto fwupgrade_crc_check_fail;
			}

			crc_chk_cnt=size;
			offset_crc+=size;
			len=size;
			ptr=data;
			while (len >= 8)
			{
				if((cur_len % EMMC_SIZE_RD_CRC) == 0)
				{
					cur_blk_cnt++;
					nf_fw_set_prgt_state((gshort)prgt_type, NF_FW_PRGT_IMG_CRC_CHECK, FW_UPGRADE_PRGT_OK,
											(guint)cur_blk_cnt, (guint)tot_blk_cnt);
					g_usleep(100);
				}

				DO8(ptr);
				len -= 8;
				crc_read_cnt +=8;
				cur_len += 8;
			}

			fseek(fp, (long int)location, SEEK_SET);
			location=(long unsigned int)ftell(fp);

			free(data);

			location=(long unsigned int)ftell(fp);
			fseek(fp, 0, SEEK_SET);
			fseek(fp, (long int)(location+chunk_len), SEEK_CUR);
			location=(long unsigned int)ftell(fp);
			//printf("location %d\n", location);

			blk += (chunk_len >> EMMC_BLKSIZE_SHIFT);

			break;

		case CHUNK_TYPE_DONT_CARE:
			if (chunk.total_sz != header.chunk_hdr_sz) {
				printf("No.%d chunk size error.\n", i);
				print_chunk_info(&chunk);
				goto fwupgrade_crc_check_fail;
			}

			chunk_len = chunk.chunk_sz * header.blk_sz;
			sparse_len += chunk_len;

			#if 0
				printf("chunk_type_dont_care shift [0x%08x] chunk_len[%d] sparse_len[%d]\n", 
							(chunk_len >> EMMC_BLKSIZE_SHIFT), chunk_len, sparse_len);
			#endif
			blk  += (chunk_len >> EMMC_BLKSIZE_SHIFT);
			break;

		default:
			printf("sparse: unknow chunk type %04x.\n",
				chunk.chunk_type);
			goto fwupgrade_crc_check_fail;
		}
	}

	free(dataBuf);

	return crc ^ 0xffffffffL;

fwupgrade_crc_check_fail:
	free(dataBuf);

	return crc ^ 0xffffffffL;
}
#endif

/**
	@brief                          image header crc check
	@param[in]  crc                 0
	@param[in]  buf                 header data
	@param[in]  len                 header length                   
	@return     gboolean            %TRUE on success, %FALSE if an error occurred
*/
gulong header_crc_check(gulong crc, guchar *buf, guint len)
{
	crc = crc ^ 0xffffffffL;
	while (len >= 8)
	{
		DO8(buf);
		len -= 8;
	}

	if (len) do {
		DO1(buf);
	} while (--len);

	return crc ^ 0xffffffffL;
}

/**
	@brief                          data crc check
	@param[in]  crc                 0
	@param[in]  fp                  file pointer
	@param[in]  len                 data length                 
	@return     gboolean            %TRUE on success, %FALSE if an error occurred
*/
gulong data_crc_check(gulong crc, FILE *fp, guint len, gint img_type)
{
	guchar buf[8]={0,}, *ptr=NULL;
	gint filesys_page_cnt=0;
	gulong tot_rd_len=0;
	glong cur_offs=0;
	#if !defined(SUPPORT_UBIFS)
	if(img_type == NF_FW_TYPE_FILESYS)
	{
		filesys_page_cnt = (gint)(len / (FW_UPGRADE_NAND_PAGE_SIZE+FW_UPGRADE_NAND_OOB_SIZE));
		len = (guint)(filesys_page_cnt * FW_UPGRADE_NAND_PAGE_SIZE);
	}
	#endif
	crc = crc ^ 0xffffffffL;
	while (len >= 8)
	{
		#if !defined(SUPPORT_UBIFS)
		if(img_type == NF_FW_TYPE_FILESYS)
		{
			if((tot_rd_len != 0) && ((tot_rd_len%FW_UPGRADE_NAND_PAGE_SIZE)==0))
			{
				cur_offs=ftell(fp);
				fseek(fp, cur_offs+(glong)FW_UPGRADE_IMG_HEADER_SIZE, SEEK_SET);
			}
		}
		#endif
		memset(buf, 0x0, sizeof(buf));
		ptr = buf;

		if(fread(buf, 8, 1, fp) != 1)
		{
			g_warning("%s data fread() Error!!!", __FUNCTION__);
			break;
		}

		DO8(ptr);
		len -= 8;
		tot_rd_len+=8;
	}

	if (len) do {
		g_message("%s remain len write[%d]", __FUNCTION__, len);
		memset(buf, 0x0, sizeof(buf));
		ptr = buf;

		if(fread(buf, 1, 1, fp) != 1)
		{
			g_warning("%s Remaining data fread() Error!!!", __FUNCTION__);
			break;
		}

		DO1(ptr);
	} while (--len);

	#if !defined(SUPPORT_UBIFS)
	if(img_type == NF_FW_TYPE_FILESYS)
	{
		cur_offs=ftell(fp);
		fseek(fp, cur_offs+(glong)FW_UPGRADE_IMG_HEADER_SIZE, SEEK_SET);
	}
	#endif
	return crc ^ 0xffffffffL;
}

