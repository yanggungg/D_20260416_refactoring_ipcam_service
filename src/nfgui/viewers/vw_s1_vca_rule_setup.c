
#include "nf_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "objects/nffixed.h"
#include "objects/nflabel.h"
#include "objects/nftable.h"
#include "objects/nftab.h"
#include "objects/nfbutton.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfspinbutton.h"
#include "objects/nfimage.h"
#include "objects/nfcombobox.h"

#include "vw_internal.h"
#include "vw_s1_vca_setup_main.h"
#include "vw_s1_vca_internal.h"
#include "vaa_s1.h"
#include "vaa.h"


#define MAX_USE_RULE_CNT	3

#define EXPLAN_INVASION 	"EXPLAN_INVASION"
#define EXPLAN_LOITERING 	"EXPLAN_LOITERING"
#define EXPLAN_ABANDON	 	"EXPLAN_ABANDON"
#define EXPLAN_STEAL 		"EXPLAN_STEAL"
#define EXPLAN_TOPPLE 		"EXPLAN_TOPPLE"
#define EXPLAN_FENCE 		"EXPLAN_FENCE"
#define EXPLAN_COUNT 		"EXPLAN_COUNT"
#define EXPLAN_TAMPERING 	"EXPLAN_TAMPERING"
#define EXPLAN_PRIVACY 		"EXPLAN_PRIVACY"


static NFOBJECT *g_setup_fixed = 0;
static NFOBJECT *g_rule_fixed[VCA_S1_RULE_MAX] = {0, };
static NFOBJECT *g_ruleSched_fixed = 0;

static NFOBJECT *g_useRule_obj[VCA_S1_RULE_MAX] = {0, };
static NFOBJECT *g_dispSchedObj[MAX_USE_RULE_CNT][2] = {0, };
static gint g_cur_ch = 0;

static NFOBJECT *g_invasion_obj[2] = {0, };
static NFOBJECT *g_loitering_obj[2] = {0, };
static NFOBJECT *g_abandon_obj[2] = {0, };
static NFOBJECT *g_steal_obj[2] = {0, };
static NFOBJECT *g_topple_obj[2] = {0, };

//static int invasion_sens[6] = { 0, 1, 2, 3, 4, 5 };
static int invasion_sens[5] = { 1, 2, 3, 4, 5 };
static int loitering_time[6] = { 5, 10, 20, 30, 60, 120 };
static int abandon_time[9] = { 1, 2, 3, 5, 10, 15, 20, 30, 60 };
static int steal_time[9] = { 1, 2, 3, 5, 10, 15, 20, 30, 60 };
//static int topple_sens[6] = { 0, 1, 2, 3, 4, 5 };
static int topple_sens[5] = { 1, 2, 3, 4, 5 };

static int _find_invasion_sens_idx(int sens)
{
	int i;
	for (i = 0; i < 5; ++i) {
		if (invasion_sens[i] == sens) return i;
	}

	return 0;
}

static int _find_loitering_time_idx(int time)
{
	int i;
	for (i = 0; i < 6; ++i) {
		if (loitering_time[i] == time) return i;
	}

	return 0;
}

static int _find_abandon_time_idx(int time)
{
	int i;
	for (i = 0; i < 9; ++i) {
		if (abandon_time[i] == time) return i;
	}

	return 0;
}

static int _find_steal_time_idx(int time)
{
	int i;
	for (i = 0; i < 9; ++i) {
		if (steal_time[i] == time) return i;
	}

	return 0;
}

static int _find_topple_sens_idx(int sens)
{
	int i;
	for (i = 0; i < 5; ++i) {
		if (topple_sens[i] == sens) return i;
	}

	return 0;
}

static int _update_widgets(gint ch)
{
	VAAID vaaid = 0;
	VARULE_CONF	confs[16];
	gint confcnt;
	gint i, idx;

	vaaid = vaa_get_vaaid(ch);
	memset(confs, 0x00, sizeof(confs));
	vaa_s1_get_rule_confs_all(vaaid, confs, &confcnt);
	
	for (i = 0; i < VCA_S1_RULE_MAX; i++)
		nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_useRule_obj[i]), confs[i].use_rule);

	idx = _find_invasion_sens_idx(confs[VCA_S1_RULE_INVASION].cfg_sens);
	nfui_combobox_set_index_no_expose(NF_COMBOBOX(g_invasion_obj[1]), idx);

	idx = _find_loitering_time_idx(confs[VCA_S1_RULE_LOITERING].cfg_time);
	nfui_combobox_set_index_no_expose(NF_COMBOBOX(g_loitering_obj[1]), idx);

	idx = _find_abandon_time_idx(confs[VCA_S1_RULE_ABANDON].cfg_time);
	nfui_combobox_set_index_no_expose(NF_COMBOBOX(g_abandon_obj[1]), idx);

	idx = _find_steal_time_idx(confs[VCA_S1_RULE_STEAL].cfg_time);
	nfui_combobox_set_index_no_expose(NF_COMBOBOX(g_steal_obj[1]), idx);

	idx = _find_topple_sens_idx(confs[VCA_S1_RULE_TOPPLE].cfg_sens);
	nfui_combobox_set_index_no_expose(NF_COMBOBOX(g_topple_obj[1]), idx);
	return 0;
}

static gint _init_capable_invasion_sensitivity(NFOBJECT *obj)
{
	gchar strBuf[16];
	gint i;

	memset(strBuf, 0x00, sizeof(strBuf));

	for (i = 0; i < 5; i++)
	{
		g_sprintf(strBuf, "%d", invasion_sens[i]);
		nfui_combobox_append_data((NFCOMBOBOX*)obj, strBuf);
	}
	return 0;
}

static gint _init_capable_loitering_time(NFOBJECT *obj)
{
	int i;
	gchar strBuf[16];

	memset(strBuf, 0x00, sizeof(strBuf));
	
	for (i = 0; i < 6; ++i) {
	    g_sprintf(strBuf, g_sec_str[0], loitering_time[i]);
		nfui_combobox_append_data((NFCOMBOBOX*)obj, strBuf);
	}
	return 0;
}

static gint _init_capable_abandon_time(NFOBJECT *obj)
{
	int i;
	gchar strBuf[16];

	memset(strBuf, 0x00, sizeof(strBuf));
	
	for (i = 0; i < 9; ++i) {
	    g_sprintf(strBuf, g_sec_str[0], abandon_time[i]);
		nfui_combobox_append_data((NFCOMBOBOX*)obj, strBuf);
	}
	return 0;
}

static gint _init_capable_steal_time(NFOBJECT *obj)
{
	int i;
	gchar strBuf[16];

	memset(strBuf, 0x00, sizeof(strBuf));
	
	for (i = 0; i < 9; ++i) {
	    g_sprintf(strBuf, g_sec_str[0], steal_time[i]);
		nfui_combobox_append_data((NFCOMBOBOX*)obj, strBuf);
	}
	return 0;
}

static gint _init_capable_topple_sensitivity(NFOBJECT *obj)
{
	gchar strBuf[16];
	gint i;

	memset(strBuf, 0x00, sizeof(strBuf));

	for (i = 0; i < 5; i++)
	{
		g_sprintf(strBuf, "%d", topple_sens[i]);
		nfui_combobox_append_data((NFCOMBOBOX*)obj, strBuf);
	}
	return 0;
}

static gint _set_disp_rule_sched(gint ch)
{
	VAAID vaaid = 0;
	VARULE_CONF	confs[16];
	gint confcnt;
	gint i, cnt = 0;

	const gchar *rule_image[VCA_S1_RULE_MAX] = {IMG_VA_ICON_01_S, IMG_VA_ICON_02_S, IMG_VA_ICON_07_S, IMG_VA_ICON_08_S, 
			IMG_VA_ICON_09_S, IMG_VA_ICON_03_S, IMG_VA_ICON_04_S, IMG_VA_ICON_06_S, IMG_VA_ICON_10_S};	
	const gchar *rule_name[VCA_S1_RULE_MAX] = {STR_RULE_INVASION, STR_RULE_LOITERING, STR_RULE_ABANDON, 
			STR_RULE_STEAL, STR_RULE_TOPPLE, STR_RULE_FENCE, STR_RULE_COUNT, STR_RULE_TAMPERING, STR_RULE_PRIVACY};

	vaaid = vaa_get_vaaid(ch);
	memset(confs, 0x00, sizeof(confs));
	vaa_s1_get_rule_confs_all(vaaid, confs, &confcnt);

	for (i = 0; i < VCA_S1_RULE_MAX; i++)
	{
		if (confs[i].use_rule)
		{
			nfui_nfimage_change_image((NFIMAGE*)g_dispSchedObj[cnt][0], rule_image[i]);
			nfui_nfimage_set_text((NFIMAGE*)g_dispSchedObj[cnt][1], rule_name[i]);		
			nfui_nfobject_show(g_dispSchedObj[cnt][0]);
			nfui_nfobject_show(g_dispSchedObj[cnt][1]);			
			cnt++;
		}
	}

	if (cnt < 3)
	{
		for (i = 2; i >= cnt; i--)
		{
			nfui_nfobject_hide(g_dispSchedObj[i][0]);
			nfui_nfobject_hide(g_dispSchedObj[i][1]);
		}
	}

	return 0;
}

static gboolean post_use_rule_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch (evt->type) 
	{
		case NFEVENT_CHECKBUTTON_CHANGED:
		{
			VAAID vaaid = vaa_get_vaaid(g_cur_ch);
			VARULE_CONF	conf, confs[16];
			gint confcnt;

			gboolean active;
			gint i;
			gint cnt = 0;
			NFWINDOW *curwnd = 0;
			NFOBJECT *fixed = 0;
			
			memset(&conf, 0x00, sizeof(conf));			
			memset(confs, 0x00, sizeof(confs));
			
			vaaid = vaa_get_vaaid(g_cur_ch);	
			active = nfui_check_button_get_active((NFCHECKBUTTON*)obj);

			if (active) 
			{
				vaa_s1_get_rule_confs_all(vaaid, confs, &confcnt);
			
				for (i = 0; i < VCA_S1_RULE_MAX; i++)
				{
					if (confs[i].use_rule) cnt++;
				}

				if (cnt+1 > 3)
				{
					curwnd = nfui_nfobject_get_top(obj);
					nftool_mbox(curwnd, "ERROR", "Only 3 rules are avaiable.", NFTOOL_MB_OK);
					nfui_check_button_set_active(obj, FALSE);
					nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
					return FALSE;
				}			
			}

			for (i = 0; i < VCA_S1_RULE_MAX; i++)
			{
				if (g_useRule_obj[i] == obj) break;
			}

			vaa_s1_get_rule_conf(vaaid, i, &conf);
			conf.use_rule = active;
			vaa_s1_set_rule_conf(vaaid, i, &conf);

			fixed = _get_S1_VCA_ruleSelect_fixed();
			_S1_VCA_RuleSelect_update();
			nfui_signal_emit(fixed, GDK_EXPOSE, TRUE);
        }
		break;

		default:
		break;		
	}	

	return FALSE;
}

static gboolean post_invasion_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_COMBOBOX_CHANGED) 
	{
		VAAID vaaid = vaa_get_vaaid(g_cur_ch);
		VARULE_CONF	conf;
		gint confcnt;
		gint idx = nfui_combobox_get_cur_index(obj);

		vaaid = vaa_get_vaaid(g_cur_ch);
		memset(&conf, 0x00, sizeof(conf));
		vaa_s1_get_rule_conf(vaaid, VCA_S1_RULE_INVASION, &conf);
		conf.cfg_sens = invasion_sens[idx];
		vaa_s1_set_rule_conf(vaaid, VCA_S1_RULE_INVASION, &conf);
	}

	return FALSE;
}

static gboolean post_loitering_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_COMBOBOX_CHANGED) 
	{
		VAAID vaaid = vaa_get_vaaid(g_cur_ch);
		VARULE_CONF	conf;
		gint confcnt;
		gint idx = nfui_combobox_get_cur_index(obj);

		vaaid = vaa_get_vaaid(g_cur_ch);
		memset(&conf, 0x00, sizeof(conf));
		vaa_s1_get_rule_conf(vaaid, VCA_S1_RULE_LOITERING, &conf);		
		conf.cfg_time = loitering_time[idx];
		vaa_s1_set_rule_conf(vaaid, VCA_S1_RULE_LOITERING, &conf);
	}

	return FALSE;
}

static gboolean post_abandon_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_COMBOBOX_CHANGED) 
	{
		VAAID vaaid = vaa_get_vaaid(g_cur_ch);
		VARULE_CONF	conf;
		gint confcnt;
		gint idx = nfui_combobox_get_cur_index(obj);

		vaaid = vaa_get_vaaid(g_cur_ch);
		memset(&conf, 0x00, sizeof(conf));
		vaa_s1_get_rule_conf(vaaid, VCA_S1_RULE_ABANDON, &conf);		
		conf.cfg_time = abandon_time[idx];
		vaa_s1_set_rule_conf(vaaid, VCA_S1_RULE_ABANDON, &conf);
	}

	return FALSE;
}

static gboolean post_steal_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_COMBOBOX_CHANGED) 
	{
		VAAID vaaid = vaa_get_vaaid(g_cur_ch);
		VARULE_CONF	conf;
		gint confcnt;
		gint idx = nfui_combobox_get_cur_index(obj);

		vaaid = vaa_get_vaaid(g_cur_ch);
		memset(&conf, 0x00, sizeof(conf));
		vaa_s1_get_rule_conf(vaaid, VCA_S1_RULE_STEAL, &conf);		
		conf.cfg_time = steal_time[idx];
		vaa_s1_set_rule_conf(vaaid, VCA_S1_RULE_STEAL, &conf);
	}

	return FALSE;
}

static gboolean post_topple_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_COMBOBOX_CHANGED) 
	{
		VAAID vaaid = vaa_get_vaaid(g_cur_ch);
		VARULE_CONF	conf;
		gint confcnt;
		gint idx = nfui_combobox_get_cur_index(obj);

		vaaid = vaa_get_vaaid(g_cur_ch);
		memset(&conf, 0x00, sizeof(conf));
		vaa_s1_get_rule_conf(vaaid, VCA_S1_RULE_TOPPLE, &conf);		
		conf.cfg_sens = topple_sens[idx];
		vaa_s1_set_rule_conf(vaaid, VCA_S1_RULE_TOPPLE, &conf);
	}

	return FALSE;
}

static gboolean post_setup_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint i;

	if (evt->type == GDK_DELETE) 
	{
		for (i = 0; i < VCA_S1_RULE_MAX; i++) {
			g_rule_fixed[i] = 0;
		}
	}	

	return FALSE;
}

static gboolean post_rule_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;
	gint i;

	switch (evt->type) 
	{
		case GDK_EXPOSE:
		{
            gint gap_x, gap_y;

    		drawable = nfui_nfobject_get_window(obj);
            gc = gdk_gc_new(drawable);

            nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);
            nfui_nfobject_get_size(obj, &size_w, &size_h);
            pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_SUB_GROUP_BG, size_w, 225);
            nfutil_draw_pixbuf(drawable, gc, pbuf, gap_x, gap_y+50, size_w, 225, NFALIGN_LEFT, 0);
        
    		nfui_nfobject_gc_unref(gc);
    	}
		break;

		case GDK_DELETE:
		{
            nfui_nfobject_get_size(obj, &size_w, &size_h);
            nfui_unref_popup_pixbuf(MK_IMG_SUB_GROUP_BG, size_w, size_h);
        }
		break;

		default:
		break;
	}	

	return FALSE;
}

static gboolean post_ruleSched_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch (evt->type) 
	{
		case GDK_EXPOSE:
		{
          
    	}
		break;

		case GDK_DELETE:
		{

        }
		break;

		default:
		break;		
	}	

	return FALSE;
}

NFOBJECT* _S1_VCA_RuleSetup_Page(NFOBJECT *parent, gint page_x, gint page_y, gint page_w, gint page_h)
{
	NFOBJECT *fixed;
	NFOBJECT *obj;
	const gchar *rule_image[VCA_S1_RULE_MAX] = {IMG_VA_ICON_01_S, IMG_VA_ICON_02_S, IMG_VA_ICON_07_S, IMG_VA_ICON_08_S, 
			IMG_VA_ICON_09_S, IMG_VA_ICON_03_S, IMG_VA_ICON_04_S, IMG_VA_ICON_06_S, IMG_VA_ICON_10_S};	
	const gchar *rule_name[VCA_S1_RULE_MAX] = {STR_RULE_INVASION, STR_RULE_LOITERING, STR_RULE_ABANDON, 
			STR_RULE_STEAL, STR_RULE_TOPPLE, STR_RULE_FENCE, STR_RULE_COUNT, STR_RULE_TAMPERING, STR_RULE_PRIVACY};
	const gchar *rule_explan[VCA_S1_RULE_MAX] = {EXPLAN_INVASION, EXPLAN_LOITERING, EXPLAN_ABANDON, 
			EXPLAN_STEAL, EXPLAN_TOPPLE, EXPLAN_FENCE, EXPLAN_COUNT, EXPLAN_TAMPERING, EXPLAN_PRIVACY};			
	gint i;

	g_cur_ch = 0;

	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(fixed, page_w, page_h);
	nfui_nfobject_show(fixed);
	nfui_nffixed_put((NFFIXED*)parent, fixed, (guint)page_x, (guint)page_y);
	nfui_regi_post_event_callback(fixed, post_setup_fixed_event_handler);	
	g_setup_fixed = fixed;

	for (i = 0; i < VCA_S1_RULE_MAX; i++)
	{
		fixed = (NFOBJECT*)nfui_nffixed_new();
		nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
		nfui_nfobject_set_size(fixed, page_w, page_h);
		nfui_nfobject_hide(fixed);
		nfui_nffixed_put((NFFIXED*)g_setup_fixed, fixed, 0, 0);
		nfui_regi_post_event_callback(fixed, post_rule_fixed_event_handler);	
		g_rule_fixed[i] = fixed;		

	    obj = (NFOBJECT*)nfui_nfimage_new(rule_image[i]);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed, obj, 0, 0);

		obj = (NFOBJECT*)nfui_nfimage_new(IMG_VASET_TITLE_BAR);
		nfui_nfimage_set_text((NFIMAGE*)obj, rule_name[i]);
		nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
		nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 16);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed, obj, 40, 0);

		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(rule_explan[i], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(122));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_set_size(obj, 390, 142);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
		nfui_nfobject_show(obj);	
		nfui_nffixed_put((NFFIXED*)fixed, obj, 17, 67);

		obj = (NFOBJECT*)nfui_checkbutton_new(FALSE);
		nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed, obj, 17, 67+166);
		nfui_regi_post_event_callback(obj, post_use_rule_event_handler);
		g_useRule_obj[i] = obj;

		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("USE RULE", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(122));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
		nfui_nfobject_set_size(obj, 300, 27);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
		nfui_nfobject_show(obj);	
		nfui_nffixed_put((NFFIXED*)fixed, obj, 17+27, 67+166);

		if (i == VCA_S1_RULE_INVASION)
		{
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SENSITIVITY", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(122));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 36);
			nfui_nfobject_set_size(obj, page_w-255, 40);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
			nfui_nfobject_show(obj);	
			nfui_nffixed_put((NFFIXED*)fixed, obj, 0, 297);
			g_invasion_obj[0] = obj;

			obj = nfui_combobox_new(NULL, 0, 0);
			_init_capable_invasion_sensitivity(obj);
			nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
			nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);	
			nfui_nfobject_set_size(obj, 255, 40);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)fixed, obj, page_w-255, 297);
			nfui_regi_post_event_callback(obj, post_invasion_event_handler);	
			g_invasion_obj[1] = obj;

		}
		else if (i == VCA_S1_RULE_LOITERING)
		{
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("TIME", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(122));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 36);
			nfui_nfobject_set_size(obj, page_w-255, 40);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
			nfui_nfobject_show(obj);	
			nfui_nffixed_put((NFFIXED*)fixed, obj, 0, 297);
			g_loitering_obj[0] = obj;

			obj = nfui_combobox_new(NULL, 0, 0);
			_init_capable_loitering_time(obj);
			nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
			nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);	
			nfui_nfobject_set_size(obj, 255, 40);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)fixed, obj, page_w-255, 297);
			nfui_regi_post_event_callback(obj, post_loitering_event_handler);	
			g_loitering_obj[1] = obj;
		}
		else if (i == VCA_S1_RULE_ABANDON)
		{
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("TIME", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(122));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 36);
			nfui_nfobject_set_size(obj, page_w-255, 40);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
			nfui_nfobject_show(obj);	
			nfui_nffixed_put((NFFIXED*)fixed, obj, 0, 297);
			g_abandon_obj[0] = obj;

			obj = nfui_combobox_new(NULL, 0, 0);
			_init_capable_abandon_time(obj);
			nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
			nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);	
			nfui_nfobject_set_size(obj, 255, 40);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)fixed, obj, page_w-255, 297);
			nfui_regi_post_event_callback(obj, post_abandon_event_handler);	
			g_abandon_obj[1] = obj;
		}
		else if (i == VCA_S1_RULE_STEAL)
		{
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("TIME", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(122));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 36);
			nfui_nfobject_set_size(obj, page_w-255, 40);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
			nfui_nfobject_show(obj);	
			nfui_nffixed_put((NFFIXED*)fixed, obj, 0, 297);
			g_steal_obj[0] = obj;

			obj = nfui_combobox_new(NULL, 0, 0);
			_init_capable_steal_time(obj);
			nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
			nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);	
			nfui_nfobject_set_size(obj, 255, 40);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)fixed, obj, page_w-255, 297);
			nfui_regi_post_event_callback(obj, post_steal_event_handler);	
			g_steal_obj[1] = obj;
		}
		else if (i == VCA_S1_RULE_TOPPLE)
		{
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SENSITIVITY", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(122));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 36);
			nfui_nfobject_set_size(obj, page_w-255, 40);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
			nfui_nfobject_show(obj);	
			nfui_nffixed_put((NFFIXED*)fixed, obj, 0, 297);
			g_topple_obj[0] = obj;

			obj = nfui_combobox_new(NULL, 0, 0);
			_init_capable_topple_sensitivity(obj);
			nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
			nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);	
			nfui_nfobject_set_size(obj, 255, 40);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)fixed, obj, page_w-255, 297);
			nfui_regi_post_event_callback(obj, post_topple_event_handler);	
			g_topple_obj[1] = obj;
		}

	}

// RULE SCHEDULE
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(fixed, page_w, page_h);
	nfui_nfobject_hide(fixed);
	nfui_nffixed_put((NFFIXED*)g_setup_fixed, fixed, 0, 0);
	nfui_regi_post_event_callback(fixed, post_ruleSched_fixed_event_handler);	
	g_ruleSched_fixed = fixed;

	for (i = 0; i < MAX_USE_RULE_CNT; i++)
	{
	    obj = (NFOBJECT*)nfui_nfimage_new(IMG_VA_ICON_01_S);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed, obj, 0, 41*i);
		g_dispSchedObj[i][0] = obj;
		
		obj = (NFOBJECT*)nfui_nfimage_new(IMG_VASET_TITLE_BAR);
		nfui_nfimage_set_text((NFIMAGE*)obj, "");
		nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
		nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 16);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed, obj, 40, 41*i);
		g_dispSchedObj[i][1] = obj;
	}

	_update_widgets(0);

	return g_setup_fixed;
}

gint _S1_VCA_RuleSetup_hide()
{
	int i;

	if (nfui_nfobject_is_shown(g_ruleSched_fixed))
		nfui_nfobject_hide(g_ruleSched_fixed);

	for (i = 0; i < VCA_S1_RULE_MAX; i++)
	{
		if (nfui_nfobject_is_shown(g_rule_fixed[i]))
			nfui_nfobject_hide(g_rule_fixed[i]);
	}
}

gint _S1_VCA_RuleSetup_set_channel(gint ch)
{
	gint i;

	if (!g_setup_fixed) return -1;

	g_cur_ch = ch;	
	return 0;
}

gint _S1_VCA_RuleSetup_update()
{
	_update_widgets(g_cur_ch);
	return 0;
}

gint _S1_VCA_RuleSetup_set_ruleIdx(gint rule_idx)
{
	gint i;

	if (!g_setup_fixed) return -1;
	if (rule_idx < 0) return -1;
	if (rule_idx >= VCA_S1_RULE_MAX) return -1;
	if (nfui_nfobject_is_shown(g_rule_fixed[rule_idx])) return -1;

	if (nfui_nfobject_is_shown(g_ruleSched_fixed))
		nfui_nfobject_hide(g_ruleSched_fixed);

	for (i = 0; i < VCA_S1_RULE_MAX; i++)
	{
		if (nfui_nfobject_is_shown(g_rule_fixed[i]))
			nfui_nfobject_hide(g_rule_fixed[i]);
	}

	if (nfui_nfobject_is_shown(g_rule_fixed[rule_idx]) == FALSE)
		nfui_nfobject_show(g_rule_fixed[rule_idx]);

	return 0;
}

gint _S1_VCA_RuleSetup_set_ruleSched()
{
	gint i;

	if (!g_setup_fixed) return -1;

	for (i = 0; i < VCA_S1_RULE_MAX; i++)
	{
		if (nfui_nfobject_is_shown(g_rule_fixed[i]))
			nfui_nfobject_hide(g_rule_fixed[i]);
	}

	_set_disp_rule_sched(g_cur_ch);

	if (nfui_nfobject_is_shown(g_ruleSched_fixed) == FALSE)
		nfui_nfobject_show(g_ruleSched_fixed);

	return 0;
}

