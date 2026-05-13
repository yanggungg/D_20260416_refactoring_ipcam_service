#include "nf_afx.h"

#include "tools/nf_ui_tool.h"
#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/util.h"
#include "support/color.h"
#include "support/nf_ui_page_manager.h"

#include "objects/nfobject.h"
#include "objects/nfbutton.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfimglabel.h"
#include "objects/nfcombobox.h"
#include "objects/nfspinbutton.h"
#include "objects/cw_slider.h"
#include "objects/nfimage.h"

#include "nf_api_ipcam.h"
#include "nf_api_cam.h"
#include "scm.h"
#include "vsm.h"
#include "ix_mem.h"
#include "smt.h"

#include "vw.h"
#include "vw_vkeyboard.h"

#include "vw_sys_camera_ipcam_dconfig.h"
#include "vw_sys_camera_ipcam_dconfig_mode.h"


#define DCONFIG_WIN_SIZE_W      (1300)
#define DCONFIG_WIN_SIZE_H      (420)

#define STR_SERVICE(use_ssl)   ((use_ssl == 1) ? "https://" : "http://")

#define PAGE_STR1   "You can configure the IP cameras directly by the web access."
#define PAGE_STR2   "- Video recording has been stopped automatically -"

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_model_obj = 0;

static gboolean _post_exit_btn_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *topwin;
    static NFOBJECT *wait_box = 0;    

	if(event->type == GDK_BUTTON_RELEASE)
	{
		if(event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        uxm_reg_imsg_event(obj, IRET_SCM_DCONFIG_MODE);
        scm_leave_direct_mode(IRET_SCM_DCONFIG_MODE);
            
        if (!wait_box)
            wait_box = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");    
	}
	else if (event->type == IRET_SCM_DCONFIG_MODE)
	{
		if(wait_box) {
			nftool_remove_waitbox(wait_box);
			wait_box = 0;
		}

        uxm_unreg_imsg_event(obj, IRET_SCM_DCONFIG_MODE);

		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);	
	}
	
	return FALSE;
}

static gboolean _post_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		gtk_main_quit();
	}

	return FALSE;
}

static gboolean _post_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if(evt->type == GDK_EXPOSE)
	{
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, size_w, size_h, NFALIGN_LEFT, 0);
    
		nfui_nfobject_gc_unref(gc);
	}
    else if (evt->type == GDK_DELETE)
    {
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
    }

	return FALSE;
}

static gboolean returnkey_proc(NFOBJECT *top, GdkEvent *event, gpointer data)
{
	return FALSE;
}

void VW_Open_DirectConfig_mode_Page(NFOBJECT *parent, gint ch)
{
    NFOBJECT *window;
	NFOBJECT *main_fixed;
	NFOBJECT *obj;

	guint pos_x, pos_y;

	IPSetupData ipdata;
    DDNSData ddnsdata;	
	NF_NETIF_GET_INFO netif_info;

    gchar strBuf[256];
	gchar strTemp[128];
    gchar ex_addr[40];

    guint port_num;
    gint use_ssl;

	window = (NFOBJECT*)nfui_nfwindow_new(parent, (DISPLAY_ACTIVE_WIDTH-DCONFIG_WIN_SIZE_W)/2, 380, DCONFIG_WIN_SIZE_W, DCONFIG_WIN_SIZE_H);
	nfui_regi_post_event_callback(window, _post_win_event_handler);
	nfui_nfwindow_set_returnkey_proc((NFWINDOW*)window, returnkey_proc);
	g_curwnd = window;

	main_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(main_fixed, DCONFIG_WIN_SIZE_W, DCONFIG_WIN_SIZE_H);
	nfui_nffixed_put((NFFIXED*)window, main_fixed, 0, 0);
	nfui_nfobject_show(main_fixed);
	nfui_regi_post_event_callback(main_fixed, _post_main_fixed_event_handler);

    pos_x = 20;
    pos_y = 2;

	obj = nfui_nflabel_new_with_pango_font("DIRECT CONFIGURATION MODE", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(30));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, DCONFIG_WIN_SIZE_W-40, 40);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

    pos_y += 100;

	obj = nfui_nflabel_new_with_pango_font(PAGE_STR1, nffont_get_pango_font(NFFONT_XLARGE_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, DCONFIG_WIN_SIZE_W-40, 60);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

    pos_y += 60;

	obj = nfui_nflabel_new_with_pango_font(PAGE_STR2, nffont_get_pango_font(NFFONT_LARGE_SEMI_1), COLOR_PRG_IDX(UX_COLOR_FF0000));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, DCONFIG_WIN_SIZE_W-40, 60);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

    pos_y += 90;

	obj = nfui_nflabel_new_with_pango_font("IP Address (Internal) :", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 500, 40);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

	obj = nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 600, 40);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+500, pos_y);

	DAL_get_ipSetup_data(&ipdata);
	DAL_get_ddns_data(&ddnsdata);
	nf_netif_get_info(&netif_info);

    port_num = nf_ipcam_get_dconf_port(ch);
    use_ssl = nf_ipcam_is_using_ssl(ch);

	memset(strBuf, 0x00, sizeof(strBuf));  
	memset(strTemp, 0x00, sizeof(strTemp));
	
    strcpy(strBuf, STR_SERVICE(use_ssl));
	g_sprintf(strTemp, "%d.%d.%d.%d:%d", ((netif_info.ipaddr & 0xff000000)>>24), ((netif_info.ipaddr & 0xff0000)>>16),
                        			((netif_info.ipaddr & 0xff00)>>8), (netif_info.ipaddr & 0xff), port_num);
    strcat(strBuf, strTemp);
	if (ipdata.dhcp) strcat(strBuf, " (DHCP)");
    
    nfui_nflabel_set_text((NFLABEL*)obj, strBuf);

    pos_y += 40;

	obj = nfui_nflabel_new_with_pango_font("IP Address (External) :", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 500, 40);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

	obj = nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 600, 40);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+500, pos_y);

    memset(strBuf, 0x00, sizeof(strBuf));
	memset(ex_addr, 0x00, sizeof(ex_addr));

    var_get_external_addr(ex_addr, 40);
        
    if (strlen(ex_addr))
    {
        g_sprintf(strBuf,"%s%s:%d", STR_SERVICE(use_ssl), ex_addr, ipdata.webPort);
        nfui_nflabel_set_text((NFLABEL*)obj, strBuf);
    }
	else {
        g_sprintf(strBuf,"N/A");
        nfui_nflabel_set_text((NFLABEL*)obj, strBuf);
	}

    pos_x = DCONFIG_WIN_SIZE_W-192-20;
    pos_y = DCONFIG_WIN_SIZE_H-44-12;

// parent - item
	obj = nftool_normal_button_create_type2("EXIT", 192);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, _post_exit_btn_event_handler);
	
	// windows show
	nfui_nfwindow_add((NFWINDOW*)window, main_fixed);
	nfui_run_main_event_handler(window);
	nfui_nfobject_show(window);
	nfui_make_key_hierarchy((NFWINDOW*)window);
	nfui_set_key_focus(obj, TRUE);

	smt_set_service(SMT_CAM_DCONFIG);
	nfui_page_open(PGID_CAM_DIRECT_CONFIG_MODE, window, ssm_get_cur_id(NULL));

	gtk_main();

	nfui_page_close(PGID_CAM_DIRECT_CONFIG_MODE, window);
	smt_return_to_previous();	
}

