/*
 * vw_qc_test_new.c
 *        - dependency :
 *
 *
 * Written by JungKyu Park. <parangi@itxsecurity.com>
 * Copyright (c) ITX security, Dec 12, 2012
 *
 * Modified by JeongHo Yang. <yanggungg@itxsecurity.com>
 * Copyright (c) ITX security, Jun 19, 2015
 */


#include <string.h>

#include "nf_afx.h"
#include "nf_common.h"
#include "nf_sysman.h"

#include "nf_qc.h"
#include "nf_qc_app.h"
#if defined(SCHIP_COPY_PROTECTION)
	#include "nf_api_param_hw_enc.h"
#endif

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"
#include "tools/nf_ui_tool.h"

#include "modules/mda.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfcombobox.h"
#include "viewers/objects/nfbutton.h"
#include "viewers/objects/nflistbox.h"
#include "viewers/objects/nftab.h"
#include "viewers/objects/nfcheckbutton.h"
#include "viewers/objects/nftimelabel.h"

#include "vw.h"
#include "vw_qc_test_new.h"
//#include "vw_qc_pannel.h"
//#include "vw_qc_test_new_keypad_popup.h"
#include "vw_qc_test_new_audio_test_popup.h"

#include "vw_desc.h"

#include "scm.h"
#include "smt.h"
#include "ix_mem.h"
#include "scm_internal.h"

#define QC_CODE_TEST_MODE       (0)



#if defined(ENABLE_QC_TEST_PANNEL)
#define QC_TEST_SIZE_W			(1024)
#else
#define QC_TEST_SIZE_W			(274 * 3 + 22)
#endif
#define QC_TEST_SIZE_H			(770)
#define QC_TEST_POS_X			(DISPLAY_ACTIVE_WIDTH-192-20-QC_TEST_SIZE_W)
#define QC_TEST_POS_Y			((DISPLAY_ACTIVE_HEIGHT-108-QC_TEST_SIZE_H)/2)

#define MSG_LIST_SIZE_W         (QC_TEST_SIZE_W-20)
#define MSG_LIST_SIZE_H         (QC_TEST_SIZE_H-100)

#define TAB_BUTTON_POS_X        (10)
#define TAB_BUTTON_POS_Y        (50)
#define TAB_BUTTON_SIZE_H       (43)

#define TAB_PAGE_SIZE_W         (QC_TEST_SIZE_W - 20)
#define TAB_PAGE_SIZE_H         (QC_TEST_SIZE_H - TAB_PAGE_POS_Y - 100)
#define TAB_PAGE_POS_X          (TAB_BUTTON_POS_X)
#define TAB_PAGE_POS_Y          (TAB_BUTTON_POS_Y + TAB_BUTTON_SIZE_H)

#define LABEL_SIZE_H            (40)
#define LABEL_SIZE_W            (((TAB_PAGE_SIZE_W * 2) / 5) - 10)
#define LABEL_SPACE             (guint)(2)
#define LABEL_TITLE_POS_X       (guint)(10)
#define LABEL_START_POS_Y       (15)

#define PIXBUF_BASE_POS_X       (TAB_PAGE_POS_X + LABEL_TITLE_POS_X + LABEL_SIZE_W + LABEL_SPACE + ((LABEL_SIZE_W - 40)/2))
#define PIXBUF_BASE_POS_Y       (TAB_PAGE_POS_Y)

#define PROG_POS_X				((DISPLAY_ACTIVE_WIDTH - 653) / 2)
#define PROG_POS_Y				((DISPLAY_ACTIVE_HEIGHT - 106) / 2)

#define FUNC_KEY_MAX            (12)
#define QC_ITEM_CNT				(NF_SYSMAN_QC_ITEM_MAX_NR)


enum{
    TAB_SYSINFO = 0,
    TAB_AUTO,
    TAB_MANUAL,
    TAB_CNT
};

enum{
    SYSINFO_MODEL = 0,
    SYSINFO_FWVER,
    SYSINFO_HWVER,
    SYSINFO_ACPOWER,
    SYSINFO_DISKCAP,
    SYSINFO_DISKUSAGE,
    SYSINFO_DISKNUM,
    SYSINFO_IPADDR,
    SYSINFO_MACADDR,
    SYSINFO_RESOLUTION,
    SYSINFO_PANELTYPE,
    SYSINFO_RCTYPE,
    SYSINFO_LOCALTIME,
    SYSINFO_RTC,

    SYSINFO_CNT
};

enum{
    AUTO_ALARM = 0,
    AUTO_NETWORK,
    AUTO_AUDIO,
    AUTO_RS485,
    AUTO_FAN,
    AUTO_TEMPER,
    AUTO_RS232,
    AUTO_POE,
    AUTO_HDD,
    AUTO_ODD,

    AUTO_CNT
};

enum{
    MANUAL_RS485 = 0,
    MANUAL_BUZZER,
    MANUAL_LED,
    MANUAL_KEYPAD,
	MANUAL_REMOCON,
    MANUAL_AUDIO,
    MANUAL_FACKEY,
    MANUAL_MAC,
    MANUAL_JOG,
    MANUAL_SHUTTLE,
    MANUAL_HDD,
    MANUAL_ODD,
    MANUAL_USB,
    MANUAL_FAN,
    MANUAL_HDMI,
    MANUAL_VGA,
    MANUAL_ANALOGVIEW_TVI,
    MANUAL_ANALOGVIEW_AHD,
    MANUAL_SPOT,
    MANUAL_RTC,
    MANUAL_LICENSE,

    MANUAL_CNT
};

typedef enum{
    LABEL_TITLE = 0,
    LABEL_STATUS,
    LABEL_TIME,
}LABEL_TYPE;

enum{
    RESULT_INFO = 0,
    RESULT_PASS,
    RESULT_FAIL,
};

enum{
    BTN_FACINIT,
    BTN_FORMAT,
    BTN_SAVE,

    BTN_CNT
};

enum {
	DF_YMD = 0,
	DF_MDY,
	DF_DMY,

	NUM_DATE_FORMATS,
};

enum {
	TIME_MODE_24H = 0,
	TIME_MODE_12H,

	NUM_TIME_MODES,
};


typedef struct _QC_RTC{
    gchar year[5];
    gchar mon[4];
    gchar day[3];
    gchar wd[4];
    gchar h[3];
    gchar m[3];
    gchar s[3];
}QC_RTC;

typedef struct _QC_TEST_NEW{
    gint expose;
    gint result;
    NFOBJECT *title;
    NFOBJECT *status;
    NFOBJECT *btn;
}QC_TEST_NEW;


typedef struct _QC_TEST
{
    NFOBJECT            *key_obj;
    gchar               label[16];
    QC_TEST_CB_FUNC	    cb_func;
	gpointer	        cb_data;
} QC_TEST;

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_msglist_obj = 0;
static NFOBJECT *g_btns[MANUAL_CNT];
static QC_TEST g_qc[FUNC_KEY_MAX] = {0, };
static QC_TEST_NEW g_qc_new[TAB_CNT][QC_ITEM_CNT] = {0, };
static NFOBJECT *wait_pop;
static NFOBJECT *g_saveBtn;
static gint g_fac_init_run = 0;
static QC_RTC g_rtc;
static guint g_time_update = 0;
static gint g_mac_error = 0;
static gboolean g_rmc_res = FALSE;
static gboolean _vw_qc_remocon_test_popup(NFWINDOW *parent, guint x, guint y);


static void _create_waitbox(void)
{
    wait_pop = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
}

static void _remove_waitbox(void)
{
    if(wait_pop)
    {
        nftool_remove_waitbox(wait_pop);
        wait_pop = NULL;
    }
}

static int _set_login_user(gint idx)
{
	FILE *fp;

	fp = fopen("./data/gui/login_user.itx", "w");

	if(fp){
		fprintf(fp, "%d", idx);
		fclose(fp);
	}

	return 1;
}

static gint _parse_rtc(gchar *rtc)
{
    gint j = 0, i, cnt = 0, len = 0;
    gchar *buf = rtc;
    gint sp_cnt = 0;

g_message("%s, %d, \n", __FUNCTION__, __LINE__);
    if(rtc)
    {
        rtc[24] = '\0';
        return 0;
    }

    while(buf != NULL){
        len++;
        buf++;
    }

    buf = rtc;
    for(i = cnt;i < len;i++)
    {
        if(buf[i] == 32){
            sp_cnt++;
            j = 0;
            continue;
        }

        if(sp_cnt == 0){
            g_rtc.wd[j++] = buf[i];
        }
        else if(sp_cnt == 1)
        {
            g_rtc.mon[j++] = buf[i];
        }
        else if(sp_cnt == 2)
        {

        }
    }
g_message("%s, %d, \n", __FUNCTION__, __LINE__);
    return 0;
}

static nftl_df_type _prvTransDateFormat(gint db_index)
{
    nftl_df_type ret;

    if(db_index == DF_YMD)          ret = NFTL_DF_YMD;
    else if(db_index == DF_MDY)     ret = NFTL_DF_MDY;
    else if(db_index == DF_DMY)     ret = NFTL_DF_DMY;
    else    ret = NFTL_DF_HIDE;

    return ret;
}

static gboolean _timeout_date_time_update(gpointer data)
{
	NFTIMELABEL* ti_obj;
	GTimeVal tv;
	GTimeVal tv_temp;
	gint i;

	memset(&tv, 0x00, sizeof(GTimeVal));
	memset(&tv_temp, 0x00, sizeof(GTimeVal));
	ti_obj = (NFTIMELABEL*)data;

	g_get_current_time(&tv);

	NFUTIL_THREADS_ENTER();

	nfui_nftimelabel_get_datetime(ti_obj, &tv_temp);

	if(tv.tv_sec != tv_temp.tv_sec)
	{
		nfui_nftimelabel_set_datetime_expose(ti_obj, &tv);
	}
	NFUTIL_THREADS_LEAVE();

	return TRUE;
}

static int _restart_timer()
{
	if (g_time_update) return -1;
	g_time_update = g_timeout_add(300, _timeout_date_time_update, g_qc_new[TAB_SYSINFO][SYSINFO_LOCALTIME].status);
	return 0;
}

static int _stop_timer()
{
	if (!g_time_update) return -1;
	g_source_remove(g_time_update);
	g_time_update = 0;
	return 0;
}

static gint _set_license()
{
    NF_SYSDB_LICENSE_INFO lic_info;
    LicenseData lic_data;
    gchar key[MAX_LICENSE_CNT][36];
    gchar lic_name[256];
    gchar strBuf[256];
    gint i, j, lic_cnt = 0, valid_key_cnt = 0;

    if (!ivsc.dfunc.support_license) return 0;
    
    memset(key, 0x00, sizeof(key));
    memset(lic_name, 0x00, sizeof(lic_name));
    memset(&lic_data, 0x00, sizeof(LicenseData));
    //api ȣ�� 
    lic_cnt = nf_sysman_qc_lic_write(key);

    if (lic_cnt < 0) {
        strcpy(strBuf, "ERROR");
        g_qc_new[TAB_MANUAL][MANUAL_LICENSE].result = RESULT_FAIL;

        nfui_nflabel_set_text((NFLABEL*)g_qc_new[TAB_MANUAL][MANUAL_LICENSE].status, strBuf);
        nfui_signal_emit(g_qc_new[TAB_MANUAL][MANUAL_LICENSE].status, GDK_EXPOSE, FALSE);
        return 0;
    }

    for (i = 0, j = 0; i < lic_cnt; i++)
    {
        memset(&lic_info, 0x00, sizeof(NF_SYSDB_LICENSE_INFO));

        if (scm_license_decoding_key(-1, key[i], &lic_info) == 0)
        {
            if (strlen(lic_name) == 0) {
                sprintf(lic_name, "%s", lic_info.name);
            }
            else {
                sprintf(lic_name, ", %s", lic_info.name);
            }

            strcpy(lic_data.key_data[j].key, key[i]);
            lic_data.key_data[j].acquired_date = ifn_get_local_timet();
            valid_key_cnt++;
            j++;
        }
    }

    memset(strBuf, 0x00, sizeof(strBuf));

    if (valid_key_cnt) {
        sprintf(strBuf, "%d (%s)", valid_key_cnt, lic_name);
        lic_data.count = valid_key_cnt;
        DAL_set_license_data(&lic_data);
        DAL_save_db("sys");
    }
    else {
        strcpy(strBuf, "There is not free license.");
    }

    nfui_nflabel_set_text((NFLABEL*)g_qc_new[TAB_MANUAL][MANUAL_LICENSE].status, strBuf);
    nfui_signal_emit(g_qc_new[TAB_MANUAL][MANUAL_LICENSE].status, GDK_EXPOSE, FALSE);

    g_qc_new[TAB_MANUAL][MANUAL_LICENSE].result = RESULT_PASS;
    
    return 0;
}

static gint _get_model_name(gchar *strBuf)
{
    SysInfoData sys_info;

	memset(&sys_info, 0, sizeof(SysInfoData));

	DAL_get_sysInfo_data(&sys_info);
    g_sprintf(strBuf, "%s", sys_info.model);

    return 0;
}

static gint _get_fw_version(gchar *strBuf)
{
    gchar strTemp[40];

    var_get_fake_fwver(strTemp, 40);
    g_sprintf(strBuf, "%s", strTemp);

    return 0;
}

static gint _get_last_fw_date(gchar *strBuf)
{
    gchar strTemp[40];
    SysInfoData sys_info;

    memset(strTemp, 0x00, sizeof(strTemp));
	memset(&sys_info, 0, sizeof(SysInfoData));

	DAL_get_sysInfo_data(&sys_info);

	if (sys_info.fwupTime) dtf_get_local_datetime(sys_info.fwupTime, strTemp);
	else sprintf(strTemp, "N/A");
    g_sprintf(strBuf, "%s", strTemp);

    return 0;
}

static gint _get_hw_version(gchar *strBuf)
{
    gchar strTemp[40];

    memset(strTemp, 0x00, sizeof(strTemp));
   	scm_get_hw_ver(strTemp, 40);
    g_sprintf(strBuf, "%s", strTemp);

    return 0;
}

static gint _get_disk_capacity(gchar *strBuf)
{
    gchar strTemp[40];
	guint64 size = 0;
	DISK_CAPINFO_T d_info;

	scm_get_disk_capinfo(INTERNAL, &d_info);
	size = d_info.tsize;
	scm_get_disk_capinfo(EXTERNAL, &d_info);
	size += d_info.tsize;

    memset(strTemp, 0x00, sizeof(strTemp));
	ifn_convert_storage_size(strTemp, size);
    g_sprintf(strBuf, "%s", strTemp);

    return 0;
}

static gint _get_disk_usage(gchar *strBuf)
{
    gchar strTemp[40];
	guint64 size;
	NF_NOTIFY_INFO info;

    memset(strTemp, 0x00, sizeof(strTemp));
	scm_get_disk_usage_data(&info);
	size = (guint64)info.d.params[0];
	size *= 1024;
	ifn_convert_storage_size(strTemp, size);
	g_sprintf(strBuf, "%s", strTemp);

    return 0;
}

static gint _get_disk_num(gchar *strBuf)
{
    gchar strTemp[40];
    DISK_CAPINFO_T cap_info;
    gint run_cnt;

    memset(&cap_info, 0x00, sizeof(DISK_CAPINFO_T));
    scm_get_disk_capinfo(INTERNAL, &cap_info);
    run_cnt = cap_info.tdisk_count;

    memset(&cap_info, 0x00, sizeof(DISK_CAPINFO_T));
    scm_get_disk_capinfo(EXTERNAL, &cap_info);
    run_cnt += cap_info.tdisk_count;

    memset(strTemp, 0x00, sizeof(strTemp));
	sprintf(strTemp, "%d", run_cnt);
    g_sprintf(strBuf, "%s", strTemp);

    return 0;
}

static gint _get_ip_addr(gchar *strBuf)
{
    gchar strTemp[40];

    memset(strTemp, 0x00, sizeof(strTemp));
	if(scm_get_ip_addr_str(strTemp, 40) < 0)
		sprintf(strTemp, "%d.%d.%d.%d", 0, 0, 0, 0);
    g_sprintf(strBuf, "%s", strTemp);

    return 0;
}

static gint _get_mac_addr(gchar *strBuf)
{
    SysInfoData sys_info;

	memset(&sys_info, 0, sizeof(SysInfoData));
	DAL_get_sysInfo_data(&sys_info);
    g_sprintf(strBuf, "%s", sys_info.macAddr);

    return 0;
}

static gint _get_new_mac_addr(gchar *strBuf)
{
#if defined(SCHIP_COPY_PROTECTION)
    strcpy(strBuf, nf_param_hw_enc_get_qc_mac());
#endif

    return 0;
}

static gint _get_rtsp_port(gchar *strBuf)
{
    gchar strTemp[40];
    SysInfoData sys_info;

    memset(strTemp, 0x00, sizeof(strTemp));
	memset(&sys_info, 0, sizeof(SysInfoData));

	DAL_get_sysInfo_data(&sys_info);
	sprintf(strTemp, "%d", sys_info.rtspPort);
    g_sprintf(strBuf, "RTSP SERVICE PORT : %s", strTemp);

    return 0;
}

static gint _get_web_port(gchar *strBuf)
{
    gchar strTemp[40];
    SysInfoData sys_info;

    memset(strTemp, 0x00, sizeof(strTemp));
	memset(&sys_info, 0, sizeof(SysInfoData));

	DAL_get_sysInfo_data(&sys_info);
	sprintf(strTemp, "%d", sys_info.webPort);
    g_sprintf(strBuf, "WEB SERVICE PORT : %s", strTemp);

    return 0;
}

static gint _get_resolution(gchar *strBuf)
{
    gchar strRes[32], strMode[32];
	gchar *uStr;

    memset(strRes, 0x00, sizeof(strRes));
    memset(strMode, 0x00, sizeof(strMode));

	if(!scm_get_video_resolution(strRes)) return FALSE;
	if(!scm_get_video_output(strMode)) 	  return FALSE;

	uStr = g_ascii_strup(strMode, -1);
    g_sprintf(strBuf, "%s (%s)", uStr, strRes);
	g_free(uStr);

    return 0;
}

static gint _get_ac_power_frequency(gchar *strBuf)
{
    gint ac;

    ac = DAL_get_sig_type();

    if(ac == -1) return -1;
    if(ac == 1){
        g_sprintf(strBuf, "50Hz");
    }
    else{
        g_sprintf(strBuf, "60Hz");
    }

    return 0;
}

static gint _get_panel_type(gchar *strBuf)
{
    gchar buf[32];

    memset(buf, 0x00, sizeof(buf));

    sprintf(strBuf, "%s", nf_api_param_hw_get_front_type());

    return 0;
}

static gint _get_rc_type(gchar *strBuf)
{
    gchar buf[32];

    memset(buf, 0x00, sizeof(buf));

    g_sprintf(strBuf, "%s", nf_api_param_hw_get_rc_type());

    return 0;
}

static gint _get_rtc_time(gchar *strBuf)
{
    gchar buf[64];

    memset(buf, 0x00, sizeof(buf));
    if(scm_appqc_get_rtc_str(buf)){
        _parse_rtc(buf);
        g_sprintf(strBuf, "%s", buf);
    }
    return 0;
}

static gint _get_odd_cnt()
{
    MEDIA_INFO_T *media_info;
    MEDIA_TYPE_E mtype;
    gint cnt, i;
    gint odd_cnt = 0;


	media_info = scm_new_media_list(&cnt);
	if (!media_info) return -1;

	for (i = 0; i < cnt; ++i)
	{
		mtype = scm_get_media_type(media_info[i].id);
		if (mtype == MTYPE_ODD)
		{
		    odd_cnt++;
		}
	}

	return odd_cnt;
}

static gint _get_supported_max_hdd_cnt()
{
    gint cnt;

    cnt = nf_sysman_hdd_num_check();

    return cnt;
}

static gint _get_result_hdd_test()
{
    gint ret = RESULT_FAIL;
    gint max_cnt = _get_supported_max_hdd_cnt();
    DISK_CAPINFO_T cap_info;
    gint run_cnt;

    memset(&cap_info, 0x00, sizeof(DISK_CAPINFO_T));
    scm_get_disk_capinfo(INTERNAL, &cap_info);
    run_cnt = cap_info.tdisk_count;

    memset(&cap_info, 0x00, sizeof(DISK_CAPINFO_T));
    scm_get_disk_capinfo(EXTERNAL, &cap_info);
    run_cnt += cap_info.tdisk_count;

    if(max_cnt == 0 || run_cnt == 0) {
        g_message("[QC] %s, %d, max_cnt : %d, run_cnt : %d", __FUNCTION__, __LINE__, max_cnt, run_cnt);
        return ret;
    }

    if(max_cnt == run_cnt){
        ret = RESULT_PASS;
    }
    else{
        ret = RESULT_FAIL;
    }

    return ret;
}

static gint _get_result_odd_test()
{
    gint ret = RESULT_FAIL;
    gint max_cnt = nf_sysman_odd_num_check();
    gint run_cnt = _get_odd_cnt();


    if(max_cnt == 0 || run_cnt == 0) {
        g_message("[QC] %s, %d, max_cnt : %d, run_cnt : %d", __FUNCTION__, __LINE__, max_cnt, run_cnt);
        return ret;
    }

    if(max_cnt == run_cnt){
        ret = RESULT_PASS;
    }
    else{
        ret = RESULT_FAIL;
    }

    return ret;
}

static gint _get_expose_alarm()
{
    gint expose = 0;

    if (ALARM_OUT_COUNT > 0) expose = 1;

    return expose;
}

static gint _get_expose_fan(gint mode)
{
    gint expose = 0;

    if (mode == TAB_AUTO)
    {
        if (ivsc.dfunc.chkevt.support_fan) expose = 1;
    }
    else
    {
        if (!ivsc.dfunc.chkevt.support_fan && FAN_CNT > 0) expose = 1;

#if defined(_IPX_0412M4) || defined(_IPX_0412M4E)
		expose = 0;
#endif

    }

    return expose;
}

static gint _get_expose_keypad()
{
    gint expose = 0;

    if (ivsc.dfunc.buzzer.support_keypad) expose = 1;

    return expose;
}

static gint _get_expose_remocon()
{
    gint expose = 0;

    if (nf_sysman_check_is_remocon() && ivsc.vendor_code != 30) expose = 1;

    return expose;
}

static gint _get_expose_rs485(gint mode)
{
    gint expose = 0;

    if (mode == TAB_AUTO)
    {
        if (RS485_CNT >= 2) expose = 1;
        if (ivsc.model_code == IPX_P4E_MODEL || ivsc.model_code == IPX_P5_MODEL) expose = 1;
    }
    else
    {
        if (RS485_CNT == 1) expose = 1;
        if (ivsc.model_code == IPX_P4E_MODEL || ivsc.model_code == IPX_P5_MODEL) expose = 0;
    }

    return expose;
}

static gint _get_expose_spot()
{
    gint expose = 0;

    if (ivsc.dfunc.support_spot) expose = 1;

    return expose;
}

static gint _get_expose_odd()
{
    gint expose = 0;

    if (nf_sysman_odd_num_check() > 0) expose = 1;

    return expose;
}

static gint _get_expose_hdd()
{
    gint expose = 1;

    return expose;
}

static gint _get_expose_rs232()
{
    gint expose = 0;

    if ((RS232_CNT > 0) && (ivsc.model_code != IPX_M4_MODEL))
		expose = 1;
#if defined(_IPX_0824P4E) || defined(_IPX_1648P4E)  || defined(_IPX_0412M4E) || defined(_IPX_0824M4E) || defined(_IPX_1648M4E) || defined(_IPX_32P4E) || defined(_IPX_32M4E) || defined(_IPX_32P5)
        expose = 0;
#endif

    return expose;
}

static gint _get_expose_fackey()
{
    gint expose = 0;

	expose = nf_sysman_is_support_factory_key();

    return expose;
}

static gint _get_expose_temperature()
{
    gint expose = 0;

    if (ivsc.dfunc.chkevt.support_temperature)
		expose = 1;

    return expose;
}

static gint _get_expose_audio()
{
    gint expose = 1;

#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
		if(nf_sysman_get_pba_type()) expose = 0;
#endif

    return expose;
}

static gint _get_expose_license()
{
    gint expose = 0;

    if (ivsc.dfunc.support_license) expose = 1;

    return expose;
}

static gint _get_expose_mac()
{
    gint expose = 0;

    if (ivsc.model_code == IPX_P4E_MODEL || ivsc.model_code == IPX_P5_MODEL) expose = 0;

    return expose;
}

static gint _get_qc_result_and_cnt(gchar result[][QC_ITEM_CNT][NF_SYSMAN_QC_ITEM_RESULT_MAX_LEN], gint *tested_cnt)
{
    gint item_cnt, j, tab = 1;
    gchar buf[64];

g_message("###yanggungg : %s, %d\n", __FUNCTION__, __LINE__);
    for(tab = 1;tab < 3;tab++)
    {
        if(tab == TAB_AUTO)
        {
            item_cnt = AUTO_CNT;
        }
        else
        {
            item_cnt = MANUAL_CNT;
        }

        for(j = 0; j < item_cnt; j++)
        {
            if(!g_qc_new[tab][j].expose) continue;

            if(tab == TAB_SYSINFO)
            {
            }
            else if(g_qc_new[tab][j].result == RESULT_PASS)
            {
                g_sprintf(buf, "%s:O", nfui_nflabel_get_text((NFLABEL*)g_qc_new[tab][j].title));
                strcpy(result[tab-1][j], buf);
            }
            else
            {
                g_sprintf(buf, "%s:X", nfui_nflabel_get_text((NFLABEL*)g_qc_new[tab][j].title));
                strcpy(result[tab-1][j], buf);
            }
            (*tested_cnt)++;
        }
    }
g_message("###yanggungg : %s, %d\n", __FUNCTION__, __LINE__);
    return 0;
}

static gint _copy_result(gchar buf_res[][QC_ITEM_CNT][NF_SYSMAN_QC_ITEM_RESULT_MAX_LEN], gchar send_res[][NF_SYSMAN_QC_ITEM_RESULT_MAX_LEN])
{
    gint tab, i, j=0;

g_message("###yanggungg : %s, %d\n", __FUNCTION__, __LINE__);
    for(tab = 0;tab < 2;tab++){
        for(i = 0;i < QC_ITEM_CNT;i++)
        {
            if(buf_res[tab][i] == NULL) continue;
            if(strlen(buf_res[tab][i]) == 0) continue;

            strcpy(send_res[j++], buf_res[tab][i]);
            g_message("SEND_RES : %s", send_res[j-1]);
        }
    }
g_message("###yanggungg : %s, %d\n", __FUNCTION__, __LINE__);
    return 0;
}

static gint _save_result()
{
    gchar buf_res[2][QC_ITEM_CNT][NF_SYSMAN_QC_ITEM_RESULT_MAX_LEN];
    gchar send_res[NF_SYSMAN_QC_ITEM_MAX_NR][NF_SYSMAN_QC_ITEM_RESULT_MAX_LEN];
    gint tested_cnt = 0, tested_tot;
    gint i, j, tab;

    memset(buf_res, NULL, sizeof(buf_res));
    _get_qc_result_and_cnt(buf_res, &tested_cnt);

    //show result
    for(tab = 0;tab < 2;tab++){
        for(i = 0;i < QC_ITEM_CNT; i++){
            g_message("buf_res : %s", buf_res[tab][i]);
        }
    }
    _copy_result(buf_res, send_res);

    nf_sysman_qc_result_save(send_res, tested_cnt);

    return 0;
}

static gint _shutdown_system()
{
    smt_set_service(SMT_SHUTDOWN);
    scm_shutdown_system(IRET_SCM_SHUTDOWN);
    scm_notify_to_system("dvr_status", NF_DVR_STATUS_SHUTDOWN);
    _create_waitbox();

    return 0;
}

static gint _get_obj_tab_and_index(NFOBJECT *obj, gint *tab)
{
    gint i, j, cnt;


    for(j = 0; j < TAB_CNT; j++)
    {
        if(j == TAB_SYSINFO){
            cnt = SYSINFO_CNT;
            *tab = TAB_SYSINFO;
        }
        else if(j == TAB_AUTO){
            cnt = AUTO_CNT;
            *tab = TAB_AUTO;
        }
        else{
            cnt = MANUAL_CNT;
            *tab = TAB_MANUAL;
        }
        for(i = 0; i < cnt; i++)
        {
            //g_message("###yanggungg : %s, %d, i : %d\n", __FUNCTION__, __LINE__, i);
            //g_message("OBJ : %p, BTN : %p, LABEL : %p\n", obj, g_qc_new[j][i].btn, g_qc_new[j][i].status);
            if(obj == g_qc_new[j][i].btn || obj == g_qc_new[j][i].status){
                return i;
            }
        }
    }
    if(j == TAB_CNT){
        g_message("###yanggungg : %s, %d, CAN NOT FOUND OBJECT\n", __FUNCTION__, __LINE__);
    }
    return -1;
}

static void _draw_image(NFOBJECT *obj, gint result)
{
    GdkPixbuf *img;
    GdkDrawable *drawable;
    GdkGC *gc;
    gint size_w, size_h;


    drawable = nfui_nfobject_get_window(obj);
    gc = nfui_nfobject_get_gc(obj);

    if(result == RESULT_PASS){
        img = nfui_get_image_from_file(("qc_test_pass.png"), NULL);
    }
    else if(result == RESULT_FAIL){
        img = nfui_get_image_from_file(("qc_test_fail.png"), NULL);
    }
    else{
        return;
    }

    nfutil_draw_pixbuf(drawable, gc, img, PIXBUF_BASE_POS_X, (obj->y + PIXBUF_BASE_POS_Y), -1, -1, NFALIGN_LEFT, 0);

	nfui_nfobject_gc_unref(gc);
}

static gboolean _check_result()
{
    gint tab, i, j;


    if(!g_fac_init_run) return FALSE;

    g_message("[QC RESULT] ======================================");
    for(tab = TAB_AUTO;tab < TAB_CNT;tab++){
        if(tab == TAB_AUTO){
            i = AUTO_CNT;
            g_message("[QC RESULT] AUTO_CNT : %d", i);
        }
        else{
            i = MANUAL_CNT;
    g_message("[QC RESULT] ======================================");
            g_message("[QC RESULT] MANUAL_CNT : %d", i);
        }
        for(j = 0;j < i;j++){
            if(!g_qc_new[tab][j].expose) {
                g_message("[QC RESULT] NOT EXPOSE : %d", j);
                continue;
            }
            g_message("[QC RESULT] ITEM : %d, result : %s", j, ((g_qc_new[tab][j].result==RESULT_PASS)?"PASS":"FAIL"));
            if(g_qc_new[tab][j].result != RESULT_PASS) return FALSE;
        }
    }
    g_message("[QC RESULT] ======================================");
    return TRUE;
}



static NFOBJECT *_make_fixed()
{
    NFOBJECT *fixed;

    fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed, ((TAB_PAGE_SIZE_W * 2) / 5) - 10, LABEL_SIZE_H);
    nfui_nfobject_modify_bg((NFOBJECT*)fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));

    return fixed;
}

static NFOBJECT *_make_label(gchar *str, LABEL_TYPE type, gint expose)
{
    NFOBJECT *obj;


    if(str == NULL){
        strcpy(*str, "");
    }
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(str, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
    if(type == LABEL_TITLE)
    {
        nfui_nfobject_set_size(obj, ((TAB_PAGE_SIZE_W * 2) / 5) - 10, LABEL_SIZE_H);
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
        nfui_nfobject_support_multi_lang(obj, TRUE);
    }
    else if(type == LABEL_TIME)
    {
        obj = (NFOBJECT*)nfui_nftimelabel_new();
    	nfui_nftimelabel_set_fg_color((NFTIMELABEL*)obj, COLOR_IDX(295));
    	nfui_nftimelabel_set_bg_color((NFTIMELABEL*)obj, COLOR_IDX(230));
    	nfui_nftimelabel_set_mode((NFTIMELABEL*)obj, _prvTransDateFormat(DF_YMD), NFTL_TM_12H);
    	nfui_nftimelabel_set_size((NFTIMELABEL*)obj, ((TAB_PAGE_SIZE_W * 2) / 5) - 10, LABEL_SIZE_H);
    	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    }
    else
    {
        nfui_nfobject_set_size(obj, ((TAB_PAGE_SIZE_W * 2) / 5) - 10, LABEL_SIZE_H);
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
        nfui_nfobject_support_multi_lang(obj, TRUE);
    }
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);

    if(expose)  nfui_nfobject_show(obj);

    return obj;
}

static NFOBJECT *_make_button(const gchar *str, gint expose)
{
    NFOBJECT *btn;

    btn = nftool_normal_button_create_subtab_type1(str, ((TAB_PAGE_SIZE_W * 1) / 5) - 10);
    nfui_nfobject_support_multi_lang(btn, TRUE);

    if(expose)  nfui_nfobject_show(btn);

    return btn;
}

static NFOBJECT *_make_check_button(gint expose, NFOBJECT *fixed)
{
    NFOBJECT *btn;
    gint size_w, size_h;
	guint inactive_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(900), COLOR_IDX(901), COLOR_IDX(902), COLOR_IDX(904)};
	guint active_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(903), COLOR_IDX(901), COLOR_IDX(902), COLOR_IDX(904)};


	btn = (NFOBJECT*)nfui_checkbutton_new(FALSE);
	nfui_check_button_set_skin_type(NF_CHECKBUTTON(btn), NFCHECK_TYPE_NORMAL);
	nfui_check_get_size((NFCHECKBUTTON*)btn, &size_w, &size_h);
	nfui_nfobject_set_size(btn, fixed->width, fixed->height);
	nfui_nffixed_put((NFFIXED*)fixed, btn, ((((TAB_PAGE_SIZE_W * 2) / 5) - 10)-size_w)/2, (LABEL_SIZE_H - size_h)/2);
	nfui_nfobject_show(btn);

	if(expose) nfui_nfobject_show(fixed);

	return btn;
}

static NFOBJECT *_make_manual_test_button(const gchar *str, gint expose)
{
    NFOBJECT *btn;
    GdkPixbuf *chk_inact_img[NFCHECK_INACTIVE_STATES];
	GdkPixbuf *chk_act_img[NFCHECK_ACTIVE_STATES];
	guint inactive_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(900), COLOR_IDX(901), COLOR_IDX(902), COLOR_IDX(904)};
	guint active_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(903), COLOR_IDX(901), COLOR_IDX(902), COLOR_IDX(904)};

	chk_inact_img[0] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_NI_164", ((TAB_PAGE_SIZE_W * 1) / 5) - 10, IMG_SUBTAB_BTN1_N_L, IMG_SUBTAB_BTN1_N_M, IMG_SUBTAB_BTN1_N_R);
	chk_inact_img[1] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_FI_164", ((TAB_PAGE_SIZE_W * 1) / 5) - 10, IMG_SUBTAB_BTN1_O_L, IMG_SUBTAB_BTN1_O_M, IMG_SUBTAB_BTN1_O_R);
	chk_inact_img[2] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_PI_164", ((TAB_PAGE_SIZE_W * 1) / 5) - 10, IMG_SUBTAB_BTN1_P_L, IMG_SUBTAB_BTN1_P_M, IMG_SUBTAB_BTN1_P_R);
	chk_inact_img[3] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_DI_164", ((TAB_PAGE_SIZE_W * 1) / 5) - 10, IMG_SUBTAB_BTN1_D_L, IMG_SUBTAB_BTN1_D_M, IMG_SUBTAB_BTN1_D_R);

	chk_act_img[0] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_SI_164", ((TAB_PAGE_SIZE_W * 1) / 5) - 10, IMG_SUBTAB_BTN1_S_L, IMG_SUBTAB_BTN1_S_M, IMG_SUBTAB_BTN1_S_R);
	chk_act_img[1] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_FI_164", ((TAB_PAGE_SIZE_W * 1) / 5) - 10, IMG_SUBTAB_BTN1_O_L, IMG_SUBTAB_BTN1_O_M, IMG_SUBTAB_BTN1_O_R);
	chk_act_img[2] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_PI_164", ((TAB_PAGE_SIZE_W * 1) / 5) - 10, IMG_SUBTAB_BTN1_P_L, IMG_SUBTAB_BTN1_P_M, IMG_SUBTAB_BTN1_P_R);
	chk_act_img[3] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_DI_164", ((TAB_PAGE_SIZE_W * 1) / 5) - 10, IMG_SUBTAB_BTN1_D_L, IMG_SUBTAB_BTN1_D_M, IMG_SUBTAB_BTN1_D_R);

	btn = (NFOBJECT*)nfui_checkbutton_new(FALSE);
	nfui_check_button_set_skin_type(NF_CHECKBUTTON(btn), NFCHECK_TYPE_DEF);
	nfui_check_set_text(NF_CHECKBUTTON(btn), str);
	nfui_check_button_set_inactive_image(NF_CHECKBUTTON(btn), chk_inact_img);
	nfui_check_button_set_active_image(NF_CHECKBUTTON(btn), chk_act_img);
	nfui_check_set_inactive_pango_font(NF_CHECKBUTTON(btn), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), inactive_font_color);
	nfui_check_set_active_pango_font(NF_CHECKBUTTON(btn), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), active_font_color);
	nfui_nfobject_set_size(btn, ((TAB_PAGE_SIZE_W * 1) / 5) - 10, 40);

	if(expose) nfui_nfobject_show(btn);

	return btn;

}

static NFOBJECT *_make_analogview_test_button(const gchar *str, gint expose)
{
    NFOBJECT *btn;
    GdkPixbuf *chk_inact_img[NFCHECK_INACTIVE_STATES];
	GdkPixbuf *chk_act_img[NFCHECK_ACTIVE_STATES];
	guint inactive_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(900), COLOR_IDX(901), COLOR_IDX(903), COLOR_IDX(904)};
	guint active_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(903), COLOR_IDX(901), COLOR_IDX(902), COLOR_IDX(904)};

	chk_inact_img[0] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_NI_164", ((TAB_PAGE_SIZE_W * 1) / 5) - 10, IMG_SUBTAB_BTN1_N_L, IMG_SUBTAB_BTN1_N_M, IMG_SUBTAB_BTN1_N_R);
	chk_inact_img[1] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_FI_164", ((TAB_PAGE_SIZE_W * 1) / 5) - 10, IMG_SUBTAB_BTN1_O_L, IMG_SUBTAB_BTN1_O_M, IMG_SUBTAB_BTN1_O_R);
	chk_inact_img[2] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_PI_164", ((TAB_PAGE_SIZE_W * 1) / 5) - 10, IMG_SUBTAB_BTN1_S_L, IMG_SUBTAB_BTN1_S_M, IMG_SUBTAB_BTN1_S_R);
	chk_inact_img[3] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_DI_164", ((TAB_PAGE_SIZE_W * 1) / 5) - 10, IMG_SUBTAB_BTN1_D_L, IMG_SUBTAB_BTN1_D_M, IMG_SUBTAB_BTN1_D_R);

	chk_act_img[0] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_SI_164", ((TAB_PAGE_SIZE_W * 1) / 5) - 10, IMG_SUBTAB_BTN1_S_L, IMG_SUBTAB_BTN1_S_M, IMG_SUBTAB_BTN1_S_R);
	chk_act_img[1] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_FI_164", ((TAB_PAGE_SIZE_W * 1) / 5) - 10, IMG_SUBTAB_BTN1_O_L, IMG_SUBTAB_BTN1_O_M, IMG_SUBTAB_BTN1_O_R);
	chk_act_img[2] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_PI_164", ((TAB_PAGE_SIZE_W * 1) / 5) - 10, IMG_SUBTAB_BTN1_P_L, IMG_SUBTAB_BTN1_P_M, IMG_SUBTAB_BTN1_P_R);
	chk_act_img[3] = nf_ui_create_image_button_method("MKB_IMG_SUBTAB_CHK1_DI_164", ((TAB_PAGE_SIZE_W * 1) / 5) - 10, IMG_SUBTAB_BTN1_D_L, IMG_SUBTAB_BTN1_D_M, IMG_SUBTAB_BTN1_D_R);

	btn = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_image((NFBUTTON*)btn, chk_inact_img);
    nfui_nfbutton_set_text((NFBUTTON*)btn, str);
    nfui_nfbutton_set_drawing_outline((NFBUTTON*)btn, FALSE);
    nfui_nfbutton_set_pango_font((NFBUTTON*)btn, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), inactive_font_color);
	nfui_nfobject_set_size(btn, ((TAB_PAGE_SIZE_W * 1) / 5) - 10, 40);

	if(expose) nfui_nfobject_show(btn);

	return btn;

}

static NFOBJECT *_make_image_button(gint expose)
{
    NFOBJECT *btn;

    return btn;
}

static int _make_mac_test_result_msg(int ret, char *msg, int error_cnt)
{
	char *str_msg[] =
	{
		"Please check the connection status of the network cable.",
		"Please connect the USB.",
		"There is no data folder. Please check the USB folder name.",
		"There is no data. Please check the USB data.",
		"It is duplicate data. The data will be deleted.",
		" It is invalid data. The data will be deleted.",
		"Please try again.",
		"It is a defective board. Please replace the board.",
	};

	if( ret == NF_SYSMAN_QC_PD_ERROR_NETWORK )
		g_sprintf(msg, "%s", str_msg[0]);
	else if( ret == NF_SYSMAN_QC_PD_ERROR_NO_USB )
		g_sprintf(msg, "%s", str_msg[1]);
	else if( ret == NF_SYSMAN_QC_PD_ERROR_NO_DATADIR )
		g_sprintf(msg, "%s", str_msg[2]);
	else if( ret == NF_SYSMAN_QC_PD_ERROR_NO_DATA )
		g_sprintf(msg, "%s", str_msg[3]);
	else if( ret == NF_SYSMAN_QC_PD_ERROR_DUPLICATION )
		g_sprintf(msg, "%s", str_msg[4]);
	else if( ret == NF_SYSMAN_QC_PD_ERROR_WRONG_DATA )
		g_sprintf(msg, "%s", str_msg[5]);
	else if( ret == NF_SYSMAN_QC_PD_ERROR_ETC )
	{
		if( error_cnt < 3 )
			g_sprintf(msg, "%s", str_msg[6]);
		else
			g_sprintf(msg, "%s", str_msg[7]);
	}

	return 0;
}

static gboolean _proc_fackey_test(gpointer data)
{
    static gint retry_cnt = 0;

    if (nf_sysman_qc_manual_test_factory_key())
    {
        _remove_waitbox();
        nftool_mbox_auto(g_curwnd, 1, "NOTICE", "FACTORY KEY TEST PASS!!!");
        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_qc_new[TAB_MANUAL][MANUAL_FACKEY].status, TRUE);
        nfui_signal_emit(g_qc_new[TAB_MANUAL][MANUAL_FACKEY].status, GDK_EXPOSE, TRUE);
        g_qc_new[TAB_MANUAL][MANUAL_FACKEY].result = RESULT_PASS;
        retry_cnt = 0;

        return FALSE;
    }
    else
    {
        retry_cnt++;
    }

    if (retry_cnt == 60)    // 60 is 6sec
    {
        _remove_waitbox();
        nftool_mbox_auto(g_curwnd, 1, "NOTICE", "FACTORY KEY TEST FAIL!!!");
        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_qc_new[TAB_MANUAL][MANUAL_FACKEY].status, FALSE);
        nfui_signal_emit(g_qc_new[TAB_MANUAL][MANUAL_FACKEY].status, GDK_EXPOSE, TRUE);
        g_qc_new[TAB_MANUAL][MANUAL_FACKEY].result = RESULT_FAIL;
        retry_cnt = 0;

        return FALSE;
    }

    return TRUE;
}

static void _refresh_hw_info()
{
    gchar buf[128];

    memset(buf, 0x00, sizeof(buf));
    _get_hw_version(buf);
    nfui_nflabel_set_text((NFLABEL*)g_qc_new[TAB_SYSINFO][SYSINFO_HWVER].status, buf);
    nfui_signal_emit(g_qc_new[TAB_SYSINFO][SYSINFO_HWVER].status, GDK_EXPOSE, TRUE);

    memset(buf, 0x00, sizeof(buf));
    _get_panel_type(buf);
    nfui_nflabel_set_text((NFLABEL*)g_qc_new[TAB_SYSINFO][SYSINFO_PANELTYPE].status, buf);
    nfui_signal_emit(g_qc_new[TAB_SYSINFO][SYSINFO_PANELTYPE].status, GDK_EXPOSE, TRUE);

    memset(buf, 0x00, sizeof(buf));
    _get_rc_type(buf);
    nfui_nflabel_set_text((NFLABEL*)g_qc_new[TAB_SYSINFO][SYSINFO_RCTYPE].status, buf);
    nfui_signal_emit(g_qc_new[TAB_SYSINFO][SYSINFO_RCTYPE].status, GDK_EXPOSE, TRUE);

    memset(buf, 0x00, sizeof(buf));
    _get_new_mac_addr(buf);
    nfui_nflabel_set_text((NFLABEL*)g_qc_new[TAB_SYSINFO][SYSINFO_MACADDR].status, buf);
    nfui_signal_emit(g_qc_new[TAB_SYSINFO][SYSINFO_MACADDR].status, GDK_EXPOSE, TRUE);
}

static gboolean _proc_mac_test(gpointer data)
{
	gint ret;
	int mode;
	char msg[256];


	mode = get_qc_mac_writer_mode();
	if(mode == 1)						//ntp
		ret = nf_sysman_qc_client();
	else if(mode == 2)					//pcp
		ret = nf_sysman_qc_pc_client();
	_remove_waitbox();

	if( ret == NF_SYSMAN_QC_PD_ERROR_OK )
	{
		g_mac_error = 0;

		nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_qc_new[TAB_MANUAL][MANUAL_MAC].status, TRUE);
        nfui_signal_emit(g_qc_new[TAB_MANUAL][MANUAL_MAC].status, GDK_EXPOSE, TRUE);

        _refresh_hw_info();

		g_qc_new[TAB_MANUAL][MANUAL_MAC].result = RESULT_PASS;
	}
	else
	{
		if(nfui_check_button_get_active((NFCHECKBUTTON*)g_qc_new[TAB_MANUAL][MANUAL_MAC].status))
		{
			nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_qc_new[TAB_MANUAL][MANUAL_MAC].status, FALSE);
	        nfui_signal_emit(g_qc_new[TAB_MANUAL][MANUAL_MAC].status, GDK_EXPOSE, TRUE);
		}

		if( ret == NF_SYSMAN_QC_PD_ERROR_ETC )
			g_mac_error ++;
		else
			g_mac_error = 0;

		if( g_mac_error > 2 )
			g_mac_error = 0;

		memset(msg, 0x00, sizeof(msg));
		_make_mac_test_result_msg(ret, msg, g_mac_error);
		nftool_mbox_auto(g_curwnd, 2, "ERROR", msg);

		g_qc_new[TAB_MANUAL][MANUAL_MAC].result = RESULT_FAIL;
	}

    return FALSE;
}

static gint _get_expose(gint tab, gint item)
{
    gint expose = 1;

#if QC_CODE_TEST_MODE
    return 1;
#endif

    if(tab == TAB_SYSINFO)
    {
        switch(item)
        {
            case SYSINFO_MODEL:
                expose = 0;
            break;
        }
    }
    else if(tab == TAB_AUTO)
    {
        switch(item)
        {
            case AUTO_RS232:
                expose = _get_expose_rs232();
                break;

            case AUTO_HDD:
                expose = _get_expose_hdd();
                break;

            case AUTO_ODD:
                expose = _get_expose_odd();
                break;

            case AUTO_ALARM:
                expose = _get_expose_alarm();
                break;

            case AUTO_RS485:
                expose = _get_expose_rs485(tab);
                break;

            case AUTO_FAN:
                expose = _get_expose_fan(tab);
                break;

            case AUTO_TEMPER:
                expose = _get_expose_temperature();
                break;

			case AUTO_AUDIO:
				expose = 0;
				break;
        }
    }
    else
    {
        switch(item)
        {
            case MANUAL_JOG:
            case MANUAL_SHUTTLE:
            case MANUAL_HDD:
            case MANUAL_ODD:
            case MANUAL_ANALOGVIEW_AHD:
            case MANUAL_ANALOGVIEW_TVI:
                expose = 0;
                break;

            case MANUAL_RS485:
                expose = _get_expose_rs485(tab);
                break;

            case MANUAL_FAN:
                expose = _get_expose_fan(tab);
                break;

            case MANUAL_KEYPAD:
                expose = _get_expose_keypad();
                break;

			case MANUAL_REMOCON:
				expose = _get_expose_remocon();
				break;

            case MANUAL_SPOT:
                expose = _get_expose_spot();
                break;

			case MANUAL_FACKEY:
				expose = _get_expose_fackey();
				break;

			 case MANUAL_AUDIO:
			 	expose = _get_expose_audio();
				break;

			case MANUAL_LICENSE:
                expose = _get_expose_license();
                break;
            
            case MANUAL_MAC:
                expose = _get_expose_mac();
                break;
        }
    }

    return expose;
}

static gboolean _post_alarm_test_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, tab;

    i = _get_obj_tab_and_index(obj, &tab);

    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        if (scm_appqc_alarm_test(INFY_APPQC_AUTO_TEST_ALARM) == -1) return FALSE;

        g_qc_new[tab][i].result = 0;
        nfui_nflabel_set_text(g_qc_new[tab][i].status, "Verifying.");
        nfui_signal_emit(g_qc_new[tab][i].status, GDK_EXPOSE, FALSE);
    }
    else if(evt->type == INFY_APPQC_AUTO_TEST_ALARM)
    {
        CMM_MESSAGE_T *res = (CMM_MESSAGE_T*)data;

        nfui_nflabel_set_text(g_qc_new[tab][i].status, "");
        g_qc_new[tab][i].result = res->param;
        nfui_signal_emit(g_qc_new[tab][i].status, GDK_EXPOSE, FALSE);
    }
    else if(evt->type == GDK_DELETE)
    {
        uxm_unreg_imsg_event(obj, INFY_APPQC_AUTO_TEST_ALARM);
    }

    return FALSE;
}

static gboolean _post_network_test_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, tab;

    i = _get_obj_tab_and_index(obj, &tab);

    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        if (scm_appqc_network_test(INFY_APPQC_AUTO_TEST_NETWORK) == -1) return FALSE;

        g_qc_new[tab][i].result = 0;
        nfui_nflabel_set_text(g_qc_new[tab][i].status, "Verifying.");
        nfui_signal_emit(g_qc_new[tab][i].status, GDK_EXPOSE, FALSE);
    }
    else if(evt->type == INFY_APPQC_AUTO_TEST_NETWORK)
    {
        CMM_MESSAGE_T *res = (CMM_MESSAGE_T*)data;

        nfui_nflabel_set_text(g_qc_new[tab][i].status, "");
        g_qc_new[tab][i].result = res->param;
        nfui_signal_emit(g_qc_new[tab][i].status, GDK_EXPOSE, FALSE);
    }
    else if(evt->type == GDK_DELETE)
    {
        uxm_unreg_imsg_event(obj, INFY_APPQC_AUTO_TEST_NETWORK);
    }

    return FALSE;
}

static gboolean _post_audio_test_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, tab;

    i = _get_obj_tab_and_index(obj, &tab);

    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        if (scm_appqc_audio_test(INFY_APPQC_AUTO_TEST_AUDIO) == -1) return FALSE;

        g_qc_new[tab][i].result = 0;
        nfui_nflabel_set_text(g_qc_new[tab][i].status, "Verifying.");
        nfui_signal_emit(g_qc_new[tab][i].status, GDK_EXPOSE, FALSE);
    }
    else if(evt->type == INFY_APPQC_AUTO_TEST_AUDIO)
    {
        CMM_MESSAGE_T *res = (CMM_MESSAGE_T*)data;

        nfui_nflabel_set_text(g_qc_new[tab][i].status, "");
        g_qc_new[tab][i].result = res->param;
        nfui_signal_emit(g_qc_new[tab][i].status, GDK_EXPOSE, FALSE);
    }
    else if(evt->type == GDK_DELETE)
    {
        uxm_unreg_imsg_event(obj, INFY_APPQC_AUTO_TEST_AUDIO);
    }

    return FALSE;
}

static gboolean _post_rs485_test_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, tab;

    i = _get_obj_tab_and_index(obj, &tab);

	if(evt->type == GDK_BUTTON_RELEASE)
	{
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        if (scm_appqc_rs485_test(INFY_APPQC_AUTO_TEST_RS485) == -1) return FALSE;

        g_qc_new[tab][i].result = 0;
        nfui_nflabel_set_text(g_qc_new[tab][i].status, "Verifying.");
        nfui_signal_emit(g_qc_new[tab][i].status, GDK_EXPOSE, FALSE);
	}
	else if (evt->type == INFY_APPQC_AUTO_TEST_RS485)
	{
        CMM_MESSAGE_T *res = (CMM_MESSAGE_T*)data;

        nfui_nflabel_set_text(g_qc_new[tab][i].status, "");
        g_qc_new[tab][i].result = res->param;
        nfui_signal_emit(g_qc_new[tab][i].status, GDK_EXPOSE, FALSE);
	}
	else if (evt->type == GDK_DELETE)
	{
	    uxm_unreg_imsg_event(obj, INFY_APPQC_AUTO_TEST_RS485);
	}

    return FALSE;
}

static gboolean _post_fan_test_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, tab;

    i = _get_obj_tab_and_index(obj, &tab);

	if(evt->type == GDK_BUTTON_RELEASE)
	{
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        if (scm_appqc_fan_test(INFY_APPQC_AUTO_TEST_FAN) == -1) return FALSE;

        g_qc_new[tab][i].result = 0;
        nfui_nflabel_set_text(g_qc_new[tab][i].status, "Verifying.");
        nfui_signal_emit(g_qc_new[tab][i].status, GDK_EXPOSE, FALSE);
	}
	else if (evt->type == INFY_APPQC_AUTO_TEST_FAN)
	{
        CMM_MESSAGE_T *res = (CMM_MESSAGE_T*)data;

        nfui_nflabel_set_text(g_qc_new[tab][i].status, "");
        g_qc_new[tab][i].result = res->param;
        nfui_signal_emit(g_qc_new[tab][i].status, GDK_EXPOSE, FALSE);
	}
	else if (evt->type == GDK_DELETE)
	{
	    uxm_unreg_imsg_event(obj, INFY_APPQC_AUTO_TEST_FAN);
	}

    return FALSE;
}

static gboolean _post_temper_test_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, tab;

    i = _get_obj_tab_and_index(obj, &tab);

	if(evt->type == GDK_BUTTON_RELEASE)
	{
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        if (scm_appqc_temper_test(INFY_APPQC_AUTO_TEST_TEMPER) == -1) return FALSE;

        g_qc_new[tab][i].result = 0;
        nfui_nflabel_set_text(g_qc_new[tab][i].status, "Verifying.");
        nfui_signal_emit(g_qc_new[tab][i].status, GDK_EXPOSE, FALSE);
	}
	else if (evt->type == INFY_APPQC_AUTO_TEST_TEMPER)
	{
        CMM_MESSAGE_T *res = (CMM_MESSAGE_T*)data;

        nfui_nflabel_set_text(g_qc_new[tab][i].status, "");
        g_qc_new[tab][i].result = res->param;
        nfui_signal_emit(g_qc_new[tab][i].status, GDK_EXPOSE, FALSE);
	}
	else if (evt->type == GDK_DELETE)
	{
	    uxm_unreg_imsg_event(obj, INFY_APPQC_AUTO_TEST_TEMPER);
	}

    return FALSE;
}

static gboolean _post_rs232_test_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, tab;

    i = _get_obj_tab_and_index(obj, &tab);

	if(evt->type == GDK_BUTTON_RELEASE)
	{
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        if (scm_appqc_rs232_test(INFY_APPQC_AUTO_TEST_RS232) == -1) return FALSE;

        g_qc_new[tab][i].result = 0;
        nfui_nflabel_set_text(g_qc_new[tab][i].status, "Verifying.");
        nfui_signal_emit(g_qc_new[tab][i].status, GDK_EXPOSE, FALSE);
	}
	else if (evt->type == INFY_APPQC_AUTO_TEST_RS232)
	{
        CMM_MESSAGE_T *res = (CMM_MESSAGE_T*)data;

        nfui_nflabel_set_text(g_qc_new[tab][i].status, "");
        g_qc_new[tab][i].result = res->param;
        nfui_signal_emit(g_qc_new[tab][i].status, GDK_EXPOSE, FALSE);
	}
	else if (evt->type == GDK_DELETE)
	{
	    uxm_unreg_imsg_event(obj, INFY_APPQC_AUTO_TEST_RS232);
	}

    return FALSE;
}

static gboolean _post_poe_test_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, tab;

    i = _get_obj_tab_and_index(obj, &tab);

	if(evt->type == GDK_BUTTON_RELEASE)
	{
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        if (scm_appqc_poe_test(INFY_APPQC_AUTO_TEST_POE) == -1) return FALSE;

        g_qc_new[tab][i].result = 0;
        nfui_nflabel_set_text(g_qc_new[tab][i].status, "Verifying.");
        nfui_signal_emit(g_qc_new[tab][i].status, GDK_EXPOSE, FALSE);
	}
	else if (evt->type == INFY_APPQC_AUTO_TEST_POE)
	{
        CMM_MESSAGE_T *res = (CMM_MESSAGE_T*)data;

        nfui_nflabel_set_text(g_qc_new[tab][i].status, "");
        g_qc_new[tab][i].result = res->param;
        nfui_signal_emit(g_qc_new[tab][i].status, GDK_EXPOSE, FALSE);
	}
	else if (evt->type == GDK_DELETE)
	{
	    uxm_unreg_imsg_event(obj, INFY_APPQC_AUTO_TEST_POE);
	}

    return FALSE;
}

static gboolean _post_buzzer_test_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
		gboolean act = FALSE;

		act = nfui_check_button_get_active(NF_CHECKBUTTON(obj));
		nf_action_buzzer_test(act);
	}

	return FALSE;
}

static gboolean _post_led_test_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
		gboolean act = FALSE;

		act = nfui_check_button_get_active(NF_CHECKBUTTON(obj));

		if(act) scm_turn_on_led_all_qc();
		else    scm_turn_off_led_all_qc();
	}

    return FALSE;
}

static gboolean _post_manual_rs485_test_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
		gboolean act = FALSE;

		act = nfui_check_button_get_active(NF_CHECKBUTTON(obj));

		if(act) nf_sysman_qc_manual_test_rs485();//ksi_test
		else    nf_sysman_qc_manual_test_rs485();//ksi_test
	}

    return FALSE;
}

static gboolean _post_keypad_test_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gboolean pass = 1;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
	    if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

	    //pass = VW_QC_Test_Keypad_Popup(g_curwnd, 0);

	    nfui_check_button_set_active(g_qc_new[TAB_MANUAL][MANUAL_KEYPAD].status, pass);
	    nfui_signal_emit(g_qc_new[TAB_MANUAL][MANUAL_KEYPAD].status, NFEVENT_CHECKBUTTON_CHANGED, FALSE);
	}

    return FALSE;
}

static gboolean _post_audio_manual_test_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gboolean pass;
    gint x, y;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
	    if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

    	nfui_nfobject_get_window_pos(obj, &x, &y);

    	x += ((GdkEventButton*)evt)->x;
    	y += ((GdkEventButton*)evt)->y;


	    pass = VW_QC_Test_Audio_Popup(g_curwnd, x, y);

	    nfui_check_button_set_active(g_qc_new[TAB_MANUAL][MANUAL_AUDIO].status, pass);
	    nfui_signal_emit(g_qc_new[TAB_MANUAL][MANUAL_AUDIO].status, NFEVENT_CHECKBUTTON_CHANGED, FALSE);
	}

    return FALSE;
}

static gboolean _post_analogview_test_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{

    return FALSE;
}

static gboolean _post_remocon_test_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    guint x, y;
    gboolean pass;

    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

    	nfui_nfobject_get_window_pos(obj, &x, &y);

    	x += ((GdkEventButton*)evt)->x;
    	y += ((GdkEventButton*)evt)->y;

        pass = _vw_qc_remocon_test_popup(g_curwnd, x, y);

	    nfui_check_button_set_active(g_qc_new[TAB_MANUAL][MANUAL_REMOCON].status, pass);
	    nfui_signal_emit(g_qc_new[TAB_MANUAL][MANUAL_REMOCON].status, NFEVENT_CHECKBUTTON_CHANGED, FALSE);
    }

    return FALSE;
}
static gboolean _post_remocon_wnd_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_REMOCON_PRESS)
    {
        g_rmc_res = TRUE;
    }
    else if (evt->type == GDK_DELETE)
    {
        gtk_main_quit();
    }

    return FALSE;
}

static gboolean _post_remocon_test_ok_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *top;

    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        top = nfui_nfobject_get_top(obj);
        nfui_nfobject_destroy(top);
    }

    return FALSE;
}

static gboolean _post_fackey_test_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
	    if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        wait_pop = nftool_mbox_wait(g_curwnd, "WAIT", "Press Factory Key...");
        g_timeout_add(100, _proc_fackey_test, NULL);
	}

    return FALSE;
}

static gboolean _post_mac_test_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
	    if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		_create_waitbox();
        g_timeout_add(100, _proc_mac_test, NULL);
	}

    return FALSE;
}

static gboolean _post_local_time_sync_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, tab, ret;

    if(evt->type == GDK_BUTTON_RELEASE)
    {
    	NFOBJECT *wbox = NULL;

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		_create_waitbox();

        i = _get_obj_tab_and_index(obj, &tab);

        _stop_timer();
        ret = scm_appqc_rtc_sync(INFY_APPQC_RTC_SYNC);

        if (ret == -1) 
        {
            _remove_waitbox();
            _restart_timer();
            return FALSE;
        }
    }
    else if(evt->type == INFY_APPQC_RTC_SYNC)
    {
		_remove_waitbox();
        _restart_timer();
    }
    else if (evt->type == GDK_DELETE)
    {
        uxm_unreg_imsg_event(obj, INFY_APPQC_RTC_SYNC);
    }

    return FALSE;
}

static gboolean _post_rtc_sync_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{

    if(evt->type == GDK_BUTTON_RELEASE)
    {
        gint i, tab;
        gchar buf[64];

        i = _get_obj_tab_and_index(obj, &tab);

        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        memset(buf, 0x00, sizeof(buf));

        _get_rtc_time(buf);
        nfui_nflabel_set_text(g_qc_new[tab][i].status, buf);
        nfui_signal_emit(g_qc_new[tab][i].status, GDK_EXPOSE, FALSE);
    }
    return FALSE;
}

static gboolean _post_dhcp_renew_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, tab, ret;
    gchar buf[64];

    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		_create_waitbox();

        ret = scm_appqc_dhcp_renew(INFY_APPQC_DHCP_RENEW);

        if (ret == -1) _remove_waitbox();
    }
    else if (evt->type == GDK_DELETE)
    {
        uxm_unreg_imsg_event(obj, INFY_APPQC_DHCP_RENEW);
    }
    else if (evt->type == INFY_APPQC_DHCP_RENEW)
    {
        i = _get_obj_tab_and_index(obj, &tab);

        _remove_waitbox();
        
        memset(buf, 0x00, sizeof(buf));
        _get_ip_addr(buf);
        g_message("%s, %d, GET IP : %s", __FUNCTION__, __LINE__, buf);
        nfui_nflabel_set_text(g_qc_new[tab][i].status, buf);
        nfui_signal_emit(g_qc_new[tab][i].status, GDK_EXPOSE, FALSE);
    }

    return FALSE;
}

static gboolean _post_status_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, tab;

    i = _get_obj_tab_and_index(obj, &tab);

    if(evt->type == GDK_EXPOSE)
    {
        if(i == -1){
            return FALSE;
        }
        else{
            if(tab == TAB_AUTO){
                if(g_qc_new[tab][i].result){
                    _draw_image(obj, g_qc_new[tab][i].result);
                }
            }
        }
    }
    else if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        if(i == -1){
            return FALSE;
        }
        else{
            if(nfui_check_button_get_active((NFCHECKBUTTON*)obj)){
                g_qc_new[tab][i].result = RESULT_PASS;
            }
            else{
                g_qc_new[tab][i].result = RESULT_FAIL;
            }
        }
        if(_check_result()){
            nfui_nfobject_enable(g_saveBtn);
            nfui_signal_emit(g_saveBtn, GDK_EXPOSE, TRUE);
        }
        else{
            nfui_nfobject_disable(g_saveBtn);
            nfui_signal_emit(g_saveBtn, GDK_EXPOSE, TRUE);
        }
    }

    return FALSE;
}

static gboolean _post_okbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    switch(evt->type)
    {
        case GDK_BUTTON_RELEASE:
        {
            if(evt->button.button == MOUSE_RIGTH_BUTTON)
                return FALSE;

            vw_qc_test_hide_new();
        }
        break;
    }

    return FALSE;
}

static gboolean _post_format_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	static NFOBJECT* prog_pop = NULL;

	switch(evt->type) {
		case GDK_BUTTON_RELEASE:
			{
				mb_type ret;

				if(evt->button.button == MOUSE_RIGTH_BUTTON) {
					return FALSE;
				}

				ret = nftool_mbox(g_curwnd, "CONFIRM", "All recorded data will be removed.\nDo you want to continue?", NFTOOL_MB_OKCANCEL);
				if(ret == NFTOOL_MB_CANCEL)
					return FALSE;

				if(!prog_pop)
					prog_pop = nftool_prog_pop_open(g_curwnd, "FORMAT", TRUE, (guint)PROG_POS_X, (guint)PROG_POS_Y, 0);

				smt_set_service(SMT_FORMAT);
				scm_format_storage(IRET_SCM_FORMAT_STORAGE);
			}
			break;

		case INFY_FORMAT_RATE:
			{
				guint rate = ((CMM_MESSAGE_T *)data)->param;

				nftool_prog_pop_set_rate(prog_pop, rate);
			}
			break;

		case INFY_FORMAT_CMPL:
			{
				g_message("FORMAT completed...but filesystem is not started yet\n");
			}
			break;

		case IRET_SCM_FORMAT_STORAGE:
			if(prog_pop) {
				nftool_prog_pop_set_rate(prog_pop, 100);

				nftool_prog_pop_close(prog_pop);
				prog_pop = NULL;
			}

			smt_return_to_previous();
			break;

		case GDK_DELETE:
			uxm_unreg_imsg_event(obj, INFY_FORMAT_RATE);
			uxm_unreg_imsg_event(obj, INFY_FORMAT_CMPL);
			uxm_unreg_imsg_event(obj, IRET_SCM_FORMAT_STORAGE);
			break;

		default:
			break;
	}

	return FALSE;
}

static gboolean _post_facbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    switch(evt->type) {
        case GDK_BUTTON_RELEASE:
            {
                NFOBJECT *top = NULL;
                mb_type ret;

                if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

                ret = nftool_mbox(g_curwnd, "WARNING", "All configurations are initialized.\nDo you want to continue?", NFTOOL_MB_OKCANCEL);

                if(ret == NFTOOL_MB_OK)
                {
                    if (scm_appqc_factory_default(INFY_APPQC_FACTORY_DEFAULT) == 0)
                    {
                        smt_set_service(SMT_FAC_DEFAULT);
                        _create_waitbox();
                    }
                }
            }
            break;

        case INFY_APPQC_FACTORY_DEFAULT:
            {
                _remove_waitbox();

                _set_license();

                g_fac_init_run = 1;
                if(_check_result()){
                    nfui_nfobject_enable(g_saveBtn);
                    nfui_signal_emit(g_saveBtn, GDK_EXPOSE, FALSE);
                }
                else{
                    nfui_nfobject_disable(g_saveBtn);
                    nfui_signal_emit(g_saveBtn, GDK_EXPOSE, FALSE);
                }
            }
            break;

        case GDK_DELETE:
            uxm_unreg_imsg_event(obj, INFY_APPQC_FACTORY_DEFAULT);
            break;

        default:
            break;
    }

    return FALSE;
}

static gboolean _post_save_finish_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{

    switch(evt->type)
    {
        case GDK_BUTTON_RELEASE:
        {
            if(evt->button.button == MOUSE_RIGTH_BUTTON)
                return FALSE;
            _save_result();
            _shutdown_system();

        }
        break;
    }

    return FALSE;
}

static gboolean _pre_page_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{

	if(evt->type == GDK_EXPOSE) {
		GdkDrawable *drawable;
		GdkColor line_color = UX_COLOR(392);
		GdkGC *line_gc;

		drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
		line_gc = nfui_nfobject_get_gc((NFOBJECT*)obj);
		gdk_gc_set_rgb_fg_color(line_gc, &line_color);

		gdk_gc_set_line_attributes(line_gc,
				1,
				GDK_LINE_SOLID,
				GDK_CAP_NOT_LAST,
				GDK_JOIN_BEVEL);

		gdk_draw_rectangle(drawable,
				line_gc,
				FALSE,
				obj->x, obj->y,
				obj->width, obj->height);

		nfui_nfobject_gc_unref(line_gc);
	}
	else if(evt->type == INFY_APPQC_AUTO_TEST_ALL)
	{
	    APPQC_RES_T *res = (APPQC_RES_T*)((CMM_MESSAGE_T*)data)->data;

	    g_qc_new[TAB_AUTO][AUTO_ALARM].result = res->alarm;
	    g_qc_new[TAB_AUTO][AUTO_AUDIO].result = res->audio;
	    g_qc_new[TAB_AUTO][AUTO_NETWORK].result = res->network;
	    g_qc_new[TAB_AUTO][AUTO_RS485].result = res->rs485;
	    g_qc_new[TAB_AUTO][AUTO_FAN].result = res->fan;
	    g_qc_new[TAB_AUTO][AUTO_TEMPER].result = res->temper;
	    g_qc_new[TAB_AUTO][AUTO_RS232].result = res->rs232;
	    g_qc_new[TAB_AUTO][AUTO_POE].result = res->poe;

	    nfui_nflabel_set_text(g_qc_new[TAB_AUTO][AUTO_ALARM].status, "");
	    nfui_nflabel_set_text(g_qc_new[TAB_AUTO][AUTO_AUDIO].status, "");
	    nfui_nflabel_set_text(g_qc_new[TAB_AUTO][AUTO_NETWORK].status, "");
	    nfui_nflabel_set_text(g_qc_new[TAB_AUTO][AUTO_RS485].status, "");
	    nfui_nflabel_set_text(g_qc_new[TAB_AUTO][AUTO_FAN].status, "");
	    nfui_nflabel_set_text(g_qc_new[TAB_AUTO][AUTO_TEMPER].status, "");
	    nfui_nflabel_set_text(g_qc_new[TAB_AUTO][AUTO_RS232].status, "");
	    nfui_nflabel_set_text(g_qc_new[TAB_AUTO][AUTO_POE].status, "");

	    _restart_timer();
	    nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
	}
	else if(evt->type == GDK_DELETE)
	{
	    uxm_unreg_imsg_event(obj, INFY_APPQC_AUTO_TEST_ALL);
	}

	return FALSE;
}

static gboolean _post_qc_test_window_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid;

	switch(evt->type)
	{
		case NFOUTEVT_BUTTON_PRESS:
		{
            vw_qc_test_hide_new();
		}
		break;

		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
		{
			kevt = (GdkEventKey*)evt;
			kpid = (KEYPAD_KID)kevt->keyval;

			if (kpid == KEYPAD_EXIT)
			{
                vw_qc_test_hide_new();
                return TRUE;
   			}
		}
		break;

		case IRET_SCM_SHUTDOWN:
        {
            _remove_waitbox();
            nftool_mbox_wait(g_curwnd, "NOTICE", "You can power off the system.");
        }
        break;

        case INFY_QC_TEST_SET_BUTTON:
        {
            CMM_MESSAGE_T *pmsg = (CMM_MESSAGE_T *)data;
            gint keyIdx;

            keyIdx = pmsg->param;

            if (!g_qc[keyIdx].key_obj) return FALSE;

            nfui_nfbutton_set_text((NFBUTTON*)g_qc[keyIdx].key_obj, g_qc[keyIdx].label);
            nfui_signal_emit(g_qc[keyIdx].key_obj, GDK_EXPOSE, TRUE);
        }
        break;

        case INFY_QC_TEST_INFO_ADD:
        {
            CMM_MESSAGE_T *pmsg = (CMM_MESSAGE_T *)data;
            gchar *pBuf;

            pBuf = (gchar*)pmsg->data;

            if (!g_msglist_obj) return FALSE;

            nfui_listbox_set_text((NFLISTBOX*)g_msglist_obj, &pBuf);
        }
        break;

        case INFY_QC_TEST_INFO_UPDATE:
		{
			if (!g_msglist_obj) return FALSE;

			nfui_signal_emit(g_msglist_obj, GDK_EXPOSE, TRUE);
        }
		break;

        case INFY_QC_TEST_INFO_CLEAR:
        {
            nfui_listbox_delete_all((NFLISTBOX*)g_msglist_obj);
            nfui_signal_emit(g_msglist_obj, GDK_EXPOSE, TRUE);
        }
        break;

		case GDK_DELETE:
		{
			g_curwnd = 0;
        }
		break;

		default:
		break;
	}

	return FALSE;
}

static gboolean _returnkey_proc(NFOBJECT *top, GdkEvent *event, gpointer data)
{
	return FALSE;
}

static gboolean _vw_qc_remocon_test_popup(NFWINDOW *parent, guint x, guint y)
{
    NFOBJECT *wnd;
    NFOBJECT *fixed;
    NFOBJECT *obj;
    gint ret = 0;

    g_rmc_res = FALSE;

    wnd = nftool_create_popup_window(parent, x - 400, y, 400, 200, "REMOCON TEST", FALSE);
    nfui_regi_post_event_callback(wnd, _post_remocon_wnd_event_cb);
    nfui_nfobject_show(wnd);

    fixed = ((NFWINDOW*)wnd)->child;

    obj = nfui_nflabel_new_with_pango_font("REMOCON TEST", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 190, LABEL_SIZE_H);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
    nfui_nfobject_use_focus(obj, FALSE);
    nfui_nfobject_support_multi_lang(obj, TRUE);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, 8, 70);

    obj = nftool_normal_button_create_type1("OK", 150);
    nfui_nfbutton_set_font_alignment((NFBUTTON*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, 125, 145);
    nfui_regi_post_event_callback(obj, _post_remocon_test_ok_event_cb);

    nfui_make_key_hierarchy((NFWINDOW*)wnd);

    nfui_set_key_focus(obj, TRUE);

    gtk_main();

    return g_rmc_res;
}

static gint _init_tabpage_sysinfo(NFOBJECT *parent)
{
    NFOBJECT *title, *status;
    NFOBJECT *btn;
    gchar buf[128];
    gint expose = 0;
    guint i, pos_y;
    gint size_w, size_h;
    gchar *strTitle[SYSINFO_CNT] = {"MODEL", "FW VERSION", "HW VERSION", "AC POWER FREQUENCY", "DISK CAPACITY", "DISK USAGE",
                                    "NUMBER OF DISKS","IP ADDRESS", "MAC ADDRESS", "RESOLUTION", "PANEL TYPE", "RC TYPE", "LOCAL TIME", "RTC"};


    pos_y = LABEL_START_POS_Y;
    for(i = 0; i < SYSINFO_CNT; i++)
    {
        memset(buf, 0x00, sizeof(buf));
        g_qc_new[TAB_SYSINFO][i].expose = _get_expose(TAB_SYSINFO, i);

        switch(i)
        {
            case SYSINFO_MODEL:
                _get_model_name(buf);
                break;

            case SYSINFO_FWVER:
                _get_fw_version(buf);
                break;

            case SYSINFO_HWVER:
                _get_hw_version(buf);
                break;

            case SYSINFO_ACPOWER:
                _get_ac_power_frequency(buf);
                break;

            case SYSINFO_DISKCAP:
                _get_disk_capacity(buf);
                break;

            case SYSINFO_DISKUSAGE:
                _get_disk_usage(buf);
                break;

            case SYSINFO_DISKNUM:
                _get_disk_num(buf);
                break;

            case SYSINFO_IPADDR:
                _get_ip_addr(buf);
                g_qc_new[TAB_SYSINFO][i].btn = _make_button("REFRESH", g_qc_new[TAB_SYSINFO][i].expose);
                nfui_regi_post_event_callback(g_qc_new[TAB_SYSINFO][i].btn, _post_dhcp_renew_event_cb);
                uxm_reg_imsg_event(g_qc_new[TAB_SYSINFO][i].btn, INFY_APPQC_DHCP_RENEW);
                uxm_monitor_on_imsg_event(g_qc_new[TAB_SYSINFO][i].btn, INFY_APPQC_DHCP_RENEW);
                break;

            case SYSINFO_MACADDR:
                _get_mac_addr(buf);
                break;

            case SYSINFO_RESOLUTION:
                _get_resolution(buf);
                break;

            case SYSINFO_PANELTYPE:
                _get_panel_type(buf);
                break;

            case SYSINFO_RCTYPE:
                _get_rc_type(buf);
                break;

            case SYSINFO_LOCALTIME:
                g_time_update = 0;
                g_qc_new[TAB_SYSINFO][i].status = _make_label(buf, LABEL_TIME, g_qc_new[TAB_SYSINFO][i].expose);
                g_time_update = g_timeout_add(300, _timeout_date_time_update, g_qc_new[TAB_SYSINFO][i].status);
                g_qc_new[TAB_SYSINFO][i].btn = _make_button("SYNC.", g_qc_new[TAB_SYSINFO][i].expose);
                nfui_regi_post_event_callback(g_qc_new[TAB_SYSINFO][i].btn, _post_local_time_sync_event_cb);
                uxm_reg_imsg_event(g_qc_new[TAB_SYSINFO][i].btn, INFY_APPQC_RTC_SYNC);
                uxm_monitor_on_imsg_event(g_qc_new[TAB_SYSINFO][i].btn, INFY_APPQC_RTC_SYNC);

                break;

            case SYSINFO_RTC:
                _get_rtc_time(buf);
                g_qc_new[TAB_SYSINFO][i].btn = _make_button("REFRESH", g_qc_new[TAB_SYSINFO][i].expose);
                nfui_regi_post_event_callback(g_qc_new[TAB_SYSINFO][i].btn, _post_rtc_sync_event_cb);
                break;

            default:
                break;
        }
        g_qc_new[TAB_SYSINFO][i].title = _make_label(strTitle[i], LABEL_TITLE, g_qc_new[TAB_SYSINFO][i].expose);
        nfui_nfobject_get_size(g_qc_new[TAB_SYSINFO][i].title, &size_w, &size_h);
        if(i != SYSINFO_LOCALTIME){
            g_qc_new[TAB_SYSINFO][i].status = _make_label(buf, LABEL_STATUS, g_qc_new[TAB_SYSINFO][i].expose);
        }
        if(g_qc_new[TAB_SYSINFO][i].expose)
        {
            nfui_nffixed_put((NFFIXED*)parent, g_qc_new[TAB_SYSINFO][i].title, LABEL_TITLE_POS_X, pos_y);
            nfui_nffixed_put((NFFIXED*)parent, g_qc_new[TAB_SYSINFO][i].status, LABEL_TITLE_POS_X + size_w + LABEL_SPACE, pos_y);
            if(g_qc_new[TAB_SYSINFO][i].btn){
                nfui_nffixed_put((NFFIXED*)parent, g_qc_new[TAB_SYSINFO][i].btn, LABEL_TITLE_POS_X + (size_w*2) + (LABEL_SPACE*2), pos_y);
            }
            pos_y += size_h + LABEL_SPACE;
        }

    }
    return 0;
}

static gint _init_tabpage_auto(NFOBJECT *parent)
{
    NFOBJECT *title, *status;
    NFOBJECT *obj;
    gchar buf[128];
    gint expose = 0;
    guint i, pos_y;
    guint size_w, size_h;
    gchar *strTitle[AUTO_CNT] = {"ALARM", "NETWORK", "AUDIO", "RS485", "FAN", "TEMPERATURE", "RS232", "POE", "HDD", "ODD"};

    pos_y = LABEL_START_POS_Y;
    for(i = 0; i < AUTO_CNT; i++)
    {
        g_message("%s, %d, make %s, idx : %d", __FUNCTION__, __LINE__, strTitle[i], i);
        memset(buf, 0x00, sizeof(buf));
        g_qc_new[TAB_AUTO][i].expose = _get_expose(TAB_AUTO, i);
        switch(i)
        {
            case AUTO_ALARM:
                g_qc_new[TAB_AUTO][i].btn = _make_button("TEST", g_qc_new[TAB_AUTO][i].expose);
                nfui_regi_post_event_callback(g_qc_new[TAB_AUTO][i].btn, _post_alarm_test_event_cb);
                uxm_reg_imsg_event(g_qc_new[TAB_AUTO][i].btn, INFY_APPQC_AUTO_TEST_ALARM);
                uxm_monitor_on_imsg_event(g_qc_new[TAB_AUTO][i].btn, INFY_APPQC_AUTO_TEST_ALARM);
                g_sprintf(buf, "%s", "Verifying.");
                break;

            case AUTO_NETWORK:
                g_qc_new[TAB_AUTO][i].btn = _make_button("TEST", g_qc_new[TAB_AUTO][i].expose);
                nfui_regi_post_event_callback(g_qc_new[TAB_AUTO][i].btn, _post_network_test_event_cb);
                uxm_reg_imsg_event(g_qc_new[TAB_AUTO][i].btn, INFY_APPQC_AUTO_TEST_NETWORK);
                uxm_monitor_on_imsg_event(g_qc_new[TAB_AUTO][i].btn, INFY_APPQC_AUTO_TEST_NETWORK);
                g_sprintf(buf, "%s", "Verifying.");
                break;

            case AUTO_AUDIO:
                g_qc_new[TAB_AUTO][i].btn = _make_button("TEST", g_qc_new[TAB_AUTO][i].expose);
                nfui_regi_post_event_callback(g_qc_new[TAB_AUTO][i].btn, _post_audio_test_event_cb);
                uxm_reg_imsg_event(g_qc_new[TAB_AUTO][i].btn, INFY_APPQC_AUTO_TEST_AUDIO);
                uxm_monitor_on_imsg_event(g_qc_new[TAB_AUTO][i].btn, INFY_APPQC_AUTO_TEST_AUDIO);
                g_sprintf(buf, "%s", "Verifying.");
                break;

            case AUTO_RS485:
                g_qc_new[TAB_AUTO][i].btn = _make_button("TEST", g_qc_new[TAB_AUTO][i].expose);
                nfui_regi_post_event_callback(g_qc_new[TAB_AUTO][i].btn, _post_rs485_test_event_cb);
                uxm_reg_imsg_event(g_qc_new[TAB_AUTO][i].btn, INFY_APPQC_AUTO_TEST_RS485);
                uxm_monitor_on_imsg_event(g_qc_new[TAB_AUTO][i].btn, INFY_APPQC_AUTO_TEST_RS485);
                g_sprintf(buf, "%s", "Verifying.");
                break;

            case AUTO_FAN:
                g_qc_new[TAB_AUTO][i].btn = _make_button("TEST", g_qc_new[TAB_AUTO][i].expose);
                nfui_regi_post_event_callback(g_qc_new[TAB_AUTO][i].btn, _post_fan_test_event_cb);
                uxm_reg_imsg_event(g_qc_new[TAB_AUTO][i].btn, INFY_APPQC_AUTO_TEST_FAN);
                uxm_monitor_on_imsg_event(g_qc_new[TAB_AUTO][i].btn, INFY_APPQC_AUTO_TEST_FAN);
                g_sprintf(buf, "%s", "Verifying.");
                break;

            case AUTO_TEMPER:
                g_qc_new[TAB_AUTO][i].btn = _make_button("TEST", g_qc_new[TAB_AUTO][i].expose);
                nfui_regi_post_event_callback(g_qc_new[TAB_AUTO][i].btn, _post_temper_test_event_cb);
                uxm_reg_imsg_event(g_qc_new[TAB_AUTO][i].btn, INFY_APPQC_AUTO_TEST_TEMPER);
                uxm_monitor_on_imsg_event(g_qc_new[TAB_AUTO][i].btn, INFY_APPQC_AUTO_TEST_TEMPER);
                g_sprintf(buf, "%s", "Verifying.");
                break;

            case AUTO_HDD:
                g_qc_new[TAB_AUTO][i].result = _get_result_hdd_test();
                break;

            case AUTO_ODD:
                g_qc_new[TAB_AUTO][i].result = _get_result_odd_test();
                break;

            case AUTO_RS232:
                g_qc_new[TAB_AUTO][i].btn = _make_button("TEST", g_qc_new[TAB_AUTO][i].expose);
                nfui_regi_post_event_callback(g_qc_new[TAB_AUTO][i].btn, _post_rs232_test_event_cb);
                uxm_reg_imsg_event(g_qc_new[TAB_AUTO][i].btn, INFY_APPQC_AUTO_TEST_RS232);
                uxm_monitor_on_imsg_event(g_qc_new[TAB_AUTO][i].btn, INFY_APPQC_AUTO_TEST_RS232);
                g_sprintf(buf, "%s", "Verifying.");
                break;

            case AUTO_POE:
                g_qc_new[TAB_AUTO][i].btn = _make_button("TEST", g_qc_new[TAB_AUTO][i].expose);
                nfui_regi_post_event_callback(g_qc_new[TAB_AUTO][i].btn, _post_poe_test_event_cb);
                uxm_reg_imsg_event(g_qc_new[TAB_AUTO][i].btn, INFY_APPQC_AUTO_TEST_POE);
                uxm_monitor_on_imsg_event(g_qc_new[TAB_AUTO][i].btn, INFY_APPQC_AUTO_TEST_POE);
                g_sprintf(buf, "%s", "Verifying.");
                break;

            default:
                break;
        }
        g_qc_new[TAB_AUTO][i].title = _make_label(strTitle[i], LABEL_TITLE, g_qc_new[TAB_AUTO][i].expose);
        nfui_nfobject_get_size(g_qc_new[TAB_AUTO][i].title, &size_w, &size_h);
        g_qc_new[TAB_AUTO][i].status = _make_label(buf, LABEL_STATUS, g_qc_new[TAB_AUTO][i].expose);
        nfui_regi_post_event_callback(g_qc_new[TAB_AUTO][i].status, _post_status_event_cb);
        if(g_qc_new[TAB_AUTO][i].expose)
        {
            nfui_nffixed_put((NFFIXED*)parent, g_qc_new[TAB_AUTO][i].title, LABEL_TITLE_POS_X, pos_y);
            nfui_nffixed_put((NFFIXED*)parent, g_qc_new[TAB_AUTO][i].status, LABEL_TITLE_POS_X + size_w + LABEL_SPACE, pos_y);
            if(g_qc_new[TAB_AUTO][i].btn){
                nfui_nffixed_put((NFFIXED*)parent, g_qc_new[TAB_AUTO][i].btn, LABEL_TITLE_POS_X + (size_w*2) + (LABEL_SPACE*2), pos_y);
            }
            pos_y += size_h + LABEL_SPACE;
        }

    }

    return 0;
}

static gint _init_tabpage_manual(NFOBJECT *parent)
{
    NFOBJECT *fixed;
    NFOBJECT *title, *status;
    NFOBJECT *btn;
    GSList *slist;
    gchar buf[128];
    gint expose = 0;
    guint i, pos_y;
    gint size_w, size_h;
    gint wid, hei;
    gchar *strTitle[MANUAL_CNT] = { "RS485", "BUZZER", "LED", "KEYPAD", "REMOCON",
                                    "AUDIO", "FACTORY KEY", "MAC", "JOG", "SHUTTLE",
                                    "HDD", "ODD", "USB", "FAN",
#if defined (_IPX_32P5)
                                    "HDMI 1", "HDMI 2",
#else
                                    "HDMI", "VGA", 
#endif
                                    "VIDEO(TVI)", "VIDEO(AHD)","SPOT", "RTC", "LICENSE"};



    pos_y = LABEL_START_POS_Y;
    for(i = 0; i < MANUAL_CNT; i++)
    {
        g_message("%s, %d, make %s, idx : %d", __FUNCTION__, __LINE__, strTitle[i], i);
        g_qc_new[TAB_MANUAL][i].expose = _get_expose(TAB_MANUAL, i);
        g_qc_new[TAB_MANUAL][i].title = _make_label(strTitle[i], LABEL_TITLE, g_qc_new[TAB_MANUAL][i].expose);
        nfui_nfobject_get_size(g_qc_new[TAB_MANUAL][i].title, &size_w, &size_h);
        fixed = _make_fixed();
        g_qc_new[TAB_MANUAL][i].status = _make_check_button(g_qc_new[TAB_MANUAL][i].expose, fixed);
        nfui_regi_post_event_callback(g_qc_new[TAB_MANUAL][i].status, _post_status_event_cb);

        memset(buf, 0x00, sizeof(buf));
        switch(i)
        {
            case MANUAL_RS485:
                g_qc_new[TAB_MANUAL][i].btn = _make_manual_test_button("TEST", g_qc_new[TAB_MANUAL][i].expose);
            	nfui_regi_post_event_callback(g_qc_new[TAB_MANUAL][i].btn, _post_manual_rs485_test_event_cb);
                break;

            case MANUAL_BUZZER:
                g_qc_new[TAB_MANUAL][i].btn = _make_manual_test_button("TEST", g_qc_new[TAB_MANUAL][i].expose);
            	nfui_regi_post_event_callback(g_qc_new[TAB_MANUAL][i].btn, _post_buzzer_test_event_cb);
                break;

            case MANUAL_LED:
                g_qc_new[TAB_MANUAL][i].btn = _make_manual_test_button("TEST", g_qc_new[TAB_MANUAL][i].expose);
                nfui_regi_post_event_callback(g_qc_new[TAB_MANUAL][i].btn, _post_led_test_event_cb);
                break;

            case MANUAL_KEYPAD:
                nfui_nfobject_disable(g_qc_new[TAB_MANUAL][i].status);
                g_qc_new[TAB_MANUAL][i].btn = _make_button("TEST", g_qc_new[TAB_MANUAL][i].expose);
            	nfui_check_get_size((NFCHECKBUTTON*)g_qc_new[TAB_MANUAL][i].status, &wid, &hei);
            	nfui_nfobject_set_size(g_qc_new[TAB_MANUAL][i].status, wid, hei);
                nfui_regi_post_event_callback(g_qc_new[TAB_MANUAL][i].btn, _post_keypad_test_event_cb);
                break;

			case MANUAL_REMOCON:
                nfui_nfobject_disable(g_qc_new[TAB_MANUAL][i].status);
                g_qc_new[TAB_MANUAL][i].btn = _make_button("TEST", g_qc_new[TAB_MANUAL][i].expose);
            	nfui_check_get_size((NFCHECKBUTTON*)g_qc_new[TAB_MANUAL][i].status, &wid, &hei);
            	nfui_nfobject_set_size(g_qc_new[TAB_MANUAL][i].status, wid, hei);
                nfui_regi_post_event_callback(g_qc_new[TAB_MANUAL][i].btn, _post_remocon_test_event_cb);
                break;

			case MANUAL_AUDIO:
                nfui_nfobject_disable(g_qc_new[TAB_MANUAL][i].status);
                g_qc_new[TAB_MANUAL][i].btn = _make_button("TEST", g_qc_new[TAB_MANUAL][i].expose);
            	nfui_check_get_size((NFCHECKBUTTON*)g_qc_new[TAB_MANUAL][i].status, &wid, &hei);
            	nfui_nfobject_set_size(g_qc_new[TAB_MANUAL][i].status, wid, hei);
                nfui_regi_post_event_callback(g_qc_new[TAB_MANUAL][i].btn, _post_audio_manual_test_event_cb);
                break;

			case MANUAL_FACKEY:
				nfui_nfobject_disable(g_qc_new[TAB_MANUAL][i].status);
                g_qc_new[TAB_MANUAL][i].btn = _make_button("TEST", g_qc_new[TAB_MANUAL][i].expose);
            	nfui_check_get_size((NFCHECKBUTTON*)g_qc_new[TAB_MANUAL][i].status, &wid, &hei);
            	nfui_nfobject_set_size(g_qc_new[TAB_MANUAL][i].status, wid, hei);
                nfui_regi_post_event_callback(g_qc_new[TAB_MANUAL][i].btn, _post_fackey_test_event_cb);
                break;

            case MANUAL_JOG:
                break;

            case MANUAL_SHUTTLE:
                break;

            case MANUAL_HDD:
                g_sprintf(buf, "%s (%dea)", strTitle[i], scm_get_disk_count());
                nfui_nflabel_set_text((NFLABEL*)g_qc_new[TAB_MANUAL][i].title, buf);
                break;

            case MANUAL_ODD:
                g_sprintf(buf, "%s (%dea)", strTitle[i], _get_odd_cnt());
                nfui_nflabel_set_text((NFLABEL*)g_qc_new[TAB_MANUAL][i].title, buf);
                break;

            case MANUAL_USB:
                break;

            case MANUAL_FAN:
                break;

            case MANUAL_HDMI:
                break;

            case MANUAL_VGA:
                break;

            case MANUAL_ANALOGVIEW_TVI:
                g_qc_new[TAB_MANUAL][i].btn = _make_analogview_test_button("TEST", g_qc_new[TAB_MANUAL][i].expose);
                slist = nfui_radio_button_get_group((NFBUTTON*)g_qc_new[TAB_MANUAL][i].btn);
                nfui_radio_button_set_toggled((NFBUTTON*) g_qc_new[TAB_MANUAL][i].btn, TRUE);
                nfui_regi_post_event_callback(g_qc_new[TAB_MANUAL][i].btn, _post_analogview_test_event_cb);
                break;

            case MANUAL_ANALOGVIEW_AHD:
                g_qc_new[TAB_MANUAL][i].btn = _make_analogview_test_button("TEST", g_qc_new[TAB_MANUAL][i].expose);
                nfui_radio_button_add_group((NFBUTTON*)g_qc_new[TAB_MANUAL][i].btn, slist);
                nfui_regi_post_event_callback(g_qc_new[TAB_MANUAL][i].btn, _post_analogview_test_event_cb);
                break;

            case MANUAL_SPOT:
                break;

            case MANUAL_RTC:
                break;

			case MANUAL_MAC:
				nfui_nfobject_disable(g_qc_new[TAB_MANUAL][i].status);
                g_qc_new[TAB_MANUAL][i].btn = _make_button("TEST", g_qc_new[TAB_MANUAL][i].expose);
            	nfui_check_get_size((NFCHECKBUTTON*)g_qc_new[TAB_MANUAL][i].status, &wid, &hei);
            	nfui_nfobject_set_size(g_qc_new[TAB_MANUAL][i].status, wid, hei);
                nfui_regi_post_event_callback(g_qc_new[TAB_MANUAL][i].btn, _post_mac_test_event_cb);
                break;

            case MANUAL_LICENSE:
                nfui_nfobject_destroy(g_qc_new[TAB_MANUAL][i].status);
                g_qc_new[TAB_MANUAL][i].status = _make_label("-", LABEL_STATUS, g_qc_new[TAB_MANUAL][i].expose);
                nfui_nffixed_put((NFFIXED*)fixed, g_qc_new[TAB_MANUAL][i].status, 0, 0);
                break;

            default:
                break;
        }
        if(g_qc_new[TAB_MANUAL][i].expose)
        {
            nfui_nffixed_put((NFFIXED*)parent, g_qc_new[TAB_MANUAL][i].title, LABEL_TITLE_POS_X, pos_y);
            nfui_nffixed_put((NFFIXED*)parent, fixed, LABEL_TITLE_POS_X + size_w + LABEL_SPACE, pos_y);
            if(g_qc_new[TAB_MANUAL][i].btn){
                nfui_nffixed_put((NFFIXED*)parent, g_qc_new[TAB_MANUAL][i].btn, LABEL_TITLE_POS_X + (size_w*2) + (LABEL_SPACE*2), pos_y);
            }
            pos_y += size_h + LABEL_SPACE;
        }

    }
    return 0;
}

gint vw_qc_test_create_new(NFWINDOW *parent)
{
    NFOBJECT *main_wnd;
	NFOBJECT *main_fixed;
    NFOBJECT *tab, *tabPage[3];
	NFOBJECT *obj;
	NFOBJECT *btn[3];
	guint lc_size[] = {MSG_LIST_SIZE_W, };
    gchar strBuf[10];
    gint pos_x, pos_y, i, add_x = 0;
    gint size_w, size_h;

    gint btnExpose[BTN_CNT] = {1, 1, 1};
    const gchar *strBtn[BTN_CNT] = {"FACTORY DEFAULT", "FORMAT", "SAVE & FINISH"};
    const gchar *strImage[2] = {
                (MKB_IMG_TAB_POP_DIR_H_N_274),
                (MKB_IMG_TAB_POP_DIR_H_S_274)
    };
    const gchar *strTitle[] = {
                "SYSTEM INFORMATION",
                "AUTOMATIC TEST",
                "MANUAL TEST"
    };
    const guint colidx[3] = {COLOR_IDX(287), COLOR_IDX(289), COLOR_IDX(288)};


    /* window */
	main_wnd = nftool_create_popup_window(parent, QC_TEST_POS_X, QC_TEST_POS_Y, QC_TEST_SIZE_W, QC_TEST_SIZE_H, "QC APPLICATION", FALSE);
	nfui_regi_post_event_callback(main_wnd, _post_qc_test_window_event_cb);
	nfui_nfwindow_use_outside_evt((NFWINDOW*)main_wnd, FALSE);
	nfui_nfwindow_set_mask((NFWINDOW*)main_wnd, GDK_BUTTON_PRESS, TRUE);
	nfui_nfwindow_set_returnkey_proc((NFWINDOW*)main_wnd, _returnkey_proc);
	g_curwnd = main_wnd;

    /* fixed */
    main_fixed = ((NFWINDOW*)main_wnd)->child;

    /* tab */
    size_w = 260;
    tab = (NFOBJECT*)nfui_nftab_new(TAB_CNT, strImage, size_w, TAB_BUTTON_SIZE_H, NFTAB_DIR_H, strTitle, colidx);
    nfui_nftab_set_pango_font((NFTAB*)tab, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nftab_set_margin((NFTAB*)tab, 10);
    nfui_nffixed_put((NFFIXED*)main_fixed, tab, TAB_BUTTON_POS_X, TAB_BUTTON_POS_Y);
    nfui_nfobject_show(tab);

    for(i = 0; i < TAB_CNT; i++)
    {
        tabPage[i] = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_set_size(tabPage[i], TAB_PAGE_SIZE_W, TAB_PAGE_SIZE_H);
        nfui_nfobject_modify_bg((NFOBJECT*)tabPage[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
        nfui_nftab_regi_page((NFTAB*)tab, tabPage[i], i);
        nfui_nffixed_put((NFFIXED*)main_fixed, tabPage[i], TAB_PAGE_POS_X, TAB_PAGE_POS_Y);
        nfui_regi_pre_event_callback(tabPage[i], _pre_page_event_cb);
    }
    _init_tabpage_sysinfo(tabPage[TAB_SYSINFO]);
    _init_tabpage_auto(tabPage[TAB_AUTO]);
    _init_tabpage_manual(tabPage[TAB_MANUAL]);
    nfui_nfobject_show(tabPage[TAB_SYSINFO]);


    /* button */
    obj = nftool_normal_button_create_type1("OK", 162);
    nfui_nfobject_get_size(obj, &size_w, &size_h);
    nfui_nfbutton_set_font_alignment((NFBUTTON*)obj, NFALIGN_CENTER, 0);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, (TAB_PAGE_SIZE_W - size_w), (QC_TEST_SIZE_H - size_h - 20));
    nfui_regi_post_event_callback(obj, _post_okbutton_event_handler);
    nfui_nfobject_show(obj);

    for(i = 0;i < BTN_CNT;i++)
    {
        btn[i] = nftool_normal_button_create_type1(strBtn[i], 190);
        nfui_nfobject_get_size(btn[i], &size_w, &size_h);
        nfui_nfbutton_set_font_alignment((NFBUTTON*)btn[i], NFALIGN_CENTER, 0);
        if(btnExpose[i]){
            nfui_nffixed_put((NFFIXED*)main_fixed, btn[i], TAB_PAGE_POS_X + add_x, (QC_TEST_SIZE_H - size_h - 20));
            nfui_nfobject_show(btn[i]);
            add_x += size_w + 5;
        }
    }
    nfui_regi_post_event_callback(btn[BTN_FACINIT], _post_facbutton_event_handler);
	nfui_regi_post_event_callback(btn[BTN_FORMAT], _post_format_event_handler);
	nfui_regi_post_event_callback(btn[BTN_SAVE], _post_save_finish_event_handler);

	nfui_nfobject_disable(btn[BTN_SAVE]);
	g_saveBtn = btn[BTN_SAVE];

    _set_login_user(-1);

    uxm_reg_imsg_event(g_curwnd, IRET_SCM_SHUTDOWN);
    uxm_reg_imsg_event(tabPage[TAB_AUTO], INFY_APPQC_AUTO_TEST_ALL);
    uxm_monitor_on_imsg_event(tabPage[TAB_AUTO], INFY_APPQC_AUTO_TEST_ALL);

    uxm_reg_imsg_event(btn[BTN_FACINIT], INFY_APPQC_FACTORY_DEFAULT);
    uxm_reg_imsg_event(btn[BTN_FORMAT], INFY_FORMAT_RATE);
	uxm_reg_imsg_event(btn[BTN_FORMAT], INFY_FORMAT_CMPL);
	uxm_reg_imsg_event(btn[BTN_FORMAT], IRET_SCM_FORMAT_STORAGE);

	nfui_nfobject_show(main_fixed);

    /* set for key navi */
    nfui_make_key_hierarchy((NFWINDOW*)main_wnd);

    /* start auto test */
    scm_appqc_all_test(INFY_APPQC_AUTO_TEST_ALL);


    return 0;
}

gint vw_qc_test_show_new()
{
    _set_login_user(-1);

	nfui_nfobject_show((NFOBJECT*)g_curwnd);
	nfui_make_key_hierarchy(g_curwnd);
	nfui_page_open(PGID_POPUPWND, (NFOBJECT*)g_curwnd, ssm_get_cur_id(NULL));

	return 0;
}

gint vw_qc_test_hide_new()
{
	nfui_nfobject_hide((NFOBJECT*)g_curwnd);
	nfui_page_close(PGID_POPUPWND, (NFOBJECT*)g_curwnd);

	return 0;
}

gboolean vw_qc_test_is_shown_new()
{
	return nfui_nfobject_is_shown((NFOBJECT*)g_curwnd);
}
