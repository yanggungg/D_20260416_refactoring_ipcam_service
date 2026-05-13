
#include "nf_afx.h"

#include "vw_sys_camera_main.h"
#include "vw_sys_camera_motion.h"
#include "vw_motion_sensor_conf.h"
#include "vw_motion_sensor_setup.h"

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
#include "objects/nfcombobox.h"
#include "objects/nflabel.h"
#include "objects/nfimglabel.h"
#include "objects/nfbutton.h"
#include "objects/nftab.h"
#include "scm.h"
#include "vsm.h"



#define PAGE_FIXED_CNT          2
#define ROW_CNT_PER_PAGE        (GUI_CHANNEL_CNT / PAGE_FIXED_CNT)

#define NUM_MOTSEN_COLUMNS          (3)

#define MOTSEN_COL_SPACE            (2)
#define MOTSEN_ROW_SPACE            (1)

#define MOTSEN_TABLE_LEFT           (27)
#define MOTSEN_TABLE_TOP            (40)

#define MOTSEN_LABEL_HEIGHT         (40)


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
static NFOBJECT *g_page_fixed[PAGE_FIXED_CNT];
static NFOBJECT *g_lb_page_num;



static void prvSetDataToObjects(gint expose)
{
    guint i;

    for(i=0; i<GUI_CHANNEL_CNT; i++)
    {
        if (motdata[i].act)
            nfui_nfobject_enable(detect[i]);
        else
            nfui_nfobject_disable(detect[i]);

        if (expose)
        {
            nfui_spin_button_set_index_no_expose(act[i], motdata[i].act);
            nfui_spin_button_set_index_no_expose(detect[i], motdata[i].detect);

            nfui_signal_emit(act[i], GDK_EXPOSE, TRUE);
            nfui_signal_emit(detect[i], GDK_EXPOSE, TRUE);
        }
        else
        {
            nfui_spin_button_set_index_no_expose(act[i], motdata[i].act);
            nfui_spin_button_set_index_no_expose(detect[i], motdata[i].detect);
        }
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
static void drawDetectAllObject(guint act)
{
    guint i;
    // set detect
    if(act)
    {
        for(i=0; i<GUI_CHANNEL_CNT; i++) 
            nfui_nfobject_enable(detect[i]);
    }
    else
    {
        for(i=0; i<GUI_CHANNEL_CNT; i++) 
            nfui_nfobject_disable(detect[i]);
    }

    for(i=0; i<GUI_CHANNEL_CNT; i++) 
        nfui_signal_emit(detect[i], GDK_EXPOSE, TRUE);
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

static gboolean post_on_off_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_COMBOBOX_CHANGED) {
        NFOBJECT **tmpObj;
        guint objIdx;
        gint idx;
        gint i;

        objIdx = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "title index"));
        switch(objIdx) {
            case 1: tmpObj = act; break;
            case 2: tmpObj = detect; break;
            default: return FALSE;
        }
        
		nfui_on_backscr(obj);
		nfui_rflip(obj);
    	
        idx = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        for(i=0; i<GUI_CHANNEL_CNT; i++) 
            nfui_spin_button_set_index((NFSPINBUTTON*)tmpObj[i], (guint)idx);

        //detect
        if(objIdx== 1)
            drawDetectAllObject(idx);
        
        for(i=0; i<GUI_CHANNEL_CNT; i++) 
            nfui_signal_emit(tmpObj[i], GDK_EXPOSE, TRUE);

		nfui_flip(obj);
		nfui_off_backscr(obj);
    }

    return FALSE;
}

static gboolean post_act_spinbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        guint idx;
        idx = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "channel"));
    
        if( nfui_spin_button_get_index(act[idx]) )
            nfui_nfobject_enable(detect[idx]);
        else
            nfui_nfobject_disable(detect[idx]);
        
        nfui_signal_emit(detect[idx], GDK_EXPOSE, TRUE);
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
    
        IPCam_MotSen_tab_out_handler();
        SystemSetupCam_Destroy(obj);
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

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_DELETE)
    {
        g_curwnd = 0;
    }

    return FALSE;
}



void init_MotSenAct_Page(NFOBJECT *parent)
{
    NFOBJECT *content_fixed;
    NFOBJECT *ntb;
    NFOBJECT *title_object[NUM_MOTSEN_COLUMNS];
    NFOBJECT *motsen_btns[MSB_BUTTONS];
    NFOBJECT *obj;
    GdkPixbuf *pbCamImage[32];
    NFOBJECT *main_page_fixed;
    NFOBJECT *page_fixed[PAGE_FIXED_CNT];
    NFOBJECT *page_ntb[PAGE_FIXED_CNT];

    const gchar *strTitle[] = { "CHANNEL",
                                "ACTIVATION",
                                "DETECTING MARK"};
    
    const gchar *strOffOn[] = {"OFF", "ON"};
    gchar strBuf[STRING_SIZE_CAMTITLE];
    guint width[NUM_MOTSEN_COLUMNS];
    guint i;
    gint size_w, size_h;
    gint pos_x, pos_y;
	gint page_num, row_num;
	GdkPixbuf *prev_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *next_img[NFOBJECT_STATE_COUNT];

#ifdef _IPX_MODEL_UX
    NFIPCamMotionProfile mot_profile;
    gint ret;
#endif

    g_curwnd = nfui_nfobject_get_top(parent);


    width[0] = 250;
    width[1] = 200;
    width[2] = 200;

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
    
	prev_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_N_BTN), NULL);
	prev_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_O_BTN), NULL);
	prev_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_P_BTN), NULL);
	prev_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_D_BTN), NULL);

	next_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_N_BTN), NULL);
	next_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_O_BTN), NULL);
	next_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_P_BTN), NULL);
	next_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_D_BTN), NULL);

    
    // DAL
    memset(motdata, 0x00, sizeof(MotionData)*GUI_CHANNEL_CNT);
    memset(org_motdata, 0x00, sizeof(MotionData)*GUI_CHANNEL_CNT);

    for(i=0; i<GUI_CHANNEL_CNT; i++)
    {
        DAL_get_motionsensor_data(&motdata[i], i);
    }
    
    content_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
    nfui_nfobject_show(content_fixed);
    nfui_regi_pre_event_callback(content_fixed, mainbg_event_handler);
    nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);
    uxm_reg_imsg_event(content_fixed, INFY_CAMDB_CHANGE_NOTIFY);
    uxm_monitor_on_imsg_event(content_fixed, INFY_CAMDB_CHANGE_NOTIFY);
    uxm_reg_imsg_event(content_fixed, INFY_USRDB_CHANGE_NOTIFY);
    uxm_monitor_on_imsg_event(content_fixed, INFY_USRDB_CHANGE_NOTIFY);
    
    // table
    ntb = (NFOBJECT*)nfui_nftable_new (NUM_MOTSEN_COLUMNS, 1, MOTSEN_COL_SPACE, MOTSEN_ROW_SPACE, width, MOTSEN_LABEL_HEIGHT);                                   
    nfui_nfobject_show(ntb);
    nfui_nffixed_put((NFFIXED*)content_fixed, ntb, MOTSEN_TABLE_LEFT, MOTSEN_TABLE_TOP); 

    // table row 0
    for(i=0; i<NUM_MOTSEN_COLUMNS; i++)
    {
        if(i == 0) {
            title_object[i] = nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
            nfui_nfobject_modify_bg(title_object[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
            nfui_nfobject_use_focus(title_object[i], NFOBJECT_FOCUS_OFF);
            nfui_nfobject_show(title_object[i]);
        }
        else {
            title_object[i] = nfui_combobox_new(strOffOn, 2, 0);
            nfui_combobox_set_display_string(NF_COMBOBOX(title_object[i]), strTitle[i]);
            nfui_combobox_set_skin_type(NF_COMBOBOX(title_object[i]), NFCOMBOBOX_TYPE_2);
            nfui_combobox_set_align(NF_COMBOBOX(title_object[i]), NFALIGN_CENTER, 0);
            nfui_nfobject_set_size(title_object[i], width[i], MOTSEN_LABEL_HEIGHT);
            nfui_nfobject_show(title_object[i]);
            nfui_nfobject_set_data(title_object[i], "title index", GUINT_TO_POINTER(i));
            nfui_regi_post_event_callback(title_object[i], post_on_off_all_event_handler);
        }
        nfui_nftable_attach((NFTABLE*)ntb, title_object[i], i, 0);
    }

    size_w = 0;
    for (i = 0; i < NUM_MOTSEN_COLUMNS; i++) {
        size_w += width[i];
    }
    
    main_page_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(main_page_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(main_page_fixed, size_w, ROW_CNT_PER_PAGE * (40 + MOTSEN_ROW_SPACE) + 80);
    nfui_nfobject_show(main_page_fixed);
    nfui_nffixed_put((NFFIXED*)content_fixed, main_page_fixed, MOTSEN_TABLE_LEFT, MOTSEN_TABLE_TOP+MOTSEN_LABEL_HEIGHT+MOTSEN_ROW_SPACE);
    
    for (i = 0; i < PAGE_FIXED_CNT; i++)
    {
        g_page_fixed[i] = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_modify_bg(g_page_fixed[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
        nfui_nfobject_set_size(g_page_fixed[i], main_page_fixed->width, ROW_CNT_PER_PAGE * (40 + MOTSEN_ROW_SPACE));
        nfui_nffixed_put((NFFIXED*)main_page_fixed, g_page_fixed[i], 0, 0);

        page_ntb[i] = (NFOBJECT*)nfui_nftable_new (NUM_MOTSEN_COLUMNS, ROW_CNT_PER_PAGE, MOTSEN_COL_SPACE, MOTSEN_ROW_SPACE, width, MOTSEN_LABEL_HEIGHT);
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

    // table row 1 ~ ..

    page_num = row_num = 0;
    
    for(i=0; i<GUI_CHANNEL_CNT; i++)
    {
        // ch
        //DAL_get_camera_title(strBuf, i);
        var_get_camtitle(strBuf, i);

        cam_ch[i] = (NFOBJECT*)nfui_nfimglabel_new(pbCamImage[i], strBuf);
        nfui_nfimglabel_set_align((NFIMGLABEL*)cam_ch[i], NFALIGN_LEFT);
        nfui_nfimglabel_set_pango_font((NFIMGLABEL*)cam_ch[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
        nfui_nfobject_support_multi_lang(cam_ch[i], FALSE);
        nfui_nfobject_use_focus(cam_ch[i], NFOBJECT_FOCUS_OFF);
        nfui_nfobject_modify_bg(cam_ch[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        nfui_nfobject_show(cam_ch[i]);
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], cam_ch[i], 0, row_num);


        // activation
        act[i] = (NFOBJECT*)nfui_spinbutton_new((gchar**)strOffOn, 2, (gint)(motdata[i].act)); 
        nfui_spinbutton_set_skin_type((NFSPINBUTTON*)act[i], NFSPINBUTTON_TYPE_1);
        nfui_nfobject_set_data(act[i], "channel", GUINT_TO_POINTER(i));
        nfui_nfobject_show(act[i]);
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], act[i], 1, row_num);

        nfui_regi_post_event_callback(act[i], post_act_spinbutton_event_handler);

        // detecting mark
        detect[i] = (NFOBJECT*)nfui_spinbutton_new((gchar**)strOffOn, 2, (gint)(motdata[i].detect));
        nfui_spinbutton_set_skin_type((NFSPINBUTTON*)detect[i], NFSPINBUTTON_TYPE_1);
        nfui_nfobject_show(detect[i]);
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], detect[i], 2, row_num);

        if(!motdata[i].act)
            nfui_nfobject_disable(detect[i]);

        row_num++;

        if (row_num == ROW_CNT_PER_PAGE) {
            row_num = 0;
            page_num++;
        }
    }
        
    motsen_btns[0] = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(motsen_btns[0]), NFALIGN_CENTER, 0); 
    nfui_nfobject_show(motsen_btns[0]);
    nfui_nffixed_put((NFFIXED*)parent, motsen_btns[0], MENU_V_SUBTAB_BTN_R3_X, MENU_V_SUBTAB_BTN_Y);

    motsen_btns[1] = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(motsen_btns[1]), NFALIGN_CENTER, 0);
    nfui_nfobject_show(motsen_btns[1]);
    nfui_nffixed_put((NFFIXED*)parent, motsen_btns[1], MENU_V_SUBTAB_BTN_R2_X, MENU_V_SUBTAB_BTN_Y);

    motsen_btns[2] = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(motsen_btns[2]), NFALIGN_CENTER, 0); 
    nfui_nfobject_show(motsen_btns[2]);
    nfui_nffixed_put((NFFIXED*)parent, motsen_btns[2], MENU_V_SUBTAB_BTN_R1_X, MENU_V_SUBTAB_BTN_Y);

    nfui_regi_post_event_callback(motsen_btns[0], post_cancelbutton_event_handler);
    nfui_regi_post_event_callback(motsen_btns[1], post_applybutton_event_handler);
    nfui_regi_post_event_callback(motsen_btns[2], post_closebutton_event_handler);

    nfui_regi_post_event_callback(parent, post_page_event_handler);

    g_memmove(org_motdata, motdata, sizeof(MotionData)*GUI_CHANNEL_CNT);
}

gboolean check_motsen_act_data_changed()
{
    prvLoadDataFromObjects();

    if(!memcmp(org_motdata, motdata, sizeof(MotionData)*GUI_CHANNEL_CNT))
        return FALSE;

    return TRUE;
}

void save_motsen_act_data_changed()
{
    scm_put_log(CHANGE_CAM_MOTION, 0, 0);

    g_memmove(org_motdata, motdata, sizeof(MotionData)*GUI_CHANNEL_CNT);
    DAL_set_motion_data_all(motdata, GUI_CHANNEL_CNT);
    DAL_notify_fire_DB_change(NF_SYSDB_CATE_ALARM);
}

void restore_motsen_act_data_changed()
{
    g_memmove(motdata, org_motdata, sizeof(MotionData)*GUI_CHANNEL_CNT);

    prvSetDataToObjects(0);
}
