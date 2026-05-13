#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "tools/nf_ui_tool.h"

#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"
#include "objects/nfimage.h"
#include "objects/nfvklabel.h"

#include "vw_vkeyboard.h"
#include "vw_vkeyboard2.h"

#include "vw_wizard_init.h"

#include "scm.h"
#include "uxm.h"
#include "ix_mem.h"


#define HELP_STR ""

#define MAX_MARGIM_SIZE			(guint)12

#define PI_WND_SIZE_WID			(guint)(610 + 200)
#define PI_WND_SIZE_HEI			(guint)(520 + 200)

#define SE_PP_POS_X				(guint)((DISPLAY_ACTIVE_WIDTH - PI_WND_SIZE_WID)/2)
#define SE_PP_POS_Y				(guint)((DISPLAY_ACTIVE_HEIGHT - PI_WND_SIZE_HEI)/2)

#define PI_FIXED_POS_X		(guint)(12)
#define PI_FIXED_POS_Y		(guint)(56)
#define PI_FIXED_SIZE_WID	(guint)(PI_WND_SIZE_WID - PI_FIXED_POS_X * 2)
#define PI_FIXED_SIZE_HEI	(guint)(PI_WND_SIZE_HEI - PI_FIXED_POS_Y - MAX_MARGIM_SIZE)

#define MENU_BTN_WIDTH					(162)
#define MENU_BTN_HEIGHT					(44)
#define MENU_BTN_GAP					(4)

#define MENU_V_BTN_R_START_X			(PI_FIXED_SIZE_WID - MENU_BTN_WIDTH)
#define MENU_V_BTN_R1_X					(MENU_V_BTN_R_START_X - MENU_BTN_GAP)
#define MENU_V_BTN_R2_X					(MENU_V_BTN_R1_X - MENU_BTN_GAP - MENU_BTN_WIDTH)
#define MENU_V_BTN_Y					(PI_FIXED_SIZE_HEI - 10 - MENU_BTN_HEIGHT)


#define	IPS_LABEL_HEIGHT			(40)
#define	IPS_LABEL_ROW_SPACE			(2)
#define CATEGORY_LABEL_LEFT         (4)
#define CATEGORY_CONTENT_GAP        (60)
#define	SUBJECT_LABEL_LEFT			(28)
#define	SUBJECT_LABEL_TOP			(42)
#define	SUBJECT_LABEL_WIDTH			(360)
#define	SUBJECT_LABEL_MARGIN		(0)

#define SUBJECT_LABEL_WIDTH1        (360)


enum {
	PIB_PREVIOUS,
	PIB_NEXT,
	PIB_EXIT,
	PIB_BUTTONS
};

enum {
	ROW_USER_ID = 0,
	ROW_ORG_PW,
	ROW_NEW_PW,
	ROW_CON_PW,

	ROW_MAX
};


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *wait_pop = NULL;
static NFOBJECT *g_lb_password[ROW_MAX];

static WIZARD_USERDATA_T *g_wizard_data;

static mb_type g_popup_ret = 0;
static gchar g_orgPW[NFUI_MAX_PW_SIZE];
static gchar g_newPW[2][NFUI_MAX_PW_SIZE];

static gint _exit_proc()
{
	nfui_nfobject_destroy(g_curwnd);	
	_wizard_finish();

	return 0;
}

static gint _next_step_proc()
{
	nfui_nfobject_destroy(g_curwnd);
    _wizard_next_step(1);

	return 0;
}

static gint _prev_proc()
{
	nfui_nfobject_destroy(g_curwnd);
    _wizard_prev_step(1);

    return 0;
}

static gint _prvLoadDataFromObjects()
{
	return 0;
}

static gint _save_SigType_data()
{
    return 0;
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
		gchar *pw;
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

        if (DAL_get_enahnced_password())
            pw = VirtualKey_Open2(g_curwnd, "ADMIN", nfui_nflabel_get_text(obj), x, y, NFUI_MAX_ENHANCED_PW_SIZE, VKEY2_PASSWORD);
        else
    		pw = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text(obj), x, y, NFUI_MAX_PW_SIZE, VKEY_PASSWORD);

		if(pw)
		{
			nfui_nflabel_set_text((NFLABEL*)obj, pw);
            
			ifree(pw);
			pw = NULL;
		}

		nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

static gboolean post_main_bg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	gint img_w, img_h;

	if(evt->type == GDK_EXPOSE) 
	{
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);
	
		nfui_nfobject_gc_unref(gc);
	}
	return FALSE;
}

static gboolean post_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
		if(evt->type == GDK_DELETE)
		{
			g_curwnd = 0;
			gtk_main_quit();
		}
	
		return FALSE;

}

static gboolean post_exitbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *topwin;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
		{
			return FALSE;
	    }
		
		_exit_proc();
	}

	return FALSE;
}

static gboolean post_previousbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		 _prev_proc();
	}

	return FALSE;
}
	
static gboolean post_nextbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top;
		mb_type ret;
        gchar *pw;
        
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}
		
		if(!strcmp(nfui_nflabel_get_text((NFLABEL*)g_lb_password[ROW_ORG_PW]), ""))
		{
			nftool_mbox(g_curwnd, "NOTICE", "Please input PW.", NFTOOL_MB_OK);
			return FALSE;
		}
		
		if(!strcmp(nfui_nflabel_get_text((NFLABEL*)g_lb_password[ROW_NEW_PW]), ""))
		{
			nftool_mbox(g_curwnd, "NOTICE", "Please input new PW.", NFTOOL_MB_OK);
			return FALSE;
		}

		if(!strcmp(nfui_nflabel_get_text((NFLABEL*)g_lb_password[ROW_CON_PW]), ""))
		{
			nftool_mbox(g_curwnd, "NOTICE", "Please input confirm PW.", NFTOOL_MB_OK);
			return FALSE;
		}
		
		if(strcmp(nfui_nflabel_get_text((NFLABEL*)g_lb_password[ROW_ORG_PW]), g_wizard_data->accData.pw))
		{
			nftool_mbox(g_curwnd, "NOTICE", "Invalid password.", NFTOOL_MB_OK);
			return FALSE;
		}
		
		if(strcmp(nfui_nflabel_get_text((NFLABEL*)g_lb_password[ROW_NEW_PW]), nfui_nflabel_get_text((NFLABEL*)g_lb_password[ROW_CON_PW])))
		{
			nftool_mbox(g_curwnd, "NOTICE", "Password doesn't match confirmation.", NFTOOL_MB_OK);
			return FALSE;
		}

		if(strcmp(nfui_nflabel_get_text((NFLABEL*)g_lb_password[ROW_ORG_PW]), nfui_nflabel_get_text((NFLABEL*)g_lb_password[ROW_NEW_PW])) == 0)
		{
			ret = nftool_mbox(g_curwnd, "NOTICE", "The new password matches the current password.\nDo you want to continue?", NFTOOL_MB_YESNO);
			if (ret == NFTOOL_MB_NO)
			{
			    return FALSE;
			}
			
		}

		pw = nfui_nflabel_get_text((NFLABEL*)g_lb_password[ROW_NEW_PW]);
		strcpy(g_wizard_data->accData.pw, pw);

		DAL_set_user_passwd(g_wizard_data->accData.pw, 0);

		nftool_mbox(g_curwnd, "NOTICE", "New password successfully changed.", NFTOOL_MB_OK);
		
		_next_step_proc();
	}

	return FALSE;
}


gint vw_wizard_password_change_open(gpointer parent, gpointer user_data)
{
	NFOBJECT *main_wnd, *main_fixed;
	NFOBJECT *fixed1;
	NFOBJECT *obj;	
	NFOBJECT *btns[PIB_BUTTONS];

	const gchar *strLabel[] = {"USER ID", "PASSWORD", "NEW PASSWORD", "CONFIRM NEW PASSWORD"};
	const gchar *strButton[] = {"PREVIOUS", "NEXT", "FINISH"};

	gint pos_x,pos_y,size_w,size_h;
    gint i, j = 0;

    g_wizard_data = (WIZARD_USERDATA_T*)user_data;

	main_wnd = nftool_create_popup_window(parent, SE_PP_POS_X, SE_PP_POS_Y, PI_WND_SIZE_WID, PI_WND_SIZE_HEI, g_wizard_data->title, FALSE);
	nfui_nfwindow_set_title(main_wnd, "NETWORK SETUP WIZARD INIT");
	nfui_regi_post_event_callback(main_wnd, post_win_event_handler);
	g_curwnd = main_wnd;

	main_fixed = ((NFWINDOW*)main_wnd)->child;

	fixed1 = nfui_nffixed_new();
	nfui_nfobject_set_size(fixed1, PI_FIXED_SIZE_WID, PI_FIXED_SIZE_HEI);
	nfui_nffixed_put((NFFIXED*)main_fixed, fixed1, PI_FIXED_POS_X, PI_FIXED_POS_Y);
	nfui_nfobject_show(fixed1);

	pos_x = (guint)CATEGORY_LABEL_LEFT;
	pos_y = (guint)4;

	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG3);
	nfui_nfimage_set_text((NFIMAGE*)obj, "CHANGE PASSWORD");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);

    pos_x = SUBJECT_LABEL_LEFT;
    pos_y += CATEGORY_CONTENT_GAP;
    
    for (i = 0; i < ROW_MAX; i++)
    {
    	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strLabel[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, SUBJECT_LABEL_MARGIN);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);

        if (i == ROW_USER_ID)
        {
            obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(g_wizard_data->accData.userid, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(139));
            nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(138));
            nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
            nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
            nfui_nfobject_support_multi_lang(obj, FALSE);
            nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH1, IPS_LABEL_HEIGHT);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x + SUBJECT_LABEL_WIDTH + IPS_LABEL_ROW_SPACE, pos_y);
        }
        else
        {
            obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
			nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, SUBJECT_LABEL_MARGIN);
			nfui_nfobject_support_multi_lang((NFOBJECT*)obj, FALSE);
			nfui_nflabel_set_invisible((NFLABEL*)obj, TRUE);
			nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH1, IPS_LABEL_HEIGHT);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x + SUBJECT_LABEL_WIDTH + IPS_LABEL_ROW_SPACE, pos_y);
			nfui_regi_post_event_callback(obj, post_pw_label_event_handler);
            g_lb_password[i] = obj;
        }
        
        pos_y += IPS_LABEL_HEIGHT + IPS_LABEL_ROW_SPACE;
    }

	for( i=0; i<PIB_BUTTONS; i++ )
	{
		btns[i] = nftool_normal_button_create_type1(strButton[i], 160);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(btns[i]), NFALIGN_CENTER, 0);	
		nfui_nfobject_show(btns[i]);
	}
	
	nfui_nffixed_put((NFFIXED*)fixed1, btns[PIB_PREVIOUS], MENU_V_BTN_R3_X, MENU_V_BTN_Y);
	nfui_nffixed_put((NFFIXED*)fixed1, btns[PIB_NEXT], MENU_V_BTN_R2_X, MENU_V_BTN_Y);
	nfui_nffixed_put((NFFIXED*)fixed1, btns[PIB_EXIT], MENU_V_BTN_R1_X, MENU_V_BTN_Y);

	nfui_regi_post_event_callback(main_fixed, post_main_bg_event_handler);
	nfui_regi_post_event_callback(btns[PIB_EXIT], post_exitbutton_event_handler);
	nfui_regi_post_event_callback(btns[PIB_PREVIOUS], post_previousbutton_event_handler);
	nfui_regi_post_event_callback(btns[PIB_NEXT], post_nextbutton_event_handler);

	nfui_nfobject_show(main_wnd);

	/* set for key navi */
	nfui_make_key_hierarchy(main_wnd);
	nfui_set_key_focus(btns[PIB_NEXT], TRUE);

	gtk_main();

	return 0;

}

