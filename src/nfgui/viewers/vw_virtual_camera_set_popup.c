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

#include "vw_virtual_camera_set_popup.h"
#include "nf_util_netif.h"
#include <regex.h>

#define STRSTR0 " * Two RTSP address can be entered"
#define STRSTR1 " ex) rtsp://192.168.100.50:554/live/main"
#define STRSTR2 "       rtsp://ADMIN:1234@192.168.100.50:554/snl/live/1/1"
#define DEFAULT_RTSP_PORT               554

#define VCAMERA_POPUP_SIZE_WIDTH        (900)
#define VCAMERA_POPUP_SIZE_HEIGHT       (640)

#define VC_INPUT_LB_SIZE_W              ((VCAMERA_POPUP_SIZE_WIDTH - 230) / 2)

#define MAX_RTSP_STRING_SIZE    (256)

typedef struct
{
    gchar path[512];
    gint ch;
    gchar id[32];
    gchar pw[32];
    gchar host[128];
    gint port;
    gchar rtsp_addr[MAX_RTSP_STRING_SIZE];
}RTSP_UNIT_T;

typedef enum {
    RS_SUCCESS = 0,
    
} RS_FAIL_E;

static RTSP_UNIT_T ru1 = {NULL,};
static RTSP_UNIT_T ru2 = {NULL,};
static RTSP_UNIT_T org_ru1 = {0,};
static RTSP_UNIT_T org_ru2 = {0,};

static NFOBJECT *g_curwnd = 0;

static NFOBJECT *g_radio[2];
static NFOBJECT *g_ipaddr;
static NFOBJECT *g_port;
static NFOBJECT *g_id;
static NFOBJECT *g_pw;
static NFOBJECT *g_rtsp1;
static NFOBJECT *g_rtsp2;
static NFOBJECT *g_vname;
static NFOBJECT *g_etc;
static NFOBJECT *g_ok_btn;
static NFOBJECT *g_ch;

static gchar g_numch = 0;
static gchar protocol[8];
static gint is_opened = 0;


static gint check_protocol()
{
    if (strcmp(protocol, "rtsp") != 0)
    {
        g_message("%s, %d FAIL", __FUNCTION__, __LINE__);
        return 0;
    }
    
    return 1;
}

static gint check_path(gchar *path)
{
}

static gint check_port(gint port)
{
    if (port <= 0 || port > 65535)
    {
        g_message("%s, %d FAIL", __FUNCTION__, __LINE__);
        return 0;
    }

    return 1;
}

static gint check_ip(gchar *ip)
{
	gchar *regex = "^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$";
	regex_t ext_regex;
	gint ret;

    g_message("%s, %d IP : %s", __FUNCTION__, __LINE__, ip);
    
	if(regcomp(&ext_regex, regex, REG_EXTENDED) != 0)
	{
        g_message("%s, %d FAIL", __FUNCTION__, __LINE__);
		return 0;
	}

	ret = regexec(&ext_regex, ip, 0, NULL, 0);
	if( !ret )
	{
		g_message("%s, %d SUCCESS", __FUNCTION__, __LINE__);
	}
	else
	{
        g_message("%s, %d FAIL", __FUNCTION__, __LINE__);
		return 0;
	}
	regfree(&ext_regex);

	return 1;
}

static gint check_pw(gchar *pw)
{
}

static gint check_id(gchar *id)
{
}

static gint _check_valid(RTSP_UNIT_T *ru)
{
	gint ret = 0;

	if (!check_protocol()) return -1;
//	if (!check_ip(ru->host)) return -1;
	if (!check_port(ru->port)) return -1;
	
	return 0;
}

static gint _get_rtsp_unit(gchar *addr, RTSP_UNIT_T *ru)
{
    gchar buffer[MAX_RTSP_STRING_SIZE];
    gchar bport[16];

    strncpy(buffer, addr, sizeof(buffer));
    g_message("buffer=<%s>\n", buffer);

    if (!strchr(buffer, '@'))
    {
    	if (sscanf(buffer,"%8[^:/]://%16[^:]:%16[^/]/%s", protocol, ru->host, bport, ru->path) == 4)
    	{
        	g_message("protocol=<%s>, host=<%s>, port=<%s>, path=<%s>\n", protocol, ru->host, bport, ru->path);
        	ru->port = atoi(bport);
    	}
		else if (sscanf(buffer,"%8[^:/]://%16[^/]/%s", protocol, ru->host, ru->path) == 3)
		{
			g_message("protocol=<%s>, host=<%s>, path=<%s>\n", protocol, ru->host, ru->path);
		}
		else
		{
		    g_message("%s, %d, No matching characters\n", __FUNCTION__, __LINE__);
		    return -1;
		}
	}
    else 
	{
		if(sscanf(buffer, "%8[^:/]://%32[^:]:%32[^@]@%32[^:]:%16[^/]/%s", protocol, ru->id, ru->pw, ru->host, bport, ru->path) == 6) 
		{
        	g_message("protocol=<%s>, id=<%s>, pw=<%s>, host=<%s>, port = <%s>, path=<%s>\n", protocol, ru->id, ru->pw, ru->host, bport, ru->path);
        	ru->port = atoi(bport);
    	}
    	else if (sscanf(buffer, "%8[^:/]://%32[^:]:%32[^@]@%32[^/]/%s", protocol, ru->id, ru->pw, ru->host, ru->path) == 5)
    	{
			g_message("protocol=<%s>, id=<%s>, pw=<%s>, host=<%s>, path=<%s>\n", protocol, ru->id, ru->pw, ru->host, ru->path);
		}
		else
		{
		    g_message("%s, %d, No matching characters\n", __FUNCTION__, __LINE__);
		    return -1;
		}
	}
    
    return 0;
}

static gint _check_rtsp_addr_availability(gchar *rtsp_addr)
{
	gchar *regex = "^rts?p://(.*:.*@)?[[:digit:]]{1,3}([.]{1}[[:digit:]]{1,3}){3}(:[[:digit:]]{1,})?.*$";
	regex_t ext_regex;
	gint ret;


	if(regcomp(&ext_regex, regex, REG_EXTENDED) != 0)
	{
		//IPCAM_DBG(WARN, "regular expression is not valid\n");
		return 0;
	}

	ret = regexec(&ext_regex, rtsp_addr, 0, NULL, 0);
	if( !ret )
	{
		//IPCAM_DBG(MINOR, "Match RTSP Address expression\n", index);
	}
	else
	{
		//IPCAM_DBG(WARN, "Not Match RTSP Address expression\n");
		return 0;
	}
	regfree(&ext_regex);

	return 1;
}

static gint _make_rtsp_addr(gchar *buf, gint buf_size, RTSP_UNIT_T *ru)
{
    gchar temp[MAX_RTSP_STRING_SIZE];

    memset(temp, 0x00, sizeof(temp));
    memset(buf, 0x00, buf_size);
    
    g_sprintf(temp, "rtsp://");

    if (ru->id && strlen(ru->id) != 0) g_sprintf(temp, "%s%s:", temp, ru->id);
    if (ru->pw && strlen(ru->pw) != 0) g_sprintf(temp, "%s%s@", temp, ru->pw);
    if (ru->host && strlen(ru->host) != 0) g_sprintf(temp, "%s%s", temp, ru->host);
    if (ru->port != 0) g_sprintf(temp, "%s:%d", temp, ru->port);
    if (ru->path != 0) g_sprintf(temp, "%s/%s", temp, ru->path);
    
    g_sprintf(buf, "%s", temp);

    return 0;
}

static gint _set_rtsp_addr(NFOBJECT* obj, RTSP_UNIT_T *ru)
{
    gchar buf[MAX_RTSP_STRING_SIZE];

    memset(buf, 0x00, sizeof(buf));
    
    _make_rtsp_addr(buf, sizeof(buf), ru);

    nfui_nflabel_set_text((NFLABEL*)obj, buf);
    nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
    
    return 0;
}

static gint _set_obj_enable(gint enable, gint expose)
{
    if (enable)
    {
        nfui_nfobject_enable(g_ipaddr);
        nfui_nfobject_enable(g_port);
        nfui_nfobject_enable(g_id);
        nfui_nfobject_enable(g_pw);
        nfui_nfobject_enable(g_etc);
        nfui_nfobject_enable(g_ch);

        nfui_nflabel_set_skin_type((NFLABEL*)g_rtsp1, NFTEXTBOX_TYPE_POPUP_OUTPUT);
        nfui_nfobject_use_focus(g_rtsp1, FALSE);
        nfui_nflabel_set_skin_type((NFLABEL*)g_rtsp2, NFTEXTBOX_TYPE_POPUP_OUTPUT);
        nfui_nfobject_use_focus(g_rtsp2, FALSE);
        nfui_make_key_hierarchy(g_curwnd);
    }
    else
    {
        nfui_nfobject_disable(g_ipaddr);
        nfui_nfobject_disable(g_port);
        nfui_nfobject_disable(g_id);
        nfui_nfobject_disable(g_pw);
        nfui_nfobject_disable(g_etc);
        nfui_nfobject_disable(g_ch);

        nfui_nflabel_set_skin_type((NFLABEL*)g_rtsp1, NFTEXTBOX_TYPE_POPUP_INPUT);
        nfui_nfobject_use_focus(g_rtsp1, TRUE);
        nfui_nflabel_set_skin_type((NFLABEL*)g_rtsp2, NFTEXTBOX_TYPE_POPUP_INPUT);
        nfui_nfobject_use_focus(g_rtsp2, TRUE);
        nfui_make_key_hierarchy(g_curwnd);
    }

    if (expose)
    {
        nfui_signal_emit(g_ipaddr, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_port, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_id, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_pw, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_etc, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_ch, GDK_EXPOSE, TRUE);

        nfui_signal_emit(g_rtsp1, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_rtsp2, GDK_EXPOSE, TRUE);
    }
}

static gboolean _post_ch_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint ch;
    
	if(evt->type == NFEVENT_COMBOBOX_CHANGED) 
	{
	   ch = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
	   ru1.ch = ch;
	   ru2.ch = ch;
	}

	return FALSE;
}

static gboolean post_onoff_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_RADIO_GET_FOCUS) 
	{
	    gint sel;

	    sel = (gint)nfui_radio_button_get_toggled((NFBUTTON*)g_radio[0]);

	    _set_obj_enable(sel, 1);
	}

	return FALSE;
}

static gboolean post_port_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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

        ru1.port = numRet;
        ru2.port = numRet;
        _set_rtsp_addr(g_rtsp1, &ru1);
        _set_rtsp_addr(g_rtsp2, &ru2);
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

		strcpy(ru1.host, hostname);
		strcpy(ru2.host, hostname);
		_set_rtsp_addr(g_rtsp1, &ru1);
		_set_rtsp_addr(g_rtsp2, &ru2);
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

    		strcpy(ru1.host, hostname);
    		strcpy(ru2.host, hostname);
    		_set_rtsp_addr(g_rtsp1, &ru1);
    		_set_rtsp_addr(g_rtsp2, &ru2);
		}
	}

	return FALSE;
}

static gboolean post_path_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_2BUTTON_PRESS)
    {
        gchar *text = NULL;
        guint x, y;
        
        if(evt->button.button == MOUSE_RIGTH_BUTTON)    return FALSE;

        nfui_nfobject_get_window_pos(obj, &x, &y);

        x += ((GdkEventButton*)evt)->x;
        y += ((GdkEventButton*)evt)->y;

        text = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text(obj), x, y, MAX_RTSP_STRING_SIZE, VKEY_RTSP);

        if(text)
        {
            nfui_nflabel_set_text((NFLABEL*)obj, text);
            strcpy(ru1.path, text);
            strcpy(ru1.path, text);
            _set_rtsp_addr(g_rtsp1, &ru1);
            _set_rtsp_addr(g_rtsp2, &ru2);

            ifree(text);
            text = NULL;
        }

        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_text_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_2BUTTON_PRESS)
    {
        gchar *text = NULL;
        guint x, y;
        
        if(evt->button.button == MOUSE_RIGTH_BUTTON)    return FALSE;

        nfui_nfobject_get_window_pos(obj, &x, &y);

        x += ((GdkEventButton*)evt)->x;
        y += ((GdkEventButton*)evt)->y;

        if (obj == g_id) 
        {
            text = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text(obj), x, y, 32, VKEY_ID);
            if (!text) return FALSE;

            nfui_nflabel_set_text((NFLABEL*)obj, text);

            strcpy(ru1.id, text);
            strcpy(ru2.id, text);
            _set_rtsp_addr(g_rtsp1, &ru1);
            _set_rtsp_addr(g_rtsp2, &ru2);
        }
        else if (obj == g_pw) 
        {
            text = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text(obj), x, y, 64, VKEY_PASSWORD);
            if (!text) return FALSE;
            
            nfui_nflabel_set_text((NFLABEL*)obj, text);

            strcpy(ru1.pw, text);
            strcpy(ru2.pw, text);
            _set_rtsp_addr(g_rtsp1, &ru1);
            _set_rtsp_addr(g_rtsp2, &ru2);
        }

        if (text)
        {
            ifree(text);
            text = NULL;
        }

        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_rtsp1_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_2BUTTON_PRESS)
    {
        RTSP_UNIT_T ru;
        gchar *addr;
        guint x, y;
        
        if(evt->button.button == MOUSE_RIGTH_BUTTON)    return FALSE;

        nfui_nfobject_get_window_pos(obj, &x, &y);

        x += ((GdkEventButton*)evt)->x;
        y += ((GdkEventButton*)evt)->y;

        addr = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text(obj), x, y, MAX_RTSP_STRING_SIZE, VKEY_RTSP);

        if(addr)
        {
            if (_get_rtsp_unit(addr, &ru) != -1)
            {
                if (_check_valid(&ru) == 0)
                {
                    strcpy(ru1.path, ru.path);
                    nfui_nflabel_set_text((NFLABEL*)obj, addr);
                    nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
                }
                else
                {
                    nftool_mbox(g_curwnd, "ERROR", "Invalid RTSP address format.", NFTOOL_MB_OK);
                }
            }
            else
            {
                nftool_mbox(g_curwnd, "ERROR", "Invalid RTSP address format.", NFTOOL_MB_OK);
            }

            ifree(addr);
            addr = NULL;
        }
    }

    return FALSE;
}

static gboolean post_rtsp2_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_2BUTTON_PRESS)
    {
        RTSP_UNIT_T ru;
        gchar *addr;
        guint x, y;
        
        if(evt->button.button == MOUSE_RIGTH_BUTTON)    return FALSE;

        nfui_nfobject_get_window_pos(obj, &x, &y);
          
        x += ((GdkEventButton*)evt)->x;
        y += ((GdkEventButton*)evt)->y;

        addr = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text(obj), x, y, MAX_RTSP_STRING_SIZE, VKEY_RTSP);

        if(addr)
        {
            if (_get_rtsp_unit(addr, &ru) != -1)
            {
                if (_check_valid(&ru) == 0)
                {
                    strcpy(ru2.path, ru.path);
                    nfui_nflabel_set_text((NFLABEL*)obj, addr);
                    nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
                }
                else
                {
                    nftool_mbox(g_curwnd, "ERROR", "Invalid RTSP address format.", NFTOOL_MB_OK);
                }
            }
            else
            {
                nftool_mbox(g_curwnd, "ERROR", "Invalid RTSP address format.", NFTOOL_MB_OK);
            }

            ifree(addr);
            addr = NULL;
        }
    }

    return FALSE;
}

static gboolean post_info_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    guint pos_x, pos_y;
    
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        nfui_nfobject_get_window_pos(obj, &pos_x, &pos_y);
          
        pos_x += ((GdkEventButton*)evt)->x;
        pos_y += ((GdkEventButton*)evt)->y;
        
        VW_virtual_camera_info_page(g_curwnd, pos_x, pos_y);
    }

    return FALSE;
}

static gboolean post_ok_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *topwin;

        gchar model_name[64];
        gchar rtsp_main[MAX_RTSP_STRING_SIZE];
        gchar rtsp_second[MAX_RTSP_STRING_SIZE];

        OPENMODE_STATE_VIRTUAL_CAM_ADD state;

        gint ret;
        gint ret_fail;

        if(evt->button.button == MOUSE_RIGTH_BUTTON)    return FALSE;

        memset(model_name, 0x00, sizeof(model_name));
        memset(rtsp_main, 0x00, sizeof(rtsp_main));
        memset(rtsp_second, 0x00, sizeof(rtsp_second));

        strcpy(model_name, nfui_nflabel_get_text((NFLABEL*)g_vname));
        strcpy(rtsp_main, nfui_nflabel_get_text((NFLABEL*)g_rtsp1));
        strcpy(rtsp_second, nfui_nflabel_get_text((NFLABEL*)g_rtsp2));

        if(!(strlen(model_name)))
        {
            nftool_mbox(g_curwnd, "NOTICE", "Input CAMERA NAME please..", NFTOOL_MB_OK);
        
            return FALSE;
        }

        ret = nf_openmode_add_virtual_camera(rtsp_main, rtsp_second, model_name, &state);

        switch(state)
        {
            case OPENMODE_STATE_VIRTUAL_CAM_ADD_OK:
            {
                strcpy(ru1.rtsp_addr, nfui_nflabel_get_text((NFLABEL*)g_rtsp1));
                strcpy(ru2.rtsp_addr, nfui_nflabel_get_text((NFLABEL*)g_rtsp2));
                
                g_memmove(&org_ru1, &ru1, sizeof(RTSP_UNIT_T));
                g_memmove(&org_ru2, &ru2, sizeof(RTSP_UNIT_T));

                topwin = nfui_nfobject_get_top(obj);
                nfui_nfobject_destroy(topwin);
                
                break;
            }   

            // FAIL ���� ���� ... jaeyoung test check please
            
            case OPENMODE_STATE_VIRTUAL_CAM_ADD_FAIL:
                nftool_mbox(g_curwnd, "NOTICE", "ADD FAIL", NFTOOL_MB_OK);
            break;

            case OPENMODE_STATE_VIRTUAL_CAM_ADD_MAIN_ADDR_FAIL:
                nftool_mbox(g_curwnd, "NOTICE", "Please check RTSP 1 address", NFTOOL_MB_OK);
            break;

            case OPENMODE_STATE_VIRTUAL_CAM_ADD_SECOND_ADDR_FAIL:
                nftool_mbox(g_curwnd, "NOTICE", "Please check RTSP 2 address", NFTOOL_MB_OK);
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
        
        g_memmove(&ru1, &org_ru1, sizeof(RTSP_UNIT_T));
        g_memmove(&ru2, &org_ru2, sizeof(RTSP_UNIT_T));
        
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

gboolean VW_virtual_camera_set_Popup(NFWINDOW *parent, gint win_x, gint win_y)
{
    NFOBJECT *main_wnd;
    NFOBJECT *nffixed;
    NFOBJECT *obj;
    
    gchar *noti_str[3] = {STRSTR0, STRSTR1, STRSTR2};
    gchar *strTitle[] = {"AUTO COMPLETE", "MANUAL INPUT"};
    gchar *strch[] = {"CH 1", "CH 2", "CH 3", "CH 4", "CH 5", "CH 6", "CH 7", "CH 8", "CH 9", "CH 10", "CH 11", "CH 12", "CH 13", "CH 14", "CH 15", "CH 16"};
	GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];
    gchar str[32];
    gint i,j;
    gint pos_x, pos_y;
    gint size_w, size_h;
    GSList *slist;


    if (!is_opened) 
    {
        strcpy(ru1.path, "live/main0");
        strcpy(ru2.path, "live/second0");
        is_opened = 1;
    }
    
    g_memmove(&org_ru1, &ru1, sizeof(RTSP_UNIT_T));
    g_memmove(&org_ru2, &ru2, sizeof(RTSP_UNIT_T));
    ru1.port = DEFAULT_RTSP_PORT;
    ru2.port = DEFAULT_RTSP_PORT;
    
    main_wnd = nftool_create_popup_window(parent, win_x, win_y, VCAMERA_POPUP_SIZE_WIDTH, VCAMERA_POPUP_SIZE_HEIGHT, "VIRTUAL CAMERA ADD", FALSE);

    nfui_regi_post_event_callback(main_wnd, post_page_event_handler);
    g_curwnd = main_wnd;

    nffixed = ((NFWINDOW*)main_wnd)->child;

    pos_x = 10;
    pos_y = 50;

	
    // CAMERA NAME

    obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("CAMERA NAME", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 200, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x, pos_y);

    pos_x += 200 + 10;

    obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("VCAM", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(115));
    nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, VC_INPUT_LB_SIZE_W, 40);
    nfui_nflabel_set_skin_type(obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(116));
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x, pos_y);    
    nfui_regi_post_event_callback(obj, post_text_label_event_handler);
    g_vname = obj;

    // IP ADDRESS

    pos_y += 43;
    pos_x = 10;
    
    obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("IP / HOST NAME", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 200, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x, pos_y);

    pos_x += 200 + 10;

	obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font(ru1.host, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(115));
    nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, VC_INPUT_LB_SIZE_W, 40);
    nfui_nflabel_set_skin_type(obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(116));
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x, pos_y);    
    nfui_regi_post_event_callback(obj, post_host_event_handler);
    g_ipaddr = obj;

    // PORT

    pos_y += 43;
    pos_x = 10;
    
    obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("RTSP PORT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
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
    nfui_regi_post_event_callback(obj, post_port_event_handler);
    g_port = obj;

    nfui_nflabel_set_number((NFLABEL*)obj, ru1.port);

    // ID

    pos_y += 43;
    pos_x = 10;
    
    obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("ID", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 200, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x, pos_y);

    pos_x += 200 + 10;

    obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font(ru1.id, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(115));
    nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, VC_INPUT_LB_SIZE_W, 40);
    nfui_nflabel_set_skin_type(obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(116));
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x, pos_y);    
    nfui_regi_post_event_callback(obj, post_text_label_event_handler);
    g_id = obj;

    // PASSWORD

    pos_y += 43;
    pos_x = 10;
    
    obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("PASSWORD", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 200, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x, pos_y);

    pos_x += 200 + 10;

    obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font(ru1.pw, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(115));
    nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, VC_INPUT_LB_SIZE_W, 40);
    nfui_nflabel_set_skin_type(obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(116));
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x, pos_y);    
    nfui_regi_post_event_callback(obj, post_text_label_event_handler);
    g_pw = obj;

    // ETC

    //pos_y += 43;
    pos_x = 10;
    
    obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("ETC", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 200, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    //nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x, pos_y);

    pos_x += 200 + 10;

    obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font(ru1.path, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(115));
    nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, VC_INPUT_LB_SIZE_W, 40);
    nfui_nflabel_set_skin_type(obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(116));
    //nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x, pos_y);    
    nfui_regi_post_event_callback(obj, post_path_label_event_handler);
    g_etc = obj;

//===> CH
    
    //pos_y += 43;
    pos_x = 10;
    
    obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("CHANNEL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 200, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    //nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x, pos_y);

    pos_x += 200 + 10;

    obj = nfui_combobox_new(strch, 16, ru1.ch);
    nfui_combobox_set_skin_type((NFCOMBOBOX*)obj, NFCOMBOBOX_TYPE_POPUP_1);
    nfui_combobox_set_align((NFCOMBOBOX*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, VC_INPUT_LB_SIZE_W, 40);
    //nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x, pos_y);    
    nfui_regi_post_event_callback(obj, _post_ch_event_handler);
    g_ch = obj;

    pos_y += 30;        //distance
    
#if 0
//===> INPUT TYPE
    pos_x = 10;
    pos_y += 40;

	radio_img[0] = nfui_get_image_from_file((IMG_N_POPUP_RADIO_OFF), NULL);
	radio_img[1] = nfui_get_image_from_file((IMG_O_POPUP_RADIO_ON), NULL);
	radio_img[2] = nfui_get_image_from_file((IMG_P_POPUP_RADIO_ON), NULL);
	radio_img[3] = nfui_get_image_from_file((IMG_D_POPUP_RADIO_OFF), NULL);

	nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

	for(i = 0; i < 2; i++) 
	{
		obj = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
		nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
		nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
		nfui_regi_post_event_callback(obj, post_onoff_event_cb);
		nfui_nfobject_show(obj);
		g_radio[i] = obj;

		if(i == 0) 
		{
			slist = nfui_radio_button_get_group(NF_BUTTON(obj));
			nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);
		} 
		else 
		{
			pos_x += (guint)size_w + 20;
			nfui_radio_button_add_group(NF_BUTTON(obj), slist);
		}
		nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x, pos_y);

		/* label */
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
		nfui_nfobject_set_size(obj, 200, size_h);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x + size_w + 5, pos_y);

		pos_x += 250 + size_w;
	}
#endif

    // RTSP 1st
    
    pos_y += 43;
    pos_x = 10;

    obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("1ST STREAM", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 200, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x, pos_y);

    pos_x += 200 + 10;

    obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("rtsp://", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(115));
    nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_LEFT, 10);
    nfui_nfobject_set_size(obj, VCAMERA_POPUP_SIZE_WIDTH - 230, 40);
    nfui_nflabel_set_skin_type(obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(116));
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x, pos_y);    
    nfui_regi_post_event_callback(obj, post_rtsp1_label_event_handler);    
    g_rtsp1 = obj;

    // RTSP 2nd
    
    pos_y += 43;
    pos_x = 10;

    obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("2ND STREAM", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 200, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x, pos_y);

    pos_x += 200 + 10;

    obj = (NFOBJECT *)nfui_nflabel_new_with_pango_font("rtsp://", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(115));
    nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_LEFT, 10);
    nfui_nfobject_set_size(obj, VCAMERA_POPUP_SIZE_WIDTH - 230, 40);
    nfui_nflabel_set_skin_type(obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(116));
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x, pos_y);    
    nfui_regi_post_event_callback(obj, post_rtsp2_label_event_handler);    
    g_rtsp2 = obj;

    pos_y += 42;
    pos_x = 10;

    for(i = 0; i < 2; i++)
    {
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(noti_str[i], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(116));
        nfui_nflabel_set_align((NFLABEL*) obj, NFALIGN_LEFT, 0);
        nfui_nfobject_set_size(obj, VCAMERA_POPUP_SIZE_WIDTH-100, 38);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)nffixed, obj, pos_x, pos_y);   

        pos_y += 38;
    }
            
    obj = nftool_normal_button_create_popup_type2("INFO", 152);
    nfui_nfobject_show(obj);    
    //nfui_nfobject_disable(obj);
    nfui_nffixed_put((NFFIXED*)nffixed, obj, 10, VCAMERA_POPUP_SIZE_HEIGHT - 45);
    nfui_regi_post_event_callback(obj, post_info_button_event_handler);

    obj = nftool_normal_button_create_popup_type2("ADD", 152);
    nfui_nfobject_show(obj);    
    nfui_nffixed_put((NFFIXED*)nffixed, obj, VCAMERA_POPUP_SIZE_WIDTH - (152*2) - 10 - 2, VCAMERA_POPUP_SIZE_HEIGHT - 45);
    nfui_regi_post_event_callback(obj, post_ok_button_event_handler);
    g_ok_btn = obj;

    obj = nftool_normal_button_create_popup_type2("CLOSE", 152);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)nffixed, obj, VCAMERA_POPUP_SIZE_WIDTH - 152 - 10, VCAMERA_POPUP_SIZE_HEIGHT - 45);
    nfui_regi_post_event_callback(obj, post_cancel_button_event_handler);

//    _set_obj_enable(nfui_radio_button_get_toggled((NFBUTTON*)g_radio[0]), 0);

    nfui_nfobject_show(main_wnd);
    nfui_make_key_hierarchy(main_wnd);
    
    gtk_main();
   
    return FALSE;
}


