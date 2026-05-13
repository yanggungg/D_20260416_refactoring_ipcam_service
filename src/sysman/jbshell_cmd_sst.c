#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/time.h>
#include <semaphore.h>
#include <errno.h>

#include "libsst.h"
#include "libarch.h"
#include "libicmem.h"
#include "jbshell.h"

/*******************************************************************************
 * typedefs                                                                    *
 ******************************************************************************/
#define _DP_	fprintf(stderr, "%s() : %d \n",__func__,__LINE__)

static void
sst_event_function(int event, int arg, void *context)
{
	switch ( event ) {
		case SST_EVT_DISKFULL:
		case SST_EVT_OWSTART:
		case SST_EVT_OVERLAP:
		case SST_EVT_IOERR:
			printf("An event from driver: %d, %d\n", event, arg);
			break;
		default:
			break;
	}
}

#if 0
static char start_sst_help[] = "start_sst [max chan] [max streams]";
static int
start_sst(int argc, char **argv)
{
	int result;
	int max_channels, max_streams;

	if ( argc < 3 ) {
		printf("Invalid arguments\n%s\n", start_sst_help);
		return -1;
	}

	max_channels = atoi(argv[1]);
	max_streams = atoi(argv[2]);
	printf("Max channels = %d, Max streams = %d\n", max_channels, max_streams);

	/* TODO: get max_chan, max_streams from argv */
	result = sst_init(max_channels, max_streams,
			sst_malloc, sst_mfree, sst_mvtop, sst_mptov,
			sst_event_function, NULL, cmem_base_p, cmem_size);
	printf("sst_init() result: %d(%s)\n", result, sst_get_error_string(result));
	printf("%s to start sst.\n", result < 0 ? " Failed" : "Success");
	return result;
}
__commandlist(start_sst, "start_sst", "Start the SST", start_sst_help);
#endif


static char stop_sst_help[] = "stop_sst";
static int
stop_sst(int argc, char **argv)
{
	int result;

	result = sst_exit();
	printf("sst_exit() result: %d(%s)\n", result, sst_get_error_string(result));
	printf("%s to stop sst.\n", result < 0 ? " Failed" : "Success");
	return result;
}
__commandlist(stop_sst, "stop_sst", "Stop the SST", stop_sst_help);

static void
fsstart_event_function(int event, int arg, void *context)
{
	switch ( event ) {
		case SST_EVT_FSSTARTBEG:
			printf("%s: %s\n", __func__, sst_get_event_string(event));
			((int *)context)[2] = 1;
			break;
		case SST_EVT_FSSTARTCMPL:
			printf("\n%s: %s, error=%d\n",
					__func__, sst_get_event_string(event), arg);
			((int *)context)[0] = 1;
			((int *)context)[1] = arg;
			((int *)context)[2] = 0;
			break;
		case SST_EVT_FORMATBEG:
			printf("%s: %s, disk=%u\n",
					__func__, sst_get_event_string(event), arg);
			((int *)context)[3] = 1;
			break;
		case SST_EVT_FORMATCMPL:
			printf("\n%s: %s, error=%d\n",
					__func__, sst_get_event_string(event), arg);
			((int *)context)[3] = 0;
			break;
		case SST_EVT_RECOVERYBEG:
			printf("%s: %s\n", __func__, sst_get_event_string(event));
			((int *)context)[4] = 1;
			break;
		case SST_EVT_RECOVERYCMPL:
			printf("\n%s: %s, error=%d\n",
					__func__, sst_get_event_string(event), arg);
			((int *)context)[4] = 0;
			break;
		default:
			break;
	}
}


/*end backgroud---------------------------------------------------------------*/

static char fs_start_help[] = "fs_start [format_flag]";
static int
fs_start(int argc, char **argv)
{
	u8 format_flag;
	int result;
	volatile int context[5];
	struct sst_progress_t progress;

	if ( argc < 2 ) {
		printf("Invalid arguments\n%s\n", fs_start_help);
		return -1;
	}
	format_flag = atoi(argv[1]);


	/* Do fs start in non-blocking mode. */
	memset((void *)context, 0, sizeof(context));
	result = sst_fs_start(format_flag, 1,
			fsstart_event_function, (void *)context);
	if ( result < 0 ) {
		printf("sst_fs_start() result: %d(%s)\n",
				result, sst_get_error_string(result));
		return result;
	}

	usleep(10000);
	while ( !context[0] ) {
		if ( context[3] ) {
			if ( !sst_get_progress(SST_PRGT_FORMAT, &progress) &&
					progress.ucType == SST_PRGT_FORMAT ) {
				printf("\rFormatting: %u%% (%u / %u)",
						progress.uCurrent * 100 / progress.uTotal,
						progress.uCurrent, progress.uTotal);
				fflush(stdout);
			}
		}
		else if ( context[4] ) {
			if ( !sst_get_progress(SST_PRGT_RECOVERY, &progress) &&
					progress.ucType == SST_PRGT_RECOVERY ) {
				printf("\rRecoverying: %u%% (%u / %u)",
						progress.uCurrent * 100 / progress.uTotal,
						progress.uCurrent, progress.uTotal);
				fflush(stdout);
			}
		}
		usleep(100000);
	}
	usleep(10000);
	printf("File system start completed. result = %d(%s)\n",
			context[1], sst_get_error_string(context[1]));
	return context[1];
}
__commandlist(fs_start, "fs_start", "Start the file system", fs_start_help);

static void
fsstop_event_function(int event, int arg, void *context)
{
	switch ( event ) {
		case SST_EVT_FSSTOPBEG:
			printf("%s: %s\n", __func__, sst_get_event_string(event));
			break;
		case SST_EVT_FSSTOPCMPL:
			printf("\n%s: %s, error=%d\n",
					__func__, sst_get_event_string(event), arg);
			((int *)context)[0] = 1;
			((int *)context)[1] = arg;
			break;
		default:
			break;
	}
}

static char fs_stop_help[] = "fs_stop";
static int
fs_stop(int argc, char **argv)
{
	int result;
	volatile int context[2];
	struct sst_progress_t progress;


	result = arch_manager_stop();
	if ( result < 0 ) {
		printf("%s : result[%d](%s)\n", __FUNCTION__, result, nf_arch_get_error_string(result)); 
	}

	/* Do fs stop in non-blocking mode. */
	memset((void *)context, 0, sizeof(context));
	result = sst_fs_stop(1, fsstop_event_function, (void *)context);
	if ( result < 0 ) {
		printf("sst_fs_stop() result: %d(%s)\n",
				result, sst_get_error_string(result));
		return result;
	}

	usleep(10000);
	while ( !context[0] ) {
		if ( !sst_get_progress(SST_PRGT_FSSTOP, &progress) &&
				progress.ucType == SST_PRGT_FSSTOP ) {
			printf("\r%u%% (%u / %u)",
					progress.uCurrent * 100 / progress.uTotal,
					progress.uCurrent, progress.uTotal);
			fflush(stdout);
		}
		usleep(100000);
	}
	usleep(10000);
	printf("File system stop completed. result = %d(%s)\n",
			context[1], sst_get_error_string(context[1]));
			
			
	return context[1];
}
__commandlist(fs_stop, "fs_stop", "Stop the file system", fs_stop_help);

static void
delete_event_function(int event, int arg, void *context)
{
	switch ( event ) {
		case SST_EVT_DELDATABEG:
			printf("%s: %s\n", __func__, sst_get_event_string(event));
			break;
		case SST_EVT_DELDATACMPL:
			printf("\n%s: %s, error=%d\n",
					__func__, sst_get_event_string(event), arg);
			((int *)context)[0] = 1;
			((int *)context)[1] = arg;
			break;
		default:
			break;
	}
}

static char fs_delete_help[] = "fs_delete [time to delete]";
static int
fs_delete(int argc, char **argv)
{
	u32 t_delete;
	int result;
	volatile int context[2];
	struct sst_progress_t progress;

	if ( argc < 2 ) {
		printf("Invalid arguments\n%s\n", fs_delete_help);
		return -1;
	}
	t_delete = atoi(argv[1]);

	/* Do delete data in non-blocking mode. */
	memset((void *)context, 0, sizeof(context));
	result = sst_fs_delete_data(t_delete, 1,
			delete_event_function, (void *)context);
	if ( result < 0 ) {
		printf("sst_fs_delete_data() result: %d(%s)\n",
				result, sst_get_error_string(result));
		return result;
	}

	usleep(10000);
	while ( !context[0] ) {
		if ( !sst_get_progress(SST_PRGT_DELETE, &progress) &&
				progress.ucType == SST_PRGT_DELETE ) {
			printf("\r%u%% (%u / %u)",
					progress.uCurrent * 100 / progress.uTotal,
					progress.uCurrent, progress.uTotal);
			fflush(stdout);
		}
		usleep(100000);
	}
	usleep(10000);
	printf("Delete data completed. result = %d(%s)\n",
			context[1], sst_get_error_string(context[1]));
	return context[1];
}
__commandlist(fs_delete, "fs_delete", "Delete recorded data", fs_delete_help);

static void
preserve_event_function(int event, int arg, void *context)
{
	switch ( event ) {
		case SST_EVT_PRESERVEBEG:
			printf("%s: %s\n", __func__, sst_get_event_string(event));
			break;
		case SST_EVT_PRESERVECMPL:
			printf("\n%s: %s, error=%d\n",
					__func__, sst_get_event_string(event), arg);
			((int *)context)[0] = 1;
			((int *)context)[1] = arg;
			break;
		default:
			break;
	}
}

static char fs_preserve_help[] = "fs_preserve [time to preserve]";
static int
fs_preserve(int argc, char **argv)
{
	u32 t_preserve;
	int result;
	volatile int context[2];
	struct sst_progress_t progress;

	if ( argc < 2 ) {
		printf("Invalid arguments\n%s\n", fs_preserve_help);
		return -1;
	}
	t_preserve = atoi(argv[1]);

	/* Do preserve data in non-blocking mode. */
	memset((void *)context, 0, sizeof(context));
	result = sst_fs_preserve_data(t_preserve, 1,
			preserve_event_function, (void *)context);
	if ( result < 0 ) {
		printf("sst_fs_preserve_data() result: %d(%s)\n",
				result, sst_get_error_string(result));
		return result;
	}

	usleep(10000);
	while ( !context[0] ) {
		if ( !sst_get_progress(SST_PRGT_PRESERVE, &progress) &&
				progress.ucType == SST_PRGT_PRESERVE ) {
			printf("\r%u%% (%u / %u)",
					progress.uCurrent * 100 / progress.uTotal,
					progress.uCurrent, progress.uTotal);
			fflush(stdout);
		}
		usleep(100000);
	}
	usleep(10000);
	printf("Preserve data completed. result = %d(%s)\n",
			context[1], sst_get_error_string(context[1]));
	return context[1];
}
__commandlist(fs_preserve, "fs_preserve", "Preserve recorded data", fs_preserve_help);

static void
format_event_function(int event, int arg, void *context)
{
	switch ( event ) {
		case SST_EVT_FORMATBEG:
			printf("%s: %s, disk=%u\n",
					__func__, sst_get_event_string(event), arg);
			break;
		case SST_EVT_FORMATCMPL:
			printf("\n%s: %s, error=%d\n",
					__func__, sst_get_event_string(event), arg);
			((int *)context)[0] = 1;
			((int *)context)[1] = arg;
			break;
		default:
			break;
	}
}

static char disk_format_help[] = "disk_format [disk_id]";
static int
disk_format(int argc, char **argv)
{
	u8 diskid;
	int result;
	volatile int context[2];
	struct sst_progress_t progress;

	if ( argc < 2 ) {
		printf("Invalid arguments\n%s\n", disk_format_help);
		return -1;
	}
	diskid = atoi(argv[1]);

	/* Do format in non-blocking mode. */
	memset((void *)context, 0, sizeof(context));
#if 1		// choissi 2010-08-05 오후 3:13:17 diskmgr
	result = sst_disk_format(0, 1, 1,
			format_event_function, (void *)context);
#else 
	result = sst_disk_format(diskid, 0, 1,
			format_event_function, (void *)context);
#endif 			
	if ( result < 0 ) {
		printf("sst_disk_format() result: %d(%s)\n",
				result, sst_get_error_string(result));
		return result;
	}

	usleep(100000);
	while ( !context[0] ) {
		if ( !sst_get_progress(SST_PRGT_FORMAT, &progress) &&
				progress.ucType == SST_PRGT_FORMAT ) {
			printf("\rDisk %u, %u%% (%u / %u)", progress.uPrivate[0],
					progress.uCurrent * 100 / progress.uTotal,
					progress.uCurrent, progress.uTotal);
			fflush(stdout);
		}
		usleep(100000);
	}
	usleep(100000);
	printf("Format completed. result = %d(%s)\n",
			context[1], sst_get_error_string(context[1]));
	return context[1];
}
__commandlist(disk_format, "disk_format", "Format disks", disk_format_help);

static void
diskcheck_event_function(int event, int arg, void *context)
{
	switch ( event ) {
		case SST_EVT_DISKCHKBEG:
			printf("\n%s: %s\n",
					__func__, sst_get_event_string(event));
			break;
		case SST_EVT_DISKCHKCMPL:
			printf("\n%s: %s, error=%d\n",
					__func__, sst_get_event_string(event), arg);
			((int *)context)[0] = 1;
			((int *)context)[1] = arg;
			break;
		case SST_EVT_DISKCHKBAD:
			printf("\n%s: %s, bad=0x%08X\n",
					__func__, sst_get_event_string(event), arg);
			break;
		default:
			break;
	}
}

static char disk_check_help[] = "disk_check [group]";
static int
disk_check(int argc, char **argv)
{
	u8 group;
	int result;
	volatile int context[2];
	struct sst_progress_t progress;

	if ( argc < 2 ) {
		printf("Invalid arguments\n%s\n", disk_check_help);
		return -1;
	}
	group = atoi(argv[1]);
	printf("group = %d\n", group);

	memset((void *)context, 0, sizeof(context));
	result = sst_disk_check_start(group, 1,
			diskcheck_event_function, (void *)context);
	if ( result < 0 ) {
		printf("sst_disk_check_start() result: %d(%s)",
				result, sst_get_error_string(result));
		return result;
	}

	usleep(10000);
	while ( !context[0] ) {
		if ( !sst_get_progress(SST_PRGT_CHKDISK, &progress) &&
				progress.ucType == SST_PRGT_CHKDISK ) {
			printf("\r%3u%% (%u / %u) bad = %u  ",
					progress.uCurrent * 100 / progress.uTotal,
					progress.uCurrent, progress.uTotal,
					progress.uPrivate[0]);
			fflush(stdout);
		}
		usleep(100000);
	}
	printf("Disk check completed, result = %d(%s)\n",
			context[1], sst_get_error_string(context[1]));
	return context[1];
}
__commandlist(disk_check, "disk_check", "Check disk", disk_check_help);

static char disk_info_help[] = "disk_info";
static int
disk_info(int argc, char **argv)
{
	return 0;
}
__commandlist(disk_info, "disk_info", "Get disk information", disk_info_help);

static char disk_view_help[] = "disk_view [disk_id] [rcount]";
static int
disk_view(int argc, char **argv)
{
	u8 disk_id;
	u32 rcount;

	if ( argc < 3 ) {
		printf("Invalid arguments\n%s\n", disk_view_help);
		return -1;
	}
	disk_id = atoi(argv[1]);
	rcount = atoi(argv[2]);
	printf("disk_id = %d%s, rcount = %d\n", disk_id,
			disk_id == 0xFF ? "(entire)" : "", rcount);

	return 0;
}
__commandlist(disk_view, "disk_view", "Get the disk view data", disk_view_help);

static char disk_usage_help[] = "disk_usage [disk_id (0xFF for entire)]";
static int
disk_usage(int argc, char **argv)
{
	u8 disk_id;
	u32 used;
	u32 total;

	if ( argc < 2 )
		disk_id = 0xFF;
	else
		disk_id = atoi(argv[1]);
	
	if(disk_id != 0) {
		printf("%s\n",disk_usage_help);
		return 0;
	}
	
	printf("disk_id = %d%s\n", disk_id, disk_id == 0xFF ? "(entire)" : "");

#if 1 // choissinf 2008-11-14 오후 3:26:53 api change
	sst_fs_get_diskusage( disk_id, &used, &total, 2);
#else
	sst_fs_get_diskusage( disk_id, &used, &total);
#endif	
	

	printf("disk id : %d, used : %d (MB), total : %d (MB)\n", disk_id, used, total);
	return 0;
}
__commandlist(disk_usage, "disk_usage", "Get the disk usage", disk_usage_help);

static u32 my_atoi(const char *name)
{
    u32 val = 0;

    for (;; name++) {
	switch (*name) {
	    case '0'...'9':
		val = 10*val+(*name-'0');
		break;
	    default:
		return val;
	}
    }
}

static void
my_itoa(int cnt, char* buf)
{
	int i, dig, num, dig_cnt = 0;
	int see[7] = {1000000, 100000, 10000, 1000, 100, 10, 1};

	for(i = 0; i < 7; i++) {
		dig = cnt / see[i];
		if (dig > 0) {
			break;
		}
	}
	num = (i == 7) ? 6 : i;
	
	for(i = num; i < 7; i++) {
		dig = cnt / see[i];
		*(buf + dig_cnt++) = '0' + dig;
		cnt -= dig*see[i];
	}
}

static char tl_help[] = "tl_get [time] [resolution] [count] [split]";
static int
tl_get(int argc, char **argv)
{
	int result;
	int i,j;
	u8 *tl_data;
	u8 hide = 0x00;
	u32 split = 0xFF;			//split Yes.
#if 0
	u8 maxch = 4;			//4 channels. ipcam
	u64	chmask = 0xF0000; 	//16 
#else
	u8 maxch = (16+4);			//16 channels.
	u64	chmask = 0xFFFFF; 	//16 channels.
#endif	
	u32 time = 0x0;
	u32 count;
	u32 resolution = 0x0;
	u32 record_rt = 0x0;
	u32 loop_cnt = 1;
	int k;

	if ( argc < 5 ) {
		printf("%s\n", tl_help);
		return -1;
	}

	time = atoi(argv[1]);
	resolution = atoi(argv[2]);
	count = atoi(argv[3]);

	if(argv[1] == NULL || argv[2] == NULL || argv[3] == NULL){
		printf("%s\n",tl_help);
		return 0;
	}
	
	if(argv[4] != NULL){
		split =  atoi(argv[4]);
		if(split != 1 && split != 0){
			printf("wrong parameter\n");
			return 0;
		}
	}

	if(count > 1000){
		printf("count is too big\n");
		return 0;
	}
	if(resolution > 86400){
		printf("resolution is too big\n");
		return 0;
	}
	

	for( k = 0; k < loop_cnt; k ++){
		result = sst_tl_get(time,resolution,
				count,maxch,(u8)split,hide,chmask,&tl_data);
		if (result < 0){
			printf("error occured!!!\n");
		}else{
			puts("Time Line Information");
			if(tl_data != NULL){
				if(split){
					for(j = 0; j < maxch; j++){
						//print per channel.
						printf("ch : %d - \t",j);
						for(i = 0; i < count; i ++){
							record_rt = tl_data[(j*count) + i];
							switch(record_rt){
								case 0:
									printf("-");
									break;
								case 1:
									printf("N");
									break;
								case 2:
									printf("A");
									break;
								case 3:
									printf("B");
									break;
								case 4:
									printf("C");
									break;
								case 5:
									printf("D");
									break;
								case 6:
									printf("E");
									break;
								case 7:
									printf("F");
									break;
								case 8:
									printf("G");
									break;
								default:
									printf("?");
									break;
							}
						}
						puts("");
					}
				}else{
					//print a merged timeline information.
					for(i = 0; i < count; i ++){
						if(tl_data[i]){
							printf("#");
						}else{
							printf("-");
						}
						//printf("%02X ",tl_data[(j*16) + i]);
					}
					printf("\n");
				}
			}
			free(tl_data);
		}
	}

	return 0;
}
__commandlist(tl_get, "tl_get", "Time Line Information", tl_help);

static char set_rtlimit_help[] = "set_rtlimit [time limit] [delete mode 0/1]";
static int
set_rtlimit(int argc, char **argv){
	u32 rtlimit = 0;
	u8 d_mode = 0;
	if(argc < 3){
		printf("%s\n",set_rtlimit_help);
		return 0;
	}
	rtlimit = atoi(argv[1]);
	d_mode = atoi(argv[2]);
	if(d_mode != 1 && d_mode != 0){
		printf("%s\n",set_rtlimit_help);
		return 0;
	}
	sst_fs_set_rtlimit(rtlimit, d_mode, 0, NULL, NULL);
	return 1;
}
__commandlist(set_rtlimit, "set_rtlimit", "set recording time limit and mode.", set_rtlimit_help);

static char get_lasttime_help[] = "get_lasttime";
static int
get_lasttime(int argc, char **argv){
	u64 lasttime;
	sst_fs_get_lasttime(&lasttime);
	printf("%08X %08X",(u32)(lasttime >> 32),(u32)lasttime);
	return 0;
}
__commandlist(get_lasttime, "get_lasttime", "get the last time stamp (64bit)", get_lasttime_help);


static char disk_get_info_help[] = "disk_get_info";
static int
disk_get_info(int argc, char **argv){
	struct disk_info_t	 disk_info;
	if(sst_disk_get_info(&disk_info) >= 0){
		printf("Total disk : %d\n",disk_info.ucTotalDisks);
		printf("GroupA (internal) : %d\n",disk_info.ucGroupDisks[0]);
		printf("GroupB (external) : %d\n",disk_info.ucGroupDisks[1]);
		printf("SmodelNum : %s\n",disk_info.sModelNum[0][0]);			
	}
	return 0;
}
__commandlist(disk_get_info,"disk_get_info","retrive disk information.",disk_get_info_help);



static char diskview_help[] = "get_diskview [disk_id] [rcount]";
static int
get_diskview(int argc, char **argv){
	u32 		esize;
	u32		rcount;
	u8		disk_id;
	u8 		*view_data;

	if(argc < 3){
		printf("%s\n",diskview_help);
		return -1;
	}
	
	disk_id = atoi( argv[1] );
	rcount = atoi( argv[2] );

	if(disk_id > 0){
		printf("Disk id beigger than 0 is not available for now\n");
		return -1;
	}
	
	if(rcount % 4 > 0){
		printf("Invalid value -'rcount' must be aligned with 4\n");
		return -1;
	}
	
	view_data = (u8*)malloc( rcount );
	printf("allocate : %08X\n",(u32)view_data);
	if(view_data != NULL){
		sst_fs_get_diskview(disk_id,rcount,&esize,view_data);
		printf("print something.....\n");
		printf("free : %08X\n",(u32)view_data);
		free(view_data);
	}else{
		printf("failed to allocate momory\n");
	}
	return 0;
}
__commandlist(get_diskview,"get_diskview","show disk bars.",diskview_help);

static char set_wmode_help[] = " set_wmode [w_mode] [restart_wo] ";

static int
set_wmode( int argc, char **argv){
	u8			w_mode;
	u8			restart_wo;
	int			res;
	if(argc < 3){
		printf("%s\n",set_wmode_help);
		return -1;
	}

	w_mode = atoi(argv[1]);
	restart_wo = atoi(argv[2]);

	res = sst_fs_set_wmode(w_mode, restart_wo);
	switch(res){
		case -SST_ERR_SEQERR :
		case -SST_ERR_INVPARAM :
		case -SST_ERR_NOMEM :
		case -SST_ERR_IO :
			printf("Something's wrong\n");
			break;
		case 0:
			break;
		default:
			printf("Wrong return value\n");
	}
	return 0;
}
__commandlist(set_wmode,"set_wmode", "Set the disk write mode",set_wmode_help);

static char get_wmode_help[] = " get_wmode ";
static int
get_wmode( int argc, char **argv){
	u8			w_mode;
	int			res;

	res = sst_fs_get_wmode( &w_mode );
	if( res != 0 ) {
		printf("Something's wrong\n");
		return 0;
	}

	switch( w_mode ){
		case 0:
			printf(" write once mode\n ");
			break;
		case 1:
			printf(" over write mode\n ");
			break;
		default:
			printf("Wrong write mode\n ");
			break;
	}
	
	return 0;
}
__commandlist(get_wmode,"get_wmode", "Get the disk write mode",get_wmode_help);

static char sstat_help[] = "";
static int
sstat(int argc, char **argv)
{
	int result;
	struct sst_sstat_t stat;

	result = sst_debug_getsstat(&stat);
	if ( result < 0 ) {
		printf("Failed to get sst status. error=%d(%s)\n",
				result, sst_get_error_string(result));
		return -1;
	}
	printf("sst status:\n");
	printf("version              = %u\n", stat.uVersion);
	printf("max streams          = %u\n", stat.uMaxStreams);
	printf("opened streams       = %u\n", stat.uOpened);
	printf("recording streams    = %u\n", stat.uOpenRecStreams);
	printf("playback streams     = %u\n", stat.uOpenPlayStreams);
	printf("ri streams           = %u\n", stat.uOpenRIStreams);
	printf("log streams          = %u\n", stat.uOpenLogStreams);
	printf("arch streams         = %u\n", stat.uOpenArchStreams);
	printf("frame alloc          = %u\n", stat.uFrameAllocCount);
	printf("frame free           = %u\n", stat.uFrameFreeCount);
	printf("frame owned count    = %u\n", stat.uOwnedFrameCount);
	printf("frame owned size     = %u\n", stat.uOwnedFrameSize);
	return 0;
}
__commandlist(sstat, "sstat", "Get status of the SST", sstat_help);

static char fstat_help[] = "";
static int
fstat(int argc, char **argv)
{
	int result;
	struct sst_fstat_t stat;

	result = sst_debug_getfstat(&stat);
	if ( result < 0 ) {
		printf("Failed to get fs status. error=%d(%s)\n",
				result, sst_get_error_string(result));
		return -1;
	}
	printf("file system status:\n");
	printf("version             = %u\n", stat.uVersion);
	printf("file system version = %u\n", stat.uFsVersion);
	printf("max channels        = %u\n", stat.ucMaxChannels);
	printf("started             = %u\n", stat.ucStarted);
	printf("system disk         = %u\n", stat.ucSysDisk);
	printf("disks               = %u\n", stat.ucNumDisks);
	printf("recording channels  = %u\n", stat.ucRecChans);
	printf("write mode          = %u\n", stat.ucWriteMode);
	printf("disk full           = %u\n", stat.ucDiskFull);
	printf("delete mode         = %u\n", stat.ucDeleteMode);
	printf("record time limit   = %u\n", stat.uRecTimeLimit);
	printf("recording mbid      = %u\n", stat.uRecMBId);
	printf("recording lbid      = %u\n", stat.uRecLBId);
	printf("write once started mbid = %u\n", stat.uWOSMBId);
	printf("free mbs            = %u\n", stat.uFreeMBs);
	printf("total mbs           = %u\n", stat.uTotalMBs);
	printf("locked mbs          = %u\n", stat.uLockedMBs);
	printf("disk read rate      = %u\n", stat.uDiskReadRate);
	printf("disk write rate     = %u\n", stat.uDiskWriteRate);
	printf("disk read total     = %u\n", stat.uDiskReads);
	printf("disk write total    = %u\n", stat.uDiskWrites);
	printf("disk access usage   = %u\n", stat.uDiskAccessUsage);
	printf("disk average seek time = %u\n", stat.uDiskAvgSeekTime);
	return 0;
}
__commandlist(fstat, "fstat", "Get status of the file system", fstat_help);
 

static char disk_smart_help[] = "disk_smart [disk_id:0]";
static int
disk_smart(int argc, char **argv)
{
	char* buffer = NULL;
	struct smart_attr_sector_t *smartval = NULL;
	struct smart_thrs_sector_t *smartthres = NULL;
	int i = 0;
	int disk_id = 0;
	
	if( argc > 1 ) 
		disk_id = strtoul(argv[1],NULL,0);
		
	buffer = sst_get_smart_data(disk_id, &smartval, &smartthres);	
	g_print( "disk_id[%d]=======\n", disk_id );
	
	if(buffer != NULL)
	{		

#if 0
		for(i=0; i < MAX_ATTRNAME_LENGTH; i++) {
			if(smartval->attrValue[i].ucAttrID == POWER_ON_HOURS_COUNT_INDEX) {
				disk0_pwrhours = (u32)a_to_u5_le(smartval->attrValue[i].ucRawValue);
				break;
			}
		}
#endif
		g_print( buffer );
		
//		nf_debug_hexdump( smartval, sizeof(struct smart_attr_sector_t));
//		nf_debug_hexdump( smartthres, sizeof(struct smart_thrs_sector_t));
		
		free(buffer);
		buffer = NULL;
	
		free(smartval);
		free(smartthres);
	}
	
	g_print( "\n" );
	
	return 0;
}
__commandlist(disk_smart, "disk_smart", "Get S.M.A.R.T. Information", disk_smart_help);

#if 0

jbsh:> disk_smart_dump 3
enabled
disk_id[3]=======

	01 Raw Read Error Rate            x000F   6  100(         0)   253 Passed
	03 Spin Up Time                   x0003   0   98(         0)    97 Passed
	04 Start/Stop Count               x0032  20  100(       111)   100 Passed
	05 Reallocated Sector Count       x0033  36  100(         0)   100 Passed
	07 Seek Error Rate                x000F  30   76(  49577161)    60 Passed
	09 Power On Hours Count           x0032   0   97(      3357)    97 Passed
	0A Spin Retry Count               x0013  97  100(         0)   100 Passed
	0C Power Cycle Count              x0032  20  100(       111)   100 Passed
	BB (Unknown attribute)            x0032   0  100(         0)   100 Passed
	BD (Unknown attribute)            x003A   0  100(         0)   100 Passed
	BE (Unknown attribute)            x0022  45   60( 672727080)    44 Passed
	C2 HDA Temperature                x0022   0   40(77309411368)    56 Passed
	C3 Ecc on the Fly Count           x001A   0   79( 196942282)    63 Passed
	C5 Current Pending Sector Count   x0012   0  100(         0)   100 Passed
	C6 Offline Scan UNC Sector Count  x0010   0  100(         0)   100 Passed
	C7 Ultra ATA CRC Error Rate       x003E   0  200(         5)   200 Passed
	C8 Multi-zone Error Rate          x0000   0  100(         0)   253 Passed
	CA TA Increase Count              x0032   0  100(         0)   253 Passed

#endif

#if 0
typedef struct _xxNF_ARCH_DEV_INFO_T {
	u8			dev_id;
	u8			dev_type;			/* SCSI peripheral device type. 00h to 1Fh */
	u8			media_checked;
	u8			reserved[5];
	u64			media_size;			/* Valid only when MediaChecked is set. */
	u64			media_avail;		/* Available media space */
	u8			vendor[9];
	u8			product[17];
	u8			rev[5];	
	u8			dev_name[6];		/* /dev/sda1 --> "sda1" */
	u8			reserved2[2]; 	
} __attribute__((packed)) xxNF_ARCH_DEV_INFO ;
		
static char disk_get_devlist_help[] = "disk_get_devlist [disk_id:0]";
static int
disk_get_devlist(int argc, char **argv)
{
	xxNF_ARCH_DEV_INFO dev_table[16];
	int i, ret;
	
	memset ( dev_table, 0x00 , sizeof(dev_table  ));	
	ret = arch_dev_get_list(dev_table, NULL, NULL); 
	if(ret<0) {
		g_print("arch_dev_get_list failed ret[%d]\n", ret);		
	}else{
		for(i=0;i<4;++i)
		{
			g_print("idx              [%d]=======================\n", i);
			g_print("dev_id           [%d]\n",    dev_table[i].dev_id );
			g_print("media_checked    [%d]\n",    dev_table[i].media_checked );			
			g_print("media_size       [%lld]\n",    dev_table[i].media_size );
			g_print("media_avail      [%lld]\n",    dev_table[i].media_avail );			
			g_print("vendor           [%-9s]\n",  dev_table[i].vendor );			
			g_print("product          [%-17s]\n", dev_table[i].product );
			g_print("rev              [%-5s]\n",  dev_table[i].rev );
			g_print("dev_name         [%-6s]\n",  dev_table[i].dev_name );			
		}
	}
	return 0;
}
__commandlist(disk_get_devlist, "disk_get_devlist", "", disk_get_devlist_help);
#endif
