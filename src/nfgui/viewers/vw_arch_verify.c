#include "nf_afx.h"

#include "scm.h"
#include "iux_msg.h"
#include "uxm.h"

//#include "services/uxm.h"

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
#include "viewers/objects/nfprogressbar.h"
#include "viewers/objects/nfcombobox.h"

#include "viewers/vw_arch_verify.h"

#include "modules/ssm.h"



// TODO: need new vkey
#include "vw_vkeyboard.h"

#define ARCH_VE_WIN_SIZE_W			(678)
#define ARCH_VE_WIN_SIZE_H			(220)

#define ARCH_VE_POS_X				((DISPLAY_ACTIVE_WIDTH - ARCH_VE_WIN_SIZE_W)/4*2)
#define ARCH_VE_POS_Y				((DISPLAY_ACTIVE_HEIGHT - ARCH_VE_WIN_SIZE_H)/2)


enum {
	VERIFY_BTN,
	CANCEL_BTN,
	BTN_COUNTS
};

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *win_obj;
static NFOBJECT *prog_obj;
static NFOBJECT *btn_obj[BTN_COUNTS];
static IMSG msg;
static ACPCTX cur_acp;
static AFILEID	afileid;
static int g_exit_key = 0;

static gboolean post_verify_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	int ret;

	if(evt->type == GDK_BUTTON_RELEASE) 
	{
		NFOBJECT *top = NULL;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

		nfui_nfobject_disable(btn_obj[VERIFY_BTN]);
		nfui_signal_emit(btn_obj[VERIFY_BTN], GDK_EXPOSE, FALSE);

		nfui_nfobject_disable(btn_obj[CANCEL_BTN]);
		nfui_signal_emit(btn_obj[CANCEL_BTN], GDK_EXPOSE, FALSE);
		
		ret = scm_verify_arch_file(cur_acp, afileid, IRET_ARCH_VERIFY);
		if (ret < 0) {
			NFOBJECT *top = NULL;
			top = nfui_nfobject_get_top(obj);
			nfui_nfobject_destroy(top);
		}
		g_exit_key = 1;
	}
	return FALSE;
}

static gboolean post_cancel_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) 
	{		
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

		NFOBJECT *top = NULL;
		
		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}

	return FALSE;
} 



static gboolean post_ve_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	//mb_type ret = -1;

	switch (evt->type) 
	{
		case GDK_EXPOSE:
		break;

		case NFEVENT_KEYPAD_RELEASE:
		case NFEVENT_REMOCON_RELEASE:
		{
			GdkEventKey *kevt;
			KEYPAD_KID kpid;

			kevt = (GdkEventKey*)evt;
			kpid = (KEYPAD_KID)kevt->keyval;

			if (g_exit_key == 1) return FALSE;

			if(kpid == KEYPAD_EXIT)
			{
				NFOBJECT *top = NULL;
		
				top = nfui_nfobject_get_top(obj);
				nfui_nfobject_destroy(top);
				return TRUE;
			}
		}
		break;

		case GDK_DELETE:
			g_curwnd = 0;
			gtk_main_quit();
		break;
		
		default:
		break;
	}

	return FALSE;
}

static void _set_progress_rate(NFOBJECT* prog, guint rate)
{
	NFOBJECT* label;
	gchar temp[10];	

	nfui_nfprogressbar_set_rate((NFPROGRESSBAR*)prog, rate);
	nfui_signal_emit(prog, GDK_EXPOSE, FALSE);

	return 0;
}


static gboolean 
post_ve_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	int result;
	int rate;
	NFOBJECT *top = NULL;
	
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	switch(evt->type) 
	{
		case GDK_EXPOSE:
		{
       		drawable = nfui_nfobject_get_window(obj);
    		gc = nfui_nfobject_get_gc(obj);

            nfui_nfobject_get_size(obj, &size_w, &size_h);
            pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
            nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
        
    		nfui_nfobject_gc_unref(gc);
    	}
		break;
		case IRET_ARCH_VERIFY:
		{
			int result;
			int box_ret;
			result = ((CMM_MESSAGE_T *)data)->param;

			if(result == 0)
			{
				box_ret = nftool_mbox(g_curwnd, "NOTICE", "Verify Success.", NFTOOL_MB_OK);
			}
			else if (result == -2)
			{
				box_ret = nftool_mbox(g_curwnd, "NOTICE", "Invalid watermark.", NFTOOL_MB_OK);
			}
			else
			{
				box_ret = nftool_mbox(g_curwnd, "NOTICE", "Verify Fail.", NFTOOL_MB_OK);
			}
			
			if(box_ret == NFTOOL_MB_OK)
			{
				NFOBJECT *top = NULL;
		
				top = nfui_nfobject_get_top(obj);
				nfui_nfobject_destroy(top);
			}
		}
		break;

		case INFY_VERIFY_RATE:
		{
			gint rate;
			
			rate = ((CMM_MESSAGE_T *)data)->param;
			printf("\n\n\n\n\n\n================ RATE !!! = : %d \n", rate);
			// 0 ~ 100 nomal, -1 complete
			if(rate >= 0 || rate <= 100)
			{
				nfui_nfprogressbar_set_rate((NFPROGRESSBAR*)prog_obj, rate);
				nfui_signal_emit(prog_obj, GDK_EXPOSE, FALSE);
			}

			_set_progress_rate(prog_obj, rate);
		}	
		break;
		
		case GDK_DELETE:
		{
            nfui_nfobject_get_size(obj, &size_w, &size_h);
            nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
		
			uxm_unreg_imsg_event(obj, IRET_ARCH_VERIFY);	
			uxm_unreg_imsg_event(obj, INFY_VERIFY_RATE);
        }
		break;

		
		break;
	}

	

	return FALSE;
}

static gboolean returnkey_proc(NFOBJECT *top, GdkEvent *event, gpointer data)
{
	if(!nfui_nfobject_is_disabled(btn_obj[CANCEL_BTN]))
		return TRUE;
	else
		return FALSE;
}

void VW_ArchVerify_Open(NFWINDOW *parent, ACPCTX acp, AFILEID id)
//void VW_ArchVerify_Open(gchar *full_name)
{
	NFOBJECT *fixed;
	NFOBJECT *obj;

	GdkPixbuf *prog_img[4];
	gint i;

	cur_acp = acp;
	afileid = id;

	g_exit_key = 0;

// windows
	win_obj = (NFOBJECT*)nfui_nfwindow_new(parent, 
										ARCH_VE_POS_X, ARCH_VE_POS_Y, 
										ARCH_VE_WIN_SIZE_W, ARCH_VE_WIN_SIZE_H);
	g_curwnd = win_obj;
	nfui_regi_post_event_callback(win_obj, post_ve_win_event_handler);
	nfui_nfwindow_set_returnkey_proc((NFWINDOW*)win_obj, returnkey_proc);


// fixed 
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, ARCH_VE_WIN_SIZE_W, ARCH_VE_WIN_SIZE_H);
	nfui_regi_post_event_callback(fixed, post_ve_fixed_event_cb);
	nfui_nfobject_show(fixed);

// title	
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("VERIFY INTEGRITY", 
									nffont_get_pango_font(NFFONT_LARGE_SEMI), 
									COLOR_IDX(205));
	nfui_nfobject_set_size(obj, 670, 36);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 11);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 4, 4);

	// progressbar
	prog_img[0] = nfui_get_image_from_file((IMG_PROGRESS_BG), NULL);
	prog_img[1] = nfui_get_image_from_file((IMG_PROGRESS_HEAD), NULL);
	prog_img[2] = nfui_get_image_from_file((IMG_PROGRESS_MIDDLE), NULL);
	prog_img[3] = nfui_get_image_from_file((IMG_PROGRESS_TAIL), NULL);

	prog_obj = nfui_nfprogressbar_new_with_images(prog_img[0], prog_img[1], prog_img[2], prog_img[3]);
	nfui_nfobject_show(prog_obj);
	nfui_nffixed_put((NFFIXED*)fixed, prog_obj, 15, 85);


	/* button */
	btn_obj[VERIFY_BTN] = nftool_normal_button_create_type1("VERIFY INTEGRITY", 230);
	nfui_regi_post_event_callback(btn_obj[VERIFY_BTN], post_verify_button_event_cb);
	nfui_nfobject_show(btn_obj[VERIFY_BTN]);
	nfui_nffixed_put((NFFIXED*)fixed, btn_obj[VERIFY_BTN], 100, 146);

	btn_obj[CANCEL_BTN] = nftool_normal_button_create_type1("CANCEL", 230);
	nfui_regi_post_event_callback(btn_obj[CANCEL_BTN], post_cancel_button_event_cb);
	nfui_nfobject_show(btn_obj[CANCEL_BTN]);
	nfui_nffixed_put((NFFIXED*)fixed, btn_obj[CANCEL_BTN], 340, 146);


	nfui_nfwindow_add((NFWINDOW*)win_obj, fixed);
	nfui_run_main_event_handler(win_obj);
	nfui_nfobject_show(win_obj);

	uxm_reg_imsg_event(fixed, INFY_VERIFY_RATE);
	uxm_reg_imsg_event(fixed, IRET_ARCH_VERIFY);
	
	nfui_make_key_hierarchy((NFWINDOW*)win_obj);
	nfui_set_key_focus(btn_obj[VERIFY_BTN], TRUE);
	nfui_page_open(PGID_ARCH_VERIFY, win_obj, ssm_get_cur_id(NULL));
	gtk_main();

	nfui_page_close(PGID_ARCH_VERIFY, win_obj);
}


