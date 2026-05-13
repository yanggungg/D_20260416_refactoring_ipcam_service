
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

#include "vw_s1_vca_setup_main.h"
#include "vw_s1_vca_internal.h"

#include "vaa.h"
#include "vaa_s1.h"

#define TBL_COL_CNT		(5)

typedef struct _S1_VCA_SELECT_CALLBACK
{
	VCA_SELECT_CB_FUNC 	cb_func;
	gpointer 			user_data;
} S1_VCA_SELECT_CALLBACK;

static S1_VCA_SELECT_CALLBACK g_select_cb = {0, };

static NFOBJECT *g_select_fixed = 0;
static NFOBJECT *g_ruleBtn[VCA_S1_RULE_MAX] = {0, };

static gint	g_cur_ch = 0;
static VARULE_CONF g_confs[16];
static gint	g_confcnt;
static VAAID g_vaaid = 0;

static gchar *rule_icon[VCA_S1_RULE_MAX][2] = {
		{IMG_D_VA_MENU_ICON_01, IMG_N_VA_MENU_ICON_01},
		{IMG_D_VA_MENU_ICON_02, IMG_N_VA_MENU_ICON_02},
		{IMG_D_VA_MENU_ICON_07, IMG_N_VA_MENU_ICON_07},
		{IMG_D_VA_MENU_ICON_08, IMG_N_VA_MENU_ICON_08},		
		{IMG_D_VA_MENU_ICON_09, IMG_N_VA_MENU_ICON_09},
		{IMG_D_VA_MENU_ICON_03, IMG_N_VA_MENU_ICON_03},
		{IMG_D_VA_MENU_ICON_04, IMG_N_VA_MENU_ICON_04},
		{IMG_D_VA_MENU_ICON_06, IMG_N_VA_MENU_ICON_06},		
//		{IMG_D_VA_MENU_ICON_10, IMG_N_VA_MENU_ICON_10},
};


static gint _change_rule_button(gint ch)
{
	GdkPixbuf *icon_pbuf[NFOBJECT_STATE_COUNT];

	VAAID vaaid = 0;
	VARULE_CONF	confs[16];
	gint confcnt;
	gint i;

	vaaid = vaa_get_vaaid(ch);
	memset(confs, 0x00, sizeof(confs));
	vaa_s1_get_rule_confs_all(vaaid, confs, &confcnt);

	for (i = 0; i < VCA_S1_RULE_MAX; i++)
	{
		if (confs[i].use_rule) 
		{
			icon_pbuf[0] = nfui_get_image_from_file((rule_icon[i][1]), NULL);
			icon_pbuf[1] = nfui_get_image_from_file((rule_icon[i][1]), NULL);
			icon_pbuf[2] = nfui_get_image_from_file((rule_icon[i][1]), NULL);
		}
		else
		{
			icon_pbuf[0] = nfui_get_image_from_file((rule_icon[i][0]), NULL);
			icon_pbuf[1] = nfui_get_image_from_file((rule_icon[i][0]), NULL);
			icon_pbuf[2] = nfui_get_image_from_file((rule_icon[i][0]), NULL);
		}		

		icon_pbuf[3] = nfui_get_image_from_file((rule_icon[i][0]), NULL);
		nfui_nfbutton_set_icon_image(NF_BUTTON(g_ruleBtn[i]), icon_pbuf);
			
		if (vaa_s1_is_enabled(vaaid))
			nfui_nfobject_enable(g_ruleBtn[i]);
		else
			nfui_nfobject_disable(g_ruleBtn[i]);
	}

	return 0;
}

static gboolean post_rule_select_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE) 
	{
		NFOBJECT *top;	
		NFOBJECT *fixed;
		gint i;

		for (i = 0; i < VCA_S1_RULE_MAX; i++)
		{
			if (g_ruleBtn[i] == obj) break;
		}

		g_message("%s, %d, select rule idx:%d", __FUNCTION__, __LINE__, i);		
		fixed = _get_S1_VCA_ruleSetup_fixed();
		_S1_VCA_RuleSetup_set_ruleIdx(i);
		nfui_signal_emit(fixed, GDK_EXPOSE, TRUE);

		if (g_select_cb.cb_func)
			g_select_cb.cb_func(i, g_select_cb.user_data);

		top = nfui_nfobject_get_top(obj);
		nfui_make_key_hierarchy(top);
	}

	return FALSE;
}

NFOBJECT* _S1_VCA_RuleSelect_Page(NFOBJECT *parent, gint page_x, gint page_y, gint page_w, gint page_h)
{
	NFOBJECT *fixed;
	NFOBJECT *tbl;
	NFOBJECT *obj;

	guint table_w[TBL_COL_CNT];
	gint size_w, size_h;
	gint i;

	ICONPOSITION pos;
	GdkPixbuf *bg_image[NFOBJECT_STATE_COUNT];
	const guint font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(1116), COLOR_IDX(1117), COLOR_IDX(1118), COLOR_IDX(1120)}; 
	const gchar *rule_name[] = {STR_RULE_INVASION, STR_RULE_LOITERING, STR_RULE_ABANDON, 
								STR_RULE_STEAL,	STR_RULE_TOPPLE, STR_RULE_FENCE, 
								STR_RULE_COUNT, STR_RULE_TAMPERING, STR_RULE_PRIVACY};

	g_cur_ch = 0;

	bg_image[0] = nfui_get_image_from_file((IMG_BT_VA_MENU_N), NULL); 
	bg_image[1] = nfui_get_image_from_file((IMG_BT_VA_MENU_O), NULL); 
	bg_image[2] = nfui_get_image_from_file((IMG_BT_VA_MENU_P), NULL); 
	bg_image[3] = nfui_get_image_from_file((IMG_BT_VA_MENU_D), NULL); 
	
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(fixed, page_w, page_h);
	nfui_nfobject_show(fixed);
	nfui_nffixed_put((NFFIXED*)parent, fixed, page_x, page_y);
	g_select_fixed = fixed;

	for (i = 0; i < TBL_COL_CNT; i++)
	{
		table_w[i] = 283;
	}

	tbl = (NFOBJECT*)nfui_nftable_new(TBL_COL_CNT, 2, 19, 20, table_w, 98);
	nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_show(tbl);
	nfui_nffixed_put((NFFIXED*)g_select_fixed, tbl, 0, 48);

	memset(&pos, 0x00, sizeof(ICONPOSITION));
	pos.x = 17;
	pos.y = 17;

	for (i = 0; i < VCA_S1_RULE_MAX; i++)
	{
		obj = nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), bg_image);
		nfui_nfbutton_set_text(NF_BUTTON(obj), rule_name[i]);
		nfui_nfbutton_set_icon_position(NF_BUTTON(obj), pos);
		nfui_nfbutton_set_pango_font(NF_BUTTON(obj), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), (guint*)font_color);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_LEFT, 42);
		nfui_nfbutton_set_drawing_outline(NF_BUTTON(obj), FALSE);
		nfui_regi_post_event_callback(obj, post_rule_select_button_event_handler);		
		nfui_nfobject_show(obj);		
		nfui_nftable_attach((NFTABLE*)tbl, obj, i%TBL_COL_CNT, i/TBL_COL_CNT);	
		g_ruleBtn[i] = obj;
	}

	_change_rule_button(g_cur_ch);

	return g_select_fixed;
}

gint _S1_VCA_RuleSelect_set_channel(gint ch)
{
	g_cur_ch = ch;
	return 0;
}

gint _S1_VCA_RuleSelect_update()
{
	_change_rule_button(g_cur_ch);
	return 0;
}

gint _S1_VCA_RuleSelect_set_ruleSched()
{
	if (g_select_cb.cb_func)
		g_select_cb.cb_func(-1, g_select_cb.user_data);

	return 0;
}

gint VW_S1_VCA_attach_select_ruleid(VCA_SELECT_CB_FUNC select_cb, gpointer user_data)
{
	g_select_cb.cb_func = select_cb;
	g_select_cb.user_data = user_data;
	return 0;
}

