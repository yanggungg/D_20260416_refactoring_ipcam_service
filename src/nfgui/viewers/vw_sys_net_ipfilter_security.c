
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
#include "objects/nfbutton.h"
#include "objects/nfimage.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfspinbutton.h"
#include "objects/nftable.h"

#include "vw_sys_net_ipfilter_internal.h"
#include "vw_sys_filter_add.h"

// FIXME
#include "nf_action.h"
#include "log.h"
#include "scm.h"

#define	IPFILTER_LABEL_LEFT			(27)

#define	FL_COLUMNS		4
#define	FL_ROWS			9

#define	FL_LABLE_LEFT				(28)
#define	FL_LABLE_TOP				(150)

#define	FL_LABLE1_WIDTH				(350)
#define	FL_LABLE1_HEIGHT			(40)

#define	FL_LABLE2_WIDTH				(FL_LABLE1_WIDTH)
#define	FL_LABLE2_HEIGHT			(FL_LABLE1_HEIGHT)

#define	FL_COL_SPACE				(2)
#define	FL_ROW_SPACE				(1)
#define	FL_LABLE_HEIGHT_WITH_SPACE	(41)



static NFWINDOW *g_curwnd = 0;


enum{
	IPFILTER_ROW_ENABLE= 0,
	IPFILTER_ROW_RULE,

	IPFILTER_ROW_NUM
};
static const gchar *strFilterRule[] = {"ALLOW LIST", "DENY LIST"};

static NFOBJECT *value_object[IPFILTER_ROW_NUM];
static NFOBJECT *object;

static IPFilterData filterdata;
static IPFilterData org_filterdata;

static NFOBJECT *fttype[IPFILTER_RULE_MAX];
static NFOBJECT *ftlist[IPFILTER_RULE_MAX];
static NFOBJECT *ftadd[IPFILTER_RULE_MAX];
static NFOBJECT *ftdel[IPFILTER_RULE_MAX];

static guint filter_count;
static guint org_filter_count;

static void showDeleteAddButton(guint cnt)
{
	guint i = 0; 

	for(i = 0; i < IPFILTER_RULE_MAX; i++)
	{
		if(i<cnt)
		{
			nfui_nfbutton_set_text(ftadd[i], "EDIT");
			nfui_nfobject_show(ftadd[i]);
			nfui_nfobject_show(ftdel[i]);
		}
		else if(i==cnt)
		{
			nfui_nfbutton_set_text(ftadd[i], "ADD");
			nfui_nfobject_show(ftadd[i]);
			nfui_nfobject_hide(ftdel[i]);
		}
		else
		{
			nfui_nfobject_hide(ftadd[i]);
			nfui_nfobject_hide(ftdel[i]);
		}
	}
}


static void setTypeText(NFOBJECT *obj, guint type)
{
	if(type	== TYPE_NET_A)
		nfui_nflabel_set_text((NFLABEL*)obj, STR_NET_A);
	else if(type	== TYPE_NET_B)
		nfui_nflabel_set_text((NFLABEL*)obj, STR_NET_B);
	else if(type	== TYPE_NET_C)
		nfui_nflabel_set_text((NFLABEL*)obj, STR_NET_C);
	else if(type	== TYPE_IP_ADDR)
		nfui_nflabel_set_text((NFLABEL*)obj, STR_IP_ADDR);
}

static void setAddrText(NFOBJECT *obj, guint ip1, guint ip2, guint ip3, guint ip4)
{
	gchar addr[32];

	sprintf(addr,"%d.%d.%d.%d",ip1,ip2,ip3,ip4);

	nfui_nflabel_set_text((NFLABEL*)obj, addr);
}

static gboolean post_edit_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint i;
		gchar *title;
		IPFilterListData filter;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		for( i=0; i<IPFILTER_RULE_MAX; i++)
		{
			if(obj == ftadd[i])
				break;
		}

		memset(&filter, 0, sizeof(IPFilterListData));

		title = nfui_nfbutton_get_text((NFBUTTON*)obj);

		if(!strcmp(title,"EDIT"))
			memcpy(&filter, &filterdata.filter_list[i], sizeof(IPFilterListData));

		if(FilterAddDlg_Open(g_curwnd, title, &filter))
		{
			setTypeText(fttype[i], filter.type);
			setAddrText(ftlist[i], filter.listAddr[0], filter.listAddr[1], 
			            filter.listAddr[2], filter.listAddr[3]);

			memcpy(&filterdata.filter_list[i], &filter, sizeof(IPFilterListData));

			if(!strcmp(title,"ADD"))
			{
				showDeleteAddButton(filter_count+1);
				filter_count++;

				if(i != (IPFILTER_RULE_MAX-1) )
					nfui_signal_emit(ftadd[i+1],GDK_EXPOSE,TRUE);
			}

			nfui_signal_emit(fttype[i],GDK_EXPOSE,TRUE);
			nfui_signal_emit(ftlist[i],GDK_EXPOSE,TRUE);
			nfui_signal_emit(ftadd[i],GDK_EXPOSE,TRUE);
			nfui_signal_emit(ftdel[i],GDK_EXPOSE,TRUE);

			if(!strcmp(title,"ADD"))
				nfui_make_key_hierarchy(g_curwnd);


		}
	}

	return FALSE;
}

static gboolean post_delete_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint i;
		guint del_idx;
		guint cnt;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}
		for( i=0; i<IPFILTER_RULE_MAX; i++)
		{
			if(obj == ftdel[i])
				break;
		}
		del_idx = i;

		memset(&filterdata.filter_list[i], 0, sizeof(IPFilterListData));
		for(i=del_idx; i< filter_count ; i++)
		{
			nfui_nflabel_set_text((NFLABEL*)fttype[i],"");
			nfui_nflabel_set_text((NFLABEL*)ftlist[i],"");
		}

		if(del_idx != filter_count-1)
		{
			memcpy(&filterdata.filter_list[del_idx], &filterdata.filter_list[del_idx+1], 
			        sizeof(IPFilterListData)*(filter_count-del_idx-1));
			for(i = del_idx; i< (filter_count-1); i++)
			{
				setTypeText(fttype[i], filterdata.filter_list[i].type);
				setAddrText(ftlist[i], filterdata.filter_list[i].listAddr[0], 
						filterdata.filter_list[i].listAddr[1],
						filterdata.filter_list[i].listAddr[2],
						filterdata.filter_list[i].listAddr[3]);
			}
		}

		showDeleteAddButton(filter_count-1);
		filter_count--;

		nfui_signal_emit(obj->parent, GDK_EXPOSE, TRUE);

		nfui_make_key_hierarchy(g_curwnd);
		nfui_set_key_focus(ftadd[del_idx],TRUE);


	}

	return FALSE;
}


static gboolean post_opmode_spinbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_SPINBUTTON_CHANGED)
	{
		filterdata.opmode = nfui_spin_button_get_index((NFSPINBUTTON*)obj);
	}
	return FALSE;
}


static gboolean post_enable_spinbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_SPINBUTTON_CHANGED)
	{
		guint i;

		if(!nfui_spin_button_get_index((NFSPINBUTTON*)obj))
		{
			nfui_nfobject_disable(value_object[IPFILTER_ROW_RULE]);
			
			for(i=0; i<IPFILTER_RULE_MAX; i++)
			{
				nfui_nfobject_disable(fttype[i]);
				nfui_nfobject_disable(ftlist[i]);
				nfui_nfobject_disable(ftadd[i]);
				nfui_nfobject_disable(ftdel[i]);
			}
		}
		else
		{
			nfui_nfobject_enable(value_object[IPFILTER_ROW_RULE]);
			
			for(i=0; i<IPFILTER_RULE_MAX; i++)
			{
				nfui_nfobject_enable(fttype[i]);
				nfui_nfobject_enable(ftlist[i]);
				nfui_nfobject_enable(ftadd[i]);
				nfui_nfobject_enable(ftdel[i]);
			}
		}

		nfui_signal_emit(value_object[IPFILTER_ROW_RULE], GDK_EXPOSE, TRUE);
		
		for(i=0; i<IPFILTER_RULE_MAX; i++)
		{
			nfui_signal_emit(fttype[i], GDK_EXPOSE, TRUE);
			nfui_signal_emit(ftlist[i], GDK_EXPOSE, TRUE);
			nfui_signal_emit(ftadd[i], GDK_EXPOSE, TRUE);
			nfui_signal_emit(ftdel[i], GDK_EXPOSE, TRUE);
		}
		filterdata.enable = nfui_spin_button_get_index((NFSPINBUTTON*)obj);
	}

	return FALSE;
}

static gboolean post_cancel_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint i;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		if(!memcmp(&org_filterdata, &filterdata, sizeof(IPFilterData)))
			return FALSE;

		g_memmove(&filterdata, &org_filterdata, sizeof(IPFilterData));
		filter_count = org_filter_count;

		nfui_spin_button_set_index((NFSPINBUTTON*)(value_object[IPFILTER_ROW_ENABLE]), filterdata.enable);
		nfui_spin_button_set_index((NFSPINBUTTON*)(value_object[IPFILTER_ROW_RULE]), filterdata.opmode);

		showDeleteAddButton(filter_count);

		for(i=0; i< IPFILTER_RULE_MAX; i++)
		{
			nfui_nflabel_set_text((NFLABEL*)fttype[i],"");
			nfui_nflabel_set_text((NFLABEL*)ftlist[i],"");
		}

		for(i=0; i< filter_count; i++)
		{
			setTypeText(fttype[i], filterdata.filter_list[i].type);
			setAddrText(ftlist[i], filterdata.filter_list[i].listAddr[0],
					filterdata.filter_list[i].listAddr[1],
					filterdata.filter_list[i].listAddr[2],
					filterdata.filter_list[i].listAddr[3] );
		}

		if(!filterdata.enable)
		{
			nfui_nfobject_disable(value_object[IPFILTER_ROW_RULE]);

			for(i=0; i<IPFILTER_RULE_MAX; i++)
			{
				nfui_nfobject_disable(fttype[i]);
				nfui_nfobject_disable(ftlist[i]);
				nfui_nfobject_disable(ftadd[i]);
				nfui_nfobject_disable(ftdel[i]);
			}
		}
		else
		{
			nfui_nfobject_enable(value_object[IPFILTER_ROW_RULE]);

			for(i=0; i<IPFILTER_RULE_MAX; i++)
			{
				nfui_nfobject_enable(fttype[i]);
				nfui_nfobject_enable(ftlist[i]);
				nfui_nfobject_enable(ftadd[i]);
				nfui_nfobject_enable(ftdel[i]);
			}
		}

		nfui_signal_emit(obj->parent, GDK_EXPOSE, TRUE);
		nfui_make_key_hierarchy(g_curwnd);
	}

	return FALSE;
}

static gboolean post_apply_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint index;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		if(memcmp(&org_filterdata, &filterdata, sizeof(IPFilterData)))
		{
			g_memmove(&org_filterdata, &filterdata, sizeof(IPFilterData));
			org_filter_count = filter_count;

			DAL_set_ipfilter_data(&filterdata);
			DAL_set_ipfilter_count(filter_count);

            //ipfilter init
            scm_apply_ipfilter();
            
			nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");

			sysnet_set_changeflag(1);
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

		NetSecurity_tab_out_handler();
		SystemSetupNetwork_Destroy(obj);
	}

	return FALSE;
}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
	}

	return FALSE;
}


void VW_Init_Security_IPFilter_Page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *obj;
	NFOBJECT *subject_object[IPFILTER_ROW_NUM];
	NFOBJECT *ntb;

	const gchar *strTitle[] = {
					"IP FILTER ENABLE",
					"IP FILTER RULE"
	};
	const gchar *strList[] =  {"TYPE", 
								"LIST" };

	gint title_ypos[2] = {13, 55};

	guint width[FL_COLUMNS];
	const gchar *strOffOn[] = {"OFF", "ON"};
	const gchar *strRule[] = {"ALLOW LIST", "DENY LIST"};


	guint i;

	guint inactive_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(900), COLOR_IDX(901), COLOR_IDX(902), COLOR_IDX(904)};
	guint active_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(903), COLOR_IDX(901), COLOR_IDX(902), COLOR_IDX(904)};

	g_curwnd = nfui_nfobject_get_top(parent);

	width[0] = FL_LABLE1_WIDTH;
	width[1] = FL_LABLE2_WIDTH;
	width[2] = 192;
	width[3] = 192;

	/* FIXED */
	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);

	memset(&filterdata, 0x00, sizeof(IPFilterData));
	memset(&org_filterdata, 0x00, sizeof(IPFilterData));

	DAL_get_ipfilter_data(&filterdata);
	g_memmove(&org_filterdata, &filterdata, sizeof(IPFilterData));

	value_object[IPFILTER_ROW_ENABLE] = (NFOBJECT*)nfui_spinbutton_new((gchar**)strOffOn, 2, (gint)filterdata.enable);
	value_object[IPFILTER_ROW_RULE] = (NFOBJECT*)nfui_spinbutton_new((gchar**)strRule, 2, (gint)filterdata.opmode);

	for(i = 0 ; i < 2; i++)
	{
		subject_object[i] = (NFOBJECT*)nfui_nflabel_new_with_pango_font((gchar*)strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
		nfui_nfobject_set_size(subject_object[i], 400, 40);
		nfui_nflabel_set_align((NFLABEL*)subject_object[i], NFALIGN_LEFT, 20);
		nfui_nfobject_modify_bg(subject_object[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
		nfui_nfobject_use_focus(subject_object[i], NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(subject_object[i]);
		nfui_nffixed_put((NFFIXED*)content_fixed, subject_object[i], IPFILTER_LABEL_LEFT, title_ypos[i]);


		nfui_spinbutton_set_skin_type((NFSPINBUTTON*)value_object[i], NFSPINBUTTON_TYPE_SUBTAB_1);
		nfui_spin_button_set_align((NFSPINBUTTON*)value_object[i], NFALIGN_LEFT, 4);
		nfui_spin_button_set_spacing((NFSPINBUTTON*)value_object[i], CONDENSED_SPACING);
		nfui_nfobject_set_size(value_object[i], 250, 40);
		nfui_nfobject_show(value_object[i]);
		nfui_nffixed_put((NFFIXED*)content_fixed, value_object[i], IPFILTER_LABEL_LEFT+400, title_ypos[i]);
	}
	nfui_regi_post_event_callback(value_object[IPFILTER_ROW_ENABLE], post_enable_spinbutton_event_handler);
	nfui_regi_post_event_callback(value_object[IPFILTER_ROW_RULE], post_opmode_spinbutton_event_handler);


	ntb = nfui_nftable_new(FL_COLUMNS, FL_ROWS, FL_COL_SPACE, FL_ROW_SPACE, width, FL_LABLE1_HEIGHT);
	nfui_nfobject_show(ntb);
	nfui_nffixed_put(content_fixed, ntb, IPFILTER_LABEL_LEFT, FL_LABLE_TOP);
	nfui_nfobject_modify_bg(ntb, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));


	for(i=0; i<2 ; i++)
	{
		object = nfui_nflabel_new_with_pango_font(strList[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
		nfui_nfobject_modify_bg(object, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_use_focus(object, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(object);
		nfui_nftable_attach((NFTABLE*)ntb, object, i, 0);
	}

	filter_count = DAL_get_ipfilter_count();
	org_filter_count = filter_count;

	for(i=0; i<IPFILTER_RULE_MAX; i++)
	{
		fttype[i] = nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_spacing((NFLABEL *)fttype[i],SEMI_CONDENSED_SPACING);
		nfui_nflabel_set_skin_type(fttype[i], NFTEXTBOX_TYPE_SUBTAB_OUTPUT);
		nfui_nfobject_show(fttype[i]);
		nfui_nfobject_use_focus(fttype[i], NFOBJECT_FOCUS_OFF);
		nfui_nftable_attach((NFTABLE*)ntb, fttype[i], 0, i+1);

		ftlist[i] = nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_spacing((NFLABEL *)ftlist[i],SEMI_CONDENSED_SPACING);
		nfui_nflabel_set_skin_type(ftlist[i], NFTEXTBOX_TYPE_SUBTAB_OUTPUT);
		nfui_nfobject_show(ftlist[i]);
		nfui_nfobject_use_focus(ftlist[i], NFOBJECT_FOCUS_OFF);
		nfui_nftable_attach((NFTABLE*)ntb, ftlist[i], 1, i+1);

//button
		ftadd[i] = nftool_normal_button_create_subtab_type1("ADD", 192);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(ftadd[i]), NFALIGN_CENTER, 0);	
		nfui_nfobject_show(ftadd[i]);
		nfui_nftable_attach((NFTABLE*)ntb, ftadd[i], 2, i+1);
		nfui_regi_post_event_callback(ftadd[i], post_edit_button_event_handler);

		ftdel[i] = nftool_normal_button_create_subtab_type1("DELETE", 192);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(ftdel[i]), NFALIGN_CENTER, 0);	
		nfui_nfobject_show(ftdel[i]);
		nfui_nftable_attach((NFTABLE*)ntb, ftdel[i], 3, i+1);
		nfui_regi_post_event_callback(ftdel[i], post_delete_button_event_handler);
	}

	showDeleteAddButton(filter_count);

	for(i=0; i< filter_count; i++)
	{
		setTypeText(fttype[i], filterdata.filter_list[i].type);
		setAddrText(ftlist[i], filterdata.filter_list[i].listAddr[0],
		            filterdata.filter_list[i].listAddr[1],
		            filterdata.filter_list[i].listAddr[2],
		            filterdata.filter_list[i].listAddr[3] );
	}

	if(!filterdata.enable)
	{
		nfui_nfobject_disable(value_object[IPFILTER_ROW_RULE]);

		for(i=0; i<IPFILTER_RULE_MAX; i++)
		{
			nfui_nfobject_disable(fttype[i]);
			nfui_nfobject_disable(ftlist[i]);
			nfui_nfobject_disable(ftadd[i]);
			nfui_nfobject_disable(ftdel[i]);
		}
	}
	else
	{
		nfui_nfobject_enable(value_object[IPFILTER_ROW_RULE]);

		for(i=0; i<IPFILTER_RULE_MAX; i++)
		{
			nfui_nfobject_enable(fttype[i]);
			nfui_nfobject_enable(ftlist[i]);
			nfui_nfobject_enable(ftadd[i]);
			nfui_nfobject_enable(ftdel[i]);
		}
	}



	/* button */
	obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R3_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_cancel_button_event_handler);

	obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R2_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_apply_button_event_handler);

	obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R1_X, MENU_V_SUBTAB_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

	nfui_regi_post_event_callback(parent, post_page_event_handler);
}

gboolean check_net_security_ipfilter_changed()
{
	if(memcmp(&org_filterdata, &filterdata, sizeof(IPFilterData)))
		return TRUE;

	return FALSE;
}

void save_net_security_ipfilter_data()
{
	if(memcmp(&org_filterdata, &filterdata, sizeof(IPFilterData)))
	{
		g_memmove(&org_filterdata, &filterdata, sizeof(IPFilterData));
		org_filter_count = filter_count;

		DAL_set_ipfilter_data(&filterdata);
		DAL_set_ipfilter_count(filter_count);

		//ipfilter init
		scm_apply_ipfilter();
	}

}

void restore_net_security_ipfilter_data()
{
	guint i;

	g_memmove(&filterdata, &org_filterdata, sizeof(IPFilterData));
	filter_count = org_filter_count;
	nfui_spin_button_set_index((NFSPINBUTTON*)(value_object[IPFILTER_ROW_ENABLE]), filterdata.enable);
	nfui_spin_button_set_index((NFSPINBUTTON*)(value_object[IPFILTER_ROW_RULE]), filterdata.opmode);

	showDeleteAddButton(filter_count);

	for(i=0; i< IPFILTER_RULE_MAX; i++)
	{
		nfui_nflabel_set_text((NFLABEL*)fttype[i],"");
		nfui_nflabel_set_text((NFLABEL*)ftlist[i],"");
	}

	for(i=0; i< filter_count; i++)
	{
		setTypeText(fttype[i], filterdata.filter_list[i].type);
		setAddrText(ftlist[i], filterdata.filter_list[i].listAddr[0],
				filterdata.filter_list[i].listAddr[1],
				filterdata.filter_list[i].listAddr[2],
				filterdata.filter_list[i].listAddr[3] );
	}

	if(!filterdata.enable)
	{
		nfui_nfobject_disable(value_object[IPFILTER_ROW_RULE]);

		for(i=0; i<IPFILTER_RULE_MAX; i++)
		{
			nfui_nfobject_disable(fttype[i]);
			nfui_nfobject_disable(ftlist[i]);
			nfui_nfobject_disable(ftadd[i]);
			nfui_nfobject_disable(ftdel[i]);
		}
	}
	else
	{
		nfui_nfobject_enable(value_object[IPFILTER_ROW_RULE]);

		for(i=0; i<IPFILTER_RULE_MAX; i++)
		{
			nfui_nfobject_enable(fttype[i]);
			nfui_nfobject_enable(ftlist[i]);
			nfui_nfobject_enable(ftadd[i]);
			nfui_nfobject_enable(ftdel[i]);
		}
	}
}

