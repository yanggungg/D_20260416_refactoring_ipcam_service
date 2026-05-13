/*
 * vw_qc_test.c
 *        - dependency :
 *                   
 *
 * Written by JungKyu Park. <parangi@itxsecurity.com>
 * Copyright (c) ITX security, Dec 12, 2012
 *
 */

 
#include <string.h>

#include "nf_afx.h"
#include "nf_common.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"

#include "tools/nf_ui_tool.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfcombobox.h"
#include "viewers/objects/nfbutton.h"
#include "viewers/objects/nflistbox.h"

#include "vw.h"
#include "vw_qc_test.h"

#include "scm.h"
#include "ix_mem.h"

#define QC_TEST_SIZE_W			(900)
#define QC_TEST_SIZE_H			(910)
#define QC_TEST_POS_X			(DISPLAY_ACTIVE_WIDTH-192-20-QC_TEST_SIZE_W)
#define QC_TEST_POS_Y			((DISPLAY_ACTIVE_HEIGHT-108-QC_TEST_SIZE_H)/2)

#define MSG_LIST_SIZE_W         (QC_TEST_SIZE_W-20)
#define MSG_LIST_SIZE_H         (QC_TEST_SIZE_H-100)

#define FUNC_KEY_MAX    (12)


typedef struct _QC_TEST
{
    NFOBJECT            *key_obj;
    gchar               label[16];
    QC_TEST_CB_FUNC	    cb_func;
	gpointer	        cb_data;    
} QC_TEST;

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_msglist_obj = 0;
static QC_TEST g_qc[FUNC_KEY_MAX] = {0, };


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

static gint _get_model_name(gchar *strBuf)
{
    SysInfoData sys_info;

	memset(&sys_info, 0, sizeof(SysInfoData));
	
	DAL_get_sysInfo_data(&sys_info);
    g_sprintf(strBuf, "MODEL : %s", sys_info.model);   

    return 0;
}

static gint _get_fw_verion(gchar *strBuf)
{
    gchar strTemp[40];

    var_get_fake_fwver(strTemp, 40);
    g_sprintf(strBuf, "FW VERSION : %s", strTemp);   

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
    g_sprintf(strBuf, "LAST FW UPDATE DATE : %s", strTemp);

    return 0;
}

static gint _get_hw_verion(gchar *strBuf)
{
    gchar strTemp[40];

    memset(strTemp, 0x00, sizeof(strTemp));
   	scm_get_hw_ver(strTemp, 40);
    g_sprintf(strBuf, "HW VERSION : %s", strTemp);

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
    g_sprintf(strBuf, "DISK CAPACITY : %s", strTemp);

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
	g_sprintf(strBuf, "DISK USAGE : %s", strTemp);

    return 0;
}

static gint _get_disk_num(gchar *strBuf)
{
    gchar strTemp[40];
	gint cnt = scm_get_disk_count();

    memset(strTemp, 0x00, sizeof(strTemp));
	sprintf(strTemp, "%d", cnt);
    g_sprintf(strBuf, "NUMBER OF DISKS : %s", strTemp);

    return 0;
}

static gint _get_ip_addr(gchar *strBuf)
{
    gchar strTemp[40];

    memset(strTemp, 0x00, sizeof(strTemp));
	if(scm_get_ip_addr_str(strTemp, 40) < 0)
		sprintf(strTemp, "%d.%d.%d.%d", 0, 0, 0, 0);
    g_sprintf(strBuf, "IP ADDRESS : %s", strTemp);

    return 0;
}

static gint _get_mac_addr(gchar *strBuf)
{
    SysInfoData sys_info;

	memset(&sys_info, 0, sizeof(SysInfoData));
	DAL_get_sysInfo_data(&sys_info);	
    g_sprintf(strBuf, "MAC ADDRESS : %s", sys_info.macAddr);

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
    g_sprintf(strBuf, "RESOLUTION : %s (%s)", uStr, strRes);
	g_free(uStr);

    return 0;
}

static void _display_model_info(gpointer data)
{
    gchar strBuf[128];

    vw_qc_test_info_erase();

    memset(strBuf, 0x00, sizeof(strBuf));
    _get_model_name(strBuf);
    vw_qc_test_info_add_line(strBuf, strlen(strBuf));

    memset(strBuf, 0x00, sizeof(strBuf));
    _get_fw_verion(strBuf);
    vw_qc_test_info_add_line(strBuf, strlen(strBuf));

#if 0
    memset(strBuf, 0x00, sizeof(strBuf));
    _get_last_fw_date(strBuf);
    vw_qc_test_info_add_line(strBuf, strlen(strBuf));
#endif 

    memset(strBuf, 0x00, sizeof(strBuf));
    _get_hw_verion(strBuf);
    vw_qc_test_info_add_line(strBuf, strlen(strBuf));

    memset(strBuf, 0x00, sizeof(strBuf));
    _get_disk_capacity(strBuf);
    vw_qc_test_info_add_line(strBuf, strlen(strBuf));

    memset(strBuf, 0x00, sizeof(strBuf));
    _get_disk_usage(strBuf);
    vw_qc_test_info_add_line(strBuf, strlen(strBuf));

    memset(strBuf, 0x00, sizeof(strBuf));
    _get_disk_num(strBuf);
    vw_qc_test_info_add_line(strBuf, strlen(strBuf));

    memset(strBuf, 0x00, sizeof(strBuf));
    _get_ip_addr(strBuf);
    vw_qc_test_info_add_line(strBuf, strlen(strBuf));

    memset(strBuf, 0x00, sizeof(strBuf));
    _get_mac_addr(strBuf);
    vw_qc_test_info_add_line(strBuf, strlen(strBuf));

#if 0
    memset(strBuf, 0x00, sizeof(strBuf));
    _get_rtsp_port(strBuf);
    vw_qc_test_info_add_line(strBuf, strlen(strBuf));

    memset(strBuf, 0x00, sizeof(strBuf));
    _get_web_port(strBuf);
    vw_qc_test_info_add_line(strBuf, strlen(strBuf));    
#endif

    memset(strBuf, 0x00, sizeof(strBuf));
    _get_resolution(strBuf);
    vw_qc_test_info_add_line(strBuf, strlen(strBuf));

	// 2012-12-13 ���� 9:11:04 choissi
	snprintf(strBuf, sizeof(strBuf), "HW PARAM(PANEL TYPE) : %s",nf_api_param_hw_get_front_type());
    vw_qc_test_info_add_line(strBuf, strlen(strBuf));    
	snprintf(strBuf, sizeof(strBuf), "HW PARAM(RC TYPE) : %s",nf_api_param_hw_get_rc_type());
    vw_qc_test_info_add_line(strBuf, strlen(strBuf));   
	// snprintf(strBuf, sizeof(strBuf), "HW PARAM(UBL CRC) : %s",nf_sysman_get_ubl_crc());
    // vw_qc_test_info_add_line(strBuf, strlen(strBuf));	
	snprintf(strBuf, sizeof(strBuf), "HW PARAM(FPGA CRC) : %s",nf_api_param_hw_get_fpga_crc());
    vw_qc_test_info_add_line(strBuf, strlen(strBuf));

    vw_qc_test_info_update();
}

static gboolean _post_func_key_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
    
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        for (i = 0; i < FUNC_KEY_MAX; i++)
        {
            if (g_qc[i].key_obj == obj) break;
        }

        if (i == FUNC_KEY_MAX) return FALSE;
        if (!g_qc[i].cb_func) return FALSE;
      
        g_qc[i].cb_func(g_qc[i].cb_data);
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
            vw_qc_test_hide();
		}
		break;		
	
		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
		{	
			kevt = (GdkEventKey*)evt;
			kpid = (KEYPAD_KID)kevt->keyval;

			if (kpid == KEYPAD_EXIT)
			{		
                vw_qc_test_hide();
                return TRUE;
   			}
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

static gboolean _post_clear_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;

	if (evt->type == GDK_BUTTON_RELEASE) 
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        vw_qc_test_info_erase();
    }

	return FALSE;
}

static gboolean _post_exit_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;

	if (evt->type == GDK_BUTTON_RELEASE) 
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        vw_qc_test_hide();
	}

	return FALSE;
}

static gboolean returnkey_proc(NFOBJECT *top, GdkEvent *event, gpointer data)
{

	return FALSE;
}

gint vw_qc_test_create(NFWINDOW *parent)
{
	NFOBJECT *win;
	NFOBJECT *main_fixed;
	NFOBJECT *obj;
    
	guint lc_size[] = {MSG_LIST_SIZE_W, };
    gchar strBuf[10];
    gint pos_x, pos_y, i;
    gint size_w, size_h;

	win = (NFOBJECT*)nfui_nfwindow_new(parent, QC_TEST_POS_X, QC_TEST_POS_Y, QC_TEST_SIZE_W, QC_TEST_SIZE_H);
	nfui_nfobject_modify_bg(win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_regi_post_event_callback(win, _post_qc_test_window_event_cb);
	nfui_nfwindow_use_outside_evt((NFWINDOW*)win, TRUE);
	nfui_nfwindow_set_mask((NFWINDOW*)win, GDK_BUTTON_PRESS, TRUE);
	nfui_nfwindow_set_returnkey_proc((NFWINDOW*)win, returnkey_proc);
	g_curwnd = win;

	main_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(main_fixed, QC_TEST_SIZE_W, QC_TEST_SIZE_H);
	nfui_nfobject_show(main_fixed);

    pos_x = 10;
    pos_y = 10;

	nfui_get_image_size(IMG_N_SCROLL_UP, &size_w, &size_h);

    lc_size[0] -= size_w;
	
	obj = nfui_listbox_new(1, lc_size, 40);	
	nfui_listbox_set_skin_type(NF_LISTBOX(obj), NFLISTBOX_TYPE_POPUP_1);
	nfui_listbox_set_pango_font(NF_LISTBOX(obj), nffont_get_pango_font(NFFONT_LARGE_SEMI));
    nfui_listbox_set_draw_inline(NF_LISTBOX(obj), TRUE, COLOR_IDX(200));	
    nfui_listbox_support_multi_lang(NF_LISTBOX(obj), FALSE);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_ON);
	nfui_nfobject_set_size(obj, MSG_LIST_SIZE_W, MSG_LIST_SIZE_H);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
    g_msglist_obj = obj;

    pos_y += (MSG_LIST_SIZE_H + 4);
    
    for (i = 0; i < FUNC_KEY_MAX; i++)
    {
        memset(strBuf, 0x00, sizeof(strBuf));

        if (strlen(g_qc[i].label))
            strcpy(strBuf, g_qc[i].label);
        else
            g_sprintf(strBuf, "F%d", i+1);

    	obj = nftool_normal_button_create_popup_type1(strBuf, 120);
    	nfui_nfobject_support_multi_lang(obj, FALSE);    	
    	nfui_nfobject_show(obj);
    	nfui_regi_post_event_callback(obj, _post_func_key_event_cb);    	
    	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
        g_qc[i].key_obj = obj;
        
        pos_x += 124;

        if (i == 5)
        {
            pos_x = 10;
            pos_y += 44;
        }
    }

    pos_x += 16;
    pos_y = 10 + (MSG_LIST_SIZE_H + 4);

	obj = nftool_normal_button_create_type3("CLEAR", 120);
	nfui_nfobject_support_multi_lang(obj, FALSE);
	nfui_nfobject_show(obj);
   	nfui_regi_post_event_callback(obj, _post_clear_event_cb);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);   

    pos_y += 44;

	obj = nftool_normal_button_create_type3("EXIT", 120);
	nfui_nfobject_support_multi_lang(obj, FALSE);
	nfui_nfobject_show(obj);
   	nfui_regi_post_event_callback(obj, _post_exit_event_cb);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);   

	nfui_nfwindow_add((NFWINDOW*)win, main_fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_hide(win);

    _display_model_info(0);

    if (!g_qc[0].cb_func)
        vw_qc_test_connect_func(0, "INFO", _display_model_info, NULL);

    _set_login_user(-1);

    uxm_reg_imsg_event(win, INFY_QC_TEST_SET_BUTTON);
    uxm_reg_imsg_event(win, INFY_QC_TEST_INFO_ADD);
    uxm_reg_imsg_event(win, INFY_QC_TEST_INFO_CLEAR);

    uxm_monitor_on_imsg_event(win, INFY_QC_TEST_SET_BUTTON);
    uxm_monitor_on_imsg_event(win, INFY_QC_TEST_INFO_ADD);    
    uxm_monitor_on_imsg_event(win, INFY_QC_TEST_INFO_CLEAR);
    
	return 0;
}

gint vw_qc_test_show()
{
    _set_login_user(-1);

	nfui_nfobject_show((NFOBJECT*)g_curwnd);
	nfui_make_key_hierarchy(g_curwnd);
	nfui_page_open(PGID_QC_TEST, (NFOBJECT*)g_curwnd, ssm_get_cur_id(NULL));

	return 0;	
}

gint vw_qc_test_hide()
{
	nfui_nfobject_hide((NFOBJECT*)g_curwnd);
	nfui_page_close(PGID_QC_TEST, (NFOBJECT*)g_curwnd);

	return 0;	
}

gboolean vw_qc_test_is_shown()
{
	return nfui_nfobject_is_shown((NFOBJECT*)g_curwnd);
}

gint vw_qc_test_connect_func(gint keyIdx, gchar *label, QC_TEST_CB_FUNC cb_func, gpointer cb_param)
{
    if ((keyIdx < 0) || (keyIdx >= FUNC_KEY_MAX)) return -1;

    if (label)
    {
        strcpy(g_qc[keyIdx].label, label);

        if (g_qc[keyIdx].key_obj)
            evt_send_to_local(INFY_QC_TEST_SET_BUTTON, keyIdx, 0, 0);
    }

    g_qc[keyIdx].cb_func = cb_func;
    g_qc[keyIdx].cb_data = cb_param;

    return 0;
}

gint vw_qc_test_info_add_line(gchar *str, gint len)
{
    gchar *pBuf;

    if (!str) return -1;
    if (len == 0) return -1;

    pBuf = imalloc(sizeof(gchar)*len+1);
    strcpy(pBuf, str);

    if (g_msglist_obj)   
        evt_send_to_local(INFY_QC_TEST_INFO_ADD, 0, 0, pBuf);
   
    return 0;
}

gint vw_qc_test_info_update()
{
    if (g_msglist_obj)
        evt_send_to_local(INFY_QC_TEST_INFO_UPDATE, 0, 0, 0);

	return 0;
}

gint vw_qc_test_info_erase()
{
    if (g_msglist_obj)
        evt_send_to_local(INFY_QC_TEST_INFO_CLEAR, 0, 0, 0);
        
    return 0;
}

