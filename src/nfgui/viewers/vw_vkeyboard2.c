
#include "support/color.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/event_loop.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nfcheckbutton.h"
#include "objects/nflabel.h"
#include "objects/nftable.h"
#include "objects/nfvklabel.h"

#include "vw_vkeyboard2.h"
#include "vw_passage_popup.h"
#include "ix_mem.h"
#include "ssm.h"
#include "cmm.h"


#define VKEY_SIZE_WID				(705)
#define VKEY_SIZE_HEI				(420)

#define VKEY_LABEL_POS_X			(15)
#define VKEY_LABEL_POS_Y			(52)
#define VKEY_LABEL_SIZE_WID			(668)
#define VKEY_LABEL_SIZE_HEI			(40)

#define VALID_LABEL_POS_X			(VKEY_LABEL_POS_X)
#define VALID_LABEL_POS_Y			(VKEY_LABEL_POS_Y + VKEY_LABEL_SIZE_HEI + 8)
#define VALID_LABEL_SIZE_WID		(668)
#define VALID_LABEL_SIZE_HEI		(40)

#define VKEY_FIXED_POS_X			(VKEY_LABEL_POS_X)
#define VKEY_FIXED_POS_Y			(VALID_LABEL_POS_Y+VALID_LABEL_SIZE_HEI+8)
#define VKEY_FIXED_SIZE_WID			(VKEY_LABEL_SIZE_WID)
#define VKEY_FIXED_SIZE_HEI			(247)

enum {
	BACKSPACE_KEY = 13,
	DELETE_KEY 	  = 27,
	ENTER_KEY 	  = 39,
	SPACE_KEY 	  = 50,
	SHIFT_KEY 	  = 51,
	KEY_CNT 
};

#define HELP_STR1 "A password must satisfy the following rules:"
#define HELP_STR2 "Must be greater than 8 characters and less than 16 characters long."
#define HELP_STR3 "Must use that consist the English capital letters, English lowercase letters, special characters and Numerics."
#define HELP_STR4 "The same characters cannot be repeated more than 3 times."
#define HELP_STR5 "You cannot use more than three sequential numbers or letters."
#define HELP_STR6 "The corresponding ID cannot be used as part of the password."

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *main_wnd;
static NFOBJECT *main_fixed;
static NFOBJECT *key_fixed;

static NFOBJECT *g_vklabel;
static NFOBJECT *g_keyBtn[KEY_CNT];
static NFOBJECT *g_okBtn;
static NFOBJECT *strValid = NULL;
static gchar *strResult = NULL;
static gint g_max_ch;
static int shift = 0;

static gchar *g_strID = 0;
static gchar *g_strPASSWORD = 0;

static guint repeat_src = 0;
static guint repeat_short = 0;

gchar *(*keyVal2)[KEY_CNT] = NULL;

gchar *keyVal2_default[2][KEY_CNT] = 
{
	{"~", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "BS", 
	"q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "{", "}", "\\", "DEL",
	"a", "s", "d", "f", "g", "h", "j", "k", "l", ".", "\"", "ENTER",
	"z", "x", "c", "v", "b", "n", "m", "<", ">", "?", "SPACE", "SHIFT"},

	{"~", "!", "@", "#", "$", "%", "^", "&", "*", "(", ")", "_", "+", "BS", 
	"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "{", "}", "|", "DEL",
	"A", "S", "D", "F", "G", "H", "J", "K", "L", ":", "\"", "ENTER",
	"Z", "X", "C", "V", "B", "N", "M", "<", ">", "?", "SPACE", "SHIFT"},
};

gchar *keyVal2_all[2][KEY_CNT] = 
{
	{"~", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "BS", 
	"q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "{", "}", "\\", "DEL",
	"a", "s", "d", "f", "g", "h", "j", "k", "l", ".", "\"", "ENTER",
	"z", "x", "c", "v", "b", "n", "m", "<", ">", "?", "SPACE", "SHIFT"},

	{"\`", "!", "@", "#", "$", "%", "^", "&", "*", "(", ")", "_", "+", "BS", 
	"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "[", "]", "|", "DEL",
	"A", "S", "D", "F", "G", "H", "J", "K", "L", ":", "\'", "ENTER",
	"Z", "X", "C", "V", "B", "N", "M", ",", ";", "/", "SPACE", "SHIFT"},
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

static void change_shift_key()
{
	gint i;
	gchar strBuf[16];
	gchar pStr = NULL;

	shift = !shift; 
	for(i=0; i<KEY_CNT; i++) {

		memset(strBuf, 0x00, sizeof(strBuf));
		printf("%s\n", keyVal2[shift][i]);
		if (strcmp(keyVal2[shift][i], "") == 0 || i==39) nfui_nfobject_disable(g_keyBtn[i]);
		else nfui_nfobject_enable(g_keyBtn[i]);

		strcpy(strBuf, keyVal2[shift][i]);
		if (i == BACKSPACE_KEY) memset(strBuf, 0x00, sizeof(strBuf));
		nfui_nfbutton_set_text((NFBUTTON*)(g_keyBtn[i]), strBuf);

		nfui_nfobject_show(g_keyBtn[i]);
		nfui_signal_emit(g_keyBtn[i], GDK_EXPOSE, TRUE);
	}
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
		{	
			if(ssm_check_enhanced_passwd(g_strID, nfui_nfvklabel_get_all_str((NFVKLABEL*)g_vklabel)) == PASS_SUCCESS) 
			{
				nfui_nflabel_set_text(strValid, "");
				nfui_nfobject_enable(g_okBtn);
			}
			else 
			{
				nfui_nflabel_set_text(strValid, "Too weak");				
				nfui_nfobject_disable(g_okBtn);
			}

			nfui_signal_emit(g_vklabel, GDK_EXPOSE, FALSE);
			nfui_signal_emit(strValid, GDK_EXPOSE, TRUE);
			nfui_signal_emit(g_okBtn, GDK_EXPOSE, TRUE);
	        }		
	}

	return FALSE;
}

static gboolean post_func_key_event_cb(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if(event->type == GDK_BUTTON_PRESS) {
		gint key_index = -1;
		
		if(event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		key_index = get_pressed_key_index(obj);
		if(key_index < 0) return FALSE;

		switch(key_index) {
			case BACKSPACE_KEY:
			{
				if (nfui_nfvklabel_erase((NFVKLABEL*)g_vklabel, VK_ERASE_BACKSPACE) == 0)
                {               
    				if(ssm_check_enhanced_passwd(g_strID, nfui_nfvklabel_get_all_str((NFVKLABEL*)g_vklabel)) == PASS_SUCCESS) 
					{
    					nfui_nflabel_set_text(strValid, "");
    					nfui_nfobject_enable(g_okBtn);
    				}
    				else 
					{
    					nfui_nflabel_set_text(strValid, "Too weak");
    					nfui_nfobject_disable(g_okBtn);
    				}

                    nfui_signal_emit(g_vklabel, GDK_EXPOSE, FALSE);
   					nfui_signal_emit(strValid, GDK_EXPOSE, TRUE);
    				nfui_signal_emit(g_okBtn, GDK_EXPOSE, TRUE);

                    if (!repeat_src) repeat_src = g_timeout_add(320, _repeat_key_proc, GINT_TO_POINTER(key_index));    				
                }
            }
			break;

			case DELETE_KEY:
			{
				if (nfui_nfvklabel_erase((NFVKLABEL*)g_vklabel, VK_ERASE_DEL) == 0)
                {
    				if(ssm_check_enhanced_passwd(g_strID, nfui_nfvklabel_get_all_str((NFVKLABEL*)g_vklabel)) == PASS_SUCCESS) 
					{
    					nfui_nflabel_set_text(strValid, "");
    					nfui_nfobject_enable(g_okBtn);
    				}
    				else 
					{
    					nfui_nflabel_set_text(strValid, "Too weak");
    					nfui_nfobject_disable(g_okBtn);
    				}

                    nfui_signal_emit(g_vklabel, GDK_EXPOSE, FALSE);
   					nfui_signal_emit(strValid, GDK_EXPOSE, TRUE);
    				nfui_signal_emit(g_okBtn, GDK_EXPOSE, TRUE);

                    if (!repeat_src) repeat_src = g_timeout_add(320, _repeat_key_proc, GINT_TO_POINTER(key_index));					    				
                }
			}
			break;

			case SPACE_KEY:
			{
				if (nfui_nfvklabel_input_character((NFVKLABEL*)g_vklabel, ' ') == 0)
				{
					if(ssm_check_enhanced_passwd(g_strID, nfui_nfvklabel_get_all_str((NFVKLABEL*)g_vklabel)) == PASS_SUCCESS) 
					{
						nfui_nflabel_set_text(strValid, "");
						nfui_nfobject_enable(g_okBtn);
					}
					else 
					{
						nfui_nflabel_set_text(strValid, "Too weak");
						nfui_nfobject_disable(g_okBtn);
					}

					nfui_signal_emit(g_vklabel, GDK_EXPOSE, FALSE);
					nfui_signal_emit(strValid, GDK_EXPOSE, TRUE);
					nfui_signal_emit(g_okBtn, GDK_EXPOSE, TRUE);
                }
			}
			break;
				
			case SHIFT_KEY:
				change_shift_key();
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

static gboolean post_help_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_BUTTON_RELEASE || kpid == KEYPAD_ENTER)
	{
		NFOBJECT *top;
		guint x, y;

        PARAGRAPH_STR *para;
        gint i;

  	   	if (evt->button.button == MOUSE_RIGTH_BUTTON)  
  	   	    return FALSE;

		nfui_nfobject_get_offset(obj, &x, &y);
		top = nfui_nfobject_get_top(obj);

        x += top->x + obj->width;
        y += top->y + obj->height + 4;

        para = imalloc(sizeof(PARAGRAPH_STR));
        
        para->intro[0] = g_strdup(HELP_STR1);
        para->intro_cnt = 1;

        para->body[0] = g_strdup(HELP_STR2);
        para->body[1] = g_strdup(HELP_STR3);
        para->body[2] = g_strdup(HELP_STR4);
        para->body[3] = g_strdup(HELP_STR5);
        para->body[4] = g_strdup(HELP_STR6);        
        para->body_cnt = 5;

        vw_passage_popup_open(g_curwnd, x, y, DIR_BOTTOM_LEFT, &para, 1);

        for (i = 0; i < para->intro_cnt; i++)
        {
            if (para->intro[i]) g_free(para->intro[i]);
        }

        for (i = 0; i < para->body_cnt; i++)
        {
            if (para->body[i]) g_free(para->body[i]);
        }

        ifree(para);
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
	else if(evt->type == GDK_DELETE)
	{
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
	}	
}


gchar* VirtualKey_Open2(NFWINDOW *parent, const gchar *id, const gchar *password, guint x, guint y, gint max_ch, vkey2_mode_type vkey2_mode)
{
	NFOBJECT *close_btn;
	NFOBJECT *ntb;
	NFOBJECT *obj = NULL;

	GdkPixbuf *bgBtn1_W44[NFOBJECT_STATE_COUNT];
	GdkPixbuf *bgBtn2_W44[NFOBJECT_STATE_COUNT];
	GdkPixbuf *bgBtn_W140[NFOBJECT_STATE_COUNT];
	GdkPixbuf *bgBtn_W92[NFOBJECT_STATE_COUNT];
	GdkPixbuf *backspace[NFOBJECT_STATE_COUNT];
	GdkPixbuf *bgBtn_help[NFOBJECT_STATE_COUNT];

	guint key_fg_1[NFOBJECT_STATE_COUNT] = {COLOR_IDX(278), COLOR_IDX(279), COLOR_IDX(280), COLOR_IDX(281)}; 
	guint key_fg_2[NFOBJECT_STATE_COUNT] = {COLOR_IDX(282), COLOR_IDX(283), COLOR_IDX(285), COLOR_IDX(286)}; 

	gint size_w, size_h;
	guint i;

	shift = 0;

	keyVal2 = keyVal2_all;

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

	bgBtn_W140[0] = nfui_get_image_from_file((IMG_N_KEY_ENTER), NULL); 
	bgBtn_W140[1] = nfui_get_image_from_file((IMG_O_KEY_ENTER), NULL); 
	bgBtn_W140[2] = nfui_get_image_from_file((IMG_P_KEY_ENTER), NULL); 
	bgBtn_W140[3] = nfui_get_image_from_file((IMG_D_KEY_ENTER), NULL); 

	bgBtn_W92[0] = nfui_get_image_from_file((IMG_N_KEY_03), NULL); 
	bgBtn_W92[1] = nfui_get_image_from_file((IMG_O_KEY_03), NULL); 
	bgBtn_W92[2] = nfui_get_image_from_file((IMG_P_KEY_03), NULL); 
	bgBtn_W92[3] = nfui_get_image_from_file((IMG_D_KEY_03), NULL); 

	bgBtn_help[0] = nfui_get_image_from_file((IMG_KEY_HELP_N), NULL); 
	bgBtn_help[1] = nfui_get_image_from_file((IMG_KEY_HELP_O), NULL); 
	bgBtn_help[2] = nfui_get_image_from_file((IMG_KEY_HELP_P), NULL); 
	bgBtn_help[3] = nfui_get_image_from_file((IMG_KEY_HELP_D), NULL); 

    g_strID = id;
    g_strPASSWORD = password;

	main_wnd = (NFOBJECT*)nftool_create_popup_window(parent, x, y, VKEY_SIZE_WID, VKEY_SIZE_HEI, "VIRTUAL KEYBOARD", FALSE);
	if(main_wnd == NULL)	return NULL;
	g_curwnd = main_wnd;
	nfui_nfwindow_set_title(main_wnd, "VIRTUAL KEYBOARD");
	nfui_regi_post_event_callback(main_wnd, post_main_win_event_handler);
	gtk_window_set_resizable(((NFWINDOW*)main_wnd)->main_widget, TRUE);
	nfui_nfobject_set_data(main_wnd, "vkey2_mode", GUINT_TO_POINTER(vkey2_mode));
	
	main_fixed = ((NFWINDOW*)main_wnd)->child;
	nfui_regi_post_event_callback(main_fixed, post_main_fixed_event_handler);

	if (password)   g_vklabel = (NFOBJECT*)nfui_nfvklabel_new_str((gchar*)password, max_ch);
    else            g_vklabel = (NFOBJECT*)nfui_nfvklabel_new_str("", max_ch);
	nfui_nfvklabel_set_pango_font((NFVKLABEL*)g_vklabel, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(221));
	nfui_nfobject_modify_bg(g_vklabel, NFOBJECT_STATE_NORMAL, COLOR_IDX(220));
	nfui_nfobject_set_size(g_vklabel, VKEY_LABEL_SIZE_WID-48, VKEY_LABEL_SIZE_HEI);
	nfui_nffixed_put((NFFIXED*)main_fixed, g_vklabel, VKEY_LABEL_POS_X, VKEY_LABEL_POS_Y);
	nfui_nfobject_show(g_vklabel);

	if (vkey2_mode == VKEY2_PASSWORD) nfui_nfvklabel_set_invisible((NFVKLABEL*)g_vklabel, 1);

    obj = (NFOBJECT*)nfui_nfbutton_new_with_param(bgBtn_help, "");
	nfui_nfbutton_set_pango_font((NFBUTTON*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), (guint*)key_fg_1);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, VKEY_LABEL_POS_X+VKEY_LABEL_SIZE_WID-42, VKEY_LABEL_POS_Y);
    nfui_regi_post_event_callback(obj, post_help_button_event_handler); 
 
	strValid = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nfobject_modify_bg(strValid, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nflabel_set_align((NFLABEL*)strValid, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(strValid, VALID_LABEL_SIZE_WID, VALID_LABEL_SIZE_HEI);
	nfui_nfobject_use_focus(strValid, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(strValid);
	nfui_nffixed_put((NFFIXED*)main_fixed, strValid, VALID_LABEL_POS_X, VALID_LABEL_POS_Y);

	key_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(key_fixed, VKEY_FIXED_SIZE_WID, VKEY_FIXED_SIZE_HEI);
	nfui_nffixed_put((NFFIXED*)main_fixed, key_fixed, VKEY_FIXED_POS_X, VKEY_FIXED_POS_Y);

	for(i=0; i<KEY_CNT; i++) {
		if(i == 13) 	 	g_keyBtn[i] = (NFOBJECT*)nfui_nfbutton_new_with_param(backspace, "");	
		else if(i == 27) 	g_keyBtn[i] = (NFOBJECT*)nfui_nfbutton_new_with_param(bgBtn2_W44, keyVal2[0][i]);	
		else if(i == 39)
		{
			g_keyBtn[i] = (NFOBJECT*)nfui_nfbutton_new_with_param(bgBtn_W140, keyVal2[0][i]);
			nfui_nfobject_support_multi_lang(g_keyBtn[i], FALSE);
			nfui_nfobject_disable(g_keyBtn[i]);
		}
		else if(i == 50) 	g_keyBtn[i] = (NFOBJECT*)nfui_nfbutton_new_with_param(bgBtn_W92, keyVal2[0][i]);	
		else if(i == 51) 	g_keyBtn[i] = (NFOBJECT*)nfui_nfbutton_new_with_param(bgBtn_W92, keyVal2[0][i]);	
		else 			 	g_keyBtn[i] = (NFOBJECT*)nfui_nfbutton_new_with_param(bgBtn1_W44, keyVal2[0][i]);

		if(i == 13 || i == 27 || i == 39 || i == 50 || i == 51) {
			nfui_nfbutton_set_pango_font((NFBUTTON*)g_keyBtn[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), (guint*)key_fg_2);
			if(i == 39) nfui_regi_post_event_callback(g_keyBtn[i], ok_button_event_handler);
			else 		nfui_regi_post_event_callback(g_keyBtn[i], post_func_key_event_cb);
		} else													{
			nfui_nfbutton_set_pango_font((NFBUTTON*)g_keyBtn[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), (guint*)key_fg_1);
			nfui_regi_post_event_callback(g_keyBtn[i], post_key_event_cb);
		}

		if (strcmp(keyVal2[0][i], "") == 0) nfui_nfobject_disable(g_keyBtn[i]);
		nfui_nfobject_show(g_keyBtn[i]);

		if(i < 14) 		 nfui_nffixed_put((NFFIXED*)key_fixed, g_keyBtn[i], 48 * i, 0);
		else if(i < 28)	 nfui_nffixed_put((NFFIXED*)key_fixed, g_keyBtn[i], 48 * (i % 14), 48);
		else if(i < 40)  nfui_nffixed_put((NFFIXED*)key_fixed, g_keyBtn[i], 48 * (i % 28), 48*2);
		else if(i == 51) nfui_nffixed_put((NFFIXED*)key_fixed, g_keyBtn[i], 48 * ((i % 40) + 1), 48*3);
		else 			 nfui_nffixed_put((NFFIXED*)key_fixed, g_keyBtn[i], 48 * (i % 40), 48*3);
	}

// OK , CANCEL BUTTON
	g_okBtn = nftool_normal_button_create_type1("OK", 192);
	nfui_regi_post_event_callback(g_okBtn, ok_button_event_handler);
	nfui_nfobject_show(g_okBtn);
	nfui_nffixed_put((NFFIXED*)key_fixed, g_okBtn, VKEY_FIXED_SIZE_WID/2-195, VKEY_FIXED_SIZE_HEI-44);

	obj = nftool_normal_button_create_type1("CANCEL", 192);
	nfui_regi_post_event_callback(obj, cancel_button_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)key_fixed, obj, VKEY_FIXED_SIZE_WID/2+3, VKEY_FIXED_SIZE_HEI-44);

	if(ssm_check_enhanced_passwd(g_strID, nfui_nfvklabel_get_all_str((NFVKLABEL*)g_vklabel)) != PASS_SUCCESS)
	{
		nfui_nfobject_disable(g_okBtn);
		nfui_nflabel_set_text(strValid, "Too weak");
	}

	nfui_nfobject_show(key_fixed);
	nfui_nfobject_show(main_fixed);
	nfui_nfobject_show(main_wnd);

	uxm_reg_imsg_event(main_wnd, INFY_VKEY_SIZE_INCREASE);
	uxm_reg_imsg_event(main_wnd, INFY_VKEY_SIZE_DECREASE);

	nfui_make_key_hierarchy(main_wnd);
	nfui_page_close(PGID_POPUPWND, main_wnd);
	nfui_page_open(PGID_SETUP_VKEY, main_wnd, nfui_get_last_user());

	gtk_main();

	nfui_page_close(PGID_SETUP_VKEY, main_wnd);

	return strResult;
}

