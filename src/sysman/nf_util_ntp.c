#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>		//for open
#include <unistd.h>		//for lseek
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "jbshell.h"
#include "nf_common.h"
#include "nf_util_ntp.h"


static guint _smart_ntp_time = 0;
static guint _smart_ntp_enable = 0;

static void _smart_ntp_reload(void)
{
	gint is_off = 0;
	
	_smart_ntp_time = nf_sysdb_get_uint( "sys.date.smart_ntp_time");

	is_off = nf_sysdb_get_bool("sys.date.time_sync_off");
	if( is_off ){
		_smart_ntp_enable = 0;
	}
	else{
		_smart_ntp_enable = 1;
	}
}

static int _update_rtc_time_by_ntp(void)
{
	gchar *timesvr;
	time_t utc_tval;
	struct timeval rtv;
	gchar rtc_str[1024];
	
	timesvr = nf_sysdb_get_str_nocopy("sys.date.timesvr");
	if( timesvr == NULL )
	{
		printf("%s => time_server invalid\n", __FUNCTION__);
		return -1;
	}

	utc_tval = nf_util_sntp(timesvr);
	if( utc_tval == 0 )
	{
		printf("%s => nf_util_sntp err\n", __FUNCTION__);
		return -1;
	}
	
//	utc_tval = 1474419600;

	if( nf_datetime_rtc_set(utc_tval) == FALSE )
	{
		printf("%s => RTC write failed\n", __FUNCTION__);
		return -1;
	}

	// nf_onvif_set_time_sync_all(); // time sync
/*
	if( nf_datetime_rtc_get(rtc_str) == FALSE )
	{
		printf("%s => RTC GET failed\n", __FUNCTION__);
	}
	else
	{
		printf("%s => RTC[%s]\n", __FUNCTION__, rtc_str);
	}
*/
	return 0;
}

static void _smart_ntp_sysdb_reload_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	g_return_if_fail(pinfo != NULL);
	g_message("%s called", __FUNCTION__);

	if( pinfo->d.params[0] == NF_SYSDB_CATE_SYS )
		_smart_ntp_reload();
}

static void _smart_ntp_thread(void *arg)
{
	guint wait_sec_cnt = 0;

	printf("%s called", __FUNCTION__);
	
	_smart_ntp_reload();

	sleep( 60 * 10 );	// wait 10 min;
	
	wait_sec_cnt = _smart_ntp_time;

	while(1)
	{
		if( _smart_ntp_time != 0 && ( _smart_ntp_time <= wait_sec_cnt ) )
		{
			g_message("%s => _smart_ntp_time [loop : %u]", __FUNCTION__, _smart_ntp_time);

			if( _smart_ntp_enable == 1 )
			{
				if( _update_rtc_time_by_ntp() < 0 )
				{
					// for retry... 1 min
					sleep(60);
					continue;
				}
				else
				{
					wait_sec_cnt = 0;
				}
			}
			else
			{
				g_message("%s => smart_ntp off", __FUNCTION__);
				wait_sec_cnt = 0;
			}
		}

		wait_sec_cnt++;
		g_usleep(1000*1000);
	}
}

gboolean nf_smart_ntp_init(void)
{
    pthread_t tid = 0;
	guint	cb_handle = 0;

	cb_handle= nf_notify_connect_cb( "sysdb_change", _smart_ntp_sysdb_reload_cb_func, NULL);
	g_message("%s reload sysdb connect_cb[%d]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

    if ( pthread_create(&tid, NULL, (void*)_smart_ntp_thread, NULL) != 0)
    {
        return 0;
    }

    pthread_detach(tid);
	
    return 1;
}

