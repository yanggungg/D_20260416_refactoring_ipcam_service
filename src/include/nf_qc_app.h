#ifndef __NF_QC_APP_H__
#define __NF_QC_APP_H__

typedef enum _NF_SYSMAN_QC_MODE
{   
	NF_SYSMAN_QC_SET = 0,
	NF_SYSMAN_QC_GET = 1

}NF_SYSMAN_QC_MODE;

typedef enum _NF_SYSMAN_RECORD_TIME_STAMP_RET
{
	NF_SYSMAN_QC_WRONG_DATA     = -1,
	NF_SYSMAN_QC_DECTECT_ERR    = 1,
	NF_SYSMAN_QC_NORMAL         = 0
}NF_SYSMAN_RECORD_TIME_STAMP_RET; 

typedef enum _NF_SYSMAN_QC_PD_ERROR_E
{
	NF_SYSMAN_QC_PD_ERROR_OK            = 0,
	NF_SYSMAN_QC_PD_ERROR_NETWORK       = 1,
	NF_SYSMAN_QC_PD_ERROR_NO_USB        = 2,
	NF_SYSMAN_QC_PD_ERROR_NO_DATADIR    = 3,
	NF_SYSMAN_QC_PD_ERROR_NO_DATA       = 4,
	NF_SYSMAN_QC_PD_ERROR_DUPLICATION   = 5,
	NF_SYSMAN_QC_PD_ERROR_WRONG_DATA    = 6,
	NF_SYSMAN_QC_PD_ERROR_ETC           = 7,
	NF_SYSMAN_QC_PD_ERROR_SET_DB        = 8,
	NF_SYSMAN_QC_PD_ERROR_NR            = 9,
} NF_SYSMAN_QC_PD_ERROR_E;

typedef enum _NF_SYSMAN_QC_LIC_ERROR_E
{
	NF_SYSMAN_QC_LIC_ERROR_NETWORK          = -1,
	NF_SYSMAN_QC_LIC_ERROR_WRONG_DATA       = -2,
	NF_SYSMAN_QC_LIC_ERROR_MAC_DATA         = -3,
	NF_SYSMAN_QC_LIC_ERROR_NO_DATA          = -4,
	NF_SYSMAN_QC_LIC_ERROR_SET_DB           = -5,
	NF_SYSMAN_QC_LIC_ERROR_MEMORY           = -6,
} NF_SYSMAN_QC_LIC_ERROR_E;

#if defined(SCHIP_COPY_PROTECTION)
// MAC WRITER TYPE
typedef struct _STPACKETDATA
{
	char cCode;
	int nParam1;
	int nParam2;
	char cParam100[100];            //for mac_data
	char cParam1000[1000];          //for hw_info
}STPacketData;

typedef struct _LICEDATA
{
	char mac[12];
	char lic[36];
	int product;
}LicData;

enum
{
	//Client To Server
	CTS_REQUESTMAC = 1,
	CTS_PRODUTCNAME,
	CTS_PRODUCTFINISH_SUCCESS,
	CTS_PRODUCTFINISH_FAIL,

	//Server To Client
	STC_RESPONSEMAC,                //5
	STC_RESPONSEPRODUCTNAME,    //6

	//Set To Client
	SetTC_SETINFO,                  //7
	SetTC_REQUESTMAC,               //8
	SetTC_ISMACSUCCESS,         //9
	SetTC_MACFILEPREPRARE,      //10

	//Client To Set
	CTSet_PREPAREINFO,              //11
	CTSet_SEND_MAC,             //12

	SetTC_SETTIME,
	CTSet_SEND_TIME,

	SetTC_REQUESTSERIAL,
	CTS_REQUESTSERIALNUM,
	STC_RESPONSESERIALNUM,
	CTSet_SEND_SERIAL,

	SetTC_ISSERIALSUCCESS,
};

enum{
	INSPECTOR_SUCCESS,                      //0
	INSPECTOR_NOTPRODUCTINFO,
	INSPECTOR_NOTSERVERCONNECT,
	INSPECTOR_NOMACFILE,
	INSPECTOR_SERVER_MACFILE_LOADERROR,
	INSPECTOR_CLIENT_MACFILE_LOADERROR,
	INSPECTOR_NOTVENDERINFO,
	INSPECTOR_NOTSERIALINFO,
};
// MAC WRITER END
#endif

#if defined(_UTM7G_1648D)
	#define QC_TEST_ALARM_ON_VAL            0xff
	#define QC_TEST_ALARM_OFF_VAL           0xff00
	#define QC_TEST_AUDIO_NUM               4
	#define QC_TEST_ALARM_NUM               16
	#define QC_TEST_FACTORY_KEY_GPIO_BASE   1
	#define QC_TEST_FACTORY_KEY_GPIO_PIN    21
#elif defined(_UTM7G_0824D)
	#define QC_TEST_ALARM_ON_VAL            0xf
	#define QC_TEST_ALARM_OFF_VAL           0xf0
	#define QC_TEST_AUDIO_NUM               4
	#define QC_TEST_ALARM_NUM               8
	#define QC_TEST_FACTORY_KEY_GPIO_BASE   1
	#define QC_TEST_FACTORY_KEY_GPIO_PIN    21
#elif defined(_UTM7G_0412D)
	#define QC_TEST_ALARM_ON_VAL            0x3
	#define QC_TEST_ALARM_OFF_VAL           0xc
	#define QC_TEST_AUDIO_NUM               4
	#define QC_TEST_ALARM_NUM               4
	#define QC_TEST_FACTORY_KEY_GPIO_BASE   2
	#define QC_TEST_FACTORY_KEY_GPIO_PIN    20
#else //B type imsi
	#define QC_TEST_ALARM_ON_VAL            0x1
	#define QC_TEST_ALARM_OFF_VAL           0x2
	#define QC_TEST_AUDIO_NUM               4
	#define QC_TEST_ALARM_NUM               2
	#define QC_TEST_FACTORY_KEY_GPIO_BASE   2 //imsi
	#define QC_TEST_FACTORY_KEY_GPIO_PIN    20 //imsi
#endif

gboolean nf_sysman_qcmode_init(void);

gboolean nf_sysman_qcmode_is_enable(void);
gboolean nf_sysman_qcmode_is_factory_default(void);
gboolean nf_sysman_qcmode_is_fwup(void);
gboolean nf_sysman_qcmode_is_format(void);
const gchar *nf_sysman_get_qcmode_option_lang(void);
void nf_sysman_set_qc_live_audio(guint is_on);
gboolean nf_sysman_check_is_loopout(void);
int nf_qc_ap_get_mac_writer_mode(void);

gboolean nf_sysman_qc_manual_test_factory_key(void);

int nf_sysman_qcmode_gui_printf(  const char *format, ... );
gboolean nf_sysman_qcmode_sysdb_init(void);
gboolean nf_sysman_qcmode_2nd_init(void);
guint nf_sysman_hdd_num_check(void);
guint nf_sysman_odd_num_check(void);

void nf_sysman_qc_set_signal(void);
gboolean nf_sysman_qc_get_ntp_serverip(char *serverip);
gboolean nf_sysman_qc_info_rtc(char *rtc_info);
#if defined(_ANF5HG_0824D) || defined(_ANF5HG_1648D) \
	|| defined(_ANF6HG_0824D) || defined(_ANF6HG_1648D)
	gboolean nf_sysman_qc_auto_test_switch(void);
#endif
gboolean nf_sysman_qc_toggle_loopout(int is_on);
gboolean nf_sysman_qc_auto_test_audio(void);
gboolean nf_sysman_qc_auto_test_rs485(void);
gboolean nf_sysman_qc_manual_test_rs485(void);
int nf_sysman_qc_record_time_stamp_test(int mode);
gboolean nf_sysman_qc_auto_test_fan(void);
gboolean nf_sysman_qc_auto_test_temper(void);

gboolean nf_sysman_qc_result_save(gchar item[][NF_SYSMAN_QC_ITEM_RESULT_MAX_LEN], int item_total_num);
gboolean nf_sysman_qc_factory_default(void);

gboolean nf_sysman_qc_auto_test_alarm(void);
 #if defined(SCHIP_COPY_PROTECTION)
	void nf_sysman_qc_get_msg(int sock, char *str);
	void nf_sysman_qc_send_msg(int clnt_sock, char *str);
	guint nf_sysman_qc_check_error_msg(char *msg);
	guint nf_sysman_qc_client(void);
	guint nf_sysman_qc_init_stpacketdata(STPacketData *packet);
	void nf_sysman_qc_send_mac_msg(int clnt_sock, STPacketData *packet);
	guint nf_sysman_qc_get_mac_msg(int clnt_sock, STPacketData *packet);
	guint nf_sysman_qc_check_error_mac_msg(STPacketData *packet);
	gboolean nf_sysman_qc_get_mac_data(int clnt_sock, char *fPath);
	void nf_sysman_set_vcode(char *param);
	guint nf_sysman_qc_pc_client(void);
	int macNameSet(char* descMac, char *srcMac, int mode );
	guint nf_sysman_qc_lic_write(char key[][36]);
	int scm_get_ntp_time_pc(char *server_name, GTimeVal *time);
#endif

#endif

