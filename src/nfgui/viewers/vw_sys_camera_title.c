#include <string.h>
#include <math.h>
#include "nf_afx.h"

#include "vw_sys_camera_main.h"
#include "vw_sys_camera_title.h"
#include "vw_sys_camera_image.h"

#include "nf_notify.h"

#include "tools/nf_ui_tool.h"

#include "support/color.h"
#include "support/nf_ui_font.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"

#include "objects/nfobject.h"
#include "objects/nfbutton.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfimage.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfimglabel.h"
#include "objects/nfspinbutton.h"

#include "vw_vkeyboard.h"
#include "scm.h"
#include "ix_mem.h"

#include "vsm.h"
#include "vw_menu.h"
#include "vw.h"

#define	CAM_SETUP_COLUMNS		2

#define MAX_STRING_SIZE			16

#define CAM_VIDEO_FIXED_X		8
#define CAM_VIDEO_FIXED_Y		0

#define CAM_VIDEO_FIXED_W		1440
#define CAM_VIDEO_FIXED_H		780

#define CAM_TITLE_LABEL_X		10
#define CAM_TITLE_LABEL_Y		10

#if defined(GUI_4CH_SUPPORT)
#define CAM_FIXED_COL_CNT		2
#define CAM_FIXED_ROW_CNT		2
#elif defined(GUI_8CH_SUPPORT)
#define CAM_FIXED_COL_CNT		3
#define CAM_FIXED_ROW_CNT		3
#elif defined(GUI_16CH_SUPPORT)
#define CAM_FIXED_COL_CNT		4
#define CAM_FIXED_ROW_CNT		4
#elif defined(GUI_32CH_SUPPORT)
#define CAM_FIXED_COL_CNT		6
#define CAM_FIXED_ROW_CNT		6
#endif

#define CAM_VIDEO_SIZE_W		(CAM_VIDEO_FIXED_W / CAM_FIXED_COL_CNT)
#define CAM_VIDEO_SIZE_H		(CAM_VIDEO_FIXED_H / CAM_FIXED_ROW_CNT)

enum {
	CSB_CANCEL = 0,
	CSB_APPLY,
	CSB_CLOSE,
	CSB_BUTTONS
};

static CameraData camdata[GUI_CHANNEL_CNT];
static CameraData org_camdata[GUI_CHANNEL_CNT];

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_video_fixed;
static NFOBJECT *title[GUI_CHANNEL_CNT];
static NFOBJECT *check_obj;
static gboolean is_show;

static gboolean start_preview = FALSE;


static void prvSetDataToObjects()
{
	guint i;
	gchar strBuf[16];

	for(i=0; i<GUI_CHANNEL_CNT; i++)
	{
		if(i >= 9)	sprintf(strBuf, "CH %d ", i+1);
		else		sprintf(strBuf, "CH %d", i+1);

		nfui_nfimglabel_set_text((NFIMGLABEL*)(title[i]), camdata[i].title);
	}
}

static void prvLoadDataFromObjects()
{
	guint i;

	for(i=0; i<GUI_CHANNEL_CNT; i++)
	{
		g_stpcpy(camdata[i].title, nfui_nfimglabel_get_text((NFIMGLABEL*)(title[i])));
	}
}

static gint _start_preview()
{
	guint ch_mask = 0;
	gint i;

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
		ch_mask |= (1 << i);

	vsm_live_preview_start(ch_mask, 
        MENU_V_PAGE_X+MENU_V_INNER_X+CAM_VIDEO_FIXED_X, 
        MENU_V_PAGE_Y+MENU_V_INNER_Y+CAM_VIDEO_FIXED_Y, 
        CAM_VIDEO_FIXED_W, 
        CAM_VIDEO_FIXED_H);

    return 0;
}

static gint _stop_preview()
{
    vsm_live_preview_stop();
    return 0;
}

static gint _show_video_obj()
{
	nfui_nfobject_modify_bg(g_video_fixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));

    return 0;
}

static gint _hide_video_obj()
{
    nfui_nfobject_modify_bg(g_video_fixed, NFOBJECT_STATE_NORMAL, VIDEO_BG_COLOR);
	nfui_signal_emit(g_video_fixed, GDK_EXPOSE, TRUE);

    return 0;
}

static gboolean post_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case GDK_EXPOSE :
		break;

		case GDK_DELETE:	
		break;
			
		default :
			break;
	}

	return FALSE;
}

static gboolean post_video_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case GDK_EXPOSE :
		{
			GdkGC *gc;
			GdkDrawable *drawable;
            gint x1, y1, x2, y2;
            gint w = 0, h = 0;
            guint i, div_cnt;

			drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
			gc = gdk_gc_new(drawable);

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_394E4A)));
			gdk_gc_set_line_attributes(gc, 2, GDK_LINE_SOLID, GDK_CAP_NOT_LAST, GDK_JOIN_MITER);

			if (var_get_ch_count() == 8) div_cnt = 3;
			else if (var_get_ch_count() == 32) div_cnt = 6;
			else div_cnt = sqrt(var_get_ch_count());

			w = CAM_VIDEO_FIXED_W / div_cnt;
			h = CAM_VIDEO_FIXED_H / div_cnt;
			
			x1 = x2 = MENU_V_PAGE_X+MENU_V_INNER_X+CAM_VIDEO_FIXED_X+1;
			y1 = y2 = MENU_V_PAGE_Y+MENU_V_INNER_Y+CAM_VIDEO_FIXED_Y;

			for (i = 0; i < div_cnt-1; i++) 
			{
				gdk_draw_line(drawable, gc, x1 += w, y1, x2 += w, y1+CAM_VIDEO_FIXED_H);
			}

			x1 = x2 = MENU_V_PAGE_X+MENU_V_INNER_X+CAM_VIDEO_FIXED_X;
			y1 = y2 = MENU_V_PAGE_Y+MENU_V_INNER_Y+CAM_VIDEO_FIXED_Y;
			
			for (i = 0; i < div_cnt-1; i++) 
			{       
				gdk_draw_line(drawable, gc, x1, y1 += h, x1+CAM_VIDEO_FIXED_W, y2 += h);
			}
			
			g_object_unref(gc);
		}
		break;

		case GDK_DELETE:
		break;
			
		default :
			break;
	}

	return FALSE;

}

static gboolean post_titlelabel_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;
    gchar *title;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER )
	{
		NFOBJECT *top;
		gchar *strTemp;
		guint x, y;
        mb_type ret = NFTOOL_MB_OK;

		if(kpid == KEYPAD_ENTER)
		{
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);

			x += (obj->width)/2 + top->x;
			y += obj->height + top->y;
		}
		else
		{
	  	   	if(evt->button.button == MOUSE_RIGTH_BUTTON)
			{					
				return FALSE;
	  	   	}
		
			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += (guint)((GdkEventButton*)evt)->x;
			y += (guint)((GdkEventButton*)evt)->y;
		}
/*
        title = nfui_nfimglabel_get_text((NFIMGLABEL*)obj);
       
        if (!ifn_is_all_itxstyle_title(title))
        {
            vw_mbox(g_curwnd, "ERROR", IMBX_UNSUPPORTED_LETTER, NFTOOL_MB_OK);        
            return FALSE;
        }
*/
        title = nfui_nfimglabel_get_text((NFIMGLABEL*)obj);
       
        if (!ifn_is_all_itxstyle_title(title))
		{
    		//strTemp = VirtualKey_Open(g_curwnd, "", x, y, MAX_STRING_SIZE, VKEY_CAMTITLE_SPACE);
    		strTemp = VirtualKey_Open(g_curwnd, "", x, y, MAX_STRING_SIZE, VKEY_CAMTITLE_SPACE|VKEY_MULTIKEYPD);
		}
        else
        {
    		//strTemp = VirtualKey_Open(g_curwnd, title, x, y, MAX_STRING_SIZE, VKEY_CAMTITLE_SPACE);        
    		strTemp = VirtualKey_Open(g_curwnd, title, x, y, MAX_STRING_SIZE, VKEY_CAMTITLE_SPACE|VKEY_MULTIKEYPD);        
        }

		if (strTemp) 
		{
			if(strlen(strTemp) <= 0) 
				ret = nftool_mbox(g_curwnd, "NOTICE", "There is no input.\nDo you want to continue?", NFTOOL_MB_OKCANCEL);

			if (ret == NFTOOL_MB_OK)
			{
				nfui_nfimglabel_set_text((NFIMGLABEL*)obj, strTemp);
        		nfui_nfobject_support_multi_lang((NFOBJECT*)obj, FALSE);		
				nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
			}
		
			ifree(strTemp);
			strTemp = NULL;
		}
	}
	return FALSE;
}

static gboolean 
post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint i;

	    if(evt->button.button == MOUSE_RIGTH_BUTTON) 
		{
		  return FALSE;
	    }
		
		g_memmove(camdata, org_camdata, sizeof(CameraData)*GUI_CHANNEL_CNT);
		prvSetDataToObjects();
			
		for(i=0; i<GUI_CHANNEL_CNT; i++)
		{
			nfui_signal_emit(title[i], GDK_EXPOSE, TRUE);
		}
	}

	return FALSE;
}

static gboolean 
post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint i;
		gint f_title=0, f_covert=0;

	    if(evt->button.button == MOUSE_RIGTH_BUTTON) 
		{					
			return FALSE;
	    }

		prvLoadDataFromObjects(); 

		is_show = nfui_check_button_get_active(check_obj);
		
		
		gboolean old_is_show = DAL_get_cam_show();
		
		if(memcmp(org_camdata, camdata, sizeof(CameraData)*GUI_CHANNEL_CNT) || 
			is_show != old_is_show)	

		{
			for(i=0; i<GUI_CHANNEL_CNT; i++)
			{
				if(strcmp(org_camdata[i].title, camdata[i].title))
					f_title = 1;

				if(org_camdata[i].covert != camdata[i].covert)
					f_covert = 1;
			}

			if(f_title)	scm_put_log(CHANGE_CAM_TITLE, 0, 0);

			g_memmove(org_camdata, camdata, sizeof(CameraData)*GUI_CHANNEL_CNT);
			DAL_set_camtitle_data_all(camdata, GUI_CHANNEL_CNT);

			guint ret = DAL_set_cam_show(is_show);

			nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
			syscam_set_changeflag(1);
		}
	}

	return FALSE;
	
}

static gboolean 
post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
		{
			return FALSE;
		}
	
		CamTitle_tab_out_handler();
		SystemSetupCam_Destroy(obj);
	}

	return FALSE;
	
}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
	}

	return FALSE;
}


///////////////////////////////////////////////////////////////////////
//
//
//

void init_CamSetup_page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *video_fixed;
	NFOBJECT *ntb;
	NFOBJECT *title_object[CAM_SETUP_COLUMNS];
	NFOBJECT *camsetup_btns[CSB_BUTTONS];

	NFOBJECT *lbTemp;

	const gchar *strButton[] = {"CANCEL", "APPLY", "CLOSE"};

	gchar strBuf[16];

	guint spin_w, spin_h;
	guint i;

	GdkPixbuf *chk_img[NFCHECK_STATES];
	GdkPixbuf *pbCamImage[32];


	g_curwnd = nfui_nfobject_get_top(parent);


// CAMERA IMAGE LOAD
	pbCamImage[0] 	= nfui_get_image_from_file(IMG_CAMERA_INPUT_ICON_01, NULL); 
	pbCamImage[1] 	= nfui_get_image_from_file(IMG_CAMERA_INPUT_ICON_02, NULL); 
	pbCamImage[2] 	= nfui_get_image_from_file(IMG_CAMERA_INPUT_ICON_03, NULL); 
	pbCamImage[3] 	= nfui_get_image_from_file(IMG_CAMERA_INPUT_ICON_04, NULL); 
	pbCamImage[4] 	= nfui_get_image_from_file(IMG_CAMERA_INPUT_ICON_05, NULL); 		
	pbCamImage[5] 	= nfui_get_image_from_file(IMG_CAMERA_INPUT_ICON_06, NULL); 
	pbCamImage[6] 	= nfui_get_image_from_file(IMG_CAMERA_INPUT_ICON_07, NULL); 
	pbCamImage[7] 	= nfui_get_image_from_file(IMG_CAMERA_INPUT_ICON_08, NULL); 
	pbCamImage[8] 	= nfui_get_image_from_file(IMG_CAMERA_INPUT_ICON_09, NULL); 
	pbCamImage[9] 	= nfui_get_image_from_file(IMG_CAMERA_INPUT_ICON_10, NULL); 	
	pbCamImage[10] 	= nfui_get_image_from_file(IMG_CAMERA_INPUT_ICON_11, NULL); 
	pbCamImage[11] 	= nfui_get_image_from_file(IMG_CAMERA_INPUT_ICON_12, NULL); 
	pbCamImage[12] 	= nfui_get_image_from_file(IMG_CAMERA_INPUT_ICON_13, NULL); 
	pbCamImage[13] 	= nfui_get_image_from_file(IMG_CAMERA_INPUT_ICON_14, NULL); 
	pbCamImage[14] 	= nfui_get_image_from_file(IMG_CAMERA_INPUT_ICON_15, NULL); 		
	pbCamImage[15] 	= nfui_get_image_from_file(IMG_CAMERA_INPUT_ICON_16, NULL); 
    pbCamImage[16] = nfui_get_image_from_file(IMG_CAMERA_INPUT_ICON_17, NULL); 
    pbCamImage[17] = nfui_get_image_from_file(IMG_CAMERA_INPUT_ICON_18, NULL); 
    pbCamImage[18] = nfui_get_image_from_file(IMG_CAMERA_INPUT_ICON_19, NULL); 
    pbCamImage[19] = nfui_get_image_from_file(IMG_CAMERA_INPUT_ICON_20, NULL); 
    pbCamImage[20] = nfui_get_image_from_file(IMG_CAMERA_INPUT_ICON_21, NULL);         
    pbCamImage[21] = nfui_get_image_from_file(IMG_CAMERA_INPUT_ICON_22, NULL); 
    pbCamImage[22] = nfui_get_image_from_file(IMG_CAMERA_INPUT_ICON_23, NULL); 
    pbCamImage[23] = nfui_get_image_from_file(IMG_CAMERA_INPUT_ICON_24, NULL); 
    pbCamImage[24] = nfui_get_image_from_file(IMG_CAMERA_INPUT_ICON_25, NULL); 
    pbCamImage[25] = nfui_get_image_from_file(IMG_CAMERA_INPUT_ICON_26, NULL);     
    pbCamImage[26] = nfui_get_image_from_file(IMG_CAMERA_INPUT_ICON_27, NULL); 
    pbCamImage[27] = nfui_get_image_from_file(IMG_CAMERA_INPUT_ICON_28, NULL); 
    pbCamImage[28] = nfui_get_image_from_file(IMG_CAMERA_INPUT_ICON_29, NULL); 
    pbCamImage[29] = nfui_get_image_from_file(IMG_CAMERA_INPUT_ICON_30, NULL); 
    pbCamImage[30] = nfui_get_image_from_file(IMG_CAMERA_INPUT_ICON_31, NULL);
    pbCamImage[31] = nfui_get_image_from_file(IMG_CAMERA_INPUT_ICON_32, NULL);  


// FIXED
	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(content_fixed, MENU_V_INNER_W, MENU_V_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);

	video_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(video_fixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
	nfui_nfobject_set_size(video_fixed, CAM_VIDEO_FIXED_W, CAM_VIDEO_FIXED_H);
	nfui_nfobject_show(video_fixed);
	nfui_nffixed_put((NFFIXED*)content_fixed, video_fixed, CAM_VIDEO_FIXED_X, CAM_VIDEO_FIXED_Y);
	g_video_fixed = video_fixed;

	memset(camdata, 0x00, sizeof(CameraData)*GUI_CHANNEL_CNT);
	memset(org_camdata, 0x00, sizeof(CameraData)*GUI_CHANNEL_CNT);

	for(i=0; i<GUI_CHANNEL_CNT; i++)
	{
		DAL_get_camtitle_data(&camdata[i], i);
	}

	guint xpos = CAM_TITLE_LABEL_X;
	guint ypos = CAM_TITLE_LABEL_Y;
	
	for(i=0; i<GUI_CHANNEL_CNT; i++)
	{
		title[i] = (NFOBJECT*)nfui_nfimglabel_new(pbCamImage[i], camdata[i].title);
		nfui_nfimglabel_set_align((NFIMGLABEL*)title[i], NFALIGN_LEFT);
		nfui_nfimglabel_set_pango_font((NFIMGLABEL*)title[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(129));
        nfui_nfobject_support_multi_lang((NFOBJECT*)title[i], FALSE);		
		nfui_nfobject_set_size(title[i], 210, 40);
		nfui_nfobject_modify_bg(title[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(128));
		nfui_nfobject_show(title[i]);
		nfui_regi_post_event_callback(title[i], post_titlelabel_event_handler);

		nfui_nffixed_put((NFFIXED*)video_fixed, title[i], xpos, ypos);

		if (((i+1)%CAM_FIXED_COL_CNT) == 0)
		{
			xpos = CAM_TITLE_LABEL_X;
			ypos += CAM_VIDEO_SIZE_H;
		}
		else
		{
			xpos += CAM_VIDEO_SIZE_W;
		}		
	}
		
	camsetup_btns[0] = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(camsetup_btns[0]), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(camsetup_btns[0]);
	nfui_nffixed_put((NFFIXED*)parent, camsetup_btns[0], MENU_V_BTN_R3_X, MENU_V_BTN_Y);

	camsetup_btns[1] = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(camsetup_btns[1]), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(camsetup_btns[1]);
	nfui_nffixed_put((NFFIXED*)parent, camsetup_btns[1], MENU_V_BTN_R2_X, MENU_V_BTN_Y);

	camsetup_btns[2] = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(camsetup_btns[2]), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(camsetup_btns[2]);
	nfui_nffixed_put((NFFIXED*)parent, camsetup_btns[2], MENU_V_BTN_R1_X, MENU_V_BTN_Y);

	nfui_regi_post_event_callback(camsetup_btns[0], post_cancelbutton_event_handler);
	nfui_regi_post_event_callback(camsetup_btns[1], post_applybutton_event_handler);
	nfui_regi_post_event_callback(camsetup_btns[2], post_closebutton_event_handler);

	nfui_regi_post_event_callback(content_fixed, post_fixed_event_handler);
	nfui_regi_post_event_callback(video_fixed, post_video_fixed_event_handler);

	nfui_regi_post_event_callback(parent, post_page_event_handler);

	g_memmove(org_camdata, camdata, sizeof(CameraData)*GUI_CHANNEL_CNT);
	
}

static gboolean _init_preview(gpointer data)
{
   _start_preview();
//    _show_video_obj();   
	return FALSE;
}

gint CamSetup_start_preview()
{
    _start_preview();
    _show_video_obj();    
//	g_timeout_add(2000, _init_preview, 0);
	return 0;
}

gboolean CamTitle_tab_in_handler()
{
    _start_preview();
    _show_video_obj();    

	return FALSE;
}

gboolean CamTitle_tab_out_handler()
{
	mb_type ret;
	guint i;
	gint f_title=0, f_covert=0;

    _hide_video_obj();    
	_stop_preview();
	
	prvLoadDataFromObjects();

	if(!memcmp(org_camdata, camdata, sizeof(CameraData)*GUI_CHANNEL_CNT))
		return FALSE;

	ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", 
							NFTOOL_MB_OKCANCEL);

	if(ret == NFTOOL_MB_OK)
	{
		for(i=0; i<GUI_CHANNEL_CNT; i++)
		{
			if(strcmp(org_camdata[i].title, camdata[i].title))
				f_title = 1;

			//if(org_camdata[i].covert != camdata[i].covert)
			//	f_covert = 1;
		}

		if(f_title)	scm_put_log(CHANGE_CAM_TITLE, 0, 0);
		

		g_memmove(org_camdata, camdata, sizeof(CameraData)*GUI_CHANNEL_CNT);
		DAL_set_camtitle_data_all((CameraData**)(&camdata), GUI_CHANNEL_CNT);
		syscam_set_changeflag(1);
	}
	else if(ret == NFTOOL_MB_CANCEL)
	{
		g_memmove(camdata, org_camdata, sizeof(CameraData)*GUI_CHANNEL_CNT);
		prvSetDataToObjects();
	}

	return FALSE;
}


