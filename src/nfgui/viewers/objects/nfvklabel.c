

/*****************************************************************************************
 *
 *
 *  NFVKLABEL Related...
 *
 *
 * **************************************************************************************/

#include "../../support/util.h"
#include "../../support/color.h"
#include "../../support/event_loop.h"
#include "nfvklabel.h"
#include "ix_mem.h"
#include "evt.h"

#define _strlen(n)	g_utf8_strlen(n, -1)
#define _strncpy	g_utf8_strncpy


#define VKLABEL_MARGIN                  (4)
#define VKLABEL_LINE_BORDER             (2)

static void nfvklabel_draw_outlines(NFOBJECT *obj, gint color_idx)
{
    NFVKLABEL* nfvklabel;
    GdkDrawable *drawable = NULL;
    GdkGC *line_gc;
    guint pos_x, pos_y;
    gint line_gap;

    drawable = nfui_nfobject_get_window(obj);

    /* outline */
    line_gc = nfui_nfobject_get_gc(obj);
    gdk_gc_set_rgb_fg_color(line_gc, &UX_COLOR(color_idx));

    gdk_gc_set_line_attributes(line_gc,
            VKLABEL_LINE_BORDER,
            GDK_LINE_SOLID,
            GDK_CAP_NOT_LAST,
            GDK_JOIN_MITER);

    nfui_nfobject_get_offset(obj, &pos_x, &pos_y);
    line_gap = VKLABEL_LINE_BORDER - 1;

    gdk_draw_rectangle(drawable,
                    line_gc,
                    FALSE,
                    pos_x + line_gap,
                    pos_y + line_gap,
                    obj->width - (line_gap * 2),
                    obj->height - (line_gap * 2));
                    
    nfui_nfobject_gc_unref(line_gc);

}

static gint _check_drag_section(NFVKLABEL *nfvklabel, gint row, gint *cursor_dsc, gint *cursor_dec)
{
    if (row == 0)
    {
        if (*cursor_dsc >= nfvklabel->row1_cc)  return 0;
        
        if (*cursor_dec >= nfvklabel->row1_cc)  *cursor_dec = nfvklabel->row1_cc-1;
    }
    else if (row == 1)
    {
        if ((*cursor_dsc < nfvklabel->row1_cc) && (*cursor_dec < nfvklabel->row1_cc))       return 0;
        if ((*cursor_dsc >= nfvklabel->row2_cc) && (*cursor_dec >= nfvklabel->row2_cc))     return 0;

        if (*cursor_dsc < nfvklabel->row1_cc)   *cursor_dsc = nfvklabel->row1_cc;
        if (*cursor_dec >= nfvklabel->row2_cc)  *cursor_dec = nfvklabel->row2_cc-1;     
    }
    else if (row == 2)
    {
        if ((*cursor_dsc < nfvklabel->row2_cc) && (*cursor_dec < nfvklabel->row2_cc))       return 0;

        if (*cursor_dsc < nfvklabel->row2_cc)   *cursor_dsc = nfvklabel->row2_cc;               
    }

    return 1;
}

static gint _get_drag_section_param(NFVKLABEL *nfvklabel, gint row, gint *section_pos_x, gint *section_pos_w)
{
    GdkDrawable *drawable = NULL;
    gint str_len, str_width = 0;
    gint tmp_str_width = 0;
    gint off_x, off_y;
    gint cursor_dsc, cursor_dec;
    gint row1_cc, row2_cc, row3_cc;
    gint i;
    
    gchar strTemp[NFVKLABEL_MAX_STRING_SIZE];

    drawable = nfui_nfobject_get_window((NFOBJECT*)nfvklabel);
    nfui_nfobject_get_offset((NFOBJECT*)nfvklabel, &off_x, &off_y);

    if (nfvklabel->cursor_dsc > nfvklabel->cursor_dec)
    {
        cursor_dsc = nfvklabel->cursor_dec;
        cursor_dec = nfvklabel->cursor_dsc;
    }
    else
    {
        cursor_dsc = nfvklabel->cursor_dsc;
        cursor_dec = nfvklabel->cursor_dec;
    }   

    if (!_check_drag_section(nfvklabel, row, &cursor_dsc, &cursor_dec)) return 0;   

    memset(strTemp, 0x00, sizeof(strTemp));

    row1_cc = nfvklabel->row1_cc;
    row2_cc = nfvklabel->row2_cc;
    row3_cc = nfvklabel->row3_cc;

    if (nfvklabel->row_count == 1)
    {
        if (nfvklabel->invisible)
        {
            strTemp[0] = '*';
            
        	str_len = _strlen(nfvklabel->row1_label);
            str_width = str_len * nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
            
            tmp_str_width = cursor_dsc * nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
        }
        else
        {
            str_width = nfutil_string_width(0, drawable, nfvklabel->pango_font, nfvklabel->row1_label, nfvklabel->spacing_type);

            if (cursor_dsc > 0)
            {
			    _strncpy(strTemp, nfvklabel->row1_label, cursor_dsc);
                tmp_str_width = nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);  
            }       
        }
    }
    else if (nfvklabel->row_count == 2)
    {
        if (cursor_dsc < row1_cc)
        {
            if (nfvklabel->invisible)
            {
                strTemp[0] = '*';
                
            	str_len = _strlen(nfvklabel->row1_label);
                str_width = str_len * nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
                
                tmp_str_width = cursor_dsc * nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
            }
            else
            {
                str_width = nfutil_string_width(0, drawable, nfvklabel->pango_font, nfvklabel->row1_label, nfvklabel->spacing_type);

                if (cursor_dsc > 0)
                {
    			    _strncpy(strTemp, nfvklabel->row1_label, cursor_dsc);
                    tmp_str_width = nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);  
                }       
            }
        }
        else
        {
            if (nfvklabel->invisible)
            {
                strTemp[0] = '*';
                
            	str_len = _strlen(nfvklabel->row2_label);
                str_width = str_len * nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
                
                tmp_str_width = (cursor_dsc-row1_cc) * nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
            }
            else
            {
                str_width = nfutil_string_width(0, drawable, nfvklabel->pango_font, nfvklabel->row2_label, nfvklabel->spacing_type);

                if (cursor_dsc > row1_cc)
                {
    			    _strncpy(strTemp, nfvklabel->row2_label, cursor_dsc-row1_cc);
                    tmp_str_width = nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);  
                }       
            }
        }
    }
    else if (nfvklabel->row_count == 3)
    {
        if (cursor_dsc < row1_cc)
        {
            if (nfvklabel->invisible)
            {
                strTemp[0] = '*';
                
            	str_len = _strlen(nfvklabel->row1_label);
                str_width = str_len * nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
                
                tmp_str_width = cursor_dsc * nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
            }
            else
            {
                str_width = nfutil_string_width(0, drawable, nfvklabel->pango_font, nfvklabel->row1_label, nfvklabel->spacing_type);

                if (cursor_dsc > 0)
                {
    			    _strncpy(strTemp, nfvklabel->row1_label, cursor_dsc);
                    tmp_str_width = nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);  
                }       
            }
        }
        else if (cursor_dsc < row2_cc)
        {
            if (nfvklabel->invisible)
            {
                strTemp[0] = '*';
                
            	str_len = _strlen(nfvklabel->row2_label);
                str_width = str_len * nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
                
                tmp_str_width = (cursor_dsc-row1_cc) * nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
            }
            else
            {
                str_width = nfutil_string_width(0, drawable, nfvklabel->pango_font, nfvklabel->row2_label, nfvklabel->spacing_type);

                if (cursor_dsc > row1_cc)
                {
    			    _strncpy(strTemp, nfvklabel->row2_label, cursor_dsc-row1_cc);
                    tmp_str_width = nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);  
                }       
            }
        }
        else
        {
            if (nfvklabel->invisible)
            {
                strTemp[0] = '*';
                
            	str_len = _strlen(nfvklabel->row2_label);
                str_width = str_len * nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
                
                tmp_str_width =  (cursor_dsc-row2_cc) * nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
            }
            else
            {
                str_width = nfutil_string_width(0, drawable, nfvklabel->pango_font, nfvklabel->row2_label, nfvklabel->spacing_type);

                if (cursor_dsc > row2_cc)
                {
    			    _strncpy(strTemp, nfvklabel->row2_label, cursor_dsc-row2_cc);
                    tmp_str_width = nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);  
                }       
            }
        }
    }

    if (nfvklabel->align == NFALIGN_LEFT)
    {
        *section_pos_x = off_x + nfvklabel->margin + tmp_str_width;
    }
    else if (nfvklabel->align == NFALIGN_RIGHT)
    {
        *section_pos_x = off_x + nfvklabel->object.width;
        *section_pos_x -= (nfvklabel->margin + str_width);
        *section_pos_x += tmp_str_width;        
    }
    else    //(nfvklabel->align == NFALIGN_CENTER)
    {   
        *section_pos_x = off_x + (nfvklabel->object.width - str_width)/2;
        *section_pos_x += tmp_str_width;        
    }

    memset(strTemp, 0x00, sizeof(strTemp));

    if (row == 0)
    {
		str_len = _strlen(nfvklabel->strLabel);

        if (str_len)
        {
            if(nfvklabel->invisible)
            {
                for(i = 0; i < cursor_dec-cursor_dsc+1; i++)
                    strTemp[i] = '*';
            }
            else                    
    			_strncpy(strTemp, g_utf8_offset_to_pointer(nfvklabel->strLabel, cursor_dsc), cursor_dec-cursor_dsc+1);
        }
        else
            strTemp[0] = 'A';
    }
    else if (row == 1)
    {
        if(nfvklabel->invisible)
        {
            for(i = 0; i < cursor_dec-cursor_dsc+1; i++)
                strTemp[i] = '*';
        }
        else                
    		_strncpy(strTemp, g_utf8_offset_to_pointer(nfvklabel->strLabel, cursor_dsc), cursor_dec-cursor_dsc+1);
    }
    else if (row == 2)
    {
        if(nfvklabel->invisible)
        {
            for(i = 0; i < cursor_dec-cursor_dsc+1; i++)
                strTemp[i] = '*';
        }
        else                
    		_strncpy(strTemp, g_utf8_offset_to_pointer(nfvklabel->strLabel, cursor_dsc), cursor_dec-cursor_dsc+1);
    }

    *section_pos_w = nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type); 
    return 1;
}

static gint _set_drag_section(NFVKLABEL *nfvklabel, gint row, gint section_pos_x, gint section_pos_w, gint section_state)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc;

    gint bg_col_idx, fg_col_idx;
    gint off_x, off_y;
    gint str_len;
    gint cursor_dsc, cursor_dec;
    gint i;
    
    gchar strTemp[NFVKLABEL_MAX_STRING_SIZE];

    drawable = nfui_nfobject_get_window((NFOBJECT*)nfvklabel);
    gc = nfui_nfobject_get_gc((NFOBJECT*)nfvklabel);
    nfui_nfobject_get_offset((NFOBJECT*)nfvklabel, &off_x, &off_y);

    if (section_state == VK_SECTION_DRAW)
    {
        bg_col_idx = COLOR_IDX(250);
        fg_col_idx = COLOR_IDX(251);
    }
    else
    {
        bg_col_idx = nfui_nfobject_get_bg_color((NFOBJECT*)nfvklabel, NFOBJECT_STATE_NORMAL);
        fg_col_idx = nfvklabel->fg_color_idx;
    }

    if (nfvklabel->cursor_dsc > nfvklabel->cursor_dec)
    {
        cursor_dsc = nfvklabel->cursor_dec;
        cursor_dec = nfvklabel->cursor_dsc;
    }
    else
    {
        cursor_dsc = nfvklabel->cursor_dsc;
        cursor_dec = nfvklabel->cursor_dec;
    }   

    _check_drag_section(nfvklabel, row, &cursor_dsc, &cursor_dec);

    memset(strTemp, 0x00, sizeof(strTemp));
    
    if (row == 0)
    {
        if(nfvklabel->invisible)
        {
            for(i = 0; i < cursor_dec-cursor_dsc+1; i++)
                strTemp[i] = '*';
        }
        else
    		_strncpy(strTemp, g_utf8_offset_to_pointer(nfvklabel->strLabel, cursor_dsc), cursor_dec-cursor_dsc+1);
    }
    else if (row == 1)
    {
        if(nfvklabel->invisible)
        {
            for(i = 0; i < cursor_dec-cursor_dsc+1; i++)
                strTemp[i] = '*';
        }
        else
    		_strncpy(strTemp, g_utf8_offset_to_pointer(nfvklabel->strLabel, cursor_dsc), cursor_dec-cursor_dsc+1);
            
        off_y += nfvklabel->row_h;
    }
    else if (row == 2)
    {
        if(nfvklabel->invisible)
        {
            for(i = 0; i < cursor_dec-cursor_dsc+1; i++)
                strTemp[i] = '*';
        }
        else
    		_strncpy(strTemp, g_utf8_offset_to_pointer(nfvklabel->strLabel, cursor_dsc), cursor_dec-cursor_dsc+1);
            
        off_y += nfvklabel->row_h*2;
    }

    gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_col_idx));
    gdk_draw_rectangle(drawable,
                    gc,
                    TRUE,
                    section_pos_x,
                    off_y+3,
                    section_pos_w,
                    nfvklabel->row_h-6);

    nfutil_draw_short_text_eng_vk(NULL, 
                                NULL, 
                                (bg_col_idx < 0 ? NULL : (&UX_COLOR(bg_col_idx))), 
                                drawable, 
								gc,
                                strTemp, 
                                0, 
                                section_pos_x, 
                                off_y, 
                                section_pos_w, 
                                nfvklabel->row_h, 
                                nfvklabel->pango_font, 
                                &UX_COLOR(fg_col_idx), 
                                NULL, 
                                nfvklabel->align, 
                                0, 
                                0, 
                                nfvklabel->spacing_type );      

    nfui_nfobject_gc_unref(gc);     
    
    return 1;
}

static void _draw_drag_section(NFVKLABEL *nfvklabel)
{
    gint i;
    gint pos_x, pos_w;

    if (!nfvklabel->use_cursor) return;

    for (i = 0; i < nfvklabel->row_count; i++)
    {
        if (_get_drag_section_param(nfvklabel, i, &pos_x, &pos_w))
            _set_drag_section(nfvklabel, i, pos_x, pos_w, VK_SECTION_DRAW);
    }
}

static void _erase_drag_section(NFVKLABEL *nfvklabel)
{
    gint i;
    gint pos_x, pos_w;

    for (i = 0; i < nfvklabel->row_count; i++)
    {
        if (_get_drag_section_param(nfvklabel, i, &pos_x, &pos_w))
            _set_drag_section(nfvklabel, i, pos_x, pos_w, VK_SECTION_ERASE);
    }
}

static void _get_cursor_param(NFVKLABEL *nfvklabel, gint *cursor_pos_x, gint *cursor_pos_w)
{
    GdkDrawable *drawable = NULL;
    gint str_len, str_width = 0;
    gint tmp_str_width = 0; 
    gint off_x, off_y;
    gint cursor_cc;
    gint row1_cc, row2_cc, row3_cc;
    gint i;

    gchar strTemp[NFVKLABEL_MAX_STRING_SIZE];
    
    drawable = nfui_nfobject_get_window((NFOBJECT*)nfvklabel);
    nfui_nfobject_get_offset((NFOBJECT*)nfvklabel, &off_x, &off_y);

    cursor_cc = nfvklabel->cursor_cc;
    row1_cc = nfvklabel->row1_cc;
    row2_cc = nfvklabel->row2_cc;
    row3_cc = nfvklabel->row3_cc;   

    memset(strTemp, 0x00, sizeof(strTemp));

    if (nfvklabel->row_count == 1)
    {
        if (nfvklabel->invisible)
        {
            strTemp[0] = '*';
            
        	str_len = _strlen(nfvklabel->row1_label);
            str_width = str_len * nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
            
            tmp_str_width = cursor_cc * nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
        }
        else
        {
            str_width = nfutil_string_width(0, drawable, nfvklabel->pango_font, nfvklabel->row1_label, nfvklabel->spacing_type);

            if (cursor_cc > 0)
            {
    		    _strncpy(strTemp, nfvklabel->row1_label, cursor_cc);
                tmp_str_width = nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
            }
        }
    }
    else if (nfvklabel->row_count == 2)
    {
        if (cursor_cc < row1_cc)
        {
            if(nfvklabel->invisible)
            {
                strTemp[0] = '*';
                
            	str_len = _strlen(nfvklabel->row1_label);
                str_width = str_len * nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
                
                tmp_str_width = cursor_cc * nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
            }
            else                
            {
                str_width = nfutil_string_width(0, drawable, nfvklabel->pango_font, nfvklabel->row1_label, nfvklabel->spacing_type);

                if (cursor_cc > 0)
                {
    			    _strncpy(strTemp, nfvklabel->row1_label, cursor_cc);
                    tmp_str_width = nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
                }
            }
        }
        else
        {
            if(nfvklabel->invisible)
            {
                strTemp[0] = '*';
                
            	str_len = _strlen(nfvklabel->row2_label);
                str_width = str_len * nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
                
                tmp_str_width = (cursor_cc-row1_cc) * nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
            }
            else                
            {
                str_width = nfutil_string_width(0, drawable, nfvklabel->pango_font, nfvklabel->row2_label, nfvklabel->spacing_type);

                if (cursor_cc > row1_cc)
                {
    			    _strncpy(strTemp, nfvklabel->row2_label, cursor_cc-row1_cc);
                    tmp_str_width = nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
                }
            }
        }
    }
    else if (nfvklabel->row_count == 3)
    {
        if (cursor_cc < row1_cc)
        {
            if(nfvklabel->invisible)
            {
                strTemp[0] = '*';
                
            	str_len = _strlen(nfvklabel->row1_label);
                str_width = str_len * nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
                
                tmp_str_width = cursor_cc * nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
            }
            else                
            {
                str_width = nfutil_string_width(0, drawable, nfvklabel->pango_font, nfvklabel->row1_label, nfvklabel->spacing_type);

                if (cursor_cc > 0)
                {
    			    _strncpy(strTemp, nfvklabel->row1_label, cursor_cc);
                    tmp_str_width = nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
                }
            }
        }
        else if (cursor_cc < row2_cc)
        {
            if(nfvklabel->invisible)
            {
                strTemp[0] = '*';
                
            	str_len = _strlen(nfvklabel->row2_label);
                str_width = str_len * nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
                
                tmp_str_width = (cursor_cc-row1_cc) * nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
            }
            else                
            {
                str_width = nfutil_string_width(0, drawable, nfvklabel->pango_font, nfvklabel->row2_label, nfvklabel->spacing_type);

                if (cursor_cc > row1_cc)
                {
    			    _strncpy(strTemp, nfvklabel->row2_label, cursor_cc-row1_cc);
                    tmp_str_width = nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
                }
            }
        }
        else
        {
            if(nfvklabel->invisible)
            {
                strTemp[0] = '*';
                
            	str_len = _strlen(nfvklabel->row3_label);
                str_width = str_len * nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
                
                tmp_str_width = (cursor_cc-row2_cc) * nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
            }
            else                
            {
                str_width = nfutil_string_width(0, drawable, nfvklabel->pango_font, nfvklabel->row3_label, nfvklabel->spacing_type);

                if (cursor_cc > row2_cc)
                {
    			    _strncpy(strTemp, nfvklabel->row3_label, cursor_cc-row2_cc);
                    tmp_str_width = nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
                }
            }
        }
    }

    if (nfvklabel->align == NFALIGN_LEFT)
    {
        *cursor_pos_x = off_x + nfvklabel->margin + tmp_str_width;
    }
    else if (nfvklabel->align == NFALIGN_RIGHT)
    {
        *cursor_pos_x = off_x + nfvklabel->object.width;
        *cursor_pos_x -= (nfvklabel->margin + str_width);
        *cursor_pos_x += tmp_str_width;     
    }
    else    //(nfvklabel->align == NFALIGN_CENTER)
    {   
        *cursor_pos_x = off_x + (nfvklabel->object.width - str_width)/2;
        *cursor_pos_x += tmp_str_width;     
    }
    
    memset(strTemp, 0x00, sizeof(strTemp));
	str_len = _strlen(nfvklabel->strLabel);

    if (str_len > cursor_cc)
    {
#if 0		
        if(nfvklabel->invisible)    
            strTemp[0] = '*';
        else
            strTemp[0] = nfvklabel->strLabel[cursor_cc];
#else
        if(nfvklabel->invisible)
    		strTemp[0] = '*';
        else
    		g_utf8_strncpy(strTemp, g_utf8_offset_to_pointer(nfvklabel->strLabel, cursor_cc), 1);
#endif		
    }
    else        
        strTemp[0] = 'A';
        
    *cursor_pos_w = nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
}

static void _set_cursor(NFVKLABEL *nfvklabel, gint cursor_pos_x, gint cursor_pos_w)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc;

    gint bg_col_idx, fg_col_idx;
    gint off_x, off_y;
    gint cursor_cc, str_len;

    gchar strTemp[NFVKLABEL_MAX_STRING_SIZE];

    drawable = nfui_nfobject_get_window((NFOBJECT*)nfvklabel);
    gc = nfui_nfobject_get_gc((NFOBJECT*)nfvklabel);
    
    nfui_nfobject_get_offset((NFOBJECT*)nfvklabel, &off_x, &off_y);

    memset(strTemp, 0x00, sizeof(strTemp));

    cursor_cc = nfvklabel->cursor_cc;
	str_len = _strlen(nfvklabel->strLabel);

    if (nfvklabel->row_count >= 2)
    {
        if (cursor_cc >= nfvklabel->row1_cc)        off_y += nfvklabel->row_h;
    }

    if (nfvklabel->row_count >= 3)
    {
        if (cursor_cc >= nfvklabel->row2_cc)        off_y += nfvklabel->row_h;
    }
    
    if (nfvklabel->cursor_flicker)
    {
        bg_col_idx = nfui_nfobject_get_bg_color((NFOBJECT*)nfvklabel, NFOBJECT_STATE_NORMAL);
        fg_col_idx = nfvklabel->fg_color_idx;
    }
    else
    {
        bg_col_idx = nfvklabel->fg_color_idx;
        fg_col_idx = nfui_nfobject_get_bg_color((NFOBJECT*)nfvklabel, NFOBJECT_STATE_NORMAL);
    }

    gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_col_idx));
    gdk_draw_rectangle(drawable,
                    gc,
                    TRUE,
                    cursor_pos_x,
                    off_y+3,
                    cursor_pos_w,
                    nfvklabel->row_h-6);

    if (str_len > cursor_cc)
    {
#if 0
        if(nfvklabel->invisible)
            strTemp[0] = '*';
        else    
            strTemp[0] = nfvklabel->strLabel[cursor_cc];
#else
        if(nfvklabel->invisible)
    		strTemp[0] = '*';
        else
			g_utf8_strncpy(strTemp, g_utf8_offset_to_pointer(nfvklabel->strLabel, cursor_cc), 1);
#endif
    }

    nfutil_draw_short_text_eng_vk(NULL, 
                                NULL, 
                                (bg_col_idx < 0 ? NULL : (&UX_COLOR(bg_col_idx))), 
                                drawable, 
								gc,
                                strTemp, 
                                0, 
                                cursor_pos_x, 
                                off_y, 
                                cursor_pos_w, 
                                nfvklabel->row_h, 
                                nfvklabel->pango_font, 
                                &UX_COLOR(fg_col_idx), 
                                NULL, 
                                nfvklabel->align, 
                                0, 
                                0, 
                                nfvklabel->spacing_type );      
                                
    nfui_nfobject_gc_unref(gc);                                 
}

static gboolean _draw_cursor_display(gpointer data)
{
    NFVKLABEL *nfvklabel = (NFVKLABEL *)data;
    gint pos_x, pos_w;

    _get_cursor_param(nfvklabel, &pos_x, &pos_w);
    _set_cursor(nfvklabel, pos_x, pos_w);

    if (nfvklabel->cursor_flicker)  
        nfvklabel->cursor_flicker = 0;
    else            
        nfvklabel->cursor_flicker = 1;

    return TRUE;
}

static void _erase_cursor_display(gpointer data)
{
    NFVKLABEL *nfvklabel = (NFVKLABEL *)data;
    gint pos_x, pos_w;

    if (nfvklabel->cursor_flicker == 1)
    {
        _get_cursor_param(nfvklabel, &pos_x, &pos_w);
        _set_cursor(nfvklabel, pos_x, pos_w);

        nfvklabel->cursor_flicker = 0;          
    }
}

static void _start_cursor_timer(NFVKLABEL *nfvklabel)
{
    if (!nfvklabel->use_cursor) return;

    nfvklabel->cursor_flicker = 0;
    _draw_cursor_display((gpointer)nfvklabel);
    
    if (nfvklabel->cursor_tid) return;

    nfvklabel->cursor_tid = g_timeout_add_full(G_PRIORITY_DEFAULT, 500, _draw_cursor_display, nfvklabel, _erase_cursor_display);
}

static void _stop_cursor_timer(NFVKLABEL *nfvklabel)
{
    if (!nfvklabel->cursor_tid) return;

    g_source_remove(nfvklabel->cursor_tid);
    nfvklabel->cursor_tid = 0;
}

static gint _get_cursor_count(NFVKLABEL *nfvklabel, gint mouse_x, gint mouse_y)
{
    GdkDrawable *drawable = NULL;
    gint off_x, off_y;

    gint cnt = 0; 
    gint row_len, str_len; 
    gint str_width;

    gint display_sx;
    gint compare_sx, compare_ex;

    gint mouse_row; 
    gint row1_cc, row2_cc, row3_cc;
    
    gchar tmpLabel[NFVKLABEL_MAX_STRING_SIZE];  
    gchar rowLabel[NFVKLABEL_MAX_STRING_SIZE];  

    drawable = nfui_nfobject_get_window((NFOBJECT*)nfvklabel);
    nfui_nfobject_get_offset((NFOBJECT*)nfvklabel, &off_x, &off_y);

    row1_cc = nfvklabel->row1_cc;
    row2_cc = nfvklabel->row2_cc;
    row3_cc = nfvklabel->row3_cc;

    if ((mouse_y >= off_y) && (mouse_y <= off_y+nfvklabel->row_h)) 
        mouse_row = 1;
    else if ((mouse_y > off_y+nfvklabel->row_h) && (mouse_y <= off_y+nfvklabel->row_h*2)) 
        mouse_row = 2;      
    else if ((mouse_y > off_y+nfvklabel->row_h*2) && (mouse_y <= off_y+nfvklabel->row_h*3)) 
        mouse_row = 3;
    else 
        g_assert(0);

    memset(rowLabel, 0x00, sizeof(rowLabel));

    if (mouse_row == 1)         strcpy(rowLabel, nfvklabel->row1_label);
    else if (mouse_row == 2)    strcpy(rowLabel, nfvklabel->row2_label);
    else if (mouse_row == 3)    strcpy(rowLabel, nfvklabel->row3_label);

    str_width = nfutil_string_width(0, drawable, nfvklabel->pango_font, rowLabel, nfvklabel->spacing_type);

    if (nfvklabel->align == NFALIGN_LEFT)
    {
        display_sx = off_x + nfvklabel->margin;
    }
    else if (nfvklabel->align == NFALIGN_RIGHT)
    {
        display_sx = off_x + nfvklabel->object.width;
        display_sx -= (nfvklabel->margin + str_width);
    }
    else    //(nfvklabel->align == NFALIGN_CENTER)
    {
        display_sx = off_x + (nfvklabel->object.width - str_width)/2;
    }
    
    if (mouse_x <= display_sx)
    {
        if (mouse_row == 1)         return 0;
        else if (mouse_row == 2)    return row1_cc;
        else if (mouse_row == 3)    return row2_cc;     
    }

	row_len = _strlen(rowLabel);

    if (row_len == 0)
    {
        if (mouse_row == 1)         return 0;
        else if (mouse_row == 2)    return row1_cc;
        else if (mouse_row == 3)    return row2_cc;
    }

    for (cnt = 0; cnt < row_len; cnt++)
    {
        memset(tmpLabel, 0x00, sizeof(tmpLabel));

        if (cnt)
        {
			_strncpy(tmpLabel, rowLabel, cnt);
            compare_sx = display_sx + nfutil_string_width(0, drawable, nfvklabel->pango_font, tmpLabel, nfvklabel->spacing_type);
        }
        else 
            compare_sx = display_sx;
                
        memset(tmpLabel, 0x00, sizeof(tmpLabel));
		_strncpy(tmpLabel, rowLabel, cnt+1);

        compare_ex = display_sx + nfutil_string_width(0, drawable, nfvklabel->pango_font, tmpLabel, nfvklabel->spacing_type);
    
        if ((mouse_x >= compare_sx) && (mouse_x <= compare_ex))
        {
            if (mouse_row == 1)         return cnt;
            else if (mouse_row == 2)    return cnt+row1_cc;
            else if (mouse_row == 3)    return cnt+row2_cc;
        }
    }

	str_len = _strlen(nfvklabel->strLabel);

    if (mouse_row == 1)
    {
        if ((row1_cc == nfvklabel->max_cc) || (row1_cc < str_len))
            return row1_cc-1;
        else
            return row1_cc;
    }
    else if (mouse_row == 2)    
    {
        if ((row2_cc == nfvklabel->max_cc) || (row2_cc < str_len))
            return row2_cc-1;
        else
            return row2_cc;
    }
    else if (mouse_row == 3)    
    {
        if (row3_cc == nfvklabel->max_cc)
            return row3_cc-1;
        else
            return row3_cc;
    }

    return 0;
}

static gint _set_row1_label(NFVKLABEL *nfvklabel)
{
    GdkDrawable *drawable = NULL;
    gchar tmp_label[NFVKLABEL_MAX_STRING_SIZE]; 
    gint str_len, str_width;
    gint i;

    nfvklabel->row_count = 1;

    drawable = nfui_nfobject_get_window((NFOBJECT*)nfvklabel);
	str_len = _strlen(nfvklabel->strLabel);

    for (i = 0; i < str_len; i++)
    {
        memset(tmp_label, 0x00, sizeof(tmp_label));
		_strncpy(tmp_label, nfvklabel->strLabel, str_len-i);
        str_width = nfutil_string_width(0, drawable, nfvklabel->pango_font, tmp_label, nfvklabel->spacing_type);

        if (str_width + nfvklabel->margin*2 < nfvklabel->object.width)
        {
            strcpy(nfvklabel->row1_label, tmp_label);
            break;
        }
    }
/*
    nfvklabel->row1_cc = str_len - i;   

    if (nfvklabel->row1_cc == nfvklabel->max_cc) return 0;
*/
    if (i == 0)
    {
        gint cursor_w;
    
        nfvklabel->row1_cc = str_len;

        memset(tmp_label, 0x00, sizeof(tmp_label));
        tmp_label[0] = 'A';     
        cursor_w = nfutil_string_width(0, drawable, nfvklabel->pango_font, tmp_label, nfvklabel->spacing_type);

        if (str_width + nfvklabel->margin*2 + cursor_w > nfvklabel->object.width)
            nfvklabel->row_count = 2;
        
        return 0;
    }

	nfvklabel->row1_cc = str_len - i;

    return 1;
}

static gint _set_row2_label(NFVKLABEL *nfvklabel)
{
    GdkDrawable *drawable = NULL;
    gchar tmp_label[NFVKLABEL_MAX_STRING_SIZE]; 
    gint str_len, str_width;
    gint i; 
    gint row1_cc;

    nfvklabel->row_count = 2;

    drawable = nfui_nfobject_get_window((NFOBJECT*)nfvklabel);
	str_len = _strlen(nfvklabel->strLabel);
    row1_cc = nfvklabel->row1_cc;

    for (i = row1_cc; i < str_len; i++)
    {
        memset(tmp_label, 0x00, sizeof(tmp_label));
		_strncpy(tmp_label, g_utf8_offset_to_pointer(nfvklabel->strLabel, row1_cc), str_len-i);
        str_width = nfutil_string_width(0, drawable, nfvklabel->pango_font, tmp_label, nfvklabel->spacing_type);

        if (str_width + nfvklabel->margin*2 < nfvklabel->object.width)
        {
            strcpy(nfvklabel->row2_label, tmp_label);
            break;
        }
    }
/*
    nfvklabel->row2_cc = str_len-i+row1_cc; 

    if (row1_cc+nfvklabel->row2_cc == nfvklabel->max_cc) return 0;
  */          
    if (i == row1_cc)
    {
        gint cursor_w;
    
		nfvklabel->row2_cc = str_len;

        memset(tmp_label, 0x00, sizeof(tmp_label));
        tmp_label[0] = 'A';     
        cursor_w = nfutil_string_width(0, drawable, nfvklabel->pango_font, tmp_label, nfvklabel->spacing_type);

        if (str_width + nfvklabel->margin*2 + cursor_w > nfvklabel->object.width)
            nfvklabel->row_count = 3;
        
        return 0;
    }

	nfvklabel->row2_cc = str_len-i+row1_cc;

    return 1;
}

static gint _set_row3_label(NFVKLABEL *nfvklabel)
{
    GdkDrawable *drawable = NULL;
    gchar tmp_label[NFVKLABEL_MAX_STRING_SIZE]; 
    gint str_len, str_width;
    gint i;
    gint row2_cc;

    nfvklabel->row_count = 3;

    drawable = nfui_nfobject_get_window((NFOBJECT*)nfvklabel);
	str_len = _strlen(nfvklabel->strLabel);
    row2_cc = nfvklabel->row2_cc;

    for (i = row2_cc; i < str_len; i++)
    {
        memset(tmp_label, 0x00, sizeof(tmp_label));
		_strncpy(tmp_label, g_utf8_offset_to_pointer(nfvklabel->strLabel, row2_cc), str_len-i);
        str_width = nfutil_string_width(0, drawable, nfvklabel->pango_font, tmp_label, nfvklabel->spacing_type);

        if (str_width + nfvklabel->margin*2 < nfvklabel->object.width)
        {
            strcpy(nfvklabel->row3_label, tmp_label);
            break;
        }
    }
        
    if (i == row2_cc)
    {
        nfvklabel->row3_cc = str_len;   
        return 0;
    }

    g_message("%s, %d, Not enough vklabel size", __FUNCTION__, __LINE__);
    g_assert(0);

    return 0;
}

static gint _init_vklabel(NFVKLABEL *nfvklabel)
{
    gint str_len;
    gint ret_val;

	str_len = _strlen(nfvklabel->strLabel);

    nfvklabel->row_count = 1;
    nfvklabel->row_h = nfvklabel->object.height;

    if (str_len)
    {
        ret_val = _set_row1_label(nfvklabel);

        if (ret_val)
        {
            ret_val = _set_row2_label(nfvklabel);

            if (ret_val)
                ret_val = _set_row3_label(nfvklabel);           
        }

        nfvklabel->cursor_state = VK_DRAG;
        nfvklabel->cursor_dsc = 0;
        nfvklabel->cursor_dec = str_len - 1;
    }

    nfvklabel->init_display = 0;

    if (nfvklabel->row_count != 1)
    {
        evt_send_to_local(INFY_VKEY_SIZE_INCREASE, 0, 0, GINT_TO_POINTER((nfvklabel->row_h)*(nfvklabel->row_count-1)));
        nfvklabel->object.height = (nfvklabel->row_h)*(nfvklabel->row_count);
        return 1;
    }

    return 0;
}

static gint _lineup_vklabel(NFVKLABEL *nfvklabel)
{
    gint row_count;
    gint str_len;
    gint ret_val;
    
    row_count = nfvklabel->row_count;

    nfvklabel->row_count = 1;
	str_len = _strlen(nfvklabel->strLabel);

    memset(nfvklabel->row1_label, 0x00, sizeof(nfvklabel->row1_label));
    memset(nfvklabel->row2_label, 0x00, sizeof(nfvklabel->row2_label));
    memset(nfvklabel->row3_label, 0x00, sizeof(nfvklabel->row3_label));

    nfvklabel->row1_cc = 0;
    nfvklabel->row2_cc = 0;
    nfvklabel->row3_cc = 0;

    if (str_len)
    {
        ret_val = _set_row1_label(nfvklabel);

        if (ret_val)
        {
            ret_val = _set_row2_label(nfvklabel);

            if (ret_val)
                ret_val = _set_row3_label(nfvklabel);           
        }
    }

    if (row_count != nfvklabel->row_count)
    {
        if (row_count > nfvklabel->row_count)
        {
            evt_send_to_local(INFY_VKEY_SIZE_DECREASE, 0, 0, GINT_TO_POINTER((nfvklabel->row_h)*(row_count - nfvklabel->row_count)));
            nfvklabel->object.height = (nfvklabel->row_h)*(nfvklabel->row_count);
            return 1;
        }
        else
        {
            evt_send_to_local(INFY_VKEY_SIZE_INCREASE, 0, 0, GINT_TO_POINTER((nfvklabel->row_h)*(nfvklabel->row_count-row_count)));
            nfvklabel->object.height = (nfvklabel->row_h)*(nfvklabel->row_count);
            return 1;
        }
    }

    return 0;   
}

static void _display_row1_label(NFVKLABEL *nfvklabel)
{
    GdkDrawable *drawable;
    GdkGC *gc;
    gchar strTemp[NFVKLABEL_MAX_STRING_SIZE];
    gint bg_col_idx, fg_col_idx;
    gint str_len;
    guint left, top;
    gint i;     

    drawable = nfui_nfobject_get_window((NFOBJECT*)nfvklabel);
    gc = nfui_nfobject_get_gc((NFOBJECT*)nfvklabel);

    bg_col_idx = nfui_nfobject_get_bg_color((NFOBJECT*)nfvklabel, NFOBJECT_STATE_NORMAL);
    fg_col_idx = nfvklabel->fg_color_idx;

    nfui_nfobject_get_offset((NFOBJECT*)nfvklabel, &left, &top);
    
    gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_col_idx));
    gdk_draw_rectangle(drawable, gc, TRUE, left, top, nfvklabel->object.width, nfvklabel->row_h);

	str_len = _strlen(nfvklabel->row1_label);

    if (str_len)
    {
        memset(strTemp, 0, sizeof(strTemp));

        if(nfvklabel->invisible)
        {
            for(i = 0; i < str_len; i++)
                strTemp[i] = '*';       
        }
        else
            strcpy(strTemp, nfvklabel->row1_label);

        nfutil_draw_short_text_eng_vk(NULL, 
                                    NULL, 
                                    (bg_col_idx < 0 ? NULL : (&UX_COLOR(bg_col_idx))), 
                                    drawable, 
									gc,
                                    strTemp, 
                                    0, 
                                    left, 
                                    top, 
                                    nfvklabel->object.width, 
                                    nfvklabel->row_h, 
                                    nfvklabel->pango_font, 
                                    &UX_COLOR(fg_col_idx), 
                                    NULL, 
                                    nfvklabel->align, 
                                    nfvklabel->margin, 
                                    0, 
                                    nfvklabel->spacing_type );
    }

    nfui_nfobject_gc_unref(gc);
}

static void _display_row2_label(NFVKLABEL *nfvklabel)
{
    GdkDrawable *drawable;
    GdkGC *gc;
    gchar strTemp[NFVKLABEL_MAX_STRING_SIZE];
    gint bg_col_idx, fg_col_idx;
    gint str_len;
    guint left, top;
    gint i;     

    drawable = nfui_nfobject_get_window((NFOBJECT*)nfvklabel);
    gc = nfui_nfobject_get_gc((NFOBJECT*)nfvklabel);

    bg_col_idx = nfui_nfobject_get_bg_color((NFOBJECT*)nfvklabel, NFOBJECT_STATE_NORMAL);
    fg_col_idx = nfvklabel->fg_color_idx;

    nfui_nfobject_get_offset((NFOBJECT*)nfvklabel, &left, &top);
    top += nfvklabel->row_h;
    
    gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_col_idx));
    gdk_draw_rectangle(drawable, gc, TRUE, left, top, nfvklabel->object.width, nfvklabel->row_h);

	str_len = _strlen(nfvklabel->row2_label);

    if (str_len)
    {
        memset(strTemp, 0, sizeof(strTemp));

        if(nfvklabel->invisible)
        {
            for(i = 0; i < str_len; i++)
                strTemp[i] = '*';       
        }
        else
            strcpy(strTemp, nfvklabel->row2_label);

        nfutil_draw_short_text_eng_vk(NULL, 
                                    NULL, 
                                    (bg_col_idx < 0 ? NULL : (&UX_COLOR(bg_col_idx))), 
                                    drawable, 
									gc,
                                    strTemp, 
                                    0, 
                                    left, 
                                    top, 
                                    nfvklabel->object.width, 
                                    nfvklabel->row_h, 
                                    nfvklabel->pango_font, 
                                    &UX_COLOR(fg_col_idx), 
                                    NULL, 
                                    nfvklabel->align, 
                                    nfvklabel->margin, 
                                    0, 
                                    nfvklabel->spacing_type );
    }

    nfui_nfobject_gc_unref(gc);
}

static void _display_row3_label(NFVKLABEL *nfvklabel)
{
    GdkDrawable *drawable;
    GdkGC *gc;
    gchar strTemp[NFVKLABEL_MAX_STRING_SIZE];
    gint bg_col_idx, fg_col_idx;
    gint str_len;
    guint left, top;
    gint i;     

    drawable = nfui_nfobject_get_window((NFOBJECT*)nfvklabel);
    gc = nfui_nfobject_get_gc((NFOBJECT*)nfvklabel);

    bg_col_idx = nfui_nfobject_get_bg_color((NFOBJECT*)nfvklabel, NFOBJECT_STATE_NORMAL);
    fg_col_idx = nfvklabel->fg_color_idx;

    nfui_nfobject_get_offset((NFOBJECT*)nfvklabel, &left, &top);
    top += nfvklabel->row_h*2;
    
    gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_col_idx));
    gdk_draw_rectangle(drawable, gc, TRUE, left, top, nfvklabel->object.width, nfvklabel->row_h);

	str_len = _strlen(nfvklabel->row3_label);

    if (str_len)
    {
        memset(strTemp, 0, sizeof(strTemp));

        if(nfvklabel->invisible)
        {
            for(i = 0; i < str_len; i++)
                strTemp[i] = '*';       
        }
        else
            strcpy(strTemp, nfvklabel->row3_label);

        nfutil_draw_short_text_eng_vk(NULL, 
                                    NULL, 
                                    (bg_col_idx < 0 ? NULL : (&UX_COLOR(bg_col_idx))), 
                                    drawable, 
									gc,
                                    strTemp, 
                                    0, 
                                    left, 
                                    top, 
                                    nfvklabel->object.width, 
                                    nfvklabel->row_h, 
                                    nfvklabel->pango_font, 
                                    &UX_COLOR(fg_col_idx), 
                                    NULL, 
                                    nfvklabel->align, 
                                    nfvklabel->margin, 
                                    0, 
                                    nfvklabel->spacing_type );
    }

    nfui_nfobject_gc_unref(gc);
}

static gint _move_cusor_cc(NFVKLABEL *nfvklabel, VK_MOVE_E move)
{
    GdkDrawable *drawable = NULL;
    gchar strTemp[NFVKLABEL_MAX_STRING_SIZE];

    gint cursor_dsc, cursor_dec;
    gint i, off_x, off_y;
    gint str_width;

    drawable = nfui_nfobject_get_window((NFOBJECT*)nfvklabel);
    nfui_nfobject_get_offset((NFOBJECT*)nfvklabel, &off_x, &off_y);

    off_y += (nfvklabel->row_h / 2);

    memset(strTemp, 0, sizeof(strTemp));
    
    if (nfvklabel->cursor_state == VK_SELECT)
    {
        if (move == VK_MOVE_LEFT)
        {
            if (nfvklabel->cursor_cc == 0) return -1;

            _stop_cursor_timer(nfvklabel);
            nfvklabel->cursor_cc -= 1;
            _start_cursor_timer(nfvklabel);     
        }
        else if (move == VK_MOVE_RIGHT)
        {       
            if (nfvklabel->cursor_cc == nfvklabel->max_cc-1) return -1;
			if (nfvklabel->cursor_cc == _strlen(nfvklabel->strLabel)) return -1;

            _stop_cursor_timer(nfvklabel);
            nfvklabel->cursor_cc += 1;
            _start_cursor_timer(nfvklabel);     
        }
        else if (move == VK_MOVE_UP)
        {
            if (nfvklabel->row_count == 1) return -1;
            if (nfvklabel->cursor_cc < nfvklabel->row1_cc) return -1;

            if (nfvklabel->row_count == 2)
            {           
                _strncpy(strTemp, nfvklabel->row2_label, nfvklabel->cursor_cc-nfvklabel->row1_cc);
            }
            else if (nfvklabel->row_count == 3)
            {
                if ((nfvklabel->cursor_cc >= nfvklabel->row1_cc) && (nfvklabel->cursor_cc < nfvklabel->row2_cc))
                {
                    _strncpy(strTemp, nfvklabel->row2_label, nfvklabel->cursor_cc-nfvklabel->row1_cc);
                }
                else
                {
                    _strncpy(strTemp, nfvklabel->row3_label, nfvklabel->cursor_cc-nfvklabel->row2_cc);
                    off_y += nfvklabel->row_h;
                }
            }

            str_width = nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
            off_x += str_width;

            memset(strTemp, 0, sizeof(strTemp));
            strTemp[0] = nfvklabel->strLabel[nfvklabel->cursor_cc];
            str_width = nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
            off_x += (str_width / 2);

            _stop_cursor_timer(nfvklabel);
            nfvklabel->cursor_cc = _get_cursor_count(nfvklabel, off_x, off_y);
            _start_cursor_timer(nfvklabel);           
        }
        else if (move == VK_MOVE_DOWN)
        {
            if (nfvklabel->row_count == 1) return -1;

            if (nfvklabel->row_count == 2)
            {
                if ((nfvklabel->cursor_cc >= nfvklabel->row1_cc) && (nfvklabel->cursor_cc < nfvklabel->row2_cc)) return -1;
            }
            else if (nfvklabel->row_count == 3)
            {
                if ((nfvklabel->cursor_cc >= nfvklabel->row2_cc) && (nfvklabel->cursor_cc < nfvklabel->row3_cc)) return -1;
            }
            
            if (nfvklabel->row_count == 2)
            {
                _strncpy(strTemp, nfvklabel->row1_label, nfvklabel->cursor_cc);
                off_y += nfvklabel->row_h;
            }
            else if (nfvklabel->row_count == 3)
            {
                if (nfvklabel->cursor_cc < nfvklabel->row1_cc)
                {
                    _strncpy(strTemp, nfvklabel->row1_label, nfvklabel->cursor_cc);
                    off_y += nfvklabel->row_h;
                }
                else
                {
                    _strncpy(strTemp, nfvklabel->row2_label, nfvklabel->cursor_cc-nfvklabel->row1_cc);
                    off_y += (nfvklabel->row_h * 2);
                }
            }

            str_width = nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
            off_x += str_width;

            memset(strTemp, 0, sizeof(strTemp));
            strTemp[0] = nfvklabel->strLabel[nfvklabel->cursor_cc];
            str_width = nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
            off_x += (str_width / 2);

            _stop_cursor_timer(nfvklabel);
            nfvklabel->cursor_cc = _get_cursor_count(nfvklabel, off_x, off_y);
            _start_cursor_timer(nfvklabel);           
        }
    }
    else
    {
        if (nfvklabel->cursor_dsc > nfvklabel->cursor_dec)
        {
            cursor_dsc = nfvklabel->cursor_dec;
            cursor_dec = nfvklabel->cursor_dsc;
        }
        else
        {
            cursor_dsc = nfvklabel->cursor_dsc;
            cursor_dec = nfvklabel->cursor_dec;
        }   
    
        _erase_drag_section(nfvklabel);
        nfvklabel->cursor_state = VK_SELECT;
    
        if (move == VK_MOVE_LEFT)
        {       
            nfvklabel->cursor_cc = cursor_dsc;
            _start_cursor_timer(nfvklabel);     
        }
        else if (move == VK_MOVE_RIGHT)
        {       
            if (cursor_dec < nfvklabel->max_cc-1) 
                nfvklabel->cursor_cc = cursor_dec + 1;
            else
                nfvklabel->cursor_cc = cursor_dec;
                
            _start_cursor_timer(nfvklabel);     
        }
        else if (move == VK_MOVE_UP)
        {
            if (nfvklabel->row_count == 1) 
            {
                _start_cursor_timer(nfvklabel); 
                return -1;
            }
            
            nfvklabel->cursor_cc = cursor_dec;
            
            if (nfvklabel->cursor_cc < nfvklabel->row1_cc) 
            {
                _start_cursor_timer(nfvklabel); 
                return -1;
            }

            if (nfvklabel->row_count == 2)
            {
                _strncpy(strTemp, nfvklabel->row2_label, nfvklabel->cursor_cc-nfvklabel->row1_cc);
            }
            else if (nfvklabel->row_count == 3)
            {
                if ((nfvklabel->cursor_cc >= nfvklabel->row1_cc) && (nfvklabel->cursor_cc < nfvklabel->row2_cc))
                {
                    _strncpy(strTemp, nfvklabel->row2_label, nfvklabel->cursor_cc-nfvklabel->row1_cc);
                }
                else
                {
                    _strncpy(strTemp, nfvklabel->row3_label, nfvklabel->cursor_cc-nfvklabel->row2_cc);
                    off_y += nfvklabel->row_h;                    
                }
            }

            str_width = nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
            off_x += str_width;

            memset(strTemp, 0, sizeof(strTemp));
            strTemp[0] = nfvklabel->strLabel[nfvklabel->cursor_cc];
            str_width = nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
            off_x += (str_width / 2);

            nfvklabel->cursor_cc = _get_cursor_count(nfvklabel, off_x, off_y);
            _start_cursor_timer(nfvklabel);           
        }
        else if (move == VK_MOVE_DOWN)
        {
            if (nfvklabel->row_count == 1) 
            {
                _start_cursor_timer(nfvklabel); 
                return -1;
            }

            nfvklabel->cursor_cc = cursor_dec;

            if (nfvklabel->row_count == 2)
            {
                if ((nfvklabel->cursor_cc >= nfvklabel->row1_cc) && (nfvklabel->cursor_cc < nfvklabel->row2_cc)) 
                {
                    _start_cursor_timer(nfvklabel); 
                    return -1;
                }
            }
            else if (nfvklabel->row_count == 3)
            {
                if ((nfvklabel->cursor_cc >= nfvklabel->row2_cc) && (nfvklabel->cursor_cc < nfvklabel->row3_cc))
                {
                    _start_cursor_timer(nfvklabel); 
                    return -1;                  
                }
            }
            
            if (nfvklabel->row_count == 2)
            {
                _strncpy(strTemp, nfvklabel->row1_label, nfvklabel->cursor_cc);
                off_y += nfvklabel->row_h;
            }
            else if (nfvklabel->row_count == 3)
            {
                if (nfvklabel->cursor_cc < nfvklabel->row1_cc)
                {
                    _strncpy(strTemp, nfvklabel->row1_label, nfvklabel->cursor_cc);
                    off_y += nfvklabel->row_h;
                }
                else
                {
                    _strncpy(strTemp, nfvklabel->row2_label, nfvklabel->cursor_cc-nfvklabel->row1_cc);
                    off_y += (nfvklabel->row_h * 2);
                }
            }

            str_width = nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
            off_x += str_width;

            memset(strTemp, 0, sizeof(strTemp));
            strTemp[0] = nfvklabel->strLabel[nfvklabel->cursor_cc];
            str_width = nfutil_string_width(0, drawable, nfvklabel->pango_font, strTemp, nfvklabel->spacing_type);
            off_x += (str_width / 2);

            nfvklabel->cursor_cc = _get_cursor_count(nfvklabel, off_x, off_y);
            _start_cursor_timer(nfvklabel); 
        }       
    }

    return 0;
}

static gboolean nfvklabel_event_handler(NFVKLABEL *nfvklabel, GdkEvent *event, gpointer data)
{
    switch(event->type)
    {
        case GDK_EXPOSE :
        {   
            if (nfvklabel->init_display)
            {
                if (_init_vklabel(nfvklabel))
                    return FALSE;
            }

            if (nfvklabel->cursor_state == VK_SELECT)
                _stop_cursor_timer(nfvklabel);
            else
                _erase_drag_section(nfvklabel);

            if (nfvklabel->row_count > 0) _display_row1_label(nfvklabel);
            if (nfvklabel->row_count > 1) _display_row2_label(nfvklabel);
            if (nfvklabel->row_count > 2) _display_row3_label(nfvklabel);               

            if(nfvklabel->object.kfocus != NFOBJECT_FOCUS)
                nfvklabel->object.grab_kfocus = FALSE;

            if (nfvklabel->draw_outline)
            {
                if (nfvklabel->object.grab_kfocus)
                    nfvklabel_draw_outlines((NFOBJECT*)nfvklabel, 147);         
                else if (nfvklabel->object.kfocus == NFOBJECT_FOCUS)
                    nfvklabel_draw_outlines((NFOBJECT*)nfvklabel, 146);
            }

            if (nfvklabel->cursor_state == VK_SELECT)
                _start_cursor_timer(nfvklabel);
            else
                _draw_drag_section(nfvklabel);
        }
        break;

        case GDK_ENTER_NOTIFY:
        case GDK_LEAVE_NOTIFY:      
        {
            if(nfvklabel->draw_outline && nfui_nfobject_is_shown((NFOBJECT*)nfvklabel))
                nfui_signal_emit((NFOBJECT*)nfvklabel, GDK_EXPOSE, FALSE);
            
            nfvklabel->is_pressed = 0;
        }
        break;

        case GDK_BUTTON_PRESS :
        {
            GdkEventButton *bevent;
            
            bevent = (GdkEventButton *)event;
            
            if (nfvklabel->cursor_state == VK_SELECT)
                _stop_cursor_timer(nfvklabel);
            else
                _erase_drag_section(nfvklabel);
            
            nfvklabel->cursor_state = VK_SELECT;                
            nfvklabel->cursor_cc = _get_cursor_count(nfvklabel, (gint)bevent->x, (gint)bevent->y);

            _start_cursor_timer(nfvklabel);

            nfvklabel->is_pressed = 1;
        }
        break;

        case GDK_MOTION_NOTIFY:
        {
            GdkEventButton *bevent;
            gint count;
            gint str_len;

            bevent = (GdkEventButton *)event;
			str_len = _strlen(nfvklabel->strLabel);

            if (nfvklabel->is_pressed)
            {
                if (!str_len) return FALSE;                 
            
                count = _get_cursor_count(nfvklabel, (gint)bevent->x, (gint)bevent->y);

                if (nfvklabel->cursor_state != VK_DRAG)
                {
                    if ((count == str_len) && (nfvklabel->cursor_cc == str_len)) return FALSE;
                
                    if (nfvklabel->cursor_state == VK_SELECT)
                        _stop_cursor_timer(nfvklabel);
                    else
                        _erase_drag_section(nfvklabel);

                    if (count >= nfvklabel->cursor_cc) 
                    {
                        nfvklabel->cursor_dsc = count;
                        nfvklabel->cursor_dec = count;
                    }
                    else
                    {
                        if (count < 1) return FALSE;
                        
                        nfvklabel->cursor_dsc = count-1;
                        nfvklabel->cursor_dec = count-1;
                    }

                    nfvklabel->cursor_state = VK_DRAG;
                    _draw_drag_section(nfvklabel);
                }
                else
                {
                    gint cursor_dsc, cursor_dec;
                
                    if (count >= nfvklabel->cursor_cc)
                    {
                        cursor_dsc = nfvklabel->cursor_cc;
                        if (count == str_len) count -= 1;
                        cursor_dec = count;
                    }
                    else
                    {
                        cursor_dsc = nfvklabel->cursor_cc - 1;
                        cursor_dec = count;
                    }

                    if ((cursor_dsc != nfvklabel->cursor_dsc) || (cursor_dec != nfvklabel->cursor_dec))
                    {
                        _erase_drag_section(nfvklabel);

                        nfvklabel->cursor_dsc = cursor_dsc;
                        nfvklabel->cursor_dec = cursor_dec;                         

                        _draw_drag_section(nfvklabel);
                    }
                }
            }
        }
        break;

        case GDK_BUTTON_RELEASE :
        {
            nfvklabel->is_pressed = 0;

            if (nfvklabel->cursor_state == VK_DRAG)
            {
                gint str_len;
            
				str_len = _strlen(nfvklabel->strLabel);

                if ((str_len == nfvklabel->cursor_cc) && (str_len == nfvklabel->cursor_dsc) && (str_len == nfvklabel->cursor_dec))
                {
                    _erase_drag_section(nfvklabel);
                    _start_cursor_timer(nfvklabel);
                }
            }
        }
        break;

        case GDK_2BUTTON_PRESS :
        {
			if (_strlen(nfvklabel->strLabel) == 0) return FALSE;
        
            if (nfvklabel->cursor_state == VK_SELECT)
                _stop_cursor_timer(nfvklabel);
            else
                _erase_drag_section(nfvklabel);
            
            nfvklabel->cursor_state = VK_DRAG;
            nfvklabel->cursor_dsc = 0;

			if (_strlen(nfvklabel->strLabel))
				nfvklabel->cursor_dec = _strlen(nfvklabel->strLabel)-1;
            else
                nfvklabel->cursor_dec = 0;              

            _draw_drag_section(nfvklabel);
        }
        break;

        case NFEVENT_KEYPAD_PRESS :
        case NFEVENT_REMOCON_PRESS :
        {
            GdkEventKey *kevt;
            KEYPAD_KID kpid;

            kevt = (GdkEventKey*)event;
            kpid = (KEYPAD_KID)kevt->keyval;

            switch(kpid)
            {
                case KEYPAD_ENTER:
                {
                    if (nfvklabel->object.grab_kfocus)
                        nfvklabel->object.grab_kfocus = FALSE;
                    else
                        nfvklabel->object.grab_kfocus = TRUE;

                    nfui_signal_emit((NFOBJECT*)nfvklabel, GDK_EXPOSE, TRUE);
                }
                break;
                
                case KEYPAD_EXIT:
                {
                    if (nfvklabel->object.grab_kfocus)
                    {
                        nfvklabel->object.grab_kfocus = FALSE;
                        nfui_signal_emit((NFOBJECT*)nfvklabel, GDK_EXPOSE, TRUE);
                    }
                }
                break;
                
                case KEYPAD_LEFT:
                {
                    if (nfvklabel->object.grab_kfocus) 
                    {
                        _move_cusor_cc(nfvklabel, VK_MOVE_LEFT);
                        return TRUE;
                    }
                }
                break;
                
                case KEYPAD_RIGHT:
                {
                    if (nfvklabel->object.grab_kfocus) 
                    {
                        _move_cusor_cc(nfvklabel, VK_MOVE_RIGHT);
                        return TRUE;
                    }
                }
                break;
                
                case KEYPAD_UP:
                {
                    if (nfvklabel->object.grab_kfocus) 
                    {               
                        _move_cusor_cc(nfvklabel, VK_MOVE_UP);
                        return TRUE;
                    }               
                }
                break;
                
                case KEYPAD_DOWN:
                {
                    if (nfvklabel->object.grab_kfocus) 
                    {               
                        _move_cusor_cc(nfvklabel, VK_MOVE_DOWN);
                        return TRUE;
                    }
                }
                break;
                
                default:
                break;
            }
        }
        break;          

        case GDK_DELETE :
        {   
            _stop_cursor_timer(nfvklabel);
        }
        break;

        default :
            break;
    }

    return FALSE;
}

NFVKLABEL* nfui_nfvklabel_new_str(gchar *str, gint max_char_cnt)
{
    NFVKLABEL *nfvklabel;
    gint length;

    nfvklabel = (NFVKLABEL*)imalloc(sizeof(NFVKLABEL));
    if(nfvklabel == NULL)   return NULL;
    
    nfui_nfobject_init((NFOBJECT*)nfvklabel);

    nfvklabel->object.width = 100;
    nfvklabel->object.height = 22;
    nfvklabel->object.type = NFOBJECT_TYPE_NFVKLABEL;
    nfvklabel->object.use_focus = NFOBJECT_FOCUS_ON;    //OFF;
    nfvklabel->object.default_event_handler = nfvklabel_event_handler;
    nfvklabel->object.use_tooltip = 1;

    nfvklabel->init_display = 1;
    nfvklabel->is_pressed = 0;
    
    nfvklabel->max_cc = max_char_cnt;

	length = _strlen(str);

    if (length)
		_strncpy(nfvklabel->strLabel, str, sizeof(gchar)*length);

    nfvklabel->row1_cc = 0;
    nfvklabel->row2_cc = 0;
    nfvklabel->row3_cc = 0;

    nfvklabel->cursor_tid = 0;
    nfvklabel->cursor_flicker = 0;  
    nfvklabel->cursor_state = VK_SELECT;
    
    if (nfvklabel->max_cc == length)
        nfvklabel->cursor_cc = length-1;
    else
        nfvklabel->cursor_cc = length;      
    nfvklabel->cursor_dsc = 0;
    nfvklabel->cursor_dec = 0;
    

    strcpy(nfvklabel->font_name, "Calibri 16");
    nfvklabel->pango_font = nfvklabel->font_name;
    nfvklabel->fg_color_idx = COLOR_IDX(129);

    nfvklabel->use_num = 0;
    nfvklabel->use_cursor = 1;
    nfvklabel->invisible = 0;   
    
    nfvklabel->draw_outline = TRUE; 

    nfvklabel->align = NFALIGN_LEFT;
    nfvklabel->margin = VKLABEL_MARGIN;

    return nfvklabel;
}

NFVKLABEL* nfui_nfvklabel_new_num(gint num, gint max_char_cnt)
{
    NFVKLABEL *nfvklabel;
    gint length;
    gchar strTemp[64];

    nfvklabel = (NFVKLABEL*)imalloc(sizeof(NFVKLABEL));
    if(nfvklabel == NULL)   return NULL;
    
    nfui_nfobject_init((NFOBJECT*)nfvklabel);

    nfvklabel->object.width = 100;
    nfvklabel->object.height = 22;
    nfvklabel->object.type = NFOBJECT_TYPE_NFVKLABEL;
    nfvklabel->object.use_focus = NFOBJECT_FOCUS_ON;    //OFF;
    nfvklabel->object.default_event_handler = nfvklabel_event_handler;
    nfvklabel->object.use_tooltip = 1;

    nfvklabel->init_display = 1;
    nfvklabel->is_pressed = 0;

    memset(strTemp, 0x00, sizeof(strTemp));
    g_sprintf(strTemp, "%d", num);

    nfvklabel->max_cc = max_char_cnt;

	length = _strlen(strTemp);

    if (length)
		_strncpy(nfvklabel->strLabel, strTemp, sizeof(gchar)*length);

    nfvklabel->row1_cc = 0;
    nfvklabel->row2_cc = 0;
    nfvklabel->row3_cc = 0;

    nfvklabel->cursor_tid = 0;
    nfvklabel->cursor_flicker = 0;  
    nfvklabel->cursor_state = VK_SELECT;
    
    if (nfvklabel->max_cc == length)
        nfvklabel->cursor_cc = length-1;
    else
        nfvklabel->cursor_cc = length;      
    nfvklabel->cursor_dsc = 0;
    nfvklabel->cursor_dec = 0;

    strcpy(nfvklabel->font_name, "Calibri 16");
    nfvklabel->pango_font = nfvklabel->font_name;   
    nfvklabel->fg_color_idx = COLOR_IDX(129);

    nfvklabel->use_num = 1;
    nfvklabel->use_cursor = 1;
    nfvklabel->invisible = 0;   
    
    nfvklabel->draw_outline = TRUE;

    nfvklabel->align = NFALIGN_LEFT;    
    nfvklabel->margin = VKLABEL_MARGIN;
 
    return nfvklabel;
}

gint nfui_nfvklabel_set_select_state(NFVKLABEL *nfvklabel, gint cc)
{
    if(nfvklabel->object.type != NFOBJECT_TYPE_NFVKLABEL) return -1;

    nfvklabel->cursor_state = VK_SELECT;
    nfvklabel->cursor_cc = cc;

    return 1;
}

gint nfui_nfvklabel_set_drag_state(NFVKLABEL *nfvklabel, gint dsc, gint dec)
{
    if(nfvklabel->object.type != NFOBJECT_TYPE_NFVKLABEL) return -1;

    nfvklabel->cursor_state = VK_DRAG;
    nfvklabel->cursor_dsc = dsc;
    nfvklabel->cursor_dec = dec;

    return 0;
}

gint nfui_nfvklabel_set_use_cursor(NFVKLABEL *nfvklabel, gint use)
{
    if(nfvklabel->object.type != NFOBJECT_TYPE_NFVKLABEL) return -1;

    nfvklabel->use_cursor = use;
    
    return 0;   
}

gint nfui_nfvklabel_modify_fg(NFVKLABEL *nfvklabel, guint fg_idx)
{
    if(nfvklabel->object.type != NFOBJECT_TYPE_NFVKLABEL) return -1;

    nfvklabel->fg_color_idx = fg_idx;

    return 0;   
}

gint nfui_nfvklabel_set_drawing_outline(NFVKLABEL *nfvklabel, gboolean draw_outline)
{
    if(nfvklabel->object.type != NFOBJECT_TYPE_NFVKLABEL) return -1;

    if(nfvklabel->draw_outline != draw_outline)
        nfvklabel->draw_outline = draw_outline;

    return 0;
}


gint nfui_nfvklabel_set_invisible(NFVKLABEL *nfvklabel, gint invisible)
{
    if(nfvklabel->object.type != NFOBJECT_TYPE_NFVKLABEL) return -1;

    nfvklabel->invisible = invisible;

    if(invisible)   nfui_nfobject_use_tooltip((NFOBJECT*)nfvklabel, FALSE);
    else            nfui_nfobject_use_tooltip((NFOBJECT*)nfvklabel, TRUE);

    return 0;   
}

gint nfui_nfvklabel_set_pango_font(NFVKLABEL *nfvklabel, gchar *pfont, guint fg_idx)
{
    if(nfvklabel->object.type != NFOBJECT_TYPE_NFVKLABEL) return -1;

    if(pfont)
    {
        if (nffont_is_system_font(pfont))
        {
            nfvklabel->pango_font = pfont;
        }
        else
        {
			_strncpy(nfvklabel->font_name, pfont, sizeof(nfvklabel->font_name));
            nfvklabel->pango_font = nfvklabel->font_name;        
        }
    }
        
    nfvklabel->fg_color_idx = fg_idx;

    return 0;   
}

gint nfui_nfvklabel_set_spacing(NFVKLABEL *nfvklabel, nfutil_pango_spacing_type spacing_type)
{
    if(nfvklabel->object.type != NFOBJECT_TYPE_NFVKLABEL) return -1;
    
    nfvklabel->spacing_type = spacing_type;

    return 0;   
}

gint nfui_nfvklabel_set_align(NFVKLABEL *nfvklabel, nfalign_type align, gint margin)
{
    if(nfvklabel->object.type != NFOBJECT_TYPE_NFVKLABEL) return -1;
    
    nfvklabel->align = align;
    nfvklabel->margin = margin;

    return 0;
}

gint nfui_nfvklabel_move(NFVKLABEL *nfvklabel, VK_MOVE_E move)
{
    gint str_len;
    gint cursor_dsc, cursor_dec;
    
    if(nfvklabel->object.type != NFOBJECT_TYPE_NFVKLABEL) return -1;

    _move_cusor_cc(nfvklabel, move);

    return 0;
}

gint nfui_nfvklabel_erase(NFVKLABEL *nfvklabel, VK_ERASE_E erase)
{
    gint str_len;
    gchar tmpLabel[NFVKLABEL_MAX_STRING_SIZE];
    gint cursor_dsc, cursor_dec;

    if(nfvklabel->object.type != NFOBJECT_TYPE_NFVKLABEL) return -1;

    memset(tmpLabel, 0x00, sizeof(tmpLabel));
	str_len = _strlen(nfvklabel->strLabel);

    if (nfvklabel->cursor_dsc > nfvklabel->cursor_dec)
    {
        cursor_dsc = nfvklabel->cursor_dec;
        cursor_dec = nfvklabel->cursor_dsc;
    }
    else
    {
        cursor_dsc = nfvklabel->cursor_dsc;
        cursor_dec = nfvklabel->cursor_dec;
    }   

    if (nfvklabel->cursor_state == VK_SELECT)
    {
        if (erase == VK_ERASE_BACKSPACE)
        {
            if (nfvklabel->cursor_cc == 0) return -1;

            _stop_cursor_timer(nfvklabel);

            if (nfvklabel->cursor_cc-1 != 0)
				_strncpy(tmpLabel, nfvklabel->strLabel, (nfvklabel->cursor_cc-1));

            if ((str_len-nfvklabel->cursor_cc) != 0)
				strcat(tmpLabel, g_utf8_offset_to_pointer(nfvklabel->strLabel, nfvklabel->cursor_cc));

            memset(nfvklabel->strLabel, 0x00, sizeof(nfvklabel->strLabel));
            strcpy(nfvklabel->strLabel, tmpLabel);
            
            nfvklabel->cursor_cc -= 1;
        }
        else    // VK_ERASE_DEL
        {
            if (nfvklabel->cursor_cc == str_len) return -1;

            _stop_cursor_timer(nfvklabel);

            if (nfvklabel->cursor_cc != 0)
				_strncpy(tmpLabel, nfvklabel->strLabel, nfvklabel->cursor_cc);
        
            if ((str_len-nfvklabel->cursor_cc-1) != 0)
				strcat(tmpLabel, g_utf8_offset_to_pointer(nfvklabel->strLabel, nfvklabel->cursor_cc+1));

            memset(nfvklabel->strLabel, 0x00, sizeof(nfvklabel->strLabel));
            strcpy(nfvklabel->strLabel, tmpLabel);
        }
    }
    else
    {
        if (!str_len) return -1;

        _erase_drag_section(nfvklabel);

        if (cursor_dsc != 0)
			_strncpy(tmpLabel, nfvklabel->strLabel, cursor_dsc);

        if ((str_len-cursor_dec-1) != 0)
			strcat(tmpLabel, g_utf8_offset_to_pointer(nfvklabel->strLabel, cursor_dec+1));

        memset(nfvklabel->strLabel, 0x00, sizeof(nfvklabel->strLabel));
        strcpy(nfvklabel->strLabel, tmpLabel);

        nfvklabel->cursor_state = VK_SELECT;
        nfvklabel->cursor_cc = cursor_dsc;
    }

    if (_lineup_vklabel(nfvklabel)) return 1;

    return 0;
}

gint nfui_nfvklabel_input_string(NFVKLABEL *nfvklabel, gchar *in_str)
{
	gint str_len;
	gchar tmpLabel[NFVKLABEL_MAX_STRING_SIZE];
	gint cursor_dsc, cursor_dec;

	if(nfvklabel->object.type != NFOBJECT_TYPE_NFVKLABEL) return -1;
	if(nfvklabel->use_num != 0) return -1;

	if (nfvklabel->cursor_dsc > nfvklabel->cursor_dec)
	{
		cursor_dsc = nfvklabel->cursor_dec;
		cursor_dec = nfvklabel->cursor_dsc;
	}
	else
	{
		cursor_dsc = nfvklabel->cursor_dsc;
		cursor_dec = nfvklabel->cursor_dec;
	}

	memset(tmpLabel, 0x00, sizeof(tmpLabel));
	str_len = _strlen(nfvklabel->strLabel);

	if (nfvklabel->cursor_state == VK_SELECT)
	{
		if (str_len == nfvklabel->max_cc) return -1;

		_stop_cursor_timer(nfvklabel);

		if (nfvklabel->cursor_cc != 0)
			_strncpy(tmpLabel, nfvklabel->strLabel, nfvklabel->cursor_cc);

		strcat(tmpLabel, in_str);

		if (str_len-nfvklabel->cursor_cc != 0)
			strcat(tmpLabel, g_utf8_offset_to_pointer(nfvklabel->strLabel, nfvklabel->cursor_cc));

		memset(nfvklabel->strLabel, 0x00, sizeof(nfvklabel->strLabel));
		//_strncpy(nfvklabel->strLabel, tmpLabel, (str_len+1));
		strcpy(nfvklabel->strLabel, tmpLabel);

		if (nfvklabel->cursor_cc+_strlen(in_str) != nfvklabel->max_cc)
			nfvklabel->cursor_cc += _strlen(in_str);
	}
	else
	{
		_erase_drag_section(nfvklabel);

		if (cursor_dsc != 0)
			_strncpy(tmpLabel, nfvklabel->strLabel, cursor_dsc);

		strcat(tmpLabel, in_str);

		if ((str_len-cursor_dec-1) != 0)
			strcat(tmpLabel, g_utf8_offset_to_pointer(nfvklabel->strLabel, cursor_dec+1));

		memset(nfvklabel->strLabel, 0x00, sizeof(nfvklabel->strLabel));
		strcpy(nfvklabel->strLabel, tmpLabel);

		nfvklabel->cursor_state = VK_SELECT;
		nfvklabel->cursor_cc = cursor_dsc;

		if (nfvklabel->cursor_cc+_strlen(in_str) != nfvklabel->max_cc)
			nfvklabel->cursor_cc += _strlen(in_str);
	}

	if (_lineup_vklabel(nfvklabel)) return 1;

	return 0;
}

gint nfui_nfvklabel_input_character(NFVKLABEL *nfvklabel, gchar character)
{   
    gint str_len;
	gchar tmpLabel[NFVKLABEL_MAX_STRING_SIZE], tmpBuf[8];
    gint cursor_dsc, cursor_dec;

    if(nfvklabel->object.type != NFOBJECT_TYPE_NFVKLABEL) return -1;
    if(nfvklabel->use_num != 0) return -1;

    if (nfvklabel->cursor_dsc > nfvklabel->cursor_dec)
    {
        cursor_dsc = nfvklabel->cursor_dec;
        cursor_dec = nfvklabel->cursor_dsc;
    }
    else
    {
        cursor_dsc = nfvklabel->cursor_dsc;
        cursor_dec = nfvklabel->cursor_dec;
    }   
    
    memset(tmpLabel, 0x00, sizeof(tmpLabel));
	memset(tmpBuf, 0x00, sizeof(tmpBuf));
	str_len = _strlen(nfvklabel->strLabel);

    if (nfvklabel->cursor_state == VK_SELECT)
    {
        if (str_len == nfvklabel->max_cc) return -1;

        _stop_cursor_timer(nfvklabel);

        if (nfvklabel->cursor_cc != 0)
			_strncpy(tmpLabel, nfvklabel->strLabel, nfvklabel->cursor_cc);

		g_sprintf(tmpBuf, "%c", character);
		strcat(tmpLabel, tmpBuf);

        if (str_len-nfvklabel->cursor_cc != 0)
			strcat(tmpLabel, g_utf8_offset_to_pointer(nfvklabel->strLabel, nfvklabel->cursor_cc));

        memset(nfvklabel->strLabel, 0x00, sizeof(nfvklabel->strLabel));     
		_strncpy(nfvklabel->strLabel, tmpLabel, (str_len+1));

        if (nfvklabel->cursor_cc+1 != nfvklabel->max_cc)
            nfvklabel->cursor_cc += 1;
    }
    else
    {
        _erase_drag_section(nfvklabel);

        if (cursor_dsc != 0)
			_strncpy(tmpLabel, nfvklabel->strLabel, cursor_dsc);

		g_sprintf(tmpBuf, "%c", character);
		strcat(tmpLabel, tmpBuf);

        if ((str_len-cursor_dec-1) != 0)
			strcat(tmpLabel, g_utf8_offset_to_pointer(nfvklabel->strLabel, cursor_dec+1));

        memset(nfvklabel->strLabel, 0x00, sizeof(nfvklabel->strLabel));
        strcpy(nfvklabel->strLabel, tmpLabel);

        nfvklabel->cursor_state = VK_SELECT;
        nfvklabel->cursor_cc = cursor_dsc;

        if (nfvklabel->cursor_cc+1 != nfvklabel->max_cc)
            nfvklabel->cursor_cc += 1;
    }

    if (_lineup_vklabel(nfvklabel)) return 1;

    return 0;
}

gint nfui_nfvklabel_input_num(NFVKLABEL *nfvklabel, gint num)
{   
    gint str_len;
	gchar tmpLabel[NFVKLABEL_MAX_STRING_SIZE], tmpBuf[8];
    gint cursor_dsc, cursor_dec;
    gchar character;
    gchar tmp[2];   

    if (nfvklabel->object.type != NFOBJECT_TYPE_NFVKLABEL) return -1;
    if (nfvklabel->use_num != 1) return -1;
    if ((num < 0) || (num > 9)) return -1;

    memset(tmp, 0x00, sizeof(tmp));
    g_sprintf(tmp, "%d", num);

    character = tmp[0];

    if (nfvklabel->cursor_dsc > nfvklabel->cursor_dec)
    {
        cursor_dsc = nfvklabel->cursor_dec;
        cursor_dec = nfvklabel->cursor_dsc;
    }
    else
    {
        cursor_dsc = nfvklabel->cursor_dsc;
        cursor_dec = nfvklabel->cursor_dec;
    }   
    
    memset(tmpLabel, 0x00, sizeof(tmpLabel));
	memset(tmpBuf, 0x00, sizeof(tmpBuf));
	str_len = _strlen(nfvklabel->strLabel);

    if (nfvklabel->cursor_state == VK_SELECT)
    {
        if (str_len == nfvklabel->max_cc) return -1;

        _stop_cursor_timer(nfvklabel);

        if (nfvklabel->cursor_cc != 0)
			_strncpy(tmpLabel, nfvklabel->strLabel, nfvklabel->cursor_cc);

		g_sprintf(tmpBuf, "%d", num);
		strcat(tmpLabel, tmpBuf);

        if (str_len-nfvklabel->cursor_cc != 0)
			strcat(tmpLabel, g_utf8_offset_to_pointer(nfvklabel->strLabel, nfvklabel->cursor_cc));

        memset(nfvklabel->strLabel, 0x00, sizeof(nfvklabel->strLabel));     
		_strncpy(nfvklabel->strLabel, tmpLabel, (str_len+1));

        if (nfvklabel->cursor_cc+1 != nfvklabel->max_cc)
            nfvklabel->cursor_cc += 1;
    }
    else
    {
        _erase_drag_section(nfvklabel);

        if (cursor_dsc != 0)
			_strncpy(tmpLabel, nfvklabel->strLabel, cursor_dsc);

		g_sprintf(tmpBuf, "%d", num);
		strcat(tmpLabel, tmpBuf);

        if ((str_len-cursor_dec-1) != 0)
			strcat(tmpLabel, g_utf8_offset_to_pointer(nfvklabel->strLabel, cursor_dec+1));

        memset(nfvklabel->strLabel, 0x00, sizeof(nfvklabel->strLabel));
        strcpy(nfvklabel->strLabel, tmpLabel);

        nfvklabel->cursor_state = VK_SELECT;
        nfvklabel->cursor_cc = cursor_dsc;

        if (nfvklabel->cursor_cc+1 != nfvklabel->max_cc)
            nfvklabel->cursor_cc += 1;
    }

    if (_lineup_vklabel(nfvklabel)) return 1;

    return 0;
}

gint nfui_nfvklabel_set_string(NFVKLABEL *nfvklabel, gchar *str)
{
    gint str_len;

    if(nfvklabel->object.type != NFOBJECT_TYPE_NFVKLABEL) return -1;
    if(nfvklabel->use_num != 0) return -1;
    if(!str) return -1;
    
    _stop_cursor_timer(nfvklabel);

    str_len = _strlen(str);

    memset(nfvklabel->strLabel, 0x00, sizeof(nfvklabel->strLabel)); 
	_strncpy(nfvklabel->strLabel, str, str_len);

    if (nfvklabel->max_cc == str_len)
        nfvklabel->cursor_cc = str_len-1;
    else
        nfvklabel->cursor_cc = str_len;     

    nfvklabel->cursor_state = VK_SELECT;

    if (_lineup_vklabel(nfvklabel)) return 1;

    return 0;
}

gint nfui_nfvklabel_set_num(NFVKLABEL *nfvklabel, gint num)
{
    gint str_len;

    if(nfvklabel->object.type != NFOBJECT_TYPE_NFVKLABEL) return -1;
    if(nfvklabel->use_num != 1) return -1;
    
    _stop_cursor_timer(nfvklabel);

    memset(nfvklabel->strLabel, 0x00, sizeof(nfvklabel->strLabel)); 
    g_sprintf(nfvklabel->strLabel, "%d", num);

    str_len = _strlen(nfvklabel->strLabel);
    
    if (nfvklabel->max_cc == str_len)
        nfvklabel->cursor_cc = str_len-1;
    else
        nfvklabel->cursor_cc = str_len;     

    nfvklabel->cursor_state = VK_SELECT;

    if (_lineup_vklabel(nfvklabel)) return 1;

    return 0;
}

gint nfui_nfvklabel_get_cursor_state(NFVKLABEL *nfvklabel)
{
    if(nfvklabel->object.type != NFOBJECT_TYPE_NFVKLABEL) return -1;

    return nfvklabel->cursor_state;
}

gint nfui_nfvklabel_get_cursor_cc(NFVKLABEL *nfvklabel)
{
    if(nfvklabel->object.type != NFOBJECT_TYPE_NFVKLABEL) return -1;

    return nfvklabel->cursor_cc;
}

gint nfui_nfvklabel_set_cursor_cc(NFVKLABEL *nfvklabel, gint cursor_cc)
{
    if(nfvklabel->object.type != NFOBJECT_TYPE_NFVKLABEL) return -1;

    nfvklabel->cursor_cc = cursor_cc;

    return 0;
}

gint nfui_nfvklabel_get_cursor_drag_cc(NFVKLABEL *nfvklabel, gint *dsc, gint *dec)
{
    if(nfvklabel->object.type != NFOBJECT_TYPE_NFVKLABEL) return -1;

    if (dsc) *dsc = nfvklabel->cursor_dsc;
    if (dec) *dec = nfvklabel->cursor_dec;

    return 0;
}

gchar* nfui_nfvklabel_get_all_str(NFVKLABEL *nfvklabel)
{
    if(nfvklabel->object.type != NFOBJECT_TYPE_NFVKLABEL) return -1;

    return nfvklabel->strLabel;
}

gint nfui_nfvklabel_get_num(NFVKLABEL *nfvklabel)
{
    if(nfvklabel->object.type != NFOBJECT_TYPE_NFVKLABEL) return -1;
    if(nfvklabel->use_num != 1) return -1;

    if (_strlen(nfvklabel->strLabel) == 0) return -1;

    return atoi(nfvklabel->strLabel);
}

