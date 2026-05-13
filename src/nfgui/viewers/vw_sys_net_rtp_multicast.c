#include "../tools/nf_ui_tool.h"

#include "../support/nf_ui_image.h"
#include "../support/event_loop.h"

#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfobject.h"
#include "objects/nfbutton.h"
#include "objects/nfimage.h"
#include "objects/nflabel.h"
#include "objects/nfspinbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nfipeditor.h"
#include "objects/nftable.h"

#include "vw_sys_net_main.h"
#include "vw_ip_editor_popup.h"

#define PAGE_FIXED_CNT          4
#define ROW_CNT_PER_PAGE        (GUI_CHANNEL_CNT / PAGE_FIXED_CNT)

#define MIN_MULTICAST_IPADDR                    (guint)(3758096384)
#define MAX_MULTICAST_IPADDR                    (guint)(4026531839)


//===> POSITION
#define RTP_CATE_TITLE_X						(8)
#define RTP_CATE_TITLE_Y						(0)

#define RTP_ITEM_TITLE_X                        (RTP_CATE_TITLE_X + 20)

//===> SIZE
#define RTP_ITEM_TITLE_H                        (40)
#define RTP_ITEM_TITLE_W                        (200)
#define RTP_ITEM_CELL_W                         (400)
#define RTP_ITEM_CELL_H                         (RTP_ITEM_TITLE_H)

//#define SHOW_MULTICAST_DATA

enum
{
    FIR_STREAM = 0,
    SEC_STREAM,

    STREAM_CNT
};

static MulticastData multidata;
static MulticastData org_multidata;

static NFWINDOW *g_curwnd;
static NFOBJECT *g_stream_ip[STREAM_CNT][GUI_CHANNEL_CNT];
static NFOBJECT *g_video_port[STREAM_CNT][GUI_CHANNEL_CNT];
static NFOBJECT *g_audio_port[STREAM_CNT][GUI_CHANNEL_CNT];
static NFOBJECT *g_ttl;
static NFOBJECT *g_page_fixed[PAGE_FIXED_CNT];
static NFOBJECT *g_lb_page_num;


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

static gint _show_multicastData(MulticastData *data)
{
    gint i, j;

#ifdef SHOW_MULTICAST_DATA
    g_message("%s, %d", __FUNCTION__, __LINE__);
    g_message("ttl : %d", data->ttl);
    for (i = 0; i < STREAM_CNT; i++)
    {
        for (j = 0; j < GUI_CHANNEL_CNT; j++)
        {
            g_message("stream_ip : %d. %d. %d. %d", data->list[i][j].stream_ip[0], data->list[i][j].stream_ip[1], data->list[i][j].stream_ip[2], data->list[i][j].stream_ip[3]);
            g_message("video_port : %d", data->list[i][j].video_port);
            g_message("audio_port : %d", data->list[i][j].audio_port);
        }
    }
#endif

    return 0;
}

static gint _init_multicastData()
{
    memset(&multidata, 0x00, sizeof(MulticastData));
    memset(&org_multidata, 0x00, sizeof(MulticastData));

    if (DAL_get_multicast_data(&multidata, STREAM_CNT, GUI_CHANNEL_CNT))
    {
        g_message("%s, %d, MULTICAST DATA LOAD FAIL!!", __FUNCTION__, __LINE__);
        memset(&multidata, 0x00, sizeof(MulticastData));

        return -1;
    }
    _show_multicastData(&multidata);

    g_memmove(&org_multidata, &multidata, sizeof(MulticastData));

    return 0;
}

static gint _set_data_to_obj(MulticastData *data, gint expose)
{
    gint i, j;


    _show_multicastData(data);

    for (i = 0; i < STREAM_CNT; i++)
    {
        for (j = 0; j < GUI_CHANNEL_CNT; j++)
        {
            nfui_nfipeditor_set_ip_array_no_expose((NFIPEDITOR*)g_stream_ip[i][j], data->list[i][j].stream_ip);
            nfui_nflabel_set_number((NFLABEL*)g_video_port[i][j], data->list[i][j].video_port);
            nfui_nflabel_set_number((NFLABEL*)g_audio_port[i][j], data->list[i][j].audio_port);
        }
    }
    nfui_nflabel_set_number((NFLABEL*)g_ttl, data->ttl);

    if (expose)
    {
        for (i = 0; i < STREAM_CNT; i++)
        {
            for (j = 0; j < GUI_CHANNEL_CNT; j++)
            {
                nfui_signal_emit(g_stream_ip[i][j], GDK_EXPOSE, TRUE);
                nfui_signal_emit(g_video_port[i][j], GDK_EXPOSE, FALSE);
                nfui_signal_emit(g_audio_port[i][j], GDK_EXPOSE, FALSE);
            }
        }
        nfui_signal_emit(g_ttl, GDK_EXPOSE, FALSE);
    }
}

static gint _get_data_from_obj(MulticastData *data)
{
    gint i, j;


    for (i = 0; i < STREAM_CNT; i++)
    {
        for (j = 0; j < GUI_CHANNEL_CNT; j++)
        {
            nfui_nfipeditor_get_ip((NFIPEDITOR*)g_stream_ip[i][j], data->list[i][j].stream_ip);
            data->list[i][j].video_port = nfui_nflabel_get_number((NFLABEL*)g_video_port[i][j]);
            data->list[i][j].audio_port = nfui_nflabel_get_number((NFLABEL*)g_audio_port[i][j]);
        }
    }
    data->ttl = nfui_nflabel_get_number((NFLABEL*)g_ttl);

    _show_multicastData(data);

    return 0;
}

static gboolean _check_ttl_validity(int ttl_num)
{

    if (ttl_num <= 0 || ttl_num > 255)
    {
        nftool_mbox(g_curwnd, "NOTICE", "TTL number must be higher than 0 and less than 256.", NFTOOL_MB_OK);
        return FALSE;
    }

	return TRUE;
}

static gboolean _check_port_validity(NFOBJECT *obj, int port_num)
{
    IPSetupData ipdata;
    RtpData rtpdata;
	gint pnum_temp;
	gint i, j;


    memset(&ipdata, 0x00, sizeof(IPSetupData));
    memset(&rtpdata, 0x00, sizeof(RtpData));

	DAL_get_ipSetup_data(&ipdata);
	DAL_get_rtp_data(&rtpdata);

    if (port_num % 2 != 0)
    {
        nftool_mbox(g_curwnd, "NOTICE", "Only even port numbers can be entered.", NFTOOL_MB_OK);
        return FALSE;
    }

    if (port_num <= 1024)
    {
        nftool_mbox(g_curwnd, "NOTICE", "Port number must be larger than 1024.", NFTOOL_MB_OK);
        return FALSE;
    }

    for (i = 0; i < STREAM_CNT; i++)
    {
        for (j = 0; j < GUI_CHANNEL_CNT; j++)
        {
            if (obj != g_video_port[i][j])
            {
                if (port_num == nfui_nflabel_get_number((NFLABEL*)g_video_port[i][j]))
                {
            		nftool_mbox(g_curwnd, "NOTICE", "It's a port number used already.", NFTOOL_MB_OK);
            	    return FALSE;
                }
            }

            if (obj != g_audio_port[i][j])
            {
                if (port_num == nfui_nflabel_get_number((NFLABEL*)g_audio_port[i][j]))
                {
            		nftool_mbox(g_curwnd, "NOTICE", "It's a port number used already.", NFTOOL_MB_OK);
            	    return FALSE;
                }
            }
        }
    }

	return TRUE;
}

static gboolean _check_ipaddr_validity(guint *ip)
{
    guint newip;
    gint i, j;

    newip = prvIPToInt(ip);

    if (newip < MIN_MULTICAST_IPADDR || newip > MAX_MULTICAST_IPADDR)
    {
        nftool_mbox(g_curwnd, "NOTICE", "Acceptable Multicast IP Address Range: [224.0.0.0 ~ 239.255.255.255]", NFTOOL_MB_OK);
        return FALSE;
    }

    for (i = 0; i < STREAM_CNT; i++)
    {
        for (j = 0; j < GUI_CHANNEL_CNT; j++)
        {
            if (newip == prvIPToInt(multidata.list[i][j].stream_ip))
            {
		        nftool_mbox(g_curwnd, "NOTICE", "This multicast IP address is already in use.", NFTOOL_MB_OK);
		        return FALSE;
            }
        }
    }

    return TRUE;
}

static gboolean post_prev_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
    gchar buf[64];
    
    if(evt->type == GDK_BUTTON_RELEASE) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
          return FALSE;
        }

        for (i = 0; i < PAGE_FIXED_CNT; i++) {
            if (nfui_nfobject_is_shown((NFOBJECT*)g_page_fixed[i])) {
                break;
            }
        }

        if (i == PAGE_FIXED_CNT) return FALSE;

        if (i == 0) return FALSE;
        
		nfui_on_backscr(obj);
		nfui_rflip(obj);

        nfui_nfobject_hide(g_page_fixed[i]);

        i--;
        
        memset(buf, 0x00, sizeof(buf));
        g_sprintf(buf, "%d / %d", i + 1, PAGE_FIXED_CNT);
        nfui_nflabel_set_text(g_lb_page_num, buf);
        nfui_signal_emit(g_lb_page_num, GDK_EXPOSE, TRUE);

        nfui_nfobject_show(g_page_fixed[i]);
        nfui_signal_emit(g_page_fixed[i], GDK_EXPOSE, TRUE);

        nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));
    	
		nfui_flip(obj);
		nfui_off_backscr(obj);
    }

    return FALSE;
}

static gboolean post_next_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
    gchar buf[64];

    if(evt->type == GDK_BUTTON_RELEASE) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
          return FALSE;
        }

        for (i = 0; i < PAGE_FIXED_CNT; i++) {
            if (nfui_nfobject_is_shown((NFOBJECT*)g_page_fixed[i])) {
                break;
            }
        }

        if (i == PAGE_FIXED_CNT) return FALSE;

        if (i == (PAGE_FIXED_CNT - 1)) return FALSE;
        
		nfui_on_backscr(obj);
		nfui_rflip(obj);

        nfui_nfobject_hide(g_page_fixed[i]);

        i++;
                
        memset(buf, 0x00, sizeof(buf));
        g_sprintf(buf, "%d / %d", i + 1, PAGE_FIXED_CNT);
        nfui_nflabel_set_text(g_lb_page_num, buf);
        nfui_signal_emit(g_lb_page_num, GDK_EXPOSE, TRUE);

        nfui_nfobject_show(g_page_fixed[i]);
        nfui_signal_emit(g_page_fixed[i], GDK_EXPOSE, TRUE);

        nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));

		nfui_flip(obj);
		nfui_off_backscr(obj);
    }

    return FALSE;
}

static gboolean post_ttl_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

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
		numRet = NumberKey_Open(g_curwnd, numTemp, x, y, 256);

		if (numRet == -1) return FALSE;

		if (_check_ttl_validity(numRet))
		{
			nfui_nflabel_set_number((NFLABEL*)obj, numRet);
			nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
		}
	}

	return FALSE;
}

static gboolean post_port_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

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

		if (_check_port_validity(obj, numRet))
		{
			nfui_nflabel_set_number((NFLABEL*)obj, numRet);
			nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
		}
	}

	return FALSE;
}

static gboolean post_stream_ip_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint result;
	IP_EDITOR_T ip_editor_data = {0, };
	gint x, y;
	guint preip, newip;


	if(evt->type == GDK_2BUTTON_PRESS)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		nfui_nfobject_get_offset(obj, &x, &y);

		nfui_nfipeditor_get_ip((NFIPEDITOR*)obj, ip_editor_data.field);
		preip = prvIPToInt(ip_editor_data.field);

		result = vw_ip_editor_popup_open(g_curwnd, x+obj->width+4, y, &ip_editor_data);
		newip = prvIPToInt(ip_editor_data.field);

		if (preip == newip) return FALSE;

		if (result == 0)
		{
		    if (_check_ipaddr_validity(ip_editor_data.field))
		    {
    			nfui_nfipeditor_set_ip_array((NFIPEDITOR*)obj, ip_editor_data.field);
		    }
		}
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_ENTER)
		{
			nfui_nfobject_get_offset(obj, &x, &y);

			nfui_nfipeditor_get_ip((NFIPEDITOR*)obj, ip_editor_data.field);
    		preip = prvIPToInt(ip_editor_data.field);

			result = vw_ip_editor_popup_open(g_curwnd, x+obj->width+4, y, &ip_editor_data);
    		newip = prvIPToInt(ip_editor_data.field);

        	if (preip == newip) return FALSE;

			if (result == 0)
    		{
    		    if (_check_ipaddr_validity(ip_editor_data.field))
    		    {
        			nfui_nfipeditor_set_ip_array((NFIPEDITOR*)obj, ip_editor_data.field);
    		    }
    		}
		}
	}

	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint stream, ch;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {
			return FALSE;
		}
    	g_memmove(&multidata, &org_multidata, sizeof(MulticastData));

        _set_data_to_obj(&multidata, 1);
    }

	return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint stream, ch;

	switch (evt->type)
	{
    	case GDK_BUTTON_RELEASE:
    	{
    		if(evt->button.button == MOUSE_RIGTH_BUTTON) {
    			return FALSE;
    		}

            _get_data_from_obj(&multidata);

    		if(memcmp(&org_multidata, &multidata, sizeof(MulticastData)))
    		{
    			gint is_changed = 0;

    			//is_changed = prvIsIPChanged();
	            DAL_set_multicast_data(&multidata, STREAM_CNT, GUI_CHANNEL_CNT);

    			if (is_changed) {
    				//wait_pop = nftool_mbox_wait(g_curwnd, "WAIT", "Network server is restarting.\nPlease wait...");
    				//DAL_save_setup_db(NFSETUP_WINDOW_NETWORK);
    				//scm_apply_netinfo_by_db(IRET_SCM_APPLY_NETINFO);
    			}
    			else {
    				nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");

                    //scm_put_log(CHANGE_NET_IP, 0, 0);
    				sysnet_set_changeflag(1);
    			}

    			g_memmove(&org_multidata, &multidata, sizeof(MulticastData));
    		}

        }
        break;
    }

	return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {
			return FALSE;
		}
		Multicast_tab_out_handler();
		SystemSetupNetwork_Destroy(obj);
	}

	return FALSE;
}

void init_NetRtp_MultiCase_Page(NFOBJECT *parent)
{
    NFOBJECT *content_fixed;
    NFOBJECT *obj;
    NFOBJECT *main_page_fixed;
    NFOBJECT *page_fixed[PAGE_FIXED_CNT];
    NFOBJECT *page_ntb[PAGE_FIXED_CNT];

    gint i, col, ch;
    guint pos_x, pos_y;
    gchar strBuf[GUI_CHANNEL_CNT][STRING_SIZE_CAMTITLE];
    gchar strBuf_num[STRING_SIZE_CAMTITLE];
    guint width[7] = {};;

    gchar *tb_title[] = {"MULTICAST IP", "VIDEO PORT", "AUDIO PORT"};
    gchar *tb_stream[] = {"1ST STREAM", "2ND STREAM"};
    
	gint size_w, size_h;
	gint page_num, row_num;
	GdkPixbuf *prev_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *next_img[NFOBJECT_STATE_COUNT];



	prev_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_N_BTN), NULL);
	prev_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_O_BTN), NULL);
	prev_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_P_BTN), NULL);
	prev_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_D_BTN), NULL);

	next_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_N_BTN), NULL);
	next_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_O_BTN), NULL);
	next_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_P_BTN), NULL);
	next_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_D_BTN), NULL);

    _init_multicastData();

	g_curwnd = nfui_nfobject_get_top(parent);

	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);


//===> STREAM TITLE
    pos_x = RTP_ITEM_TITLE_X + RTP_ITEM_TITLE_W + 2;
    pos_y = 8;

    for (i = 0; i < STREAM_CNT; i++)
    {
    	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(tb_stream[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    	nfui_nfobject_set_size(obj, 604, RTP_ITEM_CELL_H);
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

    	pos_x += 604 + 2;
    }

//===> ITEM TITLE
    pos_x = RTP_ITEM_TITLE_X;
    pos_y += RTP_ITEM_TITLE_H + 2;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CHANNEL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
	nfui_nfobject_set_size(obj, RTP_ITEM_TITLE_W, RTP_ITEM_CELL_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

    pos_x += RTP_ITEM_TITLE_W + 2;

    for (col = 0; col < STREAM_CNT; col++)
    {
        for (i = 0; i < 3; i++)
        {
        	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(tb_title[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
        	nfui_nfobject_set_size(obj, RTP_ITEM_TITLE_W, RTP_ITEM_CELL_H);
        	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
        	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        	nfui_nfobject_show(obj);
        	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	    	pos_x += RTP_ITEM_TITLE_W + 2;
        }
    }

//===> ITEM
    pos_x = RTP_ITEM_TITLE_X;
    pos_y += RTP_ITEM_TITLE_H + 2;
    
    size_w = 0;
    for (i = 0; i < 7; i++) {
        width[i] = RTP_ITEM_TITLE_W;
        size_w += width[i];
    }

    main_page_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(main_page_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(main_page_fixed, size_w, ROW_CNT_PER_PAGE * (40 + 2) + 140);
    nfui_nfobject_show(main_page_fixed);
    nfui_nffixed_put((NFFIXED*)content_fixed, main_page_fixed, pos_x, pos_y);

    for (i = 0; i < PAGE_FIXED_CNT; i++)
    {
        g_page_fixed[i] = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_modify_bg(g_page_fixed[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
        nfui_nfobject_set_size(g_page_fixed[i], size_w, ROW_CNT_PER_PAGE * (40 + 2));
        nfui_nffixed_put((NFFIXED*)main_page_fixed, g_page_fixed[i], 0, 0);

        page_ntb[i] = (NFOBJECT*)nfui_nftable_new(7, ROW_CNT_PER_PAGE, 2, 2, width, 40);
        nfui_nfobject_show(page_ntb[i]);
        nfui_nffixed_put((NFFIXED*)g_page_fixed[i], page_ntb[i], 0, 0);
    }
    nfui_nfobject_show(g_page_fixed[0]);

	nfui_get_pixbuf_size(prev_img[0], &size_w, &size_h);
	
	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), prev_img);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_page_fixed, obj, (main_page_fixed->width / 2) - (size_w + 60), main_page_fixed->height - size_h);
	nfui_regi_post_event_callback(obj, post_prev_event_handler);

	memset(strBuf, 0x00, sizeof(strBuf));
	g_sprintf(strBuf, "1 / %d", PAGE_FIXED_CNT);
	
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 100, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_page_fixed, obj, (main_page_fixed->width - 100) / 2, main_page_fixed->height - size_h);
    g_lb_page_num = obj;
    
	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), next_img);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_page_fixed, obj, (main_page_fixed->width / 2) + 60, main_page_fixed->height - size_h);
	nfui_regi_post_event_callback(obj, post_next_event_handler);

    page_num = row_num = 0;

    for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
    {
        var_get_camtitle(strBuf[ch], ch);

    	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf[ch], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    	nfui_nfobject_set_size(obj, RTP_ITEM_TITLE_W, RTP_ITEM_CELL_H);
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 8);
    	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    	nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], obj, 0, row_num);

        for (i = 0; i < STREAM_CNT; i++)
        {
        	obj = (NFOBJECT*)nfui_nfipeditor_new_with_ip(255, 255, 255, 255);
    		nfui_nfipeditor_set_pango_font((NFIPEDITOR*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(129));
        	nfui_nfobject_set_size(obj, RTP_ITEM_TITLE_W, RTP_ITEM_CELL_H);
    		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(128));
        	nfui_nfobject_show(obj);
            nfui_nftable_attach((NFTABLE*)page_ntb[page_num], obj, (i*3)+1, row_num);
        	nfui_regi_post_event_callback(obj, post_stream_ip_event_handler);
        	g_stream_ip[i][ch] = obj;

        	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
        	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
        	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
        	nfui_nflabel_use_number((NFLABEL*)obj, TRUE);
        	nfui_nfobject_set_size(obj, RTP_ITEM_TITLE_W, RTP_ITEM_CELL_H);
        	nfui_nfobject_show(obj);
            nfui_nftable_attach((NFTABLE*)page_ntb[page_num], obj, (i*3)+2, row_num);
        	nfui_regi_post_event_callback(obj, post_port_event_handler);
        	g_video_port[i][ch] = obj;

        	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
        	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
        	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
        	nfui_nflabel_use_number((NFLABEL*)obj, TRUE);
        	nfui_nfobject_set_size(obj, RTP_ITEM_TITLE_W, RTP_ITEM_CELL_H);
        	nfui_nfobject_show(obj);
            nfui_nftable_attach((NFTABLE*)page_ntb[page_num], obj, (i*3)+3, row_num);
        	nfui_regi_post_event_callback(obj, post_port_event_handler);
        	g_audio_port[i][ch] = obj;
        }

        row_num++;

        if (row_num == ROW_CNT_PER_PAGE) {
            row_num = 0;
            page_num++;
        }
    }

//===> TTL
    pos_x = 0;
    pos_y = page_ntb[0]->y + page_ntb[0]->height + 20;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("TTL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
    nfui_nfobject_set_size(obj, RTP_ITEM_TITLE_W, RTP_ITEM_CELL_H);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 8);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_page_fixed, obj, pos_x, pos_y);

    pos_x += RTP_ITEM_TITLE_W + 2;

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nflabel_use_number((NFLABEL*)obj, TRUE);
	nfui_nfobject_set_size(obj, (RTP_ITEM_TITLE_W * 2)+2, RTP_ITEM_CELL_H);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_page_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_ttl_event_handler);
	g_ttl = obj;

//===> MENU BUTTON
	obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R3_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);

	obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R2_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_applybutton_event_handler);

	obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R1_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

    _set_data_to_obj(&multidata, 0);

    return;
}

gboolean Multicast_tab_out_handler()
{
	gint stream, ch;
    mb_type ret;


    _get_data_from_obj(&multidata);

	if(!memcmp(&org_multidata, &multidata, sizeof(MulticastData))) return FALSE;

	ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

	if(ret == NFTOOL_MB_OK)
	{
        DAL_set_multicast_data(&multidata, STREAM_CNT, GUI_CHANNEL_CNT);

	    g_memmove(&org_multidata, &multidata, sizeof(MulticastData));
	}
	else
	{
    	g_memmove(&multidata, &org_multidata, sizeof(MulticastData));

        _set_data_to_obj(&multidata, 0);
	}

	return FALSE;
}
