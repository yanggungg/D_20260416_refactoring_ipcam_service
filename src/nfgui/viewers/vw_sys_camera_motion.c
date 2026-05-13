
#include "nf_afx.h"

#include "vw_sys_camera_main.h"
#include "vw_sys_camera_motion.h"
#include "vw_motion_sensor_conf.h"
#include "vw_motion_sensor_area.h"

#include "tools/nf_ui_tool.h"

#include "support/color.h"
#include "support/nf_ui_font.h"
#include "support/event_loop.h"
#include "support/util.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"
#include "objects/nfspinbutton.h"
#include "objects/nflabel.h"
#include "objects/nfimglabel.h"
#include "objects/nfbutton.h"
#include "scm.h"
#include "vsm.h"



#define	NUM_MOTSEN_COLUMNS					(5)

#define	MOTSEN_COL_SPACE			(2)
#define	MOTSEN_ROW_SPACE			(1)

#define	MOTSEN_TABLE_LEFT			(8)
#define	MOTSEN_TABLE_TOP			(39)

#define	MOTSEN_LABEL_HEIGHT			(40)

enum {
	MSB_CANCEL = 0,
	MSB_APPLY,
	MSB_CLOSE,
	MSB_BUTTONS
};


static MotionData motdata[GUI_CHANNEL_CNT];
static MotionData org_motdata[GUI_CHANNEL_CNT];

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *cam_ch[GUI_CHANNEL_CNT];
static NFOBJECT *act[GUI_CHANNEL_CNT];
static NFOBJECT *detect[GUI_CHANNEL_CNT];
static NFOBJECT *sens[GUI_CHANNEL_CNT];
static NFOBJECT *mini[GUI_CHANNEL_CNT];
static NFOBJECT *area[GUI_CHANNEL_CNT];


static void prvSetDataToObjects(gint expose)
{
	guint i;
	gchar strBuf[16];
	gchar strSen[20];
	gchar strMini[20];

	for(i=0; i<GUI_CHANNEL_CNT; i++)
	{
		if (expose)
		{
			nfui_spin_button_set_index(act[i], motdata[i].act);
			nfui_spin_button_set_index(detect[i], motdata[i].detect);
		}
		else
		{
			nfui_spin_button_set_index_no_expose(act[i], motdata[i].act);
			nfui_spin_button_set_index_no_expose(detect[i], motdata[i].detect);
		}
				
		g_sprintf(strSen, "%d / %d", motdata[i].sense_d, motdata[i].sense_n);
		g_sprintf(strMini, "%d / %d", motdata[i].mini_d, motdata[i].mini_n);

		nfui_nflabel_set_text((NFLABEL*)sens[i], strSen);
		nfui_nflabel_set_text((NFLABEL*)mini[i], strMini);

		if (expose) nfui_signal_emit(sens[i], GDK_EXPOSE, TRUE);
		if (expose) nfui_signal_emit(mini[i], GDK_EXPOSE, TRUE);
	}
}

static void prvLoadDataFromObjects()
{
	guint i;

	for(i=0; i<GUI_CHANNEL_CNT; i++)
	{
		motdata[i].act = nfui_spin_button_get_index(act[i]);
		motdata[i].detect = nfui_spin_button_get_index(detect[i]);
	}
}

static void redisp_sen_n_mblocks()
{
	gint i;
	gchar buf[16];

	for(i=0; i<GUI_CHANNEL_CNT; i++) {
		g_sprintf(buf, "%d / %d", motdata[i].sense_d, motdata[i].sense_n);

		nfui_nflabel_set_text(sens[i], buf);
		nfui_signal_emit(sens[i], GDK_EXPOSE, FALSE);
	}

	for(i=0; i<GUI_CHANNEL_CNT; i++) {
		g_sprintf(buf, "%d / %d", motdata[i].mini_d, motdata[i].mini_n);

		nfui_nflabel_set_text(mini[i], buf);
		nfui_signal_emit(mini[i], GDK_EXPOSE, FALSE);
	}
}

static gboolean 
post_area_btn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		gint ch;
		guint ch_mask = 0;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
		  return FALSE;

		for(ch=0; ch<GUI_CHANNEL_CNT; ch++)
		{
			if(obj == area[ch])
				break;
		}

		ch_mask |= (1 << ch);			
		nf_ipcam_set_mraw_ch(ch);
		vsm_live_preview_start(ch_mask, 0, 0, DISPLAY_ACTIVE_WIDTH, DISPLAY_ACTIVE_HEIGHT);

#if defined(_SUPPORT_GUI_MDRAW)
		VW_MotionDraw_init(g_curwnd);
#endif
		VW_MotionSensorArea_Open(g_curwnd, ch, motdata);

        nf_ipcam_set_mraw_ch(0xff);
		vsm_live_preview_stop();

		if(memcmp(org_motdata, motdata, sizeof(MotionData)*GUI_CHANNEL_CNT))
			redisp_sen_n_mblocks();
	}

	return FALSE;
}

static gboolean mainbg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    switch(evt->type)
	{
		case GDK_EXPOSE :
		break;

        case INFY_CAMDB_CHANGE_NOTIFY:
        case INFY_USRDB_CHANGE_NOTIFY:
        {
        	gint i;
            gchar strBuf[STRING_SIZE_CAMTITLE];

        	for (i = 0; i < GUI_CHANNEL_CNT; i++)
        	{
        	    memset(strBuf, 0x00, sizeof(strBuf));
                var_get_camtitle(strBuf, i);
        		nfui_nfimglabel_set_text((NFIMGLABEL*)cam_ch[i], strBuf);
        		nfui_signal_emit(cam_ch[i], GDK_EXPOSE, TRUE);
            }
        }       
        break;
        
		case GDK_DELETE:
            uxm_unreg_imsg_event(obj, INFY_CAMDB_CHANGE_NOTIFY);		
            uxm_unreg_imsg_event(obj, INFY_USRDB_CHANGE_NOTIFY);		
		break;
			
		default :
			break;
	}


	return FALSE;

}

static gboolean 
post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint i;
        MotionData tmp_motdata[GUI_CHANNEL_CNT];
		
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

    	for(i=0; i<GUI_CHANNEL_CNT; i++)
    		DAL_get_motionsensor_data(&tmp_motdata[i], i);

    	if (memcmp(tmp_motdata, org_motdata, sizeof(MotionData)*GUI_CHANNEL_CNT))
    	{
            DAL_set_motion_data_all(org_motdata, GUI_CHANNEL_CNT);

            for (i = 0; i < GUI_CHANNEL_CNT; i++)
                DAL_notify_fire_DB_sync(NF_SYSDB_TMP_CHANGE_EVENTID_MOTION, i);            
        }

		g_memmove(motdata, org_motdata, sizeof(MotionData)*GUI_CHANNEL_CNT);
			
		prvSetDataToObjects(1);
 	}

	return FALSE;
}

static gboolean 
post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint i;
		
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

		prvLoadDataFromObjects();

		if(memcmp(org_motdata, motdata, sizeof(MotionData)*GUI_CHANNEL_CNT))
		{
			scm_put_log(CHANGE_CAM_MOTION, 0, 0);

			g_memmove(org_motdata, motdata, sizeof(MotionData)*GUI_CHANNEL_CNT);
			DAL_set_motion_data_all(motdata, GUI_CHANNEL_CNT);
            DAL_notify_fire_DB_change(NF_SYSDB_CATE_ALARM);

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
		if( evt->button.button == MOUSE_RIGTH_BUTTON )
			return FALSE;
	
		MotSen_tab_out_handler();

		SystemSetupCam_Destroy(obj);
	}

	return FALSE;
}

static gboolean 
post_sensmini_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;
	gint ch;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

		for(ch=0; ch<GUI_CHANNEL_CNT; ch++) {
			if(obj == sens[ch] || obj == mini[ch]) 
				break;
		}
		
		VW_MotSen_Conf_Open(g_curwnd, ch, motdata);

		if(memcmp(org_motdata, motdata, sizeof(MotionData)*GUI_CHANNEL_CNT))
			redisp_sen_n_mblocks();
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


void init_MotSen_page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *ntb;
	NFOBJECT *title_object[NUM_MOTSEN_COLUMNS];
	NFOBJECT *motsen_btns[MSB_BUTTONS];
	NFOBJECT *lbTemp;
	
	GdkPixbuf *pbCamImage[32];	
	const gchar *strTitle[] = { "CHANNEL",
								"ACTIVATION",
								"DETECTING MARK",
								"SENSITIVITY",
								"MINIMUM BLOCKS"};
	
	const gchar *strOffOn[] = {"OFF", "ON"};
	const gchar *strDetect[] = {"OFF", "YELLOW", "RED", "BLUE"};
	gchar strBuf[STRING_SIZE_CAMTITLE];
	gchar strSen[16];
	gchar strMini[16];

	guint width[NUM_MOTSEN_COLUMNS];
	guint btn_x, btn_y, btn_space;
	guint i;

#ifdef _IPX_MODEL_UX
	NFIPCamMotionProfile mot_profile;
	gint ret;
#endif

	g_curwnd = nfui_nfobject_get_top(parent);


	width[0] = 250;
	width[1] = 200;
	width[2] = 200;
	width[3] = 250;
	width[4] = 250;

// CAMERA IMAGE LOAD
	pbCamImage[0]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_01, NULL); 
	pbCamImage[1]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_02, NULL); 
	pbCamImage[2]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_03, NULL); 
	pbCamImage[3]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_04, NULL); 
	pbCamImage[4]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_05, NULL); 		
	pbCamImage[5]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_06, NULL); 
	pbCamImage[6]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_07, NULL); 
	pbCamImage[7]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_08, NULL); 
	pbCamImage[8]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_09, NULL); 
	pbCamImage[9]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_10, NULL); 	
	pbCamImage[10] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_11, NULL); 
	pbCamImage[11] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_12, NULL); 
	pbCamImage[12] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_13, NULL); 
	pbCamImage[13] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_14, NULL); 
	pbCamImage[14] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_15, NULL); 		
	pbCamImage[15] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_16, NULL);
    pbCamImage[16] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_17, NULL); 
    pbCamImage[17] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_18, NULL); 
    pbCamImage[18] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_19, NULL); 
    pbCamImage[19] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_20, NULL); 
    pbCamImage[20] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_21, NULL);         
    pbCamImage[21] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_22, NULL); 
    pbCamImage[22] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_23, NULL); 
    pbCamImage[23] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_24, NULL); 
    pbCamImage[24] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_25, NULL); 
    pbCamImage[25] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_26, NULL);     
    pbCamImage[26] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_27, NULL); 
    pbCamImage[27] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_28, NULL); 
    pbCamImage[28] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_29, NULL); 
    pbCamImage[29] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_30, NULL); 
    pbCamImage[30] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_31, NULL);             
    pbCamImage[31] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_32, NULL);    

	// DAL
	memset(motdata, 0x00, sizeof(MotionData)*GUI_CHANNEL_CNT);
	memset(org_motdata, 0x00, sizeof(MotionData)*GUI_CHANNEL_CNT);

	for(i=0; i<GUI_CHANNEL_CNT; i++)
	{
		DAL_get_motionsensor_data(&motdata[i], i);
	}
	
	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(content_fixed, MENU_V_INNER_W, MENU_V_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);
	
	// table
	ntb = nfui_nftable_new(NUM_MOTSEN_COLUMNS, GUI_CHANNEL_CNT+1, MOTSEN_COL_SPACE, MOTSEN_ROW_SPACE, width, MOTSEN_LABEL_HEIGHT);
	nfui_nfobject_show(ntb);
	nfui_nffixed_put((NFFIXED*)content_fixed, ntb, MOTSEN_TABLE_LEFT, MOTSEN_TABLE_TOP);

	// table row 0
	for(i=0; i<NUM_MOTSEN_COLUMNS; i++)
	{
		title_object[i] = nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
		nfui_nfobject_modify_bg(title_object[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_use_focus(title_object[i], NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(title_object[i]);
		nfui_nftable_attach((NFTABLE*)ntb, title_object[i], i, 0);
	}
	

	// table row 1 ~ ..
	btn_x = ntb->x + ntb->width + ((NFTABLE*)ntb)->col_space;
	btn_y = ntb->y + ((NFTABLE*)ntb)->cell_height + ((NFTABLE*)ntb)->row_space;

	for(i=0; i<GUI_CHANNEL_CNT; i++)
	{
		// ch
		//DAL_get_camera_title(strBuf, i);
		var_get_camtitle(strBuf, i);

		cam_ch[i] = (NFOBJECT*)nfui_nfimglabel_new(pbCamImage[i], strBuf);
		nfui_nfimglabel_set_align((NFIMGLABEL*)cam_ch[i], NFALIGN_LEFT);
		nfui_nfimglabel_set_pango_font((NFIMGLABEL*)cam_ch[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
		nfui_nfobject_use_focus(cam_ch[i], NFOBJECT_FOCUS_OFF);
		nfui_nfobject_modify_bg(cam_ch[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_show(cam_ch[i]);
		nfui_nftable_attach((NFTABLE*)ntb, cam_ch[i], 0, i+1);


		// activation
		act[i] = (NFOBJECT*)nfui_spinbutton_new((gchar**)strOffOn, 2, (gint)(motdata[i].act)); 
		nfui_spinbutton_set_skin_type((NFSPINBUTTON*)act[i], NFSPINBUTTON_TYPE_1);
		nfui_nfobject_show(act[i]);
		nfui_nftable_attach((NFTABLE*)ntb, act[i], 1, i+1);


		// detecting mark
		detect[i] = (NFOBJECT*)nfui_spinbutton_new((gchar**)strOffOn, 2, (gint)(motdata[i].detect));
		nfui_spinbutton_set_skin_type((NFSPINBUTTON*)detect[i], NFSPINBUTTON_TYPE_1);
		nfui_nfobject_show(detect[i]);
		nfui_nftable_attach((NFTABLE*)ntb, detect[i], 2, i+1);
		
		
		// sensitivity
		g_sprintf(strSen, "%d / %d", motdata[i].sense_d, motdata[i].sense_n);

		sens[i] =  (NFOBJECT*)nfui_nflabel_new_text_box(strSen, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)sens[i], NFTEXTBOX_TYPE_INPUT);
		nfui_regi_post_event_callback(sens[i], post_sensmini_event_handler);
		nfui_nfobject_show(sens[i]);
		nfui_nftable_attach((NFTABLE*)ntb, sens[i], 3, i+1);

		
		// minimum blocks
		g_sprintf(strMini, "%d / %d", motdata[i].mini_d, motdata[i].mini_n);
#ifdef _IPX_MODEL_UX
		ret = scm_get_ipcam_motion_profile(i, &mot_profile);
#endif
		mini[i] =  (NFOBJECT*)nfui_nflabel_new_text_box(strMini, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)mini[i], NFTEXTBOX_TYPE_INPUT);
		nfui_regi_post_event_callback(mini[i], post_sensmini_event_handler);
		nfui_nfobject_show(mini[i]);
#ifdef _IPX_MODEL_UX
		if (ret == 0)
		{
			if (mot_profile.min_block == 0) nfui_nfobject_disable(mini[i]);
		}
#endif
		nfui_nftable_attach((NFTABLE*)ntb, mini[i], 4, i+1);


		// area buttons
		area[i] = nftool_normal_button_create_type3("AREA SETUP", 192);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(area[i]), NFALIGN_CENTER, 0);	
		nfui_nfbutton_set_spacing((NFBUTTON*)area[i], SEMI_CONDENSED_SPACING);
		nfui_nfobject_show(area[i]);
		nfui_regi_post_event_callback(area[i], post_area_btn_event_handler);
		nfui_nffixed_put((NFFIXED*)content_fixed, area[i], btn_x, btn_y);


		btn_y += ((NFTABLE*)ntb)->cell_height + ((NFTABLE*)ntb)->row_space;
	}
		
	motsen_btns[0] = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(motsen_btns[0]), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(motsen_btns[0]);
	nfui_nffixed_put((NFFIXED*)parent, motsen_btns[0], MENU_V_BTN_R3_X, MENU_V_BTN_Y);

	motsen_btns[1] = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(motsen_btns[1]), NFALIGN_CENTER, 0);
	nfui_nfobject_show(motsen_btns[1]);
	nfui_nffixed_put((NFFIXED*)parent, motsen_btns[1], MENU_V_BTN_R2_X, MENU_V_BTN_Y);

	motsen_btns[2] = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(motsen_btns[2]), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(motsen_btns[2]);
	nfui_nffixed_put((NFFIXED*)parent, motsen_btns[2], MENU_V_BTN_R1_X, MENU_V_BTN_Y);

	nfui_regi_pre_event_callback(content_fixed, mainbg_event_handler);
	nfui_regi_post_event_callback(motsen_btns[0], post_cancelbutton_event_handler);
	nfui_regi_post_event_callback(motsen_btns[1], post_applybutton_event_handler);
	nfui_regi_post_event_callback(motsen_btns[2], post_closebutton_event_handler);

	nfui_regi_post_event_callback(parent, post_page_event_handler);

	uxm_reg_imsg_event(content_fixed, INFY_CAMDB_CHANGE_NOTIFY);
	uxm_monitor_on_imsg_event(content_fixed, INFY_CAMDB_CHANGE_NOTIFY);	
	uxm_reg_imsg_event(content_fixed, INFY_USRDB_CHANGE_NOTIFY);
	uxm_monitor_on_imsg_event(content_fixed, INFY_USRDB_CHANGE_NOTIFY);	

	g_memmove(org_motdata, motdata, sizeof(MotionData)*GUI_CHANNEL_CNT);
}

gboolean MotSen_tab_in_handler()
{

	return FALSE;
}

gboolean MotSen_tab_out_handler()
{
	mb_type ret;
	guint i;
    MotionData tmp_motdata[GUI_CHANNEL_CNT];

	prvLoadDataFromObjects();

	if(!memcmp(org_motdata, motdata, sizeof(MotionData)*GUI_CHANNEL_CNT))
		return FALSE;

	ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

	if(ret == NFTOOL_MB_OK)
	{
		scm_put_log(CHANGE_CAM_MOTION, 0, 0);

		g_memmove(org_motdata, motdata, sizeof(MotionData)*GUI_CHANNEL_CNT);
		DAL_set_motion_data_all(motdata, GUI_CHANNEL_CNT);
        DAL_notify_fire_DB_change(NF_SYSDB_CATE_ALARM);    
        
		syscam_set_changeflag(1);
	}
	else if(ret == NFTOOL_MB_CANCEL)
	{
    	for(i=0; i<GUI_CHANNEL_CNT; i++)
    		DAL_get_motionsensor_data(&tmp_motdata[i], i);

    	if (memcmp(tmp_motdata, org_motdata, sizeof(MotionData)*GUI_CHANNEL_CNT))
    	{
            DAL_set_motion_data_all(org_motdata, GUI_CHANNEL_CNT);

            for (i = 0; i < GUI_CHANNEL_CNT; i++)
                DAL_notify_fire_DB_sync(NF_SYSDB_TMP_CHANGE_EVENTID_MOTION, i);
        }
	
		g_memmove(motdata, org_motdata, sizeof(MotionData)*GUI_CHANNEL_CNT);
			
		prvSetDataToObjects(0);			
	}

	return FALSE;
}

