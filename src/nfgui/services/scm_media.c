/*
 * scm_media.c
 * 	- scm media service
 *	- dependencies :
 *			GThread
 *			MSR
 *		
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Jan 11, 2011
 *
 */

//#include "nfdal.h"
#include "ix_mem.h"
#include "iux_afx.h"
#include "ix_func.h"
#include "mda.h"
#include <string.h>
#include "nfdal.h"


#define DBG_LEVEL		9
#define DBG_MODULE		"SCM_MEDIA"


#define MAX_NAME_LEN	IUX_MAX_NAME_LEN
#define MAX_PATH_LEN	IUX_MAX_PATH_LEN


////////////////////////////////////////////////////////////
//
// private functions
//

static int _filter_fw_file(char *file_name, void *filter_param) 
{
	gint len;
	gchar cur_ver[512];
	gint i;
	int ret = 0;

	gint uicode1, uicode2;
	gint cnt1, cnt2;
	FNAME_FILTER_PARAM_T *fparam = (FNAME_FILTER_PARAM_T*)filter_param;

	len = strlen(file_name);


	// Using cheat code..
	if (fparam->condition == FF_NOFILTER) {
		if(len < 5) return 0;

		if(!g_strncasecmp(&(file_name[len-4]), fparam->ext, 4)) 	return 1;
		else return 0;
	}

	// Minimun String length include '.nbn'
	if(len < 15) return 0;		// minimum str :: XXXXX.X.X.X.nbn

	// 1st filter, file extention
	if (fparam->condition & FF_FNAME_EXT) { 
		// Check File format.
		if (g_strncasecmp(&(file_name[len-4]), fparam->ext, 4)) return 0;
	}

	// check count of dot.
	i = 0;
	cnt1 = 0;
	while(i<len) {
		if(file_name[i] == '.') {
			cnt1++;
			if(cnt1 == 3) cnt2 = i;
		}
		else {
			if((cnt1<3) && (file_name[i]<'0' || file_name[i]>'9'))
				return 0;
		}
		i++;
	}

	if(cnt1 < 4) return 0;

	memset(cur_ver, 0, sizeof(cur_ver));
	DAL_get_fw_version(cur_ver);


	// 2nd filter, model number
	if (fparam->condition & FF_MODEL) {
		// Compare Model..
		if (g_strncasecmp(cur_ver, file_name, 4)) return 0;
	}


	// Compare Buyer Code...
	// 1. Current Buyer code of system.
	i = 0;
	cnt1 = 0;
	while(i<512) {
		if(cur_ver[i] == '.') cnt1++;
		if(cnt1 == 3) break;
		i++;
	}

	if(cnt1 != 3) return 0;

	i++;
	uicode1 = 0;
	cnt1 = 0;
	
	while(1) {
		if(!cur_ver[i]) break;
		if(cur_ver[i]<'0' || cur_ver[i]>'9') break;
		uicode1 = uicode1*10 + (cur_ver[i] - '0');
		cnt1++;
		i++;
	}

	// 2. Buyer code of Firmware file
	i = cnt2+1;
	uicode2 = 0;
	cnt2 = 0;
	while(1) {
		if(!file_name[i]) break;
		if(file_name[i]<'0' || file_name[i]>'9') break;
		uicode2 = uicode2*10 + (file_name[i] - '0');
		cnt2++;
		i++;
	}
	
	// 3rd filter, buyer code
	if (fparam->condition & FF_BUYERCODE) {
		if ((cnt1<1) || (cnt1!=cnt2) || (uicode1!=uicode2)) return 0;
	}

	return 1;
}

static int _filter_fw_file_s1(char *file_name, void *filter_param) 
{
	FNAME_FILTER_PARAM_T *fparam = (FNAME_FILTER_PARAM_T*)filter_param;
	SysInfoData sys_info;
	gchar cur_ver[512];
	gchar *pver1 = 0, *pver2 = 0;
	int len;

	DMSG(1, "");

	len = strlen(file_name);
	
	// Using cheat code..
	if (fparam->condition == FF_NOFILTER) {
		if(len < 5) return 0;

		DMSG(1, "[find fw name : %s]", file_name);
		if(!g_strncasecmp(&(file_name[len-4]), fparam->ext, 4)) 	return 1;
		else return 0;
	}

	// 1st filter, file extention
	if (fparam->condition & FF_FNAME_EXT) { 
		// Check File format.
		if (g_strncasecmp(&(file_name[len-4]), fparam->ext, 4)) return 0;
	}

	memset(&sys_info, 0, sizeof(SysInfoData));
	DAL_get_sysInfo_data(&sys_info);

	DMSG(1, "MODEL:%s", sys_info.model);

	// 2nd filter, model number
	if (fparam->condition & FF_MODEL) {
		// Compare Model..
		if (g_strncasecmp(sys_info.model, file_name, 8)) return 0;
	}

	if (fparam->condition & FF_UPPERVER) {
		memset(cur_ver, 0x00, sizeof(cur_ver));
		var_get_fake_fwver(cur_ver, 512);	
		if(cur_ver[0] != 'V') return 0;
		pver1 = cur_ver;
		
		pver2 = strstr(file_name, "_V");
		if (!pver2) return 0;

		DMSG(1, "[cur_ver:%s, find_ver:%s]", pver1+1, pver2+2);
		
		DMSG(1, "%d : %d", atoi(pver1+1), atoi(pver2+2));
		if (atoi(pver1+1) > atoi(pver2+2)) return 0;

		if (atoi(pver1+1) == atoi(pver2+2)) {
			pver1 = strstr(pver1+1, ".");
			if (!pver1) return 0;			
			pver2 = strstr(pver2+2, ".");
			if (!pver2) return 0;

			DMSG(1, "%d : %d", atoi(pver1+1), atoi(pver2+1));	
			if (atoi(pver1+1) > atoi(pver2+1)) return 0;

			if (atoi(pver1+1) == atoi(pver2+1)) {
				pver1 = strstr(pver1+1, ".");
				if (!pver1) return 0;			
				pver2 = strstr(pver2+1, ".");
				if (!pver2) return 0;

				DMSG(1, "%d : %d", atoi(pver1+1), atoi(pver2+1));	
				if (atoi(pver1+1) >= atoi(pver2+1)) return 0;
			}
		}
	}

	DMSG(1, "%s", file_name);
	return 1;
}

static int _filter_ipcam_fw_file(char *file_name, void *filter_param) 
{
	gint len;
	gchar cur_ver[512];
	gint i;
	int ret = 0;

	gint uicode1, uicode2;
	gint cnt1, cnt2;
	FNAME_FILTER_PARAM_T *fparam = (FNAME_FILTER_PARAM_T*)filter_param;

	len = strlen(file_name);

	// 1st filter, file extention
	if (fparam->condition & FF_FNAME_EXT) { 
		// Check File format.
		if (g_strncasecmp(&(file_name[len-4]), fparam->ext, 4)) return 0;
	}

	return 1;
}

static int _filter_dir_list(char *file_name, void *filter_param) 
{
	FNAME_FILTER_PARAM_T *fparam = (FNAME_FILTER_PARAM_T*)filter_param;

	return 1;
}

static int _filter_file_list(char *file_name, void *filter_param) 
{
	FNAME_FILTER_PARAM_T *fparam = (FNAME_FILTER_PARAM_T*)filter_param;
	gint len;

	len = strlen(file_name);

	// 1st filter, file extention
	if (fparam->condition & FF_FNAME_EXT) { 
		// Check File format.
		if (g_strncasecmp(&(file_name[len-4]), fparam->ext, 4)) return 0;
	}

	return 1;
}

static char **_new_fw_list(MEDIA_ID id, FWFILE_FILTER_E filter, FWFILE_OPT_E opt, int *ret_cnt)
{
	char **flist = 0;
	char mnt_path[MAX_PATH_LEN + 1];
	FNAME_FILTER_PARAM_T filter_param;
	FILTER_PROC proc;
	int vendor;

	memset(&filter_param, 0x00, sizeof(FNAME_FILTER_PARAM_T));

	if (ret_cnt) *ret_cnt = 0;
	if (!mda_is_valid_media_id(id)) return 0; 
	if (mda_get_mounted_path(id, mnt_path, MAX_PATH_LEN) == -1) return 0;

	strcpy(filter_param.ext, ".nbn");
	filter_param.condition = filter;
	filter_param.opt = opt;
	if (filter == FF_NOFILTER) proc = NULL;
	else {
		vendor = var_get_vendor_code();
		DMSG(1, "VENDOR = %d", vendor);
		switch (vendor) {
		case 30:
			 proc = _filter_fw_file_s1;
			 break;
		default:
			 proc = _filter_fw_file;
			 break;
		}
	}

	flist = ifn_new_filelist(mnt_path, proc, &filter_param, ret_cnt);

	return flist;
}

static char **_new_ipcam_fw_list(MEDIA_ID id, FWFILE_FILTER_E filter, FWFILE_OPT_E opt, int *ret_cnt)
{
	char **flist = 0;
	char mnt_path[MAX_PATH_LEN + 1];
	FNAME_FILTER_PARAM_T filter_param;
	FILTER_PROC proc;
	int vendor;

	memset(&filter_param, 0x00, sizeof(FNAME_FILTER_PARAM_T));

	if (ret_cnt) *ret_cnt = 0;
	if (!mda_is_valid_media_id(id)) return 0; 
	if (mda_get_mounted_path(id, mnt_path, MAX_PATH_LEN) == -1) return 0;

	strcpy(filter_param.ext, ".bin");
	filter_param.condition = filter;
	filter_param.opt = opt;
	if (filter == FF_NOFILTER) proc = NULL;
	else proc = _filter_ipcam_fw_file;
	
	flist = ifn_new_filelist(mnt_path, proc, &filter_param, ret_cnt);
	return flist;
}

static char **_new_ipcam_fw_list_techwin(MEDIA_ID id, FWFILE_FILTER_E filter, FWFILE_OPT_E opt, int *ret_cnt)
{
	char **flist = 0;
	char mnt_path[MAX_PATH_LEN + 1];
	FNAME_FILTER_PARAM_T filter_param;
	FILTER_PROC proc;
	int vendor;

	memset(&filter_param, 0x00, sizeof(FNAME_FILTER_PARAM_T));

	if (ret_cnt) *ret_cnt = 0;
	if (!mda_is_valid_media_id(id)) return 0; 
	if (mda_get_mounted_path(id, mnt_path, MAX_PATH_LEN) == -1) return 0;

	strcpy(filter_param.ext, ".img");
	filter_param.condition = filter;
	filter_param.opt = opt;
	if (filter == FF_NOFILTER) proc = NULL;
	else proc = _filter_ipcam_fw_file;
	
	flist = ifn_new_filelist(mnt_path, proc, &filter_param, ret_cnt);
	return flist;
}

static char **_new_ipcam_fw_list_idis(MEDIA_ID id, FWFILE_FILTER_E filter, FWFILE_OPT_E opt, int *ret_cnt)
{
	char **flist = 0;
	char mnt_path[MAX_PATH_LEN + 1];
	FNAME_FILTER_PARAM_T filter_param;
	FILTER_PROC proc;
	int vendor;

	memset(&filter_param, 0x00, sizeof(FNAME_FILTER_PARAM_T));

	if (ret_cnt) *ret_cnt = 0;
	if (!mda_is_valid_media_id(id)) return 0; 
	if (mda_get_mounted_path(id, mnt_path, MAX_PATH_LEN) == -1) return 0;

	strcpy(filter_param.ext, ".rui");
	filter_param.condition = filter;
	filter_param.opt = opt;
	if (filter == FF_NOFILTER) proc = NULL;
	else proc = _filter_ipcam_fw_file;
	
	flist = ifn_new_filelist(mnt_path, proc, &filter_param, ret_cnt);
	return flist;
}

static char **_new_face_list(MEDIA_ID id, char *file_ext, int *ret_cnt)
{
	char **flist = 0;
	char mnt_path[MAX_PATH_LEN + 1];
	FNAME_FILTER_PARAM_T filter_param;
	FILTER_PROC proc;
	int vendor;

	memset(&filter_param, 0x00, sizeof(FNAME_FILTER_PARAM_T));

	if (ret_cnt) *ret_cnt = 0;
	if (!mda_is_valid_media_id(id)) return 0; 
	if (mda_get_mounted_path(id, mnt_path, MAX_PATH_LEN) == -1) return 0;

	strcpy(filter_param.ext, file_ext);
	filter_param.condition = FF_FNAME_EXT;
	filter_param.opt = 0;

	flist = ifn_new_filelist(mnt_path, _filter_file_list, &filter_param, ret_cnt);
	return flist;
}

static char **_new_dir_list(MEDIA_ID id, int *ret_cnt)
{
	char **flist = 0;
	char mnt_path[MAX_PATH_LEN + 1];
	FNAME_FILTER_PARAM_T filter_param;
	FILTER_PROC proc;
	int vendor;

	memset(&filter_param, 0x00, sizeof(FNAME_FILTER_PARAM_T));

	if (ret_cnt) *ret_cnt = 0;
	if (!mda_is_valid_media_id(id)) return 0; 
	if (mda_get_mounted_path(id, mnt_path, MAX_PATH_LEN) == -1) return 0;

	filter_param.opt = 0;

	flist = ifn_new_dirlist(mnt_path, _filter_dir_list, &filter_param, ret_cnt);
	return flist;
}

static char **_new_file_list(MEDIA_ID id, char *file_ext, int *ret_cnt)
{
	char **flist = 0;
	char mnt_path[MAX_PATH_LEN + 1];
	FNAME_FILTER_PARAM_T filter_param;
	FILTER_PROC proc;
	int vendor;

	memset(&filter_param, 0x00, sizeof(FNAME_FILTER_PARAM_T));

	if (ret_cnt) *ret_cnt = 0;
	if (!mda_is_valid_media_id(id)) return 0; 
	if (mda_get_mounted_path(id, mnt_path, MAX_PATH_LEN) == -1) return 0;

	strcpy(filter_param.ext, file_ext);
	filter_param.condition = FF_FNAME_EXT;
	filter_param.opt = 0;

	flist = ifn_new_filelist(mnt_path, _filter_file_list, &filter_param, ret_cnt);
	return flist;
}

static int _free_fw_list(char **fw_list)
{
	return ifn_free_filelist(fw_list);
}

static int _get_fw_count(MEDIA_ID id, FWFILE_FILTER_E filter)
{
	int cnt;
	char **flist = _new_fw_list(id, filter, FO_DEFAULT, &cnt);
	_free_fw_list(flist);
	return cnt;
}

static int _free_dir_list(char **dir_list)
{
	return ifn_free_dirlist(dir_list);
}

static int _free_file_list(char **file_list)
{
	return ifn_free_filelist(file_list);
}


////////////////////////////////////////////////////////////
//
// public interfaces
//

MEDIA_INFO_T *scm_new_media_list(int *ret_cnt)
{
	return mda_new_media_list(ret_cnt);
}

int scm_free_media_list(MEDIA_INFO_T *minfo)
{
	return mda_free_media_list(minfo);
}

int scm_get_media_count()
{
	return mda_get_media_count();
}

int scm_is_mounted_media(MEDIA_ID id)
{
	return mda_is_mounted_media(id);
}

int scm_get_mounted_path(MEDIA_ID id, char *path, int path_len)
{
	return mda_get_mounted_path(id, path, path_len);
}

MEDIA_TYPE_E scm_get_media_type(MEDIA_ID id)
{
	return mda_get_media_type(id);
}

int scm_get_device_id(MEDIA_ID id)
{
	return mda_get_device_id(id);
}

int scm_mount_media(MEDIA_ID id, char *dir)
{
	return mda_mount_media(id, dir);
}

int scm_umount_media(MEDIA_ID id)
{
	return mda_umount_media(id);
}

char **scm_new_fw_list(MEDIA_ID id, FWFILE_FILTER_E filter, FWFILE_OPT_E opt, int *ret_cnt)
{
	return _new_fw_list(id, filter, opt, ret_cnt);
}

int scm_free_fw_list(char **fw_list)
{
	return _free_fw_list(fw_list);
}

char **scm_new_ipcam_fw_list(MEDIA_ID id, FWFILE_FILTER_E filter, FWFILE_OPT_E opt, int *ret_cnt)
{
	return _new_ipcam_fw_list(id, filter, opt, ret_cnt);
}

char **scm_new_ipcam_fw_list_techwin(MEDIA_ID id, FWFILE_FILTER_E filter, FWFILE_OPT_E opt, int *ret_cnt)
{
	return _new_ipcam_fw_list_techwin(id, filter, opt, ret_cnt);
}

char **scm_new_ipcam_fw_list_idis(MEDIA_ID id, FWFILE_FILTER_E filter, FWFILE_OPT_E opt, int *ret_cnt)
{
	return _new_ipcam_fw_list_idis(id, filter, opt, ret_cnt);
}

int scm_free_ipcam_fw_list(char **fw_list)
{
	return _free_fw_list(fw_list);
}

char **scm_new_dir_list(MEDIA_ID id, int *ret_cnt)
{
	return _new_dir_list(id, ret_cnt);
}

int scm_free_dir_list(char **dir_list)
{
	return _free_dir_list(dir_list);
}

char **scm_new_file_list(MEDIA_ID id, char *file_ext, int *ret_cnt)
{
	return _new_file_list(id, file_ext, ret_cnt);
}

int scm_free_file_list(char **file_list)
{
	return _free_file_list(file_list);
}

int scm_get_fw_count(MEDIA_ID id, FWFILE_FILTER_E filter)
{
	return _get_fw_count(id, filter);
}

int scm_spy_odd_name(char *odd_name)
{
#if defined(_HDI_MODEL_UX) || defined(_DVR_MODEL_UX)
	int ret = nf_arch_scanODD(odd_name);
	return (ret > 0) ? 0 : -1;
#else
	return -1;
#endif
}
