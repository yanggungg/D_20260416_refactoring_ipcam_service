/**
  Packet Dump API
 */
#ifndef __NF_IPCAM_PACKET_DUMP_C__
#define __NF_IPCAM_PACKET_DUMP_C__

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <spawn.h>
#include <sys/wait.h>
#include <mntent.h>	/* for getmntent(), et al. */
#include <ctype.h>
#include <sys/statvfs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include <sys/types.h>
#include <signal.h>

#include "nf_packet_dump.h"

#define nf_pd_msg(fmt, ...) if(1)\
	do { \
		printf("\e[1;32m[packetdump]\e[0m " fmt, ##__VA_ARGS__); \
	} while (0)

#define CH_MAX (32)

#define TCPDUMP_PATH "/usr/sbin/tcpdump"
#define FILE_PATH_LEN (128)

// __asm__(".symver posix_spawn,posix_spawn@GLIBC_2.4");
enum dump_type {
	PACKET_DUMP_TYPE_API = 1<<0,
	PACKET_DUMP_TYPE_RTP = 1<<1,
};

struct disk_info {
	unsigned long total_size;
	unsigned long free_size;
	unsigned long used_size;
};

struct dump_handler {
	int stat;
	int ch;
	pid_t pid;
	char path[128];
	struct disk_info disk_info;
};

struct dump_handler dump_handler;

static pid_t create_child_process(char *path, char *argv[]);
static char *string_trim(char* src);
static int send_signal(pid_t pid, int sig);
static int get_disk_space_info(char *path, struct disk_info *disk_info);
static int current_time_to_string(char *buffer, size_t buffer_size);
static int config_file_check(char *file_path, int *ch);

extern char **environ;
extern unsigned int get_available_ip(int port);

// EXTERN FUNCTION

void nf_ipcam_packet_dump_init()
{
	memset(&dump_handler, 0x00, sizeof(dump_handler));
}

int nf_ipcam_packet_dump_usb_check(char *path)
{
	int rtn = 0;
	struct disk_info info;
	int dump_ch = -1;
	char config_file[FILE_PATH_LEN] = {0,};
	size_t len = 0;


	nf_pd_msg("usb check: start\n");
	if(path == NULL) {
		rtn = -3;
		goto ends_label;
	}
	nf_pd_msg("usb check: path(%s)\n", path);

	if(dump_handler.stat != PACKET_DUMP_STATE_STOP) {
		nf_pd_msg("usb check: fail - status is not stop (%d)\n", dump_handler.stat);
		rtn = -1;
		goto ends_label;
	}

	len = strlen(path);

	if(path[ len - 1] == '/') {
		snprintf(config_file, FILE_PATH_LEN, "%s%s", path, "dump_config.txt");
	} else {
		snprintf(config_file, FILE_PATH_LEN, "%s/%s", path, "dump_config.txt");
	}



	// Config File Check
	if((rtn = config_file_check(config_file, &dump_ch)) != 0) {
		nf_pd_msg("usb check: fail - file check(%d)\n", rtn);
		rtn = -2;
		goto ends_label;
	}


	// TODO: check ch range
	dump_handler.ch = dump_ch - 1;
	dump_handler.stat = PACKET_DUMP_STATE_READY;
	strncpy(dump_handler.path, path, FILE_PATH_LEN);

	// USB Avail Size Check
	rtn = get_disk_space_info(path, &info);
	if(rtn != 0) {
		nf_pd_msg("usb check: fail - get disk space info fail(%d)\n", rtn);
		rtn = -3;
		goto ends_label;
	}

	strncpy(dump_handler.path, path, 128);
	memcpy(&dump_handler.disk_info, &info, sizeof(info));

ends_label:

	nf_pd_msg("usb check: end(rtn:%d)\n", rtn);
	return rtn;
}

int nf_ipcam_packet_dump_usb_remove()
{
	int status;
	int rtn = 0;
	int wait_rtn = 0;
	nf_pd_msg("remove: start\n");
	// dump status check
	if(dump_handler.stat == PACKET_DUMP_STATE_STOP ||
			dump_handler.stat == PACKET_DUMP_STATE_READY) {
		nf_pd_msg("remove: safely remove <pid(%d)/rtn(%d)> \n",dump_handler.pid, rtn);
		memset(&dump_handler, 0x00, sizeof(dump_handler));
		dump_handler.pid = -1;
		rtn = 0;
		goto ends_label;
	}

	// process running check? and kill
	rtn = send_signal(dump_handler.pid, SIGKILL);
	if(rtn != 0) {
		nf_pd_msg("remove: Send Signal Fail <pid(%d)/rtn(%d)> \n",dump_handler.pid, rtn);
	}

	// process wati
	wait_rtn = waitpid(dump_handler.pid, &status, WNOHANG);
	if(wait_rtn == 0) {
		nf_pd_msg("remove: prcoess running..| status(%i)/rtn(%d)\n", status, wait_rtn);
	} else if(wait_rtn == -1) {
		nf_pd_msg("remove: prcoess error. | status(%i)/rtn(%d)\n", status, wait_rtn);
	} else {
		nf_pd_msg("remove: prcoess finshed. | status(%i)/rtn(%d)\n", status, wait_rtn);
	}

	// dump status update
	memset(&dump_handler, 0x00, sizeof(dump_handler));
	dump_handler.pid = -1;
ends_label:

	nf_pd_msg("remove: end(%d)\n", rtn);
	return rtn;
}

int nf_ipcam_packet_dump_get_status(struct dump_status *status)
{
	int rtn = 0;
	struct disk_info di;

	nf_pd_msg("get status: start\n");

	if(status == NULL) {
		nf_pd_msg("get status: get status argument is null\n");
		rtn = -1;
		goto ends_label;
	}

	memset(status, 0x00, sizeof(struct dump_status));

	if(get_disk_space_info(dump_handler.path, &di) != 0) {
		nf_pd_msg("get status: get disk space info fail\n");
		rtn = -1;
		goto ends_label;
	}

	status->status = dump_handler.stat;
	status->size = di.total_size;
	status->used = di.used_size;
	status->avail = di.free_size;
	strncpy(status->path, dump_handler.path, 128);

ends_label:
	nf_pd_msg("get status: end(%d)\n", rtn);
	return rtn;
}

int nf_ipcam_packet_dump_stop()
{
	int rtn = 0;
	int status;
	int wait_rtn = 0;

	nf_pd_msg("dump stop: start\n");
	// dump status check
	if(dump_handler.stat != PACKET_DUMP_STATE_RUNN) {	
		nf_pd_msg("dump stop: status is not running...\n");
		rtn = -1;
		goto ends_label;
	}

	// pid process running check? and kill
	if(dump_handler.pid == -1) {
		nf_pd_msg("dump stop: dump stop fail: pid is -1\n");
		rtn = -2;
		goto ends_label;
	}

	rtn = send_signal(dump_handler.pid, SIGINT);
	if(rtn != 0) {
		nf_pd_msg("dump stop: Send Signal Fail\n");
	}

	sleep(1);

	// process wati (not retry)
	wait_rtn = waitpid(dump_handler.pid, &status, WNOHANG);
	if(wait_rtn == 0) {
		nf_pd_msg("remove: prcoess running..| status(%i)/rtn(%d)\n", status, wait_rtn);
	} else if(wait_rtn == -1) {
		nf_pd_msg("remove: prcoess error. | status(%i)/rtn(%d)\n", status, wait_rtn);
	} else {
		nf_pd_msg("remove: prcoess finshed. | status(%i)/rtn(%d)\n", status, wait_rtn);
	}

	// dump status update
	dump_handler.stat = PACKET_DUMP_STATE_READY;

ends_label:
	// sync
	nf_pd_msg("dump stop: end(rtn:%d)\n", rtn);
	return rtn;
}

int nf_ipcam_packet_dump_start()
{
	int rtn = 0;
	nf_pd_msg("dump start: start\n");
	int status;
	pid_t pid = -1;
	char dump_arg[4][64] = {{0,},};
	char time_string[64] = {0,};
	char *tmp = NULL;
	int wait_rtn = 0;
	size_t len = 0;
	char ipaddr_str[16];
	char proc_path[32] ={0,};
	struct in_addr addr;

	snprintf(proc_path, 32, "/proc/%d",dump_handler.pid);
	if(dump_handler.stat != PACKET_DUMP_STATE_READY) {
		if(access(proc_path, F_OK) == 0) {
			nf_pd_msg("dump start: already running..:pid(%d)\n", dump_handler.pid);
			goto ends_label;
		} else {
			nf_pd_msg("dump start: status is not ready(%d)\n", dump_handler.pid);
			goto ends_label;
		}
	}

	memset(dump_arg, 0x00, sizeof(dump_arg));
	memset(time_string, 0x00, sizeof(time_string));

	int ch = dump_handler.ch;
	memset(ipaddr_str, 0x00, 16);
	addr.s_addr = (in_addr_t) get_available_ip(ch);
	tmp = inet_ntoa(addr);
	strncpy(ipaddr_str, tmp, 16);

	// Argument 1: Host Address
	snprintf(dump_arg[0], 64, "host %s", ipaddr_str);
	nf_pd_msg("dump start: address(%s)\n", ipaddr_str);



	// Argument 2: Dumpfile
	current_time_to_string(time_string, 64);
	len = strlen(dump_handler.path);
	//nf_pd_msg("<DEBUG> dump_handler.path[%d] = %c\n", len - 1, dump_handler.path[ len - 1 ]);
	if(dump_handler.path[ len - 1] == '/') {
		snprintf(dump_arg[1], 64, "-w%sCH%02d_%s.pcap",dump_handler.path, dump_handler.ch+1, time_string);
	} else {
		snprintf(dump_arg[1], 64, "-w%s/CH%02d_%s.pcap",dump_handler.path, dump_handler.ch+1, time_string);
	}


	//printf("save file path: (%s)\n", dump_arg[1]);

	char *argv[] = {"tcpdump", "-v", dump_arg[0], dump_arg[1], NULL};



	// run tcpdump
	pid = create_child_process(TCPDUMP_PATH, argv);

	if(pid == -1) {
		nf_pd_msg("dump start: create child process(tcpdump) fail \n");
		rtn = pid;
		goto ends_label;
	}
	sleep(1);

	wait_rtn = waitpid(dump_handler.pid, &status, WNOHANG);
	//nf_pd_msg("<debug> status(%d) wait_rtn(%d)  %i\n", status, wait_rtn);
	if( wait_rtn != -1 && wait_rtn != 0 ) {
		nf_pd_msg("dump start: child status(%i)/rtn(%d)\n", status, wait_rtn);
	} else {
		perror("waitpid");
	}

	if(wait_rtn == 0) {
		nf_pd_msg("dump start: prcoess running..| status(%i)/rtn(%d)\n", status, wait_rtn);
	} else if(wait_rtn == -1) {
		nf_pd_msg("dump start: prcoess error. | status(%i)/rtn(%d)\n", status, wait_rtn);
	} else {
		nf_pd_msg("dump start: prcoess finshed. | status(%i)/rtn(%d)\n", status, wait_rtn);
	}



	// get pid
	dump_handler.pid = pid;
	dump_handler.stat = PACKET_DUMP_STATE_RUNN;

ends_label:
	nf_pd_msg("dump start: end(%d)\n", rtn);
	return rtn;
}


// STATIC FUNCTION

// 자식 프로세스를 생성하여 pid 를 얻는다.
static pid_t create_child_process(char *path, char *argv[])
{
	pid_t pid;
	int status;
	if(path == NULL || argv == NULL) {
		nf_pd_msg("create child: argument is NULL\n");
		pid = -1;
	}

	status = posix_spawn(&pid, path, NULL, NULL, argv, environ);

	if(status == 0) {
		nf_pd_msg("create child: child pid: %i\n", pid);

	} else {
		nf_pd_msg("create child: posix_spawn: %s\n", strerror(status));
		pid = -2;
	}
ends_label:

	return pid;
}

// pid 에 signal 을 보낸다. 
static int send_signal(pid_t pid, int sig)
{
	int status;
	status = kill(pid, sig);
#if 0
	if(waitpid(pid, &status, 0) != -1) {
		nf_pd_msg("child exited with status %i\n", status);
	} else {
		perror("waitpid");
	}

#endif
	return status;
}

// usb 의 여분공간 정보를 얻는다.
static int get_disk_space_info(char *path, struct disk_info *disk_info)
{
	int rtn  =0;
	struct statvfs  vfs;

	nf_pd_msg("disk info: start\n");
	// disk cache clean
	system("sync &");

	//nf_pd_msg("disk info: path: %s\n", path);
	if(path == NULL || disk_info == NULL) {
		nf_pd_msg("disk info: argument is null\n");
		rtn = -1;
		goto ends_label;
	}

	if(path[0] !='/') {
		nf_pd_msg("disk info: path: %s\n", path);
		nf_pd_msg("disk info: path start is not /\n");
		rtn = -2;
		goto ends_label;
	}


	//nf_pd_msg("disk info: statvfs start\n");
	if(statvfs(path, &vfs) != 0) {
		fprintf(stderr, "disk info: %s: statvfs failed: %s\n", __FUNCTION__, strerror(errno));
		rtn = -3;
		goto ends_label;
	}
	//nf_pd_msg("disk info: statvfs end\n");

	disk_info->total_size = (unsigned long) ((long)vfs.f_bsize * (unsigned long)vfs.f_blocks);
	disk_info->free_size = (unsigned long) ((long)vfs.f_bsize * (unsigned long)vfs.f_bavail);
	disk_info->used_size = (unsigned long) (disk_info->total_size - disk_info->free_size);
ends_label:
	nf_pd_msg("disk info: end(%d)\n", rtn);

	return rtn;

}

// 현재시간을 문자열로 얻는다.
static int current_time_to_string(char *buffer, size_t buffer_size)
{
	int rtn = 0;
	time_t tm_time;
	struct tm *st_time;

	if(buffer == NULL || buffer_size <= 0)
		return -1;

	time(&tm_time);
	st_time = localtime(&tm_time);
	rtn = (int)strftime(buffer, buffer_size, "%y-%m-%d_%H.%M.%S", st_time);

	return rtn;
}

/*****************************************************************************
  CONFIG FILE FORMAT
  ----------------------------------------------------------------------------
#dumpconfig
 */

// 설정파일 확인.
static int config_file_check(char *file_path, int *ch)
{
	int rtn = 0;
	char buffer[128];
	char option[32];
	char ch_tmp[8];
	FILE *fp = NULL;
	int i = 0;
	int header_flag=0;

	nf_pd_msg("config check: start file_path(%s)\n",file_path);

	if(file_path == NULL || ch == NULL)
	{
		nf_pd_msg("config check: argument is null[file_path, ch]\n");
		rtn =-1;
		goto ends_label;
	}

	fp = fopen(file_path, "r");

	if(fp == NULL)
	{
		nf_pd_msg("config check: File Open Fail\n");
		rtn =-1;
		goto ends_label;
	}

	if(access(file_path, F_OK|R_OK) != 0)
	{
		nf_pd_msg("config check: Config File Access Fail\n");
		rtn =-1;
		goto ends_label;
	}

	memset(buffer, 0x00, sizeof(buffer));
	while(fgets(buffer, 128 - 1, fp) != NULL)
	{
		nf_pd_msg("config check: data(%s)\n", buffer);

		puts("");
		if(header_flag == 0)
		{
			if(strncmp(buffer, "#dumpconfig", 11) != 0)
			{
				nf_pd_msg("config check: Header Miss Maching(%s)\n", buffer);

				rtn =-1;
				goto ends_label;
			}
			header_flag = 1;
			continue;
		}

		memset(option, 0x00, sizeof(option));

		int rtn=0;
		rtn = sscanf(buffer, "%32[a-zA-Z ]:%[ 0-9]", option, ch_tmp);
		nf_pd_msg("config check: sscanf [%d]\n", rtn);
		if(rtn > 1 && strncmp(option, "ch", 2) == 0)
		{
			rtn = sscanf(buffer, "%32[a-zA-Z]:%[ 0-9]", option, ch_tmp);
			*ch = atoi(ch_tmp);
			nf_pd_msg("config check: option [%s]\n", option);
			nf_pd_msg("config check: option value [%d]", atoi(ch_tmp));
			puts("");
			break;
		}

		memset(buffer, 0x00, sizeof(buffer));
	}

ends_label:
	nf_pd_msg("config check: end(%d)\n", rtn);

	if(fp != NULL)
		fclose(fp);

	return rtn;

}

// 문자열의 앞뒤 공백 제거
static char *string_trim(char* src)
{
	char *end;
	if(src == NULL) return src;

	while(isspace(*src)) src++;

	if(src == NULL) return src;

	end = src + strlen(src) -1;

	while(src < end && isspace(*end)) end--;

	*(end + 1) = 0;

	return src;
}



#endif //__NF_IPCAM_PACKET_DUMP_C__

