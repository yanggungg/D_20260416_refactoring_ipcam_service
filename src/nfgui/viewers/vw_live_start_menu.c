#include "nf_afx.h"

#include "nf_sysman.h"

#include "../support/event_loop.h"
#include "../support/nf_ui_font.h"
#include "../support/nf_ui_image.h"
#include "../support/nf_ui_page_manager.h"
#include "../support/nf_ui_color.h"
#include "../support/color.h"
#include "../support/util.h"

#include "../tools/nf_ui_tool.h"
#include "../tools/nf_ui_function.h"

#include "../viewers/objects/nfobject.h"
#include "../viewers/objects/nfwindow.h"
#include "../viewers/objects/nffixed.h"
#include "../viewers/objects/nfbutton.h"
#include "../viewers/objects/nfcheckbutton.h"
#include "../viewers/objects/nfimage.h"

#include "modules/ssm.h"

#include "vw_search_main.h"
#include "vw_live_start_menu.h"
#include "vw_live_statusbar.h"
#include "vw_userpwd.h"
#include "vw_image_popup.h"

//#include "vw_archiving.h"
//#include "nf_ui_arch_main.h"

#include "vw_sys_main.h"
#include "vw_main_menu.h"
#include "vw_timeline.h"

#include "stm.h"
#include "smt.h"
#include "vw_archiving.h"
#include "vw_record_main.h"
#include "uxm.h"

#define STATUSBAR_H                 (108)
#define STATUSBAR_AND_MENU_GAP      (4)

#define START_MENU_SIZE_W           (323)
#define START_MENU_SIZE_H           (292)

#define START_MENU_POS_X            (0)
#define START_MENU_POS_Y            (DISPLAY_ACTIVE_HEIGHT-STATUSBAR_H-START_MENU_SIZE_H-4)
#define START_MENU_UPPER_MARGIN     (8)
#define START_MENU_LOWER_MARGIN     (12)

#define START_BUTTON_SIZE_W         (300)
#define START_BUTTON_SIZE_H         (40)
#define START_BUTTON_SMALL_GAP      (2)
#define START_BUTTON_BIG_GAP        (10 + START_BUTTON_SMALL_GAP)

#define MENU_FRONT_MARGIN           (8)
#define MENU_REAR_MARGIN            (15)

#define MENU_DEF_SPACE              (MENU_FRONT_MARGIN + MENU_REAR_MARGIN)

#define MENU_TEXT_FRONT_MARGIN      (40)
#define MENU_TEXT_REAR_MARGIN       (0)

#define MENU_BUTTON_DEF_SPACE       (MENU_TEXT_FRONT_MARGIN + MENU_TEXT_REAR_MARGIN)

enum {
    SEQURINET_MENU = 0,
    SEARCH_MENU,
    ARCH_MENU,
    SYSTEM_MENU,
    RECORD_MENU,
    LOGOUT_MENU,
    SHUTDOWN_MENU,
    START_MENU_CNT
};

typedef struct _MENU_CONF {
    guint wsize_w;                                  // window width
    guint wsize_h;                                  // window height
    guint wpos_y;                                   // window pos_y
    guint wpos_x;                                   // window pos_x
    guint msize_w;                                  // button width

    GdkPixbuf *menu_img[START_MENU_CNT][NFOBJECT_STATE_COUNT];  // images

    NFOBJECT *start_win;
} MENU_CONF;

static MENU_CONF g_conf;

#if defined(_SEQURINET_STRING_FIX)
static const gchar *g_strBtn[START_MENU_CNT] = {"SEQURINET",
                                                "SEARCH",
                                                "ARCHIVING",
                                                "SYSTEM SETUP",
                                                "RECORD SETUP",
                                                "LOG IN",
                                                "SHUTDOWN"
                                                };
#else
static const gchar *g_strBtn[START_MENU_CNT] = {"P2P nViewer",
                                                "SEARCH",
                                                "ARCHIVING",
                                                "SYSTEM SETUP",
                                                "RECORD SETUP",
                                                "LOG IN",
                                                "SHUTDOWN"
                                                };
#endif

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *wait_pop = NULL;
static NFOBJECT *menu[START_MENU_CNT] = {0, };
static GdkPixbuf *g_icon_img[START_MENU_CNT][NFOBJECT_STATE_COUNT];
static GdkPixbuf *g_qrcode_img;


static gboolean _set_menu_size(MENU_CONF *pconf)
{
    guint str_w = 0;
    guint tmp_w = 0;
    guint gap;
    gint i;


    for(i=0; i<START_MENU_CNT; i++) {
        if ((i == SEQURINET_MENU && !var_get_supported_sequrinet()) && ivsc.vendor_code != 43)  continue;

        tmp_w = nfutil_string_width(1, NULL, nffont_get_pango_font(NFFONT_LARGE_SEMI), g_strBtn[i], NORMAL_SPACING);

        if(tmp_w > str_w)
            str_w = tmp_w;
    }

    if(str_w == 0) return FALSE;

    // set button width
    if(str_w < START_BUTTON_SIZE_W)     pconf->msize_w = START_BUTTON_SIZE_W;
    else                            pconf->msize_w = (str_w + MENU_BUTTON_DEF_SPACE);

    // set window width
    if(pconf->msize_w < START_MENU_SIZE_W) pconf->wsize_w = START_MENU_SIZE_W;
    else                            pconf->wsize_w = (pconf->msize_w + MENU_DEF_SPACE);

    //set window height
    for (i = 0; i < START_MENU_CNT; i++)
    {
        if ((i == SEQURINET_MENU && !var_get_supported_sequrinet()) && ivsc.vendor_code != 43)  continue;

        if( i == SEQURINET_MENU || i == ARCH_MENU || i == RECORD_MENU)
        {
            gap = START_BUTTON_BIG_GAP;
        }
        else
        {
            gap = START_BUTTON_SMALL_GAP;
        }

        pconf->wsize_h += START_BUTTON_SIZE_H + gap;
    }
    pconf->wsize_h += START_MENU_UPPER_MARGIN + START_MENU_LOWER_MARGIN;

    return TRUE;
}

static gboolean _set_menu_image(MENU_CONF *pconf)
{
    guint i;
	GdkPixbuf *pbArrow[NFOBJECT_STATE_COUNT];

	// arrow
	pbArrow[0] = nfui_get_image_from_file((IMG_CLICK_POP_ARROW_N), NULL);
	pbArrow[1] = nfui_get_image_from_file((IMG_CLICK_POP_ARROW_O), NULL);
	pbArrow[2] = nfui_get_image_from_file((IMG_CLICK_POP_ARROW_P), NULL);
	pbArrow[3] = nfui_get_image_from_file((IMG_CLICK_POP_ARROW_D), NULL);

    // button bg
    for (i=0; i<START_MENU_CNT; i++)
    {
		pconf->menu_img[i][0] = nf_ui_create_image_button_no_alpha(MKB_IMG_LIVE_START_N_BUTTON, pconf->msize_w, IMG_N_CLICK_POPUP_MENU_L, IMG_N_CLICK_POPUP_MENU_M, IMG_N_CLICK_POPUP_MENU_R);
		pconf->menu_img[i][1] = nf_ui_create_image_button_no_alpha(MKB_IMG_LIVE_START_O_BUTTON, pconf->msize_w, IMG_O_CLICK_POPUP_MENU_L, IMG_O_CLICK_POPUP_MENU_M, IMG_O_CLICK_POPUP_MENU_R);
		pconf->menu_img[i][2] = nf_ui_create_image_button_no_alpha(MKB_IMG_LIVE_START_P_BUTTON, pconf->msize_w, IMG_P_CLICK_POPUP_MENU_L, IMG_P_CLICK_POPUP_MENU_M, IMG_P_CLICK_POPUP_MENU_R);
		pconf->menu_img[i][3] = nf_ui_create_image_button_no_alpha(MKB_IMG_LIVE_START_D_BUTTON, pconf->msize_w, IMG_D_CLICK_POPUP_MENU_L, IMG_D_CLICK_POPUP_MENU_M, IMG_D_CLICK_POPUP_MENU_R);

		if(i == SEQURINET_MENU)
		{
		    pconf->menu_img[i][0] = nf_ui_create_image_button_no_alpha("sss", pconf->msize_w, IMG_N_CLICK_POPUP_MENU_L, IMG_N_CLICK_POPUP_MENU_M, IMG_N_CLICK_POPUP_MENU_R);
	    	pconf->menu_img[i][1] = nf_ui_create_image_button_no_alpha("ssd", pconf->msize_w, IMG_O_CLICK_POPUP_MENU_L, IMG_O_CLICK_POPUP_MENU_M, IMG_O_CLICK_POPUP_MENU_R);
	    	pconf->menu_img[i][2] = nf_ui_create_image_button_no_alpha("ssa", pconf->msize_w, IMG_P_CLICK_POPUP_MENU_L, IMG_P_CLICK_POPUP_MENU_M, IMG_P_CLICK_POPUP_MENU_R);
	    	pconf->menu_img[i][3] = nf_ui_create_image_button_no_alpha("ssw", pconf->msize_w, IMG_D_CLICK_POPUP_MENU_L, IMG_D_CLICK_POPUP_MENU_M, IMG_D_CLICK_POPUP_MENU_R);

    		gdk_pixbuf_composite(pbArrow[0], pconf->menu_img[SEQURINET_MENU][0], (START_BUTTON_SIZE_W - 21), 13, 7, 13, (gfloat)(START_BUTTON_SIZE_W - 21), 13.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
			gdk_pixbuf_composite(pbArrow[1], pconf->menu_img[SEQURINET_MENU][1], (START_BUTTON_SIZE_W - 21), 13, 7, 13, (gfloat)(START_BUTTON_SIZE_W - 21), 13.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
			gdk_pixbuf_composite(pbArrow[2], pconf->menu_img[SEQURINET_MENU][2], (START_BUTTON_SIZE_W - 21), 13, 7, 13, (gfloat)(START_BUTTON_SIZE_W - 21), 13.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
			gdk_pixbuf_composite(pbArrow[3], pconf->menu_img[SEQURINET_MENU][3], (START_BUTTON_SIZE_W - 21), 13, 7, 13, (gfloat)(START_BUTTON_SIZE_W - 21), 13.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
		}
    }

    return TRUE;
}

static gboolean _set_menu_position(MENU_CONF *pconf)
{
    if (!pconf) return FALSE;

    //set window position y
    pconf->wpos_y = DISPLAY_ACTIVE_HEIGHT - STATUSBAR_H - STATUSBAR_AND_MENU_GAP - pconf->wsize_h;

    //set window position x
    pconf->wpos_x = START_MENU_POS_X;

    return TRUE;
}

static gboolean _init_menu_conf(MENU_CONF *pconf)
{
    memset(pconf, 0x00, sizeof(MENU_CONF));

    if(!_set_menu_size(pconf))      return FALSE;
    if(!_set_menu_image(pconf))     return FALSE;
    if(!_set_menu_position(pconf))  return FALSE;

    return TRUE;
}

static gboolean _init_icon_img()
{
#if defined(_SEQURINET_STRING_FIX)
	g_icon_img[SEQURINET_MENU][0] = nfui_get_image_from_file((IMG_N_START_MENU_SEQURINET), NULL);
	g_icon_img[SEQURINET_MENU][1] = nfui_get_image_from_file((IMG_O_START_MENU_SEQURINET), NULL);
	g_icon_img[SEQURINET_MENU][2] = nfui_get_image_from_file((IMG_P_START_MENU_SEQURINET), NULL);
	g_icon_img[SEQURINET_MENU][3] = nfui_get_image_from_file((IMG_D_START_MENU_SEQURINET), NULL);
#else
	g_icon_img[SEQURINET_MENU][0] = nfui_get_image_from_file((IMG_N_START_MENU_P2P), NULL);
	g_icon_img[SEQURINET_MENU][1] = nfui_get_image_from_file((IMG_O_START_MENU_P2P), NULL);
	g_icon_img[SEQURINET_MENU][2] = nfui_get_image_from_file((IMG_P_START_MENU_P2P), NULL);
	g_icon_img[SEQURINET_MENU][3] = nfui_get_image_from_file((IMG_D_START_MENU_P2P), NULL);
#endif
	g_icon_img[SEARCH_MENU][0] = nfui_get_image_from_file((IMG_N_START_MENU_SEARCH), NULL);
	g_icon_img[SEARCH_MENU][1] = nfui_get_image_from_file((IMG_O_START_MENU_SEARCH), NULL);
	g_icon_img[SEARCH_MENU][2] = nfui_get_image_from_file((IMG_P_START_MENU_SEARCH), NULL);
	g_icon_img[SEARCH_MENU][3] = nfui_get_image_from_file((IMG_D_START_MENU_SEARCH), NULL);

	g_icon_img[ARCH_MENU][0] = nfui_get_image_from_file((IMG_N_START_MENU_ARCH), NULL);
	g_icon_img[ARCH_MENU][1] = nfui_get_image_from_file((IMG_O_START_MENU_ARCH), NULL);
	g_icon_img[ARCH_MENU][2] = nfui_get_image_from_file((IMG_P_START_MENU_ARCH), NULL);
	g_icon_img[ARCH_MENU][3] = nfui_get_image_from_file((IMG_D_START_MENU_ARCH), NULL);

	g_icon_img[SYSTEM_MENU][0] = nfui_get_image_from_file((IMG_N_START_MENU_SYS_SETUP), NULL);
	g_icon_img[SYSTEM_MENU][1] = nfui_get_image_from_file((IMG_O_START_MENU_SYS_SETUP), NULL);
	g_icon_img[SYSTEM_MENU][2] = nfui_get_image_from_file((IMG_P_START_MENU_SYS_SETUP), NULL);
	g_icon_img[SYSTEM_MENU][3] = nfui_get_image_from_file((IMG_D_START_MENU_SYS_SETUP), NULL);

	g_icon_img[RECORD_MENU][0] = nfui_get_image_from_file((IMG_N_START_MENU_REC_SETUP), NULL);
	g_icon_img[RECORD_MENU][1] = nfui_get_image_from_file((IMG_O_START_MENU_REC_SETUP), NULL);
	g_icon_img[RECORD_MENU][2] = nfui_get_image_from_file((IMG_P_START_MENU_REC_SETUP), NULL);
	g_icon_img[RECORD_MENU][3] = nfui_get_image_from_file((IMG_D_START_MENU_REC_SETUP), NULL);

	g_icon_img[LOGOUT_MENU][0] = nfui_get_image_from_file((IMG_N_START_MENU_LOGOUT), NULL);
	g_icon_img[LOGOUT_MENU][1] = nfui_get_image_from_file((IMG_O_START_MENU_LOGOUT), NULL);
	g_icon_img[LOGOUT_MENU][2] = nfui_get_image_from_file((IMG_P_START_MENU_LOGOUT), NULL);
	g_icon_img[LOGOUT_MENU][3] = nfui_get_image_from_file((IMG_D_START_MENU_LOGOUT), NULL);

	g_icon_img[SHUTDOWN_MENU][0] = nfui_get_image_from_file((IMG_N_START_MENU_SHUTDOWN), NULL);
	g_icon_img[SHUTDOWN_MENU][1] = nfui_get_image_from_file((IMG_O_START_MENU_SHUTDOWN), NULL);
	g_icon_img[SHUTDOWN_MENU][2] = nfui_get_image_from_file((IMG_P_START_MENU_SHUTDOWN), NULL);
	g_icon_img[SHUTDOWN_MENU][3] = nfui_get_image_from_file((IMG_D_START_MENU_SHUTDOWN), NULL);

}



static GdkPixbuf *_get_qrcode_image()
{
    GdkPixbuf *tbuf;
    gchar qr_url[512] = {0,};
	gint url_len = 512;

	var_get_qr_url(qr_url, url_len);
	scm_set_qrcode_use_URL("/tmp/itx", qr_url);

	tbuf = gdk_pixbuf_new_from_file("/tmp/itx.png", NULL);

    return tbuf;
}

static gboolean _post_cleanup_new_opening(void *data)
{
    NFOBJECT *obj = (NFOBJECT *)data;
    NFOBJECT *parent = NULL;

    parent = nfui_nfobject_get_top(obj);
    nfui_nfobject_hide(parent);
    nfui_page_close(PGID_LIVE_START_MENU, parent);

    if (vsm_get_omode() == OMODE_FULL)
    {
        // always mode
        if(VW_Live_StatusBar_On_Time() != 0)
            VW_Live_StatusBar_Hide();
        VW_Timeline_Hide();
    }

    return FALSE;
}

static void _change_status_by_usr_auth()
{
    gint i, j;
    gint auth[] = {USR_AUTH_SEQURINET,
                USR_AUTH_SEARCH,
                USR_AUTH_ARCHIVE,
                USR_AUTH_SYS_SETUP,
                USR_AUTH_RECORD_SETUP,
                USR_AUTH_SHUTDOWN};

    for (i = 0, j = 0; i < START_MENU_CNT; i++) {
        if ((i == SEQURINET_MENU && !var_get_supported_sequrinet()) && ivsc.vendor_code != 43)
        {
            j++;
            continue;
        }

        if(i != LOGOUT_MENU) {
            if(!ssm_check_access_auth(auth[j++]))
                nfui_nfobject_disable(menu[i]);
            else
                nfui_nfobject_enable(menu[i]);

        }
    }
}


static gboolean _sequrinet_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NF_NOTIFY_INFO pInfo;
    guint size_w, size_h;
    gboolean status = FALSE;
    mb_type ret;
    gchar qr_url[QRCODE_URL_SIZE];
	gchar strbuf[1024] = {0,};

    memset(&pInfo, 0x00, sizeof(NF_NOTIFY_INFO));

    switch(evt->type)
    {
        case GDK_BUTTON_RELEASE:
            {
                if(evt->button.button == MOUSE_RIGTH_BUTTON){
        			return FALSE;
        		}
        		
        		scm_get_wan_status_event_data(&pInfo);
        		DAL_get_Sequrinet_Status(&status);
        		
                if(pInfo.d.params[0]){
                    nftool_mbox(g_curwnd, "NOTICE", "Please check your network connection.", NFTOOL_MB_OK);
                    return FALSE;
        	    }

        	    if (!status) {
					#if defined(_SEQURINET_STRING_FIX)
                        strcpy(strbuf, "You cannot use Sequrinet.\nPlease activate Sequrinet from System setting > Network > DDNS menu.");
                    #else
                        strcpy(strbuf, "You cannot use P2P.\nPlease activate P2P from System setting > Network > DDNS menu.");
                    #endif

        	        ret = nftool_mbox(g_curwnd, "NOTICE", strbuf, NFTOOL_MB_OK);
        	        return FALSE;
        	    }

                if (g_qrcode_img)
                {
                    g_object_unref(g_qrcode_img);
                    g_qrcode_img = NULL;
                }
                
                g_qrcode_img = _get_qrcode_image();
                
                if (g_qrcode_img){
                    nfui_get_pixbuf_size(g_qrcode_img, &size_w, &size_h);
                    VW_Show_Image_Popup(g_qrcode_img, size_w, size_h);
                }
            }
            break;

        case NFEVENT_KEYPAD_PRESS:
        case NFEVENT_REMOCON_PRESS:
            {
                GdkEventKey *kevt;
        		KEYPAD_KID kpid;

        		kevt = (GdkEventKey*)evt;
        		kpid = (KEYPAD_KID)kevt->keyval;

        		if(kpid == KEYPAD_ENTER)
        		{
            		scm_get_wan_status_event_data(&pInfo);
                    if(pInfo.d.params[0]){
                        nftool_mbox(g_curwnd, "NOTICE", "Please check your network connection.", NFTOOL_MB_OK);
                        return FALSE;
            	    }

            	    if (!status) {
						#if defined(_SEQURINET_STRING_FIX)
                            strcpy(strbuf, "You cannot use Sequrinet.\nPlease activate Sequrinet from System setting > Network > DDNS menu.");
                        #else
                            strcpy(strbuf, "You cannot use P2P.\nPlease activate P2P from System setting > Network > DDNS menu.");
                        #endif

            	        ret = nftool_mbox(g_curwnd, "NOTICE", strbuf, NFTOOL_MB_OK);
            	        return FALSE;
            	    }

                    if (g_qrcode_img)
                    {
                        g_object_unref(g_qrcode_img);
                        g_qrcode_img = NULL;
                    }
                    
                    g_qrcode_img = _get_qrcode_image();
                    
                    if (g_qrcode_img){
                        nfui_get_pixbuf_size(g_qrcode_img, &size_w, &size_h);
                        VW_Show_Image_Popup(g_qrcode_img, size_w, size_h);
                    }
        		}

            }
            break;
        }

	return FALSE;
}

static gboolean _search_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	time_t cur_time;
    gboolean use_dl = 0;
	SecurityData secdata;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

        DAL_get_use_double_login(&use_dl);

        if (use_dl && !ssm_is_admin())
        {
            if (!VW_UserPwd_Open(NF_TOPWND, "DOUBLE LOGIN", -1)) return FALSE;
            if (!VW_UserPwd_Open(NF_TOPWND, "DOUBLE LOGIN2", USR_AUTH_SEARCH)) return FALSE;
        }
        else
        {
		DAL_get_security_data(&secdata);
    		if (secdata.loginSearchArch && !VW_UserPwd_Open(g_curwnd, "PASSWORD CHECK", -1)) return FALSE;
        }

		g_timeout_add(10, _post_cleanup_new_opening, obj);
//		vsm_live_stop();

		time(&cur_time);
		stm_set_time_t(cur_time-360);
		stm_set_endtime_t(cur_time-60);
		
		VW_Search_Open(NF_TOPWND, 0, vsm_create_livestart_obj());
	}	
	    
	return FALSE;
}

static gboolean _arch_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gboolean use_dl = 0;
		SecurityData secdata;

    if(evt->type == GDK_BUTTON_RELEASE)
    {
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

        DAL_get_use_double_login(&use_dl);

        if (use_dl && !ssm_is_admin())
        {
            if (!VW_UserPwd_Open(NF_TOPWND, "DOUBLE LOGIN", -1)) return FALSE;
            if (!VW_UserPwd_Open(NF_TOPWND, "DOUBLE LOGIN2", USR_AUTH_ARCHIVE)) return FALSE;
        }
        else
        {
		DAL_get_security_data(&secdata);
    		if (secdata.loginSearchArch && !VW_UserPwd_Open(g_curwnd, "PASSWORD CHECK", -1)) return FALSE;
        }

		g_timeout_add(10, _post_cleanup_new_opening, obj);
//		vsm_live_stop();

		stm_set_time_by_sys();
		VW_Archiving_Open(NF_TOPWND, vsm_create_livestart_obj(), 0);
	}	

	return FALSE;
}

static gboolean _sys_setup_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *parent = NULL;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		parent = nfui_nfobject_get_top(obj);
		nfui_nfobject_hide(parent);	
		nfui_page_close(PGID_LIVE_START_MENU, parent);
		
		if (vsm_get_omode() == OMODE_FULL)
		{
			// always mode
			if(VW_Live_StatusBar_On_Time() != 0) 
				VW_Live_StatusBar_Hide();
			VW_Timeline_Hide();
		}

		VW_MainMenu_Open(NF_TOPWND);

		VW_Timeline_ChangeMode(TL_LIVE);
	}	
	return FALSE;
}

static gboolean _rec_setup_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		g_timeout_add(10, _post_cleanup_new_opening, obj);
		vsm_live_stop();
		
		VW_RecordSetup_Open(NF_TOPWND);
		
		vsm_live_start();
		
		VW_Timeline_ChangeMode(TL_LIVE);
		if (vsm_get_omode() == OMODE_NORMAL)
		{
			VW_Live_StatusBar_Show();
			VW_Timeline_Show();
		}

	}	
	return FALSE;
}

static gboolean _logout_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *parent = NULL;
	gchar *str = NULL;
	gint retval;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		parent = nfui_nfobject_get_top(obj);
		nfui_nfobject_hide(parent);	
		nfui_page_close(PGID_LIVE_START_MENU, parent);
		
		str = nfui_nfbutton_get_text((NFBUTTON*)obj);	
		
		if(!strcmp(str, "LOG OUT")) 
		{
			scm_put_log(CLOSE_LIVE, 0, 0);
			ssm_logout();

			VW_Live_StatusBar_Menu_Disable();
			VW_Live_StatusBar_Set_User("");
			LiveStart_Popup_Menu_Disable();

			evt_send_to_local(INFY_USER_LOGON, 0, 0, NULL);

			vsm_set_covert_by_logout();			
			smt_set_service(SMT_LOGOUT);
		}
		else if(!strcmp(str, "LOG IN")) 
		{
			if (VW_UserPwd_Open(g_curwnd, "LOG IN", -1))
			{
    			UserManageData userdata;

    			memset(&userdata, 0x00, sizeof(UserManageData));
    			DAL_get_userManage_data(&userdata, ssm_get_cur_idx());

    			if ((ivsc.dfunc.support_find_passwd) && (userdata.email_certification == 0)) 
    			{
					if(ivsc.dfunc.support_auth_popup_hide) 
					{
						if(userdata.certi_popup_hide == 0) {
							retval = vw_open_authen_info_setup(g_curwnd, ssm_get_cur_idx(), 0);
						}
					}
					else
					{
						retval = vw_open_authen_info_setup(g_curwnd, ssm_get_cur_idx(), 0);
					}
    			}
    			
				smt_set_service(SMT_LIVE);
			}
		}
	}	
	return FALSE;
}

static gboolean _shutdown_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *parent = NULL;
        gint ret;

        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		parent = nfui_nfobject_get_top(obj);
		nfui_nfobject_hide(parent);	
		nfui_page_close(PGID_LIVE_START_MENU, parent);

        proxy_system("rm -f /NFDVR/data/itx_enable_capture_mode.txt", 1, 3);

        if (scm_is_exist_usb_file("itx_enable_capture_mode.txt"))
        {
            ret = nftool_mbox(g_curwnd, "CONFIRM", "Enable capture mode?", NFTOOL_MB_YESNO);
            if (ret == NFTOOL_MB_YES) {
                proxy_system("echo 1 > /NFDVR/data/itx_enable_capture_mode.txt", 1, 3);
            }
        }

		if(VW_UserPwd_Open(g_curwnd, "SHUTDOWN", -1)) {
			smt_set_service(SMT_SHUTDOWN);	

			scm_shutdown_system(IRET_SCM_SHUTDOWN);
			scm_notify_to_system("dvr_status", NF_DVR_STATUS_SHUTDOWN);

			wait_pop = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "Please wait...", "");
		}
	}

	return FALSE;
}

static gboolean _post_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if(evt->type == GDK_EXPOSE) {
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(345));

        //ex) 53 = START_MENU_UPPER_MARGIN + START_BUTTON_SIZE_H + (START_BUTTON_BIG_GAP/2)
        if(var_get_supported_sequrinet() || ivsc.vendor_code == 43)
        {
            gdk_draw_rectangle(drawable, gc, TRUE, 8, 53, g_conf.msize_w, 1);
    		gdk_draw_rectangle(drawable, gc, TRUE, 8, 147, g_conf.msize_w, 1);
    		gdk_draw_rectangle(drawable, gc, TRUE, 8, 241, g_conf.msize_w, 1);
        }
        else{
            gdk_draw_rectangle(drawable, gc, TRUE, 8, 95, g_conf.msize_w, 1);
            gdk_draw_rectangle(drawable, gc, TRUE, 8, 189, g_conf.msize_w, 1);
        }

        nfui_nfobject_gc_unref(gc);
    }
    else if (evt->type == GDK_DELETE)
    {
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
    }

	return FALSE;
}

static gboolean _post_window_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case GDK_EXPOSE:
			{

			}
			break;

		case NFOUTEVT_BUTTON_PRESS:
			{
				if(evt->button.button == 3)
					return FALSE;
				nfui_nfobject_hide(obj);
				nfui_page_close(PGID_LIVE_START_MENU, obj);
			}
			break;

		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
			{
				GdkEventKey *kevt;
				KEYPAD_KID kpid;

				kevt = (GdkEventKey*)evt;
				kpid = (KEYPAD_KID)kevt->keyval;

				if(kpid == KEYPAD_EXIT)
				{
					nfui_nfobject_hide(obj);
					nfui_page_close(PGID_LIVE_START_MENU, g_conf.start_win);
				}
			}
			break;

		case IRET_SCM_SHUTDOWN:
		    if (wait_pop)
		    {
    			nftool_remove_waitbox(wait_pop);
			}
			nftool_mbox_wait(g_curwnd, "NOTICE", "You can power off the system.");
			break;

		case GDK_DELETE:
			g_curwnd = 0;
            g_object_unref(g_qrcode_img);
            g_qrcode_img = 0;
			uxm_unreg_imsg_event(g_conf.start_win, IRET_SCM_SHUTDOWN);
			nfui_page_close(PGID_LIVE_START_MENU, g_conf.start_win);
			break;

		default:
			break;
	}

	return FALSE;
}

static gboolean returnkey_proc(NFOBJECT *top, GdkEvent *evt, gpointer data)
{
    //for SWANFFOURG-432
	return FALSE;
}

void LiveStart_Popup_Menu(NFWINDOW *parent)
{
    NFOBJECT *fixed1;
    NFOBJECT *nfwidget;
    gint size_w, size_h;
    gint gap;
    guint i, pos_x = 0, pos_y = 0;
    guint font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(340), COLOR_IDX(341), COLOR_IDX(343), COLOR_IDX(344)};

    /* init */
    _init_menu_conf(&g_conf);
    _init_icon_img();


    /* window */
    g_conf.start_win = (NFOBJECT*)nfui_nfwindow_new(parent, (guint)g_conf.wpos_x, (guint)g_conf.wpos_y, (guint)g_conf.wsize_w, (guint)g_conf.wsize_h);
    g_curwnd = g_conf.start_win;
    nfui_regi_post_event_callback(g_conf.start_win, _post_window_event_cb);
    nfui_nfwindow_set_returnkey_proc((NFWINDOW*)g_conf.start_win, returnkey_proc);
    nfui_nfobject_modify_bg(g_conf.start_win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfwindow_use_outside_evt((NFWINDOW*)g_conf.start_win, TRUE);
    nfui_nfwindow_set_mask((NFWINDOW*)g_conf.start_win, GDK_BUTTON_PRESS, TRUE);

    /* fixed */
    fixed1 = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed1, (guint)g_conf.wsize_w, (guint)g_conf.wsize_h);
    nfui_regi_post_event_callback(fixed1, _post_fixed_event_cb);
    nfui_nfobject_show(fixed1);

    pos_x = MENU_FRONT_MARGIN;
    pos_y = START_MENU_UPPER_MARGIN;

    for (i = 0; i < START_MENU_CNT; i++)
    {
        if ((i == SEQURINET_MENU && !var_get_supported_sequrinet()) && ivsc.vendor_code != 43)  continue;

        if (i == SEQURINET_MENU && ivsc.vendor_code == 28) menu[i] = (NFOBJECT*)nfui_nfbutton_new_with_param(g_conf.menu_img[i], "P2P Cloud Service");
        else menu[i] = (NFOBJECT*)nfui_nfbutton_new_with_param(g_conf.menu_img[i], g_strBtn[i]);
        nfui_nfbutton_set_font_alignment(NF_BUTTON(menu[i]), NFALIGN_LEFT, (DISPLAY_IS_D1 ? 4 : 10));
        nfui_nfbutton_set_pango_font((NFBUTTON*)menu[i], nffont_get_pango_font(NFFONT_LARGE_SEMI), (guint*)font_color);
        nfui_nfbutton_set_icon_image((NFBUTTON*)menu[i], g_icon_img[i]);
        nfui_nfobject_set_size(menu[i], START_BUTTON_SIZE_W, MENU_TEXT_FRONT_MARGIN);
        nfui_nfobject_disable(menu[i]);
        nfui_nfobject_show(menu[i]);
        nfui_nffixed_put((NFFIXED*)fixed1, menu[i], pos_x, pos_y);

        if(i == LOGOUT_MENU) nfui_nfobject_enable(menu[i]);

        if (i == SEQURINET_MENU)   nfui_regi_post_event_callback(menu[i], _sequrinet_button_event_handler);
        else if (i == SEARCH_MENU)  nfui_regi_post_event_callback(menu[i], _search_button_event_handler);
        else if (i == ARCH_MENU)    nfui_regi_post_event_callback(menu[i], _arch_button_event_handler);
        else if (i == SYSTEM_MENU)  nfui_regi_post_event_callback(menu[i], _sys_setup_button_event_handler);
        else if (i == RECORD_MENU)  nfui_regi_post_event_callback(menu[i], _rec_setup_button_event_handler);
        else if (i == LOGOUT_MENU)  nfui_regi_post_event_callback(menu[i], _logout_button_event_handler);
        else if (i == SHUTDOWN_MENU) nfui_regi_post_event_callback(menu[i], _shutdown_button_event_handler);

        if( i == SEQURINET_MENU || i == ARCH_MENU || i == RECORD_MENU)
        {
            gap = START_BUTTON_BIG_GAP;
        }
        else
        {
            gap = START_BUTTON_SMALL_GAP;
        }

        pos_y += START_BUTTON_SIZE_H + gap;
    }

    uxm_reg_imsg_event(g_conf.start_win, IRET_SCM_SHUTDOWN);
    uxm_monitor_on_imsg_event(g_conf.start_win, IRET_SCM_SHUTDOWN);

    nfui_nfwindow_add((NFWINDOW*)g_conf.start_win, fixed1);
    nfui_run_main_event_handler(g_conf.start_win);
    nfui_nfobject_show(g_conf.start_win);
    nfui_make_key_hierarchy((NFWINDOW*)g_conf.start_win);

    nfui_nfobject_hide(g_conf.start_win);

    /* make qrcode */
    g_qrcode_img = _get_qrcode_image();
    if (g_qrcode_img){
        nfui_get_pixbuf_size(g_qrcode_img, &size_w, &size_h);
    }

    if(var_get_supported_sequrinet() || ivsc.vendor_code == 43)
    {
        VW_Image_Popup_Open(g_curwnd, menu[SEQURINET_MENU], g_qrcode_img, size_w, size_h);
    }

    nfui_page_open(PGID_LIVE_START_MENU, g_conf.start_win, nfui_get_last_user());

}

int LiveStart_Popup_Menu_Destroy()
{
    if (!g_curwnd) return 0;
    nfui_nfobject_destroy(g_curwnd);
    return 0;
}

//extern int protect_mouse;
gboolean LiveStart_Popup_Refresh(void)
{
    if (nfui_nfobject_is_shown(g_conf.start_win))
    {
        nfui_nfobject_hide(g_conf.start_win);
        nfui_page_close(PGID_LIVE_START_MENU, g_conf.start_win);
    }
    else
    {
//      if (protect_mouse) return;
        _change_status_by_usr_auth();

        nfui_nfobject_show(g_conf.start_win);
        nfui_page_open(PGID_LIVE_START_MENU, g_conf.start_win, nfui_get_last_user());
    }

    return FALSE;
}

void LiveStart_Popup_Menu_Enable()
{
    _change_status_by_usr_auth();
    nfui_nfbutton_set_text((NFBUTTON*)menu[LOGOUT_MENU], "LOG OUT");
}

void LiveStart_Popup_Menu_Disable()
{
    gint i;

    for (i = 0; i < START_MENU_CNT; i++) {
        if (i == SEQURINET_MENU && !var_get_supported_sequrinet()) continue;

        if (i != LOGOUT_MENU)
            nfui_nfobject_disable(menu[i]);
    }

    nfui_nfbutton_set_text((NFBUTTON*)menu[LOGOUT_MENU], "LOG IN");
}

gboolean LiveStart_Popup_Hide(void)
{
    nfui_nfobject_hide(g_conf.start_win);
    nfui_page_close(PGID_LIVE_START_MENU, g_conf.start_win);
}

int LiveStart_Popup_Pos(int *x, int *y)
{
	*x = g_curwnd->object.x;
	*y = g_curwnd->object.y;
	return 0;
}

int LiveStart_Popup_Size(int *w, int *h)
{
	*w = g_curwnd->object.width;
	*h = g_curwnd->object.height;

	return 0;
}
