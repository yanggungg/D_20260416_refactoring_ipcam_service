#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfspinbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nfbutton.h"
#include "objects/nfimage.h"

#include "scm.h"

#include "vw_vkeyboard.h"
#include "vw_sys_disp_main.h"
#include "vw_sys_disp_posatm.h"
#include "vw_change_color_popup.h"

#include "nf_sysman.h"



#define TABLE_LEFT              (8)
#define TABLE_TOP               (0)

#define LABEL_HEIGHT            (40)


enum {
    DMB_CANCEL = 0,
    DMB_APPLY,
    DMB_CLOSE,
    DMB_BUTTONS
};

static NFWINDOW *g_curwnd = 0;

static NFOBJECT *g_mode_obj;
static NFOBJECT *g_position_obj;
static NFOBJECT *g_font_obj;
static NFOBJECT *g_ncolor_obj;
static NFOBJECT *g_duration_obj;
//static NFOBJECT *g_format_obj;
static NFOBJECT *g_scroll_obj;
static NFOBJECT *g_highlight_obj;
static NFOBJECT *g_highlight_text_obj[8];
static NFOBJECT *g_highlight_color_obj[8];
static NFOBJECT *g_exclude_obj;
static NFOBJECT *g_exclude_text_obj[8];

static PosOsdData posdata;
static PosOsdData org_posdata;


static gint _prvRgbToColorIdx(guint rgb_col)
{
    gint idx = COLOR_PRG_IDX(UX_COLOR_FFFFFF);

    switch(rgb_col)
    {
        case 0xffffff: idx = COLOR_PRG_IDX(UX_COLOR_FFFFFF); break;
        case 0x808080: idx = COLOR_PRG_IDX(UX_COLOR_808080); break;
        case 0xffff00: idx = COLOR_PRG_IDX(UX_COLOR_FFFF00); break;
        case 0x0000ff: idx = COLOR_PRG_IDX(UX_COLOR_0000FF); break;
        case 0x00ff00: idx = COLOR_PRG_IDX(UX_COLOR_00FF00); break;
        case 0xff0000: idx = COLOR_PRG_IDX(UX_COLOR_FF0000); break;        
        default: break;
    }

    return idx;
}

static guint _prvColorIdxToRgb(gint idx)
{
    guint rgb_col = 0xffffff;

    if (idx == COLOR_PRG_IDX(UX_COLOR_FFFFFF)) rgb_col = 0xffffff;
    else if (idx == COLOR_PRG_IDX(UX_COLOR_808080)) rgb_col = 0x808080;
    else if (idx == COLOR_PRG_IDX(UX_COLOR_FFFF00)) rgb_col = 0xffff00;
    else if (idx == COLOR_PRG_IDX(UX_COLOR_0000FF)) rgb_col = 0x0000ff;
    else if (idx == COLOR_PRG_IDX(UX_COLOR_00FF00)) rgb_col = 0x00ff00;
    else if (idx == COLOR_PRG_IDX(UX_COLOR_FF0000)) rgb_col = 0xff0000;

    return rgb_col;
} 

static gint _prvRgbToComboIdx(guint rgb_col)
{
    gint idx = 0;

    switch(rgb_col)
    {
        case 0xffffff: idx = 0; break;
        case 0x808080: idx = 1; break;
        case 0xffff00: idx = 2; break;
        case 0x0000ff: idx = 3; break;
        case 0x00ff00: idx = 4; break;
        case 0xff0000: idx = 5; break;        
        default: break;
    }

    return idx;
}

static guint _prvComboIdxToRgb(gint idx)
{
    guint rgb_col = 0xffffff;

    if (idx == 0) rgb_col = 0xffffff;
    else if (idx == 1) rgb_col = 0x808080;
    else if (idx == 2) rgb_col = 0xffff00;
    else if (idx == 3) rgb_col = 0x0000ff;
    else if (idx == 4) rgb_col = 0x00ff00;
    else if (idx == 5) rgb_col = 0xff0000;

    return rgb_col;
} 

static guint _prvDurationToIndex(guint duration)
{
	gint ret = 0;

	if (duration == 1) ret = 1;
	else if (duration == 3) ret = 2;
	else if (duration == 5) ret = 3;
	else if (duration == 10) ret = 4;
	else if (duration == 15) ret = 5;
	else if (duration == 30) ret = 6;
	else if (duration == 60) ret = 7;
	else ret = 0;

	return ret;
}

static guint _prvIndexToDuration(gint index)
{
	guint ret = 0;

	switch(index)
	{
		case 0:     ret = 0;		break;
		case 1:		ret = 1;		break;
		case 2:		ret = 3;		break;
		case 3:		ret = 5;		break;
		case 4:		ret = 10;		break;
		case 5:		ret = 15;		break;
		case 6:		ret = 30;		break;
		case 7:		ret = 60;		break;
		default:	ret = 0;		break;
	}

	return ret;
}

static void _prvSetDataToObjects(gint expose)
{
    gint i, index;

    nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_mode_obj, posdata.disp_mode);
    nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_position_obj, posdata.position);
    nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_font_obj, posdata.font);
    index = _prvRgbToComboIdx(posdata.normal_color);    
    nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_ncolor_obj, index);
    index = _prvDurationToIndex(posdata.duration);    
    nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_duration_obj, index);
//    nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_format_obj, posdata.format);
    nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_scroll_obj, posdata.scroll);    
    nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_highlight_obj, posdata.highlight);    
    nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_exclude_obj, posdata.exclude);    

    for (i = 0; i < 4; i++)
    {
        nfui_nflabel_set_text((NFLABEL*)g_highlight_text_obj[i], posdata.highlight_text[i]);
        index = _prvRgbToColorIdx(posdata.highlight_color[i]);
        nfui_nfobject_modify_bg(g_highlight_color_obj[i], NFOBJECT_STATE_NORMAL, index);        
    }
    
    for (i = 0; i < 4; i++)
    {
        nfui_nflabel_set_text((NFLABEL*)g_exclude_text_obj[i], posdata.exclude_text[i]);
    }

    if (expose)
    {
        nfui_signal_emit(g_mode_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_position_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_font_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_ncolor_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_duration_obj, GDK_EXPOSE, TRUE);
//        nfui_signal_emit(g_format_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_scroll_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_highlight_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_exclude_obj, GDK_EXPOSE, TRUE);        

        for (i = 0; i < 4; i++)
        {
            nfui_signal_emit(g_highlight_text_obj[i], GDK_EXPOSE, TRUE);
        }
        
        for (i = 0; i < 4; i++)
        {
            nfui_signal_emit(g_exclude_text_obj[i], GDK_EXPOSE, TRUE);
        }        
    }    
}

static void _prvLoadDataFromObjects()
{
    gint i, index;
    guint color;

    posdata.disp_mode = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_mode_obj);
    posdata.position = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_position_obj);
    posdata.font = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_font_obj);
    index = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_ncolor_obj);
    posdata.normal_color = _prvComboIdxToRgb(index);
    index = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_duration_obj);
    posdata.duration = _prvIndexToDuration(index);
//    posdata.format = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_format_obj);
    posdata.scroll = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_scroll_obj);
    posdata.highlight = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_highlight_obj);
    posdata.exclude = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_exclude_obj);

    for (i = 0; i < 4; i++)
    {
        memset(posdata.highlight_text[i], 0x00, sizeof(posdata.highlight_text[i]));
    
        if (strlen(nfui_nflabel_get_text((NFLABEL*)g_highlight_text_obj[i])))
        {
            strcpy(posdata.highlight_text[i], nfui_nflabel_get_text((NFLABEL*)g_highlight_text_obj[i]));
        }

        index = nfui_nfobject_get_bg_color(g_highlight_color_obj[i], NFOBJECT_STATE_NORMAL);
        posdata.highlight_color[i] = _prvColorIdxToRgb(index);
    }
    
    for (i = 0; i < 4; i++)
    {
        memset(posdata.exclude_text[i], 0x00, sizeof(posdata.highlight_text[i]));
    
        if (strlen(nfui_nflabel_get_text((NFLABEL*)g_exclude_text_obj[i])))
        {
            strcpy(posdata.exclude_text[i], nfui_nflabel_get_text((NFLABEL*)g_exclude_text_obj[i]));
        }
    }
}

static gboolean post_highlight_onoff_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        gint index, i;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        for (i = 0; i < 4; i++)
        {
            if (index == 0)
            {
                nfui_nfobject_disable(g_highlight_text_obj[i]);
                nfui_nfobject_disable(g_highlight_color_obj[i]);
            }
            else
            {
                nfui_nfobject_enable(g_highlight_text_obj[i]);
                nfui_nfobject_enable(g_highlight_color_obj[i]);
            }

            nfui_signal_emit(g_highlight_text_obj[i], GDK_EXPOSE, TRUE);
            nfui_signal_emit(g_highlight_color_obj[i], GDK_EXPOSE, TRUE);            
        }
    }

    return FALSE;
}

static gboolean post_highlight_text_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;
    gchar *title;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER )
	{
		NFOBJECT *top;
		gchar *strTemp;
		guint x, y;
        mb_type ret = NFTOOL_MB_OK;

		if(kpid == KEYPAD_ENTER)
		{
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);

			x += (obj->width)/2 + top->x;
			y += obj->height + top->y;
		}
		else
		{
	  	   	if(evt->button.button == MOUSE_RIGTH_BUTTON)
			{					
				return FALSE;
	  	   	}
		
			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += (guint)((GdkEventButton*)evt)->x;
			y += (guint)((GdkEventButton*)evt)->y;
		}

        title = nfui_nflabel_get_text((NFLABEL*)obj);
       
   		strTemp = VirtualKey_Open(g_curwnd, title, x, y, 16, VKEY_NORMAL);        

		if (strTemp) 
		{
			if (ret == NFTOOL_MB_OK)
			{
				nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
				nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
			}
		
			ifree(strTemp);
			strTemp = NULL;
		}
	}
	return FALSE;
}

static gboolean post_highlight_color_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if ((evt->type == GDK_EXPOSE) || (evt->type == GDK_LEAVE_NOTIFY))
    {
		GdkGC *gc;
		GdkDrawable *drawable;
		gint gap_x, gap_y;

        if (nfui_nfobject_is_disabled(obj)) return FALSE;

		nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);
	
		drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
		gc = gdk_gc_new(drawable);

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(200));
    	gdk_gc_set_line_attributes(gc, 2, GDK_LINE_SOLID, GDK_CAP_NOT_LAST, GDK_JOIN_MITER);			
		gdk_draw_rectangle(drawable, gc, FALSE, gap_x+1, gap_y+1, obj->width-2, obj->height-2);
 		g_object_unref(gc);
 	}
    else if (evt->type == GDK_BUTTON_RELEASE)
    {
        VW_COLOR_CONF_T conf;
        VW_COLOR_SHAPE_T shape;
        gint select, ret_val;
          	
        conf.ncolor = 6;        
        conf.select = 0;       
    	conf.color_idx[0] = COLOR_PRG_IDX(UX_COLOR_FFFFFF);
    	conf.color_idx[1] = COLOR_PRG_IDX(UX_COLOR_808080);
    	conf.color_idx[2] = COLOR_PRG_IDX(UX_COLOR_FFFF00);
    	conf.color_idx[3] = COLOR_PRG_IDX(UX_COLOR_0000FF);
    	conf.color_idx[4] = COLOR_PRG_IDX(UX_COLOR_00FF00);
    	conf.color_idx[5] = COLOR_PRG_IDX(UX_COLOR_FF0000);

        shape.x = 200;
        shape.y = 200;
        shape.cnt_col = 6;

        ret_val = vw_change_color_popup_open(g_curwnd, &conf, &shape);
        if (!ret_val) return FALSE;

        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, conf.color_idx[conf.select]);
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_exclude_onoff_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        gint index, i;

        index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        for (i = 0; i < 4; i++)
        {
            if (index == 0)
            {
                nfui_nfobject_disable(g_exclude_text_obj[i]);
            }
            else
            {
                nfui_nfobject_enable(g_exclude_text_obj[i]);
            }

            nfui_signal_emit(g_exclude_text_obj[i], GDK_EXPOSE, TRUE);
        }
    }

    return FALSE;
}

static gboolean post_exclude_text_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;
    gchar *title;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER )
	{
		NFOBJECT *top;
		gchar *strTemp;
		guint x, y;
        mb_type ret = NFTOOL_MB_OK;

		if(kpid == KEYPAD_ENTER)
		{
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);

			x += (obj->width)/2 + top->x;
			y += obj->height + top->y;
		}
		else
		{
	  	   	if(evt->button.button == MOUSE_RIGTH_BUTTON)
			{					
				return FALSE;
	  	   	}
		
			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += (guint)((GdkEventButton*)evt)->x;
			y += (guint)((GdkEventButton*)evt)->y;
		}

        title = nfui_nflabel_get_text((NFLABEL*)obj);
       
   		strTemp = VirtualKey_Open(g_curwnd, title, x, y, 16, VKEY_NORMAL);        

		if (strTemp) 
		{
			if (ret == NFTOOL_MB_OK)
			{
				nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
				nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
			}
		
			ifree(strTemp);
			strTemp = NULL;
		}
	}
	return FALSE;
}

static gboolean post_exclude_color_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{

    return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        g_memmove(&posdata, &org_posdata, sizeof(PosOsdData));
        _prvSetDataToObjects(1);
    }
    
    return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        _prvLoadDataFromObjects();
        
        if (memcmp(&org_posdata, &posdata, sizeof(PosOsdData)))
        {
            g_memmove(&org_posdata, &posdata, sizeof(PosOsdData));
            DAL_set_pososd_data(&posdata);

//          scm_put_log(CHANGE_DISP_OSD, 0, 0);
            nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
            sysdisp_set_changeflag(1);
        }   
    }
    
    return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
    
        DispPOSATM_tab_out_handler();
        SystemSetupDisp_Destroy(obj);
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


void init_DispPOSATM_page(NFOBJECT *parent)
{
    NFOBJECT *content_fixed;
    NFOBJECT *fixed;    
    NFOBJECT *ntb;
    NFOBJECT *obj;

    NFOBJECT *onoff_obj;

    const gchar *strTitle1[] = {
                    "DISPLAY MODE",
                    "POSITION",
                    "FONT SIZE",
                    "FONT COLOR",
                    "DWELL TIME",
//                    "NUMBER FORMAT",
                    "SCROLL TYPE",
    };

    const gchar *strTitle2[] = {
                    "ACTIVATION",
                    "TEXT %d",
                    "TEXT %d",
                    "TEXT %d",
                    "TEXT %d",
    };

    const gchar *strTitle3[] = {
                    "ACTIVATION",
                    "TEXT %d",
                    "TEXT %d",
                    "TEXT %d",
                    "TEXT %d",
    };

    gchar strBuf[64];
    guint width[2];
    guint btn_x, btn_y, btn_space;    
    gint pos_x, pos_y;
    guint i;
    
    gint index;


    g_curwnd = nfui_nfobject_get_top(parent);

    memset(&posdata, 0x00, sizeof(PosOsdData));
    memset(&org_posdata, 0x00, sizeof(PosOsdData));

    DAL_get_pososd_data(&posdata);
    g_memmove(&org_posdata, &posdata, sizeof(PosOsdData));

    content_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(content_fixed, MENU_V_INNER_W, MENU_V_INNER_H);
    nfui_nfobject_show(content_fixed);
    nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);

    pos_x = TABLE_LEFT;
    pos_y = TABLE_TOP;

    width[0] = 380;
    width[1] = 260;

    obj = nfui_nfimage_new(IMG_TITLE_BG);
    nfui_nfimage_set_text((NFIMAGE*)obj, "DISPLAY");
    nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
    nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

    pos_y += 60;

    ntb = nfui_nftable_new(2, 6, 2, 1, width, LABEL_HEIGHT);    
    nfui_nfobject_show(ntb);
    nfui_nffixed_put(content_fixed, ntb, pos_x+20, pos_y);

    for (i = 0; i < 6; i++)
    {
        obj = nfui_nflabel_new_with_pango_font(strTitle1[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);        
        nfui_nftable_attach((NFTABLE*)ntb, obj, 0, i);

        obj = nfui_combobox_new(0, 0, 0);
        nfui_combobox_set_skin_type((NFCOMBOBOX*)obj, NFCOMBOBOX_TYPE_1);
        nfui_combobox_set_align((NFCOMBOBOX*)obj, NFALIGN_CENTER, 0);
        nfui_nfobject_show(obj);        
        nfui_nftable_attach((NFTABLE*)ntb, obj, 1, i);

        if (i == 0)
        {
            nfui_combobox_append_data((NFCOMBOBOX*)obj, "OFF");
            nfui_combobox_append_data((NFCOMBOBOX*)obj, "LIVE");
            nfui_combobox_append_data((NFCOMBOBOX*)obj, "PLAYBACK");
            nfui_combobox_append_data((NFCOMBOBOX*)obj, "BOTH");

            nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, posdata.disp_mode);
            g_mode_obj = obj;
        }
        else if (i == 1)
        {
            nfui_combobox_append_data((NFCOMBOBOX*)obj, "LEFT1");
//            nfui_combobox_append_data((NFCOMBOBOX*)obj, "CENTER");
            nfui_combobox_append_data((NFCOMBOBOX*)obj, "RIGHT1");

            nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, posdata.position);
            g_position_obj = obj;
        }
        else if (i == 2)
        {
            nfui_combobox_append_data((NFCOMBOBOX*)obj, "SMALL");
            nfui_combobox_append_data((NFCOMBOBOX*)obj, "MEDIUM");
            nfui_combobox_append_data((NFCOMBOBOX*)obj, "LARGE");

            nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, posdata.font);
            g_font_obj = obj;
        }
        else if (i == 3)
        {       
            nfui_combobox_append_data((NFCOMBOBOX*)obj, "WHITE");
            nfui_combobox_append_data((NFCOMBOBOX*)obj, "GRAY");
            nfui_combobox_append_data((NFCOMBOBOX*)obj, "YELLOW");
            nfui_combobox_append_data((NFCOMBOBOX*)obj, "BLUE");
            nfui_combobox_append_data((NFCOMBOBOX*)obj, "GREEN");
            nfui_combobox_append_data((NFCOMBOBOX*)obj, "RED");

            index = _prvRgbToComboIdx(posdata.normal_color);
            nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, index);            
            g_ncolor_obj = obj;
        }
        else if (i == 4)
        {
            nfui_combobox_append_data((NFCOMBOBOX*)obj, "UNTIL NEXT");
            nfui_combobox_append_data((NFCOMBOBOX*)obj, "1 SEC");
            nfui_combobox_append_data((NFCOMBOBOX*)obj, "3 SEC");
            nfui_combobox_append_data((NFCOMBOBOX*)obj, "5 SEC");            
            nfui_combobox_append_data((NFCOMBOBOX*)obj, "10 SEC");
            nfui_combobox_append_data((NFCOMBOBOX*)obj, "15 SEC");
            nfui_combobox_append_data((NFCOMBOBOX*)obj, "30 SEC");            
            nfui_combobox_append_data((NFCOMBOBOX*)obj, "60 SEC");                        

            index = _prvDurationToIndex(posdata.duration);
            nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, index);
            g_duration_obj = obj;
        }
/*        
        else if (i == 5)
        {
            nfui_combobox_append_data((NFCOMBOBOX*)obj, "DECIMAL");
            nfui_combobox_append_data((NFCOMBOBOX*)obj, "DIGIT");

            nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, posdata.format);
            g_format_obj = obj;
        }
*/        
        else if (i == 5)
        {
            nfui_combobox_append_data((NFCOMBOBOX*)obj, "CLEAR");
            nfui_combobox_append_data((NFCOMBOBOX*)obj, "ROLL UP");

            nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, posdata.scroll);            
            g_scroll_obj = obj;
        }        
    }


    width[0] = 180;
    width[1] = 261;

    pos_y += (LABEL_HEIGHT*7 + 60);

    obj = nfui_nfimage_new(IMG_TITLE_BG);
    nfui_nfimage_set_text((NFIMAGE*)obj, "HIGHLIGHT TEXT");
    nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
    nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

    ntb = nfui_nftable_new(2, 5, 2, 1, width, LABEL_HEIGHT);    
    nfui_nfobject_show(ntb);
    nfui_nffixed_put(content_fixed, ntb, pos_x+20, pos_y+60);
    
    for (i = 0; i < 5; i++)
    {
        memset(strBuf, 0x00, sizeof(strBuf));
        if (i == 0) strcpy(strBuf, strTitle2[i]);
        else g_sprintf(strBuf, lookup_string(strTitle2[i]), i);
    
        obj = nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);        
        nfui_nftable_attach((NFTABLE*)ntb, obj, 0, i);

        fixed = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_show(fixed);
        nfui_nftable_attach((NFTABLE*)ntb, fixed, 1, i);

        if (i == 0)
        {
            obj = nfui_combobox_new(0, 0, 0);
            nfui_combobox_set_skin_type((NFCOMBOBOX*)obj, NFCOMBOBOX_TYPE_1);
            nfui_combobox_set_align((NFCOMBOBOX*)obj, NFALIGN_CENTER, 0);
            nfui_nfobject_set_size(obj, width[1], LABEL_HEIGHT);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED*)fixed, obj, 0, 0);
            nfui_regi_post_event_callback(obj, post_highlight_onoff_event_handler);
            onoff_obj = obj;            

            nfui_combobox_append_data((NFCOMBOBOX*)obj, "OFF");
            nfui_combobox_append_data((NFCOMBOBOX*)obj, "ON");

            nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, posdata.highlight);
            g_highlight_obj = obj;
        }
        else if ((i == 1) || (i == 2) || (i == 3) || (i == 4))
        {
        
            obj = (NFOBJECT*)nfui_nflabel_new_text_box(posdata.highlight_text[i-1], nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
            nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
            nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
       		nfui_nfobject_support_multi_lang((NFOBJECT*)obj, FALSE);		
            nfui_nfobject_set_size(obj, width[1]-41, LABEL_HEIGHT);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED*)fixed, obj, 0, 0);
            nfui_regi_post_event_callback(obj, post_highlight_text_event_handler);
            g_highlight_text_obj[i-1] = obj;

            index = _prvRgbToColorIdx(posdata.highlight_color[i-1]);

            obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_SMALL_SEMI));
            nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
            nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
            nfui_nflabel_set_drawing_outline((NFLABEL*)obj, TRUE);
            nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, index);
            nfui_nfobject_set_size(obj, 40, 40);    
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED*)fixed, obj, width[1]-40, 0);
            nfui_regi_post_event_callback(obj, post_highlight_color_event_handler);
            g_highlight_color_obj[i-1] = obj;

            if (nfui_combobox_get_cur_index((NFCOMBOBOX*)onoff_obj) == 0)
            {
                nfui_nfobject_disable(g_highlight_text_obj[i-1]);
                nfui_nfobject_disable(g_highlight_color_obj[i-1]);
            }
        }
    }


    width[0] = 180;
    width[1] = 261;

    pos_x += 640;

    obj = nfui_nfimage_new(IMG_TITLE_BG);
    nfui_nfimage_set_text((NFIMAGE*)obj, "EXCLUDE TEXT");
    nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
    nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

    ntb = nfui_nftable_new(2, 5, 2, 1, width, LABEL_HEIGHT);    
    nfui_nfobject_show(ntb);
    nfui_nffixed_put(content_fixed, ntb, pos_x+20, pos_y+60);
    
    for (i = 0; i < 5; i++)
    {
        memset(strBuf, 0x00, sizeof(strBuf));
        if (i == 0) strcpy(strBuf, strTitle3[i]);
        else g_sprintf(strBuf, lookup_string(strTitle3[i]), i);
    
        obj = nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);        
        nfui_nftable_attach((NFTABLE*)ntb, obj, 0, i);

        fixed = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_show(fixed);
        nfui_nftable_attach((NFTABLE*)ntb, fixed, 1, i);

        if (i == 0)
        {
            obj = nfui_combobox_new(0, 0, 0);
            nfui_combobox_set_skin_type((NFCOMBOBOX*)obj, NFCOMBOBOX_TYPE_1);
            nfui_combobox_set_align((NFCOMBOBOX*)obj, NFALIGN_CENTER, 0);            
            nfui_nfobject_set_size(obj, width[1], LABEL_HEIGHT);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED*)fixed, obj, 0, 0);
            nfui_regi_post_event_callback(obj, post_exclude_onoff_event_handler);
            onoff_obj = obj;            
            
            nfui_combobox_append_data((NFCOMBOBOX*)obj, "OFF");
            nfui_combobox_append_data((NFCOMBOBOX*)obj, "ON");

            nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, posdata.exclude);            
            g_exclude_obj = obj;
        }
        else if ((i == 1) || (i == 2) || (i == 3) || (i == 4))
        {
            obj = (NFOBJECT*)nfui_nflabel_new_text_box(posdata.exclude_text[i-1], nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
            nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
            nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
       		nfui_nfobject_support_multi_lang((NFOBJECT*)obj, FALSE);            
            nfui_nfobject_set_size(obj, width[1], LABEL_HEIGHT);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED*)fixed, obj, 0, 0);
            nfui_regi_post_event_callback(obj, post_exclude_text_event_handler);            
            g_exclude_text_obj[i-1] = obj;

            if (nfui_combobox_get_cur_index((NFCOMBOBOX*)onoff_obj) == 0)
            {
                nfui_nfobject_disable(g_exclude_text_obj[i-1]);
            }            
        }
    }
    
    
    obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R3_X, MENU_V_BTN_Y);
    nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);

    obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R2_X, MENU_V_BTN_Y);
    nfui_regi_post_event_callback(obj, post_applybutton_event_handler);

    obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R1_X, MENU_V_BTN_Y);
    nfui_regi_post_event_callback(obj, post_closebutton_event_handler);
    
    nfui_regi_post_event_callback(parent, post_page_event_handler);
}

gboolean DispPOSATM_tab_out_handler()
{
    mb_type ret;

    _prvLoadDataFromObjects();

    if (!memcmp(&org_posdata, &posdata, sizeof(PosOsdData))) return FALSE;

    ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

    if (ret == NFTOOL_MB_OK)
    {
        g_memmove(&org_posdata, &posdata, sizeof(PosOsdData));
        DAL_set_pososd_data(&posdata);

//          scm_put_log(CHANGE_DISP_OSD, 0, 0);
        sysdisp_set_changeflag(1);
    }
    else if(ret == NFTOOL_MB_CANCEL)
    {
        g_memmove(&posdata, &org_posdata, sizeof(PosOsdData));
        _prvSetDataToObjects(0);
    }
    
    return FALSE;
}




