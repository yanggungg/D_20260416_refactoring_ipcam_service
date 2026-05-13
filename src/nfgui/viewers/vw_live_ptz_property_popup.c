#include "nf_afx.h"

#include "support/nf_ui_color.h"
#include "support/nf_ui_image.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"
#include "support/color.h"

#include "services/scm.h"

#include "nf_api_live.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"
#include "objects/nfspinbutton.h"

#include "vw_sys_camera_main.h"
#include "ix_mem.h"

#define PP_LABEL_HEI                    (40)

#define PP_SIZE_WID			            (520)
#define PP_SIZE_HEI			            (460)

#define PP_POS_X			            ((1920-PP_SIZE_WID)/2)
#define PP_POS_Y			            ((1080-PP_SIZE_HEI)/2)


typedef enum _PP_ROW_E {
	PP_DRIVER		= 0,
	PP_AUTO_FOCUS,
	PP_AUTO_IRIS,	
	PP_PT_SPEED,	
	PP_ZOOM_SPEED,	
	PP_FOCUS_SPEED,	
	PP_IRIS_SPEED,	
	PP_ROW_MAX
} PP_ROW_E;


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *value[PP_ROW_MAX];

static PtzData g_ptzdata;
static PtzData *g_org_ptzdata;

static gint retVal = 0;

static gboolean post_main_wnd_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
		gtk_main_quit();
	}

	return FALSE;
}

static gboolean post_ok_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;
	
	if(evt->type == GDK_BUTTON_RELEASE) 
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
		  return FALSE;

       if (memcmp(g_org_ptzdata, &g_ptzdata, sizeof(PtzData)))
        {
            g_memmove(g_org_ptzdata, &g_ptzdata, sizeof(PtzData));
            retVal = 1;
        }
        else
            retVal = 0;
	
		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean post_cancel_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;
	
	if(evt->type == GDK_BUTTON_RELEASE) 
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
		  return FALSE;

        g_memmove(&g_ptzdata, g_org_ptzdata, sizeof(PtzData));
        retVal = 0;

		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean post_spinbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case NFEVENT_SPINBUTTON_CHANGED:
		{
			guint ch;
			gint i;

            for (i = 0; i < PP_ROW_MAX; i++)
            {
                if (value[i] == obj)
                    break;
            }

			if (i == PP_AUTO_FOCUS)
				g_ptzdata.autoFocus = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)obj);
			else if (i == PP_AUTO_IRIS)
				g_ptzdata.autoIris = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)obj);
			else if (i == PP_PT_SPEED)
				g_ptzdata.PTSpeed = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)obj);
			else if (i == PP_ZOOM_SPEED)
				g_ptzdata.zoomSpeed = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)obj);
			else if (i == PP_FOCUS_SPEED)
				g_ptzdata.focusSpeed = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)obj);
			else if (i == PP_IRIS_SPEED)
				g_ptzdata.irisSpeed = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)obj);
		}
		break;

		default:
		break;
	}

	return FALSE;
}

gint VW_PtzCtrl_Property_Open(NFWINDOW *parent, PtzData *data)
{
	NFOBJECT *main_wnd, *main_fixed;
	NFOBJECT *ntb;
	NFOBJECT *obj;

	const gchar *strTitle[] = {
				"PTZ DRIVER",
				"AUTO FOCUS",
				"AUTO IRIS",
				"P/T SPEED",
				"ZOOM SPEED",
				"FOCUS SPEED",
				"IRIS SPEED"
	};

	const gchar *str_off_on[2]={"OFF", "ON"};
	gchar **strProto = NULL;
	guint ntb_width[2];
	gint ptzProto_cnt = 0;
	guint i;

    g_memmove(&g_ptzdata, data, sizeof(PtzData));
	g_org_ptzdata = data;

	main_wnd = (NFOBJECT*)nftool_create_popup_window(parent, PP_POS_X, PP_POS_Y, PP_SIZE_WID, PP_SIZE_HEI, "PTZ PROPERTY", TRUE);
	nfui_regi_post_event_callback(main_wnd, post_main_wnd_event_handler);
	nfui_nfobject_show(main_wnd);
	g_curwnd = (NFWINDOW*)main_wnd;
	
	main_fixed = ((NFWINDOW*)main_wnd)->child;

	ntb_width[0] = 260;
	ntb_width[1] = 200;

	ntb = (NFOBJECT*)nfui_nftable_new(2, PP_ROW_MAX, (guint)(DISPLAY_IS_D1 ? 2:6), (guint)(DISPLAY_IS_D1 ? 1:4), ntb_width, PP_LABEL_HEI);
	nfui_nffixed_put((NFFIXED*)main_fixed, ntb, 22, 64);
	nfui_nfobject_show(ntb);

	for (i = 0; i < PP_ROW_MAX; i++)
	{
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)ntb, obj, 0, i);

        if (i == PP_DRIVER)
        {
            gint j;
        
        	ptzProto_cnt = scm_get_protocol_cnt();

        	if (ptzProto_cnt > 0) {	
        		strProto = imalloc(sizeof(gchar*) * ptzProto_cnt);
        		
        		for (j = 0; j < ptzProto_cnt; j++) 
        			strProto[j] = imalloc(sizeof(gchar) * MAX_PTZ_PROTO_STR_LENGTH);

        		for (j = 0; j < ptzProto_cnt; j++) 
        		{
        			if (DAL_get_ptz_protocol_name(strProto[j], j) == 0) 
        			{
        				ifree(strProto);
        				break;
        			}
        		}
        	}

        	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
        	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
        	nfui_nflabel_set_text((NFLABEL*)obj, strProto[g_ptzdata.proto]);
        	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
        	nfui_nflabel_set_spacing((NFLABEL*)obj, CONDENSED_SPACING);
        	nfui_nfobject_support_multi_lang((NFOBJECT*)obj, FALSE);
        	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		    nfui_nfobject_show(obj);
		    nfui_nftable_attach((NFTABLE*)ntb, obj, 1, i);

        	if (strProto) ifree(strProto); 
        }
        else
        {
            if (i == PP_AUTO_FOCUS)         obj = nfui_spinbutton_new((gchar**)str_off_on, 2, (gint)g_ptzdata.autoFocus);
            else if (i == PP_AUTO_IRIS)     obj = nfui_spinbutton_new((gchar**)str_off_on, 2, (gint)g_ptzdata.autoIris);
            else if (i == PP_PT_SPEED)      obj = nfui_spinbutton_new_index_with_range(g_ptzdata.PTSpeed, 1, 10, 1);
            else if (i == PP_ZOOM_SPEED)    obj = nfui_spinbutton_new_index_with_range(g_ptzdata.zoomSpeed, 1, 10, 1);
            else if (i == PP_FOCUS_SPEED)   obj = nfui_spinbutton_new_index_with_range(g_ptzdata.focusSpeed, 1, 10, 1);
            else if (i == PP_IRIS_SPEED)    obj = nfui_spinbutton_new_index_with_range(g_ptzdata.irisSpeed, 1, 10, 1);

    		nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_POPUP_1);
    		nfui_spin_button_set_align((NFSPINBUTTON*)obj, NFALIGN_CENTER, 0);    		
		    nfui_nfobject_show(obj);
		    nfui_nftable_attach((NFTABLE*)ntb, obj, 1, i);
    		nfui_regi_post_event_callback(obj, post_spinbutton_event_handler);
            value[i] = obj;	
        }
	}
	
	obj = nftool_normal_button_create_type1("OK", 192);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, PP_SIZE_WID/2-192-5, PP_SIZE_HEI-68);
	nfui_regi_post_event_callback(obj, post_ok_button_event_handler);
	
	obj = nftool_normal_button_create_type1("CANCEL", 192);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, PP_SIZE_WID/2+5, PP_SIZE_HEI-68);
	nfui_regi_post_event_callback(obj, post_cancel_button_event_handler);

	nfui_make_key_hierarchy((NFWINDOW*)main_wnd);
	nfui_set_key_focus(obj, TRUE);
	nfui_page_close(PGID_POPUPWND, main_wnd);
	nfui_page_open(PGID_PTZ_PROP, main_wnd, nfui_get_last_user());

	gtk_main();

	nfui_page_close(PGID_PTZ_PROP, main_wnd);

    return retVal;    
}

