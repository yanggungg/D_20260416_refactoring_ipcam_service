#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"
#include "support/nf_ui_common_data.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfimage.h"
#include "objects/nfcombobox.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfbutton.h"
#include "objects/nfspinbutton.h"

#include "vw_set_date_time_table.h"
#include "vw_sys_camera_ipcam_internal.h"

#include "ix_func.h"


#define SDT_WIN_SIZE_W				(485)
#define SDT_WIN_SIZE_H				(289)

#define SDT_TITLE_H					(36)

#define SDT_LEFT_GAP				(33)

#define SDT_LABEL_Y					(60)
#define SDT_LABEL_W					(63)
#define SDT_LABEL_H					(22)

#define SDT_CELL_W					(SDT_LABEL_W)
#define SDT_CELL_H					(40)

#define SDT_BTN_W					(192)	

#define SDT_OK_BTN_X				(44)	
#define SDT_OK_BTN_Y				(SDT_WIN_SIZE_H - 68)	

#define SDT_CANCEL_BTN_X			(SDT_OK_BTN_X+SDT_BTN_W+6)	
#define SDT_CANCEL_BTN_Y			(SDT_OK_BTN_Y)		


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *time_cell[4];

static gint ch_num;

static gboolean post_okbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *topwin;
	
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);	
	
	}

	return FALSE;
}

static gboolean post_window_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case GDK_EXPOSE:

		break;

		case GDK_DELETE:
			g_curwnd = 0;
			gtk_main_quit();
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

	if(evt->type == GDK_EXPOSE) {
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

gboolean _post_start_hour_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint val;
      
    if (evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        val = nfui_spin_button_get_index((NFSPINBUTTON*) obj);
        g_ipcamData[ch_num].dnn_start_hour= val; 
     
    }
    return FALSE;
}

gboolean _post_start_min_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint val;

    if (evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
           
        val = nfui_spin_button_get_index((NFSPINBUTTON*) obj);
        g_ipcamData[ch_num].dnn_start_min= val;   
       
    }

    return FALSE;
}

gboolean _post_end_hour_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint val;

    if (evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        val = nfui_spin_button_get_index((NFSPINBUTTON*) obj);
        g_ipcamData[ch_num].dnn_end_hour= val;  
      
    }
    return FALSE;
}

gboolean _post_end_min_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint val;

    if (evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {
        val = nfui_spin_button_get_index((NFSPINBUTTON*) obj);
         
        g_ipcamData[ch_num].dnn_end_min = val;  
    }
    return FALSE;
}
void VW_Set_DateTime_table_Open(NFWINDOW *parent, gchar *title, gint sdt_x, gint sdt_y, gint ch)
{
	NFOBJECT *sdt_win;
	NFOBJECT *sdt_fixed;

	NFOBJECT *obj;
    
	GdkPixbuf *up_img[NFOBJECT_STATE_COUNT];

	const gchar *sdt_str[] = {"HOUR","MIN","HOUR","MIN"};
	static const gchar *strRange1[24] ={
	"00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "10",
	"11", "12", "13", "14", "15", "16", "17", "18", "19", "20",
	"21", "22", "23", 
};
static const gchar *strRange2[60] ={
	"00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "10",
	"11", "12", "13", "14", "15", "16", "17", "18", "19", "20",
	"21", "22", "23", "24", "25", "26", "27", "28", "29", "30",
	"31", "32", "33", "34", "35", "36", "37", "38", "39", "40",
	"41", "42", "43", "44", "45", "46", "47", "48", "49", "50",

	"51", "52", "53", "54", "55", "56", "57", "58", "59", 
};

	gchar strTemp[8];

	guint pos_x, pos_y;
	guint size_w, size_h;

	guint type_gap_x = 0;

	guint i, cnt, init;
	gint start_hour,start_min, end_hour, end_min;
    ch_num = ch;

    end_hour = g_ipcamData[ch].dnn_end_hour;
    end_min = g_ipcamData[ch].dnn_end_min;
    start_hour = g_ipcamData[ch].dnn_start_hour;
    start_min = g_ipcamData[ch].dnn_start_min;
    
	if (sdt_x < 0) sdt_x = 0;
	if (sdt_y < 0) sdt_y = 0;
	//if (sdt_x + SDT_WIN_SIZE_W > 1720) sdt_x = 1720 - SDT_WIN_SIZE_W;
	//if (sdt_y + SDT_WIN_SIZE_H > 880) sdt_y = 880 - SDT_WIN_SIZE_H;

// <---- IMAGE LOAD
	up_img[0] = nfui_get_image_from_file((IMG_N_DATETIME_UP), NULL);

	nfui_get_pixbuf_size(up_img[0], &size_w, &size_h);

	memset(strTemp, 0, sizeof(strTemp));
		
	
// <---- WINDOW
	sdt_win = (NFOBJECT*)nfui_nfwindow_new(parent, sdt_x, sdt_y, SDT_WIN_SIZE_W-130, SDT_WIN_SIZE_H-70);
	g_curwnd = sdt_win;
	nfui_nfwindow_set_title(sdt_win, "SYSTEM - DATE/TIME SET POPUP");
	nfui_nfobject_modify_bg(sdt_win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_regi_post_event_callback(sdt_win, post_window_event_cb);

// <---- FIXED
	sdt_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(sdt_fixed, SDT_WIN_SIZE_W-130, SDT_WIN_SIZE_H-70);
	nfui_regi_post_event_callback(sdt_fixed, post_fixed_event_cb);
	nfui_nfobject_show(sdt_fixed);

// <---- TITLE
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(title, nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, 412, SDT_TITLE_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 90 );
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)sdt_fixed, obj, SDT_LEFT_GAP, 4);
	
	pos_x = SDT_LEFT_GAP ; // +10
	pos_y = SDT_LABEL_Y;	

	for (i = 0; i < 4; i++)
	{

// <---- LABEL
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(sdt_str[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
		nfui_nfobject_set_size(obj, SDT_LABEL_W, SDT_LABEL_H);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)sdt_fixed, obj, pos_x, pos_y);

// <---- TIME DISPLAY CELL
		pos_y += size_h + 2;

		if (i == 0)			g_sprintf(strTemp, "%02d", start_hour),cnt=24,init=start_hour;
		else if (i == 1)	g_sprintf(strTemp, "%02d", start_min),cnt=60,init=start_min;
		else if (i == 2)	g_sprintf(strTemp, "%02d", end_hour),cnt=24,init=end_hour;
	    else if (i == 3)   	g_sprintf(strTemp, "%02d", end_min),cnt=60,init=end_min;
        
        if(i==0 || i==2) time_cell[i] = (NFOBJECT*)nfui_spinbutton_new(strRange1, cnt, init);
        if(i==1 || i==3) time_cell[i] = (NFOBJECT*)nfui_spinbutton_new(strRange2, cnt, init);
        nfui_spinbutton_set_skin_type((NFSPINBUTTON*)time_cell[i], NFSPINBUTTON_TYPE_SUBTAB_1);
		nfui_nfobject_set_size(time_cell[i], SDT_CELL_W, SDT_CELL_H);
		nfui_nflabel_set_align((NFSPINBUTTON*)time_cell[i], NFALIGN_CENTER, 0);
		//nfui_nfobject_use_focus(time_cell[i], NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(time_cell[i]);
		nfui_nffixed_put((NFFIXED*)sdt_fixed, time_cell[i], pos_x, pos_y);
		if(i==0) nfui_regi_post_event_callback(time_cell[0], _post_start_hour_spin_event_handler);
		if(i==1) nfui_regi_post_event_callback(time_cell[1], _post_start_min_spin_event_handler);
		if(i==2) nfui_regi_post_event_callback(time_cell[2], _post_end_hour_spin_event_handler);
		if(i==3) nfui_regi_post_event_callback(time_cell[3], _post_end_min_spin_event_handler);

		pos_x += SDT_LABEL_W+5;
		if(i==1) pos_x += 15;

        pos_y -= size_h +2;
	}

// <---- OK, CANCEL BUTTON
	obj = nftool_normal_button_create_type1("OK", 120);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)sdt_fixed, obj, SDT_OK_BTN_X+70, pos_y+size_h+22+SDT_CELL_H);
	nfui_regi_post_event_callback(obj, post_okbutton_event_handler);

	nfui_nfwindow_add((NFWINDOW*)sdt_win, sdt_fixed);
	nfui_run_main_event_handler(sdt_win);
	nfui_nfobject_show(sdt_win);

	nfui_make_key_hierarchy(sdt_win);
	nfui_page_open(PGID_SET_DATE_TIME_POPUP, sdt_win, nfui_get_last_user());
	gtk_main();

	nfui_page_close(PGID_SET_DATE_TIME_POPUP, sdt_win);

}




