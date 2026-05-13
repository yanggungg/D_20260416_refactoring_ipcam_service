#include "nf_afx.h"
#include "nf_sysman.h"

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
#include "objects/nfcombobox.h"
#include "objects/nfspinbutton.h"

#include "vw_default_value_settings.h"

#include "scm.h"
#include "uxm.h"
#include "ix_mem.h"


#define ITX_DEFAULT_VALUE_SETTINGS_MODE_ON      "itx_default_setting_mode_on.txt"


#define MAX_MARGIM_SIZE		    	(guint)12

#define PI_WND_SIZE_WID		    	(guint)(610 + 200)
#define PI_WND_SIZE_HEI		    	(guint)(520 + 200)

#define SE_PP_POS_X			    	(guint)((DISPLAY_ACTIVE_WIDTH - PI_WND_SIZE_WID)/2)
#define SE_PP_POS_Y			    	(guint)((DISPLAY_ACTIVE_HEIGHT - PI_WND_SIZE_HEI)/2)

#define PI_FIXED_POS_X		        (guint)(12)
#define PI_FIXED_POS_Y		        (guint)(56)
#define PI_FIXED_SIZE_WID	        (guint)(PI_WND_SIZE_WID - PI_FIXED_POS_X * 2)
#define PI_FIXED_SIZE_HEI	        (guint)(PI_WND_SIZE_HEI - PI_FIXED_POS_Y - MAX_MARGIM_SIZE)

#define MENU_BTN_WIDTH			    (160)
#define MENU_BTN_HEIGHT				(44)
#define MENU_BTN_GAP				(4)

#define MENU_V_BTN_R_START_X		(PI_FIXED_SIZE_WID - MENU_BTN_WIDTH)
#define MENU_V_BTN_R1_X				(MENU_V_BTN_R_START_X - MENU_BTN_GAP)
#define MENU_V_BTN_R2_X				(MENU_V_BTN_R1_X - MENU_BTN_GAP - MENU_BTN_WIDTH)
#define MENU_V_BTN_Y				(PI_FIXED_SIZE_HEI - 10 - MENU_BTN_HEIGHT)

#define	IPS_LABEL_HEIGHT			(40)
#define	IPS_LABEL_ROW_SPACE			(2)
#define CATEGORY_LABEL_LEFT         (4)
#define CATEGORY_CONTENT_GAP        (60)
#define	SUBJECT_LABEL_LEFT			(28)
#define	SUBJECT_LABEL_TOP			(42)
#define	SUBJECT_LABEL_WIDTH			(310)
#define	SUBJECT_LABEL_MARGIN		(0)
#define DT_SETUP_CELL_W				(400)

#define LANGUAGE_STR_SIZE		    (32)
#define LABEL_COL_CNT               (3)

typedef enum {
	PIB_LANGAPPLY,
	PIB_OK,
	PIB_BUTTONS
}BUTTON_E;

typedef enum {
	SIGNAL_50 = 0,
	SIGNAL_60,
	NUM_SIGNAL,
}SIGNAL_E;

static FacInitData org_init_conf;
static FacInitData init_conf;

static gint 		g_media_cnt;
static MEDIA_INFO_T	*g_media_info;

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_curfixed;
static NFOBJECT *wait_pop = NULL;
static NFOBJECT *g_btnLang[20] = {0,};
static NFOBJECT *g_btnApply;
static NFOBJECT *timeZone_obj;
static NFOBJECT *g_objSig;

static gchar **g_strLang;
static gint g_langCnt;
static guint g_tid = 0;



static gboolean _init_lang()
{
	gint i, j = 0;
	gchar strLangAlias[64];
	gint cnt;


    g_langCnt = 0;
    
	cnt = DAL_get_support_lang_cnt();
	if (cnt < 0) 
	{
    	return FALSE;
	}

	for (i = 0; i < cnt; i++)
	{
		memset(strLangAlias, 0x00, sizeof(strLangAlias));
		DAL_get_support_lang_alias(i, strLangAlias);
		if (strlen(strLangAlias)) g_langCnt++;
	}

	g_strLang = (gchar**)imalloc(sizeof(gchar*)*g_langCnt);
	
	for (i = 0; i < cnt; i++) 
	{
		memset(strLangAlias, 0x00, sizeof(strLangAlias));
		DAL_get_support_lang_alias(i, strLangAlias);
		
		if (strlen(strLangAlias)) {
			g_strLang[j] = (gchar*)imalloc(LANGUAGE_STR_SIZE);
			g_assert(g_strLang[j]);

			DAL_get_support_lang(i, g_strLang[j]);
			j++;
		}
	}

	g_message("%s, %d, g_langCnt:%d, j:%d", __FUNCTION__, __LINE__, g_langCnt, j);

	return TRUE;
}

static gboolean _change_language_by_string(gchar *str)
{
	deinit_multi_language_support();
	
	if(0 != init_multi_language_support(str)){
		return 1;
	}

	return 0;
}

static gint _prvLoadDataFromObjects()
{
    init_conf.timeZone = nfui_combobox_get_cur_index((NFCOMBOBOX*)timeZone_obj);
    init_conf.sigType = nfui_spin_button_get_index((NFSPINBUTTON*)g_objSig);
    
	return 0;
}

static int _check_file(void)
{
	gchar dir[128];
	gchar path[256];
	gchar *file = NULL;
	gchar *dev = NULL;
	guint i, cnt = 0;

	if (g_media_info)
	{
		scm_free_media_list(g_media_info);
	}

	g_media_cnt = 0;	
	g_media_info = scm_new_media_list(&g_media_cnt);


	for (i = 0; i < g_media_cnt; i++)
	{
		if (scm_get_media_type(g_media_info[i].id) == MTYPE_USB)
		{
    		memset(dir, 0x00, sizeof(dir));
  		
    		if(scm_get_mounted_path(g_media_info[i].id, dir, 128) < 0) {
    			g_warning("%s :::: RETURN -1" , __FUNCTION__);
    			return FALSE;
    		}

            memset(path, 0x00, sizeof(path));
    		g_sprintf(path, "%s/%s", dir, ITX_DEFAULT_VALUE_SETTINGS_MODE_ON);
    		g_message("%s :::: full path : %s", __FUNCTION__, path);
    			
		    if(ifn_is_file_exist(path))
		    {
		        return 1;
		    }
		}
	}

    return 0;
}

static void print_db(FILE *f, gchar *key, gchar *type, gchar *min, gchar *max, gchar *val)
{
	gchar *markup = NULL;
	markup = g_markup_printf_escaped("<item key=\"%s\" type=\"%s\" min=\"%s\" max=\"%s\" val=\"%s\" />\n", key, type, min, max, val);
	fputs( markup, f);	
	g_free(markup);
}

static void make_private_db(void)
{
	FILE *f = NULL;	
	gchar buf[20];
	
	f = fopen("/NFDVR/data/nf_sysdb_private.conf","w");
	fprintf( f, "<nf_sysdb>\n");

	fprintf( f, "<disp>\n");
	print_db(f, "disp.osd.lang", "STRING", "0", "32", init_conf.lang);
	fprintf( f, "</disp>\n");

	fprintf( f, "<sys>\n");
	sprintf( buf, "%u", init_conf.timeZone);
	print_db(f, "sys.date.tz_index", "UINT", "0", "", buf);
	sprintf( buf, "%u", init_conf.sigType);	
	print_db(f, "sys.info.sig_type", "BOOL", "0", "1", buf);	
	fprintf( f, "</sys>\n");
	
	fprintf( f, "</nf_sysdb>\n");
	fclose(f);	
}

static gboolean post_onoff_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_RADIO_GET_FOCUS) 
	{
	    gint i;

        
	    for ( i = 0; i < g_langCnt; i++ )
	    {
	        if ( g_btnLang[i] == obj )
	        {
	            g_message("%s %d, i : %d find matching language.", __FUNCTION__, __LINE__, i);
	            break;
	        }
	    }

	    if ( i == g_langCnt )
	    {
	        g_message("%s %d, No matching language.", __FUNCTION__, __LINE__);
	        return FALSE;
	    }
        strcpy(init_conf.lang, g_strLang[i]);
	}

	return FALSE;
}

static gboolean post_apply_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top;
		mb_type ret;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

        _prvLoadDataFromObjects();
        
		make_private_db();

		DAL_set_fac_init_run(FALSE);
		DAL_set_fac_init_data(&init_conf);		
		DAL_save_setup_db(NFSETUP_WINDOW_DISPLAY);
		DAL_save_setup_db(NFSETUP_WINDOW_SYSTEM);

		if (strcmp(org_init_conf.lang, init_conf.lang) != 0)
		{
		    _change_language_by_string(init_conf.lang);
		}

		if(org_init_conf.sigType != init_conf.sigType)
		{
			top = nfui_nfobject_get_top(obj);			
			nftool_mbox(top, "NOTICE", "Video type has been changed.\nThe system will be reboot.", NFTOOL_MB_OK);			
			nf_api_param_app_set_cate(NF_SYSMAN_APP_PARAM_CATE_IS_PAL, (gint)init_conf.sigType);
			scm_reboot_system(RR_SIGNAL_CHANGE, 0);		
		}
		else
		{
			scm_apply_timezone(init_conf.timeZone, NULL);
			top = nfui_nfobject_get_top(obj);		
			nfui_nfobject_destroy(top);
		}
	}

	return FALSE;
}

static gboolean post_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
		if(evt->type == GDK_DELETE)
		{
    		gint i;

    		if (g_strLang) 
    		{
    			for (i = 0; i < g_langCnt; i++)  
    			{
    				if (g_strLang[i]) {
    					ifree(g_strLang[i]);
    					g_strLang[i] = NULL;
    				}
    			}

    			ifree(g_strLang);
    			g_strLang = 0;
    		}

            if (g_tid)
            {
                g_source_remove(g_tid);
                g_tid = 0;
            }

            if (g_media_info)
            {
                ifree(g_media_info);
                g_media_info = NULL;
                g_media_cnt = 0;
            }

			g_curwnd = 0;
			gtk_main_quit();
			g_message("###lang : %s, %d\n", __FUNCTION__, __LINE__);
		}
	
		return FALSE;

}

gint VW_Default_Value_Settings()
{
	NFOBJECT *main_wnd, *main_fixed;
	NFOBJECT *fixed1;
	NFOBJECT *obj;	
	NFOBJECT *btns;
	GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];
	GSList *slist = NULL;
	
	gchar *strTimezone[50];
	gint pos_x,pos_y,size_w,size_h;
	gint zone_count;
    gint i, cnt = 1;

    const gchar *strCategory[] = {"SELECT SYSTEM LANGUAGE", "SELECT TIMEZONE", "SELECT AC POWER FREQUENCY"};
	const gchar strButton[16] = "APPLY";
    const gchar *strSigType[] = {"60Hz", "50Hz"};
    

   
//<------DB LOAD
    memset(&org_init_conf, 0x00, sizeof(FacInitData));
    memset(&init_conf, 0x00, sizeof(FacInitData));

	DAL_get_fac_init_data(&init_conf);	
	g_memmove(&org_init_conf, &init_conf, sizeof(FacInitData));

//<------MAKE WINDOW & FIXED
	main_wnd = nftool_create_popup_window(NF_TOPWND, SE_PP_POS_X, SE_PP_POS_Y, PI_WND_SIZE_WID, PI_WND_SIZE_HEI, "DEFAULT VALUE SETTINGS", FALSE);
	nfui_regi_post_event_callback(main_wnd, post_win_event_handler);
	g_curwnd = main_wnd;

	main_fixed = ((NFWINDOW*)main_wnd)->child;
	g_curfixed = main_fixed;

	fixed1 = nfui_nffixed_new();
	nfui_nfobject_set_size(fixed1, PI_FIXED_SIZE_WID, PI_FIXED_SIZE_HEI);
	nfui_nffixed_put((NFFIXED*)main_fixed, fixed1, PI_FIXED_POS_X, PI_FIXED_POS_Y);
	nfui_nfobject_show(fixed1);

//<-------LANGUAGE SETTING
	if (!_init_lang()) 
	{
		g_warning("%s [%d] : init support language error", __FUNCTION__, __LINE__);
		return;
	}

	pos_x = (guint)4;
	pos_y = (guint)4;

	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG3);
	nfui_nfimage_set_text((NFIMAGE*)obj, strCategory[0]);
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, CATEGORY_LABEL_LEFT, pos_y);

	/* radio button */
	radio_img[0] = nfui_get_image_from_file((IMG_N_POPUP_RADIO_OFF), NULL);
	radio_img[1] = nfui_get_image_from_file((IMG_O_POPUP_RADIO_ON), NULL);
	radio_img[2] = nfui_get_image_from_file((IMG_P_POPUP_RADIO_ON), NULL);
	radio_img[3] = nfui_get_image_from_file((IMG_D_POPUP_RADIO_OFF), NULL);

	nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

	pos_x = (guint)SUBJECT_LABEL_LEFT;
	pos_y += (guint)60;

	for (i = 0; i < g_langCnt; i++) 
	{
		obj = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
		nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
		nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
		nfui_regi_post_event_callback(obj, post_onoff_event_cb);
		nfui_nfobject_show(obj);
		g_btnLang[i] = obj;

		if (i == 0) 
		{
			slist = nfui_radio_button_get_group(NF_BUTTON(obj));
		} 
		else 
		{
			nfui_radio_button_add_group(NF_BUTTON(obj), slist);
		}
		nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);

		if ( strcmp(init_conf.lang, g_strLang[i]) == 0 )
		{
		    nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);
		}

		/* label */
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(g_strLang[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
		nfui_nfobject_set_size(obj, PI_FIXED_SIZE_WID/LABEL_COL_CNT - size_w - 20, 27);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x + size_w + 10, pos_y);

        if ((cnt % LABEL_COL_CNT) == 0 || cnt == g_langCnt)
        {
            pos_y += (guint)size_h + 15;
            pos_x = SUBJECT_LABEL_LEFT;
        }
    	else
    	{
        	pos_x += PI_FIXED_SIZE_WID/LABEL_COL_CNT - 20;
    	}
    	cnt++;
	}

//<-------- TIMEZONE

    pos_y += 15;
    pos_x = CATEGORY_LABEL_LEFT;
    
	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG3);
	nfui_nfimage_set_text((NFIMAGE*)obj, strCategory[1]);
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, CATEGORY_LABEL_LEFT, pos_y);
	
    pos_x = SUBJECT_LABEL_LEFT;
    pos_y += CATEGORY_CONTENT_GAP;
    
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("TIMEZONE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
	nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);

	zone_count = nf_zoneinfo_get_count();
	
	for(i=0; i<zone_count; i++)
		strTimezone[i] = nf_zoneinfo_get_string((gint)i);

	obj = nfui_combobox_new(strTimezone, zone_count, init_conf.timeZone);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, DT_SETUP_CELL_W, IPS_LABEL_HEIGHT);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x + SUBJECT_LABEL_WIDTH + IPS_LABEL_ROW_SPACE, pos_y);
	timeZone_obj = obj;

//<-------- AC POWER FREQUENCY

    pos_y += 70;
    pos_x = CATEGORY_LABEL_LEFT;
    
	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG3);
	nfui_nfimage_set_text((NFIMAGE*)obj, strCategory[2]);
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
	nfui_nfobject_get_size(obj, &size_w, &size_h);

    pos_y += CATEGORY_CONTENT_GAP;
    pos_x = SUBJECT_LABEL_LEFT;
    
	obj = nfui_nflabel_new_with_pango_font("AC POWER FREQUENCY", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, SUBJECT_LABEL_WIDTH, IPS_LABEL_HEIGHT);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
	
	obj = nfui_spinbutton_new(strSigType, NUM_SIGNAL, init_conf.sigType);
	nfui_nfobject_set_size(obj, DT_SETUP_CELL_W, IPS_LABEL_HEIGHT);
	nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_1);
	nfui_spin_button_set_align((NFSPINBUTTON*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x + SUBJECT_LABEL_WIDTH + IPS_LABEL_ROW_SPACE, pos_y);
	g_objSig = obj;

//<-------- Make Button
    
	btns = nftool_normal_button_create_type1(strButton, MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(btns), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(btns);
	nfui_nffixed_put((NFFIXED*)fixed1, btns, MENU_V_BTN_R1_X, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(btns, post_apply_button_event_handler);
	g_btnApply = btns;

	nfui_nfobject_show(main_wnd);

	/* set for key navi */
	nfui_make_key_hierarchy(main_wnd);
	nfui_set_key_focus(btns, TRUE);

	NFUTIL_THREADS_ENTER();
	gtk_main();
	NFUTIL_THREADS_LEAVE();

	return 0;

}

gint vw_run_default_value_settings()
{
    if (nf_sysman_check_default_setting())
    {
        return 1;
    }
    
    return 0;
}

