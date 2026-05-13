#include "nf_afx.h"

#include "nf_ptz.h"
#include "nf_api_live.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_function.h"
#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"

#include "iux_afx.h"
#include "evt.h"
#include "uxm.h"
#include "scm.h"
#include "ssm.h"
#include "smt.h"
#include "iux_msg.h"
#include "nf_util_fw.h"
#include "vw.h"
#include "vw_progress_fwupdate_128MB.h"

enum {
	UPGRADE_BY_NET = 0,
	UPGRADE_BY_NEWFW = 1,
    UPGRADE_BY_CHEATKEY = 2
}; 



static NFOBJECT* prog_pop = NULL;
static NFWINDOW *g_curwnd = 0;

static gint g_upgrade_mode = -1;
static guint g_buzzer_tid = 0;
static gboolean _is_buzzer_on = FALSE;

static DETECT_VERINFO_T g_detected_verinfo;


static gboolean _off_buzzer(gpointer data)
{
    scm_buzzer_off();
    return FALSE;
}

static gboolean _onoff_buzzer(gpointer data)
{
    gint dwell;

    if (data) dwell = GPOINTER_TO_INT(data);

    scm_buzzer_on();
    g_timeout_add(dwell, _off_buzzer, NULL);    
    return TRUE;
}

static gboolean _blink_led(gpointer data)
{
    static gint onoff = 0;

	if (onoff) {
	    scm_turn_on_led_all();
		onoff = 0;
	} 
	else {
	    scm_turn_off_led_all();
		onoff = 1;
	}

    return TRUE;
}

static gint _do_ready_auto_fwupgrade()
{
	char            mount_path[128];
	char            full_name[256];
	gint 		    media_cnt = 0;
	gint            cnt = -1;
	gint            fw_cnt;
	gchar           **fw_list;
	gint pos_x, pos_y;
	MEDIA_INFO_T	*media_info;
	guint i;

	media_info = scm_new_media_list(&media_cnt);

	for (i = 0; i < media_cnt; i++)
	{
		if (scm_get_media_type(media_info[i].id) == MTYPE_USB)
		{
			cnt = i;
			break;
		}
	}

	if (cnt < 0)
	{
		g_warning("%s, %d, media_cnt:%d, can't find usb device.", __FUNCTION__, __LINE__, media_cnt);
		nftool_mbox_auto(g_curwnd, 2, "NOTICE", "Can't find usb device.");
		return -1;
	}

	scm_get_mounted_path(media_info[cnt].id, mount_path, sizeof(mount_path));
	fw_list = scm_new_fw_list(media_info[cnt].id, 
			FF_FNAME_EXT | FF_MODEL | FF_BUYERCODE | FF_UPPERVER, FO_DEFAULT, &fw_cnt);

	if ((fw_cnt <= 0) || (fw_list == NULL))
	{
		g_warning("%s, %d, fw_cnt:%d, cann't find upgrade file.", __FUNCTION__, __LINE__, fw_cnt);
		nftool_mbox_auto(g_curwnd, 2, "NOTICE", "Can't find upgrade file.");
		return -2;
	}

	g_sprintf(full_name, "%s/%s", mount_path, (fw_list[0]));

	g_message("%s, %d, fullname: %s", __FUNCTION__, __LINE__, full_name);
	
	pos_x = (DISPLAY_ACTIVE_WIDTH-653)/2;
	pos_y = (DISPLAY_ACTIVE_HEIGHT-106)/2; 

	prog_pop = nftool_prog_pop_open(g_curwnd, "F/W UPGRADE", FALSE, pos_x, pos_y, 0);

	smt_set_service(SMT_FW_UPGRADE);
	scm_upgrade_fw(full_name, IRET_SCM_UPGRADE_FW);

	return 0;	
}

static gboolean pre_main_wnd_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint result;
	gint pos_x, pos_y;
	mb_type ret;

	switch(evt->type)
	{
		case IREQ_NET_FWUP:
		{
			ret = nftool_mbox2_auto_okcancel(g_curwnd, 10,
			                                "CONFIRM",
			                                "It starts to upgrade via network remotely.",
			                                "Do you want to continue?","\n",
			                                "If no action, system upgrade within %d sec.");

			if(ret == NFTOOL_MB_OK)
			{
				smt_set_service(SMT_NET_FW_UPGRADE);

				pos_x = (DISPLAY_ACTIVE_WIDTH-653)/2;
				pos_y = (DISPLAY_ACTIVE_HEIGHT-106)/2;
				cmm_send_message(CMMPT_SCM, IRPL_NET_FWUP, NET_FW_APPLY, 0, 0);
				prog_pop = nftool_prog_pop_open(g_curwnd, "F/W UPGRADE", FALSE, pos_x, pos_y, 0);

				g_upgrade_mode = UPGRADE_BY_NET;
			}
			else if(ret == NFTOOL_MB_CANCEL)
			{
				cmm_send_message(CMMPT_SCM, IRPL_NET_FWUP, NET_FW_REJECT, 0, 0);
			}
		}
		break;

		case IREQ_SCM_CONFIRM_FWUP_BY_WEB:
	    {
	        SMT_SERVICE_E st;
	        gint ret;

	        st = smt_get_service();

	        if( (st!= SMT_LIVE) && (st != SMT_LOGOUT)) {
	            cmm_send_message(CMMPT_WEB, IRPL_SCM_CONFIRM_FWUP_BY_WEB, -1, 0, 0);
	            return FALSE;
	        }

	        ret = nftool_mbox2_auto_okcancel(g_curwnd, 10, "CONFIRM",
	                                        "It starts to upgrade via network remotely.",
	                                        "Do you want to continue?","\n",
	                                        "If no action, system upgrade within %d sec.");

	        if(ret == NFTOOL_MB_OK) {
	            cmm_send_message(CMMPT_WEB, IRPL_SCM_CONFIRM_FWUP_BY_WEB, 1, 0, 0);
    	            
				g_upgrade_mode = UPGRADE_BY_NET;
	        }
	        else if(ret == NFTOOL_MB_CANCEL) {
	            cmm_send_message(CMMPT_WEB, IRPL_SCM_CONFIRM_FWUP_BY_WEB, 0, 0, 0);
	        }
	    }
		break;

        case IREQ_SCM_128MB_REMOTE_UPDATE_ALLINONE:
	    {
	        SMT_SERVICE_E st;
	        gint ret;

	        st = smt_get_service();

	        ret = nftool_mbox2_auto_okcancel(g_curwnd, 10, "CONFIRM",
	                                        "It starts to upgrade via network remotely.",
	                                        "Do you want to continue?","\n",
	                                        "If no action, system upgrade within %d sec.");

	        if(ret == NFTOOL_MB_OK) {
				evt_send_to_local(INFY_SCM_PREPARE_FWUP_OPEN, UPGRADE_TYPE_WEB, 0, 0);
				cmm_send_message(CMMPT_SCM, IRPL_SCM_128MB_REMOTE_UPDATE_ALLINONE, NET_FW_APPLY, 0, 0);
				g_upgrade_mode = UPGRADE_BY_NET;
	        }
	        else if(ret == NFTOOL_MB_CANCEL) {
				cmm_send_message(CMMPT_SCM, IRPL_SCM_128MB_REMOTE_UPDATE_ALLINONE, NET_FW_REJECT, 0, 0);
	        }
	    }
		break;

		case INFY_CHECK_REMOTE_NEWFW:
		{
			g_message(">>>>>>>>>>>>>>>>> INFY_CHECK_REMOTE_NEWFW %s, %d", __FUNCTION__, __LINE__);

			if (!var_get_enable_remote_upgrade()) return FALSE;
			if (!ssm_check_access_auth(USR_AUTH_SYS_SETUP)) return FALSE;

			scm_check_remote_new_nvrfw();
		}
		break;

		case IREQ_DETECTED_NEWFW:
		{
			DETECT_VERINFO_T *pInfo;
			gchar strBuf[1024];
			gint retVal;
			guint send_msg_ack = NET_FW_REJECT;

			g_message(">>>>>>>>>>>>>>>>> IREQ_DETECTED_NEWFW %s, %d", __FUNCTION__, __LINE__);

			pInfo = (DETECT_VERINFO_T*)(((CMM_MESSAGE_T*)data)->data);

			memset(strBuf, 0x00, sizeof(strBuf));
			strcpy(strBuf, lookup_string("The new firmware has been released.\nDo you want to check the information of the firmware?"));
			strcat(strBuf, lookup_string(" ("));
			strcat(strBuf, lookup_string("NVR"));
			strcat(strBuf, lookup_string(")"));

			ret = nftool_mbox(g_curwnd, "NOTICE", strBuf, NFTOOL_MB_OKCANCEL);

			if (ret == NFTOOL_MB_OK)
			{
				retVal = vw_detect_newfw_popup_open(g_curwnd, pInfo);

				if (retVal)
				{
					ret = nftool_mbox(g_curwnd, "CONFIRM", "Do you want to upgrade firmware?", NFTOOL_MB_OKCANCEL);

					if (ret == NFTOOL_MB_OK)
					{
			    		smt_set_service(SMT_NET_FW_UPGRADE);

						pos_x = (DISPLAY_ACTIVE_WIDTH-653)/2;
						pos_y = (DISPLAY_ACTIVE_HEIGHT-106)/2;
						prog_pop = nftool_prog_pop_open(g_curwnd, "F/W UPGRADE", FALSE, pos_x, pos_y, 0);

						g_upgrade_mode = UPGRADE_BY_NEWFW;
						send_msg_ack = NET_FW_APPLY;
					}
				}
			}

			cmm_send_message(CMMPT_SCM, IRPL_DETECTED_NEWFW, send_msg_ack, 0, 0);
		}
		break;

		case INFY_DETECTED_NEWFW:
		{
			DETECT_VERINFO_T *pInfo;
			gchar strBuf[1024];
			gint retVal;

			g_message(">>>>>>>>>>>>>>>>> INFY_DETECTED_NEWFW %s, %d", __FUNCTION__, __LINE__);

			pInfo = (DETECT_VERINFO_T*)(((CMM_MESSAGE_T*)data)->data);

			memset(strBuf, 0x00, sizeof(strBuf));
			strcpy(strBuf, lookup_string("The new firmware has been released.\nDo you want to check the information of the firmware?"));
			strcat(strBuf, lookup_string(" ("));
			strcat(strBuf, lookup_string("DVR"));
			strcat(strBuf, lookup_string(")"));

			ret = nftool_mbox(g_curwnd, "NOTICE", strBuf, NFTOOL_MB_OKCANCEL);
			if (ret == NFTOOL_MB_CANCEL) return FALSE;

			retVal = vw_detect_newfw_popup_open(g_curwnd, pInfo);
			if (!retVal) return FALSE;

			ret = nftool_mbox(g_curwnd, "CONFIRM", "Do you want to upgrade firmware?", NFTOOL_MB_OKCANCEL);
			if (ret == NFTOOL_MB_CANCEL) return FALSE;

			evt_send_to_local(INFY_SCM_PREPARE_FWUP_OPEN, UPGRADE_TYPE_REMOTESERVER, 0, 0);

			memset(&g_detected_verinfo, 0x00, sizeof(DETECT_VERINFO_T));
			memcpy(&g_detected_verinfo, pInfo, sizeof(DETECT_VERINFO_T));

			scm_prepare_upgrade_validate_check_url(g_detected_verinfo.url, IRET_SCM_PREPARE_FWUP_VALIDATE);

			g_upgrade_mode = UPGRADE_BY_NEWFW;
		}
		break;

#ifdef _SUPPORT_NVR_S1
		case INFY_REMOTE_S1_FWUP_NOTIFY:
		{
			gint retVal;

			g_message(">>>>>>>>>>>>>>>>> INFY_REMOTE_S1_FWUP_NOTIFY %s, %d", __FUNCTION__, __LINE__);

			if (smt_get_service() != SMT_LIVE) return FALSE;
			if (!ssm_check_access_auth(USR_AUTH_SYS_SETUP)) return FALSE;

			retVal = scm_S1_check_remote_new_nvrfw();
			if (retVal == -1) scm_S1_check_remote_new_camfw();
		}
		break;

		case INFY_CHECK_REMOTE_S1_NEWFW:
		{
			gint retVal;

			g_message(">>>>>>>>>>>>>>>>> INFY_CHECK_REMOTE_S1_NEWFW %s, %d", __FUNCTION__, __LINE__);

			if (!ssm_check_access_auth(USR_AUTH_SYS_SETUP)) return FALSE;

			retVal = scm_S1_check_remote_new_nvrfw();
			if (retVal == -1) scm_S1_check_remote_new_camfw();
		}
		break;

		case IREQ_S1_DETECTED_NEWFW:
		{
			DETECT_VERINFO_T *pInfo;
			gchar strBuf[1024];
			gint retVal;
			guint send_msg_ack = NET_FW_REJECT;

			g_message(">>>>>>>>>>>>>>>>> IREQ_S1_DETECTED_NEWFW %s, %d", __FUNCTION__, __LINE__);

			pInfo = (DETECT_VERINFO_T*)(((CMM_MESSAGE_T*)data)->data);

			memset(strBuf, 0x00, sizeof(strBuf));
			strcpy(strBuf, lookup_string("The new firmware has been released.\nDo you want to check the information of the firmware?"));
			strcat(strBuf, lookup_string(" ("));
			strcat(strBuf, lookup_string("NVR"));
			strcat(strBuf, lookup_string(")"));

			ret = nftool_mbox(g_curwnd, "NOTICE", strBuf, NFTOOL_MB_OKCANCEL);

			if (ret == NFTOOL_MB_OK)
			{
				retVal = vw_detect_newfw_popup_open(g_curwnd, pInfo);

				if (retVal)
				{
					ret = nftool_mbox(g_curwnd, "CONFIRM", "Do you want to upgrade firmware?", NFTOOL_MB_OKCANCEL);

					if (ret == NFTOOL_MB_OK)
					{
			    		smt_set_service(SMT_NET_FW_UPGRADE);

						pos_x = (DISPLAY_ACTIVE_WIDTH-653)/2;
						pos_y = (DISPLAY_ACTIVE_HEIGHT-106)/2;
						prog_pop = nftool_prog_pop_open(g_curwnd, "F/W UPGRADE", FALSE, pos_x, pos_y, 0);

						g_upgrade_mode = UPGRADE_BY_NEWFW;
						send_msg_ack = NET_FW_APPLY;
					}
				}
			}

			cmm_send_message(CMMPT_SCM, IRPL_S1_DETECTED_NEWFW, send_msg_ack, 0, 0);

			if (send_msg_ack == NET_FW_REJECT)
			{
				scm_S1_check_remote_new_camfw();
			}
		}
		break;

		case IREQ_S1_DETECTED_NEWCAMFW:
		{
			DETECT_VERINFO_T *pInfo;
			CAM_PROFILE_T profile[GUI_CHANNEL_CNT];
			gchar strBuf[1024];
			gint i;
			guint retMask = 0;
			guint send_msg_ack = NET_FW_REJECT;

			g_message(">>>>>>>>>>>>>>>>> IREQ_S1_DETECTED_NEWCAMFW %s, %d", __FUNCTION__, __LINE__);

			pInfo = (DETECT_VERINFO_T*)(((CMM_MESSAGE_T*)data)->data);

			for (i = 0; i < GUI_CHANNEL_CNT; i++)
			{
				scm_get_cam_profile(i, &profile[i]);
			}

			memset(strBuf, 0x00, sizeof(strBuf));
			strcpy(strBuf, lookup_string("The new firmware has been released.\nDo you want to check the information of the firmware?"));
			strcat(strBuf, lookup_string(" ("));
			strcat(strBuf, lookup_string("IPCAM"));
			strcat(strBuf, lookup_string(")"));

			ret = nftool_mbox(g_curwnd, "NOTICE", strBuf, NFTOOL_MB_OKCANCEL);

			if (ret == NFTOOL_MB_OK)
			{
				retMask = vw_detect_newcamfw_popup_open(g_curwnd, pInfo, profile);

				g_message(">>>>>>>>>>>>>>>>> %s, %d, RETMSK : %08X", __FUNCTION__, __LINE__, retMask);

				if (retMask)
				{
					ret = nftool_mbox(g_curwnd, "CONFIRM", "Do you want to upgrade firmware?", NFTOOL_MB_OKCANCEL);

					if (ret == NFTOOL_MB_OK)
					{
		    			smt_set_service(SMT_NET_FW_UPGRADE);

						VW_IPCamera_UPGrade_Progress_Open(g_curwnd, retMask, 0, 1);
						g_upgrade_mode = UPGRADE_BY_NEWFW;
						send_msg_ack = NET_FW_APPLY;
					}
				}
			}

			cmm_send_message(CMMPT_SCM, IRPL_S1_DETECTED_NEWCAMFW, send_msg_ack, 0, GUINT_TO_POINTER(retMask));
		}
		break;
#endif

		case IREQ_CHEAT_AUTO_FWUP:
		{
			gint ret;

			g_upgrade_mode = UPGRADE_BY_CHEATKEY;
			ret = _do_ready_auto_fwupgrade();

			if (ret == 0) {
				scm_buzzer_on();
				g_timeout_add(1000, _off_buzzer, NULL);
				g_buzzer_tid = g_timeout_add(10000, _onoff_buzzer, GINT_TO_POINTER(1000));
			}
		}
		break;

		case INFY_FWUP_RATE:
		{
			gint rate = ((CMM_MESSAGE_T *)data)->param;

			if (rate < 0)
			{
				nftool_mbox(g_curwnd, "NOTICE", "F/W Upgrade Fail.\nThe system will be reboot soon.", NFTOOL_MB_NONE);
				return FALSE;
			}

			if (prog_pop)
			{
				g_message("firmware upgrade rate = [%d]\n", ((CMM_MESSAGE_T *)data)->param);
				nftool_prog_pop_set_rate(prog_pop, rate);
			}
		}
		break;

		case IRET_SCM_UPGRADE_FW:
		{
			if (g_upgrade_mode == -1) return FALSE;

			if (g_buzzer_tid) {
			    g_source_remove(g_buzzer_tid);
				g_buzzer_tid = 0;
				scm_buzzer_off();
			}

			if (prog_pop) {
				nftool_prog_pop_close(prog_pop);
				prog_pop = NULL;
			}

			result = ((CMM_MESSAGE_T *)data)->param;

			if (g_upgrade_mode == UPGRADE_BY_NET)
			{
				if (result == 0) {
					nftool_mbox(g_curwnd, "NOTICE", "F/W Upgrade Complete.\nThe system will be reboot soon.", NFTOOL_MB_NONE);
				}
				else {
					nftool_mbox(g_curwnd, "NOTICE", "F/W Upgrade Fail.\nThe system will be reboot soon.", NFTOOL_MB_NONE);
				}

			}
			else if (g_upgrade_mode == UPGRADE_BY_NEWFW)
			{
				if (result == 0) {
					nftool_mbox(g_curwnd, "NOTICE", "F/W Upgrade Complete.\nThe system will be reboot soon.", NFTOOL_MB_OK);
				}
				else {
					nftool_mbox(g_curwnd, "NOTICE", "F/W Upgrade Fail.\nThe system will be reboot soon.", NFTOOL_MB_OK);
				}

				scm_reboot_system(RR_FWUP, 0);
			}
			else if (g_upgrade_mode == UPGRADE_BY_CHEATKEY)
			{
				if (result == 0) {
					nftool_mbox(g_curwnd, "NOTICE", "F/W Upgrade Complete.\nThe system will be reboot soon.", NFTOOL_MB_NONE);
					scm_buzzer_on();
					scm_turn_on_led_all();
				}
				else {
					nftool_mbox(g_curwnd, "NOTICE", "F/W Upgrade Fail.\nThe system will be reboot soon.", NFTOOL_MB_NONE);
					scm_buzzer_on();
					scm_turn_on_led_all();
					g_timeout_add(250, _blink_led, NULL);
				}

				scm_reboot_system(RR_FWUP, 5000);
			}
		}
		break;

		case GDK_DELETE:
		{
			uxm_unreg_imsg_event(obj, IREQ_NET_FWUP);
			uxm_unreg_imsg_event(obj, INFY_FWUP_RATE);
			uxm_unreg_imsg_event(obj, IRET_SCM_UPGRADE_FW);
			uxm_unreg_imsg_event(obj, IREQ_CHEAT_AUTO_FWUP);
			uxm_unreg_imsg_event(obj, IREQ_SCM_CONFIRM_FWUP_BY_WEB);
			uxm_unreg_imsg_event(obj, IREQ_SCM_128MB_REMOTE_UPDATE_ALLINONE);

			if (strcmp(nf_sysdb_get_str_nocopy("sys.info.vendor"), "S1") == 0)
			{
				uxm_unreg_imsg_event(obj, INFY_REMOTE_S1_FWUP_NOTIFY);
				uxm_unreg_imsg_event(obj, INFY_CHECK_REMOTE_S1_NEWFW);
				uxm_unreg_imsg_event(obj, IREQ_S1_DETECTED_NEWFW);
				uxm_unreg_imsg_event(obj, IREQ_S1_DETECTED_NEWCAMFW);
			}
			else
			{
				uxm_unreg_imsg_event(obj, INFY_CHECK_REMOTE_NEWFW);
				uxm_unreg_imsg_event(obj, IREQ_DETECTED_NEWFW);
				uxm_unreg_imsg_event(obj, INFY_DETECTED_NEWFW);
			}
		}
		break;

		default:
		break;
	}
	return FALSE;
}

void VW_Net_FWUpgrade_Open(NFWINDOW *parent)
{
	NFWINDOW *win;

	win = (NFOBJECT*)nfui_nfwindow_new(parent, -1, -1, 0, 0);
	g_curwnd = win;
	nfui_nfwindow_set_title(win, "NETWORK FW UPGRADE");
	nfui_regi_pre_event_callback((NFOBJECT*)win, pre_main_wnd_event_handler);
	nfui_run_main_event_handler(win);

	uxm_reg_imsg_event(win, IREQ_NET_FWUP);
	uxm_reg_imsg_event(win, INFY_FWUP_RATE);
	uxm_reg_imsg_event(win, IRET_SCM_UPGRADE_FW);
	uxm_reg_imsg_event(win, IREQ_CHEAT_AUTO_FWUP);
	uxm_reg_imsg_event(win, IREQ_SCM_CONFIRM_FWUP_BY_WEB);
	uxm_reg_imsg_event(win, IREQ_SCM_128MB_REMOTE_UPDATE_ALLINONE);
	

	uxm_monitor_on_imsg_event(win, IREQ_NET_FWUP);	
	uxm_monitor_on_imsg_event(win, INFY_FWUP_RATE);	
	uxm_monitor_on_imsg_event(win, IRET_SCM_UPGRADE_FW);	
	uxm_monitor_on_imsg_event(win, IREQ_CHEAT_AUTO_FWUP);	
	uxm_monitor_on_imsg_event(win, IREQ_SCM_CONFIRM_FWUP_BY_WEB);
	uxm_monitor_on_imsg_event(win, IREQ_SCM_128MB_REMOTE_UPDATE_ALLINONE);
	

	if (strcmp(nf_sysdb_get_str_nocopy("sys.info.vendor"), "S1") == 0) 
	{
		uxm_reg_imsg_event(win, INFY_REMOTE_S1_FWUP_NOTIFY);
		uxm_reg_imsg_event(win, INFY_CHECK_REMOTE_S1_NEWFW);
		uxm_reg_imsg_event(win, IREQ_S1_DETECTED_NEWFW);	
		uxm_reg_imsg_event(win, IREQ_S1_DETECTED_NEWCAMFW);	
		
		uxm_monitor_on_imsg_event(win, INFY_REMOTE_S1_FWUP_NOTIFY);
		uxm_monitor_on_imsg_event(win, INFY_CHECK_REMOTE_S1_NEWFW);
		uxm_monitor_on_imsg_event(win, IREQ_S1_DETECTED_NEWFW);		
		uxm_monitor_on_imsg_event(win, IREQ_S1_DETECTED_NEWCAMFW);		
	}
	else
	{
		uxm_reg_imsg_event(win, INFY_CHECK_REMOTE_NEWFW);
		uxm_reg_imsg_event(win, IREQ_DETECTED_NEWFW);		
		uxm_reg_imsg_event(win, INFY_DETECTED_NEWFW);

		uxm_monitor_on_imsg_event(win, INFY_CHECK_REMOTE_NEWFW);
		uxm_monitor_on_imsg_event(win, IREQ_DETECTED_NEWFW);		
		uxm_monitor_on_imsg_event(win, INFY_DETECTED_NEWFW);
	}
}

