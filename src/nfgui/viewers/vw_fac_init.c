#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"
#include "nf_ui_tool.h"

#include "tools/nf_ui_tool.h"
#include "tools/ix_mem.h"

#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"
#include "objects/nfspinbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nfimage.h"

#include "smt.h"
#include "scm.h"
#include "nf_api_param_app.h"
#include "nf_sysman.h"

#define CONF_WIN_POS_X							((DISPLAY_ACTIVE_WIDTH - CONF_WIN_SIZE_W) / 2)
#define CONF_WIN_POS_Y							((DISPLAY_ACTIVE_HEIGHT - CONF_WIN_SIZE_H) / 2)
#define CONF_WIN_SIZE_W							(750)
#define CONF_WIN_SIZE_H							(250)

#define NF_TOPWND	0

#define LANGUAGE_STR_SIZE		(32)

enum {
	PAGE_LANG = 0,
	PAGE_TZ,	
	PAGE_SIG,
	PAGE_MAX
};

enum {
	LANG_CANCEL = 0,
	LANG_NEXT,	
	LANG_BUTTONS
};

enum {
	TZ_CANCEL = 0,	
	TZ_PREV,
	TZ_NEXT,	
	TZ_BUTTONS
};

enum {
	SIG_CANCEL = 0,	
	SIG_PREV,
	SIG_APPLY,	
	SIG_BUTTONS
};

gint	ret = 0;
gint    cur_page = 0;
guint	timer_src = 0;

static gchar **strLang = NULL;
static gint langCnt = 0;

static FacInitData org_init_conf;
static FacInitData init_conf;

NFOBJECT *time_label = NULL;
NFOBJECT *obj_lang = NULL;
NFOBJECT *obj_tz = NULL;
NFOBJECT *obj_sig = NULL;
NFOBJECT *page[PAGE_MAX];
NFOBJECT *focus[PAGE_MAX];

static gboolean init_lang()
{
	gint i;

	langCnt = DAL_get_support_lang_cnt();
	if(langCnt < 0)
		return FALSE;
	
	strLang = (gchar**)imalloc(sizeof(gchar*)*langCnt);
	for(i=0; i<langCnt; i++)  {
		strLang[i] = (gchar*)imalloc(LANGUAGE_STR_SIZE);

		g_assert(strLang[i]);
	}

	for(i=0; i<langCnt; i++) {
		DAL_get_support_lang(i, strLang[i]);
	}

	return TRUE;
}

static gboolean _time_remain(void *data)
{
	static gint count = 10;
	gchar time_str[30];

	if( count == 0){
		NFOBJECT *top;
		top = nfui_nfobject_get_top((NFOBJECT*)data);	
		nfui_nfobject_destroy(top);
		return FALSE;
	}

	sprintf(time_str, "AUTO CANCEL : %d SEC", count);
	nfui_nflabel_set_text((NFOBJECT*)data, time_str);
	nfui_signal_emit((NFOBJECT*)data, GDK_EXPOSE, TRUE);

	count--;

	return TRUE;
} 

static gboolean change_language_by_string(gchar *str)
{
	deinit_multi_language_support();
	
	if(0 != init_multi_language_support(str)){
		return 1;
	}

	vw_apply_new_lang();

	return 0;
}

static gboolean remove_timer(void)
{
	if(timer_src){
		g_source_remove(timer_src);
		timer_src = 0;
		return 1;
	}
	else
		return 0;
}

static void show_and_update(NFOBJECT *obj, gint num)
{
	NFOBJECT *top;
	nfui_nfobject_show(page[num]);
	top = nfui_nfobject_get_top(obj);
	nfui_make_key_hierarchy(top);
	nfui_set_key_focus(focus[num], TRUE);	
	nfui_signal_emit(top, GDK_EXPOSE, TRUE);
}

static gboolean decrease_page(NFOBJECT *obj, gint *num)
{
	gint page_num = *num;
	
	if(page_num > 0)
	{
		nfui_nfobject_hide(page[page_num]);
		page_num--;
		show_and_update(obj, page_num);
		*num = page_num;
		return 0;
	}
	else
		return 1;
}

static gboolean increase_page(NFOBJECT *obj, gint *num)
{	
	gint page_num = *num;

	if((page_num + 1) < PAGE_MAX)
	{	
		nfui_nfobject_hide(page[page_num]);
		page_num++;
		show_and_update(obj, page_num);
		*num = page_num;
		return 0;
	}
	else
		return 1;
}

static gboolean post_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_DELETE){	
		nfui_page_close(PGID_FAC_INIT, obj);		
		gtk_main_quit();
	}
	
	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top;
		
		if(strcmp(org_init_conf.lang, init_conf.lang))
		{			
			change_language_by_string(org_init_conf.lang);
		}
		
		top = nfui_nfobject_get_top(obj);		
		nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean post_return_event_handler(NFOBJECT *top, GdkEvent *event, gpointer data)
{	
	if(strcmp(org_init_conf.lang, init_conf.lang))
	{
		change_language_by_string(org_init_conf.lang);
	}

	return TRUE;
}

static gboolean post_lang_next_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(remove_timer()){
			nfui_nflabel_set_text(time_label, "");
			nfui_signal_emit(time_label, GDK_EXPOSE, TRUE);		
		}
		
		if(strcmp(init_conf.lang, nfui_spin_button_get_text((NFSPINBUTTON*)obj_lang)))
		{
			g_stpcpy(init_conf.lang, nfui_spin_button_get_text((NFSPINBUTTON*)obj_lang));
			
			change_language_by_string(init_conf.lang);
		}

		increase_page(obj, &cur_page);
	}

	return FALSE;
}

static gboolean post_prev_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		decrease_page(obj, &cur_page);
	}

	return FALSE;
}

static gboolean post_tz_next_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		init_conf.timeZone = nfui_combobox_get_cur_index(obj_tz);
		
		increase_page(obj, &cur_page);
	}

	return FALSE;
}

static void print_db(FILE *f, gchar *key, gchar *type, gchar *min, gchar *max, gchar *val)
{
	gchar *markup = NULL;
	markup = g_markup_printf_escaped("<item key=\"%s\" type=\"%s\" min=\"%s\" max=\"%s\" val=\"%s\" />\n", key, type, min, max, val);
	fputs( markup, f);	
	g_free(markup);
}

static void make_private_db(void)
{
	FILE *f = NULL;	
	gchar buf[20];
	
	f = fopen("/NFDVR/data/nf_sysdb_private.conf","w");
	fprintf( f, "<nf_sysdb>\n");

	fprintf( f, "<disp>\n");
	print_db(f, "disp.osd.lang", "STRING", "0", "32", init_conf.lang);
	fprintf( f, "</disp>\n");

	fprintf( f, "<sys>\n");
	sprintf( buf, "%u", init_conf.timeZone);
	print_db(f, "sys.date.tz_index", "UINT", "0", "", buf);
	sprintf( buf, "%u", init_conf.sigType);	
	print_db(f, "sys.info.sig_type", "BOOL", "0", "1", buf);	
	fprintf( f, "</sys>\n");
	
	fprintf( f, "</nf_sysdb>\n");
	fclose(f);	
}

static gboolean post_apply_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		init_conf.sigType = nfui_spin_button_get_index(obj_sig);
		
		make_private_db();

		DAL_set_fac_init_run(FALSE);
		DAL_set_fac_init_data(&init_conf);		
		DAL_save_setup_db(NFSETUP_WINDOW_DISPLAY);
		DAL_save_setup_db(NFSETUP_WINDOW_SYSTEM);

		if(org_init_conf.sigType != init_conf.sigType)
		{
			NFOBJECT *top;
			top = nfui_nfobject_get_top(obj);			
			nftool_mbox(top, "NOTICE", "Video type has been changed.\nThe system will be reboot.", NFTOOL_MB_OK);			
			nf_api_param_app_set_cate(NF_SYSMAN_APP_PARAM_CATE_IS_PAL, (gint)init_conf.sigType);
			scm_reboot_system(RR_SIGNAL_CHANGE, 0);		
		}
		else
		{
			NFOBJECT *top;			
			scm_apply_timezone(init_conf.timeZone, NULL);
			top = nfui_nfobject_get_top(obj);		
			nfui_nfobject_destroy(top);
		}
	}

	return FALSE;
}

static gboolean post_spinbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_SPINBUTTON_CHANGED)
	{
		if(remove_timer()){
			nfui_nflabel_set_text(time_label, "");
			nfui_signal_emit(time_label, GDK_EXPOSE, TRUE);		
		}		
	}

	return FALSE;
}

static gboolean post_lang_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		remove_timer();
		
		gint i;

		if (strLang) 
		{
			for (i = 0; i < langCnt; i++)  
			{
				if (strLang[i]) {
					ifree(strLang[i]);
					strLang[i] = NULL;
				}
			}

			ifree(strLang);
			strLang = 0;
		}
	}

	return FALSE;
}

void init_lang_open(NFOBJECT *parent)
{
	NFOBJECT *obj = NULL;
	NFOBJECT *btns[LANG_BUTTONS];
	guint i;
	const gchar *strButton[] = {"CANCEL","NEXT"};

	if(!init_lang()) {
		g_warning("%s [%d] : init support language error", __FUNCTION__, __LINE__);
		return;
	}

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DEFAULT VALUE WIZARD", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_support_multi_lang(obj, FALSE);
	nfui_nfobject_use_tooltip(obj, FALSE);
	nfui_nfobject_set_size(obj, (parent->width) - 70 - 8, 30);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, 4, 7);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("1 / 3", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nflabel_set_spacing(obj, SEMI_CONDENSED_SPACING);
	nfui_nfobject_support_multi_lang(obj, FALSE);
	nfui_nfobject_use_tooltip(obj, FALSE);
	nfui_nfobject_set_size(obj, 70, 30);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 10);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, 4 + ((parent->width) - 70 - 8), 7);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("LANGUAGE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_support_multi_lang(obj, FALSE);
	nfui_nfobject_set_size(obj, 200, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, 20, 100);

	obj = (NFOBJECT*)nfui_spinbutton_new(strLang, langCnt, 0);
	obj_lang = obj;	
	nfui_spin_button_set_text_no_expose(obj, init_conf.lang);
	nfui_spinbutton_set_skin_type(obj, NFSPINBUTTON_TYPE_1);
	nfui_spin_button_set_align(obj, NFALIGN_LEFT, 4);
	nfui_spin_button_set_spacing(obj, CONDENSED_SPACING);
	nfui_nfobject_set_size(obj, 250, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, (parent->width) - 270, 100);

	nfui_regi_post_event_callback(obj, post_spinbutton_event_handler);

	obj = nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
	time_label = obj;
	nfui_nfobject_support_multi_lang(obj, FALSE);	
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, 274, 40);	
	nfui_nffixed_put((NFFIXED*)parent, obj, 10, (parent->height) - 60);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);

	for(i=0; i<LANG_BUTTONS; i++)
	{
		btns[i] = nftool_normal_button_create_type1(strButton[i], 168);
		nfui_nfobject_support_multi_lang(btns[i], FALSE);		
		nfui_nfbutton_set_font_alignment(NF_BUTTON(btns[i]), NFALIGN_CENTER, 0);	
		nfui_nfobject_show(btns[i]);
		nfui_nffixed_put((NFFIXED*)parent, btns[i], (parent->width - 10) - (174 * (LANG_BUTTONS - i)), (parent->height) - 60);		
	}

	focus[PAGE_LANG] = btns[LANG_CANCEL];
	nfui_regi_post_event_callback(btns[LANG_CANCEL], post_cancelbutton_event_handler);
	nfui_regi_post_event_callback(btns[LANG_NEXT], post_lang_next_event_handler);

	nfui_regi_post_event_callback(parent, post_lang_page_event_handler);

	timer_src = g_timeout_add(1000, _time_remain, time_label);
}

void init_timezone_open(NFOBJECT *parent)
{
	NFOBJECT *obj = NULL;
	NFOBJECT *btns[TZ_BUTTONS];	
	guint i;
	gchar *strTimezone[50];
	guint zone_count = 0;
	const gchar *strButton[] = {"CANCEL","PREV.","NEXT"};

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DEFAULT VALUE WIZARD", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_tooltip(obj, FALSE);
	nfui_nfobject_set_size(obj, (parent->width)-70 - 8, 30);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, 4, 7);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("2 / 3", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nflabel_set_spacing(obj, SEMI_CONDENSED_SPACING);	
	nfui_nfobject_use_tooltip(obj, FALSE);
	nfui_nfobject_set_size(obj, 70, 30);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 10);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, 4 + ((parent->width) - 70 - 8), 7);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("TIMEZONE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_set_size(obj, 300, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, 20, 100);

	zone_count = nf_zoneinfo_get_count();
	
	for(i=0; i<zone_count; i++)
		strTimezone[i] = nf_zoneinfo_get_string((gint)i);

	obj = nfui_combobox_new(strTimezone, zone_count, (gint)(init_conf.timeZone));
	obj_tz = obj;
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, 400, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, (parent->width) - 420, 100);
	
	for(i=0; i<TZ_BUTTONS; i++)
	{
		btns[i] = nftool_normal_button_create_type1(strButton[i], 168);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(btns[i]), NFALIGN_CENTER, 0);	
		nfui_nfobject_show(btns[i]);
		nfui_nffixed_put((NFFIXED*)parent, btns[i], (parent->width - 10) - (174 * (TZ_BUTTONS - i)), (parent->height) - 60);		
	}

	focus[PAGE_TZ] = btns[TZ_CANCEL];
	nfui_regi_post_event_callback(btns[TZ_CANCEL], post_cancelbutton_event_handler);
	nfui_regi_post_event_callback(btns[TZ_PREV], post_prev_event_handler);
	nfui_regi_post_event_callback(btns[TZ_NEXT], post_tz_next_event_handler);
}

void init_sigtype_open(NFOBJECT *parent)
{
	NFOBJECT *obj = NULL;
	NFOBJECT *btns[SIG_BUTTONS];	
	guint i;
	const gchar *strButton[] = {"CANCEL","PREV.","APPLY"};
	const gchar *strVideo[] = {"60 Hz", "50 Hz"};

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DEFAULT VALUE WIZARD", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_tooltip(obj, FALSE);
	nfui_nfobject_set_size(obj, (parent->width) -70 - 8, 30);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, 4, 7);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("3 / 3", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nflabel_set_spacing(obj, SEMI_CONDENSED_SPACING);	
	nfui_nfobject_use_tooltip(obj, FALSE);
	nfui_nfobject_set_size(obj, 70, 30);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 10);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, 4 + ((parent->width) - 70 - 8), 7);

	obj = nfui_nflabel_new_with_pango_font("AC POWER FREQUENCY", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, 300, 40);	
	nfui_nffixed_put((NFFIXED*)parent, obj, 20, 100);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);

	obj = nfui_spinbutton_new(strVideo, 2, init_conf.sigType);
	obj_sig = obj;
	nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_1);
	nfui_nfobject_set_size(obj, 400, 40);	
	nfui_spin_button_set_align((NFSPINBUTTON*)obj, NFALIGN_CENTER, 0);
	nfui_nffixed_put((NFFIXED*)parent, obj, (parent->width) - 420, 100);	
	nfui_nfobject_show(obj);

	for(i=0; i<SIG_BUTTONS; i++)
	{
		btns[i] = nftool_normal_button_create_type1(strButton[i], 168);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(btns[i]), NFALIGN_CENTER, 0);	
		nfui_nfobject_show(btns[i]);
		nfui_nffixed_put((NFFIXED*)parent, btns[i], (parent->width - 10) - (174 * (SIG_BUTTONS - i)), (parent->height) - 60);
	}

	focus[PAGE_SIG] = btns[SIG_CANCEL];
	nfui_regi_post_event_callback(btns[SIG_CANCEL], post_cancelbutton_event_handler);
	nfui_regi_post_event_callback(btns[SIG_PREV], post_prev_event_handler);
	nfui_regi_post_event_callback(btns[SIG_APPLY], post_apply_event_handler);
}

static gboolean pre_page_nffixed_event_cb(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	NFWINDOW *popup;
	GdkDrawable *drawable;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	popup = (NFWINDOW*)(obj->parent);

	if(evt->type == GDK_EXPOSE)
	{       
		drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
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
	
		nfui_page_close(PGID_POPUPWND, (NFOBJECT*)popup);
	}
	
	return FALSE;
}

gboolean VW_fac_init(void)
{	
	NFOBJECT *nfwin, *nffixed;
	int i;
	gint width[PAGE_MAX] = { 650, 750, 750};
	gint height[PAGE_MAX] =	{ 250, 250, 250};	

	memset(&init_conf, 0x00, sizeof(FacInitData));
	DAL_get_fac_init_data(&init_conf);	
	g_memmove(&org_init_conf, &init_conf, sizeof(FacInitData));

	nfwin = (NFOBJECT*)nfui_nfwindow_new(NF_TOPWND, CONF_WIN_POS_X, CONF_WIN_POS_Y, CONF_WIN_SIZE_W, CONF_WIN_SIZE_H);
	nfui_nfobject_modify_bg(nfwin, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));	
	nfui_regi_post_event_callback(nfwin, post_win_event_handler);
	nffixed = (NFOBJECT*)nfui_nffixed_new();
	
	for(i=0; i < PAGE_MAX; i++)	
	{		
		page[i] = (NFOBJECT*)nfui_nffixed_new();
		nfui_nfobject_set_size(page[i], width[i], height[i]);		
		nfui_nffixed_put((NFFIXED*)nffixed, page[i], (CONF_WIN_SIZE_W-width[i])/2, (CONF_WIN_SIZE_H-height[i])/2);
		nfui_regi_pre_event_callback(page[i], pre_page_nffixed_event_cb);
	}

	nfui_nfobject_show(page[PAGE_LANG]);
	
	nfui_nfobject_show(nffixed);
	nfui_nfwindow_add((NFWINDOW*)nfwin, nffixed);

	nfui_run_main_event_handler(nfwin);

	nfui_page_open(PGID_FAC_INIT, nfwin, nfui_get_last_user());

	init_lang_open(page[PAGE_LANG]);
	init_timezone_open(page[PAGE_TZ]);	
	init_sigtype_open(page[PAGE_SIG]);

	nfui_nfwindow_set_returnkey_proc(nfwin, post_return_event_handler);

	nfui_nfobject_show(nfwin);
	nfui_make_key_hierarchy(nfwin);

	nfui_set_key_focus(focus[PAGE_LANG], TRUE);

	NFUTIL_THREADS_ENTER();
	gtk_main();
	NFUTIL_THREADS_LEAVE();

	return ret;
}
