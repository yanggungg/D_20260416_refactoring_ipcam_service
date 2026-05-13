#include <glib.h>
#include <glib/gprintf.h>
#include <string.h>

#include "nf_sysdb.h"
#include "nf_notify.h"
#include "nfdal.h"



static int g_ch = 0;

#define	MAX_TEMP_STR_SIZE	(20)
#define	DDEBUG_START		fprintf(stderr, "[GUI DEBUG] NFDAL %s:%d  %s Start\n", __FILE__, __LINE__, __func__);
#define	DDEBUG_END			fprintf(stderr, "[GUI DEBUG] NFDAL %s:%d  %s End\n", __FILE__, __LINE__, __func__);
#define	DDEBUG_FAIL			fprintf(stderr, "[GUI DEBUG] NFDAL %s:%d  %s Fail\n", __FILE__, __LINE__, __func__);

/**********************************************
 *  FOR BAUD RATE
 *  ******************************************/

static gint prvBaudrateToIndex(guint db_baud)
{
	gint baud = BAUD_NONE;

	switch(db_baud)
	{
		case 2400:		baud = BAUD_2400;		break;
		case 4800:		baud = BAUD_4800;		break;
		case 9600:		baud = BAUD_9600;		break;
		case 19200:		baud = BAUD_19200;		break;
		case 38400:		baud = BAUD_38400;		break;
		case 57600:		baud = BAUD_57600;		break;
		case 115200:	baud = BAUD_115200;		break;
//		default:		baud = BAUD_NONE;		break;
		default:		baud = BAUD_4800;		break;
	}

	return baud;
}

static guint prvIndexToBaudrate(gint index)
{
	guint db_baud = 0;

	switch(index)
	{
		case BAUD_2400:		db_baud = 2400;			break;
		case BAUD_4800:		db_baud = 4800;			break;
		case BAUD_9600:		db_baud = 9600;			break;
		case BAUD_19200:	db_baud = 19200;		break;
		case BAUD_38400:	db_baud = 38400;		break;
		case BAUD_57600:	db_baud = 57600;		break;
		case BAUD_115200:	db_baud = 115200;		break;
//		default:			db_baud = 0;			break;
		default:			db_baud = 4800;			break;
	}

	return db_baud;
}

/*****************************************
 * FOR MOTION SENSITIVITY
 * DB DATA	 : 0 ~ 100
 * GUI VALUE : 1 ~ 10
 * GUI INDEX : 0 ~ 9
 * **************************************/

static guint prvMotSenseToIndex(guint sense)
{
	guint ret;

	ret = (sense / 10) - 1;

	if(ret < 0)			ret = 0;
	else if(ret > 9)	ret = 9;

	return ret;
}

static guint prvIndexToMotSense(guint index)
{
	guint ret;

	ret = (index + 1) * 10;

	if(ret<0)			ret = 0;
	else if(ret>100)	ret = 100;

	return ret;
}


/*****************************************************************
 * FOR PTZ SPEED (P/T Speed, Zoom Speed, Focus Speed, Iris Speed)
 * DB DATA	 : 0 ~ 100
 * GUI VALUE : 1 ~ 10
 * GUI INDEX : 0 ~ 9
 * ***************************************************************/

static guint prvPTZSpeedToIndex(guint speed)
{
	guint ret;

	ret = (speed / 10) - 1;

	if(ret < 0)			ret = 0;
	else if(ret > 9)	ret = 9;

	return ret;
}

static char _get_yoil(UsrDefHolidayData * holi)
{
    char yoil;
    
    if(holi->yoil == 0) yoil = 'A';
    else if(holi->yoil == 1) yoil = 'B';
    else if(holi->yoil == 2) yoil = 'C';
    else if(holi->yoil == 3) yoil = 'D';
    else if(holi->yoil == 4) yoil = 'E';
    else if(holi->yoil == 5) yoil = 'F';
    else if(holi->yoil == 6) yoil = 'G';
 
return yoil;  
}

static guint prvIndexToPTZSpeed(guint index)
{
	guint ret;

	ret = (index + 1) * 10;

	if(ret<0)			ret = 0;
	else if(ret>100)	ret = 100;

	return ret;
}



/*****************************************************************
 * FOR IP ADDRESS
 * ***************************************************************/
static void prvIntToIP(guint *out, guint in)
{
	out[0] = (in >> 24) & 255;
	out[1] = (in >> 16) & 255;
	out[2] = (in >> 8) & 255;
	out[3] = in & 255;
}

static guint prvIPToInt(guint ip[4])
{
	guint ret = 0;

	ret += (ip[0] << 24);
	ret += (ip[1] << 16);
	ret += (ip[2] << 8);
	ret += ip[3];

	return ret;
}

// Holiday func
static int _get_year(char *date)
{
    char tmp[8];
    memset(tmp, 0x00, sizeof(tmp));
    strncpy(tmp, date, 4);
    return atoi(tmp);
}
static int _get_month(char *date)
{
    char tmp[8];
    memset(tmp, 0x00, sizeof(tmp));
    strncpy(tmp, date + 4, 2);
    return atoi(tmp);
}
static int _get_day(char *date)
{
    char tmp[8];
    memset(tmp, 0x00, sizeof(tmp));
    strncpy(tmp, date + 6, 2);
    return atoi(tmp);
}
static int _get_week(char *date)
{
    char tmp[8];
    memset(tmp, 0x00, sizeof(tmp));
    strncpy(tmp, date + 6, 2);
    return ((atoi(tmp) - 1) / 7) + 1;
}
static char _get_type(char *date)
{
    char tmp[8];
    memset(tmp, 0x00, sizeof(tmp));
    strncpy(tmp, date + 9, 1);
    //sprintf(tmp,"%d", tmp);
    return tmp[0];
}
static int _convert_to_holi(char *date, UsrDefHolidayData *holi)
{
    if (strlen(date) != 10) return -1;
    holi->year = _get_year(date);
    holi->month = _get_month(date);
    holi->day = _get_day(date);
    if (!ifn_is_valid_date(holi->year, holi->month, holi->day)) return -1;
    holi->yoil = ifn_get_yoil(holi->year, holi->month, holi->day);
    holi->week = _get_week(date);
    holi->type = _get_type(date);
    printf("SKSHIN] %d, %d, %d, %d, %d, %c\n", holi->year, holi->month, holi->day, holi->yoil, holi->week, holi->type);
    return 0;
}
//


/**
	@brief	sysdb¸¦ ÃÊ±âÈ­ÇÏ´Â DAL ÇÔ¼ö
*/
void DAL_init(int ch_count)
{
	DDEBUG_START
	g_ch = ch_count;
	if (g_thread_supported() == FALSE) {
		g_thread_init(NULL);
	}
	g_type_init();
//	nf_sysdb_init();
//	nf_sysdb_test();






	DDEBUG_END
}

guint DAL_notify_fire_DB_change(guint category)
{
	nf_notify_fire_params("sysdb_change", category, 0, 0, 0);
    return 0;
}

guint DAL_notify_fire_DB_change2(guint category, guint chmask, gint sub_category)
{
	nf_notify_fire_params("sysdb_change", category, chmask, sub_category, 0);
    return 0;
}

guint DAL_notify_fire_DB_sync(guint category, guint channel)
{
	nf_notify_fire_params("sysdb_tmp_change", category, channel, 0, 0);
    return 0;
}

guint DAL_notify_fire_ipcam_change(guint category, guint chmask)
{
	nf_notify_fire_params("sysdb_ipcam_change", category, chmask, 0, 0);
    return 0;
}


/**************************************************************************
 * Setup --> System --> Camera
 * ************************************************************************/

/**
	@brief
	@param[in]
	@param[in]
	@return
*/
guint DAL_get_ipCam_data(IPCamData *data, guint camIdx)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key1("ipcam.C%d.title", camIdx, &ret_value, NULL))
	{
		g_stpcpy(data->title, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	if(nf_sysdb_get_key1("ipcam.C%d.mac", camIdx, &ret_value, NULL))
	{
		g_stpcpy(data->mac, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 2;

	if(nf_sysdb_get_key1("ipcam.C%d.model", camIdx, &ret_value, NULL))
	{
		g_stpcpy(data->model, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 3;

	if(nf_sysdb_get_key1("ipcam.C%d.rx_method", camIdx, &ret_value, NULL))
	{
		g_stpcpy(data->method, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 4;

	if(nf_sysdb_get_key1("ipcam.C%d.rtsp_url", camIdx, &ret_value, NULL))
	{
		g_stpcpy(data->rtsp_url, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 5;

	if(nf_sysdb_get_key1("ipcam.C%d.rtsp_port", camIdx, &ret_value, NULL))
	{
		data->rtsp_port = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 6;

	if(nf_sysdb_get_key1("ipcam.C%d.http_url", camIdx, &ret_value, NULL))
	{
		g_stpcpy(data->http_url, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 7;

	if(nf_sysdb_get_key1("ipcam.C%d.http_port", camIdx, &ret_value, NULL))
	{
		data->http_port = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 8;

	if(nf_sysdb_get_key1("ipcam.C%d.codec", camIdx, &ret_value, NULL))
	{
		g_stpcpy(data->codec, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 9;

	if(nf_sysdb_get_key1("ipcam.C%d.size", camIdx, &ret_value, NULL))
	{
		g_stpcpy(data->size, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 10;

	if(nf_sysdb_get_key1("ipcam.C%d.motion", camIdx, &ret_value, NULL))
	{
		data->enMotion = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 11;

	if(nf_sysdb_get_key1("ipcam.C%d.alarm", camIdx, &ret_value, NULL))
	{
		data->enAlarm = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 12;

	if(nf_sysdb_get_key1("ipcam.C%d.use_master", camIdx, &ret_value, NULL))
	{
		data->useDVRPassword = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 13;

	if(nf_sysdb_get_key1("ipcam.C%d.username", camIdx, &ret_value, NULL))
	{
		g_stpcpy(data->id, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 14;

	if(nf_sysdb_get_key1("ipcam.C%d.password", camIdx, &ret_value, NULL))
	{
		g_stpcpy(data->passwd, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 15;

	if(nf_sysdb_get_key1("ipcam.C%d.covert", camIdx, &ret_value, NULL))
	{
		data->covert = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 16;

	if(nf_sysdb_get_key1("ipcam.C%d.audio_rx", camIdx, &ret_value, NULL))
	{
		data->audio = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 17;

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_set_ipCam_data(IPCamData *data, guint camIdx)
{
	guint ret = 0;
	GValue set_value = {0,};

	DDEBUG_START

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->title);
	g_message("data->title = %s cam_index = %d \n", data->title, camIdx);
	if(!nf_sysdb_set_key1("ipcam.C%d.title", camIdx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->mac);
	g_message("data->mac = %s cam_index = %d \n", data->mac, camIdx);
	if(!nf_sysdb_set_key1("ipcam.C%d.mac", camIdx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->model);
	g_message("data->model = %s cam_index = %d \n", data->model, camIdx);
	if(!nf_sysdb_set_key1("ipcam.C%d.model", camIdx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->method);
	g_message("data->method = %s cam_index = %d \n", data->method, camIdx);
	if(!nf_sysdb_set_key1("ipcam.C%d.rx_method", camIdx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->rtsp_url);
	g_message("data->rtsp_url = %s cam_index = %d \n", data->rtsp_url, camIdx);
	if(!nf_sysdb_set_key1("ipcam.C%d.rtsp_url", camIdx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->rtsp_port);
	if(!nf_sysdb_set_key1("ipcam.C%d.rtsp_port", camIdx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->http_url);
	g_message("data->http_url = %s cam_index = %d \n", data->http_url, camIdx);
	if(!nf_sysdb_set_key1("ipcam.C%d.http_url", camIdx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 7;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->http_port);
	if(!nf_sysdb_set_key1("ipcam.C%d.http_port", camIdx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 8;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->codec);
	g_message("data->codec = %s cam_index = %d \n", data->codec, camIdx);
	if(!nf_sysdb_set_key1("ipcam.C%d.codec", camIdx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 9;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->size);
	g_message("data->size = %s cam_index = %d \n", data->size, camIdx);
	if(!nf_sysdb_set_key1("ipcam.C%d.size", camIdx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 10;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->enMotion);
	if(!nf_sysdb_set_key1("ipcam.C%d.motion", camIdx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 11;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->enAlarm);
	if(!nf_sysdb_set_key1("ipcam.C%d.alarm", camIdx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 12;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->useDVRPassword);
	if(!nf_sysdb_set_key1("ipcam.C%d.use_master", camIdx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 13;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->id);
	g_message("data->id = %s cam_index = %d \n", data->id, camIdx);
	if(!nf_sysdb_set_key1("ipcam.C%d.username", camIdx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 14;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->passwd);
	g_message("data->passwd = %s cam_index = %d \n", data->passwd, camIdx);
	if(!nf_sysdb_set_key1("ipcam.C%d.password", camIdx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 15;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->covert);
	if(!nf_sysdb_set_key1("ipcam.C%d.covert", camIdx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 16;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->audio);
	if(!nf_sysdb_set_key1("ipcam.C%d.audio_rx", camIdx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 17;
	}
	g_value_unset(&set_value);

	DDEBUG_END
	return ret;
}



guint DAL_get_ipcam_count(guint cnt)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("ipcam.CCNT", &ret_value, NULL))
	{
		ret = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}

	return ret;
}

#ifdef ENABLE_HNF_IPCAM
guint DAL_set_ipcam_count(guint cnt)
{
	guint ret = 0;
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_IPCAM);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, cnt);
	nf_sysdb_set_key0("ipcam.UCNT", &set_value, NULL);

	if(!nf_sysdb_set_key0("ipcam.CCNT", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_IPCAM);
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}

	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_IPCAM, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_IPCAM);

	DDEBUG_END
	return ret;
}

guint DAL_save_ipcam_data()
{
	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_IPCAM);

	DAL_save_db("ipcam");

//	nf_notify_fire_params("sysdb_cam_title", 0, 0, 0, 0);
//	nf_notify_fire_params("sysdb_covert", 0, 0, 0, 0);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_IPCAM, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_IPCAM);

	DDEBUG_END
	return ret;
}
#endif


/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/

gboolean DAL_get_cam_show()
{
	gboolean ret = FALSE;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("cam.SHOW", &ret_value, NULL))
	{
		ret = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}

	return ret;
}

guint DAL_set_cam_show(gboolean show)
{
	guint ret = 0;
	GValue set_value = {0,};

	DDEBUG_START
	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, show);

	nf_sysdb_set_key0("cam.SHOW", &set_value, NULL);

	if(!nf_sysdb_set_key0("cam.SHOW", &set_value, NULL))
	{
		g_value_unset(&set_value);

		DDEBUG_FAIL
		return 1;
	}

	g_value_unset(&set_value);
	DDEBUG_END

	return ret;

}


guint DAL_get_camera_data(CameraData *data, guint channel)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key1("cam.C%d.title", channel,  &ret_value, NULL))
	{
		g_stpcpy(data->title, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 1;
	}

	if(nf_sysdb_get_key1("cam.C%d.covert", channel, &ret_value, NULL))
	{
		data->covert = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 2;
	}

	if(nf_sysdb_get_key1("cam.C%d.audio_ch", channel, &ret_value, NULL))
	{
		data->type = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 3;
	}

	if(nf_sysdb_get_key1("cam.C%d.cv_admin", channel, &ret_value, NULL))
	{
		data->admin = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 4;
	}

	if(nf_sysdb_get_key1("cam.C%d.cv_manager", channel, &ret_value, NULL))
	{
		data->manager = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 5;
	}

	if(nf_sysdb_get_key1("cam.C%d.cv_user", channel, &ret_value, NULL))
	{
		data->user = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 6;
	}

	if(nf_sysdb_get_key1("cam.C%d.cv_logoff", channel, &ret_value, NULL))
	{
		data->logoff = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 7;
	}

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/

static guint prvSetCameraData(CameraData data, guint channel)
{
	guint ret = 0;
	GValue set_value = {0,};

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.title);
	if(!nf_sysdb_set_key1("cam.C%d.title", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);


	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.covert);
	if(!nf_sysdb_set_key1("cam.C%d.covert", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.audio_ch);
	if(!nf_sysdb_set_key1("cam.C%d.audio_ch", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.admin);
	if(!nf_sysdb_set_key1("cam.C%d.cv_admin", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.manager);
	if(!nf_sysdb_set_key1("cam.C%d.cv_manager", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.user);
	if(!nf_sysdb_set_key1("cam.C%d.cv_user", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 7;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.logoff);
	if(!nf_sysdb_set_key1("cam.C%d.cv_logoff", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 8;
	}
	g_value_unset(&set_value);

	return ret;
}

guint DAL_set_camera_data(CameraData data, guint channel)
{
	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	ret = prvSetCameraData(data, channel);

//	nf_notify_fire_params("sysdb_cam_title", 0, 0, 0, 0);
//	nf_notify_fire_params("sysdb_covert", 0, 0, 0, 0);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END
	return ret;
}

guint DAL_set_camera_data_all(CameraData *data, guint ch_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for(i=0; i<ch_num; i++)
	{
		ret = prvSetCameraData(data[i], i);

		if(ret)	break;
	}

//	nf_notify_fire_params("sysdb_cam_title", 0, 0, 0, 0);
//	nf_notify_fire_params("sysdb_covert", 0, 0, 0, 0);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END
	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/

guint DAL_get_camtitle_data(CameraData *data, guint channel)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key1("cam.C%d.title", channel,  &ret_value, NULL))
	{
		g_stpcpy(data->title, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 1;
	}

	return ret;
}

static guint prvSetCamTitleData(CameraData data, guint channel)
{
	guint ret = 0;
	GValue set_value = {0,};

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.title);
	if(!nf_sysdb_set_key1("cam.C%d.title", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	return ret;
}

guint DAL_set_camtitle_data(CameraData data, guint channel)
{
	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	ret = prvSetCamTitleData(data, channel);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END
	return ret;
}

guint DAL_set_camtitle_data_all(CameraData *data, guint ch_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for(i=0; i<ch_num; i++)
	{
		ret = prvSetCamTitleData(data[i], i);

		if(ret)	break;
	}

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END
	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/

guint DAL_get_covert_data(CameraData *data, guint channel)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key1("cam.C%d.cv_admin", channel, &ret_value, NULL))
	{
		data->admin = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 1;
	}

	if(nf_sysdb_get_key1("cam.C%d.cv_manager", channel, &ret_value, NULL))
	{
		data->manager = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 2;
	}

	if(nf_sysdb_get_key1("cam.C%d.cv_user", channel, &ret_value, NULL))
	{
		data->user = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 3;
	}

	if(nf_sysdb_get_key1("cam.C%d.cv_logoff", channel, &ret_value, NULL))
	{
		data->logoff = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 4;
	}

	return ret;
}

static guint prvSetCovertData(CameraData data, guint channel)
{
	guint ret = 0;
	GValue set_value = {0,};

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.admin);
	if(!nf_sysdb_set_key1("cam.C%d.cv_admin", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.manager);
	if(!nf_sysdb_set_key1("cam.C%d.cv_manager", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.user);
	if(!nf_sysdb_set_key1("cam.C%d.cv_user", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.logoff);
	if(!nf_sysdb_set_key1("cam.C%d.cv_logoff", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	return ret;
}

guint DAL_set_covert_data(CameraData data, guint channel)
{
	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	ret = prvSetCovertData(data, channel);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END
	return ret;
}

guint DAL_set_covert_data_all(CameraData *data, guint ch_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for(i=0; i<ch_num; i++)
	{
		ret = prvSetCovertData(data[i], i);

		if(ret)	break;
	}

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END
	return ret;
}

gboolean DAL_get_camera_title(gchar *title, guint channel)
{
	GValue ret_value = {0,};


	if(nf_sysdb_get_key1("cam.C%d.title", channel,  &ret_value, NULL))
	{
		g_stpcpy(title, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return FALSE;
	}

	return TRUE;
}

gboolean DAL_is_covert(guint channel)
{
	GValue ret_value = {0,};
	gboolean covert = FALSE;

	if(nf_sysdb_get_key1("cam.C%d.covert", channel, &ret_value, NULL))
	{
		covert = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}

	return covert;
}

gboolean DAL_get_logoff_covert(gchar covert[33])
{
	guint i;
	GValue ret_value = {0,};

	if(covert == NULL)
		return FALSE;

	for(i=0; i<g_ch; i++) {
		if(nf_sysdb_get_key1("cam.C%d.cv_logoff", i, &ret_value, NULL))
		{
			if(g_value_get_boolean(&ret_value)) covert[i] = 0x31;		// '1'
			else								covert[i] = 0x30;		// '0'
			g_value_unset(&ret_value);
		}
		else
		{
			return FALSE;
		}
	}

	return TRUE;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
gboolean DAL_get_audio_all_ch(guint data[33])
{
	gint ch;
	GValue ret_value = {0,};

	for(ch=0; ch<32; ch++)
	{
		if(nf_sysdb_get_key1("cam.C%d.audio_ch", ch, &ret_value, NULL))
		{
			data[ch] = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			g_warning("%s : %s returns false\n", __FILE__, __FUNCTION__);

			return FALSE;
		}
	}

	return TRUE;
}




/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_color_data(ColorData *data, guint channel)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key1("cam.C%d.bright", channel,  &ret_value, NULL))
	{
		data->bright = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 1;

	if(nf_sysdb_get_key1("cam.C%d.contrast", channel,  &ret_value, NULL))
	{
		data->contrast = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 2;

	if(nf_sysdb_get_key1("cam.C%d.tint", channel,  &ret_value, NULL))
	{
		data->tint = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 3;

	if(nf_sysdb_get_key1("cam.C%d.color", channel,  &ret_value, NULL))
	{
		data->color = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 4;

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
static guint prvSetColorData(ColorData data, guint channel)
{
	guint ret = 0;
	GValue set_value = {0,};

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.bright);
	if(!nf_sysdb_set_key1("cam.C%d.bright", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.contrast);
	if(!nf_sysdb_set_key1("cam.C%d.contrast", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.tint);
	if(!nf_sysdb_set_key1("cam.C%d.tint", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.color);
	if(!nf_sysdb_set_key1("cam.C%d.color", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	return ret;
}

guint DAL_set_color_data(ColorData data, guint channel)
{
	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	ret = prvSetColorData(data, channel);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END
	return ret;
}

guint DAL_set_color_data_all(ColorData *data, guint ch_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for(i=0; i<ch_num; i++)
	{
		ret = prvSetColorData(data[i], i);

		if(ret)	break;
	}

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END
	return ret;
}


/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_advanced_data(AdvancedData *data, guint channel)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key1("cam.C%d.sharpness", channel,  &ret_value, NULL))
	{
		data->sharpness = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 1;

	if(nf_sysdb_get_key1("cam.C%d.exposure_mode", channel,  &ret_value, NULL))
	{
		data->exposure_mode = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 2;

	if(nf_sysdb_get_key1("cam.C%d.agc_gain", channel,  &ret_value, NULL))
	{
		data->agc_gain = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 3;

	if(nf_sysdb_get_key1("cam.C%d.shutter_speed", channel,  &ret_value, NULL))
	{
		data->shutter_speed = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 4;

	if(nf_sysdb_get_key1("cam.C%d.slow_shutter", channel,  &ret_value, NULL))
	{
		data->slow_shutter = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	return 5;

	if(nf_sysdb_get_key1("cam.C%d.max_agc", channel,  &ret_value, NULL))
	{
		data->max_agc = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	return 6;

	if(nf_sysdb_get_key1("cam.C%d.iris_control", channel,  &ret_value, NULL))
	{
		data->iris_control = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	return 7;

	if(nf_sysdb_get_key1("cam.C%d.blc_control", channel,  &ret_value, NULL))
	{
		data->blc_control = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	return 8;

	if(nf_sysdb_get_key1("cam.C%d.day_night_mode", channel,  &ret_value, NULL))
	{
		data->day_night_mode = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 9;

	if(nf_sysdb_get_key1("cam.C%d.day_night_duration", channel,  &ret_value, NULL))
	{
		data->day_night_duration = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 10;

	if(nf_sysdb_get_key1("cam.C%d.wb_mode", channel,  &ret_value, NULL))
	{
		data->wb_mode = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 11;

	if(nf_sysdb_get_key1("cam.C%d.mwb_mode", channel,  &ret_value, NULL))
	{
		data->mwb_mode = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 12;

	if(nf_sysdb_get_key1("cam.C%d.rotate", channel,  &ret_value, NULL))
	{
		data->rotate = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 13;

	if(nf_sysdb_get_key1("cam.C%d.antiflicker", channel,  &ret_value, NULL))
	{
		data->antiflicker = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 14;

	if(nf_sysdb_get_key1("cam.C%d.wdr_mode",channel, &ret_value, NULL))
	{
	    data->wdr_mode = g_value_get_uint(&ret_value);
	    g_value_unset(&ret_value);
	}
	else        ret = 15;

	if(nf_sysdb_get_key1("cam.C%d.wdr_level",channel, &ret_value, NULL))
	{
	    data->wdr_level = g_value_get_int(&ret_value);
	    g_value_unset(&ret_value);
	}
	else        ret =16;

	if(nf_sysdb_get_key1("cam.C%d.dnr_control",channel, &ret_value, NULL))
	{
	    data->dnr = g_value_get_uint(&ret_value);
	    g_value_unset(&ret_value);
	}
	else        ret =17;
	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
static guint prvSetAdvancedData(AdvancedData data, guint channel)
{
	guint ret = 0;
	GValue set_value = {0,};

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.sharpness);
	if(!nf_sysdb_set_key1("cam.C%d.sharpness", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.exposure_mode);
	if(!nf_sysdb_set_key1("cam.C%d.exposure_mode", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.agc_gain);
	if(!nf_sysdb_set_key1("cam.C%d.agc_gain", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.shutter_speed);
	if(!nf_sysdb_set_key1("cam.C%d.shutter_speed", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.slow_shutter);
	if(!nf_sysdb_set_key1("cam.C%d.slow_shutter", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);


	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.max_agc);
	if(!nf_sysdb_set_key1("cam.C%d.max_agc", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.iris_control);
	if(!nf_sysdb_set_key1("cam.C%d.iris_control", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 7;
	}
	g_value_unset(&set_value);


	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.blc_control);
	if(!nf_sysdb_set_key1("cam.C%d.blc_control", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 8;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.day_night_mode);
	if(!nf_sysdb_set_key1("cam.C%d.day_night_mode", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 9;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.day_night_duration);
	if(!nf_sysdb_set_key1("cam.C%d.day_night_duration", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 10;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.wb_mode);
	if(!nf_sysdb_set_key1("cam.C%d.wb_mode", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 11;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.mwb_mode);
	if(!nf_sysdb_set_key1("cam.C%d.mwb_mode", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 12;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.rotate);
	if(!nf_sysdb_set_key1("cam.C%d.rotate", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 13;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.antiflicker);
	if(!nf_sysdb_set_key1("cam.C%d.antiflicker", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 14;
	}
	g_value_unset(&set_value);

    g_value_init(&set_value, G_TYPE_UINT);
    g_value_set_uint(&set_value,data.wdr_mode);
    if(!nf_sysdb_set_key1("cam.C%d.wdr_mode", channel, &set_value, NULL))
    {
        g_value_unset(&set_value);
        DDEBUG_FAIL
        return 15;
    }
    g_value_unset(&set_value);

    g_value_init(&set_value, G_TYPE_INT);
    g_value_set_int(&set_value, data.wdr_level);
    if(!nf_sysdb_set_key1("cam.C%d.wdr_level",channel, &set_value, NULL))
    {
        g_value_unset(&set_value);
        DDEBUG_FAIL
        return 16;
    }

    g_value_init(&set_value, G_TYPE_UINT);
    g_value_set_uint(&set_value, data.dnr);
    if(!nf_sysdb_set_key1("cam.C%d.dnr_control",channel, &set_value, NULL))
    {
        g_value_unset(&set_value);
        DDEBUG_FAIL
        return 17;
    }

    g_value_init(&set_value, G_TYPE_UINT);
    g_value_set_uint(&set_value, data.adaptive_ir);
    if(!nf_sysdb_set_key1("cam.C%d.adaptive_ir",channel, &set_value, NULL))
    {
        g_value_unset(&set_value);
        DDEBUG_FAIL
        return 18;
    }

	return ret;
}

guint DAL_set_advanced_data(AdvancedData data, guint channel)
{
	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	ret = prvSetAdvancedData(data, channel);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END
	return ret;
}

guint DAL_set_advanced_data_all(AdvancedData *data, guint ch_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for(i=0; i<ch_num; i++)
	{
		ret = prvSetAdvancedData(data[i], i);

		if(ret)	break;
	}

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END
	return ret;
}


/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/

guint DAL_get_cam_poe_onoff(gboolean *onff, guint channel)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key1("cam.C%d.poe_on_off",channel, &ret_value, NULL))
	{
		*onff  = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
    else        ret = 1;

	return ret;
}

guint DAL_set_cam_poe_onoff(gboolean onoff, guint channel)
{
	guint ret = 0;
	GValue set_value = {0,};

//	DDEBUG_START
	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, onoff);
	if(!nf_sysdb_set_key1("cam.C%d.poe_on_off",channel, &set_value, NULL))
	{
		g_value_unset(&set_value);

		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	return ret ;
}

guint DAL_get_IPCamSetup_data(IPCamSetupData *data, guint channel)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key1("cam.C%d.bright", channel,  &ret_value, NULL))
	{
		data->bright = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 1;

	if(nf_sysdb_get_key1("cam.C%d.contrast", channel,  &ret_value, NULL))
	{
		data->contrast = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 2;

	if(nf_sysdb_get_key1("cam.C%d.tint", channel,  &ret_value, NULL))
	{
		data->tint = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 3;

	if(nf_sysdb_get_key1("cam.C%d.color", channel,  &ret_value, NULL))
	{
		data->color = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 4;

	if(nf_sysdb_get_key1("cam.C%d.sharpness", channel,  &ret_value, NULL))
	{
		data->sharpness = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 5;

	if(nf_sysdb_get_key1("cam.C%d.rotate", channel,  &ret_value, NULL))
	{
		data->rotate = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 6;

	if(nf_sysdb_get_key1("cam.C%d.focus_mode", channel,  &ret_value, NULL))
	{
		data->focus_mode = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 7;

	if(nf_sysdb_get_key1("cam.C%d.focus_default_speed", channel,  &ret_value, NULL))
	{
		data->focus_default_speed = g_value_get_int(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 8;

	if(nf_sysdb_get_key1("cam.C%d.focus_near_limit", channel,  &ret_value, NULL))
	{
		data->focus_near_limit = g_value_get_int(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 9;

	if(nf_sysdb_get_key1("cam.C%d.focus_far_limit", channel,  &ret_value, NULL))
	{
		data->focus_far_limit = g_value_get_int(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 10;

	if(nf_sysdb_get_key1("cam.C%d.focus_abposition", channel,  &ret_value, NULL))
	{
		data->focus_abposition = g_value_get_int(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 11;

	if(nf_sysdb_get_key1("cam.C%d.focus_abspeed", channel,  &ret_value, NULL))
	{
		data->focus_abspeed = g_value_get_int(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 12;

	if(nf_sysdb_get_key1("cam.C%d.focus_redistance", channel,  &ret_value, NULL))
	{
		data->focus_redistance = g_value_get_int(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 13;

	if(nf_sysdb_get_key1("cam.C%d.focus_respeed", channel,  &ret_value, NULL))
	{
		data->focus_respeed = g_value_get_int(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 14;

	if(nf_sysdb_get_key1("cam.C%d.focus_cospeed", channel,  &ret_value, NULL))
	{
		data->focus_cospeed = g_value_get_int(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 15;

	if(nf_sysdb_get_key1("cam.C%d.wb_mode", channel,  &ret_value, NULL))
	{
		data->wb_mode = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 16;

	if(nf_sysdb_get_key1("cam.C%d.wb_crgain", channel,  &ret_value, NULL))
	{
		data->wb_crgain = g_value_get_int(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 17;

	if(nf_sysdb_get_key1("cam.C%d.wb_cbgain", channel,  &ret_value, NULL))
	{
		data->wb_cbgain = g_value_get_int(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 18;

	if(nf_sysdb_get_key1("cam.C%d.mwb_mode", channel,  &ret_value, NULL))
	{
		data->mwb_mode = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 19;

	if(nf_sysdb_get_key1("cam.C%d.wdr_mode", channel,  &ret_value, NULL))
	{
		data->wdr_mode = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 20;

	if(nf_sysdb_get_key1("cam.C%d.wdr_level", channel,  &ret_value, NULL))
	{
		data->wdr_level = g_value_get_int(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 21;

	if(nf_sysdb_get_key1("cam.C%d.day_night_mode", channel,  &ret_value, NULL))
	{
		data->day_night_mode = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 22;

	if(nf_sysdb_get_key1("cam.C%d.day_night_duration", channel,  &ret_value, NULL))
	{
		data->day_night_duration = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 23;

	if(nf_sysdb_get_key1("cam.C%d.exposure_mode", channel,  &ret_value, NULL))
	{
		data->exposure_mode = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 24;

	if(nf_sysdb_get_key1("cam.C%d.exposure_priority", channel,  &ret_value, NULL))
	{
		data->exposure_priority = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 25;

	if(nf_sysdb_get_key1("cam.C%d.blc_control", channel,  &ret_value, NULL))
	{
		data->blc_control = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 26;

	if(nf_sysdb_get_key1("cam.C%d.blc_level", channel,  &ret_value, NULL))
	{
		data->blc_level = g_value_get_int(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 27;

	if(nf_sysdb_get_key1("cam.C%d.etime", channel,  &ret_value, NULL))
	{
		data->etime = g_value_get_int(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 28;

	if(nf_sysdb_get_key1("cam.C%d.min_etime", channel,  &ret_value, NULL))
	{
		data->min_etime = g_value_get_int(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 29;

	if(nf_sysdb_get_key1("cam.C%d.max_etime", channel,  &ret_value, NULL))
	{
		data->max_etime = g_value_get_int(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 30;

	if(nf_sysdb_get_key1("cam.C%d.slow_shutter", channel,  &ret_value, NULL))
	{
		data->slow_shutter = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	return 31;

	if(nf_sysdb_get_key1("cam.C%d.shutter_speed", channel,  &ret_value, NULL))
	{
		data->shutter_speed = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 32;

	if(nf_sysdb_get_key1("cam.C%d.gain", channel,  &ret_value, NULL))
	{
		data->gain = g_value_get_int(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 33;

	if(nf_sysdb_get_key1("cam.C%d.min_gain", channel,  &ret_value, NULL))
	{
		data->min_gain = g_value_get_int(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 34;

	if(nf_sysdb_get_key1("cam.C%d.max_gain", channel,  &ret_value, NULL))
	{
		data->max_gain = g_value_get_int(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 35;

	if(nf_sysdb_get_key1("cam.C%d.agc_gain", channel,  &ret_value, NULL))
	{
		data->agc_gain = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 36;

	if(nf_sysdb_get_key1("cam.C%d.max_agc", channel,  &ret_value, NULL))
	{
		data->max_agc = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	return 37;

	if(nf_sysdb_get_key1("cam.C%d.iris", channel,  &ret_value, NULL))
	{
		data->iris = g_value_get_int(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 38;

	if(nf_sysdb_get_key1("cam.C%d.min_iris", channel,  &ret_value, NULL))
	{
		data->min_iris = g_value_get_int(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 39;

	if(nf_sysdb_get_key1("cam.C%d.max_iris", channel,  &ret_value, NULL))
	{
		data->max_iris = g_value_get_int(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 40;

	if(nf_sysdb_get_key1("cam.C%d.iris_control", channel,  &ret_value, NULL))
	{
		data->iris_control = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	return 41;

	if(nf_sysdb_get_key1("cam.C%d.antiflicker", channel,  &ret_value, NULL))
	{
		data->antiflicker = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	return 42;

	if(nf_sysdb_get_key1("cam.C%d.max_shutter", channel,  &ret_value, NULL))
	{
		data->max_shutter = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	return 43;

    if(nf_sysdb_get_key1("cam.C%d.base_shutter", channel,  &ret_value, NULL))
	{
		data->base_shutter = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	return 44;


	if(nf_sysdb_get_key1("cam.C%d.dnr_control",channel, &ret_value, NULL))
	{
	    data->dnr = g_value_get_uint(&ret_value);
	    g_value_unset(&ret_value);
	}
	else    return 45;

	if(nf_sysdb_get_key1("cam.C%d.adaptive_ir",channel, &ret_value, NULL))
	{
	    data->adaptive_ir = g_value_get_uint(&ret_value);
	    g_value_unset(&ret_value);
	}
	else    return 46;

	if(nf_sysdb_get_key1("cam.C%d.dnn_dton",channel, &ret_value, NULL))
	{
	    data->dnn_sense_dton = g_value_get_uint(&ret_value);
	    g_value_unset(&ret_value);
	}
	else    return 47;

	if(nf_sysdb_get_key1("cam.C%d.dnn_ntod",channel, &ret_value, NULL))
	{
	    data->dnn_sense_ntod = g_value_get_uint(&ret_value);
	    g_value_unset(&ret_value);
	}
	else    return 48;

	if(nf_sysdb_get_key1("cam.C%d.hlc",channel, &ret_value, NULL))
	{
	    data->hlc = g_value_get_uint(&ret_value);
	    g_value_unset(&ret_value);
	}
	else    return 49;

	if(nf_sysdb_get_key1("cam.C%d.defog",channel, &ret_value, NULL))
	{
	    data->defog = g_value_get_uint(&ret_value);
	    g_value_unset(&ret_value);
	}
	else    return 50;

	if(nf_sysdb_get_key1("cam.C%d.focus_limit",channel, &ret_value, NULL))
	{
	    data->focus_limit= g_value_get_uint(&ret_value);
	    g_value_unset(&ret_value);
	}
	else    return 51;

	if(nf_sysdb_get_key1("cam.C%d.stabilizer",channel, &ret_value, NULL))
	{
	    data->stabilizer= g_value_get_uint(&ret_value);
	    g_value_unset(&ret_value);
	}
	else    return 52;

	if(nf_sysdb_get_key1("cam.C%d.ir_correction",channel, &ret_value, NULL))
	{
	    data->ir_correction= g_value_get_uint(&ret_value);
	    g_value_unset(&ret_value);
	}
	else    return 53;

	if(nf_sysdb_get_key1("cam.C%d.dnn_start_hour",channel, &ret_value, NULL))
	{
	    data->dnn_start_hour = g_value_get_uint(&ret_value);
	    g_value_unset(&ret_value);
	}
	else    return 54;

	if(nf_sysdb_get_key1("cam.C%d.dnn_start_min",channel, &ret_value, NULL))
	{
	    data->dnn_start_min = g_value_get_uint(&ret_value);
	    g_value_unset(&ret_value);
	}
	else    return 55;

	if(nf_sysdb_get_key1("cam.C%d.dnn_end_hour",channel, &ret_value, NULL))
	{
	    data->dnn_end_hour = g_value_get_uint(&ret_value);
	    g_value_unset(&ret_value);
	}
	else    return 56;

	if(nf_sysdb_get_key1("cam.C%d.dnn_end_min",channel, &ret_value, NULL))
	{
	    data->dnn_end_min = g_value_get_uint(&ret_value);
	    g_value_unset(&ret_value);
	}
	else    return 57;

	if(nf_sysdb_get_key1("cam.C%d.illumination_level_ctl",channel, &ret_value, NULL))
	{
	    data->illumination_control = g_value_get_uint(&ret_value);
	    g_value_unset(&ret_value);
	}
	else    return 58;

	if(nf_sysdb_get_key1("cam.C%d.illumination_level",channel, &ret_value, NULL))
	{
	    data->illumination_level = g_value_get_uint(&ret_value);
	    g_value_unset(&ret_value);
	}
	else    return 59;

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
static guint prvSetIPCamSetupData(IPCamSetupData data, guint channel)
{
	guint ret = 0;
	GValue set_value = {0,};

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.bright);
	if(!nf_sysdb_set_key1("cam.C%d.bright", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.contrast);
	if(!nf_sysdb_set_key1("cam.C%d.contrast", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.tint);
	if(!nf_sysdb_set_key1("cam.C%d.tint", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.color);
	if(!nf_sysdb_set_key1("cam.C%d.color", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.sharpness);
	if(!nf_sysdb_set_key1("cam.C%d.sharpness", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.rotate);
	if(!nf_sysdb_set_key1("cam.C%d.rotate", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.focus_mode);
	if(!nf_sysdb_set_key1("cam.C%d.focus_mode", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 7;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_INT);
	g_value_set_int(&set_value, data.focus_default_speed);
	if(!nf_sysdb_set_key1("cam.C%d.focus_default_speed", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 8;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_INT);
	g_value_set_int(&set_value, data.focus_near_limit);
	if(!nf_sysdb_set_key1("cam.C%d.focus_near_limit", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 9;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_INT);
	g_value_set_int(&set_value, data.focus_far_limit);
	if(!nf_sysdb_set_key1("cam.C%d.focus_far_limit", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 10;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_INT);
	g_value_set_int(&set_value, data.focus_abposition);
	if(!nf_sysdb_set_key1("cam.C%d.focus_abposition", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 11;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_INT);
	g_value_set_int(&set_value, data.focus_abspeed);
	if(!nf_sysdb_set_key1("cam.C%d.focus_abspeed", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 12;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_INT);
	g_value_set_int(&set_value, data.focus_redistance);
	if(!nf_sysdb_set_key1("cam.C%d.focus_redistance", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 13;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_INT);
	g_value_set_int(&set_value, data.focus_respeed);
	if(!nf_sysdb_set_key1("cam.C%d.focus_respeed", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 14;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_INT);
	g_value_set_int(&set_value, data.focus_cospeed);
	if(!nf_sysdb_set_key1("cam.C%d.focus_cospeed", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 15;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.wb_mode);
	if(!nf_sysdb_set_key1("cam.C%d.wb_mode", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 16;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_INT);
	g_value_set_int(&set_value, data.wb_crgain);
	if(!nf_sysdb_set_key1("cam.C%d.wb_crgain", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 17;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_INT);
	g_value_set_int(&set_value, data.wb_cbgain);
	if(!nf_sysdb_set_key1("cam.C%d.wb_cbgain", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 18;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.mwb_mode);
	if(!nf_sysdb_set_key1("cam.C%d.mwb_mode", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 19;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.wdr_mode);
	if(!nf_sysdb_set_key1("cam.C%d.wdr_mode", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 20;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_INT);
	g_value_set_int(&set_value, data.wdr_level);
	if(!nf_sysdb_set_key1("cam.C%d.wdr_level", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 21;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.day_night_mode);
	if(!nf_sysdb_set_key1("cam.C%d.day_night_mode", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 22;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.day_night_duration);
	if(!nf_sysdb_set_key1("cam.C%d.day_night_duration", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 23;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.exposure_mode);
	if(!nf_sysdb_set_key1("cam.C%d.exposure_mode", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 24;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.exposure_priority);
	if(!nf_sysdb_set_key1("cam.C%d.exposure_priority", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 25;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.blc_control);
	if(!nf_sysdb_set_key1("cam.C%d.blc_control", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 26;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_INT);
	g_value_set_int(&set_value, data.blc_level);
	if(!nf_sysdb_set_key1("cam.C%d.blc_level", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 27;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_INT);
	g_value_set_int(&set_value, data.etime);
	if(!nf_sysdb_set_key1("cam.C%d.etime", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 28;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_INT);
	g_value_set_int(&set_value, data.min_etime);
	if(!nf_sysdb_set_key1("cam.C%d.min_etime", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 29;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_INT);
	g_value_set_int(&set_value, data.max_etime);
	if(!nf_sysdb_set_key1("cam.C%d.max_etime", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 30;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.slow_shutter);
	if(!nf_sysdb_set_key1("cam.C%d.slow_shutter", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 31;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.shutter_speed);
	if(!nf_sysdb_set_key1("cam.C%d.shutter_speed", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 32;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_INT);
	g_value_set_int(&set_value, data.gain);
	if(!nf_sysdb_set_key1("cam.C%d.gain", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 33;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_INT);
	g_value_set_int(&set_value, data.min_gain);
	if(!nf_sysdb_set_key1("cam.C%d.min_gain", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 34;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_INT);
	g_value_set_int(&set_value, data.max_gain);
	if(!nf_sysdb_set_key1("cam.C%d.max_gain", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 35;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.agc_gain);
	if(!nf_sysdb_set_key1("cam.C%d.agc_gain", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 36;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.max_agc);
	if(!nf_sysdb_set_key1("cam.C%d.max_agc", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 37;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_INT);
	g_value_set_int(&set_value, data.iris);
	if(!nf_sysdb_set_key1("cam.C%d.iris", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 38;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_INT);
	g_value_set_int(&set_value, data.min_iris);
	if(!nf_sysdb_set_key1("cam.C%d.min_iris", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 39;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_INT);
	g_value_set_int(&set_value, data.max_iris);
	if(!nf_sysdb_set_key1("cam.C%d.max_iris", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 40;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.iris_control);
	if(!nf_sysdb_set_key1("cam.C%d.iris_control", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 41;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.antiflicker);
	if(!nf_sysdb_set_key1("cam.C%d.antiflicker", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 42;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.max_shutter);
	if(!nf_sysdb_set_key1("cam.C%d.max_shutter", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 43;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.base_shutter);
	if(!nf_sysdb_set_key1("cam.C%d.base_shutter", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 44;
	}
	g_value_unset(&set_value);


	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value,data.dnr);
	if(!nf_sysdb_set_key1("cam.C%d.dnr_control",channel, &set_value, NULL))
	{
	    g_value_unset(&set_value);
	    DDEBUG_FAIL
	    return 45;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.adaptive_ir);
	if(!nf_sysdb_set_key1("cam.C%d.adaptive_ir", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 46;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.dnn_sense_ntod);
	if(!nf_sysdb_set_key1("cam.C%d.dnn_ntod", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 47;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.dnn_sense_dton);
	if(!nf_sysdb_set_key1("cam.C%d.dnn_dton", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 48;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.defog);
	if(!nf_sysdb_set_key1("cam.C%d.defog", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 49;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.hlc);
	if(!nf_sysdb_set_key1("cam.C%d.hlc", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 50;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.focus_limit);
	if(!nf_sysdb_set_key1("cam.C%d.focus_limit", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 51;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.ir_correction);
	if(!nf_sysdb_set_key1("cam.C%d.ir_correction", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 52;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.stabilizer);
	if(!nf_sysdb_set_key1("cam.C%d.stabilizer", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 53;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.dnn_start_hour);
	if(!nf_sysdb_set_key1("cam.C%d.dnn_start_hour", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 54;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.dnn_start_min);
	if(!nf_sysdb_set_key1("cam.C%d.dnn_start_min", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 55;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.dnn_end_hour);
	if(!nf_sysdb_set_key1("cam.C%d.dnn_end_hour", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 56;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.dnn_end_min);
	if(!nf_sysdb_set_key1("cam.C%d.dnn_end_min", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 57;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.illumination_control);
	if(!nf_sysdb_set_key1("cam.C%d.illumination_level_ctl", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 58;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.illumination_level);
	if(!nf_sysdb_set_key1("cam.C%d.illumination_level", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 59;
	}
	g_value_unset(&set_value);

	return ret;
}

guint DAL_set_IPCamSetup_data(IPCamSetupData data, guint channel)
{
	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	ret = prvSetIPCamSetupData(data, channel);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END
	return ret;
}

guint DAL_set_IPCamSetup_data_all(IPCamSetupData *data, guint ch_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for(i=0; i<ch_num; i++)
	{
		ret = prvSetIPCamSetupData(data[i], i);

		if(ret)	break;
	}

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END
	return ret;
}



/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/

gboolean DAL_get_cam_reset_msg_check()
{
	gboolean ret = FALSE;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("cam.cam_reset_msg_check", &ret_value, NULL))
	{
		ret = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}

	return ret;
}

guint DAL_set_cam_reset_msg_check(gboolean check_state)
{
	guint ret = 0;
	GValue set_value = {0,};

	DDEBUG_START
	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, check_state);

	nf_sysdb_set_key0("cam.cam_reset_msg_check", &set_value, NULL);

	if(!nf_sysdb_set_key0("cam.cam_reset_msg_check", &set_value, NULL))
	{
		g_value_unset(&set_value);

		DDEBUG_FAIL
		return 1;
	}

	g_value_unset(&set_value);
	DDEBUG_END

	return ret;

}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_ptz_data(PtzData *data, guint channel)
{
//	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key1("cam.ptz.P%d.addr", channel,  &ret_value, NULL))
	{
		data->addr = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	return 1;

	if(nf_sysdb_get_key1("cam.ptz.P%d.protocol", channel,  &ret_value, NULL))
	{
		data->proto = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	return 2;

	if(nf_sysdb_get_key1("cam.ptz.P%d.baud", channel,  &ret_value, NULL))
	{
		gint tmp;

		tmp = prvBaudrateToIndex(g_value_get_uint(&ret_value));
		if(tmp == BAUD_NONE)
		{
			g_value_unset(&ret_value);
			return 3;
		}
		else	data->baud = (guint)tmp;
		g_value_unset(&ret_value);
	}
	else	 	return 3;

	if(nf_sysdb_get_key1("cam.ptz.P%d.channel", channel,  &ret_value, NULL))
	{
		data->channel = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	return 4;

	if(nf_sysdb_get_key1("cam.ptz.P%d.model", channel,  &ret_value, NULL))
	{
		g_stpcpy(data->driver, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else	 	return 5;

	if(nf_sysdb_get_key1("cam.ptz.P%d.auto_focus", channel,  &ret_value, NULL))
	{
		data->autoFocus = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	return 6;

	if(nf_sysdb_get_key1("cam.ptz.P%d.auto_iris", channel,  &ret_value, NULL))
	{
		data->autoIris = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	return 7;

	if(nf_sysdb_get_key1("cam.ptz.P%d.pt_spd", channel,  &ret_value, NULL))
	{
		data->PTSpeed = prvPTZSpeedToIndex(g_value_get_uint(&ret_value));
		g_value_unset(&ret_value);
	}
	else	 	return 8;

	if(nf_sysdb_get_key1("cam.ptz.P%d.zoom_spd", channel,  &ret_value, NULL))
	{
		data->zoomSpeed = prvPTZSpeedToIndex(g_value_get_uint(&ret_value));
		g_value_unset(&ret_value);
	}
	else	 	return 9;

	if(nf_sysdb_get_key1("cam.ptz.P%d.focus_spd", channel,  &ret_value, NULL))
	{
		data->focusSpeed = prvPTZSpeedToIndex(g_value_get_uint(&ret_value));
		g_value_unset(&ret_value);
	}
	else	 	return 10;

	if(nf_sysdb_get_key1("cam.ptz.P%d.iris_spd", channel,  &ret_value, NULL))
	{
		data->irisSpeed = prvPTZSpeedToIndex(g_value_get_uint(&ret_value));
		g_value_unset(&ret_value);
	}
	else	 	return 11;

	if(nf_sysdb_get_key1("cam.ptz.P%d.mode", channel,  &ret_value, NULL))
	{
		data->mode = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	return 12;

	if(nf_sysdb_get_key1("cam.ptz.P%d.rs485", channel,  &ret_value, NULL))
	{
		data->rs485 = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	return 6;

	return 0;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_ptz_protocol_name(gchar *name, guint proto_num)
{
	GValue ret_value = {0,};

	if(nf_sysdb_get_key1("sys.info.ptz.P%d.name", proto_num, &ret_value, NULL))
	{
		g_stpcpy(name, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else	 return 0;

	return 1;
}

gboolean DAL_get_ptz_rs485_supported(guint channel)
{
	gboolean ret = FALSE;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key1("cam.ptz.P%d.rs485", channel,  &ret_value, NULL))
	{
		ret = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
static guint prvSetPtzData(PtzData data, guint channel)
{
	gchar strTemp[33];
//	guint ret = 0;
	guint tmp;
	GValue set_value = {0,};

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.addr);
	if(!nf_sysdb_set_key1("cam.ptz.P%d.addr", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.proto);
	if(!nf_sysdb_set_key1("cam.ptz.P%d.protocol", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	tmp = prvIndexToBaudrate(data.baud);
	if(!tmp)
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 3;
	}

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, tmp);
	if(!nf_sysdb_set_key1("cam.ptz.P%d.baud", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.channel);
	if(!nf_sysdb_set_key1("cam.ptz.P%d.channel", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.driver);
	if(!nf_sysdb_set_key1("cam.ptz.P%d.model", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.autoFocus);
	if(!nf_sysdb_set_key1("cam.ptz.P%d.auto_focus", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.autoIris);
	if(!nf_sysdb_set_key1("cam.ptz.P%d.auto_iris", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 7;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, prvIndexToPTZSpeed(data.PTSpeed));
	if(!nf_sysdb_set_key1("cam.ptz.P%d.pt_spd", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 8;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, prvIndexToPTZSpeed(data.zoomSpeed));
	if(!nf_sysdb_set_key1("cam.ptz.P%d.zoom_spd", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 9;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, prvIndexToPTZSpeed(data.focusSpeed));
	if(!nf_sysdb_set_key1("cam.ptz.P%d.focus_spd", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 10;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, prvIndexToPTZSpeed(data.irisSpeed));
	if(!nf_sysdb_set_key1("cam.ptz.P%d.iris_spd", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 11;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.mode);
	if(!nf_sysdb_set_key1("cam.ptz.P%d.mode", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 12;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.rs485);
	if(!nf_sysdb_set_key1("cam.ptz.P%d.rs485", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);

	return 0;
}

guint DAL_set_ptz_data(PtzData data, guint channel)
{
	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	ret = prvSetPtzData(data, channel);

//	nf_notify_fire_params("sysdb_ptz", 0, 0, 0, 0);
//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END
	return ret;
}

guint DAL_set_ptz_data_all(PtzData *data, guint ch_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for(i=0; i<ch_num; i++)
	{
		ret = prvSetPtzData(data[i], i);
		if(ret)	break;
	}

//	nf_notify_fire_params("sysdb_ptz", 0, 0, 0, 0);
//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END
	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_ptz_preset_data(PtzPresetData *data, guint channel)
{
	guint ret = 0;
	GValue ret_value = {0,};
	guint i;

	if(nf_sysdb_get_key1("cam.ptz.P%d.preset.PCNT", channel, &ret_value, NULL))
	{
		data->cnt = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 return 0;

	for(i=0; i<MAX_PRESET_COUNT; i++)
	{
    	if(nf_sysdb_get_key2("cam.ptz.P%d.preset.P%d.number", channel, i, &ret_value, NULL))
    	{
    		data->number[i] = g_value_get_uint(&ret_value);
    		g_value_unset(&ret_value);
    	}
		else	 	ret = i;

		if(nf_sysdb_get_key2("cam.ptz.P%d.preset.P%d.name", channel, i, &ret_value, NULL))
		{
			g_sprintf(data->name[i], "%s", g_value_get_string(&ret_value));
			g_value_unset(&ret_value);
		}
		else	 	ret = i;
	}

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
static guint prvSetPtzPresetData(PtzPresetData data, guint channel)
{
	guint ret = 0;
	GValue set_value = {0,};
	guint i;

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.cnt);
	if(!nf_sysdb_set_key1("cam.ptz.P%d.preset.PCNT", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 0;
	}
	g_value_unset(&set_value);

	for(i=0; i<MAX_PRESET_COUNT; i++)
	{
    	g_value_init(&set_value, G_TYPE_UINT);
    	g_value_set_uint(&set_value, data.number[i]);
    	if(!nf_sysdb_set_key2("cam.ptz.P%d.preset.P%d.number", channel, i, &set_value, NULL))
    	{
    		g_value_unset(&set_value);
    		DDEBUG_FAIL
    		return 1;
    	}
    	g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, data.name[i]);
		if(!nf_sysdb_set_key2("cam.ptz.P%d.preset.P%d.name", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 2;
		}
		g_value_unset(&set_value);
	}

	return ret;
}

guint DAL_set_ptz_preset_data(PtzPresetData data, guint channel)
{
	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	ret = prvSetPtzPresetData(data, channel);

	// don't need 'sysdb_ptz' notify..
	//nf_notify_fire_params("sysdb_ptz", 0, 0, 0, 0);
//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END
	return ret;
}

guint DAL_set_ptz_preset_data_all(PtzPresetData *data, guint ch_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for(i=0; i<ch_num; i++)
	{
		ret = prvSetPtzPresetData(data[i], i);
		if(ret)	break;
	}

	// don't need 'sysdb_ptz' notify..
	//nf_notify_fire_params("sysdb_ptz", 0, 0, 0, 0);
//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END
	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_ptz_scan_data(PtzScanData *data, guint channel)
{
	guint ret = 0;
	GValue ret_value = {0,};
	guint i;

	for(i=0; i<2; i++)
	{
    	if(nf_sysdb_get_key2("cam.ptz.P%d.scan.S%d.number", channel, i, &ret_value, NULL))
    	{
    		data->number[i] = g_value_get_uint(&ret_value);
    		g_value_unset(&ret_value);
    	}
		else	 	ret = i;

    	if(nf_sysdb_get_key2("cam.ptz.P%d.scan.S%d.dwell", channel, i, &ret_value, NULL))
    	{
    		data->dwell[i] = g_value_get_uint(&ret_value);
    		g_value_unset(&ret_value);
    	}
		else	 	ret = i;
	}

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
static guint prvSetPtzScanData(PtzScanData data, guint channel)
{
	guint ret = 0;
	GValue set_value = {0,};
	guint i;

	for(i=0; i<2; i++)
	{
    	g_value_init(&set_value, G_TYPE_UINT);
    	g_value_set_uint(&set_value, data.number[i]);
    	if(!nf_sysdb_set_key2("cam.ptz.P%d.scan.S%d.number", channel, i, &set_value, NULL))
    	{
    		g_value_unset(&set_value);
    		DDEBUG_FAIL
    		return 1;
    	}
    	g_value_unset(&set_value);

    	g_value_init(&set_value, G_TYPE_UINT);
    	g_value_set_uint(&set_value, data.dwell[i]);
    	if(!nf_sysdb_set_key2("cam.ptz.P%d.scan.S%d.dwell", channel, i, &set_value, NULL))
    	{
    		g_value_unset(&set_value);
    		DDEBUG_FAIL
    		return 2;
    	}
    	g_value_unset(&set_value);
	}

	return ret;
}

guint DAL_set_ptz_scan_data(PtzScanData data, guint channel)
{
	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	ret = prvSetPtzScanData(data, channel);

//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END
	return ret;
}

guint DAL_set_ptz_scan_data_all(PtzScanData *data, guint ch_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for(i=0; i<ch_num; i++)
	{
		ret = prvSetPtzScanData(data[i], i);
		if(ret)	break;
	}

//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END
	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_ptz_tour_data(PtzTourData *data, guint channel)
{
	guint ret = 0;
	GValue ret_value = {0,};
	guint i;

	if(nf_sysdb_get_key1("cam.ptz.P%d.tour.TCNT", channel, &ret_value, NULL))
	{
		data->cnt = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 0;

	for(i=0; i<MAX_TOUR_COUNT; i++)
	{
    	if(nf_sysdb_get_key2("cam.ptz.P%d.tour.T%d.number", channel, i, &ret_value, NULL))
    	{
    		data->number[i] = g_value_get_uint(&ret_value);
    		g_value_unset(&ret_value);
    	}
		else	 	ret = i;

    	if(nf_sysdb_get_key2("cam.ptz.P%d.tour.T%d.dwell", channel, i, &ret_value, NULL))
    	{
    		data->dwell[i] = g_value_get_uint(&ret_value);
    		g_value_unset(&ret_value);
    	}
		else	 	ret = i;
	}

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
static guint prvSetPtzTourData(PtzTourData data, guint channel)
{
	guint ret = 0;
	GValue set_value = {0,};
	guint i;

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.cnt);
	if(!nf_sysdb_set_key1("cam.ptz.P%d.tour.TCNT", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	for(i=0; i<MAX_PRESET_COUNT; i++)
	{
    	g_value_init(&set_value, G_TYPE_UINT);
    	g_value_set_uint(&set_value, data.number[i]);
    	if(!nf_sysdb_set_key2("cam.ptz.P%d.tour.T%d.number", channel, i, &set_value, NULL))
    	{
    		g_value_unset(&set_value);
    		DDEBUG_FAIL
    		return 1;
    	}
    	g_value_unset(&set_value);

    	g_value_init(&set_value, G_TYPE_UINT);
    	g_value_set_uint(&set_value, data.dwell[i]);
    	if(!nf_sysdb_set_key2("cam.ptz.P%d.tour.T%d.dwell", channel, i, &set_value, NULL))
    	{
    		g_value_unset(&set_value);
    		DDEBUG_FAIL
    		return 2;
    	}
    	g_value_unset(&set_value);
	}

	return ret;
}

guint DAL_set_ptz_tour_data(PtzTourData data, guint channel)
{
	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	ret = prvSetPtzTourData(data, channel);

//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END
	return ret;
}

guint DAL_set_ptz_tour_data_all(PtzTourData *data, guint ch_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for(i=0; i<ch_num; i++)
	{
		ret = prvSetPtzTourData(data[i], i);
		if(ret)	break;
	}

//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END
	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_motion_data(MotionData *data, guint channel)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key1("alarm.motion.M%d.sense", channel,  &ret_value, NULL))
	{
		data->sense = prvMotSenseToIndex(g_value_get_uint(&ret_value));
		g_value_unset(&ret_value);
	}
	else	 	ret = 1;

	if(nf_sysdb_get_key1("alarm.motion.M%d.area", channel,  &ret_value, NULL))
	{
		g_stpcpy(data->area, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else	 	ret = 2;

	return ret;
}

gboolean DAL_get_motionsensor_detect(guint channel)
{
	guint ret = 0;
	gint i;
	gboolean retval =  FALSE;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key1("alarm.motion.M%d.detect", channel, &ret_value, NULL))
	{
		retval = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return FALSE;
	}

	return retval;
}

guint DAL_get_motionsensor_data(MotionData *data, guint channel)
{
	guint ret = 0;
	gint i;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key1("alarm.motion.M%d.act", channel, &ret_value, NULL))
	{
		data->act = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 1;
	}

	if(nf_sysdb_get_key1("alarm.motion.M%d.detect", channel, &ret_value, NULL))
	{
		data->detect = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 2;
	}

	if(nf_sysdb_get_key1("alarm.motion.M%d.sense_d", channel, &ret_value, NULL))
	{
		data->sense_d = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 3;
	}

	if(nf_sysdb_get_key1("alarm.motion.M%d.sense_n", channel, &ret_value, NULL))
	{
		data->sense_n = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 4;
	}

	if(nf_sysdb_get_key1("alarm.motion.M%d.mini_d", channel, &ret_value, NULL))
	{
		data->mini_d = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 5;
	}

	if(nf_sysdb_get_key1("alarm.motion.M%d.mini_n", channel, &ret_value, NULL))
	{
		data->mini_n = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 6;
	}

	if(nf_sysdb_get_key1("alarm.motion.M%d.area", channel,  &ret_value, NULL))
	{
		g_stpcpy(data->area, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 7;
	}

	if(nf_sysdb_get_key1("alarm.motion.M%d.time_start", channel, &ret_value, NULL))
	{
		data->time_start = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 8;
	}

	if(nf_sysdb_get_key1("alarm.motion.M%d.time_end", channel, &ret_value, NULL))
	{
		data->time_end = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 9;
	}

	if(nf_sysdb_get_key1("alarm.motion.M%d.rect.RCNT", channel, &ret_value, NULL))
	{
		data->rect_cnt = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 10;
	}

	for(i=0; i<MAX_RECT_COUNT; i++) {
		if(nf_sysdb_get_key2("alarm.motion.M%d.rect.R%d.row1", channel, i, &ret_value, NULL))
		{
			data->rect[i].start_r = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 11;
		}

		if(nf_sysdb_get_key2("alarm.motion.M%d.rect.R%d.col1", channel, i, &ret_value, NULL))
		{
			data->rect[i].start_c = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 12;
		}

		if(nf_sysdb_get_key2("alarm.motion.M%d.rect.R%d.row2", channel, i, &ret_value, NULL))
		{
			data->rect[i].end_r = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 13;
		}

		if(nf_sysdb_get_key2("alarm.motion.M%d.rect.R%d.col2", channel, i, &ret_value, NULL))
		{
			data->rect[i].end_c = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 14;
		}
	}

	return TRUE;

}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/

guint DAL_set_motionsensor_data(MotionData data, guint channel)
{
	guint ret = 0;
	gint i;
	GValue set_value = {0,};

	DDEBUG_START

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.act);
	if(!nf_sysdb_set_key1("alarm.motion.M%d.act", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.detect);
	if(!nf_sysdb_set_key1("alarm.motion.M%d.detect", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.sense_d);
	if(!nf_sysdb_set_key1("alarm.motion.M%d.sense_d", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.sense_n);
	if(!nf_sysdb_set_key1("alarm.motion.M%d.sense_n", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.mini_d);
	if(!nf_sysdb_set_key1("alarm.motion.M%d.mini_d", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.mini_n);
	if(!nf_sysdb_set_key1("alarm.motion.M%d.mini_n", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);


	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.area);
	if(!nf_sysdb_set_key1("alarm.motion.M%d.area", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 7;
	}
	g_value_unset(&set_value);


	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.rect_cnt);
	if(!nf_sysdb_set_key1("alarm.motion.M%d.rect.RCNT", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 8;
	}
	g_value_unset(&set_value);


	for(i=0; i<MAX_RECT_COUNT; i++) {
		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.rect[i].start_r);
		if(!nf_sysdb_set_key2("alarm.motion.M%d.rect.R%d.row1", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 9;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.rect[i].start_c);
		if(!nf_sysdb_set_key2("alarm.motion.M%d.rect.R%d.col1", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 10;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.rect[i].end_r);
		if(!nf_sysdb_set_key2("alarm.motion.M%d.rect.R%d.row2", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 11;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.rect[i].end_c);
		if(!nf_sysdb_set_key2("alarm.motion.M%d.rect.R%d.col2", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 12;
		}
		g_value_unset(&set_value);
	}

	DDEBUG_END

	return ret;

}

static guint prvSetMotionData(MotionData data, guint channel)
{
	guint ret = 0;
	gint i;
	GValue set_value = {0,};

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, prvIndexToMotSense(data.sense));
	if(!nf_sysdb_set_key1("alarm.motion.M%d.sense", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.area);
	if(!nf_sysdb_set_key1("alarm.motion.M%d.area", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	///////////////////
	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.act);
	if(!nf_sysdb_set_key1("alarm.motion.M%d.act", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.detect);
	if(!nf_sysdb_set_key1("alarm.motion.M%d.detect", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.sense_d);
	if(!nf_sysdb_set_key1("alarm.motion.M%d.sense_d", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.sense_n);
	if(!nf_sysdb_set_key1("alarm.motion.M%d.sense_n", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.mini_d);
	if(!nf_sysdb_set_key1("alarm.motion.M%d.mini_d", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 7;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.mini_n);
	if(!nf_sysdb_set_key1("alarm.motion.M%d.mini_n", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 8;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.time_start);
	if(!nf_sysdb_set_key1("alarm.motion.M%d.time_start", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 9;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.time_end);
	if(!nf_sysdb_set_key1("alarm.motion.M%d.time_end", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 10;
	}
	g_value_unset(&set_value);

	///////////////////

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.rect_cnt);
	if(!nf_sysdb_set_key1("alarm.motion.M%d.rect.RCNT", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 11;
	}
	g_value_unset(&set_value);

	for(i=0; i<MAX_RECT_COUNT; i++) {
		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.rect[i].start_r);
		if(!nf_sysdb_set_key2("alarm.motion.M%d.rect.R%d.row1", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 12;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.rect[i].start_c);
		if(!nf_sysdb_set_key2("alarm.motion.M%d.rect.R%d.col1", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 13;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.rect[i].end_r);
		if(!nf_sysdb_set_key2("alarm.motion.M%d.rect.R%d.row2", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 14;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.rect[i].end_c);
		if(!nf_sysdb_set_key2("alarm.motion.M%d.rect.R%d.col2", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 15;
		}
		g_value_unset(&set_value);
	}

	return ret;
}

guint DAL_set_motion_data(MotionData data, guint channel)
{
	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_ALARM);

	ret = prvSetMotionData(data, channel);

//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ALARM, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_ALARM);

	DDEBUG_END
	return ret;
}

guint DAL_set_motion_data_all(MotionData *data, guint ch_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_ALARM);

	for(i=0; i<ch_num; i++)
	{
		ret = prvSetMotionData(data[i], i);
		if(ret)	break;
	}

//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ALARM, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_ALARM);

	DDEBUG_END
	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/

guint DAL_get_onvif_motion_data(OnvifMotionData *data, guint channel, gint area_type)
{
	GValue ret_value = {0,};
	guint ret = 0;
	gint i;


	if(nf_sysdb_get_key1("alarm.motion.M%d.area", channel,  &ret_value, NULL))
	{
		g_stpcpy(data->area, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else 		ret = 2;

	if (nf_sysdb_get_key1("alarm.motion.M%d.smart_motion", channel, &ret_value, NULL))
	{
		data->smart_motion = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else 		ret = 16;

	if (nf_sysdb_get_key1("alarm.motion.M%d.use_ai_alarmevt", channel, &ret_value, NULL))
	{
		data->use_ai_alarmevt = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else 		ret = 18;	

#if 1
	if(nf_sysdb_get_key1("alarm.motion.M%d.rect.RCNT", channel, &ret_value, NULL))
	{
		data->rect_cnt = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 10;
	}

	if(nf_sysdb_get_key1("alarm.motion.M%d.sense_d", channel, &ret_value, NULL))
	{
		data->cell_d.sense_d = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 3;
	}

	if(nf_sysdb_get_key1("alarm.motion.M%d.sense_n", channel, &ret_value, NULL))
	{
		data->cell_d.sense_n = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 4;
	}

	if(nf_sysdb_get_key1("alarm.motion.M%d.mini_d", channel, &ret_value, NULL))
	{
		data->cell_d.mini_d = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 5;
	}

	if(nf_sysdb_get_key1("alarm.motion.M%d.mini_n", channel, &ret_value, NULL))
	{
		data->cell_d.mini_n = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 6;
	}

	if(nf_sysdb_get_key1("alarm.motion.M%d.time_start", channel, &ret_value, NULL))
	{
		data->cell_d.time_start = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 8;
	}

	if(nf_sysdb_get_key1("alarm.motion.M%d.time_end", channel, &ret_value, NULL))
	{
		data->cell_d.time_end = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 9;
	}

	if(nf_sysdb_get_key1("alarm.motion.M%d.interval", channel, &ret_value, NULL))
	{
		data->cell_d.interval = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else 		ret = 9;


	for(i=0; i<MAX_RECT_COUNT; i++) {
		if(nf_sysdb_get_key2("alarm.motion.M%d.rect.R%d.row1", channel, i, &ret_value, NULL))
		{
			data->rect[i].start_r = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 12;
		}

		if(nf_sysdb_get_key2("alarm.motion.M%d.rect.R%d.col1", channel, i, &ret_value, NULL))
		{
			data->rect[i].start_c = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 13;
		}

		if(nf_sysdb_get_key2("alarm.motion.M%d.rect.R%d.row2", channel, i, &ret_value, NULL))
		{
			data->rect[i].end_r = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 14;
		}

		if(nf_sysdb_get_key2("alarm.motion.M%d.rect.R%d.col2", channel, i, &ret_value, NULL))
		{
			data->rect[i].end_c = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 15;
		}
	}

#else 	// support later
	switch(area_type) {
		case AREA_TYPE_NONE:
		case AREA_TYPE_CELL:
			{
				data->rect_cnt = 0;

				if(nf_sysdb_get_key1("alarm.motion.M%d.sense", channel, &ret_value, NULL))
				{
					data->cell_d.sense = g_value_get_uint(&ret_value);
					g_value_unset(&ret_value);
				}
				else 		ret = 8;

				if(nf_sysdb_get_key1("alarm.motion.M%d.interval", channel, &ret_value, NULL))
				{
					data->cell_d.interval = g_value_get_uint(&ret_value);
					g_value_unset(&ret_value);
				}
				else 		ret = 9;

				if(nf_sysdb_get_key1("alarm.motion.M%d.mini", channel, &ret_value, NULL))
				{
					data->cell_d.mini_block = g_value_get_uint(&ret_value);
					g_value_unset(&ret_value);
				}
				else 		ret = 11;
			}
			break;

		case AERA_TYPE_RECT:
			{
				if(nf_sysdb_get_key1("alarm.motion.M%d.rect.RCNT", channel, &ret_value, NULL))
				{
					data->rect_cnt = g_value_get_uint(&ret_value);
					g_value_unset(&ret_value);
				}
				else 		ret = 1;

				for(i=0; i<data->rect_cnt; i++) {
					// rect row/col
					if(nf_sysdb_get_key2("alarm.motion.M%d.rect.R%d.row1", channel, i, &ret_value, NULL))
					{
						data->rect_d[i].rect.start_r = g_value_get_uint(&ret_value);
						g_value_unset(&ret_value);
					}
					else 		ret = 3;

					if(nf_sysdb_get_key2("alarm.motion.M%d.rect.R%d.col1", channel, i, &ret_value, NULL))
					{
						data->rect_d[i].rect.start_c = g_value_get_uint(&ret_value);
						g_value_unset(&ret_value);
					}
					else 		ret = 4;

					if(nf_sysdb_get_key2("alarm.motion.M%d.rect.R%d.row2", channel, i, &ret_value, NULL))
					{
						data->rect_d[i].rect.end_r = g_value_get_uint(&ret_value);
						g_value_unset(&ret_value);
					}
					else 		ret = 5;

					if(nf_sysdb_get_key2("alarm.motion.M%d.rect.R%d.col2", channel, i, &ret_value, NULL))
					{
						data->rect_d[i].rect.end_c = g_value_get_uint(&ret_value);
						g_value_unset(&ret_value);
					}
					else 		ret = 6;


					// rect info
					if(nf_sysdb_get_key2("alarm.motion.M%d.rect.R%d.name", channel, i, &ret_value, NULL))
					{
						g_stpcpy(data->rect_d[i].name, g_value_get_string(&ret_value));
						g_value_unset(&ret_value);
					}
					else		ret = 7;

					if(nf_sysdb_get_key2("alarm.motion.M%d.rect.R%d.sense", channel, i, &ret_value, NULL))
					{
						data->rect_d[i].sense = g_value_get_uint(&ret_value);
						g_value_unset(&ret_value);
					}
					else 		ret = 8;

					if(nf_sysdb_get_key2("alarm.motion.M%d.rect.R%d.interval", channel, i, &ret_value, NULL))
					{
						data->rect_d[i].interval = g_value_get_uint(&ret_value);
						g_value_unset(&ret_value);
					}
					else 		ret = 9;

					if(nf_sysdb_get_key2("alarm.motion.M%d.rect.R%d.threshold", channel, i, &ret_value, NULL))
					{
						data->rect_d[i].threshold = g_value_get_uint(&ret_value);
						g_value_unset(&ret_value);
					}
					else 		ret = 10;

					if(nf_sysdb_get_key2("alarm.motion.M%d.rect.R%d.exclude", channel, i, &ret_value, NULL))
					{
						data->rect_d[i].exclude = g_value_get_boolean(&ret_value);
						g_value_unset(&ret_value);
					}
					else 		ret = 12;
				}
			}
			break;

		default:
			return ret;
	}
#endif

	return ret;
}

static guint prvSetOnvifMotionData(OnvifMotionData data, guint channel)
{
	gint i;
	GValue set_value = {0,};
	guchar buf[1401];


	memset(buf, 0x00, sizeof(buf));
	strncpy(buf, data.area, sizeof(data.area));

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);
	if(!nf_sysdb_set_key1("alarm.motion.M%d.area", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.smart_motion);
	if(!nf_sysdb_set_key1("alarm.motion.M%d.smart_motion", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 16;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.use_ai_alarmevt);
	if(!nf_sysdb_set_key1("alarm.motion.M%d.use_ai_alarmevt", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 18;
	}
	g_value_unset(&set_value);	

//	g_value_init(&set_value, G_TYPE_UINT);
//	g_value_set_uint(&set_value, data.method);
//	if(!nf_sysdb_set_key1("alarm.motion.M%d.method", channel, &set_value, NULL))
//	{
//		g_value_unset(&set_value);
//		DDEBUG_FAIL
//		return 34;
//	}
//	g_value_unset(&set_value);

//	g_value_init(&set_value, G_TYPE_UINT);
//	g_value_set_uint(&set_value, data.cell_d.col);
//	if(!nf_sysdb_set_key1("alarm.motion.M%d.area_col", channel, &set_value, NULL))
//	{
//		g_value_unset(&set_value);
//		DDEBUG_FAIL
//		return 34;
//	}
//	g_value_unset(&set_value);
//
//	g_value_init(&set_value, G_TYPE_UINT);
//	g_value_set_uint(&set_value, data.cell_d.row);
//	if(!nf_sysdb_set_key1("alarm.motion.M%d.area_row", channel, &set_value, NULL))
//	{
//		g_value_unset(&set_value);
//		DDEBUG_FAIL
//		return 34;
//	}
//	g_value_unset(&set_value);

#if 1
	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.rect_cnt);
	if(!nf_sysdb_set_key1("alarm.motion.M%d.rect.RCNT", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 8;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.cell_d.sense_d);
	if(!nf_sysdb_set_key1("alarm.motion.M%d.sense_d", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.cell_d.sense_n);
	if(!nf_sysdb_set_key1("alarm.motion.M%d.sense_n", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

//	g_value_init(&set_value, G_TYPE_UINT);
//	g_value_set_uint(&set_value, data.cell_d.sense_min);
//	if(!nf_sysdb_set_key1("alarm.motion.M%d.sense_min", channel, &set_value, NULL))
//	{
//		g_value_unset(&set_value);
//		DDEBUG_FAIL
//		return 23;
//	}
//	g_value_unset(&set_value);
//
//	g_value_init(&set_value, G_TYPE_UINT);
//	g_value_set_uint(&set_value, data.cell_d.sense_max);
//	if(!nf_sysdb_set_key1("alarm.motion.M%d.sense_max", channel, &set_value, NULL))
//	{
//		g_value_unset(&set_value);
//		DDEBUG_FAIL
//		return 24;
//	}
//	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.cell_d.mini_d);
	if(!nf_sysdb_set_key1("alarm.motion.M%d.mini_d", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.cell_d.mini_n);
	if(!nf_sysdb_set_key1("alarm.motion.M%d.mini_n", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.cell_d.time_start);
	if(!nf_sysdb_set_key1("alarm.motion.M%d.time_start", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 9;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.cell_d.time_end);
	if(!nf_sysdb_set_key1("alarm.motion.M%d.time_end", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 10;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.cell_d.interval);
	if(!nf_sysdb_set_key1("alarm.motion.M%d.interval", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 9;
	}
	g_value_unset(&set_value);


	for(i=0; i<MAX_RECT_COUNT; i++) {
		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.rect[i].start_r);
		if(!nf_sysdb_set_key2("alarm.motion.M%d.rect.R%d.row1", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 10;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.rect[i].start_c);
		if(!nf_sysdb_set_key2("alarm.motion.M%d.rect.R%d.col1", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 12;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.rect[i].end_r);
		if(!nf_sysdb_set_key2("alarm.motion.M%d.rect.R%d.row2", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 13;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.rect[i].end_c);
		if(!nf_sysdb_set_key2("alarm.motion.M%d.rect.R%d.col2", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 14;
		}
		g_value_unset(&set_value);
	}
#else
	if(data.rect_cnt) {
		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.rect_cnt);
		if(!nf_sysdb_set_key1("alarm.motion.M%d.rect.RCNT", channel, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 1;
		}
		g_value_unset(&set_value);

		for(i=0; i<data.rect_cnt; i++) {
			g_value_init(&set_value, G_TYPE_UINT);
			g_value_set_uint(&set_value, data.rect_d[i].rect.start_r);
			if(!nf_sysdb_set_key2("alarm.motion.M%d.rect.R%d.row1", channel, i, &set_value, NULL))
			{
				g_value_unset(&set_value);
				DDEBUG_FAIL
				return 3;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_UINT);
			g_value_set_uint(&set_value, data.rect_d[i].rect.start_c);
			if(!nf_sysdb_set_key2("alarm.motion.M%d.rect.R%d.col1", channel, i, &set_value, NULL))
			{
				g_value_unset(&set_value);
				DDEBUG_FAIL
				return 4;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_UINT);
			g_value_set_uint(&set_value, data.rect_d[i].rect.end_r);
			if(!nf_sysdb_set_key2("alarm.motion.M%d.rect.R%d.row2", channel, i, &set_value, NULL))
			{
				g_value_unset(&set_value);
				DDEBUG_FAIL
				return 5;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_UINT);
			g_value_set_uint(&set_value, data.rect_d[i].rect.end_c);
			if(!nf_sysdb_set_key2("alarm.motion.M%d.rect.R%d.col2", channel, i, &set_value, NULL))
			{
				g_value_unset(&set_value);
				DDEBUG_FAIL
				return 6;
			}
			g_value_unset(&set_value);



			////////////////////////
			g_value_init(&set_value, G_TYPE_STRING);
			g_value_set_string(&set_value, data.rect_d[i].name);
			if(!nf_sysdb_set_key2("alarm.motion.M%d.rect.R%d.name", channel, i, &set_value, NULL))
			{
				g_value_unset(&set_value);
				DDEBUG_FAIL
				return 7;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_UINT);
			g_value_set_uint(&set_value, data.rect_d[i].sense);
			if(!nf_sysdb_set_key2("alarm.motion.M%d.rect.R%d.sense", channel, i, &set_value, NULL))
			{
				g_value_unset(&set_value);
				DDEBUG_FAIL
				return 8;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_UINT);
			g_value_set_uint(&set_value, data.rect_d[i].interval);
			if(!nf_sysdb_set_key2("alarm.motion.M%d.rect.R%d.interval", channel, i, &set_value, NULL))
			{
				g_value_unset(&set_value);
				DDEBUG_FAIL
				return 9;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_UINT);
			g_value_set_uint(&set_value, data.rect_d[i].threshold);
			if(!nf_sysdb_set_key2("alarm.motion.M%d.rect.R%d.threshold", channel, i, &set_value, NULL))
			{
				g_value_unset(&set_value);
				DDEBUG_FAIL
				return 10;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data.rect_d[i].exclude);
			if(!nf_sysdb_set_key2("alarm.motion.M%d.rect.R%d.exclude", channel, i, &set_value, NULL))
			{
				g_value_unset(&set_value);
				DDEBUG_FAIL
				return 12;
			}
			g_value_unset(&set_value);
		}
	}else {
		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.cell_d.sense);
		if(!nf_sysdb_set_key1("alarm.motion.M%d.sense", channel, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 8;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.cell_d.interval);
		if(!nf_sysdb_set_key1("alarm.motion.M%d.interval", channel, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 9;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.cell_d.mini_block);
		if(!nf_sysdb_set_key1("alarm.motion.M%d.mini", channel, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 11;
		}
		g_value_unset(&set_value);

	}
#endif

		return 0;
}


guint DAL_set_onvif_motion_data(OnvifMotionData data, guint channel)
{
	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_ALARM);

	ret = prvSetOnvifMotionData(data, channel);

	nf_sysdb_unlock(NF_SYSDB_CATE_ALARM);

	DDEBUG_END

	return ret;

}

guint DAL_set_onvif_motion_data_all(OnvifMotionData *data, guint ch_mask)
{
	guint ret = 0;
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_ALARM);

	for(i=0; i<g_ch; i++)
	{
		if(ch_mask & (1 << i)) {
			ret = prvSetOnvifMotionData(data[i], i);
			if(ret)	break;
		}
	}

	nf_sysdb_unlock(NF_SYSDB_CATE_ALARM);

	DDEBUG_END
	return ret;
}

guint DAL_get_onvif_ipcam_data(OnvifMotionIPCam *data, guint channel)
{
	GValue ret_value = {0,};
	gint i;

	if(nf_sysdb_get_key1("alarm.motion.M%d.method", channel, &ret_value, NULL))
	{
		data->method = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 1;
	}

	if(nf_sysdb_get_key1("alarm.motion.M%d.area_col", channel, &ret_value, NULL))
	{
		data->col = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 2;
	}

	if(nf_sysdb_get_key1("alarm.motion.M%d.area_row", channel, &ret_value, NULL))
	{
		data->row = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 3;
	}

	if(nf_sysdb_get_key1("alarm.motion.M%d.sense_min", channel, &ret_value, NULL))
	{
		data->sense_min = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 4;
	}

	if(nf_sysdb_get_key1("alarm.motion.M%d.sense_max", channel, &ret_value, NULL))
	{
		data->sense_max = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 5;
	}

	if(nf_sysdb_get_key1("alarm.motion.M%d.smart_interest_obj", channel,  &ret_value, NULL))
	{
		g_stpcpy(data->smart_interest_obj, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 6;
	}

	if(nf_sysdb_get_key1("alarm.motion.M%d.smart_motion_option_size", channel,  &ret_value, NULL))
	{
		data->smart_motion_option_size = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 7;
	}

	for (i = 0; i < 15; i++)
	{
		if(nf_sysdb_get_key2("alarm.motion.M%d.smart_motion_options.S%d.name", channel, i, &ret_value, NULL))
		{
			g_stpcpy(data->smart_motion_options[i].name, g_value_get_string(&ret_value));
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 8;
		}

		if(nf_sysdb_get_key2("alarm.motion.M%d.smart_motion_options.S%d.enable", channel, i, &ret_value, NULL))
		{
			data->smart_motion_options[i].enable = g_value_get_int(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 9;
		}

		if(nf_sysdb_get_key2("alarm.motion.M%d.smart_motion_options.S%d.threshold", channel, i, &ret_value, NULL))
		{
			data->smart_motion_options[i].threshold = g_value_get_int(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 10;
		}
	}

	return 0;
}

guint DAL_set_onvif_ipcam_data(OnvifMotionIPCam data, guint channel)
{
	GValue set_value = {0,};
	gint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_ALARM);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.method);
	if(!nf_sysdb_set_key1("alarm.motion.M%d.method", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.col);
	if(!nf_sysdb_set_key1("alarm.motion.M%d.area_col", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.row);
	if(!nf_sysdb_set_key1("alarm.motion.M%d.area_row", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);


	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.sense_min);
	if(!nf_sysdb_set_key1("alarm.motion.M%d.sense_min", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.sense_max);
	if(!nf_sysdb_set_key1("alarm.motion.M%d.sense_max", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.smart_interest_obj);
	if(!nf_sysdb_set_key1("alarm.motion.M%d.smart_interest_obj", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.smart_motion_option_size);
	if(!nf_sysdb_set_key1("alarm.motion.M%d.smart_motion_option_size", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);

	for (i = 0; i < 15; i++)
	{
		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, data.smart_motion_options[i].name);
		if(!nf_sysdb_set_key2("alarm.motion.M%d.smart_motion_options.S%d.name", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 7;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_INT);
		g_value_set_int(&set_value, data.smart_motion_options[i].enable);
		if(!nf_sysdb_set_key2("alarm.motion.M%d.smart_motion_options.S%d.enable", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 8;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_INT);
		g_value_set_int(&set_value, data.smart_motion_options[i].threshold);
		if(!nf_sysdb_set_key2("alarm.motion.M%d.smart_motion_options.S%d.threshold", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 9;
		}
		g_value_unset(&set_value);
	}

	nf_sysdb_unlock(NF_SYSDB_CATE_ALARM);

	DDEBUG_END

	return 0;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/

gint DAL_get_ipcam_login_data(IPCamLoginData *data, gint ch)
{
	GValue ret_value = {0,};

	if(nf_sysdb_get_key1("cam.logininfo.L%d.id", ch, &ret_value, NULL))
	{
		g_stpcpy(data->id, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		return 1;

	if(nf_sysdb_get_key1("cam.logininfo.L%d.pwd", ch, &ret_value, NULL))
	{
		g_stpcpy(data->password, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		return 2;


	if(nf_sysdb_get_key1("cam.logininfo.L%d.mac", ch, &ret_value, NULL))
	{
		g_stpcpy(data->mac, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		return 3;

/*
	if(nf_sysdb_get_key1("cam.logininfo.L%d.ipaddr", ch, &ret_value, NULL))
	{
		data->ipaddr = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		return 4;

	if(nf_sysdb_get_key1("cam.logininfo.L%d.subnet", ch, &ret_value, NULL))
	{
		data->subnet = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		return 5;

	if(nf_sysdb_get_key1("cam.logininfo.L%d.gateway", ch, &ret_value, NULL))
	{
		data->gateway = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		return 6;

	if(nf_sysdb_get_key1("cam.logininfo.L%d.dns1", ch, &ret_value, NULL))
	{
		data->dns1 = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		return 7;

	if(nf_sysdb_get_key1("cam.logininfo.L%d.dns2", ch, &ret_value, NULL))
	{
		data->dns2 = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		return 8;
*/

	if(nf_sysdb_get_key1("cam.logininfo.L%d.http_port", ch, &ret_value, NULL))
	{
		data->http_port = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		return 9;


	if(nf_sysdb_get_key1("cam.logininfo.L%d.rtsp_port", ch, &ret_value, NULL))
	{
		data->rtsp_port = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		return 10;


	if(nf_sysdb_get_key1("cam.logininfo.L%d.hostname", ch, &ret_value, NULL))
	{
		g_stpcpy(data->hostname, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		return 11;


	if(nf_sysdb_get_key1("cam.logininfo.L%d.vcam", ch, &ret_value, NULL))
	{
	    data->virtual_camera = g_value_get_uint(&ret_value);
	    g_value_unset(&ret_value);
	}
	else        return 12;

	if(nf_sysdb_get_key1("cam.logininfo.L%d.vcam_cnt", ch, &ret_value, NULL))
	{
	    data->vcam_cnt = g_value_get_uint(&ret_value);
	    g_value_unset(&ret_value);
	}
	else        return 13;

	if(nf_sysdb_get_key1("cam.logininfo.L%d.rtsp_addr_main",ch, &ret_value, NULL))
	{
	    g_stpcpy(data->vcam_rtsp_addr[0], g_value_get_string(&ret_value));
	    g_value_unset(&ret_value);
	}
	else        return 14;

	if(nf_sysdb_get_key1("cam.logininfo.L%d.rtsp_addr_second", ch, &ret_value, NULL))
	{
	    g_stpcpy(data->vcam_rtsp_addr[1], g_value_get_string(&ret_value));
	    g_value_unset(&ret_value);
	}
	else        return 15;

	if(nf_sysdb_get_key1("cam.C%d.model_nm", ch, &ret_value, NULL))
	{
	    g_stpcpy(data->model, g_value_get_string(&ret_value));
	    g_value_unset(&ret_value);
	}
	else        return 16;

	if(nf_sysdb_get_key1("cam.logininfo.L%d.ethernet", ch, &ret_value, NULL))
	{
		data->ethernet = g_value_get_uint(&ret_value);
	    g_value_unset(&ret_value);
	}
	else        return 17;

	return 0;
}

gint DAL_set_ipcam_login_data(IPCamLoginData data, gint ch)
{
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.id);
	if(!nf_sysdb_set_key1("cam.logininfo.L%d.id", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.password);
	if(!nf_sysdb_set_key1("cam.logininfo.L%d.pwd", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);


	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.mac);
	if(!nf_sysdb_set_key1("cam.logininfo.L%d.mac", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);
/*
	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.ipaddr);
	if(!nf_sysdb_set_key1("cam.logininfo.L%d.ipaddr", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.subnet);
	if(!nf_sysdb_set_key1("cam.logininfo.L%d.subnet", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.gateway);
	if(!nf_sysdb_set_key1("cam.logininfo.L%d.gateway", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.dns1);
	if(!nf_sysdb_set_key1("cam.logininfo.L%d.dns1", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 7;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.dns2);
	if(!nf_sysdb_set_key1("cam.logininfo.L%d.dns2", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 8;
	}
	g_value_unset(&set_value);
*/

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.http_port);
	if(!nf_sysdb_set_key1("cam.logininfo.L%d.http_port", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 9;
	}
	g_value_unset(&set_value);


	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.rtsp_port);
	if(!nf_sysdb_set_key1("cam.logininfo.L%d.rtsp_port", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 10;
	}
	g_value_unset(&set_value);


	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.hostname);
	if(!nf_sysdb_set_key1("cam.logininfo.L%d.hostname", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 11;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.virtual_camera);
	if(!nf_sysdb_set_key1("cam.logininfo.L%d.vcam", ch, &set_value, NULL))
	{
	    g_value_unset(&set_value);
	    DDEBUG_FAIL
	    return 12;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.vcam_cnt);
	if(!nf_sysdb_set_key1("cam.logininfo.L%d.vcam_cnt", ch, &set_value, NULL))
	{
	    g_value_unset(&set_value);
	    DDEBUG_FAIL
	    return 13;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.vcam_rtsp_addr[0]);
	if(!nf_sysdb_set_key1("cam.logininfo.L%d.rtsp_addr_main",ch, &set_value, NULL))
	{
	    g_value_unset(&set_value);
	    DDEBUG_FAIL
	    return 14;
	}
	g_value_unset(&set_value);

    g_value_init(&set_value, G_TYPE_STRING);
    g_value_set_string(&set_value, data.vcam_rtsp_addr[1]);
    if(!nf_sysdb_set_key1("cam.logininfo.L%d.rtsp_addr_second", ch, &set_value, NULL))
    {
        g_value_unset(&set_value);
        DDEBUG_FAIL
        return 15;
    }
    g_value_unset(&set_value);

    g_value_init(&set_value, G_TYPE_STRING);
    g_value_set_string(&set_value, data.model);
    if(!nf_sysdb_set_key1("cam.C%d.model_nm", ch, &set_value, NULL))
    {
        DDEBUG_FAIL
		g_value_unset(&set_value);
        return 16;
    }
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.ethernet);
	if(!nf_sysdb_set_key1("cam.logininfo.L%d.ethernet", ch, &set_value, NULL))
	{
	    g_value_unset(&set_value);
	    DDEBUG_FAIL
	    return 17;
	}
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END

	return 0;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/

gint DAL_get_ipcam_install_opt_data(IPCamInstOptData *data)
{
	GValue ret_value = {0,};

	if (nf_sysdb_get_key0("cam.instopt.auto_scan", &ret_value, NULL))
	{
		data->auto_scan = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	
	return 0;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
gint DAL_set_ipcam_install_opt_data(IPCamInstOptData *data)
{
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->auto_scan);
	if(!nf_sysdb_set_key0("cam.instopt.auto_scan", &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);
	
	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
	
	DDEBUG_END

	return 0;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/

gint DAL_get_privacy_max_cnt()
{
	GValue ret_value = {0,};
	guint ret = 0;
	gint i;

	if (nf_sysdb_get_key0("cam.privacy.PCNT", &ret_value, NULL))
	{
		ret = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 0;
	}

	return ret;
}

guint DAL_get_privacy_data(PrivacyData *data, guint channel)
{
	GValue ret_value = {0,};
	gint i;

	if (nf_sysdb_get_key1("cam.privacy.P%d.act", channel, &ret_value, NULL))
	{
		data->act = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 1;
	}

	if (nf_sysdb_get_key1("cam.privacy.P%d.color", channel, &ret_value, NULL))
	{
		data->color = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 2;
	}

	if (nf_sysdb_get_key1("cam.privacy.P%d.area.ACNT", channel, &ret_value, NULL))
	{
		data->area_cnt = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 3;
	}

    for (i = 0; i < MAX_PRIVACY_CNT; i++)
    {
    	if (nf_sysdb_get_key2("cam.privacy.P%d.area.A%d.sx", channel, i, &ret_value, NULL))
    	{
    		data->area[i].sx = g_value_get_int(&ret_value);
    		g_value_unset(&ret_value);
    	}
    	else
    	{
    		DDEBUG_FAIL
    		return 4;
    	}

    	if (nf_sysdb_get_key2("cam.privacy.P%d.area.A%d.sy", channel, i, &ret_value, NULL))
    	{
    		data->area[i].sy = g_value_get_int(&ret_value);
    		g_value_unset(&ret_value);
    	}
    	else
    	{
    		DDEBUG_FAIL
    		return 5;
    	}

    	if (nf_sysdb_get_key2("cam.privacy.P%d.area.A%d.ex", channel, i, &ret_value, NULL))
    	{
    		data->area[i].ex = g_value_get_int(&ret_value);
    		g_value_unset(&ret_value);
    	}
    	else
    	{
    		DDEBUG_FAIL
    		return 6;
    	}

    	if (nf_sysdb_get_key2("cam.privacy.P%d.area.A%d.ey", channel, i, &ret_value, NULL))
    	{
    		data->area[i].ey = g_value_get_int(&ret_value);
    		g_value_unset(&ret_value);
    	}
    	else
    	{
    		DDEBUG_FAIL
    		return 7;
    	}
    }

	return 0;
}

static guint prvSetPrivacyData(PrivacyData data, guint channel)
{
	GValue set_value = {0,};
	gint i;

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.act);
	if(!nf_sysdb_set_key1("cam.privacy.P%d.act", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.color);
	if(!nf_sysdb_set_key1("cam.privacy.P%d.color", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.area_cnt);
	if(!nf_sysdb_set_key1("cam.privacy.P%d.area.ACNT", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

    for (i = 0; i < MAX_PRIVACY_CNT; i++)
    {
    	g_value_init(&set_value, G_TYPE_INT);
    	g_value_set_int(&set_value, data.area[i].sx);
    	if(!nf_sysdb_set_key2("cam.privacy.P%d.area.A%d.sx", channel, i, &set_value, NULL))
    	{
    		g_value_unset(&set_value);
    		DDEBUG_FAIL
    		return 4;
    	}
    	g_value_unset(&set_value);

    	g_value_init(&set_value, G_TYPE_INT);
    	g_value_set_int(&set_value, data.area[i].sy);
    	if(!nf_sysdb_set_key2("cam.privacy.P%d.area.A%d.sy", channel, i, &set_value, NULL))
    	{
    		g_value_unset(&set_value);
    		DDEBUG_FAIL
    		return 5;
    	}
    	g_value_unset(&set_value);

    	g_value_init(&set_value, G_TYPE_INT);
    	g_value_set_int(&set_value, data.area[i].ex);
    	if(!nf_sysdb_set_key2("cam.privacy.P%d.area.A%d.ex", channel, i, &set_value, NULL))
    	{
    		g_value_unset(&set_value);
    		DDEBUG_FAIL
    		return 6;
    	}
    	g_value_unset(&set_value);

    	g_value_init(&set_value, G_TYPE_INT);
    	g_value_set_int(&set_value, data.area[i].ey);
    	if(!nf_sysdb_set_key2("cam.privacy.P%d.area.A%d.ey", channel, i, &set_value, NULL))
    	{
    		g_value_unset(&set_value);
    		DDEBUG_FAIL
    		return 7;
    	}
    	g_value_unset(&set_value);
    }

	return 0;
}

guint DAL_set_privacy_data(PrivacyData data, guint channel)
{
	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	ret = prvSetPrivacyData(data, channel);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END

	return ret;
}

guint DAL_set_privacy_data_all(PrivacyData *data, guint ch_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for (i = 0; i < ch_num; i++)
	{
		ret = prvSetPrivacyData(data[i], i);

		if (ret) break;
	}

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END

	return ret;
}


/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/

gint DAL_get_roi_area_max_cnt()
{
	GValue ret_value = {0,};
	guint ret = 0;

	if (nf_sysdb_get_key0("cam.ROI.PCNT", &ret_value, NULL))
	{
		ret = g_value_get_int(&ret_value);
		g_value_unset(&ret_value);
	}

	return ret;
}

guint DAL_get_roi_data(ROIData *data, guint channel)
{
	GValue ret_value = {0,};
	gint i;
	guint ret = 0;


	if (nf_sysdb_get_key1("cam.ROI.C%d.mode", channel, &ret_value, NULL))
	{
		data->mode = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else    ret = 1;

	if (nf_sysdb_get_key1("cam.ROI.C%d.quality", channel, &ret_value, NULL))
	{
		data->quality = g_value_get_int(&ret_value);
		g_value_unset(&ret_value);
	}
	else    ret = 2;

    for (i = 0; i < MAX_ROI_AREA_CNT; i++)
    {
    	if (nf_sysdb_get_key2("cam.ROI.C%d.area.A%d.level", channel, i, &ret_value, NULL))
    	{
    		data->area[i].level = g_value_get_int(&ret_value);
    		g_value_unset(&ret_value);
    	}
    	else    ret = 3;

    	if (nf_sysdb_get_key2("cam.ROI.C%d.area.A%d.tx", channel, i, &ret_value, NULL))
    	{
    		data->area[i].tx = g_value_get_int(&ret_value);
    		g_value_unset(&ret_value);
    	}
    	else    ret = 4;

    	if (nf_sysdb_get_key2("cam.ROI.C%d.area.A%d.ty", channel, i, &ret_value, NULL))
    	{
    		data->area[i].ty = g_value_get_int(&ret_value);
    		g_value_unset(&ret_value);
    	}
    	else    ret = 5;

    	if (nf_sysdb_get_key2("cam.ROI.C%d.area.A%d.bx", channel, i, &ret_value, NULL))
    	{
    		data->area[i].bx = g_value_get_int(&ret_value);
    		g_value_unset(&ret_value);
    	}
    	else    ret = 6;

    	if (nf_sysdb_get_key2("cam.ROI.C%d.area.A%d.by", channel, i, &ret_value, NULL))
    	{
    		data->area[i].by = g_value_get_int(&ret_value);
    		g_value_unset(&ret_value);
    	}
    	else    ret = 7;
    }

	return ret;
}

static guint prvSetROIData(ROIData data, guint channel)
{
	GValue set_value = {0,};
	gint i;

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.mode);
	if(!nf_sysdb_set_key1("cam.ROI.C%d.mode", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_INT);
	g_value_set_int(&set_value, data.quality);
	if(!nf_sysdb_set_key1("cam.ROI.C%d.quality", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

    for (i = 0; i < MAX_ROI_AREA_CNT; i++)
    {
     	g_value_init(&set_value, G_TYPE_INT);
    	g_value_set_int(&set_value, data.area[i].level);
    	if(!nf_sysdb_set_key2("cam.ROI.C%d.area.A%d.level", channel, i, &set_value, NULL))
    	{
    		g_value_unset(&set_value);
    		DDEBUG_FAIL
    		return 3;
    	}
    	g_value_unset(&set_value);

       	g_value_init(&set_value, G_TYPE_INT);
    	g_value_set_int(&set_value, data.area[i].tx);
    	if(!nf_sysdb_set_key2("cam.ROI.C%d.area.A%d.tx", channel, i, &set_value, NULL))
    	{
    		g_value_unset(&set_value);
    		DDEBUG_FAIL
    		return 4;
    	}
    	g_value_unset(&set_value);

    	g_value_init(&set_value, G_TYPE_INT);
    	g_value_set_int(&set_value, data.area[i].ty);
    	if(!nf_sysdb_set_key2("cam.ROI.C%d.area.A%d.ty", channel, i, &set_value, NULL))
    	{
    		g_value_unset(&set_value);
    		DDEBUG_FAIL
    		return 5;
    	}
    	g_value_unset(&set_value);

    	g_value_init(&set_value, G_TYPE_INT);
    	g_value_set_int(&set_value, data.area[i].bx);
    	if(!nf_sysdb_set_key2("cam.ROI.C%d.area.A%d.bx", channel, i, &set_value, NULL))
    	{
    		g_value_unset(&set_value);
    		DDEBUG_FAIL
    		return 6;
    	}
    	g_value_unset(&set_value);

    	g_value_init(&set_value, G_TYPE_INT);
    	g_value_set_int(&set_value, data.area[i].by);
    	if(!nf_sysdb_set_key2("cam.ROI.C%d.area.A%d.by", channel, i, &set_value, NULL))
    	{
    		g_value_unset(&set_value);
    		DDEBUG_FAIL
    		return 7;
    	}
    	g_value_unset(&set_value);
    }

	return 0;
}

guint DAL_set_roi_data_all(ROIData *data, guint ch_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for (i = 0; i < ch_num; i++)
	{
		ret = prvSetROIData(data[i], i);

		if (ret) break;
	}

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END

	return ret;
}

static guint prvGetVCAPropData(VCAPropData *data, guint channel)
{
	GValue ret_value = {0,};
	gchar str[256];

	guint ret = 0;
	gint i;

	memset(str, 0x00, sizeof(str));

	if (nf_sysdb_get_key1("cam.vca.cfg.R%u.unit", channel, &ret_value, NULL))
	{
		data->unit = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 1;
	}

	if (nf_sysdb_get_key1("cam.vca.opt.R%u.en_shadowrm", channel, &ret_value, NULL))
	{
		data->en_shadowrm = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 2;
	}

	if (nf_sysdb_get_key1("cam.vca.opt.R%u.track_ref", channel, &ret_value, NULL))
	{
		data->track_ref = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 3;
	}

	if (nf_sysdb_get_key1("cam.vca.opt.R%u.min_objsize", channel, &ret_value, NULL))
	{
	    g_stpcpy(str, g_value_get_string(&ret_value));
	    str ? sscanf(str, "%hd %hd", &data->min_width3d, &data->min_height3d) : 0;

		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 4;
	}

	if (nf_sysdb_get_key1("cam.vca.opt.R%u.sw_obj_bb", channel, &ret_value, NULL))
	{
		data->sw_obj_bb = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 5;
	}

	if (nf_sysdb_get_key1("cam.vca.opt.R%u.sw_obj_tr", channel, &ret_value, NULL))
	{
		data->sw_obj_tr = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 6;
	}

	if (nf_sysdb_get_key1("cam.vca.opt.R%u.sw_obj_id", channel, &ret_value, NULL))
	{
		data->sw_obj_id = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 7;
	}

	if (nf_sysdb_get_key1("cam.vca.opt.R%u.sw_obj_s3d", channel, &ret_value, NULL))
	{
		data->sw_obj_s3d = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 8;
	}

	if (nf_sysdb_get_key1("cam.vca.opt.R%u.sw_obj_w3d", channel, &ret_value, NULL))
	{
		data->sw_obj_w3d = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 9;
	}

	if (nf_sysdb_get_key1("cam.vca.opt.R%u.sw_obj_h3d", channel, &ret_value, NULL))
	{
		data->sw_obj_h3d = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 10;
	}

	if (nf_sysdb_get_key1("cam.vca.opt.R%u.sw_rule", channel, &ret_value, NULL))
	{
		data->sw_rule = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 11;
	}

	if (nf_sysdb_get_key1("cam.vca.cfg.R%u.act", channel, &ret_value, NULL))
	{
		data->active = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 12;
	}

	if (nf_sysdb_get_key1("cam.vca.cfg.R%u.detect", channel, &ret_value, NULL))
	{
		data->detect = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 13;
	}

	if (nf_sysdb_get_key1("cam.vca.opt.R%u.en_roi", channel, &ret_value, NULL))
	{
		data->en_roi = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 14;
	}

	if (nf_sysdb_get_key1("cam.vca.opt.R%u.en_tamper", channel, &ret_value, NULL))
	{
		data->en_tamper = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 15;
	}

	if (nf_sysdb_get_key1("cam.vca.opt.R%u.en_snapshot", channel, &ret_value, NULL))
	{
		data->en_snapshot = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 16;
	}

	if(nf_sysdb_get_key1("cam.vca.opt.R%u.roi_xywh", channel, &ret_value, NULL))
	{
		if (g_value_get_string(&ret_value))
		{
			sscanf(g_value_get_string(&ret_value), "%hd %hd %hd %hd", &data->roi_rect.x, &data->roi_rect.y, &data->roi_rect.w, &data->roi_rect.h);
		}
		else
		{
			data->roi_rect.x = 120;
			data->roi_rect.y = 90;
			data->roi_rect.w = 3600;
			data->roi_rect.h = 1980;
		}

		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 17;
	}

	if (nf_sysdb_get_key1("cam.vca.opt.R%u.sw_obj_ar", channel, &ret_value, NULL))
	{
		data->sw_obj_ar = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 18;
	}

	if (nf_sysdb_get_key1("cam.vca.opt.R%u.sw_obj_tm", channel, &ret_value, NULL))
	{
		data->sw_obj_tm = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 19;
	}

	if (nf_sysdb_get_key1("cam.vca.opt.R%u.sw_rule_name", channel, &ret_value, NULL))
	{
		data->sw_rule_name = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 20;
	}

	if (nf_sysdb_get_key1("cam.vca.opt.R%u.sw_roi", channel, &ret_value, NULL))
	{
		data->sw_roi = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 21;
	}

	if (nf_sysdb_get_key1("cam.vca.opt.R%u.en_usecalib", channel, &ret_value, NULL))
	{
		data->en_usecalib = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 22;
	}

	return 0;
}

static guint prvGetVCAZoneData(VCAZoneData *data, guint channel)
{
	GValue ret_value = {0,};
	gchar buf[256];
	gchar *p, *last, *tokx, *toky;
	guint ret = 0;
	gint i, j;

	memset(buf, 0x00, sizeof(buf));

	if (nf_sysdb_get_key1("cam.vca.rule.R%u.nzones", channel, &ret_value, NULL))
	{
		data->cnt = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 1;
	}

	for (i = 0; i < IVCA_MAX_ZONES; i++)
	{
		if(nf_sysdb_get_key2("cam.vca.rule.R%u.Z%u.name", channel, i, &ret_value, NULL))
		{
			g_stpcpy(data->zone[i].name, g_value_get_string(&ret_value));
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 2;
		}

		if (nf_sysdb_get_key2("cam.vca.rule.R%u.Z%u.id", channel, i, &ret_value, NULL))
		{
			data->zone[i].id = g_value_get_int(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 3;
		}

		if (nf_sysdb_get_key2("cam.vca.rule.R%u.Z%u.type", channel, i, &ret_value, NULL))
		{
			data->zone[i].type = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 4;
		}

		if (nf_sysdb_get_key2("cam.vca.rule.R%u.Z%u.active", channel, i, &ret_value, NULL))
		{
			data->zone[i].active = g_value_get_boolean(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 5;
		}

		if (nf_sysdb_get_key2("cam.vca.rule.R%u.Z%u.enabled", channel, i, &ret_value, NULL))
		{
			data->zone[i].enabled = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 6;
		}

		if(nf_sysdb_get_key2("cam.vca.rule.R%u.Z%u.time_sarlf", channel, i, &ret_value, NULL))
		{
			if (g_value_get_string(&ret_value))
			{
				sscanf(g_value_get_string(&ret_value), "%hu %hu %hu %hu %hu",
					&data->zone[i].stop_time, &data->zone[i].abandon_time, &data->zone[i].remove_time, &data->zone[i].loiter_time, &data->zone[i].fall_time);
			}
			else
				data->zone[i].stop_time = data->zone[i].abandon_time = data->zone[i].remove_time = data->zone[i].loiter_time = data->zone[i].fall_time = 5;

			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 7;
		}

		if (nf_sysdb_get_key2("cam.vca.rule.R%u.Z%u.f_colorsens", channel, i, &ret_value, NULL))
		{
			data->zone[i].ecolor = (0x00ffffff & g_value_get_uint(&ret_value));
			data->zone[i].ecolor_sens = (0xff000000 & g_value_get_uint(&ret_value)) >> 24;
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 8;
		}

		if (nf_sysdb_get_key2("cam.vca.rule.R%u.Z%u.f_size", channel, i, &ret_value, NULL))
		{
			if (g_value_get_string(&ret_value))
			{
				sscanf(g_value_get_string(&ret_value), "%hu %hu %hu %hu",
					&data->zone[i].size_min[0], &data->zone[i].size_min[1], &data->zone[i].size_max[0], &data->zone[i].size_max[1]);
			}
			else
				data->zone[i].size_min[0] = data->zone[i].size_min[1] = data->zone[i].size_max[0] = data->zone[i].size_max[1] = 0;

			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 9;
		}

		if (nf_sysdb_get_key2("cam.vca.rule.R%u.Z%u.f_speed", channel, i, &ret_value, NULL))
		{
			if (g_value_get_string(&ret_value))
				sscanf(g_value_get_string(&ret_value), "%hu %hu", &data->zone[i].speed_min, &data->zone[i].speed_max);
			else
				data->zone[i].speed_min = data->zone[i].speed_max = 0;

			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 10;
		}

		if (nf_sysdb_get_key2("cam.vca.rule.R%u.Z%u.f_class", channel, i, &ret_value, NULL))
		{
			data->zone[i].eclass = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 11;
		}

		if (nf_sysdb_get_key2("cam.vca.rule.R%u.Z%u.d_color", channel, i, &ret_value, NULL))
		{
			data->zone[i].color = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 12;
		}

		if (nf_sysdb_get_key2("cam.vca.rule.R%u.Z%u.npts", channel, i, &ret_value, NULL))
		{
			data->zone[i].npts = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 13;
		}

		if (nf_sysdb_get_key2("cam.vca.rule.R%u.Z%u.pt", channel, i, &ret_value, NULL))
		{
			if (g_value_get_string(&ret_value))
			{
				g_stpcpy(buf, g_value_get_string(&ret_value));

				for (last = buf, j = 0; j < data->zone[i].npts; j++)
				{
					tokx = strtok_r(last, " \t,.", &last);
					if ( !tokx ) break;
					toky = strtok_r(last, " \t,.", &last);
					if ( !toky ) break;

					data->zone[i].pt[j].x = atoi(tokx);
					data->zone[i].pt[j].y = atoi(toky);
				}

				if (j < data->zone[i].npts)
				{
					j = 0;
					data->zone[i].npts = 0;
				}
			}

			for ( ; j < IVCA_MAX_PTSPERZONE; j++)
				data->zone[i].pt[j].x = data->zone[i].pt[j].y = 0;

			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 14;
		}

		if (nf_sysdb_get_key2("cam.vca.rule.R%u.Z%u.sensitivity", channel, i, &ret_value, NULL))
		{
			data->zone[i].sensitivity = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 15;
		}
	}

	return 0;
}

static guint prvGetVCACntrData(VCACntrData *data, guint channel)
{
	GValue ret_value = {0,};
	gchar buf[256];
	gchar *p, *last, *tokx, *toky;
	guint ret = 0;
	gint i;

	memset(buf, 0x00, sizeof(buf));

	if (nf_sysdb_get_key1("cam.vca.rule.R%u.ncounters", channel, &ret_value, NULL))
	{
		data->cnt = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 1;
	}

	for (i = 0; i < IVCA_MAX_CNTRS; i++)
	{
		if(nf_sysdb_get_key2("cam.vca.rule.R%u.C%u.name", channel, i, &ret_value, NULL))
		{
			g_stpcpy(data->cntr[i].name, g_value_get_string(&ret_value));
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 1;
		}

		if (nf_sysdb_get_key2("cam.vca.rule.R%u.C%u.id", channel, i, &ret_value, NULL))
		{
			data->cntr[i].id = g_value_get_int(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 2;
		}

		if (nf_sysdb_get_key2("cam.vca.rule.R%u.C%u.active", channel, i, &ret_value, NULL))
		{
			data->cntr[i].active = g_value_get_boolean(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 4;
		}

		if (nf_sysdb_get_key2("cam.vca.rule.R%u.C%u.enabled", channel, i, &ret_value, NULL))
		{
			data->cntr[i].enabled = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 5;
		}

		if(nf_sysdb_get_key2("cam.vca.rule.R%u.C%u.zid", channel, i, &ret_value, NULL))
		{
			if (g_value_get_string(&ret_value))
				sscanf(g_value_get_string(&ret_value), "%hd %hd", &data->cntr[i].zid_up, &data->cntr[i].zid_dn);
			else
				data->cntr[i].zid_up = data->cntr[i].zid_dn = -1;

			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 6;
		}

		if (nf_sysdb_get_key2("cam.vca.rule.R%u.C%u.e_value", channel, i, &ret_value, NULL))
		{
			data->cntr[i].evalue = g_value_get_int(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 7;
		}

		if (nf_sysdb_get_key2("cam.vca.rule.R%u.C%u.reset_alert", channel, i, &ret_value, NULL))
		{
			data->cntr[i].resetalert = g_value_get_boolean(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 8;
		}

		if (nf_sysdb_get_key2("cam.vca.rule.R%u.C%u.d_color", channel, i, &ret_value, NULL))
		{
			data->cntr[i].color = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 9;
		}

		if (nf_sysdb_get_key2("cam.vca.rule.R%u.C%u.pt", channel, i, &ret_value, NULL))
		{
			if (g_value_get_string(&ret_value))
			{
				sscanf(g_value_get_string(&ret_value), "%hd %hd %hd %hd %hd %hd %hd %hd",
							&data->cntr[i].pt[0].x, &data->cntr[i].pt[0].y, &data->cntr[i].pt[1].x, &data->cntr[i].pt[1].y,
							&data->cntr[i].pt[2].x, &data->cntr[i].pt[2].y, &data->cntr[i].pt[3].x, &data->cntr[i].pt[3].y);

			}
			else
				memset(data->cntr[i].pt, 0, sizeof(data->cntr[i].pt));

			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 10;
		}
	}

	return 0;
}

static guint prvGetVCACalbData(VCACalbData* data, guint channel)
{
    GValue ret_value = {0,};

	guint i;
	gchar str[256];

	VCACalb *t = data->calb;

	memset(str, 0x00, sizeof(str));

	if(nf_sysdb_get_key1("cam.vca.calib.R%u.ntargets", channel, &ret_value, NULL))
	{
		data->cnt = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 1;
	}

	for (i = 0; i < 32; i++, t++)
	{
    	if(nf_sysdb_get_key2("cam.vca.calib.R%u.T%u.pt", channel, i, &ret_value, NULL))
    	{
    		g_stpcpy(str, g_value_get_string(&ret_value));
    		str ? (guint)sscanf(str, "%hu %hu %hu %hu", &t->pt[0].x, &t->pt[0].y, &t->pt[1].x, &t->pt[1].y) : 0;
    		g_value_unset(&ret_value);
    	}
    	else
    	{
    		DDEBUG_FAIL
    		return 1;
    	}

    	if(nf_sysdb_get_key2("cam.vca.calib.R%u.T%u.height", channel, i, &ret_value, NULL))
    	{
    		t->height = g_value_get_uint(&ret_value);
    		g_value_unset(&ret_value);
    	}
    	else
    	{
    		DDEBUG_FAIL
    		return 2;
    	}

	}

	return 0;
}

static guint prvGetVCACalbResData(VCACalbResData* data, guint channel)
{
    GValue ret_value = {0,};

	guint i;
	gchar str[256];

	memset(str, 0x00, sizeof(str));

    if(nf_sysdb_get_key1("cam.vca.calib.R%u.focal", channel, &ret_value, NULL))
	{
		g_stpcpy(str, g_value_get_string(&ret_value));
		str ? (guint)sscanf(str, "%f", &data->focal) : 0;
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 1;
	}

	if(nf_sysdb_get_key1("cam.vca.calib.R%u.height", channel, &ret_value, NULL))
	{
		g_stpcpy(str, g_value_get_string(&ret_value));
	    str ? (guint)sscanf(str, "%f", &data->height) : 0;
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 1;
	}

	if(nf_sysdb_get_key1("cam.vca.calib.R%u.tilt", channel, &ret_value, NULL))
	{
		g_stpcpy(str, g_value_get_string(&ret_value));
	    str ? (guint)sscanf(str, "%f", &data->tilt) : 0;
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 1;
	}

	if(nf_sysdb_get_key1("cam.vca.calib.R%u.p_width", channel, &ret_value, NULL))
	{
		data->p_width = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 1;
	}


	if(nf_sysdb_get_key1("cam.vca.calib.R%u.p_height", channel, &ret_value, NULL))
	{
		data->p_height = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 1;
	}

	if(nf_sysdb_get_key1("cam.vca.calib.R%u.paramvalid", channel, &ret_value, NULL))
	{
		data->paramvalid = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 1;
	}

	return 0;
}

guint DAL_get_vca_data(VCAData *data, guint channel)
{
	prvGetVCAPropData(&data->prop, channel);
	prvGetVCAZoneData(&data->zonelist, channel);
	prvGetVCACntrData(&data->cntrlist, channel);
	prvGetVCACalbData(&data->calblist, channel);
	prvGetVCACalbResData(&data->calbres, channel);
	return 0;
}

guint DAL_get_vca_prop_data(VCAPropData *data, guint channel)
{
	prvGetVCAPropData(data, channel);
	return 0;
}

guint DAL_get_vca_zone_data(VCAZoneData *data, guint channel)
{
	prvGetVCAZoneData(data, channel);
	return 0;
}

guint DAL_get_vca_cntr_data(VCACntrData *data, guint channel)
{
	prvGetVCACntrData(data, channel);
	return 0;
}

guint DAL_get_vca_calb_data(VCACalbData *data, guint channel)
{
	prvGetVCACalbData(data, channel);
	return 0;
}

guint DAL_get_vca_calbres_data(VCACalbResData *data, guint channel)
{
	prvGetVCACalbResData(data, channel);
	return 0;
}

static guint prvSetVCAPropData(VCAPropData data, guint channel)
{
	gint i;
	gchar buf[256];//64
	GValue set_value = {0,};
	guint ret = 0;

	memset(buf, 0x00, sizeof(buf));

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.unit);
	if(!nf_sysdb_set_key1("cam.vca.cfg.R%u.unit", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.en_shadowrm);
	if(!nf_sysdb_set_key1("cam.vca.opt.R%u.en_shadowrm", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.track_ref);
	if(!nf_sysdb_set_key1("cam.vca.opt.R%u.track_ref", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

    sprintf(buf, "%hu %hu", data.min_width3d, data.min_height3d);
	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

    g_message("%s, %d, %s", __FUNCTION__, __LINE__, buf);

	if(!nf_sysdb_set_key1("cam.vca.opt.R%u.min_objsize", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.sw_obj_bb);
	if(!nf_sysdb_set_key1("cam.vca.opt.R%u.sw_obj_bb", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.sw_obj_tr);
	if(!nf_sysdb_set_key1("cam.vca.opt.R%u.sw_obj_tr", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);
	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.sw_obj_id);
	if(!nf_sysdb_set_key1("cam.vca.opt.R%u.sw_obj_id", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 7;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.sw_obj_s3d);
	if(!nf_sysdb_set_key1("cam.vca.opt.R%u.sw_obj_s3d", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 8;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.sw_obj_w3d);
	if(!nf_sysdb_set_key1("cam.vca.opt.R%u.sw_obj_w3d", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 9;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.sw_obj_h3d);
	if(!nf_sysdb_set_key1("cam.vca.opt.R%u.sw_obj_h3d", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 10;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.sw_rule);
	if(!nf_sysdb_set_key1("cam.vca.opt.R%u.sw_rule", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 11;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.active);
	if(!nf_sysdb_set_key1("cam.vca.cfg.R%u.act", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 12;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.detect);
	if(!nf_sysdb_set_key1("cam.vca.cfg.R%u.detect", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 13;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.en_roi);
	if(!nf_sysdb_set_key1("cam.vca.opt.R%u.en_roi", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 14;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.en_tamper);
	if(!nf_sysdb_set_key1("cam.vca.opt.R%u.en_tamper", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 15;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.en_snapshot);
	if(!nf_sysdb_set_key1("cam.vca.opt.R%u.en_snapshot", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 16;
	}
	g_value_unset(&set_value);

	g_sprintf(buf, "%hu %hu %hu %hu", data.roi_rect.x, data.roi_rect.y, data.roi_rect.w, data.roi_rect.h);
	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);
	if(!nf_sysdb_set_key1("cam.vca.opt.R%u.roi_xywh", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 17;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.sw_obj_ar);
	if(!nf_sysdb_set_key1("cam.vca.opt.R%u.sw_obj_ar", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 18;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.sw_obj_tm);
	if(!nf_sysdb_set_key1("cam.vca.opt.R%u.sw_obj_tm", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 19;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.sw_rule_name);
	if(!nf_sysdb_set_key1("cam.vca.opt.R%u.sw_rule_name", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 20;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.sw_roi);
	if(!nf_sysdb_set_key1("cam.vca.opt.R%u.sw_roi", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 21;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.en_usecalib);
	if(!nf_sysdb_set_key1("cam.vca.opt.R%u.en_usecalib", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 22;
	}
	g_value_unset(&set_value);

	return 0;
}

static guint prvSetVCAZoneData(VCAZoneData data, guint channel)
{
	gint i, j;
	gchar buf[256];
	gchar *p;
	GValue set_value = {0,};

	memset(buf, 0x00, sizeof(buf));

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.cnt);
	if(!nf_sysdb_set_key1("cam.vca.rule.R%u.nzones", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	for (i = 0; i < IVCA_MAX_ZONES; i++)
	{
		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, data.zone[i].name);
		if(!nf_sysdb_set_key2("cam.vca.rule.R%u.Z%u.name", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 2;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_INT);
		g_value_set_int(&set_value, data.zone[i].id);
		if(!nf_sysdb_set_key2("cam.vca.rule.R%u.Z%u.id", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 3;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.zone[i].type);
		if(!nf_sysdb_set_key2("cam.vca.rule.R%u.Z%u.type", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 4;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data.zone[i].active);
		if(!nf_sysdb_set_key2("cam.vca.rule.R%u.Z%u.active", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 5;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.zone[i].enabled);
		if(!nf_sysdb_set_key2("cam.vca.rule.R%u.Z%u.enabled", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 6;
		}
		g_value_unset(&set_value);

		g_sprintf(buf, "%hu %hu %hu %hu %hu",
			data.zone[i].stop_time, data.zone[i].abandon_time, data.zone[i].remove_time, data.zone[i].loiter_time, data.zone[i].fall_time);
		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, buf);
		if(!nf_sysdb_set_key2("cam.vca.rule.R%u.Z%u.time_sarlf", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 7;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.zone[i].ecolor + (data.zone[i].ecolor_sens << 24));
		if(!nf_sysdb_set_key2("cam.vca.rule.R%u.Z%u.f_colorsens", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 8;
		}
		g_value_unset(&set_value);

		g_sprintf(buf, "%hu %hu %hu %hu", data.zone[i].size_min[0], data.zone[i].size_min[1], data.zone[i].size_max[0], data.zone[i].size_max[1]);
		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, buf);
		if(!nf_sysdb_set_key2("cam.vca.rule.R%u.Z%u.f_size", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 9;
		}
		g_value_unset(&set_value);

		g_sprintf(buf, "%hu %hu", data.zone[i].speed_min, data.zone[i].speed_max);
		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, buf);
		if(!nf_sysdb_set_key2("cam.vca.rule.R%u.Z%u.f_speed", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 10;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.zone[i].eclass);
		if(!nf_sysdb_set_key2("cam.vca.rule.R%u.Z%u.f_class", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 11;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.zone[i].color);
		if(!nf_sysdb_set_key2("cam.vca.rule.R%u.Z%u.d_color", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 12;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.zone[i].npts);
		if(!nf_sysdb_set_key2("cam.vca.rule.R%u.Z%u.npts", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 13;
		}
		g_value_unset(&set_value);

		for (buf[0] = '\0', p = buf, j = 0; j < data.zone[i].npts; j++)
			p += sprintf(p, "%s%hd %hd", j ? " " : "", data.zone[i].pt[j].x, data.zone[i].pt[j].y);

		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, buf);
		if(!nf_sysdb_set_key2("cam.vca.rule.R%u.Z%u.pt", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 14;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.zone[i].sensitivity);
		if(!nf_sysdb_set_key2("cam.vca.rule.R%u.Z%u.sensitivity", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 15;
		}
		g_value_unset(&set_value);
	}

	return 0;
}

static guint prvSetVCACntrData(VCACntrData data, guint channel)
{
	gint i, j;
	gchar buf[256];
	gchar *p;
	GValue set_value = {0,};

	memset(buf, 0x00, sizeof(buf));

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.cnt);
	if(!nf_sysdb_set_key1("cam.vca.rule.R%u.ncounters", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	for (i = 0; i < IVCA_MAX_CNTRS; i++)
	{
		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, data.cntr[i].name);
		if(!nf_sysdb_set_key2("cam.vca.rule.R%u.C%u.name", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 2;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_INT);
		g_value_set_int(&set_value, data.cntr[i].id);
		if(!nf_sysdb_set_key2("cam.vca.rule.R%u.C%u.id", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 3;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data.cntr[i].active);
		if(!nf_sysdb_set_key2("cam.vca.rule.R%u.C%u.active", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 4;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.cntr[i].enabled);
		if(!nf_sysdb_set_key2("cam.vca.rule.R%u.C%u.enabled", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 5;
		}
		g_value_unset(&set_value);

		g_sprintf(buf, "%hd %hd", data.cntr[i].zid_up, data.cntr[i].zid_dn);
		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, buf);
		if(!nf_sysdb_set_key2("cam.vca.rule.R%u.C%u.zid", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 6;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_INT);
		g_value_set_int(&set_value, data.cntr[i].evalue);
		if(!nf_sysdb_set_key2("cam.vca.rule.R%u.C%u.e_value", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 7;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data.cntr[i].resetalert);
		if(!nf_sysdb_set_key2("cam.vca.rule.R%u.C%u.reset_alert", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 8;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.cntr[i].color);
		if(!nf_sysdb_set_key2("cam.vca.rule.R%u.C%u.d_color", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 9;
		}
		g_value_unset(&set_value);

		for (buf[0] = '\0', p = buf, j = 0; j < 4; j++)
			p += sprintf(p, "%s%hd %hd", j ? " " : "", data.cntr[i].pt[j].x, data.cntr[i].pt[j].y);

		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, buf);
		if(!nf_sysdb_set_key2("cam.vca.rule.R%u.C%u.pt", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 10;
		}
		g_value_unset(&set_value);
	}

	return 0;
}

static guint prvSetVCACalbData(VCACalbData data, guint channel)
{
	guint i, ebase;
	gchar buf[256];
	VCACalb *t = data.calb;
	GValue set_value = {0,};

	memset(buf, 0x00, sizeof(buf));

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.cnt);
	if(!nf_sysdb_set_key1("cam.vca.calib.R%u.ntargets", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
        return 7;
	}
    g_value_unset(&set_value);

	for (i = 0; i < 32; i++, t++)
	{
		ebase = 8 + i * 2;

		sprintf(buf, "%hu %hu %hu %hu", t->pt[0].x, t->pt[0].y, t->pt[1].x, t->pt[1].y);

    	g_value_init(&set_value, G_TYPE_STRING);
    	g_value_set_string(&set_value, buf);
    	if(!nf_sysdb_set_key2("cam.vca.calib.R%u.T%u.pt", channel, i, &set_value, NULL))
    	{
    		g_value_unset(&set_value);
    		DDEBUG_FAIL
            return ebase;
        }
        g_value_unset(&set_value);

    	g_value_init(&set_value, G_TYPE_UINT);
    	g_value_set_uint(&set_value, t->height);
    	if(!nf_sysdb_set_key2("cam.vca.calib.R%u.T%u.height", channel, i, &set_value, NULL))
    	{
    		g_value_unset(&set_value);
    		DDEBUG_FAIL
            return ebase + 1;
        }
        g_value_unset(&set_value);

	}

	return 0;
}

static guint prvSetVCACalbResData(VCACalbResData data, guint channel)
{
	guint i, ebase;
	gchar buf[256];
	GValue set_value = {0,};

	memset(buf, 0x00, sizeof(buf));

	sprintf(buf, "%f", data.focal);
	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);
	if(!nf_sysdb_set_key1("cam.vca.calib.R%u.focal", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
        return 1;
    }
    g_value_unset(&set_value);

	sprintf(buf, "%f", data.height);
	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);
	if(!nf_sysdb_set_key1("cam.vca.calib.R%u.height", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
        return 2;
    }
    g_value_unset(&set_value);

	sprintf(buf, "%f", data.tilt);
	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);
	if(!nf_sysdb_set_key1("cam.vca.calib.R%u.tilt", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
        return 3;
    }
    g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.p_width);
	if(!nf_sysdb_set_key1("cam.vca.calib.R%u.p_width", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
        return 4;
	}
    g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.p_height);
	if(!nf_sysdb_set_key1("cam.vca.calib.R%u.p_height", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
        return 5;
	}
    g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.paramvalid);
	if(!nf_sysdb_set_key1("cam.vca.calib.R%u.paramvalid", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
        return 6;
	}
    g_value_unset(&set_value);

	return 0;
}

guint DAL_set_vca_data(VCAData data, guint channel)
{
	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	prvSetVCAPropData(data.prop, channel);
	prvSetVCAZoneData(data.zonelist, channel);
	prvSetVCACntrData(data.cntrlist, channel);
	prvSetVCACalbData(data.calblist, channel);
	prvSetVCACalbResData(data.calbres, channel);

//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END

	return ret;
}

guint DAL_set_vca_prop_data(VCAPropData data, guint channel)
{
	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	ret = prvSetVCAPropData(data, channel);

//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END

	return ret;
}

guint DAL_set_vca_zone_data(VCAZoneData data, guint channel)
{
	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	ret = prvSetVCAZoneData(data, channel);

//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END

	return ret;
}

guint DAL_set_vca_cntr_data(VCACntrData data, guint channel)
{
	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	ret = prvSetVCACntrData(data, channel);

//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END

	return ret;
}

guint DAL_set_vca_calb_data(VCACalbData data, guint channel)
{
	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	ret = prvSetVCACalbData(data, channel);

//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END

	return ret;
}

guint DAL_set_vca_calbres_data(VCACalbResData data, guint channel)
{
	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	ret = prvSetVCACalbResData(data, channel);

//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END

	return ret;
}

guint DAL_set_vca_data_all(VCAData *data, guint ch_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for (i = 0; i < ch_num; i++)
	{
		ret = prvSetVCAPropData(data[i].prop, i);
		if (ret) break;

		ret = prvSetVCAZoneData(data[i].zonelist, i);
		if (ret) break;

		ret = prvSetVCACntrData(data[i].cntrlist, i);
		if (ret) break;

		ret = prvSetVCACalbData(data[i].calblist, i);
		if (ret) break;

		ret = prvSetVCACalbResData(data[i].calbres, i);
		if (ret) break;
	}

//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END

	return ret;
}

guint DAL_set_vca_prop_data_all(VCAPropData *data, guint ch_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for (i = 0; i < ch_num; i++)
	{
		ret = prvSetVCAPropData(data[i], i);
		if (ret) break;
	}

//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END

	return ret;
}

guint DAL_set_vca_zone_data_all(VCAZoneData *data, guint ch_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for (i = 0; i < ch_num; i++)
	{
		ret = prvSetVCAZoneData(data[i], i);
		if (ret) break;
	}

//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END

	return ret;
}

guint DAL_set_vca_cntr_data_all(VCACntrData *data, guint ch_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for (i = 0; i < ch_num; i++)
	{
		ret = prvSetVCACntrData(data[i], i);
		if (ret) break;
	}

//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END

	return ret;
}

guint DAL_set_vca_calb_data_all(VCACalbData *data, guint ch_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for (i = 0; i < ch_num; i++)
	{
		ret = prvSetVCACalbData(data[i], i);
		if (ret) break;
	}

//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END

	return ret;
}

guint DAL_set_vca_calbres_data_all(VCACalbResData *data, guint ch_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for (i = 0; i < ch_num; i++)
	{
		ret = prvSetVCACalbResData(data[i], i);
		if (ret) break;
	}

//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END

	return ret;
}

guint DAL_get_vca_schd_data_all(VCASchedData *data, guint ch_num)
{
	gchar key[64], *str;
	guint ch;

	memset(data, 0, sizeof(VCASchedData) * ch_num);

	for (ch = 0; ch < ch_num; ch++) {
		sprintf(key, "cam.vca.cfg.R%u.sched", ch);
		str = nf_sysdb_get_str_nocopy(key);
		if ( str )
			memcpy(data[ch].sched, str, 24*7);
	}
	return 0;
}	/* DAL_get_vca_schd_data_all(... */

static guint prvSetVCASchedData(VCASchedData data, guint ch)
{
	GValue set_value = {0};

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.sched);
	if(!nf_sysdb_set_key1("cam.vca.cfg.R%u.sched", ch, &set_value, NULL)) {
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	return FALSE;
}

guint DAL_set_vca_schd_data(VCASchedData data, guint channel)
{
	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	prvSetVCASchedData(data, channel);
//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
	return 0;
}

guint DAL_set_vca_schd_data_all(VCASchedData *data, guint ch_num)
{
	guint ch;

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for (ch = 0; ch < ch_num; ch++)
		prvSetVCASchedData(data[ch], ch);
//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
	return 0;
}


static guint prvGetDvaBxPropData(DvaBxPropData *data, guint channel)
{
	GValue ret_value = {0,};
	gchar str[256];

	guint ret = 0;
	gint i;
	
	memset(str, 0x00, sizeof(str));
	
	if (nf_sysdb_get_key1("cam.dvabx.cfg.R%u.en_engine", channel, &ret_value, NULL))
	{
		data->en_engine = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 1;
	}	
	
	if (nf_sysdb_get_key1("cam.dvabx.cfg.R%u.unit", channel, &ret_value, NULL))
	{
		data->unit = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 1;
	}

	if (nf_sysdb_get_key1("cam.dvabx.opt.R%u.en_shadowrm", channel, &ret_value, NULL))
	{
		data->en_shadowrm = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 2;
	}
	
	if (nf_sysdb_get_key1("cam.dvabx.opt.R%u.track_ref", channel, &ret_value, NULL))
	{
		data->track_ref = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 3;
	}
	
	if (nf_sysdb_get_key1("cam.dvabx.opt.R%u.min_objsize", channel, &ret_value, NULL))
	{
	    g_stpcpy(str, g_value_get_string(&ret_value));
	    str ? sscanf(str, "%hd %hd", &data->min_width3d, &data->min_height3d) : 0;
	    
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 4;
	}

	if (nf_sysdb_get_key1("cam.dvabx.opt.R%u.sw_obj_bb", channel, &ret_value, NULL))
	{
		data->sw_obj_bb = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 5;
	}

	if (nf_sysdb_get_key1("cam.dvabx.opt.R%u.sw_obj_tr", channel, &ret_value, NULL))
	{
		data->sw_obj_tr = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 6;
	}
	
	if (nf_sysdb_get_key1("cam.dvabx.opt.R%u.sw_obj_id", channel, &ret_value, NULL))
	{
		data->sw_obj_id = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 7;
	}
	
	if (nf_sysdb_get_key1("cam.dvabx.opt.R%u.sw_obj_s3d", channel, &ret_value, NULL))
	{
		data->sw_obj_s3d = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 8;
	}
	
	if (nf_sysdb_get_key1("cam.dvabx.opt.R%u.sw_obj_w3d", channel, &ret_value, NULL))
	{
		data->sw_obj_w3d = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 9;
	}

	if (nf_sysdb_get_key1("cam.dvabx.opt.R%u.sw_obj_h3d", channel, &ret_value, NULL))
	{
		data->sw_obj_h3d = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 10;
	}
	
	if (nf_sysdb_get_key1("cam.dvabx.opt.R%u.sw_rule", channel, &ret_value, NULL))
	{
		data->sw_rule = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 11;
	}

	if (nf_sysdb_get_key1("cam.dvabx.cfg.R%u.act", channel, &ret_value, NULL))
	{
		data->active = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 12;
	}

	if (nf_sysdb_get_key1("cam.dvabx.cfg.R%u.detect", channel, &ret_value, NULL))
	{
		data->detect = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 13;
	}

	if (nf_sysdb_get_key1("cam.dvabx.opt.R%u.en_roi", channel, &ret_value, NULL))
	{
		data->en_roi = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 14;
	}

	if (nf_sysdb_get_key1("cam.dvabx.opt.R%u.en_tamper", channel, &ret_value, NULL))
	{
		data->en_tamper = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 15;
	}

	if (nf_sysdb_get_key1("cam.dvabx.opt.R%u.en_snapshot", channel, &ret_value, NULL))
	{
		data->en_snapshot = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 16;
	}	

	if(nf_sysdb_get_key1("cam.dvabx.opt.R%u.roi_xywh", channel, &ret_value, NULL))
	{
		if (g_value_get_string(&ret_value))
		{
			sscanf(g_value_get_string(&ret_value), "%hd %hd %hd %hd", &data->roi_rect.x, &data->roi_rect.y, &data->roi_rect.w, &data->roi_rect.h);
		}
		else
		{
			data->roi_rect.x = 120;
			data->roi_rect.y = 90;
			data->roi_rect.w = 3600;
			data->roi_rect.h = 1980;
		}

		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 17;
	}	

	if (nf_sysdb_get_key1("cam.dvabx.opt.R%u.sw_obj_ar", channel, &ret_value, NULL))
	{
		data->sw_obj_ar = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 18;
	}	

	if (nf_sysdb_get_key1("cam.dvabx.opt.R%u.sw_obj_tm", channel, &ret_value, NULL))
	{
		data->sw_obj_tm = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 19;
	}	

	if (nf_sysdb_get_key1("cam.dvabx.opt.R%u.sw_rule_name", channel, &ret_value, NULL))
	{
		data->sw_rule_name = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 20;
	}		

	if (nf_sysdb_get_key1("cam.dvabx.opt.R%u.sw_roi", channel, &ret_value, NULL))
	{
		data->sw_roi = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 21;
	}	

	if (nf_sysdb_get_key1("cam.dvabx.opt.R%u.en_usecalib", channel, &ret_value, NULL))
	{
		data->en_usecalib = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 22;
	}	

	if (nf_sysdb_get_key1("cam.dvabx.opt.R%u.en_static_filter", channel, &ret_value, NULL))
	{
		data->en_static_filter = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 23;
	}

	if (nf_sysdb_get_key1("cam.dvabx.opt.R%u.static_filter_sense", channel, &ret_value, NULL))
	{
		data->static_filter_sense = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 24;
	}

	return 0;	
}

static guint prvGetDvaBxZoneData(DvaBxZoneData *data, guint channel)
{
	GValue ret_value = {0,};
	gchar buf[256];
	gchar *p, *last, *tokx, *toky;	
	guint ret = 0;
	gint i, j;

	memset(buf, 0x00, sizeof(buf));

	if (nf_sysdb_get_key1("cam.dvabx.rule.R%u.nzones", channel, &ret_value, NULL))
	{
		data->cnt = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 1;
	}

	for (i = 0; i < IVCA_MAX_ZONES; i++)
	{
		if(nf_sysdb_get_key2("cam.dvabx.rule.R%u.Z%u.name", channel, i, &ret_value, NULL))
		{
			g_stpcpy(data->zone[i].name, g_value_get_string(&ret_value));
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 2;
		}

		if (nf_sysdb_get_key2("cam.dvabx.rule.R%u.Z%u.id", channel, i, &ret_value, NULL))
		{
			data->zone[i].id = g_value_get_int(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 3;
		}

		if (nf_sysdb_get_key2("cam.dvabx.rule.R%u.Z%u.type", channel, i, &ret_value, NULL))
		{
			data->zone[i].type = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 4;
		}

		if (nf_sysdb_get_key2("cam.dvabx.rule.R%u.Z%u.active", channel, i, &ret_value, NULL))
		{
			data->zone[i].active = g_value_get_boolean(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 5;
		}

		if (nf_sysdb_get_key2("cam.dvabx.rule.R%u.Z%u.enabled", channel, i, &ret_value, NULL))
		{
			data->zone[i].enabled = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 6;
		}

		if(nf_sysdb_get_key2("cam.dvabx.rule.R%u.Z%u.time_sarlf", channel, i, &ret_value, NULL))
		{
			if (g_value_get_string(&ret_value))
			{
				sscanf(g_value_get_string(&ret_value), "%hu %hu %hu %hu %hu", 
					&data->zone[i].stop_time, &data->zone[i].abandon_time, &data->zone[i].remove_time, &data->zone[i].loiter_time, &data->zone[i].fall_time);
			}
			else
				data->zone[i].stop_time = data->zone[i].abandon_time = data->zone[i].remove_time = data->zone[i].loiter_time = data->zone[i].fall_time = 5;

			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 7;
		}	

		if (nf_sysdb_get_key2("cam.dvabx.rule.R%u.Z%u.f_colorsens", channel, i, &ret_value, NULL))
		{
			data->zone[i].ecolor = (0x00ffffff & g_value_get_uint(&ret_value));
			data->zone[i].ecolor_sens = (0xff000000 & g_value_get_uint(&ret_value)) >> 24;
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 8;
		}

		if (nf_sysdb_get_key2("cam.dvabx.rule.R%u.Z%u.f_size", channel, i, &ret_value, NULL))
		{
			if (g_value_get_string(&ret_value))
			{
				sscanf(g_value_get_string(&ret_value), "%hu %hu %hu %hu", 
					&data->zone[i].size_min[0], &data->zone[i].size_min[1], &data->zone[i].size_max[0], &data->zone[i].size_max[1]);
			}
			else
				data->zone[i].size_min[0] = data->zone[i].size_min[1] = data->zone[i].size_max[0] = data->zone[i].size_max[1] = 0;

			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 9;
		}

		if (nf_sysdb_get_key2("cam.dvabx.rule.R%u.Z%u.f_speed", channel, i, &ret_value, NULL))
		{
			if (g_value_get_string(&ret_value))
				sscanf(g_value_get_string(&ret_value), "%hu %hu", &data->zone[i].speed_min, &data->zone[i].speed_max);
			else
				data->zone[i].speed_min = data->zone[i].speed_max = 0;

			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 10;
		}

		if (nf_sysdb_get_key2("cam.dvabx.rule.R%u.Z%u.f_class", channel, i, &ret_value, NULL))
		{
			data->zone[i].eclass = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 11;
		}

		if (nf_sysdb_get_key2("cam.dvabx.rule.R%u.Z%u.d_color", channel, i, &ret_value, NULL))
		{
			data->zone[i].color = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 12;
		}

		if (nf_sysdb_get_key2("cam.dvabx.rule.R%u.Z%u.npts", channel, i, &ret_value, NULL))
		{
			data->zone[i].npts = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 13;
		}

		if (nf_sysdb_get_key2("cam.dvabx.rule.R%u.Z%u.pt", channel, i, &ret_value, NULL))
		{
			if (g_value_get_string(&ret_value)) 
			{
				g_stpcpy(buf, g_value_get_string(&ret_value));

				for (last = buf, j = 0; j < data->zone[i].npts; j++) 
				{
					tokx = strtok_r(last, " \t,.", &last);
					if ( !tokx ) break;
					toky = strtok_r(last, " \t,.", &last);
					if ( !toky ) break;
						
					data->zone[i].pt[j].x = atoi(tokx);
					data->zone[i].pt[j].y = atoi(toky);
				}

				if (j < data->zone[i].npts) 
				{
					j = 0;
					data->zone[i].npts = 0;
				}
			}

			for ( ; j < IVCA_MAX_PTSPERZONE; j++)
				data->zone[i].pt[j].x = data->zone[i].pt[j].y = 0;			

			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 14;
		}

		if (nf_sysdb_get_key2("cam.dvabx.rule.R%u.Z%u.sensitivity", channel, i, &ret_value, NULL))
		{
			data->zone[i].sensitivity = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 15;
		}

		if(nf_sysdb_get_key2("cam.dvabx.rule.R%u.Z%u.all_detect_obj", channel, i, &ret_value, NULL))
		{
			data->zone[i].all_detect_obj = g_value_get_boolean(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 16;
		}

		if(nf_sysdb_get_key2("cam.dvabx.rule.R%u.Z%u.interest_obj", channel, i, &ret_value, NULL))
		{
            snprintf(data->zone[i].interest_obj, sizeof(data->zone[i].interest_obj)-1, "%s", g_value_get_string(&ret_value));
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 17;
		}

		if (nf_sysdb_get_key2("cam.dvabx.rule.R%u.Z%u.c_threshold", channel, i, &ret_value, NULL))
		{
			data->zone[i].c_threshold = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 18;
		}

		for (j=0; j<8; j++)
		{
			if (nf_sysdb_get_key3("cam.dvabx.rule.R%u.Z%u.E%u.event_audio", channel, i, j, &ret_value, NULL))
			{
				snprintf(data->zone[i].event_audio[j], sizeof(data->zone[i].event_audio[j])-1, "%s", g_value_get_string(&ret_value));
				g_value_unset(&ret_value);
			}
			else
			{
				DDEBUG_FAIL
				return 20;
			}
		}
	}

	return 0;
}

static guint prvGetDvaBxCntrData(DvaBxCntrData *data, guint channel)
{
	GValue ret_value = {0,};
	gchar buf[256];
	gchar *p, *last, *tokx, *toky;	
	guint ret = 0;
	gint i;

	memset(buf, 0x00, sizeof(buf));

	if (nf_sysdb_get_key1("cam.dvabx.rule.R%u.ncounters", channel, &ret_value, NULL))
	{
		data->cnt = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 1;
	}

	for (i = 0; i < IVCA_MAX_CNTRS; i++)
	{
		if(nf_sysdb_get_key2("cam.dvabx.rule.R%u.C%u.name", channel, i, &ret_value, NULL))
		{
			g_stpcpy(data->cntr[i].name, g_value_get_string(&ret_value));
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 1;
		}

		data->cntr[i].type = IVCA_RT_CNTR;

		if (nf_sysdb_get_key2("cam.dvabx.rule.R%u.C%u.id", channel, i, &ret_value, NULL))
		{
			data->cntr[i].id = g_value_get_int(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 2;
		}

		if (nf_sysdb_get_key2("cam.dvabx.rule.R%u.C%u.active", channel, i, &ret_value, NULL))
		{
			data->cntr[i].active = g_value_get_boolean(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 4;
		}

		if (nf_sysdb_get_key2("cam.dvabx.rule.R%u.C%u.enabled", channel, i, &ret_value, NULL))
		{
			data->cntr[i].enabled = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 5;
		}

		if(nf_sysdb_get_key2("cam.dvabx.rule.R%u.C%u.zid", channel, i, &ret_value, NULL))
		{
			if (g_value_get_string(&ret_value))
				sscanf(g_value_get_string(&ret_value), "%hd %hd", &data->cntr[i].zid_up, &data->cntr[i].zid_dn);
			else
				data->cntr[i].zid_up = data->cntr[i].zid_dn = -1;

			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 6;
		}	

		if (nf_sysdb_get_key2("cam.dvabx.rule.R%u.C%u.e_value", channel, i, &ret_value, NULL))
		{
			data->cntr[i].evalue = g_value_get_int(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 7;
		}

		if (nf_sysdb_get_key2("cam.dvabx.rule.R%u.C%u.reset_alert", channel, i, &ret_value, NULL))
		{
			data->cntr[i].resetalert = g_value_get_boolean(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 8;
		}

		if (nf_sysdb_get_key2("cam.dvabx.rule.R%u.C%u.d_color", channel, i, &ret_value, NULL))
		{
			data->cntr[i].color = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 9;
		}	

		if (nf_sysdb_get_key2("cam.dvabx.rule.R%u.C%u.pt", channel, i, &ret_value, NULL))
		{
			if (g_value_get_string(&ret_value))
			{
				sscanf(g_value_get_string(&ret_value), "%hd %hd %hd %hd %hd %hd %hd %hd", 
							&data->cntr[i].pt[0].x, &data->cntr[i].pt[0].y, &data->cntr[i].pt[1].x, &data->cntr[i].pt[1].y,
							&data->cntr[i].pt[2].x, &data->cntr[i].pt[2].y, &data->cntr[i].pt[3].x, &data->cntr[i].pt[3].y);

			}
			else
				memset(data->cntr[i].pt, 0, sizeof(data->cntr[i].pt));

			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 10;
		}	
	}

	return 0;
}

static guint prvGetDvaBxCalbData(DvaBxCalbData* data, guint channel)
{
    GValue ret_value = {0,};

	guint i;
	gchar str[256];

	DvaBxCalb *t = data->calb;

	memset(str, 0x00, sizeof(str));

	if(nf_sysdb_get_key1("cam.dvabx.calib.R%u.ntargets", channel, &ret_value, NULL))
	{
		data->cnt = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 1;
	}

	for (i = 0; i < 32; i++, t++) 
	{
    	if(nf_sysdb_get_key2("cam.dvabx.calib.R%u.T%u.pt", channel, i, &ret_value, NULL))
    	{
    		g_stpcpy(str, g_value_get_string(&ret_value));
    		str ? (guint)sscanf(str, "%hu %hu %hu %hu", &t->pt[0].x, &t->pt[0].y, &t->pt[1].x, &t->pt[1].y) : 0;
    		g_value_unset(&ret_value);
    	}
    	else
    	{
    		DDEBUG_FAIL
    		return 1;
    	}

    	if(nf_sysdb_get_key2("cam.dvabx.calib.R%u.T%u.height", channel, i, &ret_value, NULL))
    	{
    		t->height = g_value_get_uint(&ret_value);
    		g_value_unset(&ret_value);
    	}
    	else
    	{
    		DDEBUG_FAIL
    		return 2;
    	}

	}

	return 0;
}

static guint prvGetDvaBxCalbResData(DvaBxCalbResData* data, guint channel)
{
    GValue ret_value = {0,};

	guint i;
	gchar str[256];

	memset(str, 0x00, sizeof(str));

    if(nf_sysdb_get_key1("cam.dvabx.calib.R%u.focal", channel, &ret_value, NULL))
	{
		g_stpcpy(str, g_value_get_string(&ret_value));
		str ? (guint)sscanf(str, "%f", &data->focal) : 0;
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 1;
	}

	if(nf_sysdb_get_key1("cam.dvabx.calib.R%u.height", channel, &ret_value, NULL))
	{
		g_stpcpy(str, g_value_get_string(&ret_value));
	    str ? (guint)sscanf(str, "%f", &data->height) : 0;
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 1;
	}

	if(nf_sysdb_get_key1("cam.dvabx.calib.R%u.tilt", channel, &ret_value, NULL))
	{
		g_stpcpy(str, g_value_get_string(&ret_value));
	    str ? (guint)sscanf(str, "%f", &data->tilt) : 0;
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 1;
	}

	if(nf_sysdb_get_key1("cam.dvabx.calib.R%u.p_width", channel, &ret_value, NULL))
	{
		data->p_width = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 1;
	}
	
	
	if(nf_sysdb_get_key1("cam.dvabx.calib.R%u.p_height", channel, &ret_value, NULL))
	{
		data->p_height = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 1;
	}

	if(nf_sysdb_get_key1("cam.dvabx.calib.R%u.paramvalid", channel, &ret_value, NULL))
	{
		data->paramvalid = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 1;
	}

	return 0;
}

guint prvGetDvaBxEOsdData(DvaBxEOsdData *data, guint channel)
{
	GValue ret_value = {0};

	if (nf_sysdb_get_key1("cam.dvabx.osd.R%d.display_mode", channel, &ret_value, NULL))
	{
		data->display_mode = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}

	if (nf_sysdb_get_key1("cam.dvabx.osd.R%d.object_color", channel, &ret_value, NULL))
	{
		data->object_color = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}

	if (nf_sysdb_get_key1("cam.dvabx.osd.R%d.rule_color", channel, &ret_value, NULL))
	{
		data->rule_color = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}

	if (nf_sysdb_get_key1("cam.dvabx.osd.R%d.event_color", channel, &ret_value, NULL))
	{
		data->event_color = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}

	if (nf_sysdb_get_key1("cam.dvabx.osd.R%d.line_width", channel, &ret_value, NULL))
	{
		data->line_width = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}

	if (nf_sysdb_get_key1("cam.dvabx.osd.R%d.line_transparency", channel, &ret_value, NULL))
	{
		data->line_transparency = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}

	if (nf_sysdb_get_key1("cam.dvabx.osd.R%d.object_type", channel, &ret_value, NULL))
	{
		strcpy(data->object_type, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	
	return 0;	
}

guint DAL_get_dvabx_data(DvaBxData *data, guint channel)
{
	prvGetDvaBxPropData(&data->prop, channel);		
	prvGetDvaBxZoneData(&data->zonelist, channel);	
	prvGetDvaBxCntrData(&data->cntrlist, channel);	
	prvGetDvaBxCalbData(&data->calblist, channel);	
	prvGetDvaBxCalbResData(&data->calbres, channel);
    prvGetDvaBxEOsdData(&data->eosd, channel);
	return 0;	
}

guint DAL_get_dvabx_prop_data(DvaBxPropData *data, guint channel)
{
	prvGetDvaBxPropData(data, channel);
	return 0;	
}

guint DAL_get_dvabx_zone_data(DvaBxZoneData *data, guint channel)
{
	prvGetDvaBxZoneData(data, channel);
	return 0;	
}

guint DAL_get_dvabx_cntr_data(DvaBxCntrData *data, guint channel)
{
	prvGetDvaBxCntrData(data, channel);
	return 0;	
}

guint DAL_get_dvabx_calb_data(DvaBxCalbData *data, guint channel)
{
	prvGetDvaBxCalbData(data, channel);
	return 0;	
}

guint DAL_get_dvabx_calbres_data(DvaBxCalbResData *data, guint channel)
{
	prvGetDvaBxCalbResData(data, channel);
	return 0;	
}

guint DAL_get_dvabx_eosd_data(DvaBxEOsdData *data, guint channel)
{
	prvGetDvaBxEOsdData(data, channel);
	return 0;
}

static guint prvSetDvaBxPropData(DvaBxPropData data, guint channel)
{
	gint i;
	gchar buf[256];//64
	GValue set_value = {0,};
	guint ret = 0;

	memset(buf, 0x00, sizeof(buf));

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.en_engine);
	if(!nf_sysdb_set_key1("cam.dvabx.cfg.R%u.en_engine", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);	

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.unit);
	if(!nf_sysdb_set_key1("cam.dvabx.cfg.R%u.unit", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);
	
	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.en_shadowrm);
	if(!nf_sysdb_set_key1("cam.dvabx.opt.R%u.en_shadowrm", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);
	
	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.track_ref);
	if(!nf_sysdb_set_key1("cam.dvabx.opt.R%u.track_ref", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

    sprintf(buf, "%hu %hu", data.min_width3d, data.min_height3d);
	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);
	
	if(!nf_sysdb_set_key1("cam.dvabx.opt.R%u.min_objsize", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.sw_obj_bb);
	if(!nf_sysdb_set_key1("cam.dvabx.opt.R%u.sw_obj_bb", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);	
	
	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.sw_obj_tr);
	if(!nf_sysdb_set_key1("cam.dvabx.opt.R%u.sw_obj_tr", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);	
	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.sw_obj_id);
	if(!nf_sysdb_set_key1("cam.dvabx.opt.R%u.sw_obj_id", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 7;
	}
	g_value_unset(&set_value);	

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.sw_obj_s3d);
	if(!nf_sysdb_set_key1("cam.dvabx.opt.R%u.sw_obj_s3d", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 8;
	}
	g_value_unset(&set_value);	

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.sw_obj_w3d);
	if(!nf_sysdb_set_key1("cam.dvabx.opt.R%u.sw_obj_w3d", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 9;
	}
	g_value_unset(&set_value);	

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.sw_obj_h3d);
	if(!nf_sysdb_set_key1("cam.dvabx.opt.R%u.sw_obj_h3d", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 10;
	}
	g_value_unset(&set_value);	

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.sw_rule);
	if(!nf_sysdb_set_key1("cam.dvabx.opt.R%u.sw_rule", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 11;
	}
	g_value_unset(&set_value);	

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.active);
	if(!nf_sysdb_set_key1("cam.dvabx.cfg.R%u.act", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 12;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.detect);
	if(!nf_sysdb_set_key1("cam.dvabx.cfg.R%u.detect", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 13;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.en_roi);
	if(!nf_sysdb_set_key1("cam.dvabx.opt.R%u.en_roi", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 14;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.en_tamper);
	if(!nf_sysdb_set_key1("cam.dvabx.opt.R%u.en_tamper", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 15;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.en_snapshot);
	if(!nf_sysdb_set_key1("cam.dvabx.opt.R%u.en_snapshot", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 16;
	}
	g_value_unset(&set_value);	

	g_sprintf(buf, "%hu %hu %hu %hu", data.roi_rect.x, data.roi_rect.y, data.roi_rect.w, data.roi_rect.h);
	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);
	if(!nf_sysdb_set_key1("cam.dvabx.opt.R%u.roi_xywh", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 17;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.sw_obj_ar);
	if(!nf_sysdb_set_key1("cam.dvabx.opt.R%u.sw_obj_ar", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 18;
	}
	g_value_unset(&set_value);	

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.sw_obj_tm);
	if(!nf_sysdb_set_key1("cam.dvabx.opt.R%u.sw_obj_tm", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 19;
	}
	g_value_unset(&set_value);	

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.sw_rule_name);
	if(!nf_sysdb_set_key1("cam.dvabx.opt.R%u.sw_rule_name", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 20;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.sw_roi);
	if(!nf_sysdb_set_key1("cam.dvabx.opt.R%u.sw_roi", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 21;
	}
	g_value_unset(&set_value);
	
	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.en_usecalib);
	if(!nf_sysdb_set_key1("cam.dvabx.opt.R%u.en_usecalib", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 22;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.en_static_filter);
	if(!nf_sysdb_set_key1("cam.dvabx.opt.R%u.en_static_filter", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 23;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.static_filter_sense);
	if(!nf_sysdb_set_key1("cam.dvabx.opt.R%u.static_filter_sense", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 24;
	}
	g_value_unset(&set_value);

	return 0;
}

static guint prvSetDvaBxZoneData(DvaBxZoneData data, guint channel)
{
	gint i, j;
	gchar buf[256];
	gchar *p;	
	GValue set_value = {0,};

	memset(buf, 0x00, sizeof(buf));

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.cnt);
	if(!nf_sysdb_set_key1("cam.dvabx.rule.R%u.nzones", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);	

	for (i = 0; i < IVCA_MAX_ZONES; i++)
	{
		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, data.zone[i].name);
		if(!nf_sysdb_set_key2("cam.dvabx.rule.R%u.Z%u.name", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 2;
		}
		g_value_unset(&set_value);		

		g_value_init(&set_value, G_TYPE_INT);
		g_value_set_int(&set_value, data.zone[i].id);
		if(!nf_sysdb_set_key2("cam.dvabx.rule.R%u.Z%u.id", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 3;
		}
		g_value_unset(&set_value);		

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.zone[i].type);
		if(!nf_sysdb_set_key2("cam.dvabx.rule.R%u.Z%u.type", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 4;
		}
		g_value_unset(&set_value);		

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data.zone[i].active);
		if(!nf_sysdb_set_key2("cam.dvabx.rule.R%u.Z%u.active", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 5;
		}
		g_value_unset(&set_value);		

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.zone[i].enabled);
		if(!nf_sysdb_set_key2("cam.dvabx.rule.R%u.Z%u.enabled", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 6;
		}
		g_value_unset(&set_value);		

		g_sprintf(buf, "%hu %hu %hu %hu %hu", 
			data.zone[i].stop_time, data.zone[i].abandon_time, data.zone[i].remove_time, data.zone[i].loiter_time, data.zone[i].fall_time);
		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, buf);
		if(!nf_sysdb_set_key2("cam.dvabx.rule.R%u.Z%u.time_sarlf", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 7;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.zone[i].ecolor + (data.zone[i].ecolor_sens << 24));
		if(!nf_sysdb_set_key2("cam.dvabx.rule.R%u.Z%u.f_colorsens", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 8;
		}
		g_value_unset(&set_value);		

		g_sprintf(buf, "%hu %hu %hu %hu", data.zone[i].size_min[0], data.zone[i].size_min[1], data.zone[i].size_max[0], data.zone[i].size_max[1]);
		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, buf);
		if(!nf_sysdb_set_key2("cam.dvabx.rule.R%u.Z%u.f_size", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 9;
		}
		g_value_unset(&set_value);

		g_sprintf(buf, "%hu %hu", data.zone[i].speed_min, data.zone[i].speed_max);
		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, buf);
		if(!nf_sysdb_set_key2("cam.dvabx.rule.R%u.Z%u.f_speed", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 10;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.zone[i].eclass);
		if(!nf_sysdb_set_key2("cam.dvabx.rule.R%u.Z%u.f_class", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 11;
		}
		g_value_unset(&set_value);	

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.zone[i].color);
		if(!nf_sysdb_set_key2("cam.dvabx.rule.R%u.Z%u.d_color", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 12;
		}
		g_value_unset(&set_value);	

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.zone[i].npts);
		if(!nf_sysdb_set_key2("cam.dvabx.rule.R%u.Z%u.npts", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 13;
		}
		g_value_unset(&set_value);

		for (buf[0] = '\0', p = buf, j = 0; j < data.zone[i].npts; j++)
			p += sprintf(p, "%s%hd %hd", j ? " " : "", data.zone[i].pt[j].x, data.zone[i].pt[j].y);

		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, buf);
		if(!nf_sysdb_set_key2("cam.dvabx.rule.R%u.Z%u.pt", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 14;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.zone[i].sensitivity);
		if(!nf_sysdb_set_key2("cam.dvabx.rule.R%u.Z%u.sensitivity", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 15;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data.zone[i].all_detect_obj);
		if(!nf_sysdb_set_key2("cam.dvabx.rule.R%u.Z%u.all_detect_obj", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 16;
		}
		g_value_unset(&set_value);	

		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, data.zone[i].interest_obj);
		if(!nf_sysdb_set_key2("cam.dvabx.rule.R%u.Z%u.interest_obj", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 17;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.zone[i].c_threshold);
		if(!nf_sysdb_set_key2("cam.dvabx.rule.R%u.Z%u.c_threshold", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 18;
		}
		g_value_unset(&set_value);

		for (j=0; j<8; j++)
		{
			g_value_init(&set_value, G_TYPE_STRING);
			g_value_set_string(&set_value, data.zone[i].event_audio[j]);
			if(!nf_sysdb_set_key3("cam.dvabx.rule.R%u.Z%u.E%u.event_audio", channel, i, j, &set_value, NULL))
			{
				g_value_unset(&set_value);
				DDEBUG_FAIL
				return 20;
			}
			g_value_unset(&set_value);
		}
	}

	return 0;
}

static guint prvSetDvaBxCntrData(DvaBxCntrData data, guint channel)
{
	gint i, j;
	gchar buf[256];
	gchar *p;	
	GValue set_value = {0,};

	memset(buf, 0x00, sizeof(buf));

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.cnt);
	if(!nf_sysdb_set_key1("cam.dvabx.rule.R%u.ncounters", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);	

	for (i = 0; i < IVCA_MAX_CNTRS; i++)
	{
		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, data.cntr[i].name);
		if(!nf_sysdb_set_key2("cam.dvabx.rule.R%u.C%u.name", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 2;
		}
		g_value_unset(&set_value);		

		g_value_init(&set_value, G_TYPE_INT);
		g_value_set_int(&set_value, data.cntr[i].id);
		if(!nf_sysdb_set_key2("cam.dvabx.rule.R%u.C%u.id", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 3;
		}
		g_value_unset(&set_value);		

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data.cntr[i].active);
		if(!nf_sysdb_set_key2("cam.dvabx.rule.R%u.C%u.active", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 4;
		}
		g_value_unset(&set_value);		

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.cntr[i].enabled);
		if(!nf_sysdb_set_key2("cam.dvabx.rule.R%u.C%u.enabled", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 5;
		}
		g_value_unset(&set_value);		

		g_sprintf(buf, "%hd %hd", data.cntr[i].zid_up, data.cntr[i].zid_dn);
		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, buf);
		if(!nf_sysdb_set_key2("cam.dvabx.rule.R%u.C%u.zid", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 6;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_INT);
		g_value_set_int(&set_value, data.cntr[i].evalue);
		if(!nf_sysdb_set_key2("cam.dvabx.rule.R%u.C%u.e_value", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 7;
		}
		g_value_unset(&set_value);	

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data.cntr[i].resetalert);
		if(!nf_sysdb_set_key2("cam.dvabx.rule.R%u.C%u.reset_alert", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 8;
		}
		g_value_unset(&set_value);	

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.cntr[i].color);
		if(!nf_sysdb_set_key2("cam.dvabx.rule.R%u.C%u.d_color", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 9;
		}
		g_value_unset(&set_value);	

		for (buf[0] = '\0', p = buf, j = 0; j < 4; j++)
			p += sprintf(p, "%s%hd %hd", j ? " " : "", data.cntr[i].pt[j].x, data.cntr[i].pt[j].y);

		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, buf);
		if(!nf_sysdb_set_key2("cam.dvabx.rule.R%u.C%u.pt", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 10;
		}
		g_value_unset(&set_value);
	}

	return 0;
}

static guint prvSetDvaBxCalbData(DvaBxCalbData data, guint channel)
{
	guint i, ebase;
	gchar buf[256];
	DvaBxCalb *t = data.calb;
	GValue set_value = {0,};

	memset(buf, 0x00, sizeof(buf));

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.cnt);
	if(!nf_sysdb_set_key1("cam.dvabx.calib.R%u.ntargets", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
        return 7;
	}
    g_value_unset(&set_value);

	for (i = 0; i < 32; i++, t++) 
	{
		ebase = 8 + i * 2;
		
		sprintf(buf, "%hu %hu %hu %hu", t->pt[0].x, t->pt[0].y, t->pt[1].x, t->pt[1].y);

    	g_value_init(&set_value, G_TYPE_STRING);
    	g_value_set_string(&set_value, buf);
    	if(!nf_sysdb_set_key2("cam.dvabx.calib.R%u.T%u.pt", channel, i, &set_value, NULL))
    	{
    		g_value_unset(&set_value);
    		DDEBUG_FAIL
            return ebase;
        }	
        g_value_unset(&set_value);

    	g_value_init(&set_value, G_TYPE_UINT);
    	g_value_set_uint(&set_value, t->height);
    	if(!nf_sysdb_set_key2("cam.dvabx.calib.R%u.T%u.height", channel, i, &set_value, NULL))
    	{
    		g_value_unset(&set_value);
    		DDEBUG_FAIL
            return ebase + 1;
        }
        g_value_unset(&set_value);
				
	}

	return 0;
}

static guint prvSetDvaBxCalbResData(DvaBxCalbResData data, guint channel)
{
	guint i, ebase;
	gchar buf[256];
	GValue set_value = {0,};

	memset(buf, 0x00, sizeof(buf));

	sprintf(buf, "%f", data.focal);
	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);
	if(!nf_sysdb_set_key1("cam.dvabx.calib.R%u.focal", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
        return 1;
    }	
    g_value_unset(&set_value);

	sprintf(buf, "%f", data.height);
	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);
	if(!nf_sysdb_set_key1("cam.dvabx.calib.R%u.height", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
        return 2;
    }	
    g_value_unset(&set_value);

	sprintf(buf, "%f", data.tilt);
	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);
	if(!nf_sysdb_set_key1("cam.dvabx.calib.R%u.tilt", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
        return 3;
    }	
    g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.p_width);
	if(!nf_sysdb_set_key1("cam.dvabx.calib.R%u.p_width", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
        return 4;
	}
    g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.p_height);
	if(!nf_sysdb_set_key1("cam.dvabx.calib.R%u.p_height", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
        return 5;
	}
    g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.paramvalid);
	if(!nf_sysdb_set_key1("cam.dvabx.calib.R%u.paramvalid", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
        return 6;
	}
    g_value_unset(&set_value);

	return 0;
}

static guint prvSetDvaBxEOsdData(DvaBxEOsdData data, guint ch)
{
	GValue set_value = {0};

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.display_mode);
	if (!nf_sysdb_set_key1("cam.dvabx.osd.R%d.display_mode", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
        return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.object_color);
	if (!nf_sysdb_set_key1("cam.dvabx.osd.R%d.object_color", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
        return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.rule_color);
	if (!nf_sysdb_set_key1("cam.dvabx.osd.R%d.rule_color", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
        return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.event_color);
	if (!nf_sysdb_set_key1("cam.dvabx.osd.R%d.event_color", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
        return 4;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.line_width);
	if (!nf_sysdb_set_key1("cam.dvabx.osd.R%d.line_width", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
        return 5;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.line_transparency);
	if (!nf_sysdb_set_key1("cam.dvabx.osd.R%d.line_transparency", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
        return 6;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.object_type);
	if (!nf_sysdb_set_key1("cam.dvabx.osd.R%d.object_type", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
        return 7;
	}
	g_value_unset(&set_value);

	return 0;
}

guint DAL_set_dvabx_data(DvaBxData data, guint channel)
{
	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	prvSetDvaBxPropData(data.prop, channel);	
	prvSetDvaBxZoneData(data.zonelist, channel);	
	prvSetDvaBxCntrData(data.cntrlist, channel);
	prvSetDvaBxCalbData(data.calblist, channel);
	prvSetDvaBxCalbResData(data.calbres, channel);
	prvSetDvaBxEOsdData(data.eosd, channel);
	
//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END

	return ret;	
}

guint DAL_set_dvabx_prop_data(DvaBxPropData data, guint channel)
{
	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	ret = prvSetDvaBxPropData(data, channel);

//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END

	return ret;	
}

guint DAL_set_dvabx_zone_data(DvaBxZoneData data, guint channel)
{
	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	ret = prvSetDvaBxZoneData(data, channel);

//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END

	return ret;	
}

guint DAL_set_dvabx_cntr_data(DvaBxCntrData data, guint channel)
{
	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	ret = prvSetDvaBxCntrData(data, channel);

//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END

	return ret;	
}

guint DAL_set_dvabx_calb_data(DvaBxCalbData data, guint channel)
{
	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	ret = prvSetDvaBxCalbData(data, channel);

//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END

	return ret;	
}

guint DAL_set_dvabx_calbres_data(DvaBxCalbResData data, guint channel)
{
	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	ret = prvSetDvaBxCalbResData(data, channel);

//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END

	return ret;	
}

guint DAL_set_dvabx_eosd_data(DvaBxEOsdData data, guint channel)
{
	DDEBUG_START
	
	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	prvSetDvaBxEOsdData(data, channel);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END

	return 0;
}

guint DAL_set_dvabx_data_all(DvaBxData *data, guint ch_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for (i = 0; i < ch_num; i++)
	{
		ret = prvSetDvaBxPropData(data[i].prop, i);
		if (ret) break;

		ret = prvSetDvaBxZoneData(data[i].zonelist, i);
		if (ret) break;

		ret = prvSetDvaBxCntrData(data[i].cntrlist, i);
		if (ret) break;

		ret = prvSetDvaBxCalbData(data[i].calblist, i);
		if (ret) break;

		ret = prvSetDvaBxCalbResData(data[i].calbres, i);
		if (ret) break;
		
		ret = prvSetDvaBxEOsdData(data[i].eosd, i);
		if (ret) break;
	}

//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END
	
	return ret;
}

guint DAL_set_dvabx_prop_data_all(DvaBxPropData *data, guint ch_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for (i = 0; i < ch_num; i++)
	{
		ret = prvSetDvaBxPropData(data[i], i);		
		if (ret) break;
	}

//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END
	
	return ret;
}

guint DAL_set_dvabx_zone_data_all(DvaBxZoneData *data, guint ch_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for (i = 0; i < ch_num; i++)
	{
		ret = prvSetDvaBxZoneData(data[i], i);		
		if (ret) break;
	}

//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END
	
	return ret;
}

guint DAL_set_dvabx_cntr_data_all(DvaBxCntrData *data, guint ch_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for (i = 0; i < ch_num; i++)
	{
		ret = prvSetDvaBxCntrData(data[i], i);		
		if (ret) break;
	}

//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END
	
	return ret;
}

guint DAL_set_dvabx_calb_data_all(DvaBxCalbData *data, guint ch_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for (i = 0; i < ch_num; i++)
	{
		ret = prvSetDvaBxCalbData(data[i], i);		
		if (ret) break;
	}

//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END
	
	return ret;
}

guint DAL_set_dvabx_calbres_data_all(DvaBxCalbResData *data, guint ch_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for (i = 0; i < ch_num; i++)
	{
		ret = prvSetDvaBxCalbResData(data[i], i);		
		if (ret) break;
	}

//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END
	
	return ret;
}

guint DAL_set_dvabx_eosd_data_all(DvaBxEOsdData *data, guint ch_num)
{
	guint ret = 0;
	guint ch;

	DDEBUG_START
	
	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for (ch = 0; ch < ch_num; ch++)
	{
		ret = prvSetDvaBxEOsdData(data[ch], ch);
		if (ret) break;
	}

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END
	
	return 0;
}

static guint prvGetDvaBxFacePropData(DvaBxFacePropData *data, guint channel)
{
	GValue ret_value = {0,};
	gchar str[256];

	guint ret = 0;
	gint i;
	
	memset(str, 0x00, sizeof(str));
	
	if (nf_sysdb_get_key1("cam.dvabx.face.cfg.R%u.act", channel, &ret_value, NULL))
	{
		data->active = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 1;
	}

	if (nf_sysdb_get_key1("cam.dvabx.face.opt.R%u.sw_obj_bb", channel, &ret_value, NULL))
	{
		data->sw_obj_bb = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 1;
	}
	
	if (nf_sysdb_get_key1("cam.dvabx.face.opt.R%u.sw_grp_name", channel, &ret_value, NULL))
	{
		data->sw_obj_grpname = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 2;
	}
	
	if (nf_sysdb_get_key1("cam.dvabx.face.opt.R%u.sw_rule", channel, &ret_value, NULL))
	{
		data->sw_rule = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 3;
	}

	if (nf_sysdb_get_key1("cam.dvabx.face.opt.R%u.sw_rule_name", channel, &ret_value, NULL))
	{
		data->sw_rule_name = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 4;
	}		

	return 0;	
}

static guint prvGetDvaBxFaceZoneData(DvaBxFaceZoneData *data, guint channel)
{
	GValue ret_value = {0,};
	gchar buf[256];
	gchar *p, *last, *tokx, *toky;	
	gchar *str;
	guint ret = 0;
	gint i, j;

	memset(buf, 0x00, sizeof(buf));

	if (nf_sysdb_get_key1("cam.dvabx.face.rule.R%u.nzones", channel, &ret_value, NULL))
	{
		data->cnt = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 1;
	}

	for (i = 0; i < 4; i++)
	{
		if (nf_sysdb_get_key2("cam.dvabx.face.rule.R%u.Z%u.act", channel, i, &ret_value, NULL))
		{
			data->zone[i].active = g_value_get_boolean(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 2;
		}

		if(nf_sysdb_get_key2("cam.dvabx.face.rule.R%u.Z%u.name", channel, i, &ret_value, NULL))
		{
			g_stpcpy(data->zone[i].name, g_value_get_string(&ret_value));
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 3;
		}

		if (nf_sysdb_get_key2("cam.dvabx.face.rule.R%u.Z%u.id", channel, i, &ret_value, NULL))
		{
			data->zone[i].id = g_value_get_int(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 4;
		}

		if (nf_sysdb_get_key2("cam.dvabx.face.rule.R%u.Z%u.type", channel, i, &ret_value, NULL))
		{
			data->zone[i].type = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 5;
		}

		if (nf_sysdb_get_key2("cam.dvabx.face.rule.R%u.Z%u.d_color", channel, i, &ret_value, NULL))
		{
			data->zone[i].d_color = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 6;
		}

		if (nf_sysdb_get_key2("cam.dvabx.face.rule.R%u.Z%u.npts", channel, i, &ret_value, NULL))
		{
			data->zone[i].npts = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 7;
		}

		if (nf_sysdb_get_key2("cam.dvabx.face.rule.R%u.Z%u.pt", channel, i, &ret_value, NULL))
		{
			if (g_value_get_string(&ret_value)) 
			{
				g_stpcpy(buf, g_value_get_string(&ret_value));

				for (last = buf, j = 0; j < data->zone[i].npts; j++) 
				{
					tokx = strtok_r(last, " \t,.", &last);
					if ( !tokx ) break;
					toky = strtok_r(last, " \t,.", &last);
					if ( !toky ) break;
						
					data->zone[i].pt[j].x = atoi(tokx);
					data->zone[i].pt[j].y = atoi(toky);
				}

				if (j < data->zone[i].npts) 
				{
					j = 0;
					data->zone[i].npts = 0;
				}
			}

			for ( ; j < 16; j++)
				data->zone[i].pt[j].x = data->zone[i].pt[j].y = 0;			

			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 8;
		}

		if(nf_sysdb_get_key2("cam.dvabx.face.rule.R%u.Z%u.group_filter", channel, i, &ret_value, NULL))
		{
			snprintf(data->zone[i].group_filter, sizeof(data->zone[i].group_filter)-1, "%s", g_value_get_string(&ret_value));
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 9;
		}
		if (nf_sysdb_get_key2("cam.dvabx.face.rule.R%u.Z%u.threshold", channel, i, &ret_value, NULL))
		{
			data->zone[i].threshold = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 10;
		}		
	}

	return 0;
}

static guint prvSetDvaBxFacePropData(DvaBxFacePropData data, guint channel)
{
	gint i;
	GValue set_value = {0,};
	guint ret = 0;

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.active);
	if(!nf_sysdb_set_key1("cam.dvabx.face.cfg.R%u.act", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);	

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.sw_obj_bb);
	if(!nf_sysdb_set_key1("cam.dvabx.face.opt.R%u.sw_obj_bb", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);	

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.sw_obj_grpname);
	if(!nf_sysdb_set_key1("cam.dvabx.face.opt.R%u.sw_grp_name", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);	

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.sw_rule);
	if(!nf_sysdb_set_key1("cam.dvabx.face.opt.R%u.sw_rule", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);	

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.sw_rule_name);
	if(!nf_sysdb_set_key1("cam.dvabx.face.opt.R%u.sw_rule_name", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	return 0;
}

static guint prvSetDvaBxFaceZoneData(DvaBxFaceZoneData data, guint channel)
{
	gint i, j;
	gchar buf[256];
	gchar buf2[9];
	gchar *p;	
	GValue set_value = {0,};

	memset(buf, 0x00, sizeof(buf));

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.cnt);
	if(!nf_sysdb_set_key1("cam.dvabx.face.rule.R%u.nzones", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);	

	for (i = 0; i < 4; i++)
	{
		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data.zone[i].active);
		if(!nf_sysdb_set_key2("cam.dvabx.face.rule.R%u.Z%u.act", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 2;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_INT);
		g_value_set_int(&set_value, data.zone[i].id);
		if(!nf_sysdb_set_key2("cam.dvabx.face.rule.R%u.Z%u.id", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 3;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, data.zone[i].name);
		if(!nf_sysdb_set_key2("cam.dvabx.face.rule.R%u.Z%u.name", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 4;
		}
		g_value_unset(&set_value);		

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.zone[i].type);
		if(!nf_sysdb_set_key2("cam.dvabx.face.rule.R%u.Z%u.type", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 5;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.zone[i].npts);
		if(!nf_sysdb_set_key2("cam.dvabx.face.rule.R%u.Z%u.npts", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 6;
		}
		g_value_unset(&set_value);

		for (buf[0] = '\0', p = buf, j = 0; j < data.zone[i].npts; j++)
			p += sprintf(p, "%s%hd %hd", j ? " " : "", data.zone[i].pt[j].x, data.zone[i].pt[j].y);

		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, buf);
		if(!nf_sysdb_set_key2("cam.dvabx.face.rule.R%u.Z%u.pt", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 7;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.zone[i].d_color);
		if(!nf_sysdb_set_key2("cam.dvabx.face.rule.R%u.Z%u.d_color", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 8;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, data.zone[i].group_filter);
		if(!nf_sysdb_set_key2("cam.dvabx.face.rule.R%u.Z%u.group_filter", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 9;
		}
		g_value_unset(&set_value);


		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.zone[i].threshold);
		if(!nf_sysdb_set_key2("cam.dvabx.face.rule.R%u.Z%u.threshold", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 10;
		}
		g_value_unset(&set_value);		
	}

	return 0;
}

guint DAL_get_dvabx_face_data(DvaBxFaceData *data, guint channel)
{
	prvGetDvaBxFacePropData(&data->prop, channel);		
	prvGetDvaBxFaceZoneData(&data->zonelist, channel);
	return 0;
}

guint DAL_get_dvabx_face_prop_data(DvaBxFacePropData *data, guint channel)
{
	prvGetDvaBxFacePropData(data, channel);
	return 0;
}

guint DAL_get_dvabx_face_zone_data(DvaBxFaceZoneData *data, guint channel)
{
	prvGetDvaBxFaceZoneData(data, channel);
	return 0;
}

guint DAL_get_dvabx_face_cntr_data(DvaBxFaceCntrData *data, guint channel)
{
	guint ret = 0;
	guint i;

	return ret;
}

guint DAL_set_dvabx_face_data(DvaBxFaceData data, guint channel)
{
	guint ret = 0;

	DDEBUG_START
	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	prvSetDvaBxFacePropData(data.prop, channel);	
	prvSetDvaBxFaceZoneData(data.zonelist, channel);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
	DDEBUG_END

	return ret;
}

guint DAL_set_dvabx_face_prop_data(DvaBxFacePropData data, guint channel)
{
	guint ret = 0;

	DDEBUG_START
	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	ret = prvSetDvaBxFacePropData(data, channel);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
	DDEBUG_END

	return ret;	
}

guint DAL_set_dvabx_face_zone_data(DvaBxFaceZoneData data, guint channel)
{
	guint ret = 0;

	DDEBUG_START
	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	ret = prvSetDvaBxFaceZoneData(data, channel);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
	DDEBUG_END

	return ret;	
}

guint DAL_set_dvabx_face_cntr_data(DvaBxFaceCntrData data, guint channel)
{
	guint ret = 0;
	guint i;

	return ret;
}

guint DAL_set_dvabx_face_data_all(DvaBxFaceData *data, guint ch_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START
	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for (i = 0; i < ch_num; i++)
	{
		ret = prvSetDvaBxFacePropData(data[i].prop, i);
		if (ret) break;

		ret = prvSetDvaBxFaceZoneData(data[i].zonelist, i);
		if (ret) break;
	}

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
	DDEBUG_END
	
	return ret;
}

guint DAL_set_dvabx_face_prop_data_all(DvaBxFacePropData *data, guint ch_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START
	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for (i = 0; i < ch_num; i++)
	{
		ret = prvSetDvaBxFacePropData(data[i], i);		
		if (ret) break;
	}

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
	DDEBUG_END
	
	return ret;
}

guint DAL_set_dvabx_face_zone_data_all(DvaBxFaceZoneData *data, guint ch_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START
	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for (i = 0; i < ch_num; i++)
	{
		ret = prvSetDvaBxFaceZoneData(data[i], i);		
		if (ret) break;
	}

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
	DDEBUG_END
	
	return ret;
}

guint DAL_set_dvabx_face_cntr_data_all(DvaBxFaceCntrData *data, guint ch_num)
{
	guint ret = 0;
	guint i;

	return ret;
}

guint DAL_get_dvabx_face_grp(DvaBxFaceZoneData *data, gint ch)
{
	GValue ret_value = {0,};

	if(nf_sysdb_get_key2("cam.dvabx.face.rule.R%u.Z%u.group_filter", ch, 0, &ret_value, NULL))
	{
		snprintf(data->zone[0].group_filter, sizeof(data->zone[0].group_filter)-1, "%s", g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return -1;
	}

	return 0;
}

static guint prvGetDvaBxPlatenoPropData(DvaBxPlatenoPropData *data, guint channel)
{
	GValue ret_value = {0,};
	gchar str[256];

	guint ret = 0;
	gint i;
	
	memset(str, 0x00, sizeof(str));

	if (nf_sysdb_get_key1("cam.dvabx.plateno.cfg.R%u.act", channel, &ret_value, NULL))
	{
		data->active = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 1;
	}

	if (nf_sysdb_get_key1("cam.dvabx.plateno.opt.R%u.sw_obj_bb", channel, &ret_value, NULL))
	{
		data->sw_obj_bb = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 1;
	}
	
	if (nf_sysdb_get_key1("cam.dvabx.plateno.opt.R%u.sw_grp_name", channel, &ret_value, NULL))
	{
		data->sw_obj_grpname = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 2;
	}

	if (nf_sysdb_get_key1("cam.dvabx.plateno.opt.R%u.sw_plate_number", channel, &ret_value, NULL))
	{
		data->sw_obj_number = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 3;
	}	
	
	if (nf_sysdb_get_key1("cam.dvabx.plateno.opt.R%u.sw_rule", channel, &ret_value, NULL))
	{
		data->sw_rule = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 4;
	}

	if (nf_sysdb_get_key1("cam.dvabx.plateno.opt.R%u.sw_rule_name", channel, &ret_value, NULL))
	{
		data->sw_rule_name = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 5;
	}		

	return 0;	
}

static guint prvGetDvaBxPlatenoZoneData(DvaBxPlatenoZoneData *data, guint channel)
{
	GValue ret_value = {0,};
	gchar buf[256];
	gchar *p, *last, *tokx, *toky;	
	gchar *str;
	guint ret = 0;
	gint i, j;

	memset(buf, 0x00, sizeof(buf));

	if (nf_sysdb_get_key1("cam.dvabx.plateno.rule.R%u.nzones", channel, &ret_value, NULL))
	{
		data->cnt = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 1;
	}

	for (i = 0; i < 4; i++)
	{
		if (nf_sysdb_get_key2("cam.dvabx.plateno.rule.R%u.Z%u.act", channel, i, &ret_value, NULL))
		{
			data->zone[i].active = g_value_get_boolean(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 2;
		}

		if(nf_sysdb_get_key2("cam.dvabx.plateno.rule.R%u.Z%u.name", channel, i, &ret_value, NULL))
		{
			g_stpcpy(data->zone[i].name, g_value_get_string(&ret_value));
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 3;
		}

		if (nf_sysdb_get_key2("cam.dvabx.plateno.rule.R%u.Z%u.id", channel, i, &ret_value, NULL))
		{
			data->zone[i].id = g_value_get_int(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 4;
		}

		if (nf_sysdb_get_key2("cam.dvabx.plateno.rule.R%u.Z%u.type", channel, i, &ret_value, NULL))
		{
			data->zone[i].type = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 5;
		}

		if (nf_sysdb_get_key2("cam.dvabx.plateno.rule.R%u.Z%u.d_color", channel, i, &ret_value, NULL))
		{
			data->zone[i].d_color = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 6;
		}

		if (nf_sysdb_get_key2("cam.dvabx.plateno.rule.R%u.Z%u.npts", channel, i, &ret_value, NULL))
		{
			data->zone[i].npts = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 7;
		}

		if (nf_sysdb_get_key2("cam.dvabx.plateno.rule.R%u.Z%u.pt", channel, i, &ret_value, NULL))
		{
			if (g_value_get_string(&ret_value)) 
			{
				g_stpcpy(buf, g_value_get_string(&ret_value));

				for (last = buf, j = 0; j < data->zone[i].npts; j++) 
				{
					tokx = strtok_r(last, " \t,.", &last);
					if ( !tokx ) break;
					toky = strtok_r(last, " \t,.", &last);
					if ( !toky ) break;
						
					data->zone[i].pt[j].x = atoi(tokx);
					data->zone[i].pt[j].y = atoi(toky);
				}

				if (j < data->zone[i].npts) 
				{
					j = 0;
					data->zone[i].npts = 0;
				}
			}

			for ( ; j < 16; j++)
				data->zone[i].pt[j].x = data->zone[i].pt[j].y = 0;			

			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 8;
		}

		if(nf_sysdb_get_key2("cam.dvabx.plateno.rule.R%u.Z%u.grp_mask", channel, i, &ret_value, NULL))
		{
			str = g_value_get_string(&ret_value);

			memset(buf, 0x00, sizeof(buf));
			g_stpcpy(buf, str);

			data->zone[i].group_mask = 0;
			for (j = 0; j < 9; j++)
			{
				if (buf[j] == '1') data->zone[i].group_mask |= (1 << j);
			}
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 9;
		}
		if (nf_sysdb_get_key2("cam.dvabx.plateno.rule.R%u.Z%u.threshold", channel, i, &ret_value, NULL))
		{
			data->zone[i].threshold = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 10;
		}		
	}

	return 0;
}

static guint prvSetDvaBxPlatenoPropData(DvaBxPlatenoPropData data, guint channel)
{
	gint i;
	GValue set_value = {0,};
	guint ret = 0;

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.active);
	if(!nf_sysdb_set_key1("cam.dvabx.plateno.cfg.R%u.act", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);	

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.sw_obj_bb);
	if(!nf_sysdb_set_key1("cam.dvabx.plateno.opt.R%u.sw_obj_bb", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);	

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.sw_obj_grpname);
	if(!nf_sysdb_set_key1("cam.dvabx.plateno.opt.R%u.sw_grp_name", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);	

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.sw_obj_number);
	if(!nf_sysdb_set_key1("cam.dvabx.plateno.opt.R%u.sw_plate_number", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);	

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.sw_rule);
	if(!nf_sysdb_set_key1("cam.dvabx.plateno.opt.R%u.sw_rule", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);	

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.sw_rule_name);
	if(!nf_sysdb_set_key1("cam.dvabx.plateno.opt.R%u.sw_rule_name", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);

	return 0;
}

static guint prvSetDvaBxPlatenoZoneData(DvaBxPlatenoZoneData data, guint channel)
{
	gint i, j;
	gchar buf[256];
	gchar buf2[10];
	gchar *p;	
	GValue set_value = {0,};

	memset(buf, 0x00, sizeof(buf));

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.cnt);
	if(!nf_sysdb_set_key1("cam.dvabx.plateno.rule.R%u.nzones", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);	

	for (i = 0; i < 4; i++)
	{
		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data.zone[i].active);
		if(!nf_sysdb_set_key2("cam.dvabx.plateno.rule.R%u.Z%u.act", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 2;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_INT);
		g_value_set_int(&set_value, data.zone[i].id);
		if(!nf_sysdb_set_key2("cam.dvabx.plateno.rule.R%u.Z%u.id", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 3;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, data.zone[i].name);
		if(!nf_sysdb_set_key2("cam.dvabx.plateno.rule.R%u.Z%u.name", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 4;
		}
		g_value_unset(&set_value);		

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.zone[i].type);
		if(!nf_sysdb_set_key2("cam.dvabx.plateno.rule.R%u.Z%u.type", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 5;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.zone[i].npts);
		if(!nf_sysdb_set_key2("cam.dvabx.plateno.rule.R%u.Z%u.npts", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 6;
		}
		g_value_unset(&set_value);

		for (buf[0] = '\0', p = buf, j = 0; j < data.zone[i].npts; j++)
			p += sprintf(p, "%s%hd %hd", j ? " " : "", data.zone[i].pt[j].x, data.zone[i].pt[j].y);

		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, buf);
		if(!nf_sysdb_set_key2("cam.dvabx.plateno.rule.R%u.Z%u.pt", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 7;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.zone[i].d_color);
		if(!nf_sysdb_set_key2("cam.dvabx.plateno.rule.R%u.Z%u.d_color", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 8;
		}
		g_value_unset(&set_value);

		memset(buf2, 0, sizeof(buf2));
		for (j = 0; j < 9; j++)
		{
			if (data.zone[i].group_mask & (1 << j)) buf2[j] = '1';
			else buf2[j] = '0';
		}

		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, buf2);
		if(!nf_sysdb_set_key2("cam.dvabx.plateno.rule.R%u.Z%u.grp_mask", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 9;
		}
		g_value_unset(&set_value);


		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data.zone[i].threshold);
		if(!nf_sysdb_set_key2("cam.dvabx.plateno.rule.R%u.Z%u.threshold", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 10;
		}
		g_value_unset(&set_value);		
	}

	return 0;
}

guint DAL_get_dvabx_plateno_data(DvaBxPlatenoData *data, guint channel)
{
	prvGetDvaBxPlatenoPropData(&data->prop, channel);		
	prvGetDvaBxPlatenoZoneData(&data->zonelist, channel);
	return 0;
}

guint DAL_get_dvabx_plateno_prop_data(DvaBxPlatenoPropData *data, guint channel)
{
	prvGetDvaBxPlatenoPropData(data, channel);
	return 0;
}

guint DAL_get_dvabx_plateno_zone_data(DvaBxPlatenoZoneData *data, guint channel)
{
	prvGetDvaBxPlatenoZoneData(data, channel);
	return 0;
}

guint DAL_get_dvabx_plateno_cntr_data(DvaBxPlatenoCntrData *data, guint channel)
{
	guint ret = 0;
	guint i;

	return ret;
}

guint DAL_set_dvabx_plateno_data(DvaBxPlatenoData data, guint channel)
{
	guint ret = 0;

	DDEBUG_START
	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	prvSetDvaBxPlatenoPropData(data.prop, channel);	
	prvSetDvaBxPlatenoZoneData(data.zonelist, channel);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
	DDEBUG_END

	return ret;
}

guint DAL_set_dvabx_plateno_prop_data(DvaBxPlatenoPropData data, guint channel)
{
	guint ret = 0;

	DDEBUG_START
	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	ret = prvSetDvaBxPlatenoPropData(data, channel);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
	DDEBUG_END

	return ret;	
}

guint DAL_set_dvabx_plateno_zone_data(DvaBxPlatenoZoneData data, guint channel)
{
	guint ret = 0;

	DDEBUG_START
	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	ret = prvSetDvaBxPlatenoZoneData(data, channel);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
	DDEBUG_END

	return ret;
}

guint DAL_set_dvabx_plateno_cntr_data(DvaBxPlatenoCntrData data, guint channel)
{
	guint ret = 0;
	guint i;

	return ret;
}

guint DAL_set_dvabx_plateno_data_all(DvaBxPlatenoData *data, guint ch_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START
	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for (i = 0; i < ch_num; i++)
	{
		ret = prvSetDvaBxPlatenoPropData(data[i].prop, i);
		if (ret) break;

		ret = prvSetDvaBxPlatenoZoneData(data[i].zonelist, i);
		if (ret) break;
	}

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
	DDEBUG_END
	
	return ret;
}

guint DAL_set_dvabx_plateno_prop_data_all(DvaBxPlatenoPropData *data, guint ch_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START
	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for (i = 0; i < ch_num; i++)
	{
		ret = prvSetDvaBxPlatenoPropData(data[i], i);		
		if (ret) break;
	}

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
	DDEBUG_END
	
	return ret;
}

guint DAL_set_dvabx_plateno_zone_data_all(DvaBxPlatenoZoneData *data, guint ch_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START
	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for (i = 0; i < ch_num; i++)
	{
		ret = prvSetDvaBxPlatenoZoneData(data[i], i);		
		if (ret) break;
	}

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
	DDEBUG_END
	
	return ret;
}

guint DAL_set_dvabx_plateno_cntr_data_all(DvaBxPlatenoCntrData *data, guint ch_num)
{
	guint ret = 0;
	guint i;

	return ret;
}

guint DAL_get_dvabx_schd_data_all(DvaBxSchedData *data, guint ch_num)
{
	gchar key[64], *str;
	guint ch;

	memset(data, 0, sizeof(VCASchedData) * ch_num);

	for (ch = 0; ch < ch_num; ch++) {
		sprintf(key, "cam.dvabx.cfg.R%u.sched", ch);
		str = nf_sysdb_get_str_nocopy(key);
		if ( str )
			memcpy(data[ch].sched, str, 24*7);
	}
	return 0;
}	/* DAL_get_vca_schd_data_all(... */

static guint prvSetDvaBxSchedData(DvaBxSchedData data, guint ch)
{
	GValue set_value = {0};

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.sched);
	if(!nf_sysdb_set_key1("cam.dvabx.cfg.R%u.sched", ch, &set_value, NULL)) {
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	return FALSE;	
}

guint DAL_set_dvabx_schd_data(DvaBxSchedData data, guint channel)
{
	nf_sysdb_lock(NF_SYSDB_CATE_CAM);
	
	prvSetDvaBxSchedData(data, channel);
//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
	return 0;
}

guint DAL_set_dvabx_schd_data_all(DvaBxSchedData *data, guint ch_num)
{
	guint ch;

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for (ch = 0; ch < ch_num; ch++)
		prvSetDvaBxSchedData(data[ch], ch);
//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
	return 0;
}


//////////////////////////////////////////////////////////////////////////////////////
//captainnn
guint DAL_get_vca_prop_data_all(VCAPropData *data, guint ch_num)
{
	gchar key[64];
	guint ch;

	for (ch = 0; ch < ch_num; ch++) {
		sprintf(key, "cam.vca.cfg.R%u.act", ch);
		data[ch].active = nf_sysdb_get_bool(key);
		sprintf(key, "cam.vca.cfg.R%u.detect", ch);
		data[ch].detect = nf_sysdb_get_bool(key);
		sprintf(key, "cam.vca.cfg.R%u.unit", ch);
		data[ch].unit= (guint8)nf_sysdb_get_uint(key);
	}

	return 0;
}

//static
guint
prvGetVCAOptData(ivca_option_t*option, guint channel)
{
	gchar key[64], *p, *str;
	gint m;

	p = key + sprintf(key, "cam.vca.opt.R%u.", channel);

	sprintf(p, "en_shadowrm");
	option->en_shadowrm = nf_sysdb_get_bool(key);
	sprintf(p, "en_prediction");
	option->en_prediction = nf_sysdb_get_bool(key);
	sprintf(p, "en_roi");
	option->en_roi = nf_sysdb_get_bool(key);
	sprintf(p, "en_tamper");
	option->en_tamper = nf_sysdb_get_bool(key);
	sprintf(p, "en_usecalib");
	option->en_usecalib = nf_sysdb_get_bool(key);
	sprintf(p, "en_snapshot");
	option->en_snapshot = nf_sysdb_get_bool(key);
	sprintf(p, "en_privacy");
	option->en_privacy= nf_sysdb_get_bool(key);
	sprintf(p, "min_objsize");
	str = nf_sysdb_get_str_nocopy(key);
	m = str ? sscanf(str, "%hd %hd", &option->min_width3d, &option->min_height3d) : 0;
	sprintf(p, "roi_xywh");
	str = nf_sysdb_get_str_nocopy(key);
	m = str ? sscanf(str, "%hd %hd %hd %hd", &option->roi.x, &option->roi.y,
			&option->roi.w, &option->roi.h) : 0;
	if ( m < 4 ) {
		option->roi.x = 120;
		option->roi.y = 90;
		option->roi.w = 3600;
		option->roi.h = 1980;
	}

	sprintf(p, "track_ref");
	option->track_ref = (guint8)nf_sysdb_get_uint(key);
	sprintf(p, "sw_obj_bb");
	option->sw_obj_bb = nf_sysdb_get_bool(key);
	sprintf(p, "sw_obj_id");
	option->sw_obj_id = nf_sysdb_get_bool(key);
	sprintf(p, "sw_obj_ar");
	option->sw_obj_ar = nf_sysdb_get_bool(key);
	sprintf(p, "sw_obj_tm");
	option->sw_obj_tm = nf_sysdb_get_bool(key);
	sprintf(p, "sw_obj_tr");
	option->sw_obj_tr = nf_sysdb_get_bool(key);
	sprintf(p, "sw_obj_cl");
	option->sw_obj_cl = nf_sysdb_get_bool(key);
	sprintf(p, "sw_rule");
	option->sw_rule = nf_sysdb_get_bool(key);
	sprintf(p, "sw_rule_name");
	option->sw_rule_name = nf_sysdb_get_bool(key);
	sprintf(p, "sw_roi");
	option->sw_roi = nf_sysdb_get_bool(key);

	sprintf(p, "sw_dbg_fg");
	option->sw_dbg_fg = nf_sysdb_get_bool(key);
	sprintf(p, "sw_dbg_sh");
	option->sw_dbg_sh = nf_sysdb_get_bool(key);
	sprintf(p, "sw_dbg_info");
	option->sw_dbg_info = nf_sysdb_get_bool(key);

	sprintf(p, "sw_obj_s3d");
	option->sw_obj_s3d= nf_sysdb_get_bool(key);
	sprintf(p, "sw_obj_w3d");
	option->sw_obj_w3d = nf_sysdb_get_bool(key);
	sprintf(p, "sw_obj_h3d");
	option->sw_obj_h3d = nf_sysdb_get_bool(key);

	return 0;
}

//static
guint
prvGetVCARuleData(ivca_rule_t*rule, guint channel)
{
	guint n, m, i;
	gchar key[64], *p, *str, *last, *tokx, *toky;
	ivca_zone_t*z = rule->zonelist;
	ivca_cntr_t*c = rule->cntrlist;

	p = key + sprintf(key, "cam.vca.rule.R%u.", channel);

	sprintf(p, "n_width");
	rule->n_width = (guint16)nf_sysdb_get_uint(key);
	sprintf(p, "n_height");
	rule->n_height = (guint16)nf_sysdb_get_uint(key);
	sprintf(p, "nzones");
	rule->nzones = (guint16)nf_sysdb_get_uint(key);
	sprintf(p, "ncounters");
	rule->ncntrs = (guint16)nf_sysdb_get_uint(key);

	for (n = 0; n < IVCA_MAX_ZONES; n++, z++) {
		p = key + sprintf(key, "cam.vca.rule.R%u.Z%u.", channel, n);

		sprintf(p, "name");
		str = nf_sysdb_get_str_nocopy(key);
		strncpy(z->name, str ? str : "", sizeof(z->name));
		sprintf(p, "id");
		z->id = (gint16)nf_sysdb_get_int(key);
		sprintf(p, "type");
		z->type = (guint8)nf_sysdb_get_uint(key);
		sprintf(p, "active");
		z->active = (guint8)nf_sysdb_get_bool(key);
		sprintf(p, "enabled");
		z->enabled = nf_sysdb_get_uint(key);
		sprintf(p, "time_sarlf");
		str = nf_sysdb_get_str_nocopy(key);
		m = str ? (guint)sscanf(str, "%hu %hu %hu %hu %hu", &z->stop_time,
				&z->abandon_time, &z->remove_time, &z->loiter_time,&z->fall_time) : 0;
		if ( m < 5 )
			z->stop_time = z->abandon_time =
					z->remove_time = z->loiter_time = z->fall_time= 5;
		sprintf(p, "f_colorsens");
		*(guint *)z->ecolor = (0x00ffffff & nf_sysdb_get_uint(key));
		z->ecolor_sens = (0xff000000 & nf_sysdb_get_uint(key)) >>24;
		sprintf(p, "f_size");
		str = nf_sysdb_get_str_nocopy(key);
		m = str ? (guint)sscanf(str, "%hu %hu %hu %hu", &z->size_min[0],
				&z->size_min[1], &z->size_max[0], &z->size_max[1]) : 0;
		if ( m < 4 )
			z->size_min[0] = z->size_min[0] =
					z->size_max[0] = z->size_max[0] = 0;
		sprintf(p, "f_speed");
		str = nf_sysdb_get_str_nocopy(key);
		m = str ? (guint)sscanf(str, "%hu %hu", &z->speed_min, &z->speed_max) :
				0;
		if ( m < 2 )
			z->speed_min = z->speed_max = 0;
		sprintf(p, "f_class");
		z->eclass = (guint8)nf_sysdb_get_uint(key);
		sprintf(p, "d_color");
		m = nf_sysdb_get_uint(key);
		z->color[0] = (guint8)m;
		z->color[1] = (guint8)(m >> 8);
		z->color[2] = (guint8)(m >> 16);
		sprintf(p, "npts");
		z->npts = (guint8)nf_sysdb_get_uint(key);
		sprintf(p, "pt");
		i = 0;
		str = nf_sysdb_get_str(key);
		if ( str ) {
			for (last = str, i = 0; i < z->npts; i++) {
				tokx = strtok_r(last, " \t,.", &last);
				if ( !tokx )
					break;
				toky = strtok_r(last, " \t,.", &last);
				if ( !toky )
					break;
				z->pt[i].x = (gint16)atoi(tokx);
				z->pt[i].y = (gint16)atoi(toky);
			}
			if ( i < z->npts ) {
				i = 0;
				z->npts = 0;
			}
			free(str);
		}
		for ( ; i < IVCA_MAX_PTSPERZONE; i++)
			z->pt[i].x = z->pt[i].y = 0;

		sprintf(p, "sensitivity");
		z->sensitivity = nf_sysdb_get_uint(key);
	}

	for (n = 0; n < IVCA_MAX_CNTRS; n++, c++) {
		p = key + sprintf(key, "cam.vca.rule.R%u.C%u.", channel, n);

		sprintf(p, "name");
		str = nf_sysdb_get_str_nocopy(key);
		strncpy(c->name, str ? str : "", sizeof(c->name));
		sprintf(p, "id");
		c->id = (gint16)nf_sysdb_get_int(key);
		sprintf(p, "active");
		c->active = (guint8)nf_sysdb_get_bool(key);
		sprintf(p, "enabled");
		c->enabled = nf_sysdb_get_uint(key);
		sprintf(p, "zid");
		str = nf_sysdb_get_str_nocopy(key);
		m = str ? (guint)sscanf(str, "%hd %hd", &c->zid_up, &c->zid_dn) : 0;
		if ( m < 2 )
			c->zid_up = c->zid_dn = -1;
		sprintf(p, "e_value");
		c->evalue = nf_sysdb_get_int(key);
		sprintf(p, "reset_alert");
		c->resetalert = (guint8)nf_sysdb_get_bool(key);
		sprintf(p, "d_color");
		m = nf_sysdb_get_uint(key);
		c->color[0] = (guint8)m;
		c->color[1] = (guint8)(m >> 8);
		c->color[2] = (guint8)(m >> 16);
		sprintf(p, "pt");
		str = nf_sysdb_get_str_nocopy(key);
		m = str ? (guint)sscanf(str, "%hu %hu %hu %hu %hu %hu %hu %hu",
				&c->pt[0].x, &c->pt[0].y, &c->pt[1].x, &c->pt[1].y,
				&c->pt[2].x, &c->pt[2].y, &c->pt[3].x, &c->pt[3].y) : 0;
		if ( m < 8 )
			memset(c->pt, 0, sizeof(c->pt));
	}

	return 0;
}

guint
prvGetAIOptData(ivca_option_t*option, guint channel)
{
	gchar key[64], *p, *str;
	gint m;

	p = key + sprintf(key, "cam.dvabx.opt.R%u.", channel);

	sprintf(p, "en_shadowrm");
	option->en_shadowrm = nf_sysdb_get_bool(key);
	sprintf(p, "en_prediction");
	option->en_prediction = nf_sysdb_get_bool(key);
	sprintf(p, "en_roi");
	option->en_roi = nf_sysdb_get_bool(key);
	sprintf(p, "en_tamper");
	option->en_tamper = nf_sysdb_get_bool(key);
	sprintf(p, "en_usecalib");
	option->en_usecalib = nf_sysdb_get_bool(key);
	sprintf(p, "en_snapshot");
	option->en_snapshot = nf_sysdb_get_bool(key);
	sprintf(p, "en_privacy");
	option->en_privacy= nf_sysdb_get_bool(key);
	sprintf(p, "min_objsize");
	str = nf_sysdb_get_str_nocopy(key);
	m = str ? sscanf(str, "%hd %hd", &option->min_width3d, &option->min_height3d) : 0;
	sprintf(p, "roi_xywh");
	str = nf_sysdb_get_str_nocopy(key);
	m = str ? sscanf(str, "%hd %hd %hd %hd", &option->roi.x, &option->roi.y,
			&option->roi.w, &option->roi.h) : 0;
	if ( m < 4 ) {
		option->roi.x = 120;
		option->roi.y = 90;
		option->roi.w = 3600;
		option->roi.h = 1980;
	}

	sprintf(p, "track_ref");
	option->track_ref = (guint8)nf_sysdb_get_uint(key);
	sprintf(p, "sw_obj_bb");
	option->sw_obj_bb = nf_sysdb_get_bool(key);
	sprintf(p, "sw_obj_id");
	option->sw_obj_id = nf_sysdb_get_bool(key);
	sprintf(p, "sw_obj_ar");
	option->sw_obj_ar = nf_sysdb_get_bool(key);
	sprintf(p, "sw_obj_tm");
	option->sw_obj_tm = nf_sysdb_get_bool(key);
	sprintf(p, "sw_obj_tr");
	option->sw_obj_tr = nf_sysdb_get_bool(key);
	sprintf(p, "sw_obj_cl");
	option->sw_obj_cl = nf_sysdb_get_bool(key);
	sprintf(p, "sw_rule");
	option->sw_rule = nf_sysdb_get_bool(key);
	sprintf(p, "sw_rule_name");
	option->sw_rule_name = nf_sysdb_get_bool(key);
	sprintf(p, "sw_roi");
	option->sw_roi = nf_sysdb_get_bool(key);

	sprintf(p, "sw_dbg_fg");
	option->sw_dbg_fg = nf_sysdb_get_bool(key);
	sprintf(p, "sw_dbg_sh");
	option->sw_dbg_sh = nf_sysdb_get_bool(key);
	sprintf(p, "sw_dbg_info");
	option->sw_dbg_info = nf_sysdb_get_bool(key);

	sprintf(p, "sw_obj_s3d");
	option->sw_obj_s3d= nf_sysdb_get_bool(key);
	sprintf(p, "sw_obj_w3d");
	option->sw_obj_w3d = nf_sysdb_get_bool(key);
	sprintf(p, "sw_obj_h3d");
	option->sw_obj_h3d = nf_sysdb_get_bool(key);
	sprintf(p, "en_static_filter");
	option->en_static_filter= nf_sysdb_get_bool(key);
	sprintf(p, "static_filter_sense");
	option->static_filter_sense = (guint8)nf_sysdb_get_uint(key);
	
	return 0;
}

guint
prvGetFROptData(fr_lpr_option_t *option, guint channel)
{
	gchar key[64], *p, *str;
	gint m;

	p = key + sprintf(key, "cam.dvabx.face.opt.R%u.", channel);

	sprintf(p, "sw_obj_bb");
	option->sw_obj_bb = nf_sysdb_get_bool(key);
	sprintf(p, "sw_grp_name");
	option->sw_grp_name = nf_sysdb_get_bool(key);
	sprintf(p, "sw_rule");
	option->sw_rule = nf_sysdb_get_bool(key);
	sprintf(p, "sw_rule_name");
	option->sw_rule_name = nf_sysdb_get_bool(key);
	
	return 0;
}


guint
prvGetLPROptData(fr_lpr_option_t *option, guint channel)
{
	gchar key[64], *p, *str;
	gint m;

	p = key + sprintf(key, "cam.dvabx.plateno.opt.R%u.", channel);

	sprintf(p, "sw_obj_bb");
	option->sw_obj_bb = nf_sysdb_get_bool(key);
	sprintf(p, "sw_grp_name");
	option->sw_grp_name = nf_sysdb_get_bool(key);
	sprintf(p, "sw_rule");
	option->sw_rule = nf_sysdb_get_bool(key);
	sprintf(p, "sw_rule_name");
	option->sw_rule_name = nf_sysdb_get_bool(key);
	sprintf(p, "sw_plate_number");
	option->sw_plate_number = nf_sysdb_get_bool(key);
	
	return 0;
}


guint
prvGetAIRuleData(ivca_rule_t*rule, guint channel)
{
	GValue ret_value = {0,};
	guint n, m, i;
	gchar key[64], *p, *str, *last, *tokx, *toky;
	ivca_zone_t*z = rule->zonelist;
	ivca_cntr_t*c = rule->cntrlist;

	p = key + sprintf(key, "cam.dvabx.rule.R%u.", channel);

	sprintf(p, "n_width");
	rule->n_width = (guint16)nf_sysdb_get_uint(key);
	sprintf(p, "n_height");
	rule->n_height = (guint16)nf_sysdb_get_uint(key);
	sprintf(p, "nzones");
	rule->nzones = (guint16)nf_sysdb_get_uint(key);
	sprintf(p, "ncounters");
	rule->ncntrs = (guint16)nf_sysdb_get_uint(key);

	for (n = 0; n < IVCA_MAX_ZONES; n++, z++) {
		p = key + sprintf(key, "cam.dvabx.rule.R%u.Z%u.", channel, n);

		sprintf(p, "name");
		str = nf_sysdb_get_str_nocopy(key);
		strncpy(z->name, str ? str : "", sizeof(z->name));
		sprintf(p, "id");
		z->id = (gint16)nf_sysdb_get_int(key);
		sprintf(p, "type");
		z->type = (guint8)nf_sysdb_get_uint(key);
		sprintf(p, "active");
		z->active = (guint8)nf_sysdb_get_bool(key);
		sprintf(p, "enabled");
		z->enabled = nf_sysdb_get_uint(key);
		sprintf(p, "time_sarlf");
		str = nf_sysdb_get_str_nocopy(key);
		m = str ? (guint)sscanf(str, "%hu %hu %hu %hu %hu", &z->stop_time,
				&z->abandon_time, &z->remove_time, &z->loiter_time,&z->fall_time) : 0;
		if ( m < 5 )
			z->stop_time = z->abandon_time =
					z->remove_time = z->loiter_time = z->fall_time= 5;
		sprintf(p, "f_colorsens");
		*(guint *)z->ecolor = (0x00ffffff & nf_sysdb_get_uint(key));
		z->ecolor_sens = (0xff000000 & nf_sysdb_get_uint(key)) >>24;
		sprintf(p, "f_size");
		str = nf_sysdb_get_str_nocopy(key);
		m = str ? (guint)sscanf(str, "%hu %hu %hu %hu", &z->size_min[0],
				&z->size_min[1], &z->size_max[0], &z->size_max[1]) : 0;
		if ( m < 4 )
			z->size_min[0] = z->size_min[0] =
					z->size_max[0] = z->size_max[0] = 0;
		sprintf(p, "f_speed");
		str = nf_sysdb_get_str_nocopy(key);
		m = str ? (guint)sscanf(str, "%hu %hu", &z->speed_min, &z->speed_max) :
				0;
		if ( m < 2 )
			z->speed_min = z->speed_max = 0;
		sprintf(p, "f_class");
		z->eclass = (guint8)nf_sysdb_get_uint(key);
		sprintf(p, "d_color");
		m = nf_sysdb_get_uint(key);
		z->color[0] = (guint8)m;
		z->color[1] = (guint8)(m >> 8);
		z->color[2] = (guint8)(m >> 16);
		sprintf(p, "npts");
		z->npts = (guint8)nf_sysdb_get_uint(key);
		sprintf(p, "pt");
		i = 0;
		str = nf_sysdb_get_str(key);
		if ( str ) {
			for (last = str, i = 0; i < z->npts; i++) {
				tokx = strtok_r(last, " \t,.", &last);
				if ( !tokx )
					break;
				toky = strtok_r(last, " \t,.", &last);
				if ( !toky )
					break;
				z->pt[i].x = (gint16)atoi(tokx);
				z->pt[i].y = (gint16)atoi(toky);
			}
			if ( i < z->npts ) {
				i = 0;
				z->npts = 0;
			}
			free(str);
		}
		for ( ; i < IVCA_MAX_PTSPERZONE; i++)
			z->pt[i].x = z->pt[i].y = 0;

		sprintf(p, "sensitivity");
		z->sensitivity = nf_sysdb_get_uint(key);

		sprintf(p, "all_detect_obj");
		z->all_detect_obj = (guint8)nf_sysdb_get_bool(key);

		sprintf(p, "c_threshold");
		z->c_threshold = (guint8)nf_sysdb_get_uint(key);
		
		/*
		sprintf(p, "interest_obj");
		if(nf_sysdb_get_key2(key, channel, i, &ret_value, NULL))
		{
			z->reserved2[0] = 0;

			if (strstr(g_value_get_string(&ret_value), "person:1")) 
				z->reserved2[0] = z->reserved2[0] | 0x01;
			if (strstr(g_value_get_string(&ret_value), "bicycle:1") &&
				strstr(g_value_get_string(&ret_value), "motorbike:1") &&
				strstr(g_value_get_string(&ret_value), "bus:1") &&
				strstr(g_value_get_string(&ret_value), "car:1")) {
				z->reserved2[0] = z->reserved2[0] | 0x02;
			}
			g_value_unset(&ret_value);
		}
		*/
	}

	for (n = 0; n < IVCA_MAX_CNTRS; n++, c++) {
		p = key + sprintf(key, "cam.dvabx.rule.R%u.C%u.", channel, n);

		sprintf(p, "name");
		str = nf_sysdb_get_str_nocopy(key);
		strncpy(c->name, str ? str : "", sizeof(c->name));
		sprintf(p, "id");
		c->id = (gint16)nf_sysdb_get_int(key);
		sprintf(p, "active");
		c->active = (guint8)nf_sysdb_get_bool(key);
		sprintf(p, "enabled");
		c->enabled = nf_sysdb_get_uint(key);
		sprintf(p, "zid");
		str = nf_sysdb_get_str_nocopy(key);
		m = str ? (guint)sscanf(str, "%hd %hd", &c->zid_up, &c->zid_dn) : 0;
		if ( m < 2 )
			c->zid_up = c->zid_dn = -1;
		sprintf(p, "e_value");
		c->evalue = nf_sysdb_get_int(key);
		sprintf(p, "reset_alert");
		c->resetalert = (guint8)nf_sysdb_get_bool(key);
		sprintf(p, "d_color");
		m = nf_sysdb_get_uint(key);
		c->color[0] = (guint8)m;
		c->color[1] = (guint8)(m >> 8);
		c->color[2] = (guint8)(m >> 16);
		sprintf(p, "pt");
		str = nf_sysdb_get_str_nocopy(key);
		m = str ? (guint)sscanf(str, "%hu %hu %hu %hu %hu %hu %hu %hu",
				&c->pt[0].x, &c->pt[0].y, &c->pt[1].x, &c->pt[1].y,
				&c->pt[2].x, &c->pt[2].y, &c->pt[3].x, &c->pt[3].y) : 0;
		if ( m < 8 )
			memset(c->pt, 0, sizeof(c->pt));
	}

	return 0;
}

guint
prvGetFRRuleData(fr_lpr_rule_t*rule, guint channel)
{
	GValue ret_value = {0,};
	guint n, m, i;
	gchar key[64], *p, *str, *last, *tokx, *toky;
	fr_lpr_zone_t*z = rule->zonelist;

	p = key + sprintf(key, "cam.dvabx.face.rule.R%u.", channel);

	sprintf(p, "nzones");
	rule->nzones = (guint16)nf_sysdb_get_uint(key);

	for (n = 0; n < FR_LPR_MAX_ZONES; n++, z++) {
		p = key + sprintf(key, "cam.dvabx.face.rule.R%u.Z%u.", channel, n);

		sprintf(p, "name");
		str = nf_sysdb_get_str_nocopy(key);
		strncpy(z->name, str ? str : "", sizeof(z->name));
		sprintf(p, "id");
		z->id = (gint16)nf_sysdb_get_int(key);
		sprintf(p, "type");
		z->type = (guint8)nf_sysdb_get_uint(key);
		sprintf(p, "act");
		z->active = (guint8)nf_sysdb_get_bool(key);
		sprintf(p, "d_color");
		m = nf_sysdb_get_uint(key);
		z->color[0] = (guint8)m;
		z->color[1] = (guint8)(m >> 8);
		z->color[2] = (guint8)(m >> 16);
		sprintf(p, "npts");
		z->npts = (guint8)nf_sysdb_get_uint(key);
		sprintf(p, "pt");
		i = 0;
		str = nf_sysdb_get_str(key);
		if ( str ) {
			for (last = str, i = 0; i < z->npts; i++) {
				tokx = strtok_r(last, " \t,.", &last);
				if ( !tokx )
					break;
				toky = strtok_r(last, " \t,.", &last);
				if ( !toky )
					break;
				z->pt[i].x = (gint16)atoi(tokx);
				z->pt[i].y = (gint16)atoi(toky);
			}
			if ( i < z->npts ) {
				i = 0;
				z->npts = 0;
			}
			free(str);
		}
		for ( ; i < IVCA_MAX_PTSPERZONE; i++)
			z->pt[i].x = z->pt[i].y = 0;

		sprintf(p, "threshold");
		z->c_threshold = (guint8)nf_sysdb_get_uint(key);
		sprintf(p, "group_filter");
		str = nf_sysdb_get_str_nocopy(key);
		strncpy(z->group_filter, str ? str : "", sizeof(z->group_filter));
	
	}

	return 0;
}


guint
prvGetLPRRuleData(fr_lpr_rule_t*rule, guint channel)
{
	GValue ret_value = {0,};
	guint n, m, i;
	gchar key[64], *p, *str, *last, *tokx, *toky;
	fr_lpr_zone_t*z = rule->zonelist;

	p = key + sprintf(key, "cam.dvabx.plateno.rule.R%u.", channel);

	sprintf(p, "nzones");
	rule->nzones = (guint16)nf_sysdb_get_uint(key);

	for (n = 0; n < FR_LPR_MAX_ZONES; n++, z++) {
		p = key + sprintf(key, "cam.dvabx.plateno.rule.R%u.Z%u.", channel, n);

		sprintf(p, "name");
		str = nf_sysdb_get_str_nocopy(key);
		strncpy(z->name, str ? str : "", sizeof(z->name));
		sprintf(p, "id");
		z->id = (gint16)nf_sysdb_get_int(key);
		sprintf(p, "type");
		z->type = (guint8)nf_sysdb_get_uint(key);
		sprintf(p, "act");
		z->active = (guint8)nf_sysdb_get_bool(key);
		sprintf(p, "d_color");
		m = nf_sysdb_get_uint(key);
		z->color[0] = (guint8)m;
		z->color[1] = (guint8)(m >> 8);
		z->color[2] = (guint8)(m >> 16);
		sprintf(p, "npts");
		z->npts = (guint8)nf_sysdb_get_uint(key);
		sprintf(p, "pt");
		i = 0;
		str = nf_sysdb_get_str(key);
		if ( str ) {
			for (last = str, i = 0; i < z->npts; i++) {
				tokx = strtok_r(last, " \t,.", &last);
				if ( !tokx )
					break;
				toky = strtok_r(last, " \t,.", &last);
				if ( !toky )
					break;
				z->pt[i].x = (gint16)atoi(tokx);
				z->pt[i].y = (gint16)atoi(toky);
			}
			if ( i < z->npts ) {
				i = 0;
				z->npts = 0;
			}
			free(str);
		}
		for ( ; i < IVCA_MAX_PTSPERZONE; i++)
			z->pt[i].x = z->pt[i].y = 0;

		sprintf(p, "threshold");
		z->c_threshold = (guint8)nf_sysdb_get_uint(key);
		
		sprintf(p, "grp_mask");
		str = nf_sysdb_get_str_nocopy(key);
		{
			char tmp[10];
			memset(tmp,0x00,10);
			z->grp_mask = 0;
			strcpy(tmp,str);
			for(m=0;m<9;m++){
				if(tmp[m] =='1')
					z->grp_mask |= (1 << m);
			}		
		}
		
	}
	return 0;
}


guint
prvGetAICalibData(ivca_calib_t*calib, guint channel)
{
	guint n, m, i;
	gchar key[64], *p, *str, *last, *tokx, *toky;
	ivca_calib_target_t *t = calib->targetlist;

	p = key + sprintf(key, "cam.dvabx.calib.R%u.", channel);

	sprintf(p, "focal");
	str = nf_sysdb_get_str_nocopy(key);
	str ? (guint)sscanf(str, "%f", &calib->focal) : 0;
	sprintf(p, "height");
	str = nf_sysdb_get_str_nocopy(key);
	str ? (guint)sscanf(str, "%f", &calib->height) : 0;
	sprintf(p, "tilt");
	str = nf_sysdb_get_str_nocopy(key);
	str ? (guint)sscanf(str, "%f", &calib->tilt) : 0;

	sprintf(p, "ntargets");
	calib->ntargets = (guint16)nf_sysdb_get_uint(key);
	sprintf(p, "p_height");
	calib->p_height = (guint16)nf_sysdb_get_uint(key);
	sprintf(p, "p_width");
	calib->p_width = (guint16)nf_sysdb_get_uint(key);
	sprintf(p, "paramvalid");
	calib->paramvalid = (guint8)nf_sysdb_get_bool(key);

	for (i = 0; i < IVCA_MAX_CALIB_TARGETS; i++ , t++) {
		p = key + sprintf(key, "cam.dvabx.calib.R%u.T%u.", channel, i);

		sprintf(p, "pt");
		str = nf_sysdb_get_str_nocopy(key);
		m = str ? (guint)sscanf(str, "%hu %hu %hu %hu", &t->pt[0].x, &t->pt[0].y, &t->pt[1].x, &t->pt[1].y) : 0;
		//t->pt[0].x = t->pt[0].x*6;
		//t->pt[0].y = t->pt[0].y*6;
		//t->pt[1].x = t->pt[1].x*6;
		//t->pt[1].y = t->pt[1].y*6;
		sprintf(p, "height");
		t->height = (guint16)nf_sysdb_get_uint(key);

	}

	return 0;
}
//static
guint
prvGetVCACalibData(ivca_calib_t*calib, guint channel)
{
	guint n, m, i;
	gchar key[64], *p, *str, *last, *tokx, *toky;
	ivca_calib_target_t *t = calib->targetlist;

	p = key + sprintf(key, "cam.vca.calib.R%u.", channel);

	sprintf(p, "focal");
	str = nf_sysdb_get_str_nocopy(key);
	str ? (guint)sscanf(str, "%f", &calib->focal) : 0;
	sprintf(p, "height");
	str = nf_sysdb_get_str_nocopy(key);
	str ? (guint)sscanf(str, "%f", &calib->height) : 0;
	sprintf(p, "tilt");
	str = nf_sysdb_get_str_nocopy(key);
	str ? (guint)sscanf(str, "%f", &calib->tilt) : 0;

	sprintf(p, "ntargets");
	calib->ntargets = (guint16)nf_sysdb_get_uint(key);
	sprintf(p, "p_height");
	calib->p_height = (guint16)nf_sysdb_get_uint(key);
	sprintf(p, "p_width");
	calib->p_width = (guint16)nf_sysdb_get_uint(key);
	sprintf(p, "paramvalid");
	calib->paramvalid = (guint8)nf_sysdb_get_bool(key);

	for (i = 0; i < IVCA_MAX_CALIB_TARGETS; i++ , t++) {
		p = key + sprintf(key, "cam.vca.calib.R%u.T%u.", channel, i);

		sprintf(p, "pt");
		str = nf_sysdb_get_str_nocopy(key);
		m = str ? (guint)sscanf(str, "%hu %hu %hu %hu", &t->pt[0].x, &t->pt[0].y, &t->pt[1].x, &t->pt[1].y) : 0;
		//t->pt[0].x = t->pt[0].x*6;
		//t->pt[0].y = t->pt[0].y*6;
		//t->pt[1].x = t->pt[1].x*6;
		//t->pt[1].y = t->pt[1].y*6;
		sprintf(p, "height");
		t->height = (guint16)nf_sysdb_get_uint(key);

	}

	return 0;
}
guint
DAL_get_vca_setup_data_all(ivca_option_t*option, ivca_rule_t*rule, ivca_calib_t*calib, guint ch_num)
{
	guint ch;

//	if ( option_gbl ) {
//		memset(option_gbl, 0, sizeof(VCAOptGBLData));
//		prvGetVCAOptGblData(option_gbl);
//	}
	if ( option ) {
		memset(option, 0, sizeof(ivca_option_t) * ch_num);
		for (ch = 0; ch < ch_num; ch++)
			prvGetVCAOptData(&option[ch], ch);
	}
	if ( rule ) {
		memset(rule, 0, sizeof(ivca_rule_t) * ch_num);
		for (ch = 0; ch < ch_num; ch++)
			prvGetVCARuleData(&rule[ch], ch);
	}
	if ( calib ) {
		memset(calib, 0, sizeof(ivca_calib_t) * ch_num);
		for (ch = 0; ch < ch_num; ch++)
			prvGetVCACalibData(&calib[ch], ch);
	}
	return 0;
}	/* DAL_get_vca_setup_data_all(... */


static guint
prvSetVCAOptData(ivca_option_t*option, guint channel)
{
	gchar key[64], *p, buf[64];
	gchar result = 1;

	p = key + sprintf(key, "cam.vca.opt.R%u.", channel);

	sprintf(p, "en_shadowrm");
	if ( !nf_sysdb_set_bool(key, option->en_shadowrm) )
		return 1;
	sprintf(p, "en_prediction");
	if ( !nf_sysdb_set_bool(key, option->en_prediction) )
		return 2;
	sprintf(p, "en_roi");
	if ( !nf_sysdb_set_bool(key, option->en_roi) )
		return 3;
	sprintf(p, "en_tamper");
	if ( !nf_sysdb_set_bool(key, option->en_tamper) )
		return 4;
	sprintf(p, "en_snapshot");
	if ( !nf_sysdb_set_bool(key, option->en_snapshot) )
		return 5;
	sprintf(p, "en_usecalib");
	if ( !nf_sysdb_set_bool(key, option->en_usecalib) )
		return 6;
	sprintf(p, "en_privacy");
	if ( !nf_sysdb_set_bool(key, option->en_privacy) )
		return 7;
	sprintf(p, "min_objsize");
	sprintf(buf, "%hu %hu", option->min_width3d, option->min_height3d);
	if ( !nf_sysdb_set_str(key, buf) )
		return 8;
	sprintf(p, "roi_xywh");
	sprintf(buf, "%hu %hu %hu %hu", option->roi.x, option->roi.y,
			option->roi.w, option->roi.h);
	if ( !nf_sysdb_set_str(key, buf) )
		return 9;

	sprintf(p, "track_ref");
	if ( !nf_sysdb_set_uint(key, option->track_ref) )
		return 10;
	sprintf(p, "sw_obj_bb");
	if ( !nf_sysdb_set_bool(key, option->sw_obj_bb) )
		return 11;
	sprintf(p, "sw_obj_id");
	if ( !nf_sysdb_set_bool(key, option->sw_obj_id) )
		return 12;
	sprintf(p, "sw_obj_ar");
	if ( !nf_sysdb_set_bool(key, option->sw_obj_ar) )
		return 13;
	sprintf(p, "sw_obj_tm");
	if ( !nf_sysdb_set_bool(key, option->sw_obj_tm) )
		return 14;
	sprintf(p, "sw_obj_tr");
	if ( !nf_sysdb_set_bool(key, option->sw_obj_tr) )
		return 15;
	sprintf(p, "sw_rule");
	if ( !nf_sysdb_set_bool(key, option->sw_rule) )
		return 16;
	sprintf(p, "sw_rule_name");
	if ( !nf_sysdb_set_bool(key, option->sw_rule_name) )
		return 17;
	sprintf(p, "sw_roi");
	if ( !nf_sysdb_set_bool(key, option->sw_roi) )
		return 18;
	sprintf(p, "sw_dbg_fg");
	if ( !nf_sysdb_set_bool(key, option->sw_dbg_fg) )
		return 19;
	sprintf(p, "sw_dbg_sh");
	if ( !nf_sysdb_set_bool(key, option->sw_dbg_sh) )
		return 20;
	sprintf(p, "sw_dbg_info");
	if ( !nf_sysdb_set_bool(key, option->sw_dbg_info) )
		return 21;

	sprintf(p, "sw_obj_s3d");
	if ( !nf_sysdb_set_bool(key, option->sw_obj_s3d) )
		return 22;
	sprintf(p, "sw_obj_w3d");
	if ( !nf_sysdb_set_bool(key, option->sw_obj_w3d) )
		return 23;
	sprintf(p, "sw_obj_h3d");
	if ( !nf_sysdb_set_bool(key, option->sw_obj_h3d) )
		return 24;

	return 0;
}

static guint
prvSetVCARuleData(ivca_rule_t*rule, guint channel)
{
	guint n, i, j, ebase;
	gchar key[64], *p, buf[256];
	ivca_zone_t*z = rule->zonelist;
	ivca_cntr_t*c = rule->cntrlist;

	p = key + sprintf(key, "cam.vca.rule.R%u.", channel);

	sprintf(p, "n_width");
	if ( !nf_sysdb_set_uint(key, rule->n_width) )
		return 1;
	sprintf(p, "n_height");
	if ( !nf_sysdb_set_uint(key, rule->n_height) )
		return 2;
	sprintf(p, "nzones");
	if ( !nf_sysdb_set_uint(key, rule->nzones) )
		return 1;
	sprintf(p, "ncounters");
	if ( !nf_sysdb_set_uint(key, rule->ncntrs) )
		return 2;

	for (n = 0; n < IVCA_MAX_ZONES; n++, z++) {
		ebase = 3 + n * 13;
		p = key + sprintf(key, "cam.vca.rule.R%u.Z%u.", channel, n);

		sprintf(p, "name");
		if ( !nf_sysdb_set_str(key, z->name) )
			return ebase + 0;
		sprintf(p, "id");
		if ( !nf_sysdb_set_int(key, (gint)z->id) )
			return ebase + 1;
		sprintf(p, "type");
		if ( !nf_sysdb_set_uint(key, z->type) )
			return ebase + 2;
		sprintf(p, "active");
		if ( !nf_sysdb_set_bool(key, z->active) )
			return ebase + 3;
		sprintf(p, "enabled");
		if ( !nf_sysdb_set_uint(key, z->enabled) )
			return ebase + 4;
		sprintf(p, "time_sarlf");
		sprintf(buf, "%hu %hu %hu %hu %hu", z->stop_time, z->abandon_time,
				z->remove_time, z->loiter_time,z->fall_time);
		if ( !nf_sysdb_set_str(key, buf) )
			return ebase + 5;
		sprintf(p, "f_colorsens");
		if ( !nf_sysdb_set_uint(key, (0x00ffffff & (*(guint *)z->ecolor)) + (z->ecolor_sens <<24)) )
			return ebase + 6;
		sprintf(p, "f_size");
		sprintf(buf, "%hu %hu %hu %hu", z->size_min[0], z->size_min[1],
				z->size_max[0], z->size_max[1]);
		if ( !nf_sysdb_set_str(key, buf) )
			return ebase + 7;
		sprintf(p, "f_speed");
		sprintf(buf, "%hu %hu", z->speed_min, z->speed_max);
		if ( !nf_sysdb_set_str(key, buf) )
			return ebase + 8;
		sprintf(p, "f_class");
		if ( !nf_sysdb_set_uint(key, z->eclass) )
			return ebase + 9;
		sprintf(p, "d_color");
		if ( !nf_sysdb_set_uint(key, *(guint *)z->color & 0x00FFFFFF) )
			return ebase + 10;
		sprintf(p, "npts");
		if ( !nf_sysdb_set_uint(key, z->npts) )
			return ebase + 11;
		sprintf(p, "pt");
		for (buf[0] = '\0', p = buf, i = 0; i < z->npts; i++)
			p += sprintf(p, "%s%hu %hu", i ? " " : "", z->pt[i].x, z->pt[i].y);
		if ( !nf_sysdb_set_str(key, buf) )
			return ebase + 12;
		//captainnn
		//sprintf(p, "sensitivity");
		//if ( !nf_sysdb_set_uint(key, z->sensitivity) )
		//	return ebase + 4;
	}

	for (n = 0; n < IVCA_MAX_CNTRS; n++, c++) {
		ebase = 3 + IVCA_MAX_ZONES * 13 + n * 9;
		p = key + sprintf(key, "cam.vca.rule.R%u.C%u.", channel, n);

		sprintf(p, "name");
		if ( !nf_sysdb_set_str(key, c->name) )
			return ebase + 0;
		sprintf(p, "id");
		if ( !nf_sysdb_set_int(key, (gint)c->id) )
			return ebase + 1;
		sprintf(p, "active");
		if ( !nf_sysdb_set_bool(key, c->active) )
			return ebase + 2;
		sprintf(p, "enabled");
		if ( !nf_sysdb_set_uint(key, c->enabled) )
			return ebase + 3;
		sprintf(p, "zid");
		sprintf(buf, "%hd %hd", c->zid_up, c->zid_dn);
		if ( !nf_sysdb_set_str(key, buf) )
			return ebase + 4;
		sprintf(p, "e_value");
		if ( !nf_sysdb_set_int(key, c->evalue) )
			return ebase + 5;
		sprintf(p, "reset_alert");
		if ( !nf_sysdb_set_bool(key, c->resetalert) )
			return ebase + 6;
		sprintf(p, "d_color");
		if ( !nf_sysdb_set_uint(key, *(guint *)c->color & 0x00FFFFFF) )
			return ebase + 7;
		sprintf(p, "pt");
		j = n < rule->ncntrs ? (guint)4 : 0;
		for (buf[0] = '\0', p = buf, i = 0; i < j; i++)
			p += sprintf(p, "%s%hu %hu", i ? " " : "", c->pt[i].x, c->pt[i].y);
		if ( !nf_sysdb_set_str(key, buf) )
			return ebase + 8;
	}

	return 0;
}


guint
DAL_get_vca_calib_data_all(ivca_calib_t*calib, guint ch_num)
{
	guint ch;

	if ( calib ) {
		memset(calib, 0, sizeof(ivca_calib_t) * ch_num);
		for (ch = 0; ch < ch_num; ch++)
			prvGetVCACalibData(&calib[ch], ch);
	}
	return 0;
}	/* DAL_get_vca_calib_data_all(... */

static guint
prvSetVCACalibData(ivca_calib_t *calib, guint channel)
{
	guint n, i, j, ebase;
	gchar key[64], *p, buf[256];
	ivca_calib_target_t *t = calib->targetlist;

	p = key + sprintf(key, "cam.vca.calib.R%u.", channel);

	sprintf(p, "focal");
	sprintf(buf, "%f", calib->focal);
	if ( !nf_sysdb_set_str(key, buf) )
		return 1;
	sprintf(p, "height");
	sprintf(buf, "%f", calib->height);
	if ( !nf_sysdb_set_str(key, buf) )
		return 2;
	sprintf(p, "tilt");
	sprintf(buf, "%f", calib->tilt);
	if ( !nf_sysdb_set_str(key, buf) )
		return 3;

	sprintf(p, "ntargets");
	if ( !nf_sysdb_set_uint(key, calib->ntargets) )
		return 4;
	sprintf(p, "p_height");
	if ( !nf_sysdb_set_uint(key, calib->p_height) )
		return 5;
	sprintf(p, "p_width");
	if ( !nf_sysdb_set_uint(key, calib->p_width) )
		return 6;
	sprintf(p, "paramvalid");
	if ( !nf_sysdb_set_bool(key, calib->paramvalid) )
		return 7;

	for (i = 0; i < IVCA_MAX_CALIB_TARGETS; i++, t++) {
		p = key + sprintf(key, "cam.vca.calib.R%u.T%u.", channel,i);
		ebase = 8 + i * 2;

		sprintf(p, "pt");
		sprintf(buf, "%hu %hu %hu %hu", t->pt[0].x, t->pt[0].y,
				t->pt[1].x, t->pt[1].y);
		if ( !nf_sysdb_set_str(key, buf) )
			return ebase;

		sprintf(p, "height");
		if ( !nf_sysdb_set_uint(key, t->height) )
			return ebase + 1;
	}

	return 0;
}
guint
DAL_set_vca_calib_data_all(ivca_calib_t *calib, guint ch_num)
{
	guint ch, ret = 0;

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	if ( calib ) {
		for (ch = 0; ch < ch_num; ch++)
			ret += prvSetVCACalibData(&calib[ch], ch);
	}
	printf("DAL_set_vca_calib_data_all ret %d \n",ret);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	return ret;
}	/* DAL_set_vca_calib_data_all(... */
guint
DAL_set_vca_setup_data_all(ivca_option_t*option, ivca_rule_t*rule, ivca_calib_t*calib, guint ch_num)
{
	guint ch, ret = 0;

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

//	if ( option_gbl )
	//	ret += prvSetVCAOptGblData(option_gbl);
	if ( option ) {
		for (ch = 0; ch < ch_num; ch++)
			ret += prvSetVCAOptData(&option[ch], ch);
	}
	printf("DAL_set_vca_setup_data_all ret %d \n",ret);
	if ( rule ) {
		for (ch = 0; ch < ch_num; ch++)
			ret += prvSetVCARuleData(&rule[ch], ch);
	}
	printf("DAL_set_vca_setup_data_all ret %d \n",ret);

	if ( calib ) {
		for (ch = 0; ch < ch_num; ch++)
			ret = prvSetVCACalibData(&calib[ch], ch);
	}
	printf("DAL_set_vca_setup_data_all ret %d \n",ret);

	//nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
	return ret;
}	/* DAL_set_vca_setup_data_all(... */

static guint _get_dva_prop_idz_data(DVAIdzData *data, guint channel)
{
	GValue ret_value = {0,};

	if (nf_sysdb_get_key1("cam.dva.D%d.idz.act", channel, &ret_value, NULL)) {
		data->active = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else {
		return 0;
	}

	if (nf_sysdb_get_key1("cam.dva.D%d.idz.human.item_cnt", channel, &ret_value, NULL)) {
		data->human_item_cnt = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else {
		return 0;
	}	

	if (nf_sysdb_get_key1("cam.dva.D%d.idz.human.item_list", channel, &ret_value, NULL))
	{
		g_stpcpy(data->human_item_list, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else {
		return 0;
	}	

	if (nf_sysdb_get_key1("cam.dva.D%d.idz.vehicle.item_cnt", channel, &ret_value, NULL)) {
		data->vehicle_item_cnt = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else {
		return 0;
	}	

	if (nf_sysdb_get_key1("cam.dva.D%d.idz.vehicle.item_list", channel, &ret_value, NULL))
	{
		g_stpcpy(data->vehicle_item_list, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else {
		return 0;
	}	

	if (nf_sysdb_get_key1("cam.dva.D%d.idz.animal.item_cnt", channel, &ret_value, NULL)) {
		data->animal_item_cnt = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else {
		return 0;
	}	

	if (nf_sysdb_get_key1("cam.dva.D%d.idz.animal.item_list", channel, &ret_value, NULL))
	{
		g_stpcpy(data->animal_item_list, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else {
		return 0;
	}		

	if (nf_sysdb_get_key1("cam.dva.D%d.idz.en_static_filter", channel, &ret_value, NULL)) {
		data->en_static_filter = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else {
		return 0;
	}

	if (nf_sysdb_get_key1("cam.dva.D%d.idz.static_filter_sense", channel, &ret_value, NULL)) {
		data->static_filter_sense = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else {
		return 0;
	}

	if (nf_sysdb_get_key1("cam.dva.D%d.idz.counter.active", channel, &ret_value, NULL)) {
		data->cntr.active = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else {
		return 0;
	}

	if (nf_sysdb_get_key1("cam.dva.D%d.idz.counter.d_color", channel, &ret_value, NULL)) {
		data->cntr.display_color = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else {
		return 0;
	}	

	if (nf_sysdb_get_key1("cam.dva.D%d.idz.counter.noti", channel, &ret_value, NULL)) {
		data->cntr.notification = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else {
		return 0;
	}

	if (nf_sysdb_get_key1("cam.dva.D%d.idz.counter.e_value", channel, &ret_value, NULL)) {
		data->cntr.e_value = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else {
		return 0;
	}	

	if (nf_sysdb_get_key1("cam.dva.D%d.idz.counter.reset", channel, &ret_value, NULL)) {
		data->cntr.reset = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else {
		return 0;
	}

	if (nf_sysdb_get_key1("cam.dva.D%d.idz.c_threshold", channel, &ret_value, NULL)) {
		data->confidence_threshold = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else {
		return 0;
	}

	return 0;
}

static guint _get_dva_prop_ipz_data(DVAIpzData *data, guint channel)
{
	GValue ret_value = {0,};

	if (nf_sysdb_get_key1("cam.dva.D%d.ipz.act", channel, &ret_value, NULL)) {
		data->active = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else {
		return 0;
	}

	if (nf_sysdb_get_key1("cam.dva.D%d.ipz.item_cnt", channel, &ret_value, NULL)) {
		data->item_cnt = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else {
		return 0;
	}	

	if (nf_sysdb_get_key1("cam.dva.D%d.ipz.item_list", channel, &ret_value, NULL))
	{
		g_stpcpy(data->item_list, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else {
		return 0;
	}	

	if (nf_sysdb_get_key1("cam.dva.D%d.ipz.dwell", channel, &ret_value, NULL)) {
		data->dwell = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else {
		return 0;
	}	

	return 0;
}

static guint _get_dva_prop_lpr_data(DVALprData *data, guint channel)
{
	GValue ret_value = {0,};

	if (nf_sysdb_get_key1("cam.dva.D%d.lpr.act", channel, &ret_value, NULL)) {
		data->active = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else {
		return 0;
	}

	if (nf_sysdb_get_key1("cam.dva.D%d.lpr.white.item_cnt", channel, &ret_value, NULL)) {
		data->white_item_cnt = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else {
		return 0;
	}	

	if (nf_sysdb_get_key1("cam.dva.D%d.lpr.white.item_list", channel, &ret_value, NULL))
	{
		g_stpcpy(data->white_item_list, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else {
		return 0;
	}	

	if (nf_sysdb_get_key1("cam.dva.D%d.lpr.black.item_cnt", channel, &ret_value, NULL)) {
		data->black_item_cnt = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else {
		return 0;
	}	

	if (nf_sysdb_get_key1("cam.dva.D%d.lpr.black.item_list", channel, &ret_value, NULL))
	{
		g_stpcpy(data->black_item_list, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else {
		return 0;
	}	
	
	return 0;
}

static guint _set_dva_prop_idz_data(DVAIdzData data, guint channel)
{
	GValue set_value = {0,};

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.active);
	if(!nf_sysdb_set_key1("cam.dva.D%d.idz.act", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		return 0;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.human_item_cnt);
	if(!nf_sysdb_set_key1("cam.dva.D%d.idz.human.item_cnt", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		return 0;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.human_item_list);
	if(!nf_sysdb_set_key1("cam.dva.D%d.idz.human.item_list", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		return 0;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.vehicle_item_cnt);
	if(!nf_sysdb_set_key1("cam.dva.D%d.idz.vehicle.item_cnt", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		return 0;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.vehicle_item_list);
	if(!nf_sysdb_set_key1("cam.dva.D%d.idz.vehicle.item_list", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		return 0;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.animal_item_cnt);
	if(!nf_sysdb_set_key1("cam.dva.D%d.idz.animal.item_cnt", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		return 0;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.animal_item_list);
	if(!nf_sysdb_set_key1("cam.dva.D%d.idz.animal.item_list", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);	
		return 0;
	}
	g_value_unset(&set_value);		

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.en_static_filter);
	if(!nf_sysdb_set_key1("cam.dva.D%d.idz.en_static_filter", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		return 0;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.static_filter_sense);
	if(!nf_sysdb_set_key1("cam.dva.D%d.idz.static_filter_sense", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		return 0;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.cntr.active);
	if(!nf_sysdb_set_key1("cam.dva.D%d.idz.counter.active", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		return 0;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.cntr.display_color);
	if(!nf_sysdb_set_key1("cam.dva.D%d.idz.counter.d_color", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		return 0;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.cntr.notification);
	if(!nf_sysdb_set_key1("cam.dva.D%d.idz.counter.noti", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		return 0;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.cntr.e_value);
	if(!nf_sysdb_set_key1("cam.dva.D%d.idz.counter.e_value", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		return 0;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.cntr.reset);
	if(!nf_sysdb_set_key1("cam.dva.D%d.idz.counter.reset", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		return 0;
	}
	g_value_unset(&set_value);	

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.confidence_threshold);
	if(!nf_sysdb_set_key1("cam.dva.D%d.idz.c_threshold", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		return 0;
	}
	g_value_unset(&set_value);

	return 0;
}

static guint _set_dva_prop_ipz_data(DVAIpzData data, guint channel)
{
	GValue set_value = {0,};

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.active);
	if(!nf_sysdb_set_key1("cam.dva.D%d.ipz.act", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		return 0;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.item_cnt);
	if(!nf_sysdb_set_key1("cam.dva.D%d.ipz.item_cnt", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		return 0;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.item_list);
	if(!nf_sysdb_set_key1("cam.dva.D%d.ipz.item_list", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		return 0;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.dwell);
	if(!nf_sysdb_set_key1("cam.dva.D%d.ipz.dwell", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		return 0;
	}
	g_value_unset(&set_value);

	return 0;
}

static guint _set_dva_prop_lpr_data(DVALprData data, guint channel)
{
	GValue set_value = {0,};

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.active);
	if(!nf_sysdb_set_key1("cam.dva.D%d.lpr.act", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		return 0;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.white_item_cnt);
	if(!nf_sysdb_set_key1("cam.dva.D%d.lpr.white.item_cnt", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		return 0;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.white_item_list);
	if(!nf_sysdb_set_key1("cam.dva.D%d.lpr.white.item_list", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		return 0;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.black_item_cnt);
	if(!nf_sysdb_set_key1("cam.dva.D%d.lpr.black.item_cnt", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		return 0;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.black_item_list);
	if(!nf_sysdb_set_key1("cam.dva.D%d.lpr.black.item_list", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		return 0;
	}
	g_value_unset(&set_value);

	return 0;
}

guint DAL_get_dva_prop_data(DVAPropData *data, guint channel)
{
	GValue ret_value = {0,};

	if (nf_sysdb_get_key1("cam.dva.D%d.act", channel, &ret_value, NULL)) {
		data->active = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else {
		return 0;
	}

	if (nf_sysdb_get_key1("cam.dva.D%d.ignore_interval", channel, &ret_value, NULL)) {
		data->ignore_interval = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else {
		return 0;
	}

	if (nf_sysdb_get_key1("cam.dva.D%d.roi", channel, &ret_value, NULL))
	{
		if (g_value_get_string(&ret_value))
		{
			sscanf(g_value_get_string(&ret_value), "%f %f %f %f", &data->roi_sx, &data->roi_sy, &data->roi_ex, &data->roi_ey);
		}
		else
		{
			data->roi_sx = 0;
			data->roi_sy = 0;
			data->roi_ex = 1;
			data->roi_ey = 1;
		}
		g_value_unset(&ret_value);
	}
	else {
		return 0;
	}

	_get_dva_prop_idz_data(&data->idz, channel);
	_get_dva_prop_ipz_data(&data->ipz, channel);
	_get_dva_prop_lpr_data(&data->lpr, channel);

	return 0;
}

guint DAL_set_dva_prop_data(DVAPropData data, guint channel)
{
	GValue set_value = {0,};
	gchar buf[128];

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.active);
	if(!nf_sysdb_set_key1("cam.dva.D%d.act", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		return 0;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.ignore_interval);
	if(!nf_sysdb_set_key1("cam.dva.D%d.ignore_interval", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		return 0;
	}
	g_value_unset(&set_value);

	memset(buf, 0x00, sizeof(buf));
	g_sprintf(buf, "%.3f %.3f %.3f %.3f", data.roi_sx, data.roi_sy, data.roi_ex, data.roi_ey);
	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);
	if(!nf_sysdb_set_key1("cam.dva.D%d.roi", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		return 0;
	}
	g_value_unset(&set_value);

	_set_dva_prop_idz_data(data.idz, channel);
	_set_dva_prop_ipz_data(data.ipz, channel);
	_set_dva_prop_lpr_data(data.lpr, channel);
}

guint DAL_set_dva_prop_data_all(DVAPropData *data, guint ch_num)
{
    guint ret = 0;
    guint i;

    nf_sysdb_lock(NF_SYSDB_CATE_CAM);

    for( i = 0; i< ch_num; i++)
    {
        ret = DAL_set_dva_prop_data(data[i],i);
        if(ret) break;
    }

    nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
    nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

    DDEBUG_END

    return ret;
}

guint DAL_get_dva_schd_data(DVASchedData *data, guint ch)
{
	GValue ret_value = {0,};
	const gchar *str = NULL;
	gchar buf[169];

	if(nf_sysdb_get_key1("cam.dva.D%d.sched", ch, &ret_value, NULL))
	{
		memset(buf, 0, sizeof(buf));
		str = g_value_get_string(&ret_value);
		g_stpcpy(buf, str);

		memcpy(data->sched[0], buf, 7 * 24);
		g_value_unset(&ret_value);
		return 0;
	}

	return 0;
}

guint DAL_set_dva_schd_data(DVASchedData data, guint ch)
{
	GValue set_value = {0};
	gchar buf[169];

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data.sched[0], 7 * 24);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);
	if(!nf_sysdb_set_key1("cam.dva.D%d.sched", ch, &set_value, NULL)) {
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
		return 0;
	}
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
	
	return 0;
}

guint DAL_set_dva_schd_data_all(DVASchedData *data, guint ch_num)
{
	GValue set_value = {0};
	gchar buf[169];
	gint i;

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for (i = 0; i < ch_num; i++) 
	{
		memset(buf, 0, sizeof(buf));
		memcpy(buf, data[i].sched[0], 7 * 24);

		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, buf);
		if(!nf_sysdb_set_key1("cam.dva.D%d.sched", i, &set_value, NULL)) {
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
			return 0;
		}
		g_value_unset(&set_value);
	}

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);

	return 0;
}

guint DAL_get_aianalysis_act_data(AiAnalysisActData *data, guint channel)
{
	GValue ret_value = {0,};

	if (nf_sysdb_get_key1("cam.vca.cfg.R%u.act", channel, &ret_value, NULL)) {
		data->classic_active = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}

	if (nf_sysdb_get_key1("cam.dva.D%d.act", channel, &ret_value, NULL)) {
		data->builtin_active = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}

	if (nf_sysdb_get_key1("cam.dvabx.cfg.R%u.devcam", channel, &ret_value, NULL)) {
		data->dvacam_active = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}

	if (nf_sysdb_get_key1("cam.dvabx.cfg.R%u.devbox", channel, &ret_value, NULL)) {
		data->dvabox_active = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}

	if (nf_sysdb_get_key1("cam.ai_box.A%d.mac", channel, &ret_value, NULL))
	{
		g_stpcpy(data->dvabox_mac, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}

	if (nf_sysdb_get_key1("cam.ai_box.A%d.addr", channel, &ret_value, NULL))
	{
		data->dvabox_ipaddr = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}

	if (nf_sysdb_get_key1("cam.ai_box.A%d.id", channel, &ret_value, NULL))
	{
		g_stpcpy(data->dvabox_id, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}

	if (nf_sysdb_get_key1("cam.ai_box.A%d.pass", channel, &ret_value, NULL))
	{
		g_stpcpy(data->dvabox_pass, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}	

	return 0;
}

guint DAL_set_aianalysis_act_data(AiAnalysisActData data, guint channel)
{
	GValue set_value = {0,};
	gint dvabx_active = 0;

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.classic_active);
	nf_sysdb_set_key1("cam.vca.cfg.R%u.act", channel, &set_value, NULL);
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.builtin_active);
	nf_sysdb_set_key1("cam.dva.D%d.act", channel, &set_value, NULL);
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.dvacam_active);
	nf_sysdb_set_key1("cam.dvabx.cfg.R%u.devcam", channel, &set_value, NULL);
	g_value_unset(&set_value);	

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.dvabox_active);
	nf_sysdb_set_key1("cam.dvabx.cfg.R%u.devbox", channel, &set_value, NULL);
	g_value_unset(&set_value);

	if (data.dvacam_active || data.dvabox_active) dvabx_active = 1;
	else dvabx_active = 0;

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, dvabx_active);
	nf_sysdb_set_key1("cam.dvabx.cfg.R%u.act", channel, &set_value, NULL);
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.dvabox_mac);
	nf_sysdb_set_key1("cam.ai_box.A%d.mac", channel, &set_value, NULL);
	g_value_unset(&set_value);

    g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.dvabox_ipaddr);
	nf_sysdb_set_key1("cam.ai_box.A%d.addr", channel, &set_value, NULL);
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.dvabox_id);
	nf_sysdb_set_key1("cam.ai_box.A%d.id", channel, &set_value, NULL);
	g_value_unset(&set_value);

    g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.dvabox_pass);
	nf_sysdb_set_key1("cam.ai_box.A%d.pass", channel, &set_value, NULL);
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	return 0;
}

guint DAL_set_aianalysis_act_data_all(AiAnalysisActData *data, guint ch_num)
{
	GValue set_value = {0,};
	gint i, dvabx_active = 0;
	
	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for (i = 0; i < ch_num; i++)
	{
		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[i].classic_active);
		nf_sysdb_set_key1("cam.vca.cfg.R%u.act", i, &set_value, NULL);
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[i].builtin_active);
		nf_sysdb_set_key1("cam.dva.D%d.act", i, &set_value, NULL);
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[i].dvacam_active);
		nf_sysdb_set_key1("cam.dvabx.cfg.R%u.devcam", i, &set_value, NULL);
		g_value_unset(&set_value);	

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[i].dvabox_active);
		nf_sysdb_set_key1("cam.dvabx.cfg.R%u.devbox", i, &set_value, NULL);
		g_value_unset(&set_value);

		if (data[i].dvacam_active || data[i].dvabox_active) dvabx_active = 1;
		else dvabx_active = 0;

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, dvabx_active);
		nf_sysdb_set_key1("cam.dvabx.cfg.R%u.act", i, &set_value, NULL);
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, data[i].dvabox_mac);
		nf_sysdb_set_key1("cam.ai_box.A%d.mac", i, &set_value, NULL);
		g_value_unset(&set_value);		

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data[i].dvabox_ipaddr);
		nf_sysdb_set_key1("cam.ai_box.A%d.addr", i, &set_value, NULL);
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, data[i].dvabox_id);
		nf_sysdb_set_key1("cam.ai_box.A%d.id", i, &set_value, NULL);
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, data[i].dvabox_pass);
		nf_sysdb_set_key1("cam.ai_box.A%d.pass", i, &set_value, NULL);
		g_value_unset(&set_value);		
	}

	// nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
	
	DDEBUG_END

	return 0;
}

guint DAL_get_aianalysis_schd_data_all(AiAnalysisSchedData *data, guint ch_num)
{
	gchar key[64], *str;
	guint ch;

	memset(data, 0, sizeof(AiAnalysisSchedData) * ch_num);

	for (ch = 0; ch < ch_num; ch++) {
		sprintf(key, "cam.vca.cfg.R%u.sched", ch);
		str = nf_sysdb_get_str_nocopy(key);
		if (str) memcpy(data[ch].sched, str, 24*7);
	}

	return 0;
}

static guint prvSetAiAnalysisSchedData(AiAnalysisSchedData data, guint ch)
{
	GValue set_value = {0};

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.sched);
	nf_sysdb_set_key1("cam.vca.cfg.R%u.sched", ch, &set_value, NULL);
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.sched);
	nf_sysdb_set_key1("cam.dva.D%d.sched", ch, &set_value, NULL);
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.sched);
	nf_sysdb_set_key1("cam.dvabx.cfg.R%u.sched", ch, &set_value, NULL);
	g_value_unset(&set_value);

	return FALSE;
}

guint DAL_set_aianalysis_schd_data(AiAnalysisSchedData data, guint channel)
{
	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	prvSetAiAnalysisSchedData(data, channel);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
	return 0;
}

guint DAL_set_aianalysis_schd_data_all(AiAnalysisSchedData *data, guint ch_num)
{
	guint ch;

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for (ch = 0; ch < ch_num; ch++)
		prvSetAiAnalysisSchedData(data[ch], ch);

	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
	return 0;
}

#ifdef __SUPPORT_BS8418__
/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
gboolean DAL_get_low_contr_data(LowContrData *data)
{
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("cam.lcont.white_detect", &ret_value, NULL))
	{
		data->white_detect = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		g_warning("%d %s \n", __LINE__, "[cam.lcont.white_detect] nf_sysdb_get_key0 returns FALSE");

		return FALSE;
	}

	if(nf_sysdb_get_key0("cam.lcont.black_detect", &ret_value, NULL))
	{
		data->black_detect = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		g_warning("%d %s \n", __LINE__, "[cam.lcont.black_detect] nf_sysdb_get_key0 returns FALSE");

		return FALSE;
	}

	if(nf_sysdb_get_key0("cam.lcont.white_level", &ret_value, NULL))
	{
		data->white_level = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		g_warning("%d %s \n", __LINE__, "[cam.lcont.white_level] nf_sysdb_get_key0 returns FALSE");

		return FALSE;
	}

	if(nf_sysdb_get_key0("cam.lcont.black_level", &ret_value, NULL))
	{
		data->black_level = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		g_warning("%d %s \n", __LINE__, "[cam.lcont.black_level] nf_sysdb_get_key0 returns FALSE");

		return FALSE;
	}

	return TRUE;
}



gboolean DAL_set_low_contr_data(LowContrData *data)
{
	GValue set_value = {0,};

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->white_detect);
	if(!nf_sysdb_set_key0("cam.lcont.white_detect", &set_value, NULL))
	{
		g_value_unset(&set_value);

		g_warning("%d %s \n", __LINE__, "[cam.lcont.white_detect] nf_sysdb_get_key0 returns FALSE");

		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->black_detect);
	if(!nf_sysdb_set_key0("cam.lcont.black_detect", &set_value, NULL))
	{
		g_value_unset(&set_value);

		g_warning("%d %s \n", __LINE__, "[cam.lcont.black_detect] nf_sysdb_get_key0 returns FALSE");

		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->white_level);
	if(!nf_sysdb_set_key0("cam.lcont.white_level", &set_value, NULL))
	{
		g_value_unset(&set_value);

		g_warning("%d %s \n", __LINE__, "[cam.lcont.white_level] nf_sysdb_get_key0 returns FALSE");

		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->black_level);
	if(!nf_sysdb_set_key0("cam.lcont.black_level", &set_value, NULL))
	{
		g_value_unset(&set_value);

		g_warning("%d %s \n", __LINE__, "[cam.lcont.black_level] nf_sysdb_get_key0 returns FALSE");

		return FALSE;
	}
	g_value_unset(&set_value);

	return TRUE;
}
#endif

gboolean DAL_get_cam_install_mode()
{
	gboolean ret = FALSE;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("cam.install.mode", &ret_value, NULL))
	{
		ret = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}

	return ret;
}

guint DAL_set_cam_install_mode(gboolean mode)
{
	guint ret = 0;
	GValue set_value = {0,};

	DDEBUG_START
	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, mode);

	nf_sysdb_set_key0("cam.install.mode", &set_value, NULL);

	if(!nf_sysdb_set_key0("cam.install.mode", &set_value, NULL))
	{
		g_value_unset(&set_value);

		DDEBUG_FAIL
		return 1;
	}

	g_value_unset(&set_value);
	DDEBUG_END

	return ret;

}

gboolean DAL_get_cam_install_use_dual_lan()
{
	gboolean ret = FALSE;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("cam.install.dual_lan", &ret_value, NULL))
	{
		ret = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}

	return ret;
}

guint DAL_set_cam_install_use_dual_lan(gboolean onoff)
{
	guint ret = 0;
	GValue set_value = {0,};

	DDEBUG_START
	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, onoff);

	nf_sysdb_set_key0("cam.install.dual_lan", &set_value, NULL);

	if(!nf_sysdb_set_key0("cam.install.dual_lan", &set_value, NULL))
	{
		g_value_unset(&set_value);

		DDEBUG_FAIL
		return 1;
	}

	g_value_unset(&set_value);
	DDEBUG_END

	return ret;

}

guint DAL_get_focus_data(FocusData *data, guint channel)
{
    int i;
    guint ret =0;
    GValue ret_value = {0,};

	/*if (nf_sysdb_get_key1("cam.C%d.focus.cell_cnt", channel, &ret_value, NULL))
	{
		data->cell_cnt= g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 1;
	}*/

	if (nf_sysdb_get_key1("cam.C%d.focus_dnn_comp", channel, &ret_value, NULL))
	{
		data->day_night = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 2;
	}

	if (nf_sysdb_get_key1("cam.C%d.focus_tem_comp", channel, &ret_value, NULL))
	{
		data->temperature = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 3;
	}

	return 0;
}


guint DAL_set_focus_data(FocusData data, guint channel)
{
    GValue set_value = {0,};
    gint i;

    DDEBUG_START

    /*
    g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.cell_cnt);
	if(!nf_sysdb_set_key1("cam.C%d.focus.cell_cnt", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);
	*/


    g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.day_night);
	if(!nf_sysdb_set_key1("cam.C%d.focus_dnn_comp", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.temperature);
	if(!nf_sysdb_set_key1("cam.C%d.focus_tem_comp", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);


	DDEBUG_END

	return 0;
}

guint DAL_set_focus_data_all(FocusData *data, guint ch_num)
{
    guint ret = 0;
    guint i;

    DDEBUG_START

    nf_sysdb_lock(NF_SYSDB_CATE_CAM);

    for( i = 0; i< ch_num; i++)
    {
        ret = DAL_set_focus_data(data[i],i);

        if(ret) break;
    }

    nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
    nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

    DDEBUG_END

    return ret;
}

guint DAL_get_camera_itx_fisheye_data(CamItxFisheyeData *data, guint channel)
{
    guint ret =0;
    GValue ret_value = {0,};

	if (nf_sysdb_get_key1("cam.fisheye_itx.F%d.act", channel, &ret_value, NULL))
	{
		data->act = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 0;
	}

	if (nf_sysdb_get_key1("cam.fisheye_itx.F%d.mount_mode", channel, &ret_value, NULL))
	{
		data->mount_mode = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 0;
	}

	if (nf_sysdb_get_key1("cam.fisheye_itx.F%d.lens_mode", channel, &ret_value, NULL))
	{
		data->lens_mode = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 0;
	}

	if (nf_sysdb_get_key1("cam.fisheye_itx.F%d.lens_type", channel, &ret_value, NULL))
	{
		g_stpcpy(data->lens_type, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 0;
	}	

	if (nf_sysdb_get_key1("cam.fisheye_itx.F%d.default_view", channel, &ret_value, NULL))
	{
		data->default_view = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 0;
	}		

	return 0;
}

guint DAL_set_camera_itx_fisheye_data(CamItxFisheyeData data, guint channel)
{
    GValue set_value = {0,};
    gint i;

    DDEBUG_START

    g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.act);
	if(!nf_sysdb_set_key1("cam.fisheye_itx.F%d.act", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 0;
	}
	g_value_unset(&set_value);

    g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.mount_mode);
	if(!nf_sysdb_set_key1("cam.fisheye_itx.F%d.mount_mode", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 0;
	}
	g_value_unset(&set_value);	

    g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.lens_mode);
	if(!nf_sysdb_set_key1("cam.fisheye_itx.F%d.lens_mode", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 0;
	}
	g_value_unset(&set_value);

    g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.lens_type);
	if(!nf_sysdb_set_key1("cam.fisheye_itx.F%d.lens_type", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 0;
	}
	g_value_unset(&set_value);

    g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.default_view);
	if(!nf_sysdb_set_key1("cam.fisheye_itx.F%d.default_view", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 0;
	}
	g_value_unset(&set_value);

	DDEBUG_END

	return 0;
}

guint DAL_set_camera_itx_fisheye_data_all(CamItxFisheyeData *data, guint ch_num)
{
    guint ret = 0;
    guint i;

    DDEBUG_START

    nf_sysdb_lock(NF_SYSDB_CATE_CAM);

    for( i = 0; i< ch_num; i++)
    {
        ret = DAL_set_camera_itx_fisheye_data(data[i], i);
        if(ret) break;
    }

    nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
    nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

    DDEBUG_END

    return ret;
}

guint DAL_get_itx_tmp_fisheye_data(ItxTmpFisheyeData *data, guint channel)
{
    GValue ret_value = {0,};
	gint i;

	if (nf_sysdb_get_key1("cam.fisheye_itx.F%d.tmp_dewarp", channel, &ret_value, NULL))
	{
		data->dewarp = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 0;
	}

	if (nf_sysdb_get_key1("cam.fisheye_itx.F%d.tmp_view", channel, &ret_value, NULL))
	{
		data->viewtype = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 0;
	}

	for (i = 0; i < MAX_FISHEYE_VTYPE; i++)
	{
		if(nf_sysdb_get_key2("cam.fisheye_itx.F%d.view.V%d.tmp_ptzr", channel, i, &ret_value, NULL))
		{
			if (g_value_get_string(&ret_value))
			{
				sscanf(g_value_get_string(&ret_value), "%f,%f,%f,%f", &data->pan[i], &data->tilt[i], &data->zoom[i], &data->roll[i]);
			}
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return 0;
		}		
	}

	return 0;
}

guint DAL_set_itx_tmp_fisheye_data(ItxTmpFisheyeData data, guint channel)
{
    GValue set_value = {0,};
    gchar strBuf[128];
	gint i;

    DDEBUG_START

    g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.dewarp);
	if(!nf_sysdb_set_key1("cam.fisheye_itx.F%d.tmp_dewarp", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 0;
	}
	g_value_unset(&set_value);	

    g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.viewtype);
	if(!nf_sysdb_set_key1("cam.fisheye_itx.F%d.tmp_view", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 0;
	}
	g_value_unset(&set_value);	

	for (i = 0; i < MAX_FISHEYE_VTYPE; i++)
	{
		memset(strBuf, 0x00, sizeof(strBuf));
		sprintf(strBuf, "%f,%f,%f,%f", data.pan[i], data.tilt[i], data.zoom[i], data.roll[i]);
		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, strBuf);
		if(!nf_sysdb_set_key2("cam.fisheye_itx.F%d.view.V%d.tmp_ptzr", channel, i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 0;
		}
		g_value_unset(&set_value);
	}

	DDEBUG_END

	return 0;
}

guint DAL_get_stream_data(StreamData *data, guint channel)
{
	int i;
	guint ret = 0;
	GValue ret_value = {0,};

	for( i = 0; i < 2; i++)
	{
		if(nf_sysdb_get_key2("cam.C%d.stream.S%d.size", channel, i, &ret_value, NULL))
		{
			g_stpcpy(data->size[i], g_value_get_string(&ret_value));
			g_value_unset(&ret_value);
		}
		else		ret = 1;

		if(nf_sysdb_get_key2("cam.C%d.stream.S%d.fps", channel, i, &ret_value, NULL))
		{
    		data->max_fps[i] = g_value_get_uint(&ret_value);
    		g_value_unset(&ret_value);
		}
		else	 	ret = 2;

		if(nf_sysdb_get_key2("cam.C%d.stream.S%d.max_bps", channel, i, &ret_value, NULL))
		{
    		data->max_bps[i] = g_value_get_uint(&ret_value);
    		g_value_unset(&ret_value);
		}
		else	 	ret = 3;

		if(nf_sysdb_get_key2("cam.C%d.stream.S%d.min_bps", channel, i, &ret_value, NULL))
		{
    		data->min_bps[i] = g_value_get_uint(&ret_value);
    		g_value_unset(&ret_value);
		}
		else	 	ret = 4;

		if(nf_sysdb_get_key2("cam.C%d.stream.S%d.bitctrl", channel, i, &ret_value, NULL))
		{
            g_stpcpy(data->control[i], g_value_get_string(&ret_value));
            g_value_unset(&ret_value);
		}
		else        ret = 5;

		if(nf_sysdb_get_key2("cam.C%d.stream.S%d.vcodec", channel, i, &ret_value, NULL))
		{
            g_stpcpy(data->vcodec[i], g_value_get_string(&ret_value));
            g_value_unset(&ret_value);
		}
		else        ret = 6;
	}
	
	if(nf_sysdb_get_key1("cam.C%d.corridor_view", channel, &ret_value, NULL))
	{
		data->corridor_mode = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else        ret = 7;
}

static guint prvSetStreamData(StreamData data, guint channel)
{
	GValue set_value = {0,};
	gint i;

    for (i = 0; i < 2; i++)
    {
    	g_value_init(&set_value, G_TYPE_STRING);
    	g_value_set_string(&set_value, data.size[i]);
		if(!nf_sysdb_set_key2("cam.C%d.stream.S%d.size", channel, i, &set_value, NULL))
    	{
    		g_value_unset(&set_value);
    		DDEBUG_FAIL
    		return 1;
    	}
    	g_value_unset(&set_value);

    	g_value_init(&set_value, G_TYPE_UINT);
    	g_value_set_uint(&set_value, data.max_fps[i]);
    	if(!nf_sysdb_set_key2("cam.C%d.stream.S%d.fps", channel, i, &set_value, NULL))
    	{
    		g_value_unset(&set_value);
    		DDEBUG_FAIL
    		return 2;
    	}
    	g_value_unset(&set_value);

    	g_value_init(&set_value, G_TYPE_UINT);
    	g_value_set_uint(&set_value, data.max_bps[i]);
    	if(!nf_sysdb_set_key2("cam.C%d.stream.S%d.max_bps", channel, i, &set_value, NULL))
    	{
    		g_value_unset(&set_value);
    		DDEBUG_FAIL
    		return 3;
    	}
    	g_value_unset(&set_value);

    	g_value_init(&set_value, G_TYPE_UINT);
    	g_value_set_uint(&set_value, data.min_bps[i]);
    	if(!nf_sysdb_set_key2("cam.C%d.stream.S%d.min_bps", channel, i, &set_value, NULL))
    	{
    		g_value_unset(&set_value);
    		DDEBUG_FAIL
    		return 4;
    	}
    	g_value_unset(&set_value);


    	g_value_init(&set_value, G_TYPE_STRING);
    	g_value_set_string(&set_value, data.control[i]);
    	if(!nf_sysdb_set_key2("cam.C%d.stream.S%d.bitctrl", channel, i, &set_value, NULL))
    	{
    	    g_value_unset(&set_value);
    	    DDEBUG_FAIL
    	    return 5;
    	}
    	g_value_unset(&set_value);

        g_value_init(&set_value, G_TYPE_STRING);
    	g_value_set_string(&set_value, data.vcodec[i]);
		if(!nf_sysdb_set_key2("cam.C%d.stream.S%d.vcodec", channel, i, &set_value, NULL))
    	{
    		g_value_unset(&set_value);
    		DDEBUG_FAIL
    		return 6;
    	}
    	g_value_unset(&set_value);

    }
	
	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.corridor_mode);
	if(!nf_sysdb_set_key1("cam.C%d.corridor_view", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 7;
	}
	g_value_unset(&set_value);	

	return 0;
}

guint DAL_set_stream_data(StreamData data, guint channel)
{
	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	ret = prvSetStreamData(data, channel);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END

	return ret;
}

guint DAL_set_stream_data_all(StreamData *data, guint ch_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for (i = 0; i < ch_num; i++)
	{
		ret = prvSetStreamData(data[i], i);

		if (ret) break;
	}

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

	DDEBUG_END

	return ret;
}

guint DAL_get_fisheye_data(FishEyeData *data, gint ch)
{
    GValue ret_value = {0,};
    guint ret = 0;

    if (nf_sysdb_get_key1("cam.fisheye.f%d.dewarp_mode", ch, &ret_value, NULL))
    {
        data->dewarp_mode = g_value_get_uint(&ret_value);
        g_value_unset(&ret_value);
    }
    else ret = 1;

    if (nf_sysdb_get_key1("cam.fisheye.f%d.mount_type", ch, &ret_value, NULL))
    {
        data->mount_type = g_value_get_uint(&ret_value);
        g_value_unset(&ret_value);
    }
    else ret = 2;

    return ret;
}

guint prvSetFishEyeData(FishEyeData *data, gint ch)
{
    GValue set_value = {0,};

    g_value_init(&set_value, G_TYPE_UINT);
    g_value_set_uint(&set_value, data[ch].dewarp_mode);
    if (!nf_sysdb_set_key1("cam.fisheye.f%d.dewarp_mode", ch, &set_value, NULL))
    {
        g_value_unset(&set_value);
        nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
        DDEBUG_FAIL
        return 1;
    }
    g_value_unset(&set_value);

    g_value_init(&set_value, G_TYPE_UINT);
    g_value_set_uint(&set_value, data[ch].mount_type);
    if (!nf_sysdb_set_key1("cam.fisheye.f%d.mount_type", ch, &set_value, NULL))
    {
        g_value_unset(&set_value);
        nf_sysdb_unlock(NF_SYSDB_CATE_CAM);
        DDEBUG_FAIL
        return 2;
    }
    g_value_unset(&set_value);

    return 0;
}

guint DAL_set_fisheye_data(FishEyeData *data, gint ch_num)
{
    GValue set_value = {0,};
    guint ret;
    gint i;

    DDEBUG_START

    nf_sysdb_lock(NF_SYSDB_CATE_CAM);

    for (i = 0; i < ch_num; i++)
    {
        ret = prvSetFishEyeData(data, i);
    }

    nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);

    nf_sysdb_unlock(NF_SYSDB_CATE_CAM);

    DDEBUG_END

    return 0;
}

/*************************************************************************
 * Setup --> System --> Display
 * ***********************************************************************/

guint DAL_set_language(gchar *lang)
{
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_DISP);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, lang);
	if(!nf_sysdb_set_key0("disp.osd.lang", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_DISP, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_DISP);

	DDEBUG_END
	return 0;
}

void DAL_get_language(gchar *lang)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("disp.osd.lang", &ret_value, NULL))
	{
		g_stpcpy(lang, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
}

gint DAL_get_support_lang_cnt()
{
	gint ret = -1;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("disp.osd.cnt_lang", &ret_value, NULL))
	{
		ret = g_value_get_int(&ret_value);
		g_value_unset(&ret_value);
	}
	return ret;
}

gboolean DAL_get_support_lang(gint index, gchar *buf)
{
	GValue ret_value = {0,};

	if(nf_sysdb_get_key1("disp.osd.L%d", index, &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
		return TRUE;
	}
	return FALSE;
}

gboolean DAL_get_support_lang_alias(gint index, gchar *buf)
{
	GValue ret_value = {0,};

	if(nf_sysdb_get_key1("disp.osd.A%d", index, &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
		return TRUE;
	}
	return FALSE;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_osd_data(OsdData *data)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("disp.osd.status_bar", &ret_value, NULL))
	{
		data->statusBar = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 1;

	if(nf_sysdb_get_key0("disp.osd.cam_title", &ret_value, NULL))
	{
		if(g_value_get_boolean(&ret_value))			data->camTitle = TRUE;
		else 	data->camTitle = FALSE;
		g_value_unset(&ret_value);
	}
	else	 	ret = 2;

	if(nf_sysdb_get_key0("disp.osd.event_icon", &ret_value, NULL))
	{
		if(g_value_get_boolean(&ret_value))	data->evtIcon = TRUE;
		else	data->evtIcon = FALSE;
		g_value_unset(&ret_value);
	}
	else	 	ret = 3;

	if(nf_sysdb_get_key0("disp.osd.border", &ret_value, NULL))
	{
		data->border = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 4;

	if(nf_sysdb_get_key0("disp.osd.border_color", &ret_value, NULL))
	{
		data->borderColor = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 5;

	if(nf_sysdb_get_key0("disp.osd.menu_alpha", &ret_value, NULL))
	{
		data->menuTrans = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 6;

	if(nf_sysdb_get_key0("disp.osd.motion", &ret_value, NULL))
	{
		data->motSenDisp = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 7;

	if(nf_sysdb_get_key0("disp.osd.motion_color", &ret_value, NULL))
	{
		data->motionColor = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 8;

	if(nf_sysdb_get_key0("disp.osd.motion_alpha", &ret_value, NULL))
	{
		data->motionTrans = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 9;

	if(nf_sysdb_get_key0("disp.osd.lang", &ret_value, NULL))
	{
		g_stpcpy(data->lang, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else	 	ret = 10;

	if(nf_sysdb_get_key0("disp.osd.audio", &ret_value, NULL))
	{
		if(g_value_get_boolean(&ret_value)) data->audio = TRUE;
		else data->audio = FALSE;

		g_value_unset(&ret_value);
	}
	else	 	ret = 11;

	if(nf_sysdb_get_key0("disp.osd.timeline", &ret_value, NULL))
	{
		data->timeline = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 12;

	if(nf_sysdb_get_key0("disp.osd.user_name", &ret_value, NULL))
	{
		if(g_value_get_boolean(&ret_value)) data->user_name = TRUE;
		else data->user_name = FALSE;

		g_value_unset(&ret_value);
	}
	else	 	ret = 13;

	if(nf_sysdb_get_key0("disp.osd.side_icon", &ret_value, NULL))
	{
		if(g_value_get_boolean(&ret_value)) data->sideicon = TRUE;
		else data->sideicon = FALSE;

		g_value_unset(&ret_value);
	}
	else	 	ret = 14;

	if(nf_sysdb_get_key0("disp.osd.status_icon", &ret_value, NULL))
	{
		if(g_value_get_boolean(&ret_value)) data->statusicon = TRUE;
		else data->statusicon = FALSE;

		g_value_unset(&ret_value);
	}
	else	 	ret = 15;

	if(nf_sysdb_get_key0("disp.osd.zoom_pip", &ret_value, NULL))
    {
        data->zoompip = g_value_get_uint(&ret_value);
        g_value_unset(&ret_value);
    }
    else	ret = 16;
	
	if(nf_sysdb_get_key0("disp.osd.time", &ret_value, NULL))
	{
		if(g_value_get_boolean(&ret_value)) data->time = TRUE;
		else data->time = FALSE;
		
		g_value_unset(&ret_value);
	}
	else	 	ret = 17;


	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_set_osd_data(OsdData *data)
{
	guint ret = 0;
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_DISP);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->statusBar);
	if(!nf_sysdb_set_key0("disp.osd.status_bar", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	if(data->camTitle)	g_value_set_boolean(&set_value, TRUE);
	else	g_value_set_boolean(&set_value, FALSE);
	if(!nf_sysdb_set_key0("disp.osd.cam_title", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	if(data->evtIcon)	g_value_set_boolean(&set_value, TRUE);
	else		g_value_set_boolean(&set_value, FALSE);
	if(!nf_sysdb_set_key0("disp.osd.event_icon", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->border);
	if(!nf_sysdb_set_key0("disp.osd.border", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->borderColor);
	if(!nf_sysdb_set_key0("disp.osd.border_color", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->menuTrans);
	if(!nf_sysdb_set_key0("disp.osd.menu_alpha", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->motSenDisp);
	if(!nf_sysdb_set_key0("disp.osd.motion", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 7;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->motionColor);
	if(!nf_sysdb_set_key0("disp.osd.motion_color", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 8;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->motionTrans);
	if(!nf_sysdb_set_key0("disp.osd.motion_alpha", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 9;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->lang);
	if(!nf_sysdb_set_key0("disp.osd.lang", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 10;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	if(data->audio)	g_value_set_boolean(&set_value, TRUE);
	else	g_value_set_boolean(&set_value, FALSE);
	if(!nf_sysdb_set_key0("disp.osd.audio", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 11;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->timeline);
	if(!nf_sysdb_set_key0("disp.osd.timeline", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 12;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	if(data->user_name)	g_value_set_boolean(&set_value, TRUE);
	else	g_value_set_boolean(&set_value, FALSE);
	if(!nf_sysdb_set_key0("disp.osd.user_name", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 13;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	if(data->sideicon)	g_value_set_boolean(&set_value, TRUE);
	else	g_value_set_boolean(&set_value, FALSE);
	if(!nf_sysdb_set_key0("disp.osd.side_icon", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 14;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	if(data->statusicon)	g_value_set_boolean(&set_value, TRUE);
	else	g_value_set_boolean(&set_value, FALSE);
	if(!nf_sysdb_set_key0("disp.osd.status_icon", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 15;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
    g_value_set_uint(&set_value, data->zoompip);
    if(!nf_sysdb_set_key0("disp.osd.zoom_pip", &set_value, NULL))
    {
        nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
        DDEBUG_FAIL
        return 16;
    }
    g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	if(data->time)	g_value_set_boolean(&set_value, TRUE);
	else	g_value_set_boolean(&set_value, FALSE);
	if(!nf_sysdb_set_key0("disp.osd.time", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 17;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_DISP, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_DISP);

	DDEBUG_END
	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
gboolean DAL_get_border_color(guint *color_index)
{
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("disp.osd.border_color", &ret_value, NULL))
	{
		*color_index = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}else
		return FALSE;

	return TRUE;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_live_status_on_time()
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("disp.osd.status_bar", &ret_value, NULL))
	{
		ret = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}

	return ret;
}


/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_live_timeline_on_mode()
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("disp.osd.timeline", &ret_value, NULL))
	{
		ret = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}

	return ret;
}

guint DAL_set_live_timeline_on_mode(guint mode)
{
    guint ret = 0;
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_DISP);

    g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, mode);
	if(!nf_sysdb_set_key0("disp.osd.timeline", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return -1;
	}
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_DISP);

	DDEBUG_END

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
gboolean DAL_get_username_on_off()
{
	gboolean ret = FALSE;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("disp.osd.user_name", &ret_value, NULL))
	{
		ret = g_value_get_boolean(&ret_value);

		g_value_unset(&ret_value);
	}

	return ret;
}


/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_monitor_data(MonitorData *data)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("disp.monitor.seq_dwell", &ret_value, NULL))
	{
		data->seqDwell = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 1;

	if(nf_sysdb_get_key0("disp.monitor.spot_dwell", &ret_value, NULL))
	{
		data->spotDwell = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 2;

	if(nf_sysdb_get_key0("disp.monitor.deinterace", &ret_value, NULL))
	{
		data->deinterlace = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 3;

	if(nf_sysdb_get_key0("disp.monitor.alarm_popup", &ret_value, NULL))
	{
		data->alarmPopup = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 4;

	if(nf_sysdb_get_key0("disp.monitor.alarm_dwell", &ret_value, NULL))
	{
		data->alarmDwell = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 5;

	if(nf_sysdb_get_key0("disp.monitor.motion_popup", &ret_value, NULL))
	{
		data->motPopup = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 6;

	if(nf_sysdb_get_key0("disp.monitor.motion_dwell", &ret_value, NULL))
	{
		data->motDwell = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 7;

	if(nf_sysdb_get_key0("disp.monitor.aspect", &ret_value, NULL))
	{
		data->aspect = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 8;

	if(nf_sysdb_get_key0("disp.monitor.overscan", &ret_value, NULL))
	{
		data->overscan= g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 9;

	if(nf_sysdb_get_key0("disp.monitor.dualmonitor", &ret_value, NULL))
	{
		data->dualmonitor = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 10;

	if(nf_sysdb_get_key0("disp.monitor.resolution", &ret_value, NULL))
	{
		data->resolution= g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 11;

	if(nf_sysdb_get_key0("disp.monitor.hd_spot_dwell", &ret_value, NULL))
	{
		data->hd_spotDwell = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 12;
	
	if(nf_sysdb_get_key0("disp.monitor.hdsdi_output", &ret_value, NULL))
	{
		data->hdsdiOutMode = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 13;

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_set_monitor_data(MonitorData *data)
{
	guint ret = 0;
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_DISP);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->seqDwell);
	if(!nf_sysdb_set_key0("disp.monitor.seq_dwell", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->spotDwell);
	if(!nf_sysdb_set_key0("disp.monitor.spot_dwell", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 7;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->deinterlace);
	if(!nf_sysdb_set_key0("disp.monitor.deinterace", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->alarmPopup);
	if(!nf_sysdb_set_key0("disp.monitor.alarm_popup", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->alarmDwell);
	if(!nf_sysdb_set_key0("disp.monitor.alarm_dwell", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->motPopup);
	if(!nf_sysdb_set_key0("disp.monitor.motion_popup", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->motDwell);
	if(!nf_sysdb_set_key0("disp.monitor.motion_dwell", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->aspect);
	if(!nf_sysdb_set_key0("disp.monitor.aspect", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 7;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->overscan);
	if(!nf_sysdb_set_key0("disp.monitor.overscan", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 8;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->dualmonitor);
	if(!nf_sysdb_set_key0("disp.monitor.dualmonitor", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 9;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_DISP, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_DISP);

	DDEBUG_END
	return ret;
}



/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_sequence_dwell()
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("disp.monitor.seq_dwell", &ret_value, NULL))
	{
		ret = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);

		return ret;

	}else {
		DDEBUG_FAIL
		return 0;
	}
}

guint DAL_get_spot_dwell_time()
{
	guint dtime;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("disp.monitor.spot_dwell", &ret_value, NULL))
	{
		dtime = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 0;
	}

	return dtime;
}

guint DAL_get_alarm_popup_onoff()
{
	GValue ret_value = {0,};
	guint ret = 0;

	if(nf_sysdb_get_key0("disp.monitor.alarm_popup", &ret_value, NULL))
	{
		ret = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return 0;
	}

	return ret;
}


/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_sequence_count()
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("disp.main.MCNT", &ret_value, NULL))
	{
		ret = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 0;

	return ret;

}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_set_sequence_count(guint cnt)
{
	guint ret = 0;
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_DISP);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, cnt);
	if(!nf_sysdb_set_key0("disp.main.MCNT", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_DISP, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_DISP);

	DDEBUG_END
	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
static guint prvGetSeqElemData(SeqElementData *data, guint seq_idx, guint nth)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key2("disp.main.M%d.type.T%d.type", seq_idx, nth, &ret_value, NULL))
	{
		data->type = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	if(nf_sysdb_get_key2("disp.main.M%d.type.T%d.ch", seq_idx, nth, &ret_value, NULL))
	{
		gchar tmp[MAX_DISP_DIVISION_NUM+1];
		guint i;

		memset(tmp, 0, sizeof(gchar)*(MAX_DISP_DIVISION_NUM+1));
		g_stpcpy(tmp, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);

//		parse tmp string. and set data->conf[0] ~ data->conf[15]
		for(i=0; i<MAX_DISP_DIVISION_NUM; i++)
		{
			if(!tmp[i])
				break;
			if(tmp[i]>='A' && tmp[i]<='P')		// analog camera
			{
				data->conf[i] = tmp[i]-'A';
			}
			else if(tmp[i]>='a' && tmp[i]<='p')	// ip camera
			{
				data->conf[i] = tmp[i]-'a'+16;
			}
		}
	}
	else		ret = 2;

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
static guint prvSetSeqElemData(SeqElementData data, guint seq_idx, guint nth)
{
	guint ret = 0;
	GValue set_value = {0,};
	gchar tmp[MAX_DISP_DIVISION_NUM+1];
	guint i;

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.type);
	if(!nf_sysdb_set_key2("disp.main.M%d.type.T%d.type", seq_idx, nth, &set_value, NULL))
	{
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	memset(tmp, 0, sizeof(gchar)*(MAX_DISP_DIVISION_NUM+1));
	for(i=0; i<MAX_DISP_DIVISION_NUM; i++)
	{
		if(data.conf[i]>=0 && data.conf[i]<16)
		{
			tmp[i] = data.conf[i] + 'A';
		}
		else if(data.conf[i]>=16 && data.conf[i]<32)
		{
			tmp[i] = data.conf[i] - 16 + 'a';
		}
	}
	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, tmp);
	if(!nf_sysdb_set_key2("disp.main.M%d.type.T%d.ch", seq_idx, nth, &set_value, NULL))
	{
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_seq_data(SeqData *data, guint seq_idx)
{
	guint ret = 0;
	GValue ret_value = {0,};
	guint i = 0;

	if(nf_sysdb_get_key1("disp.main.M%d.name", seq_idx, &ret_value, NULL))
	{
		g_stpcpy(data->title, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else	ret = 1;

	if(nf_sysdb_get_key1("disp.main.M%d.creator", seq_idx, &ret_value, NULL))
	{
		g_stpcpy(data->createdby, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else	ret = 2;

	if(nf_sysdb_get_key1("disp.main.M%d.valid_mode", seq_idx, &ret_value, NULL))
	{
		data->valid_mode = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	ret = 3;

	if(nf_sysdb_get_key1("disp.main.M%d.type.TCNT", seq_idx, &ret_value, NULL))
	{
	  guint numItems = g_value_get_uint(&ret_value);

#if defined(GUI_4CH_SUPPORT) || defined(GUI_8CH_SUPPORT)
    if(seq_idx == 0 && numItems > g_ch)
    {
      numItems = g_ch;
    }
#endif

		data->numItems = numItems;
		g_value_unset(&ret_value);
	}
	else	ret = 4;

	//	SeqElementData	items[MAX_SEQ_ITEMS_NUM];
	for(i=0; i<data->numItems; i++)
	{
		if(prvGetSeqElemData(&(data->items[i]), seq_idx, i))		ret = 5;
	}

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
static guint prvSetSeqData(SeqData data, guint seq_idx)
{
	GValue set_value = {0,};
	gchar seq_name[128];
	guint i;

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.title);
	if(!nf_sysdb_set_key1("disp.main.M%d.name", seq_idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.createdby);
	if(!nf_sysdb_set_key1("disp.main.M%d.creator", seq_idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.valid_mode);
	if(!nf_sysdb_set_key1("disp.main.M%d.valid_mode", seq_idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.numItems);
	if(!nf_sysdb_set_key1("disp.main.M%d.type.TCNT", seq_idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	//	SeqElementData	items[MAX_SEQ_ITEMS_NUM];
	for(i=0; i<data.numItems; i++)
	{
		if(prvSetSeqElemData(data.items[i], seq_idx, i))
		{
			DDEBUG_FAIL
			return 5;
		}
	}

	return 0;
}

guint DAL_set_seq_data(SeqData data, guint seq_idx)
{
	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_DISP);

	ret = prvSetSeqData(data, seq_idx);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_DISP, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_DISP);

	DDEBUG_END
	return ret;
}

guint DAL_set_seq_data_all(SeqData *data, guint seq_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_DISP);

	for(i=0; i<seq_num; i++)
	{
		ret = prvSetSeqData(data[i], i);
		if(ret)	break;
	}

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_DISP, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_DISP);

	DDEBUG_END
	return ret;
}

/******************** WIN ID *******************/

gboolean DAL_get_winid_data(gint *data)
{
    GValue ret_value = {0,};
    gint i;

    for(i=0; i<32; i++)
    {
        if(nf_sysdb_get_key1("disp.info.I%d.winid",i,&ret_value,NULL))
        {
            data[i] = g_value_get_int(&ret_value);

            g_value_unset(&ret_value);
        }
        else
            return FALSE;
    }

    return TRUE;
}

gboolean DAL_set_winid_data(gint *data)
{
    GValue set_value = {0, };
    gint i;

    DDEBUG_START
    nf_sysdb_lock(NF_SYSDB_CATE_DISP);

    for(i=0; i<32; i++)
    {
        g_value_init(&set_value, G_TYPE_INT);
        g_value_set_int(&set_value, data[i]);

        if(!nf_sysdb_set_key1("disp.info.I%d.winid",i,&set_value,NULL))
        {
            g_value_unset(&set_value);

            nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
            DDEBUG_FAIL
                return FALSE;
        }
        g_value_unset(&set_value);

    }

    nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_DISP, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_DISP);

    DDEBUG_END
    return TRUE;
}

/******************** SPOT *********************/
/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_spot_count()
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("disp.spot.SCNT", &ret_value, NULL))
	{
		ret = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 0;

	return ret;

}


/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_set_spot_count(guint cnt)
{
	guint ret = 0;
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_DISP);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, cnt);
	if(!nf_sysdb_set_key0("disp.spot.SCNT", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_DISP, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_DISP);

	DDEBUG_END
	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
static guint prvGetSpotElemData(SpotElementData *data, guint spot_idx, guint nth)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key2("disp.spot.S%d.type.T%d.type", spot_idx, nth, &ret_value, NULL))
	{
		data->type = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	if(nf_sysdb_get_key2("disp.spot.S%d.type.T%d.ch", spot_idx, nth, &ret_value, NULL))
	{
		gchar tmp[MAX_SPOT_DIVISION_NUM+1];
		guint i;
    	guint conf_num = 0;

		memset(tmp, 0, sizeof(tmp));
		g_stpcpy(tmp, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);

//		parse tmp string. and set data->conf[0] ~ data->conf[15]
		for(i=0; i<MAX_SPOT_DIVISION_NUM; i++)
		{
			if(!tmp[i])
				break;
			if(tmp[i]>='A' && tmp[i]<='P')		// analog camera
			{
        conf_num = tmp[i]-'A';
#if defined(GUI_4CH_SUPPORT) || defined(GUI_8CH_SUPPORT)
        if(conf_num >= g_ch)
        {
          conf_num %= g_ch;
        }
#endif
				//data->conf[i] = tmp[i]-'A';
				data->conf[i] = conf_num;
			}
			else if(tmp[i]>='a' && tmp[i]<='p')	// ip camera
			{
				data->conf[i] = tmp[i]-'a'+16;
			}
		}
	}
	else		ret = 2;

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
static guint prvSetSpotElemData(SpotElementData data, guint spot_idx, guint nth)
{
	guint ret = 0;
	GValue set_value = {0,};
	gchar tmp[MAX_SPOT_DIVISION_NUM+1];
	guint i;

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.type);
	if(!nf_sysdb_set_key2("disp.spot.S%d.type.T%d.type", spot_idx, nth, &set_value, NULL))
	{
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	memset(tmp, 0, sizeof(tmp));
	for(i=0; i<MAX_SPOT_DIVISION_NUM; i++)
	{
		if(data.conf[i]>=0 && data.conf[i]<16)
		{
			tmp[i] = data.conf[i] + 'A';
		}
		else if(data.conf[i]>=16 && data.conf[i]<32)
		{
			tmp[i] = data.conf[i] - 16 + 'a';
		}
	}
	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, tmp);
	if(!nf_sysdb_set_key2("disp.spot.S%d.type.T%d.ch", spot_idx, nth, &set_value, NULL))
	{
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_spot_data(SpotData *data, guint spot_idx)
{
	guint ret = 0;
	GValue ret_value = {0,};
	guint i = 0;

	if(nf_sysdb_get_key1("disp.spot.S%d.name", spot_idx, &ret_value, NULL))
	{
		g_stpcpy(data->title, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else	ret = 1;

	if(nf_sysdb_get_key1("disp.spot.S%d.creator", spot_idx, &ret_value, NULL))
	{
		g_stpcpy(data->createdby, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else	ret = 2;

	if(nf_sysdb_get_key1("disp.spot.S%d.valid_mode", spot_idx, &ret_value, NULL))
	{
		data->valid_mode = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	ret = 3;

	if(nf_sysdb_get_key1("disp.spot.S%d.type.TCNT", spot_idx, &ret_value, NULL))
	{
		data->numItems = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	ret = 4;

	for(i=0; i<data->numItems; i++)
	{
		if(prvGetSpotElemData(&(data->items[i]), spot_idx, i))		ret = 5;
	}

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
static guint prvSetSpotData(SpotData data, guint spot_idx)
{
	GValue set_value = {0,};
	gchar seq_name[128];
	guint i;

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.title);
	if(!nf_sysdb_set_key1("disp.spot.S%d.name", spot_idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.createdby);
	if(!nf_sysdb_set_key1("disp.spot.S%d.creator", spot_idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.valid_mode);
	if(!nf_sysdb_set_key1("disp.spot.S%d.valid_mode", spot_idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.numItems);
	if(!nf_sysdb_set_key1("disp.spot.S%d.type.TCNT", spot_idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	for(i=0; i<data.numItems; i++)
	{
		if(prvSetSpotElemData(data.items[i], spot_idx, i))
		{
			DDEBUG_FAIL
			return 5;
		}
	}

	return 0;
}

guint DAL_set_spot_data(SpotData data, guint spot_idx)
{
	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_DISP);

	ret = prvSetSpotData(data, spot_idx);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_DISP, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_DISP);

	DDEBUG_END
	return ret;
}

guint DAL_set_spot_data_all(SpotData *data, guint spot_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_DISP);

	for(i=0; i<spot_num; i++)
	{
		ret = prvSetSpotData(data[i], i);
		if(ret)	break;
	}

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_DISP, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_DISP);

	DDEBUG_END
	return ret;
}

/******************** HD SPOT *********************/
/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_HD_spot_count()
{
	guint ret = 0;
	GValue ret_value = {0,};
	
	if(nf_sysdb_get_key0("disp.hd_spot.SCNT", &ret_value, NULL))
	{
		ret = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 0;

	return ret;

}


/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_set_HD_spot_count(guint cnt)
{
	guint ret = 0;
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_DISP);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, cnt);
	if(!nf_sysdb_set_key0("disp.hd_spot.SCNT", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_DISP, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_DISP);

	DDEBUG_END
	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
static guint prvGetHDSpotElemData(SpotElementData *data, guint spot_idx, guint nth)
{
	guint ret = 0;
	GValue ret_value = {0,};
	
	if(nf_sysdb_get_key2("disp.hd_spot.S%d.type.T%d.type", spot_idx, nth, &ret_value, NULL))
	{
		data->type = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	if(nf_sysdb_get_key2("disp.hd_spot.S%d.type.T%d.ch", spot_idx, nth, &ret_value, NULL))
	{
		gchar tmp[MAX_SPOT_DIVISION_NUM+1];
		guint i;
    	guint conf_num = 0;

		memset(tmp, 0, sizeof(tmp));
		g_stpcpy(tmp, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);

//		parse tmp string. and set data->conf[0] ~ data->conf[15]
		for(i=0; i<MAX_SPOT_DIVISION_NUM; i++)
		{	
			if(!tmp[i])
				break;
			if(tmp[i]>='A' && tmp[i]<='P')		// analog camera
			{
        conf_num = tmp[i]-'A';	
#if defined(GUI_4CH_SUPPORT) || defined(GUI_8CH_SUPPORT)        
        if(conf_num >= g_ch)
        {
          conf_num %= g_ch;
        }
#endif
				//data->conf[i] = tmp[i]-'A';
				data->conf[i] = conf_num;
			}
			else if(tmp[i]>='a' && tmp[i]<='p')	// ip camera
			{
				data->conf[i] = tmp[i]-'a'+16;
			}
		}
	}
	else		ret = 2;

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
static guint prvSetHDSpotElemData(SpotElementData data, guint spot_idx, guint nth)
{
	guint ret = 0;
	GValue set_value = {0,};
	gchar tmp[MAX_SPOT_DIVISION_NUM+1];
	guint i;

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.type);
	if(!nf_sysdb_set_key2("disp.hd_spot.S%d.type.T%d.type", spot_idx, nth, &set_value, NULL)) 
	{
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	memset(tmp, 0, sizeof(tmp));
	for(i=0; i<MAX_SPOT_DIVISION_NUM; i++)
	{
		if(data.conf[i]>=0 && data.conf[i]<16)
		{
			tmp[i] = data.conf[i] + 'A';
		}
		else if(data.conf[i]>=16 && data.conf[i]<32)
		{
			tmp[i] = data.conf[i] - 16 + 'a';
		}
	}
	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, tmp);
	if(!nf_sysdb_set_key2("disp.hd_spot.S%d.type.T%d.ch", spot_idx, nth, &set_value, NULL)) 
	{
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_HD_spot_data(SpotData *data, guint spot_idx)
{
	guint ret = 0;
	GValue ret_value = {0,};
	guint i = 0;
	
	if(nf_sysdb_get_key1("disp.hd_spot.S%d.name", spot_idx, &ret_value, NULL))
	{
		g_stpcpy(data->title, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else	ret = 1;

	if(nf_sysdb_get_key1("disp.hd_spot.S%d.creator", spot_idx, &ret_value, NULL))
	{
		g_stpcpy(data->createdby, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else	ret = 2;

	if(nf_sysdb_get_key1("disp.hd_spot.S%d.valid_mode", spot_idx, &ret_value, NULL))
	{
		data->valid_mode = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	ret = 3;

	if(nf_sysdb_get_key1("disp.hd_spot.S%d.type.TCNT", spot_idx, &ret_value, NULL))
	{
		data->numItems = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	ret = 4;

	for(i=0; i<data->numItems; i++)
	{
		if(prvGetHDSpotElemData(&(data->items[i]), spot_idx, i))		ret = 5;
	}

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
static guint prvSetHDSpotData(SpotData data, guint spot_idx)
{
	GValue set_value = {0,};
	gchar seq_name[128];
	guint i;

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.title);
	if(!nf_sysdb_set_key1("disp.hd_spot.S%d.name", spot_idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.createdby);
	if(!nf_sysdb_set_key1("disp.hd_spot.S%d.creator", spot_idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.valid_mode);
	if(!nf_sysdb_set_key1("disp.hd_spot.S%d.valid_mode", spot_idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.numItems);
	if(!nf_sysdb_set_key1("disp.hd_spot.S%d.type.TCNT", spot_idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	for(i=0; i<data.numItems; i++)
	{
		if(prvSetHDSpotElemData(data.items[i], spot_idx, i))
		{
			DDEBUG_FAIL
			return 5;
		}
	}

	return 0;
}

guint DAL_set_HD_spot_data(SpotData data, guint spot_idx)
{
	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_DISP);

	ret = prvSetHDSpotData(data, spot_idx);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_DISP, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_DISP);

	DDEBUG_END
	return ret;
}

guint DAL_set_HD_spot_data_all(SpotData *data, guint spot_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_DISP);

	for(i=0; i<spot_num; i++)
	{
		ret = prvSetHDSpotData(data[i], i);
		if(ret)	break;
	}

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_DISP, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
	
	DDEBUG_END
	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_advanced_dual_data(AdvDualData *data)
{   
	guint ret = 0;
	GValue ret_value = {0,};

    gint i, j;
	gchar tmp[STRING_SIZE_32+1];

	if(nf_sysdb_get_key0("disp.advanced_dual.mode", &ret_value, NULL))
	{
		data->mode = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 1;

	if(nf_sysdb_get_key0("disp.advanced_dual.1st_monitor.type", &ret_value, NULL))
	{
		data->monitor_type[0] = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 2;

	if(nf_sysdb_get_key0("disp.advanced_dual.1st_monitor.resol", &ret_value, NULL))
	{
		data->monitor_resol[0] = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 3;

	if(nf_sysdb_get_key0("disp.advanced_dual.2nd_monitor.type", &ret_value, NULL))
	{
		data->monitor_type[1] = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 4;

	if(nf_sysdb_get_key0("disp.advanced_dual.2nd_monitor.resol", &ret_value, NULL))
	{
		data->monitor_resol[1] = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 5;

	if(nf_sysdb_get_key0("disp.advanced_dual.spot.type.TCNT", &ret_value, NULL))
	{       
		data->spot_cnt = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 6;

	for (i = 0; i < data->spot_cnt; i++)
	{
    	if (nf_sysdb_get_key1("disp.advanced_dual.spot.type.T%d.type", i, &ret_value, NULL))
    	{
    		data->spot[i].type = g_value_get_uint(&ret_value);
    		g_value_unset(&ret_value);
    	}
    	else		ret = 7;

    	if (nf_sysdb_get_key1("disp.advanced_dual.spot.type.T%d.ch", i, &ret_value, NULL))
    	{
    		memset(tmp, 0, sizeof(gchar)*(STRING_SIZE_32+1));
    		g_stpcpy(tmp, g_value_get_string(&ret_value));
    		g_value_unset(&ret_value);

    		for (j = 0; j < 32; j++)
    		{	
    			if (!tmp[j]) break;
    			
    			if (tmp[j] >= 'A' && tmp[j] <= 'P')
    			{
    				data->spot[i].conf[j] = tmp[j]-'A';
    			}
    			else if (tmp[j] >= 'a' && tmp[j] <= 'p')
    			{
    				data->spot[i].conf[j] = tmp[j]-'a'+16;
    			}
    		}
    	}
    	else		ret = 8;

	}
	
	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_set_advanced_dual_data(AdvDualData *data)
{
	guint ret = 0;
	GValue set_value = {0,};

    gint i, j;
	gchar tmp[STRING_SIZE_32+1];

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_DISP);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->mode);
	if(!nf_sysdb_set_key0("disp.advanced_dual.mode", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->monitor_type[0]);
	if(!nf_sysdb_set_key0("disp.advanced_dual.1st_monitor.type", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->monitor_resol[0]);
	if(!nf_sysdb_set_key0("disp.advanced_dual.1st_monitor.resol", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);


	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->monitor_type[1]);
	if(!nf_sysdb_set_key0("disp.advanced_dual.2nd_monitor.type", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->monitor_resol[1]);
	if(!nf_sysdb_set_key0("disp.advanced_dual.2nd_monitor.resol", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->spot_cnt);
	if(!nf_sysdb_set_key0("disp.advanced_dual.spot.type.TCNT", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);

	for (i = 0; i < data->spot_cnt; i++)
	{
    	g_value_init(&set_value, G_TYPE_UINT);
    	g_value_set_uint(&set_value, data->spot[i].type);
    	if(!nf_sysdb_set_key1("disp.advanced_dual.spot.type.T%d.type", i, &set_value, NULL)) 
    	{
    		DDEBUG_FAIL
    		return 7;
    	}
    	g_value_unset(&set_value);

    	memset(tmp, 0, sizeof(gchar)*(STRING_SIZE_32+1));
    	for (j = 0; j < 32; j++)
    	{
    		if(data->spot[i].conf[j] >= 0 && data->spot[i].conf[j] < 16)
    		{
    			tmp[j] = data->spot[i].conf[j] + 'A';
    		}
    		else if(data->spot[i].conf[j] >= 16 && data->spot[i].conf[j] < 32)
    		{
    			tmp[j] = data->spot[i].conf[j] - 16 + 'a';
    		}
    	}
    	g_value_init(&set_value, G_TYPE_STRING);
    	g_value_set_string(&set_value, tmp);
    	if(!nf_sysdb_set_key1("disp.advanced_dual.spot.type.T%d.ch", i, &set_value, NULL)) 
    	{
    		DDEBUG_FAIL
    		return 8;
    	}
    	g_value_unset(&set_value);
	}
	
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_DISP, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_DISP);

	DDEBUG_END
	
	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_scrSaver_data(ScrSaverData *data)
{
	guint ret = 0;
	GValue ret_value = {0,};

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_set_scrSaver_data(ScrSaverData *data)
{
	guint ret = 0;
//	GValue set_value = {0,};

	DDEBUG_START

	DDEBUG_END
	return ret;
}


/********************************************************************************
 * Setup --> System --> Pos Setup
 * ******************************************************************************/
/*
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/


guint DAL_get_pososd_data(PosOsdData *data)
{
	guint ret = 0, i;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("disp.pos.mode", &ret_value, NULL)) {
		data->disp_mode = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else ret = 1;

	if(nf_sysdb_get_key0("disp.pos.position", &ret_value, NULL)) {
		data->position = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else ret = 2;

	if(nf_sysdb_get_key0("disp.pos.font", &ret_value, NULL)) {
		data->font = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else ret = 3;

	if(nf_sysdb_get_key0("disp.pos.color_normal", &ret_value, NULL)) {
		data->normal_color = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else ret = 4;

	if(nf_sysdb_get_key0("disp.pos.duration", &ret_value, NULL)) {
		data->duration = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else ret = 6;

	if(nf_sysdb_get_key0("disp.pos.format", &ret_value, NULL)) {
		data->format = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else ret = 7;

	if(nf_sysdb_get_key0("disp.pos.scroll", &ret_value, NULL)) {
		data->scroll = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else ret = 8;

	if(nf_sysdb_get_key0("disp.pos.highlight", &ret_value, NULL)) {
		data->highlight = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else ret = 9;

	for(i=0; i < 8; i++) {
		if(nf_sysdb_get_key1("disp.pos.high.H%d.txt", i, &ret_value, NULL)) {
			g_stpcpy(data->highlight_text[i], g_value_get_string(&ret_value));
			g_value_unset(&ret_value);
		}
		else ret = 10;

    	if(nf_sysdb_get_key1("disp.pos.high.H%d.color", i, &ret_value, NULL)) {
    		data->highlight_color[i] = g_value_get_uint(&ret_value);
    		g_value_unset(&ret_value);
    	}
    	else ret = 11;
	}

	if(nf_sysdb_get_key0("disp.pos.exclude", &ret_value, NULL)) {
		data->exclude = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else ret = 12;

	for(i=0; i < 8; i++) {
		if(nf_sysdb_get_key1("disp.pos.ex.E%d.txt", i, &ret_value, NULL)) {
			g_stpcpy(data->exclude_text[i], g_value_get_string(&ret_value));
			g_value_unset(&ret_value);
		}
		else ret = 13;
	}

	return ret;
}

guint DAL_set_pososd_data(PosOsdData *data)
{
	GValue set_value = {0,};
	guint tmp, i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_DISP);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->disp_mode);
	if(!nf_sysdb_set_key0("disp.pos.mode", &set_value, NULL)) {
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->position);
	if(!nf_sysdb_set_key0("disp.pos.position", &set_value, NULL)) {
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->font);
	if(!nf_sysdb_set_key0("disp.pos.font", &set_value, NULL)) {
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->normal_color);
	if(!nf_sysdb_set_key0("disp.pos.color_normal", &set_value, NULL)) {
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->duration);
	if(!nf_sysdb_set_key0("disp.pos.duration", &set_value, NULL)) {
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->format);
	if(!nf_sysdb_set_key0("disp.pos.format", &set_value, NULL)) {
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 7;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->scroll);
	if(!nf_sysdb_set_key0("disp.pos.scroll", &set_value, NULL)) {
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 8;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->highlight);
	if(!nf_sysdb_set_key0("disp.pos.highlight", &set_value, NULL)) {
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 9;
	}
	g_value_unset(&set_value);

	for(i=0; i < 8; i++) {
		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, data->highlight_text[i]);
		if(!nf_sysdb_set_key1("disp.pos.high.H%d.txt", i, &set_value, NULL)) {
			nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
			DDEBUG_FAIL
			return 10;
		}
		g_value_unset(&set_value);

    	g_value_init(&set_value, G_TYPE_UINT);
    	g_value_set_uint(&set_value, data->highlight_color[i]);
		if(!nf_sysdb_set_key1("disp.pos.high.H%d.color", i, &set_value, NULL)) {
			nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
			DDEBUG_FAIL
			return 11;
		}
		g_value_unset(&set_value);
	}

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->exclude);
	if(!nf_sysdb_set_key0("disp.pos.exclude", &set_value, NULL)) {
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 12;
	}
	g_value_unset(&set_value);

	for(i=0; i < 8; i++) {
		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, data->exclude_text[i]);
		if(!nf_sysdb_set_key1("disp.pos.ex.E%d.txt", i, &set_value, NULL)) {
			nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
			DDEBUG_FAIL
			return 13;
		}
		g_value_unset(&set_value);
	}

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_DISP, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_DISP);

	DDEBUG_END
	return 0;
}




/******************** LIVE DISPLAY *********************/
/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_live_count()
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("disp.live.DCNT", &ret_value, NULL))
	{
		ret = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 0;

	return ret;
}

guint DAL_set_live_count(guint cnt)
{
	guint ret = 0;
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_DISP);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, cnt);
	if(!nf_sysdb_set_key0("disp.live.DCNT", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_DISP, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_DISP);

	DDEBUG_END
	return ret;
}

static guint prvGetLiveElemData(LiveElementData *data, guint idx)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key1("disp.live.D%d.type", idx, &ret_value, NULL))
	{
		data->type = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	if(nf_sysdb_get_key1("disp.live.D%d.ch", idx, &ret_value, NULL))
	{
		gchar tmp[MAX_LIVE_DIVISION_NUM+1];
		guint i;
    	guint conf_num = 0;

		memset(tmp, 0, sizeof(tmp));
		g_stpcpy(tmp, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);

		//		parse tmp string. and set data->conf[0] ~ data->conf[15]
		for(i=0; i<MAX_LIVE_DIVISION_NUM; i++)
		{
			if(!tmp[i])
				break;
			if(tmp[i]>='A' && tmp[i]<='P')		// analog camera
			{
				conf_num = tmp[i]-'A';
#if defined(GUI_4CH_SUPPORT) || defined(GUI_8CH_SUPPORT)
				if(conf_num >= g_ch)
				{
					conf_num %= g_ch;
				}
#endif
				//data->conf[i] = tmp[i]-'A';
				data->conf[i] = conf_num;
			}
			else if(tmp[i]>='a' && tmp[i]<='p')	// ip camera
			{
				data->conf[i] = tmp[i]-'a'+16;
			}
		}
	}
	else		ret = 2;

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
static guint prvSetLiveElemData(LiveElementData data, guint idx)
{
	guint ret = 0;
	GValue set_value = {0,};
	gchar tmp[MAX_LIVE_DIVISION_NUM+1];
	guint i;

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.type);
	if(!nf_sysdb_set_key1("disp.live.D%d.type", idx, &set_value, NULL))
	{
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	memset(tmp, 0, sizeof(tmp));
	for(i=0; i<MAX_LIVE_DIVISION_NUM; i++)
	{
		if(data.conf[i]>=0 && data.conf[i]<16)
		{
			tmp[i] = data.conf[i] + 'A';
		}
		else if(data.conf[i]>=16 && data.conf[i]<32)
		{
			tmp[i] = data.conf[i] - 16 + 'a';
		}
	}
	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, tmp);
	if(!nf_sysdb_set_key1("disp.live.D%d.ch", idx, &set_value, NULL))
	{
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	return ret;
}

guint DAL_get_live_data(LiveData *data, guint idx)
{
	guint ret = 0;
	GValue ret_value = {0,};
	guint i = 0;

	if(nf_sysdb_get_key1("disp.live.D%d.name", idx, &ret_value, NULL))
	{
		g_stpcpy(data->title, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else	ret = 1;

	if(nf_sysdb_get_key1("disp.live.D%d.creator", idx, &ret_value, NULL))
	{
		g_stpcpy(data->createdby, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else	ret = 2;

/*
	if(nf_sysdb_get_key1("disp.live.D%d.valid_mode", idx, &ret_value, NULL))
	{
		data->valid_mode = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	ret = 3;
*/

	if(prvGetLiveElemData(&(data->items), idx))
		ret = 5;

	return ret;
}

static guint prvSetLiveData(LiveData data, guint idx)
{
	GValue set_value = {0,};
	gchar seq_name[128];
	guint i;

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.title);
	if(!nf_sysdb_set_key1("disp.live.D%d.name", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.createdby);
	if(!nf_sysdb_set_key1("disp.live.D%d.creator", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

/*
	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.valid_mode);
	if(!nf_sysdb_set_key1("disp.live.D%d.valid_mode", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);
*/

	if(prvSetLiveElemData(data.items, idx))
	{
		DDEBUG_FAIL
			return 5;
	}

	return 0;
}

guint DAL_set_live_data(LiveData data, guint idx)
{
	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_DISP);

	ret = prvSetLiveData(data, idx);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_DISP, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_DISP);

	DDEBUG_END
	return ret;
}

guint DAL_set_live_data_all(LiveData *data, guint num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_DISP);

	for(i=0; i<num; i++)
	{
		ret = prvSetLiveData(data[i], i);
		if(ret)	break;
	}

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_DISP, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_DISP);

	DDEBUG_END
	return ret;
}






/***********************************************************************
 * Setup --> System --> Sound
 * *********************************************************************/
/**
	@brief
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_audio_data(AudioData *data)
{
	guint ret = 0;
	GValue ret_value = {0,};
	gint i;
	gchar buf[20];

	if(nf_sysdb_get_key0("audio.live", &ret_value, NULL))
	{
		data->liveAudio = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	if(nf_sysdb_get_key0("audio.ch", &ret_value, NULL))
	{
		data->channel = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 2;
#ifdef __SUPPORT_BS8418__
	for(i=0; i<4; i++) {
		sprintf(buf, "audio.A%d.mode", i);

		if(nf_sysdb_get_key0(buf, &ret_value, NULL))
		{
			data->aud_mode[i] = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else		ret = 3;
	}

	for(i=0; i<4; i++) {
		sprintf(buf, "audio.A%d.audio_in", i);

		if(nf_sysdb_get_key0(buf, &ret_value, NULL))
		{
			data->aud_in[i] = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else		ret = 4;
	}
#endif
	if(nf_sysdb_get_key0("audio.tx", &ret_value, NULL))
	{
		data->netAudioTx = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 5;

	if(nf_sysdb_get_key0("audio.rx", &ret_value, NULL))
	{
		data->netAudioRx = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 6;

	if(nf_sysdb_get_key0("audio.output_type", &ret_value, NULL))
	{
		data->audioType = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 8;

	return ret;
}

/**
	@brief
	@param[in]
	@param[out]
	@return
*/
guint DAL_set_audio_data(AudioData *data)
{
	GValue set_value = {0,};
	gint i;
	gchar buf[20];

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_AUDIO);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->liveAudio);
	if(!nf_sysdb_set_key0("audio.live", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_AUDIO);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->channel);
	if(!nf_sysdb_set_key0("audio.ch", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_AUDIO);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

#ifdef __SUPPORT_BS8418__
	for(i=0; i<4; i++) {
		sprintf(buf, "audio.A%d.mode", i);

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data->aud_mode[i]);
		if(!nf_sysdb_set_key0(buf, &set_value, NULL))
		{
			nf_sysdb_unlock(NF_SYSDB_CATE_AUDIO);
			DDEBUG_FAIL
				return 3;
		}
		g_value_unset(&set_value);
	}


	for(i=0; i<4; i++) {
		sprintf(buf, "audio.A%d.audio_in", i);

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data->aud_in[i]);
		if(!nf_sysdb_set_key0(buf, &set_value, NULL))
		{
			nf_sysdb_unlock(NF_SYSDB_CATE_AUDIO);
			DDEBUG_FAIL
				return 4;
		}
		g_value_unset(&set_value);
	}
#endif
	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->netAudioTx);
	if(!nf_sysdb_set_key0("audio.tx", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_AUDIO);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->netAudioRx);
	if(!nf_sysdb_set_key0("audio.rx", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_AUDIO);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->audioType);
	if(!nf_sysdb_set_key0("audio.output_type", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_AUDIO);
		DDEBUG_FAIL
		return 8;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_AUDIO, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_AUDIO);

	DDEBUG_END
	return 0;
}

/**
	@brief
	@param[in]
	@param[out]
	@return
*/
gboolean DAL_set_audio_channel(guint ch)
{
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_AUDIO);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, ch);
	if(!nf_sysdb_set_key0("audio.ch", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_AUDIO);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_AUDIO, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_AUDIO);

	DDEBUG_END
	return TRUE;
}

#if defined(_ANF_0824CL)
/**
	@brief
	@param[in]
	@param[out]
	@return
*/
gboolean DAL_get_audio_volume(guint *volume)
{
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("audio.volume", &ret_value, NULL))
	{
		*volume = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	return TRUE;
}

/**
	@brief
	@param[in]
	@param[out]
	@return
*/
gboolean DAL_set_audio_volume(guint volume)
{
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_AUDIO);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, volume);
	if(!nf_sysdb_set_key0("audio.volume", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_AUDIO);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_AUDIO);

	DDEBUG_END
	return TRUE;
}

#endif

/**
	@brief
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_buzzer_data(BuzzerData *data)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("audio.keybuzzer", &ret_value, NULL))
	{
		data->keypad = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	if(nf_sysdb_get_key0("audio.remote", &ret_value, NULL))
	{
		data->remote = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 2;

	return ret;
}

/**
	@brief
	@param[in]
	@param[out]
	@return
*/
guint DAL_set_buzzer_data(BuzzerData *data)
{
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_AUDIO);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->keypad);
	if(!nf_sysdb_set_key0("audio.keybuzzer", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_AUDIO);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->remote);
	if(!nf_sysdb_set_key0("audio.remote", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_AUDIO);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_AUDIO, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_AUDIO);

	DDEBUG_END
	return 0;
}








/*********************************************************************************
 * Setup --> System --> System
 * *******************************************************************************/

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
/*
<item key="sys.date.daylight"			type="BOOL" 	min="" max="" val="" />
*/

guint DAL_get_Dst_data()
{
    gint ret =0;
    GValue ret_value = {0,};

    if(nf_sysdb_get_key0("sys.date.daylight", &ret_value, NULL))
	{
		ret = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else ret = 1;

	return ret;
}

guint DAL_get_dateTime_data(DateTimeData *data)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("sys.date.dateform", &ret_value, NULL))
	{
		data->dateFormat = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	if(nf_sysdb_get_key0("sys.date.timeform", &ret_value, NULL))
	{
		data->timeFormat = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 2;

	if(nf_sysdb_get_key0("sys.date.timesvr", &ret_value, NULL))
	{
		g_stpcpy(data->timeServer, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 3;

	if(nf_sysdb_get_key0("sys.date.tz_index", &ret_value, NULL))
	{
		data->timeZone = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 5;

	if(nf_sysdb_get_key0("sys.date.daylight", &ret_value, NULL))
	{
		data->dst = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 6;

	if(nf_sysdb_get_key0("sys.date.auto_sync", &ret_value, NULL))
	{
		data->auto_timesync = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 7;

	if(nf_sysdb_get_key0("sys.date.auto_interval_min", &ret_value, NULL))
	{
		data->auto_interval_min = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 8;

	if(nf_sysdb_get_key0("sys.date.sync_time", &ret_value, NULL))
	{
		data->sync_time = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 9;

	if(nf_sysdb_get_key0("sys.date.last_sync_time", &ret_value, NULL))
	{
		data->last_sync_time = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 10;

	if(nf_sysdb_get_key0("sys.date.sync_freq", &ret_value, NULL))
	{
		data->sync_freq = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 11;

	if(nf_sysdb_get_key0("sys.date.time_sync_off", &ret_value, NULL))
	{
		data->time_sync_off = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 12;

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_set_dateTime_data(DateTimeData *data)
{
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_SYS);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->dateFormat);
	if(!nf_sysdb_set_key0("sys.date.dateform", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->timeFormat);
	if(!nf_sysdb_set_key0("sys.date.timeform", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->timeServer);
	if(!nf_sysdb_set_key0("sys.date.timesvr", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->timeZone);
	if(!nf_sysdb_set_key0("sys.date.tz_index", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->dst);
	if(!nf_sysdb_set_key0("sys.date.daylight", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->auto_timesync);
	if(!nf_sysdb_set_key0("sys.date.auto_sync", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 7;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->auto_interval_min);
	if(!nf_sysdb_set_key0("sys.date.auto_interval_min", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 8;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->sync_time);
	if(!nf_sysdb_set_key0("sys.date.sync_time", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 9;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->last_sync_time);
	if(!nf_sysdb_set_key0("sys.date.last_sync_time", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 10;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->sync_freq);
	if(!nf_sysdb_set_key0("sys.date.sync_freq", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 11;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->time_sync_off);
	if(!nf_sysdb_set_key0("sys.date.time_sync_off", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 12;
	}
	g_value_unset(&set_value);

//	nf_notify_fire_params("sysdb_tformat", 0, 0, 0, 0);
//	nf_notify_fire_params("sysdb_tzone", 0, 0, 0, 0);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_SYS, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_SYS);

	DDEBUG_END
	return 0;
}

gboolean DAL_set_last_sync_time(guint sync_time)
{
    GValue set_value;

    DDEBUG_START

    nf_sysdb_lock(NF_SYSDB_CATE_SYS);

    g_value_init(&set_value, G_TYPE_UINT);
    g_value_set_uint(&set_value, sync_time);
    if(!nf_sysdb_set_key0("sys.date.last_sync_time", &set_value, NULL))
    {
        nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
        DDEBUG_FAIL
        return TRUE;
    }
    g_value_unset(&set_value);

    nf_sysdb_unlock(NF_SYSDB_CATE_SYS);

    DDEBUG_END

    return FALSE;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
gboolean DAL_get_dateTime_format(guint *dFormat, guint *tFormat)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(dFormat != NULL)
	{
		if(nf_sysdb_get_key0("sys.date.dateform", &ret_value, NULL))
		{
			*dFormat = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return FALSE;
		}
	}

	if(tFormat != NULL)
	{
		if(nf_sysdb_get_key0("sys.date.timeform", &ret_value, NULL))
		{
			*tFormat = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
			DDEBUG_FAIL
			return FALSE;
		}
	}

	return TRUE;
}

gboolean DAL_set_timezone(guint timezone)
{
    GValue set_value = {0,};
    
    DDEBUG_START

    nf_sysdb_lock(NF_SYSDB_CATE_SYS);
    
	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, timezone);
	if(!nf_sysdb_set_key0("sys.date.tz_index", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);
	
	nf_sysdb_unlock(NF_SYSDB_CATE_SYS);

	DDEBUG_END

	return TRUE;
}

gint DAL_get_dst()
{
	guint ret = 0;
	GValue ret_value = {0,};


	if(nf_sysdb_get_key0("sys.date.daylight", &ret_value, NULL))
	{
		ret = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		ret = -1;
	}

	return ret;
}

gint DAL_get_tz_data(guint *timeZone)
{
	guint ret = 0;
	GValue ret_value = {0,};


	if(nf_sysdb_get_key0("sys.date.tz_index", &ret_value, NULL))
	{
		*timeZone = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		ret = -1;
	}

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_sysManage_data(SysManageData *data)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("sys.info.sysid", &ret_value, NULL))
	{
		g_stpcpy(data->name, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	if(nf_sysdb_get_key0("sys.info.passwd_enable", &ret_value, NULL))
	{
		data->enPassword = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 2;

	if(nf_sysdb_get_key0("sys.info.passwd_expired", &ret_value, NULL))
	{
		guint temp;
		temp = g_value_get_uint(&ret_value);

		switch(temp)
		{
			case 0:		data->pwExpired = 0;	break;
			case 1:		data->pwExpired = 1;	break;
			case 2:		data->pwExpired = 2;	break;
			case 4:		data->pwExpired = 3;	break;
			case 6:		data->pwExpired = 4;	break;
			case 12:	data->pwExpired = 5;	break;
			default:	data->pwExpired = 0;	break;
		}

		g_value_unset(&ret_value);
	}
	else		ret = 3;

	if(nf_sysdb_get_key0("sys.info.poe_limit", &ret_value, NULL))
	{
		data->poeLimt = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 4;

	if(nf_sysdb_get_key0("sys.info.poe_hub_limit", &ret_value, NULL))
	{
		data->poeHubLimt = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 5;

	if(nf_sysdb_get_key0("sys.info.sig_type", &ret_value, NULL))
	{
		data->sigType = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 6;

	if(nf_sysdb_get_key0("sys.info.swmode", &ret_value, NULL))
	{
		data->swmode = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 7;

	if(nf_sysdb_get_key0("sys.info.remotefw_check", &ret_value, NULL))
	{
		data->remotefw_check = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 8;

	if(nf_sysdb_get_key0("sys.info.passwd_changed_by_force", &ret_value, NULL))
	{
		data->pwChanged = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 9;

	if(nf_sysdb_get_key0("sys.info.poemode", &ret_value, NULL))
	{
		data->poemode = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 10;

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_set_sysManage_data(SysManageData *data)
{
	GValue set_value = {0,};
	guint temp;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_SYS);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->name);
	if(!nf_sysdb_set_key0("sys.info.sysid", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->enPassword);
	if(!nf_sysdb_set_key0("sys.info.passwd_enable", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	switch(data->pwExpired)
	{
		case 0:		temp = 0;	break;
		case 1:		temp = 1;	break;
		case 2:		temp = 2;	break;
		case 3:		temp = 4;	break;
		case 4:		temp = 6;	break;
		case 5:		temp = 12;	break;
		default:	temp = 0;	break;
	}

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, temp);
	if(!nf_sysdb_set_key0("sys.info.passwd_expired", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->poeLimt);
	if(!nf_sysdb_set_key0("sys.info.poe_limit", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->poeHubLimt);
	if(!nf_sysdb_set_key0("sys.info.poe_hub_limit", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->sigType);
	if(!nf_sysdb_set_key0("sys.info.sig_type", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->swmode);
	if(!nf_sysdb_set_key0("sys.info.swmode", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 7;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->remotefw_check);
	if(!nf_sysdb_set_key0("sys.info.remotefw_check", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 8;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->pwChanged);
	if(!nf_sysdb_set_key0("sys.info.passwd_changed_by_force", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 9;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->poemode);
	if(!nf_sysdb_set_key0("sys.info.poemode", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 10;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_SYS, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_SYS);

	DDEBUG_END
	return 0;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_pw_expired_conf()
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("sys.info.passwd_expired", &ret_value, NULL))
	{
		ret = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}

	return ret;
}

gboolean DAL_get_encorder_mode()
{
    GValue ret_value = {0,};
    gboolean ret = 0;

    if (nf_sysdb_get_key0("sys.info.encoder_mode", &ret_value, NULL))
    {
        ret = g_value_get_boolean(&ret_value);
        g_value_unset(&ret_value);
    }

    return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/

guint DAL_get_mac_address(gchar *mac)
{
    guint ret = 0;
    GValue ret_value = {0,};

    if(nf_sysdb_get_key0("sys.info.mac", &ret_value, NULL))
	{
        g_stpcpy(mac, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else    ret = 1;

	return ret;
}

guint DAL_get_sysInfo_data(SysInfoData *data)
{
	guint ret = 0;
	guint tmp;
	GValue ret_value = {0,};


	if(nf_sysdb_get_key0("sys.info.model", &ret_value, NULL))
	{
		g_stpcpy(data->model, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	if(nf_sysdb_get_key0("sys.info.sysid", &ret_value, NULL))
	{
		g_stpcpy(data->sysId, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 2;

	if(nf_sysdb_get_key0("sys.info.swver", &ret_value, NULL))
	{
		g_stpcpy(data->swVer, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 3;

	if(nf_sysdb_get_key0("sys.info.hwver", &ret_value, NULL))
	{
		g_stpcpy(data->hwVer, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 4;

	if(nf_sysdb_get_key0("net.proto.ipaddr", &ret_value, NULL))
	{
		guint ip[4];

		tmp = g_value_get_uint(&ret_value);
		prvIntToIP(ip, tmp);
		g_sprintf(data->ipAddr, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
		g_value_unset(&ret_value);
	}
	else		ret = 5;

	if(nf_sysdb_get_key0("sys.info.mac", &ret_value, NULL))
	{
		g_stpcpy(data->macAddr, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 6;
#if defined(__DIGI_SUPPORT__)
	if(nf_sysdb_get_key0("net.ddns.hostname", &ret_value, NULL))
	{
		g_stpcpy(data->ddnsName, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 7;
#else   /*!__DIGI_SUPPORT__*/
	if(nf_sysdb_get_key0("net.proto.ddnssvr", &ret_value, NULL))
	{
		g_stpcpy(data->ddnsName, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 7;
#endif  /*__DIGI_SUPPORT__*/

	if(nf_sysdb_get_key0("net.rtp.rtspport", &ret_value, NULL))
	{
		data->rtspPort= g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 8;

	if(nf_sysdb_get_key0("net.proto.webport", &ret_value, NULL))
	{
		data->webPort = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 9;

//	if(nf_sysdb_get_key0("sys.info.site", &ret_value, NULL))
//	{
//		g_stpcpy(data->site, g_value_get_string(&ret_value));
//		g_value_unset(&ret_value);
//	}
//	else		ret = 9;

	if(nf_sysdb_get_key0("sys.info.fwup_time", &ret_value, NULL))
	{
		data->fwupTime = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 10;

	return ret;
}

gboolean DAL_set_fwup_state(gint fwup_state)
{
	GValue set_value = {0};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_SYS);

	g_value_init(&set_value, G_TYPE_INT);
	g_value_set_int(&set_value, fwup_state);

	if(!nf_sysdb_set_key0("sys.info.fwup_state", &set_value, NULL)) {
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_SYS, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_SYS);

	return TRUE;
}

gint DAL_get_fwup_state()
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("sys.info.fwup_state", &ret_value, NULL))
	{
		ret = g_value_get_int(&ret_value);
		g_value_unset(&ret_value);
	}
	return ret;
}

gboolean DAL_set_fwup_date(guint fwup_time)
{
	GValue set_value = {0};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_SYS);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, fwup_time);

	if(!nf_sysdb_set_key0("sys.info.fwup_time", &set_value, NULL)) {
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_SYS, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_SYS);

	return TRUE;
}

gboolean DAL_set_sig_type(gint type)
{
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_SYS);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, type);
	if(!nf_sysdb_set_key0("sys.info.sig_type", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_SYS, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_SYS);

	DDEBUG_END
	return TRUE;
}


gint DAL_get_sig_type()
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("sys.info.sig_type", &ret_value, NULL))
	{
		ret = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);

	}else
		ret = -1;

	return ret;
}

gboolean DAL_set_agr_policy(gint agr)
{
	GValue set_value = {0,};
	
	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_SYS);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, agr);
	if(!nf_sysdb_set_key0("sys.info.agr_policy", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_SYS, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_SYS);

	DDEBUG_END
	return TRUE;
}

gint DAL_get_agr_policy()
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("sys.info.agr_policy", &ret_value, NULL))
	{
		ret = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	
	}else
		ret = -1;

	return ret;
}

guint DAL_set_dev_authcode(gchar *code)
{
    GValue set_value = {0,};

    DDEBUG_START

    nf_sysdb_lock(NF_SYSDB_CATE_SYS);

    g_value_init(&set_value, G_TYPE_STRING);
    g_value_set_string(&set_value, code);
    if(!nf_sysdb_set_key0("sys.info.guard.authcode", &set_value, NULL))
    {
        nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
        DDEBUG_FAIL
        return 1;
    }
    g_value_unset(&set_value);

    nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_DISP, 0, 0, 0);

    nf_sysdb_unlock(NF_SYSDB_CATE_SYS);

    DDEBUG_END

    return 0;
}

guint DAL_get_dev_authcode(gchar *code)
{
	guint ret = 0;
	GValue ret_value = {0,};
	
	if(nf_sysdb_get_key0("sys.info.guard.authcode", &ret_value, NULL))
	{
		g_stpcpy(code, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	return ret;
}

guint DAL_get_fw_version(gchar *version)
{
	guint ret = 0;
	GValue ret_value = {0,};


	if(nf_sysdb_get_key0("sys.info.swver", &ret_value, NULL))
	{
		g_stpcpy(version, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	return ret;
}

gboolean DAL_get_fac_init_func(void)
{
	gboolean ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("sys.init.func", &ret_value, NULL))
	{
		ret = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}

	return ret;
}

gboolean DAL_get_fac_init_run(void)
{
	gboolean ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("sys.init.run", &ret_value, NULL))
	{
		ret = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}

	return ret;
}

guint DAL_set_fac_init_run(gboolean data)
{
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_SYS);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data);
	if(!nf_sysdb_set_key0("sys.init.run", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_SYS);

	DDEBUG_END

	return 0;
}

guint DAL_get_fac_init_data(FacInitData *data)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("disp.osd.lang", &ret_value, NULL))
	{
		g_stpcpy(data->lang, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else	 	ret = 1;

	if(nf_sysdb_get_key0("sys.date.tz_index", &ret_value, NULL))
	{
		data->timeZone = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 2;

	if(nf_sysdb_get_key0("sys.info.sig_type", &ret_value, NULL))
	{
		data->sigType = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 3;

	return ret;
}

guint DAL_set_fac_init_data(FacInitData *data)
{
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_DISP);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->lang);
	if(!nf_sysdb_set_key0("disp.osd.lang", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_DISP, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_DISP);

	nf_sysdb_lock(NF_SYSDB_CATE_SYS);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->timeZone);
	if(!nf_sysdb_set_key0("sys.date.tz_index", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->sigType);
	if(!nf_sysdb_set_key0("sys.info.sig_type", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_SYS, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_SYS);

	DDEBUG_END
	return 0;
}

guint DAL_get_hub1_version(gchar *version)
{
	guint ret = 0;
	GValue ret_value = {0,};


	if(nf_sysdb_get_key0("sys.info.hubver1", &ret_value, NULL))
	{
		g_stpcpy(version, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	return ret;
}

guint DAL_get_hub2_version(gchar *version)
{
	guint ret = 0;
	GValue ret_value = {0,};


	if(nf_sysdb_get_key0("sys.info.giga_hubver2", &ret_value, NULL))
	{
		g_stpcpy(version, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	return ret;
}

guint DAL_get_disp_fw_version(gchar *version)
{
	guint ret = 0;
	GValue ret_value = {0,};


	if(nf_sysdb_get_key0("sys.info.disp_swver", &ret_value, NULL))
	{
		g_stpcpy(version, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	return ret;
}

guint DAL_get_disp_hw_version(gchar *version)
{
	guint ret = 0;
	GValue ret_value = {0,};


	if(nf_sysdb_get_key0("sys.info.disp_hwver", &ret_value, NULL))
	{
		g_stpcpy(version, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_set_sysInfo_data(SysInfoData *data)
{
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_SYS);


	gchar	sysId[STRING_SIZE_32];
	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->sysId);
	if(!nf_sysdb_set_key0("sys.info.sysid", &set_value, NULL)) {
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);

		DDEBUG_FAIL
		return 1;
	}
	 g_value_unset(&set_value);

	gchar	model[STRING_SIZE_64];
	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->model);
	if(!nf_sysdb_set_key0("sys.info.model", &set_value, NULL)) {
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);

		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

#if 0
	//	gchar	swVer[STRING_SIZE_32];
	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->swVer);
	if(!nf_sysdb_set_key0("sys.info.swVer", &set_value, NULL)) {
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);

		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	//	gchar	hwVer[STRING_SIZE_32];
	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->hwVer);
	if(!nf_sysdb_set_key0("sys.info.hwVer", &set_value, NULL)) {
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);

		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	//	gchar	macAddr[STRING_SIZE_32];
	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->macAddr);
	if(!nf_sysdb_set_key0("sys.info.mac", &set_value, NULL)) {
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);

		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	//	gchar	site[STRING_SIZE_32];
	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->site);
	if(!nf_sysdb_set_key0("sys.info.site", &set_value, NULL)) {
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);

		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);

	#endif

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_SYS, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_SYS);

	DDEBUG_END
	return 0;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_SystemID()
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("sys.ctrl.addr", &ret_value, NULL))
	{
		ret = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}

	return ret;
}

guint DAL_get_RemoconID()
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("sys.ctrl.rmcID", &ret_value, NULL))
	{
		ret = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_controlDev_data(ControlDevData *data)
{
	guint ret = 0;
	GValue ret_value = {0,};
	guint tmp;

	if(nf_sysdb_get_key0("sys.ctrl.addr", &ret_value, NULL))
	{
		data->sysID = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	if(nf_sysdb_get_key0("sys.ctrl.protocol", &ret_value, NULL))
	{
		data->proto = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 2;

	if(nf_sysdb_get_key0("sys.ctrl.baud", &ret_value, NULL))
	{
		guint tmp;

		tmp = prvBaudrateToIndex(g_value_get_uint(&ret_value));
		if(tmp == BAUD_NONE)	ret = 3;
		else	data->baud = (guint)tmp;
		g_value_unset(&ret_value);
	}
	else		ret = 3;

	if(nf_sysdb_get_key0("sys.ctrl.rmcID", &ret_value, NULL))
	{
		data->rmcID = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 1;

#ifdef __SUPPORT_BS8418__
	if(nf_sysdb_get_key0("sys.ctrl.port", &ret_value, NULL))
	{
		data->port = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 4;

	if(nf_sysdb_get_key0("sys.ctrl.remote_id", &ret_value, NULL))
	{
		data->remote_id = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 5;

	if(nf_sysdb_get_key0("sys.ups.protocol", &ret_value, NULL))
	{
		data->ups_proto = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 6;
#endif
	if(nf_sysdb_get_key0("sys.secom_dual.act", &ret_value, NULL))
	{
		data->secomdual_act = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 7;

	if(nf_sysdb_get_key0("sys.secom_dual.ipaddr", &ret_value, NULL))
	{
		tmp = g_value_get_uint(&ret_value);
		prvIntToIP(data->secomdual_ipaddr, tmp);
		g_value_unset(&ret_value);
	}
	else		ret = 8;

	if(nf_sysdb_get_key0("sys.secom_dual.port", &ret_value, NULL))
	{
		data->secomdual_port = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 9;



	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/

/*
guint DAL_get_controlDev_protocol_name(gchar *name, guint proto_num)
{
	GValue ret_value = {0,};

	if(nf_sysdb_get_key1("sys.info.keyctrl.K%d.name", proto_num, &ret_value, NULL))
	{
		g_stpcpy(name, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else	 return 0;

	return 1;
}
*/

guint DAL_get_controlDev_protocol(gchar *name, gint idx)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key1("sys.info.keyctrl.K%d.name", idx,  &ret_value, NULL))
	{
		g_stpcpy(name, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else	ret = 1;

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
gboolean DAL_get_controlDev_addr(guint *addr)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("sys.ctrl.addr", &ret_value, NULL))
	{
		*addr = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	return TRUE;
}


/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_set_controlDev_data(ControlDevData *data)
{
	GValue set_value = {0,};
	guint tmp;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_SYS);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->sysID);
	if(!nf_sysdb_set_key0("sys.ctrl.addr", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->proto);
	if(!nf_sysdb_set_key0("sys.ctrl.protocol", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	tmp = prvIndexToBaudrate(data->baud);
	if(!tmp)
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 3;
	}

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, tmp);
	if(!nf_sysdb_set_key0("sys.ctrl.baud", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->rmcID);
	if(!nf_sysdb_set_key0("sys.ctrl.rmcID", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

#ifdef __SUPPORT_BS8418__
	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->port);
	if(!nf_sysdb_set_key0("sys.ctrl.port", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->remote_id);
	if(!nf_sysdb_set_key0("sys.ctrl.remote_id", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->ups_proto);
	if(!nf_sysdb_set_key0("sys.ups.protocol", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);
#endif

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->secomdual_act);
	if(!nf_sysdb_set_key0("sys.secom_dual.act", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 7;
	}
	g_value_unset(&set_value);

	tmp = prvIPToInt(data->secomdual_ipaddr);
	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, tmp);
	if(!nf_sysdb_set_key0("sys.secom_dual.ipaddr", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 8;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->secomdual_port);
	if(!nf_sysdb_set_key0("sys.secom_dual.port", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 9;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_SYS, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_SYS);

	DDEBUG_END
	return 0;
}



#if defined(__SAMSUNG_UI__)
gboolean DAL_get_remote_id(guint *id)
{
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("sys.ctrl.remote_id", &ret_value, NULL))
	{
		*id = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	return TRUE;
}

gboolean DAL_set_remote_id(guint id)
{
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_SYS);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, id);
	if(!nf_sysdb_set_key0("sys.ctrl.remote_id", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);

		DDEBUG_FAIL

		return FALSE;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_SYS, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_SYS);

	DDEBUG_END

	return TRUE;
}
#endif


guint DAL_get_security_data(SecurityData *data)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("sys.security.support_audio", &ret_value, NULL))
	{
		data->audio = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	if(nf_sysdb_get_key0("sys.security.support_snapshot", &ret_value, NULL))
	{
		data->snapshot = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 2;

	if(nf_sysdb_get_key0("sys.security.enhanced_pw", &ret_value, NULL))
	{
		data->pwrule = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 3;

	if(nf_sysdb_get_key0("sys.security.login_search_arch", &ret_value, NULL))
	{
		data->loginSearchArch = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 4;

	if(nf_sysdb_get_key0("sys.security.pw_raw_backup", &ret_value, NULL))
	{
		data->pwRawbackup = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 5;

	if(nf_sysdb_get_key0("sys.security.usable_defpw", &ret_value, NULL))
	{
		data->usable_defpw = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 6;

	if(nf_sysdb_get_key0("sys.security.double_login", &ret_value, NULL))
	{
		data->double_login = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 7;

	if(nf_sysdb_get_key0("sys.security.id_input_mode", &ret_value, NULL))
	{
		data->id_input_mode = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 8;

	if(nf_sysdb_get_key0("sys.security.pin_id_input_mode", &ret_value, NULL))
	{
		data->pin_id_input_mode = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 9;

	if(nf_sysdb_get_key0("sys.security.enhanced_id", &ret_value, NULL))
	{
		data->idrule = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 10;

	if(nf_sysdb_get_key0("sys.security.question_pw_reset_web", &ret_value, NULL))
	{
		data->question_pw_reset = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else	ret = 11;

	if(nf_sysdb_get_key0("sys.diagnostic_data.enable", &ret_value, NULL))
	{
		data->diagnostic_data = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 12;

	return ret;
}

gboolean DAL_get_support_audio()
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("sys.security.support_audio", &ret_value, NULL))
	{
		ret = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	return ret;
}

gboolean DAL_get_support_snapshot()
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("sys.security.support_snapshot", &ret_value, NULL))
	{
		ret = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}

	return ret;
}

gboolean DAL_get_enahnced_password()
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("sys.security.enhanced_pw", &ret_value, NULL))
	{
		ret = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	return ret;
}

gboolean DAL_get_use_double_login(gboolean *use)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("sys.security.double_login", &ret_value, NULL))
	{
		*use = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else return -1;
	
	return 0;
}

guint DAL_get_id_input_mode()
{
    guint ret = 0;
    GValue ret_value = {0,};

	if(nf_sysdb_get_key0("sys.security.id_input_mode", &ret_value, NULL))
	{
		ret = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else return ret;
	
    return ret;
}

guint DAL_get_pin_id_input_mode()
{
    guint ret = 0;
    GValue ret_value = {0,};

	if(nf_sysdb_get_key0("sys.security.pin_id_input_mode", &ret_value, NULL))
	{
		ret = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else return ret;
	
    return ret;
}

gboolean DAL_get_enahnced_id()
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("sys.security.enhanced_id", &ret_value, NULL))
	{
		ret = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	return ret;
}

/**
	@brief
	@param[in]
	@param[out]
	@return
*/
guint DAL_set_security_data(SecurityData *data)
{
	GValue set_value = {0,};
	gint i;
	gchar buf[20];

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_SYS);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->audio);
	if(!nf_sysdb_set_key0("sys.security.support_audio", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->snapshot);
	if(!nf_sysdb_set_key0("sys.security.support_snapshot", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->pwrule);
	if(!nf_sysdb_set_key0("sys.security.enhanced_pw", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->loginSearchArch);
	if(!nf_sysdb_set_key0("sys.security.login_search_arch", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->pwRawbackup);
	if(!nf_sysdb_set_key0("sys.security.pw_raw_backup", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->usable_defpw);
	if(!nf_sysdb_set_key0("sys.security.usable_defpw", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->double_login);
	if(!nf_sysdb_set_key0("sys.security.double_login", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 7;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->id_input_mode);
	if(!nf_sysdb_set_key0("sys.security.id_input_mode", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 8;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->pin_id_input_mode);
	if(!nf_sysdb_set_key0("sys.security.pin_id_input_mode", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 9;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->idrule);
	if(!nf_sysdb_set_key0("sys.security.enhanced_id", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 10;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->question_pw_reset);
	if(!nf_sysdb_set_key0("sys.security.question_pw_reset_web", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 11;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->diagnostic_data);
	if(!nf_sysdb_set_key0("sys.diagnostic_data.enable", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 12;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_SYS, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_SYS);

	DDEBUG_END
	return 0;
}

gboolean DAL_set_audio_on()
{
	guint i = 0;
	GValue set_value = {0,};
	gchar rec_auto_audio_off[33];

	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_REC);

	memset(rec_auto_audio_off, 0, sizeof(rec_auto_audio_off));

	for (i = 0; i < 32; i++)
	{
		rec_auto_audio_off[i] = '1';
	}

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, rec_auto_audio_off);

	if(!nf_sysdb_set_key0("rec.auto.audio", &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_REC, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_REC);

	DDEBUG_END
	return ret;
}

gboolean DAL_set_audio_off()
{
	guint i = 0;
	GValue set_value = {0,};
	gchar rec_audio_off[769];
	gchar rec_auto_audio_off[33];

	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_DISP);
	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, FALSE);
	if(!nf_sysdb_set_key0("disp.osd.audio", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISP);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_DISP, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_DISP);


	nf_sysdb_lock(NF_SYSDB_CATE_AUDIO);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, 0xff);
	if(!nf_sysdb_set_key0("audio.live", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_AUDIO);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, FALSE);
	if(!nf_sysdb_set_key0("audio.tx", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_AUDIO);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, FALSE);
	if(!nf_sysdb_set_key0("audio.rx", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_AUDIO);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_AUDIO, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_AUDIO);

	nf_sysdb_lock(NF_SYSDB_CATE_USR);
	for(i = 1; i<=2;i++)
	{
		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, FALSE);
		if(!nf_sysdb_set_key1("usr.grp.G%d.audio", i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 7;
		}
		g_value_unset(&set_value);

	}

	for(i = 1; i<=2;i++)
	{
		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, FALSE);
		if(!nf_sysdb_set_key1("usr.grp.G%d.microphone", i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
			return 7;
		}
		g_value_unset(&set_value);

	}

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_USR, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_USR);

	nf_sysdb_lock(NF_SYSDB_CATE_REC);
	memset(rec_audio_off, 0, sizeof(rec_audio_off));
	memset(rec_audio_off, '0', sizeof(rec_audio_off) -1);

	for(i = 0; i<9; i++)
	{
		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, rec_audio_off);
		if(!nf_sysdb_set_key1("rec.continuous.C%d.audio", i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_REC);
			DDEBUG_FAIL
			return 7;
		}
		g_value_unset(&set_value);

	}


	for(i = 0; i<9; i++)
	{
		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, rec_audio_off);
		if(!nf_sysdb_set_key1("rec.motion.M%d.audio", i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_REC);
			DDEBUG_FAIL
			return 8;
		}
		g_value_unset(&set_value);

	}

	for(i = 0; i<9; i++)
	{
		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, rec_audio_off);
		if(!nf_sysdb_set_key1("rec.alarm.A%d.audio", i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_REC);
			DDEBUG_FAIL
			return 9;
		}
		g_value_unset(&set_value);

	}

	memset(rec_auto_audio_off, 0, sizeof(rec_auto_audio_off));
	memset(rec_auto_audio_off, '0', sizeof(rec_auto_audio_off)-1);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, rec_auto_audio_off);


	if(!nf_sysdb_set_key0("rec.panic.audio", &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, rec_auto_audio_off);

	if(!nf_sysdb_set_key0("rec.netstream.audio", &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, rec_auto_audio_off);

	if(!nf_sysdb_set_key0("rec.auto.motion.audio", &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, rec_auto_audio_off);

	if(!nf_sysdb_set_key0("rec.auto.alarm.audio", &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, rec_auto_audio_off);

	if(!nf_sysdb_set_key0("rec.auto.motion_alarm.audio", &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, rec_auto_audio_off);

	if(!nf_sysdb_set_key0("rec.auto.m_intensive.audio", &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, rec_auto_audio_off);

	if(!nf_sysdb_set_key0("rec.auto.a_intensive.audio", &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, rec_auto_audio_off);

	if(!nf_sysdb_set_key0("rec.auto.ma_intensive.audio", &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, rec_auto_audio_off);

	if(!nf_sysdb_set_key0("rec.auto.audio", &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_REC, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_REC);

	DDEBUG_END
	return ret;
}


gboolean DAL_set_snapshot_off()
{
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, 0);
	if(!nf_sysdb_set_key0("act.event.email.jpeg", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, 0);
	if(!nf_sysdb_set_key0("act.event.ftp.jpeg", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);

	DDEBUG_END
	return TRUE;
}



/********************************************************************************
 * Setup --> System --> Pos Setup
 * ******************************************************************************/
/*
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/


guint DAL_get_posdev_data(PosDevData *data, guint channel)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key1("sys.pos.P%d.enable", channel, &ret_value, NULL))
	{
		data->enable = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else        ret = 1;

	if(nf_sysdb_get_key1("sys.pos.P%d.type", channel, &ret_value, NULL))
	{
		data->type = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else        ret = 2;

	if(nf_sysdb_get_key1("sys.pos.P%d.port", channel, &ret_value, NULL))
	{
		g_stpcpy(data->port, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 3;

	if(nf_sysdb_get_key1("sys.pos.P%d.protocol", channel, &ret_value, NULL))
	{
		data->protocol = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else        ret = 4;

	if(nf_sysdb_get_key1("sys.pos.P%d.char_set", channel, &ret_value, NULL))
	{
		data->char_set = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else        ret = 5;

	if(nf_sysdb_get_key1("sys.pos.P%d.baud", channel, &ret_value, NULL))
	{
		data->baud = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else        ret = 6;

	if(nf_sysdb_get_key1("sys.pos.P%d.parity", channel, &ret_value, NULL))
	{
		data->parity = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else        ret = 7;

	if(nf_sysdb_get_key1("sys.pos.P%d.databit", channel, &ret_value, NULL))
	{
		data->databit = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else        ret = 8;

	if(nf_sysdb_get_key1("sys.pos.P%d.stopbit", channel, &ret_value, NULL))
	{
		data->stopbit = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else        ret = 9;

	if(nf_sysdb_get_key1("sys.pos.P%d.transact_start", channel, &ret_value, NULL))
	{
		g_stpcpy(data->transact_start, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 10;

	if(nf_sysdb_get_key1("sys.pos.P%d.transact_end", channel, &ret_value, NULL))
	{
		g_stpcpy(data->transact_end, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 11;

	if(nf_sysdb_get_key1("sys.pos.P%d.endofline", channel, &ret_value, NULL))
	{
		g_stpcpy(data->endofline, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 12;

	if(nf_sysdb_get_key1("sys.pos.P%d.ignore", channel, &ret_value, NULL))
	{
		g_stpcpy(data->ignore, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 13;

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_set_posdev_data(PosDevData *data, guint channel)
{
	GValue set_value = {0,};
	guint tmp;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_SYS);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->enable);
	if(!nf_sysdb_set_key1("sys.pos.P%d.enable", channel, &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->type);
	if(!nf_sysdb_set_key1("sys.pos.P%d.type", channel, &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->port);
	if(!nf_sysdb_set_key1("sys.pos.P%d.port", channel, &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->protocol);
	if(!nf_sysdb_set_key1("sys.pos.P%d.protocol", channel, &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->char_set);
	if(!nf_sysdb_set_key1("sys.pos.P%d.char_set", channel, &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->baud);
	if(!nf_sysdb_set_key1("sys.pos.P%d.baud", channel, &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->parity);
	if(!nf_sysdb_set_key1("sys.pos.P%d.parity", channel, &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 7;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->databit);
	if(!nf_sysdb_set_key1("sys.pos.P%d.databit", channel, &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 8;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->stopbit);
	if(!nf_sysdb_set_key1("sys.pos.P%d.stopbit", channel, &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 9;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->transact_start);
	if(!nf_sysdb_set_key1("sys.pos.P%d.transact_start", channel, &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 10;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->transact_end);
	if(!nf_sysdb_set_key1("sys.pos.P%d.transact_end", channel, &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 11;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->endofline);
	if(!nf_sysdb_set_key1("sys.pos.P%d.endofline", channel, &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 12;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->ignore);
	if(!nf_sysdb_set_key1("sys.pos.P%d.ignore", channel, &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 13;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_SYS, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_SYS);

	DDEBUG_END
	return 0;
}

/********************************************************************************
 * Setup --> System --> License
 * ******************************************************************************/
/*
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/

guint DAL_get_license_data(LicenseData *data)
{
	guint ret = 0;
	GValue ret_value = {0,};
	gint i;

    for (i = 0; i < MAX_LICENSE_CNT; i++)
    {
    	if(nf_sysdb_get_key1("sys.lic.L%d.key", i, &ret_value, NULL))
    	{
    	    strcpy(data->key_data[i].key, g_value_get_string(&ret_value));
    		g_value_unset(&ret_value);
    	}
    	else        ret = 1;

    	if(nf_sysdb_get_key1("sys.lic.L%d.acquired_date", i, &ret_value, NULL))
    	{
    		data->key_data[i].acquired_date = g_value_get_uint(&ret_value);
    		g_value_unset(&ret_value);
    	}
    	else        ret = 2;

    	if(nf_sysdb_get_key1("sys.lic.L%d.expired_date", i, &ret_value, NULL))
    	{
    		data->key_data[i].expired_date = g_value_get_uint(&ret_value);
    		g_value_unset(&ret_value);
    	}
    	else		ret = 3;
	}

	if(nf_sysdb_get_key0("sys.lic.key_count", &ret_value, NULL))
	{
		data->count = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 4;

	return ret;
}

guint DAL_set_license_data(LicenseData *data)
{
	guint ret = 0;
	GValue set_value = {0,};
	gint i;

	nf_sysdb_lock(NF_SYSDB_CATE_SYS);

    for (i = 0; i < MAX_LICENSE_CNT; i++)
    {
    	g_value_init(&set_value, G_TYPE_STRING);
    	g_value_set_string(&set_value, data->key_data[i].key);
    	if(!nf_sysdb_set_key1("sys.lic.L%d.key", i, &set_value, NULL))
    	{
    		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
    		DDEBUG_FAIL
    		return 1;
    	}
    	g_value_unset(&set_value);

    	g_value_init(&set_value, G_TYPE_UINT);
    	g_value_set_uint(&set_value, data->key_data[i].acquired_date);
    	if(!nf_sysdb_set_key1("sys.lic.L%d.acquired_date", i, &set_value, NULL))
    	{
    		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
    		DDEBUG_FAIL
    		return 2;
    	}
    	g_value_unset(&set_value);

    	g_value_init(&set_value, G_TYPE_UINT);
    	g_value_set_uint(&set_value, data->key_data[i].expired_date);
    	if(!nf_sysdb_set_key1("sys.lic.L%d.expired_date", i, &set_value, NULL))
    	{
    		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
    		DDEBUG_FAIL
    		return 3;
    	}
    	g_value_unset(&set_value);
	}

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->count);
	if(!nf_sysdb_set_key0("sys.lic.key_count", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_SYS, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_SYS);

	return ret;
}




/********************************************************************************
 * Setup --> System --> User
 * ******************************************************************************/
/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_userManage_data(UserManageData *data, guint idx)
{
	guint ret = 0;
	GValue ret_value = {0,};
	const gchar *str = NULL;
	gchar buf[STRING_SIZE_32+1];

	if(nf_sysdb_get_key1("usr.U%d.name", idx, &ret_value, NULL))
	{
		g_stpcpy(data->id, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	if(nf_sysdb_get_key1("usr.U%d.pass", idx, &ret_value, NULL))
	{
		g_stpcpy(data->pw, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 2;

	if(nf_sysdb_get_key1("usr.U%d.grpname", idx, &ret_value, NULL))
	{
		g_stpcpy(data->group, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 3;

	if(nf_sysdb_get_key1("usr.U%d.email", idx, &ret_value, NULL))
	{
		g_stpcpy(data->email, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 4;

	if(nf_sysdb_get_key1("usr.U%d.email_notify", idx, &ret_value, NULL))
	{
		data->email_noti = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 5;

	if(nf_sysdb_get_key1("usr.U%d.covert", idx, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);

		memset(buf, 0, sizeof(buf));
		g_stpcpy(buf, str);

		memcpy(data->covert, buf, STRING_SIZE_32);
		g_value_unset(&ret_value);
	}
	else		ret = 6;

	if(nf_sysdb_get_key1("usr.U%d.expired_check", idx, &ret_value, NULL))
	{
		data->expired_check = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 7;

	if(nf_sysdb_get_key1("usr.U%d.pw_last_changed", idx, &ret_value, NULL))
	{
		data->pw_last_changed = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 8;

	if(nf_sysdb_get_key1("usr.U%d.phone", idx, &ret_value, NULL))
	{
		g_stpcpy(data->phone, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 9;

	if(nf_sysdb_get_key1("usr.U%d.phone_notify", idx, &ret_value, NULL))
	{
		data->phone_noti = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 10;

	if(nf_sysdb_get_key1("usr.U%d.email_serv", idx, &ret_value, NULL))
	{
		data->e_serv = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 11;

	if(nf_sysdb_get_key1("usr.U%d.init_pw_changed", idx, &ret_value, NULL))
	{
		data->init_pw_changed = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 12;

	if(nf_sysdb_get_key1("usr.U%d.email_certification", idx, &ret_value, NULL))
	{
		data->email_certification = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 13;

	if(nf_sysdb_get_key1("usr.U%d.phone_certification", idx, &ret_value, NULL))
	{
		data->phone_certification = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 14;

	if(nf_sysdb_get_key1("usr.U%d.certi_popup_hide", idx, &ret_value, NULL))
	{
		data->certi_popup_hide = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 15;

	if(nf_sysdb_get_key1("usr.U%d.use_pin", idx, &ret_value, NULL))
	{
		data->use_pin = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 16;

	if(nf_sysdb_get_key1("usr.U%d.pin_number", idx, &ret_value, NULL))
	{
		g_stpcpy(data->pin_number, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 17;

	if(nf_sysdb_get_key1("usr.U%d.question0", idx, &ret_value, NULL))
	{
		data->question[0] = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 18;

	if(nf_sysdb_get_key1("usr.U%d.question1", idx, &ret_value, NULL))
	{
		data->question[1] = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 19;

	if(nf_sysdb_get_key1("usr.U%d.question2", idx, &ret_value, NULL))
	{
		data->question[2] = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 20;

	if(nf_sysdb_get_key1("usr.U%d.answer0", idx, &ret_value, NULL))
	{
		g_stpcpy(data->answer[0], g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 21;

	if(nf_sysdb_get_key1("usr.U%d.answer1", idx, &ret_value, NULL))
	{
		g_stpcpy(data->answer[1], g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 22;

	if(nf_sysdb_get_key1("usr.U%d.answer2", idx, &ret_value, NULL))
	{
		g_stpcpy(data->answer[2], g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 23;

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
static guint prvSetUserManageData(UserManageData data, guint idx)
{
	GValue set_value = {0,};
	gchar buf[STRING_SIZE_32+1];

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.id);
	if(!nf_sysdb_set_key1("usr.U%d.name", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.pw);
	if(!nf_sysdb_set_key1("usr.U%d.pass", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.group);
	if(!nf_sysdb_set_key1("usr.U%d.grpname", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.email);
	if(!nf_sysdb_set_key1("usr.U%d.email", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.email_noti);
	if(!nf_sysdb_set_key1("usr.U%d.email_notify", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data.covert, STRING_SIZE_32);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);
	if(!nf_sysdb_set_key1("usr.U%d.covert", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);


	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.expired_check);
	if(!nf_sysdb_set_key1("usr.U%d.expired_check", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 7;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.pw_last_changed);
	if(!nf_sysdb_set_key1("usr.U%d.pw_last_changed", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 8;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.phone);
	if(!nf_sysdb_set_key1("usr.U%d.phone", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 9;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.phone_noti);
	if(!nf_sysdb_set_key1("usr.U%d.phone_notify", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 10;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.e_serv);
	if(!nf_sysdb_set_key1("usr.U%d.email_serv", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 11;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.init_pw_changed);
	if(!nf_sysdb_set_key1("usr.U%d.init_pw_changed", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 12;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.email_certification);
	if(!nf_sysdb_set_key1("usr.U%d.email_certification", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 13;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.phone_certification);
	if(!nf_sysdb_set_key1("usr.U%d.phone_certification", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 14;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.certi_popup_hide);
	if(!nf_sysdb_set_key1("usr.U%d.certi_popup_hide", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 15;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.use_pin);
	if(!nf_sysdb_set_key1("usr.U%d.use_pin", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 16;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.pin_number);
	if(!nf_sysdb_set_key1("usr.U%d.pin_number", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 17;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.question[0]);
	if(!nf_sysdb_set_key1("usr.U%d.question0", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 18;
	}
	g_value_unset(&set_value);
	
	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.question[1]);
	if(!nf_sysdb_set_key1("usr.U%d.question1", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 19;
	}
	g_value_unset(&set_value);
	
	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.question[2]);
	if(!nf_sysdb_set_key1("usr.U%d.question2", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 20;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.answer[0]);
	if(!nf_sysdb_set_key1("usr.U%d.answer0", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 21;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.answer[1]);
	if(!nf_sysdb_set_key1("usr.U%d.answer1", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 22;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.answer[2]);
	if(!nf_sysdb_set_key1("usr.U%d.answer2", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 23;
	}
	g_value_unset(&set_value);

	return 0;
}

guint DAL_set_userManage_data(UserManageData data, guint idx)
{
	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_USR);

	ret = prvSetUserManageData(data, idx);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_USR, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_USR);

	DDEBUG_END
	return ret;
}

guint DAL_set_userManage_data_nofire(UserManageData data, guint idx)
{
	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_USR);

	ret = prvSetUserManageData(data, idx);

//	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_USR, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_USR);

	DDEBUG_END
	return ret;
}

guint DAL_set_userManage_data_all(UserManageData *data, guint user_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_USR);

	for(i=0; i<user_num; i++)
	{
		ret = prvSetUserManageData(data[i], i);
		if(ret)	break;
	}

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_USR, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_USR);

	DDEBUG_END
	return ret;
}

guint DAL_get_user_count()
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("usr.UCNT", &ret_value, NULL))
	{
		ret = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}

	return ret;
}


guint DAL_get_expire_check_time(guint idx)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key1("usr.U%d.expired_check", idx, &ret_value, NULL))
	{
		ret = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}

	return ret;
}

guint DAL_set_expire_check_time(guint time, guint idx)
{
	GValue set_value = {0,};
	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_USR);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, time);
	if(!nf_sysdb_set_key1("usr.U%d.expired_check", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		nf_sysdb_unlock(NF_SYSDB_CATE_USR);
		return 1;
	}
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_USR);

	DDEBUG_END
	return 0;
};

guint DAL_get_pw_last_changed_time(guint idx)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key1("usr.U%d.pw_last_changed", idx, &ret_value, NULL))
	{
		ret = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}

	return ret;
}


guint DAL_set_pw_last_changed_time(guint time, guint idx)
{
	GValue set_value = {0,};
	DDEBUG_START
	nf_sysdb_lock(NF_SYSDB_CATE_USR);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, time);
	if(!nf_sysdb_set_key1("usr.U%d.pw_last_changed", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_USR);

	DDEBUG_END
	return 0;
}

guint DAL_get_init_pw_changed(guint idx)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key1("usr.U%d.init_pw_changed", idx, &ret_value, NULL))
	{
		ret = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}

	return ret;
}


guint DAL_set_init_pw_changed(guint time, guint idx)
{
	GValue set_value = {0,};
	DDEBUG_START
	nf_sysdb_lock(NF_SYSDB_CATE_USR);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, time);
	if(!nf_sysdb_set_key1("usr.U%d.init_pw_changed", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_USR);

	DDEBUG_END
	return 0;
}



/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_set_user_count(guint cnt)
{
	guint ret = 0;
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_USR);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, cnt);
	if(!nf_sysdb_set_key0("usr.UCNT", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_USR);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_USR, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_USR);

	DDEBUG_END
	return ret;
}

gboolean DAL_get_user_id(gchar *id, guint idx)
{
	GValue ret_value = {0,};

	if(nf_sysdb_get_key1("usr.U%d.name", idx, &ret_value, NULL))
	{
		g_stpcpy(id, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return FALSE;
	}

	return TRUE;
}

gboolean DAL_get_user_passwd(gchar *passwd, guint idx)
{
	GValue ret_value = {0,};

	if(nf_sysdb_get_key1("usr.U%d.pass", idx, &ret_value, NULL))
	{
		g_stpcpy(passwd, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return FALSE;
	}

	return TRUE;
}

gboolean DAL_set_user_passwd(gchar *passwd, guint idx)
{
	GValue set_value = {0,};
	DDEBUG_START
	nf_sysdb_lock(NF_SYSDB_CATE_USR);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, passwd);
	if(!nf_sysdb_set_key1("usr.U%d.pass", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_USR);

	DDEBUG_END
	return 0;

}


gboolean DAL_get_user_group(gchar *group, guint idx)
{
	GValue ret_value = {0,};

	if(nf_sysdb_get_key1("usr.U%d.grpname", idx, &ret_value, NULL))
	{
		g_stpcpy(group, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return FALSE;
	}

	return TRUE;
}

gboolean DAL_get_user_email(gchar *email, guint idx)
{
	GValue ret_value = {0,};

	if(nf_sysdb_get_key1("usr.U%d.email", idx, &ret_value, NULL))
	{
		g_stpcpy(email, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return FALSE;
	}

	return TRUE;
}

gboolean DAL_get_user_covert(gchar *covert, guint idx)
{
	GValue ret_value = {0,};
	const gchar *str = NULL;
	gchar buf[STRING_SIZE_32+1];

	if(nf_sysdb_get_key1("usr.U%d.covert", idx, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);

		memset(buf, 0, sizeof(buf));
		g_stpcpy(buf, str);

		memcpy(covert, buf, STRING_SIZE_32);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	return TRUE;
}

gboolean DAL_get_user_phone(gchar *phone, guint idx)
{
	GValue ret_value = {0,};

	if(nf_sysdb_get_key1("usr.U%d.phone", idx, &ret_value, NULL))
	{
		g_stpcpy(phone, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_FAIL
		return FALSE;
	}

	return TRUE;
}


/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_group_count()
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("usr.grp.GCNT", &ret_value, NULL))
	{
		ret = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}

	return ret;
}

gboolean DAL_get_grp_covert_shown_as(guint grp_idx)
{
    GValue ret_value = {0,};
    gboolean ret;

	if(nf_sysdb_get_key1("usr.grp.G%d.cvt_disp", grp_idx, &ret_value, NULL))
	{
		ret = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}

	return ret;
}


/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_userAuth_data(UserAuthData *data, guint idx)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key1("usr.grp.G%d.name", idx, &ret_value, NULL))
	{
		g_stpcpy(data->grp_name, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	if(nf_sysdb_get_key1("usr.grp.G%d.sys_setup", idx, &ret_value, NULL))
	{
		data->sys_setup = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 2;

	if(nf_sysdb_get_key1("usr.grp.G%d.search", idx, &ret_value, NULL))
	{
		data->search = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 3;

	if(nf_sysdb_get_key1("usr.grp.G%d.archive", idx, &ret_value, NULL))
	{
		data->archive = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 4;

	if(nf_sysdb_get_key1("usr.grp.G%d.rec_setup", idx, &ret_value, NULL))
	{
		data->rec_setup = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 5;

	if(nf_sysdb_get_key1("usr.grp.G%d.event", idx, &ret_value, NULL))
	{
		data->event = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 6;

	if(nf_sysdb_get_key1("usr.grp.G%d.audio", idx, &ret_value, NULL))
	{
		data->audio = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 7;

	if(nf_sysdb_get_key1("usr.grp.G%d.microphone", idx, &ret_value, NULL))
	{
		data->microphone = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 8;

	if(nf_sysdb_get_key1("usr.grp.G%d.remote", idx, &ret_value, NULL))
	{
		data->remote = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 9;

	if(nf_sysdb_get_key1("usr.grp.G%d.shutdown", idx, &ret_value, NULL))
	{
		data->shutdown = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 10;

	if(nf_sysdb_get_key1("usr.grp.G%d.cvt_disp", idx, &ret_value, NULL))
	{
		data->cvt_disp = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 11;

	if(nf_sysdb_get_key1("usr.grp.G%d.ptz", idx, &ret_value, NULL))
	{
		data->ptz = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 12;

	if(nf_sysdb_get_key1("usr.grp.G%d.sequrinet", idx, &ret_value, NULL))
	{
		data->sequrinet = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 13;

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
static guint prvSetUserAuthData(UserAuthData data, guint idx)
{
	GValue set_value = {0,};

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.sys_setup);
	if(!nf_sysdb_set_key1("usr.grp.G%d.sys_setup", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.search);
	if(!nf_sysdb_set_key1("usr.grp.G%d.search", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.archive);
	if(!nf_sysdb_set_key1("usr.grp.G%d.archive", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.rec_setup);
	if(!nf_sysdb_set_key1("usr.grp.G%d.rec_setup", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.event);
	if(!nf_sysdb_set_key1("usr.grp.G%d.event", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.audio);
	if(!nf_sysdb_set_key1("usr.grp.G%d.audio", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.microphone);
	if(!nf_sysdb_set_key1("usr.grp.G%d.microphone", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 7;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.remote);
	if(!nf_sysdb_set_key1("usr.grp.G%d.remote", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 8;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.shutdown);
	if(!nf_sysdb_set_key1("usr.grp.G%d.shutdown", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 9;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.cvt_disp);
	if(!nf_sysdb_set_key1("usr.grp.G%d.cvt_disp", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 10;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.ptz);
	if(!nf_sysdb_set_key1("usr.grp.G%d.ptz", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 11;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.sequrinet);
	if(!nf_sysdb_set_key1("usr.grp.G%d.sequrinet", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 12;
	}
	g_value_unset(&set_value);

	return 0;
}

guint DAL_set_userAuth_data(UserAuthData data, guint idx)
{
	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_USR);

	ret = prvSetUserAuthData(data, idx);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_USR, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_USR);

	DDEBUG_END
	return ret;
}

guint DAL_set_userAuth_data_all(UserAuthData *data, guint group_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_USR);

	for(i=0; i<group_num; i++)
	{
		ret = prvSetUserAuthData(data[i], i);
		if(ret)	break;
	}

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_USR, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_USR);

	DDEBUG_END
	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_logout_data(LogoutData *data)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("usr.auto_logout", &ret_value, NULL))
	{
		data->autoLogout = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	if(nf_sysdb_get_key0("usr.auto_logout_min", &ret_value, NULL))
	{
		data->duration = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 2;

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_set_logout_data(LogoutData *data)
{
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_USR);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->autoLogout);
	if(!nf_sysdb_set_key0("usr.auto_logout", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_USR);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->duration);
	if(!nf_sysdb_set_key0("usr.auto_logout_min", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_USR);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_USR, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_USR);

	DDEBUG_END
	return 0;
}













/******************************************************************************
 * Setup --> System --> Network
 * ****************************************************************************/

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_ipSetup_data(IPSetupData *data)
{
	guint ret = 0;
	guint tmp;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("net.proto.dhcpon", &ret_value, NULL))
	{
		data->dhcp = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	if(nf_sysdb_get_key0("net.proto.webon", &ret_value, NULL))
	{
		data->webServ = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 3;

	if(nf_sysdb_get_key0("net.proto.ipaddr", &ret_value, NULL))
	{
		tmp = g_value_get_uint(&ret_value);
		prvIntToIP(data->ip, tmp);

		g_value_unset(&ret_value);
	}
	else		ret = 4;

	if(nf_sysdb_get_key0("net.proto.gateway", &ret_value, NULL))
	{
		tmp = g_value_get_uint(&ret_value);
		prvIntToIP(data->gateway, tmp);
		g_value_unset(&ret_value);
	}
	else		ret = 5;

	if(nf_sysdb_get_key0("net.proto.subnet", &ret_value, NULL))
	{
		tmp = g_value_get_uint(&ret_value);
		prvIntToIP(data->subnet, tmp);
		g_value_unset(&ret_value);
	}
	else		ret = 6;

	if(nf_sysdb_get_key0("net.proto.dns1", &ret_value, NULL))
	{
		tmp = g_value_get_uint(&ret_value);
		prvIntToIP(data->dns1, tmp);
		g_value_unset(&ret_value);
	}
	else		ret = 7;

	if(nf_sysdb_get_key0("net.proto.dns2", &ret_value, NULL))
	{
		tmp = g_value_get_uint(&ret_value);
		prvIntToIP(data->dns2, tmp);
		g_value_unset(&ret_value);
	}
	else		ret = 8;

	if(nf_sysdb_get_key0("net.proto.clientport", &ret_value, NULL))
	{
		data->netPort = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 9;

	if(nf_sysdb_get_key0("net.proto.webport", &ret_value, NULL))
	{
		data->webPort = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 10;

	if(nf_sysdb_get_key0("net.proto.maxtxspeed", &ret_value, NULL))
	{
		data->txSpeed = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 11;

#ifdef _UPNP_SUPPORT_
	if(nf_sysdb_get_key0("net.rtp.rtspport", &ret_value, NULL))
	{
		data->rtspport = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 12;
#endif

	if(nf_sysdb_get_key0("net.proto.autoport", &ret_value, NULL))
	{
		data->autoPort = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 13;

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_set_ipSetup_data(IPSetupData *data)
{
	GValue set_value = {0,};
	guint temp;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_NET);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->dhcp);
	if(!nf_sysdb_set_key0("net.proto.dhcpon", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->webServ);
	if(!nf_sysdb_set_key0("net.proto.webon", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	temp = prvIPToInt(data->ip);
	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, temp);
	if(!nf_sysdb_set_key0("net.proto.ipaddr", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	temp = prvIPToInt(data->gateway);
	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, temp);
	if(!nf_sysdb_set_key0("net.proto.gateway", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);

	temp = prvIPToInt(data->subnet);
	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, temp);
	if(!nf_sysdb_set_key0("net.proto.subnet", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);

	temp = prvIPToInt(data->dns1);
	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, temp);
	if(!nf_sysdb_set_key0("net.proto.dns1", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 7;
	}
	g_value_unset(&set_value);

	temp = prvIPToInt(data->dns2);
	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, temp);
	if(!nf_sysdb_set_key0("net.proto.dns2", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 8;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->netPort);
	if(!nf_sysdb_set_key0("net.proto.clientport", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 9;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->webPort);
	if(!nf_sysdb_set_key0("net.proto.webport", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 10;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->txSpeed);
	if(!nf_sysdb_set_key0("net.proto.maxtxspeed", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 11;
	}
	g_value_unset(&set_value);

#ifdef _UPNP_SUPPORT_
	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->rtspport);
	if(!nf_sysdb_set_key0("net.rtp.rtspport", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 12;
	}
	g_value_unset(&set_value);
#endif

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->autoPort);
	if(!nf_sysdb_set_key0("net.proto.autoport", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 13;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_NET, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_NET);

	DDEBUG_END
	return 0;
}


/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
gboolean DAL_get_ipAddr(gchar *ipAddr)
{
	guint tmp;
	guint ip[4];
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("net.dhcp.ipaddr", &ret_value, NULL))
	{
		tmp = g_value_get_uint(&ret_value);
		prvIntToIP(ip, tmp);
		g_sprintf(ipAddr, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
		g_value_unset(&ret_value);
	}
	else
	{
		DDEBUG_END
		return FALSE;
	}

	return TRUE;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
gboolean DAL_is_dhcp_on()
{
	GValue ret_value = {0,};
	gboolean ret = FALSE;

	DDEBUG_START

	if(nf_sysdb_get_key0("net.proto.dhcpon", &ret_value, NULL)) {
		ret = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}

	DDEBUG_END
	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_ipv6_data(IPv6Data *data)
{
	guint ret = 0;
	guint tmp;
	GValue ret_value = {0,};
	gint i;
	
	if(nf_sysdb_get_key0("net.ipv6.using", &ret_value, NULL))
	{
		data->use = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	if(nf_sysdb_get_key0("net.ipv6.linklocal", &ret_value, NULL))
	{
		strcpy(data->linklocal, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 2;

	for (i = 0; i < SUPPORT_IPV6_ADDR_CNT; i++) 
	{
    	if(nf_sysdb_get_key1("net.ipv6.addr%d", i,  &ret_value, NULL))
    	{
    		strcpy(data->ipaddr[i], g_value_get_string(&ret_value));
    		g_value_unset(&ret_value);
    	}
    	else		ret = 3;
    	
    	if(nf_sysdb_get_key1("net.ipv6.prefix_length%d", i, &ret_value, NULL))
    	{
    		data->prefix_len[i] = g_value_get_uint(&ret_value);
    		g_value_unset(&ret_value);
    	}
    	else		ret = 4;
	}

	if(nf_sysdb_get_key0("net.ipv6.gateway", &ret_value, NULL))
	{
		strcpy(data->gateway, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 5;

	if(nf_sysdb_get_key0("net.ipv6.dns1", &ret_value, NULL))
	{
		strcpy(data->dns1, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 6;

	if(nf_sysdb_get_key0("net.ipv6.dns2", &ret_value, NULL))
	{
		strcpy(data->dns2, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 7;

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_set_ipv6_data(IPv6Data *data)
{
	GValue set_value = {0,};
	guint temp;
	gint i;
	
	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_NET);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->use);
	if(!nf_sysdb_set_key0("net.ipv6.using", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->linklocal);
	if(!nf_sysdb_set_key0("net.ipv6.linklocal", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

    for (i = 0; i < SUPPORT_IPV6_ADDR_CNT; i++)
    {
    	g_value_init(&set_value, G_TYPE_STRING);
    	g_value_set_string(&set_value, data->ipaddr[i]);
    	if(!nf_sysdb_set_key1("net.ipv6.addr%d", i, &set_value, NULL))
    	{
    		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
    		DDEBUG_FAIL
    		return 3;
    	}
    	g_value_unset(&set_value);

    	g_value_init(&set_value, G_TYPE_UINT);
    	g_value_set_uint(&set_value, data->prefix_len[i]);
    	if(!nf_sysdb_set_key1("net.ipv6.prefix_length%d", i, &set_value, NULL))
    	{
    		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
    		DDEBUG_FAIL
    		return 4;
    	}
    	g_value_unset(&set_value);
	}

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->gateway);
	if(!nf_sysdb_set_key0("net.ipv6.gateway", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->dns1);
	if(!nf_sysdb_set_key0("net.ipv6.dns1", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->dns2);
	if(!nf_sysdb_set_key0("net.ipv6.dns2", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 7;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_NET, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_NET);

	DDEBUG_END
	return 0;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_ipDualLan_data(IPDualLanData *data)
{
	guint ret = 0;
	guint tmp;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("net.eth2.dhcpsvr", &ret_value, NULL))
	{
		data->dhcpsvr = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	if(nf_sysdb_get_key0("net.eth2.ipaddr", &ret_value, NULL))
	{
		tmp = g_value_get_uint(&ret_value);
		prvIntToIP(data->ip, tmp);

		g_value_unset(&ret_value);
	}
	else		ret = 2;

	if(nf_sysdb_get_key0("net.eth2.gateway", &ret_value, NULL))
	{
		tmp = g_value_get_uint(&ret_value);
		prvIntToIP(data->gateway, tmp);

		g_value_unset(&ret_value);
	}
	else		ret = 3;

	if(nf_sysdb_get_key0("net.eth2.subnet", &ret_value, NULL))
	{
		tmp = g_value_get_uint(&ret_value);
		prvIntToIP(data->subnet, tmp);

		g_value_unset(&ret_value);
	}
	else		ret = 4;

	if(nf_sysdb_get_key0("net.eth2.dhcp.pool_start", &ret_value, NULL))
	{
		tmp = g_value_get_uint(&ret_value);
		prvIntToIP(data->dhcp_pool_start, tmp);

		g_value_unset(&ret_value);
	}
	else		ret = 5;

	if(nf_sysdb_get_key0("net.eth2.dhcp.pool_end", &ret_value, NULL))
	{
		tmp = g_value_get_uint(&ret_value);
		prvIntToIP(data->dhcp_pool_end, tmp);

		g_value_unset(&ret_value);
	}
	else		ret = 6;

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_set_ipDualLan_data(IPDualLanData *data)
{
	GValue set_value = {0,};
	guint temp;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_NET);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->dhcpsvr);
	if(!nf_sysdb_set_key0("net.eth2.dhcpsvr", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	temp = prvIPToInt(data->ip);
	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, temp);
	if(!nf_sysdb_set_key0("net.eth2.ipaddr", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	temp = prvIPToInt(data->gateway);
	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, temp);
	if(!nf_sysdb_set_key0("net.eth2.gateway", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	temp = prvIPToInt(data->subnet);
	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, temp);
	if(!nf_sysdb_set_key0("net.eth2.subnet", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	temp = prvIPToInt(data->dhcp_pool_start);
	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, temp);
	if(!nf_sysdb_set_key0("net.eth2.dhcp.pool_start", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);

	temp = prvIPToInt(data->dhcp_pool_end);
	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, temp);
	if(!nf_sysdb_set_key0("net.eth2.dhcp.pool_end", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_NET, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_NET);

	DDEBUG_END
	return 0;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_ddns_data(DDNSData *data)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("net.proto.ddnson", &ret_value, NULL))
	{
		data->enable = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	if(nf_sysdb_get_key0("net.proto.ddnssvr", &ret_value, NULL))
	{
		g_stpcpy(data->server, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 2;


	if(nf_sysdb_get_key0("net.ddns.username", &ret_value, NULL))
	{
		g_stpcpy(data->id, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 3;

	if(nf_sysdb_get_key0("net.ddns.passwd", &ret_value, NULL))
	{
		g_stpcpy(data->passwd, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 4;

	if(nf_sysdb_get_key0("net.ddns.hostname", &ret_value, NULL))
	{
		g_stpcpy(data->host_name, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 5;

	if(nf_sysdb_get_key0("net.proto.ddnssvr_ip", &ret_value, NULL))
	{
		g_stpcpy(data->serverip, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 6;

	if(nf_sysdb_get_key0("net.ddns.p2p_enable", &ret_value, NULL))
	{
		data->p2p_enable = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 7;

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_set_ddns_data(DDNSData *data)
{
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_NET);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->enable);
	if(!nf_sysdb_set_key0("net.proto.ddnson", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->server);
	if(!nf_sysdb_set_key0("net.proto.ddnssvr", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->id);
	if(!nf_sysdb_set_key0("net.ddns.username", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->passwd);
	if(!nf_sysdb_set_key0("net.ddns.passwd", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->host_name);
	if(!nf_sysdb_set_key0("net.ddns.hostname", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->serverip);
	if(!nf_sysdb_set_key0("net.proto.ddnssvr_ip", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->p2p_enable);
	if(!nf_sysdb_set_key0("net.ddns.p2p_enable", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 7;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_NET, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_NET);

	DDEBUG_END
	return 0;
}

gboolean DAL_get_ddns_hostname(gchar *hostname)
{
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("net.ddns.hostname", &ret_value, NULL))
	{
		g_stpcpy(hostname, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);

		DDEBUG_FAIL
		return TRUE;
	}

	return FALSE;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
gint DAL_get_Unimo_Data(UNIMOData *data)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("net.proto.unimo.checker", &ret_value, NULL))
	{
		data->enable = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	if(nf_sysdb_get_key0("net.proto.unimo.server", &ret_value, NULL))
	{
		g_stpcpy(data->server, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 2;

	if(nf_sysdb_get_key0("net.proto.unimo.port", &ret_value, NULL))
	{
		data->port = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 3;

	return ret;
}
/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_set_Unimo_Data(UNIMOData *data)
{
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_NET);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->enable);
	if(!nf_sysdb_set_key0("net.proto.unimo.checker", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->server);
	if(!nf_sysdb_set_key0("net.proto.unimo.server", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->port);
	if(!nf_sysdb_set_key0("net.proto.unimo.port", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_NET, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_NET);

	DDEBUG_END
	return 0;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
gboolean DAL_is_ddns_on()
{
	GValue ret_value = {0,};
	gboolean ret = FALSE;

	DDEBUG_START

	if(nf_sysdb_get_key0("net.proto.ddnson", &ret_value, NULL)) {
		ret = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}

	DDEBUG_END
	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_email_data(EmailData *data)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("net.email.smtpsvr", &ret_value, NULL))
	{
		g_stpcpy(data[0].server, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret  = 1;

	if(nf_sysdb_get_key0("net.email.smtpport", &ret_value, NULL))
	{
		data[0].port = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret  = 2;

	if(nf_sysdb_get_key0("net.email.ssl", &ret_value, NULL))
	{
		data[0].security = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret  = 3;

	if(nf_sysdb_get_key0("net.email.id", &ret_value, NULL))
	{
		g_stpcpy(data[0].user, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret  = 4;

	if(nf_sysdb_get_key0("net.email.passwd", &ret_value, NULL))
	{
		g_stpcpy(data[0].passwd, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret  = 5;

	if(nf_sysdb_get_key0("net.email.testmail", &ret_value, NULL))
	{
		g_stpcpy(data[0].testMail, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret  = 6;

	if(nf_sysdb_get_key0("net.email.from", &ret_value, NULL))
	{
		g_stpcpy(data[0].from, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret  = 7;

	if(nf_sysdb_get_key0("net.email.individual", &ret_value, NULL))
	{
		data[0].individual = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret  = 8;

	if(nf_sysdb_get_key0("net.email_2nd.smtpsvr", &ret_value, NULL))
	{
		g_stpcpy(data[1].server, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret  = 11;

	if(nf_sysdb_get_key0("net.email_2nd.smtpport", &ret_value, NULL))
	{
		data[1].port = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret  = 12;

	if(nf_sysdb_get_key0("net.email_2nd.ssl", &ret_value, NULL))
	{
		data[1].security = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret  = 13;

	if(nf_sysdb_get_key0("net.email_2nd.id", &ret_value, NULL))
	{
		g_stpcpy(data[1].user, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret  = 14;

	if(nf_sysdb_get_key0("net.email_2nd.passwd", &ret_value, NULL))
	{
		g_stpcpy(data[1].passwd, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret  = 15;

	if(nf_sysdb_get_key0("net.email_2nd.testmail", &ret_value, NULL))
	{
		g_stpcpy(data[1].testMail, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret  = 16;

	if(nf_sysdb_get_key0("net.email_2nd.from", &ret_value, NULL))
	{
		g_stpcpy(data[1].from, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret  = 17;

	if(nf_sysdb_get_key0("net.email_2nd.individual", &ret_value, NULL))
	{
		data[1].individual = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret  = 18;

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_set_email_data(EmailData *data)
{
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_NET);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data[0].server);
	if(!nf_sysdb_set_key0("net.email.smtpsvr", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data[0].port);
	if(!nf_sysdb_set_key0("net.email.smtpport", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data[0].security);
	if(!nf_sysdb_set_key0("net.email.ssl", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data[0].user);
	if(!nf_sysdb_set_key0("net.email.id", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data[0].passwd);
	if(!nf_sysdb_set_key0("net.email.passwd", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data[0].testMail);
	if(!nf_sysdb_set_key0("net.email.testmail", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data[0].from);
	if(!nf_sysdb_set_key0("net.email.from", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 7;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data[0].individual);
	if(!nf_sysdb_set_key0("net.email.individual", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 8;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data[1].server);
	if(!nf_sysdb_set_key0("net.email_2nd.smtpsvr", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 11;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data[1].port);
	if(!nf_sysdb_set_key0("net.email_2nd.smtpport", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 12;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data[1].security);
	if(!nf_sysdb_set_key0("net.email_2nd.ssl", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 13;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data[1].user);
	if(!nf_sysdb_set_key0("net.email_2nd.id", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 14;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data[1].passwd);
	if(!nf_sysdb_set_key0("net.email_2nd.passwd", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 15;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data[1].testMail);
	if(!nf_sysdb_set_key0("net.email_2nd.testmail", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 16;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data[1].from);
	if(!nf_sysdb_set_key0("net.email_2nd.from", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 17;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data[1].individual);
	if(!nf_sysdb_set_key0("net.email_2nd.individual", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 18;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_NET, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_NET);

	DDEBUG_END
	return 0;
}

guint DAL_get_encrypt_data(EncryptData *data)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("net.proto.srtspon", &ret_value, NULL))
	{
		data->srtspon = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	if(nf_sysdb_get_key0("net.proto.srtsp_method", &ret_value, NULL))
	{
		g_stpcpy(data->srtsp_method, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 2;

	if(nf_sysdb_get_key0("net.proto.srtsp_level", &ret_value, NULL))
	{
		data->srtsp_level = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret  = 3;

	if(nf_sysdb_get_key0("net.proto.httpson", &ret_value, NULL))
	{
		data->httpson = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 4;

	if(nf_sysdb_get_key0("net.proto.webport_2nd", &ret_value, NULL))
	{
		data->webport_2nd = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 5;

	if(nf_sysdb_get_key0("net.proto.httpauth_method", &ret_value, NULL))
	{
		data->httpauth_method = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 6;

		return ret;
}

guint DAL_set_encrypt_data(EncryptData *data)
{
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_NET);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->srtspon);
	if(!nf_sysdb_set_key0("net.proto.srtspon", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->srtsp_method);
	if(!nf_sysdb_set_key0("net.proto.srtsp_method", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->srtsp_level);
	if(!nf_sysdb_set_key0("net.proto.srtsp_level", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->httpson);
	if(!nf_sysdb_set_key0("net.proto.httpson", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->webport_2nd);
	if(!nf_sysdb_set_key0("net.proto.webport_2nd", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->httpauth_method);
	if(!nf_sysdb_set_key0("net.proto.httpauth_method", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_NET, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_NET);

	DDEBUG_END
	return 0;
}

gboolean DAL_get_httpson()
{
	gboolean ret = FALSE;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("net.proto.httpson", &ret_value, NULL))
	{
		ret = g_value_get_boolean(&ret_value);
	}
	
	g_value_unset(&ret_value);

	return ret;
}

static guint prvGetIPFilterListData(IPFilterListData *data, guint idx)
{
	guint ret = 0;
	GValue ret_value = {0,};
	guint tmp;

	if(nf_sysdb_get_key1("net.filter.rule.R%d.type", idx, &ret_value, NULL))
	{
		data->type = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 3;

	if(nf_sysdb_get_key1("net.filter.rule.R%d.addr", idx, &ret_value, NULL))
	{
		tmp = g_value_get_uint(&ret_value);
		prvIntToIP(data->listAddr, tmp);

		g_value_unset(&ret_value);
	}
	else		ret = 4;

	return ret;
}

guint DAL_get_ipfilter_data(IPFilterData *data)
{
	guint ret = 0;
	GValue ret_value = {0,};
	guint i;

	if(nf_sysdb_get_key0("net.filter.enable", &ret_value, NULL))
	{
		data->enable = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	if(nf_sysdb_get_key0("net.filter.opmode", &ret_value, NULL))
	{
		data->opmode = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 2;


	for(i = 0; i < IPFILTER_RULE_MAX; i++)
		prvGetIPFilterListData(&(data->filter_list[i]), i);

	return ret;
}

static guint prvSetIPFilterListData(IPFilterListData *data, guint idx)
{
	guint set = 0;
	guint temp;
	GValue set_value = {0,};

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->type);

	if(!nf_sysdb_set_key1("net.filter.rule.R%d.type", idx, &set_value, NULL))

	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	temp = prvIPToInt(data->listAddr);
	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, temp);

	if(!nf_sysdb_set_key1("net.filter.rule.R%d.addr", idx, &set_value, NULL))

	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	DDEBUG_END
	return set;
}

guint DAL_set_ipfilter_data(IPFilterData *data)
{
	guint i;
	GValue set_value = {0,};
	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_NET);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->enable);
	if(!nf_sysdb_set_key0("net.filter.enable", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->opmode);
	if(!nf_sysdb_set_key0("net.filter.opmode", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	for(i=0; i<IPFILTER_RULE_MAX; i++)
	{
		ret = prvSetIPFilterListData(&(data->filter_list[i]), i);

		if(ret)	break;
	}

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_NET, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_NET);

	DDEBUG_END
	return 0;
}

guint DAL_get_ipfilter_count()
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("net.filter.rule.RCNT", &ret_value, NULL))
	{
		ret = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}

	return ret;
}

guint DAL_set_ipfilter_count(guint cnt)
{
	guint ret = 0;
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_NET);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, cnt);
	if(!nf_sysdb_set_key0("net.filter.rule.RCNT", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_NET, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_NET);

	DDEBUG_END
	return ret;
}

gboolean DAL_get_SnmpData(SnmpData *data)
{
    guint ret = 0;
	GValue ret_value = {0,};
	gchar buf[32];
	guint i;

	memset(buf, 0x00, sizeof(buf));

	if(nf_sysdb_get_key0("net.snmp.version", &ret_value, NULL))
	{
		data->snmp_mode = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	if(nf_sysdb_get_key0("net.snmp.v2.communitystring", &ret_value, NULL))
	{
        strncpy(data->v2c_cmt_str, g_value_get_string(&ret_value), sizeof(data->v2c_cmt_str));
        g_value_unset(&ret_value);
	}
	else		ret = 2;

	if(nf_sysdb_get_key0("net.snmp.v2.trap_conf.address", &ret_value, NULL))
	{
        strncpy(data->v2c_address_tr, g_value_get_string(&ret_value), sizeof(data->v2c_address_tr));
		g_value_unset(&ret_value);
	}
	else		ret = 3;

	if(nf_sysdb_get_key0("net.snmp.v2.trap_conf.communitystring", &ret_value, NULL))
	{
        strncpy(data->v2c_cmt_str_tr, g_value_get_string(&ret_value), sizeof(data->v2c_cmt_str_tr));
		g_value_unset(&ret_value);
	}
	else		ret = 4;

	if(nf_sysdb_get_key0("net.snmp.v3.engineid", &ret_value, NULL))
	{
        strncpy(data->v3_engine_id, g_value_get_string(&ret_value), sizeof(data->v3_engine_id));
		g_value_unset(&ret_value);
	}
	else		ret = 5;

	if(nf_sysdb_get_key0("net.snmp.v3.userid", &ret_value, NULL))
	{
        strncpy(data->v3_userid, g_value_get_string(&ret_value), sizeof(data->v3_userid));
		g_value_unset(&ret_value);
	}
	else		ret = 6;

	if(nf_sysdb_get_key0("net.snmp.v3.user_auth", &ret_value, NULL))
	{
        strncpy(buf, g_value_get_string(&ret_value), sizeof(buf));

        if(!strcmp(buf,"NONE" ))     data->v3_auth_combo = 0;
        else if(!strcmp(buf, "MD5")) data->v3_auth_combo = 1;
        else if(!strcmp(buf, "SHA")) data->v3_auth_combo = 2;

		g_value_unset(&ret_value);
	}
	else		ret = 7;

	if(nf_sysdb_get_key0("net.snmp.v3.user_auth_key", &ret_value, NULL))
	{
        strncpy(data->v3_auth_key, g_value_get_string(&ret_value), sizeof(data->v3_auth_key));
		g_value_unset(&ret_value);
	}
	else		ret = 8;

	memset(buf, 0x00, sizeof(buf));

	if(nf_sysdb_get_key0("net.snmp.v3.user_priv", &ret_value, NULL))
	{
        strncpy(buf, g_value_get_string(&ret_value), sizeof(buf));

        if(!strcmp(buf,"NONE"))     data->v3_priv_combo =0;
        else if(!strcmp(buf,"DES")) data->v3_priv_combo =1;
        else if(!strcmp(buf,"AES")) data->v3_priv_combo =2;

		g_value_unset(&ret_value);
	}
	else		ret = 9;

	if(nf_sysdb_get_key0("net.snmp.v3.user_priv_key", &ret_value, NULL))
	{
        strncpy(data->v3_priv_key, g_value_get_string(&ret_value), sizeof(data->v3_priv_key));
		g_value_unset(&ret_value);
	}
	else		ret = 10;

	if(nf_sysdb_get_key0("net.snmp.v3.receiver_address", &ret_value, NULL))
	{
        strncpy(data->v3_address_tr, g_value_get_string(&ret_value), sizeof(data->v3_address_tr));
		g_value_unset(&ret_value);
	}
	else		ret = 11;

	return ret;
}


gboolean DAL_set_SnmpData(SnmpData *data)
{
    GValue set_value = {0,};
    gchar buf[32];

    memset(buf, 0x00, sizeof(buf));

    DDEBUG_START

    nf_sysdb_lock(NF_SYSDB_CATE_NET);

    g_value_init(&set_value, G_TYPE_UINT);
    g_value_set_uint(&set_value, data->snmp_mode);
    if(!nf_sysdb_set_key0("net.snmp.version", &set_value, NULL))
    {
        nf_sysdb_unlock(NF_SYSDB_CATE_NET);
        DDEBUG_FAIL
        return 1;
    }
    g_value_unset(&set_value);


    g_value_init(&set_value, G_TYPE_STRING);
    g_value_set_string(&set_value, data->v2c_cmt_str);
    if(!nf_sysdb_set_key0("net.snmp.v2.communitystring", &set_value, NULL))
    {
        nf_sysdb_unlock(NF_SYSDB_CATE_NET);
        DDEBUG_FAIL
        return 2;
    }
    g_value_unset(&set_value);

    g_value_init(&set_value, G_TYPE_STRING);
    g_value_set_string(&set_value, data->v2c_address_tr);
    if(!nf_sysdb_set_key0("net.snmp.v2.trap_conf.address", &set_value, NULL))
    {
        nf_sysdb_unlock(NF_SYSDB_CATE_NET);
        DDEBUG_FAIL
        return 3;
    }
    g_value_unset(&set_value);

    g_value_init(&set_value, G_TYPE_STRING);
    g_value_set_string(&set_value, data->v2c_cmt_str_tr);
    if(!nf_sysdb_set_key0("net.snmp.v2.trap_conf.communitystring", &set_value, NULL))
    {
        nf_sysdb_unlock(NF_SYSDB_CATE_NET);
        DDEBUG_FAIL
        return 4;
    }
    g_value_unset(&set_value);

    g_value_init(&set_value, G_TYPE_STRING);
    g_value_set_string(&set_value, data->v3_engine_id);
    if(!nf_sysdb_set_key0("net.snmp.v3.engineid", &set_value, NULL))
    {
        nf_sysdb_unlock(NF_SYSDB_CATE_NET);
        DDEBUG_FAIL
        return 5;
    }
    g_value_unset(&set_value);

    g_value_init(&set_value, G_TYPE_STRING);
    g_value_set_string(&set_value, data->v3_userid);
    if(!nf_sysdb_set_key0("net.snmp.v3.userid", &set_value, NULL))
    {
        nf_sysdb_unlock(NF_SYSDB_CATE_NET);
        DDEBUG_FAIL
        return 6;
    }
    g_value_unset(&set_value);

    g_value_init(&set_value, G_TYPE_STRING);
    switch(data->v3_auth_combo)
    {
        case 0:  strcpy(buf,"NONE"); break;
        case 1:  strcpy(buf,"MD5"); break;
        case 2:  strcpy(buf,"SHA"); break;
    }
    g_value_set_string(&set_value, buf);
    if(!nf_sysdb_set_key0("net.snmp.v3.user_auth", &set_value, NULL))
    {
        nf_sysdb_unlock(NF_SYSDB_CATE_NET);
        DDEBUG_FAIL
        return 7;
    }
    g_value_unset(&set_value);

    g_value_init(&set_value, G_TYPE_STRING);
    g_value_set_string(&set_value, data->v3_auth_key);
    if(!nf_sysdb_set_key0("net.snmp.v3.user_auth_key", &set_value, NULL))
    {
        nf_sysdb_unlock(NF_SYSDB_CATE_NET);
        DDEBUG_FAIL
        return 8;
    }
    g_value_unset(&set_value);


    memset(buf, 0x00, sizeof(buf));

    g_value_init(&set_value, G_TYPE_STRING);
    switch(data->v3_priv_combo)
    {
        case 0:     strcpy(buf,"NONE"); break;
        case 1:     strcpy(buf,"DES");  break;
        case 2:     strcpy(buf,"AES");  break;
    }
    g_value_set_string(&set_value, buf);
    if(!nf_sysdb_set_key0("net.snmp.v3.user_priv", &set_value, NULL))
    {
        nf_sysdb_unlock(NF_SYSDB_CATE_NET);
        DDEBUG_FAIL
        return 9;
    }
    g_value_unset(&set_value);

    g_value_init(&set_value, G_TYPE_STRING);
    g_value_set_string(&set_value, data->v3_priv_key);
    if(!nf_sysdb_set_key0("net.snmp.v3.user_priv_key", &set_value, NULL))
    {
        nf_sysdb_unlock(NF_SYSDB_CATE_NET);
        DDEBUG_FAIL
        return 10;
    }
    g_value_unset(&set_value);

    g_value_init(&set_value, G_TYPE_STRING);
    g_value_set_string(&set_value, data->v3_address_tr);
    if(!nf_sysdb_set_key0("net.snmp.v3.receiver_address", &set_value, NULL))
    {
        nf_sysdb_unlock(NF_SYSDB_CATE_NET);
        DDEBUG_FAIL
        return 11;
    }
    g_value_unset(&set_value);


    nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_NET, 0,0,0);

    nf_sysdb_unlock(NF_SYSDB_CATE_NET);

    DDEBUG_END

    return FALSE;
}





#ifdef __SUPPORT_BS8418__
/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_ftp_data(FtpData *data)
{
	GValue ret_value = {0,};
	guint ret = 0;
	gint i;
	gchar buf[32];


	for(i=0; i<FTP_SERVER_COUNT; i++)
	{
		sprintf(buf, "net.ftp.F%d.server", i);
		if(nf_sysdb_get_key0(buf, &ret_value, NULL))
		{
			strncpy(data->server[i], g_value_get_string(&ret_value), sizeof(data->server[i]));
			g_value_unset(&ret_value);
		}
		else		ret  = 1 + (i * 4);

		sprintf(buf, "net.ftp.F%d.username", i);
		if(nf_sysdb_get_key0(buf, &ret_value, NULL))
		{
			strncpy(data->user[i], g_value_get_string(&ret_value), sizeof(data->user[i]));
			g_value_unset(&ret_value);
		}
		else		ret  = 2 + (i * 4);

		sprintf(buf, "net.ftp.F%d.passwd", i);
		if(nf_sysdb_get_key0(buf, &ret_value, NULL))
		{
			strncpy(data->passwd[i], g_value_get_string(&ret_value), sizeof(data->passwd[i]));
			g_value_unset(&ret_value);
		}
		else		ret  = 3 + (i * 4);

		sprintf(buf, "net.ftp.F%d.path", i);
		if(nf_sysdb_get_key0(buf, &ret_value, NULL))
		{
			strncpy(data->path[i], g_value_get_string(&ret_value), sizeof(data->path[i]));
			g_value_unset(&ret_value);
		}
		else		ret  = 4 + (i * 4);
	}

	return 0;
}


/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_set_ftp_data(FtpData *data)
{
	GValue set_value = {0,};
	gint i;
	gchar buf[32];

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_NET);

	for(i=0; i<FTP_SERVER_COUNT; i++)
	{
		sprintf(buf, "net.ftp.F%d.server", i);

		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, data->server[i]);
		printf("DAL: %s \n", data->server[i]);
		if(!nf_sysdb_set_key0(buf, &set_value, NULL))
		{
			nf_sysdb_unlock(NF_SYSDB_CATE_NET);
			DDEBUG_FAIL
				return 1 + (i * 4);
		}
		g_value_unset(&set_value);


		sprintf(buf, "net.ftp.F%d.username", i);

		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, data->user[i]);
		if(!nf_sysdb_set_key0(buf, &set_value, NULL))
		{
			nf_sysdb_unlock(NF_SYSDB_CATE_NET);
			DDEBUG_FAIL
				return 2 + (i * 4);
		}
		g_value_unset(&set_value);


		sprintf(buf, "net.ftp.F%d.passwd", i);

		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, data->passwd[i]);
		if(!nf_sysdb_set_key0(buf, &set_value, NULL))
		{
			nf_sysdb_unlock(NF_SYSDB_CATE_NET);
			DDEBUG_FAIL
				return 3 + (i * 4);
		}
		g_value_unset(&set_value);


		sprintf(buf, "net.ftp.F%d.path", i);

		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, data->path[i]);
		if(!nf_sysdb_set_key0(buf, &set_value, NULL))
		{
			nf_sysdb_unlock(NF_SYSDB_CATE_NET);
			DDEBUG_FAIL
				return 4 + (i * 4);
		}
		g_value_unset(&set_value);
	}

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_NET, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_NET);

	DDEBUG_END

	return 0;
}



/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_modem_data(ModemData *data)
{
	GValue ret_value = {0,};
	guint ret = 0;

	if(nf_sysdb_get_key0("net.modem.model", &ret_value, NULL))
	{
		strncpy(data->model, g_value_get_string(&ret_value), sizeof(data->model));
		g_value_unset(&ret_value);
	}
	else		ret  = 1;

	if(nf_sysdb_get_key0("net.modem.service", &ret_value, NULL))
	{
		strncpy(data->vendor, g_value_get_string(&ret_value), sizeof(data->vendor));
		g_value_unset(&ret_value);
	}
	else		ret  = 2;

	if(nf_sysdb_get_key0("net.modem.username", &ret_value, NULL))
	{
		strncpy(data->user, g_value_get_string(&ret_value), sizeof(data->user));
		g_value_unset(&ret_value);
	}
	else		ret  = 3;

	if(nf_sysdb_get_key0("net.modem.passwd", &ret_value, NULL))
	{
		strncpy(data->passwd, g_value_get_string(&ret_value), sizeof(data->passwd));
		g_value_unset(&ret_value);
	}
	else		ret  = 4;

	if(nf_sysdb_get_key0("net.modem.retry", &ret_value, NULL))
	{
		data->retry = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret  = 5;

	return ret;
}


/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_set_modem_data(ModemData *data)
{
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_NET);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->model);
	if(!nf_sysdb_set_key0("net.modem.model", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
			return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->vendor);
	if(!nf_sysdb_set_key0("net.modem.service", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
			return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->user);
	if(!nf_sysdb_set_key0("net.modem.username", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
			return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->passwd);
	if(!nf_sysdb_set_key0("net.modem.passwd", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
			return 4;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->retry);
	if(!nf_sysdb_set_key0("net.modem.retry", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_NET, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_NET);

	DDEBUG_END

	return 0;
}
#endif

guint DAL_get_rtp_data(RtpData *data)
{
	guint ret = 0;
	GValue ret_value = {0,};

    DDEBUG_START

	if(nf_sysdb_get_key0("net.rtp.rtpsport", &ret_value, NULL))
	{
		data->rtpsport = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	if(nf_sysdb_get_key0("net.rtp.rtpeport", &ret_value, NULL))
	{
		data->rtpeport = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 2;

	if(nf_sysdb_get_key0("net.rtp.audio_backch_mode", &ret_value, NULL))
	{
		data->audio_backch_mode = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 3;

	if(nf_sysdb_get_key0("net.rtp.audio_backch_port", &ret_value, NULL))
	{
		data->audio_backch_port = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 4;
	
	DDEBUG_END

	return ret;
}

guint DAL_set_rtp_data(RtpData *data)
{
	guint ret = 0;
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_NET);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->rtpsport);
	if(!nf_sysdb_set_key0("net.rtp.rtpsport", &set_value, NULL))
	{
        nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->rtpeport);
	if(!nf_sysdb_set_key0("net.rtp.rtpeport", &set_value, NULL))
	{
        nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->audio_backch_mode);
	if(!nf_sysdb_set_key0("net.rtp.audio_backch_mode", &set_value, NULL))
	{
        nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->audio_backch_port);
	if(!nf_sysdb_set_key0("net.rtp.audio_backch_port", &set_value, NULL))
	{
        nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

    nf_sysdb_unlock(NF_SYSDB_CATE_NET);

	DDEBUG_END

	return ret;

}

guint DAL_get_multicast_data(MulticastData *data, gint stream_cnt, gint ch_cnt)
{
    GValue ret_value = {0,};
    guint tmp;
    guint ret = 0;
    gint i, j;


    if (nf_sysdb_get_key0("net.rtp.multi.ttl", &ret_value, NULL))
    {
        data->ttl = g_value_get_uint(&ret_value);
        g_value_unset(&ret_value);
    }
    else        ret = 1;

    for (i = 0; i < stream_cnt; i++)
    {
        for (j = 0; j < ch_cnt; j++)
        {
        	if(nf_sysdb_get_key2("net.rtp.multi.S%d.C%d.vaddr", i, j, &ret_value, NULL))
        	{
        	    tmp = g_value_get_uint(&ret_value);
                prvIntToIP(data->list[i][j].stream_ip, tmp);
                g_value_unset(&ret_value);
            }
        	else		ret = 2;

        	if(nf_sysdb_get_key2("net.rtp.multi.S%d.C%d.vport", i, j, &ret_value, NULL))
        	{
        	    data->list[i][j].video_port = g_value_get_uint(&ret_value);
                g_value_unset(&ret_value);
            }
        	else		ret = 3;

        	if(nf_sysdb_get_key2("net.rtp.multi.S%d.C%d.aport", i, j, &ret_value, NULL))
        	{
        	    data->list[i][j].audio_port = g_value_get_uint(&ret_value);
                g_value_unset(&ret_value);
            }
        	else		ret = 4;
    	}
	}

	return ret;
}

guint DAL_set_multicast_data(MulticastData *data, gint stream_cnt, gint ch_cnt)
{
    GValue set_value = {0,};
    guint ret = 0;
    guint tmp = 0;
    gint i, j;

    DDEBUG_START

    nf_sysdb_lock(NF_SYSDB_CATE_NET);

    g_value_init(&set_value, G_TYPE_UINT);
    g_value_set_uint(&set_value, data->ttl);
    if (!nf_sysdb_set_key0("net.rtp.multi.ttl", &set_value, NULL))
    {
        nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 1;
    }
    g_value_unset(&set_value);

    for (i = 0; i < stream_cnt; i++)
    {
        for (j = 0; j < ch_cnt; j++)
        {
            tmp = prvIPToInt(data->list[i][j].stream_ip);
            g_value_init(&set_value, G_TYPE_UINT);
            g_value_set_uint(&set_value, tmp);
            if (!nf_sysdb_set_key2("net.rtp.multi.S%d.C%d.vaddr", i, j, &set_value, NULL))
            {
                nf_sysdb_unlock(NF_SYSDB_CATE_NET);
        		DDEBUG_FAIL
        		return 2;
            }
            g_value_unset(&set_value);

            g_value_init(&set_value, G_TYPE_UINT);
            g_value_set_uint(&set_value, data->list[i][j].video_port);
            if (!nf_sysdb_set_key2("net.rtp.multi.S%d.C%d.vport", i, j, &set_value, NULL))
            {
                nf_sysdb_unlock(NF_SYSDB_CATE_NET);
        		DDEBUG_FAIL
        		return 3;
            }
            g_value_unset(&set_value);

            g_value_init(&set_value, G_TYPE_UINT);
            g_value_set_uint(&set_value, data->list[i][j].audio_port);
            if (!nf_sysdb_set_key2("net.rtp.multi.S%d.C%d.aport", i, j, &set_value, NULL))
            {
                nf_sysdb_unlock(NF_SYSDB_CATE_NET);
        		DDEBUG_FAIL
        		return 4;
            }
            g_value_unset(&set_value);
        }
    }

    nf_sysdb_unlock(NF_SYSDB_CATE_NET);

	DDEBUG_END

    return ret;
}

guint DAL_get_8021x_data(IEEE8021xData *data)
{
	guint ret = 0;
	GValue ret_value = {0,};

    DDEBUG_START

	if(nf_sysdb_get_key0("net.8021x.use", &ret_value, NULL))
	{
		data->use = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	if(nf_sysdb_get_key0("net.8021x.eap_type", &ret_value, NULL))
	{
		data->eap_type = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 2;

	if(nf_sysdb_get_key0("net.8021x.eapol_ver", &ret_value, NULL))
	{
		data->eapol_ver = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 3;

	if(nf_sysdb_get_key0("net.8021x.id", &ret_value, NULL))
	{
		g_stpcpy(data->id, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 4;

	if(nf_sysdb_get_key0("net.8021x.pw", &ret_value, NULL))
	{
		g_stpcpy(data->pw, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 5;

	if(nf_sysdb_get_key0("net.8021x.anonymous", &ret_value, NULL))
	{
		g_stpcpy(data->anonymous, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 6;

	if(nf_sysdb_get_key0("net.8021x.inner_auth", &ret_value, NULL))
	{
		g_stpcpy(data->inner_auth, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 7;

	if(nf_sysdb_get_key0("net.8021x.prikey_pw", &ret_value, NULL))
	{
		g_stpcpy(data->prikey_pw, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 8;

	DDEBUG_END

	return ret;
}

guint DAL_set_8021x_data(IEEE8021xData *data)
{
	guint ret = 0;
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_NET);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->use);
	if(!nf_sysdb_set_key0("net.8021x.use", &set_value, NULL))
	{
        nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->eap_type);
	if(!nf_sysdb_set_key0("net.8021x.eap_type", &set_value, NULL))
	{
        nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->eapol_ver);
	if(!nf_sysdb_set_key0("net.8021x.eapol_ver", &set_value, NULL))
	{
        nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->id);
	if(!nf_sysdb_set_key0("net.8021x.id", &set_value, NULL))
	{
        nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->pw);
	if(!nf_sysdb_set_key0("net.8021x.pw", &set_value, NULL))
	{
        nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->anonymous);
	if(!nf_sysdb_set_key0("net.8021x.anonymous", &set_value, NULL))
	{
        nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->inner_auth);
	if(!nf_sysdb_set_key0("net.8021x.inner_auth", &set_value, NULL))
	{
        nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 7;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->prikey_pw);
	if(!nf_sysdb_set_key0("net.8021x.prikey_pw", &set_value, NULL))
	{
        nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 8;
	}
	g_value_unset(&set_value);
	
    nf_sysdb_unlock(NF_SYSDB_CATE_NET);

	DDEBUG_END

	return ret;
}

/*********************************************************************************
 * Setup --> System --> Event / Sensor
 * *******************************************************************************/

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_hddEvt_data(HDDData *data)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("alarm.strg.smart_warn", &ret_value, NULL))
	{
		data->smart = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 2;

	if(nf_sysdb_get_key0("alarm.strg.smart_chk", &ret_value, NULL))
	{
		data->interval = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 3;

	if(nf_sysdb_get_key0("alarm.strg.dskfull", &ret_value, NULL))
	{
		data->diskFull = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 4;

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_smart_interval()
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("alarm.strg.smart_chk", &ret_value, NULL))
	{
		ret = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		ret = 25;

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_set_hddEvt_data(HDDData *data)
{
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_ALARM);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->smart);
	if(!nf_sysdb_set_key0("alarm.strg.smart_warn", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ALARM);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->interval);
	if(!nf_sysdb_set_key0("alarm.strg.smart_chk", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ALARM);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->diskFull);
	if(!nf_sysdb_set_key0("alarm.strg.dskfull", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ALARM);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ALARM, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_ALARM);

	DDEBUG_END
	return 0;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_set_smart_interval(guint interval)
{
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_ALARM);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, interval);
	if(!nf_sysdb_set_key0("alarm.strg.smart_chk", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ALARM);
		DDEBUG_FAIL
		return 0;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ALARM, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_ALARM);

	DDEBUG_END
	return 1;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_alarmInput_data(AlarmInputData *data, guint idx)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key1("alarm.sensor.S%d.desc", idx, &ret_value, NULL))
	{
		g_stpcpy(data->desc, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 1;

#ifdef __SUPPORT_BS8418__
	if(nf_sysdb_get_key1("alarm.sensor.S%d.act", idx, &ret_value, NULL))
	{
		data->oper = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 2;
#else
	if(nf_sysdb_get_key1("alarm.sensor.S%d.act", idx, &ret_value, NULL))
	{
		data->oper = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 2;
#endif

	if(nf_sysdb_get_key1("alarm.sensor.S%d.oper", idx, &ret_value, NULL))
	{
		data->type = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 3;
#ifdef __SUPPORT_BS8418__
	if(nf_sysdb_get_key1("alarm.sensor.S%d.delay", idx, &ret_value, NULL))
	{
		data->delay = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 4;


	if(nf_sysdb_get_key1("alarm.sensor.S%d.false_cnt", idx, &ret_value, NULL))
	{
		data->false_cnt = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 5;
#endif
	return ret;
}

gboolean DAL_get_alarmInput_desc(guint ch_num, gchar desc[STRING_SIZE_64])
{
	GValue ret_value = {0,};

	if(nf_sysdb_get_key1("alarm.sensor.S%d.desc", ch_num, &ret_value, NULL))
	{
		g_stpcpy(desc, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	return TRUE;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
static guint prvSetAlarmInputData(AlarmInputData data, guint idx)
{
	GValue set_value = {0,};

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.desc);
	if(!nf_sysdb_set_key1("alarm.sensor.S%d.desc", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

#ifdef __SUPPORT_BS8418__
	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.oper);
	if(!nf_sysdb_set_key1("alarm.sensor.S%d.act", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);
#else
	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.oper);
	if(!nf_sysdb_set_key1("alarm.sensor.S%d.act", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);
#endif

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.type);
	if(!nf_sysdb_set_key1("alarm.sensor.S%d.oper", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);
#ifdef __SUPPORT_BS8418__
	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.delay);
	if(!nf_sysdb_set_key1("alarm.sensor.S%d.delay", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.false_cnt);
	if(!nf_sysdb_set_key1("alarm.sensor.S%d.false_cnt", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);
#endif
	return 0;
}

guint DAL_set_alarmInput_data(AlarmInputData data, guint idx)
{
	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_ALARM);

	ret = prvSetAlarmInputData(data, idx);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ALARM, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_ALARM);
	DDEBUG_END
	return ret;
}

guint DAL_set_alarmInput_data_all(AlarmInputData *data, guint ch_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_ALARM);

	for(i=0; i<ch_num; i++)
	{
		ret = prvSetAlarmInputData(data[i], i);
		if(ret)	break;
	}

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ALARM, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_ALARM);

	DDEBUG_END
	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_alarmOut_data(AlarmOutData *data, guint channel)
{
	guint ret = 0;
	GValue ret_value = {0,};
	gchar *temp;
	guint i;

#if defined(_ANF_1648) || defined(_ANF_0824) || defined(_SNF_MODEL)
	if(channel >= g_ch) {
		channel -= g_ch;
		channel += 16;
	}
#endif

	if(nf_sysdb_get_key1("act.relay.R%d.dwell_type", channel, &ret_value, NULL))//bug fix
	{
		data->mode = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	if(nf_sysdb_get_key1("act.relay.R%d.op_type", channel, &ret_value, NULL))//bug fix
	{
		data->type = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 2;

#ifdef __SUPPORT_BS8418__
	if(nf_sysdb_get_key1("act.relay.R%d.act", channel, &ret_value, NULL))
	{
		data->oper = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 3;
#else
	if(nf_sysdb_get_key1("act.relay.R%d.act", channel, &ret_value, NULL))
	{
		data->oper = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 3;
#endif

	if(nf_sysdb_get_key1("act.relay.R%d.dwell_time", channel, &ret_value, NULL))
	{
		data->duration = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 4;

	if(nf_sysdb_get_key1("act.relay.R%d.hdd_event", channel, &ret_value, NULL))
	{
		data->hddEvt = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 5;
#ifdef __SUPPORT_BS8418__
	if(nf_sysdb_get_key1("act.relay.R%d.ups_event", channel, &ret_value, NULL))
	{
		data->upsEvt = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 6;
#endif
	if(nf_sysdb_get_key1("act.relay.R%d.alarm", channel, &ret_value, NULL))
	{
		temp = g_value_get_string(&ret_value);

#ifdef DB_ALARM_IN_4CH_SUPPORT
		for(i=0; i<16; i++)
		{
			if(i<4)
			{
				if(temp[i] == '0')	data->alarm[i] = 0;
				else				data->alarm[i] = 1;
			}
			else
			{
				data->alarm[i] = 0;
			}
		}
#else
		for(i=0; i<16; i++)
		{
			if(temp[i] == '0')	data->alarm[i] = 0;
			else				data->alarm[i] = 1;
		}
#endif
		g_value_unset(&ret_value);
	}
	else		ret = 7;

	if(nf_sysdb_get_key1("act.relay.R%d.vloss", channel, &ret_value, NULL))
	{
		temp = g_value_get_string(&ret_value);

		for(i=0; i<16; i++)
		{
			if(temp[i] == '0')	data->videoLoss[i] = 0;
			else				data->videoLoss[i] = 1;
		}

		g_value_unset(&ret_value);
	}
	else		ret = 8;

	if(nf_sysdb_get_key1("act.relay.R%d.motion", channel, &ret_value, NULL))
	{
		temp = g_value_get_string(&ret_value);

		for(i=0; i<16; i++)
		{
			if(temp[i] == '0')	data->motion[i] = 0;
			else				data->motion[i] = 1;
		}
		g_value_unset(&ret_value);
	}
	else		ret = 9;
#ifdef __SUPPORT_BS8418__
	if(nf_sysdb_get_key1("act.relay.R%d.lcont", channel, &ret_value, NULL))
	{
		temp = g_value_get_string(&ret_value);

		for(i=0; i<16; i++)
		{
			if(temp[i] == '0')	data->lowContr[i] = 0;
			else				data->lowContr[i] = 1;
		}
		g_value_unset(&ret_value);
	}
	else		ret = 10;
#endif
	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
static guint prvSetAlarmOutData(AlarmOutData data, guint channel)
{
	GValue set_value = {0,};
	gchar temp[MAX_TEMP_STR_SIZE];
	guint i;

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.mode);
	if(!nf_sysdb_set_key1("act.relay.R%d.dwell_type", channel, &set_value, NULL)) //bug fix
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.type);
	if(!nf_sysdb_set_key1("act.relay.R%d.op_type", channel, &set_value, NULL)) //bug fix
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

#ifdef __SUPPORT_BS8418__
	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.oper);
	if(!nf_sysdb_set_key1("act.relay.R%d.act", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);
#else
	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.oper);
	if(!nf_sysdb_set_key1("act.relay.R%d.act", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);
#endif

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.duration);
	if(!nf_sysdb_set_key1("act.relay.R%d.dwell_time", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.hddEvt);
	if(!nf_sysdb_set_key1("act.relay.R%d.hdd_event", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);
#ifdef __SUPPORT_BS8418__
	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.upsEvt);
	if(!nf_sysdb_set_key1("act.relay.R%d.ups_event", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);
#endif
	memset(temp, 0, sizeof(gchar)*MAX_TEMP_STR_SIZE);
#ifdef DB_ALARM_IN_4CH_SUPPORT
	for(i=0; i<16; i++)
	{
		if(i < 4)
		{
			if(data.alarm[i] == 0)	temp[i] = '0';
			else					temp[i] = '1';
		}
		else
		{
			temp[i] = '0';
		}
	}
#else
	for(i=0; i<16; i++)
	{
		if(data.alarm[i] == 0)	temp[i] = '0';
		else					temp[i] = '1';
	}
#endif
	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, temp);
	if(!nf_sysdb_set_key1("act.relay.R%d.alarm", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
			return 7;
	}
	g_value_unset(&set_value);

	memset(temp, 0, sizeof(gchar)*MAX_TEMP_STR_SIZE);
	for(i=0; i<16; i++)
	{
		if(data.videoLoss[i] == 0)	temp[i] = '0';
		else						temp[i] = '1';
	}

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, temp);
	if(!nf_sysdb_set_key1("act.relay.R%d.vloss", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 8;
	}
	g_value_unset(&set_value);

	memset(temp, 0, sizeof(gchar)*MAX_TEMP_STR_SIZE);
	for(i=0; i<16; i++)
	{
		if(data.motion[i] == 0)	temp[i] = '0';
		else					temp[i] = '1';
	}

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, temp);
	if(!nf_sysdb_set_key1("act.relay.R%d.motion", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 9;
	}
	g_value_unset(&set_value);
#ifdef __SUPPORT_BS8418__
	memset(temp, 0, sizeof(gchar)*MAX_TEMP_STR_SIZE);
	for(i=0; i<16; i++)
	{
		if(data.lowContr[i] == 0)	temp[i] = '0';
		else						temp[i] = '1';
	}

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, temp);
	if(!nf_sysdb_set_key1("act.relay.R%d.lcont", channel, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 10;
	}
	g_value_unset(&set_value);
#endif
	return 0;
}

guint DAL_set_alarmOut_data(AlarmOutData data, guint channel)
{
	guint ret = 0;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

#if defined(_ANF_1648) || defined(_ANF_0824) || defined(_SNF_MODEL)
	if(channel >= g_ch) {
		channel -= g_ch;
		channel += 16;
	}
#endif

	ret = prvSetAlarmOutData(data, channel);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);


	DDEBUG_END
	return ret;
}

guint DAL_set_alarmOut_data_all(AlarmOutData *data, guint ch_num)
{
	guint ret = 0;
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	for(i=0; i<ch_num; i++)
	{
#if defined(_ANF_1648) || defined(_ANF_0824) || defined(_SNF_MODEL)
		if(i < g_ch)
			ret = prvSetAlarmOutData(data[i], i);
		else
			ret = prvSetAlarmOutData(data[i], 16 + (i - g_ch));
#else
		ret = prvSetAlarmOutData(data[i], i);
#endif
		if(ret)	break;
	}

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);


	DDEBUG_END
		return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_buzzOut_data(BuzzOutData *data)
{
	guint ret = 0;
	GValue ret_value = {0,};
	gchar *temp;
	guint i;

	if(nf_sysdb_get_key0("act.buzzer.act", &ret_value, NULL))
	{
		data->oper = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	if(nf_sysdb_get_key0("act.buzzer.dwell_type", &ret_value, NULL))
	{
		data->mode = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 2;

	if(nf_sysdb_get_key0("act.buzzer.hdd_event", &ret_value, NULL))
	{
		data->hddEvt = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 3;

	if(nf_sysdb_get_key0("act.buzzer.dwell_time", &ret_value, NULL))
	{
		data->duration = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 4;
#ifdef __SUPPORT_BS8418__
	if(nf_sysdb_get_key0("act.buzzer.ups_event", &ret_value, NULL))
	{
		data->upsEvt = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 5;
#endif
	if(nf_sysdb_get_key0("act.buzzer.alarm", &ret_value, NULL))
	{
		temp = g_value_get_string(&ret_value);

#ifdef DB_ALARM_IN_4CH_SUPPORT
		for(i=0; i<16; i++)
		{
			if(i < 4)
			{
				if(temp[i] == '0')	data->alarm[i] = 0;
				else				data->alarm[i] = 1;
			}
			else
			{
				data->alarm[i] = 0;
			}
		}

#else
		for(i=0; i<16; i++)
		{
			if(temp[i] == '0')	data->alarm[i] = 0;
			else				data->alarm[i] = 1;
		}
#endif
		g_value_unset(&ret_value);
	}
	else		ret = 6;

	if(nf_sysdb_get_key0("act.buzzer.vloss", &ret_value, NULL))
	{
		temp = g_value_get_string(&ret_value);

		for(i=0; i<16; i++)
		{
			if(temp[i] == '0')	data->videoLoss[i] = 0;
			else				data->videoLoss[i] = 1;
		}
		g_value_unset(&ret_value);
	}
	else		ret = 7;

	if(nf_sysdb_get_key0("act.buzzer.motion", &ret_value, NULL))
	{
		temp = g_value_get_string(&ret_value);

		for(i=0; i<16; i++)
		{
			if(temp[i] == '0')	data->motion[i] = 0;
			else				data->motion[i] = 1;
		}
		g_value_unset(&ret_value);
	}
	else		ret = 8;
#ifdef __SUPPORT_BS8418__
	if(nf_sysdb_get_key0("act.buzzer.lcont", &ret_value, NULL))
	{
		temp = g_value_get_string(&ret_value);

		for(i=0; i<16; i++)
		{
			if(temp[i] == '0')	data->lowContr[i] = 0;
			else				data->lowContr[i] = 1;
		}
		g_value_unset(&ret_value);
	}
	else		ret = 9;
#endif
	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_set_buzzOut_data(BuzzOutData *data)
{
	GValue set_value = {0,};
	gchar temp[MAX_TEMP_STR_SIZE];
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->oper);
	if(!nf_sysdb_set_key0("act.buzzer.act", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->mode);
	if(!nf_sysdb_set_key0("act.buzzer.dwell_type", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->hddEvt);
	if(!nf_sysdb_set_key0("act.buzzer.hdd_event", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->duration);
	if(!nf_sysdb_set_key0("act.buzzer.dwell_time", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);
#ifdef __SUPPORT_BS8418__
	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->upsEvt);
	if(!nf_sysdb_set_key0("act.buzzer.ups_event", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);
#endif
	memset(temp, 0, sizeof(gchar)*MAX_TEMP_STR_SIZE);
#ifdef DB_ALARM_IN_4CH_SUPPORT
	for(i=0; i<16; i++)
	{
		if(i<4)
		{
			if(data->alarm[i] == 0)	temp[i] = '0';
			else					temp[i] = '1';
		}
		else
		{
			temp[i] = '0';
		}
	}
#else
	for(i=0; i<16; i++)
	{
		if(data->alarm[i] == 0)	temp[i] = '0';
		else					temp[i] = '1';
	}
#endif
	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, temp);
	if(!nf_sysdb_set_key0("act.buzzer.alarm", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	memset(temp, 0, sizeof(gchar)*MAX_TEMP_STR_SIZE);
	for(i=0; i<16; i++)
	{
		if(data->videoLoss[i] == 0)	temp[i] = '0';
		else						temp[i] = '1';
	}

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, temp);
	if(!nf_sysdb_set_key0("act.buzzer.vloss", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	memset(temp, 0, sizeof(gchar)*MAX_TEMP_STR_SIZE);
	for(i=0; i<16; i++)
	{
		if(data->motion[i] == 0)	temp[i] = '0';
		else						temp[i] = '1';
	}

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, temp);
	if(!nf_sysdb_set_key0("act.buzzer.motion", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);
#ifdef __SUPPORT_BS8418__
	memset(temp, 0, sizeof(gchar)*MAX_TEMP_STR_SIZE);
	for(i=0; i<16; i++)
	{
		if(data->lowContr[i] == 0)	temp[i] = '0';
		else						temp[i] = '1';
	}

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, temp);
	if(!nf_sysdb_set_key0("act.buzzer.lcont", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);
#endif
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);

	DDEBUG_END
	return 0;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_emailNoti_data(EmailNotiData *data)
{
	guint ret = 0;
	GValue ret_value = {0,};
	gchar *temp;
	guint i;

	if(nf_sysdb_get_key0("act.email.act", &ret_value, NULL))
	{
		data->noti = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	if(nf_sysdb_get_key0("act.email.hdd_event", &ret_value, NULL))
	{
		data->hddEvt = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 2;

	if(nf_sysdb_get_key0("act.email.setup_chg", &ret_value, NULL))
	{
		data->setupChange = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 3;

	if(nf_sysdb_get_key0("act.email.booting", &ret_value, NULL))
	{
		data->bootEvt = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 4;
#ifdef __SUPPORT_BS8418__
	if(nf_sysdb_get_key0("act.email.ups_event", &ret_value, NULL))
	{
		data->upsEvt = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 5;
#endif
	if(nf_sysdb_get_key0("act.email.frequency", &ret_value, NULL))
	{
		data->freq = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 6;

	if(nf_sysdb_get_key0("act.email.alarm", &ret_value, NULL))
	{
		temp = g_value_get_string(&ret_value);

#ifdef DB_ALARM_IN_4CH_SUPPORT
		for(i=0; i<16; i++)
		{
			if(i<4)
			{
				if(temp[i] == '0')	data->alarm[i] = 0;
				else				data->alarm[i] = 1;
			}
			else
			{
				data->alarm[i] = 0;
			}
		}
#else
		for(i=0; i<16; i++)
		{
			if(temp[i] == '0')	data->alarm[i] = 0;
			else				data->alarm[i] = 1;
		}
#endif
		g_value_unset(&ret_value);
	}
	else		ret = 7;

	if(nf_sysdb_get_key0("act.email.vloss", &ret_value, NULL))
	{
		temp = g_value_get_string(&ret_value);

		for(i=0; i<16; i++)
		{
			if(temp[i] == '0')	data->videoLoss[i] = 0;
			else				data->videoLoss[i] = 1;
		}
		g_value_unset(&ret_value);
	}
	else		ret = 8;

	if(nf_sysdb_get_key0("act.email.motion", &ret_value, NULL))
	{
		temp = g_value_get_string(&ret_value);

		for(i=0; i<16; i++)
		{
			if(temp[i] == '0')	data->motion[i] = 0;
			else				data->motion[i] = 1;
		}
		g_value_unset(&ret_value);
	}
	else		ret = 9;
#ifdef __SUPPORT_BS8418__
	if(nf_sysdb_get_key0("act.email.lcont", &ret_value, NULL))
	{
		temp = g_value_get_string(&ret_value);

		for(i=0; i<16; i++)
		{
			if(temp[i] == '0')	data->lowContr[i] = 0;
			else				data->lowContr[i] = 1;
		}
		g_value_unset(&ret_value);
	}
	else		ret = 10;
#endif
	if(nf_sysdb_get_key0("act.email.snapshot_ch", &ret_value, NULL))
	{
		data->snapshot_ch = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 11;

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_set_emailNoti_data(EmailNotiData *data)
{
	GValue set_value = {0,};
	gchar temp[MAX_TEMP_STR_SIZE];
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->noti);
	if(!nf_sysdb_set_key0("act.email.act", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->hddEvt);
	if(!nf_sysdb_set_key0("act.email.hdd_event", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->setupChange);
	if(!nf_sysdb_set_key0("act.email.setup_chg", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->bootEvt);
	if(!nf_sysdb_set_key0("act.email.booting", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);
#ifdef __SUPPORT_BS8418__
	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->upsEvt);
	if(!nf_sysdb_set_key0("act.email.ups_event", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);
#endif
	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->freq);
	if(!nf_sysdb_set_key0("act.email.frequency", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);

	memset(temp, 0, sizeof(gchar)*MAX_TEMP_STR_SIZE);
#ifdef DB_ALARM_IN_4CH_SUPPORT
	for(i=0; i<16; i++)
	{
		if(i<4)
		{
			if(data->alarm[i] == 0)	temp[i] = '0';
			else					temp[i] = '1';
		}
		else
		{
			temp[i] = '0';
		}
	}
#else
	for(i=0; i<16; i++)
	{
		if(data->alarm[i] == 0)	temp[i] = '0';
		else					temp[i] = '1';
	}
#endif
	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, temp);
	if(!nf_sysdb_set_key0("act.email.alarm", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 7;
	}
	g_value_unset(&set_value);

	memset(temp, 0, sizeof(gchar)*MAX_TEMP_STR_SIZE);
	for(i=0; i<16; i++)
	{
		if(data->videoLoss[i] == 0)	temp[i] = '0';
		else						temp[i] = '1';
	}

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, temp);
	if(!nf_sysdb_set_key0("act.email.vloss", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 8;
	}
	g_value_unset(&set_value);

	memset(temp, 0, sizeof(gchar)*MAX_TEMP_STR_SIZE);
	for(i=0; i<16; i++)
	{
		if(data->motion[i] == 0)	temp[i] = '0';
		else						temp[i] = '1';
	}

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, temp);
	if(!nf_sysdb_set_key0("act.email.motion", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 9;
	}
	g_value_unset(&set_value);
#ifdef __SUPPORT_BS8418__
	memset(temp, 0, sizeof(gchar)*MAX_TEMP_STR_SIZE);
	for(i=0; i<16; i++)
	{
		if(data->lowContr[i] == 0)	temp[i] = '0';
		else						temp[i] = '1';
	}

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, temp);
	if(!nf_sysdb_set_key0("act.email.lcont", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 10;
	}
	g_value_unset(&set_value);
#endif

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->snapshot_ch);
	if(!nf_sysdb_set_key0("act.email.snapshot_ch", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 11;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);

	DDEBUG_END
	return 0;
}


#ifdef __SUPPORT_BS8418__
/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_ptzPreset_data(PtzPresetdata *data)
{
	guint ret = 0;
	GValue ret_value = {0,};
	gchar buf[16];
	gint i;

	if(nf_sysdb_get_key0("act.ptz.alarm", &ret_value, NULL))
	{
		strncpy(buf, g_value_get_string(&ret_value), sizeof(buf));
		g_value_unset(&ret_value);

		for(i=0; i<16; i++) {
			if(buf[i] == '1') data->alarm[i] = TRUE;
			else if(buf[i] == '0') data->alarm[i] = FALSE;
		}
	}
	else		ret = 1;

	if(nf_sysdb_get_key0("act.ptz.vloss", &ret_value, NULL))
	{
		strncpy(buf, g_value_get_string(&ret_value), sizeof(buf));
		g_value_unset(&ret_value);

		for(i=0; i<16; i++) {
			if(buf[i] == '1') data->vloss[i] = TRUE;
			else if(buf[i] == '0') data->vloss[i] = FALSE;
		}
	}
	else		ret = 2;

	if(nf_sysdb_get_key0("act.ptz.motion", &ret_value, NULL))
	{
		strncpy(buf, g_value_get_string(&ret_value), sizeof(buf));
		g_value_unset(&ret_value);

		for(i=0; i<16; i++) {
			if(buf[i] == '1') data->motion[i] = TRUE;
			else if(buf[i] == '0') data->motion[i] = FALSE;
		}
	}
	else		ret = 3;

	for(i=0; i<16; i++)
	{
		if(nf_sysdb_get_key1("act.ptz.alarm.A%d.preset", i, &ret_value, NULL))
		{
			data->alarm_preset[i] = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else		ret = 4;

		if(nf_sysdb_get_key1("act.ptz.vloss.V%d.preset", i, &ret_value, NULL))
		{
			data->vloss_preset[i] = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else		ret = 5;

		if(nf_sysdb_get_key1("act.ptz.motion.M%d.preset", i, &ret_value, NULL))
		{
			data->motion_preset[i] = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else		ret = 6;
	}

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_set_ptzPreset_data(PtzPresetdata *data)
{
	GValue set_value = {0,};
	gchar buf[16];
	guint i;

	DDEBUG_START

	for(i=0; i<16; i++) {
		if(data->alarm[i])	buf[i] = '1';
		else 				buf[i] = '0';
	}

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);
	if(!nf_sysdb_set_key0("act.ptz.alarm", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);


	for(i=0; i<16; i++) {
		if(data->vloss[i])	buf[i] = '1';
		else 				buf[i] = '0';
	}

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);
	if(!nf_sysdb_set_key0("act.ptz.vloss", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);


	for(i=0; i<16; i++) {
		if(data->motion[i])	buf[i] = '1';
		else 				buf[i] = '0';
	}

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);
	if(!nf_sysdb_set_key0("act.ptz.motion", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	for(i=0; i<16; i++)
	{
		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data->alarm_preset[i]);
		if(!nf_sysdb_set_key1("act.ptz.alarm.A%d.preset", i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
				return 4;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data->vloss_preset[i]);
		if(!nf_sysdb_set_key1("act.ptz.vloss.V%d.preset", i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
				return 5;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, data->motion_preset[i]);
		if(!nf_sysdb_set_key1("act.ptz.motion.M%d.preset", i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
				return 6;
		}
		g_value_unset(&set_value);
	}

	return 0;
}
#endif


#if defined(__SUPPORT_BS8418__) || defined(__SAMSUNG_UI__)
/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_snapCapture_data(SnapCaptureData *data)
{
	guint ret = 0;
	GValue ret_value = {0,};
	gchar buf[16];
	gint tmp;
	gint i;

	if(nf_sysdb_get_key0("act.snap.alarm", &ret_value, NULL))
	{
		strncpy(buf, g_value_get_string(&ret_value), sizeof(buf));
		g_value_unset(&ret_value);

		for(i=0; i<16; i++) {
			if(buf[i] == '1') data->alarm[i] = TRUE;
			else if(buf[i] == '0') data->alarm[i] = FALSE;
		}
	}
	else		ret = 1;

	if(nf_sysdb_get_key0("act.snap.motion", &ret_value, NULL))
	{
		strncpy(buf, g_value_get_string(&ret_value), sizeof(buf));
		g_value_unset(&ret_value);

		for(i=0; i<16; i++) {
			if(buf[i] == '1') data->motion[i] = TRUE;
			else if(buf[i] == '0') data->motion[i] = FALSE;
		}
	}
	else		ret = 2;

	if(nf_sysdb_get_key0("act.snap.lcont", &ret_value, NULL))
	{
		strncpy(buf, g_value_get_string(&ret_value), sizeof(buf));
		g_value_unset(&ret_value);

		for(i=0; i<16; i++) {
			if(buf[i] == '1') data->lcont[i] = TRUE;
			else if(buf[i] == '0') data->lcont[i] = FALSE;
		}
	}
	else		ret = 3;

	for(i=0; i<16; i++)
	{
		if(nf_sysdb_get_key1("act.snap.alarm.A%d.delay", i, &ret_value, NULL))
		{
			tmp = g_value_get_int(&ret_value);
			data->alarm_delay[i] = tmp + 5;
			g_value_unset(&ret_value);
		}
		else		ret = 4;

		if(nf_sysdb_get_key1("act.snap.vloss.V%d.delay", i, &ret_value, NULL))
		{
			tmp = g_value_get_int(&ret_value);
			data->motion_delay[i] = tmp + 5;
			g_value_unset(&ret_value);
		}
		else		ret = 5;

		if(nf_sysdb_get_key1("act.snap.lcont.L%d.delay", i, &ret_value, NULL))
		{
			tmp = g_value_get_int(&ret_value);
			data->lcont_delay[i] = tmp + 5;
			g_value_unset(&ret_value);
		}
		else		ret = 6;
	}

	return ret;

}


/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_set_snapCapture_data(SnapCaptureData *data)
{
	GValue set_value = {0,};
	gchar buf[16];
	gint seconds[] = {-5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5};
	guint i;

	DDEBUG_START

	for(i=0; i<16; i++) {
		if(data->alarm[i])	buf[i] = '1';
		else 				buf[i] = '0';
	}

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);
	if(!nf_sysdb_set_key0("act.snap.alarm", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);


	for(i=0; i<16; i++) {
		if(data->motion[i])	buf[i] = '1';
		else 				buf[i] = '0';
	}

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);
	if(!nf_sysdb_set_key0("act.snap.motion", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);


	for(i=0; i<16; i++) {
		if(data->lcont[i])	buf[i] = '1';
		else 				buf[i] = '0';
	}

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);
	if(!nf_sysdb_set_key0("act.snap.lcont", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	for(i=0; i<16; i++)
	{
		g_value_init(&set_value, G_TYPE_INT);
		g_value_set_int(&set_value, seconds[data->alarm_delay[i]]);
		if(!nf_sysdb_set_key1("act.snap.alarm.A%d.delay", i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
				return 4;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_INT);
		g_value_set_int(&set_value, seconds[data->motion_delay[i]]);
		if(!nf_sysdb_set_key1("act.snap.vloss.V%d.delay", i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
				return 5;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_INT);
		g_value_set_int(&set_value, seconds[data->lcont_delay[i]]);
		if(!nf_sysdb_set_key1("act.snap.lcont.L%d.delay", i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			DDEBUG_FAIL
				return 6;
		}
		g_value_unset(&set_value);
	}

	return 0;
}
#endif




/********************************************************************************
 * Setup --> Event&Action --> Alarm Out
 * ******************************************************************************/

gboolean DAL_set_almOut_data(EA_AlmOutData data, guint ch)
{
	GValue set_value = {0,};

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	DDEBUG_START

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.name);
	if(!nf_sysdb_set_key1("act.arout.R%d.name", (ch >= g_ch ? ch - g_ch + 32 : ch), &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	if(data.type == 0) g_value_set_boolean(&set_value, FALSE);
	else			   g_value_set_boolean(&set_value, TRUE);
	if(!nf_sysdb_set_key1("act.arout.R%d.relay_type", (ch >= g_ch ? ch - g_ch + 32 : ch), &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	if(data.oper == 0) g_value_set_boolean(&set_value, FALSE);
	else			   g_value_set_boolean(&set_value, TRUE);
	if(!nf_sysdb_set_key1("act.arout.R%d.op_type", (ch >= g_ch ? ch - g_ch + 32 : ch), &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);


	g_value_init(&set_value, G_TYPE_INT);
	g_value_set_int(&set_value, data.duration);
	if(!nf_sysdb_set_key1("act.arout.R%d.duration", (ch >= g_ch ? ch - g_ch + 32 : ch), &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);

	//printf("%s : %s [%d] ::::::::::::::::::::::::::::::::::::::\n", __FILE__, __FUNCTION__, __LINE__);
	return TRUE;
}

gboolean DAL_set_almOut_data_all(EA_AlmOutData *data, guint ch)
{
	GValue set_value = {0,};
	gint i;

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	DDEBUG_START

	for(i=0; i<ch; i++) {
		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, data[i].name);
		if(!nf_sysdb_set_key1("act.arout.R%d.name", (i >= g_ch ? i - g_ch + 32 : i), &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		if(data[i].type == 0) g_value_set_boolean(&set_value, FALSE);
		else			   g_value_set_boolean(&set_value, TRUE);
		if(!nf_sysdb_set_key1("act.arout.R%d.relay_type", (i >= g_ch ? i - g_ch + 32 : i), &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		if(data[i].oper == 0) g_value_set_boolean(&set_value, FALSE);
		else			   g_value_set_boolean(&set_value, TRUE);
		if(!nf_sysdb_set_key1("act.arout.R%d.op_type", (i >= g_ch ? i - g_ch + 32 : i), &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);


		g_value_init(&set_value, G_TYPE_INT);
		g_value_set_int(&set_value, data[i].duration);
		if(!nf_sysdb_set_key1("act.arout.R%d.duration", (i >= g_ch ? i - g_ch + 32 : i), &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);
	}

	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);

	return TRUE;
}

gboolean DAL_get_almOut_data(EA_AlmOutData *data, guint ch)
{
	GValue ret_value = {0,};
	const gchar *str = NULL;

	if(nf_sysdb_get_key1("act.arout.R%d.name", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);
		g_stpcpy(data->name, str);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.arout.R%d.relay_type", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		if(g_value_get_boolean(&ret_value))		data->type = 1;			// digital out
		else									data->type = 0;			// relay out
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.arout.R%d.op_type", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		if(g_value_get_boolean(&ret_value))		data->oper = 1;			// N/C
		else 									data->oper = 0;			// N/O
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.arout.R%d.duration", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		data->duration = g_value_get_int(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	return TRUE;
}

gboolean DAL_set_nvr_almOut_data_all(EA_AlmOutData *data, guint ch)
{
	GValue set_value = {0,};
	gint i;

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	DDEBUG_START

	for(i=0; i<ch; i++) {
		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, data[i].name);
		if(!nf_sysdb_set_key1("act.nvrarout.R%d.name", (i >= g_ch ? i - g_ch + 32 : i), &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		if(data[i].type == 0) g_value_set_boolean(&set_value, FALSE);
		else			   g_value_set_boolean(&set_value, TRUE);
		if(!nf_sysdb_set_key1("act.nvrarout.R%d.relay_type", (i >= g_ch ? i - g_ch + 32 : i), &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		if(data[i].oper == 0) g_value_set_boolean(&set_value, FALSE);
		else			   g_value_set_boolean(&set_value, TRUE);
		if(!nf_sysdb_set_key1("act.nvrarout.R%d.op_type", (i >= g_ch ? i - g_ch + 32 : i), &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);


		g_value_init(&set_value, G_TYPE_INT);
		g_value_set_int(&set_value, data[i].duration);
		if(!nf_sysdb_set_key1("act.nvrarout.R%d.duration", (i >= g_ch ? i - g_ch + 32 : i), &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);
	}

	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);

	return TRUE;
}

gboolean DAL_get_nvr_almOut_data(EA_AlmOutData *data, guint ch)
{
	GValue ret_value = {0,};
	const gchar *str = NULL;

	if(nf_sysdb_get_key1("act.nvrarout.R%d.name", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);
		g_stpcpy(data->name, str);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.nvrarout.R%d.relay_type", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		if(g_value_get_boolean(&ret_value))		data->type = 1;			// digital out
		else									data->type = 0;			// relay out
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.nvrarout.R%d.op_type", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		if(g_value_get_boolean(&ret_value))		data->oper = 1;			// N/C
		else 									data->oper = 0;			// N/O
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.nvrarout.R%d.duration", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		data->duration = g_value_get_int(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	return TRUE;
}



/***************************************************************
 * Setup --> Event&Action --> Alarm Out --> On/Off Schedule
 * *************************************************************/
gboolean DAL_set_almSched_data(EA_AlmSchedData data, guint ch)
{
	GValue set_value = {0};
	gchar buf[169];


	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data.sched[0], 7 * 24);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);
	if(!nf_sysdb_set_key1("act.arout.R%d.sched", (ch >= g_ch ? ch - g_ch + 32 : ch), &set_value, NULL)) {
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);

	return FALSE;
}

gboolean DAL_set_almSched_data_all(EA_AlmSchedData *data, guint ch)
{
	GValue set_value = {0};
	gchar buf[169];
	gint i;


	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	for(i=0; i<ch; i++) {
		memset(buf, 0, sizeof(buf));
		memcpy(buf, data[i].sched[0], 7 * 24);

		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, buf);
		if(!nf_sysdb_set_key1("act.arout.R%d.sched", (i >= g_ch ? i - g_ch + 32 : i), &set_value, NULL)) {
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);
	}

	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);

	return FALSE;
}

gboolean DAL_get_almSched_data(EA_AlmSchedData *data, guint ch)
{
	GValue ret_value = {0,};
	const gchar *str = NULL;
	gchar buf[169];

	if(nf_sysdb_get_key1("act.arout.R%d.sched", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);

		memset(buf, 0, sizeof(buf));
		g_stpcpy(buf, str);

		memcpy(data->sched[0], buf, 7 * 24);
		g_value_unset(&ret_value);

		return TRUE;
	}

	return FALSE;
}

gboolean DAL_set_nvr_almSched_data_all(EA_AlmSchedData *data, guint ch)
{
	GValue set_value = {0};
	gchar buf[169];
	gint i;


	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	for(i=0; i<ch; i++) {
		memset(buf, 0, sizeof(buf));
		memcpy(buf, data[i].sched[0], 7 * 24);

		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, buf);
		if(!nf_sysdb_set_key1("act.nvrarout.R%d.sched", (i >= g_ch ? i - g_ch + 32 : i), &set_value, NULL)) {
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);
	}

	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);

	return FALSE;
}

gboolean DAL_get_nvr_almSched_data(EA_AlmSchedData *data, guint ch)
{
	GValue ret_value = {0,};
	const gchar *str = NULL;
	gchar buf[169];

	if(nf_sysdb_get_key1("act.nvrarout.R%d.sched", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);

		memset(buf, 0, sizeof(buf));
		g_stpcpy(buf, str);

		memcpy(data->sched[0], buf, 7 * 24);
		g_value_unset(&ret_value);

		return TRUE;
	}

	return FALSE;
}

/***************************************************************
 * Setup --> Event&Action --> Event Notification --> Buzzer
 * *************************************************************/
gboolean DAL_set_evtNoti_buzzer_duration(gint duration)
{
	GValue set_value = {0};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	g_value_init(&set_value, G_TYPE_INT);
	g_value_set_int(&set_value, duration);
	if(!nf_sysdb_set_key0("act.event.buzzer.duration", &set_value, NULL)) {
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);

	return TRUE;
}

gint DAL_get_evtNoti_buzzer_duration()
{
	gint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("act.event.buzzer.duration", &ret_value, NULL))
	{
		ret = g_value_get_int(&ret_value);
		g_value_unset(&ret_value);

		return ret;
	}

	return ret;
}


/***************************************************************
 * Setup --> Event&Action --> Event Notification --> Display
 * *************************************************************/
gboolean DAL_set_evtNoti_disp_data(EA_EvtNotiDispData data)
{
	guint ret = 0;
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.vpop_duration);
	if(!nf_sysdb_set_key0("act.event.display.vpopup.duration", &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.vpop_multi);
	if(!nf_sysdb_set_key0("act.event.display.vpopup.multi", &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.vpop_spot);
	if(!nf_sysdb_set_key0("act.event.display.vpopup.spot", &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);


	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.opop_duration);
	if(!nf_sysdb_set_key0("act.event.display.duration.popup", &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.opop_spot);
	if(!nf_sysdb_set_key0("act.event.display.spot.popup", &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);

	DDEBUG_START
	return TRUE;
}

gboolean DAL_get_evtNoti_disp_data(EA_EvtNotiDispData *data)
{
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("act.event.display.vpopup.duration", &ret_value, NULL))
	{
		data->vpop_duration = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key0("act.event.display.vpopup.multi", &ret_value, NULL))
	{
		data->vpop_multi = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key0("act.event.display.vpopup.spot", &ret_value, NULL))
	{
		data->vpop_spot = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;


	if(nf_sysdb_get_key0("act.event.display.duration.popup", &ret_value, NULL))
	{
		data->opop_duration = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key0("act.event.display.spot.popup", &ret_value, NULL))
	{
		data->opop_spot = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	return TRUE;
}


guint DAL_get_evtNoti_vPop_duration()
{
	GValue ret_value = {0,};
	guint ret = 0;

	if(nf_sysdb_get_key0("act.event.display.vpopup.duration", &ret_value, NULL))
	{
		ret = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}

	return ret;
}


guint DAL_get_evtNoti_oPop_duration()
{
	GValue ret_value = {0,};
	guint ret = 0;

	if(nf_sysdb_get_key0("act.event.display.duration.popup", &ret_value, NULL))
	{
		ret = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}

	return ret;
}


/***************************************************************
 * Setup --> Event&Action --> Event Notification --> E-mail
 * *************************************************************/
static gboolean prvSetEmailSchedData(EMAIL_SCHED_T *data, gint idx)
{
    GValue set_value = {0,};


	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->wday);
	if(!nf_sysdb_set_key1("act.event.email.S%d.wday", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->from_h);
	if(!nf_sysdb_set_key1("act.event.email.S%d.from_h", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->from_m);
	if(!nf_sysdb_set_key1("act.event.email.S%d.from_m", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->to_h);
	if(!nf_sysdb_set_key1("act.event.email.S%d.to_h", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->to_m);
	if(!nf_sysdb_set_key1("act.event.email.S%d.to_m", idx, &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	return TRUE;
}

gboolean DAL_set_evtNoti_email_data(EA_EvtNotiEmailData data)
{
	GValue set_value = {0};
	gboolean ret = FALSE;
	gint idx;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.count);
	if(!nf_sysdb_set_key0("act.event.ACNT", &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	for(idx=0; idx<8; idx++) {
		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, data.address[idx]);
		if(!nf_sysdb_set_key1("act.event.A%d.address", idx, &set_value, NULL)) {
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
			return FALSE;
		}
		g_value_unset(&set_value);
	}

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.frequency);
	if(!nf_sysdb_set_key0("act.event.email.freq", &set_value, NULL)) {
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.include_jpeg);
	if(!nf_sysdb_set_key0("act.event.email.jpeg", &set_value, NULL)) {
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.serv);
	if(!nf_sysdb_set_key0("act.event.email.serv", &set_value, NULL)) {
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
    g_value_set_uint(&set_value, data.from_time);
    if(!nf_sysdb_set_key0("act.event.email.sched.from", &set_value, NULL)) {
        nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
        DDEBUG_FAIL
        return FALSE;
    }
    g_value_unset(&set_value);

    g_value_init(&set_value, G_TYPE_UINT);
    g_value_set_uint(&set_value, data.to_time);
    if(!nf_sysdb_set_key0("act.event.email.sched.to", &set_value, NULL)) {
        nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
        DDEBUG_FAIL
        return FALSE;
    }
    g_value_unset(&set_value);

    g_value_init(&set_value, G_TYPE_UINT);
    g_value_set_uint(&set_value, data.day[0]);
    if(!nf_sysdb_set_key0("act.event.email.sched.wday0", &set_value, NULL)) {
        nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
        DDEBUG_FAIL
        return FALSE;
    }
    g_value_unset(&set_value);

    g_value_init(&set_value, G_TYPE_UINT);
    g_value_set_uint(&set_value, data.day[1]);
    if(!nf_sysdb_set_key0("act.event.email.sched.wday1", &set_value, NULL)) {
        nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
        DDEBUG_FAIL
        return FALSE;
    }
    g_value_unset(&set_value);

    g_value_init(&set_value, G_TYPE_UINT);
    g_value_set_uint(&set_value, data.day[2]);
    if(!nf_sysdb_set_key0("act.event.email.sched.wday2", &set_value, NULL)) {
        nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
        DDEBUG_FAIL
        return FALSE;
    }
    g_value_unset(&set_value);

    g_value_init(&set_value, G_TYPE_UINT);
    g_value_set_uint(&set_value, data.day[3]);
    if(!nf_sysdb_set_key0("act.event.email.sched.wday3", &set_value, NULL)) {
        nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
        DDEBUG_FAIL
        return FALSE;
    }
    g_value_unset(&set_value);

    g_value_init(&set_value, G_TYPE_UINT);
    g_value_set_uint(&set_value, data.day[4]);
    if(!nf_sysdb_set_key0("act.event.email.sched.wday4", &set_value, NULL)) {
        nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
        DDEBUG_FAIL
        return FALSE;
    }
    g_value_unset(&set_value);

    g_value_init(&set_value, G_TYPE_UINT);
    g_value_set_uint(&set_value, data.day[5]);
    if(!nf_sysdb_set_key0("act.event.email.sched.wday5", &set_value, NULL)) {
        nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
        DDEBUG_FAIL
        return FALSE;
    }
    g_value_unset(&set_value);

    g_value_init(&set_value, G_TYPE_UINT);
    g_value_set_uint(&set_value, data.day[6]);
    if(!nf_sysdb_set_key0("act.event.email.sched.wday6", &set_value, NULL)) {
        nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
        DDEBUG_FAIL
        return FALSE;
    }
    g_value_unset(&set_value);

    g_value_init(&set_value, G_TYPE_UINT);
    g_value_set_uint(&set_value, data.al_switch);
    if(!nf_sysdb_set_key0("act.event.email.al_switch", &set_value, NULL)) {
        nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
        DDEBUG_FAIL
        return FALSE;
    }
    g_value_unset(&set_value);

    g_value_init(&set_value, G_TYPE_UINT);
    g_value_set_uint(&set_value, data.al_switch_port);
    if(!nf_sysdb_set_key0("act.event.email.al_switch_port", &set_value, NULL)) {
        nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
        DDEBUG_FAIL
        return FALSE;
    }
    g_value_unset(&set_value);

    for (idx = 0; idx < EMAIL_SCHEDULING_MAX_CNT; idx++)
    {
        ret = prvSetEmailSchedData(&data.sched[idx], idx);
        if (!ret) {
            nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
            DDEBUG_FAIL
            return FALSE;
        }
    }

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);

    DDEBUG_END

	return TRUE;
}

static guint prvGetEmailSchedData(EMAIL_SCHED_T *data, gint idx)
{
	GValue ret_value = {0,};

	if (nf_sysdb_get_key1("act.event.email.S%d.wday", idx, &ret_value, NULL))
	{
	    data->wday = g_value_get_uint(&ret_value);
	    g_value_unset(&ret_value);
	}
	else
	{
	    g_value_unset(&ret_value);
	    return FALSE;
    }

	if (nf_sysdb_get_key1("act.event.email.S%d.from_h", idx, &ret_value, NULL))
	{
	    data->from_h = g_value_get_uint(&ret_value);
	    g_value_unset(&ret_value);
	}
	else
	{
	    g_value_unset(&ret_value);
	    return FALSE;
    }

	if (nf_sysdb_get_key1("act.event.email.S%d.from_m", idx, &ret_value, NULL))
	{
	    data->from_m = g_value_get_uint(&ret_value);
	    g_value_unset(&ret_value);
	}
	else
	{
	    g_value_unset(&ret_value);
	    return FALSE;
    }

	if (nf_sysdb_get_key1("act.event.email.S%d.to_h", idx, &ret_value, NULL))
	{
	    data->to_h = g_value_get_uint(&ret_value);
	    g_value_unset(&ret_value);
	}
	else
	{
	    g_value_unset(&ret_value);
	    return FALSE;
    }

	if (nf_sysdb_get_key1("act.event.email.S%d.to_m", idx, &ret_value, NULL))
	{
	    data->to_m = g_value_get_uint(&ret_value);
	    g_value_unset(&ret_value);
	}
	else
	{
	    g_value_unset(&ret_value);
	    return FALSE;
    }

    return TRUE;
}

gboolean DAL_get_evtNoti_email_data(EA_EvtNotiEmailData *data)
{
	GValue ret_value = {0,};
	gchar *str = NULL;
	guint ret = 0;
	gint idx;

	if(nf_sysdb_get_key0("act.event.ACNT", &ret_value, NULL))
	{
		data->count = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	for(idx=0; idx<8; idx++) {
		if(nf_sysdb_get_key1("act.event.A%d.address", idx, &ret_value, NULL)) {
			str = g_value_get_string(&ret_value);
			if(str) {
				g_stpcpy(data->address[idx], str);
				str = NULL;
			}
			g_value_unset(&ret_value);
		}else
			return FALSE;
	}

	if(nf_sysdb_get_key0("act.event.email.freq", &ret_value, NULL))
	{
		data->frequency = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);

	}else
		return FALSE;

	if(nf_sysdb_get_key0("act.event.email.jpeg", &ret_value, NULL))
	{
		data->include_jpeg = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);

	}else
		return FALSE;

	if(nf_sysdb_get_key0("act.event.email.serv", &ret_value, NULL))
	{
		data->serv = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);

	}else
		return FALSE;

	if(nf_sysdb_get_key0("act.event.email.sched.from", &ret_value, NULL))
    {
        data->from_time = g_value_get_uint(&ret_value);
        g_value_unset(&ret_value);
    }
    else
        return FALSE;

    if(nf_sysdb_get_key0("act.event.email.sched.to", &ret_value, NULL))
    {
        data->to_time = g_value_get_uint(&ret_value);
        g_value_unset(&ret_value);
    }
    else
        return FALSE;

    if(nf_sysdb_get_key0("act.event.email.sched.wday0", &ret_value, NULL))
    {
        data->day[0] = g_value_get_uint(&ret_value);
        g_value_unset(&ret_value);
    }
    else
        return FALSE;

    if(nf_sysdb_get_key0("act.event.email.sched.wday1", &ret_value, NULL))
    {
        data->day[1] = g_value_get_uint(&ret_value);
        g_value_unset(&ret_value);
    }
    else
        return FALSE;

    if(nf_sysdb_get_key0("act.event.email.sched.wday2", &ret_value, NULL))
    {
        data->day[2] = g_value_get_uint(&ret_value);
        g_value_unset(&ret_value);
    }
    else
        return FALSE;

    if(nf_sysdb_get_key0("act.event.email.sched.wday3", &ret_value, NULL))
    {
        data->day[3] = g_value_get_uint(&ret_value);
        g_value_unset(&ret_value);
    }
    else
        return FALSE;

    if(nf_sysdb_get_key0("act.event.email.sched.wday4", &ret_value, NULL))
    {
        data->day[4] = g_value_get_uint(&ret_value);
        g_value_unset(&ret_value);
    }
    else
        return FALSE;


    if(nf_sysdb_get_key0("act.event.email.sched.wday5", &ret_value, NULL))
    {
        data->day[5] = g_value_get_uint(&ret_value);
        g_value_unset(&ret_value);
    }
    else
        return FALSE;

    if(nf_sysdb_get_key0("act.event.email.sched.wday6", &ret_value, NULL))
    {
        data->day[6] = g_value_get_uint(&ret_value);
        g_value_unset(&ret_value);
    }
    else 
        return FALSE;	

    if(nf_sysdb_get_key0("act.event.email.al_switch", &ret_value, NULL))
    {
        data->al_switch= g_value_get_uint(&ret_value);
        g_value_unset(&ret_value);
    }
    else 
        return FALSE;
  
    if(nf_sysdb_get_key0("act.event.email.al_switch_port", &ret_value, NULL))
    {
        data->al_switch_port= g_value_get_uint(&ret_value);
        g_value_unset(&ret_value);
    }
    else 
        return FALSE;

    for (idx = 0; idx < EMAIL_SCHEDULING_MAX_CNT; idx++)
    {
        ret = prvGetEmailSchedData(&data->sched[idx], idx);
        if (!ret) return FALSE;
    }

	return TRUE;
}

gboolean DAL_set_evtNoti_email_address(gchar *addr, guint idx)
{
	GValue set_value = {0};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, addr);

	if(!nf_sysdb_set_key1("act.event.A%d.address", idx, &set_value, NULL)) {
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);

	return TRUE;
}

gboolean DAL_get_evtNoti_email_address(gchar *addr, guint idx)
{
	GValue ret_value = {0,};
	gchar *str = NULL;

	if(nf_sysdb_get_key1("act.event.A%d.address", idx, &ret_value, NULL)) {
		str = g_value_get_string(&ret_value);
		if(str) {
			g_stpcpy(addr, str);
			str = NULL;
			g_value_unset(&ret_value);
		}else {
			g_value_unset(&ret_value);
			return FALSE;
		}
	}else
		return FALSE;

	return TRUE;
}

gboolean DAL_set_evtNoti_email_frequency(guint frequency)
{
	GValue set_value = {0};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, frequency);
	if(!nf_sysdb_set_key0("act.event.email.freq", &set_value, NULL)) {
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);

	return TRUE;
}

guint DAL_get_evtNoti_email_frequency()
{
	GValue ret_value = {0,};
	guint ret = 0;

	if(nf_sysdb_get_key0("act.event.email.freq", &ret_value, NULL))
	{
		ret = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);

		return ret;
	}

	return ret;
}

/***************************************************************
 * Setup --> Event&Action --> Event Notification --> FTP
 * *************************************************************/
gboolean DAL_set_evtNoti_ftp_data(EA_EvtNotiFTPData data)
{
	GValue set_value = {0,};
	gchar temp[MAX_TEMP_STR_SIZE];
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.weblink);
	if(!nf_sysdb_set_key0("act.event.ftp.weblink", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.freq);
	if(!nf_sysdb_set_key0("act.event.ftp.freq", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.include_jpeg);
	if(!nf_sysdb_set_key0("act.event.ftp.jpeg", &set_value, NULL)) {
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.dir_mode);
	if(!nf_sysdb_set_key0("act.event.ftp.dir_mode", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.dir_path);
	if(!nf_sysdb_set_key0("act.event.ftp.dir_path", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.filename_mode);
	if(!nf_sysdb_set_key0("act.event.ftp.fname_mode", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.filename);
	if(!nf_sysdb_set_key0("act.event.ftp.fname_prefix", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 7;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.host);
	if(!nf_sysdb_set_key0("act.event.ftp.host", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 8;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.username);
	if(!nf_sysdb_set_key0("act.event.ftp.username", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 9;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.port);
	if(!nf_sysdb_set_key0("act.event.ftp.port", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 10;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.passwd);
	if(!nf_sysdb_set_key0("act.event.ftp.passwd", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 11;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.include_video);
	if(!nf_sysdb_set_key0("act.event.ftp.video", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 12;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);

	DDEBUG_END
	return 0;
}

gboolean DAL_get_evtNoti_ftp_data(EA_EvtNotiFTPData *data)
{
	guint ret = 0;
	GValue ret_value = {0,};
	gchar *temp;
	guint i;

	if(nf_sysdb_get_key0("act.event.ftp.weblink", &ret_value, NULL))
	{
		data->weblink = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	if(nf_sysdb_get_key0("act.event.ftp.freq", &ret_value, NULL))
	{
		data->freq = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 2;

	if(nf_sysdb_get_key0("act.event.ftp.jpeg", &ret_value, NULL))
	{
		data->include_jpeg = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);

	}
	else		ret = 3;

	if(nf_sysdb_get_key0("act.event.ftp.dir_mode", &ret_value, NULL))
	{
		data->dir_mode = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 4;

	if(nf_sysdb_get_key0("act.event.ftp.dir_path", &ret_value, NULL))
	{
		g_stpcpy(data->dir_path, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 5;

	if(nf_sysdb_get_key0("act.event.ftp.fname_mode", &ret_value, NULL))
	{
		data->filename_mode = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 6;

	if(nf_sysdb_get_key0("act.event.ftp.fname_prefix", &ret_value, NULL))
	{
		g_stpcpy(data->filename, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 7;

	if(nf_sysdb_get_key0("act.event.ftp.host", &ret_value, NULL))
	{
		g_stpcpy(data->host, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 8;

	if(nf_sysdb_get_key0("act.event.ftp.port", &ret_value, NULL))
	{
		data->port = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 9;

	if(nf_sysdb_get_key0("act.event.ftp.username", &ret_value, NULL))
	{
		g_stpcpy(data->username, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 10;

	if(nf_sysdb_get_key0("act.event.ftp.passwd", &ret_value, NULL))
	{
		g_stpcpy(data->passwd, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 11;

	if(nf_sysdb_get_key0("act.event.ftp.video", &ret_value, NULL))
	{
		data->include_video = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);

	}
	else		ret = 12;

	return ret;
}

/***************************************************************
 * Setup --> Event&Action --> Event Notification --> SMS
 * *************************************************************/
gboolean DAL_set_evtNoti_sms_data(EA_EvtNotiSMSData data)
{
	GValue set_value = {0,};
	guint i;

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.server);
	if(!nf_sysdb_set_key0("act.event.sms.server", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.appid);
	if(!nf_sysdb_set_key0("act.event.sms.appid", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.user);
	if(!nf_sysdb_set_key0("act.event.sms.user", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.password);
	if(!nf_sysdb_set_key0("act.event.sms.password", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.sched_from);
	if(!nf_sysdb_set_key0("act.event.sms.sched_from", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.sched_to);
	if(!nf_sysdb_set_key0("act.event.sms.sched_to", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.count);
	if(!nf_sysdb_set_key0("act.event.sms.count", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 7;
	}
	g_value_unset(&set_value);


	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);

	DDEBUG_END
	return 0;
}

gboolean DAL_get_evtNoti_sms_data(EA_EvtNotiSMSData *data)
{
	guint ret = 0;
	GValue ret_value = {0,};
	guint i;

	if(nf_sysdb_get_key0("act.event.sms.server", &ret_value, NULL))
	{
		g_stpcpy(data->server, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	if(nf_sysdb_get_key0("act.event.sms.appid", &ret_value, NULL))
	{
		g_stpcpy(data->appid, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 2;

	if(nf_sysdb_get_key0("act.event.sms.user", &ret_value, NULL))
	{
		g_stpcpy(data->user, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 3;

	if(nf_sysdb_get_key0("act.event.sms.password", &ret_value, NULL))
	{
		g_stpcpy(data->password, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 4;

	if(nf_sysdb_get_key0("act.event.sms.sched_from", &ret_value, NULL))
	{
		data->sched_from = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 5;

	if(nf_sysdb_get_key0("act.event.sms.sched_to", &ret_value, NULL))
	{
		data->sched_to = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 6;

	if(nf_sysdb_get_key0("act.event.sms.count", &ret_value, NULL))
	{
		data->count = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 7;

	return ret;
}



/***************************************************************
 * Setup --> Event&Action --> Event Notification --> Mobile Push
 * *************************************************************/
gboolean DAL_set_evtNoti_mobilepush_frequency(gint frequency)
{
	GValue set_value = {0};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, frequency);
	if(!nf_sysdb_set_key0("act.event.mobilepush.freq", &set_value, NULL)) {
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);

	return TRUE;
}

gint DAL_get_evtNoti_mobilepush_frequency()
{
	GValue ret_value = {0,};
	guint ret = 0;

	if(nf_sysdb_get_key0("act.event.mobilepush.freq", &ret_value, NULL))
	{
		ret = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);

		return ret;
	}

	return ret;
}


/***************************************************************
 * Setup --> Event&Action --> Alarm Sensor
 * *************************************************************/
gboolean DAL_set_almSen_data(EA_AlmSenData data, guint ch)
{
	GValue set_value = {0,};
	gchar buf[49];
	gint i, j;


	nf_sysdb_lock(NF_SYSDB_CATE_ALARM);

	DDEBUG_START

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.name);
	if(!nf_sysdb_set_key1("alarm.sensor.S%d.name", (ch >= g_ch ? ch - g_ch + 32 : ch), &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ALARM);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	if(data.oper == 0) g_value_set_boolean(&set_value, FALSE);
	else			   g_value_set_boolean(&set_value, TRUE);
	if(!nf_sysdb_set_key1("alarm.sensor.S%d.op_type", (ch >= g_ch ? ch - g_ch + 32 : ch), &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ALARM);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_ALARM);


	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	memset(buf, 0, sizeof(buf));
	for(i=0; i<32; i++)
	{
		if(data.linkCam & (1 << i))	buf[i] = '1';
		else						buf[i] = '0';
	}
	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);
	if(!nf_sysdb_set_key1("act.sensor.S%d.lcamera", (ch >= g_ch ? ch - g_ch + 32 : ch), &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	for(i=0, j=0; i<48; i++)
	{
		if(i >= g_ch) {
			if(i < 32) {
				buf[i] = '0';
				continue;
			}
		}

		if(data.almOut & (1LL << j++))	buf[i] = '1';
		else							buf[i] = '0';
	}
	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);
	if(!nf_sysdb_set_key1("act.sensor.S%d.arout", (ch >= g_ch ? ch - g_ch + 32 : ch), &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.buzzer);
	if(!nf_sysdb_set_key1("act.sensor.S%d.buzzer", (ch >= g_ch ? ch - g_ch + 32 : ch), &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.vpop);
	if(!nf_sysdb_set_key1("act.sensor.S%d.vpopup", (ch >= g_ch ? ch - g_ch + 32 : ch), &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.opop);
	if(!nf_sysdb_set_key1("act.sensor.S%d.opopup", (ch >= g_ch ? ch - g_ch + 32 : ch), &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.email);
	if(!nf_sysdb_set_key1("act.sensor.S%d.email", (ch >= g_ch ? ch - g_ch + 32 : ch), &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.ftp);
	if(!nf_sysdb_set_key1("act.sensor.S%d.ftp", (ch >= g_ch ? ch - g_ch + 32 : ch), &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.preset);
	if(!nf_sysdb_set_key1("act.sensor.S%d.preset", (ch >= g_ch ? ch - g_ch + 32 : ch), &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

    for (j = 0; j < 32; j++)
    {
    	g_value_init(&set_value, G_TYPE_UINT);
    	g_value_set_uint(&set_value, data.preset_num[j]);
    	if(!nf_sysdb_set_key2("act.sensor.S%d.cam.C%d.preset_num", (ch >= g_ch ? ch - g_ch + 32 : ch), j, &set_value, NULL)) {
    		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
    		DDEBUG_FAIL
    		return FALSE;
    	}
    	g_value_unset(&set_value);
    }

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.mobile);
	if(!nf_sysdb_set_key1("act.sensor.S%d.mobile", (ch >= g_ch ? ch - g_ch + 32 : ch), &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.mobilepush);
	if(!nf_sysdb_set_key1("act.sensor.S%d.mobilepush", (ch >= g_ch ? ch - g_ch + 32 : ch), &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);

	return TRUE;
}


gboolean DAL_set_almSen_data_all(EA_AlmSenData *data, guint ch)
{
	GValue set_value = {0,};
	gchar buf[49];
	gint i, j, k;


	nf_sysdb_lock(NF_SYSDB_CATE_ALARM);

	DDEBUG_START

	for(k=0; k<ch; k++) {
		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, data[k].name);
		if(!nf_sysdb_set_key1("alarm.sensor.S%d.name", (k >= g_ch ? k - g_ch + 32 : k), &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ALARM);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		if(data[k].oper == 0) g_value_set_boolean(&set_value, FALSE);
		else			   g_value_set_boolean(&set_value, TRUE);
		if(!nf_sysdb_set_key1("alarm.sensor.S%d.op_type", (k >= g_ch ? k - g_ch + 32 : k), &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ALARM);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);
	}

	nf_sysdb_unlock(NF_SYSDB_CATE_ALARM);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ALARM, 0, 0, 0);



	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	for(k=0; k<ch; k++) {

		memset(buf, 0, sizeof(buf));
		for(i=0; i<32; i++)
		{
			if(data[k].linkCam & (1 << i))	buf[i] = '1';
			else						buf[i] = '0';
		}
		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, buf);
		if(!nf_sysdb_set_key1("act.sensor.S%d.lcamera", (k >= g_ch ? k - g_ch + 32 : k), &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);

		memset(buf, 0, sizeof(buf));
		for(i=0, j=0; i<48; i++)
		{
			if(i >= g_ch) {
				if(i < 32) {
					buf[i] = '0';
					continue;
				}
			}

			if(data[k].almOut & (1LL << j++))	buf[i] = '1';
			else							buf[i] = '0';
		}
		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, buf);
		if(!nf_sysdb_set_key1("act.sensor.S%d.arout", (k >= g_ch ? k - g_ch + 32 : k), &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].buzzer);
		if(!nf_sysdb_set_key1("act.sensor.S%d.buzzer", (k >= g_ch ? k - g_ch + 32 : k), &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].vpop);
		if(!nf_sysdb_set_key1("act.sensor.S%d.vpopup", (k >= g_ch ? k - g_ch + 32 : k), &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].opop);
		if(!nf_sysdb_set_key1("act.sensor.S%d.opopup", (k >= g_ch ? k - g_ch + 32 : k), &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].email);
		if(!nf_sysdb_set_key1("act.sensor.S%d.email", (k >= g_ch ? k - g_ch + 32 : k), &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].ftp);
		if(!nf_sysdb_set_key1("act.sensor.S%d.ftp", (k >= g_ch ? k - g_ch + 32 : k), &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].preset);
		if(!nf_sysdb_set_key1("act.sensor.S%d.preset", (k >= g_ch ? k - g_ch + 32 : k), &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);

        for (j = 0; j < 32; j++)
        {
    		g_value_init(&set_value, G_TYPE_UINT);
    		g_value_set_uint(&set_value, data[k].preset_num[j]);
    		if(!nf_sysdb_set_key2("act.sensor.S%d.cam.C%d.preset_num", (k >= g_ch ? k - g_ch + 32 : k), j, &set_value, NULL))
    		{
    			g_value_unset(&set_value);
    			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
    			DDEBUG_FAIL
				return FALSE;
    		}
    		g_value_unset(&set_value);
        }

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].mobile);
		if(!nf_sysdb_set_key1("act.sensor.S%d.mobile", (k >= g_ch ? k - g_ch + 32 : k), &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].mobilepush);
		if(!nf_sysdb_set_key1("act.sensor.S%d.mobilepush", (k >= g_ch ? k - g_ch + 32 : k), &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);
	}

	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);
	return TRUE;
}


gboolean DAL_get_almSen_data(EA_AlmSenData *data, guint ch)
{
	GValue ret_value = {0,};
	const gchar *str = NULL;
	gint i, j;

	if(nf_sysdb_get_key1("alarm.sensor.S%d.name", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);
		g_stpcpy(data->name, str);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("alarm.sensor.S%d.op_type", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		if(g_value_get_boolean(&ret_value))		data->oper = 1;			// N/C
		else 									data->oper = 0;			// N/O
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.sensor.S%d.lcamera", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);
		for(i=0; i<32; i++)
		{
			if(str[i] == '1')	data->linkCam |= (1 << i);
			else				data->linkCam &= ~(1 << i);
		}
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.sensor.S%d.arout", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);
		for(i=0, j=0; i<48, j<48; i++)
		{
			if(i == g_ch) j = 32;

			if(str[j++] == '1')	data->almOut |= (1LL << i);
			else				data->almOut &= ~(1LL << i);
		}
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.sensor.S%d.buzzer", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		data->buzzer = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.sensor.S%d.vpopup", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		data->vpop = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.sensor.S%d.opopup", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		data->opop = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.sensor.S%d.email", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		data->email = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.sensor.S%d.ftp", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		data->ftp = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.sensor.S%d.preset", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		data->preset = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

    for (j = 0; j < 32; j++)
    {
    	if(nf_sysdb_get_key2("act.sensor.S%d.cam.C%d.preset_num", (ch >= g_ch ? ch - g_ch + 32 : ch), j, &ret_value, NULL))
    	{
    		data->preset_num[j] = g_value_get_uint(&ret_value);
    		g_value_unset(&ret_value);
    	}
    	else
    		return FALSE;
    }

	if(nf_sysdb_get_key1("act.sensor.S%d.mobile", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		data->mobile = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.sensor.S%d.mobilepush", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		data->mobilepush = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	return TRUE;
}


gboolean DAL_get_almSen_vPop_boolean(guint ch)
{
	GValue ret_value = {0,};
	gboolean ret = FALSE;

	if(nf_sysdb_get_key1("act.sensor.S%d.vpopup", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		ret = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}

	return ret;
}


gboolean DAL_get_almSen_oPop_boolean(guint ch)
{
	GValue ret_value = {0,};
	gboolean ret = FALSE;

	if(nf_sysdb_get_key1("act.sensor.S%d.opopup", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		ret = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}

	return ret;
}

gboolean DAL_get_almSen_name(guint ch, gchar buf[])
{
	GValue ret_value = {0,};
	const gchar *str = NULL;

	if(nf_sysdb_get_key1("alarm.sensor.S%d.name", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);
		g_stpcpy(buf, str);
		g_value_unset(&ret_value);

		if(strlen(buf))
			return TRUE;
	}

	return FALSE;
}

guint DAL_get_almSen_linkCam(guint ch)
{
	GValue ret_value = {0,};
	const gchar *str = NULL;
	guint ret_mask = 0;
	gint i;

	if(nf_sysdb_get_key1("act.sensor.S%d.lcamera", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);
		for(i=0; i<32; i++)
		{
			if(str[i] == '1')
				ret_mask |= (1 << i);
		}
		g_value_unset(&ret_value);
	}

	return ret_mask;
}

/***************************************************************
 * Setup --> Event&Action --> Motion Sensor
 * *************************************************************/
gboolean DAL_set_MotSen_data(EA_MotSenData data, guint ch)
{
	GValue set_value = {0,};
	gchar buf[49];
	gint i, j;

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	DDEBUG_START

	g_value_init(&set_value, G_TYPE_INT);
	g_value_set_int(&set_value, data.interval);
	if(!nf_sysdb_set_key1("act.motion.M%d.interval", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	for(i=0, j=0; i<48; i++)
	{
		if(i >= g_ch) {
			if(i < 32) {
				buf[i] = '0';
				continue;
			}
		}

		if(data.almOut & (1LL << j++))	buf[i] = '1';
		else						buf[i] = '0';
	}
	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);
	if(!nf_sysdb_set_key1("act.motion.M%d.arout", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.buzzer);
	if(!nf_sysdb_set_key1("act.motion.M%d.buzzer", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.vpop);
	if(!nf_sysdb_set_key1("act.motion.M%d.vpopup", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.email);
	if(!nf_sysdb_set_key1("act.motion.M%d.email", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.ftp);
	if(!nf_sysdb_set_key1("act.motion.M%d.ftp", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.preset);
	if(!nf_sysdb_set_key1("act.motion.M%d.preset", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

    for (j = 0; j < 32; j++)
    {
    	g_value_init(&set_value, G_TYPE_UINT);
    	g_value_set_uint(&set_value, data.preset_num[j]);
    	if(!nf_sysdb_set_key2("act.motion.M%d.cam.C%d.preset_num", ch, j, &set_value, NULL))
    	{
    		g_value_unset(&set_value);
    		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
    		DDEBUG_FAIL
    		return FALSE;
    	}
    	g_value_unset(&set_value);

    }

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.mobile);
	if(!nf_sysdb_set_key1("act.motion.M%d.mobile", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.mobilepush);
	if(!nf_sysdb_set_key1("act.motion.M%d.mobilepush", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);

	return TRUE;
}

gboolean DAL_set_MotSen_data_all(EA_MotSenData *data, guint ch)
{
	GValue set_value = {0,};
	gchar buf[49];
	gint i, j, k;

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	DDEBUG_START

	for(k=0; k<ch; k++) {
		g_value_init(&set_value, G_TYPE_INT);
		g_value_set_int(&set_value, data[k].interval);
		if(!nf_sysdb_set_key1("act.motion.M%d.interval", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
			return FALSE;
		}
		g_value_unset(&set_value);

		memset(buf, 0, sizeof(buf));
		for(i=0, j=0; i<48; i++)
		{
			if(i >= g_ch) {
				if(i < 32) {
					buf[i] = '0';
					continue;
				}
			}

			if(data[k].almOut & (1LL << j++))	buf[i] = '1';
			else						buf[i] = '0';
		}
		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, buf);
		if(!nf_sysdb_set_key1("act.motion.M%d.arout", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
			return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].buzzer);
		if(!nf_sysdb_set_key1("act.motion.M%d.buzzer", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
			return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].vpop);
		if(!nf_sysdb_set_key1("act.motion.M%d.vpopup", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
			return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].email);
		if(!nf_sysdb_set_key1("act.motion.M%d.email", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
			return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].ftp);
		if(!nf_sysdb_set_key1("act.motion.M%d.ftp", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
			return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].preset);
		if(!nf_sysdb_set_key1("act.motion.M%d.preset", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
			return FALSE;
		}
		g_value_unset(&set_value);

        for (j = 0; j < 32; j++)
        {
        	g_value_init(&set_value, G_TYPE_UINT);
        	g_value_set_uint(&set_value, data[k].preset_num[j]);
        	if(!nf_sysdb_set_key2("act.motion.M%d.cam.C%d.preset_num", k, j, &set_value, NULL))
        	{
        		g_value_unset(&set_value);
        		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
        		DDEBUG_FAIL
        		return FALSE;
        	}
        	g_value_unset(&set_value);

        }

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].mobile);
		if(!nf_sysdb_set_key1("act.motion.M%d.mobile", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
			return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].mobilepush);
		if(!nf_sysdb_set_key1("act.motion.M%d.mobilepush", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
			return FALSE;
		}
		g_value_unset(&set_value);
	}

	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);

	return TRUE;
}

gboolean DAL_get_MotSen_data(EA_MotSenData *data, guint ch)
{
	GValue ret_value = {0,};
	const gchar *str = NULL;
	gint i, j;

	if(nf_sysdb_get_key1("act.motion.M%d.interval", ch, &ret_value, NULL))
	{
		data->interval = g_value_get_int(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.motion.M%d.arout", ch, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);
		for(i=0, j=0; i<48, j<48; i++)
		{
			if(i == g_ch) j = 32;

			if(str[j++] == '1')	data->almOut |= (1LL << i);
			else				data->almOut &= ~(1LL << i);
		}
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.motion.M%d.buzzer", ch, &ret_value, NULL))
	{
		data->buzzer = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.motion.M%d.vpopup", ch, &ret_value, NULL))
	{
		data->vpop = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.motion.M%d.email", ch, &ret_value, NULL))
	{
		data->email = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.motion.M%d.ftp", ch, &ret_value, NULL))
	{
		data->ftp = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.motion.M%d.preset", ch, &ret_value, NULL))
	{
		data->preset = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

    for (j = 0; j < 32; j++)
    {
    	if(nf_sysdb_get_key2("act.motion.M%d.cam.C%d.preset_num", ch, j, &ret_value, NULL))
    	{
    		data->preset_num[j] = g_value_get_uint(&ret_value);
    		g_value_unset(&ret_value);
    	}
    	else
    		return FALSE;
    }

	if(nf_sysdb_get_key1("act.motion.M%d.mobile", ch, &ret_value, NULL))
	{
		data->mobile = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.motion.M%d.mobilepush", ch, &ret_value, NULL))
	{
		data->mobilepush = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	return TRUE;
}

gboolean DAL_get_MotSen_vPop_boolean(guint ch)
{
	GValue ret_value = {0,};
	gboolean ret = FALSE;

	if(nf_sysdb_get_key1("act.motion.M%d.vpopup", ch, &ret_value, NULL))
	{
		ret = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}

	return ret;
}

gint DAL_get_MotSen_ignor_interval(guint ch)
{
	GValue ret_value = {0,};
	gint ret = 0;

	if(nf_sysdb_get_key1("act.motion.M%d.interval", ch, &ret_value, NULL))
	{
		ret = g_value_get_int(&ret_value);
		g_value_unset(&ret_value);
	}

	return ret;
}

/***************************************************************
 * Setup --> Event&Action --> Video Loss
 * *************************************************************/
gboolean DAL_set_VLoss_Data(EA_VLossData data, guint ch)
{
	GValue set_value = {0,};
	gchar buf[49];
	gint i, j;

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	DDEBUG_START

	memset(buf, 0, sizeof(buf));
	for(i=0, j=0; i<48; i++)
	{
		if(i >= g_ch) {
			if(i < 32) {
				buf[i] = '0';
				continue;
			}
		}

		if(data.almOut & (1LL << j++))	buf[i] = '1';
		else							buf[i] = '0';
	}
	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);
	if(!nf_sysdb_set_key1("act.vloss.V%d.arout", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.buzzer);
	if(!nf_sysdb_set_key1("act.vloss.V%d.buzzer", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.email);
	if(!nf_sysdb_set_key1("act.vloss.V%d.email", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.ftp);
	if(!nf_sysdb_set_key1("act.vloss.V%d.ftp", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.preset);
	if(!nf_sysdb_set_key1("act.vloss.V%d.preset", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

    for (j = 0; j < 32; j++)
    {
    	g_value_init(&set_value, G_TYPE_UINT);
    	g_value_set_uint(&set_value, data.preset_num[j]);
    	if(!nf_sysdb_set_key2("act.vloss.V%d.cam.C%d.preset_num", ch, j, &set_value, NULL))
    	{
    		g_value_unset(&set_value);
    		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
    		DDEBUG_FAIL
    		return FALSE;
    	}
    	g_value_unset(&set_value);
    }

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.mobile);
	if(!nf_sysdb_set_key1("act.vloss.V%d.mobile", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.mobilepush);
	if(!nf_sysdb_set_key1("act.vloss.V%d.mobilepush", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);

	return TRUE;
}

gboolean DAL_set_VLoss_Data_all(EA_VLossData *data, guint ch)
{
	GValue set_value = {0,};
	gchar buf[49];
	gint i, j, k;

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	DDEBUG_START

	for(k=0; k<ch; k++) {
		memset(buf, 0, sizeof(buf));
		for(i=0, j=0; i<48; i++)
		{
			if(i >= g_ch) {
				if(i < 32) {
					buf[i] = '0';
					continue;
				}
			}

			if(data[k].almOut & (1LL << j++))	buf[i] = '1';
			else							buf[i] = '0';
		}
		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, buf);
		if(!nf_sysdb_set_key1("act.vloss.V%d.arout", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].buzzer);
		if(!nf_sysdb_set_key1("act.vloss.V%d.buzzer", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].email);
		if(!nf_sysdb_set_key1("act.vloss.V%d.email", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].ftp);
		if(!nf_sysdb_set_key1("act.vloss.V%d.ftp", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].preset);
		if(!nf_sysdb_set_key1("act.vloss.V%d.preset", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);

        for (j = 0; j < 32; j++)
        {
        	g_value_init(&set_value, G_TYPE_UINT);
        	g_value_set_uint(&set_value, data[k].preset_num[j]);
        	if(!nf_sysdb_set_key2("act.vloss.V%d.cam.C%d.preset_num", k, j, &set_value, NULL))
        	{
        		g_value_unset(&set_value);
        		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
        		DDEBUG_FAIL
        		return FALSE;
        	}
        	g_value_unset(&set_value);
        }

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].mobile);
		if(!nf_sysdb_set_key1("act.vloss.V%d.mobile", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].mobilepush);
		if(!nf_sysdb_set_key1("act.vloss.V%d.mobilepush", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);
	}

	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);

	return TRUE;
}


gboolean DAL_get_VLoss_Data(EA_VLossData *data, guint ch)
{
	GValue ret_value = {0,};
	const gchar *str = NULL;
	gint i, j;

	if(nf_sysdb_get_key1("act.vloss.V%d.arout", ch, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);
		for(i=0, j=0; i<48, j<48; i++)
		{
			if(i == g_ch) j = 32;

			if(str[j++] == '1')	data->almOut |= (1LL << i);
			else				data->almOut &= ~(1LL << i);
		}
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.vloss.V%d.buzzer", ch, &ret_value, NULL))
	{
		data->buzzer = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.vloss.V%d.email", ch, &ret_value, NULL))
	{
		data->email = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.vloss.V%d.ftp", ch, &ret_value, NULL))
	{
		data->ftp = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.vloss.V%d.preset", ch, &ret_value, NULL))
	{
		data->preset = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

    for (j = 0; j < 32; j++)
    {
    	if(nf_sysdb_get_key2("act.vloss.V%d.cam.C%d.preset_num", ch, j, &ret_value, NULL))
    	{
    		data->preset_num[j] = g_value_get_uint(&ret_value);
    		g_value_unset(&ret_value);
    	}
    	else
    		return FALSE;
    }

	if(nf_sysdb_get_key1("act.vloss.V%d.mobile", ch, &ret_value, NULL))
	{
		data->mobile = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.vloss.V%d.mobilepush", ch, &ret_value, NULL))
	{
		data->mobilepush = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	return TRUE;
}


/***************************************************************
 * Setup --> Event&Action --> AI Detection Event
 * *************************************************************/
gboolean DAL_set_dva_event_data(EA_DvaData data, guint ch)
{
	GValue set_value = {0,};
	gchar buf[49];
	gint i, j;

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	DDEBUG_START

	memset(buf, 0, sizeof(buf));
	for(i=0, j=0; i<48; i++)
	{
		if(i >= g_ch) {
			if(i < 32) {
				buf[i] = '0';
				continue;
			}
		}

		if(data.almOut & (1LL << j++))	buf[i] = '1';
		else							buf[i] = '0';
	}
	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);
	if(!nf_sysdb_set_key1("act.dva.D%d.arout", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.buzzer);
	if(!nf_sysdb_set_key1("act.dva.D%d.buzzer", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.email);
	if(!nf_sysdb_set_key1("act.dva.D%d.email", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.ftp);
	if(!nf_sysdb_set_key1("act.dva.D%d.ftp", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.preset);
	if(!nf_sysdb_set_key1("act.dva.D%d.preset", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

    for (j = 0; j < 32; j++)
    {
    	g_value_init(&set_value, G_TYPE_UINT);
    	g_value_set_uint(&set_value, data.preset_num[j]);
    	if(!nf_sysdb_set_key2("act.dva.D%d.cam.C%d.preset_num", ch, j, &set_value, NULL))
    	{
    		g_value_unset(&set_value);
    		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
    		DDEBUG_FAIL
    		return FALSE;
    	}
    	g_value_unset(&set_value);
    }

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.mobile);
	if(!nf_sysdb_set_key1("act.dva.D%d.mobile", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.mobilepush);
	if(!nf_sysdb_set_key1("act.dva.D%d.mobilepush", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);

	return TRUE;
}

gboolean DAL_set_dva_event_data_all(EA_DvaData *data, guint ch)
{
	GValue set_value = {0,};
	gchar buf[49];
	gint i, j, k;

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	DDEBUG_START

	for(k=0; k<ch; k++) {
		memset(buf, 0, sizeof(buf));
		for(i=0, j=0; i<48; i++)
		{
			if(i >= g_ch) {
				if(i < 32) {
					buf[i] = '0';
					continue;
				}
			}

			if(data[k].almOut & (1LL << j++))	buf[i] = '1';
			else							buf[i] = '0';
		}
		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, buf);
		if(!nf_sysdb_set_key1("act.dva.D%d.arout", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].buzzer);
		if(!nf_sysdb_set_key1("act.dva.D%d.buzzer", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].email);
		if(!nf_sysdb_set_key1("act.dva.D%d.email", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].ftp);
		if(!nf_sysdb_set_key1("act.dva.D%d.ftp", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].preset);
		if(!nf_sysdb_set_key1("act.dva.D%d.preset", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);

        for (j = 0; j < 32; j++)
        {
        	g_value_init(&set_value, G_TYPE_UINT);
        	g_value_set_uint(&set_value, data[k].preset_num[j]);
        	if(!nf_sysdb_set_key2("act.dva.D%d.cam.C%d.preset_num", k, j, &set_value, NULL))
        	{
        		g_value_unset(&set_value);
        		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
        		DDEBUG_FAIL
        		return FALSE;
        	}
        	g_value_unset(&set_value);
        }

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].mobile);
		if(!nf_sysdb_set_key1("act.dva.D%d.mobile", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].mobilepush);
		if(!nf_sysdb_set_key1("act.dva.D%d.mobilepush", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);
	}

	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);

	return TRUE;
}


gboolean DAL_get_dva_event_data(EA_DvaData *data, guint ch)
{
	GValue ret_value = {0,};
	const gchar *str = NULL;
	gint i, j;

	if(nf_sysdb_get_key1("act.dva.D%d.arout", ch, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);
		for(i=0, j=0; i<48, j<48; i++)
		{
			if(i == g_ch) j = 32;

			if(str[j++] == '1')	data->almOut |= (1LL << i);
			else				data->almOut &= ~(1LL << i);
		}
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.dva.D%d.buzzer", ch, &ret_value, NULL))
	{
		data->buzzer = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.dva.D%d.email", ch, &ret_value, NULL))
	{
		data->email = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.dva.D%d.ftp", ch, &ret_value, NULL))
	{
		data->ftp = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.dva.D%d.preset", ch, &ret_value, NULL))
	{
		data->preset = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

    for (j = 0; j < 32; j++)
    {
    	if(nf_sysdb_get_key2("act.dva.D%d.cam.C%d.preset_num", ch, j, &ret_value, NULL))
    	{
    		data->preset_num[j] = g_value_get_uint(&ret_value);
    		g_value_unset(&ret_value);
    	}
    	else
    		return FALSE;
    }

	if(nf_sysdb_get_key1("act.dva.D%d.mobile", ch, &ret_value, NULL))
	{
		data->mobile = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.dva.D%d.mobilepush", ch, &ret_value, NULL))
	{
		data->mobilepush = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	return TRUE;
}

/***************************************************************
 * Setup --> Event&Action --> POS/ATM
 * *************************************************************/
gboolean DAL_set_posevent_data(EA_PosData data, guint ch)
{
	GValue set_value = {0,};
	gchar buf[49];
	gint i, j;

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	DDEBUG_START

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data.text);
	if(!nf_sysdb_set_key1("act.pos.P%d.text", (ch >= g_ch ? ch - g_ch + 32 : ch), &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	for(i=0; i<32; i++)
	{
		if(data.linkCam & (1 << i))	buf[i] = '1';
		else						buf[i] = '0';
	}
	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);
	if(!nf_sysdb_set_key1("act.pos.P%d.lcamera", (ch >= g_ch ? ch - g_ch + 32 : ch), &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	for(i=0, j=0; i<48; i++)
	{
		if(i >= g_ch) {
			if(i < 32) {
				buf[i] = '0';
				continue;
			}
		}

		if(data.almOut & (1LL << j++))	buf[i] = '1';
		else							buf[i] = '0';
	}
	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);
	if(!nf_sysdb_set_key1("act.pos.P%d.arout", (ch >= g_ch ? ch - g_ch + 32 : ch), &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.buzzer);
	if(!nf_sysdb_set_key1("act.pos.P%d.buzzer", (ch >= g_ch ? ch - g_ch + 32 : ch), &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.vpop);
	if(!nf_sysdb_set_key1("act.pos.P%d.vpopup", (ch >= g_ch ? ch - g_ch + 32 : ch), &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.opop);
	if(!nf_sysdb_set_key1("act.pos.P%d.opopup", (ch >= g_ch ? ch - g_ch + 32 : ch), &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.email);
	if(!nf_sysdb_set_key1("act.pos.P%d.email", (ch >= g_ch ? ch - g_ch + 32 : ch), &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.ftp);
	if(!nf_sysdb_set_key1("act.pos.P%d.ftp", (ch >= g_ch ? ch - g_ch + 32 : ch), &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.preset);
	if(!nf_sysdb_set_key1("act.pos.P%d.preset", (ch >= g_ch ? ch - g_ch + 32 : ch), &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

    for (j = 0; j < 32; j++)
    {
    	g_value_init(&set_value, G_TYPE_UINT);
    	g_value_set_uint(&set_value, data.preset_num[j]);
    	if(!nf_sysdb_set_key2("act.pos.P%d.cam.C%d.preset_num", (ch >= g_ch ? ch - g_ch + 32 : ch), j, &set_value, NULL)) {
    		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
    		DDEBUG_FAIL
    		return FALSE;
    	}
    	g_value_unset(&set_value);
    }

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.mobile);
	if(!nf_sysdb_set_key1("act.pos.P%d.mobile", (ch >= g_ch ? ch - g_ch + 32 : ch), &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.mobilepush);
	if(!nf_sysdb_set_key1("act.pos.P%d.mobilepush", (ch >= g_ch ? ch - g_ch + 32 : ch), &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);
	return TRUE;
}

gboolean DAL_set_posevent_data_all(EA_PosData *data, guint ch)
{
	GValue set_value = {0,};
	gchar buf[49];
	gint i, j, k;

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	for(k=0; k<ch; k++) {

		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, data[k].text);
		if(!nf_sysdb_set_key1("act.pos.P%d.text", (k >= g_ch ? k - g_ch + 32 : k), &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);

		memset(buf, 0, sizeof(buf));
		for(i=0; i<32; i++)
		{
			if(data[k].linkCam & (1 << i))	buf[i] = '1';
			else						buf[i] = '0';
		}
		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, buf);
		if(!nf_sysdb_set_key1("act.pos.P%d.lcamera", (k >= g_ch ? k - g_ch + 32 : k), &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);

		memset(buf, 0, sizeof(buf));
		for(i=0, j=0; i<48; i++)
		{
			if(i >= g_ch) {
				if(i < 32) {
					buf[i] = '0';
					continue;
				}
			}

			if(data[k].almOut & (1LL << j++))	buf[i] = '1';
			else							buf[i] = '0';
		}
		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, buf);
		if(!nf_sysdb_set_key1("act.pos.P%d.arout", (k >= g_ch ? k - g_ch + 32 : k), &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].buzzer);
		if(!nf_sysdb_set_key1("act.pos.P%d.buzzer", (k >= g_ch ? k - g_ch + 32 : k), &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].vpop);
		if(!nf_sysdb_set_key1("act.pos.P%d.vpopup", (k >= g_ch ? k - g_ch + 32 : k), &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].opop);
		if(!nf_sysdb_set_key1("act.pos.P%d.opopup", (k >= g_ch ? k - g_ch + 32 : k), &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].email);
		if(!nf_sysdb_set_key1("act.pos.P%d.email", (k >= g_ch ? k - g_ch + 32 : k), &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].ftp);
		if(!nf_sysdb_set_key1("act.pos.P%d.ftp", (k >= g_ch ? k - g_ch + 32 : k), &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].preset);
		if(!nf_sysdb_set_key1("act.pos.P%d.preset", (k >= g_ch ? k - g_ch + 32 : k), &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);

        for (j = 0; j < 32; j++)
        {
    		g_value_init(&set_value, G_TYPE_UINT);
    		g_value_set_uint(&set_value, data[k].preset_num[j]);
    		if(!nf_sysdb_set_key2("act.pos.P%d.cam.C%d.preset_num", (k >= g_ch ? k - g_ch + 32 : k), j, &set_value, NULL))
    		{
    			g_value_unset(&set_value);
    			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
    			DDEBUG_FAIL
				return FALSE;
    		}
    		g_value_unset(&set_value);
        }

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].mobile);
		if(!nf_sysdb_set_key1("act.pos.P%d.mobile", (k >= g_ch ? k - g_ch + 32 : k), &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].mobilepush);
		if(!nf_sysdb_set_key1("act.pos.P%d.mobilepush", (k >= g_ch ? k - g_ch + 32 : k), &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
				return FALSE;
		}
		g_value_unset(&set_value);
	}

	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);
	return TRUE;
}

gboolean DAL_get_posevent_data(EA_PosData *data, guint ch)
{
	GValue ret_value = {0,};
	const gchar *str = NULL;
	gint i, j;

	if(nf_sysdb_get_key1("act.pos.P%d.text", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);
		g_stpcpy(data->text, str);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.pos.P%d.lcamera", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);
		for(i=0; i<32; i++)
		{
			if(str[i] == '1')	data->linkCam |= (1 << i);
			else				data->linkCam &= ~(1 << i);
		}
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.pos.P%d.arout", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);
		for(i=0, j=0; i<48, j<48; i++)
		{
			if(i == g_ch) j = 32;

			if(str[j++] == '1')	data->almOut |= (1LL << i);
			else				data->almOut &= ~(1LL << i);
		}
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.pos.P%d.buzzer", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		data->buzzer = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.pos.P%d.vpopup", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		data->vpop = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.pos.P%d.opopup", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		data->opop = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.pos.P%d.email", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		data->email = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.pos.P%d.ftp", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		data->ftp = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.pos.P%d.preset", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		data->preset = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

    for (j = 0; j < 32; j++)
    {
    	if(nf_sysdb_get_key2("act.pos.P%d.cam.C%d.preset_num", (ch >= g_ch ? ch - g_ch + 32 : ch), j, &ret_value, NULL))
    	{
    		data->preset_num[j] = g_value_get_uint(&ret_value);
    		g_value_unset(&ret_value);
    	}
    	else
    		return FALSE;
    }

	if(nf_sysdb_get_key1("act.pos.P%d.mobile", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		data->mobile = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.pos.P%d.mobilepush", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		data->mobilepush = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	return TRUE;
}


gboolean DAL_get_posevent_vPop_boolean(guint ch)
{
	GValue ret_value = {0,};
	gboolean ret = FALSE;

	if(nf_sysdb_get_key1("act.pos.P%d.vpopup", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		ret = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}

	return ret;
}


gboolean DAL_get_posevent_oPop_boolean(guint ch)
{
	GValue ret_value = {0,};
	gboolean ret = FALSE;

	if(nf_sysdb_get_key1("act.pos.P%d.opopup", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		ret = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}

	return ret;
}

guint DAL_get_posevent_linkCam(guint ch)
{
	GValue ret_value = {0,};
	const gchar *str = NULL;
	guint ret_mask = 0;
	gint i;

	if(nf_sysdb_get_key1("act.pos.P%d.lcamera", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);
		for(i=0; i<32; i++)
		{
			if(str[i] == '1')
				ret_mask |= (1 << i);
		}
		g_value_unset(&ret_value);
	}

	return ret_mask;
}


/***************************************************************
 * Setup --> Event&Action --> AI Analysis Event
 * *************************************************************/

gboolean DAL_get_analysis_vPop_boolean(guint ch)
{
	GValue ret_value = {0,};
	gboolean ret = FALSE;

	if(nf_sysdb_get_key1("act.vca.V%d.vpopup", ch, &ret_value, NULL))
	{
		ret = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}

	return ret;
}

gboolean DAL_get_analysis_event_data(EA_AnalysisData *data, guint ch)
{
	GValue ret_value = {0,};
	const gchar *str = NULL;
	gint i, j;

	if(nf_sysdb_get_key1("act.vca.V%d.interval", ch, &ret_value, NULL))
	{
		data->interval = g_value_get_int(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.vca.V%d.arout", ch, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);
		for(i=0, j=0; i<48, j<48; i++)
		{
			if(i == g_ch) j = 32;

			if(str[j++] == '1')	data->almOut |= (1LL << i);
			else				data->almOut &= ~(1LL << i);
		}
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.vca.V%d.buzzer", ch, &ret_value, NULL))
	{
		data->buzzer = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.vca.V%d.vpopup", ch, &ret_value, NULL))
	{
		data->vpop = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.vca.V%d.email", ch, &ret_value, NULL))
	{
		data->email = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.vca.V%d.ftp", ch, &ret_value, NULL))
	{
		data->ftp = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.vca.V%d.preset", ch, &ret_value, NULL))
	{
		data->preset = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

    for (j = 0; j < 32; j++)
    {
    	if(nf_sysdb_get_key2("act.vca.V%d.cam.C%d.preset_num", ch, j, &ret_value, NULL))
    	{
    		data->preset_num[j] = g_value_get_uint(&ret_value);
    		g_value_unset(&ret_value);
    	}
    	else
    		return FALSE;
    }

	if(nf_sysdb_get_key1("act.vca.V%d.mobile", ch, &ret_value, NULL))
	{
		data->mobile = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.vca.V%d.mobilepush", ch, &ret_value, NULL))
	{
		data->mobilepush = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.vca.V%d.doubleknock", ch, &ret_value, NULL))
	{
		data->doubleknock = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	return TRUE;
}

gboolean DAL_set_analysis_event_data(EA_AnalysisData data, guint ch)
{
	GValue set_value = {0,};
	gchar buf[49];
	gint i, j;

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	DDEBUG_START

	g_value_init(&set_value, G_TYPE_INT);
	g_value_set_int(&set_value, data.interval);
	nf_sysdb_set_key1("act.vca.V%d.interval", ch, &set_value, NULL);
	nf_sysdb_set_key1("act.dvabx.D%d.interval", ch, &set_value, NULL);
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	for(i=0, j=0; i<48; i++)
	{
		if (i >= g_ch) 
		{
			if(i < 32) {
				buf[i] = '0';
				continue;
			}
		}

		if (data.almOut & (1LL << j++)) buf[i] = '1';
		else buf[i] = '0';
	}

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);
	nf_sysdb_set_key1("act.vca.V%d.arout", ch, &set_value, NULL);
	nf_sysdb_set_key1("act.dva.D%d.arout", ch, &set_value, NULL);
	nf_sysdb_set_key1("act.dvabx.D%d.arout", ch, &set_value, NULL);
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.buzzer);
	nf_sysdb_set_key1("act.vca.V%d.buzzer", ch, &set_value, NULL);
	nf_sysdb_set_key1("act.dva.D%d.buzzer", ch, &set_value, NULL);
	nf_sysdb_set_key1("act.dvabx.D%d.buzzer", ch, &set_value, NULL);
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.vpop);
	nf_sysdb_set_key1("act.vca.V%d.vpopup", ch, &set_value, NULL);
	nf_sysdb_set_key1("act.dvabx.D%d.vpopup", ch, &set_value, NULL);
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.email);
	nf_sysdb_set_key1("act.vca.V%d.email", ch, &set_value, NULL);
	nf_sysdb_set_key1("act.dva.D%d.email", ch, &set_value, NULL);
	nf_sysdb_set_key1("act.dvabx.D%d.email", ch, &set_value, NULL);
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.ftp);
	nf_sysdb_set_key1("act.vca.V%d.ftp", ch, &set_value, NULL);
	nf_sysdb_set_key1("act.dva.D%d.ftp", ch, &set_value, NULL);
	nf_sysdb_set_key1("act.dvabx.D%d.ftp", ch, &set_value, NULL);
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.preset);
	nf_sysdb_set_key1("act.vca.V%d.preset", ch, &set_value, NULL);
	nf_sysdb_set_key1("act.dva.D%d.preset", ch, &set_value, NULL);
	nf_sysdb_set_key1("act.dvabx.D%d.preset", ch, &set_value, NULL);
	g_value_unset(&set_value);

    for (j = 0; j < 32; j++)
    {
    	g_value_init(&set_value, G_TYPE_UINT);
    	g_value_set_uint(&set_value, data.preset_num[j]);
    	nf_sysdb_set_key2("act.vca.V%d.cam.C%d.preset_num", ch, j, &set_value, NULL);
		nf_sysdb_set_key2("act.dva.D%d.cam.C%d.preset_num", ch, j, &set_value, NULL);
		nf_sysdb_set_key2("act.dvabx.D%d.cam.C%d.preset_num", ch, j, &set_value, NULL);
    	g_value_unset(&set_value);
    }

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.mobile);
	nf_sysdb_set_key1("act.vca.V%d.mobile", ch, &set_value, NULL);
	nf_sysdb_set_key1("act.dva.D%d.mobile", ch, &set_value, NULL);
	nf_sysdb_set_key1("act.dvabx.D%d.mobile", ch, &set_value, NULL);
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.mobilepush);
	nf_sysdb_set_key1("act.vca.V%d.mobilepush", ch, &set_value, NULL);
	nf_sysdb_set_key1("act.dva.D%d.mobilepush", ch, &set_value, NULL);
	nf_sysdb_set_key1("act.dvabx.D%d.mobilepush", ch, &set_value, NULL);
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);

	return TRUE;
}

gboolean DAL_set_analysis_event_data_all(EA_AnalysisData *data, guint ch)
{
	GValue set_value = {0,};
	gchar buf[49];
	gint i, j, k;

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	DDEBUG_START

	for(k = 0; k < ch; k++) 
	{
		g_value_init(&set_value, G_TYPE_INT);
		g_value_set_int(&set_value, data[k].interval);
		nf_sysdb_set_key1("act.vca.V%d.interval", k, &set_value, NULL);
		nf_sysdb_set_key1("act.dvabx.D%d.interval", k, &set_value, NULL);
		g_value_unset(&set_value);

		memset(buf, 0, sizeof(buf));
		for(i=0, j=0; i<48; i++)
		{
			if (i >= g_ch) 
			{
				if(i < 32) {
					buf[i] = '0';
					continue;
				}
			}

			if (data[k].almOut & (1LL << j++)) buf[i] = '1';
			else buf[i] = '0';
		}

		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, buf);
		nf_sysdb_set_key1("act.vca.V%d.arout", k, &set_value, NULL);
		nf_sysdb_set_key1("act.dva.D%d.arout", k, &set_value, NULL);
		nf_sysdb_set_key1("act.dvabx.D%d.arout", k, &set_value, NULL);
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].buzzer);
		nf_sysdb_set_key1("act.vca.V%d.buzzer", k, &set_value, NULL);
		nf_sysdb_set_key1("act.dva.D%d.buzzer", k, &set_value, NULL);
		nf_sysdb_set_key1("act.dvabx.D%d.buzzer", k, &set_value, NULL);
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].vpop);
		nf_sysdb_set_key1("act.vca.V%d.vpopup", k, &set_value, NULL);
		nf_sysdb_set_key1("act.dvabx.D%d.vpopup", k, &set_value, NULL);
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].email);
		nf_sysdb_set_key1("act.vca.V%d.email", k, &set_value, NULL);
		nf_sysdb_set_key1("act.dva.D%d.email", k, &set_value, NULL);
		nf_sysdb_set_key1("act.dvabx.D%d.email", k, &set_value, NULL);
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].ftp);
		nf_sysdb_set_key1("act.vca.V%d.ftp", k, &set_value, NULL);
		nf_sysdb_set_key1("act.dva.D%d.ftp", k, &set_value, NULL);
		nf_sysdb_set_key1("act.dvabx.D%d.ftp", k, &set_value, NULL);
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].preset);
		nf_sysdb_set_key1("act.vca.V%d.preset", k, &set_value, NULL);
		nf_sysdb_set_key1("act.dva.D%d.preset", k, &set_value, NULL);
		nf_sysdb_set_key1("act.dvabx.D%d.preset", k, &set_value, NULL);
		g_value_unset(&set_value);

		for (j = 0; j < 32; j++)
		{
			g_value_init(&set_value, G_TYPE_UINT);
			g_value_set_uint(&set_value, data[k].preset_num[j]);
			nf_sysdb_set_key2("act.vca.V%d.cam.C%d.preset_num", k, j, &set_value, NULL);
			nf_sysdb_set_key2("act.dva.D%d.cam.C%d.preset_num", k, j, &set_value, NULL);
			nf_sysdb_set_key2("act.dvabx.D%d.cam.C%d.preset_num", k, j, &set_value, NULL);
			g_value_unset(&set_value);
		}

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].mobile);
		nf_sysdb_set_key1("act.vca.V%d.mobile", k, &set_value, NULL);
		nf_sysdb_set_key1("act.dva.D%d.mobile", k, &set_value, NULL);
		nf_sysdb_set_key1("act.dvabx.D%d.mobile", k, &set_value, NULL);
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].mobilepush);
		nf_sysdb_set_key1("act.vca.V%d.mobilepush", k, &set_value, NULL);
		nf_sysdb_set_key1("act.dva.D%d.mobilepush", k, &set_value, NULL);
		nf_sysdb_set_key1("act.dvabx.D%d.mobilepush", k, &set_value, NULL);
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].doubleknock);
		nf_sysdb_set_key1("act.vca.V%d.doubleknock", k, &set_value, NULL);
		g_value_unset(&set_value);
	}

	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);

	return TRUE;
}

guint DAL_get_double_knock_data()
{
	GValue ret_value = {0,};
	guint ret = 0;

	if(nf_sysdb_get_key0("act.event.double_knock", &ret_value, NULL))
	{
		ret = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}

	return ret;
}

gboolean DAL_set_double_knock_data(guint double_knock)
{
	GValue set_value = {0,};

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);
	
	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, double_knock);
	if(!nf_sysdb_set_key0("act.event.double_knock", &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);

	return TRUE;
}

/***************************************************************
 * Setup --> Event&Action --> System Event
 * *************************************************************/
gboolean DAL_set_SysDisk_data(EA_SysDiskData data, SysDiskDataType data_type)
{
	GValue set_value = {0,};
	gchar buf[49];
	gint i, j;

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	DDEBUG_START

	switch(data_type) {
		case OW_START_EVT_DATA:
			{
				memset(buf, 0, sizeof(buf));
				for(i=0, j=0; i<48; i++)
				{
					if(i >= g_ch) {
						if(i < 32) {
							buf[i] = '0';
							continue;
						}
					}

					if(data.almOut & (1LL << j++))	buf[i] = '1';
					else							buf[i] = '0';
				}
				//g_printf("%s :::::::::::::::::::: buf %s \n", __FUNCTION__, buf);
				g_value_init(&set_value, G_TYPE_STRING);
				g_value_set_string(&set_value, buf);
				if(!nf_sysdb_set_key0("act.sys.disk.over.arout", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.buzzer);
				if(!nf_sysdb_set_key0("act.sys.disk.over.buzzer", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.opop);
				if(!nf_sysdb_set_key0("act.sys.disk.over.osdpopup", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.email);
				if(!nf_sysdb_set_key0("act.sys.disk.over.email", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.ftp);
				if(!nf_sysdb_set_key0("act.sys.disk.over.ftp", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.mobile);
				if(!nf_sysdb_set_key0("act.sys.disk.over.mobile", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.mobilepush);
				if(!nf_sysdb_set_key0("act.sys.disk.over.mobilepush", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);
			}
			break;

		case DISK_FULL_EVT_DATA:
			{
				memset(buf, 0, sizeof(buf));
				for(i=0, j=0; i<48; i++)
				{
					if(i >= g_ch) {
						if(i < 32) {
							buf[i] = '0';
							continue;
						}
					}

					if(data.almOut & (1LL << j++))	buf[i] = '1';
					else							buf[i] = '0';
				}
				g_value_init(&set_value, G_TYPE_STRING);
				g_value_set_string(&set_value, buf);
				if(!nf_sysdb_set_key0("act.sys.disk.full.arout", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.buzzer);
				if(!nf_sysdb_set_key0("act.sys.disk.full.buzzer", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.opop);
				if(!nf_sysdb_set_key0("act.sys.disk.full.osdpopup", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.email);
				if(!nf_sysdb_set_key0("act.sys.disk.full.email", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.ftp);
				if(!nf_sysdb_set_key0("act.sys.disk.full.ftp", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.mobile);
				if(!nf_sysdb_set_key0("act.sys.disk.full.mobile", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.mobilepush);
				if(!nf_sysdb_set_key0("act.sys.disk.full.mobilepush", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);
			}
			break;

		case DISK_EXHAUSTED_EVT_DATA:
			{
				memset(buf, 0, sizeof(buf));
				for(i=0, j=0; i<48; i++)
				{
					if(i >= g_ch) {
						if(i < 32) {
							buf[i] = '0';
							continue;
						}
					}

					if(data.almOut & (1LL << j++))	buf[i] = '1';
					else							buf[i] = '0';
				}
				g_value_init(&set_value, G_TYPE_STRING);
				g_value_set_string(&set_value, buf);
				if(!nf_sysdb_set_key0("act.sys.disk.exhau.arout", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.buzzer);
				if(!nf_sysdb_set_key0("act.sys.disk.exhau.buzzer", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.opop);
				if(!nf_sysdb_set_key0("act.sys.disk.exhau.osdpopup", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.email);
				if(!nf_sysdb_set_key0("act.sys.disk.exhau.email", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.ftp);
				if(!nf_sysdb_set_key0("act.sys.disk.exhau.ftp", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.mobile);
				if(!nf_sysdb_set_key0("act.sys.disk.exhau.mobile", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.mobilepush);
				if(!nf_sysdb_set_key0("act.sys.disk.exhau.mobilepush", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);
			}
			break;

		case SMART_EVT_DATA:
			{
				memset(buf, 0, sizeof(buf));
				for(i=0, j=0; i<48; i++)
				{
					if(i >= g_ch) {
						if(i < 32) {
							buf[i] = '0';
							continue;
						}
					}

					if(data.almOut & (1LL << j++))	buf[i] = '1';
					else							buf[i] = '0';
				}
				g_value_init(&set_value, G_TYPE_STRING);
				g_value_set_string(&set_value, buf);
				if(!nf_sysdb_set_key0("act.sys.disk.smart.arout", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.buzzer);
				if(!nf_sysdb_set_key0("act.sys.disk.smart.buzzer", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.opop);
				if(!nf_sysdb_set_key0("act.sys.disk.smart.osdpopup", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.email);
				if(!nf_sysdb_set_key0("act.sys.disk.smart.email", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.ftp);
				if(!nf_sysdb_set_key0("act.sys.disk.smart.ftp", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.mobile);
				if(!nf_sysdb_set_key0("act.sys.disk.smart.mobile", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.mobilepush);
				if(!nf_sysdb_set_key0("act.sys.disk.smart.mobilepush", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);
			}
			break;

		case NO_DIST_EVT_DATA:
			{
				memset(buf, 0, sizeof(buf));
				for(i=0, j=0; i<48; i++)
				{
					if(i >= g_ch) {
						if(i < 32) {
							buf[i] = '0';
							continue;
						}
					}

					if(data.almOut & (1LL << j++))	buf[i] = '1';
					else							buf[i] = '0';
				}
				g_value_init(&set_value, G_TYPE_STRING);
				g_value_set_string(&set_value, buf);
				if(!nf_sysdb_set_key0("act.sys.disk.nodisk.arout", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.buzzer);
				if(!nf_sysdb_set_key0("act.sys.disk.nodisk.buzzer", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.opop);
				if(!nf_sysdb_set_key0("act.sys.disk.nodisk.osdpopup", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.email);
				if(!nf_sysdb_set_key0("act.sys.disk.nodisk.email", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.ftp);
				if(!nf_sysdb_set_key0("act.sys.disk.nodisk.ftp", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.mobile);
				if(!nf_sysdb_set_key0("act.sys.disk.nodisk.mobile", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.mobilepush);
				if(!nf_sysdb_set_key0("act.sys.disk.nodisk.mobilepush", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);
			}
			break;
	}

	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);


	return TRUE;
}

gboolean DAL_set_SysDisk_data_all(EA_SysDiskData *data)
{
	GValue set_value = {0,};
	gchar buf[49];
	SysDiskDataType data_type;
	gint i, j, k;

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	DDEBUG_START

	for(data_type=OW_START_EVT_DATA; data_type<=NO_DIST_EVT_DATA; data_type++)
	{
		if(data_type == OW_START_EVT_DATA)
		{
			memset(buf, 0, sizeof(buf));
			for(i=0, j=0; i<48; i++)
			{
				if(i >= g_ch) {
					if(i < 32) {
						buf[i] = '0';
						continue;
					}
				}

				if(data[data_type].almOut & (1LL << j++))	buf[i] = '1';
				else							buf[i] = '0';
			}
			//g_printf("%s :::::::::::::::::::: buf %s \n", __FUNCTION__, buf);
			g_value_init(&set_value, G_TYPE_STRING);
			g_value_set_string(&set_value, buf);
			if(!nf_sysdb_set_key0("act.sys.disk.over.arout", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].buzzer);
			if(!nf_sysdb_set_key0("act.sys.disk.over.buzzer", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].opop);
			if(!nf_sysdb_set_key0("act.sys.disk.over.osdpopup", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].email);
			if(!nf_sysdb_set_key0("act.sys.disk.over.email", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].ftp);
			if(!nf_sysdb_set_key0("act.sys.disk.over.ftp", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].mobile);
			if(!nf_sysdb_set_key0("act.sys.disk.over.mobile", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].mobilepush);
			if(!nf_sysdb_set_key0("act.sys.disk.over.mobilepush", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);
		}
		else if(data_type == DISK_FULL_EVT_DATA)
		{
			memset(buf, 0, sizeof(buf));
			for(i=0, j=0; i<48; i++)
			{
				if(i >= g_ch) {
					if(i < 32) {
						buf[i] = '0';
						continue;
					}
				}

				if(data[data_type].almOut & (1LL << j++))	buf[i] = '1';
				else							buf[i] = '0';
			}
			g_value_init(&set_value, G_TYPE_STRING);
			g_value_set_string(&set_value, buf);
			if(!nf_sysdb_set_key0("act.sys.disk.full.arout", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].buzzer);
			if(!nf_sysdb_set_key0("act.sys.disk.full.buzzer", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].opop);
			if(!nf_sysdb_set_key0("act.sys.disk.full.osdpopup", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].email);
			if(!nf_sysdb_set_key0("act.sys.disk.full.email", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].ftp);
			if(!nf_sysdb_set_key0("act.sys.disk.full.ftp", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].mobile);
			if(!nf_sysdb_set_key0("act.sys.disk.full.mobile", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].mobilepush);
			if(!nf_sysdb_set_key0("act.sys.disk.full.mobilepush", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);
		}
		else if(data_type == DISK_EXHAUSTED_EVT_DATA)
		{
			memset(buf, 0, sizeof(buf));
			for(i=0, j=0; i<48; i++)
			{
				if(i >= g_ch) {
					if(i < 32) {
						buf[i] = '0';
						continue;
					}
				}

				if(data[data_type].almOut & (1LL << j++))	buf[i] = '1';
				else							buf[i] = '0';
			}
			g_value_init(&set_value, G_TYPE_STRING);
			g_value_set_string(&set_value, buf);
			if(!nf_sysdb_set_key0("act.sys.disk.exhau.arout", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].buzzer);
			if(!nf_sysdb_set_key0("act.sys.disk.exhau.buzzer", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].opop);
			if(!nf_sysdb_set_key0("act.sys.disk.exhau.osdpopup", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].email);
			if(!nf_sysdb_set_key0("act.sys.disk.exhau.email", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].ftp);
			if(!nf_sysdb_set_key0("act.sys.disk.exhau.ftp", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].mobile);
			if(!nf_sysdb_set_key0("act.sys.disk.exhau.mobile", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].mobilepush);
			if(!nf_sysdb_set_key0("act.sys.disk.exhau.mobilepush", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);
		}
		else if(data_type == SMART_EVT_DATA)
		{
			memset(buf, 0, sizeof(buf));
			for(i=0, j=0; i<48; i++)
			{
				if(i >= g_ch) {
					if(i < 32) {
						buf[i] = '0';
						continue;
					}
				}

				if(data[data_type].almOut & (1LL << j++))	buf[i] = '1';
				else							buf[i] = '0';
			}
			g_value_init(&set_value, G_TYPE_STRING);
			g_value_set_string(&set_value, buf);
			if(!nf_sysdb_set_key0("act.sys.disk.smart.arout", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].buzzer);
			if(!nf_sysdb_set_key0("act.sys.disk.smart.buzzer", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].opop);
			if(!nf_sysdb_set_key0("act.sys.disk.smart.osdpopup", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].email);
			if(!nf_sysdb_set_key0("act.sys.disk.smart.email", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].ftp);
			if(!nf_sysdb_set_key0("act.sys.disk.smart.ftp", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].mobile);
			if(!nf_sysdb_set_key0("act.sys.disk.smart.mobile", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].mobilepush);
			if(!nf_sysdb_set_key0("act.sys.disk.smart.mobilepush", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);
		}
		else if(data_type == NO_DIST_EVT_DATA)
		{
			memset(buf, 0, sizeof(buf));
			for(i=0, j=0; i<48; i++)
			{
				if(i >= g_ch) {
					if(i < 32) {
						buf[i] = '0';
						continue;
					}
				}

				if(data[data_type].almOut & (1LL << j++))	buf[i] = '1';
				else							buf[i] = '0';
			}
			g_value_init(&set_value, G_TYPE_STRING);
			g_value_set_string(&set_value, buf);
			if(!nf_sysdb_set_key0("act.sys.disk.nodisk.arout", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].buzzer);
			if(!nf_sysdb_set_key0("act.sys.disk.nodisk.buzzer", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].opop);
			if(!nf_sysdb_set_key0("act.sys.disk.nodisk.osdpopup", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].email);
			if(!nf_sysdb_set_key0("act.sys.disk.nodisk.email", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].ftp);
			if(!nf_sysdb_set_key0("act.sys.disk.nodisk.ftp", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].mobile);
			if(!nf_sysdb_set_key0("act.sys.disk.nodisk.mobile", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].mobilepush);
			if(!nf_sysdb_set_key0("act.sys.disk.nodisk.mobilepush", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);
		}
	}

	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);


	return TRUE;
}

gboolean DAL_get_SysDisk_data(EA_SysDiskData *data, SysDiskDataType data_type)
{
	GValue ret_value = {0,};
	const gchar *str = NULL;
	gint i, j;

	switch(data_type) {
		case OW_START_EVT_DATA:
			{
				if(nf_sysdb_get_key0("act.sys.disk.over.arout", &ret_value, NULL))
				{
					str = g_value_get_string(&ret_value);
					for(i=0, j=0; i<48, j<48; i++)
					{
						if(i == g_ch) j = 32;

						if(str[j++] == '1')	data->almOut |= (1LL << i);
						else				data->almOut &= ~(1LL << i);
						//g_printf("%s :::::::::string [%s] = %d \n", __FUNCTION__, str, (data->almOut & (1LL << i)));
					}
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.disk.over.buzzer", &ret_value, NULL))
				{
					data->buzzer = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.disk.over.osdpopup", &ret_value, NULL))
				{
					data->opop = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.disk.over.email", &ret_value, NULL))
				{
					data->email = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.disk.over.ftp", &ret_value, NULL))
				{
					data->ftp = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.disk.over.mobile", &ret_value, NULL))
				{
					data->mobile = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.disk.over.mobilepush", &ret_value, NULL))
				{
					data->mobilepush= g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;
			}
			break;

		case DISK_FULL_EVT_DATA:
			{
				if(nf_sysdb_get_key0("act.sys.disk.full.arout", &ret_value, NULL))
				{
					str = g_value_get_string(&ret_value);
					for(i=0, j=0; i<48, j<48; i++)
					{
						if(i == g_ch) j = 32;

						if(str[j++] == '1')	data->almOut |= (1LL << i);
						else				data->almOut &= ~(1LL << i);
					}
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.disk.full.buzzer", &ret_value, NULL))
				{
					data->buzzer = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.disk.full.osdpopup", &ret_value, NULL))
				{
					data->opop = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.disk.full.email", &ret_value, NULL))
				{
					data->email = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.disk.full.ftp", &ret_value, NULL))
				{
					data->ftp = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.disk.full.mobile", &ret_value, NULL))
				{
					data->mobile = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.disk.full.mobilepush", &ret_value, NULL))
				{
					data->mobilepush = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;
			}
			break;

		case DISK_EXHAUSTED_EVT_DATA:
			{
				if(nf_sysdb_get_key0("act.sys.disk.exhau.arout", &ret_value, NULL))
				{
					str = g_value_get_string(&ret_value);
					for(i=0, j=0; i<48, j<48; i++)
					{
						if(i == g_ch) j = 32;

						if(str[j++] == '1')	data->almOut |= (1LL << i);
						else				data->almOut &= ~(1LL << i);
					}
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.disk.exhau.buzzer", &ret_value, NULL))
				{
					data->buzzer = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.disk.exhau.osdpopup", &ret_value, NULL))
				{
					data->opop = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.disk.exhau.email", &ret_value, NULL))
				{
					data->email = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.disk.exhau.ftp", &ret_value, NULL))
				{
					data->ftp = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.disk.exhau.mobile", &ret_value, NULL))
				{
					data->mobile = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.disk.exhau.mobilepush", &ret_value, NULL))
				{
					data->mobilepush = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;
			}
			break;

		case SMART_EVT_DATA:
			{
				if(nf_sysdb_get_key0("act.sys.disk.smart.arout", &ret_value, NULL))
				{
					str = g_value_get_string(&ret_value);
					for(i=0, j=0; i<48, j<48; i++)
					{
						if(i == g_ch) j = 32;

						if(str[j++] == '1')	data->almOut |= (1LL << i);
						else				data->almOut &= ~(1LL << i);
					}
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.disk.smart.buzzer", &ret_value, NULL))
				{
					data->buzzer = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.disk.smart.osdpopup", &ret_value, NULL))
				{
					data->opop = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.disk.smart.email", &ret_value, NULL))
				{
					data->email = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.disk.smart.ftp", &ret_value, NULL))
				{
					data->ftp = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.disk.smart.mobile", &ret_value, NULL))
				{
					data->mobile = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.disk.smart.mobilepush", &ret_value, NULL))
				{
					data->mobilepush = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;
			}
			break;

		case NO_DIST_EVT_DATA:
			{
				if(nf_sysdb_get_key0("act.sys.disk.nodisk.arout", &ret_value, NULL))
				{
					str = g_value_get_string(&ret_value);
					for(i=0, j=0; i<48, j<48; i++)
					{
						if(i == g_ch) j = 32;

						if(str[j++] == '1')	data->almOut |= (1LL << i);
						else				data->almOut &= ~(1LL << i);
					}
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.disk.nodisk.buzzer", &ret_value, NULL))
				{
					data->buzzer = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.disk.nodisk.osdpopup", &ret_value, NULL))
				{
					data->opop = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.disk.nodisk.email", &ret_value, NULL))
				{
					data->email = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.disk.nodisk.ftp", &ret_value, NULL))
				{
					data->ftp = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.disk.nodisk.mobile", &ret_value, NULL))
				{
					data->mobile = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.disk.nodisk.mobilepush", &ret_value, NULL))
				{
					data->mobilepush = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;
			}
			break;
	}

	return TRUE;
}

gboolean DAL_get_SysDisk_oPop_boolean(SysDiskDataType data_type)
{
	GValue ret_value = {0,};
	gboolean ret = FALSE;

	switch(data_type) {
		case OW_START_EVT_DATA:
			if(nf_sysdb_get_key0("act.sys.disk.over.osdpopup", &ret_value, NULL))
			{
				ret = g_value_get_boolean(&ret_value);
				g_value_unset(&ret_value);
			}
			break;

		case DISK_FULL_EVT_DATA:
			if(nf_sysdb_get_key0("act.sys.disk.full.osdpopup", &ret_value, NULL))
			{
				ret = g_value_get_boolean(&ret_value);
				g_value_unset(&ret_value);
			}
			break;

		case DISK_EXHAUSTED_EVT_DATA:
			if(nf_sysdb_get_key0("act.sys.disk.exhau.osdpopup", &ret_value, NULL))
			{
				ret = g_value_get_boolean(&ret_value);
				g_value_unset(&ret_value);
			}
			break;

		case SMART_EVT_DATA:
			if(nf_sysdb_get_key0("act.sys.disk.smart.osdpopup", &ret_value, NULL))
			{
				ret = g_value_get_boolean(&ret_value);
				g_value_unset(&ret_value);
			}
			break;

		case NO_DIST_EVT_DATA:
			if(nf_sysdb_get_key0("act.sys.disk.nodisk.osdpopup", &ret_value, NULL))
			{
				ret = g_value_get_boolean(&ret_value);
				g_value_unset(&ret_value);
			}
			break;
	}

	return ret;
}

gboolean DAL_set_SysDiskEvt_data(EA_SysDiskEvtData data)
{
	GValue set_value = {0,};

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	DDEBUG_START

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data.disk_exhau);
	if(!nf_sysdb_set_key0("act.sys.disk.exhau.threshold", &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_INT);
	g_value_set_int(&set_value, data.nodisk_cnt);
	if(!nf_sysdb_set_key0("act.sys.disk.nodisk.cnt", &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);

	return TRUE;
}


gboolean DAL_get_SysDiskEvt_data(EA_SysDiskEvtData *data)
{
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("act.sys.disk.exhau.threshold", &ret_value, NULL))
	{
		data->disk_exhau = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key0("act.sys.disk.nodisk.cnt", &ret_value, NULL))
	{
		data->nodisk_cnt = g_value_get_int(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	return TRUE;
}


/***************************************************************
 * Setup --> Event&Action --> System Event --> Record
 * *************************************************************/
gboolean DAL_set_SysRec_Data(EA_SysRecData data, SysRecDataType data_type)
{
	GValue set_value = {0,};
	gchar buf[49];
	gint i, j;

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	DDEBUG_START

	memset(buf, 0, sizeof(buf));

	switch(data_type) {
		case PANIC_START_EVT_DATA:
		{
			for(i=0, j=0; i<48; i++)
			{
				if(i >= g_ch) {
					if(i < 32) {
						buf[i] = '0';
						continue;
					}
				}

				if(data.almOut & (1LL << j++))	buf[i] = '1';
				else							buf[i] = '0';
			}
			g_value_init(&set_value, G_TYPE_STRING);
			g_value_set_string(&set_value, buf);
			if(!nf_sysdb_set_key0("act.sys.rec.pstart.arout", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
				return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data.buzzer);
			if(!nf_sysdb_set_key0("act.sys.rec.pstart.buzzer", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
				return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data.osd_popup);
			if(!nf_sysdb_set_key0("act.sys.rec.pstart.osdpopup", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
				return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data.email);
			if(!nf_sysdb_set_key0("act.sys.rec.pstart.email", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
				return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data.ftp);
			if(!nf_sysdb_set_key0("act.sys.rec.pstart.ftp", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
				return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data.mobile);
			if(!nf_sysdb_set_key0("act.sys.rec.pstart.mobile", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
				return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data.mobilepush);
			if(!nf_sysdb_set_key0("act.sys.rec.pstart.mobilepush", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
				return FALSE;
			}
			g_value_unset(&set_value);
		}
		break;

		case RECORD_AUDIT_EVT_DATA:
		{
			for(i=0, j=0; i<48; i++)
			{
				if(i >= g_ch) {
					if(i < 32) {
						buf[i] = '0';
						continue;
					}
				}

				if(data.almOut & (1LL << j++))	buf[i] = '1';
				else							buf[i] = '0';
			}
			g_value_init(&set_value, G_TYPE_STRING);
			g_value_set_string(&set_value, buf);
			if(!nf_sysdb_set_key0("act.sys.rec.audit.arout", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
				return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data.buzzer);
			if(!nf_sysdb_set_key0("act.sys.rec.audit.buzzer", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
				return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data.osd_popup);
			if(!nf_sysdb_set_key0("act.sys.rec.audit.osdpopup", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
				return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data.email);
			if(!nf_sysdb_set_key0("act.sys.rec.audit.email", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
				return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data.ftp);
			if(!nf_sysdb_set_key0("act.sys.rec.audit.ftp", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
				return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data.mobile);
			if(!nf_sysdb_set_key0("act.sys.rec.audit.mobile", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
				return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data.mobilepush);
			if(!nf_sysdb_set_key0("act.sys.rec.audit.mobilepush", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
				return FALSE;
			}
			g_value_unset(&set_value);
		}
		break;
	}

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);

	return TRUE;
}

gboolean DAL_get_SysRec_data(EA_SysRecData *data, SysRecDataType data_type)
{
	GValue ret_value = {0,};
	const gchar *str = NULL;
	gint i, j;

	switch(data_type) {
		case PANIC_START_EVT_DATA:
		{
			if(nf_sysdb_get_key0("act.sys.rec.pstart.arout", &ret_value, NULL))
			{
				str = g_value_get_string(&ret_value);
				for(i=0, j=0; i<48, j<48; i++)
				{
					if(i == g_ch) j = 32;

					if(str[j++] == '1')	data->almOut |= (1LL << i);
					else				data->almOut &= ~(1LL << i);
				}
				g_value_unset(&ret_value);
			}
			else
				return FALSE;

			if(nf_sysdb_get_key0("act.sys.rec.pstart.buzzer", &ret_value, NULL))
			{
				data->buzzer = g_value_get_boolean(&ret_value);
				g_value_unset(&ret_value);
			}
			else
				return FALSE;

			if(nf_sysdb_get_key0("act.sys.rec.pstart.osdpopup", &ret_value, NULL))
			{
				data->osd_popup = g_value_get_boolean(&ret_value);
				g_value_unset(&ret_value);
			}
			else
				return FALSE;

			if(nf_sysdb_get_key0("act.sys.rec.pstart.email", &ret_value, NULL))
			{
				data->email = g_value_get_boolean(&ret_value);
				g_value_unset(&ret_value);
			}
			else
				return FALSE;

			if(nf_sysdb_get_key0("act.sys.rec.pstart.ftp", &ret_value, NULL))
			{
				data->ftp = g_value_get_boolean(&ret_value);
				g_value_unset(&ret_value);
			}
			else
				return FALSE;

			if(nf_sysdb_get_key0("act.sys.rec.pstart.mobile", &ret_value, NULL))
			{
				data->mobile = g_value_get_boolean(&ret_value);
				g_value_unset(&ret_value);
			}
			else
				return FALSE;

			if(nf_sysdb_get_key0("act.sys.rec.pstart.mobilepush", &ret_value, NULL))
			{
				data->mobilepush = g_value_get_boolean(&ret_value);
				g_value_unset(&ret_value);
			}
			else
				return FALSE;
		}
		break;

		case RECORD_AUDIT_EVT_DATA:
		{
			if(nf_sysdb_get_key0("act.sys.rec.audit.arout", &ret_value, NULL))
			{
				str = g_value_get_string(&ret_value);
				for(i=0, j=0; i<48, j<48; i++)
				{
					if(i == g_ch) j = 32;

					if(str[j++] == '1')	data->almOut |= (1LL << i);
					else				data->almOut &= ~(1LL << i);
				}
				g_value_unset(&ret_value);
			}
			else
				return FALSE;

			if(nf_sysdb_get_key0("act.sys.rec.audit.buzzer", &ret_value, NULL))
			{
				data->buzzer = g_value_get_boolean(&ret_value);
				g_value_unset(&ret_value);
			}
			else
				return FALSE;

			if(nf_sysdb_get_key0("act.sys.rec.audit.osdpopup", &ret_value, NULL))
			{
				data->osd_popup = g_value_get_boolean(&ret_value);
				g_value_unset(&ret_value);
			}
			else
				return FALSE;

			if(nf_sysdb_get_key0("act.sys.rec.audit.email", &ret_value, NULL))
			{
				data->email = g_value_get_boolean(&ret_value);
				g_value_unset(&ret_value);
			}
			else
				return FALSE;

			if(nf_sysdb_get_key0("act.sys.rec.audit.ftp", &ret_value, NULL))
			{
				data->ftp = g_value_get_boolean(&ret_value);
				g_value_unset(&ret_value);
			}
			else
				return FALSE;

			if(nf_sysdb_get_key0("act.sys.rec.audit.mobile", &ret_value, NULL))
			{
				data->mobile = g_value_get_boolean(&ret_value);
				g_value_unset(&ret_value);
			}
			else
				return FALSE;

			if(nf_sysdb_get_key0("act.sys.rec.audit.mobilepush", &ret_value, NULL))
			{
				data->mobilepush = g_value_get_boolean(&ret_value);
				g_value_unset(&ret_value);
			}
			else
				return FALSE;
		}
		break;
	}

	return TRUE;
}

guint DAL_set_RecAudit_restart(gboolean active)
{
	guint ret = 0;
	GValue set_value = {0,};

	DDEBUG_START
	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, active);
	if(!nf_sysdb_set_key0("act.sys.rec.audit.restart", &set_value, NULL))
	{
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 1;
	}

	g_value_unset(&set_value);
	DDEBUG_END

	return ret;
}

gboolean DAL_get_RecAudit_restart()
{
	gboolean ret = FALSE;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("act.sys.rec.audit.restart", &ret_value, NULL))
	{
		ret = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}

	return ret;
}


/***************************************************************
 * Setup --> Event&Action --> System Event --> System
 * *************************************************************/
gboolean DAL_set_SysSys_Data(EA_SysSysData data, SysSysDataType data_type)
{
	GValue set_value = {0,};
	gchar buf[33];
	gint i, j;

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	DDEBUG_START

	switch(data_type) {
		case BOOTING_EVT_DATA:
			{
				memset(buf, 0, sizeof(buf));
				for(i=0, j=0; i<48; i++)
				{
					if(i >= g_ch) {
						if(i < 32) {
							buf[i] = '0';
							continue;
						}
					}

					if(data.almOut & (1LL << j++))	buf[i] = '1';
					else							buf[i] = '0';
				}
				g_value_init(&set_value, G_TYPE_STRING);
				g_value_set_string(&set_value, buf);
				if(!nf_sysdb_set_key0("act.sys.sys.booting.arout", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
						return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.buzzer);
				if(!nf_sysdb_set_key0("act.sys.sys.booting.buzzer", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
						return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.osd_popup);
				if(!nf_sysdb_set_key0("act.sys.sys.booting.osdpopup", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
						return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.email);
				if(!nf_sysdb_set_key0("act.sys.sys.booting.email", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
						return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.ftp);
				if(!nf_sysdb_set_key0("act.sys.sys.booting.ftp", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
						return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.mobile);
				if(!nf_sysdb_set_key0("act.sys.sys.booting.mobile", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
						return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.mobilepush);
				if(!nf_sysdb_set_key0("act.sys.sys.booting.mobilepush", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
						return FALSE;
				}
				g_value_unset(&set_value);
			}
			break;

		case LOGON_FAIL_EVT_DATA:
			{
				memset(buf, 0, sizeof(buf));
				for(i=0, j=0; i<48; i++)
				{
					if(i >= g_ch) {
						if(i < 32) {
							buf[i] = '0';
							continue;
						}
					}

					if(data.almOut & (1LL << j++))	buf[i] = '1';
					else							buf[i] = '0';
				}
				g_value_init(&set_value, G_TYPE_STRING);
				g_value_set_string(&set_value, buf);
				if(!nf_sysdb_set_key0("act.sys.sys.logon_fail.arout", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
						return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.buzzer);
				if(!nf_sysdb_set_key0("act.sys.sys.logon_fail.buzzer", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
						return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.osd_popup);
				if(!nf_sysdb_set_key0("act.sys.sys.logon_fail.osdpopup", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
						return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.email);
				if(!nf_sysdb_set_key0("act.sys.sys.logon_fail.email", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
						return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.ftp);
				if(!nf_sysdb_set_key0("act.sys.sys.logon_fail.ftp", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
						return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.mobile);
				if(!nf_sysdb_set_key0("act.sys.sys.logon_fail.mobile", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
						return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.mobilepush);
				if(!nf_sysdb_set_key0("act.sys.sys.logon_fail.mobilepush", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
						return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_INT);
				g_value_set_int(&set_value, data.failCnt);
				if(!nf_sysdb_set_key0("act.sys.sys.logon_fail.cnt", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
						return FALSE;
				}
				g_value_unset(&set_value);
			}
			break;

		case FAN_FAIL_EVT_DATA:
			{
				memset(buf, 0, sizeof(buf));
				for(i=0, j=0; i<48; i++)
				{
					if(i >= g_ch) {
						if(i < 32) {
							buf[i] = '0';
							continue;
						}
					}

					if(data.almOut & (1LL << j++))	buf[i] = '1';
					else							buf[i] = '0';
				}
				g_value_init(&set_value, G_TYPE_STRING);
				g_value_set_string(&set_value, buf);
				if(!nf_sysdb_set_key0("act.sys.sys.fan_fail.arout", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
						return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.buzzer);
				if(!nf_sysdb_set_key0("act.sys.sys.fan_fail.buzzer", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
						return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.osd_popup);
				if(!nf_sysdb_set_key0("act.sys.sys.fan_fail.osdpopup", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
						return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.email);
				if(!nf_sysdb_set_key0("act.sys.sys.fan_fail.email", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
						return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.ftp);
				if(!nf_sysdb_set_key0("act.sys.sys.fan_fail.ftp", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
						return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.mobile);
				if(!nf_sysdb_set_key0("act.sys.sys.fan_fail.mobile", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
						return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.mobilepush);
				if(!nf_sysdb_set_key0("act.sys.sys.fan_fail.mobilepush", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
						return FALSE;
				}
				g_value_unset(&set_value);
			}
			break;

		case TEMPER_FAIL_EVT_DATA:
			{
				memset(buf, 0, sizeof(buf));
				for(i=0, j=0; i<48; i++)
				{
					if(i >= g_ch) {
						if(i < 32) {
							buf[i] = '0';
							continue;
						}
					}

					if(data.almOut & (1LL << j++))	buf[i] = '1';
					else							buf[i] = '0';
				}
				g_value_init(&set_value, G_TYPE_STRING);
				g_value_set_string(&set_value, buf);
				if(!nf_sysdb_set_key0("act.sys.sys.temper_fail.arout", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
						return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.buzzer);
				if(!nf_sysdb_set_key0("act.sys.sys.temper_fail.buzzer", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
						return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.osd_popup);
				if(!nf_sysdb_set_key0("act.sys.sys.temper_fail.osdpopup", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
						return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.email);
				if(!nf_sysdb_set_key0("act.sys.sys.temper_fail.email", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
						return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.ftp);
				if(!nf_sysdb_set_key0("act.sys.sys.temper_fail.ftp", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
						return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.mobile);
				if(!nf_sysdb_set_key0("act.sys.sys.temper_fail.mobile", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
						return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.mobilepush);
				if(!nf_sysdb_set_key0("act.sys.sys.temper_fail.mobile", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
						return FALSE;
				}
				g_value_unset(&set_value);
			}
			break;

		case POE_FAIL_EVT_DATA:
			{
				memset(buf, 0, sizeof(buf));
				for(i=0, j=0; i<48; i++)
				{
					if(i >= g_ch) {
						if(i < 32) {
							buf[i] = '0';
							continue;
						}
					}

					if(data.almOut & (1LL << j++))	buf[i] = '1';
					else							buf[i] = '0';
				}
				g_value_init(&set_value, G_TYPE_STRING);
				g_value_set_string(&set_value, buf);
				if(!nf_sysdb_set_key0("act.sys.sys.poe_fail.arout", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
						return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.buzzer);
				if(!nf_sysdb_set_key0("act.sys.sys.poe_fail.buzzer", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
						return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.osd_popup);
				if(!nf_sysdb_set_key0("act.sys.sys.poe_fail.osdpopup", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
						return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.email);
				if(!nf_sysdb_set_key0("act.sys.sys.poe_fail.email", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
						return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.ftp);
				if(!nf_sysdb_set_key0("act.sys.sys.poe_fail.ftp", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
						return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.mobile);
				if(!nf_sysdb_set_key0("act.sys.sys.poe_fail.mobile", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
						return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.mobilepush);
				if(!nf_sysdb_set_key0("act.sys.sys.poe_fail.mobilepush", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
						return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_INT);
				g_value_set_int(&set_value, data.failPoe);
				if(!nf_sysdb_set_key0("act.sys.sys.poe_fail.threshold", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
						return FALSE;
				}
				g_value_unset(&set_value);
			}
			break;
	}

	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);

	return TRUE;
}

gboolean DAL_set_SysSys_Data_all(EA_SysSysData *data)
{
	GValue set_value = {0,};
	gchar buf[49];
	SysSysDataType data_type;
	gint i, j;

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	DDEBUG_START

	for(data_type=BOOTING_EVT_DATA; data_type<=POE_FAIL_EVT_DATA; data_type++)
	{
		if(data_type == BOOTING_EVT_DATA)
		{
			memset(buf, 0, sizeof(buf));
			for(i=0, j=0; i<48; i++)
			{
				if(i >= g_ch) {
					if(i < 32) {
						buf[i] = '0';
						continue;
					}
				}

				if(data[data_type].almOut & (1LL << j++))	buf[i] = '1';
				else							buf[i] = '0';
			}
			g_value_init(&set_value, G_TYPE_STRING);
			g_value_set_string(&set_value, buf);
			if(!nf_sysdb_set_key0("act.sys.sys.booting.arout", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].buzzer);
			if(!nf_sysdb_set_key0("act.sys.sys.booting.buzzer", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].osd_popup);
			if(!nf_sysdb_set_key0("act.sys.sys.booting.osdpopup", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].email);
			if(!nf_sysdb_set_key0("act.sys.sys.booting.email", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].ftp);
			if(!nf_sysdb_set_key0("act.sys.sys.booting.ftp", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].mobile);
			if(!nf_sysdb_set_key0("act.sys.sys.booting.mobile", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].mobilepush);
			if(!nf_sysdb_set_key0("act.sys.sys.booting.mobilepush", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);
		}
		else if(data_type == LOGON_FAIL_EVT_DATA)
		{
			memset(buf, 0, sizeof(buf));
			for(i=0, j=0; i<48; i++)
			{
				if(i >= g_ch) {
					if(i < 32) {
						buf[i] = '0';
						continue;
					}
				}

				if(data[data_type].almOut & (1LL << j++))	buf[i] = '1';
				else							buf[i] = '0';
			}
			g_value_init(&set_value, G_TYPE_STRING);
			g_value_set_string(&set_value, buf);
			if(!nf_sysdb_set_key0("act.sys.sys.logon_fail.arout", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].buzzer);
			if(!nf_sysdb_set_key0("act.sys.sys.logon_fail.buzzer", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].osd_popup);
			if(!nf_sysdb_set_key0("act.sys.sys.logon_fail.osdpopup", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].email);
			if(!nf_sysdb_set_key0("act.sys.sys.logon_fail.email", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].ftp);
			if(!nf_sysdb_set_key0("act.sys.sys.logon_fail.ftp", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].mobile);
			if(!nf_sysdb_set_key0("act.sys.sys.logon_fail.mobile", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].mobilepush);
			if(!nf_sysdb_set_key0("act.sys.sys.logon_fail.mobilepush", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_INT);
			g_value_set_int(&set_value, data[data_type].failCnt);
			if(!nf_sysdb_set_key0("act.sys.sys.logon_fail.cnt", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);
		}
		else if(data_type == FAN_FAIL_EVT_DATA)
		{
			memset(buf, 0, sizeof(buf));
			for(i=0, j=0; i<48; i++)
			{
				if(i >= g_ch) {
					if(i < 32) {
						buf[i] = '0';
						continue;
					}
				}

				if(data[data_type].almOut & (1LL << j++))	buf[i] = '1';
				else							buf[i] = '0';
			}
			g_value_init(&set_value, G_TYPE_STRING);
			g_value_set_string(&set_value, buf);
			if(!nf_sysdb_set_key0("act.sys.sys.fan_fail.arout", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].buzzer);
			if(!nf_sysdb_set_key0("act.sys.sys.fan_fail.buzzer", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].osd_popup);
			if(!nf_sysdb_set_key0("act.sys.sys.fan_fail.osdpopup", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].email);
			if(!nf_sysdb_set_key0("act.sys.sys.fan_fail.email", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].ftp);
			if(!nf_sysdb_set_key0("act.sys.sys.fan_fail.ftp", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].mobile);
			if(!nf_sysdb_set_key0("act.sys.sys.fan_fail.mobile", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].mobilepush);
			if(!nf_sysdb_set_key0("act.sys.sys.fan_fail.mobilepush", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);
		}
		else if(data_type == TEMPER_FAIL_EVT_DATA)
		{
			memset(buf, 0, sizeof(buf));
			for(i=0, j=0; i<48; i++)
			{
				if(i >= g_ch) {
					if(i < 32) {
						buf[i] = '0';
						continue;
					}
				}

				if(data[data_type].almOut & (1LL << j++))	buf[i] = '1';
				else							buf[i] = '0';
			}
			g_value_init(&set_value, G_TYPE_STRING);
			g_value_set_string(&set_value, buf);
			if(!nf_sysdb_set_key0("act.sys.sys.temper_fail.arout", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].buzzer);
			if(!nf_sysdb_set_key0("act.sys.sys.temper_fail.buzzer", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].osd_popup);
			if(!nf_sysdb_set_key0("act.sys.sys.temper_fail.osdpopup", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].email);
			if(!nf_sysdb_set_key0("act.sys.sys.temper_fail.email", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].ftp);
			if(!nf_sysdb_set_key0("act.sys.sys.temper_fail.ftp", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].mobile);
			if(!nf_sysdb_set_key0("act.sys.sys.temper_fail.mobile", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].mobilepush);
			if(!nf_sysdb_set_key0("act.sys.sys.temper_fail.mobilepush", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);
		}
		else if(data_type == POE_FAIL_EVT_DATA)
		{
			memset(buf, 0, sizeof(buf));
			for(i=0, j=0; i<48; i++)
			{
				if(i >= g_ch) {
					if(i < 32) {
						buf[i] = '0';
						continue;
					}
				}

				if(data[data_type].almOut & (1LL << j++))	buf[i] = '1';
				else							buf[i] = '0';
			}
			g_value_init(&set_value, G_TYPE_STRING);
			g_value_set_string(&set_value, buf);
			if(!nf_sysdb_set_key0("act.sys.sys.poe_fail.arout", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].buzzer);
			if(!nf_sysdb_set_key0("act.sys.sys.poe_fail.buzzer", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].osd_popup);
			if(!nf_sysdb_set_key0("act.sys.sys.poe_fail.osdpopup", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].email);
			if(!nf_sysdb_set_key0("act.sys.sys.poe_fail.email", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].ftp);
			if(!nf_sysdb_set_key0("act.sys.sys.poe_fail.ftp", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].mobile);
			if(!nf_sysdb_set_key0("act.sys.sys.poe_fail.mobile", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].mobilepush);
			if(!nf_sysdb_set_key0("act.sys.sys.poe_fail.mobilepush", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_INT);
			g_value_set_int(&set_value, data[data_type].failPoe);
			if(!nf_sysdb_set_key0("act.sys.sys.poe_fail.threshold", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);
		}
	}

	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);

	return TRUE;
}

gboolean DAL_get_SysSys_Data(EA_SysSysData *data, SysSysDataType data_type)
{
	GValue ret_value = {0,};
	const gchar *str = NULL;
	gint i, j;

	switch(data_type) {
		case BOOTING_EVT_DATA:
			{
				if(nf_sysdb_get_key0("act.sys.sys.booting.arout", &ret_value, NULL))
				{
					str = g_value_get_string(&ret_value);
					for(i=0, j=0; i<48, j<48; i++)
					{
						if(i == g_ch) j = 32;

						if(str[j++] == '1')	data->almOut |= (1LL << i);
						else				data->almOut &= ~(1LL << i);
					}
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.sys.booting.buzzer", &ret_value, NULL))
				{
					data->buzzer = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.sys.booting.osdpopup", &ret_value, NULL))
				{
					data->osd_popup = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.sys.booting.email", &ret_value, NULL))
				{
					data->email = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.sys.booting.ftp", &ret_value, NULL))
				{
					data->ftp = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.sys.booting.mobile", &ret_value, NULL))
				{
					data->mobile = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.sys.booting.mobilepush", &ret_value, NULL))
				{
					data->mobilepush = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;
			}
			break;

		case LOGON_FAIL_EVT_DATA:
			{
				if(nf_sysdb_get_key0("act.sys.sys.logon_fail.arout", &ret_value, NULL))
				{
					str = g_value_get_string(&ret_value);
					for(i=0, j=0; i<48, j<48; i++)
					{
						if(i == g_ch) j = 32;

						if(str[j++] == '1')	data->almOut |= (1LL << i);
						else				data->almOut &= ~(1LL << i);
					}
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.sys.logon_fail.buzzer", &ret_value, NULL))
				{
					data->buzzer = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.sys.logon_fail.osdpopup", &ret_value, NULL))
				{
					data->osd_popup = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.sys.logon_fail.email", &ret_value, NULL))
				{
					data->email = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.sys.logon_fail.ftp", &ret_value, NULL))
				{
					data->ftp = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.sys.logon_fail.mobile", &ret_value, NULL))
				{
					data->mobile = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.sys.logon_fail.mobilepush", &ret_value, NULL))
				{
					data->mobilepush = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.sys.logon_fail.cnt", &ret_value, NULL))
				{
					data->failCnt = g_value_get_int(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;
			}
			break;

		case FAN_FAIL_EVT_DATA:
			{
				if(nf_sysdb_get_key0("act.sys.sys.fan_fail.arout", &ret_value, NULL))
				{
					str = g_value_get_string(&ret_value);
					for(i=0, j=0; i<48, j<48; i++)
					{
						if(i == g_ch) j = 32;

						if(str[j++] == '1')	data->almOut |= (1LL << i);
						else				data->almOut &= ~(1LL << i);
					}
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.sys.fan_fail.buzzer", &ret_value, NULL))
				{
					data->buzzer = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.sys.fan_fail.osdpopup", &ret_value, NULL))
				{
					data->osd_popup = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.sys.fan_fail.email", &ret_value, NULL))
				{
					data->email = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.sys.fan_fail.ftp", &ret_value, NULL))
				{
					data->ftp = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.sys.fan_fail.mobile", &ret_value, NULL))
				{
					data->mobile = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.sys.fan_fail.mobilepush", &ret_value, NULL))
				{
					data->mobilepush = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;
			}
			break;

		case TEMPER_FAIL_EVT_DATA:
			{
				if(nf_sysdb_get_key0("act.sys.sys.temper_fail.arout", &ret_value, NULL))
				{
					str = g_value_get_string(&ret_value);
					for(i=0, j=0; i<48, j<48; i++)
					{
						if(i == g_ch) j = 32;

						if(str[j++] == '1')	data->almOut |= (1LL << i);
						else				data->almOut &= ~(1LL << i);
					}
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.sys.temper_fail.buzzer", &ret_value, NULL))
				{
					data->buzzer = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.sys.temper_fail.osdpopup", &ret_value, NULL))
				{
					data->osd_popup = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.sys.temper_fail.email", &ret_value, NULL))
				{
					data->email = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.sys.temper_fail.ftp", &ret_value, NULL))
				{
					data->ftp = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.sys.temper_fail.mobile", &ret_value, NULL))
				{
					data->mobile = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.sys.temper_fail.mobilepush", &ret_value, NULL))
				{
					data->mobilepush = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;
			}
			break;

		case POE_FAIL_EVT_DATA:
			{
				if(nf_sysdb_get_key0("act.sys.sys.poe_fail.arout", &ret_value, NULL))
				{
					str = g_value_get_string(&ret_value);
					for(i=0, j=0; i<48, j<48; i++)
					{
						if(i == g_ch) j = 32;

						if(str[j++] == '1')	data->almOut |= (1LL << i);
						else				data->almOut &= ~(1LL << i);
					}
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.sys.poe_fail.buzzer", &ret_value, NULL))
				{
					data->buzzer = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.sys.poe_fail.osdpopup", &ret_value, NULL))
				{
					data->osd_popup = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.sys.poe_fail.email", &ret_value, NULL))
				{
					data->email = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.sys.poe_fail.ftp", &ret_value, NULL))
				{
					data->ftp = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.sys.poe_fail.mobile", &ret_value, NULL))
				{
					data->mobile = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.sys.poe_fail.mobilepush", &ret_value, NULL))
				{
					data->mobilepush = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.sys.poe_fail.threshold", &ret_value, NULL))
				{
					data->failPoe = g_value_get_int(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;
			}
			break;
	}

	return TRUE;
}

gboolean DAL_get_SysSys_oPop_boolean(SysSysDataType data_type)
{
	GValue ret_value = {0,};
	gboolean ret = FALSE;

	switch(data_type) {
		case BOOTING_EVT_DATA:
			if(nf_sysdb_get_key0("act.sys.sys.booting.osdpopup", &ret_value, NULL))
			{
				ret = g_value_get_boolean(&ret_value);
				g_value_unset(&ret_value);
			}
			break;

		case LOGON_FAIL_EVT_DATA:
			if(nf_sysdb_get_key0("act.sys.sys.logon_fail.osdpopup", &ret_value, NULL))
			{
				ret = g_value_get_boolean(&ret_value);
				g_value_unset(&ret_value);
			}
			break;

		case FAN_FAIL_EVT_DATA:
			if(nf_sysdb_get_key0("act.sys.sys.fan_fail.osdpopup", &ret_value, NULL))
			{
				ret = g_value_get_boolean(&ret_value);
				g_value_unset(&ret_value);
			}
			break;

		case TEMPER_FAIL_EVT_DATA:
			if(nf_sysdb_get_key0("act.sys.sys.temper_fail.osdpopup", &ret_value, NULL))
			{
				ret = g_value_get_boolean(&ret_value);
				g_value_unset(&ret_value);
			}
			break;

		case POE_FAIL_EVT_DATA:
			if(nf_sysdb_get_key0("act.sys.sys.poe_fail.osdpopup", &ret_value, NULL))
			{
				ret = g_value_get_boolean(&ret_value);
				g_value_unset(&ret_value);
			}
			break;
	    case IP_CONFLICT_EVT_DATA:
	        if(nf_sysdb_get_key0("act.sys.net.dhcp_fail.osdpopup", &ret_value, NULL))
	        {
	            ret = g_value_get_boolean(&ret_value);
	            g_value_unset(&ret_value);
	        }
	        break;
	}

	return ret;
}


/***************************************************************
 * Setup --> Event&Action --> System Event --> Network
 * *************************************************************/
gboolean DAL_set_SysNet_data(EA_SysNetData data, SysNetDataType data_type)
{
	GValue set_value = {0,};
	gchar buf[49];
	gint i, j;

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	DDEBUG_START

	switch(data_type) {
		case INTERNET_TROUBLE_EVT_DATA:
			{
				memset(buf, 0x00, sizeof(buf));
				for(i=0, j=0; i<48; i++)
				{
					if(i >= g_ch) {
						if(i < 32) {
							buf[i] = '0';
							continue;
						}
					}

					if(data.almOut & (1LL << j++))	buf[i] = '1';
					else							buf[i] = '0';
				}
				g_value_init(&set_value, G_TYPE_STRING);
				g_value_set_string(&set_value, buf);
				if(!nf_sysdb_set_key0("act.sys.net.eth_trouble.arout", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.buzzer);
				if(!nf_sysdb_set_key0("act.sys.net.eth_trouble.buzzer", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.email);
				if(!nf_sysdb_set_key0("act.sys.net.eth_trouble.email", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.ftp);
				if(!nf_sysdb_set_key0("act.sys.net.eth_trouble.ftp", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.mobile);
				if(!nf_sysdb_set_key0("act.sys.net.eth_trouble.mobile", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.mobilepush);
				if(!nf_sysdb_set_key0("act.sys.net.eth_trouble.mobilepush", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);
			}
			break;

		case REMOTE_LOGON_FAIL_EVT_DATA:
			{
				memset(buf, 0x00, sizeof(buf));
				for(i=0, j=0; i<48; i++)
				{
					if(i >= g_ch) {
						if(i < 32) {
							buf[i] = '0';
							continue;
						}
					}

					if(data.almOut & (1LL << j++))	buf[i] = '1';
					else							buf[i] = '0';
				}
				g_value_init(&set_value, G_TYPE_STRING);
				g_value_set_string(&set_value, buf);
				if(!nf_sysdb_set_key0("act.sys.net.rfail.arout", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.buzzer);
				if(!nf_sysdb_set_key0("act.sys.net.rfail.buzzer", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.email);
				if(!nf_sysdb_set_key0("act.sys.net.rfail.email", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.ftp);
				if(!nf_sysdb_set_key0("act.sys.net.rfail.ftp", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.mobile);
				if(!nf_sysdb_set_key0("act.sys.net.rfail.mobile", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.mobilepush);
				if(!nf_sysdb_set_key0("act.sys.net.rfail.mobilepush", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);
			}
			break;

		case DDNS_UPDATE_FAIL_EVT_DATA:
			{
				memset(buf, 0x00, sizeof(buf));
				for(i=0, j=0; i<48; i++)
				{
					if(i >= g_ch) {
						if(i < 32) {
							buf[i] = '0';
							continue;
						}
					}

					if(data.almOut & (1LL << j++))	buf[i] = '1';
					else							buf[i] = '0';
				}
				g_value_init(&set_value, G_TYPE_STRING);
				g_value_set_string(&set_value, buf);
				if(!nf_sysdb_set_key0("act.sys.net.ddns_fail.arout", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.buzzer);
				if(!nf_sysdb_set_key0("act.sys.net.ddns_fail.buzzer", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.email);
				if(!nf_sysdb_set_key0("act.sys.net.ddns_fail.email", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.ftp);
				if(!nf_sysdb_set_key0("act.sys.net.ddns_fail.ftp", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.mobile);
				if(!nf_sysdb_set_key0("act.sys.net.ddns_fail.mobile", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);

				g_value_init(&set_value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&set_value, data.mobilepush);
				if(!nf_sysdb_set_key0("act.sys.net.ddns_fail.mobilepush", &set_value, NULL))
				{
					g_value_unset(&set_value);
					nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
					DDEBUG_FAIL
					return FALSE;
				}
				g_value_unset(&set_value);
			}
			break;
	}

	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);

	return TRUE;
}

gboolean DAL_set_SysNet_data_all(EA_SysNetData *data)
{
	GValue set_value = {0,};
	gchar buf[49];
	SysNetDataType data_type;
	gint i, j;

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	DDEBUG_START

	for(data_type=0; data_type<SYS_NET_EVT_CNT; data_type++)
	{
		if(data_type == INTERNET_TROUBLE_EVT_DATA)
		{
			memset(buf, 0x00, sizeof(buf));
			for(i=0, j=0; i<48; i++)
			{
				if(i >= g_ch) {
					if(i < 32) {
						buf[i] = '0';
						continue;
					}
				}

				if(data[data_type].almOut & (1LL << j++))	buf[i] = '1';
				else							buf[i] = '0';
			}
			g_value_init(&set_value, G_TYPE_STRING);
			g_value_set_string(&set_value, buf);
			if(!nf_sysdb_set_key0("act.sys.net.eth_trouble.arout", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].buzzer);
			if(!nf_sysdb_set_key0("act.sys.net.eth_trouble.buzzer", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].osd);
			if(!nf_sysdb_set_key0("act.sys.net.eth_trouble.osdpopup", &set_value, NULL))
			{
			    g_value_unset(&set_value);
			    nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			    DDEBUG_FAIL
			        return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].email);
			if(!nf_sysdb_set_key0("act.sys.net.eth_trouble.email", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].ftp);
			if(!nf_sysdb_set_key0("act.sys.net.eth_trouble.ftp", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].mobile);
			if(!nf_sysdb_set_key0("act.sys.net.eth_trouble.mobile", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].mobilepush);
			if(!nf_sysdb_set_key0("act.sys.net.eth_trouble.mobilepush", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);
		}
		else if(data_type == REMOTE_LOGON_FAIL_EVT_DATA)
		{
			memset(buf, 0x00, sizeof(buf));
			for(i=0, j=0; i<48; i++)
			{
				if(i >= g_ch) {
					if(i < 32) {
						buf[i] = '0';
						continue;
					}
				}

				if(data[data_type].almOut & (1LL << j++))	buf[i] = '1';
				else							buf[i] = '0';
			}
			g_value_init(&set_value, G_TYPE_STRING);
			g_value_set_string(&set_value, buf);
			if(!nf_sysdb_set_key0("act.sys.net.rfail.arout", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].buzzer);
			if(!nf_sysdb_set_key0("act.sys.net.rfail.buzzer", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].osd);
		    if(!nf_sysdb_set_key0("act.sys.net.rfail.osdpopup", &set_value, NULL))
		    {
                g_value_unset(&set_value);
                nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
                DDEBUG_FAIL
                    return FALSE;
		    }
		    g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].email);
			if(!nf_sysdb_set_key0("act.sys.net.rfail.email", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].ftp);
			if(!nf_sysdb_set_key0("act.sys.net.rfail.ftp", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].mobile);
			if(!nf_sysdb_set_key0("act.sys.net.rfail.mobile", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].mobilepush);
			if(!nf_sysdb_set_key0("act.sys.net.rfail.mobilepush", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);
		}
		else if(data_type == DDNS_UPDATE_FAIL_EVT_DATA)
		{
			memset(buf, 0x00, sizeof(buf));
			for(i=0, j=0; i<48; i++)
			{
				if(i >= g_ch) {
					if(i < 32) {
						buf[i] = '0';
						continue;
					}
				}

				if(data[data_type].almOut & (1LL << j++))	buf[i] = '1';
				else							buf[i] = '0';
			}
			g_value_init(&set_value, G_TYPE_STRING);
			g_value_set_string(&set_value, buf);
			if(!nf_sysdb_set_key0("act.sys.net.ddns_fail.arout", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].buzzer);
			if(!nf_sysdb_set_key0("act.sys.net.ddns_fail.buzzer", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].osd);
			if(!nf_sysdb_set_key0("act.sys.net.ddns_fail.osdpopup", &set_value, NULL))
			{
			    g_value_unset(&set_value);
			    nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			    DDEBUG_FAIL
			        return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].email);
			if(!nf_sysdb_set_key0("act.sys.net.ddns_fail.email", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].ftp);
			if(!nf_sysdb_set_key0("act.sys.net.ddns_fail.ftp", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].mobile);
			if(!nf_sysdb_set_key0("act.sys.net.ddns_fail.mobile", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].mobilepush);
			if(!nf_sysdb_set_key0("act.sys.net.ddns_fail.mobilepush", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);
		}
		else if(data_type == IP_CONFLICT_EVENT)
		{
		    memset(buf, 0x00, sizeof(buf));
			for(i=0, j=0; i<48; i++)
			{
				if(i >= g_ch) {
					if(i < 32) {
						buf[i] = '0';
						continue;
					}
				}

				if(data[data_type].almOut & (1LL << j++))	buf[i] = '1';
				else							buf[i] = '0';
			}
			g_value_init(&set_value, G_TYPE_STRING);
			g_value_set_string(&set_value, buf);
			if(!nf_sysdb_set_key0("act.sys.net.dhcp_fail.arout", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].buzzer);
			if(!nf_sysdb_set_key0("act_sys.net.dhcp_fail.buzzer", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].osd);
			if(!nf_sysdb_set_key0("act_sys_net.dhcp_fail.osdpopup", &set_value, NULL))
			{
			    g_value_unset(&set_value);
			    nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			    DDEBUG_FAIL
    			    return FALSE;
			}
            g_value_unset(&set_value);

            g_value_init(&set_value, G_TYPE_BOOLEAN);
            g_value_set_boolean(&set_value, data[data_type].email);
            if(!nf_sysdb_set_key0("act.sys.net.dhcp_fail.email", &set_value, NULL))
            {
                g_value_unset(&set_value);
                nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
                DDEBUG_FAIL
                    return FALSE;
            }
            g_value_unset(&set_value);

            g_value_init(&set_value, G_TYPE_BOOLEAN);
            g_value_set_boolean(&set_value, data[data_type].ftp);
            if(!nf_sysdb_set_key0("act.sys.net.dhcp_fail.ftp", &set_value, NULL))
            {
                g_value_unset(&set_value);
                nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
                DDEBUG_FAIL
                    return FALSE;
            }
            g_value_unset(&set_value);

            g_value_init(&set_value, G_TYPE_BOOLEAN);
            g_value_set_boolean(&set_value, data[data_type].mobile);
            if(!nf_sysdb_set_key0("act.sys.net.dhcp_fail.mobile", &set_value, NULL))
            {
                g_value_unset(&set_value);
                nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
                DDEBUG_FAIL
                    return FALSE;
            }
            g_value_unset(&set_value);

            g_value_init(&set_value, G_TYPE_BOOLEAN);
            g_value_set_boolean(&set_value, data[data_type].mobilepush);
            if(!nf_sysdb_set_key0("act.sys.net.dhcp_fail.mobilepush", &set_value, NULL))
            {
                g_value_unset(&set_value);
                nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
                DDEBUG_FAIL
                    return FALSE;
            }
            g_value_unset(&set_value);
		}
#if defined(_SUPPORT_AIBOX)
		else if(data_type == AIBOX_TROUBLE_EVT_DATA)
		{
		    memset(buf, 0x00, sizeof(buf));
			for(i=0, j=0; i<48; i++)
			{
				if(i >= g_ch) {
					if(i < 32) {
						buf[i] = '0';
						continue;
					}
				}

				if(data[data_type].almOut & (1LL << j++))	buf[i] = '1';
				else							buf[i] = '0';
		    }
			g_value_init(&set_value, G_TYPE_STRING);
			g_value_set_string(&set_value, buf);
			if(!nf_sysdb_set_key0("act.sys.net.aibox_trouble.arout", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].buzzer);
			if(!nf_sysdb_set_key0("act_sys.net.aibox_trouble.buzzer", &set_value, NULL))
			{
				g_value_unset(&set_value);
				nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
				DDEBUG_FAIL
					return FALSE;
			}
			g_value_unset(&set_value);

			g_value_init(&set_value, G_TYPE_BOOLEAN);
			g_value_set_boolean(&set_value, data[data_type].osd);
			if(!nf_sysdb_set_key0("act_sys_net.aibox_trouble.osdpopup", &set_value, NULL))
			{
			    g_value_unset(&set_value);
			    nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			    DDEBUG_FAIL
    			    return FALSE;
			}
            g_value_unset(&set_value);

            g_value_init(&set_value, G_TYPE_BOOLEAN);
            g_value_set_boolean(&set_value, data[data_type].email);
            if(!nf_sysdb_set_key0("act.sys.net.aibox_trouble.email", &set_value, NULL))
            {
                g_value_unset(&set_value);
                nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
                DDEBUG_FAIL
                    return FALSE;
            }
            g_value_unset(&set_value);

            g_value_init(&set_value, G_TYPE_BOOLEAN);
            g_value_set_boolean(&set_value, data[data_type].ftp);
            if(!nf_sysdb_set_key0("act.sys.net.aibox_trouble.ftp", &set_value, NULL))
            {
                g_value_unset(&set_value);
                nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
                DDEBUG_FAIL
                    return FALSE;
            }
            g_value_unset(&set_value);

            g_value_init(&set_value, G_TYPE_BOOLEAN);
            g_value_set_boolean(&set_value, data[data_type].mobile);
            if(!nf_sysdb_set_key0("act.sys.net.aibox_trouble.mobile", &set_value, NULL))
            {
                g_value_unset(&set_value);
                nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
                DDEBUG_FAIL
                    return FALSE;
            }
            g_value_unset(&set_value);

            g_value_init(&set_value, G_TYPE_BOOLEAN);
            g_value_set_boolean(&set_value, data[data_type].mobilepush);
            if(!nf_sysdb_set_key0("act.sys.net.aibox_trouble.mobilepush", &set_value, NULL))
            {
                g_value_unset(&set_value);
                nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
                DDEBUG_FAIL
                    return FALSE;
            }
            g_value_unset(&set_value);
		}
#endif		
	}

	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);

	return TRUE;
}

gboolean DAL_get_SysNet_data(EA_SysNetData *data, SysNetDataType data_type)
{
	GValue ret_value = {0,};
	const gchar *str = NULL;
	gint i, j;

	switch(data_type) {
		case INTERNET_TROUBLE_EVT_DATA:
			{
				if(nf_sysdb_get_key0("act.sys.net.eth_trouble.arout", &ret_value, NULL))
				{
					str = g_value_get_string(&ret_value);
					for(i=0, j=0; i<48, j<48; i++)
					{
						if(i == g_ch) j = 32;

						if(str[j++] == '1')	data->almOut |= (1LL << i);
						else				data->almOut &= ~(1LL << i);
					}
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.net.eth_trouble.buzzer", &ret_value, NULL))
				{
					data->buzzer = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.net.eth_trouble.osdpopup", &ret_value, NULL))
				{
				    data->osd = g_value_get_boolean(&ret_value);
				    g_value_unset(&ret_value);
				}
				else
				    return FALSE;

				if(nf_sysdb_get_key0("act.sys.net.eth_trouble.email", &ret_value, NULL))
				{
					data->email = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.net.eth_trouble.ftp", &ret_value, NULL))
				{
					data->ftp = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.net.eth_trouble.mobile", &ret_value, NULL))
				{
					data->mobile = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.net.eth_trouble.mobilepush", &ret_value, NULL))
				{
					data->mobilepush = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;
			}
			break;

		case REMOTE_LOGON_FAIL_EVT_DATA:
			{
				if(nf_sysdb_get_key0("act.sys.net.rfail.arout", &ret_value, NULL))
				{
					str = g_value_get_string(&ret_value);
					for(i=0, j=0; i<48, j<48; i++)
					{
						if(i == g_ch) j = 32;

						if(str[j++] == '1')	data->almOut |= (1LL << i);
						else				data->almOut &= ~(1LL << i);
					}
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.net.rfail.buzzer", &ret_value, NULL))
				{
					data->buzzer = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

			    if(nf_sysdb_get_key0("act.sys.net.rfail.osdpopup", &ret_value, NULL))
			    {
			        data->osd = g_value_get_boolean(&ret_value);
			        g_value_unset(&ret_value);
			    }
			    else
			        return FALSE;

				if(nf_sysdb_get_key0("act.sys.net.rfail.email", &ret_value, NULL))
				{
					data->email = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.net.rfail.ftp", &ret_value, NULL))
				{
					data->ftp = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.net.rfail.mobile", &ret_value, NULL))
				{
					data->mobile = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.net.rfail.mobilepush", &ret_value, NULL))
				{
					data->mobilepush = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;
			}
			break;

		case DDNS_UPDATE_FAIL_EVT_DATA:
			{
				if(nf_sysdb_get_key0("act.sys.net.ddns_fail.arout", &ret_value, NULL))
				{
					str = g_value_get_string(&ret_value);
					for(i=0, j=0; i<48, j<48; i++)
					{
						if(i == g_ch) j = 32;

						if(str[j++] == '1')	data->almOut |= (1LL << i);
						else				data->almOut &= ~(1LL << i);
					}
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.net.ddns_fail.buzzer", &ret_value, NULL))
				{
					data->buzzer = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

			    if(nf_sysdb_get_key0("act.sys.net.ddns_fail.osdpopup", &ret_value, NULL))
			    {
			        data->osd = g_value_get_boolean(&ret_value);
			        g_value_unset(&ret_value);
			    }
			    else
			        return FALSE;

				if(nf_sysdb_get_key0("act.sys.net.ddns_fail.email", &ret_value, NULL))
				{
					data->email = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.net.ddns_fail.ftp", &ret_value, NULL))
				{
					data->ftp = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.net.ddns_fail.mobile", &ret_value, NULL))
				{
					data->mobile = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.net.ddns_fail.mobilepush", &ret_value, NULL))
				{
					data->mobilepush = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;
			}
			break;

		case IP_CONFLICT_EVENT :
		    {
		        if(nf_sysdb_get_key0("act.sys.net.dhcp_fail.arout", &ret_value, NULL))
				{
					str = g_value_get_string(&ret_value);
					for(i=0, j=0; i<48, j<48; i++)
					{
						if(i == g_ch) j = 32;

						if(str[j++] == '1')	data->almOut |= (1LL << i);
						else				data->almOut &= ~(1LL << i);
					}
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.net.dhcp_fail.buzzer", &ret_value, NULL))
				{
					data->buzzer= g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

			    if(nf_sysdb_get_key0("act.sys.net.dhcp_fail.osdpopup", &ret_value, NULL))
				{
					data->osd = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.net.dhcp_fail.email", &ret_value, NULL))
				{
					data->email = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

			    if(nf_sysdb_get_key0("act.sys.net.dhcp_fail.ftp", &ret_value, NULL))
				{
					data->ftp = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.net.dhcp_fail.mobile", &ret_value, NULL))
				{
					data->mobile = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

			    if(nf_sysdb_get_key0("act.sys.net.dhcp_fail.mobilepush", &ret_value, NULL))
				{
					data->mobilepush = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;
		    }
		    break;
#if defined(_SUPPORT_AIBOX)
		case AIBOX_TROUBLE_EVT_DATA:
			{
				if(nf_sysdb_get_key0("act.sys.net.aibox_trouble.arout", &ret_value, NULL))
				{
					str = g_value_get_string(&ret_value);
					for(i=0, j=0; i<48, j<48; i++)
					{
						if(i == g_ch) j = 32;

						if(str[j++] == '1')	data->almOut |= (1LL << i);
						else				data->almOut &= ~(1LL << i);
					}
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.net.aibox_trouble.buzzer", &ret_value, NULL))
				{
					data->buzzer = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.net.aibox_trouble.osdpopup", &ret_value, NULL))
				{
				    data->osd = g_value_get_boolean(&ret_value);
				    g_value_unset(&ret_value);
				}
				else
				    return FALSE;

				if(nf_sysdb_get_key0("act.sys.net.aibox_trouble.email", &ret_value, NULL))
				{
					data->email = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.net.aibox_trouble.ftp", &ret_value, NULL))
				{
					data->ftp = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.net.aibox_trouble.mobile", &ret_value, NULL))
				{
					data->mobile = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;

				if(nf_sysdb_get_key0("act.sys.net.aibox_trouble.mobilepush", &ret_value, NULL))
				{
					data->mobilepush = g_value_get_boolean(&ret_value);
					g_value_unset(&ret_value);
				}
				else
					return FALSE;
			}
			break;	
#endif
	}

	return TRUE;
}


gboolean DAL_set_SysNet_FCount(EA_SysNetFailCnt data)
{
	GValue set_value = {0,};

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	DDEBUG_START

	g_value_init(&set_value, G_TYPE_INT);
	g_value_set_int(&set_value, data.rfail_cnt);
	if(!nf_sysdb_set_key0("act.sys.net.rfail.cnt", &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_INT);
	g_value_set_int(&set_value, data.dfail_cnt);
	if(!nf_sysdb_set_key0("act.sys.net.ddns_fail.cnt", &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);

	return TRUE;
}

gboolean DAL_get_SysNet_FCount(EA_SysNetFailCnt *data)
{
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("act.sys.net.rfail.cnt", &ret_value, NULL))
	{
		data->rfail_cnt = g_value_get_int(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key0("act.sys.net.ddns_fail.cnt", &ret_value, NULL))
	{
		data->dfail_cnt = g_value_get_int(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	return TRUE;
}


/********************************************************************************
 * Setup --> Event&Action --> Tamper Event
 * ******************************************************************************/
gint DAL_set_tamper_data(EA_TamperData data, guint ch)
{
	GValue set_value = {0,};
	gchar buf[49];
	gint i, j;

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	DDEBUG_START

	g_value_init(&set_value, G_TYPE_INT);
	g_value_set_int(&set_value, data.interval);
	if(!nf_sysdb_set_key1("act.tamper.T%d.interval", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	for(i=0; i<32; i++)
	{
		if(data.linkCam & (1 << i))	buf[i] = '1';
		else						buf[i] = '0';
	}
	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);
	if(!nf_sysdb_set_key1("act.tamper.T%d.lcamera", (ch >= g_ch ? ch - g_ch + 32 : ch), &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	for(i=0, j=0; i<48; i++)
	{
		if(i >= g_ch) {
			if(i < 32) {
				buf[i] = '0';
				continue;
			}
		}

		if(data.almOut & (1LL << j++))	buf[i] = '1';
		else						buf[i] = '0';
	}
	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);
	if(!nf_sysdb_set_key1("act.tamper.T%d.arout", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.buzzer);
	if(!nf_sysdb_set_key1("act.tamper.T%d.buzzer", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.vpop);
	if(!nf_sysdb_set_key1("act.tamper.T%d.vpopup", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.email);
	if(!nf_sysdb_set_key1("act.tamper.T%d.email", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.ftp);
	if(!nf_sysdb_set_key1("act.tamper.T%d.ftp", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 7;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.mobile);
	if(!nf_sysdb_set_key1("act.tamper.T%d.mobile", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 8;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.mobilepush);
	if(!nf_sysdb_set_key1("act.tamper.T%d.mobilepush", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return 9;
	}
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);

	return 0;
}

gint DAL_get_tamper_data(EA_TamperData *data, guint ch)
{
	GValue ret_value = {0,};
	const gchar *str = NULL;
	gint i, j;

	if(nf_sysdb_get_key1("act.tamper.T%d.interval", ch, &ret_value, NULL))
	{
		data->interval = g_value_get_int(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return 1;

	if(nf_sysdb_get_key1("act.tamper.T%d.lcamera", (ch >= g_ch ? ch - g_ch + 32 : ch), &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);
		for(i=0; i<32; i++)
		{
			if(str[i] == '1')	data->linkCam |= (1 << i);
			else				data->linkCam &= ~(1 << i);
		}
		g_value_unset(&ret_value);
	}
	else
		return 2;

	if(nf_sysdb_get_key1("act.tamper.T%d.arout", ch, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);
		for(i=0, j=0; i<48, j<48; i++)
		{
			if(i == g_ch) j = 32;

			if(str[j++] == '1')	data->almOut |= (1LL << i);
			else				data->almOut &= ~(1LL << i);
		}
		g_value_unset(&ret_value);
	}
	else
		return 3;

	if(nf_sysdb_get_key1("act.tamper.T%d.buzzer", ch, &ret_value, NULL))
	{
		data->buzzer = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return 4;

	if(nf_sysdb_get_key1("act.tamper.T%d.vpopup", ch, &ret_value, NULL))
	{
		data->vpop = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return 5;

	if(nf_sysdb_get_key1("act.tamper.T%d.email", ch, &ret_value, NULL))
	{
		data->email = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return 6;

	if(nf_sysdb_get_key1("act.tamper.T%d.ftp", ch, &ret_value, NULL))
	{
		data->ftp = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return 7;

	if(nf_sysdb_get_key1("act.tamper.T%d.mobile", ch, &ret_value, NULL))
	{
		data->mobile = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return 8;

	if(nf_sysdb_get_key1("act.tamper.T%d.mobilepush", ch, &ret_value, NULL))
	{
		data->mobilepush = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return 9;

	return 0;
}

gint DAL_set_tamper_all_data(EA_TamperData *data, guint max_ch)
{
	GValue set_value = {0,};
	gchar buf[49];
	gint i, j, k;

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	DDEBUG_START

	for(k=0; k<max_ch; k++) {
		g_value_init(&set_value, G_TYPE_INT);
		g_value_set_int(&set_value, data[k].interval);
		if(!nf_sysdb_set_key1("act.tamper.T%d.interval", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
			return 1;
		}
		g_value_unset(&set_value);

		memset(buf, 0, sizeof(buf));
		for(i=0; i<32; i++)
		{
			if(data[k].linkCam & (1 << i))	buf[i] = '1';
			else							buf[i] = '0';
		}
		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, buf);
		if(!nf_sysdb_set_key1("act.tamper.T%d.lcamera", (k >= g_ch ? k - g_ch + 32 : k), &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
			return 2;
		}
		g_value_unset(&set_value);

		memset(buf, 0, sizeof(buf));
		for(i=0, j=0; i<48; i++)
		{
			if(i >= g_ch) {
				if(i < 32) {
					buf[i] = '0';
					continue;
				}
			}

			if(data[k].almOut & (1LL << j++))	buf[i] = '1';
			else							buf[i] = '0';
		}
		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, buf);
		if(!nf_sysdb_set_key1("act.tamper.T%d.arout", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
			return 3;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].buzzer);
		if(!nf_sysdb_set_key1("act.tamper.T%d.buzzer", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
			return 4;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].vpop);
		if(!nf_sysdb_set_key1("act.tamper.T%d.vpopup", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
			return 5;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].email);
		if(!nf_sysdb_set_key1("act.tamper.T%d.email", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
			return 6;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].ftp);
		if(!nf_sysdb_set_key1("act.tamper.T%d.ftp", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
			return 7;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].mobile);
		if(!nf_sysdb_set_key1("act.tamper.T%d.mobile", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
			return 8;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].mobilepush);
		if(!nf_sysdb_set_key1("act.tamper.T%d.mobilepush", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
			return 9;
		}
		g_value_unset(&set_value);
	}

	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);

	return 0;
}


/***************************************************************
 * Setup --> Event&Action --> VCA Event
 * *************************************************************/
gboolean DAL_set_VCAEvt_data(EA_VCAEvtData data, guint ch)
{
	GValue set_value = {0,};
	gchar buf[49];
	gint i, j;

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	DDEBUG_START

	g_value_init(&set_value, G_TYPE_INT);
	g_value_set_int(&set_value, data.interval);
	if(!nf_sysdb_set_key1("act.vca.V%d.interval", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	for(i=0, j=0; i<48; i++)
	{
		if(i >= g_ch) {
			if(i < 32) {
				buf[i] = '0';
				continue;
			}
		}

		if(data.almOut & (1LL << j++))	buf[i] = '1';
		else						buf[i] = '0';
	}
	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);
	if(!nf_sysdb_set_key1("act.vca.V%d.arout", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.buzzer);
	if(!nf_sysdb_set_key1("act.vca.V%d.buzzer", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.vpop);
	if(!nf_sysdb_set_key1("act.vca.V%d.vpopup", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.email);
	if(!nf_sysdb_set_key1("act.vca.V%d.email", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.ftp);
	if(!nf_sysdb_set_key1("act.vca.V%d.ftp", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.preset);
	if(!nf_sysdb_set_key1("act.vca.V%d.preset", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

    for (j = 0; j < 32; j++)
    {
    	g_value_init(&set_value, G_TYPE_UINT);
    	g_value_set_uint(&set_value, data.preset_num[j]);
    	if(!nf_sysdb_set_key2("act.vca.V%d.cam.C%d.preset_num", ch, j, &set_value, NULL))
    	{
    		g_value_unset(&set_value);
    		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
    		DDEBUG_FAIL
    		return FALSE;
    	}
    	g_value_unset(&set_value);

    }

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.mobile);
	if(!nf_sysdb_set_key1("act.vca.V%d.mobile", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data.mobilepush);
	if(!nf_sysdb_set_key1("act.vca.V%d.mobilepush", ch, &set_value, NULL))
	{
		g_value_unset(&set_value);
		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);

	return TRUE;
}

gboolean DAL_set_VCAEvt_data_all(EA_VCAEvtData *data, guint ch)
{
	GValue set_value = {0,};
	gchar buf[49];
	gint i, j, k;

	nf_sysdb_lock(NF_SYSDB_CATE_ACT);

	DDEBUG_START

	for(k=0; k<ch; k++) {
		g_value_init(&set_value, G_TYPE_INT);
		g_value_set_int(&set_value, data[k].interval);
		if(!nf_sysdb_set_key1("act.vca.V%d.interval", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
			return FALSE;
		}
		g_value_unset(&set_value);

		memset(buf, 0, sizeof(buf));
		for(i=0, j=0; i<48; i++)
		{
			if(i >= g_ch) {
				if(i < 32) {
					buf[i] = '0';
					continue;
				}
			}

			if(data[k].almOut & (1LL << j++))	buf[i] = '1';
			else						buf[i] = '0';
		}
		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value, buf);
		if(!nf_sysdb_set_key1("act.vca.V%d.arout", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
			return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].buzzer);
		if(!nf_sysdb_set_key1("act.vca.V%d.buzzer", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
			return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].vpop);
		if(!nf_sysdb_set_key1("act.vca.V%d.vpopup", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
			return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].email);
		if(!nf_sysdb_set_key1("act.vca.V%d.email", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
			return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].ftp);
		if(!nf_sysdb_set_key1("act.vca.V%d.ftp", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
			return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].preset);
		if(!nf_sysdb_set_key1("act.vca.V%d.preset", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
			return FALSE;
		}
		g_value_unset(&set_value);

        for (j = 0; j < 32; j++)
        {
        	g_value_init(&set_value, G_TYPE_UINT);
        	g_value_set_uint(&set_value, data[k].preset_num[j]);
        	if(!nf_sysdb_set_key2("act.vca.V%d.cam.C%d.preset_num", k, j, &set_value, NULL))
        	{
        		g_value_unset(&set_value);
        		nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
        		DDEBUG_FAIL
        		return FALSE;
        	}
        	g_value_unset(&set_value);

        }

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].mobile);
		if(!nf_sysdb_set_key1("act.vca.V%d.mobile", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
			return FALSE;
		}
		g_value_unset(&set_value);

		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, data[k].mobilepush);
		if(!nf_sysdb_set_key1("act.vca.V%d.mobilepush", k, &set_value, NULL))
		{
			g_value_unset(&set_value);
			nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
			DDEBUG_FAIL
			return FALSE;
		}
		g_value_unset(&set_value);
	}

	nf_sysdb_unlock(NF_SYSDB_CATE_ACT);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);

	return TRUE;
}

gboolean DAL_get_VCAEvt_data(EA_VCAEvtData *data, guint ch)
{
	GValue ret_value = {0,};
	const gchar *str = NULL;
	gint i, j;

	if(nf_sysdb_get_key1("act.vca.V%d.interval", ch, &ret_value, NULL))
	{
		data->interval = g_value_get_int(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.vca.V%d.arout", ch, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);
		for(i=0, j=0; i<48, j<48; i++)
		{
			if(i == g_ch) j = 32;

			if(str[j++] == '1')	data->almOut |= (1LL << i);
			else				data->almOut &= ~(1LL << i);
		}
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.vca.V%d.buzzer", ch, &ret_value, NULL))
	{
		data->buzzer = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.vca.V%d.vpopup", ch, &ret_value, NULL))
	{
		data->vpop = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.vca.V%d.email", ch, &ret_value, NULL))
	{
		data->email = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.vca.V%d.ftp", ch, &ret_value, NULL))
	{
		data->ftp = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.vca.V%d.preset", ch, &ret_value, NULL))
	{
		data->preset = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

    for (j = 0; j < 32; j++)
    {
    	if(nf_sysdb_get_key2("act.vca.V%d.cam.C%d.preset_num", ch, j, &ret_value, NULL))
    	{
    		data->preset_num[j] = g_value_get_uint(&ret_value);
    		g_value_unset(&ret_value);
    	}
    	else
    		return FALSE;
    }

	if(nf_sysdb_get_key1("act.vca.V%d.mobile", ch, &ret_value, NULL))
	{
		data->mobile = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	if(nf_sysdb_get_key1("act.vca.V%d.mobilepush", ch, &ret_value, NULL))
	{
		data->mobilepush = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		return FALSE;

	return TRUE;
}

gboolean DAL_get_VCAEvt_vPop_boolean(guint ch)
{
	GValue ret_value = {0,};
	gboolean ret = FALSE;

	if(nf_sysdb_get_key1("act.vca.V%d.vpopup", ch, &ret_value, NULL))
	{
		ret = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}

	return ret;
}

gint DAL_get_VCAEvt_ignor_interval(guint ch)
{
	GValue ret_value = {0,};
	gint ret = 0;

	if(nf_sysdb_get_key1("act.vca.V%d.interval", ch, &ret_value, NULL))
	{
		ret = g_value_get_int(&ret_value);
		g_value_unset(&ret_value);
	}

	return ret;
}


/********************************************************************************
 * Setup --> System --> Disk Manage
 * ******************************************************************************/

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_get_diskManage_data(DiskManageData *data)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("disk.rtime_limit", &ret_value, NULL))
	{
		data->timeLimit = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	if(nf_sysdb_get_key0("disk.write_mode", &ret_value, NULL))
	{
		data->overWrite = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 2;


	if(nf_sysdb_get_key0("disk.rtime_type", &ret_value, NULL))
	{
		data->timeType = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 3;

	if(nf_sysdb_get_key0("disk.custom_calendar", &ret_value, NULL))
	{
		data->custom_cal = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 4;


	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_set_diskManage_data(DiskManageData *data)
{
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_DISK);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->timeLimit);
	if(!nf_sysdb_set_key0("disk.rtime_limit", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISK);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->overWrite);
	if(!nf_sysdb_set_key0("disk.write_mode", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISK);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->timeType);
	if(!nf_sysdb_set_key0("disk.rtime_type", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISK);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->custom_cal);
	if(!nf_sysdb_set_key0("disk.custom_calendar", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISK);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_DISK, 0, 0, 0);
	nf_sysdb_unlock(NF_SYSDB_CATE_DISK);

	DDEBUG_END
	return 0;
}

guint DAL_get_rtime_limit()
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("disk.rtime_limit", &ret_value, NULL))
	{
		ret = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 0;

	return ret;
}

guint DAL_get_disk_write_mode()
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("disk.write_mode", &ret_value, NULL))
	{
		ret = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 2;

	return ret;
}

guint DAL_set_disk_smart_interval(guint interval)
{
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_DISK);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, interval);
	if(!nf_sysdb_set_key0("disk.smart_chk", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISK);
		DDEBUG_FAIL
		return 0;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_DISK, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_DISK);

	DDEBUG_END
	return 1;
}

guint DAL_get_disk_smart_interval()
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("disk.smart_chk", &ret_value, NULL))
	{
		ret = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else
		ret = 25;

	return ret;
}






/********************************************************************************
 * Setup --> Record Setup --> Recording Operations
 * ******************************************************************************/

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
gboolean DAL_get_recordOper_data(RecordOperData *data)
{
	GValue ret_value = {0};
	GType gtype;

	if(nf_sysdb_get_key0("rec.mode", &ret_value, NULL))
	{
		data->mode = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.mode value");

		DDEBUG_FAIL
		return FALSE;
	}

	if(nf_sysdb_get_key0("rec.priority_mode", &ret_value, NULL))
	{
		data->priMode = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.priority_mode value");

		DDEBUG_FAIL
		return FALSE;
	}

	if(nf_sysdb_get_key0("rec.auto_config", &ret_value, NULL))
	{
		data->autoConfig = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.auto_config value");

		DDEBUG_FAIL
		return FALSE;
	}

	if(nf_sysdb_get_key0("rec.sched_mode", &ret_value, NULL))
	{
		data->schedMode = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.sched_mode value");

		DDEBUG_FAIL
		return FALSE;
	}

	if(nf_sysdb_get_key0("rec.pre_rec_time", &ret_value, NULL))
	{
		data->preRecTime = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.pre_rec_time value");

		DDEBUG_FAIL
		return FALSE;
	}

	if(nf_sysdb_get_key0("rec.post_rec_time", &ret_value, NULL))
	{
		data->postRecTime = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.post_rec_time value");

		DDEBUG_FAIL
		return FALSE;
	}

	if(nf_sysdb_get_key0("rec.panic_time", &ret_value, NULL))
	{
		data->panicTime = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.panic_time value");

		DDEBUG_FAIL
		return FALSE;
	}

	if(nf_sysdb_get_key0("rec.smart_storage", &ret_value, NULL))
	{
		data->smart_storage = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.smart_storage value");

		DDEBUG_FAIL
		return FALSE;
	}

	return TRUE;
	//	return error;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
gboolean DAL_set_recordOper_data(RecordOperData *data)
{
	GValue set_value = {0};

	DDEBUG_START

	g_return_val_if_fail(data != NULL, FALSE);

	nf_sysdb_lock(NF_SYSDB_CATE_REC);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->mode);
	if(!nf_sysdb_set_key0("rec.mode", &set_value, NULL)) {
		g_warning("couldn't set a rec.mode value");

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->priMode);
	if(!nf_sysdb_set_key0("rec.priority_mode", &set_value, NULL)) {
		g_warning("couldn't set a rec.priority_mode value");

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->autoConfig);
	if(!nf_sysdb_set_key0("rec.auto_config", &set_value, NULL)) {
		g_warning("couldn't set a rec.auto_config value");

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->schedMode);
	if(!nf_sysdb_set_key0("rec.sched_mode", &set_value, NULL)) {
		g_warning("couldn't set a rec.sched_mode value");

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->preRecTime);
	if(!nf_sysdb_set_key0("rec.pre_rec_time", &set_value, NULL)) {
		g_warning("couldn't set a rec.pre_rec_time value");

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->postRecTime);
	if(!nf_sysdb_set_key0("rec.post_rec_time", &set_value, NULL)) {
		g_warning("couldn't set a rec.post_rec_time value");

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->panicTime);
	if(!nf_sysdb_set_key0("rec.panic_time", &set_value, NULL)) {
		g_warning("couldn't set a rec.panic_time value");

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->smart_storage);
	if(!nf_sysdb_set_key0("rec.smart_storage", &set_value, NULL)) {
		g_warning("couldn't set a rec.smart_storage value");

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);
	
	nf_sysdb_unlock(NF_SYSDB_CATE_REC);

	DDEBUG_END
	return TRUE;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
gboolean DAL_get_recordOper_mode_data(guint *data)
{
	GValue ret_value = {0};

	if(nf_sysdb_get_key0("rec.mode", &ret_value, NULL))
	{
		*data = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.mode value");

		DDEBUG_FAIL
		return FALSE;
	}

	return TRUE;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
gboolean DAL_set_recordOper_mode_data(RecordOperData *data)
{
	GValue set_value = {0};

	DDEBUG_START

	g_return_val_if_fail(data != NULL, FALSE);

	nf_sysdb_lock(NF_SYSDB_CATE_REC);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->mode);
	if(!nf_sysdb_set_key0("rec.mode", &set_value, NULL)) {
		g_warning("couldn't set a rec.mode value");

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_REC);

	DDEBUG_END
	return TRUE;
}


/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
gboolean DAL_get_recordContParam_data(RecordParamData *data, guint key)
{
	GValue ret_value = {0};
	const gchar *str = NULL;
	gchar buf[769];

	g_return_val_if_fail(data != NULL, FALSE);


	if(nf_sysdb_get_key1("rec.continuous.C%d.size", key, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);

		memset(buf, 0, sizeof(buf));
		g_stpcpy(buf, str);

		memcpy(data->sizeParam[0], buf, CAMERA_CHANNEL * HOURS_A_DAY);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.continuous.C%d.size value", key);

		DDEBUG_FAIL
		return FALSE;
	}

	if(nf_sysdb_get_key1("rec.continuous.C%d.fps", key, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);

		memset(buf, 0, sizeof(buf));
		g_stpcpy(buf, str);

		memcpy(data->fpsParam[0], buf, CAMERA_CHANNEL * HOURS_A_DAY);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.continuous.C%d.fps value", key);

		DDEBUG_FAIL
		return FALSE;
	}

	if(nf_sysdb_get_key1("rec.continuous.C%d.quality", key, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);

		memset(buf, 0, sizeof(buf));
		g_stpcpy(buf, str);

		memcpy(data->qualParam[0], buf, CAMERA_CHANNEL * HOURS_A_DAY);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.continuous.C%d.quality value", key);

		DDEBUG_FAIL
		return FALSE;
	}

	if(nf_sysdb_get_key1("rec.continuous.C%d.audio", key, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);

		memset(buf, 0, sizeof(buf));
		g_stpcpy(buf, str);

		memcpy(data->audioParam[0], buf, CAMERA_CHANNEL * HOURS_A_DAY);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.continuous.C%d.audio value", key);

		DDEBUG_FAIL
		return FALSE;
	}

	if(nf_sysdb_get_key1("rec.continuous.C%d.mode", key, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);

		memset(buf, 0, sizeof(buf));
		g_stpcpy(buf, str);

		memcpy(data->modeParam[0], buf, CAMERA_CHANNEL * HOURS_A_DAY);
		g_value_unset(&ret_value);

	}else {
		g_warning("couldn't get a rec.continuous.C%d.mode value", key);

		DDEBUG_FAIL
		return FALSE;
	}

	return TRUE;
}


/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
gboolean DAL_set_recordContParam_data(RecordParamData *data, guint key)
{
	GValue set_value = {0};
	gchar buf[769];
	gint i;

	DDEBUG_START

	g_return_val_if_fail(data != NULL, FALSE);

	nf_sysdb_lock(NF_SYSDB_CATE_REC);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->sizeParam[0], CAMERA_CHANNEL * HOURS_A_DAY);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key1("rec.continuous.C%d.size", key, &set_value, NULL)) {
		g_warning("couldn't set a rec.continuous.C%d.size value", key);
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);


	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->fpsParam[0], CAMERA_CHANNEL * HOURS_A_DAY);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key1("rec.continuous.C%d.fps", key, &set_value, NULL)) {
		g_warning("couldn't set a rec.continuous.C%d.fps value", key);
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);


	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->qualParam[0], CAMERA_CHANNEL * HOURS_A_DAY);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key1("rec.continuous.C%d.quality", key, &set_value, NULL)) {
		g_warning("couldn't set a rec.continuous.C%d.quality value", key);
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);



	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->audioParam[0], CAMERA_CHANNEL * HOURS_A_DAY);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key1("rec.continuous.C%d.audio", key, &set_value, NULL)) {
		g_warning("couldn't set a rec.continuous.C%d.audio value", key);
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);


	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->modeParam[0], CAMERA_CHANNEL * HOURS_A_DAY);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key1("rec.continuous.C%d.mode", key, &set_value, NULL)) {
		g_warning("couldn't set a rec.continuous.C%d.mode value", key);
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_REC);

	DDEBUG_END
	return TRUE;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
gboolean DAL_get_recordMotionParam_data(RecordParamData *data, guint key)
{
	GValue ret_value = {0};
	const gchar *str = NULL;
	gchar buf[769];

	g_return_val_if_fail(data != NULL, FALSE);


	if(nf_sysdb_get_key1("rec.motion.M%d.size", key, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);

		memset(buf, 0, sizeof(buf));
		g_stpcpy(buf, str);

		memcpy(data->sizeParam[0], buf, CAMERA_CHANNEL * HOURS_A_DAY);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.motion.M%d.size value", key);

		DDEBUG_FAIL
		return FALSE;
	}

	if(nf_sysdb_get_key1("rec.motion.M%d.fps", key, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);

		memset(buf, 0, sizeof(buf));
		g_stpcpy(buf, str);

		memcpy(data->fpsParam[0], buf, CAMERA_CHANNEL * HOURS_A_DAY);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.motion.M%d.fps value", key);

		DDEBUG_FAIL
		return FALSE;
	}

	if(nf_sysdb_get_key1("rec.motion.M%d.quality", key, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);

		memset(buf, 0, sizeof(buf));
		g_stpcpy(buf, str);

		memcpy(data->qualParam[0], buf, CAMERA_CHANNEL * HOURS_A_DAY);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.motion.M%d.quality value", key);

		DDEBUG_FAIL
		return FALSE;
	}

	if(nf_sysdb_get_key1("rec.motion.M%d.audio", key, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);

		memset(buf, 0, sizeof(buf));
		g_stpcpy(buf, str);

		memcpy(data->audioParam[0], buf, CAMERA_CHANNEL * HOURS_A_DAY);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.motion.M%d.audio value", key);

		DDEBUG_FAIL
		return FALSE;
	}

	if(nf_sysdb_get_key1("rec.motion.M%d.mode", key, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);

		memset(buf, 0, sizeof(buf));
		g_stpcpy(buf, str);

		memcpy(data->modeParam[0], buf, CAMERA_CHANNEL * HOURS_A_DAY);
		g_value_unset(&ret_value);

	}else {
		g_warning("couldn't get a rec.motion.M%d.mode value", key);

		DDEBUG_FAIL
		return FALSE;
	}

	return TRUE;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
gboolean DAL_set_recordMotionParam_data(RecordParamData *data, guint key)
{
	GValue set_value = {0};
	gchar buf[769];
	gint i;

	DDEBUG_START

	g_return_val_if_fail(data != NULL, FALSE);

	nf_sysdb_lock(NF_SYSDB_CATE_REC);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->sizeParam[0], CAMERA_CHANNEL * HOURS_A_DAY);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key1("rec.motion.M%d.size", key, &set_value, NULL)) {
		g_warning("couldn't set a rec.motion.M%d.size value", key);
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);


	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->fpsParam[0], CAMERA_CHANNEL * HOURS_A_DAY);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key1("rec.motion.M%d.fps", key, &set_value, NULL)) {
		g_warning("couldn't set a rec.motion.M%d.fps value", key);
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);


	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->qualParam[0], CAMERA_CHANNEL * HOURS_A_DAY);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key1("rec.motion.M%d.quality", key, &set_value, NULL)) {
		g_warning("couldn't set a rec.motion.M%d.quality value", key);
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);



	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->audioParam[0], CAMERA_CHANNEL * HOURS_A_DAY);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key1("rec.motion.M%d.audio", key, &set_value, NULL)) {
		g_warning("couldn't set a rec.motion.M%d.audio value", key);
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);


	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->modeParam[0], CAMERA_CHANNEL * HOURS_A_DAY);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key1("rec.motion.M%d.mode", key, &set_value, NULL)) {
		g_warning("couldn't set a rec.motion.M%d.mode value", key);
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_REC);

	DDEBUG_END
	return TRUE;
}


/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
#if defined(_ATM_1648) || defined(_ATM_1624) || defined(_ATM_0824) || defined(_ANF_0824CS) || defined(_OTM_1648) || defined(_OTM_0824)
gboolean DAL_get_recordAlarmParam_data(AlarmRecordParamData *data, guint key)
{
	GValue ret_value = {0};
	const gchar *str = NULL;
	gchar buf[769];

	g_return_val_if_fail(data != NULL, FALSE);

	if(nf_sysdb_get_key1("rec.alarm.A%d.size", key, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);

		memset(buf, 0, sizeof(buf));
		g_stpcpy(buf, str);

		memcpy(data->sizeParam[0], buf, CAMERA_CHANNEL * HOURS_A_DAY);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.alarm.A%d.size value", key);

		DDEBUG_FAIL
		return FALSE;
	}

	if(nf_sysdb_get_key1("rec.alarm.A%d.fps", key, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);

		memset(buf, 0, sizeof(buf));
		g_stpcpy(buf, str);

		memcpy(data->fpsParam[0], buf, CAMERA_CHANNEL * HOURS_A_DAY);
		g_value_unset(&ret_value);

	}else {
		g_warning("couldn't get a rec.alarm.A%d.fps value", key);

		DDEBUG_FAIL
		return FALSE;
	}

	if(nf_sysdb_get_key1("rec.alarm.A%d.quality", key, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);

		memset(buf, 0, sizeof(buf));
		g_stpcpy(buf, str);

		memcpy(data->qualParam[0], buf, CAMERA_CHANNEL * HOURS_A_DAY);
		g_value_unset(&ret_value);

	}else {
		g_warning("couldn't get a rec.alarm.A%d.quality value", key);

		DDEBUG_FAIL
		return FALSE;
	}

	if(nf_sysdb_get_key1("rec.alarm.A%d.audio", key, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);

		memset(buf, 0, sizeof(buf));
		g_stpcpy(buf, str);

		memcpy(data->audioParam[0], buf, CAMERA_CHANNEL * HOURS_A_DAY);
		g_value_unset(&ret_value);

	}else {
		g_warning("couldn't get a rec.alarm.A%d.audio value", key);

		DDEBUG_FAIL
		return FALSE;
	}


	if(nf_sysdb_get_key1("rec.alarm.A%d.alarm_ch", key, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);

		memset(buf, 0, sizeof(buf));
		g_stpcpy(buf, str);

		memcpy(data->alarmChParam[0], buf, CAMERA_CHANNEL * HOURS_A_DAY);

		g_value_unset(&ret_value);

	}else {
		g_warning("couldn't get a rec.alarm.A%d.alarm_ch value", key);

		DDEBUG_FAIL
		return FALSE;
	}


	if(nf_sysdb_get_key1("rec.alarm.A%d.mode", key, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);

		memset(buf, 0, sizeof(buf));
		g_stpcpy(buf, str);

		memcpy(data->modeParam[0], buf, CAMERA_CHANNEL * HOURS_A_DAY);

		g_value_unset(&ret_value);

	}else {
		g_warning("couldn't get a rec.alarm.A%d.mode value", key);

		DDEBUG_FAIL
		return FALSE;
	}



	return TRUE;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
gboolean DAL_set_recordAlarmParam_data(AlarmRecordParamData *data, guint key)
{
	GValue set_value = {0};
	gchar buf[769];
	gint i;

	DDEBUG_START

	g_return_val_if_fail(data != NULL, FALSE);

	nf_sysdb_lock(NF_SYSDB_CATE_REC);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->sizeParam[0], CAMERA_CHANNEL * HOURS_A_DAY);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key1("rec.alarm.A%d.size", key, &set_value, NULL)) {
		g_warning("couldn't set a rec.alarm.A%d.size value", key);
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);


	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->fpsParam[0], CAMERA_CHANNEL * HOURS_A_DAY);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key1("rec.alarm.A%d.fps", key, &set_value, NULL)) {
		g_warning("couldn't set a rec.alarm.A%d.fps value", key);
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);


	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->qualParam[0], CAMERA_CHANNEL * HOURS_A_DAY);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key1("rec.alarm.A%d.quality", key, &set_value, NULL)) {
		g_warning("couldn't set a rec.alarm.A%d.quality value", key);
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);


	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->audioParam[0], CAMERA_CHANNEL * HOURS_A_DAY);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key1("rec.alarm.A%d.audio", key, &set_value, NULL)) {
		g_warning("couldn't set a rec.alarm.A%d.audio value", key);
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);



	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->alarmChParam[0], CAMERA_CHANNEL * HOURS_A_DAY);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key1("rec.alarm.A%d.alarm_ch", key, &set_value, NULL)) {
		g_warning("couldn't set a rec.alarm.A%d.alarm_ch value", key);
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);



	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->modeParam[0], CAMERA_CHANNEL * HOURS_A_DAY);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key1("rec.alarm.A%d.mode", key, &set_value, NULL)) {
		g_warning("couldn't set a rec.alarm.A%d.mode value", key);
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_REC);

	DDEBUG_END
	return TRUE;
}

#else
gboolean DAL_get_recordAlarmParam_data(RecordParamData *data, guint key)
{
	GValue ret_value = {0};
	const gchar *str = NULL;
	gchar buf[769];

	g_return_val_if_fail(data != NULL, FALSE);


	if(nf_sysdb_get_key1("rec.alarm.A%d.size", key, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);

		memset(buf, 0, sizeof(buf));
		g_stpcpy(buf, str);

		memcpy(data->sizeParam[0], buf, CAMERA_CHANNEL * HOURS_A_DAY);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.alarm.A%d.size value", key);

		DDEBUG_FAIL
		return FALSE;
	}

	if(nf_sysdb_get_key1("rec.alarm.A%d.fps", key, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);

		memset(buf, 0, sizeof(buf));
		g_stpcpy(buf, str);

		memcpy(data->fpsParam[0], buf, CAMERA_CHANNEL * HOURS_A_DAY);
		g_value_unset(&ret_value);

	}else {
		g_warning("couldn't get a rec.alarm.A%d.fps value", key);

		DDEBUG_FAIL
		return FALSE;
	}

	if(nf_sysdb_get_key1("rec.alarm.A%d.quality", key, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);

		memset(buf, 0, sizeof(buf));
		g_stpcpy(buf, str);

		memcpy(data->qualParam[0], buf, CAMERA_CHANNEL * HOURS_A_DAY);
		g_value_unset(&ret_value);

	}else {
		g_warning("couldn't get a rec.alarm.A%d.quality value", key);

		DDEBUG_FAIL
		return FALSE;
	}

	if(nf_sysdb_get_key1("rec.alarm.A%d.audio", key, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);

		memset(buf, 0, sizeof(buf));
		g_stpcpy(buf, str);

		memcpy(data->audioParam[0], buf, CAMERA_CHANNEL * HOURS_A_DAY);
		g_value_unset(&ret_value);

	}else {
		g_warning("couldn't get a rec.alarm.A%d.audio value", key);

		DDEBUG_FAIL
		return FALSE;
	}

	if(nf_sysdb_get_key1("rec.alarm.A%d.mode", key, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);

		memset(buf, 0, sizeof(buf));
		g_stpcpy(buf, str);

		memcpy(data->modeParam[0], buf, CAMERA_CHANNEL * HOURS_A_DAY);
		g_value_unset(&ret_value);

	}else {
		g_warning("couldn't get a rec.alarm.A%d.mode value", key);

		DDEBUG_FAIL
		return FALSE;
	}

	return TRUE;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
gboolean DAL_set_recordAlarmParam_data(RecordParamData *data, guint key)
{
	GValue set_value = {0};
	gchar buf[769];
	gint i;

	DDEBUG_START

	g_return_val_if_fail(data != NULL, FALSE);

	nf_sysdb_lock(NF_SYSDB_CATE_REC);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->sizeParam[0], CAMERA_CHANNEL * HOURS_A_DAY);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key1("rec.alarm.A%d.size", key, &set_value, NULL)) {
		g_warning("couldn't set a rec.alarm.A%d.size value", key);
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);


	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->fpsParam[0], CAMERA_CHANNEL * HOURS_A_DAY);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key1("rec.alarm.A%d.fps", key, &set_value, NULL)) {
		g_warning("couldn't set a rec.alarm.A%d.fps value", key);
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);


	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->qualParam[0], CAMERA_CHANNEL * HOURS_A_DAY);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key1("rec.alarm.A%d.quality", key, &set_value, NULL)) {
		g_warning("couldn't set a rec.alarm.A%d.quality value", key);
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);


	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->audioParam[0], CAMERA_CHANNEL * HOURS_A_DAY);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key1("rec.alarm.A%d.audio", key, &set_value, NULL)) {
		g_warning("couldn't set a rec.alarm.A%d.audio value", key);
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);


	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->modeParam[0], CAMERA_CHANNEL * HOURS_A_DAY);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key1("rec.alarm.A%d.mode", key, &set_value, NULL)) {
		g_warning("couldn't set a rec.alarm.A%d.mode value", key);
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_REC);

	DDEBUG_END
	return TRUE;
}
#endif
/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
gboolean DAL_get_recordPanic_data(RecordPanicData *data)
{
	GValue ret_value = {0};
	GType gtype;
	gchar buf[33];

	g_return_val_if_fail(data != NULL, FALSE);

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.panic.size", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->sizeParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.panic.size value");

		DDEBUG_FAIL
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.panic.fps", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->fpsParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.panic.fps value");

		DDEBUG_FAIL
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.panic.quality", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->qualParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.panic.quality value");

		DDEBUG_FAIL
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.panic.audio", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->audioParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.panic.audio value");

		DDEBUG_FAIL
		return FALSE;
	}

	return TRUE;
}

gboolean DAL_set_recordPanic_data(RecordPanicData *data)
{
	GValue set_value = {0};
	gchar buf[33];

	DDEBUG_START

	g_return_val_if_fail(data != NULL, FALSE);

	nf_sysdb_lock(NF_SYSDB_CATE_REC);


	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->sizeParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.panic.size", &set_value, NULL)) {
		g_warning("couldn't set a rec.panic.size value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->fpsParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.panic.fps", &set_value, NULL)) {
		g_warning("couldn't set a rec.panic.fps value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);


	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->qualParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.panic.quality", &set_value, NULL)) {
		g_warning("couldn't set a rec.panic.quality value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);


	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->audioParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.panic.audio", &set_value, NULL)) {
		g_warning("couldn't set a rec.panic.audio value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_REC);

	DDEBUG_END
	return TRUE;
}

#if defined(_OTM_1648) || defined(_OTM_0824) || defined(_SNF_MODEL)
/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
gboolean DAL_get_setupNetStream_data(SetupNetStreamData *data)
{
	GValue ret_value = {0};
	GType gtype;
	gchar buf[33];

	g_return_val_if_fail(data != NULL, FALSE);

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.netstream.size", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->sizeParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a net.live.size value");

		DDEBUG_FAIL
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.netstream.fps", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->fpsParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a net.live.fps value");

		DDEBUG_FAIL
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.netstream.quality", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->qualParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a net.live.quality value");

		DDEBUG_FAIL
		return FALSE;
	}

	if(nf_sysdb_get_key0("rec.netstream.enable_stream_control", &ret_value, NULL))
	{
		data->enableStreamControl = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
		g_warning("couldn't get a net.enable_stream_control value");

		DDEBUG_FAIL
		return FALSE;
	}

	return TRUE;
}

gboolean DAL_get_enable_stream_control(SetupNetStreamData *data)
{
}

gboolean DAL_set_setupNetStream_data(SetupNetStreamData *data)
{
	GValue set_value = {0};
	gchar buf[33];

	DDEBUG_START

	g_return_val_if_fail(data != NULL, FALSE);

	nf_sysdb_lock(NF_SYSDB_CATE_REC);


	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->sizeParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.netstream.size", &set_value, NULL)) {
		g_warning("couldn't set a net.live.size value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->fpsParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.netstream.fps", &set_value, NULL)) {
		g_warning("couldn't set a net.live.fps value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);


	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->qualParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.netstream.quality", &set_value, NULL)) {
		g_warning("couldn't set a net.live.quality value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->enableStreamControl);

	if(!nf_sysdb_set_key0("rec.netstream.enable_stream_control", &set_value, NULL))
	{
		g_warning("couldn't set a net.enable_stream_control value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_REC);

	DDEBUG_END
	return TRUE;
}
#endif


gboolean DAL_get_recordAuto_Motion_data(RecordAutoData *data)
{
	GValue ret_value = {0};
	GType gtype;
	gchar buf[33];

	g_return_val_if_fail(data != NULL, FALSE);

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.auto.motion.size", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->sizeParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.auto.motion.size value");

		DDEBUG_FAIL
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.auto.motion.fps", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->fpsParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.auto.motion.fps value");

		DDEBUG_FAIL
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.auto.motion.quality", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->qualParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.auto.motion.quality value");

		DDEBUG_FAIL
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.auto.motion.audio", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->audioParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.auto.motion.audio value");

		DDEBUG_FAIL
		return FALSE;
	}

	return TRUE;
}

gboolean DAL_set_recordAuto_Motion_data(RecordAutoData *data)
{
	GValue set_value = {0};
	gchar buf[33];

	DDEBUG_START

	g_return_val_if_fail(data != NULL, FALSE);

	nf_sysdb_lock(NF_SYSDB_CATE_REC);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->sizeParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.auto.motion.size", &set_value, NULL)) {
		g_warning("couldn't set a auto.motion.size value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->fpsParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.auto.motion.fps", &set_value, NULL)) {
		g_warning("couldn't set a auto.motion.fps value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->qualParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.auto.motion.quality", &set_value, NULL)) {
		g_warning("couldn't set a auto.motion.quality value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->audioParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.auto.motion.audio", &set_value, NULL)) {
		g_warning("couldn't set a auto.motion.audio value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_REC);

	DDEBUG_END
	return TRUE;
}

gboolean DAL_get_recordAuto_Alarm_data(RecordAutoData *data)
{
	GValue ret_value = {0};
	GType gtype;
	gchar buf[33];

	g_return_val_if_fail(data != NULL, FALSE);

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.auto.alarm.size", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->sizeParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.auto.alarm.size value");

		DDEBUG_FAIL
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.auto.alarm.fps", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->fpsParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.auto.alarm.fps value");

		DDEBUG_FAIL
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.auto.alarm.quality", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->qualParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.auto.alarm.quality value");

		DDEBUG_FAIL
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.auto.alarm.audio", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->audioParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.auto.alarm.audio value");

		DDEBUG_FAIL
		return FALSE;
	}

	return TRUE;
}

gboolean DAL_set_recordAuto_Alarm_data(RecordAutoData *data)
{
	GValue set_value = {0};
	gchar buf[33];

	DDEBUG_START

	g_return_val_if_fail(data != NULL, FALSE);

	nf_sysdb_lock(NF_SYSDB_CATE_REC);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->sizeParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.auto.alarm.size", &set_value, NULL)) {
		g_warning("couldn't set a auto.alarm.size value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->fpsParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.auto.alarm.fps", &set_value, NULL)) {
		g_warning("couldn't set a auto.alarm.fps value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->qualParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.auto.alarm.quality", &set_value, NULL)) {
		g_warning("couldn't set a auto.alarm.quality value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->audioParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.auto.alarm.audio", &set_value, NULL)) {
		g_warning("couldn't set a auto.alarm.audio value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_REC);

	DDEBUG_END
	return TRUE;
}



gboolean DAL_get_recordAuto_MotionAlarm_data(RecordAutoData *data)
{
	GValue ret_value = {0};
	GType gtype;
	gchar buf[33];

	g_return_val_if_fail(data != NULL, FALSE);

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.auto.motion_alarm.size", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->sizeParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.auto.motion_alarm.size value");

		DDEBUG_FAIL
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.auto.motion_alarm.fps", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->fpsParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.auto.motion_alarm.fps value");

		DDEBUG_FAIL
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.auto.motion_alarm.quality", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->qualParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.auto.motion_alarm.quality value");

		DDEBUG_FAIL
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.auto.motion_alarm.audio", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->audioParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.auto.motion_alarm.audio value");

		DDEBUG_FAIL
		return FALSE;
	}

	return TRUE;
}

gboolean DAL_set_recordAuto_MotionAlarm_data(RecordAutoData *data)
{
	GValue set_value = {0};
	gchar buf[33];

	DDEBUG_START

	g_return_val_if_fail(data != NULL, FALSE);

	nf_sysdb_lock(NF_SYSDB_CATE_REC);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->sizeParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.auto.motion_alarm.size", &set_value, NULL)) {
		g_warning("couldn't set a auto.motion_alarm.size value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->fpsParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.auto.motion_alarm.fps", &set_value, NULL)) {
		g_warning("couldn't set a auto.motion_alarm.fps value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->qualParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.auto.motion_alarm.quality", &set_value, NULL)) {
		g_warning("couldn't set a auto.motion_alarm.quality value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->audioParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.auto.motion_alarm.audio", &set_value, NULL)) {
		g_warning("couldn't set a auto.motion_alarm.audio value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_REC);

	DDEBUG_END
	return TRUE;
}



gboolean DAL_get_recordAuto_intensive_Motion_data(RecordAutoIntensiveData *data)
{
	GValue ret_value = {0};
	GType gtype;
	gchar buf[33];

	g_return_val_if_fail(data != NULL, FALSE);

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.auto.m_intensive.normal.size", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->normal_sizeParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.auto.m_intensive.normal.size value");

		DDEBUG_FAIL
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.auto.m_intensive.normal.fps", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->normal_fpsParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.auto.m_intensive.normal.fps value");

		DDEBUG_FAIL
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.auto.m_intensive.normal.quality", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->normal_qualParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.auto.m_intensive.normal.quality value");

		DDEBUG_FAIL
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.auto.m_intensive.motion.size", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->event_sizeParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.auto.m_intensive.motion.size value");

		DDEBUG_FAIL
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.auto.m_intensive.motion.fps", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->event_fpsParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.auto.m_intensive.motion.fps value");

		DDEBUG_FAIL
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.auto.m_intensive.motion.quality", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->event_qualParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.auto.m_intensive.motion.quality value");

		DDEBUG_FAIL
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.auto.m_intensive.audio", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->audioParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.auto.m_intensive.audio value");

		DDEBUG_FAIL
		return FALSE;
	}

	return TRUE;
}

gboolean DAL_set_recordAuto_intensive_Motion_data(RecordAutoIntensiveData *data)
{
	GValue set_value = {0};
	gchar buf[33];

	DDEBUG_START

	g_return_val_if_fail(data != NULL, FALSE);

	nf_sysdb_lock(NF_SYSDB_CATE_REC);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->normal_sizeParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.auto.m_intensive.normal.size", &set_value, NULL)) {
		g_warning("couldn't set a auto.m_intensive.normal.size value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->normal_fpsParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.auto.m_intensive.normal.fps", &set_value, NULL)) {
		g_warning("couldn't set a auto.m_intensive.normal.fps value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->normal_qualParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.auto.m_intensive.normal.quality", &set_value, NULL)) {
		g_warning("couldn't set a auto.m_intensive.normal.quality value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->event_sizeParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.auto.m_intensive.motion.size", &set_value, NULL)) {
		g_warning("couldn't set a auto.m_intensive.motion.size value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->event_fpsParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.auto.m_intensive.motion.fps", &set_value, NULL)) {
		g_warning("couldn't set a auto.m_intensive.motion.fps value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->event_qualParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.auto.m_intensive.motion.quality", &set_value, NULL)) {
		g_warning("couldn't set a auto.m_intensive.motion.quality value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->audioParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.auto.m_intensive.audio", &set_value, NULL)) {
		g_warning("couldn't set a auto.m_intensive.audio value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_REC);

	DDEBUG_END
	return TRUE;
}

gboolean DAL_get_recordAuto_intensive_Alarm_data(RecordAutoIntensiveData *data)
{
	GValue ret_value = {0};
	GType gtype;
	gchar buf[33];

	g_return_val_if_fail(data != NULL, FALSE);

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.auto.a_intensive.normal.size", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->normal_sizeParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.auto.a_intensive.normal.size value");

		DDEBUG_FAIL
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.auto.a_intensive.normal.fps", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->normal_fpsParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.auto.a_intensive.normal.fps value");

		DDEBUG_FAIL
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.auto.a_intensive.normal.quality", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->normal_qualParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.auto.a_intensive.normal.quality value");

		DDEBUG_FAIL
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.auto.a_intensive.alarm.size", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->event_sizeParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.auto.a_intensive.alarm.size value");

		DDEBUG_FAIL
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.auto.a_intensive.alarm.fps", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->event_fpsParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.auto.a_intensive.alarm.fps value");

		DDEBUG_FAIL
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.auto.a_intensive.alarm.quality", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->event_qualParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.auto.a_intensive.alarm.quality value");

		DDEBUG_FAIL
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.auto.a_intensive.audio", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->audioParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.auto.a_intensive.audio value");

		DDEBUG_FAIL
		return FALSE;
	}

	return TRUE;
}

gboolean DAL_set_recordAuto_intensive_Alarm_data(RecordAutoIntensiveData *data)
{
	GValue set_value = {0};
	gchar buf[33];

	DDEBUG_START

	g_return_val_if_fail(data != NULL, FALSE);

	nf_sysdb_lock(NF_SYSDB_CATE_REC);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->normal_sizeParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.auto.a_intensive.normal.size", &set_value, NULL)) {
		g_warning("couldn't set a auto.a_intensive.normal.size value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->normal_fpsParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.auto.a_intensive.normal.fps", &set_value, NULL)) {
		g_warning("couldn't set a auto.a_intensive.normal.fps value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->normal_qualParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.auto.a_intensive.normal.quality", &set_value, NULL)) {
		g_warning("couldn't set a auto.a_intensive.normal.quality value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->event_sizeParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.auto.a_intensive.alarm.size", &set_value, NULL)) {
		g_warning("couldn't set a auto.a_intensive.alarm.size value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->event_fpsParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.auto.a_intensive.alarm.fps", &set_value, NULL)) {
		g_warning("couldn't set a auto.a_intensive.alarm.fps value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->event_qualParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.auto.a_intensive.alarm.quality", &set_value, NULL)) {
		g_warning("couldn't set a auto.a_intensive.alarm.quality value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->audioParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.auto.a_intensive.audio", &set_value, NULL)) {
		g_warning("couldn't set a auto.a_intensive.audio value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_REC);

	DDEBUG_END
	return TRUE;
}



gboolean DAL_get_recordAuto_intensive_MotionAlarm_data(RecordAutoIntensiveData *data)
{
	GValue ret_value = {0};
	GType gtype;
	gchar buf[33];

	g_return_val_if_fail(data != NULL, FALSE);

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.auto.ma_intensive.normal.size", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->normal_sizeParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.auto.ma_intensive.normal.size value");

		DDEBUG_FAIL
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.auto.ma_intensive.normal.fps", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->normal_fpsParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.auto.ma_intensive.normal.fps value");

		DDEBUG_FAIL
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.auto.ma_intensive.normal.quality", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->normal_qualParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.auto.ma_intensive.normal.quality value");

		DDEBUG_FAIL
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.auto.ma_intensive.motion_alarm.size", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->event_sizeParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.auto.ma_intensive.motion_alarm.size value");

		DDEBUG_FAIL
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.auto.ma_intensive.motion_alarm.fps", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->event_fpsParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.auto.ma_intensive.motion_alarm.fps value");

		DDEBUG_FAIL
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.auto.ma_intensive.motion_alarm.quality", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->event_qualParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.auto.ma_intensive.motion_alarm.quality value");

		DDEBUG_FAIL
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("rec.auto.ma_intensive.audio", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->audioParam, buf, CAMERA_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.auto.ma_intensive.audio value");

		DDEBUG_FAIL
		return FALSE;
	}

	return TRUE;
}

gboolean DAL_set_recordAuto_intensive_MotionAlarm_data(RecordAutoIntensiveData *data)
{
	GValue set_value = {0};
	gchar buf[33];

	DDEBUG_START

	g_return_val_if_fail(data != NULL, FALSE);

	nf_sysdb_lock(NF_SYSDB_CATE_REC);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->normal_sizeParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.auto.ma_intensive.normal.size", &set_value, NULL)) {
		g_warning("couldn't set a auto.ma_intensive.normal.size value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->normal_fpsParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.auto.ma_intensive.normal.fps", &set_value, NULL)) {
		g_warning("couldn't set a auto.ma_intensive.normal.fps value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->normal_qualParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.auto.ma_intensive.normal.quality", &set_value, NULL)) {
		g_warning("couldn't set a auto.ma_intensive.normal.quality value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->event_sizeParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.auto.ma_intensive.motion_alarm.size", &set_value, NULL)) {
		g_warning("couldn't set a auto.ma_intensive.motion_alarm.size value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->event_fpsParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.auto.ma_intensive.motion_alarm.fps", &set_value, NULL)) {
		g_warning("couldn't set a auto.ma_intensive.motion_alarm.fps value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->event_qualParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.auto.ma_intensive.motion_alarm.quality", &set_value, NULL)) {
		g_warning("couldn't set a auto.ma_intensive.motion_alarm.quality value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->audioParam, CAMERA_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("rec.auto.ma_intensive.audio", &set_value, NULL)) {
		g_warning("couldn't set a auto.ma_intensive.audio value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_REC);

	DDEBUG_END
	return TRUE;
}



/********************************************************************************
 * Setup --> Record Setup --> Recording Operations (IPCAM)
 * ******************************************************************************/

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
gboolean DAL_get_IPCamRecOper_data(IPCamRecOperData *data)
{
	GValue ret_value = {0};
	GType gtype;

	if(nf_sysdb_get_key0("iprec.sched_mode", &ret_value, NULL))
	{
g_message("%s 1HMKONG_MESSAGE	", __func__);
		data->schedMode = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a iprec.sched_mode value");

		DDEBUG_FAIL
		return FALSE;
	}

	if(nf_sysdb_get_key0("iprec.pre_rec_time", &ret_value, NULL))
	{
g_message("%s 4 HMKONG_MESSAGE	", __func__);
		data->preRecTime = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a iprec.pre_rec_time value");

		DDEBUG_FAIL
		return FALSE;
	}

	if(nf_sysdb_get_key0("iprec.post_rec_time", &ret_value, NULL))
	{
g_message("%s 3 HMKONG_MESSAGE	", __func__);
		data->postRecTime = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a iprec.post_rec_time value");

		DDEBUG_FAIL
		return FALSE;
	}

	return TRUE;
}


/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
#ifdef ENABLE_HNF_IPCAM

gboolean DAL_set_IPCamRecOper_data(IPCamRecOperData *data)
{
	GValue set_value = {0};

	DDEBUG_START

	g_return_val_if_fail(data != NULL, FALSE);

	nf_sysdb_lock(NF_SYSDB_CATE_IPCAM);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->schedMode);
	if(!nf_sysdb_set_key0("iprec.sched_mode", &set_value, NULL)) {
		g_warning("couldn't set a iprec.sched_mode value");

		nf_sysdb_unlock(NF_SYSDB_CATE_IPCAM);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->preRecTime);
	if(!nf_sysdb_set_key0("iprec.pre_rec_time", &set_value, NULL)) {
		g_warning("couldn't set a iprec.pre_rec_time value");

		nf_sysdb_unlock(NF_SYSDB_CATE_IPCAM);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->postRecTime);
	if(!nf_sysdb_set_key0("iprec.post_rec_time", &set_value, NULL)) {
		g_warning("couldn't set a iprec.post_rec_time value");

		nf_sysdb_unlock(NF_SYSDB_CATE_IPCAM);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_IPCAM, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_IPCAM);

	DDEBUG_END
	return TRUE;
}
#endif



/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
gboolean DAL_get_IPCamRecMotParam_data(IPCamRecParamData *data, guint key)
{
	GValue ret_value = {0};
	const gchar *str = NULL;
	gchar buf[769];
	gchar sbuf[128];

	g_return_val_if_fail(data != NULL, FALSE);


	memset(sbuf, 0, sizeof(sbuf));
	g_sprintf(sbuf, "iprec.motion.M%d.size", key);
	if(nf_sysdb_get_key1(sbuf, key, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);

		memset(buf, 0, sizeof(buf));
		g_stpcpy(buf, str);

		memcpy(data->sizeParam[0], buf, IPCAM_CHANNEL * HOURS_A_DAY);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a iprec.motion.M%d.size value", key);

		DDEBUG_FAIL
		return FALSE;
	}

	memset(sbuf, 0, sizeof(sbuf));
	g_sprintf(sbuf, "iprec.motion.M%d.fps", key);
	if(nf_sysdb_get_key1(sbuf, key, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);

		memset(buf, 0, sizeof(buf));
		g_stpcpy(buf, str);

		memcpy(data->fpsParam[0], buf, IPCAM_CHANNEL * HOURS_A_DAY);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a iprec.motion.M%d.fps value", key);

		DDEBUG_FAIL
		return FALSE;
	}

	memset(sbuf, 0, sizeof(sbuf));
	g_sprintf(sbuf, "iprec.motion.M%d.quality", key);
	if(nf_sysdb_get_key1(sbuf, key, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);

		memset(buf, 0, sizeof(buf));
		g_stpcpy(buf, str);

		memcpy(data->qualParam[0], buf, IPCAM_CHANNEL * HOURS_A_DAY);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a iprec.motion.M%d.quality value", key);

		DDEBUG_FAIL
		return FALSE;
	}

	memset(sbuf, 0, sizeof(sbuf));
	g_sprintf(sbuf, "iprec.motion.M%d.audio", key);
	if(nf_sysdb_get_key1(sbuf, key, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);

		memset(buf, 0, sizeof(buf));
		g_stpcpy(buf, str);

		memcpy(data->audioParam[0], buf, IPCAM_CHANNEL * HOURS_A_DAY);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a iprec.motion.M%d.audio value", key);

		DDEBUG_FAIL
		return FALSE;
	}

	memset(sbuf, 0, sizeof(sbuf));
	g_sprintf(sbuf, "iprec.motion.M%d.mode", key);
	if(nf_sysdb_get_key1(sbuf, key, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);

		memset(buf, 0, sizeof(buf));
		g_stpcpy(buf, str);

		memcpy(data->modeParam[0], buf, IPCAM_CHANNEL * HOURS_A_DAY);
		g_value_unset(&ret_value);

	}else {
		g_warning("couldn't get a iprec.motion.M%d.mode value", key);

		DDEBUG_FAIL
		return FALSE;
	}

	return TRUE;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
#ifdef ENABLE_HNF_IPCAM
gboolean DAL_set_IPCamRecMotParam_data(IPCamRecParamData *data, guint key)
{
	GValue set_value = {0};
	gchar sbuf[128];
	gchar buf[769];
	gint i;

	DDEBUG_START

	g_return_val_if_fail(data != NULL, FALSE);

	nf_sysdb_lock(NF_SYSDB_CATE_IPCAM);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->sizeParam[0], IPCAM_CHANNEL * HOURS_A_DAY);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	memset(sbuf, 0, sizeof(sbuf));
	g_sprintf(sbuf, "iprec.motion.M%d.size", key);
	if(!nf_sysdb_set_key1(sbuf, key, &set_value, NULL)) {
		g_warning("couldn't set a iprec.motion.M%d.size value", key);
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_IPCAM);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);


	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->fpsParam[0], IPCAM_CHANNEL * HOURS_A_DAY);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	memset(sbuf, 0, sizeof(sbuf));
	g_sprintf(sbuf, "iprec.motion.M%d.fps", key);
	if(!nf_sysdb_set_key1(sbuf, key, &set_value, NULL)) {
		g_warning("couldn't set a iprec.motion.M%d.fps value", key);
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_IPCAM);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);


	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->qualParam[0], IPCAM_CHANNEL * HOURS_A_DAY);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	memset(sbuf, 0, sizeof(sbuf));
	g_sprintf(sbuf, "iprec.motion.M%d.quality", key);
	if(!nf_sysdb_set_key1(sbuf, key, &set_value, NULL)) {
		g_warning("couldn't set a iprec.motion.M%d.quality value", key);
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_IPCAM);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);



	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->audioParam[0], IPCAM_CHANNEL * HOURS_A_DAY);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	memset(sbuf, 0, sizeof(sbuf));
	g_sprintf(sbuf, "iprec.motion.M%d.audio", key);
	if(!nf_sysdb_set_key1(sbuf, key, &set_value, NULL)) {
		g_warning("couldn't set a iprec.motion.M%d.audio value", key);
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_IPCAM);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);


	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->modeParam[0], IPCAM_CHANNEL * HOURS_A_DAY);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	memset(sbuf, 0, sizeof(sbuf));
	g_sprintf(sbuf, "iprec.motion.M%d.mode", key);
	if(!nf_sysdb_set_key1(sbuf, key, &set_value, NULL)) {
		g_warning("couldn't set a iprec.motion.M%d.mode value", key);
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_IPCAM);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_IPCAM, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_IPCAM);

	DDEBUG_END
	return TRUE;
}
#endif
/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
gboolean DAL_get_IPCamRecAlarmParam_data(IPCamRecParamData *data, guint key)
{
	GValue ret_value = {0};
	const gchar *str = NULL;
	gchar buf[769];
	gchar sbuf[128];

	g_return_val_if_fail(data != NULL, FALSE);


	memset(sbuf, 0, sizeof(sbuf));
	g_sprintf(sbuf, "iprec.alarm.A%d.size", key);
	if(nf_sysdb_get_key1(sbuf, key, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);

		memset(buf, 0, sizeof(buf));
		g_stpcpy(buf, str);

		memcpy(data->sizeParam[0], buf, IPCAM_CHANNEL * HOURS_A_DAY);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a iprec.alarm.A%d.size value", key);

		DDEBUG_FAIL
		return FALSE;
	}

	memset(sbuf, 0, sizeof(sbuf));
	g_sprintf(sbuf, "iprec.alarm.A%d.fps", key);
	if(nf_sysdb_get_key1(sbuf, key, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);

		memset(buf, 0, sizeof(buf));
		g_stpcpy(buf, str);

		memcpy(data->fpsParam[0], buf, IPCAM_CHANNEL * HOURS_A_DAY);
		g_value_unset(&ret_value);

	}else {
		g_warning("couldn't get a iprec.alarm.A%d.fps value", key);

		DDEBUG_FAIL
		return FALSE;
	}

	memset(sbuf, 0, sizeof(sbuf));
	g_sprintf(sbuf, "iprec.alarm.A%d.quality", key);
	if(nf_sysdb_get_key1(sbuf, key, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);

		memset(buf, 0, sizeof(buf));
		g_stpcpy(buf, str);

		memcpy(data->qualParam[0], buf, IPCAM_CHANNEL * HOURS_A_DAY);
		g_value_unset(&ret_value);

	}else {
		g_warning("couldn't get a iprec.alarm.A%d.quality value", key);

		DDEBUG_FAIL
		return FALSE;
	}

	memset(sbuf, 0, sizeof(sbuf));
	g_sprintf(sbuf, "iprec.alarm.A%d.audio", key);
	if(nf_sysdb_get_key1(sbuf, key, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);

		memset(buf, 0, sizeof(buf));
		g_stpcpy(buf, str);

		memcpy(data->audioParam[0], buf, IPCAM_CHANNEL * HOURS_A_DAY);
		g_value_unset(&ret_value);

	}else {
		g_warning("couldn't get a iprec.alarm.A%d.audio value", key);

		DDEBUG_FAIL
		return FALSE;
	}

	memset(sbuf, 0, sizeof(sbuf));
	g_sprintf(sbuf, "iprec.alarm.A%d.mode", key);
	if(nf_sysdb_get_key1(sbuf, key, &ret_value, NULL))
	{
		str = g_value_get_string(&ret_value);

		memset(buf, 0, sizeof(buf));
		g_stpcpy(buf, str);

		memcpy(data->modeParam[0], buf, IPCAM_CHANNEL * HOURS_A_DAY);
		g_value_unset(&ret_value);

	}else {
		g_warning("couldn't get a iprec.alarm.A%d.mode value", key);

		DDEBUG_FAIL
		return FALSE;
	}

	return TRUE;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
#ifdef ENABLE_HNF_IPCAM
gboolean DAL_set_IPCamRecAlarmParam_data(IPCamRecParamData *data, guint key)
{
	GValue set_value = {0};
	gchar sbuf[128];
	gchar buf[769];
	gint i;

	DDEBUG_START

	g_return_val_if_fail(data != NULL, FALSE);

	nf_sysdb_lock(NF_SYSDB_CATE_IPCAM);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->sizeParam[0], IPCAM_CHANNEL * HOURS_A_DAY);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	memset(sbuf, 0, sizeof(sbuf));
	g_sprintf(sbuf, "iprec.alarm.A%d.size", key);
	if(!nf_sysdb_set_key1(sbuf, key, &set_value, NULL)) {
		g_warning("couldn't set a iprec.alarm.A%d.size value", key);
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_IPCAM);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);


	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->fpsParam[0], IPCAM_CHANNEL * HOURS_A_DAY);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	memset(sbuf, 0, sizeof(sbuf));
	g_sprintf(sbuf, "iprec.alarm.A%d.fps", key);
	if(!nf_sysdb_set_key1(sbuf, key, &set_value, NULL)) {
		g_warning("couldn't set a iprec.alarm.A%d.fps value", key);
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_IPCAM);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);


	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->qualParam[0], IPCAM_CHANNEL * HOURS_A_DAY);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	memset(sbuf, 0, sizeof(sbuf));
	g_sprintf(sbuf, "iprec.alarm.A%d.quality", key);
	if(!nf_sysdb_set_key1(sbuf, key, &set_value, NULL)) {
		g_warning("couldn't set a iprec.alarm.A%d.quality value", key);
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_IPCAM);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);


	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->audioParam[0], IPCAM_CHANNEL * HOURS_A_DAY);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	memset(sbuf, 0, sizeof(sbuf));
	g_sprintf(sbuf, "iprec.alarm.A%d.audio", key);
	if(!nf_sysdb_set_key1(sbuf, key, &set_value, NULL)) {
		g_warning("couldn't set a iprec.alarm.A%d.audio value", key);
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_IPCAM);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);


	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->modeParam[0], IPCAM_CHANNEL * HOURS_A_DAY);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	memset(sbuf, 0, sizeof(sbuf));
	g_sprintf(sbuf, "iprec.alarm.A%d.mode", key);
	if(!nf_sysdb_set_key1(sbuf, key, &set_value, NULL)) {
		g_warning("couldn't set a iprec.alarm.A%d.mode value", key);
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_IPCAM);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_IPCAM, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_IPCAM);

	DDEBUG_END
	return TRUE;
}
#endif

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
gboolean DAL_get_IPCamRecPanic_data(IPCamRecPanicData *data)
{
	GValue ret_value = {0};
	GType gtype;
	gchar buf[33];

	g_return_val_if_fail(data != NULL, FALSE);

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("iprec.panic.size", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->sizeParam, buf, IPCAM_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a iprec.panic.size value");

		DDEBUG_FAIL
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("iprec.panic.fps", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->fpsParam, buf, IPCAM_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a iprec.panic.fps value");

		DDEBUG_FAIL
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("iprec.panic.quality", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->qualParam, buf, IPCAM_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a iprec.panic.quality value");

		DDEBUG_FAIL
		return FALSE;
	}

	memset(buf, 0, sizeof(buf));
	if(nf_sysdb_get_key0("iprec.panic.audio", &ret_value, NULL))
	{
		g_stpcpy(buf, g_value_get_string(&ret_value));

		memcpy(data->audioParam, buf, IPCAM_CHANNEL);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a iprec.panic.audio value");

		DDEBUG_FAIL
		return FALSE;
	}

	return TRUE;
}

#ifdef ENABLE_HNF_IPCAM
gboolean DAL_set_IPCamRecPanic_data(IPCamRecPanicData *data)
{
	GValue set_value = {0};
	gchar buf[33];

	DDEBUG_START

	g_return_val_if_fail(data != NULL, FALSE);

	nf_sysdb_lock(NF_SYSDB_CATE_IPCAM);


	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->sizeParam, IPCAM_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("iprec.panic.size", &set_value, NULL)) {
		g_warning("couldn't set a iprec.panic.size value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_IPCAM);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->fpsParam, IPCAM_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("iprec.panic.fps", &set_value, NULL)) {
		g_warning("couldn't set a iprec.panic.fps value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_IPCAM);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);


	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->qualParam, IPCAM_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("iprec.panic.quality", &set_value, NULL)) {
		g_warning("couldn't set a iprec.panic.quality value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_IPCAM);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);


	memset(buf, 0, sizeof(buf));
	memcpy(buf, data->audioParam, IPCAM_CHANNEL);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, buf);

	if(!nf_sysdb_set_key0("iprec.panic.audio", &set_value, NULL)) {
		g_warning("couldn't set a iprec.panic.audio value");
		g_value_unset(&set_value);

		nf_sysdb_unlock(NF_SYSDB_CATE_IPCAM);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_IPCAM, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_IPCAM);

	DDEBUG_END
	return TRUE;
}
#endif


#if defined(__DIGI_SUPPORT__)
guint DAL_get_schedule_arch_data(ScheduleArchData *data)
{
	guint ret = 0, i =0;
	GValue ret_value = {0,};
	gchar *temp;

	if(nf_sysdb_get_key0("disk.sched_arch.act", &ret_value, NULL))
	{
		if(g_value_get_boolean(&ret_value))			data->operation = TRUE;
              else 	data->operation = FALSE;
		g_value_unset(&ret_value);
	}
	else	 	ret = 1;

	if(nf_sysdb_get_key0("disk.sched_arch.device", &ret_value, NULL))
	{
		g_stpcpy(data->device, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else	 	ret = 2;

	if(nf_sysdb_get_key0("disk.sched_arch.is_daily", &ret_value, NULL))
	{
		if(g_value_get_boolean(&ret_value))	data->mode = TRUE;
		else	data->mode = FALSE;
		g_value_unset(&ret_value);
	}
	else	 	ret = 3;

	if(nf_sysdb_get_key0("disk.sched_arch.onstart_time", &ret_value, NULL))
	{
		data->starttime = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 4;

	if(nf_sysdb_get_key0("disk.sched_arch.from_time", &ret_value, NULL))
	{
		data->from = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 5;

	if(nf_sysdb_get_key0("disk.sched_arch.end_time", &ret_value, NULL))
	{
		data->to = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else	 	ret = 6;

	if(nf_sysdb_get_key0("disk.sched_arch.ch", &ret_value, NULL))
	{
		temp = g_value_get_string(&ret_value);

		for(i=0; i<32; i++)
		{
			if(temp[i] == '0')	data->camera[i] = 0;
			else				data->camera[i] = 1;
		}
		g_value_unset(&ret_value);
	}
	else		ret = 7;


	if(nf_sysdb_get_key0("disk.sched_arch.audio", &ret_value, NULL))
	{
		temp = g_value_get_string(&ret_value);

		for(i=0; i<32; i++)
		{
			if(temp[i] == '0')	data->audio[i] = 0;
			else				data->audio[i] = 1;
		}
		g_value_unset(&ret_value);
	}
	else		ret = 8;


	return ret;
}


guint DAL_set_schedule_arch_data(ScheduleArchData *data)
{
	GValue set_value = {0,};
	gint i;
	gchar buf[20];
	gchar temp[MAX_TEMP_STR_SIZE];


	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_DISK);



	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->operation);

	if(!nf_sysdb_set_key0("disk.sched_arch.act", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISK);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);




	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->device);
	if(!nf_sysdb_set_key0("disk.sched_arch.device", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISK);
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);




	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->mode);

	if(!nf_sysdb_set_key0("disk.sched_arch.is_daily", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISK);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);



	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->starttime);
	if(!nf_sysdb_set_key0("disk.sched_arch.onstart_time", &set_value, NULL)) {
		nf_sysdb_unlock(NF_SYSDB_CATE_DISK);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);




	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->from);
	if(!nf_sysdb_set_key0("disk.sched_arch.from_time", &set_value, NULL)) {
		nf_sysdb_unlock(NF_SYSDB_CATE_DISK);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);



	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->to);
	if(!nf_sysdb_set_key0("disk.sched_arch.end_time", &set_value, NULL)) {
		nf_sysdb_unlock(NF_SYSDB_CATE_DISK);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);



	memset(temp, 0, sizeof(gchar)*MAX_TEMP_STR_SIZE);
	for(i=0; i<g_ch; i++)
	{
		if(data->camera[i] == 0)	temp[i] = '0';
		else					temp[i] = '1';
	}
	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, temp);
	if(!nf_sysdb_set_key0("disk.sched_arch.ch", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISK);
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 7;
	}
	g_value_unset(&set_value);



	memset(temp, 0, sizeof(gchar)*MAX_TEMP_STR_SIZE);
	for(i=0; i<g_ch; i++)
	{
		if(data->audio[i] == 0)	temp[i] = '0';
		else					temp[i] = '1';
	}
	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, temp);
	if(!nf_sysdb_set_key0("disk.sched_arch.audio", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_DISK);
		g_value_unset(&set_value);
		DDEBUG_FAIL
		return 8;
	}
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_DISK);


	DDEBUG_END
	return 0;
}

#endif /*__DIGI_SUPPORT__*/

//#if defined(LOREX)
guint DAL_get_archftp_data(ArchFtpData *data)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("net.ftp.host", &ret_value, NULL))
	{
		g_stpcpy(data->host, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	if(nf_sysdb_get_key0("net.ftp.port", &ret_value, NULL))
	{
		data->port = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 2;

	if(nf_sysdb_get_key0("net.ftp.username", &ret_value, NULL))
	{
		g_stpcpy(data->username, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 3;

	if(nf_sysdb_get_key0("net.ftp.passwd", &ret_value, NULL))
	{
		g_stpcpy(data->passwd, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 4;


	if(nf_sysdb_get_key0("net.ftp.is_anon", &ret_value, NULL))
	{
		data->is_anon = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 5;

	if(nf_sysdb_get_key0("net.ftp.dir_path", &ret_value, NULL))
	{
		g_stpcpy(data->dir_path, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else		ret = 6;


	if(nf_sysdb_get_key0("net.ftp.period", &ret_value, NULL))
	{
		data->period = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 7;

	return ret;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_set_archftp_data(ArchFtpData *data)
{
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_NET);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->host);
	if(!nf_sysdb_set_key0("net.ftp.host", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->port);
	if(!nf_sysdb_set_key0("net.ftp.port", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->username);
	if(!nf_sysdb_set_key0("net.ftp.username", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 3;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->passwd);
	if(!nf_sysdb_set_key0("net.ftp.passwd", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 4;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->is_anon);
	if(!nf_sysdb_set_key0("net.ftp.is_anon", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_STRING);
	g_value_set_string(&set_value, data->dir_path);
	if(!nf_sysdb_set_key0("net.ftp.dir_path", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->period);
	if(!nf_sysdb_set_key0("net.ftp.period", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_NET);
		DDEBUG_FAIL
		return 7;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_NET, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_NET);

	DDEBUG_END
	return 0;
}
//#endif

/*************************************************
 * ETC FUNCTION PROTOTYPE
 * **********************************************/

gboolean DAL_save_db(const gchar *cat)
{
	if(cat == NULL)
	{
		g_warning("[GUI DEBUG] %s:%d Category is NULL..", __FILE__, __LINE__);
		return FALSE;
	}

	if(!nf_sysdb_save(cat))
	{
		return FALSE;
	}

	return TRUE;
}

void DAL_save_setup_db(guint wtype)
{
	switch(wtype)
	{
		case 0:	//CAMERA
			nf_sysdb_save("cam");
			nf_sysdb_save("alarm");
			break;

		case 1:	//DISPLAY:
			nf_sysdb_save("disp");
			break;

		case 2:	//SOUND:
			nf_sysdb_save("audio");
			break;

		case 3:	//USER:
			nf_sysdb_save("usr");
			break;

		case 4:	//NETWORK:
			nf_sysdb_save("net");
			break;

		case 5:	//SYSTEM:
			nf_sysdb_save("sys");
			break;

		case 6:	//DISK:
			nf_sysdb_save("disk");
			break;

		case 7:	//EVENT:
			nf_sysdb_save("alarm");
			nf_sysdb_save("act");
			break;

		case 8:	//RECORDING:
			nf_sysdb_save("rec");
			break;

		case 9:	//ARCHIVING:
			break;

		case 10:		//SEARCH:
			break;

		default:
			nf_sysdb_save_all();
			break;
	}
}

void DAL_save_db_all()
{
	nf_sysdb_save_all();
}

gboolean DAL_factory_default()
{
	return nf_sysdb_default("");
}

gboolean DAL_factory_default_without_network()
{
	return nf_sysdb_default_without_network("");
}

gboolean DAL_system_data_save(const gchar *filename)
{
	return nf_sysdb_export(filename);
}

gboolean DAL_system_data_load(const gchar *filename)
{
	return nf_sysdb_import(filename);
}

gboolean DAL_system_data_load_without_network(const gchar *filename)
{
	return nf_sysdb_import_webra(filename);
}


gboolean DAL_get_passwd_enable()
{
	gboolean is_enable = FALSE;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("sys.info.passwd_enable", &ret_value, NULL))
	{
		is_enable = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}

	return is_enable;
}

gboolean DAL_get_passwd_change_enable()
{
	gboolean is_enable = FALSE;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("sys.info.passwd_changed_by_force", &ret_value, NULL))
	{
		is_enable = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}

	return is_enable;
}

#if defined(__DIGI_SUPPORT__)
guint DAL_get_autoSync_interval_min()
{
	guint ret = 0;
	GValue ret_value = {0,};
       guint interval_min = 0;

	if(nf_sysdb_get_key0("sys.date.auto_interval_min", &ret_value, NULL))
	{
		interval_min = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}
	else		ret = 1;

	return interval_min;
}
#endif

guint DAL_get_display_resolution()
{
	guint resolution = FALSE;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("disp.monitor.resolution", &ret_value, NULL))
	{
		resolution = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}

	return resolution;
}

guint DAL_get_Netwizard_func(int *data)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("sys.info.netwizard_func", &ret_value, NULL))
	{
		*data = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else ret = 1;

	return ret;
}

guint DAL_get_Langwizard_func(int *data)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("sys.info.langwizard_func", &ret_value, NULL))
	{
		*data = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else ret = 1;

	return ret;
}

guint DAL_set_Langwizard_func(int data)
{
	GValue set_value = {0,};

    DDEBUG_START
	nf_sysdb_lock(NF_SYSDB_CATE_SYS);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data);

	if(!nf_sysdb_set_key0("sys.info.langwizard_func", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return -1;
	}
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
 	DDEBUG_END
	return 0;
}

guint DAL_get_WizardCheck_Data(WizardCheck *data, guint idx)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("sys.security.usable_defpw", &ret_value, NULL))
	{
		data->usable_defpw = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else ret = 1;

	if(nf_sysdb_get_key1("usr.U%d.pass", idx, &ret_value, NULL))
	{
		g_stpcpy(data->pw, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	else ret = 2;

	if(nf_sysdb_get_key0("sys.info.netwizard_enable", &ret_value, NULL))
	{
		data->netwiz = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else ret = 3;

	return ret;
}

guint DAL_set_WizardCheck_Data(gchar *key, gboolean data)
{

	GValue set_value = {0,};
	gchar buf[STRING_SIZE_16+1];

	DDEBUG_START
	nf_sysdb_lock(NF_SYSDB_CATE_SYS);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data);
	if(!nf_sysdb_set_key0(key, &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 1;
	}
 	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
 	DDEBUG_END
	return 0;
}

guint DAL_get_holiday_count()
{
	guint ret = 0;
	GValue ret_value = {0,};


	if(nf_sysdb_get_key0("sys.usrdef.holiday.HCNT", &ret_value, NULL))
	{
		ret = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}

	return ret;
}
guint DAL_set_holiday_count(guint cnt)
{
	guint ret = 0;
	GValue set_value = {0,};

	DDEBUG_START
	
	nf_sysdb_lock(NF_SYSDB_CATE_SYS);

	g_value_init(&set_value, G_TYPE_UINT);
    	g_value_set_uint(&set_value, cnt);

	if(nf_sysdb_set_key0("sys.usrdef.holiday.HCNT", &set_value, NULL))
	{
		g_value_unset(&set_value);
	}
	else
	{
		ret = 1;
		DDEBUG_FAIL
	}	
	
	nf_sysdb_unlock(NF_SYSDB_CATE_SYS);

	DDEBUG_END

	return ret;
}



guint DAL_get_UsrDefHoliday_Data(UsrDefHolidayData *data, guint holidayIdx)
{
	guint ret = 0;
	GValue ret_value = {0,};
	gint cnt = DAL_get_holiday_count();
	gint i;
	gchar holiday[11];

	if(nf_sysdb_get_key1("sys.usrdef.holiday.H%d.date", holidayIdx, &ret_value, NULL))
	{
		g_stpcpy(holiday, g_value_get_string(&ret_value));
		_convert_to_holi(holiday, data);
		g_value_unset(&ret_value);
	}
	else	ret = 1;
	
	return ret;
}

guint DAL_set_UsrDefHoliday_Data(UsrDefHolidayData *data, guint holidayIdx)
{
	guint ret = 0;
	GValue set_value = {0,};
	gchar holiday[11];
	gchar yoil;
	DDEBUG_START
	
	nf_sysdb_lock(NF_SYSDB_CATE_SYS);
	
	g_value_init(&set_value, G_TYPE_STRING);
	yoil = _get_yoil(data);
	g_message("yoil : %c, data->type : %c", yoil, data->type); 
	
	sprintf(holiday, "%04d%02d%02d%c%c", data->year, data->month, data->day, yoil, /*data->week,*/ data->type);
	printf("[%04d][%02d][%02d][%c][%c]", data->year, data->month, data->day, yoil, /*data->week,*/ data->type);

	g_message("holiday len: %d, holiday : %s",strlen(holiday), holiday);
	
	g_value_set_string(&set_value, holiday);
	if(!nf_sysdb_set_key1("sys.usrdef.holiday.H%d.date", holidayIdx, &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);
	
	nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
	DDEBUG_END

	return 0;
}

guint DAL_default_UsrDefHoliday_Data(guint holidayIdx)
{
	guint ret = 0;
	GValue set_value = {0,};
	gchar holiday[11];
	DDEBUG_START
	
	nf_sysdb_lock(NF_SYSDB_CATE_SYS);
	
	g_value_init(&set_value, G_TYPE_STRING);
	
	sprintf(holiday, "");

	g_value_set_string(&set_value, holiday);
	if(!nf_sysdb_set_key1("sys.usrdef.holiday.H%d.date", holidayIdx, &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);
	
	nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
	DDEBUG_END

	return 0;
}


/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
guint DAL_set_wizard_datetime_data(DATATIME_DATA_T *data)
{
	GValue set_value = {0,};

	DDEBUG_START

	nf_sysdb_lock(NF_SYSDB_CATE_SYS);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->dateFormat);
	if(!nf_sysdb_set_key0("sys.date.dateform", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 1;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->timeFormat);
	if(!nf_sysdb_set_key0("sys.date.timeform", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 2;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->timeZone);
	if(!nf_sysdb_set_key0("sys.date.tz_index", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 5;
	}
	g_value_unset(&set_value);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data->dst);
	if(!nf_sysdb_set_key0("sys.date.daylight", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 6;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_SYS, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_SYS);

	DDEBUG_END
	return 0;
}

/**
	@brief
	@param[in]
	@param[in]
	@param[out]
	@return
*/
gboolean DAL_get_wizard_record_data(RECORD_DATA_T *data)
{
	GValue ret_value = {0};
	GType gtype;


	if(nf_sysdb_get_key0("rec.auto_config", &ret_value, NULL))
	{
		data->autoConfig = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}else {
		g_warning("couldn't get a rec.auto_config value");

		DDEBUG_FAIL
		return FALSE;
	}

	return TRUE;
}

gboolean DAL_set_wizard_record_data(RECORD_DATA_T *data)
{
	GValue set_value = {0};

	DDEBUG_START

	g_return_val_if_fail(data != NULL, FALSE);

	nf_sysdb_lock(NF_SYSDB_CATE_REC);


	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, data->autoConfig);
	if(!nf_sysdb_set_key0("rec.auto_config", &set_value, NULL)) {
		g_warning("couldn't set a rec.auto_config value");

		nf_sysdb_unlock(NF_SYSDB_CATE_REC);

		DDEBUG_FAIL
		return FALSE;
	}
	g_value_unset(&set_value);

	nf_sysdb_unlock(NF_SYSDB_CATE_REC);

	DDEBUG_END
	return TRUE;
}

void DAL_get_QR_Code_URL(gchar *url)
{
	guint ret = 0;
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("net.sequrinet.easyip.mobileproxy", &ret_value, NULL))
	{
		g_stpcpy(url, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
}

gint DAL_get_Sequrinet_Enable(gboolean *enable)
{
	GValue ret_value = {0,};

	if(nf_sysdb_get_key0("sys.sequrinet.factory_enabled", &ret_value, NULL))
	{
		*enable = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
	    return -1;
	}

	return 0;
}

guint DAL_set_Sequrinet_Status(gboolean data)
{

	GValue set_value = {0,};

	DDEBUG_START
	nf_sysdb_lock(NF_SYSDB_CATE_NET);

	g_value_init(&set_value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&set_value, data);
	if(!nf_sysdb_set_key0("net.sequrinet.enabled", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		DDEBUG_FAIL
		return 1;
	}
 	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_NET, 0, 0, 0);
	
	nf_sysdb_unlock(NF_SYSDB_CATE_NET);
 	DDEBUG_END
	return 0;
}

gint DAL_get_Sequrinet_Status(gboolean *status)
{
	GValue ret_value = {0,};

    DDEBUG_START
	if(nf_sysdb_get_key0("net.sequrinet.enabled", &ret_value, NULL))
	{
		*status = g_value_get_boolean(&ret_value);
		g_value_unset(&ret_value);
	}
	else
	{
	    DDEBUG_FAIL
	    return -1;
	}
	DDEBUG_END
	return 0;
}

guint DAL_get_user_defined_dst_data(UDD_DATA_T *data)
{
    GValue ret_value = {0, };
    guint ret = 0;

    if (nf_sysdb_get_key0("sys.date.udd_use", &ret_value, NULL))
    {
        data->use = g_value_get_int(&ret_value);
        g_value_unset(&ret_value);
    }
    else ret = 1;

    if (nf_sysdb_get_key0("sys.date.dst_name", &ret_value, NULL))
    {
        g_stpcpy(data->dst_name, g_value_get_string(&ret_value));
        g_value_unset(&ret_value);
    }
    else ret = 2;

    if (nf_sysdb_get_key0("sys.date.std_name", &ret_value, NULL))
    {
        g_stpcpy(data->std_name, g_value_get_string(&ret_value));
        g_value_unset(&ret_value);
    }
    else ret = 3;

    if (nf_sysdb_get_key0("sys.date.dst_offset", &ret_value, NULL))
    {
        data->offset = g_value_get_int(&ret_value);
        g_value_unset(&ret_value);
    }
    else ret = 4;

    if (nf_sysdb_get_key0("sys.date.dst_start", &ret_value, NULL))
    {
        data->start= g_value_get_uint(&ret_value);
        g_value_unset(&ret_value);
    }
    else ret = 5;

    if (nf_sysdb_get_key0("sys.date.dst_end", &ret_value, NULL))
    {
        data->end = g_value_get_uint(&ret_value);
        g_value_unset(&ret_value);
    }
    else ret = 6;

    return ret;
}

guint DAL_set_user_defined_dst_data(UDD_DATA_T *data)
{
    GValue set_value = {0,};

    DDEBUG_START

    nf_sysdb_lock(NF_SYSDB_CATE_SYS);

    g_value_init(&set_value, G_TYPE_INT);
    g_value_set_int(&set_value, data->use);
    if (!nf_sysdb_set_key0("sys.date.udd_use", &set_value, NULL))
    {
        g_value_unset(&set_value);
        nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
        DDEBUG_FAIL
        return 1;
    }
    g_value_unset(&set_value);

    g_value_init(&set_value, G_TYPE_STRING);
    g_value_set_string(&set_value, data->dst_name);
    if (!nf_sysdb_set_key0("sys.date.dst_name", &set_value, NULL))
    {
        g_value_unset(&set_value);
        nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
        DDEBUG_FAIL
        return 2;
    }
    g_value_unset(&set_value);

    g_value_init(&set_value, G_TYPE_STRING);
    g_value_set_string(&set_value, data->std_name);
    if (!nf_sysdb_set_key0("sys.date.std_name", &set_value, NULL))
    {
        g_value_unset(&set_value);
        nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
        DDEBUG_FAIL
        return 3;
    }
    g_value_unset(&set_value);

    g_value_init(&set_value, G_TYPE_INT);
    g_value_set_int(&set_value, data->offset);
    if (!nf_sysdb_set_key0("sys.date.dst_offset", &set_value, NULL))
    {
        g_value_unset(&set_value);
        nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
        DDEBUG_FAIL
        return 4;
    }
    g_value_unset(&set_value);

    g_value_init(&set_value, G_TYPE_UINT);
    g_value_set_uint(&set_value, data->start);
    if (!nf_sysdb_set_key0("sys.date.dst_start", &set_value, NULL))
    {
        g_value_unset(&set_value);
        nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
        DDEBUG_FAIL
        return 5;
    }
    g_value_unset(&set_value);

    g_value_init(&set_value, G_TYPE_UINT);
    g_value_set_uint(&set_value, data->end);
    if (!nf_sysdb_set_key0("sys.date.dst_end", &set_value, NULL))
    {
        g_value_unset(&set_value);
        nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
        DDEBUG_FAIL
        return 6;
    }
    g_value_unset(&set_value);

    nf_sysdb_unlock(NF_SYSDB_CATE_SYS);

    DDEBUG_END

    return 0;
}

/**
	@brief
	@param[in]
	@param[out]
	@return
*/

/**
	@brief				sysdb ¼³Á¤
	@param[in]	key		¼³Á¤ ÇÏ°í ½Í?º sysdb key
	@param[in]	idx1	¼³Á¤ ÇÏ°í ½Í?º idx1 ¹øÈ£
	@param[in]	idx2	¼³Á¤ ÇÏ°í ½Í?º idx2 ¹øÈ£
	@param[out]	retval	¼³Á¤ °á°ú
	@param[out]	error	return location for a #GError, or %NULL
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
