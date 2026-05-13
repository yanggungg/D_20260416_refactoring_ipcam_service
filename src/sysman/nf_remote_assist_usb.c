/*
 * ITX Security
 *
 *  2016-07-19 Author Realizing 
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <net/if_arp.h>
#include <linux/types.h>
#include <glib.h>
#include <nf_notify.h>

#include "nf_sysman.h"
#if 0
	#include "nf_remote_assist.h"
#endif
#include "nf_remote_assist_usb.h"

#define PRINT_IP(x) ((x)&0xff>>0),(((x)&0xff00)>>8),(((x)&0xff0000)>>16),(((x)&0xff000000)>>24)

#define RASS_VNC_USB_PORT (61234)
#define FILE_PROC_NET_DEV "/proc/net/dev"
#define ETHERNET_DEV "usb0"

extern int proxy_system(const char *str, int mode, int timeout_sec);
static char g_rass_usb_vnc_pass[33] = {0,};
static char g_rass_usb_mac_addr[32] = {0,};

static guint if_get_ip(const char *dev);
static void nf_rass_usb_create_vnc_pass();
static void nf_rass_usb_th(void);

gboolean nf_rass_usb_ip_check(void)
{
	guint ip = 0;
	guchar  ip_addr[4] = {0,};

	ip = ntohl( if_get_ip(ETHERNET_DEV) );

	ip_addr[0] = (ip >> 24) & 0xff;
	ip_addr[1] = (ip >> 16) & 0xff;
	ip_addr[2] = (ip >> 8) & 0xff;
	ip_addr[3] = ip & 0xff;

	g_message("%s [%d.%d.%d.%d]", __FUNCTION__, PRINT_IP(ip));

	if( (ip_addr[0] & 0xff) == 0x0 || (ip_addr[0] & 0xff) == 0xff )
		return FALSE;

	return TRUE;
}

int nf_rass_usb_init()
{
	g_message("%s called", __FUNCTION__);

	g_thread_create((GThreadFunc)nf_rass_usb_th, 0, 0, 0);

	return TRUE;
}

static guint if_get_ip(const char *dev)
{
	int   sockfd;
	unsigned long addr = (unsigned long)-1;
	guint mac = 0;

	struct ifreq ifr;
	struct sockaddr_in *sap;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		return ((unsigned long)-1);
	}

	strncpy(ifr.ifr_name, dev, IFNAMSIZ);
	ifr.ifr_name[IFNAMSIZ-1] = '\0';
	ifr.ifr_addr.sa_family = AF_INET;

	if (ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) {
	}
	else {
		sap  = (struct sockaddr_in*)&ifr.ifr_addr;
		addr = *((unsigned long*)&sap->sin_addr);
	}
	
	if( ioctl( sockfd, SIOCGIFHWADDR, &ifr ) < 0 ) {
	}
	else {
		sprintf(g_rass_usb_mac_addr, "%02x%02x%02x%02x%02x%02x", 
				(unsigned char)ifr.ifr_hwaddr.sa_data[0],
				(unsigned char)ifr.ifr_hwaddr.sa_data[1],
				(unsigned char)ifr.ifr_hwaddr.sa_data[2],
				(unsigned char)ifr.ifr_hwaddr.sa_data[3],
				(unsigned char)ifr.ifr_hwaddr.sa_data[4],
				(unsigned char)ifr.ifr_hwaddr.sa_data[5]);
		g_message("%s mac %s", __func__, g_rass_usb_mac_addr);
	}
	close(sockfd);

	return (addr);
}

static void nf_rass_usb_create_vnc_pass()
{
	char md5hash_hex[16] = { 0, };
	char input[65] = { 0, };

	snprintf(input, sizeof(input), "%s_Qmonitor", g_rass_usb_mac_addr);

	Md5Str(md5hash_hex, input);
	sprintf(g_rass_usb_vnc_pass,"%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
					md5hash_hex[0],md5hash_hex[1],md5hash_hex[2],md5hash_hex[3],
					md5hash_hex[4],md5hash_hex[5],md5hash_hex[6],md5hash_hex[7],
					md5hash_hex[8],md5hash_hex[9],md5hash_hex[10],md5hash_hex[11],
					md5hash_hex[12],md5hash_hex[13],md5hash_hex[14],md5hash_hex[15]);
}

static void nf_rass_usb_th(void)
{
	gchar cmd[512];
	gchar *contents = NULL;
	gchar *mac_buff = NULL;
	gsize  length = 0;
	GError *error = NULL;
	static gint is_usb_teth_conneted = 0;

	g_message("%s start", __FUNCTION__);
	
	while (1)
	{
		if(!g_file_get_contents(FILE_PROC_NET_DEV, &contents, &length, &error))
		{
			g_warning("%s %s\n", __FUNCTION__,  error->message);
			g_error_free (error);
			if(contents)
				g_free(contents);
		}

		if(contents)
		{
			if(strstr(contents, ETHERNET_DEV))
			{
				if(!is_usb_teth_conneted)
				{
					memset(cmd, 0x00, sizeof(cmd));
					snprintf(cmd, sizeof(cmd), "ifconfig %s down", ETHERNET_DEV);
					
					mac_buff = nf_sysdb_get_str_nocopy("sys.info.mac");
					memset(cmd, 0x00, sizeof(cmd));
					snprintf(cmd, sizeof(cmd), "ifconfig %s hw ether %c%c:%c%c:%c%c:%c%c:%c%c:%c%c"
							, ETHERNET_DEV
							, mac_buff[0], mac_buff[1]
							, mac_buff[4], mac_buff[5]
							, mac_buff[2], mac_buff[3]
							, mac_buff[6], mac_buff[7]
							, mac_buff[8], mac_buff[9]
							, mac_buff[10], mac_buff[11]);
					proxy_system(cmd, 1, 10);

					nf_sysman_process_kill("udhcpc", ETHERNET_DEV);
					memset(cmd, 0x00, sizeof(cmd));
					snprintf(cmd, sizeof(cmd), "udhcpc -b -i %s", ETHERNET_DEV);
					proxy_system(cmd, 1, 10);

					if(nf_rass_usb_ip_check())
					{
					#if 0
						if(RASS_STATUS_OPEN == nf_notify_get_param0("pnd_rass_status"))
							nf_rass_close();
					#endif

						nf_rass_usb_create_vnc_pass();
						// Set passwd
						memset(cmd, 0x00, sizeof(cmd));
						snprintf(cmd, sizeof(cmd), "/NFDVR/x11vnc -storepasswd %s /tmp/vnc_usb_passwd", g_rass_usb_vnc_pass);
						proxy_system(cmd,1,3);
						memset(cmd, 0x00, sizeof(cmd));
						snprintf(cmd, sizeof(cmd), "/NFDVR/x11vnc -nevershared -forever -bg -rfbport %d -rfbauth /tmp/vnc_usb_passwd", RASS_VNC_USB_PORT);
						proxy_system(cmd,1,3);
						is_usb_teth_conneted = 1;
					}
				}
			}
			else
			{
				if(is_usb_teth_conneted)
				{
					proxy_system("/NFDVR/x11vnc -R stop",1,3);
					nf_sysman_process_kill("udhcpc", ETHERNET_DEV);
					is_usb_teth_conneted = 0;
				}
			}
			g_free(contents);
		}
		else
		{
			if(is_usb_teth_conneted)
			{
				proxy_system("/NFDVR/x11vnc -R stop",1,3);
				nf_sysman_process_kill("udhcpc", ETHERNET_DEV);
				is_usb_teth_conneted = 0;
			}
		}

		sleep(1);
	}
	
	g_message("%s end", __FUNCTION__);
}

