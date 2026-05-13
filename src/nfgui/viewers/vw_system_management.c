#include "nf_afx.h"

#include "nf_api_ipcam.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "objects/nffixed.h"
#include "objects/nflabel.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"
#include "objects/nfspinbutton.h"
#include "objects/nfimage.h"

#include "uxm.h"
#include "scm.h"
#include "iux_msg.h"
#include "modules/ssm.h"

#include "vw_sys_main.h"
#include "vw_system_management.h"
#include "vw_system_fw_up.h"
#include "vw_system_data_save.h"
#include "vw_system_data_load.h"
#include "vw_system_log_save.h"
#include "vw_system_self_start.h"
#include "vw_vkeyboard.h"
#include "vw_desc.h"

#include "evt.h"
#include "smt.h"
#include "vw.h"
#include "ix_conf.h"

#include "nf_api_param_app.h"
#include "nf_sysman.h"


#define SM_DEBUG

#define SM_LABEL_HEIGHT				(40)
#define SM_LABEL_WIDTH				(300)

#define SM_LABEL_TEXT_MARGIN		(20)

#define SM_CELL1_WIDTH				(200)
#define SM_CELL2_WIDTH				(200)

#define SM_TITLE_X					(8)
#define SM_TITLE_Y					(0)

#define SM_TABLE1_X					(SM_TITLE_X + SM_LABEL_TEXT_MARGIN)
#define SM_TABLE1_Y					(SM_TITLE_Y + SM_LABEL_HEIGHT + 5)

#define SM_TABLE1_ROW				(SM_OBJ1_MAX)
#define SM_TABLE1_COL				(3)

#define SM_TABLE1_ROW_SPACE			(2)
#define SM_TABLE1_COL_SPACE			(1)

#define SM_TABLE2_X					(SM_TITLE_X + SM_LABEL_TEXT_MARGIN)
#define SM_TABLE2_Y					(SM_TABLE1_Y + (SM_LABEL_HEIGHT+SM_TABLE1_ROW_SPACE)*SM_TABLE1_ROW + 30)

#define SM_TABLE2_ROW				(SM_OBJ2_MAX)
#define SM_TABLE2_COL				(2)

#define SM_TABLE2_ROW_SPACE			(2)
#define SM_TABLE2_COL_SPACE			(1)
#define MAX_STRING_SIZE				(16)

#define SLS_NOTICE              "Please note that the SYSTEM LOG will include all device information including e-mail addresses, log information, etc.\nDo you want to continue?"

#define SLS_NOTICE2              "Please note that the ANALYSIS LOG will include all device information including e-mail addresses, log information, etc.\nDo you want to continue?"

#if defined(_IPX_MODEL_UX) && defined(_SUPPORT_IPCAM_FWUP)
#define	STR_FW_UPDATE			"FW UPDATE (NVR)"
#else
#define	STR_FW_UPDATE			"FW UPDATE"
#endif

enum {
	SM_FWUP = 0,
#if defined(_SUPPORT_IPCAM_FWUP)
	SM_CAM_FWUP,
#endif
#if defined(_SUPPORT_REMOTE_UPGRADE)
	SM_REMOTE_FWUP,
#endif
#if defined(_IPX_MODEL_UX)
	SM_BLANK1,
#endif
	SM_SYS_REBOOT,
	SM_FAC_DEF,
	SM_SYS_LOG,
	SM_SYS_DATA,
#if defined(_SUPPORT_SELF_DIAGNOSIS)
	SM_SELF_TEST,
#endif
	SM_NET_WIZARD,
	SM_OBJ1_MAX
};

enum {
	SM_SYSTEM_ID = 0,
	SM_PASSWORD,
	SM_PASSWORD_EXPIRED,
	SM_PASSWORD_CHANGED,
	SM_AUTO_LOGOUT,
	SM_DURATION,
	SM_POE_LIMIT,
#if defined(GUI_16CH_SUPPORT) && defined(_SUPPORT_HUB)
	SM_POE_HUB_LIMIT,
#endif
#if defined(_SUPPORT_POE_TR_MODE)
	SM_POE_MODE,
#endif
	SM_SIGNAL_TYPE,
	SM_SWITCH_MODE,
	SM_OBJ2_MAX
};

enum {
	DURATION_1_MIN = 0,
	DURATION_2_MIN,
	DURATION_3_MIN,
	DURATION_5_MIN,
	DURATION_10_MIN,

	NUM_DURATIONS,
};

enum {
	POE_30W = 0,
	POE_38W,
	POE_40W,
	POE_60W,
	POE_72W,
	POE_80W,
	POE_86W,
#if defined(_FIXED_POE_POWER_LIMIT)
	POE_100W,
#endif
	POE_150W,
	POE_180W,
	POE_200W,
	NUM_POE,
};

enum {
	POE_HUB_40W = 0,
	POE_HUB_90W,
	NUM_POE_HUB,
};

enum {
	SIGNAL_NTSC = 0,
	SIGNAL_PAL,
	NUM_SIGNAL,
};

enum {
	MODE_CLOSE = 0,
	MODE_ATYPE = 1,
	MODE_BTYPE = 2,
	MODE_CTYPE = 3,
	MODE_DTYPE = 4,
	MODE_ETYPE = 5,
	MODE_FTYPE = 6,	// choissi 2012-11-23 ���� 8:56:42
	NUM_SWMODE,
	MODE_OPEN  = 999,
};

enum {
	MODE_STANDARD = 0,
	MODE_EXTENDED = 1,
	NUM_POEMODE,
};



static int support_swmode = 0;
static int support_net_wiz = 0;

static NFWINDOW *g_curwnd = 0;
static NFOBJECT 	*g_remotefw_obj;
static NFOBJECT 	*sm_obj[SM_OBJ2_MAX];

static gint 		media_cnt;
static MEDIA_INFO_T	*media_info;
static NFOBJECT 	*wait_pop = NULL;

static SysManageData smdata;
static SysManageData org_smdata;
static DiskManageData prev_dmdata;

static LogoutData logdata;
static LogoutData org_logdata;

static SysInfoData sys_info;
static SysInfoData org_infodata;


static void _show_system_manage_val()
{
#ifdef SM_DEBUG
    g_message("enPassword : cur : %d, org : %d", smdata.enPassword, org_smdata.enPassword);
    g_message("pwExpired : cur : %d, org : %d", smdata.pwExpired, org_smdata.pwExpired);
    g_message("name : cur : %s, org : %s", smdata.name, org_smdata.name);
    g_message("pwChanged : cur : %d, org : %d", smdata.pwChanged, org_smdata.pwChanged);
    g_message("poeLimt : cur : %d, org : %d", smdata.poeLimt, org_smdata.poeLimt);
    g_message("poeHubLimt : cur : %d, org : %d", smdata.poeHubLimt, org_smdata.poeHubLimt);
    g_message("sigType : cur : %d, org : %d", smdata.sigType, org_smdata.sigType);
    g_message("swmode : cur : %d, org : %d", smdata.swmode, org_smdata.swmode);
    g_message("remotefw_check : cur : %d, org : %d", smdata.remotefw_check, org_smdata.remotefw_check);

    g_message("autoLogout : cur : %d, org : %d", logdata.autoLogout, org_logdata.autoLogout);
    g_message("duration : cur : %d, org : %d", logdata.duration, org_logdata.duration);

    g_message("model : cur : %s, org : %s", sys_info.model, org_infodata.model);
    g_message("sysId : cur : %s, org : %s", sys_info.sysId, org_infodata.sysId);
    g_message("swVer : cur : %s, org : %s", sys_info.swVer, org_infodata.swVer);
    g_message("hwVer : cur : %s, org : %s", sys_info.hwVer, org_infodata.hwVer);
    g_message("ipAddr : cur : %s, org : %s", sys_info.ipAddr, org_infodata.ipAddr);
    g_message("macAddr : cur : %s, org : %s", sys_info.macAddr, org_infodata.macAddr);
    g_message("ddnsName : cur : %s, org : %s", sys_info.ddnsName, org_infodata.ddnsName);
    g_message("rtspPort : cur : %d, org : %d", sys_info.rtspPort, org_infodata.rtspPort);
    g_message("webPort : cur : %d, org : %d", sys_info.webPort, org_infodata.webPort);
    g_message("fwupTime : cur : %d, org : %d", sys_info.fwupTime, org_infodata.fwupTime);
#endif

    return;
}

static void _system_manage_init(void)
{
	memset(&smdata, 0x00, sizeof(SysManageData));
	memset(&org_smdata, 0x00, sizeof(SysManageData));

	DAL_get_sysManage_data(&smdata);

	g_memmove(&org_smdata, &smdata, sizeof(SysManageData));


	// logdata
	memset(&logdata, 0x00, sizeof(LogoutData));
	memset(&org_logdata, 0x00, sizeof(LogoutData));

	DAL_get_logout_data(&logdata);

	g_memmove(&org_logdata, &logdata, sizeof(LogoutData));


	// SysInfoData
	memset(&sys_info, 0x00, sizeof(SysInfoData));
	memset(&org_infodata, 0x00, sizeof(SysInfoData));

	DAL_get_sysInfo_data(&sys_info);

	g_memmove(&org_infodata, &sys_info, sizeof(SysInfoData));
}

static void _create_waitbox(void)
{
	wait_pop = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "Please wait...", "");
}

static void _remove_waitbox(void)
{
	if(wait_pop)
	{
		nftool_remove_waitbox(wait_pop);
		wait_pop = NULL;
	}
}

static gboolean _proc_escape(void *data)
{
	NFOBJECT *popup = (NFOBJECT *)data;

	nftool_remove_waitbox(popup);

    smt_set_service(SMT_SHUTDOWN);
    scm_shutdown_system(IRET_SCM_SHUTDOWN_SIGNAL_CHANGE);
    scm_notify_to_system("dvr_status", NF_DVR_STATUS_SHUTDOWN);

    nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");

	return FALSE;
}

static gint prvDurationToIndex(guint duration)
{
	gint ret = 0;

	if(duration < 2)    ret = DURATION_1_MIN;
	else if(duration < 3)   ret = DURATION_2_MIN;
	else if(duration < 5)   ret = DURATION_3_MIN;
	else if(duration < 10)  ret = DURATION_5_MIN;
	else    ret = DURATION_10_MIN;

	return ret;
}

static gint prvPoeToIndex(guint poe)
{
	gint ret = 0;

#if defined(_IPX_0412M4) || defined(_IPX_0412M4E)
	if (poe == 32)          ret = POE_30W;
#else
	if (poe == 30)          ret = POE_30W;
#endif	
	else if (poe == 38)		ret = POE_38W;
	else if (poe == 40)		ret = POE_40W;
	else if (poe == 60)		ret = POE_60W;
#if defined(_FIXED_POE_POWER_LIMIT)
	else if (poe == 100)	ret = POE_100W;
#endif
	else if (poe == 80)		ret = POE_80W;
	else if (poe == 86)		ret = POE_86W;
	else if (poe == 150)	ret = POE_150W;
	else if (poe == 180)	ret = POE_180W;
	else if (poe == 200)	ret = POE_200W;
	else                    ret = POE_72W;

	return ret;
}

static gint prvPoeHubToIndex(guint poe)
{
	gint ret = 0;

	if (poe == 40)          ret = POE_HUB_40W;
	else                    ret = POE_HUB_90W;

	return ret;
}

static gint prvRemoteFWCheckToIndex(guint period)
{
	gint ret = 0;

	if (period == 0)          ret = 0;
	else if (period == 10800)    ret = 1;
	else                    ret = 0;

	return ret;
}


static guint prvIndexToDuration(gint index)
{
	guint ret = 0;

	switch(index)
	{
		case DURATION_1_MIN:
		case DURATION_2_MIN:
		case DURATION_3_MIN:
			ret = index+1;
			break;

		case DURATION_5_MIN:
			ret = 5;
			break;

		case DURATION_10_MIN:
			ret = 10;
			break;

		default:
			ret = 0;
			break;
	}

	return ret;
}

static guint prvIndexToPoe(gint index)
{
	guint ret = 0;

	switch(index)
	{
		case POE_30W:
#if defined(_IPX_0412M4) || defined(_IPX_0412M4E)		
			ret = 32;
#else
			ret = 30;
#endif			
			break;
		case POE_38W:
			ret = 38;
			break;
		case POE_40W:
			ret = 40;
			break;
		case POE_60W:
			ret = 60;
			break;
		case POE_72W:
#if defined(_IPX_0824M4) || defined(_IPX_1648M4) || defined(_IPX_0824P4E) || defined(_IPX_1648P4E) || \
	defined(_IPX_0824M4E) || defined(_IPX_1648M4E) || defined(_IPX_32P4E) || defined(_IPX_32M4E) || defined(_IPX_32P5)
			ret = 70;
#else
			ret = 72;
#endif			
			break;
		case POE_80W:
			ret = 80;
			break;
		case POE_86W:
			ret = 86;
			break;
#if defined(_FIXED_POE_POWER_LIMIT)
		case POE_100W:
			ret = 100;
			break;
#endif
		case POE_150W:
			ret = 150;
			break;
		case POE_180W:
			ret = 180;
			break;
		case POE_200W:
			ret = 200;
			break;
		default:
			ret = 0;
			break;
	}

	return ret;
}

static guint prvIndexToPoeHub(gint index)
{
	guint ret = 0;

	switch(index)
	{
		case POE_HUB_40W:
			ret = 40;
			break;
		case POE_HUB_90W:
			ret = 90;
			break;
		default:
			ret = 0;
			break;
	}

	return ret;
}

static guint prvIndexToRemoteFWCheck(gint index)
{
	guint ret = 0;

	switch(index)
	{
		case 0:
			ret = 0;
		break;
		case 1:
			ret = 10800;
		break;

		default:
			ret = 0;
		break;
	}

	return ret;
}

static gboolean _proc_reset(void *data)
{
	scm_put_log(SYSTEM_RESTART, 0, 0);               //for S1 TTA, SWSONEDVR-561
	DAL_save_setup_db(NFSETUP_WINDOW_SYSTEM);
	DAL_save_setup_db(NFSETUP_WINDOW_USER);
	scm_reboot_system(RR_REBOOT_MENU, 30000);
	scm_reset_system();
    return FALSE;
}

static gboolean post_fwbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		VW_System_FWUpgrade_Open(g_curwnd);
	}
	return FALSE;
}

static gboolean post_net_fwbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		//NET_FWUpgrade_Open();
	}

	return FALSE;
}

static gboolean post_remoteup_run_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		gint i, retVal;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		_create_waitbox();
		retVal = scm_S1_update_remotefw_info();
		_remove_waitbox();

		if (retVal == 0)
		{
			nftool_mbox(g_curwnd, "NOTICE", "The latest updates have already been installed.", NFTOOL_MB_OK);
			return FALSE;
		}
		else if (retVal == -1)
		{
			nftool_mbox(g_curwnd, "WARNING", "Failed to connect to the server.", NFTOOL_MB_OK);
			return FALSE;
		}

		var_set_detect_fwver(" ");

		for (i = 0; i < GUI_CHANNEL_CNT; i++)
		{
			var_set_detect_cam_fwver(i, " ");
		}

		evt_send_to_local(INFY_CHECK_REMOTE_S1_NEWFW, 0, 0, 0);
	}

	return FALSE;
}

static gboolean post_cam_fwbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		VW_IPCamera_FWUpgrade_Open(g_curwnd);
	}
	return FALSE;
}

static gboolean post_rebootbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
    {
        gint ret;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		ret = nftool_mbox(g_curwnd, "NOTICE", "The System will be reboot.Do you want to contienue?", NFTOOL_MB_OKCANCEL);
        if (ret == NFTOOL_MB_CANCEL) return FALSE;

		if(VW_UserPwd_Open(g_curwnd, "REBOOT")) {
		    g_timeout_add(1000, _proc_reset, 0);
            nftool_mbox(g_curwnd, "NOTICE", "The system will be reboot soon.", NFTOOL_MB_NONE);
		}
    }

    return FALSE;
}

static gboolean post_facbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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
					smt_set_service(SMT_FAC_DEFAULT);
					scm_run_factory_default(IRET_FACTORY_DEFAULT_NOTIFY);
					_create_waitbox();
				}
			}
			break;

		case IRET_FACTORY_DEFAULT_NOTIFY:
			{
				CMM_MESSAGE_T *pmsg = (CMM_MESSAGE_T *)data;
				GTimeVal last_temp;
				gint i;
				WizardCheck usrdata;
            	int net_ret;

				printf("factory default ret = %d\n", pmsg->param);

				_remove_waitbox();

				sleep(5);

				DAL_get_WizardCheck_Data(&usrdata, 0);
				DAL_get_Netwizard_func(&net_ret);
				ssm_stop_auto_logout();

            	if(!net_ret)
            		usrdata.netwiz = FALSE;

				if(!usrdata.usable_defpw)
				{
					vw_init_userinfo_open(g_curwnd);
					DAL_set_Langwizard_func(FALSE);
					vw_wizard_init(g_curwnd, 0);
				}
				else if(usrdata.netwiz)
				{
					DAL_set_Langwizard_func(FALSE);
					vw_wizard_init(g_curwnd, 0);
				}
				
				if (DAL_get_agr_policy() == 0)
				{
					vw_provide_devinfo_notice2_open(g_curwnd);
				}
				
				if(pmsg->param == 0)
				{
					g_get_current_time(&last_temp);
					for(i=0; i<MAX_USER_COUNT; i++)
						DAL_set_pw_last_changed_time(last_temp.tv_sec, i);

					DAL_save_db("usr");
				}

				ssm_run_auto_logout();
			}
			break;

		case GDK_DELETE:
			uxm_unreg_imsg_event(obj, IRET_FACTORY_DEFAULT_NOTIFY);
			break;

		default:
			break;
	}

	return FALSE;
}

static gboolean post_create_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        mb_type ret = -1;
        gint ret_check;

		if (var_get_vendor_code() == 43)
			ret = nftool_mbox(g_curwnd, "NOTICE", SLS_NOTICE2, NFTOOL_MB_OKCANCEL);
		else
			ret = nftool_mbox(g_curwnd, "NOTICE", SLS_NOTICE, NFTOOL_MB_OKCANCEL);
        
		if(ret != NFTOOL_MB_OK){
            return FALSE;
        }

        ret_check = nf_sysman_create_sysinfo();
    }

    return FALSE;
}

static gboolean post_save_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        VW_System_Log_Save_Open(g_curwnd);

    }
    return FALSE;
}

static gboolean post_datasavebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top;
		guint x, y;
		gchar title[64] = {"SYSTEM DATA SAVE"};

		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		nfui_nfobject_get_window_pos(obj, &x, &y);

		x += ((GdkEventButton*)evt)->x;
		y += ((GdkEventButton*)evt)->y;

		if(!x && !y)
		{
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);

			x += (obj->width)/2 + top->x;
			y += obj->height + top->y;
		}

		VW_System_Data_Save_Open(g_curwnd, title, x, y);
	}

	return FALSE;
}

static gboolean post_dataloadbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;
	gchar title[32] = "SYSTEM DATA LOAD";

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint x, y;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		nfui_nfobject_get_window_pos(obj, &x, &y);

		x += ((GdkEventButton*)evt)->x;
		y += ((GdkEventButton*)evt)->y;

		if(!x && !y)
		{
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);

			x += (obj->width)/2 + top->x;
			y += obj->height + top->y;
		}

		if(VW_System_Data_Load_Open(g_curwnd, title, x, y))
		{
			ssm_run_auto_logout();
		}
	}

	return FALSE;
}

static gboolean post_net_wizard_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint x, y;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		//NetworkSetupInit_Open(g_curwnd, TRUE);
		vw_wizard_init(g_curwnd, 0);
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_ENTER) {
			//NetworkSetupInit_Open(g_curwnd, TRUE);
			vw_wizard_init(g_curwnd, 0);
        }
	}

	return FALSE;
}

static gboolean post_testbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		VW_System_Self_Start_Open(g_curwnd);
	}

	return FALSE;
}

static gboolean post_systemid_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;
    gint max_string_size = MAX_STRING_SIZE;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{
		NFOBJECT *top;
		gchar *strTemp;
		guint x, y;
    		gchar buf[256];

		if(kpid == KEYPAD_ENTER)
		{
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);

			x += (obj->width)/2 + top->x;
			y += obj->height + top->y;
		}else
		{
	  	   	if(evt->button.button == MOUSE_RIGTH_BUTTON)
			{
				return FALSE;
	  	   	}

			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += ((GdkEventButton*)evt)->x;
			y += ((GdkEventButton*)evt)->y;
		}

		strTemp = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), x, y, max_string_size, VKEY_ALPHANUMERIC);

		if(strTemp)
		{
			if(strlen(strTemp) == 0) return FALSE;

           	 	if (obj == sm_obj[SM_SYSTEM_ID])
         		{
               			nfui_nflabel_set_text((NFLABEL*)sm_obj[SM_SYSTEM_ID], sys_info.sysId);
    				nfui_signal_emit(sm_obj[SM_SYSTEM_ID], GDK_EXPOSE, TRUE);
           		}

			nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
			nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

			ifree(strTemp);
			strTemp = NULL;
		}
	}
	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint i;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		g_memmove(&smdata, &org_smdata, sizeof(SysManageData));
		g_memmove(&logdata, &org_logdata, sizeof(LogoutData));
		g_memmove(&sys_info, &org_infodata, sizeof(SysInfoData));

		nfui_nflabel_set_text(sm_obj[SM_SYSTEM_ID], sys_info.sysId);
		nfui_signal_emit(sm_obj[SM_SYSTEM_ID], GDK_EXPOSE, TRUE);

		nfui_spin_button_set_index(sm_obj[SM_PASSWORD], smdata.enPassword);

		nfui_signal_emit(sm_obj[SM_PASSWORD], GDK_EXPOSE, TRUE);
        if (smdata.enPassword)
        {
            nfui_nfobject_enable(sm_obj[SM_PASSWORD_EXPIRED]);
        }
        else
        {
            nfui_nfobject_disable(sm_obj[SM_PASSWORD_EXPIRED]);
        }

		nfui_spin_button_set_index_no_expose(sm_obj[SM_PASSWORD_EXPIRED], smdata.pwExpired);
		nfui_signal_emit(sm_obj[SM_PASSWORD_EXPIRED], GDK_EXPOSE, TRUE);

		nfui_spin_button_set_index(sm_obj[SM_PASSWORD_CHANGED], smdata.pwChanged);
		nfui_signal_emit(sm_obj[SM_PASSWORD_CHANGED], GDK_EXPOSE, TRUE);

		nfui_spin_button_set_index_no_expose(sm_obj[SM_AUTO_LOGOUT], logdata.autoLogout);
		nfui_signal_emit(sm_obj[SM_AUTO_LOGOUT], GDK_EXPOSE, TRUE);

        if (logdata.autoLogout)
        {
            nfui_nfobject_enable(sm_obj[SM_DURATION]);
        }
        else
        {
            nfui_nfobject_disable(sm_obj[SM_DURATION]);
        }

		nfui_spin_button_set_index_no_expose(sm_obj[SM_DURATION], prvDurationToIndex(logdata.duration));
		nfui_signal_emit(sm_obj[SM_DURATION], GDK_EXPOSE, TRUE);

		nfui_spin_button_set_index_no_expose(sm_obj[SM_POE_LIMIT], prvPoeToIndex(smdata.poeLimt));
		nfui_signal_emit(sm_obj[SM_POE_LIMIT], GDK_EXPOSE, TRUE);

#if defined(GUI_16CH_SUPPORT) && defined(_SUPPORT_HUB)
		nfui_spin_button_set_index_no_expose(sm_obj[SM_POE_HUB_LIMIT], prvPoeHubToIndex(smdata.poeHubLimt));
		nfui_signal_emit(sm_obj[SM_POE_HUB_LIMIT], GDK_EXPOSE, TRUE);
#endif
		nfui_spin_button_set_index_no_expose(sm_obj[SM_SIGNAL_TYPE], smdata.sigType);
		nfui_signal_emit(sm_obj[SM_SIGNAL_TYPE], GDK_EXPOSE, TRUE);

		if (support_swmode) {
			nfui_spin_button_set_index_no_expose(sm_obj[SM_SWITCH_MODE], smdata.swmode);
			nfui_signal_emit(sm_obj[SM_SWITCH_MODE], GDK_EXPOSE, TRUE);
		}

#if defined(_SUPPORT_POE_TR_MODE)
		nfui_spin_button_set_index_no_expose(sm_obj[SM_POE_MODE], smdata.poemode);
		nfui_signal_emit(sm_obj[SM_POE_MODE], GDK_EXPOSE, TRUE);
#endif
#if defined(_SUPPORT_REMOTE_UPGRADE)
		nfui_spin_button_set_index_no_expose(g_remotefw_obj, prvRemoteFWCheckToIndex(smdata.remotefw_check));
		nfui_signal_emit(g_remotefw_obj, GDK_EXPOSE, TRUE);
#endif

	}

	return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		GTimeVal tvTemp;
		gint i;
		int modified = 0;
        	gint ret_val, signal_changed = 0;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		strcpy(sys_info.sysId, nfui_nflabel_get_text((NFLABEL*)sm_obj[SM_SYSTEM_ID]));
		smdata.enPassword = nfui_spin_button_get_index(sm_obj[SM_PASSWORD]);
		smdata.pwExpired = nfui_spin_button_get_index(sm_obj[SM_PASSWORD_EXPIRED]);
		smdata.pwChanged = nfui_spin_button_get_index(sm_obj[SM_PASSWORD_CHANGED]);
		logdata.autoLogout = nfui_spin_button_get_index(sm_obj[SM_AUTO_LOGOUT]);
		logdata.duration = prvIndexToDuration(nfui_spin_button_get_index(sm_obj[SM_DURATION]));
		smdata.poeLimt = prvIndexToPoe(nfui_spin_button_get_index(sm_obj[SM_POE_LIMIT]));

#if defined(GUI_16CH_SUPPORT) && defined(_SUPPORT_HUB)
		smdata.poeHubLimt = prvIndexToPoeHub(nfui_spin_button_get_index(sm_obj[SM_POE_HUB_LIMIT]));
#endif
		smdata.sigType = nfui_spin_button_get_index(sm_obj[SM_SIGNAL_TYPE]);

#if defined(_SUPPORT_REMOTE_UPGRADE)
		smdata.remotefw_check = prvIndexToRemoteFWCheck(nfui_spin_button_get_index(g_remotefw_obj));
#endif

		if(memcmp(&org_infodata,&sys_info,sizeof(SysInfoData)))
		{
			g_memmove(&org_infodata,&sys_info,sizeof(SysInfoData));
			DAL_set_sysInfo_data(&sys_info);
			scm_put_log(CHANGE_SYS_INFO,0,0);
        	modified = 1;
		}

		if (support_swmode) {
			smdata.swmode = nfui_spin_button_get_index(sm_obj[SM_SWITCH_MODE]);
		}

#if defined(_SUPPORT_POE_TR_MODE)
		smdata.poemode = nfui_spin_button_get_index(sm_obj[SM_POE_MODE]);
#endif

		if(memcmp(&org_smdata, &smdata, sizeof(SysManageData)))
		{
			if(smdata.pwExpired != org_smdata.pwExpired)
			{
				g_get_current_time(&tvTemp);
				for(i=0; i<MAX_USER_COUNT; i++)
					DAL_set_pw_last_changed_time(tvTemp.tv_sec, i);
					//DAL_set_expire_check_time(tvTemp.tv_sec, i);

				DAL_save_db("usr");
			}

        	if (smdata.sigType != org_smdata.sigType)
        	{
                ret_val = vw_mbox(g_curwnd, "NOTICE", IMBX_REBOOT_VIDEO_TYPE_CHANGED, NFTOOL_MB_OKCANCEL);

                if (ret_val == NFTOOL_MB_OK)
                    signal_changed = 1;
                else
                {
                    smdata.sigType = org_smdata.sigType;
            		nfui_spin_button_set_index_no_expose(sm_obj[SM_SIGNAL_TYPE], smdata.sigType);
            		nfui_signal_emit(sm_obj[SM_SIGNAL_TYPE], GDK_EXPOSE, TRUE);
                }

                if(memcmp(&org_smdata, &smdata, sizeof(SysManageData)))
                {
        			g_memmove(&org_smdata, &smdata, sizeof(SysManageData));
        			DAL_set_sysManage_data(&smdata);
        			nf_api_param_app_set_cate(NF_SYSMAN_APP_PARAM_CATE_IS_PAL, (gint)smdata.sigType);

        			modified = 1;
                }
            }
            else
            {
    			g_memmove(&org_smdata, &smdata, sizeof(SysManageData));
    			DAL_set_sysManage_data(&smdata);

    			modified = 1;
            }
		}

		if (memcmp(&org_logdata, &logdata, sizeof(LogoutData))) {
			g_memmove(&org_logdata, &logdata, sizeof(LogoutData));
			DAL_set_logout_data(&logdata);


			DAL_save_db("usr");
			modified = 1;

			ssm_reconfig_auto_logout();
			scm_put_log(CHANGE_AUTO_LOGOUT, 0, 0);
		}

		if (modified == 1) {
    		if (signal_changed)	{
            	NFOBJECT *save_pop;

				save_pop = nftool_mbox_wait(g_curwnd, "NOTICE", "Configuration has been saved.");
				sysdisp_set_changeflag(1);

				DAL_save_setup_db(NFSETUP_WINDOW_SYSTEM);
				g_timeout_add(1000, _proc_escape, save_pop);
    		}
            else
            {
    			nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
    			VW_SetupSystem_set_changeflag(1);
    		}
		}
	}
	else if(evt->type == IRET_SCM_SHUTDOWN_SIGNAL_CHANGE)
	{
		scm_reboot_system(RR_SIGNAL_CHANGE, 0);
	}
	else if(evt->type == GDK_DELETE)
	{
		uxm_unreg_imsg_event(obj, IRET_SCM_SHUTDOWN_SIGNAL_CHANGE);
	}

	return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		VW_SystemManage_tab_out_handler();
		VW_SetupSystem_Destroy(obj);
	}

	return FALSE;
}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
	}

	return FALSE;
}

static gboolean post_password_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_SPINBUTTON_CHANGED)
	{
		gint idx = -1;

		idx = nfui_spin_button_get_index((NFSPINBUTTON*)obj);

		if(idx > 0) nfui_nfobject_enable(sm_obj[SM_PASSWORD_EXPIRED]);
		else		nfui_nfobject_disable(sm_obj[SM_PASSWORD_EXPIRED]);
		nfui_signal_emit(sm_obj[SM_PASSWORD_EXPIRED], GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

static gboolean post_autologout_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_SPINBUTTON_CHANGED)
	{
		gint idx = -1;

		idx = nfui_spin_button_get_index((NFSPINBUTTON*)obj);

		if(idx > 0) nfui_nfobject_enable(sm_obj[SM_DURATION]);
		else		nfui_nfobject_disable(sm_obj[SM_DURATION]);
		nfui_signal_emit(sm_obj[SM_DURATION], GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

void VW_Init_SystemManage_Page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *tbl;
	NFOBJECT *obj;
	NFOBJECT *title_objs[SM_OBJ1_MAX+SM_OBJ2_MAX];

	gchar *strLabel[] = {STR_FW_UPDATE,
#if defined(_SUPPORT_IPCAM_FWUP)
		"FW UPDATE (IPCAM)",
#endif
#if defined(_SUPPORT_REMOTE_UPGRADE)
		"REMOTE FW UPDATE",
#endif
#if defined(_IPX_MODEL_UX)
		"",
#endif
		"REBOOT SYSTEM",
		"FACTORY DEFAULT",
		"SYSTEM LOG",
		"SYSTEM DATA",
#if defined(_SUPPORT_SELF_DIAGNOSIS)
		"NVR SELF-DIAGNOSIS",
#endif
		"SETUP WIZARD",
		"SYSTEM ID",
		"PASSWORD",
		"PASSWORD EXPIRED",
		"RENEW PASSWORD",
		"AUTO LOGOUT",
		"WAIT TIME",
		"POE POWER LIMIT",
#if defined(GUI_16CH_SUPPORT) && defined(_SUPPORT_HUB)
		"POE POWER LIMIT (HUB)",
#endif
#if defined(_SUPPORT_POE_TR_MODE)
		"POE TRANSMISSION MODE",
#endif
		"AC POWER FREQUENCY",
		"SWITCH OPERATION MODE",
	};

	const gchar *strOffOn[] = {"OFF", "ON"};
	const gchar *strDuration[] = {"1 MINUTE", "2 MINUTES", "3 MINUTES", "5 MINUTES", "10 MINUTES"};
	const gchar *strExpired[] = {"NOT USED", "1 MONTH", "2 MONTHS", "4 MONTHS", "6 MONTHS", "12 MONTHS"};
#if defined(_IPX_0412M4) || defined(_IPX_0824M4) || defined(_IPX_1648M4) || defined(_IPX_0824P4E) || \
	defined(_IPX_1648P4E)  || defined(_IPX_0412M4E) || defined(_IPX_0824M4E) || defined(_IPX_1648M4E) || defined(_IPX_32P4E) || defined(_IPX_32M4E) || defined(_IPX_32P5)
	const gchar *strPoe[] = {"32W", "38W", "40W", "60 W", "70W", "80W", "86W", "100W", "150W", "180W", "200W"};
#else
	const gchar *strPoe[] = {"30W", "38W", "40W", "60 W", "72W", "80W", "86W", "100W", "150W", "180W", "200W"};
#endif
	const gchar *strPoeHub[] = {"40 W", "90W"};
	const gchar *strVideo[] = {"60 Hz", "50 Hz"};
	const gchar *strSWMode[] = {"CLOSE", "A TYPE", "B TYPE", "C TYPE", "D TYPE", "E TYPE", "F TYPE"};
	const gchar *strPOEMode[] = {"STANDARD", "EXTENDED"};

	guint table1_w[] = {SM_LABEL_WIDTH, SM_CELL1_WIDTH, SM_CELL2_WIDTH};
	guint table2_w[] = {SM_LABEL_WIDTH, SM_CELL1_WIDTH+SM_CELL2_WIDTH};
	guint i;
	int ret;


	g_curwnd = nfui_nfobject_get_top(parent);


	_system_manage_init();

	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(content_fixed, MENU_V_INNER_W, MENU_V_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);

// <----- SYSTEM INFORMATION TITLE.
	obj = nfui_nfimage_new(IMG_TITLE_BG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "SYSTEM MANAGEMENT");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), 192);
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, SM_LABEL_TEXT_MARGIN);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, SM_TITLE_X, SM_TITLE_Y);


	// table 1  /////////////////////////////////////////////////////////////////////////////////////
	tbl = (NFOBJECT*)nfui_nftable_new(SM_TABLE1_COL, SM_TABLE1_ROW, SM_TABLE1_COL_SPACE, SM_TABLE1_ROW_SPACE, table1_w, SM_LABEL_HEIGHT);
	nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_show(tbl);
	nfui_nffixed_put((NFFIXED*)content_fixed, tbl, SM_TABLE1_X, SM_TABLE1_Y);

	support_net_wiz = ivsc.dfunc.wizard.support;

	// label
	for(i=0; i<SM_TABLE1_ROW; i++) {
#if defined(_SUPPORT_IPCAM_FWUP)
	    if(var_get_supported_ipcam_fwup() == 0 && (i == SM_CAM_FWUP)) continue;
#endif

		if( (var_get_vendor_code() == 43) && (strcmp(strLabel[i],"SYSTEM LOG") == 0))
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ANALYSIS LOG", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(121));
		else
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strLabel[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(121));
		
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nftable_attach((NFTABLE*)tbl, obj,  0, i);
		nfui_nfobject_show(obj);

		title_objs[i] = obj;
		if((i == SM_NET_WIZARD) && (!support_net_wiz))
		    nfui_nfobject_hide(obj);

	}

	// button
	obj = (NFOBJECT*)nftool_normal_button_create_type3("USB", SM_CELL1_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_regi_post_event_callback(obj, post_fwbutton_event_handler);
	nfui_nftable_attach((NFTABLE*)tbl, obj,  1, SM_FWUP);

	obj = (NFOBJECT*)nftool_normal_button_create_type3("NETWORK", SM_CELL2_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	//nfui_nfobject_show(obj);
	nfui_regi_post_event_callback(obj, post_net_fwbutton_event_handler);
	nfui_nftable_attach((NFTABLE*)tbl, obj,  2, SM_FWUP);

#if defined(_SUPPORT_IPCAM_FWUP)
    if(var_get_supported_ipcam_fwup())
    {
    	obj = (NFOBJECT*)nftool_normal_button_create_type3("USB", SM_CELL1_WIDTH);
    	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    	nfui_nfobject_show(obj);
    	nfui_regi_post_event_callback(obj, post_cam_fwbutton_event_handler);
    	nfui_nftable_attach((NFTABLE*)tbl, obj,  1, SM_CAM_FWUP);
	}
#endif

// <---- REMOTE UPGRADE
#if defined(_SUPPORT_REMOTE_UPGRADE)
	g_remotefw_obj = nfui_spinbutton_new(strOffOn, 2, prvRemoteFWCheckToIndex(smdata.remotefw_check));
	nfui_spinbutton_set_skin_type((NFSPINBUTTON*)g_remotefw_obj, NFSPINBUTTON_TYPE_1);
	nfui_spin_button_set_align((NFSPINBUTTON*)g_remotefw_obj, NFALIGN_CENTER, 0);
	nfui_nfobject_show(g_remotefw_obj);
	nfui_nftable_attach((NFTABLE*)tbl, g_remotefw_obj, 1, SM_REMOTE_FWUP);

	obj = (NFOBJECT*)nftool_normal_button_create_type3("CHECK NOW", SM_CELL1_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_regi_post_event_callback(obj, post_remoteup_run_event_handler);
	nfui_nftable_attach((NFTABLE*)tbl, obj,  2, SM_REMOTE_FWUP);
#endif

	obj = (NFOBJECT*)nftool_normal_button_create_type3("REBOOT", SM_CELL1_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_regi_post_event_callback(obj, post_rebootbutton_event_handler);
	nfui_nftable_attach((NFTABLE*)tbl, obj,  1, SM_SYS_REBOOT);

	obj = (NFOBJECT*)nftool_normal_button_create_type3("LOAD", SM_CELL1_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_regi_post_event_callback(obj, post_facbutton_event_handler);
	nfui_nftable_attach((NFTABLE*)tbl, obj,  1, SM_FAC_DEF);
	uxm_reg_imsg_event(obj, IRET_FACTORY_DEFAULT_NOTIFY);

	obj = (NFOBJECT*)nftool_normal_button_create_type3("CREATE", SM_CELL1_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_regi_post_event_callback(obj, post_create_event_handler);
	nfui_nftable_attach((NFTABLE*)tbl, obj, 1, SM_SYS_LOG);

	obj = (NFOBJECT*)nftool_normal_button_create_type3("SAVE", SM_CELL1_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_regi_post_event_callback(obj, post_save_event_handler);
    nfui_nftable_attach((NFTABLE*)tbl, obj, 2, SM_SYS_LOG);

	obj = (NFOBJECT*)nftool_normal_button_create_type3("SAVE", SM_CELL1_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_regi_post_event_callback(obj, post_datasavebutton_event_handler);
	nfui_nftable_attach((NFTABLE*)tbl, obj,  1, SM_SYS_DATA);

	obj = (NFOBJECT*)nftool_normal_button_create_type3("LOAD", SM_CELL2_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_regi_post_event_callback(obj, post_dataloadbutton_event_handler);
	nfui_nftable_attach((NFTABLE*)tbl, obj,  2, SM_SYS_DATA);

#if defined(_SUPPORT_SELF_DIAGNOSIS)
	obj = (NFOBJECT*)nftool_normal_button_create_type3("TEST", SM_CELL1_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_regi_post_event_callback(obj, post_testbutton_event_handler);
	nfui_nftable_attach((NFTABLE*)tbl, obj,  1, SM_SELF_TEST);
#endif

	obj = (NFOBJECT*)nftool_normal_button_create_type3("RUN", SM_CELL1_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_regi_post_event_callback(obj, post_net_wizard_button_event_handler);
	nfui_nftable_attach((NFTABLE*)tbl, obj,  1, SM_NET_WIZARD);
	if(support_net_wiz)
	    nfui_nfobject_show(obj);

	// table 2 /////////////////////////////////////////////////////////////////////////////////////
	tbl = (NFOBJECT*)nfui_nftable_new(SM_TABLE2_COL, SM_TABLE2_ROW, SM_TABLE2_COL_SPACE, SM_TABLE2_ROW_SPACE, table2_w, SM_LABEL_HEIGHT);
	nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_show(tbl);
	nfui_nffixed_put((NFFIXED*)content_fixed, tbl, SM_TABLE2_X, SM_TABLE2_Y);

	support_swmode = ivsc.dfunc.support_swmode;

	// label
	for(i = 0; i < SM_TABLE2_ROW; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strLabel[i+SM_TABLE1_ROW], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)tbl, obj,  0, i);

		if ((i == SM_SWITCH_MODE) && (!support_swmode)) nfui_nfobject_hide(obj);
	}

// <---- SYSTEMID
	sm_obj[SM_SYSTEM_ID] = (NFOBJECT*)nfui_nflabel_new_text_box(sys_info.sysId,nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nfobject_use_focus(sm_obj[SM_SYSTEM_ID], NFOBJECT_FOCUS_ON);
	nfui_nflabel_set_skin_type((NFLABEL*)sm_obj[SM_SYSTEM_ID], NFTEXTBOX_TYPE_INPUT);
	nfui_nflabel_set_align((NFLABEL*)sm_obj[SM_SYSTEM_ID], NFALIGN_CENTER, 0);
	nfui_nfobject_support_multi_lang((NFOBJECT*)sm_obj[SM_SYSTEM_ID], FALSE);
	nfui_nfobject_show(sm_obj[SM_SYSTEM_ID]);
	nfui_nftable_attach((NFTABLE*)tbl, sm_obj[SM_SYSTEM_ID], 1, SM_SYSTEM_ID);
	nfui_regi_post_event_callback(sm_obj[SM_SYSTEM_ID], post_systemid_event_handler);

// <---- PASSWORD
	sm_obj[SM_PASSWORD] = nfui_spinbutton_new(strOffOn, 2, smdata.enPassword);
	nfui_spinbutton_set_skin_type((NFSPINBUTTON*)sm_obj[SM_PASSWORD], NFSPINBUTTON_TYPE_1);
	nfui_spin_button_set_align((NFSPINBUTTON*)sm_obj[SM_PASSWORD], NFALIGN_CENTER, 0);
	nfui_nfobject_show(sm_obj[SM_PASSWORD]);
	nfui_nftable_attach((NFTABLE*)tbl, sm_obj[SM_PASSWORD], 1, SM_PASSWORD);
	nfui_regi_post_event_callback(sm_obj[SM_PASSWORD], post_password_event_handler);


// <---- PASSWORD EXPIRED
	sm_obj[SM_PASSWORD_EXPIRED] = nfui_spinbutton_new(strExpired, 5, smdata.pwExpired);
	nfui_spinbutton_set_skin_type((NFSPINBUTTON*)sm_obj[SM_PASSWORD_EXPIRED], NFSPINBUTTON_TYPE_1);
	nfui_spin_button_set_align((NFSPINBUTTON*)sm_obj[SM_PASSWORD_EXPIRED], NFALIGN_CENTER, 0);
	nfui_nfobject_show(sm_obj[SM_PASSWORD_EXPIRED]);
	nfui_nftable_attach((NFTABLE*)tbl, sm_obj[SM_PASSWORD_EXPIRED], 1, SM_PASSWORD_EXPIRED);

	if (!smdata.enPassword)
		nfui_nfobject_disable(sm_obj[SM_PASSWORD_EXPIRED]);

// <---- RENEW PASSWORD
	sm_obj[SM_PASSWORD_CHANGED] = nfui_spinbutton_new(strOffOn, 2, smdata.pwChanged);
	nfui_spinbutton_set_skin_type((NFSPINBUTTON*)sm_obj[SM_PASSWORD_CHANGED], NFSPINBUTTON_TYPE_1);
	nfui_spin_button_set_align((NFSPINBUTTON*)sm_obj[SM_PASSWORD_CHANGED], NFALIGN_CENTER, 0);
	nfui_nfobject_show(sm_obj[SM_PASSWORD_CHANGED]);
	nfui_nftable_attach((NFTABLE*)tbl, sm_obj[SM_PASSWORD_CHANGED], 1, SM_PASSWORD_CHANGED);

// <---- AUTO LOGOUT
	sm_obj[SM_AUTO_LOGOUT] = nfui_spinbutton_new(strOffOn, 2, logdata.autoLogout);
	nfui_spinbutton_set_skin_type((NFSPINBUTTON*)sm_obj[SM_AUTO_LOGOUT], NFSPINBUTTON_TYPE_1);
	nfui_spin_button_set_align((NFSPINBUTTON*)sm_obj[SM_AUTO_LOGOUT], NFALIGN_CENTER, 0);
	nfui_nfobject_show(sm_obj[SM_AUTO_LOGOUT]);
	nfui_nftable_attach((NFTABLE*)tbl, sm_obj[SM_AUTO_LOGOUT], 1, SM_AUTO_LOGOUT);
	nfui_regi_post_event_callback(sm_obj[SM_AUTO_LOGOUT], post_autologout_event_handler);

// <---- DURATION
	sm_obj[SM_DURATION] = nfui_spinbutton_new(strDuration, NUM_DURATIONS, prvDurationToIndex(logdata.duration));
	nfui_spinbutton_set_skin_type((NFSPINBUTTON*)sm_obj[SM_DURATION], NFSPINBUTTON_TYPE_1);
	nfui_spin_button_set_align((NFSPINBUTTON*)sm_obj[SM_DURATION], NFALIGN_CENTER, 0);
	nfui_nfobject_show(sm_obj[SM_DURATION]);
	nfui_nftable_attach((NFTABLE*)tbl, sm_obj[SM_DURATION], 1, SM_DURATION);

	if (!logdata.autoLogout)
		nfui_nfobject_disable(sm_obj[SM_DURATION]);

// <---- POE LIMIT
	sm_obj[SM_POE_LIMIT] = nfui_spinbutton_new(strPoe, NUM_POE, prvPoeToIndex(smdata.poeLimt));
	nfui_spinbutton_set_skin_type((NFSPINBUTTON*)sm_obj[SM_POE_LIMIT], NFSPINBUTTON_TYPE_1);
	nfui_spin_button_set_align((NFSPINBUTTON*)sm_obj[SM_POE_LIMIT], NFALIGN_CENTER, 0);
	nfui_nfobject_show(sm_obj[SM_POE_LIMIT]);
	nfui_nftable_attach((NFTABLE*)tbl, sm_obj[SM_POE_LIMIT], 1, SM_POE_LIMIT);
#if defined(_FIXED_POE_POWER_LIMIT)
    nfui_nfobject_disable(sm_obj[SM_POE_LIMIT]);
#endif
#if defined(_NOT_SUPPORT_POECHECK)
    nfui_nfobject_disable(sm_obj[SM_POE_LIMIT]);
#endif


#if defined(GUI_16CH_SUPPORT) && defined(_SUPPORT_HUB)
	sm_obj[SM_POE_HUB_LIMIT] = nfui_spinbutton_new(strPoeHub, NUM_POE_HUB, prvPoeHubToIndex(smdata.poeHubLimt));
	nfui_spinbutton_set_skin_type((NFSPINBUTTON*)sm_obj[SM_POE_HUB_LIMIT], NFSPINBUTTON_TYPE_1);
	nfui_spin_button_set_align((NFSPINBUTTON*)sm_obj[SM_POE_HUB_LIMIT], NFALIGN_CENTER, 0);
	nfui_nfobject_show(sm_obj[SM_POE_HUB_LIMIT]);
	nfui_nftable_attach((NFTABLE*)tbl, sm_obj[SM_POE_HUB_LIMIT], 1, SM_POE_HUB_LIMIT);
#endif

// <---- VIDEO TYPE
	sm_obj[SM_SIGNAL_TYPE] = nfui_spinbutton_new(strVideo, NUM_SIGNAL, smdata.sigType);
	nfui_spinbutton_set_skin_type((NFSPINBUTTON*)sm_obj[SM_SIGNAL_TYPE], NFSPINBUTTON_TYPE_1);
	nfui_spin_button_set_align((NFSPINBUTTON*)sm_obj[SM_SIGNAL_TYPE], NFALIGN_CENTER, 0);
	nfui_nfobject_show(sm_obj[SM_SIGNAL_TYPE]);
	nfui_nftable_attach((NFTABLE*)tbl, sm_obj[SM_SIGNAL_TYPE], 1, SM_SIGNAL_TYPE);

// <---- SWITCH OPERATION MODE
	sm_obj[SM_SWITCH_MODE] = nfui_spinbutton_new(strSWMode, NUM_SWMODE, smdata.swmode);
	nfui_spinbutton_set_skin_type((NFSPINBUTTON*)sm_obj[SM_SWITCH_MODE], NFSPINBUTTON_TYPE_1);
	nfui_spin_button_set_align((NFSPINBUTTON*)sm_obj[SM_SWITCH_MODE], NFALIGN_CENTER, 0);
	nfui_nfobject_support_multi_lang(sm_obj[SM_SWITCH_MODE], FALSE);
	nfui_nftable_attach((NFTABLE*)tbl, sm_obj[SM_SWITCH_MODE], 1, SM_SWITCH_MODE);
	if (support_swmode) nfui_nfobject_show(sm_obj[SM_SWITCH_MODE]);

#if defined(_SUPPORT_POE_TR_MODE)
// <---- POE TRANSMISSION MODE
	sm_obj[SM_POE_MODE] = nfui_spinbutton_new(strPOEMode, NUM_POEMODE, smdata.poemode);
	nfui_spinbutton_set_skin_type((NFSPINBUTTON*)sm_obj[SM_POE_MODE], NFSPINBUTTON_TYPE_1);
	nfui_spin_button_set_align((NFSPINBUTTON*)sm_obj[SM_POE_MODE], NFALIGN_CENTER, 0);
	nfui_nfobject_support_multi_lang(sm_obj[SM_SWITCH_MODE], FALSE);
	nfui_nftable_attach((NFTABLE*)tbl, sm_obj[SM_POE_MODE], 1, SM_POE_MODE);
	nfui_nfobject_show(sm_obj[SM_POE_MODE]);
#endif

// <---- CANCEL, APPLY, CLOSE BUTTON
	obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R3_X, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);

	obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R2_X, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_applybutton_event_handler);
	uxm_reg_imsg_event(obj, IRET_SCM_SHUTDOWN_SIGNAL_CHANGE);

	obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R1_X, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

	nfui_regi_post_event_callback(parent, post_page_event_handler);

}


gboolean VW_SystemManage_tab_out_handler()
{
	mb_type ret;
	GTimeVal tvTemp;
	gint i;
	gint ret_val, signal_changed = 0;
	gint cmp_ret1 = 0, cmp_ret2 = 0, cmp_ret3 = 0;

	strcpy(sys_info.sysId, nfui_nflabel_get_text((NFLABEL*)sm_obj[SM_SYSTEM_ID]));
	smdata.enPassword = nfui_spin_button_get_index(sm_obj[SM_PASSWORD]);
	smdata.pwExpired = nfui_spin_button_get_index(sm_obj[SM_PASSWORD_EXPIRED]);
	smdata.pwChanged = nfui_spin_button_get_index(sm_obj[SM_PASSWORD_CHANGED]);
	logdata.autoLogout= nfui_spin_button_get_index(sm_obj[SM_AUTO_LOGOUT]);
	logdata.duration = prvIndexToDuration(nfui_spin_button_get_index(sm_obj[SM_DURATION]));
	smdata.poeLimt = prvIndexToPoe(nfui_spin_button_get_index(sm_obj[SM_POE_LIMIT]));
#if defined(GUI_16CH_SUPPORT) && defined(_SUPPORT_HUB)
	smdata.poeHubLimt = prvIndexToPoeHub(nfui_spin_button_get_index(sm_obj[SM_POE_HUB_LIMIT]));
#endif
	smdata.sigType = nfui_spin_button_get_index(sm_obj[SM_SIGNAL_TYPE]);
	if (support_swmode) {
		smdata.swmode = nfui_spin_button_get_index(sm_obj[SM_SWITCH_MODE]);
	}
#if defined(_SUPPORT_POE_TR_MODE)
	smdata.poemode = nfui_spin_button_get_index(sm_obj[SM_POE_MODE]);
#endif
#if defined(_SUPPORT_REMOTE_UPGRADE)
	smdata.remotefw_check = prvIndexToRemoteFWCheck(nfui_spin_button_get_index(g_remotefw_obj));
#endif

	if (!(cmp_ret1 = memcmp(&org_smdata, &smdata, sizeof(SysManageData))) &&
		!(cmp_ret2 = memcmp(&org_logdata, &logdata, sizeof(LogoutData))) &&
		!(cmp_ret3 = memcmp(&org_infodata, &sys_info, sizeof(SysInfoData)))) {
		return FALSE;
	}
    _show_system_manage_val();
    
	ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

	if(ret == NFTOOL_MB_OK)
	{
		if(smdata.pwExpired != org_smdata.pwExpired)
		{
			g_get_current_time(&tvTemp);
			for(i=0; i<MAX_USER_COUNT; i++)
				DAL_set_expire_check_time(tvTemp.tv_sec, i);

			DAL_save_db("usr");
		}

    	if (smdata.sigType != org_smdata.sigType)
    	{
            ret_val = vw_mbox(g_curwnd, "NOTICE", IMBX_REBOOT_VIDEO_TYPE_CHANGED, NFTOOL_MB_OKCANCEL);

            if (ret_val == NFTOOL_MB_OK)
            {
                signal_changed = 1;
                nf_api_param_app_set_cate(NF_SYSMAN_APP_PARAM_CATE_IS_PAL, (gint)smdata.sigType);
            }
            else
            {
                smdata.sigType = org_smdata.sigType;
        		nfui_spin_button_set_index_no_expose(sm_obj[SM_SIGNAL_TYPE], smdata.sigType);
            }

        	if(!memcmp(&org_smdata, &smdata, sizeof(SysManageData)))
        		return FALSE;
        }

		g_memmove(&org_smdata, &smdata, sizeof(SysManageData));
		DAL_set_sysManage_data(&smdata);
		g_memmove(&org_logdata, &logdata, sizeof(LogoutData));
		DAL_set_logout_data(&logdata);
		g_memmove(&org_infodata,&sys_info,sizeof(SysInfoData));
		DAL_set_sysInfo_data(&sys_info);

		VW_SetupSystem_set_changeflag(1);

		if (signal_changed)	{
			DAL_save_setup_db(NFSETUP_WINDOW_SYSTEM);

			smt_set_service(SMT_SHUTDOWN);
			scm_shutdown_system(IRET_SCM_SHUTDOWN_SIGNAL_CHANGE);
			scm_notify_to_system("dvr_status", NF_DVR_STATUS_SHUTDOWN);

			nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
			gtk_main();
		}

	}
	else if(ret == NFTOOL_MB_CANCEL)
	{
		guint i;

		g_memmove(&smdata, &org_smdata, sizeof(SysManageData));
		g_memmove(&logdata, &org_logdata, sizeof(LogoutData));
		g_memmove(&sys_info, &org_infodata, sizeof(SysInfoData));
		nfui_nflabel_set_text(sm_obj[SM_SYSTEM_ID], sys_info.sysId);

		nfui_spin_button_set_index_no_expose(sm_obj[SM_PASSWORD], smdata.enPassword);
		nfui_spin_button_set_index_no_expose(sm_obj[SM_PASSWORD_EXPIRED], smdata.pwExpired);
		nfui_spin_button_set_index_no_expose(sm_obj[SM_PASSWORD_CHANGED], smdata.pwChanged);
		nfui_spin_button_set_index_no_expose(sm_obj[SM_AUTO_LOGOUT], logdata.autoLogout);
		nfui_spin_button_set_index_no_expose(sm_obj[SM_DURATION], prvDurationToIndex(logdata.duration));
		nfui_spin_button_set_index_no_expose(sm_obj[SM_POE_LIMIT], prvPoeToIndex(smdata.poeLimt));
#if defined(GUI_16CH_SUPPORT) && defined(_SUPPORT_HUB)
		nfui_spin_button_set_index_no_expose(sm_obj[SM_POE_HUB_LIMIT], prvPoeHubToIndex(smdata.poeHubLimt));
#endif
		nfui_spin_button_set_index_no_expose(sm_obj[SM_SIGNAL_TYPE], smdata.sigType);
		if (support_swmode) {
			nfui_spin_button_set_index_no_expose(sm_obj[SM_SWITCH_MODE], smdata.swmode);
		}
#if defined(_SUPPORT_POE_TR_MODE)
		nfui_spin_button_set_index_no_expose(sm_obj[SM_POE_MODE], smdata.poemode);
#endif
#if defined(_SUPPORT_REMOTE_UPGRADE)
		nfui_spin_button_set_index_no_expose(g_remotefw_obj, prvRemoteFWCheckToIndex(smdata.remotefw_check));
#endif

		if (smdata.enPassword)
			nfui_nfobject_enable(sm_obj[SM_PASSWORD_EXPIRED]);
		else
			nfui_nfobject_disable(sm_obj[SM_PASSWORD_EXPIRED]);

		if (logdata.autoLogout)
			nfui_nfobject_enable(sm_obj[SM_DURATION]);
		else
			nfui_nfobject_disable(sm_obj[SM_DURATION]);

	}

	return FALSE;
}
