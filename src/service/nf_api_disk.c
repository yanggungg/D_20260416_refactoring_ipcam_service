// #include <gst/gst.h>
#include <gobj.h>
#include <gobjmedia.h>
#if 0
#include <gnu/targets/std.h>
#include <xdc/std.h>
#include <ti/sdo/ce/Engine.h>
#include <ti/sdo/ce/osal/Memory.h>
#endif
#include "nf_common.h"
#include "nf_api_disk.h"
#include "nf_api_archive.h"
#include "nf_api_eventlog.h"
#include "nf_notify.h"

#include "nf_codec_header.h"
#include "libsst.h"
#include "libicmem.h"
#include "unp.h"

#include "nf_api_raid.h"
#include "nf_watchdog.h"

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "api_disk"

#define DEBUG_API_DISK_LOG
#define ENABLE_UNC_UI_RECOVERY
#define DEBUG_AVI_PLAYER_FUNC_CALL

#ifdef ENABLE_DVR_AVI_PLAYER
#define ENABLE_DVR_MUL_PLAYER
#endif

#define	USE_SST
//#define SST_ALWAYS_FORMAT
#define DISK_USAGE_NOTIFY
#define DISK_USAGE_NOTIFY_SEC 	5
#define ENABLE_DVR_DISK_INFO		// by pakkhman
//#define ENABLE_DISK_IO_ERROR_MON

//#define ENABLE_ESTG_START	// choissi
#define ENABLE_DISK_PRESERVE // choissi
#define ENABLE_PRIVACY_DELETE // choissi

#ifdef DISK_USAGE_NOTIFY
	#include "nf_timer.h"
#endif

/** Statuc Function Definition **/
static int disk_format(void);
static void _log_put_disk_full_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
static void _log_put_disk_overwr_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
static void _log_put_disk_smart_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
static void _log_put_disk_exhaust_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
static void _log_put_disk_nodisk_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);

/** Gloval Variable Definition **/
static volatile gint _fs_is_online = 0;
static gulong _disk_usage_timer_handle = 0;
#if defined(ENABLE_DVR_DISK_INFO)
	static gulong _disk_info_timer_handle = 0;
#endif

// NF_DISK_WRITE_MODE_E
static volatile gint _disk_write_mode = 0;


#define HOT_SWAP_UNSUPPORTED
#ifdef HOT_SWAP_UNSUPPORTED
static NF_DISK_INFO st_disk_info;
static int got_disk_info = -1;
static int removed_disk_error_count = 0;
static int removed_disk_check_count = 0;
#endif

static cb_event_fxn_t 		_start_cb_event = NULL;
static gpointer 			_start_cb_ctx   = NULL;

static cb_event_fxn_t 		_stop_cb_event = NULL;
static gpointer 			_stop_cb_ctx   = NULL;

static cb_event_fxn_t 		_rtl_cb_event = NULL;
static gpointer 			_rtl_cb_ctx   = NULL;

static cb_event_fxn_t 		_delete_cb_event = NULL;
static gpointer 			_delete_cb_ctx   = NULL;

static cb_event_fxn_t 		_privacy_cb_event = NULL;
static gpointer 			_privacy_cb_ctx   = NULL;

#ifdef ENABLE_PRIVACY_DELETE
static guint				_privacy_start_time = 0;
static guint				_privacy_end_time = 0;
static guint				_privacy_ch_mask = 0;
#endif

// test
extern int test_frame_4;

/**
  Function Start!!
**/
static gboolean _disk_usage_timer_cb( gpointer arg)
{
	gboolean ret;
	
	guint used = 0;
	guint total = 0;
 
	g_return_val_if_fail( _fs_is_online == 1, 1);
	
	// FIXME disk_id 0xff => SST doesn't matter disk_id. always ALL_DISK
	ret = nf_disk_get_usage( 0, 0, &used, &total, NULL);
	if(ret && (total!=0) && (total > used))
	{
		nf_notify_fire_params("disk_usage", used, total,0,0);	// used & total ==> MegaBytes..
		g_message("%s %d/%d ",__FUNCTION__, used, total);
	}
	else
		g_message("%s fire failed %d/%d ",__FUNCTION__, used, total);
		
	return 1;
}

#if defined(ENABLE_DVR_DISK_INFO)
static gboolean _disk_info_timer_cb( gpointer arg)
{
	NF_DISK_INFO disk_info;
	guchar total_disks=0, formatted_disks=0;
	static gboolean is_rise=FALSE;

	memset(&disk_info, 0x0, sizeof(NF_DISK_INFO));
	nf_disk_get_info( &disk_info, NULL );
	total_disks=disk_info.total_disks;
	formatted_disks=disk_info.formatted_disks;
#if 0
	if((total_disks == 0) || (formatted_disks == 0))
	{
#else
	if(total_disks == 0)
	{
#endif
		if(!is_rise)
		{
			nf_notify_fire_params("disk_nodisk", 1, total_disks, formatted_disks, 0);
			is_rise=TRUE;
		}
	}
	else
		is_rise=FALSE;

	return TRUE;
}
#endif

static GQuark 
_nf_api_disk_error_quark (void)
{
	return g_quark_from_static_string ( G_LOG_DOMAIN "-error-quark");
}

#define NF_API_DISK_ERROR 	_nf_api_disk_error_quark()

#ifdef USE_SST
static void *
_nf_sst_malloc(u32 size, u32 align)
{
    return  ICMEM_alloc (size, align); 
}

static void
_nf_sst_mfree(void *mem, u32 size)
{
	gchar aaa[128];
    ICODEC_HEADER *ih = (ICODEC_HEADER *)mem;
	
	if (mem == NULL)
	{
		snprintf(aaa, 128, "SST mfree mem NULL");
		nf_eventlog_put_param( NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_WATCHDOG, aaa);
		return;
	}

	if (ih->chan > 63 && ih->chan != 127)
	{
		snprintf(aaa, 128, "SST mfree chan(%d)",ih->chan);
		nf_eventlog_put_param( NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_WATCHDOG, aaa);
		return;
	}
	if (ih->version != 1)
	{
		snprintf(aaa, 128, "SST mfree version(%d)",ih->version);
		nf_eventlog_put_param( NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_WATCHDOG, aaa);
		return;
	}
	if (ih->gst_buffer == NULL)
	{
		snprintf(aaa, 128, "SST mfree gst_buffer");
		nf_eventlog_put_param( NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_WATCHDOG, aaa);
		return;
	}

	if (0x7fffffffff < ih->gst_buffer) {
		g_message("%s():%d gst_buffer invalid addr %p\n", __FUNCTION__, __LINE__, ih->gst_buffer);
		return;
	}

//	g_print("----------------------------------------unref 0x%08x\n", GST_BUFFER_DATA(ih->gst_buffer));
//	g_print("----------------------------------------unref size %d\n", GST_BUFFER_SIZE(ih->gst_buffer));
	test_frame_4++;
    g_object_unref (ih->gst_buffer);
//	g_print("----------------------------------------unref 0x%08x\n", GST_BUFFER_DATA(ih->gst_buffer));
//	g_print("----------------------------------------unref size %d\n", GST_BUFFER_SIZE(ih->gst_buffer));
	return;
}

//convert  virtual address to physical address.
static gpointer
_nf_sst_mvtop(void *virt)
{
	
	return (gpointer)ICMEM_getBufferPhysicalAddress(virt);	
}

//convert physical address to virtual address.
static gpointer
_nf_sst_mptov(unsigned long phys)
{

	return ICMEM_getBufferVirtualAddress(phys);	
} 

static u32 _disk_io_err_cnt = 0;
static volatile guint _UNC_boot_key = 0;

static void
_fsinit_event_function(int event, int arg, void *context)
{	
	g_message("%s event[%d] arg[%d]", __FUNCTION__, event, arg);	
	
	switch ( event ) {
		case SST_EVT_DISKFULL:
			nf_notify_fire_params("disk_full",1,0,0,0);
			break;			
		case SST_EVT_UNCIOERROR:
			g_warning("%s SST_EVT_UNCIOERROR key[%x]",__FUNCTION__, arg);
			g_warning("%s SST_EVT_UNCIOERROR key[%x]",__FUNCTION__, arg);

			
			nf_sysdb_set_uint("disk.util.chkdsk_type", (guint)arg);
			nf_sysdb_save("disk");
			
			nf_sysdb_save_flush();
			nf_sysdb_save_flush();
			
			g_warning("%s SST_EVT_UNCIOERROR key[%x]",__FUNCTION__, arg);
			g_warning("%s SST_EVT_UNCIOERROR key[%x]",__FUNCTION__, arg);

			_UNC_boot_key = (guint)arg;

			sleep(3);
//			nf_dev_board_reset();
			//g_assert(0);
			
			break;
		case SST_EVT_OWSTART:
			{
				static unsigned int overwr_cnt = 0;						
				nf_notify_fire_params("disk_overwr", 1,0,0,0);
			}
			break;
		case SST_EVT_OVERLAP:		
			break;

		case SST_EVT_IOERR:
		case SST_EVT_DISKIOERR:
		case SST_EVT_RAIDIOERR:
			{

			++_disk_io_err_cnt;						
			g_warning("%s SST_EVT_IOERR cnt[%d] id[%d]",__FUNCTION__, _disk_io_err_cnt, arg);
			g_warning("%s SST_EVT_IOERR cnt[%d] id[%d]",__FUNCTION__, _disk_io_err_cnt, arg);
			
			nf_notify_fire_params("disk_write_fail", _disk_io_err_cnt, arg,0,0);
			
#ifdef	ENABLE_DISK_IO_ERROR_MON
			{
				char buff[256];
				snprintf(buff, sizeof(buff), "ERR %d", _disk_io_err_cnt);
				nf_sysdb_set_str("cam.C0.title", buff);
				nf_notify_fire_params("sysdb_change",NF_SYSDB_CATE_CAM,0,0,0);
			}
							
			nf_sysdb_set_uint("disk.extrec.E0.size", _disk_io_err_cnt);
			nf_sysdb_save("disk");
			nf_sysdb_save_flush();						
#endif

			}			
			break;
						
		default:
			break;
	}
} 

static void remap_log_to_hdd()
{
    int new_fd = -1;
	
	// nfs mount check
	if (nf_sysman_hotkey_is_nfs()) {
		printf("NFS mount detected, skip remapping log file to HDD\n");
		return;
	}
	
	new_fd = open("/mnt/fwup_hdd/serial.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (new_fd < 0) {
        perror("Failed to open new log file");
        return;
    }

    // 기존 stdout과 stderr을 새로운 파일로 변경
    dup2(new_fd, STDOUT_FILENO);
    dup2(new_fd, STDERR_FILENO);
    
    close(new_fd);
    printf("Log file successfully remapped to HDD\n");
}

static void _fsstart_complete(void)
{

	g_warning("%s !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! %d", __FUNCTION__, _fs_is_online);
	g_warning("%s !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! %d", __FUNCTION__, _fs_is_online);

	_fs_is_online = 1;
	nf_notify_fire_params("fs_status",(guint)_fs_is_online,0,0,0);
	
#ifdef DISK_USAGE_NOTIFY
	//_disk_usage_timer_cb(NULL);
		
	if( _disk_usage_timer_handle == 0)
	{
		_disk_usage_timer_handle = nf_timer_add(  DISK_USAGE_NOTIFY_SEC*1000, 
													_disk_usage_timer_cb,
													NULL);

		g_message("%s _disk_usage_timer_handle[%ld]", __FUNCTION__, 
				_disk_usage_timer_handle);
	}
#endif

#if 0
{
	static gint time_check = 0;	
	if(time_check == 0)
	{
		nf_sysman_time_check();
		time_check = 1;
	}
}
#endif	

	g_warning("%s !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! %d", __FUNCTION__, _fs_is_online);
	g_warning("%s !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! %d", __FUNCTION__, _fs_is_online);

#ifdef ENABLE_WATCHDOG
	nf_watchdog_kick( NF_WATCHDOG_MEMBER_SST );	
	nf_watchdog_ctrl( NF_WATCHDOG_MEMBER_SST, NF_WATCHDOG_TIME_SST_SEC , NF_WATCHDOG_DISABLE);	
#endif	

#ifdef ENABLE_PRIVACY_DELETE
	{
		guint seq = nf_sysdb_get_uint("disk.erase_section.seq");
		gint i, result;
		gchar buff[128];
		
		sst_privacy_info_t info_arr[NF_DISK_PRIVACY_INFO_MAX_CNT];		
		memset(info_arr, 0x00, sizeof(info_arr) );		
		
		for(i=0; i<seq && i< NF_DISK_PRIVACY_INFO_MAX_CNT; ++i)
		{
			snprintf(buff, sizeof(buff), "disk.erase_section.E%d.chmask", i);
			info_arr[i].ch_mask = nf_sysdb_get_uint(buff);	
			snprintf(buff, sizeof(buff), "disk.erase_section.E%d.from", i);
			info_arr[i].start_time = nf_sysdb_get_uint(buff);	
			snprintf(buff, sizeof(buff), "disk.erase_section.E%d.to", i);
			info_arr[i].end_time = nf_sysdb_get_uint(buff);				
		}
		result = sst_set_privacy_conf( info_arr, seq>NF_DISK_PRIVACY_INFO_MAX_CNT ? NF_DISK_PRIVACY_INFO_MAX_CNT : seq );
		if ( result < 0 ) {
			g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 	
		}								
	}
#endif
	int is_mounted = 0;
	is_mounted = nf_fw_network_upgrade_hdd_mount();
	nf_fw_network_upgrade_hdd_mount_state(0,&is_mounted);
	// if (is_mounted) remap_log_to_hdd();

}
static void
_fsstart_event_function(int event, int arg, void *context)
{
	switch ( event ) {
		case SST_EVT_FSSTARTBEG:
			printf("%s: %s\n", __FUNCTION__, sst_get_event_string(event));
			if(context) ((int *)context)[2] = 1;
			break;
		case SST_EVT_FSSTARTCMPL:
			printf("\n%s: %s, error=%d\n",
					__FUNCTION__, sst_get_event_string(event), arg);
																			
			if(context) 
			{
				((int *)context)[0] = 1;
				((int *)context)[1] = arg;
				((int *)context)[2] = 0;
			}		
			_fsstart_complete();
			break;
		case SST_EVT_FORMATBEG:
			printf("%s: %s, disk=%u\n",
					__FUNCTION__, sst_get_event_string(event), arg);			
			
			if(context) ((int *)context)[3] = 1;			
			break;
		case SST_EVT_FORMATCMPL:
			printf("\n%s: %s, error=%d\n",
					__FUNCTION__, sst_get_event_string(event), arg);
			
			if(context) ((int *)context)[3] = 0;			
			break;
		case SST_EVT_RECOVERYBEG:
			printf("%s: %s\n", __FUNCTION__, sst_get_event_string(event));
			
			if(context) ((int *)context)[4] = 1;			 
			break;
		case SST_EVT_RECOVERYCMPL:
			printf("\n%s: %s, error=%d\n",
					__FUNCTION__, sst_get_event_string(event), arg);

			if(context) ((int *)context)[4] = 0;
			break;
		default:
			break;
	}
			
	if(_start_cb_event)
		_start_cb_event( event, arg, _start_cb_ctx);
				
} 

static void _fsstop_complete(void)
{

	g_warning("%s !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! %d", __FUNCTION__, _fs_is_online);		
	g_warning("%s !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! %d", __FUNCTION__, _fs_is_online);

	_fs_is_online = 0;

	nf_notify_fire_params("fs_status",(guint)_fs_is_online,0,0,0);

	if( _disk_usage_timer_handle != 0)
	{
		nf_timer_remove( _disk_usage_timer_handle );
		_disk_usage_timer_handle = 0;
	}
	
//	nf_notify_fire_params("disk_full",0,0,0,0);
//	nf_notify_fire_params("disk_overwr", 0,0,0,0);

	g_warning("%s !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! %d", __FUNCTION__, _fs_is_online);
	g_warning("%s !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! %d", __FUNCTION__, _fs_is_online);

#ifdef ENABLE_WATCHDOG
	nf_watchdog_kick( NF_WATCHDOG_MEMBER_SST );	
	nf_watchdog_ctrl( NF_WATCHDOG_MEMBER_SST, NF_WATCHDOG_TIME_SST_SEC , NF_WATCHDOG_DISABLE);	
#endif	

}

static void
_fsstop_event_function(int event, int arg, void *context)
{
	switch ( event ) {
		case SST_EVT_FSSTOPBEG:
			printf("%s: %s\n", __FUNCTION__, sst_get_event_string(event));
			break;
		case SST_EVT_FSSTOPCMPL:
			printf("\n%s: %s, error=%d\n",
					__FUNCTION__, sst_get_event_string(event), arg);
					
			_fsstop_complete();			
			
			if(context)
			{
				((int *)context)[0] = 1;
				((int *)context)[1] = arg;
			}
			
			break;
		default:
			break;
	}
	
	if(_stop_cb_event)
		_stop_cb_event( event, arg, _stop_cb_ctx);
} 

static void
_format_event_function(int event, int arg, void *context)
{
	switch ( event ) {
		case SST_EVT_FORMATBEG:
			printf("%s: %s, disk=%u\n",
					__FUNCTION__, sst_get_event_string(event), arg);
			break;
		case SST_EVT_FORMATCMPL:
			printf("\n%s: %s, error=%d\n",
					__FUNCTION__, sst_get_event_string(event), arg);
			((int *)context)[0] = 1;
			((int *)context)[1] = arg;
			
 			nf_notify_fire_params("disk_overwr", 0,0,0,0);
			nf_notify_fire_params("disk_full",0,0,0,0);
			break;
		default:
			break;
	}
}  


static void _fsrtl_complete(void)
{
#ifdef ENABLE_WATCHDOG
	nf_watchdog_kick( NF_WATCHDOG_MEMBER_SST );	
	nf_watchdog_ctrl( NF_WATCHDOG_MEMBER_SST, NF_WATCHDOG_TIME_SST_SEC , NF_WATCHDOG_DISABLE);	
#endif				

}

static void
_fsrtl_event_function(int event, int arg, void *context)
{
	switch ( event ) {
		case SST_EVT_RTLIMITSTART:
			printf("%s: %s\n", __FUNCTION__, "SST_EVT_RTLIMITSTART");						
			break;
		case SST_EVT_RTLIMITCOMPL:
			printf("%s: %s\n", __FUNCTION__, "SST_EVT_RTLIMITCOMPL");
			_fsrtl_complete();			
			break;
		default:
			break;			
	}
			
	if(_rtl_cb_event)
		_rtl_cb_event( event, arg, _rtl_cb_ctx);				
} 


static void _delete_complete(void)
{
#ifdef ENABLE_WATCHDOG
	nf_watchdog_kick( NF_WATCHDOG_MEMBER_SST );	
	nf_watchdog_ctrl( NF_WATCHDOG_MEMBER_SST, NF_WATCHDOG_TIME_SST_SEC , NF_WATCHDOG_DISABLE);	
#endif				

}

static void
_fsdel_event_function(int event, int arg, void *context)
{

	switch ( event ) {
		case SST_EVT_DELDATABEG:
			printf("%s: %s\n", __FUNCTION__, "SST_EVT_DELDATABEG");						
			break;
		case SST_EVT_DELDATACMPL:
			printf("%s: %s\n", __FUNCTION__, "SST_EVT_DELDATACMPL");
			_delete_complete();			
			break;
		default:
			break;			
	}
			
	if(_delete_cb_event)
		_delete_cb_event( event, arg, _delete_cb_ctx);
} 

static void _privacy_complete(void)
{
#ifdef ENABLE_WATCHDOG
//	nf_watchdog_kick( NF_WATCHDOG_MEMBER_SST );	
//	nf_watchdog_ctrl( NF_WATCHDOG_MEMBER_SST, NF_WATCHDOG_TIME_SST_SEC , NF_WATCHDOG_DISABLE);	
#endif				

#ifdef ENABLE_PRIVACY_DELETE
	if(_privacy_ch_mask)
	{
		guint seq = nf_sysdb_get_uint("disk.erase_section.seq");
		char buff[128];	
		
		g_message("%s privacy seq[%d]", __FUNCTION__, seq);	
			
		snprintf(buff, sizeof(buff), "disk.erase_section.E%d.chmask", seq%NF_DISK_PRIVACY_INFO_MAX_CNT);
		nf_sysdb_set_uint(buff, _privacy_ch_mask);	
		snprintf(buff, sizeof(buff), "disk.erase_section.E%d.from", seq%NF_DISK_PRIVACY_INFO_MAX_CNT);
		nf_sysdb_set_uint(buff, _privacy_start_time);
		snprintf(buff, sizeof(buff), "disk.erase_section.E%d.to", seq%NF_DISK_PRIVACY_INFO_MAX_CNT);
		nf_sysdb_set_uint(buff, _privacy_end_time);
					
		nf_sysdb_set_uint("disk.erase_section.seq", ++seq);		
		
		nf_sysdb_save("disk");
		nf_sysdb_save_flush();

		_privacy_ch_mask = _privacy_start_time = _privacy_end_time = 0;
				
	}
#endif

}

static void
_fsprivacy_event_function(int event, int arg, void *context)
{

	switch ( event ) {
		case NF_SST_EVT_PRIVACYBEG:
			printf("%s: %s\n", __FUNCTION__, "SST_EVT_PRIVACYBEG");						
			break;
		case NF_SST_EVT_PRIVACYCMPL:
			printf("%s: %s\n", __FUNCTION__, "SST_EVT_PRIVACYCMPL");
			_privacy_complete();			
			break;
		default:
			break;			
	}
			
	if(_privacy_cb_event)
		_privacy_cb_event( event, arg, _privacy_cb_ctx);
} 

static int
fs_start(void)
{
	int format_flag;
	int result;
	volatile int context[5];
	struct sst_progress_t progress;

	format_flag = 1;

	_start_cb_event = NULL;
	_start_cb_ctx   = NULL;
		
	/* Do fs start in non-blocking mode. */
	memset((void *)context, 0, sizeof(context));
	result = sst_fs_start(format_flag, 1,
			_fsstart_event_function, (void *)context);

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

static int
fs_stop()
{
	int result;
	volatile int context[2];
	struct sst_progress_t progress;

	/* Do fs stop in non-blocking mode. */
	memset((void *)context, 0, sizeof(context));
	result = sst_fs_stop(1, _fsstop_event_function, (void *)context);
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

 
static int
disk_format(void)
{
	int	diskid;
	int result;
	volatile int context[2];
	struct sst_progress_t progress;
	
	diskid = 0;
	
	/* Do format in non-blocking mode. */
	memset((void *)context, 0, sizeof(context));
	result = sst_disk_format(diskid, 1, 1,
			_format_event_function, (void *)context);
	if ( result < 0 ) {
		printf("sst_disk_format() result: %d(%s)\n",
				result, sst_get_error_string(result));
		return result;
	}

	usleep(100000);
	while ( !context[0] ) {
		if ( !sst_get_progress(SST_PRGT_FORMAT, &progress) &&
				progress.ucType == SST_PRGT_FORMAT ) {
           if(progress.uCurrent != 0 && progress.uTotal != 0){
  			printf("\rDisk %u, %u%% (%u / %u)", progress.uPrivate[0],
					progress.uCurrent * 100 / progress.uTotal,
					progress.uCurrent, progress.uTotal);
			fflush(stdout);
           }
		}
		usleep(100000);
	}
	usleep(100000);
	printf("Format completed. result = %d(%s)\n",
			context[1], sst_get_error_string(context[1]));
	return context[1];
} 

#endif

// from nf_main.c
extern unsigned long _CMEM_physp; 
extern unsigned int _CMEM_size;	

static void 
_log_put_disk_full_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	g_return_if_fail(pinfo != NULL);
	
	g_message("%s [%ld.%06ld] type[%d] [%x]", __FUNCTION__,					
					pinfo->timestamp.tv_sec, pinfo->timestamp.tv_usec,
					pinfo->type, pinfo->d.params[0] );
					
	if( pinfo->d.params[0] )
	{
		nf_eventlog_put_param( &pinfo->timestamp, LT_HDD_FULL, 0,0, NULL);
	}	
}


static void 
_log_put_disk_overwr_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{

	g_return_if_fail(pinfo != NULL);
	
	g_message("%s [%ld.%06ld] type[%d] [%x]", __FUNCTION__,					
					pinfo->timestamp.tv_sec, pinfo->timestamp.tv_usec,
					pinfo->type, pinfo->d.params[0] );
					
	if( pinfo->d.params[0] == 1 ) // first overwrite start log
	{
		nf_eventlog_put_param( &pinfo->timestamp, LT_HDD_OW, 0,0, NULL);
	}	
	
}

static void 
_log_put_disk_smart_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{

	g_return_if_fail(pinfo != NULL);
	
	g_message("%s [%ld.%06ld] type[%d] [%x] [%x]", __FUNCTION__,					
					pinfo->timestamp.tv_sec, pinfo->timestamp.tv_usec,
					pinfo->type, pinfo->d.params[0], pinfo->d.params[1]);
					
	if( pinfo->d.params[0] == 0x01 ) // disk smart
	{
		// param2 -> disk_id , param1-> internal(0)/external(1)
		nf_eventlog_put_param( &pinfo->timestamp, LT_SMART_WARNING, ((gint)pinfo->d.params[1] + 1)/16, (gint)pinfo->d.params[1], NULL);
	}	

	if( pinfo->d.params[0] == 0x02 ) // mirroring failed!!
	{
		// param2 -> disk_id , param1-> internal(0)/external(1)
		nf_eventlog_put_param( &pinfo->timestamp, LT_SMART_WARNING, ((gint)pinfo->d.params[1] + 1)/16, (gint)pinfo->d.params[1], NULL);
	}	
}

static void 
_log_put_disk_exhaust_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	g_return_if_fail(pinfo != NULL);
	
	g_message("%s [%ld.%06ld] type[%d] [%x]  disk_write_mode[%d]", __FUNCTION__,
					pinfo->timestamp.tv_sec, pinfo->timestamp.tv_usec,
					pinfo->type, pinfo->d.params[0], _disk_write_mode );
					
	// choissi
	if( pinfo->d.params[0] == 1  && _disk_write_mode == NF_DISK_WRITE_MODE_WRITEONCE) {
		nf_eventlog_put_param( &pinfo->timestamp, LT_DISK_EVENT, 0, LP2_DISK_EVENT_DISK_EXHAUSTED, NULL);
	}
}


static void 
_log_put_disk_nodisk_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{

	g_return_if_fail(pinfo != NULL);
	
	g_message("%s [%ld.%06ld] type[%d] [%x]", __FUNCTION__,
					pinfo->timestamp.tv_sec, pinfo->timestamp.tv_usec,
					pinfo->type, pinfo->d.params[0] );

	if( pinfo->d.params[0] == 1 )
	{
		nf_eventlog_put_param( &pinfo->timestamp, LT_DISK_EVENT, 0, LP2_DISK_EVENT_NO_DISK, NULL);
	}
}


static gboolean using_usb = 0;
pthread_mutex_t using_usb_mutex;

/* USB���� �����͸� read or write�ϸ� JM39x���� SMART�� ���� ���Ѵ�.*/
void nf_set_using_usb(gboolean val)		//usb is using or not(now:usb, ftp, ODD)
{
	pthread_mutex_lock(&using_usb_mutex);
	using_usb = val;
	pthread_mutex_unlock(&using_usb_mutex);
}


static guint _UNC_key = 0;

extern gboolean nf_raid_init();	
/**
	@brief				��ũ init 
	@param[in]	type	��ȸ�� progress ����
	@param[out]	progress �����
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_filesystem_init(void)
{
	gint ret = 0, tmp_ret = 0;
	gint max_channels = 128;
	gint max_streams = 1024;
 	gulong	cb_handle;
 	
	//g_message("%s called!!", __FUNCTION__);
	g_return_val_if_fail( _fs_is_online == 0,  0);

#ifdef ENABLE_ESTG_START
	estg_start();
#endif

	ret = sst_init(max_channels, max_streams,
						_nf_sst_malloc, _nf_sst_mfree, 
						_nf_sst_mvtop, _nf_sst_mptov,
						_fsinit_event_function, NULL, 
						_CMEM_physp, _CMEM_size);

	if ( ret ) {
		g_warning("%s sst_init result[%d](%s)\n", __FUNCTION__, ret, sst_get_error_string(ret));
	}
	
	/** Disk Force Format Option!! **/
	if( 0 ) { 
		ret = disk_format();       
		if ( ret ) {
			 g_warning("%s disk_format result[%d](%s)\n", __FUNCTION__, ret, sst_get_error_string(ret)); 
		}                    
	}

	cb_handle= nf_notify_connect_cb( "disk_full", _log_put_disk_full_cb_func , (gpointer)NULL );
	g_message("%s disk_full connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "disk_overwr", _log_put_disk_overwr_cb_func , (gpointer)NULL );
	g_message("%s disk_overwr connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);		

	cb_handle= nf_notify_connect_cb( "disk_smart", _log_put_disk_smart_cb_func , (gpointer)NULL );
	g_message("%s disk_overwr connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "disk_exhaust", _log_put_disk_exhaust_cb_func , (gpointer)NULL );
	g_message("%s disk_exhaust connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "disk_nodisk", _log_put_disk_nodisk_cb_func , (gpointer)NULL );
	g_message("%s disk_nodisk connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	sleep(1);

#ifdef ENABLE_UNC_UI_RECOVERY
	{
		_UNC_key = nf_sysdb_get_uint("disk.util.chkdsk_type");
		
		if(_UNC_key) {
		
			g_message("%s UNC detected!!! key[%x]", __FUNCTION__, _UNC_key);
//			SNF_MODEL_XXX			
			ret = sst_unc_recover_set_flag ( _UNC_key);			
			g_message("%s UNC detected!!! ret[%x]", __FUNCTION__, ret);

			if( ret != 0 )
				_UNC_key = 0;
				
			nf_sysdb_set_uint("disk.util.chkdsk_type", 0);
			nf_sysdb_save("disk");
			nf_sysdb_save_flush();	
		}
	}
#endif 

#ifdef ENABLE_DISK_IO_ERROR_MON	
	_disk_io_err_cnt = nf_sysdb_get_uint("disk.extrec.E0.size");

	{
		char buff[256];
		snprintf(buff, sizeof(buff), "ERR %d", _disk_io_err_cnt);
		nf_sysdb_set_str("cam.C0.title", buff);
		nf_notify_fire_params("sysdb_change",NF_SYSDB_CATE_CAM,0,0,0);
	}
	
#endif 

#ifdef ENABLE_DVR_AVI_PLAYER
	
	tmp_ret = sst_set_board_type( DISPLAY_IS_PAL ? VIDEO_TYPE_PAL : VIDEO_TYPE_NTSC);
	g_message("%s sst_set_board_type type[%d] ret[%d]", __FUNCTION__, 
							DISPLAY_IS_PAL ? VIDEO_TYPE_PAL : VIDEO_TYPE_NTSC, tmp_ret);

	// bit mask
	// edit captainnn 2016.11.17 add h.265 codec
	tmp_ret = sst_set_codec_type( 0x00000442 /*NF_CODEC_TYPE_H264K */, 0x000000001 /*  NF_CODEC_TYPE_URAW */);
	g_message("%s sst_set_codec_type video[0x%08x] audio[0x%08x] ret[%d]", __FUNCTION__, 2, 0, tmp_ret);

#ifdef ENABLE_DVR_MUL_PLAYER
	tmp_ret = sst_mul_enable( 1 );//multi arch player enable
	g_message("%s sst_mul_enable ret[%d]", __FUNCTION__, tmp_ret);
#endif

#endif 

#if defined(ENABLE_DVR_DISK_INFO)

	if(_disk_info_timer_handle == 0)
	{
		_disk_info_timer_handle=nf_timer_add( 3*1000, 
							_disk_info_timer_cb,
							NULL );
		g_message("%s _disk_info_timer_handle[%ld]", __FUNCTION__, _disk_info_timer_handle);
	}
#endif

	nf_filesystem_set_privacy_frame();
	
#if defined(_IPX_0824P3)||defined(_IPX_1648P3)||defined(_IPX_0824P3ECO)||defined(_IPX_1648P3ECO) ||\
	defined (_IPX_0412L4) || defined(_IPX_0824P4)||defined(_IPX_1648P4)||defined(_IPX_1648P4E) ||\
	defined(_IPX_0824P4E) || defined(_IPX_32P4E)

	#ifdef ENABLE_WATCHDOG
		nf_watchdog_ctrl( NF_WATCHDOG_MEMBER_SST, NF_WATCHDOG_TIME_SST_SEC, NF_WATCHDOG_ENABLE);
		nf_watchdog_kick( NF_WATCHDOG_MEMBER_SST );	
	#endif
	
	nf_raid_init();		
	
	#ifdef ENABLE_WATCHDOG
		nf_watchdog_ctrl( NF_WATCHDOG_MEMBER_SST, NF_WATCHDOG_TIME_SST_SEC, NF_WATCHDOG_DISABLE);		
	#endif

#endif 	

	nf_raid_init();

	return ( ret == 0 ) ? 1:0;	
}

/**
	@brief				��ũ init 
	@param[in]	type	��ȸ�� progress ����
	@param[out]	progress �����
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_filesystem_init_debug( gint force_format )
{
	gint ret = 0, tmp_ret = 0;
	gint max_channels = 64;
	gint max_streams = 1024;

	//g_message("%s called!!", __FUNCTION__);
	g_return_val_if_fail( _fs_is_online == 0,  0);

#ifdef ENABLE_ESTG_START
	estg_start();
#endif

	ret = sst_init(max_channels, max_streams,
						_nf_sst_malloc, _nf_sst_mfree, 
						_nf_sst_mvtop, _nf_sst_mptov,
						_fsinit_event_function, NULL, 
						_CMEM_physp, _CMEM_size);

	if ( ret ) {
		g_warning("%s sst_init result[%d](%s)\n", __FUNCTION__, ret, sst_get_error_string(ret)); 
		return ( ret == 0 ) ? 1:0;	
	}

	
	sleep(1);
		
	g_message("%s force_format[%d]\n", __FUNCTION__, force_format);	

if( force_format ) {	
	ret = disk_format();	
	if ( ret ) {
		g_warning("%s disk_format result[%d](%s)\n", __FUNCTION__, ret, sst_get_error_string(ret)); 
		return ( ret == 0 ) ? 1:0;	
	}
}

	ret = fs_start();	
	if ( ret ) {
		g_warning("%s fs_start result[%d](%s)\n", __FUNCTION__, ret, sst_get_error_string(ret)); 
		return ( ret == 0 ) ? 1:0;	
	}

	pthread_mutex_init(&using_usb_mutex, NULL);
	
	ret = nf_arch_manager_start();
	if ( ret < 0 ) {
		g_warning("%s nf_arch_manager_start result[%d](%s)\n", __FUNCTION__, ret, sst_get_error_string(ret)); 
		return ( ret == 0 ) ? 1:0;	
	}
	
#ifdef ENABLE_DVR_AVI_PLAYER
	
	tmp_ret = sst_set_board_type( DISPLAY_IS_PAL ? VIDEO_TYPE_PAL : VIDEO_TYPE_NTSC);
	g_message("%s sst_set_board_type type[%d] ret[%d]", __FUNCTION__, 
							DISPLAY_IS_PAL ? VIDEO_TYPE_PAL : VIDEO_TYPE_NTSC, tmp_ret);

	// bit mask
	tmp_ret = sst_set_codec_type( 0x000000042 /*NF_CODEC_TYPE_H264K */, 0x000000001 /*  NF_CODEC_TYPE_URAW */);
	g_message("%s sst_set_codec_type video[0x%08x] audio[0x%08x] ret[%d]", __FUNCTION__, 2, 0, tmp_ret);

#ifdef ENABLE_DVR_MUL_PLAYER
	tmp_ret = sst_mul_enable( 1 );//multi arch player enable
	g_message("%s sst_mul_enable ret[%d]", __FUNCTION__, tmp_ret);
#endif
#endif 
	
	return ( ret == 0 ) ? 1:0;	
}


/**
	@brief				��ũ exit 
	@param[in]	type	��ȸ�� progress ����
	@param[out]	progress �����
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_filesystem_exit(void)
{
	gint result = 0;
	//g_message("%s called!!", __FUNCTION__);	
	g_return_val_if_fail( _fs_is_online == 0, 0);
		
	result =  sst_exit();
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 
	}
		
	return ( result == 0 ) ? 1:0;	
}

/**
	@brief				filesystem �� �¶��� �ΰ�?
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_filesystem_is_online(void)
{			
	return 	_fs_is_online;	
}

/**
	@brief				��ũ progress ��ȸ
	@param[in]	type	��ȸ�� progress ����
	@param[out]	progress �����
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_filesystem_get_progress(NF_SST_PRGT_E type, NF_SST_PROGRESS  *progress)
{
	//g_message("%s called!! type[%d]", __FUNCTION__, type);				
#ifdef ENABLE_WATCHDOG
	if( type == NF_SST_PRGT_RTLIMIT || 1 ) 
	{
		static guint prev_curr = 0;		
		int ret;
		
		ret = sst_get_progress(type, (struct sst_progress_t *)progress);	
		if( ret == 0 )
		{
			g_message("%s progress[%d][%d]", __FUNCTION__, type, progress->current);

			// FIXME 4TB+ format issue choissi 2014-11-29 ���� 4:46:19
			if( progress->total < progress->current)
			{
				progress->current = progress->total;
				g_message("%s FIXME!! progress[%d][%d]", __FUNCTION__, type, progress->current);
			}
			if( prev_curr != progress->current )
			{
				nf_watchdog_kick( NF_WATCHDOG_MEMBER_SST );	
				prev_curr = progress->current;
			}
		}				
		return  (ret == 0) ? 1:0;
	}
#endif
			
	return ( sst_get_progress(type, (struct sst_progress_t *)progress) == 0 ) ? 1 : 0;				
}


/**
	@brief				���� �ý��� ���� & �̺�Ʈ ���� �ڵ鷯 ���
	@param[in]	format_flag	���� �㰡 �÷���
	@param[in]	cb_event	�̺�Ʈ �ݹ� �Լ�
	@param[in]	cb_ctx		�̺�Ʈ �ݹ� context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
	
	nf_disk_get_info�� ���� ��ũ ������ ��ȸ �� �Ŀ� fs start�� ���� �����ؾ� �Ѵ�.
	�űԷ� �߰��� disk�� ���ؼ� format�� ���� ������ ���� ���ε� �ʿ�.
	nf_filesystem_start�� ȣ��Ǹ� ���� �ý����� ��Ŀ����, ���˵��� �����Ѵ�.
	�̶� ���� ��Ȳ(ū ����)�� cb_event�� ���� �ǰ� �� ���� ������
	���� api(nf_filesystem_get_progress)�� ����ؼ� ���� �Ѵ�.
*/
gboolean nf_filesystem_start( gint format_flag, cb_event_fxn_t cb_event, gpointer cb_ctx, GError **error )
{
	gint result = 0;
		
	g_message("%s format[%d] called!!", __FUNCTION__, format_flag);
			
	g_return_val_if_fail( _fs_is_online == 0, 0);

#ifdef HOT_SWAP_UNSUPPORTED
	got_disk_info = -1;
#endif

	pthread_mutex_init(&using_usb_mutex, NULL);
	
	result = nf_arch_manager_start();
	if ( result < 0 ) {
		g_warning("%s nf_arch_manager_start failed[%d]\n", __FUNCTION__, result);

		g_set_error( error, NF_API_DISK_ERROR, result, nf_arch_get_error_string(result) );
		return ( result == 0 ) ? 1:0;
	}
	
#ifdef ENABLE_WATCHDOG
	nf_watchdog_ctrl( NF_WATCHDOG_MEMBER_SST, NF_WATCHDOG_TIME_SST_SEC, NF_WATCHDOG_ENABLE);
	nf_watchdog_kick( NF_WATCHDOG_MEMBER_SST );	
#endif

	_start_cb_event = cb_event;
	_start_cb_ctx   = cb_ctx;

	if(	cb_event)
		result = sst_fs_start(format_flag, 1, _fsstart_event_function , NULL);
	else
		result = sst_fs_start(format_flag, 0, NULL, NULL);
				
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 		

		g_set_error( error, NF_API_DISK_ERROR, result, sst_get_error_string(result) );
		nf_arch_manager_stop();
		
#ifdef ENABLE_WATCHDOG
		nf_watchdog_ctrl( NF_WATCHDOG_MEMBER_SST, NF_WATCHDOG_TIME_SST_SEC, NF_WATCHDOG_DISABLE);		
#endif
		
	}
	
	if( cb_event == NULL )	// block mode
	{
		_fsstart_complete();	
	}
				
	return ( result == 0 ) ? 1:0;	
}

/**
	@brief				���� �ý��� ����	
	@param[in]	cb_event	�̺�Ʈ �ݹ� �Լ�
	@param[in]	cb_ctx		�̺�Ʈ �ݹ� context	
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_filesystem_stop( cb_event_fxn_t cb_event , gpointer cb_ctx, GError **error )
{
	gint result = 0;
	g_message("%s called!!", __FUNCTION__);

	g_return_val_if_fail( _fs_is_online == 1, 0);
	
	result = nf_arch_manager_stop();
	if ( result < 0 ) {
		g_warning("%s nf_arch_manager_stop failed[%d]\n", __FUNCTION__, result);
		
		g_set_error( error, NF_API_DISK_ERROR, result, nf_arch_get_error_string(result) );
		return ( result == 0 ) ? 1:0;
	}
	
	g_message("%s center1 called!![%p]", __FUNCTION__, cb_event );
	
#ifdef ENABLE_WATCHDOG
	nf_watchdog_ctrl( NF_WATCHDOG_MEMBER_SST, NF_WATCHDOG_TIME_SST_SEC, NF_WATCHDOG_ENABLE);
	nf_watchdog_kick( NF_WATCHDOG_MEMBER_SST );	
#endif

	_stop_cb_event = cb_event;
	_stop_cb_ctx   = cb_ctx;
	
	g_message("%s center2 called!![%p]", __FUNCTION__, cb_event );
	
	if(	cb_event )
		result = sst_fs_stop(1, _fsstop_event_function , NULL);
	else
		result = sst_fs_stop(0, NULL, NULL);
		
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 
		g_set_error( error, NF_API_DISK_ERROR, result, sst_get_error_string(result) );		

#ifdef ENABLE_WATCHDOG
		nf_watchdog_ctrl( NF_WATCHDOG_MEMBER_SST, NF_WATCHDOG_TIME_SST_SEC, NF_WATCHDOG_DISABLE);		
#endif
		
	}

	if( cb_event == NULL )	// block mode
	{
		_fsstop_complete();
	}
	
	return ( result == 0 ) ? 1:0;	
}


/**
	@brief				���丮���� ����� ������ �ð�
	@param[out]	*time	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_filesystem_get_lasttime( GTimeVal *ltime  )
{
	guint64		t_last = 0LL;	
	gint result = 0;
		
	g_message("%s called!!", __FUNCTION__);
	g_return_val_if_fail( ltime, 0);
	
	//int sst_fs_get_lasttime(u64 *t_last); 
	result = sst_fs_get_lasttime( &t_last ); 
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 
		
	}else{
		GUINT64_TO_GTIMEVAL(t_last, *ltime);
	}
		
	return ( result == 0 ) ? 1:0;	
}

/**
	@brief				��ũ ���� ��� ����					
	@param[in]	w_mode	��ũ ���� ��� 
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_filesystem_change_wmode( NF_DISK_WRITE_MODE_E w_mode, gint restart_wo, GError **error )
{	
	gint result = 0;
	g_message("%s mode[%d] wo[%d] called!!", __FUNCTION__, w_mode, restart_wo);	

	//int sst_fs_set_wmode(u8 w_mode, u8 restart_wo); 
	result = sst_fs_set_wmode((unsigned char)w_mode, (unsigned char)restart_wo); 
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 
		g_set_error( error, NF_API_DISK_ERROR, result, sst_get_error_string(result) );	
	} else{
		_disk_write_mode = w_mode;
	}

		
	return ( result == 0 ) ? 1:0;	
}

/**
	@brief				��ũ ���� ��� ���
	@param[out]	w_mode	��ũ ���� ���
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_filesystem_get_wmode( NF_DISK_WRITE_MODE_E *w_mode, GError **error )
{
	guchar mode = 0;
	gint result = 0;
	
	g_return_val_if_fail( w_mode, 0 );
	
	//int sst_fs_get_wmode(u8 *w_mode); 
	result = sst_fs_get_wmode( &mode ) ;
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result));
		g_set_error( error, NF_API_DISK_ERROR, result, sst_get_error_string(result) );
	}else{
		*w_mode = mode;	
		g_message("%s w_mode[%d]", __FUNCTION__, mode);
		_disk_write_mode = mode;
		
	}		
	return ( result == 0 ) ? 1:0;	
}

gboolean nf_filesystem_set_rtlimit_runtime( guint rtlimit)
{
	gint result = 0;

	g_message("%s rtlimit[%d] called!!", __FUNCTION__, rtlimit);
	result = sst_fs_set_rtlimit_runtime(rtlimit);
			
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 
	}	
	
	return ( result == 0 ) ? 1:0;	
}

/*
int sst_fs_get_rtlimit(u32 *rtlimit, u8 *d_mode);
int sst_fs_set_rtlimit(u32 rtlimit, u8 d_mode); 
*/

/**
	@brief				���ڵ� �ð� ���� ����	
	@param[in]	rtlimit	
	@param[in]	d_mode	
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_filesystem_set_rtlimit( guint rtlimit, guchar d_mode, cb_event_fxn_t cb_event, gpointer cb_ctx, GError **error )
{
	gint result = 0;

	g_message("%s rtlimit[%d] mode[%d] called!!", __FUNCTION__, rtlimit, d_mode);

#ifdef HOT_SWAP_UNSUPPORTED
	got_disk_info = -1;
#endif

#ifdef ENABLE_WATCHDOG
	nf_watchdog_ctrl( NF_WATCHDOG_MEMBER_SST, NF_WATCHDOG_TIME_SST_SEC*10, NF_WATCHDOG_ENABLE);
	nf_watchdog_kick( NF_WATCHDOG_MEMBER_SST );	
#endif

	_rtl_cb_event = cb_event;
	_rtl_cb_ctx   = cb_ctx;

	if(	cb_event )
		result = sst_fs_set_rtlimit(rtlimit, d_mode, 1, _fsrtl_event_function , cb_ctx); 
	else 
		result = sst_fs_set_rtlimit(rtlimit, d_mode, 0, NULL, NULL); 
			
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 
		g_set_error( error, NF_API_DISK_ERROR, result, sst_get_error_string(result) );	
#ifdef ENABLE_WATCHDOG
		nf_watchdog_ctrl( NF_WATCHDOG_MEMBER_SST, NF_WATCHDOG_TIME_SST_SEC, NF_WATCHDOG_DISABLE);		
#endif		
	}	

	if( cb_event == NULL )	// block mode
	{
		_fsrtl_complete();		// disable watchdog
	}		
	
	return ( result == 0 ) ? 1:0;	
}
/**
	@brief				���ڵ� �ð� ���� ���	
	@param[in]	rtlimit	
	@param[in]	d_mode	
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_filesystem_get_rtlimit( guint *rtlimit, guchar *d_mode, GError **error )
{
	gint result = 0;

	g_message("%s called!!", __FUNCTION__);

	g_return_val_if_fail( rtlimit, 0);
	g_return_val_if_fail( d_mode, 0);

	result = sst_fs_get_rtlimit(rtlimit, d_mode); 
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 
		g_set_error( error, NF_API_DISK_ERROR, result, sst_get_error_string(result) );	
	}	
				
	return ( result == 0 ) ? 1:0;	
}

	
/**
	@brief				���ڵ� ������ ����
	@param[in]	utc_time_sec
	@param[in]	cb_event	�̺�Ʈ �ݹ� �Լ�
	@param[in]	cb_ctx		�̺�Ʈ �ݹ� context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_filesystem_delete_data( guint utc_time_sec,
									 cb_event_fxn_t cb_event, gpointer cb_ctx, 
									 GError **error )
{
	gint result = 0;
	g_message("%s utc[%d] called!! ", __FUNCTION__, utc_time_sec);	

#ifdef ENABLE_WATCHDOG
	nf_watchdog_ctrl( NF_WATCHDOG_MEMBER_SST, NF_WATCHDOG_TIME_SST_SEC*10, NF_WATCHDOG_ENABLE);
	nf_watchdog_kick( NF_WATCHDOG_MEMBER_SST );	
#endif
	
	_delete_cb_event = cb_event;
	_delete_cb_ctx = cb_ctx;

	if(	cb_event )		
		result = sst_fs_delete_data(utc_time_sec, cb_event ? 1:0 , _fsdel_event_function, cb_ctx); 	
	else
		result = sst_fs_delete_data(utc_time_sec, 0 , NULL, NULL); 	
			
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 
		g_set_error( error, NF_API_DISK_ERROR, result, sst_get_error_string(result) );	
#ifdef ENABLE_WATCHDOG
		nf_watchdog_ctrl( NF_WATCHDOG_MEMBER_SST, NF_WATCHDOG_TIME_SST_SEC, NF_WATCHDOG_DISABLE);
#endif	
	}else{

#ifdef ENABLE_PRIVACY_DELETE
		{
			int i, is_flush = 0;
			char start_buff[128],end_buff[128];			
			guint start_time, end_time;
			
			guint seq = nf_sysdb_get_uint("disk.erase_section.seq");		
			for(i=0;i<seq && i<NF_DISK_PRIVACY_INFO_MAX_CNT;++i)
			{			
				snprintf(start_buff, sizeof(start_buff), "disk.erase_section.E%d.from", i);
				snprintf(end_buff, sizeof(end_buff), "disk.erase_section.E%d.to", i);
				
				start_time = nf_sysdb_get_uint(start_buff);
				end_time = nf_sysdb_get_uint(end_buff);
				
				g_message("%s idx[%d] [%u]~[%u]",__FUNCTION__, i, start_time, end_time);				
				if( start_time > utc_time_sec )
				{
					nf_sysdb_set_uint(start_buff, 0);
					nf_sysdb_set_uint(end_buff, 0);
					is_flush =1;
					g_message("%s -> 0~0",__FUNCTION__);
				}
	
				if( start_time < utc_time_sec && end_time > utc_time_sec)
				{				
					nf_sysdb_set_uint(end_buff, utc_time_sec);
					is_flush =1;
					g_message("%s -> [%u]~[%u]",__FUNCTION__, start_time, utc_time_sec);
				}
				
			}
			
			if(is_flush)
			{
				nf_sysdb_save("disk");
				nf_sysdb_save_flush();
			}
		}
#endif
	}	

	if(	cb_event == NULL)	// disable watchdog
	{
		_delete_complete();
	}
	
	return ( result == 0 ) ? 1:0;	
}

gboolean nf_filesystem_set_privacy_frame(void)
{
	FILE *fp = NULL;
	void *buf = NULL;
	gchar *frame_path = "/NFDVR/data/gui/frame/640x360.h264";

	guint frame_size = 0;
	gint result = 0;

	fp = fopen(frame_path, "r");

	if (fp == NULL)
	{
		g_warning("%s error no file(%s)\n", __FUNCTION__, frame_path);
		return 0;
	}
	else 
	{
		fseek(fp, 0, SEEK_END);
		frame_size = ftell(fp);
		fseek(fp, 0, SEEK_SET);
	}

	buf = (gchar *) g_malloc( frame_size );
	g_assert( buf != NULL );

	fread( buf, 1, frame_size, fp );

	g_message("%s Frame[%p] SZ[%u]", __FUNCTION__, buf, frame_size);

//	HexDump( buf, 16, 0);

	result = sst_set_privacy_frame( buf, frame_size );	

	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 
	}

	g_free( buf );
	fclose(fp);

	return ( result == 0 ) ? 1:0;;	
}

/**
	@brief		���ڵ� ������ ���� ����
	@param[in]	ch_mask		���� �� ä�ε��� ��Ʈ ����ũ
	@param[in]	*time_info	���� �� ������ ����, ���� �ð�
	@param[in]	cb_event		�̺�Ʈ �ݹ� �Լ�
	@param[in]	cb_ctx		�̺�Ʈ �ݹ� context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_filesystem_check_privacy_section( guint ch_mask,  NF_PRIVACY_TIME_INFO *time_info )
{
	gint result = 0;
	GTimeVal tval;
 	g_return_val_if_fail( ch_mask > 0, 0 );

	g_return_val_if_fail( time_info != NULL, 0 );
	g_return_val_if_fail( time_info->start_time < time_info->end_time, 0 );

	g_get_current_time( &tval );
	
	g_message("%s ch_mask[%x] Intervals[%u ~ %u] curr[%u] called!! ", __FUNCTION__, 
					ch_mask, 
					time_info->start_time, 
					time_info->end_time, 
					tval.tv_sec);	


	g_return_val_if_fail( time_info->start_time < tval.tv_sec, 0 );	
	
	if( time_info->end_time > tval.tv_sec ) 
		time_info->end_time = tval.tv_sec;
	
	g_return_val_if_fail( time_info->start_time < time_info->end_time, 0 );	
	ch_mask |= (ch_mask << 16) & 0xFFFF0000;

{
	int cnt, ret, i;
	NF_ARCH_AVI_INFO *pInfo = NULL;
	
	cnt  = nf_arch_list_get_count( NF_ARCH_TYPE_AVI, NULL, NULL);
	g_message("%s nf_arch_list_get_count[%d]", __FUNCTION__, cnt);		
	
	if( cnt == -10 /* ARCH_ERR_LISTEMPTY */  || cnt == 0 ) return 1;				
	if( cnt < 0) return 0;
				
	pInfo = calloc(1, sizeof(NF_ARCH_AVI_INFO) * (cnt+1) );
	if( !pInfo ) 
	{
		g_warning("%s malloc failed [%d]", __FUNCTION__, sizeof(NF_ARCH_AVI_INFO) * (cnt+1) );
		return 0;
	}
					
	ret = nf_arch_list_get_avi ( NF_ARCH_DIR_FORWARD, 0, cnt, pInfo, NULL, NULL);						
	g_message("%s nf_arch_list_get_avi ret[%d]", __FUNCTION__, ret);	
	if( ret == 0) {		
		free(pInfo);
		return 0;
	}

/*
case 1)
                    +-----------+
    +---------------|------+    |
    |   reserved  da|ta    |    |	

case 2)
   +----------+
   |   +------|---------------+
   |   |   res|erved  data    |
	
case 3) 
  +---------------------------+
  |  +--------------------+   |
  |  |   reserved  da|ta  |   |	
*/
		
	for(i=0;i<cnt;++i)
	{	
		GTimeVal tv_beg, tv_end;				

		GUINT64_TO_GTIMEVAL( pInfo[i].time_beg, tv_beg);
		GUINT64_TO_GTIMEVAL( pInfo[i].time_end, tv_end);						

		g_message("[%4d] [%d]~[%d] [%s]", 
				pInfo[i].arch_id, tv_beg.tv_sec, tv_end.tv_sec, pInfo[i].tag );								

		if( (time_info->start_time >= tv_beg.tv_sec && 
				time_info->start_time <= tv_end.tv_sec ) 
			|| (time_info->end_time >= tv_beg.tv_sec && 
					time_info->end_time <= tv_end.tv_sec )					
			|| (time_info->start_time <= tv_beg.tv_sec && 
					time_info->end_time >= tv_end.tv_sec )  )
		{
			g_warning("%s Confilct Reserved DATA -> arch_id[%d]", __FUNCTION__, pInfo[i].arch_id);
			free(pInfo);
			return 0;		
		}
		
	}		
	free(pInfo);
}	
					
					
	return 1;	
}

/**
	@brief		���ڵ� ������ ���� ����
	@param[in]	ch_mask		���� �� ä�ε��� ��Ʈ ����ũ
	@param[in]	*time_info	���� �� ������ ����, ���� �ð�
	@param[in]	cb_event		�̺�Ʈ �ݹ� �Լ�
	@param[in]	cb_ctx		�̺�Ʈ �ݹ� context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_filesystem_set_privacy_section( guint ch_mask,  NF_PRIVACY_TIME_INFO *time_info,
									 cb_event_fxn_t cb_event, gpointer cb_ctx, 
									 GError **error )
{
	gint result = 0;
	GTimeVal tval;
 	g_return_val_if_fail( ch_mask > 0, 0 );

	g_return_val_if_fail( time_info != NULL, 0 );
	g_return_val_if_fail( time_info->start_time < time_info->end_time, 0 );

	g_get_current_time( &tval );
	
	g_message("%s ch_mask[%x] Intervals[%u ~ %u] curr[%u] called!! ", __FUNCTION__, 
					ch_mask, 
					time_info->start_time, 
					time_info->end_time, 
					tval.tv_sec);	
		
	g_return_val_if_fail( time_info->start_time < tval.tv_sec, 0 );	
	
	if( time_info->end_time > tval.tv_sec ) 
		time_info->end_time = tval.tv_sec;
	
	g_return_val_if_fail( time_info->start_time < time_info->end_time, 0 );
	
	ch_mask |= (ch_mask << 16) & 0xFFFF0000;
	
#ifdef ENABLE_WATCHDOG
//	nf_watchdog_ctrl( NF_WATCHDOG_MEMBER_SST, NF_WATCHDOG_TIME_SST_SEC*10, NF_WATCHDOG_ENABLE);
//	nf_watchdog_kick( NF_WATCHDOG_MEMBER_SST );	
#endif
	
	_privacy_cb_event = cb_event;
	_privacy_cb_ctx = cb_ctx;

	if(	cb_event )		
		result = sst_set_privacy_section(ch_mask, time_info, cb_event ? 1:0 , _fsprivacy_event_function, cb_ctx); 	
	else
		result = sst_set_privacy_section(ch_mask, time_info, 0 , NULL, NULL); 	
			
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 
		g_set_error( error, NF_API_DISK_ERROR, result, sst_get_error_string(result) );	
#ifdef ENABLE_WATCHDOG
//		nf_watchdog_ctrl( NF_WATCHDOG_MEMBER_SST, NF_WATCHDOG_TIME_SST_SEC, NF_WATCHDOG_DISABLE);
#endif	
		_privacy_ch_mask = 0;
	}else{
		
#ifdef ENABLE_PRIVACY_DELETE		
		_privacy_start_time = time_info->start_time;
		_privacy_end_time = time_info->end_time;
		_privacy_ch_mask = ch_mask;
#endif

	}
	
	if(	cb_event == NULL)	// disable watchdog
	{
		_privacy_complete();
	}

	return ( result == 0 ) ? 1:0;	
}


#define ITX_LED_HDD1        41
#define ITX_LED_HDD2        42
#define ITX_LED_HDD3        43
#define ITX_LED_HDD4        44
#define ITX_LED_HDD5        45
#define ITX_LED_ESATA      46
#define INTERNAL_LED_MAX 5

gboolean nf_disk_HDD_LED_ON(void)
{
	gint phys = 0, i = 0;

	for ( i = 0 ; i < INTERNAL_LED_MAX ; i ++ )
	{
		phys = st_disk_info.ucPortNo[0][i];
		if ( phys == 0xff ) continue;
		if ( st_disk_info.disk_size[0][i] == 0 ) continue;

		switch (phys)
		{
			case 0: nf_dev_keypad_led_on(ITX_LED_HDD1);
				break;
			case 1: nf_dev_keypad_led_on(ITX_LED_HDD2);
				break;
			case 2: nf_dev_keypad_led_on(ITX_LED_HDD3);
				break;
			case 3: nf_dev_keypad_led_on(ITX_LED_HDD4);
				break;
			case 4: nf_dev_keypad_led_on(ITX_LED_HDD5);
				break;
		}
	}
	
	for ( i = 0 ; i < 16 ; i ++ )
	{
		if ( st_disk_info.disk_size[1][i] )
		{
			nf_dev_keypad_led_on(ITX_LED_ESATA);
			break;
		}
	}
}

/**
	@brief				��ũ ���� ���
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_disk_get_info( NF_DISK_INFO *disk_info, GError **error )
{
	gint result = 0;
	NF_DISK_INFO tmp_disk_info;
	g_message("%s called!! [%d]", __FUNCTION__, got_disk_info);
	
	g_return_val_if_fail( disk_info, 0);

#ifdef HOT_SWAP_UNSUPPORTED
	// hot-swap is not supported.
	if ( got_disk_info != -1 )
	{
		if ( got_disk_info == -8 )//-8 : nodisk
			return 0;
		
		memcpy(disk_info, &st_disk_info, sizeof(NF_DISK_INFO));	

		// 3s * 20, 1분에 한번씩 disk 상태를 체크 한다.
		if (removed_disk_check_count == 20) {
			result = sst_disk_get_info( (struct disk_info_t *)&tmp_disk_info);
			g_message("%s removed_disk_check_count = %d, result = %d\n", __FUNCTION__, removed_disk_check_count, result);
			if (result == -8) {
				removed_disk_error_count++;
			} else {
				removed_disk_error_count = 0;
			}
			removed_disk_check_count = 0;
		}

		if (removed_disk_error_count == 3) {
			removed_disk_error_count = 0;
			g_message("%s Remove disk detect..!!\n");
			//nf_dev_board_reset()으로 동작을 하지 않아 reboot 커맨드를 사용 한다.
			proxy_system("reboot", 1, 100);
			// nf_dev_board_reset();
		}

		removed_disk_check_count++;

		return 1;
	}
#endif

	result = sst_disk_get_info( (struct disk_info_t *)disk_info) ;
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 
		g_set_error( error, NF_API_DISK_ERROR, result, sst_get_error_string(result) );
	}else{

#if defined(_IPX_1648VE3) || defined(_IPX_0824VE3) || defined(_IPX_0412VE3) || defined(_IPX_1648M4) || defined(_IPX_1648M4E) || defined(_IPX_32M4E)
			// FIXME VE3 only phy port remap
			int i;					
			for (i=0; i< disk_info->group_disks[0]; ++i)
			{
				disk_info->ucPortNo[0][i]=i;
			}		
#endif

	}
	
#ifdef HOT_SWAP_UNSUPPORTED
	got_disk_info = result;
#endif

	// update raid member disk info
	{
		CONTROLLER aCTRL;	
		int ret, j, idx=0;		
		memset( &aCTRL, 0x00, sizeof(aCTRL));

		ret = nf_raid_get_ctrl_info(nf_raid_get_ctrl_id(NF_RAID_CTRL_ID_INTERNAL), &aCTRL, NULL);
		if(ret && aCTRL.Raid[0].RaidMemberCount>1 ) 
		{
				
			for(j=0; j<MAX_DISK_IN_CONTROLLER; ++j)
			{	

				g_message("%s Raid Sata Port[%d] cap[%d] [%s][%s]", __FUNCTION__, j, 
							aCTRL.Sata[j].Capacity,
							aCTRL.Sata[j].ModelName,
							aCTRL.Sata[j].SerialNumber);
														
				if( aCTRL.Sata[j].Capacity <= 1){					
					continue;
				}
									
				if( idx == 0 ){
					strncpy( disk_info->sSerialNo[0][idx], aCTRL.Sata[j].SerialNumber, 22);
					disk_info->ucPortNo[0][idx] = j;
				}else{					
					disk_info->disk_state[0][idx] += NF_DISK_INFO_FLAG_MIRROR;
					// strncpy( disk_info->model_num[0][idx], aCTRL.Sata[j].ModelName, 32);
					
					// unit size 32MB -> kb size
					disk_info->disk_size[0][idx] = (guint64) aCTRL.Sata[j].Capacity * 32 * 1024;		
					strncpy( disk_info->sSerialNo[0][idx], aCTRL.Sata[j].SerialNumber, 22);
					disk_info->ucPortNo[0][idx] = j;													
				}
				++idx;
			}									
		}
	}

	memcpy(&st_disk_info, disk_info, sizeof(NF_DISK_INFO));

	nf_disk_HDD_LED_ON();

#ifdef	DEBUG_API_DISK_LOG
	//HexDump( disk_info, sizeof(NF_DISK_INFO), 0);
#endif 

	return ( result == 0 ) ? 1:0;	
}


/**
	@brief				��ũ �����ϱ�                         	
	@param[in]	mode	��ũ ���˸�� 0:sync  1:format
	@param[in]	disk_id	�ش� �׷��� ��ũ ��ȣ : 0~15
	@param[in]	cb_event	�̺�Ʈ �ݹ� �Լ�
	@param[in]	cb_ctx		�̺�Ʈ �ݹ� context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_disk_format( gint mode, gint disk_id, cb_event_fxn_t cb_event, gpointer cb_ctx, GError **error)
{
	gint result = 0;
	
	g_message("%s mode[%d] disk[%d] called!!", __FUNCTION__, mode, disk_id );

#if 1 // choissi 2010-08-05 ���� 3:13:17 diskmgr	
	result = sst_disk_format(disk_id, mode, cb_event ? 1:0 , cb_event, cb_ctx);						
#else 
	result = sst_disk_format(disk_id, 0, cb_event ? 1:0 , cb_event, cb_ctx);						
#endif 		
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 
		g_set_error( error, NF_API_DISK_ERROR, result, sst_get_error_string(result) );	
	}else{


#ifdef ENABLE_PRIVACY_DELETE
		{
			int i;
			char buff[128];
			
			nf_sysdb_set_uint("disk.erase_section.seq", 0);
			for(i=0;i<NF_DISK_PRIVACY_INFO_MAX_CNT;++i)
			{							
				snprintf(buff, sizeof(buff), "disk.erase_section.E%d.chmask", i);
				nf_sysdb_set_uint(buff, 0);	
				snprintf(buff, sizeof(buff), "disk.erase_section.E%d.from", i);
				nf_sysdb_set_uint(buff, 0);
				snprintf(buff, sizeof(buff), "disk.erase_section.E%d.to", i);
				nf_sysdb_set_uint(buff, 0);	
			}
					
			nf_sysdb_save("disk");
			nf_sysdb_save_flush();
		}
#endif

	
	}
			
	return ( result == 0 ) ? 1:0;	
}

/**
	@brief				��ũ �˻� ���� - ��ũ �׷������ ����
	@param[in]	grp_id	��ũ �׷� ���̵� 0:������ġ 1:�ܺ���ġ
	@param[in]	cb_event	�̺�Ʈ �ݹ� �Լ�
	@param[in]	cb_ctx		�̺�Ʈ �ݹ� context	
	@param[out]	error	return location for a #GError, or %NULL		
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_disk_check_start(gint grp_id, cb_event_fxn_t cb_event, gpointer cb_ctx, GError **error)
{
	gint result = 0;
	
	g_message("%s grp[%d] called!!", __FUNCTION__, grp_id);

	g_return_val_if_fail( _fs_is_online == 0, 0);

	result = sst_disk_check_start(grp_id, cb_event ? 1:0 , cb_event, cb_ctx); 		
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 
		g_set_error( error, NF_API_DISK_ERROR, result, sst_get_error_string(result) );
	}
			
	return ( result == 0 ) ? 1:0;	
}

/**
	@brief				��ũ �˻� ����
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_disk_check_stop( void )
{
	gint result = 0;
	
	g_message("%s called!!", __FUNCTION__);
	
	g_return_val_if_fail( _fs_is_online == 0, 0);
	
	result = sst_disk_check_stop(); 
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 
	}
			
	return ( result == 0 ) ? 1:0;		
}

/**
	@brief				��ũ �� ���
	@param[in]	grp_id	��ũ �׷� ���̵� 0:������ġ 1:�ܺ���ġ 0xff:��� ��ũ
	@param[in]	disk_id	�ش� �׷��� ��ũ ��ȣ : 0~15
	@param[in]	req_count 	��û�� ����	
	@param[out]	*elm_size	�� ������ ������ (����:kbytes)	
	@param[out]	*view_data  diskview ��ȯ��
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_disk_get_view( gint grp_id, gint disk_id, guint req_count, gint *elm_size, gchar *view_disk, GError **error )
{
	gint result = 0;
	//g_message("%s called!!", __FUNCTION__);

	g_return_val_if_fail( grp_id >=0 && grp_id < 2, 0);
	g_return_val_if_fail( disk_id >=0 && disk_id < 16, 0);	
	g_return_val_if_fail(st_disk_info.disk_state[grp_id][disk_id] & 0x01 != 0, 0);
	
	result = sst_fs_get_diskview((guchar)disk_id, (guint)req_count, elm_size, view_disk); 
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 
		g_set_error( error, NF_API_DISK_ERROR, result, sst_get_error_string(result) );	
	}
			
	return ( result == 0 ) ? 1:0;	
}

/**
	@brief				�ϵ� ��ũ ��뷮 ���
	@param[in]	grp_id	��ũ �׷� ���̵� 0:������ġ 1:�ܺ���ġ 0xff:��� ��ũ
	@param[in]	disk_id	�ش� �׷��� ��ũ ��ȣ : 0~15
	@param[out]	used	����� �뷮 (����:MB)
	@param[out]	total	��ü �뷮 (����:MB)
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_disk_get_usage( gint grp_id, gint disk_id, guint *used, guint *total, GError **error )
{
	gint result = 0;
	//g_message("%s called!!", __FUNCTION__);

	g_return_val_if_fail( grp_id >=0 && grp_id < 2, 0);
	g_return_val_if_fail( disk_id >=0 && disk_id < 16, 0);	
//	g_return_val_if_fail(st_disk_info.disk_state[grp_id][disk_id] & 0x01 != 0, 0);
	g_return_val_if_fail( used, 0);
	g_return_val_if_fail( total, 0);
	
	g_return_val_if_fail( nf_filesystem_is_online() , 0);
	
#if 1 // choissinf 2008-11-14 ���� 3:26:53 api change
	result = sst_fs_get_diskusage(disk_id, used, total, 2);
#else
	result = sst_fs_get_diskusage(disk_id, used, total);
#endif	
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 
		g_set_error( error, NF_API_DISK_ERROR, result, sst_get_error_string(result) );	
	}
			
	return ( result == 0 ) ? 1:0;	
}


/**
	@brief				��ũ ���� ��� ( is_full )
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_disk_is_full( GError **error )
{
	gint result = 0;
	gint status = 0;
	
	g_return_val_if_fail( nf_filesystem_is_online() , 0);
	//g_message("%s called!!", __FUNCTION__);
	
	result = sst_check_disk_full( &status ) ;
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 
		g_set_error( error, NF_API_DISK_ERROR, result, sst_get_error_string(result) );
		
		return 0;
	}

	return status ? 1:0;
}

/**
	@brief				��ũ �� ���ڵ� min,max ���
	@param[in]	hide	������ ���� ( timeline hide�� ����)	
	@param[in]	grp_id	disk group_id
	@param[in]	disk_id	disk_id
	@param[out]	data	��ȸ ���
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_disk_rec_disk_time( gint grp_id, gint disk_id, gint hide, NF_DISK_REC_DISK_TIME *data, GError **error)
{
	struct   timeusage time_data;
	int i;
	guint write_mode, rtime_limit;
	gint result = 0;
	GTimeVal tval;

	g_message("%s called!! grp_id(%d), disk_id(%d)", __FUNCTION__, grp_id, disk_id);

	g_return_val_if_fail(grp_id >=0 && grp_id < 2, 0);
	g_return_val_if_fail(disk_id >=0 && disk_id < 16, 0);	
	g_return_val_if_fail(st_disk_info.disk_state[grp_id][disk_id] & 0x01 != 0, 0);
	g_return_val_if_fail (hide == 0 || hide == 1, 0);
	g_return_val_if_fail (data != NULL, 0);
	g_return_val_if_fail(nf_filesystem_is_online(), 0);
	
	memset(&time_data, 0, sizeof(struct timeusage));
	result = sst_fs_get_disk_time((grp_id*16) + disk_id, &time_data );

	memcpy(data->ucPortNo, st_disk_info.ucPortNo, sizeof(data->ucPortNo));

	if (result == 0)
	{
		data->min_time = 0xFFFFFFFF;
		data->max_time = 0;
		data->used_mbcnt = 0;

		time_data.cnt = 1;
        for(i = 0; i < time_data.cnt; i++)
        {
			g_message("min=%x, max=%d, mbcnt=%d, | min=%d, max=%d, MBcnt=%d", data->min_time, data->max_time, data->used_mbcnt,
						time_data.entry[i].uStartTime, time_data.entry[i].uEndTime, time_data.entry[i].uMBCount);
	
		 	if ( data->min_time > time_data.entry[i].uStartTime )
		 	{
				data->min_time = time_data.entry[i].uStartTime;
		 	}
			if ( data->max_time < time_data.entry[i].uEndTime )
			{
				data->max_time = time_data.entry[i].uEndTime;
			}
			data->used_mbcnt += time_data.entry[i].uMBCount;
        }
	}
	else
	{
		g_warning("%s max result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 
		g_set_error(error, NF_API_DISK_ERROR, result, sst_get_error_string(result) );
		return 0;
	}

	g_message( "min=%d, max=%d, mbcnt=%d cnt=%d", data->min_time, data->max_time, data->used_mbcnt, time_data.cnt);

	/* hide mode */
	write_mode = nf_sysdb_get_uint("disk.write_mode");
	rtime_limit = nf_sysdb_get_uint("disk.rtime_limit");
	
	if (hide == 1 && write_mode == 1 && rtime_limit != 0)
	{
		gettimeofday((struct timeval *)&tval, NULL);

		g_message( "min=%d, max=%d, now=%ld", data->min_time, data->max_time, tval.tv_sec);
		if (tval.tv_sec - (glong)rtime_limit > (glong)data->max_time)
		{
			data->max_time = 0;
			data->min_time = 0;
			return 1;
		}
		
		if (tval.tv_sec - rtime_limit > data->min_time)
		{
			data->min_time = tval.tv_sec - rtime_limit;
		}
	}
	return 1;
}

#if 0
#include "jbshell.h"
static char rec_disk_time_jbshell_cmd_help[] = "rec_disk_time";
static int rec_disk_time_jbshell_cmd(int argc, char **argv)
{
	NF_DISK_REC_DISK_TIME data;

	//grpid = 0, disk id =0, hide = 1,
	nf_disk_rec_disk_time(0, 0, 1, &data, NULL);

	g_message( "min_time = %d, max_time = %d, used_mbcnt = %d\n\n", 
		data.min_time, data.max_time, data.used_mbcnt);

	return 0;
}
__commandlist(rec_disk_time_jbshell_cmd, "rec_disk_time", rec_disk_time_jbshell_cmd_help, rec_disk_time_jbshell_cmd_help);
#endif

/**
	@brief				��ũ ä�κ� ���ڵ� min,max ���
	@param[in]	hide	������ ���� ( timeline hide�� ����)	
	@param[out]	data	��ȸ ���
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_disk_rec_time( gint hide, NF_DISK_REC_TIME *data, GError **error)
{
	struct recording_se_time_t  min_rectime, max_rectime;
	gint result = 0;

	g_return_val_if_fail ( hide == 0 || hide == 1, 0);
	g_return_val_if_fail ( data != NULL , 0);

	g_return_val_if_fail( nf_filesystem_is_online() , 0);

	//g_message("%s called!!", __FUNCTION__);
		
	memset(&min_rectime, 0x00, sizeof(min_rectime) );
	memset(&max_rectime, 0x00, sizeof(max_rectime) );
		
	result = sst_get_se_time( &min_rectime, (guchar)hide, (guchar)0 ) ;
	if ( result < 0 ) {
		g_warning("%s min result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 
		g_set_error( error, NF_API_DISK_ERROR, result, sst_get_error_string(result) );
		
		return 0;
	}

	result = sst_get_se_time( &max_rectime, (guchar)hide, (guchar)1 ) ;
	if ( result < 0 ) {
		g_warning("%s max result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 
		g_set_error( error, NF_API_DISK_ERROR, result, sst_get_error_string(result) );
		
		return 0;
	}
	
	memset(data, 0x00, sizeof(NF_DISK_REC_TIME));
	
	data->min_time = min_rectime.target_time;
	memcpy(data->min_time_ch, min_rectime.time_for_ch, sizeof(min_rectime.time_for_ch) );

	data->max_time = max_rectime.target_time;
	memcpy(data->max_time_ch, max_rectime.time_for_ch, sizeof(max_rectime.time_for_ch) );

	memcpy(data->ucPortNo, st_disk_info.ucPortNo, sizeof(data->ucPortNo));

	g_message("%s rec_time min[%d] max[%d]", __FUNCTION__, data->min_time, data->max_time);
	
	return 1;		
}


/**
	@brief				��ũ SMART ���� ��� (unc)
	@param[out]	status	SMART ���� ��
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_smart_get_unc ( guint *key, GError **error )
{	
	g_message("%s called!!", __FUNCTION__);
	
	g_return_val_if_fail( key, 0);

	*key = _UNC_key;
				
	return 1;	
}

/**
	@brief				��ũ SMART ���� ��� (boot unc)
	@param[out]	status	SMART ���� ��
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_smart_get_boot_unc ( guint *key, GError **error )
{	
	//g_message("%s called!!", __FUNCTION__);
	
	g_return_val_if_fail( key, 0);

	*key = _UNC_boot_key;
				
	return 1;	
}


/**
	@brief				��ũ SMART ���� ���
	@param[out]	status	SMART ���� ��
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_smart_get_status (NF_SMART_STATUS *status, GError **error)
{	
	g_message("%s called!!", __FUNCTION__);
	
	g_return_val_if_fail( status, 0);
			
	return 1;	
}

#if 0
/**
	@brief				��ũ SMART ������ ���
	@param[out]	data	SMART ���� ���� ������
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_smart_get_data( gint grp_id, gint disk_id, gchar data[4096], GError **error)
{
	char* buffer = NULL;
	
	struct smart_attr_sector_t *smartval = NULL;
	struct smart_thrs_sector_t *smartthres = NULL;
	int i;

	g_return_val_if_fail( grp_id >=0 && grp_id < 2, 0);
	g_return_val_if_fail( disk_id >=0 && disk_id < 16, 0);	
	g_return_val_if_fail(st_disk_info.disk_state[grp_id][disk_id] & 0x01 != 0, 0);
	g_return_val_if_fail( data != NULL, 0);
					
	buffer = sst_get_smart_data( (grp_id*16) + disk_id, &smartval, &smartthres);
	if(buffer != NULL)
	{		

#if 0
		for(i=0; i < MAX_ATTRNAME_LENGTH; i++) {
			if(smartval->attrValue[i].ucAttrID == POWER_ON_HOURS_COUNT_INDEX) {
				disk0_pwrhours = (u32)a_to_u5_le(smartval->attrValue[i].ucRawValue);
				break;
			}
		}
		g_print( buffer );

#endif
	
		memcpy( data, buffer, strnlen(buffer, 4095) );
						
		free(buffer); buffer = NULL;	
		free(smartval); free(smartthres);
		
		return 1;
	}else{
		return 0;
	}
}
#endif

/**
	@brief	proc 
	@param[out]	proc ������
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_get_proc_all(void)
{
	static struct timeval pre_proc_tval ={0, 0};
	s8 cmd[128];
	struct timeval tval ={0, 0};
	
	gettimeofday((struct timeval *)&tval, NULL);	
	if ( ( tval.tv_sec - pre_proc_tval.tv_sec) <= 60 )
	{
		return FALSE;
	}
	
	sprintf(cmd, "cat /proc/ifs > /tmp/webra-info/proc.log");
	proxy_system(cmd, 1, 100);
	sprintf(cmd, "cat /proc/ifs_cache_dump >> /tmp/webra-info/proc.log");
	proxy_system(cmd, 1, 100);
	sprintf(cmd, "cat /proc/ifs_cache_dump_warn >> /tmp/webra-info/proc.log");
	proxy_system(cmd, 1, 100);
	sprintf(cmd, "cat /proc/ifs_cache_lock_check >> /tmp/webra-info/proc.log");
	proxy_system(cmd, 1, 100);
	sprintf(cmd, "cat /proc/ifs_env >> /tmp/webra-info/proc.log");
	proxy_system(cmd, 1, 100);

	sprintf(cmd, "cat /proc/ifs_error_report >> /tmp/webra-info/proc.log");
	proxy_system(cmd, 1, 100);
	sprintf(cmd, "cat /proc/ifs_frame_info >> /tmp/webra-info/proc.log");
	proxy_system(cmd, 1, 100);
	sprintf(cmd, "cat /proc/ifs_mbate_lock >> /tmp/webra-info/proc.log");
	proxy_system(cmd, 1, 100);
	sprintf(cmd, "cat /proc/ifs_mblg_list >> /tmp/webra-info/proc.log");
	proxy_system(cmd, 1, 100);
	sprintf(cmd, "cat /proc/ifs_mblg_list_warn >> /tmp/webra-info/proc.log");
	proxy_system(cmd, 1, 100);


	sprintf(cmd, "cat /proc/ifs_mblg_lock >> /tmp/webra-info/proc.log");
	proxy_system(cmd, 1, 100);
	sprintf(cmd, "cat /proc/ifs_mblg_tracking >> /tmp/webra-info/proc.log");
	proxy_system(cmd, 1, 100);
	sprintf(cmd, "cat /proc/ifs_print_disk_info >> /tmp/webra-info/proc.log");
	proxy_system(cmd, 1, 100);
	sprintf(cmd, "cat /proc/ifs_print_jounal >> /tmp/webra-info/proc.log");
	proxy_system(cmd, 1, 100);
	sprintf(cmd, "cat /proc/ifs_print_mb_list >> /tmp/webra-info/proc.log");
	proxy_system(cmd, 1, 100);

	sprintf(cmd, "cat /proc/ifs_print_timeline >> /tmp/webra-info/proc.log");
	proxy_system(cmd, 1, 100);
	sprintf(cmd, "cat /proc/ifs_rec_pb_status >> /tmp/webra-info/proc.log");
	proxy_system(cmd, 1, 100);
	sprintf(cmd, "cat /proc/ifs_recording_status >> /tmp/webra-info/proc.log");
	proxy_system(cmd, 1, 100);
	sprintf(cmd, "cat /proc/ifs_unc_target_sector >> /tmp/webra-info/proc.log");
	proxy_system(cmd, 1, 100);
	sprintf(cmd, "cat /proc/ifs_view_bio_sect >> /tmp/webra-info/proc.log");
	proxy_system(cmd, 1, 100);

	return TRUE;
}

gboolean check_available_proc(const gchar *string)
{
	g_return_val_if_fail( string != NULL, 0);
	
	if ( strtol(string, NULL, 10) )
	{
		g_message("wrong parameter alphabet(%s)", string);
		return FALSE;
	}

	if ( strstr(string, "ifs") || strstr(string, "net") ||
	
		!strcmp(string, "buddyinfo") ||
		!strcmp(string, "cmdline") ||
		!strcmp(string, "cmem") ||
		!strcmp(string, "cpuinfo") ||
		!strcmp(string, "devices") ||
		
		!strcmp(string, "diskstats") ||
		!strcmp(string, "filesystems") ||
		!strcmp(string, "interrupts") ||
		!strcmp(string, "iomem") ||
		!strcmp(string, "ioports") ||

		!strcmp(string, "key-users") ||
		!strcmp(string, "keys") ||
		!strcmp(string, "kmsg") ||
		!strcmp(string, "loadavg") ||
		!strcmp(string, "locks") ||
		
		!strcmp(string, "meminfo") ||
		!strcmp(string, "misc") ||
		!strcmp(string, "modules") ||
		!strcmp(string, "mounts") ||
		!strcmp(string, "mtd") ||

		!strcmp(string, "pagetypeinfo") ||
		!strcmp(string, "partitions") ||
		!strcmp(string, "scsi/scsi") ||
		!strcmp(string, "slabinfo") ||
		!strcmp(string, "softirqs") ||
		
		!strcmp(string, "stat") ||
		!strcmp(string, "timer_list") ||
		!strcmp(string, "updatime") ||
		!strcmp(string, "version") ||
		!strcmp(string, "vmallocinfo") ||
		
		!strcmp(string, "vmstat") ||
		!strcmp(string, "yaffs") ||
		!strcmp(string, "yaffs_stats") ||
		!strcmp(string, "zoneinfo") )
	{
		return TRUE;
	}
	
	g_message("not support parameter(%s)", string);
	
	return FALSE;
}

/**
	@brief	proc 
	@param[out]	get one proc ������
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_get_proc(const gchar *string, const gchar *string2, const gchar *string3)
{
	static struct timeval pre_proc_one_tval ={0, 0};
	s8 cmd[128];
	struct timeval tval ={0, 0};

	g_return_val_if_fail( string != NULL, 0);
	gettimeofday((struct timeval *)&tval, NULL);	
	if ( ( tval.tv_sec - pre_proc_one_tval.tv_sec) <= 2 )
	{
		return FALSE;
	}

	if ( string3 )
		g_message("%s called!!(%s) (%s) (%s)", __FUNCTION__, string, string2, string3);
	else if ( string2 )
		g_message("%s called!!(%s) (%s)", __FUNCTION__, string, string2);
	else 
		g_message("%s called!!(%s) .. * ?", __FUNCTION__, string);


	if ( strstr(string, "..") || strstr(string, "*") || strstr(string, "?") )//if exist, can't detect .. * 
	{
		g_message("wrong parameter(%s)", string);
		return FALSE;
	}

	if ( string2 && ( strstr(string2, "..") || strstr(string2, "*") || strstr(string2, "?") ) )//if exist
	{
		g_message("wrong parameter(%s)", string);
		return FALSE;
	}

	if ( string3 && ( strstr(string3, "..") || strstr(string3, "*") || strstr(string3, "?") ) )//if exist
	{
		g_message("wrong parameter(%s)", string);
		return FALSE;
	}

	if ( check_available_proc(string) )
	{
		snprintf(cmd, 128, "cat /proc/%s > /tmp/webra-info/proc.log", string);
		proxy_system(cmd, 1, 10);
		
		if( string2 && check_available_proc(string2))
		{
			snprintf(cmd, 128, "cat /proc/%s >> /tmp/webra-info/proc.log", string2);
			proxy_system(cmd, 1, 10);
		}
		
		if( string3 && check_available_proc(string3))
		{
			snprintf(cmd, 128, "cat /proc/%s >> /tmp/webra-info/proc.log", string3);
			proxy_system(cmd, 1, 10);
		}
		return TRUE;
	}
	return FALSE;
}

/**
	@brief				��ũ SMART ������ ���(smartctl)
	@param[out]	data	SMART ���� ���� ������
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_get_smartctl(void)
{
	static struct timeval pre_smart_tval ={0, 0};
	s8 cmd[128];
	struct timeval tval ={0, 0};
	
	gettimeofday((struct timeval *)&tval, NULL);	
	if ( ( tval.tv_sec - pre_smart_tval.tv_sec) <= 60 )
	{
		return FALSE;
	}
	
	sprintf(cmd, "/NFDVR/smartctl -x /dev/sda > /tmp/webra-info/smart.log");
	proxy_system(cmd, 1, 100);
	sprintf(cmd, "/NFDVR/smartctl -x /dev/sdb >> /tmp/webra-info/smart.log");
	proxy_system(cmd, 1, 100);
	sprintf(cmd, "/NFDVR/smartctl -x /dev/sdc >> /tmp/webra-info/smart.log");
	proxy_system(cmd, 1, 100);
	sprintf(cmd, "/NFDVR/smartctl -x /dev/sdd >> /tmp/webra-info/smart.log");
	proxy_system(cmd, 1, 100);
	sprintf(cmd, "/NFDVR/smartctl -x /dev/sde >> /tmp/webra-info/smart.log");
	proxy_system(cmd, 1, 100);
	sprintf(cmd, "/NFDVR/smartctl -x /dev/sdf >> /tmp/webra-info/smart.log");
	proxy_system(cmd, 1, 100);
	sprintf(cmd, "/NFDVR/smartctl -x /dev/sdg >> /tmp/webra-info/smart.log");
	proxy_system(cmd, 1, 100);
	sprintf(cmd, "/NFDVR/smartctl -x /dev/sdh >> /tmp/webra-info/smart.log");
	proxy_system(cmd, 1, 100);
	sprintf(cmd, "/NFDVR/smartctl -x /dev/sdi >> /tmp/webra-info/smart.log");
	proxy_system(cmd, 1, 100);

	return TRUE;
}

/**
	@brief				NAND data ������ ���
	@param[out]	data	SMART ���� ���� ������
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_get_nand_log(void)
{
	static struct timeval pre_nand_tval ={0, 0};
	s8 cmd[128];
	struct timeval tval ={0, 0};
	gint result;
	
	gettimeofday((struct timeval *)&tval, NULL);
	if ( ( tval.tv_sec - pre_nand_tval.tv_sec ) <= 60 )
	{
		return FALSE;
	}

	result = sst_cp_nand_log();
	return ( result == 0 ) ? 1:0;
}

/**
	@brief				sysdb data ������ ���
*/
gboolean nf_get_sysdb(void)
{
	static struct timeval pre_sysdb_tval ={0, 0};
	s8 cmd[128];
	struct timeval tval ={0, 0};
	gint result;
	
	gettimeofday((struct timeval *)&tval, NULL);
	if ( ( tval.tv_sec - pre_sysdb_tval.tv_sec ) <= 60 )
	{
		return FALSE;
	}
#if 0
//	tar cvfz abc nf_sysdb.conf
	sprintf(cmd, "cp -a /NFDVR/data/nf_sysdb.conf /tmp/webra-info/nf_sysdb.conf");
	proxy_system(cmd, 1, 100);

	sprintf(cmd, "cat /NFDVR/data/nf_sysdb_version.conf >> /tmp/webra-info/nf_sysdb.conf");
	proxy_system(cmd, 1, 100);
	
	return TRUE;
#endif
	return nf_sysdb_export("/tmp/webra-info/nf_sysdb.conf");
}

static u16
le_to_u2(u8 *p)
{
	return (u16) ((u16)p[1] << 8) | p[0];
}

static u64
le_to_u6(u8 *p)
{
	return (u64) ((u64)p[5] << 40 | (u64)p[4] << 32) | (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0];
}

/*
	u32 i;
	struct smart_attr_t *pDA = pValue->attrValue;
	struct smart_attr_thrs_t *pTH = pThrs->thrsValue;
	
ATTRIBUTE_NAME          #ID        FLAG     VALUE WORST THRESH TYPE      UPDATED  WHEN_FAILED RAW_VALUE

Raw_Read_Error_Rate       1  0x01  0x000f   118   099   006    Pre-fail  Always       -       174681450
Spin_Up_Time              3  0x03  0x0003   095   095   000    Pre-fail  Always       -       0
Reallocated_Sector_Ct     5  0x05  0x0033   100   100   036    Pre-fail  Always       -       0
Seek_Error_Rate           7  0x07  0x000f   064   060   030    Pre-fail  Always       -       2693938

Reallocation_Event_Ct   196  0xc5  0x0012   100   100   000    Old_age   Always       -       0
Current_Pending_Sector  197  0xc5  0x0012   100   100   000    Old_age   Always       -       0
Offline_Uncorrectable   198  0xc6  0x0010   100   100   000    Old_age   Offline      -       0

Start_Stop_Count          4  0x04  0x0032   100   100   020    Old_age   Always       -       26
Power_On_Hours            9  0x09  0x0032   099   099   000    Old_age   Always       -       983
Spin_Retry_Count         10  0x0a  0x0013   100   100   097    Pre-fail  Always       -       0
Power_Cycle_Count        12  0x0c  0x0032   100   100   020    Old_age   Always       -       11
Temperature_Celsius     194  0xc2  0x0022   030   040   000    Old_age   Always       -       30 (0 18 0 0)
*/
	
/*
�Ʒ� �׸� üũ ��
								  flag   th  val         raw  worst status
01 Raw Read Error Rate            x000F   6  100(         0)   253 Passed
03 Spin Up Time                   x0003   0   97(         0)    97 Passed
04 Start/Stop Count               x0032  20  100(        17)   100 Passed
05 Reallocated Sector Count       x0033  36  100(         0)   100 Passed
07 Seek Error Rate                x000F  30   69(  10172637)    60 Passed
09 Power On Hours Count           x0032   0   96(      4279)    96 Passed
0A Spin Retry Count               x0013  97  100(         0)   100 Passed
0C Power Cycle Count              x0032  20  100(        12)   100 Passed
C2 HDA Temperature                x0022   0   39(98784247847)    60 Passed

C3 Ecc on the Fly Count           x001A   0   82(  29073937)    54 Passed
C5 Current Pending Sector Count   x0012   0  100(         0)   100 Passed
C6 Offline Scan UNC Sector Count  x0010   0  100(         0)   100 Passed
C7 Ultra ATA CRC Error Rate       x003E   0  200(         0)   200 Passed
C8 Multi-zone Error Rate          x0000   0  100(         0)   253 Passed
CA TA Increase Count              x0032   0  100(         0)   253 Passed
*/

/**
	@brief				��ũ SMART ���� ���
	@param[out]	info	SMART ���� ��
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
#define _DP_	fprintf(stderr, "%s() : %d \n",__func__,__LINE__)
#define STR_CAM_TITLE_BADSEC 	"@BAD)SEC_"

//guint	status;		// 0:normal 1:in_the_past 2:check 3:failing_now
gboolean nf_smart_get_info ( gint grp_id, gint disk_id, NF_SMART_DISK_INFO *info, GError **error)
{
	char* buffer = NULL;
	unsigned word[3];
	int i, j, failednow, failedever;	
	guint status;
	u64 tmp;
	struct smart_attr_sector_t *smartval = NULL;
	struct smart_thrs_sector_t *smartthres = NULL;
	struct timeval tval ={0, 0};
	gchar buf[128] = { 0, };
	
	static struct timeval pre_tval[2][16]={0,};
	static NF_SMART_DISK_INFO smart_buf[2][16]={0,};

	u64 thr_sector_count = 0;
	u32 thr_sysdb = 0;

//nf_get_smartctl();
//nf_get_nand_log();
//nf_get_sysdb();
//nf_get_proc();

//	g_message("%s called!!", __FUNCTION__);

	thr_sysdb = nf_sysdb_get_uint("act.sys.disk.smart.threshold");

	if( thr_sysdb == 0 )
		thr_sysdb = 30;

	g_return_val_if_fail( grp_id >=0 && grp_id < 2, 0);
	g_return_val_if_fail( disk_id >=0 && disk_id < 16, 0);	
	g_return_val_if_fail( (st_disk_info.disk_state[grp_id][disk_id] & (0x01+NF_DISK_INFO_FLAG_MIRROR )) != 0, 0);
	g_return_val_if_fail( info != NULL, 0);

	gettimeofday((struct timeval *)&tval, NULL);
	memset(info, 0, sizeof(NF_SMART_DISK_INFO));

	memcpy(info->ucPortNo, st_disk_info.ucPortNo, sizeof(info->ucPortNo));
	
//	g_message("[%d][%d] tval.tv_sec - pre_tval[grp_id][disk_id].tv_sec =%ld", grp_id, disk_id, tval.tv_sec - pre_tval[grp_id][disk_id].tv_sec );

	pthread_mutex_lock(&using_usb_mutex);
	if ( ( ( tval.tv_sec - pre_tval[grp_id][disk_id].tv_sec ) >= SMART_READ_INTERVAL ) &&
			( (tval.tv_sec - pre_tval[grp_id][disk_id].tv_sec ) >= info->update_time.tv_sec ) && !using_usb )
	{
		buffer = sst_get_smart_data( (grp_id*16) + disk_id, &smartval, &smartthres);
		pthread_mutex_unlock(&using_usb_mutex);
		
		if(buffer != NULL)
		{
			struct disk_io_err_t disk_io_err;

			sst_get_disk_io_err_cnt(&disk_io_err);
			info->raw_read_error_rate.value = disk_io_err.err_cnt[grp_id *16 + disk_id];
			info->raw_read_error_rate.threshold = RAW_READ_ERR_RATE_THRESHOLD;				
				
			gettimeofday((struct timeval *)&pre_tval[grp_id][disk_id], NULL);
			
			memcpy( info->raw_text, buffer, strnlen(buffer, 4095) );

_DP_;
			if ( strstr(info->raw_text, "NO_DISK") ) //NO_DISK
			{
				g_message( "====-----------raw_text==grp_id(%d), disk_id(%d)=====", grp_id, disk_id);
				g_message( "\n%s\n", info->raw_text);
				if ( buffer )
					free(buffer);
				if ( smartval )
					free(smartval);
				if ( smartthres )
					free(smartthres);
				
				info->disk_status = SMART_FAILING_NOW;
				memcpy( &smart_buf[grp_id][disk_id], info, sizeof(NF_SMART_DISK_INFO));

				g_warning("%s SMART Failed detected idx[%d]",__FUNCTION__, (grp_id*16)+disk_id);
				sprintf(buf, "SMART Read Fail(%d:%s:%lldGB):0:0:0:0:0:0:0",
							(grp_id*16)+disk_id, st_disk_info.model_num[grp_id][disk_id], st_disk_info.disk_size[grp_id][disk_id]/1024/1024);
				
				nf_eventlog_put_param( NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_SMART_CHECK, (gchar *)buf);
				nf_notify_fire_params("disk_smart", 1, grp_id*16+disk_id, 0, 0);
				
				return 1;
			}

			thr_sector_count = st_disk_info.disk_size[grp_id][disk_id] / (128*1024) * thr_sysdb / 100 ; // disk_dise:kb, MB size(128*1024k)

			for ( i = 0 ; i < NUM_SMART_ATTRIBUTES ; i ++ )
			{
				if ( smartval->attrValue[i].ucValue )
				{
					for (j=0; j<3; j++){
						word[j] = smartval->attrValue[i].ucRawValue[2*j+1];
						word[j] <<= 8;
						word[j] |= smartval->attrValue[i].ucRawValue[2*j];
					}
					
					failednow = (smartval->attrValue[i].ucValue <= smartthres->thrsValue[i].ucThreshold);
					failedever = (smartval->attrValue[i].ucWorstValue <= smartthres->thrsValue[i].ucThreshold);
					
					status = failednow?SMART_FAILING_NOW:(failedever?SMART_IN_THE_PAST:SMART_NORMAL);

					if ( smartval->attrValue[i].ucAttrID == RAW_READ_ERROR_RATE )
					{
						#if 0
						info->raw_read_error_rate.type = le_to_u2(smartval->attrValue[i].ucStatusFlags)&0x01;
						info->raw_read_error_rate.flags = le_to_u2(smartval->attrValue[i].ucStatusFlags);
						info->raw_read_error_rate.value = smartval->attrValue[i].ucValue;
						info->raw_read_error_rate.worst = smartval->attrValue[i].ucWorstValue;
						info->raw_read_error_rate.threshold = smartthres->thrsValue[i].ucThreshold;
						info->raw_read_error_rate.raw = le_to_u6(smartval->attrValue[i].ucRawValue);
						info->raw_read_error_rate.status = status;
						#endif
					}
//					else if ( smartval->attrValue[i].ucAttrID == THROUGHPUT_PERFORMANCE ){}
					else if ( smartval->attrValue[i].ucAttrID == SPIN_UP_TIME )
					{
						info->spin_up_time.type = le_to_u2(smartval->attrValue[i].ucStatusFlags)&0x01;
						info->spin_up_time.flags = le_to_u2(smartval->attrValue[i].ucStatusFlags);
						info->spin_up_time.value = smartval->attrValue[i].ucValue;
						info->spin_up_time.worst = smartval->attrValue[i].ucWorstValue;
						info->spin_up_time.threshold = smartthres->thrsValue[i].ucThreshold;
						info->spin_up_time.raw = word[0];
						info->spin_up_time.status = status;
					}
					else if ( smartval->attrValue[i].ucAttrID == START_STOP_COUNT )
					{
						info->start_stop_cnt = le_to_u6(smartval->attrValue[i].ucRawValue);
					}
					else if ( smartval->attrValue[i].ucAttrID == REALLOCATION_SECTOR_COUNT )
					{
						info->reallocated_sector_ct.type = le_to_u2(smartval->attrValue[i].ucStatusFlags)&0x01;
						info->reallocated_sector_ct.flags = le_to_u2(smartval->attrValue[i].ucStatusFlags);
						info->reallocated_sector_ct.value = smartval->attrValue[i].ucValue;
						info->reallocated_sector_ct.worst = smartval->attrValue[i].ucWorstValue;
						info->reallocated_sector_ct.threshold = smartthres->thrsValue[i].ucThreshold;
						info->reallocated_sector_ct.raw += le_to_u6(smartval->attrValue[i].ucRawValue);
						if ( info->reallocated_sector_ct.raw > thr_sector_count /* REALLOCATED_SECTOR_CNT_THRESHOLD */ )
							status = SMART_CHECK;
						info->reallocated_sector_ct.status = status;
					}
					else if ( smartval->attrValue[i].ucAttrID == SEEK_ERROR_RATE )
					{
						info->seek_error_rate.type = le_to_u2(smartval->attrValue[i].ucStatusFlags)&0x01;
						info->seek_error_rate.flags = le_to_u2(smartval->attrValue[i].ucStatusFlags);
						info->seek_error_rate.value = smartval->attrValue[i].ucValue;
						info->seek_error_rate.worst = smartval->attrValue[i].ucWorstValue;
						info->seek_error_rate.threshold = smartthres->thrsValue[i].ucThreshold;
						info->seek_error_rate.raw = le_to_u6(smartval->attrValue[i].ucRawValue);
						info->seek_error_rate.status = status;
					}
//					else if ( smartval->attrValue[i].ucAttrID == SEEK_TIME_PERFORMANCE ){}
					else if ( smartval->attrValue[i].ucAttrID == POWER_ON_HOURS_COUNT )
					{
						info->power_on_hours = le_to_u6(smartval->attrValue[i].ucRawValue);
					}
					else if ( smartval->attrValue[i].ucAttrID == SPIN_RETRY_COUNT )
					{
						info->spin_retry_cnt = le_to_u6(smartval->attrValue[i].ucRawValue);
					}
					else if ( smartval->attrValue[i].ucAttrID == REPORTED_UNCORRECT_COUNT )
					{
						//add reallocated sector count + current_pending_sector count + reported_uncorret
						info->reallocated_sector_ct.raw += le_to_u6(smartval->attrValue[i].ucRawValue);
						
						if ( info->reallocated_sector_ct.raw > thr_sector_count /* REALLOCATED_SECTOR_CNT_THRESHOLD */ )
							status = SMART_CHECK;
					}
					else if ( smartval->attrValue[i].ucAttrID == AIRFLOW_TEMPERATURE )
					{
						info->temperature_celsius = word[0];//degree
					}
					else if ( smartval->attrValue[i].ucAttrID == POWER_CYCLE_COUNT )
					{
						info->power_cycle_cnt = le_to_u6(smartval->attrValue[i].ucRawValue);
					}
//					else if ( smartval->attrValue[i].ucAttrID == POWER_OFF_RETRACT_COUNT ){}
//					else if ( smartval->attrValue[i].ucAttrID == LOAD_CYCLE_COUNT ){}
					else if ( smartval->attrValue[i].ucAttrID == TEMPERATURE )
					{
						if ( info->temperature_celsius < word[0] )
							info->temperature_celsius = word[0];//degree
					}
					else if ( smartval->attrValue[i].ucAttrID == REALLOCATED_EVENT_COUNT )
					{
						info->reallocation_event_ct.type = le_to_u2(smartval->attrValue[i].ucStatusFlags)&0x01;
						info->reallocation_event_ct.flags = le_to_u2(smartval->attrValue[i].ucStatusFlags);
						info->reallocation_event_ct.value = smartval->attrValue[i].ucValue;
						info->reallocation_event_ct.worst = smartval->attrValue[i].ucWorstValue;
						info->reallocation_event_ct.threshold = smartthres->thrsValue[i].ucThreshold;
						info->reallocation_event_ct.raw = le_to_u6(smartval->attrValue[i].ucRawValue);
						info->reallocation_event_ct.status = status;
					}
					else if ( smartval->attrValue[i].ucAttrID == CURRENT_PENDING_SECTOR )
					{
						info->current_pending_sector.type = le_to_u2(smartval->attrValue[i].ucStatusFlags)&0x01;
						info->current_pending_sector.flags = le_to_u2(smartval->attrValue[i].ucStatusFlags);
						info->current_pending_sector.value = smartval->attrValue[i].ucValue;
						info->current_pending_sector.worst = smartval->attrValue[i].ucWorstValue;
						info->current_pending_sector.threshold = smartthres->thrsValue[i].ucThreshold;
						info->current_pending_sector.raw = le_to_u6(smartval->attrValue[i].ucRawValue);
						info->current_pending_sector.status = status;

						//add reallocated sector count + current_pending_sector count + reported_uncorret
						info->reallocated_sector_ct.raw += info->current_pending_sector.raw;
						
						if ( info->reallocated_sector_ct.raw > thr_sector_count /* REALLOCATED_SECTOR_CNT_THRESHOLD */ )
							status = SMART_CHECK;
					}
					else if ( smartval->attrValue[i].ucAttrID == OFFLINE_UNCORRECTABLE )
					{
						info->offline_uncorrectable.type = le_to_u2(smartval->attrValue[i].ucStatusFlags)&0x01;
						info->offline_uncorrectable.flags = le_to_u2(smartval->attrValue[i].ucStatusFlags);
						info->offline_uncorrectable.value = smartval->attrValue[i].ucValue;
						info->offline_uncorrectable.worst = smartval->attrValue[i].ucWorstValue;
						info->offline_uncorrectable.threshold = smartthres->thrsValue[i].ucThreshold;
						info->offline_uncorrectable.raw = le_to_u6(smartval->attrValue[i].ucRawValue);
						info->offline_uncorrectable.status = status;
					}
//					else if ( smartval->attrValue[i].ucAttrID == ULTRA_DMA_CRC_ERROR_COUNT ){}

					if ( smartval->attrValue[i].ucAttrID == TEMPERATURE || smartval->attrValue[i].ucAttrID == AIRFLOW_TEMPERATURE )
					{
						if ( info->temperature_status < status )
							info->temperature_status = status;
					}
					else
					{
						if ( info->disk_status < status )
							info->disk_status = status;
					}
				}
			}

			{	// http://222.112.8.34:8080/browse/BPM-1542 //  "@BAD)SEC_"
				char tmp[128];						
				char *cam_title = NULL;
				int modify_bad_sec_cnt = 0;
				
				snprintf( tmp, sizeof(tmp)-1 , "%s", nf_sysdb_get_str_nocopy("cam.C3.title") );		
				//g_message("%s cam4_title[%s]",__func__, tmp);				
				if( cam_title = strstr(tmp, STR_CAM_TITLE_BADSEC ) )
				{					
					modify_bad_sec_cnt = atoi( cam_title + strlen(STR_CAM_TITLE_BADSEC) );															
					//g_message("%s cam4_title[%s] raw_cnt[%d] modify_cnt[%d]", __FUNCTION__, cam_title,  info->reallocated_sector_ct.raw,  modify_bad_sec_cnt);										

					info->reallocated_sector_ct.raw = modify_bad_sec_cnt;
					if ( info->reallocated_sector_ct.raw > thr_sector_count /* REALLOCATED_SECTOR_CNT_THRESHOLD */ ){
						info->reallocated_sector_ct.status = status = SMART_CHECK;
						if ( info->disk_status < status ) {
							_DP_;
							info->disk_status = status;
						}
					}
				}
			}

			//detecting proware device
			if ( info->spin_up_time.raw == 0x1883 && info->start_stop_cnt == 0x2e0 &&
				info->power_on_hours == 0xc95 && info->power_cycle_cnt == 0x2c2 )
			{
				//make to clear
				memset(info, 0, sizeof(NF_SMART_DISK_INFO));
				memcpy(info->ucPortNo, st_disk_info.ucPortNo, sizeof(info->ucPortNo));
				info->raw_read_error_rate.value = disk_io_err.err_cnt[grp_id *16 + disk_id];
				info->raw_read_error_rate.threshold = RAW_READ_ERR_RATE_THRESHOLD;
				info->disk_status = SMART_UNKNOWN;
				g_message( "=======SMART_UNKNOWN===================");
			}
			
			memcpy( &smart_buf[grp_id][disk_id], info, sizeof(NF_SMART_DISK_INFO));
			
			g_message( "====raw_text==grp_id(%d), disk_id(%d)=====", grp_id, disk_id);
			g_message( "\n%s\n", info->raw_text);
			g_message( "===========================================");

			if ( buffer )
				free(buffer);
			if ( smartval )
				free(smartval);
			if ( smartthres )
				free(smartthres);
		}else{
			//SMART ERROR
			//There was a disk in booting time. but now DVR Couldn't read SMART INFO(JMicron bug, DISK err)
			g_message( "====SMART smart reading JMB39x fail == grp_id(%d), disk_id(%d)=====", grp_id, disk_id);
		}

		if ( info->disk_status == SMART_FAILING_NOW )
		{
			g_warning("%s SMART Failed detected idx[%d]",__FUNCTION__, (grp_id*16)+disk_id);
			sprintf(buf, "Fail(%d:%s:%lldGB):%d:%lld:%lld:%d:%d:%lld:%d:%d:%d",
					(grp_id*16)+disk_id, st_disk_info.model_num[grp_id][disk_id], st_disk_info.disk_size[grp_id][disk_id]/1024/1024,
					info->raw_read_error_rate.value,
					info->spin_up_time.raw,
					info->reallocated_sector_ct.raw,
					info->seek_error_rate.value,
					info->reallocation_event_ct.value,
					info->current_pending_sector.raw,
					info->offline_uncorrectable.value,
					info->start_stop_cnt,
					info->power_on_hours);
			nf_eventlog_put_param( NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_SMART_CHECK, (gchar *)buf);
			nf_notify_fire_params("disk_smart", 1, grp_id*16+disk_id, 0, 0);
		}
		else	if ( info->disk_status == SMART_CHECK )
		{
			g_warning("%s SMART check detected idx[%d]",__FUNCTION__, (grp_id*16)+disk_id);
			sprintf(buf, "Check(%d:%s:%lldGB):%d:%lld:%lld:%d:%d:%lld:%d:%d:%d",
					(grp_id*16)+disk_id, st_disk_info.model_num[grp_id][disk_id], st_disk_info.disk_size[grp_id][disk_id]/1024/1024,
					info->raw_read_error_rate.value,
					info->spin_up_time.raw,
					info->reallocated_sector_ct.raw,
					info->seek_error_rate.value,
					info->reallocation_event_ct.value,
					info->current_pending_sector.raw,
					info->offline_uncorrectable.value,
					info->start_stop_cnt,
					info->power_on_hours);
			nf_eventlog_put_param( NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_SMART_CHECK, (gchar *)buf);
			nf_notify_fire_params("disk_smart_reqchk", 1, grp_id*16+disk_id, 0, 0);
		}
	}
	else //search interval is short => use cache
	{
		pthread_mutex_unlock(&using_usb_mutex);
		g_message( "====SMART time shortage==grp_id(%d), disk_id(%d)=====", grp_id, disk_id);
		memcpy( info, &smart_buf[grp_id][disk_id], sizeof(NF_SMART_DISK_INFO));
	}
		
	g_message( "====SMART before exit==grp_id(%d), disk_id(%d)====", grp_id, disk_id);
	return 1;
}

#if 1
#include "jbshell.h"

static char nf_get_proc_jbshell_cmd_help[] = "nf_get_proc_jbshell_cmd [string] [string] [string]";
static int nf_get_proc_jbshell_cmd(int argc, char **argv)
{
	if( argc < 2 || argc > 5 )
	{
		printf("%s\n",nf_get_proc_jbshell_cmd_help);
		return -1;
	}

	printf("%s, %d\n", argv[1] , argc);
	
	if ( argc == 2 )
		nf_get_proc(argv[1], NULL, NULL);
	if ( argc == 3 )
		nf_get_proc(argv[1], argv[2], NULL);
	if ( argc == 4 )
		nf_get_proc(argv[1], argv[2], argv[3]);


	return 0;
}
__commandlist(nf_get_proc_jbshell_cmd, "nf_get_proc", nf_get_proc_jbshell_cmd_help, nf_get_proc_jbshell_cmd_help);

static char nf_smart_get_info_jbshell_cmd_help[] = "nf_smart_get_info";
static int nf_smart_get_info_jbshell_cmd(int argc, char **argv)
{
	NF_SMART_DISK_INFO info;
	
	info.update_time.tv_sec = 120;	//120s
	
	nf_smart_get_info(0, 0, &info, NULL);

	g_message( "====raw_text=======");
	g_message( "\n%s\n", info.raw_text);
	g_message( "update_time = %ld", info.update_time.tv_sec);
	g_message( "disk_status = %d", info.disk_status);
	g_message( "start_stop_cnt = %d", info.start_stop_cnt);
	g_message( "power_on_hours = %d", info.power_on_hours);
	g_message( "spin_retry_cnt = %d", info.spin_retry_cnt);
	g_message( "power_cycle_cnt = %d", info.power_cycle_cnt);
	g_message( "tempterature_celsius = %d", info.temperature_celsius);
	
	g_message( "raw_read_error_rate = %d 0x%x %d %d %d %d %lld", 
		info.raw_read_error_rate.type, info.raw_read_error_rate.flags, 
		info.raw_read_error_rate.value, info.raw_read_error_rate.worst, 
		info.raw_read_error_rate.threshold, info.raw_read_error_rate.status,
		info.raw_read_error_rate.raw );
	g_message( "spin_up_time = %d 0x%x %d %d %d %d %lld", 
		info.spin_up_time.type, info.spin_up_time.flags, 
		info.spin_up_time.value, info.spin_up_time.worst, 
		info.spin_up_time.threshold, info.spin_up_time.status,
		info.spin_up_time.raw );	
	g_message( "reallocated_sector_ct = %d 0x%x %d %d %d %d %lld", 
		info.reallocated_sector_ct.type, info.reallocated_sector_ct.flags, 
		info.reallocated_sector_ct.value, info.reallocated_sector_ct.worst, 
		info.reallocated_sector_ct.threshold, info.reallocated_sector_ct.status,
		info.reallocated_sector_ct.raw );	
	g_message( "seek_error_rate = %d 0x%x %d %d %d %d %lld", 
		info.seek_error_rate.type, info.seek_error_rate.flags, 
		info.seek_error_rate.value, info.seek_error_rate.worst, 
		info.seek_error_rate.threshold, info.seek_error_rate.status,
		info.seek_error_rate.raw );	
	g_message( "reallocation_event_ct = %d 0x%x %d %d %d %d %lld", 
		info.reallocation_event_ct.type, info.reallocation_event_ct.flags, 
		info.reallocation_event_ct.value, info.reallocation_event_ct.worst, 
		info.reallocation_event_ct.threshold, info.reallocation_event_ct.status,
		info.reallocation_event_ct.raw );	
	g_message( "current_pending_sector = %d 0x%x %d %d %d %d %lld", 
		info.current_pending_sector.type, info.current_pending_sector.flags, 
		info.current_pending_sector.value, info.current_pending_sector.worst, 
		info.current_pending_sector.threshold, info.current_pending_sector.status,
		info.current_pending_sector.raw );	
	g_message( "offline_uncorrectable = %d 0x%x %d %d %d %d %lld", 
		info.offline_uncorrectable.type, info.offline_uncorrectable.flags, 
		info.offline_uncorrectable.value, info.offline_uncorrectable.worst, 
		info.offline_uncorrectable.threshold, info.offline_uncorrectable.status,
		info.offline_uncorrectable.raw );	
		
	return 0;
}
__commandlist(nf_smart_get_info_jbshell_cmd, "nf_smart_get_info", nf_smart_get_info_jbshell_cmd_help, nf_smart_get_info_jbshell_cmd_help);
#endif

/**
	@brief				��ũ SMART ������ üũ
	@param[out]	data	SMART ���� ���� ������
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_smart_validate_data( gchar data[4096] )
{
	char **lines = NULL;
	int i = 0;	
	
	g_return_val_if_fail( data != NULL, 0);
	
	if( strstr(data, "Failed") ) 
	{
		
		lines = g_strsplit (data, "\n", 128);
		if( lines  == NULL)	
			return 0;
						
		while( lines[i] )
		{
			if( lines[i][0] == '0' || lines[i][0] == 'C' ) {
				//g_message("check : %s", lines[i]);
				if( strstr(lines[i], "Failed") ){
					g_strfreev(lines);
					return 0;
				}
			}else{
				//g_message("pass  : %s", lines[i]);
			}			
			++i;	
		}
		
		g_strfreev(lines);				
	}
		
	return 1;		
}

#ifdef ENABLE_DVR_AVI_PLAYER

/**
	@brief				DVR AVI PLAYER START ( ��� ���۽� ȣ�� )	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_avi_player_start(void)
{
	gint 		result = 0;

#ifdef	DEBUG_AVI_PLAYER_FUNC_CALL
	g_message("%s called!!", __FUNCTION__);
#endif
	
	result = sst_avi_start(); 
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 
	}	
	return ( result == 0 ) ? 1:0;
			
}


/**
	@brief				DVR AVI PLAYER STOP ( ��� ����� ȣ�� )	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_avi_player_finish(void)
{
	gint 		result = 0;

#ifdef	DEBUG_AVI_PLAYER_FUNC_CALL
	g_message("%s called!!", __FUNCTION__);
#endif
	
	result = sst_avi_finish(); 
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 
	}	
	return ( result == 0 ) ? 1:0;	
}


/**
	@brief				DVR AVI PLAYER ����̽� �ݹ� ���
	@param[in]	fxn_cb	�ݹ� function pointer	(if blocking mode, null)
	@param[in]	ctx_cb	�ݹ� function context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_avi_player_dev_set_notify(avi_player_cb_fxn_t fxn_cb, gpointer ctx_cb, GError **error)
{
	gint 		result = 0;
	
#ifdef	DEBUG_AVI_PLAYER_FUNC_CALL
	g_message("%s called!!", __FUNCTION__);
#endif

	//g_return_val_if_fail (fxn_cb != NULL, FALSE);
	
	result = sst_avi_dev_set_notify(fxn_cb, ctx_cb); 
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 

		g_set_error( error, NF_API_DISK_ERROR, result, sst_get_error_string(result) );
		
	}		
	return ( result == 0 ) ? 1:0;
}

/**
	@brief				DVR AVI PLAYER ����̽� �ݹ� ����	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_avi_player_dev_unset_notify(void)
{
	gint 		result = 0;
	
#ifdef	DEBUG_AVI_PLAYER_FUNC_CALL
	g_message("%s called!!", __FUNCTION__);
#endif
	
	result = sst_avi_dev_unset_notify(); 
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 		
	}	
	return ( result == 0 ) ? 1:0;
}

/**
	@brief				DVR AVI PLAYER ����̽� ��� ��� (�ִ� 4��)
	@param[out]	dev_table	����̽� ��� ����(����ڰ� �Ҵ� arch_dev_info_t[4])
	@param[in]	fxn_cb	�ݹ� function pointer	(if blocking mode, null)
	@param[in]	ctx_cb	�ݹ� function context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_avi_player_dev_get_list(NF_AVI_PLAYER_DEV_INFO *dev_table, 
								avi_player_cb_fxn_t fxn_cb, gpointer ctx_cb, GError **error)
{

	gint 		result = 0;

	g_return_val_if_fail (dev_table != NULL, FALSE);
			
#ifdef	DEBUG_AVI_PLAYER_FUNC_CALL
	g_message("%s called!!", __FUNCTION__);
#endif

	result = sst_avi_dev_get_list((avi_dev_info_t *)dev_table, fxn_cb, ctx_cb); 
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 

		g_set_error( error, NF_API_DISK_ERROR, result,  sst_get_error_string(result) );
		
	}
	return ( result > 0 ) ? 1:0;
			
}


/**
	@brief				DVR AVI PLAYER ����̽� ��� ��� (�ִ� 4��) with free size
	@param[out]	dev_table	����̽� ��� ����(����ڰ� �Ҵ� arch_dev_info_t[4])
	@param[in]	fxn_cb	�ݹ� function pointer	(if blocking mode, null)
	@param[in]	ctx_cb	�ݹ� function context
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_avi_player_dev_get_size(NF_AVI_PLAYER_DEV_INFO *dev_table, 
								avi_player_cb_fxn_t fxn_cb, gpointer ctx_cb, GError **error)
{

	gint 		result = 0;

	g_return_val_if_fail (dev_table != NULL, FALSE);
			
#ifdef	DEBUG_AVI_PLAYER_FUNC_CALL
	g_message("%s called!!", __FUNCTION__);
#endif

	result = sst_avi_dev_get_list((avi_dev_info_t *)dev_table, fxn_cb, ctx_cb); 
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 

		g_set_error( error, NF_API_DISK_ERROR, result, sst_get_error_string(result) );
		
	}
	return ( result > 0 ) ? 1:0	;

}

/**
	@brief				����̽��� ���Ե� AVI ���� ���� ��ȸ	
	@param[in]	dev_id	����̽� ID	
	@return	gint		AVI ���� ����, ������ ���� �ڵ�
*/
gint nf_avi_player_get_file_num(guint dev_id)
{
	gint 		result = 0;
		
#ifdef	DEBUG_AVI_PLAYER_FUNC_CALL
	g_message("%s called!! dev_id[%d]", __FUNCTION__, dev_id);
#endif

	result = sst_avi_get_file_num( dev_id ); 
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 
	}	
	return result;
		
}


/**
	@brief				����̽��� ���Ե� AVI ���� ��� ��ȸ
	@param[in]	dev_id	����̽� ID	
	@param[out]	avi_list	avi_list ( ����ڰ� �Ҵ�, 512�� x 256�� )
	@return	gint		AVI ���� ����, ������ ���� �ڵ�
*/
gint nf_avi_player_get_file_list(guint dev_id, guchar **avi_file_list)
{

	gint 		result = 0;

	g_return_val_if_fail (avi_file_list != NULL, FALSE);

#ifdef	DEBUG_AVI_PLAYER_FUNC_CALL
	g_message("%s called!! dev_id[%d]", __FUNCTION__, dev_id);
#endif
	
	result = sst_avi_get_file_list( dev_id, avi_file_list); 
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 
	}	
	return result;
	
}


/**
	@brief				DVR AVI PLAYER�� ����� ���� ����
	@param[in]	full_path	avi ������ ����Ʈ �� full path
	@param[out]	error	return location for a #GError, or %NULL	
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_avi_player_set_play_filename(gchar *full_path, GError **error)
{
	gint 		result = 0;

	g_return_val_if_fail (full_path != NULL, FALSE);

#ifdef	DEBUG_AVI_PLAYER_FUNC_CALL
	g_message("%s called!! full_path[%s]", __FUNCTION__, full_path);
#endif
	
	result = sst_avi_set_play_filename( full_path ); 
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 
	}	
	return ( result > 0 ) ? 1:0	;
}


//----------------------------------------------------------
// function were added by yesing(2011/05/12)

/**
	@brief				DVR MUL PLAYER�� thread�� ��� �����Ǿ����� Ȯ��
	@return	gboolean	%TRUE on success, %FALSE if threads are not ready
*/
gboolean nf_avi_player_check_thread_start(void)
{
	gint 		result = 0;

#ifdef	DEBUG_AVI_PLAYER_FUNC_CALL
	g_message("%s called!!", __FUNCTION__);
#endif
	
	result = sst_check_thread_start();
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 
	}	
	return ( result == 0 ) ? 1:0;
}

/**
	@brief				DVR MUL PLAYER�� channel�� �����Ѵ�. ch�� 31���Ͽ��� �Ѵ�.
	@return	gboolean	%TRUE on success, %FALSE if channel number is bigger than MAX_VIDEO_CHAN(32)
*/
gboolean nf_avi_player_mul_set_ch(guint ch)
{
	gint 		result = 0;

#ifdef	DEBUG_AVI_PLAYER_FUNC_CALL
	g_message("%s called!!", __FUNCTION__);
#endif
	
	result = sst_mul_set_ch(ch);
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 
	}
	return ( result == 0 ) ? 1:0;
}

/**
	@brief				DVR MUL PLAYER���� file�� ������ ��� �´�.
	@param[in]	full_path	������ ��� ���ϴ� ������ full path
	@param[out]			������ ���� in info_in_junk_t
	@return	gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_avi_player_mul_get_info(gchar *full_path, NF_AVI_PLAYER_INFO_IN_JUNK *info, GError **error)
{
	gint 		result = 0;

	g_return_val_if_fail (full_path != NULL, FALSE);
	g_return_val_if_fail (info != NULL, FALSE);

#ifdef	DEBUG_AVI_PLAYER_FUNC_CALL
	g_message("%s called!! full_path[%s]", __FUNCTION__, full_path);
#endif
	
	result = sst_mul_get_info(full_path, (info_in_junk_t *)info);
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 
	}
	return ( result == 0 ) ? 1:0;
}

/**
	@brief				DVR AVI PLAYER���� MD5�� Ȯ�� �Ѵ�.
	@param[in]	full_path	������ ��� ���ϴ� ������ full path
	@return	gboolean		0 on success, -1 if an error occurred, -2 verify check result is error.
*/
gint nf_avi_player_check_md5(gchar *full_path, GError **error)
{
	gint 		result = 0;

	g_return_val_if_fail (full_path != NULL, FALSE);

#ifdef	DEBUG_AVI_PLAYER_FUNC_CALL
	g_message("%s called!! full_path[%s]", __FUNCTION__, full_path);
#endif

	nf_set_using_usb(TRUE);
	result = sst_check_md5(full_path);
	nf_set_using_usb(FALSE);
	
//	result = sst_check_file(full_path);	//for test
	if ( result < 0 ) {
		if ( result == -2 )
			g_message("%s called!! verify check result is failed.(%s)", __FUNCTION__, full_path);
		else
			g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 
	}
	return result;
}

/**
	@brief				DVR AVI PLAYER ���� MD5�� progress�� return �Ѵ�.
	@return	gint			MD5 progress percentage && -1 if MD5 progress is completed
*/
gint nf_avi_player_md5_get_progress(void)
{
	gint 		result = 0;

#ifdef	DEBUG_AVI_PLAYER_FUNC_CALL
	g_message("%s called!!", __FUNCTION__);
#endif
	
	result = sst_md5_get_progress();
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 
	}	
	return result;
}

/**
	@brief				DVR AVI PLAYER���� file�� play���� ������ Ȯ�� �Ѵ�.
	@param[in]	full_path	������ ��� ���ϴ� ������ full path
	@return	gboolean		%TRUE on success, %FALSE if an error occurred
*/
gint nf_avi_player_check_file(gchar *full_path, GError **error)
{
	gint 		result = 0;

	g_return_val_if_fail (full_path != NULL, FALSE);

#ifdef	DEBUG_AVI_PLAYER_FUNC_CALL
	g_message("%s called!! full_path[%s]", __FUNCTION__, full_path);
#endif

	result = sst_check_file(full_path);

	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 
	}
	return ( result == 0 ) ? 1:0;
}

/**
	@brief				���� check file�� cancel�Ѵ�.
	@return	gboolean		%TRUE on success, %FALSE if an error occurred
*/
gint nf_avi_player_verify_cancel(GError **error)
{
	gint 		result = 0;

#ifdef	DEBUG_AVI_PLAYER_FUNC_CALL
	g_message("%s called!!", __FUNCTION__);
#endif
	
	result = sst_verify_cancel();
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, sst_get_error_string(result)); 
	}
	return ( result == 0 ) ? 1:0;
}

#if 1
static char nf_avi_player_check_file_jbshell_cmd_help[] = "nf_avi_player_check_file_jbshell_cmd [string] [string] [string]";
static int nf_avi_player_check_file_jbshell_cmd(int argc, char **argv)
{
	if( argc != 2 )
	{
		g_message("%s",nf_avi_player_check_file_jbshell_cmd_help);
		return -1;
	}

	nf_avi_player_check_file(argv[1], NULL);

	return 0;
}
__commandlist(nf_avi_player_check_file_jbshell_cmd, "nf_get_proc", 
	nf_avi_player_check_file_jbshell_cmd_help, nf_avi_player_check_file_jbshell_cmd_help);
#endif

#endif //ENABLE_DVR_AVI_PLAYER


gboolean nf_estg_get_info(int estg_id, NF_ESTG_DISK_INFO *disk_info, cb_estg_fxn_t fxn_cb, gpointer ctx_cb, GError **error )
{
	gint result = 0;

	g_return_val_if_fail( estg_id > 0 && estg_id < 2, 0);
	g_return_val_if_fail( disk_info != NULL, 0);

	
	result = estg_get_info( estg_id, (estg_disk_info_t *)disk_info, fxn_cb, ctx_cb ) ;
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, "estg_error"); 
		g_set_error( error, NF_API_DISK_ERROR, result, "estg_error" );						
		return 0;
	}
	
	return 1;
		
}

gboolean nf_estg_get_mode(int estg_id, NF_ESTG_RAID_MODE *raid_mode, cb_estg_fxn_t fxn_cb, gpointer ctx_cb, GError **error)
{
	gint result = 0;

	g_return_val_if_fail( estg_id > 0 && estg_id < 2, 0);
	g_return_val_if_fail( raid_mode != NULL, 0);

	result = estg_get_mode( estg_id, (estg_raid_mode_t *)raid_mode, fxn_cb, ctx_cb ) ;
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, "estg_error"); 
		g_set_error( error, NF_API_DISK_ERROR, result, "estg_error" );						
		return 0;
	}
	
	return 1;

}

gboolean nf_estg_set_mode(int estg_id, NF_ESTG_RAID_MODE *raid_mode, cb_estg_fxn_t fxn_cb, gpointer ctx_cb, GError **error)
{
	gint result = 0;

	g_return_val_if_fail( estg_id > 0 && estg_id < 2, 0);
	g_return_val_if_fail( raid_mode != NULL, 0);

	result = estg_set_mode( estg_id, (estg_raid_mode_t *)raid_mode, fxn_cb, ctx_cb ) ;
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, "estg_error"); 
		g_set_error( error, NF_API_DISK_ERROR, result, "estg_error" );						
		return 0;
	}
	
	return 1;	
}

gboolean nf_estg_get_status(int estg_id, NF_ESTG_DISK_STATUS *disk_status, cb_estg_fxn_t fxn_cb, gpointer ctx_cb, GError **error)
{
	gint result = 0;

	g_return_val_if_fail( estg_id > 0 && estg_id < 2, 0);
	g_return_val_if_fail( disk_status != NULL, 0);

	result = estg_get_status( estg_id, (estg_disk_status_t *)disk_status, fxn_cb, ctx_cb ) ;
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, "estg_error"); 
		g_set_error( error, NF_API_DISK_ERROR, result, "estg_error" );						
		return 0;
	}
	
	return 1;		
}

gboolean nf_estg_get_progress(int estg_id, NF_ESTG_PROGRESS_STATE *progress_state, cb_estg_fxn_t fxn_cb, gpointer ctx_cb, GError **error)
{
	gint result = 0;

	g_return_val_if_fail( estg_id > 0 && estg_id < 2, 0);
	g_return_val_if_fail( progress_state != NULL, 0);

	result = estg_get_progress( estg_id, (estg_progress_state_t *)progress_state, fxn_cb, ctx_cb ) ;
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, "estg_error"); 
		g_set_error( error, NF_API_DISK_ERROR, result, "estg_error" );						
		return 0;
	}
	
	return 1;
}

gboolean nf_estg_reinit(int estg_id, cb_estg_fxn_t fxn_cb, gpointer ctx_cb, GError **error)
{
	gint result = 0;

	g_return_val_if_fail( estg_id > 0 && estg_id < 2, 0);

	result = estg_reinit( estg_id, fxn_cb, ctx_cb ) ;
	if ( result < 0 ) {
		g_warning("%s result[%d](%s)\n", __FUNCTION__, result, "estg_error"); 
		g_set_error( error, NF_API_DISK_ERROR, result, "estg_error" );						
		return 0;
	}
	
	return 1;	
}
/******************************************************************************/

#ifdef ENABLE_DISK_PRESERVE

/**
	@brief			playback ��带 �����Ѵ�.
	@param[in]	    mode   0:normal 1:preserved
	@param[in]		param1 preserved sessid	
	@return	gboolean		%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_disk_preserve_pb_mode( guint mode, guint param1)
{

	gint result = 0;
	g_message("%s mode[%d] param[%d]", __FUNCTION__, mode, param1);
	
	result = sst_set_pb_mode(mode, param1);
	if ( result < 0 ) {
		g_warning("%s sst_set_pb_mode result[%d]\n", __FUNCTION__, result); 		
		return 0;
	}
	
	return 1;		
}

/**
	@brief			preserved list ���� ��ȸ �Ѵ�.
	@return	gint		result count  on success, -1 if an error occurred
*/
gint nf_disk_preserve_list_cnt()
{		
	gint result = 0;	
	g_return_val_if_fail( nf_filesystem_is_online() , -1);
	
	result = sst_ps_get_total_info();
	if ( result < 0 ) {
		g_warning("%s sst_set_pb_mode result[%d]\n", __FUNCTION__, result); 		
		return -1;
	}
	g_message("%s count[%d]", __FUNCTION__, result);
	
	return result;		
}


/**
	@brief			preserved list ���� ��ȸ �Ѵ�.
	@param[in]	    start_no   start index ( no )
	@param[in]		count      request count
	@param[in]		pbuff      result buffer   NF_DISK_PRESERVE_INFO * count 
	@return	gint		result count  on success, -1 if an error occurred
*/
gint  nf_disk_preserve_list_get(  guint start_no, guint count, NF_DISK_PRESERVE_INFO *pbuff)
{		
	gint result = 0;	

	g_return_val_if_fail( nf_filesystem_is_online() , -1);
	g_return_val_if_fail( count > 0 && count < 256 , -1);
	g_return_val_if_fail( pbuff != NULL , -1);
	
	result = sst_ps_get_info(start_no, count, (struct ps_get_info_entry *)pbuff);
	if ( result < 0 ) {
		g_warning("%s sst_ps_get_info result[%d]\n", __FUNCTION__, result); 		
		return -1;
	}	
	return result;	
}


char print_preserve_info_help[] = "ps_print_info\n";
int
ps_print_info(int argc, char **argv)
{
	int res;
	int i;
	struct ps_get_info_entry *buffer;
	res = sst_ps_get_total_info();
	if(res < 0){
		printf("Can't retrieve any information for preservation\n");
	}else{
		printf("total preserve session : %u\n",res);
	}

	printf("[PRESERVE ENTRIES]========================\n");
	buffer = malloc(sizeof(struct ps_get_info_entry) *10);
	sst_ps_get_info(0,10,buffer);
	
	for( i=0; i < 10; i++){
		printf("no : %u sess_id : %u, st : %u, et : %u, size : %u, status : %u\n",
			buffer[i].no, buffer[i].sess_id, 
			buffer[i].start_timestamp, buffer[i].end_timestamp,
			buffer[i].mb_cnt*128, buffer[i].status);
	}
	free(buffer);
}
__commandlist(ps_print_info, "ps_print_info", "get preserve_information", print_preserve_info_help);


char ps_get_logs_help[] = "ps_get_logs [sess_id] [count] [loop]\n";
int
ps_get_logs(int argc, char **argv)
{
	int res;
	int i, j;
	u32 sess_id;
	int total_cnt;
	u32 count, read_count,loop_cnt;
	struct log_entry *buffer;

	if ( argc < 4 ) {
		fprintf(stderr, "check param !!!\n");
		printf("%s\n",ps_get_logs_help);
		return -1;
	}
	
	sess_id =  atoi(argv[1]);
	count =  atoi(argv[2]);
	loop_cnt =  atoi(argv[3]);
	
	buffer = malloc(sizeof(struct log_entry) *count);
	memset(buffer,0,sizeof(struct log_entry) *count);
	if((total_cnt = sst_ps_open_log_stream(sess_id)) < 0){
		printf("Cannot open a log stream from preserve data\n");
		free(buffer);
		return 0;
	}
	printf("total_cnt : %d\n",total_cnt);


	for(j = 0; j < loop_cnt; j++){
		read_count = sst_ps_get_log_stream(count,buffer);

		for( i=0; i < read_count; i++){
			printf("no : %d,type: %u timestamp : %llu, contents : %s\n",
				j*count + i, buffer[i].ucType, buffer[i].ulTimeStamp, buffer[i].sText);
		}
	}

	sst_ps_close_log_stream();
	free(buffer);
}
__commandlist(ps_get_logs, "ps_get_logs", "get logs of deleted recordings", ps_get_logs_help);


char ps_set_sessid_help[] = "ps_set_sessid [sess_id]\n";
int
ps_set_sessid(int argc, char **argv)
{
	int res;
	u32 sess_id;
	
	if ( argc < 1 ) {
		fprintf(stderr, "check param !!!\n");
		printf("%s\n",ps_set_sessid_help);
		return -1;
	}
	
	sess_id =  atoi(argv[1]);
	
	if(sess_id==0)
		res = sst_set_pb_mode(0,0);
	else 
		res = sst_set_pb_mode(1,sess_id);
		
	printf("sst_set_pb_mode res[%d]\n", res);
		
}
__commandlist(ps_set_sessid, "ps_set_sessid", "set sessid for playback", ps_set_sessid_help);

#endif


gboolean nf_set_passwd(unsigned char* passwd , unsigned int size)
{
	int res;
	
	printf("Enter nf_set_passwd SUCCESS \n");
	
	res = sst_set_passwd(passwd , size);
	
	if( res < 0 ){
		g_message("%s failed", __FUNCTION__);
		return FALSE;
	}
	
	printf("nf_set_passwd SUCCESS \n");

	return TRUE;

}
