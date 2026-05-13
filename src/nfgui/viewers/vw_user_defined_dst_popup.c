#include "iux_afx.h"
#include "nf_afx.h"
#include "nf_util_time.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "tools/nf_ui_tool.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/util.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfimage.h"
#include "viewers/objects/nfbutton.h"
#include "viewers/objects/nfspinbutton.h"
#include "viewers/objects/nftimelabel.h"
#include "viewers/objects/nfcombobox.h"

#include "vw_set_date_time.h"
#include "vw_date_time.h"
#include "vw_vkeyboard.h"

#define MAX_DST_STRING_SIZE             (5)

#define UDD_WND_POS_X                   (guint)((DISPLAY_ACTIVE_WIDTH - UDD_WND_SIZE_W)/2)
#define UDD_WND_POS_Y                   (guint)((DISPLAY_ACTIVE_HEIGHT - UDD_WND_SIZE_H)/2)
#define UDD_WND_SIZE_W                  (680)
#define UDD_WND_SIZE_H                  (450)

#define UDD_OBJ_START_X                 (4)
#define UDD_OBJ_START_Y                 (50)

#define UDD_LB_SIZE_W                   (300)
#define UDD_OBJ_SIZE_W                  (300)
#define UDD_GAP_2                       (2)
#define UDD_GAP_5                       (5)
#define UDD_GAP_10                      (10)

#define ABBREVIATION_LENGTH_LIMIT       (5)


enum {
    UDD_USE = 0,
    UDD_STD_NAME,
    UDD_DST_NAME,
    UDD_OFFSET_H,
    UDD_OFFSET_M,
    UDD_START,
    UDD_END,

    UDD_CNT
};


static UDD_DATA_T udd;
static NF_TIME_UDD *g_nf_udd = NULL;

static NFOBJECT *g_curwnd = NULL;
static NFOBJECT *g_uddobj[UDD_CNT] = {0,};
static guint g_dform, g_tform;
static gint g_tz_idx;


static void _set_copy_to_nfudd()
{
    g_nf_udd->udd_use = udd.use;
    g_nf_udd->dst_offset = udd.offset;
    g_nf_udd->dst_start = udd.start;
    g_nf_udd->dst_end = udd.end;
    strcpy(g_nf_udd->start_name, udd.std_name);
    strcpy(g_nf_udd->end_name, udd.dst_name);
}

static void _get_string_to_upper(gchar *src, gchar *dest)
{
    gint i;

    memset(dest, 0x00, sizeof(gchar)*(ABBREVIATION_LENGTH_LIMIT+1));
    
    for (i = 0; i < ABBREVIATION_LENGTH_LIMIT; i++)
    {
        dest[i] = toupper(src[i]);
    }
}

static void _get_current_date_time(time_t *tv, gint time)
{
    GTimeVal gtv;

    g_get_current_time(&gtv);
    
    if (time == UDD_START)
    {
        if (udd.start == 0)
        {
            *tv = NF_LOWER_TIMELIMIT;
            udd.start = NF_LOWER_TIMELIMIT;
        }
        else
        {
            *tv = (time_t)udd.start;
        }
    }
    else if (time == UDD_END)
    {
        if (udd.end == 0)
        {
            *tv = NF_LOWER_TIMELIMIT + (3600 * 48);
            udd.end = NF_LOWER_TIMELIMIT + (3600 * 48);
        }
        else
        {
            *tv = (time_t)udd.end;
        }
    }
}

static void _get_offset_from_sec(gint *h_idx, gint *m_idx)
{
    *h_idx = udd.offset / 3600;
    *m_idx = (udd.offset % 3600) / 60;
}

static gint _get_sec_from_offset()
{
    gint h_idx, m_idx;

    h_idx = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_uddobj[UDD_OFFSET_H]);
    m_idx = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_uddobj[UDD_OFFSET_M]);

    return ((h_idx * 3600) + (m_idx * 60));
}

static gint _load_udd_data()
{
    memset(&udd, 0x00, sizeof(UDD_DATA_T));

    if (DAL_get_user_defined_dst_data(&udd))
    {
        g_message("[%s : %d]UDD DATA LOAD FAIL", __FUNCTION__, __LINE__);
        return -1;
    }

    return 0;
}

static gint _get_data_from_obj()
{
    GTimeVal tv;
    
    udd.use = nfui_spin_button_get_index((NFSPINBUTTON*)g_uddobj[UDD_USE]);
    strcpy(udd.std_name, nfui_nflabel_get_text((NFLABEL*)g_uddobj[UDD_STD_NAME]));
    strcpy(udd.dst_name, nfui_nflabel_get_text((NFLABEL*)g_uddobj[UDD_DST_NAME]));
    udd.offset = _get_sec_from_offset();

    _set_copy_to_nfudd();
    
    return 0;
}

static gboolean _post_dst_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;
	gchar buf[ABBREVIATION_LENGTH_LIMIT];

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{
		NFOBJECT *top = 0;
		gchar *title = 0;
		gint x, y;

		if(kpid == KEYPAD_ENTER)
		{
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);

			x += (obj->width)/2 + top->x;
			y += obj->height + top->y;

		}
		else
		{
	  	   	if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			  return FALSE;
	  	   	}
		
			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += ((GdkEventButton*)evt)->x;
			y += ((GdkEventButton*)evt)->y;
		}

		title = VirtualKey_Open((NFWINDOW*)g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), (guint)x, (guint)y, MAX_DST_STRING_SIZE, VKEY_ALPHANUMERIC);
        if(!title || strlen(title) == 0) return FALSE;
        
		if(title)
		{
		    _get_string_to_upper(title, buf);
			nfui_nflabel_set_text((NFLABEL*)obj, buf);

			ifree(title);
			title = NULL;
		}

		nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

static gboolean _post_btn_start_dst_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	guint i;
	NFOBJECT *top;
	int x, y;
    gchar buf[64];
	GTimeVal pre_time;
	GTimeVal post_time;

    memset(buf, 0x00, sizeof(buf));
	memset(&pre_time, 0x00, sizeof(GTimeVal));
	memset(&post_time, 0x00, sizeof(GTimeVal));
	
	if (evt->type == GDK_BUTTON_RELEASE) 
	{	
  	   	if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
	
		nfui_nfobject_get_offset(obj, &x, &y);
		top = nfui_nfobject_get_top(obj);
	
		x += top->x + obj->width + 4;
		y += top->y;
		
		post_time.tv_sec = VW_Set_DateTime_Open_Ver2((NFWINDOW*)g_curwnd, "DATE/TIME", x, y, udd.start, SDT_TYPE_SEC, NF_LOWER_TIMELIMIT, NF_UPPER_TIMELIMIT);
        
        if (post_time.tv_sec != 0)
        {
            udd.start = post_time.tv_sec;
            ifn_get_gmtime_text(udd.start, g_dform, g_tform, buf);
    		nfui_nflabel_set_text((NFLABEL*)g_uddobj[UDD_START], buf);
    		nfui_signal_emit(g_uddobj[UDD_START], GDK_EXPOSE, TRUE);		
        }
	}		

	return FALSE;
}

static gboolean _post_btn_end_dst_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	guint i;
	NFOBJECT *top;
	int x, y;
    gchar buf[64];
	GTimeVal pre_time;
	GTimeVal post_time;

    memset(buf, 0x00, sizeof(buf));
	memset(&pre_time, 0x00, sizeof(GTimeVal));
	memset(&post_time, 0x00, sizeof(GTimeVal));
	
	if (evt->type == GDK_BUTTON_RELEASE) 
	{	
  	   	if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
	
		nfui_nfobject_get_offset(obj, &x, &y);
		top = nfui_nfobject_get_top(obj);
	
		x += top->x + obj->width + 4;
		y += top->y;
	
		post_time.tv_sec = VW_Set_DateTime_Open_Ver2((NFWINDOW*)g_curwnd, "DATE/TIME", x, y, udd.end, SDT_TYPE_SEC, udd.start + (3600 * 48), NF_UPPER_TIMELIMIT);
        
        if (post_time.tv_sec != 0)
        {
            udd.end = post_time.tv_sec;
            ifn_get_gmtime_text(udd.end, g_dform, g_tform, buf);
    		nfui_nflabel_set_text((NFLABEL*)g_uddobj[UDD_END], buf);
    		nfui_signal_emit((NFOBJECT*)g_uddobj[UDD_END], GDK_EXPOSE, TRUE);		
        }
	}		

	return FALSE;
}

static gboolean _post_ok_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        _get_data_from_obj();
		scm_apply_timezone(g_tz_idx, g_nf_udd);
		
        DAL_set_user_defined_dst_data(&udd);
        DAL_save_setup_db(NFSETUP_WINDOW_SYSTEM);
        
		nfui_nfobject_destroy(g_curwnd);
	}

	return FALSE;
}

static gboolean _post_cancel_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		nfui_nfobject_destroy(g_curwnd);
	}

	return FALSE;
}

static gboolean _post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
		gtk_main_quit();
	}
	return FALSE;
}

gint VW_User_Defined_Dst_Popup(NFWINDOW *parent, guint dform, guint tform, NF_TIME_UDD *nf_udd, gint tz_idx)
{
    NFOBJECT *obj;
    NFOBJECT *fixed;
    guint pos_x, pos_y;
    gint h_idx, m_idx;
    gint size_w, size_h;
    time_t time;
	GdkPixbuf *datetime_img[NFOBJECT_STATE_COUNT];
	gchar buf[64];
    gchar *strOffOn[] = {"OFF", "ON"};
    gchar *strHour[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12"};
    gchar *strRange2[] ={
                        	"00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "10",
                        	"11", "12", "13", "14", "15", "16", "17", "18", "19", "20",
                        	"21", "22", "23", "24", "25", "26", "27", "28", "29", "30",
                        	"31", "32", "33", "34", "35", "36", "37", "38", "39", "40",
                        	"41", "42", "43", "44", "45", "46", "47", "48", "49", "50",
                        	"51", "52", "53", "54", "55", "56", "57", "58", "59", 
                          };

    _load_udd_data();
    g_dform = dform;
    g_tform = tform;
    g_nf_udd = nf_udd;
    g_tz_idx = tz_idx;

	datetime_img[0] = nfui_get_image_from_file((IMG_BT_POP_DATETIME_N), NULL);
	datetime_img[1] = nfui_get_image_from_file((IMG_BT_POP_DATETIME_O), NULL);	
	datetime_img[2] = nfui_get_image_from_file((IMG_BT_POP_DATETIME_P), NULL);	
	datetime_img[3] = nfui_get_image_from_file((IMG_BT_POP_DATETIME_D), NULL);	
    nfui_get_pixbuf_size(datetime_img[0], &size_w, &size_h);
    
    obj = nftool_create_popup_window(parent, UDD_WND_POS_X, UDD_WND_POS_Y, UDD_WND_SIZE_W, UDD_WND_SIZE_H, "USER DEFINED DST", FALSE);
    nfui_regi_post_event_callback(obj, _post_main_win_event_handler);
    g_curwnd = obj;

    fixed = ((NFWINDOW*)g_curwnd)->child;

    pos_x = UDD_OBJ_START_X + 10;
    pos_y = UDD_OBJ_START_Y + 4;

//---> USER DEFINED DST

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("USER DEFINED DST", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, UDD_LB_SIZE_W, 40);
    nfui_nfobject_use_focus(obj, FALSE);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);

    pos_x += UDD_LB_SIZE_W + UDD_GAP_5;
    
    obj = nfui_spinbutton_new(strOffOn, 2, udd.use);
	nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_1);
	nfui_spin_button_set_align((NFSPINBUTTON*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, UDD_OBJ_SIZE_W, 40);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
	nfui_nfobject_show(obj);
    g_uddobj[UDD_USE] = obj;
    
//---> STD TIME ABBREVIATION

    pos_x = UDD_OBJ_START_X + 10;
    pos_y += 40 + UDD_GAP_2;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("STD TIME ABBREVIATION", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, UDD_LB_SIZE_W, 40);
    nfui_nfobject_use_focus(obj, FALSE);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);

    pos_x += UDD_LB_SIZE_W + UDD_GAP_5;

    _get_string_to_upper(udd.std_name, buf);
    obj = (NFOBJECT*)nfui_nflabel_new_text_box(buf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_support_multi_lang(obj, FALSE);
    nfui_nfobject_set_size(obj, UDD_OBJ_SIZE_W, 40);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);
    nfui_regi_post_event_callback(obj, _post_dst_label_event_handler);
    g_uddobj[UDD_STD_NAME] = obj;

//---> DST TIME ABBREVIATION

    pos_x = UDD_OBJ_START_X + 10;
    pos_y += 40 + UDD_GAP_2;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DST TIME ABBREVIATION", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, UDD_LB_SIZE_W, 40);
    nfui_nfobject_use_focus(obj, FALSE);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);

    pos_x += UDD_LB_SIZE_W + UDD_GAP_5;

    _get_string_to_upper(udd.dst_name, buf);
    obj = (NFOBJECT*)nfui_nflabel_new_text_box(buf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_support_multi_lang(obj, FALSE);
    nfui_nfobject_set_size(obj, UDD_OBJ_SIZE_W, 40);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);
    nfui_regi_post_event_callback(obj, _post_dst_label_event_handler);
    g_uddobj[UDD_DST_NAME] = obj;
    
//---> OFFSET OF DST

    pos_x = UDD_OBJ_START_X + 10 + UDD_LB_SIZE_W + UDD_GAP_5;
    pos_y += 40 + UDD_GAP_10;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("HOUR", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 138, 35);
    nfui_nfobject_use_focus(obj, FALSE);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);

    pos_x += 138 + 23;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("MIN", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 139, 35);
    nfui_nfobject_use_focus(obj, FALSE);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);

    pos_x = UDD_OBJ_START_X + 10;
    pos_y += 35 + UDD_GAP_2;
    
    _get_offset_from_sec(&h_idx, &m_idx);
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("OFFSET OF DST", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, UDD_LB_SIZE_W, 40);
    nfui_nfobject_use_focus(obj, FALSE);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);

    pos_x += UDD_LB_SIZE_W + UDD_GAP_5;

    obj = nfui_combobox_new(strHour, 13, h_idx);
    nfui_combobox_set_skin_type((NFCOMBOBOX*)obj, NFCOMBOBOX_TYPE_POPUP_1);
    nfui_combobox_set_align((NFCOMBOBOX*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 138, 40);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
	nfui_nfobject_show(obj);
    g_uddobj[UDD_OFFSET_H] = obj;

    pos_x += 138 + UDD_GAP_5;
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(":", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 13, 40);
    nfui_nfobject_use_focus(obj, FALSE);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);

    pos_x += 13 + UDD_GAP_5;
    
    obj = nfui_combobox_new(strRange2, 60, m_idx);
    nfui_combobox_set_skin_type((NFCOMBOBOX*)obj, NFCOMBOBOX_TYPE_POPUP_1);
    nfui_combobox_set_align((NFCOMBOBOX*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 139, 40);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
	nfui_nfobject_show(obj);
    g_uddobj[UDD_OFFSET_M] = obj;

//---> START OF DST

    pos_x = UDD_OBJ_START_X + 10;
    pos_y += 40 + UDD_GAP_2;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("START OF DST", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, UDD_LB_SIZE_W, 40);
    nfui_nfobject_use_focus(obj, FALSE);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);

    pos_x += UDD_LB_SIZE_W + UDD_GAP_5;

    _get_current_date_time(&time, UDD_START);
    ifn_get_gmtime_text(time, dform, tform, buf);
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(buf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(129));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(206));
    nfui_nfobject_set_size(obj, UDD_OBJ_SIZE_W, 40);
    nfui_nfobject_use_focus(obj, FALSE);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);
    g_uddobj[UDD_START] = obj;

    pos_x += UDD_OBJ_SIZE_W + UDD_GAP_2;
    
	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), datetime_img);
	nfui_nfobject_set_size(obj, size_w, size_h);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
	nfui_nfobject_show(obj);
	nfui_regi_post_event_callback(obj, _post_btn_start_dst_event_handler);

//---> END OF DST

    pos_x = UDD_OBJ_START_X + 10;
    pos_y += 40 + UDD_GAP_2;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("END OF DST", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, UDD_LB_SIZE_W, 40);
    nfui_nfobject_use_focus(obj, FALSE);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);

    pos_x += UDD_LB_SIZE_W + UDD_GAP_5;

    _get_current_date_time(&time, UDD_END);
    ifn_get_gmtime_text(time, dform, tform, buf);
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(buf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(129));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(206));
    nfui_nfobject_set_size(obj, UDD_OBJ_SIZE_W, 40);
    nfui_nfobject_use_focus(obj, FALSE);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);
    g_uddobj[UDD_END] = obj;

    pos_x += UDD_OBJ_SIZE_W + UDD_GAP_2;

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), datetime_img);
	nfui_nfobject_set_size(obj, size_w, size_h);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
	nfui_nfobject_show(obj);
	nfui_regi_post_event_callback(obj, _post_btn_end_dst_event_handler);

//---> button

    pos_x = (UDD_WND_SIZE_W / 2) - 162;
    pos_y = UDD_WND_SIZE_H - 40 - 15;
    
    obj = nftool_normal_button_create_type1("OK", 160);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);
	nfui_regi_post_event_callback(obj, _post_ok_event_cb);

    pos_x += 160 + 4;
    
    obj = nftool_normal_button_create_type1("CANCEL", 160);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);
	nfui_regi_post_event_callback(obj, _post_cancel_event_cb);

    nfui_nfobject_show(g_curwnd);
    nfui_make_key_hierarchy((NFWINDOW*)g_curwnd);
    
    gtk_main();
    
    return 0;
}

