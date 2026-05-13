/*
 * ITX Security
 *  System software group
 *
 *  2013-01-31 Author YiDongHyung
 */

#ifndef __IPX_HUB_MANAGER__
#define __IPX_HUB_MANAGER__

#include <nf_util_device.h>
/*
 * Define
 */
#define RECV_PORT_FROM_HUB			(32680)
#define SEND_PORT_TO_HUB			(32681)
#define HUB_TFTP_PORT				(767)
#define SOCK_BUF_SZ					(1536)
#define MAX_HUB_NUM					(4)
#define MAX_HUB_NUM_SUPPORTED		(3)

#define HUB_PRODUCT_VER_1ST			(28200)
#define HUB_PRODUCT_VER_2ND			(28210)
#define HUB_PRODUCT_VER_3RD			(28220)

#define HUB_FW_IMAGE_PATH			"/NFDVR/data/hub_fw_image"

#define CODE_DEVICE_UNLINKED        (101)
#define CODE_DEVICE_UNLINKED_ACK    (102)
#define CODE_DEVICE_LINKED          (113)
#define CODE_DEVICE_LINKED_ACK      (114)
#define CODE_DEVICE_MACADDR_RES     (121)
#define CODE_DEVICE_MACADDR_ACK     (122)
#define CODE_PORT_STATUS			(131)
#define CODE_PORT_STATUS_ACK		(132)
#define CODE_CONN_ORDER				(141)
#define CODE_POLL_NVR        		(201)
#define CODE_POLL_ACK        		(202)
#define CODE_IP_SET					(211)
#define CODE_IP_SET_ACK				(212)
#define CODE_ALIVE					(221)
#define CODE_HUB_RECONNECT_REQ		(231)
#define CODE_FWUP_REQ				(322)
#define CODE_FWUP_RES				(321)
#define CODE_TFTP_READY				(342)
#define CODE_FILE_TRANS_DONE		(381)
#define CODE_FILE_TRANS_FAIL_SZ		(383)
#define CODE_FILE_TRANS_FAIL_CRC	(384)
#define CODE_FILE_TRANS_FAIL_NAND	(385)
#define CODE_FILE_TRANS_FAIL		(386)
#define CODE_FILE_TRANS_TIMEOUT		(387)
#define CODE_REBOOT_POE_REQ			(401)
#define CODE_REBOOT_POE_RES			(402)
#define CODE_UNLINK_REQ				(501)
#define CODE_UNLINK_RES				(502)
#define CODE_CAMFWUP_REQ			(601)
#define CODE_CAMFWUP_RES			(602)
#define CODE_HUB_POE_OFF_REQ		(701)
#define CODE_HUB_POE_OFF_RES		(702)
#define CODE_HUB_POE_ON_REQ			(703)
#define CODE_HUB_POE_ON_RES			(704)
#define CODE_HUB_VCT_REQ			(801)
#define CODE_HUB_VCT_RES			(802)

#define HUB_TRUE					(1)
#define HUB_FALSE					(0)

/*
 * Enums
 */
enum __IPX_HUB_PORT_STATE_E_
{
	HUB_PORT_STATE_UNLINKED			= 0,
	HUB_PORT_STATE_LINKED			= 1<<0,

	HUB_PORT_STATE_FWUP_WAIT		= 1<<1,
	HUB_PORT_STATE_UBOOT_REQ		= 1<<2,
	HUB_PORT_STATE_FWUP_START		= 1<<3,
	HUB_PORT_STATE_FWUP_COMP		= 1<<4,

	HUB_PORT_STATE_RECONNECT_REQ	= 1<<7,

	HUB_PORT_CONNECTION_COMPLETE    = 1<<8,
	HUB_PORT_STATE_CAMFWUP_REQ		= 1<<9,
};

enum __IPX_HUB_MATCHING_CHS_E_
{
	HUB_MATCHING_CHS_NONE			= 0,
	HUB_MATCHING_CHS_1_8			= 1<<0,
	HUB_MATCHING_CHS_9_16			= 1<<1,
	HUB_MATCHING_CHS_1_4			= 1<<2,
	HUB_MATCHING_CHS_5_8			= 1<<3,
	HUB_MATCHING_CHS_9_12			= 1<<4,
	HUB_MATCHING_CHS_13_16			= 1<<5,
	HUB_MATCHING_CHS_FIXED			= 1<<8,
};

enum __IPX_HUB_MATCHING_CH_MASK_E_
{
	HUB_MATCHING_CH_MASK_1_8			= 0x00ff,
	HUB_MATCHING_CH_MASK_9_16			= 0xff00,
	HUB_MATCHING_CH_MASK_1_4			= 0x000f,
	HUB_MATCHING_CH_MASK_5_8			= 0x00f0,
	HUB_MATCHING_CH_MASK_9_12			= 0x0f00,
	HUB_MATCHING_CH_MASK_13_16			= 0xf000,
};

enum __IPX_HUB_CH_PORT_STATUS_E_
{
	HUB_CH_PORT_STATUS_UNLINKED		= 0,
	HUB_CH_PORT_STATUS_LINKED		= 1<<0,
	HUB_CH_PORT_STATUS_MACADDR		= 1<<2,

	HUB_CH_PORT_STATUS_UNLINK_REQ	= 1<<3,
	HUB_CH_PORT_STATUS_POE_REQ		= 1<<4,

	HUB_CH_PORT_STATUS_POE_OFF_REQ	= 1<<5,
	HUB_CH_PORT_STATUS_POE_ON_REQ	= 1<<6,

};

enum __IPX_HUB_VCT_STATUS_E
{
	HUB_VCT_STATUS_OFF          = 0,
	HUB_VCT_STATUS_REQ          = 1,
	HUB_VCT_STATUS_DONE         = 2,
	HUB_VCT_STATUS_WAIT         = 3,
};

/*
 * Structs
 */
typedef struct __IPX_EXT_HUB_DATA_T_ HUB_DATA_T;
struct __IPX_EXT_HUB_DATA_T_
{
	unsigned int code;
	unsigned int portNum;
	unsigned char hubmac[6];

	unsigned int data1;
	unsigned int data2;
	unsigned char fwver[24];
	unsigned char mac[6];
	unsigned short chksum;
	unsigned char all_fwver[64];
	unsigned char reserved[82];
}__attribute__((packed));

typedef struct __IPX_EXT_HUB_EXT_T_ HUB_EXT_T;
struct __IPX_EXT_HUB_EXT_T_
{
	unsigned int code;
	unsigned int data_len;
	unsigned char hubmac[6];
	unsigned int crc;
	unsigned int fname_len;
	unsigned char fname[32];
	unsigned char reserved[146];
}__attribute__((packed));

typedef struct __IPX_EXT_HUB_PORT_STATUS_T_ HUB_PORT_STATUS_T;
struct __IPX_EXT_HUB_PORT_STATUS_T_
{
	unsigned int code;
	unsigned int portNum;
	unsigned char hubmac[6];

	int     is_discovery;	// 0 : discover 1: unknown
	int     is_active;      // a/d status
	int     port_class;
	int     func_status;
	int     consumption;
	u_int   voltage;
	u_int   current_mA;
	unsigned char reserved[158];
}__attribute__((packed));

typedef struct __IPX_EXT_HUB_VCT_INFO_T_ HUB_VCT_INFO_T;
struct __IPX_EXT_HUB_VCT_INFO_T_
{
	unsigned int code;
	unsigned int portNum;
	unsigned char hubmac[6];

	char result[4];
	int length[4];
	unsigned char reserved[166];
}__attribute__((packed));


typedef struct __IPX_HUB_MANAGE_TABLE_T IPX_HUB_MANAGE_TABLE;
struct __IPX_HUB_MANAGE_TABLE_T
{
	unsigned char	macaddr[6];
	unsigned char	fw_version_hub[24];
	unsigned char	fw_version_dvr[24];
	unsigned int	assigned_ip;		// 10.234.68.XXX
	int				status;				// __IPX_HUB_PORT_STATE_E_
	int				matching_chs;		// __IPX_HUB_MATCHING_CHS_E_
	int				check_time;
	unsigned int    conn_below;
	int				pre_hub_idx;		// 0 ~ (MAX_HUB_NUM - 1): hub idx, MAX_HUB_NUM : IPX
	unsigned int	hub_max_ch;		// 4ch or 8ch 
};

typedef struct __IPX_HUB_PORT_MANAGE_TABLE_T IPX_HUB_PORT_MANAGE_TABLE;
struct __IPX_HUB_PORT_MANAGE_TABLE_T
{
	int				status;
	unsigned char	macaddr[6];
	unsigned int	recv_cnt;
	int				req_status;
	int				req_seq;
};

typedef struct __IPX_VHUB_PORT_MANAGE_TABLE_T IPX_VHUB_PORT_MANAGE_TABLE;
struct __IPX_VHUB_PORT_MANAGE_TABLE_T
{
	int				status;
	unsigned char	cam_macaddr[6];
	unsigned char	hub_macaddr[6];
	int				req_seq;
};

typedef struct __IPX_HUB_FW_VERSION_TABLE_T IPX_HUB_FW_VERSION_TABLE;
struct __IPX_HUB_FW_VERSION_TABLE_T
{
	unsigned char hubFWVersion_dvr[24];
	int dvr_product_code;
	int dvr_proto_code;
	int dvr_minor_code;
	int dvr_buyer_code;

	char fw_path[256];
	int fw_have;
};

typedef struct __IPX_HUB_VCT_INFO_T IPX_HUB_VCT_INFO;
struct __IPX_HUB_VCT_INFO_T
{
	int	status;
	char result[4];
	int length[4];
};

/*
 * Declare external functions.
 */
extern void vhub_manager_init();
extern int  vhub_get_port_linkinfo();
extern int  vhub_get_port_macaddr(int, unsigned char*);
extern void vhub_get_hub_conn_order();
extern void	vhub_set_port_unlink(int);
extern void	vhub_set_port_poe_reset(int);
extern void vhub_set_port_poe_off(int);
extern void vhub_set_port_poe_on(int);
extern void vhub_set_camfwup();
extern void vhub_set_data_clear();
extern void vhub_set_data_rebuild();
extern int  vhub_get_hub_macaddr(unsigned char*);
extern int  vhub_get_hub_fwver(unsigned char*);

extern int  ipx_hub_current_link_status(int ch);
extern int  hub_find_port(unsigned char* mac);
extern void hub_fw_upgrade(int type);
extern void hub_poe_reboot(int ch);
extern void hub_unlink_request(int ch);
extern void hub_camfwup_request();
extern void hub_set_port_vct(int, int);
extern void hub_get_port_vct(guint dev_num, NF_UTIL_SWITCH_VCT_INFO* vct_res);
extern int hub_get_port_vct_status(int dev_num, int port);
extern int hub_get_status(int dev_num);
extern int hub_fw_ver_chg(int idx, char *fw_ver);
extern int hub_fw_have_chg(char *have_str);

/*
 * Declare static functions.
 */
static void hub_recv_func();
static int  hub_recv_func_init();
static void hub_recv_func_check_wan_link();
static int  hub_recv_func_receive_msg(HUB_DATA_T*, HUB_PORT_STATUS_T*, HUB_VCT_INFO_T*);
static int  hub_recv_func_receive_msg_filter(HUB_DATA_T*, HUB_PORT_STATUS_T*);
static void	hub_recv_func_process_msg(HUB_DATA_T*, HUB_PORT_STATUS_T*, HUB_VCT_INFO_T*);
static int  hub_recv_func_process_req();
static void	hub_recv_func_process_timeout();
static void	hub_recv_func_process_job();
static void hub_recv_func_check_data_changed();

static int  hub_open_sock();
static int  hub_send_msg_broadcast(HUB_DATA_T*);
static int  hub_send_msg_target(HUB_DATA_T*, unsigned int);
static int  hub_send_ext_target(HUB_EXT_T*, unsigned int);
static void	hub_send_msg_print(HUB_DATA_T*);
static void	hub_msg_print(HUB_DATA_T*);
static void hub_ext_print(HUB_DATA_T*);
static int  hub_make_checksum(HUB_DATA_T*);
static int  hub_make_ipaddr(unsigned int*);
static int  hub_check_fwver(int);
static int  hub_parse_fwver(char*, int*, int*, int*, int*);
static int  hub_fwver_part_to_int(char*, int, int);
static void hub_tftpd_up();
static void hub_tftpd_down();
static void hub_disconnect_all_hubs();

static void hub_manage_table_init();
static void hub_manage_table_print();
static int  hub_manage_table_add(HUB_DATA_T*);
static int  hub_manage_table_search(unsigned char*);
static int  hub_manage_table_check_matching_chs(int, int);
static int  hub_manage_table_find_empty_idx();
static void hub_manage_table_set_unlink(int);
static void convertMacAddrStringIntoByte(const char *pszMACAddress, unsigned char* pbyAddress);
static unsigned char hub_manage_table_empty_entry[6] = {0,0,0,0,0,0};

static void hub_port_manage_table_print();

static void hub_handler_poll(HUB_DATA_T*);
static void hub_handler_ipset(HUB_DATA_T*);
static void hub_handler_fwup_res(HUB_DATA_T*);
static void hub_handler_fwup_done(HUB_DATA_T*);
static void hub_handler_alive(HUB_DATA_T*);
static void hub_handler_macaddr(HUB_DATA_T*);
static void hub_handler_linked(HUB_DATA_T*);
static void hub_handler_unlinked(HUB_DATA_T*);
static void hub_handler_unlink_res(HUB_DATA_T*);
static void hub_handler_poe_reset_res(HUB_DATA_T*);
static void hub_handler_poe_off_res(HUB_DATA_T*);
static void hub_handler_poe_on_res(HUB_DATA_T*);
static void hub_handler_camfwup_res(HUB_DATA_T*);
static void hub_handler_port_status(HUB_PORT_STATUS_T*);
static void hub_handler_conn_order(HUB_DATA_T*);
static void hub_handler_vct_info(HUB_VCT_INFO_T* p_vct_info);

static void hub_handler_req_fwup(int);
static void hub_handler_req_reconnect(int);
static void hub_handler_req_unlink(int, int, int);
static void hub_handler_req_poe_reset(int, int, int);
static void hub_handler_req_poe_off(int, int);
static void hub_handler_req_poe_on(int, int);
static void hub_handler_req_camfwup(int);
static void hub_handler_req_vct(int, int);

static void vhub_init();
static void vhub_table_print();
static int  vhub_process_req();
static void vhub_insert(int);
static void vhub_set_macaddr(int, int);
static int  vhub_delete_by_hub_macaddr_and_ch(unsigned char*, int);
static void vhub_delete_by_hub_macaddr(unsigned char*);

static void hub_sysdb_reload_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
static int hub_start_ch_num(int);
static int hub_end_ch_num(int);

static int hub_make_fwver_strs(char *buf, size_t buf_size);

#endif // __IPX_HUB_MANAGER__
