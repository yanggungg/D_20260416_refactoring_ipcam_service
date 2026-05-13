/*
 * iux_afx.h
 * 	- header file for the iux framework
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Dec 26, 2010
 *
 */

#include <glib.h>
#include "iux_afx.h"
#include "vsm.h"
#include "scm.h"

#include "support/multi_language_support.h"
#include "support/event_loop.h"
#include "support/util.h"
#include "tools/nf_ui_function.h"
#include "cmm.h"
#include "nfdal.h"
#include "stm.h"
#include "smt.h"
#include "ix_func.h"
#include "ix_mem.h"
#include "ix_conf.h"
#include "uxm.h"
#include "ssm.h"
#include "vfs.h"
#include "vaa.h"
#include "dvaa.h"
#include "pos.h"
#include "cdump.h"
#include "feye.h"
#include "xmm.h"
#include "nvm.h"
#include "support/color_conf.h"
#include "iva_cntr.h"

#define DBG_LEVEL		9
#define DBG_MODULE		"AFX"
#define CONF_FILE		"./data/gui/uxconf.cfg"


//#define _GUI_SAMPLE

static int _initialized = 0;

const char *infy_desc[] = {    
    "INFY_GROUP_INIT",
    INFY_GROUP(DO_DESC)
    "INFY_GROUP_END"
};

const char *infy_nfnotify_desc[] = {    
    "INFY_NFNOTIFY_GROUP_INIT",
    INFY_NFNOTIFY_GROUP(DO_DESC)
    "INFY_NFNOTIFY_GROUP_END"
};

const char *ireq_desc[] = {    
    "IREQ_GROUP_INIT",
    IREQ_GROUP(DO_DESC)
    "IREQ_GROUP_END",
};

const char *irpl_desc[] = {    
    "IRPL_GROUP_INIT",
    IRPL_GROUP(DO_DESC)
    "IRPL_GROUP_END",
};

const char *iret_desc[] = {    
    "IRET_GROUP_INIT",
    IRET_GROUP(DO_DESC)
    "IRET_GROUP_END"
};

const char *iiret_desc[] = {    
    "iRET_GROUP_INIT",
    iRET_GROUP(DO_DESC)
    "iRET_GROUP_END"
};

const char *iinfy_desc[] = {    
    "iNFY_GROUP_INIT",
    iNFY_GROUP(DO_DESC)
    "iNFY_GROUP_END"
};

const char *iireq_desc[] = {    
    "iREQ_GROUP_INIT",
    iREQ_GROUP(DO_DESC)
    "iREQ_GROUP_END"
};


////////////////////////////////////////////////////////////
//
// protected interfaces
//

static int _init_gui_lib()
{
	if (!g_thread_supported()) g_thread_init(NULL);
	// gdk_threads_init();
	gtk_init(NULL, NULL);
	return 0;
}

extern int scm_set_notifycb();
static int _init_iux(int ch_count)
{
	if (init_imalloc() == 0) DMSG(1, "imalloc init completed\n");
	else g_assert(0);

	if (icf_init(CONF_FILE) == 0) DMSG(1, "conf init completed\n");
	else g_assert(0);
 
	if (var_init() == 0) DMSG(1, "var init completed\n");
	else g_assert(0);

	var_set_ch_count(ch_count);
	DAL_init(ch_count);

	if (vw_desc_init() == 0) DMSG(1, "viewer desc init completed\n");
	else g_assert(0);

	if (color_init() == 0) DMSG(1, "color init completed\n");
	else g_assert(0);

	if (vaa_init() == 0) DMSG(1, "vaa init completed\n");
	else DMSG(1, "not supported\n");

	if (dvaa_init() == 0) DMSG(1, "dvaa init completed\n");
	else DMSG(1, "not supported\n");

	if (iva_cntr_init() == 0) DMSG(1, "iva_cntr init completed\n");
	else DMSG(1, "not supported\n");

	if (posx_init() == 0) DMSG(1, "posx init completed\n");
	else g_assert(0);

	if (cmm_init() == 0) DMSG(1, "cmm init completed\n");
	else g_assert(0);

	if (uxm_init() == 0) DMSG(1, "uxm init completed\n");
	else g_assert(0);

	if (scm_init() == 0) DMSG(1, "scm init completed\n");
	else g_assert(0);

	if (vsm_init() == 0) DMSG(1, "vsm init completed\n");
	else g_assert(0);

	if (vw_init() == 0) DMSG(1, "viewer init completed\n");
	else g_assert(0);

	if (stm_init() == 0) DMSG(1, "stm init completed\n");
	else g_assert(0);

	if (smt_init() == 0) DMSG(1, "smt init completed\n");
	else g_assert(0);
	
	if (ssm_init() == 0) DMSG(1, "ssm init completed\n");
	else g_assert(0);

	if (vfs_init() == 0) DMSG(1, "vfs init completed\n");
	else g_assert(0);

	if (cdump_init() == 0) DMSG(1, "cdump init completed\n");
	else g_assert(0);
    
	if (feye_init() == 0) DMSG(1, "feye init completed\n");
	else g_assert(0);	

	if (xmm_init() == 0) DMSG(1, "xmm init completed\n");
	else g_assert(0);	
	
	if (nvm_init() == 0) DMSG(1, "nvm init completed\n");
	else g_assert(0);	
	
	scm_set_notifycb();
	dvaa_update_external_airules();
	return 0;	
}

static int _start()
{
	uxm_bootup();
	uxm_run();
	return 0;
}

#ifndef _GUI_SAMPLE

////////////////////////////////////////////////////////////
//
// public interfaces
//
int iux_make_msg_cfg();

int start_iux(int ch_count, int skp_sst_init)
{
    iux_make_msg_cfg();

	_init_gui_lib();
	_init_iux(ch_count);
	_initialized = 1;
	_start();

	return 0;
}

int is_iux_initialized()
{
  return _initialized;
}

const char *iux_translate_msg_desc(guint msg_enum)
{    
    if ((msg_enum > INFY_GROUP_INIT) && (msg_enum < INFY_GROUP_END))
    {        
        return infy_desc[msg_enum-INFY_GROUP_INIT];
    }    

    if ((msg_enum > INFY_NFNOTIFY_GROUP_INIT) && (msg_enum < INFY_NFNOTIFY_GROUP_END))
    {        
        return infy_nfnotify_desc[msg_enum-INFY_NFNOTIFY_GROUP_INIT];
    }

    if ((msg_enum > IREQ_GROUP_INIT) && (msg_enum < IREQ_GROUP_END))
    {
        return ireq_desc[msg_enum-IREQ_GROUP_INIT];
    }

    if ((msg_enum > IRPL_GROUP_INIT) && (msg_enum < IRPL_GROUP_END))
    {
        return irpl_desc[msg_enum-IRPL_GROUP_INIT];
    }

    if ((msg_enum > IRET_GROUP_INIT) && (msg_enum < IRET_GROUP_END))
    {
        return iret_desc[msg_enum-IRET_GROUP_INIT];
    }

    if ((msg_enum > iRET_GROUP_INIT) && (msg_enum < iRET_GROUP_END))
    {
        return iiret_desc[msg_enum-iRET_GROUP_INIT];
    }

    if ((msg_enum > iNFY_GROUP_INIT) && (msg_enum < iNFY_GROUP_END))
    {
        return iinfy_desc[msg_enum-iNFY_GROUP_INIT];
    }

    if ((msg_enum > iREQ_GROUP_INIT) && (msg_enum < iREQ_GROUP_END))
    {
        return iireq_desc[msg_enum-iREQ_GROUP_INIT];
    }
    
    g_warning("%s, %d, msg_enum:%08X", __FUNCTION__, __LINE__, msg_enum);

    return NULL;
}

guint iux_translate_msg_enum(const char *msg_desc)
{    
    int i, cnt;
    
    if (!msg_desc) return 0;

    if (strncmp(msg_desc, "INFY_", 5))    
    {        
        cnt = INFY_GROUP_END-INFY_GROUP_INIT+1;   
        
        for (i = 0; i < cnt; i++)        
        {            
            if (strcmp(infy_desc[i], msg_desc)) break;        
        }        
        
        if (i != cnt) {            
            return INFY_GROUP_INIT+i;        
        }    

        cnt = INFY_NFNOTIFY_GROUP_END-INFY_NFNOTIFY_GROUP_INIT+1;   
        
        for (i = 0; i < cnt; i++)        
        {            
            if (strcmp(infy_nfnotify_desc[i], msg_desc)) break;        
        }        
        
        if (i != cnt) {            
            return INFY_NFNOTIFY_GROUP_INIT+i;        
        }
    }    
    
    if (strncmp(msg_desc, "IREQ_", 5))    
    {        
        cnt = IREQ_GROUP_END-IREQ_GROUP_INIT+1;            
        
        for (i = 0; i < cnt; i++)        
        {            
            if (strcmp(ireq_desc[i], msg_desc)) break;        
        }        
    
        if (i != cnt) {            
            return IREQ_GROUP_INIT+i;        
        }    
    }  
    
    if (strncmp(msg_desc, "IRPL_", 5))    
    {        
        cnt = IRPL_GROUP_END-IRPL_GROUP_INIT+1;            
        
        for (i = 0; i < cnt; i++)        
        {            
            if (strcmp(irpl_desc[i], msg_desc)) break;        
        }
        
        if (i != cnt) 
        {            
            return IRPL_GROUP_INIT+i;
        }    
    }    

    if (strncmp(msg_desc, "IRET_", 5))    
    {        
        cnt = IRET_GROUP_END-IRET_GROUP_INIT+1;            
        
        for (i = 0; i < cnt; i++)        
        {            
            if (strcmp(iret_desc[i], msg_desc)) break;        
        }
        
        if (i != cnt) 
        {            
            return IRET_GROUP_INIT+i;
        }    
    }    

    if (strncmp(msg_desc, "iRET_", 5))    
    {        
        cnt = iRET_GROUP_END-iRET_GROUP_INIT+1;            
        
        for (i = 0; i < cnt; i++)        
        {            
            if (strcmp(iiret_desc[i], msg_desc)) break;        
        }
        
        if (i != cnt) 
        {            
            return iRET_GROUP_INIT+i;
        }    
    }    

    if (strncmp(msg_desc, "iNFY_", 5))    
    {        
        cnt = iNFY_GROUP_END-iNFY_GROUP_INIT+1;            
        
        for (i = 0; i < cnt; i++)        
        {            
            if (strcmp(iinfy_desc[i], msg_desc)) break;        
        }
        
        if (i != cnt) 
        {            
            return iNFY_GROUP_INIT+i;
        }    
    }  

    if (strncmp(msg_desc, "iREQ_", 5))    
    {        
        cnt = iREQ_GROUP_END-iREQ_GROUP_INIT+1;            
        
        for (i = 0; i < cnt; i++)        
        {            
            if (strcmp(iireq_desc[i], msg_desc)) break;        
        }
        
        if (i != cnt) 
        {            
            return iREQ_GROUP_INIT+i;
        }    
    }  
    
    g_warning("%s, %d, msg_desc:%s", __FUNCTION__, __LINE__, msg_desc);    
    
    return 0;
}

int iux_make_msg_cfg()
{
	FILE *fp = NULL;
	gchar fname[128];
	gint i;

    g_message("%s, %d, __called", __FUNCTION__, __LINE__);
	
    memset(fname, 0x00, sizeof(fname));
    sprintf(fname, "/tmp/iux_msg.cfg");

	if ((fp = fopen(fname, "w")) == NULL) return -1;

    fprintf(fp, "<INFY_GROUP>\n");
    
    for (i = 0; i < INFY_GROUP_END-INFY_GROUP_INIT+1; i++)        
    {            
        fprintf(fp, "   0x%08X : %s\n", INFY_GROUP_INIT+i, infy_desc[i]);     
    }        

    fprintf(fp, "\n");
    fprintf(fp, "<INFY_NFNOTIFY_GROUP>\n");
    
    for (i = 0; i < INFY_NFNOTIFY_GROUP_END-INFY_NFNOTIFY_GROUP_INIT+1; i++)        
    {            
        fprintf(fp, "   0x%08X : %s\n", INFY_NFNOTIFY_GROUP_INIT+i, infy_nfnotify_desc[i]);     
    }        

    fprintf(fp, "\n");
    fprintf(fp, "<IREQ_GROUP>\n");
    
    for (i = 0; i < IREQ_GROUP_END-IREQ_GROUP_INIT+1; i++)        
    {            
        fprintf(fp, "   0x%08X : %s\n", IREQ_GROUP_INIT+i, ireq_desc[i]);     
    }        

    fprintf(fp, "\n");
    fprintf(fp, "<IRPL_GROUP>\n");
    
    for (i = 0; i < IRPL_GROUP_END-IRPL_GROUP_INIT+1; i++)        
    {            
        fprintf(fp, "   0x%08X : %s\n", IRPL_GROUP_INIT+i, irpl_desc[i]);     
    }        

    fprintf(fp, "\n");
    fprintf(fp, "<IRET_GROUP>\n");
    
    for (i = 0; i < IRET_GROUP_END-IRET_GROUP_INIT+1; i++)        
    {            
        fprintf(fp, "   0x%08X : %s\n", IRET_GROUP_INIT+i, iret_desc[i]);     
    }        

    fprintf(fp, "\n");
    fprintf(fp, "<iRET_GROUP>\n");
    
    for (i = 0; i < iRET_GROUP_END-iRET_GROUP_INIT+1; i++)        
    {            
        fprintf(fp, "   0x%08X : %s\n", iRET_GROUP_INIT+i, iiret_desc[i]);     
    }        

    fprintf(fp, "\n");
    fprintf(fp, "<iNFY_GROUP>\n");
    
    for (i = 0; i < iNFY_GROUP_END-iNFY_GROUP_INIT+1; i++)        
    {            
        fprintf(fp, "   0x%08X : %s\n", iNFY_GROUP_INIT+i, iinfy_desc[i]);     
    }        

    fprintf(fp, "\n");
    fprintf(fp, "<iREQ_GROUP>\n");
    
    for (i = 0; i < iREQ_GROUP_END-iREQ_GROUP_INIT+1; i++)        
    {            
        fprintf(fp, "   0x%08X : %s\n", iREQ_GROUP_INIT+i, iireq_desc[i]);     
    }        

	fclose(fp);

    return 0;
}

#else

///////////////////////////////////////////////////////////////////////////////////
//
// Belows are just test code
//
//
//
//
//

#include "gui/nf_afx.h"
#include <gtk/gtk.h>

#include <glib.h> 
#include <glib-object.h>
#include <glib/gprintf.h>


#include "support/event_loop.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_page_manager.h"
#include "support/color.h"

#include "tools/nf_ui_tool.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nfbutton.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfcombobox.h"
#include "objects/nfspinbutton.h"
#include "objects/nfbutton.h"
#include "objects/nflistbox.h"
#include "vw_vkeyboard.h"

static NFOBJECT *g_window;
static NFOBJECT *subject;
static NFOBJECT *value;
static NFOBJECT *label;
static NFOBJECT *cmb_obj;

static GdkPixbuf *bg = NULL;
static GdkPixbuf *spin_up[NFOBJECT_STATE_COUNT];
static GdkPixbuf *spin_down[NFOBJECT_STATE_COUNT];

#define NUM_STATUSBAR_TIMEOUT 8
static const gchar *strTimeout[NUM_STATUSBAR_TIMEOUT] = {
	"AUTO HIDE",
	"ALWAYS ON",
	"5 SEC",
	"10 SEC",
	"15 SEC",
	"20 SEC",
	"30 SEC",
	"1 MIN"
};

const gchar *strOffOn[] = {"OFF", "ON"};

GdkColor color[4] = {
	{ 0x00, 0xFF00, 0x0000, 0x0000 },	// red
	{ 0x00, 0xFF00, 0xFF00, 0x0000 },	// yellow
	{ 0x00, 0x0100, 0x0100, 0x0100 },	// black, not zero
	{ 0x00, 0xFF00, 0xFF00, 0xFF00 }	// white
};

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		gtk_main_quit();
	}

	return FALSE;
}

static gboolean pre_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{      
	GdkGC *gc;
	GdkDrawable *drawable;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;	

	switch(event->type) {
		case GDK_EXPOSE:
		{
    		drawable = nfui_nfobject_get_window(obj);
    		gc = nfui_nfobject_get_gc(obj);

            nfui_nfobject_get_size(obj, &size_w, &size_h);
            pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
            nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
        
    		nfui_nfobject_gc_unref(gc);
    	}
		break;

		case GDK_BUTTON_RELEASE:
		break;					

		case GDK_DELETE:
		{
            nfui_nfobject_get_size(obj, &size_w, &size_h);
            nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
		}
		break;

		default:
		break;
	}

	return FALSE;
}

static NFOBJECT 	*df_list_obj = NULL;
static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *topwin;
	char *ptext;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		ptext = nfui_listbox_get_focus_text((NFLISTBOX*)df_list_obj, 0);
		printf("selected item = [%s]\n", ptext);

		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);

	}

	return FALSE;
}

static int _open_dialog(NFWINDOW *parent)
{
	NFOBJECT *sds_win;
	NFOBJECT *main_fixed;
	NFOBJECT *obj;
	guint lc_size[] = {250, };
	GdkPixbuf *scroll_up[NFOBJECT_STATE_COUNT];
	GdkPixbuf *scroll_down[NFOBJECT_STATE_COUNT];

	guint li_size_w, li_size_h;
	char *item[] = {
		"ITEM 1",
		"ITEM 2",
		"ITEM 3"
	};

	scroll_up[0] = nfui_get_image_from_file((IMG_N_SCROLL_UP), NULL);
	scroll_up[1] = nfui_get_image_from_file((IMG_O_SCROLL_UP), NULL);	
	scroll_up[2] = nfui_get_image_from_file((IMG_P_SCROLL_UP), NULL);	
	scroll_up[3] = nfui_get_image_from_file((IMG_N_SCROLL_UP), NULL);	

	scroll_down[0] = nfui_get_image_from_file((IMG_N_SCROLL_DOWN), NULL);
	scroll_down[1] = nfui_get_image_from_file((IMG_O_SCROLL_DOWN), NULL);
	scroll_down[2] = nfui_get_image_from_file((IMG_P_SCROLL_DOWN), NULL);
	scroll_down[3] = nfui_get_image_from_file((IMG_N_SCROLL_DOWN), NULL);

	nfui_get_pixbuf_size(scroll_up[0], &li_size_w, &li_size_h);



// <---- WINDOW
	sds_win = (NFOBJECT*)nfui_nfwindow_new(parent, 500, 400, 440, 540);
	nfui_nfobject_modify_bg(sds_win, NFOBJECT_STATE_NORMAL, COLOR_IDX(NOT_CARE));
	nfui_regi_post_event_callback(sds_win, post_main_win_event_handler);

// <---- FIXED
	main_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(main_fixed, 440, 540);
	nfui_regi_pre_event_callback(main_fixed, pre_main_fixed_event_handler);
	nfui_nfobject_show(main_fixed);


	df_list_obj = nfui_listbox_new(1, lc_size, 40);	
	nfui_listbox_set_skin_type(NF_LISTBOX(df_list_obj), NFLISTBOX_TYPE_1);
    nfui_listbox_support_multi_lang(NF_LISTBOX(df_list_obj), FALSE);
	nfui_nfobject_use_focus(df_list_obj, NFOBJECT_FOCUS_ON);
	nfui_nfobject_set_size(df_list_obj, 250, 300);
	nfui_nfobject_show(df_list_obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, df_list_obj, 30, 70);

	nfui_listbox_delete_all(NF_LISTBOX(df_list_obj));
	nfui_listbox_set_text(NF_LISTBOX(df_list_obj), &item[0]);
	nfui_listbox_set_text(NF_LISTBOX(df_list_obj), &item[1]);
	nfui_listbox_set_text(NF_LISTBOX(df_list_obj), &item[2]);

// <---- CANCEL BUTTON
	obj = nftool_normal_button_create_type1("CLOSE", 200);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 220, 450);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);


	nfui_nfwindow_add((NFWINDOW*)sds_win, main_fixed);
	nfui_run_main_event_handler(sds_win);
	nfui_nfobject_show(sds_win);

	// PGID_SYS_LOAD_DATA_POPUP is a just sample
	nfui_page_open(PGID_SYS_LOAD_DATA_POPUP, sds_win, ssm_get_cur_id(NULL));

	gtk_main();

	nfui_page_close(PGID_SYS_LOAD_DATA_POPUP, sds_win);
}

static gboolean post_fixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	char buf[64];
	GdkDrawable *drawable;
	GdkGC *gc;

	if(event->type == GDK_EXPOSE) {

	} else if(event->type == GDK_DELETE) {
		NFOBJECT *top;

		top = nfui_nfobject_get_top(obj);

		g_object_unref(bg);
		nfui_page_close(PGID_USERPWD, top);
	}
	else if (event->type == GDK_MOTION_NOTIFY) {
		drawable = nfui_nfobject_get_window((NFOBJECT*)g_window);
		gc = gdk_gc_new(drawable);

		sprintf(buf, "%u, %u", (guint)event->motion.x, (guint)event->motion.y);
		nfutil_draw_text_with_pango(NULL, NULL, NULL, drawable, gc,
				buf, event->motion.x, event->motion.y,
				100, 20,
				nffont_get_pango_font(NFFONT_MINI_SEMI_1),
				&color[3], NFALIGN_CENTER, 0);

		g_object_unref(gc);
	}

	return FALSE;
}

static gboolean window_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{

	return FALSE;
}

#define COLOR2INT(a) ((a.red >> 8) << 24 | (a.green >> 8) << 16 | (a.blue >> 8) << 8 | 0xff)
static int pp = 0;
static int _draw_shapes(NFWINDOW *win, int x, int y, int w, int h)
{
	GdkGC *gc;
	GdkDrawable *drawable;

	int randx;
	int randy;
	int rand3;
	int rand4;

	GdkPoint pt[3];

	randx = ifn_rand() % 1920;
	randy = ifn_rand() % 1080 + 130;
	pt[0].x = randx;
	pt[0].y = randy;

	randx = ifn_rand() % 1920;
	randy = ifn_rand() % 1080 + 130;
	pt[1].x = randx;
	pt[1].y = randy;

	randx = ifn_rand() % 1920;
	randy = ifn_rand() % 1080 + 130;
	pt[2].x = randx;
	pt[2].y = randy;

	drawable = nfui_nfobject_get_window((NFOBJECT*)win);
	gc = gdk_gc_new(drawable);

	pp = !pp;
	
#if 0
	gdk_gc_set_rgb_fg_color(gc, &color[pp]);

	rand3 = ifn_rand() % 1920/2;
	rand4 = ifn_rand() % 1080/2;
	gdk_draw_rectangle(drawable, gc, TRUE, randx, randy, rand3, rand4);

	gdk_gc_set_rgb_fg_color(gc, &color[2]);
	gdk_draw_line(drawable, gc,
			randx, 100,
			randy, 200);

	gdk_gc_set_rgb_fg_color(gc, &color[2]);
	gdk_draw_polygon(drawable, gc,
						FALSE, pt, 3);

#else
/*
	gdk_pixbuf_fill(bg, COLOR2INT(color[pp]));
	gdk_draw_pixmap(drawable, gc, bg, 
			0, 0, 
			x, y,
			w, h,
			GDK_RGB_DITHER_NORMAL, 0, 0);
*/

	gdk_gc_set_rgb_fg_color(gc, &color[pp]);

	rand3 = ifn_rand() % 1920/2;
	rand4 = ifn_rand() % 1080/2;

	ifn_init_time_elap();
	gdk_draw_rectangle(drawable, gc, TRUE, 0, 0, 1920, 1080);
	ifn_print_time_elap();


#endif
	g_object_unref(gc);
	return 0;
}

static gboolean post_draw_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch (evt->type)
	{
		case GDK_BUTTON_PRESS:
//		_open_dialog(g_window);
			_draw_shapes(g_window, 0, 0, 1920, 1080);
			break;

	}
	return FALSE;
}

static int _erase(NFWINDOW *win)
{
	GdkGC *gc;
	GdkDrawable *drawable;

	drawable = nfui_nfobject_get_window((NFOBJECT*)win);
	gc = gdk_gc_new(drawable);

	gdk_gc_set_rgb_fg_color(gc, &color[2]);
	gdk_draw_rectangle(drawable, gc, TRUE, 0, 130, 1920, 1080 - 130);

	g_object_unref(gc);
	return 0;
}

static gboolean post_erase_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch (evt->type)
	{
		case GDK_BUTTON_PRESS:
			_erase(g_window);
			break;

	}
	return FALSE;
}

static gboolean post_getdata_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	int idx;

	switch (evt->type)
	{
		case GDK_BUTTON_PRESS:
			idx = nfui_spin_button_get_index((NFSPINBUTTON*)(value));
			printf("SPIN INDEX = %d\n", idx);
			break;

	}
	return FALSE;
}

static gboolean post_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid;

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{
		NFOBJECT *top;
		gchar *strTemp;
		guint x, y;
		gint string_size;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
		  return FALSE;
		}
	
		nfui_nfobject_get_window_pos(obj, &x, &y);

		x += ((GdkEventButton*)evt)->x;
		y += ((GdkEventButton*)evt)->y;


		strTemp = VirtualKey_Open(g_window, nfui_nflabel_get_text((NFLABEL*)obj), x, y, 63, VKEY_NORMAL);

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

static gboolean post_test_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED) {
	}

	return FALSE;
}

static gboolean post_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint index;
	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {	

		index = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));
		printf("combo index = %d\n", index);
	}

	return FALSE;
}

static gboolean _proc_timer(void *data)
{
	printf("TIMER = %u\n", time(0));

	/* if you want to stop the timer, return FALSE */

	return TRUE;
}




int start_iux(int ch_count, int skp_sst_init)
{
	NFOBJECT *fixed;
	NFOBJECT *obj;
	GdkPixmap *div_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *btn[NFOBJECT_STATE_COUNT];
	guint font_color[NFOBJECT_STATE_COUNT] = {
				COLOR_IDX(NOT_CARE), 
				COLOR_IDX(NOT_CARE), 
				COLOR_IDX(NOT_CARE), 
				COLOR_IDX(NOT_CARE)
	};

	GdkPixbuf *chk_img[6];
	g_message("############################## 1 #############################");

	_init_gui_lib();

	if (init_imalloc() == 0) DMSG(1, "imalloc init completed\n");
	else g_assert(0);

	if (var_init() == 0) DMSG(1, "var init completed\n");
	else g_assert(0);

	var_set_ch_count(ch_count);

	if (color_init() == 0) DMSG(1, "color init completed\n");
	else g_assert(0);

	
	g_message("############################## 2 #############################");

	/* ready multi language and font */
	if (init_multi_language_support("ENGLISH") == 0)	{
		DMSG(9, "\n\n\ninit_multi_language_support success \n\n\n");
	}
	else {
		DMSG(9, "\n\n\ninit_multi_language_support error !0\n\n\n");
	}

    nfui_preload_image();
	nfui_create_image();

	g_message("############################## 3 #############################");

	/* make button image */
	btn[0] = nfui_get_image_from_file(("bt_live_start_n.png"), NULL);
	g_message("############################## 4 #############################");
	btn[1] = nfui_get_image_from_file(("bt_live_start_o.png"), NULL);
	btn[2] = nfui_get_image_from_file(("bt_live_start_p.png"), NULL);
	btn[3] = nfui_get_image_from_file(("bt_live_start_d.png"), NULL);

	g_message("############################## 5 #############################");

	g_message("############################## 6 #############################");


	bg = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, 1920, 1080);
//	bg = gdk_pixbuf_map();


	// window
	g_window = (NFOBJECT*)nfui_nfwindow_new(NF_TOPWND, 0, 0, 1920, 1080);
	nfui_nfobject_modify_bg(g_window, NFOBJECT_STATE_NORMAL, COLOR_IDX(NOT_CARE));

	nfui_regi_post_event_callback(g_window, window_event_handler);


	// fixed
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, 400, 300);
	nfui_regi_post_event_callback(fixed, post_fixed_event_handler);
	nfui_nfobject_show(fixed);


	/* button */
	obj = nfui_nfbutton_new();
	nfui_nfbutton_set_image((NFBUTTON*)obj, btn);
	nfui_nfbutton_set_pango_font(obj, "Calibri 12", font_color);
	nfui_nfbutton_set_text(obj, "DRAW");
	nfui_nfobject_set_size(obj, 92, 92);
	nfui_regi_post_event_callback(obj, post_draw_button_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 20, 120);

	/* button */
	obj = nfui_nfbutton_new();
	nfui_nfbutton_set_image((NFBUTTON*)obj, btn);
	nfui_nfbutton_set_pango_font(obj, "Calibri 12", font_color);
	nfui_nfbutton_set_text(obj, "ERASE");
	nfui_nfobject_set_size(obj, 92, 92);
	nfui_regi_post_event_callback(obj, post_erase_button_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 140, 120);

	/* button */
	obj = nfui_nfbutton_new();
	nfui_nfbutton_set_image((NFBUTTON*)obj, btn);
	nfui_nfbutton_set_pango_font(obj, "Calibri 12", font_color);
	nfui_nfbutton_set_text(obj, "GETDATA");
	nfui_nfobject_set_size(obj, 92, 92);
	nfui_regi_post_event_callback(obj, post_getdata_button_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 260, 120);



	font_color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(NOT_CARE);
	chk_img[0] = nf_ui_create_image_button_method("MKB_IMG_BTN5_N_164", 164, IMG_BTN5_N_L, IMG_BTN5_N_M, IMG_BTN5_N_R);
	chk_img[1] = nf_ui_create_image_button_method("MKB_IMG_BTN5_O_164", 164, IMG_BTN5_O_L, IMG_BTN5_O_M, IMG_BTN5_O_R);
	chk_img[2] = nf_ui_create_image_button_method("MKB_IMG_BTN5_P_164", 164, IMG_BTN5_P_L, IMG_BTN5_P_M, IMG_BTN5_P_R);
	chk_img[3] = nf_ui_create_image_button_method("MKB_IMG_BTN5_S_164", 164, IMG_BTN5_S_L, IMG_BTN5_S_M, IMG_BTN5_S_R);
	chk_img[4] = nf_ui_create_image_button_method("MKB_IMG_BTN5_D_164", 164, IMG_BTN5_D_L, IMG_BTN5_D_M, IMG_BTN5_D_R);
	chk_img[5] = nf_ui_create_image_button_method("MKB_IMG_BTN5_D_164", 164, IMG_BTN5_D_L, IMG_BTN5_D_M, IMG_BTN5_D_R);

#if 0
	obj = (NFOBJECT*)nfui_checkbutton_new(FALSE);
	nfui_check_set_text(obj, "TEST ALL");
	nfui_check_button_set_image(obj, chk_img);
	nfui_check_set_pango_font(obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), font_color);
	nfui_regi_post_event_callback(obj, post_test_event_handler);
	nfui_nfobject_set_size(obj, 164, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 360, 120);
#endif

	/* label */
//	subject = (NFOBJECT*)nfui_nflabel_new("TEST");
	subject = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ITX",
							nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(NOT_CARE));
	nfui_nflabel_set_align((NFLABEL*)subject, NFALIGN_LEFT, 20);
	nfui_nfobject_modify_bg(subject, NFOBJECT_STATE_NORMAL, COLOR_IDX(NOT_CARE));
	nfui_nfobject_use_focus(subject, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(subject, 200, 40);
	nfui_nfobject_show(subject);
	nfui_nffixed_put((NFFIXED*)fixed, subject, 50, 50);
	nfui_nfwindow_add((NFWINDOW*)g_window, fixed);
	nfui_run_main_event_handler(g_window);
	nfui_nfobject_show(g_window);


	/* spin button */
	value = (NFOBJECT*)nfui_spinbutton_new((gchar**)strTimeout, NUM_STATUSBAR_TIMEOUT, 0);
	nfui_spinbutton_set_skin_type((NFSPINBUTTON*)value, NFSPINBUTTON_TYPE_1);
	nfui_spin_button_set_align((NFSPINBUTTON*)value, NFALIGN_LEFT, 4);
	nfui_spin_button_set_spacing((NFSPINBUTTON*)value, CONDENSED_SPACING);
	nfui_nfobject_set_size(value, 250, 40);
	nfui_nfobject_show(value);
	nfui_nffixed_put((NFFIXED*)fixed, value, 340, 50);


	/* label */
	label = nfui_nflabel_new_with_pango_font("default", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(NOT_CARE));
	nfui_nflabel_set_spacing((NFLABEL *)label, SEMI_CONDENSED_SPACING);
	nfui_nfimglabel_set_align((NFLABEL*)label, NFALIGN_LEFT);
	nfui_nfobject_set_size(label, 278, 40);
	nfui_nfobject_modify_bg(label, NFOBJECT_STATE_NORMAL, COLOR_IDX(NOT_CARE));
	nfui_nfobject_show(label);
	nfui_regi_post_event_callback(label, post_label_event_handler);

	nfui_nffixed_put((NFFIXED*)fixed, label, 660, 50);



	/* combo box */
	obj = nfui_combobox_new(strOffOn, 2, 0);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, 400, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 960, 50);
	nfui_regi_post_event_callback(obj, post_combo_event_handler);
	cmb_obj = obj;


	/* timer */
	g_timeout_add(1000, _proc_timer, 0);


	// main loop 
	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();
	
}
#endif

