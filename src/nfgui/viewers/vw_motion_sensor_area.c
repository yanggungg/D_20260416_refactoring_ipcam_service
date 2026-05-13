#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfobject.h"
#include "objects/nftile.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"

#include "tools/nf_ui_function.h"

#include "vw_motion_sensor_area.h"
#include "vw_motion_sensor_menu.h"
#include "vw_motion_sensor_conf.h"
#include "vw_sys_camera_motion.h"

#include "vsm.h"

#define MOUSE_BUTTON_LEFT 1

static NFWINDOW *g_curwnd = 0;

static MotionData *gOrg_d;
static MotionData motdata[GUI_CHANNEL_CNT];

static guint g_ch;

static gint g_area_rows;
static gint g_area_cols;

static gint g_area_mode = MAM_NONE;


static void save_motion_area_data()
{
	g_memmove(gOrg_d, motdata, sizeof(MotionData)*GUI_CHANNEL_CNT);
}

static void set_rect_data(guint r1, guint c1, guint r2, guint c2)
{
	guint i, j;
	guint sr, sc, er, ec;
	gint idx = motdata[g_ch].rect_cnt;

	/*
	if((r2 - r1) == (g_area_rows - 1) && (c2 - c1) == (g_area_cols - 1)) {
		motdata[g_ch].rect[0].start_r = r1;
		motdata[g_ch].rect[0].start_c = c1;
		motdata[g_ch].rect[0].end_r = r2;
		motdata[g_ch].rect[0].end_c = c2;

		motdata[g_ch].rect_cnt = 1;
		return;
	}

	for(i=0; i<idx; i++) {
		sr = motdata[g_ch].rect[i].start_r;
		sc = motdata[g_ch].rect[i].start_c;
		er = motdata[g_ch].rect[i].end_r;
		ec = motdata[g_ch].rect[i].end_c;

		if(sr >= r1 && sc >= c1) { 
			if(er <= r2 && ec <= c2) {
				motdata[g_ch].rect[i].start_r = r1;
				motdata[g_ch].rect[i].start_c = c1;
				motdata[g_ch].rect[i].end_r = r2;
				motdata[g_ch].rect[i].end_c = c2;
				return;
			}
		}
	}
	*/

	motdata[g_ch].rect[idx].start_r = r1;
	motdata[g_ch].rect[idx].start_c = c1;
	motdata[g_ch].rect[idx].end_r = r2;
	motdata[g_ch].rect[idx].end_c = c2;

	motdata[g_ch].rect_cnt+=1;
}

static gboolean set_area_data(guchar val, guint r1, guint c1, guint r2, guint c2)
{
	guint i, j;
	gboolean changed = FALSE;

	for(i=r1; i<=r2; i++) {
		for(j=c1; j<=c2; j++) {
			if(motdata[g_ch].area[i * g_area_cols + j] != val) {
				motdata[g_ch].area[i * g_area_cols + j] = val;

				if(!changed)
					changed = TRUE;
			}
		}
	}

	if(g_area_mode == MAM_RECTANGLE) {
		if(changed)
			set_rect_data(r1, c1, r2, c2);
	}

	return changed;
}

static void set_area_data_selectAll()
{
	if(g_area_mode == MAM_RECTANGLE) {
		motdata[g_ch].rect[0].start_r = 0;
		motdata[g_ch].rect[0].start_c = 0;
		motdata[g_ch].rect[0].end_r = g_area_rows - 1;
		motdata[g_ch].rect[0].end_c = g_area_cols - 1;

		motdata[g_ch].rect_cnt = 1;
	}
	memset(&motdata[g_ch].area, '1', (size_t)(g_area_rows * g_area_cols));
}

static void set_area_data_deselectAll()
{
	if(g_area_mode == MAM_RECTANGLE) {
		motdata[g_ch].rect_cnt = 0;
		memset(&motdata[g_ch].rect, 0x00, sizeof(RectArea) * MAX_RECT_COUNT);
	}
	memset(&motdata[g_ch].area, '0', (size_t)(g_area_rows * g_area_cols));
}

static gboolean is_selected_all()
{
	guint i, j;

	for(i = 0 ; i < g_area_rows ; i++) {
		for(j = 0 ; j < g_area_cols ; j++) {
			if(motdata[g_ch].area[i * g_area_cols + j] == '0')
				return FALSE;
		}
	}
	return TRUE;
}

static void draw_tile(NFOBJECT *tile)
{
	guint i, j;
	gint metrix = 0;

	for(i = 0 ; i < g_area_rows ; i++)
	{
		for(j = 0 ; j < g_area_cols ; j++)
		{
			if(motdata[g_ch].area[metrix] == '1')
				nfui_tile_set_selectArea(NF_TILE(tile), i, j, i, j);
			else
				nfui_tile_conv_selectArea(NF_TILE(tile), i, j, i, j);

			metrix++;
		} 
	}	
}

static gint _change_channel(NFOBJECT *tile, gint ch)
{
    guint ch_mask = 0;
    gint rows, cols;
	guint i, j;
	gint metrix = 0;    
    
    g_ch = ch;
    ch_mask |= (1 << ch);
    
    vsm_live_preview_start(ch_mask, 0, 0, DISPLAY_ACTIVE_WIDTH, DISPLAY_ACTIVE_HEIGHT);
    scm_get_motion_size(ch, &rows, &cols);
#if defined(_IPX_MODEL_UX)
    scm_get_mdraw_method(ch, &g_area_mode);
#endif

    if ((rows == g_area_rows) && (cols == g_area_cols))
    {
        draw_tile(tile);
        return -1;
    }
    
	for(i = 0; i < g_area_rows; i++)
	{
		for(j = 0; j < g_area_cols; j++)
		{
			nfui_tile_conv_selectArea(NF_TILE(tile), i, j, i, j);
			metrix++;
		} 
	}

    g_area_rows = rows;
    g_area_cols = cols;

    nfui_tile_reset_area_size(NF_TILE(tile), g_area_rows, g_area_cols);
    draw_tile(tile);
    
    return 0;
}

static gboolean post_tile_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	if(evt->type == NFEVENT_TILE_INIT)
	{
		draw_tile(obj);

		printf("=========================================================\n");
		printf("area : %s \n", motdata[g_ch].area);
		printf("=========================================================\n");

	}
	else if(evt->type == NFEVENT_TILE_END_SELECT)
	{
		mb_type mb_ret;
		guint x1, y1, x2, y2;
		guint cur_state;
		gint idx;
		guint i, j;

		nfui_tile_get_selectArea(NF_TILE(obj), &y1, &x1, &y2, &x2);
		cur_state = nfui_tile_get_cur_state(NF_TILE(obj));

		switch(g_area_mode) 
		{ 
			case MAM_RECTANGLE: 
				{
					// check max rect count
					if(motdata[g_ch].rect_cnt >= MAX_RECT_COUNT) {
						mb_ret = nftool_mbox(g_curwnd, "NOTICE", "The previously selected area will be deselected.\nDo you want to continue?",  NFTOOL_MB_OKCANCEL);

						if(mb_ret == NFTOOL_MB_OK) {
							set_area_data_deselectAll();
						} else {					
							draw_tile(obj);
#if defined(_SUPPORT_GUI_MDRAW)
                            VW_MotionDraw_stop();
                        	VW_MotionDraw_set_selectArea(motdata[g_ch].area);
                            VW_MotionDraw_start();
#endif
							return FALSE;
						}
					}
					
					// exception: deselect all
					if((y2 - y1) == (g_area_rows - 1) && (x2 - x1) == (g_area_cols - 1)) {
						if(cur_state == NFTILE_STATE_NORMAL) {
							if(is_selected_all()) {
								set_area_data_deselectAll();

								draw_tile(obj);
								return FALSE;
							}
						}
					}
					
					// exception: first select
					if(motdata[g_ch].rect_cnt == 1) {
						if(is_selected_all())
							set_area_data_deselectAll();
					}

					set_area_data('1', y1, x1, y2, x2);

					draw_tile(obj); 						
				} 
				break;

			default:
				{
					if(cur_state == NFTILE_STATE_NORMAL) set_area_data('0', y1, x1, y2, x2);
					else 								 set_area_data('1', y1, x1, y2, x2);
				}
				break;
		}

#if defined(_SUPPORT_GUI_MDRAW)
        VW_MotionDraw_stop();
    	VW_MotionDraw_set_selectArea(motdata[g_ch].area);	
    	DAL_set_motion_data(motdata[g_ch], g_ch);
    	DAL_notify_fire_DB_sync(NF_SYSDB_TMP_CHANGE_EVENTID_MOTION, g_ch);
        VW_MotionDraw_start();
#endif			
	}
	else if(evt->type == GDK_BUTTON_PRESS 
			|| evt->type == NFEVENT_KEYPAD_PRESS 
			|| evt->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(evt->button.button == MOUSE_RIGTH_BUTTON || kpid == KEYPAD_EXIT)
		{
			NFOBJECT *top;
			gint menu_rv;
			gint x, y;

#if defined(_SUPPORT_GUI_MDRAW)
            VW_MotionDraw_stop();
#endif
			gdk_display_get_pointer(gdk_display_get_default(),
					NULL, &x, &y, NULL);
			menu_rv = VW_MotionSensorMenu_Open(g_curwnd, x, y);

			if(menu_rv < 0)
			{
#if defined(_SUPPORT_GUI_MDRAW)
                VW_MotionDraw_set_selectArea(motdata[g_ch].area);
                VW_MotionDraw_start();    
#endif		
				return TRUE;
            }
            
			switch(menu_rv) 
			{
				case SELECT_ALL:
					set_area_data_selectAll();
					nfui_tile_set_selectArea((NFTILE*)obj, 0, 0, g_area_rows - 1, g_area_cols - 1);
#if defined(_SUPPORT_GUI_MDRAW)					
                    VW_MotionDraw_set_selectArea(motdata[g_ch].area);
                	DAL_set_motion_data(motdata[g_ch], g_ch);
                	DAL_notify_fire_DB_sync(NF_SYSDB_TMP_CHANGE_EVENTID_MOTION, g_ch);
                    VW_MotionDraw_start();
#endif                    
					break;

				case DESELECT_ALL:
					set_area_data_deselectAll(); 
					nfui_tile_conv_selectArea((NFTILE*)obj, 0, 0, g_area_rows - 1, g_area_cols - 1);
#if defined(_SUPPORT_GUI_MDRAW)										
                    VW_MotionDraw_set_selectArea(motdata[g_ch].area);
                	DAL_set_motion_data(motdata[g_ch], g_ch);
                	DAL_notify_fire_DB_sync(NF_SYSDB_TMP_CHANGE_EVENTID_MOTION, g_ch);
                    VW_MotionDraw_start();
#endif                    
					break;

				case OPEN_CONF:
					VW_MotSen_Conf_Open(g_curwnd, g_ch, motdata);
#if defined(_SUPPORT_GUI_MDRAW)
                    VW_MotionDraw_set_selectArea(motdata[g_ch].area);
                    VW_MotionDraw_set_daytime(motdata[g_ch].time_start, motdata[g_ch].time_end);
                    VW_MotionDraw_set_sense(motdata[g_ch].sense_d, motdata[g_ch].sense_n);
                	DAL_set_motion_data(motdata[g_ch], g_ch);
                	DAL_notify_fire_DB_sync(NF_SYSDB_TMP_CHANGE_EVENTID_MOTION, g_ch);
                    VW_MotionDraw_start();
#endif                    
					break;

				case SAVE:
					save_motion_area_data();
					break;

				case SAVE_N_EXIT:
					{
						save_motion_area_data();		
						top = nfui_nfobject_get_top(obj);
						nfui_nfobject_destroy(top);
					}
					break;

				case CANCEL:
					{
#if defined(_SUPPORT_GUI_MDRAW)
                		guint i;
                        MotionData tmp_motdata[GUI_CHANNEL_CNT];
                		
                    	for(i=0; i<GUI_CHANNEL_CNT; i++)
                    		DAL_get_motionsensor_data(&tmp_motdata[i], i);

                    	if (memcmp(tmp_motdata, gOrg_d, sizeof(MotionData)*GUI_CHANNEL_CNT))
                    	{
                            DAL_set_motion_data_all(gOrg_d, GUI_CHANNEL_CNT);

                            for (i = 0; i < GUI_CHANNEL_CNT; i++)
                                DAL_notify_fire_DB_sync(NF_SYSDB_TMP_CHANGE_EVENTID_MOTION, i);            
                        }
#endif					
						top = nfui_nfobject_get_top(obj);
						nfui_nfobject_destroy(top);
					}
					break;

				default:
					{
						if(menu_rv < 16) 
						{
                            _change_channel(obj, menu_rv);
#if defined(_SUPPORT_GUI_MDRAW)
                        	VW_MotionDraw_set_obj(g_ch, obj);
                        	VW_MotionDraw_set_selectArea(motdata[g_ch].area);
                            VW_MotionDraw_set_daytime(motdata[g_ch].time_start, motdata[g_ch].time_end);
                            VW_MotionDraw_set_sense(motdata[g_ch].sense_d, motdata[g_ch].sense_n);
                            VW_MotionDraw_start();    
#endif
						}
					}
					break;
			}
			return TRUE;
		}
	}

	return FALSE;

}

static gboolean post_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	switch(evt->type) 
	{
		default : 
		break;
	}
	
	return FALSE;

}

static gboolean post_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
#if defined(_SUPPORT_GUI_MDRAW)
        VW_MotionDraw_finalize();
#endif		
		g_curwnd = 0;
		gtk_main_quit();
	}

	return FALSE;
}

void VW_MotionSensorArea_Open(NFWINDOW *parent, gint ch, MotionData *data)
{
	NFOBJECT *main_wnd = NULL;
	NFOBJECT *main_fixed = NULL;
    NFOBJECT *obj;
	
#if defined(_HDI_MODEL_UX) || defined(_DVR_MODEL_UX)
	CONNENT_CAM_E connect_cam;	
#endif
	GdkColor select_color[4] = {UX_COLOR(734),	
								UX_COLOR(734),	
								UX_COLOR(734),	
								UX_COLOR(734)};

	g_ch = (guint)ch;
	gOrg_d = data;

	g_memmove(motdata, data, sizeof(MotionData)*GUI_CHANNEL_CNT);

    scm_get_motion_size(ch, &g_area_rows, &g_area_cols);
#if defined(_IPX_MODEL_UX)
    scm_get_mdraw_method(ch, &g_area_mode);
#endif

	main_wnd = nfui_nfwindow_new(parent, 0, 0, DISPLAY_ACTIVE_WIDTH, DISPLAY_ACTIVE_HEIGHT);
	nfui_nfobject_modify_bg(main_wnd, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
	gtk_widget_set_app_paintable(((NFWINDOW*)main_wnd)->main_widget, TRUE);
	nfui_regi_post_event_callback(main_wnd, post_win_event_handler);
	nfui_run_main_event_handler(main_wnd);
	g_curwnd = main_wnd;
	
	main_fixed = nfui_nffixed_new();
	nfui_nfobject_modify_bg(main_fixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
	nfui_regi_post_event_callback(main_fixed, post_main_fixed_event_handler);

	obj = nfui_tile_new(g_area_rows, g_area_cols);
	nfui_tile_set_fill(NF_TILE(obj), FALSE);
	nfui_tile_set_line_color(NF_TILE(obj), NFTILE_STATE_NORMAL, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_000000))); 
	nfui_tile_set_line_color(NF_TILE(obj), NFTILE_STATE_FOCUS, &UX_COLOR(733)); 
	nfui_tile_set_line_color(NF_TILE(obj), NFTILE_STATE_SELECT, select_color);
	nfui_tile_set_drawable_outline(NF_TILE(obj), FALSE);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
	nfui_nfobject_set_size(obj, DISPLAY_ACTIVE_WIDTH, DISPLAY_ACTIVE_HEIGHT);
	nfui_regi_post_event_callback(obj, post_tile_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 0, 0);

	nfui_nfwindow_add((NFWINDOW*)main_wnd, main_fixed);
	nfui_nfobject_show(main_fixed);
	nfui_nfobject_show(main_wnd);

	nfui_make_key_hierarchy((NFWINDOW*)main_wnd);
	nfui_set_key_focus(obj, TRUE);

#if defined(_SUPPORT_GUI_MDRAW)
	VW_MotionDraw_set_obj(g_ch, obj);
	VW_MotionDraw_set_plt_position(0, 0, 1920, 1080);	
    VW_MotionDraw_set_selectArea(motdata[g_ch].area);
    VW_MotionDraw_set_daytime(motdata[g_ch].time_start, motdata[g_ch].time_end);
    VW_MotionDraw_set_sense(motdata[g_ch].sense_d, motdata[g_ch].sense_n);    
    VW_MotionDraw_start();
#endif

	nfui_page_open(PGID_MOTION_AREA, main_wnd, nfui_get_last_user());

	gtk_main();

	nfui_page_close(PGID_MOTION_AREA, main_wnd);
}

void VW_MotionSensorArea_Destroy(NFOBJECT *obj)
{
	NFOBJECT *top = NULL;

#if defined(_SUPPORT_GUI_MDRAW)
	VW_MotionDraw_finalize();
#endif			

	top = nfui_nfobject_get_top(obj);
	if(top)
		nfui_nfobject_destroy(top);
}


