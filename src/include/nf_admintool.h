#ifndef H_NF_ADMINTOOL
#define H_NF_ADMINTOOL

#define PACKET_LEN		(ETH_HDR_SIZE + IP_HDR_SIZE + UDP_HDR_SIZE + DATA_SIZE)
#define ETH_HDR_SIZE	(14)
#define IP_HDR_SIZE		sizeof(struct iphdr)
#define UDP_HDR_SIZE	sizeof(struct udphdr)
#define DATA_SIZE		sizeof(netconf_Message)
#define DEF_IP_ADDR		"192.168.0.100"
#define VERSION                 "0.0.21"

/************************************/
/* Defaults _you_ may want to tweak */
/************************************/

typedef	unsigned char	u_int8_t;
typedef unsigned short	u_int16_t;
typedef unsigned int	u_int32_t;

/* where to find the strx-130 configuration files */
#define PID_FILE                "/var/run/stxcfgd.pid"

#define SET_MAGIC		(0x69547843)      // magic of seboss iTxC configuration
#define SET_VERSION		(1)				  // protocol version 1.
#define LISTEN_PORT     (32678)
#define SEND_PORT       (32679)

#define AUTH_MAGIC		"ALK"

/* miscellaneous defines */
#ifndef TRUE
#	undef  TRUE
#	define TRUE         (1)
#endif

#ifndef FALSE
#	undef  FALSE
#	define FALSE        (0)
#endif

#define MAX_BUF_SIZE    (20) /* max xxx.xxx.xxx.xxx-xxx\n */
#define MAX_IP_ADDR     (254)
#define DEST_BCAST_ADDR	"255.255.255.255"

typedef struct netconf_Message_t {
	u_int8_t	version;		// network type
	u_int8_t	opcode;			// opcode
	u_int16_t	secs;			// retransmission seconds
	u_int32_t	xid;			// transaction id, random number
	u_int32_t	magic;			// magic code
	u_int32_t	ciaddr;			// client's current ip address
	u_int8_t	chaddr[16];		// client hardware address
	u_int32_t	yiaddr;			// client's yielded ip address (your ip)
	u_int32_t	miaddr;			// net mask ip address
	u_int32_t	giaddr;			// gateway ip address
	u_int32_t	d1iaddr;		// dns first ip address
	u_int32_t	d2iaddr;		// dns second ip address
	u_int16_t	http_port;		// server (http) port to serve http service
	u_int16_t	https_port;		// server (https) port to serve http service
	u_int16_t	rtsp_port;		// server (rtsp) port to serve http service
	u_int16_t	reserve_port;	// server (reserve) port to serve http service
	u_int8_t	vend[64];		// vendor specific informations:w
} netconf_Message;

typedef struct TIPSET
{
	int				state;
	netconf_Message	netconf;
	netconf_Message	pre_netconf;
} IPSET, *PIPSET;

struct adminToolStruct{
	int recv_sock;
	struct sockaddr_in recv_addr;

	int send_sock;
	struct sockaddr_in send_addr;

	int dhcpon;
	IPSET ipset;
};

// opcode
enum {
	MSG_IP_SEARCH = 1,
	MSG_CAM_ACK     ,
	MSG_IP_ACK      ,
	MSG_IP_SET,
	MSG_CAM_SET,
	MSG_IPUTIL_SEARCH,
	MSG_IPUTIL_RUN,
	MSG_IP_SET_AUTH
};

// nettype
enum {
	NET_STATIC = 1,
	NET_DHCP        ,
	NET_PPPoE
};

enum {
	STATE_SEARCH = 1,
	STATE_APPLY,
	STATE_SET,
	STATE_END,
};

enum {
	ACK_OK = 0,
	ACK_FAIL_AUTH,
	ACK_FAIL_LIVE,
	ACK_FAIL_WEB,
	ACK_FAIL,
};

#endif // H_NF_ADMINTOOL
