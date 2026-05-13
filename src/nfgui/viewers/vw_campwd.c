#include "nf_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_font.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"
#include "support/color.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"

#include "vw_vkeyboard.h"
#include "vw_campwd.h"
#include "ix_mem.h"
#include "vw_campwd.h"
#include "vsm.h"

#define MAX_ID_STRING_SIZE		31

#define CP_LABEL_SIZE_WID		(guint)(200)
#define CP_LABEL_SIZE_HEI		(guint)(40)
#define CP_VLABEL_SIZE_WID		(guint)(190)
#define CP_VLABEL_SIZE_HEI		(guint)(40)

enum{
	CPO_USER = 0,
	CPO_PASSWD,
	CPO_MAX
};

enum {
	CPB_OK = 0,
	CPB_CANCEL,
	CPB_BUTTONS
};


static IPCamLoginData g_ipld;
static NFOBJECT *value[CPO_MAX];
static guint g_ch;
static NFOBJECT *g_wnd[VSM_WIN_MAX] = {0,};

static gboolean post_okbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *topwin;

	if(evt->type == GDK_BUTTON_RELEASE)
	{	
		if(evt->button.button == MOUSE_RIGTH_BUTTON)	return FALSE;

		strcpy(g_ipld.id, nfui_nflabel_get_text((NFLABEL*)value[0]));
		strcpy(g_ipld.password, nfui_nflabel_get_text((NFLABEL*)value[1]));

		DAL_set_ipcam_login_data(g_ipld, g_ch);
		DAL_save_setup_db(NFSETUP_WINDOW_CAMERA);

        DAL_notify_fire_DB_sync(NF_SYSDB_TMP_CHANGE_EVENTID_ONVIF, g_ch);

		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);

		g_wnd[g_ch] = 0;
	}

	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *topwin;

    if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)	return FALSE;

		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);

		g_wnd[g_ch] = 0;
	}

	return FALSE;
}

static gboolean post_id_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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
		NFOBJECT *top = 0;
		gchar *title = 0;
		guint x, y;

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

		title = VirtualKey_Open(g_wnd[g_ch], nfui_nflabel_get_text(obj), x, y, MAX_ID_STRING_SIZE, VKEY_ALPHANUMERIC);
        if(!title) return FALSE;
        
		if(title)
		{
			nfui_nflabel_set_text(obj, title);

			ifree(title);
			title = NULL;
		}

		nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

static gboolean post_pw_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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
		gchar *title;
		NFOBJECT *top;
		guint x, y;

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

		title = VirtualKey_Open(g_wnd[g_ch], nfui_nflabel_get_text(obj), x, y, 16, VKEY_PASSWORD);
        if(!title) return FALSE;
        
		if(title)
		{
			nfui_nflabel_set_text((NFLABEL*)obj, title);

			ifree(title);
			title = NULL;
		}

		nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
    if(evt->type == GDK_DELETE)
	{
		gtk_main_quit();
	}
	
	return FALSE;
}

gboolean VW_CamPwd_Open(NFWINDOW *parent, guint x, guint y, guint cam_num)
{
	NFOBJECT *main_wnd, *nffixed, *main_fixed, *lbTemp;
	NFOBJECT *cp_btns[CPB_BUTTONS];
	guint	i, pos_x ,pos_y;

	const gchar *strSubject[] = {"ID", "PASSWORD", "AUTHENTICATION"};
	const gchar *strButton[] = {"OK", "CANCEL"};
	const gchar *strAuth[] = {"NONE", "TEXT", "DIGEST"};
	gchar title[100];


	g_ch = cam_num;

	memset(&g_ipld, 0x00, sizeof(IPCamLoginData));
	DAL_get_ipcam_login_data(&g_ipld, cam_num);

	sprintf(title, "CAMERA %d", cam_num + 1);

	main_wnd = nftool_create_popup_window(parent, x, y, CAM_PASSWD_SIZE_WID, CAM_PASSWD_SIZE_HEI, title, FALSE);
	nffixed = ((NFWINDOW*)main_wnd)->child;
	nfui_regi_post_event_callback(nffixed, post_main_win_event_handler);
	g_wnd[g_ch] = main_wnd;

	value[0] = (NFOBJECT*)nfui_nflabel_new_text_box(g_ipld.id, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)value[0], NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nfobject_support_multi_lang((NFOBJECT*)value[0], FALSE);

	value[1] = (NFOBJECT*)nfui_nflabel_new_text_box(g_ipld.password, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)value[1], NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nflabel_set_invisible((NFLABEL*)value[1], TRUE);

	pos_y = 56;
	
	for(i=0; i<CPO_MAX; i++)
	{
		pos_x = 18;

		lbTemp = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strSubject[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
		nfui_nflabel_set_align((NFLABEL*)lbTemp, NFALIGN_LEFT, 0);
		nfui_nfobject_set_size(lbTemp, CP_LABEL_SIZE_WID, CP_LABEL_SIZE_HEI);
		nfui_nfobject_use_focus(lbTemp, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(lbTemp);
		nfui_nffixed_put((NFFIXED*)nffixed, lbTemp, pos_x, pos_y);

		pos_x = 208;

		nfui_nfobject_set_size(value[i], CP_VLABEL_SIZE_WID, CP_VLABEL_SIZE_HEI);		
		nfui_nfobject_show(value[i]);		
		nfui_nffixed_put((NFFIXED*)nffixed, value[i], pos_x, pos_y);
		
		pos_y += 43;
	}

	nfui_regi_post_event_callback(value[CPO_USER], post_id_label_event_handler);
	nfui_regi_post_event_callback(value[CPO_PASSWD], post_pw_label_event_handler);

	pos_x = 50;
	pos_y = 170;	

	for(i=0; i<CPB_BUTTONS; i++)
	{
		cp_btns[i] = nftool_normal_button_create_type1(strButton[i], 152);
		nfui_nfobject_show(cp_btns[i]);
		nfui_nffixed_put((NFFIXED*)nffixed, cp_btns[i], pos_x, pos_y);
		pos_x += 164;
	}
	
	nfui_regi_post_event_callback(cp_btns[CPB_OK], post_okbutton_event_handler);
	nfui_regi_post_event_callback(cp_btns[CPB_CANCEL], post_cancelbutton_event_handler);

	nfui_nfobject_show(main_wnd);
	nfui_make_key_hierarchy(main_wnd);

	nfui_set_key_focus(value[0], TRUE);

	gtk_main();
	
	return TRUE;
}

void VW_CamPwd_Close(guint cam_num)
{
    if(g_wnd[cam_num])
    {
        nfui_nfobject_destroy((NFOBJECT*)g_wnd[cam_num]);
        g_wnd[cam_num] = 0;
    }
}

