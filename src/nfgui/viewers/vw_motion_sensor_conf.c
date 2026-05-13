
#include "nf_afx.h"


#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nfspinbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nflabel.h"
#include "objects/nftimespin.h"

#include "vw_sys_camera_motion.h"
#include "ssm.h"

#ifdef _IPX_MODEL_UX
#include "nf_api_ipcam.h"
#endif


#define MSC_POS_X									((DISPLAY_ACTIVE_WIDTH - MSC_SIZE_W)/2)
#define MSC_POS_Y									((DISPLAY_ACTIVE_HEIGHT - MSC_SIZE_H)/2)
#define MSC_SIZE_W									(629)
#define MSC_SIZE_H									(367)


static MotionData *parent_motdata;
static int current_channel_number;

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *combo_obj;
static NFOBJECT *sens_d;
static NFOBJECT *sens_n;
static NFOBJECT *mini_d;
static NFOBJECT *mini_n;
static NFOBJECT *time_start;
static NFOBJECT *time_end;


static void set_data_to_obj(gint ch)
{
	nfui_spin_button_set_index((NFSPINBUTTON*)time_start, parent_motdata[ch].time_start);
	nfui_spin_button_set_index((NFSPINBUTTON*)time_end, parent_motdata[ch].time_end);

	nfui_spin_button_set_index((NFSPINBUTTON*)sens_d, parent_motdata[ch].sense_d - 1);
	nfui_spin_button_set_index((NFSPINBUTTON*)sens_n, parent_motdata[ch].sense_n - 1);

	nfui_spin_button_set_index((NFSPINBUTTON*)mini_d, parent_motdata[ch].mini_d - 1);
	nfui_spin_button_set_index((NFSPINBUTTON*)mini_n, parent_motdata[ch].mini_n - 1);
}

static void get_data_from_obj(gint ch)
{
	parent_motdata[ch].sense_d 		= nfui_spin_button_get_index(sens_d) + 1;
	parent_motdata[ch].sense_n		= nfui_spin_button_get_index(sens_n) + 1;
	parent_motdata[ch].mini_d 		= nfui_spin_button_get_index(mini_d) + 1;
	parent_motdata[ch].mini_n 		= nfui_spin_button_get_index(mini_n) + 1;
	parent_motdata[ch].time_start 	= nfui_spin_button_get_index(time_start);
	parent_motdata[ch].time_end 	= nfui_spin_button_get_index(time_end);

	printf("current ch : %d, %d, %d, %d, %d / %d - %d \n",
			ch,
			parent_motdata[ch].sense_d,
			parent_motdata[ch].sense_n,
			parent_motdata[ch].mini_d,
			parent_motdata[ch].mini_n,
			parent_motdata[ch].time_start,
			parent_motdata[ch].time_end);
}

static void redisp_data(gint ch)
{
	set_data_to_obj(ch);

	nfui_signal_emit(time_start, GDK_EXPOSE, TRUE);
	nfui_signal_emit(time_end, GDK_EXPOSE, TRUE);

	nfui_signal_emit(sens_d, GDK_EXPOSE, TRUE);
	nfui_signal_emit(sens_n, GDK_EXPOSE, TRUE);

	nfui_signal_emit(mini_d, GDK_EXPOSE, TRUE);
	nfui_signal_emit(mini_n, GDK_EXPOSE, TRUE);
}

static gboolean post_ch_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint idx;
#ifdef _IPX_MODEL_UX
	NFIPCamMotionProfile mot_profile;
	gint ret;
#endif

	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {
		idx = nfui_combobox_get_cur_index(obj);

		if(idx != -1) {
#ifdef _IPX_MODEL_UX
		    nfui_nfobject_enable(mini_d);
		    nfui_nfobject_enable(mini_n);

			ret = scm_get_ipcam_motion_profile(idx, &mot_profile);

			if (ret == 0)
			{
				if (mot_profile.min_block == 0) 
				{
				    nfui_nfobject_disable(mini_d);
				    nfui_nfobject_disable(mini_n);				    
                }
			}

			nfui_signal_emit(mini_d, GDK_EXPOSE, TRUE);
			nfui_signal_emit(mini_n, GDK_EXPOSE, TRUE);			
#endif

			redisp_data(idx);
		}
	}

	return FALSE;
}

static gboolean post_time_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_SPINBUTTON_CHANGED) {
		gint st_i, et_i;
		
		st_i = nfui_spin_button_get_index((NFSPINBUTTON*)time_start);
		et_i = nfui_spin_button_get_index((NFSPINBUTTON*)time_end);

		if(st_i > et_i) {
			if(time_start == obj) 
				nfui_spin_button_set_index((NFSPINBUTTON*)time_end, (guint)st_i);
			else if(time_end == obj) 
				nfui_spin_button_set_index((NFSPINBUTTON*)time_start, (guint)et_i);
		}
	}

	return FALSE;
}

static gboolean post_ok_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *topwin;
		gint sel_ch;
		
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

		sel_ch = nfui_combobox_get_cur_index(combo_obj);
		get_data_from_obj(sel_ch);
				
		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);
	}

	return FALSE;
}

static gboolean post_cancel_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) 
	{
		NFOBJECT *topwin;

		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);
	}

	return FALSE;
}

static gboolean post_msc_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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
	else if(evt->type == GDK_DELETE) 
	{
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
	
		gtk_main_quit();
	}

	return FALSE;
	
}

static gboolean post_msc_win_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE) 
	{
	    g_curwnd = 0;
		gtk_main_quit();
	}

	return FALSE;
	
}

void VW_MotSen_Conf_Open (NFWINDOW *parent, gint current_ch, MotionData *motdata)
{
	NFOBJECT *msc_win = NULL;
	NFOBJECT *msc_fixed = NULL;
	NFOBJECT *obj = NULL;

	const gchar *strTitle[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10"};
	gint i;

#ifdef _IPX_MODEL_UX
	NFIPCamMotionProfile mot_profile;
	gint ret;
#endif

	current_channel_number = current_ch;
	parent_motdata = motdata;

#ifdef _IPX_MODEL_UX
	ret = scm_get_ipcam_motion_profile(current_ch, &mot_profile);
#endif

	/* window */
	msc_win = (NFOBJECT*)nfui_nfwindow_new(parent, MSC_POS_X, MSC_POS_Y, MSC_SIZE_W, MSC_SIZE_H);
	nfui_nfobject_modify_bg(msc_win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_regi_post_event_callback(msc_fixed, post_msc_win_event_cb);
	g_curwnd = msc_win;

	/* fixed */
	msc_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(msc_fixed, MSC_SIZE_W, MSC_SIZE_H);
	nfui_regi_post_event_callback(msc_fixed, post_msc_fixed_event_cb);
	nfui_nfobject_show(msc_fixed);

	/* title */
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SENSITIVITY & MINIMUM BLOCKS", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, 590, 36);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)msc_fixed, obj, 4, 4);
	
	const gchar *rowTitle[] = {"CHANNEL", "DAYTIME SET"};
	
	for(i=0; i<2; i++) 
	{
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(rowTitle[i], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
		nfui_nfobject_set_size(obj, 152, 40);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nffixed_put((NFFIXED*)msc_fixed, obj, 20, 52 + (i * 47));
	}

	gint xpos = 224;
	gint ypos = 157;
	gint width[2] = {170, 200};
	const gchar *rowTitle2[] = {"SENSITIVITY", "MINIMUM BLOCKS"};
	
	for(i=0; i<2; i++)
	{
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(rowTitle2[i], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
		nfui_nfobject_set_size(obj, width[i], 40);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nffixed_put((NFFIXED*)msc_fixed, obj, xpos, ypos);

		xpos += 170;
	}

	const gchar *rowTitle3[] = {"DAYTIME", "NIGHTTIME"};
	
	for(i=0; i<2; i++) 
	{
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(rowTitle3[i], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
		nfui_nfobject_set_size(obj, 152, 40);
		
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		nfui_nffixed_put((NFFIXED*)msc_fixed, obj, 20, 200 + (i * 45));
	}

	/* combo */
	obj = nfui_combobox_new(NULL, 0, 0);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
	nfui_nfobject_set_size(obj, 320, 40);
	nfui_regi_post_event_callback(obj, post_ch_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)msc_fixed, obj, 224, 52);

	gchar strBuf_num[40];
	for (i = 0; i < GUI_CHANNEL_CNT; i++) 
	{
		g_sprintf(strBuf_num, "CH %d", i+1);
		nfui_combobox_append_data(obj, strBuf_num);
	}
	nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, current_channel_number);
	combo_obj = obj;

	const gchar *strtimeTitle[] = { "00:00", "01:00", "02:00", "03:00", "04:00",
									"05:00", "06:00", "07:00", "08:00", "09:00",
									"10:00", "11:00", "12:00", "13:00", "14:00",
									"15:00", "16:00", "17:00", "18:00", "19:00",
									"20:00", "21:00", "22:00", "23:00" };
		
	// DAYTIME SPIN BOX
	xpos = 224;

	for( i = 0 ; i < 2 ; i++ )
	{
		if(i == 0)
		{
			obj = (NFOBJECT*)nfui_spinbutton_new((gchar**)strtimeTitle, 24, 
									(gint)(parent_motdata[current_channel_number].time_start));
			time_start = obj;
		}
		else
		{
			obj = (NFOBJECT*)nfui_spinbutton_new((gchar**)strtimeTitle, 24,
									(gint)(parent_motdata[current_channel_number].time_end));
			time_end = obj;
		}	

		nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_POPUP_1);
		nfui_spin_button_set_align((NFSPINBUTTON*)obj, NFALIGN_LEFT, 2);
		nfui_spin_button_set_spacing((NFSPINBUTTON*)obj, CONDENSED_SPACING);	
		nfui_nfobject_set_size(obj, 150, 40);
		nfui_regi_post_event_callback(obj, post_time_event_cb);
		nfui_nfobject_show(obj);

		nfui_nffixed_put((NFFIXED*)msc_fixed, obj, xpos, 98);

		xpos += 169;	
		
	}

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("~", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(obj, 20, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)msc_fixed, obj, 370, 98);

	//printf("day time : %d - %d \n", parent_motdata[current_channel_number].time_start, 
	//		   					 	parent_motdata[current_channel_number].time_end);
	
// SPIN BOX
	for(i=0; i<2; i++)
	{
		if(i == 0)
		{
			obj = (NFOBJECT*)nfui_spinbutton_new((gchar**)strTitle, 10, 
							(gint)(parent_motdata[current_channel_number].sense_d -1));
			sens_d = obj;
		}
		else
		{
			obj = (NFOBJECT*)nfui_spinbutton_new((gchar**)strTitle, 10, 
							(gint)(parent_motdata[current_channel_number].sense_n -1));
			sens_n = obj;
		}
		
		nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_POPUP_1);		
		nfui_spin_button_set_align((NFSPINBUTTON*)obj, NFALIGN_LEFT, 2);
		nfui_spin_button_set_spacing((NFSPINBUTTON*)obj, CONDENSED_SPACING);	
		nfui_nfobject_set_size(obj, 150, 40);
		nfui_nfobject_show(obj);

		nfui_nffixed_put((NFFIXED*)msc_fixed, obj, 224, 200 + (i * 45));
	}

	for(i=0; i<2; i++)
	{
		if(i == 0)
		{
			obj = (NFOBJECT*)nfui_spinbutton_new((gchar**)strTitle, 10, 
							(gint)(parent_motdata[current_channel_number].mini_d - 1));
			mini_d = obj;
		}
		else
		{
			obj = (NFOBJECT*)nfui_spinbutton_new((gchar**)strTitle, 10, 
							(gint)(parent_motdata[current_channel_number].mini_n - 1));
			mini_n = obj;
		}
		
		nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_POPUP_1);
		nfui_spin_button_set_align((NFSPINBUTTON*)obj, NFALIGN_LEFT, 2);
		nfui_spin_button_set_spacing((NFSPINBUTTON*)obj, CONDENSED_SPACING);	
		nfui_nfobject_set_size(obj, 150, 40);
#ifdef _IPX_MODEL_UX
		if (ret == 0)
		{
			if (mot_profile.min_block == 0) nfui_nfobject_disable(obj);
		}
#endif

		nfui_nfobject_show(obj);

		nfui_nffixed_put((NFFIXED*)msc_fixed, obj, 393, 200 + (i * 45));
	}

	/* button */
	obj = nftool_normal_button_create_type1("OK", 192);
	nfui_regi_post_event_callback(obj, post_ok_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)msc_fixed, obj, 121, 300);

	obj = nftool_normal_button_create_type1("CANCEL", 192);
	nfui_regi_post_event_callback(obj, post_cancel_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)msc_fixed, obj, 319, 300);

	nfui_nfwindow_add((NFWINDOW*)msc_win, msc_fixed);
	nfui_run_main_event_handler(msc_win);
	nfui_nfobject_show(msc_win);
	nfui_make_key_hierarchy((NFWINDOW*)msc_win);
	
	nfui_page_open(PGID_MOTION_CONF, msc_win, ssm_get_cur_id(NULL));

	gtk_main();
	
	nfui_page_close(PGID_MOTION_CONF, msc_win);
}












