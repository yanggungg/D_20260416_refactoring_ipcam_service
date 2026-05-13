#include "nf_afx.h"

#include "tools/nf_ui_tool.h"
#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/util.h"
#include "support/color.h"

#include "objects/nftab.h"
#include "objects/nfobject.h"
#include "objects/nfbutton.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfimage.h"
#include "objects/nfimglabel.h"
#include "objects/nfcombobox.h"
#include "objects/nfspinbutton.h"
#include "objects/cw_slider.h"

#include "scm.h"
#include "vsm.h"
#include "ix_mem.h"

#include "vw.h"
#include "vw_sys_camera_ipcam_internal.h"


#define DBG_LEVEL		1
#define DBG_MODULE		"IPCAM_OBJ"


#define ROW_H               (40)
#define ROW_GAP             (2)


static NFOBJECT* _make_obj_label(OBJECT_INFO_T *obj_info)
{
    NFOBJECT *obj;
	gint color[NFOBJECT_STATE_COUNT];	    
    gint label_type = obj_info->label.data.type;
    gint width = obj_info->width;

    switch (label_type)
    {
		case LABEL_CATEGORY :	
		{
        	DMSG(9, "LABEL_CATEGORY text:%s", obj_info->label.data.category.text);

        	obj = nfui_nfimage_new(obj_info->label.data.category.img);
            nfui_nfimage_set_text((NFIMAGE*)obj, obj_info->label.data.category.text);        	
        	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
        	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 17);
        	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        	nfui_nfobject_show(obj);
		}
		break;

		case LABEL_TITLE :	
		{
        	DMSG(9, "LABEL_TITLE text:%s", obj_info->label.data.title.text);
		
        	color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(968);
        	color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(968);
        	color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(968);
        	color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(972);
		
        	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
        	nfui_nflabel_set_fg_color((NFLABEL*)obj, color);        	
            nfui_nflabel_set_text((NFLABEL*)obj, obj_info->label.data.title.text);        	
        	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
        	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
        	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_DISABLE, COLOR_IDX(186));        	
        	nfui_nfobject_set_size(obj, width, ROW_H);
        	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        	nfui_nfobject_show(obj);		
		}
		break;

		case LABEL_NUMBER :	
		{


		}
		break;

		case LABEL_LETTER :	
		{
            DMSG(9, "LABEL_LETTER text: %s ", obj_info->label.data.letter.text);

            obj = nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
            nfui_nflabel_set_fg_color((NFLABEL*)obj, color);
        	nfui_nflabel_set_skin_type((NFLABEL *) obj, NFTEXTBOX_TYPE_INPUT);
        	nfui_nflabel_set_text((NFLABEL*) obj, obj_info->label.data.letter.text);
        	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
        	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_DISABLE, COLOR_IDX(938));
        	nfui_nfobject_set_size(obj, width ,ROW_H);
        	nfui_nfobject_show(obj);
		}
		break;

		case LABEL_LEGEND :	
		{	
        	DMSG(9, "LABEL_LEGEND text:%s", obj_info->label.data.legend.text);
		
        	color[NFOBJECT_STATE_NORMAL] = COLOR_IDX(968);
        	color[NFOBJECT_STATE_PRELIGHT] = COLOR_IDX(968);
        	color[NFOBJECT_STATE_ACTIVE] = COLOR_IDX(968);
        	color[NFOBJECT_STATE_DISABLE] = COLOR_IDX(972);
		
        	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_SMALL_SEMI_1), COLOR_IDX(968));
        	nfui_nflabel_set_fg_color((NFLABEL*)obj, color);
            nfui_nflabel_set_text((NFLABEL*)obj, obj_info->label.data.legend.text);        	
        	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
        	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_DISABLE, COLOR_IDX(186));        	
        	nfui_nfobject_set_size(obj, width, ROW_H);
        	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        	nfui_nfobject_show(obj);
		}
		break;		

		default :
		    DMSG(1, "type : %d, undefined label type", label_type);
		    g_assert(0);
		break;
    }

    return obj;
}

static NFOBJECT* _make_obj_spin(OBJECT_INFO_T *obj_info )
{
    NFOBJECT *obj;
    gint spin_type = obj_info->spin.data.type;
    gint width = obj_info->width;
    
    switch (spin_type)
    {
		case SPIN_NUMBER :	
		{		
			DMSG(9, "SPIN_NUMBER init:%d", obj_info->spin.data.number.init-obj_info->spin.data.number.min); 
			DMSG(9, "SPIN_NUMBER range:%d ~ %d", obj_info->spin.data.number.min, obj_info->spin.data.number.max); 
		
        	obj = nfui_spinbutton_new_value_with_range(obj_info->spin.data.number.init, 
        	                                    obj_info->spin.data.number.min, 
        	                                    obj_info->spin.data.number.max, 
        	                                    obj_info->spin.data.number.step);
            nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_SUBTAB_1);
        	nfui_nfobject_set_size(obj, width, ROW_H);
        	nfui_nfobject_show(obj);	
		}
		break;

		case SPIN_LETTER :	
		{
            gint i;
		
        	obj = nfui_spinbutton_new(0, 0, 0);
            nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_SUBTAB_1);
        	nfui_nfobject_set_size(obj, width, ROW_H);
        	nfui_nfobject_show(obj);	
        	
           	DMSG(9, "SPIN_LETTER cnt:%d", obj_info->spin.data.letter.cnt);
           	DMSG(9, "SPIN_LETTER init:%d", obj_info->spin.data.letter.init);
           	
            for (i = 0; i < obj_info->spin.data.letter.cnt; i++)
            {
            	DMSG(9, "SPIN_LETTER data i:%d, text:%s", i, obj_info->spin.data.letter.text[i]);
                nfui_spin_button_append_data((NFSPINBUTTON*)obj, obj_info->spin.data.letter.text[i]);
			}

			if (obj_info->spin.data.letter.cnt)                
	            nfui_spin_button_set_index((NFSPINBUTTON*)obj, obj_info->spin.data.letter.init);
		}
		break;

		default :
		    DMSG(1, "type : %d, undefined spin type", spin_type);
		    g_assert(0);
		break;
    }
   
    return obj;
}



static NFOBJECT* _make_obj_combo(OBJECT_INFO_T *obj_info )
{
    NFOBJECT *obj;
    gint combo_type = obj_info->combo.data.type;
    gint width = obj_info->width;

    switch (combo_type)
    {
		case COMBO_NUMBER :	
		{	
        	obj = nfui_combobox_new("", 0, 0);
        	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_SUBTAB_1);
        	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
        	nfui_nfobject_set_size(obj, width, ROW_H);
        	nfui_nfobject_show(obj);	
		}
		break;

		case COMBO_LETTER :	
		{	
            gint i;
		
        	obj = nfui_combobox_new("", 0, 0);
        	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_SUBTAB_1);
        	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
        	nfui_nfobject_set_size(obj, width, ROW_H);
        	nfui_nfobject_show(obj);	

           //	DMSG(9, "COMBO_LETTER cnt:%d", obj_info->combo.data.letter.cnt);
           //	DMSG(9, "COMBO_LETTER init:%d", obj_info->combo.data.letter.init);

            for (i = 0; i < obj_info->combo.data.letter.cnt; i++)
            {
           //     if (strcmp(obj_info->combo.data.letter.text[i], "PARANGI") == 0)
            //    {
             //       g_message(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> %s, %d", __FUNCTION__, __LINE__);
             //   }
            
            //	DMSG(9, "COMBO_LETTER data i:%d, text:%s", i, obj_info->combo.data.letter.text[i]);
                nfui_combobox_append_data(NF_COMBOBOX(obj), obj_info->combo.data.letter.text[i]);
			}

			if (obj_info->combo.data.letter.cnt)
	            nfui_combobox_set_index(NF_COMBOBOX(obj), obj_info->combo.data.letter.init);
		}
		break;

		default :
		    DMSG(1, "type : %d, undefined combo type", combo_type);
		    g_assert(0);
		break;
    }
    
    return obj;
}

static NFOBJECT* _make_obj_slider(OBJECT_INFO_T *obj_info )
{
    NFOBJECT *obj;
    gint slider_type = obj_info->slider.data.type;
    gint width = obj_info->width;

    switch (slider_type)
    {
		case SLIDER_NORMAL :	
		{
			DMSG(9, "SLIDER_NORMAL init:%d", obj_info->slider.data.normal.init); 
			DMSG(9, "SLIDER_NORMAL cnt:%d, range:%d ~ %d", obj_info->slider.data.normal.cnt, obj_info->slider.data.normal.min, obj_info->slider.data.normal.max); 

        	obj = cw_slider_new(obj_info->slider.data.normal.init, width, ROW_H);	
        	cw_slider_set_range(obj, obj_info->slider.data.normal.min, obj_info->slider.data.normal.max, obj_info->slider.data.normal.cnt);
        	nfui_nfobject_set_size(obj, width, ROW_H);
        	nfui_nfobject_show(obj);
		}
		break;

		default :
		    DMSG(1, "type : %d, undefined slider type", slider_type);
		    g_assert(0);
		break;
    }
    
    return obj;
}

static NFOBJECT* _make_obj_button(OBJECT_INFO_T *obj_info )
{
    NFOBJECT *obj;
    gint button_type = obj_info->button.data.type;
    gint width = obj_info->width;

    switch (button_type)
    {
		case BUTTON_LETTER :	
		{
			DMSG(9, "BUTTON_LETTER:%s", obj_info->button.data.letter.text); 
		
        	obj = nftool_normal_button_create_type3(obj_info->button.data.letter.text, width);
        	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
        	nfui_nfobject_show(obj);
		}
		break;

		case BUTTON_IMAGE :	
		{	
        	obj = (NFOBJECT*)nfui_nfbutton_new();
        	nfui_nfbutton_set_image(NF_BUTTON(obj), obj_info->button.data.image.name);
        	nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
        	nfui_nfobject_set_size(obj, width, ROW_H);
        	nfui_nfobject_show(obj);
		}
		break;

		default :
		    DMSG(1, "type : %d, undefined button type", button_type);
		    g_assert(0);
		break;
    }
    
    return obj;
}

static NFOBJECT *_make_col(NFOBJECT *fixed, COL_INFO_T *col_info)
{
    NFOBJECT *obj;    
    gint obj_type = col_info->obj_info->obj_type;
 
    switch (obj_type)
    {
		case OBJ_LABEL :	
            obj = _make_obj_label(col_info->obj_info);
		break;

		case OBJ_SPIN :	
            obj = _make_obj_spin(col_info->obj_info);
		break;
 
		case OBJ_COMBO :	
            obj = _make_obj_combo(col_info->obj_info);
		break;

		case OBJ_SLIDER :	
            obj = _make_obj_slider(col_info->obj_info);
		break;

		case OBJ_BUTTON :	
            obj = _make_obj_button(col_info->obj_info);
		break;		
	
		default :
		    DMSG(1, "type : %d, undefined object type", obj_type);
		    g_assert(0);
		break;
    }

	if (!col_info->obj_info->enable)
        nfui_nfobject_disable(obj);   

	if (col_info->obj_info->handler) 
	    nfui_regi_post_event_callback(obj, col_info->obj_info->handler);

    return obj;
}

static gint _make_row(gint ch, NFOBJECT *fixed, ROW_INFO_T *row_info)
{
    NFOBJECT *obj;
    gint i, pos_x = 0;
    gint col_cnt = row_info->col_cnt;

    for (i = 0; i < col_cnt; i++)
    {
        obj = _make_col(fixed, row_info->col_info[i]);
    	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, 0);

        row_info->col_info[i]->obj = obj;
        pos_x += (row_info->col_info[i]->obj_info->width + 6);
    }

    return 0;
}

gint _make_objs_of_subFixed(FIXED_INFO_T *fixed_info, gint width)
{
    gint i, ch = 0;
    gint pos_y = 0, fixed_h = 0;
    gint row_cnt = fixed_info->row_cnt;   
    NFOBJECT *fixed;



    for (i = 0; i < row_cnt; i++)
    {
    	fixed = (NFOBJECT*)nfui_nffixed_new();
    	nfui_nfobject_set_size(fixed, width, ROW_H);
        nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));			    	
    	nfui_nfobject_hide(fixed);
    	nfui_nffixed_put((NFFIXED*)fixed_info->fixed, fixed, 0, 0);

        _make_row(ch, fixed, fixed_info->row_info[i]);
        fixed_info->row_info[i]->fixed = fixed;
    }
        
    return 0;
}


gint _set_visible_subFixed(FIXED_INFO_T *fixed_info)
{
    gint i, j;
    gint pos_y = 0;
    guint64 visible = fixed_info->visible;

    for (i = 0; i < fixed_info->row_cnt; i++)
    {
        if (fixed_info->row_info[i]->key)
        {
            if (visible & fixed_info->row_info[i]->key)
            {
                nfui_nfobject_move(fixed_info->row_info[i]->fixed, 0, pos_y);
                nfui_nfobject_show(fixed_info->row_info[i]->fixed);

                pos_y += (ROW_H + ROW_GAP);
            }
            else
            {
                nfui_nfobject_move(fixed_info->row_info[i]->fixed, 0, 0);
                nfui_nfobject_hide(fixed_info->row_info[i]->fixed);
            }
        }
        else
        {
            nfui_nfobject_move(fixed_info->row_info[i]->fixed, 0, pos_y);
            nfui_nfobject_show(fixed_info->row_info[i]->fixed);

            pos_y += (ROW_H + ROW_GAP);
        }

        for (j = 0; j < fixed_info->row_info[i]->col_cnt; j++)
        {
            if (fixed_info->row_info[i]->col_info[j]->key)
            {
                if (visible & fixed_info->row_info[i]->col_info[j]->key)
                    nfui_nfobject_show(fixed_info->row_info[i]->col_info[j]->obj);
                else
                    nfui_nfobject_hide(fixed_info->row_info[i]->col_info[j]->obj);
            }
        }        
    }
        
    return 0;
}


gint _set_enable_subFixed(FIXED_INFO_T *fixed_info)
{
    gint i, j;
    guint64 enable = fixed_info->enable;

    for (i = 0; i < fixed_info->row_cnt; i++)
    {
        for (j = 0; j < fixed_info->row_info[i]->col_cnt; j++)
        {
            if (fixed_info->row_info[i]->col_info[j]->key)
            {
                if (enable & fixed_info->row_info[i]->col_info[j]->key)
                    nfui_nfobject_enable(fixed_info->row_info[i]->col_info[j]->obj);
                else
                    nfui_nfobject_disable(fixed_info->row_info[i]->col_info[j]->obj);
            }
        
            if (!fixed_info->row_info[i]->col_info[j]->obj_info->enable)
                nfui_nfobject_disable(fixed_info->row_info[i]->col_info[j]->obj);        
        }
    }
        
    return 0;
}


