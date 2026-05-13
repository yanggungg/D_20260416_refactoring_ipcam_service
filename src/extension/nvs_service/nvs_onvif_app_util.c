
#include "onvif_common.h"
#include "nf_codec_header.h"
#include "nf_record.h"
#ifndef NVR
#include "nf_encode.h"
#endif
#include "nf_util_jpeg.h"
#include "nvs_onvif_app.h"
#include "nvs_onvif_app_util.h"
#include "../nf_onvif_server.h"
#include "nf_api_ipcam.h"
#include "nf_record.h"

extern char* nf_onvif_get_eth_str(void);
extern const char *_sysdb_cate_list[];

static gchar* size_info[CAP_RES_MAX] = {	"352x240",		//B     (0)
									"704x240",		//C     (1)
									"704x480",		//D     (2)
									"704x480P",		//E     (3)
									"352x288",		//F     (4)
									"704x288",		//G     (5)
									"704x576",		//H     (6)
									"704x576P",		//I     (7)									
									"640x480",		//J     (8)
									"720x480",		//K     (9)
									"720x576",		//L     (10)
									"800x600",		//M     (11)
									"1024x768",		//N     (12)
									"1280x1024",	//O     (13)
									"1600x1200",	//P     (14)
									"1280x720",		//Q     (15)
									"1920x1080",	//R     (16)
									"640x352",		//S     (17)
									"640x360",		//T     (18)
									"640x360I",		//U     (19)
									"1280x720I",	//V     (20)
									"1920x1280I",	//W     (21)
									"640x400",	    //X     (22)
									"800x450",	    //Y     (23)
									"1440x900",	    //Z     (24)
									"960x480",	    //a     (25)
									"960x576",	    //b     (26)
									"320x180",	    //c     (27)
									"2304x1296",	    //d     (28)
									"2048x1536",	    //e     (29)
									"2560x1440",	    //f     (30)
									"2688x1520",	    //g     (31)
									"2560x1600",	    //h     (32)
									"2560x1920",	    //i     (33)
									"2592x1920",	    //j     (34)
									"2592x1944",	    //k     (35)
									"2992x1680",	    //l     (36)
									"2880x1800",	    //m     (37)
									"3200x1800",	    //n     (38)
									"2880x2160",	    //o     (39)
									"3072x2048",	    //p     (40)
									"3200x2400",	    //q     (41)
									"3840x2160",	    //r     (42)
									"2592x1520",		//s		(43)
									"1920x1440",		//t		(44)
									"1920x1536",		//u		(45)
									"1344x1520",		//v		(46)
									"1296x1944",		//w		(47)
									"1280x1440",		//x		(48)
									"1024x1536",		//y		(49)
									"2560x1944",		//6		(50)
									"2560x1520",		//7		(51)
									"1280x1520",		//0		(52)
									"1280x1944",		//8		(53)
									"1280x2160",		//9		(54)
									"1920x2160",		//:		(55)
									"2560x2160",		//=		(56)
									"",
									"UNKNOWN",	    // error
								};

arg_resolution ipx_resolution[MAX_RESOLUTION_CNT] = {
		{ 1920, 1080, 'W' }, // 0
		{ 1920, 1080, 'R' }, // 1
		{ 1280, 720, 'Q' },  // 2
		{ 1440, 900, 'Z' },  // 3
		{ 1280, 1024, 'O' }, // 4
		{ 1024, 768, 'N' },  // 5
		{ 1280, 720, 'V' },  // 6
		{ 960, 480, 'a' },  // 8
		{ 960, 576, 'b' },   // 9
		{ 800, 600, 'M' },   // 10
		{ 800, 450, 'Y' },   // 11
		{ 704, 240, 'C' },   // 12
		{ 720, 480, 'K' },   // 13
		{ 720, 576, 'L' },   // 14
		{ 704, 288, 'G' },   // 15
		{ 704, 480, 'D' },   // 16
		{ 704, 576, 'H' },   // 17
		{ 640, 480, 'J' },   // 18
		{ 640, 400, 'X' },   // 19
		{ 640, 360, 'U' },   // 20
		{ 640, 360, 'T' },   // 21
		{ 640, 352, 'S' },   // 22
		{ 352, 288, 'F' },   // 23
		{ 352, 240, 'B' },   // 24
		{ 2304, 1296, 'd' },   // 25
		{ 2048, 1536, 'e' },   // 26
		{ 2560, 1440, 'f' },   // 27
		{ 2688, 1520, 'g' },   // 28
		{ 2560, 1600, 'h' },   // 29
		{ 2560, 1920, 'i' },   // 30
		{ 2592, 1920, 'j' },   // 31
		{ 2592, 1944, 'k' },   // 32
		{ 2992, 1680, 'l' },   // 33
		{ 2880, 1800, 'm' },   // 34
		{ 3200, 1800, 'n' },   // 35
		{ 2880, 2160, 'o' },   // 36
		{ 3072, 2048, 'p' },   // 37
		{ 3200, 2400, 'q' },   // 38
		{ 3840, 2160, 'r' },   // 39
		{ 2592, 1520, 's' },   // 40
		{ 1920, 1440, 't' },   // 41
		{ 1920, 1536, 'u' },   // 42
		{ 1344, 1520, 'v' },   // 43
		{ 1296, 1944, 'w' },   // 44
		{ 1280, 1440, 'x' },   // 45
		{ 1024, 1536, 'y' },   // 46
		{ 2560, 1944, '6' },   // 47
		{ 2560, 1520, '7' },   // 48
		{ 1280, 1520, '0' },   // 49
		{ 1280, 1944, '8' },   // 50
		{ 1280, 2160, '9' },   // 51
		{ 1920, 2160, ':' },   // 52
		{ 2560, 2160, '=' },   // 53
		{ 704, 480, 'E' },     // 54
		{ 704, 576, 'I' },     // 55
		{ 1600, 1200, 'P' },   // 56
		{ 960, 576, 'b' },     // 57
		{ 320, 180, 'c' },     // 58
};

//for ncx3
arg_2nd_resolution avail_2nd_resolution[MAX_RESOLUTION_CNT] = {
		{ 1920, 1080, "JUFB" }, /* 1920x1080 -> 640x480, 640x360, 352x288, 352x240 */
		{ 1280, 1024, "JUFB" }, /* 1280x1024 -> 640x480, 640x360, 352x288, 352x240 */
		{ 1280, 720,  "JUFB" }, /* 1280x720  -> 640x480, 640x360, 352x288, 352x240 */
		{ 1024, 768,  "JUFB" }, /* 1024x768  -> 640x480, 640x360, 352x288, 352x240 */
		{ 704,  576,  "JUFB" }, /* 704x576   -> 640x480, 640x360, 352x288, 352x240 */
		{ 704,  480,  "JUFB" }, /* 704x480   -> 640x480, 640x360, 352x288, 352x240 */
		{ 640,  480,  "JUFB" }, /* 640x480   -> 640x480, 640x360, 352x288, 352x240 */ 
		{ 640,  360,  "UFB"  }, /* 640x360   -> 640x360, 352x288, 352x240 */ 
		{ 352,  288,  "FB"   }, /* 352x288   -> 352x288, 352x240 */ 
		{ 352,  240,  "B"    }, /* 352x240   -> 352x240 */ 
		{ 0,0, "-"    }, 
		{ 0,0, "-"    }, 
		{ 0,0, "-"    }, 
		{ 0,0, "-"    }, 
		{ 0,0, "-"    }, 
		{ 0,0, "-"    }, 
		{ 0,0, "-"    }, 
		{ 0,0, "-"    }, 
		{ 0,0, "-"    }, 
		{ 0,0, "-"    }, 
		{ 0,0, "-"    }, 
		{ 0,0, "-"    }, 
		{ 0,0, "-"    }, 
		{ 0,0, "-"    }, 
		{ 0,0, "-"    }, 
};

arg_fps fps_nt[MAX_FPS] = { 
    { 'A', 30 }, { 'B', 15 }, { 'C', 7 },
    { 'D', 3 }, { 'E', 2 }, { 'F', 1 }, { 'G', 0 } 
};

arg_fps fps_pal[MAX_FPS] = {
		{ 'A', 25 }, { 'B', 12 }, { 'C', 6 },
		{ 'D', 3 }, { 'E', 2 }, { 'F', 1 }, { 'G', 0 }
};

arg_qual quality_table[MAX_QUALITY] = {
		{ 'A', 5 }, { 'B', 4 }, { 'C', 3 },
		{ 'D', 2 }, { 'E', 1 }, { 'F', 0 }
};

#define VALIDATE_RECSET_ONVIF(size, fps) do { \
  if( !validate_recset(size, fps) ) { \
    tmp->result = ONVIF_R_ERR_INVALID_PARAM; \
    return;\
  } \
} while(0)

void sysdb_save_cate(unsigned int cate) {
	nf_sysdb_lock(cate);
	nf_sysdb_save(_sysdb_cate_list[cate]);
	nf_notify_fire_params("sysdb_change", cate, 0, 0, 0);
	nf_sysdb_unlock(cate);
} 

int get_ipaddress_p(char *tmp_ip, unsigned int size)
{
	struct ifreq *ifr;
	struct sockaddr_in *sin;
	struct ifconf ifcfg;
	int fd;
	int n;
	int numreqs = 30;
	char *eth_str = nf_onvif_get_eth_str();

	if(!tmp_ip) {
		return -1;
	}

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(fd < 0) {
		return -1;
	}

	memset(&ifcfg, 0, sizeof(ifcfg));
	ifcfg.ifc_buf = NULL;
	ifcfg.ifc_len = sizeof(struct ifreq) * numreqs;
	ifcfg.ifc_buf = malloc(ifcfg.ifc_len);
	if(!ifcfg.ifc_buf) {
		close(fd);
		return -1;
	}

	if(ioctl(fd, SIOCGIFCONF, (char *)&ifcfg) < 0)
	{
		close(fd);
		if(ifcfg.ifc_buf) {
			free(ifcfg.ifc_buf);
			ifcfg.ifc_buf = NULL;
		}
		return -1;
	}

	ifr = ifcfg.ifc_req;
	for(n=0;n<ifcfg.ifc_len;n+=sizeof(struct ifreq))
	{
		if(!strcmp(ifr->ifr_name, eth_str))
		{
			sin = (struct sockaddr_in *)&ifr->ifr_addr;
			//strcpy(tmp_ip, inet_ntoa(sin->sin_addr));
			inet_ntop(AF_INET, &sin->sin_addr, tmp_ip, size);
			break;
		}
		ifr++;
	}

	if(ifcfg.ifc_buf) {
		free(ifcfg.ifc_buf);
		ifcfg.ifc_buf = NULL;
	}

	close(fd);

	return 0;
}

unsigned int get_subnet_prefix()
{
	unsigned int i, subnet, prefix_length = 0;
	if ( nf_sysdb_get_bool("net.proto.dhcpon") )
		subnet = nf_sysdb_get_uint("net.dhcp.subnet");
	else
		subnet = nf_sysdb_get_uint("net.proto.subnet");
	for(i=0; i<32; i++){
		if( subnet&(1<<i) ){
			prefix_length++;
		}
	}
	return prefix_length;
}

unsigned int get_subnet_prefix_from_subnet(unsigned int subnet)
{
	unsigned int i, prefix_length = 0;

	for(i=0; i<32; i++){
		if( subnet&(1<<i) ){
			prefix_length++;
		}
	}
	return prefix_length;
}


unsigned int set_subnet_prefix(unsigned int prefix)
{
    unsigned int subnet=0, i;

    if( prefix == 32 ){
        return -1;
    }

    for(i=0; i<prefix; i++){
        subnet |= (1<<(31-i));
    }

    return subnet;
//    nf_sysdb_set_uint("net.proto.subnet", subnet);
}
int load_current_net_conf(IPSetupData *ipdata)
{
    ipdata->dhcp = nf_sysdb_get_bool("net.proto.dhcpon");
    ipdata->webServ = nf_sysdb_get_bool("net.proto.webon");
    prvIntToIP(ipdata->ip, nf_sysdb_get_uint("net.proto.ipaddr"));
    prvIntToIP(ipdata->gateway, nf_sysdb_get_uint("net.proto.gateway"));
    prvIntToIP(ipdata->subnet, nf_sysdb_get_uint("net.proto.subnet"));
    prvIntToIP(ipdata->dns1, nf_sysdb_get_uint("net.proto.dns1"));
    prvIntToIP(ipdata->dns2, nf_sysdb_get_uint("net.proto.dns2"));
    ipdata->netPort = nf_sysdb_get_uint("net.proto.clientport");
    ipdata->webPort = nf_sysdb_get_uint("net.proto.webport");
    ipdata->txSpeed = nf_sysdb_get_uint("net.proto.maxtxspeed");
#ifdef _UPNP_SUPPORT_
    ipdata->rtspport = nf_sysdb_get_uint("net.rtp.rtspport");
#endif		
    return 1;
}
int convert_prefix_to_netmask(unsigned int prefix, guint *netmask)
{
    unsigned int subnet = set_subnet_prefix(prefix);

    netmask[0] = (subnet >> 24) & 0xff;
    netmask[1] = (subnet >> 16) & 0xff;
    netmask[2] = (subnet >> 8) & 0xff;
    netmask[3] = subnet & 0xff;
}

void prvIntToIP(guint *out, guint in)
{
	out[0] = (in >> 24) & 255;
	out[1] = (in >> 16) & 255;
	out[2] = (in >> 8) & 255;
	out[3] = in & 255;
}

guint prvIPToInt(guint ip[4])
{
	guint ret = 0;

	ret += (ip[0] << 24);
	ret += (ip[1] << 16);
	ret += (ip[2] << 8);
	ret += ip[3];

	return ret;
}

int get_ipaddress(char *tmp_ip)
{
	struct ifreq *ifr;
	struct sockaddr_in *sin;
	struct ifconf ifcfg;
	int fd;
	int n;
	int numreqs = 30;
	char *eth_str = nf_onvif_get_eth_str();

	if (!tmp_ip)
	{
		syslog(LOG_ERR, "%s arg tmp_ip is NULL\n", __FUNCTION__);
		return -1;
	}

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
	{
		syslog(LOG_ERR, "%s socket error\n", __FUNCTION__);
		return -1;
	}

	memset(&ifcfg, 0, sizeof(ifcfg));
	ifcfg.ifc_buf = NULL;
	ifcfg.ifc_len = sizeof(struct ifreq) * numreqs;
    ifcfg.ifc_buf = malloc(ifcfg.ifc_len);
	if (!ifcfg.ifc_buf)
	{
		close(fd);
		syslog(LOG_ERR, "%s malloc error\n", __FUNCTION__);
		return -1;
	}

	if (ioctl(fd, SIOCGIFCONF, (char *)&ifcfg) < 0)
	{
		close(fd);
		syslog(LOG_ERR, "%s ioctol error\n", __FUNCTION__);
        free(ifcfg.ifc_buf);
		return -1;
	}

	ifr = ifcfg.ifc_req;
	for (n = 0;n < ifcfg.ifc_len;n += sizeof(struct ifreq))
	{
        sin = (struct sockaddr_in *)&ifr->ifr_addr;
        if (!strncmp(ifr->ifr_name, eth_str, strlen(eth_str)))
		{
			sin = (struct sockaddr_in *) & ifr->ifr_addr;
            // inet_ntoa is non-reentrant.
            //strcpy(tmp_ip, inet_ntoa(sin->sin_addr));
            inet_ntop(AF_INET, &sin->sin_addr, tmp_ip, sizeof(char)*16);
			break;
		}
		ifr++;
	}

	if (ifcfg.ifc_buf)
	{
		free(ifcfg.ifc_buf);
		ifcfg.ifc_buf = NULL;
	}

	close(fd);

	return 0;
}

int getResolutionIndex(char char_tmp) {
	int i;
	for (i = 0; i < MAX_RESOLUTION_CNT; i++) {
		if (char_tmp == ipx_resolution[i].resolutionString) {
			return i;
		}
	}
	return -1;
}

void get_video_size(char char_tmp, int *width, int *height) {
	int num_tmp = getResolutionIndex(char_tmp);
	if (num_tmp >= 0 && num_tmp < MAX_RESOLUTION_CNT) {
		*width = ipx_resolution[num_tmp].width;
		*height = ipx_resolution[num_tmp].height;
	} else {
		*width = 0;
		*height = 0;
	}
}

int get_fps_from_alphabet(char ch)
{
    int i;
	
    if (DISPLAY_IS_PAL) {
        for (i = 0; i < MAX_FPS ; i++) {
        	_TTY_LOG_ONVIF_DEBUG("fps_pal[i].fps: %d ch : %d)", fps_pal[i].fps, ch);
            if (fps_pal[i].fpsCh == ch) {
                return fps_pal[i].fps;
            }
        }
    }
    else {
        for (i = 0; i < MAX_FPS ; i++) {
        	_TTY_LOG_ONVIF_DEBUG("fps_nt[i].fps: %d ch : %d)", fps_nt[i].fps, ch);
            if (fps_nt[i].fpsCh == ch) {
                return fps_nt[i].fps;
            }
        }
    }
    return -1;
}


int get_quality_from_alphabet(char ch)
{
	int i;
	for(i=0; i< MAX_QUALITY ; i++) {
		if(quality_table[i].qualCh == ch) {
			return quality_table[i].qual;
		}
	}
	return -1;
}


void set_vsource_table(arg_VideoSource *tmp)
{
	char tmp_token[COMMON_SIZE] = {0,};
	char buff[COMMON_SIZE] = {0,};
	int i;
	int vsource_cnt = ONVIF_VSOURCE_CNT;

	for(i=0; i<vsource_cnt; i++)
	{
		sprintf(buff, "onvif.vsource%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if(!strcmp(tmp->token, tmp_token))
		{
			//Do someting
			if(tmp->width != ONVIF_VSOURCE_WIDTH_MAX || tmp->height != ONVIF_VSOURCE_HEIGHT_MAX || tmp->x != 0 || tmp->y != 0) {
				tmp->result = -1;
				return;
			}

			sprintf(buff, "onvif.vsource%d.bound_width", i);
			nf_sysdb_set_uint(buff, tmp->width);
			sprintf(buff, "onvif.vsource%d.bound_height", i);
			nf_sysdb_set_uint(buff, tmp->height);

			sprintf(buff, "onvif.vsource%d.bound_x", i);
			nf_sysdb_set_uint(buff, tmp->x);
			sprintf(buff, "onvif.vsource%d.bound_y", i);
			nf_sysdb_set_uint(buff, tmp->y);

			sprintf(buff, "onvif.vsource%d.name", i);
			nf_sysdb_set_str(buff, tmp->name);

			if(tmp->save_flag == 1) {
				nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
				sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
			}

			tmp->result = 1;
			break;
		}
	}

	if(i == vsource_cnt)
{
		tmp->result = 0;
		fprintf(stderr, "%s Function %d(line) Token Not Found ERROR\n", __FUNCTION__, __LINE__);
	}
}


gint get_onvif_vencoder_codec(gint codec)
{
	gint onvif_codec = ONVIF_VIDEO_CODEC_H264;

	switch (codec){
		case NF_VIDEO_CODEC_H264:
			onvif_codec = ONVIF_VIDEO_CODEC_H264;
			break;
		case NF_VIDEO_CODEC_JPEG:
			onvif_codec = ONVIF_VIDEO_CODEC_JPEG;
			break;
		case NF_VIDEO_CODEC_H265:
			onvif_codec = ONVIF_VIDEO_CODEC_H265;
			break;
		default:
			onvif_codec = ONVIF_VIDEO_CODEC_H264;
			break;
	}

	return onvif_codec;
}

gint get_onvif_vencoder_h264_profile(gint profile)
{
	return ONVIF_H264_PROFILE_HIGH;
}

int convert_quality_to_max_bitrate(int quality)
{
	switch(quality)
	{
	case NF_QUALITY_SUPER:		return 20000;
	case NF_QUALITY_HIGHEST:	return 15000;
	case NF_QUALITY_HIGH:		return 10000;
	case NF_QUALITY_STANDARD:	return 8000;
	case NF_QUALITY_LOW:		return 3000;
	}
	return 0;
}
char *convert_max_bitrate_to_quality(int bitrate)
{
	if(bitrate > 15000)			return QUAL_DATA_HIGHEST;
	else if(bitrate > 10000)	return QUAL_DATA_HIGH;
	else if(bitrate > 8000)		return QUAL_DATA_STANDARD;
	else if(bitrate > 3000)		return QUAL_DATA_LOW;

	return QUAL_DATA_LOWEST;
}
void ext_get_img_size(guint64 res, guint *width, guint *height)
{
	onvif_get_width_height(res, width, height);

}
void onvif_get_width_height(guint64 resol, int* w, int* h)
{
	if(w == NULL || h == NULL) return;

	switch(resol)
	{
		case NF_RES_NTSC_NONE:
			*w = 0;   *h = 0;		break;
		case NF_RES_NTSC_2CIF:
			*w = 704; *h = 240; 	break;
		case NF_RES_NTSC_4CIFP:
			*w = 704; *h = 480;		break;
		case NF_RES_PAL_2CIF:
			*w = 704; *h = 288;		break;
		case NF_RES_PAL_4CIFP:
			*w = 704; *h = 576;		break;
		case NF_RES_960H_NTSC_4CIFP:
			*w = 960; *h = 480; 	break;
		case NF_RES_960H_PAL_4CIFP:
			*w = 960; *h = 576; 	break;
		case NF_RES_2592x1520:
			*w = 2592;*h = 1520;	break;
		case NF_RES_1920x1440:
			*w = 1920;*h = 1440;	break;
		case NF_RES_1920x1536:
			*w = 1920;*h = 1536;	break;
		case NF_RES_1344x1520:
			*w = 1344;*h = 1520;	break;
		case NF_RES_1296x1944:
			*w = 1296;*h = 1944;	break;
		case NF_RES_1280x1440:
			*w = 1280;*h = 1440;	break;
		case NF_RES_1024x1536:
			*w = 1024;*h = 1536;	break;
		case NF_RES_2560x1944:
			*w = 2560;*h = 1944;	break;
		case NF_RES_1280x1520:
			*w = 1280;*h = 1520;	break;
		case NF_RES_1280x1944:
			*w = 1280;*h = 1944;	break;
		case NF_RES_1280x2160:
			*w = 1280;*h = 2160;	break;
		case NF_RES_1920x2160:
			*w = 1920;*h = 2160;	break;
		case NF_RES_2560x2160:
			*w = 2560;*h = 2160;	break;
		case NF_RES_1280x960:
			*w = 1280;*h = 960;		break;
		case NF_RES_3000x3000:
			*w = 3000;*h = 3000;	break;
		case NF_RES_2048x2048:
			*w = 2048;*h = 2048;	break;
		case NF_RES_1280x1280:
			*w = 1280;*h = 1280;	break;
		case NF_RES_640x640:
			*w = 640;*h = 640;		break;
		case NF_RES_320x320:
			*w = 320;*h = 320;		break;
		case NF_RES_2560x1520:
			*w = 2560;*h = 1520;	break;
		case NF_RES_1920x1080:
		case NF_RES_1920x1080I:
			*w = 1920; *h = 1080;	break;
		case NF_RES_1280x720:
		case NF_RES_1280x720I:
			*w = 1280; *h = 720;	break;
		case NF_RES_1600x1200:
			*w = 1600; *h = 1200;	break;
		case NF_RES_800x600:
			*w = 800; *h = 600;		break;
		case NF_RES_1440x900:
			*w = 1440; *h = 900;	break;
		case NF_RES_800x450:
			*w = 800; *h = 450;		break;
		case NF_RES_640x400:
			*w = 640; *h = 400;		break;
		case NF_RES_640x360:
		case NF_RES_640x360I:
			*w = 640; *h = 360;		break;
		case NF_RES_320x180:
			*w = 320; *h = 180;		break;
		case NF_RES_1280x1024:
			*w = 1280; *h = 1024;	break;
		case NF_RES_1024x768:
			*w = 1024; *h = 768;	break;
		case NF_RES_720x576:
			*w = 720; *h = 576;		break;
		case NF_RES_720x480:
			*w = 720; *h = 480;		break;
		case NF_IPCAM_RES_704x576:
			*w = 704; *h = 576;		break;
		case NF_IPCAM_RES_704x480:
			*w = 704; *h = 480;		break;
		case NF_RES_640x480:
			*w = 640; *h = 480;		break;
		case NF_RES_640x352:
			*w = 640; *h = 352;		break;
		case NF_RES_PAL_CIF:
			*w = 352; *h = 288;		break;
		case NF_RES_NTSC_CIF:
			*w = 352; *h = 240;		break;
		case NF_RES_2304x1296:
			*w = 2304; *h = 2396;	break;
		case NF_RES_2048x1536:
			*w = 2048; *h = 1536;	break;
		case NF_RES_2560x1440:
			*w = 2560; *h = 1440;	break;
		case NF_RES_2688x1520:
			*w = 2688; *h = 1520;	break;
		case NF_RES_2560x1600:
			*w = 2560; *h = 1600;	break;
		case NF_RES_2560x1920:
			*w = 2560; *h = 1920;	break;
		case NF_RES_2592x1920:
			*w = 2592; *h = 1920;	break;
		case NF_RES_2592x1944:
			*w = 2592; *h = 1944;	break;
		case NF_RES_2992x1680:
			*w = 2992; *h = 1680;	break;
		case NF_RES_2880x1800:
			*w = 2880; *h = 1800;	break;
		case NF_RES_3200x1800:
			*w = 3200; *h = 1800;	break;
		case NF_RES_2880x2160:
			*w = 2880; *h = 2160;	break;
		case NF_RES_3072x2048:
			*w = 3072; *h = 2048;	break;
		case NF_RES_3200x2400:
			*w = 3200; *h = 2400;	break;
		case NF_RES_3840x2160:
			*w = 3840; *h = 2160;	break;
		case NF_IPCAM_RES_2592x1520:
			*w = 2592; *h = 1520;	break;
		case NF_IPCAM_RES_3000x3000:
			*w = 3000; *h = 3000;	break;
		case NF_IPCAM_RES_2048x2048:
			*w = 2048; *h = 2048;	break;
		case NF_IPCAM_RES_1280x1280:
			*w = 1280; *h = 1280;	break;
		case NF_IPCAM_RES_640x640:
			*w = 640;  *h = 640;	break;
		case NF_IPCAM_RES_320x320:
			*w = 320;  *h = 320;	break;
		default:
			*w = 0; *h = 0;			break;
	}

}
void get_vencoder_table(arg_VideoEncoder *tmp) {
	char tmp_token[COMMON_SIZE] = { 0, };
	char tmp_resolution[17] = { 0, }, tmp_fps[17] = { 0, }, tmp_quality[17] = { 0, };
	char buff[COMMON_SIZE];
	int vencoder_cnt = ONVIF_VENCODER_CNT;
	int codec;
	int width, height;
	int bitrate;
	int req_fps;
	int i;

	NFIPCamEncoderCap encoder_cap;
	int result;

#if 1
	//for utm2
	int ch = 0;
	int stream = 0;
	int init_here = 0;
	for (i = 0; i < vencoder_cnt; i++) {
		sprintf(buff, "onvif.vencoder%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);


		ch = i%ONVIF_CH;
		stream = i/ONVIF_CH;

		result = nf_ipcam_get_encoder_capability(ch, &encoder_cap);
		if (result == 0) {
			tmp->result = ONVIF_ERR_RET_INTERNAL;
			return;
		}


		if (!strcmp(tmp->token, tmp_token)) {
			/* name */
			sprintf(buff, "cam.C%d.title", ch);
			strncpy(tmp->name, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			/* codec : fixed */
			// sprintf(buff,"onvif.vencoder%d.encoding", i);

			memset(buff, 0x00, sizeof(char)*COMMON_SIZE);
			sprintf(buff, "cam.C%d.stream.S%d.vcodec", ch, stream);


			if (strncmp(nf_sysdb_get_str_nocopy(buff), "H.264", 5) == 0)
				codec = ONVIF_VIDEO_CODEC_H264;
			else if (strncmp(nf_sysdb_get_str_nocopy(buff), "H.265", 5) == 0)
				codec = ONVIF_VIDEO_CODEC_H265;
			else
				codec = ONVIF_VIDEO_CODEC_H264;

			tmp->codec = codec;


			tmp->h264_profile = ONVIF_H264_PROFILE_BASELINE;

			/* quality : fixed */
			tmp->quality = 100; //_convert_sysdb_quality(_get_sysdb_strmap4("rec.continuous.C%d.quality", i%ONVIF_CH));
				
			/* max bitrate */
			tmp->bitrate = encoder_cap.bitrate_max[stream];
			
			/* default bitrate */
			if (tmp->bitrate == 0) {
				if (stream == 0)
					tmp->bitrate = 512;
				else if (stream == 1)
					tmp->bitrate = 64;
			}

			/* gov length : gop length is fixed*/
			sprintf(buff, "onvif.vencoder%d.h264_gov", i);
			tmp->govlength = nf_sysdb_get_uint(buff);

			get_mcast_addr(&tmp->mcast, i,"onvif.vencoder");
			sprintf(buff, "onvif.vencoder%d.mcast_port", i);
			tmp->mcast.port = nf_sysdb_get_uint(buff);
			sprintf(buff, "onvif.vencoder%d.timeout", i);
			tmp->timeout = nf_sysdb_get_uint(buff);
			tmp->use_count = get_vencoder_usecount(tmp->token);

			// /* main */
			// if(stream == 0) 
			// {
		#ifdef NVR
				char str_resol_cur[6] = {0,};
    			resol_bitmask_to_char(encoder_cap.res_default[stream], DISPLAY_IS_PAL, str_resol_cur);
				// Default Resolution
				if (str_resol_cur[0] == '\0') {
					sprintf(buff, "cam.C%d.stream.S%d.size", ch, stream);
					snprintf(str_resol_cur, sizeof(str_resol_cur), "%s", nf_sysdb_get_str_nocopy(buff));
				}

				gint s_res = _convert_sysdb_resolution(str_resol_cur[0]);
				ext_get_img_size(s_res, &width, &height);
		#else
				/* resolution */
				guint res = _convert_sysdb_resolution(continuous_record_size[24*ch]);
				ext_get_img_size(res, &width, &height);
		#endif
				tmp->width = width;
				tmp->height = height;

				/* fps */
				snprintf(buff, sizeof(buff), "cam.C%d.stream.S%d.fps", ch, stream);
				tmp->fps_limit = nf_sysdb_get_uint(buff);

				/* default fps */
				if (tmp->fps_limit == 0) {
					if (stream == 0)
						tmp->fps_limit = 30;
					else if (stream == 1)
						tmp->fps_limit = 15;
				}
				tmp->fps_interval = 1; //TBD
			// }
			
			/* second : fixed spec */
		// 	else
		// 	{
		// #ifdef NVR
		// 	/* resolution */
		// 		get_cam_capa_resol(ch, resol, stream);
		// 		//printf("\e[31m ############ ch[%d] stream [%d] S_RESOL[%s] ######### \e[0m\n", ch, stream, resol);
		// 		guint s_res = _convert_sysdb_resolution(resol[0]);
		// 		ext_get_img_size(s_res, &width, &height);
		// 		//printf("\e[31m ########## width[%d] height[%d] ############ \e[0m\n", width, height);
		// 		if(width == 0 && height == 0)
		// 		{
		// 			width = 640;
		// 			height = 360;
		// 		}
				
		// 		tmp->width = width;
		// 		tmp->height = height;

		// 		/* fps */
		// 		tmp->fps_limit = 5; //TBD
		// 		tmp->fps_interval = 1; //TBD
				
		// #else
		// 		/* resolution */
		// 		tmp->width = 352;
		// 		tmp->height = 240;

		// 		/* fps */
		// 		tmp->fps_limit = 5; //TBD
		// 		tmp->fps_interval = 1; //TBD
		// #endif
		// 	}
			tmp->result = 1;
			break;
		}
	}
#else
	for (i = 0; i < vencoder_cnt; i++) {
		sprintf(buff, "onvif.vencoder%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp->token, tmp_token)) {
			/* name */
			sprintf(buff, SYSDB_REC_VIDEO_NAME, i%ONVIF_CH, i/ONVIF_CH);
			strncpy(tmp->name, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			/* codec */
			codec = ext_get_manual_codec(i%ONVIF_CH,  i/ONVIF_CH);
			tmp->codec = get_onvif_vencoder_codec(codec);

			/* h264_profile */
			tmp->h264_profile = get_onvif_vencoder_h264_profile(codec);			
			
			/* resolution */
			guint res = ext_get_manual_resolution(i%ONVIF_CH, i/ONVIF_CH);
			ext_get_img_size(res, &width, &height);			
			tmp->width = width;
			tmp->height = height;
            
			/* fps */
			tmp->fps_limit = ext_get_manual_fps_limit(i%ONVIF_CH, i/ONVIF_CH);
			sprintf(buff, SYSDB_REC_VIDEO_ENCODE_INTERVAL, i%ONVIF_CH, i/ONVIF_CH);			
			tmp->fps_interval = nf_sysdb_get_uint(buff);			

			/* quality */
			memset(buff, 0x00, sizeof(char)*COMMON_SIZE);
			snprintf(buff, sizeof(char)*COMMON_SIZE-1, SYSDB_REC_VIDEO_QUALITY,  i%ONVIF_CH, i/ONVIF_CH);
			tmp->quality = nf_sysdb_get_uint(buff);

			/* bitrate */
			memset(buff, 0x00, sizeof(char)*COMMON_SIZE);
			snprintf(buff, sizeof(char)*COMMON_SIZE-1, SYSDB_REC_VIDEO_BITRATE,  i%ONVIF_CH, i/ONVIF_CH);
			tmp->bitrate = nf_sysdb_get_uint(buff);
			
			/* gov length : gop length is fixed*/
			memset(buff, 0x00, sizeof(char)*COMMON_SIZE);
			snprintf(buff, sizeof(char)*COMMON_SIZE-1, SYSDB_REC_VIDEO_GOP,  i%ONVIF_CH, i/ONVIF_CH);
			tmp->govlength= nf_sysdb_get_uint(buff);

			get_mcast_addr(&tmp->mcast, i,"onvif.vencoder");
			sprintf(buff, "onvif.vencoder%d.mcast_port", i);
			tmp->mcast.port = nf_sysdb_get_uint(buff);
			sprintf(buff, "onvif.vencoder%d.timeout", i);
			tmp->timeout = nf_sysdb_get_uint(buff);
			tmp->use_count = get_vencoder_usecount(tmp->token);
			
			tmp->result = 1;
			break;
		}

	}

	if (i == vencoder_cnt) {
		tmp->result = 0;
		fprintf(stderr, "%s Function %d(line) Token Not Found ERROR\n",
				__FUNCTION__, __LINE__);
	}
#endif
}

static guint ipStringToUint(char* ipstring) {
  struct sockaddr_in  addr;
  guint ret;

  memset(&addr, 0x00, sizeof(struct sockaddr_in) );
  inet_aton(ipstring, &(addr.sin_addr));
  ret = (guint)addr.sin_addr.s_addr;
  ret = ntohl(ret);

  return ret;
}

int set_mcast_addr(arg_McastAddress *tmp, int ch, int port_type)
{
    char buff[64] = {0,};

    memset(buff, 0x00, sizeof(char)*64);

    switch( port_type ){
        case VIDEO_MCAST_TYPE : 
            sprintf(buff, "onvif.vencoder%d.mcast_auto", ch);
            nf_sysdb_set_bool(buff, tmp->auto_start);
            sprintf(buff, "onvif.vencoder%d.mcast_ip4addr", ch);
            nf_sysdb_set_uint(buff, tmp->ipaddr);            
            sprintf(buff, "onvif.vencoder%d.mcast_iptype", ch);
            nf_sysdb_set_uint(buff, tmp->iptype);
            sprintf(buff, "onvif.vencoder%d.mcast_port", ch);
            nf_sysdb_set_uint(buff, tmp->port);
            sprintf(buff, "onvif.vencoder%d.mcast_ttl", ch);
            nf_sysdb_set_uint(buff, tmp->ttl);
            break;
        case AUDIO_MCAST_TYPE : 
            sprintf(buff, "onvif.aencoder%d.mcast_auto", ch);
            nf_sysdb_set_bool(buff, tmp->auto_start);
            sprintf(buff, "onvif.aencoder%d.mcast_ip4addr", ch);
            nf_sysdb_set_uint(buff, tmp->ipaddr);            
            sprintf(buff, "onvif.aencoder%d.mcast_iptype", ch);
            nf_sysdb_set_uint(buff, tmp->iptype);
            sprintf(buff, "onvif.aencoder%d.mcast_port", ch);
            nf_sysdb_set_uint(buff, tmp->port);
            sprintf(buff, "onvif.aencoder%d.mcast_ttl", ch);
            nf_sysdb_set_uint(buff, tmp->ttl);
            break;
        case METADATA_MCAST_TYPE : 
            sprintf(buff, "onvif.metadata%d.mcast_auto", ch);
            nf_sysdb_set_bool(buff, tmp->auto_start);
            sprintf(buff, "onvif.metadata%d.mcast_ip4addr", ch);
            nf_sysdb_set_uint(buff, tmp->ipaddr);            
            sprintf(buff, "onvif.metadata%d.mcast_iptype", ch);
            nf_sysdb_set_uint(buff, tmp->iptype);
            sprintf(buff, "onvif.metadata%d.mcast_port", ch);
            nf_sysdb_set_uint(buff, tmp->port);
            sprintf(buff, "onvif.metadata%d.mcast_ttl", ch);
            nf_sysdb_set_uint(buff, tmp->ttl);            
            break;          
        default:
            break;
    }

    return 1;
}

void set_mcast_addr_from_rtp(arg_McastAddress *tmp, int ch, int stream, int track_type)
{
	char buff[COMMON_SIZE];
	memset(buff, 0x00, sizeof(buff));

	tmp->auto_start = 0; 

	if ( tmp->iptype == ONVIF_IPV4 )
	{
		if ( track_type == ONVIF_MULTICAST_VIDEO )
		{
			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "net.rtp.multi.S%d.C%d.vport", stream, ch);
			nf_sysdb_set_uint(buff, tmp->port);

			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "net.rtp.multi.S%d.C%d.vttl", stream, ch);
			nf_sysdb_set_uint(buff, tmp->ttl);

			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "net.rtp.multi.S%d.C%d.vaddr", stream, ch);
		}
		else if ( track_type == ONVIF_MULTICAST_AUDIO )
		{
			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "net.rtp.multi.S%d.C%d.aport", stream, ch);
			nf_sysdb_set_uint(buff, tmp->port);

			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "net.rtp.multi.S%d.C%d.attl", stream, ch);
			nf_sysdb_set_uint(buff, tmp->ttl);

			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "net.rtp.multi.S%d.C%d.aaddr", stream, ch);
		}
		else if ( track_type == ONVIF_MULTICAST_META )
		{
			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "net.rtp.multi.S%d.C%d.mport", stream, ch);
			nf_sysdb_set_uint(buff, tmp->port);

			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "net.rtp.multi.S%d.C%d.mttl", stream, ch);
			nf_sysdb_set_uint(buff, tmp->ttl);

			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "net.rtp.multi.S%d.C%d.maddr", stream, ch);
		}
		else
			return ;

		nf_sysdb_set_uint(buff, ipStringToUint(tmp->ipaddrv4));
	}
	else if ( tmp->iptype == ONVIF_IPV6 )
	{
		if ( track_type == ONVIF_MULTICAST_VIDEO )
		{
			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "net.rtp.multi.S%d.C%d.ipv6.vport", stream, ch);
			nf_sysdb_set_uint(buff, tmp->port);

			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "net.rtp.multi.S%d.C%d.ipv6.vttl", stream, ch);
			nf_sysdb_set_uint(buff, tmp->ttl);

			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "net.rtp.multi.S%d.C%d.ipv6.vaddr", stream, ch);
		}
		else if ( track_type == ONVIF_MULTICAST_AUDIO )
		{
			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "net.rtp.multi.S%d.C%d.ipv6.aport", stream, ch);
			nf_sysdb_set_uint(buff, tmp->port);

			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "net.rtp.multi.S%d.C%d.ipv6.attl", stream, ch);
			nf_sysdb_set_uint(buff, tmp->ttl);

			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "net.rtp.multi.S%d.C%d.ipv6.aaddr", stream, ch);
		}
		else if ( track_type == ONVIF_MULTICAST_META )
		{
			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "net.rtp.multi.S%d.C%d.ipv6.mport", stream, ch);
			nf_sysdb_set_uint(buff, tmp->port);

			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "net.rtp.multi.S%d.C%d.ipv6.mttl", stream, ch);
			nf_sysdb_set_uint(buff, tmp->ttl);

			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "net.rtp.multi.S%d.C%d.ipv6.maddr", stream, ch);
		}
		else
			return ;

		nf_sysdb_set_str(buff, tmp->ipaddrv6);
	}
	else
		return ;
}

void get_mcast_addr(arg_McastAddress *tmp, int index, char* encoderField) 
{

	char buff[COMMON_SIZE];

	tmp->auto_start = 0;
	tmp->iptype = ONVIF_IPV4;
	sprintf(buff, "%s%d.mcast_ip4addr",encoderField, index);
	tmp->ipaddr = nf_sysdb_get_uint(buff);
	sprintf(buff, "%s%d.mcast_ttl", encoderField,index);
	tmp->ttl = nf_sysdb_get_uint(buff);

	if( !(tmp->ipaddr & 0x1FFFFFFF) ) {	// 224.0.0.0
		// FIX ME::
//		nf_netif_create_unique_ip(0xE0000000, 0xF0000000, index, &tmp->ipaddr);
	}

	sprintf(buff, "%s%d.mcast_auto", encoderField,index);
	tmp->auto_start = nf_sysdb_get_bool(buff);


}

void get_mcast_addr_from_rtp(arg_McastAddress *tmp, int ch, int stream, int track_type)
{
	unsigned int tmpIpaddr = 0;
	char buff[COMMON_SIZE];
	memset(buff, 0x00, sizeof(buff));

	tmp->auto_start = 0;
	
	if ( tmp->iptype == ONVIF_IPV4 )
	{
		if ( track_type == ONVIF_MULTICAST_VIDEO )
		{
			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "net.rtp.multi.S%d.C%d.vport", stream, ch);
			tmp->port = nf_sysdb_get_uint(buff);

			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "net.rtp.multi.S%d.C%d.vttl", stream, ch);
			tmp->ttl= nf_sysdb_get_uint(buff);

			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "net.rtp.multi.S%d.C%d.vaddr", stream, ch);
		}
		else if ( track_type == ONVIF_MULTICAST_AUDIO )
		{
			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "net.rtp.multi.S%d.C%d.aport", stream, ch);
			tmp->port = nf_sysdb_get_uint(buff);

			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "net.rtp.multi.S%d.C%d.attl", stream, ch);
			tmp->ttl= nf_sysdb_get_uint(buff);

			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "net.rtp.multi.S%d.C%d.aaddr", stream, ch);
		}
		else if ( track_type == ONVIF_MULTICAST_META )
		{
			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "net.rtp.multi.S%d.C%d.mport", stream, ch);
			tmp->port = nf_sysdb_get_uint(buff);

			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "net.rtp.multi.S%d.C%d.mttl", stream, ch);
			tmp->ttl= nf_sysdb_get_uint(buff);

			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "net.rtp.multi.S%d.C%d.maddr", stream, ch);
		}
		else
			return ;

		tmpIpaddr = nf_sysdb_get_uint(buff);
		snprintf(tmp->ipaddrv4, sizeof(char)*COMMON_SIZE - 1, "%d.%d.%d.%d", PRINT_IP(tmpIpaddr));
		
	}
	else if ( tmp->iptype == ONVIF_IPV6 )
	{
		if ( track_type == ONVIF_MULTICAST_VIDEO )
		{
			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "net.rtp.multi.S%d.C%d.ipv6.vport", stream, ch);
			tmp->port = nf_sysdb_get_uint(buff);

			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "net.rtp.multi.S%d.C%d.ipv6.vttl", stream, ch);
			tmp->ttl= nf_sysdb_get_uint(buff);

			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "net.rtp.multi.S%d.C%d.ipv6.vaddr", stream, ch);
		}
		else if ( track_type == ONVIF_MULTICAST_AUDIO )
		{
			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "net.rtp.multi.S%d.C%d.ipv6.aport", stream, ch);
			tmp->port = nf_sysdb_get_uint(buff);

			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "net.rtp.multi.S%d.C%d.ipv6.attl", stream, ch);
			tmp->ttl= nf_sysdb_get_uint(buff);

			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "net.rtp.multi.S%d.C%d.ipv6.aaddr", stream, ch);
		}
		else if ( track_type == ONVIF_MULTICAST_META )
		{
			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "net.rtp.multi.S%d.C%d.ipv6.mport", stream, ch);
			tmp->port = nf_sysdb_get_uint(buff);

			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "net.rtp.multi.S%d.C%d.ipv6.mttl", stream, ch);
			tmp->ttl= nf_sysdb_get_uint(buff);

			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "net.rtp.multi.S%d.C%d.ipv6.maddr", stream, ch);
		}
		else
			return ;

		strncpy(tmp->ipaddrv6, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
	}
	else
		return ;
}

int get_vencoder_usecount(char *token) {
	int profile_cnt, usecount = 0;
	int i;
	char buff[COMMON_SIZE] = { 0, };
	char tmp_token[COMMON_SIZE] = { 0, };

	profile_cnt = MAX_PROFILE;//nf_sysdb_get_uint("onvif.common.profile_cnt");

	for (i = 0; i < profile_cnt; i++) {
		//if(nf_sysdb_get_uint("camera.S1.codec") == CODEC_NONE && i == 1){   // second is NONE
		//	continue;
		//}
		sprintf(buff, "onvif.profile%d.vencoder", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(token, tmp_token))
			usecount++;
	}

	return usecount;
}
/*
void getVEncoderData(int profile_cnt, arg_VideoEncoder* encoder) {
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	int i;
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	for (i = 0; i < profile_cnt; i++) {
		sprintf(buff, "onvif.vsource%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		if (!strcmp(tmp_token, encoder->token)) {
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
			//encoder->name =
			sprintf(buff, "onvif.vencoder%d.name", i);
			strncpy(encoder->name, nf_sysdb_get_str_nocopy(buff),
					COMMON_SIZE - 1);
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
			sprintf(buff, "onvif.vencoder%d.encoding", i);
			strncpy(encoder->codec, nf_sysdb_get_uint(buff), COMMON_SIZE - 1);
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
			sprintf(buff, "onvif.vencoder%d.max_bitrate", i);
			strncpy(encoder->bitrate, nf_sysdb_get_uint(buff), COMMON_SIZE - 1);
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
			sprintf(buff, "onvif.vencoder%d.h264_gov", i);
			strncpy(encoder->govlength, nf_sysdb_get_uint(buff),
					COMMON_SIZE - 1);
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
			sprintf(buff, "onvif.vencoder%d.mcast_iptype", i);
			strncpy(encoder->mcast.iptype, nf_sysdb_get_uint(buff),
					COMMON_SIZE - 1);
			if (encoder->mcast.iptype) {
				sprintf(buff, "onvif.vencoder%d.mcast_ip6addr", i);
				strncpy(encoder->mcast.ipaddr, nf_sysdb_get_uint(buff),
						COMMON_SIZE - 1);
			} else {
				sprintf(buff, "onvif.vencoder%d.mcast_ip4addr", i);

				strncpy(encoder->mcast.ipaddr, nf_sysdb_get_uint(buff),
						COMMON_SIZE - 1);
				_TTY_LOG_ONVIF_DEBUG("encoder->mcast.ipaddr %d", __FUNCTION__, __LINE__);
			}
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
			sprintf(buff, "onvif.vencoder%d.mcast_port", i);
			strncpy(encoder->mcast.port, nf_sysdb_get_uint(buff),
					COMMON_SIZE - 1);
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
			sprintf(buff, "onvif.vencoder%d.mcast_ttl", i);
			strncpy(encoder->mcast.ttl, nf_sysdb_get_uint(buff),
					COMMON_SIZE - 1);
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
			sprintf(buff, "onvif.vencoder%d.mcast_auto", i);
			strncpy(encoder->mcast.auto_start, nf_sysdb_get_bool(buff),
					COMMON_SIZE - 1);
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
			sprintf(buff, "onvif.vencoder%d.timeout", i);
			strncpy(encoder->timeout, nf_sysdb_get_uint(buff), COMMON_SIZE - 1);

		}

	}

}
*/
int is_EnableAudio(int ch) 
{
	int audioon = 0;

	audioon = (int)nf_sysdb_get_bool("audio.enable");

	return audioon;
}

int OV_is_EnableAudio(int profile_index)
{
	int num_ch  = 0;
	char vsconfig[COMMON_SIZE] = {0,}, buff[COMMON_SIZE] = {0,};

	memset(vsconfig, 0x00, sizeof(char)*COMMON_SIZE);
	sprintf(buff, "onvif.profile%d.asource", profile_index);
	strncpy(vsconfig, nf_sysdb_get_str_nocopy(buff), sizeof(char)*COMMON_SIZE-1);
	if( vsconfig != NULL ){
		if( strncmp(vsconfig, "asconfig", strlen("asconfig")) == 0){
			num_ch = atoi(vsconfig+strlen("asconfig"));
			return is_EnableAudio(num_ch);
		}
	}
	
	return 0;
}

int check_avail_resolution(int stream, char *resol, char *capa_string)
{
	char avail_res[16] = {0};
	int len = 0;
	int i = 0;
	int char_resol = onvif_get_size_data(resol);

	if(char_resol == '\0')
		return 0;
#ifdef NVR
	if(!stream) sprintf(avail_res, "%s", capa_string);
	else 		sprintf(avail_res, "%s", capa_string);
#else
	if(!stream) sprintf(avail_res, "%s", AVAIL_1ST_RESOL);
	else 		sprintf(avail_res, "%s", AVAIL_2ND_RESOL);
#endif
	len = strlen(avail_res);
	if(!len)
		return 0;
	
	for(i = 0; i < len; i++)
	{
		printf("#### avail_res[%c] char_resol[%c] ####\n", avail_res[i], char_resol);
		if(avail_res[i] == char_resol)
			return 1;
	}
	return 0;
}
void set_vencoder_table(arg_VideoEncoder *tmp)
{
	char buff[COMMON_SIZE] = {0,}, buff_value[COMMON_SIZE] = {0,}, buff_key[COMMON_SIZE] = {0,};
	char tmp_token[COMMON_SIZE] = {0,};
	int save_flag;
	int i;
	int vencoder_cnt;

#if 1
	//for utm2
	int ch = 0;
	int stream = 0;

	int j=0;
	int hour = 0;
	
	int init_here = 0;
	int motion_init_here = 0;
	int rec_changed = 0;
	int motion_rec_changed = 0;

	char char_resol[2] = {0, };
	char char_fps = 0;
	char char_quality = 0;
	char capa_string[32] = {0,};

	// char continuous_record_size[769]={0,};
	// char motion_record_size[769]={0,};
	// char continuous_record_fps[769]={0,};
	// char continuous_record_quality[769]={0,};
	char db_set_str[24]={0};

	// strncpy(continuous_record_size, nf_sysdb_get_str_nocopy("rec.continuous.C7.size"), 768);
	// strncpy(motion_record_size, nf_sysdb_get_str_nocopy("rec.motion.M7.size"), 768);
	// strncpy(continuous_record_fps, nf_sysdb_get_str_nocopy("rec.continuous.C7.fps"), 768);
	// strncpy(continuous_record_quality ,nf_sysdb_get_str_nocopy("rec.continuous.C7.quality"), 768);
	
	vencoder_cnt = ONVIF_VENCODER_CNT;
	for(i=0; i<vencoder_cnt; i++)
	{		
		ch = i%ONVIF_CH;
		stream = i/ONVIF_CH;

		sprintf(buff, "onvif.vencoder%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		
		if (!strcmp(tmp_token, tmp->token)) {
			// if(tmp->codec == ONVIF_JPEG_CODEC)
			// {
			// 	sprintf(buff, "onvif.vencoder%d.encoding", i);
			// 	nf_sysdb_set_uint(buff, tmp->codec);			
			// }
			if(tmp->codec == ONVIF_VIDEO_CODEC_H264)
			{
				if (stream == 0) { // 1st stream
					sprintf(buff, "cam.C%d.stream.S0.vcodec", ch);
					nf_sysdb_set_str(buff, "H.264");
				}
				else if (stream == 1) { // 2nd stream
					sprintf(buff, "cam.C%d.stream.S1.vcodec", ch);
					nf_sysdb_set_str(buff, "H.264");
				}
			}
			else if (tmp->codec == ONVIF_VIDEO_CODEC_H265)
			{
				if (stream == 0) { // 1st stream
					sprintf(buff, "cam.C%d.stream.S0.vcodec", ch);
					nf_sysdb_set_str(buff, "H.265");
				}
				else if (stream == 1) { // 2nd stream
					sprintf(buff, "cam.C%d.stream.S1.vcodec", ch);
					nf_sysdb_set_str(buff, "H.265");
				}
			}
			else { // not supported codec
				tmp->result = ONVIF_R_ERR_INVALID_PARAM;
				return;
			}

			// check parameter
			memset(buff_value, 0x00, sizeof(char)*COMMON_SIZE );
			snprintf(buff_value, sizeof(char)*COMMON_SIZE-1, "%dx%d",  tmp->width, tmp->height);

		#ifdef NVR
			RESOL_INFO_T resol;
			memset(&resol, 0, sizeof(resol));

			NFIPCamEncoderCap encoder_cap;
			nf_ipcam_get_encoder_capability(ch, &encoder_cap);
			resol_bitmask_to_char(encoder_cap.res_support[stream], DISPLAY_IS_PAL, capa_string);

			if (capa_string[0] == '\0')
			{
				snprintf(capa_string, 32, "RQST");
			}
		#endif
			//printf("\e[31m ####### check capa_string[%s] #### \e[0m\n", capa_string);
			if (check_avail_resolution(0, buff_value, capa_string) == 0)
			{
				tmp->result = ONVIF_R_ERR_INVALID_PARAM;
				return;	
			}
			/*
			if( check_onvif_fps_limit_interval(tmp->fps_interval, tmp->fps_limit) == 0){
				tmp->result = ONVIF_R_ERR_INVALID_PARAM;
				return;                
			}
			*/
			if(tmp->quality != -1)
			{
				if( check_onvif_quality(tmp->quality) == 0 ){
					tmp->result = ONVIF_R_ERR_INVALID_PARAM;
					return;
				}
			}

			if(tmp->govlength != -1)
			{
				if( tmp->govlength > ONVIF_VENCODER_MAX_GOV || tmp->govlength < ONVIF_VENCODER_MIN_GOV){
					tmp->result = ONVIF_R_ERR_INVALID_PARAM;
					return;
				}
			}

			/* 
			 *		<------ cam0 ----------><------ cam1 ----------><------ cam2 ----------><------ cam3 ---------->
			 *  	BBBBBBBBBBBBBBBBBBBBBBBBaaaaaaaaaaaaaaaaaaaaaaaaDDDDDDDDDDDDDDDDDDDDDDDDCCCCCCCCCCCCCCCCCCCCCCCCaaaaaaaaa
			 *
			 *		DAILY�� ���� C7�� 24���� ������� ����...  
			 *		WEEKLY�� ���� C0~C7�� 24���� ������� ����... 
			 *		C0 : SUN, C1:MON, C2:TUE...
			 */

			/* daily continuous�� enable�� ���� */

			/* name */
			if( tmp->name[0] != NULL ){
				memset(buff, 0x00, sizeof(char)*COMMON_SIZE);
				sprintf(buff, "cam.C%d.title", ch);
				nf_sysdb_set_str(buff, tmp->name);
			}
			{
				/* resolution */
				memset(buff_value, 0x00, sizeof(char)*COMMON_SIZE );			
				snprintf(buff_value, sizeof(char)*COMMON_SIZE-1, "%dx%d",  tmp->width, tmp->height);
				char_resol[0] = onvif_get_size_data(buff_value);

				memset(buff_value, 0x00, sizeof(char)*COMMON_SIZE );	
				snprintf(buff_value, sizeof(char)*COMMON_SIZE-1, "cam.C%d.stream.S%d.size",  ch, stream);
				nf_sysdb_set_str(buff_value, char_resol);				
	
				//printf("######################### char_resol[%c]\n",char_resol);
				/* fps */
				if( tmp->fps_limit != -1 ){
					memset(buff_key, 0x00, sizeof(char)*COMMON_SIZE );
					snprintf(buff_key, sizeof(char)*COMMON_SIZE-1, "cam.C%d.stream.S%d.fps", ch, stream);
					nf_sysdb_set_uint(buff_key, tmp->fps_limit);
				}
				/* quality */
				if( tmp->bitrate != -1 ){
					char_quality = onvif_get_qual_data(convert_max_bitrate_to_quality(tmp->bitrate));
				}
				// nf_sysdb_lock(NF_SYSDB_CATE_REC);
			// // Continuous
			// 	memset(db_set_str, char_resol, 24);
			// 	strncpy(&continuous_record_size[24*ch], db_set_str, 24);
			// 	continuous_record_size[768] = '\0';
			// 	nf_sysdb_set_str("rec.continuous.C7.size", continuous_record_size);
			// // Motion
			// 	strncpy(&continuous_record_size[24*ch], db_set_str, 24);
			// 	nf_sysdb_set_str("rec.motion.M7.size", motion_record_size);
			// // fps
			// 	memset(db_set_str, char_fps, 24);
			// 	strncpy(&continuous_record_fps[24*ch], db_set_str, 24);
			// 	nf_sysdb_set_str("rec.continuous.C7.fps", continuous_record_fps);
			// // quality
			// 	memset(db_set_str, char_quality, 24);
			// 	strncpy(&continuous_record_quality[24*ch], db_set_str, 24);
			// 	nf_sysdb_set_str("rec.continuous.C7.quality", continuous_record_quality);

				// nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_REC, 0, 0, 0);
				// nf_sysdb_unlock(NF_SYSDB_CATE_REC);
			}

			/* max bitrate == quality on dvr */
			if( tmp->bitrate != -1 ){
				memset(buff_key, 0x00, sizeof(char)*COMMON_SIZE );
				snprintf(buff_key, sizeof(char)*COMMON_SIZE-1, "onvif.vencoder%d.max_bitrate", i);		
				nf_sysdb_set_uint(buff_key, tmp->bitrate);
			}

			/* gov length */
			if( tmp->govlength != -1 ) {
				memset(buff_key, 0x00, sizeof(char)*COMMON_SIZE );
				snprintf(buff_key, sizeof(char)*COMMON_SIZE-1, "onvif.vencoder%d.h264_gov", i);				
				nf_sysdb_set_uint(buff_key, tmp->govlength);
			}
			/* fps interval : not supported */
			#if 0
			memset(buff_key, 0x00, sizeof(char)*COMMON_SIZE );
			snprintf(buff_key, sizeof(char)*COMMON_SIZE-1, SYSDB_REC_VIDEO_ENCODE_INTERVAL,  i%ONVIF_CH, i/ONVIF_CH);
			nf_sysdb_set_uint(buff_key, tmp->fps_interval);			
			#endif

			/* codec : only h.264 supported via RTSP */
			#if 0
			if( tmp->codec != -1){
				if( tmp->codec >= NUM_OF_CODECS || tmp->codec < 0){
					tmp->result = ONVIF_R_ERR_INVALID_PARAM;
					return;
				}

				memset(buff_value, 0x00, sizeof(char)*COMMON_SIZE );
				memset(buff_key, 0x00, sizeof(char)*COMMON_SIZE );
				snprintf(buff_key, sizeof(char)*COMMON_SIZE-1, SYSDB_REC_VIDEO_CODEC,  i%ONVIF_CH, i/ONVIF_CH);
				convert_onvif_codec(buff_value, tmp->codec);
				nf_sysdb_set_str(buff_key, buff_value);
				
			}
			#endif

			/* profile : only base24 supported */
			#if 0
			if( tmp->h264_profile != -1 ) {
				memset(buff_value, 0x00, sizeof(char)*COMMON_SIZE );
				memset(buff_key, 0x00, sizeof(char)*COMMON_SIZE );
				snprintf(buff_key, sizeof(char)*COMMON_SIZE-1, SYSDB_REC_VIDEO_PROFILE,  i%ONVIF_CH, i/ONVIF_CH);
				convert_onvif_h264_profile(buff_value, tmp->h264_profile);				
				nf_sysdb_set_str(buff_key, buff_value);
			}
			#endif

			/* quality : not supported */
			#if 0
			if( tmp->quality != -1 ){
				memset(buff_key, 0x00, sizeof(char)*COMMON_SIZE );
				snprintf(buff_key, sizeof(char)*COMMON_SIZE-1, SYSDB_REC_VIDEO_QUALITY,  i%ONVIF_CH, i/ONVIF_CH);				
				nf_sysdb_set_uint(buff_key, tmp->quality);
			}
			#endif

#if 0
			/* timeout */
			if( tmp->timeout != -1){
				int issm_ch = (stream * 16) + ch;
				issm_set_ep_track_timeout(g_issm_ep_entry_id[issm_ch], g_issm_track_v_h264_id[issm_ch]);
				snprintf(buff_key, sizeof(char)*COMMON_SIZE-1, "onvif.vencoder%d.timeout", ch);				
				nf_sysdb_set_uint(buff_key, tmp->timeout);
			}
#endif
			set_mcast_addr(&tmp->mcast, i, VIDEO_MCAST_TYPE);
		}
	}
	//token���� ã��
	tmp->result = 1;
	
#else
	vencoder_cnt = ONVIF_VENCODER_CNT;
	for(i=0; i<vencoder_cnt; i++)
	{
		char size[17];
		char fps[17];
		char quality[17];

		memset(size, 0x00, sizeof(char)*17);
		memset(fps, 0x00, sizeof(char)*17);
		memset(quality, 0x00, sizeof(char)*17);

		sprintf(buff, "onvif.vencoder%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		if (!strcmp(tmp_token, tmp->token)) {
			if( tmp->name[0] != NULL ){
				memset(buff, 0x00, sizeof(char)*COMMON_SIZE);
				snprintf(buff, sizeof(char)*COMMON_SIZE-1, SYSDB_REC_VIDEO_NAME,  i%ONVIF_CH, i/ONVIF_CH);
				nf_sysdb_set_str(buff, tmp->name);
			}

			// check parameter
			if( check_onvif_width_height(tmp->width, tmp->height) == 0){
				_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
				tmp->result = ONVIF_R_ERR_INVALID_PARAM;
				return;
			}

			if( check_onvif_fps_limit_interval(tmp->fps_interval, tmp->fps_limit) == 0){
				_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
				tmp->result = ONVIF_R_ERR_INVALID_PARAM;
				return;                
			}

			if( check_onvif_quality(tmp->quality) == 0 ){
				_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
				tmp->result = ONVIF_R_ERR_INVALID_PARAM;
				return;
			}
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);

			// apply parameter
			memset(buff_key, 0x00, sizeof(char)*COMMON_SIZE );
			memset(buff_value, 0x00, sizeof(char)*COMMON_SIZE );			
			snprintf(buff_key, sizeof(char)*COMMON_SIZE-1, SYSDB_REC_VIDEO_SIZE,  i%ONVIF_CH, i/ONVIF_CH);
			snprintf(buff_value, sizeof(char)*COMMON_SIZE-1, "%dx%d",  tmp->width, tmp->height);
			nf_sysdb_set_str(buff_key, buff_value);

			memset(buff_key, 0x00, sizeof(char)*COMMON_SIZE );
			snprintf(buff_key, sizeof(char)*COMMON_SIZE-1, SYSDB_REC_VIDEO_FPS,  i%ONVIF_CH, i/ONVIF_CH);
			nf_sysdb_set_uint(buff_key, tmp->fps_limit);
			
			memset(buff_key, 0x00, sizeof(char)*COMMON_SIZE );
			snprintf(buff_key, sizeof(char)*COMMON_SIZE-1, SYSDB_REC_VIDEO_ENCODE_INTERVAL,  i%ONVIF_CH, i/ONVIF_CH);
			nf_sysdb_set_uint(buff_key, tmp->fps_interval);			
			
			if( tmp->codec != -1){
				if( tmp->codec >= NUM_OF_CODECS || tmp->codec < 0){
					_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
					tmp->result = ONVIF_R_ERR_INVALID_PARAM;
					return;
				}
				_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);

				memset(buff_value, 0x00, sizeof(char)*COMMON_SIZE );
				memset(buff_key, 0x00, sizeof(char)*COMMON_SIZE );
				snprintf(buff_key, sizeof(char)*COMMON_SIZE-1, SYSDB_REC_VIDEO_CODEC,  i%ONVIF_CH, i/ONVIF_CH);
#if 1 //ncx3
				nf_sysdb_set_uint(buff_key, convert_onvif_codec(tmp->codec));
#else
				convert_onvif_codec(buff_value, tmp->codec);
				nf_sysdb_set_str(buff_key, buff_value);
#endif
			}

			if( tmp->h264_profile != -1 ) {
				_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
				memset(buff_value, 0x00, sizeof(char)*COMMON_SIZE );
				memset(buff_key, 0x00, sizeof(char)*COMMON_SIZE );
				snprintf(buff_key, sizeof(char)*COMMON_SIZE-1, SYSDB_REC_VIDEO_PROFILE,  i%ONVIF_CH, i/ONVIF_CH);
				convert_onvif_h264_profile(buff_value, tmp->h264_profile);				
				nf_sysdb_set_str(buff_key, buff_value);
			}

			if( tmp->bitrate != -1 ){
				_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
				memset(buff_key, 0x00, sizeof(char)*COMMON_SIZE );
				snprintf(buff_key, sizeof(char)*COMMON_SIZE-1, SYSDB_REC_VIDEO_BITRATE,  i%ONVIF_CH, i/ONVIF_CH);				
				nf_sysdb_set_uint(buff_key, tmp->bitrate);
			}

			if( tmp->quality != -1 ){
				_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
				memset(buff_key, 0x00, sizeof(char)*COMMON_SIZE );
				snprintf(buff_key, sizeof(char)*COMMON_SIZE-1, SYSDB_REC_VIDEO_QUALITY,  i%ONVIF_CH, i/ONVIF_CH);				
				nf_sysdb_set_uint(buff_key, tmp->quality);
			}

			if( tmp->govlength != -1 ) {
				_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
				if( tmp->govlength > ONVIF_VENCODER_MAX_GOV || tmp->govlength < ONVIF_VENCODER_MIN_GOV){
					_TTY_LOG_ONVIF_DEBUG("Entry: %s (tmp->govlength : %d)", __FUNCTION__, tmp->govlength);
					tmp->result = ONVIF_R_ERR_INVALID_PARAM;
					return;
				}
				_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
				memset(buff_key, 0x00, sizeof(char)*COMMON_SIZE );
				snprintf(buff_key, sizeof(char)*COMMON_SIZE-1, SYSDB_REC_VIDEO_GOP,  i%ONVIF_CH, i/ONVIF_CH);				
				nf_sysdb_set_uint(buff_key, tmp->govlength);
			}
			
			set_mcast_addr(&tmp->mcast, i, VIDEO_MCAST_TYPE);

		}
	}
	//token���� ã��
	tmp->result = 1;
#endif
}

void get_replayuri(arg_StreamSetup *tmp, const char *vencoder)
{
    NF_NETIF_GET_INFO ret_net_info;        
    struct sockaddr_in ipaddr;
    
    /* ipaddr, gateway, subnet */
    nf_netif_get_info(&ret_net_info);   
    ipaddr.sin_addr.s_addr = (unsigned long)ret_net_info.ipaddr;
    ipaddr.sin_addr.s_addr = htonl(ipaddr.sin_addr.s_addr);    
    
//  sprintf(tmp->uri, "%s:%d/playback/main%d", inet_ntoa(ipaddr.sin_addr), nf_sysdb_get_uint("net.proto.rtspport"), atoi(tmp->RecordingToken+2));

	sprintf(tmp->uri, "%s:%d/playback%d", inet_ntoa(ipaddr.sin_addr), nf_sysdb_get_uint("net.rtp.rtspport"), atoi(tmp->RecordingToken+2));
	sprintf(tmp->uri_http, "%s:%d/playback%d", inet_ntoa(ipaddr.sin_addr), nf_sysdb_get_uint("net.proto.webport"), atoi(tmp->RecordingToken+2));
}


int get_vencoder_index_from_profile(char *profile_token)
{
	char tmp[128], tmp2[128];
	int value = 0, i;
	
	if( profile_token == NULL ){
		return 0;
	}

	_TTY_LOG_ONVIF(">> [%s]", profile_token );
	for (i = 0; i < ONVIF_MAX_PROFILE_CNT; i++) {
		memset(tmp, 0x00, sizeof(char)*128);
		memset(tmp2, 0x00, sizeof(char)*128);
		sprintf(tmp, "onvif.profile%d.token", i);
		strncpy(tmp2, nf_sysdb_get_str_nocopy(tmp), COMMON_SIZE - 1);
		if (!strcmp(profile_token, tmp2)) {
			value = i;
			break;
		}
	}
	if( i == ONVIF_MAX_PROFILE_CNT ){
		return 0;
	}
	
	memset(tmp, 0x00, sizeof(char)*128);
	memset(tmp2, 0x00, sizeof(char)*128);	
	sprintf(tmp, "onvif.profile%d.vencoder", value);	
	sprintf(tmp2, "%s", nf_sysdb_get_str_nocopy(tmp));

	_TTY_LOG_ONVIF(">>tmp2 [%s]", tmp2 );
	if( tmp2[0] == 'v' && tmp2[1] == 'e' ){
		value = atoi(tmp2+strlen("ve"));
	}
	else{
		return 0;
	}

	return value;
	
}

void get_streamuri(arg_StreamUri *tmp, const char *vencoder)
{
	NF_NETIF_GET_INFO ret_net_info;        
	struct sockaddr_in ipaddr;
	int index = 0, is_second=0, num_ch=0;
	gboolean is_https = nf_sysdb_get_bool("net.proto.httpson");

	/* ipaddr, gateway, subnet */
	nf_netif_get_info(&ret_net_info);   
	ipaddr.sin_addr.s_addr = (unsigned long)ret_net_info.ipaddr;
	ipaddr.sin_addr.s_addr = htonl(ipaddr.sin_addr.s_addr);    

	//get video encoder index
	index = get_vencoder_index_from_profile(tmp->token);
	is_second = index/ONVIF_CH;
	num_ch = index%ONVIF_CH;

    if( is_second == 1 ){
		if (is_https) {
			sprintf(tmp->uri, "%s:%d/live/second%d", inet_ntoa(ipaddr.sin_addr), nf_sysdb_get_uint("net.rtp.rtspport"),num_ch );
			sprintf(tmp->uri_http, "%s:%d/live/second%d", inet_ntoa(ipaddr.sin_addr), nf_sysdb_get_uint("net.http.sslport"),num_ch );
		}
		else {
			sprintf(tmp->uri, "%s:%d/live/second%d", inet_ntoa(ipaddr.sin_addr), nf_sysdb_get_uint("net.rtp.rtspport"),num_ch );
	        sprintf(tmp->uri_http, "%s:%d/live/second%d", inet_ntoa(ipaddr.sin_addr), nf_sysdb_get_uint("net.proto.webport"),num_ch );
		}
    }
    else{
		if (is_https) {
			sprintf(tmp->uri, "%s:%d/live/main%d", inet_ntoa(ipaddr.sin_addr), nf_sysdb_get_uint("net.rtp.rtspport"),num_ch );
			sprintf(tmp->uri_http, "%s:%d/live/main%d", inet_ntoa(ipaddr.sin_addr), nf_sysdb_get_uint("net.http.sslport"),num_ch );
		}
		else {
			sprintf(tmp->uri, "%s:%d/live/main%d", inet_ntoa(ipaddr.sin_addr), nf_sysdb_get_uint("net.rtp.rtspport"),num_ch );
			sprintf(tmp->uri_http, "%s:%d/live/main%d", inet_ntoa(ipaddr.sin_addr), nf_sysdb_get_uint("net.proto.webport"),num_ch );
		}
    }

	tmp->result = 1;
}

void get_streamuri2(arg_StreamUri2 *tmp, const char *vencoder)
{
    NF_NETIF_GET_INFO ret_net_info;
    struct sockaddr_in ipaddr;
    int index = 0, is_second=0, num_ch=0;
	gboolean is_https = nf_sysdb_get_bool("net.proto.httpson");
    /* ipaddr, gateway, subnet */
    nf_netif_get_info(&ret_net_info);
    ipaddr.sin_addr.s_addr = (unsigned long)ret_net_info.ipaddr;
    ipaddr.sin_addr.s_addr = htonl(ipaddr.sin_addr.s_addr);

    //get video encoder index
    index = get_vencoder_index_from_profile(tmp->token);
    is_second = index/ONVIF_CH;
    num_ch = index%ONVIF_CH;

    if( is_second == 1 ){
		if (is_https) {
			sprintf(tmp->uri, "%s:%d/live/second%d", inet_ntoa(ipaddr.sin_addr), nf_sysdb_get_uint("net.rtp.rtspport"),num_ch );
			sprintf(tmp->uri_http, "%s:%d/live/second%d", inet_ntoa(ipaddr.sin_addr), nf_sysdb_get_uint("net.http.sslport"),num_ch );
		}
		else {
			sprintf(tmp->uri, "%s:%d/live/second%d", inet_ntoa(ipaddr.sin_addr), nf_sysdb_get_uint("net.rtp.rtspport"),num_ch );
	        sprintf(tmp->uri_http, "%s:%d/live/second%d", inet_ntoa(ipaddr.sin_addr), nf_sysdb_get_uint("net.proto.webport"),num_ch );
		}
    }
    else{
		if (is_https) {
			sprintf(tmp->uri, "%s:%d/live/main%d", inet_ntoa(ipaddr.sin_addr), nf_sysdb_get_uint("net.rtp.rtspport"),num_ch );
			sprintf(tmp->uri_http, "%s:%d/live/main%d", inet_ntoa(ipaddr.sin_addr), nf_sysdb_get_uint("net.http.sslport"),num_ch );
		}
		else {
			sprintf(tmp->uri, "%s:%d/live/main%d", inet_ntoa(ipaddr.sin_addr), nf_sysdb_get_uint("net.rtp.rtspport"),num_ch );
			sprintf(tmp->uri_http, "%s:%d/live/main%d", inet_ntoa(ipaddr.sin_addr), nf_sysdb_get_uint("net.proto.webport"),num_ch );
		}
    }
}

static int _get_jpeg_snapshot_uri(int ch, char *filepath, unsigned int buflen)
{
	gboolean is_request = 0;
	gint retry_cnt = 30;
	
	NF_JPEG_MAN_SNAPSHOT *snapshot = NULL;

	if ( filepath == NULL )
	{
		return -1;
	}

	do
	{

		if ( !is_request ) // jpeg �청
			is_request = nf_jpeg_man_request_snapshot( ch, NF_SECOND_SIZE, 1500 );

		if ( nf_jpeg_man_check_snapshot( ch, NF_SECOND_SIZE, 1500 ) )
		{
			if ( nf_jpeg_man_get_snapshot( ch, NF_SECOND_SIZE, &snapshot ) )
			{
#if 0 //for ncx
				snprintf(filepath, buflen - 1, "/jpeg");
#else				
				snprintf(filepath, buflen - 1, "/jpeg/ch%02d_%d_%06d.jpg", ch, snapshot->ctime.tv_sec, snapshot->ctime.tv_usec);
#endif
				nf_jpeg_man_free_snapshot(snapshot);
				
				break;
			}
		}
		g_usleep(33000);
	}
	while ( --retry_cnt > 0 );

	return 1;
}

void get_snapshot_uri(int index, arg_SnapshotUri *tmp)
{
	NF_NETIF_GET_INFO ret_net_info;        
	struct sockaddr_in ipaddr;
	int is_second=0, num_ch=0, i=0;
	char tmp_path[128] = {0,}, buff[COMMON_SIZE] = {0,}, profile_token[COMMON_SIZE] = {0,}, vencoder[COMMON_SIZE] = {0,};

	/* ipaddr, gateway, subnet */
	nf_netif_get_info(&ret_net_info);   
	ipaddr.sin_addr.s_addr = (unsigned long)ret_net_info.ipaddr;
	ipaddr.sin_addr.s_addr = htonl(ipaddr.sin_addr.s_addr);    

#if 0 //[[ hun_0140723_BEGIN -- del
	num_ch = index%ONVIF_CH;
#else
	memset(buff, 0x00, sizeof(char)*COMMON_SIZE);
	memset(vencoder, 0x00, sizeof(char)*COMMON_SIZE);
	sprintf(buff, "onvif.profile%d.vencoder", index);
	strncpy(vencoder, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
	num_ch = atoi(vencoder+2)%ONVIF_CH; // vencoder token : ve0
#endif //]] hun_0140723_END -- del

	// get jpeg
	memset(tmp_path, 0x00, sizeof(char)*128);
	_get_jpeg_snapshot_uri(num_ch, tmp_path, sizeof(char)*128);

	if(is_https_required())
		sprintf(tmp->uri, "https://%s:%d%s", inet_ntoa(ipaddr.sin_addr), nf_sysdb_get_uint("net.proto.sslport"), tmp_path );
	else
		sprintf(tmp->uri, "http://%s:%d%s", inet_ntoa(ipaddr.sin_addr), nf_sysdb_get_uint("net.proto.webport"), tmp_path );

	tmp->result = 1;
}


int getEncoding(int i)
{
	char buff[COMMON_SIZE] = {0,};
	int codec = -1;
	sprintf(buff, "onvif.vencoder%d.encoding", i);
	codec = nf_sysdb_get_uint(buff);
	if (codec == CODEC_H264) {
		return ONVIF_H264_CODEC;
	} else if (codec == CODEC_MJPEG) {
		return ONVIF_JPEG_CODEC;
	}
	return -1;
}
#if 0 //[[ Jeonghun_0130412_BEGIN -- Del

void getRuntime(int ch,int* capable,char capa[])
{
	mtable* runtime = NULL;
	runtime = get_runtime();
	*capable = runtime[ch].video.resolution.supported;
	resol_bitmask_to_char(*capable, FALSE, capa);
}
#endif //]] Jeonghun_0130412_END -- Del

int get_vsource_usecount(char *token) {
	int profile_cnt, usecount = 0;
	int i;
	char buff[COMMON_SIZE] = { 0, };
	char tmp_token[COMMON_SIZE] = { 0, };

	profile_cnt = MAX_PROFILE;//nf_sysdb_get_uint("onvif.common.profile_cnt");

	for (i = 0; i < profile_cnt; i++) {
		//if(nf_sysdb_get_uint("camera.S1.codec") == CODEC_NONE && i == 1){   // second is NONE
		//	continue;
		//}
		sprintf(buff, "onvif.profile%d.vsource", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(token, tmp_token))
			usecount++;
}

	return usecount;
}

float get_fps_from_index(unsigned int req_fps)
{
	if(req_fps == 0) return 30.0;
	else if(req_fps == 1) return 15.0;
	else if(req_fps == 2) return 10.0;
	else if(req_fps == 3) return 7.5;
	else if(req_fps == 4) return 6;
	else if(req_fps == 5) return 5;
	else if(req_fps == 6) return 4.3;
	else if(req_fps == 7) return 3.8;
	else if(req_fps == 8) return 3.3;
	else if(req_fps == 9) return 3.0;
	else if(req_fps == 10) return 2.7;
	else if(req_fps == 11) return 2.5;
	else return 1;
}

int get_aencoder_usecount(char *token) {
	int profile_cnt, usecount = 0;
	int i;
	char buff[COMMON_SIZE] = { 0, };
	char tmp_token[COMMON_SIZE] = { 0, };

	profile_cnt = MAX_PROFILE;//nf_sysdb_get_uint("onvif.common.profile_cnt");

	for (i = 0; i < profile_cnt; i++) {
		//if(nf_sysdb_get_uint("camera.S1.codec") == CODEC_NONE && i == 1){   // second is NONE
		//	continue;
		//}
		sprintf(buff, "onvif.profile%d.aencoder", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(token, tmp_token))
			usecount++;
	}

	return usecount;
}

int findfieldUIntCount(char* field,int num,int length)
{
	int i=0;
	char buff[COMMON_SIZE] = { 0, };
	char tmp_token[COMMON_SIZE] = { 0, };
	for (i = 0; i < length; i++) {
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		sprintf(buff, field, i);
		if(num == nf_sysdb_get_uint(buff)) {
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
			return i;
		}
	}
	return -1;
}
int findfieldStringCount(char* field,char* token,int length)
{
	int i=0;
	char buff[COMMON_SIZE] = { 0, };
	char tmp_token[COMMON_SIZE] = { 0, };
	for (i = 0; i < length; i++) {

		sprintf(buff, field, i);

		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		_TTY_LOG_ONVIF_DEBUG("token: %s (tmp_token : %s)", token, tmp_token);
		if (!strcmp(token, tmp_token)) {
			return i;
		}
	}
	return -1;
}
int get_asource_usecount(char *token)
{
	int profile_cnt, usecount=0;
	int i;
	char buff[COMMON_SIZE] = {0,};
	char tmp_token[COMMON_SIZE] = {0,};

	profile_cnt = MAX_PROFILE;//nf_sysdb_get_uint("onvif.common.profile_cnt");

	for(i=0; i<profile_cnt;i++) {
		
		sprintf(buff, "onvif.profile%d.asource", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if(!strcmp(token, tmp_token)) usecount++;
	}

	return usecount;
}

int get_metadata_usecount(char *token) {
	int profile_cnt, usecount = 0;
	int i;
	char buff[COMMON_SIZE] = { 0, };
	char tmp_token[COMMON_SIZE] = { 0, };

	profile_cnt = MAX_PROFILE;//nf_sysdb_get_uint("onvif.common.profile_cnt");

	for (i = 0; i < profile_cnt; i++) {

		sprintf(buff, "onvif.profile%d.metadata", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(token, tmp_token))
			usecount++;
	}

	return usecount;
}

unsigned int load_current_usrman(ONVIF_USR_MAN* tmp) 
{
	ONVIF_USR_MAN man[8];
	char buff[128];
	int i=0;

	memset( man, 0, sizeof(man) );
	memset( buff, 0, sizeof(buff) );
	
	man[0].usrcnt = (int)nf_sysdb_get_uint("usr.UCNT");

	for (i = 0; i < man[0].usrcnt; i++)
	{
		memset(buff, 0, sizeof(buff));
		snprintf(buff, sizeof(buff), "usr.U%d.name", i);
		snprintf(man[i].usrid, sizeof(man[i].usrid), "%s", nf_sysdb_get_str_nocopy(buff));

		memset(buff, 0, sizeof(buff));
		snprintf(buff, sizeof(buff), "usr.U%d.pass", i);
		snprintf(man[i].passwd, sizeof(man[i].passwd),"%s", nf_sysdb_get_str_nocopy(buff));

		memset(buff, 0, sizeof(buff));
		snprintf(buff, sizeof(buff), "usr.U%d.email", i);
		snprintf(man[i].email , sizeof(man[i].email),"%s", nf_sysdb_get_str_nocopy(buff));

		memset(buff, 0, sizeof(buff));
		snprintf(buff, sizeof(buff), "usr.U%d.email_notify", i);
		man[i].email_notify = (char)nf_sysdb_get_bool(buff);

		memset(buff, 0, sizeof(buff));
		snprintf(buff, sizeof(buff), "usr.U%d.grpname", i);
		snprintf(man[i].grpname, sizeof(man[i].grpname),"%s", nf_sysdb_get_str_nocopy(buff));

		memset(buff, 0, sizeof(buff));
		snprintf(buff, sizeof(buff), "usr.U%d.covert", i);
		snprintf(man[i].covert, sizeof(man[i].covert),"%s", nf_sysdb_get_str_nocopy(buff));

		memset(buff, 0, sizeof(buff));
		snprintf(buff, sizeof(buff), "usr.U%d.pw_last_changed", i);
		man[i].pw_last_changed = nf_sysdb_get_uint(buff);

		memset(buff, 0, sizeof(buff));
		snprintf(buff, sizeof(buff), "usr.U%d.expired_check", i);
		man[i].expired_check = nf_sysdb_get_uint(buff);

		_TTY_LOG_ONVIF("L[%d][%s][%s][%s]", i, man[i].usrid, man[i].passwd, man[i].grpname);

		man[i].usrcnt = (unsigned char)nf_sysdb_get_uint("usr.UCNT");			
	}

	memcpy(tmp, man, sizeof(man));

	return man[0].usrcnt;	
}

unsigned int save_current_usrman(ONVIF_USR_MAN* man)
{
	int i=0;
	char buff[128];
	GTimeVal tvTemp;	

	for (i = 0; i < man[0].usrcnt; i++) {
		memset(buff, 0, sizeof(buff));
		snprintf(buff, sizeof(buff), "usr.U%d.name", i);
		nf_sysdb_set_str(buff, man[i].usrid);

		memset(buff, 0, sizeof(buff));
		snprintf(buff, sizeof(buff), "usr.U%d.pass", i);
		nf_sysdb_set_str(buff, man[i].passwd);

		memset(buff, 0, sizeof(buff));
		snprintf(buff, sizeof(buff), "usr.U%d.email", i);
		nf_sysdb_set_str(buff, man[i].email);

		memset(buff, 0, sizeof(buff));
		snprintf(buff, sizeof(buff), "usr.U%d.email_notify", i);
		nf_sysdb_set_bool(buff, man[i].email_notify);

		memset(buff, 0, sizeof(buff));
		snprintf(buff, sizeof(buff), "usr.U%d.grpname", i);
		nf_sysdb_set_str(buff, man[i].grpname);

		memset(buff, 0, sizeof(buff));
		snprintf(buff, sizeof(buff), "usr.U%d.covert", i);
		nf_sysdb_set_str(buff, man[i].covert);

		if (man[i].pw_last_changed == 0) {
			memset(&tvTemp, 0, sizeof(GTimeVal));
			g_get_current_time(&tvTemp);
			memset(buff, 0, sizeof(buff));
			snprintf(buff, sizeof(buff), "usr.U%d.pw_last_changed", i);
			nf_sysdb_set_uint(buff, (unsigned int)tvTemp.tv_sec);
			man[i].pw_last_changed = (unsigned int)tvTemp.tv_sec;
		} else {
			memset(buff, 0, sizeof(buff));
			snprintf(buff, sizeof(buff), "usr.U%d.pw_last_changed", i);
			nf_sysdb_set_uint(buff, man[i].pw_last_changed);
		}

		memset(buff, 0, sizeof(buff));
		snprintf(buff, sizeof(buff), "usr.U%d.expired_check", i);
		nf_sysdb_set_uint(buff, man[i].expired_check);
		_TTY_LOG_ONVIF("S[%d][%s][%s][%s]", i, man[i].usrid, man[i].passwd, man[i].grpname);
	}

	nf_sysdb_set_uint("usr.UCNT", man[0].usrcnt);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_USR, 0, 0, 0);
	sysdb_save_cate(NF_SYSDB_CATE_USR);
	
	return 1;
}


/*  
This function converts ONVIF GROUP -> NORMAL GROUP.
onvif group name is defined on onvif_common.h  */
int convert_onvif_to_group(char *dest, char *onvif_grp)
{
	if (strncmp(onvif_grp, "ADMIN", sizeof("ADMIN")) == 0){
		sprintf(dest, "ADMIN");
	}
	else if (strncmp(onvif_grp, "OPERATOR", sizeof("OPERATOR")) == 0){
		sprintf(dest, "MANAGER");
	}
	else if (strncmp(onvif_grp, "USER", sizeof("USER")) == 0){
		sprintf(dest, "USER");					
	}
	else {
		return -1;					
	}			
	
	_TTY_LOG_ONVIF(" convert [%s] to [%s] ", onvif_grp, dest );
	return 1;
}

/*  
This function converts  NORMAL GROUP -> ONVIF GROUP.
onvif group name is defined on onvif_common.h  */
int convert_group_to_onvif(char *dest, char *src)
{
	if (strncmp(src, "ADMIN", sizeof("ADMIN")) == 0){
		sprintf(dest, "ADMIN");
	}
	else if (strncmp(src, "MANAGER", sizeof("MANAGER")) == 0){
		sprintf(dest, "OPERATOR");
	}
	else if (strncmp(src, "USER", sizeof("USER")) == 0){
		sprintf(dest, "USER");					
	}
	else {
		return -1;					
	}			
	
	_TTY_LOG_ONVIF(" convert [%s] to [%s] ", src, dest );
	return 1;
}
	
#ifdef ONVIF_MODEL_IPX
void getExposureImageProfile(int ch,   ONVIF_CAM_COMPATIBILITY_EXPOSURE_PROFILE* profile)
{
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
	int i=0;
	NFIPCamExposureProfile_onvif onvif;

	if (nf_ipcam_get_exposure_profile_onvif(ch, &onvif) == IPCAM_SETUP_RTN_DONE)
	  {
	    profile->supported_exposure =  onvif.supported_exposure;

	    profile->mode_cnt = onvif.mode_cnt;
	    for (i = 0; i < profile->mode_cnt; i++)
	    {
	      copy_ipcam_info_option_onvif(&profile->mode[i], onvif.mode[i]);
	    }

	    profile->priority_cnt = onvif.priority_cnt;
	    for (i = 0; i < profile->priority_cnt; i++)
	    {
	      copy_ipcam_info_option_onvif(&profile->priority[i], onvif.priority[i]);
	    }

	    profile->blc_cnt = onvif.blc_cnt;
	    for (i = 0; i < profile->blc_cnt; i++)
	    {
	      copy_ipcam_info_option_onvif(&profile->blc[i], onvif.blc[i]);
	    }
	    copy_ipcam_info_value(&profile->blclevel, onvif.blclevel);

	    copy_ipcam_info_value(&profile->minetime, onvif.minetime);
	    copy_ipcam_info_value(&profile->maxetime, onvif.maxetime);
	    copy_ipcam_info_value(&profile->etime, onvif.etime);
	    profile->slowshutter_cnt = onvif.slowshutter_cnt;
	    for (i = 0; i < profile->slowshutter_cnt; i++)
	    {
	      copy_ipcam_info_option_onvif(&profile->slowshutter[i], onvif.slowshutter[i]);
	    }

	    copy_ipcam_info_value(&profile->mingain, onvif.mingain);
	    copy_ipcam_info_value(&profile->maxgain, onvif.maxgain);
	    copy_ipcam_info_value(&profile->gain, onvif.gain);
	    profile->maxagc_cnt = onvif.maxagc_cnt;
	    for (i = 0; i < profile->maxagc_cnt; i++)
	    {
	      copy_ipcam_info_option_onvif(&profile->maxagc[i], onvif.maxagc[i]);
	    }

	    copy_ipcam_info_value(&profile->miniris, onvif.miniris);
	    copy_ipcam_info_value(&profile->maxiris, onvif.maxiris);
	    copy_ipcam_info_value(&profile->iris, onvif.iris);
	    profile->dc_iris_cnt = onvif.dc_iris_cnt;
	    for (i = 0; i < profile->dc_iris_cnt; i++)
	    {
	      copy_ipcam_info_option_onvif(&profile->dc_iris[i], onvif.dc_iris[i]);
	    }

	    copy_ipcam_info_value(&profile->bottom, onvif.bottom);
	    copy_ipcam_info_value(&profile->top, onvif.top);
	    copy_ipcam_info_value(&profile->right, onvif.right);
	    copy_ipcam_info_value(&profile->left, onvif.left);
	  }
}
 void getImageProfile(int ch, ONVIF_CAM_COMPATIBILITY_IMAGE_PROFILE *profile)
{
	 _TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
	int i=0;
	 NFIPCamImageProfile_onvif onvif;

	 _TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);

	  if (nf_ipcam_get_image_profile_onvif(ch, &onvif) == IPCAM_SETUP_RTN_DONE)
	  {
		  _TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
		  	 profile->supported_image = onvif.supported_image;
		  	//_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
		     copy_ipcam_info_value(&profile->brightness,onvif.brightness);
		     copy_ipcam_info_value(&profile->contrast,onvif.contrast);
		     copy_ipcam_info_value(&profile->sharpness,onvif.sharpness);
		     _TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);

		     copy_ipcam_info_value(&profile->color, onvif.color);
		     copy_ipcam_info_value(&profile->tint, onvif.tint);

		     _TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
		     profile->mirror_cnt = onvif.mirror_cnt;
		     for (i = 0; i < profile->mirror_cnt; i++)
		     {
		       copy_ipcam_info_option_onvif(&profile->mirror[i], onvif.mirror[i]);
		     }
		     _TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
		     profile->focus_cnt = onvif.focus_cnt;
		     _TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
		     for (i = 0; i < profile->focus_cnt; i++)
		     {
		       copy_ipcam_info_option_onvif(&profile->focus[i], onvif.focus[i]);
		     }
		     _TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
		     copy_ipcam_info_value(&profile->defaultspeed, onvif.defaultspeed);
		     copy_ipcam_info_value(&profile->nearlimit, onvif.nearlimit);
		     copy_ipcam_info_value(&profile->farlimit, onvif.farlimit);
		     copy_ipcam_info_value(&profile->abposition, onvif.abposition);
		     copy_ipcam_info_value(&profile->abspeed, onvif.abspeed);
		     copy_ipcam_info_value(&profile->redistance, onvif.redistance);
		     copy_ipcam_info_value(&profile->respeed, onvif.respeed);
		     copy_ipcam_info_value(&profile->cospeed, onvif.cospeed);

		     _TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
		     profile->wb_cnt = onvif.wb_cnt;
		     for (i = 0; i < profile->wb_cnt; i++)
		     {
		       copy_ipcam_info_option_onvif(&profile->wb[i], onvif.wb[i]);
		     }
		     _TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
		     copy_ipcam_info_value(&profile->crgain, onvif.crgain);
		     copy_ipcam_info_value(&profile->cbgain, onvif.cbgain);
		     _TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
		     profile->mwb_cnt = onvif.mwb_cnt;
		     for (i = 0; i < profile->mwb_cnt; i++)
		     {
		       copy_ipcam_info_option_onvif(&profile->mwb[i], onvif.mwb[i]);
		     }
		     _TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
		     profile->wdr_cnt = onvif.wdr_cnt;
		     for (i = 0; i < profile->wdr_cnt; i++)
		     {
		       copy_ipcam_info_option_onvif(&profile->wdr[i], onvif.wdr[i]);
		     }
		     copy_ipcam_info_value(&profile->wdrlevel, onvif.wdrlevel);
		     _TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
		     profile->ircut_cnt = onvif.ircut_cnt;
		     for (i = 0; i < profile->ircut_cnt; i++)
		     {
		       copy_ipcam_info_option_onvif(&profile->ircut[i], onvif.ircut[i]);
		     }
		     _TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
		     profile->dnn_toggle_cnt = onvif.dnn_toggle_cnt;
		     for (i = 0; i < profile->dnn_toggle_cnt; i++)
		     {
		       copy_ipcam_info_option_onvif(&profile->dnn_toggle[i], onvif.dnn_toggle[i]);
		     }

		}
	  return;
}
 int getOnvifExposureMode(int itxMode)
 {
	 _TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
	 	 int mode;
		 switch(itxMode)
		 {
		 case NF_IPCAM_EXPOSURE_MODE_ONVIF_AUTO:
			mode = ONVIF_EXPOSURE_MODE_AUTO;
			break;
		 case NF_IPCAM_EXPOSURE_MODE_ONVIF_MANUAL:
			mode = ONVIF_EXPOSURE_MODE_MANUAL;
			break;
		 default:
			 _TTY_LOG_ONVIF_DEBUG("unknown operator");
			break;
		 }
		 return mode;

 }
  int getOnvifIrcutfilterMode(int itxMode)
 {
	 _TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
	 int mode;
	 switch(itxMode)
	 {
	 case NF_IPCAM_IRCUT_MODE_ITX_NIGHTTIME:
		mode = ONVIF_IRCUT_FILTER_MODE_ON;
		break;
	 case NF_IPCAM_IRCUT_MODE_ITX_DAYTIME:
		mode = ONVIF_IRCUT_FILTER_MODE_OFF;
		break;
	 case NF_IPCAM_IRCUT_MODE_ITX_AUTO:
		mode = ONVIF_IRCUT_FILTER_MODE_AUTO;
		break;
	 default:
		 _TTY_LOG_ONVIF_DEBUG("unknown operator");
		break;
	 }
	 return mode;
 }

 int getIrcutfilterMode(int onvifMode)
 {
	 _TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
	 int mode;
	 switch(onvifMode)
	 {
	 case ONVIF_IRCUT_FILTER_MODE_ON:
		mode = NF_IPCAM_IRCUT_MODE_ITX_NIGHTTIME;
		break;
	 case ONVIF_IRCUT_FILTER_MODE_OFF:
		mode = NF_IPCAM_IRCUT_MODE_ITX_DAYTIME;
		break;
	 case ONVIF_IRCUT_FILTER_MODE_AUTO:
		mode = NF_IPCAM_IRCUT_MODE_ITX_AUTO;
		break;
	 default:
		 _TTY_LOG_ONVIF_DEBUG("unknown operator");
		break;
	 }
	 return mode;
 }
 int getOnvifBlcMode(int itxMode)
 {
	 _TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
	 _TTY_LOG_ONVIF_DEBUG("itxMode mode : %d ",itxMode);

	 int mode;
	 switch(itxMode)
	 {
	 case NF_IPCAM_BLC_MODE_ONVIF_OFF: //blc mode off
		 mode = ONVIF_VLC_MODE_OFF;
		 break;
	 case NF_IPCAM_BLC_MODE_ONVIF_ON: //blc mode on
		 mode = ONVIF_VLC_MODE_ON;
		 break;
	 default:
		 _TTY_LOG_ONVIF_DEBUG("unknown operator");
		 break;
	 }

	 return mode;
 }
 int getBlcMode(int onvifMode)
 {
	 _TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
	 _TTY_LOG_ONVIF_DEBUG("onvif mode : %d ",onvifMode);

	 int mode;
	 switch(onvifMode)
	 {
	 case ONVIF_VLC_MODE_OFF: //blc mode off
		 mode = NF_IPCAM_BLC_MODE_ONVIF_OFF;
		 break;
	 case ONVIF_VLC_MODE_ON: //blc mode on
		 mode = NF_IPCAM_BLC_MODE_ONVIF_ON;
		 _TTY_LOG_ONVIF_DEBUG("ITX mode : %d ",mode);
		 break;
	 default:
		 _TTY_LOG_ONVIF_DEBUG("unknown operator");
		 break;
	 }

	 return mode;

 }
 int getExposureMode(int onvifMode)
 {
	 int mode;
	 switch(onvifMode)
	 {
	 	 case 0: //exposure mode off
	 		 mode = NF_IPCAM_EXPOSURE_MODE_ONVIF_AUTO;
	 		 break;
	 	case 1: //exposure mode off
	 		mode = NF_IPCAM_EXPOSURE_MODE_ONVIF_MANUAL;
	 		break;
	 }
	 return mode;
 }
#endif
 void setImageSettings(int ch, arg_ImagingOption* tmp)
 {

	 _TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
	  nf_sysdb_lock(NF_SYSDB_CATE_CAM);
	  int error,mode;

	  if (tmp->sharpness != -1)
	  {
		  _TTY_LOG_ONVIF_DEBUG("tmp->sharpness %d",tmp->sharpness);
		  if(!onvif_SetDbUint("cam.C%d.sharpness", ch, tmp->sharpness))
		  {
			  _TTY_LOG_ONVIF_DEBUG("Setting Error: %s (Line : %d)",__FUNCTION__, __LINE__);
		  }

	  }
	  if(tmp->brightness != -1)
	  {
		  _TTY_LOG_ONVIF_DEBUG("tmp->brightness %d",tmp->brightness);
		  if(!onvif_SetDbUint("cam.C%d.bright", ch, tmp->brightness))
		  {
			  _TTY_LOG_ONVIF_DEBUG("Setting Error: %s (Line : %d)",__FUNCTION__, __LINE__);
		  }
	  }
	  if(tmp->contrast != -1)
	  {
		  _TTY_LOG_ONVIF_DEBUG("tmp->contrast %d",tmp->contrast);
		  if(!onvif_SetDbUint("cam.C%d.contrast", ch, tmp->contrast)) {
			  _TTY_LOG_ONVIF_DEBUG("Setting Error: %s (Line : %d)",__FUNCTION__, __LINE__);
		  }
	  }
	  if(tmp->color != -1)
	  {
		  _TTY_LOG_ONVIF_DEBUG("tmp->color %d",tmp->color);
		  if(!onvif_SetDbUint("cam.C%d.color", ch, tmp->color))
		  _TTY_LOG_ONVIF_DEBUG("Setting Error: %s (Line : %d)",__FUNCTION__, __LINE__);
	  }
	  
  #ifdef ONVIF_MODEL_IPX
	  if (tmp->ircutfilter != -1)
	  {
		  mode = getIrcutfilterMode(tmp->ircutfilter);
		  _TTY_LOG_ONVIF_DEBUG("mode : %d",mode);
		  if(!onvif_SetDbUint("cam.C%d.day_night_mode", ch, (uint)mode))
		  _TTY_LOG_ONVIF_DEBUG("Setting Error: %s (Line : %d)",__FUNCTION__, __LINE__);
	  }
	  if (tmp->blc_mode != -1)
	  {
		  mode = getBlcMode(tmp->blc_mode);
		  if(!onvif_SetDbUint("cam.C%d.blc_control", ch, (uint)mode))
		  _TTY_LOG_ONVIF_DEBUG("Setting Error: %s (Line : %d)",__FUNCTION__, __LINE__);
	  }
	  if (tmp->e_mode != -1)
	  {
		  mode = getExposureMode(tmp->e_mode);
		  if(!onvif_SetDbUint("cam.C%d.exposure_mode", ch, (uint)mode)) {
			  _TTY_LOG_ONVIF_DEBUG("Setting Error: %s (Line : %d)",__FUNCTION__, __LINE__);
		  }
		  else {
		  _TTY_LOG_ONVIF_DEBUG("Setting ok: %s (Line : %d)",__FUNCTION__, __LINE__);
		  }
	  }
	  if (tmp->e_time != -1)
	  {
		  _TTY_LOG_ONVIF_DEBUG("tmp->e_time : %d)",tmp->e_time);
		  if(!SetDbInt("cam.C%d.etime", ch, tmp->e_time)) {
			  _TTY_LOG_ONVIF_DEBUG("Setting Error: %s (Line : %d)",__FUNCTION__, __LINE__);
		  }
		  else
		  _TTY_LOG_ONVIF_DEBUG("Setting ok: %s (Line : %d)",__FUNCTION__, __LINE__);
	  }
	  if (tmp->e_gain != -1)
	  {
		  _TTY_LOG_ONVIF_DEBUG("tmp->e_gain : %d)",tmp->e_gain);
		  if(!SetDbInt("cam.C%d.gain", ch, tmp->e_gain)) {
		  _TTY_LOG_ONVIF_DEBUG("Setting Error: %s (Line : %d)",__FUNCTION__, __LINE__);
		  }
		  else
		  _TTY_LOG_ONVIF_DEBUG("Setting ok: %s (Line : %d)",__FUNCTION__, __LINE__);
	  }
	  if (tmp->e_iris != -1)
	  {
		  _TTY_LOG_ONVIF_DEBUG("tmp->e_iris : %d)",tmp->e_iris);
		  if(!SetDbInt("cam.C%d.iris", ch, tmp->e_iris)) {
		  _TTY_LOG_ONVIF_DEBUG("Setting Error: %s (Line : %d)",__FUNCTION__, __LINE__);
		  }
		  else
		  _TTY_LOG_ONVIF_DEBUG("Setting ok: %s (Line : %d)",__FUNCTION__, __LINE__);
	  }
#endif
	  nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
 }

int getOnvifAutoFocusValue(int val)
{
	int mode =0;
	 switch(val)
	 {
	 	 default:
	 		 mode = ONVIF_AUTO_FOCUS_MODE_MANUAL;
	 		 break;

	 }

	return 1;
}

int adjustValue(float val)
{
	int tmpVal;

	tmpVal = (int)abs(val*PTZ_NORMALIZE_VALUE);

	return tmpVal;
}
int getPtzZoomCmd(arg_PTZOperation *tmp, int* outInterval)
{
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);

	int cmd = -1;
	if(tmp->Zoom.isZoom)
	{
		_TTY_LOG_ONVIF_DEBUG("tmp->PanTilt.x : %f",tmp->Zoom.x);
		if(tmp->Zoom.x == 0.0 ) {
			cmd = NF_PTZ_CMD_STOP;
		}
		else if(tmp->Zoom.x > 0) {
			*outInterval = adjustValue(abs(tmp->Zoom.x));
			cmd = NF_PTZ_CMD_ZOOM_TELE;
		}else if(tmp->Zoom.x < 0){
			*outInterval = adjustValue(abs(tmp->Zoom.x));
			cmd = NF_PTZ_CMD_ZOOM_WIDE;		
		}else{
			_TTY_LOG_ONVIF_DEBUG("ASSERT: %s (Line : %d)",__FUNCTION__, __LINE__);
		}

	}
	return cmd;
}

int getPtzPanTiltCmd(arg_PTZOperation *tmp, int* outInterval)
{
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);

	int cmd = -1;
	if(tmp->PanTilt.isPantilt)
	{
		if(tmp->PanTilt.x == 0.0 && tmp->PanTilt.y == 0.0 ) {
			cmd = NF_PTZ_CMD_STOP;
		}
		else if(tmp->PanTilt.x > 0 && tmp->PanTilt.y > 0) {
			cmd = NF_PTZ_CMD_PT_RIGHTUP;
			*outInterval = adjustValue(sqrt(tmp->PanTilt.x*tmp->PanTilt.x + tmp->PanTilt.y*tmp->PanTilt.y));
		}else if(tmp->PanTilt.x > 0 && tmp->PanTilt.y < 0) {
			cmd = NF_PTZ_CMD_PT_RIGHTDOWN;
			*outInterval = adjustValue(sqrt(tmp->PanTilt.x*tmp->PanTilt.x + tmp->PanTilt.y*tmp->PanTilt.y));
		}else if(tmp->PanTilt.x < 0 && tmp->PanTilt.y > 0) {
			*outInterval = adjustValue(sqrt(tmp->PanTilt.x*tmp->PanTilt.x + tmp->PanTilt.y*tmp->PanTilt.y));
			cmd = NF_PTZ_CMD_PT_LEFTUP;
		}else if(tmp->PanTilt.x < 0 && tmp->PanTilt.y < 0) {
				*outInterval = adjustValue(sqrt(tmp->PanTilt.x*tmp->PanTilt.x + tmp->PanTilt.y*tmp->PanTilt.y));
			cmd = NF_PTZ_CMD_PT_LEFTDOWN;
		}else if(tmp->PanTilt.x > 0 && tmp->PanTilt.y == 0) {
			*outInterval = adjustValue(tmp->PanTilt.x);
			cmd = NF_PTZ_CMD_PAN_RIGHT;
		}else if(tmp->PanTilt.x < 0 && tmp->PanTilt.y == 0) {
			*outInterval = adjustValue(tmp->PanTilt.x);
			cmd = NF_PTZ_CMD_PAN_LEFT;			
		}else if(tmp->PanTilt.x == 0 && tmp->PanTilt.y > 0) {
			*outInterval = adjustValue(tmp->PanTilt.y);
			cmd = NF_PTZ_CMD_TILT_UP;
		}else if(tmp->PanTilt.x == 0 && tmp->PanTilt.y < 0) {
			*outInterval = adjustValue(tmp->PanTilt.y);
			cmd = NF_PTZ_CMD_TILT_DOWN;
		}else {
			_TTY_LOG_ONVIF_DEBUG("ASSERT: %s (Line : %d)",__FUNCTION__, __LINE__);
		}
	}


	return cmd;
}

void PTZOperationStop(int ch, int zoom_stop, int pantilt_stop)
{
	int i = 0;
	NF_PTZ_CMD ptz_cmd={0,};
	ptz_cmd.ch = ch;
	ptz_cmd.params[0] = 1;
	// if(zoom_stop==1)
	// 	ptz_cmd.params[IS_ZOOM_STOP] = 1;
	// if(pantilt_stop==1)
	// 	ptz_cmd.params[IS_PANTILT_STOP] = 1;

	for (i = 0;i < 5;i++)
	{
		ptz_cmd.cmd = NF_PTZ_CMD_STOP;
		nf_ptz_cmd(&ptz_cmd);
	}
	_TTY_LOG_ONVIF_DEBUG("nf_ptz_cmd stop activated %d cmd : %d", ptz_cmd.ch, ptz_cmd.cmd);

}

void pushPTZControlOperation(NF_PTZ_CMD *ptz_cmd, int operation)
{
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);

	int i = 0;
	_TTY_LOG_ONVIF("ch %d cmd %d params %d", ptz_cmd->ch, ptz_cmd->cmd, ptz_cmd->params[0]);

	if (ptz_cmd->cmd == NF_PTZ_CMD_SET_PRESET ||
	        ptz_cmd->cmd == NF_PTZ_CMD_CLEAR_PRESET ||
	        ptz_cmd->cmd == NF_PTZ_CMD_GOTO_PRESET ||
	        ptz_cmd->cmd == NF_PTZ_CMD_FOCUS_NEAR ||
	        ptz_cmd->cmd == NF_PTZ_CMD_FOCUS_FAR

	   )
	{
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		nf_ptz_cmd(ptz_cmd);
	}
	else
	{
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		nf_ptz_cmd(ptz_cmd);

		if (operation == CONTINUOUS)
		{
		}
		else
		{
			PTZOperationStop(ptz_cmd->ch, 1, 1);
		}
	}
}


char get_size_from_rect(int width, int height)
{
    int i = 0;
    
    for(i=0; i<MAX_RESOLUTION_CNT; i++){
        if( ipx_resolution[i].width == width && ipx_resolution[i].height == height ){
            return ipx_resolution[i].resolutionString;
        }
    }

    if( i == MAX_RESOLUTION_CNT ) return 0;
}

int get_size_string_from_rect(char *size, int ch, int width, int height)
{
    char tmp = 0;
    if( size == NULL )  return 0;
    if( ch < 0 || ch > 15 )  return 0;

    snprintf(size, sizeof(char)*17, nf_sysdb_get_str_nocopy("rec.panic.size") );

    if( width == 0 || height == 0)  return 0;
    else if( width == -1 && height == -1) return 1; // size will not be changed.
    else{
        tmp = get_size_from_rect(width, height);
        if( tmp == 0 ) return 0;
        *(size+ch) = tmp;
    }
    
    return 1;        
}

char get_fps_from_interval(int interval, int limit)
{
    int i = 0, tmp=0;

    if( interval == -1) {
        tmp = limit;
    }
    else{
        tmp = (int)(limit /(interval+1));
    }
    
    if (DISPLAY_IS_PAL) {
        // PAL
        for (i = 0; i < MAX_FPS; i++) {
        	_TTY_LOG_ONVIF_DEBUG("fps_pal[i].fps : %d (limit : %d)",fps_pal[i].fps,limit);
            if ( fps_pal[i].fps == limit) {
                return fps_pal[i].fpsCh;
            }
        }

    }
    else {
        // NTSC
        for (i = 0; i < MAX_FPS; i++) {
        	_TTY_LOG_ONVIF_DEBUG("fps_nt[i].fps : %d (limit : %d)",fps_nt[i].fps,limit);
            if ( fps_nt[i].fps == limit) {

                return fps_nt[i].fpsCh;
            }
        }
    }
    _TTY_LOG_ONVIF_DEBUG(" i: %d (MAX_FPS : %d)",i,MAX_FPS);
    if ( i == MAX_FPS ) return 0;
}

int convert_onvif_codec(int codec)
{
	if(codec == ONVIF_VIDEO_CODEC_JPEG)
		return 2;
	if (codec == ONVIF_VIDEO_CODEC_H264)
		return 1;
	if (codec == ONVIF_VIDEO_CODEC_H265)
		return 3;
	else //none
		return 0;
}

int convert_onvif_h264_profile(char *buff, int profile)
{
	switch( profile ){
		case ONVIF_H264_PROFILE_BASELINE:
			snprintf(buff, sizeof(char)*COMMON_SIZE-1, "Baseline");
			return 0;
		case ONVIF_H264_PROFILE_MAIN:
			snprintf(buff, sizeof(char)*COMMON_SIZE-1, "Main");
			return 1;
		case ONVIF_H264_PROFILE_HIGH:
			snprintf(buff, sizeof(char)*COMMON_SIZE-1, "High");
			return 3;			
		default:
			snprintf(buff, sizeof(char)*COMMON_SIZE-1, "High");
			return 1;
	}
	return 0;	
}

int check_onvif_quality(int quality)
{
	int ret = 0;

	if( quality != -1 && (quality > ONVIF_VENCODER_MAX_QUALITY || quality < ONVIF_VENCODER_MIN_QUALITY) ){
        	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d) quality[%d]",__FUNCTION__, __LINE__, quality);
		return 0;
	}
	return 1;
}

guint ext_get_resolution( guint w, guint h );

int check_onvif_width_height(int width, int height)
{
	guint res = 0;

	// res = ext_get_resolution(width, height);

	if( res == NF_RES_NTSC_NONE){
		return 0;
	}
	return 1;	
}
int check_onvif_fps_limit_interval(int interval, int limit)
{
	if( DISPLAY_IS_PAL ){
		if( interval > ONVIF_VENCODER_MAX_INTERVAL || 
			interval < ONVIF_VENCODER_MIN_INTERVAL ||
			limit > ONVIF_VENCODER_MAX_FPS_PAL ||
			limit < ONVIF_VENCODER_MIN_FPS_PAL ){
	        	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d) interval[%d] limit[%d]",__FUNCTION__, __LINE__, interval, limit);
	        	return 0;
		}
	}
	else {
		if( interval > ONVIF_VENCODER_MAX_INTERVAL || 
			interval < ONVIF_VENCODER_MIN_INTERVAL ||
			limit > ONVIF_VENCODER_MAX_FPS ||
			limit < ONVIF_VENCODER_MIN_FPS ){
	        	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d) interval[%d] limit[%d]",__FUNCTION__, __LINE__, interval, limit);
	        	return 0;
		}		
	}
      	return 1;
}

int get_fps_string_from_interval(char *fps, int ch, int interval, int limit)
{
    char tmp = 0;
    if( fps == NULL )  return 0;
    if( ch < 0 || ch > 15 )  return 0;

    snprintf(fps, sizeof(char)*17, nf_sysdb_get_str_nocopy("rec.panic.fps") );


    if( interval == -1 && limit == -1) return 1; // size will not be changed.
    else{
        tmp = get_fps_from_interval(interval, limit);
        if( tmp == 0 ) {
        	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
        	return 0;
        }
        *(fps+ch) = tmp;
    }
    
    return 1;        
}

char get_quality_from_q(int q)
{
    int i=0;
    
    for(i=0; i<MAX_QUALITY; i++){
        if ( quality_table[i].qual == q) {
            return quality_table[i].qualCh;
        }    
    }

    if( i == MAX_QUALITY )  return 0;
}

int get_quality_string_from(char *quality, int ch, int q)
{
    char tmp = 0;
    if( quality == NULL )  return 0;
    if( ch < 0 || ch > 15 )  return 0;

    snprintf(quality, sizeof(char)*17, nf_sysdb_get_str_nocopy("rec.panic.quality") );

    if( q == -1 ) return 1; // size will not be changed.
    else{
        tmp = get_quality_from_q(q);
        if( tmp == 0 ) return 0;
        *(quality+ch) = tmp;
    }
    return 1;        
}

int get_h264_options(arg_voption *voption, char *capa_string, int ch, int stream)
{
	int start = 0, end = MAX_RESOLUTION_CNT, cnt = 0, r_index = 0;

	gchar fps_capa[30] = { 0, };

	voption->codec = ONVIF_H264_CODEC;

	int rLength = strlen(capa_string);
	int cur_max_fps = 0;
	int width = 0, height = 0;

	guint64 tmp_fps_capa, tmp_fps_cur;
	
	for (cnt = 0 ; cnt < rLength; cnt++)
	{
		onvif_StrToResolution(onvif_get_size_info(capa_string[cnt]), &width, &height);
		voption->resolution[cnt].width  = width;
		voption->resolution[cnt].height = height;
	}
	voption->resolution_cnt = rLength;

	//voption->h264_profile_cnt = ONVIF_VENCODER_H264_PROFILE_CNT;
	if(ONVIF_VENCODER_H264_PROFILE_CNT > 0)	voption->h264_profile[0] = ONVIF_H264_PROFILE_HIGH;
	voption->h264_profile_cnt++;

	if(ONVIF_VENCODER_H264_PROFILE_CNT > 1)	voption->h264_profile[1] = ONVIF_H264_PROFILE_MAIN;
	voption->h264_profile_cnt++;
	if(ONVIF_VENCODER_H264_PROFILE_CNT > 2)	voption->h264_profile[2] = ONVIF_H264_PROFILE_HIGH;
	voption->h264_profile_cnt++;

	// if (DISPLAY_IS_PAL) {
	// 	voption->max_fps = ONVIF_VENCODER_MAX_FPS_PAL;
	// 	voption->min_fps = ONVIF_VENCODER_MIN_FPS_PAL;		
	// }
	// else{
		voption->max_fps = ONVIF_VENCODER_MAX_FPS;		
		voption->min_fps = ONVIF_VENCODER_MIN_FPS;
	// }
	

	voption->max_fps_interval = ONVIF_VENCODER_MAX_INTERVAL;
	voption->min_fps_interval = ONVIF_VENCODER_MIN_INTERVAL;

	voption->max_gop = ONVIF_VENCODER_MAX_GOV;
	voption->min_gop = ONVIF_VENCODER_MIN_GOV;
}

int get_h265_options(arg_voption *voption, char *capa_string, int ch, int stream)
{
	int start = 0, end = MAX_RESOLUTION_CNT, cnt = 0, r_index = 0;
	int cur_max_fps = 0;
	gchar fps_capa[30] = { 0, };

	guint64 tmp_fps_capa, tmp_fps_cur;

	voption->codec = ONVIF_VIDEO_CODEC_H265;

	int rLength = strlen(capa_string);
	
	for (cnt = 0 ; cnt < rLength; cnt++)
	{
		r_index = getResolutionIndex(capa_string[cnt]);
		voption->resolution[cnt].height = ipx_resolution[r_index].height;
		voption->resolution[cnt].width = ipx_resolution[r_index].width;
	}
	voption->resolution_cnt = rLength;

	voption->h264_profile_cnt = 1;//ONVIF_VENCODER_H264_PROFILE_CNT;
	voption->h264_profile[0] = ONVIF_H264_PROFILE_MAIN;
	
	// if (DISPLAY_IS_PAL) {
	// 	voption->max_fps = ONVIF_VENCODER_MAX_FPS_PAL;
	// 	voption->min_fps = ONVIF_VENCODER_MIN_FPS_PAL;		
	// }
	// else{
		voption->max_fps = ONVIF_VENCODER_MAX_FPS;		
		voption->min_fps = ONVIF_VENCODER_MIN_FPS;
	// }

	voption->max_fps_interval = ONVIF_VENCODER_MAX_INTERVAL;
	voption->min_fps_interval = ONVIF_VENCODER_MIN_INTERVAL;

	voption->max_gop = ONVIF_VENCODER_MAX_GOV;
	voption->min_gop = ONVIF_VENCODER_MIN_GOV;
}
int get_jpeg_options(arg_voption *voption, char *capa_string, int ch, int stream)
{
    int cnt=0, r_index=0;

    voption->codec = ONVIF_JPEG_CODEC;

    // Get Resolution options
    //getRuntime(0, &capa,capa_string);
    /*
	if ( nf_dev_board_pp_is_pal() ){
		strcpy(capa_string, NVS_RES_CAPA_PAL);
	}
	else {
		strcpy(capa_string, NVS_RES_CAPA_NTSC);		
	}
	*/
    int rLength = strlen(capa_string);

    for (cnt = 0 ; cnt < rLength; cnt++)
    {
        r_index = getResolutionIndex(capa_string[cnt]);
        voption->resolution[cnt].height = ipx_resolution[r_index].height;
        voption->resolution[cnt].width = ipx_resolution[r_index].width;
    }
    voption->resolution_cnt = rLength;

	if (DISPLAY_IS_PAL) {
		voption->max_fps = ONVIF_VENCODER_MAX_FPS_PAL;
		voption->min_fps = ONVIF_VENCODER_MIN_FPS_PAL;		
	}
	else{
		voption->max_fps = ONVIF_VENCODER_MAX_FPS;		
		voption->min_fps = ONVIF_VENCODER_MIN_FPS;
	}

	voption->max_fps_interval = ONVIF_VENCODER_MAX_INTERVAL/2.0;
	voption->min_fps_interval = ONVIF_VENCODER_MIN_INTERVAL;
	voption->max_gop = ONVIF_VENCODER_MAX_GOV;
	voption->min_gop = ONVIF_VENCODER_MIN_GOV;
}

unsigned int ip_str_to_uint(const char *temp)
{
    struct in_addr addr;
    unsigned int ip;

    if(inet_aton(temp, &addr) == 0)
        return 0;

    ip = (unsigned int)addr.s_addr;

    return ((ip & 0xFF000000) >> 24) | ((ip & 0x00FF0000) >> 8) |
        ((ip & 0x0000FF00) << 8) | ((ip & 0x000000FF) << 24);
}

gboolean nf_debug_hexdump_tty(gpointer p, int len)
{
    static char line[] =
        "00000000  xx xx xx xx  xx xx xx xx  "
        "xx xx xx xx  xx xx xx xx  yyyyyyyy yyyyyyyy";
    static char hex[] = "0123456789abcdef";
    char *s;
    int thistime;
    char *l;

    int col;
	int base = 0;
	
	_TTY_LOG_ONVIF( "--------------------------------------------------------------------------------");    
//Debug("0         1         2         3         4         5         6         7");
//Debug("0123456789012345678901234567890123456789012345678901234567890123456789012345678");
//Debug("xxxxxxxx  xx xx xx xx  xx xx xx xx  xx xx xx xx  xx xx xx xx  ........ ........");

    base -= (int)p;
    for (s = p; len; len -= thistime) {
        sprintf(line, "%08x", (unsigned int)(s + base));
        line[8] = ' ';
        thistime = len > 16 ? 16 : len;

        l = line + 10;
        for (col = 0; col < thistime; col++) {
            *l++ = hex[(*s & 0xf0) >> 4];
            *l++ = hex[*s++ & 0x0f];
            l += ((col & 3) == 3) + 1;
        }
        while (col < 16) {
            *l = l[1] = ' ';
            l += ((col++ & 3) == 3) + 3;
        }

        s -= thistime;
        for (col = 0; col < thistime; col++) {
            *l++ = isprint(*s) ? *s : '.';
            l += col == 7;
            s++;
        }
        while (col < 16) {
            *l++ = ' ';
            l += col++ == 7;
        }

	_TTY_LOG_ONVIF("%s",line);
    }

	_TTY_LOG_ONVIF("--------------------------------------------------------------------------------");    
    return 0; 
    	
}

gboolean get_dhcpon_ipaddr()
{
    if( nf_sysdb_get_bool("net.proto.dhcpon" ))  
        return 1;
    else
        return 0;
//    return nf_sysdb_get_bool("onvif.common.ipaddr_dhcp");
}

gboolean get_dhcpon_host()
{
    if( nf_sysdb_get_bool("net.proto.dhcpon" ))  return 1;
    return nf_sysdb_get_bool("onvif.common.host_dhcp");    
}

gboolean get_dhcpon_dns()
{
    if( nf_sysdb_get_bool("net.proto.dhcpon" ))  return 1;
    return nf_sysdb_get_bool("onvif.common.dns_dhcp");
}

gboolean get_dhcpon_ntp()
{
    if( nf_sysdb_get_bool("net.proto.dhcpon" ))  return 1;
    return nf_sysdb_get_bool("onvif.common.ntp_dhcp");    
}

void set_dhcp(void)
{
	char cmd[256] = {0};
    int ret = 0;
	char *eth_str = nf_onvif_get_eth_str();

    if( nf_sysdb_get_bool("net.proto.dhcpon" ) == 0 ){
#ifdef USE_PROXY_SYSTEM
        proxy_system("killall -9 udhcpc", PROXY_TYPE_SYSTEM, CMD_RUN);  // choissi
#else
        system("killall -9 udhcpc");  // choissi
#endif

	snprintf(cmd, sizeof(cmd), "udhcpc -n -i %s", eth_str);

#ifdef USE_PROXY_SYSTEM
        ret = proxy_system(cmd, PROXY_TYPE_SYSTEM, LONG_RUN);
#else
        ret = system(cmd);
#endif
    }
}

static guint if_get_ip(const char *dev)
{
    int   sockfd;
    unsigned long addr = (unsigned long)-1;

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
    close(sockfd);

    return (addr);
}

static guint if_get_dns(int index)
{
	struct sockaddr_in def = _res.nsaddr_list[index];
    char *ip = inet_ntoa(def.sin_addr);
  	
  	g_message("DNS[%d] IP = %s  [0x%08x]", index, ip, def.sin_addr); 
  	
    return *(unsigned int*)&def.sin_addr;	
}


int set_dhcp_and_get_dns(unsigned int *d1, unsigned int *d2)
{
    gint ret=0;
    guint dns1=0, dns2=0;

    set_dhcp();
    // get network information.
    res_init();

    dns1 = if_get_dns(0);
    dns2 = if_get_dns(1);
    *d1 = ntohl(dns1);
    *d2 = ntohl(dns2);
//    nf_sysdb_set_uint(DHCP_DNS1,        ntohl(dns1) );
//    nf_sysdb_set_uint(DHCP_DNS2,        ntohl(dns2) );
//    nf_sysdb_set_uint(NET_DNS1,     ntohl(dns1) );
//    nf_sysdb_set_uint(NET_DNS2,     ntohl(dns2) );

    return 0;
}

int set_dhcp_and_get_hostname(char *host)
{
    gint ret=0;
    guint ipaddr=0;
    char buff[32] = {0,};

    set_dhcp();

    // get network information.
    res_init();
    ipaddr = ntohl( if_get_ip( nf_onvif_get_eth_str() ) );
    snprintf(host, sizeof(char)*16, "%d.%d.%d.%d", PRINT_IP(ipaddr));

    return 0;
}

unsigned int dvrReady_info(void)
{
    gint dvr_online = 0;
    gboolean dvr_ready = 0;
    gboolean fs_is_online = 0;

    fs_is_online = nf_notify_get_param0("fs_status");

    if (nf_notify_get_param0("dvr_status") == NF_DVR_STATUS_INIT)
    {
        dvr_ready = FALSE;
    }
    else
    {
        dvr_ready = TRUE;
    }

    // dvr_online 0 : fs is not online      1 : disk not exist   2 : online
    if ( !fs_is_online && !dvr_ready )
    {
        dvr_online = 0;
    }
    else if ( !fs_is_online && dvr_ready )
    {
        dvr_online = 1;
    }
    else
    {
        dvr_online = 2;
    }

#if defined(__ARCHIVING_DISCONNETC__)
    if ( NF_DISCONN_SVR_ARCHIVE == nf_network_get_stop_reason() )
    {
        dvr_online = 1;
    }
#endif	
    return dvr_online;
}


#ifdef _ADMIN_IPX_0412
	const char *g_str_model = "IPX_0412\0";
#elif _ADMIN_IPX_0824
	const char *g_str_model = "IPX_0824\0";
#elif _ADMIN_IPX_0824P
	const char *g_str_model = "IPX_0824P\0";
#elif _ADMIN_IPX_1648P
	const char *g_str_model = "IPX_1648P\0";
#elif _ADMIN_IPX_32M4E
	const char *g_str_model = "IPX_32M4E\0";
#elif _ADMIN_NVS2G16
	const char *g_str_model = "NET5516\0";
#elif _ADMIN_NVS2G08
	const char *g_str_model = "NET5508\0";
#elif _ADMIN_NVS2G04
	const char *g_str_model = "NET5504\0";
#elif _ADMIN_NVS2G01
	const char *g_str_model = "NET5501\0";
#else
	#define MODELNAME "DVR7G"
#endif

char *get_onvif_model_name()
{
	return g_str_model;
}

int onvif_db_factory_init()
{
	nf_sysdb_set_str("onvif.common.fwver", nf_sysdb_get_str_nocopy("sys.info.swver2nd"));
	nf_sysdb_set_str("onvif.common.hwid", nf_sysdb_get_str_nocopy("sys.info.model"));
	nf_sysdb_set_str("onvif.common.manufacturer", nf_sysdb_get_str_nocopy("sys.info.vendor"));
	nf_sysdb_set_str("onvif.common.model", nf_sysdb_get_str_nocopy("sys.info.model"));
	nf_sysdb_set_str("onvif.common.serial", "sys.info.vendor");	
}

int apply_onvif_relay_from_sysdb()
{
	char buff[COMMON_SIZE] = { 0, };
	int i = 0, relay_active=0, idle_state=0;
	
	return 1;
}

int regExp(char* string, regmatch_t* mt,char* spliter )
{
	regex_t mregex;
	int reti;
	char msgbuf[COMMON_SIZE];

	_TTY_LOG_ONVIF_DEBUG("string: %s (Line : %d)", string, __LINE__);
	
		reti = regcomp(&mregex, spliter, 0);
		if( reti ){ _TTY_LOG_ONVIF_DEBUG("Could not compile regex\n"); }
	
			reti = regexec(&mregex, string, 1, mt, 0);
			if( !reti ){
			regfree(&mregex);				
			return 1;
	
			}	
			regfree(&mregex);

			return -1;

}

void get_ptz_config(arg_PTZConfig *tmp) {
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE];
	int ptz_cnt = ONVIF_PTZ_CNT;
	int i = 0;
	int use_ptz = 0;
	arg_GetCapa capa;
	memset(&capa, 0x00, sizeof(arg_GetCapa));

	for (i = 0; i < ONVIF_MAX_PROFILE_CNT; i++) {
		sprintf(buff, "onvif.profile%d.ptz", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		if (!strcmp(tmp->token, tmp_token))
		{
			/* name */
			sprintf(buff, "onvif.ptz%d.name", i%ONVIF_CH);
			strncpy(tmp->name, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			/* node_token */
			sprintf(buff, "onvif.ptz%d.node_token", i%ONVIF_CH);
			strncpy(tmp->node_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			sprintf(buff, "onvif.profile%d.ptz", i%ONVIF_CH);
			strncpy(tmp->token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			/* range */ 
//			tmp->P_min = -0.1;
//			tmp->P_max = 0.1;
//			tmp->T_min = -0.1;
//			tmp->T_max = 0.1;
//			tmp->Z_min = -0.1;
//			tmp->Z_max = 0.1;

			/* default speed */ 
//			tmp->P_speed = 1;
//			tmp->T_speed = 1;
//			tmp->Z_speed = 1;
			GetCapability(&capa, i);
			capa.ch = i;
			/* Space */
			if(capa.ptSupport[i]){
				strncpy(tmp->def_continous_PTSpace, CONTINUOUS_PTV_SPACE, COMMON_SIZE *2);
			}
			if(capa.zoomSupport[i]){
				strncpy(tmp->def_continous_ZSpace, CONTINUOUS_ZV_SPACE, COMMON_SIZE *2);
			}

			if(capa.ptSupport[i] || capa.zoomSupport[i])
			{
				tmp->timeout = 0;
				/* use count */
				tmp->use_count = get_ptz_usecount(tmp->token);
				//tmp->use_count = 2;
			}
			else
			{
				tmp->use_count = 0;
			}
			// Due to the PELCO scenario, if RS485 is not checked in webRA, PTZ information is not sent.
			snprintf(buff, sizeof(buff), "cam.ptz.P%d.rs485", i);
			// use_ptz = (int) nf_sysdb_get_bool(buff);
			use_ptz = 1;
			tmp->use_ptz = use_ptz;
			/* Space */
//			sprintf(buff, "onvif.ptz%d.timeout", i);
//			tmp->timeout = 1000 * (unsigned int)nf_sysdb_get_uint(buff);
			
			tmp->result = 1;
			
			break;
		}
	}
	if (i == ptz_cnt) {
		tmp->result = 0;
		printf("\e[31m #### NOT FOUND TOKEN[%s] ####\e[0m\n", tmp->token);
		fprintf(stderr, "%s Function %d(line) Token Not Found ERROR\n", __FUNCTION__, __LINE__);
	}
}

int get_ptz_usecount(char *token) {
	int profile_cnt, usecount = 0;
	int i;
	char buff[COMMON_SIZE] = { 0, };
	char tmp_token[COMMON_SIZE] = { 0, };

	profile_cnt = MAX_PROFILE;//nf_sysdb_get_uint("onvif.common.profile_cnt");

	for (i = 0; i < profile_cnt; i++) {
		sprintf(buff, "onvif.profile%d.ptz", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(token, tmp_token))
			usecount++;
	}

	return usecount;
}
#ifdef NVR
void get_cam_capa_resol(int ch, char *capa_string, int stream)
{
	char f_cap_resol[17] = {0,};
	char s_cap_resol[17] = {0,};
	
	// RESOL_INFO_T resol;
	// memset(&resol, 0, sizeof(resol));

	NFIPCamEncoderCap info;
	memset(&info, 0, sizeof(NFIPCamEncoderCap));

	onvif_get_cam_resol_profile(ch, &info);
	
	// First, Second Stream ������ ������ ���� ���� ��? ������, First(H264)�� 2�� ��?? Second(JPEG) Second Stream �������� Capability ����

	// s_cap_resol[0] = f_cap_resol[strlen(f_cap_resol) - 1];
	// f_cap_resol[1] = NULL;

	if(!stream)
	{
		// translate_data_size_capable(info.res_support[0], f_cap_resol);
		resol_bitmask_to_char(info.res_support[0], DISPLAY_IS_PAL, f_cap_resol);
		strcpy(capa_string, f_cap_resol);
	}
	else
	{
		// translate_data_size_capable(info.res_support[1], f_cap_resol);
		resol_bitmask_to_char(info.res_support[1], DISPLAY_IS_PAL, s_cap_resol);
		strcpy(capa_string, s_cap_resol);
	}
}
#endif

gchar onvif_get_qual_data(gchar *info)
{
	if(!g_ascii_strcasecmp(info, "LOWEST")) return 'E';
	else if(!g_ascii_strcasecmp(info, "LOW")) return 'D';
	else if(!g_ascii_strcasecmp(info, "STANDARD")) return 'C';
	else if(!g_ascii_strcasecmp(info, "HIGH")) return 'B';
	else if(!g_ascii_strcasecmp(info, "HIGHEST")) return 'A';
	
	return '\0';
}

gchar onvif_get_fps_data(gint ch, gchar *info)
{
	if (scm_get_cam_signal() & (1 << ch))
	{
		if(!g_ascii_strcasecmp(info, "1")) return 'F';
		else if(!g_ascii_strcasecmp(info, "2")) return 'E';
		else if(!g_ascii_strcasecmp(info, "3")) return 'D';
		else if(!g_ascii_strcasecmp(info, "6")) return 'C';
		else if(!g_ascii_strcasecmp(info, "12")) return 'B';
		else if(!g_ascii_strcasecmp(info, "25")) return 'A';
		else if(!g_ascii_strcasecmp(info, "0")) return 'G';		
	}
	else
	{
		if(!g_ascii_strcasecmp(info, "1")) return 'F';
		else if(!g_ascii_strcasecmp(info, "2")) return 'E';
		else if(!g_ascii_strcasecmp(info, "3")) return 'D';
		else if(!g_ascii_strcasecmp(info, "7")) return 'C';
		else if(!g_ascii_strcasecmp(info, "15")) return 'B';
		else if(!g_ascii_strcasecmp(info, "30")) return 'A';		
		else if(!g_ascii_strcasecmp(info, "0")) return 'G';
	}
	
	return '\0';
}

gchar onvif_get_size_data(gchar *info)
{
	if(!g_ascii_strcasecmp(info, size_info[CAP_RES_NTSC_CIF])) 			return 'B';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_NTSC_2CIF])) 	return 'C';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_NTSC_4CIF])) 	return 'D';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_NTSC_4CIFP])) 	return 'E';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_PAL_CIF])) 		return 'F';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_PAL_2CIF])) 	return 'G';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_PAL_4CIF])) 	return 'H';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_PAL_4CIFP])) 	return 'I';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_640x480])) 		return 'J';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_720x480])) 		return 'K';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_720x576])) 		return 'L';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_800x600])) 		return 'M';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_1024x768])) 	return 'N';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_1280x1024])) 	return 'O';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_1600x1200])) 	return 'P';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_1280x720])) 	return 'Q';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_1920x1080])) 	return 'R';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_640x352])) 		return 'S';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_640x360])) 		return 'T';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_640x360I])) 	return 'U';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_1280x720I])) 	return 'V';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_1920x1080I])) 	return 'W';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_640x400])) 	    return 'X';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_800x450])) 	    return 'Y';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_1440x900])) 	return 'Z';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_960x480])) 	    return 'a';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_960x576])) 	    return 'b';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_320x180])) 	    return 'c';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_2304x1296])) 	return 'd';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_2048x1536])) 	return 'e';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_2560x1440])) 	return 'f';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_2688x1520])) 	return 'g';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_2560x1600])) 	return 'h';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_2560x1920])) 	return 'i';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_2592x1920])) 	return 'j';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_2592x1944])) 	return 'k';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_2992x1680])) 	return 'l';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_2880x1800])) 	return 'm';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_3200x1800])) 	return 'n';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_2880x2160])) 	return 'o';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_3072x2048])) 	return 'p';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_3200x2400])) 	return 'q';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_3840x2160])) 	return 'r';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_2592x1520])) 	return 's';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_3000x3000])) 	return '1';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_2048x2048])) 	return '2';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_1280x1280])) 	return '3';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_640x640])) 		return '4';
	else if(!g_ascii_strcasecmp(info, size_info[CAP_RES_320x320])) 		return '5';
}

gchar* onvif_get_size_info(gchar data)
{
	guint main_cap, main_cur;
	guint sec_cap, sec_cur;
	guint result;
	guint vloss_val;

	g_return_val_if_fail(data, NULL);

	switch(data) {
		case 'B':
			return size_info[CAP_RES_NTSC_CIF];
		case 'C':
			return size_info[CAP_RES_NTSC_2CIF];
		case 'D':
			return size_info[CAP_RES_NTSC_4CIF];
		case 'E':
			return size_info[CAP_RES_NTSC_4CIFP];
		case 'F':
			return size_info[CAP_RES_PAL_CIF];
		case 'G':
			return size_info[CAP_RES_PAL_2CIF];
		case 'H':
			return size_info[CAP_RES_PAL_4CIF];
		case 'I':
			return size_info[CAP_RES_PAL_4CIFP];
		case 'J':
			return size_info[CAP_RES_640x480];
		case 'K':
			return size_info[CAP_RES_720x480];
		case 'L':
			return size_info[CAP_RES_720x576];
		case 'M':
			return size_info[CAP_RES_800x600];
		case 'N':
			return size_info[CAP_RES_1024x768];
		case 'O':
			return size_info[CAP_RES_1280x1024];
		case 'P':
			return size_info[CAP_RES_1600x1200];
		case 'Q':
			return size_info[CAP_RES_1280x720];
		case 'R':
			return size_info[CAP_RES_1920x1080];
		case 'S':
			return size_info[CAP_RES_640x352];
		case 'T':
			return size_info[CAP_RES_640x360];
		case 'U':
			return size_info[CAP_RES_640x360I];
		case 'V':
			return size_info[CAP_RES_1280x720I];			
		case 'W':
			return size_info[CAP_RES_1920x1080I];
		case 'X':
			return size_info[CAP_RES_640x400];		
		case 'Y':
			return size_info[CAP_RES_800x450];		
		case 'Z':
			return size_info[CAP_RES_1440x900];		
		case 'a':
			return size_info[CAP_RES_960x480];		
		case 'b':
			return size_info[CAP_RES_960x576];					
		case 'c':
			return size_info[CAP_RES_320x180];
		case 'd':
			return size_info[CAP_RES_2304x1296];
		case 'e':
			return size_info[CAP_RES_2048x1536];
		case 'f':
			return size_info[CAP_RES_2560x1440];
		case 'g':
			return size_info[CAP_RES_2688x1520];
		case 'h':
			return size_info[CAP_RES_2560x1600];
		case 'i':
			return size_info[CAP_RES_2560x1920];
		case 'j':
			return size_info[CAP_RES_2592x1920];
		case 'k':
			return size_info[CAP_RES_2592x1944];
		case 'l':
			return size_info[CAP_RES_2992x1680];
		case 'm':
			return size_info[CAP_RES_2880x1800];
		case 'n':
			return size_info[CAP_RES_3200x1800];
		case 'o':
			return size_info[CAP_RES_2880x2160];
		case 'p':
			return size_info[CAP_RES_3072x2048];
		case 'q':
			return size_info[CAP_RES_3200x2400];
		case 'r':
			return size_info[CAP_RES_3840x2160];
		case 's':
			return size_info[CAP_RES_2592x1520];
		case '1':
			return size_info[CAP_RES_3000x3000];
		case '2':
			return size_info[CAP_RES_2048x2048];
		case '3':
			return size_info[CAP_RES_1280x1280];
		case '4':
			return size_info[CAP_RES_640x640];
		case '5':
			return size_info[CAP_RES_320x320];
		default :
		    g_message("%s, %d, data:%c", __FUNCTION__, __LINE__, data);
			g_assert(0);
			break;
	}

	return NULL;
}
void makeConfiguration(arg_PTZConfig *tmp, int ch)
{
	arg_GetCapa capa;
	memset(&capa, 0x00, sizeof(arg_GetCapa));
	GetCapability(&capa, ch);
	capa.ch = ch;

	tmp->use_count = 1;
	tmp->timeout = capa.ptzContinuousTimeOutDefault; 

	strncpy(tmp->token, TOKEN_PTZ_CONF, sizeof(tmp->token));
	strncpy(tmp->name,  TOKEN_PTZ_CONF, sizeof(tmp->name));
	strncpy(tmp->node_token, TOKEN_PTZ_NODE,sizeof(tmp->node_token));

	if(capa.ptSupport[ch])
	{
		//DefaultAbsolutePantTiltPositionSpace
		strncpy(tmp->def_absolute_PTSpace,  ABSOLUTE_PTV_SPACE, sizeof(tmp->def_absolute_PTSpace));
		//DefaultRelativePantTiltTranslationSpace
		strncpy(tmp->def_relative_PTSpace,  RELATIVE_PTV_SPACE, sizeof(tmp->def_absolute_PTSpace));
		//DefaultContinuousPanTiltVelocitySpace
		strncpy(tmp->def_continous_PTSpace, CONTINUOUS_PTV_SPACE,sizeof(tmp->def_continous_PTSpace));
	}
	if(capa.zoomSupport[ch])
	{
		//DefaultAbsoluteZoomPositionSpace	
		strncpy(tmp->def_absolute_ZSpace,   ABSOLUTE_ZV_SPACE,  sizeof(tmp->def_absolute_ZSpace));
		//DefaultRelativeZoomTranslationSpace	
		strncpy(tmp->def_relative_ZSpace,   RELATIVE_ZV_SPACE,  sizeof(tmp->def_absolute_ZSpace));
		//DefaultContinuousZoomVelocitySpace
		strncpy(tmp->def_continous_ZSpace,  CONTINUOUS_ZV_SPACE, sizeof(tmp->def_continous_ZSpace));
	}
}
gboolean onvif_SetDbUint(char *_dbName, int _ch, gint _data)
{
  char buf[128];

  snprintf(buf, sizeof(buf), _dbName, _ch);

  return nf_sysdb_set_uint(buf, (guint)_data);
}
gboolean onvif_camera_connect_check(int ch)
{
	NFIPCamConfInfo info;
	nf_ipcam_get_config_info(ch, &info, NULL);
	if(info.video)
		return 1;
	else
		return 0;
}
gboolean onvif_is_ptz(int ch, uint64_t category)
{
	gboolean support=0;
	int vloss_flag=0;
	vloss_flag = nf_notify_get_param0("vloss");

	if(vloss_flag & (1 << ch))
		return 0;
	
	nf_ipcam_get_ptz_support(ch, category, &support);
	
	if(support)
		return 1;
	else
		return 0;
}
gboolean onvif_is_audio(int ch, int type)
{
	NFIPCamConfInfo info;
	nf_ipcam_get_config_info(ch, &info, NULL);
	if(type == ONVIF_AUDIO_IN_SUPPORTED)
	{
		if(info.audio_out)			// audio in
			return 1;
		else
			return 0;
	}
	else if(ONVIF_AUDIO_OUT_SUPPORTED)
	{
		if(info.audio_in)			// audio back channel
			return 1;
		else
			return 0;	
	}
}
gboolean onvif_is_alarm(int ch, int type)
{
	NFIPCamConfInfo info;
	nf_ipcam_get_config_info(ch, &info, NULL);
	if(type == ONVIF_ALARM_IN_SUPPORTED)
	{
		if(info.alarm_in)			// alarm in
			return 1;
		else
			return 0;
	}
	else if(ONVIF_ALARM_OUT_SUPPORTED)
	{
		if(info.alarm_out)			// alarm out - relay
			return 1;
		else
			return 0;	
	}
}
void onvif_cam_connect_db_save(int ch)
{
	char buff[COMMON_SIZE] = {0,};
	char db_char_value[COMMON_SIZE] = {0,};
	
	if(onvif_camera_connect_check(ch))
	{
		printf("\e[31m @@ CAM[%d] Supported Video @@\e[0m\n", ch);
		// vsource, vencoder
		snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.vsource", ch);
		snprintf(db_char_value, COMMON_SIZE - 1, "vsconfig%d", ch);
		nf_sysdb_set_str(buff, db_char_value);

		snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.vsource", ch + ONVIF_CH);
		nf_sysdb_set_str(buff, db_char_value);

		snprintf(buff, COMMON_SIZE - 1, "onvif.vsource%d.name", ch);
		snprintf(db_char_value, COMMON_SIZE - 1, "vs%d", ch);
		nf_sysdb_set_str(buff, db_char_value);

		snprintf(buff, COMMON_SIZE - 1, "onvif.vsource%d.token", ch);
		snprintf(db_char_value, COMMON_SIZE - 1, "vsconfig%d", ch);
		nf_sysdb_set_str(buff, db_char_value);

		snprintf(buff, COMMON_SIZE - 1, "onvif.vsource%d.source_token", ch);
		snprintf(db_char_value, COMMON_SIZE - 1, "vs%d", ch);
		nf_sysdb_set_str(buff, db_char_value);

		snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.vencoder", ch);
		snprintf(db_char_value, COMMON_SIZE - 1, "ve%d", ch);
		nf_sysdb_set_str(buff, db_char_value);

		snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.vencoder", ch + ONVIF_CH);
		snprintf(db_char_value, COMMON_SIZE - 1, "ve%d", ch + ONVIF_CH);
		nf_sysdb_set_str(buff, db_char_value);
		// first
		snprintf(buff, COMMON_SIZE - 1, "onvif.vencoder%d.token", ch);
		snprintf(db_char_value, COMMON_SIZE - 1, "ve%d", ch);
		nf_sysdb_set_str(buff, db_char_value);

		snprintf(buff, COMMON_SIZE - 1, "onvif.vencoder%d.name", ch);
		snprintf(db_char_value, COMMON_SIZE - 1, "ve%d", ch);
		nf_sysdb_set_str(buff, db_char_value);
		//second
		snprintf(buff, COMMON_SIZE - 1, "onvif.vencoder%d.token", ch + ONVIF_CH);
		snprintf(db_char_value, COMMON_SIZE - 1, "ve%d", ch + ONVIF_CH);
		nf_sysdb_set_str(buff, db_char_value);

		snprintf(buff, COMMON_SIZE - 1, "onvif.vencoder%d.name", ch + ONVIF_CH);
		snprintf(db_char_value, COMMON_SIZE - 1, "ve%d", ch + ONVIF_CH);
		nf_sysdb_set_str(buff, db_char_value);

		snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.metadata", ch);
		snprintf(db_char_value, COMMON_SIZE - 1, "m%d", ch);
		nf_sysdb_set_str(buff, db_char_value);

		snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.metadata", ch + ONVIF_CH);
		nf_sysdb_set_str(buff, db_char_value);

		snprintf(buff, COMMON_SIZE - 1, "onvif.metadata%d.token", ch);
		snprintf(db_char_value, COMMON_SIZE - 1, "m%d", ch);
		nf_sysdb_set_str(buff, db_char_value);

		snprintf(buff, COMMON_SIZE - 1, "onvif.metadata%d.name", ch);
		snprintf(db_char_value, COMMON_SIZE - 1, "m%d", ch);
		nf_sysdb_set_str(buff, db_char_value);
	}
	if(onvif_is_audio(ch, ONVIF_AUDIO_IN_SUPPORTED))
	{
		printf("\e[31m @@ CAM[%d] Supported AudioIN @@\e[0m\n", ch);
		//asource
		snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.asource", ch);
		snprintf(db_char_value, COMMON_SIZE - 1, "asconfig%d", ch);
		nf_sysdb_set_str(buff, db_char_value);

		snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.asource", ch + ONVIF_CH);
		nf_sysdb_set_str(buff, db_char_value);

		snprintf(buff, COMMON_SIZE - 1, "onvif.asource%d.token", ch);
		snprintf(db_char_value, COMMON_SIZE - 1, "asconfig%d", ch);
		nf_sysdb_set_str(buff, db_char_value);

		snprintf(buff, COMMON_SIZE - 1, "onvif.asource%d.name", ch);
		snprintf(db_char_value, COMMON_SIZE - 1, "as%d", ch);
		nf_sysdb_set_str(buff, db_char_value);

		snprintf(buff, COMMON_SIZE - 1, "onvif.asource%d.source_token", ch);
		snprintf(db_char_value, COMMON_SIZE - 1, "as%d", ch);
		nf_sysdb_set_str(buff, db_char_value);

	}
	if(onvif_is_audio(ch, ONVIF_AUDIO_OUT_SUPPORTED))
	{
		printf("\e[31m @@ CAM[%d] Supported AudioOUT @@\e[0m\n", ch);
		//aeoncoder	
		snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.aencoder", ch);
		snprintf(db_char_value, COMMON_SIZE - 1, "ae%d", ch);
		nf_sysdb_set_str(buff, db_char_value);

		snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.aencoder", ch + ONVIF_CH);
		nf_sysdb_set_str(buff, db_char_value);

		snprintf(buff, COMMON_SIZE - 1, "onvif.aencoder%d.name", ch);
		snprintf(db_char_value, COMMON_SIZE - 1, "ae%d", ch);
		nf_sysdb_set_str(buff, db_char_value);

		snprintf(buff, COMMON_SIZE - 1, "onvif.aencoder%d.token", ch);
		snprintf(db_char_value, COMMON_SIZE - 1, "ae%d", ch);
		nf_sysdb_set_str(buff, db_char_value);
	}
	if(onvif_is_ptz(ch, NF_IPCAM_IMAGE_PAN) || onvif_is_ptz(ch, NF_IPCAM_IMAGE_ZOOM))
	{
		printf("\e[31m @@ CAM[%d] Supported PTZ @@\e[0m\n", ch);
		// ptz
		snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.ptz", ch);
		snprintf(db_char_value, COMMON_SIZE - 1, "pc%d", ch);
		nf_sysdb_set_str(buff, db_char_value);

		snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.ptz", ch + ONVIF_CH);
		nf_sysdb_set_str(buff, db_char_value);
		
		snprintf(buff, COMMON_SIZE - 1, "onvif.ptz%d.name", ch);
		snprintf(db_char_value, COMMON_SIZE - 1, "pc%d", ch);
		nf_sysdb_set_str(buff, db_char_value);

		snprintf(buff, COMMON_SIZE - 1, "onvif.ptz%d.token", ch);
		snprintf(db_char_value, COMMON_SIZE - 1, "pc%d", ch);
		nf_sysdb_set_str(buff, db_char_value);

		snprintf(buff, COMMON_SIZE - 1, "onvif.ptz%d.node_token", ch);
		snprintf(db_char_value, COMMON_SIZE - 1, "ptz%d", ch);
		nf_sysdb_set_str(buff, db_char_value);
	}
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
}
void onvif_cam_disconnect_db_remove(int ch)
{
	char buff[COMMON_SIZE] = {0,};
	
	// vsource, vencoder
	snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.vsource", ch);
	nf_sysdb_set_str(buff, "");

	snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.vsource", ch + ONVIF_CH);
	nf_sysdb_set_str(buff, "");

	snprintf(buff, COMMON_SIZE - 1, "onvif.vsource%d.name", ch);
	nf_sysdb_set_str(buff, "");

	snprintf(buff, COMMON_SIZE - 1, "onvif.vsource%d.token", ch);
	nf_sysdb_set_str(buff, "");

	snprintf(buff, COMMON_SIZE - 1, "onvif.vsource%d.source_token", ch);
	nf_sysdb_set_str(buff, "");

	snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.vencoder", ch);
	nf_sysdb_set_str(buff, "");

	snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.vencoder", ch + ONVIF_CH);
	nf_sysdb_set_str(buff, "");
	// first
	snprintf(buff, COMMON_SIZE - 1, "onvif.vencoder%d.token", ch);
	nf_sysdb_set_str(buff, "");

	snprintf(buff, COMMON_SIZE - 1, "onvif.vencoder%d.name", ch);
	nf_sysdb_set_str(buff, "");
	// second
	snprintf(buff, COMMON_SIZE - 1, "onvif.vencoder%d.token", ch + ONVIF_CH);
	nf_sysdb_set_str(buff, "");

	snprintf(buff, COMMON_SIZE - 1, "onvif.vencoder%d.name", ch + ONVIF_CH);
	nf_sysdb_set_str(buff, "");
	
	snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.metadata", ch);
	nf_sysdb_set_str(buff, "");

	snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.metadata", ch + ONVIF_CH);
	nf_sysdb_set_str(buff, "");

	snprintf(buff, COMMON_SIZE - 1, "onvif.metadata%d.token", ch);
	nf_sysdb_set_str(buff, "");

	snprintf(buff, COMMON_SIZE - 1, "onvif.metadata%d.name", ch);
	nf_sysdb_set_str(buff, "");

	//asource
	snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.asource", ch);
	nf_sysdb_set_str(buff, "");

	snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.asource", ch + ONVIF_CH);
	nf_sysdb_set_str(buff, "");

	snprintf(buff, COMMON_SIZE - 1, "onvif.asource%d.token", ch);
	nf_sysdb_set_str(buff, "");

	snprintf(buff, COMMON_SIZE - 1, "onvif.asource%d.name", ch);
	nf_sysdb_set_str(buff, "");

	snprintf(buff, COMMON_SIZE - 1, "onvif.asource%d.source_token", ch);
	nf_sysdb_set_str(buff, "");

	//aeoncoder	
	snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.aencoder", ch);
	nf_sysdb_set_str(buff, "");

	snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.aencoder", ch + ONVIF_CH);
	nf_sysdb_set_str(buff, "");

	snprintf(buff, COMMON_SIZE - 1, "onvif.aencoder%d.name", ch);
	nf_sysdb_set_str(buff, "");

	snprintf(buff, COMMON_SIZE - 1, "onvif.aencoder%d.token", ch);
	nf_sysdb_set_str(buff, "");

	// ptz
	snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.ptz", ch);
	nf_sysdb_set_str(buff, "");

	snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.ptz", ch + ONVIF_CH);
	nf_sysdb_set_str(buff, "");

	snprintf(buff, COMMON_SIZE - 1, "onvif.ptz%d.name", ch);
	nf_sysdb_set_str(buff, "");

	snprintf(buff, COMMON_SIZE - 1, "onvif.ptz%d.token", ch);
	nf_sysdb_set_str(buff, "");

	snprintf(buff, COMMON_SIZE - 1, "onvif.ptz%d.node_token", ch);
	nf_sysdb_set_str(buff, "");

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
}
void onvif_db_sync(int ch, int type)
{
	if(type == ONVIF_CAM_CONNECTED)
	{
		printf("\e[31m ##### %s : CONNECTED CH[%d] #####\e[0m\n", __FUNCTION__, ch);
		onvif_cam_connect_db_save(ch);		
	}
	else if(type == ONVIF_CAM_DISCONNECTED)
	{
		printf("\e[31m ##### %s : DISCONNECTED CH[%d] #####\e[0m\n", __FUNCTION__, ch);
		onvif_cam_disconnect_db_remove(ch);
	}	
	else
		printf("### %s : %d INVALID TYPE ####\n", __FUNCTION__, __LINE__);
}
void onvif_db_init()
{
	printf("########## ONVIF DB INIT START ##########\n");
	char buff[COMMON_SIZE] = {0,};
	int ch=0;
	for(ch=0;ch<ONVIF_CH;ch++)
	{
		// vsource, vencoder
		snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.vsource", ch);
		nf_sysdb_set_str(buff, "");

		snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.vsource", ch + ONVIF_CH);
		nf_sysdb_set_str(buff, "");

		snprintf(buff, COMMON_SIZE - 1, "onvif.vsource%d.name", ch);
		nf_sysdb_set_str(buff, "");

		snprintf(buff, COMMON_SIZE - 1, "onvif.vsource%d.token", ch);
		nf_sysdb_set_str(buff, "");

		snprintf(buff, COMMON_SIZE - 1, "onvif.vsource%d.source_token", ch);
		nf_sysdb_set_str(buff, "");
		//first
		snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.vencoder", ch);
		nf_sysdb_set_str(buff, "");

		snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.vencoder", ch + ONVIF_CH);
		nf_sysdb_set_str(buff, "");

		snprintf(buff, COMMON_SIZE - 1, "onvif.vencoder%d.token", ch);
		nf_sysdb_set_str(buff, "");

		snprintf(buff, COMMON_SIZE - 1, "onvif.vencoder%d.name", ch);
		nf_sysdb_set_str(buff, "");
		// second
		snprintf(buff, COMMON_SIZE - 1, "onvif.vencoder%d.token", ch + ONVIF_CH);
		nf_sysdb_set_str(buff, "");

		snprintf(buff, COMMON_SIZE - 1, "onvif.vencoder%d.name", ch + ONVIF_CH);
		nf_sysdb_set_str(buff, "");

		snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.metadata", ch);
		nf_sysdb_set_str(buff, "");

		snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.metadata", ch + ONVIF_CH);
		nf_sysdb_set_str(buff, "");

		snprintf(buff, COMMON_SIZE - 1, "onvif.metadata%d.token", ch);
		nf_sysdb_set_str(buff, "");

		snprintf(buff, COMMON_SIZE - 1, "onvif.metadata%d.name", ch);
		nf_sysdb_set_str(buff, "");

		//asource
		snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.asource", ch);
		nf_sysdb_set_str(buff, "");

		snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.asource", ch + ONVIF_CH);
		nf_sysdb_set_str(buff, "");

		snprintf(buff, COMMON_SIZE - 1, "onvif.asource%d.token", ch);
		nf_sysdb_set_str(buff, "");

		snprintf(buff, COMMON_SIZE - 1, "onvif.asource%d.name", ch);
		nf_sysdb_set_str(buff, "");

		snprintf(buff, COMMON_SIZE - 1, "onvif.asource%d.source_token", ch);
		nf_sysdb_set_str(buff, "");

		//aeoncoder	
		snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.aencoder", ch);
		nf_sysdb_set_str(buff, "");

		snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.asource", ch + ONVIF_CH);
		nf_sysdb_set_str(buff, "");

		snprintf(buff, COMMON_SIZE - 1, "onvif.aencoder%d.name", ch);
		nf_sysdb_set_str(buff, "");

		snprintf(buff, COMMON_SIZE - 1, "onvif.aencoder%d.token", ch);
		nf_sysdb_set_str(buff, "");

		// ptz
		snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.ptz", ch);
		nf_sysdb_set_str(buff, "");

		snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.ptz", ch + ONVIF_CH);
		nf_sysdb_set_str(buff, "");

		snprintf(buff, COMMON_SIZE - 1, "onvif.ptz%d.name", ch);
		nf_sysdb_set_str(buff, "");

		snprintf(buff, COMMON_SIZE - 1, "onvif.ptz%d.token", ch);
		nf_sysdb_set_str(buff, "");

		snprintf(buff, COMMON_SIZE - 1, "onvif.ptz%d.node_token", ch);
		nf_sysdb_set_str(buff, "");
	}
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
	printf("########## ONVIF DB INIT END ##########\n");
}
int nf_onvif_find_rule_id(int type, int ch, int id)
{
	int i = 0;
	char buff[COMMON_SIZE];

	if ( type < ONVIF_CLASSIC_VA_ZONE || type > ONVIF_BUILT_IN_VA)
		return -1;
	
	if ( type == ONVIF_CLASSIC_VA_ZONE )
	{
		// Classic VA Zone
		for ( i = 0; i < IVCA_MAX_ZONES; i++ )
		{
			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "cam.vca.rule.R%d.Z%d.id", ch, i);
			if ( nf_sysdb_get_int(buff) == id )
				return i;
		}
		
	}
	if ( type == ONVIF_CLASSIC_VA_COUNTER )
	{
		// Classic VA Counter
		for ( i = 0; i < IVCA_MAX_CNTRS; i++ )
		{
			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "cam.vca.rule.R%d.C%d.id", ch, i);
			if ( nf_sysdb_get_int(buff) == id )
				return i;
		}
		
	}
	if ( type == ONVIF_AI_ZONE )
	{
		// Classic VA Counter
		for ( i = 0; i < IVCA_MAX_ZONES; i++ )
		{
			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "cam.dvabx.rule.R%d.Z%d.id", ch, i);
			if ( nf_sysdb_get_int(buff) == id )
				return i;
		}
		
	}
	if ( type == ONVIF_AI_COUNTER )
	{
		// Classic VA Counter
		for ( i = 0; i < IVCA_MAX_CNTRS; i++ )
		{
			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "cam.dvabx.rule.R%d.C%d.id", ch, i);
			if ( nf_sysdb_get_int(buff) == id )
				return i;
		}
	}
	return -1;
}

int get_codec_value(guint codec)
{
	switch(codec)
	{
		case 2:
			return ONVIF_VIDEO_CODEC_H264;
		case 4:
			return ONVIF_VIDEO_CODEC_H265;
		default:
			return ONVIF_VIDEO_CODEC_H264;
	}
}