#ifndef __NF_UTIL_FW_H__
#define __NF_UTIL_FW_H__

#include "nf_common.h"
#include "nf_util_flash.h"
#include "nf_util_http.h"

#define FW_UPGRADE_RESERVED_MTD_PONG_NUM		11

#define FW_UPGRADE_MAX_BUFFER					256
#define FW_UPGRADE_MAX_MTD_NUM					32
#define FW_UPGRADE_UBL_MTD_NUM					0
#define FW_UPGRADE_MAX_IMG_TYPE_NUM				16	


#define FW_UPGRADE_MAX_MAGIC_BUF				4
#define FW_UPGRADE_NF_MAGIC_S					"ITXS"
#define FW_UPGRADE_NF_MAGIC_E					"ITXE"
#define FW_UPGRADE_NAME_TITLE					"nf_firmware"

#if defined(SUPPORT_NAND_512M)
	#define FW_UPGRADE_NAND_PAGE_SIZE				4096
	#define FW_UPGRADE_NAND_OOB_SIZE				200
#elif defined(SUPPORT_NAND_256M)
	#define FW_UPGRADE_NAND_PAGE_SIZE				2048
	#define FW_UPGRADE_NAND_OOB_SIZE				64
#elif defined(SUPPORT_NAND_128M)
	#define FW_UPGRADE_NAND_PAGE_SIZE				2048
	#define FW_UPGRADE_NAND_OOB_SIZE				64
#endif
#define FW_UPGRADE_MTD_PING_NUM					12
#define FW_UPGRADE_MTD_PONG_NUM					26

typedef enum _NF_FW_UPGRADE_MTD_NUM_E
{
	FW_UPGRADE_UBOOT_MTD_NUM				= 2,

	FW_UPGRADE_UBOOT_PING_MTD_NUM			= 4,
	FW_UPGRADE_LOGO_PING_MTD_NUM			= 9,
	FW_UPGRADE_KERNEL_PING_MTD_NUM			= 6,
	FW_UPGRADE_DSP_PING_MTD_NUM				= 15,	// reserved ANF8G micom
	FW_UPGRADE_FILESYS_PING_MTD_NUM			= 7,

	FW_UPGRADE_UBOOT_PONG_MTD_NUM			= 21,
	FW_UPGRADE_FPGA_PONG_MTD_NUM			= 23,
	FW_UPGRADE_KERNEL_PONG_MTD_NUM			= 19,
	FW_UPGRADE_DSP_PONG_MTD_NUM				= 29,	// reserved
	FW_UPGRADE_FILESYS_PONG_MTD_NUM			= 20
} NF_FW_UPGRADE_MTD_NUM;

typedef enum _NF_FW_UPGRADE_MTD_E
{
	FW_UPGRADE_PONG			=	0,
	FW_UPGRADE_PING			=	1
} NF_FW_UPGRADE_MTD_E;

typedef enum _NF_FW_TYPE_E
{
	NF_FW_TYPE_UBOOT	=	0,
	NF_FW_TYPE_KERNEL	=	1,
	NF_FW_TYPE_DSP 		=	2,
	NF_FW_TYPE_FILESYS 	=	3,
	NF_FW_TYPE_LOGO	 	=	4,
	NF_FW_TYPE_FPGA 	=	5,
	NF_FW_TYPE_RSVD_B 	=	6
} NF_FW_TYPE_E;

#define SWAP_LONG(x) \
			((guint)( \
			(((guint)(x) & (guint)0x000000ffUL) << 24) | \
	 		(((guint)(x) & (guint)0x0000ff00UL) <<  8) | \
			(((guint)(x) & (guint)0x00ff0000UL) >>  8) | \
   			(((guint)(x) & (guint)0xff000000UL) >> 24) ))

#define		fw_ntohl(a)    SWAP_LONG(a)
#define		fw_htonl(a)    SWAP_LONG(a)

#define IH_NMLEN		32			/* Image Name Length */
#define IH_MAGIC	 	0x27051900
typedef struct image_header {
	guint	ih_magic;				/* Image Header Magic Number    */		// 4	
	guint	ih_hcrc;				/* Image Header CRC Checksum    */		// 4 + 4 = 8
	guint	ih_time;				/* Image Creation Timestamp 	*/		// 8 + 4 = 12
	guint	ih_size;				/* Image Data Size				*/		// 12 + 4 = 16
	guint	ih_load;				/* Data  Load  Address			*/		// 16 + 4 = 20
	guint	ih_ep;					/* Entry Point Address			*/		// 20 + 4 = 24
	guint	ih_dcrc;				/* Image Data CRC Checksum		*/		// 24 + 4 = 28
	gchar	ih_os;					/* Operating System				*/		// 28 + 1 = 29
	gchar	ih_arch;   				/* CPU architecture				*/		// 29 + 1 = 30
	gchar	ih_type;   				/* Image Type					*/		// 30 + 1 = 31
	gchar	ih_comp;    			/* Compression Type				*/		// 31 + 1 = 32
	gchar	ih_name[IH_NMLEN];  	/* Image Name					*/		// 32 + 32 = 64
} image_header_t;

typedef struct _NF_FW_IMAGE_HEADER_T
{
	gchar	magic[4]; 				// "ITXS"
	image_header_t	fwheader;

	gchar	model[64];
	gchar	vendor[32];
	gchar	lang[32];
	gchar	option[64];
	gchar	version[64];
	guint   seq;
	gchar	reserved[4][32];

	guint	sec_cnt;
	image_header_t sec_header[16];

} NF_FW_IMAGE_HEADER;

#define FW_UPGRADE_PRGT_ERROR				1
#define FW_UPGRADE_PRGT_OK					0

typedef struct _NF_FW_PRGT_T
{
	short	type;
	short	state;

	gint	is_error;              // OK:0 //ERR:1

	guint	current; 
	guint	total;
	guint	privates[4];
} NF_FW_PRGT;

typedef enum _NF_FW_PRGT_TYPE_E
{
	NF_FW_PRGT_TYPE_PRE					= 0,
	NF_FW_PRGT_TYPE_UBOOT				= 1,
	NF_FW_PRGT_TYPE_KERNEL				= 2,
	NF_FW_PRGT_TYPE_DSP					= 3,
	NF_FW_PRGT_TYPE_FS					= 4,
	NF_FW_PRGT_TYPE_LOGO				= 5,
	NF_FW_PRGT_TYPE_FPGA				= 6,
	NF_FW_PRGT_TYPE_FINISH				= 7
} NF_FW_PRGT_TYPE;

typedef enum _NF_FW_PRGT_PRE_E
{
	NF_FW_PRGT_PRE_START				= 0,
	NF_FW_PRGT_PRE_THREAD_CREAT			= 1,
	NF_FW_PRGT_PRE_SEQ					= 2,
	NF_FW_PRGT_PRE_FILE_OPEN			= 3,
	NF_FW_PRGT_PRE_FILE_READ			= 4,
	NF_FW_PRGT_PRE_S_MAGIC_CHECK		= 5	
} NF_FW_PRGT_PRE_E;

typedef enum _NF_FW_PRGT_FINISH_E
{
	NF_FW_PRGT_FNI_START				= 0,
	NF_FW_PRGT_FNI_E_MAGIC_CHECK		= 1, 
	NF_FW_PRGT_FNI_SEQ_UPDATE			= 2,
	NF_FW_PRGT_FNI_SET_UPDATE_VARIABLE	= 3, 
	NF_FW_PRGT_FNI_UPDATE_FNISH			= 4
} NF_FW_PRGT_FINISH_E;

typedef enum _NF_FW_PRGT_IMG_E
{
	NF_FW_PRGT_IMG_START				= 0,
	NF_FW_PRGT_IMG_ERASE				= 1,
	NF_FW_PRGT_IMG_WRITE				= 2,
	NF_FW_PRGT_IMG_CRC_CHECK			= 3,
} NF_FW_PRGT_IMG_E;

typedef struct _NF_FW_IMAGE_LIST_T
{
	gchar	filename[FW_UPGRADE_MAX_BUFFER];
	NF_FW_IMAGE_HEADER	fwheader;
} NF_FW_IMAGE_LIST; 

#define FW_UPGRADE_NF_MAGIC_U 				 "ITXU"
typedef struct _NF_FW_UPGRADE_CHECK_T
{
	gchar	magic[4];
	gint	is_upgrade;
} NF_FW_UPGRADE_CHECK;

#define IMAGE_TABLE_MAX_NUM         16

typedef struct _NF_FW_IMAGE_LIST_TABLE_T
{
	gchar   name[32];
	gint    ping_mtd_num;
	gint    pong_mtd_num;
	guint   data_len;
	guint   crc_check_len;
	guint   checksum;
	gint    autoplace;
	gint    writeoob;
	gint    img_type;
	gint    is_header_write;
	guint   rsvd1;
	guint   rsvd2;
} NF_FW_IMAGE_LIST_TABLE;

#define FW_UPGRADE_LIST_SIZE                sizeof(NF_FW_IMAGE_LIST)
#define FW_UPGRADE_IMG_HEADER_SIZE          sizeof(image_header_t)

void nf_fw_upgrade_main(void);

gboolean nf_fw_start_upgrade(gchar *filename);
gboolean nf_fw_list(gchar *path, NF_FW_IMAGE_LIST *fwimglist, gint req_cnt, gint *ret_cnt);
gboolean nf_fw_ls(gchar *path);
void nf_fw_imgh_print(image_header_t *img_header);
gboolean nf_fw_file_validate_check(const gchar *filename);
void nf_fw_set_prgt_state(gshort type, short state, gint is_error, guint current, guint total);
gboolean nf_fw_crc_check(gint mtd_num, guint file_checksum, guint filesize, gint img_type, gint prgt_type, gboolean is_header_write);
gint nf_fw_get_seq(guint *f_seq, guint *s_seq, gint *is_first);
gboolean nf_fw_set_seq(guint *ping_seq, guint *pong_seq, gint is_first, NF_FW_IMAGE_LIST *nand_imglist);

gboolean nf_util_fw_start_upgrade_hdd(void);
gboolean nf_fw_copy_from_dev_to_hdd(gchar *src);
gboolean nf_fw_bmp_write(gchar* bmp_path);
void hexa_print(guchar *dataBuf, guint offs);
void hexa_print_with_oob(guchar *dataBuf, guint offs);
void nf_fw_state_check(NF_FW_PRGT *ui_prgt);
gboolean nf_fw_update_uboot(FILE *fp, glong offset_uboot, NF_FW_IMAGE_LIST_TABLE *img_list_table);
#if 0
	gboolean nf_fw_set_update_variable(void);
#endif

gboolean nf_fw_upgrade_thread_start(gchar *fw_filename, gboolean is_nonblocking);
gboolean nf_fw_get_file_header(char *filename, NF_FW_IMAGE_HEADER *header);
void nf_fw_cb_func(NF_UTIL_FLASH_PROGRESS *data, gpointer context);
void nf_fw_continue_led(guint cnt);
gboolean nf_fw_set_app_param(void);
gboolean nf_fw_hotkey_upgrade(void);
void nf_fw_start_upgrade_beep(void);
void nf_fw_finish_upgrade_beep(void);
gint nf_fw_seq_update_test(void);
void fw_set_prgt_state(gshort type, short state, gint is_error, guint current, guint total);

#if 0
/*** 20090601 fpga upgrade **/
int nf_fw_fpga_up(void);
#endif


typedef struct  _NF_FW_NETWORK_VERINFO_T {

	char	*version;
	char	*date;
	char	*time;	
	char	*is_bridge;	// 'Y','N'
	char	*is_urgent;	// 'Y','N'
	char	*fw_file;	// http://cdn.dvrlink.net/DVR/xxxxx/yyy/xxxxx.1.zzzzz.yyy.nbn
	char	*rel_file;	// /tmp/xxxxx.1.zzzzz.yyy.rel
	char	*reserved1;	// vendor_version
	char	*reserved2;
	char	*reserved3;	
	
	int		major_ver;
	int		minor_ver;

	char	reserved[4096];	
	
} NF_FW_NETWORK_VERINFO;

/*
 xxxxx: product code
 yyy: vendor code
 zzzzz: minor version
*/


gboolean 
nf_fw_network_upgrade_download( const char *fw_file, 			
			gboolean nb_flag,  
			GError **error );

gboolean nf_fw_network_upgrade_info_download( GError **error );
gboolean nf_fw_network_upgrade_check( NF_FW_NETWORK_VERINFO *ver );
gboolean nf_fw_network_upgrade_prgt( NF_HTTP_CLIENT_PRGT *prgt );

#define NF_FW_SVR_HOST                  "http://upgrade.sequrinet.com"
#define NF_FW_SVR_FW_PATH		NF_FW_SVR_HOST"/FW/%d/%d/%d%s/"
#define NF_FW_SVR_VERINFO_FILE          NF_FW_SVR_FW_PATH"UpgradeHistory.info"

#define NF_FW_LOCAL_VERINFO_FILE	"/tmp/UpgradeHistory.info"

gboolean nf_fw_network_upgrade_hdd_init(char* fw_path, int fw_path_len);
gboolean nf_fw_network_upgrade_hdd_mount(void);
void nf_fw_network_upgrade_hdd_mount_state(int _mode, int *_val);

#define NET_FWUP_MOUNT_DIR_NAME "/mnt/fwup_hdd"
#define NET_FWUP_MOUNT_TYPE		"ext4"

#endif

