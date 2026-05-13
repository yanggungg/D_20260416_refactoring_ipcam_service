#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdint.h>
#include <libgen.h>
#include <time.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/mman.h>

#include <asm/types.h>	

#include "nf_util_flash.h"

#define DEBUG_JBSHELL_FLASH
 
#ifdef DEBUG_JBSHELL_FLASH
	#include "jbshell.h"
#endif
//#define DEBUG_NF_FLASH_PROG 
static struct nand_oobinfo none_oobinfo = {
	.useecc = MTD_NANDECC_OFF,
};

static guchar _readbuf[NAND_LARGE_BLOCK_SIZE_PAGE];
static guchar _oobbuf[NAND_LARGE_BLOCK_SIZE_OOB];

static guchar _writebuf[NAND_LARGE_BLOCK_SIZE_PAGE];
static guchar _oobreadbuf[NAND_LARGE_BLOCK_SIZE_OOB];

static struct nand_oobinfo autoplace_oobinfo = {
	.useecc = MTD_NANDECC_AUTOPLACE
};

NF_UTIL_FLASH_PROGRESS _flash_prog;

void prog_cb_func(NF_UTIL_FLASH_PROGRESS *data, gpointer context);
static void flash_test_cb(NF_UTIL_FLASH_PROGRESS *data, gpointer context);
void flash_print_cmp(guchar *dataBuf, guchar *dataBuf2, gint force_print);

/** extern function definition **/
extern void nf_fw_cb_func(NF_UTIL_FLASH_PROGRESS *data, gpointer context);
ssize_t pread(int fd, void *buf, size_t count, off_t offset);
ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset);

/**
	@brief							flash dump	
	@param[in]	mtd_block_num		mtd block number	
	@param[in]	dump_path			path and file name	ex)/home/dump.img
	@return		gboolean			%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_flash_dump(gint mtd_block_num, gchar* dump_path)
{
	gulong ofs, end_addr = 0;
	unsigned long long blockstart = 1;
	gint ret, i, fd, ofd, bs, badblock = 0;
	struct mtd_oob_buf oob = {0, 16, _oobbuf};
	mtd_info_t meminfo;
	gchar pretty_buf[80];
	gint oobinfochanged = 0 ;
	struct nand_oobinfo old_oobinfo;
	struct mtd_ecc_stats stat1, stat2;
	gchar mtddev[FLASH_BUF_MAX_LEN] = {0,};
	gchar dumpfile[FLASH_BUF_MAX_LEN] ={0,};
	// options
	gint omitbad =0;
	gint eccstats = 0;
	gint pretty_print =1;				// print nice in ascii
	gint omitoob =0;				// omit oob data
	gint noecc =1;						// don't error correct
	gulong	length =0;					// dump length
	gulong	start_addr = 0;			// start address

	g_return_val_if_fail(mtd_block_num < MAX_MTD_PARTION_CNT && mtd_block_num >=0, 0);

	sprintf(mtddev,"/dev/mtd%d", mtd_block_num);
	g_message("%s mtddev [%s]", __FUNCTION__, mtddev);

	if(dump_path != NULL)
		strcpy(dumpfile, dump_path);

	/* Open MTD device */
	if ((fd = open(mtddev, O_RDONLY)) == -1) {
		g_warning("%s open flash device error fd[%d] mtddev[%s]", __FUNCTION__, fd, mtddev);
		return 0;
	}

	/* Fill in MTD device capability structure */
	if (ioctl(fd, MEMGETINFO, &meminfo) != 0) {
		g_warning("%s MEMGETINFO Ioctl Error.. fd[%d]", __FUNCTION__, fd);
		close(fd);
		return 0;
	}

	/* Make sure device page sizes are valid */
	if (!(meminfo.oobsize == 64	&& meminfo.writesize == 2048)	&&
		!(meminfo.oobsize == 128&& meminfo.writesize == 2048)	&&
		!(meminfo.oobsize == 32	&& meminfo.writesize == 1024)	&&
		!(meminfo.oobsize == 16 && meminfo.writesize == 512 )	&&
		!(meminfo.oobsize == 8 	&& meminfo.writesize == 256 )	&&
		!(meminfo.oobsize == 200&& meminfo.writesize == 4096)) 
	{
		g_warning("%s invalid page size. meminfo.oobsize[%d] meminfo.writesize[%d]",
					__FUNCTION__, meminfo.oobsize, meminfo.writesize);
		close(fd);
		return 0;
	}
		
	/* Read the real oob length */
	oob.length = meminfo.oobsize;	//64

	//oobinfochanged == 1
	if (noecc)  {
		ret = ioctl(fd, MTDFILEMODE, (void *) MTD_MODE_RAW);	//ret -1
		if (ret == 0) {
			oobinfochanged = 2;
		} 
		else {
			switch (errno) {
				case ENOTTY:
					if (ioctl (fd, MEMGETOOBSEL, &old_oobinfo) != 0) {
						g_warning ("%s MEMGETOOBSEL Ioctl Error..", __FUNCTION__);
						close (fd);
						return 0;
					}
					if (ioctl (fd, MEMSETOOBSEL, &none_oobinfo) != 0) {
						g_warning ("%s MEMSETOOBSEL Ioctl Error..",__FUNCTION__);
						close (fd);
						return 0;
					}
					oobinfochanged = 1;			//here
					break;
				default:
					g_warning ("%s MTDFILEMODE Ioctl Error..",__FUNCTION__);
					close (fd);
					return 0;
			}
		}
	}
   	else {
		/* check if we can read ecc stats */
		if (!ioctl(fd, ECCGETSTATS, &stat1)) {
			eccstats = 1;
			g_warning("%s ECC failed           [%d]", __FUNCTION__, stat1.failed);
			g_warning("%s ECC corrected        [%d]", __FUNCTION__, stat1.corrected);
			g_warning("%s Number of bad blocks [%d]", __FUNCTION__, stat1.badblocks);
			g_warning("%s Number of bbt blocks [%d]", __FUNCTION__, stat1.bbtblocks);
		} else
			g_warning("%s No ECC status information available", __FUNCTION__);
	}

	/* Open output file for writing. If file name is "-", write to standard
	* output. */
	if (dump_path == NULL)
	{
		ofd = STDOUT_FILENO;
	}
   	else if ((ofd = open(dumpfile, O_WRONLY | O_TRUNC | O_CREAT, 0644))== -1)
	{
		g_warning ("%s Output fd[%d] Dumpfile[%s]", __FUNCTION__, ofd, dumpfile);
		close(fd);
		return 0;
	}

	/* Initialize start/end addresses and block size */
	if (length)
		end_addr = start_addr + length;
	if (!length || end_addr > meminfo.size)
		end_addr = meminfo.size;

	bs = (gint)meminfo.writesize;	//2048

	/* Print informative message */
	g_message("%s Block size %u, PageSize %u, OobSize %u\n",
				__FUNCTION__, meminfo.erasesize, meminfo.writesize, meminfo.oobsize);
	g_message("%s Dumping data starting at 0x%08x and ending at 0x%08x...\n",
				__FUNCTION__, (guint)start_addr, (guint)end_addr);

	/* Dump the flash contents */
	for (ofs = start_addr; ofs < end_addr ; ofs+=(gulong)bs) {
		// new eraseblock , check for bad block
		if (blockstart != (ofs & (~meminfo.erasesize + 1))) {
			blockstart = ofs & (~meminfo.erasesize + 1);
			
			if ((badblock = ioctl(fd, MEMGETBADBLOCK, &blockstart)) < 0) {
				g_warning("%s MEMGETBADBLOCK Ioctl Error Ret[%d]", __FUNCTION__, badblock);
				goto closeall;
			}
		}

		if (badblock)
		{
			if (omitbad)
				continue;
			memset ((void*)_readbuf, 0xff, (guint)bs);
		}
		else
		{
			 memset ((void*)_readbuf, 0xff, (guint)bs);
			/* Read page data and exit on failure */
			if (pread(fd, _readbuf, (size_t)bs, (glong)ofs) != bs)
			{
				g_warning("%s Nand Flash Page Read Rrror.. Offset[0x%lx]", __FUNCTION__, ofs);
				goto closeall;
			}
		}

		/* ECC stats available ? */
		if (eccstats) {
			if (ioctl(fd, ECCGETSTATS, &stat2)) {
				g_warning("%s ECCGETSTATS Ioctl Error..", __FUNCTION__);
				goto closeall;
			}
			if (stat1.failed != stat2.failed)
				g_warning("%s ECC: %d uncorrectable bitflip(s)"" at offset 0x%08lx\n",	
						__FUNCTION__, stat2.failed - stat1.failed, ofs);
			if (stat1.corrected != stat2.corrected)
				g_warning("%s ECC: %d corrected bitflip(s) at"" offset 0x%08lx\n", 
						__FUNCTION__, stat2.corrected - stat1.corrected, ofs);
			stat1 = stat2;
		}

		/* Write out page data */
		if (pretty_print) {
			for (i = 0; i < bs; i += 16) {
				sprintf(pretty_buf,
						"0x%08x: %02x %02x %02x %02x %02x %02x %02x "
						"%02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
						(guint)(ofs + (gulong)i),  _readbuf[i],
						_readbuf[i+1], _readbuf[i+2],
						_readbuf[i+3], _readbuf[i+4],
						_readbuf[i+5], _readbuf[i+6],
						_readbuf[i+7], _readbuf[i+8],
						_readbuf[i+9], _readbuf[i+10],
						_readbuf[i+11], _readbuf[i+12],
						_readbuf[i+13], _readbuf[i+14],
						_readbuf[i+15]);
				ret = write(ofd, pretty_buf, 60);
				if(ret != 60)
					g_warning("%s write error.. ret bytes [%d]", __FUNCTION__, ret);

			}
		}
		else
		{
			ret = write(ofd, _readbuf, (guint)bs);
			if(ret != bs)
				g_warning("%s write error.. ret bytes [%d]", __FUNCTION__, ret);

		}

		if(omitoob)
			continue;

		if(badblock)
		{
			memset (_readbuf, 0xff, meminfo.oobsize);
		}
		else {
			/* Read OOB data and exit on failure */
			oob.start = ofs;
			if (ioctl(fd, MEMREADOOB, &oob) != 0) {
				g_warning("%s MEMREADOOB Ioctl Error.. ofs[%ld]", __FUNCTION__, ofs);
				goto closeall;
			}
		}

		/* Write out OOB data */
		if (pretty_print) {
			if (meminfo.oobsize < 16) {
				sprintf(pretty_buf, "  OOB Data: %02x %02x %02x %02x %02x %02x "
								"%02x %02x\n",
								_oobbuf[0], _oobbuf[1], _oobbuf[2],
								_oobbuf[3], _oobbuf[4], _oobbuf[5],
								_oobbuf[6], _oobbuf[7]);
				ret = write(ofd, pretty_buf, 48);
				if(ret != 48)
					g_warning("%s Flash Write Error.. ret bytes [%d]", __FUNCTION__, ret);
				continue;
			}

			for(i=0; i<(gint)meminfo.oobsize; i+= 16)
			{
				sprintf(pretty_buf, "  OOB Data: %02x %02x %02x %02x %02x %02x "
								"%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
								_oobbuf[i], _oobbuf[i+1], _oobbuf[i+2],
								_oobbuf[i+3], _oobbuf[i+4], _oobbuf[i+5],
								_oobbuf[i+6], _oobbuf[i+7], _oobbuf[i+8],
								_oobbuf[i+9], _oobbuf[i+10], _oobbuf[i+11],
								_oobbuf[i+12], _oobbuf[i+13], _oobbuf[i+14],
								_oobbuf[i+15]);
				ret = write(ofd, pretty_buf, 60);
				if(ret != 60)
					g_warning("%s Flash Write Error.. ret bytes [%d]", __FUNCTION__, ret);

			}
		} 
		else
		{
			ret = write(ofd, _oobbuf, meminfo.oobsize);
			if(ret != (gint)meminfo.oobsize)
				g_warning("%s Flash Write Error.. ret bytes [%d]", __FUNCTION__, ret);

		}
	}

	/* reset oobinfo */
	if (oobinfochanged == 1) {
		if (ioctl (fd, MEMSETOOBSEL, &old_oobinfo) != 0) {
			g_warning ("%s MEMSETOOBSEL Ioctl Error", __FUNCTION__);
			close(fd);
			close(ofd);
			return 0;
		}
	}
	
	/* Close the output file and MTD device */
	close(fd);
	if(dump_path != NULL)
		close(ofd);
	
	g_message("%s Flash Dump Finish!!!", __FUNCTION__);
	return 1;


closeall:
	/* The new mode change is per file descriptor ! */
	if (oobinfochanged == 1) {
		if (ioctl (fd, MEMSETOOBSEL, &old_oobinfo) != 0)  {
			g_warning("%s MEMSETOOBSEL Ioctl Error", __FUNCTION__);
		}
	}
	close(fd);
	close(ofd);
	return 0;
}

/**
	@brief							flash erase
	@param[in]	mtd_block_num		mtd block number
	@return		gboolean			%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_flash_erase(gint mtd_block_num, cb_flash_fxn_t cb_func, gpointer context)
{
	mtd_info_t meminfo;
	gint fd;
	erase_info_t erase;
	gint isNAND, bbtest=1;
	gint quiet=1;
	gchar mtd_device[FLASH_BUF_MAX_LEN]={0,};
	gint tot_block_cnt=0;
	gboolean cb_func_is_null=FALSE;
	NF_UTIL_FLASH_PROGRESS temp;
	
	memset(&_flash_prog, 0, sizeof(_flash_prog));
	
	if(cb_func == NULL)
		cb_func_is_null = TRUE;
	else
		cb_func_is_null = FALSE;

	g_return_val_if_fail(mtd_block_num < MAX_MTD_PARTION_CNT && mtd_block_num >=0, 0);
	
	sprintf(mtd_device,"/dev/mtd%d", mtd_block_num);

	g_message("%s mtd_device : [%s]", __FUNCTION__, mtd_device);

	if ((fd = open(mtd_device, O_RDWR)) < 0) {
		_flash_prog.state = NF_UTIL_FLASH_PRGT_ERASE_FAIL;
		
		g_warning("%s Flash Device Open Error.. mtd_device [%s]", __FUNCTION__, mtd_device);
		return FALSE;
	}

	if (ioctl(fd, MEMGETINFO, &meminfo) != 0) {
		_flash_prog.state = NF_UTIL_FLASH_PRGT_ERASE_FAIL;
		
		g_warning("%s mtd_device [%s]: unable to get MTD device info", __FUNCTION__, mtd_device);
		return FALSE;
	}

	erase.length = meminfo.erasesize;
	isNAND = meminfo.type == MTD_NANDFLASH ? 1 : 0;

	tot_block_cnt=(gint)(meminfo.size/meminfo.erasesize);
	_flash_prog.tot_block_cnt = (guint)tot_block_cnt;
	
	for (erase.start = 0; erase.start < meminfo.size; 
			erase.start += meminfo.erasesize, _flash_prog.cur_block_cnt++)
	{
		if(_flash_prog.cur_block_cnt == 0)
			_flash_prog.state = NF_UTIL_FLASH_PRGT_ERASE_BEGIN;
		else if(_flash_prog.cur_block_cnt !=0 && _flash_prog.cur_block_cnt <_flash_prog.tot_block_cnt)
			_flash_prog.state = NF_UTIL_FLASH_PRGT_ERASE_RUN;

		if(!cb_func_is_null)
			cb_func(&temp, NULL);
		
		if (bbtest)
		{
			loff_t offset = erase.start;
			gint ret = ioctl(fd, MEMGETBADBLOCK, &offset);

			if (ret > 0)
			{
				if (!quiet)
					g_warning("\n %s function\nSkipping bad block at 0x%08x block num [%d]\n",
						   		__FUNCTION__, erase.start, _flash_prog.cur_block_cnt);
				_flash_prog.bad_block_cnt++;
				continue;
			}
			else if (ret < 0)
			{
				if (errno == EOPNOTSUPP)
				{
					bbtest = 0;
					if (isNAND)
					{
						g_warning("%s mtd_device [%s] : Bad block check not available", __FUNCTION__, mtd_device);
						_flash_prog.state = NF_UTIL_FLASH_PRGT_ERASE_FAIL;
				
						return FALSE;
					}
				}
				else {
					g_warning("%s mtd_device [%s] : MTD get bad block failed", __FUNCTION__, mtd_device);
					_flash_prog.state = NF_UTIL_FLASH_PRGT_ERASE_FAIL;
					
					return FALSE;
				}
			}
		}

		if (!quiet)
		{
			g_print("\rErasing %d Kibyte @ %x -- %2llu %% complete.",
						meminfo.erasesize / 1024, erase.start,
						(unsigned long long)erase.start * 100 / meminfo.size);
		}

		fflush(stdout);

		if (ioctl(fd, MEMERASE, &erase) != 0)
		{
			g_warning("%s mtd_device [%s] : MTD Erase fail", __FUNCTION__, mtd_device);
			continue;
		}
	}

	if(_flash_prog.cur_block_cnt ==_flash_prog.tot_block_cnt)
		_flash_prog.state = NF_UTIL_FLASH_PRGT_ERASE_FINISH;

	if (!quiet)
		g_print("\n");

	//erase finigh... struct init to 0	
	memset(&_flash_prog, 0, sizeof(_flash_prog));

	g_message("%s Erasing Finish", __FUNCTION__);
	
	return TRUE;
}

/**
	@brief							flash write
	@param[in]	mtd_block_num		mtd block number    
	@param[in]	img_path			image path
	@param[in]	autoplace			1 is autoplace ecc, 0 is not
	@param[in]	writeoob			1 is oob write.. 0 is not...
	@return		gboolean			%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_flash_write(gint mtd_block_num, gchar *img_path, gboolean autoplace, gboolean writeoob,
						cb_flash_fxn_t cb_func, gpointer context)
{
	gint cnt, fd, ifd, imglen=0, pagelen, baderaseblock, blockstart=-1;
	struct mtd_info_user meminfo;
	struct mtd_oob_buf oob;
	loff_t offs;
	gint ret, readlen;
	gint oobinfochanged = 0;
	struct nand_oobinfo old_oobinfo;
	gchar mtd_device[FLASH_BUF_MAX_LEN] = {0,};
	gchar img[FLASH_BUF_MAX_LEN] = {0,};
	guint mtdoffset = 0;
	/* options */
	gint quiet = 0;
	gint markbad = 0;
	gint noecc = 0;
	gint pad = 0;
	gint blockalign = 1; /*default to using 16K block size */
	gint tot_block_cnt = 0;
	gint img_tot_block_cnt = 0;
	NF_UTIL_FLASH_PROGRESS temp;
	gint i=0;

	g_return_val_if_fail(mtd_block_num < MAX_MTD_PARTION_CNT && mtd_block_num >=0, 0);
	g_return_val_if_fail(img_path != NULL,0);

	memset(_oobbuf, 0xff, sizeof(_oobbuf));
	memset(&_flash_prog, 0, sizeof(_flash_prog));

	sprintf(mtd_device,"/dev/mtd%d", mtd_block_num);
	g_message("%s mtd_device [%s] img_path [%s]", __FUNCTION__, mtd_device, img_path);

	#if 0
		g_message("autoplace [%d]   writeoob[%d]\n", autoplace, writeoob);
		g_message("mtd_device : %s\n", mtd_device);
		g_message("img : %s\n",img);
	#endif

	/* Open the device */
	if ((fd = open(mtd_device, O_RDWR)) == -1) {
		g_warning("%s Flash Device Open Error.. fd [%d] mtd_device [%s]", __FUNCTION__, fd, mtd_device);
		_flash_prog.state = NF_UTIL_FLASH_PRGT_WRITE_FAIL;
#ifdef DEBUG_NF_FLASH_PROG_CBFUNC
		cb_func(&temp, NULL);
#endif
		return 0;
	}

	/* Fill in MTD device capability structure */
	if (ioctl(fd, MEMGETINFO, &meminfo) != 0) {
		g_warning("%s MEMGETINFO Ioctl Error.. fd[%d]", __FUNCTION__, fd);
		close(fd);
		_flash_prog.state = NF_UTIL_FLASH_PRGT_WRITE_FAIL;
#ifdef DEBUG_NF_FLASH_PROG_CBFUNC
		cb_func(&temp, NULL);
#endif
		return 0;
	}

	/* Make sure device page sizes are valid */
	if (!(meminfo.oobsize == 64	&& meminfo.writesize == 2048)	&&
		!(meminfo.oobsize == 128&& meminfo.writesize == 2048)	&&
		!(meminfo.oobsize == 32	&& meminfo.writesize == 1024)	&&
		!(meminfo.oobsize == 16 && meminfo.writesize == 512 )	&&
		!(meminfo.oobsize == 8 	&& meminfo.writesize == 256 )	&&
		!(meminfo.oobsize == 200&& meminfo.writesize == 4096)) 
	{
		g_warning("%s invalid page size. meminfo.oobsize[%d] meminfo.writesize[%d]",
					__FUNCTION__, meminfo.oobsize, meminfo.writesize);
		close(fd);
		_flash_prog.state = NF_UTIL_FLASH_PRGT_WRITE_FAIL;
#ifdef DEBUG_NF_FLASH_PROG_CBFUNC
		cb_func(&temp, NULL);
#endif
		return 0;
	}

	if (autoplace)
	{
		/* Read the current oob info */
		if (ioctl (fd, MEMGETOOBSEL, &old_oobinfo) != 0) {
			g_warning ("%s MEMGETOOBSEL Ioctl Error.. fd[%d]", __FUNCTION__, fd);
			close (fd);
			_flash_prog.state = NF_UTIL_FLASH_PRGT_WRITE_FAIL;
#ifdef DEBUG_NF_FLASH_PROG_CBFUNC
			cb_func(&temp, NULL);
#endif
			return 0;
		}

		/* autoplace ECC ? */
		if (autoplace && (old_oobinfo.useecc != MTD_NANDECC_AUTOPLACE))
		{
			if (ioctl (fd, MEMSETOOBSEL, &autoplace_oobinfo) != 0) {
				g_warning ("%s MEMSETOOBSEL Ioctl Error.. fd[%d]", __FUNCTION__, fd);
				close (fd);
				_flash_prog.state = NF_UTIL_FLASH_PRGT_WRITE_FAIL;
#ifdef DEBUG_NF_FLASH_PROG_CBFUNC
				cb_func(&temp, NULL);
#endif
				return 0;
			}
			oobinfochanged = 1;
		}
	}

	oob.length = meminfo.oobsize;
	oob.ptr = noecc ? _oobreadbuf : _oobbuf;

	/* Open the input file */
	if ((ifd = open(img_path, O_RDONLY)) == -1) {
		g_warning("%s open input file error.. ifd [%d] img_path [%s]", __FUNCTION__, ifd, img_path);
		
		_flash_prog.state = NF_UTIL_FLASH_PRGT_WRITE_FAIL;
#ifdef DEBUG_NF_FLASH_PROG_CBFUNC
		cb_func(&temp, NULL);
#endif
		goto restoreoob;
	}

	// get image length
	imglen = lseek(ifd, 0, SEEK_END);
	lseek (ifd, 0, SEEK_SET);

	//if writeoob == 1, pagelen is 2112(NAND_LARGE_BLOCK_SIZE_PAGE+NAND_LARGE_BLOCK_SIZE_OOB)
	pagelen = (gint)(meminfo.writesize + ((writeoob == 1) ? meminfo.oobsize : 0));
	
	if(!(imglen %pagelen))
		img_tot_block_cnt = imglen/(gint)meminfo.erasesize;
	else
		img_tot_block_cnt = (imglen/(gint)meminfo.erasesize) + 1;

//	g_print("%s pagelen [%d] imglen [%d]\n", __FUNCTION__, pagelen, imglen);
	// Check, if file is pagealigned
	#if 0
	if ((!pad) && ((imglen % pagelen) != 0)) {
		fprintf (stderr, "Input file is not page aligned\n");
		goto closeall;
	}
	#endif
	
	// Check, if length fits into device
	if( ((guint)(imglen / pagelen) * meminfo.writesize) > (meminfo.size - mtdoffset)) {
		g_warning("%s Image %d bytes, NAND page %d bytes, OOB area %u bytes, device size %u bytes\n",
					__FUNCTION__, imglen, pagelen, meminfo.writesize, meminfo.size);
		g_warning("%s Input file does not fit into device. Input file[%s]", __FUNCTION__, img_path);
		
		_flash_prog.state = NF_UTIL_FLASH_PRGT_WRITE_FAIL;
		goto closeall;
	}

	tot_block_cnt= (gint)(meminfo.size/meminfo.erasesize);
	_flash_prog.tot_block_cnt = (guint)tot_block_cnt;

	g_print("%s img_tot_block_cnt[%d] imglen[%d] tot_block_cnt[%d]\n",
				__FUNCTION__, img_tot_block_cnt, imglen, _flash_prog.tot_block_cnt);
	/* Get data from input and write to the device */
	while (imglen && (mtdoffset < meminfo.size))
	{
		if(mtdoffset ==0)
			_flash_prog.state = NF_UTIL_FLASH_PRGT_WRITE_BEGIN;
		else if(mtdoffset !=0 && (_flash_prog.cur_block_cnt < (guint)img_tot_block_cnt))
			_flash_prog.state = NF_UTIL_FLASH_PRGT_WRITE_RUN;

		// new eraseblock , check for bad block(s)
		// Stay in the loop to be sure if the mtdoffset changes because
		// of a bad block, that the next block that will be written to
		// is also checked. Thus avoiding errors if the block(s) after the
		// skipped block(s) is also bad (number of blocks depending on
		// the blockalign
		while (blockstart != (gint)((mtdoffset & (~meminfo.erasesize + 1))))
		{
			blockstart = (gint)(mtdoffset & (~meminfo.erasesize + 1));
			offs = blockstart;
			baderaseblock = 0;
			
			if (!quiet)
			{
				g_message("%s Writing data to block %x", __FUNCTION__, blockstart);
				_flash_prog.cur_block_cnt++;
			}

			/* Check all the blocks in an erase block for bad blocks */
			do {
				if ((ret = ioctl(fd, MEMGETBADBLOCK, &offs)) < 0)
				{
					g_warning("%s MEMGETBADBLOCK Ioctl Error.. ret [%d]", __FUNCTION__, ret);
					_flash_prog.state = NF_UTIL_FLASH_PRGT_WRITE_FAIL;
					goto closeall;
				}
				
				if (ret == 1)
				{
					baderaseblock = 1;
					if (!quiet)
					{
						g_warning("%s Bad block at %x, %u block(s) ""from %x will be skipped _flash_prog.cur_block_cnt [%d]\n",
						__FUNCTION__, (gint)offs, blockalign, blockstart, _flash_prog.cur_block_cnt);
					}
				}
				/* if bad block , block cnt++ */
				if (baderaseblock) {
					mtdoffset = (guint)blockstart + meminfo.erasesize;
					_flash_prog.bad_block_cnt++;
				}
				offs +=  meminfo.erasesize / (guint)blockalign;
			} while ( offs < (guint)blockstart + meminfo.erasesize );
		}
		
	   	readlen = (gint)meminfo.writesize;
		
		if (pad && (imglen < readlen))
		{
			readlen = imglen;
			memset(_writebuf + readlen, 0xff, meminfo.writesize - (guint)readlen);
		}
		
		/* Read Page Data from input file */
		if ((cnt = read(ifd, _writebuf, (size_t)readlen)) != readlen)
		{
			#if 0
			if (cnt == 0)   // EOF
				break;
			g_warning ("File I/O error on input file");
			goto closeall;
			#endif
		}

		if (writeoob)
		{
			/* Read OOB data from input file, exit on failure */
			if ((cnt = read(ifd, _oobreadbuf, meminfo.oobsize)) != (ssize_t)meminfo.oobsize) {
				g_warning(" %s input file read error.. input file [%s]", __FUNCTION__, img_path);
				_flash_prog.state = NF_UTIL_FLASH_PRGT_WRITE_FAIL;
				goto closeall;
			}
			if (!noecc) {
				gint i, start, len;
				/*
				*  We use autoplacement and have the oobinfo with the autoplacement
				* information from the kernel available
				*
				* Modified to support out of order oobfree segments,
				* such as the layout used by diskonchip.c
				*/
				if (!oobinfochanged && (old_oobinfo.useecc == MTD_NANDECC_AUTOPLACE))
				{
					for (i = 0;old_oobinfo.oobfree[i][1]; i++) {
						/* Set the reserved bytes to 0xff */
						start = (gint)old_oobinfo.oobfree[i][0];
						len = (gint)old_oobinfo.oobfree[i][1];
						memcpy(_oobbuf + start, _oobreadbuf + start, (size_t)len);
					}
				}
				else
				{
					/* Set at least the ecc byte positions to 0xff */
					start = (gint)old_oobinfo.eccbytes;
					len = (gint)((gint)meminfo.oobsize - start);
					memcpy(_oobbuf + start, (_oobreadbuf+start), (size_t)len);
				}
			}
			
			/* Write OOB data first, as ecc will be placed in there*/
			oob.start = mtdoffset;

			if (ioctl(fd, MEMWRITEOOB, &oob) != 0) {
				g_warning(" %s MEMWRITEOOB ioctl Error.. fd [%d]", __FUNCTION__, fd);
				_flash_prog.state = NF_UTIL_FLASH_PRGT_WRITE_FAIL;
				goto closeall;
			}
			
			imglen -= (gint)meminfo.oobsize;
		}

		/* Write out the Page data */
		if ((guint)pwrite(fd, _writebuf, meminfo.writesize, (off_t)mtdoffset) != meminfo.writesize)
		{
			gint rewind_blocks;
			off_t rewind_bytes;
			erase_info_t erase;

			g_warning("%s pwrite error mtdoffset [%d]", __FUNCTION__, mtdoffset);
            
			/* Must rewind to blockstart if we can */
            rewind_blocks = (gint)((mtdoffset - (guint)blockstart) / meminfo.writesize); /* Not including the one we just attempted */
			rewind_bytes = (off_t)((rewind_blocks * (gint)meminfo.writesize) + readlen);
			if (writeoob)
				rewind_bytes += (rewind_blocks + 1) * (off_t)meminfo.oobsize;
			if (lseek(ifd, -rewind_bytes, SEEK_CUR) == -1)
			{
				g_warning("%s lseek error ifd [%d]", __FUNCTION__, ifd);
				g_warning("%s Failed to seek backwards to recover from write error", __FUNCTION__);
				_flash_prog.state = NF_UTIL_FLASH_PRGT_WRITE_FAIL;
				goto closeall;
			}
                    
		 	erase.start = (guint)blockstart;
			erase.length = meminfo.erasesize;

			g_warning("%s Erasing failed write from %08x-%08x\n",
					__FUNCTION__, erase.start, erase.start+erase.length-1);

			if(ioctl(fd, MEMERASE, &erase) != 0) {
				g_warning("%s MEMERASE ioctl error.. fd [%d]", __FUNCTION__, fd);
				_flash_prog.state = NF_UTIL_FLASH_PRGT_WRITE_FAIL;
				goto closeall;
			}
			
			if(markbad) {
				loff_t bad_addr = mtdoffset & (~(meminfo.erasesize / (guint)blockalign) + 1);
				g_message("Marking block at %08lx bad", (glong)bad_addr);

				if (ioctl(fd, MEMSETBADBLOCK, &bad_addr)) {
					g_warning("%s MEMSETBADBLOCK ioctl error.. fd [%d] bad_addr [%08lx]", __FUNCTION__, fd, (glong)bad_addr);
					/* But continue anyway */
				}
			}

			mtdoffset = (guint)blockstart + meminfo.erasesize;
			imglen += rewind_blocks * (gint)meminfo.writesize;

			continue;
		}
		
		imglen -= readlen;
		mtdoffset += meminfo.writesize;
	}

	if(writeoob)
	{
		if(_flash_prog.cur_block_cnt == ((guint)img_tot_block_cnt+_flash_prog.bad_block_cnt))
			_flash_prog.state = NF_UTIL_FLASH_PRGT_WRITE_FINISH;
	}
	else
	{
		if(_flash_prog.cur_block_cnt == (guint)tot_block_cnt)
			_flash_prog.state = NF_UTIL_FLASH_PRGT_WRITE_FINISH;
	}

	memset(&_flash_prog, 0, sizeof(_flash_prog));
	
	g_message("%s finish flash write..", __FUNCTION__);

closeall:
	close(ifd);

restoreoob:
	if(oobinfochanged == 1)
	{
		if(ioctl (fd, MEMSETOOBSEL, &old_oobinfo) != 0)
		{
			g_warning("%s MEMSETOOBSEL ioctl error.. fd [%d]", __FUNCTION__, fd);
			close (fd);
			return 0;
		}
	}
	
	close(fd);
#if 0
	if(imglen > 0)
	{
		g_warning("%s Data was only partially written due to image lengh not fit.. remaning image length [%d]", 
					__FUNCTION__, imglen);
		return TRUE;
	}
#endif
	return 1;
}

//#define DEBUG_FLASH_READ
gboolean nf_flash_read(gint mtd_block_num, guint offs, gint read_oob, guchar* dataBuf, guchar* oobBuf)
{
	gulong ofs, end_addr = 0;
	loff_t blockstart = 1;
	gint ret=0, i=0, fd=0, bs=0, badblock = 0;
	struct mtd_oob_buf oob = {0, 16, _oobbuf};
	mtd_info_t meminfo;
	gint oobinfochanged = 0 ;
	struct nand_oobinfo old_oobinfo;
	struct mtd_ecc_stats stat1, stat2;
	gchar mtddev[FLASH_BUF_MAX_LEN] = {0,};
	
	gint eccstats = 0;
	gint noecc =1;						// don't error correct
	gulong	length =0;					// dump length
	gulong	start_addr = 0;				// start address
	
	g_return_val_if_fail(mtd_block_num < MAX_MTD_PARTION_CNT && mtd_block_num >=0, 0);
	g_return_val_if_fail(dataBuf != NULL, 0);

	if(read_oob)
		g_return_val_if_fail(oobBuf != NULL, 0);

	sprintf(mtddev,"/dev/mtd%d", mtd_block_num);

#ifdef DEBUG_FLASH_READ
	g_message("%s mtddev [%s]", __FUNCTION__, mtddev);
#endif

	/* Open MTD device */
	if ((fd = open(mtddev, O_RDONLY)) == -1) {
		g_warning("%s open flash device error fd[%d] mtddev[%s]", __FUNCTION__, fd, mtddev);
		close(fd);
		return FALSE;
	}
	
	/* Fill in MTD device capability structure */
	if (ioctl(fd, MEMGETINFO, &meminfo) != 0) {
		g_warning("%s MEMGETINFO ioctl error.. fd[%d]", __FUNCTION__, fd);
		close(fd);
		return FALSE;
	}
	
	/* Make sure device page sizes are valid */
	if (!(meminfo.oobsize == 64	&& meminfo.writesize == 2048)	&&
		!(meminfo.oobsize == 128&& meminfo.writesize == 2048)	&&
		!(meminfo.oobsize == 32	&& meminfo.writesize == 1024)	&&
		!(meminfo.oobsize == 16 && meminfo.writesize == 512 )	&&
		!(meminfo.oobsize == 8 	&& meminfo.writesize == 256 )	&&
		!(meminfo.oobsize == 200&& meminfo.writesize == 4096)) 
	{
		g_warning("%s invalid page size. meminfo.oobsize[%d] meminfo.writesize[%d]",
					__FUNCTION__, meminfo.oobsize, meminfo.writesize);
		close(fd);
		return FALSE;
	}
	
	/* Read the real oob length */
	oob.length = meminfo.oobsize;		//64

	//oobinfochanged == 1
	if (noecc)  {
		ret = ioctl(fd, MTDFILEMODE, (void *) MTD_MODE_RAW);	//ret -1
		if (ret == 0) {
			oobinfochanged = 2;
		} 
		else {
			switch (errno) {
				case ENOTTY:
					if (ioctl (fd, MEMGETOOBSEL, &old_oobinfo) != 0) {
						g_warning ("%s MEMGETOOBSEL ioctl error..", __FUNCTION__);
						close (fd);
						return FALSE;
					}
					if (ioctl (fd, MEMSETOOBSEL, &none_oobinfo) != 0) {
						g_warning ("%s MEMSETOOBSEL ioctl error..",__FUNCTION__);
						close (fd);
						return FALSE;
					}
					oobinfochanged = 1;			//here
					break;
				default:
					g_warning ("%s MTDFILEMODE ioctl error..",__FUNCTION__);
					close (fd);
					return FALSE;
			}
		}
	}
   	else {
		/* check if we can read ecc stats */
		if (!ioctl(fd, ECCGETSTATS, &stat1)) {
			eccstats = 1;
			g_warning("%s ECC failed           [%d]", __FUNCTION__, stat1.failed);
			g_warning("%s ECC corrected        [%d]", __FUNCTION__, stat1.corrected);
			g_warning("%s Number of bad blocks [%d]", __FUNCTION__, stat1.badblocks);
			g_warning("%s Number of bbt blocks [%d]", __FUNCTION__, stat1.bbtblocks);
		} else
			g_warning("%s No ECC status information available", __FUNCTION__);
	}

	/* Initialize start/end addresses and block size */
	if (length)
		end_addr = start_addr + length;
	if (!length || end_addr > meminfo.size)
		end_addr = meminfo.size;
	
	if(offs > end_addr)
	{
		g_warning("%s Offset is Wrong.. Offset[0x%08x]", __FUNCTION__, offs);
		close(fd);
		return FALSE;
	}

	bs = (gint)meminfo.writesize;	//2048

	//check page aligne	
	if(offs % (guint)bs)
	{
		g_warning("%s Not Page Aligne.. Offset[0x%08x]", __FUNCTION__, offs);
		close(fd);
		return FALSE;
	}

#ifdef DEBUG_FLASH_READ
	/* Print informative message */
	g_message("%s Block size %u, page size %u, OOB size %u",
				__FUNCTION__, meminfo.erasesize, meminfo.writesize, meminfo.oobsize);
	g_message("%s Reading data at 0x%08x ...", __FUNCTION__, (guint)offs);
#endif	
	
	for (ofs = start_addr; ofs < end_addr ; ofs+=meminfo.erasesize) {
		//bad block check
		if (blockstart != (ofs & (~meminfo.erasesize + 1))) {
			blockstart = ofs & (~meminfo.erasesize + 1);
			
			if ((badblock = ioctl(fd, MEMGETBADBLOCK, &blockstart)) < 0) {
				g_warning("%s MEMGETBADBLOCK ioctl error badblock [%d]", __FUNCTION__, badblock);
				close(fd);
				return FALSE;
			}
		}
		
		if(!(badblock == 1))
		{
			if(offs >= ofs && offs < ofs+meminfo.erasesize)
				break;
		}
		else	//if bad block .. increase offset 
		{
#ifdef DEBUG_FLASH_READ
			g_warning("%s Bad Block is founded..at 0x%lx  offset will be changed [%x] ==>  [%x]", 
						__FUNCTION__, ofs, offs, offs + meminfo.erasesize);
#endif
			offs += meminfo.erasesize;
		}
	}

	memset(_readbuf, 0x0, sizeof(_readbuf));
	if (pread(fd, _readbuf, (size_t)bs, (off_t)offs) != bs)
	{
		g_warning("%s Nand Flash Page Read Error.. Offset[0x%lx]", __FUNCTION__, ofs);
		return FALSE;
	}
	memcpy(dataBuf, _readbuf, sizeof(_readbuf));

	//read oob data
	if(read_oob)
	{
		oob.start = offs;
		if (ioctl(fd, MEMREADOOB, &oob) != 0)
		{
			g_warning("%s MEMREADOOB ioctl error.. ofs[%ld]", __FUNCTION__, ofs);
			close(fd);
			return FALSE;
		}
	}
	if(read_oob)
		memcpy(oobBuf, _oobbuf, sizeof(_oobbuf)); 
#ifdef DEBUG_FLASH_READ
	for (i = 0; i < bs; i += 16) {
		g_print("0x%08x: %02x %02x %02x %02x %02x %02x %02x "
				"%02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
				(guint) (offs + i),  dataBuf[i],
				dataBuf[i+1], dataBuf[i+2],
				dataBuf[i+3], dataBuf[i+4],
				dataBuf[i+5], dataBuf[i+6],
				dataBuf[i+7], dataBuf[i+8],
				dataBuf[i+9], dataBuf[i+10],
				dataBuf[i+11], dataBuf[i+12],
				dataBuf[i+13], dataBuf[i+14],
				dataBuf[i+15]);
			
	}
	
	if(read_oob)
	{
		for(i=0; i<meminfo.oobsize; i+= 16)
		{
			g_print("  OOB Data: %02x %02x %02x %02x %02x %02x "
						"%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
						oobBuf[i], oobBuf[i+1], oobBuf[i+2],
						oobBuf[i+3], oobBuf[i+4], oobBuf[i+5],
						oobBuf[i+6], oobBuf[i+7], oobBuf[i+8],
						oobBuf[i+9], oobBuf[i+10], oobBuf[i+11],
						oobBuf[i+12], oobBuf[i+13], oobBuf[i+14],
						oobBuf[i+15]);
		}
	}
#endif
	if (oobinfochanged == 1) {
		if (ioctl (fd, MEMSETOOBSEL, &old_oobinfo) != 0) {
			g_warning ("%s MEMSETOOBSEL ioctl error", __FUNCTION__);
			close(fd);
			return FALSE;
		}
	}
	
	close(fd);
#ifdef DEBUG_FLASH_READ
	g_printf("%s Read Finish!!!\n", __FUNCTION__);
#endif
	return TRUE;
}

//#define DEBUG_FLASH_READ
/** only fw_upgrade **/
gboolean nf_flash_read_block(gint mtd_block_num, guint offs, guchar* dataBuf)
{
	gulong ofs, end_addr = 0;
	loff_t blockstart = 1;
	gint ret=0, i=0, fd=0, bs=0, badblock = 0;
	struct mtd_oob_buf oob = {0, 16, _oobbuf};
	mtd_info_t meminfo;
	gint oobinfochanged = 0 ;
	struct nand_oobinfo old_oobinfo;
	struct mtd_ecc_stats stat1, stat2;
	gchar mtddev[FLASH_BUF_MAX_LEN] = {0,};
	guchar rdBuf[NAND_ERASE_SIZE]={0, };

	gint eccstats = 0;
	gint noecc =1;                      // don't error correct
	gulong  length =0;                  // dump length
	gulong  start_addr = 0;             // start address
	static guint bad_block_cnt[MAX_MTD_PARTION_CNT] = {0,};          

	g_return_val_if_fail(mtd_block_num < MAX_MTD_PARTION_CNT && mtd_block_num >=0, 0);
	g_return_val_if_fail(dataBuf != NULL, 0);

	sprintf(mtddev,"/dev/mtd%d", mtd_block_num);

#ifdef DEBUG_FLASH_READ
	g_message("%s mtddev [%s]", __FUNCTION__, mtddev);
#endif

	/* Open MTD device */
	if ((fd = open(mtddev, O_RDONLY)) == -1) {
		g_warning("%s open flash device error fd[%d] mtddev[%s]", __FUNCTION__, fd, mtddev);
		close(fd);
		return FALSE;
	}

	/* Fill in MTD device capability structure */
	if (ioctl(fd, MEMGETINFO, &meminfo) != 0) {
		g_warning("%s MEMGETINFO ioctl error.. fd[%d]", __FUNCTION__, fd);
		close(fd);
		return FALSE;
	}

	/* Make sure device page sizes are valid */
	if (!(meminfo.oobsize == 64	&& meminfo.writesize == 2048)	&&
		!(meminfo.oobsize == 128&& meminfo.writesize == 2048)	&&
		!(meminfo.oobsize == 32	&& meminfo.writesize == 1024)	&&
		!(meminfo.oobsize == 16 && meminfo.writesize == 512 )	&&
		!(meminfo.oobsize == 8 	&& meminfo.writesize == 256 )	&&
		!(meminfo.oobsize == 200&& meminfo.writesize == 4096)) 
	{
		g_warning("%s invalid page size. meminfo.oobsize[%d] meminfo.writesize[%d]",
					__FUNCTION__, meminfo.oobsize, meminfo.writesize);
		close(fd);
		return FALSE;
	}

	/* Read the real oob length */
	oob.length = meminfo.oobsize;       //64

	//oobinfochanged == 1
	if (noecc)  {
		ret = ioctl(fd, MTDFILEMODE, (void *) MTD_MODE_RAW);    //ret -1
		if (ret == 0) {
			oobinfochanged = 2;
		}
		else {
			switch (errno) {
				case ENOTTY:
					if (ioctl (fd, MEMGETOOBSEL, &old_oobinfo) != 0) {
						g_warning ("%s MEMGETOOBSEL ioctl error..", __FUNCTION__);
						close (fd);
						return FALSE;
					}
					if (ioctl (fd, MEMSETOOBSEL, &none_oobinfo) != 0) {
						g_warning ("%s MEMSETOOBSEL ioctl error..",__FUNCTION__);
						close (fd);
						return FALSE;
					}
					oobinfochanged = 1;         //here
					break;
				default:
					g_warning ("%s MTDFILEMODE ioctl error..",__FUNCTION__);
					close (fd);
					return FALSE;
			}
		}
	}
	else {
		/* check if we can read ecc stats */
		if (!ioctl(fd, ECCGETSTATS, &stat1)) {
			eccstats = 1;
			g_warning("%s ECC failed           [%d]", __FUNCTION__, stat1.failed);
			g_warning("%s ECC corrected        [%d]", __FUNCTION__, stat1.corrected);
			g_warning("%s Number of bad blocks [%d]", __FUNCTION__, stat1.badblocks);
			g_warning("%s Number of bbt blocks [%d]", __FUNCTION__, stat1.bbtblocks);
		} else
			g_warning("%s No ECC status information available", __FUNCTION__);
	}

	/* Initialize start/end addresses and block size */
	offs += (bad_block_cnt[mtd_block_num]*meminfo.erasesize);
	start_addr = offs;
	if (length)
		end_addr = start_addr + length;
	if (!length || end_addr > meminfo.size)
		end_addr = meminfo.size;
   
	if(offs > end_addr)
	{
		g_warning("%s Offset is Wrong.. Offset[0x%08x]", __FUNCTION__, offs);
		close(fd);
		return FALSE;
	}

	bs = (gint)meminfo.erasesize;   // 0x20000

	//check block aligne 
	if(offs % (guint)bs)
	{
		g_warning("%s Not Block Aligne.. Offset[0x%08x]", __FUNCTION__, offs);
		close(fd);
		return FALSE;
	}

#ifdef DEBUG_FLASH_READ
	/* Print informative message */
	g_message("%s Block size %u, page size %u, OOB size %u",
				__FUNCTION__, meminfo.erasesize, meminfo.writesize, meminfo.oobsize);
	g_message("%s Reading data at 0x%08x ...", __FUNCTION__, (guint)offs);
#endif
	
	for (ofs = start_addr; ofs < end_addr ; ofs+=meminfo.erasesize) {
		//bad block check
		if (blockstart != (ofs & (~meminfo.erasesize + 1))) {
			blockstart = ofs & (~meminfo.erasesize + 1);

			if ((badblock = ioctl(fd, MEMGETBADBLOCK, &blockstart)) < 0) {
				g_warning("%s MEMGETBADBLOCK ioctl error badblock [%d]", __FUNCTION__, badblock);
				close(fd);
				return FALSE;
			}
		}

		if(!(badblock == 1))
		{
			if(offs >= ofs && offs < ofs+meminfo.erasesize)
				break;
		}
		else    //if bad block .. increase offset 
		{
#ifdef DEBUG_FLASH_READ
			g_warning("%s Bad Block is founded..at 0x%lx  offset will be changed [%x] ==>  [%x]",
						__FUNCTION__, ofs, offs, offs + meminfo.erasesize);
#endif
			bad_block_cnt[mtd_block_num]++;	
			offs += meminfo.erasesize;
		}
	}

	memset(rdBuf, 0x0, sizeof(rdBuf));
	if (pread(fd, rdBuf, (size_t)bs, (off_t)offs) != bs)
	{
		g_warning("%s Nand Flash Page Read Error.. Offset[0x%lx]", __FUNCTION__, ofs);
		return FALSE;
	}
	memcpy(dataBuf, rdBuf, sizeof(rdBuf));

	if (oobinfochanged == 1) {
		if (ioctl (fd, MEMSETOOBSEL, &old_oobinfo) != 0) {
			g_warning ("%s MEMSETOOBSEL ioctl error", __FUNCTION__);
			close(fd);
			return FALSE;
		}
	}
   
	close(fd);
#ifdef DEBUG_FLASH_READ
	g_printf("%s Read Finish!!!\n", __FUNCTION__);
#endif
	return TRUE;
}

/**
	@brief							page write in nand flash
	@param[in]	mtd_block_num		mtd block number
	@param[in]	dataBuf				page data buffer
	@param[in]	oobBuf				oob data buffer
	@param[in]	autoplace			1 is autoplace ecc, 0 is not
	@param[in]	writeoob			1 is oob write.. 0 is not...
	@return		gboolean			%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_flash_page_write(gint mtd_block_num, guint offs, guchar *dataBuf, guchar *oobBuf, gboolean autoplace, 
								gboolean writeoob)
{
	gint fd, baderaseblock;
	loff_t blockstart = -1;
	gulong end_addr=0, start_addr = 0, ofs=0;
	gint badblock = 0;
	struct mtd_info_user meminfo;
	struct mtd_oob_buf oob;
	gint ret, readlen;
	gint oobinfochanged = 0;
	struct nand_oobinfo old_oobinfo;
	gchar mtd_device[FLASH_BUF_MAX_LEN] = {0,};
	gchar img[FLASH_BUF_MAX_LEN] = {0,};
	
	gint noecc = 0;

	g_return_val_if_fail(mtd_block_num < MAX_MTD_PARTION_CNT && mtd_block_num >=0, 0);

	memset(_oobbuf, 0xff, sizeof(_oobbuf));
	memset(&_flash_prog, 0, sizeof(_flash_prog));

	sprintf(mtd_device,"/dev/mtd%d", mtd_block_num);
	g_return_val_if_fail(dataBuf != NULL, 0);
	
	if(writeoob)
		g_return_val_if_fail(oobBuf != NULL, 0);

	#if 0
		g_message("autoplace [%d]   writeoob[%d]\n", autoplace, writeoob);
		g_message("mtd_device : %s\n", mtd_device);
	#endif

	/* Open the device */
	if ((fd = open(mtd_device, O_RDWR)) == -1) {
		g_warning("%s open flash error.. fd [%d] mtd_device [%s]", __FUNCTION__, fd, mtd_device);
		return FALSE;
	}

	/* Fill in MTD device capability structure */
	if (ioctl(fd, MEMGETINFO, &meminfo) != 0) {
		g_warning("%s MEMGETINFO ioctl error.. fd[%d]", __FUNCTION__, fd);
		close(fd);
		return FALSE;
	}


	/* Make sure device page sizes are valid */
	if (!(meminfo.oobsize == 64	&& meminfo.writesize == 2048)	&&
		!(meminfo.oobsize == 128&& meminfo.writesize == 2048)	&&
		!(meminfo.oobsize == 32	&& meminfo.writesize == 1024)	&&
		!(meminfo.oobsize == 16 && meminfo.writesize == 512 )	&&
		!(meminfo.oobsize == 8 	&& meminfo.writesize == 256 )	&&
		!(meminfo.oobsize == 200&& meminfo.writesize == 4096)) 
	{
		g_warning("%s invalid page size. meminfo.oobsize[%d] meminfo.writesize[%d]",
					__FUNCTION__, meminfo.oobsize, meminfo.writesize);
		close(fd);
		return FALSE;
	}

	if (autoplace)
	{
		/* Read the current oob info */
		if (ioctl (fd, MEMGETOOBSEL, &old_oobinfo) != 0) {
			g_warning ("%s MEMGETOOBSEL ioctl error.. fd[%d]", __FUNCTION__, fd);
			close (fd);
			return FALSE;
		}

		/* autoplace ECC ? */
		if (autoplace && (old_oobinfo.useecc != MTD_NANDECC_AUTOPLACE))
		{
			if (ioctl (fd, MEMSETOOBSEL, &autoplace_oobinfo) != 0) {
				g_warning ("%s MEMSETOOBSEL ioctl error.. fd[%d]", __FUNCTION__, fd);
				close (fd);
				return FALSE;
			}
			oobinfochanged = 1;
		}
	}
	oob.length = meminfo.oobsize;
	oob.ptr = noecc ? oobBuf : _oobbuf;

	end_addr = meminfo.size;

	if(offs > end_addr)
	{
		g_warning("%s offset is too long.. Offset[0x%08x]", __FUNCTION__, offs);
		return FALSE;
	}

	printf("start_addr [0x%08lx] end_addr [0x%08lx]\n", start_addr, end_addr);

	for (ofs = start_addr; ofs < end_addr ; ofs+=meminfo.erasesize) {
		/* bad block check */
		if (blockstart != (ofs & (~meminfo.erasesize + 1))) {
			blockstart = ofs & (~meminfo.erasesize + 1);

			if ((badblock = ioctl(fd, MEMGETBADBLOCK, &blockstart)) < 0) {
				g_warning("%s MEMGETBADBLOCK Ioctl Error badblock.. Ret[%d]", __FUNCTION__, badblock);
				close(fd);
				return FALSE;
			}
		}

		if(!(badblock == 1))
		{
			if(offs >= ofs && offs < ofs+meminfo.erasesize)
				break;
		}
		else    //if bad block .. increase offset as erase size (0x200000) 
		{
			g_warning("%s Bad Block is founded..at 0x%lx  offset will be changed [%x] ==>  [%x]",
						__FUNCTION__, ofs, offs, offs + meminfo.erasesize);
						offs += meminfo.erasesize;
		}
	}
	
	if(writeoob)
	{
		if (!noecc) {
			gint i=0;
			guint start=0, len=0;

			/*
			*  We use autoplacement and have the oobinfo with the autoplacement
			* information from the kernel available
			*
			* Modified to support out of order oobfree segments,
			* such as the layout used by diskonchip.c
			*/
			if (!oobinfochanged && (old_oobinfo.useecc == MTD_NANDECC_AUTOPLACE))
			{
				for (i = 0;old_oobinfo.oobfree[i][1]; i++) {
					/* Set the reserved bytes to 0xff */
					start = old_oobinfo.oobfree[i][0];
					len = old_oobinfo.oobfree[i][1];
					memcpy(_oobbuf + start, oobBuf + start, (size_t)len);
				}
			}
			else
			{
				/* Set at least the ecc byte positions to 0xff */
				start = old_oobinfo.eccbytes;
				len = meminfo.oobsize - start;
				memcpy(_oobbuf + start, oobBuf + start, (size_t)len);
			}
		}
			
		/* Write OOB data first, as ecc will be placed in there*/
		oob.start = offs;
		if (ioctl(fd, MEMWRITEOOB, &oob) != 0) {
			g_warning(" %s MEMWRITEOOB Ioctl Error.. fd[%d]", __FUNCTION__, fd);
			close (fd);
			return FALSE;
		}
	}
	
	if (pwrite(fd, dataBuf, meminfo.writesize, (off_t)offs) != (ssize_t)meminfo.writesize)
	{
		g_warning("%s pwrite error offset [0x%08x]", __FUNCTION__, offs);
		close (fd);
		return FALSE;
	}

	g_message("%s Finish Flash Write..", __FUNCTION__);

	if(oobinfochanged == 1)
	{
		if(ioctl (fd, MEMSETOOBSEL, &old_oobinfo) != 0)
		{
			g_warning("%s MEMSETOOBSEL ioctl error.. fd [%d]", __FUNCTION__, fd);
			close (fd);
			return FALSE;
		}
	}
	
	close(fd);

	return TRUE;
}

/**
	@brief							flash write for firmware upgrade
	@param[in]	mtd_block_num		mtd block number    
	@param[in]	fp					image data's file pointer
	@param[in]	data_len			image file data length to write
	@param[in]	autoplace			1 is autoplace ecc, 0 is not
	@param[in]	writeoob			1 is oob write.. 0 is not...
	@param[in]	cb_func				call back function
	@param[in]	context				string..
	@return		gboolean			%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_flash_fw_write(gint mtd_block_num, FILE* fp, glong data_len, gboolean autoplace,
							gboolean writeoob, cb_flash_fxn_t cb_func, gpointer context)
{
	gint fd, pagelen, baderaseblock, blockstart = -1;
	glong imglen=0, cur_offset=0, end_addr=0;
	struct mtd_info_user meminfo;
	struct mtd_oob_buf oob;
	loff_t offs;
	gint ret=0, img_block_size=0;
	gint oobinfochanged = 0;
	struct nand_oobinfo old_oobinfo;
	gchar mtd_device[FLASH_BUF_MAX_LEN] = {0,};
	guint mtdoffset=0, readlen=0;
	//options
	gint quiet = 1;
	gint markbad = 0;
	gint noecc = 0;
	gint pad = 0;
	gint blockalign = 1; 				/* default to using 16K block size */
	gint tot_block_cnt = 0;
	gint img_tot_block_cnt = 0;
	gboolean state =FALSE;
	gboolean cb_func_is_null=FALSE;

	NF_UTIL_FLASH_PROGRESS temp;		/* for progress */

	if(cb_func == NULL)
		cb_func_is_null = TRUE;
	else
		cb_func_is_null = FALSE;

	g_return_val_if_fail(mtd_block_num < MAX_MTD_PARTION_CNT && mtd_block_num >=0, 0);

	memset(_oobbuf, 0xff, sizeof(_oobbuf));
	memset(&_flash_prog, 0, sizeof(_flash_prog));

	sprintf(mtd_device,"/dev/mtd%d", mtd_block_num);
	g_message("%s mtd_device [%s]", __FUNCTION__, mtd_device);

	#if 0
		g_message("autoplace [%d]   writeoob[%d]\n", autoplace, writeoob);
		g_message("mtd_device : %s\n", mtd_device);
	#endif

	/* Open the device */
	if ((fd = open(mtd_device, O_RDWR)) == -1) {
		g_warning("%s open flash error.. fd [%d] mtd_device [%s]", __FUNCTION__, fd, mtd_device);
		_flash_prog.state = NF_UTIL_FLASH_PRGT_WRITE_FAIL;

		if(cb_func_is_null != TRUE)
			cb_func(&temp, NULL);
		return 0;
	}

	/* Fill in MTD device capability structure */
	if (ioctl(fd, MEMGETINFO, &meminfo) != 0) {
		g_warning("%s MEMGETINFO ioctl error.. fd[%d]", __FUNCTION__, fd);
		close(fd);
		_flash_prog.state = NF_UTIL_FLASH_PRGT_WRITE_FAIL;
		
		return 0;
	}

	/* Make sure device page sizes are valid */
	if (!(meminfo.oobsize == 64	&& meminfo.writesize == 2048)	&&
		!(meminfo.oobsize == 128&& meminfo.writesize == 2048)	&&
		!(meminfo.oobsize == 32	&& meminfo.writesize == 1024)	&&
		!(meminfo.oobsize == 16 && meminfo.writesize == 512 )	&&
		!(meminfo.oobsize == 8 	&& meminfo.writesize == 256 )	&&
		!(meminfo.oobsize == 200&& meminfo.writesize == 4096)) 
	{
		g_warning("%s invalid page size. meminfo.oobsize[%d] meminfo.writesize[%d]",
					__FUNCTION__, meminfo.oobsize, meminfo.writesize);
		close(fd);
		_flash_prog.state = NF_UTIL_FLASH_PRGT_WRITE_FAIL;
		
		return 0;
	}

	if (autoplace)
	{
		/* Read the current oob info */
		if (ioctl (fd, MEMGETOOBSEL, &old_oobinfo) != 0) {
			g_warning ("%s MEMGETOOBSEL ioctl error.. fd[%d]", __FUNCTION__, fd);
			close (fd);
			_flash_prog.state = NF_UTIL_FLASH_PRGT_WRITE_FAIL;
			
			return 0;
		}

		// autoplace ECC ?
		if (autoplace && (old_oobinfo.useecc != MTD_NANDECC_AUTOPLACE))
		{
			if (ioctl (fd, MEMSETOOBSEL, &autoplace_oobinfo) != 0) {
				g_warning ("%s MEMSETOOBSEL ioctl error.. fd[%d]", __FUNCTION__, fd);
				close (fd);
				_flash_prog.state = NF_UTIL_FLASH_PRGT_WRITE_FAIL;
				
				return 0;
			}
			oobinfochanged = 1;
		}
	}

	oob.length = meminfo.oobsize;
	oob.ptr = noecc ? _oobreadbuf : _oobbuf;
	
	imglen = data_len;
	end_addr = (glong)meminfo.size;
	//if writeoob == 1, pagelen is 2112(NAND_LARGE_BLOCK_SIZE_PAGE+NAND_LARGE_BLOCK_SIZE_OOB)
	pagelen = (gint)(meminfo.writesize + ((writeoob == 1) ? meminfo.oobsize : 0));
	
	if(writeoob)		/* 1block's size is erasesize+(64* oobsize) */
	{
		img_block_size = (gint)(meminfo.erasesize + (meminfo.oobsize*(meminfo.erasesize/meminfo.writesize)));
		
		if(!(imglen % img_block_size))
			img_tot_block_cnt = imglen/img_block_size;
		else
			img_tot_block_cnt = (imglen/img_block_size) + 1;
	}
	else
	{
		img_block_size = (gint)meminfo.erasesize;
	
		if(!(imglen % img_block_size))
			img_tot_block_cnt = imglen/img_block_size;
		else
			img_tot_block_cnt = (imglen/img_block_size) + 1;
	}
	
	/* Check, if length fits into device */
	if( ((imglen / (glong)pagelen) * (glong)meminfo.writesize) > (glong)(meminfo.size - mtdoffset)) {
		g_warning("%s Image %ld bytes, NAND page %d bytes, OOB area %u bytes, device size %u bytes\n",
					__FUNCTION__, imglen, pagelen, meminfo.writesize, meminfo.size);
		g_warning("%s Input file does not fit into device", __FUNCTION__);
		
		_flash_prog.state = NF_UTIL_FLASH_PRGT_WRITE_FAIL;
		
		close(fd);
		return FALSE;
	}

	tot_block_cnt= (gint)(meminfo.size/meminfo.erasesize);
	_flash_prog.tot_block_cnt = (guint)img_tot_block_cnt;

#ifdef DEBUG_NF_FW_WRITE
	g_print("%s img_tot_block_cnt [%d] imglen [%d] tot_block_cnt [%d]\n",
				__FUNCTION__, img_tot_block_cnt, imglen, _flash_prog.tot_block_cnt);
#endif
	cur_offset=ftell(fp);

#ifdef DEBUG_NF_FW_WRITE
	g_message("image len[%d] cur_offset [%d]", imglen, cur_offset);
	g_message("end address [%x]", end_addr);
#endif

	/* Get data from input and write to the device */
	while (imglen && (mtdoffset < meminfo.size))
	{
		if(mtdoffset == 0)
			_flash_prog.state = NF_UTIL_FLASH_PRGT_WRITE_BEGIN;
		else if(mtdoffset !=0 && (_flash_prog.cur_block_cnt < (guint)img_tot_block_cnt))
			_flash_prog.state = NF_UTIL_FLASH_PRGT_WRITE_RUN;

		// new eraseblock , check for bad block(s)
		// Stay in the loop to be sure if the mtdoffset changes because
		// of a bad block, that the next block that will be written to
		// is also checked. Thus avoiding errors if the block(s) after the
		// skipped block(s) is also bad (number of blocks depending on
		// the blockalign
		while ((guint)blockstart != (mtdoffset & (~meminfo.erasesize + 1)))
		{
			blockstart = (gint)(mtdoffset & (~meminfo.erasesize + 1));
			offs = blockstart;
			baderaseblock = 0;
			
			if (!quiet)
				g_message("%s Writing data to block %x", __FUNCTION__, blockstart);

			/* Check all the blocks in an erase block for bad blocks */
			do {
				if ((ret = ioctl(fd, MEMGETBADBLOCK, &offs)) < 0)
				{
					g_warning("%s MEMGETBADBLOCK ioctl error.. ret [%d]", __FUNCTION__, ret);
					_flash_prog.state = NF_UTIL_FLASH_PRGT_WRITE_FAIL;
					close(fd);
					return FALSE;
				}

				if (ret == 1)
				{
					baderaseblock = 1;
					if (!quiet)
					{
						g_warning("%s Bad block at %x, %u block(s) ""from %x will be skipped _flash_prog.cur_block_cnt [%d]\n",
						__FUNCTION__, (gint)offs, blockalign, blockstart, _flash_prog.cur_block_cnt);
					}
				}

				/* if bad block , block cnt++ */
				if (baderaseblock) {
					mtdoffset = (guint)blockstart + meminfo.erasesize;
					_flash_prog.bad_block_cnt++;
				}
				_flash_prog.cur_block_cnt++;
				offs +=  meminfo.erasesize / (guint)blockalign;
			} while ( offs < (guint)blockstart + meminfo.erasesize );
			
			if(cb_func_is_null != TRUE)	
				cb_func(&temp, NULL);
		}
		
	   	readlen = meminfo.writesize;
		
		if (pad && ((guint)imglen < readlen))
		{
			readlen = (guint)imglen;
			memset(_writebuf + readlen, 0xff, (size_t)(meminfo.writesize - readlen));
		}
		
		/* Read Page Data from fp */
		if(readlen > (guint)imglen)
		{
			memset(_writebuf, 0xff, sizeof(_writebuf));
			g_message("%s Page not aligned.. Remaning Len[%ld]", __FUNCTION__, imglen);
			if (fread(_writebuf, (size_t)imglen, 1, fp) != 1)
			{
				g_warning ("%s Reamaining Length fread() Error!!! Remaining Len[%ld]", __FUNCTION__, imglen);
				close(fd);
				return FALSE;
			}
			
			state = TRUE;
		}
		else
		{
			if (fread(_writebuf, (size_t)readlen, 1, fp) != 1)
			{
				gulong offset=(gulong)ftell(fp);
				g_warning ("%s File Read Error!!! Remaining Len[%ld] FP offset[%lx]", __FUNCTION__, imglen, offset);
				close(fd);
				return FALSE;
			}
		}
		
		if (writeoob)
		{
			/* Read OOB data from input file, exit on failure */
			if((fread(_oobreadbuf, meminfo.oobsize, 1, fp)) != 1)
			{
				g_warning("%s File OOB Read Error!!!", __FUNCTION__);
				_flash_prog.state = NF_UTIL_FLASH_PRGT_WRITE_FAIL;
				close(fd);
				return FALSE;
			}

			if (!noecc) {
				gint i=0;
				guint start=0, len=0;
				/*
				*  We use autoplacement and have the oobinfo with the autoplacement
				* information from the kernel available
				*
				* Modified to support out of order oobfree segments,
				* such as the layout used by diskonchip.c
				*/
				if (!oobinfochanged && (old_oobinfo.useecc == MTD_NANDECC_AUTOPLACE))
				{
					for (i = 0;old_oobinfo.oobfree[i][1]; i++) {
						/* Set the reserved bytes to 0xff */
						start = old_oobinfo.oobfree[i][0];
						len = old_oobinfo.oobfree[i][1];
						memcpy(_oobbuf + start, _oobreadbuf + start, len);
					}
				}
				else
				{
					/* Set at least the ecc byte positions to 0xff */
					start = old_oobinfo.eccbytes;
					len = meminfo.oobsize - start;
					memcpy(_oobbuf + start, _oobreadbuf + start, len);
				}
			}

			/* Write OOB data first, as ecc will be placed in there*/
			oob.start = mtdoffset;

			if (ioctl(fd, MEMWRITEOOB, &oob) != 0) {
				g_warning("%s MEMWRITEOOB ioctl error.. fd [%d]", __FUNCTION__, fd);
				_flash_prog.state = NF_UTIL_FLASH_PRGT_WRITE_FAIL;
				close(fd);
				return FALSE;
			}

			imglen -= (glong)meminfo.oobsize;
		}

		/* Write out the Page data */
		if ((guint)pwrite(fd, _writebuf, meminfo.writesize, (off_t)mtdoffset) != meminfo.writesize)
		{
			g_warning("%s Page Data Write Error!!! fd[%d] Current Offset [0x%08x] Remain Len[0x%08lx]",
								__FUNCTION__, fd, mtdoffset, imglen);
			close(fd);
			return FALSE;
		}

		if(state == TRUE)
		{
			g_message("%s Remaning Len wrote..", __FUNCTION__);
			break;
		}

		imglen -= (glong)readlen;
		mtdoffset += meminfo.writesize;
	}

	if(writeoob)
	{
		if(_flash_prog.cur_block_cnt == (guint)img_tot_block_cnt+_flash_prog.bad_block_cnt)
			_flash_prog.state = NF_UTIL_FLASH_PRGT_WRITE_FINISH;
	}
	else
	{
		if(_flash_prog.cur_block_cnt == (guint)tot_block_cnt)
			_flash_prog.state = NF_UTIL_FLASH_PRGT_WRITE_FINISH;
	}

	memset(&_flash_prog, 0, sizeof(_flash_prog));
	
	if(oobinfochanged == 1)
	{
		if(ioctl (fd, MEMSETOOBSEL, &old_oobinfo) != 0)
		{
			g_warning("%s MEMSETOOBSEL Ioctl error.. fd [%d]", __FUNCTION__, fd);
			close (fd);
			return FALSE;
		}
	}

	g_message("%s Finish Flash Write..", __FUNCTION__);
	
	close(fd);

	return TRUE;
}

gboolean nf_flash_fw_yaffs_write(gint mtd_block_num, FILE* fp, glong data_len, gboolean autoplace,
							gboolean writeoob, cb_flash_fxn_t cb_func, gpointer context)
{
	struct mtd_info_user meminfo;
	struct mtd_oob_buf oob;
	gchar mtd_device[FLASH_BUF_MAX_LEN] = {0,};
	size_t left_to_write=0, offset=0;
	u_char Buf[NAND_LARGE_BLOCK_SIZE_PAGE+NAND_LARGE_BLOCK_SIZE_OOB]={0,};
	gint fd=0, ret=0, cnt=0, tot_block_cnt=0, tot_page_size=0;
	gboolean cb_func_is_null=FALSE;
	NF_UTIL_FLASH_PROGRESS temp;		/* for progress */

	sprintf(mtd_device,"/dev/mtd%d", mtd_block_num);
	g_message("%s mtd_device[%s] autoplace[%d]  writeoob[%d] datalen[%ld]\n", __FUNCTION__, mtd_device, autoplace, writeoob, data_len);

	memset(&_flash_prog, 0, sizeof(_flash_prog));

	if(cb_func == NULL)
		cb_func_is_null = TRUE;
	else
		cb_func_is_null = FALSE;

	/* Open the device */
	if ((fd = open(mtd_device, O_RDWR)) == -1) {
		g_warning("%s Open Flash Device Error.. fd [%d] mtd_device [%s]", __FUNCTION__, fd, mtd_device);
		_flash_prog.state = NF_UTIL_FLASH_PRGT_WRITE_FAIL;

		if(!cb_func_is_null)
			cb_func(&temp, NULL);
		return FALSE;
	}

	/* Fill in MTD device capability structure */
	if (ioctl(fd, MEMGETINFO, &meminfo) != 0) {
		g_warning("%s MEMGETINFO Ioctl Error.. fd[%d]", __FUNCTION__, fd);
		close(fd);
		_flash_prog.state = NF_UTIL_FLASH_PRGT_WRITE_FAIL;
		
		return FALSE;
	}

	/* Make sure device page sizes are valid */
	if (!(meminfo.oobsize == 64	&& meminfo.writesize == 2048)	&&
		!(meminfo.oobsize == 128&& meminfo.writesize == 2048)	&&
		!(meminfo.oobsize == 32	&& meminfo.writesize == 1024)	&&
		!(meminfo.oobsize == 16 && meminfo.writesize == 512 )	&&
		!(meminfo.oobsize == 8 	&& meminfo.writesize == 256 )	&&
		!(meminfo.oobsize == 200&& meminfo.writesize == 4096)) 
	{
		g_warning("%s Invalid Page Size OobSize[%d] WriteSize[%d]",
					__FUNCTION__, meminfo.oobsize, meminfo.writesize);
		close(fd);
		return FALSE;
	}

	tot_page_size = (gint)(((guint)data_len / (meminfo.writesize + meminfo.oobsize)) * meminfo.writesize);
	
	if(((guint)tot_page_size % meminfo.erasesize) == 0)
		tot_block_cnt = (gint)((guint)tot_page_size / meminfo.erasesize);
	else
		tot_block_cnt = (gint)(((guint)tot_page_size / meminfo.erasesize)+1);

	_flash_prog.tot_block_cnt = (guint)tot_block_cnt;

	offset=0x0;
	g_message("%s Nand Infomation : witesize[%d] oobsize[%d] erasesize[0x%08x] meminfo.size[%d]\n"
				"            Image Infomation : data_len[%ld] tot_page_size[%d] tot_block[%d]",
						__FUNCTION__, meminfo.writesize, meminfo.oobsize, meminfo.erasesize, meminfo.size,
						data_len, tot_page_size, ((guint)tot_page_size / meminfo.erasesize));

	oob.length = meminfo.writesize+meminfo.oobsize;
	oob.ptr=Buf;
	
	g_message("F/S Yaffs Write Funcion Start!!!!!!!!!\n");
	
	do {
		if(offset == 0)
			_flash_prog.state = NF_UTIL_FLASH_PRGT_WRITE_BEGIN;
		else
			_flash_prog.state = NF_UTIL_FLASH_PRGT_WRITE_RUN;

		if((offset % (meminfo.erasesize)) == 0)
		{
			while(1)
			{
				loff_t blockstart = offset & (~meminfo.erasesize + 1);

				// when ret is 1, bad block..  when ret is 0, not bad block
				if((ret = ioctl(fd, MEMGETBADBLOCK, &blockstart)) < 0)
				{
					g_warning("%s MEMGETBADBLOCK Ioctl Error.. ret [%d] fd[%d] offset[0x%08x]", __FUNCTION__, ret, fd, offset);
					_flash_prog.state = NF_UTIL_FLASH_PRGT_WRITE_FAIL;
					offset+=meminfo.writesize;
					close(fd);
					return FALSE;
				}
				
				if(ret == 1)
				{
					g_warning("%s Bad Block Detected!! Offset[0x%08x]", __FUNCTION__, offset);
					offset+=meminfo.erasesize;
					_flash_prog.bad_block_cnt++;
				}
				else
				{
					_flash_prog.cur_block_cnt++;
	
					if(!cb_func_is_null)
						cb_func(&temp, NULL);
					break;
				}
			}
		}

		/* Read Page data and OOB data from input file */
		if((fread(Buf, (meminfo.writesize+meminfo.oobsize), 1, fp)) != 1)
		{
			g_warning("%s Data+OOB Read Fail!!!", __FUNCTION__);
			_flash_prog.state = NF_UTIL_FLASH_PRGT_WRITE_FAIL;
			close(fd);
			return FALSE;
		}

		oob.start = offset;
		#if 0
			ret=ioctl(fd, MEMWRITEOOB, &oob);
		#else
			ret=ioctl(fd, MEMWRITEOOB_FS_YAFFS, &oob);
		#endif
		if (ret != 0) {
			g_warning("%s MEMWRITEOOB_FS_YAFFS Ioctl Error.. fd [%d] offset[0x%08x] data_len[%ld] ret[%d]",
							__FUNCTION__, fd, offset, data_len, ret);
			_flash_prog.state = NF_UTIL_FLASH_PRGT_WRITE_FAIL;
			close(fd);
			return FALSE;
		}

		if((offset % NF_FLASH_SIZE_1M) == 0)
		{
			cnt++;
			g_message("%s %dM Write!!", __FUNCTION__, cnt);
		}

		data_len -= (glong)(meminfo.writesize+meminfo.oobsize);
		offset += meminfo.writesize;
	} while(data_len > 0);
	
	
	g_message("F/S Yaffs Format Write Finish!!");

	_flash_prog.state = NF_UTIL_FLASH_PRGT_WRITE_FINISH;

	close(fd);

	return TRUE;
}

gboolean nf_flash_get_progress(NF_UTIL_FLASH_PROGRESS *data)
{
	gboolean ret=TRUE;

    g_return_val_if_fail(data != NULL, 0);
    
	memcpy( data, &_flash_prog, sizeof(NF_UTIL_FLASH_PROGRESS));
	
	g_return_val_if_fail(data->state != NF_UTIL_FLASH_PRGT_NONE , 0);

	return TRUE;
}

void test_prog_print(void)
{
	//print NF_UTIL_FLASH_PROGRESS struct member
	g_message("_flash_prog.state         [%d]", _flash_prog.state);
	g_message("_flash_prog.cur_block_cnt [%d]", _flash_prog.cur_block_cnt);
	g_message("_flash_prog.tot_block_cnt [%d]", _flash_prog.tot_block_cnt);
	g_message("_flash_prog.bad_block_cnt [%d]", _flash_prog.bad_block_cnt);
} 

void prog_cb_func(NF_UTIL_FLASH_PROGRESS *data, gpointer context)
{
//	g_message("%s called", __FUNCTION__);
	memcpy( data, &_flash_prog, sizeof(NF_UTIL_FLASH_PROGRESS));
	nf_fw_cb_func(data, NULL);
}

static void flash_test_cb(NF_UTIL_FLASH_PROGRESS *data, gpointer context)
{
	memcpy(data, &_flash_prog, sizeof(_flash_prog));
	g_message("state         [%d]", data->state);
	g_message("cur_block_cnt [%d]", data->cur_block_cnt);
	g_message("tot_block_cnt [%d]", data->tot_block_cnt);
	g_message("bad_block_cnt [%d]", data->bad_block_cnt);
} 

/**
	@brief					for jbshell command
							flash_dump, flash_erase, flash_write, flash_read, flash_page_write
*/
#ifdef DEBUG_JBSHELL_FLASH
static char flash_dump_help [] = "flash dump  [mtd block num] [dump path]";
static int flash_dump(int argc, char **argv)
{
	gint mtd_block_num;	

	if ( argc < 2 ) {
    	printf("Invalid arguments\n%s\n", flash_dump_help);
      	return -1;
    }

	mtd_block_num = strtol(argv[1] , NULL, 0);

	if(argc == 2)
		nf_flash_dump(mtd_block_num, NULL);
	else
		nf_flash_dump(mtd_block_num, argv[2]);

	return 0;
}
__commandlist(flash_dump, "flash_dump", "flash dump  [mtd block num] [dump path]",
				flash_dump_help);

static char flash_erase_help [] = "flash_erase [mtd block num] [cb_func] [data]";
static int flash_erase(int argc, char **argv)
{
	gint mtd_block_num;
		
	if ( argc < 2 ) {
    	printf("Invalid arguments\n%s\n", flash_erase_help);
      	return -1;
    }

	mtd_block_num = strtol(argv[1] , NULL, 0);
	
	nf_flash_erase(mtd_block_num, flash_test_cb, NULL);
	
	return 0;
}
__commandlist(flash_erase, "flash_erase", "flash erase [mtd block num]",
				flash_erase_help);

static char flash_write_help [] = "\nflash_write [mtd block num] [image path] [autoplace] [write oob]";
static int flash_write(int argc, char **argv)
{
	gint mtd_block_num, autoplace, writeoob;	
	
	if ( argc < 5 ) {
    	printf("Invalid arguments\n%s\n", flash_write_help);
      	return -1;
    }

	mtd_block_num = strtol(argv[1] , NULL, 0);
	autoplace = strtol(argv[3] , NULL, 0);
	writeoob = strtol(argv[4] , NULL, 0);
	
//	nf_flash_write(mtd_block_num, flash_test_cb, autoplace, writeoob, NULL, NULL);
	nf_flash_write(mtd_block_num, argv[2], autoplace, writeoob, NULL, NULL);

	return 0;
}
__commandlist(flash_write, "flash_write", "flash write [mtd block num] [image path] [autoplace] [write oob]",
	   			flash_write_help);

static char flash_read_help [] = "flash dump  [mtd block num] [offset] [read oob] [offset_cnt:1] [cnt:1]";
static int flash_read(int argc, char **argv)
{
	gint mtd_block_num=0, readoob=0;
	gint cnt=1, offset_cnt=1;
	guint offset=0;

//	guchar dataBuf[2][NAND_LARGE_BLOCK_SIZE_PAGE]={{0,0},};
	guchar oobBuf[2][NAND_LARGE_BLOCK_SIZE_OOB]={{0,0},};
	
	guchar dataBuf[NAND_LARGE_BLOCK_SIZE_PAGE]= {0, };
	gint i,j;

	if ( argc < 4 ) {
    	printf("Invalid arguments\n%s\n", flash_read_help);
      	return -1;
    }

	mtd_block_num = strtol(argv[1] , NULL, 0);
	offset = (guint)strtoul(argv[2], NULL, 0);
	readoob = (gint)strtoul(argv[3], NULL, 0);
#if 0
	if(argc>4) offset_cnt = strtoul(argv[4], NULL, 0);
	if(argc>5) cnt = strtoul(argv[5], NULL, 0);

	for(j=0; j<offset_cnt; j++)
	{	
		offset += NAND_LARGE_BLOCK_SIZE_PAGE;
		for(i=0; i<cnt; i++)
		{
			memset(dataBuf,0x00,sizeof(dataBuf));
			memset(dataBuf,0x00,sizeof(dataBuf));

			g_print("offset[0x%08x] i[%d] ====\n", offset, i);

			nf_flash_read(mtd_block_num, offset, readoob, dataBuf[0], oobBuf[0]);
			nf_flash_read(mtd_block_num, offset, readoob, dataBuf[1], oobBuf[1]);

			if( memcmp( dataBuf[0], dataBuf[1], NAND_LARGE_BLOCK_SIZE_PAGE) )
			{
				g_print("EEEEEEE offset[0x%08x] i[%d] ====\n", offset, i);
				flash_print_cmp(dataBuf[0], dataBuf[1], 0);					
			}
				
		}
	}
#endif
	g_print("mtd num [%d] offset [%08x] read oob [%d]\n", mtd_block_num, offset, readoob);

	nf_flash_read(mtd_block_num, offset, readoob, dataBuf, NULL);

#if 1
	for (i = 0; i < NAND_LARGE_BLOCK_SIZE_PAGE; i += 16) {
		g_print("0x%08x: %02x %02x %02x %02x %02x %02x %02x "
				"%02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
				(offset +(guint)i),  dataBuf[i],
				dataBuf[i+1], dataBuf[i+2],
				dataBuf[i+3], dataBuf[i+4],
				dataBuf[i+5], dataBuf[i+6],
				dataBuf[i+7], dataBuf[i+8],
				dataBuf[i+9], dataBuf[i+10],
				dataBuf[i+11], dataBuf[i+12],
				dataBuf[i+13], dataBuf[i+14],
				dataBuf[i+15]);
			
	}
	
	if(readoob)
	{
		g_print("============================================================\n");
		for(i=0; i<NAND_LARGE_BLOCK_SIZE_OOB; i+= 16)
		{
			g_print("  OOB Data: %02x %02x %02x %02x %02x %02x "
						"%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
						oobBuf[0][i], oobBuf[0][i+1], oobBuf[0][i+2],
						oobBuf[0][i+3], oobBuf[0][i+4], oobBuf[0][i+5],
						oobBuf[0][i+6], oobBuf[0][i+7], oobBuf[0][i+8],
						oobBuf[0][i+9], oobBuf[0][i+10], oobBuf[0][i+11],
						oobBuf[0][i+12], oobBuf[0][i+13], oobBuf[0][i+14],
						oobBuf[0][i+15]);
		}
	}
#endif
	return 0;
}
__commandlist(flash_read, "flash_read", "flash read  [mtd block num] [offset] [read oob] [offset_cnt:1] [cnt:1]",
				flash_read_help);

static char flash_page_write_help [] = "\nflash_page_write [write mtd block num] [read mtd block num] [offset] [autoplace] [write oob]";
static int flash_page_write(int argc, char **argv) 
{
	gint write_mtd_block_num, read_mtd_block_num, autoplace, writeoob;
	guint offset;
	guchar dataBuf[NAND_LARGE_BLOCK_SIZE_PAGE]={0,};
	guchar oobBuf[NAND_LARGE_BLOCK_SIZE_OOB]={0,};
	guchar temp_data[NAND_LARGE_BLOCK_SIZE_PAGE]={0,};
	guchar temp_oob[NAND_LARGE_BLOCK_SIZE_OOB]={0,};
	gint ret;

	if ( argc < 6 ) {
		g_warning("%s Invalid arguments\n%s", __FUNCTION__, flash_page_write_help);
		return -1;
	}
 
	write_mtd_block_num = strtol(argv[1] , NULL, 0);
	read_mtd_block_num = strtol(argv[2] , NULL, 0);

	offset = (guint)strtol(argv[3] , NULL, 0);
	autoplace = strtol(argv[4] , NULL, 0);
	writeoob = strtol(argv[5] , NULL, 0);
#if 0	
	g_message("~~~~~~~~~~~~~~~~~~~~~~~~~ data read");
	nf_flash_read(read_mtd_block_num, offset, 1, dataBuf, oobBuf);		//read oob is 1...(my configuration..)
	g_message("~~~~~~~~~~~~~~~~~~~~~~~~~ data write");
	nf_flash_page_write(write_mtd_block_num, offset, dataBuf, oobBuf, autoplace, writeoob);

	g_message("~~~~~~~~~~~~~~~~~~~~~~~~~ data compare");
	nf_flash_read(write_mtd_block_num, offset, 1, temp_data, temp_oob);		//read oob is 1...(my configuration..)

	ret = memcmp(temp_data, dataBuf, sizeof(dataBuf));
	if(!ret)
		g_message("%s page data written is same.... ret [%d]", __FUNCTION__, ret);
	else
		g_warning("%s page data written is not same.... ret [%d]", __FUNCTION__, ret);
	
	ret = memcmp(temp_oob, oobBuf, sizeof(oobBuf));
	if(!ret)
		g_message("%s oob data written is same.... ret [%d]", __FUNCTION__, ret);
	else
		g_warning("%s oob data written is not same.... ret [%d]", __FUNCTION__, ret);
#endif

	{
		gint i;
		FILE* fp = NULL;
		guchar d_Buf[NAND_LARGE_BLOCK_SIZE_PAGE]={0,};
		guchar o_Buf[NAND_LARGE_BLOCK_SIZE_OOB]={0,};
		gchar *filename = "/20110.1.1.100.img";
		struct stat f_stat;
		guint offset=0, imglen=0;
		gint autoplace=1, writeoob=1;

		if(stat(filename, &f_stat) == -1)
		{
			g_warning("%s Cannot find File Name [%s]", __FUNCTION__, filename);
			return FALSE;
		}

		fp = fopen(filename, "r");
		if(!fp)
		{
			g_warning("%s file open error!!!", __FUNCTION__);
			return 0;
		}

		imglen = (guint)f_stat.st_size;
		g_message("file length [%d]\n", imglen);
		while(imglen > 0)
		{	
			memset(d_Buf, 0x0, NAND_LARGE_BLOCK_SIZE_PAGE);
			memset(o_Buf, 0x0, NAND_LARGE_BLOCK_SIZE_OOB);
#if 1	
			if(fread(d_Buf, NAND_LARGE_BLOCK_SIZE_PAGE, 1, fp) != 1)
			{
				g_warning("%s data buf read error!!!", __FUNCTION__);
				fclose(fp);
				return -1;
			}

			if(fread(o_Buf, NAND_LARGE_BLOCK_SIZE_OOB, 1, fp) != 1)
			{
				g_warning("%s oob buf read error!!!", __FUNCTION__);
				fclose(fp);
				return -1;
			}
			nf_flash_page_write(9, offset, d_Buf, o_Buf, autoplace, writeoob);
#endif		
			imglen -= (NAND_LARGE_BLOCK_SIZE_PAGE+NAND_LARGE_BLOCK_SIZE_OOB);
			offset += 0x800;
//			g_print("imglen [%d]\n", imglen);
		}
		fclose(fp);
	}
	return 0;
}
__commandlist(flash_page_write, "flash_page_write", "flash page write [write mtd block num] [read mtd block num] [offset] [autoplace] [write oob]", flash_page_write_help);

static char flash_partion_erase_help [] = "\nflash_partion_erase";
static int flash_partion_erase(int argc, char **argv)
{
	gint mtd_num[10] = {4, 5, 7, 8, 9, 14, 15, 17, 18 ,19};
	gint i=0;

	if ( argc < 0 ) {
		g_print("Invalid arguments\n%s\n", flash_partion_erase_help);
		return -1;
	}

	for(i=0; i<10; i++)
	{	
		if(nf_flash_erase(mtd_num[i], NULL, NULL) == FALSE)
		{
			g_warning("%s flash erase fail!!!", __FUNCTION__);
			return FALSE;
		}	
	}

	return TRUE;
}__commandlist(flash_partion_erase, "flash_partion_erase", "flash_partion_erase",
	   								flash_partion_erase_help);


static char flash_bad_check_help [] = "\nflash_bad_check [0]  --> bad block check entire\n"
										"flash_bad_check [1] [offset]";
static int flash_bad_check(int argc, char **argv)
{
	gint fd=0, mtd_num=0, i=0, cnt=0, ret=0, blockstart=-1, offs=0;
	guint offset=0;
	gchar mtddev[FLASH_BUF_MAX_LEN] = {0,};
	struct mtd_info_user meminfo;

	if(argc < 3) {
		g_print("Invalid arguments\n%s\n", flash_partion_erase_help);
		return -1;
	}
	
	mtd_num = strtol(argv[1] , NULL, 10);
	offset = (guint)strtol(argv[2] , NULL, 16);
	cnt = strtol(argv[3] , NULL, 10);

	g_message("mtd_num[%d] offset[0x%08x]\n", mtd_num, offset);

	sprintf(mtddev,"/dev/mtd%d", mtd_num);

	if ((fd = open(mtddev, O_RDWR)) == -1) {
		g_warning("%s open flash device error fd[%d] mtddev[%s]", __FUNCTION__, fd, mtddev);
		return 0;
	}

	/* Fill in MTD device capability structure */
	if (ioctl(fd, MEMGETINFO, &meminfo) != 0) {
		g_warning("%s MEMGETINFO Ioctl Error.. fd[%d]", __FUNCTION__, fd);
		close(fd);
		return 0;
	}
	
	g_print("size[%08x] writesize[0x%08x] erasesize[0x%08x]\n",
				meminfo.size, meminfo.writesize, meminfo.erasesize);

	for(i=0; i<cnt; i++)
	{
		loff_t blockstart = offset & (~meminfo.erasesize + 1);

		if ((ret = ioctl(fd, MEMGETBADBLOCK, &blockstart)) < 0)
		{
			g_warning("%s MEMGETBADBLOCK Ioctl Error.. ret [%d] offset[0x%08x] offs[0x%08x]", __FUNCTION__, ret, offset, offs);
			close(fd);
			return FALSE;
		}

		g_message("offset[0x%08x] ret[%d]\n", offset, ret);
		
		offset+=meminfo.erasesize;
	}
	
	close(fd);	

	return TRUE;
}__commandlist(flash_bad_check, "flash_bad_check", flash_bad_check_help, flash_bad_check_help);

#endif

//just test function
void write_page_test(void)
{
	gint fd=0, pageCnt=0;
	guint imglen=0, offset = 0;
	guchar dataBuf[NAND_LARGE_BLOCK_SIZE_PAGE] ={0, };
	guchar oobBuf[NAND_LARGE_BLOCK_SIZE_OOB] ={0,};
	gchar imgPath[] ="/filesys_tinyx.img";
	guint pagelen=NAND_LARGE_BLOCK_SIZE_PAGE, ooblen=NAND_LARGE_BLOCK_SIZE_OOB, cnt=0;

	if ((fd = open(imgPath, O_RDONLY)) == -1)
		g_warning("%s open input file error.. fd [%d] Image Path [%s]", __FUNCTION__, fd, imgPath);
	
	// get image length
	imglen = (guint)lseek(fd, 0, SEEK_END);
	lseek (fd, 0, SEEK_SET);
	pageCnt = (gint)(imglen / (NAND_LARGE_BLOCK_SIZE_PAGE+NAND_LARGE_BLOCK_SIZE_OOB));
	g_message("%s imglen [%d] page cnt [%d]", __FUNCTION__, imglen, pageCnt);

	while(imglen)
	{
		if ((cnt = (guint)read(fd, dataBuf, pagelen)) != pagelen)
			g_warning("%s read page data failed", __FUNCTION__);
		if ((cnt = (guint)read(fd, oobBuf, ooblen)) != ooblen)
			g_warning("%s read oob data failed", __FUNCTION__);

//		flash_print(dataBuf, oobBuf, 1);
		nf_flash_page_write(5, offset, dataBuf, oobBuf, 1, 1);

		offset += NAND_LARGE_BLOCK_SIZE_PAGE;
		imglen -= (NAND_LARGE_BLOCK_SIZE_PAGE+NAND_LARGE_BLOCK_SIZE_OOB);
	}
	g_message("flash write finish");
}

void flash_print_cmp(guchar *dataBuf, guchar *dataBuf2, gint force_print)
{
	gint offset=0, i;
	for (i = 0; i < NAND_LARGE_BLOCK_SIZE_PAGE; i += 16) {
		if( force_print || memcmp( &dataBuf[i], &dataBuf2[i], 16) )		
			g_print("0x%08x: %02x %02x %02x %02x %02x %02x %02x "
				"%02x %02x %02x %02x %02x %02x %02x %02x %02x"
				" <==> %02x %02x %02x %02x %02x %02x %02x "
				"%02x %02x %02x %02x %02x %02x %02x %02x %02x\n",				
				(guint) (offset + i),  dataBuf[i],
				dataBuf[i+1], dataBuf[i+2],
				dataBuf[i+3], dataBuf[i+4],
				dataBuf[i+5], dataBuf[i+6],
				dataBuf[i+7], dataBuf[i+8],
				dataBuf[i+9], dataBuf[i+10],
				dataBuf[i+11], dataBuf[i+12],
				dataBuf[i+13], dataBuf[i+14],
				dataBuf[i+15], 
				dataBuf2[i],
				dataBuf2[i+1], dataBuf2[i+2],
				dataBuf2[i+3], dataBuf2[i+4],
				dataBuf2[i+5], dataBuf2[i+6],
				dataBuf2[i+7], dataBuf2[i+8],
				dataBuf2[i+9], dataBuf2[i+10],
				dataBuf2[i+11], dataBuf2[i+12],
				dataBuf2[i+13], dataBuf2[i+14],
				dataBuf2[i+15]		
				);
	}
	g_print("============================================================\n");
}

// for flash_read().. flash print
void flash_print(guchar *dataBuf , guchar *oobBuf, gint readoob)
{
	g_print("============================================================\n"); 
	gint offset=0, i;
	for (i = 0; i < NAND_LARGE_BLOCK_SIZE_PAGE; i += 16) {                                               
		g_print("0x%08x: %02x %02x %02x %02x %02x %02x %02x "                      
				"%02x %02x %02x %02x %02x %02x %02x %02x %02x\n",                  
				(guint) (offset + i),  dataBuf[i],                                 
				dataBuf[i+1], dataBuf[i+2],                                        
				dataBuf[i+3], dataBuf[i+4],                                        
				dataBuf[i+5], dataBuf[i+6],                                        
				dataBuf[i+7], dataBuf[i+8],                                        
				dataBuf[i+9], dataBuf[i+10],                                       
				dataBuf[i+11], dataBuf[i+12],                                      
				dataBuf[i+13], dataBuf[i+14],                                      
				dataBuf[i+15]);                                                    
	}                                                                              

	g_print("============================================================\n"); 

	if(readoob)
	{
		for(i=0; i<NAND_LARGE_BLOCK_SIZE_OOB; i+= 16)                                                     
		{                                                                          
			g_print("  OOB Data: %02x %02x %02x %02x %02x %02x "                   
					"%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",     
					oobBuf[i], oobBuf[i+1], oobBuf[i+2],                       
					oobBuf[i+3], oobBuf[i+4], oobBuf[i+5],                     
					oobBuf[i+6], oobBuf[i+7], oobBuf[i+8],                     
					oobBuf[i+9], oobBuf[i+10], oobBuf[i+11],                   
					oobBuf[i+12], oobBuf[i+13], oobBuf[i+14],                  
					oobBuf[i+15]);                                             
		}
	}
}

/*
0x00000000-0x00200000 : "boot_params" 
0x00200000-0x00400000 : "fw_params" 
0x00400000-0x00600000 : "u-boot" 
0x00600000-0x00800000 : "logoimage" 
0x00800000-0x00c00000 : "kernel" 
0x00c00000-0x01000000 : "dspimage" 
0x01000000-0x08000000 : "filesystem" 

0x08000000-0x08200000 : "boot_params" 
0x08200000-0x08400000 : "fw_params" 
0x08400000-0x08600000 : "u-boot" 
0x08600000-0x08800000 : "logoimage" 
0x08800000-0x08c00000 : "kernel" 
0x08c00000-0x09000000 : "dspimage" 
0x09000000-0x10000000 : "filesystem" 

0x10000000-0x40000000 : "reserved
*/

