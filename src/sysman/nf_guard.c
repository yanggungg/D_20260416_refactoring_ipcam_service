#include "nf_common.h"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <errno.h>
#include <linux/hdreg.h>
	
#include <openssl/engine.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>

#include "curl/curl.h"
#include <curl/easy.h>

#include "nf_guard.h"
#include "jbshell.h"
#include "nf_logevtdef.h"


#if defined(_IPX_1648M4) || defined(_IPX_0824M4) || defined(_IPX_0412M4) || defined(_IPX_1648P4E) || defined(_IPX_0824P4E) \
 || defined(_IPX_0412M4E) || defined(_IPX_0824M4E) ||defined(_IPX_1648M4E) || defined(_IPX_32P4E)  || defined(_IPX_32M4E) || defined(_IPX_32P5)

#define GUARD_PROTOCOL	"https"
#define GUARD_DOMAIN	"anti-piracy.sequrinet.com"
#define GUARD_IP		"52.68.147.79:3000"

#define URI_STATE		"dev/v1/info/state"
#define URI_INFO		"dev/v1/info"
#define URI_BLOCK		"dev/v1/block"

typedef enum
{
	REQ_GET			= 0,
	REQ_POST,
	REQ_CUSTOM_GET,
	REQ_MAX,
} NF_GUARD_METHOD;

typedef enum
{
	BLK_MAC_ADDR_INVALID,
	BLK_INPUT_PARAM_NULL,
	BLK_HW_PARAM_NULL,
	BLK_SERIAL_NOT_MATCH,
	BLK_SERIAL_MATCH,
	BLK_SERVER_RET_BLOCK,
	BLK_SERVER_RET_UNBLOCK,
	BLK_SERVER_RET_NONE,
	BLK_MAX
} NF_GUARD_BLOCK_REASON;

unsigned char blk_reason_str[BLK_MAX][256] = {
	"BLK_MAC_ADDR_INVALID",
	"BLK_INPUT_PARAM_NULL",
	"BLK_HW_PARAM_NULL",
	"BLK_SERIAL_NOT_MATCH",
	"BLK_SERIAL_MATCH",
	"BLK_SERVER_RET_BLOCK",
	"BLK_SERVER_RET_UNBLOCK",
	"BLK_SERVER_RET_NONE",
};

typedef struct _NF_GUARD_STATE_INFO{
	guint scene;
	guint timeout;
} NF_GUARD_STATE_INFO;

typedef struct _NF_GUARD_DEVICE_INFO{
	guint scene;
	gchar mac_addr[128];
	gchar serial_match[128];
	gchar serial_rand[128];
	gchar auth_code[128];
	gchar model[128];
	gchar ip_addr[128];
	guint web_port;
	gchar fw[128];
	gchar uuid[8][128];
	guint boot_cnt;
	gchar boot_date[128];
} NF_GUARD_DEVICE_INFO;

typedef struct _NF_GUARD_BLOCK_INFO{
	gchar mac_addr[128];
	gchar auth_code[128]; 
} NF_GUARD_BLOCK_INFO;

typedef struct _NF_GUARD_BLOCK_RET{
	gboolean block;
} NF_GUARD_BLOCK_RET;

static int _sysdb_reload_flag = 0;
static gchar boot_time[256];

static void _log_put_device_block(gint block, NF_GUARD_BLOCK_REASON blk_reason)
{
	GTimeVal tv;
	g_get_current_time( &tv);
	gchar text[256] = {0};
	
	snprintf(&text, sizeof(text), "%s:%s", block ? "BLOCK" : "UNBLOCK", blk_reason_str[blk_reason]);
	nf_eventlog_put_param(&tv, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_CONNECT_USER /*LP2_SYSTEM_DEBUG_NAND_LOG_STRING*/, text);
}
static gint _get_hdd_serial(guchar *serial, guint slen)
{
	#define HDD_CNT 4
	
	static struct hd_driveid hd;
	gint fd = 0, i = 0, j = 0;
	guchar hd_tbl[HDD_CNT][9] = {"/dev/sda", "/dev/sdb", "/dev/sdc", "/dev/sdd"};
	
	for(i = 0; i < HDD_CNT; i++)
	{
		if ((fd = open(hd_tbl[i], O_RDONLY | O_NONBLOCK)) >= 0)
		{
			break;
		}
		
		g_printf("[%s] open failed.\n", hd_tbl[i]);
	}
	if(i == HDD_CNT)
	{
		return -1;
	}
	
	memset(&hd, 0, sizeof(hd));
	if (!ioctl(fd, HDIO_GET_IDENTITY, &hd))
	{
		for(i = 0; i < strlen(hd.serial_no); i++)
		{
			if(i >= slen) break;

			if(hd.serial_no[i] != 32)
			{
				serial[j++] = hd.serial_no[i];
			}
		}
	}
	else if (errno == -ENOMSG)
	{
		printf("No serial number available\n");
		return -1;
	}
	else
	{
		perror("ERROR: HDIO_GET_IDENTITY");
		return -1;
	}
	return 0;
}

static guchar* get_hdd_serial(void)
{
	static guchar serial[32] = {0};
	static gint serial_get = 0;
	
	if(serial_get == 0)
	{
		serial_get = 1;
		_get_hdd_serial(serial, 32);
	}

	return serial;
}

static gboolean _get_sysdb_auth_code(char *buff, int buff_size)
{
	char *sysdb;

	memset(buff, 0x0, buff_size);	
	
	sysdb = nf_sysdb_get_str_nocopy("sys.info.guard.authcode");
		
	if(sysdb)
	{
		snprintf(buff, buff_size, "%s", sysdb);
	}

	return TRUE;
}

static guint _get_sysdb_last_scene()
{
	guint ret;

	ret = nf_sysdb_get_uint("sys.info.guard.last_scene");

	return ret;
}

static gboolean _set_sysdb_last_scene(guint last_scene)
{
	nf_sysdb_set_uint("sys.info.guard.last_scene", last_scene);
	nf_sysdb_save("sys");

	return TRUE;
}

static gboolean _get_sysdb_policy()
{
	gboolean ret;

	ret = nf_sysdb_get_bool("sys.info.agr_policy");

	return ret;
}

static gboolean _get_sysdb_dev_block()
{
	gboolean ret;

	ret = nf_sysdb_get_bool("sys.info.guard.dev_block");

	return ret;
}

static gboolean _set_sysdb_dev_block(gboolean dev_block, NF_GUARD_BLOCK_REASON reason)
{
	g_printf("GUARD : DEV_BLOCK => %d\n", dev_block);

	if (_get_sysdb_dev_block() != dev_block)
	{
		_log_put_device_block(dev_block, reason);
		
		nf_sysdb_set_bool("sys.info.guard.dev_block", dev_block);
		nf_notify_fire_params("sysdb_change",NF_SYSDB_CATE_SYS, 0, 0, 0);
		nf_sysdb_save("sys");
	}

	return TRUE;
}

static guint _get_sysdb_boot_cnt()
{
	guint ret;

	ret = nf_sysdb_get_uint("sys.info.guard.boot_cnt");

	return ret;
}

static gboolean _set_sysdb_boot_cnt(guint cnt)
{
	nf_sysdb_set_uint("sys.info.guard.boot_cnt", cnt);
	nf_sysdb_save("sys");

	return TRUE;
}

size_t _guard_request_write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

static int _guard_request_to_server(char *svr_path, char *loc_path, NF_GUARD_METHOD type, char *post_data, GError **error)
{
    CURL *curl;	
	int ret = 0;

	g_assert(svr_path);
	g_assert(loc_path);

	g_message("%s - START => path:%s", __FUNCTION__, svr_path);

#if 1
	if( post_data )
		g_message("%s - post_data:%s", __FUNCTION__, post_data);
#endif

    curl = curl_easy_init();

	remove(loc_path);
	
    if (curl) {
	    FILE *fp;
		
        fp = fopen(loc_path,"wb");

		if(fp)
		{
		    CURLcode res;			
		    struct curl_slist *chunk = NULL;
			char header[256];	
	
			gchar err_str[CURL_ERROR_SIZE];
		
			snprintf(header, sizeof(header), "Content-Type: application/json");
	    	chunk = curl_slist_append(chunk, header);

	        curl_easy_setopt(curl, CURLOPT_URL, svr_path);
	        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _guard_request_write_data);
	        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

			curl_easy_setopt(curl, CURLOPT_USERNAME, "device");
			curl_easy_setopt(curl, CURLOPT_PASSWORD, "device");
			curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);				

			if( type == REQ_CUSTOM_GET ){
		        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
		        curl_easy_setopt(curl, CURLOPT_POST, 1L);				
				curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
			}
			else if( type == REQ_POST ){
		        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
		        curl_easy_setopt(curl, CURLOPT_POST, 1L);
			}

			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);				
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
			curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, err_str);

		    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
			curl_easy_setopt(curl, CURLOPT_MAXFILESIZE, 10*1000*1000);	
			curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
			curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 60);
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20);

	        res = curl_easy_perform(curl);

			if( res == 0 )
				ret = 1;

		    curl_slist_free_all(chunk);					
			
        	fclose(fp);	
		}
		else{
			g_message("%s - file open error", __FUNCTION__);
		}

        curl_easy_cleanup(curl);
    }
	else{
		g_message("%s - curl init error", __FUNCTION__);
	}

	g_message("%s - END => ret:%d", __FUNCTION__, ret);
	
	return ret;
}

static int _get_converted_mac_str(gchar *buff, int buff_size)
{
	gchar *mac = NULL;
	int count = 0;
	int i = 0, j = 0;

	memset(buff, 0x0, buff_size); 

	mac = nf_sysdb_get_str_nocopy("sys.info.mac");

	if( strlen(mac) != 12 )
		return -1;

	if( buff_size < 18 )
		return -1;

	for(i=0; i < 12; i++)
	{
		if(count == 2)
		{
			count = 0;
			buff[j] = ':';			
			j++;
		}
		
		buff[j] = mac[i];
		j++;
		count++;
	}

	g_message("%s - %s", __FUNCTION__, buff);

	return 0;
}

static gint _make_block_info(NF_GUARD_BLOCK_INFO *info)
{
	gchar mac[64];
	
	if( _get_converted_mac_str(mac, sizeof(mac)) < 0 )
		return -1;
	else{
		snprintf(info->mac_addr, sizeof(info->mac_addr), "%s", mac);

		_get_sysdb_auth_code(info->auth_code, sizeof(info->auth_code));
		
//		snprintf(info->auth_code, sizeof(info->auth_code), "1234");
	}

	return 0;
}

static gint _convert_block_info_to_json(NF_GUARD_BLOCK_INFO *info, char *buff, gint buff_size) 
{
	char *pos = buff;

	pos += snprintf(pos, buff_size-((guint)pos-(guint)buff), "{");
	pos += snprintf(pos, buff_size-((guint)pos-(guint)buff), "\"mac_addr\":\"%s\",", info->mac_addr);
	pos += snprintf(pos, buff_size-((guint)pos-(guint)buff), "\"auth_code\":\"%s\"", info->auth_code);
	pos += snprintf(pos, buff_size-((guint)pos-(guint)buff), "}");

	return 0;
}

static gint _make_dev_info(NF_GUARD_DEVICE_INFO *info)
{
	gchar mac[64];
	gchar *sysdb = NULL;
	gchar extern_ip[64];

	memset(	mac, 0x0, sizeof(mac));
	memset(	extern_ip, 0x0, sizeof(extern_ip));
	
	if( _get_converted_mac_str(mac, sizeof(mac)) < 0 )
		return -1;
	else{	
		info->scene = _get_sysdb_last_scene();	
		snprintf(info->mac_addr, sizeof(info->mac_addr), "%s", mac);

		//sysdb = nf_sysman_get_serial_match(); 
		//snprintf(info->serial_match, sizeof(info->serial_match), "%s", sysdb);

		//sysdb = nf_sysman_get_serial_rand(); 
		//snprintf(info->serial_rand, sizeof(info->serial_rand), "%s", sysdb);

		_get_sysdb_auth_code(info->auth_code, sizeof(info->auth_code));

		sysdb = nf_sysdb_get_str_nocopy("sys.info.model");
		snprintf(info->model, sizeof(info->model), "%s", sysdb);

		if( nf_get_external_ip(extern_ip) != 0 ){
			memset(extern_ip, 0x0, sizeof(extern_ip));
		}
		snprintf(info->ip_addr, sizeof(info->ip_addr), "%s", extern_ip);

		info->web_port = nf_sysdb_get_uint("net.proto.webport");

		sysdb = nf_sysdb_get_str_nocopy("sys.info.swver");
		snprintf(info->fw, sizeof(info->fw), "%s", sysdb);

	//	uuid_info	
	//	snprintf(info->fw, sizeof(info->fw), "51110.2.6555.100");

		info->boot_cnt = _get_sysdb_boot_cnt();
		snprintf(info->boot_date, sizeof(info->boot_date), "%s", boot_time);
	}
	
	return 0;
}

static gint _convert_dev_info_to_json(NF_GUARD_DEVICE_INFO *info, char *buff, gint buff_size)
{
	char *pos = buff;
	char uuid_info[256];

	snprintf(uuid_info, sizeof(uuid_info), "{\"uuid\" : \"%s\"}", get_hdd_serial());

	pos += snprintf(pos, buff_size-((guint)pos-(guint)buff), "{");
	pos += snprintf(pos, buff_size-((guint)pos-(guint)buff), "\"scene\" : %u,", info->scene);
	pos += snprintf(pos, buff_size-((guint)pos-(guint)buff), "\"mac_addr\" : \"%s\",", info->mac_addr);
	pos += snprintf(pos, buff_size-((guint)pos-(guint)buff), "\"serial_match\" : \"%s\",", info->serial_match);
	pos += snprintf(pos, buff_size-((guint)pos-(guint)buff), "\"serial_rand\" : \"%s\",", info->serial_rand);
	pos += snprintf(pos, buff_size-((guint)pos-(guint)buff), "\"auth_code\" : \"%s\",", info->auth_code);
	pos += snprintf(pos, buff_size-((guint)pos-(guint)buff), "\"model\" : \"%s\",", info->model);
	pos += snprintf(pos, buff_size-((guint)pos-(guint)buff), "\"ip_addr\" : \"%s\",", info->ip_addr);
	pos += snprintf(pos, buff_size-((guint)pos-(guint)buff), "\"web_port\" : %u,", info->web_port);
	pos += snprintf(pos, buff_size-((guint)pos-(guint)buff), "\"fw\" : \"%s\",", info->fw);
	pos += snprintf(pos, buff_size-((guint)pos-(guint)buff), "\"uuidinfo\" : [%s],", uuid_info);
	pos += snprintf(pos, buff_size-((guint)pos-(guint)buff), "\"boot_cnt\" : %u,", info->boot_cnt);
	pos += snprintf(pos, buff_size-((guint)pos-(guint)buff), "\"boot_date\" : \"%s\"", info->boot_date);
	pos += snprintf(pos, buff_size-((guint)pos-(guint)buff), "}");
	
	return 0;
}

static int _read_state_info(gchar *loc_path, NF_GUARD_STATE_INFO *stat_info)
{
	gchar  *contents = NULL;
	GError *error = NULL;	
	gsize  length = 0;
	int ret = -1;	

	if (!g_file_get_contents (loc_path, &contents, &length, &error))
	{
		g_warning("%s\n", error->message);
		g_error_free (error);

		if(contents)
			g_free(contents);
		ret = -1;
	}
	else
	{
		if(contents)
		{
			char *p;
			char *scene;
			char *timeout;
			char temp[64];
			
			snprintf(temp, sizeof(temp), "\"scene\"");
			
			scene = strstr(contents, temp);
			if( scene )
			{			
				scene += strlen(temp);

				while(isspace(*scene) || (*scene == ':'))
					scene = scene + 1;
				
				stat_info->scene = strtoul(scene, NULL, 10);
				
				snprintf(temp, sizeof(temp), "\"timeout\"");			
		
				timeout = strstr(contents, temp);

				if( timeout )
				{
					timeout += strlen(temp);

					while(isspace(*timeout) || (*timeout == ':'))
						timeout = timeout + 1;

					stat_info->timeout = strtoul(timeout, NULL, 10);

					ret = 0;
				}
				else
				{
					ret = -1;
				}	
			}
			else
			{
				ret = -1;
			}

			g_free(contents);
		}
		else
		{
			ret = -1;
		}
	}

	return ret;
}

static int _read_blk_ret(gchar *loc_path, NF_GUARD_BLOCK_RET *blk_ret)
{
	gchar  *contents = NULL;
	GError *error = NULL;	
	gsize  length = 0;
	int ret = -1;	

	if (!g_file_get_contents (loc_path, &contents, &length, &error))
	{
		g_warning("%s\n", error->message);
		g_error_free (error);

		if(contents)
			g_free(contents);
		ret = -1;
	}	
	else
	{
		if(contents)
		{
			char *p;
			char *block;
			char temp[64];
			char buff[256];
			
			snprintf(temp, sizeof(temp), "\"block\"");
			
			block = strstr(contents, temp);
			if( block )
			{			
				block += strlen(temp);

				while(isspace(*block) || (*block == ':'))
					block = block + 1;

				memset(buff, 0x0, sizeof(buff));

				p = buff;

				while(isalpha(*block))
				{
					*p = *block;
					p++;
					block++;
				}

				if(!strcmp(buff, "true"))
				{
					blk_ret->block = 1;
					ret = 0;
				}
				else if(!strcmp(buff, "false"))
				{
					blk_ret->block = 0;
					ret = 0;
				}
				else
				{
					ret = -1;
				}
			}
			else
			{
				ret = -1;
			}

			g_free(contents);
		}
		else
		{
			ret = -1;
		}
	}

	return ret;	
}

static int _read_device_info_ret(gchar *loc_path)
{
	gchar  *contents = NULL;
	GError *error = NULL;	
	gsize  length = 0;
	int ret = -1;	

	if (!g_file_get_contents (loc_path, &contents, &length, &error))
	{
		g_warning("%s\n", error->message);
		g_error_free (error);

		if(contents)
			g_free(contents);
		ret = -1;
	}	
	else
	{
#if 0 //todo
		if(contents) //eeprom overwrite
		{
			char *ps = NULL;
			char *pe = NULL;
			unsigned char base64enc[1024] = {0};
			unsigned char base64dec[1024] = {0};
			int slen = 0, i = 0;
			
			ps = strstr(contents, "eeprom"); //{"status":200,"result":"success","eeprom":"..."}
			if(!ps) return 0;
			ps += 9;
			
			pe = strstr(ps, "\"");
			if(!pe) return -1;
			
			slen = pe - ps;
			strncpy(base64enc, ps, slen);
			slen = Base64decode(base64dec, base64enc);

			ret = nf_board_pp_set_eeprom(0, 0, slen, base64dec);
			g_printf("e2prom len[%d] => ret[%d]\n", slen, ret);			
			ret = 0;
		}
#endif
	}

	return ret;	
}

static void _make_server_path_by_uri(char *uri, char *ret_path, int ret_path_size)
{
	struct addrinfo	hints, *res_hints = NULL;
	
	bzero(&hints, sizeof(struct addrinfo));

	hints.ai_family =  AF_INET; // AF_UNSPEC;  AF_INET,AF_INET6
	hints.ai_socktype = SOCK_STREAM;

	if ( getaddrinfo(GUARD_DOMAIN, NULL, &hints, &res_hints) == 0 )
	{
		freeaddrinfo(res_hints);
		
		snprintf(ret_path, ret_path_size, "%s://%s/%s", GUARD_PROTOCOL, GUARD_DOMAIN, uri);
	}
	else
	{
		snprintf(ret_path, ret_path_size, "%s://%s/%s", GUARD_PROTOCOL, GUARD_IP, uri);
	}
}

static int _guard_recv_state_info(NF_GUARD_STATE_INFO *stat_info)
{
	int guard_ret = 0;
	int ret = -1;
	char loc_path[1024];
	char server_path[1024];

	memset(stat_info, 0x0, sizeof(NF_GUARD_STATE_INFO));
	
	snprintf(loc_path, sizeof(loc_path), "/tmp/get.tmp");

	_make_server_path_by_uri(URI_STATE, server_path, sizeof(server_path));
	
	guard_ret = _guard_request_to_server(server_path, loc_path, REQ_GET, NULL, NULL);

	g_message("GUARD - %s - GUARD_RET = %d", __FUNCTION__, guard_ret);

	if(guard_ret == 1)
	{
		if( _read_state_info(loc_path, stat_info) < 0 )
			ret = -1;
		else
			ret = 0;
	}
	else
	{
		ret = -1;
	}	

	return ret;
}

static int _guard_send_device_info()
{
	int ret;
	char buff[1024];
	char loc_path[1024];
	char server_path[1024];	
	NF_GUARD_DEVICE_INFO info;
	
	memset(&info, 0x0, sizeof(NF_GUARD_DEVICE_INFO));

	snprintf(loc_path, sizeof(loc_path), "/tmp/post.tmp");

	_make_dev_info(&info);
	_convert_dev_info_to_json(&info, buff, sizeof(buff));

	_make_server_path_by_uri(URI_INFO, server_path, sizeof(server_path));

	ret = _guard_request_to_server(server_path, loc_path, REQ_POST, buff, NULL);

	if(ret != 1)
		ret = -1;
	
	else if(_read_device_info_ret(loc_path) < 0)
		ret = -1;

	g_message("GUARD - %s - RET = %d", __FUNCTION__, ret);
	
	return 0;
}

static int _guard_verify_device(NF_GUARD_BLOCK_RET *blk_ret)
{
	int guard_ret = 0;	
	int ret = -1;
	char buff[1024];
	char loc_path[1024];
	char server_path[1024];		
	NF_GUARD_BLOCK_INFO info;	

	memset(blk_ret, 0x0, sizeof(NF_GUARD_BLOCK_RET));

	memset(buff, 0x0, sizeof(buff));
	memset(loc_path, 0x0, sizeof(loc_path));
	memset(&info, 0x0, sizeof(NF_GUARD_BLOCK_INFO));	

	snprintf(loc_path, sizeof(loc_path), "/tmp/cus_get.tmp");

	_make_block_info(&info);
	_convert_block_info_to_json(&info, buff, sizeof(buff));

	_make_server_path_by_uri(URI_BLOCK, server_path, sizeof(server_path));
		
	guard_ret = _guard_request_to_server(server_path, loc_path, REQ_CUSTOM_GET, buff, NULL);
	
	g_message("GUARD - %s - GUARD_RET = %d", __FUNCTION__, guard_ret);

	if(guard_ret == 1)
	{
		if( _read_blk_ret(loc_path, blk_ret) < 0 )
			ret = -1;
		else
			ret = 0;
	}
	else
	{
		ret = -1;
	}	

	return ret;
}


void encrypt_hmac_sha512(unsigned char *str, unsigned char *key, unsigned char *b64_buf)
{
		unsigned int enc_len = 128;
		unsigned char *enc_buf = (unsigned char*) malloc(sizeof(char) * enc_len);
		HMAC_CTX ctx;
		
		ENGINE_load_builtin_engines();
		ENGINE_register_all_complete();
		
		HMAC_CTX_init(&ctx);
		HMAC_Init_ex(&ctx, key, strlen(key), EVP_sha512(), NULL);
		HMAC_Update(&ctx, str, strlen(str));
		HMAC_Final(&ctx, enc_buf, &enc_len);
		HMAC_CTX_cleanup(&ctx);
		
		Base64encode(b64_buf, enc_buf, enc_len);
		free(enc_buf);
}

int get_serial_match(unsigned char *mac, unsigned char *serial_match)
{
		unsigned char *key = "eiyl248i239ol";
		unsigned char enc_buf[128] = {0};
		
		if(mac == NULL || serial_match == NULL)
			return -1;
		
		/* mac to lower */
		unsigned char *p = mac;
		for ( ; *p; ++p) *p = tolower(*p);
		
		/* get encrypted string */
		encrypt_hmac_sha512(mac, key, enc_buf);
		
		/* slice(-16) */
		strncpy(serial_match, enc_buf + strlen(enc_buf) - 16, 16);
		
		return 0;
}

static NF_GUARD_BLOCK_REASON _is_valid_serial_match()
{
	gchar mac[64];

	gchar *serial_match_curr;
	gchar serial_match_want[32];

	memset(serial_match_want, 0x0, sizeof(serial_match_want));

	if( _get_converted_mac_str(mac, sizeof(mac)) < 0 )
	{
		return BLK_MAC_ADDR_INVALID;
	}

	if(get_serial_match(mac, serial_match_want) < 0)
	{
		return BLK_INPUT_PARAM_NULL;
	}

/*	serial_match_curr = nf_sysman_get_serial_match();

	if( serial_match_curr == NULL )
	{
		return BLK_HW_PARAM_NULL;
	}*/ 

//	g_message("%s - serial_match_curr[%s]", __FUNCTION__, serial_match_curr);
//	g_message("%s - serial_match_want[%s]", __FUNCTION__, serial_match_want);	

		return BLK_SERIAL_MATCH;
	if( !strcmp(serial_match_curr, serial_match_want) )
	{
		g_message("%s - MATCH", __FUNCTION__);
		return BLK_SERIAL_MATCH;
	}
	else
	{
		g_message("%s - MISMATCH", __FUNCTION__);		
		return BLK_SERIAL_NOT_MATCH;
	}
}

static char guard_serial_match_help[] = "guard_serial_match [mac]";
static int guard_serial_match(int argc, char **argv)
{
	gchar *mac;
	gchar serial_match[32];

	if( argc < 2 )
	{
		g_message("%s - PARAM ERROR", __FUNCTION__);
		return 0;
	}

	mac = argv[1];
	
	if(get_serial_match(mac, serial_match) < 0)
	{
		g_message("%s - get_serial_match fail.", __FUNCTION__);
		return 0;
	}
	
	g_message("%s - serial_match => %s", __FUNCTION__, serial_match);
	
	return 0;
}
__commandlist(guard_serial_match, "guard_serial_match", guard_serial_match_help, guard_serial_match_help);

static void _notify_cb_sysdb_chnage( NF_NOTIFY_INFO *pinfo, gpointer data )
{		
	if(pinfo == NULL)
	{
		g_warning("%s pinfo is NULL!!", __FUNCTION__);
		return ;
	}

	if(pinfo->d.params[0] == NF_SYSDB_CATE_SYS){
		_sysdb_reload_flag = 1;
	}
}

#define STATE_CNT_MAX (24 * 60 * 60)
#define BLOCK_CNT_MAX (48 * 60 * 60)
//#define STATE_CNT_MAX (3 * 60)
//#define BLOCK_CNT_MAX (3 * 60)

static void _guard_information_thread()
{
	gboolean policy;
	guint boot_cnt;

	gint state_cnt;
	gint block_cnt;

	time_t     tm_time;
	struct tm *st_time;

 	time( &tm_time);
	st_time = gmtime( &tm_time);
	strftime(boot_time, sizeof(boot_time), "%Y%m%d%H%M%S", st_time);

	boot_cnt = _get_sysdb_boot_cnt();
	boot_cnt ++;
	_set_sysdb_boot_cnt(boot_cnt);
	
	state_cnt = STATE_CNT_MAX;
	block_cnt = BLOCK_CNT_MAX;

	policy = _get_sysdb_policy();

	g_message("%s - %d - policy = %d", __FUNCTION__, __LINE__, policy);

	while(1)
	{				
		if(_sysdb_reload_flag == 1 )
		{
			gboolean tmp_policy;
			
			_sysdb_reload_flag = 0;

			tmp_policy = _get_sysdb_policy();

			if(policy != tmp_policy)
			{
				policy = tmp_policy;

				if(policy == TRUE)
				{
					state_cnt = STATE_CNT_MAX;
					block_cnt = BLOCK_CNT_MAX;
				}
			}			
		}

		if( state_cnt >= STATE_CNT_MAX )
		{
			state_cnt = 0;
			
			if(policy == TRUE)
			{
				gint ret;
				NF_GUARD_STATE_INFO stat_info;
				
				ret = _guard_recv_state_info(&stat_info);

				if( ret == 0 )
				{
					guint last_scene;
					
					last_scene = _get_sysdb_last_scene();
					
					if( stat_info.scene != last_scene )
					{
						_set_sysdb_last_scene(stat_info.scene);
						_guard_send_device_info();
					}
				}
			}
		}

		if( block_cnt >= BLOCK_CNT_MAX )
		{
			gboolean should_block = FALSE;
			NF_GUARD_BLOCK_REASON reason;
			
			block_cnt = 0;
			
			reason = _is_valid_serial_match();
			if(reason == BLK_SERIAL_MATCH)
			{	
				reason = BLK_SERVER_RET_NONE;
				
				if(policy == TRUE)
				{				
					gint ret;				
					NF_GUARD_BLOCK_RET blk_ret;

					ret = _guard_verify_device(&blk_ret);
					if( ret == 0 )
					{
						should_block = blk_ret.block;
						reason = blk_ret.block ? BLK_SERVER_RET_BLOCK : BLK_SERVER_RET_UNBLOCK;
					}
				}
			}
			else
			{
				should_block = TRUE;
			}
			_set_sysdb_dev_block(should_block, reason);
		}	

		state_cnt++;
		block_cnt++;

		sleep(1);
	}
}

gboolean nf_guard_init(void)
{
	gulong cb_handle = 0;
    pthread_t tid = 0;

#ifdef DEV_BLK_SKIP
	return TRUE;
#endif
	
    if ( pthread_create(&tid, NULL, (void*)_guard_information_thread, NULL) != 0)
    {
        return FALSE;
    }	

    pthread_detach(tid);

	cb_handle= nf_notify_connect_cb( "sysdb_change", _notify_cb_sysdb_chnage, NULL);
	g_assert( cb_handle >0);

	return TRUE;
}

static char guard_request_help[] = "guard_request [opt]";
static int guard_request(int argc, char **argv)
{
	int ret;
	int type;
	char buff[1024];
	char loc_path[1024];

	memset(buff, 0x0, sizeof(buff));
	memset(loc_path, 0x0, sizeof(loc_path));	

	if( argc < 2 )
	{
		g_message("%s - ARGC error", __FUNCTION__);
		return 0;
	}

	type = atoi(argv[1]);

	if( type == REQ_CUSTOM_GET ){
/*		
		NF_GUARD_BLOCK_INFO info;

		memset(&info, 0x0, sizeof(NF_GUARD_BLOCK_INFO));		

		snprintf(loc_path, sizeof(loc_path), "/tmp/cus_get.tmp");

		_make_block_info(&info);
		_convert_block_info_to_json(&info, buff, sizeof(buff));
			
		ret = _guard_request_to_server("https://anti-piracy.sequrinet.com/dev/v1/block", loc_path, REQ_CUSTOM_GET, buff, NULL);

		g_message("KJH - %s - RET = %d", __FUNCTION__, ret);	
*/
		NF_GUARD_BLOCK_RET blk_ret;

		g_message("%s - %d", __FUNCTION__, __LINE__);

		if( _guard_verify_device(&blk_ret) < 0 )
		{
			g_message("%s - VERIFY RECV ERROR", __FUNCTION__);			
		}
		else
		{
			g_message("%s - VERIFY RECV OK => block:%d", __FUNCTION__, blk_ret.block);
		}		
	}
	else if( type == REQ_POST ){
/*		
		NF_GUARD_DEVICE_INFO info;
		
		memset(&info, 0x0, sizeof(NF_GUARD_DEVICE_INFO));

		snprintf(loc_path, sizeof(loc_path), "/tmp/post.tmp");
		
		_make_dev_info(&info);
		_convert_dev_info_to_json(&info, buff, sizeof(buff));

		ret = _guard_request_to_server("https://anti-piracy.sequrinet.com/dev/v1/info", loc_path, REQ_POST, buff, NULL);

		g_message("KJH - %s - RET = %d", __FUNCTION__, ret);
*/
		char cmd[1024];

		g_message("%s - %d", __FUNCTION__, __LINE__);

		snprintf(cmd, sizeof(cmd), "cat %s", "/tmp/post.tmp");

		_guard_send_device_info();

		proxy_system(cmd, 1, 3);
	}	
	else{
/*		
		snprintf(loc_path, sizeof(loc_path), "/tmp/get.tmp");

		ret = _guard_request_to_server("https://anti-piracy.sequrinet.com/dev/v1/info/state", loc_path, REQ_GET, NULL, NULL);

		g_message("KJH - %s - RET = %d", __FUNCTION__, ret);
*/
		NF_GUARD_STATE_INFO stat_info;

		g_message("%s - %d", __FUNCTION__, __LINE__);

		if( _guard_recv_state_info(&stat_info) < 0 )
		{
			g_message("%s - STATE RECV ERROR", __FUNCTION__);
		}
		else
		{
			g_message("%s - STATE RECV OK => scene:%u, timeout:%u", __FUNCTION__, stat_info.scene, stat_info.timeout);
		}
	}
}
__commandlist(guard_request, "guard_request", guard_request_help, guard_request_help);
#endif

static const unsigned char pr2six[256] =
{
    /* ASCII table */
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
    64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
    64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
};

int Base64decode_len(const char *bufcoded)
{
    int nbytesdecoded;
    register const unsigned char *bufin;
    register int nprbytes;

    bufin = (const unsigned char *) bufcoded;
    while (pr2six[*(bufin++)] <= 63);

    nprbytes = (bufin - (const unsigned char *) bufcoded) - 1;
    nbytesdecoded = ((nprbytes + 3) / 4) * 3;

    return nbytesdecoded + 1;
}

int Base64decode(char *bufplain, const char *bufcoded)
{
    int nbytesdecoded;
    register const unsigned char *bufin;
    register unsigned char *bufout;
    register int nprbytes;

    bufin = (const unsigned char *) bufcoded;
    while (pr2six[*(bufin++)] <= 63);
    nprbytes = (bufin - (const unsigned char *) bufcoded) - 1;
    nbytesdecoded = ((nprbytes + 3) / 4) * 3;

    bufout = (unsigned char *) bufplain;
    bufin = (const unsigned char *) bufcoded;

    while (nprbytes > 4) {
    *(bufout++) =
        (unsigned char) (pr2six[*bufin] << 2 | pr2six[bufin[1]] >> 4);
    *(bufout++) =
        (unsigned char) (pr2six[bufin[1]] << 4 | pr2six[bufin[2]] >> 2);
    *(bufout++) =
        (unsigned char) (pr2six[bufin[2]] << 6 | pr2six[bufin[3]]);
    bufin += 4;
    nprbytes -= 4;
    }

    /* Note: (nprbytes == 1) would be an error, so just ingore that case */
    if (nprbytes > 1) {
    *(bufout++) =
        (unsigned char) (pr2six[*bufin] << 2 | pr2six[bufin[1]] >> 4);
    }
    if (nprbytes > 2) {
    *(bufout++) =
        (unsigned char) (pr2six[bufin[1]] << 4 | pr2six[bufin[2]] >> 2);
    }
    if (nprbytes > 3) {
    *(bufout++) =
        (unsigned char) (pr2six[bufin[2]] << 6 | pr2six[bufin[3]]);
    }

    *(bufout++) = '\0';
    nbytesdecoded -= (4 - nprbytes) & 3;
    return nbytesdecoded;
}

static const char basis_64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int Base64encode_len(int len)
{
    return ((len + 2) / 3 * 4) + 1;
}

int Base64encode(char *encoded, const char *string, int len)
{
    int i;
    char *p;

    p = encoded;
    for (i = 0; i < len - 2; i += 3) {
    *p++ = basis_64[(string[i] >> 2) & 0x3F];
    *p++ = basis_64[((string[i] & 0x3) << 4) |
                    ((int) (string[i + 1] & 0xF0) >> 4)];
    *p++ = basis_64[((string[i + 1] & 0xF) << 2) |
                    ((int) (string[i + 2] & 0xC0) >> 6)];
    *p++ = basis_64[string[i + 2] & 0x3F];
    }
    if (i < len) {
    *p++ = basis_64[(string[i] >> 2) & 0x3F];
    if (i == (len - 1)) {
        *p++ = basis_64[((string[i] & 0x3) << 4)];
        *p++ = '=';
    }
    else {
        *p++ = basis_64[((string[i] & 0x3) << 4) |
                        ((int) (string[i + 1] & 0xF0) >> 4)];
        *p++ = basis_64[((string[i + 1] & 0xF) << 2)];
    }
    *p++ = '=';
    }

    *p++ = '\0';
    return p - encoded;
}