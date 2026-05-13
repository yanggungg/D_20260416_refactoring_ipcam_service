
#include "nf_afx.h"

#include "tools/nf_ui_tool.h"

#include "support/color.h"
#include "support/nf_ui_font.h"
#include "support/event_loop.h"
#include "support/util.h"
#include "support/nf_ui_image.h"

#include "objects/nffixed.h"
#include "objects/nflabel.h"
#include "objects/nfbutton.h"
#include "objects/nfcombobox.h"
#include "objects/cw_slider.h"
#include "objects/nfimglabel.h"
#include "objects/nftable.h"

#include "scm.h"
#include "vsm.h"

static FishEyeData g_fedata[GUI_CHANNEL_CNT];
static FishEyeData g_org_fedata[GUI_CHANNEL_CNT];

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *cam_ch[GUI_CHANNEL_CNT];
static NFOBJECT *dewarp[GUI_CHANNEL_CNT];
static NFOBJECT *mount[GUI_CHANNEL_CNT];


static guint _trans_idx_to_val(gint idx)
{
    gint val = 0;

    val |= (1 << idx);

    return val;
}

static gint _trans_val_to_idx(guint val)
{
/*ref. nf_api_ipcam.h
    NF_IPCAM_MOUNT_TYPES_E
    NF_IPCAM_DEWARP_MODES_E
*/
    gint i;

    for (i = 0; i < 64; i++)
    {
        if (val & (1 << i)) break;
    }

    if (i == 64) return 0;

    return i;
}

static void _get_data_from_obj()
{
    gint i;
    
    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        if (!scm_ipcam_is_fisheye(i)) continue;
        
		g_fedata[i].dewarp_mode = _trans_idx_to_val(nfui_combobox_get_cur_index((NFCOMBOBOX*)dewarp[i]));
		g_fedata[i].mount_type = _trans_idx_to_val(nfui_combobox_get_cur_index((NFCOMBOBOX*)mount[i]));
	}
}

static void _set_disable_obj(gint ch, gint expose)
{
    if (!scm_ipcam_is_fisheye(ch))
    {
        nfui_nfobject_disable(dewarp[ch]);
        nfui_nfobject_disable(mount[ch]);
    }

    if (expose)
    {
        nfui_signal_emit(dewarp[ch], GDK_EXPOSE, TRUE);
        nfui_signal_emit(mount[ch], GDK_EXPOSE, TRUE);
    }
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
            case 1: tmpObj = dewarp; break;
            case 2: tmpObj = mount; break;
            default: return FALSE;
        }

        idx = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        for(i=0; i<GUI_CHANNEL_CNT; i++) {
            if (!scm_ipcam_is_fisheye(i)) continue;

            nfui_combobox_set_index_no_expose((NFCOMBOBOX*)tmpObj[i], (guint)idx);
        }

        //mount

        for(i=0; i<GUI_CHANNEL_CNT; i++) {
            if (!scm_ipcam_is_fisheye(i)) continue;

            nfui_signal_emit(tmpObj[i], GDK_EXPOSE, TRUE);
        }
    }

    return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
	    gint i;
	
		if (evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

        _get_data_from_obj();
        
		if (!memcmp(&g_fedata, &g_org_fedata, sizeof(g_fedata))) return FALSE;
		
        memmove(&g_fedata, g_org_fedata, sizeof(g_fedata));

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            if (!scm_ipcam_is_fisheye(i)) continue;
            
            nfui_combobox_set_index_no_expose((NFCOMBOBOX*)dewarp[i], _trans_val_to_idx(g_fedata[i].dewarp_mode));
            nfui_combobox_set_index_no_expose((NFCOMBOBOX*)mount[i], _trans_val_to_idx(g_fedata[i].mount_type));
            nfui_signal_emit(dewarp[i], GDK_EXPOSE, TRUE);
            nfui_signal_emit(mount[i], GDK_EXPOSE, TRUE);
        }
 	}

	return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
    
	if (evt->type == GDK_BUTTON_RELEASE)
	{	
		if (evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

        _get_data_from_obj();

        if (memcmp(&g_fedata, &g_org_fedata, sizeof(g_fedata)))
        {
            DAL_set_fisheye_data(g_fedata, GUI_CHANNEL_CNT);
            
            memmove(&g_org_fedata, &g_fedata, sizeof(g_org_fedata));
    		syscam_set_changeflag(1);		
    		
    		nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
        }
	}

	return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
    	mb_type ret;
	
		if (evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;
			
        VW_SpecialCam_FishEye_tab_out_handler();
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
	switch(evt->type) {
		case GDK_EXPOSE:
		break;

		case GDK_DELETE:
			g_curwnd = 0;
		break;

		default:
		break;
	}

	return FALSE;
}

void VW_init_SpecialCam_FishEye_Page(NFOBJECT *parent)
{
    NFOBJECT *content_fixed;
    NFOBJECT *ntb;
    NFOBJECT *title_object[3];
    NFOBJECT *btns[3];
    
    GdkPixbuf *pbCamImage[32];

    const gchar *strTitle[] = { "CHANNEL",
                                "DEWARP MODE",
                                "MOUNT TYPE"};

    const gchar *strDewarp[] = {"FISHEYE", "SINGLE PANORAMA", "DOUBLE PANORAMA", "4PTZ", "FISHEYE + 3_PTZ", "FISHEYE + 5_PTZ", "FISHEYE + 7_PTZ"};
    const gchar *strMount[] = {"WALL", "CEILING", "TABLE"};
    gchar strBuf[STRING_SIZE_CAMTITLE];

    guint width[3];
    guint i;

    g_curwnd = nfui_nfobject_get_top(parent);


    width[0] = 250;
    width[1] = 300;
    width[2] = 300;

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
    memset(&g_fedata, 0x00, sizeof(g_fedata));
    memset(&g_org_fedata, 0x00, sizeof(g_org_fedata));

    for (i = 0; i < GUI_CHANNEL_CNT; i++) {
        DAL_get_fisheye_data(&g_fedata[i], i);
    }
    memmove(&g_org_fedata, &g_fedata, sizeof(g_org_fedata));
    
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
    ntb = nfui_nftable_new(3, GUI_CHANNEL_CNT+1, 2, 1, width, 40);
    nfui_nfobject_show(ntb);
    nfui_nffixed_put((NFFIXED*)content_fixed, ntb, 27, 40);

    // table row 0
    for(i=0; i<3; i++)
    {
        if(i == 0) {
            title_object[i] = nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
            nfui_nfobject_modify_bg(title_object[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
            nfui_nfobject_use_focus(title_object[i], NFOBJECT_FOCUS_OFF);
            nfui_nfobject_show(title_object[i]);
        }
        else if (i == 1)
        {
            title_object[i] = nfui_combobox_new(strDewarp, 7, 0);
            nfui_combobox_set_display_string(NF_COMBOBOX(title_object[i]), strTitle[i]);
            nfui_combobox_set_skin_type(NF_COMBOBOX(title_object[i]), NFCOMBOBOX_TYPE_2);
            nfui_combobox_set_align(NF_COMBOBOX(title_object[i]), NFALIGN_CENTER, 0);
            nfui_nfobject_set_size(title_object[i], width[i], 40);
            nfui_nfobject_show(title_object[i]);
            nfui_nfobject_set_data(title_object[i], "title index", GUINT_TO_POINTER(i));
            nfui_regi_post_event_callback(title_object[i], post_on_off_all_event_handler);
        }
        else
        {
            title_object[i] = nfui_combobox_new(strMount, 3, 0);
            nfui_combobox_set_display_string(NF_COMBOBOX(title_object[i]), strTitle[i]);
            nfui_combobox_set_skin_type(NF_COMBOBOX(title_object[i]), NFCOMBOBOX_TYPE_2);
            nfui_combobox_set_align(NF_COMBOBOX(title_object[i]), NFALIGN_CENTER, 0);
            nfui_nfobject_set_size(title_object[i], width[i], 40);
            nfui_nfobject_show(title_object[i]);
            nfui_nfobject_set_data(title_object[i], "title index", GUINT_TO_POINTER(i));
            nfui_regi_post_event_callback(title_object[i], post_on_off_all_event_handler);
        }
        nfui_nftable_attach((NFTABLE*)ntb, title_object[i], i, 0);
    }

    // table row 1 ~ ..
    for(i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        var_get_camtitle(strBuf, i);

        cam_ch[i] = (NFOBJECT*)nfui_nfimglabel_new(pbCamImage[i], strBuf);
        nfui_nfimglabel_set_align((NFIMGLABEL*)cam_ch[i], NFALIGN_LEFT);
        nfui_nfimglabel_set_pango_font((NFIMGLABEL*)cam_ch[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
        nfui_nfobject_support_multi_lang(cam_ch[i], FALSE);
        nfui_nfobject_use_focus(cam_ch[i], NFOBJECT_FOCUS_OFF);
        nfui_nfobject_modify_bg(cam_ch[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        nfui_nfobject_show(cam_ch[i]);
        nfui_nftable_attach((NFTABLE*)ntb, cam_ch[i], 0, i+1);

        // activation
        dewarp[i] = (NFOBJECT*)nfui_combobox_new((gchar**)strDewarp, 7, _trans_val_to_idx(g_fedata[i].dewarp_mode)); 
        nfui_combobox_set_skin_type((NFCOMBOBOX*)dewarp[i], NFCOMBOBOX_TYPE_SUBTAB_1);
        nfui_combobox_set_align((NFCOMBOBOX*)dewarp[i], NFALIGN_CENTER, 0);
        nfui_nfobject_set_data(dewarp[i], "channel", GUINT_TO_POINTER(i));
        nfui_nfobject_show(dewarp[i]);
        nfui_nftable_attach((NFTABLE*)ntb, dewarp[i], 1, i+1);

        // detecting mark
        mount[i] = (NFOBJECT*)nfui_combobox_new((gchar**)strMount, 3, _trans_val_to_idx(g_fedata[i].mount_type));
        nfui_combobox_set_align((NFCOMBOBOX*)mount[i], NFALIGN_CENTER, 0);
        nfui_combobox_set_skin_type((NFCOMBOBOX*)mount[i], NFCOMBOBOX_TYPE_SUBTAB_1);
        nfui_nfobject_show(mount[i]);
        nfui_nftable_attach((NFTABLE*)ntb, mount[i], 2, i+1);

        _set_disable_obj(i, 0);
    }
        
    btns[0] = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(btns[0]), NFALIGN_CENTER, 0); 
    nfui_nfobject_show(btns[0]);
    nfui_nffixed_put((NFFIXED*)parent, btns[0], MENU_V_SUBTAB_BTN_R3_X, MENU_V_SUBTAB_BTN_Y);

    btns[1] = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(btns[1]), NFALIGN_CENTER, 0);
    nfui_nfobject_show(btns[1]);
    nfui_nffixed_put((NFFIXED*)parent, btns[1], MENU_V_SUBTAB_BTN_R2_X, MENU_V_SUBTAB_BTN_Y);

    btns[2] = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(btns[2]), NFALIGN_CENTER, 0); 
    nfui_nfobject_show(btns[2]);
    nfui_nffixed_put((NFFIXED*)parent, btns[2], MENU_V_SUBTAB_BTN_R1_X, MENU_V_SUBTAB_BTN_Y);

    nfui_regi_post_event_callback(btns[0], post_cancelbutton_event_handler);
    nfui_regi_post_event_callback(btns[1], post_applybutton_event_handler);
    nfui_regi_post_event_callback(btns[2], post_closebutton_event_handler);

    nfui_regi_post_event_callback(parent, post_page_event_handler);
}

void VW_SpecialCam_FishEye_tab_out_handler()
{
    mb_type ret;
    gint i;

    _get_data_from_obj();
	
    if (!memcmp(&g_fedata, &g_org_fedata, sizeof(g_fedata))) return;

    ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

    if (ret == NFTOOL_MB_OK)
    {
        DAL_set_fisheye_data(g_fedata, GUI_CHANNEL_CNT);
        
        memmove(&g_org_fedata, &g_fedata, sizeof(g_org_fedata));
		syscam_set_changeflag(1);		
    }
    else
    {
        memmove(&g_fedata, g_org_fedata, sizeof(g_fedata));

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            if (!scm_ipcam_is_fisheye(i)) continue;
            
            nfui_combobox_set_index_no_expose((NFCOMBOBOX*)dewarp[i], _trans_val_to_idx(g_fedata[i].dewarp_mode));
            nfui_combobox_set_index_no_expose((NFCOMBOBOX*)mount[i], _trans_val_to_idx(g_fedata[i].mount_type));
        }
    }
}

