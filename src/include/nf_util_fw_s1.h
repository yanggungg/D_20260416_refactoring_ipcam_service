#ifndef __NF_UTIL_FW_S1_H__
#define __NF_UTIL_FW_S1_H__

#define S1_FW_PACKAGE_LIST_PATH_TEST	"http://192.168.100.14/~kjh81/packlist/pkglist.txt"
#define S1_FW_PACKAGE_LIST_PATH			"http://updateserver.s1.co.kr/pkglist.txt"
#define S1_FW_PACKAGE_LIST_NAME			"/tmp/packlist.info"
#define S1_FW_PROFILE_NAME				"/tmp/packprofile.info"
#define S1_FW_FAKE_VER_FILE				"/NFDVR/data/gui/uxconf.cfg"

typedef struct _NF_FW_NETWORK_S1_INFO{
	int need;
	char model[64];
	char new_ver[32];
	char url[1024];
	char level[64];
	char general[1024];
	char fixes[1024];
	char updates[1024];
	char link[1024];
} NF_FW_NETWORK_S1_INFO;

typedef enum _NF_FW_FILE_TYPE_E
{
	NF_FW_FILE_TYPE_NBN     = 0,
	NF_FW_FILE_TYPE_NBS     = 1,
	NF_FW_FILE_TYPE_NBE     = 2,
} NF_FW_FILE_TYPE_E;

typedef enum _NF_FW_VER_DIV_E
{
	NF_FW_VER_DIV_ALL       = 0,
	NF_FW_VER_DIV_MODEL     = 1,
	NF_FW_VER_DIV_PROTO     = 2,
	NF_FW_VER_DIV_MINOR     = 3,
	NF_FW_VER_DIV_VENDOR    = 4,
} NF_FW_VER_DIV_E;

gboolean nf_fw_network_s1_get_update_state();
int nf_fw_network_s1_set_update_state(gboolean state);
int nf_fw_network_s1_update_profile(void);
int nf_fw_network_s1_get_cam_fw_info(int ch, NF_FW_NETWORK_S1_INFO *fw_info);
int nf_fw_network_s1_get_nvr_fw_info(NF_FW_NETWORK_S1_INFO *fw_info);
gboolean nf_fw_get_version_info(gchar *fw_path, gchar *version, gint fw_type, gint ret_type);

#endif

