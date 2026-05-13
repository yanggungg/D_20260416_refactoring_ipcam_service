#ifndef __NF_UTIL_FLASH_RW_H__
#define __NF_UTIL_FLASH_RW_H__

#include <glib.h>
#include <stdint.h>
#include <stdio.h>

#if defined(SUPPORT_NAND_512M)
	#define NAND_LARGE_BLOCK_SIZE_PAGE 		4096	// Large Page
	#define NAND_LARGE_BLOCK_SIZE_OOB 		200		// Large Page
	#define NAND_ERASE_SIZE					0x40000
	#define MAX_MTD_PARTION_CNT				20
	#define NF_FLASH_SIZE_1M				0x100000
#elif defined(SUPPORT_NAND_256M)
	#define NAND_LARGE_BLOCK_SIZE_PAGE 		2048	// Large Page
	#define NAND_LARGE_BLOCK_SIZE_OOB 		64		// Large Page
	#define NAND_ERASE_SIZE					0x20000
	#define MAX_MTD_PARTION_CNT				20
	#define NF_FLASH_SIZE_1M				0x100000
#elif defined(SUPPORT_NAND_128M)
	#define NAND_LARGE_BLOCK_SIZE_PAGE 		2048	// Large Page
	#define NAND_LARGE_BLOCK_SIZE_OOB 		64		// Large Page
	#define NAND_ERASE_SIZE					0x20000
	#define MAX_MTD_PARTION_CNT				10
	#define NF_FLASH_SIZE_1M				0x100000
#endif


#define FLASH_BUF_MAX_LEN 128

struct erase_info_user {
	guint start;
	guint length;
};

struct erase_info_user64 {
    unsigned long long start;
    unsigned long long length;
};

struct mtd_oob_buf {
	guint start;
	guint length;
	guchar *ptr;
};

struct mtd_oob_buf64 {
	unsigned long long start;
	guint	pad;
	guint	length;
	gulong	usr_ptr;
};

#define MTD_ABSENT			0
#define MTD_RAM				1
#define MTD_ROM				2
#define MTD_NORFLASH        3
#define MTD_NANDFLASH       4
#define MTD_DATAFLASH       6
#define MTD_UBIVOLUME       7
 
#define MTD_WRITEABLE       0x400   /* Device is writeable */
#define MTD_BIT_WRITEABLE   0x800   /* Single bits can be flipped */
#define MTD_NO_ERASE        0x1000  /* No erase necessary */
#define MTD_STUPID_LOCK     0x2000  /* Always locked after reset */

// Some common devices / combinations of capabilities
#define MTD_CAP_ROM     0
#define MTD_CAP_RAM     (MTD_WRITEABLE | MTD_BIT_WRITEABLE | MTD_NO_ERASE)
#define MTD_CAP_NORFLASH    (MTD_WRITEABLE | MTD_BIT_WRITEABLE)
#define MTD_CAP_NANDFLASH   (MTD_WRITEABLE)
 
/* ECC byte placement */
#define MTD_NANDECC_OFF     0   // Switch off ECC (Not recommended)
#define MTD_NANDECC_PLACE   1   // Use the given placement in the structure (YAFFS1 legacy mode)
#define MTD_NANDECC_AUTOPLACE   2   // Use the default placement scheme
#define MTD_NANDECC_PLACEONLY   3   // Use the given placement in the structure (Do not store ecc result on read)
#define MTD_NANDECC_AUTOPL_USR  4   // Use the given autoplacement scheme rather than using the default
 
/* OTP mode selection */
#define MTD_OTP_OFF     0
#define MTD_OTP_FACTORY     1
#define MTD_OTP_USER        2

struct mtd_info_user {
	guchar type;
	guint flags;
	guint size;   // Total size of the MTD
	guint erasesize;
	guint writesize;
	guint oobsize;   // Amount of OOB data per block (e.g. 16)
	/* The below two fields are obsolete and broken, do not use them
	* (TODO: remove at some point) */
	guint ecctype;
	guint eccsize;
};  
   
struct region_info_user {
	guint offset;        /* At which this region starts, * from the beginning of the MTD */
	guint erasesize;     /* For this region */
	guint numblocks;     /* Number of blocks in this region */
	guint regionindex;
};  
    
struct otp_info {
	guint start;
	guint length;
	guint locked;
};

struct mtd_epage_buf { 
	unsigned long long start;
	guint data_len;
	guint oob_len;
	guchar *data_ptr;
	guchar *oob_ptr;
};

//typedef long long   __kernel_loff_t;

#define MEMGETINFO			_IOR('M', 1, struct mtd_info_user)
#define MEMERASE			_IOW('M', 2, struct erase_info_user)
#define MEMWRITEOOB			_IOWR('M', 3, struct mtd_oob_buf)
#define MEMREADOOB			_IOWR('M', 4, struct mtd_oob_buf)
#define MEMLOCK				_IOW('M', 5, struct erase_info_user)
#define MEMUNLOCK			_IOW('M', 6, struct erase_info_user)
#define MEMGETREGIONCOUNT	_IOR('M', 7, gint)
#define MEMGETREGIONINFO	_IOWR('M', 8, struct region_info_user)
#define MEMSETOOBSEL		_IOW('M', 9, struct nand_oobinfo)
#define MEMGETOOBSEL		_IOR('M', 10, struct nand_oobinfo)
#define MEMGETBADBLOCK		_IOW('M', 11, loff_t)
#define MEMSETBADBLOCK		_IOW('M', 12, loff_t)
#define OTPSELECT			_IOR('M', 13, gint)
#define OTPGETREGIONCOUNT	_IOW('M', 14, gint)
#define OTPGETREGIONINFO	_IOW('M', 15, struct otp_info)
#define OTPLOCK				_IOR('M', 16, struct otp_info)
#define ECCGETLAYOUT		_IOR('M', 17, struct nand_ecclayout)
#if 0
	#define ECCGETSTATS			_IOR('M', 18, struct mtd_ecc_stats)
	#define MTDFILEMODE			_IO('M', 19)
	#define MEMREADPAGE         _IOR('M', 20, char*)
	#define MEMGETMTDINFO       _IOR('M', 21, struct mtd_info)
#else
	#define ECCGETSTATS     _IOR('M', 18, struct mtd_ecc_stats)
	#define MTDFILEMODE     _IO('M', 19)
	#define MEMERASE64      _IOW('M', 20, struct erase_info_user64)
	#define MEMWRITEOOB64       _IOWR('M', 21, struct mtd_oob_buf64)
	#define MEMREADOOB64        _IOWR('M', 22, struct mtd_oob_buf64)
	#define MEMISLOCKED     _IOR('M', 23, struct erase_info_user)
	#define MEMEREADPAGE        _IOWR('M', 24, struct mtd_epage_buf)
	#define MEMWRITEOOB_FS_YAFFS    _IOWR('M', 25, struct mtd_oob_buf)
//	#define MEMFORCEERASEBLOCK  _IOW('M', 128, __kernel_loff_t)

	#endif
 /*
* Obsolete legacy interface. Keep it in order not to break userspace
* interfaces
*/
struct nand_oobinfo {
	guint useecc;
	guint eccbytes;
	guint oobfree[8][2];
	guint eccpos[32];
};

struct nand_oobfree {
	guint offset;
	guint length;
};

#define MTD_MAX_OOBFREE_ENTRIES 8
#define MTD_MAX_ECCPOS_ENTRIES  64
/*
* ECC layout control structure. Exported to userspace for
* diagnosis and to allow creation of raw images
*/
struct nand_ecclayout {
	guint eccbytes;
	guint eccpos[64];
	guint oobavail;
	struct nand_oobfree oobfree[MTD_MAX_OOBFREE_ENTRIES];
};

struct nand_ecclayout_user {
	guint eccbytes;
	guint eccpos[MTD_MAX_ECCPOS_ENTRIES];
	guint oobavail;
	struct nand_oobfree oobfree[MTD_MAX_OOBFREE_ENTRIES];
};

/**
* struct mtd_ecc_stats - error correction stats
*
* @corrected:  number of corrected bits
* @failed: number of uncorrectable errors
* @badblocks:  number of bad blocks in this partition
* @bbtblocks:  number of blocks reserved for bad block tables
*/
struct mtd_ecc_stats {
	guint corrected;
	guint failed;
	guint badblocks;
	guint bbtblocks;
};

/*
* Read/write file modes for access to MTD
*/
enum mtd_file_modes {
	MTD_MODE_NORMAL = MTD_OTP_OFF,
	MTD_MODE_OTP_FACTORY = MTD_OTP_FACTORY,
	MTD_MODE_OTP_USER = MTD_OTP_USER,
	MTD_MODE_RAW,
};

typedef struct mtd_info_user mtd_info_t;
typedef struct erase_info_user erase_info_t;
typedef struct region_info_user region_info_t;
typedef struct nand_oobinfo nand_oobinfo_t;
typedef struct nand_ecclayout nand_ecclayout_t;

#if defined (_IPX_32P5)
	#define NF_FLASH_PING_FW_PARAM_MTD_NUM					12
	#define NF_FLASH_PONG_FW_PARAM_MTD_NUM					30
	#define NF_FLASH_PING_APP_PARAM_MTD_NUM					11
	#define NF_FLASH_PONG_APP_PARAM_MTD_NUM					29
	#if 0//defined(CONFIG_FWUPGRADE_SINGLE)
		#define NF_FLASH_PONG_HW_PARAM_MTD_NUM					32
	#else
		#define NF_FLASH_PONG_HW_PARAM_MTD_NUM					14
	#endif
#else
	#define NF_FLASH_PING_FW_PARAM_MTD_NUM					4
	#define NF_FLASH_PONG_FW_PARAM_MTD_NUM					14
	#define NF_FLASH_PING_APP_PARAM_MTD_NUM					3
	#if defined(CONFIG_FWUPGRADE_SINGLE)
		#define NF_FLASH_PONG_HW_PARAM_MTD_NUM					8
	#else
		#define NF_FLASH_PONG_HW_PARAM_MTD_NUM					12
	#endif
	#define NF_FLASH_PONG_APP_PARAM_MTD_NUM					13
#endif

typedef enum _NF_UTIL_FLASH_PRGT_E
{
	NF_UTIL_FLASH_PRGT_NONE		= 0,
	
	NF_UTIL_FLASH_PRGT_ERASE_BEGIN 	= 1,
	NF_UTIL_FLASH_PRGT_ERASE_RUN	= 2,
	NF_UTIL_FLASH_PRGT_ERASE_FINISH	= 3,
	NF_UTIL_FLASH_PRGT_ERASE_FAIL	= 4,

	NF_UTIL_FLASH_PRGT_WRITE_BEGIN 	= 5,
	NF_UTIL_FLASH_PRGT_WRITE_RUN	= 6,
	NF_UTIL_FLASH_PRGT_WRITE_FINISH	= 7,
	NF_UTIL_FLASH_PRGT_WRITE_FAIL	= 8,
	
	NF_UTIL_FLASH_PRGT_NR
} NF_UTIL_FLASH_PRGT_E;

typedef struct _NF_UTIL_FLASH_PROGRESS_T
{
	guint state;			/*	NF_UTIL_FLASH_CB_E		*/
	guint cur_block_cnt;	/*	processed block count	*/
	guint tot_block_cnt;	
	guint bad_block_cnt;
} NF_UTIL_FLASH_PROGRESS;

typedef void (*cb_flash_fxn_t)(NF_UTIL_FLASH_PROGRESS *data, gpointer context); 

gboolean nf_flash_get_progress( NF_UTIL_FLASH_PROGRESS *data );

gboolean nf_flash_write(gint mtd_block_num, gchar *img_path, gboolean autoplace, gboolean writeoob, 
			    cb_flash_fxn_t cb_func, gpointer context);
gboolean nf_flash_erase(gint mtd_block_num, cb_flash_fxn_t cb_func, gpointer context);
gboolean nf_flash_dump(gint mtd_block_num, gchar* dump_path );
gboolean nf_flash_read(gint mtd_block_num, guint offs, gint read_oob, guchar* buf, guchar* oobBuf);
gboolean nf_flash_read_block(gint mtd_block_num, guint offs, guchar* dataBuf);
gboolean nf_flash_page_write(gint mtd_block_num, guint offs, guchar *dataBuf, guchar* oobBuf, 
								gboolean autoplace, gboolean writeoob);
gboolean nf_flash_fw_write(gint mtd_block_num, FILE* fp, glong data_len, gboolean autoplace,
							gboolean writeoob, cb_flash_fxn_t cb_func, gpointer context);

gboolean nf_flash_fw_yaffs_write(gint mtd_block_num, FILE* fp, glong data_len, gboolean autoplace,
							gboolean writeoob, cb_flash_fxn_t cb_func, gpointer context);
void write_page_test();
void flash_print(guchar *dataBuf , guchar *oobBuf, gint readoob);
void prog_cb_func(NF_UTIL_FLASH_PROGRESS *data, gpointer context);
void test_prog_print();

#endif

