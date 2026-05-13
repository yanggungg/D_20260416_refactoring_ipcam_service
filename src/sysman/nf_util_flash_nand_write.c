#include "nf_common.h"

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <getopt.h>

#include <asm/types.h>
#include "mtd_nfhost/mtd-user.h"
//#include "common.h"
#include <libmtd.h>

/*
	Gloval Function Definition
*/
static void erase_buffer(void *buffer, size_t size);
static int is_virt_block_bad(struct mtd_dev_info *mtd, int fd, long long offset, int blockalign);
static void *xmalloc(size_t size);
static inline int buffer_check_pattern(unsigned char *buffer, size_t size, unsigned char pattern);


gboolean nf_util_flash_nand_write(int mtdnum, long long imglen)
{
	int fd=-1;
	int ret=0;
	int pagelen=0;
	long long blockstart = -1;
	struct mtd_dev_info mtd;
	/* contains all the data read from the file so far for the current eraseblock */
	unsigned char *filebuf = NULL;
	size_t filebuf_max = 0;
	size_t filebuf_len = 0;
	/* points to the current page inside filebuf */
	unsigned char *writebuf = NULL;
	/* points to the OOB for the current page in filebuf */
	unsigned char *oobbuf = NULL;

	char mtd_device[64] = {0,};
	libmtd_t mtd_desc;
	int ebsize_aligned;
	int blockalign = 1; /* default to using actual block size */
	uint8_t write_mode;


	long long mtdoffset = 0;
	gboolean quiet=FALSE;
	gboolean writeoob=FALSE, onlyoob=FALSE, markbad=FALSE, noecc=FALSE, autoplace=FALSE, skipallffs=FALSE, noskipbad=FALSE;
	gboolean pad=FALSE, skip_bad_blocks_to_start=FALSE;

	sprintf(mtd_device, "/dev/mtd%d", mtdnum);

	/* Open the device */
	if ((fd = open(mtd_device, O_RDWR)) == -1) {
		printf("[NF_UTIL][NAND][Write] %s open failed..\n", mtd_device);

		close(fd);
		goto nf_util_flash_nand_write_fail;
	}

	mtd_desc = libmtd_open();
	if (!mtd_desc) {
		printf("[NF_UTIL][NAND][Write] libmtd open failed..\n");

		close(fd);
		goto nf_util_flash_nand_write_fail;
	}

	/* Fill in MTD device capability structure */
	if (mtd_get_dev_info(mtd_desc, mtd_device, &mtd) < 0) {
		printf("[NF_UTIL][NAND][Write] mtd_get_dev_info failed..\n");

		libmtd_close(mtd_desc);
		close(fd);

		goto nf_util_flash_nand_write_fail;
	}

	/*
	 * Pretend erasesize is specified number of blocks - to match jffs2
	 *   (virtual) block size
	 * Use this value throughout unless otherwise necessary
	 */
	ebsize_aligned = mtd.eb_size * blockalign;

	if (mtdoffset & (mtd.min_io_size - 1)) {
		printf("[NF_UTIL][NAND][Write] The start address is not page-aligned !\n"
			   "The pagesize of this NAND Flash is 0x%x.\n", mtd.min_io_size);
	}

	/* Select OOB write mode */
	if (noecc)
		write_mode = MTD_OPS_RAW;
	else if (autoplace)
		write_mode = MTD_OPS_AUTO_OOB;
	else
		write_mode = MTD_OPS_PLACE_OOB;

	if (noecc)  {
		ret = ioctl(fd, MTDFILEMODE, MTD_FILE_MODE_RAW);
		if (ret) {
			switch (errno) {
			case ENOTTY:
				printf("[NF_UTIL][NAND][Write] ioctl MTDFILEMODE is missing\n");
			default:
				printf("[NF_UTIL][NAND][Write] MTDFILEMODE\n");
			}
			
			libmtd_close(mtd_desc);
			close(fd);

			goto nf_util_flash_nand_write_fail;
		}
	}

	#if 0
		/* Determine if we are reading from standard input or from a file. */
		if (strcmp(img, standard_input) == 0)
			ifd = STDIN_FILENO;
		else
			ifd = open(img, O_RDONLY);

		if (ifd == -1) {
			perror(img);
			goto closeall;
		}
	#endif

	pagelen = mtd.min_io_size + ((writeoob) ? mtd.oob_size : 0);

	#if 0
		if (ifd == STDIN_FILENO) {
			imglen = inputsize ? : pagelen;
			if (inputskip) {
				errmsg("seeking stdin not supported");
				goto closeall;
			}
		} else {
			if (!inputsize) {
				struct stat st;
				if (fstat(ifd, &st)) {
					sys_errmsg("unable to stat input image");
					goto closeall;
				}
				imglen = st.st_size - inputskip;
			} else
				imglen = inputsize;

			if (inputskip && lseek(ifd, inputskip, SEEK_CUR) == -1) {
				sys_errmsg("lseek input by %lld failed", inputskip);
				goto closeall;
			}
		}
	#endif

	/* Check, if file is page-aligned */
	if (!pad && (imglen % pagelen) != 0) {
		printf("[NF_UTIL][NAND][Write] Input file is not page-aligned. Use the padding option.\n");
		goto nf_util_flash_nand_write_fail2;
	}

	/* Skip bad blocks on the way to the start address if necessary */
	if (skip_bad_blocks_to_start) {
		long long bbs_offset = 0;
		while (bbs_offset < mtdoffset) {
			ret = is_virt_block_bad(&mtd, fd, bbs_offset, blockalign);
			if (ret < 0) {
				printf("[NF_UTIL][NAND][Write] %s: MTD get bad block failed\n", mtd_device);
				goto nf_util_flash_nand_write_fail2;
			} else if (ret == 1) {
				if (!quiet) {
					printf("[NF_UTIL][NAND][Write] Bad block at %llx, %u block(s) from %llx will be skipped\n",
						bbs_offset, blockalign, bbs_offset);
				}
				mtdoffset += ebsize_aligned;
			}
			bbs_offset += ebsize_aligned;
		}
	}

	/* Check, if length fits into device */
	if ((imglen / pagelen) * mtd.min_io_size > mtd.size - mtdoffset) {
		printf("[NF_UTIL][NAND][Write] Image %lld bytes, NAND page %d bytes, OOB area %d bytes, device size %lld bytes\n",
				imglen, pagelen, mtd.oob_size, mtd.size);
		printf("[NF_UTIL][NAND][Write] Input file does not fit into device\n");
		goto nf_util_flash_nand_write_fail2;
	}

	/*
	 * Allocate a buffer big enough to contain all the data (OOB included)
	 * for one eraseblock. The order of operations here matters; if ebsize
	 * and pagelen are large enough, then "ebsize_aligned * pagelen" could
	 * overflow a 32-bit data type.
	 */
	filebuf_max = ebsize_aligned / mtd.min_io_size * pagelen;
	filebuf = xmalloc(filebuf_max);
	if(filebuf == NULL) {
		printf("[NF_UTIL][NAND][Write] out of memory. alloc fail..\n");
	}
	erase_buffer(filebuf, filebuf_max);

	/*
	 * Get data from input and write to the device while there is
	 * still input to read and we are still within the device
	 * bounds. Note that in the case of standard input, the input
	 * length is simply a quasi-boolean flag whose values are page
	 * length or zero.
	 */
	while ((imglen > 0 || writebuf < filebuf + filebuf_len)
		&& mtdoffset < mtd.size) {
		/*
		 * New eraseblock, check for bad block(s)
		 * Stay in the loop to be sure that, if mtdoffset changes because
		 * of a bad block, the next block that will be written to
		 * is also checked. Thus, we avoid errors if the block(s) after the
		 * skipped block(s) is also bad (number of blocks depending on
		 * the blockalign).
		 */ 
		while (blockstart != (mtdoffset & (~ebsize_aligned + 1))) {
			blockstart = mtdoffset & (~ebsize_aligned + 1);

			/*
			 * if writebuf == filebuf, we are rewinding so we must
			 * not reset the buffer but just replay it
			 */
			if (writebuf != filebuf) {
				erase_buffer(filebuf, filebuf_len);
				filebuf_len = 0;
				writebuf = filebuf;
			}

			if (!quiet)
				printf("[NF_UTIL][NAND][Write] Writing data to block %lld at offset 0x%llx\n", 
						blockstart / ebsize_aligned, blockstart);

			if (noskipbad)
				continue;

			ret = is_virt_block_bad(&mtd, fd, blockstart, blockalign);

			if (ret < 0) {
				printf("[NF_UTIL][NAND][Write] %s: MTD get bad block failed\n", mtd_device);
				goto nf_util_flash_nand_write_fail2;
			} else if (ret == 1) {
				if (!quiet)
					printf("[NF_UTIL][NAND][Write] Bad block at %llx, %u block(s) will be skipped\n", blockstart, blockalign);

				mtdoffset = blockstart + ebsize_aligned;

				if (mtdoffset > mtd.size) {
					printf("[NF_UTIL][NAND][Write] too many bad blocks, cannot complete request\n");
					goto nf_util_flash_nand_write_fail2;
				}
			}
		}

		#if 0
		/* Read more data from the input if there isn't enough in the buffer */
		if (writebuf + mtd.min_io_size > filebuf + filebuf_len) {
			size_t readlen = mtd.min_io_size;
			size_t alreadyread = (filebuf + filebuf_len) - writebuf;
			size_t tinycnt = alreadyread;
			ssize_t cnt = 0;

			while (tinycnt < readlen) {
				cnt = read(ifd, writebuf + tinycnt, readlen - tinycnt);
				if (cnt == 0) { /* EOF */
					break;
				} else if (cnt < 0) {
					perror("File I/O error on input");
					goto closeall;
				}
				tinycnt += cnt;
			}

			/* No padding needed - we are done */
			if (tinycnt == 0) {
				/*
				 * For standard input, set imglen to 0 to signal
				 * the end of the "file". For nonstandard input,
				 * leave it as-is to detect an early EOF.
				 */
				if (ifd == STDIN_FILENO)
					imglen = 0;

				break;
			}

			/* Padding */
			if (tinycnt < readlen) {
				if (!pad) {
					fprintf(stderr, "Unexpected EOF. Expecting at least "
							"%zu more bytes. Use the padding option.\n",
							readlen - tinycnt);
					goto closeall;
				}
				erase_buffer(writebuf + tinycnt, readlen - tinycnt);
			}

			filebuf_len += readlen - alreadyread;
			if (ifd != STDIN_FILENO) {
				imglen -= tinycnt - alreadyread;
			} else if (cnt == 0) {
				/* No more bytes - we are done after writing the remaining bytes */
				imglen = 0;
			}
		}
		#endif	

		#if 0
		if (writeoob) {
			oobbuf = writebuf + mtd.min_io_size;

			/* Read more data for the OOB from the input if there isn't enough in the buffer */
			if (oobbuf + mtd.oob_size > filebuf + filebuf_len) {
				size_t readlen = mtd.oob_size;
				size_t alreadyread = (filebuf + filebuf_len) - oobbuf;
				size_t tinycnt = alreadyread;
				ssize_t cnt;

				while (tinycnt < readlen) {
					cnt = read(ifd, oobbuf + tinycnt, readlen - tinycnt);
					if (cnt == 0) { /* EOF */
						break;
					} else if (cnt < 0) {
						printf("[NF_UTIL][NAND][Write] File I/O error on input\n");
						goto nf_util_flash_nand_write_fail2;
					}
					tinycnt += cnt;
				}

				if (tinycnt < readlen) {
						printf("[NF_UTIL][NAND][Write] Unexpected EOF. Expecting at least zu more bytes for OOB\n",
									readlen - tinycnt);
					goto nf_util_flash_nand_write_fail2;
				}

				filebuf_len += readlen - alreadyread;
				if (ifd != STDIN_FILENO) {
					imglen -= tinycnt - alreadyread;
				} else if (cnt == 0) {
					/* No more bytes - we are done after writing the remaining bytes */
					imglen = 0;
				}
			}
		}
		#endif

		ret = 0;
		if (!skipallffs || !buffer_check_pattern(writebuf, mtd.min_io_size, 0xff)) {
			/* Write out data */
			ret = mtd_write(mtd_desc, &mtd, fd, mtdoffset / mtd.eb_size,
					mtdoffset % mtd.eb_size,
					onlyoob ? NULL : writebuf,
					onlyoob ? 0 : mtd.min_io_size,
					writeoob ? oobbuf : NULL,
					writeoob ? mtd.oob_size : 0,
					write_mode);
		}

		if (ret) {
			if (errno != EIO) {
				printf("[NF_UTIL][NAND][Write] %s: MTD write failure\n", mtd_device);
				goto nf_util_flash_nand_write_fail2;
			}

			/* Must rewind to blockstart if we can */
			writebuf = filebuf;

			printf("[NF_UTIL][NAND][Write] Erasing failed write from %#08llx to %#08llx\n",
				blockstart, blockstart + ebsize_aligned - 1);

			if (mtd_erase_multi(mtd_desc, &mtd, fd,
					blockstart / mtd.eb_size, blockalign)) {
				int errno_tmp = errno;
				printf("[NF_UTIL][NAND][Write] %s: MTD Erase failure\n", mtd_device);
				if (errno_tmp != EIO)
					goto nf_util_flash_nand_write_fail2;
			}

			if (markbad) {
				printf("[NF_UTIL][NAND][Write] Marking block at %08llx bad\n", mtdoffset & (~mtd.eb_size + 1));
				if (mtd_mark_bad(&mtd, fd, mtdoffset / mtd.eb_size)) {
					printf("[NF_UTIL][NAND][Write] %s: MTD Mark bad block failure\n", mtd_device);
					goto nf_util_flash_nand_write_fail2;
				}
			}
			mtdoffset = blockstart + ebsize_aligned;

			continue;
		}
		mtdoffset += mtd.min_io_size;
		writebuf += pagelen;
	}
//	close(ifd);
	libmtd_close(mtd_desc);
	close(fd);

	return TRUE;

nf_util_flash_nand_write_fail:

	return FALSE;

nf_util_flash_nand_write_fail2:

	libmtd_close(mtd_desc);
	close(fd);

	return FALSE;
}

static void erase_buffer(void *buffer, size_t size)
{
	const uint8_t kEraseByte = 0xff;

	if (buffer != NULL && size > 0)
		memset(buffer, kEraseByte, size);
}

static int is_virt_block_bad(struct mtd_dev_info *mtd, int fd,
				long long offset, int blockalign)
{
	int i, ret = 0;

	for (i = 0; i < blockalign; ++i) {
		ret = mtd_is_bad(mtd, fd, offset / mtd->eb_size + i);
		if (ret)
			break;
	}

	return ret;
}

static void *xmalloc(size_t size)
{
	void *ptr = malloc(size);

	if (ptr == NULL && size != 0) {
		return NULL;
	}
	return ptr;
}

/* Check whether buffer is filled with character 'pattern' */
static inline int buffer_check_pattern(unsigned char *buffer, size_t size,
					   unsigned char pattern)
{
	/* Invalid input */
	if (!buffer || (size == 0))
		return 0;

	/* No match on first byte */
	if (*buffer != pattern)
		return 0;

	/* First byte matched and buffer is 1 byte long, OK. */
	if (size == 1)
		return 1;

	/*
	 * Check buffer longer than 1 byte. We already know that buffer[0]
	 * matches the pattern, so the test below only checks whether the
	 * buffer[0...size-2] == buffer[1...size-1] , which is a test for
	 * whether the buffer is filled with constant value.
	 */
	return !memcmp(buffer, buffer + 1, size - 1);
}

