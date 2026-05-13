#include "nf_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"
#include "support/multi_language_support.h"

#include "tools/nf_ui_tool.h"
#include "ix_mem.h"

#include "scm.h"
#include "vsm.h"
#include "uxm.h"

#include "objects/nffixed.h"
#include "objects/nflabel.h"
#include "objects/nfbutton.h"
#include "objects/nfspinbutton.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nftile.h"
#include "objects/cw_slider.h"
#include "objects/nftab.h"

#include "vw_vkeyboard.h"
#include "vw_sys_camera_main.h"
#include "vw_sys_camera_motion_setup.h"
#include "vw_motion_sensor_setup.h"
#include "vw_motion_draw.h"

#define MOTION_SETUP_FIXED_W		(476)
#define MOTION_SETUP_FIXED_H		(220)
#define MOTION_SMART_FIXED_H		(260)
//#define MOTION_SETUP_FIXED_H		(600) // using later

#define MOTION_AREA_SIZE_W				(338)			// for corridor mode
#define MOTION_AREA_BACK_SIZE_W			(1000)
#define MOTION_AREA_BACK_SIZE_H			(600)

#define PRINT_MSG 0

#define STR_MSG1 "This camera does not detect motion\nin the motion sensor area."

 
enum {
	SET_IDX_SENSE = 0,
	SET_IDX_INTERVAL,
	SET_IDX_THRESHOLD
};

enum {
	DAYTIME_SET_PAGE = 0,
	SENSITIVITY_PAGE,
	MINI_BLOCK_PAGE,
//    INTERVAL_PAGE,
	SUBPAGE_CNT
};

enum {
	BTNS_SELECT_ALL = 0,
	BTNS_DESELECT_ALL,

	BTNS_CNT
};

enum {
	MOTION_OBJ_PERSON = 0,
	MOTION_OBJ_CAR,
	MOTION_OBJ_BIKE,
	MOTION_OBJ_CNT
};

typedef struct _IPCamMotionSetup_t	{
	gint ch;				// current ch
	gint preCh;				// only used to show preview
	guint ch_mask;			// loaded data ch

	gint area_rows;
	gint area_cols;
	gint area_method;

	gint max_rect_cnt;
}IPCamMotionSetup_i;


static OnvifMotionData org_omd[GUI_CHANNEL_CNT];		// onvif motion data
static OnvifMotionData omd[GUI_CHANNEL_CNT];

static IPCamMotionSetup_i g_msi;						// motion setup info

static NFIPCamMotionProfile g_mot_prof;					// ipcam motion profile

static gint ai_disable[GUI_CHANNEL_CNT] = {0, };

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_video_back_fixed = NULL;
static NFOBJECT *g_area;
static NFOBJECT *g_tab;
static NFOBJECT *g_tabPage[SUBPAGE_CNT];
static NFOBJECT *g_timeObj[2];							// spinbutton obj
static NFOBJECT *g_sensObj[2];							// spinbutton obj
static NFOBJECT *g_minsObj[2];							// spinbutton obj
static NFOBJECT *g_intvObj;								// spinbutton obj
static NFOBJECT *g_btns[BTNS_CNT];

static NFOBJECT *g_setup_fixed;
static NFOBJECT *g_smart_fixed;
static NFOBJECT *g_tab_fixed;

static NFOBJECT *g_classic_motion_obj;
static NFOBJECT *g_smart_motion_obj;
static NFOBJECT *g_smart_motion_chk[15];
static NFOBJECT *g_smart_motion_label[15];
static NFOBJECT *g_smart_motion_spin[15];
static NFOBJECT *g_ai_alarmevt_obj;

static gint g_mot_area_w = 0;
static gint g_corridor_mode[GUI_CHANNEL_CNT] = {0, };


//static NFOBJECT *g_warn_msg[2];

static void draw_tile(NFOBJECT *tile);
static void reset_data(gint ch);
static void update_all_data(gint ch);
static void enable_mini_tab();
static void disable_mini_tab();


static void printf_all_data(gint ch)
{
#if PRINT_MSG
	g_message("%s :::::::::channel: %d::::::::::::::::", __FUNCTION__, ch);

	g_printf("ch: %d\npreCh: %d\nch_mask: %d\narea_rows: %d\narea_cols: %d\narea_method: %d\nmax_cnt: %d\n",
	g_msi.ch, g_msi.preCh, g_msi.ch_mask, g_msi.area_rows, g_msi.area_cols, g_msi.area_method, g_msi.max_rect_cnt);
	
	g_printf("\norg_omd[%d].rect_cnt: %d\nomd[%d].rect_cnt: %d\n",
	ch, org_omd[ch].rect_cnt, ch, omd[ch].rect_cnt);

	g_printf("\nsens_d: %d\nsens_n: %d\nmini_d: %d\nmini_n: %d\ntime_s: %d\ntime_e: %d\ninterval: %d\n",
	omd[ch].cell_d.sense_d, omd[ch].cell_d.sense_n, omd[ch].cell_d.mini_d, omd[ch].cell_d.mini_n, 
	omd[ch].cell_d.time_start, omd[ch].cell_d.time_end, omd[ch].cell_d.interval);

	g_printf("\nmethod: %d\nrow: %d\ncol: %d\nsense_min: %d\nsense_max: %d\nobj:%s\n",
	omd[ch].ipcam_d.method, omd[ch].ipcam_d.row, omd[ch].ipcam_d.col, 
	omd[ch].ipcam_d.sense_min, omd[ch].ipcam_d.sense_max, omd[ch].ipcam_d.smart_interest_obj);

	g_printf("\norg!!!!!!!!!!!!!!!\nsens_d: %d\nsens_n: %d\nmini_d: %d\nmini_n: %d\ntime_s: %d\ntime_e: %d\ninterval: %d\n",
	org_omd[ch].cell_d.sense_d, org_omd[ch].cell_d.sense_n, org_omd[ch].cell_d.mini_d, org_omd[ch].cell_d.mini_n, 
	org_omd[ch].cell_d.time_start, org_omd[ch].cell_d.time_end, org_omd[ch].cell_d.interval);

	g_printf("\nmethod: %d\nrow: %d\ncol: %d\nsense_min: %d\nsense_max: %d\nobj:%s\n",
	org_omd[ch].ipcam_d.method, org_omd[ch].ipcam_d.row, org_omd[ch].ipcam_d.col, 
	org_omd[ch].ipcam_d.sense_min, org_omd[ch].ipcam_d.sense_max, org_omd[ch].ipcam_d.smart_interest_obj);

	g_message(":::::::::::::::::::::::::::::::::::", __FUNCTION__);
#endif
}

static void _trans_db_to_val(gint ch)
{
	guchar db_area[1400] = {0,};
	gint height, width;
	gint block_height, block_width;
	gint width_index, height_index;
	gint i = 0, j = 0;

	g_memmove(db_area, omd[ch].area, sizeof(guchar) * 1400);
	scm_get_motion_size(ch, &block_height, &block_width);
	
	height = block_width;
	width = block_height;

	if (g_corridor_mode[ch] == 1)
	{
		for (width_index = 0; width_index < width; width_index++)
		{
			for (height_index = height - 1; height_index >= 0; height_index--)
			{
				omd[ch].area[i] = db_area[width * height_index + width_index];
				i++;
			}
		}
	}
	else if (g_corridor_mode[ch] == 2)
	{
		for (width_index = width - 1; width_index >= 0; width_index--)
		{
			for (height_index = 0; height_index < height; height_index++)
			{
				omd[ch].area[i] = db_area[width * height_index + width_index];
				i++;
			}
		}
	}
}

static void _trans_val_to_db(gint ch, OnvifMotionData *src, OnvifMotionData *dest)
{
	gint i = 0;
	gint height, width;
	gint block_height, block_width;
	gint w_idx, h_idx;

	scm_get_motion_size(ch, &height, &width);

	if (g_corridor_mode[ch] == 1)
	{
		for (w_idx = 0; w_idx < width; w_idx++)
		{
			for (h_idx = 1; h_idx <= height; h_idx++)
			{
				dest->area[i] = src->area[width * h_idx - w_idx - 1];
				i++;
			}
		}
	}
	else if (g_corridor_mode[ch] == 2)
	{
		for (w_idx = 0; w_idx < width; w_idx++)
		{
			for (h_idx = (height - 1); h_idx >= 0; h_idx--)
			{
				dest->area[i] = src->area[width * h_idx + w_idx];
				i++;
			}
		}
	}
}

static void _set_mot_area_width(gint ch)
{
	gint ratio_w, ratio_h;
	gint tmp = 0;

	scm_ipcam_get_main_stream_ratio(ch, &ratio_w, &ratio_h);
	g_message("[%s, %d], ratio_w : %d, ratio_h : %d", __FUNCTION__, __LINE__, ratio_w, ratio_h);
	
	if (scm_get_ipcam_corridor_mode(ch) != 0)
	{
		tmp = ratio_w;
		ratio_w = ratio_h;
		ratio_h = tmp;
		
		g_mot_area_w = MOTION_AREA_BACK_SIZE_H * ratio_w / ratio_h;
	}
	else
	{
		g_mot_area_w = MOTION_AREA_BACK_SIZE_W;
	}
	// g_message("[%s, %d], g_mot_area_w : %d", __FUNCTION__, __LINE__, g_mot_area_w);
}

static void init_motion_setup_info()
{
	g_msi.ch = 0;
	g_msi.preCh = -1;
	g_msi.ch_mask = 0;

	g_msi.area_rows = 0;
	g_msi.area_cols = 0;
	g_msi.area_method = MAM_NONE;
	g_msi.max_rect_cnt = 0;
}

static void update_motion_setup_info(gint ch)
{
	g_msi.ch = ch;
	g_msi.area_rows = g_mot_prof.block_height;
	g_msi.area_cols = g_mot_prof.block_width;
	g_msi.area_method = g_mot_prof.area_method;
	g_msi.max_rect_cnt = g_mot_prof.rect_num;
}

static void load_onvif_motion_data(gint ch)
{
	DAL_get_onvif_motion_data(&omd[ch], (guint)ch, -1);
	
	if (g_corridor_mode[ch] != 0)
	{
		_trans_db_to_val(ch);
	}
	g_memmove(&org_omd[ch], &omd[ch], sizeof(OnvifMotionData));

	g_msi.ch_mask |= (1 << ch);
}


static void _print_onvif_motion_data(gint ch, OnvifMotionIPCam *org, OnvifMotionIPCam *new)
{
	g_message("ch : %d", ch);
	g_message("org->method : %d, new->method : %d, memcmp(method) : %d", org->method, new->method, memcmp(&org->method, &new->method, sizeof(guint)));
	g_message("org->row : %d, new->row : %d, memcmp(row) : %d", org->row, new->row, memcmp(&org->row, &new->row, sizeof(guint)));
	g_message("org->col : %d, new->col : %d, memcmp(col) : %d", org->col, new->col, memcmp(&org->col, &new->col, sizeof(guint)));
	g_message("org->sense_min : %d, new->sense_min : %d, memcmp(sense_min) : %d", org->sense_min, new->sense_min, memcmp(&org->sense_min, &new->sense_min, sizeof(guint)));
	g_message("org->sense_max : %d, new->sense_max : %d memcmp(sense_max) : %d", org->sense_max, new->sense_max, memcmp(&org->sense_max, &new->sense_max, sizeof(guint)));
	g_message("org->smart_motion_option_size : %d, new->smart_motion_option_size : %d memcmp(option_size) : %d", org->smart_motion_option_size, new->smart_motion_option_size, memcmp(&org->smart_motion_option_size, &new->smart_motion_option_size, sizeof(guint)));
	g_message("org->smart_interest_obj : %s, new->smart_interest_obj : %s, memcmp(obj) : %d", org->smart_interest_obj, new->smart_interest_obj, memcmp(org->smart_interest_obj, new->smart_interest_obj, 256));

	int i = 0;
	int j = 0;
	for (i = 0; i < 15; i++)
	{
		g_message("org->smart_motion_options[%d].name : %s, new->smart_motion_options[%d].name : %s, memcmp(name) : %d", i, org->smart_motion_options[i].name, i, new->smart_motion_options[i].name, memcmp(org->smart_motion_options[i].name, new->smart_motion_options[i].name, 50));
		printf("org->smart_motion_options[%d].name : ", i);
		for (j = 0; j < 50; j++)
		{
			printf("%c", org->smart_motion_options[i].name[j]);
		}
		printf("\n");

		printf("new->smart_motion_options[%d].name : ", i);
		for (j = 0; j < 50; j++)
		{
			printf("%c", new->smart_motion_options[i].name[j]);
		}
		printf("\n");
		g_message("org->smart_motion_options[%d].enable : %d, new->smart_motion_options[%d].enable : %d, memcmp(enable) : %d", i, org->smart_motion_options[i].enable, i, new->smart_motion_options[i].enable, memcmp(&org->smart_motion_options[i].enable, &new->smart_motion_options[i].enable, sizeof(guint)));
		g_message("org->smart_motion_options[%d].threshold : %d, new->smart_motion_options[%d].threshold : %d, memcmp(threshold) : %d", i, org->smart_motion_options[i].threshold, i, new->smart_motion_options[i].threshold, memcmp(&org->smart_motion_options[i].threshold, &new->smart_motion_options[i].threshold, sizeof(guint)));
	}

	g_message("memcmp(org, new, sizeof(OnvifMotionIPCam)) : %d", memcmp(org, new, sizeof(OnvifMotionIPCam)));
}

static gboolean update_profile_data(gint ch)
{
	gboolean ret = FALSE;
	OnvifMotionData db_data;
	OnvifMotionIPCam ipcam_d;
	int i;

	memset(&g_mot_prof, 0x00, sizeof(NFIPCamMotionProfile));

	if (scm_get_ipcam_motion_profile(ch, &g_mot_prof) == 0)
	{
		if (g_mot_prof.area_method == MAM_NONE)
		{
			memset(&g_mot_prof, 0x00, sizeof(NFIPCamMotionProfile));
			return ret;
		}
		memset(&ipcam_d, 0x00, sizeof(OnvifMotionIPCam));
		DAL_get_onvif_ipcam_data(&ipcam_d, (guint)ch);

		// g_message("%s(%d) => block_height:%d block_width:%d", __FUNCTION__, __LINE__, g_mot_prof.block_height, g_mot_prof.block_width);
		omd[ch].ipcam_d.method = (guint)g_mot_prof.area_method;
		omd[ch].ipcam_d.row = (guint)g_mot_prof.block_height;
		omd[ch].ipcam_d.col = (guint)g_mot_prof.block_width;
		omd[ch].ipcam_d.sense_min = (guint)g_mot_prof.sensitivity_min;
		omd[ch].ipcam_d.sense_max = (guint)g_mot_prof.sensitivity_max;
		omd[ch].ipcam_d.smart_motion_option_size = (guint)g_mot_prof.smart_motion_option_size;
		strncpy(omd[ch].ipcam_d.smart_interest_obj, g_mot_prof.db_str, 256);
		g_memmove(omd[ch].ipcam_d.smart_motion_options, g_mot_prof.smart_motion_options, sizeof(SysCamSmartMotion_t) * 15);

		g_memmove(&org_omd[ch], &omd[ch], sizeof(OnvifMotionData));

		// _print_onvif_motion_data(ch, &ipcam_d, &omd[ch].ipcam_d);
		if (memcmp(&ipcam_d, &omd[ch].ipcam_d, sizeof(OnvifMotionIPCam)))
		{
			DAL_set_onvif_ipcam_data(omd[ch].ipcam_d, (guint)ch);

			reset_data(ch);
			memset(&db_data, 0x00, sizeof(db_data));
			g_memmove(&db_data, &omd[ch], sizeof(OnvifMotionData));

			_trans_val_to_db(ch, &omd[ch], &db_data);

			DAL_set_onvif_motion_data(db_data, ch);

			sysact_set_changeflag(1);
		}
		ret = TRUE;
	}

	return ret;
}

static void update_corridor_mode(gint ch)
{
	if (!scm_get_ipcam_corridor_supp(ch)) 
	{
		g_corridor_mode[ch] = 0;
		return;
	}
	
	g_corridor_mode[ch] = scm_get_ipcam_corridor_mode(ch);
}

static void init_all_data()
{
	gint i;

	memset(omd, 0x00, sizeof(OnvifMotionData)*GUI_CHANNEL_CNT);
	memset(org_omd, 0x00, sizeof(OnvifMotionData)*GUI_CHANNEL_CNT);
	memset(ai_disable, 0x00, sizeof(ai_disable));

	init_motion_setup_info();

	for(i=0; i<GUI_CHANNEL_CNT; i++)
	{
		update_all_data(i);
	}

	update_all_data(0);
}

static void update_all_data(gint ch)
{
	guint mask = g_msi.ch_mask;
	printf_all_data(ch);

	update_corridor_mode(ch);
	
	if(!(mask & (1 << ch))) 
		load_onvif_motion_data(ch);

	update_profile_data(ch);
	update_motion_setup_info(ch);
	printf_all_data(ch);
}

static void set_mot_data()
{
	OnvifMotionData db_data[GUI_CHANNEL_CNT];
	guint mask = g_msi.ch_mask;
	gint i;

	scm_put_log(CHANGE_CAM_MOTION, 0, 0);

	memset(db_data, 0x00, sizeof(OnvifMotionData) * GUI_CHANNEL_CNT);

	for(i=0; i<GUI_CHANNEL_CNT; i++) {
		if(mask & (1 << i)) {
			g_memmove(&org_omd[i], &omd[i], sizeof(OnvifMotionData));
			g_memmove(&db_data[i], &omd[i], sizeof(OnvifMotionData));
			if (g_corridor_mode[i] != 0)
			{
				_trans_val_to_db(i, &omd[i], &db_data[i]);
			}
		}
	}

	if(mask) {
		DAL_set_onvif_motion_data_all(db_data, mask);
		for(i=0; i<GUI_CHANNEL_CNT; i++) 
		{
			DAL_set_onvif_ipcam_data(omd[i].ipcam_d, (guint)i);
		}
		DAL_notify_fire_DB_change(NF_SYSDB_CATE_ALARM);
	}
}

static gint _get_tile_cell_cnt()
{
	gint i, cnt = 0;

	gint ch = g_msi.ch;
	guint r = (guint)g_msi.area_rows;
	guint c = (guint)g_msi.area_cols;

	if (r == 0 || c == 0) return -1;

    for(i = 0 ; i < r*c ; i++) 
    {
		if(omd[ch].area[i] == '1') cnt++;
	}	

	return cnt;
}

static gint _get_minium_block_cnt()
{
	gint cnt;
	gint ch = g_msi.ch;

	if (nfui_nfobject_is_disabled((NFSPINBUTTON*)g_minsObj[0])) return -1;

	cnt = (omd[ch].cell_d.mini_d > omd[ch].cell_d.mini_n) ? omd[ch].cell_d.mini_d : omd[ch].cell_d.mini_n;
	return cnt;
}

static gint _set_warning_msg(gboolean expose)
{
	gint i;
	gchar strBuf[64];

/*http://222.112.8.34:8080/browse/BPM-2251
    if(var_get_vendor_code() == 28)	return 0;
	
	if ((g_mot_prof.area_method != MAM_NONE) && (g_mot_prof.area_method != MAM_GENERAL) && (g_mot_prof.is_itx_cam != 1))
	{
		memset(strBuf, 0x00, sizeof(strBuf));
		g_sprintf(strBuf, "* %s", lookup_string("NOTICE"));

		if (strcmp(nfui_nflabel_get_text((NFLABEL*)g_warn_msg[0]), strBuf) != 0) {
			nfui_nflabel_set_text((NFLABEL*)g_warn_msg[0], strBuf);
			if (expose) nfui_signal_emit((NFLABEL*)g_warn_msg[0], GDK_EXPOSE, TRUE);
		}
	
		if (strcmp(nfui_nflabel_get_text((NFLABEL*)g_warn_msg[1]), STR_MSG1) != 0) {
			nfui_nflabel_set_text((NFLABEL*)g_warn_msg[1], STR_MSG1);
			if (expose) nfui_signal_emit((NFLABEL*)g_warn_msg[1], GDK_EXPOSE, TRUE);
		}
	}
	else
	{
		for (i = 0; i < 2; i++)
		{
			if (strcmp(nfui_nflabel_get_text((NFLABEL*)g_warn_msg[i]), "") != 0) {
				nfui_nflabel_set_text((NFLABEL*)g_warn_msg[i], "");
				if (expose) nfui_signal_emit((NFLABEL*)g_warn_msg[i], GDK_EXPOSE, TRUE);
			}		
		}	
	}
*/

	return 0;
}

static void set_data_from_obj(gint page)
{
	gint min_v;
	gint ch = g_msi.ch;

	switch(page) {
		case DAYTIME_SET_PAGE:
			omd[ch].cell_d.time_start = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)g_timeObj[0]);
			omd[ch].cell_d.time_end  = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)g_timeObj[1]);
			break;

		case SENSITIVITY_PAGE:
			min_v = g_mot_prof.sensitivity_min;

			omd[ch].cell_d.sense_d = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)g_sensObj[0]);
			omd[ch].cell_d.sense_n = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)g_sensObj[1]);
			omd[ch].cell_d.sense_d += min_v;
			omd[ch].cell_d.sense_n += min_v;
			break;

		case MINI_BLOCK_PAGE:
			min_v = g_mot_prof.min_block;

			omd[ch].cell_d.mini_d = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)g_minsObj[0]);
			omd[ch].cell_d.mini_n = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)g_minsObj[1]);
			omd[ch].cell_d.mini_d += min_v;
			omd[ch].cell_d.mini_n += min_v;
			break;

/*
		case INTERVAL_PAGE:
			omd[ch].cell_d.interval = (guint)nfui_spin_button_get_index((NFSPINBUTTON*)g_intvObj);
			omd[ch].cell_d.interval += 1;
			break;
*/            
	}
}

static void reset_data(gint ch)
{
	MOTION_AREA_METHOD_E method = g_mot_prof.area_method;
	gint r = g_mot_prof.block_height;
	gint c = g_mot_prof.block_width;

	//memset(&omd[ch].area, '1', sizeof(omd[ch].area));
	//memset(&org_omd[ch].area, '1', sizeof(org_omd[ch].area));

	if(method == MAM_RECTANGLE) {
		memset(&omd[ch].rect, 0x00, sizeof(RectArea) * MAX_RECT_COUNT);
		memset(&org_omd[ch].rect, 0x00, sizeof(RectArea) * MAX_RECT_COUNT);

		//omd[ch].rect_cnt = 0;
		org_omd[ch].rect[0].start_r = omd[ch].rect[0].start_r = 0;
		org_omd[ch].rect[0].start_c = omd[ch].rect[0].start_c = 0;
		org_omd[ch].rect[0].end_r = omd[ch].rect[0].end_r = r;
		org_omd[ch].rect[0].end_c = omd[ch].rect[0].end_c = c;
	}

    if (omd[ch].cell_d.sense_d > g_mot_prof.sensitivity_max)
    {
        omd[ch].cell_d.sense_d = g_mot_prof.sensitivity_max;
        org_omd[ch].cell_d.sense_n = omd[ch].cell_d.sense_d;
    }

    if (omd[ch].cell_d.sense_d < g_mot_prof.sensitivity_min)
    {
        omd[ch].cell_d.sense_d = g_mot_prof.sensitivity_min;
        org_omd[ch].cell_d.sense_n = omd[ch].cell_d.sense_d;
    }

//	org_omd[ch].cell_d.sense_d = omd[ch].cell_d.sense_d = (guint)g_mot_prof.sensitivity_max;
//	org_omd[ch].cell_d.sense_n = omd[ch].cell_d.sense_n = (guint)g_mot_prof.sensitivity_max;
}

static void set_area_data_selectAll()
{
	gint i;
	gint ch = g_msi.ch;
	gint r = g_msi.area_rows;
	gint c = g_msi.area_cols;
	gint method = g_msi.area_method;

	if(method == MAM_RECTANGLE) {
		omd[ch].rect_cnt = 1;

		omd[ch].rect[0].start_r = 0;
		omd[ch].rect[0].start_c = 0;
		omd[ch].rect[0].end_r = r - 1;
		omd[ch].rect[0].end_c = c - 1;
	}
	memset(&omd[ch].area, '1', (size_t)(r * c));
}

static void set_area_data_deselectAll()
{
	gint i;
	gint ch = g_msi.ch;
	gint r = g_msi.area_rows;
	gint c = g_msi.area_cols;
	gint method = g_msi.area_method;

	if(method == MAM_RECTANGLE) {
		omd[ch].rect_cnt = 0;
		memset(&omd[ch].rect, 0x00, sizeof(RectArea) * MAX_RECT_COUNT);
	}
	memset(&omd[ch].area, '0', (size_t)(r * c));
}

static gboolean is_selected_all()
{
	gint i, j;
	gint ch = g_msi.ch;
	gint r = g_msi.area_rows;
	gint c = g_msi.area_cols;

	for(i = 0 ; i < r ; i++) {
		for(j = 0 ; j < c ; j++) {
			if(omd[ch].area[i * c + j] == '0')
				return FALSE;
		}
	}
	return TRUE;
}

static void set_rect_data(guint r1, guint c1, guint r2, guint c2)
{
	guint i, j;
	guint sr, sc, er, ec;
	gint ch = g_msi.ch;
	gint idx = omd[ch].rect_cnt;

	omd[ch].rect[idx].start_r = r1;
	omd[ch].rect[idx].start_c = c1;
	omd[ch].rect[idx].end_r = r2;
	omd[ch].rect[idx].end_c = c2;

	omd[ch].rect_cnt+=1;
}

static gboolean set_area_data(guchar val, guint r1, guint c1, guint r2, guint c2)
{
	gint ch = g_msi.ch;
	guint i, j;
	gboolean changed = FALSE;
	gint col_size = g_msi.area_cols;
	gint method = g_msi.area_method;

	for(i=r1; i<=r2; i++) {
		for(j=c1; j<=c2; j++) {
			if(omd[ch].area[i * col_size + j] != val) {
				omd[ch].area[i * col_size + j] = val;

				if(!changed)
					changed = TRUE;
			}
		}
	}

	if(method == MAM_RECTANGLE) {
//		if(changed)
			set_rect_data(r1, c1, r2, c2);
	}

	return changed;
}

static void set_data_to_time_page(gboolean expose)
{
	NFOBJECT *obj[2];
	gint ch = g_msi.ch;

	nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)g_timeObj[0], (guint)omd[ch].cell_d.time_start);
	nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)g_timeObj[1], (guint)omd[ch].cell_d.time_end);

	if(expose) {
		nfui_signal_emit(g_timeObj[0], GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_timeObj[1], GDK_EXPOSE, TRUE);
	}
}

static void set_data_to_sens_page(gboolean expose)
{
	NFOBJECT *obj[2];
	gint ch = g_msi.ch;
	gint min_v = g_mot_prof.sensitivity_min;

	nfui_spin_button_set_range((NFSPINBUTTON*)g_sensObj[0], (gdouble)g_mot_prof.sensitivity_min, (gdouble)g_mot_prof.sensitivity_max);
	nfui_spin_button_set_range((NFSPINBUTTON*)g_sensObj[1], (gdouble)g_mot_prof.sensitivity_min, (gdouble)g_mot_prof.sensitivity_max);

	nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)g_sensObj[0], (guint)omd[ch].cell_d.sense_d - min_v);
	nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)g_sensObj[1], (guint)omd[ch].cell_d.sense_n - min_v);

	obj[0] = (NFOBJECT*)nfui_nfobject_get_data(g_sensObj[0], "slider-obj");
	obj[1] = (NFOBJECT*)nfui_nfobject_get_data(g_sensObj[1], "slider-obj");

	cw_slider_set_range((CWSLIDER*)obj[0], g_mot_prof.sensitivity_min, g_mot_prof.sensitivity_max, g_mot_prof.sensitivity_max); 
	cw_slider_set_range((CWSLIDER*)obj[1], g_mot_prof.sensitivity_min, g_mot_prof.sensitivity_max, g_mot_prof.sensitivity_max); 

	cw_slider_set_value((CWSLIDER*)obj[0], (gint)omd[ch].cell_d.sense_d);
	cw_slider_set_value((CWSLIDER*)obj[1], (gint)omd[ch].cell_d.sense_n);

	if(expose) {
		nfui_signal_emit(g_sensObj[0], GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_sensObj[1], GDK_EXPOSE, TRUE);

		nfui_signal_emit(obj[0], GDK_EXPOSE, FALSE);
		nfui_signal_emit(obj[1], GDK_EXPOSE, FALSE);
	}
#if PRINT_MSG
	g_message("::::::::::::: %s ::::::::::::::::", __FUNCTION__);
	g_printf("prof sense max: %d, min: %d\n", g_mot_prof.sensitivity_max, g_mot_prof.sensitivity_min);
	g_printf("db sense d: %d, n: %d\n", omd[ch].cell_d.sense_d, omd[ch].cell_d.sense_n);
	g_message(":::::::::::::::::::::::::::::::::");
#endif
}

static void set_data_to_min_page(gboolean expose)
{
	NFOBJECT *obj[2];
	gint ch = g_msi.ch;
	gint min_v = g_mot_prof.min_block;

	nfui_spin_button_set_range((NFSPINBUTTON*)g_minsObj[0], (gdouble)g_mot_prof.min_block, (gdouble)g_mot_prof.num_blocks);
	nfui_spin_button_set_range((NFSPINBUTTON*)g_minsObj[1], (gdouble)g_mot_prof.min_block, (gdouble)g_mot_prof.num_blocks);

	nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)g_minsObj[0], (guint)omd[ch].cell_d.mini_d - min_v);
	nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)g_minsObj[1], (guint)omd[ch].cell_d.mini_n - min_v);

	obj[0] = (NFOBJECT*)nfui_nfobject_get_data(g_minsObj[0], "slider-obj");
	obj[1] = (NFOBJECT*)nfui_nfobject_get_data(g_minsObj[1], "slider-obj");

	cw_slider_set_range((CWSLIDER*)obj[0], g_mot_prof.min_block, g_mot_prof.num_blocks, g_mot_prof.num_blocks); 
	cw_slider_set_range((CWSLIDER*)obj[1], g_mot_prof.min_block, g_mot_prof.num_blocks, g_mot_prof.num_blocks); 

	cw_slider_set_value((CWSLIDER*)obj[0], (gint)omd[ch].cell_d.mini_d);
	cw_slider_set_value((CWSLIDER*)obj[1], (gint)omd[ch].cell_d.mini_n);

	if(expose) {
		nfui_signal_emit(g_minsObj[0], GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_minsObj[1], GDK_EXPOSE, TRUE);

		nfui_signal_emit(obj[0], GDK_EXPOSE, FALSE);
		nfui_signal_emit(obj[1], GDK_EXPOSE, FALSE);
	}
#if PRINT_MSG
	g_message("::::::::::::: %s ::::::::::::::::", __FUNCTION__);
	g_printf("prof mini max: %d, min: %d\n", g_mot_prof.num_blocks, g_mot_prof.min_block);
	g_printf("db mini d: %d, n: %d\n", omd[ch].cell_d.mini_d, omd[ch].cell_d.mini_n);
	g_message(":::::::::::::::::::::::::::::::::");
#endif
}

static void set_data_to_intv_page(gboolean expose)
{
	NFOBJECT *obj[2];
	gint ch = g_msi.ch;
	gint i;
	
	nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)g_intvObj, (guint)omd[ch].cell_d.interval-1);

	obj[0] = (NFOBJECT*)nfui_nfobject_get_data(g_intvObj, "slider-obj");
	cw_slider_set_value((CWSLIDER*)obj[0], (gint)omd[ch].cell_d.interval);

	if(expose) {
		nfui_signal_emit(g_intvObj, GDK_EXPOSE, TRUE);
		nfui_signal_emit(obj[0], GDK_EXPOSE, FALSE);
	}
}

static gboolean redraw_tile(gpointer data)
{
	NFUTIL_THREADS_ENTER();
	draw_tile(g_area);
	NFUTIL_THREADS_LEAVE();

	return FALSE;
}

static void set_tile()
{
	guint i, j;
	gint metrix = 0;

	guint r = (guint)g_msi.area_rows;
	guint c = (guint)g_msi.area_cols;
	gint ch = g_msi.ch;

    for(i = 0 ; i < r ; i++) {
        for(j = 0 ; j < c ; j++) {
				if(omd[ch].area[metrix] == '1')
					nfui_tile_set_select_state(NF_TILE(g_area), i, j);
				else
					nfui_tile_set_conv_select_state(NF_TILE(g_area), i, j);

				metrix++;
			} 
		}	
}

static void set_data_to_obj(gboolean expose)
{
	gint ch = g_msi.ch;
	gchar *token_ptr;
	gint i;
	gint temp = 0;
	gchar tempbuf[256];
	gchar *pbuf = NULL;
	gchar *pnext = NULL;

	if (omd[ch].smart_motion) nfui_radio_button_set_toggled(NF_BUTTON(g_smart_motion_obj), TRUE);
	else nfui_radio_button_set_toggled(NF_BUTTON(g_classic_motion_obj), TRUE);

#if 0	// skshin	
	nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_ai_alarmevt_obj, FALSE);
#endif

	for(i=0; i<MOTION_OBJ_CNT; i++)
	{
		if (omd[ch].ipcam_d.smart_motion_options[i].enable) nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_smart_motion_chk[i], TRUE);
		else nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_smart_motion_chk[i], FALSE);
		nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)g_smart_motion_spin[i], (guint)omd[ch].ipcam_d.smart_motion_options[i].threshold - 1);
	}

#if 0 // skshin	
	if (omd[ch].use_ai_alarmevt) nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_ai_alarmevt_obj, TRUE);
#endif

	memset(tempbuf, 0x00, 256);
	memcpy(tempbuf, omd[ch].ipcam_d.smart_interest_obj, 256);

	if (omd[ch].smart_motion) {
		token_ptr = strtok_r(tempbuf, ",", &pnext);
		pbuf = pnext;
	while (token_ptr != 0) {
			temp += 1;
			token_ptr = strtok_r(pbuf, ",", &pnext);
			pbuf = pnext;
		}

		if(temp == 1)
		{
			for(i=0; i<MOTION_OBJ_CNT; i++)
			{
				nfui_nfobject_disable(g_smart_motion_chk[i]);
				nfui_nfobject_disable(g_smart_motion_spin[i]);
			}
			ai_disable[ch] = 1;
		}
		else
		{
			for(i=0; i<MOTION_OBJ_CNT; i++)
			{
				nfui_nfobject_enable(g_smart_motion_chk[i]);
				nfui_nfobject_enable(g_smart_motion_spin[i]);
			}
			ai_disable[ch] = 0;
		}

	}
	else {
		for(i=0; i<MOTION_OBJ_CNT; i++)
		{
			nfui_nfobject_disable(g_smart_motion_chk[i]);
			nfui_nfobject_disable(g_smart_motion_spin[i]);
		}
	}

	if (expose) nfui_signal_emit(g_classic_motion_obj, GDK_EXPOSE, TRUE);
	if (expose) nfui_signal_emit(g_smart_motion_obj, GDK_EXPOSE, TRUE);
	for(i=0; i<MOTION_OBJ_CNT; i++)
	{
		if (expose) nfui_signal_emit(g_smart_motion_chk[i], GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_smart_motion_spin[i], GDK_EXPOSE, TRUE);
	}

#if 0 // skshin
	if (expose) nfui_signal_emit(g_ai_alarmevt_obj, GDK_EXPOSE, TRUE);
#endif

	// set data to page objs
	set_data_to_time_page(expose);
	set_data_to_sens_page(expose);
	set_data_to_min_page(expose);
//    set_data_to_intv_page(expose);
	
	// draw tile
	if(expose)
		g_timeout_add(150, redraw_tile, NULL);
    else
        set_tile();
}

static gint _get_smart_object_filter_string(gchar *str, gint len, gint ch)
{
	gint i;
	gchar temp[256];

	for(i=0; i<MOTION_OBJ_CNT; i++)
	{
		//if (nfui_check_button_get_active(NF_CHECKBUTTON(g_smart_motion_chk[i]))) {
		if (strlen(str) == 0) 
			{
				//sprintf(str, "%s:%03d:%d", nfui_nflabel_get_text((NFLABEL *)g_smart_label_obj[i]), nfui_spin_button_get_value(g_smart_spin_obj[i]), nfui_check_button_get_active(NF_CHECKBUTTON(g_smart_obj[i])));
				sprintf(str, "%s:%03d:%d", nfui_nflabel_get_text((NFLABEL *)g_smart_motion_label[i]), nfui_spin_button_get_value(g_smart_motion_spin[i]), nfui_check_button_get_active(NF_CHECKBUTTON(g_smart_motion_chk[i])));
				strcpy(temp, str);
			}
			else 
			{
				//sprintf(str, "%s,%s:%03d:%d", tempbuf, nfui_nflabel_get_text((NFLABEL *)g_smart_label_obj[i]), nfui_spin_button_get_value(g_smart_spin_obj[i]), nfui_check_button_get_active(NF_CHECKBUTTON(g_smart_obj[i])));
				sprintf(str, "%s,%s:%03d:%d", temp, nfui_nflabel_get_text((NFLABEL *)g_smart_motion_label[i]), nfui_spin_button_get_value(g_smart_motion_spin[i]), nfui_check_button_get_active(NF_CHECKBUTTON(g_smart_motion_chk[i])));
				strcpy(temp, str);
			}
		//}
	}

	// g_message("%s(%d) => %s", __FUNCTION__, __LINE__, str);
	return 0;
}

static void show_preview(gint ch)
{
	guint ch_mask = 0;
	gint x, y;

	if(ch < 0) return;
	//if(g_preCh != ch) g_preCh = ch;
	if(g_msi.preCh != ch) g_msi.preCh = ch;

	nfui_nfobject_get_offset(g_area, &x, &y);

	ch_mask |= (1 << ch);
	nf_ipcam_set_mraw_ch(ch);	
	vsm_live_preview_start(ch_mask, (guint)x, (guint)y, g_mot_area_w, MOTION_AREA_BACK_SIZE_H);		
}

static void pause_preview()
{
    if (nfui_nfobject_is_shown(g_area))
    {
    	GdkDrawable *drawable = nfui_nfobject_get_window((NFOBJECT*)g_area);
    	GdkGC *gc = nfui_nfobject_get_gc((NFOBJECT*)g_area);
    	GdkColor color = UX_COLOR(COLOR_PRG_IDX(UX_COLOR_808080));
    	gint off_x, off_y;

    	gdk_gc_set_rgb_fg_color(gc, &color);
    	nfui_nfobject_get_offset(g_area, &off_x, &off_y);

    	gdk_draw_rectangle(drawable,
    			gc,
    			TRUE,
    			off_x, off_y,
    			g_mot_area_w, MOTION_AREA_BACK_SIZE_H);

    	nfui_nfobject_gc_unref(gc);
    }
	
	vsm_live_preview_stop();
}

static void stop_preview()
{
	g_msi.preCh = -1;

	nf_ipcam_set_mraw_ch(0xff);
	vsm_live_preview_stop();
}

static void draw_tile(NFOBJECT *tile)
{
	guint i, j;
	gint metrix = 0;
	guint r = (guint)g_msi.area_rows;
	guint c = (guint)g_msi.area_cols;
	gint ch = g_msi.ch;

	if(nfui_nfobject_is_disabled(tile)) 
	{
		nfui_tile_conv_selectArea(NF_TILE(tile), 0, 0, r - 1, c - 1);
	} 
	else 
	{
		for(i = 0 ; i < r ; i++) {
			for(j = 0 ; j < c ; j++) {
				if(omd[ch].area[metrix] == '1')
					nfui_tile_set_select_state(NF_TILE(tile), i, j);
				else
					nfui_tile_set_conv_select_state(NF_TILE(tile), i, j);

				metrix++;
			} 
		}	
		nfui_tile_draw_area(NF_TILE(tile), 0, 0, r - 1, c - 1);
	}
}

static void enable_setup(gint method)
{
	NFOBJECT *child = NULL;
	gint i = 0, j = 0;
#if 0
	if(method == MAM_RECTANGLE) {
		nfui_nfobject_hide(g_area[CELL]);
		nfui_nfobject_enable(g_area[CELL]);
		nfui_signal_emit(g_area[CELL], GDK_EXPOSE, TRUE);

		nfui_nfobject_show(g_area[RECT]);
		nfui_nfobject_enable(g_area[RECT]);
		nfui_signal_emit(g_area[RECT], GDK_EXPOSE, TRUE);
	}
	else if(method == MAM_CELL) {
		//nfui_nfobject_show(g_area[CELL]);
		nfui_nfobject_enable(g_area[CELL]);
		nfui_signal_emit(g_area[CELL], GDK_EXPOSE, TRUE);
	}
#endif

	for(i=0; i<SUBPAGE_CNT; i++) {
		while(1) {
			child = g_slist_nth_data(((NFFIXED*)g_tabPage[i])->children, j++);

			if(!child) break;
			if(((NFOBJECT*)child)->type == NFOBJECT_TYPE_NFLABEL) continue;

			nfui_nfobject_enable(child);
			nfui_signal_emit(child, GDK_EXPOSE, TRUE);
		}
		j = 0;
	}

	//nfui_nfobject_show(g_area);
	nfui_nfobject_enable(g_area);
	nfui_nfobject_enable(g_btns[BTNS_SELECT_ALL]);
	nfui_nfobject_enable(g_btns[BTNS_DESELECT_ALL]);

	nfui_signal_emit(g_area, GDK_EXPOSE, TRUE);
	nfui_signal_emit(g_btns[BTNS_SELECT_ALL], GDK_EXPOSE, TRUE);
	nfui_signal_emit(g_btns[BTNS_DESELECT_ALL], GDK_EXPOSE, TRUE);
}

static void disable_setup()
{
	NFOBJECT *child = NULL;
	gint i = 0, j = 0;
#if 0
	nfui_nfobject_hide(g_area[RECT]);
	nfui_nfobject_disable(g_area[RECT]);
	nfui_signal_emit(g_area[RECT], GDK_EXPOSE, TRUE);
#endif

	for(i=0; i<SUBPAGE_CNT; i++) {
		while(1) {
			child = g_slist_nth_data(((NFFIXED*)g_tabPage[i])->children, j++);

			if(!child) break;
			if(((NFOBJECT*)child)->type == NFOBJECT_TYPE_NFLABEL) continue;

			nfui_nfobject_disable(child);
			nfui_signal_emit(child, GDK_EXPOSE, TRUE);
		}
		j = 0;
	}

	//nfui_nfobject_show(g_area);
	nfui_nfobject_disable(g_area);
	nfui_nfobject_disable(g_btns[BTNS_SELECT_ALL]);
	nfui_nfobject_disable(g_btns[BTNS_DESELECT_ALL]);

	nfui_signal_emit(g_area, GDK_EXPOSE, TRUE);
	nfui_signal_emit(g_btns[BTNS_SELECT_ALL], GDK_EXPOSE, TRUE);
	nfui_signal_emit(g_btns[BTNS_DESELECT_ALL], GDK_EXPOSE, TRUE);
}

static void enable_mini_tab()
{
	NFOBJECT *child = NULL;
	gint i = 0;

	while(1) {
		child = g_slist_nth_data(((NFFIXED*)g_tabPage[MINI_BLOCK_PAGE])->children, i++);

		if(!child) break;
		if(((NFOBJECT*)child)->type == NFOBJECT_TYPE_NFLABEL) continue;

		nfui_nfobject_enable(child);
		nfui_signal_emit(child, GDK_EXPOSE, TRUE);
	}
}

static void disable_mini_tab()
{
	NFOBJECT *child = NULL;
	gint i = 0;

	while(1) {
		child = g_slist_nth_data(((NFFIXED*)g_tabPage[MINI_BLOCK_PAGE])->children, i++);

		if(!child) break;
		if(((NFOBJECT*)child)->type == NFOBJECT_TYPE_NFLABEL) continue;

		nfui_nfobject_disable(child);
		nfui_signal_emit(child, GDK_EXPOSE, TRUE);
	}
}

static gint _config_smart_motion_fixed(gint is_supported_smart, gint expose)
{
	if (is_supported_smart)
	{
		nfui_nfobject_move(g_tab_fixed, 0, 300);

		nfui_nfobject_show(g_smart_fixed);
		nfui_nfobject_show(g_tab_fixed);
	}
	else
	{
		nfui_nfobject_move(g_tab_fixed, 0, 20);

		nfui_nfobject_hide(g_smart_fixed);
		nfui_nfobject_show(g_tab_fixed);
	}

	if (expose) {
		nfui_signal_emit(g_setup_fixed, GDK_EXPOSE, TRUE);
		nfui_make_key_hierarchy((NFWINDOW*)g_curwnd);
	}

	return 0;
}

static void change_area(gint method)
{
	/*
	switch(method) {
		case MAM_RECTANGLE:
			if(!nfui_nfobject_is_shown(g_area[RECT])) {
				nfui_nfobject_hide(g_area[CELL]);
				nfui_signal_emit(g_area[CELL], GDK_EXPOSE, FALSE);

				nfui_nfobject_show(g_area[RECT]);
				nfui_signal_emit(g_area[RECT], GDK_EXPOSE, FALSE);
			}
			break;

		case MAM_CELL:
			if(!nfui_nfobject_is_shown(g_area[CELL])) {
				nfui_nfobject_hide(g_area[RECT]);
				nfui_signal_emit(g_area[RECT], GDK_EXPOSE, FALSE);

				nfui_nfobject_show(g_area[CELL]);
				nfui_signal_emit(g_area[CELL], GDK_EXPOSE, FALSE);
			}
			break;
	}
	*/
}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_EXPOSE:
			{
				if(g_msi.preCh < 0) {
					show_preview(0);
				}
			}
			break;

		case GDK_DELETE:
#if defined(_SUPPORT_GUI_MDRAW)
            VW_MotionDraw_finalize();
#endif        	
			g_curwnd = 0;
			break;

		default:
			break;
	}

	return FALSE;
}

static gboolean pre_setup_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case GDK_EXPOSE:
		{
			GdkGC *gc;
			GdkDrawable *drawable = NULL;
			gint off_x, off_y;

			drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
			gc = gdk_gc_new(drawable);

			nfui_nfobject_get_offset(obj, &off_x, &off_y);

			nfutil_draw_image(drawable, gc, MK_IMG_GROUP_BG_476_220, 
					obj->x+off_x, obj->y+off_y-40, -1, -1, NFALIGN_LEFT, 0);
					//1400, 320, -1, -1, NFALIGN_LEFT, 0);

	 		nfui_nfobject_gc_unref(gc);
	 	}
		break;

		default :
		break;
	}

	return FALSE;
}

static gboolean post_ch_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {
		MOTION_AREA_METHOD_E method;
		gint new_r, new_c;
		gint new_ch = -1;
		gint old_ch = g_msi.ch;
		gint old_r = g_msi.area_rows;
		gint old_c = g_msi.area_cols;
		gint old_method = g_msi.area_method;
		guint mask = g_msi.ch_mask;
		gint max_cnt;
		gint i, j;
		gint off_x, off_y;

		if (check_motsen_block_cnt() == 0) 
		{
			nfui_combobox_set_index_no_expose(NF_COMBOBOX(obj), old_ch);
			nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
			return FALSE;
		}
#if 0
		if (check_motsen_detect_object() == 0)
		{
			nfui_combobox_set_index_no_expose(NF_COMBOBOX(obj), old_ch);
			nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
			return FALSE;
		}
#endif
		new_ch = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));

		update_all_data(new_ch);
		_set_mot_area_width(new_ch);
		
		nfui_nfobject_set_size(g_area, g_mot_area_w, MOTION_AREA_BACK_SIZE_H);
		nfui_nffixed_put((NFFIXED*)g_video_back_fixed, g_area, (MOTION_AREA_BACK_SIZE_W-g_mot_area_w)/2, 0);
		
		nfui_nfobject_get_offset(g_area, &off_x, &off_y);
		
#if defined(_SUPPORT_GUI_MDRAW)
		VW_MotionDraw_stop();
		VW_MotionDraw_set_obj(new_ch, g_area);
		VW_MotionDraw_set_plt_position(off_x, off_y, g_mot_area_w, MOTION_AREA_BACK_SIZE_H);
        VW_MotionDraw_set_selectArea(omd[new_ch].area);
#endif

		if (g_mot_prof.smart_motion_support) _config_smart_motion_fixed(1, 1);
		else _config_smart_motion_fixed(0, 1);

		method = g_msi.area_method;

		switch(method) {
			case MAM_NONE:
				{
					//if(old_method == MAM_NONE) return FALSE;
					
					if(old_ch != new_ch) 
						nfui_tile_conv_selectArea(NF_TILE(g_area), (guint)0, (guint)0, (guint)old_r - 1, (guint)old_c - 1);

					if(old_method != MAM_NONE) disable_setup();

					nfui_tile_reset_area_size((NFTILE*)g_area, 1, 1);
					
					show_preview(new_ch);
				}
				break;

			case MAM_RECTANGLE:
			case MAM_CELL:
				{
					// preview 
					show_preview(new_ch);

					if(old_method == MAM_NONE) enable_setup(MAM_RECTANGLE);

					if(old_ch != new_ch) 
						nfui_tile_conv_selectArea(NF_TILE(g_area), (guint)0, (guint)0, (guint)old_r - 1, (guint)old_c - 1);

					new_r = g_msi.area_rows;
					new_c = g_msi.area_cols;
					if(old_r != new_r || old_c != new_c) 
						nfui_tile_reset_area_size((NFTILE*)g_area, (guint)(new_r == 0 ? 1 : new_r), (guint)(new_c == 0 ? 1 : new_c));

					disable_mini_tab();
					set_data_to_obj(TRUE);
				}
				break;

			case MAM_RAW_STREAM:
			case MAM_GENERAL:			
				{
					// preview 
					show_preview(new_ch);

					if(old_method == MAM_NONE) enable_setup(MAM_CELL);

					if(old_ch != new_ch)
						nfui_tile_conv_selectArea(NF_TILE(g_area), (guint)0, (guint)0, (guint)old_r - 1, (guint)old_c - 1);

					new_r = g_msi.area_rows;
					new_c = g_msi.area_cols;
					if(old_r != new_r || old_c != new_c) 
						nfui_tile_reset_area_size((NFTILE*)g_area, (guint)(new_r == 0 ? 1 : new_r), (guint)(new_c == 0 ? 1 : new_c));

					enable_mini_tab();
					set_data_to_obj(TRUE);
				}
				break;

			default:
				return FALSE;
		}
		
		nfui_signal_emit(g_area, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_video_back_fixed, GDK_EXPOSE, TRUE);

		_set_warning_msg(TRUE);	

#if defined(_SUPPORT_GUI_MDRAW)
        VW_MotionDraw_set_daytime(omd[new_ch].cell_d.time_start, omd[new_ch].cell_d.time_end);
        VW_MotionDraw_set_sense(omd[new_ch].cell_d.sense_d, omd[new_ch].cell_d.sense_n);
		VW_MotionDraw_start(); 
#endif		
	}
	else if ((evt->type == INFY_CAMDB_CHANGE_NOTIFY) || (evt->type == INFY_USRDB_CHANGE_NOTIFY)) {
		gchar strCh[STRING_SIZE_CAMTITLE+8];
		gint i, j;
		gint ch = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));

		nfui_combobox_remove_all((NFCOMBOBOX*)obj);

		for (i = 0; i<GUI_CHANNEL_CNT; i++) {
			memset(strCh, 0, (STRING_SIZE_CAMTITLE+8));
			j = sprintf(strCh, "%s%d - ", lookup_string("CH"), i+1);

			//DAL_get_camera_title(&strCh[j], (guint)i);
			var_get_camtitle(&strCh[j], (guint)i);
			nfui_combobox_append_data((NFCOMBOBOX*)obj, strCh);
		}

		nfui_combobox_set_index_no_expose(NF_COMBOBOX(obj), ch);
		nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
	}
	else if(evt->type == GDK_DELETE)  {
		uxm_unreg_imsg_event(obj, INFY_CAMDB_CHANGE_NOTIFY);
		uxm_unreg_imsg_event(obj, INFY_USRDB_CHANGE_NOTIFY);
	}

	return FALSE;
}

static gboolean post_new_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{
		NFOBJECT *top;
		gchar *strTemp;
		gint x, y;
		gint string_size;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;


		nfui_nfobject_get_offset(obj, &x, &y);
		top = nfui_nfobject_get_top(obj);

		x += (obj->width)/2 + top->x;
		y += obj->height + top->y;
		
		string_size = GPOINTER_TO_INT((gint)nfui_nfobject_get_data(obj, "string_size"));
		strTemp = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), 
								  (guint)x, (guint)y, 
								  string_size, 
								  VKEY_NORMAL);
		
		if(strTemp)
		{
			nfui_nflabel_set_text((NFLABEL*)obj, strTemp);

			ifree(strTemp);
			strTemp = NULL;
		}

		nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
		
	}

	return FALSE;
}

static gboolean post_new_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;
	}
	return FALSE;
}

static gboolean post_del_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;
	}
	return FALSE;
}

static gboolean post_edit_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;
	}
	return FALSE;
}

static gboolean post_tile_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_TILE_INIT)
	{
		draw_tile(obj);
	}
	else if(evt->type == NFEVENT_TILE_END_SELECT)
	{
		mb_type mb_ret;
		gint ch = g_msi.ch;
		gint r = g_msi.area_rows;
		gint c = g_msi.area_cols;
		gint method = g_msi.area_method;
		gint max_cnt = g_msi.max_rect_cnt;
		gint idx;
		guint x1, y1, x2, y2;
		guint cur_state;
		guint i, j;

		nfui_tile_get_selectArea(NF_TILE(obj), &y1, &x1, &y2, &x2);
		cur_state = nfui_tile_get_cur_state(NF_TILE(obj));

		switch(method) 
		{ 
			case MAM_RECTANGLE: 
			{
				// check max rect count
				if (omd[ch].rect_cnt >= max_cnt) 
				{
					draw_tile(obj);
					mb_ret = nftool_mbox(g_curwnd, "NOTICE", "The previously selected area will be deselected.\nDo you want to continue?",  NFTOOL_MB_OKCANCEL);

					if (mb_ret == NFTOOL_MB_CANCEL) break;

					set_area_data_deselectAll();
				}
				
				// exception: deselect all
				if((y2 - y1) == (r - 1) && (x2 - x1) == (c - 1)) 
				{
					if(cur_state == NFTILE_STATE_NORMAL) 
					{
						if (is_selected_all()) 
						{
							set_area_data_deselectAll();
							draw_tile(obj);
							return FALSE;
						}
					}
				}
				
				// exception: first select
				if (omd[ch].rect_cnt <= 1) 
				{
					if(is_selected_all()) set_area_data_deselectAll();
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
    	VW_MotionDraw_set_selectArea(omd[ch].area);	
        VW_MotionDraw_start();
#endif			
	}
	return FALSE;
}

static gboolean post_smart_motion_onoff_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_RADIO_GET_FOCUS_ALWAYS) 
	{
		gint ch = g_msi.ch;
		gint index, i;

		index = nfui_radio_button_get_index((NFBUTTON*)obj);

		if (index == 1) omd[ch].smart_motion = 1;
		else omd[ch].smart_motion = 0;

		for(i=0; i<MOTION_OBJ_CNT; i++)
		{
			if (omd[ch].smart_motion) 
			{
				if(!ai_disable[ch])
				{
					nfui_nfobject_enable(g_smart_motion_chk[i]);
					nfui_nfobject_enable(g_smart_motion_spin[i]);
				}
			}
			else 
			{
				nfui_nfobject_disable(g_smart_motion_chk[i]);
				nfui_nfobject_disable(g_smart_motion_spin[i]);
			}

			nfui_signal_emit(g_smart_motion_chk[i], GDK_EXPOSE, TRUE);
			nfui_signal_emit(g_smart_motion_spin[i], GDK_EXPOSE, TRUE);
		}
	}
	return FALSE;
}

static gboolean post_smart_motion_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_CHECKBUTTON_CHANGED || evt->type == NFEVENT_SPINBUTTON_CHANGED) 
	{
		gint ch = g_msi.ch;
		gint temp = 0;
		gint i;

	if (evt->type == NFEVENT_CHECKBUTTON_CHANGED) 
	{
			for(i=0; i<MOTION_OBJ_CNT; i++)
			{
				temp += nfui_check_button_get_active(g_smart_motion_chk[i]);
			}

			if(temp == 0)
			{
				nftool_mbox(g_curwnd, "NOTICE", "Please select at least one object.", NFTOOL_MB_OK);
				nfui_check_button_set_active(obj, TRUE);
				return FALSE;
			}
		}

		memset(omd[ch].ipcam_d.smart_interest_obj, 0x00, sizeof(omd[ch].ipcam_d.smart_interest_obj));
		_get_smart_object_filter_string(omd[ch].ipcam_d.smart_interest_obj, sizeof(omd[ch].ipcam_d.smart_interest_obj), ch);
	}
	else if(evt->type == GDK_2BUTTON_PRESS)
    {
        NFOBJECT *top;
		guint x, y;
		gint i, num;
		gint ch = g_msi.ch;

		for(i=0; i<MOTION_OBJ_CNT; i++)
		{
			if(obj == g_smart_motion_spin[i]) break;
		}

		if(i == MOTION_OBJ_CNT) return FALSE;

        top = nfui_nfobject_get_top(obj);

        if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

        nfui_nfobject_get_window_pos(obj, &x, &y);

        x += ((GdkEventButton*)evt)->x;
        y += ((GdkEventButton*)evt)->y;

		while(1)
		{
			num = NumberKey_Open(top, nfui_spin_button_get_value((NFSPINBUTTON*)obj), x, y, 200);

			if(num == 0) nftool_mbox((NFWINDOW*)top, "NOTICE", "Can't select 0", NFTOOL_MB_OK);
			else break;
		}

        nfui_spin_button_set_index((NFSPINBUTTON*)obj, num - 1);
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

		memset(omd[ch].ipcam_d.smart_interest_obj, 0x00, sizeof(omd[ch].ipcam_d.smart_interest_obj));
		_get_smart_object_filter_string(omd[ch].ipcam_d.smart_interest_obj, sizeof(omd[ch].ipcam_d.smart_interest_obj), ch);
	}
	return FALSE;
}

static gboolean post_rcv_motion_alarmevt_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_CHECKBUTTON_CHANGED) 
	{
		gint ch = g_msi.ch;

		if (nfui_check_button_get_active(NF_CHECKBUTTON(obj))) omd[ch].use_ai_alarmevt = 1;
		else omd[ch].use_ai_alarmevt = 0;
	}
	return FALSE;
}

static gboolean post_select_all_area_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) {

		set_area_data_selectAll();
		draw_tile(g_area);
#if defined(_SUPPORT_GUI_MDRAW)					
		VW_MotionDraw_set_selectArea(omd[g_msi.ch].area);
		VW_MotionDraw_start();
#endif			
	}
	return FALSE;
}

static gboolean post_deselect_all_area_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) {
		set_area_data_deselectAll(); 
		draw_tile(g_area);
#if defined(_SUPPORT_GUI_MDRAW)										
		VW_MotionDraw_set_selectArea(omd[g_msi.ch].area);
		VW_MotionDraw_start();
#endif			
	}
	return FALSE;
}

static gboolean post_sense_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case GDK_BUTTON_RELEASE :
		case NFEVENT_CWSLIDER_CHANGED_RELEASE :
			{
				NFOBJECT *spin;
				gint min_v = g_mot_prof.sensitivity_min;
				gint val;

				val = cw_slider_get_value((CWSLIDER*)obj);

				spin = (NFOBJECT*)nfui_nfobject_get_data(obj, "spin-obj");
				nfui_spin_button_set_index((NFSPINBUTTON*)spin, (guint)val - min_v);

				set_data_from_obj(SENSITIVITY_PAGE);

#if defined(_SUPPORT_GUI_MDRAW)
                VW_MotionDraw_stop();
				VW_MotionDraw_set_selectArea(omd[g_msi.ch].area);
                VW_MotionDraw_set_sense(omd[g_msi.ch].cell_d.sense_d, omd[g_msi.ch].cell_d.sense_n);
                VW_MotionDraw_start();
#endif
			}
			break;

		default : 
			break;
	}
	return FALSE;
}

static gboolean post_sense_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_SPINBUTTON_CHANGED) {
		NFOBJECT *slider;
		gint min_v = g_mot_prof.sensitivity_min;
		gint val;

		val = nfui_spin_button_get_index((NFSPINBUTTON*)obj);
		
		slider = (NFOBJECT*)nfui_nfobject_get_data(obj, "slider-obj");
		cw_slider_set_value((CWSLIDER*)slider, val + min_v);
		nfui_signal_emit(slider, GDK_EXPOSE, TRUE);

		set_data_from_obj(SENSITIVITY_PAGE);

#if defined(_SUPPORT_GUI_MDRAW)
        VW_MotionDraw_stop();
		VW_MotionDraw_set_selectArea(omd[g_msi.ch].area);
        VW_MotionDraw_set_sense(omd[g_msi.ch].cell_d.sense_d, omd[g_msi.ch].cell_d.sense_n);
        VW_MotionDraw_start();
#endif
		
	}
	return FALSE;
}

static gboolean post_mini_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case GDK_BUTTON_RELEASE :
		case NFEVENT_CWSLIDER_CHANGED_RELEASE :
			{
				NFOBJECT *spin;
				gint min_v = g_mot_prof.min_block;
				gint val;

				val = cw_slider_get_value((CWSLIDER*)obj);

				spin = (NFOBJECT*)nfui_nfobject_get_data(obj, "spin-obj");
				nfui_spin_button_set_index((NFSPINBUTTON*)spin, (guint)val - min_v);

				set_data_from_obj(MINI_BLOCK_PAGE);
			}
			break;

		default : 
			break;
	}
	return FALSE;
}

static gboolean post_mini_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_SPINBUTTON_CHANGED) {
		NFOBJECT *slider;
		gint min_v = g_mot_prof.min_block;
		gint val;

		val = nfui_spin_button_get_index((NFSPINBUTTON*)obj);
		
		slider = (NFOBJECT*)nfui_nfobject_get_data(obj, "slider-obj");
		cw_slider_set_value((CWSLIDER*)slider, val + min_v);
		nfui_signal_emit(slider, GDK_EXPOSE, TRUE);

		set_data_from_obj(MINI_BLOCK_PAGE);
	}
	return FALSE;
}

static gboolean post_intv_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case GDK_BUTTON_RELEASE :
		case NFEVENT_CWSLIDER_CHANGED_RELEASE :
			{
				NFOBJECT *spin;
				gint val;

				val = cw_slider_get_value((CWSLIDER*)obj);

				spin = (NFOBJECT*)nfui_nfobject_get_data(obj, "spin-obj");
				nfui_spin_button_set_index((NFSPINBUTTON*)spin, (guint)val-1);

//                set_data_from_obj(INTERVAL_PAGE);
			}
			break;

		default : 
			break;
	}

	return FALSE;
}

static gboolean post_intv_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_SPINBUTTON_CHANGED) {
		NFOBJECT *slider;
		gint val;

		val = nfui_spin_button_get_index((NFSPINBUTTON*)obj);
		
		slider = (NFOBJECT*)nfui_nfobject_get_data(obj, "slider-obj");
		cw_slider_set_value((CWSLIDER*)slider, val);
		nfui_signal_emit(slider, GDK_EXPOSE, TRUE);

//        set_data_from_obj(INTERVAL_PAGE);
	}

	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

    	if (memcmp(org_omd, omd, sizeof(OnvifMotionData)*GUI_CHANNEL_CNT))
    	{
			g_memmove(omd, org_omd, sizeof(OnvifMotionData)*GUI_CHANNEL_CNT);

			set_data_to_obj(TRUE);

#if defined(_SUPPORT_GUI_MDRAW)
            VW_MotionDraw_stop();
            VW_MotionDraw_set_selectArea(omd[g_msi.ch].area);
            VW_MotionDraw_set_daytime(omd[g_msi.ch].cell_d.time_start, omd[g_msi.ch].cell_d.time_end);
            VW_MotionDraw_set_sense(omd[g_msi.ch].cell_d.sense_d, omd[g_msi.ch].cell_d.sense_n);			
            VW_MotionDraw_start();
#endif
        }
 	}

	return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint i;
		
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
		if (check_motsen_block_cnt() == 0) return FALSE;
		//if (check_motsen_detect_object() == 0) return FALSE;

		if(memcmp(org_omd, omd, sizeof(OnvifMotionData)*GUI_CHANNEL_CNT))
		{
			set_mot_data();

			nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");

			syscam_set_changeflag(1);
		}
	}

	return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		if (check_motsen_block_cnt() == 0) return FALSE;
		//if (check_motsen_detect_object() == 0) return FALSE;

#if defined(_SUPPORT_GUI_MDRAW)
        VW_MotionDraw_stop();
#endif        
		stop_preview();

		IPCam_MotSen_tab_out_handler();
		SystemSetupCam_Destroy(obj);
	}

	return FALSE;
}

static gboolean post_time_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_SPINBUTTON_CHANGED) {
		gint st_i, et_i;
		
		st_i = nfui_spin_button_get_index((NFSPINBUTTON*)g_timeObj[0]);
		et_i = nfui_spin_button_get_index((NFSPINBUTTON*)g_timeObj[1]);

		if(st_i > et_i) {
			if(g_timeObj[0] == obj) 
				nfui_spin_button_set_index((NFSPINBUTTON*)g_timeObj[1], (guint)st_i);
			else if(g_timeObj[1] == obj) 
				nfui_spin_button_set_index((NFSPINBUTTON*)g_timeObj[0], (guint)et_i);
		}

		set_data_from_obj(DAYTIME_SET_PAGE);

#if defined(_SUPPORT_GUI_MDRAW)
        VW_MotionDraw_stop();
		VW_MotionDraw_set_selectArea(omd[g_msi.ch].area);
        VW_MotionDraw_set_daytime(omd[g_msi.ch].cell_d.time_start, omd[g_msi.ch].cell_d.time_end);
        VW_MotionDraw_start();
#endif
		
	}

	return FALSE;
}

static gboolean pre_page_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkGC *gc;
    GdkDrawable *drawable = NULL;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;		
    gint off_x, off_y;
		
	if(evt->type == GDK_EXPOSE) {
		drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
		gc = gdk_gc_new(drawable);

		nfui_nfobject_get_offset(obj, &off_x, &off_y);
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_TAB_SUB_GROUP_BG, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, obj->x+off_x, obj->y+off_y-40, -1, -1, NFALIGN_LEFT, 0);

		nfui_nfobject_gc_unref(gc);
	}
    else if (evt->type == GDK_DELETE)
    {
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_TAB_SUB_GROUP_BG, size_w, size_h);
    }

	return FALSE;
}


static gboolean daytime_set_page(NFOBJECT *page)
{
	NFOBJECT *obj;
	gint xpos;
	gint i;

	const gchar *strtimeTitle[] = { "00:00", "01:00", "02:00", "03:00", "04:00",
									"05:00", "06:00", "07:00", "08:00", "09:00",
									"10:00", "11:00", "12:00", "13:00", "14:00",
									"15:00", "16:00", "17:00", "18:00", "19:00",
									"20:00", "21:00", "22:00", "23:00" };

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DAYTIME SET", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nfobject_set_size(obj, 152, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(980));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);

	nfui_nffixed_put((NFFIXED*)page, obj, 10, 34);

	xpos = 176;
	for( i = 0 ; i < 2 ; i++ ) {
		if(i == 0) 
			obj = (NFOBJECT*)nfui_spinbutton_new((gchar**)strtimeTitle, 24, omd[0].cell_d.time_start);
		else
			obj = (NFOBJECT*)nfui_spinbutton_new((gchar**)strtimeTitle, 24, omd[0].cell_d.time_end);

		nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_SUBTAB_1);
		nfui_spin_button_set_align((NFSPINBUTTON*)obj, NFALIGN_LEFT, 2);
		nfui_spin_button_set_spacing((NFSPINBUTTON*)obj, CONDENSED_SPACING);	
		nfui_nfobject_set_size(obj, 130, 40);
		nfui_regi_post_event_callback(obj, post_time_event_cb);
		nfui_nfobject_show(obj);

		nfui_nffixed_put((NFFIXED*)page, obj, xpos, 34);

		xpos += 149;	
		
		g_timeObj[i] = obj;
	}

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("~", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nfobject_set_size(obj, 20, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(980));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)page, obj, 306, 34);

	return TRUE;
}

static gboolean sensitivity_set_page(NFOBJECT *page)
{
	NFOBJECT *obj;
	NFOBJECT *tmpObj;
	gchar *strSlider[] = {"DAYTIME", "NIGHTTIME"};
	gint i;


	for(i = 0; i < 2; i++)
	{
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strSlider[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));		
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(980));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_set_size(obj, 130, 40);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)page, obj, 10, 34 + (i * 44));


		tmpObj = (NFOBJECT*)cw_slider_new(50, 220, 40);
		//cw_slider_set_range((CWSLIDER*)tmpObj, 0, 10, 11); 
		cw_slider_set_range((CWSLIDER*)tmpObj, g_mot_prof.sensitivity_min, g_mot_prof.sensitivity_max, g_mot_prof.sensitivity_max+1); 
		nfui_nfobject_modify_bg(tmpObj, NFOBJECT_STATE_NORMAL, COLOR_IDX(980));
		nfui_nfobject_set_size(tmpObj, 220, 40);
		nfui_nfobject_show(tmpObj);
		nfui_regi_post_event_callback(tmpObj, post_sense_slider_event_handler);
		nfui_nffixed_put((NFFIXED*)page, tmpObj, (10 + 130 + 3), 34 + (i * 44));

		if(i == 0) 		cw_slider_set_value((CWSLIDER*)tmpObj, omd[0].cell_d.sense_d);
		else if(i == 1) cw_slider_set_value((CWSLIDER*)tmpObj, omd[0].cell_d.sense_n);

		//obj = nfui_spinbutton_new_with_range(50, 1, 10, 1);
		obj = nfui_spinbutton_new_value_with_range(0, g_mot_prof.sensitivity_min, g_mot_prof.sensitivity_max, 1);
		nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_SUBTAB_1);
		nfui_spin_button_set_align((NFSPINBUTTON*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_set_size(obj, 100, 40);
		nfui_nfobject_show(obj);
		nfui_regi_post_event_callback(obj, post_sense_spin_event_handler);
		nfui_nffixed_put((NFFIXED*)page, obj, (10 + 130 + 3 + 220 + 3), 34 + (i * 44));

		if(i == 0) 		nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, (guint)omd[0].cell_d.sense_d-g_mot_prof.sensitivity_min);
		else if(i == 1) nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, (guint)omd[0].cell_d.sense_n-g_mot_prof.sensitivity_min);

		nfui_nfobject_set_data(obj, "slider-obj", tmpObj);
		nfui_nfobject_set_data(tmpObj, "spin-obj", obj);

		g_sensObj[i] = obj;
	}

	return TRUE;
}

static gboolean minimum_block_set_page(NFOBJECT *page)
{
	NFOBJECT *obj;
	NFOBJECT *tmpObj;
	gchar *strSlider[] = {"DAYTIME", "NIGHTTIME"};

	gint i;

	for(i = 0; i < 2; i++)
	{
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strSlider[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));		
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(980));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_set_size(obj, 130, 40);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)page, obj, 10, 34 + (i * 44));


		tmpObj = (NFOBJECT*)cw_slider_new(50, 220, 40);
		//cw_slider_set_range((CWSLIDER*)tmpObj, 0, 10, 11); 
		cw_slider_set_range((CWSLIDER*)tmpObj, g_mot_prof.min_block, g_mot_prof.num_blocks, g_mot_prof.num_blocks+1); 
		nfui_nfobject_modify_bg(tmpObj, NFOBJECT_STATE_NORMAL, COLOR_IDX(980));
		nfui_nfobject_set_size(tmpObj, 220, 40);
		nfui_nfobject_show(tmpObj);
		nfui_regi_post_event_callback(tmpObj, post_mini_slider_event_handler);
		nfui_nffixed_put((NFFIXED*)page, tmpObj, (10 + 130 + 3), 34 + (i * 44));

		if(i == 0) 		cw_slider_set_value((CWSLIDER*)tmpObj, omd[0].cell_d.mini_d);
		else if(i == 1) cw_slider_set_value((CWSLIDER*)tmpObj, omd[0].cell_d.mini_n);

		//obj = nfui_spinbutton_new_with_range(0, 1, 10, 1);
		obj = nfui_spinbutton_new_value_with_range(0, g_mot_prof.min_block, g_mot_prof.num_blocks, 1);
		nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_SUBTAB_1);
		nfui_spin_button_set_align((NFSPINBUTTON*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_set_size(obj, 100, 40);
		nfui_nfobject_show(obj);
		nfui_regi_post_event_callback(obj, post_mini_spin_event_handler);
		nfui_nffixed_put((NFFIXED*)page, obj, (10 + 130 + 3 + 220 + 3), 34 + (i * 44));

		if(i == 0) 		nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, (guint)omd[0].cell_d.mini_d-g_mot_prof.min_block);
		else if(i == 1) nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, (guint)omd[0].cell_d.mini_n-g_mot_prof.min_block);

		nfui_nfobject_set_data(obj, "slider-obj", tmpObj);
		nfui_nfobject_set_data(tmpObj, "spin-obj", obj);

		g_minsObj[i] = obj;
	}

	return TRUE;
}

static gboolean interval_set_page(NFOBJECT *page)
{
	NFOBJECT *obj;
	NFOBJECT *tmpObj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("INTERVAL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));		
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(980));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 130, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)page, obj, 10, 34);


	tmpObj = (NFOBJECT*)cw_slider_new(50, 220, 40);
	cw_slider_set_range((CWSLIDER*)tmpObj, 0, 100, 101); 
	nfui_nfobject_modify_bg(tmpObj, NFOBJECT_STATE_NORMAL, COLOR_IDX(980));
	nfui_nfobject_set_size(tmpObj, 220, 40);
	nfui_nfobject_show(tmpObj);
	nfui_regi_post_event_callback(tmpObj, post_intv_slider_event_handler);
	nfui_nffixed_put((NFFIXED*)page, tmpObj, (10 + 130 + 3), 34);

	cw_slider_set_value((CWSLIDER*)tmpObj, omd[0].cell_d.interval);

	obj = nfui_spinbutton_new_value_with_range(50, 1, 100, 1);
	nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_SUBTAB_1);
	nfui_spin_button_set_align((NFSPINBUTTON*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, 100, 40);
	nfui_nfobject_show(obj);
	nfui_regi_post_event_callback(obj, post_intv_spin_event_handler);
	nfui_nffixed_put((NFFIXED*)page, obj, (10 + 130 + 3 + 220 + 3), 34);

	nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, (guint)omd[0].cell_d.interval-1);

	nfui_nfobject_set_data(obj, "slider-obj", tmpObj);
	nfui_nfobject_set_data(tmpObj, "spin-obj", obj);

	g_intvObj = obj;

	return TRUE;
}

void init_MotSenSetup_Page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *video_back_fixed;
	NFOBJECT *smart_fixed;
	NFOBJECT *tab_fixed;
	NFOBJECT *obj;
	NFOBJECT *tmpObj;
	gint temp = 0;

	const gchar *strImage_h[2] =  {
				(MKB_IMG_SUBTAB_DIR_H_N_116), 
				(MKB_IMG_SUBTAB_DIR_H_S_116)
	};
	const gchar *strTitle[4] = {
				"DAYTIME",
				"SENSITIVITY",
				"MIN BLOCK",
				"INTERVAL"
	};

	const guint colidx[3] = {COLOR_IDX(189), COLOR_IDX(188), COLOR_IDX(188)};

	gchar strCh[STRING_SIZE_CAMTITLE+8];
	gchar *strSlider[] = {"SENSITIVITY", "INTERVAL", "THRESHOLD"};

	GdkColor select_color[4] = {UX_COLOR(734),	
								UX_COLOR(734),	
								UX_COLOR(734),	
								UX_COLOR(734)};
	gint off_x, off_y;
	guint pos_x, pos_y;
	guint i = 0, j = 0;

	gint btn_w, btn_h;

	GSList *slist = NULL;
	GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];

	gchar lfBuf[1024];

	gchar strBuf[64];
	gchar *token_ptr;
	gchar tempbuf[256];
	gchar *pbuf = NULL;
	gchar *pnext = NULL;


	radio_img[0] = nfui_get_image_from_file((IMG_N_SUBTAB_RADIO_OFF), NULL);
	radio_img[1] = nfui_get_image_from_file((IMG_O_SUBTAB_RADIO_ON), NULL);
	radio_img[2] = nfui_get_image_from_file((IMG_P_SUBTAB_RADIO_ON), NULL);
	radio_img[3] = nfui_get_image_from_file((IMG_D_SUBTAB_RADIO_OFF), NULL);

	init_all_data();
	
	_set_mot_area_width(0);
	//g_message("%s(%d) => %d", __FUNCTION__, __LINE__, omd[0].smart_motion);
#if defined(_SUPPORT_GUI_MDRAW)
	VW_MotionDraw_init(parent);
#endif

	g_curwnd = (NFWINDOW*)nfui_nfobject_get_top(parent);
	nfui_regi_post_event_callback(parent, post_page_event_handler);


	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);



	// ch combobox 
	pos_x = 27;
	//pos_y = 54;
	pos_y = 40;


	obj = nfui_combobox_new(NULL, 0, 0);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_SUBTAB_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, 290, 40); 
	nfui_nfobject_show(obj); 
	nfui_regi_post_event_callback(obj, post_ch_combo_event_handler);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

	for (i = 0; i<GUI_CHANNEL_CNT; i++)
	{
		memset(strCh, 0, (STRING_SIZE_CAMTITLE+8));
		j = sprintf(strCh, "%s%d - ", lookup_string("CH"), i+1);
		//DAL_get_camera_title(&strCh[j], (guint)i);
		var_get_camtitle(&strCh[j], (guint)i);

		nfui_combobox_append_data((NFCOMBOBOX*)obj, strCh);
	}

	uxm_reg_imsg_event(obj, INFY_CAMDB_CHANGE_NOTIFY);
	uxm_monitor_on_imsg_event(obj, INFY_CAMDB_CHANGE_NOTIFY);
	uxm_reg_imsg_event(obj, INFY_USRDB_CHANGE_NOTIFY);
	uxm_monitor_on_imsg_event(obj, INFY_USRDB_CHANGE_NOTIFY);
	
	// preview label
	pos_y = 160;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PREVIEW", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 160, 40); 
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);


	// background fixed
	pos_y += 40;

	video_back_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(video_back_fixed, NFOBJECT_STATE_NORMAL, VIDEO_BG_COLOR);
	nfui_nfobject_set_size(video_back_fixed, MOTION_AREA_BACK_SIZE_W, MOTION_AREA_BACK_SIZE_H);
	nfui_nfobject_show(video_back_fixed);
	nfui_nffixed_put((NFFIXED*)content_fixed, video_back_fixed, pos_x, pos_y);
	g_video_back_fixed = video_back_fixed;
	// motion area

	if(g_msi.area_rows == 0 || g_msi.area_cols == 0)
		obj = nfui_tile_new(1, 1);
	else
		obj = nfui_tile_new(g_msi.area_rows, g_msi.area_cols);
	nfui_tile_set_fill(NF_TILE(obj), FALSE);
	nfui_tile_set_line_border(NF_TILE(obj), 2);
	nfui_tile_set_line_color(NF_TILE(obj), NFTILE_STATE_NORMAL, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_000000))); 
	nfui_tile_set_line_color(NF_TILE(obj), NFTILE_STATE_FOCUS, &UX_COLOR(733)); 
	nfui_tile_set_line_color(NF_TILE(obj), NFTILE_STATE_SELECT, select_color);
	nfui_tile_set_drawable_outline(NF_TILE(obj), FALSE);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
	nfui_nfobject_set_size(obj, g_mot_area_w, MOTION_AREA_BACK_SIZE_H);
	nfui_regi_post_event_callback(obj, post_tile_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)video_back_fixed, obj, (MOTION_AREA_BACK_SIZE_W-g_mot_area_w)/2, 0);
	g_area = obj;

    pos_x += MENU_V_SUBTAB_FIXED_X+MENU_V_SUBTAB_PAGE_X+MENU_V_SUBTAB_INNER_X + (MOTION_AREA_BACK_SIZE_W-g_mot_area_w)/2;
    pos_y += MENU_V_SUBTAB_FIXED_Y+MENU_V_SUBTAB_PAGE_Y+MENU_V_SUBTAB_INNER_Y;
	
#if defined(_SUPPORT_GUI_MDRAW)
	VW_MotionDraw_set_obj(0, obj);
	VW_MotionDraw_set_plt_position(pos_x, pos_y, g_mot_area_w, MOTION_AREA_BACK_SIZE_H);
    VW_MotionDraw_set_selectArea(omd[0].area);
    VW_MotionDraw_set_daytime(omd[0].cell_d.time_start, omd[0].cell_d.time_end);
    VW_MotionDraw_set_sense(omd[0].cell_d.sense_d, omd[0].cell_d.sense_n);    
#endif


	/****************************************************************************************
	 *
	 * TILE AREA SETUP 
	 *
	 ****************************************************************************************/ 


	pos_x = 27;
	pos_x += 1010;

	g_setup_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(g_setup_fixed, MOTION_SETUP_FIXED_W, 600);
	//nfui_nfobject_modify_bg(g_setup_fixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_00FF00));
	nfui_nfobject_modify_bg(g_setup_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_show(g_setup_fixed);
	nfui_nffixed_put((NFFIXED*)content_fixed, g_setup_fixed, pos_x, 200);
	nfui_nfobject_show(g_setup_fixed);


	// setup fixed 
	smart_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(smart_fixed, MOTION_SETUP_FIXED_W, MOTION_SMART_FIXED_H);
	//nfui_nfobject_modify_bg(smart_fixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_FF0000));
	nfui_nfobject_modify_bg(smart_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_show(smart_fixed);
	nfui_nffixed_put((NFFIXED*)g_setup_fixed, smart_fixed, 0, 20);
	nfui_nfobject_show(smart_fixed);
	g_smart_fixed = smart_fixed;

	nfui_get_pixbuf_size(radio_img[0], &btn_w, &btn_h);

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
	nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
	nfui_nfobject_set_size(obj, (guint)btn_w, (guint)btn_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)smart_fixed, obj, 8+0, 0+(40-btn_h)/2);
	nfui_regi_post_event_callback(obj, post_smart_motion_onoff_event_handler);    
	g_classic_motion_obj = obj;

	if (!omd[0].smart_motion) nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);
	slist = nfui_radio_button_get_group(NF_BUTTON(obj));

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("MOTION", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 280, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)smart_fixed, obj, 8+40, 0);
    nfui_nfobject_show(obj);

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
	nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
	nfui_nfobject_set_size(obj, (guint)btn_w, (guint)btn_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)smart_fixed, obj, 8+0, 0+41+(40-btn_h)/2);
	nfui_regi_post_event_callback(obj, post_smart_motion_onoff_event_handler);    
	g_smart_motion_obj = obj;

	if (omd[0].smart_motion) nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);
	nfui_radio_button_add_group(NF_BUTTON(obj), slist);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("AI MOTION", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 280, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)smart_fixed, obj, 8+40, 41);
    nfui_nfobject_show(obj);

	for (i=0; i<MOTION_OBJ_CNT; i++)
	{
    obj = nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_SUBTAB_NORMAL);
    nfui_check_get_size(obj, &btn_w, &btn_h);
    nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)smart_fixed, obj, 30, 41*2 + 40*i);
		nfui_regi_post_event_callback(obj, post_smart_motion_event_handler);    
		g_smart_motion_chk[i] = obj;

		// g_message("%s, %d, interest:%s", __FUNCTION__, __LINE__, omd[0].ipcam_d.smart_interest_obj);

		if (omd[0].ipcam_d.smart_motion_options[i].enable) nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)obj, TRUE);
		else nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)obj, FALSE);

		if (omd[0].smart_motion) nfui_nfobject_enable(obj);
    	else nfui_nfobject_disable(obj);

		if (i == MOTION_OBJ_PERSON) obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("person", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
		else if (i == MOTION_OBJ_CAR) obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("car", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
		else if (i == MOTION_OBJ_BIKE) obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("bike", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_set_size(obj, 130, 40);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nffixed_put((NFFIXED*)smart_fixed, obj, 30+40, 41*2 + 40*i);
		nfui_nfobject_show(obj); 
		g_smart_motion_label[i] = obj;

		obj = nfui_spinbutton_new_value_with_range(1, 1, 100, 1);
		nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_SUBTAB_1);
		nfui_spin_button_set_align((NFSPINBUTTON*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_set_size(obj, 100, 40);
		nfui_nfobject_show(obj);
		nfui_regi_post_event_callback(obj, post_smart_motion_event_handler);
		nfui_nffixed_put((NFFIXED*)smart_fixed, obj, 30+40+180, 41*2 + 40*i);
		if (g_mot_prof.smart_motion_support) nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, (guint)omd[0].ipcam_d.smart_motion_options[i].threshold - 1);
		g_smart_motion_spin[i] = obj;

	if (omd[0].smart_motion) nfui_nfobject_enable(obj);
	else nfui_nfobject_disable(obj);

		obj = nfui_nflabel_new_with_pango_font("[1 .. 100]", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_set_size(obj, 100, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nffixed_put((NFFIXED*)smart_fixed, obj, 30+40+180+105, 41*2 + 40*i);
    nfui_nfobject_show(obj); 
	}

	memset(tempbuf, 0x00, 256);
	memcpy(tempbuf, omd[0].ipcam_d.smart_interest_obj, 256);

	token_ptr = strtok_r(tempbuf, ",", &pnext);
	pbuf = pnext;
	while (token_ptr != 0) {
		temp += 1;
		token_ptr = strtok_r(pbuf, ",", &pnext);
		pbuf = pnext;
	}

	if(temp == 1)
	{
		for(i=0; i<MOTION_OBJ_CNT; i++)
		{
			nfui_nfobject_disable(g_smart_motion_chk[i]);
			nfui_nfobject_disable(g_smart_motion_spin[i]);
		}
		
		ai_disable[0] = 1;
	}

#if 0 // skshin
    obj = nfui_checkbutton_new(omd[0].use_ai_alarmevt);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_SUBTAB_NORMAL);
    nfui_check_get_size(obj, &btn_w, &btn_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)smart_fixed, obj, 8, 41*3+20+(40-btn_h)/2);
	nfui_regi_post_event_callback(obj, post_rcv_motion_alarmevt_event_handler);  
	g_ai_alarmevt_obj = obj;

	nfutil_get_line_feed_string(lookup_string("Receive alarm when object is detected through AI analysis"), 390, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), lfBuf, sizeof(lfBuf));

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(lfBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 400, 60);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	//nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_00FF00));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)smart_fixed, obj, 8+40, 41*3+20);
    nfui_nfobject_show(obj);
#endif


	// setup fixed 

	tab_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(tab_fixed, MOTION_SETUP_FIXED_W, MOTION_SETUP_FIXED_H);
	nfui_nfobject_modify_bg(tab_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_show(tab_fixed);
	nfui_nffixed_put((NFFIXED*)g_setup_fixed, tab_fixed, 0, 300);
	nfui_nfobject_show(tab_fixed);
	g_tab_fixed = tab_fixed;


    g_tab = (NFOBJECT*)nfui_nftab_new(SUBPAGE_CNT, strImage_h, 116, 43, NFTAB_DIR_H, strTitle, colidx);
	nfui_nftab_set_pango_font((NFTAB*)g_tab, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nftab_set_margin((NFTAB*)g_tab, 2);
	nfui_nfobject_show(g_tab);
	nfui_nffixed_put((NFFIXED*)tab_fixed, g_tab, 0, 0);

	for(i=0; i<SUBPAGE_CNT; i++) {
		g_tabPage[i] = (NFOBJECT*)nfui_nffixed_new();
		nfui_nfobject_set_size(g_tabPage[i], MOTION_SETUP_FIXED_W, 220);
		nfui_nftab_regi_page((NFTAB*)g_tab, g_tabPage[i], i);
		nfui_nffixed_put((NFFIXED*)tab_fixed, g_tabPage[i], 0, 40);

		nfui_regi_pre_event_callback(g_tabPage[i], pre_page_event_cb);
	}

	daytime_set_page(g_tabPage[DAYTIME_SET_PAGE]);
	sensitivity_set_page(g_tabPage[SENSITIVITY_PAGE]);
	minimum_block_set_page(g_tabPage[MINI_BLOCK_PAGE]);
//    interval_set_page(g_tabPage[INTERVAL_PAGE]);

	if(var_get_vendor_code() == 108) {		// ASP
		nfui_nftab_unregi_page((NFTAB*)g_tab, MINI_BLOCK_PAGE);
	}

    nfui_nfobject_show(g_tabPage[DAYTIME_SET_PAGE]);

	// button
	obj = nftool_normal_button_create_type3("SELECT ALL", 192);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_regi_post_event_callback(obj, post_select_all_area_event_handler);
	nfui_nfobject_show(obj);
	g_btns[BTNS_SELECT_ALL] = obj;
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, 27, 815);

	obj = nftool_normal_button_create_type3("DESELECT ALL", 192);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_regi_post_event_callback(obj, post_deselect_all_area_event_handler);
	nfui_nfobject_show(obj);
	g_btns[BTNS_DESELECT_ALL] = obj;
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, 27 + 202, 815);



	obj = (NFOBJECT*)nftool_normal_button_create_type1("CANCEL", 192);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R3_X, MENU_V_SUBTAB_BTN_Y);

	obj = (NFOBJECT*)nftool_normal_button_create_type1("APPLY", 192);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_regi_post_event_callback(obj, post_applybutton_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R2_X, MENU_V_SUBTAB_BTN_Y);

	obj = (NFOBJECT*)nftool_normal_button_create_type2("CLOSE", 192);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R1_X, MENU_V_SUBTAB_BTN_Y);

	switch(g_msi.area_method) {
		case MAM_NONE: 		
			disable_setup();
			break;

		case MAM_RECTANGLE:
		case MAM_CELL:
			disable_mini_tab();
			break;

		case MAM_RAW_STREAM:
			break;
		/*
		case MAM_CELL: 		
		nfui_nfobject_hide(g_area[RECT]); 
		break;
		case MAM_RECTANGLE: 
		nfui_nfobject_hide(g_area[CELL]); 
		break;
		*/

		default:	break;
	}

	// g_message("%s, %d, supp_smart_motion:%d", __FUNCTION__, __LINE__, g_mot_prof.smart_motion_support);

	if (g_mot_prof.smart_motion_support) _config_smart_motion_fixed(1, 0);
	else _config_smart_motion_fixed(0, 0);

	_set_warning_msg(FALSE);
}

// TEMPORARY API
void MotSen_Show_Preview()
{
	show_preview(g_msi.ch);
#if defined(_SUPPORT_GUI_MDRAW)	
	VW_MotionDraw_set_selectArea(omd[g_msi.ch].area);
    VW_MotionDraw_start();	
#endif    
}

void MotSen_Pause_Preview()
{
#if defined(_SUPPORT_GUI_MDRAW)
    VW_MotionDraw_stop();
#endif    
	pause_preview();
}

void MotSen_Stop_Preview()
{
#if defined(_SUPPORT_GUI_MDRAW)
    VW_MotionDraw_stop();
#endif    
	stop_preview();
}

gboolean check_motsen_setup_data_changed()
{
	gint i;
	if(!memcmp(org_omd, omd, sizeof(OnvifMotionData)*GUI_CHANNEL_CNT))
	{
		return FALSE;
	}

	for(i=0; i<GUI_CHANNEL_CNT; i++)
	{
		printf_all_data(i);
	}

	return TRUE;
}

void save_motsen_setup_data_changed()
{
	set_mot_data();
}

void restore_motsen_setup_data_changed()
{
	g_memmove(omd, org_omd, sizeof(OnvifMotionData)*GUI_CHANNEL_CNT);

	set_data_to_obj(FALSE);

#if defined(_SUPPORT_GUI_MDRAW)
    VW_MotionDraw_set_selectArea(omd[g_msi.ch].area);
    VW_MotionDraw_set_daytime(omd[g_msi.ch].cell_d.time_start, omd[g_msi.ch].cell_d.time_end);
    VW_MotionDraw_set_sense(omd[g_msi.ch].cell_d.sense_d, omd[g_msi.ch].cell_d.sense_n);
#endif		
}

gint check_motsen_block_cnt()
{
	if (_get_tile_cell_cnt() < _get_minium_block_cnt()) {
		nftool_mbox(g_curwnd, "WARNING", 
			"If the motion sensor area is set to be fewer blocks than the minimum required,\nmotion detection does not register.", 
			NFTOOL_MB_OK);
		return 0;
	}
	
	return 1;
}

gint check_motsen_detect_object()
{
	if ((omd[g_msi.ch].smart_motion == 1) && (strlen(omd[g_msi.ch].ipcam_d.smart_interest_obj) == 0)) {
		nftool_mbox(g_curwnd, "NOTICE", "Please select at least one object.", NFTOOL_MB_OK);
		return 0;
	}
	
	return 1;
}
