#include "nf_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_font.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"
#include "support/color.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfimage.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nflabel.h"
#include "objects/nflistbox.h"
#include "objects/nfipeditor.h"

#include "vw_ip_editor_popup.h"
#include "vw_vkeyboard.h"
#include "ix_mem.h"
#include "nf_sysman.h"
#include "vsm.h"

#include "vw_specific_camera_set_popup.h"
#include "nf_util_netif.h"
#include <regex.h>

#define VCAMERA_POPUP_SIZE_WIDTH        (570)
#define VCAMERA_POPUP_SIZE_HEIGHT       (230)

#define VC_INPUT_LB_SIZE_W              (335)

#define MAX_RTSP_STRING_SIZE    (256)

static NFOBJECT *g_curwnd = 0;

static NFOBJECT *g_ipaddr;
static NFOBJECT *g_http_port;
static NFOBJECT *g_rtsp_port;
static NFOBJECT *g_ok_btn;

static gchar g_ip[128];
static gint g_port;

static gint _check_possible_specific_search()
{
	gchar *host;
	gint http_port;
	gint rtsp_port;

	host = nfui_nflabel_get_text((NFLABEL*)g_ipaddr);
	http_port = nfui_nflabel_get_number((NFLABEL*)g_http_port);
	//rtsp_port = nfui_nflabel_get_number((NFLABEL*)g_rtsp_port);

	if (!strlen(host)) return 1;
	else if(http_port == 0) return 2;
	//else if(rtsp_port == 0) return 3;

	return 0;
}

static gboolean post_http_port_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid=KEYPAD_NONE;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{	
		NFOBJECT *top = NULL;
		guint x, y;
		gint numTemp;
		gint numRet;

		if(kpid == KEYPAD_ENTER)
		{
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);

			x += (obj->width)/2 + top->x;
			y += obj->height + top->y;
		}
		else
		{
			if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
				return FALSE;
			}
		
			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += ((GdkEventButton*)evt)->x;
			y += ((GdkEventButton*)evt)->y;
		}

		numTemp = nfui_nflabel_get_number((NFLABEL*)obj);
		numRet = NumberKey_Open(g_curwnd, numTemp, x, y, 65535);

		if (numRet == -1) return FALSE;

		nfui_nflabel_set_number((NFLABEL*)obj, numRet);
		nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

		g_port = numRet;
	}

	return FALSE;
}

static gboolean post_rtsp_port_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid=KEYPAD_NONE;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{	
		NFOBJECT *top = NULL;
		guint x, y;
		gint numTemp;
		gint numRet;

		if(kpid == KEYPAD_ENTER)
		{
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);

			x += (obj->width)/2 + top->x;
			y += obj->height + top->y;
		}
		else
		{
			if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
				return FALSE;
			}
		
			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += ((GdkEventButton*)evt)->x;
			y += ((GdkEventButton*)evt)->y;
		}

		numTemp = nfui_nflabel_get_number((NFLABEL*)obj);
		numRet = NumberKey_Open(g_curwnd, numTemp, x, y, 65535);

		if (numRet == -1) return FALSE;

    	nfui_nflabel_set_number((NFLABEL*)obj, numRet);
    	nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

static gboolean post_host_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint x, y;
	gchar hostname[128];
	gchar *host;

	if(evt->type == GDK_2BUTTON_PRESS)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		nfui_nfobject_get_window_pos(obj, &x, &y);

		x += ((GdkEventButton*)evt)->x;
		y += ((GdkEventButton*)evt)->y;

        	host = VirtualKey_Open((NFWINDOW*)g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), x, y, 128, VKEY_RTSP);

        	if (!host) return FALSE;

        	strcpy(hostname, host);
        	ifree(host);

        	if (strlen(hostname) == 0) {
        	    nftool_mbox((NFWINDOW*)g_curwnd, "ERROR", "Please enter an IP address or host name.", NFTOOL_MB_OK);
        	    return FALSE;
        	}

        	nfui_nflabel_set_text((NFLABEL*)obj, hostname);
        	nfui_signal_emit(obj, GDK_EXPOSE, FALSE);

        	memset(g_ip, 0x00, sizeof(g_ip));
        	strcpy(g_ip, hostname);
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_ENTER)
		{
			nfui_nfobject_get_window_pos(obj, &x, &y);
			y += obj->y + obj->height;
			x += obj->x;

            	host = VirtualKey_Open((NFWINDOW*)g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), x, y, 128, VKEY_RTSP);

            	if (!host) return FALSE;

            	strcpy(hostname, host);
            	ifree(host);

            	if (strlen(hostname) == 0) {
            	    nftool_mbox((NFWINDOW*)g_curwnd, "ERROR", "Please enter an IP address or host name.", NFTOOL_MB_OK);
            	    return FALSE;
            	}

            	nfui_nflabel_set_text((NFLABEL*)obj, hostname);
            	nfui_signal_emit(obj, GDK_EXPOSE, FALSE);

            	memset(g_ip, 0x00, sizeof(g_ip));
            	strcpy(g_ip, hostname);
		}
	}

	return FALSE;
}

static gboolean post_ok_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *topwin;

		gchar host[256];
		gint http_port = 0;
		gint rtsp_port = 0;

        OPENMODE_STATE_VIRTUAL_CAM_ADD state;

        gint ret;

        if(evt->button.button == MOUSE_RIGTH_BUTTON)    return FALSE;

		memset(host, 0x00, sizeof(host));

        strcpy(host, nfui_nflabel_get_text((NFLABEL*)g_ipaddr));
		http_port = nfui_nflabel_get_number((NFLABEL*)g_http_port);
		rtsp_port = nfui_nflabel_get_number((NFLABEL*)g_rtsp_port);

		ret = _check_possible_specific_search();
		switch(ret)
		{
			case 1:
				nftool_mbox(g_curwnd, "NOTICE", "Input IP Address please..", NFTOOL_MB_OK);
				break;
			case 2:
				nftool_mbox(g_curwnd, "NOTICE", "Input Http Port Please ...", NFTOOL_MB_OK);
				break;
			case 3:
				nftool_mbox(g_curwnd, "NOTICE", "Input RTSP Port please..", NFTOOL_MB_OK);
				break;
			default:
				nf_openmode_add_device_manual(host, http_port);
				break;
		}

    }

    return FALSE;
}

static gboolean post_cancel_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *topwin;
        
        if(evt->button.button == MOUSE_RIGTH_BUTTON)    return FALSE;
        
        topwin = nfui_nfobject_get_top(obj);
        nfui_nfobject_destroy(topwin);
    }

    return FALSE;
}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
		gtk_main_quit();
	}

	return FALSE;
}

gboolean VW_specific_camera_set_Popup(NFWINDOW *parent, gint win_x, gint win_y)
{
    NFOBJECT *main_wnd;
    NFOBJECT *nffixed;
    NFOBJECT *obj;

	GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];
    gchar str[32];
    gint i,j;
    gint pos_x, pos_y;
    gint size_w, size_h;
    GSList *slist;

    main_wnd = nftool_create_popup_window(parent, win_x, win_y, VCAMERA_POPUP_SIZE_WIDTH, VCAMERA_POPUP_SIZE_HEIGHT, "SPECIFIC IP / HOST NAME", FALSE);
    nfui_regi_post_event_callback(main_wnd, post_page_event_handler);
    g_curwnd = main_wnd;

    nffixed = ((NFWINDOW*)main_wnd)->child;

    pos_x = 10;
    pos_y = 50;

    obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("IP / HOST NAME", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 200, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x, pos_y);

    pos_x += 200 + 10;

	obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font(g_ip, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(115));
    nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, VC_INPUT_LB_SIZE_W, 40);
    nfui_nflabel_set_skin_type(obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(116));
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x, pos_y);    
    nfui_regi_post_event_callback(obj, post_host_event_handler);
    g_ipaddr = obj;

	// HTTP PORT

    pos_y += 43;
    pos_x = 10;
    
    obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("HTTP PORT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 200, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x, pos_y);

    pos_x += 200 + 10;

    obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(115));
    nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_CENTER, 0);
    nfui_nflabel_use_number((NFLABEL*)obj, TRUE);
    nfui_nfobject_set_size(obj, VC_INPUT_LB_SIZE_W, 40);
    nfui_nflabel_set_skin_type(obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(116));
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x, pos_y);    
    nfui_regi_post_event_callback(obj, post_http_port_event_handler);
    g_http_port = obj;

    nfui_nflabel_set_number((NFLABEL*)obj, g_port);

    // RTSP PORT

    pos_y += 43;
    pos_x = 10;
    
    obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("RTSP PORT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 200, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    //nfui_nfobject_show(obj);
	nfui_nfobject_hide(obj);
    nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x, pos_y);

    pos_x += 200 + 10;

    obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(115));
    nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_CENTER, 0);
    nfui_nflabel_use_number((NFLABEL*)obj, TRUE);
    nfui_nfobject_set_size(obj, VC_INPUT_LB_SIZE_W, 40);
    nfui_nflabel_set_skin_type(obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(116));
    //nfui_nfobject_show(obj);
	nfui_nfobject_hide(obj);
    nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x, pos_y);    
    nfui_regi_post_event_callback(obj, post_rtsp_port_event_handler);
    g_rtsp_port = obj;

    obj = nftool_normal_button_create_popup_type2("ADD", 152);
    nfui_nfobject_show(obj);    
    nfui_nffixed_put((NFFIXED*)nffixed, obj, VCAMERA_POPUP_SIZE_WIDTH - (152*2) - 10 - 2, VCAMERA_POPUP_SIZE_HEIGHT - 45);
    nfui_regi_post_event_callback(obj, post_ok_button_event_handler);
    g_ok_btn = obj;

    obj = nftool_normal_button_create_popup_type2("CLOSE", 152);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)nffixed, obj, VCAMERA_POPUP_SIZE_WIDTH - 152 - 10, VCAMERA_POPUP_SIZE_HEIGHT - 45);
    nfui_regi_post_event_callback(obj, post_cancel_button_event_handler);

    nfui_nfobject_show(main_wnd);
    nfui_make_key_hierarchy(main_wnd);
    
    gtk_main();
   
    return FALSE;
}


