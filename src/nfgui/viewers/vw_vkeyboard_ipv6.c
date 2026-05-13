
#include "support/color.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/event_loop.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"

#include "tools/nf_ui_tool.h"

#include "services/uxm.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nfcheckbutton.h"
#include "objects/nflabel.h"
#include "objects/nftable.h"
#include "objects/nfvklabel.h"

#include "vw_vkeyboard_ipv6.h"
#include "ix_mem.h"
#include "cmm.h"


#define VKEY_SIZE_WID				(500)
#define VKEY_SIZE_HEI				(276)

#define VKEY_LABEL_POS_X			(15)
#define VKEY_LABEL_POS_Y			(52)
#define VKEY_LABEL_SIZE_WID			(473)
#define VKEY_LABEL_SIZE_HEI			(40)

#define VKEY_FIXED_POS_X			(VKEY_LABEL_POS_X)
#define VKEY_FIXED_POS_Y			(VKEY_LABEL_POS_Y+VKEY_LABEL_SIZE_HEI+8)
#define VKEY_FIXED_SIZE_WID			(VKEY_LABEL_SIZE_WID)
#define VKEY_FIXED_SIZE_HEI			(151)

enum {
	BACKSPACE_KEY = 9,
	DELETE_KEY 	  = 19,
	KEY_CNT 
};

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *main_wnd;
static NFOBJECT *main_fixed;
static NFOBJECT *key_fixed;

static NFOBJECT *g_vklabel;
static NFOBJECT *g_keyBtn[KEY_CNT];
static gchar *strResult = NULL;
static gint g_max_ch;
static int shift = 0;

static guint repeat_src = 0;
static guint repeat_short = 0;

static gchar *(*keyVal)[KEY_CNT] = NULL;
static gchar *(*active)[KEY_CNT] = NULL;


static gchar *keyVal_ipv6[2][KEY_CNT] = 
{
	{"0", "1", "2", "3", "4", "5", "6", "7", ":", "BS", 
	"8", "9", "a", "b", "c", "d", "e", "f", "", "DEL",},

	{"0", "1", "2", "3", "4", "5", "6", "7", ":", "BS", 
	"8", "9", "a", "b", "c", "d", "e", "f", "", "DEL",},
};

static gchar *actv_ipv6[2][KEY_CNT] = 
{
	{"0", "1", "2", "3", "4", "5", "6", "7", ":", "BS", 
	"8", "9", "a", "b", "c", "d", "e", "f", "", "DEL",},

	{"0", "1", "2", "3", "4", "5", "6", "7", ":", "BS", 
	"8", "9", "a", "b", "c", "d", "e", "f", "", "DEL",},
};



static gint get_pressed_key_index(NFOBJECT *obj)
{
	gint index = 0;

	while(index < KEY_CNT) {
		if(g_keyBtn[index] == obj) 
			return index;	

		++index;
	}

	return -1;
}

static gboolean add_letter(gchar letter)
{
	gchar strTemp[256];
	gint len = 0;

	if(letter == 0) return FALSE;

	memset(strTemp, 0x00, sizeof(strTemp));

	g_stpcpy(strTemp, nfui_nflabel_get_text((NFLABEL*)g_vklabel));
	len = strlen(strTemp);

	if(len >= g_max_ch) return FALSE;

	strTemp[len] = letter;
	nfui_nflabel_set_text((NFLABEL*)g_vklabel, strTemp);
	nfui_signal_emit(g_vklabel, GDK_EXPOSE, TRUE);
	
	return TRUE;
}

static gboolean delete_letter()
{
	gchar strTemp[256];
	gint len = 0;

	memset(strTemp, 0x00, sizeof(strTemp));

	g_stpcpy(strTemp, nfui_nflabel_get_text((NFLABEL*)g_vklabel));
	len = strlen(strTemp);

	if(len <= 0) return FALSE;

	strTemp[len-1] = '\0';
	nfui_nflabel_set_text((NFLABEL*)g_vklabel, strTemp);
	nfui_signal_emit(g_vklabel, GDK_EXPOSE, TRUE);
	
	return TRUE;
}

static gboolean delete_all_letter()
{
	gchar *pStr;
	gint len = 0;

	pStr = nfui_nflabel_get_text((NFLABEL*)g_vklabel);
	if(!pStr) return FALSE;

	len = strlen(pStr);

	if(len <= 0) return FALSE;

	nfui_nflabel_set_text((NFLABEL*)g_vklabel, "");
	nfui_signal_emit(g_vklabel, GDK_EXPOSE, TRUE);
	
	return TRUE;
}

static gboolean _repeat_key_short_proc(gpointer data)
{
    gint key_index = -1;
    gint vk_erase = 0;
    gint ret;
   	gchar *st;

    key_index = GPOINTER_TO_INT(data);

    if ((key_index == BACKSPACE_KEY) || (key_index == DELETE_KEY))
    {
        if (key_index == BACKSPACE_KEY)     vk_erase = VK_ERASE_BACKSPACE;
        else if (key_index == DELETE_KEY)   vk_erase = VK_ERASE_DEL;        

        ret = nfui_nfvklabel_erase((NFVKLABEL*)g_vklabel, vk_erase);

        if (ret == 0) {
            nfui_signal_emit(g_vklabel, GDK_EXPOSE, FALSE);
        }
        else {
            repeat_short = 0;
            return FALSE;
        }
    }

    return TRUE;
}

static gboolean _repeat_key_proc(gpointer data)
{
    gint key_index = -1;
    gint vk_erase = 0;
    gint ret;

    key_index = GPOINTER_TO_INT(data);

    if ((key_index == BACKSPACE_KEY) || (key_index == DELETE_KEY))
    {
        if (key_index == BACKSPACE_KEY)     vk_erase = VK_ERASE_BACKSPACE;
        else if (key_index == DELETE_KEY)   vk_erase = VK_ERASE_DEL;        

        ret = nfui_nfvklabel_erase((NFVKLABEL*)g_vklabel, vk_erase);

        if (ret != -1) {
			if (ret == 0) nfui_signal_emit(g_vklabel, GDK_EXPOSE, FALSE);
            if (!repeat_short) repeat_short = g_timeout_add(160, _repeat_key_short_proc, GINT_TO_POINTER(key_index));
        }    
    }

    repeat_src = 0;

    return FALSE;
}

static gboolean post_key_event_cb(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if(event->type == GDK_BUTTON_PRESS) {
		gint key_index = -1;
		gchar val[8];

		if(event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		key_index = get_pressed_key_index(obj);
		if(key_index < 0) return FALSE;

		strcpy(val, nfui_nfbutton_get_text((NFBUTTON*)(obj)));

		if (nfui_nfvklabel_input_character((NFVKLABEL*)g_vklabel, val[0]) == 0)
			nfui_signal_emit(g_vklabel, GDK_EXPOSE, FALSE);	
	}

	return FALSE;
}

static gboolean post_func_key_event_cb(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if (event->type == GDK_BUTTON_PRESS) 
	{
		gint key_index = -1;
		
		if(event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		key_index = get_pressed_key_index(obj);
		if(key_index < 0) return FALSE;

		switch(key_index) {
			case BACKSPACE_KEY:
			{
				if (nfui_nfvklabel_erase((NFVKLABEL*)g_vklabel, VK_ERASE_BACKSPACE) == 0)
				{
					nfui_signal_emit(g_vklabel, GDK_EXPOSE, FALSE);
                    if (!repeat_src) repeat_src = g_timeout_add(320, _repeat_key_proc, GINT_TO_POINTER(key_index));
                }
            }
			break;
				
			case DELETE_KEY:
			{
				if (nfui_nfvklabel_erase((NFVKLABEL*)g_vklabel, VK_ERASE_DEL) == 0)
				{
					nfui_signal_emit(g_vklabel, GDK_EXPOSE, FALSE);
                    if (!repeat_src) repeat_src = g_timeout_add(320, _repeat_key_proc, GINT_TO_POINTER(key_index));					
                }
            }
			break;

			default:
				return FALSE;
		}
	}
	else if ((event->type == GDK_BUTTON_RELEASE) || (event->type == GDK_LEAVE_NOTIFY))
	{
		if(event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        if (repeat_src)
        {      
            g_source_remove(repeat_src);
            repeat_src = 0;
        }
            
        if (repeat_short)  
        {
            g_source_remove(repeat_short);
            repeat_short = 0;
        }
	}

	return FALSE;
}

static gboolean ok_button_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	guint len;
	gchar *st;

	if(event->type == GDK_BUTTON_RELEASE)
	{
		if(event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		st = nfui_nfvklabel_get_all_str((NFVKLABEL*)g_vklabel);
		len = strlen(st);

		strResult = imalloc(sizeof(gchar)*(len+1));
		if(len)	g_stpcpy(strResult, st);

		nfui_nfobject_destroy(main_wnd);
	}

	return FALSE;
}

static gboolean cancel_button_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if(event->type == GDK_BUTTON_RELEASE)
	{
		if(event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
	
		strResult = NULL;
		nfui_nfobject_destroy(main_wnd);
	}

	return FALSE;
}

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_EXPOSE)
	{

	}	
	else if ((evt->type == NFEVENT_REMOCON_RELEASE) || (evt->type == NFEVENT_KEYPAD_RELEASE))
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		gchar buf[8];

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		switch(kpid) 
		{
			case KEYPAD_CH01:
			case KEYPAD_CH02:
			case KEYPAD_CH03:
			case KEYPAD_CH04:
			case KEYPAD_CH05:
			case KEYPAD_CH06:
			case KEYPAD_CH07:
			case KEYPAD_CH08:
			case KEYPAD_CH09:
			{
				g_sprintf(buf, "%d", kpid + 1);

				if (nfui_nfvklabel_input_character((NFVKLABEL*)g_vklabel, buf[0]) == 0)
					nfui_signal_emit(g_vklabel, GDK_EXPOSE, FALSE);
			}
			break;

			case KEYPAD_CH00:
			case RMC_NUM_0:
			{
				g_sprintf(buf, "%d", 0);

				if (nfui_nfvklabel_input_character((NFVKLABEL*)g_vklabel, buf[0]) == 0)
					nfui_signal_emit(g_vklabel, GDK_EXPOSE, FALSE);
			}
			break;
			
			default:
			return FALSE;
		}		
	}		
	else if (evt->type == INFY_VKEY_SIZE_INCREASE)
	{		
		CMM_MESSAGE_T *pmsg;
		gint increase;
		gint win_x, win_y;
		gint off_x, off_y;
		gint size_w, size_h;
		
		pmsg = (CMM_MESSAGE_T *)data;			
		increase = GPOINTER_TO_INT(pmsg->data);

		nfui_nfobject_hide(obj);

		nfui_nfobject_get_size(obj, &size_w, &size_h);
		nfui_nfobject_set_size(obj, size_w, size_h+increase);

		nfui_nfobject_get_window_pos(obj, &win_x, &win_y);
		nfui_nfobject_move(obj, win_x, win_y-increase);

		nfui_nfobject_get_offset(key_fixed, &off_x, &off_y);
		nfui_nfobject_move(key_fixed, off_x, off_y+increase);

		nfui_nfobject_show(obj);
	}
	else if (evt->type == INFY_VKEY_SIZE_DECREASE)
	{	
		CMM_MESSAGE_T *pmsg;
		gint decrease;
		gint win_x, win_y;
		gint off_x, off_y;		
		gint size_w, size_h;
	
		pmsg = (CMM_MESSAGE_T *)data;			
		decrease = GPOINTER_TO_INT(pmsg->data);

		nfui_nfobject_hide(obj);
		
		nfui_nfobject_get_size(obj, &size_w, &size_h);	
		nfui_nfobject_set_size(obj, size_w, size_h-decrease);

		nfui_nfobject_get_window_pos(obj, &win_x, &win_y);
		nfui_nfobject_move(obj, win_x, win_y+decrease);

		nfui_nfobject_get_offset(key_fixed, &off_x, &off_y);
		nfui_nfobject_move(key_fixed, off_x, off_y-decrease);

		nfui_nfobject_show(obj);
	}	
	else if (evt->type == GDK_DELETE)
	{
		uxm_unreg_imsg_event(obj, INFY_VKEY_SIZE_INCREASE);
		uxm_unreg_imsg_event(obj, INFY_VKEY_SIZE_DECREASE);

        if (repeat_src)
        {
            g_source_remove(repeat_src);
            repeat_src = 0;
        }
            
        if (repeat_short)  
        {
            g_source_remove(repeat_short);
            repeat_short = 0;
        }
		
		g_curwnd = 0;
		gtk_main_quit();
	}

	return FALSE;
}

static gboolean post_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
		GdkDrawable *drawable = NULL;
		GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
		gint size_w, size_h;

	if (evt->type == GDK_EXPOSE)
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
}

gchar* VirtualKey_IPv6_Open(NFWINDOW *parent, const gchar *str, guint x, guint y, gint max_ch)
{
	NFOBJECT *close_btn;
	NFOBJECT *ntb;
	NFOBJECT *obj = NULL;

	GdkPixbuf *bgBtn1_W44[NFOBJECT_STATE_COUNT];
	GdkPixbuf *bgBtn2_W44[NFOBJECT_STATE_COUNT];
	GdkPixbuf *bgBtn_W140[NFOBJECT_STATE_COUNT];
	GdkPixbuf *bgBtn_W92[NFOBJECT_STATE_COUNT];
	GdkPixbuf *backspace[NFOBJECT_STATE_COUNT];


	guint key_fg_1[NFOBJECT_STATE_COUNT] = {COLOR_IDX(278), COLOR_IDX(279), COLOR_IDX(280), COLOR_IDX(281)}; 
	guint key_fg_2[NFOBJECT_STATE_COUNT] = {COLOR_IDX(282), COLOR_IDX(283), COLOR_IDX(285), COLOR_IDX(286)}; 

	gint size_w, size_h;
	guint i;

	shift = 0;

    active = actv_ipv6;
    keyVal = keyVal_ipv6;
	strResult = NULL;
	g_max_ch = max_ch;
	
	if(x + VKEY_SIZE_WID >= DISPLAY_ACTIVE_WIDTH)	x = DISPLAY_ACTIVE_WIDTH - VKEY_SIZE_WID - 6;
	if(y + VKEY_SIZE_HEI >= DISPLAY_ACTIVE_HEIGHT)	y = DISPLAY_ACTIVE_HEIGHT - VKEY_SIZE_HEI - 6;

	bgBtn1_W44[0] = nfui_get_image_from_file((IMG_N_KEY_01), NULL); 
	bgBtn1_W44[1] = nfui_get_image_from_file((IMG_O_KEY_01), NULL); 
	bgBtn1_W44[2] = nfui_get_image_from_file((IMG_P_KEY_01), NULL); 
	bgBtn1_W44[3] = nfui_get_image_from_file((IMG_D_KEY_01), NULL); 

	bgBtn2_W44[0] = nfui_get_image_from_file((IMG_N_KEY_02), NULL); 
	bgBtn2_W44[1] = nfui_get_image_from_file((IMG_O_KEY_02), NULL); 
	bgBtn2_W44[2] = nfui_get_image_from_file((IMG_P_KEY_02), NULL); 
	bgBtn2_W44[3] = nfui_get_image_from_file((IMG_D_KEY_02), NULL); 

	backspace[0] = nfui_get_image_from_file((MK_IMG_N_KEY_BACKSPACE), NULL); 
	backspace[1] = nfui_get_image_from_file((MK_IMG_O_KEY_BACKSPACE), NULL); 
	backspace[2] = nfui_get_image_from_file((MK_IMG_P_KEY_BACKSPACE), NULL); 
	backspace[3] = nfui_get_image_from_file((MK_IMG_D_KEY_BACKSPACE), NULL); 

	main_wnd = (NFOBJECT*)nftool_create_popup_window(parent, x, y, VKEY_SIZE_WID, VKEY_SIZE_HEI, "VIRTUAL KEYBOARD", FALSE);
	if(main_wnd == NULL)	return NULL;
	g_curwnd = (NFWINDOW*)main_wnd;
	nfui_nfwindow_set_title((NFWINDOW*)main_wnd, "VIRTUAL KEYBOARD");
	nfui_regi_post_event_callback(main_wnd, post_main_win_event_handler);
	gtk_window_set_resizable(((NFWINDOW*)main_wnd)->main_widget, TRUE);
	
	main_fixed = ((NFWINDOW*)main_wnd)->child;
	nfui_regi_post_event_callback(main_fixed, post_main_fixed_event_handler);

	g_vklabel = (NFOBJECT*)nfui_nfvklabel_new_str((gchar*)str, max_ch);
	nfui_nfvklabel_set_pango_font((NFVKLABEL*)g_vklabel, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(221));
	nfui_nfobject_modify_bg(g_vklabel, NFOBJECT_STATE_NORMAL, COLOR_IDX(220));
	nfui_nfobject_set_size(g_vklabel, VKEY_LABEL_SIZE_WID, VKEY_LABEL_SIZE_HEI);
	nfui_nffixed_put((NFFIXED*)main_fixed, g_vklabel, VKEY_LABEL_POS_X, VKEY_LABEL_POS_Y);
	nfui_nfobject_show(g_vklabel);
    
	key_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(key_fixed, VKEY_FIXED_SIZE_WID, VKEY_FIXED_SIZE_HEI);
	nfui_nffixed_put((NFFIXED*)main_fixed, key_fixed, VKEY_FIXED_POS_X, VKEY_FIXED_POS_Y);

	for(i=0; i<KEY_CNT; i++) {
		if(i == BACKSPACE_KEY) 	 	g_keyBtn[i] = (NFOBJECT*)nfui_nfbutton_new_with_param(backspace, "");	
		else if(i == DELETE_KEY) 	g_keyBtn[i] = (NFOBJECT*)nfui_nfbutton_new_with_param(bgBtn2_W44, keyVal[0][i]);	
        else 		  	 	        g_keyBtn[i] = (NFOBJECT*)nfui_nfbutton_new_with_param(bgBtn1_W44, keyVal[0][i]);

		if(i == BACKSPACE_KEY || i == DELETE_KEY) {
			nfui_nfbutton_set_pango_font((NFBUTTON*)g_keyBtn[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), (guint*)key_fg_2);
			nfui_regi_post_event_callback(g_keyBtn[i], post_func_key_event_cb);
		} else													{
			nfui_nfbutton_set_pango_font((NFBUTTON*)g_keyBtn[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), (guint*)key_fg_1);
			nfui_regi_post_event_callback(g_keyBtn[i], post_key_event_cb);
		}

		if (strcmp(active[0][i], "") == 0) nfui_nfobject_disable(g_keyBtn[i]);
		
		nfui_nfobject_show(g_keyBtn[i]);

		if(i < 10) 	nfui_nffixed_put((NFFIXED*)key_fixed, g_keyBtn[i], 48 * i, 0);
		else        nfui_nffixed_put((NFFIXED*)key_fixed, g_keyBtn[i], 48 * (i % 10), 48);
	}

// OK , CANCEL BUTTON
	obj = nftool_normal_button_create_type1("OK", 160);
	nfui_regi_post_event_callback(obj, ok_button_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)key_fixed, obj, VKEY_FIXED_SIZE_WID/2-162, VKEY_FIXED_SIZE_HEI-44);

	obj = nftool_normal_button_create_type1("CANCEL", 160);
	nfui_regi_post_event_callback(obj, cancel_button_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)key_fixed, obj, VKEY_FIXED_SIZE_WID/2+2, VKEY_FIXED_SIZE_HEI-44);

	nfui_nfobject_show(key_fixed);
	nfui_nfobject_show(main_fixed);
	nfui_nfobject_show(main_wnd);

	uxm_reg_imsg_event(main_wnd, INFY_VKEY_SIZE_INCREASE);
	uxm_reg_imsg_event(main_wnd, INFY_VKEY_SIZE_DECREASE);

	nfui_make_key_hierarchy((NFWINDOW*)main_wnd);
	nfui_set_key_focus(g_vklabel, TRUE);
	nfui_page_close(PGID_POPUPWND, main_wnd);
	nfui_page_open(PGID_SETUP_VKEY, main_wnd, nfui_get_last_user());

	gtk_main();

	nfui_page_close(PGID_SETUP_VKEY, main_wnd);

	return strResult;
}
