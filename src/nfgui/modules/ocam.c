#include "ocam.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


////////////////////////////////////////////////////////////
//
// private data type
//

#define MAX_OC	1024

enum {
    OCAM_SHOW = 0,
    OCAM_HIDE
};

typedef struct _CAMERA_LIST_T {
	CAMERA_INFO_T cam_info;
	int assign_cnt;
	int hidden;
	int filtered;
} CAMERA_LIST_T;

////////////////////////////////////////////////////////////
//
// private variables 
//

CAMERA_LIST_T ioc[MAX_OC];
static int	sortidx[MAX_OC];
static unsigned long int g_fr = 0, g_to = 0;

////////////////////////////////////////////////////////////
//
// private functions
//

static int _find_empty_slot()
{
	int i = 0;
	for (i = 0; i < MAX_OC; ++i) {
		if (strlen(ioc[i].cam_info.url) == 0) return i;
	}
	return -1;
}

static int _is_empty_slot(int idx)
{
	return (strlen(ioc[idx].cam_info.url) == 0);
}

static int _find_camera(char *url)
{
	int i;
	char *p = url;

/*
	p = strstr(url, "rtsp://");
	if (p == NULL) {
		p = url;
	}
*/

	for (i = 0; i < MAX_OC; ++i) {
		if (strcmp(ioc[i].cam_info.url, p) == 0) return i;
	}
	return -1;
}

static int _clear_camera(int idx)
{
	memset(&ioc[idx], 0x00, sizeof(CAMERA_LIST_T));
	return 0;
}

static int _copy_caminfo(CAMERA_INFO_T *dst, CAMERA_INFO_T *src)
{
	iassert(dst);
	iassert(src);
	memcpy(dst, src, sizeof(CAMERA_INFO_T));
	return 0;
}

static int make_ipaddr(char *value, char *from, char *to)
{
	char tmp_from[64];
	char tmp_to[64];
	char *p = strrchr(value, '-');
	strncpy(tmp_from, value, p - value);
	ifn_trim_char(tmp_from, '.');	// anti exception
	if (ifn_count_char(tmp_from, '.') != 3) return -1;
	strcpy(from, tmp_from);	

	strcpy(tmp_to, p + 1);
	ifn_trim_char(tmp_to, '.');		// anti exception
	char *q = tmp_to;

	int cnt = 3 - ifn_count_char(q, '.');
	char *e = ifn_strnchar(tmp_from, '.', cnt);
	strncpy(to, tmp_from, e - tmp_from);
	strcat(to, ".");
	strcat(to, q);
	ifn_trim_char(to, '.');		// anti exception
	if (ifn_count_char(to, '.') != 3) {	// anti exception
		strcpy(to, from);
	}

	return 0;
}

static int _filter_hostaddr_range(int idx, char *op, unsigned long int fr, unsigned long int to)
{
	unsigned long int ucur, cur;
	ucur = inet_addr(ioc[idx].cam_info.url);
	cur = ntohl(ucur);

	if (strcmp(op, "~") == 0) {			// ip range to show
		if (cur < fr || to < cur) return 1;
	}
	else if (strcmp(op, "!") == 0) {	// ip range not to show
		if (fr <= cur && cur <= to) return 1;
	}

	return 0;
}

static int _filter_hostaddr_cmp(int idx, char *op, char *value)
{
	if (strcmp(op, "~") == 0) {			// to show
		if (strstr(ioc[idx].cam_info.url, value) == NULL) return 1;	// reverse filter
	}
	else if (strcmp(op, "!") == 0) {	// not to show
		if (strstr(ioc[idx].cam_info.url, value) != NULL) return 1;	// reverse filter
	}
	return 0;
}

static int _release_integer_addr()
{
	g_fr = 0;
	g_to = 0;
	return 0;
}

static int _prepare_integer_addr(char *value)
{
	char addr[256];
	char strfrom[64];
	char strto[64];
	unsigned long int ufrom, uto;
	int ret;

	if (g_fr != 0) return 1;
	memset(strfrom, 0x00, sizeof(strfrom));
	memset(strto, 0x00, sizeof(strto));

	strcpy(addr, value);
	ifn_trim_char(addr, '.');	// anti exception
	ret = make_ipaddr(addr, strfrom, strto);
	if (ret == -1) {
		_release_integer_addr();
		return 0;
	}
	ufrom = inet_addr(strfrom);
	uto = inet_addr(strto);

	g_fr = ntohl(ufrom);
	g_to = ntohl(uto);

	if (g_fr > g_to) {
		_release_integer_addr();
	}

	return 0;
}

static int _filter_hostaddr(int idx, char *op, char *value)
{
	int ret = 0;
	if (!op) return 0;
	if (!value) return 0;

	if (strstr(value, "-") != NULL) {
		_prepare_integer_addr(value);		// for efficiency
		ret = _filter_hostaddr_range(idx, op, g_fr, g_to);
	}
	else {
		ret = _filter_hostaddr_cmp(idx, op, value);
	}
	return ret;
}

static int _filter_model(int idx, char *op, char *value)
{
	if (!op) return 0;
	if (!value) return 0;

	if (strcmp(op, "~") == 0) {			// including
		if (strstr(ioc[idx].cam_info.model, value) == NULL) return 1;		// reverse filter
	}
	else if (strcmp(op, "!") == 0) {	// excluding
		if (strstr(ioc[idx].cam_info.model, value) != NULL) return 1;		// reverse filter
	}
	return 0;
}

static int _filter_hide(int idx)
{
	return (ioc[idx].hidden == 1);
}

static int _filter_assigned(int idx)
{
	return (ioc[idx].assign_cnt > 0);
}

static int _clear_sort()
{
	int i;
	for (i = 0; i < MAX_OC; ++i) {
		sortidx[i] = -1;
	}
	return 0;
}

static int _is_sorted()
{
	return (sortidx[0] != -1);
}

static int _sort_url(int dir)
{
	// dir == 0 : ascending  (a -> z)
	// dir == 1 : descending (z -> a)
	int i, j;
	int cur;

	_clear_sort();

	for (i = 0; i < MAX_OC; ++i) {
		cur = 0;
		if (_is_empty_slot(i)) continue;

		for (j = 0; j < MAX_OC; ++j) {
			if (i == j) continue;
			if (strlen(ioc[j].cam_info.url) == 0) continue;

			if (dir == 1) {
				if (strcmp(ioc[i].cam_info.url, ioc[j].cam_info.url) < 0) {
					cur++;
				}
			}
			else {
				if (strcmp(ioc[i].cam_info.url, ioc[j].cam_info.url) > 0) {
					cur++;
				}
			}
		}

		while (1) {
			if (sortidx[cur] == -1) {
				sortidx[cur] = i;
				break;
			}
			cur++;
		}
	}

}

static int _is_hidden_cam(int idx)
{
	return (ioc[idx].hidden);
}


////////////////////////////////////////////////////////////
//
// public interfaces 
//

int ocam_init()
{
	memset(ioc, 0x00, sizeof(CAMERA_LIST_T) * MAX_OC);
	return 0;
}

int ocam_add_cam(CAMERA_INFO_T *info)
{
	int idx;
	int find = _find_camera(info->url);
	if (find != -1) {
		idx = find;
	}
	else {
		idx = _find_empty_slot();
		if (idx == -1) return -1;
    	ioc[idx].assign_cnt = 0;
	}

	_copy_caminfo(&ioc[idx].cam_info, info);

	if (_is_sorted())  _clear_sort();
	return 0;
}

int ocam_reg_cam(CAMERA_INFO_T *info)
{
	int idx;
	int find = _find_camera(info->url);
	if (find != -1) {
		idx = find;
	}
	else {
		idx = _find_empty_slot();
		if (idx == -1) return -1;
	}

	//_copy_caminfo(&ioc[idx].cam_info, info);
	ioc[idx].assign_cnt++;

	if (_is_sorted())  _clear_sort();
	return 0;
}

int ocam_remove_cam(char *url)
{
	int idx = _find_camera(url);
	if (idx == -1) return -1;

	_clear_camera(idx);
	if (_is_sorted())  _clear_sort();
	return 0;

}

int ocam_hide_cam(char *url)
{
	int idx = _find_camera(url);
	if (idx == -1) return -1;

	ioc[idx].hidden = 1;
	return 0;
}

int ocam_show_cam(char *url)
{
	int idx = _find_camera(url);
	if (idx == -1) return -1;

	ioc[idx].hidden = 0;
	return 0;
}

int ocam_assign(char *url)
{
	int idx = _find_camera(url);
	if (idx == -1) return -1;

	ioc[idx].assign_cnt++;
	return 0;
}

int ocam_unassign(char *url)
{
    int i;
	int idx = _find_camera(url);

/*	
	for (i = 0; i < MAX_OC; ++i) {
	    printf("[IUX:OCAM]%s, %d, idx : %d, target url : %s, src url : %s\n", __FUNCTION__, __LINE__, i, url, ioc[i].cam_info.url);
	}
*/
//    printf("[IUX:OCAM]%s, %d idx : %d\n", __FUNCTION__, __LINE__, idx);
	if (idx == -1) return -1;

	ioc[idx].assign_cnt--;
//    printf("[IUX:OCAM]%s, %d ioc[idx].assign_cnt : %d\n", __FUNCTION__, __LINE__, ioc[idx].assign_cnt);
	return 0;
}

int ocam_get_card_idx()
{
}

int ocam_save()
{
}

int ocam_reload()
{
}

int ocam_sort_by_url(int dir)
{
	_sort_url(dir);
	return 0;
}

int ocam_clear_sort()
{
	_clear_sort();
	return 0;
}

int ocam_filter(OCAM_FILTER_F field, char *op, char *value, OCAM_FILTER_CMB cmb)
{
	int i;
	int ret;
	
	if (field < OCAM_F_HOSTADDR || field > OCAM_F_NO) return -1;
	if (cmb < OCAM_C_AND || cmb > OCAM_C_NO) return -1;

	for (i = 0; i < MAX_OC; ++i) {
		if (cmb == OCAM_C_RST) ioc[i].filtered = OCAM_SHOW;
		else if (cmb == OCAM_C_AND) {
			if (ioc[i].filtered == OCAM_SHOW) continue;
		}

		if (_is_empty_slot(i)) continue;

		switch (field) {
		case OCAM_F_HOSTADDR:
			ret = _filter_hostaddr(i, op, value);
			break;
		case OCAM_F_MODEL:
			ret = _filter_model(i, op, value);
			break;
		case OCAM_F_HIDDEN:
			ret = _filter_hide(i);
			break;
		case OCAM_F_ASSIGNED:
			ret = _filter_assigned(i);
			break;
		}

		if (ret) ioc[i].filtered = OCAM_HIDE;
	}

	_release_integer_addr();
}

int ocam_clear_filter()
{
	int i;
	for (i = 0; i < MAX_OC; ++i) {
		ioc[i].filtered = OCAM_SHOW;
	}

	return 0;
}

int ocam_get_caminfo(BITMASK opt, CAMERA_INFO_T *buf, int *ret_cnt, int len)
{
	int i, j = 0;
	int cnt = 0;
	int idx;

	if (!_is_sorted()) opt &= ~OCAM_B_SORT;

	for (i = 0; i < MAX_OC; ++i) {
		if (_is_empty_slot(i)) continue;
		
		if (opt & OCAM_B_SORT) {
			idx = sortidx[i];
		}
		else {
			idx = i;
		}

		if (opt & OCAM_B_HIDE) {
			if (ioc[idx].hidden == 1) continue;
		}

		if (opt & OCAM_B_FILT) {
			if (ioc[idx].filtered == OCAM_HIDE) continue;
		}

		_copy_caminfo(&buf[j], &ioc[idx].cam_info);
		++j;
		if (j == len) break;
	}

	*ret_cnt = j;
	return 0;
}

int ocam_check_filter()
{
    int i;

    for (i = 0; i < MAX_OC; i++)
    {
        if (ioc[i].filtered) return 1;
    }

    return 0;
}

