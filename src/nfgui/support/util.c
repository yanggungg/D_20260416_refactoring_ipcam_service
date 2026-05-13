
/**********************************************************************************
 *
 *	COMMON FUNCTION
 *	
 * *******************************************************************************/

#include "nf_afx.h"
#include "util.h"
#include "nf_ui_tool.h"

#include "color.h"
#include "multi_language_support.h"


#define	AVG_CHAR_WIDTH	(8)

gboolean nfutil_check_email(const gchar *strName)
{

	return FALSE;
}

void nfutil_nfpoint_set(nfpoint* ptPoint, guint x, guint y)
{
	ptPoint->x = x;
	ptPoint->y = y;
}

void nfutil_nfrect_set(nfrect* rtRect, guint x, guint y, guint width, guint height)
{
	rtRect->x = x;
	rtRect->y = y;
	rtRect->width = width;
	rtRect->height = height;
}


void nfutil_draw_pixbuf(GdkDrawable *drawable, GdkGC *gc, GdkPixbuf *pbuf, gint x, gint y, gint w, gint h, nfalign_type halign, guint margin)
{
	gint pbuf_w, pbuf_h;
	gint dest_x, dest_y;
	gint draw_w, draw_h;

	if (!pbuf) return;
	nfui_get_pixbuf_size(pbuf, &pbuf_w, &pbuf_h);

	if(w == -1)	w = pbuf_w;
	if(h == -1)	h = pbuf_h;

	if(pbuf_w < w)	draw_w = pbuf_w;
	else			draw_w = w;

	if(halign == NFALIGN_LEFT)					dest_x = x + margin;
	else if(halign == NFALIGN_CENTER)			dest_x = x + (w-pbuf_w)/2;
	else if(halign == NFALIGN_RIGHT)			dest_x = x + w - pbuf_w - margin;
	else if(halign == NFALIGN_CENTER_LEFT)		dest_x = x + w/2 - pbuf_w - margin;
	else if(halign == NFALIGN_CENTER_RIGHT)		dest_x = x + w/2 + margin;
	else										dest_x = x + margin;
	

#if 1
	if(pbuf_h < h)
	{
		draw_h = pbuf_h;
		//jbyim
		dest_y = y + (h-pbuf_h)/2;
		//dest_y = y + (draw_h-pbuf_h)/2;
	}
	else
	{
		draw_h = h;
		dest_y = y;
	}
#else
	if(pbuf_h < h)
	{
		draw_h = pbuf_h;
		dest_y = y + (draw_h-pbuf_h)/2;
	}
	else
	{
		draw_h = h;
		dest_y = y;
	}
#endif	

  	gdk_draw_pixbuf(drawable, gc, pbuf, 0, 0, dest_x, dest_y, draw_w, draw_h, GDK_RGB_DITHER_NONE, 0, 0);
}

void nfutil_draw_image_no_cache(GdkDrawable *drawable, GdkGC *gc, const gchar *path, gint x, gint y, gint w, gint h, nfalign_type halign, guint margin)
{
	GdkPixbuf *pbuf = NULL;
	gint pbuf_w, pbuf_h;
	gint dest_x, dest_y;
	gint draw_w, draw_h;

	pbuf = nfui_get_pixbuf_from_file(path, NULL);
	if(!pbuf)	return;
	nfui_get_pixbuf_size(pbuf, &pbuf_w, &pbuf_h);
	//gdk_drawable_get_size(pbuf, &pbuf_w, &pbuf_h);
	
	if(w == -1)	w = pbuf_w;
	if(h == -1)	h = pbuf_h;

	if(pbuf_w < w)	draw_w = pbuf_w;
	else			draw_w = w;

	if(halign == NFALIGN_LEFT)					dest_x = x + margin;
	else if(halign == NFALIGN_CENTER)			dest_x = x + (w-pbuf_w)/2;
	else if(halign == NFALIGN_RIGHT)			dest_x = x + w - pbuf_w - margin;
	else if(halign == NFALIGN_CENTER_LEFT)		dest_x = x + w/2 - pbuf_w - margin;
	else if(halign == NFALIGN_CENTER_RIGHT)		dest_x = x + w/2 + margin;
	else										dest_x = x + margin;
	

	if(pbuf_h < h)
	{
		draw_h = pbuf_h;
		dest_y = y + (draw_h-pbuf_h)/2;
	}
	else
	{
		draw_h = h;
		dest_y = y;
	}


 	gdk_draw_pixbuf(drawable, gc, pbuf, 0, 0, dest_x, dest_y, draw_w, draw_h, GDK_RGB_DITHER_NONE, 0, 0);
	g_object_unref(pbuf);
}

void nfutil_draw_image(GdkDrawable *drawable, GdkGC *gc, const gchar *path, gint x, gint y, gint w, gint h, nfalign_type halign, guint margin)
{
	GdkPixbuf *pbuf = NULL;
	gint pbuf_w, pbuf_h;
	gint dest_x, dest_y;
	gint draw_w, draw_h;

	pbuf = nfui_get_image_from_file(path, NULL);
	if(!pbuf)	return;
	nfui_get_pixbuf_size(pbuf, &pbuf_w, &pbuf_h);
	//gdk_drawable_get_size(pbuf, &pbuf_w, &pbuf_h);
	
	
// special code	
#if 0
if (strncmp(path, "MK_", 3) == 0) {
	printf("reduced image = %d, %d\n", pbuf_w, pbuf_h);
	w = pbuf_w * 2;
	h = pbuf_h * 2;

	pbuf_w = w;
	pbuf_h = h;
}
else {
	
	if(w == -1)	w = pbuf_w;
	if(h == -1)	h = pbuf_h;

}
#else
	if(w == -1)	w = pbuf_w;
	if(h == -1)	h = pbuf_h;
#endif

	if(pbuf_w < w)	draw_w = pbuf_w;
	else			draw_w = w;

	if(halign == NFALIGN_LEFT)					dest_x = x + margin;
	else if(halign == NFALIGN_CENTER)			dest_x = x + (w-pbuf_w)/2;
	else if(halign == NFALIGN_RIGHT)			dest_x = x + w - pbuf_w - margin;
	else if(halign == NFALIGN_CENTER_LEFT)		dest_x = x + w/2 - pbuf_w - margin;
	else if(halign == NFALIGN_CENTER_RIGHT)		dest_x = x + w/2 + margin;
	else										dest_x = x + margin;
	

	if(pbuf_h < h)
	{
		draw_h = pbuf_h;
		dest_y = y + (draw_h-pbuf_h)/2;
	}
	else
	{
		draw_h = h;
		dest_y = y;
	}


// special code for reducing the mem size
#if 0
{
	GdkPixbuf *tmp;
	if (strncmp(path, "MK_", 3) == 0) {
		tmp = gdk_pixbuf_scale_simple(pbuf, w, h, GDK_RGB_DITHER_NORMAL);
		
 		gdk_draw_pixbuf(drawable, gc, tmp, 0, 0, dest_x, dest_y, draw_w, draw_h, GDK_RGB_DITHER_NORMAL, 0, 0);
		g_object_unref(tmp);

	}
	else {
	 	gdk_draw_pixbuf(drawable, gc, pbuf, 0, 0, dest_x, dest_y, draw_w, draw_h, GDK_RGB_DITHER_NORMAL, 0, 0);
	}
}
#else
 	gdk_draw_pixbuf(drawable, gc, pbuf, 0, 0, dest_x, dest_y, draw_w, draw_h, GDK_RGB_DITHER_NONE, 0, 0);
#endif

}

void nfutil_draw_image2(GdkDrawable *drawable, GdkGC *gc, const gchar *path, gint src_x, gint src_y, gint dst_x, gint dst_y, gint width, gint height)
{
	GdkPixbuf *pbuf = NULL;

	pbuf = nfui_get_image_from_file(path, NULL);
	if(!pbuf)	return;

 	gdk_draw_pixbuf(drawable, gc, pbuf, src_x, src_y, dst_x, dst_y, width, height, GDK_RGB_DITHER_NONE, 0, 0);
}

struct LAYOUTSIZE {
	gint width;
	gint height;
};

static GHashTable* lsize_hash_table = NULL;
static GHashTable* pango_hash_table = NULL;

static void prvDrawTextWithPango(gint conv, const gchar *key, GdkPixbuf *bgimg, GdkColor *bgcol, GdkDrawable *drawable, GdkGC* gc,  const gchar* str, gint valid_cnt, gint x, gint y, gint w, gint h, const gchar *font, GdkColor *font_color, GdkColor *out_color, nfalign_type halign, guint margin, gint shadow, nfutil_pango_spacing_type spacing_type)
{
	GdkScreen *screen;
	PangoRenderer *renderer;
	PangoMatrix matrix = PANGO_MATRIX_INIT;
	PangoContext *context;
	PangoLayout *layout;
	PangoFontDescription *desc;

	PangoAttrList *attrs = NULL;
	PangoAttribute *attr = NULL;

	gchar strHashKey[NF_PANGO_CASHING_KEY_MAX];
	gchar lsizeKey[NF_PANGO_CASHING_KEY_MAX];

	guint lw, lh;
	gint left, top;
	guint pw, ph;

	gint nx, ny;

	gchar* p = NULL;
	gchar *utf_str = NULL;
	gchar* strip_str = NULL;
	gchar strTemp[2048];

	GdkDrawable *pDrawable = NULL;
	GdkPixmap *pango_pbuf = NULL;
	GdkGC *pGc = NULL;
	struct LAYOUTSIZE *lsize = NULL;

	if(str == NULL || strlen(str) == 0)
		return;

	if(key)
	{
		
//		if(pango_hash_table == NULL)
//		{
//			pango_hash_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
//		}
		
//		memset(strHashKey, 0, sizeof(strHashKey));
//		g_sprintf(strHashKey, "%s_%s_%d_%d%d_%s_#%x%x%x_%d_%d_%d", key, str, valid_cnt, w, h, font, font_color->red, font_color->green, font_color->blue, halign, margin, shadow);

//		pango_pbuf = (GdkPixmap *)g_hash_table_lookup(pango_hash_table, strHashKey);

//		if(pango_pbuf)
//		{
//			g_assert(0);
//			gdk_draw_drawable(drawable, gc, pango_pbuf, 0, 0, x, y, w, h);
//			return;
//		}

//		pango_pbuf = gdk_pixmap_new(drawable, w, h, -1);
//		pGc = gdk_gc_new(pango_pbuf);
		pGc = gdk_gc_new(drawable);

//		pDrawable = (GdkDrawable*)pango_pbuf;
		pDrawable = (GdkDrawable*)drawable;
//		nx = 0;
//		ny = 0;

nx = x;
ny = y;
	}
	else
	{
		pDrawable = drawable;
		pGc = gdk_gc_new(drawable);
		nx = x;
		ny = y;
	}

	if(bgimg)
	{
		gdk_draw_pixbuf(pDrawable, pGc, bgimg, 0, 0, nx, ny, w, h, GDK_RGB_DITHER_NONE, 0, 0);
	}
	else {
/*		if(bgcol)
		{
			gdk_gc_set_rgb_fg_color(pGc, bgcol);
			gdk_draw_rectangle(pDrawable, pGc, TRUE, nx, ny, w, h);
		}*/
	}

	if(conv)
	{
		utf_str = lookup_string(str);
	}

	if(utf_str == NULL)
		utf_str = str;

	if(valid_cnt <= 0)
	{
		strip_str = utf_str;
	}
	else
	{
		gint k=0;

		memset(strTemp, 0, sizeof(strTemp));
		g_utf8_strncpy(strTemp, utf_str, valid_cnt);

		while(1)
		{
			if(strTemp[k] == NULL)
				break;
			k++;
		}

		strTemp[k++] = '.';
		strTemp[k++] = '.';
		strTemp[k++] = '.';
		
		strip_str = strTemp;
	}

	screen = gdk_drawable_get_screen(drawable);
	renderer = gdk_pango_renderer_get_default(screen);

	gdk_pango_renderer_set_drawable(GDK_PANGO_RENDERER(renderer), pDrawable);
	gdk_pango_renderer_set_gc(GDK_PANGO_RENDERER(renderer), pGc);

	context = gdk_pango_context_get_for_screen(screen);
gdk_pango_context_set_colormap(context, gdk_rgb_get_colormap());

	layout = pango_layout_new(context);

	if(nftool_cur_language_is_japanese())
		pango_layout_set_spacing(layout, (-1)*PANGO_SCALE);
	//else
	//	pango_layout_set_spacing(layout, (-4)*PANGO_SCALE);
	
	desc = pango_font_description_from_string(font);
	pango_layout_set_font_description(layout, desc);

	if(spacing_type != NORMAL_SPACING)
	{
		attrs = pango_layout_get_attributes(layout); 
		if(attrs == NULL)
		{
			attrs = pango_attr_list_new();
			pango_layout_set_attributes(layout, attrs);
		//	pango_attr_list_unref(attrs);
		}

		attr = pango_attr_letter_spacing_new(nfutil_pango_get_spacing(spacing_type));
		attr->start_index = 0;
//		attr->end_index = g_utf8_strlen(strip_str, -1)*sizeof(gchar);
		attr->end_index = strlen(strip_str)*sizeof(gchar);

//		pango_attr_list_change(attrs, attr);
		pango_attr_list_insert(attrs, attr);
	}

	///////////////////////////////////////////////////////////////////////////////////////
	if(bgcol) {
		attrs = pango_layout_get_attributes(layout); 
		if(attrs == NULL)
		{
			attrs = pango_attr_list_new();
			pango_layout_set_attributes(layout, attrs);
		//	pango_attr_list_unref(attrs);
		}

		if(attrs) {
			attr = pango_attr_background_new(bgcol->red, bgcol->green, bgcol->blue);
			//pango_attr_list_change(attrs, attr);
			pango_attr_list_insert(attrs, attr);
			//pango_attribute_destroy(attr);
		}
	}
	///////////////////////////////////////////////////////////////////////////////////////
	

	pango_layout_set_text(layout, strip_str, -1);

	pango_matrix_translate(&matrix, (float)(nx+w/2), (float)(ny+h/2));
	pango_context_set_matrix(context, &matrix);

	pango_layout_context_changed(layout);
	pango_layout_get_size(layout, &lw, &lh);
	pango_layout_get_pixel_size(layout, &pw, &ph);

	top = -(lh/2);

	if(halign == NFALIGN_LEFT)				left = (-(w/2) + margin) * PANGO_SCALE;
	else if(halign == NFALIGN_CENTER)		left = -(lw/2);
	else if(halign == NFALIGN_RIGHT)		left = -lw + (((w/2-margin)) * PANGO_SCALE);
	else if(halign == NFALIGN_CENTER_LEFT)	left = (nx+w/2-margin)*PANGO_SCALE - lw;
	else if(halign == NFALIGN_CENTER_RIGHT)	left = margin*PANGO_SCALE;
	else if(halign == NFALIGN_CENTER_DOWN){	left = -(lw/2); top = -lh + (((h/2-margin)) * PANGO_SCALE); } //wiggls - otmii
	else if(halign == NFALIGN_LEFT_UP){	left = (-(w/2) + margin) * PANGO_SCALE; top = (-(h/2) * PANGO_SCALE); }
	else 									left = -(lw/2);

	if(shadow)
	{
		gint temp;

		temp = PANGO_SCALE;

		if(out_color)
			gdk_pango_renderer_set_override_color(GDK_PANGO_RENDERER(renderer), PANGO_RENDER_PART_FOREGROUND, out_color);
		else
			gdk_pango_renderer_set_override_color(GDK_PANGO_RENDERER(renderer), PANGO_RENDER_PART_FOREGROUND, &UX_COLOR(308));
		pango_renderer_draw_layout(renderer, layout, left-temp, top-temp);
		pango_renderer_draw_layout(renderer, layout, left+temp, top-temp);
		pango_renderer_draw_layout(renderer, layout, left+temp, top+temp);
		pango_renderer_draw_layout(renderer, layout, left-temp, top+temp);
		pango_renderer_draw_layout(renderer, layout, left-temp, top);
		pango_renderer_draw_layout(renderer, layout, left+temp, top);
		pango_renderer_draw_layout(renderer, layout, left, top-temp);
		pango_renderer_draw_layout(renderer, layout, left, top+temp);
	}

	gdk_pango_renderer_set_override_color(GDK_PANGO_RENDERER(renderer), PANGO_RENDER_PART_FOREGROUND, font_color);

	pango_renderer_draw_layout(renderer, layout, left, top);

	pango_font_description_free(desc);	// just moved here  
	gdk_pango_renderer_set_override_color(GDK_PANGO_RENDERER(renderer), PANGO_RENDER_PART_FOREGROUND, NULL);
	gdk_pango_renderer_set_drawable(GDK_PANGO_RENDERER(renderer), NULL);
	gdk_pango_renderer_set_gc(GDK_PANGO_RENDERER(renderer), NULL);

	if(attrs)
		pango_attr_list_unref(attrs);

	g_object_unref(layout);
	g_object_unref(context);

	g_object_unref(pGc);
	pGc = NULL;

// skshin	
#if 0
	if(pango_pbuf)
	{
	   	g_hash_table_insert(pango_hash_table, g_strdup(strHashKey), pango_pbuf);
		gdk_draw_drawable(drawable, gc, pango_pbuf, 0, 0, x, y, w, h);

//#define	__MEMORY_PROBE__
#ifdef	__MEMORY_PROBE__
		static double pango_use_mem = 0.0;
		int pango_added_mem = 0;

		pango_added_mem = (w * h * 2);
		pango_use_mem += (pango_added_mem / 1024.0);

		DMSG(1, "util.c:%d [PANGO NEW CASHING...] 0x%x [%s]\tMEMORY[%d bytes / %.3fKB]\n", __LINE__, pango_pbuf, strHashKey, pango_added_mem, pango_use_mem);
#endif

		/* FOR LAYOUT SIZE */
		if(lsize_hash_table == NULL)
			lsize_hash_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

		memset(lsizeKey, 0, sizeof(lsizeKey));
		g_sprintf(lsizeKey, "%s_%s_%d", str, font, spacing_type);

		lsize = (struct LAYOUTSIZE *)g_hash_table_lookup(lsize_hash_table, lsizeKey);

		if(lsize == NULL)
		{
			lsize = g_malloc0(sizeof(struct LAYOUTSIZE));

			lsize->width = pw;
			lsize->height = ph;

			g_hash_table_insert(lsize_hash_table, g_strdup(lsizeKey), lsize);
		}
	}
#endif
}

static void prvDrawTextWithPangoVk(gint conv, const gchar *key, GdkPixbuf *bgimg, GdkColor *bgcol, GdkDrawable *drawable, GdkGC* gc,  const gchar* str, gint valid_cnt, gint x, gint y, gint w, gint h, const gchar *font, GdkColor *font_color, GdkColor *out_color, nfalign_type halign, guint margin, gint shadow, nfutil_pango_spacing_type spacing_type)
{
	GdkScreen *screen;
	PangoRenderer *renderer;
	PangoMatrix matrix = PANGO_MATRIX_INIT;
	PangoContext *context;
	PangoLayout *layout;
	PangoFontDescription *desc;

	PangoAttrList *attrs = NULL;
	PangoAttribute *attr = NULL;

	gchar strHashKey[NF_PANGO_CASHING_KEY_MAX];
	gchar lsizeKey[NF_PANGO_CASHING_KEY_MAX];

	guint lw, lh;
	gint left, top;
	guint pw, ph;

	gint nx, ny;

	gchar* p = NULL;
	gchar *utf_str = NULL;
	gchar* strip_str = NULL;
	gchar strTemp[2048];

	GdkDrawable *pDrawable = NULL;
	GdkPixmap *pango_pbuf = NULL;
	GdkGC *pGc = NULL;
	struct LAYOUTSIZE *lsize = NULL;

	if(str == NULL || strlen(str) == 0)
		return;

	if(key)
	{
		
		if(pango_hash_table == NULL)
		{
			pango_hash_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
		}
		
		memset(strHashKey, 0, sizeof(strHashKey));
		g_sprintf(strHashKey, "%s_%s_%d_%d%d_%s_#%x%x%x_%d_%d_%d", key, str, valid_cnt, w, h, font, font_color->red, font_color->green, font_color->blue, halign, margin, shadow);

		pango_pbuf = (GdkPixmap *)g_hash_table_lookup(pango_hash_table, strHashKey);

		if(pango_pbuf)
		{
			g_assert(0);
			return;
		}

		pGc = gdk_gc_new(drawable);

		pDrawable = (GdkDrawable*)drawable;

		nx = x;
		ny = y;
		
	}
	else
	{
		pDrawable = drawable;
		pGc = gdk_gc_new(drawable);
		nx = x;
		ny = y;
	}

	if(bgimg)
	{
		gdk_draw_pixbuf(pDrawable, pGc, bgimg, 0, 0, nx, ny, w, h, GDK_RGB_DITHER_NONE, 0, 0);
	}
	else {
/*		if(bgcol)
		{
			gdk_gc_set_rgb_fg_color(pGc, bgcol);
			gdk_draw_rectangle(pDrawable, pGc, TRUE, nx, ny, w, h);
		}*/
	}

	if(conv)
	{
		utf_str = lookup_string(str);
	}

	if(utf_str == NULL)
		utf_str = str;

	if(valid_cnt <= 0)
	{
		strip_str = utf_str;
	}
	else
	{
		gint size = strlen(utf_str);
		gint utf_str_output_pos = strlen(utf_str) - valid_cnt;
		gint i;

		memset(strTemp, 0, sizeof(strTemp));

		for( i = 0 ; i < size ; i++)
		{
			if(i < 3)
				strTemp[i] = '.';
			else
				strTemp[i] = utf_str[utf_str_output_pos++];
		}
		
		strip_str = strTemp;
	}

	screen = gdk_drawable_get_screen(drawable);
	renderer = gdk_pango_renderer_get_default(screen);

	gdk_pango_renderer_set_drawable(GDK_PANGO_RENDERER(renderer), pDrawable);
	gdk_pango_renderer_set_gc(GDK_PANGO_RENDERER(renderer), pGc);

	context = gdk_pango_context_get_for_screen(screen);
	gdk_pango_context_set_colormap(context, gdk_rgb_get_colormap());

	layout = pango_layout_new(context);

	if(nftool_cur_language_is_japanese())
		pango_layout_set_spacing(layout, (-1)*PANGO_SCALE);
	//else
	//	pango_layout_set_spacing(layout, (-4)*PANGO_SCALE);
	
	desc = pango_font_description_from_string(font);

	pango_layout_set_font_description(layout, desc);

	if(spacing_type != NORMAL_SPACING)
	{
		attrs = pango_layout_get_attributes(layout); 
		if(attrs == NULL)
		{
			attrs = pango_attr_list_new();
			pango_layout_set_attributes(layout, attrs);
			pango_attr_list_unref(attrs);
		}

		attr = pango_attr_letter_spacing_new(nfutil_pango_get_spacing(spacing_type));
		attr->start_index = 0;
//		attr->end_index = g_utf8_strlen(strip_str, -1)*sizeof(gchar);
		attr->end_index = strlen(strip_str)*sizeof(gchar);

		pango_attr_list_change(attrs, attr);
	}

	pango_layout_set_text(layout, strip_str, -1);

	pango_matrix_translate(&matrix, (float)(nx+w/2), (float)(ny+h/2));
	pango_context_set_matrix(context, &matrix);

	pango_layout_context_changed(layout);
	pango_layout_get_size(layout, &lw, &lh);
	pango_layout_get_pixel_size(layout, &pw, &ph);

	top = -(lh/2);

	if(halign == NFALIGN_LEFT)
	{
		left = (-(w/2) + margin) * PANGO_SCALE;
	}
	else if(halign == NFALIGN_CENTER)
	{
		left = -(lw/2);
	}
	else if(halign == NFALIGN_RIGHT)
	{
		left = (-(w/2) + margin) * PANGO_SCALE;
	}
	else if(halign == NFALIGN_CENTER_LEFT)	
	{
		left = (nx+w/2-margin)*PANGO_SCALE - lw;
	}
	else if(halign == NFALIGN_CENTER_RIGHT)	
	{
		left = margin*PANGO_SCALE;
	}
	else if(halign == NFALIGN_CENTER_DOWN)
	{
		left = -(lw/2); top = -lh + (((h/2-margin)) * PANGO_SCALE); 
	} //wiggls - otmii
	else if(halign == NFALIGN_LEFT_UP)
	{
		left = (-(w/2) + margin) * PANGO_SCALE; top = (-(h/2) * PANGO_SCALE); 
	}
	else 
	{
		//printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> ELSE \n");
		left = -(lw/2);
	}

	if(shadow)
	{
		gint temp;

		temp = PANGO_SCALE;

		if(out_color)
			gdk_pango_renderer_set_override_color(GDK_PANGO_RENDERER(renderer), PANGO_RENDER_PART_FOREGROUND, out_color);
		else
			gdk_pango_renderer_set_override_color(GDK_PANGO_RENDERER(renderer), PANGO_RENDER_PART_FOREGROUND, &UX_COLOR(308));
		pango_renderer_draw_layout(renderer, layout, left-temp, top-temp);
		pango_renderer_draw_layout(renderer, layout, left+temp, top-temp);
		pango_renderer_draw_layout(renderer, layout, left+temp, top+temp);
		pango_renderer_draw_layout(renderer, layout, left-temp, top+temp);
		pango_renderer_draw_layout(renderer, layout, left-temp, top);
		pango_renderer_draw_layout(renderer, layout, left+temp, top);
		pango_renderer_draw_layout(renderer, layout, left, top-temp);
		pango_renderer_draw_layout(renderer, layout, left, top+temp);
	}

	gdk_pango_renderer_set_override_color(GDK_PANGO_RENDERER(renderer), PANGO_RENDER_PART_FOREGROUND, font_color);

	pango_renderer_draw_layout(renderer, layout, left, top);

	pango_font_description_free(desc);	// just moved here  
	gdk_pango_renderer_set_override_color(GDK_PANGO_RENDERER(renderer), PANGO_RENDER_PART_FOREGROUND, NULL);
	gdk_pango_renderer_set_drawable(GDK_PANGO_RENDERER(renderer), NULL);
	gdk_pango_renderer_set_gc(GDK_PANGO_RENDERER(renderer), NULL);


	g_object_unref(layout);
	g_object_unref(context);

	g_object_unref(pGc);
	pGc = NULL;

// skshin	
#if 0
	if(pango_pbuf)
	{
	   	g_hash_table_insert(pango_hash_table, g_strdup(strHashKey), pango_pbuf);
		gdk_draw_drawable(drawable, gc, pango_pbuf, 0, 0, x, y, w, h);

//#define	__MEMORY_PROBE__
#ifdef	__MEMORY_PROBE__
		static double pango_use_mem = 0.0;
		int pango_added_mem = 0;

		pango_added_mem = (w * h * 2);
		pango_use_mem += (pango_added_mem / 1024.0);

		DMSG(1, "util.c:%d [PANGO NEW CASHING...] 0x%x [%s]\tMEMORY[%d bytes / %.3fKB]\n", __LINE__, pango_pbuf, strHashKey, pango_added_mem, pango_use_mem);
#endif

		/* FOR LAYOUT SIZE */
		if(lsize_hash_table == NULL)
			lsize_hash_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

		memset(lsizeKey, 0, sizeof(lsizeKey));
		g_sprintf(lsizeKey, "%s_%s_%d", str, font, spacing_type);

		lsize = (struct LAYOUTSIZE *)g_hash_table_lookup(lsize_hash_table, lsizeKey);

		if(lsize == NULL)
		{
			lsize = g_malloc0(sizeof(struct LAYOUTSIZE));

			lsize->width = pw;
			lsize->height = ph;

			g_hash_table_insert(lsize_hash_table, g_strdup(lsizeKey), lsize);
		}
	}
#endif
}

gint nfutil_pango_get_spacing(nfutil_pango_spacing_type spacing_type)
{
  gint spacing_val = 0;
  
  switch(spacing_type)
  {
    case SEMI_CONDENSED_SPACING:
    {
      spacing_val = SEMI_CONDENSED_SPACING_GAP;
    }
    break;
    case CONDENSED_SPACING:
    {
      spacing_val = CONDENSED_SPACING_GAP;
    }
    break;
    case EXTRA_CONDENSED_SPACING:
    {
      spacing_val = EXTRA_CONDENSED_SPACING_GAP;
    }
    break;
    case ULTRA_CONDENSED_SPACING:
    {
      spacing_val = ULTRA_CONDENSED_SPACING_GAP;
    }
    break;
    default:
    break;
  }

  return spacing_val;
}

static void prvDrawTextWithPangoSpacing(gint conv, const gchar *key, GdkPixbuf *bgimg, 
                                        GdkColor *bgcol, GdkDrawable *drawable, 
                                        GdkGC* gc,  const gchar* str, 
                                        gint x, gint y, gint w, gint h, 
                                        const gchar *font, GdkColor *font_color, 
                                        nfalign_type halign, guint margin, 
                                        gint shadow, nfutil_pango_spacing_type spacing_type)
{
	GdkScreen *screen;
	PangoRenderer *renderer;
	PangoMatrix matrix = PANGO_MATRIX_INIT;
	PangoContext *context;
	PangoLayout *layout;
	PangoFontDescription *desc;
    PangoAttrList *attrs = NULL;
    PangoAttribute *attr = NULL;

	gchar strHashKey[NF_PANGO_CASHING_KEY_MAX];
	gchar lsizeKey[NF_PANGO_CASHING_KEY_MAX];

	guint lw, lh;
	gint left, top;
	guint pw, ph;

	gint nx, ny;

	gchar* p = NULL;

	GdkDrawable *pDrawable = NULL;
	GdkPixmap *pango_pbuf = NULL;
	GdkGC *pGc = NULL;
	struct LAYOUTSIZE *lsize = NULL;

	if(str == NULL || strlen(str) == 0)
		return;

	if(key)
	{
		if(pango_hash_table == NULL)
		{
			pango_hash_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
		}
		
		memset(strHashKey, 0, sizeof(strHashKey));
		g_sprintf(strHashKey, "%s_%s_%d%d_%s_#%x%x%x_%d_%d_%d", key, str, w, h, font, font_color->red, font_color->green, font_color->blue, halign, margin, shadow);

		pango_pbuf = (GdkPixmap *)g_hash_table_lookup(pango_hash_table, strHashKey);

		if(pango_pbuf)
		{
			g_assert(0);
//			gdk_draw_drawable(drawable, gc, pango_pbuf, 0, 0, x, y, w, h);
			return;
		}

//		pango_pbuf = gdk_pixmap_new(drawable, w, h, -1);
//		pGc = gdk_gc_new(pango_pbuf);
		pGc = gdk_gc_new(drawable);

		pDrawable = (GdkDrawable*)drawable;
//		pDrawable = (GdkDrawable*)pango_pbuf;
//		nx = 0;
//		ny = 0;
		nx = x;
		ny = y;
	}
	else
	{
		pDrawable = drawable;
		pGc = gdk_gc_new(drawable);
		nx = x;
		ny = y;
	}
	

	if(bgimg)
	{
		//gdk_draw_drawable(pDrawable, pGc, bgimg, 0, 0, nx, ny, w, h);
		gdk_draw_pixbuf(pDrawable, pGc, bgimg, 0, 0, nx, ny, w, h, GDK_RGB_DITHER_NORMAL, 0, 0);
	}
	else {
/*		if(bgcol)
		{
			gdk_gc_set_rgb_fg_color(pGc, bgcol);
			gdk_draw_rectangle(pDrawable, pGc, TRUE, nx, ny, w, h);
		}*/
	}



	if(conv)
		p = lookup_string(str);

	screen = gdk_drawable_get_screen(drawable);
	renderer = gdk_pango_renderer_get_default(screen);

	gdk_pango_renderer_set_drawable(GDK_PANGO_RENDERER(renderer), pDrawable);
	gdk_pango_renderer_set_gc(GDK_PANGO_RENDERER(renderer), pGc);

	context = gdk_pango_context_get_for_screen(screen);
	gdk_pango_context_set_colormap(context, gdk_rgb_get_colormap());

	layout = pango_layout_new(context);



	// added hmkong 
	//pango_layout_set_spacing(layout, (-4)*PANGO_SCALE);

	desc = pango_font_description_from_string(font);
	pango_layout_set_font_description(layout, desc);

  attrs = pango_layout_get_attributes(layout); 
  if(attrs == NULL)
  {
    attrs = pango_attr_list_new();
    pango_layout_set_attributes(layout, attrs);
    pango_attr_list_unref(attrs);
  }

  attr = pango_attr_letter_spacing_new(nfutil_pango_get_spacing(spacing_type));
  attr->start_index = 0;
  if(p==NULL)
  {
    attr->end_index = strlen(str)*sizeof(gchar);
  }
  else
  {
    attr->end_index = strlen(p)*sizeof(gchar);
  }
  pango_attr_list_change(attrs, attr);

#if 1
	if(p!=NULL)		pango_layout_set_text(layout, p, -1);
	else 			pango_layout_set_text(layout, str, -1);
#endif 

	pango_matrix_translate(&matrix, (float)(nx+w/2), (float)(ny+h/2));
	pango_context_set_matrix(context, &matrix);

	pango_layout_context_changed(layout);
	pango_layout_get_size(layout, &lw, &lh);
	pango_layout_get_pixel_size(layout, &pw, &ph);

	top = -(lh/2);

	if(halign == NFALIGN_LEFT)				left = (-(w/2) + margin) * PANGO_SCALE;
	else if(halign == NFALIGN_CENTER)		left = -(lw/2);
	else if(halign == NFALIGN_RIGHT)		left = -lw + (((w/2-margin)) * PANGO_SCALE);
	else if(halign == NFALIGN_CENTER_LEFT)	left = (nx+w/2-margin)*PANGO_SCALE - lw;
	else if(halign == NFALIGN_CENTER_RIGHT)	left = margin*PANGO_SCALE;
	else if(halign == NFALIGN_CENTER_DOWN){	left = -(lw/2); -lh + (((h/2-margin)) * PANGO_SCALE);} //wiggls - otmii
	else if(halign == NFALIGN_LEFT_UP){	left = (-(w/2) + margin) * PANGO_SCALE; top = (-(h/2) * PANGO_SCALE);}
	else 	left = -(lw/2);

	if(shadow)
	{
		gint temp;

		temp = PANGO_SCALE;

		gdk_pango_renderer_set_override_color(GDK_PANGO_RENDERER(renderer), PANGO_RENDER_PART_FOREGROUND, &UX_COLOR(308));
		pango_renderer_draw_layout(renderer, layout, left-temp, top-temp);
		pango_renderer_draw_layout(renderer, layout, left+temp, top-temp);
		pango_renderer_draw_layout(renderer, layout, left+temp, top+temp);
		pango_renderer_draw_layout(renderer, layout, left-temp, top+temp);
		pango_renderer_draw_layout(renderer, layout, left-temp, top);
		pango_renderer_draw_layout(renderer, layout, left+temp, top);
		pango_renderer_draw_layout(renderer, layout, left, top-temp);
		pango_renderer_draw_layout(renderer, layout, left, top+temp);
	}

	gdk_pango_renderer_set_override_color(GDK_PANGO_RENDERER(renderer), PANGO_RENDER_PART_FOREGROUND, font_color);
	pango_renderer_draw_layout(renderer, layout, left, top);

	pango_font_description_free(desc);	// just moved here  
	gdk_pango_renderer_set_override_color(GDK_PANGO_RENDERER(renderer), PANGO_RENDER_PART_FOREGROUND, NULL);
	gdk_pango_renderer_set_drawable(GDK_PANGO_RENDERER(renderer), NULL);
	gdk_pango_renderer_set_gc(GDK_PANGO_RENDERER(renderer), NULL);


//////
//////
       //pango_attribute_destroy(attr);
       //pango_attr_list_unref(attrs);


	g_object_unref(layout);
	g_object_unref(context);


//attr = NULL;
//attrs = NULL;


	g_object_unref(pGc);
	pGc = NULL;


// skshin
#if 0
	if(pango_pbuf)
	{
	   	g_hash_table_insert(pango_hash_table, g_strdup(strHashKey), pango_pbuf);
		gdk_draw_drawable(drawable, gc, pango_pbuf, 0, 0, x, y, w, h);

//#define	__MEMORY_PROBE__
#ifdef	__MEMORY_PROBE__
		static double pango_use_mem = 0.0;
		int pango_added_mem = 0;

		pango_added_mem = (w * h * 2);
		pango_use_mem += (pango_added_mem / 1024.0);

		DMSG(1, "##### util.c:%d [PANGO NEW CASHING...] 0x%x [%s]\tMEMORY[%d bytes / %.3fKB]\n", __LINE__, pango_pbuf, strHashKey, pango_added_mem, pango_use_mem);
#endif

		/* FOR LAYOUT SIZE */
		if(lsize_hash_table == NULL)
			lsize_hash_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

		memset(lsizeKey, 0, sizeof(lsizeKey));
		g_sprintf(lsizeKey, "%s_%s", str, font);

		lsize = (struct LAYOUTSIZE *)g_hash_table_lookup(lsize_hash_table, lsizeKey);

		if(lsize == NULL)
		{
			lsize = g_malloc0(sizeof(struct LAYOUTSIZE));

			lsize->width = pw;
			lsize->height = ph;

			g_hash_table_insert(lsize_hash_table, g_strdup(lsizeKey), lsize);
		}
	}
#endif
}




gint nfutil_is_text_cashed(const gchar *key)
{
	GdkPixbuf *pbuf = NULL;

	if(pango_hash_table == NULL)
		return 0;

	pbuf = (GdkPixbuf *)g_hash_table_lookup(pango_hash_table, key);

	if(pbuf)	return 1;
	
	return 0;
}

void nfutil_draw_text_with_pango(const gchar *key, GdkPixbuf *bgimg, GdkColor *bgcol, GdkDrawable *drawable, GdkGC* gc,  const gchar* str, gint x, gint y, gint w, gint h, const gchar *font, GdkColor *font_color, nfalign_type halign, guint margin)
{
	prvDrawTextWithPango(1, key, bgimg, bgcol, drawable, gc, str, 0, x, y, w, h, font, font_color, NULL, halign, margin, 0, NORMAL_SPACING);
}

void nfutil_draw_text_with_pango_outline(const gchar *key, GdkPixbuf *bgimg, GdkColor *bgcol, GdkDrawable *drawable, GdkGC* gc,  const gchar* str, gint x, gint y, gint w, gint h, const gchar *font, GdkColor *font_color, nfalign_type halign, guint margin)
{
	prvDrawTextWithPango(1, key, bgimg, bgcol, drawable, gc, str, 0, x, y, w, h, font, font_color, NULL, halign, margin, 1, NORMAL_SPACING);
}

void nfutil_draw_text_with_pango_eng(const gchar *key, GdkPixbuf *bgimg, GdkColor *bgcol, GdkDrawable *drawable, GdkGC* gc,  const gchar* str, gint x, gint y, gint w, gint h, const gchar *font, GdkColor *font_color, nfalign_type halign, guint margin)
{
	prvDrawTextWithPango(0, key, bgimg, bgcol, drawable, gc, str, 0, x, y, w, h, font, font_color, NULL, halign, margin, 0, NORMAL_SPACING);
}

void nfutil_draw_text_with_pango_outline_eng(const gchar *key, GdkPixbuf *bgimg, GdkColor *bgcol, GdkDrawable *drawable, GdkGC* gc,  const gchar* str, gint x, gint y, gint w, gint h, const gchar *font, GdkColor *font_color, nfalign_type halign, guint margin)
{
	prvDrawTextWithPango(0, key, bgimg, bgcol, drawable, gc, str, 0, x, y, w, h, font, font_color, NULL, halign, margin, 1, NORMAL_SPACING);
}


void nfutil_draw_text_with_pango_spacing(const gchar *key, GdkPixbuf *bgimg, GdkColor *bgcol, 
                                         GdkDrawable *drawable, GdkGC* gc,  
                                         const gchar* str, gint x, gint y, gint w, gint h, 
                                         const gchar *font, GdkColor *font_color, 
                                         nfalign_type halign, guint margin, nfutil_pango_spacing_type spacing_type)
{
	prvDrawTextWithPangoSpacing(1, key, bgimg, bgcol, drawable, gc, str, x, y, w, h, font, font_color, halign, margin, 0, spacing_type);
}

void nfutil_draw_text_with_pango_spacing_outline(const gchar *key, GdkPixbuf *bgimg, GdkColor *bgcol, 
                                                 GdkDrawable *drawable, GdkGC* gc,  
                                                 const gchar* str, gint x, gint y, gint w, gint h, 
                                                 const gchar *font, GdkColor *font_color, 
                                                 nfalign_type halign, guint margin, nfutil_pango_spacing_type spacing_type)
{
	prvDrawTextWithPangoSpacing(1, key, bgimg, bgcol, drawable, gc, str, x, y, w, h, font, font_color, halign, margin, 1, spacing_type);
}


void nfutil_draw_text_with_pango_spacing_eng(const gchar *key, GdkPixbuf *bgimg, GdkColor *bgcol, 
                                         GdkDrawable *drawable, GdkGC* gc,  
                                         const gchar* str, gint x, gint y, gint w, gint h, 
                                         const gchar *font, GdkColor *font_color, 
                                         nfalign_type halign, guint margin, nfutil_pango_spacing_type spacing_type)
{
	prvDrawTextWithPangoSpacing(0, key, bgimg, bgcol, drawable, gc, str, x, y, w, h, font, font_color, halign, margin, 0, spacing_type);
}

void nfutil_draw_text_with_pango_spacing_outline_eng(const gchar *key, GdkPixbuf *bgimg, GdkColor *bgcol, 
                                                 GdkDrawable *drawable, GdkGC* gc,  
                                                 const gchar* str, gint x, gint y, gint w, gint h, 
                                                 const gchar *font, GdkColor *font_color, 
                                                 nfalign_type halign, guint margin, nfutil_pango_spacing_type spacing_type)
{
	prvDrawTextWithPangoSpacing(0, key, bgimg, bgcol, drawable, gc, str, x, y, w, h, font, font_color, halign, margin, 1, spacing_type);
}

void nfutil_draw_short_text(const gchar *key, GdkPixbuf *bgimg, GdkColor *bgcol, GdkDrawable *drawable, GdkGC* gc, const gchar* str, gint valid_cnt,
							gint x, gint y, gint w, gint h, const gchar *font, GdkColor *font_color, GdkColor *out_color, nfalign_type halign, guint margin, gint shadow, nfutil_pango_spacing_type spacing_type)
{
	prvDrawTextWithPango(1, key, bgimg, bgcol, drawable, gc, str, valid_cnt, x, y, w, h, font, font_color, out_color, halign, margin, shadow, spacing_type);
}

void nfutil_draw_short_text_eng(const gchar *key, GdkPixbuf *bgimg, GdkColor *bgcol, GdkDrawable *drawable, GdkGC* gc, const gchar* str, gint valid_cnt,
							gint x, gint y, gint w, gint h, const gchar *font, GdkColor *font_color, GdkColor *out_color, nfalign_type halign, guint margin, gint shadow, nfutil_pango_spacing_type spacing_type)
{
	prvDrawTextWithPango(0, key, bgimg, bgcol, drawable, gc, str, valid_cnt, x, y, w, h, font, font_color, out_color, halign, margin, shadow, spacing_type);
}

void nfutil_draw_short_text_vk(const gchar *key, GdkPixbuf *bgimg, GdkColor *bgcol, GdkDrawable *drawable, GdkGC* gc, const gchar* str, gint valid_cnt,
							gint x, gint y, gint w, gint h, const gchar *font, GdkColor *font_color, GdkColor *out_color, nfalign_type halign, guint margin, gint shadow, nfutil_pango_spacing_type spacing_type)
{
	prvDrawTextWithPangoVk(1, key, bgimg, bgcol, drawable, gc, str, valid_cnt, x, y, w, h, font, font_color, out_color, halign, margin, shadow, spacing_type);
}

void nfutil_draw_short_text_eng_vk(const gchar *key, GdkPixbuf *bgimg, GdkColor *bgcol, GdkDrawable *drawable, GdkGC* gc, const gchar* str, gint valid_cnt,
							gint x, gint y, gint w, gint h, const gchar *font, GdkColor *font_color, GdkColor *out_color, nfalign_type halign, guint margin, gint shadow, nfutil_pango_spacing_type spacing_type)
{
	prvDrawTextWithPangoVk(0, key, bgimg, bgcol, drawable, gc, str, valid_cnt, x, y, w, h, font, font_color, out_color, halign, margin, shadow, spacing_type);
}

static gint prvGetStringSize(gint conv, GdkDrawable *drawable, const gchar *str, const gchar *font, gint *width, gint *height, nfutil_pango_spacing_type spacing_type)
{
//	struct LAYOUTSIZE *lsize = NULL;
//	gchar lsizeKey[NF_PANGO_CASHING_KEY_MAX];
	gint str_len = -1;

//	if(lsize_hash_table == NULL)
//		lsize_hash_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

//	memset(lsizeKey, 0, sizeof(lsizeKey));
//	g_sprintf(lsizeKey, "%s_%s_%d", str, font, spacing_type);
	
//	lsize = (struct LAYOUTSIZE *)g_hash_table_lookup(lsize_hash_table, lsizeKey);

//	if(lsize == NULL)
	{
		gchar *p = NULL;
		gchar *utf_str = NULL;
		GdkScreen *screen;
		PangoMatrix matrix = PANGO_MATRIX_INIT;
		PangoContext *context;
		PangoLayout *layout;
		PangoFontDescription *desc;

		PangoAttrList *attrs = NULL;
		PangoAttribute *attr = NULL;

		gint pw, ph;

		// don't change to imalloc
//		lsize = (struct LAYOUTSIZE*)g_malloc0(sizeof(struct LAYOUTSIZE));

		if(conv)
			p = lookup_string(str);

		screen = gdk_drawable_get_screen(drawable);
		context = gdk_pango_context_get_for_screen(screen);

		layout = pango_layout_new(context);

		//pango_layout_set_spacing(layout, (-4)*PANGO_SCALE);
	
		desc = pango_font_description_from_string(font);
		pango_layout_set_font_description(layout, desc);
    
		if(p!=NULL)
		{
			utf_str = p;
			str_len = g_utf8_strlen(p, -1);
		}
		else
		{
			utf_str = str;
			str_len = strlen(str);
		}

		if(spacing_type != NORMAL_SPACING)
		{
			attrs = pango_layout_get_attributes(layout); 
			if(attrs == NULL)
			{
				attrs = pango_attr_list_new();
				pango_layout_set_attributes(layout, attrs);
				pango_attr_list_unref(attrs);
			}

			attr = pango_attr_letter_spacing_new(nfutil_pango_get_spacing(spacing_type));
			attr->start_index = 0;
			attr->end_index = strlen(utf_str)*sizeof(gchar);

			pango_attr_list_change(attrs, attr);
		}

		pango_layout_set_text(layout, utf_str, -1);

		pango_context_set_matrix(context, &matrix);

		pango_layout_context_changed(layout);
		pango_layout_get_pixel_size(layout, &pw, &ph);

//		lsize->width = pw;
//		lsize->height = ph;
		*width = pw;
		*height = ph;
		
 //   	g_hash_table_insert(lsize_hash_table, g_strdup(lsizeKey), lsize);

		pango_font_description_free(desc);
		g_object_unref(layout);
		g_object_unref(context);
	}

//	*width = lsize->width;
//	*height = lsize->height;

	return str_len;
}

static void prvGetSpacingStringSize(gint conv, GdkDrawable *drawable, const gchar *str, const gchar *font, gint *width, gint *height, nfutil_pango_spacing_type spacing_type)
{
//	struct LAYOUTSIZE *lsize = NULL;
//	gchar lsizeKey[NF_PANGO_CASHING_KEY_MAX];
  PangoAttrList *attrs = NULL;
  PangoAttribute *attr = NULL;


//	if(lsize_hash_table == NULL)
//		lsize_hash_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

//	memset(lsizeKey, 0, sizeof(lsizeKey));
//	g_sprintf(lsizeKey, "%s_%s", str, font);
		
//	lsize = (struct LAYOUTSIZE *)g_hash_table_lookup(lsize_hash_table, lsizeKey);

//	if(lsize == NULL)
	{
		gchar *p = NULL;
		GdkScreen *screen;
		PangoMatrix matrix = PANGO_MATRIX_INIT;
		PangoContext *context;
		PangoLayout *layout;
		PangoFontDescription *desc;

		gint pw, ph;

		// don't change to imalloc
//		lsize = (struct LAYOUTSIZE*)g_malloc0(sizeof(struct LAYOUTSIZE));

		if(conv)
			p = lookup_string(str);
		screen = gdk_drawable_get_screen(drawable);
		context = gdk_pango_context_get_for_screen(screen);

		layout = pango_layout_new(context);

		//pango_layout_set_spacing(layout, (-4)*PANGO_SCALE);
	
		desc = pango_font_description_from_string(font);
		pango_layout_set_font_description(layout, desc);

    if(spacing_type != NORMAL_SPACING)
    {
      attrs = pango_layout_get_attributes(layout); 
      if(attrs == NULL)
      {
        attrs = pango_attr_list_new();
        pango_layout_set_attributes(layout, attrs);
        pango_attr_list_unref(attrs);
      }

      attr = pango_attr_letter_spacing_new(nfutil_pango_get_spacing(spacing_type));
      attr->start_index = 0;
      attr->end_index = strlen(str)*sizeof(gchar);
      pango_attr_list_change(attrs, attr);
    }



		if(p!=NULL)		pango_layout_set_text(layout, p, -1);
		else 			pango_layout_set_text(layout, str, -1);

		pango_context_set_matrix(context, &matrix);

		pango_layout_context_changed(layout);
		pango_layout_get_pixel_size(layout, &pw, &ph);

//		lsize->width = pw;
//		lsize->height = ph;
		*width = pw;
		*height = ph;
		
 //   	g_hash_table_insert(lsize_hash_table, g_strdup(lsizeKey), lsize);

		g_object_unref(layout);
		g_object_unref(context);
	}

//	*width = lsize->width;
//	*height = lsize->height;
}

static void prvGetNotCashingStringSize(gint conv, GdkDrawable *drawable, const gchar *str, const gchar *font, gint *width, gint *height)
{		
    gchar *p = NULL;
    GdkScreen *screen;
    PangoMatrix matrix = PANGO_MATRIX_INIT;
    PangoContext *context;
    PangoLayout *layout;
    PangoFontDescription *desc;

    gint pw, ph;

    if(conv)
    	p = lookup_string(str);


    screen = gdk_drawable_get_screen(drawable);
    context = gdk_pango_context_get_for_screen(screen);

    layout = pango_layout_new(context);

    //pango_layout_set_spacing(layout, (-4)*PANGO_SCALE);

    desc = pango_font_description_from_string(font);
    pango_layout_set_font_description(layout, desc);
          pango_font_description_free(desc);

    if(p!=NULL)		pango_layout_set_text(layout, p, -1);
    else 			pango_layout_set_text(layout, str, -1);

    pango_context_set_matrix(context, &matrix);

    pango_layout_context_changed(layout);
    pango_layout_get_pixel_size(layout, &pw, &ph);

    g_object_unref(layout);
    g_object_unref(context);

    *width = pw;
    *height =ph;
}


guint nfutil_string_width(gint conv, GdkDrawable *drawable, const gchar *font, const gchar *str, nfutil_pango_spacing_type spacing_type)
{
	GdkPixmap *pbuf;

	gint width;
	gint height;

	if(drawable == NULL)
	{
		GdkWindow *rootwin;
		rootwin = gdk_get_default_root_window();
		pbuf = gdk_pixmap_new(rootwin, 10, 10, -1);
		prvGetStringSize(conv, pbuf, str, font, &width, &height, spacing_type);
		g_object_unref(pbuf);
	}
	else
	{
		prvGetStringSize(conv, drawable, str, font, &width, &height, spacing_type);
	}

	return width;
}




guint nfutil_not_cashing_string_width(gint conv, GdkDrawable *drawable, const gchar *font, const gchar *str)
{
	GdkPixmap *pbuf;

	gint width = 0;
	gint height;


	if(drawable == NULL)
	{
		GdkWindow *rootwin;
	       g_message("<%s><%d>", __FUNCTION__, __LINE__);
		rootwin = gdk_get_default_root_window();   
		pbuf = gdk_pixmap_new(rootwin, 10, 10, -1);
		prvGetNotCashingStringSize(conv, pbuf, str, font, &width, &height);
		g_object_unref(pbuf);
	}
	else
	{
		prvGetNotCashingStringSize(conv, drawable, str, font, &width, &height);
	}

	return width;
}



guint nfutil_spacing_string_width(GdkDrawable *drawable, const gchar *font, const gchar *str, nfutil_pango_spacing_type spacing_type)
{
	GdkPixmap *pbuf;

	gint width;
	gint height;

  width = nfutil_string_width(0, drawable, font, str, spacing_type);

  switch(spacing_type)
  {
    case EXTRA_CONDENSED_SPACING:
    case ULTRA_CONDENSED_SPACING:    
    {
      width = width - 1;
    }
    break;
    default:
    break;
  }
#if 0
	if(drawable == NULL)
	{
		pbuf = gdk_pixmap_new(NULL, 10, 10, 16);
		prvGetSpacingStringSize(1, pbuf, str, font, &width, &height, spacing_type);
		g_object_unref(pbuf);
	}
	else
	{
		prvGetSpacingStringSize(1, drawable, str, font, &width, &height, spacing_type);
	}
#endif

  if(width < 0)
  {
    width = 0;
  }
	return width;
}

guint nfutil_string_height(GdkDrawable *drawable, const gchar *font, const gchar *str, nfutil_pango_spacing_type spacing_type)
{
	GdkPixmap *pbuf;

	gint width;
	gint height;

	if(drawable == NULL)
	{
		GdkWindow *rootwin;
		rootwin = gdk_get_default_root_window();
		pbuf = gdk_pixmap_new(rootwin, 10, 10, -1);
		prvGetStringSize(1, pbuf, str, font, &width, &height, spacing_type);
		g_object_unref(pbuf);
	}
	else
	{
		prvGetStringSize(1, drawable, str, font, &width, &height, spacing_type);
	}

	return height;
}

static void prvGetUtfStringInfo(gint conv, GdkDrawable *drawable, const gchar *str, const gchar *font, gint *length, gint *width, gint *height)
{
	gint str_len = -1;
	gint utf_wid, utf_hei;

	GdkScreen *screen;
	PangoMatrix matrix = PANGO_MATRIX_INIT;
	PangoContext *context;
	PangoLayout *layout;
	PangoFontDescription *desc;

	gchar *utf_str = NULL;

	if(conv)
		utf_str = lookup_string(str);

	if(utf_str == NULL)
		utf_str = str;


	screen = gdk_drawable_get_screen(drawable);
	context = gdk_pango_context_get_for_screen(screen);

	layout = pango_layout_new(context);

	//pango_layout_set_spacing(layout, (-4)*PANGO_SCALE);
	
	desc = pango_font_description_from_string(font);
	pango_layout_set_font_description(layout, desc);

	pango_layout_set_text(layout, utf_str, -1);
	str_len = g_utf8_strlen(utf_str, -1);

	pango_context_set_matrix(context, &matrix);

	pango_layout_context_changed(layout);
	pango_layout_get_pixel_size(layout, &utf_wid, &utf_hei);

	pango_font_description_free(desc);
	g_object_unref(layout);
	g_object_unref(context);

	// RESULT
	if(length)	*length = str_len;
	if(width)	*width = utf_wid;
	if(height)	*height = utf_hei;
}

gint nfutil_string_get_valid_count(gint conv, GdkDrawable *drawable, const gchar *font, const gchar *str, gint drawing_width)
{
	gchar *lang_str = NULL;
	gint length, width;	// of lang_str
	gint len2, wid2;
	gint concat_width;
	gint limit;

	gdouble rate;
	gchar strTemp[1024];

	gint flag = 0;

	if(drawing_width <= 0)
		return 0;

	if(conv)
		lang_str = lookup_string(str);

	if(lang_str == NULL)
		lang_str = str;

	prvGetUtfStringInfo(conv, drawable, lang_str, font, &length, &width, NULL);

	if(width <= drawing_width)
		return 0;


	concat_width = nfutil_string_width(conv, drawable, font, "...", NORMAL_SPACING);
	limit = drawing_width - concat_width;

	if(limit <= 0 || width <= 0)
		return 0;

	rate = (gdouble)limit / (gdouble)width;
	len2 = length * rate;

	if(len2 == 0)
		return 0;

	while(1)
	{
		memset(strTemp, 0, sizeof(strTemp));
		g_utf8_strncpy(strTemp, lang_str, len2);
		prvGetUtfStringInfo(conv, drawable, strTemp, font, NULL, &width, NULL);

		if(width > limit)
		{
			flag = 1;
			break;
		}
		else if(width >= limit - AVG_CHAR_WIDTH)
		{
			flag = 0;
			break;
		}

		len2++;
	}

	if(flag == 0)
		return len2;

	while(1)
	{
		len2--;

		memset(strTemp, 0, sizeof(strTemp));
		g_utf8_strncpy(strTemp, lang_str, len2);
		prvGetUtfStringInfo(conv, drawable, strTemp, font, NULL, &width, NULL);

		if(width <= limit)
			break;
	}

	return len2;
}

gint nfutil_string_get_valid_count2(gint conv, GdkDrawable *drawable, const gchar *font, const gchar *str, gint drawing_width)
{
	gchar *lang_str = NULL;
	gint length, width;	// of lang_str
	gint len2, wid2;

	gdouble rate;
	gchar strTemp[1024];

	gint flag = 0;

	if(drawing_width <= 0)
		return 0;

	if(conv)
		lang_str = lookup_string(str);

	if(lang_str == NULL)
		lang_str = str;

	prvGetUtfStringInfo(conv, drawable, lang_str, font, &length, &width, NULL);

	if(width <= drawing_width)
		return 0;

	if(width <= 0)
		return 0;

	rate = (gdouble)drawing_width / (gdouble)width;
	len2 = length * rate;

	if(len2 == 0)
		return 0;

	while(1)
	{
		memset(strTemp, 0, sizeof(strTemp));
		g_utf8_strncpy(strTemp, lang_str, len2);
		prvGetUtfStringInfo(conv, drawable, strTemp, font, NULL, &width, NULL);

		if(width > drawing_width)
		{
			flag = 1;
			break;
		}
		else if(width >= drawing_width - AVG_CHAR_WIDTH)
		{
			flag = 0;
			break;
		}

		len2++;
	}

	if(flag == 0)
		return len2;

	while(1)
	{
		len2--;

		memset(strTemp, 0, sizeof(strTemp));
		g_utf8_strncpy(strTemp, lang_str, len2);
		prvGetUtfStringInfo(conv, drawable, strTemp, font, NULL, &width, NULL);

		if(width <= drawing_width)
			break;
	}

	return len2;
}

gint nfutil_string_get_valid_count_word(gint conv, GdkDrawable *drawable, const gchar *font, const gchar *str, gint drawing_width)
{
	gchar *lang_str = NULL;
	gchar *p, *op = 0;
	gint length, width;	// of lang_str
	gint len2 = 0;

	gchar *strTmp;

	if (!nftool_cur_language_is_eng()) return -1;
	if (drawing_width <= 0) return 0;

	if (conv) lang_str = lookup_string(str);
	if (lang_str == NULL) lang_str = str;

	prvGetUtfStringInfo(conv, drawable, lang_str, font, &length, &width, NULL);

	if (width <= drawing_width) return 0;
	if (width <= 0) return 0;

	p = strchr(lang_str, ' ');
	if (!p) {
		prvGetUtfStringInfo(conv, drawable, lang_str, font, NULL, &width, NULL);
		if (width <= drawing_width) return 0;
		else return -1;
	}

	while (p != 0)
	{
		strTmp = g_strndup(lang_str, p-lang_str+1);
		prvGetUtfStringInfo(conv, drawable, strTmp, font, NULL, &width, NULL);
		g_free(strTmp);
		if (width > drawing_width) break;

		op = p;
		p = strchr(p+1, ' ');
	}
	if (!op) return -1;

	return op-lang_str+1;
}

gint nfutil_get_line_feed_string(gchar *in_str, gint layout_w, const gchar *font, gchar *out_str, gint out_str_size)
{
	GdkPixmap *pbuf;
	gchar *p, *strTmp, *pf, *out_str_sp;
	gint valid_cnt ;

	GdkWindow *rootwin;
	rootwin = gdk_get_default_root_window();
	pbuf = gdk_pixmap_new(rootwin, 10, 10, -1);

	memset(out_str, 0x00, out_str_size);
	p = in_str;
	out_str_sp = out_str;

	while (1)
	{	
		pf = strchr(p, '\n');

		if (pf) strTmp = g_strndup(p, pf-p+1);
		else strTmp = g_strndup(p, strlen(p));

		valid_cnt = nfutil_string_get_valid_count_word(1, pbuf, font, strTmp, layout_w);
		if (valid_cnt == -1) {
		valid_cnt = nfutil_string_get_valid_count2(1, pbuf, font, strTmp, layout_w);
		}

		if (valid_cnt)
		{
			g_utf8_strncpy(out_str, strTmp, valid_cnt);
			p += strlen(out_str);

			strcat(out_str, "\n");
		}
		else
		{
			g_utf8_strncpy(out_str, strTmp, strlen(strTmp));
			p += strlen(out_str);
		}
	
		g_free(strTmp);

		if (strlen(out_str_sp) >= out_str_size) {
		    out_str_sp[out_str_size -1] = '\0';
		    break;
		}

		if ((pf == 0) && (valid_cnt == 0)) break;

		out_str += strlen(out_str);
	}

	g_object_unref(pbuf);

	return 0;
}

gint nfutil_get_line_feed_string_delimiter(gchar *in_str, gint layout_w, gchar *out_str, gint out_str_size, gchar delimiter)
{
	gchar *p, *tmp;

	p = in_str;

	while(1)
	{
		tmp = strchr(p, delimiter);

		if (!tmp) break;

		*tmp = '\n';
		p = tmp+1;
	}

	nfutil_get_line_feed_string(in_str, layout_w, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), out_str, out_str_size);	
	return 0;
}

/*************************************************************************************
 * DST Time
 * ***********************************************************************************/

static void prvStTimeSubtractHMS(struct tm *stTime, gint hour, gint min, gint sec)
{
	gint day=0;

	if(stTime == NULL)	return;

	if(sec)
	{
		if(stTime->tm_sec >= sec)
			stTime->tm_sec -= sec;
		else
		{
			stTime->tm_sec -= (sec-60);
			min++;
		}
	}

	if(min)
	{
		if(stTime->tm_min >= min)
			stTime->tm_min -= min;
		else
		{
			stTime->tm_min -= (min-60);
			hour++;
		}
	}

	if(hour)
	{
		if(stTime->tm_hour >= hour)
			stTime->tm_hour -= hour;
		else
		{
			stTime->tm_hour -= (hour-24);
			day++;
		}
	}

	if(day)
	{
		GDate *date = NULL;

		date = g_date_new();

		g_date_set_dmy(date, stTime->tm_mday, stTime->tm_mon+1, stTime->tm_year+1900);

		g_date_subtract_days(date, day);

		stTime->tm_year = g_date_get_year(date) - 1900;
		stTime->tm_mon = g_date_get_month(date) - 1;
		stTime->tm_mday = g_date_get_day(date);

		g_date_free(date);
		date = NULL;
	}
}


time_t GET_DST_INTERVAL()
{
	GTimeVal tvCur;

	struct tm stCur;

	time_t tReal;
	time_t tDST;

	time_t interval = 0;

	gint i;


	g_get_current_time(&tvCur);
	stCur = *LOCALTIME_R(&(tvCur.tv_sec));

	if(stCur.tm_isdst)
	{
		tDST = tvCur.tv_sec;
		
		stCur.tm_isdst = 0;
		tReal = mktime(&stCur);

		interval = tReal - tDST;

		return interval;
	}


	for(i=0; i<12; i++)
	{
		stCur.tm_mon = i;

		tDST = mktime(&stCur);
		stCur = *LOCALTIME_R(&tDST);

		if(stCur.tm_isdst)
			break;
	}

	if(i>=12)
	{
		g_warning("It is not used Daylight Saving Time Rule at current locale..");
		return 0;
	}

	stCur.tm_isdst = 0;
	tReal = mktime(&stCur);

	interval = tReal - tDST;

	return interval;
}

struct tm* NFLOCALTIME(time_t *ttime)
{
	static struct tm stTime;
	gint db_isdst;

	memset(&stTime, 0, sizeof(struct tm));

	stTime = *LOCALTIME_R(ttime);

	if(stTime.tm_isdst)
	{
		struct tm stTime_temp;
		time_t ttime_temp = 0;
		time_t real_time = 0;

		db_isdst = DAL_get_dst();

		if(!db_isdst)
		{
			time_t time_gab=0;
			gint hour=0, min=0, sec=0, temp=0;

			ttime_temp = *ttime;
			stTime_temp = stTime;

			stTime_temp.tm_isdst = 0;

			real_time = mktime(&stTime_temp);

			time_gab = real_time - ttime_temp;

			hour = time_gab / 3600;
			temp = time_gab % 3600;

			if(temp)
			{
				min = temp / 60;
				temp %= 60;

				sec = temp;
			}

			prvStTimeSubtractHMS(&stTime, hour, min, sec);
			stTime.tm_isdst = 0;
		}
	}

	return &stTime;
}


struct tm *LOCALTIME_R(time_t *ttime)
{
//	struct tm stTime;		//2010.02.25. bugpatch : parangi. local variable return.
	static struct tm stTime;


    memset(&stTime, 0x00, sizeof(stTime));
    localtime_r(ttime, &stTime);
    return &stTime;
}

struct tm* NFLOCALTIME_TV(GTimeVal tv)
{
	static struct tm *ret;

	ret = NFLOCALTIME(&(tv.tv_sec));

	return ret;
}

time_t NFMKTIME(struct tm *stime, gint isdst)
{
	time_t ret;
	struct tm temp;

	temp = *stime;

	temp.tm_isdst = isdst;
	ret = mktime(&temp);

	return ret;
}


//static gint prvGetStartTime(GTimeVal *tv)
gint nfutil_get_start_time(GTimeVal *tv)
{
	GTimeVal tv_temp;
	struct tm stTime1;
	gint dst_day;

	tv->tv_usec = 0;

	memset(&tv_temp, 0, sizeof(GTimeVal));
	memset(&stTime1, 0, sizeof(struct tm));

	tv_temp = *tv;
	stTime1 = *NFLOCALTIME(&(tv_temp.tv_sec));

	dst_day = nfutil_check_DST_day(tv_temp); 

	switch(dst_day)
	{
		case DST_START_DAY:
		case DST_END_DAY:
			if(!(stTime1.tm_isdst))
				tv_temp.tv_sec -=  3600;
			break;

		default:
			break;
	}

	tv_temp.tv_sec -= (stTime1.tm_hour*3600);
	tv_temp.tv_sec -= (stTime1.tm_min)*60;
	tv_temp.tv_sec -= stTime1.tm_sec;

	*tv = tv_temp;

	return dst_day;
}

//static void prvGetStartDateTime(GTimeVal *tv)
void nfutil_get_start_datetime(GTimeVal *tv)
{
	GTimeVal tv_temp;
	time_t ttime;
	struct tm stTime1;
	struct tm stTime2;
	gint prev_mon;
	gint i;

	tv->tv_usec = 0;

	memset(&tv_temp, 0, sizeof(GTimeVal));
	memset(&stTime1, 0, sizeof(struct tm));
	memset(&stTime2, 0, sizeof(struct tm));

	ttime = tv->tv_sec;
	stTime1 = *NFLOCALTIME(&ttime);

	ttime -= (86400*(stTime1.tm_mday-1));
	stTime2 = *NFLOCALTIME(&ttime);

	if(stTime1.tm_mon != stTime2.tm_mon)
		ttime += 3600;

	tv_temp.tv_sec = ttime;
	stTime2 = *NFLOCALTIME(&ttime);

	switch(nfutil_check_DST_day(tv_temp))
	{
		case DST_NORMAL_DAY:
		case DST_DST_DAY:
			if(stTime2.tm_mday > 1)
				ttime -=  86400;
			break;

		case DST_START_DAY:
			if(stTime2.tm_isdst)
			{
				if(stTime2.tm_mday > 1)
					ttime -= 86400;
				ttime += 3600;
			}
			else
			{
				if(stTime2.tm_mday > 1)
					ttime -=  86400;
			}
			break;

		case DST_END_DAY:
			if(!(stTime2.tm_isdst))
				ttime -= 3600;
			break;

		default:
			break;
	}

	ttime -= (stTime2.tm_hour*3600);
	ttime -= (stTime2.tm_min)*60;
	ttime -= stTime2.tm_sec;

	tv->tv_sec = ttime;
}

//static gint prvCheckDSTDay(GTimeVal tv)
gint nfutil_check_DST_day(GTimeVal tv)
{
	gint ret = DST_NORMAL_DAY;

	time_t ttime;
	struct tm stTime;
	struct tm stPrev;

	gint cnt = 0;
	gint i;

	ttime = tv.tv_sec;
	stTime = *NFLOCALTIME(&ttime);

	i = stTime.tm_hour;

	cnt = 0;
	stPrev = stTime;
	ttime -= ((i+1)*3600);

	while(1)
	{
		stTime = *NFLOCALTIME(&ttime);

		if(stPrev.tm_mday == stTime.tm_mday)
			break;
		
		ttime += 3600;
	}

	while(1)
	{
		stTime = *NFLOCALTIME(&ttime);

		if(stPrev.tm_mday == stTime.tm_mday)
			cnt++;
		else
			break;

		ttime += 3600;
	}

	if(cnt == 24)
	{
		if(stTime.tm_isdst == 0)		ret = DST_NORMAL_DAY;
		else if(stTime.tm_isdst == 1)	ret = DST_DST_DAY;
		else	ret = -1;
	}
	else if(cnt < 24)	ret = DST_START_DAY;
	else if(cnt > 24)	ret = DST_END_DAY;
	else	ret = -1;

	return ret;
}



/******************************************************************
 * Device Mount / Unmount
 * ****************************************************************/

#define	MOUNT_TRY_CNT	(5)

#if 0
// not using in IPX
//
//
gboolean nfutil_device_mount(const gchar *device_name, const gchar *mnt_path, const gchar *fs, unsigned long flag)
{
	gint ret;
	gint try;

	rmdir(mnt_path);
	mkdir("/mnt", 644);

	for(try=0; try<MOUNT_TRY_CNT; try++)
	{
		if(mkdir(mnt_path, 644) == 0)
			break;
	}

	for(try=0; try<MOUNT_TRY_CNT; try++)
	{
		ret = mount(device_name, mnt_path, fs, flag, NULL);

		if(!ret)
		{
			return TRUE;
		}
		else
		{
			gint m_error = errno;

			if(m_error == EBUSY)
			{
				return TRUE;
			}
		}
	}

	if(try==MOUNT_TRY_CNT)
	{
		perror("mount");
		g_warning("DEVICE MOUNT ERROR!! Device name[%s], Mount directory[%s] ret[%d]", device_name, mnt_path, ret);

		rmdir(mnt_path);

		return FALSE;
	}

	return TRUE;
}


gboolean nfutil_device_unmount(const gchar *device_name, const gchar *mnt_path)
{
	if(umount(mnt_path) != 0)
	{
		g_warning("UNMOUNT ERROR... Retry by dev_name..");
		umount(device_name);
	}

	rmdir(mnt_path);

	return TRUE;
}

#endif
