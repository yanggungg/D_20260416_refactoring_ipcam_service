

#include "nfipeditor.h"
#include "nffixed.h"
#include "nflabel.h"
#include "nfbutton.h"

#include "objects/nfbutton.h"

#include "../../support/color.h"

#include "../../support/event_loop.h"
#include "../../tools/nf_ui_tool.h"
#include "ix_mem.h"


#define	IPE_KEYPAD_WIDTH			(248)
#define	IPE_KEYPAD_HEIGHT			(312)

#define IPE_LINE_BORDER				(2)

static void prvDrawOutLines(NFOBJECT *obj)
{
	NFIPEDITOR *ipe;
	GdkDrawable *drawable = NULL;
	GdkGC *line_gc;
	guint pos_x, pos_y;
	gint line_gap;

	drawable = nfui_nfobject_get_window(obj);

	/* outline */
	line_gc = nfui_nfobject_get_gc(obj);
	gdk_gc_set_rgb_fg_color(line_gc, &UX_COLOR(136));
	gdk_gc_set_line_attributes(line_gc,
			IPE_LINE_BORDER,
			GDK_LINE_SOLID,
			GDK_CAP_NOT_LAST,
			GDK_JOIN_MITER);

	nfui_nfobject_get_offset(obj, &pos_x, &pos_y);
	line_gap = IPE_LINE_BORDER - 1;

	gdk_draw_rectangle(drawable,
					line_gc,
					FALSE,
					pos_x + line_gap,
					pos_y + line_gap,
					obj->width - (line_gap * 2),
					obj->height - (line_gap * 2));

	nfui_nfobject_gc_unref(line_gc);

}

static gboolean nfipeditor_event_handler(NFIPEDITOR *ipe, GdkEvent *event, gpointer data)
{
	GdkDrawable *drawable;
	GdkGC *gc;

	switch(event->type)
	{
		case GDK_EXPOSE :
		{
			guint left, top;
			gchar strIP[32];

			drawable = nfui_nfobject_get_window((NFOBJECT*)ipe);
			gc = nfui_nfobject_get_gc((NFOBJECT*)ipe);

			nfui_nfobject_get_offset((NFOBJECT*)ipe, &left, &top);

			g_sprintf(strIP, "%d.%d.%d.%d", ipe->ip[0], ipe->ip[1], ipe->ip[2], ipe->ip[3]);

			if(ipe->object.status == NFOBJECT_STATE_DISABLE)
			{
				nfutil_draw_text_with_pango(NULL, NULL, NULL, 
				                    drawable, gc, strIP, 
				                    left, top, ipe->object.width, ipe->object.height, 
							ipe->pango_font, &UX_COLOR(145), 
				                    NFALIGN_CENTER, 0);				

			}
			else
			{
				nfutil_draw_text_with_pango(NULL, NULL, NULL, 
				                    drawable, gc, strIP, 
				                    left, top, ipe->object.width, ipe->object.height, 
				                    ipe->pango_font, &UX_COLOR(ipe->fg_color), 
				                    NFALIGN_CENTER, 0);				
			}

			nfui_nfobject_gc_unref(gc);

			if(ipe->object.kfocus == NFOBJECT_FOCUS) 
				prvDrawOutLines((NFOBJECT*)ipe);
		}
		break;

		case GDK_ENTER_NOTIFY:
		case GDK_LEAVE_NOTIFY:
			if(nfui_nfobject_is_shown((NFOBJECT*)ipe))
				nfui_signal_emit((NFOBJECT*)ipe, GDK_EXPOSE, TRUE);
			break;

		case GDK_DELETE :
			break;

		default :
			break;
	}

	return FALSE;
}


NFIPEDITOR* nfui_nfipeditor_new()
{
	NFIPEDITOR *ipe;

	ipe = (NFIPEDITOR*)imalloc(sizeof(NFIPEDITOR));
	if(ipe == NULL)	return NULL;
	
#if 0
	ipe->object.parent = NULL;
	ipe->object.x = 0;
	ipe->object.y = 0;
	ipe->object.width = (guint)(DISPLAY_IS_D1 ? 50:100);
	ipe->object.height = (guint)(DISPLAY_IS_D1 ? 16:22);
	ipe->object.type = NFOBJECT_TYPE_NFIPEDITOR;
	ipe->object.show = NFOBJECT_HIDE;
	ipe->object.use_focus = NFOBJECT_FOCUS_ON;
	ipe->object.kfocus = NFOBJECT_UNFOCUS;
	ipe->object.mfocus = NFOBJECT_UNFOCUS;
	ipe->object.bg_color[NFOBJECT_STATE_NORMAL] = -1;
	ipe->object.bg_color[NFOBJECT_STATE_PRELIGHT] = -1;
	ipe->object.bg_color[NFOBJECT_STATE_ACTIVE] = -1;
	ipe->object.bg_color[NFOBJECT_STATE_DISABLE] = NFUI_DISABLED_COLOR;

	ipe->object.pre_event_handler = NULL;
	ipe->object.default_event_handler = nfipeditor_event_handler;
	ipe->object.post_event_handler = NULL;
#else
	nfui_nfobject_init((NFOBJECT*)ipe);

	ipe->object.width = (guint)(DISPLAY_IS_D1 ? 50:100);
	ipe->object.height = (guint)(DISPLAY_IS_D1 ? 16:22);
	ipe->object.type = NFOBJECT_TYPE_NFIPEDITOR;
	ipe->object.use_focus = NFOBJECT_FOCUS_ON;
	ipe->object.default_event_handler = nfipeditor_event_handler;

	ipe->object.use_tooltip = 1;

	strcpy(ipe->font_name, "Calibri 16");
	ipe->pango_font = ipe->font_name;
#endif

	ipe->use_cashing = 0;

	ipe->fg_color = COLOR_IDX(129);
	nfui_nfobject_set_tooltip(ipe, "0.0.0.0");

	return ipe;
}


NFIPEDITOR* nfui_nfipeditor_new_with_ip(guint ip1, guint ip2, guint ip3, guint ip4)
{
	NFIPEDITOR *ipe;
	gchar strTemp[32];

	ipe = nfui_nfipeditor_new();
	if(ipe == NULL)	return NULL;

	if(ip1<0)	ip1=0;
	if(ip2<0)	ip2=0;
	if(ip3<0)	ip3=0;
	if(ip4<0)	ip4=0;
	
	if(ip1>255)	ip1=255;
	if(ip2>255)	ip2=255;
	if(ip3>255)	ip3=255;
	if(ip4>255)	ip4=255;

	ipe->ip[0] = ip1;
	ipe->ip[1] = ip2;
	ipe->ip[2] = ip3;
	ipe->ip[3] = ip4;

	memset(strTemp, 0, sizeof(strTemp));
	g_sprintf(strTemp, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);
	nfui_nfobject_set_tooltip(ipe, strTemp);

	return ipe;
}

void nfui_nfipeditor_set_ip(NFIPEDITOR *ipe, guint ip1, guint ip2, guint ip3, guint ip4)
{
	gchar strTemp[32];

	if(ipe->object.type != NFOBJECT_TYPE_NFIPEDITOR)
	{
		return;
	}

	ipe->ip[0] = ip1;
	ipe->ip[1] = ip2;
	ipe->ip[2] = ip3;
	ipe->ip[3] = ip4;

	if(((NFOBJECT*)ipe)->use_tooltip)
	{
		memset(strTemp, 0, sizeof(strTemp));
		g_sprintf(strTemp, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);
		nfui_nfobject_set_tooltip(ipe, strTemp);
	}

	nfui_signal_emit(ipe, GDK_EXPOSE, TRUE);
}

void nfui_nfipeditor_set_ip_array(NFIPEDITOR *ipe, guint ip[4])
{
	gchar strTemp[32];

	if(ipe->object.type != NFOBJECT_TYPE_NFIPEDITOR)
	{
		return;
	}

	ipe->ip[0] = ip[0];
	ipe->ip[1] = ip[1];
	ipe->ip[2] = ip[2];
	ipe->ip[3] = ip[3];

	if(((NFOBJECT*)ipe)->use_tooltip)
	{
		memset(strTemp, 0, sizeof(strTemp));
		g_sprintf(strTemp, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
		nfui_nfobject_set_tooltip(ipe, strTemp);
	}

	nfui_signal_emit(ipe, GDK_EXPOSE, TRUE);
}

void nfui_nfipeditor_set_ip_array_no_expose(NFIPEDITOR *ipe, guint ip[4])
{
	gchar strTemp[32];

	if(ipe->object.type != NFOBJECT_TYPE_NFIPEDITOR)
	{
		return;
	}

	ipe->ip[0] = ip[0];
	ipe->ip[1] = ip[1];
	ipe->ip[2] = ip[2];
	ipe->ip[3] = ip[3];

	if(((NFOBJECT*)ipe)->use_tooltip)
	{
		memset(strTemp, 0, sizeof(strTemp));
		g_sprintf(strTemp, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
		nfui_nfobject_set_tooltip(ipe, strTemp);
	}
}

void nfui_nfipeditor_get_ip(NFIPEDITOR *ipe, guint *ip)
{
	if(ipe->object.type != NFOBJECT_TYPE_NFIPEDITOR)
	{
		return;
	}

	ip[0] = ipe->ip[0];
	ip[1] = ipe->ip[1];
	ip[2] = ipe->ip[2];
	ip[3] = ipe->ip[3];
}

void nfui_nfipeditor_get_ip_string(NFIPEDITOR *ipe, gchar *ip)
{
	if(ipe->object.type != NFOBJECT_TYPE_NFIPEDITOR)
	{
		return;
	}

	g_sprintf(ip, "%d.%d.%d.%d", ipe->ip[0], ipe->ip[1], ipe->ip[2], ipe->ip[3]);
}

void nfui_nfipeditor_set_pango_font(NFIPEDITOR *ipe, const gchar *pfont, guint fg_color)
{
	if(ipe->object.type != NFOBJECT_TYPE_NFIPEDITOR)
	{
		return;
	}

//	strncpy(ipe->pango_font, pfont, sizeof(ipe->pango_font));
	if (pfont) {
		if (nffont_is_system_font(pfont))
			ipe->pango_font = pfont;
		else {
			strncpy(ipe->font_name, pfont, sizeof(ipe->font_name));
			ipe->pango_font = ipe->font_name;
		}
	}
	ipe->fg_color = fg_color;
}

void nfui_nfipeditor_use_pango_cashing(NFIPEDITOR *ipe, gint cashing, gchar *key)
{
	if(ipe->object.type != NFOBJECT_TYPE_NFIPEDITOR)
	{
		return;
	}

	if(cashing)	ipe->use_cashing = 1;
	else		ipe->use_cashing = 0;

	if(key && strlen(key)>0)
		strncpy(ipe->cashing_key, key, sizeof(ipe->cashing_key));
	else
		memset(ipe->cashing_key, 0, sizeof(ipe->cashing_key));
}

