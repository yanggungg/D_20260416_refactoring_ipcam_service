


#include "../../support/util.h"
#include "../../support/color.h"

#include "nflabel.h"
#include "nfthumbnail.h"
#include "thumbnail_manager.h"
#include "ix_mem.h"

#include "../../support/event_loop.h"

/*
		-----------------------------
		|			 LABEL			|
		|---------------------------|
		|							|
		|							|
		|			IMAGE			|
		|							|
		|							|
		-----------------------------
*/





static void nfthumbnail_set_child_position(NFTHUMBNAIL *thumbnail)
{
	guint fixed_w, fixed_h;
	
	g_return_if_fail(thumbnail != NULL);

	fixed_w = thumbnail->th_fixed.object.width;	
	fixed_h = thumbnail->th_fixed.object.height;	

	nfui_nfobject_set_size(thumbnail->subject_label, fixed_w, thumbnail->subject_label_h);
	nfui_nffixed_put((NFFIXED*)thumbnail, thumbnail->subject_label, 0, THUMBNAIL_LINE_BORDER);

	nfui_nfobject_set_size(thumbnail->status_label, fixed_w - THUMBNAIL_LINE_BORDER*2, 
					fixed_h - thumbnail->subject_label_h  - THUMBNAIL_LINE_BORDER);
	nfui_nffixed_put((NFFIXED*)thumbnail, thumbnail->status_label, THUMBNAIL_LINE_BORDER, thumbnail->subject_label_h);	
}

static void nfthumbnail_change_child_position(NFTHUMBNAIL *thumbnail)
{
	guint fixed_w, fixed_h;
	
	g_return_if_fail(thumbnail != NULL);

	fixed_w = thumbnail->th_fixed.object.width;	
	fixed_h = thumbnail->th_fixed.object.height;	

	nfui_nfobject_set_size(thumbnail->subject_label, fixed_w, thumbnail->subject_label_h);
	((NFOBJECT *)thumbnail->subject_label)->x = 0;
	((NFOBJECT *)thumbnail->subject_label)->y = THUMBNAIL_LINE_BORDER;


	nfui_nfobject_set_size(thumbnail->status_label, fixed_w - THUMBNAIL_LINE_BORDER*2, 
					fixed_h - thumbnail->subject_label_h  - THUMBNAIL_LINE_BORDER);
	((NFOBJECT *)thumbnail->status_label)->x = THUMBNAIL_LINE_BORDER;
	((NFOBJECT *)thumbnail->status_label)->y = thumbnail->subject_label_h;
}

static void nfthumbnail_draw_bg(NFTHUMBNAIL *thumbnail, guint fg_idx, guint bg_idx)
{
	GdkDrawable *drawable = NULL;
	GdkPoint points[5];
	GdkGC *line_gc;

	guint pos_x, pos_y;

	gint obj_width, obj_height;

	g_return_if_fail(thumbnail != NULL);

	if(nfui_nfobject_is_disabled((NFOBJECT*)thumbnail))
		return;

// label
    nfui_nflabel_modify_fg((NFLABEL*)thumbnail->subject_label, fg_idx);
	nfui_nfobject_modify_bg(thumbnail->subject_label, NFOBJECT_STATE_NORMAL, bg_idx);
	nfui_signal_emit(thumbnail->subject_label, GDK_EXPOSE, FALSE);

	if (thumbnail_manager_get_covert(thumbnail))
    {   
		if (!nfui_nfobject_is_shown(thumbnail->status_label) || 
		    (strcmp(nfui_nflabel_get_text(thumbnail->status_label), "COVERT") &&
		     strcmp(nfui_nflabel_get_text(thumbnail->status_label), "NO VIDEO")))
		{
		    if (thumbnail_manager_get_covert_shown_as())
    		    nfui_nflabel_set_text((NFLABEL*)thumbnail->status_label, "COVERT");
		    else
		        nfui_nflabel_set_text((NFLABEL*)thumbnail->status_label, "NO VIDEO");
			nfui_nfobject_show(thumbnail->status_label);
			nfui_signal_emit(thumbnail->status_label, GDK_EXPOSE, FALSE);
		}
    }
	else if (thumbnail_manager_get_result(thumbnail))
	{
		if (nfui_nfobject_is_shown(thumbnail->status_label))
		{
			nfui_nfobject_hide(thumbnail->status_label);		
			nfui_signal_emit(thumbnail->status_label, GDK_EXPOSE, FALSE);
		}
		
		thumbnail_manager_own_image_load(thumbnail);
	}
	else
	{
		if (!nfui_nfobject_is_shown(thumbnail->status_label) || 
		    strcmp(nfui_nflabel_get_text(thumbnail->status_label), "NO IMAGE"))
		{
		    nfui_nflabel_set_text((NFLABEL*)thumbnail->status_label, "NO IMAGE");
			nfui_nfobject_show(thumbnail->status_label);
			nfui_signal_emit(thumbnail->status_label, GDK_EXPOSE, FALSE);
		}
	}

	drawable = nfui_nfobject_get_window((NFOBJECT*)thumbnail);
	line_gc = nfui_nfobject_get_gc((NFOBJECT*)thumbnail);

	nfui_nfobject_get_offset((NFOBJECT*)thumbnail, &pos_x, &pos_y);

	obj_width = thumbnail->th_fixed.object.width;
	obj_height = thumbnail->th_fixed.object.height;

// outline stroke
	gdk_gc_set_rgb_fg_color(line_gc, &UX_COLOR(bg_idx));

	gdk_gc_set_line_attributes(line_gc,
			THUMBNAIL_LINE_BORDER,
			GDK_LINE_SOLID,
			GDK_CAP_NOT_LAST,
			GDK_JOIN_MITER);


	points[0].x = pos_x;
	points[0].y = pos_y;
	points[1].x = pos_x + obj_width - THUMBNAIL_LINE_BORDER;
	points[1].y = pos_y;
	points[2].x = pos_x + obj_width - THUMBNAIL_LINE_BORDER;
	points[2].y = pos_y + obj_height - THUMBNAIL_LINE_BORDER;
	points[3].x = pos_x;
	points[3].y = pos_y + obj_height - THUMBNAIL_LINE_BORDER;
	points[4].x = pos_x;
	points[4].y = pos_y;

	gdk_draw_lines(drawable,
			line_gc,
			points,
			5);

	nfui_nfobject_gc_unref(line_gc);	
}

static gboolean nfthumbnail_event_handler(NFTHUMBNAIL *thumbnail, GdkEvent *event, gpointer data)
{
	guint bg_idx;

	switch(event->type)
	{
		case GDK_EXPOSE :
		{	
			// SKSHIN
			//if(!((NFFIXED*)thumbnail)->children)
			//	nfthumbnail_set_child_position(thumbnail);
			nfthumbnail_change_child_position(thumbnail);

			if (thumbnail->focus_draw)
			{
				if (thumbnail->th_fixed.object.kfocus == NFOBJECT_FOCUS) 
					nfthumbnail_draw_bg(thumbnail, THUMB_FOCUS_FG, THUMB_FOCUS_BG);
				else
				{
					if (thumbnail->state == THUMB_STATE_SELECT)
						nfthumbnail_draw_bg(thumbnail, THUMB_SELECT_FG, THUMB_SELECT_BG);
					else
						nfthumbnail_draw_bg(thumbnail, THUMB_NORMAL_FG, THUMB_NORMAL_BG);
				}
			}
			else
				nfthumbnail_draw_bg(thumbnail, THUMB_NORMAL_FG, THUMB_NORMAL_BG);
			
		}
		break;

		case GDK_ENTER_NOTIFY :
		{
			if ((nfui_nfobject_is_shown((NFOBJECT*)thumbnail)) && (thumbnail->focus_draw))
				nfthumbnail_draw_bg(thumbnail, THUMB_FOCUS_FG, THUMB_FOCUS_BG);
		}
		break;
		
		case GDK_LEAVE_NOTIFY :
		{
			if ((nfui_nfobject_is_shown((NFOBJECT*)thumbnail)) && (thumbnail->focus_draw))
			{
				if (thumbnail->th_fixed.object.kfocus == NFOBJECT_FOCUS) 
					nfthumbnail_draw_bg(thumbnail, THUMB_FOCUS_FG, THUMB_FOCUS_BG);
				else
				{
					if (thumbnail->state == THUMB_STATE_SELECT)
						nfthumbnail_draw_bg(thumbnail, THUMB_SELECT_FG, THUMB_SELECT_BG);
					else
						nfthumbnail_draw_bg(thumbnail, THUMB_NORMAL_FG, THUMB_NORMAL_BG);
				}	
			}
		}
		break;

		case GDK_DELETE :
		{
			thumbnail_manager_obj_disconnect(thumbnail);

			// SKSHIN
			/*
			if(!((NFFIXED*)thumbnail)->children) {
				if(thumbnail->subject_label) {
					nfui_nfobject_destroy(thumbnail->subject_label);
					thumbnail->subject_label = NULL;
				}
				if(thumbnail->status_label) {
					nfui_nfobject_destroy(thumbnail->status_label);
					thumbnail->status_label = NULL;
				}
			}
			*/
		}
		break;

		default :
		break;
	}

	return FALSE;
}

static gboolean nfthumbnail_label_cb(NFOBJECT *label, GdkEvent *event, gpointer data)
{
	NFTHUMBNAIL *thumbnail;
	NFOBJECT *item;

	thumbnail = NF_THUMBNAIL(label->parent);

	switch(event->type) {
		case GDK_BUTTON_PRESS:
		case GDK_BUTTON_RELEASE:
			{
				if(event->button.button == MOUSE_RIGTH_BUTTON) 					
					return FALSE;

				if(nfui_is_focus_at_child((NFOBJECT*)thumbnail)) {
					nfui_set_key_focus((NFOBJECT*)thumbnail, TRUE);					
					nfui_signal_emit((NFOBJECT*)thumbnail, GDK_EXPOSE, TRUE);
				}
			}
			break;

		default:
			break;
	}

	return FALSE;
}

NFTHUMBNAIL* nfui_nfthumbnail_new(gchar *strLabel, gchar *pfont)
{
	NFTHUMBNAIL *thumbnail;
	gint length;
	guint i;
	gint color[NFOBJECT_STATE_COUNT];	

	thumbnail = (NFTHUMBNAIL*)imalloc(sizeof(NFTHUMBNAIL));
	if(thumbnail == NULL)
		return NULL;

	nfui_nfobject_init((NFOBJECT*)thumbnail);

	thumbnail->th_fixed.object.width = 140;
	thumbnail->th_fixed.object.height = 22;
	thumbnail->th_fixed.object.type = NFOBJECT_TYPE_NFTHUMBNAIL;
	thumbnail->th_fixed.object.use_focus = NFOBJECT_FOCUS_ON;
	thumbnail->th_fixed.object.default_event_handler = nfthumbnail_event_handler;
	
	thumbnail->subject_label_h = 0;
	thumbnail->state = THUMB_STATE_NORMAL;
	thumbnail->focus_draw = TRUE;

	thumbnail->subject_label = (NFOBJECT*)nfui_nflabel_new(strLabel);
	nfui_nflabel_set_drawing_outline((NFLABEL*)(thumbnail->subject_label), FALSE);
	nfui_nflabel_set_align((NFLABEL*)thumbnail->subject_label, NFALIGN_CENTER, 0);
	nfui_nflabel_set_pango_font((NFLABEL*)thumbnail->subject_label, pfont, 651);
	nfui_nfobject_show(thumbnail->subject_label); 	

	thumbnail->status_label = (NFOBJECT*)nfui_nflabel_new("");
	nfui_nflabel_set_drawing_outline((NFLABEL*)(thumbnail->status_label), FALSE);
	nfui_nflabel_set_align((NFLABEL*)thumbnail->status_label, NFALIGN_CENTER, 0);
	nfui_nflabel_set_pango_font((NFLABEL*)thumbnail->status_label, nffont_get_pango_font(NFFONT_MINI_SEMI_4), COLOR_IDX(659));
	nfui_nfobject_modify_bg(thumbnail->status_label, NFOBJECT_STATE_NORMAL, COLOR_IDX(658));
	nfui_nfobject_show(thumbnail->status_label); 	

	// SKSHIN
	nfthumbnail_set_child_position(thumbnail);

	thumbnail_manager_object_connect(thumbnail);
	
	return thumbnail;
}

void nfui_nfthumbnail_use_focus_draw(NFTHUMBNAIL *thumbnail, gboolean use)
{
	g_return_if_fail(thumbnail != NULL); 
	
	if(thumbnail->th_fixed.object.type != NFOBJECT_TYPE_NFTHUMBNAIL)
	{
		return;
	}

	thumbnail->focus_draw = use;
}

////////////////////////////////////////////////////////////
//
//	public interfaces (from vw)
//

void nfui_nfthumbnail_subject_label_height(NFTHUMBNAIL *thumbnail, guint size_h)
{
	g_return_if_fail(thumbnail != NULL); 
	
	if(thumbnail->th_fixed.object.type != NFOBJECT_TYPE_NFTHUMBNAIL)
		return;

	thumbnail->subject_label_h = size_h;
}

THUMB_STATE_E nfui_nfthumbnail_get_state(NFTHUMBNAIL *thumbnail)
{
	g_return_if_fail(thumbnail != NULL); 
	
	if(thumbnail->th_fixed.object.type != NFOBJECT_TYPE_NFTHUMBNAIL)
		return;

	return thumbnail->state;
}

void nfui_nfthumbnail_set_state(NFTHUMBNAIL *thumbnail, THUMB_STATE_E state)
{
	g_return_if_fail(thumbnail != NULL); 
	
	if(thumbnail->th_fixed.object.type != NFOBJECT_TYPE_NFTHUMBNAIL)
		return;

	if (nfui_nfobject_is_shown((NFOBJECT*)thumbnail))
	{
		if (state == THUMB_STATE_SELECT)
			nfthumbnail_draw_bg(thumbnail, THUMB_SELECT_FG, THUMB_SELECT_BG);
		else
			nfthumbnail_draw_bg(thumbnail, THUMB_NORMAL_FG, THUMB_NORMAL_BG);
	}

	thumbnail->state = state;
}

////////////////////////////////////////////////////////////
//
//	thumbnail manager setting
//

void nfui_nfthumbnail_image_open()
{
	thumbnail_manager_set_inst(TH_OPEN);
}

void nfui_nfthumbnail_image_close()
{
	thumbnail_manager_set_inst(TH_CLOSE);
}

void nfui_nfthumbnail_get_image()
{
	thumbnail_manager_set_state(TH_CHANGE);
}

void nfui_nfthumbnail_set_image_buf(NFTHUMBNAIL *thumbnail, guchar *buf)
{
	g_return_if_fail(thumbnail != NULL); 

	if(thumbnail->th_fixed.object.type != NFOBJECT_TYPE_NFTHUMBNAIL)
	{
		return;
	}

	thumbnail_manager_attach_image_buf(thumbnail, buf);
}

void nfui_nfthumbnail_set_channel(NFTHUMBNAIL *thumbnail, gint ch)
{
	g_return_if_fail(thumbnail != NULL); 

	if(thumbnail->th_fixed.object.type != NFOBJECT_TYPE_NFTHUMBNAIL)
	{
		return;
	}

	thumbnail_manager_attach_ch(thumbnail, ch);
}

void nfui_nfthumbnail_set_period(NFTHUMBNAIL *thumbnail, time_t start_time, time_t end_time)
{
	g_return_if_fail(thumbnail != NULL); 

	if(thumbnail->th_fixed.object.type != NFOBJECT_TYPE_NFTHUMBNAIL)
	{
		return;
	}

	thumbnail_manager_attach_period(thumbnail, start_time, end_time);
}

void nfui_nfthumbnail_get_period(NFTHUMBNAIL *thumbnail, time_t *start_time, time_t *end_time)
{
	g_return_if_fail(thumbnail != NULL); 

	if(thumbnail->th_fixed.object.type != NFOBJECT_TYPE_NFTHUMBNAIL)
	{
		return;
	}

	thumbnail_manager_get_period(thumbnail, start_time, end_time);
}

void nfui_nfthumbnail_set_image_size(NFTHUMBNAIL *thumbnail, gint width, gint height)
{
	guint obj_x, obj_y;
	guint tmp_y, tmp_h;
	guint img_x, img_y;

	g_return_if_fail(thumbnail != NULL); 

	if(thumbnail->th_fixed.object.type != NFOBJECT_TYPE_NFTHUMBNAIL)
	{
		return;
	}

	thumbnail->image_width = width;
	thumbnail->image_height = height;
}

void nfui_nfthumbnail_set_subject_label_text(NFTHUMBNAIL *thumbnail, gchar *strLabel)
{
	gint length;

	g_return_if_fail(thumbnail != NULL); 

	if(thumbnail->th_fixed.object.type != NFOBJECT_TYPE_NFTHUMBNAIL)
	{
		return;
	}
	
	memset(thumbnail->strLabel, 0, sizeof(thumbnail->strLabel));
	
	length = strlen(strLabel);

	if(length > 0)
	{
		strncpy(thumbnail->strLabel, strLabel, sizeof(gchar)*length);
		nfui_nflabel_set_text(thumbnail->subject_label, strLabel);
	}
}

gboolean nfui_nfthumbnail_get_image_time(NFTHUMBNAIL *thumbnail, time_t *image_time)
{
	return thumbnail_manager_get_time(thumbnail, image_time);
}

// 0 : fail - no image
// 1 : success.
// 2 : thumbnail loading
gint nfui_nfthumbnail_get_focused_image_time(NFTHUMBNAIL *thumbnail, time_t *image_time)
{
	return thumbnail_manager_get_focused_time(thumbnail, image_time);
}

gboolean nfui_nfthumbnail_get_focused_start_time(NFTHUMBNAIL *thumbnail, time_t *image_time)
{
	return thumbnail_manager_get_start_time(thumbnail, image_time);
}

gboolean nfui_nfthumbnail_check_running(void)
{
	return thumbnail_manager_is_running();
}

int nfui_nfthumbnail_wait_for_stop()
{
	thumbnail_manager_set_inst(TH_CLOSE);

	while (1) {
		usleep(100000);
		if (!thumbnail_manager_is_running()) break;
	}
	return 0;
}
