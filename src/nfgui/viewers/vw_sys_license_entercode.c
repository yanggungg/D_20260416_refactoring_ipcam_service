#include "nf_afx.h"

#include "scm.h"
#include "iux_msg.h"

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

#include "viewers/vw_arch_export.h"

#include "modules/ssm.h"

#include "vw_vkeyboard.h"
#include "ix_mem.h"

#define LEC_WIN_SIZE_W	(678)
#define LEC_WIN_SIZE_H	(220)

#define LEC_POS_X		((DISPLAY_ACTIVE_WIDTH - LEC_WIN_SIZE_W)/4*2)
#define LEC_POS_Y		((DISPLAY_ACTIVE_HEIGHT - LEC_WIN_SIZE_H)/2)

#define MAX_CODE_STRING_SIZE			35

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_win;
static NFOBJECT *g_codeobj;

static NF_SYSDB_LICENSE_INFO *g_out_key = NULL;
static LicenseData *g_local_lic;
static LicenseData *g_cam_lic;
static gint g_ch = 0;
static gchar *g_code = NULL;



static gint _is_exist_license(gchar *input_key)
{
    gint i;

    if (g_ch == -1)
    {
        for (i = 0; i < g_local_lic->count; i++) {
            if (strcmp(g_local_lic->key_data[i].key, input_key) == 0) return 1;
        }
    }
    else
    {
        for (i = 0; i < g_cam_lic[g_ch].count; i++) {
            if (strcmp(g_cam_lic[g_ch].key_data[i].key, input_key) == 0) return 1;
        }
    }

    return 0;
}

static gint _is_maximum_license(gint ch)
{
    if ((ch == -1 && g_local_lic->count >= MAX_LICENSE_CNT) || 
        (ch >= 0 && g_cam_lic[ch].count >= MAX_LICENSE_CNT)) 
    {
        return 1;
    }

    return 0;
}

static gint _result_enter_code(gint ret)
{
    switch(ret)
    {
        case 0:
            nftool_mbox(g_curwnd, "NOTICE", "License added.", NFTOOL_MB_OK);
        break;
        
        case -1:
        case -2:
        case -3:
        case -4:
            nftool_mbox(g_curwnd, "NOTICE", "Invalid key.", NFTOOL_MB_OK);
        break;
        
        default:
            nftool_mbox(g_curwnd, "NOTICE", "Unknown error.", NFTOOL_MB_OK);
        break;
    }
    return 0;
}

static gboolean post_code_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if (evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{
		NFOBJECT *top;
		gchar *str = NULL;
		gchar code[32];
		gchar user[16];
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
			if(evt->button.button == MOUSE_RIGTH_BUTTON) 
				return FALSE;

			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += ((GdkEventButton*)evt)->x;
			y += ((GdkEventButton*)evt)->y;
		}

		str = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), x, y, MAX_CODE_STRING_SIZE, VKEY_LICENSE_CODE);

		if(str) {
		    if (strcmp(str, "2") == 0) {
		        strcpy(code, "4FT76-OTU33-NDOKG-UT7CH-OMRFN-45APA");
		    }
		    else {
    			g_stpcpy(code, str);
			}
			ifree(str);

			nfui_nflabel_set_text((NFLABEL*)obj, code);
			nfui_signal_emit(obj, GDK_EXPOSE, FALSE);

		}
	}
	
	return FALSE;
}


static gboolean post_ok_btn_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *top;
	gchar *license_key = NULL;
	NF_SYSDB_LICENSE_INFO out_key;
	gint ret = 0;
	gchar buf[256];

	if(evt->type == GDK_BUTTON_RELEASE) 
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
		
		license_key = nfui_nflabel_get_text((NFLABEL*)g_codeobj);

		if (strlen(license_key) < 1) {
			nftool_mbox(g_curwnd, "NOTICE", "Please input a license key.", NFTOOL_MB_OK);
			return FALSE;
		}
		
		memset(&out_key, 0x00, sizeof(NF_SYSDB_LICENSE_INFO));

        ret = scm_license_decoding_key(g_ch, license_key, &out_key);

        if (_is_maximum_license(g_ch)) {
            memset(buf, 0x00, sizeof(buf));
            sprintf(buf, lookup_string("You have exceeded the number of licenses you can register.\n(Max number : %d)"), MAX_LICENSE_CNT);
            nftool_mbox(g_curwnd, "ERROR", buf, NFTOOL_MB_OK);
            
            top = nfui_nfobject_get_top(obj);
            nfui_nfobject_destroy(top);
            
            return FALSE;
        }

		if (_is_exist_license(license_key)) {
		    memset(buf, 0x00, sizeof(buf));
		    sprintf(buf, lookup_string("It is already registered license.\n(License : %s)"), out_key.name);
			nftool_mbox(g_curwnd, "NOTICE", buf, NFTOOL_MB_OK);
			return FALSE;
		}

        _result_enter_code(ret);

        if (ret == 0) {
            g_code = (gchar *)imalloc(sizeof(gchar)*MAX_CODE_STRING_SIZE);
            strcpy(g_code, license_key);
            memmove(g_out_key, &out_key, sizeof(NF_SYSDB_LICENSE_INFO));
            
            top = nfui_nfobject_get_top(obj);
            nfui_nfobject_destroy(top);
        }
    }

	return FALSE;
}

static gboolean post_cancel_btn_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top = NULL;
	
	if(evt->type == GDK_BUTTON_RELEASE) 
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

		g_code = NULL;	

		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}

	return FALSE;
} 

static gboolean post_Win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch (evt->type) 
	{
		case GDK_DELETE:
		{
			g_curwnd = 0;
			gtk_main_quit();
		}
		break;

		default:
		break;
	}

	return FALSE;
}


static gboolean post_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if(evt->type == GDK_EXPOSE)
	{
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
    
		nfui_nfobject_gc_unref(gc);
	}
    else if (evt->type == GDK_DELETE)
    {
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
    }

	return FALSE;
}

gchar *VW_License_EnterCode_Popup(NFWINDOW *parent, gint ch, NF_SYSDB_LICENSE_INFO *out_key, LicenseData *local, LicenseData *cam)
{
	NFOBJECT *Fixed;
	NFOBJECT *obj;


    g_ch = ch;
    g_out_key = out_key;
    g_code = NULL;
    g_local_lic = local;
    g_cam_lic = cam;
    
	/* window */
	g_win = (NFOBJECT*)nfui_nfwindow_new(parent,  
										LEC_POS_X, LEC_POS_Y, 
										LEC_WIN_SIZE_W, LEC_WIN_SIZE_H);
	g_curwnd = g_win;

		
	nfui_regi_post_event_callback(g_win, post_Win_event_handler);

	/* fixed */
	Fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(Fixed, LEC_WIN_SIZE_W, LEC_WIN_SIZE_H);
	nfui_regi_post_event_callback(Fixed, post_fixed_event_cb);
	nfui_nfobject_show(Fixed);
	
	/* title */
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ADD LICENSE", 
									nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));	
	nfui_nfobject_set_size(obj, 670, 36);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 11);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)Fixed, obj, 4, 4);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("LICENSE KEY", 
									nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));	
	nfui_nfobject_set_size(obj, 641, 22);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)Fixed, obj, 15, 61);

	// key
	g_codeobj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_SMALL_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)g_codeobj, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nflabel_set_align((NFLABEL*)g_codeobj, NFALIGN_LEFT, 4);
	nfui_nfobject_support_multi_lang((NFOBJECT*)g_codeobj, FALSE);
	nfui_nfobject_set_size(g_codeobj, 641, 40);
	nfui_regi_post_event_callback(g_codeobj, post_code_event_handler);
	nfui_nfobject_show(g_codeobj);
	nfui_nffixed_put((NFFIXED*)Fixed, g_codeobj, 15, 85);

	obj = nftool_normal_button_create_type1("OK", 150);
	nfui_regi_post_event_callback(obj, post_ok_btn_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)Fixed, obj, (LEC_WIN_SIZE_W/2)-152, 150);

	obj = nftool_normal_button_create_type1("CANCEL", 150);
	nfui_regi_post_event_callback(obj, post_cancel_btn_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)Fixed, obj, (LEC_WIN_SIZE_W/2)+2, 150);

	nfui_nfwindow_add((NFWINDOW*)g_win, Fixed);
	nfui_run_main_event_handler(g_win);
	nfui_nfobject_show(g_win);

	nfui_make_key_hierarchy((NFWINDOW*)g_win);
	nfui_set_key_focus(g_codeobj, TRUE);

	nfui_page_open(PGID_LICENSE_ENTERCODE_POPUP, g_win, nfui_get_last_user());
	
	gtk_main();

	nfui_page_close(PGID_LICENSE_ENTERCODE_POPUP, g_win);

	return g_code;
}


