/*
 * ix_func.c
 * 	- utility functions
 * 	- Dependency
 * 		ix_mem
 * 		GDK
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Dec 22, 2010
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <sys/mount.h>
#include <sys/time.h>
#include <errno.h>
#include "ix_func.h"
#include "ix_mem.h"
#include <execinfo.h>
#include <glib/gprintf.h>



#define DBG_LEVEL		9
#define DBG_MODULE		"IX_FUNC"
#define DMSG(level, format, args...) \
	do { \
		if (DBG_LEVEL && level && DBG_LEVEL >= level) { \
			fprintf(stderr, "[IUX:"DBG_MODULE"] %s():%d: "format"\n", __FUNCTION__, __LINE__, ##args); \
		} \
	} while (0)



#define MAX_LINE_LEN    511



#define GUINT64_SECOND  (G_USEC_PER_SEC * G_GINT64_CONSTANT (1000))
#define GUINT64_USECOND (GUINT64_SECOND / G_GINT64_CONSTANT (1000000))
#define GUINT64_TO_GTIMEVAL(t,tv) \
do { \
(tv).tv_sec  = ((guint64) (t)) / GUINT64_SECOND;          \
(tv).tv_usec = (((guint64) (t)) -                         \
               ((guint64) (tv).tv_sec) * GUINT64_SECOND)  \
					                   / GUINT64_USECOND; \
} while (0) 



///////////////////////////////////////////////////////
//
// gdk
//

gboolean ifn_is_in_rect(GdkRectangle *rect, gdouble x, gdouble y)
{
	int ix = (int)x;
	int iy = (int)y;

	return ((rect->x <= ix && ix <= rect->x + rect->width) &&
			(rect->y <= iy && iy <= rect->y + rect->height));
}

gboolean ifn_is_in_igrect(IGRECT *rect, int x, int y)
{
	int ix = (int)x;
	int iy = (int)y;

	return ((rect->x <= ix && ix <= rect->x + rect->w) &&
			(rect->y <= iy && iy <= rect->y + rect->h));
}

int ifn_set_gdkcolor(int red, int green, int blue, GdkColor *color)
{
	color->red = (unsigned short)(red << 8);
	color->green = (unsigned short)(green << 8);
	color->blue = (unsigned short)(blue << 8);
	return 0;
}

int ifn_save_pixbuf(GdkPixbuf *pbuf, const char *fullpath)
{
	int ret = 0;
	
    ret = gdk_pixbuf_save(pbuf, fullpath, "jpeg", NULL, "quality", "100", NULL);
    if (!ret) g_message("%s, %d, gdk_pixbuf_save FAIL", __FUNCTION__, __LINE__);
    
    return (ret == 0 ? -1 : 0);
}

int ifn_save_drawable(GdkDrawable *drawable, GdkRectangle *rect, const char *fullpath)
{
	GdkPixbuf *nbuf = NULL;
	int ret = 0;

    nbuf = gdk_pixbuf_get_from_drawable(0, drawable, 0, rect->x, rect->y, 0, 0, rect->width, rect->height);
    if (!nbuf) {
        g_message("%s, %d, gdk_pixbuf_get_from_drawable FAIL", __FUNCTION__, __LINE__);    
        return -1;
    }
    
    ret = gdk_pixbuf_save(nbuf, fullpath, "jpeg", NULL, "quality", "100", NULL);
    if (!ret) g_message("%s, %d, gdk_pixbuf_save FAIL", __FUNCTION__, __LINE__);
    
    g_object_unref(nbuf);
    
    return (ret == 0 ? -1 : 0);
}


///////////////////////////////////////////////////////
//
// 
//

static int _is_valid_special_letter(int letter)
{
    int ret = 0;

    g_message("%s, %d, letter:%d", __FUNCTION__, __LINE__, letter);

    switch(letter)
    {
        case 35:    // "#"
        case 95:    // "_"
        case 45:    // "-"
        case 40:    // "("
        case 41:    // ")"
        case 64:    // "@"
        case 46:    // "."
            ret = 1;
        break;
        default:                    
        break;
    }

    return ret;
}


int ifn_is_all_digit(const char *str)
{
	int ret = strlen(str);
	if (*str == '-' || *str == '+') {
		if (ret == 1) return 0;
		ret--;
		str++;
	}
	while (isdigit(*str++)) --ret;
	return (ret == 0);		// 1 means all digit, 0 means not all
}

int ifn_is_all_alnum(const char *str)
{
	int ret = strlen(str);
	
	while (isalnum(*str++)) --ret;
	return (ret == 0);		// 1 means num and alphabet
}

int ifn_is_all_itxstyle_title(const char *str)
{
	int ret = strlen(str);
	
	while (isalnum(*str) || _is_valid_special_letter(*str)) { *str++; --ret; }
	return (ret == 0);		// 1 means num, alphabet and special letter("#", "_", "-", "(", ")", "@", ".")
}

///////////////////////////////////////////////////////
//
// threads
//

GThread *ifn_make_thread(void *(*proc)(void *), void *data)
{
	GError *error = NULL;
	GThread *thread = NULL;
	thread = g_thread_create((GThreadFunc)proc, data, FALSE, &error);
	//thread = g_thread_create((GThreadFunc)proc, data, TRUE, &error);
	if (thread == NULL) {
		DMSG(1, "GThread creating is fail. (err msg = %s)\n", error->message);
		g_error_free (error);
	}
	return thread;
}

int ifn_init_thread()
{
	if (!g_thread_supported()) g_thread_init(NULL);
	return 0;
}

///////////////////////////////////////////////////////
//
// assert
//

int ifn_show_backtrace()
{
	void *frame_addrs[16];
	char **frame_strings;
	size_t backtrace_size;
	int i;

	backtrace_size = backtrace(frame_addrs, 16);
	frame_strings = backtrace_symbols(frame_addrs, backtrace_size);
	for (i = 0; i < backtrace_size; ++i) {
		printf("%d: [0x%x] %s\n", i, frame_addrs[i], frame_strings[i]);
	}
	free(frame_strings);
	return 0;
}

void iassert(int expr)
{
	if (expr) return;
	ifn_show_backtrace();
	g_assert(expr);	
}

///////////////////////////////////////////////////////
//
// rand
//

static int seed = NULL;
int ifn_rand()
{
	GTimeVal cur;
	int ret;
	g_get_current_time(&cur);
	seed += cur.tv_usec;
	ret = rand_r(&seed);
	seed += ret;
	return ret; 
}

///////////////////////////////////////////////////////
//
// string 
//

char *ifn_strltrim(char *str)
{
	int len = strlen(str);
	while (str[0] == ' ' || str[0] == '\t') memmove(str, str + 1, len--); 
	return str;
}

char *ifn_strrtrim(char *str)
{
	int len = strlen(str);
	while ( str[len - 1] == ' ' || str[len - 1] == '\t') str[len-- - 1] = NULL;
	return str;
}

char *ifn_strtrim(char *str)
{
	return ifn_strrtrim(ifn_strltrim(str));
}

char *ifn_strltrim_b(char *str, char *buf)
{
	int len = strlen(str);
	strcpy(buf, str);
	while (buf[0] == ' ' || buf[0] == '\t') memmove(buf, buf + 1, len--); 
	return buf;
}

char *ifn_strrtrim_b(char *str, char *buf)
{
	int len = strlen(str);
	strcpy(buf, str);
	while ( buf[len - 1] == ' ' || buf[len - 1] == '\t') buf[len-- - 1] = NULL;
	return buf;
}

char *ifn_strtrim_b(char *str, char *buf)
{
	ifn_strltrim_b(str, buf);
	return ifn_strrtrim(buf);
}

int ifn_tolower(char *str)
{
	int i;
	int len = strlen(str);
	for (i = 0; i < len; ++i) 
	{	
		*str = tolower(*str);
		str++;
	}
	return 0;
}

int ifn_toupper(char *str)
{
	int i;
	int len = strlen(str);
	for (i = 0; i < len; ++i) 
	{
		*str = toupper(*str);
		str++;
	}
	return 0;
}

int ifn_count_char(char *str, char c)
{
	int cnt = 0;
	while (*str++ != 0) cnt += (*str == c);	
	return cnt;
}

char *ifn_strrnchar(char *p, char c, int n)
{
	char *q = p + (strlen(p) - 1);
	while (q != p) {
		if (*q == c) --n;
		if (n == 0) break;
		--q;
	}
	return q;
}

char *ifn_strnchar(char *p, char c, int n)
{
	while (*p != 0) {
		if (*p == c) --n;
		if (n == 0) break;
		++p;
	}
	return p;
}

void ifn_trim_char(char *str, char c)
{
	char *p, *q;
	p = str;
	q = str + (strlen(str) - 1);
	while (*p == c && *p++ != 0);
	while (*q == c && q-- != p);
	memmove(str, p, q - p + 1);
	str[q - p + 1] = 0;
}

///////////////////////////////////////////////////////
//
// 
//

char *ifn_find_file_name_in_path(char *full_path)
{
	char *ptr = strrchr(full_path, '/');
	return (ptr == 0) ? ptr = full_path : ptr + 1;
}

int ifn_make_pathname(char *dir, char *file, char *pathname)
{
    sprintf(pathname, "%s/%s", dir, file);
	return 0;
}

static char **_new_dirlist(char *dir, FILTER_PROC filter, FNAME_FILTER_PARAM_T *fparam, int *sum_cnt, int recurs)
{
	DIR *dp;
	struct dirent *entry;
	struct stat statbuf;
	int filter_pass = 1;
	char cur_dir[256];
	char path[256];

	static int cnt = 0;
	static char **name_buf = 0;

	if (recurs == 0) {
		cnt = 0;
		name_buf = 0;
	}   

	if ((dp = opendir(dir)) == NULL) {
		DMSG(1, "cannot open directory: %s\n", dir);
		return;
	}   
	chdir(dir);
	while ((entry = readdir(dp)) != NULL) {
		lstat(entry->d_name, &statbuf);
		if (S_ISDIR(statbuf.st_mode)) {
			if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0) continue;
			if (fparam->opt & IFO_RECURSIVE_SEARCH)
				_new_dirlist(entry->d_name, filter, fparam, sum_cnt, 1); 

			if (filter) filter_pass = filter(entry->d_name, fparam);
			if (filter_pass) {
				cnt++;
				name_buf = (char **)irealloc(name_buf, cnt * sizeof(char*));

				getcwd(cur_dir, 255);
				if (fparam->opt & IFO_INCLUDE_FULLPATH)
					ifn_make_pathname(cur_dir, entry->d_name, path);
				else
					strcpy(path, entry->d_name);

				*(name_buf + cnt - 1) = imalloc(strlen(path) + 1);
				strcpy(*(name_buf + cnt - 1), path);
			}
		}   
	}   
	chdir("..");
	closedir(dp);
	if (sum_cnt) *sum_cnt = cnt;
	return name_buf;
}


static char **_new_filelist(char *dir, FILTER_PROC filter, FNAME_FILTER_PARAM_T *fparam, int *sum_cnt, int recurs)
{
	DIR *dp;
	struct dirent *entry;
	struct stat statbuf;
	int filter_pass = 1;
	char cur_dir[256];
	char path[256];

	static int cnt = 0;
	static char **name_buf = 0;

	if (recurs == 0) {
		cnt = 0;
		name_buf = 0;
	}   

	if ((dp = opendir(dir)) == NULL) {
		DMSG(1, "cannot open directory: %s\n", dir);
		return;
	}   
	chdir(dir);
	while ((entry = readdir(dp)) != NULL) {
		lstat(entry->d_name, &statbuf);
		if (S_ISDIR(statbuf.st_mode)) {
			if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0) continue;
			if (fparam->opt & IFO_RECURSIVE_SEARCH)
				_new_filelist(entry->d_name, filter, fparam, sum_cnt, 1); 
		}   
		else {
			if (filter) filter_pass = filter(entry->d_name, fparam);
			if (filter_pass) {
				cnt++;
				name_buf = (char **)irealloc(name_buf, cnt * sizeof(char*));

				getcwd(cur_dir, 255);
				if (fparam->opt & IFO_INCLUDE_FULLPATH)
					ifn_make_pathname(cur_dir, entry->d_name, path);
				else
					strcpy(path, entry->d_name);

				*(name_buf + cnt - 1) = imalloc(strlen(path) + 1);
				strcpy(*(name_buf + cnt - 1), path);
			}
		}   
	}   
	chdir("..");
	closedir(dp);
	if (sum_cnt) *sum_cnt = cnt;
	return name_buf;
}

char **ifn_new_filelist(char *dir, FILTER_PROC proc, FNAME_FILTER_PARAM_T *fparam, int *ret_cnt)
{
	char **flist;
	char *cur_dir = getenv("PWD");
	int cnt;
	
	flist = _new_filelist(dir, proc, fparam, &cnt, 0); 
	if (cnt > 0) {
		flist = (char **)irealloc(flist, (cnt + 1) * sizeof(char*));
		flist[cnt] = 0;    // null termination of the file name list
		if (ret_cnt) *ret_cnt = cnt;
	}

	chdir(cur_dir);

	return flist;
}

int ifn_free_filelist(char **name_buf)
{
	int i;
	for (i = 0; name_buf[i]; ++i) ifree(name_buf[i]);
	ifree(name_buf); 
	return 0;
}

char **ifn_new_dirlist(char *dir, FILTER_PROC proc, FNAME_FILTER_PARAM_T *fparam, int *ret_cnt)
{
	char **dlist;
	char *cur_dir = getenv("PWD");
	int cnt;
	
	dlist = _new_dirlist(dir, proc, fparam, &cnt, 0); 
	if (cnt > 0) {
		dlist = (char **)irealloc(dlist, (cnt + 1) * sizeof(char*));
		dlist[cnt] = 0;    // null termination of the file name list
		if (ret_cnt) *ret_cnt = cnt;
	}

	chdir(cur_dir);

	return dlist;
}

int ifn_free_dirlist(char **name_buf)
{
	int i;
	for (i = 0; name_buf[i]; ++i) ifree(name_buf[i]);
	ifree(name_buf); 
	return 0;
}

int ifn_is_file_exist(const char *path)
{
	if (access(path, F_OK) == 0) return 1;
	return 0;
}

int ifn_make_dir_p(const char *s) 
{
	char *p; 
	char name[1024];
	memset(name , 0x00, sizeof(name));

	for (p = strchr(s + 1, '/'); p; p = strchr(p + 1, '/')){
		strncpy(name, s, p - s); 
		if(access(name, F_OK) == -1 && mkdir(name, 0744) == -1) return -1;
	}   
	if(access(s, F_OK) == -1) mkdir(s, 0744);
	return 0;
}

int ifn_delete_file_in_dir(char *dir)
{
	DIR *dp;
	struct dirent *entry;
	struct stat statbuf;

	if ((dp = opendir(dir)) == NULL) {
		DMSG(1, "cannot open directory: %s\n", dir);
		return -1;
	}
	
	chdir(dir);
	while ((entry = readdir(dp)) != NULL) {
		lstat(entry->d_name, &statbuf);
		if (S_ISDIR(statbuf.st_mode)) {
			/* Found a directory, but ignore . and .. */
			if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0)
				continue;
			ifn_delete_file_in_dir(entry->d_name);
			remove(entry->d_name);
		}   
		else remove(entry->d_name);
	}   
	chdir("..");
	closedir(dp);
	return 0;
}

int ifn_remove_dir_rf(char *dir)
{
	char *cur_dir = getenv("PWD");
	char dirpath[256];
	
	if (strcmp(dir, ".") == 0) return -1;
	if (strcmp(dir, "..") == 0) return -1;

	chdir(dir);
	getcwd(dirpath, 255);
	if (strcmp(cur_dir, dirpath) == 0) return -1;

	ifn_delete_file_in_dir(dir);
	remove(dir);

	chdir(cur_dir);
	DMSG(1, "current directory : %s", cur_dir);
	
	return 0;
}

int ifn_mount_device(const char *device_name, const char *mnt_path, const gchar *fs)
{
	int ret;
	time_t t = time(NULL);
	struct tm lt = {0};
	char options[128];			

	localtime_r(&t, &lt);		
	snprintf( options, sizeof(options), "time_offset=%d", (lt.tm_gmtoff/60) ); 
	g_message("%s %s(%s) fs[%s] options[%s]",__FUNCTION__, device_name, mnt_path, fs, options);
	
	if ((ret = mount(device_name, mnt_path, fs, 0, options)) == -1) {
		
#if 0
		char cmd[128];
		int ret;
		sprintf(cmd, "/NFDVR/ntfs-3g %s %s",device_name, mnt_path);
		ret = proxy_system(cmd, 1, 1000);	
#else
		g_message("%s ret[%d]",__FUNCTION__, ret);
		
		//FIXME choissi 2014-01-28 ���� 2:14:58   try to ntfs
		if ((ret = mount(device_name, mnt_path, "ntfs", 0, NULL)) == -1) {
			return -1;
		}else{
			char cmd[128];
			int ret;

			umount(mnt_path);
/*
			sprintf(cmd, "/NFDVR/ntfs-3g %s %s",device_name, mnt_path);
			ret = proxy_system(cmd, 1, 1000);	
*/
		}
#endif

	}
	if (ret == 0) return 0;
	if (errno == EBUSY) return 0;
	return -1;
}

int ifn_unmount_device(const char *device_name, const char *mnt_path)
{
	if (umount(mnt_path) == -1) return -1;
	return 0;
}

int ifn_is_mounted_dev(char *dev_name)
{
	int ret = 0;
	char line[MAX_LINE_LEN + 1]; 
	char name[255];
	FILE *fp = fopen("/proc/mounts", "r");

	if (fp == NULL) return 0; 
	while (fgets(line, MAX_LINE_LEN, fp) != NULL) {   
		if ((strstr(line, "/dev/") - line) == 0) {   
			sscanf(line, "%s", name);
			if (strcmp(dev_name, name) == 0) { ret = 1; break; }
		}   
	}   
	fclose(fp);
	return ret;
}

char *ifn_find_token(char *string, char delimiter, int n, char *buf)
{
	int cnt = 0;
	char *p1 = string;
	char *p2 = string;

	while (cnt <= n) {
		p1 = strchr(p1, delimiter);
		if (!p1) break;
		++cnt;
		++p1;
	}   

	cnt = 0;
	while (cnt <= n + 1) {
		p2 = strchr(p2, delimiter);
		if (!p2) break;
		++cnt;
		++p2;
	}   

	if (!p1 || !p2) return 0;
	strncpy(buf, p1, (p2 - p1 - 1));
	*(buf + (p2 - p1 - 1)) = 0;
	return buf;
}


///////////////////////////////////////////////////////
//
// date / time
//

int ifn_convert_date(struct tm *ttm, FM_DATE_E fm_date, char *buf_date)
{
	*buf_date = 0;

	switch (fm_date) {
	case YYYYMMDD:
		sprintf(buf_date, "%04d-%02d-%02d", 
			ttm->tm_year + 1900,
			ttm->tm_mon + 1,
			ttm->tm_mday);
		break;
	case YYMMDD:
		sprintf(buf_date, "%02d-%02d-%02d", 
			ttm->tm_year + 1900,
			ttm->tm_mon + 1,
			ttm->tm_mday);
		break;
	case MMDDYY:
		sprintf(buf_date, "%02d-%02d-%02d", 
			ttm->tm_mon + 1,
			ttm->tm_mday,
			ttm->tm_year + 1900);
		break;
	case MMDDYYYY:
		sprintf(buf_date, "%02d-%02d-%04d", 
			ttm->tm_mon + 1,
			ttm->tm_mday,
			ttm->tm_year + 1900);
		break;
	case DDMMYYYY:
		sprintf(buf_date, "%02d-%02d-%04d", 
			ttm->tm_mday,
			ttm->tm_mon + 1,
			ttm->tm_year + 1900);
		break;
	case YYMMDD_HOL:
		sprintf(buf_date, "%04d%02d%02d", 
			//(ttm->tm_year + 1900) % 100,
			ttm->tm_year + 1900,
			ttm->tm_mon + 1,
			ttm->tm_mday);
		break;
	}

	return strlen(buf_date);
}

int ifn_convert_day(struct tm *ttm, int* week, int* mon, int* day)
{
	int i, y, m, d;
	int res;

	y  =  ttm->tm_year + 1900;
	m  =  ttm->tm_mon + 1;
	d  =  ttm->tm_mday;
	
	*day = d;
	*mon = m;

	if(m < 3){
		y--;
		m+=12;
	}

	// Day Of Week
	res = (y+y/4-y/100+y/400+(13*m+8)/5+d)%7; 	
	
	// Week
	for(i = 1; i < 6; i++){
	    if(d < ( (7*i)+1) ){
		*week = i-1;
		break;	
	    }
	}
	//g_message("Func : %s, Line : %d, result : %d week : %d", __func__, __LINE__, res, *week);
	
	return res;
}

int ifn_convert_time(struct tm *ttm, FM_TIME_E fm_time, char *buf_time)
{
	guint temp;
	*buf_time = 0;

	temp = ttm->tm_hour;
	if (fm_time == H12)
	{
		if(temp>12)		temp -= 12;
		else if(!temp)	temp = 12;
	}

	switch (fm_time) {
	case H24:
		sprintf(buf_time, "%02d:%02d:%02d", 
			temp,
			ttm->tm_min,
			ttm->tm_sec);
		break;
	case H12:
		sprintf(buf_time, "%02d:%02d:%02d %s", 
			temp,
			ttm->tm_min,
			ttm->tm_sec,
			(ttm->tm_hour >= 12 ? "PM" : "AM"));
		break;
	}

	return strlen(buf_time);
}

int ifn_convert_date_ex(struct tm *ttm, FM_DATE_E fm_date, char *buf_date)
{
	*buf_date = 0;

	switch (fm_date) {
	case YYYYMMDD:
		sprintf(buf_date, "%04d%02d%02d", 
			ttm->tm_year + 1900,
			ttm->tm_mon + 1,
			ttm->tm_mday);
		break;
	case YYMMDD:
		sprintf(buf_date, "%02d%02d%02d", 
			ttm->tm_year + 1900,
			ttm->tm_mon + 1,
			ttm->tm_mday);
		break;
	case MMDDYY:
		sprintf(buf_date, "%02d%02d%02d", 
			ttm->tm_mon + 1,
			ttm->tm_mday,
			ttm->tm_year + 1900);
		break;
	case MMDDYYYY:
		sprintf(buf_date, "%02d%02d%04d", 
			ttm->tm_mon + 1,
			ttm->tm_mday,
			ttm->tm_year + 1900);
		break;
	case DDMMYYYY:
		sprintf(buf_date, "%02d%02d%04d", 
			ttm->tm_mday,
			ttm->tm_mon + 1,
			ttm->tm_year + 1900);
		break;
	}

	return strlen(buf_date);
}

int ifn_convert_time_ex(struct tm *ttm, FM_TIME_E fm_time, char *buf_time)
{
	guint temp;
	*buf_time = 0;

	temp = ttm->tm_hour;
	if (fm_time == H12)
	{
		if(temp>12)		temp -= 12;
		else if(!temp)	temp = 12;
	}

	switch (fm_time) {
	case H24:
		sprintf(buf_time, "%02d%02d%02d", 
			temp,
			ttm->tm_min,
			ttm->tm_sec);
		break;
	case H12:
		sprintf(buf_time, "%02d%02d%02d%s", 
			temp,
			ttm->tm_min,
			ttm->tm_sec,
			(ttm->tm_hour >= 12 ? "PM" : "AM"));
		break;
	}

	return strlen(buf_time);
}

int ifn_add_day(GTimeVal *time, int day)
{
	time->tv_sec += (day * 24 * 60 * 60);
	return 0;
}

int ifn_add_hour(GTimeVal *time, int hour)
{
	time->tv_sec += (hour * 60 * 60);
	return 0;
}

int ifn_add_minute(GTimeVal *time, int minute)
{
	time->tv_sec += (minute * 60);
	return 0;
}

int ifn_add_day_b(GTimeVal *time, int day, GTimeVal *ret)
{
	ret->tv_sec = time->tv_sec + (day * 24 * 60 * 60);
	ret->tv_usec = time->tv_usec;
	return 0;
}

int ifn_add_hour_b(GTimeVal *time, int hour, GTimeVal *ret)
{
	ret->tv_sec = time->tv_sec + (hour * 60 * 60);
	ret->tv_usec = time->tv_usec;
	return 0;
}

int ifn_add_minute_b(GTimeVal *time, int minute, GTimeVal *ret)
{
	ret->tv_sec = time->tv_sec + (minute * 60);
	ret->tv_usec = time->tv_usec;
	return 0;
}

int ifn_get_localtime_text(time_t time, FM_DATE_E fm_date, FM_TIME_E fm_time, char *buf)
{
	struct tm ttm;
	char buf_date[16];
	char buf_time[16];

	memset(buf_date, 0x00, sizeof(buf_date));
	memset(buf_time, 0x00, sizeof(buf_time));

	localtime_r(&time, &ttm);

	ifn_convert_date(&ttm, fm_date, buf_date);
	ifn_convert_time(&ttm, fm_time, buf_time);

	strcpy(buf, buf_date);
	strcat(buf, "  ");	
	strcat(buf, buf_time);

	return 0;
}

int ifn_get_localtime_text_ex(time_t time, FM_DATE_E fm_date, FM_TIME_E fm_time, char *buf)
{
	struct tm ttm;
	char buf_date[16];
	char buf_time[16];

	memset(buf_date, 0x00, sizeof(buf_date));
	memset(buf_time, 0x00, sizeof(buf_time));

	localtime_r(&time, &ttm);

	ifn_convert_date_ex(&ttm, fm_date, buf_date);
	ifn_convert_time_ex(&ttm, fm_time, buf_time);

	strcpy(buf, buf_date);
	strcat(buf, "_");	
	strcat(buf, buf_time);

	return 0;
}

int ifn_get_local_day_text(time_t time, FM_DATE_E fm_date, char *buf)
{
	struct tm ttm;
	char buf_time[32];

	memset(buf_time, 0x00, sizeof(buf_time));
	localtime_r(&time, &ttm);
	ifn_convert_date(&ttm, fm_date, buf_time);
	strcpy(buf, buf_time);

	return 0;
}

int ifn_get_local_hourmin_text(time_t time, FM_TIME_E fm_time, char *buf)
{
	struct tm ttm;
	char buf_time[16];

	memset(buf_time, 0x00, sizeof(buf_time));
	localtime_r(&time, &ttm);
	ifn_convert_time(&ttm, fm_time, buf_time);
	strcpy(buf, buf_time);

	return 0;
}

int ifn_get_localtime_text_g(GTimeVal *time, FM_DATE_E fm_date, FM_TIME_E fm_time, char *buf)
{
	struct tm ttm;
	char buf_date[16];
	char buf_time[16];

	memset(buf_date, 0x00, sizeof(buf_date));
	memset(buf_time, 0x00, sizeof(buf_time));

	localtime_r(&time->tv_sec, &ttm);

	ifn_convert_date(&ttm, fm_date, buf_date);
	ifn_convert_time(&ttm, fm_time, buf_time);

	strcpy(buf, buf_date);
	strcat(buf, "  ");	
	strcat(buf, buf_time);

	return 0;
}

int ifn_get_gmtime_text(time_t time, FM_DATE_E fm_date, FM_TIME_E fm_time, char *buf)
{
	struct tm ttm;
	char buf_date[16];
	char buf_time[16];

	memset(buf_date, 0x00, sizeof(buf_date));
	memset(buf_time, 0x00, sizeof(buf_time));

	gmtime_r(&time, &ttm);

	ifn_convert_date(&ttm, fm_date, buf_date);
	ifn_convert_time(&ttm, fm_time, buf_time);

	strcpy(buf, buf_date);
	strcat(buf, "  ");	
	strcat(buf, buf_time);

	return 0;
}

int ifn_get_gmtime_text_g(GTimeVal *time, FM_DATE_E fm_date, FM_TIME_E fm_time, char *buf)
{
	struct tm ttm;
	char buf_date[16];
	char buf_time[16];

	memset(buf_date, 0x00, sizeof(buf_date));
	memset(buf_time, 0x00, sizeof(buf_time));

	gmtime_r(&time->tv_sec, &ttm);

	ifn_convert_date(&ttm, fm_date, buf_date);
	ifn_convert_time(&ttm, fm_time, buf_time);

	strcpy(buf, buf_date);
	strcat(buf, "  ");	
	strcat(buf, buf_time);

	return 0;
}

int ifn_get_local_hourmin(time_t timet, int *hour, int *min, int *sec)
{
	struct tm ttm;
	localtime_r(&timet, &ttm);
	if (hour) *hour = ttm.tm_hour;
	if (min) *min = ttm.tm_min;
	if (sec) *sec = ttm.tm_sec;
	return 0;
}

int ifn_get_gmt_hourmin(time_t timet, int *hour, int *min, int *sec)
{
	struct tm ttm;
	gmtime_r(&timet, &ttm);
	if (hour) *hour = ttm.tm_hour;
	if (min) *min = ttm.tm_min;
	if (sec) *sec = ttm.tm_sec;
	return 0;
}

int ifn_get_gmt_datetime(time_t timet, int *year, int *mon, int *day, int *hour, int *min, int *sec)
{
	struct tm ttm;

	gmtime_r(&timet, &ttm);
	if (year) *year = ttm.tm_year + 1900;
	if (mon) *mon = ttm.tm_mon + 1;
	if (day) *day = ttm.tm_mday;
	if (hour) *hour = ttm.tm_hour;
	if (min) *min = ttm.tm_min;
	if (sec) *sec = ttm.tm_sec;
	
	return 0;
}

int ifn_get_local_day(time_t timet, int *year, int *mon, int *day)
{
	struct tm ttm;
	localtime_r(&timet, &ttm);
	if (year) *year = ttm.tm_year + 1900;
	if (mon) *mon = ttm.tm_mon + 1;
	if (day) *day = ttm.tm_mday;

	return 0;
}

int ifn_get_gmt_day(time_t timet, int *year, int *mon, int *day)
{
	struct tm ttm;
	gmtime_r(&timet, &ttm);
	if (year) *year = ttm.tm_year + 1900;
	if (mon) *mon = ttm.tm_mon + 1;
	if (day) *day = ttm.tm_mday;
	return 0;
}

int ifn_get_tm_from_timet()
{
}

time_t ifn_get_gmt_from_date(int year, int mon, int day)
{
	return ifn_get_gmt_from_local(year, mon, day, 0, 0, 0);
}
       
time_t ifn_get_gmt_from_data_tml(int year, int mon, int day, int hour)
{
	return ifn_get_gmt_from_local(year, mon, day, hour, 0, 0);
}

int ifn_timet_to_gtv(time_t timet, GTimeVal *ret)
{
	ret->tv_sec = timet;
	ret->tv_usec = 0;
	return 0;
}

time_t ifn_gtv_to_timet(GTimeVal *gtv)
{
	return gtv->tv_sec;
}

time_t ifn_get_gmt_midnight(time_t time_sec)
{
	return ((time_sec / 86400) * 86400);
}

time_t ifn_get_local_midnight(time_t time_sec)
{
	int year, mon, day, hour;
	
	ifn_get_local_day(time_sec, &year, &mon, &day);
	
	return ifn_get_gmt_from_date(year, mon, day);
}

time_t ifn_get_local_midnight_tml(time_t time_sec)
{
	int year, mon, day, hour;
	
	ifn_get_local_day(time_sec, &year, &mon, &day);
	ifn_get_local_hourmin(time_sec, &hour,0, 0);
	
	return ifn_get_gmt_from_data_tml(year, mon, day, hour);
}

time_t ifn_get_gmtime_from_local(time_t time_sec)
{
    int year, mon, day, hour, min, sec;

	ifn_get_local_day(time_sec, &year, &mon, &day);
	ifn_get_local_hourmin(time_sec, &hour, &min, &sec);
	
	return ifn_get_gmt_from_local(year, mon, day, hour, min, sec);
}

time_t ifn_get_gmt_from_local_ex(int year, int mon, int day, int hour, int min, int sec, int *is_dst)
{
	time_t gmt;
	struct tm ttm;

	memset(&ttm, 0x00, sizeof(struct tm));
	ttm.tm_year = year - 1900;
	ttm.tm_mon = mon - 1;
	ttm.tm_mday = day;
	ttm.tm_hour = hour;
	ttm.tm_min = min;
	ttm.tm_sec = sec;
	gmt = mktime(&ttm);
	if (is_dst) *is_dst = ttm.tm_isdst;

	gmt -= (timezone + ttm.tm_gmtoff);
	return gmt;
}

time_t ifn_get_gmt_from_local(int year, int mon, int day, int hour, int min, int sec)
{
	time_t gmt;
	struct tm ttm;

	memset(&ttm, 0x00, sizeof(struct tm));
	ttm.tm_year = year - 1900;
	ttm.tm_mon = mon - 1;
	ttm.tm_mday = day;
	ttm.tm_hour = hour;
	ttm.tm_min = min;
	ttm.tm_sec = sec;
	gmt = mktime(&ttm);

	gmt -= (timezone + ttm.tm_gmtoff);
	return gmt;
}

time_t ifn_get_local_timet()
{
    GTimeVal tv;

    g_get_current_time(&tv);

    return ifn_gtv_to_timet(&tv);
}

int ifn_is_in_dst(time_t utc)
{
    time_t cv_utc;
    struct tm ttm;
    localtime_r(&utc, &ttm);
    return (ttm.tm_isdst);
}

int ifn_get_hours_in_day(time_t daytime, char *hour_arr, int arr_cnt)
{
	time_t pos = ifn_get_local_midnight(daytime);
	int bday;
	int day;
	int hour;
	int cnt = 0;

	ifn_get_local_day(pos, 0, 0, &bday);
	while (1) {
		ifn_get_local_day(pos, 0, 0, &day);
		if (bday != day) break;

		ifn_get_local_hourmin(pos, &hour, 0, 0);
		if (hour_arr && arr_cnt > cnt) hour_arr[cnt] = hour;
		++cnt;
		pos += 3600;
	}

	return cnt;	
}

int ifn_is_leap_year(int year)
{
	return ((year > 0) && !(year % 4) && ((year % 100) || !(year % 400)));
}

int ifn_get_days_in_month(int year, int mon)
{
	int days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	int rdays = days[mon - 1];
	if (ifn_is_leap_year(year) && mon == 2) ++rdays;
	return rdays;
}

int ifn_has_dst_change(int year, int mon)
{
	int days = ifn_get_days_in_month(year, mon);
	time_t tt = ifn_get_gmt_from_local(year, mon, 1, 0, 0, 0);
	int i;
	int diff1 = 0;
	int diff2 = 0;
	
	diff1 = ifn_is_in_dst(tt);
	for (i = 2; i <= days; ++i) {
		tt = ifn_get_gmt_from_local(year, mon, i, 0, 0, 0);
		diff2 = ifn_is_in_dst(tt);
		if (diff1 != diff2) return 1;
		diff1 = diff2;
	}

	return 0;
}

int ifn_is_same_day(time_t t1, time_t t2)
{
	int year1, mon1, day1;
	int year2, mon2, day2;
	ifn_get_local_day(t1, &year1, &mon1, &day1);
	ifn_get_local_day(t2, &year2, &mon2, &day2);

	return (year1 == year2 && mon1 == mon2 && day1 == day2);
}

time_t ifn_convert_guint64_to_timet(guint64 timeval)
{
	GTimeVal tmp;
	memset(&tmp, 0x00, sizeof(GTimeVal));
	GUINT64_TO_GTIMEVAL(timeval, tmp);
	return tmp.tv_sec;
}

///////////// unit m < - > yard ///// 
double ifn_unit_change(gdouble data,gint flag)
{  
    if(flag)
        data = (data)*(3.28084);  
    else        
        data = (data)*(0.3048);  
    return data;

}

double ifn_unit_change_digit(gdouble data,guint digit)
{
    gint i=0, value=1;
    
    for(i=0; i<digit ; i++) value*=10;
    
    data = (int)(data*value);
    data = data/value;
    return data;
}

double ifn_unit_change_3d(gdouble data, gint flag)
{
    if(flag)
        data = (data)*(3.28084);
    else
        data = (data)*(0.3048);

    data = (int)(data*100);
    data = data/100;

    return data;
}


double ifn_unit_change_speed(gdouble data,gint flag)
{
    if(flag)
        data = (data)*(0.621372);
    else
        data = (data)*(1.609344);
    return data;
}

// not thread-safe
static struct timeval _ifn_tv_n, _ifn_tv_o;
int _ifn_tv_cnt;
int ifn_init_time_elap()
{
	_ifn_tv_cnt = 0;
	gettimeofday(&_ifn_tv_o, 0);
	return 0;
}

int ifn_show_time_elap(char *file, int line)
{
	_ifn_tv_cnt++;
	gettimeofday(&_ifn_tv_n, 0);
	printf("[%s, %d] [%02d: elapsed time = %ld\n",
			file, line, _ifn_tv_cnt,
			(_ifn_tv_n.tv_sec * 1000000 + _ifn_tv_n.tv_usec) -
			(_ifn_tv_o.tv_sec * 1000000 + _ifn_tv_o.tv_usec));
	_ifn_tv_o = _ifn_tv_n;
}

///////////////////////////////////////////////////////
//
// 
//

// le : little endian
int ifn_hex2btxt_le(unsigned int in, char buf[33])
{
	int p;
	for (p = 31; p >= 0; --p) buf[p] = in & (1 << (31 - p)) ? '1' : '0';
	buf[32] = 0;
	return 0;
}

///////////////////////////////////////////////////////
//
// 
//

enum {
	KB	= 0,
	MB	= 1,
	GB 	= 2,
	TB  = 3,
	PB  = 4,
	EB  = 5,
	ZB  = 6,
	YB  = 7,
};

static float _get_round_off_value(guint64 val, int *unit)
{
	float ret = 1.0;
	int p = KB;

	*unit = KB;
	if (val == 0) return 0;

	*unit = MB;
	do {
		if (val < 1024) return ret;
		if (val < 1024 * 1024) ret = (float)(val) / 1024;
		val >>= 10;
		*unit = ++p;
	} while (1);

	DMSG(9, "ROUND SIZE = %f\n", ret);
	return ret;
}

int ifn_convert_storage_size(gchar *buf, guint64 kb_size)
{
	gint unit = 0;
	float fsize = 0;

	*buf = 0;
	fsize = _get_round_off_value(kb_size, &unit);
	
	if (unit == KB) 	 g_sprintf(buf, "%.1f KB", fsize);
	else if (unit == MB) g_sprintf(buf, "%.1f MB", fsize);
	else if (unit == GB) g_sprintf(buf, "%.1f GB", fsize);
	else if (unit == TB) g_sprintf(buf, "%.1f TB", fsize);
	else if (unit == PB) g_sprintf(buf, "%.1f PB", fsize);
	else if (unit == EB) g_sprintf(buf, "%.1f EB", fsize);
	else if (unit == ZB) g_sprintf(buf, "%.1f ZB", fsize);
	else if (unit == YB) g_sprintf(buf, "%.1f YB", fsize);
	return 0;
}

/**
 * @brief  Tests whether a point (x, y) is near line or not.
 *
 * @param[in] x  x coordinate of the test point.
 * @param[in] y  y coordinate of the test point.
 * @param[in] d  Tolerance. (distance)
 * @param[in] pt1  The first point of the line.
 * @param[in] pt2  The second point of the line.
 *
 * @return
 *  - @c 1 if the point is closer than d.
 *  - @c 0 otherwise.
 */
int ifn_point_is_closer_line(gint x, gint y, gint d, IXPOINT *pt1, IXPOINT *pt2)
{
	gint l[3];
	long long d2;

	if ( ((pt1->x - d <= x && x <= pt2->x + d) ||
			(pt2->x - d <= x && x <= pt1->x + d)) &&
			((pt1->y - d <= y && y <= pt2->y + d) ||
			(pt2->y - d <= y && y <= pt1->y + d)) ) {
		/* Compute line. */
		l[0] = pt1->y - pt2->y;
		l[1] = pt2->x - pt1->x;
		l[2] = pt1->x * pt2->y - pt2->x * pt1->y;
		/* Compute distance. */
		d2 = (long long)(l[0] * x + l[1] * y + l[2]) *
				(l[0] * x + l[1] * y + l[2]) / (l[0] * l[0] + l[1] * l[1]);

		if ( d2 <= d * d )	/* Square of distance. */
			return 1;
	}
	
	return 0;
}

/**
 * @brief  Tests whether a point (x, y) is inside polygon or not.
 *
 * @param[in] x  x coordinate of the test point.
 * @param[in] y  y coordinate of the test point.
 * @param[in] d  Tolerance. (distance)
 * @param[in] p  polygon
 *
 * @return
 *  - @c 0 if the point is exterior.
 *  - @c 1 if the point is interior.
 */
int ifn_point_is_inside_polygon(gint x, gint y, gint d, IXPOLYGON *p)
{
	gint i, j, left = 0, dotp;

	for (i = 0, j = p->cnt - 1; i < p->cnt; j = i, i++) {
		if ( (p->pt[i].y < y && y <= p->pt[j].y) || (p->pt[j].y < y && y <= p->pt[i].y) ) {
			/* Check the sign of dot(l, x). */
			dotp = (p->pt[j].y - p->pt[i].y) * x + (p->pt[i].x - p->pt[j].x) * y +
					p->pt[j].x * p->pt[i].y - p->pt[i].x * p->pt[j].y;
			if ( p->pt[j].y < p->pt[i].y )
				dotp = -dotp;
			if ( dotp < 0 )
				left++;
		}
	}
	if ( d ) {
		for (i = 0, j = p->cnt - 1; i < p->cnt; j = i, i++) {
			if ( ifn_point_is_closer_line(x, y, d, &p->pt[j], &p->pt[i]) ) {
				left = 1;
				break;
			}
		}
	}
	
	return left & 1;	/* Odd count means (x, y) is interior point. */
}

int ifn_point_is_over_line(gint x, gint y, gint d, IXPOLYGON *p)
{
	gint i, j, left = 0, dotp;

	if ( d ) {
		for (i = 0, j = p->cnt - 1; i < p->cnt; j = i, i++) {
			if ( ifn_point_is_closer_line(x, y, d, &p->pt[j], &p->pt[i]) ) {
				left = 1;
				break;
			}
		}
	}
	
	return left & 1;	/* Odd count means (x, y) is interior point. */
}

int ifn_get_outbound_square(IXPOLYGON *p, IXSQUARE *square)
{
	int i;
	int minx = 0, miny = 0;
	int maxx = 0, maxy = 0;

	if (p->cnt == 0) return -1;
	minx = p->pt[0].x;
	miny = p->pt[0].y;
	maxx = p->pt[0].x;
	maxy = p->pt[0].y;

	for (i = 0; i < p->cnt; ++i) {
		if (minx > p->pt[i].x) minx = p->pt[i].x;
		if (miny > p->pt[i].y) miny = p->pt[i].y;
		if (maxx < p->pt[i].x) maxx = p->pt[i].x;
		if (maxy < p->pt[i].y) maxy = p->pt[i].y;
	}

	square->x1 = minx;
	square->x2 = maxx;
	square->y1 = miny;
	square->y2 = maxy;
	return 0;
}

int ifn_get_area(IXSQUARE *square)
{
	return abs(square->x1 - square->x2) * abs(square->y1 - square->y2);
}

int ifn_is_leapyear(int year)
{
	if ((year % 400) == 0) return 1;
	if ((year % 100) && ((year % 4) == 0)) return 1;

	return 0;
}

int ifn_get_monthdays(int year, int month)
{
	static int mdays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	int md = mdays[month - 1];
	if (ifn_is_leapyear(year) && month == 2) md++;
	return md;
}

int ifn_get_totaldays(int year, int month, int day)
{
	int y1 = year - 1;
	int total = 365 * y1;
	int m;

	total += (y1 / 400 - y1 / 100 + y1 / 4);
	for (m = 1; m < month; m++) {
		total += ifn_get_monthdays(year, m);
	}

	total += day;
	return total;
}

int ifn_get_yoil(int year, int month, int day)
{
	return (ifn_get_totaldays(year, month, day)) % 7;
}

int ifn_is_valid_date(int year, int month, int day)
{
	int mdays = ifn_get_monthdays(year, month);
	if (year < 0 || year > 9999) return 0;
	if (month < 1 || month > 12) return 0;
	if (day < 1 || day > mdays) return 0;

	return 1;
}

int ifn_get_week_of_month(int day)
{
	return ((day - 1) / 7) + 1;
}
