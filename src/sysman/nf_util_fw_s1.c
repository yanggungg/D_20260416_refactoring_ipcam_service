#include "nf_common.h"

#include "nf_util_fw.h"
#include "nf_util_fw_s1.h"
#include "nf_util_parse_s1.h"
#include "nf_util_http.h"
#include "nf_api_ipcam.h"

/*
	Static Function Definition
*/
static void str_trim(char *str, int size);
static NF_HTTP_CLIENT_REQ* _s1_new_http_client_req(gchar *str);
static int _s1_download_http_client_file(char *svr_path, char *loc_path, GError **error);
static int _s1_get_package_list();
static gboolean _s1_get_profile_url(char *buf, int buf_size, char *model);
static int _s1_get_package_profile(void);
static TreeNode* _s1_get_version_tree(TreeNode *rootTree, char *ver);
static int _s1_compare_fw_version(char *expect_high, char *expect_low);
static void _s1_get_fw_info(TreeNode *verTree, NF_FW_NETWORK_S1_INFO *fw_info);
static int _s1_find_new_version(char *model, char *ver, NF_FW_NETWORK_S1_INFO *fw_info);


/*
	Gloval Variable Definition
*/
static gboolean running_flag = FALSE;

static void str_trim(char *str, int size)
{
	int i = 0;
	int j = 0;

	for(i=0; i < size; i++)
	{
		if( (str[i] != ' ') && (str[i] != '\r') && (str[i] != '\n') )
		{
			str[j] = str[i];
			j++;
		}
	}
}
 
static NF_HTTP_CLIENT_REQ* _s1_new_http_client_req(gchar *str)
{
	NF_HTTP_CLIENT_REQ *req;

	g_assert(str);

	req = g_malloc0( sizeof(NF_HTTP_CLIENT_REQ) );

	snprintf( req->url, sizeof(req->url)-1, str);

	req->is_debug = 0;
	req->timeout_connect_sec = 7;
	req->timeout_rx_sec = req->timeout_tx_sec = 5;

	req->is_cusx = TRUE;
	strncpy(req->mac, nf_sysdb_get_str_nocopy("sys.info.mac"), sizeof(req->mac));
	strncpy(req->model, nf_sysdb_get_str_nocopy("sys.info.model"), sizeof(req->model));
	str_trim(req->model, sizeof(req->model));	

	return req;
}

static int _s1_download_http_client_file(char *svr_path, char *loc_path, GError **error)
{
	gboolean ret;
	NF_HTTP_CLIENT_REQ *req;

	g_assert(svr_path);
	g_assert(loc_path);

	req = _s1_new_http_client_req(svr_path);

	ret = nf_http_client_get_file( req, loc_path, error);

	g_free(req);

	g_message("NET_FW_S1 : %s - path:%s - ret:%d", __FUNCTION__, svr_path, ret);

	return ret;
}

static int _s1_get_package_list()
{
	GError *error = NULL;
	int ret = 0;
	char path[1024];
	char tmp[256];
	
// TEST CODE
	strncpy(tmp, nf_sysdb_get_str_nocopy("net.email.smtpsvr"), sizeof(tmp));
	if(!strcmp(tmp, "itx_net_fw_test"))
		strncpy(path, S1_FW_PACKAGE_LIST_PATH_TEST, sizeof(path));
	else
		strncpy(path, S1_FW_PACKAGE_LIST_PATH, sizeof(path));
	
	ret = _s1_download_http_client_file(path, S1_FW_PACKAGE_LIST_NAME, &error);

	if(!ret)
	{
		if(error)
			g_error_free( error);
	}

	g_message("NET_FW_S1 : %s - ret:%d", __FUNCTION__, ret);
		
	return ret;
}

static gboolean _s1_get_profile_url(char *buf, int buf_size, char *model)
{
	FILE *fp = NULL;
	gboolean ret = FALSE;

	g_assert(buf);
	g_assert(model);

	if( buf_size < 4 )
		return FALSE;
	
	fp = fopen( S1_FW_PACKAGE_LIST_NAME, "r");

	if( !fp )
		return FALSE;

	if( fgets( buf, 4, fp))
	{	
		if( ((buf[0] & 0xff) == 0xef && (buf[1] & 0xff) == 0xbb && (buf[2] & 0xff) == 0xbf) == 0 )
		{
			fclose(fp);
			return FALSE;
		}
	}
	else
	{
		fclose(fp);
		return FALSE;
	}	

	while( fgets( buf , buf_size-1, fp ) )
	{
		str_trim(buf, buf_size);
		
		if(!strcmp( buf, model)){
			if( fgets( buf , buf_size-1, fp ) ){
				str_trim(buf, buf_size);
				ret = TRUE;
				break;
			}
			else{
				break;
			}
		}
	}

	fclose(fp);
	return ret;
}

static int _s1_get_package_profile(void)
{
	GError *error = NULL;
	int ret = 0;
	char buf[1024];


	int ch;
	TreeNode* syntaxTree;

	NFIPCamModelInfo model_info;

	char *nvr;
	char model[128];

	S1_init_RootTrees();

	memset(model, 0x0 , sizeof(model));

	nvr = nf_sysdb_get_str_nocopy( "sys.info.model");
	if( strlen(nvr) > 0 )
	{
		int i;
		int size = sizeof(model);
		
		strncpy(model, nvr, sizeof(model));
		str_trim(model, sizeof(model));

		if( _s1_get_profile_url( buf, sizeof(buf), model) )
		{		
			if(_s1_download_http_client_file(buf, S1_FW_PROFILE_NAME, &error))
			{
				syntaxTree = new_TreeNodeFromFile(S1_FW_PROFILE_NAME);
				if( syntaxTree )
				{
					if(S1_AddRootTree(syntaxTree))
						ret++;
					else
						free_TreeNode(syntaxTree);
				}
			}
			else
			{
				if(error)				
					g_error_free(error);
			}
		}
		
	}	

	for(ch = 0; ch < NUM_ACTIVE_CH ; ch++)
	{
		memset( &model_info, 0x0, sizeof(NFIPCamModelInfo) );
		memset(model, 0x0 , sizeof(model));
		
		nf_ipcam_get_model_info(ch, &model_info, NULL);

		if( strlen(model_info.name) > 0 )
		{
			strncpy(model, model_info.name, sizeof(model));		
			str_trim(model, sizeof(model));

			if(S1_GetRootTreeByModel(model))
				continue;

			if( _s1_get_profile_url( buf, sizeof(buf), model) )
			{	
				if(_s1_download_http_client_file(buf, S1_FW_PROFILE_NAME, &error))
				{
					syntaxTree = new_TreeNodeFromFile(S1_FW_PROFILE_NAME);
					if( syntaxTree )
					{
						if(S1_AddRootTree(syntaxTree))
							ret++;
						else
							free_TreeNode(syntaxTree);
					}
				}
				else
				{
					if(error)				
						g_error_free(error);
				}
			}			
		}
	}
		
	return ret;
}

static TreeNode* _s1_get_version_tree(TreeNode *rootTree, char *ver)
{
	int i = 0;
	TreeNode *subTree;
	char *new_ver = NULL;

	g_assert(rootTree);
	g_assert(ver);
	
	do
	{
		subTree = S1_GetSubTree(rootTree, "Version", i++);
		if(subTree)
		{
			new_ver = S1_GetItem(subTree, "ID", 0);
			if(new_ver)
			{
				if(!strcmp(ver, new_ver))
					break;
			}
		}
	} while(subTree);

	return subTree;
}

static int _s1_compare_fw_version(char *expect_high, char *expect_low)
{
	int left[3];	
	int right[3];
	int i = 0;
	
	char tmp[20];
	char *temp;
	char *temp_str1;
	char *temp_str2;
	int check_count = 0;
	
	temp = strstr(expect_high, "V");
	if(temp)
	{
		left[0] = strtol(temp+1, &temp_str1, 10);
		i = 1;

		while((temp_str1[0] == '.') && (i < 3))
		{
			left[i] = strtol(temp_str1+1, &temp_str2, 10);
			temp_str1 = temp_str2;
			i++;
		}

		if( i != 3 )
			return -1;
	}
	else
		return -1;

	temp = strstr(expect_low, "V");	
	if(temp)
	{
		right[0] = strtol(temp+1, &temp_str1, 10);
		i = 1;

		while((temp_str1[0] == '.') && (i < 3))
		{
			right[i] = strtol(temp_str1+1, &temp_str2, 10);
			temp_str1 = temp_str2;
			i++;
		}

		if( i != 3 )
			return -1;
	}
	else
		return -1;

	if( left[0] > right[0] )
		return 1;
	else if( left[0] < right[0] )
		return 0;
	else
	{
		if( left[1] > right[1] )
			return 1;
		else if( left[1] < right[1] )
			return 0;		
		else
		{
			if( left[2] > right[2] )
				return 1;
			else if( left[2] < right[2] )
				return 0;			
			else
				return 1;
		}
	}
}

static void _s1_get_fw_info(TreeNode *verTree, NF_FW_NETWORK_S1_INFO *fw_info)
{
	TreeNode *fileTree;
	TreeNode *relTree;
	char *item;
	
	g_assert(verTree);
	g_assert(fw_info);
	
	fileTree = S1_GetSubTree(verTree, "File", 0);			
	if(fileTree)
	{
		item = S1_GetItem(fileTree, "URL", 0);
		if(item)
			strncpy(fw_info->url, item, sizeof(fw_info->url));
	}

	item = S1_GetItem(verTree, "Level", 0);
	if(item)
		strncpy(fw_info->level, item, sizeof(fw_info->level));

	relTree = S1_GetSubTree(verTree, "ReleaseNote", 0);
	if(relTree)
	{
		item = S1_GetItem(relTree, "General", 0);
		if(item)
			strncpy(fw_info->general, item, sizeof(fw_info->general));

		item = S1_GetItem(relTree, "Fixes", 0);
		if(item)
			strncpy(fw_info->fixes, item, sizeof(fw_info->fixes));

		item = S1_GetItem(relTree, "Updates", 0);
		if(item)
			strncpy(fw_info->updates, item, sizeof(fw_info->updates));

		item = S1_GetItem(relTree, "Link", 0);
		if(item)
			strncpy(fw_info->link, item, sizeof(fw_info->link));
	}
}

static int _s1_find_new_version(char *model, char *ver, NF_FW_NETWORK_S1_INFO *fw_info)
{
	char *new_ver = NULL;
	char *cur_ver = NULL;
	char *req_ver = NULL;
	char *model_id = NULL;
	TreeNode *rootTree;
	TreeNode *subTree;
	TreeNode *verTree;
	int ret = FALSE;
	int comp = 0;

	g_assert(model);
	g_assert(ver);
	g_assert(fw_info);	

	rootTree = S1_GetRootTreeByModel(model);
	if(rootTree)
	{
		subTree = S1_GetSubTree(rootTree, "Product", 0);
		model_id = S1_GetItem(subTree, "ID", 0);
		strncpy(fw_info->model, model_id, sizeof(fw_info->model));		
		
		subTree = S1_GetSubTree(rootTree, "Version", 0);
		if(subTree)
		{
			new_ver = S1_GetItem(subTree, "ID", 0);
			if(new_ver)
			{
				comp = _s1_compare_fw_version(ver, new_ver);
				if(comp == 1)
				{
					_s1_get_fw_info(subTree, fw_info);
					strncpy(fw_info->new_ver, new_ver, sizeof(fw_info->new_ver));
					ret = TRUE;
				}
				else if(comp == 0)
				{
					req_ver = new_ver;					
					
					while(1)
					{						
						cur_ver = req_ver;

						verTree = _s1_get_version_tree(rootTree, cur_ver);
						if(verTree)
						{
							req_ver = S1_GetItem(verTree, "PrerequisiteVersionID", 0);
							if(req_ver){
								comp = _s1_compare_fw_version(ver, req_ver);
								if(comp == 1)
								{
									_s1_get_fw_info(verTree, fw_info);								
									fw_info->need = 1;
									strncpy(fw_info->new_ver, cur_ver, sizeof(fw_info->new_ver));
									
									ret = TRUE;
									break;
								}
								else if(comp == -1)
									break;
							}
							else
							{
								_s1_get_fw_info(verTree, fw_info);								
								fw_info->need = 1;
								strncpy(fw_info->new_ver, cur_ver, sizeof(fw_info->new_ver));
								ret = TRUE;
								break;
							}
						}
						else
							break;
					}
				}
			}
		}
	}

	return ret;
}

gboolean nf_fw_network_s1_get_update_state()
{
	return	running_flag;
}

int nf_fw_network_s1_set_update_state(gboolean state)
{
	g_message("NET_FW_S1 : %s - state :%d", __FUNCTION__, state);
	
	running_flag = state;

	return 0;
}

int nf_fw_network_s1_update_profile(void)
{
	if( _s1_get_package_list() )
	{
		int n;
		
		n = _s1_get_package_profile();
		if(n)
		{
			int i;
			NF_FW_NETWORK_S1_INFO fw_info;
			
			for(i=0; i < NUM_ACTIVE_CH; i++)
			{
				if (nf_fw_network_s1_get_cam_fw_info(i, &fw_info) )
				{
					if( fw_info.need )
						return 1;
				}
			}

			if ( nf_fw_network_s1_get_nvr_fw_info( &fw_info ) )
			{
				if( fw_info.need )
					return 1;
			}
		}
	}
	else
	{
		return -1;
	}

	return 0;
}
 
int nf_fw_network_s1_get_cam_fw_info(int ch, NF_FW_NETWORK_S1_INFO *fw_info)
{
	int ret = 0;
	NFIPCamModelInfo model_info;
	char model_str[128];
	
	g_assert(fw_info);
	
	memset( fw_info, 0x0, sizeof(NF_FW_NETWORK_S1_INFO));	
	memset( &model_info, 0x0, sizeof(NFIPCamModelInfo));

	nf_ipcam_get_model_info(ch, &model_info, NULL);

	strncpy(model_str, model_info.name, sizeof(model_str));
	str_trim(model_str, sizeof(model_str));
	
	ret = _s1_find_new_version(model_str, model_info.swver, fw_info);

#if 0
	g_message("FW_INFO :%s ", __FUNCTION__);
	g_message("FW_INFO : need : %d", fw_info->need );
	g_message("FW_INFO : model : %s", fw_info->model);
	g_message("FW_INFO : new_ver : %s", fw_info->new_ver);
	g_message("FW_INFO : url : %s", fw_info->url);
	g_message("FW_INFO : level : %s", fw_info->level);
	g_message("FW_INFO : general : %s", fw_info->general);
	g_message("FW_INFO : fixes : %s", fw_info->fixes);
	g_message("FW_INFO : updates : %s", fw_info->updates);
	g_message("FW_INFO : link : %s", fw_info->link);
#endif

	return ret;
}

int nf_fw_network_s1_get_nvr_fw_info(NF_FW_NETWORK_S1_INFO *fw_info)
{	
	int ret = 0;
	char *model;
	char model_str[128];
	char ver_str[128];

	gchar  *contents = NULL;
	gsize  length = 0;
	GError *error = NULL;
	
	g_assert(fw_info);
	
	memset( fw_info, 0x0, sizeof(NF_FW_NETWORK_S1_INFO));
	memset( model_str, 0x0, sizeof(model_str));
	memset( ver_str, 0x0, sizeof(ver_str));

	model = nf_sysdb_get_str_nocopy( "sys.info.model");

	if(!model)
		return FALSE;

	if( strlen(model) > 0 ) 
	{
		strncpy(model_str, model, sizeof(model_str));
		str_trim(model_str, sizeof(model_str));
	}
	else
		return FALSE;

	if (!g_file_get_contents (S1_FW_FAKE_VER_FILE, &contents, &length, &error))
	{
		g_warning("%s\n", error->message);
		g_error_free (error);

		if(contents)
			g_free(contents);
		
		return FALSE;
	}		
	else
	{
		if(contents)
		{
			char *tmp_str = NULL;
			int i = 0;

			tmp_str = strstr(contents, "<fake_fwver>");
			if(tmp_str)
			{
				int size;
				size = sizeof(ver_str);
				tmp_str = tmp_str + 12;

				while( (*tmp_str != '<') && (i < size) )
				{
					ver_str[i] = *tmp_str;
					tmp_str = tmp_str + 1;
					i++;
				}

				ver_str[i] = 0;

				str_trim(ver_str, sizeof(ver_str));
			}
			else
			{
				g_free(contents);
				return FALSE;
			}
				
			g_free(contents);
		}
		else
		{
			return FALSE;
		}
	}

	ret = _s1_find_new_version(model_str, ver_str, fw_info);

#if 0
	g_message("FW_INFO :%s ", __FUNCTION__);
	g_message("FW_INFO : need : %d", fw_info->need );
	g_message("FW_INFO : model : %s", fw_info->model);
	g_message("FW_INFO : new_ver : %s", fw_info->new_ver);
	g_message("FW_INFO : url : %s", fw_info->url);
	g_message("FW_INFO : level : %s", fw_info->level);
	g_message("FW_INFO : general : %s", fw_info->general);
	g_message("FW_INFO : fixes : %s", fw_info->fixes);
	g_message("FW_INFO : updates : %s", fw_info->updates);
	g_message("FW_INFO : link : %s", fw_info->link);
#endif

	return ret;
}

#define SQUASHFS_NBN_MOUNT_PATH "/mnt/squashfs_nbn"
#define MOUNTED_NBS_FILE "/mnt/squashfs_nbn/38100.nbn"

gboolean nf_fw_get_version_info(gchar *fw_path, gchar *version, gint fw_type, gint ret_type)
{
	FILE *fp=NULL;
	NF_FW_IMAGE_LIST file_imglist;
	gchar fw_filename[1024] = {0,};

	gchar *token = NULL;
	gchar ver_parse[4][16] = {0,}, parse_tmp[64] = {0,}, i=0;

	char nbs_mount_path[32] = SQUASHFS_NBN_MOUNT_PATH;	
	char mounted_nbs_file[32] = MOUNTED_NBS_FILE;
	char cmd_buff[255]={0,};
	char *rbuf;

	g_return_val_if_fail(fw_path != NULL, 0);

	if(fw_type == NF_FW_FILE_TYPE_NBS)
	{
		mkdir( nbs_mount_path, 0755 );
		snprintf(cmd_buff, sizeof(cmd_buff)-1, "mount -o loop \'%s\' %s", fw_path, nbs_mount_path );
		proxy_system(cmd_buff, 1, 5);
		g_message("%s mounted [%s]->[%s]",__FUNCTION__, fw_path, nbs_mount_path);
		strncpy(fw_filename, mounted_nbs_file, sizeof(mounted_nbs_file));
	}

	else if(fw_type == NF_FW_FILE_TYPE_NBE)
	{
		g_warning("%s not yet implement\n", __FUNCTION__);
		return FALSE;
	}
	else
	{
		strcpy(fw_filename, fw_path);
	}

	if((fp=fopen(fw_filename, "r")) == NULL)
	{
		g_warning("%s File Open Error!!! Image Name [%s]", __FUNCTION__, fw_filename);
		goto get_ver_fail;
	}

	memset(&file_imglist, 0x0, FW_UPGRADE_LIST_SIZE);
	if(fread(&file_imglist, FW_UPGRADE_LIST_SIZE, 1, fp) != 1)
	{
		g_warning("%s fread() error!!! ", __FUNCTION__);
		fclose(fp);
		goto get_ver_fail;
	}
	if(strncmp(file_imglist.fwheader.magic, FW_UPGRADE_NF_MAGIC_S, sizeof(file_imglist.fwheader.magic)) !=0)
	{
		g_warning("%s Not Match Start Magic. Magic Name [%s]", __FUNCTION__, file_imglist.fwheader.magic);
		fclose(fp);
		goto get_ver_fail;
	}

	strcpy(parse_tmp, file_imglist.fwheader.version);
	memset(version, 0x00, 64);

	token = strtok_r(parse_tmp, ".", &rbuf);
	strcpy(ver_parse[0], token);

	i++;
	while(token = strtok_r(NULL, ".", &rbuf))
	{
		strcpy(ver_parse[i], token);
		i++;
	}

	if(ret_type == NF_FW_VER_DIV_MODEL)
		strcpy(version, ver_parse[0]);
	else if(ret_type == NF_FW_VER_DIV_PROTO)
		strcpy(version, ver_parse[1]);
	else if(ret_type == NF_FW_VER_DIV_MINOR)
		strcpy(version, ver_parse[2]);
	else if(ret_type == NF_FW_VER_DIV_VENDOR)
		strcpy(version, ver_parse[3]);
	else
		strcpy(version, file_imglist.fwheader.version);

	fclose(fp);
	if(fw_type == NF_FW_FILE_TYPE_NBS)
	{
		snprintf(cmd_buff, sizeof(cmd_buff)-1, "umount %s", nbs_mount_path );
		proxy_system(cmd_buff, 1, 5);
		g_message("%s unmounted [%s]",__FUNCTION__, nbs_mount_path);
	}
	return TRUE;

get_ver_fail:
	if(fw_type == NF_FW_FILE_TYPE_NBS)
	{
		snprintf(cmd_buff, sizeof(cmd_buff)-1, "umount %s", nbs_mount_path );
		proxy_system(cmd_buff, 1, 5);
		g_message("%s unmounted [%s]",__FUNCTION__, nbs_mount_path);
	}
	return FALSE;
}
